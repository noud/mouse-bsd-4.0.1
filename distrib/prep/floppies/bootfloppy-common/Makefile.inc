#	$NetBSD: Makefile.inc,v 1.10 2006/03/23 18:51:19 garbled Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE
#	KERNEL

MDEC?=		${DESTDIR}/usr/mdec
BOOTLOADER?=	${MDEC}/boot_com0
TMPKERNEL=	${.OBJDIR}/netbsd

${IMAGE}: ${KERNEL}
	gzip -cd ${KERNEL} > ${TMPKERNEL}
	${TOOL_PREPMKBOOTIMAGE} -b ${BOOTLOADER} -k ${TMPKERNEL} ${.TARGET}
	-rm -f ${TMPKERNEL}

KFILES=	${IMAGE}

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/floppy

.include <bsd.prog.mk>
