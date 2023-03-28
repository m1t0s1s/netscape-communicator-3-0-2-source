#! gmake

DEPTH = ../..

PACKAGES =		  \
	sun/tools/asm	  \
	sun/tools/java	  \
	sun/tools/javac	  \
	sun/tools/javadoc \
	sun/tools/tree	  \
	sun/tools/zip	  \
	sun/tools/debug	  \
	sun/tools/ttydebug

TARGETS = targets

include $(DEPTH)/sun-java/config/rules.mk

targets: javac
	cd ../classsrc; $(MAKE) PACKAGES="$(PACKAGES)"
	cd ../classsrc; $(MAKE) ZIPS=../javac/javac.zip

export:: targets
	$(INSTALL) javac $(DIST)/bin
	$(INSTALL) javac.zip $(DIST)/lib

install:: export
