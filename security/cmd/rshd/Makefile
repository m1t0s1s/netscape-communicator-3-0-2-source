#! gmake

DEPTH = ../../..

include $(DEPTH)/config/config.mk

HSRCS 	= ssldutil.h

CSRCS	= rshd.c ssldutil.c
OBJS	= $(CSRCS:.c=.o)

PROGS	= $(OBJDIR)/rshd

TARGETS = $(PROGS)

include $(DEPTH)/security/secrules.mk

DEFINES += -DUSE_SSL -D_BSD_SIGNALS -DBSD_COMP
INCLUDES += -I../include

$(PROGS):$(OBJDIR)/%: $(OBJS)
	$(CCF) -o $@ $(OBJS) $(LDFLAGS) $(OS_LIBS)

export:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin
