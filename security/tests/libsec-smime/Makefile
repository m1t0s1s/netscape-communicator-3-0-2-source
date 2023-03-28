#! gmake

DEPTH = ../../..

include $(DEPTH)/config/config.mk

CSRCS = \
	sim_pos_int.c \
	sim_neg_int.c \
	sim_zero_int.c \
	sim_null_int.c \
	sim_pos_ia5.c \
	sim_neg_ia5.c \
	sim_ctrl_ia5.c \
	sim_pos_objid.c \
	sim_neg_objid.c \
	sim_pos_utc.c \
	sim_pos_uint.c \
	sim_pos_print.c \
	str_pos_seq.c \

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
	$(DIST)/lib/libnspr.a \
#	$(DIST)/lib/libpurenspr.a \
	$(NULL)

#There is a circular dependancy in security/lib, and here is a gross fix
SEC_LIBS = \
	$(DIST)/lib/libsecnav.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libcert.a \
        $(DIST)/lib/libkey.a \
        $(DIST)/lib/libcrypto.a \
        $(DIST)/lib/libsecutil.a \
        $(DIST)/lib/libhash.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libcert.a \
        $(DIST)/lib/libkey.a \
        $(DIST)/lib/libcrypto.a \
	$(DIST)/lib/libsecutil.a \
	$(NULL)
  
MYLIB	= $(DIST)/lib/libsectool.a

US_LIBS	= $(MYLIB) $(SEC_LIBS) $(BASE_LIBS)
EX_LIBS	= $(MYLIB) $(SEC_LIBS) $(BASE_LIBS)

REQUIRES = libxp nspr security

OBJS	= $(CSRCS:.c=.o) 

PROGS		= $(addprefix $(OBJDIR)/, $(CSRCS:.c=))


NON_DIRS = $(PROGS)
TARGETS = $(DIRS) $(NON_DIRS)

include $(DEPTH)/config/rules.mk


$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(EXPORT_FLAGS) -c $*.c

$(PROGS):$(OBJDIR)/%: $(OBJDIR)/%.o $(EX_LIBS)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $@.o $(LDFLAGS) $(EX_LIBS) $(OS_LIBS)

export:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(NON_DIRS) $(DIST)/bin

clean:: rm *.a *.o
