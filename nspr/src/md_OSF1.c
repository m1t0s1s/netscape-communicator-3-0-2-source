#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "mdint.h"

/* Names of the weak routines */
extern int __read(int, void*, size_t);
extern int __write(int, const void*, size_t);
extern int __open(const char *, int, int);
extern int __close(int);
extern int __fcntl(int, int, int);
extern int __ioctl(int, int, int);
extern int __send(int, const void*, size_t, int);
extern int __recv(int, void*, size_t, int);
extern int __select (int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern int __socket(int, int, int);
extern int __connect(int, void*, int);

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

/*
** XXX NOTE: This code is not properly implmeneted because of errno races
** (however, it will probably work because of the way the nspr kernel
** scheduler and clock code handle errno) . We really need the assembly
** stubs in os_OSF1.s, but we don't know how to write them...
*/

int _OS_READ(int fd, void *buf, size_t len)
{
    int rv;
    rv = __read(fd, buf, len);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_WRITE(int fd, const void *buf, size_t len)
{
    int rv;
    rv = __write(fd, buf, len);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_OPEN(const char *path, int oflag, int mode)
{
    int rv;
    rv = __open(path, oflag, mode);
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_CLOSE(int fd)
{
    int rv;
    rv = __close(fd);
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_FCNTL(int fd, int flag, int value)
{
    int rv;
    rv = __fcntl(fd, flag, value);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_IOCTL(int fd, int tag, int arg)
{
    int rv;
    rv = __ioctl(fd, tag, arg);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SEND(int fd, const void *buf, size_t len)
{
    int rv;
    rv = __send(fd, buf, len, 0);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_RECV(int fd, void *buf, size_t len)
{
    int rv;
    rv = __recv(fd, buf, len, 0);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SELECT(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv;
    rv = __select(fd, r, w, e, t);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SOCKET(int domain, int type, int protocol)
{
    int rv;
    rv = __socket(domain, type, protocol);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_CONNECT(int fd, void *addr, int addrlen)
{
    int rv;
    rv = __connect(fd, addr, addrlen);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}
