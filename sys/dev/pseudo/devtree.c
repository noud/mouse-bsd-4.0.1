#include <dev/pseudo/devtree-vers.h>

#define DT_INCLUDE_INCLUDES
#include <dev/pseudo/devtree-vers.h>

#include <dev/pseudo/devtree-intf.h>

#define MAXUNIT 256
#define MAXQUEUE 8192

typedef struct softc SOFTC;
typedef struct dblk DBLK;

struct softc {
  int unit;
  unsigned int flags;
#define SF_OPEN    0x00000001
#define SF_NBIO    0x00000002
#define SF_WATCH   0x00000004
#define SF_OVERRUN 0x00000008
  DBLK *dq;
  DBLK **dqt;
  int dqlen;
  struct selinfo rsel;
  } ;

struct dblk {
  DBLK *link;
  int len;
  int ptr;
  } ;

static SOFTC **dtscv;
static int ndevtree;
static int hookid;

int devtree_verbose = 0;

void devtreeattach(int);

void devtreeattach(int n)
{
 ndevtree = 0;
 dtscv = 0;
 hookid = -1;
}

#ifdef DIAGNOSTIC

#define SC_DBQLEN_CHECK(sc) do { sc_dbqlen_check((sc)); } while (0)

static void sc_dbqlen_check(SOFTC *sc)
{
 int n;
 DBLK *db;

 n = 0;
 for (db=sc->dq;db;db=db->link) n += db->len - db->ptr;
 if (n != sc->dqlen)
  { printf("*** devtree driver found qlen %d should be %d\n",sc->dqlen,n);
    sc->dqlen = n;
  }
}

#else

#define SC_DBQLEN_CHECK(sc) do { } while (0)

#endif

static SOFTC *ensure_softc(int u)
{
 SOFTC *s;

 if ((u < 0) || (u >= MAXUNIT)) panic("ensure_softc: impossible unit %d",u);
 if (u >= ndevtree)
  { SOFTC **newv;
    newv = malloc((u+1)*sizeof(SOFTC *),M_DEVBUF,M_WAITOK);
    if (ndevtree)
     { bcopy(dtscv,newv,ndevtree*sizeof(SOFTC *));
       free(dtscv,M_DEVBUF);
     }
    dtscv = newv;
    while (ndevtree <= u) newv[ndevtree++] = 0;
  }
 s = dtscv[u];
 if (! s)
  { s = malloc(sizeof(SOFTC),M_DEVBUF,M_WAITOK);
    s->unit = u;
    s->flags = 0;
    s->dq = 0;
    s->dqt = &s->dq;
    s->dqlen = 0;
     { /* How the fsck are you supposed to initialize a struct selinfo?
	  This seems to work, but is it right?  Who knows.... */
       static const struct selinfo selinfo_init;
       s->rsel = selinfo_init;
     }
    dtscv[u] = s;
  }
 return(s);
}

static void flush_dq(SOFTC *sc)
{
 DBLK *d;

 while ((d = sc->dq))
  { sc->dq = d->link;
    free(d,M_DEVBUF);
  }
 sc->dqt = &sc->dq;
 sc->dqlen = 0;
}

static void append_dblk(SOFTC *sc, const void *data, int len)
{
 DBLK *b;

 b = malloc(sizeof(DBLK)+len,M_DEVBUF,M_WAITOK);
 bcopy(data,b+1,len);
 b->len = len;
 b->ptr = 0;
 b->link = 0;
 *sc->dqt = b;
 sc->dqt = &b->link;
 WAKEUP_SELECT(sc);
 wakeup(sc);
}

static void set_overrun(SOFTC *sc)
{
 static const char om = DTMSG_OVERRUN;

 append_dblk(sc,&om,1);
 sc->flags |= SF_OVERRUN;
}

static void tell_watchers(const char *msg, int msglen)
{
 int i;
 SOFTC *sc;

 for (i=ndevtree-1;i>=0;i--)
  { sc = dtscv[i];
    if (sc && ((sc->flags & (SF_WATCH|SF_OVERRUN)) == SF_WATCH))
     { if (sc->dqlen > MAXQUEUE)
	{ set_overrun(sc);
	}
       else
	{ append_dblk(sc,msg,msglen);
	}
     }
  }
}

static void dump_current_tree(SOFTC *sc)
{
 char msg[1+2+(2*sizeof(((device_t)0)->dv_xname))];
 device_t d;
 int l;

 if (sc->flags & SF_OVERRUN) return;
 if (sc->dqlen > MAXQUEUE)
  { set_overrun(sc);
    return;
  }
 msg[0] = DTMSG_TREE;
 TAILQ_FOREACH(d,&alldevs,dv_list)
  { msg[1] = strlen(&d->dv_xname[0]);
    bcopy(&d->dv_xname[0],&msg[3],msg[1]);
    if (d->dv_parent)
     { msg[2] = strlen(&d->dv_parent->dv_xname[0]);
       bcopy(&d->dv_parent->dv_xname[0],&msg[3+msg[1]],msg[2]);
       l = 3 + msg[1]+ msg[2];
     }
    else
     { msg[2] = 0xff;
       l = 3 + msg[1];
     }
    append_dblk(sc,&msg[0],l);
  }
 msg[1] = 0xff;
 append_dblk(sc,&msg[0],2);
}

