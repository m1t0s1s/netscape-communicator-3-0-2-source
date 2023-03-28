#include "prtypes.h"
#include "prthread.h"
#include "prosdep.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
 
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
 
int _OS_OPEN(const char *path, int oflag, mode_t mode)
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
 
int _OS_SELECT(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv;
    rv = _select(fd, r, w, e, t);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_FCNTL(int filedes, int cmd, int arg)
{
    int rv = _fcntl(filedes, cmd, arg);
    if(rv < 0)
        return -errno;
    return rv;
}
 
int _OS_SOCKET(int domain, int type, int flags)
{
    int rv = socket(domain, type, flags);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_CONNECT(int fd, struct sockaddr *addr, int addrlen)
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
