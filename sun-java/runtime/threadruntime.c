/*
 * @(#)threadruntime.c	1.52 95/10/23  
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
 */

#include <stdio.h>
#include <signal.h>

#include "tree.h"
#include "monitor.h"
#include "signature.h"
#include "javaThreads.h"
#include "exceptions.h"
#include "log.h"
#include "javaString.h"

/*
 * Pointer to the Thread class block.
 */
ClassClass *Thread_classblock;

uint32_t JavaStackSize = JAVASTACKSIZE;

long *
getclassvariable(struct ClassClass *cb, char *fname)
{
    int    n = cb->fields_count;
    struct fieldblock *fb;

    for (fb = cbFields(cb); --n >= 0; fb++)
    if ((fb->access & ACC_STATIC) && strcmp(fname, fieldname(fb)) == 0) {
	char *signature = fieldsig(fb);
	if (signature[0] == SIGNATURE_LONG ||
	    signature[0] == SIGNATURE_DOUBLE)
	    return (long *)twoword_static_address(fb);
	else 
	    return (long *)normal_static_address(fb);
    }
    return (long *)0;
}

HThread *
InitializeClassThread(ExecEnv *ee, char **errmsg)
{
    HThread *main;
    HThreadGroup *systemgroup;
    ClassClass *cb;
    struct Classjava_lang_Thread *tid;
    extern void ResolveInit(void);
    sys_thread_t *t;

    ResolveInit();
    cb = FindClassFromClass(ee, JAVAPKG "Thread", TRUE, NULL);
    if (!cb) {
		*errmsg = "cannot find class " JAVAPKG "Thread";
		return 0;
    }

    Thread_classblock = cb;

    /*
     * We want to bootstrap the currently executing context into the
     * threads package.  Instantiate Thread, and hand to the thread's
     * package.
     */
    main = (HThread *) ObjAlloc(cb, 0);
    if (main == 0) {
	return 0;
    }

    tid = (struct Classjava_lang_Thread *) unhand(main);

    /* Need to set unhand(main)->eetop before threadBootstrap() */
    unhand(main)->eetop = (long)ee;
    ee->thread = (JHandle *)main;

    t = sysThreadSelf();
    THREAD(main)->PrivateInfo = (long) t;

    /* Set up backpointer to java thread object 
    **
    ** It is important to set up this backpointer before ANY java code is
    ** executed for the thread...  Until the backpointer is set the GC will
    ** NOT be able to scavange the java stack for the thread...
    */
    sysThreadSetBackPtr(t, (void *) main);

    /* Find the ThreadGroup class */
    cb = FindClassFromClass(ee, JAVAPKG "ThreadGroup", TRUE, NULL);
    if (!cb) {
	*errmsg = "cannot find class " JAVAPKG "ThreadGroup";
	return 0;
    }

    /* Allocate the system thread group */
    systemgroup = 
	(HThreadGroup *)execute_java_constructor(PRIVILEGED_EE, 0, cb, "()");
    if (systemgroup == 0) {
	return 0;
    }

    /* For now pretent that the main thread is in the system group. */
    unhand(main)->group = systemgroup;

    /* Now bootstrap the other threads */
    threadBootstrap(main, mainstktop);	/* mainstktop set in javai.c */

    *errmsg = 0;
    return main;
}

HThreadGroup *maingroup;

void
InitializeMainThread(void)
{
    HThread *main = threadSelf();

    /* Allocate the main thread group */
    maingroup = 
	(HThreadGroup *)execute_java_constructor(0, JAVAPKG "ThreadGroup", 0, 
						 "(Ljava/lang/String;)", 
						 makeJavaString("main", 4));
    if (maingroup == 0) {
	return;
    }
    
    /* Now call the constructor of the main thread. 
     * This will add the current thread to the main group
     */

    execute_java_dynamic_method(0, (void *)main, "init", 
				"(Ljava/lang/ThreadGroup;Ljava/lang/Runnable;Ljava/lang/String;)V", 
				maingroup, /* ThreadGroup */
				NULL,      /* runnable */
				makeJavaString("main", 4));
}

void
TraceMethodCalls(int on)
{
#ifdef TRACING
    tracem = on;
#endif
}


void
TraceInstructions(int on)
{
#ifdef TRACING
    trace = on;
#endif
}


/**
 ** These are stubs that are required by other parts of Java
 **/

void
setThreadName(HThread *ht, HArrayOfChar *newName)
{
    struct Classjava_lang_Thread *t;
    t = (struct Classjava_lang_Thread *) unhand(ht);
    t->name = newName;
}

HArrayOfChar *
getThreadName()
{
    struct Classjava_lang_Thread *t;
    t = (struct Classjava_lang_Thread *) unhand(threadSelf());
    return (HArrayOfChar *)t->name;
}


HThread *
getThreadNext(HThread *p)
{
    struct Classjava_lang_Thread *tid;

    tid = (struct Classjava_lang_Thread *) unhand(p);
    return (tid->threadQ);
}

long
setThreadEETop(HThread *p, long val)
{
    long r;
    struct Classjava_lang_Thread *tid;

    tid = (struct Classjava_lang_Thread *) unhand(p);
    r = tid->eetop;
    tid->eetop = val;
    return r;
}

