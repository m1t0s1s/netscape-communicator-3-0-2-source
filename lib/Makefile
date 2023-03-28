#! gmake

DEPTH = ..

include $(DEPTH)/config/config.mk

DIRS =	layout libcgi libdbm libi18n libimg libmime libmisc libmsg \
	libnet libparse plugin xlate xp libjar

ifndef NO_JAVA
DIRS += libjava
endif

ifndef NO_MOCHA
DIRS += libmocha
endif

include $(DEPTH)/config/rules.mk


# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild the libraries, then relink cmd/xfe/netscape-export
#
netscape-export netscape-export.pure: export
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
