#ifdef XP_UNIX
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include "gcint.h"
#include "mdint.h"
#include "prlog.h"
#include "prthread.h"
#include "prdump.h"
#include "prfile.h"
#include "prprf.h"

PR_LOG_DEFINE(IO);

#ifdef SW_THREADS
#include "swkern.h"
#endif

int _pr_no_clock;
PRMonitor *_pr_md_lock;

PRMonitor *_pr_select_lock;
PRMonitor *_pr_read_lock;
PRMonitor *_pr_write_lock;

PRCList _pr_ioq = PR_INIT_STATIC_CLIST(&_pr_ioq);

int64 _pr_lastTimePolled;
PRBool _pr_pollForIO;

static sigset_t timer_set;

#define LOCK_MD() PR_EnterMonitor(_pr_md_lock)
#define UNLOCK_MD() PR_ExitMonitor(_pr_md_lock)

#ifndef PROT_NONE
#define PROT_NONE 0
#endif

static int zero_fd = -1;

#if defined(NCR) || defined(SNI)

#include <X11/Xlib.h>
#include <dlfcn.h>

typedef Display* XOpenDisplay_type(char*);

Display*
XOpenDisplay(_Xconst char* display_name)
{
    static XOpenDisplay_type* real_XOpenDisplay = NULL;
    Display* dpy;
    
    if ( real_XOpenDisplay == NULL ) {
        void* handle = dlopen("/usr/lib/libX11.so", RTLD_LAZY);
        if ( handle ) {
            real_XOpenDisplay = (XOpenDisplay_type*) dlsym(handle, "XOpenDisplay");
        }
    }

    dpy = real_XOpenDisplay ? real_XOpenDisplay(display_name) : NULL;

    if ( !_pr_no_clock ) {
        struct sigaction vtact;
        vtact.sa_handler = (void (*)()) _PR_ClockInterruptHandler;
        vtact.sa_flags = SA_RESTART;
        sigfillset(&vtact.sa_mask);
        sigaction(SIGALRM, &vtact, 0);
    }

    return dpy; 
}


#endif

void _MD_OpenZero(void)
{
    /* Open /dev/zero if we haven't already */
    if (zero_fd == -1) {
#ifdef DEBUG
	/*
	** Disable using mmap(2) if NSPR_NO_MMAP is set
	*/
	if (getenv("NSPR_NO_MMAP")) {
	    zero_fd = -2;
	    return;
	}
#endif

	/*
	** Open /dev/zero and mmap in a hunk of memory which will
	** correspond to our maximum heap size
	*/
	zero_fd = open("/dev/zero", O_RDWR);
	if (zero_fd < 0) {
	    zero_fd = -2;
	}
    }
}

void *_MD_GrowGCHeap(uint32 *sizep)
{
    static void *lastaddr = (void*) _MD_GC_VMBASE;
    void *addr;
    size_t size;

    size = *sizep;

    LOCK_MD();
    if (zero_fd == -1) {
	_MD_OpenZero();
    }
    if (zero_fd == -2) {
	goto mmap_loses;
    }

    /* Extend the mapping */
    addr = mmap(lastaddr, size, PROT_READ|PROT_WRITE|PROT_EXEC,
#ifdef OSF1
		MAP_PRIVATE|MAP_FIXED,
#else
		MAP_PRIVATE,
#endif
		zero_fd, 0);
    if (addr == (void*)-1) {
        zero_fd = -2;
	goto mmap_loses;
    }
    lastaddr = ((char*)addr + size);
#ifdef DEBUG
    PR_LOG(GC, warn,
	   ("GC: heap extends from %08x to %08x\n",
	    _MD_GC_VMBASE,
	    _MD_GC_VMBASE + (char*)lastaddr - (char*)_MD_GC_VMBASE));
#endif
    UNLOCK_MD();
    return addr;

  mmap_loses:
    UNLOCK_MD();
    return malloc(size);
}

void _MD_FreeGCSegment(void *base, int32 len)
{
    if (zero_fd == -2) {
        free(base);
    } else {
        munmap(base, len);
    }
}

/************************************************************************/

/* List of free stack virtual memory chunks */
PRCList _pr_free_stacks = PR_INIT_STATIC_CLIST(&_pr_free_stacks);

/* How much space to leave between the stacks, at each end */
#define REDZONE		(2 << pr_pageShift)

#define THREAD_STACK_PTR(_qp) \
    ((PRThreadStack*) ((char*) (_qp) - offsetof(PRThreadStack,links)))

