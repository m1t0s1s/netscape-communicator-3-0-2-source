#ifdef _WIN32

#include <windows.h>
#include <io.h>                     /* _open() and _close() */
#include <fcntl.h>                  /* O_RDONLY */
#include <stddef.h>                 /* offsetof() */
#include "prthread.h"
#include "prmon.h"
#include "prprf.h"
#include "mdint.h"
#include "prlog.h"

/************************************************************************/
/*
** Machine dependent initialization routines:
**    __md_InitOS
*/
/************************************************************************/

static PRMonitor freePageMonitor;

static int dummy_fds[3] = {-1, -1, -1};
void __md_InitOS(int when)
{
    if (when == _MD_INIT_AT_START) {
    }
    else if (when == _MD_INIT_READY) {
	PR_InitMonitor(&freePageMonitor, 0, "FreePageMonitor");
        /*
        ** Windows 95 does not reserve the file descriptors 0, 1, 2 for
        ** stdio...  So, reserve them now !!
        */
        for(;;) {
            int fd;

            fd = _open("NUL", O_RDONLY);
            if( fd == -1 ) {
                break;
            } else if( fd >= 3 ) {
                _close(fd);
                break;
            } else {
                dummy_fds[fd] = fd;
            }
        }
    }
}

/************************************************************************/
/*
** Machine dependent GC Heap management routines:
**    _MD_GrowGCHeap
*/
/************************************************************************/

struct FreePageChunk
{
    struct FreePageChunk *next;             /* Link to next chunk of free pages */
    char *addr;                                             /* First page in chunk of free pages */
    uint32 size;                                    /* Total size of chunk of free pages in bytes */
};

typedef struct FreePageChunk FreePageChunk;

static FreePageChunk *freePageChunkList = NULL; /* Root of sorted linked list of free page chunks */

static FreePageChunk *unusedFreePageChunkList = NULL; /* Root of unused FreePageChunk records */

#define FreePageChunkClumpSize 2

static char *gcHeapBoundary = 0;
static char *gcHeapBase = (char*)GC_VMBASE;

static void DeleteFreePageChunk(FreePageChunk *chunk)
{
    chunk->next = unusedFreePageChunkList;
    unusedFreePageChunkList = chunk;
}

static FreePageChunk *NewFreePageChunk(void)
{
    FreePageChunk *chunk = unusedFreePageChunkList;
    if (chunk)
	unusedFreePageChunkList = chunk->next;
    else
    {
	/* Allocate a clump of chunk records at once, and link all except the first onto the
	 * list of unused FreePageChunks */
	chunk = (FreePageChunk *)malloc(FreePageChunkClumpSize * sizeof(FreePageChunk));
	if (chunk)
	{
	    FreePageChunk *chunk2 = chunk + (FreePageChunkClumpSize-1);
	    while (chunk2 != chunk)
		DeleteFreePageChunk(chunk2--);
	}
    }
    return chunk;
}

/* Search the FreePageChunk list looking for the first chunk of consecutive free pages that
 * is at least size bytes long.  If there is one, remove these pages from the free page list
 * and return their address; if not, return nil. */
static char *AllocSegmentFromFreeList(uint32 size)
{
    FreePageChunk **p = &freePageChunkList;
    FreePageChunk *chunk;
    while ((chunk = *p))
    {
	if (chunk->size >= size)
	{
	    char *addr = chunk->addr;
	    if (chunk->size == size)
	    {
		*p = chunk->next;
		DeleteFreePageChunk(chunk);
	    }
	    else
	    {
		chunk->addr += size;
		chunk->size -= size;
	    }
	    return addr;
	}
	p = &chunk->next;
    }
    return 0;
}

/* Add the segment to the FreePageChunk list, coalescing it with any chunks already in the list
 * when possible. */
static void AddSegmentToFreeList(char *addr, uint32 size)
{
    FreePageChunk **p = &freePageChunkList;
    FreePageChunk *chunk;
    FreePageChunk *newChunk;

    while ((chunk = *p))
    {
	if (chunk->addr + chunk->size == addr)
	{
	    /* Coalesce with the previous chunk. */
	    FreePageChunk *next = chunk->next;

	    chunk->size += size;
	    if (next && next->addr == addr + size)
	    {
		/* We can coalesce with both the previous and the next chunk. */
		chunk->size += next->size;
		chunk->next = next->next;
		DeleteFreePageChunk(next);
	    }
	    return;
	}
	if (chunk->addr == addr + size)
	{
	    /* Coalesce with the next chunk. */
	    chunk->addr -= size;
	    chunk->size += size;
	    return;
	}
	if (chunk->addr > addr)
	{
	    PR_ASSERT(chunk->addr > addr + size);
	    break;
	}
	PR_ASSERT(chunk->addr + chunk->size < addr);
	p = &chunk->next;
    }
    newChunk = NewFreePageChunk();
    /* In the unlikely event that this malloc fails, we drop the free chunk on the floor.
       The only consequence is that the memory mapping table becomes slightly larger. */
    if (newChunk)
    {
	newChunk->next = chunk;
	newChunk->addr = addr;
	newChunk->size = size;
	*p = newChunk;
    }
}


