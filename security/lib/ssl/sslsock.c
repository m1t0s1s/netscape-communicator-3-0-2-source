#include "xp.h"
#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "nspr.h"

extern int XP_ERRNO_EINVAL;
extern int XP_ERRNO_EBADF;
extern int SEC_ERROR_INVALID_ARGS;

SSLSocketOps ssl_default_ops = {
    ssl_DefConnect,
    ssl_DefAccept,
    ssl_DefBind,
    ssl_DefListen,
    ssl_DefShutdown,
    ssl_DefIoctl,
    ssl_DefClose,
    ssl_DefRecv,
    ssl_DefSend,
    ssl_DefRead,
    ssl_DefWrite,
    ssl_DefGetpeername,
    ssl_DefGetsockname,
    ssl_DefGetsockopt,
    ssl_DefSetsockopt,
    ssl_DefImportFd,
#ifdef XP_UNIX
    ssl_DefDup2,
#endif
};

SSLSocketOps ssl_socks_ops = {
    ssl_SocksConnect,
    ssl_SocksAccept,
    ssl_SocksBind,
    ssl_SocksListen,
    ssl_DefShutdown,
    ssl_DefIoctl,
    ssl_DefClose,
    ssl_SocksRecv,
    ssl_SocksSend,
    ssl_SocksRead,
    ssl_SocksWrite,
    ssl_DefGetpeername,
    ssl_SocksGetsockname,
    ssl_DefGetsockopt,
    ssl_DefSetsockopt,
    ssl_DefImportFd,
#ifdef XP_UNIX
    ssl_DefDup2,
#endif
};

#ifndef NADA_VERSION
SSLSocketOps ssl_secure_ops = {
    ssl_SecureConnect,
    ssl_SecureAccept,
    ssl_DefBind,
    ssl_DefListen,
    ssl_DefShutdown,
    ssl_DefIoctl,
    ssl_SecureClose,
    ssl_SecureRecv,
    ssl_SecureSend,
    ssl_SecureRead,
    ssl_SecureWrite,
    ssl_DefGetpeername,
    ssl_DefGetsockname,
    ssl_DefGetsockopt,
    ssl_DefSetsockopt,
    ssl_SecureImportFd,
#ifdef XP_UNIX
    ssl_DefDup2,
#endif
};

SSLSocketOps ssl_secure_socks_ops = {
    ssl_SecureSocksConnect,
    ssl_SecureSocksAccept,
    ssl_SocksBind,
    ssl_SocksListen,
    ssl_DefShutdown,
    ssl_DefIoctl,
    ssl_SecureClose,
    ssl_SecureRecv,
    ssl_SecureSend,
    ssl_SecureRead,
    ssl_SecureWrite,
    ssl_DefGetpeername,
    ssl_SocksGetsockname,
    ssl_DefGetsockopt,
    ssl_DefSetsockopt,
    ssl_SecureImportFd,
#ifdef XP_UNIX
    ssl_DefDup2,
#endif
};
#endif

/*
** default settings for socket enables
*/
static struct {
    unsigned char useSecurity;
    unsigned char useSocks;
    unsigned char requestCertificate;
    unsigned char asyncWrites;
    unsigned char delayedHandshake;
    unsigned char handshakeAsClient;
    unsigned char handshakeAsServer;
    unsigned char enableSSL2;
    unsigned char enableSSL3;
    unsigned char noCache;
} ssl_defaults = {
    0, /* useSecurity */
    0, /* useSocks */
    0, /* requestCertificate */
    0, /* asyncWrites */
    0, /* delayedHandshake */
    0, /* handshakeAsClient */
    0, /* handshakeAsServer */
    1, /* enableSSL2 */
    1, /* enableSSL3 */
    0, /* noCache */
};

char ssl_debug, ssl_trace;
int ssl_extended_error;
SSLSessionIDLookupFunc ssl_sid_lookup;
SSLSessionIDCacheFunc ssl_sid_cache;
SSLSessionIDUncacheFunc ssl_sid_uncache;
SSLAcceptFunc ssl_accept_func = ssl_UnderlyingAccept;

/************************************************************************/

static SSLSocket *fdhash[16];
#define FDHASH(fd) ((fd) & 15)
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
#include "nspr/prmon.h"
static int fdhashLockInit = 0;
static PRMonitor *fdhashLock;
#endif

