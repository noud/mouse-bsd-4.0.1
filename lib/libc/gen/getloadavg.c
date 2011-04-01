/*	$NetBSD: getloadavg.c,v 1.13 2003/08/07 16:42:50 agc Exp $	*/

/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getloadavg.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: getloadavg.c,v 1.13 2003/08/07 16:42:50 agc Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <uvm/uvm_param.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#ifdef __weak_alias
__weak_alias(getloadavg,_getloadavg)
#endif

/*
 * getloadavg() -- Get system load averages.
 *
 * Put `nelem' samples into `loadavg' array.
 * Return number of samples retrieved, or -1 on error.
 */
int
getloadavg(loadavg, nelem)
	double loadavg[];
	int nelem;
{
	struct loadavg loadinfo;
	int i, mib[2];
	size_t size;

	_DIAGASSERT(loadavg != NULL);
	_DIAGASSERT(nelem >= 0);

	mib[0] = CTL_VM;
	mib[1] = VM_LOADAVG;
	size = sizeof(loadinfo);
	if (sysctl(mib, 2, &loadinfo, &size, NULL, 0) < 0)
		return (-1);

	nelem = MIN((size_t) nelem, sizeof(loadinfo.ldavg) / sizeof(fixpt_t));
	for (i = 0; i < nelem; i++)
		loadavg[i] = (double) loadinfo.ldavg[i] / loadinfo.fscale;
	return (nelem);
}
