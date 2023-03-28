#
# Common rules used by lots of makefiles...
#
ifndef NS_CONFIG_MK
include $(DEPTH)/config/config.mk
endif

ifndef TARGETS
TARGETS		= $(PROGRAM) $(LIBRARY)
endif

ifdef PROGRAM
PROGRAM		:= $(addprefix $(OBJDIR)/, $(PROGRAM))
endif

ifdef LIBRARY
LIBRARY		:= $(addprefix $(OBJDIR)/, $(LIBRARY))
endif

ifdef OBJS
OBJS		:= $(addprefix $(OBJDIR)/, $(OBJS))
endif

define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); $(NSINSTALL) -D $(@D); fi
endef

ifdef ALL_PLATFORMS
all_platforms:: $(NFSPWD)
	@d=`$(NFSPWD)`;                                                       \
	if test ! -d LOGS; then rm -rf LOGS; mkdir LOGS; fi;                  \
	for h in $(PLATFORM_HOSTS); do                                        \
		echo "On $$h: $(MAKE) $(ALL_PLATFORMS) >& LOGS/$$h.log";      \
		rsh $$h -n "(chdir $$d;                                       \
			     $(MAKE) $(ALL_PLATFORMS) >& LOGS/$$h.log;        \
			     echo DONE) &" 2>&1 > LOGS/$$h.pid &              \
		sleep 1;                                                      \
	done

$(NFSPWD):
	cd $(@D); $(MAKE) $(@F)
endif

all:: $(TARGETS) $(DIRS)

$(PROGRAM): $(OBJS)
	@$(MAKE_OBJDIR)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS)

$(LIBRARY): $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

.SUFFIXES: .i .pl .class .java .html

.PRECIOUS: .java

$(OBJDIR)/%: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ $(CFLAGS) $*.c $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(CFLAGS) $*.c

