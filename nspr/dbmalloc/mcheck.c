
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

#ifndef lint
static
char rcs_hdr[] = "$Id: mcheck.c,v 1.1 1996/06/18 03:29:23 warren Exp $";
#endif

#define malloc_in_arena(ptr) (   ((DATATYPE*)(ptr) >= malloc_data_start) \
			      && ((DATATYPE*)(ptr) <= malloc_data_end) )

/*
 * Function:	malloc_check_str()
 *
 * Arguments:	func	- name of function calling this routine
 *		str	- pointer to area to check
 *
 * Purpose:	to verify that if str is within the malloc arena, the data 
 *		it points to does not extend beyond the applicable region.
 *
 * Returns:	Nothing of any use (function is void).
 *
 * Narrative:
 *   IF pointer is within malloc arena
 *      determin length of string
 *      call malloc_verify() to verify data is withing applicable region
 *   return 
 *
 * Mod History:	
 *   90/01/24	cpcahil		Initial revision.
 *   90/01/29	cpcahil		Added code to ignore recursive calls.
 */
VOIDTYPE
malloc_check_str(func,file,line,str)
	CONST char		* func;
	CONST char		* file;
	int			  line;
	CONST char		* str;
{
	static int		  layers;
	register CONST char	* s;

	MALLOC_INIT();

	/*
	 * if we are already in the malloc library somewhere, don't check
	 * things again.
	 */
	if( in_malloc_code || (! (malloc_opts & MOPT_CKDATA) ) )
	{
		return;
	}

	if( (layers++ == 0) &&  malloc_in_arena(str) )
	{
		for( s=str; *s; s++)
		{
		}
		
		malloc_verify(func,file,line,str,(SIZETYPE)(s-str+1));
	}

	layers--;
}

/*
 * Function:	malloc_check_strn()
 *
 * Arguments:	func	- name of function calling this routine
 *		str	- pointer to area to check
 * 		len     - max length of string
 *
 * Purpose:	to verify that if str is within the malloc arena, the data 
 *		it points to does not extend beyond the applicable region.
 *
 * Returns:	Nothing of any use (function is void).
 *
 * Narrative:
 *   IF pointer is within malloc arena
 *      determin length of string
 *      call malloc_verify() to verify data is withing applicable region
 *   return 
 *
 * Mod History:	
 *   90/01/24	cpcahil		Initial revision.
 *   90/01/29	cpcahil		Added code to ignore recursive calls.
 *   90/08/29	cpcahil		added length (for strn* functions)
 */
VOIDTYPE
malloc_check_strn(func,file,line,str,len)
	CONST char	* func;
	CONST char 	* file;
	int		  line;
	CONST char	* str;
	STRSIZE		  len;
{
	register STRSIZE	  i;
	static int		  layers;
	register CONST char	* s;

	MALLOC_INIT();

	/*
	 * if we are already in the malloc library somewhere, don't check
	 * things again.
	 */
	if( in_malloc_code || (! (malloc_opts & MOPT_CKDATA) ) )
	{
		return;
	}

	if( (layers++ == 0) &&  malloc_in_arena(str) )
	{
		for( s=str,i=0; (i < len) && (*s != '\0'); i++,s++)
		{
		}

		/*
		 * if we found a null byte before len, add one to s so
		 * that we ensure that the null is counted in the bytes to
		 * check.
		 */
		if( i < len )
		{
			s++;
		}
		malloc_verify(func,file,line,str,(SIZETYPE)(s-str));
	}

	layers--;
}

/*
 * Function:	malloc_check_data()
 *
 * Arguments:	func	- name of function calling this routine
 *		ptr	- pointer to area to check
 *		len 	- length to verify
 *
 * Purpose:	to verify that if ptr is within the malloc arena, the data 
 *		it points to does not extend beyond the applicable region.
 *
 * Returns:	Nothing of any use (function is void).
 *
 * Narrative:
 *   IF pointer is within malloc arena
 *      call malloc_verify() to verify data is withing applicable region
 *   return 
 *
 * Mod History:	
 *   90/01/24	cpcahil		Initial revision.
 *   90/01/29	cpcahil		Added code to ignore recursive calls.
 */
