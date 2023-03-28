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
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>

#include "sysdefs.h"

#ifdef NEED_WAIT
#include <sys/wait.h>
#endif

/* 
 * make sure mallocin.h doesn't include sys/types.h since we have already
 * included it.
 */
#ifndef SYS_TYPES_H_INCLUDED
#define SYS_TYPES_H_INCLUDED 1
#endif

#include "mallocin.h"
#include "tostring.h"

#include "debug.h"

#ifndef lint
static char rcs_hdr[] = "$Id: malloc.c,v 1.1 1996/06/18 03:29:19 warren Exp $";
#endif

int		  in_malloc_code;
SIZETYPE	  malloc_align;
int		  malloc_boundsize = M_DFLT_BSIZE;
DATATYPE	* malloc_data_start;
DATATYPE	* malloc_data_end;
struct mlist 	* malloc_end;
int		  malloc_errfd = 2;
int		  malloc_errno;
int		  malloc_fatal_level = M_HANDLE_ABORT;
int		  malloc_fill;
int		  malloc_fillbyte = M_DFLT_FILL;
int		  malloc_freebyte = M_DFLT_FREE_FILL;
struct mlist	* malloc_freelist;
long		  malloc_hist_id;
int		  malloc_opts = MOPT_MFILL  | MOPT_FFILL | MOPT_DFILL | 
				MOPT_CKDATA | MOPT_REUSE | MOPT_FREEMARK |
				MOPT_ZERO;
int		  malloc_round = M_RND-1;
struct mlist	  malloc_start;
int		  malloc_warn_level;

/*
 * perform string copy, but make sure that we don't go beyond the end of the
 * buffer (leaving room for trailing info (5 bytes) like the return and a null
 */
#define COPY(s,t,buf,len)	while( (*(s) = *((t)++) ) \
				       && ( (s) < ((buf)+(len)-5) ) ) { (s)++; }

#define DUMP_PTR	0
#define DUMP_NEXT	1
#define DUMP_PREV	2

#define ERRBUFSIZE	1024

/*
 * Function:	malloc()
 *
 * Purpose:	low-level interface to the debug malloc lib.  This should only
 * 		be called from code that is not recompilable.  The recompilable
 *		code should include malloc.h and therefore its calls to malloc
 *		will be #defined to be calls to debug_malloc with the
 *		appropriate arguments.
 *
 * Arguments:	size	- size of data area needed
 *
 * Returns:	whatever debug_malloc returns.
 *
 * Narrative:
 *
 */
DATATYPE *
unlocked_malloc(size)
	SIZETYPE	  size;
{
	return( debug_malloc(NULL,-1,size) );
}

/*
 * Function:	debug_malloc()
 *
 * Purpose:	the real memory allocator
 *
 * Arguments:	size	- size of data area needed
 *
 * Returns:	pointer to allocated area, or NULL if unable
 *		to allocate addtional data.
 *
 * Narrative:
 *
 */
DATATYPE *
debug_malloc(file,line,size)
	CONST char	* file;
	int		  line;
	SIZETYPE	  size;
{
	static IDTYPE	  call_counter;

	/*
	 * increment the counter for the number of calls to this func.
	 */
	call_counter++;

	return( DBFmalloc("malloc",M_T_MALLOC,call_counter,file,line,size) );

} /* debug_malloc(... */

