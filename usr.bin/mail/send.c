/*	$NetBSD: send.c,v 1.28 2006/11/28 18:45:32 christos Exp $	*/

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
#if 0
static char sccsid[] = "@(#)send.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: send.c,v 1.28 2006/11/28 18:45:32 christos Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif


/*
 * Mail -- a mail program
 *
 * Mail to others.
 */

/*
 * compare twwo name structures.  Support for get_smopts.
 */
static int
namecmp(struct name *np1, struct name *np2)
{
	for (/*EMPTY*/; np1 && np2; np1 = np1->n_flink, np2 = np2->n_flink) {
		if (strcmp(np1->n_name, np2->n_name) != 0)
			return 1;
	}
	return np1 || np2;
}

/*
 * Get the sendmail options from the smopts list.
 */
static struct name *
get_smopts(struct name *to)
{
	struct smopts_s *sp;
	struct name *smargs, *np;
	smargs = NULL;
	for (np = to; np; np = np->n_flink) {
		if ((sp = findsmopts(np->n_name, 0)) == NULL)
			continue;
		if (smargs == NULL)
			smargs = sp->s_smopts;
		else if (namecmp(smargs, sp->s_smopts) != 0)
			return NULL;
	}
	if (smargs &&
	    smargs->n_flink == NULL &&
	    (smargs->n_name == NULL || smargs->n_name[0] == '\0'))
		return NULL;
	return smargs;
}

/*
 * Output a reasonable looking status field.
 */
static void
statusput(struct message *mp, FILE *obuf, const char *prefix)
{
	char statout[3];
	char *cp = statout;

	if (mp->m_flag & MREAD)
		*cp++ = 'R';
	if ((mp->m_flag & MNEW) == 0)
		*cp++ = 'O';
	*cp = 0;
	if (statout[0])
		(void)fprintf(obuf, "%sStatus: %s\n",
			prefix == NULL ? "" : prefix, statout);
}

/*
 * Send message described by the passed pointer to the
 * passed output buffer.  Return -1 on error.
 * Adjust the status: field if need be.
 * If doign is given, suppress ignored header fields.
 * prefix is a string to prepend to each output line.
 */
