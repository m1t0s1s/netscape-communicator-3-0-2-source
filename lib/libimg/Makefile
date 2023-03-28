#! gmake

DEPTH = ../..

CSRCS =		   \
	color.c	   \
	colormap.c \
	dither.c   \
	external.c \
	gif.c	   \
	if.c	   \
	ilclient.c \
	jpeg.c	   \
	scale.c	   \
	xbm.c	   \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libimg.a

include $(DEPTH)/config/rules.mk

# XXX should use $(DIST)/include/jpeg/*.h
INCLUDES += -I$(DEPTH)/jpeg

netscape-export netscape-export.pure: libimg.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
