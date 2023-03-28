#
# Config stuff for SCO Unixware 2.1
#

CC = $(DEPTH)/build/hcc
CCC = $(DEPTH)/build/hCC

RANLIB = true

#
# -DX11R5 gets around the broken XCreateFontSet that would hang the server 
# with a huge FontInfo request
#
OS_CFLAGS = -DSVR4 -DSYSV -DHAVE_STRERROR -DSW_THREADS -DUNIXWARE
OS_LIBS = -lsocket -lc /usr/ucblib/libucb.a

XINC = /usr/X/include
MOTIFLIB = -lXm
INCLUDES += -I$(XINC)

CPU_ARCH = x86
GFX_ARCH = x
ARCH=sco

LOCALE_MAP = $(DEPTH)/cmd/xfe/intl/unixware.lm
EN_LOCALE = C
DE_LOCALE = de_DE.ISO8859-1
FR_LOCALE = fr_FR.ISO8859-1
JP_LOCALE = ja
SJIS_LOCALE = ja_JP.SJIS
KR_LOCALE = ko_KR.EUC
CN_LOCALE = zh
TW_LOCALE = zh
I2_LOCALE = i2

LOC_LIB_DIR = /usr/lib/X11

NOSUCHFILE = /solaris-rm-f-sucks

BSDECHO	= echo
