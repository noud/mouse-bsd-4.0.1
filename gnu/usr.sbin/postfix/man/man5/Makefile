#	$NetBSD: Makefile,v 1.11 2006/09/19 20:10:23 wiz Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/man/man5
.PATH:	${DIST}

MAN=	access.5 aliases.5 bounce.5 canonical.5 relocated.5 \
	transport.5 virtual.5 pcre_table.5 regexp_table.5 \
	cidr_table.5 header_checks.5 master.5  \
	nisplus_table.5 generic.5
MAN+=	postconf.5

MLINKS=	header_checks.5 body_checks.5

# XXX notyet:
# The base install does include support for these:
# ldap_table.5 mysql_table.5 pgsql_table.5

.include <bsd.man.mk>
