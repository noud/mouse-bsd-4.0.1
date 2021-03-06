/*	$NetBSD: ntp_debug.h,v 1.1.1.1.4.3 2007/08/21 13:22:26 ghen Exp $	*/

/*
 * $Header: /cvsroot/src/dist/ntp/include/ntp_debug.h,v 1.1.1.1.4.3 2007/08/21 13:22:26 ghen Exp $
 *
 * Created: Sat Aug 20 14:23:01 2005
 *
 * Copyright (C) 2005 by Frank Kardel
 */
#ifndef NTP_DEBUG_H
#define NTP_DEBUG_H

/*
 * macros for debugging output - cut down on #ifdef pollution in the code
 */

#ifdef DEBUG
#define DPRINTF(_lvl_, _arg_)                   \
        if (debug >= (_lvl_))                   \
                printf _arg_;
#else
#define DPRINTF(_lvl_, _arg_)
#endif

#endif
