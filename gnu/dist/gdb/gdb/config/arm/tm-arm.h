/* Definitions to target GDB to ARM targets.
   Copyright 1986, 1987, 1988, 1989, 1991, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef TM_ARM_H
#define TM_ARM_H

#ifndef GDB_MULTI_ARCH
#define GDB_MULTI_ARCH 1
#endif

#ifdef ARM_26BIT_R15
/* Functions to unpack and pack R15 on 26-bit ARMs. */
void arm_supply_26bit_r15 (char *);
void arm_read_26bit_r15 (char *);
#endif

/* Most ARMs don't have single stepping capability, so provide a
   single-stepping mechanism by default */
#ifndef SOFTWARE_SINGLE_STEP_P
#define SOFTWARE_SINGLE_STEP_P 1
#endif
#if SOFTWARE_SINGLE_STEP_P
#define SOFTWARE_SINGLE_STEP(sig,bpt) arm_software_single_step((sig), (bpt))
void arm_software_single_step PARAMS((int, int));
#endif

/* Specify that for the native compiler variables for a particular
   lexical context are listed after the beginning LBRAC instead of
   before in the executables list of symbols.  */
#define VARIABLES_INSIDE_BLOCK(desc, gcc_p) (!(gcc_p))

#endif /* TM_ARM_H */
