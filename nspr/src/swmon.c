#ifdef SW_THREADS
#include "prmem.h"
#include "prlog.h"
#include "mdint.h"
#include "swkern.h"
#include <string.h>
#include "prdump.h"

#ifdef XP_MAC
#include <Types.h>
#include <Events.h>
extern long gInTimerCallback;
#endif

PRCList _pr_all_monitors = PR_INIT_STATIC_CLIST(&_pr_all_monitors);

/*
** InvertPriority: Thread is the thread that wants the monitor.
*/
void
_PR_InvertPriority(PRMonitor* mon, PRThread* thread)
{
    PRThread* owner = mon->owner;
    int ownerPri = owner->priority;
    int newPri = thread->priority;

    PR_ASSERT(_pr_intsOff != 0);
    if (ownerPri < newPri) {
	if (mon->oldPriority == -1) {
	    mon->oldPriority = ownerPri;
	}
	/*
	** Ignore whether we need to yield here, because we're going to
	** anyway:
	*/
	(void)_PR_SetThreadPriority(owner, newPri);
#if 0
	fprintf(stderr, "*** Setting priority to %d ", newPri);
	PR_Thread_print(stderr, owner);
	fprintf(stderr, "\n");
#endif
    }
}

/*
** RevertPriority: Thread is the new best thread that is getting the
** monitor.
*/
void
_PR_RevertPriority(PRMonitor* mon, PRThread* oldOwner)
{
    if (oldOwner && mon->oldPriority != -1) {
	int maxPri = mon->oldPriority;
#if 0
	if (oldOwner->monitors.prev != &mon->links
	    && oldOwner->monitors.prev != oldOwner->monitors.next) {
	    /*
	    ** If this wasn't the last monitor we entered, we have to run
	    ** down the list of all the monitors, looking for the highest
	    ** priority waiter, and set our priority to that instead.
	    */
	    PRCList* qp = oldOwner->monitors.next;
	    while (qp != &oldOwner->monitors) {
		PRMonitor* m = MONITOR_PTR(qp);
		PRCList* waiters = m->lockQ.next;
		while (waiters != &m->lockQ) {
		    PRThread* thread = THREAD_PTR(waiters);
		    if (thread->priority > maxPri)
			maxPri = thread->priority;
		    waiters = waiters->next;
		}
		qp = qp->next;
	    }
	    fprintf(stderr, "*** Hit that nasty loop\n");
	}
#endif
	(void)_PR_SetThreadPriority(oldOwner, maxPri);
	mon->oldPriority = -1;
#if 0
	fprintf(stderr, "*** Resetting priority to %d ", maxPri);
	PR_Thread_print(stderr, oldOwner);
	fprintf(stderr, "\n");
#endif
    }
}

/*
** Wait for a monitor. Somebody else has it and we want it.
*/
#include <stdio.h>
#include "prdump.h"
void _PR_MonWait(PRMonitor *mon)
{
    PRThread *current = _pr_current_thread;

    PR_ASSERT(_pr_intsOff != 0);

    /* Wait forever to get the monitor */
    current->monitor = mon;
    current->monitorEntryCount = 1;
    PR_APPEND_LINK(&current->waitLinks, &mon->lockQ);
    PR_APPEND_LINK(&current->runqLinks, &_pr_monitorq);
    current->state = _PR_MON_WAIT;
#if 0
    _PR_InvertPriority(mon, current);
#endif

#ifdef XP_MAC
	if (gInTimerCallback)
		DebugStr("\pFATAL ERROR: Attempt to reschedule at interrupt time!");
#endif

    /* Deschedule this thread until we get the monitor */
    _MD_SWITCH_CONTEXT(current);

    /* We got the monitor */
    PR_ASSERT(current == _pr_current_thread);
    PR_ASSERT(PR_CLIST_IS_EMPTY(&current->waitLinks));
    PR_ASSERT(!PR_CLIST_IS_EMPTY(&mon->links));
    PR_ASSERT(_pr_intsOff != 0);
    PR_ASSERT(mon->owner == current);
    PR_ASSERT(mon->count == 1);
}

