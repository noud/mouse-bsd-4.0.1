# $NetBSD: Makefile,v 1.12 2003/01/06 17:40:21 lukem Exp $

# Makefile for alpha tags file and boot blocks

TALPHA=	../alpha/tags
SALPHA=	../alpha/alpha/*.[ch] ../alpha/include/*.h \
	../alpha/pci/*.[ch] ../alpha/tlsb/*.[ch] ../alpha/tc/*.[ch]
AALPHA=	../alpha/alpha/*.S

# Directories in which to place tags links
DALPHA=	tc include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TALPHA} ${SALPHA} ${COMM} 
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AALPHA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TALPHA}
	sort -o ${TALPHA} ${TALPHA}

links:
	-for i in ${DALPHA}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
