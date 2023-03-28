#! gmake

DEPTH = ..

HEADERS = jconfig.h jerror.h jinclude.h jmorecfg.h jpeglib.h

CSRCS =		   \
	jdapimin.c \
	jdapistd.c \
	jdatasrc.c \
	jdmaster.c \
        jdinput.c  \
	jdmarker.c \
	jdhuff.c   \
	jdphuff.c  \
	jdmainct.c \
	jdcoefct.c \
        jdpostct.c \
	jddctmgr.c \
	jidctfst.c \
	jidctflt.c \
	jidctint.c \
	jidctred.c \
	jdsample.c \
	jdcolor.c  \
	jquant1.c  \
	jquant2.c  \
	jdmerge.c  \
	jcomapi.c  \
	jutils.c   \
	jerror.c   \
	jmemmgr.c  \
	jmemnobs.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libjpeg.a

include $(DEPTH)/config/rules.mk

export:: $(TARGETS)
	$(INSTALL) -m 444 $(HEADERS) $(DIST)/include/jpeg

install:: export
