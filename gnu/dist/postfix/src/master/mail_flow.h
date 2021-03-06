/*	$NetBSD: mail_flow.h,v 1.1.1.3 2006/07/19 01:17:31 rpaulo Exp $	*/

#ifndef _MAIL_FLOW_H_INCLUDED_
#define _MAIL_FLOW_H_INCLUDED_

/*++
/* NAME
/*	mail_flow 3h
/* SUMMARY
/*	global mail flow control
/* SYNOPSIS
/*	#include <mail_flow.h>
/* DESCRIPTION
/* .nf

 /*
  * Functional interface.
  */
extern ssize_t mail_flow_get(ssize_t);
extern ssize_t mail_flow_put(ssize_t);
extern ssize_t mail_flow_count(void);

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

#endif
