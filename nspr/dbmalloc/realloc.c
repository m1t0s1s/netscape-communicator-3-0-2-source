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
char rcs_hdr[] = "$Id: realloc.c,v 1.1 1996/06/18 03:29:26 warren Exp $";
#endif

#include <stdio.h>

#include "mallocin.h"

DATATYPE *
unlocked_realloc(cptr,size)
	DATATYPE		* cptr;
	SIZETYPE		  size;
{
	return( debug_realloc(NULL,-1,cptr,size) );
}

DATATYPE *
debug_realloc(file,line,cptr,size)
	CONST char		* file;
	int			  line;
	DATATYPE		* cptr;
	SIZETYPE		  size;
{
	static IDTYPE		  call_counter;

	/*
	 * increment the call counter
	 */
	call_counter++;

	return( DBFrealloc("realloc",M_T_REALLOC,call_counter,
			  file,line,cptr,size) );

}

/*
 * Function:	DBFrealloc()
 *
 * Purpose:	to re-allocate a data area.
 *
 * Arguments:	cptr	- pointer to area to reallocate
 *		size	- size to change area to
 *
 * Returns:	pointer to new area (may be same area)
 *
 * Narrative:	verify pointer is within malloc region
 *		obtain mlist pointer from cptr
 *		verify magic number is correct
 *		verify inuse flag is set
 *		verify connection to adjoining segments is correct
 *		save requested size
 *		round-up size to appropriate boundry
 *		IF size is bigger than what is in this segment
 *		    try to join next segment to this segment
 *		IF size is less than what is is this segment
 *		    determine leftover amount of space
 *		ELSE
 *		    allocate new segment of size bites
 *		    IF allocation failed
 *		        return NULL
 *		    copy previous data to new segment
 *		    free previous segment
 *		    return new pointer
 *		split of extra space in this segment (if any)
 *		clear bytes beyound what they had before
 *		return pointer to data 
 */

