#	$NetBSD: Makefile,v 1.3 2006/08/06 17:21:58 dyoung Exp $

# Makefile for evbmips tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

# XXX !  fix this
TEVBMIPS=	${SYSDIR}/arch/evbmips/tags
SEVBMIPS=	${SYSDIR}/arch/evbmips/evbmips/*.[ch] \
		${SYSDIR}/arch/evbmips/include/*.h \
		${SYSDIR}/arch/evbmips/isa/*.[ch]
AEVBMIPS=	${SYSDIR}/arch/evbmips/evbmips/*.S

# Directories in which to place tags links
DEVBMIPS= evbmips include isa pci

.include "../../kern/Make.tags.inc"

tags:
	-echo ${SEVBMIPS} ${SMIPS} ${COMM} | xargs ctags -wadtf ${TEVBMIPS}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${AEVBMIPS} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TEVBMIPS}
	sort -o ${TEVBMIPS} ${TEVBMIPS}

links:
	-for i in ${DEVBMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
