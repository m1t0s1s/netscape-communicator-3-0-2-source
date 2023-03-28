#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prthread.h"
#include "prlog.h"
#include "gcint.h"
#include "mdint.h"

#if defined(XP_PC) && !defined(_WIN32)
#include "swkern.h"             /* _pr_top_of_task_stack */
#endif

/*
** Generic GC implementation independent code for the NSPR GC
*/

RootFinder *_pr_rootFinders;

GCType *_pr_gcTypes[GC_MAX_TYPES];

int _pr_threadCBIX;		/* CBIX for thread object's */

int _pr_monCBIX;		/* CBIX for monitor object's */

/* GC State information */
GCInfo _pr_gcData;

GCBeginGCHook *_pr_beginGCHook;
void *_pr_beginGCHookArg;
GCBeginGCHook *_pr_endGCHook;
void *_pr_endGCHookArg;

GCBeginFinalizeHook *_pr_beginFinalizeHook;
void *_pr_beginFinalizeHookArg;
GCBeginFinalizeHook *_pr_endFinalizeHook;
void *_pr_endFinalizeHookArg;

FILE *_pr_dump_file;
int _pr_do_a_dump;

/************************************************************************/

/*
** Scan a thread's stack memory and it's registers, looking for pointers
** into the GC heap.
*/
static int ScanOneThread(PRThread *t, int i, void *notused)
{
    prword_t *sp, *esp, *p0;
    PRThread *current = PR_CurrentThread();
    PRPerThreadData *ptd;
    void (*processRoot)(void **base, int32 count);
    int n;
    int stack_end;

#if defined(XP_PC) && defined(_WIN32)
    /*
    ** In Win32 the thread stack is not available until the thread has been 
    ** started...  So, unborn threads have no stack (yet).
    */
    if (t->state == _PR_UNBORN) {
        return 1;
    }
#endif

    processRoot = _pr_gcData.processRoot;
    if( (t->flags & _PR_NO_SUSPEND) && (t != current) ) {
        GCTRACE(GC_ROOTS, ("Oh no Mr. Bill thread: %s is not suspended.",
			   t->name));
	PR_ASSERT((t->flags & _PR_NO_SUSPEND) && (t != current) );
        return 1;
    } 

    GCTRACE(GC_ROOTS, ("Scanning thread %s for roots", t->name));

    /*
    ** Store the thread's registers in the thread structure so the GC
    ** can scan them. Then scan them.
    */
    p0 = _MD_HomeGCRegisters(t, t == current, &n);
    GCTRACE(GC_ROOTS, ("Scanning %d registers", n));
    (*processRoot)((void**)p0, n);

    /* Scan the C stack for pointers into the GC heap */
#if defined(XP_PC) && !defined(_WIN32)
    /*
    ** Under WIN16, the stack of the current thread is always mapped into
    ** the "task stack" (at SS:xxxx).  So, if t is the current thread, scan
    ** the "task stack".  Otherwise, scan the "cached stack" of the inactive
    ** thread...
    */
    if (t == current) {
        sp  = (prword_t*) &stack_end;
        esp = (prword_t*) _pr_top_of_task_stack;

        PR_ASSERT(sp <= esp);
    } else {
        sp  = (prword_t*) PR_GetSP(t);
        esp = (prword_t*) t->stack->stackTop;

	PR_ASSERT((t->stack->stackSize == 0) ||
                 ((sp >  (prword_t*)t->stack->stackBottom) &&
		  (sp <= (prword_t*)t->stack->stackTop)));
    }
#else   /* ! WIN16 */
#ifdef HAVE_STACK_GROWING_UP
    if (t == current) {
        esp = (prword_t*) &stack_end;
    } else {
        esp = (prword_t*) PR_GetSP(t);
    }
    sp = (prword_t*) t->stack->stackTop;
    if (t->stack->stackSize) {
        PR_ASSERT((esp > (prword_t*)t->stack->stackTop) &&
                  (esp < (prword_t*)t->stack->stackBottom));
    }
#else   /* ! HAVE_STACK_GROWING_UP */
    if (t == current) {
        sp = (prword_t*) &stack_end;
    } else {
        sp = (prword_t*) PR_GetSP(t);
    }
    esp = (prword_t*) t->stack->stackTop;
    if (t->stack->stackSize) {
	PR_ASSERT((sp > (prword_t*)t->stack->stackBottom) &&
		  (sp < (prword_t*)t->stack->stackTop));
    }
#endif  /* ! HAVE_STACK_GROWING_UP */
#endif  /* ! WIN16 */

#if defined(XP_PC) && !defined(_WIN32)
    PRWin16ScanCStack(t, sp, esp);
#else
    if (sp < esp) {
        GCTRACE(GC_ROOTS, ("Scanning C stack from %p to %p", sp, esp));
        (*processRoot)((void**)sp, esp - sp);
    }
#endif

    /*
    ** Mark all of the per-thread-data items attached to this thread
    */
    GCTRACE(GC_ROOTS, ("Scanning per-thread-data"));
    ptd = t->ptd;
    while (ptd) {
        (*processRoot)((void**)&ptd->priv, 1);
	ptd = ptd->next;
    }

    return 1;
}

