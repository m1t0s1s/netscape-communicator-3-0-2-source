#ifndef prmon_h___
#define prmon_h___

/*
** API to NSPR monitor's (modeled after java monitors which have a
** condition variable folded into the monitor)
*/
#include "prclist.h"
#include "prlong.h"
#include "prmacros.h"

NSPR_BEGIN_EXTERN_C

/*
** Create a new monitor.
**      "entryCount" the number of times that monitor should be entered during
**         creation.
**
** This may fail if memory is tight or if some operating system resource
** is low. For PR_NewNamedMonitor the string is duplicated.
*/
extern PR_PUBLIC_API(PRMonitor *) PR_NewMonitor(int entryCount);
extern PR_PUBLIC_API(PRMonitor *) PR_NewNamedMonitor(int entryCount, char *name);

/*
** Like PR_NewMonitor, except that the caller has allocated the storage
** for the monitor. This performs the init routines needed to make the
** monitor real. The string is duplicated.
*/
extern PR_PUBLIC_API(void) PR_InitMonitor(PRMonitor *mon, int entryCount,
                                         char *name);

/*
** Destroy a monitor. The monitor's entry count must be XXX
*/
extern PR_PUBLIC_API(void) PR_DestroyMonitor(PRMonitor *mon);

/*
** Enter a monitor. The calling thread will block indefinitely until the
** monitor is available. The calling thread can nest calls to the monitor
** without blocking.
*/
extern PR_PUBLIC_API(void) PR_EnterMonitor(PRMonitor *mon);

/*
** Returns non-zero if we have the monitor, zero otherwise.
*/
extern PR_PUBLIC_API(int) PR_InMonitor(PRMonitor *mon);

/*
** Exit a monitor. Reduce the count of how many times the calling thread
** has entered the monitor by one, and if zero makes the monitor
** available for another thread. If another thread is waiting for the
** monitor then the other thread will get a chance to run (if it is of
** higher priority than the current thread then it will run).
**
** Returns:
**  -1: if the caller has not entered the monitor,
**   0: otherwise.
*/
extern PR_PUBLIC_API(int) PR_ExitMonitor(PRMonitor *mon);

/*
** Wait for a notify on the condition variable. Sleep for "sleep" amount
** of time. While the thread is waiting it loses the monitor (as if it
** called PR_ExitMonitor as many times as it had called PR_EnterMonitor).
** When the wait has finished the thread regains control of the monitor
** with the same entry count as before the wait began.
**
** Returns:
**  -1: if the caller has not entered the monitor,
**   0: otherwise.
*/
extern PR_PUBLIC_API(int) PR_Wait(PRMonitor *mon, int64 sleep);

/*
** Notify the highest priority waiters on the condition variable. If a
** thread is waiting on the condition variable (using PR_Wait) then it is
** awakened and begins waiting on the monitor.
**
** Returns:
**  -1: if the caller has not entered the monitor,
**   0: otherwise.
**
** XXX is it really the highest priority waiter?
*/
extern PR_PUBLIC_API(int) PR_Notify(PRMonitor *mon);

/*
** Notify all of the threads waiting on the condition variable.
**
** Returns:
**  -1: if the caller has not entered the monitor,
**   0: otherwise.
*/
extern PR_PUBLIC_API(int) PR_NotifyAll(PRMonitor *mon);

#if defined(XP_UNIX)
/*
** Global lock variable used to bracket calls into rusty libraries that
** aren't thread safe (like libc, libX, etc).
*/
extern PRMonitor *_pr_rusty_lock;

extern void PR_XLock(void);
extern void PR_XUnlock(void);
extern int  PR_XIsLocked(void);
#endif /* XP_UNIX */

/************************************************************************/
/*
** Interface to monitor cache.
*/

extern PR_PUBLIC_API(int) PR_CEnterMonitor(void *address);
extern PR_PUBLIC_API(int) PR_CExitMonitor(void *address);
extern PR_PUBLIC_API(int) PR_CWait(void *address, int64 howlong);
extern PR_PUBLIC_API(int) PR_CNotify(void *address);
extern PR_PUBLIC_API(int) PR_CNotifyAll(void *address);

/************************************************************************/
/*
** Interface to selectable monitors.
*/

extern PR_PUBLIC_API(int) PR_MakeMonitorSelectable(PRMonitor* mon);

extern PR_PUBLIC_API(void) PR_SelectNotify(PRMonitor* mon);

extern PR_PUBLIC_API(void) PR_ClearSelectNotify(PRMonitor* mon);

#ifdef XP_UNIX

extern PR_PUBLIC_API(int) PR_GetSelectFD(PRMonitor* mon);

#else /* XP_MAC */

/* #error Mac Guys */

#endif

/************************************************************************/

struct PRMonitorStr {
    PRCList links;

    PRCList condQ;

    PRCList lockQ;

    PRCList allLinks;           /* linkage for list of all monitors */

    PRThread *owner;
    int count;
    int oldPriority;

    int flags;

    char *name;

    int eventPipe[2];

    int stickyCount;            /* sticky notify count */

#ifdef HW_THREADS
#if defined(XP_PC) && defined(_WIN32)
    CRITICAL_SECTION mutexHandle;       /* Handle to a critical section      */
#endif
#endif  /* HW_THREADS */
};

/* monitor->flags */
#define _PR_HAS_EVENT_PIPE      0x1

/*
** Macros used to get the monitor pointer from a PRCList link. These
** macros handle the fact that the linkage cells are not always at the
** start of the monitor struct.
*/
#define MONITOR_PTR(_qp) \
    ((PRMonitor*) ((char*) (_qp) - offsetof(PRMonitor,links)))

#define MONITOR_ALL_PTR(_qp) \
    ((PRMonitor*) ((char*) (_qp) - offsetof(PRMonitor,allLinks)))

NSPR_END_EXTERN_C

#endif /* prmon_h___ */

