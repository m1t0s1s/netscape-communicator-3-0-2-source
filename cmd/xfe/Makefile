#! gmake

DEPTH = ../..

include $(DEPTH)/config/config.mk

#
# Makefile for the Mozilla X front end.
#
#   There are N dimensions to a full complement of netscape binaries:
#
#	- target architecture
#	- debug vs optimize
#	- normal vs vendor-cobranded version (number, anim, etc)
#	- normal vs purified
#
#   A directory is needed for each of the first two dimensions (target
#   and debug) but executables for all of the others will then be built
#   in the same directory from the same .o files.  The files have names
#   as follows:
#
#	netscape-export			2.0
#	netscape-nis-export		2.0  (SunOS 4 only)
#	netscape-sgi-export		2.0S (Irix5 only)
#	netscape-export.pure		2.0  (SunOS 4 and Solaris only)
#	 ...etc...
#
#   The only difference in the executables is which version of config-*.o
#   is linked in, which version of libsecurity.a is linked in, and whether
#   -lresolv was used.
#
#   config.c is compiled in N different ways, and .o files are put in the
#   config/ subdirectory.  The same trick is used for the fallback resources
#   (which contain a version number) and for the license data.
#
#   The "release" target will create a release/ subdirectory, and N subdirs
#   of that in which the various tar files will go.  The tar files will be
#   uniquely named, though the executables in them are all called "netscape".
#
#   The exception to this is that the -nis versions don't get their own tar
#   files, they are lumped in with the normal SunOS 4 package with the names
#   "netscape" (for the NIS version) and "netscape_dns" (for the -lresolv
#   version.)
#

HPANE_DIR = $(DEPTH)/cmd/xfe/hpane
MICROLINE_DIR = $(DEPTH)/cmd/xfe/Microline/XmL

DIRS = hpane GNU_movemail Microline/XmL

CSRCS =		     \
	pref_helpers.c \
        colors.c     \
	commands.c   \
	convert.c    \
	dialogs.c    \
	dragdrop.c   \
	e_kit.c	     \
	fonts.c	     \
	forms.c	     \
	history.c    \
	hot.c	     \
	icons.c	     \
	images.c     \
	lay.c	     \
	locale.c     \
	mailnews.c   \
	menu.c	     \
	movemail.c   \
	mozilla.c    \
	new_manage.c \
	outline.c    \
	passwd.c     \
        plugin.c     \
	prefdialogs.c\
	prefs.c	     \
	remote-s.c   \
	scroll.c     \
	scroller.c   \
	strids.c     \
	strings.c    \
	thermo.c     \
	visual.c     \
	xfe.c	     \
	xfe-dns.c    \
	$(NULL)

ifdef BUILD_EDITOR
CSRCS += editor.c editordialogs.c

#    HP requires C++ programs to have main() in a C++ file.
ifeq ($(OS_ARCH),HP-UX)
C++SRCS = cplusplusmain.cc
CFLAGS += -DCPLUSPLUS_LINKAGE
endif
endif

OBJS = $(CSRCS:.c=.o) $(C++SRCS:.cc=.o)

# Only need to change it in one place now.
JAVA_CLASS_FILE	= $(DEPTH)/sun-java/classsrc/java_302

MICROLINE_LIB	= $(DIST)/lib/libXmL.a
HPANE_LIB	= $(DIST)/lib/libHPane.a

#EXPORT_LIB	= $(DIST)/lib/libsec-export.a

ifdef HAVE_SECURITY
EXPORT_LIB	= \
	$(DIST)/lib/libsecnav.a \
	$(DIST)/lib/libsecmod.a \
	$(DIST)/lib/libpkcs12.a \
        $(DIST)/lib/libhash.a \
        $(DIST)/lib/libssl.a \
        $(DIST)/lib/libpkcs7.a \
        $(DIST)/lib/libcert.a \
        $(DIST)/lib/libkey.a \
        $(DIST)/lib/libcrypto.a \
	$(DIST)/lib/libsecutil.a \
	$(DIST)/lib/libjar.a \
	$(NULL)
else
EXPORT_LIB = $(DIST)/lib/libhash.a
endif



