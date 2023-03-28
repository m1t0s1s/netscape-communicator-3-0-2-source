#! gmake

NSPRDEPTH = ..

include $(NSPRDEPTH)/config/config.mk

CFILES=		malloc.c	\
		datamc.c	\
		datams.c	\
		dgmalloc.c	\
		fill.c		\
		free.c		\
		realloc.c	\
		calloc.c	\
		string.c	\
		mcheck.c	\
		mchain.c	\
		memory.c	\
		tostring.c	\
		m_perror.c	\
		m_init.c	\
		mallopt.c	\
		dump.c 		\
		stack.c		\
		xmalloc.c	\
		xheap.c		\
		malign.c	\
		size.c		\
		abort.c		\
		leak.c	

OBJS		= $(CFILES:.c=.o)

LIBRARY		= dbmalloc.a

TARGETS		= $(LIBRARY)

INCLUDES += -I../include

include $(NSPRDEPTH)/config/rules.mk

export:: $(TARGETS)
	$(INSTALL) -m 444 $(TARGETS) $(DIST)/lib

install:: export