char *
DBFmalloc(func,type,call_counter,file,line,size)
	CONST char		* func;
	int			  type;
	IDTYPE			  call_counter;
	CONST char		* file;
	int			  line;
	SIZETYPE		  size;
{
	char			* cptr;
	SIZETYPE		  fill;
	SIZETYPE		  fit = 0;
	SIZETYPE		  lastfit;
	struct mlist		* lastptr;
	SIZETYPE		  need;
	int			  newmalloc = 0;
	register struct mlist	* oldptr;
	register struct mlist	* ptr;
	SIZETYPE		  r_size;

	MALLOC_INIT();

	/*
	 * If malloc chain checking is on, go do it.
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
		malloc_warning(func,file,line,(struct mlist *) NULL);
	}

	/*
	 * save the original requested size;
	 */
	r_size = size;

	/*
	 * always make sure there is at least on extra byte in the malloc
	 * area so that we can verify that the user does not overrun the
	 * data area.
	 */
	size += malloc_boundsize;

	/*
	 * Now look for a free area of memory of size bytes...
	 */
	oldptr = NULL;
	ptr = malloc_freelist;

	/*
	 * if low fragmentation has been turned on, look for a best fit
	 * segment (otherwise, for performance reasons, we will look for
	 * a first fit)
	 */
	if( malloc_opts & MOPT_LOWFRAG )
	{  
		lastfit = -1;
		lastptr = NULL;
		/*
		 * until we run out of segments, or find an exact match
		 */
		for(; (ptr != NULL) && (lastfit != 0); ptr=ptr->freenext)
		{
			/*
			 * quick check to make sure our free-list hasn't been
			 * broken.
			 */
			if((oldptr != NULL) && (ptr->freeprev != oldptr) )
			{
				malloc_errno = M_CODE_CHAIN_BROKE;
				malloc_fatal(func,file,line,oldptr);
				return(NULL);
			}

			/*
			 * if this is to be an aligned segment and the pointer
			 * is not already aligned on the correct boundry
			 */
			if(    (type == M_T_ALIGNED)
			    && ((((SIZETYPE)ptr->data)&(malloc_align-1)) != 0)	
			    && (ptr->s.size > size ) )
			{
				fit = AlignedFit(ptr,malloc_align,size);
			}
			/*
			 * else if this segment is big enough for our needs
			 */
			else if( ptr->s.size > size )
			{
				/*
			 	 * calculate how close we fit. 
				 */
				fit = ptr->s.size - size;
			}
			else
			{
				fit = -1;
			}

			if( fit == 0 )
			{
				break;
			}
			else if( fit > 0 )
			{	
				/*
			 	 * if this is the first one we fit int, or
				 * if this a closer fit
			 	 */
				if( (lastfit == -1) || (fit < lastfit) )
				{
					lastptr = ptr;
					lastfit = fit;
				}
			}
			oldptr = ptr;

		} /* for(... */

		/*
		 * if we found an entry, use it
		 */
		if( (fit != 0) && (lastptr != NULL) )
		{
			ptr = lastptr;
		}
	}
	else
	{
		/*
		 * until we run out of free entries, or find an entry that has
		 * enough room
		 */
		for( ; (ptr != NULL) ; ptr=ptr->freenext)
		{
			if((oldptr != NULL) && (ptr->freeprev != oldptr) )
			{
				malloc_errno = M_CODE_CHAIN_BROKE;
				malloc_fatal(func,file,line,oldptr);
				return(NULL);
			}

			/*
			 * if this is an aligned request and it isn't already
			 * aligned and it appears to be big enough
			 */
			if(    (type == M_T_ALIGNED)
			    && ((((SIZETYPE)ptr->data)&(malloc_align-1)) != 0)	
			    && (ptr->s.size >= size ) )
			{
				/*
				 * if an aligned segment does fit within the
				 * specified segment, break out of loop
				 */
				if( AlignedFit(ptr,malloc_align,size) >= 0)
				{
					break;
				}
			}
			/*
			 * else if this segment is big enough
			 */
			else if( ptr->s.size >= size )
			{
				break;
			}

			oldptr = ptr;

		} /* for(... */
	}

	/*
	 * If ptr is null, we have run out of memory and must sbrk more
	 */
	if( ptr == NULL )
	{
		/*
		 * calculate how much space we need.  If they ask for more
		 * than 100k, get exactly what they want.  Otherwise get
		 * twice what they ask for so that we are ready for the
		 * next allocation they ask for
		 */
		need = size + sizeof(struct mlist);
		if( size < 100*1024 )
		{
			need <<= 1;
		}

		/*
		 * figure out any additional bytes we might need in order to
		 * obtain the correct alignment
		 */
		cptr = sbrk(0);

		fill = ((SIZETYPE)cptr) & (M_RND-1);
		if( fill )
		{
			fill = M_RND - fill;
		}

		/*
		 * if this is an aligned request, then make sure we have 
		 * enough room for the aligment 
		 */
		if( type == M_T_ALIGNED )
		{
			cptr += fill + M_SIZE;

			/*
			 * if this new value will not be on a valid 
			 * alignment, add malloc_align + sizeof(struct mlist)
			 * to the amount of space that we need to make sure 
			 * that we have enough for the aligned allocation.
			 */
			if( (((SIZETYPE)cptr) & (malloc_align-1)) != 0 )
			{
				need += malloc_align + sizeof(struct mlist);
			}
		}

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
		 * add in that fill space we determined before so that after
		 * this allocation, future allocations should be alligned
		 * appropriately
	 	 */
		need += fill;

		/*
		 * get the space from the os
	 	 */
		cptr = sbrk((int)need);

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
		 * make sure that the pointer returned by sbrk is on the
		 * correct alignment boundry.
		 */
		DEBUG2(10,"sbrk() returned 0x%lx, skipping %d bytes to align",
			cptr, fill);
		ptr = (struct mlist *) (cptr + fill);

		/*
		 * readjust the amount of space we obtained to reflect the fill
		 * area we are skipping.
		 */
		need -= fill;
	
		/*
		 * mark our end point
		 */
		malloc_data_end = sbrk((int)0);

		/*
		 * hook the new segment into the malloc chain
		 */
		InitMlist(ptr,M_T_SPLIT);
		ptr->prev     = malloc_end;
		ptr->s.size   = need - M_SIZE;
		ptr->r_size   = ptr->s.size;
	
		malloc_end->next = ptr;
		malloc_end = ptr;

		newmalloc = 1;

	} /* if( ptr ==... */
	/*
	 * else we found a free slot that has enough room,
	 *    so take it off the free list
	 */
	else
	{
		/*
		 * if this module is already in use
		 */
		if( (ptr->flag & M_INUSE) != 0 )
		{
			malloc_errno = M_CODE_FREELIST_BAD;
			malloc_fatal(func,file,line,ptr);
			return(NULL);
		}
		
	}
		

	/*
	 * Now ptr points to a memory location that can store
	 * this data, so lets go to work.
	 */

	/*
	 * if this is an aligned request and the data is not already on the
	 * correct alignment
	 */
	if(    (type == M_T_ALIGNED)
	    && ((((SIZETYPE)(ptr->data)) & (malloc_align-1)) != 0) )
	{
		/*
		 * if this is a new allocation, we have to check to see if
		 * the new segment can be joined to the previous segment 
		 * before we pull out the aligned segment.  This is required
		 * since the previous segment may be free and we would end
		 * up with two adjacent free segments if we didn't do this.
		 */
		if( newmalloc )
		{
			/*
			 * if there is a previous segment
			 */
			if( (oldptr = ptr->prev) != NULL )
			{
				/*
				 * attempt to join the previous segment with
				 * the new segment (if the prev seg is free).
				 */
				malloc_join(ptr->prev,ptr, NOTINUSE, DOFILL);

				/*
				 * if they were joined, use the new pointer.
				 */
				if( oldptr->next != ptr )
				{
					ptr = oldptr;
				}
			}

			/*
			 * if this segment did not get joined to a previous
			 * segment, add the new segment to the free list.
			 */
			if( oldptr != ptr )
			{
				malloc_freeseg(M_FREE_ADD, ptr);
			}
		}

		/*
		 * break out the new segment.  Note that AlignedMakeSeg depends
		 * upon the fact that ptr points to a segment that will have
		 * enough room for the alignment within the segment.
		 */
		ptr = AlignedMakeSeg(ptr,malloc_align);
	}
	/*
	 * else we are going to use the indicated segment, so let's remove it
	 * from the free list.
	 */
	else if( ! newmalloc )
	{
		/*
		 * remove the segment from the free list
		 */
		malloc_freeseg(M_FREE_REMOVE,ptr);
	}

	/*
	 * for the purpose of splitting, have the requested size appear to be
	 * the same as the actualsize
	 */
	ptr->r_size = size;		/* save requested size	*/

	/*
	 * split off unneeded data area in this block, if possible...
	 */
	malloc_split(ptr);

	/*
	 * set inuse flag after split, so that the split routine knows that
	 * this module can see that the module was free and therefore won't 
	 * re-fill it if not necessary.
	 */
	ptr->flag |= M_INUSE;

	/*
	 * save the real requested size
	 */
	ptr->r_size = r_size;

	/*
	 * fill in the buffer areas (and possibly the data area) in
	 * order to track underflow, overflow, and uninitialized use
	 */
	FILLDATA(ptr, FILL_MALLOC, (SIZETYPE)0, (struct mlist *) NULL);

	/*
	 * store the identification information
	 */
	ptr->file      = file;
	ptr->line      = line;
	ptr->id        = call_counter;
	ptr->hist_id   = malloc_hist_id++;
	ptr->stack     = StackCurrent();
	ptr->freestack = NULL;
	ptr->freenext  = NULL;
	ptr->freeprev  = NULL;

	SETTYPE(ptr,type);

	/*
	 * return the pointer to the data area for the user.
	 */
	return( ptr->data );


} /* DBFmalloc(... */

