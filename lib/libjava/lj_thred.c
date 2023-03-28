/**********************************************************************
 *
 *  code that allows two threads to merge for a little
 *  while, used by mocha/java glue.
 *
 **********************************************************************/

#include "lj.h"
#include "java.h"
#include "jri.h"

#include "prthread.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"


/* state of each thread */
typedef enum {
    LJ_FREE,		/* not participating yet */
    LJ_RUN,		/* running */
    LJ_WAIT,		/* not done yet, wait for notify */
    LJ_DONE,		/* done, threads go their separate ways */
    LJ_CALL		/* thread has to do something */
} LJState;

/* indices into the state array */
enum {
    LJ_MOZILLA = 0,
    LJ_JAVA = 1
};

/* LJThread represents the relationship between java and mocha code
 * that are running in separate threads but making synchronous calls
 * to each other.
 *
 * (it isn't necessary if the java code is running in the mozilla thread?)
 */
typedef struct {
    /* only one java thread may be synching with mozilla at once */
    PRMonitor		ownerLock;
    JRIEnv		*owner;

    /* we have two monitors here because we need two conditions.
     * but we always grab both monitors together (mozilla then java).
     * it's cheaper to grab extra locks than it is to respond to
     * unnecessary notifications */
    PRMonitor		monitors[2];  /* LJ_MOZILLA or LJ_JAVA */

    /* the state of the two-thread system is encoded into two
     * state variables.  however, there are only six interesting
     * states (of the form Mozilla|Java):
     *    wait|run    - java is running
     *    wait|call   - java should execute the "doit" action
     *    wait|done   - java can return (mozilla is finished)
     *     run|wait   - mozilla is running
     *    call|wait   - mozilla should execute the "doit" action
     *    done|wait   - mozilla can return (java is finished)
     */
    LJState		states[2];  /* LJ_MOZILLA or LJ_JAVA */

    /* if state == LJ_CALL, call this callback */
    LJCallback		doit;
    void		*data;

    /* XXX should probably record the java thread id too */
} LJThread;

/* global instance, there can be only one */
static LJThread _lj_javasync;
LJThread *lj_javasync = 0;

static void lj_Run(LJThread *ljt, int us);
static void lj_CallOther(JRIEnv *env, int us, LJCallback doit, void *data);

/* initialize this part of the system */
void
lj_InitThread()
{
    /* _lj_javasync is static to avoid a PR_NEWZAP */
    lj_javasync = &_lj_javasync;

    /* XXX Kipp sez PR_InitMonitor's going away. */
    PR_InitMonitor(&lj_javasync->ownerLock, 0, "mozillas-pal-java");
    PR_InitMonitor(&lj_javasync->monitors[LJ_MOZILLA],
                   0, "mozilla-waits-on-java");
    PR_InitMonitor(&lj_javasync->monitors[LJ_JAVA],
                   0, "java-waits-on-mozilla");

    lj_javasync->owner = 0;
}

/* call mozilla from a java thread */
/* CALLED FROM THE JAVA THREAD ONLY */
void
LJ_CallMozilla(JRIEnv *env, LJCallback doit, void *data)
{
    /* XXX should have an assertion checking that the env
     * matches PR_CurrentThread() */

    /* XXX assert that the rusty lock is open */

    /* if the java env is the same as the mozilla env, things
     * are easy */
    if (env == mozenv) {
#ifdef XP_UNIX
        /* entering unsafe code */
        PR_XLock();
#endif
        doit(data);
#ifdef XP_UNIX
        /* leaving unsafe code */
        PR_XUnlock();
#endif
        return;
    }

    lj_CallOther(env, LJ_JAVA, doit, data);
}

/* call java from mozilla, using a particular java thread */
/* CALLED FROM MOZILLA THREAD ONLY */
void
LJ_CallJava(JRIEnv *env, LJCallback doit, void *data)
{
    /* XXX assert that the current thread is the mozilla thread */

#ifdef XP_UNIX
    /* entering safe code */
    PR_XUnlock();
#endif

    /* if the java env is the same as the mozilla env, things
     * are easy */
    if (env == mozenv)
        doit(data);
    else
        lj_CallOther(env, LJ_MOZILLA, doit, data);

#ifdef XP_UNIX
    /* leaving safe code */
    PR_XLock();
#endif
}

/*
 * this event is sent to mozilla to capture its thread
 */

typedef struct MozillaEvent_SlaveToJava {
    MWContextEvent	ce;
    LJThread		*ljt;
} MozillaEvent_SlaveToJava;

