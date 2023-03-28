#! gmake

DEPTH = ../..

CSRCS =		    \
	cvactive.c  \
	cvcolor.c  \
	cvdisk.c    \
	cvextcon.c  \
	cvmime.c    \
	cvproxy.c   \
	cvview.c    \
	extcache.c  \
	mkaccess.c  \
	mkautocf.c  \
	mkcache.c   \
	mkconect.c  \
	mkextcac.c  \
	mkfile.c    \
	mkformat.c  \
	mkfsort.c   \
	mkftp.c	    \
	mkgeturl.c  \
	mkgopher.c  \
	mkhelp.c    \
	mkhttp.c    \
	mkinit.c    \
	mkmailbx.c  \
	mkmemcac.c  \
	mkmessag.c  \
	mkmocha.c   \
	mknews.c    \
	mknewsgr.c  \
	mkparse.c   \
	mkpop3.c    \
	mkremote.c  \
	mksmtp.c    \
	mksockrw.c  \
	mksort.c    \
	mkstream.c  \
	mkutils.c   \
	txview.c    \
	unix-dns.c  \
	$(NULL)

OBJS = $(CSRCS:.c=.o)

LIBRARY = libnet.a

include $(DEPTH)/config/rules.mk

# This is a kludge so that running "make netscape-export" in this directory
#   will rebuild this library, and a couple other libraries that jwz tends
#   to change, and then relink ../../cmd/xfe/netscape-export
#
#   LJM - Added same kludge to ns tree. :)
netscape-export netscape-export.pure: libnet.a
	cd $(DEPTH)/lib/libmime; $(MAKE) libmime.a
	cd $(DEPTH)/lib/libmsg; $(MAKE) libmsg.a
	cd $(DEPTH)/lib/xp; $(MAKE) libxp.a
	cd $(DEPTH)/cmd/xfe; $(MAKE) $@

DEFINES += -DNEW_DECODERS

INCLUDES += -I../libparse

mkformat.o: mktypes.h

#
# Workaround for a SCO compiler bug.
#
# If mkutils.c is compiled optimized, password authorization
# always fails.  Probably due to NET_UUEncode routine.
#
# We could try using GCC 2.7.x instead, since that is the first
# version that can produce SCO ELF binaries.
#
ifeq ($(OS_ARCH),SCO_SV)
$(OBJDIR)/mkutils.o: mkutils.c
	$(CCF) -O0 -o $@ -c $<
endif

# This converts mime.types to C source code,
#  and also to an HTML table that shows what's in it.

MIME_TYPES         = ./mime.types
MIME_TYPES_UNIX    = ./mime.types-unix
MIME_TYPES_NONUNIX = ./mime.types-nonunix
AD2C               = sh $(DEPTH)/cmd/xfe/ad2c

mktypes.h: $(MIME_TYPES) $(MIME_TYPES_UNIX) $(MIME_TYPES_NONUNIX)
	rm -f $@
	@echo "generating $@ from $<..."			; \
	 (echo '/* Generated file - do not edit! */'		; \
	  echo ''						; \
	  cat $(MIME_TYPES) | grep -v '^#' | $(AD2C)		; \
	  echo ''						; \
	  echo '#ifdef XP_UNIX'					; \
	  cat $(MIME_TYPES_UNIX) | grep -v '^#' | $(AD2C)	; \
	  echo '#else  /* !XP_UNIX */'				; \
	  cat $(MIME_TYPES_NONUNIX) | grep -v '^#' | $(AD2C)	; \
	  echo '#endif /* !XP_UNIX */'				; \
	 ) > $@

mime-types.html: $(MIME_TYPES) $(MIME_TYPES_UNIX)
	rm -f $@
	@echo "generating $@ from $<..."				; \
	 (echo '<TITLE>Netscape MIME Types</TITLE>'			; \
	  echo '<H1 ALIGN=CENTER>Netscape MIME Types</H1>'		; \
	  echo '<CENTER>'						; \
	  echo '<TABLE BORDER CELLSPACING=0 CELLPADDING=5>'		; \
	  echo '<TR><TH></TH><TH>Content Type:</TH>'			; \
	  echo '<TH>Extensions:</TH><TH>Description:</TH></TR>'		; \
	  echo '<TR></TR><TR></TR><TR></TR>'	; \
	  cat $(MIME_TYPES) $(MIME_TYPES_UNIX) | grep -v '^#' |		   \
	  sed  's@exts="\([^"]*\)"[ 	]*[\]*@<TR><TD><TT>\1</TT></TD>@;  \
		s@desc="\([^"]*\)"[ 	]*[\]*@<TD>\1</TD>@;		   \
		s@enc=\([^ 	]*\)[ 	]*[\]*$$@<TD>encoding/\1</TD>@;	   \
		s@type=\([^ 	]*\)[ 	]*[\]*$$@<TD>\1</TD>@;		   \
		s@icon=\([^ 	]*\)[ 	]*[\]*$$@<TD><IMG SRC=\1></TD></TR>@'\
	  | sed 's/,/ /g'						  \
	  | awk '{if ( $$0 ~ /^<TR/ ) {printf "%s" , $$0}		  \
		  else {printf "%s\n" , $$0} }'				  \
	  | sed 's@\(<TR>\)\(<TD>.*</TD>\)\(<TD>.*</TD>\)\(<TD>.*</TD>\)\(<TD>.*</TD>\)\(</TR>\)$$@\1\5!\3!\2!\4\6@' \
	  | tr '!' '\012'						  \
	  | sed 's@<TD></TD>@<TD>\&nbsp;</TD>@'				; \
	  echo '</TABLE>'						; \
	  echo '</CENTER>'						; \
	 ) > $@
