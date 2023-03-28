
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
#include "tostring.h"

#ifndef lint
static
char rcs_hdr[] = "$Id: dump.c,v 1.1 1996/06/18 03:29:15 warren Exp $";
#endif

/*
 * various macro definitions used within this module.
 */

#define WRITEOUT(fd,str,len)	if( write(fd,str,(WRTSIZE)(len)) != (len) ) \
				{ \
					VOIDCAST write(2,ERRSTR,\
						     (WRTSIZE)strlen(ERRSTR));\
					exit(120); \
				}

#define DETAIL_NONE 		0
#define DETAIL_NOT_SET_YET	-1
#define DETAIL_ST_COL		(sizeof(DETAIL_HDR_3)-1)
#define ERRSTR	"I/O Error on malloc dump file descriptor\n"
#define FILE_LEN		20
#define LIST_ALL		1
#define LIST_SOME		2
#define NUM_BYTES		7
#define TITLE			" Dump of Malloc Chain "

#define DETAIL_HDR_1 \
     "                             FREE     FREE                  ACTUAL SIZE    "
#define DETAIL_HDR_2 \
     "  PTR      NEXT     PREV     NEXT     PREV      FLAGS       INT    HEX     "
#define DETAIL_HDR_3 \
     "-------- -------- -------- -------- -------- ---------- -------- --------- "

#define NORMAL_HDR_1 \
 "POINTER     FILE  WHERE         LINE      ALLOC        DATA     HEX DUMP   \n"
#define NORMAL_HDR_2 \
 "TO DATA      ALLOCATED         NUMBER     FUNCT       LENGTH  OF BYTES 1-7 \n"
#define NORMAL_HDR_3 \
 "-------- -------------------- ------- -------------- ------- --------------\n"

#define THISBUFSIZE (sizeof(NORMAL_HDR_3)+sizeof(DETAIL_HDR_3))


/*
 * Function:	malloc_dump()
 *
 * Purpose:	to dump a printed copy of the malloc chain and
 *		associated data elements
 *
 * Arguments:	fd	- file descriptor to write data to
 *
 * Returns:	nothing of any use
 *
 * Narrative:	Just print out all the junk
 *
 */
VOIDTYPE
malloc_dump(fd)
	int		fd;
{
	malloc_list_items(fd,LIST_ALL,0L,0L);
}

/*
 * Function:	malloc_list()
 *
 * Purpose:	to dump a printed copy of the malloc chain and
 *		associated data elements
 *
 * Arguments:	fd	- file descriptor to write data to
 *		histid1 - id of the first record to display
 *		histid2 - id one above the last record to display
 *
 * Returns:	nothing of any use
 *
 * Narrative:	Just call malloc_list_items to display the junk
 *
 */
VOIDTYPE
malloc_list(fd,histid1,histid2)
	int		fd;
	IDTYPE		histid1;
	IDTYPE		histid2;
{
	malloc_list_items(fd, LIST_SOME, histid1, histid2);
}

/*
 * Function:	malloc_list_items()
 *
 * Purpose:	to dump a printed copy of the malloc chain and
 *		associated data elements
 *
 * Arguments:	fd	  - file descriptor to write data to
 *		list_type - type of list (all records, or a selected list)
 *		histid1	  - first id to list (if type is some)
 *		histid2   - one above last id to list
 *
 * Returns:	nothing of any use
 *
 * Narrative:	Just print out all the junk
 *
 * Notes:	This function is implemented using low level calls because
 * 		of the likelyhood that the malloc tree is damaged when it
 *		is called.  (Lots of things in the c library use malloc and
 *		we don't want to get into a catch-22).
 *
 */
