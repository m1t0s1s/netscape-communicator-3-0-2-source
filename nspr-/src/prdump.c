#include "prdump.h"
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include "prglobal.h"
#include "prmacros.h"
#include "prprf.h"
#ifndef XP_MAC
#include <prfile.h>
#endif
#ifdef SW_THREADS
#include "swkern.h"
#endif
#ifdef HW_THREADS
#include "hwkern.h"
#endif

static char *null_str = "<null>";

#define FIX_NULL(_p) ((_p) ? (_p) : null_str)

#if defined(XP_PC) && defined(_WIN32)

#include "prprf.h"
/*
** For the PC, _PR_DebugPrint writes to stdout which is caught by NSPR and
** redirected to an output window...
*/
int
_PR_DebugPrint(FILE *f, const char *fmt, ...)
{
    char buffer[512];
    int ret = 0;

    va_list args;
    va_start(args, fmt);
    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);
    if( f == stderr ) {
        ret = PR_WRITE(1, buffer, strlen(buffer));
    } else {
        ret = fprintf(f, "%s", buffer);
    }
    va_end(args);
    return ret;
}
#endif  /* XP_PC && _WIN32 */

PR_PUBLIC_API(size_t)
PR_PrintMicrosecondsToBuf(char* buf, size_t buflen, int64 v)
{
    size_t count = 0;

#define TRY_TIME(lower, upper, units)			   \
    {							   \
	int64 lb, ub, tmp;				   \
	LL_I2L(lb, lower);				   \
	LL_DIV(tmp, v, lb);	/* divide down */	   \
	v = tmp;					   \
	LL_I2L(ub, upper);				   \
	if (LL_CMP(v, <, ub)) {				   \
	    int i;					   \
	    LL_L2I(i, v);				   \
	    count += PR_snprintf(&buf[count], buflen-count,\
				 "%d" units " ", i);	   \
	    return count;				   \
	}						   \
    }							   \

    TRY_TIME(   1, 1000, "us");
    TRY_TIME(1000, 1000, "ms");
    TRY_TIME(1000,   60, "sec");
    TRY_TIME(  60,   60, "min");
    TRY_TIME(  60,   24, "hrs");
    TRY_TIME(  24,  365, "days");
    TRY_TIME( 365, 1000, "yrs");

#undef TRY_TIME
    count += PR_snprintf(&buf[count], buflen-count, "inf ");
    return count;
}

PR_PUBLIC_API(void)
PR_PrintMicroseconds(FILE* file, int64 v)
{
    char buf[256];
    PR_PrintMicrosecondsToBuf(buf, sizeof(buf), v);
    _PR_DebugPrint(file, buf);
}

static void
pr_DumpOneThread(FILE* out, PRThread *t)
{
    int in = _PR_IntsOff();
#ifdef SW_THREADS
    _PR_DebugPrint(out, "  %-20s %8lx %3d ", FIX_NULL(t->name), (long)t, (int)t->priority);
#else
    _PR_DebugPrint(out, "  %-20s %8x %3d ", FIX_NULL(t->name), t->id, t->priority);
#endif
    switch (t->state) {
      case _PR_UNBORN:
	_PR_DebugPrint(out, "unborn");
	break;
      case _PR_RUNNING:
	_PR_DebugPrint(out, "running");
	break;
      case _PR_RUNNABLE:
	_PR_DebugPrint(out, "runnable");
	break;
      case _PR_SUSPENDED:
	_PR_DebugPrint(out, "suspended");
	break;
      case _PR_SLEEPING:
	_PR_DebugPrint(out, "sleeping ");
	PR_PrintMicroseconds(out, t->sleep);
	break;
      case _PR_COND_WAIT:
	_PR_DebugPrint(out, "condwait ");
	PR_PrintMicroseconds(out, t->sleep);
	PR_Monitor_print(out, t->monitor);
	break;
      case _PR_MON_WAIT:
	_PR_DebugPrint(out, "monwait  ");
	PR_Monitor_print(out, t->monitor);
	break;
      case _PR_ZOMBIE:
	_PR_DebugPrint(out, "zombie");
	break;
    }
    _PR_DebugPrint(out, "\n");
    if (t->dump) {
	(*t->dump)(t, out);
    }
    _PR_IntsOn(in, PR_FALSE);
}

