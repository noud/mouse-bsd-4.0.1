/*	$NetBSD: cards.c,v 1.8 2005/07/02 08:32:32 jmc Exp $	*/

/*-
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
static char sccsid[] = "@(#)cards.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: cards.c,v 1.8 2005/07/02 08:32:32 jmc Exp $");
#endif
#endif /* not lint */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "deck.h"
#include "cribbage.h"


/*
 * Initialize a deck of cards to contain one of each type.
 */
void
makedeck(CARD d[])
{
	int i, j, k;

	i = time(NULL);
	i = ((i & 0xff) << 8) | ((i >> 8) & 0xff) | 1;
	srand(i);
	k = 0;
	for (i = 0; i < RANKS; i++)
		for (j = 0; j < SUITS; j++) {
			d[k].suit = j;
			d[k++].rank = i;
		}
}

/*
 * Given a deck of cards, shuffle it -- i.e. randomize it
 * see Knuth, vol. 2, page 125.
 */
void
shuffle(CARD d[])
{
	int j, k;
	CARD c;

	for (j = CARDS; j > 0; --j) {
		k = (rand() >> 4) % j;		/* random 0 <= k < j */
		c = d[j - 1];			/* exchange (j - 1) and k */
		d[j - 1] = d[k];
		d[k] = c;
	}
}

/*
 * return true if the two cards are equal...
 */
int
eq(CARD a, CARD b)
{
	return ((a.rank == b.rank) && (a.suit == b.suit));
}

/*
 * is_one returns TRUE if a is in the set of cards b
 */
int
is_one(CARD a, const CARD b[], int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (eq(a, b[i]))
			return (TRUE);
	return (FALSE);
}

/*
 * remove the card a from the deck d of n cards
 */
void
cremove(CARD a, CARD d[], int n)
{
	int i, j;

	for (i = j = 0; i < n; i++)
		if (!eq(a, d[i]))
			d[j++] = d[i];
	if (j < n)
		d[j].suit = d[j].rank = EMPTY;
}

/*
 * sorthand:
 *	Sort a hand of n cards
 */
void
sorthand(CARD h[], int n)
{
	CARD *cp, *endp;
	CARD c;

	for (endp = &h[n]; h < endp - 1; h++)
		for (cp = h + 1; cp < endp; cp++)
			if ((cp->rank < h->rank) ||
			    (cp->rank == h->rank && cp->suit < h->suit)) {
				c = *h;
				*h = *cp;
				*cp = c;
			}
}
