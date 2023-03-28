#ifdef DEBUG
/*
 * @(#)debug.c	1.19 95/10/16
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
 * Debugging help for users.
 */

#include <stdio.h>

#include "javaThreads.h"
#include "debug.h"
#include "monitor.h"
#include "monitor_cache.h"
#include "exceptions.h"
#include "bool.h"
#include "interpreter.h"

#include "java_lang_Thread.h"


/*
 * Access to thread information.  Quietly truncate name on jio_snprintf()
 * buffer overflow.
 */
char *
threadName(void* tid)
{
    /* WARNING: There is no actual limit on the length of a thread name */
#define THREADNAMEMAX 80
    /* WARNING: Returning a name as a static buffer is not MT-safe! */
    static char	namebuf[THREADNAMEMAX];

    if (!tid) {
	(void) jio_snprintf(namebuf, sizeof(namebuf), "name unknown (nil)");
    } else {
        (void) jio_snprintf(namebuf, sizeof(namebuf), "%s (0x%8x)",
	        Object2CString((JHandle *) THREAD((TID) tid)->name), tid);
    } 

    return (namebuf);
}

void
threadDumpInfo(TID tid, bool_t verbose)
{
    int32_t priority;
    fprintf(stderr, "    %s", Object2CString((JHandle *) THREAD(tid)->name));
    if (verbose == TRUE) {
	fprintf(stderr, " (TID:0x%x", tid);
	if (THREAD(tid)->PrivateInfo) {
	    fprintf(stderr, ", sys_thread_t:0x%x", SYSTHREAD(tid));	
            sysThreadDumpInfo(SYSTHREAD(tid), stderr);
            fprintf(stderr, ")");
	    threadGetPriority(tid, &priority);
	    fprintf(stderr, " prio=%d", priority);
	    if (unhand(tid)->eetop &&
		exceptionOccurred((struct execenv *)unhand(tid)->eetop)) {
	        char buf[256];
	        exception_t exc =
		    ((struct execenv *)unhand(tid)->eetop)->exception.exc;
	        fprintf(stderr, ": pending=%s",
			classname2string(classname(obj_classblock(exc)),
			buf, sizeof(buf)));
	    }
	    fprintf(stderr, "%s\n",
		tid == threadSelf() ? " *current thread*" : "");
    	} else {
	    fprintf(stderr, ") : <thread not active>\n");
	}
    } else {
	fprintf(stderr, "\n");
    }

    return;
}

static int
DumpThreadsHelper(sys_thread_t *t, void *arg)
{
    HThread *p = (HThread *) sysThreadGetBackPtr(t);
    struct Classjava_lang_Thread *tid = unhand(p);
    long n = 20;
    struct execenv *ee = (struct execenv *) tid->eetop;
    threadDumpInfo(p, TRUE);
    if (ee) {
	JavaFrame frame_buf, *frame = ee->current_frame;
	char pcb[200];
	for ( ; n > 0 && frame;) {
	    if (frame->current_method) {
			frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
				CompiledFramePrev(frame, &frame_buf) : frame->prev;
			pc2string(frame->lastpc, frame->current_method, 
					  pcb, pcb + sizeof pcb);
			fprintf(stderr, "\t%s\n", pcb); 
	    } else {
	    frame = frame->prev;
        }
		n--;
	}
	}
    return SYS_OK;
}

void
DumpThreads()
{
    fprintf(stderr, "\nFull thread dump:\n");

    sysThreadEnumerateOver(DumpThreadsHelper, (void *) 0);
}

void
DumpAllStacks()
{
    DumpThreads();		/* For backward compatibility */
}


/*
 * Access to monitor information
 */
static void
registeredDumpHelper(reg_mon_t *reg, void *ignored)
{
    fprintf(stderr, "    %s: ", reg->name);
    sysMonitorDumpInfo(reg->mid);
}

void
DumpMonitors()
{
    /* Defined in gc.c as it needs to know about the heap. */
    extern void monitorDumpHelper(monitor_t *, void *);

    /*
     * Dump the contents of the monitor cache
     */
    fprintf(stderr, "Monitor Cache Dump:\n");
    monitorEnumerate(monitorDumpHelper, 0);

    /*
     * Also dump other well-known monitors: what these will be is
     * system-dependent and does not belong in share code.
     */
    fprintf(stderr, "Registered Monitor Dump:\n");
    registeredEnumerate(registeredDumpHelper, 0);
}

#endif