/*
** Wait for a notify/timeout.
*/
void _PR_CondWait(PRMonitor *mon, int64 sleep)
{
    PRThread *current = _pr_current_thread;
    int entryCount;

    PR_ASSERT(_pr_intsOff != 0);
    PR_ASSERT(mon->owner == current);
    PR_ASSERT(mon->count > 0);

    /* Deassign the monitor */
    PR_REMOVE_AND_INIT_LINK(&mon->links);
    entryCount = mon->count;
    mon->count = 0;
    mon->owner = 0;

    /* Give up monitor to highest priority waiter (if any) */
    (void) _PR_MonNotify(mon, current);

    /* Put on sleepQ and monitor condQ */
    _PR_PutOnSleepQ(current, sleep);
    PR_APPEND_LINK(&current->waitLinks, &mon->condQ);
    current->state = _PR_COND_WAIT;
    current->monitor = mon;
    current->monitorEntryCount = entryCount;

#ifdef XP_MAC
	if (gInTimerCallback)
		DebugStr("\pFATAL ERROR: Attempt to reschedule at interrupt time!");
#endif

   /* Deschedule this thread until we get the notify/timeout and the monitor */
    _MD_SWITCH_CONTEXT(current);

    /* We got the notify/timeout and the monitor */
    PR_ASSERT(current == _pr_current_thread);
    PR_ASSERT(PR_CLIST_IS_EMPTY(&current->waitLinks));
    PR_ASSERT(_pr_intsOff != 0);
    PR_ASSERT(mon->owner == current);
    PR_ASSERT(mon->count == entryCount);
}

/*
** Notify a monitor. Pick highest priority waiter on monitor's lockQ.
** Return non-zero if the processor should be rescheduled.
*/
int _PR_MonNotify(PRMonitor *mon, PRThread* oldOwner)
{
    PRThread *current = _pr_current_thread;
    PRThread *best, *t;
    int needYield, pri;
    PRCList *qp;

    /* PR_ASSERT(SchedIsLocked()); XXX might be called from clock... */
    PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->links));
    PR_ASSERT(mon->owner == 0);
    PR_ASSERT(mon->count == 0);
    needYield = 0;

    /* Search lockQ for highest priority runnable thread */
    pri = 0;
    best = 0;
    qp = mon->lockQ.next;
    while (qp != &mon->lockQ) {
	t = WAITER_PTR(qp);
	PR_ASSERT(t->state == _PR_MON_WAIT);
	if (t->flags & _PR_SUSPENDING) {
	    /*
	    ** We don't give monitors to threads that are about to
	    ** suspend. Therefore, let the thread sit on the monitor's
	    ** lockQ until it is resumed or dies.
	    */
	} else {
	    if (!best || (t->priority > best->priority)) {
		best = t;
		pri = t->priority;
	    }
	}
	qp = qp->next;
    }
    if (best) {
	PR_LOG(MONITOR, warn,
	       ("MonNotify: best=%s mon=%s[%x]", best->name, mon->name, mon));
	PR_ASSERT(best->state == _PR_MON_WAIT);
	PR_ASSERT(best->monitor == mon);
	PR_ASSERT(best->monitorEntryCount >= 1);
	PR_ASSERT(!PR_CLIST_IS_EMPTY(&best->runqLinks));

	/* Take best thread off sleepQ if it was on it */
	if (best->flags & _PR_ON_SLEEPQ) {
	    PR_LOG(MONITOR, warn, ("MonNotify: take thread=%s off sleepQ",
				   best->name));
	    /*
	    ** Give remaining sleep time to thread after "best" in the
	    ** sleepQ.
	    */
	    if (best->runqLinks.next != &_pr_sleepq) {
		t = THREAD_PTR(best->runqLinks.next);
		PR_ASSERT(THREAD_PTR(t->runqLinks.prev) == best);
		LL_ADD(t->sleep, t->sleep, best->sleep);
	    }
	    best->flags &= ~_PR_ON_SLEEPQ;
	}

	/* Assign monitor to best thread */
	mon->owner = best;
	mon->count = best->monitorEntryCount;
	PR_APPEND_LINK(&mon->links, &best->monitors);
	best->monitorEntryCount = 0;
	best->monitor = 0;
	best->state = _PR_RUNNABLE;

	PR_LOG(MONITOR, warn, ("MonNotify: %s on to runq at priority %d",
			       best->name, pri));
	PR_REMOVE_AND_INIT_LINK(&best->waitLinks);	/* Take off lockQ */
	PR_REMOVE_LINK(&best->runqLinks);	/* Take off monitorQ/sleepQ */
	PR_APPEND_LINK(&best->runqLinks, &_pr_runq[pri]);
	_pr_runq_ready_mask |= (1L << pri);
	if (pri >= current->priority) {
	    needYield = 1;
	}
#if 0
	_PR_RevertPriority(mon, oldOwner);
#endif
    }
    return needYield;
}