VOIDTYPE
malloc_list_items(fd,list_type,histid1,histid2)
	int			  fd;
	int			  list_type;
	IDTYPE			  histid1;
	IDTYPE			  histid2;
{
	char			  buffer[THISBUFSIZE];
	int			  detail;
	int			  first_time = 1;
	CONST char		* func;
	int			  i;
	int			  loc;
	struct mlist 		* ptr;

	MALLOC_INIT();

	if( (malloc_opts & MOPT_DETAIL) != 0 )
	{
		detail = DETAIL_ST_COL;
	}
	else
	{
		detail = DETAIL_NONE;
	}

	/*
	 * for each element in the trace
	 */
	for(ptr = &malloc_start; ptr; ptr = ptr->next)
	{
		/*
		 * if this item is not in use or it is a stack element
		 *     and we are not in detail mode or list-all mode.
		 */
		if(  ( ((ptr->flag & M_INUSE)==0) || (GETTYPE(ptr)==M_T_STACK) )
		    && ((detail == DETAIL_NONE) || (list_type != LIST_ALL)) )
		{
			continue;
		}
		/*
		 * else if we are only listing a range of items, check to see
		 * if this item is in the correct range and is not marked. 
		 * if not, skip it
	 	 */
		else if(   (list_type == LIST_SOME)
			&& (    (ptr->hist_id < histid1)
			     || (ptr->hist_id >= histid2)
			     || ((ptr->flag & M_MARKED) != 0) ) )
		{
			continue;
		}

		/*
		 * if this is the first time, put out the headers.
		 */
		if( first_time )
		{
			/*
			 * fill the title line with asterisks
			 */
			for(i=0; i < (detail + sizeof(NORMAL_HDR_3)-1); i++)
			{
				buffer[i] = '*';
			}
			buffer[i] = '\n';
			buffer[i+1] = EOS;

			/*
			 * add in the title  (centered, of course)
			 */
			loc = (i - sizeof(TITLE)) / 2;
			for(i=0; i < (sizeof(TITLE)-1); i++)
			{
				buffer[loc+i] = TITLE[i];
			}

			/*
			 * and write it out
			 */
			WRITEOUT(fd,buffer,strlen(buffer));

			/*
			 * write out the column headers
			 */
			if( detail != DETAIL_NONE )
			{
				WRITEOUT(fd,DETAIL_HDR_1,
						sizeof(DETAIL_HDR_1)-1);
				WRITEOUT(fd,NORMAL_HDR_1,
						sizeof(NORMAL_HDR_1)-1);
				WRITEOUT(fd,DETAIL_HDR_2,
						sizeof(DETAIL_HDR_2)-1);
				WRITEOUT(fd,NORMAL_HDR_2,
						sizeof(NORMAL_HDR_2)-1);
				WRITEOUT(fd,DETAIL_HDR_3,
						sizeof(DETAIL_HDR_3)-1);
				WRITEOUT(fd,NORMAL_HDR_3,
						sizeof(NORMAL_HDR_3)-1);
			}
			else
			{
				WRITEOUT(fd,NORMAL_HDR_1,
						sizeof(NORMAL_HDR_1)-1);
				WRITEOUT(fd,NORMAL_HDR_2,
						sizeof(NORMAL_HDR_2)-1);
				WRITEOUT(fd,NORMAL_HDR_3,
						sizeof(NORMAL_HDR_3)-1);
			}

			first_time = 0;
		}

		/*
		 * fill in the string with blanks
		 */
		for(i=0; i < (sizeof(DETAIL_HDR_3)+sizeof(NORMAL_HDR_3)); i++)
		{
			buffer[i] = ' ';
		}

		/*
	 	 * handle detail output
		 */
		if( detail != DETAIL_NONE )
		{
			VOIDCAST tostring(buffer,
					(ULONG)ptr,8,B_HEX,'0');
			VOIDCAST tostring(buffer+9,
					(ULONG)ptr->next,8,B_HEX,'0');
			VOIDCAST tostring(buffer+18,
					(ULONG)ptr->prev,8,B_HEX,'0');
			VOIDCAST tostring(buffer+27,
					(ULONG)ptr->freenext,8,B_HEX,'0');
			VOIDCAST tostring(buffer+36,
					(ULONG)ptr->freeprev,8,B_HEX,'0');
			VOIDCAST tostring(buffer+45,
					(ULONG)ptr->flag,10,B_HEX,'0');
			VOIDCAST tostring(buffer+56,
					(ULONG)ptr->s.size,8,B_DEC,' ');
			VOIDCAST tostring(buffer+65,
					(ULONG)ptr->s.size,8,B_HEX,'0');
			buffer[64] = '(';
			buffer[74] = ')';
		}

		/*
		 * and now add in the normal stuff
		 */
	 	VOIDCAST tostring(buffer+detail,
				(ULONG) ptr->data, 8, B_HEX, '0');

		/*
		 * if a file has been specified
		 */
		if( (ptr->file != NULL)	 && (ptr->file[0] != EOS) )
		{

			for(i=0; (i < FILE_LEN) && (ptr->file[i] != EOS); i++)
			{
				buffer[detail+9+i] = ptr->file[i];
			}

			VOIDCAST tostring(buffer+detail+30,
						(ULONG)ptr->line,7,B_DEC, ' ');
		}
		else
		{
			for(i=0; i < (sizeof("unknown")-1); i++)
			{
				buffer[detail+9+i] = "unknown"[i];
			}
		}
			
		func = MallocFuncName(ptr);
		/*
		 * copy the function name into the string.
		 */
		for( i=0; func[i] != EOS; i++)
		{
			buffer[detail+38+i] = func[i];
		}

		/*
		 * add the call number
		 */
		buffer[detail+38+ i++] = '(';
		i += tostring(buffer+detail+38+i,(ULONG)ptr->id,0,B_DEC,' ');
		buffer[detail+38+i] = ')';
	
		/*
		 * display the length of the segment
		 */
		VOIDCAST tostring(buffer+detail+53,
				(ULONG)ptr->r_size,7,B_DEC,' ');

		/* 
		 * display the first seven bytes of data
		 */
		for( i=0; (i < NUM_BYTES) && (i < ptr->r_size); i++)
		{
			VOIDCAST tostring(buffer+detail + 61 + (i * 2),
					(ULONG)ptr->data[i], 2, B_HEX, '0');
		}

		buffer[detail + sizeof(NORMAL_HDR_3)] = '\n';
		WRITEOUT(fd,buffer,detail+sizeof(NORMAL_HDR_3)+1);

		/*
		 * and dump any stack info (if it was specified by the user)
		 */
		StackDump(fd,(char *) NULL,ptr->stack);

	}

	if( detail != DETAIL_NONE )
	{
		WRITEOUT(fd,"Malloc start:      ",19);
		VOIDCAST tostring(buffer, (ULONG)&malloc_start, 10, B_HEX, '0');
		buffer[10] = '\n';
		WRITEOUT(fd,buffer,11);

		WRITEOUT(fd,"Malloc end:        ", 19);
		VOIDCAST tostring(buffer, (ULONG) malloc_end, 10, B_HEX, '0');
		buffer[10] = '\n';
		WRITEOUT(fd,buffer,11);

		WRITEOUT(fd,"Malloc data start: ", 19);
		VOIDCAST tostring(buffer,(ULONG)malloc_data_start,10,B_HEX,'0');
		buffer[10] = '\n';
		WRITEOUT(fd,buffer,11);

		WRITEOUT(fd,"Malloc data end:   ", 19);
		VOIDCAST tostring(buffer,(ULONG)malloc_data_end, 10,B_HEX, '0');
		buffer[10] = '\n';
		WRITEOUT(fd,buffer,11);

		WRITEOUT(fd,"Malloc free list:  ", 19);
		VOIDCAST tostring(buffer, (ULONG)malloc_freelist, 10,B_HEX,'0');
		buffer[10] = '\n';
		WRITEOUT(fd,buffer,11);

		for(ptr=malloc_freelist->freenext; ptr!=NULL; ptr=ptr->freenext)
		{
			WRITEOUT(fd,"                -> ", 19);
			VOIDCAST tostring(buffer, (ULONG) ptr, 10, B_HEX, '0');
			buffer[10] = '\n';
			WRITEOUT(fd,buffer,11);
		}

	}

	WRITEOUT(fd,"\n",1);
	
} /* malloc_dump(... */


