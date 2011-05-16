#include "synaptics.h"

#if NSYNAPTICS > 0

#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <dev/pckbport/pmsvar.h>
#include <dev/pckbport/synapticsdev.h>

typedef struct softc SOFTC;

#define SYNRINGSIZE 256
#define RING_ADV(x) (((x) < 1) ? SYNRINGSIZE-1 : ((x)-1))

struct softc {
  int unit;
  unsigned int flags;
#define SYNF_EXISTS  0x00000001
#define SYNF_COPY    0x00000002
#define SYNF_STEAL   0x00000004
#define SYNF_OPEN (SYNF_COPY | SYNF_STEAL)
#define SYNF_OLOCK   0x00000008
#define SYNF_NBIO    0x00000010
#define SYNF_WAKEUP  0x00000020
#define SYNF_SELECT  0x00000040
  struct pms_softc *psc;
  unsigned char (*pktring)[6];
  unsigned int pktrh;
  unsigned int pktrt;
  unsigned int pktrn;
  struct selinfo rsel;
  } ;
#define SYN_KIND_COPY  0
#define SYN_KIND_STEAL 1

static SOFTC softc_vec[NSYNAPTICS];

static void maybe_init(void)
{
 static int initted = 0;
 int i;
 SOFTC *sc;

 if (initted) return;
 aprint_normal("synaptics init\n");
 initted = 1;
 for (i=NSYNAPTICS-1;i>=0;i--)
  { sc = &softc_vec[i];
    sc->unit = i;
    sc->flags = 0;
    /* what's the right way to init a struct selinfo? */
     { static const struct selinfo selinit;
       sc->rsel = selinit;
     }
  }
}

/* XXX this declaration should come from config(8)-generated autoconf machinery */
extern void synapticsattach(int);
void synapticsattach(int n)
{
 maybe_init();
 printf("synapticsattach(%d)\n",n);
}

static void syn_queue_data(struct pms_softc *psc)
{
 struct synaptics_softc *ssc;
 SOFTC *sc;
 int s;
 unsigned char (*re)[6];
 unsigned int flags;

 ssc = &psc->u.synaptics;
 if (ssc->dev_unit < 0) panic("synaptics queueing data on nonexistent unit");
 sc = &softc_vec[ssc->dev_unit];
 s = splhigh();
 if (sc->flags & SYNF_OPEN)
  { re = &sc->pktring[sc->pktrh];
    sc->pktrh = RING_ADV(sc->pktrh);
    if (sc->pktrn > SYNRINGSIZE) panic("syn_queue_data overfull ring");
    if (sc->pktrn == SYNRINGSIZE)
     { sc->pktrt = RING_ADV(sc->pktrt);
     }
    else
     { sc->pktrn ++;
     }
    bcopy(&psc->packet[0],&re[0][0],6);
  }
 flags = sc->flags;
 sc->flags = flags & ~(SYNF_WAKEUP|SYNF_SELECT);
 splx(s);
 if (flags & SYNF_WAKEUP) wakeup(sc);
 if (flags & SYNF_SELECT) selnotify(&sc->rsel,0);
}

int syndev_alloc_unit(struct pms_softc *psc)
{
 int i;
 SOFTC *sc;

 maybe_init();
 printf("syndev_alloc_unit for %s -> ",&psc->sc_dev.dv_xname[0]);
 for (i=0;i<NSYNAPTICS;i++)
  { sc = &softc_vec[i];
    if (sc->flags & SYNF_EXISTS) continue;
    sc->flags |= SYNF_EXISTS;
    sc->psc = psc;
    printf("%d\n",i);
    return(i);
  }
 printf("fail\n");
 return(-1);
}

void syndev_setup(int unit)
{
}

