#if defined(XP_UNIX) && defined(HW_THREADS)
#include "prmem.h"
#include "prlog.h"
#include "hwkern.h"

#include <string.h>    /* strdup */


#if defined(IRIX)
#include <bstring.h>   /* FD_ZERO uses bzero */
#include <sys/time.h>  /* struct timeval */
#endif


PRCList _pr_all_monitors = PR_INIT_STATIC_CLIST(&_pr_all_monitors);


/*
** Wait for a notify/timeout.
**
** The sleep time is represented in microseconds...
*/
void _PR_CondWait(PRMonitor *mon, int64 sleep)
{
    PRThread *current = _pr_current_thread;
    unsigned long count = mon->notifycount, ltemp;
    int64 t0, t1, elapsed, lltemp, long_usec_per_sec, long_1000;
#if defined(SOLARIS) || defined(USE_PTHREADS)
    struct timespec tt;
#elif defined(IRIX)
    struct timeval tv, *tvp;
    unsigned long remaining;
    fd_set rd;
    int rv;
#endif
    PR_ASSERT( IS_SCHEDULER_LOCKED() );
    mon->pending++;
    UNLOCK_SCHEDULER();
    for(;;) {
#if defined(SOLARIS) || defined(USE_PTHREADS)
        /* the cond_wait below unlocks the mutex */
#elif defined(IRIX)
        /* Done below */
#endif
        t0 = PR_Now();

#if defined(SOLARIS) || defined(USE_PTHREADS)
        PR_LOG(MONITOR, out,
	       ("0x%x: event-condvar wait for 0x%x", PR_CurrentThread(), mon));
        if(LL_EQ(sleep, LL_MAXINT))
            COND_WAIT(mon->eventHandle, mon->mutexHandle);
        else {
            /* Longlongs are such a pain in the rear. */
            GETTIME(&tt);
            LL_UI2L(long_usec_per_sec, PR_USEC_PER_SEC);
            LL_DIV(lltemp, sleep, long_usec_per_sec);
            LL_L2UI(ltemp, lltemp);
            tt.tv_sec += ltemp;
            /* Prevent overflow, because under Solaris time_t is not unsigned */
            if(tt.tv_sec < 0) tt.tv_sec = 2147483647L;
            /* Convert the int64 microseconds into nanoseconds */
            LL_MOD(lltemp, sleep, long_usec_per_sec);
            LL_UI2L(long_1000, 1000L);
            LL_MUL(lltemp, lltemp, long_1000);
            LL_L2UI(ltemp, lltemp);
            tt.tv_nsec += ltemp;
            /* Prevent overflow, because under Solaris time_t is not unsigned */
            if(tt.tv_nsec < 0) tt.tv_nsec = 2147483647L;
            COND_TIMEDWAIT(mon->eventHandle, mon->mutexHandle, &tt);
        }
        PR_LOG(MONITOR, out,
	       ("0x%x: event-condvar wait done 0x%x", PR_CurrentThread(), mon));
#elif defined(IRIX)
        if(LL_EQ(sleep, LL_MAXINT))
            tvp = NULL;
        else {
            LL_L2UI(remaining, sleep);
            tv.tv_sec = remaining / PR_USEC_PER_SEC;
            tv.tv_usec = remaining % PR_USEC_PER_SEC;
            tvp = &tv;
        }
        FD_ZERO(&rd);
        FD_SET(mon->eventFD, &rd);
        /* 
           This may not actually get the sem, it just puts us on a queue.
           Doing this here avoids the race of unlocking the mutex, then
           going to sleep on the event (when someone may send the event 
           between these two actions). 
         */
        rv = uspsema(mon->eventHandle);
        MUTEX_UNLOCK(mon->mutexHandle);
        /* Select actually acquires the semaphore */
        if(!rv) select(mon->eventFD+1, &rd, NULL, NULL, tvp);
        MUTEX_LOCK(mon->mutexHandle);
#endif

        t1 = PR_Now();
        /* Do not go back to sleep if an exception is pending */
        if( current->pendingException ) {
            mon->pending -= 1;
            break;
        }

        if( mon->notifications > 0 && count != mon->notifycount ) {
            mon->pending -= 1;
            mon->notifications -= 1;
            break;
        }

        LL_SUB(elapsed, t1, t0);
        if ( LL_CMP( elapsed, >=, sleep ) ) {
            mon->pending -= 1;
            break;
        }
        LL_SUB( sleep, sleep, elapsed );
    }
}


