/*	$NetBSD: i80321_i2c.c,v 1.2 2005/12/11 12:16:51 christos Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
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
 * Intel i80321 I/O Processor I2C Controller Unit support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i80321_i2c.c,v 1.2 2005/12/11 12:16:51 christos Exp $");

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <dev/i2c/i2cvar.h>

#include <arm/xscale/iopi2creg.h>
#include <arm/xscale/iopi2cvar.h>

static int
iic321_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iopxs_attach_args *ia = aux;

	if (strcmp(cf->cf_name, ia->ia_name) == 0)
		return (1);

	return (0);
}

static void
iic321_attach(struct device *parent, struct device *self, void *aux)
{
	struct iopiic_softc *sc = (void *) self;
	struct iopxs_attach_args *ia = aux;
	int error;
	uint8_t gpio_bits;

	aprint_naive(": I2C controller\n");
	aprint_normal(": I2C controller\n");

	sc->sc_st = ia->ia_st;
	if ((error = bus_space_subregion(sc->sc_st, ia->ia_sh,
					 ia->ia_offset, ia->ia_size,
					 &sc->sc_sh)) != 0) {
		aprint_error("%s: unable to subregion registers, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}

	gpio_bits = (ia->ia_offset == VERDE_I2C_BASE0) ?
	    (1U << 7) | (1U << 6) : (1U << 5) | (1U << 4);
	i80321_gpio_set_val(gpio_bits, 0);
	i80321_gpio_set_direction(gpio_bits, 0);

	/* XXX Reset the I2C unit? */

	lockinit(&sc->sc_buslock, PRIBIO|PCATCH, "iopiiclk", 0, 0);

	/* XXX We don't currently use interrupts.  Fix this some day. */
#if 0
	sc->sc_ih = i80321_intr_establish((ia->ia_offset == VERDE_I2C_BASE0) ?
	    ICU_INT_I2C0 : ICU_INT_I2C1, IPL_BIO, iopiic_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: unable to establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return;
	}
#endif

	/*
	 * Enable the I2C unit as a master.
	 * No, we do not support slave mode.
	 */
	sc->sc_icr = IIC_ICR_GCD | IIC_ICR_UE | IIC_ICR_SCLE;
	bus_space_write_4(sc->sc_st, sc->sc_sh, IIC_ICR, 0);
	bus_space_write_4(sc->sc_st, sc->sc_sh, IIC_ISAR, 0);
	bus_space_write_4(sc->sc_st, sc->sc_sh, IIC_ICR, sc->sc_icr);

	iopiic_attach(sc);
}

CFATTACH_DECL(iopiic, sizeof(struct iopiic_softc),
    iic321_match, iic321_attach, NULL, NULL);
