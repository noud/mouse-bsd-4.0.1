/*	$NetBSD: exception_vector.S,v 1.20 2006/09/21 20:40:16 uwe Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "assym.h"

#include <sh3/param.h>
#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/exception.h>
#include <sh3/ubcreg.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

__KERNEL_RCSID(0, "$NetBSD: exception_vector.S,v 1.20 2006/09/21 20:40:16 uwe Exp $")


/*
 * Exception vectors. The following routines are copied to vector addreses.
 *	sh_vector_generic:	VBR + 0x100
 *	sh_vector_tlbmiss:	VBR + 0x400
 *	sh_vector_interrupt:	VBR + 0x600
 */

#define VECTOR_END_MARKER(sym)			\
		.globl	_C_LABEL(sym);		\
	_C_LABEL(sym):


/*
 * LINTSTUB: Var: char sh_vector_generic[1];
 *
 * void sh_vector_generic(void) __attribute__((__noreturn__))
 *	Copied to VBR+0x100.  This code should be position independent
 *	and no more than 786 bytes long (== 0x400 - 0x100).
 */
NENTRY(sh_vector_generic)
	__EXCEPTION_ENTRY
	__INTR_MASK(r0, r1)
	/* Identify exception cause */
	MOV	(EXPEVT, r0)
	mov.l	@r0,	r0
	mov.l	r0,	@(TF_EXPEVT, r14) /* trapframe->tf_expevt = EXPEVT */
	/* Get curlwp */
	mov.l	_L.curlwp, r1
	mov.l	@r1,	r4	/* 1st arg */
	/* Get TEA */
	MOV	(TEA,	r1)
	mov.l	@r1,	r6	/* 3rd arg */
	/* Check TLB exception or not */
	mov.l	_L.TLB_PROT_ST, r1
	cmp/hi	r1,	r0
	bt	1f

	/* tlb_exception(curlwp, trapframe, TEA); */
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	_L.tlb, r0
	jsr	@r0
	 mov	r14,	r5	/* 2nd arg */
	bra	2f
	 nop

	/* general_exception(curlwp, trapframe, TEA); */
1:	mov	r4,	r8
#ifdef DDB
	mov	#0,	r2
	MOV	(BBRA, r1)
	mov.w	r2,	@r1	/* disable UBC */
	mov.l	r2,	@(TF_UBC, r14)	/* clear trapframe->tf_ubc */
#endif /* DDB */
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	_L.general, r0
	jsr	@r0
	 mov	r14,	r5	/* 2nd arg */

	/* Check for ASTs on exit to user mode. */
	mov	r8,	r4
	mov.l	_L.ast,	r0
	jsr	@r0
	 mov	r14,	r5
#ifdef DDB	/* BBRA = trapframe->tf_ubc */
	__EXCEPTION_BLOCK(r0, r1)
	mov.l	@(TF_UBC, r14), r0
	MOV	(BBRA, r1)
	mov.w	r0,	@r1
#endif /* DDB */
2:	__EXCEPTION_RETURN
	/* NOTREACHED */
	.align	2
_L.curlwp:	.long	_C_LABEL(curlwp)
REG_SYMBOL(EXPEVT)
REG_SYMBOL(BBRA)
REG_SYMBOL(TEA)
_L.tlb:		.long	_C_LABEL(tlb_exception)
_L.general:	.long	_C_LABEL(general_exception)
_L.ast:		.long	_C_LABEL(ast)
_L.TLB_PROT_ST:	.long	0xc0

/* LINTSTUB: Var: char sh_vector_generic_end[1]; */
VECTOR_END_MARKER(sh_vector_generic_end)
	SET_ENTRY_SIZE(sh_vector_generic)


#ifdef SH3
/*
 * LINTSTUB: Var: char sh3_vector_tlbmiss[1];
 *
 * void sh3_vector_tlbmiss(void) __attribute__((__noreturn__))
 *	Copied to VBR+0x400.  This code should be position independent
 *	and no more than 512 bytes long (== 0x600 - 0x400).
 */
NENTRY(sh3_vector_tlbmiss)
	__EXCEPTION_ENTRY
	mov	#(SH3_TEA & 0xff), r0
	mov.l	@r0,	r6	/* 3rd arg: va = TEA */
#if !defined(P1_STACK)
	/* Load kernel stack */
	mov.l	__L.VPN_MASK, r0
	and	r6,	r0
	tst	r0,	r0	/* check VPN == 0 */
	bt	6f
	mov.l	_L.CURUPTE, r1
	mov.l	@r1,	r1
	mov	#UPAGES,r3
	mov	#1,	r2
4:	mov.l	@r1+,	r7
	cmp/eq	r7,	r0	/* md_upte.addr: u-area VPN */
	bt	5f
	add	#4,	r1	/* skip md_upte.data */
	cmp/eq	r2,	r3
	bf/s	4b
	 add	#1,	r2
	bra	7f		/* pull insn at 6f into delay slot */
	 mov	#(SH3_EXPEVT & 0xff), r0
