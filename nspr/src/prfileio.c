#include <stdarg.h>
#include <string.h>
#include "prfile.h"

#ifdef SW_THREADS
#include "swkern.h"
#else
#include "hwkern.h"
#endif

#if !defined(XP_STRLEN)
#define XP_STRLEN(str)          strlen(str)
#endif

#define _OS_ERROR_EAGAIN        (-EAGAIN)
#define _OS_ERROR_EINTR         (-EINTR)

#define PR_ERROR_INVALID_FD     (-EBADF)
#define PR_ERROR_WOULD_BLOCK    (-EWOULDBLOCK)

PRIOMethods _pr_file_methods;

extern PRFileHandle _PR_InitFileHandle(PROSFD osfd, PRIOMethods *m);

/*
** Open a file on the native filesystem.
*/
PR_PUBLIC_API(PRFileHandle) PR_OpenFile(const char *fileName, int flags, 
                                       int mode)
{
    PRFileHandle hdl = -1;
    int oserror;
    char *fn;

    _PR_InitIO();

    fn = _OS_PREPARE_NAME(fileName, XP_STRLEN(fileName));
    if (!fn) {
        return 0;
    }

#if defined(XP_UNIX)
    flags |= O_NONBLOCK;

#elif defined(XP_PC)

#elif defined(XP_MAC)
#endif
    oserror = _OS_OPEN(fn, flags, mode);
    if (oserror < 0) {
        /* unable to open the file. */
        PR_SetError(oserror);
        goto done;
    } 

    /* the os-file-handle is valid*/
    hdl = _PR_InitFileHandle((PROSFD) oserror, &_pr_file_methods);
    if( hdl == -1 ) {
        /*
	** Unable to get a slot in the FD table. Ignore error from
	** OS_CLOSE so we don't hide table allocation failure.
	*/
        (void) _OS_CLOSE((PROSFD) oserror);
        hdl = -1;
    }

   done:
    _OS_FREE_NAME(fn);
    return hdl;
}

/*
** Read data from a file object into buffer. This allows for async file
** i/o in case either the underlying system supports it or if (for
** example) files are used to represent devices (e.g. unix).
*/
int32 _PR_ReadFile(PRFileDesc *fd, void *buf, int32 amount)
{
    int32 oserror;

    if (!fd || _OS_INVALID_FD(fd)) {
        PR_SetError(PR_ERROR_INVALID_FD);
        return -1;
    }

    oserror = _OS_READ(fd->osfd, buf, amount);
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
        oserror = _OS_READ(fd->osfd, buf, amount);
    }
    return oserror;
}

#if 0
/*
** Scatter gather data from a file object into buffer. This allows for
** async file i/o in case either the underlying system supports it or if
** (for example) files are used to represent devices (e.g. unix).
*/
int32 _PR_ReadvFile(PRFileDesc *fd, PRIOVec *iov, int iovcnt)
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
** Write data to a file object from a buffer. This allows for async file
** i/o in case either the underlying system supports it or if (for
** example) files are used to represent devices (e.g. unix).
*/
int32 _PR_WriteFile(PRFileDesc *fd, const void *buf, int32 amount)
{
    const char *bp = (const char*) buf;
    int32 oserror, total_written;

    if (!fd || _OS_INVALID_FD(fd)) {
        PR_SetError(PR_ERROR_INVALID_FD);
        return -1;
    }
    total_written = 0;
    while ((total_written < amount) && !_PR_PENDING_EXCEPTION()) {
        oserror = _OS_WRITE(fd->osfd, bp+total_written, amount-total_written);
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
            oserror = _OS_WRITE(fd->osfd, bp+total_written,
                                amount-total_written);
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
int32 _PR_WritevFile(PRFileDesc *fd, PRIOVec *iov, int iovcnt)
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

int32 _PR_CloseFile(PRFileDesc *fd)
{
    int oserror = PR_ERROR_INVALID_FD;

    if (fd) {
        oserror = _OS_CLOSE(fd->osfd);
        fd->osfd = _OS_INVALID_FD_VALUE;
    }
    PR_SetError(oserror);
    return oserror;
}

PRIOMethods _pr_file_methods = {
    0,
    _PR_CloseFile,
    _PR_ReadFile,
    _PR_WriteFile,
};


