/*
 * @(#)gc.c	1.94 95/12/06
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*
 * Java memory management and garbage collection
 *
 * Implementation notes:
 *
 * We really don't need to allocate mark bits for soft references.
 * It might be better to keep a small list of the soft references we
 * find while doing the mark phase.  This would save 24KB of mark bits,
 * and shouldn't cost too much in the way of cpu.
 *
 * I [Tim] tried to split this file up into two: heap.c, covering heap
 * initialization and allocation routines, and gc.c, covering GC and heap
 * expansion.  Doing so changed the basic heap pointers from statics,
 * and the result (for the SunPro compiler, at least), was a 10% slowdown
 * of the garbage collector.  So now we've got a big file again.
 */

#include "typedefs.h"
#include "profile.h"
#include "tree.h"
#include "oobj.h"
#include "interpreter.h"
#include "typecodes.h"
#include "monitor.h"
#include "javaThreads.h"
#include "signature.h"
#include "monitor_cache.h"
#include "finalize.h"

#include "sys_api.h"

#include "java_lang_Thread.h"

int manageAllocFailure(long n, int overflow_type, int overflow_act);
extern void runFinalization(void);

/* Define this if you want lots of verbose info at runtime */
#define TRACEGC 1
int tracegc = 0;

/*
 * Lock against heap modification.
 */
sys_mon_t *_heap_lock;
#define HEAP_LOCK_INIT()    monitorRegister(_heap_lock, "Heap lock")
#define HEAP_LOCK()	    sysMonitorEnter(_heap_lock)
#define HEAP_UNLOCK()	    sysMonitorExit(_heap_lock)
#define HEAP_LOCKED()	    sysMonitorEntered(_heap_lock)

extern sys_mon_t *_hasfinalq_lock;

/*
 * Define this if you want the mark phase to detect pointers into the
 * interior of objects.
 */
/* #define CHECK_INTERIOR_POINTERS */

#define OBJECTGRAIN     8
#define HANDLEGRAIN     8
#define BITSPERCHAR     8

/* 
 * Heap layout, pointers and pointer aliases set by SetLimits():
 *
 * |----------------------------mapped memory---------------------------|
 *          |------------------committed memory----------------|
 * 
 *  -------------------------------------------------------------------
 * |   ... <|  handles   |          objects                    |> ...  |
 *  -------------------------------------------------------------------
 *  ^        ^          ^ ^                                   ^         ^
 *  |        |          | |                                   |         |
 *  |       hpool       | hpoollimit                  opoollimit        |
 * heapbase  |          | opool                               |     heaptop
 *          hpmin   hpmax opmin                            opmax
 *
 * Constraints:
 *    hpool/hpmin and hpoollimit are HANDLEGRAIN aligned
 *    opool/opmin must be OBJECTGRAIN aligned *plus* sizeof(hdr)
 *    the committed memory must be contained by the mapped memory
 */

/* The extent of the object pool (must be committed memory) */
static unsigned char *opool, *opoollimit, *opoolhand;
/* The extent of the handle pool (must be committed memory) */
static unsigned char *hpool, *hpoollimit, *hpoolhand;
/* The extent of the mapped memory */
static unsigned char *heapbase, *heaptop;

#define ValidObject(p)	((((int)(p)) & (OBJECTGRAIN-1)) == 0 && 	\
			 (unsigned char *)(p) >= opmin && 		\
			 (unsigned char *)(p) <  opmax)
#define ValidHandle(p)	(((int) (p) & (sizeof(JHandle)-1)) == 0 && 	\
		         (unsigned char *)(p) >= hpmin && 		\
		         (unsigned char *)(p) <= hpmax)
/* ValidHorO() assumes OBJECTGRAIN=sizeof(JHandle)... */
#define ValidHorO(p)	(((int) (p) & (OBJECTGRAIN-1)) == 0 && 		\
		         (unsigned char *)(p) >= hpmin && 		\
		         (unsigned char *)(p) <= opmax)
#define SetLimits() 							\
	register unsigned char *const opmin = opool, 			\
			       *const opmax = opoollimit, 		\
			       *const hpmin = hpool, 			\
			       *const hpmax = hpoollimit-sizeof(JHandle)

static unsigned int *markbits;
#define BITSPERMARK (BITSPERCHAR * sizeof(*markbits))	/* 32 */
static long FreeObjectCtr;
static long TotalObjectCtr;
static long FreeHandleCtr;
static long TotalHandleCtr;
static long marksize;
static long markmax;
static long FreeMemoryLowWaterMark;

unsigned char *gc0(int, unsigned int);

/*
 * Memory block header (bottom three bits are flags):
 *
 * -------------------------------------------------------------
 * | <--- length --->| unused | <- obj swapped -> | <- free -> |
 * -------------------------------------------------------------
 * 31                3        2                   1            0
 */
typedef long hdr;

#define obj_geth(p) (*((hdr *)(p)))
#define obj_seth(p, h) (*((hdr *)(p)) = (h))
#define h_len(h) ((h) & ~(OBJECTGRAIN-1))
#define h_free(h) ((h) & 1)
#define h_bumplen(h, l) ((h) += (l))

#define obj_len(p) (obj_geth(p)&~(OBJECTGRAIN-1))
#define obj_setlf(p, l, f) (obj_geth(p) = (l)|(f))
#define obj_bumplen(p, l) (obj_geth(p) += (l))
#define obj_free(p) (obj_geth(p)&1)
#define obj_setfree(p) (obj_geth(p) |= 1)
#define obj_clearfree(p) (obj_geth(p) &= ~1)

#define HardMark 3
#define SoftMark 1

/* Mark bit access assumes contiguity of handles and objects */
#define MARKINDEX(p)    (((unsigned char *)(p) - hpmin) >> 7)
#define BITOFFSET(p)    ((((unsigned char *)(p) - hpmin) >> 2) & 0x1e)

#define _MarkPtr(p, v)  (markbits[MARKINDEX(p)] |= (v) << BITOFFSET(p))
#define _ClearMarkPtr(p, v) (markbits[MARKINDEX(p)] &= ~((v) << BITOFFSET(p)))
#define _IsMarked(p)    ((markbits[MARKINDEX(p)] >> BITOFFSET(p)) &3)
#define MarkPtr(p, v) _MarkPtr(((unsigned int) (p) & ~(OBJECTGRAIN - 1)), v)
#define ClearMarkPtr(p, v) _ClearMarkPtr(((unsigned int)(p)&~(OBJECTGRAIN-1)),v)
#define IsMarked(p) _IsMarked((unsigned int) (p) & ~(OBJECTGRAIN - 1))

#define SOFTREFBAGSIZE 200  /* max number of soft refs to kill in one cycle */

/*
 * Types of overflows: we might respond to an overflow of a particular
 * error differently, e.g. expanding only the overflowing area.
 */
#define OVERFLOW_NONE	 0
#define OVERFLOW_OBJECTS 1
#define OVERFLOW_HANDLES 2

/*
 * Possible actions to take on overflows.  manageAllocFailure()
 * decides between these.
 */
#define OVERFLOW_ACT_FAIL	0
#define OVERFLOW_ACT_GC		1
#define OVERFLOW_ACT_FINALIZE	2
#define OVERFLOW_ACT_REFS	3
#define OVERFLOW_ACT_EXPAND	4
#define OVERFLOW_ACT_DESPERATE  5

/*
 * Rough counter of heap modification events: a lack of known permu-
 * tations of the heap is used to suppress async GC.  While this isn't
 * a perfect indicator of good times to GC, it is unlikely that there 
 * will be many good times to GC when we aren't turning over memory.
 * Current incrementers of heap_memory_changes include realObjectAlloc()
 * and execute_finalizer().
 */
int heap_memory_changes = 0;

int64_t
TotalObjectMemory(void)
{
    return int2ll(TotalObjectCtr);
}

int64_t
FreeObjectMemory(void)
{
    return int2ll(FreeObjectCtr);
}

int64_t
TotalHandleMemory(void)
{
    return int2ll(TotalHandleCtr);
}

int64_t
FreeHandleMemory(void)
{
    return int2ll(FreeHandleCtr);
}

static HObject *
realObjAlloc(struct methodtable *mptr, long n0)
{
    register unsigned char *p;
    register unsigned char *limit;
    register long n;
    HObject *ret;
    int overflow_type = OVERFLOW_NONE;
    int on_overflow = OVERFLOW_ACT_GC;

    n = (n0 + sizeof(hdr) + (OBJECTGRAIN - 1)) & ~(OBJECTGRAIN - 1);
    HEAP_LOCK();
    p = opoolhand;
    limit = opoollimit;
    while (1) {
	while (p < limit) {     /* search for a free block */
	    register unsigned char *next;
	    register hdr h = obj_geth(p);
	    register hdr h2;
	    sysAssert(h_len(h) >= sizeof(hdr));
	    next = p + h_len(h);
	    sysAssert(next <= opoollimit + sizeof(ClassObject));
	    if (h_free(h)) {
		while (h_free(h2 = obj_geth(next))) {
		    h_bumplen(h, h_len(h2));
		    next = p + h_len(h);
		    obj_seth(p, h);
		}
		h2 = h_len(h) - n;
		if (h2 >= 0) {  /* bingo! */
		    if (h2 > 0) {
			obj_setlf(p + n, h2, 1);
#ifdef DEBUG
			((long *) (p + n))[1] = 0x55555555;
#endif /* DEBUG */
		    }
		    obj_setlf(p, n, 0);
#ifdef DEBUG
		    ((long *) p)[1] = 0x55555555;
#endif /* DEBUG */
		    opoolhand = p + n;
		    sysAssert(((int) p + sizeof(hdr) & (OBJECTGRAIN - 1)) == 0);
		    FreeObjectCtr -= n;
		    sysAssert(FreeObjectCtr >= 0);
		    goto GotIt;
		}
	    }
	    p = next;
	}
	if (limit == opoollimit && opoolhand > opool) {
	    /* not found, search other half */
	    p = opool;
	    limit = opoolhand;
	    if (opoolhand == opoollimit) {
		opoolhand = p;
	    }
	    continue;
	}
#ifdef DEBUG
	if (verbosegc) {
	    fprintf(stderr, "<GC: out of object space wanting %d bytes>\n", n);
	}
#endif /* DEBUG */
	overflow_type = OVERFLOW_OBJECTS;

Overflow:
	/* Allocation failed for lack of space: garbage collect or expand */
	on_overflow = manageAllocFailure(n, overflow_type, on_overflow);
	if (on_overflow == OVERFLOW_ACT_FAIL) {
	    /* Tried everything, couldn't satisfy request, so fail */
	    HEAP_UNLOCK();
	    return 0;
	} else {
	    /* opoolhand has been reset by GC */
	    p = opool;
	    limit = opoollimit;
	}
    }

GotIt:                          /* p points to new object */
    ret = AllocHandle(mptr, (ClassObject *) (p + sizeof(hdr)));
    if (ret == 0) {
	obj_setfree(p);
	FreeObjectCtr += obj_len(p);
	sysAssert(FreeObjectCtr <= TotalObjectCtr);
	/* on_overflow is OVERFLOW_ACT_GC */
	overflow_type = OVERFLOW_HANDLES;
	goto Overflow;
    }

    heap_memory_changes++;
    HEAP_UNLOCK();
	    
    return ret;
}

