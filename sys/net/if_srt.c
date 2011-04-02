#include "opt_inet.h"
#include "opt_pfil_hooks.h"

#if !defined(INET) && !defined(INET6)
#error "srt with neither INET nor INET6"
#endif

#ifndef SRT_MAXUNIT
#define SRT_MAXUNIT 255
#endif

#include <net/if.h>
#include <net/bpf.h>
#include <sys/lock.h>
#include <sys/mbuf.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/malloc.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <net/if_types.h>
#include <machine/intr.h>
#include <machine/stdarg.h>

#include "if_srt.h"
#include "bpfilter.h"
#include "opt_altq_enabled.h"

#ifdef PFIL_HOOKS
/* XXX, but this is how ip_output.c does it... */
extern struct pfil_head inet_pfil_hook;
#endif

typedef struct softc SOFTC;
typedef struct srt_rt RT;

struct softc {
  struct ifnet intf;
  struct simplelock lock;
  int unit;
  int nrt;
  RT **rts;
  unsigned int flags; /* SSF_* values from if_srt.h */
#define SSF_UCHG (SSF_MTULOCK) /* userland-changeable bits */
  unsigned int kflags; /* internal */
#define SKF_CDEVOPEN 0x00000001
  } ;

/* Lock nesting:
   softcv_lock
     SOFTC locks
       gblf_lock
   That is, for example, never take softcv_lock when holding either a
   SOFTC's lock or gblf_lock.
 */
static struct simplelock softcv_lock;
static SOFTC *softcv[SRT_MAXUNIT+1];
static struct simplelock gblf_lock;
static unsigned int global_flags;

#define SRT_DEFMTU 1500
#define SRT_MAXMTU 9000

static unsigned int ipv4_masks[33]
 = { 0x00000000, /* /0 */
     0x80000000, 0xc0000000, 0xe0000000, 0xf0000000, /* /1 - /4 */
     0xf8000000, 0xfc000000, 0xfe000000, 0xff000000, /* /5 - /8 */
     0xff800000, 0xffc00000, 0xffe00000, 0xfff00000, /* /9 - /12 */
     0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000, /* /13 - /16 */
     0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000, /* /17 - /20 */
     0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00, /* /21 - /24 */
     0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0, /* /25 - /28 */
     0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff  /* /29 - /32 */ };

static void update_mtu(SOFTC *sc)
{
 int mtu;
 int i;
 RT *r;

 if (sc->flags & SSF_MTULOCK) return;
 mtu = 65535;
 for (i=sc->nrt-1;i>=0;i--)
  { r = sc->rts[i];
    if (r->u.dstifp->if_mtu < mtu) mtu = r->u.dstifp->if_mtu;
  }
 sc->intf.if_mtu = mtu;
}

static RT *find_rt(SOFTC *sc, int af, ...)
{
 int i;
 RT *r;
 struct in_addr ia;
 struct in6_addr ia6;
 va_list ap;
 unsigned int cmp4;

 /* gcc's -Wuninitialized thinks these may be used unitialized.
    It's wrong, but how to explain that to it?
    So we waste a few cycles. :( */
 ia.s_addr = 0;
 ia6.s6_addr[0] = 0;
 cmp4 = 0;
 /* End incorrect -Wuninitialized workaround */
 va_start(ap,af);
 switch (af)
  { case AF_INET:
       ia = va_arg(ap,struct in_addr);
       cmp4 = ntohl(ia.s_addr);
       break;
    case AF_INET6:
       ia6 = va_arg(ap,struct in6_addr);
       break;
    default:
       panic("if_srt find_rt: impossible address family");
       break;
  }
 va_end(ap);
 for (i=0;i<sc->nrt;i++)
  { r = sc->rts[i];
    if (r->af != af) continue;
    switch (af)
     { case AF_INET:
	  if ((cmp4 & ipv4_masks[r->srcmask]) == r->srcmatch.v4.s_addr) return(r);
	  break;
       case AF_INET6:
	  if ((r->srcmask >= 8) && bcmp(&ia6,&r->srcmatch.v6,r->srcmask/8)) continue;
	  if ( (r->srcmask % 8) &&
	       ( ( ia6.s6_addr[r->srcmask/8] ^
		   r->srcmatch.v6.s6_addr[r->srcmask/8] ) &
		 0xff & (0xff00 >> (r->srcmask%8)) ) ) continue;
	  return(r);
	  break;
       default:
	  panic("if_srt find_rt: impossible address family 2");
	  break;
     }
  }
 return(0);
}

