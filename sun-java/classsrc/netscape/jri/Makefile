#! gmake

DEPTH = ../../../..

PACKAGES = netscape/jri

include $(DEPTH)/sun-java/config/classes.mk

BUILD_CLASSPATH = $(BOOT_CLASSPATH):$(DIST_CLASSPATH):$(DEPTH)/sun-java/classsrc/

JMC = java -classpath $(DEPTH)/sun-java/classsrc netscape.tools.jmc.Main

all::
#	$(JMC) -classpath .:../.. -prefix NS -interface -verbose Test
#	$(JMC) -classpath .:../.. -prefix NS -module    -verbose Test
	$(JMC) -classpath .:../.. -prefix JRI -interface -verbose JRIEnv JRIRuntime JRIEnvControl JRIReflection JRIDebugger
	$(JMC) -classpath .:../.. -prefix JRI -module    -verbose JRIEnv JRIRuntime JRIEnvControl JRIReflection JRIDebugger
