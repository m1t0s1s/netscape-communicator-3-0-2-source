#! gmake

DEPTH = ../..

include $(DEPTH)/config/config.mk

#CSRCS = \
#	atob.c \
#	berdump.c \
#	btoa.c \
#	certutil.c \
#	checkcert.c \
#	derdump.c \
#	dumpcert.c \
#	listcerts.c \
#	mkdongle.c \
#	pp.c \
#	vector.c \
#	$(NULL)


# the base files for the executables
# hey -- keep these alphabetical, please
EXEC_SRCS = \
	atob.c \
	btoa.c \
	certutil.c \
	crlutil.c \
	checkcert.c \
	derdump.c \
	digest.c \
	dumpcert.c \
	listcerts.c \
	oidcalc.c \
	p7content.c \
	p7sign.c \
	p7verify.c \
	pp.c	  \
	rsaperf.c \
	selfserv.c \
	$(NULL)

# files that generate two separate objects and executables
BI_SRCS	= \
	keyutil.c \
	p7env.c \
	tstclnt.c \
	$(NULL)


INCLUDES += \
	-I$(DIST)/../public/security \
	-I$(DEPTH)/security/lib/cert \
	-I$(DEPTH)/security/lib/key \
	-I$(DEPTH)/security/lib/util  \
	-I./include \
	$(NULL)

DIRS = lib

TBD_DIRS = rsh rshd rdist ssld

# For the time being, sec stuff is export only
# US_FLAGS = -DEXPORT_VERSION -DUS_VERSION

US_FLAGS = -DEXPORT_VERSION
EXPORT_FLAGS = -DEXPORT_VERSION

BASE_LIBS = \
	$(DIST)/lib/libdbm.a \
	$(DIST)/lib/libxp.a \
	$(DIST)/lib/nspr.a \
#	$(DIST)/lib/libpurenspr.a \
	$(NULL)

#There is a circular dependancy in security/lib, and here is a gross fix
SEC_LIBS = \
	$(DIST)/lib/libsecnav.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libcert.a \
        $(DIST)/lib/libkey.a \
	$(DIST)/lib/libsecmod.a \
        $(DIST)/lib/libcrypto.a \
        $(DIST)/lib/libsecutil.a \
        $(DIST)/lib/libhash.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libcert.a \
	$(DIST)/lib/libsecmod.a \
        $(DIST)/lib/libkey.a \
        $(DIST)/lib/libcrypto.a \
        $(DIST)/lib/libpkcs7.a \
	$(DIST)/lib/libsecutil.a \
	$(NULL)

MYLIB	= lib/$(OBJDIR)/libsectool.a

US_LIBS	= $(MYLIB) $(SEC_LIBS) $(BASE_LIBS)
EX_LIBS	= $(MYLIB) $(SEC_LIBS) $(BASE_LIBS)

REQUIRES = libxp nspr security

CSRCS	= $(EXEC_SRCS) $(BI_SRCS)

OBJS	= $(CSRCS:.c=.o) $(BI_SRCS:.c=-us.o) $(BI_SRCS:.c=-ex.o)

PROGS		= $(addprefix $(OBJDIR)/, $(EXEC_SRCS:.c=))
US_PROGS 	= $(addprefix $(OBJDIR)/, $(BI_SRCS:.c=-us))
EX_PROGS	= $(addprefix $(OBJDIR)/, $(BI_SRCS:.c=-ex))


NON_DIRS = $(PROGS) $(US_PROGS) $(EX_PROGS)
TARGETS = $(NON_DIRS)

include $(DEPTH)/security/secrules.mk


$(OBJDIR)/%-us.o: %.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(US_FLAGS) -c $*.c

$(OBJDIR)/%-ex.o: %.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(EXPORT_FLAGS) -c $*.c

$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(EXPORT_FLAGS) -c $*.c

$(US_PROGS):$(OBJDIR)/%-us: $(OBJDIR)/%-us.o $(US_LIBS)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(OBJDIR)/$*-us.o $(LDFLAGS) $(US_LIBS) $(OS_LIBS)

$(EX_PROGS):$(OBJDIR)/%-ex: $(OBJDIR)/%-ex.o $(EX_LIBS)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(OBJDIR)/$*-ex.o $(LDFLAGS) $(EX_LIBS) $(OS_LIBS)

$(PROGS):$(OBJDIR)/%: $(OBJDIR)/%.o $(EX_LIBS)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $@.o $(LDFLAGS) $(EX_LIBS) $(OS_LIBS)

export:: 

libs:: $(TARGEST)

install:: $(TARGETS)
	$(INSTALL) $(NON_DIRS) $(DIST)/bin
