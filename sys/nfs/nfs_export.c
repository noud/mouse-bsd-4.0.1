/*	$NetBSD: nfs_export.c,v 1.22.2.1 2007/02/17 23:27:51 tron Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_subr.c	8.13 (Berkeley) 4/18/94
 */

/*
 * VFS exports list management.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nfs_export.c,v 1.22.2.1 2007/02/17 23:27:51 tron Exp $");

#include "opt_compat_netbsd.h"
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/dirent.h>
#include <sys/socket.h>		/* XXX for AF_MAX */
#include <sys/kauth.h>

#include <net/radix.h>

#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfs_var.h>

/*
 * Network address lookup element.
 */
struct netcred {
	struct	radix_node netc_rnodes[2];
	int	netc_refcnt;
	int	netc_exflags;
	kauth_cred_t netc_anon;
};

/*
 * Network export information.
 */
struct netexport {
	CIRCLEQ_ENTRY(netexport) ne_list;
	struct mount *ne_mount;
	struct netcred ne_defexported;		      /* Default export */
	struct radix_node_head *ne_rtable[AF_MAX+1]; /* Individual exports */
};
CIRCLEQ_HEAD(, netexport) netexport_list =
    CIRCLEQ_HEAD_INITIALIZER(netexport_list);

/* Malloc type used by the mount<->netexport map. */
MALLOC_DEFINE(M_NFS_EXPORT, "nfs_export", "NFS export data");

/* Publicly exported file system. */
struct nfs_public nfs_pub;

/*
 * Local prototypes.
 */
static int init_exports(struct mount *, struct netexport **);
static int hang_addrlist(struct mount *, struct netexport *,
    const struct export_args *);
static int sacheck(struct sockaddr *);
static int free_netcred(struct radix_node *, void *);
static int export(struct netexport *, const struct export_args *);
static int setpublicfs(struct mount *, struct netexport *,
    const struct export_args *);
static struct netcred *netcred_lookup(struct netexport *, struct mbuf *);
static struct netexport *netexport_lookup(const struct mount *);
static struct netexport *netexport_lookup_byfsid(const fsid_t *);
static void netexport_clear(struct netexport *);
static void netexport_insert(struct netexport *);
static void netexport_remove(struct netexport *);
static void netexport_wrlock(void);
static void netexport_wrunlock(void);

/*
 * PUBLIC INTERFACE
 */

/*
 * Declare and initialize the file system export hooks.
 */
static void nfs_export_unmount(struct mount *);

struct vfs_hooks nfs_export_hooks = {
	nfs_export_unmount
};
VFS_HOOKS_ATTACH(nfs_export_hooks);

/*
 * VFS unmount hook for NFS exports.
 *
 * Releases NFS exports list resources if the given mount point has some.
 * As allocation happens lazily, it may be that it doesn't has this
 * information, although it theorically should.
 */
static void
nfs_export_unmount(struct mount *mp)
{
	struct netexport *ne;

	KASSERT(mp != NULL);

	netexport_wrlock();
	ne = netexport_lookup(mp);
	if (ne == NULL) {
		netexport_wrunlock();
		return;
	}
	netexport_clear(ne);
	netexport_remove(ne);
	netexport_wrunlock();
	free(ne, M_NFS_EXPORT);
}

/*
 * Atomically set the NFS exports list of the given file system, replacing
 * it with a new list of entries.
 *
 * Returns zero on success or an appropriate error code otherwise.
 *
 * Helper function for the nfssvc(2) system call (NFSSVC_SETEXPORTSLIST
 * command).
 */
