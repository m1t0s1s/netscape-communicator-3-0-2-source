#ifdef SW_THREADS
#include "prmem.h"
#include "prlog.h"
#include "swkern.h"
#include "mdint.h"
#include <string.h>

PR_LOG_DEFINE(THREAD);

/* XXX fix up most of the ints-off code to use the per-thread monitor */
#define LOCK_THREAD(t) PR_CEnterMonitor(t)
#define UNLOCK_THREAD(t) PR_CExitMonitor(t)

/*
** Software threading package. Designed to run on top of a SINGLE native
** OS process.
*/
int32 _pr_unique_thread_id;

#ifdef DEBUG
int _pr_scanning_threads;
#endif

#define _CLAMP_PRIORITY(newPri)		      \
    if (newPri < 0) {			      \
	newPri = 0;			      \
    } else if (newPri >= PR_NUM_PRIORITIES) { \
	newPri = PR_NUM_PRIORITIES - 1;	      \
    }

PR_PUBLIC_API(PRThread *) PR_CreateThread(char *name, int priority, 
                                        size_t stackSize)
{
    PRThread *t;

    /* Before we make a new thread, free any old ones */
    _PR_FreeZombies();

    _CLAMP_PRIORITY(priority);

    t = (PRThread*) calloc(1, sizeof(PRThread));
    if (t) {
	t->id = ++_pr_unique_thread_id;
	PR_INIT_CLIST(&t->monitors);
	PR_INIT_CLIST(&t->runqLinks);
	PR_INIT_CLIST(&t->waitLinks);
	t->name = strdup(name);
	if (!t->name) {
	    free(t);
	    return 0;
	}
	t->priority = priority;

	/* Allocate stack if necessary */
	if (!_MD_NewStack(t, stackSize)) {
	    free(t->name);
	    free(t);
	    return 0;
	}
    }
    return t;
}

/*
** Attach a thread object to an existing thread (the current thread).
*/
PR_PUBLIC_API(PRThread *) PR_AttachThread(char *name, int priority, 
                                        PRThreadStack *stack)
{
    PRThread *t;

    /* Before we make a new thread, free any old ones */
    _PR_FreeZombies();

    _CLAMP_PRIORITY(priority);

    t = (PRThread*) calloc(1, sizeof(PRThread));
    if (t) {
	t->id = ++_pr_unique_thread_id;
	PR_INIT_CLIST(&t->monitors);
	PR_INIT_CLIST(&t->runqLinks);
	PR_INIT_CLIST(&t->waitLinks);
	t->name = strdup(name);
	if (!t->name) {
	    free(t);
	    return 0;
	}
	t->priority = priority;

#if defined(XP_PC) && !defined(_WIN32)
	/* Allocate stack if necessary */
	if (!_MD_NewStack(t, stack->stackSize)) {
	    free(t->name);
	    free(t);
	    return 0;
	}
#else
	t->stack = stack;
#endif
    }
    return t;
}

