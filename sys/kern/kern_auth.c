/* $NetBSD: kern_auth.c,v 1.32.2.4 2007/01/07 10:51:15 bouyer Exp $ */

/*-
 * Copyright (c) 2005, 2006 Elad Efrat <elad@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_auth.c,v 1.32.2.4 2007/01/07 10:51:15 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/pool.h>
#include <sys/kauth.h>
#include <sys/acct.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>

/* 
 * Credentials.
 */
struct kauth_cred {
	struct simplelock cr_lock;	/* lock on cr_refcnt */
	u_int cr_refcnt;		/* reference count */
	uid_t cr_uid;			/* user id */
	uid_t cr_euid;			/* effective user id */
	uid_t cr_svuid;			/* saved effective user id */
	gid_t cr_gid;			/* group id */
	gid_t cr_egid;			/* effective group id */
	gid_t cr_svgid;			/* saved effective group id */
	u_int cr_ngroups;		/* number of groups */
	gid_t cr_groups[NGROUPS];	/* group memberships */
};

/*
 * Listener.
 */
struct kauth_listener {
	kauth_scope_callback_t		func;		/* callback */
	kauth_scope_t			scope;		/* scope backpointer */
	u_int				refcnt;		/* reference count */
	SIMPLEQ_ENTRY(kauth_listener)	listener_next;	/* listener list */
};

/*
 * Scope.
 */
struct kauth_scope {
	const char		       *id;		/* scope name */
	void			       *cookie;		/* user cookie */
	u_int				nlisteners;	/* # of listeners */
	SIMPLEQ_HEAD(, kauth_listener)	listenq;	/* listener list */
	SIMPLEQ_ENTRY(kauth_scope)	next_scope;	/* scope list */
};

static POOL_INIT(kauth_cred_pool, sizeof(struct kauth_cred), 0, 0, 0,
    "kauthcredpl", &pool_allocator_nointr);

/* List of scopes and its lock. */
static SIMPLEQ_HEAD(, kauth_scope) scope_list;
static struct simplelock scopes_lock;

/* Built-in scopes: generic, process. */
static kauth_scope_t kauth_builtin_scope_generic;
static kauth_scope_t kauth_builtin_scope_system;
static kauth_scope_t kauth_builtin_scope_process;
static kauth_scope_t kauth_builtin_scope_network;
static kauth_scope_t kauth_builtin_scope_machdep;
static kauth_scope_t kauth_builtin_scope_device;

static boolean_t listeners_have_been_loaded = FALSE;

/* Allocate new, empty kauth credentials. */
kauth_cred_t
kauth_cred_alloc(void)
{
	kauth_cred_t cred;

	cred = pool_get(&kauth_cred_pool, PR_WAITOK);
	memset(cred, 0, sizeof(*cred));
	simple_lock_init(&cred->cr_lock);
	cred->cr_refcnt = 1;

	return (cred);
}

/* Increment reference count to cred. */
void
kauth_cred_hold(kauth_cred_t cred)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt > 0);

        simple_lock(&cred->cr_lock);
        cred->cr_refcnt++;
        simple_unlock(&cred->cr_lock);
}

/* Decrease reference count to cred. If reached zero, free it. */
void
kauth_cred_free(kauth_cred_t cred)
{
	u_int refcnt;

	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt > 0);

	simple_lock(&cred->cr_lock);
	refcnt = --cred->cr_refcnt;
	simple_unlock(&cred->cr_lock);

	if (refcnt == 0)
		pool_put(&kauth_cred_pool, cred);
}

void
kauth_cred_clone(kauth_cred_t from, kauth_cred_t to)
{
	KASSERT(from != NULL);
	KASSERT(to != NULL);
	KASSERT(from->cr_refcnt > 0);

	to->cr_uid = from->cr_uid;
	to->cr_euid = from->cr_euid;
	to->cr_svuid = from->cr_svuid;
	to->cr_gid = from->cr_gid;
	to->cr_egid = from->cr_egid;
	to->cr_svgid = from->cr_svgid;
	to->cr_ngroups = from->cr_ngroups;
	memcpy(to->cr_groups, from->cr_groups, sizeof(to->cr_groups));
}

/*
 * Duplicate cred and return a new kauth_cred_t.
 */
kauth_cred_t
kauth_cred_dup(kauth_cred_t cred)
{
	kauth_cred_t new_cred;

	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt > 0);

	new_cred = kauth_cred_alloc();

	kauth_cred_clone(cred, new_cred);

	return (new_cred);
}

