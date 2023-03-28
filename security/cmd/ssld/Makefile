#! gmake

DEPTH = ../../..

include $(DEPTH)/config/config.mk

US_FLAGS = -DEXPORT_VERSION -DUS_VERSION
EXPORT_FLAGS = -DEXPORT_VERSION

BASE_LIBS = \
	$(DIST)/lib/libdbm.a \
	$(DIST)/lib/libxp.a \
	$(DIST)/lib/libnspr.a \
	$(NULL)

OTHER_SRCS = ssld_conf.c
OTHER_OBJS = $(addprefix $(OBJDIR)/, $(OTHER_SRCS:.c=.o))

MYLIB = ../lib/$(OBJDIR)/libsecutil.a

US_LIBS	= $(OTHER_OBJS) $(MYLIB) $(DIST)/lib/libsec-us.a     $(BASE_LIBS)
EX_LIBS	= $(OTHER_OBJS) $(MYLIB) $(DIST)/lib/libsec-export.a $(BASE_LIBS)


# files that generate two separate objects and executables
BI_SRCS	= ssld.c

#CSRCS	= $(BI_SRCS) $(OTHER_SRCS)
OBJS	= $(OTHER_SRCS:.c=.o) $(BI_SRCS:.c=-us.o) $(BI_SRCS:.c=-ex.o)

US_PROGS 	= $(addprefix $(OBJDIR)/, $(BI_SRCS:.c=-us))
EX_PROGS	= $(addprefix $(OBJDIR)/, $(BI_SRCS:.c=-ex))


TARGETS = $(US_PROGS) $(EX_PROGS)

INCLUDES += -I../include

include $(DEPTH)/security/secrules.mk


$(OBJDIR)/%-us.o: %.c
	$(CCF) -o $@ $(US_FLAGS) -c $*.c

$(OBJDIR)/%-ex.o: %.c
	$(CCF) -o $@ $(EXPORT_FLAGS) -c $*.c

$(US_PROGS):$(OBJDIR)/%-us: $(US_LIBS) $(OBJDIR)/%-us.o
	$(CCF) -o $@ $(OBJDIR)/$*-us.o $(LDFLAGS) $(US_LIBS) $(OS_LIBS)

$(EX_PROGS):$(OBJDIR)/%-ex: $(EX_LIBS) $(OBJDIR)/%-ex.o
	$(CCF) -o $@ $(OBJDIR)/$*-ex.o $(LDFLAGS) $(EX_LIBS) $(OS_LIBS)

export:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin
