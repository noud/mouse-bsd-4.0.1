/*	$NetBSD: hn_dref_isc3.h,v 1.1.1.5 2005/04/23 18:12:20 christos Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) xstrlcpy((dst), (src), MAXHOSTNAMELEN)
