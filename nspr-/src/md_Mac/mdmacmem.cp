#include "MemoryHeap.h"
#include "ObjectHeap.h"
#include "BestFitHeap.h"
#include "mdmacmem.h"
#include "prmacos.h"
#include <stdlib.h>
#include "MacErrorHandling.h"
#include <string.h> 

#define DEBUG_MAC_MEMORY 			0

#if DEBUG_MAC_MEMORY
	#define FORCE_HEAP_SCRAMBLING	{														\
										Handle		tmpHandle;								\
																							\
										tmpHandle = NewHandle(0);							\
										if ((tmpHandle != NULL) && (MemError() == noErr))	\
											DisposeHandle(tmpHandle);						\
									}
#else
	#define FORCE_HEAP_SCRAMBLING
#endif


//##############################################################################
//##############################################################################
#pragma mark DECLARATIONS AND ENUMERATIONS

typedef struct MemoryCacheFlusherProcRec MemoryCacheFlusherProcRec;

struct MemoryCacheFlusherProcRec {
	MemoryCacheFlusherProc			flushProc;
	MemoryCacheFlusherProcRec		*next;
};

typedef struct PreAllocationProcRec PreAllocationProcRec;

struct PreAllocationProcRec {
	PreAllocationHookProc			preAllocProc;
	PreAllocationProcRec			*next;
};

MemoryCacheFlusherProcRec	*gFirstFlusher = NULL;
PreAllocationProcRec		*gFirstPreAllocator = NULL;

void CallPreAllocators(void);

#if DEBUG_MAC_MEMORY
void VerifyAllocatedBlock(void *block, char *message);
void VerifyEntireMallocHeap(void);
#endif

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark STDC-STYLE MEMORY

ObjectHeap 	*sObjectHeap = NULL;

//
//	NOTE:  	The following declarations are used to hint to the 
//			best-fit heap memory manager which block sizes are
//			used most commonly.  Right now, these are
//
//				sizeof( lo_FontStack )
//				sizeof( PA_Tag ) 
//				sizeof( LO_Element )
//

static const FW_BlockSize blockSizes[] = {
					4 	+ ChunkyBlock::kBusyOverhead, 
					12	+ ChunkyBlock::kBusyOverhead, 
					26 	+ ChunkyBlock::kBusyOverhead,
					0};

// 
//	NOTE:	The following declarations indicate to the best-bit
//			heap manager what default blocks to allocate.
//

#define K			1024
#define SYSTEM_RESERVE		32 * (K)
#define MEMORY_INCREMENT	24 * (K)
#define MEMORY_LIMIT		32 * (K) / 2
#define	INITIAL_MEMORY		24 * (K)
#define CHUNKS_PER_BLOCK	16

void MacintoshInitializeMemory(void)
{

	MaxApplZone();
	for (uint32 i = 1; i <= 30; i++) {
		MoreMasters();
	}

	if ( !sObjectHeap )
	{
		sObjectHeap = new ObjectHeap(
								blockSizes, 
								INITIAL_MEMORY,
								MEMORY_INCREMENT,
								CHUNKS_PER_BLOCK );
		sObjectHeap->SetLimitFromMax( MEMORY_LIMIT );
		sObjectHeap->IObjectHeap();
	}

}

#if DEBUG_MAC_MEMORY

enum {
	kAllocatedBlockHeaderTag	= 'nsbh',
	kAllocatedBlockTrailerTag	= 'nsbt',
	kFreeBlockHeaderTag			= 'nsfh',
	kFreeBlockTrailerTag		= 'nsft',
	kMemUsedFillPattern			= 0xEFEFEFEF,
	kMemFreeFillPattern			= 0xDFDFDFDF
};

typedef struct DebugBlockHeader DebugBlockHeader;

static void *					gFirstMemoryBlock = NULL;
static UInt32					gBlockCount = 0;
static UInt32					gBlockAllocations = 0; 
static uint32					gVerifyMallocHeapOnMemOp = false;
static uint32					gFindMallocCaller = false;

struct DebugBlockHeader {
	void 						*prevBlock;
	void						*nextBlock;
	void						*callerPC;
	uint32						blockNum;
	size_t						blockSize;
	uint32						blockTag;
};

typedef struct DebugBlockTrailer DebugBlockTrailer;

struct DebugBlockTrailer {
	uint32						blockTag;
};

