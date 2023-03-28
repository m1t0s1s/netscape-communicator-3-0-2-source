/*
 * @(#)socket.c	1.29 95/12/04 Jonathan Payne
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#ifdef __linux
# include <endian.h>
#endif

#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include "socket_md.h"
#include <stdio.h>
#include "iomgr.h"

#include "java_net_Socket.h"
#include "java_net_SocketImpl.h"
#include "java_net_PlainSocketImpl.h"
#include "java_net_ServerSocket.h"
#include "java_net_SocketInputStream.h"
#include "java_net_SocketOutputStream.h"
#include "java_net_InetAddress.h"
#include "java_net_DatagramPacket.h"
#include "java_net_DatagramSocket.h"
#include "java_io_FileDescriptor.h"
#include "fd_md.h"

#define JAVANETPKG 	"java/net/"

#define BODYOF(h)   unhand(h)->body

extern long
sysSocketAvailableFD(Classjava_io_FileDescriptor * fdobj, long *pbytes);

extern void 
sysSocketInitializeFD(Classjava_io_FileDescriptor *fdptr, int infd);

extern int 
sysListenFD(Classjava_io_FileDescriptor *fdobj, long count);

extern int
sysConnectFD(Classjava_io_FileDescriptor *fdobj, struct sockaddr *him, int len);

extern int 
sysAcceptFD(Classjava_io_FileDescriptor *fdobj, struct sockaddr *him, 
int *len);

extern int 
sysCloseSocketFD(Classjava_io_FileDescriptor *fd);

extern int 
sysSendtoFD(Classjava_io_FileDescriptor * fdobj, char *buf, int len, int flags, struct sockaddr *to, int tolen);

extern int
sysRecvfromFD(Classjava_io_FileDescriptor * fdobj, char *buf, int nbytes, int flags, struct sockaddr *from, int *fromlen);

/*
 * InetAddress
 */

struct Hjava_lang_String *
java_net_InetAddress_getLocalHostName(struct Hjava_net_InetAddress *this)
{
    char    hostname[MAXHOSTNAMELEN+1];

    sysGethostname(hostname, sizeof(hostname)-1);
    return makeJavaString(hostname, strlen(hostname));
}

void 
java_net_InetAddress_makeAnyLocalAddress(struct Hjava_net_InetAddress *this, struct Hjava_net_InetAddress *address)
{
    Classjava_net_InetAddress *addr = unhand(address);
    addr->address = htonl(INADDR_ANY);
    addr->family = AF_INET;
}

long 
java_net_InetAddress_getInetFamily(struct Hjava_net_InetAddress *dummy)
{
    return AF_INET;
}

/*
 * Find an internet address for a given hostname.  Note that this
 * code only works for addresses of type INET.  Furthurmore while
 * the APIs are prepared for expansion of IP addresses to 64 bits,
 * the implementaiton, particularly around the translation of
 * "xxx.xxx.xxx.xxx" to an address, will need some work.
 */
