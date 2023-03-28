#include "prtypes.h"
#include "prtime.h"
#include "secder.h"
#include "prlong.h"

extern int SEC_ERROR_INVALID_TIME;

#define HIDIGIT(v) (((v) / 10) + '0')
#define LODIGIT(v) (((v) % 10) + '0')

#define C_SINGLE_QUOTE '\047'

#define DIGITHI(dig) (((dig) - '0') * 10)
#define DIGITLO(dig) ((dig) - '0')
#define ISDIGIT(dig) (((dig) >= '0') && ((dig) <= '9'))
#define CAPTURE(var,p,label)				  \
{							  \
    if (!ISDIGIT((p)[0]) || !ISDIGIT((p)[1])) goto label; \
    (var) = ((p)[0] - '0') * 10 + ((p)[1] - '0');	  \
}

#define SECMIN ((time_t) 60L)
#define SECHOUR (60L*SECMIN)
#define SECDAY (24L*SECHOUR)
#define SECYEAR (365L*SECDAY)

static long monthToDayInYear[12] = {
    0,
    31,
    31+28,
    31+28+31,
    31+28+31+30,
    31+28+31+30+31,
    31+28+31+30+31+30,
    31+28+31+30+31+30+31,
    31+28+31+30+31+30+31+31,
    31+28+31+30+31+30+31+31+30,
    31+28+31+30+31+30+31+31+30+31,
    31+28+31+30+31+30+31+31+30+31+30,
};

/* gmttime must contains UTC time in micro-seconds unit */
SECStatus
DER_TimeToUTCTime(SECItem *dst, int64 gmttime)
{
#ifndef NSPR20
    PRTime printableTime;
#else
    PRExplodedTime printableTime;
#endif /* NSPR20 */
    unsigned char *d;

    dst->len = 13;
    dst->data = d = (unsigned char*) PORT_Alloc(13);
    if (!d) {
	return SECFailure;
    }

    /*Convert a int64 time to a printable format. This is a temporary call
	  until we change to NSPR 2.0
     */
#ifndef NSPR20
    PR_ExplodeGMTTime(&printableTime, gmttime);
#else
    PR_ExplodeTime(gmttime, PR_GMTParameters, &printableTime);
#endif /* NSPR20 */

    /* The month in UTC time is base one */
#ifndef NSPR20
    printableTime.tm_mon++;
#else
    printableTime.tm_month++;
#endif /* NSPR20 */

    /* UTC time does not handle the years before 1970 */
    if (printableTime.tm_year < 1970)
	    return SECFailure;
    /* remove the centry since it's added to the tm_year by the 
       PR_ExplodeTime routine, but is not needed for UTC time */
    printableTime.tm_year %= 100; 

    d[0] = HIDIGIT(printableTime.tm_year);
    d[1] = LODIGIT(printableTime.tm_year);
#ifndef NSPR20
    d[2] = HIDIGIT(printableTime.tm_mon);
    d[3] = LODIGIT(printableTime.tm_mon);
#else
    d[2] = HIDIGIT(printableTime.tm_month);
    d[3] = LODIGIT(printableTime.tm_month);
#endif /* NSPR20 */
    d[4] = HIDIGIT(printableTime.tm_mday);
    d[5] = LODIGIT(printableTime.tm_mday);
    d[6] = HIDIGIT(printableTime.tm_hour);
    d[7] = LODIGIT(printableTime.tm_hour);
    d[8] = HIDIGIT(printableTime.tm_min);
    d[9] = LODIGIT(printableTime.tm_min);
    d[10] = HIDIGIT(printableTime.tm_sec);
    d[11] = LODIGIT(printableTime.tm_sec);
    d[12] = 'Z';
    return SECSuccess;
}

