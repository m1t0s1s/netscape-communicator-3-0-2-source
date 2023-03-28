#ifndef prthread_h___
#define prthread_h___

#include "prclist.h"
#include "prmem.h"
#include "prlong.h"
#include "prmacros.h"
#include "prtypes.h"
#include "prtime.h"

typedef struct PRPerThreadDataStr PRPerThreadData;
typedef struct PRThreadStackStr PRThreadStack;

/*
** API for NSPR pre-emptible threads. On some architectures (MAC, WIN16)
** pre-emptibility is not guaranteed. See machdep.h for those machines
** for a macro that can be used to see if a pre-empt is desired.
*/

/*
** Number of different thread priorities...
*/
#define PR_NUM_PRIORITIES       32

/*
**      The main application's thread.  Initialized very early in the application's
**      startup.
*/
#ifdef XP_MAC
extern PRThread* mozilla_thread;
#endif

NSPR_BEGIN_EXTERN_C

/*
** Return the current thread object for the currently running code.
** Never returns NULL.
*/
extern PR_PUBLIC_API(PRThread *) PR_CurrentThread(void);

/*
** Create a new, unborn, thread object.
**      "name" the name of the thread (used for debugging)
**      "priority" the initial priority of the thread
**      "stackSize" the size of the stack, in bytes. The value can be zero
**         and then a machine specific stack size will be chosen.
**
** This can return NULL if some kind of error occurs, or if memory is
** tight. The thread is unborn and is unscheduled until PR_Start is
** called.
*/
extern PR_PUBLIC_API(PRThread *) PR_CreateThread(char *name, int priority,
                                               size_t stackSize);

/*
** Associate a thread object with an existing thread.
**      "name" the name of the thread (used for debugging)
**      "priority" the initial priority of the thread
**      "stack" a pointer to the bottom of the stack memory (e.g. the address
**         that would be returned from a malloc() call. Can be zero for the
**         "primordial" thread (the thread which is started initially by
**         the operating system).
**      "stackSize" the size of the stack, in bytes. The value can be zero
**         but only for the primordial stack.
**
** This can return NULL if some kind of error occurs, or if memory is
** tight. The thread is running and does not require PR_Start() to be
** called.
*/
extern PR_PUBLIC_API(PRThread *) PR_AttachThread(char *name, int priority,
                                               PRThreadStack *stk);

/*
** Destroy a thread. If the thread is unborn, it is killed
** immediately. If it is alive, it is descheduled and then destroyed.
*/
extern PR_PUBLIC_API(void) PR_DestroyThread(PRThread *thread);

/*
** Destroy the thread objec associated with a thread. The actual thread
** is not destroyed.
*/
extern PR_PUBLIC_API(void) PR_DetachThread(PRThread *thread);

/*
** Manually yield the processor to an equivalent priority thread.
*/
extern PR_PUBLIC_API(void) PR_Yield(void);

/*
** Make the current thread sleep until "sleep" amount of time has expired.
*/
extern PR_PUBLIC_API(void) PR_Sleep(int64 sleep);

/*
** Exit. Equivalent to PR_DestroyThread(PR_CurrentThread());
*/
extern PR_PUBLIC_API(void) PR_Exit(void);

/*
** Start a thread running. Can be called only once for a thread (the thread
** has to be in the unborn state for this to work). The thread will
** execute function "e" with arguments "o" and "a" when started.
**
** This call returns -1 if the thread is already started.
*/
extern PR_PUBLIC_API(int) PR_Start(PRThread *thread, void (*e)(void*,void*),
                                  void *o, void *a);

/*
** Execute the function "f(o,a)" in "thread's" execution context. This is
** done at the next available scheduling opportunity for thread.
**
** XXX does it interrupt the thread?
*/
extern PR_PUBLIC_API(int) PR_AsyncCall(PRThread *thread, void (*f)(void*,void*),
                                      void *o, void *a);

