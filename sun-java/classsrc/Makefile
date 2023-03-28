#! gmake --no-print-directory

DEPTH = ../..

# A list of the packages we will build
PACKAGES =		   \
	java/lang	   \
	java/io		   \
	java/util	   \
	java/net	   \
	java/awt	   \
	java/awt/peer	   \
	java/awt/image	   \
	java/applet	   \
	sun/audio	   \
	sun/awt		   \
	sun/awt/image	   \
	sun/awt/macos	   \
	sun/awt/motif	   \
	sun/awt/win32	   \
	sun/awt/windows	   \
	sun/misc	   \
	netscape/applet	   \
	netscape/plugin	   \
	netscape/javascript\
	netscape/net       

OPTIONAL_PACKAGES =		  \
	sun/net			  \
	sun/net/ftp		  \
	sun/net/nntp		  \
	sun/net/smtp		  \
	sun/net/www		  \
	sun/net/www/auth	  \
	sun/net/www/content/image \
	sun/net/www/content/text  \
	sun/net/www/http	  \
	sun/net/www/protocol/doc  \
	sun/net/www/protocol/file \
	sun/net/www/protocol/http \
	$(NULL)

# A list of the packages that are not being built. This is needed to make
# sure that all .class files get clobbered properly
NOTBUILT =			  \
	sun/applet		  \
	sun/net			  \
	sun/net/ftp		  \
	sun/net/nntp		  \
	sun/net/smtp		  \
	sun/net/www		  \
	sun/net/www/auth	  \
	sun/net/www/content/image \
	sun/net/www/content/text  \
	sun/net/www/http	  \
	sun/net/www/protocol/doc  \
	sun/net/www/protocol/file \
	sun/net/www/protocol/http \
	sun/tools/asm		  \
	sun/tools/java		  \
	sun/tools/javac		  \
	sun/tools/javadoc	  \
	sun/tools/tree		  \
	sun/tools/zip		  \
	sun/tools/debug		  \
	sun/tools/ttydebug	  \
	netscape/java/tests	  \
	netscape/net/tests	  \
	netscape/nspr/tests

TARGETS = targets

include $(DEPTH)/sun-java/config/rules.mk

JAVAC_ZIP = /usr/local/netscape/java/lib/javac.zip
#JAVAC_ZIP = ../javac/javac.zip

# Force JAVAC to something that makes the runtime not get . in it's path.
# Otherwise if there were incompatible changes in the source tree, the
# runtime would use them and crash.
JAVAC =	$(JAVAI) -classpath $(JAVAC_ZIP) \
	     -ms8m -verify sun.tools.javac.Main

# Force JAVADOC to something that makes the runtime not get . in it's path.
# Otherwise if there were incompatible changes in the source tree, the
# runtime would use them and crash.
JAVADOC = $(JAVAI) -classpath .:$(JAVAC_ZIP) \
	     -ms8m sun.tools.javadoc.Main

# Similar to the above, except that javac.car doesn't have netscape/tools
# in it. So tack on a "." in the path so that the car program can find
# it's own classes. The remainder of the runtime will use javac.car. As
# long as netscape.tools.car.* is compatible across runtime changes this
# will work dandy.
CAR =	$(JAVAI) -classpath $(JAVAC_ZIP):. \
	     -ms8m netscape.tools.car.Main

all:: $(TARGETS)

# Build everything relative to this source. Use the bootstrap compiler.
# This means you have to build in ../boot before you can build in here.

#JAVAI_FLAGS = -classpath ../boot/boot.car -ms8m
JAVAC_CLASSPATH = .
INCLUDES += -I../boot/$(OBJDIR)

JAVADIR = sun/tools/java

GARBAGE += $(JAVADIR)/RuntimeConstants.java

targets::
	$(MAKE) $(JAVADIR)/RuntimeConstants.java
	$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_NSINSTALL=$(NSINSTALL) build_packages
	$(MAKE) BUILD_OPT=1 ODIST=$(DIST) REAL_NSINSTALL=$(NSINSTALL) zips

export:: targets

# Compiler has an overwhelming preference for source vs. binary when
# you put the binary files in a different directory than the source.
# SIGH. We could do that, but then the builds slow *way* down

.PHONY: build_packages

build_packages:
	@for i in $(PACKAGES); do		 \
	    list=`perl outofdate.pl $$i/*.java`; \
	    if test "$$list"x != "x"; then	 \
		echo $(JCCF) $$list;		 \
		$(JCCF) $$list;			 \
	    fi;					 \
	done

optional::
	@for i in $(OPTIONAL_PACKAGES); do	 \
	    list=`perl outofdate.pl $$i/*.java`; \
	    if test "$$list"x != "x"; then	 \
		echo $(JCCF) $$list;		 \
		$(JCCF) $$list;			 \
	    fi;					 \
	done

