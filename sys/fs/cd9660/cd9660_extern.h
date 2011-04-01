/*	$NetBSD: cd9660_extern.h,v 1.20 2006/07/13 12:00:25 martin Exp $	*/

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *	@(#)iso.h	8.4 (Berkeley) 12/5/94
 */

/*
 * Definitions used in the kernel for cd9660 file system support.
 */
#ifndef _ISOFS_CD9660_CD9660_EXTERN_H_
#define _ISOFS_CD9660_CD9660_EXTERN_H_

/*
 * Sysctl values for the cd9660 filesystem.
 */
#define CD9660_UTF8_JOLIET	1	/* UTF-8 encode Joliet file names */

#define CD9660_NAMES { \
        { 0, 0 }, \
        { "utf8_joliet", CTLTYPE_INT }, \
}


/* CD-ROM Format type */
enum ISO_FTYPE  { ISO_FTYPE_DEFAULT, ISO_FTYPE_9660, ISO_FTYPE_RRIP, ISO_FTYPE_ECMA };

#ifndef	ISOFSMNT_ROOT
#define	ISOFSMNT_ROOT	0
#endif

struct iso_mnt {
	int im_flags;
	int im_joliet_level;

	struct mount *im_mountp;
	dev_t im_dev;
	struct vnode *im_devvp;

	int logical_block_size;
	int im_bshift;
	int im_bmask;

	int volume_space_size;

	char root[ISODCL (157, 190)];
	int root_extent;
	int root_size;
	enum ISO_FTYPE  iso_ftype;

	int rr_skip;
	int rr_skip0;
};

#define VFSTOISOFS(mp)	((struct iso_mnt *)((mp)->mnt_data))

#define blkoff(imp, loc)	((loc) & (imp)->im_bmask)
#define lblktosize(imp, blk)	((blk) << (imp)->im_bshift)
#define lblkno(imp, loc)	((loc) >> (imp)->im_bshift)
#define blksize(imp, ip, lbn)	((imp)->logical_block_size)

#ifdef _KERNEL

#include <sys/mallocvar.h>
MALLOC_DECLARE(M_ISOFSMNT);

extern struct pool cd9660_node_pool;
extern int cd9660_utf8_joliet;

int cd9660_mount(struct mount *,
	    const char *, void *, struct nameidata *, struct lwp *);
int cd9660_start(struct mount *, int, struct lwp *);
int cd9660_unmount(struct mount *, int, struct lwp *);
int cd9660_root(struct mount *, struct vnode **);
int cd9660_quotactl(struct mount *, int, uid_t, void *, struct lwp *);
int cd9660_statvfs(struct mount *, struct statvfs *, struct lwp *);
int cd9660_sync(struct mount *, int, kauth_cred_t, struct lwp *);
int cd9660_vget(struct mount *, ino_t, struct vnode **);
int cd9660_fhtovp(struct mount *, struct fid *, struct vnode **);
int cd9660_vptofh(struct vnode *, struct fid *, size_t *);
void cd9660_init(void);
void cd9660_reinit(void);
void cd9660_done(void);

#ifdef SYSCTL_SETUP_PROTO
SYSCTL_SETUP_PROTO(sysctl_vfs_cd9660_setup);
#endif /* SYSCTL_SETUP_PROTO */

int cd9660_mountroot(void);

extern int (**cd9660_vnodeop_p)(void *);
extern int (**cd9660_specop_p)(void *);
extern int (**cd9660_fifoop_p)(void *);

int isochar(const u_char *, const u_char *, int, u_int16_t *);
int isofncmp(const u_char *, size_t, const u_char *, size_t, int);
void isofntrans(u_char *, int, u_char *, u_short *, int, int, int, int);
ino_t isodirino(struct iso_directory_record *, struct iso_mnt *);
#endif /* _KERNEL */

#endif /* _ISOFS_CD9660_CD9660_EXTERN_H_ */
