/*	$NetBSD: secalg.h,v 1.1.1.3.4.1 2007/05/17 00:41:05 jdc Exp $	*/

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

/* Id: secalg.h,v 1.13.18.2 2005/04/29 00:16:21 marka Exp */

#ifndef DNS_SECALG_H
#define DNS_SECALG_H 1

/*! \file */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_secalg_fromtext(dns_secalg_t *secalgp, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNSSEC security algorithm value.
 * The text may contain either a mnemonic algorithm name or a decimal algorithm
 * number.
 *
 * Requires:
 *\li	'secalgp' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_RANGE			numeric type is out of range
 *\li	DNS_R_UNKNOWN			mnemonic type is unknown
 */

isc_result_t
dns_secalg_totext(dns_secalg_t secalg, isc_buffer_t *target);
/*%<
 * Put a textual representation of the DNSSEC security algorithm 'secalg'
 * into 'target'.
 *
 * Requires:
 *\li	'secalg' is a valid secalg.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures,
 *	if the result is success:
 *\li		The used space in 'target' is updated.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	ISC_R_NOSPACE			target buffer is too small
 */

ISC_LANG_ENDDECLS

#endif /* DNS_SECALG_H */