/*
 * REMIND: Why don't we keep free handles in a linked list???
 */
HObject *
AllocHandle(struct methodtable *mptr, ClassObject *p)
{
    register unsigned char *hp = hpoolhand;
    register unsigned char *limit = hpoollimit - sizeof(JHandle);
    if (p == 0) {
	return 0;
    }
    while (hp <= limit) {
	if (((JHandle *) hp)->obj == 0) {
	    goto GotHandle;
	} else {
	    hp += sizeof(JHandle);
	}
    }
    limit = hpoolhand;
    hp = hpool;
#ifdef TRACEGC
    if (tracegc) {
	fprintf(stderr, "<GC: handle pool wraps>\n");
    }
#endif /* TRACEGC */
    while (hp < limit) {
	if (((JHandle *) hp)->obj == 0) {
	    goto GotHandle;
	} else {
	    hp += sizeof(JHandle);
	}
    }
#ifdef DEBUG
    if (verbosegc) {
	fprintf(stderr, "<GC: out of handle space>\n");
    }
#endif /* DEBUG */
    return 0;
GotHandle:
    ((JHandle *) hp)->methods = mptr;
    ((JHandle *) hp)->obj = (ClassObject *) p;
    hpoolhand = hp + sizeof(JHandle);
    FreeHandleCtr -= sizeof(JHandle);
    sysAssert(FreeHandleCtr >= 0);
    return (HObject *) hp;
}


/*
 * Calculate the size in bytes of an array of type t and length l.
 */
int32_t
sizearray(int32_t t, int32_t l)
{
    int size = 0;

    switch(t){
    case T_CLASS:
	size = sizeof(OBJECT);
	break;
    default:
	size = T_ELEMENT_SIZE(t);
	break;
    }
    size *= l;
    return size;
}

/*
 * Allocate an array of type 't' with initial size 'l'.  This routine
 * takes care of setting the method table pointer correctly.  It also
 * takes into consideration any special sizeing requirements, like 
 * those needed for arrays of classes.
 */
HObject *
ArrayAlloc(int t, int l)
{
    sysAssert(t >= T_CLASS && t < T_MAXNUMERIC);
/*
 *  Uncomment me to find all the places where the code creates zero length
 *  arrays.
 *
 *  if (l == 0) {
 *	extern void DumpThreads();
 *	printf("zero length array created %s[%d]\n", arrayinfo[t].name, l);
 *	DumpThreads();
 *  }
 */

    /*
     * Check whether there is in principle enough memory to satisfy the
     * allocation.  We want to fail this early because the array size
     * calculation might otherwise overflow, which would pass a garbage
     * size to realObjAlloc().  Note that a T_CLASS array allocates
     * an extra OBJECT; hence the l - 1 which otherwise makes the test
     * one element conservative.  Note that l may be 0!
     */
    if (l && ((l - 1) > ((heaptop - heapbase) / 
		   (t == T_CLASS ? sizeof(OBJECT) : T_ELEMENT_SIZE(t))))) {
 	return 0;
    }

    return realObjAlloc((struct methodtable *) mkatype(t, l),
	   sizearray(t, l) + (t == T_CLASS ? sizeof(OBJECT) : 0));
}

/*
 * Allocate an object.  This routine is in flux.  For now, the second 
 * parameter should always be zero and the first must alwaysd point to
 * a valid classblock.  This routine should not be used to allocate
 * arrays; use ArrayAlloc for that.
 */
HObject *
ObjAlloc(ClassClass *cb, long n0)
{
    HObject *handle;

#ifdef DEBUG
    if (n0 != 0 || cb == 0) {
	sysAbort();
    }
#endif /* DEBUG */

    n0 = cbInstanceSize(cb);
    handle = realObjAlloc(cbMethodTable(cb), n0);

    /* 
     * If the class is a normal class and has a finalization method, flag
     * the instance and prepend it to the head of the HasFinalizerQ.  Note
     * that handle can be nil if we are out of memory!
     */
    if (handle && obj_flags(handle) == T_NORMAL_OBJECT && cb->finalizer) {
	finalizer_t *final;

        final = (finalizer_t *) sysMalloc(sizeof(finalizer_t));
        final->handle = handle;
	HASFINALQ_LOCK();
        final->next = HasFinalizerQ;
        HasFinalizerQ = final;
	HASFINALQ_UNLOCK();
    }

    return handle;
}

/*
 * Search the handle table for instances of class Ref, and clear the
 * object pointer in the lowest priority group.  If async_call is true,
 * we are being called asynchronously, and should watch for pending
 * interrupts.
 *
 * Return the number of refs zeroed, and set the int argument pointer
 * to the number of refs found in the system.
 */
static int
clearRefPointers(int async_call, int free_space_goal, int *totalrefs)
{
    int nsoftrefs = 0;
    struct {
	JHandle *ref;
	long priority;
    } bag[SOFTREFBAGSIZE];
    JHandle *hp;
    int ninbag = 0;
    int freed = 0;
    SetLimits();

    for (hp = (JHandle *) hpool; hp <= (JHandle *) hpmax; hp++) {
	if (ValidObject(hp->obj)
		&& obj_flags(hp) == T_NORMAL_OBJECT
		&& CCIs(obj_classblock(hp), SoftRef)
		&& ValidHandle(obj_getslot(hp, 0))
		&& IsMarked(obj_getslot(hp, 0)) == SoftMark) {
	    int prio = obj_getslot(hp, 1);
	    if (ninbag >= SOFTREFBAGSIZE
		    && prio < bag[SOFTREFBAGSIZE - 1].priority) {
		ninbag--;
	    }
	    if (ninbag < SOFTREFBAGSIZE) {
		int i = ninbag;
		while (i > 0 && prio < bag[i - 1].priority) {
		    bag[i] = bag[i - 1];
		    i--;
		}
		bag[i].ref = hp;
		bag[i].priority = prio;
		ninbag++;
	    }
	    nsoftrefs++;
	}
    }

    if (async_call && INTERRUPTS_PENDING()) {
	return 0;
    }

    if (ninbag) {
	int i;
	int freemem;

	/* Don't free more than half the Refs when there are more than 10 */
	if ((i = (nsoftrefs + 1) >> 1) <= ninbag && ninbag > 10) {
	    ninbag = i;
	}

	freemem = FreeObjectCtr;
	for (i = ninbag; --i >= 0;) {
	    /*
	     * Only free as many refs as needed to meet free_space_goal.
	     * Of course, this assumes that there are no non-ref references
	     * to the freed objects, which needs not be the case.A
	     */
	    freemem += obj_len((char *)bag[i].ref - sizeof(hdr));
#ifdef TRACEGC
	    if (tracegc > 1) {
		fprintf(stderr, "clear soft ref: 0x%x: %s\n",
			(unsigned int) bag[i].ref, Object2CString(bag[i].ref));
	    }
#endif /* TRACEGC */
	    obj_setslot(bag[i].ref, 0, 0);
	    freed++;
	    /* This is a weak check as it doesn't require contiguous memory */
	    if (freemem >= free_space_goal) {
		break;
	    }
	}
    }

    *totalrefs = nsoftrefs;
    return freed;		/* Return the number of refs freed */
}

/*
 * Expand the mark bits array.  We don't care whether the expansion that
 * led to this is of the handle or object space, just that the contiguous
 * memory that the mark bits must span has expanded by the given amount.
 *
 * There is no slack in expanding the mark bits: we have already alloca-
 * ted the additional memory that the increment represents, so we have
 * got to be able to allocate mark bits to cover it.  We don't need to
 * initialize the new mark bits: they're zeroed on the start of each GC.
 *
 * Note: Given that sysCommitMem() should be assumed to actually allocate
 * swap space, it is arguable that we should put off committing memory
 * for the mark bits until GC actually needs it.  But GC is pretty much
 * inevitable in Java, so it's not clear there's any real benefit.
 */
int
expandMarkBits(long incr)
{
    long markincr;
    unsigned int *incrbits;

    markincr = ((incr/(OBJECTGRAIN*BITSPERMARK) + 1) * 2) * sizeof(*markbits);
    if (marksize + markincr > markmax) {
	if (verbosegc) {
	    fprintf(stderr, "<GC: tried to expand mark bits over limit>\n");
	}
	return 0;
    } else {
	incrbits = sysCommitMem((char *)markbits+marksize, markincr, &markincr);
	if (!incrbits) {
	    if (verbosegc) {
		fprintf(stderr, "<GC: expansion of mark bits failed>\n");
	    }
	    return 0;
	}
	sysAssert(incrbits == (unsigned int *)((char *)markbits + marksize));
	marksize += markincr;
	return 1;
    }
}

/*
 * Expand the handle space.  The behavior of the handle space is simpler
 * than that of the object space: handles never move and the space can't
 * be fragmented, but that also means that contracting the handle space
 * is easy to block.
 *
 * If we were quicker to allocate new handles than GC to reclaim handles,
 * initially expanding by a fixed amount would be quick and easy.  But
 * given the attempt to stay small as much as possible, we are instead
 * using a strategy similar to the object store's, where we GC first,
 * and if we have to expand, we preallocate new space that scales with
 * the amount of existing space, subject to a minimum.
 * 
 * Be sure to zero all the new handles: a free handle is recognized by
 * having its obj field zeroed.  Remember that the base of the handle
 * space must be HANDLEGRAIN aligned within the committed memory.
 *
 * If we are going to seriously try to contract the handle space we
 * should instead allocate handles from higher addresses toward lower
 * addresses.
 */

