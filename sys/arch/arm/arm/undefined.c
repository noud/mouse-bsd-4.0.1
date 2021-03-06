/*	$NetBSD: undefined.c,v 1.27 2006/07/19 21:11:40 ad Exp $	*/

/*
 * Copyright (c) 2001 Ben Harris.
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * undefined.c
 *
 * Fault handler
 *
 * Created      : 06/01/95
 */

#define FAST_FPE

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

__KERNEL_RCSID(0, "$NetBSD: undefined.c,v 1.27 2006/07/19 21:11:40 ad Exp $");

#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syslog.h>
#include <sys/vmmeter.h>
#include <sys/savar.h>
#ifdef FAST_FPE
#include <sys/acct.h>
#endif
#include <sys/userret.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>
#include <machine/trap.h>

#include <arch/arm/arm/disassem.h>

#ifdef DDB
#include <ddb/db_output.h>
#include <machine/db_machdep.h>
#endif

#ifdef acorn26
#include <machine/machdep.h>
#endif

static int gdb_trapper(u_int, u_int, struct trapframe *, int);

#ifdef FAST_FPE
extern int want_resched;
#endif

LIST_HEAD(, undefined_handler) undefined_handlers[NUM_UNKNOWN_HANDLERS];


void *
install_coproc_handler(int coproc, undef_handler_t handler)
{
	struct undefined_handler *uh;

	KASSERT(coproc >= 0 && coproc < NUM_UNKNOWN_HANDLERS);
	KASSERT(handler != NULL); /* Used to be legal. */

	/* XXX: M_TEMP??? */
	MALLOC(uh, struct undefined_handler *, sizeof(*uh), M_TEMP, M_WAITOK);
	uh->uh_handler = handler;
	install_coproc_handler_static(coproc, uh);
	return uh;
}

void
install_coproc_handler_static(int coproc, struct undefined_handler *uh)
{

	LIST_INSERT_HEAD(&undefined_handlers[coproc], uh, uh_link);
}

void
remove_coproc_handler(void *cookie)
{
	struct undefined_handler *uh = cookie;

	LIST_REMOVE(uh, uh_link);
	FREE(uh, M_TEMP);
}


static int
gdb_trapper(u_int addr, u_int insn, struct trapframe *frame, int code)
{
	struct lwp *l;
	l = (curlwp == NULL) ? &lwp0 : curlwp;

#ifdef THUMB_CODE
	if (frame->tf_spsr & PSR_T_bit) {
		if (insn == GDB_THUMB_BREAKPOINT)
			goto bkpt;
	}
	else
#endif
	{
		if (insn == GDB_BREAKPOINT || insn == GDB5_BREAKPOINT) {
#ifdef THUMB_CODE
		bkpt:
#endif
			if (code == FAULT_USER) {
				ksiginfo_t ksi;

				KSI_INIT_TRAP(&ksi);
				ksi.ksi_signo = SIGTRAP;
				ksi.ksi_code = TRAP_BRKPT;
				ksi.ksi_addr = (u_int32_t *)addr;
				ksi.ksi_trap = 0;
				KERNEL_PROC_LOCK(l);
				trapsignal(l, &ksi);
				KERNEL_PROC_UNLOCK(l);
				return 0;
			}
#ifdef KGDB
			return !kgdb_trap(T_BREAKPOINT, frame);
#endif
		}
	}
	return 1;
}

static struct undefined_handler gdb_uh;
#ifdef THUMB_CODE
static struct undefined_handler gdb_uh_thumb;
#endif

void
undefined_init()
{
	int loop;

	/* Not actually necessary -- the initialiser is just NULL */
	for (loop = 0; loop < NUM_UNKNOWN_HANDLERS; ++loop)
		LIST_INIT(&undefined_handlers[loop]);

	/* Install handler for GDB breakpoints */
	gdb_uh.uh_handler = gdb_trapper;
	install_coproc_handler_static(CORE_UNKNOWN_HANDLER, &gdb_uh);
#ifdef THUMB_CODE
	gdb_uh_thumb.uh_handler = gdb_trapper;
	install_coproc_handler_static(THUMB_UNKNOWN_HANDLER, &gdb_uh_thumb);
#endif
}