/*
 * Similar to crcopy(), only on a kauth_cred_t.
 * XXX: Is this even needed? [kauth_cred_copy]
 */
kauth_cred_t
kauth_cred_copy(kauth_cred_t cred)
{
	kauth_cred_t new_cred;

	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt > 0);

	/* If the provided credentials already have one reference, use them. */
	if (cred->cr_refcnt == 1)
		return (cred);

	new_cred = kauth_cred_alloc();

	kauth_cred_clone(cred, new_cred);

	kauth_cred_free(cred);

	return (new_cred);
}

uid_t
kauth_cred_getuid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_uid);
}

uid_t
kauth_cred_geteuid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_euid);
}

uid_t
kauth_cred_getsvuid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_svuid);
}

gid_t
kauth_cred_getgid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_gid);
}

gid_t
kauth_cred_getegid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_egid);
}

gid_t
kauth_cred_getsvgid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_svgid);
}

void
kauth_cred_setuid(kauth_cred_t cred, uid_t uid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);

	cred->cr_uid = uid;
}

void
kauth_cred_seteuid(kauth_cred_t cred, uid_t uid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);

	cred->cr_euid = uid;
}

void
kauth_cred_setsvuid(kauth_cred_t cred, uid_t uid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);

	cred->cr_svuid = uid;
}

void
kauth_cred_setgid(kauth_cred_t cred, gid_t gid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);

	cred->cr_gid = gid;
}

void
kauth_cred_setegid(kauth_cred_t cred, gid_t gid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);

	cred->cr_egid = gid;
}

void
kauth_cred_setsvgid(kauth_cred_t cred, gid_t gid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);

	cred->cr_svgid = gid;
}

/* Checks if gid is a member of the groups in cred. */
int
kauth_cred_ismember_gid(kauth_cred_t cred, gid_t gid, int *resultp)
{
	int i;

	KASSERT(cred != NULL);
	KASSERT(resultp != NULL);

	*resultp = 0;

	for (i = 0; i < cred->cr_ngroups; i++)
		if (cred->cr_groups[i] == gid) {
			*resultp = 1;
			break;
		}

	return (0);
}

u_int
kauth_cred_ngroups(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_ngroups);
}

/*
 * Return the group at index idx from the groups in cred.
 */
gid_t
kauth_cred_group(kauth_cred_t cred, u_int idx)
{
	KASSERT(cred != NULL);
	KASSERT(idx < cred->cr_ngroups);

	return (cred->cr_groups[idx]);
}

/* XXX elad: gmuid is unused for now. */
int
kauth_cred_setgroups(kauth_cred_t cred, gid_t *grbuf, size_t len, uid_t gmuid)
{
	KASSERT(cred != NULL);
	KASSERT(cred->cr_refcnt == 1);
	KASSERT(len <= sizeof(cred->cr_groups) / sizeof(cred->cr_groups[0]));

	if (len)
		memcpy(cred->cr_groups, grbuf, len * sizeof(cred->cr_groups[0]));
	memset(cred->cr_groups + len, 0xff,
	    sizeof(cred->cr_groups) - (len * sizeof(cred->cr_groups[0])));

	cred->cr_ngroups = len;

	return (0);
}

int
kauth_cred_getgroups(kauth_cred_t cred, gid_t *grbuf, size_t len)
{
	KASSERT(cred != NULL);
	KASSERT(len <= cred->cr_ngroups);

	memset(grbuf, 0xff, sizeof(*grbuf) * len);
	memcpy(grbuf, cred->cr_groups, sizeof(*grbuf) * len);

	return (0);
}

/*
 * Match uids in two credentials.
 */
int
kauth_cred_uidmatch(kauth_cred_t cred1, kauth_cred_t cred2)
{
	KASSERT(cred1 != NULL);
	KASSERT(cred2 != NULL);

	if (cred1->cr_uid == cred2->cr_uid ||
	    cred1->cr_euid == cred2->cr_uid ||
	    cred1->cr_uid == cred2->cr_euid ||
	    cred1->cr_euid == cred2->cr_euid)
		return (1);

	return (0);
}

u_int
kauth_cred_getrefcnt(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_refcnt);
}

/*
 * Convert userland credentials (struct uucred) to kauth_cred_t.
 * XXX: For NFS & puffs
 */
