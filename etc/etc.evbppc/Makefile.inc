#	$NetBSD: Makefile.inc,v 1.5 2005/08/03 14:55:37 shige Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

KERNEL_SETS=		WALNUT EV64260 EXPLORA451 OPENBLOCKS200 OPENBLOCKS266
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL_WALNUT INSTALL_OPENBLOCKS266
