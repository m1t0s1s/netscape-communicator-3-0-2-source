#! gmake

# Configuration information for building in the NSPR source module

DEPTH = $(NSPRDEPTH)/..

include $(DEPTH)/config/config.mk

DEFINES += -D_NSPR