VOIDTYPE
malloc_check_data(func,file,line,ptr,len)
	CONST char	* func;
	CONST char	* file;
	int		  line;
	CONST DATATYPE	* ptr;
	SIZETYPE	  len;
{
	static int	  layers;

	MALLOC_INIT();

	/*
	 * if we are already in the malloc library somewhere, don't check
	 * things again.
	 */
	if( in_malloc_code || (! (malloc_opts & MOPT_CKDATA) ) )
	{
		return;
	}

	if( layers++ == 0 )
	{
		DEBUG3(40,"malloc_check_data(%s,0x%x,%d) called...",
			func,ptr,len);
		if( malloc_in_arena(ptr) )
		{
			DEBUG0(10,"pointer in malloc arena, verifying...");
			malloc_verify(func,file,line,ptr,len);
		}
	}

	layers--;
}

/*
 * Function:	malloc_verify()
 *
 * Arguments:	func	- name of function calling the malloc check routines
 *		ptr	- pointer to area to check
 *		len 	- length to verify
 *
 * Purpose:	to verify that the data ptr points to does not extend beyond
 *		the applicable malloc region.  This function is only called 
 *		if it has been determined that ptr points into the malloc arena.
 *
 * Returns:	Nothing of any use (function is void).
 *
 * Narrative:
 *
 * Mod History:	
 *   90/01/24	cpcahil		Initial revision.
 */
VOIDTYPE
malloc_verify(func,file,line,ptr,len)
	CONST char		* func;
	CONST char		* file;
	int			  line;
	register CONST DATATYPE	* ptr;
	register SIZETYPE	  len;
{
	register CONST struct mlist	* mptr = NULL;
	
	DEBUG5(40,"malloc_verify(%s, %s, %s, 0x%x,%d) called...",
		func, file, line, ptr, len);

	/*
	 * since we know we have a pointer that is somewhere within the
	 * malloc region, let's take a guess that the current pointer is
	 * at the begining of a region and see if it has the correct values
	 */

	/*
	 * if we are on the correct boundry for a malloc data area pointer
	 */
	if(   (((long)ptr) & malloc_round) == 0 )
	{
		mptr = (CONST struct mlist *)(((CONST char *) ptr ) - M_SIZE);

		if( ! GoodMlist(mptr) )
		{
			mptr = (CONST struct mlist *) NULL;
		}
	}

	/*
	 * if we haven't found it yet, let's try to look back just a few bytes
	 * to see if it is nearby
	 */
	if( mptr == (struct mlist *) NULL)
	{
		register long	* lptr;
		register int	  i;

		lptr = (long *) (((long)ptr) & ~malloc_round);

		for(i=0; i < MAX_KLUDGE_CHECKS; i++)
		{
			lptr--;
			if( ! malloc_in_arena(lptr) )
			{
				mptr = (CONST struct mlist *) NULL;
				break;
			}

			/*
			 * if we found the magic number
		 	 */
			if( ((*lptr) & M_MAGIC_BITS) == M_MAGIC )
			{
				mptr = (CONST struct mlist *)
					(((CONST char *)lptr) - M_FLAGOFF);

				/*
				 * if the pointer is good.
			 	 */
				if( GoodMlist(mptr) )
				{
					break;
				}
			}

		}

		/*
		 * if we didn't find an entry, make sure mptr is null
		 */
		if( i == MAX_KLUDGE_CHECKS )
		{
			mptr = (CONST struct mlist *) NULL;
		}
	}


	/*
	 * if we still haven't found the right segment, we have to do it the
	 * hard way and search along the malloc list
	 */
	if( mptr == (CONST struct mlist *) NULL)
	{
		/*
		 * Find the malloc block that includes this pointer
		 */
		mptr = &malloc_start;
		while( mptr
			&& ! ((((CONST DATATYPE *)mptr) < (CONST DATATYPE *)ptr)
			&&   (((DATATYPE *)(mptr->data+mptr->s.size)) > ptr) ) )
		{
			mptr = mptr->next;
		}
	}

	/*
	 * if ptr was not in a malloc block, it must be part of
	 *    some direct sbrk() stuff, so just return.
	 */
	if( ! mptr )
	{
		DEBUG1(10,"ptr (0x%x) not found in malloc search", ptr);
		return;
	}
	
	/*
 	 * Now we have a valid malloc block that contains the indicated
	 * pointer.  We must verify that it is withing the requested block
	 * size (as opposed to the real block size which is rounded up to
	 * allow for correct alignment).
	 */

	DEBUG4(60,"Checking  0x%x-0x%x, 0x%x-0x%x",
			ptr, ptr+len, mptr->data, mptr->data+mptr->r_size);
	
	if(    (ptr < (DATATYPE *)mptr->data)
  	    || ((((CONST char *)ptr)+len) > (char *)(mptr->data+mptr->r_size)) )
	{
		DEBUG4(0,"pointer not within region 0x%x-0x%x, 0x%x-0x%x",
			ptr, ptr+len, mptr->data, mptr->data+mptr->r_size);

		malloc_errno = M_CODE_OUTOF_BOUNDS;
		malloc_warning(func,file,line,mptr);
	}
	else if ( (mptr->flag&M_INUSE) == 0 )
	{
		DEBUG4(0,"segment not in use 0x%x-0x%x, 0x%x-0x%x",
			ptr, ptr+len, mptr->data, mptr->data+mptr->r_size);

		malloc_errno = M_CODE_NOT_INUSE;
		malloc_warning(func,file,line,mptr);
	}

	return;
}

