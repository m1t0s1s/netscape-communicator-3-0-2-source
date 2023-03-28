#! gmake

DEPTH = ../..

CSRCS =		    \
	nscstr.c   \
	autocvt.c   \
	b52cns.c    \
	cns2b5.c    \
	cvchcode.c  \
	euc2jis.c   \
	euc2sjis.c  \
	euckr2is.c  \
	fe_ccc.c    \
	is2euckr.c  \
	jis2oth.c   \
	npc.c       \
	sjis2euc.c  \
	sjis2jis.c  \
	testnpc.c   \
	uscan.c      \
	ugen.c      \
	ugendata.c  \
	umap.c      \
	ucs2.c      \
	$(NULL)

# testnpc.c

OBJS = $(CSRCS:.c=.o)

LIBRARY = libi18n.a

include $(DEPTH)/config/rules.mk

DEFINES += -DHAS_FE

INCLUDES += -Iunicode/ufrmtbl -Iunicode/utotbl
