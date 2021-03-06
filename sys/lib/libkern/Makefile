#	$NetBSD: Makefile,v 1.84 2006/11/13 03:26:43 dyoung Exp $

LIB=		kern
NOPIC=		# defined
LLIBS=		# defined

.include <bsd.own.mk>

.if defined(LIBKERN_ARCH) && !empty(LIBKERN_ARCH) && \
    exists(${KERNDIR}/arch/${LIBKERN_ARCH})
ARCHSUBDIR=     ${LIBKERN_ARCH}
.elif defined(MACHINE_ARCH) && !empty(MACHINE_ARCH) && \
    exists(${KERNDIR}/arch/${MACHINE_ARCH})
ARCHSUBDIR=     ${MACHINE_ARCH}
.elif defined(MACHINE_CPU) && !empty(MACHINE_CPU) && \
    exists(${KERNDIR}/arch/${MACHINE_CPU})
ARCHSUBDIR=     ${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

M= ${KERNDIR}/arch/${ARCHSUBDIR}

CPPFLAGS=	-I$M ${KERNCPPFLAGS} ${KERNMISCCPPFLAGS}

.include "${.PARSEDIR}/../../../common/lib/libc/Makefile.inc"
.include "${.PARSEDIR}/../../../common/lib/libprop/Makefile.inc"

CPPFLAGS+=	-I${KERNDIR}/../../../common/include

.PATH.c: ${KERNDIR}
.if exists ($M/Makefile.inc)
.PATH.c: $M
.PATH.S: $M
.include "$M/Makefile.inc"
.endif

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.endif

# Other stuff
SRCS+=	__cmsg_alignbytes.c inet_addr.c intoa.c
SRCS+=	md4c.c md5c.c rmd160.c sha1.c sha2.c
SRCS+=	pmatch.c _que.c arc4random.c bcd.c mcount.c

SRCS+=	strsep.c strstr.c strlcpy.c strlcat.c

SRCS+=	strtoll.c strtoull.c strtoumax.c

SRCS+=	xlat_mbr_fstype.c

# Files to clean up
CLEANFILES+= lib${LIB}.o lib${LIB}.po

# XXX these stub builds should be cleaned up even further.

# if no machine specific memcpy(3), build one out of bcopy(3) based stub.
.if empty(SRCS:Mmemcpy.S)
SRCS+=	memcpy.c
.endif

# if no machine specific memmove(3), build one out of bcopy(3) based stub.
.if empty(SRCS:Mmemmove.S)
SRCS+=	memmove.c
.endif

# if no machine specific strchr(3), build one out of index(3) based stub.
.if empty(SRCS:Mstrchr.S)
SRCS+=	strchr.c
.endif

# if no machine specific strrchr(3), build one out of rindex(3) based stub.
.if empty(SRCS:Mstrrchr.S)
SRCS+=	strrchr.c
.endif

# only needed during build
libinstall::

.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `NM=${NM} ${LORDER} ${OBJS} | ${TSORT}`

lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `NM=${NM} ${LORDER} ${POBJS} | ${TSORT}`

showsources: ${SRCS}
	@echo ${.ALLSRC}
