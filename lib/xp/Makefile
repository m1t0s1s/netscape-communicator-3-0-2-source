#! gmake

DEPTH = ../..

CSRCS =		   \
	allxpstr.c \
	xp_alloc.c \
	xp_cntxt.c \
	xp_core.c  \
	xp_error.c \
	xp_file.c  \
	xp_hash.c  \
	xp_intl.c  \
	xp_list.c  \
	xp_md5.c   \
	xp_reg.c   \
	xp_rgb.c   \
	xp_sec.c   \
	xp_str.c   \
	xp_thrmo.c \
	xp_time.c  \
	xp_trace.c \
	xp_wrap.c  \
	xpassert.c \
	xplocale.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libxp.a

GEN_IN = hype.au photo.jpg rsalogo.gif biglogo.gif javalogo.gif qt_logo.gif

GEN_OUT = hype.h photo.h rsalogo.h biglogo.h javalogo.h qt_logo.h

#TARGETS = $(LIBRARY) $(GEN_OUT)
TARGETS = $(LIBRARY)

include $(DEPTH)/config/rules.mk

# Generate c header's for various binary files

ECHO = echo
EMACS = lemacs

XFE_PROGCLASS = Netscape
XFE_NAME = XFE_PROGCLASS

.SUFFIXES: .au .gif .jpg

.au.h:
	@$(ECHO) generating $*.h from $<...;		     \
	CNAME=`$(ECHO) $*_au`;				     \
	$(EMACS) -batch -q -l bin-to-c.el -f batch-bin-to-c  \
		$$CNAME $< $*.h

.gif.h:
	@$(ECHO) generating $*.h from $<...;		     \
	CNAME=`$(ECHO) $*_gif`;				     \
	$(EMACS) -batch -q -l bin-to-c.el -f batch-bin-to-c  \
		$$CNAME $< $*.h

.jpg.h:
	@$(ECHO) generating $*.h from $<...;		     \
	CNAME=`$(ECHO) $*_jpg`;				     \
	$(EMACS) -batch -q -l bin-to-c.el -f batch-bin-to-c  \
		$$CNAME $< $*.h

hype.h: hype.au
photo.h: photo.jpg
rsalogo.h: rsalogo.gif
biglogo.h: biglogo.gif
javalogo.h: javalogo.gif
qt_logo.h: qt_logo.gif

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: libxp.a
	cd $(DEPTH)/lib/libnet; $(MAKE) libnet.a
	cd $(DEPTH)/lib/libmsg; $(MAKE) libmsg.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
