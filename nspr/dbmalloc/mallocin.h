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
/*
 * $Id: mallocin.h,v 1.1 1996/06/18 03:29:21 warren Exp $
 */
#ifndef _MALLOCIN_H
#define _MALLOCIN_H 1

/*
 * this file contains macros that are internal to the malloc library
 * and therefore, are not needed in malloc.h.
 */

#if POSIX_HEADERS
#include <unistd.h>
#endif
#if ANSI_HEADERS
#include <stdlib.h>
#endif

#ifndef SYS_TYPES_H_INCLUDED
#include <sys/types.h>
#endif

#define IN_MALLOC_CODE	1
#include "sysdefs.h"
#include "malloc.h"

#define IDTYPE unsigned long
#define ULONG  unsigned long

/*
 * Data structures used by stack mechanism
 */
struct stack
{
	struct stack	* beside;
	struct stack	* below;
	struct stack 	* above;
	CONST char	* func;
	CONST char	* file;
	int		  line;
};

/*
 * minimum round up to get to a doubleword boundry, assuming it is the
 * strictest alignment requirement on the system.  If not, the union s
 * in struct mlist will also have to be changed.  Note that this must be
 * a power of two because of how it is used throughout the code.
 */
#define M_RND		0x08
#define M_YOFFSET       (sizeof(SIZETYPE))

#define LONGFILL 2

struct mlist
{
	struct mlist	* next;			/* next entry in chain	*/
	struct mlist	* prev;			/* prev entry in chain	*/
	struct mlist	* freenext;		/* next ent in free chn */
	struct mlist	* freeprev;		/* prev ent in free chn */
	struct stack	* stack;		/* current stack level	*/
	struct stack	* freestack;		/* free stack level	*/
	long	 	  flag;			/* inuse flag		*/
	CONST char	* file;			/* file where called fm	*/
	int		  line;			/* line where called fm	*/
	CONST char	* ffile;		/* file where freed 	*/
	int		  fline;		/* line where freed 	*/
	IDTYPE		  fid;			/* free call number	*/
	IDTYPE		  hist_id;		/* historical id	*/
	IDTYPE		  id;			/* malloc call number	*/
	SIZETYPE	  r_size;		/* requested size	*/
	union
	{
		SIZETYPE	  size;		/* actual size		*/
		SIZETYPE	  filler[LONGFILL];
		double		  unused_just_for_alignment;
	} s;
	char		  data[M_RND];
};

/*
 * pre-filler size
 */
#define FILLSIZE	(sizeof(mlist.s) - sizeof(mlist.s.filler[0]))

/*
 * kludge to get offsetof the data element for the mlist structure
 */
#define M_SIZE		((SIZETYPE)(char *)((struct mlist *)0)->data)
#define M_FLAGOFF	((SIZETYPE)(char *)&((struct mlist *)0)->flag)

#define M_INUSE 	0x01
#define M_FILLED 	0x02
#define M_MARKED	0x04
#define M_DFILLED	0x08
#define M_MAGIC 	0x31561000
#define M_MAGIC_BITS	0xfffff000

#define M_BLOCKSIZE	(1024*8)

#define EOS			'\0'
#define M_DFLT_FILL		'\01'
#define M_DFLT_FREE_FILL	'\02'
#define M_DFLT_BSIZE		1
#define M_FILL			((char)malloc_fillbyte)
#define M_FREE_FILL		((char)malloc_freebyte)
#define M_BSIZE			(malloc_boundsize)

#define MALLOC_INIT()	if( malloc_data_start == (DATATYPE *)0 ) malloc_init()

/* 
 * malloc internal options
 */
