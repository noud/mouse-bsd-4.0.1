/*	$NetBSD: memccpy.S,v 1.3 1998/04/03 22:58:09 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: memccpy.S,v 1.3 1998/04/03 22:58:09 matthias Exp $")
#endif

/*
 * void *
 * memccpy (void *d, const void *s, int c, size_t len)
 *	copy at most len bytes from the string s to d and stop on c.
 */

KENTRY(memccpy, 16)
	enter	[r4],0
	movd	B_ARG3,r0
	movd	B_ARG2,r4
	movd	B_ARG1,r1
	movd	B_ARG0,r2
	movsb	u
	bfc	0f
	movb	r4,0(r2)
	addr	1(r2),r0
0:	exit	[r4]
	ret	ARGS
