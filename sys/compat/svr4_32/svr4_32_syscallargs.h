/* $NetBSD: svr4_32_syscallargs.h,v 1.13 2006/05/29 09:46:54 drochner Exp $ */

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.9 2006/05/29 09:44:51 drochner Exp
 */

#ifndef _SVR4_32_SYS_SYSCALLARGS_H_
#define	_SVR4_32_SYS_SYSCALLARGS_H_

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)							\
	union {								\
		register32_t pad;						\
		struct { x datum; } le;					\
		struct { /* LINTED zero array dimension */		\
			int8_t pad[  /* CONSTCOND */			\
				(sizeof (register32_t) < sizeof (x))	\
				? 0					\
				: sizeof (register32_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct svr4_32_sys_open_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct svr4_32_sys_wait_args {
	syscallarg(netbsd32_intp) status;
};

struct svr4_32_sys_creat_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(int) mode;
};

struct svr4_32_sys_execv_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(netbsd32_charpp) argp;
};

struct svr4_32_sys_time_args {
	syscallarg(svr4_32_time_tp) t;
};

struct svr4_32_sys_mknod_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct svr4_32_sys_break_args {
	syscallarg(netbsd32_caddr_t) nsize;
};

struct svr4_32_sys_stat_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_statp) ub;
};

struct svr4_32_sys_alarm_args {
	syscallarg(unsigned) sec;
};

struct svr4_32_sys_fstat_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_statp) sb;
};

struct svr4_32_sys_utime_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_utimbufp) ubuf;
};

struct svr4_32_sys_access_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(int) flags;
};

struct svr4_32_sys_nice_args {
	syscallarg(int) prio;
};

struct svr4_32_sys_kill_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
};

struct svr4_32_sys_pgrpsys_args {
	syscallarg(int) cmd;
	syscallarg(int) pid;
	syscallarg(int) pgid;
};

struct svr4_32_sys_times_args {
	syscallarg(svr4_32_tms_tp) tp;
};

struct svr4_32_sys_signal_args {
	syscallarg(int) signum;
	syscallarg(svr4_sig_t) handler;
};
#ifdef SYSVMSG

struct svr4_32_sys_msgsys_args {
	syscallarg(int) what;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
	syscallarg(int) a5;
};
#else
#endif

struct svr4_32_sys_sysarch_args {
	syscallarg(int) op;
	syscallarg(netbsd32_voidp) a1;
};
#ifdef SYSVSHM

struct svr4_32_sys_shmsys_args {
	syscallarg(int) what;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
};
#else
#endif
#ifdef SYSVSEM

struct svr4_32_sys_semsys_args {
	syscallarg(int) what;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
	syscallarg(int) a5;
};
#else
#endif

struct svr4_32_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(netbsd32_u_long) com;
	syscallarg(netbsd32_caddr_t) data;
};

struct svr4_32_sys_utssys_args {
	syscallarg(netbsd32_voidp) a1;
	syscallarg(netbsd32_voidp) a2;
	syscallarg(int) sel;
	syscallarg(netbsd32_voidp) a3;
};

struct svr4_32_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(netbsd32_charp) arg;
};

struct svr4_32_sys_ulimit_args {
	syscallarg(int) cmd;
	syscallarg(netbsd32_long) newlimit;
};

struct svr4_32_sys_getdents_args {
	syscallarg(int) fd;
	syscallarg(netbsd32_charp) buf;
	syscallarg(int) nbytes;
};

struct svr4_32_sys_getmsg_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_strbuf_tp) ctl;
	syscallarg(svr4_32_strbuf_tp) dat;
	syscallarg(netbsd32_intp) flags;
};

struct svr4_32_sys_putmsg_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_strbuf_tp) ctl;
	syscallarg(svr4_32_strbuf_tp) dat;
	syscallarg(int) flags;
};

struct svr4_32_sys_lstat_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_stat_tp) ub;
};

