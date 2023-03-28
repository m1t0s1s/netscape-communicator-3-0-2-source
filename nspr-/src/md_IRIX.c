#include <signal.h>
#include <unistd.h>
#include "prlog.h"
#include "mdint.h"
#include "prdump.h"
#include "prprf.h"
#include "prgc.h"

/*
** Under IRIX, SIGALRM's cause connect to break when you are connecting
** non-blocking. We work around the bug by holding SIGALRM's during the
** connect system call.
*/
sigset_t _pr_holdALRM;

#ifdef DEBUG 
#include "gcint.h"

static void ShowStatus(void)
{
    (*PR_ShowStatusHook)();
    PR_LOG_FLUSH();
}

static void DumpGCHeap(void)
{
    static unsigned int n = 1;
    FILE *fp;
    char *name;

    /* Then dump what's left */
    name = PR_smprintf("GCHEAP-%d", n++);
    if (name) {
	fp = fopen(name, "w");
	free(name);
	if (fp) {
	    if (_pr_dump_file) {
		fclose(_pr_dump_file);
	    }
	    _pr_dump_file = fp;
	    _pr_do_a_dump = 1;
	}
    }
}
#endif /* DEBUG */

static void CatchSegv(void)
{
#ifdef DEBUG 
    ShowStatus();
#endif /* DEBUG */
    kill(getpid(), SIGBUS);
}

#ifdef DEBUG
static char altstack[65536];

static stack_t segv_altstack = {
    altstack + sizeof(altstack) - 64,
    sizeof(altstack) - 64,
    0,
};
#endif

void _MD_InitOS(int when)
{
    struct sigaction vtact;

    _MD_InitUnixOS(when);

    if (when == _MD_INIT_READY) {
	sigaddset(&_pr_holdALRM, SIGALRM);

#ifdef DEBUG
	if (!getenv("NSPR_NOSEGV")) {
	    /*
	    ** Catch SIGSEGV so that we can get a stack trace. Under IRIX
	    ** if you touch a random address the kernel often times
	    ** thinks you are trying to grow the stack and then kills the
	    ** process with no hope of a stack trace.
	    */
            if (sigaltstack(&segv_altstack, 0) < 0) {
                perror("sigaltstack");
            }
	    vtact.sa_handler = (void (*)()) CatchSegv;
	    vtact.sa_flags = SA_ONSTACK;
	    sigemptyset(&vtact.sa_mask);
	    sigaddset(&vtact.sa_mask, SIGALRM);
	    sigaction(SIGSEGV, &vtact, 0);
	}

	/* Cause SIGUSR1 (16) to dump out thread information: */
	vtact.sa_handler = (void (*)()) ShowStatus;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGUSR1, &vtact, 0);

	/* Cause SIGUSR2 (17) to dump out gc information: */
	vtact.sa_handler = (void (*)()) DumpGCHeap;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGUSR2, &vtact, 0);
#endif /* DEBUG */
    }
} 

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    extern void _MD_SaveSRegs(void*);

    if (isCurrent) {
	_MD_SaveSRegs(&t->context.uc_mcontext.gregs[0]);
	*np = 9;
    } else {
	*np = NGREG;
    }
    return (prword_t*) &t->context.uc_mcontext.gregs[0];
}

void abort(void)
{
    PR_LOG_FLUSH();
    kill(getpid(), SIGBUS);
}

/*
** IRIX specific malloc routines that need wrapping
*/
void *valloc(size_t size)
{
    return malloc(size);
}

void *memalign(size_t alignment, size_t size)
{
    /* NOT YET IMPLEMENTED */
    PR_Abort();
}

/************************************************************************/

#if XXX
/*
** XXX It is really hard to get this stuff right given that IRIX's libc
** links strongly against itself. This means that, for example, calling
** popen will get you to _pipe and _close and so on. Which means that
** we can't properly keep track of the state of a file descriptor. We
** could call fcntl on every i/o operation I suppose...
*/

/*
** Thread safe versions of some of the I/O routines
*/
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include "prfile.h"
#include "swkern.h"

extern int _read(int, void*, size_t);
extern int _recv(int, void*, size_t, int);
extern int _open(const char *, int, int);
extern int _close(int);

#define MAX_FD	256

static int nb[MAX_FD];

static int CheckFD(int fd)
{
    if ((fd < 0) || (fd >= MAX_FD)) {
	errno = EBADF;
        return -1;
    }
    if (!nb[fd] && (fd > 2)) {
	int flgs;

	nb[fd] = 1;
	flgs = fcntl(fd, F_GETFL, 0);
	if (flgs == -1) {
	    return -1;
	}
	if (flgs & O_NONBLOCK) {
	    nb[fd] = 2;
	} else {
	    fcntl(fd, F_SETFL, flgs | O_NONBLOCK);
	}
    }
    return 0;
}

ssize_t read(int fd, void *buf, size_t amount)
{
    int rv;

    if (!_pr_read_lock) {
	return _read(fd, buf, amount);
    }

    if (CheckFD(fd) < 0) {
	return -1;
    }

    rv = _read(fd, buf, amount);
    while ((rv < 0) && !_PR_PENDING_EXCEPTION() &&
           (errno == EAGAIN || errno == EINTR)) {
        if (errno == EAGAIN) {
            if (nb[fd] == 2) {
                return -1;
            }
	    _PR_WaitForFH(fd, 1);
        }
        rv = _read(fd, buf, amount);
    }
    return rv;
}

ssize_t recv(int fd, void *buf, size_t amount, int flags)
{
    int rv;

    if (!_pr_read_lock) {
	return _recv(fd, buf, amount, flags);
    }

    if (CheckFD(fd) < 0) {
	return -1;
    }

    rv = _recv(fd, buf, amount, flags);
    while ((rv < 0) && !_PR_PENDING_EXCEPTION() &&
           (errno == EAGAIN || errno == EINTR)) {
        if (errno == EAGAIN) {
            if (nb[fd] == 2) {
                return -1;
            }
	    _PR_WaitForFH(fd, 1);
        }
        rv = _recv(fd, buf, amount, flags);
    }
    return rv;
}

int open(const char *name, int flags, ...)
{
    int mode, rv;

    if (flags & O_CREAT) {
	va_list ap;

	va_start(ap, flags);
	mode = va_arg(ap, int);
	va_end(ap);
    } else {
	mode = 0;
    }
    rv = _open(name, flags, mode);
    PR_LOG(IO, warn,
	   ("open returns %d; name='%s' flags=0%o mode=0%o",
	    rv, name, flags, mode));
    return rv;
}

int close(int fd)
{
    int rv = _close(fd);

    PR_LOG(IO, warn, ("close returns %d; fd=%d", rv, fd));

    if ((fd >= 0) && (fd < MAX_FD)) {
	nb[fd] = 0;
    }
    return rv;
}
#endif
