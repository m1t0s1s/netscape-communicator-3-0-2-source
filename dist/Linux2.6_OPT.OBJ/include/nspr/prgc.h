#ifndef prgc_h___
#define prgc_h___

/*
** API to NSPR gc memory system.
*/
#include "prosdep.h"
#include "prmacros.h"
#include "prtypes.h"
#include <stdio.h>

NSPR_BEGIN_EXTERN_C

struct PRHashTableStr;

/*
** Initialize the garbage collector.
** 	"flags" is the trace flags (see below).
** 	"initialHeapSize" is the initial size of the heap and may be zero
** 	   if the default is desired.
*/
extern PR_PUBLIC_API(void) PR_InitGC(prword_t flags, 
                                     prword_t initialHeapSize);

/*
** GC Type record. This defines all of the GC operations used on a
** particular object type. These structures are passed to
** PR_RegisterType.
*/
typedef struct GCTypeStr {
    /*
    ** Scan an object that is in the GC heap and call PR_LiveObject on
    ** all of the pointers in it. If this slot is null then the object
    ** won't be scanned (i.e. it has no embedded pointers).
    */
    void (*scan)(void GCPTR *obj);

    /*
    ** Finalize an object that has no references. This is called by the
    ** GC after it has determined where the object debris is but before
    ** it has moved the debris to the logical "free list". The object is
    ** marked alive for this call and removed from the list of objects
    ** that need finalization (finalization only happens once for an
    ** object). If this slot is null then the object doesn't need
    ** finalization.
    */
    void (*finalize)(void GCPTR *obj);

    /*
    ** Dump out an object during a PR_DumpGCHeap(). This is used as a
    ** debugging tool.
    */
    void (*dump)(FILE *out, void GCPTR *obj, PRBool detailed, int indentLevel);

    /*
    ** Add object to summary table.
    */
    void (*summarize)(void GCPTR *obj, size_t bytes);

    /*
    ** Free hook called by GC when the object is being freed.
    */
    void (*free)(void *obj);
} GCType;

/*
** This data structure must be added as the hash table passed to 
** the summarize method of GCType.
*/ 
typedef struct PRSummaryEntry {
    void*	clazz;
    int32	instancesCount;
    int32	totalSize;
} PRSummaryEntry;

/*
** This function pointer must be registered by users of nspr
** to produce the finally summary after all object in the
** heap have been visited.
*/
typedef void (*PRSummaryPrinter)(FILE *out, void* closure);

extern PR_PUBLIC_API(void) 
PR_RegisterSummaryPrinter(PRSummaryPrinter fun, void* closure);

typedef void GCRootFinder(void *arg);
typedef void GCBeginFinalizeHook(void *arg);
typedef void GCEndFinalizeHook(void *arg);
typedef void GCBeginGCHook(void *arg);
typedef void GCEndGCHook(void *arg);

/*
** Hooks which are called at the beginning and end of the GC process.
** The begin hooks are called before the root finding step. The hooks are
** called with threading disabled, so it is now allowed to re-enter the
** kernel. The end hooks are called after the gc has finished but before
** the finalizer has run.
*/
extern PR_PUBLIC_API(void) PR_SetBeginGCHook(GCBeginGCHook *hook, void *arg);
extern PR_PUBLIC_API(void) PR_GetBeginGCHook(GCBeginGCHook **hook, void **arg);
extern PR_PUBLIC_API(void) PR_SetEndGCHook(GCBeginGCHook *hook, void *arg);
extern PR_PUBLIC_API(void) PR_GetEndGCHook(GCEndGCHook **hook, void **arg);

/*
** Hooks which are called at the beginning and end of the GC finalization
** process. After the GC has identified all of the dead objects in the
** heap, it looks for objects that need finalization. Before it calls the
** first finalization proc (see the GCType structure above) it calls the
** begin hook. When it has finalized the last object it calls the end
** hook.
*/
extern PR_PUBLIC_API(void) PR_SetBeginFinalizeHook(GCBeginFinalizeHook *hook, 
                                                  void *arg);
