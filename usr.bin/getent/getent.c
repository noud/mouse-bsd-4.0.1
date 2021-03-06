/*	$NetBSD: getent.c,v 1.9 2006/08/27 06:58:55 simonb Exp $	*/

/*-
 * Copyright (c) 2004-2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: getent.c,v 1.9 2006/08/27 06:58:55 simonb Exp $");
#endif /* not lint */

#include <sys/socket.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <netinet/in.h>		/* for INET6_ADDRSTRLEN */

#include <rpc/rpcent.h>

static int	usage(void);
static int	parsenum(const char *, unsigned long *);
static int	ethers(int, char *[]);
static int	group(int, char *[]);
static int	hosts(int, char *[]);
static int	networks(int, char *[]);
static int	passwd(int, char *[]);
static int	protocols(int, char *[]);
static int	rpc(int, char *[]);
static int	services(int, char *[]);
static int	shells(int, char *[]);

enum {
	RV_OK		= 0,
	RV_USAGE	= 1,
	RV_NOTFOUND	= 2,
	RV_NOENUM	= 3,
};

static struct getentdb {
	const char	*name;
	int		(*callback)(int, char *[]);
} databases[] = {
	{	"ethers",	ethers,		},
	{	"group",	group,		},
	{	"hosts",	hosts,		},
	{	"networks",	networks,	},
	{	"passwd",	passwd,		},
	{	"protocols",	protocols,	},
	{	"rpc",		rpc,		},
	{	"services",	services,	},
	{	"shells",	shells,		},

	{	NULL,		NULL,		},
};


int
main(int argc, char *argv[])
{
	struct getentdb	*curdb;

	setprogname(argv[0]);

	if (argc < 2)
		usage();
	for (curdb = databases; curdb->name != NULL; curdb++) {
		if (strcmp(curdb->name, argv[1]) == 0) {
			exit(curdb->callback(argc, argv));
			break;
		}
	}
	fprintf(stderr, "Unknown database: %s\n", argv[1]);
	usage();
	/* NOTREACHED */
	return RV_USAGE;
}

static int
usage(void)
{
	struct getentdb	*curdb;

	fprintf(stderr, "Usage: %s database [key ...]\n",
	    getprogname());
	fprintf(stderr, "       database may be one of:\n\t");
	for (curdb = databases; curdb->name != NULL; curdb++) {
		fprintf(stderr, " %s", curdb->name);
	}
	fprintf(stderr, "\n");
	exit(RV_USAGE);
	/* NOTREACHED */
}

static int
parsenum(const char *word, unsigned long *result)
{
	unsigned long	num;
	char		*ep;

	assert(word != NULL);
	assert(result != NULL);

	if (!isdigit((unsigned char)word[0]))
		return 0;
	errno = 0;
	num = strtoul(word, &ep, 10);
	if (num == ULONG_MAX && errno == ERANGE)
		return 0;
	if (*ep != '\0')
		return 0;
	*result = num;
	return 1;
}

/*
 * printfmtstrings --
 *	vprintf(format, ...),
 *	then the aliases (beginning with prefix, separated by sep),
 *	then a newline
 */
static void
printfmtstrings(char *strings[], const char *prefix, const char *sep,
	const char *fmt, ...)
{
	va_list		ap;
	const char	*curpref;
	int		i;

	va_start(ap, fmt);
	vprintf(fmt, ap);

	curpref = prefix;
	for (i = 0; strings[i] != NULL; i++) {
		printf("%s%s", curpref, strings[i]);
		curpref = sep;
	}
	printf("\n");
}


		/*
		 * ethers
		 */

static int
ethers(int argc, char *argv[])
{
	char		hostname[MAXHOSTNAMELEN + 1], *hp;
	struct ether_addr ea, *eap;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

#define ETHERSPRINT	printf("%-17s  %s\n", ether_ntoa(eap), hp)

	rv = RV_OK;
	if (argc == 2) {
		fprintf(stderr, "Enumeration not supported on ethers\n");
		rv = RV_NOENUM;
	} else {
		for (i = 2; i < argc; i++) {
			if ((eap = ether_aton(argv[i])) == NULL) {
				eap = &ea;
				hp = argv[i];
				if (ether_hostton(hp, eap) != 0) {
					rv = RV_NOTFOUND;
					break;
				}
			} else {
				hp = hostname;
				if (ether_ntohost(hp, eap) != 0) {
					rv = RV_NOTFOUND;
					break;
				}
			}
			ETHERSPRINT;
		}
	}
	return rv;
}

		/*
		 * group
		 */

