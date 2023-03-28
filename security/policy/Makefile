#! gmake
# 
#           CONFIDENTIAL AND PROPRIETARY SOURCE CODE OF
#              NETSCAPE COMMUNICATIONS CORPORATION
# Copyright (C) 1997 Netscape Communications Corporation.  All Rights
# Reserved.  Use of this Source Code is subject to the terms of the
# applicable license agreement from Netscape Communications Corporation.
# The copyright notice(s) in this Source Code does not indicate actual or
# intended publication of this Source Code.
#

include manifest.mn
include $(DEPTH)/security/secrules.mk

.SUFFIXES: .zip .rsa

# add any new policies to this line
# JARS	= us.zip export.zip france.zip
JARS	= \
	en-export.zip en-france.zip en-us.zip \
	fr-export.zip fr-france.zip fr-us.zip \
	da-export.zip \
	de-export.zip \
	es-export.zip \
	it-export.zip \
	ja-export.zip \
	ko-export.zip \
	nl-export.zip \
	pt-export.zip \
	sv-export.zip \
	$(NULL)


INSTALL_DIR = $(XPDIST)/xpdist
TARGETS = $(addprefix $(INSTALL_DIR)/, $(JARS))
SIGNATURES = $(JARS:.zip=/META-INF/zigbert.rsa)
META_DIRS = $(JARS:.zip=/META-INF)
RAW_META= $(JARS:zip=meta)

ifdef MAKE_POLICY_JARS

NOCVS	= -x CVS
GARBAGE += $(JARS)

#%.zip:	%/META-INF/zigbert.rsa
$(JARS):	$(@:.zip=/META-INF/zigbert.rsa)
	cd $*; $(ZIP) ../$@  META-INF/*.* $(NOCVS); cd ..

endif

$(INSTALL_DIR):
	-mkdir $(INSTALL_DIR)		2>/dev/null || true

#$(TARGETS):: $(JARS) $(INSTALL_DIR)
#	$(INSTALL) $(JARS) $(INSTALL_DIR)
$(XPDIST)/xpdist/%:: $(INSTALL_DIR)

$(XPDIST)/xpdist/%:: %
	$(INSTALL) $< $(INSTALL_DIR)

install:: $(TARGETS) 

# Note, the rmdir command below does not have switches to force a recursive
# delete.  It is intended to fail if any files remain in $(INSTALL_DIR)
clobber_all::
	-rmdir $(INSTALL_DIR) 		2>/dev/null || true

# steps to manually run zigbert to sign policy files
ifdef RUN_ZIGBERT

ZIGDIR	= /u/tdell
ZIGBERT	= $(ZIGDIR)/zigbert
NICKNAME= SOBSigner
KEYDB_DIR=$(HOME)/tmp

# rule for generating new signatures
#$(SIGNATURES):: $(@:/META-INF/zigbert.rsa=.meta)
%/META-INF/zigbert.rsa:: %.meta
	-mkdir tmp			2> /dev/null || true
	$(ZIGBERT) -k$(NICKNAME) -d$(KEYDB_DIR) -m"$<" tmp
	-mkdir $(<:.meta=)		2> /dev/null || true
	-mkdir $(<:.meta=/META-INF)	2> /dev/null || true
	cp tmp/META-INF/*.* $(@:/zigbert.rsa=)
	-rm -rf tmp

signatures: $(SIGNATURES)

endif

targets: $(TARGETS)

jars:	$(JARS)

showsymbols::
	@echo "INSTALL_DIR = $(INSTALL_DIR)"
	@echo "JARS        = $(JARS)"
	@echo "TARGETS     = $(TARGETS)"
	@echo "SIGNATURES  = $(SIGNATURES)"
	@echo "RAW_META    = $(RAW_META)"
	@echo "META_DIRS   = $(META_DIRS)"
	@echo "DEPENDENCIES= $(DEPENDENCIES)"
	@echo "OBJS	= $(OBJS)"
	@echo "OBJDIR	= $(OBJDIR)"
	@echo "GARBAGE	= $(GARBAGE)"
	@echo "NOSUCHFILE = $(NOSUCHFILE)"