PR_PUBLIC_API(void)
PR_DumpOneThread(PRThread *t)
{
    pr_DumpOneThread(stderr, t);
}

/* XXX use enumerate threads */
static void
pr_DumpThreads(FILE* out)
{
    PRCList *qp;
    int j;
    int in = _PR_IntsOff();

    _PR_DebugPrint(out, "\n--Thread---------------ID-------Pri-State--------------------------------------\n");
    if (PR_CurrentThread()) {
	_PR_DebugPrint(out, "Current:\n");
	pr_DumpOneThread(out, PR_CurrentThread());
    } else {
	_PR_DebugPrint(out, "Current: none\n");
    }

    _PR_DebugPrint(out, "RunQ:\n");
    for (j = 0; j < PR_NUM_PRIORITIES; j++) {
	qp = _pr_runq[j].next;
	while (qp != &_pr_runq[j]) {
	    pr_DumpOneThread(out, THREAD_PTR(qp));
	    qp = qp->next;
	}
    }
#ifdef SW_THREADS
    _PR_DebugPrint(out, "MonitorQ:\n");
    qp = _pr_monitorq.next;
    while (qp != &_pr_monitorq) {
	pr_DumpOneThread(out, THREAD_PTR(qp));
	qp = qp->next;
    }
    _PR_DebugPrint(out, "SleepQ:\n");
    qp = _pr_sleepq.next;
    while (qp != &_pr_sleepq) {
	pr_DumpOneThread(out, THREAD_PTR(qp));
	qp = qp->next;
    }
    _PR_DebugPrint(out, "SuspendQ:\n");
    qp = _pr_suspendq.next;
    while (qp != &_pr_suspendq) {
	pr_DumpOneThread(out, THREAD_PTR(qp));
	qp = qp->next;
    }
#endif
    _PR_DebugPrint(out, "-------------------------------------------------------------------------------\n");

    _PR_IntsOn(in, PR_FALSE);
}

PR_PUBLIC_API(void)
PR_DumpThreads(void)
{
    pr_DumpThreads(stderr);
}

static int
PR_Monitor_isActive(PRMonitor* mon)
{
    return (mon != NULL
	    && mon->owner != NULL
	    && mon->count != 0
	    && !PR_CLIST_IS_EMPTY(&mon->links));
}

static void
pr_DumpMonitors(FILE* out)
{
    PRCList *qp;
    int in = _PR_IntsOff();
 
    _PR_DebugPrint(out, "\n--Monitors---------------------------------------------------------------------");
    qp = _pr_all_monitors.next;
    while (qp != &_pr_all_monitors) {
	PRMonitor* mon = MONITOR_ALL_PTR(qp);
	if (PR_Monitor_isActive(mon)) {
	    _PR_DebugPrint(out, "\n  ");
	    PR_Monitor_print(out, mon);
	}
	qp = qp->next;
    }
    _PR_DebugPrint(out, "\n-------------------------------------------------------------------------------\n");

    _PR_IntsOn(in, PR_FALSE);
}

PR_PUBLIC_API(void)
PR_DumpMonitors(void)
{
    pr_DumpMonitors(stderr);
}

#undef abort

/* in prmcache.c */
extern void pr_DumpMonitorCache(FILE* out);

static void
pr_DumpStuff(FILE* out, PRBool detailed)
{
    pr_DumpThreads(out);
    pr_DumpMonitors(out);
    pr_DumpMonitorCache(out);
}

PR_PUBLIC_API(void)
PR_DumpStuff()
{
    pr_DumpStuff(stderr, PR_FALSE);
#if !defined(XP_UNIX) && !defined(XP_PC)
    abort();
#endif
}

/******************************************************************************/

PR_PUBLIC_API(void)
PR_Monitor_print(FILE* file, PRMonitor* mon)
{
    int in = _PR_IntsOff();
    if (PR_Monitor_isActive(mon))
	_PR_DebugPrint(file, "[Monitor %s count=%d flags=%x owner=%s %lx]",
		   FIX_NULL(mon->name), mon->count, mon->flags,
		   (mon->owner ? FIX_NULL(mon->owner->name) : "<none>"), (long)mon);
    else
	_PR_DebugPrint(file, "[Monitor %s %lx]", FIX_NULL(mon->name), (long)mon);
    _PR_IntsOn(in, PR_FALSE);
}

