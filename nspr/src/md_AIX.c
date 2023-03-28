#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include "prlog.h"
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

/* To implement select using poll */
#include <stropts.h>
#include <poll.h>
#include <sys/select.h>

#include "mdint.h"

void _MD_InitOS(int when)
{
    _MD_InitUnixOS(when);
}

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}

/************************************************************************/

int _OS_OPEN(const char *path, int oflag, mode_t mode)
{
    int rv = open(path, oflag, mode);
    return (rv < 0 ? -errno : rv);
}

int _OS_CLOSE(int fd)
{
    int rv = close(fd);
    return (rv < 0 ? -errno : rv);
}

int _OS_READ(int fd, void *buf, unsigned nbyte)
{
    int rv = read(fd, buf, nbyte);
    return (rv < 0 ? -errno : rv);
}

int _OS_WRITE(int fd, void *buf, unsigned nbyte)
{
    int rv = write(fd, buf, nbyte);
    return (rv < 0 ? -errno : rv);
}

int _OS_SOCKET(int domain, int type, int flags)
{
    int rv = socket(domain, type, flags);
    return (rv < 0 ? -errno : rv);
}

int _OS_CONNECT(int fd, void *addr, int addrlen)
{
    int rv = connect(fd, addr, addrlen);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_RECV(int s, char *buf, int len, int flags)
{
    int rv = recv(s, buf, len, flags);
    if(rv < 0) {
        return -errno;
    }
    return rv;
}

int _OS_SEND(int s, const char *msg, int len, int flags)
{
    int rv = send(s, msg, len, flags);
    if(rv < 0) {
        return -errno;
    }
    return rv;
}

int _OS_FCNTL(int filedes, int cmd, int arg)
{
    int rv = fcntl(filedes, cmd, arg);
    if(rv < 0)
        return -errno;
    return rv;
}
