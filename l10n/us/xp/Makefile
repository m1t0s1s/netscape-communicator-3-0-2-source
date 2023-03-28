#! gmake

L10NDEPTH	= ../..
REAL_XPDIR	= $(L10NDEPTH)/../lib/xp
EMACS           = xemacs

HSRCS		= $(wildcard *.html)
MSRCS		= $(wildcard *.msg)

#
# tr has a bug on linux.
#	tr '[\001-\272]' '[\106-\377]'
# does not translate char ']' So we use our own program to do this
# - dp
TR69		= $(OBJDIR)/tr69

TARGETS		= $(TR69)
TARGETS		+= $(HSRCS:.html=.h) $(MSRCS:.msg=.h) splash.h splash-java.h

US_SPLASH_IS_DIFFERENT	= TRUE

include $(L10NDEPTH)/config/rules.mk

ifndef NO_EDITOR
ifdef BUILD_EDITOR
ifdef BUILD_EDITOR_UI
TARGETS		+= about-gold.h about-gold-java.h splash-gold.h splash-gold-java.h
endif
endif
endif

GARBAGE		+= splash.html

.SUFFIXES:	.html .msg

#
# Welcome to sHELL....
#

$(TR69):	tr69.c
	$(CC) -o $@ $<

about.h:	about-all.html
	@echo generating $@ from $<;					\
	NAME=Netscape;							\
	CLASS=Netscape;							\
	RTMP=/tmp/nsrot$$$$;						\
	rm -f $@ $*.pc;							\
	sed    "s/@NAME@/$$NAME/g;					\
		s/@CLASS@/$$CLASS/g;					\
		s/@PROGNAME@/$(PROGNAME)/g" $< |			\
	awk    'BEGIN { skipline = 0 }					\
		/about:javalogo/,/<\/TR>/ { skipline = 1; next }	\
		skipline == 1 && $$1 !~ /<\/TR>/ { next }		\
		skipline == 1 && $$1 ~ /<\/TR>/ { skipline = 0; next }	\
		/Mercutio/ { next }					\
		/ViviStar/ { next } { print }' |			\
	tee $$RTMP |							\
	awk    'BEGIN { skipline = 0 }					\
		/about:qtlogo/,/<\/TR>/ { skipline = 1; next }		\
		skipline == 1 && $$1 !~ /<\/TR>/ { next }		\
		skipline == 1 && $$1 ~ /<\/TR>/ { skipline = 0; next }	\
		{ print }' |						\
	tee splash.input |						\
	tr '[\001-\272]' '[\106-\377]' |				\
	od -b |								\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;				\
		s/ /\\/g;						\
		s/\(.*\)/ "\1"/;					\
		s/^ *"\\"$$//' >> $@;					\
	echo generating $*.pc from $<;					\
	$(EMACS) -batch -q -l $(REAL_XPDIR)/html-to-pc.el			\
		-f batch-html-to-pc $* $$RTMP $*.pc;			\
	echo;								\
	$(RM) $$RTMP

about-gold.h:	about-all.html
	@echo generating $@ from $<;					\
	NAME=Netscape;							\
	CLASS=Netscape;							\
	RTMP=/tmp/nsrot$$$$;						\
	rm -f $@;							\
	awk    '/Navigator<SUP>/ { printf("%s Gold\n", $$0); next }	\
		{ print }' $< |						\
	sed    "s/@NAME@/$$NAME/g;					\
		s/@CLASS@/$$CLASS/g;					\
		s/@PROGNAME@/$(PROGNAME)/g" |				\
	awk    'BEGIN { skipline = 0 }					\
		/about:javalogo/,/<\/TR>/ { skipline = 1; next }	\
		skipline == 1 && $$1 !~ /<\/TR>/ { next }		\
		skipline == 1 && $$1 ~ /<\/TR>/ { skipline = 0; next }	\
		/Mercutio/ { next }					\
		/ViviStar/ { next } { print }' |			\
	tee $$RTMP |							\
	awk    'BEGIN { skipline = 0 }					\
		/about:qtlogo/,/<\/TR>/ { skipline = 1; next }		\
		skipline == 1 && $$1 !~ /<\/TR>/ { next }		\
		skipline == 1 && $$1 ~ /<\/TR>/ { skipline = 0; next }	\
		{ print }' |						\
	tr '[\001-\272]' '[\106-\377]' |				\
	od -b |								\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;				\
		s/ /\\/g;						\
		s/\(.*\)/ "\1"/;					\
		s/^ *"\\"$$//' >> $@;					\
	echo generating $*.pc from $<;					\
	$(EMACS) -batch -q -l $(REAL_XPDIR)/html-to-pc.el			\
		-f batch-html-to-pc $* $$RTMP $*.pc;			\
	echo;								\
	$(RM) $$RTMP

about-java.h:	about-all.html
	@echo generating $@ from $<;					\
	NAME=Netscape;							\
	CLASS=Netscape;							\
	RTMP=/tmp/nsrot$$$$;						\
	rm -f $@ $*.pc;							\
	sed    "s/@NAME@/$$NAME/g;					\
		s/@CLASS@/$$CLASS/g;					\
		s/@PROGNAME@/$(PROGNAME)/g" $< |			\
	awk    'BEGIN { skipline = 0 }					\
		/Mercutio/ { next }					\
		/ViviStar/ { next } { print }' |			\
	tee $$RTMP |							\
	awk    'BEGIN { skipline = 0 }					\
		/about:qtlogo/,/<\/TR>/ { skipline = 1; next }		\
		skipline == 1 && $$1 !~ /<\/TR>/ { next }		\
		skipline == 1 && $$1 ~ /<\/TR>/ { skipline = 0; next }	\
		{ print }' |						\
	tee splash-java.input |						\
	tr '[\001-\272]' '[\106-\377]' |				\
	od -b |								\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;				\
		s/ /\\/g;						\
		s/\(.*\)/ "\1"/;					\
		s/^ *"\\"$$//' >> $@;					\
	echo generating $*.pc from $<;					\
	$(EMACS) -batch -q -l $(REAL_XPDIR)/html-to-pc.el			\
		-f batch-html-to-pc $* $$RTMP $*.pc;			\
	echo;								\
	$(RM) $$RTMP