/*
** Allocate a stack for a thread.
*/
int _MD_NewStack(PRThread *thread, size_t stackSize)
{
    static char *lastaddr = (char*) _MD_STACK_VMBASE;
    char *p;
    PRCList *qp;
    PRThreadStack *ts = 0;
    int rv;

    /* Adjust stackSize. Round up to a page boundary */
    if (stackSize == 0) {
	stackSize = _MD_DEFAULT_STACK_SIZE;
    }
    stackSize = (stackSize + pr_pageSize - 1) >> pr_pageShift;
    stackSize <<= pr_pageShift;

    /*
    ** Open up the /dev/zero device if we haven't already done it. Some
    ** machines can't do it.
    */
    LOCK_MD();
    if (zero_fd == -1) {
	_MD_OpenZero();
    }
    if (zero_fd == -2) {
	UNLOCK_MD();
	goto mmap_loses;
    }

    /*
    ** Find a free thread stack. This searches the list of free'd up
    ** virtually mapped thread stacks.
    */
    qp = _pr_free_stacks.next;
    ts = 0;
    while (qp != &_pr_free_stacks) {
	ts = THREAD_STACK_PTR(qp);
	qp = qp->next;
	if ((ts->flags == 0) && ((ts->allocSize - 2*REDZONE) >= stackSize)) {
	    /* Found a stack that is not in use and is big enough */
	    PR_REMOVE_LINK(&ts->links);
	    ts->links.next = 0;
	    ts->links.prev = 0;
	    ts->flags = _PR_THREAD_STACK_BUSY;
	    break;
	}
	ts = 0;
    }
    UNLOCK_MD();

    if (!ts) {
	/* Make a new thread stack object. */
	ts = (PRThreadStack*) calloc(1, sizeof(PRThreadStack));
	if (!ts) {
	    return 0;
	}

	/*
	** Assign some of the virtual space to the new stack object. We
	** may not get that piece of VM, but if nothing else we will
	** advance the pointer so we don't collide (unless the OS screws
	** up).
	*/
	ts->allocBase = lastaddr;
	ts->allocSize = stackSize + 2*REDZONE;
	lastaddr = lastaddr + stackSize + 2*REDZONE;
    }

    /*
    ** Map the portion of the stack memory we are going to use, including
    ** the two redzone's. Don't map the entire allocSize, just the
    ** portion we are using this time around.
    */
    p = (char*) mmap(ts->allocBase, stackSize + 2*REDZONE,
		     PROT_READ|PROT_WRITE|PROT_EXEC,
#ifdef OSF1
		     MAP_PRIVATE|MAP_FIXED,
#else
		     MAP_PRIVATE,
#endif
		     zero_fd, 0);
    if (p == (char*)-1) {
	goto mmap_loses;
    }

    /*
    ** Make a redzone at both ends of the stack segment. Disallow access
    ** to those pages of memory. It's ok if the mprotect call's don't
    ** work - it just means that we don't really have a functional
    ** redzone.
    */
    ts->allocBase = p;
    rv = mprotect((void*)p, REDZONE, PROT_NONE);
    PR_ASSERT(rv >= 0);
    rv = mprotect((void*) (p + REDZONE + stackSize), REDZONE, PROT_NONE);
    PR_ASSERT(rv >= 0);
    ts->flags = _PR_THREAD_STACK_BUSY|_PR_THREAD_STACK_MAPPED;
    PR_LOG(THREAD, warn,
	   ("virtual thread stack: base=0x%x limit=0x%x bottom=0x%x top=0x%x\n",
	    ts->allocBase, ts->allocBase + ts->allocSize - 1,
	    ts->allocBase + REDZONE, ts->allocBase + REDZONE + stackSize - 1));
    goto done;

  mmap_loses:
    /*
    ** Use malloc to get the stack memory. This is the fallback code used
    ** on systems which:
    **
    ** 	(1) don't have a /dev/zero, or
    ** 	(2) have a random problem with mmaping.
    **
    ** When we create a stack this way, we can't get a proper redzone so
    ** we just pad the allocation.
    */
    if (!ts) {
	ts = (PRThreadStack*) calloc(1, sizeof(PRThreadStack));
	if (!ts) {
	    return 0;
	}
    }
    p = malloc(stackSize + 2*REDZONE);
    if (!p) {
	free(ts);
	return 0;
    }
    ts->allocBase = p;
    ts->allocSize = stackSize + 2*REDZONE;
    ts->flags = _PR_THREAD_STACK_BUSY;
#ifdef DEBUG
    /*
    ** Fill the redzone memory with a specific pattern so that we can
    ** check for it.
    */
    memset(p, 0xBF, REDZONE);
    memset(p + REDZONE + stackSize, 0xBF, REDZONE);
#endif
    PR_LOG(THREAD, warn,
	   ("malloc thread stack: base=0x%x limit=0x%x bottom=0x%x top=0x%x\n",
	    ts->allocBase, ts->allocBase + ts->allocSize - 1,
	    ts->allocBase + REDZONE, ts->allocBase + REDZONE + stackSize - 1));

  done:
    /*
    ** Setup stackTop and stackBottom values. Note that we are very
    ** careful to use the stackSize variable, NOT the ts->allocSize
    ** variable. The ts->allocSize variable represents the allocated size
    ** of the stack's VIRTUAL region, not the amount in use at the
    ** moment.
    */
    ts->stackSize = stackSize;
#ifdef HAVE_STACK_GROWING_UP
    ts->stackTop = ts->allocBase + REDZONE;
    ts->stackBottom = ts->stackTop + stackSize;
#else
    ts->stackBottom = ts->allocBase + REDZONE;
    ts->stackTop = ts->stackBottom + stackSize;
#endif
#if defined(DEBUG) && !defined(SOLARIS)
    /*
    ** Fill stack memory with something that turns into an illegal
    ** pointer value. This will sometimes find runtime references to
    ** uninitialized pointers. We don't do this for solaris because we
    ** can use purify instead.
    */
    if (ts->stackBottom < ts->stackTop) {
	memset(ts->stackBottom, 0xfe, stackSize);
    } else {
	memset(ts->stackTop, 0xfe, stackSize);
    }
#endif
    thread->stack = ts;
    return 1;
}

