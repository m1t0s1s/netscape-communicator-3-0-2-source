#
# Config stuff for SunOS5.3
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

# Purify doesn't like -MDupdate
NOMD_OS_CFLAGS = $(MOZ_CFLAGS) -DSW_THREADS -DSOLARIS -DHAVE_WEAK_IO_SYMBOLS

OS_CFLAGS = $(NOMD_OS_CFLAGS) -MDupdate $(DEPENDENCIES)
OS_LIBS = -lsocket -lnsl -ldl -L/tools/ns/lib

MOTIF = /usr/local/Motif/opt/ICS/Motif/usr
MOTIFLIB = $(MOTIF)/lib/libXm.a
INCLUDES += -I$(MOTIF)/include -I/usr/openwin/include

MKSHLIB = $(LD) -G -L $(MOTIF)/lib

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