/*
 * Function:	malloc_split()
 *
 * Purpose:	to split a malloc segment if there is enough room at the
 *		end of the segment that isn't being used
 *
 * Arguments:	ptr	- pointer to segment to split
 *
 * Returns:	nothing of any use.
 *
 * Narrative:
 *		get the needed size of the module
 * 		round the size up to appropriat boundry
 *		calculate amount of left over space
 *		if there is enough left over space
 *		    create new malloc block out of remainder
 *		    if next block is free 
 *			join the two blocks together
 *		    fill new empty block with free space filler
 * 		    re-adjust pointers and size of current malloc block
 *		
 *		
 *
 * Mod History:	
 *   90/01/27	cpcahil		Initial revision.
 */
VOIDTYPE
malloc_split(ptr)
	struct mlist		* ptr;
{
	SIZETYPE		  rest;
	SIZETYPE		  size;
	struct mlist		* tptr;
	static int		  call_counter;

	size = ptr->r_size;

	/*
	 * roundup size to the appropriate boundry
	 */
	M_ROUNDUP(size);

	/*
	 * figure out how much room is left in the array.
	 * if there is enough room, create a new mlist 
	 *     structure there.
	 */
	if( ptr->s.size > size )
	{
		rest = ptr->s.size - size;
	}
	else
	{
		rest = 0;
	}

	/*
	 * if there is at least enough room to create another malloc block
	 */
	if( rest > sizeof(struct mlist) )
	{
		tptr = (struct mlist *) (ptr->data+size);
		tptr->file  = NULL;
		tptr->ffile  = NULL;
		tptr->stack = NULL;
		tptr->prev  = ptr;
		tptr->next  = ptr->next;
		tptr->flag  = M_MAGIC;
		SETTYPE(tptr,M_T_SPLIT);
		tptr->s.size = rest - M_SIZE;
		tptr->r_size = tptr->s.size;
		tptr->freenext = NULL;
		tptr->freeprev = NULL;
		tptr->id    = ++call_counter;

		/*
		 * if this area is not already free-filled
		 */
		if( ((ptr->flag& M_INUSE) != 0) || ((ptr->flag&M_FILLED) == 0) )
		{
			/*
			 * fill freed areas
			 */
			FILLDATA(tptr,FILL_SPLIT,(SIZETYPE)0,
					(struct mlist *) NULL);
		}
		else if( (ptr->flag&M_INUSE) == 0 )
		{
			/*
			 * since it was already free-filled, just set the flag
			 */
			tptr->flag |= M_FILLED;
		}

		/*
		 * If possible, join this segment with the next one
		 */
		malloc_join(tptr, tptr->next,NOTINUSE,DOFILL);

		if( tptr->next )
		{
			tptr->next->prev = tptr;
		}

		ptr->next = tptr;
		ptr->s.size = size;

		if( malloc_end == ptr )
		{
			malloc_end = tptr;
		}

		/*
		 * add the new segment to the free list
		 */
		malloc_freeseg(M_FREE_ADD,tptr);

	}

} /* malloc_split(... */

