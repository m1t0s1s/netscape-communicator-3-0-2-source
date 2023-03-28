#ifdef __linux /* jwz */
# include <endian.h>
#endif

#include "fd_md.h"
#include "socket_md.h"
#include "sys_api.h"
#include "iomgr.h"

#ifdef XP_MAC
#include <Types.h>

#include "xp_sock.h"
#endif


/*
** XXX This entire file needs to be encapsualted into nspr so that the
** XXX extra subroutine layers disappear
*/
#if defined(XP_PC)
void sysSocketInitializeFD(Classjava_io_FileDescriptor *fdptr, int infd)
{
    fdptr->fd = infd+1;

    /* make the socket address immediatly available for reuse */
    setsockopt(infd, SOL_SOCKET, SO_REUSEADDR, "yes", 4);
}


long sysSocketAvailable(int fdnum, long *pbytes)
{
    int ret = 1;

    if (ioctlsocket(fdnum, FIONREAD, pbytes) < 0) {
	ret = 0;
    }
    return ret;
}
#endif

void sysSocketFD(Classjava_io_FileDescriptor *fd, long fdnum)
{
    if ((fdnum >= 0) || (fdnum <= 2)) {
	SignalError(0, JAVAPKG "SecurityException", 0);
	return;
    }
    fd->fd = fdnum+1;
}

int sysListenFD(Classjava_io_FileDescriptor *fd, long count)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysListen(fdnum, count);
}

int sysAcceptFD(Classjava_io_FileDescriptor *fd, struct sockaddr *him, int *len)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysAccept(fdnum, him, len);
}

int sysBindFD(Classjava_io_FileDescriptor *fd, void *him, int len)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysBind(fdnum, (struct sockaddr*)him, len);
}

int sysConnectFD(Classjava_io_FileDescriptor *fd, struct sockaddr *him, size_t len)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysConnect(fdnum, him, len);
}

int sysRecvFD(Classjava_io_FileDescriptor *fd, char *buf, int nbytes, int32_t flags)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysRecv(fdnum, buf, nbytes, flags);
}

int sysSendFD(Classjava_io_FileDescriptor *fd, char *buf, int nbytes, int32_t flags)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysSend(fdnum, buf, nbytes, flags);
}

int sysSendtoFD(Classjava_io_FileDescriptor *fd, char *buf, int nbytes,
		int flags, struct sockaddr *to, int tolen)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysSendTo(fdnum, buf, nbytes, flags, to, tolen);
}

int sysRecvfromFD(Classjava_io_FileDescriptor *fd, char *buf, int nbytes,
		  int flags, struct sockaddr *from, int *fromlen)
{
    int ret;
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    ret = sysRecvFrom(fdnum, buf, nbytes, flags, from, fromlen);

#ifdef XP_PC
    /*
    ** Unlike the Unix recvfrom(...) winsock will return an error
    ** if the input buffer is smaller than the datagram.  So, to emulate
    ** the expected behavior, ignore the error and return the buffer
    ** size.  
    ** Fortunately, winsock does fill the buffer before returning
    ** an error...
    */
    if( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAEMSGSIZE) ) {
        ret = nbytes;
    }
#endif

    return ret;
}

long sysSocketAvailableFD(Classjava_io_FileDescriptor *fd, long *pbytes)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    return sysSocketAvailable(fdnum, pbytes);
}

/************************************************************************/

#ifdef XP_UNIX
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int sysConnect(int fd, struct sockaddr* sa, int len)
{
#ifdef IRIX
    static int first_time = 1;
    static sigset_t timer_set;
    sigset_t oldset;
#endif
    int rv;

#ifdef IRIX
    if (first_time) {
	sigemptyset(&timer_set);
	sigaddset(&timer_set, SIGALRM);
	first_time = 0;
    }
    sigprocmask(SIG_BLOCK, &timer_set, &oldset);
#endif
    for (;;) {
	rv = connect(fd, sa, len);
	if (rv == 0) {
	    break;
	}
	if ((errno == EINTR) || (errno == EWOULDBLOCK) ||
	    (errno == EAGAIN) || (errno == EINPROGRESS)) {
	    /* Call select here and wait for the connect to finish */
	    fd_set wr;
	    memset(&wr, 0, sizeof(wr));
	    FD_SET(fd, &wr);
	    rv = select(fd+1, 0, &wr, 0, 0);
	    continue;
	}
	if ((errno == EISCONN) || (errno == EALREADY)) {
	    /* Connection finished */
	    rv = 0;
	}
	/* Some other error */
	break;
    }
#ifdef IRIX
    sigprocmask(SIG_SETMASK, &oldset, 0);
#endif
    return rv;
}

