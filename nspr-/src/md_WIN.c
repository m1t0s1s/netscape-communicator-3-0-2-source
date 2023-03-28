#include <windows.h>
#include <io.h>                     /* _open() and _close() */
#include <fcntl.h>                  /* O_RDONLY */
#include <stddef.h>                 /* offsetof() */
#include <string.h>                 /* memset() */
#include "prthread.h"
#include "prmon.h"
#include "prprf.h"
#include "mdint.h"
#include "prlog.h"

#ifdef SW_THREADS
#include "swkern.h"
#endif

/* The "main" thread which performed the NSPR initialization. */
PRThread *_pr_primary_thread;

/************************************************************************/
/*
** Machine Dependent routines for thread-stack management:
*/
/************************************************************************/

/* List of free stack virtual memory chunks */
PRCList _pr_free_stacks = PR_INIT_STATIC_CLIST(&_pr_free_stacks);

/*
** Macros for locking the list of freee stack structures...
**
** Since, Win16 is not a preemptive environment, no locking
** is necessary.
*/
#ifdef _WIN32
PRMonitor *_pr_md_lock;

#define CREATE_MD_LOCK()    _pr_md_lock = PR_NewNamedMonitor(0, "md-lock");
#define LOCK_MD()           PR_EnterMonitor(_pr_md_lock)
#define UNLOCK_MD()         PR_ExitMonitor(_pr_md_lock)

#else  /* ! _WIN32 */

#define CREATE_MD_LOCK()
#define LOCK_MD()   
#define UNLOCK_MD() 

#endif  /* ! WIN32 */

#define THREAD_STACK_PTR(_qp) \
    ((PRThreadStack*) ((char*) (_qp) - offsetof(PRThreadStack,links)))


#ifdef HW_THREADS
/************************************************************************/
/*
** Native threading implementations of:
**   _MD_NewStack
**   _MD_FreeStack
**   _MD_InitializeStack (Native threads only)
*/
/************************************************************************/

