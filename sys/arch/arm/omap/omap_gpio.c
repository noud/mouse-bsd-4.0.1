/*	$NetBSD: omap_gpio.c,v 1.2.6.1 2007/02/21 18:40:59 snj Exp $ */

/*
 * The OMAP GPIO Controller interface is inspired by pxa2x0_gpio.c
 *
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap_gpio.c,v 1.2.6.1 2007/02/21 18:40:59 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/omap/omap_tipb.h>
#include <arm/omap/omap_gpio.h>
#include <arm/omap/omap_gpioreg.h>

#include "omapgpio.h"

#define GPIOEVNAMESZ 25

struct gpio_irq_handler {
	int (*gh_func)(void *);
	void *gh_arg;
	int gh_spl;
	u_int gh_gpio;
    	char name[GPIOEVNAMESZ];
	struct evcnt ev;
};

struct omapgpio_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	void *sc_irqcookie;
	u_int16_t sc_mask;
	struct gpio_irq_handler *sc_handlers[GPIO_NPINS];
};

static int	omapgpio_match(struct device *, struct cfdata *, void *);
static void	omapgpio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(omapgpio, sizeof(struct omapgpio_softc),
    omapgpio_match, omapgpio_attach, NULL, NULL);

static struct omapgpio_softc *omapgpio_units[NOMAPGPIO];

static int	omapgpio_intr(void *);

static int
omapgpio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1 || tipb->tipb_intr == -1) {
		panic("omapgpio must have addr and intr specified in config.");
	}

	tipb->tipb_size = OMAP_GPIO_SIZE;

	return (1);
}

void
omapgpio_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapgpio_softc *sc = (struct omapgpio_softc*) self;
	struct tipb_attach_args *tipb = aux;
	u_int32_t reg;

	sc->sc_bust = tipb->tipb_iot;

	aprint_normal(": GPIO Controller\n");

	if (self->dv_unit > NOMAPGPIO - 1) {
		aprint_error("%s: Unsupported GPIO module unit number.\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_map(sc->sc_bust, tipb->tipb_addr, tipb->tipb_size, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Failed to map registers.\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_mask = 0;
	memset(sc->sc_handlers, 0, sizeof(sc->sc_handlers));

	/* Reset the module and wait for it to come back online. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SYSCONFIG, 0x3);
	do {
		reg = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_SYSSTATUS);
	} while ((reg & 1) == 0);

	/* Install our ISR. */
	sc->sc_irqcookie = omap_intr_establish(tipb->tipb_intr, IPL_BIO,
	    sc->sc_dev.dv_xname, omapgpio_intr, sc);
	if (sc->sc_irqcookie == NULL) {
		aprint_error("%s: Failed to install interrupt handler.\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Save off our softc. */
	omapgpio_units[self->dv_unit] = sc;
}

u_int
omap_gpio_get_direction(u_int gpio)
{
	struct omapgpio_softc *sc;
	u_int32_t reg, bit;
	u_int rval;

	KDASSERT(gpio < NOMAPGPIO * GPIO_NPINS);

	sc = omapgpio_units[GPIO_MODULE(gpio)];
	if (sc == NULL) {
		panic("omapgpio: GPIO Module for pin %d not configured.\n", gpio);
	}

	rval = 0;
	bit = GPIO_BIT(gpio);
	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_DIRECTION);

	if ((reg & bit) == 0)
		/* Output. */
		rval |= GPIO_OUT;

	return (rval);
}

void
omap_gpio_set_direction(u_int gpio, u_int dir)
{
	struct omapgpio_softc *sc;
	u_int32_t reg, bit;

	KDASSERT(gpio < NOMAPGPIO * GPIO_NPINS);

	sc = omapgpio_units[GPIO_MODULE(gpio)];
	if (sc == NULL) {
		panic("omapgpio: GPIO Module for pin %d not configured.\n", gpio);
	}

	bit = GPIO_BIT(gpio);
	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush,
	    GPIO_DIRECTION);

	if (GPIO_IS_IN(dir)) {
		/* Input. */
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
		    GPIO_DIRECTION, reg | bit);
	} else {
		/* Output. */
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
		    GPIO_DIRECTION, (reg & ~bit));
	}
}

u_int omap_gpio_read(u_int gpio)
{
	struct omapgpio_softc *sc;
	u_int bit;

	sc = omapgpio_units[GPIO_MODULE(gpio)];

	if (sc == NULL)
		panic("omapgpio: GPIO Module for pin %d not configured.",
		      gpio);

	bit = GPIO_BIT(gpio);

	return (bus_space_read_4(sc->sc_bust, sc->sc_bush,
				 GPIO_DATAIN) & bit) ? 1 : 0;
}

void omap_gpio_write(u_int gpio, u_int value)
{
	struct omapgpio_softc *sc;
	u_int bit;

	sc = omapgpio_units[GPIO_MODULE(gpio)];

	if (sc == NULL)
		panic("omapgpio: GPIO Module for pin %d not configured.",
		      gpio);

	bit = GPIO_BIT(gpio);

	if (value) {
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
				  GPIO_SET_DATAOUT, bit);
	} else {
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
				  GPIO_CLEAR_DATAOUT, bit);
	}
}

