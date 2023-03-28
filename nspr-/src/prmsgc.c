#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include "prmacros.h"
#include "prclist.h"
#include "prgc.h"
#include "prthread.h"
#include "prglobal.h"
#include "prlog.h"
#include "prtime.h"
#include "prprf.h"
#include "gcint.h"
#include "mdint.h"

#ifdef SW_THREADS
#include "swkern.h"
#else
#include "hwkern.h"
#endif

/*
** Simple mark/sweep garbage collector
*/

PR_LOG_DEFINE(GC);

#ifdef DEBUG
#define GCMETER
#endif
#ifdef DEBUG_jwz
# undef GCMETER
#endif /* 1 */

#ifdef GCMETER
#define METER(x) x
#else
#define METER(x)
#endif


/*
** Make this constant bigger to reduce the amount of recursion during
** garbage collection.
*/
#define MAX_SCAN_Q	100L

#if defined(XP_PC) && !defined(_WIN32)
#define MAX_SEGS            400L
#define MAX_SEGMENT_SIZE    (65536L - 4096L)
#define SEGMENT_SIZE        (65536L - 4096L)
#define MAX_ALLOC_SIZE      (65536L - 4096L)

#else
#define MAX_SEGS            400

#define MAX_SEGMENT_SIZE    (2 * 256 * 1024)
#define SEGMENT_SIZE        (1 * 256 * 1024)
#endif  

static unsigned int segmentSize = SEGMENT_SIZE;

#ifdef GCMETER
uint32 _pr_gcMeter;

#define _GC_METER_STATS         0x01L
#define _GC_METER_GROWTH        0x02L
#define _GC_METER_FREE_LIST     0x04L
#endif

#ifdef DEBUG
static void nop(void) { }
#endif

/************************************************************************/

#define NUM_BINS        64

#ifdef OSF1
#define MIN_ALLOC       32
#define MIN_ALLOC_LOG2  5
#else
#define MIN_ALLOC       16
#define MIN_ALLOC_LOG2  4
#endif

#define BIG_ALLOC       16384L

typedef struct GCFreeChunk {
    struct GCFreeChunk *next;
    struct GCSeg *segment;
    size_t chunkSize;
} GCFreeChunk;

typedef struct GCSegInfo {
    struct GCSegInfo *next;

    char GCPTR *base;
    char GCPTR *limit;
    prword_t GCPTR *hbits;
} GCSegInfo;
    
typedef struct GCSeg {
    char GCPTR *base;
    char GCPTR *limit;
    prword_t GCPTR *hbits;
    GCSegInfo *info;
} GCSeg;

#ifdef GCMETER
typedef struct GCMeter {
    int32 allocBytes;
    int32 zeroBytes;
    int32 wastedBytes;
    int32 numFreeChunks;
} GCMeter;
static GCMeter meter;
#endif

/*
** There is one of these for each segment of GC'able memory.
*/
static GCSeg segs[MAX_SEGS];
static GCSegInfo *freeSegs;
static int nsegs;
static GCFreeChunk *bins[NUM_BINS];

/*
** Scan Q used to avoid deep recursion when scanning live objects for
** heap pointers
*/
typedef struct GCScanQStr {
    prword_t GCPTR *q[MAX_SCAN_Q];
    int queued;
} GCScanQ;

static GCScanQ *pScanQ;

#ifdef GCMETER
int32 _pr_maxScanDepth;
int32 _pr_scanDepth;
#endif

/*
** Keeps track of the number of bytes allocated via the BigAlloc() 
** allocator.  When the number of bytes allocated, exceeds the 
** BIG_ALLOC_GC_SIZE, then a GC will occur before the next allocation
** is done...
*/
#define BIG_ALLOC_GC_SIZE       (4*SEGMENT_SIZE)
static prword_t bigAllocBytes = 0;


/*
** There is one GC header word in front of each GC allocated object.  We
** use it to contain information about the object (what TYPEIX to use for
** scanning it, how big it is, it's mark status, and if it's a root).
*/
#define TYPEIX_BITS	8L
#define WORDS_BITS	22L
#define MAX_CBS		(1L << GC_TYPEIX_BITS)
#define MAX_WORDS	(1L << GC_WORDS_BITS)
#define TYPEIX_SHIFT	24L
#define MAX_TYPEIX	((1L << TYPEIX_BITS) - 1L)
#define TYPEIX_MASK	PR_BITMASK(TYPEIX_BITS)
#define WORDS_SHIFT	2L
#define WORDS_MASK	PR_BITMASK(WORDS_BITS)
#define MARK_BIT	1L
#define FINAL_BIT	2L

#define MAKE_HEADER(_cbix,_words)			  \
    ((prword_t) (((unsigned long)(_cbix) << TYPEIX_SHIFT) \
		 | ((unsigned long)(_words) << WORDS_SHIFT)))

#define GET_TYPEIX(_h) \
    (((uprword_t)(_h) >> TYPEIX_SHIFT) & 0xff)

#define MARK(_sp,_p) \
    (((prword_t GCPTR *)(_p))[0] |= MARK_BIT)
#define IS_MARKED(_sp,_p) \
    (((prword_t GCPTR *)(_p))[0] & MARK_BIT)

#if defined(XP_PC) && !defined(_WIN32)
#define OBJ_BYTES(_h) \
    ( ((size_t) ((_h) & 0x00fffffcL) << (BYTES_PER_WORD_LOG2-2L)) )

#else
#define OBJ_BYTES(_h) \
    (((uprword_t) (_h) & 0x00fffffcL) << (BYTES_PER_WORD_LOG2-2L))

#endif

/************************************************************************/

/*
** Mark the start of an object in a segment. Note that we mark the header
** word (which we always have), not the data word (which we may not have
** for empty objects).
** XXX tune: put subtract of _sp->base into _sp->hbits pointer?
*/
#if !defined(XP_PC) || defined(_WIN32)
#define SET_HBIT(_sp,_ph) \
    SET_BIT((_sp)->hbits, (((prword_t*)(_ph)) - ((prword_t*) (_sp)->base)))

#define CLEAR_HBIT(_sp,_ph) \
    CLEAR_BIT((_sp)->hbits, (((prword_t*)(_ph)) - ((prword_t*) (_sp)->base)))

#define IS_HBIT(_sp,_ph) \
    TEST_BIT((_sp)->hbits, (((prword_t*)(_ph)) - ((prword_t*) (_sp)->base)))
#else

#define SET_HBIT(_sp,_ph) set_hbit(_sp,_ph)

#define CLEAR_HBIT(_sp,_ph) clear_hbit(_sp,_ph)

#define IS_HBIT(_sp,_ph) is_hbit(_sp,_ph)

static void
set_hbit(GCSeg *sp, prword_t GCPTR *p)
{
	unsigned long	distance;
	unsigned long	index;
	unsigned long	mask;

#if 1
        PR_ASSERT( SELECTOROF(p) == SELECTOROF(sp->base) );
        PR_ASSERT( OFFSETOF(p)   >= OFFSETOF(sp->base) );

        distance = (OFFSETOF(p) - OFFSETOF(sp->base)) >> 2;
	index    = distance >> BITS_PER_WORD_LOG2;
	mask	 = 1L << (distance&(BITS_PER_WORD-1));
#else
	distance = ((unsigned long)(((unsigned char *) p) - sp->base)) >> 2L;
	index    = distance >> BITS_PER_WORD_LOG2;
	mask	 = 1L << (distance&(BITS_PER_WORD-1));
#endif
	sp->hbits[index] |= mask;
}

static void
clear_hbit(GCSeg *sp, prword_t GCPTR *p)
{
	unsigned long	distance;
	unsigned long	index;
	unsigned long	mask;

#if 1
        PR_ASSERT( SELECTOROF(p) == SELECTOROF(sp->base) );
        PR_ASSERT( OFFSETOF(p)   >= OFFSETOF(sp->base) );

        distance = (OFFSETOF(p) - OFFSETOF(sp->base)) >> 2;
	index    = distance >> BITS_PER_WORD_LOG2;
	mask	 = 1L << (distance&(BITS_PER_WORD-1));
#else
	distance = ((unsigned long)(((unsigned char *) p) - sp->base)) >> 2L;
	index    = distance >> BITS_PER_WORD_LOG2;
	mask	 = 1L << (distance&(BITS_PER_WORD-1));
#endif
	sp->hbits[index] &= ~mask;
}