/*
 * Function:	malloc_join()
 *
 * Purpose:	to join two malloc segments together (if possible)
 *
 * Arguments:	ptr	- pointer to segment to join to.
 * 		nextptr	- pointer to next segment to join to ptr.
 *
 * Returns:	nothing of any values.
 *
 * Narrative:
 *
 * Mod History:	
 *   90/01/27	cpcahil		Initial revision.
 */
VOIDTYPE
malloc_join(ptr,nextptr, inuse_override, fill_flag)
	struct mlist	* ptr;
	struct mlist	* nextptr;
	int		  inuse_override;
	int		  fill_flag;
{
	SIZETYPE	  newsize;
	SIZETYPE	  start;

	/*
	 * if      the current segment exists
	 *    AND  it is not inuse (or if we don't care that it is inuse)
 	 *    AND  the next segment exits
	 *    AND  it is not in use
	 *    AND  it is adjacent to this segment
	 *    THEN we can join the two together
	 */
	if(    ( ptr != NULL )
	    && ((inuse_override & INUSEOK) || ! (ptr->flag & M_INUSE))
	    && ( nextptr != NULL )
	    && ((inuse_override&NEXTPTR_INUSEOK) || !(nextptr->flag & M_INUSE))
	    && ((ptr->data+ptr->s.size) == (char *) nextptr) )
	{
		/*
		 * remove nextptr from the freelist
		 */
		malloc_freeseg(M_FREE_REMOVE,nextptr);

		/*
		 * if the segment was the end pointer
		 */
		if( malloc_end == nextptr )
		{
			malloc_end = ptr;
		}
		ptr->next = nextptr->next;
		newsize = nextptr->s.size + M_SIZE;

		start = ptr->s.size;

		ptr->s.size += newsize;
		if( ptr->next )
		{
			ptr->next->prev = ptr;
		}

		/*
		 * if the segment is free, set the requested size to include
		 * the requested size within the next segment.
		 */
		if( ! (ptr->flag & M_INUSE) )
		{
			ptr->r_size = start + M_SIZE + nextptr->r_size;
		}

		/*
		 * if we should fill in the segment
	 	 */
		if( fill_flag )
		{
			FILLDATA(ptr,FILL_JOIN,start,nextptr);
		}

	}

} /* malloc_join(... */


/*
 * The following mess is just to ensure that the versions of these functions in
 * the current library are included (to make sure that we don't accidentaly get 
 * the libc versions. (This is the lazy man's -u ld directive)
 */

VOIDTYPE	(*malloc_void_funcs[])() =
{
	free,
};

int		(*malloc_int_funcs[])() =
{
	strcmp,
	memcmp,
};

DATATYPE	* (*malloc_char_star_funcs[])() =
{
	_malloc,
	debug_realloc,
	debug_calloc,
};

/*
 * Function:	malloc_fatal()
 *
 * Purpose:	to display fatal error message and take approrpriate action
 *
 * Arguments:	funcname - name of function calling this routine
 *		mptr	 - pointer to malloc block associated with error
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *
 * Notes:	This routine does not make use of any libc functions to build
 *		and/or disply the error message.  This is due to the fact that
 *		we are probably at a point where malloc is having a real problem
 *		and we don't want to call any function that may use malloc.
 */
VOIDTYPE
malloc_fatal(funcname,file,line,mptr)
	CONST char		* funcname;
	CONST char		* file;
	int			  line;
	CONST struct mlist	* mptr;
{
	char		  errbuf[ERRBUFSIZE];
	char		* s;
	CONST char	* t;

	s = errbuf;
	t = "MALLOC Fatal error from ";
	COPY(s,t,errbuf,ERRBUFSIZE);

	t = funcname;
	COPY(s,t,errbuf,ERRBUFSIZE);

	t = "()";
	COPY(s,t,errbuf,ERRBUFSIZE);

	/*
	 * if we have a file and line number, show it
 	 */
	if( file != NULL )
	{
		t = " (called from ";
		COPY(s,t,errbuf,ERRBUFSIZE);
		
		t = file;
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = " line ";
		COPY(s,t,errbuf,ERRBUFSIZE);

		s += tostring(s,(ULONG)line,0,10,' ');

		*s++ = ')';

	}

	*s++ = ':';
	*s++ = '\n';

	t = malloc_err_strings[malloc_errno];
	COPY(s,t,errbuf,ERRBUFSIZE);

	*(s++) = '\n';

	if( write(malloc_errfd,errbuf,(WRTSIZE)(s-errbuf)) != (s-errbuf))
	{
		VOIDCAST write(2,"I/O error to error file\n",(WRTSIZE)24);
		exit(110);
	}

	/*
	 * if this error was associated with an identified malloc block,
	 * dump the malloc info for the block.
	 */
	if( mptr )
	{
		malloc_dump_info_block(mptr,DUMP_PTR);
	}

	malloc_err_handler(malloc_fatal_level);

} /* malloc_fatal(... */