#undef max
#define	max(a, b) 		((a) < (b) ? (b) : (a))

#define MIN_HANDLE_EXPANSION 4*1024

float preallocFactor = 0.25;	/* REMIND: NEEDS TO MOVE! */

int
expandHandleSpace(void)
{
    long incr;

    /*
     * Expand by the maximum of a constant and the amount we need to
     * make FreeObjectCtr-n >= TotalObjectCtr*preallocFactor.
     */
    incr = MIN_HANDLE_EXPANSION;
    incr = max(incr, TotalHandleCtr*preallocFactor - FreeHandleCtr);
    hpool = hpool - incr;
    if (hpool < heapbase) {	/* We might get by with less... */
	if (verbosegc) {
	    fprintf(stderr, "<GC: tried to expand handle space over limit>\n");
	}
	return 0;
    } else {
	hpoolhand = hpool;
	hpool = sysCommitMem(hpool, incr, &incr);
	if (!hpool) {
	    if (verbosegc) {
		fprintf(stderr, "<GC: expansion of handle space failed>\n");
	    }
	    return 0;
	}
	/* Need to be HANDLEGRAIN aligned within committed memory */
	hpool = (unsigned char *)
	    (((int) hpool + HANDLEGRAIN-1) & ~(HANDLEGRAIN-1));
	memset(hpool, 0, hpoolhand - hpool);
	hpoolhand = hpool;
	FreeHandleCtr  += incr;
	TotalHandleCtr += incr;
	if (verbosegc) {
	    fprintf(stderr,
		"<GC: expanded handle space by %d to %d bytes, %d%% free>\n",
		incr, TotalHandleCtr, 100 * FreeHandleCtr / TotalHandleCtr);
	}
	return expandMarkBits(incr);
    }
}


/*
 * Expand the object space.  To go on we need to at least cover the
 * currently failing allocation, meaning we need a contiguous chunk of
 * its size minus the amount of space in the trailing free chunk, if any.
 * However, we normally also want to leave a user-controllable amount
 * of preallocated space free *after* we have satisfied the current
 * allocation.  This preallocated space is calculated as a percentage
 * of the total free space, so given possible fragmentation, that free
 * space does not guarantee large contiguous allocations.  But given
 * that most allocations are not that large, preallocation can generally
 * reduce the number of interruptions due to memory management operations.
 * The user controls the rate of preallocation by varying the value of
 * the preallocFactor variable. 
 *
 * [REMIND: Currently preallocFactor is global.  That is probably right
 * for a single application, but could it lead to applets fighting over
 * its value???]
 *
 * The last_free parameter passed in here is the last free chunk
 * of memory as determined by the most recent heap compaction in
 * *this* round of memory management.  If the last chunk isn't known,
 * last_free is 0.  There is no guarantee that the last chunk abuts
 * the end of the object space, but if it does, any new memory allocated
 * here is coalesced with the last chunk.
 *
 * Because we are expanding at the top of the space, we leave opoolhand
 * where it is.
 */
int
expandObjectSpace(long n, unsigned char *last_free)
{
    long incr = 0;
    unsigned char *incrptr;

    /*
     * Expand by the maximum of the minimum we need to satisfy the current
     * allocation (taking into account the last_free chunk) and the amount
     * we need such that FreeObjectCtr-n >= TotalObjectCtr*preallocFactor.
     */
    if (last_free) {
	if (n > obj_len(last_free)) {
	    /* May be expanding to satisfy an allocation, not preallocate */
	    incr = n - obj_len(last_free);
	} else {
	    /* We must be expanding to preallocate */
	    incr = 0;
	}
    } else {
	incr = n;
    }
    incr = max(incr, TotalObjectCtr*preallocFactor - (FreeObjectCtr-n));
    /* Always scale incr up to a multiple of OBJECTGRAIN */
    incr = (incr + OBJECTGRAIN - 1) & ~(OBJECTGRAIN - 1);

    if (opoollimit + incr > heaptop) {
	if (verbosegc) {
	    fprintf(stderr, "<GC: tried to expand object space over limit>\n");
	}
	return 0;
    } else {
	/* We could go on with anything down to n - obj_len(last_free) */
	incrptr = sysCommitMem(opoollimit + sizeof(hdr), incr, &incr);	
	if (!incrptr) {
	    if (verbosegc) {
		fprintf(stderr, "<GC: expansion of object space failed>\n");
	    }
	    return 0;
	}
	sysAssert(incrptr == opoollimit + sizeof(hdr));
	if (last_free && last_free + obj_len(last_free) == opoollimit) {
	    /*
	     * The last free chunk abuts the end of the object space, so
	     * we can coalesce it with the new memory.
	     */
	    obj_bumplen(last_free, incr);
#ifdef DEBUG
	    ((long *) last_free)[1] = 0x55555555;
#endif /* DEBUG */
	} else {
	    /*
	     * The last free chunk either doesn't abut the end of the
	     * object space or we don't know where it is, so we don't
	     * try to coalesce the new space.
	     */
	    obj_setlf(opoollimit, incr, 1);
#ifdef DEBUG
	    ((long *) opoollimit)[1] = 0x55555555;
#endif /* DEBUG */
	}
	opoollimit = opoollimit + incr;
	obj_setlf(opoollimit, 0, 0);
	FreeObjectCtr  += incr;
	TotalObjectCtr += incr;
	if (verbosegc) {
	    fprintf(stderr,
	    	"<GC: expanded object space by %d to %d bytes, %d%% free>\n",
	    	incr, TotalObjectCtr, 100 * FreeObjectCtr / TotalObjectCtr);
	}
	return expandMarkBits(incr);
    }
}

/*
 * Deal with an out-of-memory situation caused by an overflow within
 * the Java heap, either of the handle area or the object area.  This
 * thing is called with the heap lock, so doesn't have to worry about
 * someone else coming along and allocating or causing another GC.
 */
int
manageAllocFailure(long n, int overflow_type, int overflow_act) {
    int freeObject;
    int freeHandle;
    int nsoftrefs = 0;
    int ninbag = 0;
    unsigned char *last_free = 0;

    sysAssert(HEAP_LOCKED());

    /*
     * As a special case, if the preallocFactor >= 1.0, then there is
     * no way that GC will ever satisfy the preallocation criterion.
     * Take this as a signal to turn off GC entirely and continue by
     * expansion alone.  REMIND: Should we have a distinct switch to
     * turn off GC?
     */
    if (preallocFactor >= 1.0) {
	overflow_act = OVERFLOW_ACT_EXPAND;
    }

    while (1) {

	/*
	 * Because of fragmentation, it isn't good enough only to know 
	 * how much free space is available.  Before we go try to allocate
	 * again, we want to know whether the last memory management
	 * operation we did made new progress.  So each time around the
	 * while() we reset the progress indicators.
	 */
	if (overflow_type == OVERFLOW_OBJECTS) {
	    freeObject = FreeObjectCtr;
	} else {
	    freeHandle = FreeHandleCtr;
	}

        switch (overflow_act) {

	case OVERFLOW_ACT_GC:
	    /*
	     * Try normal garbage collection; otherwise try finalizing.
	     */
	    last_free = gc0(0, n);
	    /*
	     * I'm not sure it's really worth finalizing and freeing
	     * refs for a handle overflow, as those are not good sources
	     * of many handles.  The alternative is going straight to
	     * OVERFLOW_ACT_EXPAND if overflow_type == OVERFLOW_HANDLES.
	     */
	    overflow_act = OVERFLOW_ACT_FINALIZE;
	    break;

	case OVERFLOW_ACT_FINALIZE:
	    /*
	     * Try finalizing then GCing again; next free refs
	     *
	     * If there are objects on the FinalizeMeQ they are known to
	     * be garbage that can't be freed until they are finalized.
	     * Synchronously finalize them, then garbage collect again.
	     */
	    overflow_act = OVERFLOW_ACT_REFS;
	    if (FinalizeMeQ) {
		if (verbosegc) {
		    int count = 0;
		    finalizer_t *finalizer = FinalizeMeQ;
		    while (finalizer) {
			count++;
			finalizer = finalizer->next;
		    }
		    fprintf(stderr,
			    "<GC: synchronously running %d finalizers>\n",
			    count);
		}
		runFinalization();
		last_free = gc0(0, n);
	        break;
	    } else {
		continue;	/* Don't bother checking for progress */
	    }

	case OVERFLOW_ACT_REFS:	    
	    /*
	     * If there are any refs defined, free some and GC again;
	     * keep trying this so long as there remain refs to free,
	     * and otherwise expand.
	     */
	    if (ninbag = clearRefPointers(0, TotalObjectCtr, &nsoftrefs)) {
		if (verbosegc) {
		    fprintf(stderr, "<GC: zeroed %d of %d soft refs>\n",
			    ninbag, nsoftrefs);
		}
		last_free = gc0(0, n);
		overflow_act = OVERFLOW_ACT_REFS;
		break;
	    } else {
		overflow_act = OVERFLOW_ACT_EXPAND;
		continue;	/* Don't bother checking for progress */
	    }

	case OVERFLOW_ACT_EXPAND:
	    /*
	     * Expand the overflowing area: Successful expansion should
	     * guarantee that the allocation will succeed, and should not
	     * over(pre)allocate.
	     */
	    overflow_act = OVERFLOW_ACT_DESPERATE;
	    if (overflow_type == OVERFLOW_HANDLES) {
		if (expandHandleSpace()) {
		    /* We got *some* memory */
		    return overflow_act;
		}
	    } else if (overflow_type == OVERFLOW_OBJECTS) {
		if (expandObjectSpace(n, last_free)) {
		    /* We got *some* memory */
		    return overflow_act;
		}
	    }
	    break;

	case OVERFLOW_ACT_DESPERATE:
	    if (verbosegc) {
		fprintf(stderr, "<GC: totally out of heap space>\n");
	    }
	    /* Failed to make any progress: really out of heap */
    	    return OVERFLOW_ACT_FAIL;

	default:
	    return OVERFLOW_ACT_FAIL;
        }

	/*
	 * Test whether the last memory management operation made
	 * sufficient progress to try allocating again.  Otherwise,
	 * go directly to trying the next operation.  Subtlety: because
	 * of fragmentation, apparently adequate free space is not always
	 * enough, so we also require evidence of recent progress.  Even
	 * so, there's still no guarantee allocation will go through.
	 * Note that we subtract off the current allocation amount: the
	 * preallocated memory is the amount after this allocation.
	 */
	if (overflow_type == OVERFLOW_OBJECTS) {
	    if (FreeObjectCtr > freeObject &&
		FreeObjectCtr-n >= TotalObjectCtr*preallocFactor) {
        	return overflow_act;	/* Try to allocate again */
	    }   /* else go back and switch on new overflow_act */
	} else {
	    if (FreeHandleCtr > freeHandle &&
		FreeHandleCtr-sizeof(JHandle) >= TotalHandleCtr*preallocFactor) {
		return overflow_act;	/* Try to allocate again */
	    }   /* else go back and switch on new overflow_act */
        }
    }
}

