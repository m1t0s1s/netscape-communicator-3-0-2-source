#! gmake

DEPTH = ../..

CSRCS =		   \
	pa_amp.c   \
	pa_hash.c  \
	pa_mdl.c   \
	pa_parse.c \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libparse.a

include $(DEPTH)/config/rules.mk

ifndef NO_MOCHA
DEFINES += -DMOCHA
endif

# XXX this should actually be "pa_hash.c pa_hash.rmap:" and be in the GARBAGE
# XXX list and so on, but we don't have perl everywhere yet...
#
# XXX also, should share string constants between pa_hash.c and pa_hash.rmap

hash: pa_tags.h gentab.pl pa_hash.template
	@echo Generating pa_hash.c from pa_tags.h and pa_hash.template
	@rm -f pa_hash.c
	@./gentab.pl > pa_hash.c
