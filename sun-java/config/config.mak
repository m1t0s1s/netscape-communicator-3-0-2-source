
# Configuration information for building in the java source module

!ifdef BUILD_OPT
LCFLAGS = -DTRIMMED 
!else
LCFLAGS = $(XCFLAGS) -DHASHMETER -DHEAPMETER -DTRACING -D_USE_PR_DEBUG_MEMORY
!endif

!ifdef INTLUNICODE
LCFLAGS=$(LCFLAGS) -DINTLUNICODE -DIN_JRT -DNETSCAPE -DCOMPILER
!else
LCFLAGS=$(LCFLAGS) -DIN_JRT -DNETSCAPE -DCOMPILER
!endif

LINCS=$(LINCS) -I$(DEPTH)/sun-java/include -I$(DEPTH)/sun-java/md-include \
      -I$(DEPTH)/sun-java/include/_gen -I$(DEPTH)/dist/public/security

LIBJRT=$(DIST)\lib\jrt$(OS_RELEASE)301.lib
LIBJAVA=$(DIST)\lib\javart$(OS_RELEASE).lib
LIBMD=$(DIST)\lib\win$(OS_RELEASE)md.lib

LIBRARIES = $(LIBJAVA) $(LIBMD) $(LIBNSPR) $(OS_LIBS) $(DEBUG_LIBS)

CLASSSRC = $(DEPTH)/sun-java/classsrc

JVH = $(JAVAH) -classpath $(BOOT_CLASSPATH) 

BOOT_CLASSPATH=.;$(DEPTH)/sun-java/classsrc
DIST_CLASSPATH=$(DEPTH)/dist/classes
CLASSPATH = $(DIST_CLASSPATH)

JCC = $(JAVAC)
JCFLAGS = -classpath $(CLASSPATH)
JCCF = $(JCC) $(JCFLAGS)

# Rules to build java .html files from java source files

JAVADOC = $(JAVAI) java.tools.javadoc.Main
JAVADOC_FLAGS = -classpath $(CLASSPATH)
JAVADOCFF = $(JAVADOC) $(JAVADOC_FLAGS)