PUBLIC int
sendmessage(struct message *mp, FILE *obuf, struct ignoretab *doign,
    const char *prefix, struct mime_info *mip)
{
	off_t len;
	FILE *ibuf;
	char line[LINESIZE];
	int isheadflag, infld, ignoring, dostat, firstline;
	char *cp, *cp2;
	int c;	/* XXX - this variable is horribly abused */
	size_t length;
	size_t prefixlen;

	c = 0;
	ignoring = 0;
	prefixlen = 0;

	/*
	 * Compute the prefix string, without trailing whitespace
	 */
	if (prefix != NULL) {
		const char *dp, *dp2 = NULL;
		for (dp = prefix; *dp; dp++)
			if (*dp != ' ' && *dp != '\t')
				dp2 = dp;
		prefixlen = dp2 == 0 ? 0 : dp2 - prefix + 1;
	}
	ibuf = setinput(mp);
	len = mp->m_size;
	isheadflag = 1;
	dostat = doign == 0 || !isign("status", doign);
	infld = 0;
	firstline = 1;
	/*
	 * Process headers first
	 */
#ifdef MIME_SUPPORT
	if (mip)
		obuf = mime_decode_header(mip);
#endif
	while (len > 0 && isheadflag) {
		if (fgets(line, LINESIZE, ibuf) == NULL)
			break;
		len -= length = strlen(line);
		if (firstline) {
			/* 
			 * First line is the From line, so no headers
			 * there to worry about
			 */
			firstline = 0;
			ignoring = doign == ignoreall;
#ifdef MIME_SUPPORT
			/* XXX - ignore multipart boundary lines! */
			if (line[0] == '-' && line[1] == '-')
				ignoring = 1;
#endif
		} else if (line[0] == '\n') {
			/*
			 * If line is blank, we've reached end of
			 * headers, so force out status: field
			 * and note that we are no longer in header
			 * fields
			 */
			if (dostat) {
				statusput(mp, obuf, prefix);
				dostat = 0;
			}
			isheadflag = 0;
			ignoring = doign == ignoreall;
		} else if (infld && (line[0] == ' ' || line[0] == '\t')) {
			/*
			 * If this line is a continuation (via space or tab)
			 * of a previous header field, just echo it
			 * (unless the field should be ignored).
			 * In other words, nothing to do.
			 */
		} else {
			/*
			 * Pick up the header field if we have one.
			 */
			for (cp = line; (c = *cp++) && c != ':' && !isspace(c); /*EMPTY*/)
				continue;
			cp2 = --cp;
			while (isspace((unsigned char)*cp++))
				continue;
			if (cp[-1] != ':') {
				/*
				 * Not a header line, force out status:
				 * This happens in uucp style mail where
				 * there are no headers at all.
				 */
				if (dostat) {
					statusput(mp, obuf, prefix);
					dostat = 0;
				}
				if (doign != ignoreall)
					/* add blank line */
					(void)putc('\n', obuf);
				isheadflag = 0;
				ignoring = 0;
			} else {
				/*
				 * If it is an ignored field and
				 * we care about such things, skip it.
				 */
				*cp2 = 0;	/* temporarily null terminate */
				if (doign && isign(line, doign))
					ignoring = 1;
				else if ((line[0] == 's' || line[0] == 'S') &&
					 strcasecmp(line, "status") == 0) {
					/*
					 * If the field is "status," go compute
					 * and print the real Status: field
					 */
					if (dostat) {
						statusput(mp, obuf, prefix);
						dostat = 0;
					}
					ignoring = 1;
				} else {
					ignoring = 0;
					*cp2 = c;	/* restore */
				}
				infld = 1;
			}
		}
		if (!ignoring) {
			/*
			 * Strip trailing whitespace from prefix
			 * if line is blank.
			 */
			if (prefix != NULL) {
				if (length > 1)
					(void)fputs(prefix, obuf);
				else
					(void)fwrite(prefix, sizeof *prefix,
							prefixlen, obuf);
			}
			(void)fwrite(line, sizeof *line, length, obuf);
			if (ferror(obuf))
				return -1;
		}
	}
	/*
	 * Copy out message body
	 */
#ifdef MIME_SUPPORT
	if (mip) {
		obuf = mime_decode_body(mip);
		if (obuf == NULL) /* XXX - early out */
			return 0;
	}
#endif
	c = 0;	/* This is needed if len == 0.  It's used differently above. */
	if (doign == ignoreall)
		len--;		/* skip final blank line */

	if (prefix != NULL)
		while (len > 0) {
			if (fgets(line, LINESIZE, ibuf) == NULL) {
				c = 0;
				break;
			}
			len -= c = strlen(line);
			/*
			 * Strip trailing whitespace from prefix
			 * if line is blank.
			 */
			if (c > 1)
				(void)fputs(prefix, obuf);
			else
				(void)fwrite(prefix, sizeof *prefix,
						prefixlen, obuf);
			(void)fwrite(line, sizeof *line, (size_t)c, obuf);
			if (ferror(obuf))
				return -1;
		}
	else
		while (len > 0) {
			c = (int)(len < LINESIZE ? len : LINESIZE);
			if ((c = fread(line, sizeof *line, (size_t)c, ibuf)) == 0)
				break;
			len -= c;
			if (fwrite(line, sizeof *line, (size_t)c, obuf) != (size_t)c)
				return -1;
		}
	if (doign == ignoreall && c > 0 && line[c - 1] != '\n')
		/* no final blank line */
		if ((c = getc(ibuf)) != EOF && putc(c, obuf) == EOF)
				return -1;
	return 0;
}

/*
 * Fix the header by glopping all of the expanded names from
 * the distribution list into the appropriate fields.
 */
static void
fixhead(struct header *hp, struct name *tolist)
{
	struct name *np;

	hp->h_to = NULL;
	hp->h_cc = NULL;
	hp->h_bcc = NULL;
	for (np = tolist; np != NULL; np = np->n_flink) {
		if (np->n_type & GDEL)
			continue;	/* Don't copy deleted addresses to the header */
		if ((np->n_type & GMASK) == GTO)
			hp->h_to =
				cat(hp->h_to, nalloc(np->n_name, np->n_type));
		else if ((np->n_type & GMASK) == GCC)
			hp->h_cc =
				cat(hp->h_cc, nalloc(np->n_name, np->n_type));
		else if ((np->n_type & GMASK) == GBCC)
			hp->h_bcc =
				cat(hp->h_bcc, nalloc(np->n_name, np->n_type));
	}
}

/*
 * Format the given header line to not exceed 72 characters.
 */
static void
fmt(const char *str, struct name *np, FILE *fo, int comma)
{
	int col, len;

	comma = comma ? 1 : 0;
	col = strlen(str);
	if (col)
		(void)fputs(str, fo);
	for (; np != NULL; np = np->n_flink) {
		if (np->n_flink == NULL)
			comma = 0;
		len = strlen(np->n_name);
		col++;		/* for the space */
		if (col + len + comma > 72 && col > 4) {
			(void)fputs("\n    ", fo);
			col = 4;
		} else
			(void)putc(' ', fo);
		(void)fputs(np->n_name, fo);
		if (comma)
			(void)putc(',', fo);
		col += len + comma;
	}
	(void)putc('\n', fo);
}

