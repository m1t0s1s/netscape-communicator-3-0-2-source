#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include <sys/syscall.h>
#ifdef _PROTOTYPES
#undef _PROTOTYPES
#include <sys/time.h>
#define _PROTOTYPES
#else
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <errno.h>
#include "mdint.h"
#include <setjmp.h>
#include <sys/socket.h>
#include <signal.h>
#include <math.h>
#include "prdump.h"
#include "prlog.h"
#include <values.h>

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}

#define PIDOOMA_STACK_SIZE 524288
#define BACKTRACE_SIZE 8192

#ifdef DEBUG
static void
ShowStatus(void)
{
    (*PR_ShowStatusHook)();
    PR_LOG_FLUSH();
}
#endif /* DEBUG */

static void
CatchSegv(void)
{
#ifdef DEBUG
    ShowStatus();
#endif /* DEBUG */
    kill(getpid(), SIGBUS);
}

static void
CatchFPE( int sig, int code, struct sigcontext *scp )
{
    /*
    ** On HP-UX we need to define a SIGFPE handler because coercion of a
    ** NaN to an int causes SIGFPE to be raised. Thanks to Marianne
    ** Mueller and Doug Priest at SunSoft for this fix.
    */
    unsigned i, e;
    int r, t;
    int *source, *destination;

#ifdef DIAGNOSTIC
    printf( "signal %d, code %d, address %x, next %x\n", sig, code,
	    scp->sc_pcoq_head, scp->sc_pcoq_tail );
    printf( "%08X %08X\n", scp->sc_sl.sl_ss.ss_frstat,
	    scp->sc_sl.sl_ss.ss_frexcp1 );
    printf( "%08X %08X\n", scp->sc_sl.sl_ss.ss_frexcp2,
	    scp->sc_sl.sl_ss.ss_frexcp3 );
    printf( "%08X %08X\n", scp->sc_sl.sl_ss.ss_frexcp4,
	    scp->sc_sl.sl_ss.ss_frexcp5 );
    printf( "%08X %08X\n", scp->sc_sl.sl_ss.ss_frexcp6,
	    scp->sc_sl.sl_ss.ss_frexcp7 );
#endif /* DIAGNOSTIC */

    /* check excepting instructions */
    for ( i = 0; i < 7; i++ )
    {
	e = *(i+&(scp->sc_sl.sl_ss.ss_frexcp1));
	if ( e & 0xfc000000 != 0 )
	{
	    if ((e & 0xf4017720) == 0x24010200)
            {
                r = ((e >> 20) & 0x3e);
                t = (e & 0x1f) << 1;
                if (e & 0x08000000)
                {
                    r |= (e >> 7) & 1;
                    t |= (e >> 6) & 1;
                }
                source = (int *)(&scp->sc_sl.sl_ss.ss_frstat + r);
                destination = (int *)(&scp->sc_sl.sl_ss.ss_frstat + t);
                *destination = *source < 0 ? -MAXINT-1 : MAXINT;
            }
	}
	*(i+&(scp->sc_sl.sl_ss.ss_frexcp1)) = 0;

#ifdef DIAGNOSTIC
	e &= 0x03ffffff; /* mask off exception flags */
	printf( "Bit pattern: %08x\n", e );
	switch ( e & 0x600 )
	{
	  case 0:
	    printf( "  class 0, sub-op %d\n", ( e >> 13 ) & 3 );
	    break;

	  case 0x200:
	    printf( "  class 1, sub-op %d\n", ( e >> 13 ) & 3 );
	    break;

	  case 0x400:
	    printf( "  class 2, sub-op %d\n", ( e >> 13 ) & 3 );
	    break;

	  default:
	    printf( "  class 3, sub-op %d\n", ( e >> 13 ) & 3 );
	    break;
	}
	*(i+&(scp->sc_sl.sl_ss.ss_frexcp1)) = 0;
#endif /* DIAGNOSTIC */
    }

    /* clear T-bit */
    scp->sc_sl.sl_ss.ss_frstat &= ~0x40;

    /* advance the IAQs */
/*
    scp->sc_pcoq_head = scp->sc_pcoq_tail;
    scp->sc_pcsq_head = scp->sc_pcsq_tail;
*/

    signal(SIGFPE,CatchFPE);
}

void _MD_InitOS(int when)
{
    jmp_buf jb;
    char *newstack;
    char *oldstack;

    _MD_InitUnixOS(when);

    if (when == _MD_INIT_READY) {
	/* Catch SIGSEGV so that we can flush the log */
	struct sigaction vtact;
	vtact.sa_handler = (void (*)()) CatchSegv;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGSEGV, &vtact, 0);

#ifdef DEBUG
	/* Cause SIGUSR1 (16) to dump out thread information: */
	vtact.sa_handler = (void (*)()) ShowStatus;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGUSR1, &vtact, 0);
