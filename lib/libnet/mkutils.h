/* -*- Mode: C; tab-width: 4 -*- */
#ifndef MKUTILS_H
#define MKUTILS_H

/* use the fancy new mail reading stuff
 */
#undef NEW_MAIL_STUFF
 
#include "net.h"
#include "xp.h"                 /* The cross-platform API */

#define CONST const             /* "const" only exists in STDC? */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef XP_UNIX
#include <unistd.h>
#include <signal.h>
#endif /* XP_UNIX */
#include "version.h"

#if defined(__sun)
# include <nspr/sunos4.h>
#endif /* __sun */

extern int NET_URL_Type (CONST char *URL);
extern void del_front_spaces (char *string);
extern void NET_f_a_c (char **obj);
#define FREE_AND_CLEAR(x) NET_f_a_c(&x) 

/*
 * This function takes an error code and associated error data
 * and creates a string containing a textual description of
 * what the error is and why it happened.
 *
 * The returned string is allocated and thus should be freed
 * once it has been used.
 *
 * This function is defined in mkmessag.c.
 */
extern char * NET_ExplainErrorDetails (int code, ...);

/* Find an Author's mail address
 *
 * THE EMAIL ADDRESS IS CORRUPTED
 *
 * For example, returns "montulli@netscape.com" if given any of
 *      " Lou Montullism <montulli@netscape.com> "
 *  or  " montulli@netscape.com ( Lou "The Stud :)" Montulli ) "
 *  or  "montulli@netscape.com"
*/
extern char * NET_EmailAddress (char *email);

/* this function is called repeatably to write
 * the post data body onto the network.
 *
 * Returns negative on fatal error
 * Returns zero when done
 * Returns positive if not yet completed
 */
extern int
NET_WritePostData(MWContext  *context,
				  URL_Struct *URL_s,
                  NETSOCK     sock,
                  void      **write_post_data_data,
                  XP_Bool     add_crlf_to_line_endings);




/* semaphore, if non zero then "call me all the time" is enabled
 *
 * defined in mkcache.c
 */
extern int net_call_all_the_time_count;

extern char * NET_SpaceToPlus(char * string);

/* returns true if the functions thinks the string contains
 * HTML
 */
extern Bool NET_ContainsHTML(char * string, int32 length);

/* try to make sure that this is a fully qualified
 * email address including a host and domain
 */
extern Bool NET_IsFQDNMailAddress(const char * string);

extern int NET_UUEncode(unsigned char *src, unsigned char *dst, int srclen);

extern void NET_Progress(MWContext *context, char *msg);

#define FREE(obj)    XP_FREE(obj)
#define FREEIF(obj)  {if(obj) {XP_FREE(obj); obj=0;}}

/* TRACE routines */
extern int MKLib_trace_flag;

#ifdef TRACEMSG
#undef TRACEMSG
#endif
#ifdef DEBUG
#ifdef JAVA
extern void _MK_TraceMsg(char*, ...);
#define TRACEMSG(msg)  _MK_TraceMsg msg
#else
#define TRACEMSG(msg)  do {if(MKLib_trace_flag>1)  XP_Trace msg; } while (0)
#endif
#else
#define TRACEMSG(msg)  
#endif /* DEBUG */

#endif /* MKUTILS_H */


