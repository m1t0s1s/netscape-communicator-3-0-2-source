#ifdef SW_THREADS
#include "prlog.h"
#include "swkern.h"
#include "mdint.h"

#ifdef XP_MAC
#include <Types.h>
#endif

PR_LOG_DEFINE(CLOCK);

int _pr_intsOff;
int _pr_missed_clock_intrs;
int _pr_missed_child_intrs;
int _pr_missed_io_intrs;

#ifdef PR_NO_PREEMPT
void (*pr_intr_switch_hook)(void*);
void *pr_intr_switch_hook_arg;
#endif

/*
** User threading interrupt handling code.
*/

/*
** Clock interrupt handler. Update sleeping threads timeout state.
*/
int _PR_ClockTick(void)
{
    PRThread *thread;
    PRMonitor *mon;
    int64 now, elapsed;
    int wantResched = 0;

#if defined(XP_UNIX)
    static int in_clock_tick = 0;

    if (in_clock_tick) return 0;
    in_clock_tick = 1;
#endif

    /* Get current time */
    now = PR_Now();

    /* Figure out how much time elapsed since the last clock tick */
    LL_SUB(elapsed, now, _pr_now);
    _pr_now = now;
    if (!LL_GE_ZERO(elapsed)) {
        /*
        ** Somebody set the system clock back in time! Try to recover
        ** gracefully. Set the elapsed time to 50ms or the first sleepers
        ** sleep time, whichever is smaller.
        */
        LL_I2L(elapsed, 50*1000L);
        if (_pr_sleepq.next != &_pr_sleepq) {
            int64 sleep = THREAD_PTR(_pr_sleepq.next)->sleep;
            if (LL_CMP(sleep, <, elapsed)) {
                elapsed = sleep;
            }
        }
    }

    /*
    ** Wakeup any sleepers that are waiting for a timeout. There are two
    ** kinds of sleepers: Those just waiting for time to pass and those
    ** waiting for a condition variable. For the latter case we have to
    ** remove the thread from the condition variables condQ.
    */
    while (_pr_sleepq.next != &_pr_sleepq) {
        thread = THREAD_PTR(_pr_sleepq.next);
        if (LL_CMP(elapsed, <, thread->sleep)) {
            LL_SUB(thread->sleep, thread->sleep, elapsed);
            PR_ASSERT(LL_GE_ZERO(thread->sleep));
            PR_LOG(CLOCK, debug, ("nop tick"));
            break;
        }

        /*
        ** Consume this sleeper's amount of elapsed time from the elapsed
        ** time value. The next remaining piece of elapsed time will be
        ** available for the next sleeping thread's timer.
        */
        PR_LOG(CLOCK, out, ("clock: thread=%s done sleeping; state=%d",
                            thread->name, thread->state));
        LL_SUB(elapsed, elapsed, thread->sleep);
        PR_ASSERT(LL_GE_ZERO(elapsed));
        PR_ASSERT(thread->flags & _PR_ON_SLEEPQ);
        thread->flags &= ~_PR_ON_SLEEPQ;
        PR_REMOVE_LINK(&thread->runqLinks);	/* take off _pr_sleepq */
	
        switch (thread->state) {
          case _PR_SLEEPING:
            PR_ASSERT(PR_CLIST_IS_EMPTY(&thread->waitLinks));
            if (thread->flags & _PR_SUSPENDING) {
                /*
                ** Sleeper needs to be suspended. Clear suspending bit
                ** and set state to suspended and put on _pr_suspendq.
                */
                thread->flags &= ~_PR_SUSPENDING;
                thread->state = _PR_SUSPENDED;
                PR_APPEND_LINK(&thread->runqLinks, &_pr_suspendq);
            } else {
                /*
                ** All done sleeping, ready to run again
                */
                PR_LOG(CLOCK, out, ("clock: %s on to runq at priority %d",
                                    thread->name, thread->priority));
                PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[thread->priority]);
                _pr_runq_ready_mask |= (1L << thread->priority);
                thread->state = _PR_RUNNABLE;
                if (thread->priority >= _pr_current_thread->priority) {
                    wantResched = 1;
                }
            }
            break;
	
          case _PR_COND_WAIT:
            /*
            ** Transfer thread from condition wait Q to monitor wait Q.
            */
            PR_ASSERT(!PR_CLIST_IS_EMPTY(&thread->waitLinks));
            PR_ASSERT(thread->monitor != 0);
            PR_ASSERT(thread->monitorEntryCount != 0);
            PR_LOG(CLOCK, out,
                   ("clock: CondNotify: thread=%s monitor=%s[%x]",
                    thread->name, thread->monitor->name, thread->monitor));
            PR_APPEND_LINK(&thread->runqLinks, &_pr_monitorq);
	
            /*
            ** Move sleeping thread from condQ to waitQ. Then pass the
            ** monitor to somebody if nobody is currently using it.
            */
            mon = thread->monitor;
            _PR_NotifyOneThread(mon, thread);
            if (mon->owner == 0) {
                if (_PR_MonNotify(mon, 0)) {
                    wantResched = 1;
                }
            }
            break;
	
          default:
            PR_Abort();
        }
	
        /*
        ** Loop again, apply elapsed's remaining time to the next sleeper
        */
    }

