#ifdef SW_THREADS
#include <stdio.h>
#include "prlog.h"
#include "prtime.h"
#include "swkern.h"
#include "mdint.h"

#ifdef XP_MAC
#include "prmacos.h"
#endif

#ifdef PURIFY
#include "pure_thr.h"
#endif

PR_LOG_DEFINE(SCHED);
extern PRLogModule CLOCKLog;
#ifdef XP_UNIX
extern PRLogModule IOLog;
#endif

#ifdef PR_NO_PREEMPT
int pr_want_resched;
#endif

#if defined(XP_PC) && !defined(_WIN32)
char *  _pr_top_of_task_stack;
uint32  _pr_PostEventMsgId;
#endif  /* XP_PC && ! _WIN32 */

PRCList _pr_runq[32];
PRCList _pr_sleepq;
PRCList _pr_zombieq;
PRCList _pr_suspendq;
PRCList _pr_lockq;
PRCList _pr_monitorq;

int32 _pr_runq_ready_mask;
int64 _pr_now;
int _pr_user_active;
int _pr_system_active;
PRThread *_pr_current_thread;
PRThread *_pr_idle;

PRMonitor *_pr_heap_lock;
PRMonitor *_pr_rusty_lock;
PRMonitor *_pr_single_lock;
PRMonitor *_pr_dns_lock;
PRMonitor *_pr_child_intr;
PRMonitor *_pr_async_io_intr;
/* This is used by utilities like WH_TempName and WH_FileName: */
PRMonitor *_pr_TempName_lock;

PR_PUBLIC_API(void) PR_SingleThread(void)
{
#ifdef XP_MAC
	// Turn off the time-slicing timer.  When the timer fires, we check to see
	// if there are sleeping threads & rewake them if necessary.  That would be bad. 
    DeactivateTimer();
#endif

    PR_EnterMonitor(_pr_single_lock);
}

PR_PUBLIC_API(void) PR_MultiThread(void)
{
    PR_ExitMonitor(_pr_single_lock);

#ifdef XP_MAC
    ActivateTimer();
#endif
}

/************************************************************************/

/*
** Pick a new thread to run.
*/
void _PR_Schedule(void)
{
    PRThread *thread;
    int32 pri;
    unsigned long r;

    /* Interrupts must be off the entire time we are doing this */
    PR_ASSERT(_pr_intsOff != 0);

#ifdef PR_NO_PREEMPT
    /* Since we are rescheduling, we no longer want to */
    pr_want_resched = 0;
#endif

    /*
    ** Find highest priority thread to run. Bigger priority numbers are
    ** higher priority threads
    */
    r = _pr_runq_ready_mask;
    pri = 0;
    if (r >> 16) { pri += 16; r >>= 16; }
    if (r >> 8) { pri += 8; r >>= 8; }
    if (r >> 4) { pri += 4; r >>= 4; }
    if (r >> 2) { pri += 2; r >>= 2; }
    if (r >> 1) { pri++; }
    PR_ASSERT(_pr_runq[pri].next != &_pr_runq[pri]);

    /* Pull thread off of its run queue */
    thread = THREAD_PTR(_pr_runq[pri].next);
    PR_REMOVE_LINK(&thread->runqLinks);
    PR_INIT_CLIST(&thread->runqLinks);
    if (_pr_runq[pri].next == &_pr_runq[pri]) {
	_pr_runq_ready_mask &= ~(1L << pri);
    }

    /* Resume the thread */
    thread->state = _PR_RUNNING;

    /* _pr_current_thread is now updated inside of _MD_RESTORE_CONTEXT */
    _MD_RESTORE_CONTEXT(thread);

    PR_NOT_REACHED("No more processes to schedule -- deadlock.");
}

#ifdef XP_MAC
// FIX ME!!!!
//	This is a temporary hack!!!
#include <Types.h>
extern long gInTimerCallback;
#endif

void _PR_SwitchThread(PRThread *thread)
{
    PR_ASSERT(thread == _pr_current_thread);
    PR_ASSERT(_pr_intsOff != 0);
    
#ifdef XP_MAC
	if (gInTimerCallback)
		DebugStr("\pFATAL ERROR: Attempt to reschedule at interrupt time!");
#endif

    _MD_SWITCH_CONTEXT(thread);

    PR_ASSERT(thread == _pr_current_thread);
    PR_ASSERT(_pr_intsOff != 0);

#ifdef PURIFY
    pure_notice_context_switch(&thread->id);
#endif
}

/*
** Put thread on sleepQ in the right position for it's desired sleep time
*/
void _PR_PutOnSleepQ(PRThread *thread, int64 sleep)
{
    PRCList *qp;
    PRThread *t;

    PR_ASSERT(_pr_intsOff != 0);
    PR_ASSERT(LL_GE_ZERO(sleep));
    PR_ASSERT(PR_CLIST_IS_EMPTY(&thread->runqLinks));

    /* Sort thread into sleepQ at appropriate point */
    PR_LOG(SCHED, debug,
	   ("put %s on sleepQ for %lld", thread->name, sleep));
    qp = _pr_sleepq.next;
    while (qp != &_pr_sleepq) {
	t = THREAD_PTR(qp);
	if (LL_CMP(sleep, <, t->sleep)) {
	    /* Found sleeper to insert in front of */
	    break;
	}
	LL_SUB(sleep, sleep, t->sleep);
	qp = qp->next;
    }
    thread->sleep = sleep;
    PR_INSERT_BEFORE(&thread->runqLinks, qp);
    thread->flags |= _PR_ON_SLEEPQ;

    /*
    ** Subtract our sleeptime from the sleeper that follows us (if there
    ** is one) so that they remain relative to us.
    */
    if (thread->runqLinks.next != &_pr_sleepq) {
	t = THREAD_PTR(thread->runqLinks.next);
	PR_ASSERT(THREAD_PTR(t->runqLinks.prev) == thread);
	LL_SUB(t->sleep, t->sleep, sleep);
    }

#ifdef DEBUG
    {
	int i = 0;
	PR_LOG(SCHED, debug, ("_pr_sleepq: "));
	qp = _pr_sleepq.next;
	while (qp != &_pr_sleepq) {
	    t = THREAD_PTR(qp);
	    PR_LOG(SCHED, debug,
		   ("[%d] %s %lld", i, t->name, t->sleep));
	    i++;
	    qp = qp->next;
	}
    }
#endif
}

