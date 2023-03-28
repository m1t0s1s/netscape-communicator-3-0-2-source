#! gmake

DEPTH = ../../../../..

PACKAGES = netscape/tools/DumpClass

include $(DEPTH)/sun-java/config/classes.mk

BUILD_CLASSPATH = $(BOOT_CLASSPATH):$(DIST_CLASSPATH):$(DEPTH)/sun-java/classsrc/

DUMP = java -classpath $(DEPTH)/sun-java/classsrc netscape.tools.DumpClass.DumpClass

test: all
	$(DUMP) $(DEPTH)/sun-java/classsrc/java/lang/Math.class