$(OBJDIR)/%.o: %.s
	@$(MAKE_OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $*.s

$(OBJDIR)/%.o: %.S
	@$(MAKE_OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $*.S

$(OBJDIR)/%: %.cpp
	@$(MAKE_OBJDIR)
	$(CCC) -o $@ $(CFLAGS) $*.c $(LDFLAGS)

#	Please keep the next two rules in sync.
$(OBJDIR)/%.o: %.cc
	@$(MAKE_OBJDIR)
	$(CCC) -o $@ -c $(CFLAGS) $*.cc

$(OBJDIR)/%.o: %.cpp
	@$(MAKE_OBJDIR)
ifdef STRICT_CPLUSPLUS_SUFFIX
	echo "#line 1 \"$*.cpp\"" | cat - $*.cpp > $(OBJDIR)/t_$*.cc
	$(CCC) -o $@ -c $(CFLAGS) $(OBJDIR)/t_$*.cc
	rm -f $(OBJDIR)/t_$*.cc
else
	$(CCC) -o $@ -c $(CFLAGS) $*.cpp
endif #STRICT_CPLUSPLUS_SUFFIX

%.i: %.cpp
	$(CCC) -C -E $(CFLAGS) $< > $*.i

%.i: %.c
	$(CC) -C -E $(CFLAGS) $< > $*.i

%: %.pl
	rm -f $@; cp $*.pl $@; chmod +x $@

%: %.sh
	rm -f $@; cp $*.sh $@; chmod +x $@

ifdef DIRS

LOOP_OVER_DIRS =						\
	@for d in $(DIRS); do					\
		if test -d $$d; then				\
			set -e;					\
			echo "cd $$d; $(MAKE) $@";		\
			cd $$d; $(MAKE) $@; cd ..;		\
			set +e;					\
		else						\
			echo "Skipping non-directory $$d...";	\
		fi;						\
	done

$(DIRS)::
	@if test -d $@; then				\
		set -e;					\
		echo "cd $@; $(MAKE)";			\
		cd $@; $(MAKE);				\
		set +e;					\
	else						\
		echo "Skipping non-directory $@...";	\
	fi

endif # DIRS

clean::
	rm -f $(OBJS) $(NOSUCHFILE)
	+$(LOOP_OVER_DIRS)

clobber::
	rm -f $(OBJS) $(TARGETS) $(GARBAGE) $(NOSUCHFILE)
	+$(LOOP_OVER_DIRS)

clobber_all::
	rm -rf LOGS TAGS *.OBJ $(OBJS) $(TARGETS) $(GARBAGE)
	+$(LOOP_OVER_DIRS)

export::
	+$(LOOP_OVER_DIRS)

install::
	+$(LOOP_OVER_DIRS)

mac::
	+$(LOOP_OVER_DIRS)

# Provide default export & install rules when using LIBRARY
ifdef LIBRARY
# XXX arguably wrong - kipp
export:: $(LIBRARY)
	$(INSTALL) -m 444 $(LIBRARY) $(DIST)/lib

install:: $(LIBRARY)
	$(INSTALL) -m 444 $(LIBRARY) $(DIST)/lib
endif

-include $(DEPENDENCIES)

# Can't use sed because of its 4000-char line length limit, so resort to perl
.DEFAULT:
	@perl -e '                                                            \
	    open(MD, "< $(DEPENDENCIES)");                                    \
	    while (<MD>) {                                                    \
		if (m@ \.*/*$< @) {                                           \
		    $$found = 1;                                              \
		    last;                                                     \
		}                                                             \
	    }                                                                 \
	    if ($$found) {                                                    \
		print "Removing stale dependency $< from $(DEPENDENCIES)\n";  \
		seek(MD, 0, 0);                                               \
		$$tmpname = "$(OBJDIR)/fix.md" . $$$$;                        \
		open(TMD, "> " . $$tmpname);                                  \
		while (<MD>) {                                                \
		    s@ \.*/*$< @ @;                                           \
		    if (!print TMD "$$_") {                                   \
			unlink(($$tmpname));                                  \
			exit(1);                                              \
		    }                                                         \
		}                                                             \
		close(TMD);                                                   \
		if (!rename($$tmpname, "$(DEPENDENCIES)")) {                  \
		    unlink(($$tmpname));                                      \
		}                                                             \
	    } elsif ("$<" ne "$(DEPENDENCIES)") {                             \
		print "$(MAKE): *** No rule to make target $<.  Stop.\n";     \
		exit(1);                                                      \
	    }'

-include $(MY_RULES)

$(MY_CONFIG):
$(MY_RULES):

alltags:
	rm -f TAGS
	find . -name dist -prune -o \( -name '*.[hc]' -o -name '*.cc' -o -name '*.cp' -o -name '*.cpp' \) -print | xargs etags -a

# Generate Emacs tags in a file named TAGS if ETAGS was set in $(MY_CONFIG)
# or in $(MY_RULES)
ifdef ETAGS
ifneq ($(CSRCS)$(HEADERS),)
all:: TAGS
TAGS:: $(CSRCS) $(HEADERS)
	$(ETAGS) $(CSRCS) $(HEADERS)
endif
endif

# Generate JRI Headers and Stubs into the _gen and _stubs directories

CLASSSRC = $(DEPTH)/sun-java/classsrc
JRIH = $(DIST)/bin/javah -jri -classpath $(CLASSSRC)

ifdef JRI_HEADER_CLASSES

JRI_HEADER_CLASSFILES = $(patsubst %,$(CLASSSRC)/%.class,$(subst .,/,$(JRI_HEADER_CLASSES)))
JRI_HEADER_CFILES = $(patsubst %,_gen/%.c,$(subst .,_,$(JRI_HEADER_CLASSES)))

$(JRI_HEADER_CFILES): $(JRI_HEADER_CLASSFILES)

mac_jri_headers:
	@echo Generating/Updating JRI headers for the Mac
	$(JRIH) -mac -d $(DEPTH)/lib/mac/Java/_gen $(JRI_HEADER_CLASSES)

jri_headers:
	@echo Generating/Updating JRI headers
	$(JRIH) -d _gen $(JRI_HEADER_CLASSES)

endif

ifdef JRI_STUB_CLASSES

JRI_STUB_CLASSFILES = $(patsubst %,$(CLASSSRC)/%.class,$(subst .,/,$(JRI_STUB_CLASSES)))
JRI_STUB_CFILES = $(patsubst %,_stubs/%.c,$(subst .,_,$(JRI_STUB_CLASSES)))

$(JRI_STUB_CFILES): $(JRI_STUB_CLASSFILES)

mac_jri_stubs:
	@echo Generating/Updating JRI stubs for the Mac
	$(JRIH) -mac -stubs -d $(DEPTH)/lib/mac/Java/_stubs $(JRI_STUB_CLASSES)

jri_stubs:
	@echo Generating/Updating JRI stubs
	$(JRIH) -stubs -d _stubs $(JRI_STUB_CLASSES)

endif

.PHONY: all all_platforms alltags clean clobber clobber_all boot export \
	install jri_headers jri_stubs mac_jri_headers mac_jri_stubs \
	$(OBJDIR) $(DIRS)

