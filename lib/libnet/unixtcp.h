
#ifndef UNIXTCP_H
#define UNIXTCP_H

#ifdef NO_SSL
/*
 * These defines will be used throughout all the protocol modules
 */
#define NETCLOSE   close         /* closes a network socket */
#define NETSELECT  select        /* select on network socket */
#define NETBIND    bind          /* bind a network socket */
#define NETLISTEN  listen        /* listen to network socket */
#define NETSOCKET  socket        /* create a network socket */
#define NETCONNECT connect       /* connect to host */
#define NETACCEPT  accept        /* accept tcp connection */
#define NETSOCK    int 	         /* socket type */

#define NETGETSOCKNAME getsockname  /* standard get sockname routine */

#define SOCKET_INVALID -1   /* value that determines if a socket is inactive */

#ifdef DEBUG_TCP
#define NETREAD      NET_DebugNetRead  /* read from a network socket */
#define NETWRITE     NET_DebugNetWrite /* write to a network socket */
#else
#define NETREAD      read              /* read from a network socket */
#define NETWRITE     write             /* write to a network socket */
#endif /* DEBUG_TCP */

/* these routines are should be the lowest level tcp
 * routines.  They are to be used by the routines in mktcp.c
 */
#define SOCKET_IOCTL  ioctl   /* normal ioctl routine for sockets */
#define SOCKET_WRITE  write   /* write to a network socket  */
#define SOCKET_READ   read    /* normal socket read routine */
#define SOCKET_ERRNO  errno   /* normal socket errno */


#else /* SSL stuff */

#include "ssl.h"

#define NETCLOSE    SSLclose
#define NETSELECT   select
#define NETBIND     SSLbind
#define NETLISTEN   SSLlisten
#define NETSOCKET   SSLsocket
#define NETCONNECT  SSLconnect
#define NETACCEPT   SSLaccept    /* accept tcp connection */

#define NETSOCK     int

#ifdef DEBUG
#define NETREAD      NET_DebugNetRead  /* read from a network socket */
#define NETWRITE     NET_DebugNetWrite /* write to a network socket */
#else
#define NETREAD      SSLread
#define NETWRITE     SSLwrite
#endif /* DEBUG */

#define NETGETSOCKNAME SSLgetsockname  /* standard get sockname routine */

/* these routines are should be the lowest level tcp
 * routines.  They are to be used by the routines in mktcp.c
 */
#define SOCKET_IOCTL  ioctl      /* normal ioctl routine for sockets */
#define SOCKET_WRITE  SSLwrite   /* write to a network socket  */
#define SOCKET_READ   SSLread    /* normal socket read routine */
#define SOCKET_ERRNO  xp_errno      /* normal socket errno */

#define SOCKET_INVALID -1   /* value that determines if a socket is inactive */

#endif /* SSL stuff */

/* tcp includes
 */
#include <sys/types.h>
#include <string.h>

#include <errno.h>    
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/file.h>  
#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#ifndef __hpux 
#include <arpa/inet.h>
#endif /* __hpux */
#include <netdb.h>

/* macros for select and stuff
 * dont do if we had <select.h>
 */
#ifndef FD_SET
typedef unsigned int fd_set;
#define FD_CLR(fd,fd_mask)     (*(fd_mask)) &= ~(1<<(fd))
#define FD_SET(fd,fd_mask)     (*(fd_mask)) |=  (1<<(fd))
#define FD_ISSET(fd,fd_mask)   (*(fd_mask)  &   (1<<(fd)))
#define FD_ZERO(fd_mask)       (*(fd_mask))=0
#endif  /* FD_SET */

/* SunOS 4.1.3 + GCC doesn't have prototypes for a lot of functions...
 */
#ifdef __sun
extern int socket (int domain, int type, int protocol);
extern int getsockname (int s, struct sockaddr *name, int *namelen);
extern int bind (int s, const struct sockaddr *name, int namelen);
extern int accept (int s, struct sockaddr *addr, int *addrlen);
extern int connect (int s, const struct sockaddr *name, int namelen);
extern int ioctl (int fildes, int request, ...);
extern int connect (int s, const struct sockaddr *name, int namelen);
extern int listen (int s, int backlog);
extern int select (int nfds, fd_set *readfds, fd_set *writefds,
		   fd_set *exceptfds, struct timeval *timeout);
extern int gethostname (char *name, int namelen);
#endif /* __sun */

#endif /* UNIXTCP_H */