void
markChildren(register JHandle * h, register JHandle * limit)
{
    register ClassClass *cb;
    register JHandle *sub, *needsmark;
    register ClassObject *p;
    SetLimits();
ScanNext:
    needsmark = 0;
    p = unhand(h);
#ifdef TRACEGC
    if (tracegc > 1) {
	fprintf(stderr, "<%X/%X>\n", h, p);
	fprintf(stderr, " <  %s\n", Object2CString(h));
    }
#endif /* TRACEGC */
    switch (obj_flags(h)) {
	case T_NORMAL_OBJECT:
	    cb = obj_classblock(h);
	    do {
		register long n = cb->fields_count;
		register struct fieldblock *fb = cbFields(cb);
		while (--n >= 0) {
		    if ((fieldIsArray(fb) || fieldIsClass(fb))
			&& !(fb->access & ACC_STATIC)) {
			sub = *(JHandle **) ((char *) p + fb->u.offset);
			if (ValidHandle(sub) && !IsMarked(sub)) {
			    MarkPtr(sub, CCIs(cb, SoftRef)
				    && fb->u.offset == 0
				    ? SoftMark : HardMark);
#ifdef TRACEGC
			    if (tracegc > 1)  {
				fprintf(stderr, " mo %s\n", Object2CString(sub));
			    }
#endif /* TRACEGC */
			    if (sub < limit) {
				if (needsmark) {
				    markChildren(needsmark, limit);
				}
				needsmark = sub;
			    }
			}
		    }
		    fb++;
		}
		if (cbSuperclass(cb) == 0) {
		    break;
		}
		cb = unhand(cbSuperclass(cb));
	    } while (cb);
	    break;

	case T_CLASS: {     /* an array of classes */
	    register long n = obj_length(h);
	    while (--n >= 0) {
		sub = ((ArrayOfObject *) p)->body[n];
		if (ValidHandle(sub) && !IsMarked(sub)) {
		    MarkPtr(sub, HardMark);
#ifdef TRACEGC
		    if (tracegc > 1) {
			fprintf(stderr, " ma %s\n", Object2CString(sub));
		    }
#endif /* TRACEGC */
		    if (sub < limit) {
			if (needsmark) {
			    markChildren(needsmark, limit);
			}
			needsmark = sub;
		    }
		}
	    }
	}
    }
    if (h = needsmark) {
	goto ScanNext;          /* tail recursion */
    }
#ifdef TRACEGC
    if (tracegc > 1) {
	fprintf(stderr, " >  %s\n", Object2CString(h));
    }
#endif /* TRACEGC */
}


static int
scanThread(sys_thread_t *t, void *arg)
{
    extern JHandle *getThreadNext();
    JHandle *me = (JHandle *) arg;
    JHandle *p = (JHandle *) sysThreadGetBackPtr(t);
    unsigned char **ssc, **limit;
    stackp_t base;
    SetLimits();

#ifdef DEBUG
    if (!ValidHandle(p)) {
	sysAbort();
    }
#endif /* DEBUG */
#ifdef TRACEGC
    if (tracegc > 1 && !IsMarked(p)) {
	if (((JHandle *)p)->obj && ((JHandle *)p)->methods) {
	    fprintf(stderr, " ms %s\n", Object2CString((JHandle *)p));
	} else {
	    fprintf(stderr, " ms %8X\n", p);
	}
    }
#endif /* TRACEGC */
    /* Mark thread object */
    MarkPtr(p, HardMark);

    base = threadStackBase((TID)p);
    ssc  = threadStackPointer((TID)p);

    if (p == me) {
	ssc = (unsigned char **) &me;
    }

    if (ssc == 0 || base == 0 || (ssc == base)) {
	/*
	 * If the stack does not have a top of stack pointer or a base
	 * pointer then it hasn't run yet and we don't need to scan 
	 * its stack.  When exactly each of these data becomes available
	 * may be system-dependent, but we need both to bother scanning.
	 */
	return SYS_OK;
    }

    limit = (unsigned char **) base;

#ifdef TRACEGC
    if (tracegc)  {
	fprintf(stderr, "%8X [%10X,%10X] %d\n", p, ssc, limit, limit - ssc);
    }
#endif /* TRACEGC */
    sysAssert(ssc != limit);

    while (ssc < limit) {
	register unsigned char *ptr = *ssc;
	if (ValidHorO(ptr)) {
#ifdef TRACEGC
	    if (tracegc > 1 && !IsMarked(ptr) && ValidHandle(ptr)) {
		if (((JHandle *)ptr)->obj && ((JHandle *)ptr)->methods) {
		    fprintf(stderr, " ms %s\n", Object2CString((JHandle *) ptr));
		} else {
		    fprintf(stderr, " ms %8X\n", ptr);
		}
	    }
#endif /* TRACEGC */
	    MarkPtr(ptr, HardMark);
	}
	ssc++;
    }

    {
	ExecEnv *ee = (ExecEnv *)THREAD(p)->eetop;
	JavaFrame *frame;
	register unsigned char *ptr;
	if ((ee != 0) && ((frame = ee->current_frame) != 0)) {
	    stack_item *top_top_stack = frame->current_method 
		  ? &frame->ostack[frame->current_method->maxstack] 
		  : frame->optop;
	    limit = (unsigned char **)top_top_stack;
	    for(;;) {
#ifdef TRACEGC
		if (tracegc) {
		    fprintf(stderr, " scanning frame %8X\n", frame);
		}
#endif /* TRACEGC */

		for ( ssc= (unsigned char **)(frame->ostack); 
		       ssc < limit; ssc++) {
		    ptr = *ssc;
		    if (ValidHorO(ptr)) {
#ifdef TRACEGC
			if (tracegc > 1 && !IsMarked(ptr) && ValidHandle(ptr)) {
			    fprintf(stderr, " ms %8X\n", ptr);
			}
#endif /* TRACEGC */
			MarkPtr(ptr, HardMark);
		    }
		}
		ssc = (unsigned char **)(frame->vars);
		if (ssc) {
		    limit = (unsigned char **)frame;
		    for (; ssc < limit; ssc ++) {
			ptr = *ssc;
			if (ValidHorO(ptr)) {
#ifdef TRACEGC
			    if (tracegc > 1 && !IsMarked(ptr) && ValidHandle(ptr)) {
			    	fprintf(stderr, " ms %8X\n", ptr);
			    }
#endif /* TRACEGC */
			    MarkPtr(ptr, HardMark);
			}
		    }
		}
		frame = frame->prev;
		if (!frame) break;
		limit = (unsigned char **)(frame->optop);

	    }
	} else {
#ifdef TRACEGC 
	    if (tracegc) {
		fprintf(stderr, "ee=%x\n", ee);
		if (ee) {
		    fprintf(stderr, "frame=%x\n", ee->current_frame);
		}
	    }
#endif /* TRACEGC */
	}

    }

    return SYS_OK;
}

static void
scanThreads(void)
{
    JHandle *self;
    int err;

    self = (JHandle *) sysThreadGetBackPtr(sysThreadSelf());
    err = sysThreadEnumerateOver(scanThread, (void *) self);
    /* REMIND - should do something with the return value. */
}


/* Look through the constant pool of each of the classes to see if it
 * contains any objects (like Strings) that need to be marked.
 */
 
static void
scanClasses(void)
{
    int i, j;
    ClassClass **pcb =  binclasses;
    struct fieldblock *fb;
    SetLimits();

    for (j = nbinclasses; --j >= 0; pcb++) {
	ClassClass *cb = *pcb;
        if (!cb) continue;
	if (CCIs(cb, Resolved)) {
	    union cp_item_type *constant_pool = cbConstantPool(cb);
	    union cp_item_type *cpp = constant_pool+ CONSTANT_POOL_UNUSED_INDEX;
	    union cp_item_type *end_cpp = &constant_pool[cb->constantpool_count];
	    for ( ; cpp < end_cpp; cpp++) {
		void *ptr = (*cpp).p;
		if (ValidHorO(ptr)) {
#ifdef TRACEGC
		    if (tracegc > 1 && !IsMarked(ptr) && ValidHandle(ptr)) {
		        if (((JHandle *)ptr)->obj && ((JHandle *)ptr)->methods) {
			    fprintf(stderr, " ms %s\n", 
				    Object2CString((JHandle *) ptr));
			} else {
			    fprintf(stderr, " ms %8X\n", ptr);
			}
		    }
#endif /* TRACEGC */
		    MarkPtr(ptr, HardMark);
		}
	    }
	}
	/* Scan class definitions looking for statics */
	for (i = cb->fields_count, fb = cbFields(cb); --i >= 0; fb++) {
	    if ((fieldIsArray(fb) || fieldIsClass(fb)) && (fb->access & ACC_STATIC)) {
		JHandle *sub = *(JHandle **)normal_static_address(fb);
		if (ValidHandle(sub) && !IsMarked(sub)) {
		    MarkPtr(sub, HardMark);
#ifdef TRACEGC
		    if (tracegc > 1) {
			fprintf(stderr, " mc %s\n", Object2CString(sub));
		    }
#endif /* TRACEGC */
		}
	    }
	}
	/* don't have to mark cb->superclass, because it's caught by
	   the HandleToSelf for that class */
	if (ValidHandle(cbHandle(cb))) {
	    MarkPtr((JHandle *) cbHandle(cb), HardMark);
	}
	if (ValidHandle(cbClassnameArray(cb))) {
	    MarkPtr((JHandle *) cbClassnameArray(cb), HardMark);
	}
	if (ValidHandle(cbLoader(cb))) {
	    MarkPtr((JHandle *) cbLoader(cb), HardMark);
	}
    }
}

