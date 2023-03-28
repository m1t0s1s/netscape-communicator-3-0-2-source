#! gmake

DEPTH = ../../..

HEADERS = *.h

include $(DEPTH)/nspr/config/rules.mk

export:: $(HEADERS)
	$(INSTALL) -m 444 $(HEADERS) $(DIST)/include/nspr/os

install:: export
