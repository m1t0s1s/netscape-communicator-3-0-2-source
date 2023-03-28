/*
 * @(#)finalize.c	1.23 95/11/29  
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
 * Support for finalization: the finalizer thread runs the methods
 * associated with objects that have been freed.
 */

#include "finalize.h"
#include "javaThreads.h"
#include "monitor.h"
#include "oobj.h"
#include "log.h"
#include "typecodes.h"		/* For T_NORMAL_OBJECT */
#include "sys_api.h"

sys_mon_t *_hasfinalq_lock;	/* Lock for the HasFinalizerQ queue */
sys_mon_t *_finalmeq_lock;	/* Lock for the FinalizeMeQ queue */

finalizer_t *HasFinalizerQ;	/* Live objects w/finalization method */
finalizer_t *FinalizeMeQ;	/* Reclaimed objects needing finalization */
finalizer_t *BeingFinalized;	/* The object currently being finalized; */
				/* this has to be marked in GC so it's */
				/* not thrown away if the finalizer */
				/* thread is interrupted! */

static void
execute_finalizer(finalizer_t *final)
{
    JHandle *handle = final->handle;
    struct methodblock *mb;
    extern int heap_memory_changes;

    if (handle->obj != 0) {
	sysAssert(BeingFinalized == final);
	sysAssert(obj_flags(handle) == T_NORMAL_OBJECT);
	mb = obj_classblock(handle)->finalizer;
	sysAssert(mb);

	do_execute_java_method(0 /*ee*/, handle, 0, 0, mb, FALSE);
	BeingFinalized = 0;	/* Feel free to GC that object now! */
	sysFree(final);
	heap_memory_changes++;
    } /* else, another thread got to this one before we could */
}

/*
 * Explicitly, synchronously flush the FinalizeMeQ.  Normally this
 * is not necessary, as the finalizer thread will clean up.  However,
 * if the system is heavily loaded, there can be no guarantee that
 * the finalization thread will be run at all.  In this case, the
 * user can call Runtime.runFinalization() at reasonable intervals, or
 * when the allocation of a finalized resource fails.  Note that there
 * isn't a built-in equivalent to the garbage collector being run when
 * the heap fills up.  Finalized resources are arbitrary, so we can't
 * predict up front when to flush the queue.  However, for a given
 * resource users (or the system) can build equivalent functionality
 * into their allocators.
 */
void
runFinalization(void)
{
    finalizer_t *final;

    FINALMEQ_LOCK();
    while (FinalizeMeQ) {
	/*
	 * If someone else is finalizing, bail out.  We can do better
	 * than this, but for now it's at least safe and preserves order.
	 */
        if (BeingFinalized) {
	    goto unlock;
	}
	final = BeingFinalized = FinalizeMeQ;
	FinalizeMeQ = final->next;
	FINALMEQ_UNLOCK();
	execute_finalizer(final);	/* Zeros BeingFinalized */
	FINALMEQ_LOCK();
    }
unlock:
    FINALMEQ_UNLOCK();
}

/*
 * Note that the finalizer thread runs at low priority, which means it
 * may *never* run.  Also note that it does not lock the scheduler: that
 * would be troublesome given that the finalization method is random
 * user Oak code.  Rather, we just allow ourselves to be preempted as
 * the system chooses.  This shouldn't get us into trouble, as the
 * objects we are working with are known not to be referenced by any-
 * thing except the FinalizeMeQ.
 */
static void
finalizer_loop(TID tid)
{
    ExecEnv ee;
    finalizer_t *final;

    InitializeExecEnv(&ee, (JHandle *) tid);
    if (ee.initial_stack == 0) {
	out_of_memory();
	return;
    }

    threadInit(tid, (stackp_t) &tid);

    FINALMEQ_LOCK();
    while (1) {
	FINALMEQ_WAIT();

	Log(2, "Finalization thread is running\n");

	/*
	 * Drop and reacquire the FINALQ lock after every object so that
	 * a more important thread won't have to wait for it until we've
 	 * finalized all the outstanding objects before it gets to run.
	 */
	while (FinalizeMeQ) {
	    /* Someone may be synchronously finalizing */
	    if (BeingFinalized) {
		/* Stay locked! */
		break;
	    }
	    final = BeingFinalized = FinalizeMeQ;
	    FinalizeMeQ = final->next;
	    FINALMEQ_UNLOCK();	/* Open door for someone more important */
	    execute_finalizer(final);	/* Zeros BeingFinalized */
	    FINALMEQ_LOCK();
	}
    }
    /* FINALMEQ_UNLOCK(); */
}

/*
 * Create the finalization thread.
 *
 * Interaction between finalization and GC: There can only be finalization
 * methods to run after GC has occurred.  Once there are finalization
 * methods to call, there is no expectation that a given run of the final-
 * izer thread will clear them all.  The finalizer should also be prepared
 * to be interrupted between finalizing any two objects, like async GC.
 *
 * An object cannot be considered free for compaction until its finalization
 * method has been run.  This means that either we finalize an object prior
 * to compaction in the GC run that freed the object, or the object must
 * only be reclaimed in the next GC run.  This happens automatically if we
 * trace the FinalizeMeQ, which after GC should retain the sole reference
 * to the object.
 */
void
InitializeFinalizer()
{
	if (_finalmeq_lock == 0) {
	    _finalmeq_lock = (sys_mon_t *) sysMalloc(sysMonitorSizeof());
    	memset(_finalmeq_lock, 0, sysMonitorSizeof());
	    FINALMEQ_LOCK_INIT();
	}
}

void
InitializeFinalizerThread()
{
    TID tid;
    char *name = "Finalizer thread";
    extern ClassClass *Thread_classblock;

	InitializeFinalizer();
	
    tid = (TID)execute_java_constructor(NULL, NULL, Thread_classblock, "()");

    /* 
     * The finalization thread can run arbitrary code, so is given
     * a normal-sized stack.  This may be overkill, but is safe.
     */
    if (threadCreate(tid, THR_SYSTEM, ProcStackSize,
		     (void * (*)(void *)) finalizer_loop) != 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
    } else {
	/* Set fields others will expect... */
	THREAD(tid)->name = MakeString(name, strlen(name));
	THREAD(tid)->priority = MinimumPriority;
	THREAD(tid)->daemon = 1;
	threadSetPriority(tid, MinimumPriority);
	threadResume(tid);
    }
}

#ifdef DEBUG

/*
 * Debugging support: It is safe to call these things inside GC because
 * although the scheduler is locked, we always precede GC by grabbing
 * the finalization queue locks.
 */

void
DumpHasFinalizerQ()
{
    finalizer_t *finalizer;
    char *name;

    HASFINALQ_LOCK();
    fprintf(stderr, "\nLiving objects with finalization methods:\n");
    finalizer = HasFinalizerQ;
    while (finalizer) {
	name = Object2CString(finalizer->handle);
	fprintf(stderr, "    Has finalizer: %s (handle 0x%lx)\n",
	        name, finalizer->handle);
	finalizer = finalizer->next;
    }
    HASFINALQ_UNLOCK();
}

void
DumpFinalizeMeQ()
{
    finalizer_t *finalizer;
    char *name;

    FINALMEQ_LOCK();
    fprintf(stderr, "\nDead objects pending finalization:\n");
    finalizer = FinalizeMeQ;
    while (finalizer) {
	name = Object2CString(finalizer->handle);
	fprintf(stderr, "    To be finalized: %s (handle 0x%lx)\n",
		name, finalizer->handle);
	finalizer = finalizer->next;
    }
    FINALMEQ_UNLOCK();
}

#endif