/*
 * Scan the heap and chase pointers.  Scanning from pointers into the
 * middle of objects is normally disabled, apparently because it was
 * slow.  This should be revisited, especially if it reduces pinning.
 */
static void
scanHeap(void)
{
    SetLimits();
    unsigned char *o, *oo, *next;
    int i, size;

#ifdef TRACEGC
    if (tracegc) {
        fprintf(stderr, "\nRecursive mark scan:\n");
    }
#endif /* TRACEGC */

#ifdef CHECK_INTERIOR_POINTERS
    /*
     * Search the entire memory pool looking for references into the
     * the middle of objects.  If any are found, mark the object they
     * are part of.  Pointers into the middle of objects can happen
     * frequently, especially in the case of arrays.
     *
     * This piece of code is tricky because it needs to be fast.
     * It is basically walking through most of the mark bits looking
     * for set bits.  It does it in 32-bit increments for speed.
     */
    o = oo = opmin;
    next = o + obj_len(o);
    size = ((int) (opoollimit - opmin) / (OBJECTGRAIN * BITSPERMARK) + 1) * 2;
    for (i=0; i < size; i++) {
        sysAssert(((unsigned int)(oo - opmin) >> 7) == i);
        if (markbits[i]) {
	    unsigned int bits = markbits[i];
	    unsigned int shifts = OBJECTGRAIN * BITSPERMARK / 2;
	    while (bits && oo < opmax) {
		if (bits & 0x3) {
		    /* Mark the object, not the header */
		    MarkPtr((o + sizeof(hdr)), HardMark);
		}
		bits >>= 2;
		shifts -= OBJECTGRAIN;
		oo += OBJECTGRAIN;
		if (oo == next) {
		    o = next;
		    next = o + obj_len(o);
		}
	    }
	    oo += shifts;
	} else {
	    oo += OBJECTGRAIN * BITSPERMARK / 2;
	}

	if (oo >= opmax) {
	    break;
	}

	while (oo >= next) {
	    o = next;
	    next = o + obj_len(o);
	}
    }
#endif /* CHECK_INTERIOR_POINTERS */
}

/*
 * Walk the entire handle pool, and do a recursive search for
 * references.
 */
static void
scanHandles(void)
{
    SetLimits();
    JHandle *hp;

    hp = (JHandle *) hpool;
    while ((unsigned char *) hp <= hpmax) {
        if (hp->obj) {
	    if (ValidObject(hp->obj) && IsMarked(hp->obj)) {
		MarkPtr((JHandle *) hp, HardMark);
	    }
	    if (IsMarked(hp)) {
		markChildren(hp, hp + 1);
	    }
        }
        hp++;
    }
}

/*
 * Take care of finalization
 *
 * At this point we have marked all reachable objects.  Anything
 * not marked and having a finalization method will be queued for
 * finalization.  However, it is not the case that everything
 * else that's unmarked can just be freed.  An object to be
 * finalized expects everything it references to be retained by
 * GC, whether or not those things are themselves finalized.  So
 * we make a pass over the HasFinalizerQ that moves freed objects
 * needing finalization to the FinalizeMeQ.  We then make a pass
 * over the FinalizeMeQ recursively marking everything on it.  It
 * is important that there is one pass over each queue: doing it
 * in a single pass over the HasFinalizerQ causes different
 * behavior as finalizable objects referenced by earlier finali-
 * zable objects will be marked and not be themselves finalized.
 * We could potentially allow interruption between the two passes.
 *
 * After this pass we are in a consistent state (only objects that
 * are unreachable have been put on the FinalizeMeQ), so we can
 * allow async GC to be interrupted.  Note that this means that
 * an object may be finalized earlier than its non-finalized peers
 * are freed.  Note that the FinalizeMeQ scan also marks objects that
 * were on the FinalizeMeQ when we started, which we needed to
 * do anyhow.  Removing an object from the HasFinalizerQ ensures
 * that we will never try to finalize it again.
 */
static void
prepareFinalization(void)
{    
    if (HasFinalizerQ) {
        SetLimits();
        JHandle *hp;
        finalizer_t *prev = (finalizer_t *) &HasFinalizerQ;
        finalizer_t *final;

        /* Pass over HasFinalizerQ: */
        while (final = prev->next) {
	    hp = final->handle;
	    if (hp->obj && !IsMarked(hp) && ValidObject(hp->obj)) {
		/* Important: Put in FinalizeMeQ in reverse order */
		prev->next = final->next;
		final->next = FinalizeMeQ;
		FinalizeMeQ = final;
	    } else { 	/* still in use, so leave alone */
		prev = final;
	    }
	}

	/* Trace the FinalizeMeQ: */
	final = FinalizeMeQ;
	while (final) {
	    hp = final->handle;
	    /* Mark the handle itself */
	    MarkPtr((JHandle *) hp, HardMark);
	    /* Recursively mark all objects referencable via the handle */
	    markChildren(hp, (JHandle *) hpmax);
	    final = final->next;
	}

	/* Trace the BeingFinalized object (see comments in finalize.c): */
	final = BeingFinalized;
	if (final) {
	    hp = final->handle;
	    MarkPtr((JHandle *) hp, HardMark);
	    markChildren(hp, (JHandle *) hpmax);
	}
    }
}

/*
 * Scan for remaining unmarked objects and free them.  All objects
 * that are to be finalized have been marked.
 */ 
static long nfreed;		/* For -verbosegc */
static long bytesfreed;		/* For -verbosegc */

static int
freeSweep(unsigned int free_space_goal)
{
    SetLimits();
    JHandle *hp;
    int do_compact_heap = 1;

#ifdef TRACEGC
    if (tracegc) {
        fprintf(stderr, "\nfree scan:\n");
    }
#endif /* TRACEGC */
    hp = (JHandle *) hpool;
    while ((unsigned char *) hp <= hpmax) {
        if (hp->obj && !IsMarked(hp) && ValidObject(hp->obj)) {
#ifdef TRACEGC
	    if (tracegc) {
		fprintf(stderr, " fr %s (%8X/%8X)\n",
			Object2CString(hp), hp, hp->obj);
	    }
#endif /* TRACEGC */
#ifdef DEBUG
	    memset((char *) hp->obj, 0x55,
		obj_len((char *) hp->obj - sizeof(hdr)) - sizeof(hdr));
#endif /* DEBUG */
	    FreeObjectCtr += obj_len((char *) hp->obj - sizeof(hdr));
	    sysAssert(FreeObjectCtr <= TotalObjectCtr);
	    if (verbosegc) {
	        nfreed++;
	        bytesfreed += obj_len((char *) hp->obj - sizeof(hdr));
	    }
	    if (obj_len((char *)hp->obj - sizeof(hdr)) >= free_space_goal) {
	        do_compact_heap = 0;
	    }
	    obj_setfree((char *) hp->obj - sizeof(hdr));
	    hp->obj = 0;
#ifdef DEBUG
	    hp->methods = 0;    /* paranoia */
#endif /* DEBUG */
	    FreeHandleCtr += sizeof(JHandle);
	    sysAssert(FreeHandleCtr <= TotalHandleCtr);
	}
	hp++;
    }

    /* Reset allocation hands, so that freed memory gets used */
    opoolhand = opmin;
    hpoolhand = hpool;

    return do_compact_heap;
}

/*
 * Concept: Assuming optimal packing, there are only two places that
 * holes, or bubbles in the heap can occur.  These are immediately before
 * each pinned range of memory, and at the tail of the heap (the last
 * range is always free).
 *
 * This algorithm walks the heap from beginning to end, performing
 * several operations:
 *  1. Coalesce adjacent free blocks.
 *  2. Record bubbles in the free_blocks[] array.  This is an array
 *     containing the biggest free blocks found.
 *  3. Copy unpinned, allocated blocks down into the lowest free
 *     block available.
 *
 * The goal of this scheme is to leave as many large free blocks available
 * as is possible.
 */
