#! gmake

NSPRDEPTH = ..

CSRCS = sysinfo.c suspending.c hello.c block.c create.c preempt.c mutex.c stdio.c dns.c \
	pipe.c socket.c sprintf.c locker.c log.c usdate.c dtoa.c
PROGS = $(addprefix $(OBJDIR)/, $(CSRCS:.c=))

TARGETS = $(PROGS)

include $(NSPRDEPTH)/config/rules.mk

ifeq ($(OS_ARCH),AIX)
$(OBJDIR)/%: %.c
	@$(MAKE_OBJDIR)
	rm -f $@ $(AIX_TMP)
	$(CCF) $*.c $(AIX_LINK_OPTS) $(LIBNSPR) $(OS_LIBS) -o $(AIX_TMP)
	$(CC) -o $@ $(AIX_TMP) $(AIX_WRAP) $(AIX_FINAL_LDFLAGS)
	rm -f $(AIX_TMP)
else
$(OBJDIR)/%: %.c
	@$(MAKE_OBJDIR)
	$(CCF) $*.c $(LIBNSPR) $(OS_LIBS) -o $@
endif

#ifdef HAVE_PURIFY
$(OBJDIR)/%.pure: $(OBJDIR)/%.o
	@$(MAKE_OBJDIR)
	$(PURIFY) $(CCF) $(OBJDIR)/$*.o $(PURELIBNSPR) $(OS_LIBS) -o $@
#endif

$(PROGS): $(LIBNSPR)

install:: $(TARGETS)
export:: all
clean::
	rm -f $(TARGETS)
