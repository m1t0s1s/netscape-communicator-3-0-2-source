/*
** Purify threads interface for NSPR.
**
** XXX all we really want for locks are mutexs, not PRMonitors.
*/
#include <stdio.h>
#include "prmon.h"
#include "prthread.h"
#include "pure_thr.h"

unsigned int
pure_lock_size()
{
    return sizeof(PRMonitor);
}

void
pure_initialize_lock(void *lock)
{
    PRMonitor *mon = lock;
    static int counter = 1;
    char buf[80];

    sprintf(buf, "pure-lock-%d", counter++);
    PR_InitMonitor(mon, 0, buf);
}

int
pure_get_lock(void *lock, int wait_for_lock)
{
    PRMonitor *mon = lock;

    if (!wait_for_lock) {
        abort();	/* XXX not yet implemented */
    }
    PR_EnterMonitor(mon);
    return 1;
}

void
pure_release_lock(void *lock)
{
    PRMonitor *mon = lock;

    PR_ExitMonitor(mon);
}

unsigned int
pure_thread_id_size()
{
    return sizeof(int32);
}

void
pure_thread_id(void *id_p)
{
    *(int32 *)id_p = PR_CurrentThread()->id;
}

int
pure_thread_id_equal(void *id1_p, void *id2_p)
{
    return *(int32 *)id1_p == *(int32 *)id2_p;
}

int pure_thread_init_protocol = PURE_THREAD_INIT_EXPLICIT;

int pure_thread_switch_protocol = PURE_THREAD_PROTOCOL_EXPLICIT_CONTEXT_SWITCH;