/*
** Notify condition. 
*/
void _PR_CondNotify(PRMonitor *mon, int all)
{
    if( (mon->pending > mon->notifications) || (all && mon->pending > 0) ) {
        mon->notifications = all ? mon->pending : mon->notifications + 1;
        mon->notifycount++;
#if defined(IRIX)
        MUTEX_UNLOCK(mon->eventHandle);
#elif defined(SOLARIS) || defined(USE_PTHREADS)
        PR_LOG(MONITOR, out,
	       ("0x%x: entering mutex for 0x%x", PR_CurrentThread(), mon));
        MUTEX_LOCK(mon->mutexHandle);
        PR_LOG(MONITOR, out,
	       ("0x%x: signaling event for 0x%x", PR_CurrentThread(), mon));
        COND_SIGNAL(mon->eventHandle);
        PR_LOG(MONITOR, out,
	       ("0x%x: unlocking mutex for 0x%x", PR_CurrentThread(), mon));
        MUTEX_UNLOCK(mon->mutexHandle);
#endif
    }
}


PR_PUBLIC_API PRMonitor *PR_NewMonitor(int entryCount)
{
    PRMonitor *mon;

    mon = (PRMonitor*) calloc(1, sizeof(PRMonitor));
    if (mon) {
        PR_InitMonitor( mon, entryCount, "anon" );
        PR_LOG(MONITOR, out, ("new anonymous monitor: %s[%x]",
			      mon->name, mon));
    }
    return mon;
}

PR_PUBLIC_API PRMonitor *PR_NewNamedMonitor(int entryCount, char *name)
{
    PRMonitor *mon;

    mon = (PRMonitor*) calloc(1, sizeof(PRMonitor));
    if (mon) {
        PR_InitMonitor(mon, entryCount, name);
        PR_LOG(MONITOR, out, ("new monitor: %s[%x]", name, mon));
    }
    return mon;
}

PR_PUBLIC_API void PR_InitMonitor(PRMonitor *mon, int entryCount, char *name)
{
    if (mon->links.next == 0) {
        LOCK_SCHEDULER();
        PR_LOG(MONITOR, out,
               ("InitMonitor: 0x%x: count %d", mon, entryCount));

#if defined(IRIX)
        MUTEX_INIT(mon->mutexHandle);
        usctlsema(mon->mutexHandle, CS_RECURSIVEON);
        mon->eventHandle = usnewpollsema(_pr_irix_uarena, 1);
        /* Make sure first waiter gets blocked. */
        MUTEX_LOCK(mon->eventHandle);
        mon->eventFD = usopenpollsema(mon->eventHandle, 0666);

#elif defined(SOLARIS) || defined(USE_PTHREADS)
        MUTEX_INIT(mon->mutexHandle);
        COND_INIT(mon->eventHandle);
        COND_INIT(mon->enterCV);
        mon->num_blocked = 0;
#endif  /* XP_PC && WIN32 */

        mon->pending = 0;
        mon->name = strdup(name);
        PR_INIT_CLIST(&mon->links);
        PR_INIT_CLIST(&mon->lockQ);
        PR_APPEND_LINK(&mon->allLinks, &_pr_all_monitors);
        if (entryCount) {
            PRThread *thread = _pr_current_thread;

            PR_ASSERT( thread != NULL );
#if defined(IRIX)
            MUTEX_LOCK(mon->mutexHandle);
#elif defined(SOLARIS) || defined(USE_PTHREADS)
            /* mutexes are not recursive */
#endif
            mon->owner = thread;
            mon->count = entryCount;
            PR_APPEND_LINK(&mon->links, &thread->monitors);
            PR_LOG(MONITOR, out,
                   ("%s(0X%x): InitMonitor[entered] 0x%x count: %d", thread->name, 
		    thread, mon, mon->count));
        }

        UNLOCK_SCHEDULER();
    } else {
        PR_LOG(MONITOR, out,
               ("%s(0X%x): InitMonitor: 0x%x: avoided",
		_pr_current_thread->name, _pr_current_thread, mon));
    }
}

