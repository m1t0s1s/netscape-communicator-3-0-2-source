#! gmake

DEPTH = ..

MSRCS =		      \
	Makefile      \
	IRIX5.3	      \
	SunOS4.1.3_U1 \
	SunOS5.3      \
	config.mk     \
	rules.mk      \
	$(NULL)

HSRCS	= pathsub.h
CSRCS	= nsinstall.c pathsub.c

PLSRCS	= nfspwd.pl revdepth.pl

PROGRAM	= nsinstall
OBJS	= $(CSRCS:.c=.o)

TARGETS = $(PROGRAM) $(PLSRCS:.pl=)

# IMPORTANT: Disable NSBUILDROOT for this directory only, otherwise we have
# a recurvise rule for finding nsinstall and the perl scripts
ifdef NSBUILDROOT
override NSBUILDROOT :=
endif

include $(DEPTH)/config/rules.mk

# Redefine MAKE_OBJDIR for just this directory
define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); mkdir $(@D); fi
endef

export:: $(TARGETS)
install:: $(TARGETS)