/*
** Free a stack for a thread
*/
void _MD_FreeStack(PRThread *thread)
{
    PRThreadStack *ts;

    ts = thread->stack;
    if (!ts) {
	return;
    }

    /* Zap thread->stack to prevent the GC from looking at a dead stack */
    thread->stack = 0;

    if (ts->flags & _PR_THREAD_STACK_MAPPED) {
	(void) munmap(ts->allocBase, ts->allocSize);
	LOCK_MD();
	PR_APPEND_LINK(&ts->links, &_pr_free_stacks);
	ts->flags = 0;
	UNLOCK_MD();
    } else {
	free(ts->allocBase);
	ts->allocBase = 0;
	free(ts);
    }
}

/************************************************************************/

void _MD_StartClock(void)
{
}

#ifdef DEBUG
void DumpStuffAndDie(void)
{
    PR_DumpStuff();
    abort();
}
#endif

#if (defined(IRIX) || defined(SOLARIS)) && defined(DEBUG)
#include <sys/resource.h>
#endif

void _MD_InitUnixOS(int when)
{
    if (when == _MD_INIT_AT_START) {
#ifdef DEBUG
	struct sigaction vtact;
	char *p;

	/*
	** Catch SIGQUIT's early so that we can interrupt what is going
	** on and find out thread status.
	*/
	vtact.sa_handler = (void (*)()) DumpStuffAndDie;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGQUIT, &vtact, 0);

#if defined(IRIX) || defined(SOLARIS)
	p = getenv("PRVM_DATA");
	if (p && *p) {
	    struct rlimit lim;
	    int vlimit = atoi(p);
	    lim.rlim_max = lim.rlim_cur = vlimit * 1024 * 1024;
	    setrlimit(RLIMIT_DATA, &lim);
	}
	p = getenv("PRVM_STACK");
	if (p && *p) {
	    struct rlimit lim;
	    int vlimit = atoi(p);
	    lim.rlim_max = lim.rlim_cur = vlimit * 1024 * 1024;
	    setrlimit(RLIMIT_STACK, &lim);
	}
	p = getenv("PRVM_VM");
	if (p && *p) {
	    struct rlimit lim;
	    int vlimit = atoi(p);
	    lim.rlim_max = lim.rlim_cur = vlimit * 1024 * 1024;
	    setrlimit(RLIMIT_VMEM, &lim);
	}
#endif

#endif
	_pr_md_lock = PR_NewNamedMonitor(0, "md-lock");
    }
}

#ifndef HPUX
void PR_StartEvents(int usec)
{
#ifndef PURIFY
    struct sigaction vtact;
    struct itimerval itval;

    if (getenv("NSPR_NOCLOCK")) {
	_pr_no_clock = 1;
	return;
    }
    if (usec == 0) {
	usec = 50000;
    }

    vtact.sa_handler = (void (*)()) _PR_ClockInterruptHandler;
    vtact.sa_flags = SA_RESTART;
    sigfillset(&vtact.sa_mask);
    sigaction(SIGALRM, &vtact, 0);

    itval.it_interval.tv_sec = usec / 1000000;
    itval.it_interval.tv_usec = usec % 1000000;
    itval.it_value = itval.it_interval;
    if (setitimer(ITIMER_REAL, &itval, 0) < 0) {
        extern int errno;
        fprintf(stderr, "nspr: can't set interval timer: %d\n", errno);
	_pr_no_clock = 1;
    }
#endif
}

