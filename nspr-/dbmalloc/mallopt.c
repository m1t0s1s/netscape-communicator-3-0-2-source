
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
 * Function:	dbmallopt()
 *
 * Purpose:	to set options for the malloc debugging library
 *
 * Arguments:	none
 *
 * Returns:	nothing of any value
 *
 * Narrative:	
 *
 */

#ifndef lint
static
char rcs_hdr[] = "$Id: mallopt.c,v 1.1 1996/06/18 03:29:22 warren Exp $";
#endif

int
dbmallopt(cmd,value)
	int			  cmd;
	union dbmalloptarg	* value;
{
	int			  baseflags = 0;
	int			  i;
	int			  newflag = 0;
	register char		* s;
	int			  turnon = 0;

	MALLOC_INIT();

	switch(cmd)
	{
		case MALLOC_CKCHAIN:
			newflag = MOPT_CKCHAIN;
			turnon = value->i;
			break;

		case MALLOC_CKDATA:
			newflag = MOPT_CKDATA;
			turnon = value->i;
			break;

		case MALLOC_DETAIL:
			turnon = value->i;
			newflag = MOPT_DETAIL;
			break;

		case MALLOC_ERRFILE:
			
			if( strcmp(value->str,"-") != 0 )
			{
				if( malloc_errfd != 2 )
				{
					close(malloc_errfd);	
				}
				i = open(value->str,
					 O_CREAT|O_APPEND|O_WRONLY,0666);
				if( i == -1 )
				{
					VOIDCAST write(2,
					  "Unable to open malloc error file: ",
					  (WRTSIZE) 34);
					for(s=value->str; *s; s++)
					{
						/* do nothing */;
					}
					VOIDCAST write(2,value->str,
						     (WRTSIZE)(s-(value->str)));
					VOIDCAST write(2,"\n",(WRTSIZE)1);
				}
				else
				{
					if( malloc_errfd != 2 )
					{
						VOIDCAST close(malloc_errfd);
					}
					malloc_errfd = i;
				}
			}
			else
			{
				if( malloc_errfd != 2 )
				{
					close(malloc_errfd);	
				}
				malloc_errfd = 2;
			}
			
			break;

		case MALLOC_FATAL:
			malloc_fatal_level = value->i;
			break;

		case MALLOC_FREEMARK:
			turnon = value->i;
			newflag = MOPT_FREEMARK;
			break;

		case MALLOC_FILLAREA:
			baseflags = MOPT_MFILL | MOPT_FFILL;
			switch(value->i)
			{
				case 1:
					newflag = MOPT_DFILL;
					break;
				case 2:
					newflag = MOPT_MFILL | MOPT_DFILL;
					break;
				case 0:
				case 3:
				default:
					newflag = MOPT_MFILL | MOPT_FFILL
							     | MOPT_DFILL;
					break;
			}
			turnon = value->i;	

			/*
			 * if we ever enable malloc_checking, then we set the
			 * malloc_check flag to non-zero.  Then it can never be
			 * set back to zero.  This is done as a performance 
			 * increase if filling is never enabled.
			 */
			if( (turnon != 0) && (newflag != 0) )
			{
				malloc_fill = 1;
			}
			
			break;

		case MALLOC_LOWFRAG:
			newflag = MOPT_LOWFRAG;
			turnon = value->i;	
			break;

		case MALLOC_REUSE:
			turnon = value->i;
			newflag = MOPT_REUSE;
			break;

		case MALLOC_SHOWLINKS:
			turnon = value->i;
			newflag = MOPT_SLINKS;
			break;

		case MALLOC_WARN:
			malloc_warn_level = value->i;
			break;

		case MALLOC_ZERO:
			turnon = value->i;
			newflag = MOPT_ZERO;
			break;

		default:
			return(1);
	}

	/*
	 * if there are base flags, remove all of them so that they will
	 * not remain on forever.
	 */
	if( baseflags )
	{
		malloc_opts &= ~baseflags;
	}

	/*
	 * if we have a new option flag, apply it to the options variable.
	 */
	if( newflag )
	{
		if( turnon )
		{
			malloc_opts |= newflag;
		}
		else
		{
			malloc_opts &= ~newflag;
		}
	}

	return(0);
}

/*
 * $Log: mallopt.c,v $
 * Revision 1.1  1996/06/18 03:29:22  warren
 * Added debug malloc package.
 *
 * Revision 1.21  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.20  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.19  1992/07/02  15:35:52  cpcahil
 * misc cleanups for PL13
 *
 * Revision 1.18  1992/06/30  13:06:39  cpcahil
 * added support for aligned allocations
 *
 * Revision 1.17  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.16  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.15  1992/04/15  12:51:06  cpcahil
 * fixes per testing of patch 8
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
 * Revision 1.9  1991/12/04  09:23:42  cpcahil
 * several performance enhancements including addition of free list
 *
 * Revision 1.8  91/11/25  14:42:03  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.7  91/11/24  00:49:30  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.6  90/08/29  22:23:36  cpcahil
 * fixed mallopt to use a union as an argument.
 * 
 * Revision 1.5  90/08/29  21:22:51  cpcahil
 * miscellaneous lint fixes
 * 
 * Revision 1.4  90/05/11  00:13:10  cpcahil
 * added copyright statment
 * 
 * Revision 1.3  90/02/25  11:03:26  cpcahil
 * changed to return int so that it agrees with l libmalloc.a's mallopt()
 * 
 * Revision 1.2  90/02/25  11:01:21  cpcahil
 * added support for malloc chain checking.
 * 
 * Revision 1.1  90/02/24  21:50:24  cpcahil
 * Initial revision
 * 
 * Revision 1.1  90/02/24  17:10:53  cpcahil
 * Initial revision
 * 
 */
