#	$NetBSD: Makefile,v 1.4.24.1 2007/07/19 14:38:23 liamjfoy Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
MKMAN=	no
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.include <bsd.kmod.mk>
