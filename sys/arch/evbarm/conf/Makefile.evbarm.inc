#	$NetBSD: Makefile.evbarm.inc,v 1.17 2005/11/24 12:54:29 dbj Exp $

.if defined(BOARDMKFRAG)	# Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif

.if defined(KERNEL_BASE_PHYS)

LINKFLAGS=	-T ldscript

netbsd: ldscript             # XXX
EXTRA_CLEAN+= ldscript tmp

# generate ldscript from common template 
ldscript: ${THISARM}/conf/ldscript.evbarm ${THISARM}/conf/Makefile.evbarm.inc
	echo ${KERNELS}
	sed -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.endif	# KERNEL_BASE_PHYS
