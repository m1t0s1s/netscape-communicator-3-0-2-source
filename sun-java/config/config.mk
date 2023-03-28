#! gmake

# Configuration information for building in the java source module

# Include baseline configuration
include $(DEPTH)/config/config.mk

DEFINES += -DNETSCAPE -DCOMPILER

# Only build in method tracing for debug version of the runtime
ifndef BUILD_OPT
DEFINES += -DTRACING
else
DEFINES += -DTRIMMED
endif

# XXX for now we dont't build in the breakpoints support

INCLUDES += -I$(DEPTH)/sun-java/include -I$(DEPTH)/sun-java/md-include \
	    -I$(DEPTH)/sun-java/include/_gen -I$(DIST)/include/nspr \
	    -I$(DIST)/include/jpeg

LIBJAVA = $(DIST)/lib/javart.a
LIBMD = $(DIST)/lib/sunmd.a
# LIBJAVADLL is also javart.a because it defines the jri 
# symbols that won't otherwise be pulled in:
LIBJAVADLL = $(DIST)/lib/javart.a
LIBJPEG = $(DIST)/lib/libjpeg.a

LIBRARIES =	$(LIBJAVA) $(LIBMD) $(LIBJAVADLL) $(LIBJPEG) \
                $(LIBNSPR) -lm $(OS_LIBS) $(DEBUG_LIBS)

CLASSSRC = $(DEPTH)/sun-java/classsrc
RT_CLASSPATH = $(CLASSSRC)
JAVAC_CLASSPATH = $(CLASSSRC)

# Never optimize java code when we compile here
JAVA_OPTIMIZER	= -g
JAVAI_FLAGS = -classpath $(CLASSSRC) -ms8m

JAVAH = $(DIST)/bin/javah
JVH = $(JAVAH) -classpath $(CLASSSRC)