/*
 * Function:	malloc_warning()
 *
 * Purpose:	to display warning error message and take approrpriate action
 *
 * Arguments:	funcname - name of function calling this routine
 *		mptr	 - pointer to malloc block associated with error
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *
 * Notes:	This routine does not make use of any libc functions to build
 *		and/or disply the error message.  This is due to the fact that
 *		we are probably at a point where malloc is having a real problem
 *		and we don't want to call any function that may use malloc.
 */
VOIDTYPE
malloc_warning(funcname,file,line,mptr)
	CONST char		* funcname;
	CONST char		* file;
	int			  line;
	CONST struct mlist	* mptr;
{
	char		  errbuf[ERRBUFSIZE];
	char		* s;
	CONST char	* t;

	s = errbuf;
	t = "MALLOC Warning from ";
	COPY(s,t,errbuf,ERRBUFSIZE);

	t = funcname;
	COPY(s,t,errbuf,ERRBUFSIZE);

	t = "()";
	COPY(s,t,errbuf,ERRBUFSIZE);

	/*
	 * if we have a file and line number, show it
 	 */
	if( file != NULL )
	{
		t = " (called from ";
		COPY(s,t,errbuf,ERRBUFSIZE);
		
		t = file;
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = " line ";
		COPY(s,t,errbuf,ERRBUFSIZE);

		s += tostring(s,(ULONG)line,0,10,' ');

		*s++ = ')';

	}

	*s++ = ':';
	*s++ = '\n';

	t = malloc_err_strings[malloc_errno];
	COPY(s,t,errbuf,ERRBUFSIZE);

	*(s++) = '\n';

	if( write(malloc_errfd,errbuf,(WRTSIZE)(s-errbuf)) != (s-errbuf))
	{
		VOIDCAST write(2,"I/O error to error file\n",(WRTSIZE)24);
		exit(110);
	}

	/*
	 * if this error was associated with an identified malloc block,
	 * dump the malloc info for the block.
	 */
	if( mptr )
	{
		malloc_dump_info_block(mptr,DUMP_PTR);
	}

		
	malloc_err_handler(malloc_warn_level);

} /* malloc_warning(... */

/*
 * Function:	malloc_dump_info_block()
 *
 * Purpose:	to display identifying information on an offending malloc
 *		block to help point the user in the right direction
 *
 * Arguments:	mptr - pointer to malloc block
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *
 * Notes:	This routine does not make use of any libc functions to build
 *		and/or disply the error message.  This is due to the fact that
 *		we are probably at a point where malloc is having a real
 *		problem and we don't want to call any function that may use
 * 		malloc.
 */
