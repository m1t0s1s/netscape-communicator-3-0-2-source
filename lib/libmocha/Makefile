#! gmake

DEPTH = ../..

include $(DEPTH)/config/config.mk

ifndef NO_MOCHA

INCLUDES += -I../layout -I../libnet -I../libparse -I../libimg \
	    -I$(DIST)/include/mocha

CSRCS =              \
	  lm_applt.c \
	  lm_doc.c   \
	  lm_embed.c \
	  lm_form.c  \
	  lm_hist.c  \
	  lm_href.c  \
	  lm_img.c   \
	  lm_init.c  \
	  lm_input.c \
	  lm_msg.c   \
	  lm_nav.c   \
	  lm_plgin.c \
	  lm_prefs.c \
	  lm_table.c \
	  lm_taint.c \
	  lm_url.c   \
	  lm_win.c   \
	  $(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libmocha.a

endif

include $(DEPTH)/config/rules.mk

# add to the include path for lm_applt.c
APPLET_CFLAGS = $(CFLAGS)		\
        -I$(DEPTH)/sun-java/include	\
        -I$(DEPTH)/sun-java/md-include	\
        -I$(DEPTH)/sun-java/netscape/applet/_gen \
        -I$(DEPTH)/lib/libjava

$(OBJDIR)/lm_applt.o: lm_applt.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(APPLET_CFLAGS) lm_applt.c

EMBED_CFLAGS = $(CFLAGS)		\
        -I$(DEPTH)/lib/plugin

$(OBJDIR)/lm_embed.o: lm_embed.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(EMBED_CFLAGS) lm_embed.c

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
#   LJM - Added same kludge to ns tree. :)
netscape-export netscape-export.pure: libmocha.a
	cd $(DEPTH)/lib/xp; $(MAKE) libxp.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
