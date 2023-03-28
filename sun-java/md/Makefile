#! gmake

DEPTH = ../..

CFILES =		\
	agentthread.c	\
	compiler_md.c	\
	gc_md.c		\
	io_md.c		\
	ionet_md.c	\
	link_md.c	\
	math_md.c	\
	misc_md.c	\
	mon_md.c	\
	path_md.c	\
	process_md.c	\
	properties_md.c	\
	socket_md.c	\
	stubs_md.c	\
	system_md.c	\
	thread_md.c	\
	trace_md.c	\
	$(NULL)

JAVAH_CSRCS =	      \
	path_md.c     \
	stubs_md.c    \
	system_md.c   \
	$(NULL)

JAVAP_CSRCS =	      \
	path_md.c     \
	system_md.c   \
	$(NULL)

OBJS = $(CFILES:.c=.o)
JAVAH_OBJS = $(addprefix $(OBJDIR)/, $(JAVAH_CSRCS:.c=.o))
JAVAP_OBJS = $(addprefix $(OBJDIR)/, $(JAVAP_CSRCS:.c=.o))

LIBNAME = $(OBJDIR)/sunmd.a
TARGETS = $(LIBNAME)

GENERATED_HEADERS_CLASSFILES =	\
	java.lang.ProcessReaper	\
	java.lang.UNIXProcess	\
	$(NULL)

include $(DEPTH)/sun-java/config/rules.mk


# This is duplicated in ns/sun-java/classsrc/Makefile
VNNH=../../cmd/xfe/versionn.h
 get_vn=sed -n 's/^.define VERSION_NUMBER *\(.*\)$$/java_\1/p' $(VNNH)
 CLASS_FILE=${shell $(get_vn)}

DEFINES += -DZIP_NAME=\"$(CLASS_FILE)\"

# This file needs to be recompiled when -DZIP_NAME changes.
# Aw, shit.  This doesn't work because of : versus :: brain damage!
# $(OBJDIR)/path_md.o:: $(VNNH)


$(LIBNAME): $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

javah_objs: $(JAVAH_OBJS)

javap_objs: $(JAVAP_OBJS)

install:: $(LIBNAME)
	$(INSTALL) $(LIBNAME) $(DIST)/lib

export:: install

# XXX can't use this quite yet
# Rules to build generated headers

INCLUDES += -I_gen


