#ifndef mdint_h___
#define mdint_h___

#ifdef XP_MAC
#include "prtypes.h"
#endif

/*
** Save the registers that the GC would find interesting into the thread
** "t". isCurrent will be non-zero if the thread state that is being
** saved is the currently executing thread. Return the address of the
** first register to be scanned as well as the number of registers to
** scan in "np".
**
** If "isCurrent" is non-zero then it is allowed for the thread context
** area to be used as scratch storage to hold just the registers
** necessary for scanning.
*/
extern prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np);

#ifdef HW_THREADS
/* Initialize the stack for a thread */
extern void _MD_InitializeStack(PRThreadStack *ts);
#endif

/* Init machine dependent parts of the OS */
extern void _MD_InitOS(int when);
#define _MD_INIT_AT_START	0
#define _MD_INIT_READY		1

/*
** Grow the GC Heap.
*/
extern void *_MD_GrowGCHeap(uint32 *sizep);

/*
** Free a GC segment.
*/
extern void _MD_FreeGCSegment(void *base, int32 len);

/*
** Create a new thread stack
*/
extern int _MD_NewStack(PRThread *thread, size_t stackSize);

/*
** Free a thread stack. The current thread is guaranteed to not be using
** it.
*/
extern void _MD_FreeStack(PRThread *thread);

/*
** Start the kernel clock ticking
*/
extern void _MD_StartClock(void);

/*
** Global variable, which when non-zero, means run without the kernel
** clock.
*/
extern int _pr_no_clock;

#ifdef XP_PC
/* Message ID sent for NSPR event notifications */
extern uint32  _pr_PostEventMsgId;
#endif  /* XP_PC */
/*
** APIs to return system information...
*/

#ifdef XP_UNIX
/* unixs which support sysinfo() */
#if defined(IRIX) || defined(SOLARIS) || defined(SNI)
#include <sys/systeminfo.h>

#define _MD_GetOSName(__buf, __len)       sysinfo(SI_SYSNAME, (__buf), (__len))
#define _MD_GetOSVersion(__buf, __len)    sysinfo(SI_RELEASE, (__buf), (__len))
#define _MD_GetArchitecture(__buf, __len) sysinfo(SI_ARCHITECTURE, (__buf), (__len))
#else
extern long _MD_GetOSName(char *buf, long count);
extern long _MD_GetOSVersion(char *buf, long count);
extern long _MD_GetArchitecture(char *buf, long count);
#endif
#endif

#ifdef XP_PC
extern long _MD_GetOSName(char *buf, long count);
extern long _MD_GetOSVersion(char *buf, long count);
extern long _MD_GetArchitecture(char *buf, long count);
#endif

#ifdef XP_MAC
extern long _MD_GetOSName(char *buf, long count);
extern long _MD_GetOSVersion(char *buf, long count);
extern long _MD_GetArchitecture(char *buf, long count);
#endif

/* Shutdown routines for various NSPR sub-systems */
extern void _PR_ShutdownLinker(void);


/* XXX where should this stuff go? */
#ifdef XP_UNIX
#include "prclist.h"
#include "prlong.h"

extern void _MD_InitUnixOS(int when);

/* I/O event mechanisms */

typedef struct PRPollDesc PRPollDesc;
typedef struct PRPollQueue PRPollQueue;

struct PRPollDesc {
    PRCList links;		/* for making lists of polldesc's */
    int fd;
    int in_flags;		/* value given to pr_io_wait */
    int out_flags;		/* return value from pr_io_wait */
};

/* in_flags/out_flags */
#define PR_POLL_READ		0x01
#define PR_POLL_WRITE		0x02
#define PR_POLL_EXCEPTION	0x04
#define PR_POLL_ERROR		0x08
#define PR_POLL_TIMEOUT		0x10

struct PRPollQueue {
    PRCList links;		/* for making lists of pollqueue's */
    PRCList queue;		/* list of polldesc's */
    int64 timeout;		/* how long to wait for i/o to complete */
    int on_ioq;			/* non-zero if on ioq list */
#if defined(XP_UNIX)
    PRMonitor *mon;             /* monitor to notify */
#endif
};

extern int _MD_IOWait(PRPollQueue *q);
extern int _PR_WaitForFH(int fd, int reading);

#define POLL_QUEUE_PTR(_qp) \
    ((PRPollQueue*) ((char*) (_qp) - offsetof(PRPollQueue,links)))

#define POLL_DESC_PTR(_qp) \
    ((PRPollDesc*) ((char*) (_qp) - offsetof(PRPollDesc,links)))

extern PRMonitor *_pr_io_lock;
extern PRMonitor *_pr_read_lock;
extern PRMonitor *_pr_write_lock;
extern PRMonitor *_pr_select_lock;

#endif /* XP_UNIX */

#endif /* mdint_h___ */