void PR_InitInterrupts(void)
{
    struct sigaction sa;

    /* Setup death of a child handler */
    sa.sa_handler = (void (*)()) _PR_ChildDeathInterruptHandler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, 0);

    /* Setup async IO handler */
    sa.sa_handler = (void (*)()) _PR_AsyncIOInterruptHandler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
#ifdef SCO
    sigaction(SIGPOLL, &sa, 0);
#else
    sigaction(SIGIO, &sa, 0);
#endif
}
#endif /* !HPUX */

/************************************************************************/

#if defined(AIX)
char *strdup(const char *src)
{
    char *p;
    int len;

    if (!src) return 0;
    len = strlen(src) + 1;
    p = (char*) malloc(len);
    if (p) {
	memcpy(p, src, len);
    }
    return p;
}
#endif

/************************************************************************/

/*
** Special hacks for xlib. Xlib/Xt/Xm is not re-entrant nor is it thread
** safe.  Unfortunately, neither is mozilla. To make these programs work
** in a pre-emptive threaded environment, we need to use a lock.
*/
int _pr_x_locked = 0;

void PR_XLock(void)
{
    PR_EnterMonitor(_pr_rusty_lock);
    PR_ASSERT(_pr_x_locked == 0);		/* MUST NOT BE RE-ENTERED! */
    _pr_x_locked = 1;
}

void PR_XWait(int ms)
{
    int64 sleep, c;

    LL_I2L(sleep, ms);
    LL_I2L(c, 1000);
    LL_MUL(sleep, sleep, c);

    PR_ASSERT(_pr_x_locked == 1);		/* MUST NOT BE RE-ENTERED! */
    _pr_x_locked = 0;
    PR_Wait(_pr_rusty_lock, sleep);
    PR_ASSERT(_pr_x_locked == 0);		/* MUST NOT BE RE-ENTERED! */
    _pr_x_locked = 1;
}

void PR_XNotify(void)
{
    PR_ASSERT(_pr_x_locked);
    PR_Notify(_pr_rusty_lock);
}

void PR_XNotifyAll(void)
{
    PR_ASSERT(_pr_x_locked);
    PR_NotifyAll(_pr_rusty_lock);
}

void PR_XUnlock(void)
{
    PR_ASSERT(_pr_x_locked);
    _pr_x_locked = 0;
    PR_ExitMonitor(_pr_rusty_lock);
}

int PR_XIsLocked(void)
{
    return _pr_x_locked;
}

/************************************************************************/

typedef struct _PRPollHook _PRPollHook;
struct _PRPollHook {
    int (*func)(int);
};

static int maxPollFDs;
static _PRPollHook *pollHooks;

void PR_SetPollHook(int fd, int (*func)(int))
{
    _PRPollHook *p, *oldPollHooks;
    int is;

    if (fd < 0) return;

    if (func == 0) {
        /* Removing an entry */
        is = _PR_IntsOff();
        pollHooks[fd].func = 0;
        _PR_IntsOn(is, PR_FALSE);
        return;
    }

    if (fd >= maxPollFDs) {
        p = (_PRPollHook*) malloc(sizeof(_PRPollHook) * (fd + 10));

        is = _PR_IntsOff();
        if (maxPollFDs) {
            memcpy(p, pollHooks, maxPollFDs * sizeof(_PRPollHook));
        }
        memset(p + maxPollFDs, 0, ((fd + 10) - maxPollFDs)*sizeof(_PRPollHook));

        oldPollHooks = pollHooks;
        maxPollFDs = fd + 10;
        pollHooks = p;
        pollHooks[fd].func = func;
        _PR_IntsOn(is, PR_FALSE);
        if (oldPollHooks) free(oldPollHooks);
    } else {
        is = _PR_IntsOff();
        pollHooks[fd].func = func;
        _PR_IntsOn(is, PR_FALSE);
    }
}

#define ZAP_SET(_to, _width)				      \
    NSPR_BEGIN_MACRO					      \
	memset(_to, 0,					      \
	       ((_width + 8*sizeof(int)-1) / (8*sizeof(int))) \
		* sizeof(int)				      \
	       );					      \
    NSPR_END_MACRO

#define COPY_SET(_to, _from, _width)			      \
    NSPR_BEGIN_MACRO					      \
	memcpy(_to, _from,				      \
	       ((_width + 8*sizeof(int)-1) / (8*sizeof(int))) \
		* sizeof(int)				      \
	       );					      \
    NSPR_END_MACRO

void _OS_InitIO(void)
{
    if (!_pr_select_lock) {
	_pr_select_lock = PR_NewNamedMonitor(0, "select-lock");
	_pr_read_lock = PR_NewNamedMonitor(0, "read-lock");
	_pr_write_lock = PR_NewNamedMonitor(0, "write-lock");
	sigemptyset(&timer_set);
	sigaddset(&timer_set, SIGALRM);
    }
}