DATATYPE *
DBFrealloc(func,type,call_counter,file,line,cptr,size)
	CONST char		* func;
	int			  type;
	IDTYPE			  call_counter;
	CONST char		* file;
	int			  line;
	DATATYPE		* cptr;
	SIZETYPE		  size;
{
	SIZETYPE		  i;
	char			* new_cptr;
	int			  marked;
	SIZETYPE		  need;
	struct mlist		* optr;
	struct mlist		* ptr;
	SIZETYPE		  r_size;
	SIZETYPE		  start;

	MALLOC_INIT();

	/*
	 * IF malloc chain checking is on, go do it.
	 */
	if( malloc_opts & MOPT_CKCHAIN )
	{
		VOIDCAST DBFmalloc_chain_check(func,file,line,1);
	}

	/*
	 * if the user wants to be warned about zero length mallocs, do so
	 */
	if( ((malloc_opts & MOPT_ZERO) != 0) && (size == 0) )
	{
		malloc_errno = M_CODE_ZERO_ALLOC;
		malloc_warning(func,file,line,(struct mlist *)NULL);
	}

	/*
	 * if this is an ansi-c compiler and we want to use the realloc(0) 
	 * paradigm, or if this is a call from xtrealloc, then if the 
	 * pointer is a null, act as if this is a call to malloc.
	 */
#if defined(ANSI_NULLS) || (__STDC__ && ! defined(NO_ANSI_NULLS))
	if( cptr == NULL )
#else
	if( (cptr == NULL) && (type == M_T_XTREALLOC) )
#endif
	{
		/*
		 * allocate the new chunk
		 */
		new_cptr = DBFmalloc(func,type,call_counter,file,line,size);

		return(new_cptr);
	}

	/*
	 * verify that cptr is within the malloc region...
	 */
	if(    (cptr < malloc_data_start)
	    || (cptr > malloc_data_end) 
	    || ((((long)cptr) & malloc_round) != 0 ) )
	{
		malloc_errno = M_CODE_BAD_PTR;
		malloc_warning(func,file,line,(struct mlist *)NULL);
		return (NULL);
	}

	/* 
	 * convert pointer to mlist struct pointer.  To do this we must 
	 * move the pointer backwards the correct number of bytes...
	 */
	
	ptr = (struct mlist *) (((char *)cptr) - M_SIZE);
	
	if( (ptr->flag&M_MAGIC_BITS) != M_MAGIC )
	{
		malloc_errno = M_CODE_BAD_MAGIC;
		malloc_warning(func,file,line,(struct mlist *)NULL);
		return(NULL);
	}

	if( ! (ptr->flag & M_INUSE) )
	{
		malloc_errno = M_CODE_NOT_INUSE ;
		malloc_warning(func,file,line,ptr);
		return(NULL);
	}

 	if( (ptr->prev && (ptr->prev->next != ptr) ) ||
	    (ptr->next && (ptr->next->prev != ptr) ) ||
	    ((ptr->next == NULL) && (ptr->prev == NULL)) )
	{
		malloc_errno = M_CODE_BAD_CONNECT;
		malloc_warning(func,file,line,ptr);
		return(NULL);
	}

	/*
	 * save the marked status
	 */
	marked = ptr->flag & M_MARKED;

	/*
	 * save the requested size;
	 */
	r_size = size;

	/*
	 * make sure we have the full boundary that is needed
	 */
	size += malloc_boundsize;

	M_ROUNDUP(size);

	if( (size > ptr->s.size) && ((malloc_opts & MOPT_REUSE) != 0) )
	{
		malloc_join(ptr,ptr->next,INUSEOK,DOFILL);
	}

	/*
	 * if we still don't have enough room, and we are at the end of the
	 * malloc chain and we are up against the current sbrk, we can just
 	 * sbrk more room for our data area.
 	 */	
	if(    (size > ptr->s.size)
	    && (ptr == malloc_end) 
	    && ((ptr->data+ptr->s.size) == sbrk(0) ) )
	{
		need = size - ptr->s.size;

		/*
		 * if the need is less than the minimum block size,
		 *     get the minimum block size
	 	 */
		if( need < M_BLOCKSIZE )
		{
			need = M_BLOCKSIZE;
		}
		/*
		 * else if the need is not an even multiple of the block size,
		 *     round it up to an even multiple
		 */
		else if( need & (M_BLOCKSIZE-1) )
		{
			need &= ~(M_BLOCKSIZE-1);
			need += M_BLOCKSIZE;
		}

		/*
		 * get the space from the os
	 	 */
		cptr = sbrk(need);

		/*
		 * if we failed to get the space, tell the user about it
		 */
		if( cptr == (char *) -1 )
		{
			malloc_errno = M_CODE_NOMORE_MEM;
			malloc_fatal(func,file,line, (struct mlist *)NULL);
			return(NULL);
		}

		/*
		 * adjust our segment size (extra space will be split off later
		 */
		start = ptr->s.size;
		ptr->s.size += need;

		/*
		 * we have to act like this was a join of a new segment so
		 * we need to call the fill routine to get it to fill the
		 * new data area.
		 */
		FILLDATA(ptr,FILL_JOIN,start,(struct mlist *)NULL);

		/*
		 * mark our end point
		 */
		malloc_data_end = sbrk((int)0);
	
	}
	
		
	/*
	 * if the size is still too small and the previous segment is free
	 * and it would be big enough it if was joined to the current segment
	 */
	if(    ((malloc_opts & MOPT_CKCHAIN) != 0)
	    && (size > ptr->s.size)
	    && ((ptr->prev->flag & M_INUSE) == 0)
	    && (size < (ptr->s.size + ptr->prev->s.size + M_SIZE) ) )
	{
		/*
		 * save the old pointer
		 */
		optr = ptr;

		/*
		 * move out pointer to the proper area.
		 */
		ptr = ptr->prev;
	
		/*
	 	 * force this pointer to be inuse
		 */
		ptr->flag |= M_INUSE;
		ptr->r_size = ptr->next->r_size;
		
		/*
	 	 * join the two segments
		 */
		malloc_join(ptr, ptr->next, ANY_INUSEOK, DONTFILL);

		/*
		 * remove ptr from the free list
		 */
		malloc_freeseg(M_FREE_REMOVE,ptr);

		/*
		 * copy data from the current space to the new space.  Note
	 	 * that the data areas for this copy will likely overlap.
		 */
		DataMC(ptr->data,optr->data,ptr->r_size);

		/*
		 * note that we don't fill in the areas here.  It will be
		 * filled later
		 */

	}
	if( size > ptr->s.size )
	{
		/*
		 * else we can't combine it, so lets allocate a new chunk,
		 * copy the data and free the old chunk...
		 */
		new_cptr = DBFmalloc(func,type,call_counter,file,line,size);

		if( new_cptr == (char *) 0)
		{
			return(new_cptr);
		}

		if( r_size < ptr->r_size )
		{
			i = r_size;
		}
		else
		{
			i = ptr->r_size;
		}
		in_malloc_code++;
		VOIDCAST memcpy(new_cptr,ptr->data,i);
		in_malloc_code--;

		/*
		 * if the old segment was marked, unmark it and mark the new
		 * segment.
		 */
		if( marked )
		{
			ptr->flag &= ~M_MARKED;
			malloc_mark(new_cptr);
		}

		/*
		 * free the old segment since it is no longer needed.
		 */
		DBFfree("realloc:free",F_T_REALLOC,call_counter,file,line,cptr);

		
		return(new_cptr);

	} /* if( size... */

	/*
	 * save amount of real data in new segment (this will be used in the
	 * memset later) and then save requested size of this segment.
	 */
	if( ptr->r_size < r_size )
	{
		i = ptr->r_size;
	}
	else
	{
		i = r_size;
	}

	ptr->r_size = r_size;

	/*
	 * split off extra free space at end of this segment, if possible...
	 */

	malloc_split(ptr);

	/*
	 * save the id info.
	 */	
	ptr->file      = file;
	ptr->line      = line;
	ptr->id        = call_counter;
	ptr->hist_id   = malloc_hist_id++;
	ptr->stack     = StackCurrent();
	ptr->freestack = NULL;
	SETTYPE(ptr,type);

	/*
	 * fill data and/or boundary areas
	 */
	FILLDATA(ptr,FILL_REALLOC,i, (struct mlist *) NULL);
	
	return(ptr->data);

} /* DBFrealloc(... */