/*
 * GoodMlist() - return true if mptr appears to point to a valid segment
 */
int
GoodMlist(mptr)
	register CONST struct mlist * mptr;
{

	/*
	 * return true if
	 * 	1. the segment has a valid magic number
	 * 	2. the inuse flag is set
	 *	3. the previous linkage is set correctly
	 *	4. the next linkage is set correctly
	 *	5. either next and prev is not null
	 */
		
	return(    ((mptr->flag&M_MAGIC_BITS) == M_MAGIC)
		&& ((mptr->flag&M_INUSE) != 0 )
 		&& (mptr->prev ?
		    (malloc_in_arena(mptr->prev) && (mptr->prev->next == mptr)) : 1 )
 		&& (mptr->next ?
		    (malloc_in_arena(mptr->next) && (mptr->next->prev == mptr)) : 1 )
	    	&& ((mptr->next == NULL) || (mptr->prev == NULL)) );

} /* GoodMlist(... */
/*
 * $Log: mcheck.c,v $
 * Revision 1.1  1996/06/18 03:29:23  warren
 * Added debug malloc package.
 *
 * Revision 1.22  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.21  1992/06/27  22:48:48  cpcahil
 * misc fixes per bug reports from first week of reviews
 *
 * Revision 1.20  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.19  1992/05/09  00:16:16  cpcahil
 * port to hpux and lots of fixes
 *
 * Revision 1.18  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.17  1992/05/08  01:44:11  cpcahil
 * more performance enhancements
 *
 * Revision 1.16  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.15  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.14  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.13  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.12  1992/01/10  17:51:03  cpcahil
 * more void stuff that slipped by
 *
 * Revision 1.11  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.10  1991/12/31  02:23:29  cpcahil
 * fixed verify bug of strncpy when len was exactly same as strlen
 *
 * Revision 1.9  91/12/02  19:10:12  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.8  91/11/25  14:42:01  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.7  91/11/24  00:49:29  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.6  91/11/20  11:54:11  cpcahil
 * interim checkin
 * 
 * Revision 1.5  90/08/29  22:23:48  cpcahil
 * added new function to check on strings up to a specified length 
 * and used it within several strn* functions.
 * 
 * Revision 1.4  90/05/11  00:13:09  cpcahil
 * added copyright statment
 * 
 * Revision 1.3  90/02/24  21:50:22  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.2  90/02/24  17:29:38  cpcahil
 * changed $Header to $Id so full path wouldnt be included as part of rcs 
 * id string
 * 
 * Revision 1.1  90/02/24  14:57:03  cpcahil
 * Initial revision
 * 
 */
