/*	$NetBSD: pipe.S,v 1.7 2003/11/13 06:29:39 simonb Exp $	*/

/* 
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * HELSINKI UNIVERSITY OF TECHNOLOGY ALLOWS FREE USE OF THIS SOFTWARE IN
 * ITS "AS IS" CONDITION. HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIMS ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * 29-Apr-92  Johannes Helander (jvh) at Helsinki University of Technology
 *	Created.
 */

#include "SYS.h"

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: pipe.S,v 1.7 2003/11/13 06:29:39 simonb Exp $")
#endif

#ifdef WEAK_ALIAS
WEAK_ALIAS(pipe,_pipe)
#endif

_SYSCALL(_pipe,pipe)
	movd	S_ARG0,r2
	movd	r0,0(r2)
	movd	r1,4(r2)
	movqd	0,r0
	ret	0