int
mountd_set_exports_list(const struct mountd_exports_list *mel, struct lwp *l)
{
	int error;
#ifdef notyet
	/* XXX: See below to see the reason why this is disabled. */
	size_t i;
#endif
	struct mount *mp;
	struct netexport *ne;
	struct nameidata nd;
	struct vnode *vp;
	struct fid *fid;
	size_t fid_size;

	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    &l->l_acflag) != 0)
		return EPERM;

	/* Lookup the file system path. */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, mel->mel_path, l);
	error = namei(&nd);
	if (error != 0)
		return error;
	vp = nd.ni_vp;
	mp = vp->v_mount;

	fid_size = 0;
	if ((error = VFS_VPTOFH(vp, NULL, &fid_size)) == E2BIG) {
		fid = malloc(fid_size, M_TEMP, M_NOWAIT);
		if (fid != NULL) {
			error = VFS_VPTOFH(vp, fid, &fid_size);
			free(fid, M_TEMP);
		}
	}
	if (error != 0) {
		vput(vp);
		return EOPNOTSUPP;
	}

	/* Mark the file system busy. */
	error = vfs_busy(mp, LK_NOWAIT, NULL);
	vput(vp);
	if (error != 0)
		return error;

	netexport_wrlock();
	ne = netexport_lookup(mp);
	if (ne == NULL) {
		error = init_exports(mp, &ne);
		if (error != 0) {
			goto out;
		}
	}

	KASSERT(ne != NULL);
	KASSERT(ne->ne_mount == mp);

	/*
	 * XXX: The part marked as 'notyet' works fine from the kernel's
	 * point of view, in the sense that it is able to atomically update
	 * the complete exports list for a file system.  However, supporting
	 * this in mountd(8) requires a lot of work; so, for now, keep the
	 * old behavior of updating a single entry per call.
	 *
	 * When mountd(8) is fixed, just remove the second branch of this
	 * preprocessor conditional and enable the first one.
	 */
#ifdef notyet
	netexport_clear(ne);
	for (i = 0; error == 0 && i < mel->mel_nexports; i++)
		error = export(ne, &mel->mel_exports[i]);
#else
	if (mel->mel_nexports == 0)
		netexport_clear(ne);
	else if (mel->mel_nexports == 1)
		error = export(ne, &mel->mel_exports[0]);
	else {
		printf("mountd_set_exports_list: Cannot set more than one "
		    "entry at once (unimplemented)\n");
		error = EOPNOTSUPP;
	}
#endif

out:
	netexport_wrunlock();
	vfs_unbusy(mp);
	return error;
}

static void
netexport_insert(struct netexport *ne)
{

	CIRCLEQ_INSERT_HEAD(&netexport_list, ne, ne_list);
}

static void
netexport_remove(struct netexport *ne)
{

	CIRCLEQ_REMOVE(&netexport_list, ne, ne_list);
}

static struct netexport *
netexport_lookup(const struct mount *mp)
{
	struct netexport *ne;

	CIRCLEQ_FOREACH(ne, &netexport_list, ne_list) {
		if (ne->ne_mount == mp) {
			goto done;
		}
	}
	ne = NULL;
done:
	return ne;
}

static struct netexport *
netexport_lookup_byfsid(const fsid_t *fsid)
{
	struct netexport *ne;

	CIRCLEQ_FOREACH(ne, &netexport_list, ne_list) {
		const struct mount *mp = ne->ne_mount;

		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == fsid->__fsid_val[0] &&
		    mp->mnt_stat.f_fsidx.__fsid_val[1] == fsid->__fsid_val[1]) {
			goto done;
		}
	}
	ne = NULL;
done:

	return ne;
}

/*
 * Check if the file system specified by the 'mp' mount structure is
 * exported to a client with 'anon' anonymous credentials.  The 'mb'
 * argument is an mbuf containing the network address of the client.
 * The return parameters for the export flags for the client are returned
 * in the address specified by 'wh'.
 *
 * This function is used exclusively by the NFS server.  It is generally
 * invoked before VFS_FHTOVP to validate that client has access to the
 * file system.
 */

int
netexport_check(const fsid_t *fsid, struct mbuf *mb, struct mount **mpp,
    int *wh, kauth_cred_t *anon)
{
	struct netexport *ne;
	struct netcred *np;

	ne = netexport_lookup_byfsid(fsid);
	if (ne == NULL) {
		return EACCES;
	}
	np = netcred_lookup(ne, mb);
	if (np == NULL) {
		return EACCES;
	}

	*mpp = ne->ne_mount;
	*wh = np->netc_exflags;
	*anon = np->netc_anon;

	return 0;
}

#ifdef COMPAT_30
/*
 * Handles legacy export requests.  In this case, the export information
 * is hardcoded in a specific place of the mount arguments structure (given
 * in data); the request for an update is given through the fspec field
 * (also in a known location), which must be a null pointer.
 *
 * Returns EJUSTRETURN if the given command was not a export request.
 * Otherwise, returns 0 on success or an appropriate error code otherwise.
 */
int
nfs_update_exports_30(struct mount *mp, const char *path, void *data,
    struct lwp *l)
{
	int error;
	struct {
		const char *fspec;
		struct export_args30 eargs;
	} args;
	struct mountd_exports_list mel;

	mel.mel_path = path;

