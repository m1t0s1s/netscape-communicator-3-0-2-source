#! gmake

NSPRDEPTH = ..

DIRS = os

include $(NSPRDEPTH)/config/config.mk

HEADERS = *.h $(OBJDIR)/prcpucfg.h
TARGETS = $(OBJDIR)/prcpucfg.h

include $(NSPRDEPTH)/config/rules.mk

$(OBJDIR)/prcpucfg.h: $(OBJDIR)/gencfg
	rm -f $@
	$(OBJDIR)/gencfg > $@

$(OBJDIR)/gencfg: $(OBJDIR)/gencfg.o
	$(CC) -o $@ $(OBJDIR)/gencfg.o

export:: $(HEADERS)
	$(INSTALL) -m 444 $(HEADERS) $(DIST)/include/nspr

install:: export

clobber::
	rm -f $(OBJDIR)/gencfg $(OBJDIR)/gencfg.o