SECStatus
DER_AsciiToTime(int64 *dst, char *string)
{
    long year, month, mday, hour, minute, second, hourOff, minOff, days;
    int64 result, tmp1, tmp2;
    
    /* Verify time is formatted properly and capture information */
    second = 0;
    hourOff = 0;
    minOff = 0;
    CAPTURE(year,string+0,loser);
    if (year < 70) {
	/* ASSUME that year # is in the 2000's, not the 1900's */
	year += 100;
    }
    CAPTURE(month,string+2,loser);
    if ((month == 0) || (month > 12)) goto loser;
    CAPTURE(mday,string+4,loser);
    if ((mday == 0) || (mday > 31)) goto loser;
    CAPTURE(hour,string+6,loser);
    if (hour > 23) goto loser;
    CAPTURE(minute,string+8,loser);
    if (minute > 59) goto loser;
    if (ISDIGIT(string[10])) {
	CAPTURE(second,string+10,loser);
	if (second > 59) goto loser;
	string += 2;
    }
    if (string[10] == '+') {
	CAPTURE(hourOff,string+11,loser);
	if (hourOff > 23) goto loser;
	CAPTURE(minOff,string+13,loser);
	if (minOff > 59) goto loser;
    } else if (string[10] == '-') {
	CAPTURE(hourOff,string+11,loser);
	if (hourOff > 23) goto loser;
	hourOff = -hourOff;
	CAPTURE(minOff,string+13,loser);
	if (minOff > 59) goto loser;
	minOff = -minOff;
    } else if (string[10] != 'Z') {
	goto loser;
    }
    
    
    /* Convert pieces back into a single value year  */
    LL_I2L(tmp1, (year-70L));
    LL_I2L(tmp2, SECYEAR);
    LL_MUL(result, tmp1, tmp2);
    
    LL_I2L(tmp1, ( (mday-1L)*SECDAY + hour*SECHOUR + minute*SECMIN -
		  hourOff*SECHOUR - minOff*SECMIN + second ) );
    LL_ADD(result, result, tmp1);

    /*
    ** Have to specially handle the day in the month and the year, to
    ** take into account leap days. The return time value is in
    ** seconds since January 1st, 12:00am 1970, so start examining
    ** the time after that. We can't represent a time before that.
    */

    /* Using two digit years, we can only represent dates from 1970
       to 2069. As a result, we cannot run into the leap year rule
       that states that 1700, 2100, etc. are not leap years (but 2000
       is). In other words, there are no years in the span of time
       that we can represent that are == 0 mod 4 but are not leap
       years. Whew.
       */

    days = monthToDayInYear[month-1];
    days += (year - 68)/4;

    if (((year % 4) == 0) && (month < 3)) {
	days--;
    }
   
    LL_I2L(tmp1, (days * SECDAY) );
    LL_ADD(result, result, tmp1 );

    /* convert to micro seconds */
    LL_I2L(tmp1, PR_USEC_PER_SEC);
    LL_MUL(result, result, tmp1);

    *dst = result;
    return SECSuccess;

  loser:
    PORT_SetError(SEC_ERROR_INVALID_TIME);
    return SECFailure;
	
}

SECStatus
DER_UTCTimeToTime(int64 *dst, SECItem *time)
{
    return DER_AsciiToTime(dst, (char*) time->data);
}

/*
   gmttime must contains UTC time in micro-seconds unit.
   Note: the caller should make sure that Generalized time
   should only be used for certifiate validities after the
   year 2049.  Otherwise, UTC time should be used.  This routine
   does not check this case, since it can be used to encode
   certificate extension, which does not have this restriction. 
 */
SECStatus
DER_TimeToGeneralizedTime(SECItem *dst, int64 gmttime)
{
#ifndef NSPR20
    PRTime printableTime;
#else
    PRExplodedTime printableTime;
#endif /* NSPR20 */
    unsigned char *d;

    dst->len = 15;
    dst->data = d = (unsigned char*) PORT_Alloc(15);
    if (!d) {
	return SECFailure;
    }

    /*Convert a int64 time to a printable format. This is a temporary call
	  until we change to NSPR 2.0
     */
#ifndef NSPR20
    PR_ExplodeGMTTime(&printableTime, gmttime);
#else
    PR_ExplodeTime(gmttime, PR_GMTParameters, &printableTime);
#endif /* NSPR20 */

    /* The month in Generalized time is base one */
#ifndef NSPR20
    printableTime.tm_mon++;
#else
    printableTime.tm_month++;
#endif /* NSPR20 */

    d[0] = (printableTime.tm_year /1000) + '0';
    d[1] = ((printableTime.tm_year % 1000) / 100) + '0';
    d[2] = ((printableTime.tm_year % 100) / 10) + '0';
    d[3] = (printableTime.tm_year % 10) + '0';
#ifndef NSPR20
    d[4] = HIDIGIT(printableTime.tm_mon);
    d[5] = LODIGIT(printableTime.tm_mon);
#else
    d[4] = HIDIGIT(printableTime.tm_month);
    d[5] = LODIGIT(printableTime.tm_month);
#endif /* NSPR20 */
    d[6] = HIDIGIT(printableTime.tm_mday);
    d[7] = LODIGIT(printableTime.tm_mday);
    d[8] = HIDIGIT(printableTime.tm_hour);
    d[9] = LODIGIT(printableTime.tm_hour);
    d[10] = HIDIGIT(printableTime.tm_min);
    d[11] = LODIGIT(printableTime.tm_min);
    d[12] = HIDIGIT(printableTime.tm_sec);
    d[13] = LODIGIT(printableTime.tm_sec);
    d[14] = 'Z';
    return SECSuccess;
}

