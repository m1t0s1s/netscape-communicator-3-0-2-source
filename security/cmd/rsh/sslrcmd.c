/*
 * Copyright (c) 1983, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rcmd.c	8.3 (Berkeley) 3/26/94";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/signal.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "sslrcmd.h"
#ifdef sgi
#include <bstring.h>
#endif
#ifdef LINUX
#include <sys/time.h>
#endif

#if defined (__sun)
#undef sigmask
#define sigmask(m) (m > 32 ? 0 : ( 1 << ((m)-1)))
#endif


#define DEF_SOCKD_PORT 1080

#ifdef NO_STRERROR
char errbuf[10];

char *
strerror(int err)
{
	sprintf(errbuf, "%d", err);
	return(errbuf);
}
#endif
extern int rresvport( int *port );

static char *defaultCertDir = "/usr/etc/ssl";

static int usingSecurity = 0;

SECKEYPrivateKey *privkey = NULL;

int
ssl_ParseFlags(char *argstring, int *pencrypt, int *pproxy)
{
    int c;
    int encryptflag;
    int proxyflag;

    if ( !argstring || ( strlen(argstring) != 2 ) ) {
        return(-1);
    }

    c = argstring[0];
    if ( c == 'e' ) {
        encryptflag = SSL_ENCRYPT;
	usingSecurity = 1;
    } else {
        encryptflag = SSL_DONT_ENCRYPT;
    }

    c = argstring[1];
    if ( c == 'p' ) {
        proxyflag = SSL_PROXY;
    } else {
        proxyflag = SSL_NO_PROXY;
    }

    if ( pencrypt ) {
        *pencrypt = encryptflag;
    }

    if ( pproxy ) {
        *pproxy = proxyflag;
    }

    return( encryptflag | proxyflag );
}

static int
ssl_InitWithDirectory(const char *dir)
{
    char *newdir;
    CERTCertDBHandle *certHandle;

    if (dir) {
	newdir = SECU_ConfigDirectory(dir);
    } else {
	newdir = SECU_DefaultSSLDir();
	if (!newdir)
	    newdir = defaultCertDir;
	else
	    SECU_ConfigDirectory(newdir);
    }
    
    certHandle = (CERTCertDBHandle *)PORT_ZAlloc(sizeof(CERTCertDBHandle));
    if (!certHandle) {
	fprintf(stderr, "unable to allocate cert database handle\n");
	return -1;
    }

    /* open up database for authenticating server */
    if (CERT_OpenCertDB(certHandle, SECU_DatabaseFileName(xpCertDB),TRUE))
	if (CERT_OpenVolatileCertDB(certHandle)) {
	    fprintf(stderr, "unable to open cert database\n");
	    return -1;
	}

    CERT_SetDefaultCertDB(certHandle);

    return 0;
}

static int
ssl_SetupSockd(int fd)
{
    int rv;
    unsigned short port;
    unsigned long host;
    char *hostname;
    struct hostent *hent;
    struct servent *sp;

    sp = getservbyname("socks", "tcp");
    if (sp) {
	port = sp->s_port;
    }
    if (!port) {
	port = DEF_SOCKD_PORT;
    }

    if (hostname = getenv("SOCKS_HOST")) {
	hent = gethostbyname(hostname);
	if (hent) {
	    host = *(unsigned long *)hent->h_addr_list[0];
	} else {
	    errno = EINVAL;
	    return -1;
	}
    } else {
	errno = EINVAL;
	return -1;
    }

    rv = SSL_ConfigSockd(fd, host, port);
    return rv;
}

static int
ssl_EnableWithFlags(int fd, int createflags)
{
    int rv;

    /* set up socks and/or security as requested */
    switch (createflags & SSL_PROXY_MASK) {
        case SSL_NO_PROXY:
	    rv = 0;
	    break;
        case SSL_PROXY:
	    rv = SSL_Enable(fd, SSL_SOCKS, 1);
	    if (rv < 0)
		return rv;
	    rv = ssl_SetupSockd(fd);
	    if (rv < 0)
		return rv;
	    break;
        case SSL_SECURE_PROXY:
	    rv = SSL_Enable(fd, SSL_SECURITY, 1);
	    if (rv < 0)
		return rv;
	    rv = SSL_Enable(fd, SSL_SOCKS, 1);
	    if (rv < 0)
		return rv;
	    rv = ssl_SetupSockd(fd);
	    if (rv < 0)
		return rv;
	    break;
        default:
            PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return -1;
    }
    
    switch (createflags & SSL_ENCRYPT_MASK) {
        case SSL_ENCRYPT:
	    rv = SSL_Enable(fd, SSL_SECURITY, 1);
	    if (rv < 0)
		return rv;
	    break;
        case SSL_DONT_ENCRYPT:
	    rv = 0;
	    break;
        default:
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return -1;
    }

    rv = SSL_Enable(fd, SSL_HANDSHAKE_AS_CLIENT, 1);
    
    if (usingSecurity) {
	SSL_AuthCertificateHook(fd, SSL_AuthCertificate,
				(void *)CERT_GetDefaultCertDB());
    }

    return rv;
}


