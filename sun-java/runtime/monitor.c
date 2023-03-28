/*
 * @(#)monitor.c	1.29 95/12/03
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*
 * System-independent, reentrant monitors.
 */

#include <monitor.h>
#include <monitor_md.h>
#include <threads_md.h>
#include <monitor_cache.h>
#include <javaThreads.h>
#include <log.h>
#include <sys_api.h>

#include <java_lang_Thread.h>

/*-
 * Must be called for each monitor prior to use.
 */
void 
monitorInit(monitor_t *mon) 
{
    sysMonitorInit(&sysmon(mon), TRUE);
}

/*-
 * Look up a monitor in the monitor cache and enter it using
 * monitorEnter().  If there is no monitor cached for the given key
 * create one on the fly.
 */
JRI_PUBLIC_API(void)
monitorEnter(unsigned int key)
{
    monitor_t *mid = createMonitor(key);

    sysAssert(mid != MID_NULL);
    sysMonitorEnter(&sysmon(mid));
    CACHE_LOCK();
    mid->uninit_count--;
    CACHE_UNLOCK();
}

JRI_PUBLIC_API(void)
monitorExit(unsigned int key)
{
    monitor_t *mid;
    int ret;

    CACHE_LOCK();
    mid = lookupMonitor(key);
    if (mid == MID_NULL || (ret = sysMonitorExit(&sysmon(mid))) == SYS_ERR) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "IllegalMonitorStateException",
		    "current thread not owner");
	goto unlock;
    }

    /*
     * Used to not want to monitorDestroy() in sysMonitorExit(), as we 
     * couldn't tell which monitors were in the cache.  We can now, so
     * we don't really need to pass back SYS_DESTROY.
     */
    if (ret == SYS_DESTROY) {
	monitorDestroy(mid, key);
    }

unlock:
    CACHE_UNLOCK();
}

/*-
 * Wake up the first thread waiting on the condition variable
 * associated with this monitor.
 */
JRI_PUBLIC_API(void)
monitorNotify(unsigned int key)
{
    monitor_t *mid = lookupMonitor(key);

    if (mid == MID_NULL || sysMonitorNotify(&sysmon(mid)) != SYS_OK) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "IllegalMonitorStateException",
		    "current thread not owner");
	return;
    }
}

/*-
 * Wake up *all* the threads waiting on the condition variable
 * associated with this monitor.
 */
void
monitorNotifyAll(unsigned int key)
{
    monitor_t *mid = lookupMonitor(key);

    if (mid == MID_NULL || sysMonitorNotifyAll(&sysmon(mid)) != SYS_OK) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "IllegalMonitorStateException",
		    "current thread not owner");
	return;
    }
}

void
monitorWait(unsigned int key, int millis)
{
    monitor_t *mid = lookupMonitor(key);
    int ret;

    if (mid == MID_NULL) {
	ret = SYS_ERR;
    } else {
	ret = sysMonitorWait(&sysmon(mid), millis);
	if (ret == SYS_TIMEOUT) {
	    ret = SYS_OK;
	}
    }

    if (ret != SYS_OK) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "IllegalMonitorStateException",
		    "current thread not owner");
	return;
    }
}

/*-
 * Registration of monitors not allocated from the monitor cache.
 *
 * All monitors except those in the monitor cache should be registered
 * via monitorRegister().
 *
 * One tricky point is that access to the monitor registry is
 * protected by a registered monitor.  That may require special
 * treatment when doing some operations, e.g. when a thread is dying
 * it leaves all held monitors, but uses the registry monitor to do
 * so.  The responsibility is currently put on the helper function
 * passed to registeredEnumerate() to take care of this special case.
 * registeredEnumerate() is intended to be the only access to the
 * monitor registry.
 *
 * Note also that monitorRegister() is used on the registry lock, and
 * also uses the lock.  This works because we don't actually need the
 * monitor to be registered to use it -- we only need it to be
 * initialized.
 */

sys_mon_t *_registry_lock;
reg_mon_t *MonitorRegistry;  /* Root of registered monitors */