PR_PUBLIC_API void PR_EnterMonitor(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;

    PR_ASSERT(thread != NULL );

    LOCK_SCHEDULER(); 
    PR_LOG(MONITOR, out,
	   ("%s(0X%x): EnterMonitor[begin] 0x%x", thread->name, thread, mon));

    thread->monitor = mon;
    thread->monitorEntryCount = 1;
    thread->state = _PR_MON_WAIT;
    PR_APPEND_LINK(&thread->waitLinks, &mon->lockQ);
    UNLOCK_SCHEDULER(); 

#if defined(IRIX)
    /* Wait forever to get the monitor. */
    MUTEX_LOCK(mon->mutexHandle);
    /* This thread now owns the monitor */
    LOCK_SCHEDULER(); 
    thread->monitor = 0;
    thread->monitorEntryCount = 0;
    thread->state = _PR_RUNNING;
    PR_REMOVE_AND_INIT_LINK(&thread->waitLinks);
    
    if (mon->owner == thread) {
        mon->count++;
        MUTEX_UNLOCK(mon->mutexHandle);
    } else {
        mon->owner = thread;
        mon->count = 1;
        PR_APPEND_LINK(&mon->links, &thread->monitors);
    }
#elif defined(SOLARIS) || defined(USE_PTHREADS)
    /* Just wait for access to the data structure */
    PR_LOG(MONITOR, out,
	   ("0x%x: entering mutex for 0x%x", PR_CurrentThread(), mon));
    MUTEX_LOCK(mon->mutexHandle);
    if(mon->owner && (mon->owner != thread)) {
        ++mon->num_blocked;
        PR_LOG(MONITOR, out,
	       ("0x%x: enter-condvar wait for 0x%x", PR_CurrentThread(), mon));
        COND_WAIT(mon->enterCV, mon->mutexHandle);
        --mon->num_blocked;
    }
    /* Either we had the monitor, or just got it. */
    LOCK_SCHEDULER();
    thread->monitor = 0;
    thread->monitorEntryCount = 0;
    thread->state = _PR_RUNNING;
    PR_REMOVE_AND_INIT_LINK(&thread->waitLinks);

    if(!mon->owner) {
        mon->owner = thread;
        PR_APPEND_LINK(&mon->links, &thread->monitors);
        mon->count = 1;
    }
    else
        mon->count++;

    PR_LOG(MONITOR, out,
	   ("0x%x: unlocking mutex for 0x%x", PR_CurrentThread(), mon));
    MUTEX_UNLOCK(mon->mutexHandle);
#endif
    PR_LOG(MONITOR, out, 
           ("%s(0X%x): EnterMonitor[done] 0x%x count: %d", thread->name, 
	    thread, mon, mon->count));
    UNLOCK_SCHEDULER(); 
}

