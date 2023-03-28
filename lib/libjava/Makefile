#! gmake

DEPTH = ../..

CSRCS =		   \
	lj_cons.c  \
	lj_embed.c \
	lj_init.c  \
	lj_run.c   \
	lj_thred.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBNAME = $(OBJDIR)/libjava.a
TARGETS = targets

include $(DEPTH)/config/rules.mk

# duplicated in ns/sun-java/classsrc/Makefile
VNNH=../../cmd/xfe/versionn.h
CLASS_FILE=$(shell \
              sed -n 's/^.define VERSION_NUMBER *\(.*\)$$/java_\1/p' \
              $(VNNH))
DEFINES += -DZIP_NAME=\"$(CLASS_FILE)\"

targets::
	$(MAKE) headers
	$(MAKE) $(LIBNAME)

$(LIBNAME): $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

export:: targets
	$(INSTALL) $(LIBNAME) $(DIST)/lib

install:: export

headers:: $(OBJDIR)/ljbuild.h

GARBAGE += $(OBJDIR)/ljbuild.h

BUILD_NUMBER = $(DEPTH)/build/build_number

$(OBJDIR)/ljbuild.h:: Makefile $(BUILD_NUMBER)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(BSDECHO) "static int build_number = "`cat $(BUILD_NUMBER)`";" > $@

# XXX stop reaching around into the sun-java tree
JAVA_DIR = $(DEPTH)/sun-java
INCLUDES +=	-I$(DEPTH)/lib/layout -I$(JAVA_DIR)/include \
                -I$(JAVA_DIR)/md-include		    \
		-I$(JAVA_DIR)/netscape/applet/_gen	    \
		-I$(OBJDIR)				    \
                -I$(DEPTH)/mocha/include
