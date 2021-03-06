/*	$NetBSD: pci_usrreq.c,v 1.13 2006/11/16 01:33:09 christos Exp $	*/

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
 * User -> kernel interface for PCI bus access.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_usrreq.c,v 1.13 2006/11/16 01:33:09 christos Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/fcntl.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciio.h>

static int
pciopen(dev_t dev, int flags, int mode,
    struct lwp *l)
{
	struct pci_softc *sc;
	int unit;

	unit = minor(dev);
	sc = device_lookup(&pci_cd, unit);
	if (sc == NULL)
		return (ENXIO);

	return (0);
}

static int
pciioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct pci_softc *sc = device_lookup(&pci_cd, minor(dev));
	struct pciio_bdf_cfgreg *bdfr = (void *) data;
	struct pciio_businfo *binfo = (void *) data;
	pcitag_t tag;

	switch (cmd) {
	case PCI_IOC_BDF_CFGREAD:
	case PCI_IOC_BDF_CFGWRITE:
		if (bdfr->bus > 255 || bdfr->device >= sc->sc_maxndevs ||
		    bdfr->function > 7)
			return (EINVAL);
		tag = pci_make_tag(sc->sc_pc, bdfr->bus, bdfr->device,
		    bdfr->function);
		if (cmd == PCI_IOC_BDF_CFGREAD)
			bdfr->cfgreg.val = pci_conf_read(sc->sc_pc, tag,
			    bdfr->cfgreg.reg);
		else {
			if ((flag & FWRITE) == 0)
				return (EBADF);
			pci_conf_write(sc->sc_pc, tag, bdfr->cfgreg.reg,
			    bdfr->cfgreg.val);
		}
		break;

	case PCI_IOC_BUSINFO:
		binfo->busno = sc->sc_bus;
		binfo->maxdevs = sc->sc_maxndevs;
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

static paddr_t
pcimmap(dev_t dev, off_t offset, int prot)
{
#if 0
	struct pci_softc *sc = device_lookup(&pci_cd, minor(dev));

	/*
	 * Since we allow mapping of the entire bus, we
	 * take the offset to be the address on the bus,
	 * and pass 0 as the offset into that range.
	 *
	 * XXX Need a way to deal with linear/prefetchable/etc.
	 */
	return (bus_space_mmap(sc->sc_memt, offset, 0, prot, 0));
#else
	/* XXX Consider this further. */
	return (-1);
#endif
}

const struct cdevsw pci_cdevsw = {
	pciopen, nullclose, noread, nowrite, pciioctl,
	nostop, notty, nopoll, pcimmap, nokqfilter, D_OTHER,
};

/*
 * pci_devioctl:
 *
 *	PCI ioctls that can be performed on devices directly.
 */
int
pci_devioctl(pci_chipset_tag_t pc, pcitag_t tag, u_long cmd, caddr_t data,
    int flag, struct lwp *l)
{
	struct pciio_cfgreg *r = (void *) data;

	switch (cmd) {
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		if (cmd == PCI_IOC_CFGREAD)
			r->val = pci_conf_read(pc, tag, r->reg);
		else {
			if ((flag & FWRITE) == 0)
				return (EBADF);
			pci_conf_write(pc, tag, r->reg, r->val);
		}
		break;

	default:
		return (EPASSTHROUGH);
	}

	return (0);
}
