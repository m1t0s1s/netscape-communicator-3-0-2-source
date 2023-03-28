/*
 * xp_locale.h
 * -----------
 * Defines some cross platform functions so that
 * we can consistently have localized sorting rules
 * and date string formats across platforms.
 * Comments need some serious expanding - see
 * http://warp/engr/client/Cheddar/xp_locale_API.html
 * for now. 
 */
 
 
#include <string.h>
#include <time.h>
 
#include "xp_core.h"
#include "ntypes.h"
#ifndef __XPLOCALE__
#define __XPLOCALE__

XP_BEGIN_PROTOS

/* We keep the old enum here so it won't break unix and window build 
   before they implement new XP_Strxfrm(). Tony and Erik: please delete 
   the following ifdef after you check-in your implementation.
*/

enum {
	XP_TIME_FORMAT = 0,
	XP_WEEKDAY_TIME_FORMAT = 1,
	XP_DATE_TIME_FORMAT = 2,
	XP_LONG_DATE_TIME_FORMAT = 3
};

/* XP_StrColl
 * Takes two strings to compare, compares them, 
 * and returns a number less than 0 if the second
 * string is greater, 0 if they are the same, 
 * and greater than 0 if the first string is 
 * greater, according to the sorting rules 
 * appropriate for the current locale.
 */ 
 
int XP_StrColl(const char* s1, const char* s2);

extern int FE_StrColl(const char* s1, const char* s2);

/* XP_Strftime
 * Takes a format string which works according to the rules
 * of the UNIX strftime, and a struct tm * which has all
 * the relevant date info, and puts the appropriate (localized)
 * string to indicate the time according to the format into the
 * result buffer. Returns the length of the result (in characters).
 */
 
size_t XP_StrfTime(MWContext* context, char *result, size_t maxsize, int format,
    const struct tm *timeptr);

extern size_t FE_StrfTime(MWContext* context,  char *result, size_t maxsize, int format,
    const struct tm *timeptr);

/* XP_Strxfrm
 * See 'man strxfrm.' ONLY WORKS ON UNIX AND WIN 32. Returns
 * (size_t) -1 on error. Calling it on a mac or in 16 bit 
 * windows counts as an error.
 */
 
 size_t XP_Strxfrm ( char *s1, const  char *s2, size_t n);
 
/*	INTL_ctime is a wrapper of XP_Strxfrm which accept time_t as parameter,
    it always use XP_DATETIMEFORMAT with XP_Strxfrm */
/*  Rename from XP_ctime to INTL_ctime 				*/
const char* INTL_ctime(MWContext* context, time_t *date);

XP_END_PROTOS

#endif
