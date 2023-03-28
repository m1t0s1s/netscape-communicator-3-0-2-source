#include "secutil.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "pk11func.h"
#include "secitem.h"

static void
Usage(const char *progName)
{
    fprintf(stderr, "Usage: %s -n nickname -p port [-d dbdir]\n", progName);
    exit(-1);
}

void
server_main(int port)
{
    int listen_sock;
    int fd;
    int rv;
    struct sockaddr_in sin;
    char buf[1000];
    int len;

    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_family = AF_INET;
    sin.sin_port = port;

    listen_sock = SSL_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
	exit(-1);
    }
    rv = SSL_Bind(listen_sock, (struct sockaddr *) &sin, sizeof(sin));
    if (rv < 0) {
	exit(-1);
    }
    rv = SSL_Enable(listen_sock, SSL_SECURITY, 1);
    if (rv < 0) {
	exit(-1);
    }
    rv = SSL_Enable(listen_sock, SSL_REQUEST_CERTIFICATE, 1);
    if (rv < 0) {
	exit(-1);
    }
    rv = SSL_Listen(listen_sock, 5);
    if (rv < 0) {
	exit(-1);
    }

    len = sizeof(sin);
    while((fd = SSL_Accept(listen_sock, (struct sockaddr *)&sin,
			   &len)) > 0) {
	do {
	    rv = SSL_Read(fd, buf, sizeof(buf));
	} while((rv == -1) && (PORT_GetError() == EWOULDBLOCK));
	SSL_Write(fd, "foo\n", 5);
	SSL_Close(fd);
    }
}

char *
get_certdb_name(void *cbarg, int dbVersion)
{
    return SECU_DatabaseFileName(xpCertDB);
}

int
main(int argc, char **argv)
{
    int o, port = 0;
    char *progName = NULL;
    char *nickname = NULL;
    CERTCertificate *cert;
    SECItem *ssl2cert;
    CERTCertificateList *ssl3chain;
    SECKEYPrivateKey *privKey;
    CERTCertDBHandle *cert_db_handle;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    while ((o = getopt(argc, argv, "d:p:n:")) != -1) {
	switch(o) {
	  default:
	  case '?':
	    Usage(progName);
	    break;

	  case 'd':
	    SECU_ConfigDirectory(optarg);
	    break;

          case 'n':
	    nickname = optarg;
	    break;

	  case 'p':
	    port = PORT_Atoi(optarg);
	}
    }

    if (nickname == NULL)
	Usage(progName);

    if (port == 0)
	Usage(progName);

    /* Call the libsec initialization routines */
    PR_Init("selfserv", 1, 1, 0);
    SECU_PKCS11Init();     
    RNG_SystemInfoForRNG();

    /* Open the server cert database */
    cert_db_handle = (CERTCertDBHandle *)PORT_ZAlloc(sizeof(CERTCertDBHandle));
    if (!cert_db_handle) {
	exit(-1);
    }

    /* Open default database, readonly false */
    CERT_OpenCertDB(cert_db_handle, PR_TRUE, get_certdb_name, NULL);

    /* XXX - this should not be required for SSL */
    CERT_SetDefaultCertDB(cert_db_handle);

    cert = PK11_FindCertFromNickname(nickname, NULL);
    if (cert == NULL) {
	fprintf(stderr, "Can't find certificate %s\n", nickname);
	exit(-1);
    }

    ssl2cert = SECITEM_DupItem(&cert->derCert);
    ssl3chain = CERT_CertChainFromCert(CERT_GetDefaultCertDB(), cert);
    privKey = PK11_FindPrivateKeyFromCert(PK11_GetInternalKeySlot(),
					  cert, NULL);

    SSL_ConfigSecureServer(ssl2cert, privKey, ssl3chain,
			   CERT_GetSSLCACerts(CERT_GetDefaultCertDB()));
    SSL_ConfigServerSessionIDCache(10, 0, 0, ".");

    server_main(port);
}