NSPR_LIB	= $(DIST)/lib/nspr.a
ifdef DBMALLOC
NSPR_LIB	+= $(DIST)/lib/dbmalloc.a
endif

BASIC_LIBS =			\
	$(MICROLINE_LIB)	\
	$(HPANE_LIB)		\
	$(DIST)/lib/libxlate.a	\
	$(DIST)/lib/libnet.a	\
	$(DIST)/lib/liblay.a	\
	$(DIST)/lib/libi18n.a	\
	$(DIST)/lib/libimg.a	\
	$(DIST)/lib/libjpeg.a	\
	$(DIST)/lib/libmisc.a	\
	$(DIST)/lib/libparse.a	\
	$(DIST)/lib/libmsg.a	\
	$(DIST)/lib/libmime.a	\
	$(DIST)/lib/libplugin.a	\
	$(DIST)/lib/libdbm.a

BASIC_LIBS_2 = $(DIST)/lib/libxp.a

ifndef NO_JAVA
BASIC_LIBS +=			\
	$(DIST)/lib/libjava.a	\
	$(DIST)/lib/libnsn.a	\
	$(DIST)/lib/libnjs.a	\
	$(DIST)/lib/libnsa.a	\
	$(DIST)/lib/libplugin.a	\
	$(DIST)/lib/javai.a	\
	$(DIST)/lib/javart.a	\
	$(DIST)/lib/sunmd.a	\
	$(DIST)/lib/libawt.a	\
	$(DIST)/lib/libmmedia.a
endif

ifndef NO_MOCHA
BASIC_LIBS += $(DIST)/lib/libmocha.a $(DIST)/lib/mocha.a $(DIST)/lib/libmisc.a	\

endif

XFE_PROGNAME = netscape

ALL_TARGETS = $(OBJDIR)/$(XFE_PROGNAME)-export \

TARGETS = $(OBJDIR)/$(XFE_PROGNAME)-export

default:: $(TARGETS)

all:: $(ALL_TARGETS)

include $(DEPTH)/config/rules.mk

DEFINES += -DNEW_DECODERS -UX_PLUGINS

INCLUDES += -I$(DEPTH)/lib/plugin   \
	    -I$(DEPTH)/lib/libparse \
	    -I$(DEPTH)/lib/layout   \
	    -IMicroline

#######################################################################

# Set defaults for all platforms. Each OS_ARCH will override this if
# neccessary

# All platforms except SunOS dont need two versions NIS and DNS.
# SunOS DNS only needs this.
NIS_SRC =
NIS_OBJS =
NIS_LIB =
DNS_LIB =

# All platforms except SunOS dont need separate YP versions
NEED_YP_VERSION = 0

# All platforms except bsdi, linux and SunOS 4.1.3 dont need nls/
NEED_NLS = 0

# All platforms except Solaris 5.3,5.4 dont have MCS
MCS_CMD = true

# hpux, irix52, linux, SunOS 4.1.3, 5.3, 5.4 need this; others don't.
EXTRA_REL_FILES =

#######################################################################

# Plugin related defines

PLUGIN_DSO	= $(DIST)/bin/libnullplugin.so
TEST_PLUGIN	=

ifdef BUILD_UNIX_PLUGINS
TEST_PLUGIN	= $(DIST)/bin/libtextplugin.so
ifneq ($(OS_ARCH),Linux)
EXTRA_REL_FILES	+= $(PLUGIN_DSO)
endif
endif

# These will be done only for release targets

$(PLUGIN_DSO):
	@cd plugins/nullplugin; $(MAKE)

$(TEST_PLUGIN):
	@cd plugins/textplugin; $(MAKE)

#######################################################################

# Adjust build based on OS_ARCH. Try to use static libraries when
# possible because jwz hates shared libraries.

X_OBJS = $(OBJDIR)/config-YYY.o	   \
	 $(OBJDIR)/license-YYY.o   \
	 $(OBJDIR)/resources-YYY.o \
	 $(OBJDIR)/icondata.o

EXPORT_OBJS     = $(subst YYY,export,$(X_OBJS))
NIS_EXPORT_OBJS = $(subst YYY,nis-export,$(X_OBJS))
SGI_EXPORT_OBJS = $(subst YYY,sgi-export,$(X_OBJS))

