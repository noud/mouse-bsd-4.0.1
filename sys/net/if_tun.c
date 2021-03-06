/*	$NetBSD: if_tun.c,v 1.94 2006/11/16 01:33:40 christos Exp $	*/

/*
 * Copyright (c) 1988, Julian Onions <jpo@cs.nott.ac.uk>
 * Nottingham University 1987.
 *
 * This source may be freely distributed, however I would be interested
 * in any changes that are made.
 *
 * This driver takes packets off the IP i/f and hands them up to a
 * user process to have its wicked way with. This driver has its
 * roots in a similar driver written by Phil Cockcroft (formerly) at
 * UCL. This driver is based much more on read/write/poll mode of
 * operation though.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_tun.c,v 1.94 2006/11/16 01:33:40 christos Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>


#if defined(INET) || defined(INET6)
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#endif


#include "bpfilter.h"
#if NBPFILTER > 0
#include <sys/time.h>
#include <net/bpf.h>
#endif

#include <net/if_tun.h>

#define TUNDEBUG	if (tundebug) printf
int	tundebug = 0;

extern int ifqmaxlen;
void	tunattach(int);

static LIST_HEAD(, tun_softc) tun_softc_list;
static LIST_HEAD(, tun_softc) tunz_softc_list;
static struct simplelock tun_softc_lock;

static int	tun_ioctl(struct ifnet *, u_long, caddr_t);
static int	tun_output(struct ifnet *, struct mbuf *, struct sockaddr *,
		       struct rtentry *rt);
static int	tun_clone_create(struct if_clone *, int);
static int	tun_clone_destroy(struct ifnet *);

static struct if_clone tun_cloner =
    IF_CLONE_INITIALIZER("tun", tun_clone_create, tun_clone_destroy);

static void tunattach0(struct tun_softc *);
#ifdef ALTQ
static void tunstart(struct ifnet *);
#endif
static struct tun_softc *tun_find_unit(dev_t);
static struct tun_softc *tun_find_zunit(int);

static dev_type_open(tunopen);
static dev_type_close(tunclose);
static dev_type_read(tunread);
static dev_type_write(tunwrite);
static dev_type_ioctl(tunioctl);
static dev_type_poll(tunpoll);
static dev_type_kqfilter(tunkqfilter);

const struct cdevsw tun_cdevsw = {
	tunopen, tunclose, tunread, tunwrite, tunioctl,
	nostop, notty, tunpoll, nommap, tunkqfilter, D_OTHER,
};

void
tunattach(int unused)
{

	simple_lock_init(&tun_softc_lock);
	LIST_INIT(&tun_softc_list);
	LIST_INIT(&tunz_softc_list);
	if_clone_attach(&tun_cloner);
}

/*
 * Find driver instance from dev_t.
 * Call at splnet().
 * Returns with tp locked (if found).
 */
static struct tun_softc *
tun_find_unit(dev_t dev)
{
	struct tun_softc *tp;
	int unit = minor(dev);

	simple_lock(&tun_softc_lock);
	LIST_FOREACH(tp, &tun_softc_list, tun_list)
		if (unit == tp->tun_unit)
			break;
	if (tp)
		simple_lock(&tp->tun_lock);
	simple_unlock(&tun_softc_lock);

	return (tp);
}

/*
 * Find zombie driver instance by unit number.
 * Call at splnet().
 * Remove tp from list and return it unlocked (if found).
 */
static struct tun_softc *
tun_find_zunit(int unit)
{
	struct tun_softc *tp;

	simple_lock(&tun_softc_lock);
	LIST_FOREACH(tp, &tunz_softc_list, tun_list)
		if (unit == tp->tun_unit)
			break;
	if (tp)
		LIST_REMOVE(tp, tun_list);
	simple_unlock(&tun_softc_lock);
#ifdef DIAGNOSTIC
	if (tp != NULL && (tp->tun_flags & (TUN_INITED|TUN_OPEN)) != TUN_OPEN)
		printf("tun%d: inconsistent flags: %x\n", unit, tp->tun_flags);
#endif

	return (tp);
}