/*
** If there is a thread on the sleepQ following "thread", give it
** thread's remaining sleep time to keep the sleepQ accurate.
*/
void _PR_FixSleepQ(PRThread *thread)
{
    if (thread->runqLinks.next != &_pr_sleepq) {
	/* There is a thread after us */
	PRThread *t = THREAD_PTR(thread->runqLinks.next);
	PR_ASSERT(THREAD_PTR(t->runqLinks.prev) == thread);
	LL_ADD(t->sleep, t->sleep, thread->sleep);
    }
}

/************************************************************************/

#ifndef XP_UNIX
void _PR_Idle(void)
{
    for (;;) {
    PRThread *t = PR_CurrentThread();
	_PR_FreeZombies();
	PR_LOG_BEGIN(SCHED, warn, ("idle: begin waiting for i/o"));
        _PR_Pause();
	PR_LOG_END(SCHED, warn, ("idle: end waiting for i/o"));
    }
}
#else
/* Look in prunix.c */
extern void _PR_Idle(void);
#endif

static PRBool initialized;

PR_PUBLIC_API(PRBool) PR_Initialized(void)
{
    return initialized;
}

/*
** Initialize the kernel
*/
PR_PUBLIC_API(void) PR_Init(char *name, int priority, int maxCpus, int flags_notused)
{
    PRThread *thread;
    int i;
    PRThreadStack *ts;

    if (initialized) return;
    initialized = PR_TRUE;

#ifdef PURIFY
    pure_init_threads();
#endif

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
    for (i = 0; i < 32; i++) {
	PR_INIT_CLIST(&_pr_runq[i]);
    }
    PR_INIT_CLIST(&_pr_suspendq);
    PR_INIT_CLIST(&_pr_sleepq);
    PR_INIT_CLIST(&_pr_zombieq);
    PR_INIT_CLIST(&_pr_monitorq);

    /* Create thread that represents the caller */
    ts = (PRThreadStack*) calloc(1, sizeof(PRThreadStack));
    PR_ASSERT(ts != 0);
#ifdef HAVE_STACK_GROWING_UP
    ts->stackTop = (char*)
	((((prword_t)&name) >> pr_pageShift) << pr_pageShift);
#else
#if defined(XP_PC) && !defined(_WIN32)
    _pr_top_of_task_stack = (char *) &name;
    ts->stackTop = (char *) &name;
    ts->stackSize = _MD_THREAD_STACK_SIZE;
#else
    ts->stackTop = (char*)
	((((prword_t)&name + pr_pageSize - 1) >> pr_pageShift) << pr_pageShift);
#endif
#endif
    thread = PR_AttachThread(name, priority, ts);
    PR_ASSERT(thread != 0);
    thread->state = _PR_RUNNING;
    thread->flags = 0;

    _pr_current_thread = thread;
    _pr_user_active++;

    /* Get misc locks now that enough init has been done */
    _pr_heap_lock = PR_NewNamedMonitor(0, "heap-lock");
    PR_ASSERT(_pr_heap_lock != 0);
    _pr_rusty_lock = PR_NewNamedMonitor(0, "rusty-lock");
    PR_ASSERT(_pr_rusty_lock != 0);
    _pr_dns_lock = PR_NewNamedMonitor(0, "dns-lock");
    PR_ASSERT(_pr_dns_lock != 0);
    _pr_child_intr = PR_NewNamedMonitor(0, "child-intr");
    PR_ASSERT(_pr_child_intr != 0);
    _pr_async_io_intr = PR_NewNamedMonitor(0, "async-io-intr");
    PR_ASSERT(_pr_async_io_intr != 0);
    _pr_TempName_lock = PR_NewNamedMonitor(0, "TempName-lock");
    PR_ASSERT(_pr_TempName_lock != 0);

    /* Get single/multi lock */
    _pr_single_lock = PR_NewNamedMonitor(0, "single-lock");
    PR_ASSERT(_pr_single_lock != 0);
    _PR_InitMonitorCache();
    _PR_InitIO();
    _PR_InitPrivateIDs();

    /* Create idle thread */
    _pr_idle = PR_CreateThread("idle", 0, 0);
    PR_ASSERT(_pr_idle != 0);
    _pr_idle->flags |= _PR_SYSTEM;
    if (PR_Start(_pr_idle, (void (*)(void*,void*))_PR_Idle, 0, 0) < 0) {
	fprintf(stderr, "PR_Init: can't start idle thread\n");
	PR_Abort();
    }

    /* Initialize current time and start the clock */
    _pr_now = PR_Now();
    _MD_StartClock();

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
#endif
