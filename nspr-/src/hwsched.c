#ifdef HW_THREADS
#include <stdio.h>
#include "prlog.h"
#include "prtime.h"
#include "hwkern.h"
#include "mdint.h"

PR_LOG_DEFINE(SCHED);
PR_LOG_DEFINE(CLOCK);
#ifdef XP_UNIX
extern PRLogModule IOLog;
#endif

#define NUM_PRIORITIES 32

/*
** Kernel data structures...
*/
PRCList _pr_runq[32];
int _pr_user_active;
int _pr_system_active;

#if defined( XP_PC ) && defined( _WIN32 )
__declspec( thread ) PRThread * _pr_current_thread;
__declspec( thread ) int        _pr_intsOff;

CRITICAL_SECTION                _pr_schedLock;
uint32                          _pr_PostEventMsgId;
#endif  /* XP_PC && _WIN32 */

PRThread *_pr_idle;
PRThread *_pr_main_thread;
PRMonitor *_pr_heap_lock;
PRMonitor *_pr_dns_lock;
/* This is used by utilities like WH_TempName and WH_FileName: */
PRMonitor *_pr_TempName_lock;

/*
** Disable interrupts. Return old interrupt status value.
*/
int _PR_IntsOff(void)
{
    LOCK_SCHEDULER();
    return _pr_intsOff;
}

/*
** Restore interrupt status value from it's previously stacked value.
*/
int _PR_IntsOn(int status, int wantYield)
{
    UNLOCK_SCHEDULER();
    return _pr_intsOff;
}

void PR_SingleThread(void)
{
    int j;
    PRCList *qp;
    PRThread *current, *t;

    LOCK_SCHEDULER();
    current = PR_CurrentThread();

    for (j = 0; j < PR_NUM_PRIORITIES; j++) {
        qp = _pr_runq[j].next;
        while (qp != &_pr_runq[j]) {
            t = THREAD_PTR(qp);
            if( t != current ) {
                PR_Suspend(t);
            }
            qp = qp->next;
        }
    }
    UNLOCK_SCHEDULER();
}

void PR_MultiThread(void)
{
    int j;
    PRCList *qp;
    PRThread *current, *t;

    LOCK_SCHEDULER();
    current = PR_CurrentThread();

    for (j = 0; j < PR_NUM_PRIORITIES; j++) {
        qp = _pr_runq[j].next;
        while (qp != &_pr_runq[j]) {
            t = THREAD_PTR(qp);
            if( t != current ) {
                PR_Resume(t);
            }
            qp = qp->next;
        }
    }
    UNLOCK_SCHEDULER();
}

extern void _PR_Pause(void);
static void Idle(void)
{
    for (;;) {
        _PR_Pause();
    }
}

static PRBool initialized;

PR_PUBLIC_API(PRBool) PR_Initialized(void)
{
    return initialized;
}

/*
** Initialize the kernel
*/
PR_PUBLIC_API(void) PR_Init(char *name, int priority, int maxCpus, int flags)
{
    int i;
    PRThread *thread;
    PRThreadStack *ts;

    if (initialized) return;
    initialized = PR_TRUE;

    /* Init the system resources */
#if defined( XP_PC ) && defined( _WIN32 )
    InitializeCriticalSection( &_pr_schedLock );

    _pr_PostEventMsgId = RegisterWindowMessage("NSPR_PostEvent");
#endif  /* XP_PC && _WIN32 */

    _MD_InitOS(_MD_INIT_AT_START);

#ifdef DEBUG
    /*
    ** We need to pre-initialize these because the logging mechanism
    ** depends on them.
    */
    PR_LogInit(&SCHEDLog);
    PR_LogInit(&CLOCKLog);
#ifdef XP_UNIX
    PR_LogInit(&IOLog);
#endif
#endif

    if (!pr_pageSize) {
        PR_InitPageStuff();
    }
    /* Setup various queue's */
    for (i = 0; i < PR_NUM_PRIORITIES; i++) {
        PR_INIT_CLIST(&_pr_runq[i]);
    }
    PR_INIT_CLIST(&_pr_all_monitors);

    /* Setup the IO subsystem. */
    _PR_InitIO();

    /* Create thread that represents the caller */
    ts = (PRThreadStack*) calloc(1, sizeof(PRThreadStack));

    /* Set up the thread stack information */
    ts->stackSize   = _MD_DEFAULT_STACK_SIZE;  /* rjp - main stack size ??? */
    ts->allocBase   = (char *)&name;
    ts->stackTop    = (char *)&name;
    ts->stackBottom = ts->stackTop - _MD_DEFAULT_STACK_SIZE;

    _pr_main_thread = thread = PR_AttachThread(name, priority, ts);

    thread->flags = flags & (_PR_NO_SUSPEND);     /* only this bit! */


    /* Get heapLock now that enough init has been done */
    _pr_heap_lock = PR_NewNamedMonitor(0, "heap-lock");
    PR_ASSERT(_pr_heap_lock != 0);
    _pr_dns_lock  = PR_NewNamedMonitor(0, "dns-lock");
    PR_ASSERT(_pr_dns_lock != 0);
    _pr_TempName_lock = PR_NewNamedMonitor(0, "TempName-lock");
    PR_ASSERT(_pr_TempName_lock != 0);

    _PR_InitMonitorCache();

#if 0
    /* Create idle thread */
    _pr_idle = PR_CreateThread("idle", 0, 0, 32768);
    _pr_idle->flags |= _PR_SYSTEM;
    if (PR_Start(_pr_idle, (void (*)(void*,void*))Idle, 0, 0) < 0) {
        PR_LOG(SCHED, out,
               ("PR_Init: can't start idle thread"));
        PR_Abort();
    }
#endif

    /* Finish up machine dependent os init */
    _MD_InitOS(_MD_INIT_READY);
}

/*
** Shutdown NSPR
*/
PR_PUBLIC_API(void) PR_Shutdown(void)
{
    /* Unload all loaded DLLs */
    _PR_ShutdownLinker();
}

#endif  /* HW_THREADS */