/*
** Increments the pending exception flag for a thread. This causes it to
** interrupt any i/o calls it might be lingering in and invoke the async
** call handler (if set). As long as the pending exception flag is set
** the thread will not block in i/o calls.
**
** This should have the effect of causing the thread to return from it's
** i/o calls and begin exception processing. The caller and the thread
** have to arrange for how this exception processing is done.
*/
extern PR_PUBLIC_API(void) PR_SetPendingException(PRThread *thread);

/*
** Return non-zero if the current thread has a pending exception
*/
extern PR_PUBLIC_API(int) PR_PendingException(PRThread *thread);

/*
** Decrements the pending exception flag for a thread. When the count
** goes back to zero the thread will again be able to block in the i/o
** routines.
*/
extern PR_PUBLIC_API(void) PR_ClearPendingException(PRThread *thread);

/*
** Suspend a thread's execution.
**
** XXX may not be nested (bug)
** XXX need to use counting semaphore semantics to make suspend/resume race free
*/
extern PR_PUBLIC_API(int) PR_Suspend(PRThread *thread);

/*
** Resume a thread's execution.
*/
extern PR_PUBLIC_API(int) PR_Resume(PRThread *thread);

/*
** Get the priority of the named thread to newPri.
*/
extern PR_PUBLIC_API(int) PR_GetThreadPriority(PRThread *thread);

/*
** Change the priority of the named thread to newPri.
*/
extern PR_PUBLIC_API(void) PR_SetThreadPriority(PRThread *thread, int newPri);

/*
** Return the number of active threads in the system. Active threads are
** threads that have been started (unborn threads are not counted), and
** are not dead.
*/
extern PR_PUBLIC_API(int) PR_ActiveCount(void);

/*
** Lock the scheduler. Prevent all other threads from running. This
** basically allows the current thread to enter a critical section that
** prevents all other threads from executing. The current thread can nest
** calls to this safely. A side effect of this is that the threads save
** their current register state in the thread context.
*/
extern PR_PUBLIC_API(void) PR_SingleThread(void);

/*
** Unlock the scheduler. Undoes the above lock.
*/
extern PR_PUBLIC_API(void) PR_MultiThread(void);

/*
** Enumeration function that applies "func(thread,i,arg)" to each active
** thread in the process. The enumerator returns 0 if the enumeration
** should stop or non-zero if the enumeration should continue.
*/
typedef int (*PREnumerator)(PRThread *t, int i, void *arg);
extern PR_PUBLIC_API(int) PR_EnumerateThreads(PREnumerator func, void *arg);

/*
** Define some per-thread-private data. id is a "unique" id used to
** distinguish each per-thread-private data type.
*/
extern PR_PUBLIC_API(int) PR_SetThreadPrivate(PRThread *t, int32 id,
                                             void GCPTR *priv);

/*
** Recover the per-thread-private data for thread t. id is a "unique" id
** used to distinguish each per-thread-private data type.
*/
extern PR_PUBLIC_API(void GCPTR *)PR_GetThreadPrivate(PRThread *t, int32 id);

/*
** Assign a new ID for use as a key for ThreadPrivateData calls. This key
** is guaranteed to be unique within this process.
*/
extern PR_PUBLIC_API(int32) PR_NewThreadPrivateID(void);

extern PR_PUBLIC_API(char *) PR_GetThreadName(PRThread *t);

extern PR_PUBLIC_API(int32) PR_GetCurrentThreadID(void);

extern PR_PUBLIC_API(int) PR_GetThreadPriority(PRThread *thread);

extern void _PR_InitPrivateIDs(void);

#ifdef SW_THREADS
extern int _pr_no_clock;

#ifdef PR_NO_PREEMPT
/*
** Set the interrupt switch hook. The hook function is invoked when an
** interrupt handler wants to switch to another thread (but can't because
** we are a no-premption system). The system invokes this function giving
** an external package a chance to do something locally.  The external
** function can basically store something into memory, and thats about
** it.
*/
extern PR_PUBLIC_API(void) PR_SetInterruptSwitchHook(void (*fn)(void *arg), void *arg);