static int
is_hbit(GCSeg *sp, prword_t GCPTR *p)
{
	unsigned long	distance;
	unsigned long	index;
	unsigned long	mask;

#if 1
        PR_ASSERT( SELECTOROF(p) == SELECTOROF(sp->base) );
        PR_ASSERT( OFFSETOF(p)   >= OFFSETOF(sp->base) );

        distance = (OFFSETOF(p) - OFFSETOF(sp->base)) >> 2;
	index    = distance >> BITS_PER_WORD_LOG2;
	mask	 = 1L << (distance&(BITS_PER_WORD-1));
#else
	distance = ((unsigned long)(((unsigned char *) p) - sp->base)) >> 2L;
	index    = distance >> BITS_PER_WORD_LOG2;
	mask	 = 1L << (distance&(BITS_PER_WORD-1));
#endif
	return ((sp->hbits[index] & mask) != 0);
}


#endif

/*
** Given a pointer into this segment, back it up until we are at the
** start of the object the pointer points into. Each heap segment has a
** bitmap that has one bit for each word of the objects it contains.  The
** bit's are set for the firstword of an object, and clear for it's other
** words.
*/
#ifdef UNUSED
static prword_t *FindObject(GCSeg *sp, prword_t *p)
{
    prword_t *base;
    
    /* Align p to it's proper boundary before we start fiddling with it */
    p = (prword_t*) ((prword_t)p & ~(BYTES_PER_WORD-1L));

    base = (prword_t *) sp->base;
#if defined(XP_PC) && !defined(_WIN32)
    PR_ASSERT( SELECTOROF(p) == SELECTOROF(base));
#endif
    do {
	if (IS_HBIT(sp, p)) {
	    return (p);
	}
	p--;
    } while ( p >= base );

    /* Heap is corrupted! */
    GCTRACE(GC_TRACE, ("ERROR: The heap is corrupted!!! aborting now!"));
    abort();
}
#endif /* UNUSED */

/************************************************************************/
#ifndef XP_PC
#define OutputDebugString(msg)
#endif 

#if !defined(XP_PC) || defined(_WIN32)
#define IN_SEGMENT(_sp, _p)		     \
    ((((char GCPTR*)(_p)) >= (_sp)->base) && \
     (((char GCPTR*)(_p)) < (_sp)->limit))
#else
#define IN_SEGMENT(_sp, _p)		     \
    ((((prword_t)(_p)) >= ((prword_t)(_sp)->base)) && \
     (((prword_t)(_p)) < ((prword_t)(_sp)->limit)))
#endif

#if defined(XP_PC) && !defined(_WIN32)
static GCSeg *InHeap(void GCPTR *p)
{
    GCSeg *sp, *esp;
    static GCSeg *last = 0;

    if (last && IN_SEGMENT(last, p)) {
	return last;
    }

    sp = segs;
    esp = segs + nsegs;
    for (; sp < esp; sp++) {
	if (IN_SEGMENT(sp, p)) {
	    last = sp;
	    return sp;
	}
    }
    return 0;
}
#endif /* defined(XP_PC) && !defined(_WIN32) */

/*
** Grow the heap by allocating another segment. Fudge the requestedSize
** value to try to pre-account for the HBITS.
*/
static GCSeg* DoGrowHeap(size_t requestedSize, PRBool exactly)
{
    GCSeg *sp;
    GCSegInfo *segInfo;
    GCFreeChunk *cp;
    prword_t nhbytes, nhbits;
    char GCPTR *base;
    prword_t GCPTR *hbits;
    uint32 	 allocSize;
    
    int32 bin;

    if (nsegs == MAX_SEGS) {
	/* No room for more segments */
	return 0;
    }

    segInfo = (GCSegInfo*) malloc(sizeof(GCSegInfo));
#ifdef DEBUG
    {
	char str[256];
	sprintf(str, "[1] Allocated %d bytes at %p\n", sizeof(GCSegInfo), segInfo);
	OutputDebugString(str);
    }
#endif
    if (!segInfo) {
	return 0;
    }

#if defined(XP_PC) && !defined(_WIN32)
    if (requestedSize > segmentSize) {
	free(segInfo);
	return 0;
    }
#endif

    /* Get more memory from the OS */
	allocSize = requestedSize;
	allocSize = (size_t)((allocSize + pr_pageSize - 1L) >> pr_pageShift);
	allocSize <<= pr_pageShift;
	base = _MD_GrowGCHeap(&allocSize);
#ifdef DEBUG
	{
	    char str[256];
	    sprintf(str, "[2] Allocated %d bytes at %p\n", allocSize, base);
	    OutputDebugString(str);
	}
#endif
    if (!base) {
	free(segInfo);
	return 0;
    }

    nhbits = (allocSize + BYTES_PER_WORD - 1L) >> BYTES_PER_WORD_LOG2;
    nhbytes = ((nhbits + BITS_PER_WORD - 1L) >> BITS_PER_WORD_LOG2)
	* sizeof(prword_t);

    /* Get bitmap memory from malloc heap */
#if defined(XP_PC) && !defined(_WIN32)
    PR_ASSERT( nhbytes < MAX_ALLOC_SIZE );
#endif
    hbits = (prword_t GCPTR*) calloc(1, (size_t)nhbytes);
    if (!hbits) {
	/* Loser! */
	free(segInfo);
	_MD_FreeGCSegment(base, allocSize);
	return 0;
    }

    /*
    ** Setup new segment.
    */
    sp = &segs[nsegs++];
    segInfo->base = sp->base = base;
    segInfo->limit = sp->limit = base + allocSize;
    segInfo->hbits = sp->hbits = hbits;
    sp->info = segInfo;

#ifdef GCMETER
    if (_pr_gcMeter & _GC_METER_GROWTH) {
        fprintf(stderr, "[GC: new segment base=%p size=%d]\n",
                sp->base, allocSize);
    }
#endif    

    _pr_gcData.allocMemory += allocSize;
    _pr_gcData.freeMemory  += allocSize;

    if (!exactly) {
        /* Put free memory into a freelist bin */
        bin = allocSize >> MIN_ALLOC_LOG2;
        if (bin >= NUM_BINS) {
            bin = NUM_BINS - 1;
        }
        cp = (GCFreeChunk GCPTR*) base;
        cp->segment = sp;
        cp->chunkSize = allocSize;
        cp->next = bins[bin];
        bins[bin] = cp;
    } else {
        /*
        ** When exactly allocating the entire segment is given over to a
        ** single object to prevent fragmentation
        */
    }

    if (!_pr_gcData.lowSeg) {
	_pr_gcData.lowSeg  = (prword_t*) sp->base;
	_pr_gcData.highSeg = (prword_t*) sp->limit;
    } else {
	if ((prword_t*)sp->base < _pr_gcData.lowSeg) {
	    _pr_gcData.lowSeg = (prword_t*) sp->base;
	}
	if ((prword_t*)sp->limit > _pr_gcData.highSeg) {
	    _pr_gcData.highSeg = (prword_t*) sp->limit;
	}
    }

    /* 
    ** Get rid of the GC pointer in case it shows up in some uninitialized
    ** local stack variable later (while scanning the C stack looking for
    ** roots).
    */ 
    memset(&base, 0, sizeof(base));  /* optimizers beware */
    PR_LOG(GC, warn, ("grow heap: total gc memory now %d",
                      _pr_gcData.allocMemory));
    return sp;
}

static GCSeg *GrowHeapExactly(size_t requestedSize)
{
    GCSeg *sp = DoGrowHeap(requestedSize, PR_TRUE);
    return sp;
}

static PRBool GrowHeap(size_t requestedSize)
{
    void *p = DoGrowHeap(requestedSize, PR_FALSE);
    return p != NULL;
}

/*
** Release a segment when it is entirely free.
*/
static void ShrinkGCHeap(GCSeg *sp)
{
#ifdef GCMETER
    if (_pr_gcMeter & _GC_METER_GROWTH) {
        fprintf(stderr, "[GC: free segment base=%p size=%d]\n",
                sp->base, sp->limit - sp->base);
    }
#endif    

    /*
     * Put segment onto free seginfo list (we can't call free right now
     * because we have the GC lock and all of the other threads are
     * suspended; if one of them has the malloc lock we would deadlock)
     */
    sp->info->next = freeSegs;
    freeSegs = sp->info;
    _pr_gcData.allocMemory -= sp->limit - sp->base;

    /* Squish out disappearing segment from segment table */
    --nsegs;
    if ((sp - segs) != nsegs) {
        *sp = segs[nsegs];
    } else {
        sp->base = 0;
        sp->limit = 0;
        sp->hbits = 0;
	sp->info = 0;
    }

    /* Recalculate the lowSeg and highSeg values */
    _pr_gcData.lowSeg  = (prword_t*) segs[0].base;
    _pr_gcData.highSeg = (prword_t*) segs[0].limit;
    for (sp = segs; sp < &segs[nsegs]; sp++) {
	if ((prword_t*)sp->base < _pr_gcData.lowSeg) {
	    _pr_gcData.lowSeg = (prword_t*) sp->base;
	}
	if ((prword_t*)sp->limit > _pr_gcData.highSeg) {
	    _pr_gcData.highSeg = (prword_t*) sp->limit;
	}
    }
}

