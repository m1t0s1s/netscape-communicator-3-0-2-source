#include "fastmem.h"
#include "prmacos.h"
#include "mdmacmem.h"
#include "prgc.h"
#include "stdlib.h"
#include <Memory.h>

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
void MacintoshInitializeMemory(void);

//##############################################################################
//##############################################################################
#pragma mark INITIALIZATION

void MacintoshInitializeMemory(void)
{
	UInt32 i;
	MaxApplZone();
	for (i = 1; i <= 30; i++)
		MoreMasters();
#if DEBUG_MAC_MEMORY
	InstallMemoryManagerPatches();
#endif
}

//##############################################################################
//##############################################################################
#pragma mark FIXED-SIZE ALLOCATION DECLARATIONS

DeclareFixedSizeFastAllocator(4, 8);
DeclareFixedSizeFastAllocator(8, 8);
DeclareFixedSizeFastAllocator(12, 8);
//DeclareFixedSizeCompactAllocator(4);
//DeclareFixedSizeCompactAllocator(8);
//DeclareFixedSizeCompactAllocator(12);
DeclareFixedSizeCompactAllocator(16);
DeclareFixedSizeCompactAllocator(24);
DeclareFixedSizeCompactAllocator(32);
DeclareSmallHeapAllocator();

AllocMemoryBlockDescriptor gFastMemSmallSizeAllocators[] = {
	{	&StandardAlloc,				0 	},
	{	&FixedSizeFastAlloc,		(void *)&gFixedSizeFast4Root 	},
	{	&FixedSizeFastAlloc,		(void *)&gFixedSizeFast8Root 	},
	{	&FixedSizeFastAlloc,		(void *)&gFixedSizeFast12Root 	},
	{	&FixedSizeCompactAlloc,		(void *)&gFixedSizeCompact16Root 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&FixedSizeCompactAlloc,		(void *)&gFixedSizeCompact24Root 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&FixedSizeCompactAlloc,		(void *)&gFixedSizeCompact32Root 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	},
	{	&SmallHeapAlloc,			(void *)&gSmallHeapRoot 	}
};


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark INSTALLING MEMORY MANAGER HOOKS

void InstallPreAllocationHook(PreAllocationHookProc newHook)
{
	PreAllocationProcRec			*preAllocatorRec;

	preAllocatorRec = (PreAllocationProcRec *)(NewPtr(sizeof(PreAllocationProcRec)));

	if (preAllocatorRec != NULL) {
	
		preAllocatorRec->next = gFirstPreAllocator;
		preAllocatorRec->preAllocProc = newHook;
		gFirstPreAllocator = preAllocatorRec;		
	
	}

}

