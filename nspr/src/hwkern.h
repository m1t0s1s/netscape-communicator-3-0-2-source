#ifndef nspr_hwkern_h___
#define nspr_hwkern_h___

#include <prclist.h>
#include <prthread.h>
#include <prmon.h>
#include <prlong.h>
#include <stddef.h>

extern PRCList _pr_runq[32];
extern PRCList _pr_all_monitors;

extern int _pr_user_active;
extern int _pr_system_active;

extern PRThread *_pr_idle;
extern PRThread *_pr_main_thread;
extern PRMonitor *_pr_heap_lock;
extern PRMonitor *_pr_dns_lock;

#ifdef WIN32
extern CRITICAL_SECTION _pr_schedLock;
#endif

/*
** Thread local data structures...
*/
#ifdef WIN32
extern __declspec( thread ) int         _pr_intsOff;
extern __declspec( thread ) PRThread *  _pr_current_thread;
#endif  /* WIN32 */

extern int  _PR_IntsOff(void);
extern int  _PR_IntsOn(int status, int wantYield);
extern void _PR_InitIO(void);
extern void _PR_InitMonitorCache(void);

/*
** Macros for locking and unlocking the scheduler...
**
** _pr_intsOff is a tls variable which is ONLY used as a flag
** to determine if a particular thread has locked the scheduler.
*/

#if defined( XP_PC ) && defined( WIN32 )
#define LOCK_SCHEDULER()                            \
    if( _pr_intsOff == 0 ) {                        \
        EnterCriticalSection( &_pr_schedLock );     \
        _pr_intsOff++;                              \
    } else {                                        \
        _pr_intsOff++;                              \
    }

#define UNLOCK_SCHEDULER()                          \
    PR_ASSERT( _pr_intsOff != 0 );                  \
    if( --_pr_intsOff == 0 ) {                      \
        LeaveCriticalSection( &_pr_schedLock );     \
    }

#define IS_SCHEDULER_LOCKED() (_pr_intsOff != 0)
#endif  /* XP_PC && WIN32 */

/*
** Returns non-zero if an exception is pending for this thread
*/
#define _PR_PENDING_EXCEPTION() \
    ( FALSE )

/*
** Disable all other threads from entering a critical section of code
** for a (hopefully) brief period of time.
*/
#define _PR_BEGIN_CRITICAL_SECTION(_s)  \
    LOCK_SCHEDULER();                   \
    (_s) = _pr_intsOff;

/*
** Enable all other threads to run again
*/
#define _PR_END_CRITICAL_SECTION(_s)    \
    UNLOCK_SCHEDULER();

#endif  /* nspr_hwkern_h__ */