PR_PUBLIC_API int PR_ExitMonitor(PRMonitor *mon)
{
    int rv = 0;
    PRThread *thread = _pr_current_thread;

    LOCK_SCHEDULER();
#if defined(SOLARIS) || defined(USE_PTHREADS)
    PR_LOG(MONITOR, out,
	   ("0x%x: locking mutex for (exiting) 0x%x", PR_CurrentThread(), mon));
    MUTEX_LOCK(mon->mutexHandle);
#endif
    PR_ASSERT(thread != NULL );
    if (mon->owner != thread) {
        rv = -1;
        goto done;
    }

    --mon->count;
    PR_ASSERT(mon->count >= 0);
    PR_LOG(MONITOR, out,
           ("%s(0X%x): PR_ExitMonitor 0x%x count: %d", thread->name, 
	    thread, mon, mon->count));

    if (mon->count == 0) {
        /* Take monitor off of this threads list of monitors */
        PR_ASSERT(!PR_CLIST_IS_EMPTY(&mon->links));
        PR_REMOVE_AND_INIT_LINK(&mon->links);

        mon->owner = 0;
#if defined(IRIX)
        MUTEX_UNLOCK(mon->mutexHandle);
#elif defined(SOLARIS) || defined(USE_PTHREADS)
        /* mutex unlock is below */
        if(mon->num_blocked) {
            PR_LOG(MONITOR, out,
		   ("0x%x: enter-condvar signal for 0x%x", PR_CurrentThread(),
                    mon));
            COND_SIGNAL(mon->enterCV);
        }
#endif
    }


done:
#if defined(SOLARIS) || defined(USE_PTHREADS)
    PR_LOG(MONITOR, out,
	   ("0x%x: unlocking mutex for 0x%x", PR_CurrentThread(), mon));
    MUTEX_UNLOCK(mon->mutexHandle);
#endif
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API int PR_InMonitor(PRMonitor *mon)
{
    if (mon && (mon->owner == _pr_current_thread)) {
        return 1;
    }
    return 0;
}

/*
** Wait for a monitor to be notify'd
*/
PR_PUBLIC_API int PR_Wait(PRMonitor *mon, int64 sleep)
{
    PRThread *thread = _pr_current_thread;
    int rv = 0;
    int entryCount;

    PR_ASSERT(thread != NULL );

    LOCK_SCHEDULER();
    PR_LOG(MONITOR, out,
           ("%s(0X%x): CondWait 0x%x [for %lld]",
	    thread->name, thread, mon, sleep));
#if defined(SOLARIS) || defined(USE_PTHREADS)
    PR_LOG(MONITOR, out,
	   ("0x%x: locking mutex for 0x%x", PR_CurrentThread(), mon));
    MUTEX_LOCK(mon->mutexHandle);
#endif
    if (mon->owner != thread) {
        rv = -1;
        goto done;
    }
    /* Do not wait if an exception is pending for the thread... */
    if( thread->pendingException ) {
        goto done;
    }

    /* Deassign the monitor */
    PR_REMOVE_LINK(&mon->links);
    entryCount = mon->count;
    mon->count = 0;
    mon->owner = 0;

#if defined(SOLARIS) || defined(USE_PTHREADS)
    if(mon->num_blocked) {
        PR_LOG(MONITOR, out,
	       ("0x%x: enter-condvar signal for 0x%x", PR_CurrentThread(),
                mon));
        COND_SIGNAL(mon->enterCV);
    }
#endif

    /* Give up the monitor to the highest priority waiter (if any) */
    thread->state = _PR_COND_WAIT;
    thread->monitor = mon;
    thread->monitorEntryCount = entryCount;

    /* if the sleep time is negative then make it a big positive value */
    if (!LL_GE_ZERO(sleep)) {
        LL_USHR(sleep, sleep, 1);
    }

    _PR_CondWait( mon, sleep );     /* CondWait releases the scheduler lock */
    /* Unlike NT hwmon.c, the scheduler is NOT locked here. This is to avoid
       deadlocking where the waiting thread is holding the monitor mutex,
       and waits to lock the scheduler... while another thread locks the 
       scheduler and wants to release the monitor mutex.
     */
    PR_ASSERT(!IS_SCHEDULER_LOCKED());

    /* Under Solaris and Posix threads, we can own the mutex right now but
       not the monitor. */
#if defined(SOLARIS) || defined(USE_PTHREADS)
    if(mon->owner && (mon->owner != thread)) {
        ++mon->num_blocked;
        PR_LOG(MONITOR, out,
	       ("0x%x: enter-condvar wait for 0x%x", PR_CurrentThread(), mon));
        COND_WAIT(mon->enterCV, mon->mutexHandle);
        --mon->num_blocked;
    }
#endif

    /* This thread now owns the monitor. */

    thread->monitor = 0;
    thread->monitorEntryCount = 0;
    thread->state = _PR_RUNNING;

    /* reset the state of the monitor */
    mon->owner = thread;
    mon->count = entryCount;
    PR_APPEND_LINK(&mon->links, &thread->monitors);
    PR_LOG(MONITOR, out,
           ("%s(0X%x): CondWait 0x%x done", thread->name, thread, mon));

#if defined(SOLARIS) || defined(USE_PTHREADS)
    PR_LOG(MONITOR, out,
	   ("0x%x: unlocking mutex for 0x%x", PR_CurrentThread(), mon));
    MUTEX_UNLOCK(mon->mutexHandle);
#endif
    return rv;

  done:
    UNLOCK_SCHEDULER();

    return rv;
}

PR_PUBLIC_API int PR_Notify(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;
    int rv;

    /*
    ** We must own the monitor underneath the condition variable to
    ** manipulate it.
    */
    PR_ASSERT(thread != NULL );

    LOCK_SCHEDULER();
    PR_LOG(MONITOR, out,
           ("%s(0X%x): CondNotify: 0x%x: count %d", thread->name, thread, 
	    mon, mon->count));
    if (mon->owner != thread) {
        rv = -1;
        goto done;
    }

    /* Wakeup first runnable thread waiting */
    _PR_CondNotify(mon, 0);
    rv = 0;

  done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API int PR_NotifyAll(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;
    int rv;

    /*
    ** We must own the monitor underneath the condition variable to
    ** manipulate it.
    */
    PR_ASSERT(thread != NULL );

    LOCK_SCHEDULER();
    PR_LOG(MONITOR, out, 
           ("%s(0X%x): CondNotifyAll 0x%x", thread->name, thread, mon));
    if (mon->owner != thread) {
        rv = -1;
        goto done;
    }

    /* Wakeup first runnable thread waiting */
    _PR_CondNotify(mon, 1);
    rv = 0;

  done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API void PR_DestroyMonitor(PRMonitor *mon)
{
    PR_ASSERT(_pr_current_thread != NULL );
    if (mon) {
        LOCK_SCHEDULER();
        
        PR_ASSERT(mon->owner == 0);
        PR_ASSERT(mon->count == 0);
        PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->links));
        PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->lockQ));

        /* Remove from the list of all monitors */
        PR_REMOVE_LINK(&mon->allLinks);

#if defined(IRIX)
        MUTEX_DESTROY(mon->mutexHandle);
        close(mon->eventFD);
        MUTEX_DESTROY(mon->eventHandle);
#elif defined(SOLARIS) || defined(USE_PTHREADS)
        MUTEX_DESTROY(mon->mutexHandle);
        COND_DESTROY(mon->eventHandle);
#endif
        UNLOCK_SCHEDULER();

        /* Free the memory for the monitor structure */
        free(mon);
    }
}

/************************************************************************/
/*
** Interface to selectable monitors.
*/

PR_PUBLIC_API int
PR_MakeMonitorSelectable(PRMonitor* mon)
{
    /* All monitors are "selectable" */
    return 0;
}

PR_PUBLIC_API void
PR_SelectNotify(PRMonitor* mon)
{
}

PR_PUBLIC_API void
PR_ClearSelectNotify(PRMonitor* mon)
{
    /* Currently, there is no way to clear a notification */
}

/************************************************************************/

#endif  /* HW_THREADS */
