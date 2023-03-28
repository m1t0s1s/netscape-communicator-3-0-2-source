#include "sys_api.h"
#include "threads_md.h"
#include "javaThreads.h"
#include "prlog.h"
#include "prglobal.h"
#include "prmon.h"
#include "prdump.h"
#include "finalize.h"
#ifdef XP_MAC
#include <Memory.h>
#endif
/* ID used to distinguish java's thread-private-data */
#define JAVA_TPD_ID 0xcafebabe

int soft_yield_count;

long ProcStackSize = 0;		/* Let machine dependent code pick it */

PRMonitor *pr_threadStartLock;

static char *noname = "_java";

/*
** ASSUMED to be the first thing called!
*/
void intrInit(void)
{
    PR_ASSERT(PR_CurrentThread() != 0);
    pr_threadStartLock = PR_NewNamedMonitor(0, "thread-start-lock");
}

int WaitToDie()
{
    PR_Exit();
#if !defined(XP_PC) || defined(_WIN32)
    PR_NOT_REACHED("Continuing after WaitToDie.");
#endif
    return 0;
}

#ifdef PR_NO_PREEMPT
void PR_CALLBACK force_yield(void *notused)
{
    ExecEnv *ee;
    
    /*	We have to get the ee ourself here... the function EE
    //	checks for NULL and throws an assert, which can actually
    //	happen here.  This is the case when a thread is dying
    //	so eetop gets set to zero, but on the MAC we get pre-empted
    //	and call force_yield.  It is okay to ingnore a NULL
    //	ee in this case because the thread is dying anyway, and
    //	a bonafide yield cannot be far off...
    */
    
    if (threadSelf() == 0)
		ee = DefaultExecEnv;
	else
		ee = (ExecEnv *) THREAD(threadSelf())->eetop;
		
	if (ee && !exceptionOccurred(ee)) {
		ee->exceptionKind = EXCKIND_YIELD;
    }
}
#endif

/*
** Create the primordial thread. It happened when PR_Init is called...
*/
int sysThreadBootstrap(sys_thread_t **tp)
{
    sys_thread_t *t = PR_CurrentThread();
    *tp = t;
    t->dump = sysThreadDumpInfo;
#ifdef PR_NO_PREEMPT
    PR_SetInterruptSwitchHook( (void (*)(void *)) force_yield, 0);
#endif
    return SYS_OK;
}

void sysThreadInitializeSystemThreads(void)
{
    InitializeFinalizerThread();
}

void PR_CALLBACK start_thread(void *o, void *a)
{
    void (*func)(void*) = (void (*)(void*)) o;

    /* Wait for creator thread to Resume us */
    PR_EnterMonitor(pr_threadStartLock);
    PR_ExitMonitor(pr_threadStartLock);

    /* And away we go... */
    (*func)(a);

    /* Bye bye */
    PR_Exit();
}

int sysThreadCreate(long stackSize, uint_t flags, 
		    void *(*func)(void *), 
		    sys_thread_t **tp,
		    void *cookie)
{
    PRThread *t;

#ifdef XP_MAC
	//	Make sure the Mac has at least 96K of
	//	free memory before starting up a thread...
	//	This is part of our "pre-flight" memory
	//	strategy.
	Handle	rainyDayBlock;
	rainyDayBlock = NewHandle(96 * 1024);
	if (rainyDayBlock != NULL)
		DisposeHandle(rainyDayBlock);
	else
		return SYS_NORESOURCE;
#endif

    /*
    ** Create thread at java priority 0. Adjust the java priority for
    ** NSPR priority levels
    */
    t = PR_CreateThread(noname, NormalPriority+10, stackSize);
    if (t) {
	sysThreadSetBackPtr(t, cookie);
	*tp = t;

	if (flags == THR_SYSTEM) {
	    t->flags |= _PR_SYSTEM;	/* XXX bug in NSPR API*/
	}
	/* Use a monitor to force thread to wait for us to resume it */
	PR_EnterMonitor(pr_threadStartLock);
	PR_Start(t, (void (*)(void*,void*))start_thread, func, cookie);
	PR_Suspend(t);
	PR_ExitMonitor(pr_threadStartLock);

	t->dump = sysThreadDumpInfo;

	return SYS_OK;
    }
    *tp = 0;
    return SYS_NORESOURCE;
}

int sysThreadKill(sys_thread_t *t)
{
    PR_DestroyThread(t);
    return SYS_OK;
}

void sysThreadExit(void)
{
    PRThread *t;
    JHandle *p;
    ExecEnv *eep;

    t = PR_CurrentThread();
    p = PR_GetThreadPrivate(t, JAVA_TPD_ID);

    /*
    ** Break association to hardware thread to avoid dangling pointer
    ** problems.
    */
    if (p) {
	THREAD(p)->PrivateInfo = 0;
 
	/* Delete java stack now that we are done with it */
	eep = (ExecEnv *) THREAD(p)->eetop;
	if (eep) {                		/* eep == 0 does happen... */
	    DeleteExecEnv(eep, p);		/* zeros eetop */
	}
    }

    PR_Exit();
}

sys_thread_t *sysThreadSelf(void)
{
    return PR_CurrentThread();
}

void sysThreadYield(void)
{
    PR_Yield();
}

int sysThreadSuspend(sys_thread_t *t)
{
    return PR_Suspend(t);
}

