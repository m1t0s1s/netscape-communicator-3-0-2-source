#! gmake

DEPTH = ../..

include $(DEPTH)/sun-java/config/config.mk

CSRCS =		       \
	jrijdk.c       \
	agent.c	       \
	check_class.c  \
	check_code.c   \
	class.c	       \
	classloader.c  \
	classruntime.c \
	compiler.c     \
	gifdecoder.c   \
	date.c	       \
	debug.c	       \
	exception.c    \
	exceptions.c   \
	filesystem.c   \
	globals.c      \
	interpreter.c  \
	io.c	       \
	jpegdecoder.c  \
	math.c	       \
	object.c       \
	opnames.c      \
	profiler.c     \
	runtime.c      \
	security.c     \
	string.c       \
	stubs.c	       \
	system.c       \
	thread.c       \
	threads.c      \
	utf.c	       \
	util.c	       \
	zip.c	       \
	rt_nodl.c      \
	$(NULL)

# List of long filename CSRCS. The BSDI ar command is hosed, so we
# need this list
LONG_CSRCS =		    \
	classinitialize.c   \
	classresolver.c	    \
	common_exceptions.c \
	threadruntime.c	    \
	$(NULL)

BSDI_LONG_OBJS =       \
	classinit.o    \
	classresolve.o \
	common_exc.o   \
	threadrt.o     \
	$(NULL)

JAVAH_CSRCS =		      \
	classloader.c	      \
	classresolver.c	      \
	classresolver_stubs.c \
	globals.c	      \
	runtime_stubs.c	      \
	utf.c		      \
	util.c		      \
	zip.c		      \
	$(NULL)

JAVAP_CSRCS =		      \
	check_class.c	      \
	check_code.c	      \
	decode.c	      \
	classloader.c	      \
	classresolver.c	      \
	classresolver_stubs.c \
	globals.c	      \
	runtime_stubs.c	      \
	utf.c		      \
	util.c		      \
	zip.c		      \
	$(NULL)

PACKAGES = java/lang/ java/io/ java/net/ java/util/

ifeq ($(OS_ARCH),BSD_386)
OBJS = $(CSRCS:.c=.o) $(BSDI_LONG_OBJS)
else
OBJS = $(CSRCS:.c=.o) $(LONG_CSRCS:.c=.o)
endif

JAVAH_OBJS = $(addprefix $(OBJDIR)/, $(JAVAH_CSRCS:.c=.o))
JAVAP_OBJS = $(addprefix $(OBJDIR)/, $(JAVAP_CSRCS:.c=.o))

LIBTARGETS = $(OBJDIR)/javart.a
TARGETS = targets

include classfiles.mk

include $(DEPTH)/sun-java/config/rules.mk

ifdef MKSHLIB
LIBTARGETS += $(OBJDIR)/javart.so
CFLAGS += $(DSO_CFLAGS)
endif

targets:
	@$(MAKE) stubs
	@$(MAKE) libtargets

mac:: mac_stubs

libtargets: $(LIBTARGETS)

$(OBJDIR)/javart.so: $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(MKSHLIB) -o $@ $(OBJS)

$(OBJDIR)/javart.a: $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

javah_objs: $(JAVAH_OBJS)

javap_objs: $(JAVAP_OBJS)

# Rules to build renamed object files to work around bsdi bug
ifeq ($(OS_ARCH),BSD_386)
$(OBJDIR)/classinit.o: classinitialize.c
	$(CCF) -o $@ -c $<

$(OBJDIR)/classresolve.o: classresolver.c
	$(CCF) -o $@ -c $<

$(OBJDIR)/common_exc.o: common_exceptions.c
	$(CCF) -o $@ -c $<

$(OBJDIR)/threadrt.o: threadruntime.c
	$(CCF) -o $@ -c $<
endif

#
# Workaround for a SCO compiler bug.
#
# We could try using GCC 2.7.x instead, since that is the first
# version that can produce SCO ELF binaries.
#
ifeq ($(OS_ARCH),SCO_SV)
$(OBJDIR)/interpreter.o: interpreter.c
	$(CCF) -O0 -o $@ -c $<
endif

clean::
	rm -f $(OBJDIR)/*.o

clobber::
	rm -f $(OBJDIR)/*.o

######################################################################

# Generate rt_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),NEVER)

rt_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	rm -f $@
	perl $(DEPTH)/sun-java/config/nodl.pl "rt_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

clobber::
	rm -rf _stubs

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export