struct svr4_32_sys_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(const svr4_32_sigset_tp) set;
	syscallarg(svr4_32_sigset_tp) oset;
};

struct svr4_32_sys_sigsuspend_args {
	syscallarg(const svr4_32_sigset_tp) set;
};

struct svr4_32_sys_sigaltstack_args {
	syscallarg(const svr4_32_sigaltstack_tp) nss;
	syscallarg(svr4_32_sigaltstack_tp) oss;
};

struct svr4_32_sys_sigaction_args {
	syscallarg(int) signum;
	syscallarg(const svr4_32_sigaction_tp) nsa;
	syscallarg(svr4_32_sigaction_tp) osa;
};

struct svr4_32_sys_sigpending_args {
	syscallarg(int) what;
	syscallarg(svr4_32_sigset_tp) set;
};

struct svr4_32_sys_context_args {
	syscallarg(int) func;
	syscallarg(svr4_32_ucontext_tp) uc;
};

struct svr4_32_sys_statvfs_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_statvfs_tp) fs;
};

struct svr4_32_sys_fstatvfs_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_statvfs_tp) fs;
};

struct svr4_32_sys_waitsys_args {
	syscallarg(int) grp;
	syscallarg(int) id;
	syscallarg(svr4_32_siginfo_tp) info;
	syscallarg(int) options;
};

struct svr4_32_sys_hrtsys_args {
	syscallarg(int) cmd;
	syscallarg(int) fun;
	syscallarg(int) sub;
	syscallarg(netbsd32_voidp) rv1;
	syscallarg(netbsd32_voidp) rv2;
};

struct svr4_32_sys_pathconf_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(int) name;
};

struct svr4_32_sys_mmap_args {
	syscallarg(netbsd32_voidp) addr;
	syscallarg(svr4_32_size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(svr4_32_off_t) pos;
};

struct svr4_32_sys_fpathconf_args {
	syscallarg(int) fd;
	syscallarg(int) name;
};

struct svr4_32_sys_xstat_args {
	syscallarg(int) two;
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_xstat_tp) ub;
};

struct svr4_32_sys_lxstat_args {
	syscallarg(int) two;
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_xstat_tp) ub;
};

struct svr4_32_sys_fxstat_args {
	syscallarg(int) two;
	syscallarg(int) fd;
	syscallarg(svr4_32_xstat_tp) sb;
};

struct svr4_32_sys_xmknod_args {
	syscallarg(int) two;
	syscallarg(netbsd32_charp) path;
	syscallarg(svr4_32_mode_t) mode;
	syscallarg(svr4_dev_t) dev;
};

struct svr4_32_sys_setrlimit_args {
	syscallarg(int) which;
	syscallarg(const svr4_32_rlimit_tp) rlp;
};

struct svr4_32_sys_getrlimit_args {
	syscallarg(int) which;
	syscallarg(svr4_32_rlimit_tp) rlp;
};

struct svr4_32_sys_memcntl_args {
	syscallarg(netbsd32_voidp) addr;
	syscallarg(svr4_32_size_t) len;
	syscallarg(int) cmd;
	syscallarg(netbsd32_voidp) arg;
	syscallarg(int) attr;
	syscallarg(int) mask;
};

struct svr4_32_sys_uname_args {
	syscallarg(svr4_32_utsnamep) name;
	syscallarg(int) dummy;
};

struct svr4_32_sys_sysconfig_args {
	syscallarg(int) name;
};

struct svr4_32_sys_systeminfo_args {
	syscallarg(int) what;
	syscallarg(netbsd32_charp) buf;
	syscallarg(netbsd32_long) len;
};

struct svr4_32_sys__lwp_info_args {
	syscallarg(svr4_32_lwpinfop) lwpinfo;
};

struct svr4_32_sys_utimes_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(netbsd32_timevalp_t) tptr;
};

struct svr4_32_sys_gettimeofday_args {
	syscallarg(netbsd32_timevalp_t) tp;
};