#if defined(XP_UNIX)
    /*
    ** Because we don't yet support async i/o signals on unix we need to
    ** periodically poll the the i/o descriptors when there are cpu
    ** consumptive threads running that lock out the idle thread.
    */
    {
        extern int64 _pr_lastTimePolled;
        extern PRBool _pr_pollForIO;
        extern int _PR_PauseForIO(PRBool poll);
        int64 delta, c;

        if (_pr_pollForIO) {
            LL_I2L(c, 200*1000);                           /* .2 seconds */
            LL_SUB(delta, now, _pr_lastTimePolled);
            LL_SUB(delta, delta, c);
            if (LL_GE_ZERO(delta)) {
                /*
                ** It's now officially been a long time since idle ran. Check
                ** for i/o that is ready to complete. Call the i/o handling
                ** function to check the pollable descriptors for i/o ready
                ** to be handled.
                */
                _pr_pollForIO = PR_FALSE;
                wantResched |= _PR_PauseForIO(PR_TRUE);
                _pr_pollForIO = PR_TRUE;

                /*
                ** Update last time polled in case we are doing this alot
                ** and idle is not running. Otherwise we would
                ** continuosly call this routine for every clock tick.
                */
                _pr_lastTimePolled = now;
            }
        }
    }
#endif

    _pr_missed_clock_intrs = 0;

#if defined(XP_UNIX)
    in_clock_tick = 0;
#endif
    return wantResched;
}

/************************************************************************/

/*
** Child death handling routine. Called when a child process dies.
**
** XXX should be able to wakeup a single waiter for a single process
*/
int _PR_ChildDeath(void)
{
    int needYield;

    /* Notify all threads waiting for a SIGCHLD */
    needYield = _PR_CondNotify(_pr_child_intr, _PR_NOTIFY_ALL);
    _pr_missed_child_intrs = 0;
    return needYield;
}

/************************************************************************/

/*
** Async I/O interrupt handler. Called when an async i/o operation
** completes. Wakeup all async io waiters.
**
** XXX should be able to wakeup a single waiter for a single descriptor
*/
int _PR_AsyncIO(void)
{
    int needYield;

    /* Notify all threads waiting for a SIGIO */
    needYield = _PR_CondNotify(_pr_async_io_intr, _PR_NOTIFY_ALL);
    _pr_missed_io_intrs = 0;
    return needYield;
}

/************************************************************************/

