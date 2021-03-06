/*	$NetBSD: fmt.c,v 1.25 2006/01/15 14:41:45 christos Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)fmt.c	8.1 (Berkeley) 7/20/93";
#endif
__RCSID("$NetBSD: fmt.c,v 1.25 2006/01/15 14:41:45 christos Exp $");
#endif /* not lint */

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"

/*
 * fmt -- format the concatenation of input files or standard input
 * onto standard output.  Designed for use with Mail ~|
 *
 * Syntax : fmt [ goal [ max ] ] [ name ... ]
 * Authors: Kurt Shoens (UCB) 12/7/78;
 *          Liz Allen (UMCP) 2/24/83 [Addition of goal length concept].
 */

/* LIZ@UOM 6/18/85 --New variables goal_length and max_length */
#define GOAL_LENGTH 65
#define MAX_LENGTH 75
static size_t	goal_length;	/* Target or goal line length in output */
static size_t	max_length;	/* Max line length in output */
static size_t	pfx;		/* Current leading blank count */
static int	lineno;		/* Current input line */
static int	mark;		/* Last place we saw a head line */
static int	center;
static struct buffer outbuf;

static const char	*headnames[] = {"To", "Subject", "Cc", 0};

static void	fmt(FILE *);
static int	ispref(const char *, const char *);
static void	leadin(void);
static void	oflush(void);
static void	pack(const char *, size_t);
static void	prefix(const struct buffer *, int);
static void	split(const char *, int);
static void	tabulate(struct buffer *);


int		ishead(const char *);

/*
 * Drive the whole formatter by managing input files.  Also,
 * cause initialization of the output stuff and flush it out
 * at the end.
 */

int
main(int argc, char **argv)
{
	FILE *fi;
	int errs = 0;
	int number;		/* LIZ@UOM 6/18/85 */

	goal_length = GOAL_LENGTH;
	max_length = MAX_LENGTH;
	buf_init(&outbuf);
	lineno = 1;
	mark = -10;

	setprogname(*argv);
	(void)setlocale(LC_ALL, "");

	/*
	 * LIZ@UOM 6/18/85 -- Check for goal and max length arguments 
	 */
	if (argc > 1 && !strcmp(argv[1], "-C")) {
		center++;
		argc--;
		argv++;
	}
	if (argc > 1 && (1 == (sscanf(argv[1], "%d", &number)))) {
		argv++;
		argc--;
		goal_length = abs(number);
		if (argc > 1 && (1 == (sscanf(argv[1], "%d", &number)))) {
			argv++;
			argc--;
			max_length = abs(number);
		}
	}
	if (max_length <= goal_length) {
		errx(1, "Max length (%zu) must be greater than goal "
		    "length (%zu)", max_length, goal_length);
	}
	if (argc < 2) {
		fmt(stdin);
		oflush();
		return 0;
	}
	while (--argc) {
		if ((fi = fopen(*++argv, "r")) == NULL) {
			warn("Cannot open `%s'", *argv);
			errs++;
			continue;
		}
		fmt(fi);
		(void)fclose(fi);
	}
	oflush();
	buf_end(&outbuf);
	return errs;
}

/*
 * Read up characters from the passed input file, forming lines,
 * doing ^H processing, expanding tabs, stripping trailing blanks,
 * and sending each line down for analysis.
 */
