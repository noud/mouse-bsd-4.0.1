/*	$NetBSD: uvm_pdaemon.c,v 1.80 2006/11/01 10:18:27 yamt Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by Charles D. Cranor,
 *      Washington University, the University of California, Berkeley and
 *      its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)vm_pageout.c        8.5 (Berkeley) 2/14/94
 * from: Id: uvm_pdaemon.c,v 1.1.2.32 1998/02/06 05:26:30 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * uvm_pdaemon.c: the page daemon
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pdaemon.c,v 1.80 2006/11/01 10:18:27 yamt Exp $");

#include "opt_uvmhist.h"
#include "opt_readahead.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/buf.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>

/*
 * UVMPD_NUMDIRTYREACTS is how many dirty pages the pagedaemon will reactivate
 * in a pass thru the inactive list when swap is full.  the value should be
 * "small"... if it's too large we'll cycle the active pages thru the inactive
 * queue too quickly to for them to be referenced and avoid being freed.
 */

#define UVMPD_NUMDIRTYREACTS 16


/*
 * local prototypes
 */

static void	uvmpd_scan(void);
static void	uvmpd_scan_queue(void);
static void	uvmpd_tune(void);

/*
 * XXX hack to avoid hangs when large processes fork.
 */
int uvm_extrapages;

/*
 * uvm_wait: wait (sleep) for the page daemon to free some pages
 *
 * => should be called with all locks released
 * => should _not_ be called by the page daemon (to avoid deadlock)
 */

void
uvm_wait(const char *wmsg)
{
	int timo = 0;
	int s = splbio();

	/*
	 * check for page daemon going to sleep (waiting for itself)
	 */

	if (curproc == uvm.pagedaemon_proc && uvmexp.paging == 0) {
		/*
		 * now we have a problem: the pagedaemon wants to go to
		 * sleep until it frees more memory.   but how can it
		 * free more memory if it is asleep?  that is a deadlock.
		 * we have two options:
		 *  [1] panic now
		 *  [2] put a timeout on the sleep, thus causing the
		 *      pagedaemon to only pause (rather than sleep forever)
		 *
		 * note that option [2] will only help us if we get lucky
		 * and some other process on the system breaks the deadlock
		 * by exiting or freeing memory (thus allowing the pagedaemon
		 * to continue).  for now we panic if DEBUG is defined,
		 * otherwise we hope for the best with option [2] (better
		 * yet, this should never happen in the first place!).
		 */

		printf("pagedaemon: deadlock detected!\n");
		timo = hz >> 3;		/* set timeout */
#if defined(DEBUG)
		/* DEBUG: panic so we can debug it */
		panic("pagedaemon deadlock");
#endif
	}

	simple_lock(&uvm.pagedaemon_lock);
	wakeup(&uvm.pagedaemon);		/* wake the daemon! */
	UVM_UNLOCK_AND_WAIT(&uvmexp.free, &uvm.pagedaemon_lock, FALSE, wmsg,
	    timo);

	splx(s);
}

/*
 * uvm_kick_pdaemon: perform checks to determine if we need to
 * give the pagedaemon a nudge, and do so if necessary.
 */

void
uvm_kick_pdaemon(void)
{

	if (uvmexp.free + uvmexp.paging < uvmexp.freemin ||
	    (uvmexp.free + uvmexp.paging < uvmexp.freetarg &&
	     uvmpdpol_needsscan_p())) {
		wakeup(&uvm.pagedaemon);
	}
}

/*
 * uvmpd_tune: tune paging parameters
 *
 * => called when ever memory is added (or removed?) to the system
 * => caller must call with page queues locked
 */

static void
uvmpd_tune(void)
{
	UVMHIST_FUNC("uvmpd_tune"); UVMHIST_CALLED(pdhist);

	uvmexp.freemin = uvmexp.npages / 20;

	/* between 16k and 256k */
	/* XXX:  what are these values good for? */
	uvmexp.freemin = MAX(uvmexp.freemin, (16*1024) >> PAGE_SHIFT);
	uvmexp.freemin = MIN(uvmexp.freemin, (256*1024) >> PAGE_SHIFT);

	/* Make sure there's always a user page free. */
	if (uvmexp.freemin < uvmexp.reserve_kernel + 1)
		uvmexp.freemin = uvmexp.reserve_kernel + 1;

	uvmexp.freetarg = (uvmexp.freemin * 4) / 3;
	if (uvmexp.freetarg <= uvmexp.freemin)
		uvmexp.freetarg = uvmexp.freemin + 1;

	uvmexp.freetarg += uvm_extrapages;
	uvm_extrapages = 0;

	uvmexp.wiredmax = uvmexp.npages / 3;
	UVMHIST_LOG(pdhist, "<- done, freemin=%d, freetarg=%d, wiredmax=%d",
	      uvmexp.freemin, uvmexp.freetarg, uvmexp.wiredmax, 0);
}