int syndev_input(struct pms_softc *psc)
{
 struct synaptics_softc *ssc;
 SOFTC *sc;
 int s;
 unsigned int flags;
 static typeof(time_second) tstamp;
 typeof(time_second) curts;

 ssc = &psc->u.synaptics;
 if (ssc->dev_unit < 0) return(0);
 sc = &softc_vec[ssc->dev_unit];
 s = splhigh();
 flags = sc->flags;
 splx(s);
 switch (flags & (SYNF_COPY|SYNF_STEAL))
  { case SYNF_COPY:
       syn_queue_data(psc);
       curts = time_second;
       if (curts != tstamp)
	{ printf("c");
	  tstamp = curts;
	}
       return(0);
       break;
    case SYNF_STEAL:
       syn_queue_data(psc);
       curts = time_second;
       if (curts != tstamp)
	{ printf("s");
	  tstamp = curts;
	}
       return(1);
       break;
    case 0:
       curts = time_second;
       if (curts != tstamp)
	{ printf("-");
	  tstamp = curts;
	}
       return(0);
       break;
  }
 panic("syndev_input: impossible flags");
}

static int synapticsopen(dev_t dev, int flag, int mode, struct lwp *p)
{
 unsigned int unit;
 unsigned int kind;
 SOFTC *sc;
 int s;

 printf("synapticsopen dev %x\n",dev);
 unit = minor(dev);
 kind = unit % 8;
 unit /= 8;
 printf("synapticsopen kind %u, unit %u\n",kind,unit);
 if (unit >= NSYNAPTICS)
  { printf("synapticsopen ENXIO unit >= NSYNAPTICS (%d)\n",NSYNAPTICS);
    return(ENXIO);
  }
 s = splhigh();
 sc = &softc_vec[unit];
 if (! (sc->flags & SYNF_EXISTS))
  { splx(s);
    printf("synapticsopen ENXIO unit %d !SYNF_EXISTS\n",unit);
    return(ENXIO);
  }
 if (sc->flags & (SYNF_OPEN|SYNF_OLOCK))
  { splx(s);
    printf("synapticsopen EBUSY unit %d flags %x\n",unit,sc->flags);
    return(EBUSY);
  }
 sc->flags |= SYNF_OLOCK;
 splx(s);
 sc->pktring = malloc(SYNRINGSIZE*sizeof(*sc->pktring),M_DEVBUF,M_WAITOK);
 sc->pktrh = 0;
 sc->pktrt = 0;
 sc->pktrn = 0;
 s = splhigh();
 sc->flags &= ~SYNF_OLOCK;
 switch (kind)
  { case SYN_KIND_COPY:
       sc->flags |= SYNF_COPY;
       break;
    case SYN_KIND_STEAL:
       sc->flags |= SYNF_STEAL;
       break;
    default:
       splx(s);
       printf("synapticsopen ENXIO bad kind %d\n",kind);
       return(ENXIO);
       break;
  }
 if (flag & O_NONBLOCK) sc->flags |= SYNF_NBIO; else sc->flags &= ~SYNF_NBIO;
 splx(s);
 printf("synapticsopen ok\n");
 return(0);
}

static int synapticsclose(dev_t dev, int flag, int mode, struct lwp *p)
{
 unsigned int unit;
 unsigned int kind;
 SOFTC *sc;
 int s;
 unsigned char (*oldring)[6];

 unit = minor(dev);
 kind = unit % 8;
 unit /= 8;
 if (unit >= NSYNAPTICS) panic("closing out-of-range synaptics");
 s = splhigh();
 sc = &softc_vec[unit];
 if (! (sc->flags & SYNF_EXISTS)) panic("closing nonexistent synaptics");
 switch (kind)
  { case SYN_KIND_COPY:
       sc->flags &= ~SYNF_COPY;
       break;
    case SYN_KIND_STEAL:
       sc->flags &= ~SYNF_STEAL;
       break;
    default:
       panic("closing bad-kind synaptics");
       break;
  }
 if (sc->flags & SYNF_OPEN) panic("synapticsclose: still open");
 oldring = sc->pktring;
 splx(s);
 free(oldring,M_DEVBUF);
 return(0);
}

