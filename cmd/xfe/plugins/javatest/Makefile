#! gmake

DEPTH = ../../../..

# This is required for compilation of only this directory
ifeq ($(OS_ARCH),HP-UX)
CFLAGS += $(DSO_CFLAGS)
endif

LIBTARGETS = $(OBJDIR)/javatestplugin.so
TARGETS = targets

CSRCS = npshell.c stubs.c
OBJS  = $(CSRCS:.c=.o)

# Rules to build generated headers

JRI_HEADER_CLASSES =	       \
	JavaTestPlugin	       \
	java.lang.String       \
	java.awt.Window	       \
	java.awt.Component     \
	java.awt.Color	       \
	java.lang.Class

JRI_STUB_CLASSES =	       \
	JavaTestPlugin	       \
	java.lang.String       \
	java.awt.Window	       \
	java.awt.Component     \
	java.awt.Color	       \
	java.lang.Class

include $(DEPTH)/config/rules.mk

CLASSSRC := .:$(DEPTH)/sun-java/classsrc

INCLUDES += -I_gen

targets:
	@$(MAKE) classes
	@$(MAKE) jri_headers jri_stubs
	@$(MAKE) libtargets

JAVAC =	$(JAVAI) -classpath /usr/local/netscape/java/lib/javac.zip \
	     -ms8m sun.tools.javac.Main

classes: JavaTestPlugin.java
	$(JAVAC) -classpath $(CLASSSRC) JavaTestPlugin.java

libtargets: $(LIBTARGETS)

$(OBJDIR)/javatestplugin.so: $(OBJS)
	$(LD) $(DSO_LDOPTS) -o $@ $(OBJS) $(DSO_LDFLAGS)

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin

INCLUDES += -I$(DEPTH)/lib/plugin
GARBAGE += so_locations

clobber::
	rm -rf _gen _stubs so_locations

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export