5:	mov.l	@r1,	r2	/* md_upte.data: u-area PTE */
	mov	#(SH3_PTEL & 0xff), r1
	mov.l	r2,	@r1
	mov	#(SH3_PTEH & 0xff), r1
	mov.l	@r1,	r2
	mov.l	__L.VPN_MASK, r0
	and	r2,	r0
	mov.l	r0,	@r1	/* ASID 0 */
	ldtlb
	bra	3f
	 mov.l	r2,	@r1	/* restore ASID */
#endif /* !P1_STACK */
6:	mov	#(SH3_EXPEVT & 0xff), r0
7:	mov.l	@r0,	r0
	mov.l	r0,	@(TF_EXPEVT, r14) /* trapframe->tf_expevt = EXPEVT */
	mov.l	2f,	r0
	mov.l	@r0,	r4	/* 1st arg */
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	1f,	r0
	jsr	@r0
	 mov	r14,	r5	/* 2nd arg */
3:	__EXCEPTION_RETURN
	.align	2
2:		.long	_C_LABEL(curlwp)
1:		.long	_C_LABEL(tlb_exception)
__L.VPN_MASK:	.long	0xfffff000
_L.CURUPTE:	.long	_C_LABEL(curupte)

/* LINTSTUB: Var: char sh3_vector_tlbmiss_end[1]; */
VECTOR_END_MARKER(sh3_vector_tlbmiss_end)
	SET_ENTRY_SIZE(sh3_vector_tlbmiss)
#endif /* SH3 */


#ifdef SH4
/*
 * LINTSTUB: Var: char sh4_vector_tlbmiss[1];
 *
 * void sh4_vector_tlbmiss(void) __attribute__((__noreturn__))
 *	Copied to VBR+0x400.  This code should be position independent
 *	and no more than 512 bytes long (== 0x600 - 0x400).
 */
NENTRY(sh4_vector_tlbmiss)
	__EXCEPTION_ENTRY
	mov.l	_L.TEA4, r0
	mov.l	@r0,	r6
	mov.l	_L.EXPEVT4, r0
	mov.l	@r0,	r0
	mov.l	r0,	@(TF_EXPEVT, r14) /* trapframe->tf_expevt = EXPEVT */
	mov.l	2f,	r0
	mov.l	@r0,	r4	/* 1st arg */
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	1f,	r0
	jsr	@r0
	 mov	r14,	r5	/* 2nd arg */
	__EXCEPTION_RETURN
	.align	2
1:		.long	_C_LABEL(tlb_exception)
2:		.long	_C_LABEL(curlwp)
_L.EXPEVT4:	.long	SH4_EXPEVT
_L.TEA4:	.long	SH4_TEA

/* LINTSTUB: Var: char sh4_vector_tlbmiss_end[1]; */
VECTOR_END_MARKER(sh4_vector_tlbmiss_end)
	SET_ENTRY_SIZE(sh4_vector_tlbmiss)
#endif /* SH4 */


/*
 * LINTSTUB: Var: char sh_vector_interrupt[1];
 *
 * void sh_vector_interrupt(void) __attribute__((__noreturn__)):
 *	copied to VBR+0x600. This code should be relocatable.
 */
NENTRY(sh_vector_interrupt)
	__EXCEPTION_ENTRY
	xor	r0,	r0
	mov.l	r0,	@(TF_EXPEVT, r14) /* (for debug) */
	stc	r0_bank,r6	/* ssp */
	/* Enable exception for P3 access */
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	/* uvmexp.intrs++ */
	mov.l	__L.uvmexp.intrs, r0
	mov.l	@r0,	r1
	add	#1	r1
	mov.l	r1,	@r0
	/* Dispatch interrupt handler */
	mov.l	__L.intc_intr, r0
	jsr	@r0		/* intc_intr(ssr, spc, ssp) */
	 nop
	/* Check for ASTs on exit to user mode. */
	mov.l	1f,	r0
	mov.l	@r0,	r4	/* 1st arg */
	mov.l	__L.ast, r0
	jsr	@r0
	 mov	r14,	r5	/* 2nd arg */
	__EXCEPTION_RETURN
	.align	2
1:			.long	_C_LABEL(curlwp)
__L.intc_intr:		.long	_C_LABEL(intc_intr)
__L.ast:		.long	_C_LABEL(ast)
__L.uvmexp.intrs:	.long	_C_LABEL(uvmexp) + UVMEXP_INTRS

/* LINTSTUB: Var: char sh_vector_interrupt_end[1]; */
VECTOR_END_MARKER(sh_vector_interrupt_end)
	SET_ENTRY_SIZE(sh_vector_interrupt)
