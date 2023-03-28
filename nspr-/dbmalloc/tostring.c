
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
#include "tostring.h"
#include "mallocin.h"

/*
 * Function:	tostring()
 *
 * Purpose:	to convert an integer to an ascii display string
 *
 * Arguments:	buf	- place to put the 
 *		val	- integer to convert
 *		len	- length of output field (0 if just enough to hold data)
 *		base	- base for number conversion (only works for base <= 16)
 *		fill	- fill char when len > # digits
 *
 * Returns:	length of string
 *
 * Narrative:	IF fill character is non-blank
 *		    Determine base
 *		        If base is HEX
 *		            add "0x" to begining of string
 *		        IF base is OCTAL
 *		            add "0" to begining of string
 *
 *		While value is greater than zero
 *		    use val % base as index into xlation str to get cur char
 *		    divide val by base
 *
 *		Determine fill-in length
 *
 *		Fill in fill chars
 *
 *		Copy in number
 *		
 *
 * Mod History:	
 *   90/01/24	cpcahil		Initial revision.
 */

#ifndef lint
static
char rcs_hdr[] = "$Id: tostring.c,v 1.1 1996/06/18 03:29:31 warren Exp $";
#endif

#define T_LEN 15

int
tostring(buf,val,len,base,fill)
	char		* buf;
	unsigned long	  val;
	int		  len;
	int		  base;
	char		  fill;
	
{
	char		* bufstart = buf;
	int		  filled = 0;
	int		  i = T_LEN;
	CONST char	* xbuf = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char		  tbuf[T_LEN];

	/*
	 * if we are filling with non-blanks, make sure the
	 * proper start string is added
	 */
	if( fill != ' ' )
	{
		switch(base)
		{
			case B_HEX:
				if( (len == 0) ||  (len > 2) )
				{
					filled = 2;
					*(buf++) = '0';
					*(buf++) = 'x';
					if( len )
					{
						len -= 2;
					}
				}
				break;
			case B_OCTAL:
				*(buf++) = fill;
				if( len )
				{
					len--;
				}
				break;
			default:
				break;
		}
	}

	/*
	 * convert the value to a string
	 */
	do
	{
		tbuf[--i] = xbuf[val % (unsigned long)base];
		val = val / (unsigned long)base;
	}
	while( val > 0 );


	if( len )
	{
		len -= (T_LEN - i);

		/*
		 * if we are out of room and we already filled in some of the
		 * output buffer (usually 0x for hex output), strip the pre-fill
		 * so that we get as much data as we can.
		 */
		if( (len < 0) && filled )
		{
			len += filled;
			buf -= filled;
		}

		if( len > 0 )
		{
			while(len-- > 0)
			{
				*(buf++) = fill;
			}
		}
		else
		{
			/* 
			 * string is too long so we must truncate
			 * off some characters.  We do this the easiest
			 * way by just incrementing i.  This means the
			 * most significant digits are lost.
			 */
			while( len++ < 0 )
			{
				i++;
			}
		}
	}

	while( i < T_LEN )
	{
		*(buf++) = tbuf[i++];
	}

	return( (int) (buf - bufstart) );

} /* tostring(... */

/*
 * $Log: tostring.c,v $
 * Revision 1.1  1996/06/18 03:29:31  warren
 * Added debug malloc package.
 *
 * Revision 1.9  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.8  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.7  1992/04/14  02:27:30  cpcahil
 * adjusted output of pointes so that values that had the high bit
 * set would print correctly.
 *
 * Revision 1.6  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.5  1991/11/25  14:42:06  cpcahil
 * Final changes in preparation for patch 4 release
 *
 * Revision 1.4  90/05/11  00:13:11  cpcahil
 * added copyright statment
 * 
 * Revision 1.3  90/02/24  21:50:33  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.2  90/02/24  17:29:42  cpcahil
 * changed $Header to $Id so full path wouldnt be included as part of rcs 
 * id string
 * 
 * Revision 1.1  90/02/22  23:17:44  cpcahil
 * Initial revision
 * 
 */
