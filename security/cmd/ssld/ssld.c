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
*/
#include <sys/wait.h>

#if !defined(LINUX)
#include <sys/filio.h>
#endif

#include <sys/resource.h>
#include <netinet/tcp.h>
#include "ssld.h"
#include "secutil.h"

extern int getopt(int, char *const*, const char*);
extern char *optarg;
extern int optind;
extern int rresvport( int *port );

static char *defaultKeyDir = "/usr/etc/ssl";
static char *defaultConfFile = "ssld.conf";
static SECCertDBHandle *cert_db_handle;

char *progName;
int dbg, interactive = 1;

static int nfd;
static int max_files = 32;
static fd_set work;


#ifdef __alpha
void DebugPrint(va_alist)
va_dcl
{
    va_list ap;
    char *fmt;

    if (dbg) {
	va_start(ap);
	fmt = va_arg(ap, char *);
	if (interactive) {
	    fprintf(stderr, "%s: ", progName);
	    vfprintf(stderr, fmt, ap);
	    if (strrchr(fmt, '\n') == 0) {
		fprintf(stderr, "\n");
	    }
	} else {
	    char *buf = malloc(50000);
	    vsprintf(buf, fmt, ap);
	    syslog(LOG_DEBUG, buf);
	    free(buf);
	}
	va_end(ap);
    }
}

void Notice(va_alist)
va_dcl
{
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    if (interactive) {
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, fmt, ap);
	if (strrchr(fmt, '\n') == 0) {
	    fprintf(stderr, "\n");
	}
    } else {
	char *buf = malloc(50000);
	vsprintf(buf, fmt, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
    }
    va_end(ap);
}

void Error(char *va_alist, ...)
{
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    if (interactive) {
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, fmt, ap);
	if (strrchr(fmt, '\n') == 0) {
	    fprintf(stderr, "\n");
	}
    } else {
	char *buf = malloc(50000);
	vsprintf(buf, fmt, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
    }
    va_end(ap);
}

void Fatal(char *va_alist, ...)
{
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);

    if (interactive) {
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, fmt, ap);
	if (strrchr(fmt, '\n') == 0) {
	    fprintf(stderr, "\n");
	}
    } else {
	char *buf = malloc(50000);
	vsprintf(buf, fmt, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
    }
    va_end(ap);
    exit(-1);
}
#else
void DebugPrint(char *va_alist, ...)
{
    va_list ap;

    if (dbg) {
#ifdef va_dcl
	va_start(ap);
#else
	va_start(ap, va_alist);
#endif
	if (interactive) {
	    fprintf(stderr, "%s: ", progName);
	    vfprintf(stderr, va_alist, ap);
	    if (strrchr(va_alist, '\n') == 0) {
		fprintf(stderr, "\n");
	    }
	} else {
#ifdef va_dcl
	    char *buf = malloc(50000);
	    vsprintf(buf, va_alist, ap);
	    syslog(LOG_DEBUG, buf);
	    free(buf);
#else
	    vsyslog(LOG_DEBUG, va_alist, ap);
#endif
	}
	va_end(ap);
    }
}

void Notice(char *va_alist, ...)
{
    va_list ap;

#ifdef va_dcl
    va_start(ap);
#else
    va_start(ap, va_alist);
#endif
    if (interactive) {
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, va_alist, ap);
	if (strrchr(va_alist, '\n') == 0) {
	    fprintf(stderr, "\n");
	}
    } else {
#ifdef va_dcl
	char *buf = malloc(50000);
	vsprintf(buf, va_alist, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
#else
	vsyslog(LOG_NOTICE, va_alist, ap);
#endif
    }
    va_end(ap);
}

void Error(char *va_alist, ...)
{
    va_list ap;

#ifdef va_dcl
    va_start(ap);
#else
    va_start(ap, va_alist);
#endif
    if (interactive) {
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, va_alist, ap);
	if (strrchr(va_alist, '\n') == 0) {
	    fprintf(stderr, "\n");
	}
    } else {
#ifdef va_dcl
	char *buf = malloc(50000);
	vsprintf(buf, va_alist, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
#else
	vsyslog(LOG_ERR, va_alist, ap);
#endif
    }
    va_end(ap);
}