PR_PUBLIC_API(void) PR_DestroyThread(PRThread *thread)
{
    int is;

    is = _PR_IntsOff();

    PR_LOG(SCHED, out, ("About to destroy thread %s [%d]",
			thread->name, thread->state));
    if (thread->flags & _PR_SYSTEM) {
	_pr_system_active--;
	PR_ASSERT(_pr_system_active >= 0);
    } else {
	_pr_user_active--;
	PR_ASSERT(_pr_user_active >= 0);
    }
    if (_pr_user_active == 0) {
	/* Can't call libc with interrupts off! */
	_PR_IntsOn(is, 0);
	exit(0);
    }

    switch (thread->state) {
      case _PR_UNBORN:
      case _PR_ZOMBIE:
	break;

      case _PR_RUNNING:
	/*
	** Put current thread on zombie list if it's ready to go there.
	** This will get processed the next time we are called. This is
	** done to avoid freeing the stack we are executing on. Note also
	** that this is done with interrupts off and that idle, which
	** cleans up the zombie list also works on the zombieq with
	** interrupts off. Finally, this is done synchronously so we know
	** that we cannot be screwing up the list.
	*/
	thread->state = _PR_ZOMBIE;
	PR_APPEND_LINK(&thread->runqLinks, &_pr_zombieq);
	_PR_Schedule();
	PR_NOT_REACHED("Continuing after destroying running thread.");

      case _PR_RUNNABLE:
	PR_REMOVE_AND_INIT_LINK(&thread->runqLinks);
	if (PR_CLIST_IS_EMPTY(&_pr_runq[thread->priority])) {
	    _pr_runq_ready_mask &= ~(1L << thread->priority);
	}
	break;

      case _PR_SLEEPING:
	_PR_FixSleepQ(thread);
	PR_REMOVE_AND_INIT_LINK(&thread->runqLinks);
	break;

      case _PR_SUSPENDED:
	PR_REMOVE_AND_INIT_LINK(&thread->runqLinks);
	break;

      case _PR_COND_WAIT:
	PR_REMOVE_AND_INIT_LINK(&thread->runqLinks);
	PR_REMOVE_AND_INIT_LINK(&thread->waitLinks);
	break;

      case _PR_MON_WAIT:
	PR_REMOVE_AND_INIT_LINK(&thread->waitLinks);
	break;
    }
    _PR_IntsOn(is, 0);

    _PR_DestroyThread(thread);
}

/*
** Do the actual work of destroying a thread.
*/
void _PR_DestroyThread(PRThread *thread)
{
    PRMonitor *mon;
    PRPerThreadData **pptd, *ptd;

    /* Release all monitors that we have entered */
    while (thread->monitors.next != &thread->monitors) {
	mon = MONITOR_PTR(thread->monitors.next);
	PR_REMOVE_AND_INIT_LINK(&mon->links);
	mon->owner = 0;
	mon->count = 0;
	(void) _PR_MonNotify(mon, NULL);
    }
    PR_ASSERT(PR_CLIST_IS_EMPTY(&thread->monitors));
    PR_ASSERT(PR_CLIST_IS_EMPTY(&thread->runqLinks));
    PR_ASSERT(PR_CLIST_IS_EMPTY(&thread->waitLinks));

    /* Free up memory and we're gone... */
    _MD_FreeStack(thread);

    /* XXX: this assumes that ptd->priv is GC'able memory or static */
    pptd = &thread->ptd;
    while ((ptd = *pptd) != 0) {
	*pptd = ptd->next;
	free(ptd);
    }

    if (thread->name) {
	free(thread->name);
	thread->name = 0;
    }

    /* Now we can free up the thread object */
    free(thread);
}

/*
** Eliminate any zombies. This is done synchronously, but with interrupts
** off (basically we are using interrupts off to lock the zombieQ which
** can't really be locked). Before we disable interrupts we get the heap
** lock so that the free calls in _PR_DestroyThread won't lock
** up.
*/
void _PR_FreeZombies(void)
{
    int 		is;
    PRCList 	zombieq, *qp;
    PRThread 	*thread;

    if (_pr_heap_lock) {
	/*
	** Snatch a copy of the current zombieQ. This is necessary
	** because we can't call free with interrupts disabled. We have
	** to run down the list to copy it because of the way the list
	** linkage works.
	*/
	PR_INIT_CLIST(&zombieq);
	is = _PR_IntsOff();
	qp = _pr_zombieq.next;
	while (qp != &_pr_zombieq) {
	    thread = THREAD_PTR(qp);
	    qp = qp->next;
	    PR_REMOVE_LINK(&thread->runqLinks);
	    PR_APPEND_LINK(&thread->runqLinks, &zombieq);
	}
	_PR_IntsOn(is, 0);

	qp = zombieq.next;
	while (qp != &zombieq) {
	    thread = THREAD_PTR(qp);
	    qp = qp->next;
	    PR_REMOVE_AND_INIT_LINK(&thread->runqLinks);
	    PR_LOG(SCHED, warn, ("free-zombies: destroying thread %s",
				 thread->name));
	    _PR_DestroyThread(thread);
	}
    }
}