/*
** Move one thread from the condQ to the waitQ. Called by the clock code
** when a PR_Wait timeout expires, or by the notify code when an actual
** notify comes in.
*/
void _PR_NotifyOneThread(PRMonitor *mon, PRThread *thread)
{
    PR_ASSERT(thread->state == _PR_COND_WAIT);
    PR_ASSERT(thread->monitor == mon);
    PR_ASSERT(thread->monitorEntryCount > 0);

    /*
    ** Take off of the sleepQ, if still on it (the clock handler may
    ** have already taken the thread off).
    */
    if (thread->flags & _PR_ON_SLEEPQ) {
	PR_LOG(MONITOR, warn,
	       ("CondNotify: take thread=%s off sleepQ",
		thread->name));
	if (thread->runqLinks.next != &_pr_sleepq) {
	    PRThread *t = THREAD_PTR(thread->runqLinks.next);
	    PR_ASSERT(THREAD_PTR(t->runqLinks.prev) == thread);
	    LL_ADD(t->sleep, t->sleep, thread->sleep);
	}
	thread->flags &= ~_PR_ON_SLEEPQ;
    }
    PR_REMOVE_LINK(&thread->runqLinks); /* remove from monitorQ/sleepQ */
    PR_REMOVE_LINK(&thread->waitLinks); /* remove from condQ */

    /*
    ** Move winning thread to monitor wait state. Put it on the monitorQ
    ** and the monitors lockQ (as if it had entered the EnterMonitor code
    ** and blocked).
    */
    thread->state = _PR_MON_WAIT;
    PR_APPEND_LINK(&thread->waitLinks, &mon->lockQ);
    PR_APPEND_LINK(&thread->runqLinks, &_pr_monitorq);
    PR_LOG(MONITOR, warn,
	   ("CondNotify: condQ -> lockQ: thread=%s mon=%s[%x]",
	    thread->name, mon->name, mon));
}


/*
** Notify condition. Moves first thread found from condQ to lockQ. If
** condQ goes empty and the monitor is no longer owned then we do a
** MonNotify.
*/
int _PR_CondNotify(PRMonitor *mon, _PRNotifyEnum how)
{
    PRThread *winner;

    if ((how == _PR_NOTIFY_STICKY) && PR_CLIST_IS_EMPTY(&mon->condQ)) {
        mon->stickyCount++;
    }

    while (!PR_CLIST_IS_EMPTY(&mon->condQ)) {
	/*
	** Get first winning waiter. Remove it from the condQ and from
	** the sleepQ. The waiter may also be on the sleepQ because it
	** was sleeping awaiting a timeout.
	*/
	winner = WAITER_PTR(mon->condQ.next);
	_PR_NotifyOneThread(mon, winner);
	if (((winner->flags & _PR_SUSPENDING) == 0)
            && (how != _PR_NOTIFY_ALL)) {
	    break;
	}
	/*
	** Continue examining the condQ if the thread is suspended or if
	** we are notifying all threads. Suspended threads don't count
	** because the don't get the monitor until they are resumed.
	*/
    }
    if (mon->owner == 0) {
	PR_LOG(MONITOR, warn,
	       ("CondNotify: condQ is %s mon=%s[%x]",
		PR_CLIST_IS_EMPTY(&mon->condQ) ? "empty" : "not empty",
		mon->name, mon));
	/* Pass monitor to highest priority waiting thread */
	return _PR_MonNotify(mon, NULL);
    }
    return 0;
}

