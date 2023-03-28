#
# Config stuff for Reliant UNIX (SINIX)
#
CPU_ARCH = mips
GFX_ARCH = x

RANLIB = /bin/true

# use gcc -tf-
NS_USE_GCC = 1

ifdef NS_USE_GCC
CC = gcc
CCC = g++
AS = $(CC) -x assembler-with-cpp
ODD_CFLAGS = -pipe -Wall -Wno-format
ifdef BUILD_OPT
OPTIMIZER = -O
endif
else
CCC = CC
ODD_CFLAGS = -fullwarn -xansi
ifdef BUILD_OPT
OPTIMIZER += -Olimit 4000
endif
endif

INCLUDES += -I/usr/local/include

# For purify
ifeq ($(USE_KERNEL_THREADS), 1)
NOMD_OS_CFLAGS = $(ODD_CFLAGS) -DSVR4 -DSNI
else
NOMD_OS_CFLAGS = $(ODD_CFLAGS) -DSVR4 -DSW_THREADS -DSNI
endif

# OS_CFLAGS = $(NOMD_OS_CFLAGS) -MDupdate $(DEPENDENCIES)
OS_CFLAGS = $(NOMD_OS_CFLAGS)

OS_LIBS = -lsocket -lnsl -lgen -ldl

MKSHLIB = $(LD) -G


HAVE_PURIFY = 0

LOCALE_MAP = $(DEPTH)/cmd/xfe/intl/irix.lm

EN_LOCALE = En
DE_LOCALE = De
FR_LOCALE = Fr
JP_LOCALE = ja_JP.EUC
SJIS_LOCALE = ja_JP.SJIS
KR_LOCALE = ko_KR.euc
CN_LOCALE = zh_CN.ugb
TW_LOCALE = zh_TW.ucns
I2_LOCALE = i2
IT_LOCALE = it
SV_LOCALE = sv
ES_LOCALE = es
NL_LOCALE = nl
PT_LOCALE = pt

LOC_LIB_DIR = /usr/lib/locale

# to get around SGI's problems with the Asian input method
MAIL_IM_HACK = *Mail*preeditType:none
NEWS_IM_HACK = *News*preeditType:none

BSDECHO	= /usr/ucb/echo
ifeq ($(PRODUCT),"Netscape Enterprise Server")
BUILD_UNIX_PLUGINS = 1
else
BUILD_UNIX_PLUGINS =
endif

#
# These defines are for building unix plugins
DSO_LDOPTS = -G -Xlinker -Blargedynsym
DSO_LDFLAGS = -lXm -lXt -lX11 -lsocket -lnsl -lgen

NOSUCHFILE = /sni-rm-f-sucks