static int synapticsread(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 SOFTC *sc;
 int s;
 unsigned int flags;
 int e;
 unsigned char pkt[6];
 int any;

 unit = minor(dev) / 8;
 if (unit >= NSYNAPTICS) panic("reading out-of-range synaptics");
 sc = &softc_vec[unit];
 if (uio->uio_resid < 6) return(EMSGSIZE);
 s = splhigh();
 flags = sc->flags;
 splx(s);
 if (! (flags & SYNF_EXISTS)) panic("reading nonexistent synaptics");
 if (! (flags & SYNF_OPEN)) panic("reading non-open synaptics");
 any = 0;
 while (uio->uio_resid >= 6)
  { s = splhigh();
    if (sc->pktrn > 0)
     { bcopy(&sc->pktring[sc->pktrt],&pkt[0],6);
       sc->pktrn --;
       sc->pktrt = RING_ADV(sc->pktrt);
       splx(s);
       e = uiomove(&pkt[0],6,uio);
       if (e) return(e);
       any = 1;
     }
    else
     { if (any || (flags & SYNF_NBIO))
	{ splx(s);
	  return(0);
	}
       sc->flags |= SYNF_WAKEUP;
       e = tsleep(sc,PZERO|PCATCH,"synread",0);
       splx(s);
       if (e) return(e);
     }
  }
 return(0);
}

static int synapticswrite(dev_t dev, struct uio *uio, int ioflag)
{
 return(EIO);
}

static int synapticsioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *p)
{
 unsigned int unit;
 SOFTC *sc;
 int s;
 unsigned int flags;
 int v;

 unit = minor(dev) / 8;
 if (unit >= NSYNAPTICS) panic("ioctling out-of-range synaptics");
 sc = &softc_vec[unit];
 s = splhigh();
 flags = sc->flags;
 splx(s);
 if (! (flags & SYNF_EXISTS)) panic("ioctling nonexistent synaptics");
 if (! (flags & SYNF_OPEN)) panic("ioctling non-open synaptics");
 switch (cmd)
  { case FIONBIO:
       v = *(int *)data;
       s = splhigh();
       if (v) sc->flags |= SYNF_NBIO; else sc->flags &= ~SYNF_NBIO;
       splx(s);
       break;
    case FIOASYNC:
       /* annoying bug in fcntl(F_SETFL) on devices in some versions:
	  the device must support both FIONBIO and FIOASYNC ioctls,
	  even if the corresponding fcntl bits are never set, or
	  nonblocking mode is gratuitously turned off! */
       if (*(int *)data) return(EOPNOTSUPP);
       break;
    case _IOR('s',1,unsigned int):
       *(unsigned int *)data = sc->psc->u.synaptics.flags;
       break;
    default:
       return(ENOTTY);
       break;
  }
 return(0);
}

static int synapticspoll(dev_t dev, int events, struct lwp *p)
{
 unsigned int unit;
 SOFTC *sc;
 int s;
 unsigned int flags;
 int revents;

 unit = minor(dev) / 8;
 if (unit >= NSYNAPTICS) panic("polling out-of-range synaptics");
 sc = &softc_vec[unit];
 s = splhigh();
 flags = sc->flags;
 splx(s);
 if (! (flags & SYNF_EXISTS)) panic("ioctling nonexistent synaptics");
 if (! (flags & SYNF_OPEN)) panic("ioctling non-open synaptics");
 revents = events & (POLLOUT|POLLWRNORM);
 s = splhigh();
 if (sc->pktrn > 0)
  { revents |= events & (POLLIN|POLLRDNORM);
  }
 else
  { selrecord(p,&sc->rsel);
    sc->flags |= SYNF_SELECT;
  }
 splx(s);
 return(revents);
}

const struct cdevsw synaptics_cdevsw
 = { &synapticsopen, &synapticsclose, &synapticsread,
     &synapticswrite, &synapticsioctl, nostop, notty,
     &synapticspoll, nommap, nokqfilter, D_OTHER };

#endif