# If the version number changes, all these need to be rebuilt.
# (config-* doesn't need to be in here, since that already has a rule.)
VERSION_OBJS = $(OBJDIR)/license-export.o \
               $(OBJDIR)/license-nis-export.o $(OBJDIR)/license-sgi-export.o \
               $(OBJDIR)/resources-export.o \
               $(OBJDIR)/resources-nis-export.o \
               $(OBJDIR)/resources-sgi-export.o
$(VERSION_OBJS):: versionn.h

########################################
# SGI Machines
ifeq ($(OS_ARCH),IRIX)

ALL_TARGETS +=				 \
	$(OBJDIR)/$(XFE_PROGNAME)-sgi-export

ifndef NO_JAVA
OTHER_LIBS	= -lSgm -lXm -lXmu -lXt -lX11 -lgen -laudio -lm $(OS_LIBS)
else
OTHER_LIBS	= -lSgm -lXm -lXmu -lXt -lX11 -lgen -lm $(OS_LIBS)
endif
EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                  $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

#
# On Irix, tag the executable for use by the Indigo Magic Desktop.
# This magic number comes from /usr/lib/filetype/install/netscape.ftr
# shipped by SGI along with their n.nnS version of Mozilla.
#
EXTRA_POST_LINK_CMD = /usr/sbin/tag 67150

## For release only
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

########################################
# Sun Machines
ifeq ($(OS_ARCH),SunOS)

EXTRA_POST_LINK_CMD = echo

ifeq ($(OS_RELEASE),4.1.3_U1)
# Old SunOS

ALL_TARGETS +=					\
	$(OBJDIR)/$(XFE_PROGNAME)-nis-export

OTHER_LIBS = -L/home/motif/usr/lib		\
		/home/motif/usr/lib/libXm.a	\
		/home/motif/usr/lib/libXmu.a	\
		/home/motif/usr/lib/libXt.a	\
		/home/motif/usr/lib/libXext.a	\
		/home/motif/usr/lib/libX11.a

ifdef BUILD_EDITOR
OTHER_LIBS	+= -L/tools/ns/lib

#
#    Need this guy because the one in libg++.a is broken.
#
BASIC_LIBS	+= $(OBJDIR)/regex.o
$(OBJDIR)/regex.o:	/lib/libc.a
	ar x /lib/libc.a regex.o && mv regex.o $@
endif

EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                  $(OTHER_LIBS) $(NSPR_LIB) -lm

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

# SunOS had 2 executables. Only the non-nis version need this
NIS_OBJS	= $(OBJDIR)/dns-stub.o
NIS_SRCS	= dns-stub.c
NIS_LIB		= $(NIS_OBJS)
DNS_LIB		= -lresolv

# For release only.
# This is only for SunOS as it has both nis and non-nis
# version of the browser packaged together.
EXTRA_EXPORT_OBJS = $(XFE_PROGNAME)-nis-export

NEED_YP_VERSION=1
NEED_NLS = 1
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

ifeq ($(OS_RELEASE),5.3)
# Solaris 5.3
USRLIBDIR = /usr/openwin/lib
#
# NOTE: libXt.a should be a symlink to a copy of the SunOS 5.4 libXt.a.
#       The 5.3 version seems to make us dump core on startup.
#
OTHER_LIBS = -L$(USRLIBDIR) $(MOTIFLIB)	\
	$(USRLIBDIR)/libXt.a		\
	$(USRLIBDIR)/libXmu.a		\
	$(USRLIBDIR)/libXext.a		\
	-lX11				\
	$(OS_LIBS) -lgen -lresolv -lm

#
# NOTE: The -Xlinker flag is GCC-specific.
#
EXPORT_LDFLAGS	=  -z defs -L$(USRLIBDIR) -Xlinker -R$(USRLIBDIR) \
                   $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                   $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

# For release only
MCS_CMD = mcs -d
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

ifeq ($(OS_RELEASE),5.4)
# Solaris 5.4
USRLIBDIR = /usr/openwin/lib
OTHER_LIBS = $(MOTIFLIB) -lXt -lXmu -lXext -lX11			\
	$(OS_LIBS) -lgen -lresolv -lm

