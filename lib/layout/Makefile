#! gmake

DEPTH = ../..

include $(DEPTH)/config/config.mk

CSRCS =		    \
	bullet.c    \
	clipline.c  \
	layarena.c  \
	laycell.c   \
	laycols.c   \
	laydisp.c   \
	layembed.c  \
	layfind.c   \
	layform.c   \
	layfree.c   \
	laygrid.c   \
	layhrule.c  \
	layimage.c  \
	layinfo.c   \
	laylist.c   \
	laymap.c    \
	layout.c    \
	laysel.c    \
	layspace.c  \
	laysub.c    \
	laytable.c  \
	laytags.c   \
	laytext.c   \
	layutil.c   \
	ptinpoly.c  \
	$(NULL)

ifndef NO_MOCHA
CSRCS += laymocha.c layscrip.c

endif

ifndef NO_JAVA
CSRCS += layjava.c
endif

ifdef BUILD_EDITOR
CSRCS += layedit.c
endif

OBJS = $(CSRCS:.c=.o)

ifdef BUILD_EDITOR
C++SRCS = editor.cpp edtbuf.cpp edtsave.cpp edtcmd.cpp edtele.cpp edtutil.cpp
OBJS += $(C++SRCS:.cpp=.o)
endif

LIBRARY = liblay.a

INCLUDES += -I../libparse

include $(DEPTH)/config/rules.mk

# add to the include path for laymocha.c
MOCHA_CFLAGS = $(CFLAGS)		\
        -I$(DEPTH)/mocha/include

$(OBJDIR)/laymocha.o: laymocha.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(MOCHA_CFLAGS) laymocha.c

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: $(LIBRARY)
	cd $(DEPTH)/security/lib/nav; $(MAKE)
	cd $(DEPTH)/lib/libparse; $(MAKE)
	cd $(DEPTH)/lib/xp; $(MAKE)
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
