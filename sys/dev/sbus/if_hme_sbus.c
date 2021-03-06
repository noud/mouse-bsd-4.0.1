/*	$NetBSD: if_hme_sbus.c,v 1.21 2005/12/11 12:23:44 christos Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * SBus front-end device driver for the HME ethernet device.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_hme_sbus.c,v 1.21 2005/12/11 12:23:44 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <dev/ic/hmevar.h>

struct hmesbus_softc {
	struct	hme_softc	hsc_hme;	/* HME device */
	struct	sbusdev		hsc_sbus;	/* SBus device */
};

int	hmematch_sbus(struct device *, struct cfdata *, void *);
void	hmeattach_sbus(struct device *, struct device *, void *);

CFATTACH_DECL(hme_sbus, sizeof(struct hmesbus_softc),
    hmematch_sbus, hmeattach_sbus, NULL, NULL);

int
hmematch_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_name, sa->sa_name) == 0 ||
	    strcmp("SUNW,qfe", sa->sa_name) == 0 ||
	    strcmp("SUNW,hme", sa->sa_name) == 0);
}

void
hmeattach_sbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct hmesbus_softc *hsc = (void *)self;
	struct hme_softc *sc = &hsc->hsc_hme;
	struct sbusdev *sd = &hsc->hsc_sbus;
	u_int32_t burst, sbusburst;
	int node;

	node = sa->sa_node;

	/* Pass on the bus tags */
	sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;

	printf(": Sun Happy Meal Ethernet (%s)\n",
	    sa->sa_name);

	if (sa->sa_nreg < 5) {
		printf("%s: only %d register sets\n",
			self->dv_xname, sa->sa_nreg);
		return;
	}

	/*
	 * Map five register banks:
	 *
	 *	bank 0: HME SEB registers
	 *	bank 1: HME ETX registers
	 *	bank 2: HME ERX registers
	 *	bank 3: HME MAC registers
	 *	bank 4: HME MIF registers
	 *
	 */
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[0].oa_space,
			 sa->sa_reg[0].oa_base,
			 (bus_size_t)sa->sa_reg[0].oa_size,
			 0, &sc->sc_seb) != 0) {
		printf("%s: cannot map SEB registers\n", self->dv_xname);
		return;
	}
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[1].oa_space,
			 sa->sa_reg[1].oa_base,
			 (bus_size_t)sa->sa_reg[1].oa_size,
			 0, &sc->sc_etx) != 0) {
		printf("%s: cannot map ETX registers\n", self->dv_xname);
		return;
	}
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[2].oa_space,
			 sa->sa_reg[2].oa_base,
			 (bus_size_t)sa->sa_reg[2].oa_size,
			 0, &sc->sc_erx) != 0) {
		printf("%s: cannot map ERX registers\n", self->dv_xname);
		return;
	}
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[3].oa_space,
			 sa->sa_reg[3].oa_base,
			 (bus_size_t)sa->sa_reg[3].oa_size,
			 0, &sc->sc_mac) != 0) {
		printf("%s: cannot map MAC registers\n", self->dv_xname);
		return;
	}
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[4].oa_space,
			 sa->sa_reg[4].oa_base,
			 (bus_size_t)sa->sa_reg[4].oa_size,
			 0, &sc->sc_mif) != 0) {
		printf("%s: cannot map MIF registers\n", self->dv_xname);
		return;
	}

	sd->sd_reset = (void *)hme_reset;
	sbus_establish(sd, self);

	prom_getether(node, sc->sc_enaddr);

	/*
	 * Get transfer burst size from PROM and pass it on
	 * to the back-end driver.
	 */
	sbusburst = ((struct sbus_softc *)parent)->sc_burst;
	if (sbusburst == 0)
		sbusburst = SBUS_BURST_32 - 1; /* 1->16 */

	burst = prom_getpropint(node, "burst-sizes", -1);
	if (burst == -1)
		/* take SBus burst sizes */
		burst = sbusburst;

	/* Clamp at parent's burst sizes */
	burst &= sbusburst;

	/* Translate into plain numerical format */
	sc->sc_burst =  (burst & SBUS_BURST_32) ? 32 :
			(burst & SBUS_BURST_16) ? 16 : 0;

	sc->sc_pci = 0; /* XXXXX should all be done in bus_dma. */
	hme_config(sc);

	/* Establish interrupt handler */
	if (sa->sa_nintr != 0)
		(void)bus_intr_establish(sa->sa_bustag, sa->sa_pri, IPL_NET,
					 hme_intr, sc);
}
