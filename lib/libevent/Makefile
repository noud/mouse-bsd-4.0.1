#	$NetBSD: Makefile,v 1.3 2005/04/17 07:20:00 provos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

NOLINT=		# Until someone explains to me how to avoid lint stupidity
USE_SHLIBDIR=	yes

CPPFLAGS+=-DHAVE_CONFIG_H
.include <bsd.own.mk>

LIB=	event
SRCS=	buffer.c evbuffer.c event.c kqueue.c poll.c signal.c log.c

INCS=	event.h
INCSDIR=/usr/include

MAN=	event.3

MLINKS+=event.3 event_init.3
MLINKS+=event.3 event_dispatch.3
MLINKS+=event.3 event_loop.3
MLINKS+=event.3 event_set.3
MLINKS+=event.3 event_add.3
MLINKS+=event.3 event_del.3
MLINKS+=event.3 event_pending.3
MLINKS+=event.3 event_initialized.3
MLINKS+=event.3 signal_set.3
MLINKS+=event.3 signal_add.3
MLINKS+=event.3 signal_del.3
MLINKS+=event.3 signal_pending.3
MLINKS+=event.3 signal_initialized.3

.include <bsd.lib.mk>
