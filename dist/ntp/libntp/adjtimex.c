/*	$NetBSD: adjtimex.c,v 1.3 2003/12/04 16:23:36 drochner Exp $	*/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz  "11/19/91       ULTRIX  @(#)adjtime.c   6.1"
#endif not lint

#include "SYS.h"

SYSCALL(adjtimex)
        ret