static int
tun_clone_create(struct if_clone *ifc, int unit)
{
	struct tun_softc *tp;

	if ((tp = tun_find_zunit(unit)) == NULL) {
		/* Allocate a new instance */
		tp = malloc(sizeof(struct tun_softc), M_DEVBUF, M_WAITOK);
		(void)memset(tp, 0, sizeof(struct tun_softc));

		tp->tun_unit = unit;
		simple_lock_init(&tp->tun_lock);
	} else {
		/* Revive tunnel instance; clear ifp part */
		(void)memset(&tp->tun_if, 0, sizeof(struct ifnet));
	}

	(void)snprintf(tp->tun_if.if_xname, sizeof(tp->tun_if.if_xname),
			"%s%d", ifc->ifc_name, unit);
	tunattach0(tp);
	tp->tun_flags |= TUN_INITED;

	simple_lock(&tun_softc_lock);
	LIST_INSERT_HEAD(&tun_softc_list, tp, tun_list);
	simple_unlock(&tun_softc_lock);

	return (0);
}

static void
tunattach0(struct tun_softc *tp)
{
	struct ifnet *ifp;

	ifp = &tp->tun_if;
	ifp->if_softc = tp;
	ifp->if_mtu = TUNDEFMTU;
	ifp->if_ioctl = tun_ioctl;
	ifp->if_output = tun_output;
#ifdef ALTQ
	ifp->if_start = tunstart;
#endif
	ifp->if_flags = IFF_POINTOPOINT;
	ifp->if_type = IFT_TUNNEL;
	ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_collisions = 0;
	ifp->if_ierrors = 0;
	ifp->if_oerrors = 0;
	ifp->if_ipackets = 0;
	ifp->if_opackets = 0;
	ifp->if_ibytes   = 0;
	ifp->if_obytes   = 0;
	ifp->if_dlt = DLT_NULL;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	if_alloc_sadl(ifp);
#if NBPFILTER > 0
	bpfattach(ifp, DLT_NULL, sizeof(u_int32_t));
#endif
}

static int
tun_clone_destroy(struct ifnet *ifp)
{
	struct tun_softc *tp = (void *)ifp;
	int s, zombie = 0;

	s = splnet();
	simple_lock(&tun_softc_lock);
	simple_lock(&tp->tun_lock);
	LIST_REMOVE(tp, tun_list);
	if (tp->tun_flags & TUN_OPEN) {
		/* Hang on to storage until last close */
		zombie = 1;
		tp->tun_flags &= ~TUN_INITED;
		LIST_INSERT_HEAD(&tunz_softc_list, tp, tun_list);
	}
	simple_unlock(&tun_softc_lock);

	IF_PURGE(&ifp->if_snd);
	ifp->if_flags &= ~IFF_RUNNING;

	if (tp->tun_flags & TUN_RWAIT) {
		tp->tun_flags &= ~TUN_RWAIT;
		wakeup((caddr_t)tp);
	}
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_HUP, 0, NULL);

	selwakeup(&tp->tun_rsel);

	simple_unlock(&tp->tun_lock);
	splx(s);

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);

	if (!zombie)
		free(tp, M_DEVBUF);

	return (0);
}

/*
 * tunnel open - must be superuser & the device must be
 * configured in
 */
static int
tunopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ifnet	*ifp;
	struct tun_softc *tp;
	int	s, error;

	if ((error = kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    &l->l_acflag)) != 0)
		return (error);

	s = splnet();
	tp = tun_find_unit(dev);

	if (tp == NULL) {
		(void)tun_clone_create(&tun_cloner, minor(dev));
		tp = tun_find_unit(dev);
		if (tp == NULL) {
			error = ENXIO;
			goto out_nolock;
		}
	}

	if (tp->tun_flags & TUN_OPEN) {
		error = EBUSY;
		goto out;
	}

	ifp = &tp->tun_if;
	tp->tun_flags |= TUN_OPEN;
	TUNDEBUG("%s: open\n", ifp->if_xname);