	error = copyin(data, &args, sizeof(args));
	if (error != 0)
		return EJUSTRETURN;

	if (args.fspec != NULL)
		return EJUSTRETURN;

	if (args.eargs.ex_flags & 0x00020000) {
		/* Request to delete exports.  The mask above holds the
		 * value that used to be in MNT_DELEXPORT. */
		mel.mel_nexports = 0;
	} else {
		struct export_args eargs;

		/* The following assumes export_args has not changed since
		 * export_args30. */
		memcpy(&eargs, &args.eargs, sizeof(struct export_args));

		mel.mel_nexports = 1;
		mel.mel_exports = &eargs;
	}

	return mountd_set_exports_list(&mel, l);
}
#endif

/*
 * INTERNAL FUNCTIONS
 */

/*
 * Initializes NFS exports for the file system given in 'mp' if it supports
 * file handles; this is determined by checking whether mp's vfs_vptofh and
 * vfs_fhtovp operations are NULL or not.
 *
 * If successful, returns 0 and sets *mnpp to the address of the new
 * mount_netexport_pair item; otherwise returns an appropriate error code
 * and *mnpp remains unmodified.
 */
static int
init_exports(struct mount *mp, struct netexport **nep)
{
	int error;
	struct export_args ea;
	struct netexport *ne;

	KASSERT(mp != NULL);

	/* Ensure that we do not already have this mount point. */
	KASSERT(netexport_lookup(mp) == NULL);

	ne = malloc(sizeof(*ne), M_NFS_EXPORT, M_WAITOK | M_ZERO);
	ne->ne_mount = mp;

	/* Set the default export entry.  Handled internally by export upon
	 * first call. */
	memset(&ea, 0, sizeof(ea));
	ea.ex_root = -2;
	if (mp->mnt_flag & MNT_RDONLY)
		ea.ex_flags |= MNT_EXRDONLY;
	error = export(ne, &ea);
	if (error != 0) {
		free(ne, M_NFS_EXPORT);
	} else {
		netexport_insert(ne);
		*nep = ne;
	}

	return error;
}

/*
 * Build hash lists of net addresses and hang them off the mount point.
 * Called by export() to set up a new entry in the lists of export
 * addresses.
 */
static int
hang_addrlist(struct mount *mp, struct netexport *nep,
    const struct export_args *argp)
{
	int error, i;
	struct netcred *np, *enp;
	struct radix_node_head *rnh;
	struct sockaddr *saddr, *smask;
	struct domain *dom;

	smask = NULL;

	if (argp->ex_addrlen == 0) {
		if (mp->mnt_flag & MNT_DEFEXPORTED)
			return EPERM;
		np = &nep->ne_defexported;
		KASSERT(np->netc_anon == NULL);
		np->netc_anon = kauth_cred_alloc();
		np->netc_exflags = argp->ex_flags;
		kauth_uucred_to_cred(np->netc_anon, &argp->ex_anon);
		mp->mnt_flag |= MNT_DEFEXPORTED;
		return 0;
	}

	if (argp->ex_addrlen > MLEN || argp->ex_masklen > MLEN)
		return EINVAL;

	i = sizeof(struct netcred) + argp->ex_addrlen + argp->ex_masklen;
	np = malloc(i, M_NETADDR, M_WAITOK | M_ZERO);
	np->netc_anon = kauth_cred_alloc();
	saddr = (struct sockaddr *)(np + 1);
	error = copyin(argp->ex_addr, saddr, argp->ex_addrlen);
	if (error)
		goto out;
	if (saddr->sa_len > argp->ex_addrlen)
		saddr->sa_len = argp->ex_addrlen;
	if (sacheck(saddr) == -1)
		return EINVAL;
	if (argp->ex_masklen) {
		smask = (struct sockaddr *)((caddr_t)saddr + argp->ex_addrlen);
		error = copyin(argp->ex_mask, smask, argp->ex_masklen);
		if (error)
			goto out;
		if (smask->sa_len > argp->ex_masklen)
			smask->sa_len = argp->ex_masklen;
		if (smask->sa_family != saddr->sa_family)
			return EINVAL;
		if (sacheck(smask) == -1)
			return EINVAL;
	}
	i = saddr->sa_family;
	if ((rnh = nep->ne_rtable[i]) == 0) {
		/*
		 * Seems silly to initialize every AF when most are not
		 * used, do so on demand here
		 */
		DOMAIN_FOREACH(dom) {
			if (dom->dom_family == i && dom->dom_rtattach) {
				dom->dom_rtattach((void **)&nep->ne_rtable[i],
					dom->dom_rtoffset);
				break;
			}
		}
		if ((rnh = nep->ne_rtable[i]) == 0) {
			error = ENOBUFS;
			goto out;
		}
	}

	enp = (struct netcred *)(*rnh->rnh_addaddr)(saddr, smask, rnh,
	    np->netc_rnodes);
	if (enp != np) {
		if (enp == NULL) {
			enp = (struct netcred *)(*rnh->rnh_lookup)(saddr,
			    smask, rnh);
			if (enp == NULL) {
				error = EPERM;
				goto out;
			}
		} else
			enp->netc_refcnt++;

		goto check;
	} else
		enp->netc_refcnt = 1;

	np->netc_exflags = argp->ex_flags;
	kauth_uucred_to_cred(np->netc_anon, &argp->ex_anon);
	return 0;
check:
	if (enp->netc_exflags != argp->ex_flags ||
	    kauth_cred_uucmp(enp->netc_anon, &argp->ex_anon) != 0)
		error = EPERM;
	else
		error = 0;
out:
	KASSERT(np->netc_anon != NULL);
	kauth_cred_free(np->netc_anon);
	free(np, M_NETADDR);
	return error;
}