void VerifyAllocatedBlock(void *block, char *message)
{
	DebugBlockHeader		*header;
	DebugBlockTrailer		*trailer;

	block = (char *)block - sizeof(DebugBlockHeader);

	header = (DebugBlockHeader *)block;
	trailer = (DebugBlockTrailer *)(((char *)block) + header->blockSize - sizeof(DebugBlockTrailer));
	
	if (header->blockTag != kAllocatedBlockHeaderTag) {
		if (header->blockTag == kFreeBlockHeaderTag)
			DebugStr("\pblock corrupt - possible double dispose");
		else
			DebugStr("\pmalloc block corrupt - possible corrupt block");
	}
	
	if (trailer->blockTag != kAllocatedBlockTrailerTag) {
		if (trailer->blockTag == kFreeBlockTrailerTag)
			DebugStr("\pmalloc block corrupt - possible double dispose");
		else
			DebugStr("\pmalloc block corrupt - possible corrupt block");
	}

}		

void VerifyEntireMallocHeap(void)
{
	uint32			totalFoundBlocks = 0;
	void			*currentBlock = gFirstMemoryBlock;
	
	while (currentBlock != NULL) {

		DebugBlockHeader		*header = (DebugBlockHeader *)currentBlock;	
		VerifyAllocatedBlock((char *)header + sizeof(DebugBlockHeader), "heap verify");
		totalFoundBlocks++;
		currentBlock = header->nextBlock;

	}
	
	if (totalFoundBlocks != gBlockCount)
		DebugStr("\pheap verify: not all blocks accounted for");
	
}


#endif

void* malloc( size_t size )
{	
#if DEBUG_MAC_MEMORY
	UInt32		toFindStackPtr;
#endif
	char 		*newBlock = NULL;	
	UInt8		tryAgain = true;

#if DEBUG_MAC_MEMORY
	size += sizeof(DebugBlockHeader) + sizeof(DebugBlockTrailer);
#endif

	//	For the bootstrapping case, make sure that we call NewPtr
	//	instead of sObjectHeap->Allocate when sObjectHeap is NULL
	
	if (sObjectHeap == NULL)
		return (void *)NewPtr(size);

	CallPreAllocators();
	
#if DEBUG_MAC_MEMORY

	if (gVerifyMallocHeapOnMemOp)
		VerifyEntireMallocHeap();

	newBlock = (char *)NewPtr(size);

	if (newBlock != NULL) {
	
		DebugBlockHeader		*header;
		DebugBlockTrailer		*trailer;
		
		header = (DebugBlockHeader *)newBlock;
		trailer = (DebugBlockTrailer *)(((char *)newBlock) + size - sizeof(DebugBlockTrailer));

		if (gFindMallocCaller) {
			header->callerPC = *((void **)((char *)&toFindStackPtr + 104 - 60));  	// 	TOTAL HACK TO GET LR!!!
		}
		
		else {
			header->callerPC = NULL;	
		}
		
		header->blockTag = kAllocatedBlockHeaderTag;
		header->blockSize = size;
		trailer->blockTag = kAllocatedBlockTrailerTag;
		
		header->blockNum = gBlockAllocations;
		
		gBlockAllocations++;
		gBlockCount++;

		//	Insert the new memory block into our linked list of blocks.
		
		header->nextBlock = gFirstMemoryBlock;
		header->prevBlock = NULL;
		
		if (gFirstMemoryBlock != NULL)
			((DebugBlockHeader *)gFirstMemoryBlock)->prevBlock = header;

		gFirstMemoryBlock = header;
		
		newBlock += sizeof(DebugBlockHeader);

		memset(newBlock, kMemUsedFillPattern, size - sizeof(DebugBlockHeader) - sizeof(DebugBlockTrailer));

	}

#else

	while (tryAgain && (newBlock == NULL)) {
		newBlock = (char *)sObjectHeap->Allocate(size);
		if (newBlock == NULL)
			tryAgain = CallCacheFlushers(size);
	}

#endif

	FORCE_HEAP_SCRAMBLING

	return newBlock;

}