HArrayOfByte *
java_net_InetAddress_lookupHostAddr(Hjava_net_InetAddress *dummy, HString *host)
{
    char hostname[MAXHOSTNAMELEN+1];
    char buf[MAXHOSTNAMELEN*2 + 512];
    struct hostent *hp, hpbuf;
    HArrayOfByte *ret;
    int h_error = 0;

    if (host == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    javaString2CString(host, hostname, sizeof (hostname));
    if (isdigit(hostname[0])) {
	/*
	 * This is totally bogus. inet_addr returns a 4-byte value
	 * by definition, thereby making the transition to 64-bit IP
	 * addresses much harder.  It should instead write into a
	 * user-supplied "struct in_addr"!  We'll deal with this
	 * later when 64-bit IP addresses are more of a reality -
	 * csw.
	 */
	unsigned long iaddr = inet_addr(hostname);
	if ((int)iaddr == -1) {
	    SignalError(0, JAVANETPKG "UnknownHostException", hostname);
	    return 0;
	}
	ret = (HArrayOfByte *) ArrayAlloc(T_BYTE, sizeof(iaddr));
	if (ret == 0) {
	    return 0;
	}
	memcpy((char *)BODYOF(ret), (char *)&iaddr, sizeof(iaddr));
    } else if ((hp = sysGethostbyname(hostname, &hpbuf, buf, sizeof(buf),
				      &h_error)) != NULL) {
	ret = (HArrayOfByte *) ArrayAlloc(T_BYTE, sizeof(struct in_addr));
	if (ret == 0) {
	    return 0;
	}
	memcpy((char *)BODYOF(ret), hp->h_addr, sizeof(struct in_addr));
    } else {
	SignalError(0, JAVANETPKG "UnknownHostException", hostname);
	return 0;
    }
    return (HArrayOfByte *) ret;
}

/*
 * Find an internet address for a given hostname.  Not this this
 * code only works for addresses of type INET.  Furthurmore while
 * the APIs are prepared for expansion of IP addresses to 64 bits,
 * the implementaiton, particularly around the translation of
 * "xxx.xxx.xxx.xxx" to an address, will need some work.
 */
HArrayOfArray *
java_net_InetAddress_lookupAllHostAddr(Hjava_net_InetAddress *dummy, HString *host)
{
    char hostname[MAXHOSTNAMELEN+1];
    char buf[MAXHOSTNAMELEN*2 + 512];
    struct hostent *hp, hpbuf;
    HArrayOfArray *ret;
#undef h_errno
    int h_errno = 0;

    if (host == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    javaString2CString(host, hostname, sizeof (hostname));
    if (isdigit(hostname[0])) { /* presume IP address spelling */
        HArrayOfByte *barray = java_net_InetAddress_lookupHostAddr(dummy, host);
        if (!barray)
              return 0;

        ret = (HArrayOfArray *) ArrayAlloc(T_CLASS, 1);
	if (!ret) {
	    return 0;
	}
	unhand(ret)->body[1] = (HObject*)FindClass(0, "[B", TRUE);

        unhand(ret)->body[0] = (Hjava_lang_Object *) barray;

	return ret;
    } else if ((hp = sysGethostbyname(hostname, &hpbuf, buf, sizeof(buf),
				      &h_errno)) != NULL) {
	struct in_addr **addrp = (struct in_addr **) hp->h_addr_list;
	int i = 0;

	while (*addrp != (struct in_addr *) 0) {
	    i++;
	    addrp++;
	}

        ret = (HArrayOfArray *) ArrayAlloc(T_CLASS, i);
	if (ret == 0) {
	    return 0;
	}
	unhand(ret)->body[i] = (HObject*)FindClass(0, "[B", TRUE);

	addrp = (struct in_addr **) hp->h_addr_list;
	i = 0;
	while (*addrp != (struct in_addr *) 0) {
	    HArrayOfByte *barray = (HArrayOfByte *) 
		ArrayAlloc(T_BYTE, sizeof(struct in_addr));
	    if (barray == 0) {
		return 0;
	    }
	    memcpy((char *)BODYOF(barray),
		   (char *) (*addrp), sizeof(struct in_addr));
	    unhand(ret)->body[i] = (Hjava_lang_Object *) barray;
	    addrp++;
	    i++;
	}
	return ret;
    } else {
	SignalError(0, JAVANETPKG "UnknownHostException", hostname);
	return 0;
    }
}

/*
 * Find a hostname for a given internet address.  Only the first
 * hostname is returned.
 */
HString *
java_net_InetAddress_getHostByAddr(Hjava_net_InetAddress *dummy, long addr)
{
    long netAddr;
    struct hostent hent, *hp;
    char buf[2*MAXHOSTNAMELEN + 512];
    int h_errno = 0;

    /*
     * We are careful here to use the reentrant version of
     * gethostbyname because at the Java level this routine is not
     * protected by any synchronization.
     */

    netAddr = htonl(addr);
    hp = sysGethostbyaddr((char *) &netAddr, sizeof(netAddr), AF_INET, &hent,
			  buf, sizeof(buf), &h_errno);
    if (hp == NULL) {
	SignalError(0, JAVANETPKG "UnknownHostException", NULL);
	return 0;
    }
    return makeJavaString(hp->h_name, strlen(hp->h_name));
}


/*
 * PlainSocketImpl
 */

void 
java_net_PlainSocketImpl_socketCreate(struct Hjava_net_PlainSocketImpl *this, long stream)
{
    Classjava_net_PlainSocketImpl *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    int fd;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    fd = sysSocket(AF_INET, stream ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (fd == -1) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	return;
    }
    sysSocketInitializeFD(fdptr, fd);
}


void 
java_net_PlainSocketImpl_socketConnect(struct Hjava_net_PlainSocketImpl *this, struct Hjava_net_InetAddress *address, long port)
{
    Classjava_net_PlainSocketImpl *thisptr = unhand(this);
    Classjava_net_InetAddress *addrptr;
    struct sockaddr_in him;
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (address == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    addrptr = unhand(address);

    /* connect */
    memset((char *) &him, 0, sizeof(him));
    him.sin_port = htons((short) port);
    him.sin_addr.s_addr = (unsigned long) htonl(addrptr->address);
    him.sin_family = addrptr->family;

    if (sysConnectFD(fdptr, (struct sockaddr *) &him, sizeof(him)) < 0) {
	struct execenv *ee = EE();
	if (ee && exceptionOccurred(ee)) {
	    return;
	}
	if (errno == EPROTO) {
	    SignalError(0, JAVANETPKG "ProtocolException", strerror(errno));
	} else {
	    SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	}
	return;
    }

    /* set the remote peer address and port */
    thisptr->address = address;
    thisptr->port = port;

    /* we need to intialize the local port field
       If bind was called previously to the connect (by the client) then
       localport field will already be initialized 
    */
    if (thisptr->localport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object. 
  	 */
	int len = sizeof(him);
	if (getsockname(fdptr->fd-1, (struct sockaddr *)&him, &len) == -1) {
	    SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	    return;
	}
	thisptr->localport = ntohs(him.sin_port);
    } 
}

void 
java_net_PlainSocketImpl_socketBind(struct Hjava_net_PlainSocketImpl *this, struct Hjava_net_InetAddress *address, long lport)
{
    Classjava_net_PlainSocketImpl *thisptr = unhand(this);
    Classjava_net_InetAddress *addrptr;
    struct sockaddr_in him;
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (address == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    addrptr = unhand(address);

    /* bind */
    memset((char *) &him, 0, sizeof(him));
    him.sin_port = htons((short)lport);
    him.sin_addr.s_addr = (unsigned long) htonl(addrptr->address);
    him.sin_family = addrptr->family;

    if (sysBindFD(fdptr, (struct sockaddr *)&him, sizeof(him)) == -1) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	return;
    }

    /* set the address */
    thisptr->address = address;

    /* intialize the local port */
    if (lport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object. 
  	 */
	int len = sizeof(him);
	if (getsockname(fdptr->fd - 1, (struct sockaddr *)&him, &len) == -1) {
	    SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	    return;
	}
	thisptr->localport = ntohs(him.sin_port);
    } else {
	thisptr->localport = lport;
    }
}

void 
java_net_PlainSocketImpl_socketListen(struct Hjava_net_PlainSocketImpl *this, long count)
{
    Classjava_net_PlainSocketImpl *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }
   
    if (sysListenFD(fdptr, count) == -1) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
    }
}

void 
java_net_PlainSocketImpl_socketAccept(struct Hjava_net_PlainSocketImpl *this, struct Hjava_net_SocketImpl *s)
{
    Classjava_net_PlainSocketImpl *thisptr = unhand(this);
    Classjava_io_FileDescriptor *in_fdptr = unhand(thisptr->fd);
    Classjava_net_InetAddress *addrptr;
    Classjava_net_SocketImpl *sptr;
    Classjava_io_FileDescriptor *sptr_fdptr;
    struct sockaddr_in him;
    int len = sizeof(him);
    struct execenv *ee = EE();
    int fd;

    if (in_fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (s == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    sptr = unhand(s);
    sptr_fdptr = unhand(sptr->fd);
    if ((sptr->address == 0) || (sptr_fdptr == 0)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    addrptr = unhand(sptr->address);

    fd = sysAcceptFD(in_fdptr, (struct sockaddr *)&him, &len);
    /*
     * This is a gross temporary workaround for a bug in libsocket
     * of solaris2.3.  They exit a mutex twice without entering it
     * and our monitor stuff throws an exception.  The workaround is
     * to simply ignore InternalErrors from accept.  If we get
     * an exception, and it has an exception object, and we can find
     * the class InternalError in the java pakage, and the
     * exception that was thrown was an instance of
     * InternalError, then ignore it.
     */
    if (exceptionOccurred(ee)) {
	JHandle *h = ee->exception.exc;
	if (h != NULL) {
	    ClassClass *cb;
	    cb = FindClassFromClass(ee, JAVAPKG "InternalError", TRUE, NULL);
	    if (cb != NULL && is_instance_of(h, cb, ee)) {
		exceptionClear(ee);	
	    }
	}
    }

    if (fd < 0) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	return;
    }

    sysSocketInitializeFD(sptr_fdptr, fd);

    /* fill up the remote peer port and address in the new socket structure */
    sptr->port = ntohs(him.sin_port);
    addrptr->family = him.sin_family;
    addrptr->address = ntohl(him.sin_addr.s_addr);
    /* also fill up the local port information */
    sptr->localport = thisptr->localport;
}

long
java_net_PlainSocketImpl_socketAvailable(struct Hjava_net_PlainSocketImpl *Hthis)
{
    long ret = 0;
    Classjava_net_PlainSocketImpl *this = unhand(Hthis);
    Classjava_io_FileDescriptor *fdptr = unhand(this->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return ret;
    }

    /* sysSocketAvailableFD returns 0 for failure, 1 for success */
    if (!sysSocketAvailableFD(fdptr, &ret)){
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
    }

    return ret;
}

void 
java_net_PlainSocketImpl_socketClose(struct Hjava_net_PlainSocketImpl *this)
{
    Classjava_net_PlainSocketImpl *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    sysCloseSocketFD(fdptr);
}
/*
 * SocketInputStream
 */

long 
java_net_SocketInputStream_socketRead(struct Hjava_net_SocketInputStream *this, 
				      HArrayOfByte *data, long off, long len)
{
    Classjava_net_SocketInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char *dataptr;
    int datalen, n;
 
    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return -1;
    }

    if (data == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return -1;
    }
 
    datalen = obj_length(data);
    dataptr = unhand(data)->body;
 
    if (len < 0 || len + off > datalen) {
        SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
        return 0;
    }
 
    n = sysRecvFD(fdptr, dataptr + off, len, 0);
    if (n == -1) {
        struct execenv *ee = EE();
        if (!(ee && exceptionOccurred(ee))) {
            SignalError(0,  "java/io/IOException", strerror(errno));
        }
    }
    /* AVH: this is bogus but it stops the pointer from being gc'd */
    if (dataptr == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
    }
    return n;
}

/*
 * SocketOutputStream
 */

void
java_net_SocketOutputStream_socketWrite(
	struct Hjava_net_SocketOutputStream *this,
	HArrayOfByte *data, long off, long len)
{
    Classjava_net_SocketOutputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char *dataptr;
    int datalen, n;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    datalen = obj_length(data);
    dataptr = unhand(data)->body;

    if ((len < 0) || (off < 0) || (len + off > datalen)) {
        SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
        return;
    }

    while (len > 0) {
	n = sysSendFD(fdptr, dataptr + off, len, 0);
	if (n == -1) {
	    struct execenv *ee = EE();
	    if (!(ee && exceptionOccurred(ee))) {
		SignalError(0,  "java/io/IOException", strerror(errno));
	    }
	    break;
	}
        off += n;
	len -= n;
    }
}

/* Datagram support */



void java_net_DatagramSocket_datagramSocketCreate(struct Hjava_net_DatagramSocket *this)
{
    Classjava_net_DatagramSocket *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    int fd;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    fd =  socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	return;
    }
    sysSocketInitializeFD(fdptr, fd);
}

void java_net_DatagramSocket_datagramSocketClose(struct Hjava_net_DatagramSocket *this)
{
    Classjava_net_DatagramSocket *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    sysCloseSocketFD(fdptr);
}

long java_net_DatagramSocket_datagramSocketBind(struct Hjava_net_DatagramSocket *this, long port)
{
    Classjava_net_DatagramSocket *thisptr = unhand(this);
    struct sockaddr_in lcladdr;
    int lcladdrlen= sizeof(lcladdr);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return -1;
    }

    /* bind - pick a port number for local addr*/
    memset((char *) &lcladdr, 0, sizeof(lcladdr));
    lcladdr.sin_family      = AF_INET;
    lcladdr.sin_port        = htons((short)port);
    lcladdr.sin_addr.s_addr = INADDR_ANY;

    if (sysBindFD(fdptr, (struct sockaddr *)&lcladdr, sizeof(lcladdr)) == -1) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	return -1;
    }

    /* find what port system picked for us - obviously brain dead interface,
       should have told me in the bind call itself */
    if (getsockname(fdptr->fd - 1, (struct sockaddr *)&lcladdr, &lcladdrlen) == -1) {
	SignalError(0, JAVANETPKG "SocketException", strerror(errno));
	return -1;
    }
    thisptr->localPort = ntohs(lcladdr.sin_port);
    return thisptr->localPort;
}

