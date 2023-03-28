

JSRC = *.java
JOBJ = $(JSRC:.java=.class)

DOC_PACKAGE = $(subst /,.,$(PACKAGE))
DOCOBJ = $(addprefix $(DOC_PACKAGE).,$(JSRC:.java=.html))

TARGETS = $(JOBJ) $(DOCOBJ) $(DOC_PACKAGE).html

include <$(DEPTH)/sun-java/config/rules.mak>

JCC = $(DEPTH)\sun-java\java\$(OBJDIR)\java java.tools.javac.Main -verbose

install:: $(TARGETS)
	$(INSTALL) *.class $(DIST)\classes\$(PACKAGE)
	$(INSTALL) *.html $(DIST)\doc\api

doc::
	echo $(DOC_PACKAGE)

$(DOC_PACKAGE).%.html: %.java
	-$(JAVADOCFF) $<

$(DOC_PACKAGE).html: Makefile $(JSRC) $(DEPTH)\sun-java\config\classes.mk
	@echo Generating  $@
	rm -f $@
	echo '<ul>' >> $@
	for %%i in ( $(DOCOBJ) ) do	\
		echo '<li> <a href="%%i">'%%i'</a>' >> $@
	echo '</ul>' >> $@