install:: $(TARGETS)
	$(INSTALL) *.zip $(DIST)/classes;			 \
	for i in $(PACKAGES); do				 \
		echo $(INSTALL) $$i/*.class $(DIST)/classes/$$i; \
		$(INSTALL) $$i/*.class $(DIST)/classes/$$i;	 \
	done

doc::
	$(INSTALL) ../images $(DIST)/doc
	$(INSTALL) index.html $(DIST)/doc
	$(JAVADOC) -d $(DIST)/doc	\
		netscape.plugin		\
		netscape.javascript

clobber::
	@$(MAKE) real_clobber
	@$(MAKE) BUILD_OPT=1 real_clobber
	@for i in $(PACKAGES) $(NOTBUILT); do \
		echo rm -f $$i/*.class;	      \
		rm -f $$i/*.class;	      \
	done

real_clobber::
	rm -rf $(OBJDIR)

clean:: clobber

natives_list:: force
	rm -rf $@
	find . -name "*.class" -print | sed 's@\./\(.*\)\.class$$@\1@' | \
		sed 's@/@.@g' | xargs $(JVH) -natives | sort > $@

force::

check_natives:: natives_list
	rm -f found_natives
	nm -B ../runtime/$(OBJDIR)/*.o ../md/$(OBJDIR)/*.o \
		../awt/x/$(OBJDIR)/*.o ../mmedia/irix/$(OBJDIR)/*.o \
		../netscape/applet/$(OBJDIR)/*.o \
		../netscape/plugin/$(OBJDIR)/*.o \
		../netscape/javascript/$(OBJDIR)/*.o \
		../netscape/net/$(OBJDIR)/*.o \
	    | egrep "Java.*_stub" | awk '{ print $$3; }' | sort > found_natives
	diff found_natives natives_list

######################################################################

# Rules to build RuntimeConstants.java

GENPROG = $(OBJDIR)/genconst

CSRCS = genconst.c

OBJS = $(OBJDIR)/genconst.o

INCLUDES += -I$(ODIST)/include

OPCODES = ../include/opcodes.init ../include/opcodes.length

$(GENPROG): $(OPCODES) $(OBJS)
	@$(MAKE_OBJDIR)
	$(CCF) -o $@ $(OBJS)

$(JAVADIR)/RuntimeConstants.java: $(GENPROG) Makefile
	@echo Generating $@;			     \
	(echo "/*" `date` " */";		     \
         echo "package sun.tools.java;";	     \
	 echo "public interface RuntimeConstants {"; \
	 $(OBJDIR)/genconst;			     \
	 echo "}") > $@

$(OPCODES):
	cd ../include; $(MAKE) opcodes

######################################################################

# Rules to build the ".car" files

# Packages used for the various car's

# The boot car is used for boostrapping the compiler. It contains just
# enough classes to execute the compiler AND to run javah to produce
# all of the includes that the runtime depends on (see include/classfiles.mk)
BOOT_CAR =		\
	java/io		\
	java/lang	\
	java/util	\
	java/net	\
	java/awt/image	\
	java/awt/peer	\
	sun/tools/asm	\
	sun/tools/java	\
	sun/tools/javac	\
	sun/tools/tree	\
	sun/awt/image

