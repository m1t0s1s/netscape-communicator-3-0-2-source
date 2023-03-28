# jwz hacks

DEBUG_jwz = 1
NO_MAC_JAVA_SHIT = 1
#NO_MOCHA = 1
NO_JAVA = 1
BUILD_UNIX_PLUGINS=
#HAVE_SECURITY = 1

ifndef BUILD_OPT
  ifeq ($(OS_ARCH),IRIX)
    NS_USE_GCC = 1
  endif
endif

# have to do this here, since myconfig.mk is loaded after $(OS_CONFIG).mk
ifdef NS_USE_GCC
  CC  = gcc
  CCC = g++
  AS = $(CC) -x assembler-with-cpp
  ODD_CFLAGS = -Wall -Wno-format -pipe
  MKSHLIB = $(CC) -shared

  ifdef BUILD_OPT
    OPTIMIZER = -O3
  endif

  ifeq ($(OS_ARCH),IRIX)
    CC  += -DSVR4 -DIRIX
    CCC += -DSVR4 -DIRIX
    ifeq ($(OS_RELEASE),6.2)
      CC  += -DIRIX6_2
      CCC += -DIRIX6_2
    endif
  endif # IRIX

endif  # NS_USE_GCC

ETAGS=true etags

# Do this even in optimized builds.
ifdef BUILD_OPT
DEFINES += -DDEBUG_jwz
endif

# jwz
ifdef HAVE_SECURITY
DEFINES += -UNO_SSL -DHAVE_SECURITY
else
DEFINES += -DNO_SSL -UHAVE_SECURITY
endif

# when doing debug builds, optimize all directories except those enumerated
# below.  The lack of debug info in the uninteresting directories makes
# linking faster.
#
ifndef BUILD_OPT

OPTIMIZER = -O
ifeq ($(SRCDIR),cmd/xfe)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libparse)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libsec)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),cmd/security)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libmisc)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libmsg)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libnet)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/xp)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/xlate)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libmime)
OPTIMIZER = -g
endif

ifeq ($(SRCDIR),lib/libmime2)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),cmd/intertwingle)
OPTIMIZER = -g
endif

ifeq ($(SRCDIR),security/lib/cert)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/crypto)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/hash)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/key)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/nav)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/pkcs11)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/pkcs12)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/pkcs7)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/ssl)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),security/lib/util)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/layout)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libmocha)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),lib/libimg)
OPTIMIZER = -g
endif
ifeq ($(SRCDIR),nspr/tests)
OPTIMIZER = -g
endif

endif
