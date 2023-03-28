#! gmake --no-print-directory

DEPTH	= .

include $(DEPTH)/config/config.mk

DIRS = config include nspr jpeg

ifndef NO_JAVA
DIRS += sun-java
endif

ifndef NO_MOCHA
DIRS += mocha
endif

DIRS += lib l10n cmd

# ifdef HAVE_SECURITY
DIRS += security
# else
# DIRS += security/lib/hash
# endif

# Must export the security headers before building in any other directory.
export::
	cd security; $(MAKE) export_hdrs


include $(DEPTH)/config/rules.mk

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild the libraries, then relink cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: export
	D=$(DEPTH)/cmd/xfe/$(OBJDIR_NAME); \
	if test ! -d $$D; then rm -rf $$D; $(NSINSTALL) -D $$D; fi
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@

# Running this rule assembles all the SDK source pieces into dist/sdk.
# You'll need to run this rule on every platform to get all the 
# binaries (e.g. javah) copied there. You'll also have to do special
# magic on a Mac.
sdk-src::
	$(SDKINSTALL) include/npapi.h			$(SDK)/include/
	$(SDKINSTALL) include/jri_md.h			$(SDK)/include/
	$(SDKINSTALL) include/jritypes.h		$(SDK)/include/
	$(SDKINSTALL) include/jri.h			$(SDK)/include/
	$(SDKINSTALL) lib/plugin/npupp.h		$(SDK)/include/
	$(SDKINSTALL) sdk/common/*.c*			$(SDK)/common/
	$(SDKINSTALL) sun-java/classsrc/java_30.x	$(SDK)/classes/
#	$(SDKINSTALL) sdk/examples/npavi/Source/*.java		$(SDK)/examples/npavi/Source/
#	$(SDKINSTALL) sdk/examples/npavi/Source/*.class		$(SDK)/examples/npavi/Source/
#	$(SDKINSTALL) sdk/examples/npavi/Source/*.cpp		$(SDK)/examples/npavi/Source/
#	$(SDKINSTALL) sdk/examples/npavi/Source/*.h		$(SDK)/examples/npavi/Source/
#	$(SDKINSTALL) sdk/examples/npavi/Source/*.def		$(SDK)/examples/npavi/Source/
#	$(SDKINSTALL) sdk/examples/npavi/Source/*.rc		$(SDK)/examples/npavi/Source/
#	$(SDKINSTALL) sdk/examples/npavi/Source/makefile	$(SDK)/examples/npavi/Windows/
	$(SDKINSTALL) sdk/examples/simple/Source/*.c		$(SDK)/examples/simple/Source/
	$(SDKINSTALL) sdk/examples/simple/Source/*.java		$(SDK)/examples/simple/Source/
	$(SDKINSTALL) sdk/examples/simple/Source/*.class	$(SDK)/examples/simple/Source/
	$(SDKINSTALL) sdk/examples/simple/Source/_gen/*.h	$(SDK)/examples/simple/Source/_gen/
	$(SDKINSTALL) sdk/examples/simple/Source/_stubs/*.c	$(SDK)/examples/simple/Source/_stubs/
	$(SDKINSTALL) sdk/examples/simple/Unix/makefile.*	$(SDK)/examples/simple/Unix/
	$(SDKINSTALL) sdk/examples/simple/Testing/SimpleExample.html	$(SDK)/examples/simple/Testing/
	$(SDKINSTALL) sdk/examples/simple/readme.html		$(SDK)/examples/simple/
	$(SDKINSTALL) sdk/examples/UnixTemplate/Source/*.c	$(SDK)/examples/UnixTemplate/Source/
	$(SDKINSTALL) sdk/examples/UnixTemplate/Testing/Test.html	$(SDK)/examples/UnixTemplate/Testing/
	$(SDKINSTALL) sdk/examples/UnixTemplate/Unix/makefile.*	$(SDK)/examples/UnixTemplate/Unix/
	$(SDKINSTALL) sdk/examples/UnixTemplate/readme.html	$(SDK)/examples/UnixTemplate/

# Run this on every platform
sdk-bin::
	cd sdk; $(MAKE); cd ..
	$(SDKINSTALL) $(DIST)/bin/javah							$(SDK)/bin/$(OS_CONFIG)/
	$(SDKINSTALL) sdk/examples/simple/Source/$(OBJDIR_NAME)/npsimple.so		$(SDK)/bin/$(OS_CONFIG)/
	$(SDKINSTALL) sdk/examples/UnixTemplate/Source/$(OBJDIR_NAME)/nptemplate.so	$(SDK)/bin/$(OS_CONFIG)/