CAR_OBJF = $(wildcard $(dir)/*.class)
BOOT_JOBJ = $(foreach dir,$(BOOT_CAR),$(CAR_OBJF))

MOZILLA_CAR =		\
	java/applet	\
	java/awt	\
	java/awt/image	\
	java/awt/peer	\
	java/io		\
	java/lang	\
	java/net	\
	java/util	\
	netscape/applet	\
	netscape/plugin	\
	netscape/javascript	\
	netscape/net	\
	sun/audio	\
	sun/awt		\
	sun/awt/image	

MOZ_WIN_CAR =	\
	sun/awt/windows	

MOZ_MAC_CAR =	\
	sun/awt/macos

MOZ_X_CAR =	\
	sun/awt/motif

MOZILLA_COM_JOBJ = $(foreach dir,$(MOZILLA_CAR),$(CAR_OBJF)) \
	sun/misc/Cache.class				 \
	sun/misc/Ref.class

MOZILLA_WIN_JOBJ = $(foreach dir,$(MOZ_WIN_CAR),$(CAR_OBJF)) 

MOZILLA_MAC_JOBJ = $(foreach dir,$(MOZ_MAC_CAR),$(CAR_OBJF))

MOZILLA_X_JOBJ = $(foreach dir,$(MOZ_X_CAR),$(CAR_OBJF))

# The compiler car file should have everything in the public API in it
# so that we can compile anything that uses the public API, even if
# the compiler itself doesn't need it
JAVAC_CAR =		  \
	java/applet	  \
	java/awt	  \
	java/awt/image	  \
	java/awt/peer	  \
	java/io		  \
	java/lang	  \
	java/net	  \
	java/util	  \
	sun/tools/asm	  \
	sun/tools/java	  \
	sun/tools/javac	  \
	sun/tools/javadoc \
	sun/tools/tree	  \
	sun/tools/zip	  \
	sun/tools/debug	  \
	sun/tools/ttydebug

JAVAC_JOBJ = $(foreach dir,$(JAVAC_CAR),$(CAR_OBJF)) 

JDB_CAR = java sun

# List of classes which are excluded from the mozilla car file
# These are unsafe classes whose security mechanisms are not
# sufficiently trusted yet.
MOZILLA_EXCLUDE =				  \
	java/lang/UNIXProcess.class		  \
	netscape/net/NSAPISocketImpl.class	  \
	netscape/net/NSAPISocketImplFactory.class \
	netscape/net/NSAPISocketInputStream.class \
	netscape/net/NSAPISocketOutputStream.class

ZIPF	= zip -0rq


# You're all a bunch of weenies.
# Nuke the non-jwz branch to Do the Right Thing.
#
ifndef DEBUG_jwz

ZIPS 	= java_301

else	# do the right thing!

# This is duplicated in ns/sun-java/md/Makefile
VNNH=../../cmd/xfe/versionn.h
 get_vn=sed -n 's/^.define VERSION_NUMBER *\(.*\)$$/java_\1/p' $(VNNH)
 CLASS_FILE=${shell $(get_vn)}

ZIPS=$(CLASS_FILE)

endif

ZIPS_WIN = $(ZIPS).win
ZIPS_MAC = $(ZIPS).mac
ZIPS_X   = $(ZIPS).x

zips: $(ZIPS) $(ZIPS_WIN) $(ZIPS_MAC) $(ZIPS_X)

BUILD_NUMBER = $(DEPTH)/build/build_number

$(ZIPS): $(MOZILLA_COM_JOBJ) $(MOZILLA_WIN_JOBJ) $(MOZILLA_MAC_JOBJ) \
           $(MOZILLA_X_JOBJ) $(BUILD_NUMBER) Makefile
	@rm -rf $@
	@echo Building $@
	@bn=`cat $(BUILD_NUMBER)`;				  \
	 bnf=$$bn".class";					  \
	 echo $$bn > $$bnf;		   			  \
	 $(ZIPF) $@ $$bnf $(MOZILLA_COM_JOBJ) $(MOZILLA_WIN_JOBJ) \
          $(MOZILLA_MAC_JOBJ) $(MOZILLA_X_JOBJ) -x $(MOZILLA_EXCLUDE) ; \
         mv $@.zip $@; \
	 rm -f $$bnf 

$(ZIPS_WIN): $(MOZILLA_COM_JOBJ) $(MOZILLA_WIN_JOBJ) $(BUILD_NUMBER) Makefile
	@rm -rf $@
	@echo Building $@
	@bn=`cat $(BUILD_NUMBER)`;				  \
	 bnf=$$bn".class";					  \
	 echo $$bn > $$bnf;		   			  \
	 $(ZIPF) $@ $$bnf $(MOZILLA_COM_JOBJ) $(MOZILLA_WIN_JOBJ) \
          -x $(MOZILLA_EXCLUDE) ; \
	 rm -f $$bnf 

$(ZIPS_MAC): $(MOZILLA_COM_JOBJ) $(MOZILLA_MAC_JOBJ) $(BUILD_NUMBER) Makefile
	@rm -rf $@
	@echo Building $@
	@bn=`cat $(BUILD_NUMBER)`;				  \
	 bnf=$$bn".class";					  \
	 echo $$bn > $$bnf;		   			  \
	 $(ZIPF) $@ $$bnf $(MOZILLA_COM_JOBJ) $(MOZILLA_MAC_JOBJ) \
          -x $(MOZILLA_EXCLUDE) ; \
	 rm -f $$bnf 

$(ZIPS_X): $(MOZILLA_COM_JOBJ) $(MOZILLA_X_JOBJ) $(BUILD_NUMBER) Makefile
	@rm -rf $@
	@echo Building $@
	@bn=`cat $(BUILD_NUMBER)`;				\
	 bnf=$$bn".class";					\
	 echo $$bn > $$bnf;		   			\
	 $(ZIPF) $@ $$bnf $(MOZILLA_COM_JOBJ) $(MOZILLA_X_JOBJ) \
          -x $(MOZILLA_EXCLUDE) ; \
	 rm -f $$bnf 

../javac/javac.zip: $(JAVAC_JOBJ)
	@rm -rf $@
	@echo Building $@
	@$(ZIPF) $@ $(JAVAC_JOBJ)

../debug/jdb.zip: FORCE
	@rm -rf $@
	@echo Building $@
	@$(ZIPF) $@ $(JDB_CAR)

../javac/javac.car: $(JAVAC_JOBJ)
	@rm -rf $@
	@echo Building $@
	@$(CAR) -c $@ $(JAVAC_JOBJ)

../debug/jdb.car: FORCE
	@rm -rf $@
	@echo Building $@
	@$(CAR) -c $@ $(JDB_CAR)

FORCE:
