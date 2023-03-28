#ifndef swkern_h___
#define swkern_h___

#include <errno.h>
#include <stddef.h>
#include "prclist.h"
#include "prthread.h"
#include "prmon.h"
#include "prlong.h"

NSPR_BEGIN_EXTERN_C

extern PRCList _pr_runq[32];
extern PRCList _pr_sleepq;
extern PRCList _pr_zombieq;
extern PRCList _pr_suspendq;
extern PRCList _pr_lockq;
extern PRCList _pr_monitorq;
extern PRCList _pr_all_monitors;

extern int32 _pr_runq_ready_mask;
extern int64 _pr_now;
extern int _pr_user_active;
extern int _pr_system_active;
extern PRThread *_pr_current_thread;
extern PRThread *_pr_idle;
extern PRMonitor *_pr_heap_lock;
extern PRMonitor *_pr_single_lock;
extern PRMonitor *_pr_child_intr;
extern PRMonitor *_pr_async_io_intr;

extern int _pr_intsOff;

#if defined(XP_PC) && !defined(_WIN32)
extern char *_pr_top_of_task_stack;
#endif

typedef enum _PRNotifyEnum {
    _PR_NOTIFY_ONE,
    _PR_NOTIFY_ALL,
    _PR_NOTIFY_STICKY
} _PRNotifyEnum;

extern void _PR_Schedule(void);
extern int _PR_IntsOff(void);
extern int _PR_IntsOn(int status, int wantYield);
extern void _PR_SwitchThread(PRThread *t);
extern int _PR_WakeupThread(PRThread *t);
extern void _PR_DestroyThread(PRThread *t);
extern void _PR_FreeZombies(void);
extern void _PR_PutOnSleepQ(PRThread *t, int64 sleep);
extern void _PR_FixSleepQ(PRThread *thread);
extern int _PR_SetThreadPriority(PRThread* thread, int priority);

extern void _PR_MonWait(PRMonitor *mon);
extern int _PR_MonNotify(PRMonitor *mon, PRThread* oldOwner);
extern void _PR_CondWait(PRMonitor *mon, int64 sleep);
extern int _PR_CondNotify(PRMonitor *m, _PRNotifyEnum how);
extern void _PR_NotifyOneThread(PRMonitor *m, PRThread *t);

extern int _PR_ClockTick(void);
extern void _PR_Pause(void);
extern void _PR_InitIO(void);
extern void _PR_InitMonitorCache(void);

extern void _PR_ClockInterruptHandler(void);
extern void _PR_ChildDeathInterruptHandler(void);
extern void _PR_AsyncIOInterruptHandler(void);

/*
** Returns non-zero if an exception is pending for this thread
*/
#define _PR_PENDING_EXCEPTION()	\
    (_pr_current_thread->pendingException != 0)

/*
** Disable all other threads from entering a critical section of code
** for a (hopefully) brief period of time.
*/
#define _PR_BEGIN_CRITICAL_SECTION(_s) \
    (_s) = _PR_IntsOff()

/*
** Enable all other threads to run again
*/
#define _PR_END_CRITICAL_SECTION(_s) \
    _PR_IntsOn((_s), 0)

/*
** The thread enumeration code is not re-entrant, nor can other parts
** of the kernel be invoked while it is in progress.
*/
#ifdef DEBUG
extern int _pr_scanning_threads;

#define PR_BEGIN_SCANNING_THREADS() \
    _pr_scanning_threads++; PR_ASSERT(_pr_scanning_threads == 1)

#define PR_END_SCANNING_THREADS() \
    _pr_scanning_threads--; PR_ASSERT(_pr_scanning_threads == 0)

#else

#define PR_BEGIN_SCANNING_THREADS()
#define PR_END_SCANNING_THREADS()

#endif /* DEBUG */

NSPR_END_EXTERN_C

#endif /* swkern_h___ */