static int
group(int argc, char *argv[])
{
	struct group	*gr;
	unsigned long	id;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

#define GROUPPRINT	printfmtstrings(gr->gr_mem, ":", ",", "%s:%s:%u", \
			    gr->gr_name, gr->gr_passwd, gr->gr_gid)

	setgroupent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((gr = getgrent()) != NULL)
			GROUPPRINT;
	} else {
		for (i = 2; i < argc; i++) {
			if (parsenum(argv[i], &id))
				gr = getgrgid(id);
			else
				gr = getgrnam(argv[i]);
			if (gr != NULL)
				GROUPPRINT;
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endgrent();
	return rv;
}


		/*
		 * hosts
		 */

static void
hostsprint(const struct hostent *he)
{
	char	buf[INET6_ADDRSTRLEN];

	assert(he != NULL);
	if (inet_ntop(he->h_addrtype, he->h_addr, buf, sizeof(buf)) == NULL)
		strlcpy(buf, "# unknown", sizeof(buf));
	printfmtstrings(he->h_aliases, "  ", " ", "%-16s  %s", buf, he->h_name);
}

static int
hosts(int argc, char *argv[])
{
	struct hostent	*he;
	char		addr[IN6ADDRSZ];
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	sethostent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((he = gethostent()) != NULL)
			hostsprint(he);
	} else {
		for (i = 2; i < argc; i++) {
			if (inet_pton(AF_INET6, argv[i], (void *)addr) > 0)
				he = gethostbyaddr(addr, IN6ADDRSZ, AF_INET6);
			else if (inet_pton(AF_INET, argv[i], (void *)addr) > 0)
				he = gethostbyaddr(addr, INADDRSZ, AF_INET);
			else
				he = gethostbyname(argv[i]);
			if (he != NULL)
				hostsprint(he);
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endhostent();
	return rv;
}


		/*
		 * networks
		 */

static void
networksprint(const struct netent *ne)
{
	char		buf[INET6_ADDRSTRLEN];
	struct	in_addr	ianet;

	assert(ne != NULL);
	ianet = inet_makeaddr(ne->n_net, 0);
	if (inet_ntop(ne->n_addrtype, &ianet, buf, sizeof(buf)) == NULL)
		strlcpy(buf, "# unknown", sizeof(buf));
	printfmtstrings(ne->n_aliases, "  ", " ", "%-16s  %s", ne->n_name, buf);
}

static int
networks(int argc, char *argv[])
{
	struct netent	*ne;
	in_addr_t	net;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	setnetent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((ne = getnetent()) != NULL)
			networksprint(ne);
	} else {
		for (i = 2; i < argc; i++) {
			net = inet_network(argv[i]);
			if (net != INADDR_NONE)
				ne = getnetbyaddr(net, AF_INET);
			else
				ne = getnetbyname(argv[i]);
			if (ne != NULL)
				networksprint(ne);
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endnetent();
	return rv;
}


		/*
		 * passwd
		 */

static int
passwd(int argc, char *argv[])
{
	struct passwd	*pw;
	unsigned long	id;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

#define PASSWDPRINT	printf("%s:%s:%u:%u:%s:%s:%s\n", \
			    pw->pw_name, pw->pw_passwd, pw->pw_uid, \
			    pw->pw_gid, pw->pw_gecos, pw->pw_dir, pw->pw_shell)

	setpassent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((pw = getpwent()) != NULL)
			PASSWDPRINT;
	} else {
		for (i = 2; i < argc; i++) {
			if (parsenum(argv[i], &id))
				pw = getpwuid(id);
			else
				pw = getpwnam(argv[i]);
			if (pw != NULL)
				PASSWDPRINT;
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endpwent();
	return rv;
}


		/*
		 * protocols
		 */

static int
protocols(int argc, char *argv[])
{
	struct protoent	*pe;
	unsigned long	id;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

#define PROTOCOLSPRINT	printfmtstrings(pe->p_aliases, "  ", " ", \
			    "%-16s  %5d", pe->p_name, pe->p_proto)

	setprotoent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((pe = getprotoent()) != NULL)
			PROTOCOLSPRINT;
	} else {
		for (i = 2; i < argc; i++) {
			if (parsenum(argv[i], &id))
				pe = getprotobynumber(id);
			else
				pe = getprotobyname(argv[i]);
			if (pe != NULL)
				PROTOCOLSPRINT;
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endprotoent();
	return rv;
}

		/*
		 * rpc
		 */

static int
rpc(int argc, char *argv[])
{
	struct rpcent	*re;
	unsigned long	id;
	int		i, rv;
	
	assert(argc > 1);
	assert(argv != NULL);

#define RPCPRINT	printfmtstrings(re->r_aliases, "  ", " ", \
				"%-16s  %6d", \
				re->r_name, re->r_number)

	setrpcent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((re = getrpcent()) != NULL)
			RPCPRINT;
	} else {
		for (i = 2; i < argc; i++) {
			if (parsenum(argv[i], &id))
				re = getrpcbynumber(id);
			else
				re = getrpcbyname(argv[i]);
			if (re != NULL)
				RPCPRINT;
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endrpcent();
	return rv;
}

		/*
		 * services
		 */

static int
services(int argc, char *argv[])
{
	struct servent	*se;
	unsigned long	id;
	char		*proto;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

#define SERVICESPRINT	printfmtstrings(se->s_aliases, "  ", " ", \
			    "%-16s  %5d/%s", \
			    se->s_name, ntohs(se->s_port), se->s_proto)

	setservent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((se = getservent()) != NULL)
			SERVICESPRINT;
	} else {
		for (i = 2; i < argc; i++) {
			proto = strchr(argv[i], '/');
			if (proto != NULL)
				*proto++ = '\0';
			if (parsenum(argv[i], &id))
				se = getservbyport(htons(id), proto);
			else
				se = getservbyname(argv[i], proto);
			if (se != NULL)
				SERVICESPRINT;
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endservent();
	return rv;
}


		/*
		 * shells
		 */

static int
shells(int argc, char *argv[])
{
	const char	*sh;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

#define SHELLSPRINT	printf("%s\n", sh)

	setusershell();
	rv = RV_OK;
	if (argc == 2) {
		while ((sh = getusershell()) != NULL)
			SHELLSPRINT;
	} else {
		for (i = 2; i < argc; i++) {
			setusershell();
			while ((sh = getusershell()) != NULL) {
				if (strcmp(sh, argv[i]) == 0) {
					SHELLSPRINT;
					break;
				}
			}
			if (sh == NULL) {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endusershell();
	return rv;
}