out:
	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
	return (error);
}

/*
 * tunclose - close the device
 */
int
tunclose(dev_t dev, int flag, int mode,
    struct lwp *l)
{
	int	s;
	struct tun_softc *tp;
	struct ifnet	*ifp;

	s = splnet();
	if ((tp = tun_find_zunit(minor(dev))) != NULL) {
		/* interface was "destroyed" before the close */
		free(tp, M_DEVBUF);
		goto out_nolock;
	}

	if ((tp = tun_find_unit(dev)) == NULL)
		goto out_nolock;

	ifp = &tp->tun_if;

	tp->tun_flags &= ~TUN_OPEN;

	/*
	 * junk all pending output
	 */
	IFQ_PURGE(&ifp->if_snd);

	tp->tun_pgid = 0;
	selnotify(&tp->tun_rsel, 0);

	TUNDEBUG ("%s: closed\n", ifp->if_xname);
	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
	return (0);
}

/*
 * Process an ioctl request.
 */
static int
tun_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
 int error;
 int s;
 struct tun_softc *tp;

 error = 0;
 s = splnet();
 tp = (struct tun_softc *) ifp->if_softc;
 simple_lock(&tp->tun_lock);
 switch (cmd)
  { case SIOCSIFADDR:
#define ifa ((struct ifaddr *)data)
       switch (ifa->ifa_addr->sa_family)
	{
#ifdef INET
	     case AF_INET:
	     ifp->if_flags |= IFF_UP;
	     break;
#endif
#ifdef INET6
	  case AF_INET6:
	     ifp->if_flags |= IFF_UP;
	     break;
#endif
	  default:
	     error = EAFNOSUPPORT;
	     break;
	}
#undef ifa
       TUNDEBUG("%s: address set\n",&ifp->if_xname[0]);
       break;
    case SIOCSIFDSTADDR:
	{ const char *tag;
	  tag = "destination";
	  if (0)
	   {
    case SIOCSIFBRDADDR:
	     tag = "broadcast";
	   }
#define ifa ((struct ifaddr *)data)
	  switch (ifa->ifa_addr->sa_family)
	   {
#ifdef INET
	     case AF_INET:
		break;
#endif
#ifdef INET6
	     case AF_INET6:
		break;
#endif
	     default:
		error = EAFNOSUPPORT;
		break;
	   }
#undef ifa
	  TUNDEBUG("%s: %s address set\n",&ifp->if_xname[0],tag);
	}
       break;
    case SIOCSIFMTU:
#define ifr ((struct ifreq *)data)
       if ((ifr->ifr_mtu < 576) || (ifr->ifr_mtu > TUNMAXMTU))
	{ error = EINVAL;
	  break;
	}
#ifdef INET6
       if (ifr->ifr_mtu < IPV6_MMTU)
	{ struct ifaddr *ifa;
	  TAILQ_FOREACH(ifa,&ifp->if_addrlist,ifa_list)
	   { switch (((struct sockaddr *)ifa->ifa_addr)->sa_family)
	      { case AF_INET6:
		   error = EINVAL;
		   goto done; /* wish we had lcs! */
	      }
	   }
	}
#endif
       TUNDEBUG("%s: interface MTU set\n",&ifp->if_xname[0]);
       ifp->if_mtu = ifr->ifr_mtu;
#undef ifr
       break;
    case SIOCGIFMTU:
#define ifr ((struct ifreq *)data)
       ifr->ifr_mtu = ifp->if_mtu;
#undef ifr
       break;
    case SIOCADDMULTI:
    case SIOCDELMULTI:
#define ifr ((struct ifreq *)data)
       if (ifr == 0)
	{ error = EAFNOSUPPORT; /* XXX */
	  break;
	}
       switch (ifr->ifr_addr.sa_family)
	{
#ifdef INET
	  case AF_INET:
	     break;
#endif
#ifdef INET6
	  case AF_INET6:
	     break;
#endif
	  default:
	     error = EAFNOSUPPORT;
	     break;
	}
       break;
    case SIOCSIFFLAGS:
       break;
    default:
       error = EINVAL;
       break;
  }
