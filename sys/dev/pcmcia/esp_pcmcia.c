/*	$NetBSD: esp_pcmcia.c,v 1.29 2006/11/16 01:33:20 christos Exp $	*/

/*-
 * Copyright (c) 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: esp_pcmcia.c,v 1.29 2006/11/16 01:33:20 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

struct esp_pcmcia_softc {
	struct ncr53c9x_softc	sc_ncr53c9x;	/* glue to MI code */

	int		sc_active;		/* Pseudo-DMA state vars */
	int		sc_tc;
	int		sc_datain;
	size_t		sc_dmasize;
	size_t		sc_dmatrans;
	char		**sc_dmaaddr;
	size_t		*sc_pdmalen;

	/* PCMCIA-specific goo. */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */
#ifdef ESP_PCMCIA_POLL
	struct callout sc_poll_ch;
#endif
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;

	int sc_state;
#define ESP_PCMCIA_ATTACHED	3
};

int	esp_pcmcia_match(struct device *, struct cfdata *, void *);
int	esp_pcmcia_validate_config(struct pcmcia_config_entry *);
void	esp_pcmcia_attach(struct device *, struct device *, void *);
void	esp_pcmcia_init(struct esp_pcmcia_softc *);
int	esp_pcmcia_detach(struct device *, int);
int	esp_pcmcia_enable(struct device *, int);

CFATTACH_DECL(esp_pcmcia, sizeof(struct esp_pcmcia_softc),
    esp_pcmcia_match, esp_pcmcia_attach, esp_pcmcia_detach, NULL);

/*
 * Functions and the switch for the MI code.
 */
#ifdef ESP_PCMCIA_POLL
void	esp_pcmcia_poll(void *);
#endif
u_char	esp_pcmcia_read_reg(struct ncr53c9x_softc *, int);
void	esp_pcmcia_write_reg(struct ncr53c9x_softc *, int, u_char);
int	esp_pcmcia_dma_isintr(struct ncr53c9x_softc *);
void	esp_pcmcia_dma_reset(struct ncr53c9x_softc *);
int	esp_pcmcia_dma_intr(struct ncr53c9x_softc *);
int	esp_pcmcia_dma_setup(struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *);
void	esp_pcmcia_dma_go(struct ncr53c9x_softc *);
void	esp_pcmcia_dma_stop(struct ncr53c9x_softc *);
int	esp_pcmcia_dma_isactive(struct ncr53c9x_softc *);

const struct ncr53c9x_glue esp_pcmcia_glue = {
	esp_pcmcia_read_reg,
	esp_pcmcia_write_reg,
	esp_pcmcia_dma_isintr,
	esp_pcmcia_dma_reset,
	esp_pcmcia_dma_intr,
	esp_pcmcia_dma_setup,
	esp_pcmcia_dma_go,
	esp_pcmcia_dma_stop,
	esp_pcmcia_dma_isactive,
	NULL,			/* gl_clear_latched_intr */
};

const struct pcmcia_product esp_pcmcia_products[] = {
	{ PCMCIA_VENDOR_PANASONIC, PCMCIA_PRODUCT_PANASONIC_KXLC002,
	  PCMCIA_CIS_PANASONIC_KXLC002 },

	{ PCMCIA_VENDOR_RATOC, PCMCIA_PRODUCT_RATOC_REX_9530,
	  PCMCIA_CIS_RATOC_REX_9530 },
};
const size_t esp_pcmcia_nproducts =
    sizeof(esp_pcmcia_products) / sizeof(esp_pcmcia_products[0]);

int
esp_pcmcia_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa, esp_pcmcia_products, esp_pcmcia_nproducts,
	    sizeof(esp_pcmcia_products[0]), NULL))
		return (1);
	return (0);
}

int
esp_pcmcia_validate_config(cfe)
	struct pcmcia_config_entry *cfe;
{
	if (cfe->iftype != PCMCIA_IFTYPE_IO ||
	    cfe->num_memspace != 0 ||
	    cfe->num_iospace != 1)
		return (EINVAL);
	return (0);
}