about-gold-java.h:	about-all.html
	@echo generating $@ from $<;					\
	NAME=Netscape;							\
	CLASS=Netscape;							\
	RTMP=/tmp/nsrot$$$$;						\
	rm -f $@;							\
	awk    '/Navigator<SUP>/ { printf("%s Gold\n", $$0); next }	\
		{ print }' $< |						\
	sed    "s/@NAME@/$$NAME/g;					\
		s/@CLASS@/$$CLASS/g;					\
		s/@PROGNAME@/$(PROGNAME)/g" |				\
	awk    'BEGIN { skipline = 0 }					\
		/Mercutio/ { next }					\
		/ViviStar/ { next } { print }' |			\
	tee $$RTMP |							\
	awk    'BEGIN { skipline = 0 }					\
		/about:qtlogo/,/<\/TR>/ { skipline = 1; next }		\
		skipline == 1 && $$1 !~ /<\/TR>/ { next }		\
		skipline == 1 && $$1 ~ /<\/TR>/ { skipline = 0; next }	\
		{ print }' |						\
	tr '[\001-\272]' '[\106-\377]' |				\
	od -b |								\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;				\
		s/ /\\/g;						\
		s/\(.*\)/ "\1"/;					\
		s/^ *"\\"$$//' >> $@;					\
	echo generating $*.pc from $<;					\
	$(EMACS) -batch -q -l $(REAL_XPDIR)/html-to-pc.el			\
		-f batch-html-to-pc $* $$RTMP $*.pc;			\
	echo;								\
	$(RM) $$RTMP

about-all.h:	about-all.html
	@echo "Garbage file" > $@

splash.input:		about.h
splash-java.input:	about-java.h

splash.html:		splash.input
	@echo generating $@ from $?...
	@sed 's|\(.*\)<A [^>]*>\(.*\)|\1\2|g;s|</A>||g' < $? > $@

splash-java.html:	splash-java.input
	@echo generating $@ from $?...
	@sed 's|\(.*\)<A [^>]*>\(.*\)|\1\2|g;s|</A>||g' < $? > $@

splash-gold.h:		splash.html
	@echo generating $@ from $<;			\
	rm -f $@;					\
	awk    '/Navigator<SUP>/ { printf("%s Gold\n", $$0); next }	\
		{ print }' $< |						\
	tr '[\001-\272]' '[\106-\377]' |		\
	od -b |						\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;		\
		s/ /\\/g;				\
		s/\(.*\)/ "\1"/;			\
		s/^ *"\\"$$//' >> $@;			\
	echo;

splash-gold-java.h:	splash-java.html
	@echo generating $@ from $<;			\
	rm -f $@;					\
	awk    '/Navigator<SUP>/ { printf("%s Gold\n", $$0); next }	\
		{ print }' $< |						\
	tr '[\001-\272]' '[\106-\377]' |		\
	od -b |						\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;		\
		s/ /\\/g;				\
		s/\(.*\)/ "\1"/;			\
		s/^ *"\\"$$//' >> $@;			\
	echo;

#
# Handle the rest of the HTML files.
#
.html.h:
	@echo generating $*.h from $<;			\
	NAME=Netscape;					\
	CLASS=Netscape;					\
	RTMP=/tmp/nsrot$$$$;				\
	rm -f $*.h $*.pc;				\
	sed    "s/@NAME@/$$NAME/g;			\
		s/@CLASS@/$$CLASS/g;			\
		s/@PROGNAME@/$(PROGNAME)/g"		\
		< $< > $$RTMP;				\
	$(TR69) < $$RTMP |				\
	od -b |						\
	sed    's/^[0-7][0-7]* *\(.*\)/\\\1/;		\
		s/ /\\/g;				\
		s/\(.*\)/ "\1"/;			\
		s/^ *"\\"$$//' >> $*.h;			\
	echo generating $*.pc from $<;			\
	$(EMACS) -batch -q -l $(REAL_XPDIR)/html-to-pc.el	\
		-f batch-html-to-pc $* $$RTMP $*.pc;	\
	echo;						\
	$(RM) $$RTMP

.msg.h:
	@echo generating $*.h from $<;			\
	NAME=Netscape; CLASS=Netscape;			\
	rm -f $*.h;					\
	RTMP=/tmp/nsrot$$$$;				\
	sed    "s/@NAME@/$$NAME/g;			\
		s/@CLASS@/$$CLASS/g;			\
		s/@PROGNAME@/$(PROGNAME)/g"		\
		< $< > $$RTMP;				\
	tr '[\001-\272]' '[\106-\377]' < $$RTMP |	\
	od -b | sed 's/^[0-7][0-7]* *\(.*\)/\\\1/;	\
		s/ /\\/g;s/\(.*\)/ "\1"/;s/^ *"\\"$$//'	\
	  >> $*.h;					\
	$(RM) $$RTMP

$(TARGETS):	Makefile

export::	$(TARGETS)