VOIDTYPE
malloc_dump_info_block(mptr,id)
	CONST struct mlist	* mptr;
	int			  id;
{
	char		  errbuf[ERRBUFSIZE];
	CONST char	* funcname;
	char		* s;
	CONST char	* t;

	/*
	 * if the mlist struct does not have a valid magic number, skip it
	 * because we probably have gotten clobbered.
	 */
	if( (mptr->flag&M_MAGIC_BITS) != M_MAGIC )
	{
		return;
	}

	s = errbuf;

	if( id == DUMP_PTR )
	{
		t = "This error is *probably* associated with the following";
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = " allocation:\n\n\tA call to ";
	}
	else if( id == DUMP_PREV )
	{
		if( mptr == NULL || (mptr == &malloc_start) )
		{
			t = "\tThere is no malloc chain element prior to the";
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = " suspect\n\t element identified above";
		}
		else
		{
			t = "\tThe malloc chain element prior to the suspect";
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = " allocation is from:\n\n\tA call to ";
		}
	}
	else
	{
		if( (mptr == NULL) ||
		    ((mptr == malloc_end) && (GETTYPE(mptr) == M_T_SPLIT)) )
		{
			t = "\tThere is no malloc chain element after the";
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = " suspect\n\t element identified above";
		}
		else
		{
			t ="\tThe malloc chain element following the suspect";
			COPY(s,t,errbuf,ERRBUFSIZE);
		
			t = " allocation is from:\n\n\tA call to ";
		}
	}
	COPY(s,t,errbuf,ERRBUFSIZE);

	/*
	 * if this is a real-live malloc block (the starting block and
	 * the last block (if it was generated by a malloc split) don't
	 * count as real blocks since the user never allocated them.
	 */
	if( (mptr != NULL) && (mptr != &malloc_start)  &&
	    ((mptr != malloc_end) || (GETTYPE(mptr) != M_T_SPLIT)) )
	{

		funcname = t = MallocFuncName(mptr);
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = " for ";
		COPY(s,t,errbuf,ERRBUFSIZE);

		s += tostring(s,(ULONG)mptr->r_size,0,10,' ');

		t = " bytes in ";
		COPY(s,t,errbuf,ERRBUFSIZE);

		/*
		 * if we don't have file info
	 	 */
		if( (mptr->file == NULL) || (mptr->file[0] == EOS) )
		{
			t = "an unknown file";
			COPY(s,t,errbuf,ERRBUFSIZE);
		}
		else
		{
			t = mptr->file;
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = " on line ";
			COPY(s,t,errbuf,ERRBUFSIZE);
			
			s += tostring(s,(ULONG)mptr->line,0,10,' ');
		}

		t = ".\n\tThis was the ";
		COPY(s,t,errbuf,ERRBUFSIZE);

		s += tostring(s,(ULONG)mptr->id,0,10,' ');

		t = malloc_int_suffix(mptr->id);
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = " call to ";
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = funcname;
		COPY(s,t,errbuf,ERRBUFSIZE);

		t = ".\n";
		COPY(s,t,errbuf,ERRBUFSIZE);
		
		if( write(malloc_errfd,errbuf,(WRTSIZE)(s-errbuf))!=(s-errbuf))
		{
			VOIDCAST write(2,"I/O error to error file\n",
					(WRTSIZE)24);
			exit(110);
		}

		StackDump(malloc_errfd, "\tStack from where allocated:\n",
				  mptr->stack);

		s = errbuf;
		*s++ = '\n';
		
		/*
		 * if the block is not currently in use
		 */
		if(  (mptr->flag & M_INUSE) == 0 )
		{
			t="\tThis block was freed on the ";
			COPY(s,t,errbuf,ERRBUFSIZE);

			s += tostring(s,(ULONG)mptr->fid,0,10,' ');

			t = malloc_int_suffix(mptr->fid);
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = " call to ";
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = FreeFuncName(mptr);
			COPY(s,t,errbuf,ERRBUFSIZE);

			t = "()";
			COPY(s,t,errbuf,ERRBUFSIZE);

			/*
			 * if we know where it was freed
			 */
			if( mptr->ffile != NULL )
			{
				t = "\n\tin ";
				COPY(s,t,errbuf,ERRBUFSIZE);

				t = mptr->ffile;
				COPY(s,t,errbuf,ERRBUFSIZE);

				t = " on line ";
				COPY(s,t,errbuf,ERRBUFSIZE);
			
				s += tostring(s,(ULONG)mptr->fline,0,10,' ');
			}

			t = ".\n";
			COPY(s,t,errbuf,ERRBUFSIZE);
		
			if( write(malloc_errfd,errbuf,(WRTSIZE)(s-errbuf))
				!= (s-errbuf))
			{
				VOIDCAST write(2,"I/O error to error file\n",
					(WRTSIZE)24);
				exit(110);
			}

			StackDump(malloc_errfd, "\tStack from where freed:\n",
				  mptr->freestack);

			s = errbuf;
			*s++ = '\n';
		}
		
	}
	else
	{
		t = ".\n\n";
		COPY(s,t,errbuf,ERRBUFSIZE);
	}

		
	if( write(malloc_errfd,errbuf,(WRTSIZE)(s-errbuf)) != (s-errbuf))
	{
		VOIDCAST write(2,"I/O error to error file\n", (WRTSIZE)24);
		exit(110);
	}

	/*
	 * if this is the primary suspect and we are showing links
	 */
	if( (malloc_opts & MOPT_SLINKS) && (id == DUMP_PTR ) )
	{
		/*
		 * show the previous and next malloc regions.
		 */
		malloc_dump_info_block(mptr->prev,DUMP_PREV);
		malloc_dump_info_block(mptr->next,DUMP_NEXT);
	}
		
} /* malloc_dump_info_block(... */

/*
 * Function:	malloc_err_handler()
 *
 * Purpose:	to take the appropriate action for warning and/or fatal 
 *		error conditions.
 *
 * Arguments:	level - error handling level 
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *
 * Notes:	This routine does not make use of any libc functions to build
 *		and/or disply the error message.  This is due to the fact that
 *		we are probably at a point where malloc is having a real problem
 *		and we don't want to call any function that may use malloc.
 */
VOIDTYPE
malloc_err_handler(level)
	int		  level;
{

	if( level & M_HANDLE_DUMP )
	{
		malloc_dump(malloc_errfd);
	}

	switch( level & ~M_HANDLE_DUMP )
	{
		/*
		 * If we are to drop a core file and exit
		 */
		case M_HANDLE_ABORT:
			VOIDCAST signal(ABORT_SIGNAL, SIG_DFL);

			/*
			 * first try the generic abort function
		 	 */
			VOIDCAST malloc_abort();

			/*
		 	 * if we are still here, then use the real abort
			 */
			VOIDCAST abort();

			/*
		 	 * and if we are still here, just exit.
			 */
			exit(201);
			break;

		/*
		 * If we are to exit..
		 */
		case M_HANDLE_EXIT:
			exit(200);
			break;

		/*
		 * If we are to dump a core, but keep going on our merry way
		 */
		case M_HANDLE_CORE:
			{
				int	  pid;

				/*
				 * fork so child can abort (and dump core)
				 */
				if( (pid = fork()) == 0 )
				{
					VOIDCAST write(malloc_errfd,
							"Child dumping core\n",
							(WRTSIZE)19);
					VOIDCAST signal(ABORT_SIGNAL, SIG_DFL);
					/*
					 * first try the generic abort function
					 */
					VOIDCAST malloc_abort();
					/*
					 * then try the real abort
					 */
					VOIDCAST abort();
					/*
					 * if the abort doesn't cause us to
					 * die, then we may as well just exit
					 */
					exit(201);
				}

				/*
 				 * wait for child to finish dumping core
				 */
				while( wait((int *)0) != pid)
				{
				}

				/*
				 * Move core file to core.pid.cnt so 
				 * multiple cores don't overwrite each
				 * other.
				 */
				if( access("core",0) == 0 )
				{
					static int	  corecnt;
					char	  	  filenam[32];
					filenam[0] = 'c';
					filenam[1] = 'o';
					filenam[2] = 'r';
					filenam[3] = 'e';
					filenam[4] = '.';
					VOIDCAST tostring(filenam+5,
						(ULONG)getpid(),
						5, B_DEC, '0');
					filenam[10] = '.';
					VOIDCAST tostring(filenam+11,
						(ULONG)corecnt++,
						3, B_DEC, '0');
					filenam[14] = '\0';
					VOIDCAST unlink(filenam);
					if( link("core",filenam) == 0)
					{
						VOIDCAST unlink("core");
					}
				}
			}


		/* 
		 * If we are to just ignore the error and keep on processing
		 */
		case M_HANDLE_IGNORE:
			break;

	} /* switch(... */

} /* malloc_err_handler(... */

