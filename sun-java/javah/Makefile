#! gmake

DEPTH = ../..

RTDIR = $(DEPTH)/sun-java/runtime
MDDIR = $(DEPTH)/sun-java/md
ifdef NSBUILDROOT
RTOBJDIR = $(BUILD)/sun-java/runtime
MDOBJDIR = $(BUILD)/sun-java/md
else
RTOBJDIR = $(RTDIR)/$(OBJDIR)
MDOBJDIR = $(MDDIR)/$(OBJDIR)
endif

RTOBJS =					\
	$(RTOBJDIR)/classloader.o		\
	$(RTOBJDIR)/classresolver.o		\
	$(RTOBJDIR)/classresolver_stubs.o	\
	$(RTOBJDIR)/globals.o			\
	$(RTOBJDIR)/runtime_stubs.o		\
	$(RTOBJDIR)/utf.o			\
	$(RTOBJDIR)/util.o			\
	$(RTOBJDIR)/zip.o			\
	$(NULL)

MDOBJS =			  \
	$(MDOBJDIR)/path_md.o	  \
	$(MDOBJDIR)/stubs_md.o	  \
	$(MDOBJDIR)/system_md.o	  \
	$(NULL)

CSRCS =		\
	javah.c	\
	$(NULL)

OBJS = $(CSRCS:.c=.o)
PROG = $(OBJDIR)/javah

GLUE_FILES =		\
	jriauto.c	\
	$(NULL)

GLUE_LIB = $(OBJDIR)/libjri.a

#TARGETS = javah_objs $(GLUE_LIB) $(PROG)
TARGETS = javah_objs $(PROG)

include $(DEPTH)/sun-java/config/rules.mk

GLUE_OBJS := $(addprefix $(OBJDIR)/, $(GLUE_FILES:.c=.o))

all default::	$(TARGETS)

.PHONY:	javah_objs

javah_objs:
	cd $(RTDIR); $(MAKE) $@
	cd $(MDDIR); $(MAKE) $@

ifeq ($(OS_ARCH),AIX)
$(PROG): $(OBJS) $(LIBNSPR)
	@$(MAKE_OBJDIR)
	@rm -f $@ $(AIX_TMP)
	$(CCF) -o $(AIX_TMP) $(AIX_LINK_OPTS) \
		$(OBJS) $(RTOBJS) $(MDOBJS) $(LIBNSPR) -lm $(OS_LIBS)
	$(CC) -o $@ $(AIX_TMP) $(AIX_WRAP) $(AIX_FINAL_LDFLAGS)
	@rm -f $(AIX_TMP)
else
$(PROG): $(OBJS) $(LIBNSPR)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(OBJS) $(RTOBJS) $(MDOBJS) $(LIBNSPR) -lm $(OS_LIBS)
endif

export:: $(TARGETS)
	$(INSTALL) $(PROG) $(DIST)/bin
	$(INSTALL) $(GLUE_LIB) $(DIST)/lib

install:: export

$(GLUE_LIB): $(GLUE_OBJS)
	@$(MAKE_OBJDIR)
	@rm -f $@
	$(AR) cr $@ $(GLUE_OBJS)
	$(RANLIB) $@

$(LIBNSPR):
	cd $(DEPTH)/nspr; $(MAKE) export

#ifdef HAVE_PURIFY
javah.pure: $(OBJS) $(LIBRARIES)
	$(PURECCF) -o $@ $(OBJS) $(LIBRARIES)
#endif
