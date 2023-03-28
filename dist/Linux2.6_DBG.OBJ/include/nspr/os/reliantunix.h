/*
 * sinix.h
 * 5/18/96
 * Taken from nec.h by Christian.Kaiser@mch.sni.de
 */
#ifndef nspr_sinix_defs_h___
#define nspr_sinix_defs_h___

#define PR_LINKER_ARCH	"sinix"

#define _MD_GC_VMBASE		0x09000000
#define _MD_STACK_VMBASE	0x0a000000
#define _MD_DEFAULT_STACK_SIZE	65536L

#ifdef SW_THREADS 

#include <ucontext.h>
 
#define PR_GetSP(_t)    (_t)->context.uc_mcontext.gpregs[CXT_SP]
#define PR_NUM_GCREGS   NGREG
#define PR_CONTEXT_TYPE ucontext_t
 
#define CONTEXT(_thread) (&(_thread)->context)
 
/*
** Initialize the thread context preparing it to execute "e(o,a)"
** - get a nice, fresh context
** - set its SP to the stack we allcoated for it
** - set it to start things at "e"
*/
#define _MD_INIT_CONTEXT(thread, e, o, a)                           \
{                                                                   \
    getcontext(CONTEXT(thread));                                    \
    CONTEXT(thread)->uc_stack.ss_sp = thread->stack->stackBottom;   \
    CONTEXT(thread)->uc_stack.ss_size = thread->stack->stackSize;   \
    CONTEXT(thread)->uc_mcontext.gpregs[CXT_SP] = thread->stack->stackTop - 64; \
    makecontext(CONTEXT(thread), HopToad, 3, e, o, a);              \
}
 
/*
** Save current context as it is scheduled away
*/
#define _MD_SWITCH_CONTEXT(_thread)      \
    if (!getcontext(CONTEXT(_thread))) { \
        (_thread)->errcode = errno;      \
        _PR_Schedule();                  \
    }
 
/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
**  CXT_V0 is the register that holds the return value.
**   We must set it to 1 so that we can see if the return from
**   getcontext() is the result of calling getcontext() or
**   setcontext()...
**   setting a context got with getcontext() appears to
**   return from getcontext(), too!
**  CXT_A3 is the register that holds the fourth arg.
**   Dunno why I should set this, so I don't (which is probably a mistake).
*/
#define _MD_RESTORE_CONTEXT(_thread)   \
{                                      \
    ucontext_t *uc = CONTEXT(_thread); \
    uc->uc_mcontext.gpregs[CXT_V0] = 1;\
 /* uc->uc_mcontext.gpregs[CXT_A3] = 0; */ \
    errno = (_thread)->errcode;        \
    _pr_current_thread = _thread;	\
    PR_LOG(SCHED, warn, ("Scheduled"));	\
    setcontext(uc);                    \
}

#endif /* SW_THREADS */

#undef  HAVE_STACK_GROWING_UP
#undef	HAVE_LONG_LONG
#undef	HAVE_ALIGNED_DOUBLES
#undef	HAVE_ALIGNED_LONGLONGS
#define	HAVE_WEAK_IO_SYMBOLS
#define	HAVE_WEAK_MALLOC_SYMBOLS
#define	HAVE_DLL
#define	USE_DLFCN

#define NEED_TIME_R

#if !defined(S_ISSOCK) && defined(S_IFSOCK)
#define S_ISSOCK(mode)   ((mode&0xF000) == 0xC000)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
#define S_ISLNK(mode)   ((mode&0xA000) == 0xC000)
#endif

#endif /* nspr_sinix_defs_h___ */
