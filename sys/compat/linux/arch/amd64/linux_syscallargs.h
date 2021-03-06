/* $NetBSD: linux_syscallargs.h,v 1.16.6.1 2007/04/20 20:18:03 bouyer Exp $ */

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.15.6.1 2007/04/20 20:14:12 bouyer Exp
 */

#ifndef _LINUX_SYS_SYSCALLARGS_H_
#define	_LINUX_SYS_SYSCALLARGS_H_

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)							\
	union {								\
		register_t pad;						\
		struct { x datum; } le;					\
		struct { /* LINTED zero array dimension */		\
			int8_t pad[  /* CONSTCOND */			\
				(sizeof (register_t) < sizeof (x))	\
				? 0					\
				: sizeof (register_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct linux_sys_open_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct linux_sys_stat64_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_fstat64_args {
	syscallarg(int) fd;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_lstat64_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_mprotect_args {
	syscallarg(const void *) start;
	syscallarg(unsigned long) len;
	syscallarg(int) prot;
};

struct linux_sys_brk_args {
	syscallarg(char *) nsize;
};

struct linux_sys_rt_sigaction_args {
	syscallarg(int) signum;
	syscallarg(const struct linux_sigaction *) nsa;
	syscallarg(struct linux_sigaction *) osa;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(const linux_sigset_t *) set;
	syscallarg(linux_sigset_t *) oset;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(u_long) com;
	syscallarg(caddr_t) data;
};

struct linux_sys_pread_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(linux_off_t) offset;
};

struct linux_sys_pwrite_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(linux_off_t) offset;
};

struct linux_sys_access_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
};

struct linux_sys_pipe_args {
	syscallarg(int *) pfds;
};

struct linux_sys_select_args {
	syscallarg(int) nfds;
	syscallarg(fd_set *) readfds;
	syscallarg(fd_set *) writefds;
	syscallarg(fd_set *) exceptfds;
	syscallarg(struct timeval *) timeout;
};

struct linux_sys_mremap_args {
	syscallarg(void *) old_address;
	syscallarg(size_t) old_size;
	syscallarg(size_t) new_size;
	syscallarg(u_long) flags;
};

struct linux_sys_msync_args {
	syscallarg(caddr_t) addr;
	syscallarg(int) len;
	syscallarg(int) fl;
};
#ifdef SYSVSHM

struct linux_sys_shmget_args {
	syscallarg(key_t) key;
	syscallarg(size_t) size;
	syscallarg(int) shmflg;
};

struct linux_sys_shmctl_args {
	syscallarg(int) shmid;
	syscallarg(int) cmd;
	syscallarg(struct linux_shmid_ds *) buf;
};
#else
#endif

struct linux_sys_alarm_args {
	syscallarg(unsigned int) secs;
};

struct linux_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};

struct linux_sys_connect_args {
	syscallarg(int) s;
	syscallarg(const struct osockaddr *) name;
	syscallarg(unsigned int) namelen;
};

struct linux_sys_accept_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int *) anamelen;
};

struct linux_sys_sendto_args {
	syscallarg(int) s;
	syscallarg(void *) msg;
	syscallarg(int) len;
	syscallarg(int) flags;
	syscallarg(struct osockaddr *) to;
	syscallarg(int) tolen;
};

struct linux_sys_recvfrom_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(size_t) len;
	syscallarg(int) flags;
	syscallarg(struct osockaddr *) from;
	syscallarg(unsigned int *) fromlenaddr;
};

struct linux_sys_sendmsg_args {
	syscallarg(int) s;
	syscallarg(const struct msghdr *) msg;
	syscallarg(int) flags;
};

struct linux_sys_recvmsg_args {
	syscallarg(int) s;
	syscallarg(struct msghdr *) msg;
	syscallarg(int) flags;
};

struct linux_sys_bind_args {
	syscallarg(int) s;
	syscallarg(const struct osockaddr *) name;
	syscallarg(unsigned int) namelen;
};

struct linux_sys_getsockname_args {
	syscallarg(int) fdec;
	syscallarg(caddr_t) asa;
	syscallarg(int *) alen;
};

