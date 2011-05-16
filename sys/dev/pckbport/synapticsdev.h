#ifndef _SYNAPTICSDEV_H_c761e174_
#define _SYNAPTICSDEV_H_c761e174_

struct pms_softc;

extern int syndev_alloc_unit(struct pms_softc *);
extern void syndev_setup(int);
extern int syndev_input(struct pms_softc *);

#endif
