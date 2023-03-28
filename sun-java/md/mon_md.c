#include "oobj.h"
#include "interpreter.h"
#include "javaThreads.h"
#include "sys_api.h"
#include "bool.h"
#include "monitor_md.h"
#include "string.h"
#include "stdio.h"
#include "prlog.h"
#include "prdump.h"
#include "monitor.h"

#if 0

#define PR_MON_IN_CACHE	0x8000/* XXX */

int sysMonitorInit(sys_mon_t *mon, bool_t inCache)
{
    memset(mon, 0, sizeof(sys_mon_t));
    PR_InitMonitor(mon, 0, "java");
    if (inCache) {
	mon->flags |= PR_MON_IN_CACHE;/* XXX */
    }
    return SYS_OK;
}

/*
** This cannot be called. In the sun md code they call back into java
** (tsk tsk) and call monitorCleanup which would get us here. We don't
** need to do that because the lower level kernel can do it.
*/
int sysMonitorDestroy(sys_mon_t *mon, sys_thread_t *t)
{
    PR_NOT_REACHED("sysMonitorDestroy: shouldn't be called.");
    return 0;
}

int sysMonitorEnter(sys_mon_t *mon)
{
    PR_EnterMonitor(mon);
    return SYS_OK;
}

bool_t sysMonitorEntered(sys_mon_t *mon)
{
    return PR_InMonitor(mon);
}

int sysMonitorNotify(sys_mon_t *mon)
{
    return PR_Notify(mon);
}

int sysMonitorNotifyAll(sys_mon_t *mon)
{
    return PR_NotifyAll(mon);
}

int sysMonitorWait(sys_mon_t *mon, int milliseconds)
{
    int64 sleep, ms2us;

    if (milliseconds == SYS_TIMEOUT_INFINITY) {
	/* Sleep forever */
	sleep = LL_MAXINT;
    } else {
	/* Convert milliseconds to microseconds */ 
	LL_I2L(ms2us, 1000L);
	LL_UI2L(sleep, milliseconds);
	LL_MUL(sleep, sleep, ms2us);
    }
    return PR_Wait(mon, sleep);
}
#endif

void sysMonitorDumpInfo(sys_mon_t *mon)
{
}

void monitorCacheInit(void) { }
void monitorEnumerate(void (*f)(monitor_t*, void*), void *a) { }

/* XXX these should be throwing IllegalMonitorStateException's */

JRI_PUBLIC_API(void) monitorEnter(uint32 key)
{
    PR_CEnterMonitor((void*) key);
}

JRI_PUBLIC_API(void) monitorExit(uint32 key)
{
    PR_CExitMonitor((void*) key);
}

JRI_PUBLIC_API(void) monitorNotify(uint32 key)
{
    PR_CNotify((void*) key);
}

JRI_PUBLIC_API(void) monitorNotifyAll(uint32 key)
{
    PR_CNotifyAll((void*) key);
}

JRI_PUBLIC_API(void) monitorWait(uint32 key, int32_t milliseconds)
{
    int64 sleep, ms2us;
    int rv;

    if (milliseconds == SYS_TIMEOUT_INFINITY) {
	/* Sleep forever */
	sleep = LL_MAXINT;
    } else {
	/* Convert milliseconds to nanoseconds */ 
	LL_I2L(ms2us, PR_USEC_PER_MSEC);
	LL_UI2L(sleep, milliseconds);
	LL_MUL(sleep, sleep, ms2us);
    }
    rv = PR_CWait((void*) key, sleep);
    if (rv != SYS_OK) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "InternalError",
		    "monitorWait(): current thread not owner");
	return;
    }
    else if (PR_PendingException(PR_CurrentThread()) != 0) {
	ExecEnv *ee = (ExecEnv *) THREAD(threadSelf())->eetop;
	if (!exceptionOccurred(ee)) {
	    /* XXX Hmmm. Wonder how we got here? */
	    SignalError(ee, JAVAPKG "ThreadDeath", "rest in peace");
	}
	return;
    }
}

void monitorRegistryInit(void) { }

void registeredEnumerate(void (*f)(reg_mon_t*, void*), void *a) { }

void monitorRegister(sys_mon_t *mon, char *name)
{
    PR_InitMonitor(mon, 0, name);
}

int sysMonitorSizeof(void)
{
    return sizeof(sys_mon_t);
}

int sysMonitorEnter(sys_mon_t *mon)
{
    PR_EnterMonitor(mon);
    return SYS_OK;
}

int sysMonitorExit(sys_mon_t *mon)
{
    int rv = PR_ExitMonitor(mon);
    if (rv < 0)
	return SYS_ERR;
    return SYS_OK;
}
