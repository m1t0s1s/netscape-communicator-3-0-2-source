#! gmake

JSRCS = $(wildcard *.java)
JOBJS = $(JSRCS:.java=.class)

TARGETS = jobjs

include $(DEPTH)/sun-java/config/rules.mk

JAVAC =	$(JAVAI) -classpath /usr/local/netscape/java/lib/javac.zip \
	     -ms8m sun.tools.javac.Main
JAVAC_CLASSPATH = $(CLASSSRC):.

jobjs::
	@list=`perl $(DEPTH)/sun-java/classsrc/outofdate.pl $(PERLARG)  \
		    $(JSRCS)`;						\
	if test "$$list"x != "x"; then					\
	    echo $(JCCF) $$list;					\
	    $(JCCF) $$list;						\
	fi

clobber::
	rm -f *.class
