#
# Config stuff for Solaris 2.4 on x86
# 

ifdef NS_USE_NATIVE
CC		= cc
CCC		= $(CC)
else
CC		= gcc
CCC		= g++
endif

BSDECHO		= /usr/ucb/echo
ECHO		= $(ECHO)
ETAGS		= echo etags
MCS		= mcs -d
MKSHLIB		= $(LD) -G -L $(MOTIF)/lib -L/usr/openwin/lib
NOSUCHFILE	= /solaris-rm-f-sucks
RANLIB		= echo

LOC_LIB_DIR	= /usr/openwin/lib/locale
MOTIF		= /usr/dt
MOTIFLIB	= -lXm
INCLUDES	+= -I$(MOTIF)/include -I/usr/openwin/include

HAVE_PURIFY	= 1

ASFLAGS		+= -x assembler-with-cpp

MOZ_CFLAGS	= -DSVR4 -DSYSV -DNSPR

# for purify
NOMD_OS_CFLAGS	= -Wall -Wno-format $(MOZ_CFLAGS) -DSW_THREADS -D_REENTRANT -DSOLARIS -DHAVE_WEAK_IO_SYMBOLS -D__svr4__ -Di386

#OS_CFLAGS	= -Wall -Wno-format -DSVR4 -DHAVE_STRERROR -DSW_THREADS -D__svr4__ -DSOLARIS -Di386
OS_CFLAGS	= $(NOMD_OS_CFLAGS) -MDupdate $(DEPENDENCIES)
OS_LIBS		= -lsocket -lnsl -lgen -lresolv -ldl

CPU_ARCH	= x86
GFX_ARCH	= x
ARCH		=

LOCALE_MAP	= $(DEPTH)/cmd/xfe/intl/sunos.lm

EN_LOCALE	= en_US
DE_LOCALE	= de
FR_LOCALE	= fr
JP_LOCALE	= ja
SJIS_LOCALE	= ja_JP.SJIS
KR_LOCALE	= ko
CN_LOCALE	= zh
TW_LOCALE	= zh_TW
I2_LOCALE	= i2
IT_LOCALE	= it
SV_LOCALE	= sv
ES_LOCALE	= es
NL_LOCALE	= nl
PT_LOCALE	= pt

#
# These defines are for building unix plugins
#
BUILD_UNIX_PLUGINS	= 1
DSO_LDOPTS	= -G
DSO_LDFLAGS	=
