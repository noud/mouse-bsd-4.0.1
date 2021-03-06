/*	$NetBSD: procfs_status.c,v 1.29 2006/11/16 01:33:38 christos Exp $	*/

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)procfs_status.c	8.4 (Berkeley) 6/15/94
 */

/*
 * Copyright (c) 1993 Jan-Simon Pendry
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)procfs_status.c	8.4 (Berkeley) 6/15/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_status.c,v 1.29 2006/11/16 01:33:38 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/kauth.h>

#include <miscfs/procfs/procfs.h>

int
procfs_dostatus(
    struct lwp *curl,
    struct lwp *l,
    struct pfsnode *pfs,
    struct uio *uio
)
{
	struct session *sess;
	struct tty *tp;
	kauth_cred_t cr;
	struct proc *p = l->l_proc;
	char *ps;
	const char *sep;
	int pid, ppid, pgid, sid;
	u_int i;
	char psbuf[256+MAXHOSTNAMELEN];		/* XXX - conservative */
	uint16_t ngroups;

	if (uio->uio_rw != UIO_READ)
		return (EOPNOTSUPP);

	pid = p->p_pid;
	ppid = p->p_pptr ? p->p_pptr->p_pid : 0,
	pgid = p->p_pgrp->pg_id;
	sess = p->p_pgrp->pg_session;
	sid = sess->s_sid;

/* comm pid ppid pgid sid maj,min ctty,sldr start ut st wmsg uid gid groups ... */

	ps = psbuf;
	memcpy(ps, p->p_comm, MAXCOMLEN);
	ps[MAXCOMLEN] = '\0';
	ps += strlen(ps);
	ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), " %d %d %d %d ",
	    pid, ppid, pgid, sid);

	if ((p->p_flag&P_CONTROLT) && (tp = sess->s_ttyp))
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), "%d,%d ",
		    major(tp->t_dev), minor(tp->t_dev));
	else
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), "%d,%d ",
		    -1, -1);

	sep = "";
	if (sess->s_ttyvp) {
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), "%sctty", sep);
		sep = ",";
	}
	if (SESS_LEADER(p)) {
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), "%ssldr", sep);
		sep = ",";
	}
	if (*sep != ',')
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), "noflags");

	if (l->l_flag & L_INMEM)
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), " %lld,%ld",
		    (long long int)p->p_stats->p_start.tv_sec, (long int)p->p_stats->p_start.tv_usec);
	else
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), " -1,-1");

	{
		struct timeval ut, st;

		calcru(p, &ut, &st, (void *) 0);
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf),
		    " %lld,%ld %lld,%ld", (long long int)ut.tv_sec, (long int)ut.tv_usec, (long long int)st.tv_sec,
		    (long int)st.tv_usec);
	}

	ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), " %s",
	    (l->l_wchan && l->l_wmesg) ? l->l_wmesg : "nochan");

	cr = p->p_cred;

	ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), " %d",
		       kauth_cred_geteuid(cr));
	ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), " %d",
		       kauth_cred_getegid(cr));
	ngroups = kauth_cred_ngroups(cr);
	for (i = 0; i < ngroups; i++)
		ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), ",%d",
		    kauth_cred_group(cr, i));
	ps += snprintf(ps, sizeof(psbuf) - (ps - psbuf), "\n");

	return (uiomove_frombuf(psbuf, ps - psbuf, uio));
}
