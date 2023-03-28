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

CSRCS =		\
	javap.c	\
	$(NULL)

RTOBJS =				  \
	$(RTOBJDIR)/check_class.o	  \
	$(RTOBJDIR)/check_code.o	  \
	$(RTOBJDIR)/decode.o		  \
	$(RTOBJDIR)/classloader.o	  \
	$(RTOBJDIR)/classresolver.o	  \
	$(RTOBJDIR)/classresolver_stubs.o \
	$(RTOBJDIR)/globals.o		  \
	$(RTOBJDIR)/runtime_stubs.o	  \
	$(RTOBJDIR)/utf.o		  \
	$(RTOBJDIR)/util.o		  \
	$(RTOBJDIR)/zip.o		  \
	$(NULL)

MDOBJS =			  \
	$(MDOBJDIR)/path_md.o	  \
	$(MDOBJDIR)/system_md.o	  \
	$(NULL)

OBJS = $(CSRCS:.c=.o)
LIBS = $(LIBNSPR) -lm $(OS_LIBS)

PROG = $(OBJDIR)/javap
TARGETS = targets

include $(DEPTH)/sun-java/config/rules.mk

targets:
	cd $(RTDIR); $(MAKE) javap_objs
	cd $(MDDIR); $(MAKE) javap_objs
	@$(MAKE) $(PROG)

$(PROG): $(OBJS) $(LIBNSPR)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(OBJS) $(RTOBJS) $(MDOBJS) $(LIBS)

$(LIBNSPR):
	cd $(DEPTH)/nspr; $(MAKE) export

export:: targets
	$(INSTALL) $(PROG) $(DIST)/bin

install:: export

ifdef HAVE_PURIFY
javap.pure: $(OBJS) $(LIBMD) $(LIBNSPR)
	$(PURECCF) -o $@ $(OBJS) $(LIBRARIES)
endif