void *
omap_gpio_intr_establish(u_int gpio, int level, int spl,
    const char *name, int (*func)(void *), void *arg)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh;
	u_int32_t bit, levelreg;
	u_int dir, relnum, off, reg;
	int levelctrl;

	KDASSERT(gpio < NOMAPGPIO * GPIO_NPINS);

	sc = omapgpio_units[GPIO_MODULE(gpio)];
	if (sc == NULL) {
		panic("omapgpio: GPIO Module for pin %d not configured.", gpio);
	}

	dir = omap_gpio_get_direction(gpio);
	if (!GPIO_IS_IN(dir)) {
		panic("omapgpio: GPIO pin %d not an input.", gpio);
	}

	relnum = GPIO_RELNUM(gpio);
	bit = GPIO_BIT(gpio);

	if (sc->sc_handlers[relnum] != NULL) {
		panic("omapgpio: Illegal shared interrupt on pin %d", gpio);
	}

	MALLOC(gh, struct gpio_irq_handler *, sizeof(struct gpio_irq_handler),
	    M_DEVBUF, M_NOWAIT);

	if (gh == NULL)
		return gh;

	gh->gh_func = func;
	gh->gh_arg = arg;
	gh->gh_spl = spl;
	gh->gh_gpio = gpio;
	sc->sc_handlers[relnum] = gh;
	sc->sc_mask |= bit;
	snprintf(gh->name, GPIOEVNAMESZ, "#%d %s", gpio, name);
	evcnt_attach_dynamic(&gh->ev, EVCNT_TYPE_INTR, NULL,
			     "omap gpio", gh->name);

	/*
	 * Set up the level control.
	 *
	 * Note: Pins 0->7 on a module use EDGE_CTRL1, pins 8->15 use
	 * EDGE_CTRL2.
	 */
	levelctrl = 0;
	switch (level) {
	case IST_EDGE_FALLING:
	case IST_LEVEL_LOW: /* emulation */
		levelctrl = 1;
		break;
	case IST_EDGE_RISING:
	case IST_LEVEL_HIGH: /* emulation */
		levelctrl = 2;
		break;
	case IST_EDGE_BOTH:
		levelctrl = 3;
		break;
	default:
		panic("omapgpio: Unknown level %d.", level);
		/* NOTREACHED */
	}

	off = 2 * (relnum & 7);
	if (relnum < 8) {
		reg = GPIO_EDGE_CTRL1;
	} else {
		reg = GPIO_EDGE_CTRL2;
	}

	/* Temporarily set the level control to no trigger. */
	levelreg = bus_space_read_4(sc->sc_bust, sc->sc_bush, reg);
	levelreg &= ~(0x3 << off);
 	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, levelreg);

	/* Clear the IRQSTATUS bit for the pin we're about to change. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS, bit);

	/* Set the new level control value. */
	levelreg |= levelctrl << off;
	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, levelreg);

	/* Set the wakeup enable for this pin. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SET_WAKEUPENA,
	    bit);

	/* Enable interrupt generation for that pin. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SET_IRQENABLE,
	    bit);

	return (gh);
}

void
omap_gpio_intr_disestablish(void *cookie)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh = cookie;
	u_int32_t bit;
	u_int gpio, relnum;

	KDASSERT(cookie != NULL);

	gpio = gh->gh_gpio;
	sc = omapgpio_units[GPIO_MODULE(gpio)];
	bit = GPIO_BIT(gpio);
	relnum = GPIO_RELNUM(gpio);
	evcnt_detach(&gh->ev);

	/* Disable Wakeup enable for this gpio. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_CLEAR_WAKEUPENA,
	    bit);

	/* Disable interrupt generation for that gpio. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_CLEAR_IRQENABLE,
	    bit);

	sc->sc_mask &= ~bit;
	sc->sc_handlers[relnum] = NULL;

	FREE(gh, M_DEVBUF);
}

static int
omapgpio_intr(void *arg)
{
	struct omapgpio_softc *sc = arg;
	struct gpio_irq_handler *gh;
	u_int32_t gedr;
	int idx, handled, s, nattempts;

	/* Fetch the GPIO interrupts pending.  */
	gedr = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS);
	gedr &= GPIO_REG_MASK;
 	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS, gedr);

	/*
	 * Since IRQSTATUS can change out from under us while we are busy
	 * servicing everything, keep on doing things until the IRQSTATUS
	 * register is clear.
	 */
	for (nattempts = 0, handled = 0;;) {
		/*
		 * Note: Apparently the GPIO block will set the bits in IRQSTATUS if
		 * the level trigger for that pin is set to anything other than NONE
		 * regardless of the IRQENABLE status.  Just mask off the ones that we
		 * care about when processing the ISR.
		 */
		gedr &= sc->sc_mask;
		if (gedr == 0) {
			/* Pretend that we handled everything. */
			return (1);
		}

		for (idx = 0; idx < GPIO_NPINS; idx++, gedr >>= 1) {
			if ((gedr & 1) == 0)
				continue;

			if ((gh = sc->sc_handlers[idx]) == NULL) {
				printf("%s: unhandled GPIO interrupt. GPIO# %d\n",
				    sc->sc_dev.dv_xname, idx);
				continue;
			}

			gh->ev.ev_count++;
			s = _splraise(gh->gh_spl);
			handled |= (gh->gh_func)(gh->gh_arg);
  			splx(s);
		}

		/* Check IRQSTATUS again. */
		gedr = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS);
		gedr &= GPIO_REG_MASK;
		if (gedr == 0) {
			/* Done servicing interrupts. */
			break;
		} else if (nattempts++ == 10000) {
			/* TODO: Fix up the # of attempts and this logic after
			   some experimentation. */

			/* Ensure that we don't get stuck here. */
			panic("%s: Stuck in GPIO interrupt service routine.",
			    sc->sc_dev.dv_xname);
		}
		bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS, gedr);
	}

	return (handled);
}