#if 0
/*
** List out the monitors owned by the current thread
*/
void _PR_LogMonitors(void)
{
    PRThread *thread = _pr_current_thread;
    PRMonitor *mon;
    PRCList *qp;

    qp = thread->monitors.next;
    if (qp != &thread->monitors) {
	PR_LOG(MONITOR, out, ("monitors: "));
	while (qp != &thread->monitors) {
	    mon = MONITOR_PTR(qp);
	    PR_LOG(MONITOR, out, ("%s[%x] count=%d",
				  mon->name, mon, mon->count));
	    if (qp->next == qp) {
		PR_LOG(MONITOR, out, ("CIRCULAR"));
		PR_Abort();
	    }
	    qp = qp->next;
	}
    } else {
	PR_LOG(MONITOR, out, ("monitors: empty list"));
    }
}
#else
#define _PR_LogMonitors()
#endif

/************************************************************************/

PR_PUBLIC_API(PRMonitor *) PR_NewMonitor(int entryCount)
{
    PRMonitor *mon;

    mon = (PRMonitor*) malloc(sizeof(PRMonitor));
    if (mon) {
        mon->links.next = 0;
	PR_InitMonitor(mon, entryCount, "anon");
	PR_LOG(MONITOR, warn, ("new anonymous monitor: %s[%x]",
			       mon->name, mon));
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
	PR_LOG(MONITOR, warn, ("new monitor: %s[%x]", name, mon));
    }
    return mon;
}

PR_PUBLIC_API(void) PR_InitMonitor(PRMonitor *mon, int entryCount, char *name)
{
    if (mon->links.next == 0) {
        /* Make sure *all* fields get initialized */
        memset(mon, 0, sizeof(PRMonitor));

	mon->name = strdup(name);
	mon->oldPriority = -1;
	PR_INIT_CLIST(&mon->links);
	PR_INIT_CLIST(&mon->condQ);
	PR_INIT_CLIST(&mon->lockQ);
	PR_APPEND_LINK(&mon->allLinks, &_pr_all_monitors);
	if (entryCount) {
	    PRThread *thread = _pr_current_thread;
	    mon->owner = thread;
	    mon->count = entryCount;
	    PR_APPEND_LINK(&mon->links, &thread->monitors);
	}
    }
}

PR_PUBLIC_API(void) PR_EnterMonitor(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;
    int is;

    is = _PR_IntsOff();
    PR_ASSERT(is == 0);
    if (mon->owner == thread) {
	mon->count++;
	PR_LOG(MONITOR, debug,
	       ("EnterMonitor %s[%x] count -> %d",
		mon->name, mon, mon->count));
    } else if (mon->owner == 0) {
	mon->owner = thread;
	mon->count = 1;
	PR_APPEND_LINK(&mon->links, &thread->monitors);
	PR_LOG(MONITOR, debug,
	       ("EnterMonitor %s[%x] acquired",
		mon->name, mon));
	_PR_LogMonitors();
    } else {
	_PR_MonWait(mon);
	PR_ASSERT(mon->owner == thread);
	PR_ASSERT(mon->count == 1);
	PR_ASSERT(!PR_CLIST_IS_EMPTY(&mon->links));
	PR_LOG(MONITOR, debug,
	       ("EnterMonitor %s[%x] acquired (waited)",
		mon->name, mon));
	_PR_LogMonitors();
    }
    _PR_IntsOn(is, 0);

#if 0
    /* Check for pending async call */
    if (thread->asyncCall) {
	void (*fp)(void*,void*) = thread->asyncCall;
	void *a0 = thread->asyncArg0;
	void *a1 = thread->asyncArg1;
	thread->asyncCall = 0;
	thread->asyncArg0 = 0; 
	thread->asyncArg1 = 0; 
	(*fp)(a0, a1);
    }
#endif
}

