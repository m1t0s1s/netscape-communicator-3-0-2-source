#
# Config stuff for Linux1.2.13
#

CC			= gcc
CCC			= g++
RANLIB			= ranlib

# #### figure out some way to compute this?
#jwz: fuck this
#OS_OBJTYPE		= ELF
OS_OBJTYPE		= 

NEED_XMOS		= 1

# fixme OS_CFLAGS	= -m486 -ansi -Wall -pipe -MDupdate $(DEPENDENCIES)
OS_CFLAGS		= -ansi -Wall -pipe

OS_CFLAGS		+= -DLINUX -DLINUX1_2 -D_POSIX_SOURCE -D_BSD_SOURCE -DHAVE_STRERROR -DSW_THREADS
OS_LIBS			= -L /lib -ldl -lc

CPU_ARCH		= x86
GFX_ARCH		= x
ARCH			= linux

EN_LOCALE		= C
DE_LOCALE		= de_DE.ISO8859-1
FR_LOCALE		= fr_FR.ISO8859-1
JP_LOCALE		= ja
SJIS_LOCALE		= ja_JP.SJIS
KR_LOCALE		= ko_KR.EUC
CN_LOCALE		= zh
TW_LOCALE		= zh
I2_LOCALE		= i2

BUILD_UNIX_PLUGINS	= 1
DSO_CFLAGS		= -fpic
DSO_LDOPTS		= -shared
DSO_LDFLAGS		=

XINC			= /usr/X11R6/include
INCLUDES		+= -I/b/local/osf-motif/motif-1.2.4/include/ -I$(XINC)

BSDECHO			= echo
