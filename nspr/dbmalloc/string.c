
/*
 * (c) Copyright 1990, 1991, 1992 Conor P. Cahill (cpcahil@virtech.vti.com)
 *
 * This software may be distributed freely as long as the following conditions
 * are met:
 * 		* the distribution, or any derivative thereof, may not be
 *		  included as part of a commercial product
 *		* full source code is provided including this copyright
 *		* there is no charge for the software itself (there may be
 *		  a minimal charge for the copying or distribution effort)
 *		* this copyright notice is not modified or removed from any
 *		  source file
 */

/*
 * NOTE: if you have problems compiling this file, the first thing to try is
 * to take out the include of string.h.  This is due to the fact that some
 * systems (like ultrix) have conflicting definitions and some (like aix)
 * even set up some of these functions to be in-lined.
 */

#include <stdio.h>
#if ! defined(_IBMR2) && ! defined(ultrix)
#include <string.h>
#endif

#include "mallocin.h"


#include <ctype.h>

#ifndef CTYPE_SUPPORT
#define CTYPE_SUPPORT 1
#endif

#if CTYPE_SUPPORT == 3

#undef islower
#undef isascii
#undef toupper

#endif

#if CTYPE_SUPPORT > 1

#define isascii(a) 1

#endif

#if CTYPE_SUPPORT == 2

#include <locale.h>

static int locale_initialized;

#define INIT_CTYPE() (local_initialized ? 0 : \
			 local_initialized++, setlocale(LC_CTYPE,"") )

#else /* CTYPE_SUPPORT == 2 */

#define INIT_CTYPE()

#endif /* CTYPE_SUPPORT == 2 */

#ifndef lint
static
char rcs_hdr[] = "$Id: string.c,v 1.1 1996/06/18 03:29:27 warren Exp $";
#endif

static int	in_string_code;

#define CompareUpper(c) ((STRCMPTYPE) \
			(isascii((int)(unsigned char)(c)) \
				&& islower( (int) (unsigned char)(c)) ? \
				toupper((int)(unsigned char)(c)) \
			 /* else */ : \
				(c)))

/*
 * strcat - concatenate a string onto the end of another string
 */
char *
strcat(str1,str2)
	char		* str1;
	CONST char	* str2;
{
	return( DBstrcat((char *)NULL,0,str1,str2) );
}

char *
DBstrcat(file,line,str1,str2)
	CONST char		* file;
	int			  line;
	register char		* str1;
	register CONST char	* str2;
{
	char			* rtn;
	SIZETYPE		  len;

	/* 
	 * check pointers agains malloc region.  The malloc* functions
	 * will properly handle the case where a pointer does not
	 * point into malloc space.
	 */
	in_string_code++;

	len = strlen(str2);
	malloc_check_str("strcat", file, line, str2);

	len += strlen(str1) + 1;
	in_string_code--;

	malloc_check_data("strcat", file, line, str1, (SIZETYPE)len);

	rtn = str1;

	while( *str1 )
	{
		str1++;
	}
	
	while( (*str1 = *str2) != '\0' )
	{
		str1++;
		str2++;
	}
	
	return(rtn);
}

/*
 * strdup - duplicate a string
 */
char *
strdup(str1)
	CONST char	* str1;
{
	return( DBstrdup((char *)NULL, 0, str1) );
}

char *
DBstrdup(file, line, str1)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
{
	char			* rtn;
	register char		* str2;

	malloc_check_str("strdup", file, line, str1);

	in_string_code++;
	rtn = str2 = malloc((SIZETYPE)strlen(str1)+1);
	in_string_code--;

	if( rtn != (char *) 0)
	{
		while( (*str2 = *str1) != '\0' )
		{
			str1++;
			str2++;
		}
	}

	return(rtn);
}

/*
 * strncat - concatenate a string onto the end of another up to a specified len
 */
char *
strncat(str1,str2,len)
	char		* str1;
	CONST char	* str2;
	STRSIZE 	  len;
{
	return( DBstrncat((char *)NULL, 0, str1, str2, len) );
}

char *
DBstrncat(file, line, str1, str2, len)
	CONST char		* file;
	int			  line;
	register char		* str1;
	register CONST char	* str2;
	register STRSIZE 	  len;
{
	STRSIZE 	  len1;
	STRSIZE 	  len2;
	char		* rtn;

	malloc_check_strn("strncat", file, line, str2, len);

	in_string_code++;

	len2 = strlen(str2) + 1;
	len1 = strlen(str1);

	in_string_code--;


	if( (len+1) < len2 )
	{
		len1 += len + 1;
	}
	else
	{
		len1 += len2;
	}
	malloc_check_data("strncat", file, line, str1, (SIZETYPE)len1);

	rtn = str1;

	while( *str1 )
	{
		str1++;
	}

	while( len && ((*str1++ = *str2++) != '\0') )
	{
		len--;
	}
	
	if( ! len )
	{
		*str1 = '\0';
	}

	return(rtn);
}