static void devtree_hook(device_t dev, enum config_hook_op op)
{
 char msg[1+2+(2*sizeof(dev->dv_xname))];
 int l;

 switch (op)
  { case CONFIG_HOOK_ATTACH_REAL:
       msg[0] = DTMSG_ATTACH_REAL;
       msg[1] = strlen(&dev->dv_xname[0]);
       bcopy(&dev->dv_xname[0],&msg[3],msg[1]);
       if (dev->dv_parent)
	{ msg[2] = strlen(&dev->dv_parent->dv_xname[0]);
	  bcopy(&dev->dv_parent->dv_xname[0],&msg[3+msg[1]],msg[2]);
	  l = 3 + msg[1] + msg[2];
	}
       else
	{ msg[2] = 0xff;
	  l = 3 + msg[1];
	}
       tell_watchers(&msg[0],l);
       printf("devtree_hook %s ATTACH_REAL\n",&dev->dv_xname[0]);
       break;
    case CONFIG_HOOK_ATTACH_PSEUDO:
       msg[0] = DTMSG_ATTACH_PSEUDO;
       msg[1] = strlen(&dev->dv_xname[0]);
       bcopy(&dev->dv_xname[0],&msg[2],msg[1]);
       tell_watchers(&msg[0],2+msg[1]);
       printf("devtree_hook %s ATTACH_PSEUDO\n",&dev->dv_xname[0]);
       break;
    case CONFIG_HOOK_DETACH:
       msg[0] = DTMSG_DETACH;
       msg[1] = strlen(&dev->dv_xname[0]);
       bcopy(&dev->dv_xname[0],&msg[2],msg[1]);
       tell_watchers(&msg[0],2+msg[1]);
       printf("devtree_hook %s DETACH\n",&dev->dv_xname[0]);
       break;
    default:
       printf("devtree_hook %s ?%d\n",&dev->dv_xname[0],(int)op);
       break;
  }
}

DEVSW_SCLASS int devtreeopen(dev_t dev, int flag, int mode, PROCTYPE p)
{
 unsigned int u;
 SOFTC *sc;

 u = minor(dev);
 if (u >= MAXUNIT) return(ENXIO);
 sc = ensure_softc(u);
 if (! (sc->flags & SF_OPEN))
  { sc->flags = 0;
    sc->dq = 0;
    sc->dqt = &sc->dq;
    sc->dqlen = 0;
  }
 if (hookid < 0) hookid = config_set_attach_detach_hook(&devtree_hook);
 sc->flags |= SF_OPEN;
 if (devtree_verbose) printf("devtreeopen: %d succeeded\n",u);
 return(0);
}

DEVSW_SCLASS int devtreeclose(dev_t dev, int flag, int mode, PROCTYPE p)
{
 unsigned int u;
 SOFTC *sc;
 int i;

 u = minor(dev);
 if (u >= ndevtree) panic("closing devtree #%d",u);
 sc = dtscv[u];
 sc->flags = 0;
 flush_dq(sc);
 WAKEUP_SELECT(sc);
 wakeup(sc);
 for (i=ndevtree-1;i>=0;i--)
  { sc = dtscv[i];
    if (sc->flags & SF_OPEN) break;
  }
 if (i < 0)
  { config_clear_attach_detach_hook(hookid);
    hookid = -1;
  }
 if (devtree_verbose) printf("devtreeclose: %d succeeded\n",u);
 return(0);
}

DEVSW_SCLASS int devtreeread(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int u;
 SOFTC *sc;
 int e;
 DBLK *db;
 int n;

 u = minor(dev);
 if (u >= ndevtree) panic("reading devtree #%d",u);
 sc = dtscv[u];
 if (devtree_verbose) printf("devtreeread %d: entry\n",u);
 while (! sc->dq)
  { if (sc->flags & SF_NBIO)
     { if (devtree_verbose) printf("devtreeread %d: EWOULDBLOCK\n",u);
       return(EWOULDBLOCK);
     }
    e = tsleep(sc,PZERO|PCATCH,"acread",0);
    if (e)
     { if (devtree_verbose) printf("devtreeread %d: %d (tsleep)\n",u,e);
       return(e);
     }
  }
 while (uio->uio_resid && (db = sc->dq))
  { n = db->len - db->ptr;
    if (n > uio->uio_resid) n = uio->uio_resid;
    e = uiomove(db->ptr+(char *)(db+1),n,uio);
    if (e)
     { if (devtree_verbose) printf("devtreeread %d: %d (uiomove)\n",u,e);
       return(e);
     }
    db->ptr += n;
    sc->dqlen -= n;
    if (db->ptr >= db->len)
     { if (! (sc->dq = db->link)) sc->dqt = &sc->dq;
       free(db,M_DEVBUF);
     }
  }
 SC_DBQLEN_CHECK(sc);
 if (devtree_verbose) printf("devtreeread %d: done\n",u);
 return(0);
}

