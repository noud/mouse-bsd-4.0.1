#	$NetBSD: Makefile,v 1.11 2005/10/27 19:03:58 jmc Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/doc
.PATH:	${DIST}

TEXINFO=	am-utils.texi
INFOFLAGS=	-I${DIST} -I${.CURDIR}

CLEANFILES+=	version.texi

am-utils.info: version.texi

version.texi:
	echo @set UPDATED $$(${HOST_SH} ${DIST}/mdate-sh \
	    ${DIST}/am-utils.texi) > ${.TARGET}
	echo @set UPDATED-MONTH $$(${HOST_SH} ${DIST}/mdate-sh \
	    ${DIST}/am-utils.texi) >> ${.TARGET}
	echo @set EDITION $$( (cat ${.CURDIR}/../include/config.h && \
	    echo VERSION) | ${CPP} -P ${CPPFLAGS} | tr -d \") >> ${.TARGET}
	echo @set VERSION $$( (cat ${.CURDIR}/../include/config.h && \
	    echo VERSION) | ${CPP} -P ${CPPFLAGS} | tr -d \") >> ${.TARGET}


.include <bsd.info.mk>