PR_PUBLIC_API(void)
PR_Thread_print(FILE* file, PRThread* t)
{
    int in = _PR_IntsOff();
    if (!t)
	_PR_DebugPrint(file, "[Thread 0]");
    else {
	_PR_DebugPrint(file, "[Thread %s pri=%d flags=%x state=",
		FIX_NULL(t->name), (int)t->priority, t->flags);
	switch (t->state) {
	  case _PR_UNBORN:
	    _PR_DebugPrint(file, "UNBORN");
	    break;
	  case _PR_RUNNING:
	    _PR_DebugPrint(file, "RUNNING");
	    break;
	  case _PR_RUNNABLE:
	    _PR_DebugPrint(file, "RUNNABLE");
	    break;
	  case _PR_SUSPENDED:
	    _PR_DebugPrint(file, "SUSPENDED");
	    break;
	  case _PR_SLEEPING:
	    _PR_DebugPrint(file, "SLEEPING ");
	    PR_PrintMicroseconds(file, t->sleep);
	    break;
	  case _PR_COND_WAIT:
	    _PR_DebugPrint(file, "COND_WAIT ");
	    PR_PrintMicroseconds(file, t->sleep);
	    PR_Monitor_print(file, t->monitor);
	    break;
	  case _PR_MON_WAIT:
	    _PR_DebugPrint(file, "MON_WAIT ");
	    PR_Monitor_print(file, t->monitor);
	    break;
	  case _PR_ZOMBIE:
	    _PR_DebugPrint(file, "ZOMBIE");
	    break;
	  default:
	    _PR_DebugPrint(file, "?");
	}
#ifdef SW_THREADS
	_PR_DebugPrint(file, " %lx]", (long)t);
#else
	_PR_DebugPrint(file, " %x]", t->id);
#endif
    }
    _PR_IntsOn(in, PR_FALSE);
}

static void
pr_ThreadFun(FILE* out, char* funName, PRThread* thread, PRMonitor* mon)
{
    _PR_DebugPrint(out, "### %s: ", FIX_NULL(funName));
    PR_Thread_print(out, thread);
    if (mon) {
	_PR_DebugPrint(out, ", ");
	PR_Monitor_print(out, mon);
    }
    _PR_DebugPrint(out, "\n");
}

PR_PUBLIC_API(void)
PR_ThreadFun(char* funName, PRThread* thread, PRMonitor* mon)
{
    pr_ThreadFun(stderr, funName, thread, mon);
}

/******************************************************************************/

static void
_PR_DefaultShowStatusHook(void)
{
    int in = _PR_IntsOff();
    pr_DumpStuff(stderr, PR_FALSE);
    _PR_IntsOn(in, PR_FALSE);
}

void (*PR_ShowStatusHook)(void) = _PR_DefaultShowStatusHook;

/******************************************************************************/

#ifndef _WIN32
#define OutputDebugString(msg)
#endif

PR_PUBLIC_API(void)
PR_DumpToFile(char* filename, char* msg, PRFileDumper dump, PRBool detailed)
{
    FILE *out;
    OutputDebugString(msg);
    out = fopen(filename, "a");
    if (!out) {
	char buf[64];
	PR_ASSERT(strlen(filename) < sizeof(buf) - 16);
	PR_snprintf(buf, sizeof(buf), "Can't open \"%s\"\n",
		    filename);
	OutputDebugString(buf);
    }
    else
    {
	struct tm *newtime;
	time_t aclock;
	int i;

	time(&aclock);
	newtime = localtime(&aclock);
	fprintf(out, "%s on %s\n", msg, asctime(newtime));  /* Print current time */
	dump(out, detailed);
	fprintf(out, "\n\n");
	for (i = 0; i < 80; i++)
	    fprintf(out, "=");
	fprintf(out, "\n\n");
	fclose(out);
    }
    OutputDebugString(" done\n");
}

/******************************************************************************/

PR_PUBLIC_API(void)
PR_DumpStuffToFile(void)
{
    PR_DumpToFile("memory.out", "Thread Summary", pr_DumpStuff, PR_FALSE);
}

/******************************************************************************/
