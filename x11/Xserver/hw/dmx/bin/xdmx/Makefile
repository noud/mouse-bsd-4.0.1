# $NetBSD: Makefile,v 1.2 2006/09/11 22:28:33 dbj Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/examples
PROG=	xdmx
MKMAN=	no

LDADD=	-ldmx -lXext -lX11
DPADD=  ${LIBDMX} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.SERVER}

PROGNAME=dmx

.include <bsd.x11.mk>
.include <bsd.prog.mk>
