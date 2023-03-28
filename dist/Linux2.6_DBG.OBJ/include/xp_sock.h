#ifndef __XP_SOCK_h_
#define __XP_SOCK_h_
/*
	This file contains a canonical set of macros for network
	access. It includes SSL if it's being compiled for SSL use.
	Otherwise it maps to native TCP calls for each platform.
	It also includes all system headers required to do networking.
*/

#ifdef NO_SSL /* added by jwz */
# define DONT_USE_SSL_CALLS
#endif /* NO_SSL -- added by jwz */


#include "xp_core.h"
#include "xp_error.h"

#ifdef HAVE_SECURITY /* jwz */
# include "ssl.h"			/* SSL network functions */
#endif

#ifdef XP_UNIX

#ifdef AIXV3
#include <sys/signal.h>
#include <sys/select.h>
#endif /* AIXV3 */

#include <sys/types.h>
#include <string.h>

#include <errno.h>   
#include <sys/time.h>
#include <sys/stat.h>

#undef MAX  /* mcom_db.h defines it, then sys/param.h redefines it. shut up. */
#undef MIN  /* mcom_db.h defines it, then sys/param.h redefines it. shut up. */
#include <sys/param.h>
#undef MAX  /* mcom_db.h defines it, then sys/param.h redefines it. shut up. */
#undef MIN  /* mcom_db.h defines it, then sys/param.h redefines it. shut up. */
#define MIN(a,b) (((a)<(b))?(a):(b))	/* Sun, you fuckers */
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <sys/file.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#ifndef __hpux 
#include <arpa/inet.h> 
#endif /* __hpux */
#include <netdb.h>
#endif /* XP_UNIX */

#ifdef XP_MAC
#include "macsock.h"
#define SOCKET_BUFFER_SIZE 4096
#endif /* XP_MAC */

#ifdef XP_WIN
#include "winsock.h"
#define SOCKET_BUFFER_SIZE 4096

#ifndef EPIPE
#define EPIPE ECONNRESET
#endif

#undef BOOLEAN
#define BOOLEAN char
#endif /* XP_WIN */

#ifdef DONT_USE_SSL_CALLS
/* Network porting interface used by libnet */
# define NETSELECT       select
# define NETCLOSE        close  /* no such thing as closesocket - jwz */
# define NETBIND         bind
# define NETLISTEN       listen
# define NETSOCKET       socket
# define NETCONNECT      connect
# define NETACCEPT       accept
# ifdef XP_WIN
#  define NETSOCK         SOCKET   /* unsigned int */
# else
#  define NETSOCK         int
# endif /* XP_WIN */
# define NETREAD         read
# define NETWRITE        write
# define NETGETSOCKNAME  getsockname
# define SOCKET_IOCTL    ioctl

# ifdef XP_WIN
#  define XP_SOCK_READ(s,b,l) recv(s,b,l,0)
#  define XP_SOCK_WRITE(s,b,l) send(s,b,l,0)
# else
#  ifdef XP_MAC
#   define SOCKET_READ 	macsock_read
#   define SOCKET_WRITE 	macsock_write
#  else
#   define SOCKET_WRITE    write
#   define SOCKET_READ     read
#  endif
# endif /* XP_WIN */

# ifdef XP_WIN
#  define SOCKET_ERRNO    WSAGetLastError()
# else
#  define SOCKET_ERRNO    errno
# endif /* XP_WIN */

# define NETGETFD(x)     x
# ifdef XP_WIN
#  define SOCKET_INACTIVE INVALID_SOCKET
#  define SOCKET_INVALID  INVALID_SOCKET
# else
#  define SOCKET_INACTIVE ((NETSOCK)-1)
#  define SOCKET_INVALID  ((NETSOCK)-1)
# endif /* XP_WIN */

#else

/* Network porting interface used by libnet */
#define NETSELECT       select
#define NETCLOSE        SSL_Close
#define NETBIND         SSL_Bind
#define NETLISTEN       SSL_Listen
#define NETSOCKET       SSL_Socket
#define NETCONNECT      SSL_Connect
#define NETACCEPT       SSL_Accept
#ifdef XP_WIN
#define NETSOCK         SOCKET   /* unsigned int */
#else
#define NETSOCK         int
#endif /* XP_WIN */
#define NETREAD         SSL_Read
#define NETWRITE        SSL_Write
#define NETGETSOCKNAME  SSL_GetSockName
#define SOCKET_IOCTL    SSL_Ioctl
#define SOCKET_WRITE    SSL_Write
#define SOCKET_READ     SSL_Read
#define SOCKET_ERRNO    XP_GetError()
#define NETGETFD(x)     x
#ifdef XP_WIN
#define SOCKET_INACTIVE INVALID_SOCKET
#define SOCKET_INVALID  INVALID_SOCKET
#else
#define SOCKET_INACTIVE ((NETSOCK)-1)
#define SOCKET_INVALID  ((NETSOCK)-1)
#endif /* XP_WIN */

#endif /* dont use ssl */

/************************************************************************/

#ifdef XP_UNIX

/* Network i/o wrappers */
#define XP_SOCKET int
#define XP_SOCK_ERRNO errno
#define XP_SOCK_SOCKET socket
#define XP_SOCK_CONNECT connect
#define XP_SOCK_ACCEPT accept
#define XP_SOCK_BIND bind
#define XP_SOCK_LISTEN listen
#define XP_SOCK_SHUTDOWN shutdown
#define XP_SOCK_IOCTL ioctl
#define XP_SOCK_RECV recv
#define XP_SOCK_RECVFROM recvfrom
#define XP_SOCK_RECVMSG recvmsg
#define XP_SOCK_SEND send
#define XP_SOCK_SENDTO sendto
#define XP_SOCK_SENDMSG sendmsg
#define XP_SOCK_READ read
#define XP_SOCK_WRITE write
#define XP_SOCK_READV readv
#define XP_SOCK_WRITEV writev
#define XP_SOCK_GETPEERNAME getpeername
#define XP_SOCK_GETSOCKNAME getsockname
#define XP_SOCK_GETSOCKOPT getsockopt
#define XP_SOCK_SETSOCKOPT setsockopt
#define XP_SOCK_CLOSE close
#define XP_SOCK_DUP dup

#if defined(__sun) && !defined(__svr4__)
extern int recvfrom (int s, char *buf, int len, int flags, 
		     struct sockaddr *from, int *fromlen);
extern int recvmsg (int s, struct msghdr *msg, int flags);
extern int send (int s, char *msg, int len, int flags);
extern int sendto (int s, char *msg, int len, int flags, struct sockaddr *to,
		   int tolen);
extern int sendmsg (int s, struct msghdr *msg, int flags);
extern int readv (int fd, struct iovec *iov, int iovcnt);
extern int writev (int fd, struct iovec *iov, int iovcnt);
extern int connect(int s, struct sockaddr *name, int namelen);
extern int accept (int s, struct sockaddr *addr, int *addrlen);
extern int bind (int s, struct sockaddr *name, int namelen);
extern int listen (int s, int backlog);
extern int shutdown (int s, int how);
extern int ioctl (int fd, int request, int *arg);
extern int getpeername (int s, struct sockaddr *name, int *namelen);
extern int getsockname (int s, struct sockaddr *name, int *namelen);
extern int getsockopt (int s, int level, int optname, char *optval,
		       int *optlen);
extern int setsockopt (int s, int level, int optname, char *optval,
		       int optlen);
extern int socket(int domain, int type, int protocol);
#endif

#endif /* XP_UNIX */

#ifdef XP_WIN
#define XP_SOCKET SOCKET
#define XP_SOCK_ERRNO WSAGetLastError()

#define XP_SOCK_SOCKET socket
#define XP_SOCK_CONNECT connect
#define XP_SOCK_ACCEPT accept
#define XP_SOCK_BIND bind
#define XP_SOCK_LISTEN listen
#define XP_SOCK_SHUTDOWN shutdown
#define XP_SOCK_IOCTL ioctlsocket
#define XP_SOCK_RECV recv
#define XP_SOCK_RECVFROM recvfrom
#define XP_SOCK_RECVMSG recvmsg
#define XP_SOCK_SEND send
#define XP_SOCK_SENDTO sendto
#define XP_SOCK_SENDMSG sendmsg
#define XP_SOCK_READ(s,b,l) recv(s,b,l,0)
#define XP_SOCK_WRITE(s,b,l) send(s,b,l,0)
#define XP_SOCK_READV readv
#define XP_SOCK_WRITEV writev
#define XP_SOCK_GETPEERNAME getpeername
#define XP_SOCK_GETSOCKNAME getsockname
#define XP_SOCK_GETSOCKOPT getsockopt
#define XP_SOCK_SETSOCKOPT setsockopt
#define XP_SOCK_CLOSE closesocket
#define XP_SOCK_DUP dupsocket


#endif /* XP_WIN */

#ifdef XP_MAC
/*
	Remap unix sockets into GUSI
*/
#define XP_SOCKET int
#define XP_SOCK_ERRNO errno
#define XP_SOCK_SOCKET macsock_socket
#define XP_SOCK_CONNECT macsock_connect
#define XP_SOCK_ACCEPT macsock_accept
#define XP_SOCK_BIND macsock_bind
#define XP_SOCK_LISTEN macsock_listen
#define XP_SOCK_SHUTDOWN macsock_shutdown
#define XP_SOCK_IOCTL macsock_ioctl
#define XP_SOCK_RECV(s,b,l,f) XP_SOCK_READ(s,b,l)
#define XP_SOCK_SEND(s,b,l,f) XP_SOCK_WRITE(s,b,l)
#define XP_SOCK_READ macsock_read
#define XP_SOCK_WRITE macsock_write
#define XP_SOCK_GETPEERNAME macsock_getpeername
#define XP_SOCK_GETSOCKNAME macsock_getsockname
#define XP_SOCK_GETSOCKOPT macsock_getsockopt
#define XP_SOCK_SETSOCKOPT macsock_setsockopt
#define XP_SOCK_CLOSE macsock_close
#define XP_SOCK_DUP macsock_dup

#endif /* XP_MAC */

#endif /* __XP_SOCK_h_ */


