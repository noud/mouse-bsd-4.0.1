#ifndef _DEVTREE_VERS_H_563d7b91_
#define _DEVTREE_VERS_H_563d7b91_

/*
 * We have to include this very early, to get __NetBSD_Version__.
 *
 * This also hides a lot of include-file bugs; I don't entirely
 *  understand _why_ including <sys/param.h> hides so many of them when
 *  they're clearly not missing parameters - for example, uvm/uvm.h,
 *  through the chain uvm/uvm_extern.h -> uvm/uvm_param.h ->
 *  machine/vmparam.h, uses struct simplelock without making sure it's
 *  defined first.  Since <sys/param.h> doesn't define it, I don't
 *  understand why including it first hides this bug, but it does.
 *
 * This is a mixed blessing.  It means we don't have to fix (or paper
 *  over) a lot of stuff - but it also means we lose out on a chance to
 *  find (and fix properly) that same lot of stuff.  Since we have to
 *  include it anyway to get __NetBSD_Version__, we make a virtue of
 *  necessity and declare we consider the former the greater. :)
 */
#include <sys/param.h>

/*
 * Unfortunately there is no way for us to create a #define which
 *  devtree.c can use to get the appropriate list of include files.
 *  So, instead, there's a flag (DT_INCLUDE_INCLUDES) which can be
 *  defined before including this; when that's done, it includes the
 *  appropriate include files, based on a flag define set up the first
 *  time through (which need not be different from the time with
 *  DT_INCLUDE_INCLUDES set).  DT_INCLUDE_INCLUDES is undefined after
 *  the include files are brought in.
 *
 * This is a bit ugly, in that it separates the includes from the code,
 *  but I consider the alternative - flag macros here which devtree.c
 *  uses to include the correct files - even worse; it means that
 *  porting to a new OS version requires creating a new flag macro
 *  visible to devtree.c (the current scheme requires creating one
 *  anyway, but it's private to this file), and clutters devtree.c
 *  (which I'd like to keep relatively uncluttered) with lots of
 *  "extra" #include lines.
 *
 * However, C's preprocessor design pretty much compels something of
 *  the sort.  This choice means that this file can't have multiple
 *  inclusions streamlined by (eg) gcc's recognition and optimization
 *  of the #ifndef/#define/.../#endif idiom, but I think that counts as
 *  optimizing machine costs (a little unnecessary processing, which
 *  doesn't happen much anyway since we don't include this file very
 *  often) at the expense of human costs (dealing with cluttered code).
 *  (The apparent idempotency guard at the top of this file actually
 *  protects only part of the file; its matching #endif is the one just
 *  before "#ifdef DT_INCLUDE_INCLUDES", not the one at the end of the
 *  file.)
 */

#define DT_VERSION_NOT_SUPPORTED

#ifndef __NetBSD_Version__
#error "No __NetBSD_Version__ - this version not supported"
#endif

#if __NetBSD_Version__ == 400000003
/* 4.0.1 */
/*
 * The description of __NetBSD_Version__ in <sys/param.h> leads me to
 *  expect it to be 400000100, not 400000003.  I don't know why the
 *  discrepancy, but 400000003 is what it's set to in 4.0.1.
 */
#undef DT_VERSION_NOT_SUPPORTED
#define DT_DEVICE_ROUTINE_DECLS /* nothing */
#define WAKEUP_SELECT(sc)\
	selnotify(&sc->rsel,0)
#define DEVSW_SCLASS static
#define PROCTYPE struct lwp *
#define DEFINE_CDEVSW\
	const struct cdevsw devtree_cdevsw				\
	 = { &devtreeopen, &devtreeclose, &devtreeread,			\
	     &devtreewrite, &devtreeioctl, nostop, notty,		\
	     &devtreepoll, nommap, nokqfilter, D_OTHER };
#define DT_VERSION_4_0_1
#endif

#if __NetBSD_Version__ == 104200000
/* 1.4T */
#undef DT_VERSION_NOT_SUPPORTED
#define DT_DEVICE_ROUTINE_DECLS \
	extern int devtreeopen(dev_t, int, int, struct proc *);	\
	extern int devtreeclose(dev_t, int, int, struct proc *);	\
	extern int devtreeread(dev_t, struct uio *, int);		\
	extern int devtreewrite(dev_t, struct uio *, int);		\
	extern int devtreeioctl(dev_t, u_long, caddr_t, int, struct proc *);\
	extern int devtreepoll(dev_t, int, struct proc *);
#define WAKEUP_SELECT(sc)\
	selwakeup(&sc->rsel)
#define DEVSW_SCLASS /* nothing */
#define PROCTYPE struct proc *
#define DEFINE_CDEVSW /* nothing */
#define DT_VERSION_1_4T
#endif

#ifdef DT_VERSION_NOT_SUPPORTED
#error "This NetBSD version not supported"
#endif

#endif

#ifdef DT_INCLUDE_INCLUDES

#ifdef DT_VERSION_4_0_1
#undef DT_INCLUDE_INCLUDES
#if 0
#include <sys/lwp.h>
#endif
#include <sys/poll.h>
#include <sys/conf.h>
#if 0
#include <sys/file.h>
#endif
#include <sys/proc.h>
#if 0
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#endif
#include <sys/malloc.h>
#include <sys/select.h>
#if 0
#include <sys/selinfo.h>
#include <sys/filedesc.h>
#include <miscfs/specfs/specdev.h>
#endif
#endif

#ifdef DT_VERSION_1_4T
#undef DT_INCLUDE_INCLUDES
#if 0
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/filio.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <vm/vm_kern.h>
#include <sys/filedesc.h>
#include <uvm/uvm_extern.h>
#include <miscfs/specfs/specdev.h>
#endif
#endif

#ifdef DT_INCLUDE_INCLUDES
#error "missing includes block - DT_INCLUDE_INCLUDES survived"
#endif

#endif