/* this is what mozilla calls when it gets the initial event
 * telling it to execute some mocha code.  once we arrive
 * here, we have control of both the mozilla and the java threads
 * and the fun can really begin */
/* CALLED FROM MOZILLA THREAD ONLY */
static void
lj_HandleEvent_SlaveToJava(MozillaEvent_SlaveToJava* e)
{
    LJThread *ljt = e->ljt;

#ifdef XP_UNIX
    /* entering safe code */
    PR_XUnlock();
#endif

    /* initial state is call|wait */

    /* go into the main loop for mozilla, starting by doing
     * whatever the java thread wanted us to do */
    lj_Run(ljt, LJ_MOZILLA);

    /* state now should be done|done */

    /* once we come back to here, synchronization is over and
     * the two threads can go their merry ways */

#ifdef XP_UNIX
    /* leaving safe code */
    PR_XLock();
#endif

    /* return to the event loop */
}

static void
lj_DestroyEvent_SlaveToJava(MozillaEvent_SlaveToJava* event)
{
    XP_FREE(event);
}

static void
LJ_PostSlaveToJava(LJThread *ljt)
{
    MozillaEvent_SlaveToJava* event = PR_NEW(MozillaEvent_SlaveToJava);
    if (!event) {
        /* XXX java thread calling us will wait forever for mozilla
         * to receive the event.  what else can we do? */
        /* make another attempt for the Mac */
        event = PR_NEW(MozillaEvent_SlaveToJava);
        if (!event)
            return;
    }
    PR_InitEvent(&event->ce.event,
                 (PRHandleEventProc)lj_HandleEvent_SlaveToJava,
                 (PRDestroyEventProc)lj_DestroyEvent_SlaveToJava);

    /* XXX we don't really have a context in mind, we just want
     * the thread. */
    event->ce.context = 0;
    event->ljt = ljt;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
}

/* this routine is sort of the main loop for both of the
 * cooperating threads.  "us" is the LJThreadMonitor for
 * the currently running thread, "them" is the other one.
 */
/* CALLED FROM EITHER THREAD */
static void
lj_Run(LJThread *ljt, int us)
{
    int them = 1 - us;
    PRBool done;
    LJCallback doit;
    void *data;

    /* event loop */
    do {
        done = PR_FALSE;

        /* this Enter is matched by an Exit in each branch of the switch */
        PR_EnterMonitor(&ljt->monitors[us]);

        switch (ljt->states[us]) {

        case LJ_CALL:   /* call|wait -> run|wait -> wait|done */
            ljt->states[us] = LJ_RUN;

            /* make copies of data we need from inside the
             * monitor and release it */
            doit = ljt->doit;
            data = ljt->data;
            PR_ExitMonitor(&ljt->monitors[us]);
            
            /* do what was asked, outside the monitor */
            /* if this results in a call to the other side, it must
             * set them->state=LJ_DOIT and us->state=LJ_WAIT, then
             * recursively enter lj_Run */
#ifdef XP_UNIX
            if (us == LJ_MOZILLA)
                /* entering unsafe code */
                PR_XLock();
#endif
            doit(data);
#ifdef XP_UNIX
            if (us == LJ_MOZILLA)
                /* leaving unsafe code */
                PR_XUnlock();
#endif

            /* notify the other thread that we're done, and
             * wait for their next order */

            PR_EnterMonitor(&ljt->monitors[us]);
            ljt->states[us] = LJ_WAIT;
            PR_ExitMonitor(&ljt->monitors[us]);

            PR_EnterMonitor(&ljt->monitors[them]);
            ljt->states[them] = LJ_DONE;
            PR_Notify(&ljt->monitors[them]);
            PR_ExitMonitor(&ljt->monitors[them]);

            break;

        case LJ_WAIT:	/* wait|? */
            /* the other thread is active on our behalf */
            PR_Wait(&ljt->monitors[us], LL_MAXINT);

            PR_ExitMonitor(&ljt->monitors[us]);

            break;

        case LJ_DONE:	/* done|wait -> run|wait */
            done = PR_TRUE;
            ljt->states[us] = LJ_RUN;

            PR_ExitMonitor(&ljt->monitors[us]);

            break;

        default:	/* uh oh */
	    done = PR_TRUE;
            fprintf(stderr, "unexpected state %d|%d in LJThread, us=%d\n",
                    &ljt->states[LJ_MOZILLA], &ljt->states[LJ_JAVA], us);
            PR_ExitMonitor(&ljt->monitors[us]);
            break;
        }
    } while (!done);
}