static void FreeSegments(void)
{
    GCSegInfo *si;

    while (freeSegs != 0) {
	LOCK_GC();
	si = freeSegs;
	if (si) {
	    freeSegs = si->next;
	}
	UNLOCK_GC();

	if (!si) {
	    break;
	}
	_MD_FreeGCSegment(si->base, si->limit - si->base);
#ifdef DEBUG
	{
	    char str[256];
	    sprintf(str, "[3] Deallocated bytes at %p\n", si->base);
	    OutputDebugString(str);
	}
#endif
	free(si->hbits);
	free(si);
#ifdef DEBUG
	{
	    char str[256];
	    sprintf(str, "[4] Deallocated bytes at %p\n", si);
	    OutputDebugString(str);
	}
#endif
    }
}

/************************************************************************/

void ScanScanQ(GCScanQ *iscan)
{
    prword_t GCPTR *p;
    prword_t GCPTR **pp;
    prword_t GCPTR **epp;
    GCScanQ nextQ, *scan, *next, *temp;
    prword_t h;
    uprword_t tix;
    GCType *gct;

    if (!iscan->queued) return;

    GCTRACE(GC_MARK, ("begin scanQ @ 0x%x (%d)", iscan, iscan->queued));
    scan = iscan;
    next = &nextQ;
    while (scan->queued) {
	GCTRACE(GC_MARK, ("continue scanQ @ 0x%x (%d)", scan, scan->queued));
	/* Set pointer to current scanQ so that pr_liveObject can find it */
	pScanQ = next;
	next->queued = 0;

	/* Now scan the scan Q */
	pp = scan->q;
	epp = &scan->q[scan->queued];
	scan->queued = 0;
	while (pp < epp) {
	    p = *pp++;
	    h = p[0];
	    tix = GET_TYPEIX(h);
	    /*
	    ** If type code is invalid, or if we don't have a handler for
	    ** the type or if the handler doesn't need to scan it's
	    ** objects (for example, the type is used to alloc C
	    ** strings...).
	    */
#ifdef DEBUG
	    if (tix != FREE_MEMORY_TYPEIX) {
		PR_ASSERT(_pr_gcTypes[tix] != 0);
	    }
#endif
	    if (!(gct = _pr_gcTypes[tix]) || (gct->scan == 0)) {
		continue;
	    }

	    /* Scan object ... */
	    (*gct->scan)(p + 1);
	}

	/* Exchange pointers so that we scan next */
	temp = scan;
	scan = next;
	next = temp;
    }

    pScanQ = iscan;
    PR_ASSERT(nextQ.queued == 0);
    PR_ASSERT(iscan->queued == 0);
}

/*
** Called during root finding step to identify "root" pointers into the
** GC heap. First validate if it is a real heap pointer and then mark the
** object being pointed to and add it to the scan Q for eventual
** scanning.
*/
static void PR_CALLBACK ProcessRoot(void **base, int32 count)
{
    GCSeg *last;
    prword_t *p0, *p, h, tix, *low, *high, *segBase;
    GCType *gct;
#ifdef DEBUG
    void **base0 = base;
#endif

    low = _pr_gcData.lowSeg;
    high = _pr_gcData.highSeg;
    last = segs;
    while (--count >= 0) {
        p0 = (prword_t*) *base++;
        /*
        ** XXX:  
        ** Until Win16 maintains lowSeg and highSeg correctly,
        ** (ie. lowSeg=MIN(all segs) and highSeg = MAX(all segs))
        ** Allways scan through the segment list
        */
#if !defined(XP_PC) || defined(_WIN32)
        if (p0 < low) continue;                  /* below gc heap */
        if (p0 >= high) continue;                /* above gc heap */
#endif
        /* NOTE: inline expansion of InHeap */
        /* Find segment */
        if (!IN_SEGMENT(last,p0)) {
            GCSeg *sp = segs;
            GCSeg *esp = segs + nsegs;
            for (; sp < esp; sp++) {
                if (IN_SEGMENT(sp, p0)) {
                    last = sp;
                    goto find_object;
                }
            }
            continue;
        }

      find_object:
        /* NOTE: Inline expansion of FindObject */
        /* Align p to it's proper boundary before we start fiddling with it */
        p = (prword_t*) ((prword_t)p0 & ~(BYTES_PER_WORD-1L));
        segBase = (prword_t *) last->base;
        do {
            if (IS_HBIT(last, p)) {
                goto winner;
            }
            p--;
        } while (p >= segBase);

        /*
        ** We have a pointer into the heap, but it has no header
        ** bit. This means that somehow the very first object in the heap
        ** doesn't have a header. This is impossible so when debugging
        ** lets abort.
        */
#ifdef DEBUG
        PR_Abort();
#endif

      winner:
        h = p[0];
        if ((h & MARK_BIT) == 0) {
#ifdef DEBUG
            GCTRACE(GC_ROOTS,
		    ("root 0x%p (%d) base0=%p off=%d",
		     p, OBJ_BYTES(h), base0, (base-1) - base0));
#endif

            /* Mark the root we just found */
            p[0] = h | MARK_BIT;

            /*
            ** See if object we just found needs scanning. It must have a
            ** gctype and it must have a scan function to be placed on
            ** the scanQ.
            */
            tix = GET_TYPEIX(h);
	    if (!(gct = _pr_gcTypes[tix]) || (gct->scan == 0)) {
		continue;
	    }

            /*
            ** Put a pointer onto the scan Q. We use the scan Q to avoid
            ** deep recursion on the C call stack. Objects are added to
            ** the scan Q until the scan Q fills up. At that point we
            ** make a call to ScanScanQ which proceeds to scan each of
            ** the objects in the Q. This limits the recursion level by a
            ** large amount though the stack frames get larger to hold
            ** the GCScanQ's.
            */
            pScanQ->q[pScanQ->queued++] = p;
            if (pScanQ->queued == MAX_SCAN_Q) {
                METER(_pr_scanDepth++);
                ScanScanQ(pScanQ);
            }
        }
    }
}

/************************************************************************/

/*
** Clear out header bits for the space occupied by a free chunk.
**
** XXX tune
*/
static void ClearHBits(GCSeg *sp, prword_t *p, size_t bytes)
{
    GCTRACE(GC_SWEEP, ("clearing hbits for 0x%x to 0x%x", p,
		       (char*)p + bytes - 1));
    bytes >>= BYTES_PER_WORD_LOG2;
    while (bytes) {
	CLEAR_HBIT(sp, p);
	bytes--;
	p++;
    }
}

/*
** Empty the freelist for each segment. This is done to make sure that
** the root finding step works properly (otherwise, if we had a pointer
** into a free section, we might not find its header word and abort in
** FindObject)
*/
static void EmptyFreelists(void)
{
    GCFreeChunk GCPTR *cp;
    GCFreeChunk GCPTR *next;
    GCSeg *sp;
    prword_t GCPTR *p;
    size_t chunkSize;
    int32 bin;

    /*
    ** Run over the freelist and make all of the free chunks look like
    ** object debris.
    */
    for (bin = 0; bin < NUM_BINS; bin++) {
        cp = bins[bin];
        while (cp) {
            next = cp->next;
            sp = cp->segment;
            chunkSize = cp->chunkSize >> BYTES_PER_WORD_LOG2;
            p = (prword_t GCPTR *) cp;
            PR_ASSERT(chunkSize != 0);
            p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, chunkSize);
            SET_HBIT(sp, p);
            cp = next;
        }
        bins[bin] = 0;
    }
}

