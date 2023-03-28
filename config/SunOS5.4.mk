#
# Config stuff for SunOS5.4
#

ifdef NS_USE_NATIVE
CC = cc
CCC = CC
else
CC = gcc -Wall -Wno-format
CCC = g++ -Wall -Wno-format
endif

RANLIB = echo

CPU_ARCH = sparc
GFX_ARCH = x

MOZ_CFLAGS = -DSVR4 -DSYSV -DNSPR -D__svr4

# for purify
NOMD_OS_CFLAGS = $(MOZ_CFLAGS) -DSW_THREADS -DSOLARIS -DHAVE_WEAK_IO_SYMBOLS

ifdef NO_MDUPDATE
OS_CFLAGS = $(NOMD_OS_CFLAGS)
else
ifdef NS_USE_NATIVE
OS_CFLAGS = $(NOMD_OS_CFLAGS)
else
OS_CFLAGS = $(NOMD_OS_CFLAGS) -MDupdate $(DEPENDENCIES)
endif
endif

OS_LIBS = -lsocket -lnsl -ldl -L/tools/ns/lib

MOTIF = /usr/dt
MOTIFLIB = -lXm
INCLUDES += -I$(MOTIF)/include -I/usr/openwin/include

MKSHLIB = $(LD) -G -L $(MOTIF)/lib -L/usr/openwin/lib

HAVE_PURIFY = 1

ASFLAGS += -x assembler-with-cpp

NOSUCHFILE = /solaris-rm-f-sucks

LOCALE_MAP = $(DEPTH)/cmd/xfe/intl/sunos.lm

EN_LOCALE = en_US
DE_LOCALE = de
FR_LOCALE = fr
JP_LOCALE = ja
SJIS_LOCALE = ja_JP.SJIS
KR_LOCALE = ko
CN_LOCALE = zh
TW_LOCALE = zh_TW
I2_LOCALE = i2
IT_LOCALE = it
SV_LOCALE = sv
ES_LOCALE = es
NL_LOCALE = nl
PT_LOCALE = pt

LOC_LIB_DIR = /usr/openwin/lib/locale

BSDECHO	= /usr/ucb/echo

#
# These defines are for building unix plugins
#
BUILD_UNIX_PLUGINS = 1
DSO_LDOPTS = -G
DSO_LDFLAGS =