static int 
compactHeap(int async_call, unsigned char **last_freep)
{
    SetLimits();
    register JHandle *hp;
    unsigned char *p, *next, *prev;
#define NUMFREEBLOCKS 16
    unsigned char *free_blocks[NUMFREEBLOCKS];
    unsigned int plen;
    int blocks_moved = 0;
    unsigned char *last_free = 0;

    p = opmin;
    prev = 0;
    memset((char *)free_blocks, 0, sizeof(free_blocks));

    /*
     * Put a pointer to each object's handle in the first word of the object,
     * saving the first word of the object in the object pointer of the
     * handle. This way we can compact the heap looking only at the heap,
     * then go back and fix up the handles.  (we need to scan all objects in
     * heap order, not handle order to effectively compact everything).
     */
#define obj_swapped(p)      (obj_geth(p) & 2)
#define obj_setswapped(p)   (obj_geth(p) |= 2)
#define obj_clearswapped(p) (obj_geth(p) &= ~2)

#define handle_swapped(hp)      (IsMarked(hp) & 0x1)
#define handle_setswapped(hp)   (MarkPtr(hp, 0x1))
#define handle_clearswapped(hp) (ClearMarkPtr(hp, 0x1))

    for (hp = (JHandle *) hpool; hp <= (JHandle *) hpmax; hp++) {
	if (ValidObject(hp->obj)) {
	    OBJECT T = *hp->obj;
	    unsigned char *p = (unsigned char *)(hp->obj) - sizeof(hdr);
	    *hp->obj = (OBJECT) hp;
	    hp->obj = (OBJECT *) T;
	    if (obj_swapped(p)) {
		handle_setswapped(hp);
	    } else {
		obj_setswapped(p);
		handle_clearswapped(hp);
	    }
	}
    }

    /*
     * This is the reverse of the above swap.  It is called once
     * on each allocated block and fixes the handle/pointer swap.
     */
#define REVERSE_SWAP(p) { \
    JHandle *hp, *nexthp; \
    sysAssert(obj_swapped(p)); \
    obj_clearswapped(p); \
    hp = *(JHandle **) (p + sizeof(hdr)); \
    while (handle_swapped(hp)) { \
	nexthp = (JHandle *) (hp->obj); \
	hp->obj = (OBJECT *) (p + sizeof(hdr)); \
	hp = nexthp; \
    } \
    *(OBJECT *) (p + sizeof(hdr)) = (OBJECT) hp->obj; \
    hp->obj = (OBJECT *) (p + sizeof(hdr)); \
    handle_setswapped(hp); \
}

    /*
     * The main loop - look at every object in the pool
     */
    while (p < opmax) {
	plen = obj_len(p);
	next = p + plen;

	if (async_call && INTERRUPTS_PENDING()) {
	    blocks_moved = 0;
	    /*
	     * Cycle through the remaining blocks as quickly as possible,
	     * reversing all of the handle pointers
	     */
	    while (p < opmax) {
		if (!obj_free(p)) {
		    REVERSE_SWAP(p);
		}
		p += obj_len(p);
	    }
	    last_free = 0;
	    break;
	}

	if (obj_free(p)) {
	    unsigned int i, smallest = 0, len = (unsigned) ~0;

	    /* 1. Coalesce adjacent free blocks */

	    while (next && obj_free(next)) {
		plen += obj_len(next);
		next += obj_len(next);
		blocks_moved++;
	    }
	    obj_setlf(p, plen, 1);

	    /*
	     * Record the most recently seen free block.  When compaction
	     * finishes this will be the highest free block.  It is very
	     * likely that it will span to the end of the object space,
	     * giving us the option of freeing some of it.
	     */
	    last_free = p;

	    /* 2. Record the fact that p is a free block */

	    /*
	     * Search for the smallest sized free range, and if it
	     * is smaller than the size of p, replace it with p.
	     * There may be empty slots in the buffer. Use them
	     * if possible.
	     */
	    for (i = 0; i < NUMFREEBLOCKS; i++) {
		sysAssert(free_blocks[i] == 0 || obj_free(free_blocks[i]));
		if (free_blocks[i] == 0 || free_blocks[i] == p) {
		    len = 0;
		    smallest = i;
		    break;
		} else if (obj_len(free_blocks[i]) < len) {
		    smallest = i;
		    len = obj_len(free_blocks[i]);
		}
	    }

	    if (len < obj_len(p)) {
		free_blocks[smallest] = p;
	    }

	    prev = p;

	} else if (!IsMarked(p + sizeof(hdr))) {
	    int i, lowest, prev_block = -1;
	    unsigned int len, lowest_len;
	    unsigned char *lowest_addr;

	    /* 3. Copy p down into the lowest available free block */

	    /*
	     * Find lowest block that is big enough to hold the block.
	     * Ignore 'prev' as we will handle it with a simpler case.
	     */
	    lowest = -1;
	    lowest_len = 0;
	    lowest_addr = opmax;
	    for (i = 0; i < NUMFREEBLOCKS; i++) {
		if (!free_blocks[i]) {
		    continue;
		}
		if (free_blocks[i] == prev) {
		    prev_block = i;
		}
		len = obj_len(free_blocks[i]);
		if (len > plen && lowest_addr > free_blocks[i]) {
		    lowest = i;
		    lowest_addr = free_blocks[i];
		    lowest_len = len;
		}
	    }

	    /* did we find a slot? */
	    if (lowest >= 0 && lowest != prev_block) {
		unsigned char *oldp = p;
		/*
		 * If a slot is found, move the block, and mark the old
		 * slot as free.  If the free space was bigger than we needed,
		 * break off a new chunk.
		 */
		sysAssert(obj_free(free_blocks[lowest]));
		memmove(free_blocks[lowest], p, plen);
		blocks_moved++;
		obj_setfree(p);
#ifdef DEBUG
		((long *) p)[1] = 0x55555555;
#endif /* DEBUG */
		p = free_blocks[lowest];

		/* it was bigger than we needed, break off a piece. */
		if (lowest_len > plen) {
		    unsigned char *fp = free_blocks[lowest];
		    fp += plen;
		    obj_setlf(fp, (lowest_len - plen), 1);
#ifdef DEBUG
		    ((long *) fp)[1] = 0x55555555;
#endif /* DEBUG */
		    if (prev <= free_blocks[lowest]) {
			free_blocks[lowest] = 0;
		    } else {
			free_blocks[lowest] = fp;
		    }
		} else {
		    free_blocks[lowest] = 0;
		}

		if (obj_free(prev)) {
		    next = prev;
		    prev = 0;
		} else {
		    next = oldp;
		}
	    } else {
		/*
		 * no free slot found or it is the previous slot.  See if prev
		 * is free, and if so, just slide this range back.
		 */
		if (prev && obj_free(prev)) {
		    memmove(prev, p, plen);
		    blocks_moved++;
		    next = prev + plen;
		    obj_setlf(next, (p - prev), 1);
#ifdef DEBUG
		    ((long *) next)[1] = 0x55555555;
#endif /* DEBUG */
		    p = prev;
		    if (prev_block >= 0) {
			free_blocks[prev_block] = 0;
		    }

		} else {
		    prev = p;
		}
	    }
	} else {
	    prev = p;
	}

	if (next > p && !obj_free(p)) {
	    REVERSE_SWAP(p);
	}
	p = next;
    }

#ifdef TRACEGC
    if (tracegc && blocks_moved) {
	fprintf(stderr, "<GC: compacted %d objects>\n", blocks_moved);
    }
#endif /* TRACEGC */

    sysAssert(last_free == 0 || obj_free(last_free));
    *last_freep = last_free;

    return async_call ? blocks_moved : 0;
}


/*
 * Garbage collect memory.  If 'async_call' is true, this function is being
 * called from the asych GC thread, and should immediately exit whenever
 * the INTERRUPTS_PENDING() macro returns true.  The 'free_space_goal' is
 * the amount of memory in a single block which the caller is interested
 * in having free.  If such a block does not exist, compact the heap.
 *
 * gc0_locked() should only be called when holding the heap lock and the
 * finalization queue locks.  gc() or realObjAlloc() acquires the heap
 * lock, then calls gc0() which grabs the finalization locks and calls
 * gc_locked(), which does system-specific tasks then calls gc0_locked().
 */
unsigned char *
gc0_locked(int async_call, unsigned int free_space_goal)
{
    /* SPARC only: "ta ST_FLUSH_WINDOWS" done by "gc" stub in machgc.s */
    int do_compact_heap;
    long start_time = now();
    unsigned char *last_free = 0;

    sysAssert(HEAP_LOCKED());
    sysAssert(HASFINALQ_LOCKED());
    sysAssert(FINALMEQ_LOCKED());
    sysAssert(QUEUE_LOCKED());

    if (verbosegc) {
	nfreed = 0;
	bytesfreed = 0;
    }

    memset((char *) markbits, 0, marksize);
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;
    }

    MARK(GCTHREAD);
    scanThreads();
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;
    }

    MARK(GCCLASS);
    scanClasses();
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;
    }

    MARK(GCHEAP);
    scanHeap();
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;
    }

    MARK(GCHANDLES);
    scanHandles();
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;
    }

    MARK(GCFINAL);
    prepareFinalization();
    if (async_call && INTERRUPTS_PENDING()) {
       goto unlock;
    }

    MARK(GCSWEEP);
    do_compact_heap = freeSweep(free_space_goal);
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;
    }

    MARK(COMPACT);
    if (do_compact_heap) {
	while (compactHeap(async_call, &last_free)) ;
    }
    if (async_call && INTERRUPTS_PENDING()) {
	goto unlock;	/* I.e. don't print <GC(async):... message */
    }

    if (verbosegc) {
	long total_time = now() - start_time;
	fprintf(stderr,
		"<GC%s: freed %d objects, %d bytes in %d msec, %d%% free>\n",
	        async_call ? "(async)" : "",
	        nfreed, bytesfreed, total_time,
	        100 * FreeObjectCtr / TotalObjectCtr);
    }

  unlock:
    if (java_monitor && !async_call) {
	ExecEnv *ee = EE();
	if (ee && ee->current_frame && ee->current_frame->current_method) {
	    java_mon(ee->current_frame->current_method,
		    (struct methodblock *)-1, now() - start_time);
	}
    }


    return last_free;

}

/*
 * Interface to synchronous garbage collection from other memory manage-
 * ment code.  This is called call when memory allocation fails.  We
 * expect to have the heap lock when it is called. 
 */

unsigned char *gc_locked(int, unsigned int);	/* Call holding locks! */

unsigned char *
gc0(int async_call, unsigned int free_space_goal)
{
    unsigned char *last_free = 0;

    sysAssert(HEAP_LOCKED());
    HASFINALQ_LOCK();
    FINALMEQ_LOCK();
    QUEUE_LOCK();

    if (sysThreadSingle() == SYS_OK) {
	last_free = gc_locked(async_call, free_space_goal);
    }
    sysThreadMulti();

    /* Notify the finalizer thread if objects pending finalization */
    if (FinalizeMeQ) {
	FINALMEQ_NOTIFY();
    }

    heap_memory_changes = 0;

    QUEUE_UNLOCK();
    FINALMEQ_UNLOCK();
    HASFINALQ_UNLOCK();

    return last_free;
}
    
/*
 * User interface to synchronous garbage collection.  This is called
 * by an explicit call to GC.  Asynchronous GC has its own entry point.
 * At the call we do not already have the heap lock, so grab it.
 */
void
gc(int async_call, unsigned int free_space_goal)
{
    HEAP_LOCK();
    (void) gc0(async_call, free_space_goal);
    HEAP_UNLOCK();
}

/*
 * Called whenever the system thinks it is idle, and might want to
 * do a GC.  This might also want to grow the heap spaces to maintain
 * the desired free space.
 */