/*
** Sweep a segment, cleaning up all of the debris. Coallese the debris
** into GCFreeChunk's which are added to the freelist bins.
*/
static PRBool SweepSegment(GCSeg *sp)
{
    prword_t h, tix;
    prword_t GCPTR *p;
    prword_t GCPTR *np;
    prword_t GCPTR *limit;
    GCFreeChunk GCPTR *cp;
    size_t bytes, chunkSize;
    int32 totalFree;
    int32 bin, segmentSize;

    /*
    ** Now scan over the segment's memory in memory order, coallescing
    ** all of the debris into a FreeChunk list.
    */
    totalFree = 0;
    segmentSize = sp->limit - sp->base;
    p = (prword_t GCPTR *) sp->base;
    limit = (prword_t GCPTR *) sp->limit;
    PR_ASSERT(segmentSize > 0);
    while (p < limit) {
	chunkSize = 0;
	cp = (GCFreeChunk GCPTR *) p;

	/* Attempt to coallesce any neighboring free objects */
	for (;;) {
	    PR_ASSERT(IS_HBIT(sp, p) != 0);
	    h = p[0];
	    bytes = OBJ_BYTES(h);
	    PR_ASSERT(bytes != 0);
	    np = (prword_t GCPTR *) ((char GCPTR *)p + bytes);
            tix = GET_TYPEIX(h);
	    if ((h & MARK_BIT) && (tix != FREE_MEMORY_TYPEIX)) {
		/* Not a free object. Time to stop looking */
#ifdef DEBUG
		if (tix != FREE_MEMORY_TYPEIX) {
		    PR_ASSERT(_pr_gcTypes[tix] != 0);
		}
#endif
		p[0] = h & ~(MARK_BIT|FINAL_BIT);
		GCTRACE(GC_SWEEP, ("busy 0x%x (%d)", p, bytes));
		break;
	    }
	    GCTRACE(GC_SWEEP, ("free 0x%x (%d)", p, bytes));

	    /* Found a free object */
            if (_pr_gcTypes[tix] && _pr_gcTypes[tix]->free) {
                (*_pr_gcTypes[tix]->free)(p + 1);
            }
	    chunkSize = chunkSize + bytes;
	    if (np == limit) {
		/* Found the end of heap */
		break;
	    }
	    PR_ASSERT(np < limit);
	    p = np;
	}

	if (chunkSize) {
	    GCTRACE(GC_SWEEP, ("free chunk 0x%x to 0x%x (%d)",
			       cp, (char*)cp + chunkSize - 1, chunkSize));
	    if (chunkSize < MIN_ALLOC) {
		/* Lost a tiny fragment until (maybe) next time */
                METER(meter.wastedBytes += chunkSize);
		p = (prword_t GCPTR *) cp;
		chunkSize >>= BYTES_PER_WORD_LOG2;
		PR_ASSERT(chunkSize != 0);
		p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, chunkSize);
		SET_HBIT(sp, p);
	    } else {
                /* See if the chunk constitutes the entire segment */
                if ((int32)chunkSize == segmentSize) {
                    /* Free up the segment right now */
		    ShrinkGCHeap(sp);
		    return PR_TRUE;
                }

                /* Put free chunk into the appropriate bin */
                bin = chunkSize >> MIN_ALLOC_LOG2;
                if (bin >= NUM_BINS) {
                    bin = NUM_BINS - 1;
                }
                cp->segment = sp;
		cp->chunkSize = chunkSize;
                cp->next = bins[bin];
                bins[bin] = cp;
                METER(meter.numFreeChunks++);

		ClearHBits(sp, (prword_t GCPTR*)cp, chunkSize);
#ifdef DEBUG
		/*
		** XXX I wonder if the sweeper should always initialize
		** memory and then we can take out the zeroing code in
		** AllocMemory? If we do this, then we need to go nuke
		** the memcpy's and memset's buried in java.
		*/
		if (_pr_gcData.flags & GC_DEBUG) {
		    memset(cp+1, 0xf8, chunkSize - sizeof(*cp));
		}
#endif
		totalFree += chunkSize;
	    }
	}

	/* Advance to next object */
	p = np;
    }

    PR_ASSERT(totalFree <= segmentSize);

    _pr_gcData.freeMemory += totalFree;
    _pr_gcData.busyMemory += (sp->limit - sp->base) - totalFree;
    return PR_FALSE;
}

/************************************************************************/

PRCList _pr_pendingFinalQueue;
PRCList _pr_finalQueue;

typedef struct GCFinalStr {
    PRCList links;
    prword_t *object;
} GCFinal;

/*
** Find pointer to GCFinal struct from the list linkaged embedded in it
*/
#define FinalPtr(_qp) \
    ((GCFinal*) ((char*) (_qp) - offsetof(GCFinal,links)))

#ifdef DEBUG
int32 finalNodes = 0;
#endif

/* XXX revist these: we might want to keep a freelist... */
static GCFinal *AllocFinalNode(void)
{
#ifdef DEBUG
    finalNodes++;
#endif
    return calloc(1, sizeof(GCFinal));
}

static void FreeFinalNode(GCFinal *node)
{
    free(node);
#ifdef DEBUG
    finalNodes--;
#endif
}

/*
** Prepare for finalization. At this point in the GC cycle we have
** identified all of the live objects. For each object on the
** pendingFinalizationQueue see if the object is alive or dead. If it's
** dead, resurrect it and remove it from the pendingFinalizationQueue to
** the finalizationQueue (object's only get finalized once).
**
** Once the pendingFinalizationQueue has been processed we can finish the
** GC and free up memory and release the threading lock. After that we
** can invoke the finalization procs for each object that is on the
** finalizationQueue.
*/
static void PrepareFinalize(void)
{
    PRCList *qp;
    GCFinal *fp;
    prword_t h;
    prword_t *p;
    void (*live_object)(void **base, int32 count);

    /* This must be done under the same lock that the finalizer uses */
    PR_ASSERT( GC_IS_LOCKED() );

    /* cache this ptr */
    live_object = _pr_gcData.liveObject;

    /*
     * Pass #1: Identify objects that are to be finalized, set their
     * FINAL_BIT.
     */
    qp = _pr_pendingFinalQueue.next;
    while (qp != &_pr_pendingFinalQueue) {
	fp = FinalPtr(qp);
	qp = qp->next;
	h = fp->object[0];		/* Grab header word */
	if (h & MARK_BIT) {
	    /* Object is already alive */
	    continue;
	}
	PR_ASSERT(_pr_gcTypes[GET_TYPEIX(h)] != 0);
	PR_ASSERT(_pr_gcTypes[GET_TYPEIX(h)]->finalize != 0);
	fp->object[0] |= FINAL_BIT;
	GCTRACE(GC_FINAL, ("moving %p (%d) to finalQueue",
			   fp->object, OBJ_BYTES(h)));
    }

    /*
     * Pass #2: For each object that is going to be finalized, move it to
     * the finalization queue and resurrect it
     */
    qp = _pr_pendingFinalQueue.next;
    while (qp != &_pr_pendingFinalQueue) {
	fp = FinalPtr(qp);
	qp = qp->next;
	h = fp->object[0];		/* Grab header word */
	if ((h & FINAL_BIT) == 0) {
	    continue;
	}

	/* Resurrect the object and any objects it refers to */
        p = &fp->object[1];
	(*live_object)((void**)&p, 1);
	PR_REMOVE_LINK(&fp->links);
	PR_APPEND_LINK(&fp->links, &_pr_finalQueue);
    }
}

/*
** Scan the finalQ, marking each and every object on it live.  This is
** necessary because we might do a GC before objects that are on the
** final queue get finalized. Since there are no other references
** (otherwise they would be on the final queue), we have to scan them.
** This really only does work if we call the GC before the finalizer
** has a chance to do its job.
*/
void _PR_ScanFinalQueue(void *notused)
{
    PRCList *qp;
    GCFinal *fp;
    prword_t *p;
    void (*live_object)(void **base, int32 count);

    live_object = _pr_gcData.liveObject;
    qp = _pr_finalQueue.next;
    while (qp != &_pr_finalQueue) {
	fp = FinalPtr(qp);
	GCTRACE(GC_FINAL, ("marking 0x%x (on final queue)", fp->object));
        p = &fp->object[1];
	(*live_object)((void**)&p, 1);
	qp = qp->next;
    }
}

#ifdef DEBUG
int32
QueueLength(PRCList* queue)
{
    int32 count = 0;
    PRCList* qp = queue->next;
    while (qp != queue) {
	count++;
	qp = qp->next;
    }
    return count;
}
#endif

