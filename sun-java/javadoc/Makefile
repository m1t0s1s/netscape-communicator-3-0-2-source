#! gmake

DEPTH = ../..

TARGETS = javadoc

include $(DEPTH)/sun-java/config/rules.mk

export:: $(TARGETS)
	$(INSTALL) javadoc $(DIST)/bin

install:: export
	$(INSTALL) index.html $(DIST)/doc/api
	$(INSTALL) *.gif $(DIST)/doc/api/images