/*
 * Dump the to, subject, cc header on the
 * passed file buffer.
 */
PUBLIC int
puthead(struct header *hp, FILE *fo, int w)
{
	int gotcha;

	gotcha = 0;
	if (hp->h_to != NULL && w & GTO)
		fmt("To:", hp->h_to, fo, w&GCOMMA), gotcha++;
	if (hp->h_subject != NULL && w & GSUBJECT)
		(void)fprintf(fo, "Subject: %s\n", hp->h_subject), gotcha++;
	if (hp->h_smopts != NULL && w & GSMOPTS)
		(void)fprintf(fo, "(sendmail options: %s)\n", detract(hp->h_smopts, GSMOPTS)), gotcha++;
	if (hp->h_cc != NULL && w & GCC)
		fmt("Cc:", hp->h_cc, fo, w&GCOMMA), gotcha++;
	if (hp->h_bcc != NULL && w & GBCC)
		fmt("Bcc:", hp->h_bcc, fo, w&GCOMMA), gotcha++;
	if (hp->h_in_reply_to != NULL && w & GMISC)
		(void)fprintf(fo, "In-Reply-To: %s\n", hp->h_in_reply_to), gotcha++;
	if (hp->h_references != NULL && w & GMISC)
		fmt("References:", hp->h_references, fo, w&GCOMMA), gotcha++;
#ifdef MIME_SUPPORT
	if (w & GMIME && (hp->h_attach || value(ENAME_MIME_ENCODE_MSG)))
		mime_putheader(fo, hp), gotcha++;
#endif
	if (gotcha && w & GNL)
		(void)putc('\n', fo);
	return 0;
}

/*
 * Prepend a header in front of the collected stuff
 * and return the new file.
 */
static FILE *
infix(struct header *hp, FILE *fi)
{
	FILE *nfo, *nfi;
	int c, fd;
	char tempname[PATHSIZE];

	(void)snprintf(tempname, sizeof(tempname),
	    "%s/mail.RsXXXXXXXXXX", tmpdir);
	if ((fd = mkstemp(tempname)) == -1 ||
	    (nfo = Fdopen(fd, "w")) == NULL) {
		if (fd != -1)
			(void)close(fd);
		warn("%s", tempname);
		return fi;
	}
	if ((nfi = Fopen(tempname, "r")) == NULL) {
		warn("%s", tempname);
		(void)Fclose(nfo);
		(void)rm(tempname);
		return fi;
	}
	(void)rm(tempname);
#ifdef MIME_SUPPORT
	(void)puthead(hp, nfo, GTO|GSUBJECT|GCC|GBCC|GMISC|GMIME|GNL|GCOMMA);
#else
	(void)puthead(hp, nfo, GTO|GSUBJECT|GCC|GBCC|GMISC|GNL|GCOMMA);
#endif

	c = getc(fi);
	while (c != EOF) {
		(void)putc(c, nfo);
		c = getc(fi);
	}
	if (ferror(fi)) {
		warn("read");
		rewind(fi);
		return fi;
	}
	(void)fflush(nfo);
	if (ferror(nfo)) {
		warn("%s", tempname);
		(void)Fclose(nfo);
		(void)Fclose(nfi);
		rewind(fi);
		return fi;
	}
	(void)Fclose(nfo);
	(void)Fclose(fi);
	rewind(nfi);
	return nfi;
}

/*
 * Save the outgoing mail on the passed file.
 */