static void
fmt(FILE *fi)
{
	struct buffer lbuf, cbuf;
	char *cp, *cp2;
	int c, add_space;
	size_t len, col;

	if (center) {
		for (;;) {
			cp = fgetln(fi, &len);
			if (!cp)
				return;
			cp2 = cp + len - 1;
			while (len-- && isspace((unsigned char)*cp))
				cp++;
			while (cp2 > cp && isspace((unsigned char)*cp2))
				cp2--;
			if (cp == cp2)
				(void)putchar('\n');
			col = cp2 - cp;
			if (goal_length > col)
				for (c = 0; c < (goal_length - col) / 2; c++)
					(void)putchar(' ');
			while (cp <= cp2)
				(void)putchar(*cp++);
			(void)putchar('\n');
		}
	}

	buf_init(&lbuf);
	buf_init(&cbuf);
	c = getc(fi);

	while (c != EOF) {
		/*
		 * Collect a line, doing ^H processing.
		 * Leave tabs for now.
		 */
		buf_reset(&lbuf);
		while (c != '\n' && c != EOF) {
			if (c == '\b') {
				(void)buf_unputc(&lbuf);
				c = getc(fi);
				continue;
			}
			if(!(isprint(c) || c == '\t' || c >= 160)) {
				c = getc(fi);
				continue;
			}
			buf_putc(&lbuf, c);
			c = getc(fi);
		}
		buf_putc(&lbuf, '\0');
		(void)buf_unputc(&lbuf);
		add_space = c != EOF;

		/*
		 * Expand tabs on the way.
		 */
		col = 0;
		cp = lbuf.bptr;
		buf_reset(&cbuf);
		while ((c = *cp++) != '\0') {
			if (c != '\t') {
				col++;
				buf_putc(&cbuf, c);
				continue;
			}
			do {
				buf_putc(&cbuf, ' ');
				col++;
			} while ((col & 07) != 0);
		}

		/*
		 * Swipe trailing blanks from the line.
		 */
		for (cp2 = cbuf.ptr - 1; cp2 >= cbuf.bptr && *cp2 == ' '; cp2--)
			continue;
		cbuf.ptr = cp2 + 1;
		buf_putc(&cbuf, '\0');
		(void)buf_unputc(&cbuf);
		prefix(&cbuf, add_space);
		if (c != EOF)
			c = getc(fi);
	}
	buf_end(&cbuf);
	buf_end(&lbuf);
}

/*
 * Take a line devoid of tabs and other garbage and determine its
 * blank prefix.  If the indent changes, call for a linebreak.
 * If the input line is blank, echo the blank line on the output.
 * Finally, if the line minus the prefix is a mail header, try to keep
 * it on a line by itself.
 */
static void
prefix(const struct buffer *buf, int add_space)
{
	const char *cp;
	const char **hp;
	size_t np;
	int h;

	if (buf->ptr == buf->bptr) {
		oflush();
		(void)putchar('\n');
		return;
	}
	for (cp = buf->bptr; *cp == ' '; cp++)
		continue;
	np = cp - buf->bptr;

	/*
	 * The following horrible expression attempts to avoid linebreaks
	 * when the indent changes due to a paragraph.
	 */
	if (np != pfx && (np > pfx || abs((int)(pfx - np)) > 8))
		oflush();
	if ((h = ishead(cp)) != 0) {
		oflush();
		mark = lineno;
	}
	if (lineno - mark < 3 && lineno - mark > 0)
		for (hp = &headnames[0]; *hp != NULL; hp++)
			if (ispref(*hp, cp)) {
				h = 1;
				oflush();
				break;
			}
	if (!h && (h = (*cp == '.')))
		oflush();
	pfx = np;
	if (h) {
		pack(cp, (size_t)(buf->ptr - cp));
		oflush();
	} else
		split(cp, add_space);
	lineno++;
}

/*
 * Split up the passed line into output "words" which are
 * maximal strings of non-blanks with the blank separation
 * attached at the end.  Pass these words along to the output
 * line packer.
 */
