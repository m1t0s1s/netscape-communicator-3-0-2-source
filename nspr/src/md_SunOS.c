#include "prtypes.h"
#include "prlog.h"
#include "prosdep.h"
#include "prthread.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/filio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "mdint.h"
#include "prdump.h"

/*
** XXX NOTE: This code is not properly implmeneted because of errno races
** (however, it will probably work because of the way the nspr kernel
** scheduler and clock code handle errno) . We really need the assembly
** stubs in os_SunOS.s, but we don't know how to write them...
*/

#ifdef SOLARIS
#include <synch.h>
extern int _read(int, void*, size_t);
extern int _write(int, const void*, size_t);
extern int _open(const char *, int, int);
extern int _close(int);
extern int _fcntl(int, int, int);
extern int _ioctl(int, int, int);
extern int _send(int, const void*, size_t, int);
extern int _recv(int, void*, size_t, int);
extern int _select (int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern int _socket(int, int, int);
extern int _connect(int, void*, int);

PRMonitor *_pr_sbrk_lock;
#endif

extern int syscall(int, ...);

#ifdef DEBUG
static void ShowStatus(void)
{
    (*PR_ShowStatusHook)();
    PR_LOG_FLUSH();
}
#endif /* DEBUG */

static void CatchSegv(void)
{
#ifdef DEBUG
    ShowStatus();
#endif /* DEBUG */
    kill(getpid(), SIGBUS);
}

void _MD_InitOS(int when)
{
    struct sigaction vtact;

    _MD_InitUnixOS(when);

    if (when == _MD_INIT_READY) {
	/* Catch SIGSEGV so that we can flush the log */
	vtact.sa_handler = (void (*)()) CatchSegv;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGSEGV, &vtact, 0);
#if defined(SOLARIS) && defined(SAFE_LIBC)
	_pr_sbrk_lock = PR_NewNamedMonitor(0, "sun-sbrk-lock");
#endif

#ifdef DEBUG
	/* Cause SIGUSR1 (16) to dump out thread information: */
	vtact.sa_handler = (void (*)()) ShowStatus;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGUSR1, &vtact, 0);
#endif /* DEBUG */
    }
} 

void abort(void)
{
    PR_LOG_FLUSH();
    kill(getpid(), SIGBUS);
#if 0
    /* Don't do this!  Forcing your debugger to continue from an abort
       is a perfectly reasonable thing to do. -- jwz. */
    PR_NOT_REACHED("Continuing after abort.");
#endif
}

#ifdef USE_SETJMP
prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}
#else
prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) getcontext(CONTEXT(t));	/* XXX tune me: set md_IRIX.c */
    }
    *np = NGREG;
    return (prword_t*) &t->context.uc_mcontext.gregs[0];
}
#endif

/************************************************************************/

/*
** XXX these don't really work right because they use errno which can be
** XXX raced against
*/

int _OS_READ(int fd, void *buf, size_t len)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_read, fd, buf, len);
#else
    rv = _read(fd, buf, len);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_WRITE(int fd, const void *buf, size_t len)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_write, fd, buf, len);
#else
    rv = _write(fd, buf, len);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_OPEN(const char *path, int oflag, int mode)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_open, path, oflag, mode);
#else
    rv = _open(path, oflag, mode);
#endif
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_CLOSE(int fd)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_close, fd);
#else
    rv = _close(fd);
#endif
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_FCNTL(int fd, int flag, int value)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_fcntl, fd, flag, value);
#else
    rv = _fcntl(fd, flag, value);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_IOCTL(int fd, int tag, int arg)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_ioctl, fd, tag, arg);
#else
    rv = _ioctl(fd, tag, arg);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SEND(int fd, const void *buf, int len, int flags)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_send, fd, buf, len, flags);
#else
    rv = _send(fd, buf, len, flags);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_RECV(int fd, void *buf, int len, int flags)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_recv, fd, buf, len, flags);
#else
    rv = _recv(fd, buf, len, flags);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SELECT(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_select, fd, r, w, e, t);
#else
    rv = _select(fd, r, w, e, t);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SOCKET(int domain, int type, int protocol)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_socket, domain, type, protocol);
