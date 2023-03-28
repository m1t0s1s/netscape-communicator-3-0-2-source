#! gmake

DEPTH = ../../../..

include $(DEPTH)/config/config.mk

# This is required for compilation of only this directory
ifeq ($(OS_ARCH),HP-UX)
CFLAGS += $(DSO_CFLAGS)
DSO_LDOPTS += +e NP_GetValue +e NP_Initialize +e NP_Shutdown +e NP_GetMIMEDescription
endif

# On Linux ELF, motif cannot be used by plugin. So diable the motif part.
# and enable plugin trace
ifeq ($(OS_ARCH),Linux)
ifdef BUILD_UNIX_PLUGINS
CFLAGS += -DPLUGIN_TRACE -UTEXT_PLUGIN
endif
else
CFLAGS += -DTEXT_PLUGIN
endif

CSRCS = npshell.c stubs.c
OBJS  = $(CSRCS:.c=.o)

#
# Defines to build the unix plugin sdk automatically
#
SDK_FILES =	\
	README	\
	Test.html \
	Test.txt \
	makefile.hpux \
	makefile.osf1 \
	makefile.sgi \
	makefile.sol23 \
	makefile.sol24 \
	makefile.sun4 \
	makefile.linux \
	npshell.c \
	$(DEPTH)/cmd/xfe/plugins/common/npunix.c \
	$(DEPTH)/lib/plugin/npupp.h \
	$(DEPTH)/include/npapi.h \
	$(DEPTH)/include/jri.h \
	$(DEPTH)/include/jri_md.h

SDK_TEST_FILES =	\
	$(DEPTH)/cmd/xfe/plugins/test/text*


UNIX_SDK = unix-sdk

TARGETS = $(OBJDIR)/libtextplugin.so

default:: $(TARGETS) install

all:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin

unix-sdk:: $(SDK_FILES)
	echo "Generating unix plugin sdk..."; \
	rm -rf $(OBJDIR)/sdk; \
	mkdir -p $(OBJDIR)/sdk; \
	cp $(SDK_FILES) $(OBJDIR)/sdk/.; \
	mkdir -p $(OBJDIR)/sdk/test; \
	cp $(SDK_TEST_FILES) $(OBJDIR)/sdk/test/.; \
	cd $(OBJDIR)/sdk; \
	FILES=`echo *`; \
	tar cvf - $$FILES | compress -c > $(UNIX_SDK).tar.Z; \
	rm -rf $$FILES

include $(DEPTH)/config/rules.mk

# DEFINES  += -DPLUGIN_TRACE
INCLUDES += -I$(DEPTH)/lib/plugin
GARBAGE += so_locations $(UNIX_SDK)
OPTIMIZER=-g

$(OBJDIR)/libtextplugin.so: $(OBJS)
	$(LD) $(DSO_LDOPTS) -o $@ $(OBJS) $(DSO_LDFLAGS)
