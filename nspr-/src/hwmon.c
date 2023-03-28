#ifdef HW_THREADS
#include "prmem.h"
#include "prlog.h"
#include "hwkern.h"
#include "mdint.h"

PRCList _pr_all_monitors = PR_INIT_STATIC_CLIST(&_pr_all_monitors);

/*
** Wait for a notify/timeout.
**
** The sleep time is represented in microseconds...
*/
void _PR_CondWait(PRMonitor *mon, int64 sleep)
{
    PRThread *current = _pr_current_thread;
    unsigned long waitTime;

    PR_ASSERT( IS_SCHEDULER_LOCKED() );

    /*
     * Update misc fields in the monitor and thread to indicate
     * the new state...
     */
    current->sleep = sleep;

    /*
     * Place the thread on the CondQ of the monitor.  This queue
     * is traversed when a Notify occurs...
     */
    PR_APPEND_LINK(&current->waitLinks, &mon->condQ);

    /*
     * Convert the sleep time from an int64 in microseconds to
     * an int32 in milliseconds
     */
    LL_L2UI( waitTime, sleep );
    waitTime = waitTime / PR_USEC_PER_MSEC;

    UNLOCK_SCHEDULER();

    /*
     * Give up the monitor and wait until one of the following
     * occurs:
     *     1. The thread is notified via PR_Notify...
     *     2. The sleep time is reached.
     *     3. The thread receives an exception
     */
    LeaveCriticalSection( &(mon->mutexHandle) );
    if (WaitForSingleObject(current->condEvent, waitTime) == WAIT_TIMEOUT) {
        PR_REMOVE_AND_INIT_LINK(&current->waitLinks); /* remove from condQ */
    }

    /* Verify that the thread is no longer on any monitor condQ */
    PR_ASSERT(PR_CLIST_IS_EMPTY(&current->waitLinks));

    /* Time to wake up... Acquire the monitor again. */
    EnterCriticalSection( &(mon->mutexHandle) );

    LOCK_SCHEDULER();
}

/*
** Notify condition.
*/
void _PR_CondNotify(PRMonitor *mon, int all)
{
    PRThread *winner;

    while (!PR_CLIST_IS_EMPTY(&mon->condQ)) {
        /*
        ** Get waiting thread from the condQ.
        */
        winner = WAITER_PTR(mon->condQ.next);

        /*
        ** Notify the winning thread.  This effectively causes the
        ** thread to try and acquire the monitor again (see _PR_CondWait)...
        */
        PR_REMOVE_AND_INIT_LINK(&winner->waitLinks); /* remove from condQ */
        if ( SetEvent(winner->condEvent) ) {
            PR_LOG(MONITOR, out,
                   ("%s(0X%x): CondNotify 0x%x",
                    mon->owner->name, mon->owner, mon));
        } else {
            PR_LOG(MONITOR, out,
                   ("CondNotify: Cannot signal event object."));
        }

        /*
        ** Continue examining the condQ if the thread is suspended or if
        ** we are notifying all threads. Suspended threads don't count
        ** because the don't get the monitor until they are resumed.
        */
        if (((winner->flags & _PR_SUSPENDING) == 0) && !all) {
            break;
        }
    }
}

PR_PUBLIC_API(PRMonitor *) PR_NewMonitor(int entryCount)
{
    PRMonitor *mon;

    mon = (PRMonitor*) malloc(sizeof(PRMonitor));
    if (mon) {
        mon->links.next = 0;
        PR_InitMonitor( mon, entryCount, "anon" );
        PR_LOG(MONITOR, out,
               ("new anonymous monitor: %s[%x]", mon->name, mon));
    }
    return mon;
}

PR_PUBLIC_API(PRMonitor *) PR_NewNamedMonitor(int entryCount, char *name)
{
    PRMonitor *mon;

    mon = (PRMonitor*) malloc(sizeof(PRMonitor));
    if (mon) {
        mon->links.next = 0;
        PR_InitMonitor(mon, entryCount, name);
        PR_LOG(MONITOR, out, ("new monitor: %s[%x]", name, mon));
    }
    return mon;
}

PR_PUBLIC_API(void) PR_InitMonitor(PRMonitor *mon, int entryCount, char *name)
{
    if (mon->links.next == 0) {
        /* Make sure *all* fields get initialized */
        memset(mon, 0, sizeof(PRMonitor));

        PR_LOG(MONITOR, out,
               ("InitMonitor: 0x%x: count %d", mon, entryCount));

#if defined( XP_PC ) && defined( _WIN32 )
        /* create an OS Critical Section for syncronization */
        InitializeCriticalSection( &(mon->mutexHandle) );
#endif  /* XP_PC && _WIN32 */

        mon->name = strdup(name);
        PR_INIT_CLIST(&mon->links);
        PR_INIT_CLIST(&mon->condQ);
        PR_INIT_CLIST(&mon->lockQ);

        LOCK_SCHEDULER();
        PR_APPEND_LINK(&mon->allLinks, &_pr_all_monitors);
        UNLOCK_SCHEDULER();

        if (entryCount) {
            PRThread *thread = _pr_current_thread;

            PR_ASSERT( thread != NULL );
            EnterCriticalSection( &(mon->mutexHandle) );
            mon->owner = thread;
            mon->count = entryCount;
#ifdef DEBUG
            PR_APPEND_LINK(&mon->links, &thread->monitors);
#endif
            PR_LOG(MONITOR, out,
                   ("%s(0X%x): InitMonitor[entered] 0x%x count: %d",
                    thread->name, thread, mon, mon->count));
        }
    } else {
        PR_LOG(MONITOR, out,
               ("%s(0X%x): InitMonitor: 0x%x: avoided",
                _pr_current_thread->name, _pr_current_thread, mon));
    }
}