/*
** Pause for i/o, processing the io queue in ioq. This is called with
** interrupts off.
*/
int _PR_PauseForIO(PRBool poll)
{
    PRCList *wq, *pq, *ioq;
    PRPollQueue *pollq;
    PRPollDesc *polld;
    fd_set rd, wr, ex;
    int fd, rv, nr, nw, ne, wantResched, nfds, valid, notify, didNotify;
    int64 max_timeout, timeout, s, us, c;
    struct timeval tv;
    sigset_t oldset;

    sigemptyset(&oldset);

    wantResched = 0;

    /* Make, arbitrarily, the maximum timeout be 24 hours */
    LL_I2L(s, 24*60*60L);
    LL_I2L(c, 1000000L);
    LL_MUL(max_timeout, s, c);

    ioq = &_pr_ioq;

    /*
    ** Scan workq and find largest descriptor numbers. Also find the
    ** smallest timeout value.
    */
    nr = nw = ne = -1;
    wq = ioq->next;
    timeout = max_timeout;
    while (wq != ioq) {
        pollq = POLL_QUEUE_PTR(wq);
        pq = pollq->queue.next;
        valid = 0;
        while (pq != &pollq->queue) {
            polld = POLL_DESC_PTR(pq);
            fd = polld->fd;
            if (polld->in_flags & PR_POLL_READ) {
                if (fd > nr) nr = fd;
                valid = 1;
            }
            if (polld->in_flags & PR_POLL_WRITE) {
                if (fd > nw) nw = fd;
                valid = 1;
            }
            if (polld->in_flags & PR_POLL_EXCEPTION) {
                if (fd > ne) ne = fd;
                valid = 1;
            }
            pq = pq->next;
        }
        if (valid) {
            if (LL_CMP(pollq->timeout, <, timeout)) {
                timeout = pollq->timeout;
            }
        }
        wq = wq->next;
    }

    if (poll) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    } else {
#ifdef SW_THREADS
        /*
        ** Limit timeout to the smaller of the requested timeout or the
        ** first sleepQ waiters time out
        */
        if (!PR_CLIST_IS_EMPTY(&_pr_sleepq)) {
            PRThread *t = THREAD_PTR(_pr_sleepq.next);
            if (LL_CMP(t->sleep, <, timeout)) {
                timeout = t->sleep;
            }
        }
#endif
        /* Convert timeout value into select timeout value */
        LL_DIV(s, timeout, c);
        LL_MOD(us, timeout, c);
        LL_L2I(tv.tv_sec, s);
        LL_L2I(tv.tv_usec, us);
    }

    nfds = -1;
    if (nr > nfds) nfds = nr;
    if (nw > nfds) nfds = nw;
    if (ne > nfds) nfds = ne;
    if (nfds < 0) {
        if (poll) {
            /* Nothing to do right now */
            goto done;
        }

        /*
        ** Nothing to do. Wait for something to do. This is really
        ** where idle acts like idle.
        **
        ** There is *really* nobody to run. Use select to do a
        ** (hopefully) short timeout. Disable clock interrupts
        ** (so that select will actually timeout).
        **
        ** This path of code is executed to keep the scheduler
        ** from blowing up in the uncommon case that there is
        ** no-one else to run.
        */
        sigprocmask(SIG_BLOCK, &timer_set, &oldset);

		PR_LOG(IO, warn, ("begin pause for %d.%d",
                tv.tv_sec, tv.tv_usec));
        (void) _OS_SELECT(0, 0, 0, 0, &tv);
		PR_LOG(IO, warn, ("end pause"));

        wantResched = _PR_ClockTick();
        sigprocmask(SIG_SETMASK, &oldset, 0);
        goto done;
    }
    nfds++;

    /* Clear out select sets using the maximum descriptor number */
    if (nr >= 0) ZAP_SET(&rd, nfds);
    if (nw >= 0) ZAP_SET(&wr, nfds);
    if (ne >= 0) ZAP_SET(&ex, nfds);

    /* Now set the bits that we will be selecting on */
    wq = ioq->next;
    didNotify = 0;
    while (wq != ioq) {
        int notify = 0;
        pollq = POLL_QUEUE_PTR(wq);
        for (pq = pollq->queue.next; pq != &pollq->queue; pq = pq->next) {
            polld = POLL_DESC_PTR(pq);
            fd = polld->fd;
            if (polld->in_flags & PR_POLL_READ) {
                FD_SET(fd, &rd);
                if ((fd < maxPollFDs) && (pollHooks[fd].func != 0)) {
                    int nb = (*pollHooks[fd].func)(fd);
                    if (nb > 0) {
                        /* Pretend that select found something already */
                        polld->out_flags = PR_POLL_READ;
                        notify = 1;
			continue;
                    }
                }
            }
            if (polld->in_flags & PR_POLL_WRITE) {
                FD_SET(fd, &wr);
            }
            if (polld->in_flags & PR_POLL_EXCEPTION) {
                FD_SET(fd, &ex);
            }
        }
        wq = wq->next;
        if (notify) {
            /* Notify the io-waiter */
            PR_ASSERT(pollq->on_ioq);
            PR_REMOVE_LINK(&pollq->links);
            pollq->on_ioq = 0;
            wantResched |= _PR_CondNotify(pollq->mon, _PR_NOTIFY_STICKY);
            didNotify = 1;
        }
    }
    if (didNotify) {
        return wantResched;
    }

    /*
    ** Disable timers during timed select to work around bug in most
    ** unix kernels: if they SA_RESTART a timer then the select never
    ** times out
    */
    sigprocmask(SIG_BLOCK, &timer_set, &oldset);
    PR_LOG(IO, debug,
	   ("select: nfds=%d rd=%x wr=%x ex=%x tv=%d.%d", nfds,
            (nr >= 0) ? *(int*)&rd : -1,
            (nw >= 0) ? *(int*)&wr : -1,
            (ne >= 0) ? *(int*)&ex : -1,
            tv.tv_sec, tv.tv_usec));
    rv = _OS_SELECT(nfds,
                    (nr >= 0) ? &rd : 0,
                    (nw >= 0) ? &wr : 0,
                    (ne >= 0) ? &ex : 0,
                    &tv);
    PR_LOG(IO, debug,
	   ("select returns %d: rd=%x wr=%x ex=%x", rv,
            (nr >= 0) ? *(int*)&rd : -1,
            (nw >= 0) ? *(int*)&wr : -1,
            (ne >= 0) ? *(int*)&ex : -1));
    /*
    ** Select has returned. We need to tick the clock because we
    ** disabled timers (or we don't have a clock). We don't
    ** deschedule this thread until we have to.
    **
    ** Note that we are careful to call the _PR_ClockTick routine
    ** *before* we re-enable the clock signal handler. This way we
    ** are guarantted to not re-enter the _PR_ClockTick routine which
    ** would be disastrous.
    */
    wantResched = _PR_ClockTick();
    sigprocmask(SIG_SETMASK, &oldset, 0);
    if (rv <= 0) {
        /* Timeout or error */
        if (rv == 0) {
            /* Timeout */
            goto done;
        }
        if (rv == -EINTR) {
            /* We were interrupted by something. Oh well */
            goto done;
        }
        /*
        ** Assume (oh boy) that we got an EBADF and that one of the
        ** descriptors we were handed is bad. The code below
        ** discovers which descriptor it was and marks it bad.
        */
	    PR_LOG(IO, error, ("select returns error %d", -rv));
    }

    /* Notify threads waiting for i/o to complete */
    wq = ioq->next;
    while (wq != ioq) {
        pollq = POLL_QUEUE_PTR(wq);

        /* Scan each pollq on the io queue */
        pq = pollq->queue.next;
        notify = 0;
        while (pq != &pollq->queue) {
            polld = POLL_DESC_PTR(pq);
            fd = polld->fd;
            polld->out_flags = 0;
            if (rv < 0) {
                struct stat sb;
                if (fstat(polld->fd, &sb) < 0) {
                    /* Found a bad descriptor */
                    polld->out_flags |= PR_POLL_ERROR;
			PR_LOG(IO, error,
			       ("fd %d was bad", polld->fd));
                }
            } else {
                if (polld->in_flags & PR_POLL_READ) {
                    if (FD_ISSET(fd, &rd)) {
                        polld->out_flags |= PR_POLL_READ;
                    }
                }
                if (polld->in_flags & PR_POLL_WRITE) {
                    if (FD_ISSET(fd, &wr)) {
                        polld->out_flags |= PR_POLL_WRITE;
                    }
                }
                if (polld->in_flags & PR_POLL_EXCEPTION) {
                    if (FD_ISSET(fd, &ex)) {
                        polld->out_flags |= PR_POLL_EXCEPTION;
                    }
                }
            }
            if (polld->out_flags != 0) {
                notify = 1;
            }
            pq = pq->next;
        }

        wq = wq->next;
        if (notify) {
            /* Notify the io-waiter */
            PR_ASSERT(pollq->on_ioq);
            PR_REMOVE_LINK(&pollq->links);
            pollq->on_ioq = 0;
            wantResched |= _PR_CondNotify(pollq->mon, _PR_NOTIFY_STICKY);
        }
    }
  done:;
    return wantResched;
}

