#	$NetBSD: Makefile.arch.inc,v 1.2 2005/12/11 12:19:47 christos Exp $

.if defined(MACHINE) && ${MACHINE} != "xen"
XEN_BUILD?=	${MACHINE}
.else
XEN_BUILD?=	${MACHINE_ARCH:Ui386}
.endif

.if ${XEN_BUILD} == "i386"
XEN_MACHINE_ARCHS=	i386 x86
.else
XEN_MACHINE_ARCHS=	${XEN_BUILD}
.endif