int sysThreadResume(sys_thread_t *t)
{
    /* HACK: reflect java's name for the thread back to NSPR */
    if (t->name &&
	(t->name[0] == '_') &&
	(t->name[1] == 'j') &&
	(t->name[2] == 'a') &&
	(t->name[3] == 'v') &&
	(t->name[4] == 'a') &&
	(t->name[5] == '\0')) {
	TID tid;
	char *name;

	tid = (TID) PR_GetThreadPrivate(t, JAVA_TPD_ID);
	name = Object2CString((JHandle *) THREAD(tid)->name);
	if (name) {
		/* If the thread already has a name, free it. */
		if (t->name)
			sysFree(t->name);
	    t->name = strdup(name);
	}
    }
    return PR_Resume(t);
}

int sysThreadSetPriority(sys_thread_t *t, int32_t n)
{
    int nsprPrio = n + 10;
    PR_ASSERT(java_lang_Thread_MIN_PRIORITY <= n && n <= java_lang_Thread_MAX_PRIORITY);
    PR_ASSERT(nsprPrio < PR_NUM_PRIORITIES);
    PR_SetThreadPriority(t, nsprPrio);
    return SYS_OK;
}

int sysThreadGetPriority(sys_thread_t *t, int32_t *io)
{
    int nsprPrio = PR_GetThreadPriority(t);
    int n = nsprPrio - 10;
    PR_ASSERT(nsprPrio < PR_NUM_PRIORITIES);
    PR_ASSERT(java_lang_Thread_MIN_PRIORITY <= n && n <= java_lang_Thread_MAX_PRIORITY);
    *io = n;
    return SYS_OK;
}

/* XXX bogus: PR_GetSP should deal with current thread */
void *sysThreadStackPointer(sys_thread_t *t)
{
    if (t == PR_CurrentThread()) {
	return (void*) &t;
    } else {
	return (void*) PR_GetSP(t);
    }
}
 
stackp_t sysThreadStackBase(sys_thread_t *t)
{
    return (stackp_t) t->stack;
}
 
#if 0
int sysThreadSingle(void)
{
    PR_SingleThread();
    return SYS_OK;
}

void sysThreadMulti(void)
{
    PR_MultiThread();
}
#endif

struct FooFooStr {
    int (*func)(sys_thread_t *, void *);
    void *arg;
};

static int EnumOneThread(PRThread *t, int i, void *arg)
{
    struct FooFooStr *fp = (struct FooFooStr*) arg;
    void *handle;

    /* Only scan over java threads (skip the non-java threads) */
    handle = PR_GetThreadPrivate(t, JAVA_TPD_ID);
    if (handle) {
	return (*fp->func)(t, fp->arg);
    }
    return 1;
}

int sysThreadEnumerateOver(int (*func)(sys_thread_t *, void *), void *arg)
{
    struct FooFooStr foo;

    foo.func = func;
    foo.arg = arg;
    PR_EnumerateThreads(EnumOneThread, &foo);
    return SYS_OK;
}

void sysThreadSetBackPtr(sys_thread_t *t, void *cookie)
{
    if (!PR_SetThreadPrivate(t, JAVA_TPD_ID, cookie)) {
	/* Ran out of memory ! */
	abort();/* XXX */
    }
}

void *sysThreadGetBackPtr(sys_thread_t *t)
{
    return PR_GetThreadPrivate(t, JAVA_TPD_ID);
}

void sysThreadInit(sys_thread_t *t, stackp_t sb)
{
}

int sysThreadCheckStack(void)
{
    /* XXX stack is always... OK */
    return 1;
}

void
sysThreadPostException(sys_thread_t *t, void *exc) 
{ 
    TID tid;
    struct execenv *ee;

    tid = (TID) PR_GetThreadPrivate(t, JAVA_TPD_ID);

#if 0
    {
	char *name = t->name;

	if (name == noname) {
	    /* Get java's name */
	    name = Object2CString((JHandle *) THREAD(tid)->name);
	}
	fprintf(stderr,
	   "attempting to kill thread %s using sysThreadPostException...\n",
	   name);
    }
#endif

    ee = (ExecEnv *) THREAD(tid)->eetop;
    PR_ASSERT(ee != 0);

    if (ee != (ExecEnv *) 0) {   
	exceptionThrow(ee, (JHandle*) exc);
    }
    PR_SetPendingException(t);
    PR_Yield();
} 

JAVA_PUBLIC_API(void)
sysThreadDumpInfo(sys_thread_t* t, FILE* out)
{
    TID tid = (TID)sysThreadGetBackPtr(t);
    if (tid)
    {
	JavaFrame* frame;
	struct Classjava_lang_Thread* thread = THREAD(tid);
	ExecEnv* ee = (ExecEnv*)thread->eetop;
	struct Hjava_lang_ThreadGroup* group = thread->group;
	if (group)
	{
	    char name[256];
	    javaString2CString(unhand(group)->name, name, sizeof(name));
	    fprintf(out, "    in group %s\n", name);
	}
	else {
	    fprintf(out, "    no group\n");
	}
	if (ee) {
	    for (frame = ee->current_frame; frame != NULL; frame = frame->prev)
	    {
		char where[256];
		pc2string(frame->lastpc, 0, where, where + sizeof(where));
		fprintf(out, "\t%s\n", where);
	    }
	}
    }
}
