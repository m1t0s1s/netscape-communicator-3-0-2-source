#include <stdarg.h>
#include <memory.h>
#include "prfile.h"
#include "prmon.h"

#ifdef SW_THREADS
#include "swkern.h"
#endif

#if !defined(XP_STRLEN)
#define XP_STRLEN(str)          strlen(str)
#endif

#if defined(IRIX)
#include <signal.h>
extern sigset_t _pr_holdALRM;
#endif

#ifdef SOLARIS
#include <sys/filio.h>
#endif

#if defined(SCO)
#include <sys/socket.h>
#endif

/* XXX into a header file somewhere */
#if defined(XP_UNIX)
#include <sys/time.h>
#define bzero(a,b) memset(a,0,b)

#define _OS_ERROR_EAGAIN        (-EAGAIN)
#define _OS_ERROR_EINTR         (-EINTR)
#define _OS_ERROR_EINPROGRESS	(-EINPROGRESS)
#define _OS_ERROR_EALREADY	(-EALREADY)
#define _OS_ERROR_ECONNREFUSED	(-ECONNREFUSED)
#define _OS_ERROR_EISCONN	(-EISCONN)

#define PR_ERROR_INVALID_FD     (-EBADF)
#define PR_ERROR_WOULD_BLOCK    (-EWOULDBLOCK)

#elif defined(XP_PC)
write me (use wsa error codes);
#elif defined(XP_MAC)
write me;
#endif

PRIOMethods _pr_net_methods;

extern PRFileHandle _PR_InitFileHandle(PROSFD osfd, PRIOMethods *m);

/*
** Import an existing OS socket to NSPR. Under UNIX, useful for importing
** pipes and socketpairs.
*/
PRFileHandle PR_ImportSocket(PROSFD osfd)
{
    PRFileHandle hdl = -1;

    _PR_InitIO();
    /* Make socket non-blocking */
#if defined(XP_UNIX)
    {
	int flgs;
	if ((flgs = _OS_FCNTL(osfd, F_GETFL, 0)) == -1) {
	    /* We are hosed */
	    (void) _OS_CLOSE_SOCKET((PROSFD) osfd);
	    PR_SetError(flgs);
	    goto done;
	}
	_OS_FCNTL(osfd, F_SETFL, flgs | O_NONBLOCK);
    }
#elif defined(XP_PC)
    write me;
#elif defined(XP_MAC)
    write me;
#endif

    /* the os-file-handle is valid */
    hdl = _PR_InitFileHandle((PROSFD) osfd, &_pr_net_methods);
    if( !hdl ) {
        /*
	** Unable to get a slot in the FD table. Ignore error from
	** OS_CLOSE so we don't hide table allocation failure.
	*/
        (void) _OS_CLOSE_SOCKET((PROSFD) osfd);
        hdl = -1;
    }

   done:
    return hdl;
}


/*
** Create a socket. Uses PR_ImportSocket to introduce OS socket to NSPR.
*/
PRFileHandle PR_CreateSocket(int domain, int type, int protocol)
{
    int osfd;

    _PR_InitIO();

    osfd = _OS_SOCKET(domain, type, protocol);
    if (osfd < 0) {
        /* unable to open the file. */
        PR_SetError(osfd);
        return -1;
    } 
    return PR_ImportSocket(osfd);
}

/*
 * On SunOS 2.3 the connect routine (really a library call) can
 * sometimes return an error and errno == EPROTO.  in this case it
 * is pointless to restart the connect call as it will just fail
 * again with EPROTO.  I don't know what causes this but it seem
 * like for now that the only way to fix the problem is to shut the
 * socket down and open it up again.  We report the error to Java and
 * it can try to restart the connection.  Also, if the connection is
 * refused, it seems that select() returns 1, as if everything is
 * OK, but there's no way to find out that the connection failed
 * until you try something!  So, now we try something immediately,
 * and see what we get.  What this boils down to is socket "system
 * calls" do not work when the socket is in nonblocking I/O mode.
 */