/*
 * $Log: dump.c,v $
 * Revision 1.1  1996/06/18 03:29:15  warren
 * Added debug malloc package.
 *
 * Revision 1.20  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
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
 * Revision 1.16  1992/04/24  12:09:13  cpcahil
 * (hopefully) final cleanup for patch 10
 *
 * Revision 1.15  1992/04/22  18:17:32  cpcahil
 * added support for Xt Alloc functions, linted code
 *
 * Revision 1.14  1992/04/15  12:51:06  cpcahil
 * fixes per testing of patch 8
 *
 * Revision 1.13  1992/04/14  02:27:30  cpcahil
 * adjusted output of pointes so that values that had the high bit
 * set would print correctly.
 *
 * Revision 1.12  1992/04/13  19:57:15  cpcahil
 * more patch 8 fixes
 *
 * Revision 1.11  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.10  1992/01/30  12:23:06  cpcahil
 * renamed mallocint.h -> mallocin.h
 *
 * Revision 1.9  1992/01/10  17:28:03  cpcahil
 * Added support for overriding void datatype
 *
 * Revision 1.8  1991/12/04  09:23:36  cpcahil
 * several performance enhancements including addition of free list
 *
 * Revision 1.7  91/11/25  14:41:52  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.6  91/11/24  00:49:25  cpcahil
 * first cut at patch 4
 * 
 * Revision 1.5  90/08/29  21:22:37  cpcahil
 * miscellaneous lint fixes
 * 
 * Revision 1.4  90/05/11  00:13:08  cpcahil
 * added copyright statment
 * 
 * Revision 1.3  90/02/24  21:50:07  cpcahil
 * lots of lint fixes
 * 
 * Revision 1.2  90/02/24  17:27:48  cpcahil
 * changed $header to $Id to remove full path from rcs id string
 * 
 * Revision 1.1  90/02/22  23:17:43  cpcahil
 * Initial revision
 * 
 */
