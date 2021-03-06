#	$NetBSD: Makefile,v 1.54.2.2 2007/03/22 20:25:56 jdc Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"
GREP?= grep

SETSCMD=	cd ${.CURDIR} && \
		DESTDIR=${DESTDIR:Q} \
		MACHINE=${MACHINE:Q} MACHINE_ARCH=${MACHINE_ARCH:Q} \
		CKSUM=${TOOL_CKSUM:Q} \
		DB=${TOOL_DB:Q} \
		HOST_SH=${HOST_SH:Q} \
		MAKE=${MAKE:Q} \
		MKTEMP=${TOOL_MKTEMP:Q} \
		MTREE=${TOOL_MTREE:Q} \
		PAX=${TOOL_PAX:Q} \
		TSORT=${TSORT:Q} \
		${HOST_SH}

.if ${MKX11} != "no"
MAKEFLIST_FLAGS=	-b
MAKESRCTARS_FLAGS=	-x ${X11SRCDIR} -N ${NETBSDSRCDIR}/etc
.else
MAKEFLIST_FLAGS=
MAKESRCTARS_FLAGS= 	-N ${NETBSDSRCDIR}/etc
.endif

.if !defined(MAKETARSETS)
MAKETARSETS!= ${SETSCMD} ./makeflist -l ${MAKEFLIST_FLAGS}
.endif

print_have_gcc: .PHONY
	@echo ${HAVE_GCC}

print_machine: .PHONY
	@echo ${MACHINE}

print_machine_arch: .PHONY
	@echo ${MACHINE_ARCH}

print_machine_cpu: .PHONY
	@echo ${MACHINE_CPU}

print_object_fmt: .PHONY
	@echo ${OBJECT_FMT}

print_toolchain_missing: .PHONY
	@echo "${TOOLCHAIN_MISSING}"

print_x11_version: .PHONY
	${HOST_SH} -c '. ${.CURDIR}/sets.subr; echo $$x11_version'

# This target has debugging value only, really.
makeflist: .PHONY .PRECIOUS check_DESTDIR
	${SETSCMD} ./makeflist ${MAKEFLIST_FLAGS}

#
# SET BUILDING TARGETS
#

