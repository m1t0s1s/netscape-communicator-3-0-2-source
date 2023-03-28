#! gmake

DEPTH = ../..

RTDIR = $(DEPTH)/sun-java/runtime
JAVADIR = $(DEPTH)/sun-java/java
ifdef NSBUILDROOT
RTOBJDIR = $(BUILD)/sun-java/runtime
JAVAOBJDIR = $(BUILD)/sun-java/java
else
RTOBJDIR = $(RTDIR)/$(OBJDIR)
JAVAOBJDIR = $(JAVADIR)/$(OBJDIR)
endif

RTOBJS =					\
	$(RTOBJDIR)/stubs.o			\
	$(NULL)

JAVA_OBJS =					\
	$(JAVAOBJDIR)/javai.o			\
	$(NULL)

CSRCS =		\
	nsjava.c	\
	$(NULL)

OBJS = $(CSRCS:.c=.o)
DLL	= nsjava32.$(DLL_SUFFIX)
LIB = nsjava32.$(LIB_SUFFIX)
LLFLAGS = /DEF:"nsjava32.def"
EXTRA_LIBS = $(RTOBJS) $(JAVA_OBJS) $(LIBNSPR) $(LIBMD) $(LIBJAVA)
TARGETS = $(DLL)

include $(DEPTH)/sun-java/config/rules.mk

export:: $(TARGETS)
	$(INSTALL) -m 444 $(TARGETS) $(DIST)/bin
	$(INSTALL) -m 444 $(LIB) $(DIST)/lib

install:: export

$(RTOBJS):
	cd $(DEPTH)/sun-java; $(MAKE)
	cd $(RTDIR); $(MAKE) export

$(JAVA_OBJS):
	cd $(JAVADIR); $(MAKE) nsjava_objs

$(LIBNSPR):
	cd $(DEPTH)/nspr; $(MAKE) export

$(LIBJAVA):
	cd $(DEPTH)/sun-java/runtime; $(MAKE) export

$(LIBMD):
	cd $(DEPTH)/sun-java/md; $(MAKE) export