static int srt_if_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
#define ifa ((struct ifaddr *)data)
#define ifr ((struct ifreq *)data)
 int err;
 int s;
 SOFTC *sc;

 err = 0;
 s = splnet();
 sc = ifp->if_softc;
 simple_lock(&sc->lock);
 switch (cmd)
  { case SIOCSIFADDR:
       switch (ifa->ifa_addr->sa_family)
	{
#ifdef INET
	  case AF_INET:
#endif
#ifdef INET6
	  case AF_INET6:
#endif
	     break;
	  default:
	     err = EAFNOSUPPORT;
	     break;
	}
       break;
    case SIOCSIFDSTADDR:
       switch (ifa->ifa_addr->sa_family)
	{
#ifdef INET
	  case AF_INET:
#endif
#ifdef INET6
	  case AF_INET6:
#endif
	     break;
/* XXX we should support INET6 too */
	  default:
	     err = EAFNOSUPPORT;
	     break;
	}
       break;
    case SIOCSIFMTU:
       if ((ifr->ifr_mtu < 576) || (ifr->ifr_mtu > SRT_MAXMTU))
	{ err = EINVAL;
	  break;
	}
       /* XXX should do tun-style INET6 MTU check */
       ifp->if_mtu = ifr->ifr_mtu;
       break;
    case SIOCGIFMTU:
       ifr->ifr_mtu = ifp->if_mtu;
       break;
    case SIOCADDMULTI:
    case SIOCDELMULTI:
       if (! ifr)
	{ err = EAFNOSUPPORT; /* XXX */
	  break;
	}
       switch (ifr->ifr_addr.sa_family)
	{
#ifdef INET
	  case AF_INET:
#endif
#ifdef INET6
	  case AF_INET6:
#endif
	     break;
	  default:
	     err = EAFNOSUPPORT;
	     break;
	}
       break;
    case SIOCSIFFLAGS:
       break;
    default:
       err = EINVAL;
       break;
  }
#undef ifa
#undef ifr
 simple_unlock(&sc->lock);
 splx(s);
 return(err);
}

static int srt_if_output(
	struct ifnet *ifp,
	struct mbuf *m,
	struct sockaddr *dst,
	struct rtentry *rt)
{
 SOFTC *sc;
 RT *r;
 int rv;

 sc = ifp->if_softc;
 if (! (ifp->if_flags & IFF_UP))
  { m_freem(m);
    return(ENETDOWN);
  }
 simple_lock(&sc->lock);
 switch (dst->sa_family)
  {
#ifdef INET
    case AF_INET:
	{ struct ip *ip;
	  ip = mtod(m,struct ip *);
	  r = find_rt(sc,AF_INET,ip->ip_src);
	}
       break;
#endif
#ifdef INET6
    case AF_INET6:
	{ struct ip6_hdr *ip;
	  ip = mtod(m,struct ip6_hdr *);
	  r = find_rt(sc,AF_INET6,ip->ip6_src);
	}
       break;
#endif
    default:
       simple_unlock(&sc->lock);
       IF_DROP(&ifp->if_snd);
       m_freem(m);
       return(EAFNOSUPPORT);
       break;
  }
 ifp->if_opackets ++;
 if (! r)
  { ifp->if_oerrors ++;
    simple_unlock(&sc->lock);
    m_freem(m);
    return(0);
  }
 if (! (m->m_flags & M_PKTHDR))
  { simple_unlock(&sc->lock);
    printf("srt_if_output no PKTHDR\n");
    m_freem(m);
    return(0);
  }
 ifp->if_obytes += m->m_pkthdr.len;
 if (! (r->u.dstifp->if_flags & IFF_UP))
  { simple_unlock(&sc->lock);
    m_freem(m);
    return(0); /* XXX ENETDOWN? */
  }
#ifdef PFIL_HOOKS
 rv = pfil_run_hooks(&inet_pfil_hook,&m,r->u.dstifp,PFIL_OUT);
 if (rv)
  { simple_unlock(&sc->lock);
    return(rv);
  }
#endif
 /*
  * We have to hold sc->lock across the underlying interface's output
  *  call, because it uses r->dst.sa.  This means we will deadlock if
  *  someone configures a loop and then sends a packet to it - but
  *  that's not so bad; the alternative is to recurse until we run out
  *  of stack and panic.
  */
 /* XXX is 0 ok as the last arg here? */
 rv = (*r->u.dstifp->if_output)(r->u.dstifp,m,&r->dst.sa,0);
 simple_unlock(&sc->lock);
 return(rv);
}

