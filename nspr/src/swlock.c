#ifdef SW_THREADS
#include "prlock.h"
#include "prlog.h"
#include "prmem.h"
#include "swkern.h"

/*
** XXX need per-thread list of locks thread owns so that they can get
** XXX fixed up when thread is zapped
**
** XXX this is work in progress; they aren't finished yet, and I'm not
** XXX sure we need them since monitors will suffice
*/

PR_PUBLIC_API(PRLock *) PR_NewLock(int locked)
{
    PRLock *lock;

    lock = PR_NEW(PRLock);
    if (lock) {
	PR_INIT_CLIST(&lock->waitQ);
	if (locked) {
	    lock->owner = _pr_current_thread;
	} else {
	    lock->owner = 0;
	}
    }
    return lock;
}

PR_PUBLIC_API(void) PR_InitLock(PRLock *lock, int locked)
{
    PR_ASSERT(lock->waitQ.next == 0);
    PR_INIT_CLIST(&lock->waitQ);
    if (locked) {
	lock->owner = _pr_current_thread;
    } else {
	lock->owner = 0;
    }
}

PR_PUBLIC_API(void) PR_DestroyLock(PRLock *lock)
{
    PR_ASSERT(PR_CLIST_IS_EMPTY(&lock->waitQ));
    PR_ASSERT(lock->owner == 0);
    free(lock);
}

PR_PUBLIC_API(void) PR_Lock(PRLock *lock)
{
    PRThread *me = _pr_current_thread;
    int is;

    is = _PR_IntsOff();
    for (;;) {
	if (lock->owner == 0) {
	    lock->owner = me;
	    break;
	}
	/* XXX who deals with priority inversion? */
	PR_APPEND_LINK(&me->runqLinks, &lock->waitQ);
	me->state = _PR_LOCK_WAIT;
	_PR_SwitchThread(me);
    }
    _PR_IntsOn(is, 0);
}

PR_PUBLIC_API(void) PR_Unlock(PRLock *lock)
{
    PRThread *me = _pr_current_thread;
    PRThread *best, *waiter;
    int bestPri;
    PRCList *qp;
    int needYield = 0;
    int is;

    is = _PR_IntsOff();

    PR_ASSERT(lock->owner == me);

    best = 0;
    bestPri = -1;
    lock->owner = 0;
    qp = lock->waitQ.next;
    while (qp != &lock->waitQ) {
	waiter = THREAD_PTR(qp);
	qp = waiter->runqLinks.next;
	if (waiter->flags & _PR_SUSPENDING) {
	    /*
	    ** Don't wakeup waiters that are going to be suspended once
	    ** they get the lock.
	    */
	} else if ((int)waiter->priority > bestPri) {
	    /* Found a higher priority waiter */
	    best = waiter;
	    bestPri = (int) waiter->priority;
	}
    }
    if (best) {
	/*
	** Take best waiter off of the lock's waitQ and put it onto the
	** runq.
	*/
	PR_REMOVE_AND_INIT_LINK(&best->runqLinks);
	_pr_runq_ready_mask |= (1L << bestPri);
	PR_APPEND_LINK(&best->runqLinks, &_pr_runq[bestPri]);
	needYield = bestPri > me->priority;
    }

    /* XXX who deals with undoing priority inversion? */
    _PR_IntsOn(is, needYield);
}

#endif /* SW_THREADS */