void
undefinedinstruction(trapframe_t *frame)
{
	struct lwp *l;
	u_int fault_pc;
	int fault_instruction;
	int fault_code;
	int coprocessor;
	int user;
	struct undefined_handler *uh;
#ifdef VERBOSE_ARM32
	int s;
#endif

	/* Enable interrupts if they were enabled before the exception. */
#ifdef acorn26
	if ((frame->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
#else
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);
#endif

#ifndef acorn26
#ifdef THUMB_CODE
	if (frame->tf_spsr & PSR_T_bit)
		frame->tf_pc -= THUMB_INSN_SIZE;
	else
#endif
	{
		frame->tf_pc -= INSN_SIZE;
	}
#endif

#ifdef __PROG26
	fault_pc = frame->tf_r15 & R15_PC;
#else
	fault_pc = frame->tf_pc;
#endif

	/* Get the current lwp/proc structure or lwp0/proc0 if there is none. */
	l = curlwp == NULL ? &lwp0 : curlwp;

#ifdef __PROG26
	if ((frame->tf_r15 & R15_MODE) == R15_MODE_USR) {
#else
	if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE) {
#endif
		user = 1;
		LWP_CACHE_CREDS(l, l->l_proc);
	} else
		user = 0;


#ifdef THUMB_CODE
	if (frame->tf_spsr & PSR_T_bit) {
		fault_instruction = fusword((void *)(fault_pc & ~1));
	}
	else
#endif
	{
		/*
		 * Make sure the program counter is correctly aligned so we
		 * don't take an alignment fault trying to read the opcode.
		 */
		if (__predict_false((fault_pc & 3) != 0)) {
			ksiginfo_t ksi;
			/* Give the user an illegal instruction signal. */
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGILL;
			ksi.ksi_code = ILL_ILLOPC;
			ksi.ksi_addr = (u_int32_t *)(intptr_t) fault_pc;
			KERNEL_PROC_LOCK(l);
			trapsignal(l, &ksi);
			KERNEL_PROC_UNLOCK(l);
			userret(l);
			return;
		}
	 	/*
		 * Should use fuword() here .. but in the interests of
		 * squeezing every  bit of speed we will just use
		 * ReadWord(). We know the instruction can be read
		 * as was just executed so this will never fail unless
		 * the kernel is screwed up in which case it does
		 * not really matter does it ?
		 */

		fault_instruction = *(u_int32_t *)fault_pc;
	}

	/* Update vmmeter statistics */
	uvmexp.traps++;

#ifdef THUMB_CODE
	if (frame->tf_spsr & PSR_T_bit) {
		coprocessor = THUMB_UNKNOWN_HANDLER;
	}
	else
#endif
	{
		/* Check for coprocessor instruction */

		/*
		 * According to the datasheets you only need to look at
		 * bit 27 of the instruction to tell the difference
		 * between and undefined instruction and a coprocessor
		 * instruction following an undefined instruction trap.
		 *
		 * ARMv5 adds undefined instructions in the NV space,
		 * even when bit 27 is set.
		 */

		if ((fault_instruction & (1 << 27)) != 0
		    && (fault_instruction & 0xf0000000) != 0xf0000000)
			coprocessor = (fault_instruction >> 8) & 0x0f;
		else
			coprocessor = CORE_UNKNOWN_HANDLER;
	}

	if (user) {
		/*
		 * Modify the fault_code to reflect the USR/SVC state at
		 * time of fault.
		 */
		fault_code = FAULT_USER;
		l->l_addr->u_pcb.pcb_tf = frame;
	} else
		fault_code = 0;

	/* OK this is were we do something about the instruction. */
	LIST_FOREACH(uh, &undefined_handlers[coprocessor], uh_link)
	    if (uh->uh_handler(fault_pc, fault_instruction, frame,
			       fault_code) == 0)
		    break;

	if (uh == NULL) {
		/* Fault has not been handled */
		ksiginfo_t ksi; 
		
#ifdef VERBOSE_ARM32
		s = spltty();

		if ((fault_instruction & 0x0f000010) == 0x0e000000) {
			printf("CDP\n");
			disassemble(fault_pc);
		} else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
			printf("LDC/STC\n");
			disassemble(fault_pc);
		} else if ((fault_instruction & 0x0f000010) == 0x0e000010) {
			printf("MRC/MCR\n");
			disassemble(fault_pc);
		} else if ((fault_instruction & ~INSN_COND_MASK)
			 != (KERNEL_BREAKPOINT & ~INSN_COND_MASK)) {
			printf("Undefined instruction\n");
			disassemble(fault_pc);
		}

		splx(s);
#endif
        
		if ((fault_code & FAULT_USER) == 0) {
#ifdef DDB
			db_printf("Undefined instruction in kernel\n");
			kdb_trap(T_FAULT, frame);
#else
			panic("undefined instruction in kernel");
#endif
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (u_int32_t *)fault_pc;
		ksi.ksi_trap = fault_instruction;
		KERNEL_PROC_LOCK(l);
		trapsignal(l, &ksi);
		KERNEL_PROC_UNLOCK(l);
	}

	if ((fault_code & FAULT_USER) == 0)
		return;

#ifdef FAST_FPE
	/* Optimised exit code */
	{

		/*
		 * Check for reschedule request, at the moment there is only
		 * 1 ast so this code should always be run
		 */

		if (want_resched) {
			/*
			 * We are being preempted.
			 */
			preempt(0);
		}

		/* Invoke MI userret code */
		mi_userret(l);

		l->l_priority = l->l_usrpri;

		curcpu()->ci_schedstate.spc_curpriority = l->l_priority;
	}

#else
	userret(l);
#endif
}
