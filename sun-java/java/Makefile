#! gmake

DEPTH = ../..

LSRCS =			\
	javai.c		\
	$(NULL)

CSRCS =			\
	java_stub.c	\
	javai.c		\
	$(NULL)

LOBJS = $(LSRCS:.c=.o)
OBJS = $(CSRCS:.c=.o)
SOBJS = $(OBJDIR)/java_stub.o $(OBJDIR)/javai_s.o

SLIBRARIES	= $(LIBJAVA) $(LIBMD) $(LIBJAVADLL) $(LIBAWT) $(LIBMMEDIA) $(LIBNSPR) \
		  -lXm -lXt -lX11 -laudio -lm $(OS_LIBS) $(DEBUG_LIBS)

LIBNAME = $(OBJDIR)/javai.a
TARGETS = targets

include $(DEPTH)/sun-java/config/rules.mk
LOBJS := $(addprefix $(OBJDIR)/, $(LOBJS))

# AIX needs to know what symbols to export for dynamic linking
AIX_EXPORTS = -bE:../runtime/exports

targets:
	cd $(DEPTH)/sun-java/include; $(MAKE) export
	$(MAKE) $(OBJDIR)/java
	$(MAKE) $(LIBNAME)

$(LIBNAME): $(LOBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(LOBJS)
	$(RANLIB) $@

ifeq ($(OS_ARCH),AIX)
$(OBJDIR)/java: $(OBJS) $(LIBNSPR) $(LIBJAVA) $(LIBMD)
	@$(MAKE_OBJDIR)
	rm -f $@ $(AIX_TMP)
	$(CCF) -o $(AIX_TMP) $(AIX_LINK_OPTS) $(OBJS) $(LIBRARIES)
	$(CC) -o $@ $(AIX_TMP) $(AIX_WRAP) $(AIX_FINAL_LDFLAGS)
	rm -f $(AIX_TMP)
else
$(OBJDIR)/java: $(OBJS) $(LIBNSPR) $(LIBJAVA) $(LIBMD)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(OBJS) $(LIBRARIES)
endif

$(OBJDIR)/java_s: $(SOBJS) $(LIBNSPR) $(LIBJAVA) $(LIBMD) $(LIBAWT) $(LIBMMEDIA)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(SOBJS) $(SLIBRARIES)

$(OBJDIR)/javai_s.o: javai.c
	$(CCF) -DSTATIC_JAVA -c -o $@ javai.c

$(OBJDIR)/java_p: $(OBJS) $(LIBNSPR) $(LIBJAVA)
	$(CCF) -p -o $@ $(OBJS) $(LIBRARIES)

$(OBJDIR)/java.pure: $(OBJS) $(LIBNSPR) $(LIBJAVA)
	$(PURECCF) -o $@ $(OBJS) $(LIBRARIES)

$(OBJDIR)/java.pixie: $(OBJDIR)/java
	cd $(OBJDIR); pixie java

$(LIBJAVA):
	cd $(DEPTH)/sun-java/runtime; $(MAKE) export

$(LIBMD):
	cd $(DEPTH)/sun-java/md; $(MAKE) export

$(LIBAWT):
	cd $(DEPTH)/sun-java/awt; $(MAKE) export

$(LIBMMEDIA):
	cd $(DEPTH)/sun-java/awt; $(MAKE) export

install:: targets
	$(INSTALL) $(LIBNAME) $(DIST)/lib
	$(INSTALL) $(OBJDIR)/java $(DIST)/bin
	$(INSTALL) $(OBJDIR)/java_s $(DIST)/bin

export:: install
