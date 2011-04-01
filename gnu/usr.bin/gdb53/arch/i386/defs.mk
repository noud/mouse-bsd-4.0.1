# $NetBSD: defs.mk,v 1.6 2006/02/17 07:58:59 skrll Exp $
G_GDB_OBJS=\
annotate.o \
arch-utils.o \
ax-gdb.o \
ax-general.o \
bcache.o \
blockframe.o \
breakpoint.o \
buildsym.o \
builtin-regs.o \
c-exp.tab.o \
c-lang.o \
c-typeprint.o \
c-valprint.o \
ch-exp.o \
ch-lang.o \
ch-typeprint.o \
ch-valprint.o \
cli-cmds.o \
cli-decode.o \
cli-dump.o \
cli-out.o \
cli-script.o \
cli-setshow.o \
coffread.o \
complaints.o \
completer.o \
copying.o \
corefile.o \
corelow.o \
cp-abi.o \
cp-valprint.o \
dbxread.o \
dcache.o \
demangle.o \
doublest.o \
dwarf2read.o \
dwarfread.o \
elfread.o \
environ.o \
eval.o \
event-loop.o \
event-top.o \
exec.o \
expprint.o \
f-exp.tab.o \
f-lang.o \
f-typeprint.o \
f-valprint.o \
findvar.o \
fork-child.o \
frame.o \
gdb.o \
gdb-events.o \
gdbarch.o \
gdbtypes.o \
gnu-v2-abi.o \
gnu-v3-abi.o \
hpacc-abi.o \
i386-tdep.o \
i386bsd-nat.o \
i386bsd-tdep.o \
i386nbsd-nat.o \
i386nbsd-tdep.o \
i387-tdep.o \
inf-loop.o \
infcmd.o \
inflow.o \
infptrace.o \
infrun.o \
inftarg.o \
init.o \
jv-exp.tab.o \
jv-lang.o \
jv-typeprint.o \
jv-valprint.o \
kcore-nbsd.o \
kod-cisco.o \
kod.o \
language.o \
linespec.o \
m2-exp.tab.o \
m2-lang.o \
m2-typeprint.o \
m2-valprint.o \
macrocmd.o \
macroexp.o \
macroscope.o \
macrotab.o \
main.o \
maint.o \
mdebugread.o \
mem-break.o \
memattr.o \
mi-cmd-break.o \
mi-cmd-disas.o \
mi-cmd-stack.o \
mi-cmd-var.o \
mi-cmds.o \
mi-console.o \
mi-getopt.o \
mi-main.o \
mi-out.o \
mi-parse.o \
minsyms.o \
mipsread.o \
nbsd-tdep.o \
nbsd-thread.o \
nbsd-proc.o \
nlmread.o \
objfiles.o \
os9kread.o \
osabi.o \
p-exp.tab.o \
p-lang.o \
p-typeprint.o \
p-valprint.o \
parse.o \
printcmd.o \
regcache.o \
remote-utils.o \
remote.o \
scm-exp.o \
scm-lang.o \
scm-valprint.o \
ser-pipe.o \
ser-tcp.o \
ser-unix.o \
serial.o \
signals.o \
solib-svr4.o \
solib.o \
source.o \
stabsread.o \
stack.o \
std-regs.o \
symfile.o \
symmisc.o \
symtab.o \
target.o \
thread.o \
top.o \
tracepoint.o \
typeprint.o \
ui-file.o \
ui-out.o \
utils.o \
valarith.o \
valops.o \
valprint.o \
values.o \
varobj.o \
version.o \
wrapper.o

G_SIM_OBJS=

G_BFD_CPPFLAGS=-DDEFAULT_VECTOR=bfd_elf32_i386_vec -DSELECT_VECS="&bfd_elf32_i386_vec,&i386netbsd_vec,&bfd_elf32_little_generic_vec,&bfd_elf32_big_generic_vec" -DSELECT_ARCHITECTURES="&bfd_i386_arch" -DHAVE_bfd_elf32_i386_vec -DHAVE_i386netbsd_vec -DHAVE_bfd_elf32_little_generic_vec -DHAVE_bfd_elf32_big_generic_vec
G_BFD_OBJS=\
aout32.o \
archive.o \
archive64.o \
archures.o \
bfd.o \
binary.o \
cache.o \
coffgen.o \
corefile.o \
cpu-i386.o \
dwarf1.o \
dwarf2.o \
elf-eh-frame.o \
elf-strtab.o \
elf.o \
elf32-gen.o \
elf32-i386.o \
elf32.o \
elflink.o \
format.o \
hash.o \
i386netbsd.o \
ihex.o \
init.o \
libbfd.o \
linker.o \
merge.o \
netbsd-core.o \
opncls.o \
reloc.o \
section.o \
srec.o \
stab-syms.o \
stabs.o \
syms.o \
targets.o \
tekhex.o

G_TUI_OBJS= \
tui-file.o \
tui-hooks.o \
tui-out.o \
tui.o \
tuiCommand.o \
tuiData.o \
tuiDataWin.o \
tuiDisassem.o \
tuiGeneralWin.o \
tuiIO.o \
tuiLayout.o \
tuiRegs.o \
tuiSource.o \
tuiSourceWin.o \
tuiStack.o \
tuiWin.o

G_OPCODES_OBJS=\
dis-buf.o \
i386-dis.o