/*
 * uvm_pageout: the main loop for the pagedaemon
 */

void
uvm_pageout(void *arg)
{
	int bufcnt, npages = 0;
	int extrapages = 0;
	UVMHIST_FUNC("uvm_pageout"); UVMHIST_CALLED(pdhist);

	UVMHIST_LOG(pdhist,"<starting uvm pagedaemon>", 0, 0, 0, 0);

	/*
	 * ensure correct priority and set paging parameters...
	 */

	uvm.pagedaemon_proc = curproc;
	uvm_lock_pageq();
	npages = uvmexp.npages;
	uvmpd_tune();
	uvm_unlock_pageq();

	/*
	 * main loop
	 */

	for (;;) {
		simple_lock(&uvm.pagedaemon_lock);

		UVMHIST_LOG(pdhist,"  <<SLEEPING>>",0,0,0,0);
		UVM_UNLOCK_AND_WAIT(&uvm.pagedaemon,
		    &uvm.pagedaemon_lock, FALSE, "pgdaemon", 0);
		uvmexp.pdwoke++;
		UVMHIST_LOG(pdhist,"  <<WOKE UP>>",0,0,0,0);

		/*
		 * now lock page queues and recompute inactive count
		 */

		uvm_lock_pageq();
		if (npages != uvmexp.npages || extrapages != uvm_extrapages) {
			npages = uvmexp.npages;
			extrapages = uvm_extrapages;
			uvmpd_tune();
		}

		uvmpdpol_tune();

		/*
		 * Estimate a hint.  Note that bufmem are returned to
		 * system only when entire pool page is empty.
		 */
		bufcnt = uvmexp.freetarg - uvmexp.free;
		if (bufcnt < 0)
			bufcnt = 0;

		UVMHIST_LOG(pdhist,"  free/ftarg=%d/%d",
		    uvmexp.free, uvmexp.freetarg, 0,0);

		/*
		 * scan if needed
		 */

		if (uvmexp.free + uvmexp.paging < uvmexp.freetarg ||
		    uvmpdpol_needsscan_p()) {
			uvmpd_scan();
		}

		/*
		 * if there's any free memory to be had,
		 * wake up any waiters.
		 */

		if (uvmexp.free > uvmexp.reserve_kernel ||
		    uvmexp.paging == 0) {
			wakeup(&uvmexp.free);
		}

		/*
		 * scan done.  unlock page queues (the only lock we are holding)
		 */

		uvm_unlock_pageq();

		buf_drain(bufcnt << PAGE_SHIFT);

		/*
		 * drain pool resources now that we're not holding any locks
		 */

		pool_drain(0);

		/*
		 * free any cached u-areas we don't need
		 */
		uvm_uarea_drain(TRUE);

	}
	/*NOTREACHED*/
}


/*
 * uvm_aiodone_daemon:  main loop for the aiodone daemon.
 */

