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
static char rcs_hdr[] = "$Id: xmalloc.c,v 1.1 1996/06/18 03:29:32 warren Exp $";
#endif

/* $XConsortium: Alloc.c,v 1.46 91/07/30 11:04:41 rws Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <stdio.h>

#include "sysdefs.h"

#if FOUND_X_INTRINSIC
#include "X11/Intrinsic.h"
#endif

#include "mallocin.h"

void _XtAllocError(type)
    CONST char * type;
{
    Cardinal num_params = 1;
    extern String XtCXtToolkitError;

    if (type == NULL) type = "local memory allocation";
    XtErrorMsg("allocError", type, XtCXtToolkitError,
	       "Cannot perform %s", &type, &num_params);
}

void
_XtBCopy(b1, b2, length)
	char		* b1;
	char		* b2;
	int		  length;
{
	DBFmemcpy("_XtBCopy", (char *)NULL, 0, b2, b1, (MEMSIZE)length);
}

void
debug_XtBcopy(file, line, b1, b2, length)
	char 		* file;
	int		  line;
	char		* b1;
	char		* b2;
	int		  length;
{
	DBFmemcpy("_XtBCopy", file, line, b2, b1, (MEMSIZE) length);
}

char *
XtMalloc(size)
	unsigned int	  size;
{
	return( debug_XtMalloc((char *)NULL, 0, size) );
}

char *
debug_XtMalloc(file,line,size)
	CONST char	* file;
	int		  line;
	unsigned int	  size;
{
	static IDTYPE	  call_counter;
	char		* ptr;

	/*
	 * increment call counter
	 */
	call_counter++;

	ptr = (char *) DBFmalloc("XtMalloc",M_T_XTMALLOC, call_counter,
				  file, line, (SIZETYPE)size);

	if( ptr == NULL )
	{
        	_XtAllocError("malloc");
	}

	return(ptr);
}

char *
XtRealloc(ptr, size)
	char		* ptr;
	unsigned int	  size;
{
	return( debug_XtRealloc((char *)NULL,0,ptr,size) );
}

char *
debug_XtRealloc(file,line,ptr, size)
	CONST char	* file;
	int		  line;
	char   		* ptr;
	unsigned int	  size;
{
	static IDTYPE	  call_counter;

	/*
	 * increment call counter
	 */
	call_counter++;
	
	ptr = (char *) DBFrealloc("XtRealloc",M_T_XTREALLOC, call_counter,
				  file,line,ptr,(SIZETYPE)size);

	if( ptr == NULL )
	{
        	_XtAllocError("realloc");
	}

	return(ptr);
}

char *
XtCalloc(num, size)
	unsigned int	  num;
	unsigned int	  size;
{
	return( debug_XtCalloc((char *)NULL, 0, num,size) );
}

char *
debug_XtCalloc(file,line,num,size)
	CONST char	* file;
	int		  line;
	unsigned int	  num;
	unsigned int	  size;
{
	static IDTYPE	  call_counter;
	char		* ptr;

	/*
	 * increment call counter
	 */
	call_counter++;

	ptr = (char *) DBFcalloc("XtCalloc",M_T_XTCALLOC, call_counter,
				  file,line,(SIZETYPE)num,(SIZETYPE)size);

	if( ptr == NULL )
	{
        	_XtAllocError("calloc");
	}

	return(ptr);
}

void
XtFree(ptr)
	char	* ptr;
{
	debug_XtFree( (char *) NULL, 0, ptr);
}

void
debug_XtFree(file,line,ptr)
	CONST char 	* file;
	int		  line;
	char		* ptr;
{
	static IDTYPE	  call_counter;
	
	/*
	 * increment call counter
	 */
	call_counter++;

	DBFfree("XtFree",F_T_XTFREE,call_counter,file,line,ptr);
}


#ifndef DONT_FORCE_HEAPSTUFF

void
NeverCalledFunctionFromAnywhere()
{
	_XtHeapInit(NULL);
}

#endif