PR_PUBLIC_API(void) PR_Exit(void)
{
    PR_ASSERT(_pr_current_thread->state == _PR_RUNNING);
    PR_LOG_FLUSH();
    PR_DestroyThread(_pr_current_thread);
#if !defined(XP_PC) || defined(_WIN32)
    PR_NOT_REACHED("Continuing after exit.");
#endif
}

PR_PUBLIC_API(void) PR_Yield(void)
{
    PRThread *thread = _pr_current_thread;
    int is;
    int32 wantResched;

    is = _PR_IntsOff();
    wantResched = _pr_runq_ready_mask >> thread->priority;
#ifdef PR_NO_PREEMPT
    wantResched |= pr_want_resched;
#endif
    if (wantResched) {
	/* Give cpu to next thread in _pr_runq */
	if (thread->priority == 0)
		PR_LOG(THREAD, debug, ("Yield: on to runq at priority %d", thread->priority));
	else
		PR_LOG(THREAD, out, ("Yield: on to runq at priority %d", thread->priority));
	thread->state = _PR_RUNNABLE;
	PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[thread->priority]);
	_pr_runq_ready_mask |= (1L << thread->priority);
	_PR_SwitchThread(thread);
    }
    _PR_IntsOn(is, 0);
}

/*
** Intermediate that invokes startup routine for thread. Knows enough to
** release scheduler lock and call exit if thread returns.
*/
void HopToad(void (*e)(void*,void*), void *o, void *a)
{
    PR_ASSERT(_pr_intsOff != 0);
    PR_LOG(SCHED, out, ("Starting [sp=%x]", &e));
    _PR_IntsOn(0, 0);
    (*e)(o, a);
    PR_Exit();
}

/*
** Used to avoid assembly language in certain setjmp implementations
*/
void HopToadNoArgs(void)
{
    PRThread *thread = _pr_current_thread;
    void (*e)(void*,void*) = thread->asyncCall;
    void *o = thread->asyncArg0;
    void *a = thread->asyncArg1;

    thread->asyncCall = 0;
    thread->asyncArg0 = 0;
    thread->asyncArg1 = 0;
    HopToad(e, o, a);
}

PR_PUBLIC_API(int) PR_Start(PRThread *thread, void (*e)(void*,void*), 
                           void *o, void *a)
{
    int rv, needYield;
    int is;

    needYield = rv = 0;
    is = _PR_IntsOff();
    if (thread->state != _PR_UNBORN) {
	rv = -1;
	goto done;
    }
    if (thread->flags & _PR_SYSTEM) {
	_pr_system_active++;
    } else {
	_pr_user_active++;
    }
    _MD_INIT_CONTEXT(thread, e, o, a);

    /* Put thread on _pr_runq */
    thread->state = _PR_RUNNABLE;
    PR_LOG(THREAD, out, ("Start: %s on to runq at priority %d",
			 thread->name, thread->priority));
    PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[thread->priority]);
    _pr_runq_ready_mask |= (1L << thread->priority);
    if (thread->priority > _pr_current_thread->priority) {
	needYield = 1;
    }

  done:
    _PR_IntsOn(is, needYield);
    return rv;
}

#if 0
PR_PUBLIC_API(int) PR_AsyncCall(PRThread *thread, void (*f)(void*,void*), 
                               void *o, void *a)
{
    int rv = 0;

    LockSched();
    switch (thread->state) {
      case PR_UNBORN:
      case PR_ZOMBIE:
	rv = -1;
	break;

      case PR_RUNNING:
	UnlockSched();
	(*f)(o, a);
	goto done;

      default:
	thread->asyncCall = f;
	thread->asyncArg0 = o;
	thread->asyncArg1 = a;
	break;
    }
    UnlockSched();

  done:
    return rv;
}