void
uvm_aiodone_daemon(void *arg)
{
	int s, free;
	struct buf *bp, *nbp;
	UVMHIST_FUNC("uvm_aiodoned"); UVMHIST_CALLED(pdhist);

	for (;;) {

		/*
		 * carefully attempt to go to sleep (without losing "wakeups"!).
		 * we need splbio because we want to make sure the aio_done list
		 * is totally empty before we go to sleep.
		 */

		s = splbio();
		simple_lock(&uvm.aiodoned_lock);
		if (TAILQ_FIRST(&uvm.aio_done) == NULL) {
			UVMHIST_LOG(pdhist,"  <<SLEEPING>>",0,0,0,0);
			UVM_UNLOCK_AND_WAIT(&uvm.aiodoned,
			    &uvm.aiodoned_lock, FALSE, "aiodoned", 0);
			UVMHIST_LOG(pdhist,"  <<WOKE UP>>",0,0,0,0);

			/* relock aiodoned_lock, still at splbio */
			simple_lock(&uvm.aiodoned_lock);
		}

		/*
		 * check for done aio structures
		 */

		bp = TAILQ_FIRST(&uvm.aio_done);
		if (bp) {
			TAILQ_INIT(&uvm.aio_done);
		}

		simple_unlock(&uvm.aiodoned_lock);
		splx(s);

		/*
		 * process each i/o that's done.
		 */

		free = uvmexp.free;
		while (bp != NULL) {
			nbp = TAILQ_NEXT(bp, b_freelist);
			(*bp->b_iodone)(bp);
			bp = nbp;
		}
		if (free <= uvmexp.reserve_kernel) {
			s = uvm_lock_fpageq();
			wakeup(&uvm.pagedaemon);
			uvm_unlock_fpageq(s);
		} else {
			simple_lock(&uvm.pagedaemon_lock);
			wakeup(&uvmexp.free);
			simple_unlock(&uvm.pagedaemon_lock);
		}
	}
}

/*
 * uvmpd_trylockowner: trylock the page's owner.
 *
 * => called with pageq locked.
 * => resolve orphaned O->A loaned page.
 * => return the locked simplelock on success.  otherwise, return NULL.
 */

struct simplelock *
uvmpd_trylockowner(struct vm_page *pg)
{
	struct uvm_object *uobj = pg->uobject;
	struct simplelock *slock;

	UVM_LOCK_ASSERT_PAGEQ();
	if (uobj != NULL) {
		slock = &uobj->vmobjlock;
	} else {
		struct vm_anon *anon = pg->uanon;

		KASSERT(anon != NULL);
		slock = &anon->an_lock;
	}

	if (!simple_lock_try(slock)) {
		return NULL;
	}

	if (uobj == NULL) {

		/*
		 * set PQ_ANON if it isn't set already.
		 */

		if ((pg->pqflags & PQ_ANON) == 0) {
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->pqflags |= PQ_ANON;
			/* anon now owns it */
		}
	}

	return slock;
}

#if defined(VMSWAP)
struct swapcluster {
	int swc_slot;
	int swc_nallocated;
	int swc_nused;
	struct vm_page *swc_pages[howmany(MAXPHYS, MIN_PAGE_SIZE)];
};

static void
swapcluster_init(struct swapcluster *swc)
{

	swc->swc_slot = 0;
}

static int
swapcluster_allocslots(struct swapcluster *swc)
{
	int slot;
	int npages;

	if (swc->swc_slot != 0) {
		return 0;
	}

	/* Even with strange MAXPHYS, the shift
	   implicitly rounds down to a page. */
	npages = MAXPHYS >> PAGE_SHIFT;
	slot = uvm_swap_alloc(&npages, TRUE);
	if (slot == 0) {
		return ENOMEM;
	}
	swc->swc_slot = slot;
	swc->swc_nallocated = npages;
	swc->swc_nused = 0;

	return 0;
}

static int
swapcluster_add(struct swapcluster *swc, struct vm_page *pg)
{
	int slot;
	struct uvm_object *uobj;

	KASSERT(swc->swc_slot != 0);
	KASSERT(swc->swc_nused < swc->swc_nallocated);
	KASSERT((pg->pqflags & PQ_SWAPBACKED) != 0);

	slot = swc->swc_slot + swc->swc_nused;
	uobj = pg->uobject;
	if (uobj == NULL) {
		LOCK_ASSERT(simple_lock_held(&pg->uanon->an_lock));
		pg->uanon->an_swslot = slot;
	} else {
		int result;

		LOCK_ASSERT(simple_lock_held(&uobj->vmobjlock));
		result = uao_set_swslot(uobj, pg->offset >> PAGE_SHIFT, slot);
		if (result == -1) {
			return ENOMEM;
		}
	}
	swc->swc_pages[swc->swc_nused] = pg;
	swc->swc_nused++;

	return 0;
}