EXPORT_LDFLAGS	= -z defs -L/usr/dt/lib -L$(USRLIBDIR)			\
		  -R/usr/dt/lib -R$(USRLIBDIR)				\
		  $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2)		\
		  $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

# For release only
MCS_CMD = mcs -d
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

ifeq ($(OS_RELEASE),5.4_i86pc)
# Solaris 5.4 on x86 machines

USRLIBDIR	= /usr/openwin/lib
OTHER_LIBS	= $(MOTIFLIB) -lXt -lXmu -lXext -lX11 $(OS_LIBS) -lm

EXPORT_LDFLAGS	= -z defs -L$(MOTIF)/lib -L$(USRLIBDIR) -R$(MOTIF)/lib -R$(USRLIBDIR) \
		  $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

# For release only
MCS_CMD		= mcs -d
ifndef NO_JAVA
EXTRA_REL_FILES	+= $(JAVA_CLASS_FILE)
endif

endif

ifeq ($(OS_RELEASE),5.5)
# Solaris 5.5
USRLIBDIR = /usr/openwin/lib
OTHER_LIBS = $(MOTIFLIB) -lXt -lXmu -lXext -lX11			\
	$(OS_LIBS) -lgen -lresolv -lm

EXPORT_LDFLAGS	= -z defs -L/usr/dt/lib -L$(USRLIBDIR)			\
		  -R/usr/dt/lib -R$(USRLIBDIR)				\
		  $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2)		\
		  $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

# For release only
MCS_CMD = mcs -d
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

endif

########################################
# Dec Machines
ifeq ($(OS_ARCH),OSF1)

# We would like to link OSF1 static.
# This is because of motif problems (BadMatch errors on non-default visual)
# on 3.2 and 3.0 (and not on 2.0).
# But libX11 needs to be dynamic, otherwise the locale stuff doesn't work
# and you get warnings and core dump when pasting into Mozilla.
# Also, libXm must be dynamic, otherwise Japanese text widgets hang
# the process.
# What a delightful state of affairs. -- erik

USRLIBDIR = /usr/lib
OTHER_LIBS	= -lXm -lXmu \
		  -lXt -lX11 \
		  -ldnet_stub -lm $(OS_LIBS)
EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                   $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

EXTRA_POST_LINK_CMD = echo

# For release only
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

########################################
# HP Machines
ifeq ($(OS_ARCH),HP-UX)

OTHER_LIBS	= -L/usr/lib/X11R5 -L/usr/lib/Motif1.2 \
			-lXm -lXmu -lXt -lX11 -lm $(OS_LIBS)
EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                   $(NSPR_LIB) $(OTHER_LIBS)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

EXTRA_POST_LINK_CMD = echo

# For release only
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

########################################
# IBM Machines

ifeq ($(OS_ARCH),AIX)

OTHER_LIBS =	-L/usr/local/aix32/lib -lIM -lXm -lXt -lXmu -lXext -lX11\
		-liconv -lm $(OS_LIBS)

# Link with nspr twice: the first time to force in it's malloc; the second
# time to get anything else referenced by later libraries
EXPORT_LDFLAGS	= $(NSPR_LIB) $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                  $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

EXTRA_POST_LINK_CMD = echo

# For release only
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

########################################
# SCO Open Server

ifeq ($(OS_ARCH),SCO_SV)

ifdef NEED_XMOS
LIB_XMOS	= $(OBJDIR)/Xmos.o
endif

OTHER_LIBS	= $(LIB_XMOS) -lXm -lXt -lXmu -lXext -lX11 -lm -lPW

EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
		  $(OTHER_LIBS) $(NSPR_LIB) $(OS_LIBS)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

EXTRA_POST_LINK_CMD = echo

# For release only.
ifndef NO_JAVA
EXTRA_REL_FILES	+= $(JAVA_CLASS_FILE)
endif

endif

########################################
# SCO Unixware 2.1

ifeq ($(OS_ARCH),UNIX_SV)

ifdef NEED_XMOS
LIB_XMOS	= $(OBJDIR)/Xmos.o
endif

OTHER_LIBS	= $(LIB_XMOS) -L /usr/X/lib -lXm -lXt -lXmu -lXext -lX11 -lm

EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
		  $(OTHER_LIBS) $(NSPR_LIB) -lresolv -lsocket -lc /usr/ucblib/libucb.a

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB) 

EXTRA_POST_LINK_CMD = echo

# For release only.
ifndef NO_JAVA
EXTRA_REL_FILES	+= $(JAVA_CLASS_FILE)
endif

endif

########################################
# BSDI
ifeq ($(OS_ARCH),BSD_386)
OTHER_LIBS =	-L/usr/X386/lib				 \
		/h/neon/f/ebina/motifbsdI/lib/Xm/libXm.a \
		/usr/X386/lib/libXt.a			 \
		/usr/X386/lib/libXmu.a			 \
		/usr/X386/lib/libXext.a			 \
		/usr/X386/lib/libX11.a			 \
		-lm $(OS_LIBS)
EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                   $(OTHER_LIBS) $(NSPR_LIB)

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB)

EXTRA_POST_LINK_CMD = echo

# For release only.
NEED_NLS = 1
ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

########################################
# Linux

# to build non-ELF on 1.2.13, compile/link with "-b i486-linuxaout"
# and link with "-b i486-linuxaout -static -L/usr/local/lib/old-aout-libs"

ifeq ($(OS_ARCH),Linux)

EXTRA_POST_LINK_CMD = echo

# jwz: fuck this
#ifeq ($(OS_OBJTYPE),ELF)
# Actually, maybe this should be "if you want to link dynamically against R6"
         XTOOLLIB = -L /usr/X11R6/lib -lXt -lSM -lICE
             XLIB = -lXext -lX11
           XMULIB = -lXmu -lXpm
#        MOTIFLIB = -rdynamic /usr/X11R6/lib/libXm.a
#        MOTIFLIB = /usr/X11R6/lib/libXm.a
#	 MOTIFLIB = -L/usr/local/motif-2.1/lib -lXm -lXp 
	 MOTIFLIB = -L/usr/local/motif-1.2.4/lib -lXm
#else
#        USRLIBDIR = /usr/XF86-2.1.1/lib
#         MOTIFLIB = /usr/local/lib/libXm.a
#         XTOOLLIB = $(USRLIBDIR)/libXt.a
#           XMULIB = $(USRLIBDIR)/libXmu.a
#     EXTENSIONLIB = $(USRLIBDIR)/libXext.a
#             XLIB = $(USRLIBDIR)/libX11.a
#endif

       OTHER_LIBS = $(MOTIFLIB) $(XTOOLLIB) $(XMULIB) \
                    $(EXTENSIONLIB) $(XLIB) /usr/lib/libm.a

ifdef BUILD_UNIX_PLUGINS
       OTHER_LIBS += -ldl
endif

EXPORT_LDFLAGS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) \
                   $(OTHER_LIBS) $(NSPR_LIB) -lc

EXPORT_DEPLIBS	= $(BASIC_LIBS) $(EXPORT_LIB) $(BASIC_LIBS_2) $(NSPR_LIB) -lc

# For release only.
ifeq ($(OS_OBJTYPE),ELF)
NEED_NLS = 0
else
NEED_NLS = 1
endif

ifndef NO_JAVA
EXTRA_REL_FILES += $(JAVA_CLASS_FILE)
endif

endif

########################################

$(MICROLINE_LIB):
	cd Microline/XmL; $(MAKE)

$(HPANE_LIB):
	cd hpane; $(MAKE)

$(JAVA_CLASS_FILE):
	@cd $(@D); ln -s $(@F).x $(@F)

#######################################################################

# Rules to build mkicons program and then icondata.c

MKICONS_EXE = $(OBJDIR)/mkicons
# hack: don't rebuild mkicons just because libxp changed.
MKICONS_DEPLIB = $(DIST)/lib/libimg.a $(DIST)/lib/libjpeg.a
MKICONS_LIB = $(MKICONS_DEPLIB) $(DIST)/lib/libxp.a

$(MKICONS_EXE): mkicons.c $(MKICONS_DEPLIB)
	$(CCF) -o $@ $(LDOPTIONS) -I$(DEPTH)/lib/libimg $< $(MKICONS_LIB)