void
esp_pcmcia_attach(struct device *parent, struct device *self,
    void *aux)
{
	struct esp_pcmcia_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;
	int error;

	esc->sc_pf = pf;

	error = pcmcia_function_configure(pf, esp_pcmcia_validate_config);
	if (error) {
		aprint_error("%s: configure failed, error=%d\n", self->dv_xname,
		    error);
		return;
	}

	cfe = pf->cfe;
	esc->sc_iot = cfe->iospace[0].handle.iot;
	esc->sc_ioh = cfe->iospace[0].handle.ioh;
	esp_pcmcia_init(esc);

	error = esp_pcmcia_enable(self, 1);
	if (error)
		goto fail;

	sc->sc_adapter.adapt_minphys = minphys;
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	sc->sc_adapter.adapt_enable = esp_pcmcia_enable;
	sc->sc_adapter.adapt_refcnt = 1;

	ncr53c9x_attach(sc);
	scsipi_adapter_delref(&sc->sc_adapter);
	esc->sc_state = ESP_PCMCIA_ATTACHED;
	return;

fail:
	pcmcia_function_unconfigure(pf);
}

void
esp_pcmcia_init(esc)
	struct esp_pcmcia_softc *esc;
{
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;

	/* id 7, clock 40M, parity ON, sync OFF, fast ON, slow ON */

	sc->sc_glue = &esp_pcmcia_glue;

#ifdef ESP_PCMCIA_POLL
	callout_init(&esc->sc_poll_ch);
#endif

	sc->sc_rev = NCR_VARIANT_ESP406;
	sc->sc_id = 7;
	sc->sc_freq = 40;
	/* try -PARENB -SLOW */
	sc->sc_cfg1 = sc->sc_id | NCRCFG1_PARENB | NCRCFG1_SLOW;
	/* try +FE */
	sc->sc_cfg2 = NCRCFG2_SCSI2;
	/* try -IDM -FSCSI -FCLK */
	sc->sc_cfg3 = NCRESPCFG3_CDB | NCRESPCFG3_FCLK | NCRESPCFG3_IDM |
	    NCRESPCFG3_FSCSI;
	sc->sc_cfg4 = NCRCFG4_ACTNEG;
	/* try +INTP */
	sc->sc_cfg5 = NCRCFG5_CRS1 | NCRCFG5_AADDR | NCRCFG5_PTRINC;
	sc->sc_minsync = 0;
	sc->sc_maxxfer = 64 * 1024;
}

int
esp_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct esp_pcmcia_softc *sc = (void *)self;
	int error;

	if (sc->sc_state != ESP_PCMCIA_ATTACHED)
		return (0);

	error = ncr53c9x_detach(&sc->sc_ncr53c9x, flags);
	if (error)
		return (error);

	pcmcia_function_unconfigure(sc->sc_pf);

	return (0);
}

int
esp_pcmcia_enable(arg, onoff)
	struct device *arg;
	int onoff;
{
	struct esp_pcmcia_softc *sc = (void *)arg;
	int error;

	if (onoff) {
#ifdef ESP_PCMCIA_POLL
		callout_reset(&sc->sc_poll_ch, 1, esp_pcmcia_poll, sc);
#else
		/* Establish the interrupt handler. */
		sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_BIO,
		    ncr53c9x_intr, &sc->sc_ncr53c9x);
		if (!sc->sc_ih)
			return (EIO);
#endif

		error = pcmcia_function_enable(sc->sc_pf);
		if (error) {
			pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
			sc->sc_ih = 0;
			return (error);
		}

		/* Initialize only chip.  */
		ncr53c9x_init(&sc->sc_ncr53c9x, 0);
	} else {
		pcmcia_function_disable(sc->sc_pf);
#ifdef ESP_PCMCIA_POLL
		callout_stop(&sc->sc_poll_ch);
#else
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
		sc->sc_ih = 0;
#endif
	}

	return (0);
}