int32 PR_ConnectSocket(PRFileDesc *fd, void *addr, int addrlen)
{
    int oserror;
    fd_set write_fds;
    struct timeval poll;

    _PR_InitIO();

    FD_ZERO(&write_fds);
    FD_SET(fd->osfd, &write_fds);
    poll.tv_sec = poll.tv_usec = 0;

#if defined(IRIX)
    /*
    ** Block out timer interrupts. On IRIX taking an interrupt during a
    ** connect sometimes causes an EADDRINUSE
    */
    sigprocmask(SIG_BLOCK, &_pr_holdALRM, 0);
#endif

    oserror = _OS_CONNECT(fd->osfd, addr, addrlen);

#if defined(IRIX)
    /*
    ** Release timer interrupts. On IRIX taking an interrupt during a
    ** connect sometimes causes an EADDRINUSE
    */
    sigprocmask(SIG_UNBLOCK, &_pr_holdALRM, 0);
#endif

    while ((oserror < 0) && !_PR_PENDING_EXCEPTION()) {
	if ((oserror == _OS_ERROR_EINPROGRESS) ||
	    (oserror == _OS_ERROR_EALREADY)) {
#ifdef SOLARIS
	    int	cnt = _OS_SELECT(fd->osfd+1, 0, &write_fds, 0, &poll);
	    FD_SET(fd->osfd, &write_fds);
	    if (cnt == 1) {
		int bytes;
		int ioc;

		if ((ioc =_OS_IOCTL(fd->osfd, FIONREAD, (int)&bytes)) < 0) {
		    /* If this fails with EPIPE, then we assume
		       connection is refused - who knows what's
		       right?  Not I. */
		    if (ioc == -EPIPE) {
			oserror = _OS_ERROR_ECONNREFUSED;
		    }
		    PR_SetError(oserror);
		    break;
		}
		oserror = 0;
		break;
	    }
#endif
            _PR_WaitForFD(fd, 0);

	    /* Try again... */
	    oserror = _OS_CONNECT(fd->osfd, addr, addrlen);
	} else if (oserror == _OS_ERROR_EISCONN) {
	    oserror = 0;
	} else {
	    break;
	}
    }
    return oserror;
}

/*
** Read data from a socket object into buffer. This allows for async
** socket i/o.
*/
int32 _PR_ReadSocket(PRFileDesc *fd, void *buf, int32 amount)
{
    int32 oserror;

    if (!fd || _OS_INVALID_FD(fd)) {
        PR_SetError(PR_ERROR_INVALID_FD);
        return -1;
    }

    oserror = _OS_RECV(fd->osfd, buf, amount, 0);
    while ((oserror < 0) && !_PR_PENDING_EXCEPTION() &&
           (oserror == _OS_ERROR_EAGAIN || oserror == _OS_ERROR_EINTR)) {
        if (oserror == _OS_ERROR_EAGAIN) {
            if (fd->flags & _PR_USER_ASYNC) {
                oserror = PR_ERROR_WOULD_BLOCK;
                PR_SetError(oserror);
                return oserror;
            }
            _PR_WaitForFD(fd, 1);
        }
        oserror = _OS_RECV(fd->osfd, buf, amount, 0);
    }
    return oserror;
}

#if 0
/*
** Scatter gather data from a file object into buffer. This allows for
** async file i/o in case either the underlying system supports it or if
** (for example) files are used to represent devices (e.g. unix).
*/
int32 _PR_ReadvSocket(PRFileDesc *fd, PRIOVec *iov, int iovcnt)
{
    int32 oserror;

    if (!fd || _OS_INVALID_FD(fd)) {
        PR_SetError(PR_ERROR_INVALID_FD);
        return -1;
    }

    oserror = _OS_READV(fd->osfd, iov, iovcnt);
    while ((oserror < 0) && !_PR_PENDING_EXCEPTION() &&
           ((oserror == _OS_ERROR_EAGAIN) || (oserrno == _OS_ERROR_EINTR))) {
        if (oserror == _OS_ERROR_EAGAIN) {
            if (fd->flags & _PR_USER_ASYNC) {
                oserror = PR_ERROR_WOULD_BLOCK;
                PR_SetError(oserror);
                return oserror;
            }
            _PR_WaitForFD(fd, 1);
        }
        oserror = _OS_READV(fd->osfd, iov, iovcnt);
    }
    return oserror;
}
#endif

