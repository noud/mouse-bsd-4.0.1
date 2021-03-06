/*	$NetBSD: machdep.c,v 1.88 2005/12/24 20:06:58 perry Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.88 2005/12/24 20:06:58 perry Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ksyms.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/bootinfo.h>
#include <machine/autoconf.h>
#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <powerpc/oea/bat.h>

#include <dev/cons.h>

#include "pfb.h"
#include "ksyms.h"

#include "pc.h"
#if (NPC > 0)
#include <machine/pccons.h>
#endif

#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pckbport/pckbportvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Global variables used here and there
 */
char bootinfo[BOOTINFO_MAXSIZE];

paddr_t bebox_mb_reg;		/* BeBox MotherBoard register */

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

char *bootpath;

paddr_t avail_end;			/* XXX temporary */

void bebox_bus_space_init(void);
void consinit(void);
void ext_intr(void);

extern void *startsym, *endsym;

void
initppc(startkernel, endkernel, args, btinfo)
	u_int startkernel, endkernel, args;
	void *btinfo;
{
	/*
	 * copy bootinfo
	 */
	memcpy(bootinfo, btinfo, sizeof (bootinfo));

	/*
	 * BeBox memory region set
	 */
	{
		struct btinfo_memory *meminfo;

		meminfo =
			(struct btinfo_memory *)lookup_bootinfo(BTINFO_MEMORY);
		if (!meminfo)
			panic("not found memory information in bootinfo");
		physmemr[0].start = 0;
		physmemr[0].size = meminfo->memsize & ~PGOFSET;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = meminfo->memsize - availmemr[0].start;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Get CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		extern u_long ticks_per_sec, ns_per_tick;

		clockinfo =
			(struct btinfo_clock *)lookup_bootinfo(BTINFO_CLOCK);
		if (!clockinfo)
			panic("not found clock information in bootinfo");
		ticks_per_sec = clockinfo->ticks_per_sec;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * BeBox MotherBoard's Register
	 *  Interrupt Mask Reset
	 */
	*(volatile u_int *)(MOTHER_BOARD_REG + CPU0_INT_MASK) = 0x0ffffffc;
	*(volatile u_int *)(MOTHER_BOARD_REG + CPU0_INT_MASK) = 0x80000023;
	*(volatile u_int *)(MOTHER_BOARD_REG + CPU1_INT_MASK) = 0x0ffffffc;

	/*
	 * boothowto
	 */
	boothowto = args;

	/*
	 * Set up initial BAT table
	 */
	oea_batinit(
	    BEBOX_BUS_SPACE_IO,  BAT_BL_256M,
	    BEBOX_BUS_SPACE_MEM, BAT_BL_256M,
	    0);

	/*
	 * Initialize the vector table and interrupt routine.
	 */
	oea_init(ext_intr);

	/*
	 * Init the I/O stuff before the console
	 */
	bebox_bus_space_init();

	/*
	 * i386 port says, that this shouldn't be here,
	 * but I really think the console should be initialized
	 * as early as possible.
	 */
	consinit();

        /*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif
#ifdef IPKDB
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
}

void
mem_regions(mem, avail)
	struct mem_region **mem, **avail;
{
	*mem = physmemr;
	*avail = availmemr;
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup()
{
	/*
	 * BeBox Mother Board's Register Mapping
	 */
	bebox_mb_reg = (vaddr_t) mapiodev(MOTHER_BOARD_REG, PAGE_SIZE);
	if (!bebox_mb_reg)
		panic("cpu_startup: no room for interrupt register");

	/*
	 * Do common VM initialization
	 */
	oea_startup(NULL);

	/*
	 * Now that we have VM, malloc's are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;

		splhigh();
		__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
	}
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
		(size_t)help < (size_t)bootinfo + sizeof (bootinfo));

	return (NULL);
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	struct btinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

	consinfo = (struct btinfo_console *)lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
		panic("not found console information in bootinfo");

#if (NPFB > 0)
	if (!strcmp(consinfo->devname, "be")) {
		pfb_cnattach(consinfo->addr);
#if (NPCKBC > 0)
		pckbc_cnattach(&bebox_isa_io_bs_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif

#if (NPC > 0) || (NVGA > 0)
	if (!strcmp(consinfo->devname, "vga")) {
#if (NVGA > 0)
		if (!vga_cnattach(&bebox_io_bs_tag, &bebox_mem_bs_tag,
				  -1, 1))
			goto dokbd;
#endif
#if (NPC > 0)
		pccnattach();
#endif
#if (NVGA > 0)
dokbd:
#endif
#if (NPCKBC > 0)
		pckbc_cnattach(&bebox_isa_io_bs_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* PC | VGA */

#if (NCOM > 0)
	if (!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = &bebox_isa_io_bs_tag;

		if(comcnattach(tag, consinfo->addr, consinfo->speed,
		    COM_FREQ, COM_TYPE_NORMAL,
		    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
			panic("can't init serial console");

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#if (NPCKBC > 0) && (NPCKBD == 0)
/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbport_machdep_cnattach(kbctag, kbcslot)
	pckbport_tag_t kbctag;
	pckbport_slot_t kbcslot;
{
#if (NPC > 0)
	return (pcconskbd_cnattach(kbctag, kbcslot));
#else
	return (ENXIO);
#endif
}
#endif

/*
 * Stray interrupts.
 */
void
strayintr(irq)
	int irq;
{
	log(LOG_ERR, "stray interrupt %d\n", irq);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(howto, what)
	int howto;
	char *what;
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		printf("halted\n\n");
#if 0
		ppc_exit();
#endif
	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
	doshutdownhooks();
	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;
#if 0
	ppc_boot(str);
#endif
	while (1);
}

void
lcsplx(ipl)
	int ipl;
{
	splx(ipl);
}

struct powerpc_bus_space bebox_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	BEBOX_BUS_SPACE_IO, 0x00000000, 0x3f800000,
};
struct powerpc_bus_space bebox_isa_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	BEBOX_BUS_SPACE_IO, 0x00000000, 0x00010000,
};
struct powerpc_bus_space bebox_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	BEBOX_BUS_SPACE_MEM, 0x00000000, 0x3f000000,
};
struct powerpc_bus_space bebox_isa_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	BEBOX_BUS_SPACE_MEM, 0x00000000, 0x01000000,
};

static char ex_storage[2][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

void
bebox_bus_space_init(void)
{
	int error;

	error = bus_space_init(&bebox_io_bs_tag, "ioport",
	    ex_storage[0], sizeof(ex_storage[0]));
	if (error)
		panic("bebox_bus_space_init: can't init io tag");

	error = extent_alloc_region(bebox_io_bs_tag.pbs_extent,
	    0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("bebox_bus_space_init: can't block out reserved I/O"
		    " space 0x10000-0x7fffff: error=%d", error);
	error = bus_space_init(&bebox_mem_bs_tag, "iomem",
	    ex_storage[1], sizeof(ex_storage[1]));
	if (error)
		panic("bebox_bus_space_init: can't init mem tag");

	bebox_isa_io_bs_tag.pbs_extent = bebox_io_bs_tag.pbs_extent;
	error = bus_space_init(&bebox_isa_io_bs_tag, "isa-ioport", NULL, 0);
	if (error)
		panic("bebox_bus_space_init: can't init isa io tag");

	bebox_isa_mem_bs_tag.pbs_extent = bebox_mem_bs_tag.pbs_extent;
	error = bus_space_init(&bebox_isa_mem_bs_tag, "isa-iomem", NULL, 0);
	if (error)
		panic("bebox_bus_space_init: can't init isa mem tag");
}
