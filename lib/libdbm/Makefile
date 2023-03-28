#! gmake

DEPTH = ../..

CSRCS =		   \
	db.c	   \
	h_bigkey.c \
	h_func.c   \
	h_log2.c   \
	h_page.c   \
	hash.c	   \
	hash_buf.c \
	hsearch.c  \
	memmove.c  \
	mktemp.c   \
	ndbm.c	   \
	snprintf.c \
	strerror.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libdbm.a

include $(DEPTH)/config/rules.mk

# XXXbe required because $(DEPTH)/include/mcom_db.h includes "compat.h"
INCLUDES += -I.

DEFINES += -DMEMMOVE -D__DBINTERFACE_PRIVATE $(SECURITY_FLAG)