struct svr4_32_sys__lwp_create_args {
	syscallarg(svr4_32_ucontext_tp) uc;
	syscallarg(netbsd32_u_long) flags;
	syscallarg(svr4_32_lwpid_tp) lwpid;
};

struct svr4_32_sys__lwp_suspend_args {
	syscallarg(svr4_lwpid_t) lwpid;
};

struct svr4_32_sys__lwp_continue_args {
	syscallarg(svr4_lwpid_t) lwpid;
};

struct svr4_32_sys__lwp_kill_args {
	syscallarg(svr4_lwpid_t) lwpid;
	syscallarg(int) signum;
};

struct svr4_32_sys__lwp_setprivate_args {
	syscallarg(netbsd32_voidp) buffer;
};

struct svr4_32_sys__lwp_wait_args {
	syscallarg(svr4_lwpid_t) wait_for;
	syscallarg(svr4_32_lwpid_tp) departed_lwp;
};

struct svr4_32_sys_pread_args {
	syscallarg(int) fd;
	syscallarg(netbsd32_voidp) buf;
	syscallarg(netbsd32_size_t) nbyte;
	syscallarg(svr4_32_off_t) off;
};

struct svr4_32_sys_pwrite_args {
	syscallarg(int) fd;
	syscallarg(const netbsd32_voidp) buf;
	syscallarg(netbsd32_size_t) nbyte;
	syscallarg(svr4_32_off_t) off;
};

struct svr4_32_sys_llseek_args {
	syscallarg(int) fd;
	syscallarg(netbsd32_long) offset1;
	syscallarg(netbsd32_long) offset2;
	syscallarg(int) whence;
};

struct svr4_32_sys_acl_args {
	syscallarg(netbsd32_charp) path;
	syscallarg(int) cmd;
	syscallarg(int) num;
	syscallarg(svr4_32_aclent_tp) buf;
};

struct svr4_32_sys_auditsys_args {
	syscallarg(int) code;
	syscallarg(int) a1;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
	syscallarg(int) a5;
};

struct svr4_32_sys_facl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(int) num;
	syscallarg(svr4_32_aclent_tp) buf;
};

struct svr4_32_sys_schedctl_args {
	syscallarg(unsigned int) x;
	syscallarg(int) y;
	syscallarg(void **) z;
};

struct svr4_32_sys_resolvepath_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(netbsd32_charp) buf;
	syscallarg(netbsd32_size_t) bufsiz;
};

struct svr4_32_sys_getdents64_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_dirent64_tp) dp;
	syscallarg(int) nbytes;
};

struct svr4_32_sys_mmap64_args {
	syscallarg(netbsd32_voidp) addr;
	syscallarg(svr4_32_size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(svr4_32_off64_t) pos;
};

struct svr4_32_sys_stat64_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_stat64_tp) sb;
};

struct svr4_32_sys_lstat64_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_stat64_tp) sb;
};

struct svr4_32_sys_fstat64_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_stat64_tp) sb;
};

struct svr4_32_sys_statvfs64_args {
	syscallarg(const netbsd32_charp) path;
	syscallarg(svr4_32_statvfs64_tp) fs;
};

struct svr4_32_sys_fstatvfs64_args {
	syscallarg(int) fd;
	syscallarg(svr4_32_statvfs64_tp) fs;
};

struct svr4_32_sys_setrlimit64_args {
	syscallarg(int) which;
	syscallarg(const svr4_32_rlimit64_tp) rlp;
};

struct svr4_32_sys_getrlimit64_args {
	syscallarg(int) which;
	syscallarg(svr4_32_rlimit64_tp) rlp;
};

struct svr4_32_sys_pread64_args {
	syscallarg(int) fd;
	syscallarg(netbsd32_voidp) buf;
	syscallarg(netbsd32_size_t) nbyte;
	syscallarg(svr4_32_off64_t) off;
};