#define MOPT_MFILL	0x0001		/* fill malloc'd segments	*/
#define MOPT_FFILL	0x0002		/* fill free'd segments		*/
#define MOPT_REUSE	0x0004		/* reuse free segments?		*/
#define MOPT_LOWFRAG	0x0008		/* low fragmentation		*/
#define MOPT_CKCHAIN	0x0010		/* enable chain checking	*/
#define MOPT_CKDATA	0x0020		/* enable data checking		*/
#define MOPT_DFILL	0x0040		/* fill over/underflow areas	*/
#define MOPT_SLINKS     0x0080		/* turn off/on adjacent link disp */
#define MOPT_DETAIL     0x0100		/* turn off/on detailed output  */
#define MOPT_FREEMARK   0x0200		/* warning when freeing marked	*/
#define MOPT_ZERO       0x0400		/* warning about zero length allocs */

/*
 * malloc_freeseg() operation arguments
 */
#define M_FREE_REMOVE	1
#define M_FREE_ADD	2
/*
 * Malloc types
 */
#define M_T_MALLOC	0x010
#define M_T_REALLOC	0x020
#define M_T_CALLOC	0x030
#define M_T_SPLIT	0x040
#define M_T_STACK	0x050
#define M_T_XTMALLOC	0x060
#define M_T_XTREALLOC	0x070
#define M_T_XTCALLOC	0x080
#define M_T_ALIGNED	0x090
#define M_T_BITS	0x0F0

#define GETTYPE(_ptr)		(_ptr->flag & M_T_BITS)
#define SETTYPE(_ptr,_type) 	(_ptr->flag = ((_ptr->flag & ~M_T_BITS)|_type))

#define DATATOMLIST(_ptr) ((struct mlist *) (((char *)_ptr) - M_SIZE))

/*
 * Free types
 */
#define F_T_FREE	0x100
#define F_T_CFREE	0x200
#define F_T_XTFREE	0x300
#define F_T_REALLOC	0x400
#define F_T_BITS	0xF00

#define GETFTYPE(_ptr)		(_ptr->flag & F_T_BITS)
#define SETFTYPE(_ptr,_type) 	(_ptr->flag = ((_ptr->flag & ~F_T_BITS)|_type))

/*
 * Fill types
 */
#define FILL_MALLOC	1
#define FILL_REALLOC	2
#define FILL_CALLOC	3
#define FILL_FREE	4
#define FILL_SPLIT	5
#define FILL_JOIN	6

#define FILLCHECK(a,b,c,d,e) ( malloc_fill ? FillCheck(a,b,c,d,e) : 0 )
#define FILLDATA(a,b,c,d)     if( malloc_fill ) FillData(a,b,c,d)

/*
 * malloc_chain_check flags
 */
#define SHOWERRORS	1
#define DONTSHOWERRS	0

/*
 * DBFmalloc fill flags
 */
#define DOFILL		1
#define DONTFILL	0

/*
 * malloc join flags
 */
#define NOTINUSE	0	/* don't join inuse segments		*/
#define INUSEOK		1	/* ok to join if current seg is in use	*/
#define NEXTPTR_INUSEOK	2
#define ANY_INUSEOK	3

/*
 * number of longwords to search for magic number before we give up and 
 * search the entire malloc list.
 */
#define MAX_KLUDGE_CHECKS	100
/*
 * Misc stuff..
 */
#define M_ROUNDUP(size)	{\
				if( size & (M_RND-1) ) \
				{ \
					size += (M_RND-1); \
					size &= ~(M_RND-1); \
				} \
			}

EXITTYPE	  exit __STDCARGS((int));
char		* getenv __STDCARGS((CONST char *));
DATATYPE	* sbrk __STDCARGS((int));

/*
 * stuff for X compatibility routines (needed here so that the prototypes 
 * don't fail
 */
typedef struct {
    char*       start;
    char*       current;
    int         bytes_remaining;
} Heap;

#ifndef _XtIntrinsic_h
typedef unsigned int Cardinal;
typedef char * String;
#endif

#include "prototypes.h"

/*
 * global variables
 */