DEVSW_SCLASS int devtreewrite(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int u;
 SOFTC *sc;
 int e;
 char cmd;

 u = minor(dev);
 if (u >= ndevtree) panic("writing devtree #%d",u);
 sc = dtscv[u];
 if (devtree_verbose) printf("devtreewrite %d: entry\n",u);
 if (uio->uio_resid < 1)
  { if (devtree_verbose) printf("devtreewrite %d: zero-length uio\n",u);
    return(0);
  }
 e = uiomove(&cmd,1,uio);
 if (e)
  { if (devtree_verbose) printf("devtreewrite %d: %d (uiomove cmd)\n",u,e);
    return(e);
  }
 switch (cmd)
  { default:
       if (devtree_verbose) printf("devtreewrite %d: EIO (bad cmd)\n",u);
       return(EIO);
       break;
    case DTCMD_VERBOSE:
       if (uio->uio_resid < 1)
	{ if (devtree_verbose) printf("devtreewrite %d: EIO (v without arg)\n",u);
	  return(EIO);
	}
       e = uiomove(&cmd,1,uio);
       if (e)
	{ if (devtree_verbose) printf("devtreewrite %d: %d (uiomove v arg)\n",u,e);
	  return(e);
	}
       switch (cmd)
	{ case DTCMD_ARG_OFF:
	     devtree_verbose = 0;
	     break;
	  case DTCMD_ARG_ON:
	     devtree_verbose = 1;
	     break;
	  default:
	     if (devtree_verbose) printf("devtreewrite %d: EIO (bad v arg)\n",u);
	     return(EIO);
	     break;
	}
       break;
    case DTCMD_WATCH:
       if (uio->uio_resid < 1)
	{ if (devtree_verbose) printf("devtreewrite %d: EIO (w without arg)\n",u);
	  return(EIO);
	}
       e = uiomove(&cmd,1,uio);
       if (e)
	{ if (devtree_verbose) printf("devtreewrite %d: %d (uiomove w arg)\n",u,e);
	  return(e);
	}
       switch (cmd)
	{ case DTCMD_ARG_OFF:
	     sc->flags &= ~SF_WATCH;
	     break;
	  case DTCMD_ARG_ON:
	     sc->flags |= SF_WATCH;
	     break;
	  default:
	     if (devtree_verbose) printf("devtreewrite %d: EIO (bad v arg)\n",u);
	     return(EIO);
	     break;
	}
       break;
    case DTCMD_OVERRUN:
       if (sc->dqlen == 0) sc->flags &= ~SF_OVERRUN; else return(EINVAL);
       break;
    case DTCMD_TREE:
       dump_current_tree(sc);
       break;
  }
 if (devtree_verbose) printf("devtreewrite %d: done\n",u);
 return(0);
}

DEVSW_SCLASS int devtreeioctl(dev_t dev, u_long cmd, caddr_t data, int flag, PROCTYPE p)
{
 unsigned int u;
 SOFTC *sc;

 u = minor(dev);
 if (u >= ndevtree) panic("ioctling devtree #%d",u);
 sc = dtscv[u];
 if (devtree_verbose) printf("devtreeioctl %d: entry\n",u);
 switch (cmd)
  { case FIONBIO:
       if (devtree_verbose) printf("devtreeioctl %d: FIONBIO %s\n",u,(*(int *)data)?"on":"off");
       if (*(int *)data) sc->flags |= SF_NBIO; else sc->flags &= ~SF_NBIO;
       break;
    case FIOASYNC:
       /* annoying bug in fcntl(F_SETFL) on devices in some versions:
	  the device must support both FIONBIO and FIOASYNC ioctls,
	  even if the corresponding fcntl bits are never set, or
	  nonblocking mode is gratuitously turned off! */
       if (*(int *)data) return(EOPNOTSUPP);
       break;
    default:
       if (devtree_verbose) printf("devtreeioctl %d: ENOTTY cmd %08lx\n",u,(unsigned long int)cmd);
       return(ENOTTY);
       break;
  }
 if (devtree_verbose) printf("devtreeioctl %d: done\n",u);
 return(0);
}

DEVSW_SCLASS int devtreepoll(dev_t dev, int events, PROCTYPE p)
{
 unsigned int u;
 SOFTC *sc;
 int revents;

 u = minor(dev);
 if (u >= ndevtree) panic("polling devtree #%d",u);
 sc = dtscv[u];
 if (devtree_verbose) printf("devtreeioctl %d: entry\n",u);
 revents = events & (POLLOUT|POLLWRNORM);
 if (sc->dq)
  { revents |= events & (POLLIN|POLLRDNORM);
  }
 else
  { selrecord(p,&sc->rsel);
  }
 return(revents);
}

DEFINE_CDEVSW