/* this is what a thread calls when it wants to run some code
 * in the other thread, after the initial rendezvous.
 * it delegates the call to the other thread and tells it to run,
 * then waits for it to come back.
 */
/* THIS MUST BE CALLED BY JAVA FIRST, SINCE MOZILLA CAN'T LURE
 * A JAVA THREAD INTO A RENDEZVOUS */
/* CALLED FROM EITHER THREAD */
static void
lj_CallOther(JRIEnv *env, int us, LJCallback doit, void *data)
{
    int them = 1 - us;
    PRBool owned = PR_TRUE;
    LJThread *ljt = lj_javasync;
    PRBool java_called_mozilla_first = PR_TRUE;

    /* otherwise, life is more complicated.  start by making
     * sure that the given env has exclusive access to mozilla */
    PR_EnterMonitor(&ljt->ownerLock);
    while (ljt->owner != env) {
        /* if this is the mozilla thread and we haven't rendezvoused
         * already there is no way to proceed */
        if (us == LJ_MOZILLA) {
            java_called_mozilla_first = PR_FALSE;
            break;
        }
        if (ljt->owner) {        /* someone else has it */
            PR_Wait(&ljt->ownerLock, LL_MAXINT);
        } else {
            ljt->owner = env;
            owned = PR_FALSE;
        }
    }
    PR_ExitMonitor(&ljt->ownerLock);

    /* if you die here, you tried to initiate the java/mozilla
     * connection from the mozilla side, which can't be done */
    /* XXX we should really fail utterly regardless of DEBUG */
    PR_ASSERT(java_called_mozilla_first);

    /* if the connection was already made, our state is run|wait,
     * otherwise it's unknown.  either way, the state we want
     * to reach is wait|call */

    PR_EnterMonitor(&ljt->monitors[us]);
    /* this thread starts by waiting */
    ljt->states[us] = LJ_WAIT;
    PR_ExitMonitor(&ljt->monitors[us]);

    PR_EnterMonitor(&ljt->monitors[them]);

    /* tell the other thread what to do */
    ljt->states[them] = LJ_CALL;
    ljt->doit = doit;
    ljt->data = data;

    /* if we just claimed the lj_javasync structure, we have to
     * rendezvous first.  a java thread can rendezvous with mozilla by
     * sending a callback event, but the other way is impossible */
    if (!owned) {
        /* put out the bait which will lure mozilla into our trap */
        LJ_PostSlaveToJava(ljt);
    } else {
        PR_Notify(&ljt->monitors[them]);
    }

    PR_ExitMonitor(&ljt->monitors[them]);

    /* wait for the other thread to finish or ask us for
     * something by entering lj_Run (possibly recursively) */
    lj_Run(ljt, us);

    /* if this was the first call (from java to mozilla) we
     * release mozilla and the ljthread afterward */
    if (us == LJ_JAVA && !owned) {
        /* free the monster! */
        PR_EnterMonitor(&ljt->monitors[LJ_MOZILLA]);
        ljt->states[LJ_MOZILLA] = LJ_DONE;
        PR_Notify(&ljt->monitors[LJ_MOZILLA]);
        PR_ExitMonitor(&ljt->monitors[LJ_MOZILLA]);

        PR_EnterMonitor(&ljt->ownerLock);
        ljt->owner = 0;
        PR_Notify(&ljt->ownerLock);
        PR_ExitMonitor(&ljt->ownerLock);
    }
}

/* return PR_TRUE if the mozilla thread is running and able to handle
 * requests.  if it returns PR_FALSE, then any synchronous network
 * activity (i.e. java stuf) will deadlock the browser.
 * see also sun-java/netscape/net/inStr.c */
PRBool
LJ_MozillaRunning(JRIEnv *env)
{
    /* mac doesn't have this problem */
    extern PRThread* mozilla_thread;
    PRThread *curthread = PR_CurrentThread();
    /* if this is the mozilla thread, then mozilla is not
     * available to service any requests we might make */
    if (curthread == mozilla_thread
	/* if the current thread owns the lj_javasync structure,
	 * then mozilla is tied up in a mocha call somewhere.
	 * we could look at lj_javasync->monitors[0] to see if
	 * the moz thread is waiting there but this should work
	 * as well. */
	|| lj_javasync->owner == env)
      return PR_TRUE;
    return PR_FALSE;
}
