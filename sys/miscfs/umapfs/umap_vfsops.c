/*	$NetBSD: umap_vfsops.c,v 1.61 2006/11/16 01:33:40 christos Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * the UCLA Ficus project.
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
 *	from: @(#)null_vfsops.c       1.5 (Berkeley) 7/10/92
 *	@(#)umap_vfsops.c	8.8 (Berkeley) 5/14/95
 */

/*
 * Umap Layer
 * (See mount_umap(8) for a description of this layer.)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umap_vfsops.c,v 1.61 2006/11/16 01:33:40 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/kauth.h>

#include <miscfs/umapfs/umap.h>
#include <miscfs/genfs/layer_extern.h>

int	umapfs_mount(struct mount *, const char *, void *,
			  struct nameidata *, struct lwp *);
int	umapfs_unmount(struct mount *, int, struct lwp *);

/*
 * Mount umap layer
 */
int
umapfs_mount(mp, path, data, ndp, l)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct lwp *l;
{
	struct umap_args args;
	struct vnode *lowerrootvp, *vp;
	struct umap_mount *amp;
	int error;
#ifdef UMAPFS_DIAGNOSTIC
	int i;
#endif

	if (mp->mnt_flag & MNT_GETARGS) {
		amp = MOUNTTOUMAPMOUNT(mp);
		if (amp == NULL)
			return EIO;
		args.la.target = NULL;
		args.nentries = amp->info_nentries;
		args.gnentries = amp->info_gnentries;
		return copyout(&args, data, sizeof(args));
	}

	/* only for root */
	if ((error = kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    &l->l_acflag)) != 0)
		return error;

#ifdef UMAPFS_DIAGNOSTIC
	printf("umapfs_mount(mp = %p)\n", mp);
#endif

	/*
	 * Get argument
	 */
	error = copyin(data, &args, sizeof(struct umap_args));
	if (error)
		return (error);

	/*
	 * Update is not supported
	 */
	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	/*
	 * Find lower node
	 */
	NDINIT(ndp, LOOKUP, FOLLOW|LOCKLEAF,
		UIO_USERSPACE, args.umap_target, l);
	if ((error = namei(ndp)) != 0)
		return (error);

	/*
	 * Sanity check on lower vnode
	 */
	lowerrootvp = ndp->ni_vp;
#ifdef UMAPFS_DIAGNOSTIC
	printf("vp = %p, check for VDIR...\n", lowerrootvp);
#endif

	if (lowerrootvp->v_type != VDIR) {
		vput(lowerrootvp);
		return (EINVAL);
	}

#ifdef UMAPFS_DIAGNOSTIC
	printf("mp = %p\n", mp);
#endif

	amp = (struct umap_mount *) malloc(sizeof(struct umap_mount),
				M_UFSMNT, M_WAITOK);	/* XXX */
	memset(amp, 0, sizeof(struct umap_mount));

	mp->mnt_data = amp;
	mp->mnt_leaf = lowerrootvp->v_mount->mnt_leaf;
	amp->umapm_vfs = lowerrootvp->v_mount;
	if (amp->umapm_vfs->mnt_flag & MNT_LOCAL)
		mp->mnt_flag |= MNT_LOCAL;

	/*
	 * Now copy in the number of entries and maps for umap mapping.
	 */
	if (args.nentries > MAPFILEENTRIES || args.gnentries > GMAPFILEENTRIES) {
		vput(lowerrootvp);
		return (error);
	}

	amp->info_nentries = args.nentries;
	amp->info_gnentries = args.gnentries;
	error = copyin(args.mapdata, amp->info_mapdata,
	    2*sizeof(u_long)*args.nentries);
	if (error) {
		vput(lowerrootvp);
		return (error);
	}

#ifdef UMAPFS_DIAGNOSTIC
	printf("umap_mount:nentries %d\n",args.nentries);
	for (i = 0; i < args.nentries; i++)
		printf("   %ld maps to %ld\n", amp->info_mapdata[i][0],
	 	    amp->info_mapdata[i][1]);
#endif

	error = copyin(args.gmapdata, amp->info_gmapdata,
	    2*sizeof(u_long)*args.gnentries);
	if (error) {
		vput(lowerrootvp);
		return (error);
	}

