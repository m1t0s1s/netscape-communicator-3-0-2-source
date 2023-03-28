#! gmake

DEPTH = ../../..

include $(DEPTH)/sun-java/config/config.mk

CSRCS =			    \
	tiny_Font.c	    \
	tiny_Graphics.c	    \
	tiny_Toolkit.c	    \
	tiny_Window.c	    \
	stubs.c		    \
	../x/color.c	    \
	../x/image.c	    \
	../x/imageconvert.c \
	$(NULL)

LIBOBJS := $(addprefix $(OBJDIR)/,$(notdir $(CSRCS:.c=.o)))
SO_OBJS	:= $(LIBOBJS)

TARGETS = targets

LIBTARGETS = $(OBJDIR)/libtawt.a
ifdef MKSHLIB
LIBTARGETS += $(OBJDIR)/libtawt.so
endif

GENERATED_HEADERS_CLASSFILES =		   \
	java.awt.Color			   \
	java.awt.Event			   \
	java.awt.Font			   \
	java.awt.Frame			   \
	java.awt.Graphics		   \
	java.awt.Rectangle		   \
	java.awt.image.ColorModel	   \
	java.awt.image.DirectColorModel \
	java.awt.image.ImageConsumer	   \
	java.awt.image.ImageObserver	   \
	sun.awt.image.Image		   \
	sun.awt.image.ImageRepresentation  \
	sun.awt.image.OffScreenImageSource \
	sun.awt.tiny.TinyEventThread	   \
	sun.awt.tiny.TinyFontMetrics	   \
	sun.awt.tiny.TinyGraphics	   \
	sun.awt.tiny.TinyInputThread	   \
	sun.awt.tiny.TinyToolkit	   \
	sun.awt.tiny.TinyWindow		   \
	$(NULL)

GENERATED_STUBS_CLASSFILES =		   \
	sun.awt.image.ImageRepresentation  \
	sun.awt.image.OffScreenImageSource \
	sun.awt.tiny.TinyEventThread	   \
	sun.awt.tiny.TinyFontMetrics	   \
	sun.awt.tiny.TinyGraphics	   \
	sun.awt.tiny.TinyInputThread	   \
	sun.awt.tiny.TinyToolkit	   \
	sun.awt.tiny.TinyWindow		   \
	$(NULL)

include $(DEPTH)/sun-java/config/rules.mk

AWT_DEPLIBS = -lX11
ifeq ($(OS_RELEASE),4.1.3_U1)
AWT_DEPLIBS = -lXm -lXt -lX11
endif
ifeq ($(OS_ARCH),HP-UX)
AWT_DEPLIBS =			   \
	/usr/lib/X11R5/libX11.sl
OS_CFLAGS += +z	# position-independent code
endif

INCLUDES += -I../x

targets:
	@$(MAKE) headers stubs
	@$(MAKE) libtargets

libtargets: $(LIBTARGETS)

$(OBJDIR)/libtawt.so: $(SO_OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(MKSHLIB) -o $@ $(SO_OBJS) $(AWT_DEPLIBS)

$(OBJDIR)/libtawt.a: $(LIBOBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(LIBOBJS)
	$(RANLIB) $@

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export

######################################################################

# Generate tawt_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),IRIX)

tawt_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	rm -f $@
	perl $(DEPTH)/sun-java/config/nodl.pl "tawt_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

# Rules to build generated headers

INCLUDES += -I_gen

clobber::
	rm -rf _gen _stubs so_locations

$(OBJDIR)/color.o: ../x/color.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/image.o: ../x/image.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<

$(OBJDIR)/imageconvert.o: ../x/imageconvert.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ -c $<
