/*
 * @(#)date.c	1.16 95/11/29 Arthur van Hoff
 * 
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for NON-COMMERCIAL purposes and without fee is hereby
 * granted provided that this copyright notice appears in all copies. Please
 * refer to the file "copyright.html" for further important copyright and
 * licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

#include <time.h>

#include "oobj.h"
#include "interpreter.h"
#include "sys_api.h"
#include "javaString.h"

#include "java_util_Date.h"

/* tm members are defined to be "natural ints", but our java_util_Date uses
 * longs, so we have a problem in 16-bit environments. This procedure centralizes
 * the conversion from a java_util_Date to a tm. Notice that it is not exported.
 */
static void
java_utilDate_toPlatformTM(Classjava_util_Date * thisptr, struct tm *tm)
{
    tm->tm_sec = (int) thisptr->tm_sec;
    tm->tm_min = (int) thisptr->tm_min;
    tm->tm_hour = (int) thisptr->tm_hour;
    tm->tm_mday = (int) thisptr->tm_mday;
    tm->tm_wday = (int) thisptr->tm_wday;
    tm->tm_yday = (int) thisptr->tm_yday;
    tm->tm_mon = (int) thisptr->tm_mon;
    tm->tm_year = (int) thisptr->tm_year;
    tm->tm_isdst = (int) thisptr->tm_isdst;
}


void
java_util_Date_expand(struct Hjava_util_Date * this)
{
    struct tm tm;
    Classjava_util_Date *thisptr = unhand(this);
    int64_t secs = ll_div(thisptr->value, int2ll(1000));
    time_t t = ll2int(secs);
    if (t < 0 || ll_ne(secs, int2ll(t))) {
	/* Bogus 32 bit times couldn't represent this value */
	SignalError(0, JAVAPKG "IllegalArgumentException",
		    "time out of range for timezone calculation.");
	return;
    }
    (void) sysLocaltime(&t, &tm);
    thisptr->tm_millis = ll2int(ll_rem(thisptr->value, int2ll(1000)));
    thisptr->tm_sec = tm.tm_sec;
    thisptr->tm_min = tm.tm_min;
    thisptr->tm_hour = tm.tm_hour;
    thisptr->tm_mday = tm.tm_mday;
    thisptr->tm_mon = tm.tm_mon;
    thisptr->tm_wday = tm.tm_wday;
    thisptr->tm_yday = tm.tm_yday;
    thisptr->tm_year = tm.tm_year;
    thisptr->tm_isdst = tm.tm_isdst;
    thisptr->expanded = TRUE;
}

void
java_util_Date_computeValue(struct Hjava_util_Date * this)
{
    struct tm tm;
    Classjava_util_Date *thisptr = unhand(this);
	java_utilDate_toPlatformTM(thisptr, &tm);
    tm.tm_isdst = -1;
    if (tm.tm_year < 70 || tm.tm_year > 137) {
	/* Bogus 32 bit times overflow in 2037 */
	SignalError(0, JAVAPKG "IllegalArgumentException",
		    "year out of range.");
    } else {
	thisptr->value = ll_mul(int2ll(sysMktime(&tm)), int2ll(1000));
	thisptr->valueValid = TRUE;
    }
}

/*
 * Convert the date to a string
 */
HString *
java_util_Date_toString(struct Hjava_util_Date * this)
{
    struct tm tm;
    Classjava_util_Date *thisptr = unhand(this);
    char buf[100];
    if (!thisptr->expanded) {
	java_util_Date_expand(this);
	if (!thisptr->expanded)
	    return 0;
    }
	java_utilDate_toPlatformTM(thisptr, &tm);
    sysStrftime(buf, sizeof buf, "%a %b %d %H:%M:%S %Z %Y", &tm);
    return makeJavaString(buf, strlen(buf));
}

/*
 * Convert the date to a string in local-specific form.
 */
HString *
java_util_Date_toLocaleString(struct Hjava_util_Date * this)
{
    struct tm tm;
    Classjava_util_Date *thisptr = unhand(this);
    char buf[100];
    if (!thisptr->expanded) {
	java_util_Date_expand(this);
	if (!thisptr->expanded)
	    return 0;
    }
	java_utilDate_toPlatformTM(thisptr, &tm);
    sysStrftime(buf, sizeof buf, "%c", &tm);
    return makeJavaString(buf, strlen(buf));
}

static char *englishMonths[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/*
 * Convert the date to a string in GMT form, which should be universally
 * parse-able.  We explicitly don't use sysStrftime here because the date
 * string MUST NOT be localized.
 */
HString *
java_util_Date_toGMTString(struct Hjava_util_Date * this)
{
    struct tm tm;
    Classjava_util_Date *thisptr = unhand(this);
    char buf[100];
    if (!thisptr->valueValid) {
	java_util_Date_computeValue(this);
	return 0;
    } else {
	int64_t secs = ll_div(thisptr->value, int2ll(1000));
	time_t t = ll2int(secs);
	if (t < 0 || !ll_eq(secs, int2ll(t))) {
	    /* Bogus 32 bit times couldn't represent this value */
	    SignalError(0, JAVAPKG "IllegalArgumentException",
			"time out of range for timezone calculation.");
	    return 0;
	}
	(void) sysGmtime(&t, &tm);
	jio_snprintf(buf, sizeof buf, "%d %s %04d %02d:%02d:%02d GMT",
		     tm.tm_mday, englishMonths[tm.tm_mon], 1900 + tm.tm_year,
		     tm.tm_hour, tm.tm_min, tm.tm_sec);
	return makeJavaString(buf, strlen(buf));
    }
}