/*
 * The locking (and unlocking) of the fdhash is done to protect the linked
 * lists of SSLSocket structures hanging off of each of the fdhash buckets.
 * This protection is only needed in a multi-threaded world which is
 * typically only in the server.
 *
 * To avoid this cost, an extra table is made for "small" values of file
 * descriptors (which tend to be small ints). Since the entries can be
 * added or removed atomically, no locking will be needed.
 */

#if defined(SERVER_BUILD)
#define SMALL_FD 2500		/* 25 would be useful for testing purposes */
#else
#define SMALL_FD 256
#endif
static SSLSocket *fdTable[SMALL_FD];

/*
** Lookup a socket structure given a file descriptor.
*/
SSLSocket *ssl_FindSocket(int fd)
{
    SSLSocket *ss;

    if (fd < SMALL_FD) {
	ss = fdTable[fd];
    } else {
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
	PORT_Assert(fdhashLockInit);
	PR_EnterMonitor(fdhashLock);
#endif
	ss = fdhash[FDHASH(fd)];
	while (ss && (ss->fd != (NETSOCK)fd))
	    ss = ss->next;
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
	PR_ExitMonitor(fdhashLock);
#endif
    }
    if (ss == NULL)
	PORT_SetError(XP_ERRNO_EBADF);
    return ss;
}

static SSLSocket *
ssl_FindSocketAndDelink(int fd)
{
    SSLSocket *ss, **ssp;

    if (fd < SMALL_FD) {
	ss = fdTable[fd];
	fdTable[fd] = NULL;
    } else {
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
	PORT_Assert(fdhashLockInit);
	PR_EnterMonitor(fdhashLock);
#endif
	ssp = &fdhash[FDHASH(fd)];
	while (((ss = *ssp) != NULL) && (ss->fd != (NETSOCK)fd)) {
	    ssp = &ss->next;
	}
	if (ss != NULL)
	    *ssp = ss->next;      /* Remove from hash chain */
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
	PR_ExitMonitor(fdhashLock);
#endif
    }
    return ss;
}

static void
ssl_RememberSocket(SSLSocket *ss, int fd)
{
    PORT_Assert(ss != NULL);
    if (fd < SMALL_FD) {
	PORT_Assert(fdTable[fd] == NULL);
	fdTable[fd] = ss;
    } else {
	int fdh = FDHASH(fd);
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
	PR_EnterMonitor(fdhashLock);
#endif
	ss->next = fdhash[fdh];
	fdhash[fdh] = ss;
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
	PR_ExitMonitor(fdhashLock);
#endif
    }
}

/*
** Create a newsocket structure for a file descriptor.
*/
SSLSocket *ssl_NewSocket(int fd)
{
    SSLSocket *ss;
#ifdef XP_UNIX
    static int firsttime = 1;
    if (firsttime) {
	firsttime = 0;
#ifdef DEBUG
	{
	    char *ev = getenv("SSLDEBUG");
	    if (ev && ev[0]) {
		ssl_debug = atoi(ev);
		XP_TRACE(("SSL: debugging set to %d", ssl_debug));
	    }
	}
#endif
#ifdef TRACE
	{
	    char *ev = getenv("SSLTRACE");
	    if (ev && ev[0]) {
		ssl_trace = atoi(ev);
		XP_TRACE(("SSL: tracing set to %d", ssl_trace));
	    }
	}
#endif
    }
#endif

    /*
    ** Fix up case where socket was left open without calling SSL_Close.
    ** Scan hash chains for fd and if we find a matching socket, destroy
    ** it by calling it's close proc. Step on the fd field so that the
    ** close proc won't actually close the fd!
    */

    ss = ssl_FindSocket(fd);
    if (ss) {
	(void) ssl_FindSocketAndDelink(fd);
	ssl_FreeSocket(ss);
    }

    /* Make a new socket and get it ready */
    ss = (SSLSocket*) PORT_ZAlloc(sizeof(SSLSocket));
    if (ss) {
	ss->fd = fd;
	ss->ops = &ssl_default_ops;
	ss->useSecurity = ssl_defaults.useSecurity;
	ss->useSocks = ssl_defaults.useSocks;
	ss->requestCertificate = ssl_defaults.requestCertificate;
	ss->asyncWrites = ssl_defaults.asyncWrites;
	ss->delayedHandshake = ssl_defaults.delayedHandshake;
	ss->handshakeAsClient = ssl_defaults.handshakeAsClient;
	ss->handshakeAsServer = ssl_defaults.handshakeAsServer;
	ss->enableSSL2 = ssl_defaults.enableSSL2;
	ss->enableSSL3 = ssl_defaults.enableSSL3;
	ss->peer = 0;
	ss->port = 0;
	ss->noCache = ssl_defaults.noCache;
	ss->peerID = NULL;
	ssl_RememberSocket(ss, fd);
#if defined(XP_UNIX) && !defined(SERVER_BUILD) && !defined(NSPR20)
	PR_SetPollHook(fd, SSL_DataPending);
#endif
    }
    return ss;
}

