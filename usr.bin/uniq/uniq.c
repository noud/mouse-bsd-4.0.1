/*	$NetBSD: uniq.c,v 1.11 2003/08/07 11:16:56 agc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Case Larsen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)uniq.c	8.3 (Berkeley) 5/4/95";
#endif
__RCSID("$NetBSD: uniq.c,v 1.11 2003/08/07 11:16:56 agc Exp $");
#endif /* not lint */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	MAXLINELEN	(8 * 1024)

int cflag, dflag, uflag;
int numchars, numfields, repeats;

FILE	*file __P((char *, char *));
int	 main __P((int, char **));
void	 show __P((FILE *, char *));
char	*skip __P((char *));
void	 obsolete __P((char *[]));
void	 usage __P((void));

int
main (argc, argv)
	int argc;
	char *argv[];
{
	char *t1, *t2;
	FILE *ifp, *ofp;
	int ch;
	char *prevline, *thisline, *p;

	ifp = ofp = NULL;
	obsolete(argv);
	while ((ch = getopt(argc, argv, "-cdf:s:u")) != -1)
		switch (ch) {
		case '-':
			--optind;
			goto done;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			numfields = strtol(optarg, &p, 10);
			if (numfields < 0 || *p)
				errx(1, "illegal field skip value: %s", optarg);
			break;
		case 's':
			numchars = strtol(optarg, &p, 10);
			if (numchars < 0 || *p)
				errx(1, "illegal character skip value: %s",
				    optarg);
			break;
		case 'u':
			uflag = 1;
			break;
		case '?':
		default:
			usage();
	}

done:	argc -= optind;
	argv +=optind;

	/* If no flags are set, default is -d -u. */
	if (cflag) {
		if (dflag || uflag)
			usage();
	} else if (!dflag && !uflag)
		dflag = uflag = 1;

	switch(argc) {
	case 0:
		ifp = stdin;
		ofp = stdout;
		break;
	case 1:
		ifp = file(argv[0], "r");
		ofp = stdout;
		break;
	case 2:
		ifp = file(argv[0], "r");
		ofp = file(argv[1], "w");
		break;
	default:
		usage();
	}

	prevline = malloc(MAXLINELEN);
	thisline = malloc(MAXLINELEN);
	if (prevline == NULL || thisline == NULL)
		err(1, "malloc");

	if (fgets(prevline, MAXLINELEN, ifp) == NULL)
		exit(0);

	while (fgets(thisline, MAXLINELEN, ifp)) {
		/* If requested get the chosen fields + character offsets. */
		if (numfields || numchars) {
			t1 = skip(thisline);
			t2 = skip(prevline);
		} else {
			t1 = thisline;
			t2 = prevline;
		}

		/* If different, print; set previous to new value. */
		if (strcmp(t1, t2)) {
			show(ofp, prevline);
			t1 = prevline;
			prevline = thisline;
			thisline = t1;
			repeats = 0;
		} else
			++repeats;
	}
	show(ofp, prevline);
	exit(0);
}

/*
 * show --
 *	Output a line depending on the flags and number of repetitions
 *	of the line.
 */
void
show(ofp, str)
	FILE *ofp;
	char *str;
{

	if (cflag && *str)
		(void)fprintf(ofp, "%4d %s", repeats + 1, str);
	if ((dflag && repeats) || (uflag && !repeats))
		(void)fprintf(ofp, "%s", str);
}

char *
skip(str)
	char *str;
{
	int infield, nchars, nfields;

	for (nfields = numfields, infield = 0; nfields && *str; ++str)
		if (isspace((unsigned char)*str)) {
			if (infield) {
				infield = 0;
				--nfields;
			}
		} else if (!infield)
			infield = 1;
	for (nchars = numchars; nchars-- && *str; ++str);
	return(str);
}

FILE *
file(name, mode)
	char *name, *mode;
{
	FILE *fp;

	if ((fp = fopen(name, mode)) == NULL)
		err(1, "%s", name);
	return(fp);
}

void
obsolete(argv)
	char *argv[];
{
	char *ap, *p, *start;

	while ((ap = *++argv) != NULL) {
		/* Return if "--" or not an option of any form. */
		if (ap[0] != '-') {
			if (ap[0] != '+')
				return;
		} else if (ap[1] == '-')
			return;
		if (!isdigit((unsigned char)ap[1]))
			continue;
		/*
		 * Digit signifies an old-style option.  Malloc space for dash,
		 * new option and argument.
		 */
		asprintf(&p, "-%c%s", ap[0] == '+' ? 's' : 'f', ap + 1);
		if (!p)
			err(1, "malloc");
		start = p;
		*argv = start;
	}
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: uniq [-c | -du] [-f fields] [-s chars] [input [output]]\n");
	exit(1);
}
