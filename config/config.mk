#! gmake

# define an include-at-most-once flag
NS_CONFIG_MK	= 1

ifdef BUILD_OPT
OPTIMIZER	= -O
JAVA_OPTIMIZER	= -O
DEFINES		= -UDEBUG -DNDEBUG
OBJDIR_TAG	= _OPT
else
OPTIMIZER	= -g
JAVA_OPTIMIZER	= -g
DEFINES		= -DDEBUG -UNDEBUG -DDEBUG_$(shell whoami)
OBJDIR_TAG	= _DBG
endif

######################################################################

LIBNSPR		= $(DIST)/lib/nspr.a
LIBAWT		= $(DIST)/lib/libawt.a
LIBMMEDIA	= $(DIST)/lib/libmmedia.a
PURELIBNSPR	= $(DIST)/lib/purenspr.a

INCLUDES	= -I$(DIST)/include -I$(DEPTH)/include -I$(DIST)/include/nspr
CFLAGS		= -DXP_UNIX $(OPTIMIZER) $(OS_CFLAGS) $(DEFINES) $(INCLUDES) \
		  $(XCFLAGS)
# For purify
NOMD_CFLAGS	= -DXP_UNIX $(OPTIMIZER) $(NOMD_OS_CFLAGS) $(DEFINES) \
		  $(INCLUDES) $(XCFLAGS)
AS		= $(CC)
ASFLAGS		= $(CFLAGS)

CCF		= $(CC) $(CFLAGS)

PURIFY		= purify $(PURIFYOPTIONS)

# Load os+release dependent makefile rules
OS_ARCH		:= $(subst /,_,$(shell uname -s))

# Force the IRIX64 machines to use IRIX.
ifeq ($(OS_ARCH),IRIX64)
OS_ARCH		:= IRIX
endif

# Attempt to differentiate between SunOS 5.4 and x86 5.4
OS_TEST		:= $(shell uname -m)
ifeq ($(OS_TEST),i86pc)
OS_RELEASE	:= $(shell uname -r)_$(OS_TEST)
else
OS_RELEASE	:= $(shell uname -r)
endif

# jwz: only use first two version numbers on linux
ifeq ($(OS_ARCH),Linux)
OS_RELEASE := $(shell echo "$(OS_RELEASE)" | sed 's/^\([0-9]*\.[0-9]*\)\..*/\1/')
endif


include $(DEPTH)/config/$(OS_ARCH)$(OS_RELEASE).mk

OS_CONFIG	:= $(OS_ARCH)$(OS_OBJTYPE)$(OS_RELEASE)

# Name of the binary code directories
OBJDIR_NAME	= $(OS_CONFIG)$(OBJDIR_TAG).OBJ

# Relative pathname from top-of-tree to current source directory
REVDEPTH	:= $(DEPTH)/config/revdepth
SRCDIR		:= $(shell perl $(REVDEPTH).pl $(DEPTH))

# Figure out where the binary code lives. It either lives in the src
# tree (NSBUILDROOT is undefined) or somewhere else.
ifdef NSBUILDROOT
BUILD		= $(NSBUILDROOT)/$(OBJDIR_NAME)/build
OBJDIR		= $(BUILD)/$(SRCDIR)
DIST		= $(NSBUILDROOT)/$(OBJDIR_NAME)/dist
else
BUILD		= $(OBJDIR_NAME)
OBJDIR		= $(OBJDIR_NAME)
DIST		= $(DEPTH)/dist/$(OBJDIR_NAME)
endif

VPATH		= $(OBJDIR)
DEPENDENCIES	= $(OBJDIR)/.md

include $(DEPTH)/config/$(OS_CONFIG).mk

# Personal makefile customizations go in these optional make include files.
MY_CONFIG	= $(DEPTH)/config/myconfig.mk
MY_RULES	= $(DEPTH)/config/myrules.mk

-include $(MY_CONFIG)

######################################################################

# Specify that we are building a client.
# This will instruct the cross platform libraries to
# include all the client specific cruft.
DEFINES += -DMOZILLA_CLIENT

# Now test variables that might have been set or overridden by $(MY_CONFIG).

