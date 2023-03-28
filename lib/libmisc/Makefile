#! gmake

DEPTH = ../..

CSRCS =		   \
	bkmks.c	   \
	bkmutils.c \
	glhist.c   \
	hotlist.c  \
	mime.c	   \
	shist.c	   \
	undo.c     \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

# jwz
### ifdef HAVE_SECURITY
 CSRCS += htmldlgs.c
### endif


LIBRARY = libmisc.a

include $(DEPTH)/config/rules.mk

DEFINES += -DHAS_FE

INCLUDES += -I$(DEPTH)/lib/libnet -I$(DEPTH)/lib/libmsg -I.

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: libmisc.a
	cd $(DEPTH)/lib/libnet; $(MAKE) libnet.a
	cd $(DEPTH)/lib/libmsg; $(MAKE) libmsg.a
	cd $(DEPTH)/lib/xp; $(MAKE) libxp.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) 
