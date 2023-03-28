#! gmake
# 
#           CONFIDENTIAL AND PROPRIETARY SOURCE CODE OF
#              NETSCAPE COMMUNICATIONS CORPORATION
# Copyright (C) 1996 Netscape Communications Corporation.  All Rights
# Reserved.  Use of this Source Code is subject to the terms of the
# applicable license agreement from Netscape Communications Corporation.
# The copyright notice(s) in this Source Code does not indicate actual or
# intended publication of this Source Code.
#

include manifest.mn
include $(DEPTH)/security/secrules.mk


# Some kludgy stuff to automate the updating of allxpstr.h when we've 
# changed the Security Advisor HTML in secprefs.html...

ifneq (,$(findstring AKBAR,$(DEFINES)))
 AKBAR=1
else
 AKBAR=0
endif

secprefs.h secprefs.rc allxpstr.ins: secprefs.html
	@AKBAR=${AKBAR} sh convhtml.sh

all export:: $(DEPTH)/include/allxpstr.h

$(DEPTH)/include/allxpstr.h:: allxpstr.ins
	@(set -e ;	 					\
	  echo inserting $< into $@... ;			\
	  TMP=/tmp/allxpstr.$$$$ ;				\
	  $(RM) $$TMP ;						\
	  awk '/-\*-/,/ResDef\(SA_STRINGS_BEGIN/' < $@		\
	  | sed '/ResDef(SA_STRINGS_BEGIN/d'			\
	  > $$TMP ;						\
								\
	  cat $< >> $$TMP ;					\
								\
	  awk '/ResDef\(SA_VARNAMES_END/,/_bite_me_/' < $@	\
	  | tail +2						\
	  >> $$TMP ;						\
								\
	  mv $@ $@~ ;						\
	  mv $$TMP $@ ;						\
								\
	  echo 'rebuiling libxp.a...' ;				\
	  ( cd $(DEPTH)/lib/xp; $(MAKE) libxp.a );		\
	  )