int32 totalVirtual = 0;
#ifdef DEBUG

static void VerifyChunkList(int32 sizeDelta)
{
    static uint32 expectedBytesUsed = 0;
    uint32 calculatedBytesUsed;
    char *lastChunkEnd = 0;
    FreePageChunk *chunk;
    char str[256];
    expectedBytesUsed += sizeDelta;
    calculatedBytesUsed = gcHeapBoundary - gcHeapBase;
    sprintf(str, "[Chunks: %p", gcHeapBase);
    OutputDebugString(str);
    for (chunk = freePageChunkList; chunk; chunk = chunk->next)
    {
	PR_ASSERT(chunk->addr > lastChunkEnd);
	calculatedBytesUsed -= chunk->size;
	lastChunkEnd = chunk->addr + chunk->size;
	sprintf(str, "..%p, %p", chunk->addr-1, chunk->addr + chunk->size);
	OutputDebugString(str);
    }
    sprintf(str, "..%p]\n", gcHeapBoundary);
    OutputDebugString(str);
    PR_ASSERT(lastChunkEnd < gcHeapBoundary);
    PR_ASSERT(calculatedBytesUsed == expectedBytesUsed);
}
#endif

void *_MD_GrowGCHeap(uint32 *sizep)
{
    static PRBool virtualSpaceAllocated = PR_FALSE;
    char *addr;
    uint32 size = *sizep;
    PR_ASSERT((size & pr_pageSize - 1) == 0 && size != 0);
    PR_EnterMonitor(&freePageMonitor);
    /* Reserve a block of memory for the GC */
    if (!virtualSpaceAllocated)
    {
#ifdef DEBUG
        addr = (char *)VirtualAlloc(gcHeapBase, GC_VMLIMIT, MEM_RESERVE, PAGE_READWRITE);
        if (addr == NULL)
	{
#endif
	    gcHeapBase = (char*)0;	/* let the vm place the heap */
	    addr = (char *)VirtualAlloc(gcHeapBase, GC_VMLIMIT, MEM_RESERVE, PAGE_READWRITE);
	    if (addr == NULL)
	    {
		PR_ExitMonitor(&freePageMonitor);
		return 0;
	    }
#ifdef DEBUG
	}
#endif
	gcHeapBoundary = gcHeapBase = addr;
	virtualSpaceAllocated = PR_TRUE;
    }
    addr = AllocSegmentFromFreeList(size);
    if (!addr && gcHeapBoundary + size <= gcHeapBase + GC_VMLIMIT)
    {
	addr = gcHeapBoundary;
	gcHeapBoundary += size;
    }
    if (addr)
    {
	/* Extend the mapping */
	char *newAddr = (char *)VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);

        totalVirtual += size;
#ifdef DEBUG
	VerifyChunkList(size);
#endif
	PR_ASSERT(((uint32)addr & pr_pageSize - 1) == 0);
	if (addr)
	{
	    PR_ASSERT(newAddr == addr);
	    PR_LOG(GC, out, ("GC: allocated %08x to %08x", newAddr, newAddr + size));
	}
	else
	    _MD_FreeGCSegment(addr, size);
    }
    PR_ExitMonitor(&freePageMonitor);
    return addr;
}

void _MD_FreeGCSegment(void *base, int32 size)
{
    BOOL freeResult;
    char *addr = (char *)base;
    PR_ASSERT(((uint32)base & pr_pageSize - 1) == 0 && (size & pr_pageSize - 1) == 0 && size != 0);
    freeResult = VirtualFree(base, size, MEM_DECOMMIT);

    totalVirtual -= size;

    PR_ASSERT(freeResult);
    PR_EnterMonitor(&freePageMonitor);
    if (addr + size == gcHeapBoundary)
    {
	FreePageChunk **p;
	FreePageChunk *chunk;
	/* We deallocated the last set of chunks.  Move the boundary lower. */
	gcHeapBoundary = addr;
	/* The last free chunk might now be adjacent to the boundary; if so, move the boundary
	 * before that chunk and delete that chunk altogether. */
	p = &freePageChunkList;
	while ((chunk = *p))
	{
	    if (!chunk->next && chunk->addr + chunk->size == gcHeapBoundary)
	    {
		*p = 0;
		gcHeapBoundary = chunk->addr;
		DeleteFreePageChunk(chunk);
	    }
	    else
		p = &chunk->next;
	}
    }
    else
	AddSegmentToFreeList(addr, size);
#ifdef DEBUG
    VerifyChunkList(-size);
#endif
    PR_ExitMonitor(&freePageMonitor);
}

#endif /* _WIN32 */
