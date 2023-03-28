// MemoryHeap.cp 
// Copyright © 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef PLATFORMMEMORY_H
#include "PlatformMemory.h" // CHANGE removed <>
#endif

#ifndef __MEMORYHEAP__
#include "MemoryHeap.h" // CHANGE removed <>
#endif

#ifndef __MEMORY__
#include <Memory.h>
#endif

#ifndef __STDIO__
#include <stdio.h>
#endif

#ifdef PROFILE
#pragma profile on
#endif


//========================================================================================
// CLASS MemoryHook (DEBUG only)
//========================================================================================

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::MemoryHook
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHook::MemoryHook()
{
	fNextHook = fPreviousHook = this;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::~MemoryHook
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHook::~MemoryHook()
{
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::AboutToAllocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FW_BlockSize MemoryHook::AboutToAllocate(FW_BlockSize size)
{
	return size;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::DidAllocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHook::DidAllocate(void* blk, FW_BlockSize)
{
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::AboutToFW_BlockSize
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHook::AboutToBlockSize(void* blk)
{
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::AboutToFree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHook::AboutToFree(void* blk)
{
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::AboutToRealloc
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHook::AboutToRealloc(void* &, FW_BlockSize &)
{
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::DidRealloc
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHook::DidRealloc(void *blk, FW_BlockSize)
{
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::AboutToReset
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHook::AboutToReset()
{
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHook::Comment
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHook::Comment(const char*)
{
}
#endif


//========================================================================================
// CLASS MemoryHookList (DEBUG only)
//========================================================================================

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::MemoryHookList
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHookList::MemoryHookList()
{
	fCurrentHook = &fHead;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::Add
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHookList::Add(MemoryHook* aMemoryHook)
{
	// Add at the fEnd of the list by adding after the last hook in the list.
	
	MemoryHook* afterHook = fHead.fPreviousHook;
	
	aMemoryHook->fNextHook = afterHook->fNextHook;
	afterHook->fNextHook->fPreviousHook = aMemoryHook;
	aMemoryHook->fPreviousHook = afterHook;
	afterHook->fNextHook = aMemoryHook;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::Remove
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHookList::Remove(MemoryHook* aMemoryHook)
{
	aMemoryHook->fPreviousHook->fNextHook = aMemoryHook->fNextHook;
	aMemoryHook->fNextHook->fPreviousHook = aMemoryHook->fPreviousHook;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::First
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHook* MemoryHookList::First()
{
	fCurrentHook = fHead.fNextHook;
	return fCurrentHook != &fHead ? fCurrentHook : NULL;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::Next
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHook* MemoryHookList::Next()
{
	if (fCurrentHook != &fHead)
		fCurrentHook = fCurrentHook->fNextHook;
	return fCurrentHook != &fHead ? fCurrentHook : NULL;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::Previous
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHook* MemoryHookList::Previous()
{
	if (fCurrentHook != &fHead)
		fCurrentHook = fCurrentHook->fPreviousHook;
	return fCurrentHook != &fHead ? fCurrentHook : NULL;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::Last
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHook* MemoryHookList::Last()
{
	fCurrentHook = fHead.fPreviousHook;
	return fCurrentHook != &fHead ? fCurrentHook : NULL;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHookList::~MemoryHookList
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHookList::~MemoryHookList()
{
}
#endif


//========================================================================================
// CLASS MemoryHeap
//========================================================================================

#ifdef DEBUG
const char*		MemoryHeap::kDefaultDescription = "MemoryHeap";
#endif

MemoryHeap*		MemoryHeap::fHeapList;								// Don't initialize!

//----------------------------------------------------------------------------------------
// MemoryHeap::Allocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void* MemoryHeap::Allocate( FW_BlockSize size )
{
	FW_BlockSize		allocatedSize;
	
#ifdef DEBUG
	size = this->CallAboutToAllocateHooks(size);
#endif
	
	void* blk = this->DoAllocate( size, allocatedSize );
	
	if ( blk )
	{
		if ( fZapOnAllocate )
		{
			char* chrBlk = (char*)blk;
			for ( FW_BlockSize i = 0; i < allocatedSize; i++ )
				*chrBlk++ = 0;
		}
			
		fBytesAllocated += allocatedSize;
		fNumberAllocatedBlocks++;
	}

#ifdef DEBUG
	if ( blk )
		blk = this->CallDidAllocateHooks( blk, size );
#endif

	return blk;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::Reallocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHeap::Reallocate(void *blk, FW_BlockSize newSize)
{
	if (blk == NULL)
		return this->Allocate(newSize);
		
	FW_BlockSize allocatedSize;
	FW_BlockSize oldBlkSize = (FW_BlockSize) this->DoBlockSize(blk);

#ifdef DEBUG
	this->CallAboutToReallocHooks(blk, newSize);
#endif
	
	// LAM why would we want things to stay LARGER????
	if (oldBlkSize >= newSize)	// ATOTIC - do not reallocate if we are smaller
		allocatedSize = newSize;
	else
		blk = this->DoReallocate(blk, newSize, allocatedSize);
	
	if (blk != NULL)
		fBytesAllocated += allocatedSize - oldBlkSize;
			
#ifdef DEBUG
	blk = this->CallDidAllocateHooks(blk, newSize);
#endif

	return blk;
}
		
//----------------------------------------------------------------------------------------
// MemoryHeap::Free
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::Free(void* blk)
{
	if (blk == NULL)
		return;
		
#ifdef DEBUG
	if (fAutoValidation)
		this->ValidateAndReport(blk);
		
	blk = this->CallAboutToFreeHooks(blk);
#endif
	
	FW_BlockSize allocatedSize = (FW_BlockSize) this->DoBlockSize(blk);
	
	if (fZapOnFree)
	{
		char *chrBlk = (char *) blk;
		for (FW_BlockSize i = 0; i < allocatedSize; i++)
			*chrBlk++ = 0;
	}

	this->DoFree(blk);

	fBytesAllocated -= allocatedSize;
	fNumberAllocatedBlocks--;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::GetFirstHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHeap* MemoryHeap::GetFirstHeap()
{
	return fHeapList;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::FW_BlockSize
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FW_BlockSize MemoryHeap::BlockSize(const void *blk) const
{
	if (blk == NULL)
		return 0;
		
#ifdef DEBUG
	if (fAutoValidation)
		this->ValidateAndReport((void*)blk);
		
	blk = ((MemoryHeap *) this)->CallAboutToFW_BlockSizeHooks((void*)blk);
#endif

	return this->DoBlockSize((void*)blk);
}

//----------------------------------------------------------------------------------------
// MemoryHeap::BytesAllocated
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

unsigned long MemoryHeap::BytesAllocated() const
{
	return fBytesAllocated;
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::GetAutoValidation (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean MemoryHeap::GetAutoValidation() const
{
	return fAutoValidation;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::GetDescription (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

const char *MemoryHeap::GetDescription() const
{
	return fDescription;
}
#endif

//----------------------------------------------------------------------------------------
// MemoryHeap::GetNextHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHeap *MemoryHeap::GetNextHeap() const
{
	return fNextHeap;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::GetZapOnAllocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean MemoryHeap::GetZapOnAllocate() const
{
	return fZapOnAllocate;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::GetZapOnFree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean MemoryHeap::GetZapOnFree() const
{
	return fZapOnFree;
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::InstallHook (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::InstallHook(MemoryHook *memoryHook)
{
	fMemoryHookList.Add(memoryHook);
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::IsValidBlock (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean MemoryHeap::IsValidBlock(void *blk) const
{
	if (blk == NULL)
		return false;
		
	if (this->IsMyBlock(blk))
		return DoIsValidBlock(blk);
	else
		return false;
}
#endif

//----------------------------------------------------------------------------------------
// MemoryHeap::NumberAllocatedBlocks
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

unsigned long MemoryHeap::NumberAllocatedBlocks() const
{
	return fNumberAllocatedBlocks;
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::RemoveHook (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::RemoveHook(MemoryHook *memoryHook)
{
	fMemoryHookList.Remove(memoryHook);
}
#endif

//----------------------------------------------------------------------------------------
// MemoryHeap::Reset
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::Reset()
{
#ifdef DEBUG
	this->CallAboutToResetHooks();
#endif
	
	fBytesAllocated = 0;
	fNumberAllocatedBlocks = 0;
	
	this->DoReset();
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::SetAutoValidation (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::SetAutoValidation(Boolean autoValidation)
{
	fAutoValidation = autoValidation;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::SetDescription (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::SetDescription(const char *description)
{
	// ------ Set the description without depending on other code
	
	const char *src = description;
	char *dst = fDescription;
	int i;
	
	for (i = 0; i < kDescriptionLength - 1 && *src; i++)
		*dst++ = *src++;
	fDescription[i] = 0;
}
#endif

//----------------------------------------------------------------------------------------
// MemoryHeap::SetZapOnAllocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::SetZapOnAllocate(Boolean zapOnAllocate)
{
	fZapOnAllocate = zapOnAllocate;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::SetZapOnFree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::SetZapOnFree(Boolean zapOnFree)
{
	fZapOnFree = zapOnFree;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::~MemoryHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHeap::~MemoryHeap()
{
	// Remove from the static list of heaps
	
	MemoryHeap *lastHeap = NULL, *currentHeap = fHeapList;
	
	while (currentHeap != NULL)
	{
		if (this == currentHeap)
		{
			if (lastHeap == NULL)
				fHeapList = currentHeap->GetNextHeap();
			else
				lastHeap->fNextHeap = currentHeap->GetNextHeap();
			
			currentHeap = NULL;
		}
		else
		{
			lastHeap = currentHeap;
			currentHeap = currentHeap->GetNextHeap();
		}
	}
}

//----------------------------------------------------------------------------------------
// MemoryHeap::MemoryHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

MemoryHeap::MemoryHeap( Boolean autoValidation, Boolean zapOnAllocate, Boolean zapOnFree )
{
#ifdef DEBUG
	// ------ Set the default description without depending on other code
	
	const char *src = kDefaultDescription;
	char *dst = fDescription;	
	for (; *src;)
		*dst++ = *src++;
	*dst = 0;
#endif

	fAutoValidation = autoValidation;
	fZapOnAllocate = zapOnAllocate;
	fZapOnFree = zapOnFree;
	fBytesAllocated = 0;
	fNumberAllocatedBlocks = 0;
	
	// Add to the static list of heaps
	fNextHeap = fHeapList;
	fHeapList = this;
}

//----------------------------------------------------------------------------------------
// MemoryHeap::operator new
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void* MemoryHeap::operator new(SIZE_T size)
{
	return PlatformAllocateBlock(size);
}

//----------------------------------------------------------------------------------------
// MemoryHeap::operator delete
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::operator delete(void* ptr)
{
	PlatformFreeBlock(ptr);
}

//----------------------------------------------------------------------------------------
// MemoryHeap::AllocateRawMemory
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHeap::AllocateRawMemory(FW_BlockSize size)
{
	void*	ptr = NewPtr( size );
	
	//FailNIL(ptr); CHANGE not usefull
	PLATFORM_ASSERT( ptr ); // CHANGE added
	
	return ptr;
/* ALEKS */
}

//----------------------------------------------------------------------------------------
// MemoryHeap::DoReallocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void* MemoryHeap::DoReallocate( void* block, FW_BlockSize newSize, FW_BlockSize &allocatedSize )
{
	FW_BlockSize oldRealSize = this->DoBlockSize( block );
	
	void* newBlock = this->DoAllocate( newSize, allocatedSize );
	
	if ( newBlock != NULL )
	{
		FW_BlockSize copySize = ( newSize <= oldRealSize ) ? newSize : oldRealSize;
		PlatformCopyMemory( block, newBlock, copySize );

		this->DoFree( block );
	}
	else if ( newSize <= oldRealSize )   //if unable to get new fMem, and newSize is <= real size
	  	return block;               //then return original ptr unchanged

	return newBlock;
}
		
//----------------------------------------------------------------------------------------
// MemoryHeap::FreeRawMemory
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::FreeRawMemory(void* ptr)
{
	PlatformFreeBlock(ptr);
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CompilerCheck (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::CompilerCheck()
{
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallAboutToAllocateHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FW_BlockSize MemoryHeap::CallAboutToAllocateHooks(FW_BlockSize size)
{
	for (MemoryHook *hook = fMemoryHookList.First();
			hook != NULL; hook = fMemoryHookList.Next())
		size = hook->AboutToAllocate(size);
		
	return size;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallDidAllocateHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHeap::CallDidAllocateHooks(void* blk, FW_BlockSize size)
{
	for (MemoryHook *hook = fMemoryHookList.First();
			hook != NULL; hook = fMemoryHookList.Next())
		blk = hook->DidAllocate(blk, size);
		
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallAboutToFW_BlockSizeHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHeap::CallAboutToFW_BlockSizeHooks(void* blk)
{
	for (MemoryHook *hook = fMemoryHookList.Last();
			hook != NULL; hook = fMemoryHookList.Previous())
		blk = hook->AboutToBlockSize(blk);
		
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallAboutToFreeHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHeap::CallAboutToFreeHooks(void* blk)
{
	for (MemoryHook *hook = fMemoryHookList.Last();
			hook != NULL; hook = fMemoryHookList.Previous())
		blk = hook->AboutToFree(blk);
		
	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallAboutToReallocHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::CallAboutToReallocHooks(void* &blk, FW_BlockSize& size)
{
	for (MemoryHook *hook = fMemoryHookList.Last();
			hook != NULL; hook = fMemoryHookList.Previous())
		hook->AboutToRealloc(blk, size);
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallDidReallocHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void *MemoryHeap::CallDidReallocHooks(void *blk, FW_BlockSize size)
{
	for (MemoryHook *hook = fMemoryHookList.First();
			hook != NULL; hook = fMemoryHookList.Next())
		blk = hook->DidRealloc(blk, size);

	return blk;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallAboutToResetHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::CallAboutToResetHooks()
{
	for (MemoryHook *hook = fMemoryHookList.First();
			hook != NULL; hook = fMemoryHookList.Next())
		hook->AboutToReset();
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::CallCommentHooks (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::CallCommentHooks(const char* comment)
{
	for (MemoryHook *hook = fMemoryHookList.First();
			hook != NULL; hook = fMemoryHookList.Next())
		hook->Comment(comment);
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// MemoryHeap::ValidateAndReport (DEBUG only)
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void MemoryHeap::ValidateAndReport(void *blk) const
{
	PLATFORM_ASSERT(this->IsValidBlock(blk));
}
#endif

#ifdef PROFILE
#pragma profile off
#endif