/*
 * strcmp - compare two strings
 */
int
XXXstrcmp(str1,str2)
	register CONST char	* str1;
	register CONST char	* str2;
{
	return( DBstrcmp((char *) NULL, 0, str1, str2) );
}

int
DBstrcmp(file, line, str1, str2)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
{
	malloc_check_str("strcmp", file, line, str1);
	malloc_check_str("strcmp", file, line, str2);

	while( *str1 && (*str1 == *str2) )
	{
		str1++;
		str2++;
	}


	/*
	 * in order to deal with the case of a negative last char of either
	 * string when the other string has a null
	 */
	if( (*str2 == '\0') && (*str1 == '\0') )
	{
		return(0);
	}
	else if( *str2 == '\0' )
	{
		return(1);
	}
	else if( *str1 == '\0' )
	{
		return(-1);
	}
	
	return( *(CONST STRCMPTYPE *)str1 - *(CONST STRCMPTYPE *)str2 );
}

/*
 * strncmp - compare two strings up to a specified length
 */
int
strncmp(str1,str2,len)
	register CONST char	* str1;
	register CONST char	* str2;
	register STRSIZE 	  len;
{
	return( DBstrncmp((char *)NULL, 0, str1, str2, len) );
}

int
DBstrncmp(file, line, str1,str2,len)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
	register STRSIZE 	  len;
{
	malloc_check_strn("strncmp", file, line, str1, len);
	malloc_check_strn("strncmp", file, line, str2, len);

	while( len > 0 && *str1 && (*str1 == *str2) )
	{
		len--;
		str1++;
		str2++;
	}

	if( len == 0 )
	{
		return(0);
	}
	/*
	 * in order to deal with the case of a negative last char of either
	 * string when the other string has a null
	 */
	if( (*str2 == '\0') && (*str1 == '\0') )
	{
		return(0);
	}
	else if( *str2 == '\0' )
	{
		return(1);
	}
	else if( *str1 == '\0' )
	{
		return(-1);
	}
	
	return( *(CONST STRCMPTYPE *)str1 - *(CONST STRCMPTYPE *)str2 );
}

/*
 * stricmp - compare two strings
 */
int
stricmp(str1,str2)
	register CONST char	* str1;
	register CONST char	* str2;
{
	return( DBstricmp((char *) NULL, 0, str1, str2) );
}

int
DBstricmp(file, line, str1, str2)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
{

	/*
	 * Make sure ctype is initialized
	 */
	INIT_CTYPE();

	malloc_check_str("stricmp", file, line, str1);
	malloc_check_str("stricmp", file, line, str2);

	while( *str1 && (CompareUpper(*str1) == CompareUpper(*str2)) )
	{
		str1++;
		str2++;
	}


	/*
	 * in order to deal with the case of a negative last char of either
	 * string when the other string has a null
	 */
	if( (*str2 == '\0') && (*str1 == '\0') )
	{
		return(0);
	}
	else if( *str2 == '\0' )
	{
		return(1);
	}
	else if( *str1 == '\0' )
	{
		return(-1);
	}
	
	return( CompareUpper(*str1) - CompareUpper(*str2) );

} /* DBstricmp(... */

/*
 * strincmp - compare two strings up to a specified length, ignoring case
 */
int
strincmp(str1,str2,len)
	register CONST char	* str1;
	register CONST char	* str2;
	register STRSIZE 	  len;
{
	return( DBstrincmp((char *)NULL, 0, str1, str2, len) );
}

int
DBstrincmp(file, line, str1,str2,len)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
	register STRSIZE 	  len;
{

	/*
	 * Make sure ctype is initialized
	 */
	INIT_CTYPE();

	malloc_check_strn("strincmp", file, line, str1, len);
	malloc_check_strn("strincmp", file, line, str2, len);

	while( len > 0 && *str1 && (CompareUpper(*str1)==CompareUpper(*str2)) )
	{
		len--;
		str1++;
		str2++;
	}

	if( len == 0 )
	{
		return(0);
	}
	/*
	 * in order to deal with the case of a negative last char of either
	 * string when the other string has a null
	 */
	if( (*str2 == '\0') && (*str1 == '\0') )
	{
		return(0);
	}
	else if( *str2 == '\0' )
	{
		return(1);
	}
	else if( *str1 == '\0' )
	{
		return(-1);
	}
	
	return( CompareUpper(*str1) - CompareUpper(*str2) );

} /* DBstrincmp(... */

