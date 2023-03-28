#! gmake

DEPTH = ../../..

include $(DEPTH)/config/config.mk


CSRCS	= dongle.c secpwd.c derprint.c err.c secarb.c secutil.c

OBJS	= $(CSRCS:.c=.o)

LIBRARY_NAME = sectool

INCLUDES += -I../include \
	-I$(DEPTH)/dist/public/security\
	-I$(DEPTH)/dist/public/nspr\
	-I../../lib/cert\
	$(NULL)

DEFINES += -DUSE_SSL

include $(DEPTH)/security/secrules.mk