void FinalizerLoop(void)
{
    GCFinal *fp;
    prword_t GCPTR *p;
    prword_t h, tix;
    GCType *gct;

    LOCK_GC();
    for (;;) {
	PR_Wait(_pr_gcData.lock, LL_MAXINT);

#if defined(_WIN32) && defined(MSWINDBGMALLOC)
	_RPT0(_CRT_WARN, "--finalization---------------------------------------\n");
	_RPT3(_CRT_WARN, "finalQueue %ld pendingFinalQueue %ld nodes %ld\n", 
	      QueueLength(&_pr_finalQueue), QueueLength(&_pr_pendingFinalQueue), finalNodes);
#endif
	GCTRACE(GC_FINAL, ("begin finalization"));
	while (_pr_finalQueue.next != &_pr_finalQueue) {
	    fp = FinalPtr(_pr_finalQueue.next);
	    PR_REMOVE_LINK(&fp->links);
	    p = fp->object;
	    FreeFinalNode(fp);

	    h = p[0];		/* Grab header word */
	    tix = GET_TYPEIX(h);
	    if ((gct = _pr_gcTypes[tix]) == 0) {
		/* Heap is probably corrupted */
		continue;
	    }
	    GCTRACE(GC_FINAL, ("finalize 0x%x (%d)", p, OBJ_BYTES(h)));

	    /*
	    ** Give up the GC lock so that other threads can allocate memory
	    ** while this finalization method is running. Get it back
	    ** afterwards so that the list remains thread safe.
	    */
        UNLOCK_GC();
	    PR_ASSERT(gct->finalize != 0);
	    (*gct->finalize)(p + 1);
        LOCK_GC();
	}
	GCTRACE(GC_FINAL, ("end finalization"));
    }
}

static void NotifyFinalizer(void)
{
    if (!PR_CLIST_IS_EMPTY(&_pr_finalQueue)) {
    PR_ASSERT( GC_IS_LOCKED() );
	PR_Notify(_pr_gcData.lock);
    }
}

void _PR_CreateFinalizer(void)
{
    if (!_pr_gcData.finalizer) {
	_pr_gcData.finalizer = PR_CreateThread("finalizer", 1, 0);
    
    if (_pr_gcData.finalizer == NULL)
	    /* We are doomed if we can't start the finalizer */
        PR_Abort();

	_pr_gcData.finalizer->flags |= _PR_SYSTEM;
	if (PR_Start(_pr_gcData.finalizer, (void (*)(void*,void*))FinalizerLoop,
		     0, 0) < 0) {
	    /* We are doomed if we can't start the finalizer */
	    PR_Abort();
	}
    }
}

PR_PUBLIC_API(void) PR_ForceFinalize()
{
    LOCK_GC();
    while (!PR_CLIST_IS_EMPTY(&_pr_finalQueue)) {
    PR_ASSERT( GC_IS_LOCKED() );
	PR_Notify(_pr_gcData.lock);
    }
    UNLOCK_GC();

    /* XXX I don't know how to make it wait (yet) */
}

/************************************************************************/

/*
** Perform a complete garbage collection
*/
#include "prdump.h"     /* XXX */


static void dogc(void)
{
    RootFinder *rf;
    GCScanQ scanQ;
    GCSeg *sp, *esp;
#ifdef GCMETER
    int64 start, end;
#endif

#ifdef GCMETER
    start = PR_NowMS();
#endif

    /*
    ** Stop all of the other threads. This also promises to capture the
    ** register state of each and every thread
    */
    PR_SingleThread();

#ifdef GCMETER
    /* Reset meter info */
    if (_pr_gcMeter & _GC_METER_STATS) {
        fprintf(stderr,
                "[GCSTATS: zeroed:%d busy:%d, alloced:%d+wasted:%d+free:%d = total:%d]\n",
                meter.zeroBytes, _pr_gcData.busyMemory,
                meter.allocBytes, meter.wastedBytes, _pr_gcData.freeMemory,
                _pr_gcData.allocMemory);
    }        
    memset(&meter, 0, sizeof(meter));
#endif

    PR_LOG(GC, out, ("begin mark phase; busy=%d free=%d total=%d",
                     _pr_gcData.busyMemory, _pr_gcData.freeMemory,
                     _pr_gcData.allocMemory));

    if (_pr_beginGCHook) {
	(*_pr_beginGCHook)(_pr_beginGCHookArg);
    }

    /*
    ** Initialize scanQ to all zero's so that root finder doesn't walk
    ** over it...
    */
    memset(&scanQ, 0, sizeof(scanQ));
    pScanQ = &scanQ;

    /******************************************/
    /* MARK PHASE */

    EmptyFreelists();

    /* Find root's */
    PR_LOG(GC, warn,
           ("begin mark phase; busy=%d free=%d total=%d",
	    _pr_gcData.busyMemory, _pr_gcData.freeMemory,
            _pr_gcData.allocMemory));
    METER(_pr_scanDepth = 0);
    rf = _pr_rootFinders;
    while (rf) {
	GCTRACE(GC_ROOTS, ("finding roots in %s", rf->name));
	(*rf->func)(rf->arg);
	rf = rf->next;
    }
    GCTRACE(GC_ROOTS, ("done finding roots"));

    /* Scan remaining object's that need scanning */
    ScanScanQ(&scanQ);
    PR_ASSERT(pScanQ == &scanQ);
    PR_ASSERT(scanQ.queued == 0);
    METER({
	if (_pr_scanDepth > _pr_maxScanDepth) {
	    _pr_maxScanDepth = _pr_scanDepth;
	}
    });

    /******************************************/
    /* FINALIZATION PHASE */

    METER(_pr_scanDepth = 0);
    PrepareFinalize();

    /* Scan any resurrected objects found during finalization */
    ScanScanQ(&scanQ);
    PR_ASSERT(pScanQ == &scanQ);
    PR_ASSERT(scanQ.queued == 0);
    METER({
	if (_pr_scanDepth > _pr_maxScanDepth) {
	    _pr_maxScanDepth = _pr_scanDepth;
	}
    });

    /******************************************/
    /* SWEEP PHASE */

    /*
    ** Sweep each segment clean. While we are at it, figure out which
    ** segment has the most free space and make that the current segment.
    */
    GCTRACE(GC_SWEEP, ("begin sweep phase"));
    _pr_gcData.freeMemory = 0;
    _pr_gcData.busyMemory = 0;
    sp = segs;
    esp = sp + nsegs;
    while (sp < esp) {
        if (SweepSegment(sp)) {
            /*
            ** Segment is now free and has been replaced with a different
            ** segment object.
            */
            esp--;
            continue;
        }
        sp++;
    }

#ifdef GCMETER
    end = PR_NowMS();
    LL_SUB(end, end, start);
    PR_LOG(GC, out,
	   ("done; busy=%d free=%d time=%lldms",
	    _pr_gcData.busyMemory, _pr_gcData.freeMemory, end));
#ifdef DEBUG_kipp	/* jwz asked that this be removed */
    fprintf(stderr, "[GC done busy=%d free=%d chunks=%d total=%d time=%lldms]\n",
	    _pr_gcData.busyMemory, _pr_gcData.freeMemory, 
            meter.numFreeChunks, _pr_gcData.allocMemory, end);
#endif
    if (_pr_gcMeter & _GC_METER_FREE_LIST) {
        int32 bin;
        fprintf(stderr,
               "Freelist bins: minAlloc=%d numBins=%d maxBin=%d\n",
                MIN_ALLOC, NUM_BINS, (NUM_BINS - 1) * MIN_ALLOC);
        for (bin = 0; bin < NUM_BINS; bin++) {
            GCFreeChunk *cp = bins[bin];
            while (cp != NULL) {
                fprintf(stderr, "%3d: %p %8d\n", bin, cp, cp->chunkSize);
                cp = cp->next;
            }
        }
    }
#endif

    if (_pr_endGCHook) {
	(*_pr_endGCHook)(_pr_endGCHookArg);
    }

    /* clear the running total of the bytes allocated via BigAlloc() */
    bigAllocBytes = 0;

    /* And resume multi-threading */
    PR_MultiThread();

    /* Kick finalizer */
    NotifyFinalizer();
}

PR_PUBLIC_API(void) PR_GC(void)
{
#ifdef HW_THREADS
	LOCK_SCHEDULER();
#endif

    LOCK_GC();
    dogc();
    UNLOCK_GC();

#ifdef HW_THREADS
	UNLOCK_SCHEDULER();
#endif

}

/*******************************************************************************
 * Heap Walker
 ******************************************************************************/

typedef void (*WalkObject_t)(FILE *out, GCType* tp, prword_t *obj,
			     size_t bytes, PRBool detailed);
typedef void (*WalkUnknown_t)(FILE *out, prword_t tix, prword_t *p,
			      size_t bytes, PRBool detailed);
