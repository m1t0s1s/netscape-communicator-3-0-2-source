#ifdef HW_THREADS
#include <prmem.h>
#include <prlog.h>
#include <prdump.h>
#include <prfile.h>
#include "hwkern.h"
#include "mdint.h"

PR_LOG_DEFINE(THREAD);

/*
** Software threading package. Designed to run on top of MULTIPLE native
** OS threads.
*/
PR_PUBLIC_API(DWORD) PR_GetSP( PRThread * thread )
{
    CONTEXT context;
    DWORD thread_sp = 0;
    int count = 0;

again:
    context.ContextFlags = CONTEXT_CONTROL;
    if( GetThreadContext( thread->handle, &context ) ) {
        thread_sp = context.Esp;
    }
    /*
    ** This check is necessary for Win95 which sometimes leaves a thread
    ** in a state where its esp IS NOT valid!  If this happens, try it
    ** a few more times... If esp is still bogus, then return the max
    ** valid stack.  This will keep the GC from getting corrupted.
    */
    if( (thread_sp > (DWORD)thread->stack->stackTop) ||
            thread_sp < (DWORD)(thread->stack->stackBottom) ) {
        PR_LOG(THREAD, out,
               ("SP is bogus: %x.  Trying again!", thread_sp));
        if( ++count < 50 ) {
            Sleep(0);
            goto again;
        } else {
            thread_sp = (DWORD)((prword_t *)thread->stack->stackTop - 0x2000);
/*          thread_sp = (DWORD)((prword_t *)thread->stack->stackBottom + 1); */
        }
    }

    return thread_sp;
}

PR_PUBLIC_API(PRThread *) PR_CreateThread(char *name, int priority,
                                        size_t stackSize)
{
    PRThread *t;

    t = (PRThread*) calloc(1, sizeof(PRThread));
    if (t) {
        PR_INIT_CLIST(&t->monitors);
        PR_INIT_CLIST(&t->runqLinks);
        PR_INIT_CLIST(&t->waitLinks);

        t->state    = _PR_UNBORN;
        t->priority = priority;

        /* Set up the thread stack information */
        if( !_MD_NewStack(t, stackSize) ) {
            free(t);
            t = 0;
            goto done;
        }

        /* Copy the name of the thread  */
        t->name = strdup(name);
        if( !t->name ) {
            free(t);
            t = 0;
            goto done;
        }

        /* Create the Event object used by condition variables */
        t->condEvent = CreateEvent(NULL, FALSE, FALSE, NULL );
        if (!t->condEvent) {
            free(t->name);
            free(t);
            t = 0;
            goto done;
        }

    }
done:
    return t;
}

PR_PUBLIC_API(void) PR_DestroyThread(PRThread *thread)
{
    PRPerThreadData **pptd, *ptd;
    HANDLE hdl;
    PRMonitor *mon;

    LOCK_SCHEDULER();
    hdl = thread->handle;

    /*
    ** First, suspend the thread (unless it's the active one)
    */
    if ( thread != _pr_current_thread ) {
#if defined( XP_PC ) && defined( _WIN32 )
        SuspendThread( hdl );
#endif  /* XP_PC && _WIN32 */
    }
     PR_LOG(THREAD, out,
            ("%s(0X%x)[DestroyThread]", thread->name, thread));
    PR_REMOVE_LINK(&thread->runqLinks); /* take off _pr_runq */

    /*
    ** Release any monitors held by the thread...
    */
    while (thread->monitors.next != &thread->monitors) {
        mon = MONITOR_PTR(thread->monitors.next);
        PR_REMOVE_AND_INIT_LINK(&mon->links);
        mon->owner = 0;
        mon->count = 0;

        /* release the monitor */
         PR_LOG(THREAD, out,
                ("%s(0X%x)[Releasing Monitor] %s", thread->name, thread,
                 mon->name));
        LeaveCriticalSection( &(mon->mutexHandle) );
    }

    if (thread->flags & _PR_SYSTEM) {
        _pr_system_active--;
        PR_ASSERT(_pr_system_active >= 0);
    } else {
        _pr_user_active--;
        PR_ASSERT(_pr_user_active >= 0);
    }

    /* Free up memory and we're gone... */
    _MD_FreeStack(thread);

    /* XXX: this assumes that ptd->priv is GC'able memory or static */
    pptd = &thread->ptd;
    while ((ptd = *pptd) != 0) {
        *pptd = ptd->next;
        free(ptd);
    }

    /* Free up the name */
    if (thread->name) {
        free(thread->name);
        thread->name = 0;
    }

    /* Free the Event object used by condition variables */
    if (thread->condEvent) {
        CloseHandle(thread->condEvent);
    }

    /* Now we can free up the thread object */
    free( thread );

    if (_pr_user_active == 0) {
        UNLOCK_SCHEDULER();
        exit(0);
    }
    UNLOCK_SCHEDULER();

#if defined( XP_PC ) && defined( _WIN32 )
    if ( thread != _pr_current_thread ) {
        TerminateThread( hdl, 0L );
    } else {
        CloseHandle(hdl);
        ExitThread(0);
    }
#endif  /* XP_PC && _WIN32 */
}

