
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
#include "mallocin.h"

/*
 * Function:	malloc_chain_check()
 *
 * Purpose:	to verify malloc chain is intact
 *
 * Arguments:	todo	- 0 - just check and return status
 *			  1 - call malloc_warn if error detected
 *
 * Returns:	0	- malloc chain intact & no overflows
 *		other	- problems detected in malloc chain
 *
 * Narrative:
 *
 * Notes:	If todo is non-zero the malloc_warn function, when called
 *		may not return (i.e. it may exit)
 *
 */
#ifndef lint
static
char rcs_hdr[] = "$Id: mchain.c,v 1.1 1996/06/18 03:29:22 warren Exp $";
#endif


int
malloc_chain_check(todo)
	int		  todo;
{
	return( DBmalloc_chain_check( (char *)NULL, 0, todo) );
}

int
DBmalloc_chain_check(file,line,todo)
	CONST char	* file;
	int		  line;
	int		  todo;
{
	return( DBFmalloc_chain_check("malloc_chain_check",file,line,todo) );
}

int
DBFmalloc_chain_check(func,file,line,todo)
	CONST char	* func;
	CONST char	* file;
	int		  line;
	int		  todo;
{
	register struct mlist	* oldptr;
	register struct mlist	* ptr;
	int			  rtn = 0;

	MALLOC_INIT();

	/*
	 * first check the full malloc chain
	 */
	oldptr = &malloc_start;
	for(ptr = malloc_start.next; ; oldptr = ptr, ptr = ptr->next)
	{
		/*
		 * Since the malloc chain is a forward only chain, any
		 * pointer that we get should always be positioned in 
		 * memory following the previous pointer.  If this is not
		 * so, we must have a corrupted chain.
		 */
		if( ptr )
		{
			if(ptr < oldptr )
			{
				malloc_errno = M_CODE_CHAIN_BROKE;
				if( todo )
				{
					malloc_fatal(func,file,line,oldptr);
				}
				rtn++;
				break;
			}
		}
		else
		{
			if( malloc_end && (oldptr != malloc_end) )
			{
				/*
				 * This should never happen.  If it does, then
				 * we got a real problem.
				 */
				malloc_errno = M_CODE_NO_END;
				if( todo )
				{
					malloc_fatal(func,file,line,oldptr);
				}
				rtn++;
			}
			break;
		}
		
		/*
		 * verify that ptr is within the malloc region...
		 * since we started within the malloc chain this should never
		 * happen.
		 */
		if(    ((DATATYPE *)ptr < malloc_data_start)
		    || ((DATATYPE *)ptr > malloc_data_end) 
		    || ((((long)ptr) & malloc_round) != 0) )
		{
			malloc_errno = M_CODE_BAD_PTR;
			if( todo )
			{
				malloc_fatal(func,file,line,oldptr);
			}
			rtn++;
			break;
		}

		/* 
		 * verify magic flag is set
		 */
		if( (ptr->flag&M_MAGIC_BITS) != M_MAGIC )
		{
			malloc_errno = M_CODE_BAD_MAGIC;
			if( todo )
			{
				malloc_warning(func,file,line,
						(struct mlist *)NULL);
			}
			rtn++;
			continue;
		}

		/* 
		 * verify segments are correctly linked together
		 */
		if( (ptr->prev && (ptr->prev->next != ptr) ) ||
		    (ptr->next && (ptr->next->prev != ptr) ) ||
		    ((ptr->next == NULL) && (ptr->prev == NULL)) )
		{
			malloc_errno = M_CODE_BAD_CONNECT;
			if( todo )
			{
				malloc_warning(func,file,line,ptr);
			}
			rtn++;
			continue;
		}

		/*
		 * check for under and/or overflow on this segment
		 */
		rtn +=  FILLCHECK(func,file,line,ptr,todo);

	} /* for(... */

	/*
	 * and now check the free list
	 */
	oldptr = NULL;
	for(ptr=malloc_freelist; (rtn == 0) && (ptr != NULL); ptr=ptr->freenext)
	{
		/*
		 * Since the malloc chain is a forward only chain, any
		 * pointer that we get should always be positioned in 
		 * memory following the previous pointer.  If this is not
		 * so, we must have a corrupted chain.
		 */
		if( (oldptr != NULL) && (ptr < oldptr) )
		{
			malloc_errno = M_CODE_CHAIN_BROKE;
			if( todo )
			{
				malloc_fatal(func,file,line,oldptr);
			}
			rtn++;
		}
		/*
		 * verify that ptr is within the malloc region...
		 * since we started within the malloc chain this should never
		 * happen.
		 */
		else if(    ((DATATYPE *)ptr < malloc_data_start)
		         || ((DATATYPE *)ptr > malloc_data_end) 
     		         || ((((long)ptr) & malloc_round) != 0) )
		{
			malloc_errno = M_CODE_BAD_PTR;
			if( todo )
			{
				malloc_fatal(func,file,line,oldptr);
			}
			rtn++;
		}
		/* 
		 * verify magic flag is set
		 */
		else if( (ptr->flag&M_MAGIC_BITS) != M_MAGIC )
		{
			malloc_errno = M_CODE_BAD_MAGIC;
			if( todo )
			{
				malloc_warning(func,file,line,
						(struct mlist *)NULL);
			}
			rtn++;
		}
		/* 
		 * verify segments are correctly linked together
		 */
		else if(   (ptr->freeprev && (ptr->freeprev->freenext != ptr) )
			|| (ptr->freenext && (ptr->freenext->freeprev != ptr) ))
		{
			malloc_errno = M_CODE_BAD_CONNECT;
			if( todo )
			{
				malloc_warning(func,file,line,ptr);
			}
			rtn++;
		}
		/*
		 * else if this segment is in use
		 */
		else if( (ptr->flag & M_INUSE) != 0 )
		{
			malloc_errno = M_CODE_FREELIST_BAD;
			if( todo )
			{
				malloc_warning(func,file, line,ptr);
			}
			rtn++;
		}
		/*
		 * else we have to check the filled areas.
		 */
		else
		{
			/*
			 * check for underflow and/or reuse of this segment
			 */
			rtn +=  FILLCHECK(func,file,line,ptr,todo);
		}

	} /* for(... */

	return(rtn);

} /* malloc_chain_check(... */
