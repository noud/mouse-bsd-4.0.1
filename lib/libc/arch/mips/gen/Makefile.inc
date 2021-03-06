#	$NetBSD: Makefile.inc,v 1.28 2006/07/03 12:58:55 drochner Exp $

SRCS+=	fabs.S ldexp.S modf.S

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	isinff_ieee754.c isinfd_ieee754.c
SRCS+=	isnanf_ieee754.c isnand_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c

SRCS+=	nanf.c

SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	setjmp.S longjmp.c
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

SRCS+=	makecontext.c resumecontext.c _resumecontext.S swapcontext.S _lwp.c

# mips abi builtin extensions (used by GCC for lexical-closure trampoline)
SRCS+=	cacheflush.c

SRCS.mips.gen=	Lint__setjmp.c Lint_bswap16.c Lint_bswap32.c Lint_swapcontext.c
LSRCS+=		${SRCS.mips.gen}
DPSRCS+=	${SRCS.mips.gen}
CLEANFILES+=	${SRCS.mips.gen}
