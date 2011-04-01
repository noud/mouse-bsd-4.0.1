/* $NetBSD: lfs_user.h,v 1.2 2006/07/18 23:37:13 perseant Exp $ */
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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

#include <stdarg.h>

struct lfs;
struct segsum;
struct finfo;

/*
 * In the fsck code we don't need lfs_unlockvp, but we don't have a mount
 * structure to give us the devvp.  Overload the unlockvp with devvp.
 */
#define lfs_devvp lfs_unlockvp

/*
 * This is outside of the kernel, so no locking primitives available.
 */
#undef simple_lock
#undef simple_unlock

#define simple_lock(arg)	/* empty */
#define simple_unlock(arg)	/* empty */

/*
 * Structure used to pass around logical block paths generated by
 * ufs_getlbns and used by truncate and bmap code.
 */
struct indir {
	daddr_t in_lbn;		/* Logical block number. */
	int in_off;		/* Offset in buffer. */
	int in_exists;		/* Flag if the block exists. */
};

typedef int32_t ufs_daddr_t;

/* Convert between inode pointers and vnode pointers. */
#define	VTOI(vp)	((struct inode *)(vp)->v_data)
#define VTOD(vp)	(VTOI(vp)->i_din.ffs1_din)

#define sbdirty()	++fsdirty

extern int fsdirty;
extern struct uvnodelst vnodelist;

int lfs_vop_strategy(struct ubuf *);
int lfs_vop_bwrite(struct ubuf *);
int lfs_vop_bmap(struct uvnode *, daddr_t, daddr_t *);

struct uvnode *lfs_raw_vget(struct lfs *, ino_t, int, ufs_daddr_t);
struct lfs *lfs_init(int, daddr_t, daddr_t, int, int);
struct lfs *lfs_verify(struct lfs *, struct lfs *, struct uvnode *, int);
int check_summary(struct lfs *, struct segsum *, ufs_daddr_t, int, struct uvnode *, void (*)(ufs_daddr_t, struct finfo *));
ufs_daddr_t try_verify(struct lfs *, struct uvnode *, ufs_daddr_t, int);
struct ufs1_dinode *lfs_ifind(struct lfs *, ino_t, struct ubuf *);
void call_panic(const char *, ...);
void my_vpanic(int, const char *, va_list);
int extend_ifile(struct lfs *);
struct uvnode *lfs_valloc(struct lfs *, ino_t);
int lfs_balloc(struct uvnode *, off_t, int, struct ubuf **);