extern PR_PUBLIC_API(void) PR_GetBeginFinalizeHook(GCBeginFinalizeHook **hook, 
                                                  void **arg);
extern PR_PUBLIC_API(void) PR_SetEndFinalizeHook(GCBeginFinalizeHook *hook, 
                                                void *arg);
extern PR_PUBLIC_API(void) PR_GetEndFinalizeHook(GCEndFinalizeHook **hook, 
                                                void **arg);

/*
** Register a GC type. Return's the index into the GC internal type
** table. The returned value is passed to PR_AllocMemory. After the call,
** the "type" memory belongs to the GC (the caller must not free it or
** change it).
*/
extern PR_PUBLIC_API(int) PR_RegisterType(GCType *type);

/*
** Register a root finder with the collector. The collector will call
** these functions to identify all of the roots before collection
** proceeds. "arg" is passed to the function when it is called.
*/
extern PR_PUBLIC_API(int) PR_RegisterRootFinder(GCRootFinder func,
						char *name, void *arg);

#ifdef DEBUG
/*
** Logging message function that root processors can call.
*/
#define PR_PROCESS_ROOT_LOG(x) (*PR_GetGCInfo()->processRootLog) x
#else
#define PR_PROCESS_ROOT_LOG(x)
#endif /* DEBUG */

#ifdef DEBUG
/*
** Logging message function that root processors can call.
*/
#define PR_LIVE_OBJECT_LOG(x) (*pr_liveObjectLog) x
#else
#define PR_LIVE_OBJECT_LOG(x)
#endif /* DEBUG */

/*
** Allocate some GC'able memory. The object must be at least bytes in
** size. The type index function for the object is specified. "flags"
** specifies some control flags. If PR_ALLOC_CLEAN is set then the memory
** is zero'd before being returned. If PR_ALLOC_DOUBLE is set then the
** allocated memory is double aligned.
**
** Any memory cell that you store a pointer to something allocated by
** this call must be findable by the GC. Use the PR_RegisterRootFinder to
** register new places where the GC will look for pointers into the heap.
** The GC already knows how to scan any NSPR threads or monitors.
*/
extern PR_PUBLIC_API(prword_t GCPTR *)PR_AllocMemory(prword_t bytes, 
						     int typeIndex, int flags);

/*
** This function can be used to cause PR_AllocMemory to always return
** NULL. This may be useful in low memory situations when we're trying to
** shutdown applets.
*/
extern PR_PUBLIC_API(void) PR_EnableAllocation(int yesOrNo);

/* flags bits */
#define PR_ALLOC_CLEAN		1
#define PR_ALLOC_DOUBLE		2
#define PR_ALLOC_ZERO_HANDLE    3               /* XXX yes, it's a hack */

#if 0
/* Not yet... */
extern PR_PUBLIC_API(char *) PR_AllocString(prword_t size);
extern PR_PUBLIC_API(char *) PR_DupString(char *in);
#endif

/*
** Force a garbage collection right now. Return when it completes.
*/
extern PR_PUBLIC_API(void) PR_GC(void);

/*
** Force a finalization right now. Return when finalization has
** completed. Finalization completes when there are no more objects
** pending finalization. This does not mean there are no objects in the
** gc heap that will need finalization should a collection be done after
** this call.
*/
extern PR_PUBLIC_API(void) PR_ForceFinalize(void);

/*
** Dump the GC heap out to the given file. This will stop the system dead
** in it's tracks while it is occuring.
*/
extern PR_PUBLIC_API(void) PR_DumpGCHeap(FILE *out, PRBool detailed);

/*
** Wrapper for PR_DumpGCHeap
*/
extern PR_PUBLIC_API(void) PR_DumpMemory(PRBool detailed);

/*
** Dump summary of objects allocated.
*/
extern PR_PUBLIC_API(void) PR_DumpMemorySummary(void);

/*
** Dump the application heaps.
*/
extern PR_PUBLIC_API(void) PR_DumpApplicationHeaps(void);