#else
    rv = _socket(domain, type, protocol);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_CONNECT(int fd, void *addr, int addrlen)
{
    int rv;
#ifdef SUNOS4
    rv = syscall(SYS_connect, fd, addr, addrlen);
#else
    rv = _connect(fd, addr, addrlen);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

/************************************************************************/

#ifdef SOLARIS
#include <fcntl.h>
#include <stropts.h>
#include <stdarg.h>
#include "prfile.h"
#include "swkern.h"

/* XXX see the comment in md_IRIX.c */
#ifdef XXX
static int nb[128];

static int CheckFD(int fd)
{
    if ((fd < 0) || (fd >= 128)) {
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
	    PR_LOG(IO, debug,
		   ("read blocking, fd=%d buf=%x amount=%d",
		    fd, buf, amount));
	    _PR_WaitForFH(fd, 1);
        }
        rv = _read(fd, buf, amount);
    }
    return rv;
}

ssize_t recv(int fd, void *buf, size_t amount, int flags)
{
    int rv;

    if (flags == 0) {
	/*
	** Avoid bugs in Solaris5.3 in recv... Can't use read if flags is
	** non-zero though!
	*/
	return read(fd, buf, amount);
    }

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
	    PR_LOG(IO, debug,
		   ("recv blocking, fd=%d buf=%x amount=%d flags=%d",
		    fd, buf, amount, flags));
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

int socket(int domain, int type, int proto)
{
    int rv = _socket(domain, type, proto);
    PR_LOG(IO, warn,
	   ("socket returns %d; domain=%d type=%d proto=%d",
	    rv, domain, type, proto));
    return rv;
}

int close(int fd)
{
    int rv = _close(fd);
    PR_LOG(IO, warn, ("close returns %d; fd=%d", rv, fd));
    return rv;
}
#endif /* XXX */

#ifdef SW_THREADS
#include "swkern.h"
#endif

/*
** Layer solaris library locking on top of the nspr monitor cache, but
** only until we have the monitor cache initialized. Once it's
** initialized start using it.
**
 * Logic taken from solaris/java/green_threads/src/synch.c
*/
extern int _pr_mcache_ready;

int _mutex_lock(mutex_t *mp)
{
#ifdef SW_THREADS
    /*
    ** Assert that interrupts are not disabled (== scheduling locking).
    ** This is to ensure that we don't call libc from the scheduling code
    ** because if we did we could easily have a deadly embrace.
    */
    PR_ASSERT(_pr_intsOff == 0);
#endif

    if (_pr_mcache_ready) {
/*XX	PR_LOG(IO, debug, ("mutex lock 0x%x", mp)); */
	PR_CEnterMonitor(mp);
    }
    return 0;
}

int _mutex_unlock(mutex_t *mp)
{
#ifdef SW_THREADS
    /*
    ** Assert that interrupts are not disabled (== scheduling locking).
    ** This is to ensure that we don't call libc from the scheduling code
    ** because if we did we could easily have a deadly embrace.
    */
    PR_ASSERT(_pr_intsOff == 0);
#endif

    if (_pr_mcache_ready) {
/*XXX	PR_LOG(IO, debug, ("mutex unlock 0x%x", mp)); */
	PR_CExitMonitor(mp);
    }
    return 0;
}

/*
 * SPECIAL CASE: In Solaris, malloc() calls sbrk() for more memory,
 * and both call _mutex_lock(), which we override to make them Green
 * thread-safe.  But either _mutex_lock() call can thus cause monitor
 * cache expansion, which can lead to reentrant malloc calls (we try
 * to expand when entering sbrk() called from malloc()).  malloc()
 * isn't reentrant, so this causes trouble.  Instead, we special-case
 * sbrk() by wrapping it up in a non-cache monitor. 
 *
 * Code (and comment) taken from solaris/java/green_threads/src/synch.c
 */ 
void *_sbrk(int increment)
{
    void *ret;
    extern void *_sbrk_unlocked();

    if (_pr_sbrk_lock) {
	PR_EnterMonitor(_pr_sbrk_lock);
        ret = _sbrk_unlocked(increment);
	PR_ExitMonitor(_pr_sbrk_lock);
    } else {
	ret = _sbrk_unlocked(increment);
    }

    return ret;
}

#endif /* SOLARIS */
