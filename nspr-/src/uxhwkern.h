#ifndef nspr_uxhwkern_h___
#define nspr_uxhwkern_h___

#include <nspr/prclist.h>
#include <nspr/prthread.h>
#include <nspr/prmon.h>
#include <nspr/prlong.h>
#include <stddef.h>

#ifdef SOLARIS
#define THREAD_KEY_T thread_key_t
#define THR_GETSPECIFIC(key, val) thr_getspecific(key, (void **) val)
#define THR_SETSPECIFIC(key, val) thr_setspecific(key, (void *) val)
#define THR_KEYCREATE thr_keycreate
#define THR_SELF thr_self
#define MUTEX_T mutex_t
#define MUTEX_INIT(mutex) mutex_init(&(mutex), USYNC_THREAD, NULL)
#define MUTEX_LOCK(mutex) mutex_lock(&(mutex))
#define MUTEX_UNLOCK(mutex) mutex_unlock(&(mutex))
#define MUTEX_DESTROY(mutex) mutex_destroy(&(mutex))
#define COND_T cond_t
#define COND_INIT(cond) cond_init(&(cond), USYNC_THREAD, 0)
#define COND_WAIT(cond, mutex) cond_wait(&(cond), &(mutex))
#define COND_TIMEDWAIT(cond, mutex, tspec) \
                                     cond_timedwait(&(cond), &(mutex), tspec)
#define COND_SIGNAL(cond) cond_signal(&(cond))
#define COND_DESTROY(cond) cond_destroy(&(cond))
#define YIELD() thr_yield()
#include <time.h>
#define GETTIME(tt) clock_gettime(CLOCK_REALTIME, (tt))


#elif defined(USE_PTHREADS)
#define THREAD_KEY_T pthread_key_t
#define THR_GETSPECIFIC(key, val) \
                       pthread_getspecific(key, (pthread_addr_t *) val)
#define THR_SETSPECIFIC(key, val) \
                       pthread_setspecific(key, (pthread_addr_t) val)
#define THR_KEYCREATE pthread_keycreate
#define THR_SELF pthread_self
#define MUTEX_T pthread_mutex_t
#define MUTEX_INIT(mutex) \
                       pthread_mutex_init(&(mutex), pthread_mutexattr_default)
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&(mutex))
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&(mutex))
#define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&(mutex))
#define COND_T pthread_cond_t
#define COND_INIT(cond) pthread_cond_init(&(cond), pthread_condattr_default)
#define COND_WAIT(cond, mutex) pthread_cond_wait(&(cond), &(mutex))
#define COND_TIMEDWAIT(cond, mutex, tspec) \
                       pthread_cond_timedwait(&(cond), &(mutex), tspec)
#define COND_SIGNAL(cond) pthread_cond_signal(&(cond))
#define COND_DESTROY(cond) pthread_cond_destroy(&(cond))
#define YIELD() pthread_yield()
/* XXXrobm for some reason, defining POSIX_4D9 and including timers.h doesn't 
   work */
#define TIMEOFDAY 1
#define GETTIME(tt) getclock(TIMEOFDAY, (tt))

#elif defined(IRIX)

extern usptr_t *_pr_irix_uarena;

/* 
   TLS stuff is defined in md_IRIX.c. It assumes only two keys will ever
   be necessary.
 */
#define THREAD_KEY_T int
#define THR_GETSPECIFIC(key, val) \
                       _irix_tls_get(key, (void **) val)
extern void _irix_tls_get(int key, void **val);
#define THR_SETSPECIFIC(key, val) \
                       _irix_tls_set(key, (void *) val)
extern void _irix_tls_set(int key, void *val);
#define THR_KEYCREATE _irix_tls_keycreate
extern void _irix_tls_keycreate(int *key, void *unused);
extern void _irix_tls_init(void);
extern void _irix_tls_freedata(void);

#define THR_SELF getpid
#define MUTEX_T usema_t*
#define MUTEX_INIT(mutex) ((mutex) = usnewsema(_pr_irix_uarena, 1))
#define MUTEX_LOCK uspsema
#define MUTEX_UNLOCK usvsema
#define MUTEX_DESTROY(mutex) usfreesema(mutex, _pr_irix_uarena)

#define COND_T void*
#define COND_INIT(cond) (cond) = _irix_cond_init();
#define COND_WAIT(cond, mutex) _irix_cond_wait(cond, mutex)
#define COND_TIMEDWAIT(cond, mutex, tspec) \
                       _irix_cond_timedwait(cond, mutex, tspec)
#define COND_SIGNAL(cond) _irix_cond_signal(cond)
#define COND_DESTROY(cond) _irix_cond_destroy(cond)

#define YIELD() sginap(0)

#endif


extern PRCList _pr_runq[32];
extern PRCList _pr_all_monitors;

extern int _pr_user_active;
extern int _pr_system_active;

extern PRThread *_pr_idle;
extern PRThread *_pr_main_thread;
extern PRMonitor *_pr_heap_lock;
extern PRMonitor *_pr_dns_lock;


/*
** Thread local data structures...
*/
extern int _pr_intsOff_tls(int op, int newval);
extern PRThread *_pr_current_thread_tls(int op, PRThread *newval);
extern MUTEX_T _pr_schedLock;
#define _pr_intsOff _pr_intsOff_tls(0, 0)
#define PR_SET_INTSOFF(newval) _pr_intsOff_tls(1, newval)
#define PR_INC_INTSOFF() _pr_intsOff_tls(2, 0)
#define PR_DEC_INTSOFF() _pr_intsOff_tls(3, 0)

#define _pr_current_thread _pr_current_thread_tls(0, 0)
#define PR_SET_CURRENT_THREAD(newval) _pr_current_thread_tls(1, newval)


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

#define LOCK_SCHEDULER()						    \
    if( _pr_intsOff == 0 ) {						    \
        PR_LOG(MONITOR, out,						    \
	       ("0x%x: locking scheduler mutex", PR_CurrentThread()));	    \
        MUTEX_LOCK( _pr_schedLock );					    \
        PR_LOG_BEGIN(MONITOR, out,					    \
		     ("0x%x: locked scheduler mutex", PR_CurrentThread())); \
    }									    \
    PR_INC_INTSOFF();							    \

#define UNLOCK_SCHEDULER()						    \
    PR_ASSERT( _pr_intsOff != 0 );					    \
    if( PR_DEC_INTSOFF() == 0 ) {					    \
        PR_LOG_END(MONITOR, out,					    \
		   ("0x%x: unlocking scheduler", PR_CurrentThread()));	    \
        MUTEX_UNLOCK( _pr_schedLock );					    \
    }									    \

#define IS_SCHEDULER_LOCKED() (_pr_intsOff != 0)


/*
** Returns non-zero if an exception is pending for this thread
*/

#define _PR_PENDING_EXCEPTION()	  ( 0 )


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
