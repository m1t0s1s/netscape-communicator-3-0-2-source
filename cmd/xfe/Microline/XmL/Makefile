#! gmake

DEPTH = ../../../..

CSRCS =		   \
	Folder.c   \
	Grid.c	   \
	Progress.c \
	XmL.c	   \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libXmL.a

include $(DEPTH)/config/rules.mk

INCLUDES += -I..
