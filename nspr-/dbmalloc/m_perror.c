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

#ifndef lint
static
char rcsid[] = "$Id: m_perror.c,v 1.1 1996/06/18 03:29:17 warren Exp $";
#endif

#include "mallocin.h"

/*
 * malloc errno error strings...
 */

CONST char *malloc_err_strings[] = 
{
	"No errors",
	"Malloc chain is corrupted, pointers out of order",
	"Malloc chain is corrupted, end before end pointer",
	"Pointer is not within malloc area",
	"Malloc region does not have valid magic number in header",
	"Pointers between this segment and adjoining segments are invalid",
	"Data has overrun beyond requested number of bytes",
	"Data in free'd area has been modified",
	"Data area is not in use (can't be freed or realloced, or used)",
	"Unable to get additional memory from the system",
	"Pointer within malloc region, but outside of malloc data bounds",
	"Malloc segment in free list is in-use",
	"Unable to determine doubleword boundary",
	"No current function on stack, probably missing call to malloc_enter ",
	"Current function name doesn't match name on stack",
	"Data has written before beginning of requested bytes",
	"Free of a marked segment",
	"Allocation of zero length segment",
	(CONST char *) 0
};

/*
 * Function:	malloc_perror()
 *
 * Purpose:	to print malloc_errno error message
 *
 * Arguments:	str	- string to print with error message
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 */
VOIDTYPE
malloc_perror(str)
	CONST char	* str;
{
	register CONST char 	* s;
	register CONST char 	* t;

	if( str && *str)
	{
		for(s=str; *s; s++)
		{
			/* do nothing */;
		}

		VOIDCAST write(2,str,(WRTSIZE)(s-str));
		VOIDCAST write(2,": ",(WRTSIZE)2);
	}

	t = malloc_err_strings[malloc_errno];

	for(s=t; *s; s++)
	{
		/* do nothing */;
	}

	VOIDCAST write(2,t,(WRTSIZE)(s-t));

	VOIDCAST write(2,"\n",(WRTSIZE)1);
}

/*
 * $Log: m_perror.c,v $
 * Revision 1.1  1996/06/18 03:29:17  warren
 * Added debug malloc package.
 *
 * Revision 1.23  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.22  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.21  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.20  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.19  1992/05/08  01:44:11  cpcahil
 * more performance enhancements
 *
 * Revision 1.18  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.17  1992/04/20  22:29:14  cpcahil
 * changes to fix problems introduced by insertion of size_t
 *
 * Revision 1.16  1992/04/15  12:51:06  cpcahil
 * fixes per testing of patch 8
 *
 * Revision 1.15  1992/04/15  11:47:54  cpcahil
 * spelling changes.
 *
 * Revision 1.14  1992/04/14  01:15:25  cpcahil
 * port to RS/6000
 *
 * Revision 1.13  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.12  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.11  1992/02/19  01:42:29  cpcahil
 * fixed typo in error message
 *
 * Revision 1.10  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.9  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.8  1991/12/04  09:23:38  cpcahil
 * several performance enhancements including addition of free list
 *
 * Revision 1.7  91/11/25  14:41:55  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.6  91/11/24  00:49:27  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.5  90/08/29  21:25:08  cpcahil
 * added additional error message that was missing (and 
 * caused a core dump)
 * 
 * Revision 1.4  90/05/11  00:13:08  cpcahil
 * added copyright statment
 * 
 * Revision 1.3  90/02/24  21:50:21  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.2  90/02/24  17:39:55  cpcahil
 * 1. added function header
 * 2. added rcs id and log strings.
 * 
 */