#ifdef UMAPFS_DIAGNOSTIC
	printf("umap_mount:gnentries %d\n",args.gnentries);
	for (i = 0; i < args.gnentries; i++)
		printf("\tgroup %ld maps to %ld\n",
		    amp->info_gmapdata[i][0],
	 	    amp->info_gmapdata[i][1]);
#endif

	/*
	 * Make sure the mount point's sufficiently initialized
	 * that the node create call will work.
	 */
	vfs_getnewfsid(mp);
	amp->umapm_size = sizeof(struct umap_node);
	amp->umapm_tag = VT_UMAP;
	amp->umapm_bypass = umap_bypass;
	amp->umapm_alloc = layer_node_alloc;	/* the default alloc is fine */
	amp->umapm_vnodeop_p = umap_vnodeop_p;
	simple_lock_init(&amp->umapm_hashlock);
	amp->umapm_node_hashtbl = hashinit(NUMAPNODECACHE, HASH_LIST, M_CACHE,
	    M_WAITOK, &amp->umapm_node_hash);


	/*
	 * fix up umap node for root vnode.
	 */
	error = layer_node_create(mp, lowerrootvp, &vp);
	/*
	 * Make sure the node alias worked
	 */
	if (error) {
		vput(lowerrootvp);
		free(amp, M_UFSMNT);	/* XXX */
		return (error);
	}
	/*
	 * Unlock the node (either the lower or the alias)
	 */
	VOP_UNLOCK(vp, 0);

	/*
	 * Keep a held reference to the root vnode.
	 * It is vrele'd in umapfs_unmount.
	 */
	vp->v_flag |= VROOT;
	amp->umapm_rootvp = vp;

	error = set_statvfs_info(path, UIO_USERSPACE, args.umap_target,
	    UIO_USERSPACE, mp, l);
#ifdef UMAPFS_DIAGNOSTIC
	printf("umapfs_mount: lower %s, alias at %s\n",
		mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname);
#endif
	return error;
}

/*
 * Free reference to umap layer
 */
int
umapfs_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	struct vnode *rtvp = MOUNTTOUMAPMOUNT(mp)->umapm_rootvp;
	int error;
	int flags = 0;

#ifdef UMAPFS_DIAGNOSTIC
	printf("umapfs_unmount(mp = %p)\n", mp);
#endif

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	/*
	 * Clear out buffer cache.  I don't think we
	 * ever get anything cached at this level at the
	 * moment, but who knows...
	 */
#ifdef notyet
	mntflushbuf(mp, 0);
	if (mntinvalbuf(mp, 1))
		return (EBUSY);
#endif
	if (rtvp->v_usecount > 1)
		return (EBUSY);
	if ((error = vflush(mp, rtvp, flags)) != 0)
		return (error);

#ifdef UMAPFS_DIAGNOSTIC
	vprint("alias root of lower", rtvp);
#endif
	/*
	 * Release reference on underlying root vnode
	 */
	vrele(rtvp);
	/*
	 * And blow it away for future re-use
	 */
	vgone(rtvp);
	/*
	 * Finally, throw away the umap_mount structure
	 */
	free(mp->mnt_data, M_UFSMNT);	/* XXX */
	mp->mnt_data = 0;
	return (0);
}

SYSCTL_SETUP(sysctl_vfs_umap_setup, "sysctl vfs.umap subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "umap",
		       SYSCTL_DESCR("UID/GID remapping file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 10, CTL_EOL);
	/*
	 * XXX the "10" above could be dynamic, thereby eliminating
	 * one more instance of the "number to vfs" mapping problem,
	 * but "10" is the order as taken from sys/mount.h
	 */
}

extern const struct vnodeopv_desc umapfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const umapfs_vnodeopv_descs[] = {
	&umapfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops umapfs_vfsops = {
	MOUNT_UMAP,
	umapfs_mount,
	layerfs_start,
	umapfs_unmount,
	layerfs_root,
	layerfs_quotactl,
	layerfs_statvfs,
	layerfs_sync,
	layerfs_vget,
	layerfs_fhtovp,
	layerfs_vptofh,
	layerfs_init,
	NULL,
	layerfs_done,
	NULL,				/* vfs_mountroot */
	layerfs_snapshot,
	vfs_stdextattrctl,
	umapfs_vnodeopv_descs,
	0,				/* vfs_refcount */
	{ NULL, NULL },
};
VFS_ATTACH(umapfs_vfsops);
