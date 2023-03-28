#ifndef nspr_win32_defs_h___
#define nspr_win32_defs_h___
#include <prtypes.h>

#define PR_LINKER_ARCH      "win32"

#define GC_VMBASE               0x40000000
#define GC_VMLIMIT              ((64*1024*1024)-1)
#define _MD_DEFAULT_STACK_SIZE  (2*65536L)

#ifdef SW_THREADS
/*
** If thread emulation is used, then setjmp/longjmp stores the register
** state of each thread.
*/
#define USE_SETJMP

#define PR_CONTEXT_TYPE     jmp_buf
#define PR_NUM_GCREGS       sizeof(jmp_buf)
#define PR_GetSP(_t)        (_t)->context[4]    /* Esp */
#define CONTEXT(_t)         ((_t)->context)

/*
** Initialize a thread context to run "e(o,a)" when started
** Note:
**    context[0] == Ebp
**    context[4] == Esp
**    context[5] == Eip
*/
#define _MD_INIT_CONTEXT(_thread, e, o, a)      \
{                                               \
     setjmp(CONTEXT(_thread));                  \
     (_thread)->asyncCall = e;                  \
     (_thread)->asyncArg0 = o;                  \
     (_thread)->asyncArg1 = a;                  \
     (_thread)->context[0] = 0;                 \
     (_thread)->context[4] = (unsigned char*)   \
    ((_thread)->stack->stackTop - 64);          \
     (_thread)->context[5] = HopToadNoArgs;     \
}

#define _MD_SWITCH_CONTEXT(_thread)             \
    if (!setjmp(CONTEXT(_thread))) {            \
        (_thread)->errcode = errno;             \
        _PR_Schedule();                         \
    }

#define _MD_SAVE_CONTEXT(_thread)               \
    setjmp(CONTEXT(_thread))

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread)            \
{                                               \
    _pr_current_thread = _thread;               \
    PR_LOG(SCHED, warn, ("Scheduled"));         \
    errno = (_thread)->errcode;                 \
    longjmp(CONTEXT(_thread), 1);               \
}

#else   /* ! SW_THREADS */
/*
** If OS threads are used then only store eax, ebx, ecx, edx, esi, edi and
** esp.  These are needed by the GC when searching the thread for possible
** roots.  _MD_GetRegisters() fills in the PR_CONTEXT_TYPE structure.
*/

#define PR_NUM_GCREGS       8
typedef prword_t            PR_CONTEXT_TYPE[PR_NUM_GCREGS];
PR_PUBLIC_API(unsigned long) PR_GetSP(PRThread *t);

#endif  /* ! SW_THREADS */

#define HAVE_DLL
#define HAVE_LONG_LONG

#endif /* nspr_win32_defs_h___ */