PR_PUBLIC_API(int) PR_ExitMonitor(PRMonitor *mon)
{
    PRThread *current = _pr_current_thread;
    int needYield, rv = 0;
    int is;

#ifdef DEBUG_nix
    /* ... like anybody ever checks for ExitMonitor failing ... */
    PR_ASSERT(mon->owner == current);
#endif

    if (mon->owner != current) {
	return -1;
    }
    needYield = 0;

    is = _PR_IntsOff();
    PR_ASSERT(is == 0);
    --mon->count;
    PR_ASSERT(mon->count >= 0);
    if (mon->count == 0) {
	PR_LOG(MONITOR, debug, ("ExitMonitor %s[%x] release",
				mon->name, mon));
	/* Take monitor off of this threads list of monitors */
	PR_ASSERT(!PR_CLIST_IS_EMPTY(&mon->links));
	PR_REMOVE_AND_INIT_LINK(&mon->links);
	mon->owner = 0;
	needYield = _PR_MonNotify(mon, current);
    } else {
	PR_LOG(MONITOR, debug,
	       ("ExitMonitor %s[%x] count -> %d",
		mon->name, mon, mon->count));
    }
    _PR_LogMonitors();

    _PR_IntsOn(is, needYield);
    return rv;
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
    int is, rv = 0;
    int entryCount;

    if (!LL_GE_ZERO(sleep)) {
	/* Leave it really big, but positive */
	LL_USHR(sleep, sleep, 1);
    }

    is = _PR_IntsOff();
    if (mon->stickyCount) {
        mon->stickyCount--;
        goto done;
    }

    PR_ASSERT(is == 0);
    entryCount = mon->count;
    PR_LOG(MONITOR, warn, ("CondWait %s[%x] for %lld, count %d",
			   mon->name, mon, sleep, entryCount));
    if (mon->owner != thread) {
	/* Oops */
	rv = -1;
	goto done;
    }

    _PR_CondWait(mon, sleep);
    PR_LOG(MONITOR, warn,
	   ("CondWait %s[%x] done", mon->name, mon));
    PR_ASSERT(mon->owner == thread);
    PR_ASSERT(mon->count == entryCount);

  done:
    _PR_IntsOn(is, 0);

#if 0
    /* Check for pending async call */
    if (thread->asyncCall) {
	void (*fp)(void*,void*) = thread->asyncCall;
	void *a0 = thread->asyncArg0;
	void *a1 = thread->asyncArg1;
	thread->asyncCall = 0;
	thread->asyncArg0 = 0; 
	thread->asyncArg1 = 0; 
	(*fp)(a0, a1);
    }
#endif
    return rv;
}

PR_PUBLIC_API(int) PR_Notify(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;
    int rv, is;
    int needYield;

    /*
    ** We must own the monitor underneath the condition variable to
    ** manipulate it.
    */
    is = _PR_IntsOff();
    PR_ASSERT(is == 0);
    PR_LOG(MONITOR, warn, ("CondNotify %s[%x]: count %d",
			   mon->name, mon, mon->count));
    if (mon->owner != thread) {
	rv = -1;
	needYield = 0;
	goto done;
    }

    /* Wakeup first runnable thread waiting */
    needYield = _PR_CondNotify(mon, _PR_NOTIFY_ONE);
    rv = 0;

  done:
    _PR_IntsOn(is, needYield);
    return rv;
}

PR_PUBLIC_API(int) PR_NotifyAll(PRMonitor *mon)
{
    PRThread *thread = _pr_current_thread;
    int is, rv;
    int needYield;

    /*
    ** We must own the monitor underneath the condition variable to
    ** manipulate it.
    */
    is = _PR_IntsOff();
    PR_ASSERT(is == 0);
    PR_LOG(MONITOR, warn,
	   ("CondNotifyAll %s[%x]", mon->name, mon));
    if (mon->owner != thread) {
	rv = -1;
	needYield = 0;
	goto done;
    }

    /* Wakeup first runnable thread waiting */
    needYield = _PR_CondNotify(mon, _PR_NOTIFY_ALL);
    rv = 0;

  done:
    _PR_IntsOn(is, needYield);
    return rv;
}