void _PR_Idle(void)
{
    for (;;) {
        int is = _PR_IntsOff();
        /*
        ** When idle is actually running we don't need or want to poll
        ** for i/o
        */
        _pr_pollForIO = PR_FALSE;
        _PR_PauseForIO(PR_FALSE);

        /* Now that idle is done for awhile, enable polling */
        _pr_pollForIO = PR_TRUE;
        _pr_lastTimePolled = PR_Now();

        /* Give anybody else a chance to run */
        _PR_IntsOn(is, PR_TRUE);
    }
}

extern PRMonitor *_PR_CGetMonitor(void *address);

int _PR_IOWait(PRPollQueue *pollq)
{
    PRMonitor *mon;
    int is;

    /*
    ** Get (and enter) cache monitor for pollq; store it where the io
    ** handler can find it to do a sticky notify
    */
    mon = _PR_CGetMonitor(pollq);
    pollq->mon = mon;

    /* Add q to i/o handlers work queue */
    is = _PR_IntsOff();
    PR_APPEND_LINK(&pollq->links, &_pr_ioq);
    pollq->on_ioq = 1;
    _PR_IntsOn(is, PR_FALSE);

    /* Wait for i/o to finish or timeout */
    PR_CWait(pollq, pollq->timeout);
    PR_CExitMonitor(pollq);

    /*
    ** Remove pollq from ioq, now that the i/o is done or timed out. We
    ** only need to remove it from the ioq when the PR_CWait returns
    ** before the idle thread notifies us.
    */
    if (pollq->on_ioq) {
        is = _PR_IntsOff();
	if (pollq->on_ioq) {		/* check again because we are racing */
	    PR_REMOVE_LINK(&pollq->links);
	    pollq->on_ioq = 0;
	}
        _PR_IntsOn(is, PR_FALSE);
    }

    pollq->mon = 0;
    return 0;
}