#ifdef ESP_PCMCIA_POLL
void
esp_pcmcia_poll(arg)
	void *arg;
{
	struct esp_pcmcia_softc *esc = arg;

	(void) ncr53c9x_intr(&esc->sc_ncr53c9x);
	callout_reset(&esc->sc_poll_ch, 1, esp_pcmcia_poll, esc);
}
#endif

/*
 * Glue functions.
 */
u_char
esp_pcmcia_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;
	u_char v;

	v = bus_space_read_1(esc->sc_iot, esc->sc_ioh, reg);
	return v;
}

void
esp_pcmcia_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;
	u_char v = val;

	if (reg == NCR_CMD && v == (NCRCMD_TRANS|NCRCMD_DMA))
		v = NCRCMD_TRANS;
	bus_space_write_1(esc->sc_iot, esc->sc_ioh, reg, v);
}

int
esp_pcmcia_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{

	return NCR_READ_REG(sc, NCR_STAT) & NCRSTAT_INT;
}

void
esp_pcmcia_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;

	esc->sc_active = 0;
	esc->sc_tc = 0;
}

int
esp_pcmcia_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;
	u_char	*p;
	u_int	espphase, espstat, espintr;
	int	cnt;

	if (esc->sc_active == 0) {
		printf("%s: dma_intr--inactive DMA\n", sc->sc_dev.dv_xname);
		return -1;
	}

	if ((sc->sc_espintr & NCRINTR_BS) == 0) {
		esc->sc_active = 0;
		return 0;
	}

	cnt = *esc->sc_pdmalen;
	if (*esc->sc_pdmalen == 0) {
		printf("%s: data interrupt, but no count left\n",
		    sc->sc_dev.dv_xname);
	}

	p = *esc->sc_dmaaddr;
	espphase = sc->sc_phase;
	espstat = (u_int) sc->sc_espstat;
	espintr = (u_int) sc->sc_espintr;
	do {
		if (esc->sc_datain) {
			*p++ = NCR_READ_REG(sc, NCR_FIFO);
			cnt--;
			if (espphase == DATA_IN_PHASE)
				NCR_WRITE_REG(sc, NCR_CMD, NCRCMD_TRANS);
			else
				esc->sc_active = 0;
		} else {
			if (espphase == DATA_OUT_PHASE ||
			    espphase == MESSAGE_OUT_PHASE) {
				NCR_WRITE_REG(sc, NCR_FIFO, *p++);
				cnt--;
				NCR_WRITE_REG(sc, NCR_CMD, NCRCMD_TRANS);
			} else
				esc->sc_active = 0;
		}

		if (esc->sc_active) {
			while (!(NCR_READ_REG(sc, NCR_STAT) & NCRSTAT_INT));
			espstat = NCR_READ_REG(sc, NCR_STAT);
			espintr = NCR_READ_REG(sc, NCR_INTR);
			espphase = (espintr & NCRINTR_DIS)
				    ? /* Disconnected */ BUSFREE_PHASE
				    : espstat & PHASE_MASK;
		}
	} while (esc->sc_active && espintr);
	sc->sc_phase = espphase;
	sc->sc_espstat = (u_char) espstat;
	sc->sc_espintr = (u_char) espintr;
	*esc->sc_dmaaddr = p;
	*esc->sc_pdmalen = cnt;

	if (*esc->sc_pdmalen == 0)
		esc->sc_tc = NCRSTAT_TC;
	sc->sc_espstat |= esc->sc_tc;
	return 0;
}

int
esp_pcmcia_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;

	esc->sc_dmaaddr = addr;
	esc->sc_pdmalen = len;
	esc->sc_datain = datain;
	esc->sc_dmasize = *dmasize;
	esc->sc_tc = 0;

	return 0;
}

void
esp_pcmcia_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;

	esc->sc_active = 1;
}

void
esp_pcmcia_dma_stop(struct ncr53c9x_softc *sc)
{
}

int
esp_pcmcia_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_pcmcia_softc *esc = (struct esp_pcmcia_softc *)sc;

	return (esc->sc_active);
}