#ifdef INET6 /* used only if INET6 above */
done:;
#endif
 simple_unlock(&tp->tun_lock);
 splx(s);
 return(error);
}

/*
 * tun_output - queue packets from higher level ready to put out.
 */
static int
tun_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst,
    struct rtentry *rt)
{
	struct tun_softc *tp = ifp->if_softc;
	int		s;
	int		error;
#if defined(INET) || defined(INET6)
	int		mlen;
	uint32_t	*af;
	int		non_inet_ok;
#endif

	ALTQ_DECL(struct altq_pktattr pktattr;)

	s = splnet();
	simple_lock(&tp->tun_lock);
	TUNDEBUG ("%s: tun_output\n", ifp->if_xname);

	switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
			break;
#endif
#ifdef INET6
		case AF_INET6:
			break;
#endif
		default:
			printf("%s: af%d not supported\n",&ifp->if_xname[0],dst->sa_family);
			m_freem(m0);
			error = EAFNOSUPPORT;
			goto out;
	}
	if (! (tp->tun_flags & TUN_OPEN)) {
		error = ENETDOWN;
		goto out;
	}

	/*
	 * if the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m0, dst->sa_family, &pktattr);

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap_af(ifp->if_bpf, dst->sa_family, m0);
#endif

	switch (dst->sa_family) {
#ifdef INET6
	case AF_INET6:
#endif
#ifdef INET
	case AF_INET:
#endif
#if defined(INET) || defined(INET6)
		non_inet_ok = 0;
		if (tp->tun_flags & TUN_PREPADDR) {
			/* Simple link-layer header */
			M_PREPEND(m0, dst->sa_len, M_DONTWAIT);
			if (m0 == NULL) {
				IF_DROP(&ifp->if_snd);
				error = ENOBUFS;
				goto out;
			}
			bcopy(dst, mtod(m0, char *), dst->sa_len);
			non_inet_ok = 1;
		}
		if (tp->tun_flags & TUN_IFHEAD) {
			/* Prepend the address family */
			M_PREPEND(m0, sizeof(*af), M_DONTWAIT);
			if (m0 == NULL) {
				IF_DROP(&ifp->if_snd);
				error = ENOBUFS;
				goto out;
			}
			af = mtod(m0,uint32_t *);
			*af = htonl(dst->sa_family);
			non_inet_ok = 1;
		}
		if (! non_inet_ok) {
#ifdef INET
			if (dst->sa_family != AF_INET)
#endif
			{
				m_freem(m0);
				error = EAFNOSUPPORT;
				goto out;
			}
		}
		/* FALLTHROUGH */
	case AF_UNSPEC:
		IFQ_ENQUEUE(&ifp->if_snd, m0, &pktattr, error);
		if (error) {
			ifp->if_collisions++;
			error = EAFNOSUPPORT;
			goto out;
		}
		mlen = m0->m_pkthdr.len;
		ifp->if_opackets++;
		ifp->if_obytes += mlen;
		break;
#endif
	default:
		m_freem(m0);
		error = EAFNOSUPPORT;
		goto out;
	}

	if (tp->tun_flags & TUN_RWAIT) {
		tp->tun_flags &= ~TUN_RWAIT;
		wakeup((caddr_t)tp);
	}
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_IN, POLLIN|POLLRDNORM,
		    NULL);

	selnotify(&tp->tun_rsel, 0);
out:
	simple_unlock(&tp->tun_lock);
	splx(s);
	return (0);
}

/*
 * the cdevsw interface is now pretty minimal.
 */
