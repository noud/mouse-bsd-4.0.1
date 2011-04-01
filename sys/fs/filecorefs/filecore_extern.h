/*	$NetBSD: filecore_extern.h,v 1.14 2006/07/13 12:00:25 martin Exp $	*/

/*-
 * Copyright (c) 1994 The Regents of the University of California.
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
 *	filecore_extern.h	1.1	1998/6/26
 */

/*-
 * Copyright (c) 1998 Andrew McMurry
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
 *	filecore_extern.h	1.1	1998/6/26
 */

/*
 * Definitions used in the kernel for Acorn Filecore file system support.
 */
#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

#ifndef	FILECOREMNT_ROOT
#define	FILECOREMNT_ROOT	0
#endif

#include <sys/mallocvar.h>

MALLOC_DECLARE(M_FILECOREMNT);

struct filecore_mnt {
	struct mount *fc_mountp;
	dev_t fc_dev;
	struct vnode *fc_devvp;

	u_int32_t blksize;
	u_int32_t log2bsize;
	u_int32_t map;
	u_int32_t idspz;
	u_int32_t mask;
	u_int64_t nblks;

	uid_t fc_uid;		/* uid to set as owner of the files */
	gid_t fc_gid;		/* gid to set as owner of the files */
	int fc_mntflags;
	struct filecore_disc_record drec;
};

#define VFSTOFILECORE(mp)	((struct filecore_mnt *)((mp)->mnt_data))

#define blkoff(fcp, loc)	((loc) & ((fcp)->blksize-1))
#define lblktosize(fcp, blk)	((blk) << (fcp)->log2bsize)
#define lblkno(fcp, loc)	((loc) >> (fcp)->log2bsize)
#define blksize(fcp, ip, lbn)	((fcp)->blksize)

extern struct pool filecore_node_pool;

int filecore_mount __P((struct mount *,
	    const char *, void *, struct nameidata *, struct lwp *));
int filecore_start __P((struct mount *, int, struct lwp *));
int filecore_unmount __P((struct mount *, int, struct lwp *));
int filecore_root __P((struct mount *, struct vnode **));
int filecore_quotactl __P((struct mount *, int, uid_t, void *, struct lwp *));
int filecore_statvfs __P((struct mount *, struct statvfs *, struct lwp *));
int filecore_sync __P((struct mount *, int, kauth_cred_t, struct lwp *));
int filecore_vget __P((struct mount *, ino_t, struct vnode **));
int filecore_fhtovp __P((struct mount *, struct fid *, struct vnode **));
int filecore_checkexp __P((struct mount *, struct mbuf *, int *,
	    kauth_cred_t *));
int filecore_vptofh __P((struct vnode *, struct fid *, size_t *));
void filecore_init __P((void));
void filecore_reinit __P((void));
void filecore_done __P((void));

#ifdef SYSCTL_SETUP_PROTO
SYSCTL_SETUP_PROTO(sysctl_vfs_filecore_setup);
#endif /* SYSCTL_SETUP_PROTO */

extern int (**filecore_vnodeop_p) __P((void *));

int filecore_bbchecksum __P((void *));
int filecore_bread __P((struct filecore_mnt *, u_int32_t, int,
    kauth_cred_t, struct buf **));
int filecore_map __P((struct filecore_mnt *, u_int32_t, daddr_t, daddr_t *));