/*
 * strcpy - copy a string somewhere else
 */
char *
strcpy(str1,str2)
	register char		* str1;
	register CONST char	* str2;
{
	return( DBstrcpy((char *)NULL, 0, str1, str2) );
}

char *
DBstrcpy(file, line, str1, str2)
	CONST char		* file;
	int			  line;
	register char		* str1;
	register CONST char	* str2;
{
	char		* rtn;
	SIZETYPE	  len;

	in_string_code++;
	len = strlen(str2) + 1;
	in_string_code--;

	malloc_check_data("strcpy", file, line, str1, len);
	malloc_check_data("strcpy", file, line, str2, len);

	rtn = str1;

	DataMC(str1, str2, len);

	return(rtn);
}

/*
 * strncpy - copy a string upto a specified number of chars somewhere else
 */
char *
strncpy(str1,str2,len)
	register char		* str1;
	register CONST char	* str2;
	register STRSIZE 	  len;
{
	return( DBstrncpy((char *)NULL, 0, str1, str2, len) );
}

char *
DBstrncpy(file,line,str1,str2,len)
	CONST char		* file;
	int			  line;
	register char		* str1;
	register CONST char	* str2;
	STRSIZE		 	  len;
{
	register SIZETYPE		  i;
	char			* rtn = str1;

	malloc_check_data("strncpy", file, line, str1, len);

	for(i=0; (i < len) && (str2[i] != '\0'); i++)
	{
		/* do nothing */;
	}

	malloc_check_data("strncpy", file, line, str2, i);

	/*
	 * copy the real data
	 */
	DataMC(str1,str2,i);

	/*
	 * if there is data left over
	 */
	if( i < len )
	{
		/*
		 * figure out where and how much is left over
		 */
		str1 += i;
		len  -= i;

		/*
		 * fill in the rest of the bytes with nulls
		 */
		DataMS(str1, '\0', len);
	}

	return(rtn);
}

/*
 * strlen - determine length of a string
 */
STRSIZE 
strlen(str1)
	CONST char	* str1;
{
	return( DBstrlen((char *) NULL, 0, str1) );
}

STRSIZE 
DBstrlen(file, line, str1)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
{
	register CONST char	* s;

	if(! in_string_code )
	{
		malloc_check_str("strlen", file, line, str1);
	}

	for( s = str1; *s; s++)
	{
	}

	return( s - str1 );
}

/*
 * strchr - find location of a character in a string
 */
char *
strchr(str1,c)
	CONST char	* str1;
	int		  c;
{
	return( DBstrchr((char *)NULL,0,str1,c) );
}

char *
DBstrchr(file, line, str1,c)
	CONST char		* file;
	int			  line;
	CONST char		* str1;
	int			  c;
{
	return( DBFstrchr("strchr",file,line,str1,c) );
}

char *
DBFstrchr(func,file, line, str1,c)
	CONST char		* func;
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register int		  c;
{
	malloc_check_str(func, file, line, str1);

	while( *str1 && (*str1 != (char) c) )
	{
		str1++;
	}

	if(*str1 != (char) c)
	{
		str1 = (char *) 0;
	}

	return((char *)str1);
}

/*
 * strrchr - find rightmost location of a character in a string
 */

char *
strrchr(str1,c)
	CONST char	* str1;
	int		  c;
{
	return( DBstrrchr( (char *)NULL, 0, str1, c) );
}

char *
DBstrrchr(file,line,str1,c)
	CONST char		* file;
	int			  line;
	CONST char		* str1;
	int			  c;
{
	return( DBFstrrchr("strchr",file,line,str1,c) );
}

char *
DBFstrrchr(func,file,line,str1,c)
	CONST char		* func;
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register int		  c;
{
	char	* rtn = (char *) 0;

	malloc_check_str(func, file, line, str1);

	while( *str1 )
	{
		if(*str1 == (char) c )
		{
			rtn = (char *)str1;
		}
		str1++;
	}

	if( *str1 == (char) c)
	{
		rtn = (char *)str1;
	}

	return(rtn);
}

/*
 * index - find location of character within string
 */
