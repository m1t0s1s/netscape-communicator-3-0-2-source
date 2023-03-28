#
# Config stuff for HP-UX.10.01
#

CC				= cc -Ae
CCC				= CC -Aa +a1
RANLIB			= echo

CPU_ARCH		= hppa
OS_LIBS			= -ldld -lm
OS_CFLAGS		= -DHAVE_STRERROR -DHPUX -D$(CPU_ARCH) -DSW_THREADS -D_HPUX_SOURCE
ELIBS_CFLAGS	= -g -DHAVE_STRERROR 

MKSHLIB			= $(LD) -b

LOCALE_MAP		= $(DEPTH)/cmd/xfe/intl/hpux.lm

EN_LOCALE		= american.iso88591
DE_LOCALE		= german.iso88591
FR_LOCALE		= french.iso88591
JP_LOCALE		= japanese.euc
SJIS_LOCALE		= japanese
KR_LOCALE		= korean
CN_LOCALE		= chinese-s
TW_LOCALE		= chinese-t.big5
I2_LOCALE		= i2
IT_LOCALE		= it
SV_LOCALE		= sv
ES_LOCALE		= es
NL_LOCALE		= nl
PT_LOCALE		= pt

LOC_LIB_DIR		= /usr/lib/X11

# HPUX doesn't have a BSD-style echo, so this home-brewed version will deal
# with '-n' instead.
BSDECHO			= /usr/local/bin/bsdecho

#
# These defines are for building unix plugins
#
BUILD_UNIX_PLUGINS	= 1
DSO_LDOPTS		= -b
DSO_LDFLAGS		=
DSO_CFLAGS		= +z
