#! gmake

DEPTH = ../../..

include $(DEPTH)/sun-java/config/config.mk

CSRCS =		  \
	audio.c	  \
	stubs.c	  \
	au_nodl.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBTARGETS = $(OBJDIR)/libmmedia.a
ifdef MKSHLIB
LIBTARGETS += $(OBJDIR)/libmmedia.so
endif
MMEDIA_DEPLIBS =

TARGETS = targets

GENERATED_HEADERS_CLASSFILES = \
	sun.audio.AudioDevice  \
	$(NULL)

GENERATED_STUBS_CLASSFILES =  \
	sun.audio.AudioDevice \
	$(NULL)

include $(DEPTH)/sun-java/config/rules.mk

ifeq ($(OS_ARCH),HP-UX)
OS_CFLAGS += +z	# position-independent code
endif

targets:
	@$(MAKE) headers stubs
	@$(MAKE) libtargets

libtargets: $(LIBTARGETS)

$(OBJDIR)/libmmedia.so: $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(MKSHLIB) -o $@ $(OBJS) $(MMEDIA_DEPLIBS)

$(OBJDIR)/libmmedia.a: $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export

######################################################################

# Generate au_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),IRIX)

au_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	perl $(DEPTH)/sun-java/config/nodl.pl "au_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

# Rules to build generated headers

INCLUDES += -I_gen

clobber::
	rm -rf _gen _stubs