void Fatal(char *va_alist, ...)
{
    va_list ap;

#ifdef va_dcl
    va_start(ap);
#else
    va_start(ap, va_alist);
#endif
    if (interactive) {
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, va_alist, ap);
	if (strrchr(va_alist, '\n') == 0) {
	    fprintf(stderr, "\n");
	}
    } else {
#ifdef va_dcl
	char *buf = malloc(50000);
	vsprintf(buf, va_alist, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
#else
	vsyslog(LOG_ERR, va_alist, ap);
#endif
    }
    va_end(ap);
    exit(-1);
}
#endif

int
checkACL(char *name, Service *s)
{
    ACLNode *acl;

    acl = s->acl;
    
    if ( acl ) {
	while ( acl ) {
	    if ( strcmp(name, acl->name) == 0 ) {
		/* match */
		return(0);
	    }
	    acl = acl->next;
	}
	return(-1);
    } else {
	return(0);
    }
}

static void Shutdown(Service *s, Todo *t, Sock *psock, fd_set *pfds)
{
    Todo **tp = &s->todo;
    Todo *n;
    Sock *pcursock, *plastsock;
    int waslast;

    DebugPrint("shutdown connection on port %d using %d,%d",
	  s->port, psock->sslfd, psock->clearfd);

    SSL_Close(psock->sslfd);
    close(psock->clearfd); 
    FD_CLR(psock->clearfd, pfds);
    FD_CLR(psock->sslfd, pfds);

    waslast = 0;

    /* unline psock from the sockets list */
    plastsock = 0;
    pcursock = t->sockets;
    while ( pcursock ) {
	if ( pcursock == psock ) {
	    if ( plastsock ) {
		/* not head of list */
		plastsock->next = pcursock->next;
	    } else {
		t->sockets = pcursock->next;
		if ( pcursock->next == 0 ) {
		    /* only element on list */
		    waslast = 1;
		}
	    }
	    free(pcursock);
	    break;
	}
	plastsock = pcursock;
	pcursock = plastsock->next;
    }

    /* if the above loop fell through then we were called incorrectly */
    if ( !pcursock ) {
	Fatal("Shutdown called with bad psock");
    }

    /* The last sock of this todo was closed, so get rid of todo struct */
    if ( waslast ) {
	DebugPrint("final shutdown of connection on port %d\n", s->port);
	while ((n = *tp) != 0) {
	    if (n == t) {
		if ( t->controlfd != -1 ) {
		    close( t->controlfd );
		    FD_CLR(t->controlfd, pfds);
		}
		*tp = t->next;
		free(t);
		return;
	    }
	    tp = &n->next;
	}
    }
}

/*
** Forward data from s2 to host specified in s. If asServer is true, then
** s2 is sslfd. If client, then s2 is clearfd.
*/

static Todo *SetupForwarder(Service *s, int s2, int asServer)
{
    Todo *t;
    int s3, rv;
    u_long addr;
    Sock *psock;

    /* Setup forwarding agent */
    s3 = socket(AF_INET, SOCK_STREAM, 0);
    if (s3 < 0) {
	Error("socket create failed: %s", strerror(errno));
	return 0;
    }

    /* Connect to forwarding address */
    rv = connect(s3, (struct sockaddr*)&s->forward, sizeof(s->forward));
    if (rv < 0) {
	addr = ntohl(s->forward.sin_addr.s_addr);
	Error("forwarder connect failed to %d.%d.%d.%d:%d %s",
	      (addr >> 24) & 0xff,
	      (addr >> 16) & 0xff,
	      (addr >> 8) & 0xff,
	      (addr >> 0) & 0xff,
	      ntohs(s->forward.sin_port),
	      strerror(errno));
	SSL_Close(s2);
	close(s3);
	return 0;
    }

    addr = ntohl(s->forward.sin_addr.s_addr);
    DebugPrint("forwarding %s data to %d.%d.%d.%d:%d using %d",
	  asServer ? "clear" : "ssl",
	  (addr >> 24) & 0xff,
	  (addr >> 16) & 0xff,
	  (addr >> 8) & 0xff,
	  (addr >> 0) & 0xff,
	  ntohs(s->forward.sin_port), s3);

    /* Ready to go... */
    t = (Todo*) calloc(1, sizeof(Todo));
    t->next = 0;
    psock = (Sock*) calloc(1, sizeof(Sock));
    if (asServer) {
	psock->clearfd = s3;
	psock->sslfd = s2;
    } else { /* XXX this does not work */
	psock->clearfd = s2;
	psock->sslfd = s3;
    }
    psock->next = 0;

    t->sockets = psock;
    t->controlfd = -1;

    return t;
}

