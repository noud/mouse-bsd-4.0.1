/*	$NetBSD: key_25.h,v 1.1.1.3.4.1 2007/05/17 00:41:19 jdc Exp $	*/

/*
 * Copyright (C) 2004, 2005  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef GENERIC_KEY_25_H
#define GENERIC_KEY_25_H 1

/* Id: key_25.h,v 1.15.18.2 2005/04/29 00:16:33 marka Exp */

/*!
 * \brief Per RFC2535 */

typedef struct dns_rdata_key_t {
        dns_rdatacommon_t	common;
        isc_mem_t *		mctx;
        isc_uint16_t		flags;
        isc_uint8_t		protocol;
        isc_uint8_t		algorithm;
        isc_uint16_t		datalen;
        unsigned char *		data;
} dns_rdata_key_t;


#endif /* GENERIC_KEY_25_H */
