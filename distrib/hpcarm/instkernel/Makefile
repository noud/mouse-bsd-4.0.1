#	$NetBSD: Makefile,v 1.1.10.2 2008/09/22 18:15:00 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${DISTRIBDIR}/miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		INSTALL_IPAQ	${MINIROOT}	netbsd-IPAQ
MDSET_RELEASEDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
