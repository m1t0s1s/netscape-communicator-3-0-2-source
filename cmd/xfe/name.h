/* -*- Mode: C; tab-width: 8 -*-
   name.h --- what are we calling it today.
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 3-Feb-95.
 */

#define XFE_NAME      Netscape
#define XFE_PROGNAME  netscape
#define XFE_PROGCLASS Netscape
#define XFE_LEGALESE "(c) 1997 Netscape Communications Corp."

/* I don't pretend to understand this. */
#define cpp_stringify_noop_helper(x)#x
#define cpp_stringify(x) cpp_stringify_noop_helper(x)

#define XFE_NAME_STRING      cpp_stringify(XFE_NAME)
#define XFE_PROGNAME_STRING  cpp_stringify(XFE_PROGNAME)
#define XFE_PROGCLASS_STRING cpp_stringify(XFE_PROGCLASS)