struct svr4_32_sys_pwrite64_args {
	syscallarg(int) fd;
	syscallarg(const netbsd32_voidp) buf;
	syscallarg(netbsd32_size_t) nbyte;
	syscallarg(svr4_32_off64_t) off;
};

struct svr4_32_sys_creat64_args {
	syscallarg(netbsd32_charp) path;
	syscallarg(int) mode;
};

struct svr4_32_sys_open64_args {
	syscallarg(netbsd32_charp) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct svr4_32_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};
#if defined(NTP) || !defined(_KERNEL)
#else
#endif

/*
 * System call prototypes.
 */

int	sys_nosys(struct lwp *, void *, register_t *);

int	netbsd32_exit(struct lwp *, void *, register_t *);

int	sys_fork(struct lwp *, void *, register_t *);

int	netbsd32_read(struct lwp *, void *, register_t *);

int	netbsd32_write(struct lwp *, void *, register_t *);

int	svr4_32_sys_open(struct lwp *, void *, register_t *);

int	netbsd32_close(struct lwp *, void *, register_t *);

int	svr4_32_sys_wait(struct lwp *, void *, register_t *);

int	svr4_32_sys_creat(struct lwp *, void *, register_t *);

int	netbsd32_link(struct lwp *, void *, register_t *);

int	netbsd32_unlink(struct lwp *, void *, register_t *);

int	svr4_32_sys_execv(struct lwp *, void *, register_t *);

int	netbsd32_chdir(struct lwp *, void *, register_t *);

int	svr4_32_sys_time(struct lwp *, void *, register_t *);

int	svr4_32_sys_mknod(struct lwp *, void *, register_t *);

int	netbsd32_chmod(struct lwp *, void *, register_t *);

int	netbsd32___posix_chown(struct lwp *, void *, register_t *);

int	svr4_32_sys_break(struct lwp *, void *, register_t *);

