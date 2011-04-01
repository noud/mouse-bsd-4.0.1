#! /bin/sh
# Generate nltvals.def, a file that describes various newlib/libgloss
# target values used by the host/target interface.
#
# Syntax: /bin/sh gennltvals.sh shell srcroot cpp

shell=$1
srcroot=$2
cpp=$3

srccom=$srcroot/sim/common

echo '/* Newlib/libgloss macro values needed by remote target support.  */'
echo '/* This file is machine generated by gennltvals.sh.  */'

$shell ${srccom}/gentvals.sh "" errno ${srcroot}/newlib/libc/include \
	"errno.h sys/errno.h" 'E[A-Z0-9]*' "${cpp}"

$shell ${srccom}/gentvals.sh "" signal ${srcroot}/newlib/libc/include \
	"signal.h sys/signal.h" 'SIG[A-Z0-9]*' "${cpp}"

$shell ${srccom}/gentvals.sh "" open ${srcroot}/newlib/libc/include \
	"fcntl.h sys/fcntl.h" 'O_[A-Z0-9]*' "${cpp}"

# Unfortunately, each newlib/libgloss port has seen fit to define their own
# syscall.h file.  This means that system call numbers can vary for each port.
# Support for all this crud is kept here, rather than trying to get too fancy.
# If you want to try to improve this, please do, but don't break anything.
# Note that there is a standard syscall.h file (libgloss/syscall.h) now which
# hopefully more targets can use.

dir=newlib/libc/sys/d10v/sys target=d10v
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

# OBSOLETE dir=libgloss target=d30v
# OBSOLETE $shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
# OBSOLETE 	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss target=fr30
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss/i960 target=i960
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss target=m32r
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss target=mn10200
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss target=mn10300
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss target=sparc
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"

dir=libgloss/v850/sys target=v850
$shell ${srccom}/gentvals.sh $target sys ${srcroot}/$dir \
	"syscall.h" 'SYS_[_A-Za-z0-9]*' "${cpp}"