#ifdef ALTQ
/* I'm not sure how relevant ALTQ is in view of the way srt works... */
static void srt_if_start(struct ifnet *ifp)
{
 printf("srt if_start called\n");
}
#endif

static int srt_clone_create(struct if_clone *ifc, int unit)
{
 SOFTC *sc;

 if ((unit < 0) || (unit > SRT_MAXUNIT)) return(ENXIO);
 simple_lock(&softcv_lock);
 if (softcv[unit]) return(EBUSY);
 simple_unlock(&softcv_lock);
 sc = malloc(sizeof(SOFTC),M_DEVBUF,M_WAITOK);
 simple_lock(&softcv_lock);
 if (softcv[unit])
  { simple_unlock(&softcv_lock);
    free(sc,M_DEVBUF);
    return(EBUSY);
  }
 sc->unit = unit;
 sc->nrt = 0;
 sc->rts = 0;
 sc->flags = 0;
 sc->kflags = 0;
 bzero(&sc->intf,sizeof(sc->intf)); /* XXX interface botch */
 snprintf(&sc->intf.if_xname[0],sizeof(sc->intf.if_xname),"%s%d",ifc->ifc_name,sc->unit);
 sc->intf.if_softc = sc;
 sc->intf.if_mtu = 65535;
 sc->intf.if_ioctl = &srt_if_ioctl;
 sc->intf.if_output = &srt_if_output;
#ifdef ALTQ
 sc->ifnet.if_start = &srt_if_start;
#endif
 sc->intf.if_flags = IFF_POINTOPOINT;
 sc->intf.if_type = IFT_OTHER; /* XXX is IFT_OTHER right? */
  { extern int ifqmaxlen; /* XXX belongs in a .h file */
    sc->intf.if_snd.ifq_maxlen = ifqmaxlen;
  }
 sc->intf.if_collisions = 0;
 sc->intf.if_ierrors = 0;
 sc->intf.if_oerrors = 0;
 sc->intf.if_ipackets = 0;
 sc->intf.if_opackets = 0;
 sc->intf.if_ibytes = 0;
 sc->intf.if_obytes = 0;
 sc->intf.if_dlt = DLT_NULL;
 IFQ_SET_READY(&sc->intf.if_snd);
 if_attach(&sc->intf);
 if_alloc_sadl(&sc->intf);
#if NBPFILTER > 0
 bpfattach(&sc->intf,DLT_NULL,sizeof(u_int32_t));
#endif
 simple_lock_init(&sc->lock);
 softcv[unit] = sc;
 simple_unlock(&softcv_lock);
 return(0);
}

static int srt_clone_destroy(struct ifnet *ifp)
{
 SOFTC *sc;
 int s;

 simple_lock(&softcv_lock);
 sc = ifp->if_softc;
 if ((sc->unit < 0) || (sc->unit > SRT_MAXUNIT))
  { panic("srt_clone_destroy: impossible unit %d\n",sc->unit);
  }
 if (sc != softcv[sc->unit])
  { panic("srt_clone_destroy: bad backpointer ([%d]=%p not %p)\n",
			sc->unit,(void *)softcv[sc->unit],(void *)sc);
  }
 simple_lock(&sc->lock);
 if ((ifp->if_flags & IFF_UP) || (sc->kflags & SKF_CDEVOPEN))
  { simple_unlock(&sc->lock);
    simple_unlock(&softcv_lock);
    return(EBUSY);
  }
 softcv[sc->unit] = 0;
 simple_unlock(&softcv_lock);
 s = splnet();
 IF_PURGE(&ifp->if_snd);
 ifp->if_flags &= ~IFF_RUNNING;
 splx(s);
#if NBPFILTER > 0
 bpfdetach(ifp);
#endif
 if_detach(ifp);
 simple_unlock(&sc->lock);
 free(sc,M_DEVBUF);
 return(0);
}

struct if_clone srt_cloner
 = IF_CLONE_INITIALIZER("srt",&srt_clone_create,&srt_clone_destroy);

void srtattach(int);
void srtattach(int n __attribute__((__unused__)))
{
 int i;

 for (i=SRT_MAXUNIT;i>=0;i--) softcv[i] = 0;
 global_flags = 0;
 simple_lock_init(&softcv_lock);
 simple_lock_init(&gblf_lock);
 if_clone_attach(&srt_cloner);
}