void
asyncGC()
{
    int freemem = FreeObjectCtr;
    int totalrefs, freedrefs;

    HEAP_LOCK();
    HASFINALQ_LOCK();
    FINALMEQ_LOCK();
    QUEUE_LOCK();

    if (sysThreadSingle() != SYS_OK) {
	goto unlock;
    }
    /*
     * We currently don't try to expand asynchronously, so ignore the
     * return value (last_free) of gc_locked().
     */
    (void) gc_locked(1, ~0);
    if (INTERRUPTS_PENDING()) {
	goto unlock;
    }

    /* Don't want to zero this unless gc actually completed... */
    heap_memory_changes = 0;

    /*
     * If we didn't free anything, and we hit the low water mark,
     * try clearing some soft refs.
     */
    if ((freemem == FreeObjectCtr) && (freemem < FreeMemoryLowWaterMark)) {
	/* If we can't clear any, just return */
	if (!(freedrefs =
		clearRefPointers(1, FreeMemoryLowWaterMark, &totalrefs))) {
	    goto unlock;
	}
	if (verbosegc) {
	    fprintf(stderr, "<GC(async): Asynchronously zeroed %d of %d soft refs>\n",
		freedrefs, totalrefs);
	}
	if (INTERRUPTS_PENDING()) {
	    goto unlock;
	}
	(void) gc_locked(1, ~0);
    }

unlock:
    sysThreadMulti();

    /* Notify the finalizer thread if objects pending finalization */
    if (FinalizeMeQ) {
	FINALMEQ_NOTIFY();
    }

    QUEUE_UNLOCK();
    FINALMEQ_UNLOCK();
    HASFINALQ_UNLOCK();
    HEAP_UNLOCK();

    return;
}


/*
 * Initialize structures supporting GC: allocate and zero the mark bits.
 * [Should we be allocating the mark bits at GC time instead?]
 */
void
InitializeGC(int max, int min)
{
    /*
     * Map the mark bits array, whose size is defined by the maximum
     * (mapped) memory, and commit to enough memory to map the committed
     * heap.
     */
    markmax = ((max/(OBJECTGRAIN*BITSPERMARK) + 1) * 2) * sizeof(*markbits);
    markbits = (unsigned int *) sysMapMem(markmax, &markmax);
    if (!markbits) {
	out_of_memory();	/* Can't start up */
    }

    /* Could postpone commit until GC; see comment at expandMarkBits() */
    marksize = ((min/(OBJECTGRAIN*BITSPERMARK) + 1) * 2) * sizeof(*markbits);
    markbits = sysCommitMem(markbits, marksize, &marksize);
    if (!markbits) {
	out_of_memory();	/* Can't start up */
    }
}

/*
 * Initialize the Java heap.  Argument max_request is the maximum heap
 * memory we are prepared to devote to the heap, and min_request is the
 * minimum memory and amount allocated on startup.
 */

#define BACKOFF_FACTOR 0.75

void
InitializeAlloc(long max_request, long min_request)
{
    long max, min;

    /*
     * Sanity check the initial requests: enforce constraint that
     * max_request >= min_request.  For backward compatibility, min_request
     * overrides max_request.  REMIND: this still allows us to try to start
     * up with ridiculous values, but enforcing reasonable values is
     * machine-dependent and must be done elsewhere.
     */
    if (max_request < min_request) {
	max_request = min_request;
    }

    /*
     * Allocate the initial heap memory.  In the abstract we first map
     * the maximum we are interested in, then commit to the minimum/initial
     * amount.  Note that as actually implemented on some platforms,
     * commitment might be a no-op, min might equal max etc.  
     *
     * Because we don't really need more than min_request to start up,
     * we can back off on the heap mapping if it initially fails.  There
     * is no guarantee that we will be allowed to expand to the max anyhow.
     * Related spaces (e.g. mark bits) scale off what we actually get.
     * We should back off in fairly large chunks so if we are successful
     * we'll be able to map the rest of what we'll need.
     */
    sysAssert(hpool == 0);
    heapbase = (unsigned char *) sysMapMem(max_request, &max);
    while (!heapbase) {
	max_request -= (max_request * BACKOFF_FACTOR);
	if (max_request < min_request) {
	    out_of_memory();	/* Can't start up */
	}
	heapbase = (unsigned char *) sysMapMem(max_request, &max);
    }
    heaptop = heapbase + max;

    /*
     * We want same partition proportions (20%/80%) of the mapped 
     * memory as the committed memory.  The handle pool base must be
     * HANDLEGRAIN aligned within the committed memory.
     */
    hpool = heapbase + (int) ((heaptop - heapbase) * 0.20);
    hpool = hpool - (int) (min_request * 0.20);
    hpool = sysCommitMem(hpool, min_request, &min);
    if (!hpool) {
	out_of_memory();	/* Can't start up */
    }
    hpool = (unsigned char *)
	(((int) hpool + HANDLEGRAIN-1) & ~(HANDLEGRAIN-1));
    hpoollimit = hpool + min;

    /*
     * Allocate the initial GC mark bits array
     */
    InitializeGC(max, min);

    /*
     * Leave the bottom 20% of the pool for handles, the rest for handles
     */
    opoollimit = hpoollimit - sizeof(hdr);
    hpoollimit = hpool +
	((int) ((hpoollimit - hpool) * 0.20) & ~(HANDLEGRAIN - 1));

    /*
     * The object pool base must be sizeof(hdr) past OBJECTGRAIN alignment
     */
    opool = hpoollimit;
    while (((int) opool + sizeof(hdr)) & (OBJECTGRAIN - 1)) {
	opool += sizeof(hdr);
    }

    hpoolhand = hpool;

    /*
     * Make sure that all the handles are zero.  We recognize a free handle
     * by the fact that its ->obj field is 0.
     */
    memset(hpoolhand, 0, hpoollimit - hpoolhand);

    obj_setlf(opool, opoollimit - opool, 1);
#ifdef DEBUG
    ((long *) opool)[1] = 0x55555555;
#endif /* DEBUG */
    obj_setlf(opoollimit, 0, 0);
    FreeObjectCtr = opoollimit - opool;
    TotalObjectCtr = FreeObjectCtr;
    FreeHandleCtr = hpoollimit - hpool;
    TotalHandleCtr = FreeHandleCtr;

    /*
     * The free memory low water mark is used when deciding whether to
     * free refs.
     */
    FreeMemoryLowWaterMark = (long) (0.2 * FreeObjectCtr) & ~(OBJECTGRAIN - 1);
    opoolhand = opool;

    _hasfinalq_lock = (sys_mon_t *) sysMalloc(sysMonitorSizeof());
    memset(_hasfinalq_lock, 0, sysMonitorSizeof());
    HASFINALQ_LOCK_INIT();
    _heap_lock = (sys_mon_t *) sysMalloc(sysMonitorSizeof());
    memset((char *) _heap_lock, 0, sysMonitorSizeof());
    HEAP_LOCK_INIT();
}


/*
 * Miscellaneous stuff
 */

/*
 * Support for DumpMonitors(): we want to print whose monitor this is
 * if it is associated with an object.  To check whether that's so we
 * need to see whether the handle is in the handle pool, and the limits
 * of the handle pool aren't visible out of gc.c.
 */ 
void
monitorDumpHelper(monitor_t *mid, void *name)
{
    TID t;
    unsigned int key = mid->key;
    SetLimits();

    if (mid && (mid->flags & MON_IN_USE) != 0) {
	if (name == 0) {
	    if (ValidHandle(key)) {
		name = Object2CString((JHandle *) key);
	    } else {
		name = "unknown key";
	    }
	}
	fprintf(stderr, "    %s (key=0x%x): ", name, key);
	sysMonitorDumpInfo((sys_mon_t *)&mid->mid);
    }
    return;
}


/*
 * Profiling support
 */
#define MAX_CLASS_HASH 1023

struct arrayinfo arrayinfo[] = {
    { 0,             0, 		 "N/A",          0},
    { 0,             0, 		 "N/A",          0},
    { T_CLASS,      SIGNATURE_CLASS,	"class[]",      sizeof(OBJECT)},
    { 0,             0, 		 "N/A",          0},
    { T_BOOLEAN,    SIGNATURE_BOOLEAN,	"bool[]",       sizeof(char)},
    { T_CHAR,       SIGNATURE_CHAR,	"char[]",       sizeof(unicode)},
    { T_FLOAT,      SIGNATURE_FLOAT,	"float[]",      sizeof(float)},
    { T_DOUBLE,     SIGNATURE_DOUBLE,	"double[]",     sizeof(double)},
    { T_BYTE,       SIGNATURE_BYTE,	"byte[]",       sizeof(char)},
    { T_SHORT,      SIGNATURE_SHORT,	"short[]",      sizeof(short)},
    { T_INT,        SIGNATURE_INT,	"int[]",        sizeof(int)},
    { T_LONG,       SIGNATURE_LONG,	"long[]",       sizeof(int64_t)},
#ifdef NO_LONGER_SUPPORTED
    { T_UBYTE,      SIGNATURE_BYTE,	"ubyte[]",      sizeof(char)},
    { T_USHORT,     SIGNATURE_SHORT,	"ushort[]",     sizeof(unsigned short)},
    { T_UINT,       SIGNATURE_INT,	"uint[]",       sizeof(int)},
    { T_ULONG,      SIGNATURE_LONG,	"ulong[]",      sizeof(int64_t)},
#endif
};

#define ARRAYTYPES 	(sizeof(arrayinfo) / sizeof(arrayinfo[0]))

