#ifndef nspr_osf1_defs_h___
#define nspr_osf1_defs_h___

#define PR_LINKER_ARCH	"osf"

#define _MD_GC_VMBASE		0x40000000
#define _MD_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	131072L

#define USE_SETJMP

#ifdef SW_THREADS 
/* Why we can't have JB_SP is unclear, but we need it */
#undef _POSIX_SOURCE
#undef _XOPEN_SOURCE
#include <setjmp.h>
#define _POSIX_SOURCE
#define _XOPEN_SOURCE

#define PR_GetSP(_t) (_t)->context[JB_SP]
#define PR_NUM_GCREGS _JBLEN
#define PR_CONTEXT_TYPE jmp_buf

#define CONTEXT(_th) ((_th)->context)

/* XXX osf1 has setcontext/getcontext! */

/*
** Initialize a thread context to run "e(o,a)" when started
*/
#define _MD_INIT_CONTEXT(_thread, e, o, a)				  \
{									  \
    if (setjmp(CONTEXT(_thread))) {					  \
	HopToadNoArgs();						  \
    }									  \
    (_thread)->asyncCall = e;						  \
    (_thread)->asyncArg0 = o;						  \
    (_thread)->asyncArg1 = a;						  \
    (_thread)->context[JB_SP] = (long) ((_thread)->stack->stackTop - 64); \
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

#endif /* SW_THREADS */

#undef  HAVE_STACK_GROWING_UP
#define	HAVE_LONG_LONG
#define HAVE_ALIGNED_DOUBLES
#define HAVE_ALIGNED_LONGLONGS
#undef 	HAVE_WEAK_IO_SYMBOLS
#undef 	HAVE_WEAK_MALLOC_SYMBOLS
#define HAVE_DLL

#define NEED_TIME_R
#define USE_DLFCN

#endif /* nspr_osf1_defs_h___ */