/* 
** Helper function used by dump routines to do the indentation in a
** consistent fashion.
*/
extern PR_PUBLIC_API(void) PR_DumpIndent(FILE *out, int indent);

/*
** The GCInfo structure contains all of the GC state...
**
** busyMemory:
**    The amount of GC heap memory that is busy at this instant. Busy
**    doesn't mean alive, it just means that it has been
**    allocated. Immediately after a collection busy means how much is
**    alive.
**
** freeMemory:
**    The amount of GC heap memory that is as yet unallocated.
**
** allocMemory:
**    The sum of free and busy memory in the GC heap.
**
** maxMemory:
**    The maximum size that the GC heap is allowed to grow.
**
** lowSeg:
**    The lowest segment currently used in the GC heap.
**
** highSeg:
**    The highest segment currently used in the GC heap.  
**    The lowSeg and highSeg members are used for a "quick test" of whether 
**    a pointer falls within the GC heap. [ see GC_IN_HEAP(...) ]
**
** lock:
**    Monitor used for syncronization within the GC.
**
** finalizer:
**    Thread in which the GC finalizer is running.
**
** liveObject:
**    Object scanning functions call through this function pointer to
**    register a potential block of pointers with the collector. (This is
**    currently not at all different than processRoot.)
**
** liveObjectLog:
**    Logging message function that object scanners can call.
**
** processRoot:
**    When a root finder identifies a root it should call through this
**    function pointer so that the GC can process the root. The call takes
**    a base address and count which the gc will examine for valid heap
**    pointers.
**
** processRootLog:
**    Logging message function that root processors can call.
*/
typedef struct GCInfoStr {
    prword_t  flags;         /* trace flags (see below)               */
    prword_t  busyMemory;    /* memory in use right now               */
    prword_t  freeMemory;    /* memory free right now                 */
    prword_t  allocMemory;   /* sum of busy & free memory             */
    prword_t  maxMemory;	 /* max memory we are allowed to allocate */
    prword_t *lowSeg;        /* lowest segment in the GC heap         */
    prword_t *highSeg;       /* higest segment in the GC heap         */

    PRMonitor *lock;
    PRThread  *finalizer;
    void (*liveObject)(void **base, int32 count);
    void (*processRoot)(void **base, int32 count);

    void (*liveObjectLog) (const char *msg, ...);
    void (*processRootLog)(const char *msg, ...);
} GCInfo;

extern PR_PUBLIC_API(GCInfo *) PR_GetGCInfo(void);
extern PR_PUBLIC_API(PRBool) PR_GC_In_Heap(void GCPTR *object);

/*
** Simple bounds check to see if a pointer is anywhere near the GC heap.
** Used to avoid calls to PR_ProcessRoot and PR_LiveObject by object
** scanning code.
*/
#if !defined(XP_PC) || defined(_WIN32)
#define GC_IN_HEAP(_info, _p) (((prword_t*)(_p) >= (_info)->lowSeg) && \
                               ((prword_t*)(_p) <  (_info)->highSeg))
#else
/*
** The simple bounds check, above, doesn't work in Win16, because we don't
** maintain: lowSeg == MIN(all segments) and highSeg == MAX(all segments).
** So we have to do a little better.
*/
#define GC_IN_HEAP(_info, _p) PR_GC_In_Heap(_p)
#endif

/************************************************************************/

/* Trace flags (passed to PR_InitGC or in environment GCLOG) */
#define GC_TRACE	0x0001
#define GC_ROOTS	0x0002
#define GC_LIVE		0x0004
#define GC_ALLOC	0x0008
#define GC_MARK		0x0010
#define GC_SWEEP	0x0020
#define GC_DEBUG	0x0040
#define GC_FINAL	0x0080

#ifdef DEBUG
#define GCTRACE(x, y) if (PR_GetGCInfo()->flags & x) GCTrace y
extern PR_PUBLIC_API(void) GCTrace(char *fmt, ...);
#else
#define GCTRACE(x, y)
#endif

NSPR_END_EXTERN_C

#endif /* prgc_h___ */
