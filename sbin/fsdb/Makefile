#	$NetBSD: Makefile,v 1.20 2006/08/27 01:37:28 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	fsdb
MAN=	fsdb.8
SRCS=	fsdb.c fsdbutil.c
SRCS+=	dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+=	ffs_bswap.c ffs_subr.c ffs_tables.c ffs_appleufs.c
SRCS+=	fsutil.c partutil.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
FSCK_FFS=${NETBSDSRCDIR}/sbin/fsck_ffs
CPPFLAGS+= -I${FSCK} -I${FSCK_FFS}
.PATH:	${FSCK} ${FSCK_FFS} ${NETBSDSRCDIR}/sys/ufs/ffs

LDADD+= -lutil -ledit -ltermcap
.ifndef HOSTPROG
DPADD+= ${LIBUTIL} ${LIBEDIT} ${LIBTERMCAP}
.endif

.if ${HAVE_GCC} == 4
.for f in fsdb ffs_appleufs
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor
.endif

.if ${HAVE_GCC} == 4 && ${MACHINE_ARCH} == "m68000"
COPTS.pass1.c+=	-O0
.endif

.include <bsd.prog.mk>