int sysAccept(int fd, struct sockaddr* sa, int *lenp)
{
#ifdef IRIX
    static int first_time = 1;
    static sigset_t timer_set;
    sigset_t oldset;
#endif
    int rv;

#ifdef IRIX
    if (first_time) {
	sigemptyset(&timer_set);
	sigaddset(&timer_set, SIGALRM);
	first_time = 0;
    }
    sigprocmask(SIG_BLOCK, &timer_set, &oldset);
#endif
    for (;;) {
	rv = accept(fd, sa, lenp);
	if (rv >= 0) {
	    break;
	}
	if ((errno == EINTR) || (errno == EWOULDBLOCK) ||
	    (errno == EAGAIN) || (errno == EINPROGRESS)) {
	    /* Call select here and wait for the accept to finish */
	    fd_set rd;
	    memset(&rd, 0, sizeof(rd));
	    FD_SET(fd, &rd);
	    rv = select(fd+1, &rd, 0, 0, 0);
	    continue;
	}
	/* Some other error */
	break;
    }
#ifdef IRIX
    sigprocmask(SIG_SETMASK, &oldset, 0);
#endif
    return rv;
}

int sysRecv(int fd, void *buf, int len, int flags)
{
    int rv;

    for (;;) {
	rv = recv(fd, buf, len, flags);
	if (rv < 0) {
	    if ((errno == EINTR) || (errno == EWOULDBLOCK) ||
		(errno == EAGAIN)) {
		/* Wait in select */
		fd_set rd;
		memset(&rd, 0, sizeof(rd));
		FD_SET(fd, &rd);
		rv = select(fd+1, &rd, 0, 0, 0);
		continue;
	    }
	}
	break;
    }
    return rv;
}

int sysSend(int fd, const void *buf, int len, int flags)
{
    int rv;

    for (;;) {
	rv = send(fd, buf, len, flags);
	if (rv < 0) {
	    if ((errno == EINTR) || (errno == EWOULDBLOCK) ||
		(errno == EAGAIN)) {
		/* Wait in select */
		fd_set wr;
		memset(&wr, 0, sizeof(wr));
		FD_SET(fd, &wr);
		rv = select(fd+1, 0, &wr, 0, 0);
		continue;
	    }
	}
	break;
    }
    return rv;
}

int sysRecvFrom(int fd, void *buf, int len, int flags,
		struct sockaddr *from, int *fromlen)
{
    int rv;

    for (;;) {
	rv = recvfrom(fd, buf, len, flags, from, fromlen);
	if (rv < 0) {
	    if ((errno == EINTR) || (errno == EWOULDBLOCK) ||
		(errno == EAGAIN)) {
		/* Wait in select */
		fd_set rd;
		memset(&rd, 0, sizeof(rd));
		FD_SET(fd, &rd);
		rv = select(fd+1, &rd, 0, 0, 0);
		continue;
	    }
	}
	break;
    }
    return rv;
}

int sysSendTo(int fd, const void *buf, int len, int flags,
	      struct sockaddr *to, int tolen)
{
    int rv;

    for (;;) {
	rv = sendto(fd, buf, len, flags, to, tolen);
	if (rv < 0) {
	    if ((errno == EINTR) || (errno == EWOULDBLOCK) ||
		(errno == EAGAIN)) {
		/* Wait in select */
		fd_set wr;
		memset(&wr, 0, sizeof(wr));
		FD_SET(fd, &wr);
		rv = select(fd+1, 0, &wr, 0, 0);
		continue;
	    }
	}
	break;
    }
    return rv;
}

long sysSocketAvailable(int fdnum, long *pbytes)
{
    int ret = 1;

    if (ioctl(fdnum, FIONREAD, pbytes) < 0) {
	ret = 0;
    }
    return ret;
}

void sysSocketInitializeFD(Classjava_io_FileDescriptor *fdptr, int fdnum)
{
    int flags;

    fdptr->fd = fdnum+1;

    /* make the socket address immediatly available for reuse */
    setsockopt(fdnum, SOL_SOCKET, SO_REUSEADDR, "yes", 4);

    /* make the socket non-blocking with async-io */
    flags = fcntl(fdnum, F_GETFL, 0);
#if defined(FNDELAY)
    fcntl(fdnum, F_SETFL, flags | FNDELAY);/* XXX async i/o ! */
#elif defined(O_NONBLOCK)
    fcntl(fdnum, F_SETFL, flags | O_NONBLOCK);/* XXX async i/o ! */
#endif
}

#endif /* XP_UNIX */



#ifdef XP_MAC

long sysSocketAvailable(int fdnum, long *pbytes) 
{
	return macsock_socketavailable(fdnum, (size_t *)pbytes);
}


void sysSocketInitializeFD(Classjava_io_FileDescriptor *fdptr, int fdnum)
{
	Boolean		setBlocking = true;
	
    fdptr->fd = fdnum+1;
    
    /* make the socket address immediatly available for reuse */
    XP_SOCK_SETSOCKOPT(fdnum, SOL_SOCKET, SO_REUSEADDR, "yes", 4);
    sysIOCtl(fdnum, FIONBIO, &setBlocking);
    
    /* Make sure the socket is non-blocking. */
}
#endif /* XP_MAC */
