#	$NetBSD: Makefile,v 1.5 2005/10/03 20:22:08 macallan Exp $

.include <bsd.own.mk>

PROG=	Xsun24
BINMODE=4711

MKMAN=	no

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/sun

LDADD=  ${LIBDIR.hw/sun}/sunInitMulti.o ${LDADD.dix} ${LDADD.hw/sun} \
	${LDADD.os} ${LDADD.cfb16} ${LDADD.cfb24} ${LDADD.cfb32} \
	${LDADD.cfb} ${LDADD.mfb} ${LDADD.dix/xpstubs} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xkb}

.if ${MACHINE} == "sparc64" || ${MACHINE} == "sparc"
LDADD+= ${LDADD.xinput}
DPADD=	${DPADD.xinput}
.endif

LDADD+=	${LDADD.lbx} -llbxutil ${LDADD.dbe} ${LDADD.record} \
	${LDADD.xtrap} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.randr} \
	${LDADD.render} -lXfont -lfreetype ${LDADD.dix/xpstubs} -lfreetype \
	-lXau -lXdmcp -lz -lm

DPADD+=	${LIBDIR.hw/sun}/sunInitMulti.o ${DPADD.hw/sun} ${DPADD.dix} \
	${DPADD.os} ${DPADD.cfb16} ${DPADD.cfb24} ${DPADD.cfb32} \
	${DPADD.cfb} ${DPADD.mfb} ${DPADD.mi} ${LIBXEXT} ${DPADD.xkb} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.xtrap} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.randr} \
	${DPADD.render} ${LIBXFONT} ${DPADD.dix/xpstubs} ${LIBFREETYPE} \
	${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
