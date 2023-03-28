
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
#include <stdio.h>

#include "mallocin.h"

#ifndef lint
static char rcs_header[] = "$Id: calloc.c,v 1.1 1996/06/18 03:29:12 warren Exp $";
#endif

/*
 * Function:	calloc()
 *
 * Purpose:	to call debug_calloc to do the allocation
 *
 * Arguments:	nelem	- number of elements
 *		elsize	- size of each element
 *
 * Returns:	whatever debug_calloc returns
 *
 * Narrative:	call debug_calloc and return it's return
 */
DATATYPE *
unlocked_calloc(nelem,elsize)
	SIZETYPE 	  nelem;
	SIZETYPE	  elsize;
{
	return( debug_calloc((char *)NULL,(int)-1,nelem,elsize) );
}

/*
 * Function:	debug_calloc()
 *
 * Purpose:	to allocate and nullify a data area
 *
 * Arguments:	nelem	- number of elements
 *		elsize	- size of each element
 *
 * Returns:	NULL	- if malloc fails
 *		or pointer to allocated space
 *
 * Narrative:	determine size of area to malloc
 *		malloc area.
 *		if malloc succeeds
 *		    fill area with nulls
 *		return ptr to malloc'd region
 */

DATATYPE *
debug_calloc(file,line,nelem,elsize)
	CONST char	* file;
	int		  line;
	SIZETYPE 	  nelem;
	SIZETYPE 	  elsize;
{
	static IDTYPE	  call_counter;

	/*
	 * increment call counter
	 */
	call_counter++;

	return( DBFcalloc("calloc",M_T_CALLOC,call_counter,
			   file,line,nelem,elsize) );

}

char *
DBFcalloc(func,type,call_counter,file,line,nelem,elsize)
	CONST char	* func;
	int		  type;
	IDTYPE		  call_counter;
	CONST char	* file;
	int		  line;
	SIZETYPE 	  nelem;
	SIZETYPE 	  elsize;
{
	DATATYPE	* ptr;
	SIZETYPE	  size;

	/*
	 * make sure malloc sub-system is initialized.
	 */
	MALLOC_INIT();

	/*
	 * calculate the size to allocate
	 */
	size = elsize * nelem;

	/*
	 * if the user wants to be warned about zero length mallocs, do so
	 */
	if( ((malloc_opts & MOPT_ZERO) != 0) && (size == 0) )
	{
		malloc_errno = M_CODE_ZERO_ALLOC;
		malloc_warning(func,file,line,(struct mlist *) NULL);
	}

	ptr = DBFmalloc(func,type,call_counter,file,line,size);

	if( ptr != NULL )
	{
		/*
		 * clear the area that was allocated
		 */
		VOIDCAST memset(ptr,'\0',size);
	}

	return(ptr);

} /* DBFcalloc(... */

/*
 * Function:	cfree()
 *
 * Purpose:	to free an area allocated by calloc (actually frees any
 *		allocated area)
 *
 * Arguments:	cptr	- pointer to area to be freed
 *
 * Returns:	nothing of any use
 *
 * Narrative:	just call the appropriate function to perform the free
 *
 * Note:	most systems do not have such a call, but for the few that do,
 *		it is here.
 */
FREETYPE
cfree( cptr )
	DATATYPE	* cptr;
{
	debug_cfree((CONST char *)NULL,(int)-1, cptr);
}

FREETYPE
debug_cfree(file,line,cptr)
	CONST char	* file;
	int		  line;
	DATATYPE	* cptr;
{
	static IDTYPE	  call_counter;

	call_counter++;	

	DBFfree("cfree",F_T_CFREE,call_counter,file,line,cptr);
}

/*
 * $Log: calloc.c,v $
 * Revision 1.1  1996/06/18 03:29:12  warren
 * Added debug malloc package.
 *
 * Revision 1.17  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.16  1992/07/12  15:30:58  cpcahil
 * Merged in Jonathan I Kamens' changes
 *
 * Revision 1.15  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.14  1992/04/22  18:17:32  cpcahil
 * added support for Xt Alloc functions, linted code
 *
 * Revision 1.13  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.12  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.11  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.10  1991/12/06  17:58:42  cpcahil
 * added cfree() for compatibility with some wierd systems
 *
 * Revision 1.9  91/12/02  19:10:08  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.8  91/11/25  14:41:51  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.7  91/11/24  00:49:21  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.6  90/05/11  00:13:07  cpcahil
 * added copyright statment
 * 
 * Revision 1.5  90/02/24  20:41:57  cpcahil
 * lint changes.
 * 
 * Revision 1.4  90/02/24  17:25:47  cpcahil
 * changed $header to $id so full path isn't included.
 * 
 * Revision 1.3  90/02/24  13:32:24  cpcahil
 * added function header.  moved log to end of file.
 * 
 * Revision 1.2  90/02/22  23:08:26  cpcahil
 * fixed rcs_header line
 * 
 * Revision 1.1  90/02/22  23:07:38  cpcahil
 * Initial revision
 * 
 */