/*
 * Ensure that the address stored in 'sa' is valid.
 * Returns zero on success, otherwise -1.
 */
static int
sacheck(struct sockaddr *sa)
{

	switch (sa->sa_family) {
#ifdef INET
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		char *p = (char *)sin->sin_zero;
		size_t i;

		if (sin->sin_len != sizeof(*sin))
			return -1;
		if (sin->sin_port != 0)
			return -1;
		for (i = 0; i < sizeof(sin->sin_zero); i++)
			if (*p++ != '\0')
				return -1;
		return 0;
	}
#endif
#ifdef INET6
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

		if (sin6->sin6_len != sizeof(*sin6))
			return -1;
		if (sin6->sin6_port != 0)
			return -1;
		return 0;
	}
#endif
	default:
		return -1;
	}
}

/*
 * Free the netcred object pointed to by the 'rn' radix node.
 * 'w' holds a pointer to the radix tree head.
 */
static int
free_netcred(struct radix_node *rn, void *w)
{
	struct radix_node_head *rnh = (struct radix_node_head *)w;
	struct netcred *np = (struct netcred *)(void *)rn;

	(*rnh->rnh_deladdr)(rn->rn_key, rn->rn_mask, rnh);
	if (--(np->netc_refcnt) <= 0) {
		KASSERT(np->netc_anon != NULL);
		kauth_cred_free(np->netc_anon);
		free(np, M_NETADDR);
	}
	return 0;
}

/*
 * Clears the exports list for a given file system.
 */
static void
netexport_clear(struct netexport *ne)
{
	struct radix_node_head *rnh;
	struct mount *mp = ne->ne_mount;
	int i;

	if (mp->mnt_flag & MNT_EXPUBLIC) {
		setpublicfs(NULL, NULL, NULL);
		mp->mnt_flag &= ~MNT_EXPUBLIC;
	}

	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = ne->ne_rtable[i]) != NULL) {
			(*rnh->rnh_walktree)(rnh, free_netcred, rnh);
			free(rnh, M_RTABLE);
			ne->ne_rtable[i] = NULL;
		}
	}

	if ((mp->mnt_flag & MNT_DEFEXPORTED) != 0) {
		struct netcred *np = &ne->ne_defexported;

		KASSERT(np->netc_anon != NULL);
		kauth_cred_free(np->netc_anon);
		np->netc_anon = NULL;
	} else {
		KASSERT(ne->ne_defexported.netc_anon == NULL);
	}

	mp->mnt_flag &= ~(MNT_EXPORTED | MNT_DEFEXPORTED);
}

/*
 * Add a new export entry (described by an export_args structure) to the
 * given file system.
 */
static int
export(struct netexport *nep, const struct export_args *argp)
{
	struct mount *mp = nep->ne_mount;
	int error;

	if (argp->ex_flags & MNT_EXPORTED) {
		if (argp->ex_flags & MNT_EXPUBLIC) {
			if ((error = setpublicfs(mp, nep, argp)) != 0)
				return error;
			mp->mnt_flag |= MNT_EXPUBLIC;
		}
		if ((error = hang_addrlist(mp, nep, argp)) != 0)
			return error;
		mp->mnt_flag |= MNT_EXPORTED;
	}
	return 0;
}