/*
** Generic interrupt handler. Invoke the lower level function and
** then reschedule the processor if necessary.
*/
static void HandleInterrupt(int (*func)(void), char *name)
{
    PRThread *thread;
    int wantResched;
    int olderrno;
    
#ifdef XP_UNIX
    /* Save errno in case interrupt handler trashes it */
    olderrno = errno;
#endif
    wantResched = (*func)();

#ifdef XP_UNIX
    /* Restore errno back to its former glory (ha) */
    errno = olderrno;
#endif

    thread = _pr_current_thread;
    if (_pr_single_lock->owner == 0) {
	/*
	** If the interrupt wants a resched or if some other thread at
	** the same priority needs the cpu, reschedule.
	*/
	if (wantResched || (_pr_runq_ready_mask >> thread->priority)) {
#ifdef PR_NO_PREEMPT
	    PR_LOG(CLOCK, out, ("%s: want resched from %s",
				name, thread->name));
	    pr_want_resched = 1;
	    _pr_intsOff = 1;
	    if (pr_intr_switch_hook) {
		(*pr_intr_switch_hook)(pr_intr_switch_hook_arg);
	    }
#else
	    PR_LOG(CLOCK, out, ("%s: resched from %s",
				name, thread->name));
	    thread->state = _PR_RUNNABLE;
	    PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[thread->priority]);
	    _pr_runq_ready_mask |= (1L << thread->priority);
	    _pr_intsOff = 1;
	    _MD_SWITCH_CONTEXT(thread);
	    PR_ASSERT(_pr_intsOff == 1);
#endif
	}
    }

#ifdef XP_UNIX
    /* Restore errno back to its former glory (ha) */
    errno = olderrno;
#endif

    _pr_intsOff = 0;
}

void _PR_ClockInterruptHandler(void)
{
    if (_pr_intsOff) {
	_pr_missed_clock_intrs++;
	return;
    }
    HandleInterrupt(_PR_ClockTick, "clock");
}

void _PR_ChildDeathInterruptHandler(void)
{
    if (_pr_intsOff) {
	_pr_missed_child_intrs++;
	return;
    }
    HandleInterrupt(_PR_ChildDeath, "child-death");
}

void _PR_AsyncIOInterruptHandler(void)
{
    if (_pr_intsOff) {
	_pr_missed_io_intrs++;
	return;
    }
    HandleInterrupt(_PR_AsyncIO, "async-io");
}

/************************************************************************/

/*
** Disable all interrupts. Return old interrupt status value.
*/
int _PR_IntsOff(void)
{
    int rv = _pr_intsOff;
    _pr_intsOff = 1;
    return rv;
}

/*
** Restore interrupt status value from it's previously stacked value.
** Yield the process if interrupts are being enabled and wantYield is
** true.
*/
int _PR_IntsOn(int status, int wantYield)
{
    PR_ASSERT(_pr_intsOff != 0);
    if (status == 0) {
	/*
	** We are about to re-enable interrupts. If the clock tried to
	** tick while interrupts were disabled, let it tick now. The
	** interrupt handler will not re-enter while we are doing this
	** because _pr_intsOff is still non-zero.
	*/
	if (_pr_missed_clock_intrs) {
	    wantYield |= _PR_ClockTick();
	}
	if (_pr_missed_child_intrs) {
	    wantYield |= _PR_ChildDeath();
	}
	if (_pr_missed_io_intrs) {
	    wantYield |= _PR_AsyncIO();
	}
	_pr_intsOff = 0;

#ifdef PR_NO_PREEMPT
	wantYield |= pr_want_resched;
#endif

	/* Reschedule cpu if we need to */
	if (wantYield) {
	    PR_Yield();
	    wantYield = 0;
	}
    }
    return wantYield;
}

#ifdef PR_NO_PREEMPT
PR_PUBLIC_API(void) PR_SetInterruptSwitchHook(void (*fn)(void *arg), void *arg)
{
    pr_intr_switch_hook = fn;
    pr_intr_switch_hook_arg = arg;
}
#endif

#endif /* SW_THREADS */