char *
index(str1,c)
	CONST char		* str1;
	char		  c;
{
	return( DBindex((char *) NULL, 0, str1, c) );
}
char *
DBindex(file, line, str1, c)
	CONST char	* file;
	int		  line;
	CONST char	* str1;
	char		  c;
{
	return( DBFstrchr("index",file,line,str1,c) );
}

/*
 * rindex - find rightmost location of character within string
 */
char *
rindex(str1,c)
	CONST char	* str1;
	char		  c;
{
	return( DBrindex((char *)NULL, 0, str1, c) );
}

char *
DBrindex(file, line, str1, c)
	CONST char	* file;
	int		  line;
	CONST char	* str1;
	char		  c;
{
	return( DBFstrrchr("rindex",file,line,str1,c) );
}

/*
 * strpbrk - find the first occurance of any character from str2 in str1
 */
char *
strpbrk(str1,str2)
	CONST char	* str1;
	CONST char	* str2;
{
	return( DBstrpbrk((char *)NULL, 0, str1, str2) );
}

char *
DBstrpbrk(file, line, str1,str2)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
{
	register CONST char	* tmp;

	malloc_check_str("strpbrk", file, line, str1);
	malloc_check_str("strpbrk", file, line, str2);

	while(*str1)
	{
		for( tmp=str2; *tmp && *tmp != *str1; tmp++)
		{
		}
		if( *tmp )
		{
			break;
		}
		str1++;
	}

	if( ! *str1 )
	{
		str1 = (char *) 0;
	}

	return( (char *) str1);
}

/*
 * strspn - get length of str1 that consists totally of chars from str2
 */
STRSIZE 
strspn(str1,str2)
	CONST char	* str1;
	CONST char	* str2;
{
	return( DBstrspn((char *)NULL, 0, str1, str2) );
}

STRSIZE 
DBstrspn(file, line, str1,str2)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
{
	register CONST char	* tmp;
	CONST char		* orig = str1;

	malloc_check_str("strspn", file, line, str1);
	malloc_check_str("strspn", file, line, str2);

	while(*str1)
	{
		for( tmp=str2; *tmp && *tmp != *str1; tmp++)
		{
		}
		if(! *tmp )
		{
			break;
		}
		str1++;
	}

	return( (STRSIZE ) (str1 - orig) );
}

/*
 * strcspn - get lenght of str1 that consists of no chars from str2
 */
STRSIZE 
strcspn(str1,str2)
	CONST char	* str1;
	CONST char	* str2;
{
	return( DBstrcspn((char *)NULL,0,str1,str2) );
}

STRSIZE 
DBstrcspn(file, line, str1,str2)
	CONST char		* file;
	int			  line;
	register CONST char	* str1;
	register CONST char	* str2;
{
	register CONST char	* tmp;
	CONST char	* orig = str1;

	malloc_check_str("strcspn", file, line, str1);
	malloc_check_str("strcspn", file, line, str2);

	while(*str1)
	{
		for( tmp=str2; *tmp && *tmp != *str1; tmp++)
		{
		}
		if( *tmp )
		{
			break;
		}
		str1++;
	}

	return( (int) (str1 - orig) );
}

/*
 * strstr - locate string within another string
 */
char *
strstr(str1, str2)
	CONST char	* str1;
	CONST char	* str2;
{
	return( DBstrstr((char *)NULL, 0, str1, str2) );
}

char *
DBstrstr(file, line, str1, str2)
	CONST char	* file;
	int		  line;
	CONST char	* str1;
	CONST char	* str2;
{
	register CONST char	* s;
	register CONST char	* t;
	
	malloc_check_str("strstr", file, line, str1);
	malloc_check_str("strstr", file, line, str2);

	/*
	 * until we run out of room in str1
	 */
	while( *str1 != '\0' )
	{
		/*
		 * get tmp pointers to both strings
		 */
		s = str2;
		t = str1;

		/*
		 * see if they match
		 */
		while( *s &&  (*s == *t) )
		{
			s++;
			t++;
		}

		/*
		 * if we ran out of str2, we found the match,
		 * so return the pointer within str1.
		 */
		if( ! *s )
		{
			return( (char *) str1);
		}
		str1++;
	}

	if( *str2 == '\0' )
	{
		return( (char *) str1);
	}
	return(NULL);
}

/*
 * strtok() source taken from that posted to comp.lang.c by Chris Torek
 * in Jan 1990.
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * Get next token from string s (NULL on 2nd, 3rd, etc. calls),
 * where tokens are nonempty strings separated by runs of
 * chars from delim.  Writes NULs into s to end tokens.  delim need not
 * remain CONSTant from call to call.
 *
 * Modified by cpc: 	changed variable names to conform with naming
 *			conventions used in rest of code.  Added malloc pointer
 *			check calls.
 */

