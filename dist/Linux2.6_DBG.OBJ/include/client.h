
/*
 * This file should be included after xp_mcom.h
 *
 * All definitions for intermodule communications in the Netscape
 * client should be contained in this file
 */

#ifndef _CLIENT_H_
#define _CLIENT_H_

#define NEW_FE_CONTEXT_FUNCS

/* include header files needed for prototypes/etc */

#include "xp_mcom.h"

#include "ntypes.h" /* typedefs for commonly used Netscape data structures */
#include "fe_proto.h" /* all the standard FE functions */
#include "proto.h" /* library functions */

/* global data structures */
#include "structs.h"
#include "merrors.h"

#ifndef XP_MAC /* don't include everything in the world */

/* --------------------------------------------------------------------- */
/* include other bits of the Netscape client library */
#include "lo_ele.h"  /* Layout structures */
#include "net.h"
#include "il.h"
#include "gui.h"
#include "shist.h"
#include "hotlist.h"
#include "glhist.h"
#include "mime.h"

#endif /* !XP_MAC */

#endif /* _CLIENT_H_ */

