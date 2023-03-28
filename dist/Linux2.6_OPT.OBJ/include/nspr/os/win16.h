#ifndef nspr_win16_defs_h___
#define nspr_win16_defs_h___
#include <prtypes.h>

#define PR_LINKER_ARCH      "win16"

#define _MD_DEFAULT_STACK_SIZE  32767L

/*
** If thread emulation is used, then setjmp/longjmp stores the register
** state of each thread.
**
** CatchBuf layout:
**  context[0] - IP
**  context[1] - CS
**  context[2] - SP
**  context[3] - BP
**  context[4] - SI
**  context[5] - DI
**  context[6] - DS
**  context[7] - ?? (maybe flags)
**  context[8] - SS
*/
#define PR_CONTEXT_TYPE     CATCHBUF
#define PR_NUM_GCREGS       9

extern void
PRWin16ScanCStack(PRThread *, prword_t *, prword_t*);

#define PR_GetSP(_t)        ((_t)->stack->SP)

#define CONTEXT(_t)         ((_t)->context)

/*
** This is the default size of a thread stack... This size is used whenever
** PR_AttachThread(...) is called.
*/
#define _MD_THREAD_STACK_SIZE   30000


/*
** Initialize a thread context to run "e(o,a)" when started
*/
#define _MD_INIT_CONTEXT(_t, e, o, a)           \
{                                               \
     Catch(CONTEXT(_t));                        \
     (_t)->asyncCall  = e;                      \
     (_t)->asyncArg0  = o;                      \
     (_t)->asyncArg1  = a;                      \
     (_t)->context[3] = 0;                      \
     (_t)->context[2] =                         \
        OFFSETOF(_pr_top_of_task_stack - 64);   \
     (_t)->context[1] =                         \
        SELECTOROF(HopToadNoArgs);              \
     (_t)->context[0] =                         \
        OFFSETOF(HopToadNoArgs);                \
}

#define _MD_SWITCH_CONTEXT(_t)                  \
    if (!Catch(CONTEXT(_t))) {                  \
        (_t)->errcode = errno;                  \
        _PR_Schedule();                         \
    }

#define _MD_SAVE_CONTEXT(_t)                    \
    Catch(CONTEXT((_t)))

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
extern void _MD_RESTORE_CONTEXT(PRThread *t);

/*
** Win16 does not use a preemptive scheduling scheme...
*/
#define PR_NO_PREEMPT

#define USE_SETJMP
#define HAVE_DLL
#undef HAVE_LONG_LONG

#endif /* nspr_win16_defs_h___ */