int	svr4_32_sys_stat(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_olseek(struct lwp *, void *, register_t *);

int	sys_getpid(struct lwp *, void *, register_t *);

int	netbsd32_setuid(struct lwp *, void *, register_t *);

int	sys_getuid_with_euid(struct lwp *, void *, register_t *);

int	svr4_32_sys_alarm(struct lwp *, void *, register_t *);

int	svr4_32_sys_fstat(struct lwp *, void *, register_t *);

int	svr4_32_sys_pause(struct lwp *, void *, register_t *);

int	svr4_32_sys_utime(struct lwp *, void *, register_t *);

int	svr4_32_sys_access(struct lwp *, void *, register_t *);

int	svr4_32_sys_nice(struct lwp *, void *, register_t *);

int	sys_sync(struct lwp *, void *, register_t *);

int	svr4_32_sys_kill(struct lwp *, void *, register_t *);

int	svr4_32_sys_pgrpsys(struct lwp *, void *, register_t *);

int	netbsd32_dup(struct lwp *, void *, register_t *);

int	sys_pipe(struct lwp *, void *, register_t *);

int	svr4_32_sys_times(struct lwp *, void *, register_t *);

int	netbsd32_setgid(struct lwp *, void *, register_t *);

int	sys_getgid_with_egid(struct lwp *, void *, register_t *);

int	svr4_32_sys_signal(struct lwp *, void *, register_t *);

#ifdef SYSVMSG
int	svr4_32_sys_msgsys(struct lwp *, void *, register_t *);

#else
#endif
int	svr4_32_sys_sysarch(struct lwp *, void *, register_t *);

#ifdef SYSVSHM
int	svr4_32_sys_shmsys(struct lwp *, void *, register_t *);

#else
#endif
#ifdef SYSVSEM
int	svr4_32_sys_semsys(struct lwp *, void *, register_t *);

#else
#endif
int	svr4_32_sys_ioctl(struct lwp *, void *, register_t *);

int	svr4_32_sys_utssys(struct lwp *, void *, register_t *);

int	netbsd32_fsync(struct lwp *, void *, register_t *);

int	netbsd32_execve(struct lwp *, void *, register_t *);

int	netbsd32_umask(struct lwp *, void *, register_t *);

int	netbsd32_chroot(struct lwp *, void *, register_t *);

int	svr4_32_sys_fcntl(struct lwp *, void *, register_t *);

int	svr4_32_sys_ulimit(struct lwp *, void *, register_t *);

int	netbsd32_rmdir(struct lwp *, void *, register_t *);

int	netbsd32_mkdir(struct lwp *, void *, register_t *);

int	svr4_32_sys_getdents(struct lwp *, void *, register_t *);

int	svr4_32_sys_getmsg(struct lwp *, void *, register_t *);

int	svr4_32_sys_putmsg(struct lwp *, void *, register_t *);

int	netbsd32_poll(struct lwp *, void *, register_t *);

int	svr4_32_sys_lstat(struct lwp *, void *, register_t *);

int	netbsd32_symlink(struct lwp *, void *, register_t *);

int	netbsd32_readlink(struct lwp *, void *, register_t *);

int	netbsd32_getgroups(struct lwp *, void *, register_t *);

int	netbsd32_setgroups(struct lwp *, void *, register_t *);

int	netbsd32_fchmod(struct lwp *, void *, register_t *);

int	netbsd32___posix_fchown(struct lwp *, void *, register_t *);

int	svr4_32_sys_sigprocmask(struct lwp *, void *, register_t *);

int	svr4_32_sys_sigsuspend(struct lwp *, void *, register_t *);

int	svr4_32_sys_sigaltstack(struct lwp *, void *, register_t *);

int	svr4_32_sys_sigaction(struct lwp *, void *, register_t *);

int	svr4_32_sys_sigpending(struct lwp *, void *, register_t *);

int	svr4_32_sys_context(struct lwp *, void *, register_t *);

int	svr4_32_sys_statvfs(struct lwp *, void *, register_t *);

int	svr4_32_sys_fstatvfs(struct lwp *, void *, register_t *);

int	svr4_32_sys_waitsys(struct lwp *, void *, register_t *);

int	svr4_32_sys_hrtsys(struct lwp *, void *, register_t *);

int	svr4_32_sys_pathconf(struct lwp *, void *, register_t *);

int	svr4_32_sys_mmap(struct lwp *, void *, register_t *);

int	netbsd32_mprotect(struct lwp *, void *, register_t *);

int	netbsd32_munmap(struct lwp *, void *, register_t *);

int	svr4_32_sys_fpathconf(struct lwp *, void *, register_t *);

int	sys_vfork(struct lwp *, void *, register_t *);

int	netbsd32_fchdir(struct lwp *, void *, register_t *);

int	netbsd32_readv(struct lwp *, void *, register_t *);

int	netbsd32_writev(struct lwp *, void *, register_t *);

int	svr4_32_sys_xstat(struct lwp *, void *, register_t *);

int	svr4_32_sys_lxstat(struct lwp *, void *, register_t *);

int	svr4_32_sys_fxstat(struct lwp *, void *, register_t *);

int	svr4_32_sys_xmknod(struct lwp *, void *, register_t *);

int	svr4_32_sys_setrlimit(struct lwp *, void *, register_t *);

int	svr4_32_sys_getrlimit(struct lwp *, void *, register_t *);

int	netbsd32_lchown(struct lwp *, void *, register_t *);

int	svr4_32_sys_memcntl(struct lwp *, void *, register_t *);

int	netbsd32___posix_rename(struct lwp *, void *, register_t *);

int	svr4_32_sys_uname(struct lwp *, void *, register_t *);

int	netbsd32_setegid(struct lwp *, void *, register_t *);

int	svr4_32_sys_sysconfig(struct lwp *, void *, register_t *);

int	netbsd32_adjtime(struct lwp *, void *, register_t *);

int	svr4_32_sys_systeminfo(struct lwp *, void *, register_t *);

int	netbsd32_seteuid(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_info(struct lwp *, void *, register_t *);

int	netbsd32_fchroot(struct lwp *, void *, register_t *);

int	svr4_32_sys_utimes(struct lwp *, void *, register_t *);

int	svr4_32_sys_vhangup(struct lwp *, void *, register_t *);

int	svr4_32_sys_gettimeofday(struct lwp *, void *, register_t *);

int	netbsd32_getitimer(struct lwp *, void *, register_t *);

int	netbsd32_setitimer(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_create(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_exit(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_suspend(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_continue(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_kill(struct lwp *, void *, register_t *);

int	svr4_sys__lwp_self(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_getprivate(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_setprivate(struct lwp *, void *, register_t *);

int	svr4_32_sys__lwp_wait(struct lwp *, void *, register_t *);

int	svr4_32_sys_pread(struct lwp *, void *, register_t *);

int	svr4_32_sys_pwrite(struct lwp *, void *, register_t *);

int	svr4_32_sys_llseek(struct lwp *, void *, register_t *);

int	svr4_32_sys_acl(struct lwp *, void *, register_t *);

int	svr4_32_sys_auditsys(struct lwp *, void *, register_t *);

int	netbsd32_nanosleep(struct lwp *, void *, register_t *);

int	svr4_32_sys_facl(struct lwp *, void *, register_t *);

int	netbsd32_setreuid(struct lwp *, void *, register_t *);

int	netbsd32_setregid(struct lwp *, void *, register_t *);

int	svr4_32_sys_schedctl(struct lwp *, void *, register_t *);

int	svr4_32_sys_resolvepath(struct lwp *, void *, register_t *);

int	svr4_32_sys_getdents64(struct lwp *, void *, register_t *);

int	svr4_32_sys_mmap64(struct lwp *, void *, register_t *);

int	svr4_32_sys_stat64(struct lwp *, void *, register_t *);

int	svr4_32_sys_lstat64(struct lwp *, void *, register_t *);

int	svr4_32_sys_fstat64(struct lwp *, void *, register_t *);

int	svr4_32_sys_statvfs64(struct lwp *, void *, register_t *);

int	svr4_32_sys_fstatvfs64(struct lwp *, void *, register_t *);

int	svr4_32_sys_setrlimit64(struct lwp *, void *, register_t *);

int	svr4_32_sys_getrlimit64(struct lwp *, void *, register_t *);

int	svr4_32_sys_pread64(struct lwp *, void *, register_t *);

int	svr4_32_sys_pwrite64(struct lwp *, void *, register_t *);

int	svr4_32_sys_creat64(struct lwp *, void *, register_t *);

int	svr4_32_sys_open64(struct lwp *, void *, register_t *);

int	svr4_32_sys_socket(struct lwp *, void *, register_t *);

int	netbsd32_socketpair(struct lwp *, void *, register_t *);

int	netbsd32_bind(struct lwp *, void *, register_t *);

int	netbsd32_listen(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_oaccept(struct lwp *, void *, register_t *);

int	netbsd32_connect(struct lwp *, void *, register_t *);

int	netbsd32_shutdown(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_orecv(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_orecvfrom(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_orecvmsg(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_osend(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_osendmsg(struct lwp *, void *, register_t *);

int	netbsd32_sendto(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_ogetpeername(struct lwp *, void *, register_t *);

int	compat_43_netbsd32_ogetsockname(struct lwp *, void *, register_t *);

int	netbsd32_getsockopt(struct lwp *, void *, register_t *);

int	netbsd32_setsockopt(struct lwp *, void *, register_t *);

#if defined(NTP) || !defined(_KERNEL)
int	netbsd32_ntp_adjtime(struct lwp *, void *, register_t *);

#else
#endif
#endif /* _SVR4_32_SYS_SYSCALLARGS_H_ */