struct linux_sys_getpeername_args {
	syscallarg(int) fdes;
	syscallarg(struct sockaddr *) asa;
	syscallarg(unsigned int *) alen;
};

struct linux_sys_socketpair_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
	syscallarg(int *) rsv;
};

struct linux_sys_setsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(void *) optval;
	syscallarg(int) optlen;
};

struct linux_sys_getsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(void *) optval;
	syscallarg(int *) optlen;
};

struct linux_sys_clone_args {
	syscallarg(int) flags;
	syscallarg(void *) stack;
	syscallarg(void *) parent_tidptr;
	syscallarg(void *) child_tidptr;
};

struct linux_sys_execve_args {
	syscallarg(const char *) path;
	syscallarg(char **) argp;
	syscallarg(char **) envp;
};

struct linux_sys_wait4_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct rusage *) rusage;
};

struct linux_sys_kill_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
};

struct linux_sys_uname_args {
	syscallarg(struct linux_utsname *) up;
};
#ifdef SYSVSEM

struct linux_sys_semctl_args {
	syscallarg(int) semid;
	syscallarg(int) semnum;
	syscallarg(int) cmd;
	syscallarg(union linux_semun) arg;
};
#else
#endif
#ifdef SYSVSHM
#else
#endif
#ifdef SYSVMSG

struct linux_sys_msgctl_args {
	syscallarg(int) msqid;
	syscallarg(int) cmd;
	syscallarg(struct linux_msqid_ds *) buf;
};
#else
#endif

struct linux_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct linux_sys_fdatasync_args {
	syscallarg(int) fd;
};

struct linux_sys_truncate64_args {
	syscallarg(const char *) path;
	syscallarg(off_t) length;
};

struct linux_sys_ftruncate64_args {
	syscallarg(unsigned int) fd;
	syscallarg(off_t) length;
};

struct linux_sys_getdents_args {
	syscallarg(int) fd;
	syscallarg(struct linux_dirent *) dent;
	syscallarg(unsigned int) count;
};

struct linux_sys_chdir_args {
	syscallarg(const char *) path;
};

struct linux_sys_rename_args {
	syscallarg(const char *) from;
	syscallarg(const char *) to;
};

struct linux_sys_mkdir_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
};

struct linux_sys_rmdir_args {
	syscallarg(const char *) path;
};

struct linux_sys_creat_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
};

struct linux_sys_link_args {
	syscallarg(const char *) path;
	syscallarg(const char *) link;
};

struct linux_sys_unlink_args {
	syscallarg(const char *) path;
};

struct linux_sys_symlink_args {
	syscallarg(const char *) path;
	syscallarg(const char *) to;
};

struct linux_sys_readlink_args {
	syscallarg(const char *) name;
	syscallarg(char *) buf;
	syscallarg(int) count;
};

struct linux_sys_chmod_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
};

struct linux_sys_chown_args {
	syscallarg(const char *) path;
	syscallarg(uid_t) uid;
	syscallarg(gid_t) gid;
};

struct linux_sys_lchown_args {
	syscallarg(const char *) path;
	syscallarg(uid_t) uid;
	syscallarg(gid_t) gid;
};

struct linux_sys_gettimeofday_args {
	syscallarg(struct timeval *) tp;
	syscallarg(struct timezone *) tzp;
};

struct linux_sys_getrlimit_args {
	syscallarg(int) which;
	syscallarg(struct rlimit *) rlp;
};

struct linux_sys_sysinfo_args {
	syscallarg(struct linux_sysinfo *) arg;
};

struct linux_sys_times_args {
	syscallarg(struct times *) tms;
};

struct linux_sys_ptrace_args {
	syscallarg(long) request;
	syscallarg(long) pid;
	syscallarg(long) addr;
	syscallarg(long) data;
};

struct linux_sys_setresuid_args {
	syscallarg(uid_t) ruid;
	syscallarg(uid_t) euid;
	syscallarg(uid_t) suid;
};

struct linux_sys_getresuid_args {
	syscallarg(uid_t *) ruid;
	syscallarg(uid_t *) euid;
	syscallarg(uid_t *) suid;
};

struct linux_sys_setresgid_args {
	syscallarg(gid_t) rgid;
	syscallarg(gid_t) egid;
	syscallarg(gid_t) sgid;
};

