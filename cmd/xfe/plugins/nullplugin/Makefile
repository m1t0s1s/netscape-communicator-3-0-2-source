#! gmake

DEPTH = ../../../..

include $(DEPTH)/config/config.mk

# This is required for compilation of only this directory
ifeq ($(OS_ARCH),HP-UX)
CFLAGS += $(DSO_CFLAGS)
DSO_LDOPTS += +e NP_GetValue +e NP_Initialize +e NP_Shutdown +e NP_GetMIMEDescription
endif

CSRCS = npshell.c stubs.c nullplugin.c
OBJS  = $(CSRCS:.c=.o)

TARGETS = $(OBJDIR)/libnullplugin.so

default:: $(TARGETS) install

all:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin

include $(DEPTH)/config/rules.mk

# DEFINES  += -DPLUGIN_TRACE
INCLUDES += -I$(DEPTH)/lib/plugin
GARBAGE += so_locations

$(OBJDIR)/libnullplugin.so: $(OBJS)
	$(LD) $(DSO_LDOPTS) -o $@ $(OBJS) $(DSO_LDFLAGS)