/*
 * Function:	malloc_int_suffix()
 *
 * Purpose:	determine the correct suffix for the integer passed 
 *		(i.e. the st on 1st, nd on 2nd).
 *
 * Arguments:	i - the integer whose suffix is desired.
 *
 * Returns:	pointer to the suffix
 *
 * Narrative:
 *
 */
CONST char *
malloc_int_suffix(i)
	IDTYPE	  i;
{
	int		  j;
	CONST char	* rtn;

	/*
	 * since the suffixes repeat for the same number within a
	 * given 100 block (i.e. 111 and 211 use the same suffix), get the 
	 * integer moded by 100.
	 */
	i = i % 100;
	j = i % 10;

	/*
	 * if the number is 11, or 12, or 13 or its singles digit is
	 * not a 1, 2, or 3, the suffix must be th.
	 */
	if( (i == 11) || (i == 12) || (i == 13) ||
		( (j != 1) && (j != 2) && (j != 3) ) )
	{
		rtn = "th";
	}
	else 
	{
		switch(j)
		{
			case 1:
				rtn = "st";
				break;
			case 2:
				rtn = "nd";
				break;
			case 3:
				rtn = "rd";
				break;
			default:
				rtn = "th";
				break;
		}
	}

	return(rtn);
	
} /* malloc_int_suffix(... */

/*
 * Function:	malloc_freeseg()
 *
 * Purpose:	to add or remove a segment from the list of free segments
 *
 * Arguments:	op  - operation (M_FREE_REMOVE or M_FREE_ADD)
 *		ptr - ptr to segment to be added/removed
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *
 */
VOIDTYPE
malloc_freeseg(op,ptr)
	int		  op;
	struct mlist	* ptr;
{

	/*
	 * if we are to remove it from the list
	 */
	if( op == M_FREE_REMOVE )
	{
		/*
		 * if this is the head of the list, get a new head pointer
		 */
		if( ptr == malloc_freelist )
		{
			malloc_freelist = malloc_freelist->freenext;
		}

		/*
		 * if there is an item after this one in the free list,
		 *    link it to our prev item (which may be null)
		 */
		if( ptr->freenext != (struct mlist *) NULL)
		{
			ptr->freenext->freeprev = ptr->freeprev;
		}

		/*
		 * if there is an item before this one in the free list,
		 *    link it to the next item (which may also be NULL)
	 	 */
		if( ptr->freeprev != (struct mlist *) NULL)
		{
			ptr->freeprev->freenext = ptr->freenext;
		}

		/*
		 * disable the free list pointers on the segment that was
		 * removed from the list.
		 */
		ptr->freenext = ptr->freeprev = (struct mlist *) NULL;

	}
	else /* it is an add */
	{
		/*
		 * setup the new links for the new head pointer (new items are
	 	 * always placed at the begining of the list.  However, they may
		 * be removed from anywhere in the list (hence the double
		 * linking))
		 */
		ptr->freeprev = (struct mlist *) NULL;
		ptr->freenext = malloc_freelist;
		

		/*
	 	 * if there was already a valid head pointer
		 */
		if( malloc_freelist != (struct mlist *) NULL )
		{
			/*
			 * link it back to the new head pointer
			 */
			malloc_freelist->freeprev = ptr;
		}
		/*
		 * store the new head pointer
		 */
		malloc_freelist = ptr;

	}

} /* malloc_freeseg(... */

CONST char *
MallocFuncName(mptr)
	CONST struct mlist	* mptr;
{
	CONST char			* rtn;

	/*
	 * get the function name 
	 */
	switch(GETTYPE(mptr))
	{
		case M_T_MALLOC:
			rtn = "malloc";
			break;
		case M_T_REALLOC:
			rtn = "realloc";
			break;
		case M_T_CALLOC:
			rtn = "calloc";
			break;
		case M_T_SPLIT:
			rtn = "splitfunc";
			break;
		case M_T_XTMALLOC:
			rtn = "XtMalloc";
			break;
		case M_T_XTREALLOC:
			rtn = "XtRealloc";
			break;
		case M_T_XTCALLOC:
			rtn = "XtCalloc";
			break;
		case M_T_ALIGNED:
			rtn = "memalign";
			break;
		default:
			rtn = "Unknown";
			break;
	}

	return( rtn );

} /* MallocFuncName(... */

