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
#include "debug.h"

/*
 * Function:	malloc_size()
 *
 * Purpose:	return the size of the allocated segment associated with ptr
 *
 * Arguments:	ptr	- pointer to allocated area 
 *
 * Returns:	the size of the segment
 *
 * Narrative:
 *		verify pointer is within malloc region
 *		get mlist pointer from passed address
 *		verify magic number
 *		verify inuse flag
 *		verify pointer connections with surrounding segments
 *		return size of segment
 */
#ifndef lint
static
char rcs_hdr[] = "$Id: size.c,v 1.1 1996/06/18 03:29:26 warren Exp $";
#endif

SIZETYPE
malloc_size(cptr)
	CONST DATATYPE	* cptr;
{
	return( DBmalloc_size((char *)NULL, 0, cptr) );
}

SIZETYPE
DBmalloc_size(file,line,cptr)
	CONST char	* file;
	int		  line;
	CONST DATATYPE	* cptr;
{
	char			* func = "malloc_size";
	register struct mlist	* ptr;

	/*
	 * initialize the malloc sub-system.
	 */
	MALLOC_INIT();

	/*
	 * IF malloc chain checking is on, go do it.
	 */
	if( malloc_opts & MOPT_CKCHAIN )
	{
		VOIDCAST DBFmalloc_chain_check(func,file,line,1);
	}

	/*
	 * verify that cptr is within the malloc region and that it is on
	 * the correct alignment
	 */
	if(        (cptr < malloc_data_start)
		|| (cptr > malloc_data_end)
		|| ((((long)cptr) & malloc_round) != 0)  )
	{
		malloc_errno = M_CODE_BAD_PTR;
		malloc_warning(func,file,line,(struct mlist *)NULL);
		return( (SIZETYPE) -1);
	}

	/* 
	 * convert pointer to mlist struct pointer.  To do this we must 
	 * move the pointer backwards the correct number of bytes...
	 */
	ptr = DATATOMLIST(cptr);

	/*
	 * check the magic number 
	 */	
	if( (ptr->flag&M_MAGIC_BITS) != M_MAGIC )
	{
		malloc_errno = M_CODE_BAD_MAGIC;
		malloc_warning(func,file,line,(struct mlist *)NULL);
		return((SIZETYPE) -1);
	}

	/*
	 * if this segment is not flagged as being in use
	 */
	if( ! (ptr->flag & M_INUSE) )
	{
		malloc_errno = M_CODE_NOT_INUSE;
		malloc_warning(func,file,line,ptr);
		return( (SIZETYPE) -1 );
	}

	/*
	 * check to see that the pointers are still connected
	 */
 	if( (ptr->prev && (ptr->prev->next != ptr) ) ||
	    (ptr->next && (ptr->next->prev != ptr) ) ||
	    ((ptr->next == NULL) && (ptr->prev == NULL)) )
	{
		malloc_errno = M_CODE_BAD_CONNECT;
		malloc_warning(func,file,line,ptr);
		return( (SIZETYPE) -1 );
	}

	/*
	 * check fill regions for overflow
	 */
	VOIDCAST FILLCHECK(func,file,line,ptr,SHOWERRORS);

	return(ptr->r_size);

} /* DBmalloc_size(... */

/*
 * $Log: size.c,v $
 * Revision 1.1  1996/06/18 03:29:26  warren
 * Added debug malloc package.
 *
 * Revision 1.4  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.3  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.2  1992/07/02  15:35:52  cpcahil
 * misc cleanups for PL13
 *
 * Revision 1.1  1992/07/02  13:49:54  cpcahil
 * added support for new malloc_size function and additional tests to testerr
 *
 */
