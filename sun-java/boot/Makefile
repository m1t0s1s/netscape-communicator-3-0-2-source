#! gmake

DEPTH = ../..

include $(DEPTH)/sun-java/config/config.mk

#
# Makefile used to boostrap java building. Construct javah and build headers
# for building the runtime. Then build the runtime.
#

RTDIR = ../runtime
MDDIR = ../md

JAVAH_SRCS =			       \
	../javah/javah.c	       \
	$(RTDIR)/classloader.c	       \
	$(RTDIR)/classresolver.c       \
	$(RTDIR)/classresolver_stubs.c \
	$(RTDIR)/globals.c	       \
	$(RTDIR)/runtime_stubs.c       \
	$(RTDIR)/utf.c		       \
	$(RTDIR)/util.c		       \
	$(RTDIR)/zip.c		       \
	$(MDDIR)/car_md.c	       \
	$(MDDIR)/path_md.c	       \
	$(MDDIR)/stubs_md.c	       \
	$(MDDIR)/system_md.c	       \
	$(NULL)

JAVAH_OBJS := $(addprefix $(OBJDIR)/,$(notdir $(JAVAH_SRCS:.c=.o)))

JAVAI_SRCS =			     \
	../java/javai.c		     \
	../java/java_stub.c	     \
	$(RTDIR)/CompSupport.c	     \
	$(RTDIR)/check_class.c	     \
	$(RTDIR)/check_code.c	     \
	$(RTDIR)/class.c	     \
	$(RTDIR)/classloader.c	     \
	$(RTDIR)/classruntime.c	     \
	$(RTDIR)/date.c		     \
	$(RTDIR)/debug.c	     \
	$(RTDIR)/exception.c	     \
	$(RTDIR)/exceptions.c	     \
	$(RTDIR)/filesystem.c	     \
	$(RTDIR)/globals.c	     \
	$(RTDIR)/interpreter.c	     \
	$(RTDIR)/io.c		     \
	$(RTDIR)/math.c		     \
	$(RTDIR)/object.c	     \
	$(RTDIR)/opnames.c	     \
	$(RTDIR)/profiler.c	     \
	$(RTDIR)/runtime.c	     \
	$(RTDIR)/security.c	     \
	$(RTDIR)/string.c	     \
	$(RTDIR)/system.c	     \
	$(RTDIR)/thread.c	     \
	$(RTDIR)/threads.c	     \
	$(RTDIR)/utf.c		     \
	$(RTDIR)/util.c		     \
	$(RTDIR)/zip.c		     \
	$(RTDIR)/classinitialize.c   \
	$(RTDIR)/classresolver.c     \
	$(RTDIR)/common_exceptions.c \
	$(RTDIR)/threadruntime.c     \
	$(MDDIR)/compiler_md.c	     \
	$(MDDIR)/car_md.c	     \
	$(MDDIR)/gc_md.c	     \
	$(MDDIR)/io_md.c	     \
	$(MDDIR)/link_md.c	     \
	$(MDDIR)/math_md.c	     \
	$(MDDIR)/misc_md.c	     \
	$(MDDIR)/mon_md.c	     \
	$(MDDIR)/path_md.c	     \
	$(MDDIR)/process_md.c	     \
	$(MDDIR)/properties_md.c     \
	$(MDDIR)/stubs_md.c	     \
	$(MDDIR)/system_md.c	     \
	$(MDDIR)/thread_md.c	     \
	$(MDDIR)/trace_md.c	     \
	bt_nodl.c		     \
	stubs.c			     \
	$(NULL)

JAVAI_OBJS := $(addprefix $(OBJDIR)/,$(notdir $(JAVAI_SRCS:.c=.o)))

TARGETS = $(OBJDIR)/javah $(OBJDIR)/java

# Set VPATH so that make can find all of the source. We do this to make
# sure that the .o's and the dependencies end up in this directory
# VPATH = ../javah:../java:../runtime:../md

default all install:: export

export::
	@$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_LIBNSPR=$(LIBNSPR) \
		REAL_NSINSTALL=$(NSINSTALL) javah_target
	@$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_LIBNSPR=$(LIBNSPR) \
		REAL_NSINSTALL=$(NSINSTALL) includes stubs
	@$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_LIBNSPR=$(LIBNSPR) \
		REAL_NSINSTALL=$(NSINSTALL) java_target

clobber::
	@$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_LIBNSPR=$(LIBNSPR) \
		REAL_NSINSTALL=$(NSINSTALL) real_clobber

clobber_all::
	@$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_LIBNSPR=$(LIBNSPR) \
		REAL_NSINSTALL=$(NSINSTALL) real_clobber_all

clean::
	@$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_LIBNSPR=$(LIBNSPR) \
		REAL_NSINSTALL=$(NSINSTALL) real_clean

# This rule uses REAL_NSINSTALL so that it gets the one normally built
# before we fiddled with BUILD_OPT
define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); $(REAL_NSINSTALL) -D $(@D); fi
endef

real_clobber::
	rm -rf $(OBJDIR) _stubs