/* These are unnecessary for declaration; they are here to provoke errors
   if the definitions below get out of sync with the macros. */
static dev_type_open(srt_ctl_open);
static dev_type_close(srt_ctl_close);
static dev_type_read(srt_ctl_read);
static dev_type_write(srt_ctl_write);
static dev_type_ioctl(srt_ctl_ioctl);

static int srt_ctl_open(dev_t dev, int flag, int mode, struct lwp *l)
{
 SOFTC *sc;
 int unit;

 unit = minor(dev);
 simple_lock(&softcv_lock);
 sc = softcv[unit];
 if (! sc)
  { simple_unlock(&softcv_lock);
    return(ENXIO);
  }
 simple_lock(&sc->lock);
 sc->kflags |= SKF_CDEVOPEN;
 simple_unlock(&sc->lock);
 simple_unlock(&softcv_lock);
 return(0);
}

static int srt_ctl_close(
	dev_t dev __attribute__((__unused__)),
	int flag __attribute__((__unused__)),
	int mode __attribute__((__unused__)),
	struct lwp *l __attribute__((__unused__)) )
{
 int unit;
 SOFTC *sc;

 unit = minor(dev);
 if ((unit < 0) || (unit > SRT_MAXUNIT)) return(ENXIO);
 simple_lock(&softcv_lock);
 sc = softcv[unit];
 if (! sc)
  { simple_unlock(&softcv_lock);
    return(ENXIO);
  }
 simple_lock(&sc->lock);
 sc->kflags &= ~SKF_CDEVOPEN;
 simple_unlock(&sc->lock);
 simple_unlock(&softcv_lock);
 return(0);
}

static int srt_ctl_read(dev_t dev __attribute__((__unused__)), struct uio *uio __attribute__((__unused__)), int flag __attribute__((__unused__)))
{
 return(EIO);
}

static int srt_ctl_write(dev_t dev __attribute__((__unused__)), struct uio *uio __attribute__((__unused__)), int flag __attribute__((__unused__)))
{
 return(EIO);
}

