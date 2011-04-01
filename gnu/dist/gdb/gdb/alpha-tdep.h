/* Common target dependent code for GDB on Alpha systems.
   Copyright 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2002 Free
   Software Foundation, Inc.

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

#ifndef ALPHA_TDEP_H
#define ALPHA_TDEP_H

#include "osabi.h"

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places;  REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */
#define ALPHA_REGISTER_SIZE 8

/* Number of machine registers.  */
#define ALPHA_NUM_REGS 66

/* Total amount of space needed to store our copies of the machine's
   register state.  */
#define ALPHA_REGISTER_BYTES (ALPHA_NUM_REGS * 8)

/* Largest value REGISTER_RAW_SIZE can have.  */
#define ALPHA_MAX_REGISTER_RAW_SIZE 8

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */
#define ALPHA_MAX_REGISTER_VIRTUAL_SIZE 8

/* Register numbers of various important registers.
   Note that most of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and FP_REGNUM is a "phony" register number which is too large
   to be an actual register number as far as the user is concerned
   but serves to get the desired value when passed to read_register.  */

#define ALPHA_V0_REGNUM	     0  /* Function integer return value */
#define ALPHA_T7_REGNUM	     8  /* Return address register for OSF/1 __add* */
#define ALPHA_S0_REGNUM	     9  /* First callee-saved register */  
#define ALPHA_GCC_FP_REGNUM 15  /* Used by gcc as frame register */
#define ALPHA_A0_REGNUM     16  /* Loc of first arg during a subr call */
#define ALPHA_T9_REGNUM     23  /* Return address register for OSF/1 __div* */
#define ALPHA_T12_REGNUM    27  /* Contains start addr of current proc */
#define ALPHA_SP_REGNUM     30  /* Contains address of top of stack */
#define ALPHA_RA_REGNUM     26  /* Contains return address value */
#define ALPHA_ZERO_REGNUM   31  /* Read-only register, always 0 */
#define ALPHA_FP0_REGNUM    32  /* Floating point register 0 */
#define ALPHA_FPA0_REGNUM   48  /* First float arg during a subr call */
#define ALPHA_FPCR_REGNUM   63  /* Floating point control register */
#define ALPHA_PC_REGNUM     64  /* Contains program counter */
#define ALPHA_FP_REGNUM     65  /* Virtual frame pointer */

/* The alpha has two different virtual pointers for arguments and locals.
   
   The virtual argument pointer is pointing to the bottom of the argument
   transfer area, which is located immediately below the virtual frame
   pointer. Its size is fixed for the native compiler, it is either zero
   (for the no arguments case) or large enough to hold all argument registers.
   gcc uses a variable sized argument transfer area. As it has
   to stay compatible with the native debugging tools it has to use the same
   virtual argument pointer and adjust the argument offsets accordingly.
   
   The virtual local pointer is localoff bytes below the virtual frame
   pointer, the value of localoff is obtained from the PDR.  */
#define ALPHA_NUM_ARG_REGS   6

/* Target-dependent structure in gdbarch.  */
struct gdbarch_tdep
{
  enum gdb_osabi osabi;		/* OS/ABI of inferior.  */

  CORE_ADDR vm_min_address;	/* used by heuristic_proc_start */

  /* If PC is inside a dynamically-generated signal trampoline function
     (i.e. one copied onto the user stack at run-time), return how many
     bytes PC is beyond the start of that function.  Otherwise, return -1.  */
  LONGEST (*dynamic_sigtramp_offset) (CORE_ADDR);

  /* If FRAME refers to a sigtramp frame, return the address of the next
     frame.  */
  CORE_ADDR (*skip_sigtramp_frame) (struct frame_info *, CORE_ADDR);

  /* Translate a signal handler frame into the address of the sigcontext
     structure for that signal handler.  */
  CORE_ADDR (*sigcontext_addr) (struct frame_info *);

  int jb_pc;			/* Offset to PC value in jump buffer.
				   If htis is negative, longjmp support
				   will be disabled.  */
  size_t jb_elt_size;		/* And the size of each entry in the buf. */
};

void alpha_software_single_step (enum target_signal, int);

#endif /* ALPHA_TDEP_H */
