# $NetBSD: Makefile,v 1.20 2006/05/11 15:37:07 sjg Exp $
#
# Unit tests for make(1)
# The main targets are:
# 
# all:	run all the tests
# test:	run 'all', capture output and compare to expected results
# accept: move generated output to expected results
#
# Adding a test case.  
# Each feature should get its own set of tests in its own suitably
# named makefile which should be added to SUBFILES to hook it in.
# 

.MAIN: all

UNIT_TESTS:= ${.PARSEDIR}

# Simple sub-makefiles - we run them as a black box
# keep the list sorted.
SUBFILES= \
	comment \
	cond1 \
	dotwait \
	moderrs \
	modmatch \
	modmisc \
	modorder \
	modts \
	modword \
	posix \
	ternary \
	varcmd

all: ${SUBFILES}

# the tests are actually done with sub-makes.
.PHONY: ${SUBFILES}
.PRECIOUS: ${SUBFILES}
${SUBFILES}:
	-@${.MAKE} -k -f ${UNIT_TESTS}/$@

clean:
	rm -f *.out *.fail *.core

.include <bsd.obj.mk>

TEST_MAKE?= ${.MAKE}

# The driver.
# We always pretend .MAKE was called 'make' 
# and strip ${.CURDIR}/ from the output
# and replace anything after 'stopped in' with unit-tests
# so the results can be compared.
test:
	@echo "${TEST_MAKE} -f ${MAKEFILE} > ${.TARGET}.out 2>&1"
	@cd ${.OBJDIR} && ${TEST_MAKE} -f ${MAKEFILE} 2>&1 | \
	sed -e 's,^${TEST_MAKE:T:C/\./\\\./g}:,make:,' \
	-e '/stopped/s, /.*, unit-tests,' \
	-e 's,${.CURDIR:C/\./\\\./g}/,,g' \
	-e 's,${UNIT_TESTS:C/\./\\\./g}/,,g' > ${.TARGET}.out || { \
	tail ${.TARGET}.out; mv ${.TARGET}.out ${.TARGET}.fail; exit 1; }
	diff -u ${UNIT_TESTS}/${.TARGET}.exp ${.TARGET}.out

accept:
	mv test.out ${.CURDIR}/test.exp