typedef void (*WalkFree_t)(FILE *out, prword_t *p, size_t size, PRBool detailed);
typedef void (*WalkSegment_t)(FILE *out, GCSeg* sp, PRBool detailed);

static void
pr_WalkSegment(FILE* out, GCSeg* sp, PRBool detailed,
	       char* enterMsg, char* exitMsg,
	       WalkObject_t walkObject, WalkUnknown_t walkUnknown, WalkFree_t walkFree)
{
    prword_t *p, *limit;

    p = (prword_t *) sp->base;
    limit = (prword_t *) sp->limit;
    if (enterMsg)
	fprintf(out, enterMsg, p);
    while (p < limit)
    {
	if (IS_HBIT(sp, p)) /* Is this an object header? */
	{
	    prword_t h = p[0];
	    prword_t tix = GET_TYPEIX(h);
	    size_t bytes = OBJ_BYTES(h);
	    prword_t* np = (prword_t*) ((char*)p + bytes);

	    GCType* tp = _pr_gcTypes[tix];
	    if ((0 != tp) && walkObject)
		walkObject(out, tp, p, bytes, detailed);
	    else if (walkUnknown)
		walkUnknown(out, tix, p, bytes, detailed);
	    p = np;
	}
	else
	{
	    /* Must be a freelist item */
	    size_t size = ((GCFreeChunk*)p)->chunkSize;
	    if (walkFree)
		walkFree(out, p, size, detailed);
	    p = (prword_t*)((char*)p + size);
	}
    }
    if (p != limit)
	fprintf(out, "SEGMENT OVERRUN (end should be at 0x%p)\n", limit);
    if (exitMsg)
	fprintf(out, exitMsg, p);
}

static void
pr_WalkSegments(FILE *out, WalkSegment_t walkSegment, PRBool detailed)
{
    GCSeg *sp = segs;
    GCSeg *esp;

    LOCK_GC();
    esp = sp + nsegs;
    while (sp < esp)
    {
	walkSegment(out, sp, detailed);
	sp++;
    }
    fprintf(out, "End of heap\n");
    UNLOCK_GC();
}

/*******************************************************************************
 * Heap Dumper
 ******************************************************************************/

PR_PUBLIC_API(void)
PR_DumpIndent(FILE *out, int indent)
{
    while (--indent >= 0)
	fprintf(out, " ");
}

static void
PR_DumpHexWords(FILE *out, prword_t *p, int nWords, 
		int indent, int nWordsPerLine)
{
    while (nWords > 0)
    {
	int i;

	PR_DumpIndent(out, indent);
	i = nWordsPerLine;
	if (i > nWords)
	    i = nWords;
	nWords -= i;
	while (i--)
	{
	    fprintf(out, "0x%.8X", *p++);
	    if (i)
		fputc(' ', out);
	}
	fputc('\n', out);
    }
}

static void
pr_DumpObject(FILE *out, GCType* tp, prword_t *p, 
	      size_t bytes, PRBool detailed)
{
    fprintf(out, "0x%p: 0x%.6X  ", p, bytes);
    if (tp->dump)
	(*tp->dump)(out, (void*) (p + 1), detailed, 0);
    PR_DumpHexWords(out, p, bytes>>2, 22, 4);
}

static void
pr_DumpUnknown(FILE *out, prword_t tix, prword_t *p, 
	       size_t bytes, PRBool detailed)
{
    fprintf(out, "0x%p: 0x%.6X  ", p, bytes);
    fprintf(out, "UNKNOWN KIND %d\n", tix);
    PR_DumpHexWords(out, p, bytes>>2, 22, 4);
}

static void
pr_DumpFree(FILE *out, prword_t *p, size_t size, PRBool detailed)
{
    fprintf(out, "0x%p: 0x%.6X  FREE\n", p, size);
}

static void
pr_DumpSegment(FILE* out, GCSeg* sp, PRBool detailed)
{
    pr_WalkSegment(out, sp, detailed,
		   "\n   Address: Length\n0x%p: Beginning of segment\n",
		   "0x%p: End of segment\n\n",
		   pr_DumpObject, pr_DumpUnknown, pr_DumpFree);
}

/*
** Dump out the GC heap.
*/
PR_PUBLIC_API(void)
PR_DumpGCHeap(FILE *out, PRBool detailed)
{
    pr_WalkSegments(out, pr_DumpSegment, detailed);
}

PR_PUBLIC_API(void)
PR_DumpMemory(PRBool detailed)
{
    PR_DumpToFile("memory.out", "Dumping memory", PR_DumpGCHeap, detailed);
}

/*******************************************************************************
 * Heap Summary Dumper
 ******************************************************************************/

PRSummaryPrinter summaryPrinter = NULL;
void* summaryPrinterClosure = NULL;

PR_PUBLIC_API(void) 
PR_RegisterSummaryPrinter(PRSummaryPrinter fun, void* closure)
{
    summaryPrinter = fun;
    summaryPrinterClosure = closure;
}

static void
pr_SummarizeObject(FILE *out, GCType* tp, prword_t *p,
		   size_t bytes, PRBool detailed)
{
    if (tp->summarize)
	(*tp->summarize)((void GCPTR*)(p + 1), bytes);
}

static void
pr_DumpSummary(FILE* out, GCSeg* sp, PRBool detailed)
{
    pr_WalkSegment(out, sp, detailed, NULL, NULL,
  		   pr_SummarizeObject, NULL, NULL);
}

PR_PUBLIC_API(void)
PR_DumpGCSummary(FILE *out, PRBool detailed)
{
    if (summaryPrinter) {
	pr_WalkSegments(out, pr_DumpSummary, detailed);
	summaryPrinter(out, summaryPrinterClosure);
    }
#ifdef _WIN32
    {
	extern int32 totalVirtual;
	fprintf(out, "Virtual memory reserve: %ld, in use: %ld (%ld%%)\n",
		GC_VMLIMIT, totalVirtual, (totalVirtual * 100 / GC_VMLIMIT));
    }
#endif
#if 0
    fprintf(out, "\nFinalizable objects:\n");
    {
	PRCList *qp;
	qp = _pr_pendingFinalQueue.next;
	while (qp != &_pr_pendingFinalQueue) {
	    GCFinal* fp = FinalPtr(qp);
	    prword_t h = fp->object[0];		/* Grab header word */
	    prword_t tix = GET_TYPEIX(h);
	    GCType* tp = _pr_gcTypes[tix];
	    size_t bytes = OBJ_BYTES(h);
	    pr_DumpObject(out, tp, fp->object, bytes, PR_FALSE);
	    qp = qp->next;
	}
    }
#endif
}

PR_PUBLIC_API(void)
PR_DumpMemorySummary(void)
{
    PR_DumpToFile("memory.out", "Memory Summary", PR_DumpGCSummary, PR_FALSE);
}

/*******************************************************************************
 * End Of Heap Walker 
 ******************************************************************************/

#if defined(DEBUG) && defined(_WIN32)
static void DumpApplicationHeap(FILE *out, HANDLE heap)
	{
	PROCESS_HEAP_ENTRY entry;
	DWORD err;

	if (!HeapLock(heap))
		OutputDebugString("Can't lock the heap.\n");
	entry.lpData = 0;
	fprintf(out, "   address:       size ovhd region\n");
	while (HeapWalk(heap, &entry))
		{
		WORD flags = entry.wFlags;

		fprintf(out, "0x%.8X: 0x%.8X 0x%.2X 0x%.2X  ", entry.lpData, entry.cbData,
					 entry.cbOverhead, entry.iRegionIndex);
		if (flags & PROCESS_HEAP_REGION)
			fprintf(out, "REGION  committedSize=0x%.8X uncommittedSize=0x%.8X firstBlock=0x%.8X lastBlock=0x%.8X",
					entry.Region.dwCommittedSize, entry.Region.dwUnCommittedSize,
					entry.Region.lpFirstBlock, entry.Region.lpLastBlock);
		else if (flags & PROCESS_HEAP_UNCOMMITTED_RANGE)
			fprintf(out, "UNCOMMITTED");
		else if (flags & PROCESS_HEAP_ENTRY_BUSY)
			{
			if (flags & PROCESS_HEAP_ENTRY_DDESHARE)
				fprintf(out, "DDEShare ");
			if (flags & PROCESS_HEAP_ENTRY_MOVEABLE)
				fprintf(out, "Moveable Block  handle=0x%.8X", entry.Block.hMem);
			else
				fprintf(out, "Block");
			}
		fprintf(out, "\n");
		}
	if ((err = GetLastError()) != ERROR_NO_MORE_ITEMS)
		fprintf(out, "ERROR %d iterating through the heap\n", err);
	if (!HeapUnlock(heap))
		OutputDebugString("Can't unlock the heap.\n");
	}

