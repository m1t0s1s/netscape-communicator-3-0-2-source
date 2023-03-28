#
# Config stuff for IRIX
#
CPU_ARCH = mips
GFX_ARCH = x

RANLIB = /bin/true

#NS_USE_GCC = 1

ifdef NS_USE_GCC
CC = gcc
CCC = g++
AS = $(CC) -x assembler-with-cpp
ODD_CFLAGS = -Wall -Wno-format
ifdef BUILD_OPT
OPTIMIZER = -O6
endif
else
ifeq ($(OS_RELEASE),6.2)
CC	= cc -32 -DIRIX6_2
endif
CCC = CC
ODD_CFLAGS = -fullwarn -xansi
ifdef BUILD_OPT
OPTIMIZER += -Olimit 4000
endif
endif


# For purify
NOMD_OS_CFLAGS = $(ODD_CFLAGS) -DSVR4 -DSW_THREADS -DIRIX

OS_CFLAGS = $(NOMD_OS_CFLAGS) -MDupdate $(DEPENDENCIES)

MKSHLIB = $(LD) -shared


HAVE_PURIFY = 1

LOCALE_MAP = $(DEPTH)/cmd/xfe/intl/irix.lm

EN_LOCALE = en_US
DE_LOCALE = de
FR_LOCALE = fr
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

LOC_LIB_DIR = /usr/lib/X11

# to get around SGI's problems with the Asian input method
MAIL_IM_HACK = *Mail*preeditType:none
NEWS_IM_HACK = *News*preeditType:none

BSDECHO	= echo
BUILD_UNIX_PLUGINS = 1

#
# These defines are for building unix plugins
DSO_LDOPTS = -elf -shared -all
DSO_LDFLAGS = -nostdlib -L/lib -L/usr/lib  -L/usr/lib -lXm -lXt -lX11 -lgen
