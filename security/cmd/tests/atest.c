#include "ssl.h"
#include "sec.h"
#include "xp_mcom.h"
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

/*
** Simple client/server test program for SSL library.
*/

#define PORTNUM 4567

static struct in_addr sockshost;
static int requestCertificate;

static void Error(const char *msg)
{
    printf(msg, XP_GetError());
    exit(-1);
}

static void ShowStatus(int fd)
{
    int on, keySize, secretKeySize, rv;
    char *cipher, *issuer, *subject;

    rv = SSL_SecurityStatus(fd, &on, &cipher, &keySize, &secretKeySize,
			    &issuer, &subject);
    if (rv) {
	fprintf(stderr, "security status failed, error=0x%x\n", rv);
	exit(-1);
    }
    fprintf(stderr, "Security is %s. %s key size is %d/%d.\n",
	    on ? "on" : "off", cipher, keySize, secretKeySize);
    fprintf(stderr, "Certificate issuer is '%s'\nCertificate subject is '%s'\n",
	    issuer, subject);
}

/* XXX this leaks memory */
static SECPrivateKey *GetPrivateKey(void *notused)
{
    SECPrivateKey *kp;

    kp = SEC_GetPrivateKey("TestClientKey.der", stdin, stderr);
    if (kp) {
	return kp;
    }
    fprintf(stderr, "bad key file: TestKey.der\n");
    exit(0);
}

static int GetCertificate(void *notused, SECItem *result)
{
    FILE *cf;
    char *typeTag;
    DSStatus rv;

    cf = fopen("TestClientCert.der", "r");
    if (!cf) {
	fprintf(stderr, "can't open certificate file\n");
	return -1;
    }
    typeTag = SEC_CT_CERTIFICATE;
    rv = SEC_ReadTypedData(result, cf, &typeTag);
    if (rv) {
	fprintf(stderr, "bad der in cert file\n");
	return -1;
    }
    return 0;
}

void ClientMain(int numConnections, char *hostname)
{
    int s, i;
    int rv, opt, nb;
    char pattern[100];
    struct sockaddr_in addr;
    struct hostent *hp;

    rv = SSL_SetupDefaultCertificates();
    if (rv) {
	Error("Client: setup default certificates lossage\n");
    }

    /* Lookup address */
    hp = gethostbyname(hostname);
    if (!hp) {
	Error("Client: can't find host: errno=%d\n");
    }
    XP_BZERO(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = PORTNUM;
    addr.sin_addr = *(struct in_addr*) hp->h_addr_list[0];
    printf("Client: connecting to host %s (address=%d.%d.%d.%d)\n",
	   hostname,
	   (addr.sin_addr.s_addr >> 24) & 0xff,
	   (addr.sin_addr.s_addr >> 16) & 0xff,
	   (addr.sin_addr.s_addr >> 8) & 0xff,
	   (addr.sin_addr.s_addr >> 0) & 0xff);

    for (i = 0; i < numConnections; i++) {
	sprintf(pattern, "%010d\n", lrand48());
	printf("%s", pattern);

	/* Create socket */
	s = SSL_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) {
	    Error("Client: socket: errno=%d\n");
	}
#ifdef TRACE
	printf("Client socket #%d\n", s);
#endif

	/* Configure socket */
	opt = 1;
	rv = SSL_Enable(s, SSL_SECURITY, 1);
	if (rv < 0) {
	    Error("Client: config: errno=%d\n");
	}
	rv = SSL_GetCertificateHook(s, GetCertificate, 0);
	rv = SSL_GetPrivateKeyHook(s, GetPrivateKey, 0);
	if (sockshost.s_addr != 0) {
	    rv = SSL_Enable(s, SSL_SOCKS, 1);
	    if (rv < 0) {
		Error("Client: config: errno=%d\n");
	    }
	    rv = SSL_ConfigSockd(s, sockshost.s_addr, 0);
	    if (rv < 0) {
		Error("Client: config: errno=%d\n");
	    }
	}

	/* Try to connect to the server */
	rv = SSL_Connect(s, &addr, sizeof(addr));
	if (rv < 0) {
	    Error("Client: connect: errno=%d\n");
	}
#ifdef TRACE
	printf("Client connected to server\n");
#endif
	ShowStatus(s);

	/* Send data to server */
	nb = SSL_Write(s, pattern, strlen(pattern));
	if (nb < 0) {
	    Error("ClientMain: SSLwrite: errno=%d\n");
	}
#ifdef TRACE
	printf("Client done sending to server\n");
#endif

	SSL_Close(s);
#ifdef TRACE
	printf("Client closed connection and exit\n");
#endif
    }
}

/************************************************************************/

/* XXX leaks memory */
int AuthCertificate(void *notused, SECCertificate *c,
		    SECItem *data, SECItem *sig)
{
    int rv;

    rv = SSL_AuthCertificate(notused, c, data, sig);
    if (rv) {
	fprintf(stderr,
		"server: authentication of client certificate failed (%d)\n",
		XP_GetError());
	return -1;
    }
    fprintf(stderr, "server: client cert ok, name='%s'\n",
	    SEC_NameToAscii(&c->subject));
    return 0;
}

