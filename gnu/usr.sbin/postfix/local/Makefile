#	$NetBSD: Makefile,v 1.4 2002/09/19 03:09:45 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	local

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	alias.c command.c delivered.c dotforward.c file.c forward.c \
	include.c indirect.c local.c mailbox.c recipient.c resolve.c token.c \
	deliver_attr.c maildir.c biff_notify.c unknown.c \
	local_expand.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
