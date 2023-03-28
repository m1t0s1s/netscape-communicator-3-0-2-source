# kludge to build ns/security/ in a 3.01-based tree instead of 4.0.

OBJS = $(CSRCS:.c=.o)
LIBRARY = lib$(LIBRARY_NAME).a

#all:: export_hdrs
all::

export_hdrs::
	+$(LOOP_OVER_DIRS)

ifdef EXPORTS
export_hdrs:: $(EXPORTS)
	$(INSTALL) -m 444 $(EXPORTS) $(DIST)/include
export:: export_hdrs
endif

include $(DEPTH)/config/rules.mk

# This is a kludge so that running "make netscape-export" in any of these
# directories will rebuild the libraries, then relink cmd/xfe/netscape-us
#
netscape-export netscape-export.pure: export
	cd $(DEPTH)/lib/xp; $(MAKE)
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@
