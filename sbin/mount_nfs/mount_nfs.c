/*	$NetBSD: mount_nfs.c,v 1.53 2006/11/09 10:07:00 yamt Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_nfs.c	8.11 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: mount_nfs.c,v 1.53 2006/11/09 10:07:00 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>

#ifdef ISO
#include <netiso/iso.h>
#endif

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nqnfs.h>
#include <nfs/nfsmount.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <mntopts.h>

#include "mount_nfs.h"

#define	ALTF_BG		0x00000001
#define ALTF_CONN	0x00000002
#define ALTF_DUMBTIMR	0x00000004
#define ALTF_INTR	0x00000008
#define ALTF_KERB	0x00000010
#define ALTF_NFSV3	0x00000020
#define ALTF_RDIRPLUS	0x00000040
#define	ALTF_MNTUDP	0x00000080
#define ALTF_NORESPORT	0x00000100
#define ALTF_SEQPACKET	0x00000200
#define ALTF_NQNFS	0x00000400
#define ALTF_SOFT	0x00000800
#define ALTF_TCP	0x00001000
#define ALTF_NFSV2	0x00002000
#define ALTF_PORT	0x00004000
#define ALTF_RSIZE	0x00008000
#define ALTF_WSIZE	0x00010000
#define ALTF_RDIRSIZE	0x00020000
#define ALTF_MAXGRPS	0x00040000
#define ALTF_LEASETERM	0x00080000
#define ALTF_READAHEAD	0x00100000
#define ALTF_DEADTHRESH	0x00200000
#define ALTF_TIMEO	0x00400000
#define ALTF_RETRANS	0x00800000

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_FORCE,
	MOPT_UPDATE,
	MOPT_GETARGS,
	{ "bg", 0, ALTF_BG, 1 },
	{ "conn", 0, ALTF_CONN, 1 },
	{ "dumbtimer", 0, ALTF_DUMBTIMR, 1 },
	{ "intr", 0, ALTF_INTR, 1 },
	{ "nfsv3", 0, ALTF_NFSV3, 1 },
	{ "rdirplus", 0, ALTF_RDIRPLUS, 1 },
	{ "mntudp", 0, ALTF_MNTUDP, 1 },
	{ "noresport", 0, ALTF_NORESPORT, 1 },
#ifdef ISO
	{ "seqpacket", 0, ALTF_SEQPACKET, 1 },
#endif
	{ "nqnfs", 0, ALTF_NQNFS, 1 },
	{ "soft", 0, ALTF_SOFT, 1 },
	{ "tcp", 0, ALTF_TCP, 1 },
	{ "nfsv2", 0, ALTF_NFSV2, 1 },
	{ "port", 0, ALTF_PORT, 1 },
	{ "rsize", 0, ALTF_RSIZE, 1 },
	{ "wsize", 0, ALTF_WSIZE, 1 },
	{ "rdirsize", 0, ALTF_RDIRSIZE, 1 },
	{ "maxgrps", 0, ALTF_MAXGRPS, 1 },
	{ "leaseterm", 0, ALTF_LEASETERM, 1 },
	{ "readahead", 0, ALTF_READAHEAD, 1 },
	{ "deadthresh", 0, ALTF_DEADTHRESH, 1 },
	{ "timeo", 0, ALTF_TIMEO, 1 },
	MOPT_NULL,

};

struct nfs_args nfsdefargs = {
	NFS_ARGSVERSION,
	(struct sockaddr *)0,
	sizeof (struct sockaddr_in),
	SOCK_DGRAM,
	0,
	(u_char *)0,
	0,
	NFSMNT_NFSV3|NFSMNT_NOCONN|NFSMNT_RESVPORT,
	NFS_WSIZE,
	NFS_RSIZE,
	NFS_READDIRSIZE,
	10,
	NFS_RETRANS,
	NFS_MAXGRPS,
	NFS_DEFRAHEAD,
	NQ_DEFLEASE,
	NQ_DEADTHRESH,
	(char *)0,
};

int retrycnt = 0;
int opflags = 0;
int nfsproto = IPPROTO_UDP;
int force2 = 0;
int force3 = 0;
int mnttcp_ok = 1;
int port = 0;

static void	shownfsargs(const struct nfs_args *);
#ifdef ISO
static struct	iso_addr *iso_addr(const char *);
#endif
int	mount_nfs(int argc, char **argv);
/* void	set_rpc_maxgrouplist(int); */
static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_nfs(argc, argv);
}
#endif