void    
kauth_uucred_to_cred(kauth_cred_t cred, const struct uucred *uuc)
{       
	KASSERT(cred != NULL);
	KASSERT(uuc != NULL);
 
	cred->cr_refcnt = 1;
	cred->cr_uid = uuc->cr_uid;
	cred->cr_euid = uuc->cr_uid;
	cred->cr_svuid = uuc->cr_uid;
	cred->cr_gid = uuc->cr_gid;
	cred->cr_egid = uuc->cr_gid;
	cred->cr_svgid = uuc->cr_gid;
	cred->cr_ngroups = min(uuc->cr_ngroups, NGROUPS);
	kauth_cred_setgroups(cred, __UNCONST(uuc->cr_groups),
	    cred->cr_ngroups, -1);
}

/*
 * Convert kauth_cred_t to userland credentials (struct uucred).
 * XXX: For NFS & puffs
 */
void    
kauth_cred_to_uucred(struct uucred *uuc, const kauth_cred_t cred)
{       
	KASSERT(cred != NULL);
	KASSERT(uuc != NULL);
	int ng;

	ng = min(cred->cr_ngroups, NGROUPS);
	uuc->cr_uid = cred->cr_euid;  
	uuc->cr_gid = cred->cr_egid;  
	uuc->cr_ngroups = ng;
	kauth_cred_getgroups(cred, uuc->cr_groups, ng);
}

/*
 * Compare kauth_cred_t and uucred credentials.
 * XXX: Modelled after crcmp() for NFS.
 */
int
kauth_cred_uucmp(kauth_cred_t cred, const struct uucred *uuc)
{
	KASSERT(cred != NULL);
	KASSERT(uuc != NULL);

	if (cred->cr_euid == uuc->cr_uid &&
	    cred->cr_egid == uuc->cr_gid &&
	    cred->cr_ngroups == uuc->cr_ngroups) {
		int i;

		/* Check if all groups from uuc appear in cred. */
		for (i = 0; i < uuc->cr_ngroups; i++) {
			int ismember;

			ismember = 0;
			if (kauth_cred_ismember_gid(cred, uuc->cr_groups[i],
			    &ismember) != 0 || !ismember)
				return (1);
		}

		return (0);
	}

	return (1);
}

/*
 * Make a struct ucred out of a kauth_cred_t.  For compatibility.
 */
void
kauth_cred_toucred(kauth_cred_t cred, struct ucred *uc)
{
	KASSERT(cred != NULL);
	KASSERT(uc != NULL);

	uc->cr_ref = cred->cr_refcnt;
	uc->cr_uid = cred->cr_euid;
	uc->cr_gid = cred->cr_egid;
	uc->cr_ngroups = min(cred->cr_ngroups,
			     sizeof(uc->cr_groups) / sizeof(uc->cr_groups[0]));
	memcpy(uc->cr_groups, cred->cr_groups,
	       uc->cr_ngroups * sizeof(uc->cr_groups[0]));
}

/*
 * Make a struct pcred out of a kauth_cred_t.  For compatibility.
 */
void
kauth_cred_topcred(kauth_cred_t cred, struct pcred *pc)
{
	KASSERT(cred != NULL);
	KASSERT(pc != NULL);

	pc->pc_ucred = NULL;
	pc->p_ruid = cred->cr_uid;
	pc->p_svuid = cred->cr_svuid;
	pc->p_rgid = cred->cr_gid;
	pc->p_svgid = cred->cr_svgid;
	pc->p_refcnt = cred->cr_refcnt;
}

/*
 * Return kauth_cred_t for the current LWP.
 */
kauth_cred_t
kauth_cred_get(void)
{
	return (curlwp->l_cred);
}

/*
 * Returns a scope matching the provided id.
 * Requires the scope list lock to be held by the caller.
 */
static kauth_scope_t
kauth_ifindscope(const char *id)
{
	kauth_scope_t scope;

	/* XXX: assert lock on scope list? */

	scope = NULL;
	SIMPLEQ_FOREACH(scope, &scope_list, next_scope) {
		if (strcmp(scope->id, id) == 0)
			break;
	}

	return (scope);
}

/*
 * Register a new scope.
 *
 * id - identifier for the scope
 * callback - the scope's default listener
 * cookie - cookie to be passed to the listener(s)
 */
