
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
 * Function:	free()
 *
 * Purpose:	to deallocate malloced data
 *
 * Arguments:	ptr	- pointer to data area to deallocate
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *		verify pointer is within malloc region
 *		get mlist pointer from passed address
 *		verify magic number
 *		verify inuse flag
 *		verify pointer connections with surrounding segments
 *		turn off inuse flag
 *		verify no data overrun into non-malloced area at end of segment
 *		IF possible join segment with next segment
 *		IF possible join segment with previous segment
 *		Clear all data in segment (to make sure it isn't reused)
 *
 */
#ifndef lint
static
char rcs_hdr[] = "$Id: free.c,v 1.1 1996/06/18 03:29:16 warren Exp $";
#endif

FREETYPE
unlocked_free(cptr)
	DATATYPE	* cptr;
{
	debug_free((char *)NULL, 0, cptr);
}

FREETYPE
debug_free(file,line,cptr)
	CONST char	* file;
	int		  line;
	DATATYPE	* cptr;
{
	static IDTYPE	  counter;

	counter++;

	DBFfree("free",F_T_FREE,counter,file,line,cptr);
}

FREETYPE
DBFfree(func,type,counter,file,line,cptr)
	CONST char	* func;
	int		  type;
	IDTYPE		  counter;
	CONST char	* file;
	int		  line;
	DATATYPE	* cptr;
{
	register struct mlist	* oldptr;
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

#if defined(ANSI_NULLS) || (__STDC__ && ! defined(NO_ANSI_NULLS))

	if( cptr == NULL )
	{
		return;
	}

#else

	if( (cptr == NULL) && (type == F_T_XTFREE) )
	{
		return;
	}

#endif

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
		return;
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
		return;
	}

	/*
	 * if this segment is not flagged as being in use
	 */
	if( ! (ptr->flag & M_INUSE) )
	{
		malloc_errno = M_CODE_NOT_INUSE;
		malloc_warning(func,file,line,ptr);
		return;
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
		return;
	}

	/*
	 * check fill regions for overflow
	 */
	VOIDCAST FILLCHECK(func,file,line,ptr,SHOWERRORS);

	/*
	 * if this block has been marked and we are warning about marked frees
	 * give the user a warning about the free.
	 */
	if( ((malloc_opts&MOPT_FREEMARK) != 0) && ((ptr->flag & M_MARKED) != 0))
	{
		malloc_errno = M_CODE_FREEMARK;
		malloc_warning(func,file,line,ptr);
	}

	/*
	 * flag block as freed
	 */
	ptr->flag &= ~M_INUSE;

	DEBUG3(10,"pointers: prev: 0x%.7x,  ptr: 0x%.7x, next: 0x%.7x",
			ptr->prev, ptr, ptr->next);
	
	DEBUG3(10,"size:     prev: %9d,  ptr: %9d, next: %9d",
			ptr->prev->s.size, ptr->s.size, ptr->next->s.size);
	
	DEBUG3(10,"flags:    prev: 0x%.7x,  ptr: 0x%.7x, next: 0x%.7x",
			ptr->prev->flag, ptr->flag, ptr->next->flag);
	
	/*
	 * identify where this section was freed
	 */
	ptr->ffile     = file;
	ptr->fline     = line;
	ptr->fid       = counter;
	ptr->freestack = StackCurrent();
	SETFTYPE(ptr,type);
	
	/*
	 * Fill in the freed segment
	 */
	FILLDATA(ptr,FILL_FREE,0,(struct mlist *)NULL);

	/*
	 * if we are reusing code
	 */
	if( malloc_opts & MOPT_REUSE  )
	{
		/*
		 * check to see if this block can be combined with the next
		 * and/or previous block.  Since it may be joined with the
	 	 * previous block we will save a pointer to the previous
		 * block and test to verify if it is joined (it's next ptr
		 * will no longer point to ptr).
 		 */
		malloc_join(ptr,ptr->next,NOTINUSE,DOFILL);

		oldptr = ptr->prev;

		malloc_join(ptr->prev, ptr,NOTINUSE,DOFILL);

		if( oldptr->next != ptr )
		{
			DEBUG0(10,"Oldptr was changed");
			ptr = oldptr;
		}

		/*
		 * else, since the oldptr did not change, ptr is now a new free
		 * segment that must be added to the free list, so go do it.
		 */
		else
		{
			malloc_freeseg(M_FREE_ADD,ptr);
		}
	}

} /* DBFfree(... */

/*
 * $Log: free.c,v $
 * Revision 1.1  1996/06/18 03:29:16  warren
 * Added debug malloc package.
 *
 * Revision 1.29  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.28  1992/07/12  15:30:58  cpcahil
 * Merged in Jonathan I Kamens' changes
 *
 * Revision 1.27  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.26  1992/07/02  13:49:54  cpcahil
 * added support for new malloc_size function and additional tests to testerr
 *
 * Revision 1.25  1992/05/14  23:02:27  cpcahil
 * added support for ANSI NULL behavior even with non-ansi compilers (if
 * chosen at compile time).
 *
 * Revision 1.24  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.23  1992/04/24  11:18:52  cpcahil
 * Fixes from Denny Page and Better integration of Xt alloc hooks
 *
 * Revision 1.22  1992/04/22  18:17:32  cpcahil
 * added support for Xt Alloc functions, linted code
 *
 * Revision 1.21  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.20  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.19  1992/02/19  01:41:35  cpcahil
 * added check for alignment on the free'd pointer.
 *
 * Revision 1.18  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.17  1992/01/28  14:30:18  cpcahil
 * Changes from the c.s.r second review
 *
 * Revision 1.16  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.15  1991/12/06  17:58:44  cpcahil
 * added cfree() for compatibility with some wierd systems
 *
 * Revision 1.14  91/12/06  08:54:17  cpcahil
 * cleanup of __STDC__ usage and addition of CHANGES file
 * 
 * Revision 1.13  91/12/04  09:23:37  cpcahil
 * several performance enhancements including addition of free list
 * 
 * Revision 1.12  91/12/02  19:10:09  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.11  91/11/25  14:41:53  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.10  91/11/24  00:49:25  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.9  90/08/29  21:22:48  cpcahil
 * miscellaneous lint fixes
 * 
 * Revision 1.8  90/05/11  00:13:08  cpcahil
 * added copyright statment
 * 
 * Revision 1.7  90/02/25  11:00:18  cpcahil
 * added support for malloc chain checking.
 * 
 * Revision 1.6  90/02/24  21:50:18  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.5  90/02/24  17:29:13  cpcahil
 * changed $Header to $Id so full path wouldnt be included as part of rcs 
 * id string
 * 
 * Revision 1.4  90/02/24  15:15:32  cpcahil
 * 1. changed ALREADY_FREE errno to NOT_INUSE so that the same errno could
 *    be used by both free and realloc (since it was the same error).
 * 2. fixed coding bug
 * 
 * Revision 1.3  90/02/24  14:23:45  cpcahil
 * fixed malloc_warning calls
 * 
 * Revision 1.2  90/02/24  13:59:10  cpcahil
 * added function header.
 * Modified calls to malloc_warning/malloc_fatal to use new code error messages
 * Added support for malloc_errno setting of error codes.
 * 
 * Revision 1.1  90/02/22  23:17:43  cpcahil
 * Initial revision
 * 
 */