/*
 * SSL_ReplaceSocket is badly named. What it really does is change the FD
 * in the socket structure. This means the SSLSocket* should be moved
 * from the old slot (whether in the small table or in a linked list in
 * the cache) to the new place.
 */
#ifdef XP_WIN32
int SSL_ReplaceSocket(int OldFd, int NewFd)
{
    SSLSocket *ss, **ssp;
    int hashValue;

    if (OldFd == NewFd) return NewFd;

    ss = ssl_FindSocketAndDelink(OldFd);
    if (ss == NULL) {
	PORT_SetError(XP_ERRNO_EBADF);
	return -1;
    }

    closesocket(ss->fd);
    ss->fd = NewFd;

    ssl_RememberSocket(ss, NewFd);

    return NewFd;
}
#endif /* XP_WIN32 */

SSLSocket *ssl_DupSocket(SSLSocket *os, int newfd)
{
    SSLSocket *ss;
    int rv;

    /*
     * XXX PK 13 Mar 96 -- This seems bogus. The socket is put into the
     * table before it is fully initialized.
     */
    ss = ssl_NewSocket(newfd);
    if (ss) {
	ss->useSocks = os->useSocks;
	ss->useSecurity = os->useSecurity;
	ss->requestPassword = os->requestPassword;
	ss->requestCertificate = os->requestCertificate;
	ss->delayedHandshake = os->delayedHandshake;
	ss->handshakeAsClient = os->handshakeAsClient;
	ss->handshakeAsServer = os->handshakeAsServer;
	ss->enableSSL3 = os->enableSSL3;
	ss->enableSSL2 = os->enableSSL2;
	ss->noCache = os->noCache;
	if (os->peerID == NULL) {
	    ss->peerID = NULL;
	} else {
	    ss->peerID = PORT_Strdup(os->peerID);
	}
	ss->ops = os->ops;
	ss->peer = os->peer;
	ss->port = os->port;
#ifndef NADA_VERSION
	if (ss->useSecurity) {
	    /* Create security data */
	    rv = ssl_CopySecurityInfo(ss, os);
	    if (rv) {
		goto losage;
	    }
	}
#endif
	if (ss->useSocks) {
	    /* Create security data */
	    rv = ssl_CopySocksInfo(ss, os);
	    if (rv) {
		goto losage;
	    }
	}
    }
    return ss;

  losage:
    /*
     * XXX PK 13 Mar 96 -- The allocated storage is not freed. The now
     * partially initalized structure is in the cache where some innocent
     * might find it.
     */
    return 0;
}

/*
 * free an SSLSocket struct, and all the stuff that hangs off of it
 */
void
ssl_FreeSocket(SSLSocket *ss)
{
    /* if a dialog is pending, then just mark for later freeing */
    if ( ss->dialogPending ) {
	ss->beenFreed = PR_TRUE;
	return;
    }
    
    /* Free up socket */
    ssl_DestroySocksInfo(ss->socks);
#ifndef NADA_VERSION
    ssl_DestroySecurityInfo(ss->sec);
    ssl3_DestroySSL3Info(ss->ssl3);
#endif
    PORT_FreeBlock(ss->saveBuf.buf);
    if (ss->gather) {
	ssl_DestroyGather(ss->gather);
    }
    if (ss->peerID != NULL)
	PORT_Free(ss->peerID);

    PORT_Free(ss);
    return;
}

int ssl_DefClose(SSLSocket *ss)
{
    SSLSocket *found;
    int fd, rv;

    fd = ss->fd;
    found = ssl_FindSocketAndDelink(fd);
    
#if defined(XP_UNIX) && !defined(SERVER_BUILD) && !defined(NSPR20)
    PR_SetPollHook(fd, NULL);
#endif

    ssl_FreeSocket(ss);
    rv = XP_SOCK_CLOSE(fd);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    SSL_TRC(5, ("%d: SSL[%d]: closing, rv=%d errno=%d",
		SSL_GETPID(), fd, rv, PORT_GetError()));
    return rv;
}