static void
split(const char line[], int add_space)
{
	const char *cp;
	struct buffer word;
	size_t wlen;

	buf_init(&word);
	cp = line;
	while (*cp) {
		buf_reset(&word);
		wlen = 0;

		/*
		 * Collect a 'word,' allowing it to contain escaped white
		 * space. 
		 */
		while (*cp && *cp != ' ') {
			if (*cp == '\\' && isspace((unsigned char)cp[1]))
				buf_putc(&word, *cp++);
			buf_putc(&word, *cp++);
			wlen++;
		}

		/*
		 * Guarantee a space at end of line. Two spaces after end of
		 * sentence punctuation. 
		 */
		if (*cp == '\0' && add_space) {
			buf_putc(&word, ' ');
			if (strchr(".:!", cp[-1]))
				buf_putc(&word, ' ');
		}
		while (*cp == ' ')
			buf_putc(&word, *cp++);

		buf_putc(&word, '\0');
		(void)buf_unputc(&word);

		pack(word.bptr, wlen);
	}
	buf_end(&word);
}

/*
 * Output section.
 * Build up line images from the words passed in.  Prefix
 * each line with correct number of blanks.
 *
 * At the bottom of this whole mess, leading tabs are reinserted.
 */

/*
 * Pack a word onto the output line.  If this is the beginning of
 * the line, push on the appropriately-sized string of blanks first.
 * If the word won't fit on the current line, flush and begin a new
 * line.  If the word is too long to fit all by itself on a line,
 * just give it its own and hope for the best.
 *
 * LIZ@UOM 6/18/85 -- If the new word will fit in at less than the
 *	goal length, take it.  If not, then check to see if the line
 *	will be over the max length; if so put the word on the next
 *	line.  If not, check to see if the line will be closer to the
 *	goal length with or without the word and take it or put it on
 *	the next line accordingly.
 */

static void
pack(const char *word, size_t wlen)
{
	const char *cp;
	size_t s, t;

	if (outbuf.bptr == outbuf.ptr)
		leadin();
	/*
	 * LIZ@UOM 6/18/85 -- change condition to check goal_length; s is the
	 * length of the line before the word is added; t is now the length
	 * of the line after the word is added
	 */
	s = outbuf.ptr - outbuf.bptr;
	t = wlen + s;
	if ((t <= goal_length) || ((t <= max_length) &&
	    (s <= goal_length) && (t - goal_length <= goal_length - s))) {
		/*
		 * In like flint! 
		 */
		for (cp = word; *cp;)
			buf_putc(&outbuf, *cp++);
		return;
	}
	if (s > pfx) {
		oflush();
		leadin();
	}
	for (cp = word; *cp;)
		buf_putc(&outbuf, *cp++);
}

/*
 * If there is anything on the current output line, send it on
 * its way.  Reset outbuf.
 */
static void
oflush(void)
{
	if (outbuf.bptr == outbuf.ptr)
		return;
	buf_putc(&outbuf, '\0');
	(void)buf_unputc(&outbuf);
	tabulate(&outbuf);
	buf_reset(&outbuf);
}

/*
 * Take the passed line buffer, insert leading tabs where possible, and
 * output on standard output (finally).
 */
static void
tabulate(struct buffer *buf)
{
	char *cp;
	size_t b, t;

	/*
	 * Toss trailing blanks in the output line.
	 */
	for (cp = buf->ptr - 1; cp >= buf->bptr && *cp == ' '; cp--)
		continue;
	*++cp = '\0';
	
	/*
	 * Count the leading blank space and tabulate.
	 */
	for (cp = buf->bptr; *cp == ' '; cp++)
		continue;
	b = cp - buf->bptr;
	t = b / 8;
	b = b % 8;
	if (t > 0)
		do
			(void)putchar('\t');
		while (--t);
	if (b > 0)
		do
			(void)putchar(' ');
		while (--b);
	while (*cp)
		(void)putchar(*cp++);
	(void)putchar('\n');
}

/*
 * Initialize the output line with the appropriate number of
 * leading blanks.
 */
static void
leadin(void)
{
	size_t b;

	buf_reset(&outbuf);

	for (b = 0; b < pfx; b++)
		buf_putc(&outbuf, ' ');
}

/*
 * Is s1 a prefix of s2??
 */
static int
ispref(const char *s1, const char *s2)
{

	while (*s1++ == *s2)
		continue;
	return *s1 == '\0';
}
