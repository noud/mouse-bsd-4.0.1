#	$NetBSD: Makefile,v 1.6 2006/11/10 17:36:08 christos Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	nlsut
SRCS= \
addext.c \
allocsa.c \
argmatch.c \
backupfile.c \
basename.c \
c-ctype.c \
c-strcasecmp.c \
c-strncasecmp.c \
classpath.c \
closeout.c \
concatpath.c \
copy-file.c \
csharpcomp.c \
csharpexec.c \
error-progname.c \
error.c \
execute.c \
exitfail.c \
fatal-signal.c \
findprog.c \
fstrcmp.c \
fwriteerror.c \
full-write.c \
gcd.c \
getline.c \
getndelim2.c \
hash.c \
javacomp.c \
javaexec.c \
linebreak.c \
mbswidth.c \
obstack.c \
pipe.c \
progname.c \
quote.c \
quotearg.c \
safe-read.c \
safe-write.c \
sh-quote.c \
stpcpy.c \
stpncpy.c \
tmpdir.c \
wait-process.c \
xallocsa.c \
xerror.c \
xgetcwd.c \
xmalloc.c \
xsetenv.c \
xstrdup.c

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/gettext-tools/lib

CPPFLAGS+=-DEXEEXT=\"\"

COPTS.csharpcomp.c = -Wno-stack-protector
COPTS.csharpexec.c = -Wno-stack-protector
COPTS.javacomp.c = -Wno-stack-protector
COPTS.javaexec.c = -Wno-stack-protector

.include <bsd.lib.mk>