static void
swapcluster_flush(struct swapcluster *swc, boolean_t now)
{
	int slot;
	int nused;
	int nallocated;
	int error;

	if (swc->swc_slot == 0) {
		return;
	}
	KASSERT(swc->swc_nused <= swc->swc_nallocated);

	slot = swc->swc_slot;
	nused = swc->swc_nused;
	nallocated = swc->swc_nallocated;

	/*
	 * if this is the final pageout we could have a few
	 * unused swap blocks.  if so, free them now.
	 */

	if (nused < nallocated) {
		if (!now) {
			return;
		}
		uvm_swap_free(slot + nused, nallocated - nused);
	}

	/*
	 * now start the pageout.
	 */

	uvmexp.pdpageouts++;
	error = uvm_swap_put(slot, swc->swc_pages, nused, 0);
	KASSERT(error == 0);

	/*
	 * zero swslot to indicate that we are
	 * no longer building a swap-backed cluster.
	 */

	swc->swc_slot = 0;
}

/*
 * uvmpd_dropswap: free any swap allocated to this page.
 *
 * => called with owner locked.
 * => return TRUE if a page had an associated slot.
 */

static boolean_t
uvmpd_dropswap(struct vm_page *pg)
{
	boolean_t result = FALSE;
	struct vm_anon *anon = pg->uanon;

	if ((pg->pqflags & PQ_ANON) && anon->an_swslot) {
		uvm_swap_free(anon->an_swslot, 1);
		anon->an_swslot = 0;
		pg->flags &= ~PG_CLEAN;
		result = TRUE;
	} else if (pg->pqflags & PQ_AOBJ) {
		int slot = uao_set_swslot(pg->uobject,
		    pg->offset >> PAGE_SHIFT, 0);
		if (slot) {
			uvm_swap_free(slot, 1);
			pg->flags &= ~PG_CLEAN;
			result = TRUE;
		}
	}

	return result;
}

/*
 * uvmpd_trydropswap: try to free any swap allocated to this page.
 *
 * => return TRUE if a slot is successfully freed.
 */

boolean_t
uvmpd_trydropswap(struct vm_page *pg)
{
	struct simplelock *slock;
	boolean_t result;

	if ((pg->flags & PG_BUSY) != 0) {
		return FALSE;
	}

	/*
	 * lock the page's owner.
	 */

	slock = uvmpd_trylockowner(pg);
	if (slock == NULL) {
		return FALSE;
	}

	/*
	 * skip this page if it's busy.
	 */

	if ((pg->flags & PG_BUSY) != 0) {
		simple_unlock(slock);
		return FALSE;
	}

	result = uvmpd_dropswap(pg);

	simple_unlock(slock);

	return result;
}

#endif /* defined(VMSWAP) */

/*
 * uvmpd_scan_queue: scan an replace candidate list for pages
 * to clean or free.
 *
 * => called with page queues locked
 * => we work on meeting our free target by converting inactive pages
 *    into free pages.
 * => we handle the building of swap-backed clusters
 */

