#ifndef nspr_irix_defs_h___
#define nspr_irix_defs_h___

#define PR_LINKER_ARCH	"irix"

#define _MD_GC_VMBASE		0x40000000
#define _MD_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	65536L

#ifdef SW_THREADS 
#include <ucontext.h>

#define PR_GetSP(_t)	(_t)->context.uc_mcontext.gregs[CTX_SP]
#define PR_NUM_GCREGS	9
#define PR_CONTEXT_TYPE	ucontext_t

#define CONTEXT(_thread) (&(_thread)->context)

/*
** Initialize the thread context preparing it to execute "e(o,a)"
*/
#define _MD_INIT_CONTEXT(thread, e, o, a)			    \
{								    \
    getcontext(CONTEXT(thread));				    \
    CONTEXT(thread)->uc_stack.ss_sp = thread->stack->stackTop - 64; \
    makecontext(CONTEXT(thread), HopToad, 3, e, o, a);		    \
}

#define _MD_SWITCH_CONTEXT(_thread)	 \
    if (!getcontext(CONTEXT(_thread))) { \
	(_thread)->errcode = errno;	 \
	_PR_Schedule();			 \
    }

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread)   \
{				       \
    ucontext_t *uc = CONTEXT(_thread); \
    uc->uc_mcontext.gregs[CTX_V0] = 1; \
    uc->uc_mcontext.gregs[CTX_A3] = 0; \
    _pr_current_thread = _thread;      \
    PR_LOG(SCHED, warn, ("Scheduled")); \
    errno = (_thread)->errcode;	       \
    setcontext(uc);		       \
}

#endif /* SW_THREADS */

#undef  HAVE_STACK_GROWING_UP
#define	HAVE_LONG_LONG
#ifdef IRIX6_2
#define	HAVE_ALIGNED_DOUBLES
#else
#undef	HAVE_ALIGNED_DOUBLES
#endif /* IRIX6_2 */
#undef	HAVE_ALIGNED_LONGLONGS
#define	HAVE_WEAK_IO_SYMBOLS
#define	HAVE_WEAK_MALLOC_SYMBOLS
#define	HAVE_DLL
#define	USE_DLFCN

#endif /* nspr_irix_defs_h___ */