PR_PUBLIC_API(void) PR_Exit(void)
{
    PR_LOG_FLUSH();
    PR_DestroyThread( _pr_current_thread );
    PR_NOT_REACHED("Continuing after exit.");
}

PR_PUBLIC_API(void) PR_Yield(void)
{
    Sleep(0);
}

/*
** Intermediate that invokes startup routine for thread. Hoptoad is executed
** in the context of the new thread...
*/
#ifdef _WIN32
static DWORD Win32_HopToad( PRThread *thread )
{
    _pr_current_thread = thread;

    /* Set up the thread stack information */
    _MD_InitializeStack(thread->stack);

    thread->state = _PR_RUNNING;
#if 1
    __try {
        (thread->asyncCall)(thread->asyncArg1, thread->asyncArg2);
    }
    __except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                                    ? EXCEPTION_EXECUTE_HANDLER
                                    : EXCEPTION_CONTINUE_SEARCH ) {
#ifdef DEBUG
        const char *msg = "This thread has caused an access violation.  It will be terminated\n";

        (void)MessageBox(NULL, msg, "Internal Java Error", MB_OK);
        DebugBreak();
#endif  /* DEBUG */
        PR_Exit();
    }
#else
    (thread->asyncCall)(thread->asyncArg1, thread->asyncArg2);
#endif
    PR_Exit();

    PR_NOT_REACHED("Exiting HopToad.");
    return 0L;
}
#endif /* _WIN32 */