/*
** Write data to a socket object from a buffer. This allows for async
** socket i/o.
*/
int32 _PR_WriteSocket(PRFileDesc *fd, const void *buf, int32 amount)
{
    const char *bp = (const char*) buf;
    int32 oserror, total_written;

    if (!fd || _OS_INVALID_FD(fd)) {
        PR_SetError(PR_ERROR_INVALID_FD);
        return -1;
    }
    total_written = 0;
    while ((total_written < amount) && !_PR_PENDING_EXCEPTION()) {
        oserror = _OS_SEND(fd->osfd, bp+total_written, amount-total_written, 0);
        while ((oserror < 0) && !_PR_PENDING_EXCEPTION()) {
            if (oserror == _OS_ERROR_EAGAIN) {
                if (fd->flags & _PR_USER_ASYNC) {
                    if (total_written == 0) {
                        oserror = PR_ERROR_WOULD_BLOCK;
                        PR_SetError(oserror);
                        total_written = oserror;
                    }
                    return total_written;
                }
                _PR_WaitForFD(fd, 0);
            } else if (oserror != _OS_ERROR_EINTR) {
                total_written = oserror;
                return total_written;
            }
            oserror = _OS_SEND(fd->osfd, bp+total_written,
			       amount-total_written, 0);
        }
        total_written += oserror;
    }
    return total_written;
}

#if 0
/*
** Scatter write data to a file object from some buffers. This allows for
** async file i/o in case either the underlying system supports it or if
** (for example) files are used to represent devices (e.g. unix).
*/
int32 _PR_WritevSocket(PRFileDesc *fd, PRIOVec *iov, int iovcnt)
{
    int32 total_written = 0;
    int32 nwrote, ndone = 0;
    int32 oserror;

    if (!fd || _OS_INVALID_FD(fd)) {
        PR_SetError(PR_ERROR_INVALID_FD);
        return -1;
    }

    LOCK(pr_asyncOutput);
    _PR_AsyncFD(fd);

    while ((iovcnt > 0) && !pendingException() ) {
        iov->iov_len -= ndone;
        iov->iov_base += ndone;
        nwrote = OS_WRITEV_FILE(fd->osfd, iov, iovcnt, &oserror);
        while (oserror) {
            if (errno == EAGAIN) {
                if (fd->flags & FD_ASYNC) {
                    if (total_written == 0) {
                        total_written = -1;
                    }
                    goto unlock;
                }
                _PR_WaitForFD(fd, 0);
            } else if (errno != EINTR) {
                total_written = -1;
                goto unlock;
            }
            nwrote = OS_WRITEV_FILE(fd->osfd, iov, iovcnt, &oserror);
        }
        total_written += nwrote;
        iov->iov_len += ndone;
        iov->iov_base -= ndone;
        ndone += nwrote;
        for (; (iovcnt > 0) && (ndone >= iov->iov_len) ; iovcnt--, iov++) {
            ndone -= iov->iov_len;
        }
    }

unlock:
    UNLOCK(pr_asyncOutput);
    return total_written;
}
#endif

int32 _PR_CloseSocket(PRFileDesc *fd)
{
    int oserror = PR_ERROR_INVALID_FD;

    if (fd) {
        oserror = _OS_CLOSE_SOCKET(fd->osfd);
        fd->osfd = _OS_INVALID_FD_VALUE;
    }
    PR_SetError(oserror);
    return oserror;
}

PRIOMethods _pr_net_methods = {
    0,
    _PR_CloseSocket,
    _PR_ReadSocket,
    _PR_WriteSocket,
};