/* XXX go away? */
int _PR_WaitForFD(PRFileDesc *fd, int rd)
{
    return _PR_WaitForFH(fd->osfd, rd);
}

int _PR_WaitForFH(int fd, int reading)
{
    PRPollQueue q;
    PRPollDesc d;

    q.timeout = LL_MAXINT;
    PR_INIT_CLIST(&q.links);
    PR_INIT_CLIST(&q.queue);
    d.fd = fd;
    d.in_flags = reading ? PR_POLL_READ : PR_POLL_WRITE;
    PR_APPEND_LINK(&d.links, &q.queue);
    _PR_IOWait(&q);
    return (d.out_flags & PR_POLL_ERROR) ? -1 : 0;
}

/*
** Wrap up the select system call so that we can deschedule a thread that
** tries to wait for i/o.
*/
#define MAX_DESC	50
#ifdef AIX
int wrap_select(unsigned long width, void *rl, void *wl, void *el,
		struct timeval *tv)
#elif HPUX
int select(size_t width, int *rl, int *wl, int *el, const struct timeval *tv)
#else
int select(int width, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *tv)
#endif
{
    int i, nfds;
    PRPollQueue q;
    PRPollDesc desc[MAX_DESC], *pd, *pdbase, *pdend;
    struct timeval poll;
#if defined(AIX) || defined(HPUX)
    fd_set *rd = (fd_set*) rl;
    fd_set *wr = (fd_set*) wl;
    fd_set *ex = (fd_set*) el;
#endif
    fd_set r, w, x;

    if (width < 0) {
	errno = EINVAL;
	return -1;
    }

    /*
    ** Call select right now with a zero timeout to see if i/o is already
    ** ready for the caller. If not, then go through the slower path of
    ** blocking this thread.
    **
    ** NOTE: timeout must be zero unless signals are blocked. See the
    ** commentary in _PR_PauseForIO
    */
    poll.tv_sec = 0;
    poll.tv_usec = 0;
    if (rd) { COPY_SET(&r, rd, width); }
    if (wr) { COPY_SET(&w, wr, width); }
    if (ex) { COPY_SET(&x, ex, width); }
    nfds = _OS_SELECT(width, rd, wr, ex, &poll);
    if (nfds < 0) {
        errno = -nfds;
        return -1;
    }
    if (nfds > 0) {
        return nfds;
    }

    /* Setup for pr_io_wait */
    pd = pdbase = desc;
    pdend = desc + MAX_DESC;
  start_over:
    PR_INIT_CLIST(&q.links);
    PR_INIT_CLIST(&q.queue);
    for (i = 0; i < width; i++) {
	int in_flags = 0;
	if (rd && FD_ISSET(i, &r)) {
            if ((i < maxPollFDs) && (pollHooks[i].func != 0)) {
                int nb = (*pollHooks[i].func)(i);
                if (nb > 0) {
                    /* Pretend that select found something already */
                    ZAP_SET(rd, width);
                    if (wr) ZAP_SET(wr, width);
                    if (ex) ZAP_SET(ex, width);
                    FD_SET(i, rd);
                    return 1;
                }
            }
	    in_flags |= PR_POLL_READ;
	}
	if (wr && FD_ISSET(i, &w)) {
	    in_flags |= PR_POLL_WRITE;
	}
	if (ex && FD_ISSET(i, &x)) {
	    in_flags |= PR_POLL_EXCEPTION;
	}
	if (in_flags) {
	    pd->fd = i;
	    pd->in_flags = in_flags;
#ifdef DEBUG_kipp
	    pd->out_flags = 0x8000;
#else
	    pd->out_flags = 0;
#endif
	    PR_APPEND_LINK(&pd->links, &q.queue);
	    pd++;
	    if (pd == pdend) {
		/*
		** Whoops. Ran out of room. Malloc some space for the
		** poll desc's. Then start over and rebuild the list
		** using the new descriptor memory.
		*/
		PR_ASSERT(pdbase == desc);
		pdbase = (PRPollDesc *) calloc(width+1,sizeof(PRPollDesc));
		if (!pdbase) {
		    /* Losing big time */
		    errno = ENOMEM;
		    return -1;
		}
		pdend = pdbase + width + 1;
		pd = pdbase;
		goto start_over;
	    }
	}
    }
    if (!tv) {
        PR_ASSERT(pd > pdbase);
    }

    /* Compute timeout */
    if (tv) {
	int64 s, us, m;
	int32 is, ius;

	is = tv->tv_sec;
	ius = tv->tv_usec;
	while (ius > 1000000) {
	    is++;
	    ius -= 1000000;
	}
	LL_I2L(s, is);
	LL_I2L(us, ius);
	LL_I2L(m, 1000000);
	LL_MUL(s, s, m);
	LL_ADD(q.timeout, s, us);
    } else {
	q.timeout = LL_MAXINT;
    }

    /* Check for no descriptors case (just doing a timeout) */
    if (width == 0) {
	PR_CEnterMonitor(&q);
	PR_CWait(&q, q.timeout);
	PR_CExitMonitor(&q);
	return 0;
    }

    /* wait for i/o */
    _PR_IOWait(&q);

    /* Compute select results */
    if (rd) ZAP_SET(rd, width);
    if (wr) ZAP_SET(wr, width);
    if (ex) ZAP_SET(ex, width);
    nfds = 0;
    while (--pd >= pdbase) {
	if (pd->out_flags) {
	    if (pd->out_flags & PR_POLL_ERROR) {
		/* Tsk tsk. */
		errno = EBADF;
		PR_LOG(IO, error,
		       ("select returns EBADF for %d", pd->fd));
		nfds = -1;
		break;
	    }
	    if (rd && (pd->out_flags & PR_POLL_READ)) {
		FD_SET(pd->fd, rd);
	    }
	    if (wr && (pd->out_flags & PR_POLL_WRITE)) {
		FD_SET(pd->fd, wr);
	    }
	    if (ex && (pd->out_flags & PR_POLL_EXCEPTION)) {
		FD_SET(pd->fd, ex);
	    }
#ifdef DEBUG_kipp
	    if (pd->out_flags == 0x8000) {
		continue;
	    }
#endif
	    nfds++;
	}
    }
    if (!tv) {
        PR_ASSERT(nfds != 0);
    }
    PR_LOG(IO, debug, ("select returns %d", nfds));
    if (pdbase != desc) {
	free(pdbase);
    }
    return nfds;
}