/*
** Allocate a stack for a thread.
*/
int _MD_NewStack(PRThread *thread, size_t stackSize)
{
    PRCList *qp;
    PRThreadStack *ts = 0;

    /* Adjust stackSize. Round up to a page boundary */
    if (stackSize == 0) {
        stackSize = _MD_DEFAULT_STACK_SIZE;
    }
    stackSize = (stackSize + pr_pageSize - 1) >> pr_pageShift;
    stackSize <<= pr_pageShift;

    /*
    ** Find a free thread stack. This searches the list of free'd up
    ** virtually mapped thread stacks.
    */
    LOCK_MD();
    qp = _pr_free_stacks.next;
    ts = 0;
    while (qp != &_pr_free_stacks) {
        ts = THREAD_STACK_PTR(qp);
        qp = qp->next;
        if (ts->flags == 0) {
            /* Found a stack that is not in use */
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
        ts->flags = _PR_THREAD_STACK_BUSY;
    }

    ts->stackSize = stackSize;
    ts->stackTop  = 0;
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

    /* place the thread stack back in the free list */
    LOCK_MD();
    PR_APPEND_LINK(&ts->links, &_pr_free_stacks);
    ts->flags = 0;
    UNLOCK_MD();
}

/*
** Initialize a stack for a thread
*/
void _MD_InitializeStack(PRThreadStack *ts)
{
    if( ts && (ts->stackTop == 0) ) {
        /* In Win32 the stacks are NOT allocated from the heap...*/
        ts->allocBase = (char *)&ts;
        ts->allocSize = ts->stackSize;

        /*
        ** Setup stackTop and stackBottom values. 
        ** On PCs the stack grows "down"
        */
        ts->stackTop    = ts->allocBase;
        ts->stackBottom = ts->allocBase - ts->stackSize;
    }
}
#endif  /* HW_THREADS */



#ifdef SW_THREADS
/************************************************************************/
/*
** User threading implementations of:
**   _MD_NewStack
**   _MD_FreeStack
*/
/************************************************************************/

/* How much space to leave between the stacks, at each end */
#define REDZONE     (2 << pr_pageShift)

/*
** Allocate a stack for a thread.
*/
int _MD_NewStack(PRThread *thread, size_t stackSize)
{
    char *p;
    PRThreadStack *ts = 0;

    /* Adjust stackSize. Round up to a page boundary */
    if (stackSize == 0) {
        stackSize = _MD_DEFAULT_STACK_SIZE;
    }
    stackSize = (size_t)((stackSize + pr_pageSize - 1) >> pr_pageShift);
    stackSize <<= pr_pageShift;

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
    ts = (PRThreadStack*) calloc(1, sizeof(PRThreadStack));
    if (!ts) {
        return 0;
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
	   ("malloc thread stack: base=0x%x limit=0x%x bottom=0x%x top=0x%x",
            ts->allocBase, ts->allocBase + ts->allocSize - 1,
            ts->allocBase + REDZONE, ts->allocBase + REDZONE + stackSize - 1));

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
#if !defined(_WIN32)
    ts->SP = ts->stackTop;
#endif
#if defined(DEBUG)
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

    free(ts->allocBase);
    ts->allocBase = 0;
    free(ts);
}

#endif  /* SW_THREADS */



/************************************************************************/
/*
** Machine dependent initialization routines:
**    _MD_InitOS
**    _MD_StartClock
*/
/************************************************************************/
extern void __md_InitOS(int);

void _MD_InitOS(int when)
{
    if (when == _MD_INIT_AT_START) {
        CREATE_MD_LOCK();
        /* Register the windows message for NSPR Event notification */
        _pr_PostEventMsgId = RegisterWindowMessage("NSPR_PostEvent");
    } 
    else if (when == _MD_INIT_READY) {
        _pr_primary_thread = PR_CurrentThread();
    }
    /*
    ** Perform any Win16/Win32 only initializations...
    */
    __md_InitOS(when);
}

/************************************************************************/
/*
** Machine dependent timer routines:
**    _MD_StartClock
*/
/************************************************************************/
#ifdef SW_THREADS

extern void _PR_ClockInterruptHandler(void);

extern void CALLBACK _loadds TickTimerProc(HWND,UINT,UINT,DWORD);

static UINT TimerId;


void _MD_StartClock(void)
{
    /* Initialize a Windows timer */
    TimerId = SetTimer(NULL, 0, 100, TickTimerProc);

    /*
    ** XXX:  How do we fail if a timer is unavailable ??
    */
    if( TimerId == 0 ) {
        exit(1);
    }
}

void _PR_Pause(void) 
{
    MSG msg;

    /*
    ** Call PeekMessage(...) to allow other Tasks to run...
    ** (This is important for Win16 only)
    */
    (void) PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    PR_Yield();
}

/*
** Timer callback routine...
** 
** This routine is called for every WM_TIMER message.
*/
static int OldPriorityOfPrimaryThread   = -1;
static int TimeSlicesOnNonPrimaryThread =  0;

void CALLBACK _loadds TickTimerProc(HWND hWnd, UINT msg, UINT  uTimerId, DWORD time) 
{
    /* Tell nspr that a clock interrupt occured. */
    _PR_ClockInterruptHandler();
}

#endif /* SW_THREADS */

#ifdef HW_THREADS
/*
** The native threading implementation does NOT require timers to
** perform the thread scheduling...
*/
void _MD_StartClock(void) {}

void _PR_Pause(void) 
{
    PR_Yield();
}

#endif /* HW_THREADS */

/************************************************************************/
/*
** Machine dependent routines for returning system information:
**    _MD_GetHostname
**    _MD_GetOSName
**    _MD_GetOSVersion
**    _MD_GetArchitecture
*/
/************************************************************************/

long _MD_GetHostname(char *buf, long count)
{
    long i = 0;

    if( gethostname(buf, (int)count) == 0 ) {
        while( buf[i] && i < count ) {
            if( buf[i] == '.' ) {
                buf[i] = '\0';
                break;
            }
            i += 1;
        }    
    }
    return i;
}

long _MD_GetOSName(char *buf, long count)
{
    long len = 0;

#ifdef _WIN32
    char *name;
    OSVERSIONINFO info;

    name = NULL;
    info.dwOSVersionInfoSize = sizeof(info);
    if( GetVersionEx( &info ) ) {
        switch( info.dwPlatformId ) {
          case VER_PLATFORM_WIN32s:
            name = "Win32s";
            break;

          case VER_PLATFORM_WIN32_WINDOWS:
            name = "Windows 95";
            break;

          case VER_PLATFORM_WIN32_NT:
            name = "Windows NT";
            break;
        }  
        if( name ) {
            len = PR_snprintf(buf, count, name);
        }
    }
#else   /* ! _WIN32 */
    len = PR_snprintf(buf, (size_t)count, "16-bit Windows");
#endif  /* !_WIN32 */

    return len;
}


long _MD_GetOSVersion(char *buf, long count)
{
    long len = 0;

#ifdef _WIN32
    char *version;
    OSVERSIONINFO info;

    version = NULL;
    info.dwOSVersionInfoSize = sizeof(info);
    if( GetVersionEx( &info ) ) {
        len = PR_snprintf(buf, count, "%d.%d", info.dwMajorVersion, 
                          info.dwMinorVersion);
    }
#else   /* !_WIN32 */
    len = PR_snprintf(buf, (size_t)count, "3.1");
#endif  /* !_WIN32 */

    return len;
}


long _MD_GetArchitecture(char *buf, long count)
{
    long len = 0;

#ifdef _WIN32
    SYSTEM_INFO info;
    char *type = "";

    GetSystemInfo( &info );

    switch( info.wProcessorArchitecture ) {
      case PROCESSOR_ARCHITECTURE_INTEL:
        /*
        ** Contrary to the Win32 documentation the wProcessorLevel field of 
        ** the SYSTEM_INFO struct is NOT initialized in Windows 95.  Therefore,
        ** we must use the "obsolete" dwProcessorType field instead.
        */
        switch( info.dwProcessorType ) {
          case PROCESSOR_INTEL_386:     type = "80386"  ; break;
          case PROCESSOR_INTEL_486:     type = "80486"  ; break;
          case PROCESSOR_INTEL_PENTIUM: type = "Pentium"; break;
        }
        break;

      case PROCESSOR_ARCHITECTURE_MIPS:
        type = "MIPS";
        break;

      case PROCESSOR_ARCHITECTURE_ALPHA:
        type = "Alpha";
        break;

      case PROCESSOR_ARCHITECTURE_PPC:
        type = "PPC";
        break;
    }
    if( type ) {
        len = PR_snprintf(buf, count, type);
    }
#else   /* !_WIN32 */
    len = PR_snprintf(buf, (size_t)count, "Intel");
#endif  /* !_WIN32 */
    
    return len;
}

