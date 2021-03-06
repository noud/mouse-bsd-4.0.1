/*	$NetBSD: nullcons_subr.c,v 1.4 2005/12/11 12:20:53 christos Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nullcons_subr.c,v 1.4 2005/12/11 12:20:53 christos Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <dev/cons.h>


extern struct consdev *cn_tab;		/* physical console device info */

static struct tty *nulltty;		/* null console tty */

cons_decl(null);

dev_type_read(nullcndev_read);
dev_type_ioctl(nullcndev_ioctl);
dev_type_tty(nullcndev_tty);

static int	nullcons_newdev(struct consdev *);

const struct cdevsw nullcn_devsw = {
	nullopen, nullclose, nullcndev_read, nullwrite, nullcndev_ioctl,
	nullstop, nullcndev_tty, nopoll, nommap, ttykqfilter, D_TTY
};

/*
 * null console device. We need it because of the ioctl() it handles,
 * which in particular allows control terminal allocation through
 * TIOCSCTTY ioctl. Without the latter, system won't even boot past init(8)
 * invocation.
 */
int
nullcndev_read(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{

	for(;;);
	return (0);
}

int
nullcndev_ioctl(dev, cmd, data, flag, l)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct lwp *l;
{
	int error;

	error = (*nulltty->t_linesw->l_ioctl)(nulltty, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(nulltty, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	return (0);
}

struct tty*
nullcndev_tty(dev)
	dev_t dev;
{

	return nulltty;
}

/*
 * Mark console as no-op (null) console. Proper initialization is deferred
 * to nullconsattach().
 */
void
nullcnprobe(cn)
	struct consdev *cn;
{

	cn->cn_pri = CN_NULL;
	cn->cn_dev = NODEV;
}

/*
 * null console initialization. This includes allocation of a new device and
 * a new tty.
 */
void
nullcninit(cn)
	struct consdev *cn;
{
	static struct consdev nullcn = cons_init(null);

	nullcnprobe(&nullcn);
	cn_tab = &nullcn;
}

/*
 * Dumb getc() implementation. Simply blocks on call.
 */
int
nullcngetc(dev)
	dev_t dev;
{

	for(;;);
	return (0);
}

/*
 * Dumb putc() implementation.
 */
void
nullcnputc(dev, c)
	dev_t dev;
	int c;
{

}

/*
 * Allocate a new console device and a tty to handle console ioctls.
 */
int
nullcons_newdev(cn)
	struct consdev *cn;
{
	int error;
	int bmajor = -1, cmajor = -1;

	if ((cn == NULL) || (cn->cn_pri != CN_NULL) || (cn->cn_dev != NODEV))
		return (0);

	/*
	 * Attach no-op device to the device list.
	 */
	error = devsw_attach("nullcn", NULL, &bmajor, &nullcn_devsw, &cmajor);
	if (error != 0)
		return (error);

	/*
	 * Allocate tty (mostly to have sane ioctl()).
	 */
	nulltty = ttymalloc();
	nulltty->t_dev = makedev(cmajor, 0);
	tty_attach(nulltty);
	cn->cn_dev = nulltty->t_dev;

	return (0);
}

/*
 * Pseudo-device attach function -- it's the right time to do the rest of
 * initialization.
 */
void
nullconsattach(pdev_count)
	int pdev_count;
{

	nullcons_newdev(cn_tab);
}
