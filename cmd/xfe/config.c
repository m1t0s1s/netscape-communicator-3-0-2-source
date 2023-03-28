/* -*- Mode: C; tab-width: 8 -*-
   config.c --- link-time configuration of the X front end.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 26-Feb-95.

   Parameters:

    - whether this is the "net" version or the "for sale" version
      this is done by changing the value of `fe_version' and `fe_version_short'
      based on -DVERSION=1.1N
    - whether this is linked with DNS or NIS for name resolution
      this is done by changing the value of `fe_HaveDNS'
      based on optional -DHAVE_NIS
    - which animation to use
      this is done by linking in a particular version of icondata.o
      along with optional -DVENDOR_ANIM
 */

#include "name.h"

const char fe_BuildConfiguration[] = cpp_stringify(CONFIG);

const char fe_version[] = cpp_stringify(VERSION);
const char fe_long_version[] =
 "@(#)" XFE_NAME_STRING

#if defined(EDITOR) && defined(EDITOR_UI)
 " Gold "
#else
 " "
#endif

 cpp_stringify(VERSION)

 "/SMIME"

#ifdef HAVE_NIS
 "/NIS"
#endif

#ifdef DEBUG
 "/DEBUG"
#endif /* DEBUG */

 ", " cpp_stringify(DATE) "; " XFE_LEGALESE
;

/*
 For example:

 Netscape 3.02, 26-Feb-95; (c) 1995 Netscape Communications Corp.
 Netscape 3.02S, 26-Feb-95; (c) 1995 Netscape Communications Corp.
 Netscape 3.02/NIS, 26-Feb-95; (c) 1995 Netscape Communications Corp.
 Netscape 3.02/NIS/DEBUG, 26-Feb-95; (c) 1995 Netscape Communications Corp.
*/

/* initially set to plain version, without locale string */
char *fe_version_and_locale = (char *) fe_version;


#ifdef HAVE_NIS
int fe_HaveDNS = 0;
#else
int fe_HaveDNS = 1;
#endif

#ifdef VENDOR_ANIM
int fe_VendorAnim = 1;
#else
int fe_VendorAnim = 0;
#endif
