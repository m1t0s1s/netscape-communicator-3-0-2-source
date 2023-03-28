/*
** Copyright (c) 1995.  Netscape Communications Corporation.  All rights
** reserved.  This use of this Secure Sockets Layer Reference
** Implementation (the "Software") is governed by the terms of the SSL
** Reference Implementation License Agreement.  Please read the
** accompanying "License" file for a description of the rights granted.
** Any other third party materials you use with this Software may be
** subject to additional license restrictions from the licensors of such
** third party software and/or additional export restrictions.  The SSL
** Implementation License Agreement grants you no rights to any such
** third party material.
**
** Sample client side test program that uses SSL and libsec
**
*/

#include "secutil.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>

#ifdef AIX
#include <sys/select.h>
#endif

static char *progName;

static void Usage(const char *progName)
{
    printf("Usage:  %s -h host -p port [-d certdir] [-n nickname]\n", progName);
    printf("%-20s Hostname to connect with\n", "-h host");
    printf("%-20s Port number for SSL server\n", "-p port");
    printf("%-20s Directory with cert database (default is ~/.netscape)\n",
	  "-d certdir");
    printf("%-20s Nickname of key and cert for client auth\n", "-n nickname");
    exit(-1);
}

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    struct hostent *hp;
    int s, o, maxfd;
    SECStatus rv;
    fd_set rd;
    CERTCertDBHandle *handle;
    char *host, *port, *certDir, *nickname;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    port = host = certDir = nickname = NULL;

    while ((o = getopt(argc, argv, "h:p:d:n:")) != EOF) {
	switch (o) {
	  case '?':
	    Usage(progName);

          case 'h':
	    host = strdup(optarg);
	    break;

	  case 'p':
	    port = strdup(optarg);
	    break;

	  case 'd':
	    certDir = strdup(optarg);
	    SECU_ConfigDirectory(certDir);
	    break;

	  case 'n':
	    nickname = strdup(optarg);
	    break;
	}
    }

    if (!host || !port) Usage(progName);

    if (!certDir) {
	certDir = SECU_DefaultSSLDir();
	if (certDir)
	    SECU_ConfigDirectory(certDir);
    }

    PR_Init("tstclnt", 1, 1, 0);
    SECU_PKCS11Init();
    
    /* Lookup host */
    hp = gethostbyname(host);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr = *(struct in_addr*) hp->h_addr_list[0];
    printf("%s: connecting to %s:%d (address=%d.%d.%d.%d)\n",
	   progName, host, ntohs(addr.sin_port),
	   (addr.sin_addr.s_addr >> 24) & 0xff,
	   (addr.sin_addr.s_addr >> 16) & 0xff,
	   (addr.sin_addr.s_addr >> 8) & 0xff,
	   (addr.sin_addr.s_addr >> 0) & 0xff);

    /* Create socket */
    s = SSL_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
	SECU_PrintError(progName, "error creating socket");
	return -1;
    }

    /* Initialize all the libsec goodies */
    SEC_Init();

    rv = SSL_Enable(s, SSL_SECURITY, 1);
    if (rv < 0) {
        SECU_PrintError(progName, "error enabling socket");
	return -1;
    }


    rv = SSL_Enable(s, SSL_HANDSHAKE_AS_CLIENT, 1);
    if (rv < 0) {
	SECU_PrintError(progName, "error enabling client handshake");
	return -1;
    }

    handle = (CERTCertDBHandle *)PORT_ZAlloc(sizeof(CERTCertDBHandle));
    if (!handle) {
	SECU_PrintError(progName, "could not allocate database handle");
        return -1;
    }
   
    /* Open up the certificate database */
    rv = CERT_OpenCertDBFilename(handle, SECU_DatabaseFileName(xpCertDB), 
				 TRUE);
    if ( rv ) {
	SECU_PrintError(progName, "unable to open cert database");
        rv = CERT_OpenVolatileCertDB(handle);
    }

    CERT_SetDefaultCertDB(handle);
    SSL_AuthCertificateHook(s, SSL_AuthCertificate, (void *)handle);
    SSL_GetClientAuthDataHook(s, SECU_GetClientAuthData, (void *)nickname);

    /* Try to connect to the server */
    rv = SSL_Connect(s, (struct sockaddr*) &addr, sizeof(addr));
    if (rv < 0) {
	SECU_PrintError(progName, "unable to connect");
	return -1;
    }

    /*
    ** Select on stdin and on the socket. Write data from stdin to
    ** socket, read data from socket and write to stdout.
    */
    memset(&rd, 0, sizeof(rd));
    FD_SET(0, &rd);
    FD_SET(s, &rd);
    maxfd = s;

    printf("%s: ready...\n", progName);
    while (FD_ISSET(s, &rd)) {
	fd_set r;
	char buf[4000];
	int nb;

	r = rd;
	rv = select(maxfd+1, &r, 0, 0, 0);
	if (rv < 0) {
	    SECU_PrintSystemError(progName, "select failed");
	    goto done;
	}
	if (FD_ISSET(0, &r)) {
	    /* Read from stdin and write to socket */
	    nb = read(0, buf, sizeof(buf));
	    printf("%s: stdin head %d bytes\n", progName, nb);
	    if (nb < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
		    /* Try again later... */
		} else {
		    SECU_PrintSystemError(progName, "read from stdin failed");
		    goto done;
		}
	    }
	    if (nb == 0) {
		FD_CLR(0, &rd);
	    } else {
		printf("%s: Writing %d bytes to server\n", progName, nb);
		(void) SSL_Write(s, buf, nb);
	    }
	}

	if (FD_ISSET(s, &r)) {
	    /* Read from socket and write to stdout */
	    nb = SSL_Read(s, buf, sizeof(buf));
	    printf("%s: Read from server %d bytes\n", progName, nb);
	    if (nb < 0) {
		if (PORT_GetError() == EWOULDBLOCK)
		    continue;
		SECU_PrintError(progName, "read from socket failed");
		goto done;
	    }
	    if (nb == 0) {
		/* EOF from socket... bye bye */
		FD_CLR(s, &rd);
	    } else
		(void) write(1, buf, nb);
	}
    }

  done:
    SSL_Close(s);
    return 0;
}