void ServerMain(int numConnections)
{
    int i, s;
    int news;
    int rv, opt;
    SECPrivateKey *key;
    struct sockaddr_in addr;
    struct hostent *hp;
    char hname[1000];
    FILE *cf;
    SECItem certder;
    char *typeTag;

    rv = SSL_SetupDefaultCertificates();
    if (rv) {
	Error("Server: setup default certificates lossage\n");
    }

    /* Lookup address */
    rv = gethostname(hname, sizeof(hname));
    if (rv < 0) {
	Error("Server: can't get hostname: errno=%d\n");
    }
    hp = gethostbyname(hname);
    if (!hp) {
	Error("Server: can't find this host: errno=%d\n");
    }
    XP_BZERO(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = PORTNUM;
    addr.sin_addr = *(struct in_addr*) hp->h_addr_list[0];
    printf("Server: listening at (address=%d.%d.%d.%d)\n",
	   (addr.sin_addr.s_addr >> 24) & 0xff,
	   (addr.sin_addr.s_addr >> 16) & 0xff,
	   (addr.sin_addr.s_addr >> 8) & 0xff,
	   (addr.sin_addr.s_addr >> 0) & 0xff);

    key = SEC_GetPrivateKey("TestServerKey.der", stdin, stderr);
    if (!key) {
	Error("Server: unable to read server key: errno=%d\n");
    }
    cf = fopen("TestServerCert.der", "r");
    if (!cf) {
	Error("Server: unable to open server certificate\n");
    }
    typeTag = SEC_CT_CERTIFICATE;
    rv = SEC_ReadTypedData(&certder, cf, &typeTag);
    if (rv) {
	Error("Server: bad der for certificate\n");
    }
    rv = SSL_ConfigSecureServer(&certder, key);
    if (rv) {
	Error("Server: unable to configure security\n");
    }
    rv = SSL_ConfigServerSessionIDCache(0, 0, 0);
    if (rv) {
	Error("Server: unable to configure security\n");
    }

    /* Create socket */
    s = SSL_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
	Error("Server: socket: errno=%d\n");
    }
#ifdef TRACE
    printf("Server socket #%d\n", s);
#endif

    /* Configure socket */
    opt = 1;
    rv = SSL_Enable(s, SSL_SECURITY, 1);
    if (rv) {
	Error("Server: config: errno=%d\n");
    }
    if (requestCertificate) {
	rv = SSL_Enable(s, SSL_REQUEST_CERTIFICATE, requestCertificate);
	if (rv) {
	    Error("Server: enable certificate screwup\n");
	}
	rv = SSL_AuthCertificateHook(s, AuthCertificate, 0);
	if (rv) {
	    Error("Server: auth certificate screwup\n");
	}
    }

    /* Turn on socks if desired */
    if (sockshost.s_addr != 0) {
	rv = SSL_Enable(s, SSL_SOCKS, 1);
	if (rv < 0) {
	    Error("Client: config: errno=%d\n");
	}
	rv = SSL_ConfigSockd(s, sockshost.s_addr, 0);
	if (rv < 0) {
	    Error("Client: config: errno=%d\n");
	}
    }

    /* Bind our address to it */
    rv = SSL_Bind(s, &addr, sizeof(addr));
    if (rv < 0) {
	Error("Server, bind: errno=%d\n");
    }
#ifdef TRACE
    printf("Server bound address\n");
#endif

    /* Listen for a connection */
    rv = SSL_Listen(s, 1);
    if (rv < 0) {
	Error("Server, listen: errno=%d\n");
    }
#ifdef TRACE
    printf("Server listening\n");
#endif

    for (i = 0; i < numConnections; i++) {
	/* Accept connection */
#ifdef TRACE
	printf("Server waiting to accept...\n");
#endif
	news = SSL_Accept(s, 0, 0);
	if (news < 0) {
	    fprintf(stderr, "Server, accept: errno=%d\n", errno);
	    continue;
	}
#ifdef TRACE
	printf("Server accepted a connection [fd %d]\n", news);
#endif
	ShowStatus(news);

	/* Read data from client and output it */
	for (;;) {
	    int nb;
	    char buf[1025];
	    nb = SSL_Read(news, buf, sizeof(buf));
	    if (nb <= 0) {
		if (nb < 0) {
		    Error("ServerMain: SSLread: errno=%d\n");
		}
		break;
	    }
	    nb = write(1, buf, nb);
	    if (nb < 0) {
		Error("ServerMain: write: errno=%d\n");
	    }
	}
#ifdef TRACE
	printf("Server closing connection [fd %d]\n", news);
#endif
	SSL_Close(news);
    }
    SSL_Close(s);
    SEC_DestroyPrivateKey(key, 1);
}

static void Usage(void)
{
    printf("Usage: atest [-h hostname] [-c count | -s count]\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    int i;
    char hname[1000];
    struct hostent *hp;
    char *hostname = 0;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-socks") == 0) {
	    if (++i == argc) Usage();
	    hp = gethostbyname(argv[i]);
	    if (!hp) {
		Error("atest: can't find socks host: errno=%d\n");
	    }
	    sockshost = *(struct in_addr*)hp->h_addr_list[0];
	} else
	if (strcmp(argv[i], "-h") == 0) {
	    if (++i == argc) Usage();
	    hostname = argv[i];
	} else
	if (strcmp(argv[i], "-c") == 0) {
	    if (++i == argc) Usage();
	    if (!hostname) {
		if (gethostname(hname, sizeof(hname)) < 0) {
		    Error("atest: can't find our hostname: errno=%d\n");
		}
		hostname = hname;
	    }
	    ClientMain(atoi(argv[i]), hostname);
	} else if (strcmp(argv[i], "-rc") == 0) {
	    requestCertificate = 1;
	} else if (strcmp(argv[i], "-s") == 0) {
	    if (++i == argc) Usage();
	    ServerMain(atoi(argv[i]));
	} else {
	    Usage();
	}
    }
    return 0;
}
