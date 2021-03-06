/*	$NetBSD: verified_exec.c,v 1.52.2.1 2007/01/19 22:12:47 bouyer Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org>
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brett Lymn and Elad Efrat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of The NetBSD Foundation nor the names of its
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
#if defined(__NetBSD__)
__KERNEL_RCSID(0, "$NetBSD: verified_exec.c,v 1.52.2.1 2007/01/19 22:12:47 bouyer Exp $");
#else
__RCSID("$Id: verified_exec.c,v 1.52.2.1 2007/01/19 22:12:47 bouyer Exp $\n$NetBSD: verified_exec.c,v 1.52.2.1 2007/01/19 22:12:47 bouyer Exp $");
#endif

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/verified_exec.h>
#include <sys/kauth.h>
#include <sys/syslog.h>

#ifdef __FreeBSD__
#include <sys/kernel.h>
#include <sys/device_port.h>
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#include <sys/device.h>
#define DEVPORT_DEVICE struct device
#endif

#include <prop/proplib.h>

struct veriexec_softc {
        DEVPORT_DEVICE veriexec_dev;
};

#if defined(__FreeBSD__)
# define CDEV_MAJOR 216
# define BDEV_MAJOR -1
#endif

const struct cdevsw veriexec_cdevsw = {
        veriexecopen,
	veriexecclose,
	noread,
	nowrite,
        veriexecioctl,
#ifdef __NetBSD__
	nostop,
	notty,
#endif
	nopoll,
	nommap,
#if defined(__NetBSD__)
       nokqfilter,
       D_OTHER,
#elif defined(__FreeBSD__)
       nostrategy,
       "veriexec",
       CDEV_MAJOR,
       nodump,
       nopsize,
       0,                              /* flags */
       BDEV_MAJOR
#endif
};

static int veriexec_query(prop_dictionary_t, prop_dictionary_t, struct lwp *);
static int veriexec_delete(prop_dictionary_t, struct lwp *);

/* count of number of times device is open (we really only allow one open) */
static unsigned int veriexec_dev_usage;

void
veriexecattach(DEVPORT_DEVICE *parent, DEVPORT_DEVICE *self,
    void *aux)
{
	veriexec_dev_usage = 0;

	if (veriexec_verbose >= 2)
		log(LOG_DEBUG, "Veriexec: Pseudo-device attached.\n");
}

int
veriexecopen(dev_t dev, int flags,
		 int fmt, struct lwp *l)
{
	if (veriexec_verbose >= 2) {
		log(LOG_DEBUG, "Veriexec: Pseudo-device open attempt by "
		    "uid=%u, pid=%u. (dev=%u)\n",
		    kauth_cred_geteuid(l->l_cred), l->l_proc->p_pid,
		    dev);
	}

	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    &l->l_acflag) != 0)
		return (EPERM);

	if (veriexec_dev_usage > 0) {
		if (veriexec_verbose >= 2)
			log(LOG_ERR, "Veriexec: pseudo-device already in "
			    "use.\n");

		return(EBUSY);
	}

	veriexec_dev_usage++;
	return (0);
}

int
veriexecclose(dev_t dev, int flags, int fmt,
    struct lwp *l)
{
	if (veriexec_dev_usage > 0)
		veriexec_dev_usage--;
	return (0);
}

int
veriexecioctl(dev_t dev, u_long cmd, caddr_t data, int flags,
    struct lwp *l)
{
	struct plistref *plistref;
	prop_dictionary_t dict;
	int error = 0;

	if (veriexec_strict > VERIEXEC_LEARNING) {
		log(LOG_WARNING, "Veriexec: Strict mode, modifying tables not "
		    "permitted.\n");

		return (EPERM);
	}

	plistref = (struct plistref *)data;

	switch (cmd) {
	case VERIEXEC_TABLESIZE:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_table_add(l, dict);
		prop_object_release(dict);
		break;

	case VERIEXEC_LOAD:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_file_add(l, dict);
		prop_object_release(dict);
		break;

	case VERIEXEC_DELETE:
		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			break;

		error = veriexec_delete(dict, l);
		prop_object_release(dict);
		break;

	case VERIEXEC_QUERY: {
		prop_dictionary_t rdict;

		error = prop_dictionary_copyin_ioctl(plistref, cmd, &dict);
		if (error)
			return (error);

		rdict = prop_dictionary_create();
		if (rdict == NULL) {
			error = ENOMEM;
			break;
		}

		error = veriexec_query(dict, rdict, l);
		if (error == 0) {
			error = prop_dictionary_copyout_ioctl(plistref, cmd,
			    rdict);
		}

		prop_object_release(rdict);
		prop_object_release(dict);

		break;
		}

	default:
		/* Invalid operation. */
		error = ENODEV;
		break;
	}

	return (error);
}

#if defined(__FreeBSD__)
static void
veriexec_drvinit(void *unused)
{
	make_dev(&verifiedexec_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600,
	    "veriexec");
	verifiedexecattach(0, 0, 0);
}

SYSINIT(veriexec, SI_SUB_PSEUDO, SI_ORDER_ANY, veriexec_drvinit, NULL);
#endif

static int
veriexec_delete(prop_dictionary_t dict, struct lwp *l)
{
	struct nameidata nid;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "file")), l);
	error = namei(&nid);
	if (error)
		return (error);

	/* XXX this should be done differently... */
	if (nid.ni_vp->v_type == VREG)
		error = veriexec_file_delete(l, nid.ni_vp);
	else if (nid.ni_vp->v_type == VDIR)
		error = veriexec_table_delete(l, nid.ni_vp->v_mount);

	vrele(nid.ni_vp);

	return (error);
}

static int
veriexec_query(prop_dictionary_t dict, prop_dictionary_t rdict, struct lwp *l)
{
	struct nameidata nid;
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "file")), l);
	error = namei(&nid);
	if (error)
		return (error);

	error = veriexec_convert(nid.ni_vp, rdict);

	vrele(nid.ni_vp);

	return (error);
}
