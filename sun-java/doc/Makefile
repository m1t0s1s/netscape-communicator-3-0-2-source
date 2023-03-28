#! gmake

DEPTH = ../..

# List of packages that we generate documentation for

DOC_PACKAGES =	       \
	java/lang      \
	java/io	       \
	java/util      \
	java/net       \
	java/awt       \
	java/awt/peer  \
	java/awt/image \
	java/applet

TARGETS = documentation

include $(DEPTH)/sun-java/config/rules.mk

documentation::
	for i in $(DOC_PACKAGES); do		   \
	    $(MAKE) build_package_doc PACKAGE=$$i; \
	done

# Get list of .class files that were built in classsrc
JSRC := $(shell cd ../classsrc; echo $(PACKAGE)/*.java)
DOBJ = $(subst /,.,$(JSRC:.java=.html))

VPATH = ../classsrc

DOC_PACKAGE = $(subst /,.,$(PACKAGE))

build_package_doc: $(OBJDIR)/$(DOC_PACKAGE).html

$(OBJDIR)/$(DOC_PACKAGE).html: $(JSRC)
	@$(MAKE_OBJDIR)
	-$(JAVADOCFF) -d $(OBJDIR) $?
	echo "Generating " $@;				     \
	rm -f $@;					     \
	echo '<ul>' >> $@;				     \
	for i in $(DOBJ); do				     \
		b=`basename $$i`;			     \
		echo '<li> <a href="'$$b'">'$$b'</a>' >> $@; \
	done;						     \
	echo '</ul>' >> $@

install:: documentation
	$(INSTALL) $(OBJDIR)/*.html $(DIST)/doc/api
	$(INSTALL) *.html $(DIST)/doc/cmd

clobber::
	rm -rf $(OBJDIR)
