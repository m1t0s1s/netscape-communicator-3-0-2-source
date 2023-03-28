#ifndef nspr_xhppa_defs_h___
#define nspr_xhppa_defs_h___

#define PR_LINKER_ARCH	"hpux"

#define _MD_GC_VMBASE		0x40000000
#define _MD_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	65536L

#ifdef SW_THREADS 
#define USE_SETJMP

#include <setjmp.h>
#define PR_GetSP(_t) (*((int *)((_t)->context) + 1))
#define PR_NUM_GCREGS _JBLEN
/* Caveat: This makes jmp_buf full of doubles. */
#define PR_CONTEXT_TYPE jmp_buf

#define CONTEXT(_th) ((_th)->context)

#define _MD_INIT_CONTEXT(_thread, e, o, a)			       \
{								       \
    if(setjmp(CONTEXT(_thread))) HopToadNoArgs();		       \
    (_thread)->asyncCall = e;					       \
    (_thread)->asyncArg0 = o;					       \
    (_thread)->asyncArg1 = a;					       \
    /* Stack needs two frames (64 bytes) at the bottom */	       \
    (PR_GetSP(_thread)) = (int) ((_thread)->stack->stackTop + (64*2)); \
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
#endif

#define NEED_TIME_R

#define HAVE_STACK_GROWING_UP
#undef 	HAVE_LONG_LONG
#define	HAVE_ALIGNED_DOUBLES
#undef 	HAVE_ALIGNED_LONGLONGS
#undef	HAVE_WEAK_IO_SYMBOLS
#undef	HAVE_WEAK_MALLOC_SYMBOLS
#define	HAVE_DLL
#define USE_HPSHL
#endif /* nspr_xhppa_defs_h___ */
