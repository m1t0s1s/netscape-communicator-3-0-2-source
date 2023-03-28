/* -*- Mode: C++; tab-width: 4 -*- */
#include "xp.h"
#include <ctype.h>
#include <stdarg.h>

#ifdef PROFILE
#pragma profile on
#endif

#ifndef XP_WIN /* whatever, keep this out of the winfe damnit */
#if defined(DEBUG)

char *
NOT_NULL (const char * p)
{
	XP_ASSERT(p);
	return (char*) p;
}

#endif
#endif

/* find a substring within a string with a case insensitive search
 */
PUBLIC char *
strcasestr (const char * str, const char * substr)
{
    register const char *pA;
    register const char *pB;
    register const char *pC;

	if(!str)
		return(NULL);
	
	for(pA=str; *pA; pA++)
	  {
		if(XP_TO_UPPER(*pA) == XP_TO_UPPER(*substr))
		  {
    		for(pB=pA, pC=substr; ; pB++, pC++)
              {
                if(!(*pC))
                    return((char *)pA);

				if(!(*pB))
					break;

				if(XP_TO_UPPER(*pB) != XP_TO_UPPER(*pC))
					break;
              }
		  }
	  }

	return(NULL);
}

/* find a substring within a specified length string with a case 
 * insensitive search
 */
PUBLIC char *
strncasestr (const char * str, const char * substr, int32 len)
{
	register int count=0;
	register int count2=0;
    register const char *pA;
    register const char *pB;
    register const char *pC;

    if(!str || !substr)
        return(NULL);
   
    for(pA=str; count < len; pA++, count++)
      {
        if(XP_TO_UPPER(*pA) == XP_TO_UPPER(*substr))
          {
            for(pB=pA, pC=substr, count2=count; count2<len; 
													count2++,pB++,pC++)
              {
                if(!(*pC))
                    return((char *)pA);

                if(!(*pB))
                    break;

                if(XP_TO_UPPER(*pB) != XP_TO_UPPER(*pC))
                    break;
              }
          }
      }

    return(NULL);
}


/*	compare strings in a case insensitive manner
*/
PUBLIC int 
strcasecomp (const char* one, const char *two)
{
	const char *pA;
	const char *pB;

	for(pA=one, pB=two; *pA && *pB; pA++, pB++) 
	  {
	    int tmp = XP_TO_LOWER(*pA) - XP_TO_LOWER(*pB);
	    if (tmp) 
			return tmp;
	  }
	if (*pA) 
		return 1;	
	if (*pB) 
		return -1;
	return 0;	
}


/*	compare strings in a case insensitive manner with a length limit
*/
PUBLIC int 
strncasecomp (const char* one, const char * two, int n)
{
	const char *pA;
	const char *pB;
	
	for(pA=one, pB=two;; pA++, pB++) 
	  {
	    int tmp;
	    if (pA == one+n) 
			return 0;	
	    if (!(*pA && *pB)) 
			return *pA - *pB;
	    tmp = XP_TO_LOWER(*pA) - XP_TO_LOWER(*pB);
	    if (tmp) 
			return tmp;
	  }
}

/*	Allocate a new copy of a block of binary data, and returns it
 */
PUBLIC char * 
NET_BACopy (char **destination, const char *source, size_t length)
{
	if(*destination)
	  {
	    XP_FREE(*destination);
		*destination = 0;
	  }

    if (! source)
	  {
        *destination = NULL;
	  }
    else 
	  {
        *destination = (char *) XP_ALLOC (length);
        if (*destination == NULL) 
	        return(NULL);
        XP_MEMCPY(*destination, source, length);
      }
    return *destination;
}

/*	binary block Allocate and Concatenate
 *
 *   destination_length  is the length of the existing block
 *   source_length   is the length of the block being added to the 
 *   destination block
 */
PUBLIC char * 
NET_BACat (char **destination, 
		   size_t destination_length, 
		   const char *source, 
		   size_t source_length)
{
    if (source) 
	  {
        if (*destination) 
	      {
      	    *destination = (char *) XP_REALLOC (*destination, destination_length + source_length);
            if (*destination == NULL) 
	          return(NULL);

            XP_MEMMOVE (*destination + destination_length, source, source_length);

          } 
		else 
		  {
            *destination = (char *) XP_ALLOC (source_length);
            if (*destination == NULL) 
	          return(NULL);

            XP_MEMCPY(*destination, source, source_length);
          }
    }

  return *destination;
}

/*	Very similar to strdup except it free's too
 */
PUBLIC char * 
NET_SACopy (char **destination, const char *source)
{
	if(*destination)
	  {
	    XP_FREE(*destination);
		*destination = 0;
	  }
    if (! source)
	  {
        *destination = NULL;
	  }
    else 
	  {
        *destination = (char *) XP_ALLOC (XP_STRLEN(source) + 1);
        if (*destination == NULL) 
 	        return(NULL);

        XP_STRCPY (*destination, source);
      }
    return *destination;
}

/*  Again like strdup but it concatinates and free's and uses Realloc
*/
PUBLIC char *
NET_SACat (char **destination, const char *source)
{
    if (source && *source)
      {
        if (*destination)
          {
            int length = XP_STRLEN (*destination);
            *destination = (char *) XP_REALLOC (*destination, length + XP_STRLEN(source) + 1);
            if (*destination == NULL)
            return(NULL);

            XP_STRCPY (*destination + length, source);
          }
        else
          {
            *destination = (char *) XP_ALLOC (XP_STRLEN(source) + 1);
            if (*destination == NULL)
                return(NULL);

             XP_STRCPY (*destination, source);
          }
      }
    return *destination;
}

/* remove front and back white space
 * modifies the original string
 */
PUBLIC char *
XP_StripLine (char *string)
{
    char * ptr;

	/* remove leading blanks */
    while(*string=='\t' || *string==' ' || *string=='\r' || *string=='\n')
		string++;    

    for(ptr=string; *ptr; ptr++)
		;   /* NULL BODY; Find end of string */

	/* remove trailing blanks */
    for(ptr--; ptr >= string; ptr--) 
	  {
        if(*ptr=='\t' || *ptr==' ' || *ptr=='\r' || *ptr=='\n') 
			*ptr = '\0'; 
        else 
			break;
	  }

    return string;
}

/************************************************************************/

char *XP_AppendStr(char *in, char *append)
{
    int alen, inlen;

    alen = XP_STRLEN(append);
    if (in) {
		inlen = XP_STRLEN(in);
		in = (char*) XP_REALLOC(in,inlen+alen+1);
		if (in) {
			XP_MEMCPY(in+inlen, append, alen+1);
		}
    } else {
		in = (char*) XP_ALLOC(alen+1);
		if (in) {
			XP_MEMCPY(in, append, alen+1);
		}
    }
    return in;
}

char *XP_Cat(char *a0, ...)
{
    va_list ap;
    char *a, *result, *cp;
    int len;

	/* Count up string length's */
    va_start(ap, a0);
	len = 1;
	a = a0;
    while (a != (char*) NULL) {
		len += XP_STRLEN(a);
		a = va_arg(ap, char*);
    }
	va_end(ap);

	/* Allocate memory and copy strings */
    va_start(ap, a0);
	result = cp = (char*) XP_ALLOC(len);
	if (!cp) return 0;
	a = a0;
	while (a != (char*) NULL) {
		len = XP_STRLEN(a);
		XP_MEMCPY(cp, a, len);
		cp += len;
		a = va_arg(ap, char*);
	}
	*cp = 0;
	va_end(ap);
    return result;
}

#ifdef PROFILE
#pragma profile off
#endif
