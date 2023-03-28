/*
 * md_NEWS-OS.c
 * By Daniel Malmer
 * 5/6/96
 */

#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include "mdint.h"
#include <errno.h>

extern int syscall(int, ...);

void _MD_InitOS(int when)
{
    _MD_InitUnixOS(when);
}

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) sigsetjmp(CONTEXT(t), 1);
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}

/************************************************************************/

int _OS_READ(int fd, void *buf, size_t len)
{
    int rv;
    rv = _read(fd, buf, len);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_WRITE(int fd, const void *buf, size_t len)
{
    int rv;
    rv = _write(fd, buf, len);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_SELECT(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv;
    rv = _select(fd, r, w, e, t);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

ssize_t _OS_OPEN(const char *path, int oflag, mode_t mode)
{
    int rv;
    rv = _open(path, oflag, mode);
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_CLOSE(int fd)
{
    int rv;
    rv = _close(fd);
    if(rv < 0)
        return -errno;
    return rv;
}

int _OS_FCNTL(int fd, int flag, int value)
{
    int rv;
    rv = _fcntl(fd, flag, value);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}

int _OS_SEND(int fd, const void *buf, size_t len)
{
    int rv;
    rv = send(fd, buf, len, 0);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}

int _OS_RECV(int fd, void *buf, size_t len)
{
    int rv;
    rv = recv(fd, buf, len, 0);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}

int _OS_SOCKET(int domain, int type, int protocol)
{   
    int rv;
    rv = socket(domain, type, protocol);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}

int _OS_CONNECT(int fd, void *addr, int addrlen)
{
    int rv;
    rv = connect(fd, addr, addrlen);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}   

