#
# Config stuff for SunOS5.5
#

CC = gcc -Wall -Wno-format
CCC = g++ -Wall -Wno-format

#CC = /opt/SUNWspro/SC3.0.1/bin/cc
RANLIB = echo

#.c.o:
#	$(CC) -c -MD $*.d $(CFLAGS) $<

CPU_ARCH = sparc
GFX_ARCH = x

MOZ_CFLAGS = -DSVR4 -DSYSV -DNSPR
SOL_CFLAGS = -DSOLARIS -D_SVID_GETTOD

# for purify
NOMD_OS_CFLAGS = $(MOZ_CFLAGS) -DSW_THREADS $(SOL_CFLAGS)  -DHAVE_WEAK_IO_SYMBOLS

#OS_CFLAGS = $(NOMD_OS_CFLAGS) -MDupdate $(DEPENDENCIES)
OS_CFLAGS = $(NOMD_OS_CFLAGS) -MD

OS_LIBS = -lsocket -lnsl -ldl

MOTIF = /usr/dt
MOTIFLIB = -lXm
INCLUDES += -I$(MOTIF)/include -I/usr/openwin/include

MKSHLIB = $(LD) -G -L $(MOTIF)/lib -L/usr/openwin/lib

HAVE_PURIFY = 1

ASFLAGS += -x assembler-with-cpp

NOSUCHFILE = /solaris-rm-f-sucks

LOCALE_MAP = $(DEPTH)/cmd/xfe/intl/sunos.lm

EN_LOCALE = C
DE_LOCALE = de_DE.ISO8859-1
FR_LOCALE = fr_FR.ISO8859-1
JP_LOCALE = ja
SJIS_LOCALE = ja_JP.SJIS
KR_LOCALE = ko_KR.EUC
CN_LOCALE = zh
TW_LOCALE = zh
I2_LOCALE = i2

LOC_LIB_DIR = /usr/openwin/lib/locale

BSDECHO	= /usr/ucb/echo

#
# These defines are for building unix plugins
#
BUILD_UNIX_PLUGINS = 1
DSO_LDOPTS = -G
DSO_LDFLAGS =

