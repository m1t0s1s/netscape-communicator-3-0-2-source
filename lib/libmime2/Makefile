#! gmake

# compile-time options:
#
#   MIME_TO_HTML	build in support for converting MIME to HTML
#			(Mozilla needs this, obviously.)
#   MIME_SECURITY	build in support for S/MIME (Mozilla again.)
#   MIME_LEAF_TEXT	build in support for extracting the text from MIME
#			objects (Intertwingle needs this for indexing.)
#
#   It's ok to define all of the above; but if you leave some of them
#   undefined, the amount of object code pulled in will be smaller.
#
#   NOTE!  Changing these changes structure layout, so if you change these
#   options, you need to do a clean build of this library and everything
#   that includes libmime.h.
#
#MIME_TO_HTML=1
#MIME_SECURITY=1
MIME_LEAF_TEXT=1



DEPTH = ../..

BASE_LIB_SRCS =	   \
	mimei.c    \
	mimecont.c \
	mimeenc.c  \
	mimeeobj.c \
	mimehdrs.c \
	mimeleaf.c \
	mimemdig.c \
	mimemmix.c \
	mimemsg.c  \
	mimemult.c \
	mimeobj.c  \
	mimepbuf.c \
	mimesun.c  \
	mimetext.c \
	mimetpla.c \
	mimeunty.c \
	$(NULL)

HTML_LIB_SRCS =	   \
	mimeebod.c \
	mimeiimg.c \
	mimemalt.c \
	mimemapl.c \
	mimemoz.c  \
	mimempar.c \
	mimemrel.c \
	mimetenr.c \
	mimethtm.c \
	mimetric.c \
	$(NULL)

CRYPTO_LIB_SRCS =  \
	mimecryp.c \
	mimempkc.c \
	mimemsig.c \
	mimepkcs.c \
	$(NULL)

FILT_SRCS =	   \
	mimefilt.c \
	mimestub.c \
	$(NULL)

LIB_SRCS = $(BASE_LIB_SRCS)

ifdef MIME_TO_HTML
  LIB_SRCS += $(HTML_LIB_SRCS)
endif

ifdef MIME_SECURITY
  LIB_SRCS += $(CRYPTO_LIB_SRCS)
endif

LIB_OBJS  = $(LIB_SRCS:.c=.o)

LIBRARY = libmime.a
OBJS = $(LIB_OBJS)

include $(DEPTH)/config/rules.mk

FILT_OBJS = $(FILT_SRCS:.c=.o)
FILT_OBJS := $(addprefix $(OBJDIR)/, $(FILT_OBJS))


ifdef MIME_SECURITY
  DEFINES += -DMIME_SECURITY
endif
ifdef MIME_TO_HTML
  DEFINES += -DMIME_TO_HTML
endif
ifdef MIME_LEAF_TEXT
  DEFINES += -DMIME_LEAF_TEXT
endif


# Building the "mimefilt" executable, which reads a message from stdin and
# writes HTML to stdout
#
LIBMSG_DIR     = ../libmsg
LIBXP_DIR      = ../xp
LIBNS_DIR      = ../../nspr/src
LIBXP_BIN_DIR  = $(LIBXP_DIR)/$(OBJDIR_NAME)
LIBMSG_BIN_DIR = $(LIBMSG_DIR)/$(OBJDIR_NAME)
LIBNS_BIN_DIR  = $(LIBNS_DIR)/$(OBJDIR_NAME)

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

BASE_FILT_STUBS =				\
	$(LIBMSG_BIN_DIR)/addr.o		\
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

CRYPTO_FILT_STUBS =				\
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

FILT_STUBS = $(BASE_FILT_STUBS)

ifdef MIME_SECURITY
  FILT_STUBS += $(CRYPTO_FILT_STUBS)
endif

$(OBJDIR)/mimefilt: $(FILT_OBJS) $(LIBRARY) $(FILT_STUBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CC) -o $@ $(CFLAGS) $(FILT_OBJS) $(LIBRARY) $(FILT_STUBS) $(LDFLAGS)

$(OBJDIR)/mimefilt.pure: $(LIBRARY) $(FILT_OBJS) $(FILT_STUBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(PURIFY) $(CC) -o $@ $(NOMD_CFLAGS) \
	  $(FILT_OBJS) $(LIBRARY) $(FILT_STUBS) $(LDFLAGS)

clean::
	-$(RM) $(LIBRARY)
	-$(RM) $(OBJDIR)/mimefilt
	-$(RM) $(OBJDIR)/mimefilt.pure
	-$(RM) $(addprefix $(OBJDIR)/, $(BASE_LIB_SRCS:.c=.o))
	-$(RM) $(addprefix $(OBJDIR)/, $(HTML_LIB_SRCS:.c=.o))
	-$(RM) $(addprefix $(OBJDIR)/, $(CRYPTO_LIB_SRCS:.c=.o))
	-$(RM) $(addprefix $(OBJDIR)/, $(FILT_SRCS:.c=.o))


TAGS:: $(LIB_SRCS) $(FILT_SRCS) $(HEADERS)
	$(ETAGS) $(LIB_SRCS) $(FILT_SRCS) $(HEADERS)


# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: libmime.a
	cd $(DEPTH)/lib/libnet; $(MAKE) libnet.a
	cd $(DEPTH)/lib/libmsg; $(MAKE) libmsg.a
	cd $(DEPTH)/lib/xp; $(MAKE) libxp.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