void
profHandles(FILE *fp) {
    SetLimits();
    register JHandle *hp = (JHandle *) hpmin;
    ClassClass *cb;
    struct hash_entry {
	ClassClass *cb;
	int count;
	int arraycount;
	int arraylength;
    } *tab, *p;
    int arraycount[ARRAYTYPES];
    int arraylength[ARRAYTYPES];
    int handles_count = (JHandle *)hpmax - (JHandle *)hpmin;
    int handles_used = 0;
    int i;

    /* array type mapping */
    int tmap[64];
    for (i = 0 ; i < ARRAYTYPES ; i++) {
	tmap[arrayinfo[i].index] = i;
    }

    tab = (struct hash_entry *)sysMalloc(sizeof(struct hash_entry) * MAX_CLASS_HASH);
    memset((char *)tab, 0, sizeof(struct hash_entry) * MAX_CLASS_HASH);
    memset((char *)arraycount, 0, sizeof(arraycount));
    memset((char *)arraylength, 0, sizeof(arraylength));

    for (; hp <= (JHandle *) hpmax ; hp++) {
	if ((hp->obj == 0) || (obj_free((unsigned char *)unhand(hp)))) {
	    continue;
	}
	switch (obj_flags(hp)) {
	  case T_NORMAL_OBJECT:
	    cb = obj_classblock(hp);
	    p = &tab[(((long)cb) >> 2) % MAX_CLASS_HASH];
	    while ((p->cb != 0) && (p->cb != cb)) {
		if (p-- == tab) {
		    p = &tab[MAX_CLASS_HASH - 1];
		}
	    }
	    if (p->cb == 0) {
		p->cb = cb;
	    }
	    p->count++;
	    break;

	  case T_CLASS:
	    cb = ((ClassClass **)unhand((HArrayOfObject*)hp)->body)[obj_length(hp)];
	    p = &tab[(((long)cb) >> 2) % MAX_CLASS_HASH];
	    while ((p->cb != 0) && (p->cb != cb)) {
		if (p-- == tab) {
		    p = &tab[MAX_CLASS_HASH - 1];
		}
	    }
	    if (p->cb == 0) {
		p->cb = cb;
	    }
	    p->arraycount++;
	    p->arraylength += obj_length(hp);
	    break;

	  default:
	    if (obj_flags(hp) < T_BOOLEAN) {
		continue;
	    }
	    arraycount[tmap[obj_flags(hp)]]++;
	    arraylength[tmap[obj_flags(hp)]] += obj_length(hp);
	    break;
	}
	handles_used++;
    }

    fprintf(fp, "# handles-used, handles-free heap-used heap-free\n");
    fprintf(fp, "%d %d %d %d\n", handles_used, handles_count - handles_used, 
	    TotalObjectCtr - FreeObjectCtr, FreeObjectCtr);

    fprintf(fp, "# type count bytes\n");
    for (i = 0 ; i < ARRAYTYPES ; i++) {
	if (arraycount[i]) {
	    fprintf(fp, "[%c %d %d\n", arrayinfo[i].sig, arraycount[i], arraylength[i] * arrayinfo[i].factor);
	}
    }
    p = &tab[MAX_CLASS_HASH];
    while (p-- != tab) {
	if (p->cb != 0) {
	    if (p->count > 0) {
		fprintf(fp, "L%s; %d %d\n", 
			classname(p->cb), p->count, 
			p->count * cbInstanceSize(p->cb));
	    }
	    if (p->arraycount > 0) {
		fprintf(fp, "[L%s; %d %d\n", classname(p->cb), p->arraycount, p->arraylength * sizeof(OBJECT));
	    }
	}
    }
    sysFree(tab);
}

void
prof_heap(FILE *fp)
{

    gc(0, ~0);
    profHandles(fp);
}


/*
 * Debugging facilities and useful stuff to call from dbx...
 */

#ifdef DEBUG

void
DumpHeapInfo()
{
    fprintf(stderr, "Heap layout info:\n");
    fprintf(stderr, "  Mapped memory:\t[0x%x,0x%x), size: 0x%x\n",
	    heapbase, heaptop, heaptop-heapbase);
    fprintf(stderr, "  Committed memory:\t[0x%x,0x%x), size: 0x%x\n",
	    hpool, opoollimit, opoollimit-hpool);
    fprintf(stderr, "  Handle space: \t[0x%x,0x%x], size: 0x%x\n",
	    hpool, hpoollimit, opool-hpool);
    fprintf(stderr, "  Object space: \t[0x%x,0x%x], size: 0x%x\n",
	    opool, opoollimit, opoollimit-opool+sizeof(hdr));
}

int
isMarked(char *p)
{
    SetLimits();
    return (IsMarked(p) ? 1 : 0);
}

int
isHandle(p)
{
    SetLimits();
    return (ValidHandle(p) ? 1 : 0);
}

int
isObject(p)
{
    SetLimits();
    return (ValidObject(p) ? 1 : 0);
}

int
isHorO(p)
{
    SetLimits();
    return (ValidHorO(p) ? 1 : 0);
}

int GCValidHandle(JHandle *h) {
    SetLimits();

    return ValidHandle(h) && h->obj != 0;
}

void
printPool(char *s)
{
    SetLimits();
    register unsigned char *p = opmin;
    register col = 0;
    char buf[100];
    if (s) {
	printf("\014%s\n", s);
    }
    while (p < opmax) {
	if (++col > 7) {
	    putchar('\n'), col = 1;
	}
	if (obj_len(p) < 4 || obj_len(p) > (opoollimit-opool)) {
	    printf("Bogus length");
	    break;
	}
	printf("%d%c%c%c\t", obj_len(p),
	       p == opoolhand ? '*' : ' ',
	       obj_free(p) ? 'F' : ' ',
	       IsMarked(p + sizeof(hdr)) ? 'P' : ' ');
	p += obj_len(p);
    }
    putchar('\n');
}

void
printHandles(char *s)
{
    SetLimits();
    JHandle *hp;
    printf("\n\n%s\n", s);
    for (hp = (JHandle *) hpool; hp <= (JHandle *) hpmax; hp++) {
	if (hp->obj != 0) {
	    printf("%X %X%c\n", hp, hp->obj,
		 ValidObject(hp->obj) ? (IsMarked(hp->obj) ? 'P' : ' ') : '*');
	}
    }
    printf("\n");
}

void
verifyObjectCtrs(void)
{
    SetLimits();
    register unsigned char *p = opmin;
    int free = 0;
    int inuse = 0;
    int total = 0;

    while (p < opmax) {
	if (obj_free(p)) {
	    free += obj_len(p);
	} else {
	    inuse += obj_len(p);
	}
	total += obj_len(p);
	p += obj_len(p);
    }

    sysAssert(free == FreeObjectCtr);
    sysAssert(inuse == TotalObjectCtr - FreeObjectCtr);
    sysAssert(total == TotalObjectCtr);

/*  For more verbose output:

    fprintf(stderr, "verifyObjectCtrs() detected:\n");
    fprintf(stderr, "    %d bytes free versus %d bytes accumulated\n",
	    free, FreeObjectCtr);
    fprintf(stderr, "    %d bytes in use versus %d bytes accumulated\n",
	    inuse, TotalObjectCtr - FreeObjectCtr);
    fprintf(stderr, "    %d bytes total versus %d bytes accumulated\n",
	    total, TotalObjectCtr);
 */
}

void
verifyHandleCtrs(void)
{
    SetLimits();
    register JHandle *hp;
    int free = 0;
    int inuse = 0;
    int total = 0;

    for (hp = (JHandle *) hpool; hp <= (JHandle *) hpmax; hp++) {
	if (hp->obj == 0) {
	    free += sizeof(JHandle);
	} else {
	    inuse += sizeof(JHandle);
	}
	total += sizeof(JHandle);
    }

    sysAssert(free == FreeHandleCtr);
    sysAssert(inuse == TotalHandleCtr - FreeHandleCtr);
    sysAssert(total == TotalHandleCtr);

/*  For more verbose output:

    fprintf(stderr, "verifyHandleCtrs() detected:\n");
    fprintf(stderr, "    %d bytes free versus %d bytes accumulated\n",
	    free, FreeHandleCtr);
    fprintf(stderr, "    %d bytes in use versus %d bytes accumulated\n",
	    inuse, TotalHandleCtr - FreeHandleCtr);
    fprintf(stderr, "    %d bytes total versus %d bytes accumulated\n",
	    total, TotalHandleCtr);
 */
}

void
validatePool(long quick)
{
    SetLimits();
    register unsigned char *p = opmin;
    register JHandle *hp;
    int err = 0;
    char buf[100];

    while (p < opmax) {
	sysAssert(obj_len(p) >= 4);
	sysAssert(obj_len(p) < (opoollimit-opool));
	if (obj_free(p)) { 
	    sysAssert(((long *) p)[1] == 0x55555555);
	} else if (!quick) {
	    for (hp = (JHandle *) hpool; hp <= (JHandle *) hpmax; hp++) {
		if (hp->obj == (ClassObject *) (p + sizeof(hdr))) {
		    goto HasHandle;
		}
	    }
	    fprintf(stderr, "@%X -- alloced object with no handle\n", p), err++;
	}
HasHandle:
	p += obj_len(p);
    }
    for (hp = (JHandle *) hpool; hp <= (JHandle *) hpmax; hp++) {
	if (hp->obj)
	    if (ValidObject(hp->obj) && obj_free((char *) hp->obj - sizeof(hdr))) {
		fprintf(stderr, "@%X/%X -- Handle points to free block\n", hp, hp->obj), err++;
	    } else {
		switch (obj_flags(hp)) {
		case T_NORMAL_OBJECT:
		    if (obj_methodtable(hp) == 0)
			fprintf(stderr, "1 @%X/%X -- Invalid method table\n", hp, hp->methods), err++;
		    else if (obj_methodtable(hp) != cbMethodTable(obj_classblock(hp)))
			fprintf(stderr, "2 @%X/%X -- Invalid method table\n", hp, hp->methods), err++;
		   break;

		default:
		   if (obj_flags(hp) < T_CLASS )
		       fprintf(stderr, "3 @%X/%X -- Invalid method table\n", hp, hp->methods), err++;
		}
	    }
    }
    fprintf(stderr, err == 0 ? "OK memory pool\n" : "***%d errors in the memory pool***\n", err);
}
#else
#define validatepool(a) 0
#endif /* DEBUG */


#ifdef NOTCURRENTLYUSED

void
objFree(HObject * o)
{
    register unsigned char *p;

    HEAP_LOCK();
#ifdef DEBUG
    if (o == 0) {
	printf("freeing null object\n");
	sysAbort();
    }
#endif /* DEBUG */

    p = ((unsigned char *) unhand(o)) - sizeof(hdr);

#ifdef DEBUG
    if (p > opoollimit || p < opool) {
	printf("freeing object not in the pool\n");
	sysAbort();
    }
#endif /* DEBUG */

    if (!obj_free(p)) {
	FreeObjectCtr += obj_len(p);
	obj_setfree(p);
	unhand(o) = 0;
#ifdef DEBUG
	((long *) p)[1] = 0x55555555;
#endif /* DEBUG */
    }
    HEAP_UNLOCK();
}

#endif /* NOTCURRENTLYUSED */
