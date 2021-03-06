/*	$NetBSD: rootns.h,v 1.1.1.3.4.1 2007/05/17 00:41:03 jdc Exp $	*/

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

/* Id: rootns.h,v 1.9.18.3 2005/04/27 05:01:38 sra Exp */

#ifndef DNS_ROOTNS_H
#define DNS_ROOTNS_H 1

/*! \file */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rootns_create(isc_mem_t *mctx, dns_rdataclass_t rdclass,
                  const char *filename, dns_db_t **target);

void
dns_root_checkhints(dns_view_t *view, dns_db_t *hints, dns_db_t *db);
/*
 * Reports differences between hints and the real roots.
 *
 * Requires view, hints and (cache) db to be valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ROOTNS_H */
