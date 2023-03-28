#! gmake

DEPTH = ../..

CSRCS =		   \
	addr.c	   \
	compose.c  \
	composew.c \
	flag.c	   \
	folders.c  \
	mailbox.c  \
	mailsum.c  \
	messages.c \
	mimehtml.c \
	msgbiff.c  \
	msginit.c  \
	msgundo.c  \
	msgutils.c \
	newsgrp.c  \
	newsrc.c   \
	spool.c	   \
	threads.c  \
		   \
	ad_strm.c  \
	appledbl.c \
	ap_decod.c \
	ap_encod.c \
	bh_strm.c  \
	m_binhex.c \
	$(NULL)

# ifdef HAVE_SECURITY
 CSRCS += composec.c
# endif

OBJS = $(CSRCS:.c=.o)

LIBRARY = libmsg.a

include $(DEPTH)/config/rules.mk

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: libmsg.a
	cd $(DEPTH)/lib/libmime; $(MAKE) libmime.a
	cd $(DEPTH)/lib/libnet; $(MAKE) libnet.a
	cd $(DEPTH)/lib/xp; $(MAKE) libxp.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