static void
uvmpd_scan_queue(void)
{
	struct vm_page *p;
	struct uvm_object *uobj;
	struct vm_anon *anon;
#if defined(VMSWAP)
	struct swapcluster swc;
#endif /* defined(VMSWAP) */
	int dirtyreacts;
	struct simplelock *slock;
	UVMHIST_FUNC("uvmpd_scan_queue"); UVMHIST_CALLED(pdhist);

	/*
	 * swslot is non-zero if we are building a swap cluster.  we want
	 * to stay in the loop while we have a page to scan or we have
	 * a swap-cluster to build.
	 */

#if defined(VMSWAP)
	swapcluster_init(&swc);
#endif /* defined(VMSWAP) */

	dirtyreacts = 0;
	uvmpdpol_scaninit();

	while (/* CONSTCOND */ 1) {

		/*
		 * see if we've met the free target.
		 */

		if (uvmexp.free + uvmexp.paging >= uvmexp.freetarg << 2 ||
		    dirtyreacts == UVMPD_NUMDIRTYREACTS) {
			UVMHIST_LOG(pdhist,"  met free target: "
				    "exit loop", 0, 0, 0, 0);
			break;
		}

		p = uvmpdpol_selectvictim();
		if (p == NULL) {
			break;
		}
		KASSERT(uvmpdpol_pageisqueued_p(p));
		KASSERT(p->wire_count == 0);

		/*
		 * we are below target and have a new page to consider.
		 */

		anon = p->uanon;
		uobj = p->uobject;

		/*
		 * first we attempt to lock the object that this page
		 * belongs to.  if our attempt fails we skip on to
		 * the next page (no harm done).  it is important to
		 * "try" locking the object as we are locking in the
		 * wrong order (pageq -> object) and we don't want to
		 * deadlock.
		 *
		 * the only time we expect to see an ownerless page
		 * (i.e. a page with no uobject and !PQ_ANON) is if an
		 * anon has loaned a page from a uvm_object and the
		 * uvm_object has dropped the ownership.  in that
		 * case, the anon can "take over" the loaned page
		 * and make it its own.
		 */

		slock = uvmpd_trylockowner(p);
		if (slock == NULL) {
			continue;
		}
		if (p->flags & PG_BUSY) {
			simple_unlock(slock);
			uvmexp.pdbusy++;
			continue;
		}

		/* does the page belong to an object? */
		if (uobj != NULL) {
			uvmexp.pdobscan++;
		} else {
#if defined(VMSWAP)
			KASSERT(anon != NULL);
			uvmexp.pdanscan++;
#else /* defined(VMSWAP) */
			panic("%s: anon", __func__);
#endif /* defined(VMSWAP) */
		}


		/*
		 * we now have the object and the page queues locked.
		 * if the page is not swap-backed, call the object's
		 * pager to flush and free the page.
		 */

#if defined(READAHEAD_STATS)
		if ((p->pqflags & PQ_READAHEAD) != 0) {
			p->pqflags &= ~PQ_READAHEAD;
			uvm_ra_miss.ev_count++;
		}
#endif /* defined(READAHEAD_STATS) */

		if ((p->pqflags & PQ_SWAPBACKED) == 0) {
			uvm_unlock_pageq();
			(void) (uobj->pgops->pgo_put)(uobj, p->offset,
			    p->offset + PAGE_SIZE, PGO_CLEANIT|PGO_FREE);
			uvm_lock_pageq();
			continue;
		}

		/*
		 * the page is swap-backed.  remove all the permissions
		 * from the page so we can sync the modified info
		 * without any race conditions.  if the page is clean
		 * we can free it now and continue.
		 */

		pmap_page_protect(p, VM_PROT_NONE);
		if ((p->flags & PG_CLEAN) && pmap_clear_modify(p)) {
			p->flags &= ~(PG_CLEAN);
		}
		if (p->flags & PG_CLEAN) {
			int slot;
			int pageidx;

			pageidx = p->offset >> PAGE_SHIFT;
			uvm_pagefree(p);
			uvmexp.pdfreed++;

			/*
			 * for anons, we need to remove the page
			 * from the anon ourselves.  for aobjs,
			 * pagefree did that for us.
			 */

			if (anon) {
				KASSERT(anon->an_swslot != 0);
				anon->an_page = NULL;
				slot = anon->an_swslot;
			} else {
				slot = uao_find_swslot(uobj, pageidx);
			}
			simple_unlock(slock);

			if (slot > 0) {
				/* this page is now only in swap. */
				simple_lock(&uvm.swap_data_lock);
				KASSERT(uvmexp.swpgonly < uvmexp.swpginuse);
				uvmexp.swpgonly++;
				simple_unlock(&uvm.swap_data_lock);
			}
			continue;
		}

#if defined(VMSWAP)
		/*
		 * this page is dirty, skip it if we'll have met our
		 * free target when all the current pageouts complete.
		 */

		if (uvmexp.free + uvmexp.paging > uvmexp.freetarg << 2) {
			simple_unlock(slock);
			continue;
		}

		/*
		 * free any swap space allocated to the page since
		 * we'll have to write it again with its new data.
		 */

		uvmpd_dropswap(p);

		/*
		 * if all pages in swap are only in swap,
		 * the swap space is full and we can't page out
		 * any more swap-backed pages.  reactivate this page
		 * so that we eventually cycle all pages through
		 * the inactive queue.
		 */

		if (uvm_swapisfull()) {
			dirtyreacts++;
			uvm_pageactivate(p);
			simple_unlock(slock);
			continue;
		}

		/*
		 * start new swap pageout cluster (if necessary).
		 */

		if (swapcluster_allocslots(&swc)) {
			simple_unlock(slock);
			dirtyreacts++; /* XXX */
			continue;
		}

		/*
		 * at this point, we're definitely going reuse this
		 * page.  mark the page busy and delayed-free.
		 * we should remove the page from the page queues
		 * so we don't ever look at it again.
		 * adjust counters and such.
		 */

		p->flags |= PG_BUSY;
		UVM_PAGE_OWN(p, "scan_queue");

		p->flags |= PG_PAGEOUT;
		uvmexp.paging++;
		uvm_pagedequeue(p);

		uvmexp.pgswapout++;
		uvm_unlock_pageq();

		/*
		 * add the new page to the cluster.
		 */

		if (swapcluster_add(&swc, p)) {
			p->flags &= ~(PG_BUSY|PG_PAGEOUT);
			UVM_PAGE_OWN(p, NULL);
			uvm_lock_pageq();
			uvmexp.paging--;
			dirtyreacts++;
			uvm_pageactivate(p);
			simple_unlock(slock);
			continue;
		}
		simple_unlock(slock);

		swapcluster_flush(&swc, FALSE);
		uvm_lock_pageq();

		/*
		 * the pageout is in progress.  bump counters and set up
		 * for the next loop.
		 */

		uvmexp.pdpending++;

#else /* defined(VMSWAP) */
		uvm_pageactivate(p);
		simple_unlock(slock);
#endif /* defined(VMSWAP) */
	}

#if defined(VMSWAP)
	uvm_unlock_pageq();
	swapcluster_flush(&swc, TRUE);
	uvm_lock_pageq();
#endif /* defined(VMSWAP) */
}

