#	$NetBSD: Makefile,v 1.6 2004/05/23 02:24:51 lukem Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)
MKPRIVATELIB=	yes

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

LIB=		opcodes
SRCS=		${G_OPCODES_OBJS:.o=.c}
CPPFLAGS=	-I${.CURDIR}  -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/include -I${DIST}/bfd -I${.CURDIR}/../bfd \
		-DHAVE_CONFIG_H

.endif

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/opcodes