#endif /* DEBUG */

	/*
	** On HP-UX we need to define a SIGFPE handler because coercion
	** of a NaN to an int causes SIGFPE to be raised. Thanks to
	** Marianne Mueller and Doug Priest at SunSoft for this fix.
	*/
	{
	    struct sigvec v;

	    v.sv_handler = CatchFPE;
	    v.sv_mask = 0;
	    v.sv_flags = 0;
	    sigvector( SIGFPE, &v, NULL );
	    fpsetmask( FP_X_INV );
	}
    }

    if(!setjmp(jb)) {
        /* 
           XXXrobm Horrific. To avoid a problem with HP's system call
           code, we allocate a new stack for the primordial thread and
           use it. However, we don't know how far back the original stack
           goes. We should create a routine that performs a backtrace and
           finds out just how much we need to copy. As a temporary measure,
           I just copy an arbitrary guess.
         */
        newstack = (char *) malloc(PIDOOMA_STACK_SIZE);
        oldstack = (char *) (*(((int *) jb) + 1) - BACKTRACE_SIZE);
        memcpy(newstack, oldstack, BACKTRACE_SIZE);
        *(((int *) jb) + 1) = (int) (newstack + BACKTRACE_SIZE);
        longjmp(jb, 1);
    }
}

void ClockInterruptHandler(int sig, int code, struct sigcontext *scp)
{
    _PR_ClockInterruptHandler();
    scp->sc_syscall_action = SIG_RESTART;
}

void ChildDeathInterruptHandler(int sig, int code, struct sigcontext *scp)
{
    _PR_ChildDeathInterruptHandler();
    scp->sc_syscall_action = SIG_RESTART;
}

void AsyncIOInterruptHandler(int sig, int code, struct sigcontext *scp)
{
    _PR_AsyncIOInterruptHandler();
    scp->sc_syscall_action = SIG_RESTART;
}

void PR_StartEvents(int usec)
{
    struct sigvec vec;
    struct itimerval itval;

    if (getenv("NSPR_NOCLOCK")) {
	_pr_no_clock = 1;
	return;
    }
    if (usec == 0) {
	usec = 50000;
    }

    vec.sv_handler = (void (*)()) ClockInterruptHandler;
    vec.sv_mask = ~0;
    vec.sv_flags = 0;
    sigvector(SIGALRM, &vec, 0);

    itval.it_interval.tv_sec = usec / 1000000;
    itval.it_interval.tv_usec = usec % 1000000;
    itval.it_value = itval.it_interval;
    if (setitimer(ITIMER_REAL, &itval, 0) < 0) {
        extern int errno;
        fprintf(stderr, "nspr: can't set interval timer: %d\n", errno);
	_pr_no_clock = 1;
    }
}

void PR_InitInterrupts(void)
{
    struct sigvec vec;

    /* Setup death of a child handler */
    vec.sv_handler = (void (*)()) ChildDeathInterruptHandler;
    vec.sv_mask = ~0;
    vec.sv_flags = 0;
    sigvector(SIGCLD, &vec, 0);

    /* Setup async IO handler */
    vec.sv_handler = (void (*)()) AsyncIOInterruptHandler;
    vec.sv_mask = ~0;
    vec.sv_flags = 0;
    sigvector(SIGIO, &vec, 0);
}

/*
 * The HP version of strchr is buggy. It looks past the end of the string and
 * causes a segmentation fault when our (NSPR) version of malloc is used.
 *
 * A better solution might be to put a cushion in our malloc just in case HP's
 * version of strchr somehow gets used instead of this one.
 */
char *
strchr(const char *s, int c)
{
	char	ch;

	if (!s)
	{
		return NULL;
	}

	ch = (char) c;

	while ((*s) && ((*s) != ch))
	{
		s++;
	}

	if ((*s) == ch)
	{
		return (char *) s;
	}

	return NULL;
}

/******************************************************************************/
/* Define the libc syscalls in terms of our _OS_ routines: */
#if 0
#define WRAP_SYSCALL(Ret, name, Arglist, arglist)  \
	Ret name Arglist			   \
	{					   \
	    int rv = XXX##name arglist;		   \
	    if (rv < -1) {			   \
		errno = -rv;			   \
		return -1;			   \
	    }					   \
	    return rv;				   \
	}					   \

void exit(int status)
{
    int rv = XXXexit(status);
}

WRAP_SYSCALL(int, creat, (const char *path, mode_t mode), (path, mode))

WRAP_SYSCALL(int, unlink, (const char *path), (path))

WRAP_SYSCALL(int, fchmod, (int fildes, mode_t mode), (fildes, mode))

WRAP_SYSCALL(int, chdir, (const char *path), (path))

WRAP_SYSCALL(int, chmod, (const char *path, mode_t mode), (path, mode))

