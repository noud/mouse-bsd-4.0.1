#	$NetBSD: Makefile,v 1.33.2.1 2007/07/19 14:38:20 liamjfoy Exp $

# For now, we install the machine and arch includes, and symlink 'machine'
# to the location of the machine includes (usually).
#
# Eventually, we should install everything.

.include <bsd.own.mk>

SUBDIR=	${MACHINE}
ARCHSUBDIR= ${MACHINE_CPU}

.if ${ARCHSUBDIR} == "mips64"
ARCHSUBDIR= mips
.endif
.if ${ARCHSUBDIR} == "powerpc64"
ARCHSUBDIR= powerpc
.endif

.if ${MACHINE} != ${ARCHSUBDIR}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif
.if ${MACHINE} == sparc
SUBDIR+= sparc64
.endif
.if (${MACHINE} == hpcmips || ${MACHINE} == hpcsh)
SUBDIR+= hpc
.endif
.if (${MACHINE} == sun2 || ${MACHINE} == sun3)
SUBDIR+= sun68k
.endif
.if defined(XEN_BUILD)
SUBDIR+= xen
.endif

#SUBDIR=acorn26 acorn32 algor alpha amiga amigappc arm arc atari \
#	bebox \
#	cats cesfic cobalt \
#	dreamcast \
#	evbarm evbmips evbppc evbsh3 ews4800mips\
#	hp300 hpc hpcarm hpcmips hpcsh \
#	i386 iyonix \
#	luna68k \
#	m68k mac68k macppc mips mipsco mmeye mvme68k \
#	netwinder news68k newsmips next68k \
#	ofppc \
#	pc532 playstation2 pmax powerpc prep \
#	sandpoint sbmips sgimips sh3 shark sparc sparc64 sun2 sun3 sun68k \
#	vax \
#	x68k x86_64 xen

INCSYMLINKS= ${MACHINE} /usr/include/machine

INCSYMLINKS+= machine/float.h /usr/include/float.h \
	machine/stdarg.h /usr/include/stdarg.h

.include <bsd.kinc.mk>
