#	$NetBSD: Makefile.inc,v 1.24 2005/12/11 12:24:44 christos Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c \
	memchr.c memcmp.c \
	strcat.c strcmp.c strcpy.c strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c \
	scanc.c skpc.c \
	htonl.c htons.c ntohl.c ntohs.c \
	random.c strtoul.c \
	syncicache.c

SRCS+=	ffs.S memset.S strlen.S
