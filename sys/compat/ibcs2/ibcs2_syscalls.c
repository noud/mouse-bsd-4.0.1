/* $NetBSD: ibcs2_syscalls.c,v 1.40 2005/12/11 12:20:03 christos Exp $ */

/*
 * System call names.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.30 2004/03/26 15:18:54 drochner Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_syscalls.c,v 1.40 2005/12/11 12:20:03 christos Exp $");

#if defined(_KERNEL_OPT)
#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_statfs.h>
#endif /* _KERNEL_OPT */

const char *const ibcs2_syscallnames[] = {
	"syscall",			/* 0 = syscall */
	"exit",			/* 1 = exit */
	"fork",			/* 2 = fork */
	"read",			/* 3 = read */
	"write",			/* 4 = write */
	"open",			/* 5 = open */
	"close",			/* 6 = close */
	"waitsys",			/* 7 = waitsys */
	"creat",			/* 8 = creat */
	"link",			/* 9 = link */
	"unlink",			/* 10 = unlink */
	"execv",			/* 11 = execv */
	"chdir",			/* 12 = chdir */
	"time",			/* 13 = time */
	"mknod",			/* 14 = mknod */
	"chmod",			/* 15 = chmod */
	"chown",			/* 16 = chown */
	"obreak",			/* 17 = obreak */
	"stat",			/* 18 = stat */
	"lseek",			/* 19 = lseek */
	"getpid_with_ppid",			/* 20 = getpid_with_ppid */
	"mount",			/* 21 = mount */
	"umount",			/* 22 = umount */
	"setuid",			/* 23 = setuid */
	"getuid_with_euid",			/* 24 = getuid_with_euid */
	"stime",			/* 25 = stime */
	"#26 (unimplemented ibcs2_ptrace)",		/* 26 = unimplemented ibcs2_ptrace */
	"alarm",			/* 27 = alarm */
	"fstat",			/* 28 = fstat */
	"pause",			/* 29 = pause */
	"utime",			/* 30 = utime */
	"#31 (unimplemented was stty)",		/* 31 = unimplemented was stty */
	"gtty",			/* 32 = gtty */
	"access",			/* 33 = access */
	"nice",			/* 34 = nice */
	"statfs",			/* 35 = statfs */
	"sync",			/* 36 = sync */
	"kill",			/* 37 = kill */
	"fstatfs",			/* 38 = fstatfs */
	"pgrpsys",			/* 39 = pgrpsys */
	"#40 (unimplemented ibcs2_xenix)",		/* 40 = unimplemented ibcs2_xenix */
	"dup",			/* 41 = dup */
	"pipe",			/* 42 = pipe */
	"times",			/* 43 = times */
	"#44 (unimplemented profil)",		/* 44 = unimplemented profil */
	"plock",			/* 45 = plock */
	"setgid",			/* 46 = setgid */
	"getgid_with_egid",			/* 47 = getgid_with_egid */
	"sigsys",			/* 48 = sigsys */
#ifdef SYSVMSG
	"msgsys",			/* 49 = msgsys */
#else
	"#49 (unimplemented msgsys)",		/* 49 = unimplemented msgsys */
#endif
	"sysmachine",			/* 50 = sysmachine */
	"#51 (unimplemented ibcs2_acct)",		/* 51 = unimplemented ibcs2_acct */
#ifdef SYSVSHM
	"shmsys",			/* 52 = shmsys */
#else
	"#52 (unimplemented shmsys)",		/* 52 = unimplemented shmsys */
#endif
#ifdef SYSVSEM
	"semsys",			/* 53 = semsys */
#else
	"#53 (unimplemented semsys)",		/* 53 = unimplemented semsys */
#endif
	"ioctl",			/* 54 = ioctl */
	"uadmin",			/* 55 = uadmin */
	"#56 (unimplemented)",		/* 56 = unimplemented */
	"utssys",			/* 57 = utssys */
	"fsync",			/* 58 = fsync */
	"execve",			/* 59 = execve */
	"umask",			/* 60 = umask */
	"chroot",			/* 61 = chroot */
	"fcntl",			/* 62 = fcntl */
	"ulimit",			/* 63 = ulimit */
	"#64 (unimplemented reserved for unix/pc)",		/* 64 = unimplemented reserved for unix/pc */
	"#65 (unimplemented reserved for unix/pc)",		/* 65 = unimplemented reserved for unix/pc */
	"#66 (unimplemented reserved for unix/pc)",		/* 66 = unimplemented reserved for unix/pc */
	"#67 (unimplemented reserved for unix/pc)",		/* 67 = unimplemented reserved for unix/pc */
	"#68 (unimplemented reserved for unix/pc)",		/* 68 = unimplemented reserved for unix/pc */
	"#69 (unimplemented reserved for unix/pc)",		/* 69 = unimplemented reserved for unix/pc */
	"#70 (obsolete rfs_advfs)",		/* 70 = obsolete rfs_advfs */
	"#71 (obsolete rfs_unadvfs)",		/* 71 = obsolete rfs_unadvfs */
	"#72 (obsolete rfs_rmount)",		/* 72 = obsolete rfs_rmount */
	"#73 (obsolete rfs_rumount)",		/* 73 = obsolete rfs_rumount */
	"#74 (obsolete rfs_rfstart)",		/* 74 = obsolete rfs_rfstart */
	"#75 (obsolete rfs_sigret)",		/* 75 = obsolete rfs_sigret */
	"#76 (obsolete rfs_rdebug)",		/* 76 = obsolete rfs_rdebug */
	"#77 (obsolete rfs_rfstop)",		/* 77 = obsolete rfs_rfstop */
	"#78 (unimplemented rfs_rfsys)",		/* 78 = unimplemented rfs_rfsys */
	"rmdir",			/* 79 = rmdir */
	"mkdir",			/* 80 = mkdir */
	"getdents",			/* 81 = getdents */
	"#82 (unimplemented)",		/* 82 = unimplemented */
	"#83 (unimplemented)",		/* 83 = unimplemented */
	"sysfs",			/* 84 = sysfs */
	"getmsg",			/* 85 = getmsg */
	"putmsg",			/* 86 = putmsg */
	"poll",			/* 87 = poll */
	"#88 (unimplemented)",		/* 88 = unimplemented */
	"#89 (unimplemented)",		/* 89 = unimplemented */
	"symlink",			/* 90 = symlink */
	"lstat",			/* 91 = lstat */
	"readlink",			/* 92 = readlink */
	"fchmod",			/* 93 = fchmod */
	"fchown",			/* 94 = fchown */
	"#95 (unimplemented)",		/* 95 = unimplemented */
	"sigreturn",			/* 96 = sigreturn */
	"sigaltstack",			/* 97 = sigaltstack */
	"#98 (unimplemented)",		/* 98 = unimplemented */
	"#99 (unimplemented)",		/* 99 = unimplemented */
	"#100 (unimplemented getcontext/setcontext/sigsetjmp)",		/* 100 = unimplemented getcontext/setcontext/sigsetjmp */
	"#101 (unimplemented)",		/* 101 = unimplemented */
	"#102 (unimplemented)",		/* 102 = unimplemented */
	"statvfs",			/* 103 = statvfs */
	"fstatvfs",			/* 104 = fstatvfs */
	"#105 (unimplemented)",		/* 105 = unimplemented */
	"#106 (unimplemented)",		/* 106 = unimplemented */
	"#107 (unimplemented waitid)",		/* 107 = unimplemented waitid */
	"#108 (unimplemented sigsendset)",		/* 108 = unimplemented sigsendset */
	"#109 (unimplemented)",		/* 109 = unimplemented */
	"#110 (unimplemented)",		/* 110 = unimplemented */
	"#111 (unimplemented)",		/* 111 = unimplemented */
	"#112 (unimplemented priocntl)",		/* 112 = unimplemented priocntl */
	"#113 (unimplemented)",		/* 113 = unimplemented */
	"#114 (unimplemented)",		/* 114 = unimplemented */
	"mmap",			/* 115 = mmap */
	"mprotect",			/* 116 = mprotect */
	"munmap",			/* 117 = munmap */
	"#118 (unimplemented)",		/* 118 = unimplemented */
	"#119 (unimplemented vfork)",		/* 119 = unimplemented vfork */
	"fchdir",			/* 120 = fchdir */
	"readv",			/* 121 = readv */
	"writev",			/* 122 = writev */
	"#123 (unimplemented xstat)",		/* 123 = unimplemented xstat */
	"#124 (unimplemented lxstat)",		/* 124 = unimplemented lxstat */
	"#125 (unimplemented fxstat)",		/* 125 = unimplemented fxstat */
	"#126 (unimplemented)",		/* 126 = unimplemented */
	"#127 (unimplemented)",		/* 127 = unimplemented */
	"#128 (unimplemented setrlimit)",		/* 128 = unimplemented setrlimit */
	"#129 (unimplemented getrlimit)",		/* 129 = unimplemented getrlimit */
	"#130 (unimplemented lchown)",		/* 130 = unimplemented lchown */
	"memcntl",			/* 131 = memcntl */
	"#132 (unimplemented getpmsg)",		/* 132 = unimplemented getpmsg */
	"#133 (unimplemented putpmsg)",		/* 133 = unimplemented putpmsg */
	"#134 (unimplemented)",		/* 134 = unimplemented */
	"#135 (unimplemented)",		/* 135 = unimplemented */
	"#136 (unimplemented setegid)",		/* 136 = unimplemented setegid */
	"#137 (unimplemented)",		/* 137 = unimplemented */
	"#138 (unimplemented adjtime)",		/* 138 = unimplemented adjtime */
	"#139 (unimplemented)",		/* 139 = unimplemented */
	"#140 (unimplemented)",		/* 140 = unimplemented */
	"#141 (unimplemented seteuid)",		/* 141 = unimplemented seteuid */
	"#142 (unimplemented)",		/* 142 = unimplemented */
	"#143 (unimplemented)",		/* 143 = unimplemented */
	"#144 (unimplemented)",		/* 144 = unimplemented */
	"#145 (unimplemented)",		/* 145 = unimplemented */
	"#146 (unimplemented)",		/* 146 = unimplemented */
	"#147 (unimplemented)",		/* 147 = unimplemented */
	"#148 (unimplemented)",		/* 148 = unimplemented */
	"#149 (unimplemented)",		/* 149 = unimplemented */
	"#150 (unimplemented)",		/* 150 = unimplemented */
	"#151 (unimplemented)",		/* 151 = unimplemented */
	"#152 (unimplemented)",		/* 152 = unimplemented */
	"#153 (unimplemented)",		/* 153 = unimplemented */
	"#154 (unimplemented)",		/* 154 = unimplemented */
	"#155 (unimplemented)",		/* 155 = unimplemented */
	"#156 (unimplemented)",		/* 156 = unimplemented */
	"#157 (unimplemented)",		/* 157 = unimplemented */
	"#158 (unimplemented)",		/* 158 = unimplemented */
	"#159 (unimplemented)",		/* 159 = unimplemented */
	"#160 (unimplemented)",		/* 160 = unimplemented */
	"#161 (unimplemented)",		/* 161 = unimplemented */
	"#162 (unimplemented)",		/* 162 = unimplemented */
	"#163 (unimplemented)",		/* 163 = unimplemented */
	"#164 (unimplemented)",		/* 164 = unimplemented */
	"#165 (unimplemented)",		/* 165 = unimplemented */
	"#166 (unimplemented)",		/* 166 = unimplemented */
	"#167 (unimplemented)",		/* 167 = unimplemented */
	"#168 (unimplemented)",		/* 168 = unimplemented */
	"#169 (unimplemented)",		/* 169 = unimplemented */
	"#170 (unimplemented)",		/* 170 = unimplemented */
	"gettimeofday",			/* 171 = gettimeofday */
	"settimeofday",			/* 172 = settimeofday */
	"#173 (unimplemented)",		/* 173 = unimplemented */
	"#174 (unimplemented)",		/* 174 = unimplemented */
	"#175 (unimplemented)",		/* 175 = unimplemented */
	"#176 (unimplemented)",		/* 176 = unimplemented */
	"#177 (unimplemented)",		/* 177 = unimplemented */
	"#178 (unimplemented)",		/* 178 = unimplemented */
	"#179 (unimplemented)",		/* 179 = unimplemented */
	"#180 (unimplemented)",		/* 180 = unimplemented */
	"#181 (unimplemented)",		/* 181 = unimplemented */
	"#182 (unimplemented)",		/* 182 = unimplemented */
	"#183 (unimplemented)",		/* 183 = unimplemented */
	"#184 (unimplemented)",		/* 184 = unimplemented */
	"#185 (unimplemented)",		/* 185 = unimplemented */
	"#186 (unimplemented)",		/* 186 = unimplemented */
	"#187 (unimplemented)",		/* 187 = unimplemented */
	"#188 (unimplemented)",		/* 188 = unimplemented */
	"#189 (unimplemented)",		/* 189 = unimplemented */
	"#190 (unimplemented)",		/* 190 = unimplemented */
	"truncate",			/* 191 = truncate */
	"ftruncate",			/* 192 = ftruncate */
	"#193 (unimplemented)",		/* 193 = unimplemented */
	"#194 (unimplemented)",		/* 194 = unimplemented */
	"#195 (unimplemented)",		/* 195 = unimplemented */
	"#196 (unimplemented)",		/* 196 = unimplemented */
	"#197 (unimplemented)",		/* 197 = unimplemented */
	"#198 (unimplemented)",		/* 198 = unimplemented */
	"#199 (unimplemented)",		/* 199 = unimplemented */
	"#200 (unimplemented)",		/* 200 = unimplemented */
	"locking",			/* 201 = locking */
	"#202 (unimplemented xenix_creatsem)",		/* 202 = unimplemented xenix_creatsem */
	"#203 (unimplemented xenix_opensem)",		/* 203 = unimplemented xenix_opensem */
	"#204 (unimplemented xenix_sigsem)",		/* 204 = unimplemented xenix_sigsem */
	"#205 (unimplemented xenix_waitsem)",		/* 205 = unimplemented xenix_waitsem */
	"#206 (unimplemented xenix_nbwaitsem)",		/* 206 = unimplemented xenix_nbwaitsem */
	"rdchk",			/* 207 = rdchk */
	"#208 (unimplemented)",		/* 208 = unimplemented */
	"#209 (unimplemented)",		/* 209 = unimplemented */
	"chsize",			/* 210 = chsize */
	"ftime",			/* 211 = ftime */
	"nap",			/* 212 = nap */
	"#213 (unimplemented xenix_sdget)",		/* 213 = unimplemented xenix_sdget */
	"#214 (unimplemented xenix_sdfree)",		/* 214 = unimplemented xenix_sdfree */
	"#215 (unimplemented xenix_sdenter)",		/* 215 = unimplemented xenix_sdenter */
	"#216 (unimplemented xenix_sdleave)",		/* 216 = unimplemented xenix_sdleave */
	"#217 (unimplemented xenix_sdgetv)",		/* 217 = unimplemented xenix_sdgetv */
	"#218 (unimplemented xenix_sdwaitv)",		/* 218 = unimplemented xenix_sdwaitv */
	"#219 (unimplemented)",		/* 219 = unimplemented */
	"#220 (unimplemented)",		/* 220 = unimplemented */
	"#221 (unimplemented)",		/* 221 = unimplemented */
	"#222 (unimplemented)",		/* 222 = unimplemented */
	"#223 (unimplemented)",		/* 223 = unimplemented */
	"#224 (unimplemented)",		/* 224 = unimplemented */
	"#225 (unimplemented)",		/* 225 = unimplemented */
	"#226 (unimplemented)",		/* 226 = unimplemented */
	"#227 (unimplemented)",		/* 227 = unimplemented */
	"#228 (unimplemented)",		/* 228 = unimplemented */
	"#229 (unimplemented)",		/* 229 = unimplemented */
	"#230 (unimplemented)",		/* 230 = unimplemented */
	"#231 (unimplemented)",		/* 231 = unimplemented */
	"#232 (unimplemented xenix_proctl)",		/* 232 = unimplemented xenix_proctl */
	"#233 (unimplemented xenix_execseg)",		/* 233 = unimplemented xenix_execseg */
	"#234 (unimplemented xenix_unexecseg)",		/* 234 = unimplemented xenix_unexecseg */
	"#235 (unimplemented)",		/* 235 = unimplemented */
	"select",			/* 236 = select */
	"eaccess",			/* 237 = eaccess */
	"#238 (unimplemented xenix_paccess)",		/* 238 = unimplemented xenix_paccess */
	"sigaction",			/* 239 = sigaction */
	"sigprocmask",			/* 240 = sigprocmask */
	"sigpending",			/* 241 = sigpending */
	"sigsuspend",			/* 242 = sigsuspend */
	"getgroups",			/* 243 = getgroups */
	"setgroups",			/* 244 = setgroups */
	"sysconf",			/* 245 = sysconf */
	"pathconf",			/* 246 = pathconf */
	"fpathconf",			/* 247 = fpathconf */
	"rename",			/* 248 = rename */
	"#249 (unimplemented)",		/* 249 = unimplemented */
	"scoinfo",			/* 250 = scoinfo */
	"#251 (unimplemented)",		/* 251 = unimplemented */
	"#252 (unimplemented)",		/* 252 = unimplemented */
	"#253 (unimplemented)",		/* 253 = unimplemented */
	"#254 (unimplemented)",		/* 254 = unimplemented */
	"#255 (unimplemented getitimer)",		/* 255 = unimplemented getitimer */
	"#256 (unimplemented setitimer)",		/* 256 = unimplemented setitimer */
	"#257 (unimplemented)",		/* 257 = unimplemented */
	"#258 (unimplemented setreuid)",		/* 258 = unimplemented setreuid */
	"#259 (unimplemented setregid)",		/* 259 = unimplemented setregid */
};