static void DumpApplicationHeaps(FILE *out)
	{
	HANDLE mainHeap;
	HANDLE heaps[100];
	int32 nHeaps;
	int32 i;

	mainHeap = GetProcessHeap();
	nHeaps = GetProcessHeaps(100, heaps);
	if (nHeaps > 100)
		nHeaps = 0;
	fprintf(out, "%d heaps:\n", nHeaps);
	for (i = 0; i<nHeaps; i++)
		{
		HANDLE heap = heaps[i];

		fprintf(out, "Heap at 0x%.8X", heap);
		if (heap == mainHeap)
			fprintf(out, " (main)");
		fprintf(out, ":\n");
		DumpApplicationHeap(out, heap);
		fprintf(out, "\n");
		}
	fprintf(out, "End of heap dump\n\n");
	}


PR_PUBLIC_API(void) PR_DumpApplicationHeaps(void)
	{
	FILE *out;

	OutputDebugString("Dumping heaps...");
	out = fopen("heaps.out", "a");
	if (!out)
		OutputDebugString("Can't open \"heaps.out\"\n");
	else
		{
		struct tm *newtime;
		time_t aclock;

		time(&aclock);
		newtime = localtime(&aclock);
		fprintf(out, "Heap dump on %s\n", asctime(newtime));	/* Print current time */
		DumpApplicationHeaps(out);
		fprintf(out, "\n\n");
		fclose(out);
		}
	OutputDebugString(" done\n");
	}
#else

PR_PUBLIC_API(void) PR_DumpApplicationHeaps(void)
	{
	}
#endif

/************************************************************************/

/*
** Scan the freelist bins looking for a big enough chunk of memory to
** hold "bytes" worth of allocation. "bytes" already has the
** per-allocation header added to it. Return a pointer to the object with
** its per-allocation header already prepared.
*/
static prword_t *BinAlloc(int cbix, size_t bytes, int dub)
{
    GCFreeChunk *cp, *cpNext;
    GCFreeChunk **cpp;
    GCSeg *sp;
    size_t chunkSize;
    prword_t words;
    prword_t GCPTR *p;
    prword_t GCPTR *np;
    prword_t h;
    int32 bin, newbin;

    /* Compute bin that allocation belongs in */
    bin = bytes >> MIN_ALLOC_LOG2;
    if (bin >= NUM_BINS) {
        bin = NUM_BINS - 1;
    }

    /* XXX Use a bitfield to skip over empty bins */

    /* Search in the bin, and larger bins, for a big enough piece */
    for (; bin < NUM_BINS; bin++) {
        cpp = &bins[bin];
        while ((cp = *cpp) != 0) {
            p = (prword_t GCPTR*) cp;
            sp = cp->segment;
            cpNext = cp->next;
            chunkSize = cp->chunkSize;

#ifdef DEBUG
            if (bin != NUM_BINS - 1) {
                PR_ASSERT(chunkSize >= bytes);
            }
#endif

            /* All memory is double aligned on 64 bit machines... */
#ifndef IS_64
            if (dub && (((prword_t)p & (BYTES_PER_DWORD-1)) == 0)) {
                /*
                ** Need an extra word if double aligning and "p" is
                ** misaligned. Note that we want "p" to NOT be on a
                ** double-word boundary so that the data that follows the one
                ** word object header will be on a double-word boundary!
                */
                if (chunkSize < bytes + BYTES_PER_WORD) {
                    cpp = &cp->next;
                    continue;
                }

                /*
                ** Consume the first word of the chunk with a dummy
                ** unreferenced object.
                */
                p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, 1);
                SET_HBIT(sp, p);
                p++;
                chunkSize -= BYTES_PER_WORD;
                PR_ASSERT(((prword_t)p & (BYTES_PER_DWORD-1)) != 0);
            } else
#endif
            {
                if (chunkSize < bytes) {
                    cpp = &cp->next;
                    continue;
                }
            }
            

            /* Found a large enough piece */
            np = (prword_t GCPTR*) ((char GCPTR*) p + bytes);
            chunkSize -= bytes;
            if (chunkSize < MIN_ALLOC) {
                /*
                ** Remaining chunk is too small to hold a GCFreeChunk. Just
                ** toss the fragment away, marking it as belonging to the
                ** free object allocator. GC Sweeper will put the pieces back
                ** together.
                */
                if (chunkSize) {
                    words = chunkSize >> BYTES_PER_WORD_LOG2;
                    PR_ASSERT(words != 0);
                    np[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, words);
                    SET_HBIT(sp, np);
                    _pr_gcData.freeMemory -= chunkSize;
                    METER(meter.wastedBytes += chunkSize);
                }
                *cpp = cpNext;
            } else {
                /*
                ** Setup GCFreeChunk data for remaining portion. It might
                ** need to move to a new bin.
                */
                PR_ASSERT(chunkSize != 0);
                newbin = chunkSize >> MIN_ALLOC_LOG2;
                if (newbin >= NUM_BINS - 1) {
                    newbin = NUM_BINS - 1;
                }
                cp = (GCFreeChunk*) np;
                cp->segment = sp;
                cp->chunkSize = chunkSize;
                if (newbin != bin) {
                    *cpp = cpNext;                /* take old cp off old list */
                    cp->next = bins[newbin];      /* put new cp on new list */
                    bins[newbin] = cp;
                } else {
                    cp->next = cpNext;
                    *cpp = (GCFreeChunk GCPTR*) np;
                }
            }
            h = MAKE_HEADER(cbix, (bytes >> BYTES_PER_WORD_LOG2));
            p[0] = h;
            SET_HBIT(sp, p);
            _pr_gcData.freeMemory -= bytes;
            _pr_gcData.busyMemory += bytes;
            return p;
        }
    }
    return 0;
}

/*
** Allocate a piece of memory that is "big" in it's own segment.  Make
** the object consume the entire segment to avoid fragmentation.  When
** the object is no longer referenced, the segment is freed.
*/
static prword_t *BigAlloc(int cbix, size_t bytes, int dub)
{
    GCSeg *sp;
    prword_t *p, h;
    int32 chunkSize;

    /*
    ** If the number of bytes allocated via BigAlloc() since the last GC
    ** exceeds BIG_ALLOC_GC_SIZE then do a GC Now...
    */
    if (bigAllocBytes >= BIG_ALLOC_GC_SIZE) {
        dogc();
    }
    bigAllocBytes += bytes;

    /* Get a segment to hold this allocation */
#ifndef IS_64
    /* XXX this has to be a waste with all the rounding up going on, right? */
    sp = GrowHeapExactly(bytes + (dub ? BYTES_PER_WORD : 0));
#else
    sp = GrowHeapExactly(bytes);
#endif

    if (sp) {
        p = (prword_t*) sp->base;
        chunkSize = sp->limit - sp->base;

        /* All memory is double aligned on 64 bit machines... */
#ifndef IS_64
        if (dub && (((prword_t)p & (BYTES_PER_DWORD-1)) == 0)) {
            /*
            ** Consume the first word of the chunk with a dummy
            ** unreferenced object.
            */
            p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, 1);
            SET_HBIT(sp, p);
            p++;
            chunkSize -= BYTES_PER_WORD;
            _pr_gcData.freeMemory -= BYTES_PER_WORD;
            _pr_gcData.busyMemory += BYTES_PER_WORD;
            PR_ASSERT(((prword_t)p & (BYTES_PER_DWORD-1)) != 0);
        }
#endif
        /* Consume the *entire* segment with a single allocation */
        h = MAKE_HEADER(cbix, (chunkSize >> BYTES_PER_WORD_LOG2));
        p[0] = h;
        SET_HBIT(sp, p);
        _pr_gcData.freeMemory -= chunkSize;
        _pr_gcData.busyMemory += chunkSize;
	return p;
    }
    return 0;
}

int allocationEnabled = 1;	/* we disable gc allocation during low memory conditions */

PR_PUBLIC_API(void)
PR_EnableAllocation(int yesOrNo)
{
    allocationEnabled = yesOrNo;
}

