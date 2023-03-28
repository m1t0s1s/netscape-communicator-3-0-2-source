
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
 * datams.c - this module contains functions designed to efficiently 
 *	      set memory areas to specified values in a portable fasion.
 *
 * The configure script will usually override this module with a copy
 * of the system suplied memset() utility if it can figure out how to
 * convert the module name to DataMS.
 */
#ifndef lint
static
char rcs_hdr[] = "$Id: datams.c,v 1.1 1996/06/18 03:29:14 warren Exp $";
#endif

#include <stdio.h>
#include "mallocin.h"

typedef int word;

#define wordmask (sizeof(word)-1)
#define FILL_ARRSIZ 	256

void
DataMS(ptr1,ch, len)
	MEMDATA			* ptr1;
	int			  ch;
	register MEMSIZE	  len;
{
	MEMSIZE			  i;
	register unsigned char	* ptr;
	word			  fillword;
	static word		  fillwords[FILL_ARRSIZ] = { -1 };

	/*
	 * if nothing to do, return immediatly.
	 */
	if( len <= 0 )
	{
		return;
	}

	/*
	 * if we haven't filled the fillwords array
	 */
	if( fillwords[0] == -1 )
	{
		int		  j;
		int		  k;

		/*
		 * fill em all at this time.
		 */
		for(k=0; k < FILL_ARRSIZ; k++)
		{
			ptr = (unsigned char *) &fillwords[k];
			for(j=0; j < sizeof(word); j++)
			{
				*(ptr++) = (unsigned char) k;
			}
		}
	}

	/*
	 * if the character is outside of the proper range, use 255
 	 */
	if( ((unsigned)ch) > 0xFF )
	{
		ch = ((unsigned)ch) & 0xFF;
	}

	/*
	 * save the word we will use for filling
	 */
	fillword = fillwords[(unsigned)ch];
			

	/*
	 * get the original pointer
	 */
	ptr = (unsigned char *) ptr1;

	/*
	 * if we are not at a word offset, handle the single bytes at the
	 * begining of the set
	 */
	if( (i = (((long)ptr) & wordmask)) != 0 )
	{
		i = sizeof(word) - i;
		while( (i-- > 0) && (len > 0) )
		{
			*(ptr++) = ch;
			len--;
		}
	}

	/*
	 * convert remaining number of bytes to number of words
	 */
	i = len >> (sizeof(word)/2);

	/*
	 * and fill them
	 */
	while( i-- )
	{
		*(word *)ptr = fillword;
		ptr += sizeof(word);
	}

	/*
	 * and now handle any trailing bytes
	 */
	if( (i = (len & wordmask)) != 0 )
	{
		while(i-- > 0 )
		{
			*(ptr++) = ch;
		}
	}

	return;

} /* DataMS(... */

