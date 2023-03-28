#include <sys/param.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/siginfo.h>
#include <sys/stat.h>
#include <sys/tihdr.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/tiuser.h>
#include <sys/sockmod.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>
#include <unistd.h>
#include "prlog.h"
#include "mdint.h"
#include "prdump.h"
#include "prprf.h"
#include "prgc.h"
#include "swkern.h"

/*
** Under IRIX, SIGALRM's cause connect to break when you are connecting
** non-blocking. We work around the bug by holding SIGALRM's during the
** connect system call.
*/
sigset_t _pr_holdALRM;

/*
** most simple...
*/
void _MD_InitOS(int when)
{
    _MD_InitUnixOS(when);
} 

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    extern void _MD_SaveSRegs(void*);

    if (isCurrent) {
	_MD_SaveSRegs(&t->context.uc_mcontext.gpregs[0]);
	*np = 9;
    } else {
	*np = NGREG;
    }
    return (prword_t*) &t->context.uc_mcontext.gpregs[0];
}

extern int _select();

int _OS_SELECT(nfds, readfds, writefds, exceptfds, timeout)
int nfds;
fd_set *readfds, *writefds, *exceptfds;
struct timeval *timeout;
{
    int result;

    result = _select(nfds, readfds, writefds, exceptfds, timeout);
    if (result < 0)
	    return (-errno);
    return result;
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

int _OS_RECV(int s, void *buf, size_t len, int flags)
{
    int rv;
    rv = recv(s, buf, len, flags);
    if (rv < 0) {
        return -errno;
    }
    return rv;
#if 0
    register struct _si_user        *siptr;
    struct msghdr                   msg;
    struct iovec                    msg_iov[1];

    if ((siptr = _s_checkfd(s)) == NULL)
        return (-1);

    msg.msg_iovlen = 1;
    msg.msg_iov = msg_iov;
    msg.msg_iov[0].iov_base = buf;
    msg.msg_iov[0].iov_len = len;
    msg.msg_namelen = 0;
    msg.msg_name = NULL;
    msg.msg_accrightslen = 0;
    msg.msg_accrights = NULL;

    if ((rv = _s_soreceive(siptr, &msg, flags)) < 0)
	return -1;
    else
	return rv;
#endif
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

#include <string.h>
#include <ctype.h>

/*
 * strcasecmp
 * Used so we don't have to link with libucb.a.
 */
int
strcasecmp(char* s1, char* s2)
{
   for ( ; *s1 && *s2 && toupper(*s1) == toupper(*s2); s1++, s2++ ) ;

   if ( *s1 == '\0' && *s2 == '\0' ) return 0;

   return ( toupper(*s1) > toupper(*s2) ? 1 : -1 );
}
