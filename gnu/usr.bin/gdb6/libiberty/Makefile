#	$NetBSD: Makefile,v 1.4 2006/11/10 17:36:08 christos Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

LIB=		iberty

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/include

COPTS.argv.c = -Wno-stack-protector
COPTS.cp-demangle.c = -Wno-stack-protector
COPTS.make-relative-prefix.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
