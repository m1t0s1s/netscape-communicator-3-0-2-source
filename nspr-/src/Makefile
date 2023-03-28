#! gmake

NSPRDEPTH = ..

GC =		   \
	prgcapi.c  \
	prmsgc.c   \
	$(NULL)

include $(NSPRDEPTH)/config/config.mk

# Disable optimization of the nspr on SunOS4.1.3
ifeq ($(OS_ARCH),SunOS)
ifeq ($(OS_RELEASE),4.1.3_U1)
OPTIMIZER =
endif
endif

CFILES =		\
	file.c		\
	getopt.c	\
	linker.c	\
	log2.c		\
	longlong.c	\
	page.c		\
	prabort.c	\
	prarena.c	\
	prdump.c	\
	prevent.c	\
	prhash.c	\
	prmcache.c	\
	prnetdb.c	\
	prtime.c	\
	prmjtime.c	\
	prfile.c	\
	prfileio.c	\
	prlog.c		\
	prnetio.c	\
	prdtoa.c	\
	prprf.c		\
	prsystem.c	\
	$(GC)		\
	swintr.c	\
	swmon.c		\
	swsched.c	\
	swthread.c	\
	md_$(OS_ARCH).c	\
	$(NULL)

ifndef DBMALLOC
CFILES += prmalloc.c
endif

PURE_CFILES =	 \
	mdunix.c \
	prmem.c	 \
	$(NULL)

ifeq ($(OS_TEST),i86pc)
ASFILES		= os_$(OS_ARCH)_x86.s
else
ASFILES		= os_$(OS_ARCH).s
endif


OBJS		= $(CFILES:.c=.o) $(PURE_CFILES:.c=.o) $(ASFILES:.s=.o)

PURE_OBJS	= $(addprefix $(OBJDIR)/,$(CFILES:.c=.o))  \
		  $(addprefix $(OBJDIR)/,$(ASFILES:.s=.o)) \
		  $(OBJDIR)/pureprmem.o			   \
		  $(OBJDIR)/puremdunix.o

LIBRARY		= nspr.a
PURE_LIBRARY	= $(OBJDIR)/purenspr.a

TARGETS		= $(LIBRARY)
ifdef HAVE_PURIFY
TARGETS		+= $(PURE_LIBRARY)
endif

ifeq ($(OS_ARCH),AIX)
TARGETS += $(OBJDIR)/aixwrap.o
endif

INCLUDES += -I../include

include $(NSPRDEPTH)/config/rules.mk

$(PURE_LIBRARY): $(PURE_OBJS)
	rm -f $@
	$(AR) cr $@ $(PURE_OBJS)
	$(RANLIB) $@

$(OBJDIR)/pureprmem.o: prmem.c
	$(CC) -o $@ -c -DPURIFY $(CFLAGS) prmem.c

$(OBJDIR)/puremdunix.o: mdunix.c
	$(CC) -o $@ -c -DPURIFY $(CFLAGS) mdunix.c

export:: $(TARGETS)
	$(INSTALL) -m 444 $(TARGETS) $(DIST)/lib

install:: export

clobber::
	rm -f $(PURE_OBJS)
