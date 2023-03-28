#! gmake

CSRCS =         \
    extcache.c  \
    mkaccess.c  \
    mkautocf.c  \
    mkcache.c   \
    mkconect.c  \
    mkextcac.c  \
    mkfile.c    \
    mkformat.c  \
    mkfsort.c   \
    mkftp.c     \
    mkgeturl.c  \
    mkgopher.c  \
    mkhttp.c    \
    mkinit.c    \
    mkmemcac.c  \
    mkmessag.c  \
    mkmocha.c   \
    mknews.c    \
    mknewsgr.c  \
    mkparse.c   \
    mkremote.c  \
    mksmtp.c    \
    mksockrw.c  \
    mksort.c    \
    mkstream.c  \
    mkutils.c   \
    txview.c    \
    $(NULL)

OBJS = $(CSRCS:.c=.o)

OBJDIR=IRIX5.3_DBG.OBJ

DEPTH = ../../..
DIST = $(DEPTH)/dist/$(OBJDIR)
LIBDIST = $(DIST)/lib

LIBNET = $(LIBDIST)/libnet.a 
LIBSEC = $(LIBDIST)/libsec-us.a 
LIBXP  = $(LIBDIST)/libxp.a 
LIBDBM = $(LIBDIST)/libdbm.a 
NSPR   = $(LIBDIST)/nspr.a 

CFLAGS = -g -DDEBUG \
		-I$(DEPTH)/dist/$(OBJDIR)/include \
		-I$(DEPTH)/lib/libnet \
		-I$(DEPTH)/nspr/include \
		-I$(DEPTH)/nspr/include/IRIX5.3_DBG.OBJ/

TESTSRCS    =     \
	cfile.c   \
	testnet.c \
	xpfile.c  \
	$(NULL)

TESTOBJS = $(TESTSRCS:.c=.o)

all: $(OBJS) $(TESTOBJS)
	$(CC) -g -o nettest $(TESTOBJS) $(OBJS) $(LIBSEC) $(LIBXP) $(LIBDBM) $(NSPR) 

depend:
	makedepend $(INCLUDES) *.c *.h *.cpp > /dev/null  2>&1

test:
	@echo BUILD_SERVER = $(BUILD_SERVER)

dolink:; ln -s ../*.c .

.PHONY: dolink

# DO NOT DELETE THIS LINE -- make depend depends on it.

