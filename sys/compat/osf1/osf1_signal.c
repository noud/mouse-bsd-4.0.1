/*	$NetBSD: osf1_signal.c,v 1.27 2005/12/11 12:20:23 christos Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: osf1_signal.c,v 1.27 2005/12/11 12:20:23 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_signal.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>

#if 0
int
osf1_sys_kill(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_kill_args *uap = v;
	struct sys_kill_args ka;

	if (SCARG(uap, signum) < 0 || SCARG(uap, signum) > OSF1_NSIG)
		return EINVAL;
	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = osf1_to_native_signo[SCARG(uap, signum)];
	return sys_kill(l, &ka, retval);
}
#endif

int
osf1_sys_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sigaction_args *uap = v;
	struct proc *p = l->l_proc;
	struct osf1_sigaction *nosa, *oosa, tmposa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, signum) < 0 || SCARG(uap, signum) > OSF1_NSIG)
		return EINVAL;
	nosa = SCARG(uap, nsa);
	oosa = SCARG(uap, osa);

	if (nosa != NULL) {
		if ((error = copyin(nosa, &tmposa, sizeof(tmposa))) != 0)
			return error;
		osf1_cvt_sigaction_to_native(&tmposa, &nbsa);
	}

	if ((error = sigaction1(p,
				osf1_to_native_signo[SCARG(uap, signum)],
				(nosa ? &nbsa : NULL),
				(oosa ? &obsa : NULL),
				NULL, 0)) != 0)
		return error;

	if (oosa != NULL) {
		osf1_cvt_sigaction_from_native(&obsa, &tmposa);
		if ((error = copyout(&tmposa, oosa, sizeof(tmposa))) != 0)
			return error;
	}

	return 0;
}

int
osf1_sys_sigaltstack(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sigaltstack_args *uap = v;
	struct proc *p = l->l_proc;
	struct osf1_sigaltstack *noss, *ooss, tmposs;
	struct sigaltstack *nbss, *obss, tmpbss;
	struct sys___sigaltstack14_args sa;
	caddr_t sg;
	int error;

	sg = stackgap_init(p, 0);
	noss = SCARG(uap, nss);
	ooss = SCARG(uap, oss);

	if (ooss != NULL)
		obss = stackgap_alloc(p, &sg, sizeof(struct sigaltstack));
	else
		obss = NULL;

	if (noss != NULL) {
		nbss = stackgap_alloc(p, &sg, sizeof(struct sigaltstack));
		if ((error = copyin(noss, &tmposs, sizeof(tmposs))) != 0)
			return error;
		if ((error = osf1_cvt_sigaltstack_to_native(&tmposs, &tmpbss)) != 0)
			return error;
		if ((error = copyout(&tmpbss, nbss, sizeof(tmpbss))) != 0)
			return error;
	} else
		nbss = NULL;

	SCARG(&sa, nss) = nbss;
	SCARG(&sa, oss) = obss;

	if ((error = sys___sigaltstack14(l, &sa, retval)) != 0)
		return error;

	if (obss != NULL) {
		if ((error = copyin(obss, &tmpbss, sizeof(tmpbss))) != 0)
			return error;
		osf1_cvt_sigaltstack_from_native(&tmpbss, &tmposs);
		if ((error = copyout(&tmposs, ooss, sizeof(tmposs))) != 0)
			return error;
	}

	return 0;
}

#if 0
int
osf1_sys_signal(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_signal_args *uap = v;
	struct proc *p = l->l_proc;
	int signum;
	int error;
	caddr_t sg = stackgap_init(p, 0);

	if (SCARG(uap, signum) < 0 || SCARG(uap, signum) > OSF1_NSIG)
		return EINVAL;
	signum = osf1_to_native_signo[OSF1_SIGNO(SCARG(uap, signum))];
	if (signum <= 0 || signum >= OSF1_NSIG) {
		if (OSF1_SIGCALL(SCARG(uap, signum)) == OSF1_SIGNAL_MASK ||
		    OSF1_SIGCALL(SCARG(uap, signum)) == OSF1_SIGDEFER_MASK)
			*retval = (int)OSF1_SIG_ERR;
		return EINVAL;
	}

	switch (OSF1_SIGCALL(SCARG(uap, signum))) {
	case OSF1_SIGDEFER_MASK:
		/*
		 * sigset is identical to signal() except
		 * that SIG_HOLD is allowed as
		 * an action.
		 */
		if (SCARG(uap, handler) == OSF1_SIG_HOLD) {
			struct sys_sigprocmask_args sa;

			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(signum);
			return sys_sigprocmask(l, &sa, retval);
		}
		/* FALLTHROUGH */

	case OSF1_SIGNAL_MASK:
		{
			struct sys_sigaction_args sa_args;
			struct sigaction *nbsa, *obsa, sa;

			nbsa = stackgap_alloc(p, &sg, sizeof(struct sigaction));
			obsa = stackgap_alloc(p, &sg, sizeof(struct sigaction));
			SCARG(&sa_args, signum) = signum;
			SCARG(&sa_args, nsa) = nbsa;
			SCARG(&sa_args, osa) = obsa;

			sa.sa_handler = SCARG(uap, handler);
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = 0;
#if 0
			if (signum != SIGALRM)
				sa.sa_flags = SA_RESTART;
#endif
			if ((error = copyout(&sa, nbsa, sizeof(sa))) != 0)
				return error;
			if ((error = sys_sigaction(l, &sa_args, retval)) != 0) {
				DPRINTF(("signal: sigaction failed: %d\n",
					 error));
				*retval = (int)OSF1_SIG_ERR;
				return error;
			}
			if ((error = copyin(obsa, &sa, sizeof(sa))) != 0)
				return error;
			*retval = (int)sa.sa_handler;
			return 0;
		}

	case OSF1_SIGHOLD_MASK:
		{
			struct sys_sigprocmask_args sa;

			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(signum);
			return sys_sigprocmask(l, &sa, retval);
		}

	case OSF1_SIGRELSE_MASK:
		{
			struct sys_sigprocmask_args sa;

			SCARG(&sa, how) = SIG_UNBLOCK;
			SCARG(&sa, mask) = sigmask(signum);
			return sys_sigprocmask(l, &sa, retval);
		}

	case OSF1_SIGIGNORE_MASK:
		{
			struct sys_sigaction_args sa_args;
			struct sigaction *bsa, sa;

			bsa = stackgap_alloc(p, &sg, sizeof(struct sigaction));
			SCARG(&sa_args, signum) = signum;
			SCARG(&sa_args, nsa) = bsa;
			SCARG(&sa_args, osa) = NULL;

			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = 0;
			if ((error = copyout(&sa, bsa, sizeof(sa))) != 0)
				return error;
			if ((error = sys_sigaction(l, &sa_args, retval)) != 0) {
				DPRINTF(("sigignore: sigaction failed\n"));
				return error;
			}
			return 0;
		}

	case OSF1_SIGPAUSE_MASK:
		{
			struct sys_sigsuspend_args sa;

			SCARG(&sa, mask) = p->p_sigmask & ~sigmask(signum);
			return sys_sigsuspend(l, &sa, retval);
		}

	default:
		return ENOSYS;
	}
}

