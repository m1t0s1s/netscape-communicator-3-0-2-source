#include "prtypes.h"
#include "prlog.h"
#include "prosdep.h"
#include "prthread.h"
#include <sys/time.h>
#include <sys/types.h>
#include <setjmp.h>
#include <errno.h>
#include <syscall.h>
#include <linux/net.h>
#include "mdint.h"
#include <signal.h>

static void CatchSegv(void)
{
    PR_LOG_FLUSH();
    kill(getpid(), SIGBUS);
}

void _MD_InitOS(int when)
{
    struct sigaction vtact;

    _MD_InitUnixOS(when);

    if (when == _MD_INIT_READY) {
	vtact.sa_handler = (void (*)()) CatchSegv;
	vtact.sa_flags = 0;
	sigemptyset(&vtact.sa_mask);
	sigaddset(&vtact.sa_mask, SIGALRM);
	sigaction(SIGSEGV, &vtact, 0);

	/* and ignore FPE */
	sigemptyset(&vtact.sa_mask);
	vtact.sa_handler = SIG_IGN;
	sigaction(SIGFPE, &vtact, 0);
    }
} 

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) sigsetjmp(CONTEXT(t),1);
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}

/************************************************************************/

/*
** XXX these don't really work right because they use errno which can be
** XXX raced against
*/

int _OS_READ(int fd, void *buf, size_t len)
{
    int rv;
    rv = syscall(SYS_read, fd, buf, len);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_WRITE(int fd, const void *buf, size_t len)
{
    int rv;
    rv = syscall(SYS_write, fd, buf, len);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_OPEN(const char *path, int oflag, int mode)
{
    int rv;
    rv = syscall(SYS_open, path, oflag, mode);
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_CLOSE(int fd)
{
    int rv;
    rv = syscall(SYS_close, fd);
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_FCNTL(int fd, int flag, int value)
{
    int rv;
    rv = syscall(SYS_fcntl, fd, flag, value);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_IOCTL(int fd, int tag, int arg)
{
    int rv;
    rv = syscall(SYS_ioctl, fd, tag, arg);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

/* XXX Stubs until we get to linux 1.3 */

#ifndef LINUX1_2
void * dlopen(const char *filename, int flag)
{
    return 0;
}

const char *dlerror(void)
{
    return "dlerror doesn't work";
}

void *dlsym(void *h, char *sym)
{
    return 0;
}
#endif /* LINUX1_2 */
