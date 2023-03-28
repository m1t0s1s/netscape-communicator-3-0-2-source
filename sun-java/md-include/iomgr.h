#ifndef sun_java_iomgr_h___
#define sun_java_iomgr_h___

#include "nspr_md.h"
#include "prfile.h"

#if defined(XP_UNIX)
#include <sys/socket.h>
#include <netdb.h>

#elif defined(XP_PC)
#include <winsock.h>

#elif defined(XP_MAC)
#include "prnetdb.h"
#include "macsock.h"
#endif

#define IO_DONTBLOCK 0
#define IO_BLOCK 1
#define nonblock_io(a,b) PR_NonBlockIO(a,b)

#if defined(XP_UNIX)
extern int sysConnect(int, struct sockaddr*, int);
extern int sysAccept(int, struct sockaddr*, int*);
extern int sysRecv(int, void *, int, int);
extern int sysSend(int, const void *, int, int);
extern int sysRecvFrom(int fd, void *buf, int len, int flags,
		       struct sockaddr *from, int *fromlen);
extern int sysSendTo(int fd, const void *buf, int len, int flags,
		     struct sockaddr *to, int tolen);

#define sysBind(a,b,c)			bind(a,b,c)
#define sysListen(a,b)			listen(a,b)
#define sysSocket(a,b,c)		socket(a,b,c)

#elif defined(XP_PC)
#define sysConnect(a,b,c)		connect(a,b,c)
#define sysAccept(a,b,c)		accept(a,b,c)
#define sysRecv(a,b,c,d)		recv(a,b,c,d)
#define sysSend(a,b,c,d)		send(a,b,c,d)
#define sysRecvFrom(a,b,c,d,e,f)	recvfrom(a,b,c,d,e,f)
#define sysSendTo(a,b,c,d,e,f)		sendto(a,b,c,d,e,f)

#define sysBind(a,b,c)			bind(a,b,c)
#define sysListen(a,b)			listen(a,b)
#define sysSocket(a,b,c)		socket(a,b,c)

#elif defined(XP_MAC)
#define sysAccept			macsock_accept
#define sysBind				macsock_bind
#define sysConnect			macsock_connect
#define	sysIOCtl			macsock_ioctl
#define sysListen			macsock_listen
#define sysRecv				macsock_recv
#define sysRecvFrom			macsock_recvfrom
#define sysSend				macsock_send
#define sysSendTo			macsock_sendto
#define sysSocket			macsock_socket
#endif 


extern long sysSocketAvailable(int, long *);

#define sysGethostname(a,b)		gethostname(a,b)
#define sysGethostbyname(a,b,c,d,e)	PR_gethostbyname(a,b,c,d,e)
#define sysGethostbyaddr(a,b,c,d,e,f,g)	PR_gethostbyaddr(a,b,c,d,e,f,g)

#endif /* sun_java_iomgr_h___ */