int
tunioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct tun_softc *tp;
	int s, error = 0;

	s = splnet();
	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL) {
		error = ENXIO;
		goto out_nolock;
	}

	switch (cmd) {
	case TUNSDEBUG:
		tundebug = *(int *)data;
		break;

	case TUNGDEBUG:
		*(int *)data = tundebug;
		break;

	case TUNSIFMODE:
		switch (*(int *)data) {
		case IFF_POINTOPOINT:
		case IFF_BROADCAST:
			if ( ((tp->tun_if.if_flags ^ *(int *)data) &
				(IFF_BROADCAST|IFF_POINTOPOINT)) == 0)
				break;
			if (tp->tun_if.if_flags & IFF_UP) {
				error = EBUSY;
				goto out;
			}
			tp->tun_if.if_flags &=
				~(IFF_BROADCAST|IFF_POINTOPOINT);
			tp->tun_if.if_flags |= *(int *)data;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		break;

	case TUNSLMODE:
		if (*(int *)data) {
			tp->tun_flags |= TUN_PREPADDR;
			tp->tun_flags &= ~TUN_IFHEAD;
		} else
			tp->tun_flags &= ~TUN_PREPADDR;
		break;

	case TUNSIFHEAD:
		if (*(int *)data) {
			tp->tun_flags |= TUN_IFHEAD;
			tp->tun_flags &= ~TUN_PREPADDR;
		} else
			tp->tun_flags &= ~TUN_IFHEAD;
		break;

	case TUNGIFHEAD:
		*(int *)data = (tp->tun_flags & TUN_IFHEAD);
		break;

	case FIONBIO:
		if (*(int *)data)
			tp->tun_flags |= TUN_NBIO;
		else
			tp->tun_flags &= ~TUN_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			tp->tun_flags |= TUN_ASYNC;
		else
			tp->tun_flags &= ~TUN_ASYNC;
		break;

	case FIONREAD:
		if (tp->tun_if.if_snd.ifq_head)
			*(int *)data = tp->tun_if.if_snd.ifq_head->m_pkthdr.len;
		else
			*(int *)data = 0;
		break;

	case TIOCSPGRP:
	case FIOSETOWN:
		error = fsetown(l->l_proc, &tp->tun_pgid, cmd, data);
		break;

	case TIOCGPGRP:
	case FIOGETOWN:
		error = fgetown(l->l_proc, tp->tun_pgid, cmd, data);
		break;

	default:
		error = ENOTTY;
	}

out:
	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
	return (error);
}

/*
 * The cdevsw read interface - reads a packet at a time, or at
 * least as much of a packet as can be read.
 */
int
tunread(dev_t dev, struct uio *uio, int ioflag)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	struct mbuf	*m, *m0;
	int		error = 0, len, s, index;

	s = splnet();
	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL) {
		error = ENXIO;
		goto out_nolock;
	}

	index = tp->tun_if.if_index;
	ifp = &tp->tun_if;

	TUNDEBUG ("%s: read\n", ifp->if_xname);
	if ((tp->tun_flags & TUN_READY) != TUN_READY) {
		TUNDEBUG ("%s: not ready 0%o\n", ifp->if_xname, tp->tun_flags);
		error = EHOSTDOWN;
		goto out;
	}

	tp->tun_flags &= ~TUN_RWAIT;

	do {
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == 0) {
			if (tp->tun_flags & TUN_NBIO) {
				error = EWOULDBLOCK;
				goto out;
			}
			tp->tun_flags |= TUN_RWAIT;
			if (ltsleep((caddr_t)tp, PZERO|PCATCH|PNORELOCK,
					"tunread", 0, &tp->tun_lock) != 0) {
				error = EINTR;
				goto out_nolock;
			} else {
				/*
				 * Maybe the interface was destroyed while
				 * we were sleeping, so let's ensure that
				 * we're looking at the same (valid) tun
				 * interface before looping.
				 */
				tp = tun_find_unit(dev);
				if (tp == NULL) {
					error = ENXIO;
					goto out_nolock;
				}
				if (tp->tun_if.if_index != index) {
					error = ENXIO;
					goto out;
				}
			}
		}
	} while (m0 == 0);

	simple_unlock(&tp->tun_lock);
	splx(s);

	/* Copy the mbuf chain */
	while (m0 && uio->uio_resid > 0 && error == 0) {
		len = min(uio->uio_resid, m0->m_len);
		if (len != 0)
			error = uiomove(mtod(m0, caddr_t), len, uio);
		MFREE(m0, m);
		m0 = m;
	}

	if (m0) {
		TUNDEBUG("Dropping mbuf\n");
		m_freem(m0);
	}
	if (error)
		ifp->if_ierrors++;

	return (error);