extern int pr_want_resched;
#endif /* PR_NO_PREEMPT */
#endif /* SW_THREADS */

/*
** Gets the per-thread errno.
*/
extern PR_PUBLIC_API(int) PR_GetError(void);

/*
** Sets the per-thread errno.
*/
extern PR_PUBLIC_API(void) PR_SetError(int oserror);

/*
** Gets a per-thread error message string (if an error has
** occurred). Getting the error string causes the error string to be
** cleared. Currently, this is really only set by the prlink.h routines.
*/
extern PR_PUBLIC_API(char*) PR_GetErrorString(void);

/************************************************************************/

struct PRPerThreadDataStr {
    int32 id;
    void *priv;
    PRPerThreadData *next;
};

struct PRThreadStackStr {
    PRCList links;
    char *allocBase;            /* base of stack's allocated memory */
    size_t allocSize;           /* size of stack's allocated memory */
    char *stackBottom;          /* bottom of stack from C's point of view */
    char *stackTop;             /* top of stack from C's point of view */
    size_t stackSize;           /* size of usable portion of the stack */
    int flags;
#if defined(XP_PC) && !defined(_WIN32)
    char *SP;                   /* current SP of the thread (wrt.StackTop) */
#endif
};

#define _PR_THREAD_STACK_BUSY   0x1
#define _PR_THREAD_STACK_MAPPED 0x2

typedef void (*PRDumpThreadProc)(PRThread *t, FILE* out);
struct PRThreadStr {
    /*
    ** Linkage used when thread is on the global runQ, sleepQ, zombieQ,
    ** suspendQ, or monitorQ.
    */
    PRCList runqLinks;

    /* Linkage used when thread is on a monitor's waitQ/lockQ */
    PRCList waitLinks;

    /* List of all monitors that this thread has entered */
    PRCList monitors;

    /* Monitor/condition variable thread is waiting/sleeping on */
    PRMonitor *monitor;
    int monitorEntryCount;

    /* Remaining time to sleep */
    int64 sleep;

    char *name;
    PRThreadStack *stack;

    uint8 state;
    uint8 flags;
    uint8 priority;
    uint8 oldPriority;
    int32 id;

    void (*asyncCall)(void*,void*);
    void *asyncArg0;
    void *asyncArg1;
    void *asyncArg2;            /* Extra arg for thread startup */

    int pendingException;

    PRDumpThreadProc dump;

    PRPerThreadData *ptd;

    int errcode;                        /* Per-thread errno state */
    char* errstr;                       /* Per-thread error string */

    PR_CONTEXT_TYPE context;

#ifdef HW_THREADS
#if defined(XP_PC) && defined(_WIN32)
    HANDLE handle;
    HANDLE condEvent;
#endif
#endif  /* HW_THREADS */
};

/* thread->state values */
#define _PR_UNBORN      0
#define _PR_RUNNING     1
#define _PR_RUNNABLE    2
#define _PR_LOCK_WAIT   3
#define _PR_COND_WAIT   4
#define _PR_MON_WAIT    5
#define _PR_SLEEPING    6
#define _PR_SUSPENDED   7
#define _PR_ZOMBIE      8

/* thread->flag bits */
#define _PR_ON_SLEEPQ           0x1
#define _PR_SUSPENDING          0x2
#define _PR_SYSTEM              0x4
#define _PR_NO_SUSPEND          0x20

/*
** Macros used to get the thread pointer from a PRCList link. These
** macros handle the fact that the linkage cells are not always at the
** start of the thread struct.
*/
#define THREAD_PTR(_qp) \
    ((PRThread*) ((char*) (_qp) - offsetof(PRThread,runqLinks)))

#define WAITER_PTR(_qp) \
    ((PRThread*) ((char*) (_qp) - offsetof(PRThread,waitLinks)))

NSPR_END_EXTERN_C

#endif /* prthread_h___ */