PR_PUBLIC_API(void) PR_EnterMonitor(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;

    PR_ASSERT(thread != NULL );
    PR_LOG(MONITOR, out,
           ("%s(0X%x): EnterMonitor[begin] 0x%x", thread->name, thread, mon));

    if (mon->owner == thread) {
        /* Just advance the entry count since we already own the monitor */
        mon->count++;
    } else {
        /* Wait for the other thread (if any) to exit the monitor */
        thread->state = _PR_MON_WAIT;
	thread->monitor = mon;	/* used during PR_Thread_print */
        EnterCriticalSection(&mon->mutexHandle);
#ifdef DEBUG
        PR_APPEND_LINK(&mon->links, &thread->monitors);
#endif
        thread->state = _PR_RUNNING;
        mon->owner = thread;
        mon->count = 1;
    }
}

PR_PUBLIC_API(int) PR_ExitMonitor(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;

    PR_ASSERT(thread != NULL );
    PR_ASSERT(mon->count > 0);
    PR_LOG(MONITOR, out,
           ("%s(0X%x): PR_ExitMonitor 0x%x count: %d", thread->name,
            thread, mon, mon->count));

    if (mon->owner != thread) {
        return -1;
    }
    if (--mon->count == 0) {
        mon->owner = 0;
#ifdef DEBUG
        PR_REMOVE_AND_INIT_LINK(&mon->links);
#endif
        LeaveCriticalSection(&mon->mutexHandle);
    }
    return 0;
}

PR_PUBLIC_API(int) PR_InMonitor(PRMonitor *mon)
{
    if (mon && (mon->owner == _pr_current_thread)) {
        return 1;
    }
    return 0;
}

/*
** Wait for a monitor to be notify'd
*/
PR_PUBLIC_API(int) PR_Wait(PRMonitor *mon, int64 sleep)
{
    PRThread *thread = _pr_current_thread;
    int rv = 0;
    int entryCount;

    PR_ASSERT(thread != NULL );

    LOCK_SCHEDULER();
    PR_LOG(MONITOR, out,
           ("%s(0X%x): CondWait 0x%x [for %lld]",
            thread->name, thread, mon, sleep));
    if (mon->owner != thread) {
        rv = -1;
        goto done;
    }
    /* Do not wait if an exception is pending for the thread... */
    if( thread->pendingException ) {
        goto done;
    }

    /* Deassign the monitor (_PR_CondWait will Leave the critical
       section which is where it will actually become available to
       other threads) */
#ifdef DEBUG
    PR_REMOVE_LINK(&mon->links);
#endif
    entryCount = mon->count;
    mon->count = 0;
    mon->owner = 0;

    /* if the sleep time is negative then make it a big positive value */
    if (!LL_GE_ZERO(sleep)) {
        LL_USHR(sleep, sleep, 1);
    }

    thread->state = _PR_COND_WAIT;
    thread->monitor = mon;	/* used during PR_Thread_print */
    _PR_CondWait( mon, sleep );     /* CondWait releases the scheduler lock */
    thread->state = _PR_RUNNING;

    /* reset the state of the monitor */
    mon->owner = thread;
    mon->count = entryCount;
#ifdef DEBUG
    PR_APPEND_LINK(&mon->links, &thread->monitors);
#endif

    PR_LOG(MONITOR, out,
           ("%s(0X%x): CondWait 0x%x done", thread->name, thread, mon));

  done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(int) PR_Notify(PRMonitor *mon)
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
    thread->monitor = 0;	/* used during PR_Thread_print */
    rv = 0;

  done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(int) PR_NotifyAll(PRMonitor *mon)
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
    thread->monitor = 0;	/* used during PR_Thread_print */
    rv = 0;

  done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(void) PR_DestroyMonitor(PRMonitor *mon)
{
    PR_ASSERT(_pr_current_thread != NULL );
    if (mon) {
        PR_ASSERT(mon->owner == 0);
        PR_ASSERT(mon->count == 0);
        PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->links));
        PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->lockQ));

        /* Remove from the list of all monitors */
        LOCK_SCHEDULER();
        PR_REMOVE_LINK(&mon->allLinks);
        UNLOCK_SCHEDULER();

        /* Free the memory for the monitor name */
        if( mon->name ) {
            free(mon->name);
            mon->name = 0;
        }
        /* Release the underlying OS syncronization objects */
        DeleteCriticalSection( &(mon->mutexHandle) );

        /* Free the memory for the monitor structure */
        free(mon);
    }
}

/************************************************************************/
/*
** Interface to selectable monitors.
*/

PR_PUBLIC_API(int)
PR_MakeMonitorSelectable(PRMonitor* mon)
{
    /* All monitors are "selectable" */
    return 0;
}

PR_PUBLIC_API(void)
PR_SelectNotify(PRMonitor* mon)
{
#ifdef XP_PC
    /*
    ** Post a message to the main thread to requesting it to process
    ** the pending events
    */
    PostThreadMessage( _pr_main_thread->id, _pr_PostEventMsgId,
                      (WPARAM)0, (LPARAM)mon);
#endif
}

PR_PUBLIC_API(void)
PR_ClearSelectNotify(PRMonitor* mon)
{
    /* Currently, there is no way to clear a notification */
}

/************************************************************************/

#endif  /* HW_THREADS */