/*
** Allocate memory from the GC Heap. Performs garbage collections if
** memory gets tight and grows the heap as needed. May return NULL if
** memory cannot be found.
**
** XXX win16: should we use this to do the HUGE allocs for big things?
*/
PR_PUBLIC_API(prword_t GCPTR *)
PR_AllocMemory(prword_t requestedBytes, int tix, int flags)
{
    prword_t bytes, dataBytes;
    prword_t GCPTR *p;
    int tries;
    GCFinal *final;
    prword_t *(*alloc)(int cbix, size_t bytes, int dub);
#if defined(XP_PC) && !defined(_WIN32)
    prword_t shiftVal;
#endif

    if (!allocationEnabled) return NULL;

#ifdef DEBUG
    if (_pr_do_a_dump) {
	int64 pause;

	LL_I2L(pause, 1000000L);
	/*
	** Collect, pause for a second (lets finalizer run), and then GC
	** again.
	*/
	PR_GC();
	PR_Sleep(pause);
	PR_GC();
	PR_DumpGCHeap(_pr_dump_file, PR_TRUE);
	_pr_do_a_dump = 0;
    }
#endif

    PR_ASSERT(_pr_gcTypes[tix] != 0);
    final = 0;
    tries = 0;

    bytes = requestedBytes;
#if defined(XP_PC) && !defined(_WIN32)
    PR_ASSERT( bytes < MAX_ALLOC_SIZE );
#endif
    /*
    ** Align bytes to a multiple of a prword_t, then add in enough space
    ** to hold the header word.
    **
    ** MSVC 1.52 crashed on the ff. code because of the "complex" shifting :-(
    */
#if !defined(XP_PC) || defined(_WIN32) 
    if (flags & PR_ALLOC_DOUBLE) {
	bytes = (bytes + BYTES_PER_DWORD - 1) >> BYTES_PER_DWORD_LOG2;
	bytes <<= BYTES_PER_DWORD_LOG2;
    } else {
	bytes = (bytes + BYTES_PER_WORD - 1) >> BYTES_PER_WORD_LOG2;
	bytes <<= BYTES_PER_WORD_LOG2;
    }

    bytes += sizeof(prword_t);
    bytes = ((bytes + MIN_ALLOC - 1) >> MIN_ALLOC_LOG2) << MIN_ALLOC_LOG2;

#else 
    /* 
    ** For WIN16 the shifts have been broken out into separate statements
    ** to prevent the compiler from crashing...
    */
    if (flags & PR_ALLOC_DOUBLE) {
	bytes += BYTES_PER_DWORD - 1L;
	shiftVal = BYTES_PER_DWORD_LOG2;
    } else {
	bytes += BYTES_PER_WORD - 1L;
	shiftVal = BYTES_PER_WORD_LOG2;
    }
    bytes >>= shiftVal;
    bytes <<= shiftVal;

    bytes += sizeof(prword_t);
    bytes += MIN_ALLOC - 1L;
    shiftVal = MIN_ALLOC_LOG2;
    bytes >>= shiftVal;
    bytes <<= shiftVal;
#endif
    
    dataBytes = bytes - sizeof(prword_t);

    if (_pr_gcTypes[tix]->finalize != 0) {
	/*
	** Allocate a GCFinal struct for this object in advance. Don't put
	** it on the pending list until we have allocated the object
	*/
	final = AllocFinalNode();
	if (!final) {
	    /* XXX THIS IS NOT ACCEPTABLE! */
	    return 0;
	}
    }
    LOCK_GC();

    if ((bytes >= BIG_ALLOC) && (nsegs < MAX_SEGS)) {
	alloc = BigAlloc;
    } else {
	alloc = BinAlloc;
    }
    p = (*alloc)(tix, (size_t)bytes, flags & PR_ALLOC_DOUBLE);
    if (p) {
	goto found;
    }

    /* Run garbage collector */
    GCTRACE(GC_ALLOC, ("force GC: want %d", bytes));
    dogc();
    PR_ASSERT( GC_IS_LOCKED() );

    /* Try again now that gc has run */
    p = (*alloc)(tix, (size_t)bytes, flags & PR_ALLOC_DOUBLE);
    if (p) {
	goto found;
    }

    /*
    ** Last resort: if we can grow the heap, do so now.
    */
    if (_pr_gcData.allocMemory < _pr_gcData.maxMemory) {
        if (!GrowHeap(PR_MAX(bytes,segmentSize))) {
            goto lost;
        }
    }
    p = BinAlloc(tix, (size_t)bytes, flags & PR_ALLOC_DOUBLE);
    if (!p) {
        /* Total lossage */
        goto lost;
    }

  found:
    if (final) {
	GCTRACE(GC_ALLOC, ("alloc 0x%x (%d) final=0x%x", p, bytes, final));
	final->object = p;
	PR_APPEND_LINK(&final->links, &_pr_pendingFinalQueue);
    } else {
	GCTRACE(GC_ALLOC, ("alloc 0x%x (%d)", p, bytes));
    }

    /*
    ** XXX This is a total hack that eliminates duplicate monitor entry
    ** for java. This total hack speeds up memory allocation alot!
    */
    if (flags & PR_ALLOC_ZERO_HANDLE) {
        p[1] = 0;
        p[2] = 0;
    }

    UNLOCK_GC();
    METER(meter.allocBytes += bytes);
    METER(meter.wastedBytes += (bytes - requestedBytes));
    if (flags & PR_ALLOC_CLEAN) {
	memset(p + 1, 0, (size_t)dataBytes);
	METER(meter.zeroBytes += dataBytes);
    }
#ifdef DEBUG
    else if (_pr_gcData.flags & GC_DEBUG) {
	/*
	** Because the caller didn't request a zeroing of memory we
	** should fill it with a pattern that won't trigger the assert in
	** LiveObject.
	*/
	memset(p+1, 0, (size_t)dataBytes);
    }
#endif
    if (freeSegs) {
	FreeSegments();
    }
    return p + 1;

  lost:
    /* Out of memory */
    if (final) {
	FreeFinalNode(final);
    }
    UNLOCK_GC();
    if (freeSegs) {
	FreeSegments();
    }
    return 0;
}

PR_PUBLIC_API(void) PR_InitGC(prword_t flags, prword_t initialHeapSize)
{
    static char firstTime = 1;

    if (!firstTime) return;
    firstTime = 0;

    /* This *MUST* be true */
    PR_ASSERT(sizeof(GCFreeChunk) <= MIN_ALLOC);

#if defined(XP_PC) && !defined(_WIN32)
    PR_ASSERT( initialHeapSize < MAX_ALLOC_SIZE );
#endif

    /* Get memory from MD code */
#ifdef DEBUG
    {
	char *ev = getenv("GC_SEGMENT_SIZE");
	if (ev && ev[0]) {
	    initialHeapSize = atoi(ev);
	    segmentSize = (size_t)initialHeapSize;
	}
	ev = getenv("GC_FLAGS");
	if (ev && ev[0]) {
	    flags |= atoi(ev);
	}
#ifdef GCMETER
        ev = getenv("GC_METER");
        if (ev && ev[0]) {
            _pr_gcMeter = atoi(ev);
        }
#endif
    }
#endif
    _pr_gcData.maxMemory   = MAX_SEGS * segmentSize;
    _pr_gcData.liveObject  = ProcessRoot;
    _pr_gcData.processRoot = ProcessRoot;

#ifdef DEBUG
    _pr_gcData.liveObjectLog  = (void (*)(const char*, ...)) nop;
    _pr_gcData.processRootLog = (void (*)(const char*, ...)) nop;
#endif

    PR_INIT_CLIST(&_pr_pendingFinalQueue);
    PR_INIT_CLIST(&_pr_finalQueue);
    _PR_InitGC(flags);

    /* Create finalizer thread */
    _PR_CreateFinalizer();
    /* Initialize the GC heap */
    GrowHeap((initialHeapSize < segmentSize)
             ? segmentSize
             : initialHeapSize);
}

#if defined(XP_PC) && !defined(_WIN32)
/*
** For WIN16 the GC_IN_HEAP() macro must call the private InHeap function.
** This public wrapper function makes this possible...
*/
PR_PUBLIC_API(PRBool)
PR_GC_In_Heap(void GCPTR *object)
{
	return InHeap( object ) != NULL;	
}
#endif

#ifdef DEBUG
void _PR_DumpSegmentFreelist(GCSeg *sp)
{
#if 0
    GCFreeChunk *cp;
    int32 bin;

    for (bin = 0; bin < NUM_BINS; bin++) {
        cp = bins[bin];
        while (cp) {
            fprintf(stderr, "%08x: %5d -> %08x\n", cp, cp->chunkSize, cp->next);
            cp = cp->next;
        }
    }
#endif
}
#endif