/*
 * Set the publicly exported filesystem (WebNFS).  Currently, only
 * one public filesystem is possible in the spec (RFC 2054 and 2055)
 */
static int
setpublicfs(struct mount *mp, struct netexport *nep,
    const struct export_args *argp)
{
	char *cp;
	int error;
	struct vnode *rvp;
	size_t fhsize;

	/*
	 * mp == NULL -> invalidate the current info, the FS is
	 * no longer exported. May be called from either export
	 * or unmount, so check if it hasn't already been done.
	 */
	if (mp == NULL) {
		if (nfs_pub.np_valid) {
			nfs_pub.np_valid = 0;
			if (nfs_pub.np_handle != NULL) {
				free(nfs_pub.np_handle, M_TEMP);
				nfs_pub.np_handle = NULL;
			}
			if (nfs_pub.np_index != NULL) {
				FREE(nfs_pub.np_index, M_TEMP);
				nfs_pub.np_index = NULL;
			}
		}
		return 0;
	}

	/*
	 * Only one allowed at a time.
	 */
	if (nfs_pub.np_valid != 0 && mp != nfs_pub.np_mount)
		return EBUSY;

	/*
	 * Get real filehandle for root of exported FS.
	 */
	if ((error = VFS_ROOT(mp, &rvp)))
		return error;

	fhsize = 0;
	error = vfs_composefh(rvp, NULL, &fhsize);
	if (error != E2BIG)
		return error;
	nfs_pub.np_handle = malloc(fhsize, M_TEMP, M_NOWAIT);
	if (nfs_pub.np_handle == NULL)
		error = ENOMEM;
	else
		error = vfs_composefh(rvp, nfs_pub.np_handle, &fhsize);
	if (error)
		return error;

	vput(rvp);

	/*
	 * If an indexfile was specified, pull it in.
	 */
	if (argp->ex_indexfile != NULL) {
		MALLOC(nfs_pub.np_index, char *, MAXNAMLEN + 1, M_TEMP,
		    M_WAITOK);
		error = copyinstr(argp->ex_indexfile, nfs_pub.np_index,
		    MAXNAMLEN, (size_t *)0);
		if (!error) {
			/*
			 * Check for illegal filenames.
			 */
			for (cp = nfs_pub.np_index; *cp; cp++) {
				if (*cp == '/') {
					error = EINVAL;
					break;
				}
			}
		}
		if (error) {
			FREE(nfs_pub.np_index, M_TEMP);
			return error;
		}
	}

	nfs_pub.np_mount = mp;
	nfs_pub.np_valid = 1;
	return 0;
}

/*
 * Lookup an export entry in the exports list that matches the address
 * stored in 'nam'.  If no entry is found, the default one is used instead
 * (if available).
 */
static struct netcred *
netcred_lookup(struct netexport *ne, struct mbuf *nam)
{
	struct netcred *np;
	struct radix_node_head *rnh;
	struct sockaddr *saddr;

	if ((ne->ne_mount->mnt_flag & MNT_EXPORTED) == 0) {
		return NULL;
	}

	/*
	 * Lookup in the export list first.
	 */
	np = NULL;
	if (nam != NULL) {
		saddr = mtod(nam, struct sockaddr *);
		rnh = ne->ne_rtable[saddr->sa_family];
		if (rnh != NULL) {
			np = (struct netcred *)
				(*rnh->rnh_matchaddr)((caddr_t)saddr,
						      rnh);
			if (np && np->netc_rnodes->rn_flags & RNF_ROOT)
				np = NULL;
		}
	}
	/*
	 * If no address match, use the default if it exists.
	 */
	if (np == NULL && ne->ne_mount->mnt_flag & MNT_DEFEXPORTED)
		np = &ne->ne_defexported;

	return np;
}

static struct lock netexport_lock = LOCK_INITIALIZER(PVFS, "netexp", 0, 0);

void
netexport_rdlock(void)
{

	lockmgr(&netexport_lock, LK_SHARED, NULL);
}

void
netexport_rdunlock(void)
{

	lockmgr(&netexport_lock, LK_RELEASE, NULL);
}

static void
netexport_wrlock(void)
{

	lockmgr(&netexport_lock, LK_EXCLUSIVE, NULL);
}

static void
netexport_wrunlock(void)
{

	lockmgr(&netexport_lock, LK_RELEASE, NULL);
}