/************************************************************************/


int SSL_Import(int fd)
{
    SSLSocket *ss;

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    PORT_Assert(fdhashLockInit); /* failure means SEC_Init not called */
#endif
    ss = ssl_FindSocket(fd);
    if (ss) {
	/* Already been done... */
	PORT_SetError(XP_ERRNO_EINVAL);
	return -1;
    }
    ss = ssl_NewSocket(fd);
    if (!ss) {
	return -1;
    }
    return 0;
}


int SSL_Socket(int domain, int type, int protocol)
{
    SSLSocket *ss;
    int fd;

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    PORT_Assert(fdhashLockInit); /* failure means SEC_Init not called */
#endif

    /* Sorry, only stream based sockets work here */
    if (type != SOCK_STREAM) {
	PORT_SetError(XP_ERRNO_EINVAL);
	return -1;
    }

    fd = XP_SOCK_SOCKET(domain, type, protocol);
    if (fd == SOCKET_INVALID) {
	PORT_SetError(XP_SOCK_ERRNO);
	return fd;
    }

    ss = ssl_NewSocket(fd);
    if (!ss) {
	XP_SOCK_CLOSE(fd);
	SSL_DBG(("%d: SSL[%d]: unable to make socket object",
		 SSL_GETPID(), fd));
	return -1;
    }
    SSL_TRC(5, ("%d: SSL[%d]: new socket", SSL_GETPID(), fd));
    return fd;
}

static int PrepareSocket(SSLSocket *ss)
{
    unsigned char ix;
    int rv;

    ix = 0;
    if (ss->useSocks) {
	ix |= 1;
	rv = ssl_CreateSocksInfo(ss);
	if (rv) {
	    return rv;
	}
    }
#ifndef NADA_VERSION
    if (ss->useSecurity) {
	ix |= 2;
	rv = ssl_CreateSecurityInfo(ss);
	if (rv) {
	    return rv;
	}
    }
#endif

    switch (ix) {
      case 0:
	ss->ops = &ssl_default_ops;
	break;
      case 1:
	ss->ops = &ssl_socks_ops;
	break;
#ifndef NADA_VERSION
      case 2:
	ss->ops = &ssl_secure_ops;
	break;
      case 3:
	ss->ops = &ssl_secure_socks_ops;
	break;
#endif
    }
    return 0;
}

int SSL_Enable(int s, int which, int on)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in Enable", SSL_GETPID(), s));
	PORT_SetError(XP_ERRNO_EBADF);
	return -1;
    }

    rv = 0;
    switch (which) {
      case SSL_SOCKS:
	ss->useSocks = on ? 1 : 0;
	rv = PrepareSocket(ss);
	break;

#ifndef NADA_VERSION
      case SSL_SECURITY:
	ss->useSecurity = on ? 1 : 0;
	rv = PrepareSocket(ss);
	break;

      case SSL_REQUEST_CERTIFICATE:
	ss->requestCertificate = on ? 1 : 0;
	break;
#endif
      case SSL_ASYNC_WRITES:
	ss->asyncWrites = on ? 1 : 0;
	break;

      case SSL_DELAYED_HANDSHAKE:
	ss->delayedHandshake = on ? 1 : 0;
	break;

      case SSL_HANDSHAKE_AS_CLIENT:
	if ( ss->handshakeAsServer && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return -1;
	}
	ss->handshakeAsClient = on ? 1 : 0;
	break;

      case SSL_HANDSHAKE_AS_SERVER:
	if ( ss->handshakeAsClient && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return -1;
	}
	ss->handshakeAsServer = on ? 1 : 0;
	break;

      case SSL_ENABLE_SSL3:
	ss->enableSSL3 = on ? 1 : 0;
	break;

      case SSL_ENABLE_SSL2:
	ss->enableSSL2 = on ? 1 : 0;
	break;

      case SSL_NO_CACHE:
	ss->noCache = on ? 1 : 0;
	break;
	
      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return -1;
    }
    return rv;
}