void
java_net_DatagramSocket_datagramSocketSend(struct Hjava_net_DatagramSocket *this,
struct Hjava_net_DatagramPacket *packet)
{
    Classjava_net_DatagramSocket *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    Classjava_net_DatagramPacket *packetptr = unhand(packet);
    Classjava_net_InetAddress *addrptr = unhand(packetptr->address);
    HArrayOfByte *data = packetptr->buf;
    char *dataptr;
    int datalen;
    int n;
    struct sockaddr_in rmtaddr;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    dataptr = unhand(data)->body;
    datalen = packetptr->length;
    rmtaddr.sin_port = htons((short)(packetptr->port));
    rmtaddr.sin_addr.s_addr = htonl(addrptr->address);
    rmtaddr.sin_family = addrptr->family;

    n = sysSendtoFD(fdptr, dataptr, datalen, 0, (struct sockaddr *)&rmtaddr, sizeof(rmtaddr));
    if (n == -1) {
      struct execenv *ee = EE();
      if (!(ee && exceptionOccurred(ee))) {
		SignalError(0, "java/io/IOException", strerror(errno));
      }
      packetptr->length = 0;
    }

    packetptr->length = n;
    return;
}

long
java_net_DatagramSocket_datagramSocketPeek(struct Hjava_net_DatagramSocket *this, struct Hjava_net_InetAddress *in)
{
    Classjava_net_DatagramSocket *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    Classjava_net_InetAddress *addrptr = unhand(in);
    int n;
    size_t	peekBufSize;
    struct sockaddr_in remote_addr;
    int remote_addrsize = sizeof (remote_addr);
    char buf[1];

    if (addrptr==0 || fdptr==0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return 0;
    }

#ifdef XP_MAC
	peekBufSize = 0;				// Mac canÕt do true data peeking.
#else
	peekBufSize = 1;
#endif

    n = sysRecvfromFD(fdptr, buf, peekBufSize, MSG_PEEK, 
		 (struct sockaddr *)&remote_addr, &remote_addrsize);

    if (n==-1) {
        SignalError(0, "java/io/IOException", 0);
        return 0;
    }
 
    addrptr->family = remote_addr.sin_family;
    addrptr->address = ntohl(remote_addr.sin_addr.s_addr);

    /* return port */
    return ntohs(remote_addr.sin_port);
}

