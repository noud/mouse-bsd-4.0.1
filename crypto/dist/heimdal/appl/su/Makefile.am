# $Heimdal: Makefile.am,v 1.7 2001/08/28 08:31:22 assar Exp $
# $NetBSD: Makefile.am,v 1.1.1.5 2002/09/12 12:41:33 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_krb4) $(INCLUDE_des)

bin_PROGRAMS = su
bin_SUIDS = su
su_SOURCES = su.c

LDADD = $(LIB_kafs) \
	$(top_builddir)/lib/krb5/libkrb5.la \
	$(LIB_krb4) \
	$(LIB_des) \
	$(top_builddir)/lib/asn1/libasn1.la \
	$(LIB_roken)