# if ((BUILD_EDITOR || BUILD_EDT) && !NO_EDITOR) -> -DEDITOR is defined
ifndef NO_EDITOR
ifdef BUILD_EDITOR
OBJDIR_TAG	:= $(OBJDIR_TAG)_EDT
DEFINES		+= -DEDITOR -DGOLD
# This is the product classification not the feature classification.
# It effects things like where are the release notes, etc..
BUILD_GOLD = yea
else
# We ought to get rid of this now that BUILD_EDITOR has replaced it.
ifdef BUILD_EDT
OBJDIR_TAG	:= $(OBJDIR_TAG)_EDT
DEFINES		+= -DEDITOR -DGOLD
BUILD_EDITOR	= yea
endif 
endif 
endif

# if (BUILD_EDITOR_UI && !NO_EDITOR_UI) -> -DEDITOR_UI is defined
ifdef BUILD_EDITOR_UI
ifndef NO_EDITOR_UI
DEFINES		+= -DEDITOR_UI
endif
endif

ifdef BUILD_DEBUG_GC
DEFINES		+= -DDEBUG_GC
endif

ifdef BUILD_UNIX_PLUGINS
# UNIX_EMBED Should not be needed. For now these two defines go
# together until I talk with jg.  --dp
DEFINES         += -DUNIX_EMBED -DX_PLUGINS
endif

#
# Platform dependent switching off of NSPR, JAVA and MOCHA
#
ifndef NO_NSPR
DEFINES		+= -DNSPR
endif

ifndef NO_JAVA
DEFINES		+= -DJAVA
endif

ifndef NO_MOCHA
DEFINES		+= -DMOCHA
endif

DEFINES += -DAKBAR

######################################################################

GARBAGE		= $(DEPENDENCIES) core

NFSPWD		= $(DEPTH)/config/nfspwd
NSINSTALL	= $(DEPTH)/config/$(OBJDIR_NAME)/nsinstall

ifeq ($(NSDISTMODE),copy)
# copy files, but preserve source mtime
INSTALL = $(NSINSTALL) -t
else
ifeq ($(NSDISTMODE),absolute_symlink)
# install using absolute symbolic links
INSTALL = $(NSINSTALL) -L `$(NFSPWD)`
else
# install using relative symbolic links
INSTALL = $(NSINSTALL) -R
endif
endif

ifndef PLATFORM_HOSTS
PLATFORM_HOSTS	=		\
		  atm		\
		  bsdi		\
		  diva		\
		  gunwale	\
		  openwound	\
		  server2	\
		  server3	\
		  server9	\
		  zot		\
		  $(NULL)
endif

######################################################################

# always copy files for the sdk
SDKINSTALL	= $(NSINSTALL) -t

ifndef SDK
SDK		= $(DEPTH)/dist/sdk
endif

######################################################################

# Rules to build java .class files from .java source files

JAVAI = java
JAVAH = javah
JAVAC = javac

#
# There are two classpath's to worry about: the classpath used by
# the runtime to execute the compiler (RT_CLASSPATH), and the classpath
# used by the compiler itself for dependency checking (JAVAC_CLASSPATH).
#
# If the makefile user sets the CLASSPATH environment variable or
# make variable, then use that path for both.
#

# Let user over-ride CLASSPATH from environment
ifdef CLASSPATH
RT_CLASSPATH = $(CLASSPATH)
JAVAC_CLASSPATH = .:$(CLASSPATH)
else
RT_CLASSPATH = $(DEPTH)/sun-java/classsrc
JAVAC_CLASSPATH = .:$(DEPTH)/sun-java/classsrc
endif

JAVAI_FLAGS = $(JAVAC_CAR) -classpath $(RT_CLASSPATH) -ms8m

JCC = $(JAVAC)
JCFLAGS = -classpath $(JAVAC_CLASSPATH) $(JAVA_OPTIMIZER)
JCCF = $(JCC) $(JCFLAGS)

# Rules to build java .html files from java source files

JAVADOC = $(JAVAI) $(JAVAI_FLAGS) sun.tools.javadoc.Main
JAVADOC_FLAGS = -classpath $(JAVAC_CLASSPATH)
JAVADOCFF = $(JAVADOC) $(JAVADOC_FLAGS)
