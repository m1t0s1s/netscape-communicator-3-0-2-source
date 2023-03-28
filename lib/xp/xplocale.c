/* xplocale.c
* -----------
*/

/* xp headers */ 
#include "xplocale.h"
#include "ntypes.h"
#include "xp_str.h"

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

/* fun: XP_StrColl 
* ---------------
* Takes two strings to compare, compares them, 
* and returns a number less than 0 if the second
* string is greater, 0 if they are the same, 
* and greater than 0 if the first string is 
* greater, according to the sorting rules 
* appropriate for the current locale.
*/

int XP_StrColl(const char* s1, const  char* s2) 
{
	return(FE_StrColl(s1, s2));	
}	


/* XP_StrfTime */
/* Returns 0 on error, size of return string otherwise */


size_t XP_StrfTime(MWContext* context, char *result, size_t maxsize, int format,
    const struct tm *timeptr)

{

	if(!timeptr)
	{
		if(result)
			*result = '\0';

		return 0;
	}

/* Maybe eventually do some locale setting here */
return(FE_StrfTime(context, result, maxsize, format, timeptr));

} 




/* XP_Strxfrm
* ----------
* For UNIX & possibly WIN 32
*/

size_t XP_Strxfrm ( char *s1, const  char *s2, size_t n) 
{

#ifdef XP_UNIX

return(strxfrm(s1, s2, n));

#endif


#ifdef XP_WIN_32

return(strxfrm(s1, s2, n));

#endif

return( (size_t) -1 ); 
}

const char* INTL_ctime(MWContext* context, time_t *date)
{
  static char result[40];	
  if(date != NULL)
  {
	  XP_StrfTime(context, result, sizeof(result), XP_LONG_DATE_TIME_FORMAT, localtime(date));
  } else {
  	  result[0] = '\0';
  }
  return result;
}