void
monitorRegistryInit()
{
    _registry_lock = (sys_mon_t *) sysMalloc(sysMonitorSizeof());
    memset((char*)_registry_lock, 0, sysMonitorSizeof());
    REGISTRY_LOCK_INIT();
}

void
monitorRegister(sys_mon_t *mid, char *name)
{
    reg_mon_t *reg;

    sysMonitorInit(mid, FALSE);
    reg = (reg_mon_t *) sysMalloc(sizeof(reg_mon_t));
    if (!reg) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "OutOfMemoryError", 0);
	return;
    }
    reg->mid = mid;
    if (!(reg->name = strdup(name))) {
	SignalError((struct execenv *) THREAD(threadSelf())->eetop,
		    JAVAPKG "OutOfMemoryError", 0);
	return;
    }
    REGISTRY_LOCK();
    reg->next = MonitorRegistry;
    MonitorRegistry = reg;
    REGISTRY_UNLOCK();
}

/*-
 * If monitors are dynamically allocated and deallocated (none currently
 * are), it will be necessary that a registered monitor be unregistered
 * prior to deallocation to avoid dangling pointers in the monitor 
 * registry.
 */
void
monitorUnregister(sys_mon_t *mid)
{
    reg_mon_t **prevp = &MonitorRegistry;
    reg_mon_t *reg;

    REGISTRY_LOCK();
    reg = *prevp;
    while (reg && reg->mid != mid) {
	prevp = &reg->next;
	reg = reg->next;
    }

    sysAssert(reg);	/* Else bad call or inconsistent state */

    *prevp = reg->next;
    sysFree(reg->name);
    sysFree(reg);
    REGISTRY_UNLOCK();
}

/*-
 * Helper function for registeredEnumerate() to clean up monitors owned
 * by a dying thread.  It is called with sysThreadSelf() in possession
 * of the registry lock.  If self is the thread that's dying we don't
 * want it to clean up that lock -- registeredEnumerate() will do that.
 * If other than self is being killed, sysMonitorDestroy() ignores it.
 * Hence, we can just skip the registry lock here.
 */
static void
registeredCleanupHelper(reg_mon_t *reg, void *T)
{
    sys_thread_t *tid = (sys_thread_t *) T;

    sysAssert(REGISTRY_LOCKED());
    if (reg->mid != _registry_lock) {
        (void) sysMonitorDestroy(reg->mid, tid);  
    }
}

/*-
 * Call a function for each registered monitor.  Cf. monitorEnumerate().
 *
 * It is intended that this is the only access to the monitor registry.
 * The monitors it contains, of course, are as accessible as desired.
 */
void
registeredEnumerate(void (*fcn)(reg_mon_t *, void *), void *cookie)
{
    reg_mon_t *reg;

    REGISTRY_LOCK();
    reg = MonitorRegistry;
    while (reg) {
	fcn(reg, cookie);
	reg = reg->next;
    }
    REGISTRY_UNLOCK();
}

static void
monitorCleanupHelper(monitor_t *mid, void *T)
{
    sys_thread_t *tid = (sys_thread_t *) T;

    sysAssert(CACHE_LOCKED());
    if (sysMonitorDestroy(&sysmon(mid), tid) == SYS_DESTROY) {
	/* See comment on monitorExit() */
	monitorDestroy(mid, mid->key);
    }
}

/*-
 * monitorCleanup() is called when a thread is killed or exited.
 *
 * The dying thread might be executing in some number of monitors.  We need
 * to exit from each.  At that point we may optionally destroy the monitor.
 */
void
monitorCleanup(TID tid)
{
    /*
     * If this thread currently owns any monitors in the monitor
     * cache, then it exits each.
     */
    monitorEnumerate(monitorCleanupHelper, (void *) SYSTHREAD(tid));

    /*
     * Similarly, if this thread owns any registered static monitors,
     * it exits them.
     */
    registeredEnumerate(registeredCleanupHelper, (void *) SYSTHREAD(tid));
}
