#	$NetBSD: Makefile.inc,v 1.5 2004/07/21 20:27:46 tshiozak Exp $

# sources
.PATH: ${ARCHDIR}/citrus ${.CURDIR}/citrus

CITRUS?=	yes

.if ${CITRUS} == "yes"
MODULE_SHLIB_MAJOR!= . ${NETBSDSRCDIR}/lib/i18n_module/shlib_version ; echo $$major
CPPFLAGS+= -DI18NMODULE_MAJOR=${MODULE_SHLIB_MAJOR} -DCITRUS

SRCS+=	citrus_bcs.c citrus_csmapper.c citrus_ctype.c citrus_ctype_fallback.c \
	citrus_db.c citrus_db_hash.c citrus_esdb.c citrus_hash.c \
	citrus_iconv.c citrus_lookup.c \
	citrus_mapper.c citrus_memstream.c citrus_mmap.c citrus_module.c \
	citrus_none.c citrus_stdenc.c

.endif
