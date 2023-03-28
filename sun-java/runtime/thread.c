/*
 * @(#)thread.c	1.46 96/02/13
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

/*-
 *      Stuff for dealing with threads.
 *      originally in threadruntime.c, Sun Sep 22 12:09:39 1991
 */

#include <stdio.h>
#include <signal.h>

#include "tree.h"
#include "javaThreads.h"

#include "monitor.h"
#include "exceptions.h"
#include "sys_api.h"

#ifdef XP_MAC
#include "jdk_java_lang_Object.h"
#else
#include "java_lang_Object.h"
#endif

#include "java_lang_Throwable.h"
#include "java_lang_Thread.h"

static void    /* first routine called in the new thread */
ThreadRT0(Hjava_lang_Thread *p)
{
    ExecEnv *ee;
    int count = 3; 

#ifndef trace
    if (trace) {
	fprintf(stderr, "Started thread %X\n", p);
    }
#endif
    /* REMIND: the EE must be freed in interpreter.c in DeleteExecEnv(...) */
    ee = (ExecEnv *)sysMalloc(sizeof(ExecEnv));
    InitializeExecEnv(ee, (JHandle *) p);
    
    if (ee->initial_stack == NULL)
    	SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
    else {
    threadInit(p, (stackp_t) &p);
    if (unhand(p)->stillborn) {
	/* This thread was asked to exit before it got to run */
	threadExit();
    }
    execute_java_dynamic_method(ee, (void *)p, "run", "()V");
    }

    if (exceptionOccurred(ee)) {
        if (THREAD(p)->group != NULL) {
            void *t = (void *)ee->exception.exc;
            exceptionClear(ee);
            execute_java_dynamic_method(ee, (void *)THREAD(p)->group,
                "uncaughtException", "(Ljava/lang/Thread;Ljava/lang/Throwable;)V", p, t);
        }
    }

    THREAD(p)->stillborn = 1;

      /* We're done, let the thread clean up */
      /* The reason hte count is more than one, is that it is likely
	 that an exception is thrown by another thread (for example
	 when doing a stop) during the exit call. In such a case,
	 thread will not be removed from the group - so we need to 
	 retry calling exit. We can infinitely call exit until the 
	 group field is null, but this would result in hanging of browsers
	 in case of any undetected bug/race on our side */

     while (THREAD(p)->group != NULL && (count-- > 0)) {
	exceptionClear(ee);	/* don't run exit with a pending exception */
	execute_java_dynamic_method(ee, (void *)p, "exit", "()V");
    }
    sysAssert(THREAD(p)->group == NULL); 

    /* Falling off bottom of thread... */
    threadExit();
}

void
java_lang_Thread_start(Hjava_lang_Thread *p) {
    struct Classjava_lang_Thread *tid;

    monitorEnter(obj_monitor(p));

    tid = (struct Classjava_lang_Thread *) unhand(p);

    /* It is illegal to call Start on an active thread */
    if (tid->PrivateInfo) {
	SignalError(0, JAVAPKG "IllegalThreadStateException", 0);
    } else {
	uint_t flag = (tid->daemon != 0) ? THR_SYSTEM : THR_USER;

	if (threadCreate(p, flag, (size_t) ProcStackSize, (void *(*)(void *)) ThreadRT0)) {
	    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	} else {
	    /* Set a default priority: see comments on {s,g}etPriority() */
	    threadSetPriority(p, tid->priority);
	    threadResume(p);
	}
    }

    monitorExit(obj_monitor(p));
}

/*
 * We normally stop a thread by posting a ThreadDeath exception against
 * it.  When it handles the exception, the thread cooperatively exits.
 * However, if a thread hasn't started running yet, it is not prepared
 * to handle an exception.  That case motivated the use of a "stillborn"
 * flag that can be safely set before the thread starts running.  When
 * the thread does run, if it is marked to be stillborn, it immediately
 * exits.  However, the mechanism can also be used to fix an old bug
 * and handle other boundary cases.  If we *always* set tid->stillborn
 * to 1 when stop is called on a thread, this ensures that a stopped
 * thread can't be resurrected -- attempting to restart it will cause
 * it to exit as stillborn.  We only call threadPostException() if
 * tid->PrivateInfo is set just in case it ever assumes system-specific
 * thread structure is there.
 */