struct linux_sys_getresgid_args {
	syscallarg(gid_t *) rgid;
	syscallarg(gid_t *) egid;
	syscallarg(gid_t *) sgid;
};

struct linux_sys_getpgid_args {
	syscallarg(int) pid;
};

struct linux_sys_setfsuid_args {
	syscallarg(uid_t) uid;
};

struct linux_sys_rt_sigpending_args {
	syscallarg(linux_sigset_t *) set;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_queueinfo_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
	syscallarg(siginfo_t *) uinfo;
};

struct linux_sys_rt_sigsuspend_args {
	syscallarg(linux_sigset_t *) unewset;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_sigaltstack_args {
	syscallarg(const struct linux_sigaltstack *) ss;
	syscallarg(struct linux_sigaltstack *) oss;
};

struct linux_sys_utime_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_utimbuf *) times;
};

struct linux_sys_mknod_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};
#ifdef EXEC_AOUT

struct linux_sys_uselib_args {
	syscallarg(const char *) path;
};
#else
#endif

struct linux_sys_personality_args {
	syscallarg(int) per;
};

struct linux_sys_statfs_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_statfs *) sp;
};

struct linux_sys_fstatfs_args {
	syscallarg(int) fd;
	syscallarg(struct linux_statfs *) sp;
};

struct linux_sys_getpriority_args {
	syscallarg(int) which;
	syscallarg(int) who;
};

struct linux_sys_sched_setparam_args {
	syscallarg(pid_t) pid;
	syscallarg(const struct linux_sched_param *) sp;
};

struct linux_sys_sched_getparam_args {
	syscallarg(pid_t) pid;
	syscallarg(struct linux_sched_param *) sp;
};

struct linux_sys_sched_setscheduler_args {
	syscallarg(pid_t) pid;
	syscallarg(int) policy;
	syscallarg(const struct linux_sched_param *) sp;
};

struct linux_sys_sched_getscheduler_args {
	syscallarg(pid_t) pid;
};

struct linux_sys_sched_get_priority_max_args {
	syscallarg(int) policy;
};

struct linux_sys_sched_get_priority_min_args {
	syscallarg(int) policy;
};

struct linux_sys_modify_ldt_args {
	syscallarg(int) func;
	syscallarg(void *) ptr;
	syscallarg(size_t) bytecount;
};

struct linux_sys___sysctl_args {
	syscallarg(struct linux___sysctl *) lsp;
};

struct linux_sys_arch_prctl_args {
	syscallarg(int) code;
	syscallarg(unsigned long) addr;
};

struct linux_sys_setrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct rlimit *) rlp;
};

struct linux_sys_settimeofday_args {
	syscallarg(struct timeval *) tp;
	syscallarg(struct timezone *) tzp;
};

struct linux_sys_swapon_args {
	syscallarg(char *) name;
};

struct linux_sys_swapoff_args {
	syscallarg(const char *) path;
};

