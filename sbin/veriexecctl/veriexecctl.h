/*	$NetBSD: veriexecctl.h,v 1.9 2006/11/28 22:22:03 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org> 
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#ifndef _VERIEXECCTL_H_
#define _VERIEXECCTL_H_

#define	STATUS_STRING(status)	((status) == FINGERPRINT_NOTEVAL ?	\
					     "not evaluated" :		\
				 (status) == FINGERPRINT_VALID ?	\
					     "valid" :			\
				 (status) == FINGERPRINT_NOMATCH ?	\
					     "mismatch" :		\
					     "<unknown>")

#define dict_sets(d, k, v) \
	prop_dictionary_set(d, k, prop_string_create_cstring(v))

#define dict_gets(d, k) \
	prop_string_cstring_nocopy(prop_dictionary_get(d, k))

#define	dict_setd(d, k, v, n) \
	prop_dictionary_set(d, k, prop_data_create_data(v, n))

#define	dict_getd(d, k) \
	prop_data_data_nocopy(prop_dictionary_get(d, k))

CIRCLEQ_HEAD(veriexec_ups, veriexec_up) params_list;
struct veriexec_up {
	prop_dictionary_t vu_preload;
        CIRCLEQ_ENTRY(veriexec_up) vu_list;
};

extern int gfd, verbose, phase;
extern size_t line;
extern char *infile;
extern FILE *yyin;

int yywrap(void);
int yylex(void);
int yyparse(void);
void yyerror(const char *);
struct veriexec_up *dev_lookup(char *);
struct veriexec_up *dev_add(char *);
void phase2_load(void);

#endif /* _VERIEXECCTL_H_ */
