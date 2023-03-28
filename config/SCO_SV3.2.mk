#
# Config stuff for SCO Unix for x86.
#

CC		= cc -b elf
CCC		= hCC +.cpp +w
RANLIB		= /bin/true

#
# -DSCO_PM - Policy Manager AKA: SCO Licensing
# -DSCO - Changes to Netscape source (consistent with AIX, LINUX, etc..)
# -Dsco - Needed for /usr/include/X11/*
#
OS_CFLAGS	= -DSCO_SV -DSYSV -DHAVE_STRERROR -DSW_THREADS -DSCO_PM -DSCO -Dsco
OS_LIBS		= -lpmapi -lsocket

MKSHLIB		= $(LD) -G

XINC		= /usr/include/X11
MOTIFLIB	= -lXm
INCLUDES	+= -I$(XINC)

CPU_ARCH	= x86
GFX_ARCH	= x
ARCH		= sco

LOCALE_MAP	= $(DEPTH)/cmd/xfe/intl/sco.lm
EN_LOCALE	= C
DE_LOCALE	= de_DE.ISO8859-1
FR_LOCALE	= fr_FR.ISO8859-1
JP_LOCALE	= ja
SJIS_LOCALE	= ja_JP.SJIS
KR_LOCALE	= ko_KR.EUC
CN_LOCALE	= zh
TW_LOCALE	= zh
I2_LOCALE	= i2

LOC_LIB_DIR	= /usr/lib/X11

NOSUCHFILE	= /solaris-rm-f-sucks

BSDECHO		= /bin/echo

#
# These defines are for building unix plugins
#
BUILD_UNIX_PLUGINS	= 1
DSO_LDOPTS	= -G
DSO_LDFLAGS	=
