#
# Config stuff for AIX3.2
#

CC		= cc
CCC		= xlC -+

CPU_ARCH	= rs6000
GFX_ARCH	= x

RANLIB		= ranlib

#
# XXX we are defining AIXV3 for V2. We should hunt down and kill the
# lines that depend on that.  WRONG.  The AIX flavor of uname reverses
# the major and minor version numbers.  We are using the minor number
# to describe AIX 3.2.5.
#
OS_CFLAGS	= -DAIX -DSW_THREADS -DAIXV3 -DSYSV
OS_LIBS		= -lbsd

#
# Special link info for constructing AIX programs. On AIX we have to
# statically link programs that use NSPR into a single .o, rewriting the
# calls to select to call "wrap_select". Once that is done we then can
# link that .o with a .o built in nspr which implements the system call.
#
AIX_LINK_OPTS	= -bnso -berok -brename:.select,.wrap_select
AIX_WRAP	= $(DIST)/lib/aixwrap.o
AIX_FINAL_LDFLAGS	= -bI:/usr/lib/syscalls.exp -bI:/usr/lpp/X11/bin/smt.exp
AIX_TMP		= $(OBJDIR)/_aix_tmp.o

########################################

LOCALE_MAP	= $(DEPTH)/cmd/xfe/intl/aix.lm

EN_LOCALE	= en_US.ISO8859-1
DE_LOCALE	= de_DE.ISO8859-1
FR_LOCALE	= fr_FR.ISO8859-1
JP_LOCALE	= ja_JP.IBM-eucJP
SJIS_LOCALE	= Ja_JP.IBM-932
KR_LOCALE	= ko_KR.IBM-eucKR
CN_LOCALE	= zh_CN
TW_LOCALE	= zh_TW.IBM-eucTW
I2_LOCALE	= iso88592

LOC_LIB_DIR	= /usr/lib/X11

BSDECHO		= echo
