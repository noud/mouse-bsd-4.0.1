/*	$NetBSD: intr.h,v 1.15 2005/12/11 12:17:19 christos Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _HP300_INTR_H_
#define	_HP300_INTR_H_

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/psl.h>

/*
 * Interrupt "levels".  These are a more abstract representation
 * of interrupt levels, and do not have the same meaning as m68k
 * CPU interrupt levels.  They serve two purposes:
 *
 *	- properly order ISRs in the list for that CPU ipl
 *	- compute CPU PSL values for the spl*() calls.
 */
#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_BIO		1	/* disable block I/O interrupts */
#define	IPL_NET		2	/* disable network interrupts */
#define	IPL_TTY		3	/* disable terminal interrupts */
#define	IPL_TTYNOBUF	4	/* IPL_TTY + higher ISR priority */
#define	IPL_CLOCK	5	/* disable clock interrupts */
#define	IPL_HIGH	6	/* disable all interrupts */

/* Copied from alpha/include/intr.h */
#define	IPL_SOFTSERIAL	0	/* serial software interrupts */
#define	IPL_SOFTNET	1	/* network software interrupts */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts */
#define	IPL_SOFT	3	/* other software interrupts */
#define	IPL_NSOFT	4

#define	IPL_SOFTNAMES {							\
	"serial",							\
	"net",								\
	"clock",							\
	"misc",								\
}

/*
 * Convert PSL values to CPU IPLs and vice-versa.
 */
#define	PSLTOIPL(x)	(((x) >> 8) & 0xf)
#define	IPLTOPSL(x)	((((x) & 0xf) << 8) | PSL_S)

#ifdef _KERNEL
/*
 * This array contains the appropriate PSL_S|PSL_IPL? values
 * to raise interrupt priority to the requested level.
 */
extern unsigned short hp300_ipls[];

#define	HP300_IPL_SOFT		0
#define	HP300_IPL_BIO		1
#define	HP300_IPL_NET		2
#define	HP300_IPL_TTY		3
#define	HP300_IPL_VM		4
#define	HP300_IPL_CLOCK		5
#define	HP300_IPL_HIGH		6
#define	HP300_NIPLS		7

/* These spl calls are _not_ to be used by machine-independent code. */
#define	splhil()	splraise1()
#define	splkbd()	splhil()

/* These spl calls are used by machine-independent code. */
/* spl0 requires checking for software interrupts */
#define	spllowersoftclock() spl1()
#define	splsoft()	splraise1()
#define	splsoftclock()	splsoft()
#define	splsoftnet()	splsoft()
#define splsoftserial	splsoft()
#define	splbio()	_splraise(hp300_ipls[HP300_IPL_BIO])
#define	splnet()	_splraise(hp300_ipls[HP300_IPL_NET])
#define	spltty()	_splraise(hp300_ipls[HP300_IPL_TTY])
#define	splserial()	_splraise(hp300_ipls[HP300_IPL_TTY])
#define	splvm()		_splraise(hp300_ipls[HP300_IPL_VM])
#define	splclock()	spl6()
#define	splstatclock()	splclock()
#define	splhigh()	spl7()
#define	splsched()	spl7()
#define	spllock()	spl7()

/* watch out for side effects */
#define	splx(s)		((s) & PSL_IPL ? _spl((s)) : spl0())

struct hp300_intrhand {
	LIST_ENTRY(hp300_intrhand) ih_q;
	int (*ih_fn)(void *);
	void *ih_arg;
	int ih_ipl;
	int ih_priority;
};

struct hp300_intr {
	LIST_HEAD(, hp300_intrhand) hi_q;
	struct evcnt hi_evcnt;
};

/*
 * Software Interrupts.
 */

struct hp300_soft_intrhand {
	LIST_ENTRY(hp300_soft_intrhand) sih_q;
	struct hp300_soft_intr *sih_intrhead;
	void (*sih_fn)(void *);
	void *sih_arg;
	volatile int sih_pending;
};

struct hp300_soft_intr {
	LIST_HEAD(, hp300_soft_intrhand) hsi_q;
	struct evcnt hsi_evcnt;
	uint8_t hsi_ipl;
};

void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_init(void);
void	softintr_dispatch(void);

extern volatile uint8_t ssir;
#define setsoft(x)	ssir |= (1<<(x))

#define softintr_schedule(arg)				\
do {							\
	struct hp300_soft_intrhand *__sih = (arg);	\
	__sih->sih_pending = 1;				\
	setsoft(__sih->sih_intrhead->hsi_ipl);		\
} while (0)

/* XXX For legacy software interrupts */
extern struct hp300_soft_intrhand *softnet_intrhand;
#define setsoftnet()	softintr_schedule(softnet_intrhand)

/* locore.s */
int	spl0(void);

/* intr.c */
void	intr_init(void);
void	*intr_establish(int (*)(void *), void *, int, int);
void	intr_disestablish(void *);
void	intr_dispatch(int);
void	intr_printlevels(void);
void	netintr(void);

#endif /* _KERNEL */

#endif /* _HP300_INTR_H_ */
