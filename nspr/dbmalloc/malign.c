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
static char rcs_hdr[] = "$Id: malign.c,v 1.1 1996/06/18 03:29:18 warren Exp $";
#endif

/*
 * Function:	mem_align()
 *
 * Purpose:	allocate memory aligned to a multiple of the specified 
 *		alignment.
 *
 * Arguments:	size	- size of data area needed
 *
 * Returns:	whatever debug_malloc returns.
 *
 * Narrative:
 *
 */
DATATYPE *
memalign(align,size)
	SIZETYPE	  align;
	SIZETYPE	  size;
{
	return( DBmemalign(NULL,-1,align,size) );
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
DBmemalign(file,line,align,size)
	CONST char	* file;
	int		  line;
	SIZETYPE	  align;
	SIZETYPE	  size;
{
	SIZETYPE	  bitcnt = 0;
	static IDTYPE	  call_counter;
	SIZETYPE	  i;

	/*
	 * increment the counter for the number of calls to this func.
	 */
	call_counter++;

	/*
	 * perform some checks first 
	 */

	/*
	 * count up the number of bits that have been set (a number
	 * that is a power of two will only have one bit set)
	 */
	for( i=0; i < (sizeof(align) * 8); i++)
	{
		if ( (align & (0x01 << i)) != 0 )
		{
			bitcnt++;
		}
	}

	/*
	 * if there is other than a single bit set, there was a problem,
	 * so return NULL.
	 */
	if( bitcnt != 1 )
	{
		return( (DATATYPE *) NULL);
	}

	/*
	 * if the alignment is too small, increase it until it is 
	 * large enough 
	 */
	while( align < malloc_round )
	{
		align <<= 1;
	}

	/*
	 * set the alignment value for this call
	 */
	malloc_align = align;

	/*
	 * call the malloc function and return its result.
	 */
	return( DBFmalloc("memalign",M_T_ALIGNED,call_counter,file,line,size) );

} /* DBmemalign(... */

/*
 * AlignedFit() - determine how close the aligned requirement fits within
 *		  the specified malloc segment.
 *
 * This function takes into account the amount of offset into the specified
 * segment the new malloc pointer will have to be in order to be aligned
 * on the correct boundry.
 */
int
AlignedFit(mptr,align,size)
	struct mlist	* mptr;
	SIZETYPE	  align;
	SIZETYPE	  size;
{
	int		  fit;
	SIZETYPE	  offset;

	offset = AlignedOffset(mptr,align);

	fit = mptr->s.size - (offset + size);

	return( fit );

} /* AlignedFit(... */

/*
 * AlignedMakeSeg() - make a new segment at the correct offset within
 * 		      the specified old segment such that this new 
 *		      segment will have it's data pointer aligned at the
 *		      correct offset.
 */

struct mlist *
AlignedMakeSeg(mptr,align)
	struct mlist 	* mptr;
	SIZETYPE	  align;
{
	struct mlist	* newptr;
	SIZETYPE	  offset;

	DEBUG2(10,"AlignedMakeSeg(0x%lx,%d) called...", mptr, align);

	/*
	 * get the offset of the pointer which will ensure that we have
	 * a new segment with the correct alignment.
	 */
	offset = AlignedOffset(mptr,align);

	if( offset > mptr->s.size )
	{
		abort();
	}
	DEBUG2(20,"Adjusting space (0x%lx) by %d bytes to get alignment",
		mptr->data, offset);

	/*
	 * get a pointer to the new segment
	 */
	newptr = (struct mlist *) (((char *)mptr) + offset);

	/*
	 * initialize the new segment
 	 */
	InitMlist(newptr,M_T_SPLIT);

	/*
	 * link in the new segment
	 */
	newptr->prev = mptr;
	newptr->next = mptr->next;
	if( newptr->next )
	{
		newptr->next->prev = newptr;
	}
	mptr->next   = newptr;

	/*
	 * set the size in the new segment
	 */
	newptr->s.size = mptr->s.size - offset;
	newptr->r_size = newptr->s.size;

	/*
	 * set the size in the old segment
	 */
	DEBUG2(20,"Adjusting old segment size from %d to %d",
			mptr->s.size, offset-M_SIZE);
	mptr->s.size = offset - M_SIZE;
	mptr->r_size = mptr->s.size;

	/*
	 * if mptr was the end of the list, newptr is the new end.
	 */
	if( mptr == malloc_end )
	{
		malloc_end = newptr;
	}

	return(newptr);

} /* AlignedMakeSeg(... */

/*
 * AlignedOffset() - calculate the offset from the current data pointer to
 *		     a new data pointer that will have the specified
 *		     alignment
 */
SIZETYPE
AlignedOffset(mptr,align)
	struct mlist	* mptr;
	SIZETYPE	  align;
{
	SIZETYPE	  offset;

	/*
	 * figure out the offset between the current data pointer and the next
	 * correctly aligned location
	 */
	offset = align - (((SIZETYPE)mptr->data) & (align-1));

	/*
	 * keep incrementing the offset until we have at least the malloc
	 * struct overhead in the offset (so we can make a new segment at
	 * the begining of this segment
	 */
	while( offset < sizeof(struct mlist) )
	{
		offset += align;
	}

	return(offset);
}

