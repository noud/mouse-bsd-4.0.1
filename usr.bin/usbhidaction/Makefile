#	$NetBSD: Makefile,v 1.5 2006/05/19 14:43:58 christos Exp $

.include <bsd.own.mk>
WARNS?=4

PROG=	usbhidaction
SRCS=	usbhidaction.c

LDADD+=	-lusbhid -lutil
DPADD+=	${LIBUSBHID} ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
