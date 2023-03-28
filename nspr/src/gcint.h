#ifndef gcint_h___
#define gcint_h___

#include "prmon.h"
#include "prgc.h"

extern GCInfo _pr_gcData;

#define LOCK_GC()       PR_EnterMonitor(_pr_gcData.lock)
#define UNLOCK_GC()     PR_ExitMonitor (_pr_gcData.lock)
#define GC_IS_LOCKED()  (PR_InMonitor  (_pr_gcData.lock))

extern GCBeginGCHook *_pr_beginGCHook;
extern void *_pr_beginGCHookArg;
extern GCBeginGCHook *_pr_endGCHook;
extern void *_pr_endGCHookArg;

extern GCBeginFinalizeHook *_pr_beginFinalizeHook;
extern void *_pr_beginFinalizeHookArg;
extern GCBeginFinalizeHook *_pr_endFinalizeHook;
extern void *_pr_endFinalizeHookArg;

extern int _pr_do_a_dump;
extern FILE *_pr_dump_file;

/*
** Root finders. Root finders are used by the GC to find pointers into
** the GC heap that are not contained in the GC heap.
*/
typedef struct RootFinderStr RootFinder;

struct RootFinderStr {
    RootFinder *next;
    GCRootFinder *func;
    char *name;
    void *arg;
};
extern RootFinder *_pr_rootFinders;

/*
** GC type table. There is one entry in this table for each type of
** object that can be managed by the GC.
*/
#define GC_MAX_TYPES	256
extern GCType *_pr_gcTypes[GC_MAX_TYPES];

/* Slot in _pr_gcTypes used for free memory */
#define FREE_MEMORY_TYPEIX 255

extern int _pr_threadCBIX;		/* CBIX for thread object's */
extern int _pr_monCBIX;		/* CBIX for monitor object's */

extern void _PR_InitGC(prword_t flags);
extern void _PR_ScanFinalQueue(void *notused);

#endif /* gcint_h___ */