void
java_lang_Thread_stop0(Hjava_lang_Thread *p, struct Hjava_lang_Object *exc) {
    struct Classjava_lang_Thread *tid;

    monitorEnter(obj_monitor(p));

    if (exc == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    tid = (struct Classjava_lang_Thread *) unhand(p);
    if (tid->PrivateInfo && !tid->stillborn) {
	tid->stillborn = 1;
	(void) threadPostException(p, exc);
    }

    monitorExit(obj_monitor(p));
}

/*
 * Note that we don't go to the native thread to determine whether the Java
 * thread is alive.  The status of the native thread doesn't matter, to the
 * first approximation at least, if the Java thread object isn't set up
 * properly.  However, we could also double-check the liveness of the native
 * thread.
 */
long
java_lang_Thread_isAlive(Hjava_lang_Thread *p)
{
    struct Classjava_lang_Thread *tid;
    tid = (struct Classjava_lang_Thread *) unhand(p);
    return ((long)tid->PrivateInfo);
}

void
java_lang_Thread_suspend0(Hjava_lang_Thread *p) {
    struct Classjava_lang_Thread *tid;

    tid = (struct Classjava_lang_Thread *) unhand(p);
    if (tid->PrivateInfo) {
	(void) threadSuspend(p);
    }
}

void
java_lang_Thread_resume0(Hjava_lang_Thread *p)
{
    struct Classjava_lang_Thread *tid;

    tid = (struct Classjava_lang_Thread *) unhand(p);
    if (tid->PrivateInfo) {
	(void) threadResume(p);
    }
}

/* We want to rely on priorities as set/got via the threadX functions.
 * However, it is legal to call setPriority() and getPriority() after
 * 'new' and before start() is called, in which case the thread's TID has
 * been allocated but its thread not yet created.  So we also maintain
 * tid->priority to bridge the gap.
 *
 * The priority should already have been check to fall within a
 * valid range by the software above.  All we do is pass it on to
 * the system level.  The JAVA level also set the priority field.
 */
void
java_lang_Thread_setPriority0(register Hjava_lang_Thread *p, long prio) {
    struct Classjava_lang_Thread *tid;

    tid = (struct Classjava_lang_Thread *) unhand(p);
    if (tid->PrivateInfo) {
	threadSetPriority(p, prio);
    } 
}

long
java_lang_Thread_getPriority(Hjava_lang_Thread *p)
{
    long prio;
    struct Classjava_lang_Thread *tid;
    tid = (struct Classjava_lang_Thread *) unhand(p);
    if (tid->PrivateInfo) {
	/* When it's been set, rely on the thread's actual priority */
        threadGetPriority(p, (int32_t*)&prio);
    } else {
	prio = tid->priority;
    }
    return prio;
}

void
java_lang_Thread_yield(Hjava_lang_Thread *p)
{
    threadYield();
}

void
java_lang_Thread_sleep(Hjava_lang_Thread *p, int64_t millis)
{
    /* REMIND: ThreadSleep should really take long */
    int32_t ms = ll2int(millis);

    if (ms < 5) {
	threadYield();
	return;
    }

    threadSleep(ms);
    return;
}

Hjava_lang_Thread *
java_lang_Thread_currentThread(Hjava_lang_Thread *p)
{
    return threadSelf();
}

long
java_lang_Thread_countStackFrames(Hjava_lang_Thread *hp)
{
    if (hp == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    } else {
	Classjava_lang_Thread *p = unhand(hp);
	ExecEnv *this_ee = (ExecEnv *) p->eetop;
	JavaFrame *frame, frame_buf;
	int count;

	if (this_ee == 0) {
	    /* Thread hasn't any state yet. */
	    return 0;
	}

    for (count = 0, frame = EE()->current_frame ; frame != 0 ;) {
		if (frame->current_method != 0) {
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ? 
				CompiledFramePrev(frame, &frame_buf) : frame->prev;
            count++;
		} else {
			frame = frame->prev;
		} 
    }

	return count;
    }
}