/*
 * uvmpd_scan: scan the page queues and attempt to meet our targets.
 *
 * => called with pageq's locked
 */

static void
uvmpd_scan(void)
{
	int swap_shortage, pages_freed;
	UVMHIST_FUNC("uvmpd_scan"); UVMHIST_CALLED(pdhist);

	uvmexp.pdrevs++;

#ifndef __SWAP_BROKEN

	/*
	 * swap out some processes if we are below our free target.
	 * we need to unlock the page queues for this.
	 */

	if (uvmexp.free < uvmexp.freetarg && uvmexp.nswapdev != 0) {
		uvmexp.pdswout++;
		UVMHIST_LOG(pdhist,"  free %d < target %d: swapout",
		    uvmexp.free, uvmexp.freetarg, 0, 0);
		uvm_unlock_pageq();
		uvm_swapout_threads();
		uvm_lock_pageq();

	}
#endif

	/*
	 * now we want to work on meeting our targets.   first we work on our
	 * free target by converting inactive pages into free pages.  then
	 * we work on meeting our inactive target by converting active pages
	 * to inactive ones.
	 */

	UVMHIST_LOG(pdhist, "  starting 'free' loop",0,0,0,0);

	pages_freed = uvmexp.pdfreed;
	uvmpd_scan_queue();
	pages_freed = uvmexp.pdfreed - pages_freed;

	/*
	 * detect if we're not going to be able to page anything out
	 * until we free some swap resources from active pages.
	 */

	swap_shortage = 0;
	if (uvmexp.free < uvmexp.freetarg &&
	    uvmexp.swpginuse >= uvmexp.swpgavail &&
	    !uvm_swapisfull() &&
	    pages_freed == 0) {
		swap_shortage = uvmexp.freetarg - uvmexp.free;
	}

	uvmpdpol_balancequeue(swap_shortage);
}

/*
 * uvm_reclaimable: decide whether to wait for pagedaemon.
 *
 * => return TRUE if it seems to be worth to do uvm_wait.
 *
 * XXX should be tunable.
 * XXX should consider pools, etc?
 */

boolean_t
uvm_reclaimable(void)
{
	int filepages;
	int active, inactive;

	/*
	 * if swap is not full, no problem.
	 */

	if (!uvm_swapisfull()) {
		return TRUE;
	}

	/*
	 * file-backed pages can be reclaimed even when swap is full.
	 * if we have more than 1/16 of pageable memory or 5MB, try to reclaim.
	 *
	 * XXX assume the worst case, ie. all wired pages are file-backed.
	 *
	 * XXX should consider about other reclaimable memory.
	 * XXX ie. pools, traditional buffer cache.
	 */

	filepages = uvmexp.filepages + uvmexp.execpages - uvmexp.wired;
	uvm_estimatepageable(&active, &inactive);
	if (filepages >= MIN((active + inactive) >> 4,
	    5 * 1024 * 1024 >> PAGE_SHIFT)) {
		return TRUE;
	}

	/*
	 * kill the process, fail allocation, etc..
	 */

	return FALSE;
}

void
uvm_estimatepageable(int *active, int *inactive)
{

	uvmpdpol_estimatepageable(active, inactive);
}