/* 
** This is our callback for client auth, when server wants our cert & key
*/
int
GetClientAuthData(void *arg, int fd, struct CERTDistNamesStr *caNames,
                      struct CERTCertificateStr **pRetCert,
                      struct SECKEYPrivateKeyStr **pRetKey)
{
    SECKEYKeyDBHandle *handle;
    CERTCertificate *cert;
    int rv, errsave;
    static PRBool dbOpen = PR_FALSE;
    SECKEYPrivateKey *key;

    if (arg == NULL) {
        fprintf(stderr, "SSL_Rcmd: no key/cert name specified for server's request for client auth\n");
        return -1;
    }

    if (dbOpen == PR_FALSE) {
	handle = SECKEY_OpenKeyDB(SECU_DatabaseFileName(xpKeyDB));
	if (handle == NULL) {
	    fprintf(stderr, "SSL_Rcmd: unable to open key database for client auth\n");
	    return -1;
	}
	dbOpen = PR_TRUE;
    }

    if (!privkey) {
	key = SECU_GetPrivateDongleKey(handle, arg,
				       SECU_ConfigDirectory(NULL));
	errsave = PORT_GetError();
	privkey = SECKEY_CopyPrivateKey(key);
    } else {
	key = SECKEY_CopyPrivateKey(privkey);
    }

    /* XXX this seems to irrevocably zero out privKeyDB  in keydb.c */
    /*
    SECKEY_CloseKeyDB();
    */

    if (!key) {
        if (errsave == SEC_ERROR_BAD_PASSWORD)
            fprintf(stderr, "Bad password\n");
        else if (errsave > 0)
            fprintf(stderr, "Unable to read key (error %d)\n", errsave);
        else if (errsave == SEC_ERROR_BAD_DATABASE)
            fprintf(stderr, "Unable to get key from database (%d)\n", errsave);
        else
            fprintf(stderr, "SECKEY_FindKeyByName: internal error %d\n", errsave);
        return -1;
    }

    cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), arg);
    if (!cert) {
        fprintf(stderr, "Unable to get certificate (%s)\n",
		SECU_ErrorString(PORT_GetError()));
        return -1;
    }

    *pRetCert = cert;
    *pRetKey = key;

    return 0;
}

int HandshakeDone(int fd, void *data)
{
    if (privkey)
	SECKEY_DestroyPrivateKey(privkey);
    privkey = NULL;
    return 0;
}

