#ifndef nspr_sunos_defs_h___
#define nspr_sunos_defs_h___

#define PR_LINKER_ARCH	"sunos"

/*
** For sunos type machines, don't specify an address because the
** NetBSD/SPARC O.S. does the wrong thing.
*/
#define _MD_GC_VMBASE		0
#define _MD_STACK_VMBASE	0
#define _MD_DEFAULT_STACK_SIZE	65536L

#define USE_SETJMP

#ifdef SW_THREADS
#include <setjmp.h>

#define PR_GetSP(_t)    (_t)->context[2]

#define PR_NUM_GCREGS	_JBLEN
#define PR_CONTEXT_TYPE	jmp_buf

#define CONTEXT(_th) ((_th)->context)

#define _MD_INIT_CONTEXT(_thread, e, o, a)			      \
{								      \
    if (setjmp(CONTEXT(_thread))) {				      \
	HopToadNoArgs();					      \
    }								      \
    (_thread)->asyncCall = e;					      \
    (_thread)->asyncArg0 = o;					      \
    (_thread)->asyncArg1 = a;					      \
    (_thread)->context[2] = (long) ((_thread)->stack->stackTop - 64); \
}

#define _MD_SWITCH_CONTEXT(_thread)  \
    if (!setjmp(CONTEXT(_thread))) { \
	(_thread)->errcode = errno;  \
	_PR_Schedule();		     \
    }

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread) \
{				     \
    _pr_current_thread = _thread;    \
    PR_LOG(SCHED, warn, ("Scheduled")); \
    errno = (_thread)->errcode;	     \
    longjmp(CONTEXT(_thread), 1);    \
}

#pragma unknown_control_flow(longjmp)
#pragma unknown_control_flow(setjmp)
#pragma unknown_control_flow(_PR_Schedule)

#endif /* SW_THREADS */

#undef	HAVE_LONG_LONG
#define HAVE_ALIGNED_DOUBLES
#define HAVE_ALIGNED_LONGLONGS
#define HAVE_DLL

#define NEED_TIME_R
#define USE_DLFCN

/*
** Missing function prototypes
*/

extern int socket (int domain, int type, int protocol);
extern int getsockname (int s, struct sockaddr *name, int *namelen);
extern int accept (int s, struct sockaddr *addr, int *addrlen);
extern int listen (int s, int backlog);
extern int brk(void *);
extern void *sbrk(int);

#endif /* nspr_sparc_defs_h___ */
