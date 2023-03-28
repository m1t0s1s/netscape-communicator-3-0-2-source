#! gmake

DEPTH = ../../..

CSRCS =		      \
	jsStubs.c     \
	stubs.c	      \
	njs_nodl.c    \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBTARGETS = $(OBJDIR)/libnjs.a
TARGETS = targets

# Rules to build generated headers

JRI_HEADER_CLASSES=                          \
	netscape.javascript.JSObject         \
	netscape.javascript.JSException      \
	$(NULL)

JRI_STUB_CLASSES=                            \
	netscape.javascript.JSObject         \
	netscape.javascript.JSException      \
	$(NULL)

include $(DEPTH)/config/rules.mk

INCLUDES += -I_gen -I$(DEPTH)/mocha/include -I$(DEPTH)/lib/libjava \
        -I$(DEPTH)/sun-java/include -I$(DEPTH)/sun-java/md-include \
        -I$(DEPTH)/sun-java/netscape/applet/_gen

targets:
	@$(MAKE) jri_headers jri_stubs
	@$(MAKE) libtargets

mac:: mac_jri_headers mac_jri_stubs

libtargets: $(LIBTARGETS)

$(OBJDIR)/libnjs.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

install:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

######################################################################

# Generate njs_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),IRIX)

njs_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	rm -f $@
	perl $(DEPTH)/sun-java/config/nodl.pl "njs_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

clobber::
	rm -rf _gen _stubs so_locations

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export