int PR_Select(int width, fd_set *readfds, fd_set *writefds,
	      fd_set *exceptfds, struct timeval *tv)
{
    /* XXXrobm If OSFD's are distinct from PRFileHandles, we have to convert
       a PRFileHandle select set to an OSFD one. */
    return select(width, readfds, writefds, exceptfds, tv);
}

void PR_NonBlockIO(int desc, int on)
{
}
#endif /* XP_UNIX */

/*
** APIs to return system information...
*/
#if !defined(IRIX) && !defined(SOLARIS)  && !defined(SNI)
long _MD_GetOSName(char *buf, long count)
{
    struct utsname info;
    long len = 0;

    if( uname(&info) != EFAULT ) {
        len = PR_snprintf(buf, count, info.sysname);
    }
    return len;
}

long _MD_GetOSVersion(char *buf, long count)
{
    struct utsname info;
    long len = 0;

    if( uname(&info) != EFAULT ) {
        len = PR_snprintf(buf, count, info.release);
    }
    return len;
}

long _MD_GetArchitecture(char *buf, long count)
{
    struct utsname info;
    long len = 0;

    if( uname(&info) != EFAULT ) {
        len = PR_snprintf(buf, count, info.machine);
    }
    return len;
}
#endif /* !defined(IRIX) && !defined(SOLARIS) !defined(SNI) */