void
java_net_DatagramSocket_datagramSocketReceive(struct Hjava_net_DatagramSocket *this, struct Hjava_net_DatagramPacket *packet)
{
    Classjava_net_DatagramSocket *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    Classjava_net_DatagramPacket *packetptr = unhand(packet);
    Classjava_net_InetAddress *addrptr=0;
    HArrayOfByte *data = packetptr->buf;
    char *dataptr;
    int datalen;
    int n;
    struct sockaddr_in remote_addr;
    int remote_addrsize = sizeof (remote_addr);

    packetptr->address = (Hjava_net_InetAddress *) execute_java_constructor(EE(),
				  "java/net/InetAddress",
				  (ClassClass *) 0, "()"); 

    if (data == 0 || (packetptr->address==0) || fdptr==0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }
    addrptr = unhand(packetptr->address);

    dataptr = unhand(data)->body;
    datalen = packetptr->length;
 
    while (1) {
      n = sysRecvfromFD(fdptr, dataptr, datalen, 0, 
		 (struct sockaddr *)&remote_addr, &remote_addrsize);
      if ((n!=-1) || (errno != EINTR)) break;
    }
    if (n == -1) {
        struct execenv *ee = EE();
        if (!(ee && exceptionOccurred(ee))) {
            SignalError(0, "java/io/IOException", strerror(errno));
        }
	packetptr->length = 0;
    } else { /* success */
      packetptr->port = ntohs(remote_addr.sin_port);
      packetptr->length = n;
      addrptr->family = remote_addr.sin_family;
      addrptr->address = ntohl(remote_addr.sin_addr.s_addr);
    }

    return;
}

/* Hook to support multicast receives for @Home */
void
java_net_DatagramSocket_multicastJoinLeave
	(struct Hjava_net_DatagramSocket *this, struct Hjava_net_InetAddress *inetaddr, long join)
{
#ifdef XP_PC
	Classjava_net_DatagramSocket *thisptr = unhand(this);
	Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
	
	int option = (join == TRUE ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP);
	
	struct ip_mreq		mreq;

    if (inetaddr == 0 || fdptr == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }

	mreq.imr_multiaddr.s_addr = htonl(unhand(inetaddr)->address);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);	  

	if ( setsockopt(fdptr->fd - 1, IPPROTO_IP, option,
					(char *) &mreq, sizeof(mreq)) == -1 )
	{
		char buf[63];
		sprintf(buf, "%i", WSAGetLastError());
		SignalError(0, JAVANETPKG "SocketException", buf);
		return;	  
	}
#endif
}