out:
	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
	return (error);
}

/*
 * the cdevsw write interface - an atomic write is a packet - or else!
 */
int
tunwrite(dev_t dev, struct uio *uio, int ioflag)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	struct mbuf	*top, **mp, *m;
	struct ifqueue	*ifq;
	struct sockaddr_storage	dst;
	int		isr, error = 0, s, tlen, mlen;
	uint32_t	family;

	s = splnet();
	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL) {
		error = ENXIO;
		goto out_nolock;
	}

	/* Unlock until we've got the data */
	simple_unlock(&tp->tun_lock);
	splx(s);

	ifp = &tp->tun_if;

	TUNDEBUG("%s: tunwrite\n", ifp->if_xname);

 if (tp->tun_flags & TUN_PREPADDR)
#define PREFN (offsetof(struct sockaddr_storage,ss_len)+sizeof(dst.ss_len))
  { if (uio->uio_resid < PREFN)
     { error = EIO;
       goto out0;
     }
    error = uiomove(&dst,PREFN,uio);
    if (error) goto out0;
    if ( (dst.ss_len <= sizeof(dst.ss_len)) ||
	 (dst.ss_len > sizeof(dst)) )
     { error = EINVAL;
       goto out0;
     }
    error = uiomove(1+&dst.ss_len,dst.ss_len-PREFN,uio);
    if (error) goto out0;
	} else if (tp->tun_flags & TUN_IFHEAD) {
		if (uio->uio_resid < sizeof(family)){
			error = EIO;
			goto out0;
		}
		error = uiomove((caddr_t)&family, sizeof(family), uio);
		dst.ss_family = ntohl(family);
	}
 else
  { dst.ss_len = sizeof(struct sockaddr_in);
    dst.ss_family = AF_INET;
    /* Error out below ifndef INET */
  }

	if (uio->uio_resid > TUNMAXMTU) {
		TUNDEBUG("%s: len=%lu!\n", ifp->if_xname,
		    (unsigned long)uio->uio_resid);
		error = EIO;
		goto out0;
	}

	switch (dst.ss_family) {
#ifdef INET
	case AF_INET:
		ifq = &ipintrq;
		isr = NETISR_IP;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ifq = &ip6intrq;
		isr = NETISR_IPV6;
		break;
#endif
	default:
		ifp->if_ierrors ++;
		error = EAFNOSUPPORT;
		goto out0;
	}

	tlen = uio->uio_resid;

	/* get a header mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		error = ENOBUFS;
		goto out0;
	}
	mlen = MHLEN;

	top = NULL;
	mp = &top;
	while (error == 0 && uio->uio_resid > 0) {
		m->m_len = min(mlen, uio->uio_resid);
		error = uiomove(mtod(m, caddr_t), m->m_len, uio);
		*mp = m;
		mp = &m->m_next;
		if (error == 0 && uio->uio_resid > 0) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				error = ENOBUFS;
				break;
			}
			mlen = MLEN;
		}
	}
	if (error) {
		if (top != NULL)
			m_freem (top);
		ifp->if_ierrors++;
		goto out0;
	}

	top->m_pkthdr.len = tlen;
	top->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap_af(ifp->if_bpf, dst.ss_family, top);
#endif

	s = splnet();
	simple_lock(&tp->tun_lock);
	if ((tp->tun_flags & TUN_INITED) == 0) {
		/* Interface was destroyed */
		error = ENXIO;
		goto out;
	}
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		ifp->if_collisions++;
		m_freem(top);
		error = ENOBUFS;
		goto out;
	}

	IF_ENQUEUE(ifq, top);
	ifp->if_ipackets++;
	ifp->if_ibytes += tlen;
	schednetisr(isr);
