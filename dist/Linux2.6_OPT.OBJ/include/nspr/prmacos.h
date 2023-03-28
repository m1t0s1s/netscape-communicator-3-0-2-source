#ifndef prmacos_h___
#define prmacos_h___

//
// This file contains all changes and additions which need to be made to the NSPR runtime 
// for the Macintosh platform (specifically the Metrowerks environment).  This file should 
// only be incluced in Macintosh builds.
//

#include "prmacros.h"
#include "prtypes.h"
#include "prsystem.h"

NSPR_BEGIN_EXTERN_C

#include <stddef.h>
#include <setjmp.h>

#if _NSPR
	#ifndef TRUE					// Mac standard is lower case true
		#define TRUE 1
	#endif
	#ifndef FALSE					// Mac standard is lower case true
		#define FALSE 0
	#endif
#endif

#define GCPTR
#define CALLBACK
typedef int (*FARPROC)();
#define gcmemcpy(a,b,c) memcpy(a,b,c)

#define PR_CONTEXT_TYPE 								\
struct {												\
	jmp_buf		 		registers;						\
}

#if defined(powerc) || defined(__powerc)
	#define PR_GetSP(_t) (*((uint32 *)((_t)->context.registers) + 2))
	#define INIT_STACKPTR(stackTop) ((unsigned char*)stackTop - 128)
	#define PR_NUM_GCREGS 70
#else
	#define PR_GetSP(_t) (*((uint32 *)((_t)->context.registers) + 12))
	#define INIT_STACKPTR(stackTop) ((unsigned char*)stackTop - 4)
	#define PR_NUM_GCREGS 13
#endif

#define PR_NO_PREEMPT	1
#define MAX_NON_PRIMARY_TIME_SLICES 	6

extern long gTimeSlicesOnNonPrimaryThread;
extern PRThread *gPrimaryThread;

#define _MD_INIT_CONTEXT(_thread, e, o, a)							\
{																	\
 	unsigned char *sp;												\
 	if(setjmp(_thread->context.registers)) HopToadNoArgs();   		\
    (_thread)->asyncCall = e;										\
    (_thread)->asyncArg0 = o;										\
    (_thread)->asyncArg1 = a;										\
    sp = INIT_STACKPTR((_thread)->stack->stackTop);					\
    (PR_GetSP(_thread)) = (long) sp;   								\
}

#define _MD_SWITCH_CONTEXT(_thread)	 				\
{													\
    if (!setjmp(_thread->context.registers)) { 															\
		_PR_Schedule();			 					\
    }												\
}


/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread)   	\
{				       					\
    _pr_current_thread = _thread;       \
    PR_LOG(SCHED, warn, ("Scheduled")); \
    longjmp(_thread->context.registers, 1);																\
}

PRThread *PR_GetPrimaryThread();

typedef short PROSFD;

// Errors not found in the Mac StdCLib
#define EACCES  		13      	// Permission denied
#define ENOENT			-43			// No such file or directory
//#define EAGAIN			10		// No more processes
#define EINTR			11			// interrupted system call
#define EBADF			-1			// Bad file number
//#define EWOULDBLOCK		12		// Operation would block
#define _OS_INVALID_FD_VALUE -1

#define	STDERR_FILENO	2

#define MAC_PATH_SEPARATOR 				':'
#define PATH_SEPARATOR 					':'
#define DIRECTORY_SEPARATOR				'/'
#define DIRECTORY_SEPARATOR_STR			"/"
#define UNIX_THIS_DIRECTORY_STR			"./"
#define UNIX_PARENT_DIRECTORY_STR		"../"

#define MAX_PATH 			512
#define MAX_MAC_FILENAME	31


// Alias a few names
#define putenv	PR_PutEnv
#define getenv	PR_GetEnv

typedef unsigned char (*MemoryCacheFlusherProc)(size_t size);
typedef void (*PreAllocationHookProc)(void);

extern char *strdup(const char *source);

extern void InstallPreAllocationHook(PreAllocationHookProc newHook);
extern void InstallMemoryCacheFlusher(MemoryCacheFlusherProc newFlusher);

extern char *PR_GetDLLSearchPath(void);

extern int strcmp(const char *str1, const char *str2);
extern int strcasecmp(const char *str1, const char *str2);

extern void MapFullToPartialMacFile(char *);
extern char *MapPartialToFullMacFile(const char *);

extern void ResetTimer(void);
extern void PR_PeriodicIdle(void);
extern void ActivateTimer(void);
extern void DeactivateTimer(void);
extern void PR_InitMemory(void);

#define gethostbyaddr		macsock_gethostbyaddr
#define getsockname 		macsock_getsockname
#define socket 				macsock_socket
#define listen				macsock_listen
#define accept				macsock_accept
#define bind				macsock_bind
#define connect				macsock_connect
#define send				macsock_send
#define sendto				macsock_sendto
#define socketavailable		macsock_socketavailable


extern struct hostent *gethostbyaddr(const void *addr, int addrlen, int type);

extern short GetVolumeRefNumFromName(const char *);


//
//	Macintosh only private parts.
//
#define	dprintTrace			";dprintf;doTrace"
#define	dprintNoTrace		";dprintf"
extern void dprintf(const char *format, ...);


// Entry into the memory system’s cache flushing
extern uint8 CallCacheFlushers(size_t blockSize);

enum {
	kPrivateNSPREventType = 13
};

extern void* reallocSmaller(void* block, size_t newSize);


NSPR_END_EXTERN_C

#endif /* prmacos_h___ */