struct linux_sys_reboot_args {
	syscallarg(int) magic1;
	syscallarg(int) magic2;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct linux_sys_setdomainname_args {
	syscallarg(char *) domainname;
	syscallarg(int) len;
};

struct linux_sys_iopl_args {
	syscallarg(int) level;
};

struct linux_sys_ioperm_args {
	syscallarg(unsigned int) lo;
	syscallarg(unsigned int) hi;
	syscallarg(int) val;
};

struct linux_sys_setxattr_args {
	syscallarg(char *) path;
	syscallarg(char *) name;
	syscallarg(void *) value;
	syscallarg(size_t) size;
	syscallarg(int) flags;
};

struct linux_sys_lsetxattr_args {
	syscallarg(char *) path;
	syscallarg(char *) name;
	syscallarg(void *) value;
	syscallarg(size_t) size;
	syscallarg(int) flags;
};

struct linux_sys_fsetxattr_args {
	syscallarg(int) fd;
	syscallarg(char *) name;
	syscallarg(void *) value;
	syscallarg(size_t) size;
	syscallarg(int) flags;
};

struct linux_sys_getxattr_args {
	syscallarg(char *) path;
	syscallarg(char *) name;
	syscallarg(void *) value;
	syscallarg(size_t) size;
};

struct linux_sys_lgetxattr_args {
	syscallarg(char *) path;
	syscallarg(char *) name;
	syscallarg(void *) value;
	syscallarg(size_t) size;
};

struct linux_sys_fgetxattr_args {
	syscallarg(int) fd;
	syscallarg(char *) name;
	syscallarg(void *) value;
	syscallarg(size_t) size;
};

struct linux_sys_listxattr_args {
	syscallarg(char *) path;
	syscallarg(char *) list;
	syscallarg(size_t) size;
};

struct linux_sys_llistxattr_args {
	syscallarg(char *) path;
	syscallarg(char *) list;
	syscallarg(size_t) size;
};

struct linux_sys_flistxattr_args {
	syscallarg(int) fd;
	syscallarg(char *) list;
	syscallarg(size_t) size;
};

struct linux_sys_removexattr_args {
	syscallarg(char *) path;
	syscallarg(char *) name;
};

struct linux_sys_lremovexattr_args {
	syscallarg(char *) path;
	syscallarg(char *) name;
};

struct linux_sys_fremovexattr_args {
	syscallarg(int) fd;
	syscallarg(char *) name;
};

struct linux_sys_tkill_args {
	syscallarg(int) tid;
	syscallarg(int) sig;
};

struct linux_sys_time_args {
	syscallarg(linux_time_t *) t;
};

struct linux_sys_futex_args {
	syscallarg(int *) uaddr;
	syscallarg(int) op;
	syscallarg(int) val;
	syscallarg(const struct timespec *) timeout;
	syscallarg(int *) uaddr2;
	syscallarg(int) val3;
};

struct linux_sys_sched_setaffinity_args {
	syscallarg(pid_t) pid;
	syscallarg(unsigned int) len;
	syscallarg(unsigned long *) mask;
};

struct linux_sys_sched_getaffinity_args {
	syscallarg(pid_t) pid;
	syscallarg(unsigned int) len;
	syscallarg(unsigned long *) mask;
};

struct linux_sys_getdents64_args {
	syscallarg(int) fd;
	syscallarg(struct linux_dirent64 *) dent;
	syscallarg(unsigned int) count;
};

struct linux_sys_set_tid_address_args {
	syscallarg(int *) tid;
};

struct linux_sys_clock_settime_args {
	syscallarg(clockid_t) which;
	syscallarg(struct linux_timespec *) tp;
};

struct linux_sys_clock_gettime_args {
	syscallarg(clockid_t) which;
	syscallarg(struct linux_timespec *) tp;
};

struct linux_sys_clock_getres_args {
	syscallarg(clockid_t) which;
	syscallarg(struct linux_timespec *) tp;
};

struct linux_sys_clock_nanosleep_args {
	syscallarg(clockid_t) which;
	syscallarg(int) flags;
	syscallarg(struct linux_timespec *) rqtp;
	syscallarg(struct linux_timespec *) rmtp;
};

struct linux_sys_exit_group_args {
	syscallarg(int) error_code;
};

struct linux_sys_tgkill_args {
	syscallarg(int) tgid;
	syscallarg(int) tid;
	syscallarg(int) sig;
};

/*
 * System call prototypes.
 */

int	sys_read(struct lwp *, void *, register_t *);

int	sys_write(struct lwp *, void *, register_t *);

int	linux_sys_open(struct lwp *, void *, register_t *);

int	sys_close(struct lwp *, void *, register_t *);

int	linux_sys_stat64(struct lwp *, void *, register_t *);

int	linux_sys_fstat64(struct lwp *, void *, register_t *);

int	linux_sys_lstat64(struct lwp *, void *, register_t *);

int	sys_poll(struct lwp *, void *, register_t *);

int	compat_43_sys_lseek(struct lwp *, void *, register_t *);

int	linux_sys_mmap(struct lwp *, void *, register_t *);

int	linux_sys_mprotect(struct lwp *, void *, register_t *);

int	sys_munmap(struct lwp *, void *, register_t *);

int	linux_sys_brk(struct lwp *, void *, register_t *);

int	linux_sys_rt_sigaction(struct lwp *, void *, register_t *);

int	linux_sys_rt_sigprocmask(struct lwp *, void *, register_t *);

int	linux_sys_rt_sigreturn(struct lwp *, void *, register_t *);

int	linux_sys_ioctl(struct lwp *, void *, register_t *);

int	linux_sys_pread(struct lwp *, void *, register_t *);

int	linux_sys_pwrite(struct lwp *, void *, register_t *);

int	sys_readv(struct lwp *, void *, register_t *);

int	sys_writev(struct lwp *, void *, register_t *);

int	linux_sys_access(struct lwp *, void *, register_t *);

int	linux_sys_pipe(struct lwp *, void *, register_t *);

int	linux_sys_select(struct lwp *, void *, register_t *);

int	linux_sys_sched_yield(struct lwp *, void *, register_t *);

int	linux_sys_mremap(struct lwp *, void *, register_t *);

int	linux_sys_msync(struct lwp *, void *, register_t *);

int	sys_mincore(struct lwp *, void *, register_t *);

int	sys_madvise(struct lwp *, void *, register_t *);

#ifdef SYSVSHM
int	linux_sys_shmget(struct lwp *, void *, register_t *);

int	sys_shmat(struct lwp *, void *, register_t *);

int	linux_sys_shmctl(struct lwp *, void *, register_t *);

#else
#endif
int	sys_dup(struct lwp *, void *, register_t *);

int	sys_dup2(struct lwp *, void *, register_t *);

int	linux_sys_pause(struct lwp *, void *, register_t *);

int	sys_nanosleep(struct lwp *, void *, register_t *);

int	sys_getitimer(struct lwp *, void *, register_t *);

int	linux_sys_alarm(struct lwp *, void *, register_t *);

int	sys_setitimer(struct lwp *, void *, register_t *);

int	linux_sys_getpid(struct lwp *, void *, register_t *);

int	linux_sys_socket(struct lwp *, void *, register_t *);

int	linux_sys_connect(struct lwp *, void *, register_t *);

int	linux_sys_accept(struct lwp *, void *, register_t *);

int	linux_sys_sendto(struct lwp *, void *, register_t *);

int	linux_sys_recvfrom(struct lwp *, void *, register_t *);

int	linux_sys_sendmsg(struct lwp *, void *, register_t *);

int	linux_sys_recvmsg(struct lwp *, void *, register_t *);

int	sys_shutdown(struct lwp *, void *, register_t *);

int	linux_sys_bind(struct lwp *, void *, register_t *);

int	sys_listen(struct lwp *, void *, register_t *);

int	linux_sys_getsockname(struct lwp *, void *, register_t *);

int	linux_sys_getpeername(struct lwp *, void *, register_t *);

int	linux_sys_socketpair(struct lwp *, void *, register_t *);

int	linux_sys_setsockopt(struct lwp *, void *, register_t *);

int	linux_sys_getsockopt(struct lwp *, void *, register_t *);

int	linux_sys_clone(struct lwp *, void *, register_t *);

int	sys_fork(struct lwp *, void *, register_t *);

int	sys___vfork14(struct lwp *, void *, register_t *);

int	linux_sys_execve(struct lwp *, void *, register_t *);

int	sys_exit(struct lwp *, void *, register_t *);

int	linux_sys_wait4(struct lwp *, void *, register_t *);

int	linux_sys_kill(struct lwp *, void *, register_t *);

int	linux_sys_uname(struct lwp *, void *, register_t *);

#ifdef SYSVSEM
int	sys_semget(struct lwp *, void *, register_t *);

int	sys_semop(struct lwp *, void *, register_t *);

int	linux_sys_semctl(struct lwp *, void *, register_t *);

#else
#endif
#ifdef SYSVSHM
int	sys_shmdt(struct lwp *, void *, register_t *);

#else
#endif
#ifdef SYSVMSG
int	sys_msgget(struct lwp *, void *, register_t *);

int	sys_msgsnd(struct lwp *, void *, register_t *);

int	sys_msgrcv(struct lwp *, void *, register_t *);

int	linux_sys_msgctl(struct lwp *, void *, register_t *);

#else
#endif
int	linux_sys_fcntl(struct lwp *, void *, register_t *);

int	sys_flock(struct lwp *, void *, register_t *);

int	sys_fsync(struct lwp *, void *, register_t *);

int	linux_sys_fdatasync(struct lwp *, void *, register_t *);

int	linux_sys_truncate64(struct lwp *, void *, register_t *);

int	linux_sys_ftruncate64(struct lwp *, void *, register_t *);

int	linux_sys_getdents(struct lwp *, void *, register_t *);

int	sys___getcwd(struct lwp *, void *, register_t *);

int	linux_sys_chdir(struct lwp *, void *, register_t *);

int	sys_fchdir(struct lwp *, void *, register_t *);

int	linux_sys_rename(struct lwp *, void *, register_t *);

int	linux_sys_mkdir(struct lwp *, void *, register_t *);

int	linux_sys_rmdir(struct lwp *, void *, register_t *);

int	linux_sys_creat(struct lwp *, void *, register_t *);

int	linux_sys_link(struct lwp *, void *, register_t *);

int	linux_sys_unlink(struct lwp *, void *, register_t *);

int	linux_sys_symlink(struct lwp *, void *, register_t *);

int	linux_sys_readlink(struct lwp *, void *, register_t *);

int	linux_sys_chmod(struct lwp *, void *, register_t *);

int	sys_fchmod(struct lwp *, void *, register_t *);

int	linux_sys_chown(struct lwp *, void *, register_t *);

int	sys___posix_fchown(struct lwp *, void *, register_t *);

int	linux_sys_lchown(struct lwp *, void *, register_t *);

int	sys_umask(struct lwp *, void *, register_t *);

int	linux_sys_gettimeofday(struct lwp *, void *, register_t *);

int	linux_sys_getrlimit(struct lwp *, void *, register_t *);

int	sys_getrusage(struct lwp *, void *, register_t *);

int	linux_sys_sysinfo(struct lwp *, void *, register_t *);

int	linux_sys_times(struct lwp *, void *, register_t *);

int	linux_sys_ptrace(struct lwp *, void *, register_t *);

int	sys_getuid(struct lwp *, void *, register_t *);

int	sys_getgid(struct lwp *, void *, register_t *);

int	sys_setuid(struct lwp *, void *, register_t *);

int	sys_setgid(struct lwp *, void *, register_t *);

int	sys_geteuid(struct lwp *, void *, register_t *);

int	sys_getegid(struct lwp *, void *, register_t *);

int	sys_setpgid(struct lwp *, void *, register_t *);

int	linux_sys_getppid(struct lwp *, void *, register_t *);

int	sys_getpgrp(struct lwp *, void *, register_t *);

int	sys_setsid(struct lwp *, void *, register_t *);

int	sys_setreuid(struct lwp *, void *, register_t *);

int	sys_setregid(struct lwp *, void *, register_t *);

int	sys_getgroups(struct lwp *, void *, register_t *);

int	sys_setgroups(struct lwp *, void *, register_t *);

int	linux_sys_setresuid(struct lwp *, void *, register_t *);

int	linux_sys_getresuid(struct lwp *, void *, register_t *);

int	linux_sys_setresgid(struct lwp *, void *, register_t *);

int	linux_sys_getresgid(struct lwp *, void *, register_t *);

int	linux_sys_getpgid(struct lwp *, void *, register_t *);

int	linux_sys_setfsuid(struct lwp *, void *, register_t *);

int	linux_sys_getfsuid(struct lwp *, void *, register_t *);

int	sys_getsid(struct lwp *, void *, register_t *);

int	linux_sys_rt_sigpending(struct lwp *, void *, register_t *);

int	linux_sys_rt_queueinfo(struct lwp *, void *, register_t *);

int	linux_sys_rt_sigsuspend(struct lwp *, void *, register_t *);

int	linux_sys_sigaltstack(struct lwp *, void *, register_t *);

int	linux_sys_utime(struct lwp *, void *, register_t *);

int	linux_sys_mknod(struct lwp *, void *, register_t *);

#ifdef EXEC_AOUT
int	linux_sys_uselib(struct lwp *, void *, register_t *);

#else
#endif
int	linux_sys_personality(struct lwp *, void *, register_t *);

int	linux_sys_statfs(struct lwp *, void *, register_t *);

int	linux_sys_fstatfs(struct lwp *, void *, register_t *);

int	linux_sys_getpriority(struct lwp *, void *, register_t *);

int	sys_setpriority(struct lwp *, void *, register_t *);

int	linux_sys_sched_setparam(struct lwp *, void *, register_t *);

int	linux_sys_sched_getparam(struct lwp *, void *, register_t *);

int	linux_sys_sched_setscheduler(struct lwp *, void *, register_t *);

int	linux_sys_sched_getscheduler(struct lwp *, void *, register_t *);

int	linux_sys_sched_get_priority_max(struct lwp *, void *, register_t *);

int	linux_sys_sched_get_priority_min(struct lwp *, void *, register_t *);

int	sys_mlock(struct lwp *, void *, register_t *);

int	sys_munlock(struct lwp *, void *, register_t *);

int	sys_mlockall(struct lwp *, void *, register_t *);

int	sys_munlockall(struct lwp *, void *, register_t *);

int	linux_sys_modify_ldt(struct lwp *, void *, register_t *);

int	linux_sys___sysctl(struct lwp *, void *, register_t *);

int	linux_sys_arch_prctl(struct lwp *, void *, register_t *);

int	linux_sys_setrlimit(struct lwp *, void *, register_t *);

int	sys_chroot(struct lwp *, void *, register_t *);

int	sys_sync(struct lwp *, void *, register_t *);

int	sys_acct(struct lwp *, void *, register_t *);

int	linux_sys_settimeofday(struct lwp *, void *, register_t *);

int	linux_sys_swapon(struct lwp *, void *, register_t *);

int	linux_sys_swapoff(struct lwp *, void *, register_t *);

int	linux_sys_reboot(struct lwp *, void *, register_t *);

int	compat_43_sys_sethostname(struct lwp *, void *, register_t *);

int	linux_sys_setdomainname(struct lwp *, void *, register_t *);

int	linux_sys_iopl(struct lwp *, void *, register_t *);

int	linux_sys_ioperm(struct lwp *, void *, register_t *);

int	linux_sys_gettid(struct lwp *, void *, register_t *);

int	linux_sys_setxattr(struct lwp *, void *, register_t *);

int	linux_sys_lsetxattr(struct lwp *, void *, register_t *);

int	linux_sys_fsetxattr(struct lwp *, void *, register_t *);

int	linux_sys_getxattr(struct lwp *, void *, register_t *);

int	linux_sys_lgetxattr(struct lwp *, void *, register_t *);

int	linux_sys_fgetxattr(struct lwp *, void *, register_t *);

int	linux_sys_listxattr(struct lwp *, void *, register_t *);

int	linux_sys_llistxattr(struct lwp *, void *, register_t *);

int	linux_sys_flistxattr(struct lwp *, void *, register_t *);

int	linux_sys_removexattr(struct lwp *, void *, register_t *);

int	linux_sys_lremovexattr(struct lwp *, void *, register_t *);

int	linux_sys_fremovexattr(struct lwp *, void *, register_t *);

int	linux_sys_tkill(struct lwp *, void *, register_t *);

int	linux_sys_time(struct lwp *, void *, register_t *);

int	linux_sys_futex(struct lwp *, void *, register_t *);

int	linux_sys_sched_setaffinity(struct lwp *, void *, register_t *);

int	linux_sys_sched_getaffinity(struct lwp *, void *, register_t *);

int	linux_sys_getdents64(struct lwp *, void *, register_t *);

int	linux_sys_set_tid_address(struct lwp *, void *, register_t *);

int	linux_sys_clock_settime(struct lwp *, void *, register_t *);

int	linux_sys_clock_gettime(struct lwp *, void *, register_t *);

int	linux_sys_clock_getres(struct lwp *, void *, register_t *);

int	linux_sys_clock_nanosleep(struct lwp *, void *, register_t *);

int	linux_sys_exit_group(struct lwp *, void *, register_t *);

int	linux_sys_tgkill(struct lwp *, void *, register_t *);

int	linux_sys_nosys(struct lwp *, void *, register_t *);

#endif /* _LINUX_SYS_SYSCALLARGS_H_ */
