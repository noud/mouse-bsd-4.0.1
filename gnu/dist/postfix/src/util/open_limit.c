/*	$NetBSD: open_limit.c,v 1.1.1.2 2004/05/31 00:25:00 heas Exp $	*/

/*++
/* NAME
/*	open_limit 3
/* SUMMARY
/*	set/get open file limit
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	open_limit(int limit)
/* DESCRIPTION
/*	The \fIopen_limit\fR() routine attempts to change the maximum
/*	number of open files to the specified limit.  Specify a null
/*	argument to effect no change. The result is the actual open file
/*	limit for the current process. The number can be smaller or larger
/*	than the requested limit.
/* DIAGNOSTICS
/*	open_limit() returns -1 in case of problems. The errno
/*	variable gives hints about the nature of the problem.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System libraries. */

#include "sys_defs.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

/* Application-specific. */

#include "iostuff.h"

 /*
  * 44BSD compatibility.
  */
#ifndef RLIMIT_NOFILE
#ifdef RLIMIT_OFILE
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
#endif

/* open_limit - set/query file descriptor limit */

int     open_limit(int limit)
{
#ifdef RLIMIT_NOFILE
    struct rlimit rl;
#endif

    if (limit < 0) {
	errno = EINVAL;
	return (-1);
    }
#ifdef RLIMIT_NOFILE
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
	return (-1);
    if (limit > 0) {
	if (limit > rl.rlim_max)
	    rl.rlim_cur = rl.rlim_max;
	else
	    rl.rlim_cur = limit;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
	    return (-1);
    }
    return (rl.rlim_cur);
#endif

#ifndef RLIMIT_NOFILE
    return (getdtablesize());
#endif
}

