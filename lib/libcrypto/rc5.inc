#	$NetBSD: rc5.inc,v 1.7 2006/03/17 23:10:48 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /users/itojun/NetBSD/basesrc/lib/libcrypto/../../crypto/dist/openssl/crypto/rc5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc5


RC5_SRCS = rc5_ecb.c rc5cfb64.c rc5ofb64.c
# RC5 is a patented algorithm; abort().
# see ../libcrypto_rc5
RC5_SRCS += rc5_skey_dummy.c rc5_enc_dummy.c
#SRCS+=rc5_skey.c rc5_enc.c
SRCS += ${RC5_SRCS}

.for cryptosrc in ${RC5_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rc5
.endfor