int HandshakeCallback(int fd, void *data)
{
    Todo *t;
    Callback *callback;
    Service *s;
    Sock *psock;
    char *peercn;
    SECCertificate *cert;

    callback = data;
    s = callback->service;
    t = callback->todo;
    psock = callback->sock;

    DebugPrint("handshake finished on %d:", fd);
    DebugPrint("\tclear:   %d", t->sockets->clearfd);
    if (t->controlfd != -1)
	DebugPrint("\tcontrol: %d", t->controlfd);

    if (s->auth) {
	cert = SSL_PeerCertificate(fd);
	if (!cert) {
	    Error("Could not obtain peer certificate");
	    nfd = -1;
	    Shutdown(s, t, psock, &work);
	    free(callback);
	    return -1;
	}
	peercn = SEC_GetCommonName(&cert->subject);
	if( peercn ) {
	    DebugPrint("peer authenticated as %s", peercn);
	}
	else {
	    DebugPrint("peer not authenticated");
	}
	
	if ( checkACL(peercn, s) ) {
	    Error("%s does not have access to port %d", peercn, s->port);
	    nfd = -1;
	    Shutdown(s, t, psock, &work);
	    free(callback);
	    return -1;
	}
    }

    /* check to see if this todo has finished handshake */
    FD_SET(t->sockets->clearfd, &work);
    if (t->sockets->clearfd > nfd) {
	nfd = t->sockets->clearfd;
    }
    if ( t->controlfd != -1 ) {
	FD_SET(t->controlfd, &work);
	if (t->controlfd > nfd) {
	    nfd = t->controlfd;
	}
    }

    free(callback);
    return 0;
}


/*
** Connect as a client to an ssl server out there somewhere.
*/
static Todo *ConnectClient(Service *s, int s2)
{
    Todo *t;

    /* Setup forwarding agent, calling internal function */
    t = SetupForwarder(s, s2, SSLD_CLIENT);
    return t;
}

/*
** Connect as a server to an ssl client out there somewhere.
*/
static Todo *ConnectServer(Service *s, int s2)
{
    Todo *t;
    int sv[2], rv, pid, val;
    int ctrlsv[2];
    Sock *psock;
    
    switch (s->action) {
      case FORWARDER:
	t = SetupForwarder(s, s2, SSLD_SERVER);
	break;

      case EXECER:
	/* Create socketpair to talk with child */
	rv = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	if (rv < 0) {
	    /*
	    ** This might work later so just abandon the new connection
	    */
	    Error("socketpair failed: %s", strerror(errno));
	    SSL_Close(s2);
	    return 0;
	}
	/* create a socketpair for the control connection */
	rv = socketpair(AF_UNIX, SOCK_STREAM, 0, ctrlsv);
	if (rv < 0) {
	    /*
	    ** This might work later so just abandon the new connection
	    */
	    Error("control socketpair failed: %s", strerror(errno));
	    SSL_Close(s2);
	    close(sv[0]);
	    close(sv[1]);
	    return 0;
	}
	val = 1;
	setsockopt(sv[0], SOL_SOCKET, SO_KEEPALIVE, (void*)&val, sizeof(val));
	setsockopt(ctrlsv[0], SOL_SOCKET, SO_KEEPALIVE, (void*)&val, sizeof(val));

	/* Fork child to process clear connection */
	pid = fork();
	if (pid < 0) {
	    /*
	    ** Fork's might work later, so just abandon the new
	    ** connection.
	    */
	    Error("fork failed: %s", strerror(errno));
	    close(sv[0]);
	    close(sv[1]);
	    close(ctrlsv[0]);
	    close(ctrlsv[1]);
	    SSL_Close(s2);
	    return 0;
	}
	if (pid == 0) {
	    int i;
	    /* Child */
	    dup2(sv[0], 0);
	    dup2(sv[0], 1);
	    dup2(sv[0], 2);
	    dup2(ctrlsv[0], 3); /* control socket is fd 3 */
	    for (i = 4; i < max_files; i++) {
		close(i);
	    }
	    signal(SIGPIPE, SIG_DFL);
	    execv(s->file, s->avp);
	    Error("execv failed: %s", strerror(errno));
	    exit(-1);
	}

	/* Parent */
	close(sv[0]);
	close(ctrlsv[0]);

	DebugPrint("feeding clear data to %s (pid %d) using %d",
	      s->file, pid, sv[1]);

	/* Create Todo item to shovel the i/o around */
	t = (Todo *) calloc(1, sizeof(Todo));
	psock = (Sock *) calloc(1, sizeof(Sock));
	t->controlfd = ctrlsv[1];
	DebugPrint("control fd is %d", t->controlfd);
	t->cbufoff = 0;
	t->sockets = psock;

	psock->clearfd = sv[1];
	psock->sslfd = s2;
	break;
    }
    return t;
}