PR_PUBLIC_API(int) PR_PendingException(void)
{
    PRThread *thread = _pr_current_thread;
    if (thread->asyncCall) {
	return 1;
    } else {
	return 0;
    }
}
#endif

PR_PUBLIC_API(int) PR_Suspend(PRThread *thread)
{
    int rv = 0;
    int is;

    is = _PR_IntsOff();
    switch (thread->state) {
      case _PR_UNBORN:
      case _PR_ZOMBIE:
      case _PR_SUSPENDED:
	rv = -1;
	break;

      case _PR_RUNNING:
	thread->state = _PR_SUSPENDED;
	PR_APPEND_LINK(&thread->runqLinks, &_pr_suspendq);
	_PR_SwitchThread(thread);
	break;

      case _PR_RUNNABLE:
	thread->state = _PR_SUSPENDED;
	PR_REMOVE_LINK(&thread->runqLinks);
	if (PR_CLIST_IS_EMPTY(&_pr_runq[thread->priority])) {
	    _pr_runq_ready_mask &= ~(1L << thread->priority);
	}
	PR_APPEND_LINK(&thread->runqLinks, &_pr_suspendq);
	break;

      case _PR_MON_WAIT:
      case _PR_COND_WAIT:
      case _PR_SLEEPING:
	thread->flags |= _PR_SUSPENDING;
	break;
    }
    _PR_IntsOn(is, 0);
    return rv;
}

PR_PUBLIC_API(int) PR_Resume(PRThread *thread)
{
    int rv = 0;
    int needYield = 0;
    int is;

    is = _PR_IntsOff();
    switch (thread->state) {
      case _PR_UNBORN:
      case _PR_ZOMBIE:
      case _PR_RUNNING:
      case _PR_RUNNABLE:
	rv = -1;
	break;

      case _PR_SUSPENDED:
	PR_LOG(THREAD, out, ("Resume: %s on to runq at priority %d",
			     thread->name, thread->priority));
	thread->state = _PR_RUNNABLE;
	PR_REMOVE_LINK(&thread->runqLinks);
	PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[thread->priority]);
	_pr_runq_ready_mask |= (1L << thread->priority);
	if (thread->priority >= _pr_current_thread->priority) {
	    needYield = 1;
	}
	break;

      case _PR_MON_WAIT:
	if (thread->flags & _PR_SUSPENDING) {
	    PRMonitor *mon;

	    /* Clear suspending flag because we aren't trying to anymore */
	    thread->flags &= ~_PR_SUSPENDING;

	    /* If monitor is idle, give it to the thread */
	    PR_ASSERT(thread->monitor != 0);
	    PR_ASSERT(thread->monitorEntryCount > 0);
	    mon = thread->monitor;
	    if (mon->owner == 0) {
		PR_ASSERT(mon->count == 0);
		needYield = _PR_MonNotify(mon, NULL);
	    }
	} else {
	    rv = -1;
	}
	break;

      default:
	if (thread->flags & _PR_SUSPENDING) {
	    /* Pretend it didn't happen */
	    thread->flags &= ~_PR_SUSPENDING;
	} else {
	    rv = -1;
	}
	break;
    }
    _PR_IntsOn(is, needYield);
    return rv;
}

PR_PUBLIC_API(int) PR_GetThreadPriority(PRThread *thread)
{
    return thread->priority;
}

