#	$NetBSD: Makefile,v 1.29 2006/08/08 18:41:11 wiz Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

NOOBJ=	# defined

FILES=	NetBSD.el acronyms acronyms.comp airport ascii \
	birthtoken bsd-family-tree \
	country domains \
	eqnchar flowers indent.pro inter.phone language man.template \
	mdoc.template na.phone na.postal operator style zipcodes
FILESDIR=${BINDIR}/misc


# According to iana@ISI.EDU, the URL below is the authoritative list
# distributed by the ISO 3166 Maintenance Agency.
update-domains:
	( \
	  mkdir /tmp/domains.$$$$; cd /tmp/domains.$$$$ ; \
	  ftp -o - http://www.iana.org/cctld/cctld-whois.htm | \
	  sed -f ${.CURDIR}/domains.sed > list ;\
	  echo '# $$''NetBSD: $$' ;  \
	  echo '# ISO 3166 country codes, from http://www.iana.org/cctld/cctld-whois.htm' ; \
	  cat top ; echo '#' ; cat list ; \
	 ) > domains ; \
	rm -r /tmp/domains.$$$$

update-na.phone:
	< /dev/null awk -f nanpa.awk > na.phone

.include <bsd.prog.mk>