/* This just waits for child process to change state */
static void Reaper(int i)
{
    int s;

    while (waitpid(-1, &s, WNOHANG) > 0)
	;
}


/* Initialize the key & certificate database stuff */
void ssld_init(Service *s, char *keyDir)
{
    SECKeyDBHandle *keydb;
    SECPrivateKey *key;
    SECCertificate *cert;
    int errsave;
    int rv;

    SEC_Init();

    /* Set up default directory for databases */
    keyDir = SECU_ConfigDirectory(keyDir);

    /* Open the key pair database */
    keydb = SEC_OpenKeyDB(SECU_DatabaseFileName(xpKeyDB));
    if (keydb == NULL) {
	Fatal("Unable to read key file (%s)", SECU_ErrorString(XP_GetError()));
    }
    SEC_SetDefaultKeyDB(keydb);

    /*
    ** Get the server private key from the database, prompting for
    ** the password.
    */
    key = SECU_GetPrivateDongleKey(keydb, s->keyname, keyDir);
    errsave = XP_GetError();

    /* Handle failure to get key, if any */
    if (!key) {
	if (errsave == SEC_ERROR_BAD_PASSWORD)
	    Fatal("Bad password");
	else if (errsave > 0)
	    Fatal("Unable to read key file (%s)", SECU_ErrorString(errsave));
	else if (errsave == SEC_ERROR_BAD_DATABASE)
	    Fatal("Unable to get key from database (%s)", SECU_ErrorString(errsave));
	else
	    Fatal("%s", SECU_ErrorString(errsave));
    }

    /* Open the server cert database */
    cert_db_handle = (SECCertDBHandle *)DS_Zalloc(sizeof(SECCertDBHandle));
    if (!cert_db_handle) {
	Fatal("Could not allocate database handle");
    }

    /* XXX - this should not be required for SSL */
    SEC_SetDefaultCertDB(cert_db_handle);

    /*rv = SEC_OpenCertDB(cert_db_handle, SECU_DatabaseFileName(xpCertDB), TRUE  );*/

    rv = SEC_OpenCertDB(cert_db_handle, NULL, FALSE);  /* Open default database, readonly false */

    if (rv)
	Fatal("Open of server cert database failed (%s)", 
	      SECU_ErrorString(XP_GetError()));

    /* Read the server cert from the database */
    cert = SEC_FindCertByNickname(cert_db_handle, s->certname);
    if (!cert)
	Fatal("Could not read server certificate (%s)",
	      SECU_ErrorString(XP_GetError()));

    rv = SEC_CertTimesValid(cert);
    if (rv)
	Fatal("Server certificate is not valid");

    rv = SSL_ConfigSecureServer(&cert->derCert, key,
				SEC_CertChainFromCert(cert_db_handle, cert),
				SEC_GetSSLCACerts(cert_db_handle));
    if (rv)
	Fatal("Server key/certificate is bad (%s)", 
	      SECU_ErrorString(XP_GetError()));

    rv = SSL_ConfigServerSessionIDCache(0, 0, 0, 0);
    if (rv) {
	if (XP_GetError() == ENOSPC)
	    Fatal("Config of server nonce cache failed, "
		  "out of disk space! Make more room in /tmp "
		  "and try again.");
	else
	    Fatal("Config of server nonce cache failed (%s)", 
		  SECU_ErrorString(XP_GetError()));
    }

    SEC_DestroyPrivateKey(key);
}