static int srt_ctl_ioctl_core(SOFTC *sc, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
 RT *dr;
 RT *scr;
 struct ifnet *ifp;
 char nbuf[IFNAMSIZ];

 switch (cmd)
  { case SRT_GETNRT:
       if (! (flag & FREAD)) return(EBADF);
       *(unsigned int *)data = sc->nrt;
       return(0);
       break;
    case SRT_GETRT:
       if (! (flag & FREAD)) return(EBADF);
       dr = (RT *) data;
       if (dr->inx >= sc->nrt) return(EDOM);
       scr = sc->rts[dr->inx];
       dr->af = scr->af;
       dr->srcmatch = scr->srcmatch;
       dr->srcmask = scr->srcmask;
       switch (scr->af)
	{
#ifdef INET
	  case AF_INET:
	     dr->srcmatch.v4.s_addr = htonl(scr->srcmatch.v4.s_addr);
	     break;
#endif
	}
       strncpy(&dr->u.dstifn[0],&scr->u.dstifp->if_xname[0],IFNAMSIZ);
       memcpy(&dr->dst,&scr->dst,scr->dst.sa.sa_len);
       return(0);
       break;
    case SRT_SETRT:
       if (! (flag & FWRITE)) return(EBADF);
       dr = (RT *) data;
       if (dr->inx > sc->nrt) return(EDOM);
       strncpy(&nbuf[0],&dr->u.dstifn[0],IFNAMSIZ);
       nbuf[IFNAMSIZ-1] = '\0';
       if (dr->dst.sa.sa_family != dr->af) return(EIO);
       switch (dr->af)
	{
#ifdef INET
	  case AF_INET:
	     if (dr->dst.sa.sa_len != sizeof(dr->dst.sin)) return(EIO);
	     if (dr->srcmask > 32) return(EIO);
	     break;
#endif
#ifdef INET6
	  case AF_INET6:
	     if (dr->dst.sa.sa_len != sizeof(dr->dst.sin6)) return(EIO);
	     if (dr->srcmask > 128) return(EIO);
	     break;
#endif
	  default:
	     return(EAFNOSUPPORT);
	     break;
	}
       ifp = ifunit(&nbuf[0]);
       if (! ifp) return(ENXIO); /* needs translation */
       if (dr->inx == sc->nrt)
	{ RT **tmp;
	  tmp = malloc((sc->nrt+1)*sizeof(*tmp),M_DEVBUF,M_WAITOK);
	  if (! tmp) return(ENOBUFS);
	  tmp[sc->nrt] = 0;
	  if (sc->nrt > 0)
	   { memcpy(tmp,sc->rts,sc->nrt*sizeof(*tmp));
	     free(sc->rts,M_DEVBUF);
	   }
	  sc->rts = tmp;
	  sc->nrt ++;
	}
       scr = sc->rts[dr->inx];
       if (scr == 0)
	{ scr = malloc(sizeof(RT),M_DEVBUF,M_WAITOK);
	  if (scr == 0) return(ENOBUFS);
	  scr->inx = dr->inx;
	  scr->af = AF_UNSPEC;
	  sc->rts[dr->inx] = scr;
	}
       scr->af = dr->af;
       scr->srcmatch = dr->srcmatch;
       switch (scr->af)
	{
#ifdef INET
	  case AF_INET:
	     scr->srcmatch.v4.s_addr = ntohl(scr->srcmatch.v4.s_addr);
	     break;
#endif
	}
       scr->srcmask = dr->srcmask;
       scr->u.dstifp = ifp;
       memcpy(&scr->dst,&dr->dst,dr->dst.sa.sa_len);
       update_mtu(sc);
       return(0);
       break;
    case SRT_DELRT:
	{ unsigned int i;
	  if (! (flag & FWRITE)) return(EBADF);
	  i = *(unsigned int *)data;
	  if (i >= sc->nrt) return(EDOM);
	  scr = sc->rts[i];
	  sc->rts[i] = 0;
	  free(scr,M_DEVBUF);
	  sc->nrt --;
	  if (i < sc->nrt) memcpy(sc->rts+i,sc->rts+i+1,(sc->nrt-i)*sizeof(*sc->rts));
	  if (sc->nrt == 0)
	   { free(sc->rts,M_DEVBUF);
	     sc->rts = 0;
	     sc->intf.if_flags &= ~IFF_UP;
	   }
	}
       update_mtu(sc);
       return(0);
       break;
    case SRT_SFLAGS:
	{ unsigned int f;
	  if (! (flag & FWRITE)) return(EBADF);
	  f = *(unsigned int *)data & SSF_UCHG;
	  simple_lock(&gblf_lock);
	  global_flags = (global_flags & ~SSF_UCHG) | (f & SSF_GLOBAL);
	  simple_unlock(&gblf_lock);
	  sc->flags = (sc->flags & ~SSF_UCHG) | (f & ~SSF_GLOBAL);
	}
       return(0);
       break;
    case SRT_GFLAGS:
       if (! (flag & FREAD)) return(EBADF);
       simple_lock(&gblf_lock);
       *(unsigned int *)data = sc->flags | global_flags;
       simple_unlock(&gblf_lock);
       return(0);
       break;
    case SRT_SGFLAGS:
	{ unsigned int o;
	  unsigned int n;
	  if ((flag & (FWRITE|FREAD)) != (FWRITE|FREAD)) return(EBADF);
	  simple_lock(&gblf_lock);
	  o = sc->flags | global_flags;
	  n = *(unsigned int *)data & SSF_UCHG;
	  global_flags = (global_flags & ~SSF_UCHG) | (n & SSF_GLOBAL);
	  simple_unlock(&gblf_lock);
	  sc->flags = (sc->flags & ~SSF_UCHG) | (n & ~SSF_GLOBAL);
	  *(unsigned int *)data = o;
	}
       return(0);
       break;
    case SRT_DEBUG:
       return(0);
       break;
  }
 return(ENOTTY);
}

static int srt_ctl_ioctl(
	dev_t dev,
	u_long cmd,
	caddr_t data,
	int flag,
	struct lwp *l )
{
 SOFTC *sc;
 int err;
 int unit;

 unit = minor(dev);
 if ((unit < 0) || (unit > SRT_MAXUNIT)) return(ENXIO);
 simple_lock(&softcv_lock);
 sc = softcv[unit];
 if (! sc)
  { simple_unlock(&softcv_lock);
    return(ENXIO);
  }
 simple_lock(&sc->lock);
 simple_unlock(&softcv_lock);
 err = srt_ctl_ioctl_core(sc,cmd,data,flag,l);
 simple_unlock(&sc->lock);
 return(err);
}

const struct cdevsw srt_cdevsw
 = { &srt_ctl_open, &srt_ctl_close, &srt_ctl_read, &srt_ctl_write,
     &srt_ctl_ioctl, nostop, notty, nullpoll, nommap, nokqfilter, D_OTHER };