/*
    The caller should make sure that the generalized time should only
    be used for the certificate validity after the year 2051; otherwise,
    the certificate should be consider invalid!?
 */
SECStatus
DER_GeneralizedTimeToTime(int64 *dst, SECItem *time)
{
#ifndef NSPR20
    PRTime genTime;
#else
    PRExplodedTime genTime;
#endif /* NSPR20 */
    char *string;
    long hourOff, minOff;
    uint16 centry;

    string = (char *)time->data;
    PORT_Memset (&genTime, 0, sizeof (genTime));

    /* Verify time is formatted properly and capture information */
    hourOff = 0;
    minOff = 0;

    CAPTURE(centry, string+0, loser);
    centry *= 100;
    CAPTURE(genTime.tm_year,string+2,loser);
    genTime.tm_year += centry;

#ifndef NSPR20
    CAPTURE(genTime.tm_mon,string+4,loser);
    if ((genTime.tm_mon == 0) || (genTime.tm_mon > 12)) goto loser;
#else
    CAPTURE(genTime.tm_month,string+4,loser);
    if ((genTime.tm_month == 0) || (genTime.tm_month > 12)) goto loser;
#endif /* NSPR20 */

    /* NSPR month base is 0 */
#ifndef NSPR20
    --genTime.tm_mon;
#else
    --genTime.tm_month;
#endif /* NSPR20 */
    
    CAPTURE(genTime.tm_mday,string+6,loser);
    if ((genTime.tm_mday == 0) || (genTime.tm_mday > 31)) goto loser;
    
    CAPTURE(genTime.tm_hour,string+8,loser);
    if (genTime.tm_hour > 23) goto loser;
    
    CAPTURE(genTime.tm_min,string+10,loser);
    if (genTime.tm_min > 59) goto loser;
    
    if (ISDIGIT(string[12])) {
	CAPTURE(genTime.tm_sec,string+12,loser);
	if (genTime.tm_sec > 59) goto loser;
	string += 2;
    }
    if (string[12] == '+') {
	CAPTURE(hourOff,string+13,loser);
	if (hourOff > 23) goto loser;
	CAPTURE(minOff,string+15,loser);
	if (minOff > 59) goto loser;
    } else if (string[12] == '-') {
	CAPTURE(hourOff,string+13,loser);
	if (hourOff > 23) goto loser;
	hourOff = -hourOff;
	CAPTURE(minOff,string+15,loser);
	if (minOff > 59) goto loser;
	minOff = -minOff;
    } else if (string[12] != 'Z') {
	goto loser;
    }

    /* Since the values of hourOff and minOff are small, there will
       be no loss of data by the conversion to int8 */
#ifdef NSPR20
    /* Convert the GMT offset to seconds and save it it genTime
       for the implode time process */
    genTime.tm_params.tp_gmt_offset = (PRInt32)((hourOff * 60L + minOff) * 60L);
    *dst = PR_ImplodeTime (&genTime);
#else
    *dst = PR_ImplodeTime (&genTime, (int8)hourOff,(int8)minOff);
#endif
    return SECSuccess;

  loser:
    PORT_SetError(SEC_ERROR_INVALID_TIME);
    return SECFailure;
	
}
