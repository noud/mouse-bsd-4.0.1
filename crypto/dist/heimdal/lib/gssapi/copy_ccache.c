/*
 * Copyright (c) 2000 - 2001, 2003 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include "gssapi_locl.h"

__RCSID("$Heimdal: copy_ccache.c,v 1.3 2003/03/16 17:47:44 lha Exp $"
        "$NetBSD: copy_ccache.c,v 1.5 2003/05/15 21:36:41 lha Exp $");

OM_uint32
gss_krb5_copy_ccache(OM_uint32 *minor_status,
		     gss_cred_id_t cred,
		     krb5_ccache out)
{
    krb5_error_code kret;

    if (cred->ccache == NULL) {
 	*minor_status = EINVAL;
	return GSS_S_FAILURE;
    }

    kret = krb5_cc_copy_cache(gssapi_krb5_context, cred->ccache, out);
    if (kret) {
	*minor_status = kret;
	gssapi_krb5_set_error_string ();
	return GSS_S_FAILURE;
    }
    *minor_status = 0;
    return GSS_S_COMPLETE;
}
