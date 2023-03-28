#! gmake

DEPTH = ../../..

CSRCS =		      \
	appletStubs.c \
	stubs.c	      \
	na_nodl.c     \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBTARGETS = $(OBJDIR)/libnsa.a
TARGETS = targets

# Rules to build generated headers

JRI_HEADER_CLASSES=			     \
	java.lang.Object		     \
	java.io.InputStream		     \
	java.applet.Applet                   \
	netscape.applet.AppletClassLoader    \
	netscape.applet.Console              \
	netscape.applet.EmbeddedAppletFrame  \
	netscape.applet.MozillaAppletContext \
	netscape.plugin.Plugin		     \
	java.lang.String		     \
	$(NULL)

JRI_STUB_CLASSES=                            \
	java.applet.Applet                   \
	java.io.InputStream		     \
	netscape.applet.AppletClassLoader    \
	netscape.applet.Console              \
	netscape.applet.EmbeddedAppletFrame  \
	netscape.applet.MozillaAppletContext \
	netscape.plugin.Plugin		     \
	$(NULL)

include $(DEPTH)/config/rules.mk

INCLUDES += -I_gen		       \
	-I$(DEPTH)/sun-java/include    \
	-I$(DEPTH)/sun-java/md-include \

targets:
	@$(MAKE) jri_headers jri_stubs
	@$(MAKE) libtargets

libtargets: $(LIBTARGETS)

mac:: mac_jri_headers mac_jri_stubs

$(OBJDIR)/libnsa.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

install:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

######################################################################

# Generate na_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

#ifeq ($(OS_ARCH),IRIX)
#
#na_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
#	rm -f $@
#	perl $(DEPTH)/sun-java/config/nodl.pl "na_nodl_tab" \
#	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@
#
#endif

######################################################################

clobber::
	rm -rf _gen _stubs so_locations

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export