int
mount_nfs(int argc, char *argv[])
{
	int c, retval;
	struct nfs_args *nfsargsp;
	struct nfs_args nfsargs;
	struct nfsd_cargs ncd;
	struct sockaddr_storage sa;
	int mntflags, altflags, i, nfssvc_flag, num;
	char name[MAXPATHLEN], *p, *spec;
	mntoptparse_t mp;
	retrycnt = DEF_RETRY;

	mntflags = 0;
	altflags = 0;
	nfsargs = nfsdefargs;
	nfsargsp = &nfsargs;
	while ((c = getopt(argc, argv,
	    "23a:bcCdD:g:I:iKL:lm:o:PpqR:r:sTt:w:x:UX")) != -1)
		switch (c) {
		case '3':
		case 'q':
			if (force2)
				errx(1, "conflicting version options");
			force3 = 1;
			break;
		case '2':
			if (force3)
				errx(1, "conflicting version options");
			force2 = 1;
			nfsargsp->flags &= ~NFSMNT_NFSV3;
			break;
		case 'a':
			num = strtol(optarg, &p, 10);
			if (*p || num < 0)
				errx(1, "illegal -a value -- %s", optarg);
			nfsargsp->readahead = num;
			nfsargsp->flags |= NFSMNT_READAHEAD;
			break;
		case 'b':
			opflags |= BGRND;
			break;
		case 'c':
			nfsargsp->flags |= NFSMNT_NOCONN;
			break;
		case 'C':
			nfsargsp->flags &= ~NFSMNT_NOCONN;
			break;
		case 'D':
			/* ignore */
			break;
		case 'd':
			nfsargsp->flags |= NFSMNT_DUMBTIMR;
			break;
#if 0 /* XXXX */
		case 'g':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -g value -- %s", optarg);
			set_rpc_maxgrouplist(num);
			nfsargsp->maxgrouplist = num;
			nfsargsp->flags |= NFSMNT_MAXGRPS;
			break;
#endif
		case 'I':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -I value -- %s", optarg);
			nfsargsp->readdirsize = num;
			nfsargsp->flags |= NFSMNT_READDIRSIZE;
			break;
		case 'i':
			nfsargsp->flags |= NFSMNT_INT;
			break;
		case 'L':
			/* ignore */
			break;
		case 'l':
			nfsargsp->flags |= NFSMNT_RDIRPLUS;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, &altflags);
			if (mp == NULL)
				err(1, "getmntopts");
			if (altflags & ALTF_BG)
				opflags |= BGRND;
			if (altflags & ALTF_CONN)
				nfsargsp->flags &= ~NFSMNT_NOCONN;
			if (altflags & ALTF_DUMBTIMR)
				nfsargsp->flags |= NFSMNT_DUMBTIMR;
			if (altflags & ALTF_INTR)
				nfsargsp->flags |= NFSMNT_INT;
			if (altflags & (ALTF_NFSV3|ALTF_NQNFS)) {
				if (force2)
					errx(1, "conflicting version options");
				force3 = 1;
			}
			if (altflags & ALTF_NFSV2) {
				if (force3)
					errx(1, "conflicting version options");
				force2 = 1;
				nfsargsp->flags &= ~NFSMNT_NFSV3;
			}
			if (altflags & ALTF_RDIRPLUS)
				nfsargsp->flags |= NFSMNT_RDIRPLUS;
			if (altflags & ALTF_MNTUDP)
				mnttcp_ok = 0;
			if (altflags & ALTF_NORESPORT)
				nfsargsp->flags &= ~NFSMNT_RESVPORT;
#ifdef ISO
			if (altflags & ALTF_SEQPACKET)
				nfsargsp->sotype = SOCK_SEQPACKET;
#endif
			if (altflags & ALTF_SOFT)
				nfsargsp->flags |= NFSMNT_SOFT;
			if (altflags & ALTF_TCP) {
				nfsargsp->sotype = SOCK_STREAM;
				nfsproto = IPPROTO_TCP;
			}
			if (altflags & ALTF_PORT) {
				port = getmntoptnum(mp, "port");
			}
			if (altflags & ALTF_RSIZE) {
				nfsargsp->rsize =
				    (int)getmntoptnum(mp, "rsize");
				nfsargsp->flags |= NFSMNT_RSIZE;
			}
			if (altflags & ALTF_WSIZE) {
				nfsargsp->wsize =
				    (int)getmntoptnum(mp, "wsize");
				nfsargsp->flags |= NFSMNT_WSIZE;
			}
			if (altflags & ALTF_RDIRSIZE) {
				nfsargsp->rsize =
				    (int)getmntoptnum(mp, "rdirsize");
				nfsargsp->flags |= NFSMNT_READDIRSIZE;
			}
#if 0
			if (altflags & ALTF_MAXGRPS) {
				set_rpc_maxgrouplist(num);
				nfsargsp->maxgrouplist =
				    (int)getmntoptnum(mp, "maxgrps");
				nfsargsp->flags |= NFSMNT_MAXGRPS;
			}
#endif
			if (altflags & ALTF_LEASETERM) {
				nfsargsp->leaseterm =
				(int)getmntoptnum(mp, "leaseterm");
				nfsargsp->flags |= NFSMNT_LEASETERM;
			}
			if (altflags & ALTF_READAHEAD) {
				nfsargsp->readahead =
				    (int)getmntoptnum(mp, "readahead");
				nfsargsp->flags |= NFSMNT_READAHEAD;
			}
			if (altflags & ALTF_DEADTHRESH) {
				nfsargsp->deadthresh = 
				    (int)getmntoptnum(mp, "deadthresh");
				nfsargsp->flags |= NFSMNT_DEADTHRESH;
			}
			if (altflags & ALTF_TIMEO) {
				nfsargsp->timeo = 
				    (int)getmntoptnum(mp, "timeo");
				nfsargsp->flags |= NFSMNT_TIMEO;
			}
			if (altflags & ALTF_RETRANS) {
				nfsargsp->retrans = 
				    (int)getmntoptnum(mp, "retrans");
				nfsargsp->flags |= NFSMNT_RETRANS;
			}
			altflags = 0;
			freemntopts(mp);
			break;
		case 'P':
			nfsargsp->flags |= NFSMNT_RESVPORT;
			break;
		case 'p':
			nfsargsp->flags &= ~NFSMNT_RESVPORT;
			break;
		case 'R':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -R value -- %s", optarg);
			retrycnt = num;
			break;
		case 'r':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -r value -- %s", optarg);
			nfsargsp->rsize = num;
			nfsargsp->flags |= NFSMNT_RSIZE;
			break;
#ifdef ISO
		case 'S':
			nfsargsp->sotype = SOCK_SEQPACKET;
			break;
#endif
		case 's':
			nfsargsp->flags |= NFSMNT_SOFT;
			break;
		case 'T':
			nfsargsp->sotype = SOCK_STREAM;
			nfsproto = IPPROTO_TCP;
			break;
		case 't':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -t value -- %s", optarg);
			nfsargsp->timeo = num;
			nfsargsp->flags |= NFSMNT_TIMEO;
			break;
		case 'w':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -w value -- %s", optarg);
			nfsargsp->wsize = num;
			nfsargsp->flags |= NFSMNT_WSIZE;
			break;
		case 'x':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -x value -- %s", optarg);
			nfsargsp->retrans = num;
			nfsargsp->flags |= NFSMNT_RETRANS;
			break;
		case 'X':
			nfsargsp->flags |= NFSMNT_XLATECOOKIE;
			break;
		case 'U':
			mnttcp_ok = 0;
			break;
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	spec = *argv++;
	if (realpath(*argv, name) == NULL)           /* Check mounton path */
		err(1, "realpath %s", *argv);
	if (strncmp(*argv, name, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", *argv);
		warnx("using \"%s\" instead.", name);
	}

retry:
	if ((mntflags & MNT_GETARGS) != 0) {
		memset(&sa, 0, sizeof(sa));
		nfsargsp->addr = (struct sockaddr *)&sa;
		nfsargsp->addrlen = sizeof(sa);
	} else {
		char *tspec;

		if ((tspec = strdup(spec)) == NULL) {
			err(1, "strdup");
		}
		if (!getnfsargs(tspec, nfsargsp)) {
			exit(1);
		}
		free(tspec);
	}
	if ((retval = mount(MOUNT_NFS, name, mntflags, nfsargsp))) {
		/* Did we just default to v3 on a v2-only kernel?
		 * If so, default to v2 & try again */
		if (errno == EPROGMISMATCH &&
		    (nfsargsp->flags & NFSMNT_NFSV3) != 0 && !force3) {
			/*
			 * fall back to v2.  XXX lack of V3 umount.
			 */
			nfsargsp->flags &= ~NFSMNT_NFSV3;
			goto retry;
		}
	}
	if (retval)
		err(1, "%s on %s", spec, name);
	if (mntflags & MNT_GETARGS) {
		shownfsargs(nfsargsp);
		return (0);
	}
		
	if (nfsargsp->flags & NFSMNT_KERB) {
		if ((opflags & ISBGRND) == 0) {
			if ((i = fork()) != 0) {
				if (i == -1)
					err(1, "nqnfs 1");
				exit(0);
			}
			(void) setsid();
			(void) close(STDIN_FILENO);
			(void) close(STDOUT_FILENO);
			(void) close(STDERR_FILENO);
			(void) chdir("/");
		}
		openlog("mount_nfs", LOG_PID, LOG_DAEMON);
		nfssvc_flag = NFSSVC_MNTD;
		ncd.ncd_dirp = name;
		while (nfssvc(nfssvc_flag, (caddr_t)&ncd) < 0) {
			if (errno != ENEEDAUTH) {
				syslog(LOG_ERR, "nfssvc err %m");
				continue;
			}
			nfssvc_flag =
			    NFSSVC_MNTD | NFSSVC_GOTAUTH | NFSSVC_AUTHINFAIL;
		}
	}
	exit(0);
}

static void
shownfsargs(const struct nfs_args *nfsargsp)
{
	char fbuf[2048];
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	int error;

	(void)snprintb(fbuf, sizeof(fbuf), NFSMNT_BITS, nfsargsp->flags);
	if (nfsargsp->addr != NULL) {
		error = getnameinfo(nfsargsp->addr, nfsargsp->addrlen, host,
		    sizeof(host), serv, sizeof(serv),
		    NI_NUMERICHOST | NI_NUMERICSERV);
		if (error != 0)
			warnx("getnameinfo: %s", gai_strerror(error));
	} else
		error = -1;

	if (error == 0)
		printf("addr=%s, port=%s, addrlen=%d, ",
		    host, serv, nfsargsp->addrlen);
	printf("sotype=%d, proto=%d, fhsize=%d, "
	    "flags=%s, wsize=%d, rsize=%d, readdirsize=%d, timeo=%d, "
	    "retrans=%d, maxgrouplist=%d, readahead=%d, leaseterm=%d, "
	    "deadthresh=%d\n",
	    nfsargsp->sotype,
	    nfsargsp->proto,
	    nfsargsp->fhsize,
	    fbuf,
	    nfsargsp->wsize,
	    nfsargsp->rsize,
	    nfsargsp->readdirsize,
	    nfsargsp->timeo,
	    nfsargsp->retrans,
	    nfsargsp->maxgrouplist,
	    nfsargsp->readahead,
	    nfsargsp->leaseterm,
	    nfsargsp->deadthresh);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: mount_nfs %s\n%s\n%s\n%s\n%s\n",
"[-23bcCdilpPqsTUX] [-a maxreadahead] [-D deadthresh]",
"\t[-g maxgroups] [-I readdirsize] [-L leaseterm]",
"\t[-o options] [-R retrycnt] [-r readsize] [-t timeout]",
"\t[-w writesize] [-x retrans]",
"\trhost:path node");
	exit(1);
}