extern int		  in_malloc_code;
extern SIZETYPE		  malloc_align;
extern int		  malloc_boundsize;
extern DATATYPE		* malloc_data_start;
extern DATATYPE		* malloc_data_end;
extern struct mlist 	* malloc_end;
extern int		  malloc_errfd;
extern int		  malloc_errno;
extern CONST char	* malloc_err_strings[];
extern int		  malloc_fatal_level;
extern int		  malloc_fill;
extern int		  malloc_fillbyte;
extern int		  malloc_freebyte;
extern struct mlist	* malloc_freelist;
extern long		  malloc_hist_id;
extern int		  malloc_opts;
extern int		  malloc_round;
extern struct mlist	  malloc_start;
extern int		  malloc_warn_level;

#endif /* _MALLOCIN_H */

/*
 * $Log: mallocin.h,v $
 * Revision 1.1  1996/06/18 03:29:21  warren
 * Added debug malloc package.
 *
 * Revision 1.27  1992/09/03  22:24:33  cpcahil
 * final changes for PL14
 *
 * Revision 1.26  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.25  1992/07/12  15:30:58  cpcahil
 * Merged in Jonathan I Kamens' changes
 *
 * Revision 1.24  1992/07/03  00:03:25  cpcahil
 * more fixes for pl13, several suggestons from Rich Salz.
 *
 * Revision 1.23  1992/07/02  13:49:54  cpcahil
 * added support for new malloc_size function and additional tests to testerr
 *
 * Revision 1.22  1992/06/30  13:06:39  cpcahil
 * added support for aligned allocations
 *
 * Revision 1.21  1992/06/22  23:40:10  cpcahil
 * many fixes for working on small int systems
 *
 * Revision 1.20  1992/05/09  00:16:16  cpcahil
 * port to hpux and lots of fixes
 *
 * Revision 1.19  1992/05/08  02:30:35  cpcahil
 * minor cleanups from minix/atari port
 *
 * Revision 1.18  1992/05/08  01:44:11  cpcahil
 * more performance enhancements
 *
 * Revision 1.17  1992/05/06  05:37:44  cpcahil
 * added overriding of fill characters and boundary size
 *
 * Revision 1.16  1992/05/06  04:53:29  cpcahil
 * performance enhancments
 *
 * Revision 1.15  1992/04/24  12:09:13  cpcahil
 * (hopefully) final cleanup for patch 10
 *
 * Revision 1.14  1992/04/24  11:18:52  cpcahil
 * Fixes from Denny Page and Better integration of Xt alloc hooks
 *
 * Revision 1.13  1992/04/22  18:17:32  cpcahil
 * added support for Xt Alloc functions, linted code
 *
 * Revision 1.12  1992/04/20  22:29:14  cpcahil
 * changes to fix problems introduced by insertion of size_t
 *
 * Revision 1.11  1992/04/13  18:26:30  cpcahil
 * changed sbrk arg to int which it should be on all systems.
 *
 * Revision 1.10  1992/04/13  14:16:11  cpcahil
 * ansi changes corresponding to changes made in stack.c
 *
 * Revision 1.9  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.8  1992/03/01  12:42:38  cpcahil
 * added support for managing freed areas and fixed doublword bndr problems
 *
 * Revision 1.7  1991/12/31  21:31:26  cpcahil
 * changes for patch 6.  See CHANGES file for more info
 *
 * Revision 1.6  1991/12/06  08:54:18  cpcahil
 * cleanup of __STDC__ usage and addition of CHANGES file
 *
 * Revision 1.5  91/12/04  18:01:22  cpcahil
 * cleand up some aditional warnings from gcc -Wall
 * 
 * Revision 1.4  91/12/04  09:23:42  cpcahil
 * several performance enhancements including addition of free list
 * 
 * Revision 1.3  91/12/02  19:10:12  cpcahil
 * changes for patch release 5
 * 
 * Revision 1.2  91/11/25  14:42:02  cpcahil
 * Final changes in preparation for patch 4 release
 * 
 * Revision 1.1  91/11/24  00:49:30  cpcahil
 * first cut at patch 4
 * 
 */
