/*	$NetBSD: elf_machdep.h,v 1.3 2005/12/11 12:18:31 christos Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_NS32K:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID		EM_NS32K

#define	ARCH_ELFSIZE		32	/* MD native binary size */

/* XXX relocations */
