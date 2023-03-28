#! gmake

DEPTH = ../../..

include $(DEPTH)/sun-java/config/config.mk

CSRCS =			 \
	awt_Button.c	 \
	awt_Canvas.c	 \
	awt_Checkbox.c	 \
	awt_Component.c	 \
	awt_Dialog.c	 \
	awt_FileDialog.c \
	awt_Font.c	 \
	awt_Frame.c	 \
	awt_Graphics.c	 \
	awt_Label.c	 \
	awt_List.c	 \
	awt_Menu.c	 \
	awt_MenuBar.c	 \
	awt_MenuItem.c	 \
	awt_MToolkit.c	 \
	awt_Scrollbar.c	 \
	awt_TextArea.c	 \
	awt_TextField.c	 \
	awt_Window.c	 \
	awt_util.c	 \
	canvas.c	 \
	color.c		 \
	image.c		 \
	imageconvert.c	 \
	stubs.c		 \
	awt_nodl.c	 \
	$(NULL)

I18NSRCS =	  \
	ustring.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o) $(I18NSRCS:.c=.o)
XSO_OBJS = $(CSRCS:.c=.o) i18nstubs.o
SO_OBJS	:= $(addprefix $(OBJDIR)/, $(XSO_OBJS))

TARGETS = targets

LIBTARGETS = $(OBJDIR)/libawt.a
ifdef MKSHLIB
LIBTARGETS += $(OBJDIR)/libawt.so
endif

GENERATED_HEADERS_CLASSFILES =		    \
	java.awt.Button			    \
	java.awt.Canvas			    \
	java.awt.CheckboxGroup		    \
	java.awt.Checkbox		    \
	java.awt.Color			    \
	java.awt.Component		    \
	java.awt.Dialog			    \
	java.awt.Event			    \
	java.awt.FileDialog		    \
	java.awt.Font			    \
	java.awt.FontMetrics		    \
	java.awt.Frame			    \
	java.awt.Graphics		    \
	java.awt.Image			    \
	java.awt.Insets			    \
	java.awt.Label			    \
	java.awt.List			    \
	java.awt.Menu			    \
	java.awt.MenuBar		    \
	java.awt.MenuItem		    \
	java.awt.Rectangle		    \
	java.awt.Scrollbar		    \
	java.awt.TextField		    \
	java.awt.Window			    \
	java.awt.image.ColorModel	    \
	java.awt.image.ImageConsumer	    \
	java.awt.image.ImageObserver	    \
	java.awt.image.IndexColorModel	    \
	java.awt.image.DirectColorModel	    \
	sun.awt.image.Image		    \
	sun.awt.image.ImageRepresentation   \
	sun.awt.image.OffScreenImageSource  \
	sun.awt.motif.InputThread	    \
	sun.awt.motif.MButtonPeer	    \
	sun.awt.motif.MCanvasPeer	    \
	sun.awt.motif.MComponentPeer	    \
	sun.awt.motif.MCheckboxPeer	    \
	sun.awt.motif.MCheckboxMenuItemPeer \
	sun.awt.motif.MDialogPeer	    \
	sun.awt.motif.MFileDialogPeer	    \
	sun.awt.motif.MFramePeer	    \
	sun.awt.motif.MLabelPeer	    \
	sun.awt.motif.MListPeer		    \
	sun.awt.motif.MMenuBarPeer	    \
	sun.awt.motif.MMenuItemPeer	    \
	sun.awt.motif.MMenuPeer		    \
	sun.awt.motif.MScrollbarPeer	    \
	sun.awt.motif.MTextAreaPeer	    \
	sun.awt.motif.MTextFieldPeer	    \
	sun.awt.motif.MToolkit		    \
	sun.awt.motif.MWindowPeer	    \
	sun.awt.motif.X11FontMetrics	    \
	sun.awt.motif.X11Graphics	    \
	sun.awt.motif.X11Image		    \
	netscape.applet.MozillaAppletContext\
	netscape.applet.EmbeddedAppletFrame \
	$(NULL)

GENERATED_STUBS_CLASSFILES =		    \
	sun.awt.image.ImageRepresentation   \
	sun.awt.image.OffScreenImageSource  \
	sun.awt.motif.InputThread	    \
	sun.awt.motif.MButtonPeer	    \
	sun.awt.motif.MCanvasPeer	    \
	sun.awt.motif.MCheckboxPeer	    \
	sun.awt.motif.MCheckboxMenuItemPeer \
	sun.awt.motif.MComponentPeer	    \
	sun.awt.motif.MDialogPeer	    \
	sun.awt.motif.MFileDialogPeer	    \
	sun.awt.motif.MFramePeer	    \
	sun.awt.motif.MLabelPeer	    \
	sun.awt.motif.MListPeer		    \
	sun.awt.motif.MMenuBarPeer	    \
	sun.awt.motif.MMenuItemPeer	    \
	sun.awt.motif.MMenuPeer		    \
	sun.awt.motif.MScrollbarPeer	    \
	sun.awt.motif.MTextAreaPeer	    \
	sun.awt.motif.MTextFieldPeer	    \
	sun.awt.motif.MToolkit		    \
	sun.awt.motif.MWindowPeer	    \
	sun.awt.motif.X11FontMetrics	    \
	sun.awt.motif.X11Graphics	    \
	$(NULL)

include $(DEPTH)/sun-java/config/rules.mk

AWT_DEPLIBS = -lXm -lgen -lXt -lX11
ifeq ($(OS_RELEASE),4.1.3_U1)
AWT_DEPLIBS = -lXm -lXt -lX11
endif
ifeq ($(OS_ARCH),HP-UX)
AWT_DEPLIBS =			   \
	/usr/lib/Motif1.2/libXm.sl \
	/usr/lib/X11R5/libXt.sl	   \
	/usr/lib/X11R5/libX11.sl
OS_CFLAGS += +z	# position-independent code
endif

targets:
	@$(MAKE) headers stubs
	@$(MAKE) libtargets

libtargets: $(LIBTARGETS)

$(OBJDIR)/libawt.so: $(SO_OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(MKSHLIB) -o $@ $(SO_OBJS) $(AWT_DEPLIBS)

$(OBJDIR)/libawt.a: $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

export:: targets
	$(INSTALL) $(LIBTARGETS) $(DIST)/lib

install:: export

######################################################################

# Generate awt_nodl.c (so that you can check it in)
# These make rules only works on IRIX...sigh

ifeq ($(OS_ARCH),IRIX)

awt_nodl.c: $(OBJDIR)/stubs.o Makefile $(DEPTH)/sun-java/config/nodl.pl
	rm -f $@
	perl $(DEPTH)/sun-java/config/nodl.pl "awt_nodl_tab" \
	    `nm -Bn $(OBJDIR)/stubs.o | egrep Java_.*_stub | awk '{print $$3;}'` > $@

endif

######################################################################

# Rules to build generated headers

INCLUDES += -I_gen

clobber::
	rm -rf _gen _stubs so_locations
