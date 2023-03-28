
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
static char rcs_hdr[] = "$Id: xheap.c,v 1.1 1996/06/18 03:29:32 warren Exp $";
#endif

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

/*
 * X Toolkit Memory Allocation Routines
 *
 * Uses Xlib memory management, which is spec'd to be re-entrant.
 */

#if FOUND_X_INTRINSIC
#include "X11/Intrinsic.h"
#endif

#include "mallocin.h"

#ifndef NULL
#define NULL ((char *)0)
#endif

void _XtHeapInit(heap)
    Heap*	heap;
{
    heap->start = NULL;
    heap->bytes_remaining = 0;
}

#ifndef HEAP_SEGMENT_SIZE
#define HEAP_SEGMENT_SIZE 1492
#endif

char* _XtHeapAlloc(heap, bytes)
    Heap*	heap;
    Cardinal	bytes;
{
    register char* heap_loc;
    if (heap == (Heap *) NULL) return XtMalloc(bytes);
    if (heap->bytes_remaining < (int)bytes) {
	if ((bytes + sizeof(char*)) >= (HEAP_SEGMENT_SIZE>>1)) {
	    /* preserve current segment; insert this one in front */
#ifdef _TRACE_HEAP
	    printf( "allocating large segment (%d bytes) on heap %#x\n",
		    bytes, heap );
#endif
	    heap_loc = XtMalloc(bytes + sizeof(char*));
	    if (heap->start) {
		*(char**)heap_loc = *(char**)heap->start;
		*(char**)heap->start = heap_loc;
	    }
	    else {
		*(char**)heap_loc = NULL;
		heap->start = heap_loc;
	    }
	    return heap_loc + sizeof(char*);
	}
	/* else discard remainder of this segment */
#ifdef _TRACE_HEAP
	printf( "allocating new segment on heap %#x\n", heap );
#endif
	heap_loc = XtMalloc((unsigned)HEAP_SEGMENT_SIZE);
	*(char**)heap_loc = heap->start;
	heap->start = heap_loc;
	heap->current = heap_loc + sizeof(char*);
	heap->bytes_remaining = HEAP_SEGMENT_SIZE - sizeof(char*);
    }
#ifdef WORD64
    /* round to nearest 8-byte boundary */
    bytes = (bytes + 7) & (~7);
#else
    /* round to nearest 4-byte boundary */
    bytes = (bytes + 3) & (~3);
#endif /* WORD64 */
    heap_loc = heap->current;
    heap->current += bytes;
    heap->bytes_remaining -= bytes; /* can be negative, if rounded */
    return heap_loc;
}

void _XtHeapFree(heap)
    Heap*	heap;
{
    char* segment = heap->start;
    while (segment != NULL) {
	char* next_segment = *(char**)segment;
	XtFree(segment);
	segment = next_segment;
    }
    heap->start = NULL;
    heap->bytes_remaining = 0;
}