/*
 * $Log: realloc.c,v $
 * Revision 1.1  1996/06/18 03:29:26  warren
 * Added debug malloc package.
 *
 * Revision 1.26  1992/09/03  22:24:33  cpcahil
 * final changes for PL14
 *
 * Revision 1.25  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.24  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.23  1992/05/14  23:02:27  cpcahil
 * added support for ANSI NULL behavior even with non-ansi compilers (if
 * chosen at compile time).
 *
 * Revision 1.22  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.21  1992/05/06  05:37:44  cpcahil
 * added overriding of fill characters and boundary size
 *
 * Revision 1.20  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.19  1992/04/22  18:17:32  cpcahil
 * added support for Xt Alloc functions, linted code
 *
 * Revision 1.18  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.17  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.16  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.15  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.14  1991/12/06  08:54:19  cpcahil
 * cleanup of __STDC__ usage and addition of CHANGES file
 *
 * Revision 1.13  91/12/04  09:23:44  cpcahil
 * several performance enhancements including addition of free list
 * 
 * Revision 1.12  91/12/02  19:10:14  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.11  91/11/25  14:42:05  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.10  91/11/24  00:49:32  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.9  91/11/20  11:54:11  cpcahil
 * interim checkin
 * 
 * Revision 1.8  90/08/29  21:22:52  cpcahil
 * miscellaneous lint fixes
 * 
 * Revision 1.7  90/05/11  00:13:10  cpcahil
 * added copyright statment
 * 
 * Revision 1.6  90/02/25  11:01:20  cpcahil
 * added support for malloc chain checking.
 * 
 * Revision 1.5  90/02/24  21:50:31  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.4  90/02/24  17:29:39  cpcahil
 * changed $Header to $Id so full path wouldnt be included as part of rcs 
 * id string
 * 
 * Revision 1.3  90/02/24  17:20:00  cpcahil
 * attempt to get rid of full path in rcs header.
 * 
 * Revision 1.2  90/02/24  15:14:20  cpcahil
 * 1. added function header 
 * 2. changed calls to malloc_warning to conform to new usage
 * 3. added setting of malloc_errno
 *  4. broke up bad pointer determination so that errno's would be more
 *    descriptive
 * 
 * Revision 1.1  90/02/22  23:17:43  cpcahil
 * Initial revision
 * 
 */