kauth_scope_t
kauth_register_scope(const char *id, kauth_scope_callback_t callback,
    void *cookie)
{
	kauth_scope_t scope;
	kauth_listener_t listener = NULL; /* XXX gcc */

	/* Sanitize input */
	if (id == NULL)
		return (NULL);

	/* Allocate space for a new scope and listener. */
	scope = kmem_alloc(sizeof(*scope), KM_SLEEP);
	if (scope == NULL)
		return NULL;
	if (callback != NULL) {
		listener = kmem_alloc(sizeof(*listener), KM_SLEEP);
		if (listener == NULL) {
			kmem_free(scope, sizeof(*scope));
			return (NULL);
		}
	}

	/*
	 * Acquire scope list lock.
	 *
	 * XXXSMP insufficient locking.
	 */
	simple_lock(&scopes_lock);

	/* Check we don't already have a scope with the same id */
	if (kauth_ifindscope(id) != NULL) {
		simple_unlock(&scopes_lock);

		kmem_free(scope, sizeof(*scope));
		if (callback != NULL)
			kmem_free(listener, sizeof(*listener));

		return (NULL);
	}

	/* Initialize new scope with parameters */
	scope->id = id;
	scope->cookie = cookie;
	scope->nlisteners = 1;

	SIMPLEQ_INIT(&scope->listenq);

	/* Add default listener */
	if (callback != NULL) {
		listener->func = callback;
		listener->scope = scope;
		listener->refcnt = 0;
		SIMPLEQ_INSERT_HEAD(&scope->listenq, listener, listener_next);
	}

	/* Insert scope to scopes list */
	SIMPLEQ_INSERT_TAIL(&scope_list, scope, next_scope);

	simple_unlock(&scopes_lock);

	return (scope);
}

/*
 * Initialize the kernel authorization subsystem.
 *
 * Initialize the scopes list lock.
 * Register built-in scopes: generic, process.
 */
void
kauth_init(void)
{
	SIMPLEQ_INIT(&scope_list);
	simple_lock_init(&scopes_lock);

	/* Register generic scope. */
	kauth_builtin_scope_generic = kauth_register_scope(KAUTH_SCOPE_GENERIC,
	    NULL, NULL);

	/* Register system scope. */
	kauth_builtin_scope_system = kauth_register_scope(KAUTH_SCOPE_SYSTEM,
	    NULL, NULL);

	/* Register process scope. */
	kauth_builtin_scope_process = kauth_register_scope(KAUTH_SCOPE_PROCESS,
	    NULL, NULL);

	/* Register network scope. */
	kauth_builtin_scope_network = kauth_register_scope(KAUTH_SCOPE_NETWORK,
	    NULL, NULL);

	/* Register machdep scope. */
	kauth_builtin_scope_machdep = kauth_register_scope(KAUTH_SCOPE_MACHDEP,
	    NULL, NULL);

	/* Register device scope. */
	kauth_builtin_scope_device = kauth_register_scope(KAUTH_SCOPE_DEVICE,
	    NULL, NULL);
}

/*
 * Deregister a scope.
 * Requires scope list lock to be held by the caller.
 *
 * scope - the scope to deregister
 */
void
kauth_deregister_scope(kauth_scope_t scope)
{
	if (scope != NULL) {
		/* Remove scope from list */
		SIMPLEQ_REMOVE(&scope_list, scope, kauth_scope, next_scope);
		kmem_free(scope, sizeof(*scope));
	}
}

/*
 * Register a listener.
 *
 * id - scope identifier.
 * callback - the callback routine for the listener.
 * cookie - cookie to pass unmoidfied to the callback.
 */
kauth_listener_t
kauth_listen_scope(const char *id, kauth_scope_callback_t callback,
   void *cookie)
{
	kauth_scope_t scope;
	kauth_listener_t listener;

	/*
	 * Find scope struct.
	 *
	 * XXXSMP insufficient locking.
	 */
	simple_lock(&scopes_lock);
	scope = kauth_ifindscope(id);
	simple_unlock(&scopes_lock);
	if (scope == NULL)
		return (NULL);

	/* Allocate listener */
	listener = kmem_alloc(sizeof(*listener), KM_SLEEP);
	if (listener == NULL)
		return (NULL);

	/* Initialize listener with parameters */
	listener->func = callback;
	listener->refcnt = 0;

	/* Add listener to scope */
	SIMPLEQ_INSERT_TAIL(&scope->listenq, listener, listener_next);

	/* Raise number of listeners on scope. */
	scope->nlisteners++;
	listener->scope = scope;

	listeners_have_been_loaded = TRUE;

	return (listener);
}

/*
 * Deregister a listener.
 *
 * listener - listener reference as returned from kauth_listen_scope().
 */
