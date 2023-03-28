#! gmake

DEPTH	= ..

CSRCS	= xrescont.c
OBJS	= $(CSRCS:.c=.o)

PROGRAM	= xrescont

DIRS	= us

#
# Always build the us version first, so any of the others can fail
# without affecting the us/export build.
#
ifdef BUILD_INTL
#DIRS	+= tools de fr ja_euc ja it sv es nl pt ko da
DIRS	+= tools de fr ja_euc ja
endif

include $(DEPTH)/config/rules.mk