char *
strtok(str1, str2)
	char 		* str1;
	CONST char	* str2;
{
	return( DBstrtok( (char *)NULL, 0, str1, str2) );
}

char *
DBstrtok(file, line, str1, str2)
	CONST char	* file;
	int		  line;
	char 		* str1;
	CONST char	* str2;
{
	static char 	* last;

	if( str1 )
	{
		malloc_check_str("strtok", file, line, str1);
		last = str1;
	}
	malloc_check_str("strtok", file, line, str2);

	return (strtoken(&last, str2, 1));
}


/*
 * Get next token from string *stringp, where tokens are (possibly empty)
 * strings separated by characters from delim.  Tokens are separated
 * by exactly one delimiter iff the skip parameter is false; otherwise
 * they are separated by runs of characters from delim, because we
 * skip over any initial `delim' characters.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim will usually, but need not, remain CONSTant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strtoken returns NULL.
 */
char *
strtoken(stringp, delim, skip)
	register char **stringp;
	register CONST char *delim;
	int skip;
{
	register char *s;
	register CONST char *spanp;
	register int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);

	if (skip) {
		/*
		 * Skip (span) leading delimiters (s += strspn(s, delim)).
		 */
	cont:
		c = *s;
		for (spanp = delim; (sc = *spanp++) != 0;) {
			if (c == sc) {
				s++;
				goto cont;
			}
		}
		if (c == 0) {		/* no token found */
			*stringp = NULL;
			return (NULL);
		}
	}

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return( (char *) tok );
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

/*
 * $Log: string.c,v $
 * Revision 1.1  1996/06/18 03:29:27  warren
 * Added debug malloc package.
 *
 * Revision 1.30  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.29  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.28  1992/06/30  13:06:39  cpcahil
 * added support for aligned allocations
 *
 * Revision 1.27  1992/06/27  22:48:48  cpcahil
 * misc fixes per bug reports from first week of reviews
 *
 * Revision 1.26  1992/06/23  11:12:54  cpcahil
 * fixed bug in case insensitive comparisons.
 *
 * Revision 1.25  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.24  1992/05/09  21:27:09  cpcahil
 * final (hopefully) changes for patch 11
 *
 * Revision 1.23  1992/05/09  00:16:16  cpcahil
 * port to hpux and lots of fixes
 *
 * Revision 1.22  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.21  1992/05/08  01:44:11  cpcahil
 * more performance enhancements
 *
 * Revision 1.20  1992/04/20  22:29:14  cpcahil
 * changes to fix problems introduced by insertion of size_t
 *
 * Revision 1.19  1992/04/14  01:15:25  cpcahil
 * port to RS/6000
 *
 * Revision 1.18  1992/04/13  19:08:18  cpcahil
 * fixed case insensitive stuff
 *
 * Revision 1.17  1992/04/13  18:41:18  cpcahil
 * added case insensitive string comparison routines
 *
 * Revision 1.16  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.15  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.14  1991/12/04  09:23:44  cpcahil
 * several performance enhancements including addition of free list
 *
 * Revision 1.13  91/12/02  19:10:14  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.12  91/11/25  14:42:05  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.11  91/11/24  16:56:43  cpcahil
 * porting changes for patch level 4
 * 
 * Revision 1.10  91/11/24  00:49:32  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.9  91/11/20  11:54:11  cpcahil
 * interim checkin
 * 
 * Revision 1.8  91/05/30  12:07:04  cpcahil
 * added fix to get the stuff to compile correctly on ultirx and aix systems.
 * 
 * Revision 1.7  90/08/29  22:24:19  cpcahil
 * added new function to check on strings up to a specified length 
 * and used it within several strn* functions.
 * 
 * Revision 1.6  90/07/16  20:06:56  cpcahil
 * fixed several minor bugs found with Henry Spencer's string/mem function
 * tester program.
 * 
 * Revision 1.5  90/06/10  14:59:49  cpcahil
 * Fixed a couple of bugs in strncpy & strdup
 * 
 * Revision 1.4  90/05/11  00:13:10  cpcahil
 * added copyright statment
 * 
 * Revision 1.3  90/02/24  21:50:32  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.2  90/02/24  17:29:40  cpcahil
 * changed $Header to $Id so full path wouldnt be included as part of rcs 
 * id string
 * 
 * Revision 1.1  90/02/22  23:17:44  cpcahil
 * Initial revision
 * 
 */