void InstallMemoryCacheFlusher(MemoryCacheFlusherProc newFlusher)
{	
	MemoryCacheFlusherProcRec		*previousFlusherRec = NULL;
	MemoryCacheFlusherProcRec		*cacheFlusherRec = gFirstFlusher;
	
	while (cacheFlusherRec != NULL) {
		previousFlusherRec = cacheFlusherRec;
		cacheFlusherRec = cacheFlusherRec->next;
	}

	cacheFlusherRec =  (MemoryCacheFlusherProcRec *)NewPtrClear(sizeof(MemoryCacheFlusherProcRec));
	
	if (cacheFlusherRec == NULL)
		return;

	cacheFlusherRec->flushProc = newFlusher;

	if (previousFlusherRec != NULL) {
		previousFlusherRec->next = cacheFlusherRec;
	}
	
	else {
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
		result |= (*(currentCacheFlusher->flushProc))(blockSize);
		currentCacheFlusher = currentCacheFlusher->next;
	}
	
	//	The garbage collection flusher should always be last
	
	result |= GarbageCollectorCacheFlusher(blockSize);
	
	return result;
	
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark RAW MEMORY ALLOCATION

//	kArchitechtureStartupOverhead is negative for 68K because
//	when the 68K version first starts up, not all of it's code
//	resources are loaded, even though there is space in the heap
//	for them.  This makes the initialize overhead
//	kArchitechtureStartupOverhead - (Size of missing code = 1.74MB) = -1MB

#define 	kMallocFractionOfFigmentHeap 	0.6
#if GENERATINGPOWERPC
#define		kArchitechtureStartupOverhead	740 * 1024
#else
#define		kArchitechtureStartupOverhead	(-1024 * 1024)
#endif

enum {
	kMallocCleanupEmergencyFundSize			= 8 * 1024,
	kMallocCleanupEmergencyFundIncrement	= 128
};

Handle		gMallocCleanupEmergencyFund = NULL;
Ptr			gMallocSubAllocationHeap = NULL;

long pascal MallocGrowZoneProc(Size cbNeeded)
{
	return 0;
}

#if GENERATINGCFM
RoutineDescriptor 	gMallocGrowZoneProcRD = BUILD_ROUTINE_DESCRIPTOR(uppGrowZoneProcInfo, &MallocGrowZoneProc);
#else
#define gMallocGrowZoneProcRD MallocGrowZoneProc
#endif

Ptr AllocateRawMemory(Size blockSize)
{
	THz			savedZone;
	Ptr			resultPtr;

	if (gMallocCleanupEmergencyFund == NULL)
		gMallocCleanupEmergencyFund = NewHandle(kMallocCleanupEmergencyFundSize);

	savedZone = GetZone();

	//	If we don't have a sub-heap yet, create one.
	
	if (gMallocSubAllocationHeap == NULL) {
		Size		newHeapSize;
		float		tmp;
		newHeapSize = FreeMem();
		newHeapSize += kArchitechtureStartupOverhead;
		tmp = (float)newHeapSize * kMallocFractionOfFigmentHeap;
		newHeapSize = tmp;
		gMallocSubAllocationHeap = NewPtr(newHeapSize);
		InitZone((GrowZoneUPP)&gMallocGrowZoneProcRD, 16, gMallocSubAllocationHeap + newHeapSize, gMallocSubAllocationHeap);
	}

	SetZone((THz)gMallocSubAllocationHeap);

	resultPtr = NewPtr(blockSize);
	
	if (resultPtr == NULL) {

		if (InMemory_ReserveInMacHeap())
			return NULL;

		SetZone(savedZone);
		Memory_ReserveInMallocHeap(blockSize);
		SetZone((THz)gMallocSubAllocationHeap);
		resultPtr = NewPtr(blockSize);
	
	}
	
	SetZone(savedZone);

#if DEBUG
	if (resultPtr == NULL)
		DebugStr("\pAllocateRawMemory FAILED!");
#endif

	return resultPtr;
	
}

void FreeRawMemory(Ptr reclaimedRawBlock)
{
	DisposePtr(reclaimedRawBlock);
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark MEMORY UTILS

Boolean gInMemory_ReserveInMacHeap = false;

Boolean InMemory_ReserveInMacHeap()
{
	return gInMemory_ReserveInMacHeap;
}

Boolean ReclaimMemory(size_t amountNeeded)
{
	THz				savedZone;
	Boolean			result;
	Size			reclaimSize;
	
	savedZone = GetZone();
	SetZone((THz)gMallocSubAllocationHeap);

	if (gMallocCleanupEmergencyFund != NULL)
		DisposeHandle(gMallocCleanupEmergencyFund);
		
	SetZone(savedZone);
	result = CallCacheFlushers(amountNeeded);

	SetZone((THz)gMallocSubAllocationHeap);
	
	reclaimSize = kMallocCleanupEmergencyFundSize;
	gMallocCleanupEmergencyFund = NULL;
	
	while ((gMallocCleanupEmergencyFund == NULL) && (reclaimSize != 0)) {
		gMallocCleanupEmergencyFund = NewHandle(reclaimSize);
		reclaimSize -= kMallocCleanupEmergencyFundIncrement;
	}
	
	SetZone(savedZone);
	
	return result;

}

Boolean Memory_ReserveInMacHeap(size_t spaceNeeded)
{
	Boolean		result = true;
	
	gInMemory_ReserveInMacHeap = true;
	
	if (MaxBlock() < spaceNeeded)
		result = ReclaimMemory(spaceNeeded);
	
	gInMemory_ReserveInMacHeap = false;

	return result;
	
}

Boolean Memory_ReserveInMallocHeap(size_t spaceNeeded)
{
	THz			savedZone;
	Boolean		result = true;
	Size		freeMem;
	
	gInMemory_ReserveInMacHeap = true;
	
	savedZone = GetZone();
	SetZone((THz)gMallocSubAllocationHeap);
	freeMem = MaxBlock();
	SetZone(savedZone);
	
	if (freeMem < spaceNeeded)
		result = ReclaimMemory(spaceNeeded);

	gInMemory_ReserveInMacHeap = false;

	return result;
	
}

size_t Memory_FreeMemoryRemaining()
{
	size_t		mainHeap;
	size_t		mallocHeap;
	THz			savedZone;
	
	mainHeap = FreeMem();
	
	savedZone = GetZone();
	SetZone((THz)gMallocSubAllocationHeap);
	mallocHeap = FreeMem();
	SetZone(savedZone);
	
	return (mainHeap < mallocHeap) ? mainHeap : mallocHeap; 

}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark FLUSHING THE GARBAGE COLLECTOR

extern int _pr_intsOff;

unsigned char GarbageCollectorCacheFlusher(size_t size)
{
	UInt32		oldPriority;

	//	If java wasn't completely initialized, then bail 
	//	harmlessly.
	
	if (PR_GetGCInfo()->lock == NULL)
		return false;

#if DEBUG
	if (_pr_intsOff == 1)
		DebugStr("\pGarbageCollectorCacheFlusher at interrupt time!");
#endif

	//	The synchronization here is very tricky.  We really
	//	don't want any other threads to run while we are 
	//	cleaning up the gc heap... they could call malloc,
	//	and then we would be in trouble in a big way.  So,
	//	we jack up our priority and that of the finalizer
	//	so that we won't yield to other threads.
	//	dkc 5/17/96

	oldPriority = PR_GetThreadPriority(PR_CurrentThread());
	_pr_intsOff = 1;
	_PR_SetThreadPriority(PR_CurrentThread(), 30);
	_pr_intsOff = 0;

	//	Garbage collect twice.  This will finalize any
	//	dangling AWT resources (images, components), and
	//	then free up their GC space, too.
	//	dkc 2/15/96
	
	PR_GC();
	
	//	By setting the finalizer priority to 31, then we 
	//	ensure it will run before us.  When it finishes
	//	its list of finalizations, it returns to us
	//	for the second garbage collection.
	
	PR_Yield();

	PR_GC();
	
	//	Restore our old priorities.
	
	_pr_intsOff = 1;
	_PR_SetThreadPriority(PR_CurrentThread(), oldPriority);
	_pr_intsOff = 0;

	//	And release any other resources that might be around.
	
	_PR_FreeZombies();

	return false;

}