CONST char *
FreeFuncName(mptr)
	CONST struct mlist	* mptr;
{
	CONST char		* rtn;

	/*
	 * get the function name 
	 */
	switch(GETFTYPE(mptr))
	{
		case F_T_FREE:
			rtn = "free";
			break;
		case F_T_CFREE:
			rtn = "cfree";
			break;
		case F_T_XTFREE:
			rtn = "XtFree";
			break;
		case F_T_REALLOC:
			rtn = "realloc";
			break;
		default:
			rtn = "Unknown";
			break;
	}

	return(rtn);

} /* FreeFuncName(... */

void
InitMlist(mptr,type)
	struct mlist	* mptr;
	int		  type;
{
	DataMS((MEMDATA *) mptr,'\0', sizeof( struct mlist));

	mptr->flag = M_MAGIC;
	SETTYPE(mptr,type);

	mptr->hist_id = malloc_hist_id++;

}

/*
 * $Log: malloc.c,v $
 * Revision 1.1  1996/06/18 03:29:19  warren
 * Added debug malloc package.
 *
 * Revision 1.43  1992/09/03  22:24:33  cpcahil
 * final changes for PL14
 *
 * Revision 1.42  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.41  1992/07/31  11:47:30  cpcahil
 * added setting of free-file pointer when splitting off a segment
 *
 * Revision 1.40  1992/07/12  15:30:58  cpcahil
 * Merged in Jonathan I Kamens' changes
 *
 * Revision 1.39  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.38  1992/06/30  13:06:39  cpcahil
 * added support for aligned allocations
 *
 * Revision 1.37  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.36  1992/05/09  00:16:16  cpcahil
 * port to hpux and lots of fixes
 *
 * Revision 1.35  1992/05/08  02:39:58  cpcahil
 * minor fixups
 *
 * Revision 1.34  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.33  1992/05/06  05:37:44  cpcahil
 * added overriding of fill characters and boundary size
 *
 * Revision 1.32  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.31  1992/04/24  12:09:13  cpcahil
 * (hopefully) final cleanup for patch 10
 *
 * Revision 1.30  1992/04/24  11:18:52  cpcahil
 * Fixes from Denny Page and Better integration of Xt alloc hooks
 *
 * Revision 1.29  1992/04/22  18:17:32  cpcahil
 * added support for Xt Alloc functions, linted code
 *
 * Revision 1.28  1992/04/20  22:29:14  cpcahil
 * changes to fix problems introduced by insertion of size_t
 *
 * Revision 1.27  1992/04/20  22:03:44  cpcahil
 * added setting of filled flag.
 *
 * Revision 1.26  1992/04/15  12:51:06  cpcahil
 * fixes per testing of patch 8
 *
 * Revision 1.25  1992/04/13  17:26:25  cpcahil
 * minor portability changes
 *
 * Revision 1.24  1992/04/13  13:56:02  cpcahil
 * added auto-determination of abort signal to ensure that we have
 * a valid signal for this system.
 *
 * Revision 1.23  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.22  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.21  1992/02/19  02:33:07  cpcahil
 * added code to ensure that aborts really happen.
 *
 * Revision 1.20  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.19  1992/01/28  16:35:37  cpcahil
 * increased size of error string buffers and added overflow checks
 *
 * Revision 1.18  1992/01/10  17:51:03  cpcahil
 * more void stuff that slipped by
 *
 * Revision 1.17  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.16  1992/01/08  19:40:07  cpcahil
 * fixed write() count to display entire message.
 *
 * Revision 1.15  1991/12/31  21:31:26  cpcahil
 * changes for patch 6.  See CHANGES file for more info
 *
 * Revision 1.14  1991/12/06  08:50:48  cpcahil
 * fixed bug in malloc_safe_memset introduced in last change.
 *
 * Revision 1.13  91/12/04  18:01:21  cpcahil
 * cleand up some aditional warnings from gcc -Wall
 * 
 * Revision 1.12  91/12/04  09:23:39  cpcahil
 * several performance enhancements including addition of free list
 * 
 * Revision 1.11  91/12/02  19:10:10  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.10  91/11/25  14:41:59  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.9  91/11/24  16:56:41  cpcahil
 * porting changes for patch level 4
 * 
 * Revision 1.8  91/11/24  00:49:27  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.7  91/11/20  11:54:09  cpcahil
 * interim checkin
 * 
 * Revision 1.6  90/05/11  00:13:09  cpcahil
 * added copyright statment
 * 
 * Revision 1.5  90/02/25  11:01:18  cpcahil
 * added support for malloc chain checking.
 * 
 * Revision 1.4  90/02/24  21:50:21  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.3  90/02/24  14:51:18  cpcahil
 * 1. changed malloc_fatal and malloc_warn to use malloc_errno and be passed
 *    the function name as a parameter.
 * 2. Added several function headers.
 * 3. Changed uses of malloc_fatal/warning to conform to new usage.
 * 
 * Revision 1.2  90/02/23  18:05:23  cpcahil
 * fixed open of error log to use append mode.
 * 
 * Revision 1.1  90/02/22  23:17:43  cpcahil
 * Initial revision
 * 
 */
