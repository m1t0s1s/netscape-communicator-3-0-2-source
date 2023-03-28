#! gmake

DEPTH = ../..

include $(DEPTH)/sun-java/config/config.mk

DIRS = stub

ifeq ($(OS_ARCH), IRIX)
DIRS = irix
endif

ifeq ($(OS_ARCH), SunOS)
DIRS = solaris
endif

include $(DEPTH)/sun-java/config/rules.mk