static void Usage(char *progName)
{
    printf("Usage:  %s [-d keydir] [-i] [-D] [-c conffile] [-C chrootdir]\n",
           progName);
    printf("%-20s directory for the key & certficate databases and\n",
           "-d keydir");
    printf("%-20s \tconfiguration file (default is %s)\n", "", defaultKeyDir);
    printf("%-20s run in interactive mode (don't fork)\n",
           "-i");
    printf("%-20s enable debugging\n",
           "-D");
    printf("%-20s specify the config file (default is %s)\n",
           "-c conffile", defaultConfFile);
    printf("%-20s chroot to the named directory\n",
           "-C chrootdir");
    exit(-1);
}

void main(int argc, char **argv)
{
    char *newRoot, *keyDir, *conf;
    int o, sfd, rv, bi;
    Service *s, *servs;
    int shutdown;
    Sock *psock, *nsock;
    Callback *callback;
    int lport;
    short sport;
    int fd;
    struct rlimit rlp;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    bi = 0;
    newRoot = 0;
    keyDir = 0;
    conf = 0;

    while ((o = getopt(argc, argv, "c:C:Did:")) != EOF) {
	switch (o) {
	  case '?':
	    Usage(progName);

	  case 'C':
	    newRoot = strdup(optarg);
	    break;

	  case 'c':
	    conf = strdup(optarg);
	    break;

	  case 'D':
	    dbg = 1;
	    bi = 1;
	    break;

	  case 'i':
	    bi = 1;
	    break;

	  case 'd':
	    keyDir = strdup(optarg);
	    break;
	}
    }

    /* If no keyDir specified, get environ variable or set to default */
    if (!keyDir) {
        keyDir = SECU_DefaultSSLDir();
	if (!keyDir)
	    keyDir = defaultKeyDir;
    }

    /* If no conf specified, set to environ variable or to default */
    if (!conf) {
        conf = getenv("SSLD_CONF");
	if (!conf) { 
	    conf = SECU_AppendFilenameToDir(keyDir, defaultConfFile);
	}
	else if (conf[0] == '/')
	    ;
	else {
	    conf = SECU_AppendFilenameToDir(keyDir, conf);
	}
    }
    /* else conf was specified in command line */
    else { 
	if (conf[0] == '/')
	    ;
	else
	     conf = SECU_AppendFilenameToDir(keyDir, conf);
    }

    if (newRoot) {
	rv = chroot(newRoot);
	if (rv < 0) {
	    Fatal("chroot to %s failed", newRoot);
	}
    }

    /* initialize PR before ReadConfFile, since it calls gethostbyname */
    PR_Init("ssld", 1, 1, 0);

    rv = getrlimit(RLIMIT_NOFILE, &rlp);
    if (rv >= 0) {
	max_files = rlp.rlim_cur;
	Notice("Max file limit: %d\n", max_files);
    }

    DebugPrint("using database directory: %s", keyDir);
    /*
    ** Process config file
    */
    servs = ReadConfFile(conf);
    if (!servs) {
	Notice("unable to parse config file %s", conf);
	exit(-1);
    }

    /*
    ** Initialize libsec and certificate/key database 
    ** XXX This is ONLY using the first Service struct returned by
    ** ReadConfFile above. Need to prompt for all passwords in beginning
    ** during ssld_init, not just first one. /dk
    */ 
    ssld_init(servs, keyDir);

    /*
    ** Create sockets for listening
    */
    FD_ZERO(&work);
    nfd = 0;
    for (s = servs; s; s = s->next) {
	struct sockaddr_in sin;
	int s1, val;

   	/* Setup sockets for listening */

	s1 = SSL_Socket(AF_INET, SOCK_STREAM, 0);
	if (s1 < 0) Fatal("socket creation failed: %s",
			  SECU_ErrorString(XP_GetError()));
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_family = AF_INET;
	sin.sin_port = s->port;
	val = 1;
	SSL_SetSockOpt(s1, SOL_SOCKET, SO_REUSEADDR, (void*)&val, sizeof(val));

	rv = SSL_Enable(s1, SSL_SECURITY, 1);
	if (rv < 0) {
	    Fatal("%s: could not enable ssl socket", progName);
	} else {
	    DebugPrint("%s: enabled security on %d", progName, s1);
	}

	rv = SSL_Enable(s1, SSL_HANDSHAKE_AS_SERVER, 1);
	if (rv < 0) {
	    Fatal("%s: could not enable handshake as server", progName);
	}

	rv = SSL_Bind(s1, (struct sockaddr*) &sin, sizeof(sin));
	if (rv < 0) Fatal("socket bind to %d failed: %s",
			  ntohs(s->port), SECU_ErrorString(XP_GetError()));
	rv = SSL_Listen(s1, 5);
	if (rv < 0) Fatal("socket listen failed: %s",
			  SECU_ErrorString(XP_GetError()));
	s->listen = s1;
	FD_SET(s1, &work);
	if (s1 > nfd) nfd = s1;

	/* Set the certificate */
        rv = SSL_AuthCertificateHook(s1, SSL_AuthCertificate,
				     (void *)cert_db_handle);
	if (rv)
	    DebugPrint("%s: could not set certificate auth hook", progName);
    }
    sfd = nfd;

    interactive = bi;
    if (!interactive) {
	int pid;

	openlog(progName, LOG_PID, LOG_DAEMON);
	pid = fork();
	if (pid < 0) Fatal("fork failed: %s", SECU_ErrorString(errno));
	if (pid != 0) {
	    /* Parent exits */
	    exit(0);
	}

	setsid();
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCLD, Reaper);

    /*
    ** Run daemon loop waiting for a connections to come in and servicing
    ** i/o operations.
    */
    for (;;) {
	fd_set rd;

	rd = work;
	DebugPrint("to SELECT: 0x%08x", rd.fds_bits[0]);
	rv = select(nfd+1, &rd, 0, 0, 0);
	if (rv < 0) {
	    if ( errno == EINTR ) {
		continue;
	    } else {
		Fatal("select failed, errno = %d", errno);
	    }
	}

	DebugPrint("from SELECT: 0x%08x [%d]", rd.fds_bits[0], rv);

	/* Figure out what happened */
	for (s = servs; s; s = s->next) {
	    Todo *t, *nxt;
	    Sock *psock;

	    /* See if listen socket is ready for another connection */
	    if (FD_ISSET(s->listen, &rd)) {
		int al, s2;
		struct sockaddr_in sin;

		DebugPrint("connection ready on %d", s->listen);

		/* Connection came in for this service */
		al = sizeof(sin);

		if (s->auth) {
		    rv = SSL_Enable(s->listen, SSL_REQUEST_CERTIFICATE, 1);
		    if ( rv ) {
			Error("Could not enable client auth");
		    }
		}

		rv = SSL_Enable(s->listen, SSL_DELAYED_HANDSHAKE, 1);
		if (rv) {
		    DebugPrint("could not enable delayed handshake");
		}

		/* Accept this connection (it was enabled above) */
		s2 = SSL_Accept(s->listen, (struct sockaddr*) &sin, &al);
		if (s2 < 0) {
		    Error("accept failed on %d: %s", s->listen, 
			  SECU_ErrorString(XP_GetError()));
		} else {
		    u_long addr = ntohl(sin.sin_addr.s_addr);
		    DebugPrint("ready for %s data from %d.%d.%d.%d:%d on %d",
			  s->client ? "clear" : "ssl", 
			  (addr >> 24) & 0xff,
			  (addr >> 16) & 0xff,
			  (addr >> 8) & 0xff,
			  (addr >> 0) & 0xff,
			  s->port, s2);
		    if (s->client) {
			t = ConnectClient(s,s2); /* XXX client is broken */
		    } else {
			t = ConnectServer(s, s2);
		    }
		    if (t) {
			FD_SET(t->sockets->sslfd, &work);
			if (t->sockets->sslfd > nfd) {
			    nfd = t->sockets->sslfd;
			}
			t->next = s->todo;
			s->todo = t;

			callback = (Callback *)calloc(1, sizeof(Callback));
			callback->service = s;
			callback->todo = t;
			callback->sock = t->sockets;

			rv = SSL_HandshakeCallback(s2, HandshakeCallback,
						   (void *)callback);
			if (rv)
			    DebugPrint("error in set handshake callback on %d",
				       s2);
		    }
		}
	    }

	    for (t = s->todo; t; t = nxt) {
		char buf[8192];

		nxt = t->next;

		/* is there anything on the control channel for this client */
		if ( ( t->controlfd >= 0 ) && FD_ISSET(t->controlfd, &rd) ) {
		    struct sockaddr_in dstsin;
		    short port;
		    long addr;

		    rv = read(t->controlfd, &t->controlbuf[t->cbufoff],
				sizeof(t->controlbuf) - t->cbufoff);
		    if ( rv > 0 ) { /* read returns data */
			/* bump buffer fill count */
			t->cbufoff += rv;
			if ( t->cbufoff == sizeof(t->controlbuf) ) {
			    /* buffer is full */
			    struct ssld_proto *pctrlbuf;

			    pctrlbuf = (struct ssld_proto *)t->controlbuf;
			    t->cbufoff = 0;

			    if ( pctrlbuf->cmd == SSLD_CONNECT ) {

				/* extract port and address from buffer */
				memcpy(&port, &pctrlbuf->port,
				       sizeof(pctrlbuf->port));
				memcpy(&addr, &pctrlbuf->addr,
				       sizeof(pctrlbuf->addr));
				DebugPrint("SSLD_CONNECT request on %d(control) for %d.%d.%d.%d:%d",
					t->controlfd,
					(addr >> 24) & 0xff,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr >> 0) & 0xff,
					port);

				/* destination address */
				dstsin.sin_family = AF_INET;
				dstsin.sin_port = port;
				dstsin.sin_addr.s_addr = addr;

				/* set up new Sock structure */
				psock = calloc(1, sizeof(Sock));
				lport = IPPORT_RESERVED;
				fd = rresvport(&lport);
				if ( fd < 0 ) {
				    DebugPrint("first rresvport failed\n");
				}
				sport = (short)lport;
				/* listen for his connect */
				rv = listen(fd, 1);
				if ( rv < 0 ) {
				    DebugPrint("listen failed\n");
				}
				/* send our port to the client */
				memcpy(&pctrlbuf->port, &sport,
						sizeof(pctrlbuf->port));
				write(t->controlfd, pctrlbuf, SSLD_PROTO_LEN);
				DebugPrint("SSLD_CONNECT returns port %d", 
					   sport);

				psock->clearfd = accept(fd, 0, 0);
				if ( psock->clearfd < 0 ) {
				    DebugPrint("accept failed");
				} else {
				    DebugPrint("accept succeeded on %d(clear)",
							psock->clearfd);
				}
				close(fd);

				/* get reserved port for destination */
				fd = rresvport(&lport);
				if ( fd < 0 ) {
				    DebugPrint("second rresvport failed\n");
				}
			   
				/* make it a groovy ssl socket */
				rv = SSL_Import(fd);
				if (rv < 0)
				    DebugPrint("unable to import new sock");

				/* set up ssl on this socket */
				rv = SSL_Enable(fd, SSL_SECURITY, 1);
				if (rv < 0)
				    DebugPrint("enable failed\n");

				rv = SSL_Enable(fd, SSL_HANDSHAKE_AS_SERVER, 1);
				if (rv < 0)
				    DebugPrint("handshake enable failed\n");

				rv = SSL_AuthCertificateHook(fd, SSL_AuthCertificate,
							     (void *)SEC_GetDefaultCertDB());
				if (s->auth) {
				    rv = SSL_Enable(fd, SSL_REQUEST_CERTIFICATE, 1);				  
				    if (rv)
					DebugPrint("cert req failed on %d(ssl)", fd);
				}

				callback = (Callback *)calloc(1, sizeof(Callback));
				callback->service = s;
				callback->sock = psock;
				callback->todo = t;

				rv = SSL_HandshakeCallback(fd, HandshakeCallback, (void *)callback);
				if (rv)
				    DebugPrint("handshake callback failed on %d(ssl)", fd);

				/* now connect */	
				rv = SSL_Connect(fd, &dstsin, sizeof(dstsin));
				if ( rv < 0 ) {
				    DebugPrint("connect failed on %d(ssl)", fd);
				} else {
				    DebugPrint("connect succeeded on %d(ssl)",fd);
				}

				psock->sslfd = fd;

				/* link into list */
				psock->next = t->sockets;
				t->sockets = psock;

				/* add to work sockets */
				FD_SET(psock->sslfd, &work);
				if( psock->sslfd > nfd ) {
				    nfd = psock->sslfd;
				}
				DebugPrint("Set up back channel from %d(clear) to %d(ssl)", psock->clearfd, psock->sslfd);
			    } else if ( pctrlbuf->cmd == SSLD_GETPEER ) {
				struct sockaddr_in peersin;
				int peerlen = sizeof(struct sockaddr_in);
				/* XXX - need better fd */
				DebugPrint("SSLD_GETPEER request from %d(control)",
					t->controlfd);
				if ( getpeername(t->sockets->sslfd,
						(struct sockaddr *)&peersin,
						&peerlen) < 0 ) {
				    DebugPrint("getpeername failed");
				}

				memcpy(&pctrlbuf->port, &peersin.sin_port,
							sizeof(pctrlbuf->port));
				memcpy(&pctrlbuf->addr,
					&peersin.sin_addr.s_addr,
					sizeof(pctrlbuf->addr));
				addr = peersin.sin_addr.s_addr;
				port = peersin.sin_port;

				DebugPrint("SSLD_GETPEER of %d(ssl) is %d.%d.%d.%d:%d",
					t->sockets->sslfd,
					(addr >> 24) & 0xff,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr >> 0) & 0xff,
					port);
				write(t->controlfd, pctrlbuf, SSLD_PROTO_LEN);
				
			    }
			}
		    } else {
			FD_CLR(t->controlfd, &work);
			if (rv < 0) {
			    DebugPrint("control read (%d) returned %d (error=%d)",
				       t->controlfd, rv, errno);
			} else
			    DebugPrint("control read (%d) EOF", t->controlfd);
		    }
		}

		/* See if i/o sockets are ready for work to be done */
		for ( psock = t->sockets; psock; psock = nsock ) {
		    nsock = psock->next;
		    shutdown = 0;

		    if ( (psock->clearfd >= 0) &&
			  FD_ISSET(psock->clearfd, &rd) ) {
			int rv;
			int nb;
			nb = read(psock->clearfd, buf, sizeof(buf));
			DebugPrint("clear read (%d), %d bytes: [0x%02x]", 
				   psock->clearfd, nb, buf[0]);
			if (nb <= 0) {
			    DebugPrint("SHUTDOWN (clear read returned %d)",
				       nb);
			    shutdown = 1;
			    nfd = -1;
			} else {
			    rv = SSL_Write(psock->sslfd, buf, nb);
			    DebugPrint("ssl write (%d), %d of %d bytes",
				       psock->sslfd, rv, nb);
			    
			    if (rv < 0) {
				DebugPrint("SHUTDOWN (ssl write returned %d)",
					   rv);
				shutdown = 1;
				nfd = -1;
			    }
			}
		    }
		    if ( ( !shutdown ) &&
			 (psock->sslfd >= 0) &&
			 FD_ISSET(psock->sslfd, &rd) ) {
			int rv;
			int nb;
			nb = SSL_Read(psock->sslfd, buf, sizeof(buf));
			DebugPrint("ssl read (%d), %d bytes: [0x%02x]",
				   psock->sslfd, nb, buf[0]);
			if (nb < 0) {
			    if (XP_GetError() == EWOULDBLOCK)
				continue;
			    DebugPrint("SHUTDOWN (ssl read returned %d), %s", nb, SECU_ErrorString(XP_GetError()));
			    shutdown = 1;
			    nfd = -1;
			} else
			if (nb == 0) {
			    DebugPrint("SHUTDOWN (ssl read returned %d)", nb);
			    shutdown = 1;
			    nfd = -1;
			} else {
			    rv = write(psock->clearfd, buf, nb);
			    DebugPrint("clear write (%d), %d of %d bytes",
				       psock->clearfd, rv, nb);
			    if (rv < 0) {
				DebugPrint("SHUTDOWN (clear write returned %d)", rv);
				shutdown = 1;
				nfd = -1;
			    }
			}
		    }
		    if ( shutdown ) {
			Shutdown(s, t, psock, &work);
		    }
		}
	    }
	}

	if (nfd < 0) {
	    /* Recompute nfd value */
	    for (s = servs; s; s = s->next) {
		Todo *t;
		if (s->listen > nfd) nfd = s->listen;
		for (t = s->todo; t; t = t->next) {
		    for ( psock = t->sockets; psock; psock = psock->next ) {
			if (psock->sslfd > nfd) nfd = psock->sslfd;
			if (psock->clearfd > nfd) nfd = psock->clearfd;
		    }
		}
	    }
	}
    }
}