out:
	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
out0:
	return (error);
}

#ifdef ALTQ
/*
 * Start packet transmission on the interface.
 * when the interface queue is rate-limited by ALTQ or TBR,
 * if_start is needed to drain packets from the queue in order
 * to notify readers when outgoing packets become ready.
 *
 * Should be called at splnet.
 */
static void
tunstart(struct ifnet *ifp)
{
	struct tun_softc *tp = ifp->if_softc;

	if (!ALTQ_IS_ENABLED(&ifp->if_snd) && !TBR_IS_ENABLED(&ifp->if_snd))
		return;

	simple_lock(&tp->tun_lock);
	if (!IF_IS_EMPTY(&ifp->if_snd)) {
		if (tp->tun_flags & TUN_RWAIT) {
			tp->tun_flags &= ~TUN_RWAIT;
			wakeup((caddr_t)tp);
		}
		if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
			fownsignal(tp->tun_pgid, SIGIO, POLL_OUT,
				POLLOUT|POLLWRNORM, NULL);

		selwakeup(&tp->tun_rsel);
	}
	simple_unlock(&tp->tun_lock);
}
#endif /* ALTQ */
/*
 * tunpoll - the poll interface, this is only useful on reads
 * really. The write detect always returns true, write never blocks
 * anyway, it either accepts the packet or drops it.
 */
int
tunpoll(dev_t dev, int events, struct lwp *l)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	int		s, revents = 0;

	s = splnet();
	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL)
		goto out_nolock;

	ifp = &tp->tun_if;

	TUNDEBUG("%s: tunpoll\n", ifp->if_xname);

	if (events & (POLLIN | POLLRDNORM)) {
		if (!IFQ_IS_EMPTY(&ifp->if_snd)) {
			TUNDEBUG("%s: tunpoll q=%d\n", ifp->if_xname,
			    ifp->if_snd.ifq_len);
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			TUNDEBUG("%s: tunpoll waiting\n", ifp->if_xname);
			selrecord(l, &tp->tun_rsel);
		}
	}

	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);

	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
	return (revents);
}

static void
filt_tunrdetach(struct knote *kn)
{
	struct tun_softc *tp = kn->kn_hook;
	int s;

	s = splnet();
	SLIST_REMOVE(&tp->tun_rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_tunread(struct knote *kn, long hint)
{
	struct tun_softc *tp = kn->kn_hook;
	struct ifnet *ifp = &tp->tun_if;
	struct mbuf *m;
	int s;

	s = splnet();
	IF_POLL(&ifp->if_snd, m);
	if (m == NULL) {
		splx(s);
		return (0);
	}

	for (kn->kn_data = 0; m != NULL; m = m->m_next)
		kn->kn_data += m->m_len;

	splx(s);
	return (1);
}

static const struct filterops tunread_filtops =
	{ 1, NULL, filt_tunrdetach, filt_tunread };

static const struct filterops tun_seltrue_filtops =
	{ 1, NULL, filt_tunrdetach, filt_seltrue };

int
tunkqfilter(dev_t dev, struct knote *kn)
{
	struct tun_softc *tp;
	struct klist *klist;
	int rv = 0, s;

	s = splnet();
	tp = tun_find_unit(dev);
	if (tp == NULL)
		goto out_nolock;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &tp->tun_rsel.sel_klist;
		kn->kn_fop = &tunread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &tp->tun_rsel.sel_klist;
		kn->kn_fop = &tun_seltrue_filtops;
		break;

	default:
		rv = 1;
		goto out;
	}

	kn->kn_hook = tp;

	SLIST_INSERT_HEAD(klist, kn, kn_selnext);

out:
	simple_unlock(&tp->tun_lock);
out_nolock:
	splx(s);
	return (rv);
}