/*
** Scan all of the threads C stack's and registers, looking for "root"
** pointers into the GC heap. These are the objects that the GC cannot
** move and are considered "live" by the GC. Caller has stopped all of
** the threads from running.
*/
static void ScanThreads(void *notused)
{
    PR_EnumerateThreads(ScanOneThread, 0);
}

static GCType threadType = {
    ScanThreads,
};

/************************************************************************/

PR_PUBLIC_API(GCInfo *) PR_GetGCInfo(void)
{
    return &_pr_gcData;
}


PR_PUBLIC_API(int) PR_RegisterType(GCType *t)
{
    int i, rv = -1;

    LOCK_GC();
    for (i = 0; i < GC_MAX_TYPES; i++) {
	if (_pr_gcTypes[i] == 0) {
	    _pr_gcTypes[i] = t;
	    rv = i;
	    break;
	}
    }
    UNLOCK_GC();
    return rv;
}

PR_PUBLIC_API(int) PR_RegisterRootFinder(GCRootFinder f, char *name, void *arg)
{
    RootFinder *rf = (RootFinder*) calloc(1, sizeof(RootFinder));
    if (rf) {
	rf->func = f;
	rf->name = name;
	rf->arg = arg;

	LOCK_GC();
	rf->next = _pr_rootFinders;
	_pr_rootFinders = rf;
	UNLOCK_GC();
	return 0;
    }
    return -1;
}

PR_PUBLIC_API(void) PR_SetBeginGCHook(GCBeginGCHook *hook, void *arg)
{
    LOCK_GC();
    _pr_beginGCHook = hook;
    _pr_beginGCHookArg = arg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_GetBeginGCHook(GCBeginGCHook **hook, void **arg)
{
    LOCK_GC();
    *hook = _pr_beginGCHook;
    *arg = _pr_beginGCHookArg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_SetEndGCHook(GCEndGCHook *hook, void *arg)
{
    LOCK_GC();
    _pr_endGCHook = hook;
    _pr_endGCHookArg = arg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_GetEndGCHook(GCEndGCHook **hook, void **arg)
{
    LOCK_GC();
    *hook = _pr_endGCHook;
    *arg = _pr_endGCHookArg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_SetBeginFinalizeHook(GCBeginFinalizeHook *hook, void *arg)
{
    LOCK_GC();
    _pr_beginFinalizeHook = hook;
    _pr_beginFinalizeHookArg = arg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_GetBeginFinalizeHook(GCBeginFinalizeHook **hook, 
                                           void **arg)
{
    LOCK_GC();
    *hook = _pr_beginFinalizeHook;
    *arg = _pr_beginFinalizeHookArg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_SetEndFinalizeHook(GCEndFinalizeHook *hook, void *arg)
{
    LOCK_GC();
    _pr_endFinalizeHook = hook;
    _pr_endFinalizeHookArg = arg;
    UNLOCK_GC();
}

PR_PUBLIC_API(void) PR_GetEndFinalizeHook(GCEndFinalizeHook **hook, void **arg)
{
    LOCK_GC();
    *hook = _pr_endFinalizeHook;
    *arg = _pr_endFinalizeHookArg;
    UNLOCK_GC();
}

#ifdef DEBUG
#include "prprf.h"

PR_PUBLIC_API(void) GCTrace(char *fmt, ...)
{	
    va_list ap;
    char buf[400];

    va_start(ap, fmt);
    PR_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    PR_LOG(GC, out, ("%s", buf));
}
#endif

void _PR_InitGC(prword_t flags)
{
    static char firstTime = 1;

    if (!firstTime) return;
    firstTime = 0;

    if (flags == 0) {
	char *ev = getenv("GCLOG");
	if (ev && ev[0]) {
	    flags = atoi(ev);
	}
    }
    _pr_gcData.flags = flags;

    _pr_gcData.lock = PR_NewNamedMonitor(0, "gc-lock");
    PR_RegisterRootFinder(ScanThreads, "scan threads", 0);
    PR_RegisterRootFinder(_PR_ScanFinalQueue, "scan final queue", 0);
    _pr_threadCBIX = PR_RegisterType(&threadType);
}
