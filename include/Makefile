#! gmake --no-print-directory

DEPTH	= ..

HEADERS	= *.h

include $(DEPTH)/config/rules.mk

export::
	$(INSTALL) -m 444 $(HEADERS) $(DIST)/include

install:: export
