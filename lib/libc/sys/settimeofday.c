/*	$NetBSD: settimeofday.c,v 1.10 2006/10/07 20:02:01 kardel Exp $ */

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.      
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: settimeofday.c,v 1.10 2006/10/07 20:02:01 kardel Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <sys/clockctl.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
 
#ifdef __weak_alias
__weak_alias(settimeofday,_settimeofday)
#endif 

int __clockctl_fd = -1;

int
settimeofday(tv, tzp)
	const struct timeval *tv;
	const void *tzp;
{
	struct clockctl_settimeofday args;
	quad_t q;
	int rv;

	/*
	 * try syscal first and attempt to switch to clockctl
	 * if that fails with EPERM
	 */
	if (__clockctl_fd == -1) {
		q = __syscall((quad_t)SYS_settimeofday, tv, tzp);
		if (/* LINTED constant */ sizeof (quad_t) == sizeof (register_t)
		    || /* LINTED constant */ BYTE_ORDER == LITTLE_ENDIAN)
			rv = (int)q;
		else
			rv = (int)((u_quad_t)q >> 32); 
	
		/*
		 * switch to clockctl if we fail with EPERM, this
		 * may be cause by an attempt to set the time backwards
		 * but we should leave the access permission checking
		 * entirely to the kernel
		 */
		if (rv != -1 || errno != EPERM)
			return rv;

		__clockctl_fd = open(_PATH_CLOCKCTL, O_WRONLY, 0);
		if (__clockctl_fd == -1) {
			/* original error was EPERM - don't leak open errors */
			errno = EPERM;
			return -1;
		}

		(void) fcntl(__clockctl_fd, F_SETFD, FD_CLOEXEC);
	}

	/* 
	 * If __clockctl_fd >=0, clockctl has already been open
	 * and used, so we carry on using it.
	 */
	args.tv = tv;
	args.tzp = tzp;
	return ioctl(__clockctl_fd, CLOCKCTL_SETTIMEOFDAY, &args);
}
