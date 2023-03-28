#
# Config stuff for DEC OSF/1
#

CC			= cc $(NON_LD_FLAGS)

# This differentiation is required so we can link.
CCC_MINIMUM		= cxx
CCC			= $(CCC_MINIMUM) -x cxx $(NON_LD_FLAGS)

RANLIB			= /bin/true

CPU_ARCH		= alpha
GFX_ARCH		= x

ifdef BUILD_OPT
OPTIMIZER		+= -Olimit 4000
endif

NON_LD_FLAGS		= -ieee_with_inexact
OS_CFLAGS		= -std -DSW_THREADS -DOSF1 -DIS_64 -taso
OS_LIBS			=

LOCALE_MAP		= $(DEPTH)/cmd/xfe/intl/osf1.lm

EN_LOCALE		= en_US.ISO8859-1
DE_LOCALE		= de_DE.ISO8859-1
FR_LOCALE		= fr_FR.ISO8859-1
JP_LOCALE		= ja_JP.eucJP
SJIS_LOCALE		= ja_JP.SJIS
KR_LOCALE		= ko_KR.eucKR
CN_LOCALE		= zh_CN
TW_LOCALE		= zh_TW.eucTW
I2_LOCALE		= i2
IT_LOCALE		= it
SV_LOCALE		= sv
ES_LOCALE		= es
NL_LOCALE		= nl
PT_LOCALE		= pt

LOC_LIB_DIR		= /usr/lib/X11

#undef HAVE_PURIFY

BSDECHO			= echo

#
# These defines are for building unix plugins
#
BUILD_UNIX_PLUGINS	= 1
DSO_LDOPTS		= -shared
DSO_LDFLAGS		= -lXm -lXt -lX11 -lc
