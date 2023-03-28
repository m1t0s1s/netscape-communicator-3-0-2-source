DEPTH		= $(L10NDEPTH)/..

XFEDIR		= $(DEPTH)/cmd/xfe
XFE_VERSION	= $(XFEDIR)/versionn.h
APP_DEFAULT	= $(OBJDIR)/$(XFE_PROGCLASS)-$(SUFFIX).ad

XFE_NAME	= netscape
XFE_PROGNAME	= netscape
XFE_PROGCLASS	= Netscape

include $(DEPTH)/config/rules.mk

NOSUCHFILE	= $(OBJDIR)/strdat \
		  $(OBJDIR)/strs \
		  $(OBJDIR)/install \
		  $(OBJDIR)/uninstall \
		  $(OBJDIR)/icondata.c \
		  $(OBJDIR)/g2x \
		  $(OBJDIR)/data.c \
		  $(OBJDIR)/$(OS_CONFIG).tar.gz \
		  $(OBJDIR)/$(OS_CONFIG).tar.Z \
		  $(OBJDIR)/icons/*.xpm

LOC_ICONS	= $(wildcard icons/[AC-Z]*.gif icons/B*.gif icons/app.gif)

INCLUDES	+= -I$(XFEDIR)

GUESS_CONFIG	:= $(shell $(DEPTH)/config/config.guess | sed 's/i[2345]86/x86/')
VERSNAME	:= $(shell $(L10NDEPTH)/tools/get_version $(DEPTH)/cmd/xfe/versionn.h)
TARFILE		:= $(PROGNAME)-$(LOC_LANG_DIRNAME)-v$(VERSNAME)-$(SUFFIX).$(GUESS_CONFIG).tar

$(APP_DEFAULT): $(OBJDIR)/.md resources $(LOCALE_MAP) $(OBJDIR)/strs $(XFE_VERSION)
	@$(MAKE_OBJDIR)
	@cp ../../tools/g2x.c .;						\
	cp ../../tools/g2x.h .;							\
	cp ../../../cmd/xfe/icondata.h .;					\
	echo generating $@ from resources...;					\
	NAME=`echo $(XFE_PROGCLASS)`;						\
	VN=`sed -n 's/^#define VERSION_NUMBER *\(.*\)$$/\1/p' $(XFE_VERSION)`;	\
	VERS=`echo $${VN}$(VSUFFIX)`;						\
	rm -f $@;								\
	cat resources $(LOCAL_FONTS) urls/locurls $(LOCALE_MAP) $(OBJDIR)/strs |	\
	sed "s/@NAME@/$$NAME/g;							\
	     s/@CLASS@/$(XFE_PROGCLASS)/g;					\
	     s/@PROGNAME@/$(XFE_PROGNAME)/g;					\
	     s/@VERSION@/$$VERS/g;						\
	     s/@LOC_LANG@/$(LOC_LANG)/g;					\
	     s/@LOC@//g;							\
	     s/@LTD@//g;							\
	     s:@LIBDIR@:$(LOC_LIB_DIR):g;					\
	     s/@MAIL_IM_HACK@/$(MAIL_IM_HACK)/g;				\
	     s/@NEWS_IM_HACK@/$(NEWS_IM_HACK)/g;				\
	     s/@URLVERSION@/$$VERS/g" > $@

$(OBJDIR)/strs:	$(OBJDIR)/strdat
	$(OBJDIR)/strdat > $(OBJDIR)/strs

$(OBJDIR)/resources-$(SUFFIX).o: $(APP_DEFAULT)
	@$(MAKE_OBJDIR)
	@echo generate $@ from $(APP_DEFAULT);					\
	TMP=/tmp/nsres$$$$.c;							\
	rm -f $$TMP $@;								\
	echo 'char *fe_fallbackResources[] = {' > $$TMP;			\
	/bin/sh ./ad2c $(APP_DEFAULT) >> $$TMP;					\
	echo '0};' >> $$TMP;							\
	$(CC) -c $(CFLAGS) $$TMP;						\
	mv nsres$$$$.o $@;							\
	rm -f $$TMP

#
# This is a dummy target to make sure OBJDIR gets created.
# MAKE_OBJDIR is a define that expects $@ to be of the form
# dir/file.  I think we should use -mkdir $(OBJDIR), but
# what do I know. --briano.
#
$(OBJDIR)/.md:
	@$(MAKE_OBJDIR)

$(OBJDIR)/strdat:
	@cat ../../../cmd/xfe/strdat.c | sed "s/<allxpstr.h>/\"..\/xp\/allxpstr.h\"/g;\
        s/\"xfe_err.h\"/\".\/xfe_err.h\"/g; s/mcom_cmd_xfe_xfe_err_h_strings();/mcom_cmd_xfe_xfe_err_h_strings();\
	return 0;/g" > strdat.c
	$(CC) -o $@ $(CFLAGS) strdat.c

$(OBJDIR)/install: 
	@echo generating $@
	@echo "#!/bin/sh" >  $@
	@echo "if test ! -d $(LOC_LIB_DIR)/$(LOC_LANG)/app-defaults" >> $@
	@echo "then" >> $@
	@echo "mkdir -p $(LOC_LIB_DIR)/$(LOC_LANG)/app-defaults" >> $@
	@echo "fi" >> $@
	@echo "cp $(XFE_PROGCLASS).ad-$(LOC_LANG_DIRNAME) $(LOC_LIB_DIR)/$(LOC_LANG)/app-defaults/$(XFE_PROGCLASS)" >> $@
	@echo "if test ! -d $(LOC_LIB_DIR)/$(LOC_LANG)/$(XFE_PROGNAME)" >> $@
	@echo "then" >> $@
	@echo "mkdir -p $(LOC_LIB_DIR)/$(LOC_LANG)/$(XFE_PROGNAME)" >> $@
	@echo "fi" >> $@
	@echo "cp about splash license mail.msg icons/*.xpm $(LOC_LIB_DIR)/$(LOC_LANG)/$(XFE_PROGNAME)" >> $@
	@echo "exit 0" >> $@
	@chmod +x $@

$(OBJDIR)/uninstall: 
	@echo generating $@
	@echo "#!/bin/sh" > $@
	@echo "rm -f $(LOC_LIB_DIR)/$(LOC_LANG)/app-defaults/$(XFE_PROGCLASS)" >> $@
	@echo "rm -rf $(LOC_LIB_DIR)/$(LOC_LANG)/$(XFE_PROGNAME)" >> $@
	@echo "exit 0" >> $@
	@chmod +x $@

XPMS: $(OBJDIR)/g2x
	@echo generating xpms..
	@if test ! -d $(OBJDIR)/icons; then mkdir -p $(OBJDIR)/icons; fi
	$(OBJDIR)/g2x $(OBJDIR)/icons/

$(OBJDIR)/g2x: g2x.c $(OBJDIR)/icondata.c $(OBJDIR)/data.c
	cd $(OBJDIR);$(CC) -I.. -o g2x icondata.c data.c ../g2x.c;cd ..

$(OBJDIR)/icondata.c: ../../../cmd/xfe/$(OBJDIR)/mkicons $(LOC_ICONS)
	../../../cmd/xfe/$(OBJDIR)/mkicons $(LOC_ICONS) > $@

../../../cmd/xfe/$(OBJDIR)/mkicons:
	@if test ! -d ../../../cmd/xfe/$(OBJDIR); then mkdir -p ../../../cmd/xfe/$(OBJDIR); fi
	@cd ../../../cmd/xfe; gmake $(OBJDIR)/mkicons

$(OBJDIR)/data.c: ../../tools/$(OBJDIR)/help $(OBJDIR)/icondata.c
	../../tools/$(OBJDIR)/help  < $(OBJDIR)/icondata.c > $@
		
$(OBJDIR)/$(TARFILE):	$(OBJDIR)/install $(OBJDIR)/uninstall
	@rm -rf $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)
	@mkdir -p $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)
	@cp $(OBJDIR)/$(XFE_PROGCLASS)-$(SUFFIX).ad $(OBJDIR)/$(XFE_PROGCLASS).ad
	@cp $(OBJDIR)/$(XFE_PROGCLASS)-$(SUFFIX).ad $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/$(XFE_PROGCLASS).ad-$(LOC_LANG_DIRNAME)
	@cp $(OBJDIR)/install $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)
	@cp $(OBJDIR)/uninstall $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)
	@cp README $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)
	@cp ../xp/LICENSE-export $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/license
	@cp ../xp/about-java.html $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/about
	@cp ../xp/splash-java.html $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/splash
	@cp ../xp/mail.msg $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)
	@mkdir -p  $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/icons
	@cp $(OBJDIR)/icons/*.xpm  $(OBJDIR)/$(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/icons
	@echo generating $@.gz ...
	@cd $(OBJDIR); tar -cf $(TARFILE) $(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/*; gzip -f $(TARFILE)
	@echo generating $@.Z ...
	@cd $(OBJDIR); tar -cf $(TARFILE) $(XFE_PROGNAME)-$(LOC_LANG_DIRNAME)/*; compress -f $(TARFILE)

