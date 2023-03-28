// MemoryHeap.h 
// Copyright й 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef __MEMORYHEAP__
#define __MEMORYHEAP__

#ifndef PLATFORMMEMORY_H
#include "PlatformMemory.h" // CHANGE removed <>
#endif

#ifndef __STDDEF__
#include <stddef.h>
#endif

#ifndef __TYPES__
#include <Types.h>
#endif

#ifdef DEBUG
//#include <iostream.h>
#endif

// еее JS NEW еее MW
#define SIZE_T size_t

//========================================================================================
// Forward class declarations
//========================================================================================

class MemoryHookList;


//========================================================================================
// Type definitions
//========================================================================================

typedef unsigned long			FW_BytePtr;
typedef unsigned long			FW_BlockSize;

#ifdef DEBUG
//========================================================================================
// MemoryHook (DEBUG only)
//
//		A Memory hook that can be used to track heap operations by subclassing MemoryHook
//		and overriding the methods of interest. Your subclass is then registered with a
//		MemoryHeap.
//
//========================================================================================

class MemoryHook
{
public:
	// These methods should all be abstract, but MemoryHookList instantiates a
	// MemoryHook to use as the head of its linked list, so here we just use empty
	// inlines. A MemoryHook still cannot be instantiated because the constructor
	// is protected.

	virtual FW_BlockSize AboutToAllocate(FW_BlockSize size);
	virtual void* DidAllocate(void* blk, FW_BlockSize);
	virtual void* AboutToBlockSize(void* blk);
	virtual void* AboutToFree(void* blk);
	virtual void AboutToRealloc(void*& , FW_BlockSize&);
	virtual void* DidRealloc(void* blk, FW_BlockSize);
	virtual void AboutToReset();
	virtual void Comment(const char*);

	virtual~ MemoryHook();

protected:
	MemoryHook();

private:
	MemoryHook* fNextHook, * fPreviousHook;

	friend MemoryHookList;
	
	MemoryHook(const MemoryHook& blk);
	MemoryHook& operator=(const MemoryHook& blk);
		// This class shouldn't be copied.
};
#endif


#ifdef DEBUG
//========================================================================================
// MemoryHookList (DEBUG only)
//
//		A list of memory hooks. This is a special linked list that assumes the links for
//		the list are fields of MemoryHook. This avoids endless recursion were we to
//		allocate nodes for the MemoryHooks here.
//
//========================================================================================

class MemoryHookList
{
public:
	MemoryHookList();

	void Add(MemoryHook* aMemoryHook);
	void Remove(MemoryHook* aMemoryHook);
	MemoryHook* First();
	MemoryHook* Next();
	MemoryHook* Previous();
	MemoryHook* Last();

	~MemoryHookList();

private:
	MemoryHook fHead;
	MemoryHook* fCurrentHook;
	
	MemoryHookList(const MemoryHookList& blk);
	MemoryHookList& operator=(const MemoryHookList& blk);
		// This class shouldn't be copied.
};
#endif


//========================================================================================
// MemoryHeap
//
//		Abstract base class for memory heaps.
//
//========================================================================================

class MemoryHeap
{
public:

	void*					Allocate(FW_BlockSize size);
	void					Free( void* );
	void*					Reallocate( void* block, FW_BlockSize newSize );
	void*					operator new( SIZE_T size );
	void					operator delete( void* ptr );

	FW_BlockSize			BlockSize( const void* block ) const;

	virtual unsigned long	BytesAllocated() const;
	virtual unsigned long	BytesFree() const = 0;
	virtual unsigned long	HeapSize() const = 0;
	virtual unsigned long	NumberAllocatedBlocks() const;
	void					Reset();

	virtual MemoryHeap*		GetNextHeap() const;

	virtual Boolean			GetZapOnAllocate() const;
	virtual Boolean			GetZapOnFree() const;
	virtual void			SetZapOnAllocate( Boolean = false );
	virtual void			SetZapOnFree( Boolean = false );


	virtual~				MemoryHeap();

	enum { kBlockTypeId = 0 };

#ifdef DEBUG
	static const char*		kDefaultDescription;

	virtual const char*		GetDescription() const;
	virtual void			SetDescription( const char* description = kDefaultDescription );

	virtual void			InstallHook( MemoryHook* MemoryHook );
	virtual void			RemoveHook( MemoryHook* MemoryHook );

	virtual Boolean			GetAutoValidation() const;
	virtual void			SetAutoValidation( Boolean = false );
	Boolean					IsValidBlock( void* blk ) const;
	virtual Boolean			IsMyBlock( void* blk ) const = 0;

	virtual void			Check() const = 0;
	virtual void			Print( char* msg = "" ) const = 0;

#endif

	static MemoryHeap*		fHeapList;
	static MemoryHeap*		GetFirstHeap();

protected:
							MemoryHeap(	Boolean autoValidation = false,
				  						Boolean zapOnAllocate = true,
				   						Boolean zapOnFree = false );

	virtual void*			AllocateRawMemory( FW_BlockSize size );
	virtual void*			DoAllocate( FW_BlockSize size, FW_BlockSize& allocatedSize ) = 0;
	virtual FW_BlockSize	DoBlockSize( const void* block ) const = 0;
	virtual void			DoFree(void*) = 0;
	virtual void			DoReset() = 0;
	virtual void*			DoReallocate( void* block, FW_BlockSize newSize, FW_BlockSize& allocatedSize );
	virtual void			FreeRawMemory(void* ptr);

#ifdef DEBUG
	virtual Boolean			DoIsValidBlock( void* blk ) const = 0;
	virtual void			CompilerCheck();
#endif

private:
	MemoryHeap*				fNextHeap;
	Boolean					fZapOnAllocate;
	Boolean					fZapOnFree;
	Boolean					fAutoValidation;
	unsigned long			fBytesAllocated;
	unsigned long			fNumberAllocatedBlocks;

#ifdef DEBUG
	FW_BlockSize			CallAboutToAllocateHooks( FW_BlockSize size );
	void*					CallDidAllocateHooks( void* blk, FW_BlockSize size );
	void*					CallAboutToFW_BlockSizeHooks( void* blk );
	void*					CallAboutToFreeHooks( void* blk );
	void					CallAboutToReallocHooks( void*& blk, FW_BlockSize& size );
	void*					CallDidReallocHooks( void* blk, FW_BlockSize size );
	void					CallAboutToResetHooks();
	void					CallCommentHooks( const char* comment );
	void					ValidateAndReport( void* blk ) const;

	enum { kDescriptionLength = 32 };
	char					fDescription[kDescriptionLength];
	MemoryHookList			fMemoryHookList;
#endif
	
	MemoryHeap( const MemoryHeap& blk );
	MemoryHeap& operator=( const MemoryHeap& blk );
		// This class shouldn't be copied.
};


#endif