int
_PR_SetThreadPriority(PRThread* thread, int newPri)
{
    /* returns whether the thread needs to be rescheduled */
    int needYield = 0;
    PR_ASSERT(_pr_intsOff != 0);

    PR_LOG(THREAD, out, ("SetThreadPriority: %s set to %d",
			 thread->name, newPri));
    if (newPri != thread->priority) {
	int oldPri = thread->priority;
	thread->priority = newPri;
	switch (thread->state) {
	  case _PR_RUNNING:
	    /* Change my priority */
	    if (_pr_runq_ready_mask >> (newPri + 1)) {
		/* Somebody else now has a higher priority */
		needYield = 1;
	    }
	    break;

	  case _PR_RUNNABLE:
	    /* Move to different _pr_runq */
	    PR_LOG(THREAD, out,
		   ("SetThreadPriority: %s on to runq at priority %d",
		    thread->name, newPri));
	    PR_REMOVE_LINK(&thread->runqLinks);
	    if (PR_CLIST_IS_EMPTY(&_pr_runq[oldPri])) {
		_pr_runq_ready_mask &= ~(1L << oldPri);
	    }
	    PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[newPri]);
	    _pr_runq_ready_mask |= (1L << newPri);
	    if (newPri > _pr_current_thread->priority) {
		needYield = 1;
	    }
	    break;
	}
    }
    return needYield;
}

PR_PUBLIC_API(void)
PR_SetThreadPriority(PRThread *thread, int newPri)
{
    int needYield;
    int is;

    _CLAMP_PRIORITY(newPri);

    is = _PR_IntsOff();
    needYield = _PR_SetThreadPriority(thread, newPri);
    _PR_IntsOn(is, needYield);
}

PR_PUBLIC_API(int) PR_ActiveCount(void)
{
    return _pr_user_active + _pr_system_active;
}

PR_PUBLIC_API(int) PR_EnumerateThreads(PREnumerator func, void *arg)
{
    int i, j, rv;
    PRCList *qp;
    int is;

    is = _PR_IntsOff();
    PR_BEGIN_SCANNING_THREADS();
    i = 0;
    rv = (*func)(_pr_current_thread, i, arg);
    if (rv == 0) goto done;
    i++;

    for (j = 0; j < 32; j++) {
	qp = _pr_runq[j].next;
	while (qp != &_pr_runq[j]) {
	    rv = (*func)(THREAD_PTR(qp), i, arg);
	    if (rv == 0) goto done;
	    i++;
	    qp = qp->next;
	}
    }
    qp = _pr_sleepq.next;
    while (qp != &_pr_sleepq) {
	rv = (*func)(THREAD_PTR(qp), i, arg);
	if (rv == 0) goto done;
	i++;
	qp = qp->next;
    }
    qp = _pr_suspendq.next;
    while (qp != &_pr_suspendq) {
	rv = (*func)(THREAD_PTR(qp), i, arg);
	if (rv == 0) goto done;
	i++;
	qp = qp->next;
    }
    qp = _pr_monitorq.next;
    while (qp != &_pr_monitorq) {
	rv = (*func)(THREAD_PTR(qp), i, arg);
	if (rv == 0) goto done;
	i++;
	qp = qp->next;
    }

  done:
    PR_END_SCANNING_THREADS();
    _PR_IntsOn(is, 0);
    return i;
}

/*
** Sleep until sleep expires. However, if an exception is pending don't
** sleep.
*/
PR_PUBLIC_API(void) PR_Sleep(int64 sleep)
{
    PRThread *thread = _pr_current_thread;
    int is;

    is = _PR_IntsOff();
    if (!thread->pendingException) {
	_PR_PutOnSleepQ(thread, sleep);
	thread->state = _PR_SLEEPING;
	_PR_SwitchThread(thread);
    }
    _PR_IntsOn(is, 0);
}

/* XXX if priv == 0 we should remove the ptd */
PR_PUBLIC_API(int) PR_SetThreadPrivate(PRThread *t, int32 id, void GCPTR *priv)
{
    PRPerThreadData **pptd = &t->ptd;
    PRPerThreadData *ptd, *newptd;
    int is, rv = 0;
    int needmalloc = 1;

    /* Assume that we will need to create a new per-thread-data structure.
       We do this because we can't invoke libc with interrupts locked */
    newptd = malloc(sizeof(PRPerThreadData));

    is = _PR_IntsOff();
    while ((ptd = *pptd) != 0) {
	if (ptd->id == id) {
	    ptd->priv = priv;
	    rv = 1;
	    needmalloc = 0;
	    goto done;
	}
	pptd = &ptd->next;
    }

    if (newptd) {
	newptd->id = id;
	newptd->priv = priv;
	newptd->next = 0;
	*pptd = newptd;
	rv = 1;
    }

  done:
    _PR_IntsOn(is, 0);
    if (!needmalloc && newptd) {
	free(newptd);
    }
    return rv;
}