TARDIR=		${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets
SOURCETARDIR=	${RELEASEDIR}/source/sets

.if ${MKUNPRIVED} == "no"
METALOG.unpriv=
METALOG.sanitised.unpriv=
.else
METALOG.unpriv=	-M ${METALOG}
METALOG.sanitised= ${METALOG}.sanitised
METALOG.sanitised.unpriv= -M ${METALOG.sanitised}
.endif

.if ${MKUNPRIVED} == "no"
sanitise_METALOG: .PHONY
	@true
.else
sanitise_METALOG: .PHONY ${METALOG.sanitised}
${METALOG.sanitised}: ${METALOG}
	( cat ${NETBSDSRCDIR}/etc/mtree/NetBSD.dist ; \
		echo '/unset all' ; \
		cat ${METALOG} ) \
	| ${TOOL_MTREE} -C -k all -N ${NETBSDSRCDIR}/etc > ${METALOG}.new && \
		( rm -f ${METALOG} ; \
		mv ${METALOG}.new ${METALOG} )
	cat ${METALOG} | ${GREP} -v " optional" > ${.TARGET}
.endif

.if defined(DESTDIR) && ${DESTDIR} != ""
checkflist_if_DESTDIR: checkflist
.else
checkflist_if_DESTDIR:
.endif

checkflist: .PRECIOUS .PHONY check_DESTDIR sanitise_METALOG
	${SETSCMD} ${.CURDIR}/checkflist \
	    ${MAKEFLIST_FLAGS} ${CHECKFLIST_FLAGS} ${METALOG.sanitised.unpriv}

checkflist-x11: .PHONY check_DESTDIR
	${SETSCMD} ./checkflist -x ${CHECKFLIST_FLAGS}


# Note that the .WAIT inside the ${MAKETARSETS:@...} loop is not
# logically necessary, but many systems exhibit poor performance when
# running multiple parallel disk-intensive processes.
maketars: .PRECIOUS .PHONY check_DESTDIR check_RELEASEDIR \
		sanitise_METALOG checkflist_if_DESTDIR .WAIT \
		maketarsetup .WAIT ${MAKETARSETS:@.TARS.@.WAIT do-${.TARS.}@}
	@true

maketarsetup: .EXEC
.if defined(DESTDIR) && ${DESTDIR} != ""
	${_MKMSG} "execute  checkflist"
	${MAKE} checkflist
.endif
	mkdir -p ${TARDIR}
	for i in BSDSUM CKSUM MD5 SHA512 SYSVSUM; do \
		rm -f ${TARDIR}/$$i ${TARDIR}/$$i.tmp; \
	done

makesetfiles: .PHONY
	${_MKMSG_CREATE} "set lists"
	${SETSCMD} ./maketars -S -d ${DESTDIR:S,^$,/,} ${METALOG.unpriv} \
	    -N ${NETBSDSRCDIR}/etc ${MAKEFLIST_FLAGS} -t ${TARDIR}

.for tar in ${MAKETARSETS}
do-${tar}: .PHONY sanitise_METALOG
	${SETSCMD} ./maketars -d ${DESTDIR:S,^$,/,} \
	    ${METALOG.sanitised.unpriv} \
	    -N ${NETBSDSRCDIR}/etc -t ${TARDIR} ${tar} \
	|| rm -f ${TARDIR}/${tar}.tgz
.endfor


makesrctars: .PRECIOUS .PHONY check_RELEASEDIR
	mkdir -p ${SOURCETARDIR}
	${SETSCMD} ./makesrctars ${MAKESRCTARS_FLAGS} \
	    ${NETBSDSRCDIR} ${SOURCETARDIR}


makesums: .PRECIOUS .PHONY check_RELEASEDIR .WAIT \
		${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}
	for i in BSDSUM CKSUM MD5 SHA512 SYSVSUM; do \
		mv ${TARDIR}/$$i.tmp ${TARDIR}/$$i; \
	done

.for tar in ${MAKETARSETS}
do-sum-${tar}: .PHONY do-${tar}
	${MAKESUMS} -t ${TARDIR} ${tar}.tgz
	for i in BSDSUM CKSUM MD5 SHA512 SYSVSUM; do \
		${TOOL_CAT} ${TARDIR}/$$i >> ${TARDIR}/$$i.tmp; \
	done
.endfor
.ORDER: ${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}


installsets: .PHONY check_DESTDIR sanitise_METALOG
.if !defined(INSTALLDIR)
	@echo "setenv INSTALLDIR before doing that!"
	@false
.endif
	${SETSCMD} ./maketars -d ${DESTDIR:S,^$,/,} \
	    ${METALOG.sanitised.unpriv} -N ${NETBSDSRCDIR}/etc \
	    ${MAKEFLIST_FLAGS} -i ${INSTALLDIR} ${INSTALLSETS}

# Should we ignore errors like extra or missing files in the flists?
SLOPPY_FLIST?= NO
.if !empty(SLOPPY_FLIST:M[Yy][Ee][Ss])
CHECKFLIST_FLAGS+=	-e -m
REGPKG.sloppy=		-m
.endif

.if ${MAKEVERBOSE} == 0
REGPKG.verbose?= -q
.elif ${MAKEVERBOSE} == 1
REGPKG.verbose?=
.else	# MAKEVERBOSE == 2
REGPKG.verbose?= -v
.endif
REGPKG.force?=		# -f, or empty
REGPKG.cache?= -c	# -c, or empty
REGPKG.update:= ${MKUPDATE:tl:Nno:C/..*/-u/}
SYSPKGSETS?= all
makesyspkgs: .PHONY check_DESTDIR check_RELEASEDIR \
		sanitise_METALOG checkflist_if_DESTDIR
	mkdir -p ${RELEASEDIR}/${MACHINE}/binary/syspkgs
	${SETSCMD} ${.CURDIR}/regpkgset \
	    ${REGPKG.verbose} ${REGPKG.force} ${REGPKG.sloppy} \
	    ${REGPKG.cache} ${REGPKG.update} \
	    -d ${DESTDIR:S,^$,/,} ${METALOG.sanitised.unpriv} \
	    -N ${NETBSDSRCDIR}/etc \
	    -t ${RELEASEDIR}/${MACHINE}/binary/syspkgs ${SYSPKGSETS}

makesyspkgsums: .PHONY check_RELEASEDIR
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/binary/syspkgs

# Update the "deps" file.
# XXX: Why is "deps" checked in to the source tree, instead of
# just being created as necessary?
makedeps: .PHONY
	${SETSCMD} ./syspkgdeps all >${.CURDIR}/deps

#
# MAIN ENTRY POINTS
#

syspkgs: .PHONY makesyspkgs .WAIT makesyspkgsums
	@true

sets: .PHONY maketars .WAIT makesums
	@true

sourcesets: .PHONY makesrctars
	@true

.include <bsd.sys.mk>
