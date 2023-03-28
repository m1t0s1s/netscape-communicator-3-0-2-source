#
# Config stuff for BSDI Unix for x86.
#

CC		= gcc -Wall -Wno-format
CCC		= g++
RANLIB		= ranlib

OS_CFLAGS	= -DBSDI -DHAVE_STRERROR -DSW_THREADS -D__386BSD__
OS_LIBS		= -lcompat

XINC		= /usr/X11/include
MOTIFLIB	= -lXm
INCLUDES	+= -I$(XINC)

CPU_ARCH	= x86
GFX_ARCH	= x
ARCH		= bsdi

LOCALE_MAP	= $(DEPTH)/cmd/xfe/intl/bsd386.lm

EN_LOCALE	= C
DE_LOCALE	= de_DE.ISO8859-1
FR_LOCALE	= fr_FR.ISO8859-1
JP_LOCALE	= ja
SJIS_LOCALE	= ja_JP.SJIS
KR_LOCALE	= ko_KR.EUC
CN_LOCALE	= zh
TW_LOCALE	= zh
I2_LOCALE	= i2

LOC_LIB_DIR	= /usr/X11/lib

NOSUCHFILE	= /solaris-rm-f-sucks

BSDECHO		= echo
