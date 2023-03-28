#include "xp.h"
#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
extern int XP_ERRNO_EWOULDBLOCK;
#ifdef XP_UNIX
int ssl_DefDup2(SSLSocket *ss, int newfd)
{
    SSLSocket *ds;
    int rv;

    if (ss->fd == newfd) {
	return newfd;
    }
    rv = dup2(ss->fd, newfd);
    if (rv < 0) {
	PORT_SetError(errno);
	SSL_TRC(7, ("%d: SSL[%d]: dup2 failed: %d",
		    SSL_GETPID(), ss->fd, PORT_GetError()));
	return rv;
    }

    /* Create ssl object for dup */
    ds = ssl_DupSocket(ss, newfd);
    if (!ds) {
	close(newfd);
	return rv;
    }
    SSL_TRC(7, ("%d: SSL[%d]: dup2 ok: oldfd=%d newfd=%d",
		SSL_GETPID(), ss->fd, ss->fd, newfd));
    return newfd;
}
#endif

int ssl_UnderlyingAccept(int fd, struct sockaddr *a, int *ap)
{
    return XP_SOCK_ACCEPT(fd, a, ap);
}

int ssl_DefConnect(SSLSocket *ss, const void *sa, int namelen)
{
    int rv = XP_SOCK_CONNECT(ss->fd, (struct sockaddr*) sa, namelen);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefAccept(SSLSocket *ss, void *addr, int* addrlenp)
{
    SSLSocket *ns;
    int newfd;

    newfd = (*ssl_accept_func)(ss->fd, (struct sockaddr*) addr, addrlenp);
    if (newfd < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
	return newfd;
    }

    /* Create new socket */
    ns = ssl_DupSocket(ss, newfd);
    if (!ns) {
	XP_SOCK_CLOSE(newfd);
	return -1;
    }
    return newfd;
}

int ssl_DefImportFd(SSLSocket *ss, int fd)
{
    SSLSocket *ns;

    /* Create new socket */
    ns = ssl_NewSocket(fd);
    if (!ns) {
	return -1;
    }
    ns->ops = ss->ops;
    return fd;
}

int ssl_DefBind(SSLSocket *ss, const void *addr, int addrlen)
{
    int rv = XP_SOCK_BIND(ss->fd, (struct sockaddr*) addr, addrlen);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefListen(SSLSocket *ss, int backlog)
{
    int rv = XP_SOCK_LISTEN(ss->fd, backlog);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefShutdown(SSLSocket *ss, int how)
{
    int rv = XP_SOCK_SHUTDOWN(ss->fd, how);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefIoctl(SSLSocket *ss, int how, void *buf)
{
    int rv;
#ifdef XP_MAC
    rv = XP_SOCK_IOCTL(ss->fd, how, (int*) buf);
#else
    rv = XP_SOCK_IOCTL(ss->fd, how, buf);
#endif
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefRecv(SSLSocket *ss, void *buf, int len, int flags)
{
    int rv = XP_SOCK_RECV(ss->fd, buf, len, flags);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefSend(SSLSocket *ss, const void *buf, int len, int flags)
{
    int rv, count;

    count = 0;
    for (;;) {
#if defined(__sun) && defined(SYSV)
        /* Brutal workaround for a bug in the Solaris 2.4 send routine */
	rv = XP_SOCK_WRITE(ss->fd, (char *) buf, len);
#else
	rv = XP_SOCK_SEND(ss->fd, (char *) buf, len, flags);
#endif
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	    if (PORT_GetError() == XP_ERRNO_EWOULDBLOCK) {
		/*
		** Retry the send for awhile. Hopefully it won't take
		** long.
		*/
		if (ss->asyncWrites) {
		    /* Return short write if some data already went out... */
		    return count ? count : rv;
		}

#if defined(XP_WIN) && defined(MOZILLA_CLIENT)
		FEU_StayingAlive();
#endif /* XP_WIN */

		continue;
	    }
	    /* Loser */
	    return rv;
	}
	count += rv;
	if (rv != len) {
	    /* Short send. Send the rest in the next call */
	    buf = (const void*) ((const unsigned char*)buf + rv);
	    len -= rv;
	    continue;
	}
	break;
    }
    return count;
}

int ssl_DefRead(SSLSocket *ss, void *buf, int len)
{
    int rv = XP_SOCK_READ(ss->fd, buf, len);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefWrite(SSLSocket *ss, const void *buf, int len)
{
    int rv, count;

    count = 0;
    for (;;) {
	rv = XP_SOCK_WRITE(ss->fd, (const char *)buf, len);
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	    if (PORT_GetError() == XP_ERRNO_EWOULDBLOCK) {
		/*
		** Retry the write for awhile. Hopefully it won't take
		** long.
		*/
		if (ss->asyncWrites) {
		    /* Return short write if some data already went out... */
		    return count ? count : rv;
		}

#if defined(XP_WIN) && defined(MOZILLA_CLIENT)
		FEU_StayingAlive();
#endif /* XP_WIN */

		continue;
	    }
	    /* Loser */
	    return rv;
	}
	count += rv;
	if (rv != len) {
	    /* Short write. Send the rest in the next call */
	    buf = (const void*) ((const unsigned char*)buf + rv);
	    len -= rv;
	    continue;
	}
	break;
    }
    return count;
}

int ssl_DefGetpeername(SSLSocket *ss, void *name, int *namelenp)
{
    int rv = XP_SOCK_GETPEERNAME(ss->fd, (struct sockaddr *) name, namelenp);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefGetsockname(SSLSocket *ss, void *name, int *namelenp)
{
    int rv = XP_SOCK_GETSOCKNAME(ss->fd, (struct sockaddr *) name, namelenp);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefGetsockopt(SSLSocket *ss, int level, int optname, void *optval,
		      int *optlen)
{
    int rv = XP_SOCK_GETSOCKOPT(ss->fd, level, optname, optval, optlen);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_DefSetsockopt(SSLSocket *ss, int level, int optname,
		      const void * optval, int optlen)
{
    int rv = XP_SOCK_SETSOCKOPT(ss->fd, level, optname, (char *)optval, optlen);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}
