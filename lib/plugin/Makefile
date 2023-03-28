#! gmake

DEPTH = ../..

include $(DEPTH)/config/config.mk

CSRCS =		  \
	npassoc.c \
	npglue.c  \
	upi.c	  \
	testupi.c \
	$(NULL)

ifndef NO_JAVA
CSRCS += npjava.c
endif
	
OBJS = $(CSRCS:.c=.o)

LIBRARY = libplugin.a

#DEFINES += -DTESTUPI

INCLUDES += -I../layout

ifndef NO_JAVA
JAVA_DIR = $(DEPTH)/sun-java
INCLUDES +=	-I$(JAVA_DIR)/netscape/applet/_gen
endif

include $(DEPTH)/config/rules.mk

