#! gmake

DEPTH = ../../../../..

PACKAGES = netscape/tools/jric

include $(DEPTH)/sun-java/config/classes.mk

BUILD_CLASSPATH = $(BOOT_CLASSPATH):$(DIST_CLASSPATH):$(DEPTH)/sun-java/classsrc/

JRIC = java -classpath $(DEPTH)/sun-java/classsrc netscape.tools.jric.Main

test:	all
	$(JRIC) -classpath $(DEPTH)/sun-java/classsrc -stubs	java.lang.Math
	$(JRIC) -classpath $(DEPTH)/sun-java/classsrc		java.lang.Math
