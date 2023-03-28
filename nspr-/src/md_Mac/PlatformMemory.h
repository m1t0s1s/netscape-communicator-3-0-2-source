// PlatformMemory.h 
// Copyright © 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef PLATFORMMEMORY_H
#define PLATFORMMEMORY_H

#ifndef __STDDEF__
#include <stddef.h>
#endif

#ifndef __MEMORY__
#include <Memory.h>
#endif

#ifndef __STDIO__
#include <stdio.h>
#endif

#ifdef __xlC
typedef unsigned long SIZE_T;
#else
typedef unsigned int SIZE_T;
#endif

#include "prlog.h"

//========================================================================================
// Debugging
//========================================================================================

#define PLATFORM_ASSERT 			PR_ASSERT
#define PLATFORM_DEBUGGER_STRING(X) DebugStr("\p"X);
#define PLATFORM_PRINT_STRING 		PR_LOG

//========================================================================================
// Global function definitions
//========================================================================================

//----------------------------------------------------------------------------------------
// PlatformAllocateBlock
//----------------------------------------------------------------------------------------

inline void *PlatformAllocateBlock(SIZE_T size)
{
	void *ptr = NewPtr(size);
	PR_ASSERT(ptr);
	
	return ptr;
}

//----------------------------------------------------------------------------------------
// PlatformFreeBlock
//----------------------------------------------------------------------------------------

inline void PlatformFreeBlock(void *ptr)
{
	DisposePtr((Ptr) ptr);
}

//----------------------------------------------------------------------------------------
// PlatformCopyMemory
//----------------------------------------------------------------------------------------

inline void PlatformCopyMemory(void *source, void *destination, size_t length)
{
	BlockMoveData(source, destination, (Size) length);
}

#endif