int
SSL_Rcmd(char **ahost,
	 int rport,
	 char *locuser,
	 char *remuser,
	 char *cmd,
	 int *fd2p,
	 int createflags,
	 char *certDir,
	 char *nickname)
{
    struct hostent *hp;
    struct sockaddr_in sin, from;
    fd_set reads;
    long oldmask;
    pid_t pid;
    int s, s3, lport, timo, rv;
    char c;
	
    PR_Init("rsh", 1, 1, 0);
    
    /* get all hosts and IPs squared away */
    pid = getpid();
    hp = gethostbyname(*ahost);
    if (hp == NULL) {
	fprintf(stderr, "SSL_Rcmd: gethostbyname: %s\n", *ahost);
	return (0);
    }

    sin.sin_family = hp->h_addrtype;
    PORT_Memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
    sin.sin_port = rport;

    *ahost = hp->h_name;
    oldmask = sigblock(sigmask(SIGURG));

    /* get ready to use libsec */
    SEC_Init();

    if (usingSecurity) {
	rv = ssl_InitWithDirectory(certDir);
	if (rv < 0) {
	    fprintf(stderr, "SSL_Rcmd: unable to intialize security\n");
	    sigsetmask(oldmask);
	    return 0;
	}
    }

    for (timo = 1, lport = IPPORT_RESERVED - 1;;) {
	s = rresvport(&lport);
	if (s < 0) {
	    if (errno == EAGAIN)
		(void)fprintf(stderr, "SSL_Rcmd: socket: All ports in use\n");
	    else
		(void)fprintf(stderr, "SSL_Rcmd: socket: %s\n", 
			      strerror(errno));
	    sigsetmask(oldmask);
	    return (0);
	}
#ifdef __hpux
	ioctl(s, SIOCSPGRP, pid);
#else
	fcntl(s, F_SETOWN, pid);
#endif

	/* prepare this new fd for SSL */
	rv = SSL_Import(s);
	if (rv < 0) {
	    (void)fprintf(stderr, "SSL_Rcmd: unable to import ssl sock\n");
	}

	/* set up socks and/or security as requested */
	rv = ssl_EnableWithFlags(s, createflags);
	if (rv < 0) {
	    fprintf(stderr, "SSL_Rcmd: unable to enable socket %d\n", s);
	}

	if (usingSecurity) {
	    SSL_GetClientAuthDataHook(s, GetClientAuthData,
				      (void *)nickname);
	}

	/* now connect */
	rv = SSL_Connect(s, (struct sockaddr *)&sin, sizeof(sin));
	if (rv >= 0)
	    break;

	if (PORT_GetError() == XP_ERRNO_EIO) {
	    fprintf(stderr, "SSL_Rcmd: unable to connect\n");
	    goto bad;
	}

	SSL_Close(s);

	if (errno == EADDRINUSE) {
	    lport--;
	    continue;
	}
	if (errno == ECONNREFUSED && timo <= 16) {
	    (void)sleep(timo);
	    timo *= 2;
	    continue;
	}
	if (hp->h_addr_list[1] != NULL) {
	    int oerrno = errno;
	    
	    (void)fprintf(stderr, "connect to address %s: ",
			  inet_ntoa(sin.sin_addr));
	    errno = oerrno;
	    perror(0);
	    hp->h_addr_list++;
	    memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
	    (void)fprintf(stderr, "Trying %s...\n", inet_ntoa(sin.sin_addr));
	    continue;
	}
	(void)fprintf(stderr, "%s: %s\n", hp->h_name, strerror(errno));
	sigsetmask(oldmask);
	return (0);
    }

    lport--;
    if (fd2p == 0) {
	SSL_Write(s, "", 1);
	lport = 0;
    } else {
	char num[8];
	int s2;
	int len = sizeof(from);
	struct sockaddr_in backsin;
	int sinlen = sizeof(backsin);
	
	s2 = rresvport(&lport);
	
	if (s2 < 0)
	    goto bad;
	
	/* import s2 for ssl */
	rv = SSL_Import(s2);
	if (rv < 0) {
	    fprintf(stderr, "SSL_Rcmd: unable to import control ssl\n");
	}

	rv = ssl_EnableWithFlags(s2, createflags);
	if (rv < 0) {
	    fprintf(stderr, "SSL_Rcmd: unable to enable ctrl sock %d\n", s2);
	}

	SSL_GetClientAuthDataHook(s2, GetClientAuthData,
				  (void *)nickname);

	if (usingSecurity) {
	    rv = SSL_HandshakeCallback(s2, HandshakeDone, NULL);
	    if (rv)
		fprintf(stderr, "SSL_Rcmd: handshake callback failed on %d\n", s2);
	}

	/* send a bind message to sockd if necessary */
	if ( (createflags & SSL_PROXY_MASK ) != SSL_NO_PROXY ) {
	    if (SSL_CheckDirectSock(s2) == 0) {
		rv = SSL_BindForSockd(s2, &backsin, sizeof(backsin),
				      sin.sin_addr.s_addr);
		if (rv) {
		    fprintf(stderr, "SSL_Rcmd: SSL_BindForSockd: %s\n", 
			    SECU_ErrorString(PORT_GetError()));
		    SSL_Close(s2);
		    goto bad;
		}
	    }
	}

	if( SSL_GetSockName(s2, (struct sockaddr *)&backsin, &sinlen) < 0) {
	    fprintf(stderr, "SSL_Rcmd: SSL_GetSockName: %s\n", 
			    SECU_ErrorString(PORT_GetError()));
	    SSL_Close(s2);
	    goto bad;
	}
	
	rv = SSL_Listen(s2, 1);
	if (rv < 0) {
	    fprintf(stderr, "SSL_Rcmd: SSL_Listen failed, %s",
		    SECU_ErrorString(PORT_GetError()));
	}
	(void)sprintf(num, "%u", ntohs(backsin.sin_port));
	
	/* if usingSecurity, we have to finish handshake on s before listening
	**   on next port s2
	*/
	if (usingSecurity) {
	    FD_ZERO(&reads);
	    FD_SET(s, &reads);
	    while (select(32, &reads, 0, 0, 0) > 0) {
		rv = SSL_ForceHandshake(s);
		if (rv == SECSuccess)
		    break;

		if (rv == SECFailure) {
		    fprintf(stderr, "SSL_Rcmd: unable to finish handshake on %d: %s\n", s, SECU_ErrorString(PORT_GetError()));
		    SSL_Close(s2);
		    goto bad;
		}
	    }
	}

	rv = SSL_Write(s, num, strlen(num)+1);
	if (rv != strlen(num)+1) {
	    if (PORT_GetError() == SSL_ERROR_BAD_CERTIFICATE)
		fprintf(stderr, "SSL_Rcmd: bad certificate from server\n");
	    else
		fprintf(stderr, 
			"SSL_Rcmd: (sending port number on stderr): %s\n",
			SECU_ErrorString(PORT_GetError()));
	    SSL_Close(s2);
	    goto bad;
	}
	FD_ZERO(&reads);
	FD_SET(s, &reads);
	FD_SET(s2, &reads);
	errno = 0;
	if (select(32, &reads, 0, 0, 0) < 1 || !FD_ISSET(s2, &reads)) {
	    if (errno != 0)
		(void)fprintf(stderr,
			      "SSL_Rcmd: select failed (for stderr): %s\n",
			      strerror(errno));
	    else
		(void)fprintf(stderr,
			      "SSL_Rcmd: protocol failure (data ready on %d before %d)\n", s, s2);
		SSL_Close(s2);
		goto bad;
	}
	s3 = SSL_Accept(s2, (struct sockaddr *)&from, &len);
	SSL_Close(s2);
	if (s3 < 0) {
	    (void)fprintf(stderr, "SSL_Rcmd: SSL_Accept: %s\n", 
			  SECU_ErrorString(PORT_GetError()));
	    lport = 0;
	    goto bad;
	}

	/* XXX We have to force the handshake from the client side */
	do {
	    rv = SSL_ForceHandshake(s3);
	    if (rv == -1) {
		fprintf(stderr, "SSL_Rcmd: could not finish handshake on %d\n", s3);
		SSL_Close(s3);
		goto bad;
	    }
	} while (rv == -2);

	*fd2p = s3;
	from.sin_port = ntohs((u_short)from.sin_port);
	rv = SSL_CheckDirectSock(s3);
	if ( ( !rv ) && 
	     (from.sin_family != AF_INET ||
	      from.sin_port >= IPPORT_RESERVED ||
	      from.sin_port < IPPORT_RESERVED / 2) ) {
	    (void)fprintf(stderr,
			  "socket:2: protocol failure in circuit setup.\n");
	    goto bad2;
	}
    }
    rv = SSL_Write(s, locuser, strlen(locuser)+1);
    if (rv >= 0)
	rv = SSL_Write(s, remuser, strlen(remuser)+1);
    if (rv >= 0)
	rv = SSL_Write(s, cmd, strlen(cmd)+1);
    if (rv < 0) {
	(void)fprintf(stderr, "SSL_Rcmd: error writing to ssl %d\n", s);
    }

    if (SSL_Read(s, &c, 1) != 1) {
	fprintf(stderr, "SSL_Rcmd: %s: %s\n", *ahost, 
		SECU_ErrorString(PORT_GetError()));
	goto bad2;
    }
    /* rshd returned an error */
    if (c != 0) {
	while (rv = SSL_Read(s, &c, 1)) {
	    if (rv < 0) {
		if (PORT_GetError() == EWOULDBLOCK) {
		    continue;
		} else {
		    fprintf(stderr, "SSL_Rcmd: SSL_Read: %s\n",
			    SECU_ErrorString(PORT_GetError()));
		    goto bad2;
		}
	    }
	    (void)write(STDERR_FILENO, &c, 1);
	    if (c == '\n')
		break;
	}
	goto bad2;
    }
    sigsetmask(oldmask);
    return (s);
bad2:
    if (lport)
	SSL_Close(s3);
    if (privkey) {
	SECKEY_DestroyPrivateKey(privkey);
	privkey = NULL;
    }
bad:
    SSL_Close(s);
    sigsetmask(oldmask);
    if (privkey)
	SECKEY_DestroyPrivateKey(privkey);
    return (0);
}

