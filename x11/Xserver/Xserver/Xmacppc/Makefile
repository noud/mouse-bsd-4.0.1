#	$NetBSD: Makefile,v 1.3 2004/02/21 08:33:23 rtr Exp $

.include <bsd.own.mk>

PROG=	Xmacppc
MKMAN=	no
BINMODE=4711

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/macppc

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.mi} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.hw/netbsd/macppc} ${LDADD.cfb} ${LDADD.mfb} \
	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.hw/netbsd/macppc} ${LDADD.Xext/ext} ${LDADD.dbe} \
	${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} \
	${LIBCWRAPPER} -lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lXext -lX11 -lz -lm

DPADD+=	${DPADD.dix} ${DPADD.mi} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.hw/netbsd/macppc} ${DPADD.cfb} ${DPADD.mfb} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.hw/netbsd/macppc} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} ${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBXEXT} ${LIBX11} ${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
