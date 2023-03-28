#! gmake

DEPTH = ../..

include $(DEPTH)/config/config.mk

SRCS =			\
	mbox.c		\
	leaf.c		\
	scan.c		\
	html.c		\
	pprint.c	\
	$(NULL)

OBJS = $(SRCS:.c=.o)

TARGETS = $(OBJDIR)/scan

all:: $(TARGETS)

include $(DEPTH)/config/rules.mk

LIBMIME_DIR     = $(DEPTH)/lib/libmime
LIBMSG_DIR	= $(DEPTH)/lib/libmsg
LIBXP_DIR	= $(DEPTH)/lib/xp
LIBNS_DIR	= $(DEPTH)/nspr/src

LIBMIME_BIN_DIR = $(LIBMIME_DIR)/$(OBJDIR_NAME)
LIBXP_BIN_DIR	= $(LIBXP_DIR)/$(OBJDIR_NAME)
LIBMSG_BIN_DIR	= $(LIBMSG_DIR)/$(OBJDIR_NAME)
LIBNS_BIN_DIR	= $(LIBNS_DIR)/$(OBJDIR_NAME)

LIBMIME		= $(LIBMIME_BIN_DIR)/libmime.a


# This crap cloned from libmime/Makefile; fixme, fixme.
# 
$(LIBMSG_BIN_DIR)/addr.o    \
$(LIBXP_BIN_DIR)/xp_file.o  \
$(LIBXP_BIN_DIR)/xp_str.o   \
$(LIBXP_BIN_DIR)/xp_intl.o  \
$(LIBXP_BIN_DIR)/xp_error.o \
$(LIBXP_BIN_DIR)/xp_trace.o \
$(LIBXP_BIN_DIR)/xp_reg.o   \
$(LIBXP_BIN_DIR)/xp_hash.o  \
$(LIBXP_BIN_DIR)/xp_list.o:
	@cd $(shell dirname $@)/../ ; \
	$(MAKE_OBJDIR) ; \
	$(MAKE) $(subst $(LIBXP_BIN_DIR)/,,$(subst $(LIBMSG_BIN_DIR)/,,$@))

BASE_MIME_STUBS =				\
	$(LIBMIME_BIN_DIR)/mimestub.o		\
	$(LIBMSG_BIN_DIR)/addr.o		\
	$(LIBXP_BIN_DIR)/xp_time.o		\
	$(LIBXP_BIN_DIR)/xp_file.o		\
	$(LIBXP_BIN_DIR)/xp_str.o		\
	$(LIBXP_BIN_DIR)/xp_intl.o		\
	$(LIBXP_BIN_DIR)/xp_error.o		\
	$(LIBXP_BIN_DIR)/xp_trace.o		\
	$(LIBXP_BIN_DIR)/xp_reg.o		\
	$(LIBXP_BIN_DIR)/xp_hash.o		\
	$(LIBXP_BIN_DIR)/xp_list.o		\
	$(LIBXP_BIN_DIR)/allxpstr.o		\
	$(LIBNS_BIN_DIR)/prprf.o		\
	$(NULL)

CRYPTO_MIME_STUBS =				\
	$(DIST)/lib/libpkcs7.a			\
	$(DIST)/lib/libsecmod.a			\
	$(DIST)/lib/libcert.a			\
	$(DIST)/lib/libkey.a			\
	$(DIST)/lib/libsecutil.a		\
	$(DIST)/lib/libhash.a			\
	$(DIST)/lib/libssl.a			\
	$(DIST)/lib/libcrypto.a			\
	$(DIST)/lib/libdbm.a			\
	$(DIST)/lib/nspr.a			\
	$(NULL)

MIME_STUBS = $(BASE_MIME_STUBS)

#MIME_SECURITY=1
ifdef MIME_SECURITY
  MIME_STUBS += $(CRYPTO_MIME_STUBS)
endif

INCLUDES	+= -I$(LIBMIME_DIR)

$(OBJDIR)/scan: $(OBJS) $(MIME_STUBS) $(LIBMIME)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LIBMIME) $(MIME_STUBS) $(LDFLAGS)

$(OBJDIR)/scan.pure: $(OBJS) $(LIBMIME)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(PURIFY) \
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LIBMIME) $(MIME_STUBS) $(LDFLAGS)

clean::
	-$(RM) $(OBJS)
	-$(RM) $(OBJDIR)/scan $(OBJDIR)/scan.pure

TAGS:: $(SRCS)
	$(ETAGS) *.[ch]