PR_PUBLIC_API(void) PR_DestroyMonitor(PRMonitor *mon)
{
    PR_ASSERT(mon->owner == 0);
    PR_ASSERT(mon->count == 0);
    PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->links));
    PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->condQ));
    PR_ASSERT(PR_CLIST_IS_EMPTY(&mon->lockQ));
    PR_REMOVE_LINK(&mon->allLinks);
    if (mon->name) {
	free(mon->name);
	mon->name = 0;
    }
#ifdef XP_UNIX
    if ((mon->flags & _PR_HAS_EVENT_PIPE) != 0) {
	close(mon->eventPipe[0]);
	close(mon->eventPipe[1]);
    }
#elif defined(XP_PC)

#elif defined(XP_MAC)
	#pragma unused (mon)
#endif
    free(mon);
}

/************************************************************************/
/*
** Interface to selectable monitors.
*/

PR_PUBLIC_API(int)
PR_MakeMonitorSelectable(PRMonitor* mon)
{
#ifdef XP_UNIX
    int err = 0;
    if ((mon->flags & _PR_HAS_EVENT_PIPE) == 0) {
	err = pipe(mon->eventPipe);
	if (err) return err;

	/* success */
	mon->flags |= _PR_HAS_EVENT_PIPE;
    }
    return err;

#elif defined(XP_PC)
    return 0;

#elif defined(XP_MAC)
	#pragma unused (mon)
	return 0;
#endif
}

#define NOTIFY_TOKEN	0xFA

int notifyCount = 0;

PR_PUBLIC_API(void)
PR_SelectNotify(PRMonitor* mon)
{
#ifdef XP_UNIX
    int status;
    unsigned char buf[] = { NOTIFY_TOKEN };

    PR_ASSERT(mon->owner == PR_CurrentThread());
    PR_ASSERT(mon->count != 0);
    PR_ASSERT((mon->flags & _PR_HAS_EVENT_PIPE) != 0);

    status = write(mon->eventPipe[1], buf, 1);
    PR_ASSERT(status == 1);
    notifyCount++;

#elif defined(XP_PC)
#ifdef _WIN32
    // FIX ME - SW monitors are not implemented for WIN32
    PR_ASSERT(0);
#else
    PostAppMessage( GetCurrentTask(), (UINT)_pr_PostEventMsgId, 
                    (WPARAM)0, (LPARAM)mon);
#endif  /* ! _WIN32 */

#elif defined(XP_MAC)
    notifyCount++;
	PostEvent(app2Evt, 0);
#endif
}

#ifdef XP_UNIX

PR_PUBLIC_API(int)
PR_GetSelectFD(PRMonitor* mon)
{
    PR_ASSERT((mon->flags & _PR_HAS_EVENT_PIPE) != 0);
    return mon->eventPipe[0];
}

PR_PUBLIC_API(void)
PR_ClearSelectNotify(PRMonitor* mon)
{
    int count;
    unsigned char c;

    PR_ASSERT(mon->owner == PR_CurrentThread());
    PR_ASSERT(mon->count != 0);
    PR_ASSERT((mon->flags & _PR_HAS_EVENT_PIPE) != 0);

    if (notifyCount <= 0) return;
    count = read(mon->eventPipe[0], &c, 1);
    PR_ASSERT(count == 1 && c == NOTIFY_TOKEN);
    notifyCount--;
}

#elif defined(XP_PC)
    // FIX ME
PR_PUBLIC_API(void)
PR_ClearSelectNotify(PRMonitor* mon)
{
    notifyCount--;
}
#elif defined(XP_MAC)
void
PR_ClearSelectNotify(PRMonitor* mon)
{
    notifyCount--;
}
#endif

/************************************************************************/

#endif /* SW_THREADS */
