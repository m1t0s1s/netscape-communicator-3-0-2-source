#! gmake

DEPTH = ../../..

CSRCS =		\
	audio.c	\
	stubs.c	\
	$(NULL)

OBJS = $(CSRCS:.c=.o)

MMEDIA_DEPLIBS =

LIBTARGETS = $(OBJDIR)/libmmedia.so $(OBJDIR)/libmmedia.a
TARGETS = targets

GENERATED_HEADERS_CLASSFILES = \
	sun.audio.AudioDevice  \
	$(NULL)

GENERATED_STUBS_CLASSFILES =  \
	sun.audio.AudioDevice \
	$(NULL)

include $(DEPTH)/sun-java/config/rules.mk

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

# Rules to build generated headers

INCLUDES += -I_gen

clobber::
	rm -rf _gen _stubs