PR_PUBLIC_API(int) PR_Start(PRThread *thread, void (*e)(void*,void*),
                           void *o, void *a)
{
    int rv;
    int pri;

    LOCK_SCHEDULER();

    rv = -1;
    if (thread->state != _PR_UNBORN ||
        (thread->priority > 31 || thread->priority < 0)) {
        goto done;
    }

    thread->asyncCall = e;
    thread->asyncArg1 = o;
    thread->asyncArg2 = a;

    /* Initialize and start up the new thread */
#if defined( XP_PC ) && defined( _WIN32 )
    thread->handle = CreateThread(
                    NULL,                                 /* security attrib */
                    0,                                    /* def. stack size */
                    (LPTHREAD_START_ROUTINE)Win32_HopToad,/* startup routine */
                    thread,                               /* thread param    */
                    CREATE_SUSPENDED,                     /* create flags    */
                    &(thread->id) );                      /* thread id       */

    if (thread->handle) {
        /* Update count of active threads */
        if (thread->flags & _PR_SYSTEM) {
            _pr_system_active++;
        } else {
            _pr_user_active++;
        }

        /*
        ** Set the thread priority.  This will also place the thread on
        ** the runQ.
        **
        ** Force PR_SetThreadPriority to set the priority by
        ** setting thread->priority to 100.
        */
        pri = thread->priority;
        thread->priority = 100;
        PR_SetThreadPriority( thread, pri );

        PR_LOG(THREAD, out,
               ("%s(0X%x)[Start]: on to runq at priority %d",
                thread->name, thread, thread->priority));

        /* Activate the thread */
        if ( ResumeThread( thread->handle ) != 0xFFFFFFFF ) {
            rv = 0;
        }
    }
#endif  /* XP_PC && _WIN32 */

done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(int) PR_Suspend(PRThread *thread)
{
    int rv = 0;

    LOCK_SCHEDULER();

    PR_LOG(THREAD, out,
           ("%s(0X%x)[Suspend]: on to runq at priority %d",
            thread->name, thread, thread->priority));
    switch (thread->state) {
      case _PR_UNBORN:
      case _PR_ZOMBIE:
      case _PR_SUSPENDED:
        rv = -1;
        break;

      case _PR_RUNNING:
        thread->state = _PR_SUSPENDED;

        /* Release the thread lock if the current thread is suspending */
        if( thread == _pr_current_thread ) {
            UNLOCK_SCHEDULER();
        }
        /*
        ** Suspend the thread...
        */
        SuspendThread( thread->handle );

        /* Get the thread lock if the current thread just woke up */
        if( thread == _pr_current_thread ) {
            LOCK_SCHEDULER();
        }
        break;

      case _PR_SLEEPING:
      case _PR_COND_WAIT:
      case _PR_MON_WAIT:
        if( !(thread->flags & _PR_SUSPENDING) ) {
            SuspendThread( thread->handle );
            thread->flags |= _PR_SUSPENDING;
        } else {
            rv = -1;
        }
        break;
    }

    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(int) PR_Resume(PRThread *thread)
{

    int rv = 0;

    LOCK_SCHEDULER();

    switch (thread->state) {
      case _PR_UNBORN:
      case _PR_ZOMBIE:
      case _PR_RUNNING:
        rv = -1;
        break;

      case _PR_SUSPENDED:
        PR_LOG(THREAD, out,
               ("%s(0X%x)[Resume]: on to runq at priority %d",
                thread->name, thread, thread->priority));
        thread->state = _PR_RUNNING;
        ResumeThread( thread->handle );
        break;

      case _PR_SLEEPING:
      case _PR_COND_WAIT:
      case _PR_MON_WAIT:
        if (thread->flags & _PR_SUSPENDING) {
           PR_LOG(THREAD, out,
                  ("%s(0X%x)[Resume]: Thread is now waiting",
                   thread->name, thread));
            thread->flags &= ~_PR_SUSPENDING;
            ResumeThread( thread->handle );
        } else {
            rv = -1;
        }
        break;
    }

    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(int) PR_GetThreadPriority(PRThread *thread)
{
    return thread->priority;
}

PR_PUBLIC_API(void) PR_SetThreadPriority(PRThread *thread, int newPri)
{
    LOCK_SCHEDULER();

    if (newPri < 0) {
        newPri = 0;
    } else if (newPri >= PR_NUM_PRIORITIES) {
        newPri = PR_NUM_PRIORITIES - 1;
    }

    if (newPri != thread->priority) {
        thread->priority = newPri;

        /* Move to different _pr_runq */
        PR_REMOVE_LINK(&thread->runqLinks);
        PR_APPEND_LINK(&thread->runqLinks, &_pr_runq[newPri]);

        /* Set the thread priority */
        if( newPri < 4 ) {
            newPri = THREAD_PRIORITY_IDLE;
        } else if( newPri < 8 ) {
            newPri = THREAD_PRIORITY_LOWEST;
        } else if( newPri < 12 ) {
            newPri = THREAD_PRIORITY_BELOW_NORMAL;
        } else if( newPri < 16 ) {
            newPri = THREAD_PRIORITY_NORMAL;
        } else if( newPri < 24 ) {
            newPri = THREAD_PRIORITY_ABOVE_NORMAL;
        } else if( newPri < 28 ) {
            newPri = THREAD_PRIORITY_HIGHEST;
        } else if( newPri < 32 ) {
            newPri = THREAD_PRIORITY_TIME_CRITICAL;
        }
        PR_LOG(THREAD, out,
               ("%s(0X%x)[SetThreadPriority]: on to runq at priority %d",
                thread->name, thread, thread->priority));
        if( ! SetThreadPriority( thread->handle, newPri ) ) {
            PR_LOG(THREAD, out,
                   ("PR_SetThreadPriority: can't set thread priority"));
        }
    }

    UNLOCK_SCHEDULER();
}

PR_PUBLIC_API(int) PR_ActiveCount(void)
{
    return _pr_user_active + _pr_system_active;
}

PR_PUBLIC_API(int) PR_EnumerateThreads(PREnumerator func, void *arg)
{
    int i, j, rv;
    PRCList *qp;
    PRThread *t;

    LOCK_SCHEDULER();
    i = 0;
    for (j = 0; j < PR_NUM_PRIORITIES; j++) {
        qp = _pr_runq[j].next;
        while (qp != &_pr_runq[j]) {
            t = THREAD_PTR(qp);
            rv = (*func)(t, i, arg);
            if (rv == 0) goto done;
            i++;
            qp = qp->next;
        }
    }

  done:
    UNLOCK_SCHEDULER();
    return i;
}

/*
** Sleep for an amount of time.
**
** The sleep time is represented in microseconds...
*/
PR_PUBLIC_API(void) PR_Sleep(int64 sleep)
{
    PRThread *thread;
    unsigned long waitTime;

    PR_ASSERT(LL_GE_ZERO(sleep));
    thread = PR_CurrentThread();

    LOCK_SCHEDULER();

    if( !thread->pendingException ) {
        PR_LOG(THREAD, warn,
               ("put %s(0X%x) on sleepQ for %lld",
                thread->name, thread, sleep));

        thread->flags |= _PR_ON_SLEEPQ;
        thread->sleep = sleep;
        thread->state = _PR_SLEEPING;

        /*
         * Convert the sleep time from an int64 in microseconds to
         * an int32 in milliseconds
         */
        LL_L2UI( waitTime, sleep );
        waitTime = waitTime / PR_USEC_PER_MSEC;

        UNLOCK_SCHEDULER();

        /*
        ** Put the thread to sleep for the requested time...
        */
#if defined( XP_PC ) && defined( _WIN32 )
        WaitForSingleObject( thread->condEvent, waitTime );
#endif  /* XP_PC && _WIN32 */

        LOCK_SCHEDULER();

        /*
        ** The thread is now awake again...
        */
        PR_LOG(THREAD, warn,
               ("thread=%s(0X%x) done sleeping; state=%d",
                thread->name, thread, thread->state));
        thread->state = _PR_RUNNING;
    }

    UNLOCK_SCHEDULER();
}

PR_PUBLIC_API(PRThread *) PR_AttachThread(char *name, int priority,
                                        PRThreadStack *stack)
{
    PRThread *thread;

    LOCK_SCHEDULER();

    thread = (PRThread *) calloc( 1, sizeof(PRThread) );
    if( thread ) {
        /* Perform the PR_Create() initializations */
        PR_INIT_CLIST(&thread->monitors);
        PR_INIT_CLIST(&thread->runqLinks);

        thread->name = strdup(name);
        if( ! thread->name ) {
            free(thread);
            thread = 0;
            goto done;
        }

        /* Create the Event object used by condition variables */
        thread->condEvent = CreateEvent(NULL, FALSE, FALSE, NULL );
        if (!thread->condEvent) {
            free(thread);
            thread = 0;
            goto done;
        }

        thread->state    = _PR_RUNNING;
        thread->id       = GetCurrentThreadId();

        /*
        ** Warning:
        ** --------
        ** NSPR requires a real handle to every thread.  GetCurrentThread()
        ** returns a pseudo-handle which is not suitable for some thread
        ** operations (ie. suspending).  Therefore, get a real handle from
        ** the pseudo handle via DuplicateHandle(...)
        */
        DuplicateHandle( GetCurrentProcess(),     /* Process of source handle   */
                         GetCurrentThread(),      /* Pseudo Handle to duplicate */
                         GetCurrentProcess(),     /* Process of target handle   */
                         &(thread->handle),       /* resulting handle           */
                         0L,                      /* access flags               */
                         FALSE,                   /* Inheritable                */
                         DUPLICATE_SAME_ACCESS ); /* Options                    */

        /* Perform all of the HopToad() initializations */
        _pr_current_thread = thread;

        /* Set up the thread stack information */
        _MD_InitializeStack(stack);
        thread->stack = stack;

        /*
        ** Set the thread priority.  This will also place the thread
        ** on the runQ
        **
        ** Force PR_SetThreadPriority to set the priority by
        ** setting thread->priority to 100.
        */
        thread->priority = 100;
        PR_SetThreadPriority( thread, priority );


        PR_LOG(THREAD, out,
               ("%s(0X%x)[Start]: on to runq at priority %d",
                thread->name, thread, thread->priority));

        /* Update count of active threads */
        if (thread->flags & _PR_SYSTEM) {
            _pr_system_active++;
        } else {
            _pr_user_active++;
        }
    }

done:
    UNLOCK_SCHEDULER();

    return thread;
}

PR_PUBLIC_API(void) PR_DetachThread(PRThread *thread)
{
    HANDLE hdl;

    LOCK_SCHEDULER();

    hdl = thread->handle;
    PR_LOG(THREAD, out,
           ("%s(0X%x)[DetachThread]", thread->name, thread));
    PR_REMOVE_LINK(&thread->runqLinks); /* take off runQ */

    if (thread->flags & _PR_SYSTEM) {
        _pr_system_active--;
        PR_ASSERT(_pr_system_active >= 0);
    } else {
        _pr_user_active--;
        PR_ASSERT(_pr_user_active >= 0);
    }

    /* Free the storage for the thread name */
    if( thread->name ) {
        free(thread->name);
        thread->name = 0;
    }

    /* Free the storage for the thread structure */
    free( thread );

    /* Release the handle that was duplicated in PR_AttachThread(...) */
    CloseHandle( hdl );

    if (_pr_user_active == 0) {
        UNLOCK_SCHEDULER();
        exit(0);
    }
    UNLOCK_SCHEDULER();
}

PR_PUBLIC_API(int) PR_SetThreadPrivate(PRThread *t, int32 id, void GCPTR *priv)
{
    int rv;
    PRPerThreadData **pptd = &t->ptd;
    PRPerThreadData *ptd;

    rv = 0;
    LOCK_SCHEDULER();
    while ((ptd = *pptd) != 0) {
        if (ptd->id == id) {
            ptd->priv = priv;
            rv = 1;
            goto done;
        }
        pptd = &ptd->next;
    }
    ptd = calloc(1, sizeof(PRPerThreadData));/* XXX switch to GC heap */
    if (ptd) {
        ptd->id = id;
        ptd->priv = priv;
        ptd->next = 0;
        *pptd = ptd;
        rv = 1;
        goto done;
    }

done:
    UNLOCK_SCHEDULER();
    return rv;
}

PR_PUBLIC_API(void) GCPTR *PR_GetThreadPrivate(PRThread *t, int32 id)
{
    void GCPTR *private;
    PRPerThreadData *ptd;

    LOCK_SCHEDULER();

    ptd = t->ptd;
    private = 0;
    while (ptd) {
        if (ptd->id == id) {
            private = ptd->priv;
            break;
        }
        ptd = ptd->next;
    }

    UNLOCK_SCHEDULER();
    return private;
}

PR_PUBLIC_API(PRThread *) PR_CurrentThread()
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
    PRThread *thread = _pr_current_thread;
    char* str;

    LOCK_SCHEDULER();
    str = thread->errstr;
    thread->errstr = NULL;
    UNLOCK_SCHEDULER();
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
    LOCK_SCHEDULER();

     PR_LOG(THREAD, out,
            ("%s(0X%x)[SetPendingException]", thread->name, thread));

    if( thread->pendingException++ == 0 ) {
        switch (thread->state) {
        case _PR_COND_WAIT: {
            /* Kick the thread out of COND_WAIT state */
            PR_REMOVE_AND_INIT_LINK(&thread->waitLinks); /* remove from condQ */
            PulseEvent( thread->condEvent );
            PR_LOG(THREAD, out,
                   ("%s(0X%x)[SetPendingException] kick out of CondWait",
                    thread->name, thread));
            break;
        }

        case _PR_SLEEPING:
            /* Kick the thread out of Sleep state */
            PulseEvent( thread->condEvent );
            PR_LOG(THREAD, out,
                   ("%s(0X%x)[SetPendingException] kick out of Sleep",
                    thread->name, thread));
            break;
        }
    }

    UNLOCK_SCHEDULER();
}

PR_PUBLIC_API(void) PR_ClearPendingException(PRThread *thread)
{
    LOCK_SCHEDULER();

    PR_LOG(THREAD, out,
           ("%s(0X%x)[ClearPendingException]", thread->name, thread));
    thread->pendingException--;

    UNLOCK_SCHEDULER();
}

#endif  /* HW_THREADS */