WRAP_SYSCALL(int, chown, (const char *path, uid_t owner, gid_t group), (path, owner, group))

WRAP_SYSCALL(int, recvmsg, (int s, struct msghdr msg[], int flags), (s, msg, flags))

WRAP_SYSCALL(int, sendmsg, (int s, const struct msghdr msg[], int flags), (s, msg, flags))

WRAP_SYSCALL(int, shutdown, (int s, int how), (s, how))

WRAP_SYSCALL(int, rename, (const char* source, const char* target), (source, target))

WRAP_SYSCALL(int, link, (const char* path1, const char* path2), (path1, path2))

WRAP_SYSCALL(int, stat, (const char* path, struct stat* buf), (path, buf))

WRAP_SYSCALL(int, fchown, (int fildes, uid_t owner, gid_t group), (fildes, owner, group))

WRAP_SYSCALL(ssize_t, read, (int fildes, void* buf, size_t nbyte), (fildes, buf, nbyte))

WRAP_SYSCALL(ssize_t, write, (int fildes, const void *buf, size_t nbyte), (fildes, buf, nbyte))

WRAP_SYSCALL(int, open, (const char *path, int oflag, mode_t mode), (path, oflag, mode))

WRAP_SYSCALL(int, close, (int fildes), (fildes))

WRAP_SYSCALL(off_t, lseek, (int fildes, off_t offset, int whence), (fildes, offset, whence))

WRAP_SYSCALL(int, fcntl, (int fildes, int cmd, int value), (fildes, cmd, value))

WRAP_SYSCALL(int, dup2, (int fildes, int fildes2), (fildes, fildes2))

WRAP_SYSCALL(ssize_t, readv, (int fildes, const struct iovec* iov, size_t iovcnt), (fildes, iov, iovcnt))

WRAP_SYSCALL(ssize_t, writev, (int fildes, const struct iovec* iov, size_t iovcnt), (fildes, iov, iovcnt))

WRAP_SYSCALL(int, fstat, (int fildes, struct stat* buf), (fildes, buf))

WRAP_SYSCALL(int, pipe, (int fildes[2]), (fildes))

WRAP_SYSCALL(int, dup, (int fildes), (fildes))

WRAP_SYSCALL(pid_t, fork, (void), ())

WRAP_SYSCALL(int, execve, (const char *file, char *const argv[], char *const envp[]), (file, argv, envp))

WRAP_SYSCALL(pid_t, wait3, (int *stat_loc, int options, int *reserved), (stat_loc, options, reserved))

WRAP_SYSCALL(pid_t, waitpid, (pid_t pid, int *stat_loc, int options), (pid, stat_loc, options))

WRAP_SYSCALL(int, _MD_select, (size_t nfds,
           int *readfds,
           int *writefds,
           int *exceptfds,
           const struct timeval *timeout), (nfds, readfds, writefds, exceptfds, timeout))

WRAP_SYSCALL(int, getdirentries, (int fildes,
           struct direct *buf,
           size_t nbytes,
           off_t *basep), (fildes, buf, nbytes, basep))

WRAP_SYSCALL(int, accept, (int s, void *addr, int *addrlen), (s, addr, addrlen))

WRAP_SYSCALL(int, bind, (int s, const void *addr, int addrlen), (s, addr, addrlen))

WRAP_SYSCALL(int, getpeername, (int s, void *addr, int *addrlen), (s, addr, addrlen))

WRAP_SYSCALL(int, getsockopt, (int s,
           int level,
           int optname,
           void *optval,
           int *optlen), (s, level, optname, optval, optlen))

WRAP_SYSCALL(int, listen, (int s, int backlog), (s, backlog))

WRAP_SYSCALL(int, socket, (int af, int type, int protocol), (af, type, protocol))

WRAP_SYSCALL(int, send, (int s, const void *msg, int len, int flags), (s, msg, len, flags))

WRAP_SYSCALL(int, sendto, (int s,
           const void *msg,
           int len,
           int flags,
           const void *to,
           int tolen), (s, msg, len, flags, to, tolen))

WRAP_SYSCALL(int, recv, (int s, void *buf, int len, int flags), (s, buf, len, flags))

WRAP_SYSCALL(int, recvfrom, (int s,
           void *buf,
           int len,
           int flags,
           void *from,
           int *fromlen), (s, buf, len, flags, from, fromlen))

WRAP_SYSCALL(int, setsockopt, (int s,
           int level,
           int optname,
           const void *optval,
           int optlen), (s, level, optname, optval, optlen))
/*
WRAP_SYSCALL(int, ioctl, (int fildes, int request, int arg), (fildes, request, arg))
*/
WRAP_SYSCALL(int, sigprocmask, (int how,
           const sigset_t *set,
           sigset_t *oset), (how, set, oset))
#endif
/******************************************************************************/
