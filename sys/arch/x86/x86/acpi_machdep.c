/*	$NetBSD: acpi_machdep.c,v 1.12.6.2 2007/10/14 00:32:20 xtraeme Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent routines for ACPICA.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.12.6.2 2007/10/14 00:32:20 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

#include <machine/acpi_machdep.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>
#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/pic.h>

#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include "ioapic.h"

#include "acpi.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

static int acpi_intrcold = 1;

struct acpi_intr_defer {
	UINT32	number;
	ACPI_OSD_HANDLER function;
	void *context;
	void *ih;
	LIST_ENTRY(acpi_intr_defer) list;
};

static LIST_HEAD(, acpi_intr_defer) acpi_intr_deferq =
    LIST_HEAD_INITIALIZER(acpi_intr_deferq);

ACPI_STATUS
acpi_md_OsInitialize(void)
{

	/* Nothing to do, yet. */
	return (AE_OK);
}

ACPI_STATUS
acpi_md_OsTerminate(void)
{

	/* Nothing to do, yet. */
	return (AE_OK);
}

ACPI_STATUS
acpi_md_OsGetRootPointer(UINT32 Flags, ACPI_POINTER *PhysicalAddress)
{

	return (AcpiFindRootPointer(Flags, PhysicalAddress));
}

ACPI_STATUS
acpi_md_OsInstallInterruptHandler(UINT32 InterruptNumber,
    ACPI_OSD_HANDLER ServiceRoutine, void *Context, void **cookiep)
{
	void *ih;
	struct pic *pic;
	int irq, pin, trigger;
	struct acpi_intr_defer *aip;

	if (acpi_intrcold) {
		aip = malloc(sizeof(struct acpi_intr_defer), M_TEMP, M_WAITOK);
		aip->number = InterruptNumber;
		aip->function = ServiceRoutine;
		aip->context = Context;
		aip->ih = NULL;

		LIST_INSERT_HEAD(&acpi_intr_deferq, aip, list);

		*cookiep = (void *)aip;
		return AE_OK;
	}

	trigger = IST_LEVEL;

#if NACPI > 0 && NIOAPIC > 0
	/*
	 * Can only match on ACPI global interrupt numbers if the ACPI
	 * interrupt info was extracted, which is in the ACPI case.
	 */
	if (mpacpi_sci_override != NULL) {
		pic = mpacpi_sci_override->ioapic;
		pin = mpacpi_sci_override->ioapic_pin;
		if (mpacpi_sci_override->redir & IOAPIC_REDLO_LEVEL)
			trigger = IST_LEVEL;
		else
			trigger = IST_EDGE;
		if (pic->pic_type == PIC_IOAPIC)
			irq = -1;
		else
			irq = (int)InterruptNumber;
		goto sci_override;
	}
#endif

 	/*
 	 * There was no ACPI interrupt source override,
 	 *
 	 * If the interrupt is handled via IOAPIC, mark it
 	 * as level-triggered, active low in the table.
 	 */
 
#if NIOAPIC > 0
	pic = (struct pic *)ioapic_find_bybase(InterruptNumber);
	if (pic != NULL) {
		struct ioapic_softc *sc = (struct ioapic_softc *)pic;
		struct mp_intr_map *mip;

		if (pic->pic_type == PIC_IOAPIC) {
			pin = (int)InterruptNumber - pic->pic_vecbase;
			irq = -1;
		} else {
			irq = pin = (int)InterruptNumber;
		}

		mip = sc->sc_pins[pin].ip_map;
		if (mip) {
			mip->flags &= ~3;
			mip->flags |= MPS_INTPO_ACTLO;
			mip->redir |= IOAPIC_REDLO_ACTLO;
		}
	} else
#endif
	{
		pic = &i8259_pic;
		irq = pin = (int)InterruptNumber;
	}

#if NACPI > 0 && NIOAPIC > 0
#endif

	trigger = IST_LEVEL;

#if NIOAPIC > 0
sci_override:
#endif
	/*
	 * XXX probably, IPL_BIO is enough.
	 */
	ih = intr_establish(irq, pic, pin, trigger, IPL_VM,
	    (int (*)(void *)) ServiceRoutine, Context);
	if (ih == NULL)
		return (AE_NO_MEMORY);
	*cookiep = ih;
	return (AE_OK);
}

void
acpi_md_OsRemoveInterruptHandler(void *cookie)
{
	struct acpi_intr_defer *aip;

	LIST_FOREACH(aip, &acpi_intr_deferq, list) {
		if (aip == cookie) {
			if (aip->ih != NULL)
				intr_disestablish(aip->ih);
			return;
		}
	}

	intr_disestablish(cookie);
}

ACPI_STATUS
acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress,
    UINT32 Length, void **LogicalAddress)
{

	if (_x86_memio_map(X86_BUS_SPACE_MEM, PhysicalAddress, Length,
	    0, (bus_space_handle_t *) LogicalAddress) == 0)
		return (AE_OK);

	return (AE_NO_MEMORY);
}

void
acpi_md_OsUnmapMemory(void *LogicalAddress, UINT32 Length)
{

	(void) _x86_memio_unmap(X86_BUS_SPACE_MEM,
	    (bus_space_handle_t) LogicalAddress, Length, NULL);
}

ACPI_STATUS
acpi_md_OsGetPhysicalAddress(void *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), (vaddr_t) LogicalAddress, &pa)) {
		*PhysicalAddress = pa;
		return (AE_OK);
	}

	return (AE_ERROR);
}

BOOLEAN
acpi_md_OsReadable(void *Pointer, UINT32 Length)
{
	BOOLEAN rv = TRUE;
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t) Pointer);
	eva = round_page((vaddr_t) Pointer + Length);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return (FALSE);

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & PG_V) == 0) {
			rv = FALSE;
			break;
		}
	}

	return (rv);
}

BOOLEAN
acpi_md_OsWritable(void *Pointer, UINT32 Length)
{
	BOOLEAN rv = FALSE;
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t) Pointer);
	eva = round_page((vaddr_t) Pointer + Length);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return (FALSE);

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & (PG_V|PG_W)) != (PG_V|PG_W)) {
			rv = FALSE;
			break;
		}
	}

	return (rv);
}

void
acpi_md_OsDisableInterrupt(void)
{
	disable_intr();
}

void
acpi_md_callback(struct device *acpi)
{
	struct acpi_intr_defer *aip;

#if NACPI > 0
#ifdef MPBIOS
	if (!mpbios_scanned)
#endif
	mpacpi_find_interrupts(acpi);
#endif
	acpi_intrcold = 0;

	/* Proces deferred interrupt handler establish calls. */
	LIST_FOREACH(aip, &acpi_intr_deferq, list) {
		acpi_md_OsInstallInterruptHandler(aip->number, aip->function,
		    aip->context, &aip->ih);
	}
}
