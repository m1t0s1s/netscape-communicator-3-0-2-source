#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include "prlog.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <ieeefp.h>
#include <sys/select.h>
#include <sys/syscall.h>

#include <values.h>
#include <sys/poll.h>

#ifndef NULL
#define NULL    0
#endif


#include "mdint.h"


#if defined(NCR_2)
#include <dlfcn.h>
#include <netdb.h>
#include <errno.h>
 
typedef struct hostent* gethostbyname_type(char*);
 
static gethostbyname_type* good_gethostbyname;
 
/*
 * fix_gethostbyname
 */
static void
fix_gethostbyname(void)
{
    char* library = "/usr/lib/libnet.so";
    void* handle;
 
    if ( (handle = dlopen(library, RTLD_LAZY)) == NULL ) {
        perror(library);
        exit(errno);
    }
 
    if ( (good_gethostbyname = dlsym(handle, "gethostbyname")) == NULL ) {
        perror(library);
        exit(errno);
    }
}

/*
 * gethostbyname
 */
struct hostent*
gethostbyname(char* name)
{
    return good_gethostbyname(name);
}
#endif /* NCR_2 */
 

void _MD_InitOS(int when)
{
    _MD_InitUnixOS(when);

    if (when == _MD_INIT_READY) {
	fpsetmask( 0 );
    }

#if defined(NCR_2)
    fix_gethostbyname();
#endif
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

int _OS_SEND(int fd, const void *buf, int len, int flags)
{
    int rv;
    rv = send(fd, buf, len, flags);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

int _OS_RECV(int fd, void *buf, int len, int flags)
{
    int rv;
#ifdef NOT 
    /* recv on socketpairs seems to be broken in SVR4.  See md_SunOS.c */
    rv = recv(fd, buf, len, flags);
#else
    rv = read(fd, buf, len);
#endif
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

/*
 * Unixware hides select behind _abi_select.  Since X calls _abi_select,
 * we need to intercept that one
 */

int _abi_select(int width, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *tv)
{
    return select(width, rd, wr, ex, tv);
}

#ifdef NCR
#define max(a, b) ((a>b) ? a : b)
 
int
poll(struct pollfd* pfds, unsigned long width, int timeout)
{
    int i;
    int rv;
    int nfds;
    fd_set in, out, ex;
    struct timeval tv;
 
    FD_ZERO(&in);
    FD_ZERO(&out);
    FD_ZERO(&ex);
 
    /*
     * determine nfds
     */
    nfds = 0;
    for ( i = 0; i < width; i++ ) {
        nfds = max(nfds, pfds[i].fd+1);
    }
 
    for ( i = 0; i < width; i++ ) {
        if ( pfds[i].events & POLLIN ) FD_SET(pfds[i].fd, &in);
        if ( pfds[i].events & POLLOUT ) FD_SET(pfds[i].fd, &out);
        if ( pfds[i].events & POLLPRI ) FD_SET(pfds[i].fd, &ex);
    }
 
    /*
     * timeout is in milliseconds
     */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
 
    rv = select(nfds, &in, &out, &ex, timeout < 0 ? NULL : &tv);
 
    for ( i = 0; i < width; i++ ) {
        pfds[i].revents = 0;
    }
 
    if ( rv == 0 ) return 0;
 
    if ( rv < 0 ) {
        for ( i = 0; i < width; i++ ) {
            pfds[i].revents = POLLNVAL;
        }
        return rv;
    }
 
    rv = 0;
    for ( i = 0; i < width; i++ ) {
        if ( FD_ISSET(pfds[i].fd, &in) ) pfds[i].revents = POLLIN;
        if ( FD_ISSET(pfds[i].fd, &out) ) pfds[i].revents = POLLOUT;
        if ( FD_ISSET(pfds[i].fd, &ex) ) pfds[i].revents = POLLERR;
        if (  pfds[i].revents != 0 ) rv++;
    }
 
    return rv;
}
#endif


int _OS_SELECT(nfds, in0, out0, ex0, tv)
        int nfds;
        fd_set *in0, *out0, *ex0;
        struct timeval *tv;
{
        /* register declarations ordered by expected frequency of use */
        register long *in, *out, *ex;
        register u_long m;      /* bit mask */
        register int j;         /* loop counter */
        register u_long b;      /* bits to test */
        register int n, rv, ms;
        struct pollfd pfd[FD_SETSIZE];
        register struct pollfd *p = pfd;
        int lastj = -1;
        /* "zero" is read-only, it could go in the text segment */
        static fd_set zero = { 0 };

        if (nfds < 0) {
                errno = EINVAL;
                return (-errno);
        }

        /*
         * If any input args are null, point them at the null array.
         */
        if (in0 == NULL)
                in0 = &zero;
        if (out0 == NULL)
                out0 = &zero;
        if (ex0 == NULL)
                ex0 = &zero;

        /*
         * For each fd, if any bits are set convert them into
         * the appropriate pollfd struct.
         */
        in = (long *)in0->fds_bits;
        out = (long *)out0->fds_bits;
        ex = (long *)ex0->fds_bits;
        for (n = 0; n < nfds; n += NFDBITS) {
                b = (u_long)(*in | *out | *ex);
                for (j = 0, m = 1; b != 0; j++, b >>= 1, m <<= 1) {
                        if (b & 1) {
                                p->fd = n + j;
                                if (p->fd >= nfds)
                                        goto done;
                                p->events = 0;
                                if (*in & m)
                                        p->events |= POLLRDNORM;
                                if (*out & m)
                                        p->events |= POLLWRNORM;
                                if (*ex & m)
                                        p->events |= POLLRDBAND;
                                p++;
                        }
                }
                in++;
                out++;
                ex++;
        }
done:
        /*
         * Convert timeval to a number of millseconds.
         * Test for zero cases to avoid doing arithmetic.
         * XXX - this is very complex, is it worth it?
         */
        if (tv == NULL) {
                ms = -1;
        } else if (tv->tv_sec == 0) {
                if (tv->tv_usec == 0) {
                        ms = 0;
                } else if (tv->tv_usec < 0 || tv->tv_usec > 1000000) {
                        errno = EINVAL;
                        return (-errno);
                } else {
                        /*
                         * lint complains about losing accuracy,
                         * but I know otherwise.  Unfortunately,
                         * I can't make lint shut up about this.
                         */
                        ms = (int)(tv->tv_usec / 1000);
                }
        } else if (tv->tv_sec > (MAXINT) / 1000) {
                if (tv->tv_sec > 100000000) {
                        errno = EINVAL;
                        return (-errno);
                } else {
                        ms = MAXINT;
                }
        } else if (tv->tv_sec > 0) {
                /*
                 * lint complains about losing accuracy,
                 * but I know otherwise.  Unfortunately,
                 * I can't make lint shut up about this.
                 */
                ms = (int)((tv->tv_sec * 1000) + (tv->tv_usec / 1000));
        } else {        /* tv_sec < 0 */
                errno = EINVAL;
                return (-errno);
        }

        /*
         * Now do the poll.
         */
        n = p - pfd;            /* number of pollfd's */
#ifdef NCR
        rv = syscall(SYS_poll, pfd, (u_long)n, ms);
#else
        rv = poll(pfd, (u_long)n, ms);
#endif
        if (rv < 0)             /* no need to set bit masks */
                return (rv);
        else if (rv == 0) {
                /*
                 * Clear out bit masks, just in case.
                 * On the assumption that usually only
                 * one bit mask is set, use three loops.
                 */
                if (in0 != &zero) {
                        in = (long *)in0->fds_bits;
                        for (n = NFDBITS ; n <= nfds; n += NFDBITS)
                                *in++ = 0;
						n -= NFDBITS ; 

						for ( ; n < nfds ; n++ )
							FD_CLR( n, in0 ) ;
                }
                if (out0 != &zero) {
                        out = (long *)out0->fds_bits;
                        for (n = NFDBITS ; n <= nfds; n += NFDBITS)
                                *out++ = 0;
						n -= NFDBITS ; 

						for ( ; n < nfds ; n++ )
							FD_CLR( n, out0 ) ;
                }
                if (ex0 != &zero) {
                        ex = (long *)ex0->fds_bits;
                        for (n = NFDBITS ; n <= nfds; n += NFDBITS)
                                *ex++ = 0;
						n -= NFDBITS ; 

						for ( ; n < nfds ; n++ )
							FD_CLR( n, ex0 ) ;
                }
                return (0);
        }

        /*
         * Check for EINVAL error case first to avoid changing any bits
         */
        for (p = pfd, j = n; j-- > 0; p++) {
                /*
                 * select will return EBADF immediately if any fd's
                 * are bad.  poll will complete the poll on the
                 * rest of the fd's and include the error indication
                 * in the returned bits.  This is a rare case so we
                 * accept this difference and return the error after
                 * doing more work than select would've done.
                 */
                if (p->revents & POLLNVAL) {
                        errno = EBADF;
                        return (-errno);
                }
        }

        /*
         * Convert results of poll back into bits
         * in the argument arrays.
         *
         * We assume POLLRDNORM, POLLWRNORM, and POLLRDBAND will only be set
         * on return from poll if they were set on input, thus we don't
         * worry about accidentally setting the corresponding bits in the
         * zero array if the input bit masks were null.
         *
         * Must return number of bits set, not number of ready descriptors
         * (as the man page says, and as poll() does).
         */
        rv = 0;
        for (p = pfd; n-- > 0; p++) {
                j = p->fd / NFDBITS;
                /* have we moved into another word of the bit mask yet? */
                if (j != lastj) {
                        /* clear all output bits to start with */
                        in = (long *)&in0->fds_bits[j];
                        out = (long *)&out0->fds_bits[j];
                        ex = (long *)&ex0->fds_bits[j];
                        /*
                         * In case we made "zero" read-only (e.g., with
                         * cc -R), avoid actually storing into it.
                         */
                        if (in0 != &zero)
                                *in = 0;
                        if (out0 != &zero)
                                *out = 0;
                        if (ex0 != &zero)
                                *ex = 0;
                        lastj = j;
                }
                if (p->revents) {
                        m = 1 << (p->fd % NFDBITS);
                        if (p->revents & POLLRDNORM) {
                                *in |= m;
                                rv++;
                        }
                        if (p->revents & POLLWRNORM) {
                                *out |= m;
                                rv++;
                        }
                        if (p->revents & POLLRDBAND) {
                                *ex |= m;
                                rv++;
                        }
                        /*
                         * Only set this bit on return if we asked about
                         * input conditions.
                         */
                        if ((p->revents & (POLLHUP|POLLERR)) &&
                            (p->events & POLLRDNORM)) {
                                if ((*in & m) == 0)
                                        rv++;   /* wasn't already set */
                                *in |= m;
                        }
                        /*
                         * Only set this bit on return if we asked about
                         * output conditions.
                         */
                        if ((p->revents & (POLLHUP|POLLERR)) &&
                            (p->events & POLLWRNORM)) {
                                if ((*out & m) == 0)
                                        rv++;   /* wasn't already set */
                                *out |= m;
                        }
                }
        }
	return (rv < 0 ? -errno : rv);
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

#ifdef NCR
 
/*
 * NCR's kernel is the best.
 * ioctl() (and maybe others...) doesn't get restarted
 * after being interrupted by a signal.
 * We solve this by forcing all calls to _ioctl and ioctl to
 * turn off the clock first.
 */
 
#include <sys/syscall.h>

#pragma weak _ioctl = pr_ioctl
#pragma weak ioctl = pr_ioctl

static int first_time = 1;
static sigset_t timer_set;

int
pr_ioctl(int fd, int request, void* arg)
{
    int rv;
    sigset_t oldset;
 
    if ( first_time ) {
        sigemptyset(&timer_set);
        sigaddset(&timer_set, SIGALRM);
        first_time = 0;
    }
    sigemptyset(&oldset);
    sigprocmask(SIG_BLOCK, &timer_set, &oldset);
    rv = syscall(SYS_ioctl, fd, request, arg);
    sigprocmask(SIG_SETMASK, &oldset, 0);
 
    return rv;
}
#endif