real_clobber_all::
	rm -rf *.OBJ _stubs

real_clean::
	rm -rf $(OBJDIR) _stubs

javah_target: $(OBJDIR)/javah

$(OBJDIR)/javah: $(JAVAH_OBJS) $(REAL_LIBNSPR)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(JAVAH_OBJS) $(REAL_LIBNSPR) -lm $(OS_LIBS)

INCLUDES := -I$(OBJDIR) -I$(ODIST)/include/nspr -I$(ODIST)/include $(INCLUDES)

java_target: $(OBJDIR)/java

$(OBJDIR)/java: $(JAVAI_OBJS) $(REAL_LIBNSPR)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(JAVAI_OBJS) $(REAL_LIBNSPR) -lm $(OS_LIBS)

######################################################################

# Generate bt_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),IRIX)

bt_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	rm -f $@
	perl $(DEPTH)/sun-java/config/nodl.pl "rt_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

# Generate generated includes needed by the runtime. Generate them into
# the $(OBJDIR) directory.

JAVAH = $(OBJDIR)/javah

# BOOT_CLASSPATH is when the boot.car is out of date with respect to
# the source code (e.g. when we get a code drop from sun)
H_JVH = $(JAVAH) -classpath $(BOOT_CLASSPATH):boot.car

includes::
	@$(MAKE_OBJDIR)
	@echo $(MAKE) -f ../include/Makefile generated
	@$(MAKE) -f ../include/Makefile generated \
		JAVAH="$(JAVAH)"		  \
		JVH="$(H_JVH)"			  \
		JAVAH_OPTS="-d $(OBJDIR)"	  \
		VPATH=../include		  \
		DESTDIR=$(OBJDIR)/		  \
		RT_CLASSPATH=boot.car

# Generate generated stub files need by the runtime. Generate them into
# the _stubs directory.

include ../runtime/classfiles.mk

S_JVH = $(H_JVH) -stubs -d _stubs

stubs::
	@$(MAKE_OBJDIR)
	@echo Generating/updating stubs
	@$(S_JVH) $(GENERATED_STUBS_CLASSFILES)

######################################################################

# Gmake doesn't turn static rules (those % thingys) into real live
# dependenices. So we need to tell it how to build each and every
# file.

$(OBJDIR)/javah.o: ../javah/javah.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/javai.o: ../java/javai.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/java_stub.o: ../java/java_stub.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/classresolver_stubs.o: $(RTDIR)/classresolver_stubs.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/runtime_stubs.o: $(RTDIR)/runtime_stubs.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/CompSupport.o: $(RTDIR)/CompSupport.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/check_class.o: $(RTDIR)/check_class.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/check_code.o: $(RTDIR)/check_code.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/class.o: $(RTDIR)/class.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/classloader.o: $(RTDIR)/classloader.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/classruntime.o: $(RTDIR)/classruntime.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/date.o: $(RTDIR)/date.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/debug.o: $(RTDIR)/debug.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/exception.o: $(RTDIR)/exception.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/exceptions.o: $(RTDIR)/exceptions.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/filesystem.o: $(RTDIR)/filesystem.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/globals.o: $(RTDIR)/globals.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/interpreter.o: $(RTDIR)/interpreter.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/io.o: $(RTDIR)/io.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/math.o: $(RTDIR)/math.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/object.o: $(RTDIR)/object.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/opnames.o: $(RTDIR)/opnames.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/profiler.o: $(RTDIR)/profiler.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/runtime.o: $(RTDIR)/runtime.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/security.o: $(RTDIR)/security.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/string.o: $(RTDIR)/string.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/system.o: $(RTDIR)/system.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/thread.o: $(RTDIR)/thread.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/threads.o: $(RTDIR)/threads.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/utf.o: $(RTDIR)/utf.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/util.o: $(RTDIR)/util.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/zip.o: $(RTDIR)/zip.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/classinitialize.o: $(RTDIR)/classinitialize.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/classresolver.o: $(RTDIR)/classresolver.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/common_exceptions.o: $(RTDIR)/common_exceptions.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/threadruntime.o: $(RTDIR)/threadruntime.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/compiler_md.o: $(MDDIR)/compiler_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/car_md.o: $(MDDIR)/car_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/gc_md.o: $(MDDIR)/gc_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/io_md.o: $(MDDIR)/io_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/link_md.o: $(MDDIR)/link_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/math_md.o: $(MDDIR)/math_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/misc_md.o: $(MDDIR)/misc_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/mon_md.o: $(MDDIR)/mon_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/path_md.o: $(MDDIR)/path_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/process_md.o: $(MDDIR)/process_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/properties_md.o: $(MDDIR)/properties_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/stubs_md.o: $(MDDIR)/stubs_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/system_md.o: $(MDDIR)/system_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/thread_md.o: $(MDDIR)/thread_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/trace_md.o: $(MDDIR)/trace_md.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/stubs.o: stubs.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/bt_nodl.o: bt_nodl.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

-include $(DEPENDENCIES)
