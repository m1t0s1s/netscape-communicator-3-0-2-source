#! gmake

DEPTH = ../../..

CSRCS =		   \
	netStubs.c \
	conn.c     \
	inStr.c    \
	outStr.c   \
	javactxt.c \
	stubs.c	   \
	nn_nodl.c  \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBTARGETS = $(OBJDIR)/libnsn.a
TARGETS = targets

GENERATED_HEADERS_CLASSFILES =		     \
	netscape.net.URLStreamHandlerFactory \
	netscape.net.URLStreamHandler	     \
	netscape.net.URLConnection	     \
	netscape.net.URLInputStream	     \
	netscape.net.URLOutputStream	     \
	netscape.applet.AppletThreadGroup    \
	$(NULL)

GENERATED_STUBS_CLASSFILES =		    \
	netscape.net.URLStreamHandlerFactory\
	netscape.net.URLConnection	    \
	netscape.net.URLInputStream	    \
	netscape.net.URLOutputStream	    \
	$(NULL)

JRI_HEADER_CLASSES=			    \
	netscape.applet.EmbeddedAppletFrame \
	$(NULL)

JRI_STUB_CLASSES=			    \
	netscape.applet.EmbeddedAppletFrame \
	$(NULL)

include $(DEPTH)/sun-java/config/rules.mk

targets:
	@$(MAKE) headers stubs jri_headers jri_stubs
	@$(MAKE) libtargets

mac:: mac_headers mac_stubs mac_jri_headers mac_jri_stubs

libtargets: $(LIBTARGETS)

$(OBJDIR)/libnsn.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

install:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

######################################################################

# Generate nn_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),IRIX)

nn_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	rm -f $@
	perl $(DEPTH)/sun-java/config/nodl.pl "nn_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

# Rules to build generated headers

INCLUDES += -I_gen -I../applet/_gen

clobber::
	rm -rf _gen _stubs so_locations

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export