/*ARGSUSED*/
static int
savemail(const char name[], FILE *fi)
{
	FILE *fo;
	char buf[LINESIZE];
	int i;
	time_t now;
	mode_t m;

	m = umask(077);
	fo = Fopen(name, "a");
	(void)umask(m);
	if (fo == NULL) {
		warn("%s", name);
		return -1;
	}
	(void)time(&now);
	(void)fprintf(fo, "From %s %s", myname, ctime(&now));
	while ((i = fread(buf, 1, sizeof buf, fi)) > 0)
		(void)fwrite(buf, 1, (size_t)i, fo);
	(void)putc('\n', fo);
	(void)fflush(fo);
	if (ferror(fo))
		warn("%s", name);
	(void)Fclose(fo);
	rewind(fi);
	return 0;
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */
PUBLIC void
mail1(struct header *hp, int printheaders)
{
	const char *cp;
	int pid;
	const char **namelist;
	struct name *to;
	FILE *mtf;

	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */
	if ((mtf = collect(hp, printheaders)) == NULL)
		return;
	if (value(ENAME_INTERACTIVE) != NULL) {
		if (value(ENAME_ASKCC) != NULL || value(ENAME_ASKBCC) != NULL) {
			if (value(ENAME_ASKCC) != NULL)
				(void)grabh(hp, GCC);
			if (value(ENAME_ASKBCC) != NULL)
				(void)grabh(hp, GBCC);
		} else {
			(void)printf("EOT\n");
			(void)fflush(stdout);
		}
	}
	if (fsize(mtf) == 0) {
		if (value(ENAME_DONTSENDEMPTY) != NULL)
			goto out;
		if (hp->h_subject == NULL)
			(void)printf("No message, no subject; hope that's ok\n");
		else
			(void)printf("Null message body; hope that's ok\n");
	}
	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */
	senderr = 0;
	to = usermap(cat(hp->h_bcc, cat(hp->h_to, hp->h_cc)));
	if (to == NULL) {
		(void)printf("No recipients specified\n");
		senderr++;
	}
#ifdef MIME_SUPPORT
	/*
	 * If there are attachments, repackage the mail as a
	 * multi-part MIME message.
	 */
	if (hp->h_attach || value(ENAME_MIME_ENCODE_MSG))
		mtf = mime_encode(mtf, hp);
#endif
	/*
	 * Look through the recipient list for names with /'s
	 * in them which we write to as files directly.
	 */
	to = outof(to, mtf, hp);
	if (senderr)
		savedeadletter(mtf);
	to = elide(to);
	if (count(to) == 0)
		goto out;
	fixhead(hp, to);
	if ((mtf = infix(hp, mtf)) == NULL) {
		(void)fprintf(stderr, ". . . message lost, sorry.\n");
		return;
	}
	if (hp->h_smopts == NULL) {
		hp->h_smopts = get_smopts(to);
		if (hp->h_smopts != NULL &&
		    hp->h_smopts->n_name[0] != '\0' &&
		    value(ENAME_SMOPTS_VERIFY) != NULL)
			if (grabh(hp, GSMOPTS)) {
				(void)printf("mail aborted!\n");
				savedeadletter(mtf);
				goto out;
			}
	}
	namelist = unpack(cat(hp->h_smopts, to));
	if (debug) {
		const char **t;

		(void)printf("Sendmail arguments:");
		for (t = namelist; *t != NULL; t++)
			(void)printf(" \"%s\"", *t);
		(void)printf("\n");
		goto out;
	}
	if ((cp = value(ENAME_RECORD)) != NULL)
		(void)savemail(expand(cp), mtf);
	/*
	 * Fork, set up the temporary mail file as standard
	 * input for "mail", and exec with the user list we generated
	 * far above.
	 */
	pid = fork();
	if (pid == -1) {
		warn("fork");
		savedeadletter(mtf);
		goto out;
	}
	if (pid == 0) {
		sigset_t nset;
		(void)sigemptyset(&nset);
		(void)sigaddset(&nset, SIGHUP);
		(void)sigaddset(&nset, SIGINT);
		(void)sigaddset(&nset, SIGQUIT);
		(void)sigaddset(&nset, SIGTSTP);
		(void)sigaddset(&nset, SIGTTIN);
		(void)sigaddset(&nset, SIGTTOU);
		prepare_child(&nset, fileno(mtf), -1);
		if ((cp = value(ENAME_SENDMAIL)) != NULL)
			cp = expand(cp);
		else
			cp = _PATH_SENDMAIL;
		(void)execv(cp, (char *const *)__UNCONST(namelist));
		warn("%s", cp);
		_exit(1);
	}
	if (value(ENAME_VERBOSE) != NULL)
		(void)wait_child(pid);
	else
		free_child(pid);
out:
	(void)Fclose(mtf);
}

/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */
PUBLIC int
mail(struct name *to, struct name *cc, struct name *bcc,
     struct name *smopts, char *subject, struct attachment *attach)
{
	struct header head;

	/* ensure that all header fields are initially NULL */
	(void)memset(&head, 0, sizeof(head));

	head.h_to = to;
	head.h_subject = subject;
	head.h_cc = cc;
	head.h_bcc = bcc;
	head.h_smopts = smopts;
#ifdef MIME_SUPPORT
	head.h_attach = attach;
#endif
	mail1(&head, 0);
	return 0;
}

/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail1 routine below.
 */
PUBLIC int
sendmail(void *v)
{
	char *str = v;
	struct header head;

	/* ensure that all header fields are initially NULL */
	(void)memset(&head, 0, sizeof(head));

	head.h_to = extract(str, GTO);

	mail1(&head, 0);
	return 0;
}