int SSL_EnableDefault(int which, int on)
{
    switch (which) {
      case SSL_SOCKS:
	ssl_defaults.useSocks = on ? 1 : 0;
	break;

#ifndef NADA_VERSION
      case SSL_SECURITY:
	ssl_defaults.useSecurity = on ? 1 : 0;
	break;

      case SSL_REQUEST_CERTIFICATE:
	ssl_defaults.requestCertificate = on ? 1 : 0;
	break;
#endif
      case SSL_ASYNC_WRITES:
	ssl_defaults.asyncWrites = on ? 1 : 0;
	break;

      case SSL_DELAYED_HANDSHAKE:
	ssl_defaults.delayedHandshake = on ? 1 : 0;
	break;

      case SSL_HANDSHAKE_AS_CLIENT:
	if ( ssl_defaults.handshakeAsServer && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return -1;
	}
	ssl_defaults.handshakeAsClient = on ? 1 : 0;
	break;

      case SSL_HANDSHAKE_AS_SERVER:
	if ( ssl_defaults.handshakeAsClient && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return -1;
	}
	ssl_defaults.handshakeAsServer = on ? 1 : 0;
	break;

      case SSL_ENABLE_SSL3:
	ssl_defaults.enableSSL3 = on ? 1 : 0;
	break;

      case SSL_ENABLE_SSL2:
	ssl_defaults.enableSSL2 = on ? 1 : 0;
	break;

      case SSL_NO_CACHE:
	ssl_defaults.noCache = on ? 1 : 0;
	break;

      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return -1;
    }
    return 0;
}

void SSL_AcceptHook(SSLAcceptFunc func)
{
    ssl_accept_func = func;
}

/************************************************************************/

#ifdef XP_UNIX
int SSL_Dup2(int oldfd, int newfd)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(oldfd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in dup2", SSL_GETPID(), oldfd));
	return -1;
    }
    return (*ss->ops->dup2)(ss, newfd);
}
#endif

int SSL_Ioctl(int s, int tag, void *result)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in ioctl", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->ioctl)(ss, tag, result);
}

int SSL_Accept(int s, void *sockaddr, int *namelen)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in accept", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->accept)(ss, sockaddr, namelen);
}

int SSL_ImportFd(int s, int fd)
{
    SSLSocket *ss;

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    PORT_Assert(fdhashLockInit); /* failure means SEC_Init not called */
#endif
    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in importfd", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->importfd)(ss, fd);
}

int SSL_Connect(int s, const void *sockaddr, int namelen)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in connect", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->connect)(ss, sockaddr, namelen);
}

int SSL_Bind(int s, const void *addr, int addrlen)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in bind", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->bind)(ss, addr, addrlen);
}

int SSL_Listen(int s, int backlog)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in listen", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->listen)(ss, backlog);
}

int SSL_Shutdown(int s, int how)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in shutdown", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->shutdown)(ss, how);
}

int SSL_Close(int s)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in close", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->close)(ss);
}

int SSL_Recv(int s, void *buf, int len, int flags)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in recv", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->recv)(ss, buf, len, flags);
}

int SSL_Send(int s, const void *buf, int len, int flags)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in send", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->send)(ss, buf, len, flags);
}

int SSL_Read(int s, void *buf, int len)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in read", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->read)(ss, buf, len);
}

int SSL_Write(int s, const void *buf, int len)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in write", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->write)(ss, buf, len);
}

int SSL_GetPeerName(int s, void *addr, int *addrlenp)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in getpeername", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->getpeername)(ss, addr, addrlenp);
}

int SSL_GetSockName(int s, void *name, int *namelenp)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in getsockname", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->getsockname)(ss, name, namelenp);
}

int SSL_GetSockOpt(int s, int level, int optname, void *optval,
		  int *optlen)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in getsockopt", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->getsockopt)(ss, level, optname, optval, optlen);
}

int SSL_SetSockOpt(int s, int level, int optname, const void *optval,
		  int optlen)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in setsockopt", SSL_GETPID(), s));
	return -1;
    }
    return (*ss->ops->setsockopt)(ss, level, optname, optval, optlen);
}

void SSL_InitHashLock(void)
{
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    if (!fdhashLockInit) {
        fdhashLock = PR_NewMonitor();
	fdhashLockInit = 1;
    }
#endif
}

int
SSL_SetSockPeerID(int s, char *peerID)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SSL_SetCacheIndex",
		 SSL_GETPID(), s));
	return -1;
    }

    ss->peerID = PORT_Strdup(peerID);
    return 0;
}
