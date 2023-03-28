SHELL		= /bin/sh
DEPTH		= ../../..

include $(DEPTH)/config/config.mk

BINDIR		= $(OBJDIR)
CFLAGS		= -O -DNETSCAPE $(PLAT_SPECIFIC)
RM		= /bin/rm -rf

# Platform-specific (e.g., DOS -- obj,exe,com).
BINSUFFIX	=
OBJSUFFIX	= o
O2C_RULE	= $(@F:.o=.c)

# gmake-specific
ifeq ($(OS_ARCH),UNIX_SV)
PLAT_SPECIFIC	= -DUSG
endif

ifeq ($(OS_ARCH),SCO_SV)
PLAT_SPECIFIC	= -DUSG
endif

ifeq ($(OS_ARCH),SunOS)
ifneq ($(OS_RELEASE),4.1.3_U1)
PLAT_SPECIFIC	= -DUSG
endif
endif

SRCS		= movemail.c
OBJS		= $(OBJDIR)/movemail.$(OBJSUFFIX)
TRASH		= ,* *~ .*~ core $(OBJDIR)

TARGETS		= $(BINDIR)/movemail$(BINSUFFIX)

############################################################################

all export install default:		$(OBJDIR) $(SRCS) $(TARGETS)

$(BINDIR)/movemail$(BINSUFFIX):		$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@
	@strip $@

$(OBJS):
	$(CC) $(CFLAGS) -c -o $@ $(O2C_RULE)

$(OBJDIR):
	@-mkdir $@

clean clobber clobber_all:
	$(RM) $(TARGETS) $(TRASH)

#
# Dependencies.
#
# Note: each object file must depend on at least the associated source file.
#
$(OBJDIR)/movemail.$(OBJSUFFIX):	movemail.c
