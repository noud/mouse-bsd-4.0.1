/*	$NetBSD: ex_stop.c,v 1.9 2005/02/12 12:53:23 aymeric Exp $	*/

/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char sccsid[] = "@(#)ex_stop.c	10.10 (Berkeley) 3/6/96";
#else
__RCSID("$NetBSD: ex_stop.c,v 1.9 2005/02/12 12:53:23 aymeric Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../common/common.h"

/*
 * ex_stop -- :stop[!]
 *	      :suspend[!]
 *	Suspend execution.
 *
 * PUBLIC: int ex_stop __P((SCR *, EXCMD *));
 */
int
ex_stop(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	int allowed;

	/* For some strange reason, the force flag turns off autowrite. */
	if (!FL_ISSET(cmdp->iflags, E_C_FORCE) && file_aw(sp, FS_ALL))
		return (1);

	if (sp->gp->scr_suspend(sp, &allowed))
		return (1);
	if (!allowed)
		ex_emsg(sp, NULL, EXM_NOSUSPEND);
	return (0);
}