PR_PUBLIC_API(void GCPTR *) PR_GetThreadPrivate(PRThread *t, int32 id)
{
    PRPerThreadData *ptd;
    void *priv = 0;
    int is;

    is = _PR_IntsOff();
    ptd = t->ptd;
    while (ptd) {
	if (ptd->id == id) {
	    priv = ptd->priv;
	    break;
	}
	ptd = ptd->next;
    }
    _PR_IntsOn(is, 0);
    return priv;
}


static PRMonitor *_pr_threadKeyMonitor;
static int32 _pr_threadKeyNumber;

void _PR_InitPrivateIDs(void)
{
    _pr_threadKeyMonitor = PR_NewMonitor(0);
    _pr_threadKeyNumber = 0;
}

PR_PUBLIC_API(int32) PR_NewThreadPrivateID(void)
{
    int32 ret;
    PR_EnterMonitor(_pr_threadKeyMonitor);
    ret = _pr_threadKeyNumber++;
    PR_ExitMonitor(_pr_threadKeyMonitor);
    return ret;
}

PR_PUBLIC_API(PRThread *) PR_CurrentThread(void)
{
    return _pr_current_thread;
}

PR_PUBLIC_API(void) PR_SetError(int oserror)
{
    PRThread *thread = _pr_current_thread;
    thread->errcode = oserror;
}

PR_PUBLIC_API(int) PR_GetError(void)
{
    PRThread *thread = _pr_current_thread;
    return thread->errcode;
}

PR_PUBLIC_API(char*) PR_GetErrorString(void)
{
    PRThread* thread;
    char* str;
    int is;

    is = _PR_IntsOff();
    thread = _pr_current_thread;
    str = thread->errstr;
    thread->errstr = NULL;
    _PR_IntsOn(is, 0);
    return (str ? str : "");
}

PR_PUBLIC_API(char *) PR_GetThreadName(PRThread *t)
{
    return t->name;
}

PR_PUBLIC_API(int32) PR_GetCurrentThreadID(void)
{
    PRThread *thread = _pr_current_thread;
    return thread->id;
}

PR_PUBLIC_API(int) PR_PendingException(PRThread *thread)
{
    return thread->pendingException;
}

PR_PUBLIC_API(void) PR_SetPendingException(PRThread *thread)
{
    int is;
    int needYield = 0;

    is = _PR_IntsOff();
    if (thread->pendingException++ == 0) {
	switch (thread->state) {
	  case _PR_COND_WAIT: {
	    /* Kick the thread out of COND_WAIT state */
	    PRMonitor* mon = thread->monitor;
	    PR_ASSERT(mon != 0);
	    _PR_NotifyOneThread(mon, thread);
	    if (mon->owner == 0) {
		if (_PR_MonNotify(mon, NULL)) {
		    needYield = 1;
		}
	    }
	    break;
	  }
	  case _PR_SLEEPING:
	    _PR_FixSleepQ(thread);
	    PR_REMOVE_LINK(&thread->runqLinks);

	    /* Put it back on the runQ */
	    PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[thread->priority]);
	    _pr_runq_ready_mask |= (1L << thread->priority);
	    thread->state = _PR_RUNNABLE;
	    if (thread->priority >= _pr_current_thread->priority) {
		needYield = 1;
	    }
	    break;
	}
    }
    _PR_IntsOn(is, needYield);
}

PR_PUBLIC_API(void) PR_ClearPendingException(PRThread *thread)
{
    thread->pendingException--;
}

#endif /* SW_THREADS */
