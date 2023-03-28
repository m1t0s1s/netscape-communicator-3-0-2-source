#
# Config stuff for Sony Mips SYSV
#
 
CPU_ARCH	= mips
GFX_ARCH	= x
ARCH		=
 
CC		= cc
ODD_CFLAGS	= -Xa -fullwarn
OPTIMIZER	= -Olimit 4000
 
MKSHLIB		= $(LD) -G
 
RANLIB		= /bin/true
 
INC		= -I/usr/include
INCLUDES	+= $(INC)
EXTRA_LIBRARIES	= -lsocket -lnsl -lresolv
MOTIFLIB	= -L/usr/lib -lXm
LOC_LIB_DIR	= /usr/lib/X11
 
OS_CFLAGS	= $(ODD_CFLAGS) -DSVR4 -DHAVE_STRERROR -DSW_THREADS -DSONY
OS_LIBS		= -lsocket -lnsl -lgen -lresolv
 
EN_LOCALE	= C
ECHO		= /usr/bin/echo
BSDECHO		= /usr/ucb/echo
MCS		= /usr/ccs/bin/mcs -d
NOSUCHFILE	= /sony-rm-f-sucks
 
HAVE_PURIFY	=
 
#
# XXX etags??
#
ETAGS		= echo etags
 
DE_LOCALE	= de_DE.ISO8859-1
FR_LOCALE	= fr_FR.ISO8859-1
JP_LOCALE	= ja.JP.EUC
SJIS_LOCALE	= ja_JP.SJIS
KR_LOCALE	= ko_KR.EUC
CN_LOCALE	= zh_CN.ugb
TW_LOCALE	= zh_TW.ucns
I2_LOCALE	= i2
 
#
# These defines are for building unix plugins
#
ifeq ($(PRODUCT),"Netscape Enterprise Server")
BUILD_UNIX_PLUGINS	= 1
else
BUILD_UNIX_PLUGINS	=
endif
DSO_LDOPTS		= -G
DSO_LDFLAGS		=
