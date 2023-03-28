/*
 * @(#)threads.c	1.43 95/11/16  
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

#include <sys/types.h>

#ifndef XP_MAC
#include <memory.h>
#endif

#include <string.h>

#include "exceptions.h"
#include "javaThreads.h"
#include "debug.h"
#include "log.h"
#include "timeval.h"
#include "monitor.h"
#include "monitor_cache.h"
#include "sys_api.h"

#include "java_lang_Thread.h"
#include "java_lang_Throwable.h"

extern void InitializeFinalizer(void);

#ifdef LOGGING
int logging_level;	/* this has to go somewhere! */
#endif

/*
 * A count of all active threads and of those all user threads.  These
 * counts are maintained by code below the HPI.
 */
int ActiveThreadCount = 0;		/* All threads */
int UserThreadCount   = 0;		/* User threads */

sys_mon_t *_queue_lock;	/* Protects thread queue, thread count */

/*
 * The following are defined as macros in threads.h:
 *
 * void threadInit(TID tid, stackp_t sb)
 * stackp_t threadStackBase(TID tid)
 * void * threadStackPointer(TID tid)
 * void threadYield(void)
 * int threadSetPriority(TID tid, int priority)
 * int threadGetPriority(TID tid, int *priority)
 * int threadResume(TID tid)
 * int threadSuspend(TID tid)
 * void threadExit(void) 
 */

/*
 * Initialize the threads package.  The caller will return, executing
 * in the context of the first thread.
 */
void
threadBootstrap(TID tid, stackp_t sb)
{
    sys_thread_t *t;

    sysAssert(SYSTHREAD(tid));

    /* Initialize the queue lock monitor */
    _queue_lock = (sys_mon_t *)sysMalloc(sysMonitorSizeof());
    memset((char *)_queue_lock, 0, sysMonitorSizeof());
    QUEUE_LOCK_INIT();

    /* Bootstrap the current process into the main, primordial thread */

    sysThreadBootstrap(&t);

    THREAD(tid)->priority = NormalPriority;
    threadSetPriority(tid, NormalPriority);

    /* Create system dependent threads */
    sysThreadInitializeSystemThreads();

    /* Preallocate the internal exception objects */
    exceptionInit();

    threadInit(tid, sb);
}

int
threadCreate(TID tid, unsigned int flags, size_t stack_size, void *(*func)(void *))
{
    int err;
    sys_thread_t *t;

    sysAssert(SYSTHREAD(tid) == 0);

    err = sysThreadCreate(stack_size, flags,
			(void * (*)(void *)) func, &t, (void *) tid);
    
    if ((err == SYS_ERR) || (err == SYS_NORESOURCE)) {
        return 1;
    }

    /* Set up backpointer to java thread object */
    sysThreadSetBackPtr(t, (void *) tid);
    /* t->cookie = (void *) tid; */

    THREAD(tid)->PrivateInfo = (long) t;

    return 0;
}

TID
threadSelf(void)				/* Common: should be a macro? */
{
    sys_thread_t *t = sysThreadSelf();
    if (t) {
	return (TID) sysThreadGetBackPtr(t);
	/* return (TID) t->cookie; */
    } else {
	return 0;
    }
}

/*
 * This structure is used during the enumeration of threads.  It
 * holds the state of the enumeration while walking through the
 * threads list.
 */
typedef struct {
    TID *array;
    int index;
    int maxindex;
} tid_array_t;

static int
threadEnumeratorHelper(sys_thread_t *t, void *arg)
{
    tid_array_t *tarr = (tid_array_t *) arg;
    TID tid;

    tid = (TID) sysThreadGetBackPtr(t);
    /* tid = (TID) t->cookie; */
    if (tarr->array != NULL) {
	if (tarr->index >= tarr->maxindex) {
	    return SYS_ERR;
	}
	tarr->array[tarr->index] = tid;
    }
    tarr->index++;
    return SYS_OK;
}

/*
 * Enumerate all threads in the system
 */
int
threadEnumerate(TID *tarr, int count)
{
    tid_array_t helper_data;
    int i;

    helper_data.array = tarr;
    helper_data.maxindex = count;
    helper_data.index = 0;

    /* Currently ignoring error return */
    (void) sysThreadEnumerateOver(threadEnumeratorHelper, (void *) &helper_data);
    for (i = helper_data.index; i < count; i++) {
	tarr[i] = (TID) 0;
    }

    return helper_data.index;
}

/*
 * Count all threads in the system.
 */
int
threadCount()
{
    tid_array_t helper_data;

    helper_data.array = 0;
    helper_data.maxindex = 0;
    helper_data.index = 0;

    sysThreadEnumerateOver(threadEnumeratorHelper, (void *) &helper_data);
    return helper_data.index;
}

void
threadSleep(int32_t millis)
{
    /* Need a unique key per thread */
    int32_t sleep_key = (int32_t) SYSTHREAD(threadSelf());

    if (millis == 0) {
	threadYield();
    } else {
	Log2(4, "threadSleep: TID %s millis %d\n",
	     threadName(threadSelf()), millis);
	monitorEnter((uint_t) sleep_key);
	monitorWait((uint_t) sleep_key, millis);
	monitorExit((uint_t) sleep_key);
    }
}

int
threadPostException(TID tid, void *vexc)
{ 
    JHandle *exc = (JHandle*) vexc;
    struct execenv *ee;  


    ee = (ExecEnv *) THREAD(tid)->eetop;
    if (ee != (ExecEnv *) 0) {   
        /* For now we throw this random exception */ 
	if (is_instance_of(exc, classJavaLangThrowable, ee)) {
	    fillInStackTrace((Hjava_lang_Throwable *)exc, ee);
	}
        sysThreadPostException(SYSTHREAD(tid), exc);
	return TRUE;
    }
    return FALSE;
} 
