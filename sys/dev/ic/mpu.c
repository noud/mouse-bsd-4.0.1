/*	$NetBSD: mpu.c,v 1.14 2006/11/16 01:32:51 christos Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org).
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
__KERNEL_RCSID(0, "$NetBSD: mpu.c,v 1.14 2006/11/16 01:32:51 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/midi_if.h>

#include <dev/ic/mpuvar.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (mpudebug) printf x
#define DPRINTFN(n,x)	if (mpudebug >= (n)) printf x
int	mpudebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define MPU_DATA		0
#define MPU_COMMAND	1
#define  MPU_RESET	0xff
#define  MPU_UART_MODE	0x3f
#define  MPU_ACK		0xfe
#define MPU_STATUS	1
#define  MPU_OUTPUT_BUSY	0x40
#define  MPU_INPUT_EMPTY	0x80

#define MPU_MAXWAIT	10000	/* usec/10 to wait */

#define MPU_GETSTATUS(iot, ioh) (bus_space_read_1(iot, ioh, MPU_STATUS))

int	mpu_reset(struct mpu_softc *);
static	inline int mpu_waitready(struct mpu_softc *);
void	mpu_readinput(struct mpu_softc *);

int	mpu_open(void *, int,
			 void (*iintr)(void *, int),
			 void (*ointr)(void *), void *arg);
void	mpu_close(void *);
int	mpu_output(void *, int);
void	mpu_getinfo(void *, struct midi_info *);

const struct midi_hw_if mpu_midi_hw_if = {
	mpu_open,
	mpu_close,
	mpu_output,
	mpu_getinfo,
	0,			/* ioctl */
};

int
mpu_find(sc)
	struct mpu_softc *sc;
{
	if (MPU_GETSTATUS(sc->iot, sc->ioh) == 0xff) {
		DPRINTF(("mpu_find: No status\n"));
		goto bad;
	}
	sc->open = 0;
	sc->intr = 0;
	if (mpu_reset(sc) == 0)
		return 1;
bad:
	return 0;
}

void
mpu_attach(sc)
	struct mpu_softc *sc;
{
	midi_attach_mi(&mpu_midi_hw_if, sc, &sc->sc_dev);
}

static inline int
mpu_waitready(sc)
	struct mpu_softc *sc;
{
	int i;

	for(i = 0; i < MPU_MAXWAIT; i++) {
		if (!(MPU_GETSTATUS(sc->iot, sc->ioh) & MPU_OUTPUT_BUSY))
			return 0;
		delay(10);
	}
	return 1;
}

int
mpu_reset(sc)
	struct mpu_softc *sc;
{
	bus_space_tag_t iot = sc->iot;
	bus_space_handle_t ioh = sc->ioh;
	int i;
	int s;

	if (mpu_waitready(sc)) {
		DPRINTF(("mpu_reset: not ready\n"));
		return EIO;
	}
	s = splaudio();		/* Don't let the interrupt get our ACK. */
	bus_space_write_1(iot, ioh, MPU_COMMAND, MPU_RESET);
	for(i = 0; i < 2*MPU_MAXWAIT; i++) {
		if (!(MPU_GETSTATUS(iot, ioh) & MPU_INPUT_EMPTY) &&
		    bus_space_read_1(iot, ioh, MPU_DATA) == MPU_ACK) {
			splx(s);
			return 0;
		}
	}
	splx(s);
	DPRINTF(("mpu_reset: No ACK\n"));
	return EIO;
}

int
mpu_open(void *addr, int flags, void (*iintr)(void *, int),
    void (*ointr)(void *), void *arg)
{
	struct mpu_softc *sc = addr;

        DPRINTF(("mpu_open: sc=%p\n", sc));

	if (sc->open)
		return EBUSY;
#ifndef AUDIO_NO_POWER_CTL
	if (sc->powerctl)
		sc->powerctl(sc->powerarg, 1);
#endif
	if (mpu_reset(sc) != 0) {
#ifndef AUDIO_NO_POWER_CTL
		if (sc->powerctl)
			sc->powerctl(sc->powerarg, 0);
#endif
		return EIO;
	}

	bus_space_write_1(sc->iot, sc->ioh, MPU_COMMAND, MPU_UART_MODE);
	sc->open = 1;
	sc->intr = iintr;
	sc->arg = arg;
	return 0;
}

void
mpu_close(addr)
	void *addr;
{
	struct mpu_softc *sc = addr;

        DPRINTF(("mpu_close: sc=%p\n", sc));

	sc->open = 0;
	sc->intr = 0;
	mpu_reset(sc); /* exit UART mode */

#ifndef AUDIO_NO_POWER_CTL
	if (sc->powerctl)
		sc->powerctl(sc->powerarg, 0);
#endif
}

void
mpu_readinput(sc)
	struct mpu_softc *sc;
{
	bus_space_tag_t iot = sc->iot;
	bus_space_handle_t ioh = sc->ioh;
	int data;

	while(!(MPU_GETSTATUS(iot, ioh) & MPU_INPUT_EMPTY)) {
		data = bus_space_read_1(iot, ioh, MPU_DATA);
		DPRINTFN(3, ("mpu_rea: sc=%p 0x%02x\n", sc, data));
		if (sc->intr)
			sc->intr(sc->arg, data);
	}
}

int
mpu_output(addr, d)
	void *addr;
	int d;
{
	struct mpu_softc *sc = addr;
	int s;

	DPRINTFN(3, ("mpu_output: sc=%p 0x%02x\n", sc, d));
	if (!(MPU_GETSTATUS(sc->iot, sc->ioh) & MPU_INPUT_EMPTY)) {
		s = splaudio();
		mpu_readinput(sc);
		splx(s);
	}
	if (mpu_waitready(sc)) {
		DPRINTF(("mpu_output: not ready\n"));
		return EIO;
	}
	bus_space_write_1(sc->iot, sc->ioh, MPU_DATA, d);
	return 0;
}

void
mpu_getinfo(addr, mi)
	void *addr;
	struct midi_info *mi;
{
	struct mpu_softc *sc = addr;

	mi->name = sc->model;
	mi->props = MIDI_PROP_CAN_INPUT;
}

int
mpu_intr(addr)
	void *addr;
{
	struct mpu_softc *sc = addr;

	if (MPU_GETSTATUS(sc->iot, sc->ioh) & MPU_INPUT_EMPTY) {
		DPRINTF(("mpu_intr: no data\n"));
		return (0);
	} else {
		mpu_readinput(sc);
		return (1);
	}
}
