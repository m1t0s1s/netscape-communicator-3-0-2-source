
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
char rcs_hdr[] = "$Id: m_init.c,v 1.1 1996/06/18 03:29:17 warren Exp $";
#endif

#include <stdio.h>
#include "mallocin.h"

/*
 * Function:	malloc_init()
 *
 * Purpose:	to initialize the pointers and variables use by the
 *		malloc() debugging library
 *
 * Arguments:	none
 *
 * Returns:	nothing of any value
 *
 * Narrative:	Just initialize all the needed variables.  Use dbmallopt
 *		to set options taken from the environment.
 *
 */
VOIDTYPE
malloc_init()
{
	char			* cptr;
	union dbmalloptarg	  m;
	int			  size;
	int			  round;

	/*
 	 * If already initialized...
	 */
	if( malloc_data_start != (char *) 0)
	{
		return;
	}


	malloc_data_start = sbrk(0);
	malloc_data_end = malloc_data_start;
	malloc_start.s.size = 0;
	malloc_end = &malloc_start;

	/*
	 * test to see what rounding we need to use for this system
	 */
	size = M_SIZE;
	round = M_RND;
	while( round > 0 )
	{
	
		if( (size & (round-1)) == 0 )
		{
			malloc_round = round-1;
			break;
		}
		round >>= 1;
	}

	if( round == 0 )
	{
		malloc_errno = M_CODE_NOBOUND;
		malloc_fatal("malloc_init",__FILE__,__LINE__,(struct mlist*)0);
	}

	/*
	 * the following settings can only be set in the environment.  They
	 * cannot be set via calls to dbmallopt().
	 */
	if( (cptr=getenv("MALLOC_BOUNDSIZE")) != NULL )
	{
		malloc_boundsize = atoi(cptr);

		if( malloc_boundsize < 1 )
		{
			malloc_boundsize = M_DFLT_BSIZE;
		}
	}

	
	if( (cptr=getenv("MALLOC_CKCHAIN")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_CKCHAIN,&m);
	}

	if( (cptr=getenv("MALLOC_CKDATA")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_CKDATA,&m);
	}

	if( (cptr=getenv("MALLOC_DETAIL")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_DETAIL,&m);
	}

	if( (cptr=getenv("MALLOC_ERRFILE")) != NULL)
	{
		m.str = cptr;
		VOIDCAST dbmallopt(MALLOC_ERRFILE,&m);
	}

	if( (cptr=getenv("MALLOC_FATAL")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_FATAL,&m);
	}

	if( (cptr=getenv("MALLOC_FILLAREA")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_FILLAREA,&m);
	}

	if( (cptr=getenv("MALLOC_FILLBYTE")) != NULL )
	{
		malloc_fillbyte = atoi(cptr);

		if( (malloc_fillbyte < 0) || (malloc_fillbyte > 255) )
		{
			malloc_fillbyte = M_DFLT_FILL;
		}
	}

	if( (cptr=getenv("MALLOC_FREEBYTE")) != NULL )
	{
		malloc_freebyte = atoi(cptr);

		if( (malloc_freebyte < 0) || (malloc_freebyte > 255) )
		{
			malloc_freebyte = M_DFLT_FREE_FILL;
		}
	}

	if( (cptr=getenv("MALLOC_FREEMARK")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_FREEMARK,&m);
	}

	if( (cptr=getenv("MALLOC_LOWFRAG")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_LOWFRAG,&m);
	}

	if( (cptr=getenv("MALLOC_REUSE")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_REUSE,&m);
	}

	if( (cptr=getenv("MALLOC_SHOWLINKS")) != NULL)
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_SHOWLINKS,&m);
	}

	if( (cptr=getenv("MALLOC_WARN")) != NULL )
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_WARN,&m);
	}

	if( (cptr=getenv("MALLOC_ZERO")) != NULL )
	{
		m.i = atoi(cptr);
		VOIDCAST dbmallopt(MALLOC_ZERO,&m);
	}

	/*
	 * set the malloc_fill initial value 
	 */
	if( (malloc_opts & (MOPT_MFILL | MOPT_FFILL | MOPT_DFILL)) != 0 )
	{
		malloc_fill = 1;
	}

} /* malloc_init(... */

/*
 * $Log: m_init.c,v $
 * Revision 1.1  1996/06/18 03:29:17  warren
 * Added debug malloc package.
 *
 * Revision 1.22  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.21  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.20  1992/07/02  15:35:52  cpcahil
 * misc cleanups for PL13
 *
 * Revision 1.19  1992/06/30  13:06:39  cpcahil
 * added support for aligned allocations
 *
 * Revision 1.18  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.17  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.16  1992/05/06  05:37:44  cpcahil
 * added overriding of fill characters and boundary size
 *
 * Revision 1.15  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.14  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.13  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.12  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.11  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.10  1991/12/31  21:31:26  cpcahil
 * changes for patch 6.  See CHANGES file for more info
 *
 * Revision 1.9  1991/12/04  09:23:38  cpcahil
 * several performance enhancements including addition of free list
 *
 * Revision 1.8  91/11/25  14:41:54  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.7  91/11/24  00:49:26  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.6  90/08/29  22:23:21  cpcahil
 * fixed mallopt to use a union as an argument.
 * 
 * Revision 1.5  90/08/29  21:22:50  cpcahil
 * miscellaneous lint fixes
 * 
 * Revision 1.4  90/05/11  15:53:35  cpcahil
 * fixed bug in initialization code.
 * 
 * Revision 1.3  90/05/11  00:13:08  cpcahil
 * added copyright statment
 * 
 * Revision 1.2  90/02/24  21:50:20  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.1  90/02/24  17:10:53  cpcahil
 * Initial revision
 * 
 */
