# This file is automatically generated.  DO NOT EDIT!
# Generated from: 	NetBSD: mknative-binutils,v 1.5 2006/02/02 20:06:04 skrll Exp 
# Generated from: NetBSD: mknative.common,v 1.8 2006/05/26 19:17:21 mrg Exp 
#
G_DEFS=-DHAVE_CONFIG_H -I. -I${GNUHOSTDIST}/ld -I.
G_EMUL=elf64ppc
G_EMULATION_OFILES=eelf64ppc.o eelf32ppc.o eelf32ppclinux.o eelf32ppcsim.o
G_INCLUDES=-D_GNU_SOURCE -I. -I${GNUHOSTDIST}/ld -I../bfd -I${GNUHOSTDIST}/ld/../bfd -I${GNUHOSTDIST}/ld/../include -I${GNUHOSTDIST}/ld/../intl -I../intl   -DLOCALEDIR="\"/usr/local/share/locale\""
G_OFILES=ldgram.o ldlex.o lexsup.o ldlang.o mri.o ldctor.o ldmain.o  ldwrite.o ldexp.o  ldemul.o ldver.o ldmisc.o  ldfile.o ldcref.o eelf64ppc.o eelf32ppc.o eelf32ppclinux.o eelf32ppcsim.o 
G_STRINGIFY=astring.sed
G_TEXINFOS=ld.texinfo
G_target_alias=powerpc64--netbsd