# Disgusting order of icons below is because, for some reason, mkicons dies
# if the biff icons are first.  Something about their transparancy colors
# upsets it.  Oh, well...

ICONS	= $(wildcard icons/[AC-Z]*.gif icons/B*.gif)

ifdef BUILD_GOLD
ICONS += icons/gold_app.gif
else
ICONS += icons/app.gif
endif

ifdef BUILD_EDITOR
EDITOR_ICONS	= $(wildcard icons/ed*.gif)
endif

ANIM	= $(wildcard icons/anim/meteors/*.gif \
		     icons/anim/mozilla/*.gif \
		     icons/anim/compass/*.gif \
		     icons/anim/sgi/*.gif)

$(OBJDIR)/icondata.c: $(MKICONS_EXE) $(ICONS) $(EDITOR_ICONS) $(ANIM)
	@echo Generating $@... ; \
	$(MKICONS_EXE) $(ICONS) $(EDITOR_ICONS) $(ANIM) > $@.$$$$ && mv $@.$$$$ $@

$(OBJDIR)/icondata.o: $(OBJDIR)/icondata.c
	$(CC) -c -I. -o $@ $(OBJDIR)/icondata.c

#######################################################################

# XXX does gmake have macros?

# Rules to build config*.o files

GUESS_CONFIG := $(shell $(DEPTH)/config/config.guess | sed 's/i[2345]86/x86/')

$(OBJDIR)/config-export.o: config.c versionn.h Makefile $(OBJS) $(EXPORT_DEPLIBS)
	@echo Generating $@ from config.c...;				      \
	X=' ';								      \
	VN=`sed -n$$X 's/^#define VERSION_NUMBER[ 	]*\(.*\)$$/\1/p' versionn.h` ; \
        $(CC) -c $(CFLAGS) -o $@ config.c				      \
		-DCONFIG="$(GUESS_CONFIG)"				      \
		-DDATE="`date +%d-%h-%y`"				      \
		-DVERSION=$${VN}					      \
		-UHAVE_NIS -UVENDOR_ANIM

$(OBJDIR)/config-nis-export.o: config.c versionn.h Makefile $(OBJS) $(NIS_OBJS) $(EXPORT_DEPLIBS)
	@echo Generating $@ from config.c...;				      \
	X=' ';								      \
	VN=`sed -n$$X 's/^#define VERSION_NUMBER[ 	]*\(.*\)$$/\1/p' versionn.h` ; \
        $(CC) -c $(CFLAGS) -o $@ config.c				      \
		-DCONFIG="$(GUESS_CONFIG)"				      \
		-DDATE="`date +%d-%h-%y`"				      \
		-DVERSION=$${VN}					      \
		-DHAVE_NIS -UVENDOR_ANIM

$(OBJDIR)/config-sgi-export.o: config.c versionn.h Makefile $(OBJS) $(EXPORT_DEPLIBS)
	@echo Generating $@ from config.c...;				      \
	X=' ';								      \
	VN=`sed -n$$X 's/^#define VERSION_NUMBER[ 	]*\(.*\)$$/\1/p' versionn.h` ; \
        $(CC) -c $(CFLAGS) -o $@ config.c				      \
		-DCONFIG="$(GUESS_CONFIG)"				      \
		-DDATE="`date +%d-%h-%y`"				      \
		-DVERSION=$${VN}S					      \
		-UHAVE_NIS -DVENDOR_ANIM

#######################################################################

# Rules to build license.o files

LICENSE_DIR = $(DEPTH)/l10n/us/xp
LICENSES = $(LICENSE_DIR)/LICENSE-alpha	      \
	   $(LICENSE_DIR)/LICENSE-export-beta \
	   $(LICENSE_DIR)/LICENSE-export      \
	   $(LICENSE_DIR)/LICENSE-export-sgi

# DO NOT CHANGE THE ORDER OF THE DEPENDENCIES.  Add new ones to the end.
license-%.o: LICENSE-%.c Makefile
	$(CC) -c -o $@ $<

$(OBJDIR)/LICENSE-export.c: Makefile $(LICENSES) make-license
	@./make-license $@ $(LICENSE_DIR) "" export

$(OBJDIR)/LICENSE-nis-export.c: Makefile $(LICENSES) make-license
	@./make-license $@ $(LICENSE_DIR) "" export

$(OBJDIR)/LICENSE-sgi-export.c: Makefile $(LICENSES) make-license
	@./make-license $@ $(LICENSE_DIR) sgi export

$(OBJDIR)/LICENSE-export: $(OBJDIR)/license-export.o make-license
	@./make-license $@ $(LICENSE_DIR) "" export

$(OBJDIR)/LICENSE-sgi-export: $(OBJDIR)/license-sgi-export.o make-license
	@./make-license $@ $(LICENSE_DIR) "" export

#######################################################################

# Rules to build README files ( release specific )

ifdef BUILD_GOLD
    GOLD_SFX = Gold
endif

$(OBJDIR)/README-export: README.tmpl versionn.h make-readme
	@./make-readme $@ Netscape Netscape "$(GOLD_SFX)" export $(XFE_PROGNAME)

$(OBJDIR)/README-sgi-export: README.tmpl versionn.h make-readme
	@./make-readme $@ Netscape Netscape "S$(GOLD_SFX)" export $(XFE_PROGNAME)

#######################################################################

# Rules to build resources

LOCALES = $(LOCALE_MAP) $(MAIL_IM_HACK) $(NEWS_IM_HACK)

.SUFFIXES: .ad

# DO NOT CHANGE THE ORDER OF THE DEPENDENCIES.  Add new ones to the end.
resources-%.o: RESOURCES-%.c Makefile
	$(CC) -c -o $@ $<

RESOURCES-%.c: Netscape-%.ad ad2c Makefile
	@echo 'char *fe_fallbackResources[] = {' > $@; \
	./ad2c $< >> $@; \
	echo '0};' >> $@

# Explicit dependency list to ensure that e_kit_resources.h gets built.
$(OBJDIR)/e_kit.o:	e_kit.c e_kit_resources.h

# Turn ekit app-defaults into a C file
e_kit_resources.h: e_kit.ad
	@echo 'char* fe_ekitDefaultDatabase = ' > $@
	cat $< | tr '[\001-\272]' '[\106-\377]' |    \
        od -b | sed 's/^[0-7][0-7]* *\(.*\)/\\\1/;      \
                s/ /\\/g;s/\(.*\)/ "\1"/;s/^ *"\\"$$//' >> $@
	@echo ';' >> $@

$(OBJDIR)/Netscape-nis-export.ad $(OBJDIR)/Netscape-export.ad: Makefile resources versionn.h strs make-resources $(LOCALE_MAP)
	@./make-resources $@ Netscape Netscape "" export $(LOCALES)

$(OBJDIR)/Netscape-sgi-export.ad: Makefile resources versionn.h strs make-resources $(LOCALE_MAP)
	@./make-resources $@ Netscape Netscape "S" export $(LOCALES)

#######################################################################

CCLD = $(CC)

ifdef BUILD_EDITOR

CCLD = $(CCC)

ifeq ($(OS_ARCH),OSF1)
#
#    OSF's cxx link command will link in _dynamic_ version of two C++
#    libraries. This is bad, they are not shipped on a base system.
#    So , we call this special hacked driver script to do the job.
#
CCLD = ./cxxlink-driver -cxx_platform OSF1 -cxx_command cxx

#    Hack to add a dependency for executable.
EXPORT_DEPLIBS += cxxlink-driver

#    This will also work, but will probably break easily as more C++
#    *features* get used (like static constructors - gasp!)
#CCLD = cc
#OTHER_LIBS += /usr/lib/cmplrs/cxx/libcxx.a /usr/lib/cmplrs/cxx/libexc.a
endif

ifeq ($(OS_ARCH),HP-UX)
EXPORT_DEPLIBS += cxxlink-filter
CCLD = CC -tl,./cxxlink-filter
endif

ifeq ($(OS_ARCH),IRIX)
EXPORT_DEPLIBS += cxxlink-filter
CCLD = CC -tl,./cxxlink-filter
endif

endif

# The basic navigator

ifeq ($(OS_ARCH),AIX)

$(OBJDIR)/netscape-export: $(OBJS) $(EXPORT_OBJS) $(EXPORT_DEPLIBS)
	@$(MAKE_OBJDIR)
	rm -f $@ $(AIX_TMP)
	$(CCLD) -o $(AIX_TMP) $(AIX_LINK_OPTS) $(OBJS) $(EXPORT_OBJS) $(EXPORT_LDFLAGS) $(DNS_LIB)
	$(CCLD) -o $@ $(AIX_TMP) $(AIX_WRAP) $(AIX_FINAL_LDFLAGS)
	@$(EXTRA_POST_LINK_CMD) $@
	rm -f $(AIX_TMP)

else

#
# Build rules for non-AIX systems
#

$(OBJDIR)/netscape-export: $(OBJS) $(EXPORT_OBJS) $(EXPORT_DEPLIBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CCLD) -o $@ $(CFLAGS) $(OBJS) $(EXPORT_OBJS) $(EXPORT_LDFLAGS) $(DNS_LIB)
	@$(EXTRA_POST_LINK_CMD) $@

$(OBJDIR)/netscape-export.pure: $(OBJS) $(EXPORT_OBJS) $(EXPORT_DEPLIBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(PURIFY) $(CCLD) -o $@ $(NOMD_CFLAGS) $(OBJS) $(EXPORT_OBJS) $(EXPORT_LDFLAGS) $(DNS_LIB)
	@$(EXTRA_POST_LINK_CMD) $@

$(OBJDIR)/netscape-export.prof: $(OBJS) $(EXPORT_OBJS) $(EXPORT_DEPLIBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CCLD) -p -o $@ $(CFLAGS) $(OBJS) $(EXPORT_OBJS) $(EXPORT_LDFLAGS) $(DNS_LIB)
	@$(EXTRA_POST_LINK_CMD) $@

# NIS versions of the above

$(OBJDIR)/netscape-nis-export: $(OBJS) $(NIS_EXPORT_OBJS) $(NIS_OBJS) $(EXPORT_DEPLIBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CCLD) -o $@ $(CFLAGS) $(OBJS) $(NIS_EXPORT_OBJS) $(EXPORT_LDFLAGS) $(NIS_LIB)
	@$(EXTRA_POST_LINK_CMD) $@

# SGI versions of the navigator (no nis, no net)

$(OBJDIR)/netscape-sgi-export: $(OBJS) $(SGI_EXPORT_OBJS) $(EXPORT_DEPLIBS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(CCLD) -o $@ $(CFLAGS) $(OBJS) $(SGI_EXPORT_OBJS) $(EXPORT_LDFLAGS) $(DNS_LIB)
	@$(EXTRA_POST_LINK_CMD) $@

endif

install:: $(TARGETS)
	$(INSTALL) $(TARGETS) $(DIST)/bin

#######################################################################

# Release target

README_FILE	:= README.tmpl

ifdef BUILD_INTL
	README_FILE	:= $(DEPTH)/l10n/us/xfe/README.intl
endif

COMMON_REL_DEPS	= versionn.h make-release $(EXTRA_REL_FILES) $(TEST_PLUGIN) $(README_FILE)

RELEASES	= release-export \
		  release-sgi-export

release:: $(RELEASES)

release-export: $(XFE_PROGNAME)-export $(OBJDIR)/README-export $(OBJDIR)/LICENSE-export $(EXTRA_EXPORT_OBJS) $(COMMON_REL_DEPS)
	./make-release $(OBJDIR) $(XFE_PROGNAME) Netscape "" export $(GUESS_CONFIG) $(NEED_YP_VERSION) "$(MCS_CMD)" $(NEED_NLS) "$(EXTRA_REL_FILES)" $(README_FILE)

release-sgi-export: $(XFE_PROGNAME)-sgi-export $(OBJDIR)/README-sgi-export $(OBJDIR)/LICENSE-sgi-export $(COMMON_REL_DEPS)
	./make-release $(OBJDIR) $(XFE_PROGNAME) Netscape "S" sgi-export $(GUESS_CONFIG) $(NEED_YP_VERSION) "$(MCS_CMD)" $(NEED_NLS) "$(EXTRA_REL_FILES)" $(README_FILE)
