#include "nspr_md.h"
#include "sys_api.h"
#include "prtime.h"

#ifdef XP_UNIX
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifdef sgi
char *vbase = (char*) 0x40000000;
#endif

#endif

#ifdef XP_PC
#include <time.h>
char *vbase = (char*) 0x40000000;
#endif

#ifdef XP_MAC
char *vbase = (char*) 0x40000000;
#endif

long sysGetMilliTicks(void)
{
    int64 now;
    long l;

    now = PR_NowMS();
    LL_L2I(l, now);
    return l;
}

long sysTime(long *p)
{
    return (long) time((time_t*)p);/* XXX bug! */
}

int64_t sysTimeMillis(void)
{
    int64 now;

    now = PR_NowMS();
    return now;
}

struct tm * sysGmtime(time_t * timer, struct tm *result)
{
	uint64	timeInMicros,
			s2us;
	PRTime	localPRTimeRecord;

    LL_I2L(timeInMicros, *timer);
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_MUL(timeInMicros, timeInMicros, s2us);
    
    timeInMicros = PR_ToGMT(timeInMicros);
    
    PR_ExplodeTime(&localPRTimeRecord, timeInMicros);
	
   	result->tm_sec = localPRTimeRecord.tm_sec;
 	result->tm_min = localPRTimeRecord.tm_min;
 	result->tm_hour = localPRTimeRecord.tm_hour;
 	result->tm_mday = localPRTimeRecord.tm_mday;
 	result->tm_mon = localPRTimeRecord.tm_mon;
 	result->tm_wday = localPRTimeRecord.tm_wday;
 	result->tm_year = localPRTimeRecord.tm_year - 1900;
 	result->tm_yday = localPRTimeRecord.tm_yday;
 	result->tm_isdst = localPRTimeRecord.tm_isdst;
    
    return result;
}

struct tm * sysLocaltime( time_t *timer, struct tm * result )
{
	uint64	timeInMicros,
			s2us;
	PRTime	localPRTimeRecord;

    LL_I2L(timeInMicros, *timer);
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_MUL(timeInMicros, timeInMicros, s2us);
    
    PR_ExplodeTime(&localPRTimeRecord, timeInMicros);
	
   	result->tm_sec = localPRTimeRecord.tm_sec;
 	result->tm_min = localPRTimeRecord.tm_min;
 	result->tm_hour = localPRTimeRecord.tm_hour;
 	result->tm_mday = localPRTimeRecord.tm_mday;
 	result->tm_mon = localPRTimeRecord.tm_mon;
 	result->tm_wday = localPRTimeRecord.tm_wday;
 	result->tm_year = localPRTimeRecord.tm_year - 1900;
 	result->tm_yday = localPRTimeRecord.tm_yday;
 	result->tm_isdst = localPRTimeRecord.tm_isdst;
    
    return result;
}

void sysStrftime(char *buf, int buflen, char *fmt, struct tm *t)
{
	PRTime prTimeRecord;

	prTimeRecord.tm_usec = 0;
	prTimeRecord.tm_sec = t->tm_sec;
	prTimeRecord.tm_min = t->tm_min;
	prTimeRecord.tm_hour = t->tm_hour;
	prTimeRecord.tm_mday = t->tm_mday;
	prTimeRecord.tm_mon = t->tm_mon;
	prTimeRecord.tm_wday = t->tm_wday;
	prTimeRecord.tm_year = t->tm_year + 1900;
	prTimeRecord.tm_yday = t->tm_yday;
	prTimeRecord.tm_isdst = t->tm_isdst;

	PR_FormatTime(buf, buflen, fmt, &prTimeRecord);
	
}

time_t sysMktime(struct tm *t)
{
	PRTime	 	prTimeRecord;
	uint64		timeInMicros;
	time_t		result;
	uint64		resultLong,
				divisor;

	prTimeRecord.tm_usec = 0;
	prTimeRecord.tm_sec = t->tm_sec;
	prTimeRecord.tm_min = t->tm_min;
	prTimeRecord.tm_hour = t->tm_hour;
	prTimeRecord.tm_mday = t->tm_mday;
	prTimeRecord.tm_mon = t->tm_mon;
	prTimeRecord.tm_wday = t->tm_wday;
	prTimeRecord.tm_year = t->tm_year + 1900;
	prTimeRecord.tm_yday = t->tm_yday;
	prTimeRecord.tm_isdst = t->tm_isdst;
	
	timeInMicros = PR_ComputeTime(&prTimeRecord);
	
	LL_I2L(divisor, PR_USEC_PER_SEC);
	LL_DIV(resultLong, timeInMicros, divisor);
	LL_L2I(result, resultLong);

	return result;

}


#ifdef XP_PC
/* rjp -- revisit */
double rint( double a0 )
{
    int a;

    a = (int) a0;
    return a*1.0;
}
#endif


#if (defined(XP_PC) || defined(XP_MAC))

/*
** More missing RTL functions that are required by the runtime...
*/


void srand48( long seed )
{
    srand( (unsigned int) seed );
}

double drand48( void )
{
    return rand() / 32787.0;
}

#endif
/*
** Reentrant versions of the runtime functions gmtime() and localtime()
*/

#ifdef XP_PC
#include <socket_md.h>

/*
   rjp -- this assumes that the multi-threaded RTL uses tls for the
          tm structure
*/
struct tm * gmtime_r( const time_t * timer, struct tm *result )
{
    *result = *gmtime( timer );
    return result;
}

struct tm * localtime_r( const time_t *timer, struct tm * result )
{
    *result = *localtime( timer );
    return result;
}

#endif