int
osf1_sys_sigpending(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sigpending_args *uap = v;
	struct proc *p = l->l_proc;
	sigset_t bss;
	osf1_sigset_t oss;

	bss = p->p_siglist & p->p_sigmask;
	osf1_cvt_sigset_from_native(&bss, &oss);

	return copyout(&oss, SCARG(uap, mask), sizeof(oss));
}

int
osf1_sys_sigprocmask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sigprocmask_args *uap = v;
	struct proc *p = l->l_proc;
	osf1_sigset_t oss;
	sigset_t bss;
	int error = 0;

	if (SCARG(uap, oset) != NULL) {
		/* Fix the return value first if needed */
		osf1_cvt_sigset_from_native(&p->p_sigmask, &oss);
		if ((error = copyout(&oss, SCARG(uap, oset), sizeof(oss))) != 0)
			return error;
	}

	if (SCARG(uap, set) == NULL)
		/* Just examine */
		return 0;

	if ((error = copyin(SCARG(uap, set), &oss, sizeof(oss))) != 0)
		return error;

	osf1_cvt_sigset_to_native(&oss, &bss);

	(void) splsched();	/* XXXSMP */

	switch (SCARG(uap, how)) {
	case OSF1_SIG_BLOCK:
		p->p_sigmask |= bss & ~sigcantmask;
		break;

	case OSF1_SIG_UNBLOCK:
		p->p_sigmask &= ~bss;
		break;

	case OSF1_SIG_SETMASK:
		p->p_sigmask = bss & ~sigcantmask;
		break;

	default:
		error = EINVAL;
		break;
	}

	(void) spl0();

	return error;
}

int
osf1_sys_sigsuspend(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sigsuspend_args *uap = v;
	osf1_sigset_t oss;
	sigset_t bss;
	struct sys_sigsuspend_args sa;
	int error;

	if ((error = copyin(SCARG(uap, ss), &oss, sizeof(oss))) != 0)
		return error;

	osf1_cvt_sigset_to_native(&oss, &bss);

	SCARG(&sa, mask) = bss;
	return sys_sigsuspend(l, &sa, retval);
}
#endif