void
kauth_unlisten_scope(kauth_listener_t listener)
{
	if (listener != NULL) {
		SIMPLEQ_REMOVE(&listener->scope->listenq, listener,
		    kauth_listener, listener_next);
		listener->scope->nlisteners--;
		kmem_free(listener, sizeof(*listener));
	}
}

/*
 * Authorize a request.
 *
 * scope - the scope of the request as defined by KAUTH_SCOPE_* or as
 *	   returned from kauth_register_scope().
 * credential - credentials of the user ("actor") making the request.
 * action - request identifier.
 * arg[0-3] - passed unmodified to listener(s).
 */
int
kauth_authorize_action(kauth_scope_t scope, kauth_cred_t cred,
		       kauth_action_t action, void *arg0, void *arg1,
		       void *arg2, void *arg3)
{
	kauth_listener_t listener;
	int error, allow, fail;

#if 0 /* defined(LOCKDEBUG) */
	spinlock_switchcheck();
	simple_lock_only_held(NULL, "kauth_authorize_action");
#endif

	KASSERT(cred != NULL);
	KASSERT(action != 0);

	/* Short-circuit requests coming from the kernel. */
	if (cred == NOCRED || cred == FSCRED)
		return (0);

	KASSERT(scope != NULL);

	if (!listeners_have_been_loaded) {
		KASSERT(SIMPLEQ_EMPTY(&scope->listenq));

		return (0);
	}

	fail = 0;
	allow = 0;
	SIMPLEQ_FOREACH(listener, &scope->listenq, listener_next) {
		error = listener->func(cred, action, scope->cookie, arg0,
				       arg1, arg2, arg3);

		if (error == KAUTH_RESULT_ALLOW)
			allow = 1;
		else if (error == KAUTH_RESULT_DENY)
			fail = 1;
	}

	return ((allow && !fail) ? 0 : EPERM);
};

/*
 * Generic scope authorization wrapper.
 */
int
kauth_authorize_generic(kauth_cred_t cred, kauth_action_t action, void *arg0)
{
	return (kauth_authorize_action(kauth_builtin_scope_generic, cred, 
	    action, arg0, NULL, NULL, NULL));
}

/*
 * System scope authorization wrapper.
 */
int
kauth_authorize_system(kauth_cred_t cred, kauth_action_t action,
    enum kauth_system_req req, void *arg1, void *arg2, void *arg3)
{
	return (kauth_authorize_action(kauth_builtin_scope_system, cred,
	    action, (void *)req, arg1, arg2, arg3));
}

/*
 * Process scope authorization wrapper.
 */
int
kauth_authorize_process(kauth_cred_t cred, kauth_action_t action,
    struct proc *p, void *arg1, void *arg2, void *arg3)
{
	return (kauth_authorize_action(kauth_builtin_scope_process, cred,
	    action, p, arg1, arg2, arg3));
}

/*
 * Network scope authorization wrapper.
 */
int
kauth_authorize_network(kauth_cred_t cred, kauth_action_t action,
    enum kauth_network_req req, void *arg1, void *arg2, void *arg3)
{
	return (kauth_authorize_action(kauth_builtin_scope_network, cred,
	    action, (void *)req, arg1, arg2, arg3));
}

int
kauth_authorize_machdep(kauth_cred_t cred, kauth_action_t action,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	return (kauth_authorize_action(kauth_builtin_scope_machdep, cred,
	    action, arg0, arg1, arg2, arg3));
}

int
kauth_authorize_device(kauth_cred_t cred, kauth_action_t action,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	return (kauth_authorize_action(kauth_builtin_scope_device, cred,
	    action, arg0, arg1, arg2, arg3));
}

int
kauth_authorize_device_tty(kauth_cred_t cred, kauth_action_t action,
    struct tty *tty)
{
	return (kauth_authorize_action(kauth_builtin_scope_device, cred,
	    action, tty, NULL, NULL, NULL));
}

int
kauth_authorize_device_spec(kauth_cred_t cred, enum kauth_device_req req,
    struct vnode *vp)
{
	return (kauth_authorize_action(kauth_builtin_scope_device, cred,
	    KAUTH_DEVICE_RAWIO_SPEC, (void *)req, vp, NULL, NULL));
}

int
kauth_authorize_device_passthru(kauth_cred_t cred, dev_t dev, u_long bits,
    void *data)
{
	return (kauth_authorize_action(kauth_builtin_scope_device, cred,
	    KAUTH_DEVICE_RAWIO_PASSTHRU, (void *)bits, (void *)(u_long)dev,
	    data, NULL));
}
