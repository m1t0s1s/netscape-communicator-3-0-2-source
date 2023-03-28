#! gmake

DEPTH = ../../..

include $(DEPTH)/config/config.mk

# the base files for the executables

EXEC_SRCS = \
	zigbert.c \
	$(NULL)


INCLUDES += \
	-I$(DIST)/../public/security \
	-I$(DEPTH)/security/lib/cert \
	-I$(DEPTH)/security/lib/key \
	-I$(DEPTH)/security/lib/util  \
	-I$(DEPTH)/lib/libjar \
	-I../include \
	$(NULL)

US_FLAGS = -DEXPORT_VERSION
EXPORT_FLAGS = -DEXPORT_VERSION

BASE_LIBS = \
	$(DIST)/lib/libdbm.a \
	$(DIST)/lib/libxp.a \
	$(DIST)/lib/libnspr.a \
	$(DEPTH)/lib/libjar/$(OBJDIR)/libjar.a \
	$(DEPTH)/modules/zlib/src/$(OBJDIR)/libzlib.a \
	$(NULL)

#There is a circular dependancy in security/lib, and here is a gross fix

SEC_LIBS = \
        $(DIST)/lib/libsecnav.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libsecmod.a \
        $(DIST)/lib/libcert.a \
        $(DIST)/lib/libkey.a \
        $(DIST)/lib/libcrypto.a \
        $(DIST)/lib/libsecutil.a \
        $(DIST)/lib/libhash.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libsecmod.a \
        $(DIST)/lib/libcert.a \
        $(DIST)/lib/libkey.a \
        $(DIST)/lib/libcrypto.a \
        $(DIST)/lib/libsecutil.a \
        $(NULL)
 
MYLIB   = ../lib/$(OBJDIR)/libsectool.a 

LIBS	= $(MYLIB) $(SEC_LIBS) $(BASE_LIBS) $(SEC_LIBS) $(BASE_LIBS)

REQUIRES = libxp nspr security

CSRCS	= $(EXEC_SRCS) 

OBJS	= $(CSRCS:.c=.o) 

PROGS		= $(addprefix $(OBJDIR)/, $(EXEC_SRCS:.c=))


NON_DIRS = $(PROGS) 
TARGETS = $(NON_DIRS)

include $(DEPTH)/security/secrules.mk


$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(EXPORT_FLAGS) -c $*.c

$(PROGS):$(OBJDIR)/%: $(OBJDIR)/%.o $(LIBS)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $@.o $(LDFLAGS) $(LIBS) $(OS_LIBS)

export:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(NON_DIRS) $(DIST)/bin
