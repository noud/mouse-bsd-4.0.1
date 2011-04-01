/*	$NetBSD: ops_pcfs.c,v 1.1.1.9 2005/09/20 17:14:59 rpaulo Exp $	*/

/*
 * Copyright (c) 1997-2005 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *
 * File: am-utils/amd/ops_pcfs.c
 *
 */

/*
 * PC (MS-DOS) file system
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

/* forward definitions */
static char *pcfs_match(am_opts *fo);
static int pcfs_mount(am_node *am, mntfs *mf);
static int pcfs_umount(am_node *am, mntfs *mf);

/*
 * Ops structure
 */
am_ops pcfs_ops =
{
  "pcfs",
  pcfs_match,
  0,				/* pcfs_init */
  pcfs_mount,
  pcfs_umount,
  amfs_error_lookup_child,
  amfs_error_mount_child,
  amfs_error_readdir,
  0,				/* pcfs_readlink */
  0,				/* pcfs_mounted */
  0,				/* pcfs_umounted */
  amfs_generic_find_srvr,
  0,				/* pcfs_get_wchan */
  FS_MKMNT | FS_UBACKGROUND | FS_AMQINFO,	/* nfs_fs_flags */
#ifdef HAVE_FS_AUTOFS
  AUTOFS_PCFS_FS_FLAGS,
#endif /* HAVE_FS_AUTOFS */
};



/*
 * PCFS needs remote filesystem.
 */
static char *
pcfs_match(am_opts *fo)
{
  if (!fo->opt_dev) {
    plog(XLOG_USER, "pcfs: no source device specified");
    return 0;
  }
  dlog("PCFS: mounting device \"%s\" on \"%s\"", fo->opt_dev, fo->opt_fs);

  /*
   * Determine magic cookie to put in mtab
   */
  return strdup(fo->opt_dev);
}


static int
mount_pcfs(char *mntdir, char *fs_name, char *opts, int on_autofs)
{
  pcfs_args_t pcfs_args;
  mntent_t mnt;
  int flags;

  /*
   * Figure out the name of the file system type.
   */
  MTYPE_TYPE type = MOUNT_TYPE_PCFS;

  memset((voidp) &pcfs_args, 0, sizeof(pcfs_args)); /* Paranoid */

  /*
   * Fill in the mount structure
   */
  memset((voidp) &mnt, 0, sizeof(mnt));
  mnt.mnt_dir = mntdir;
  mnt.mnt_fsname = fs_name;
  mnt.mnt_type = MNTTAB_TYPE_PCFS;
  mnt.mnt_opts = opts;

  flags = compute_mount_flags(&mnt);
#ifdef HAVE_FS_AUTOFS
  if (on_autofs)
    flags |= autofs_compute_mount_flags(&mnt);
#endif /* HAVE_FS_AUTOFS */

#ifdef HAVE_PCFS_ARGS_T_FSPEC
  pcfs_args.fspec = fs_name;
#endif /* HAVE_PCFS_ARGS_T_FSPEC */

#ifdef HAVE_PCFS_ARGS_T_MASK
  pcfs_args.mask = 0777;	/* this may be the msdos file modes */
#endif /* HAVE_PCFS_ARGS_T_MASK */

#ifdef HAVE_PCFS_ARGS_T_DIRMASK
  pcfs_args.dirmask = 0777;    /* this may be the msdos dir modes */
#endif /* HAVE_PCFS_ARGS_T_DIRMASK */

#ifdef HAVE_PCFS_ARGS_T_UID
  pcfs_args.uid = 0;		/* root */
#endif /* HAVE_PCFS_ARGS_T_UID */

#ifdef HAVE_PCFS_ARGS_T_GID
  pcfs_args.gid = 0;		/* wheel */
#endif /* HAVE_PCFS_ARGS_T_GID */

#ifdef HAVE_PCFS_ARGS_T_SECONDSWEST
  pcfs_args.secondswest = 0;	/* XXX: fill in correct values */
#endif /* HAVE_PCFS_ARGS_T_SECONDSWEST */
#ifdef HAVE_PCFS_ARGS_T_DSTTIME
  pcfs_args.dsttime = 0;	/* XXX: fill in correct values */
#endif /* HAVE_PCFS_ARGS_T_DSTTIME */

  /*
   * Call generic mount routine
   */
  return mount_fs(&mnt, flags, (caddr_t) & pcfs_args, 0, type, 0, NULL, mnttab_file_name, on_autofs);
}


static int
pcfs_mount(am_node *am, mntfs *mf)
{
  int on_autofs = mf->mf_flags & MFF_ON_AUTOFS;
  int error;

  error = mount_pcfs(mf->mf_mount, mf->mf_info, mf->mf_mopts, on_autofs);
  if (error) {
    errno = error;
    plog(XLOG_ERROR, "mount_pcfs: %m");
    return error;
  }

  return 0;
}


static int
pcfs_umount(am_node *am, mntfs *mf)
{
  int unmount_flags = (mf->mf_flags & MFF_ON_AUTOFS) ? AMU_UMOUNT_AUTOFS : 0;

  return UMOUNT_FS(mf->mf_mount, mnttab_file_name, unmount_flags);
}
