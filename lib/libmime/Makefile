#! gmake

DEPTH = ../..

LIB_SRCS =	   \
	mimei.c    \
	mimecont.c \
	mimeenc.c  \
	mimeeobj.c \
	mimeebod.c \
	mimehdrs.c \
	mimeiimg.c \
	mimeleaf.c \
	mimemalt.c \
	mimemapl.c \
	mimemdig.c \
	mimemmix.c \
	mimemoz.c  \
	mimempar.c \
	mimemrel.c \
	mimemsg.c  \
	mimemult.c \
	mimeobj.c  \
	mimepbuf.c \
	mimesun.c  \
	mimetenr.c \
	mimetext.c \
	mimethtm.c \
	mimetpla.c \
	mimetric.c \
	mimeunty.c \
	$(NULL)

#	mimethpl.c \

# ifdef HAVE_SECURITY
LIB_SRCS += mimempkc.c mimepkcs.c mimecryp.c mimemsig.c
# endif


FILT_SRCS =	   \
	mimefilt.c \
	mimestub.c \
	$(NULL)

LIB_OBJS  = $(LIB_SRCS:.c=.o)

LIBRARY = libmime.a
OBJS = $(LIB_OBJS)

include $(DEPTH)/config/rules.mk

FILT_OBJS = $(FILT_SRCS:.c=.o)
FILT_OBJS := $(addprefix $(OBJDIR)/, $(FILT_OBJS))


# Building the "mimefilt" executable, which reads a message from stdin and
# writes HTML to stdout
#
LIBMSG_DIR     = ../libmsg
LIBXP_DIR      = ../xp
LIBXP_BIN_DIR  = $(LIBXP_DIR)/$(OBJDIR_NAME)
LIBMSG_BIN_DIR = $(LIBMSG_DIR)/$(OBJDIR_NAME)

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

FILT_STUBS =					\
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



$(OBJDIR)/mimefilt: $(FILT_OBJS) $(LIBRARY) $(FILT_STUBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CC) -o $@ $(CFLAGS) $(FILT_OBJS) $(LIBRARY) $(FILT_STUBS) $(LDFLAGS)

$(OBJDIR)/mimefilt.pure: $(LIBRARY) $(FILT_OBJS) $(FILT_STUBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(PURIFY) $(CC) -o $@ $(NOMD_CFLAGS) \
	  $(FILT_OBJS) $(LIBRARY) $(FILT_STUBS) $(LDFLAGS)


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