void free(void* block)
{
	if (block == NULL)
		return;

#if DEBUG_MAC_MEMORY

	if (gVerifyMallocHeapOnMemOp)
		VerifyEntireMallocHeap();

	if (block != NULL) {

		DebugBlockHeader		*header;
		DebugBlockTrailer		*trailer;

		VerifyAllocatedBlock(block, "free");

		block = (char *)block - sizeof(DebugBlockHeader);

		header = (DebugBlockHeader *)block;
		trailer = (DebugBlockTrailer *)(((char *)block) + header->blockSize - sizeof(DebugBlockTrailer));

		memset((char *)header + sizeof(DebugBlockHeader), kMemFreeFillPattern, 
			header->blockSize - sizeof(DebugBlockHeader) - sizeof(DebugBlockTrailer));

		header->blockTag = kFreeBlockHeaderTag;
		trailer->blockTag = kFreeBlockTrailerTag;

		//	Take this block out of the list of used blocks.
		
		if (header->nextBlock != NULL)
			((DebugBlockHeader *)header->nextBlock)->prevBlock = header->prevBlock;

		if (header->prevBlock != NULL)
			((DebugBlockHeader *)header->prevBlock)->nextBlock = header->nextBlock;
		else
			gFirstMemoryBlock = header->nextBlock;

		gBlockCount--;

		DisposePtr((Ptr)block);

	}
	
#else	
		
	sObjectHeap->Free(block);

#endif

	FORCE_HEAP_SCRAMBLING

}

void* realloc( void* block, size_t newSize )
{
	void 		*newBlock = NULL;	
	UInt8		tryAgain = true;
	
	CallPreAllocators();
	
#if DEBUG_MAC_MEMORY

	if (gVerifyMallocHeapOnMemOp)
		VerifyEntireMallocHeap();

	VerifyAllocatedBlock(block, "realloc");

	newBlock = malloc(newSize);
	
	if (newBlock != NULL) {	
		BlockMoveData(block, newBlock, newSize);
		free(block);
	}

#else

	while (tryAgain && (newBlock == NULL)) {
		newBlock = sObjectHeap->Reallocate(block, newSize);
		if (newBlock == NULL)
			tryAgain = CallCacheFlushers(newSize);
	}


#endif

	FORCE_HEAP_SCRAMBLING

	return newBlock;
}

void *calloc(size_t nele, size_t elesize)
{
	char 		*newBlock = NULL;
	UInt8		tryAgain = true;
	size_t		totalSize = nele * elesize;

	CallPreAllocators();

#if DEBUG_MAC_MEMORY

	if (gVerifyMallocHeapOnMemOp)
		VerifyEntireMallocHeap();

	newBlock = (char *)malloc(totalSize);

#else

	while (tryAgain && (newBlock == NULL)) {
		newBlock = (char *)sObjectHeap->Allocate(totalSize);
		if (newBlock == NULL)
			tryAgain = CallCacheFlushers(totalSize);
	}

#endif
		
	if (newBlock != NULL)
		memset(newBlock, 0, totalSize);

	FORCE_HEAP_SCRAMBLING

	return newBlock;
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark INSTALLING MEMORY MANAGER HOOKS

void InstallPreAllocationHook(PreAllocationHookProc newHook)
{
	PreAllocationProcRec			*preAllocatorRec;

	preAllocatorRec = (PreAllocationProcRec *)(sObjectHeap->Allocate(sizeof(PreAllocationProcRec)));

	if (preAllocatorRec != NULL) {
	
		preAllocatorRec->next = gFirstPreAllocator;
		preAllocatorRec->preAllocProc = newHook;
		gFirstPreAllocator = preAllocatorRec;		
	
	}

}

void InstallMemoryCacheFlusher(MemoryCacheFlusherProc newFlusher)
{	
	MemoryCacheFlusherProcRec		*cacheFlusherRec;
	
	cacheFlusherRec =  (MemoryCacheFlusherProcRec *)(sObjectHeap->Allocate(sizeof(MemoryCacheFlusherProcRec)));

	if (cacheFlusherRec != NULL) {
	
		cacheFlusherRec->next = gFirstFlusher;
		cacheFlusherRec->flushProc = newFlusher;
		gFirstFlusher = cacheFlusherRec;		

	}
	
}

void CallPreAllocators(void)
{
	PreAllocationProcRec		*currentPreAllocator = gFirstPreAllocator;

	while (currentPreAllocator != NULL) {
		(*(currentPreAllocator->preAllocProc))();
		currentPreAllocator = currentPreAllocator->next;
	}
	
}

UInt8 CallCacheFlushers(size_t blockSize)
{
	MemoryCacheFlusherProcRec		*currentCacheFlusher = gFirstFlusher;
	UInt8							result = false;

	while (currentCacheFlusher != NULL) {
		result = (*(currentCacheFlusher->flushProc))(blockSize);
		if (result)
			break;
		currentCacheFlusher = currentCacheFlusher->next;
	}
	
	return result;
	
}
