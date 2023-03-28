#! gmake

DEPTH = ../../..

include $(DEPTH)/config/config.mk

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
	$(NULL)

LIBS = \
	../lib/$(OBJDIR)/libsectool.a \
	$(SEC_LIBS)\
	$(DIST)/lib/libdbm.a \
	$(DIST)/lib/libxp.a \
	$(DIST)/lib/libnspr.a \
	$(NULL)


HSRCS	= pathnames.h

CSRCS	= rsh.c sslrcmd.c
OBJS	= $(CSRCS:.c=.o)

PROGS	= $(OBJDIR)/rsh

TARGETS = $(PROGS)

include $(DEPTH)/security/secrules.mk

DEFINES += -DUSE_SSL -D_BSD_SIGNALS -DBSD_COMP
INCLUDES += \
	-I../include \
	-I$(DIST)/../public/security \
	-I$(DEPTH)/security/lib/cert \
	-I$(DEPTH)/security/lib/key \
	-I$(DEPTH)/security/lib/util \
	-I$(DIST)/../public/nspr \
	$(NULL)

ifeq ($(OS_ARCH),SunOS)
OS_LIBS += /usr/ucblib/libucb.a
endif

$(PROGS):$(OBJDIR)/%: $(OBJS) $(LIBS)
	$(CCF) -o $@ $(OBJS) $(LDFLAGS) $(LIBS) $(OS_LIBS)

export:: $(TARGETS)

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin
