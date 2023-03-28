/*                System dependencies in the W3 library
 *                                  SYSTEM DEPENDENCIES
 *                                           
 * System-system differences for TCP include files and macros. This
 * file includes for each system the files necessary for the network
 * 
 */

#ifndef WINTCP_H
#define WINTCP_H

#define NO_BCOPY
#include <time.h>  

#ifndef NO_SSL
#define NO_SSL
#endif

#define SOCKET_BUFFER_SIZE 4096

#ifdef NO_SSL
/*
 * These defines will be used throughout all the protocol modules
 */
#define NETCLOSE   closesocket   /* Routine to close a TCP-IP socket */
#define NETSELECT  select        /* Routine to select on TCP-IP socket */
#define NETBIND    bind          /* Routine to bind a TCP-IP socket */
#define NETLISTEN  listen        /* Routine to listen to TCP-IP socket */
#define NETSOCKET  socket        /* Routine to creat a TCP-IP socket */
#define NETCONNECT connect       /* Routine to connect to a host */
#define NETACCEPT  accept        /* Routine to accept tcp connection */
#define NETSOCK    SOCKET           /* socket type */

#define NETGETSOCKNAME getsockname /* standard sockname routine */

#define NETGETFD         /* return the file descriptor (nothing) */

#define SOCKET_INACTIVE ((SOCKET)-1)   /* value that determines if a socket is inactive */
#define SOCKET_INVALID  ((SOCKET)-1)

#ifdef DEBUG
#define NETREAD      NET_DebugNetRead  /* Routine to read from a TCP-IP socket */
#define NETWRITE     NET_DebugNetWrite /* Routine to write to a TCP-IP socket */
#else
#define NETREAD(s,b,l)      recv(s,b,l,0)              /* Routine to read from a TCP-IP socket */
#define NETWRITE(s,b,l)     send(s,b,l,0)             /* Routine to write to a TCP-IP socket */
#endif /* DEBUG */

/* these routines are should be the lowest level tcp 
 * routines.  They are to be used by the routines in mktcp.c
 */
#define SOCKET_IOCTL    ioctlsocket   /* normal ioctl routine for sockets */
#define SOCKET_WRITE(s,b,l)     send(s,b,l,0)   /* Routine to write to a TCP-IP socket  */
#define SOCKET_READ(s,b,l)      recv(s,b,l,0)    /* normal socket read routine */
#define SOCKET_ERRNO    WSAGetLastError()   /* normal socket errno */

#else /* SSL stuff */

/* These will probably need to be redone at some point */
    
#define NETCLOSE    SSLclose
#define NETSELECT   select
#define NETBIND     SSLbind
#define NETLISTEN   SSLlisten
#define NETSOCKET   SSLsocket
#define NETCONNECT  SSLconnect
#define NETACCEPT   SSLaccept    /* Routine to accept tcp connection */

#define NETSOCK     SSLSocket 

#define NETREAD     SSLread
#define NETWRITE    SSLwrite

#define NETGETSOCKNAME SSLgetsockname  /* standard get sockname routine */

/* these routines are should be the lowest level tcp
 * routines.  They are to be used by the routines in mktcp.c
 */
#define SOCKET_IOCTL  ioctlsocket /* normal ioctl routine for sockets */
#define SOCKET_WRITE  SSLwrite   /* Routine to write to a TCP-IP socket  */
#define SOCKET_READ   SSLread    /* normal socket read routine */
#define SOCKET_ERRNO  xp_errno      /* normal socket errno */

#define NETGETFD      SSLgetfd   /* return the file descriptor */

#define SOCKET_INACTIVE 0   /* value that determines if a socket is inactive */
#define SOCKET_INVALID  -1

#endif /* SSL stuff */

#define EPIPE ECONNRESET

//#define USE_BLOCKING_CONNECT
/*
 * INCLUDE FILES FOR TCP
 */
#include "winsock.h"

#endif /* TCP_H */
