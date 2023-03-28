#include "xp.h"
#include "cert.h"
#include "xp_file.h"
#include "ssl.h"
#include "sslimpl.h"


extern int XP_ERRNO_EISCONN;
extern int XP_ERRNO_EBADF;
extern int XP_ERRNO_ECONNREFUSED;
extern int XP_ERRNO_EINVAL;
extern int XP_ERRNO_EIO;
extern int XP_ERRNO_EWOULDBLOCK;

#ifdef XP_UNIX
#define SOCKS_FILE	"/etc/socks.conf"
#endif
#ifdef XP_MAC
#define SOCKS_FILE NULL
#endif
#ifdef XP_WIN
#define SOCKS_FILE  NULL
#endif

#define SOCKS_VERSION	4

#define DEF_SOCKD_PORT	1080

#define SOCKS_CONNECT	1
#define SOCKS_BIND	2

#define SOCKS_RESULT	90
#define SOCKS_FAIL	91
#define SOCKS_NO_IDENTD	92 /* Failed to connect to Identd on client machine */
#define SOCKS_BAD_ID	93 /* Client's Identd reported a different user-id */

#define MAKE_IN_ADDR(a,b,c,d) \
    htonl(((unsigned long)(a) << 24) | ((unsigned long)(b) << 16) | \
	  ((c) << 8) | (d))

typedef struct SocksConfItemStr SocksConfItem;

struct SocksConfItemStr {
    char direct;
    unsigned long daddr;
    unsigned long dmask;
    int op;
    unsigned short port;
    SocksConfItem *next;
};

/* Values for op */
#define OP_LESS		1
#define OP_EQUAL	2
#define OP_LEQUAL	3
#define OP_GREATER	4
#define OP_NOTEQUAL	5
#define OP_GEQUAL	6
#define OP_ALWAYS	7

static unsigned long ourHost;
static SocksConfItem *ssl_socks_confs;

int ssl_CreateSocksInfo(SSLSocket *ss)
{
    SSLSocksInfo *si;

    if (ss->socks) {
	/* Already been done */
	return 0;
    }

    si = (SSLSocksInfo*) PORT_ZAlloc(sizeof(SSLSocksInfo));
    if (si) {
	ss->socks = si;
	if (!ss->gather) {
	    ss->gather = ssl_NewGather();
	    if (!ss->gather) {
		return -1;
	    }
	}
	return 0;
    }
    return -1;
}

int ssl_CopySocksInfo(SSLSocket *ss, SSLSocket *os)
{
    int rv;

#ifdef __cplusplus
    os = os;
#endif
    rv = ssl_CreateSocksInfo(ss);
    return rv;
}

void ssl_DestroySocksInfo(SSLSocksInfo *si)
{
    if (si) {
	PORT_Free(si);
    }
}

#if defined(XP_UNIX) || defined(XP_MAC)
#include "prnetdb.h"
#else
#define PRHostEnt struct hostent
#define PR_NETDB_BUF_SIZE 5
#endif
static int GetOurHost(void)
{
    char name[100];
    PRHostEnt hpbuf, *hp;
    char dbbuf[PR_NETDB_BUF_SIZE];

    gethostname(name, sizeof(name));
#if defined(XP_UNIX) || defined(XP_MAC)
    hp = PR_gethostbyname(name, &hpbuf, dbbuf, sizeof(dbbuf), 0);
#else
    hp = gethostbyname(name);
#endif
    if (hp) {
	ourHost = *(unsigned long *)hp->h_addr;
    } else {
	/* Total lossage! */
	return -1;
    }
    return 0;
}

/*
** Setup default SocksConfItem list so that loopback is direct, things to the
** same subnet (?) address are direct, everything else uses sockd
*/
static void BuildDefaultConfList(void)
{
    SocksConfItem *ci;
    SocksConfItem **lp;

    /* Put loopback onto direct list */
    lp = &ssl_socks_confs;
    ci = (SocksConfItem*) PORT_ZAlloc(sizeof(SocksConfItem));
    if (!ci) {
	return;
    }
    ci->direct = 1;
    ci->daddr = MAKE_IN_ADDR(127,0,0,1);
    ci->dmask = MAKE_IN_ADDR(255,255,255,255);
    ci->op = OP_ALWAYS;
    *lp = ci;
    lp = &ci->next;

    /* Put our hosts's subnet onto direct list */
    ci = (SocksConfItem*) PORT_ZAlloc(sizeof(SocksConfItem));
    if (!ci) {
	return;
    }
    ci->direct = 1;
    ci->daddr = htonl((ntohl(ourHost) & ~0xff) | 0xff);
    ci->dmask = MAKE_IN_ADDR(255,255,255,0);
    ci->op = OP_ALWAYS;
    *lp = ci;
    lp = &ci->next;

    /* Everything else goes to sockd */
    ci = (SocksConfItem*) PORT_ZAlloc(sizeof(SocksConfItem));
    if (!ci) {
	return;
    }
    ci->daddr = MAKE_IN_ADDR(255,255,255,255);
    ci->op = OP_ALWAYS;
    *lp = ci;
}

static int FragmentLine(char *cp, char **argv, int maxargc)
{
    int argc = 0;
    char *save;
    char ch;

    save = cp;
    for (; (ch = *cp) != 0; cp++) {
	if ((ch == '#') || (ch == '\n')) {
	    /* Done */
	    break;
	}
	if (ch == ':') {
	    break;
	}
	if ((ch == ' ') || (ch == '\t')) {
	    /* Seperator. see if it seperated anything */
	    if (cp - save > 0) {
		/* Put a null at the end of the word */
		*cp = 0;
		argc++;
		*argv++ = save;
		SSL_TRC(20, ("%d: SSL: argc=%d word=\"%s\"",
			     SSL_GETPID(), argc, save));
		if (argc == maxargc) {
		    return argc;
		}
	    }
	    save = cp + 1;
	}
    }
    if (cp - save > 0) {
	*cp = 0;
	argc++;
	*argv = save;
	SSL_TRC(20, ("%d: SSL: argc=%d word=\"%s\"",
		     SSL_GETPID(), argc, save));
    }
    return argc;
}

/* XXX inet_addr? */
static char *ConvertOne(char *cp, unsigned char *rvp)
{
    char *s = PORT_Strchr(cp, '.');
    if (s) {
	*s = 0;
    }
    *rvp = PORT_Atoi(cp) & 0xff;
    return s ? s+1 : cp;
}

static unsigned long ConvertAddr(char *buf)
{
    unsigned char b0, b1, b2, b3;
    unsigned long addr;

    buf = ConvertOne(buf, &b0);
    buf = ConvertOne(buf, &b1);
    buf = ConvertOne(buf, &b2);
    buf = ConvertOne(buf, &b3);
    addr = ((unsigned long)b0 << 24) |
	((unsigned long)b1 << 16) |
	((unsigned long)b2 << 8) |
	(unsigned long)b3;

    return htonl(addr);
}

static int ReadConfFile(void)
{
    int rv;

    rv = GetOurHost();
    if (rv < 0) {
	/* If we can't figure out our host id, use socks. Loser! */
	return -1;
    }

    {
	SocksConfItem *ci;
	SocksConfItem **lp;
	XP_File fp;
	char *file = SOCKS_FILE;
	int op, direct, port = 0, lineNumber = 0;

	fp = XP_FileOpen(file, xpSocksConfig, XP_FILE_READ);
	if (!fp) {
	    BuildDefaultConfList();
	    return 0;
	}

	/* Parse config file and generate config item list */
	lp = &ssl_socks_confs;
	for (;;) {
	    char buf[1000];
	    char *s, *argv[10];
	    int argc;

	    s = XP_FileReadLine(buf, sizeof(buf), fp);
	    if (!s) {
		break;
	    }
	    lineNumber++;
	    argc = FragmentLine(buf, argv, 10);
	    if (argc < 3) {
		if (argc == 0) {
		    /* must be a comment/empty line */
		    continue;
		}
#ifdef XP_UNIX
		fprintf(stderr, "%s:%d: bad config line\n",
			file, lineNumber);
#endif
		continue;
	    }
	    if (PORT_Strcmp(argv[0], "direct") == 0) {
		direct = 1;
	    } else if (PORT_Strcmp(argv[0], "sockd") == 0) {
		direct = 0;
	    } else {
#ifdef XP_UNIX
		fprintf(stderr, "%s:%d: bad command: \"%s\"\n",
			file, lineNumber, argv[0]);
#endif
		continue;
	    }

	    /* Look for port spec */
	    op = OP_ALWAYS;
	    if (argc > 4) {
		if (PORT_Strcmp(argv[3], "lt") == 0) {
		    op = OP_LESS;
		} else if (PORT_Strcmp(argv[3], "eq") == 0) {
		    op = OP_EQUAL;
		} else if (PORT_Strcmp(argv[3], "le") == 0) {
		    op = OP_LEQUAL;
		} else if (PORT_Strcmp(argv[3], "gt") == 0) {
		    op = OP_GREATER;
		} else if (PORT_Strcmp(argv[3], "neq") == 0) {
		    op = OP_NOTEQUAL;
		} else if (PORT_Strcmp(argv[3], "ge") == 0) {
		    op = OP_GEQUAL;
		} else {
#ifdef XP_UNIX
		    fprintf(stderr, "%s:%d: bad comparison op: \"%s\"\n",
			    file, lineNumber, argv[3]);
#endif
		    continue;
		}
		port = PORT_Atoi(argv[4]);
	    }

	    ci = (SocksConfItem*) PORT_ZAlloc(sizeof(SocksConfItem));
	    if (!ci) {
		break;
	    }
	    ci->direct = direct;
	    ci->daddr = ConvertAddr(argv[1]);
	    ci->dmask = ConvertAddr(argv[2]);
	    ci->op = op;
	    ci->port = port;
	    SSL_TRC(10, ("%d: SSL: line=%d direct=%d addr=%d.%d.%d.%d mask=%d.%d.%d.%d op=%d port=%d",
			 SSL_GETPID(), lineNumber, ci->direct,
			 (ntohl(ci->daddr) >> 24) & 0xff,
			 (ntohl(ci->daddr) >> 16) & 0xff,
			 (ntohl(ci->daddr) >> 8) & 0xff,
			 (ntohl(ci->daddr) >> 0) & 0xff,
			 (ntohl(ci->dmask) >> 24) & 0xff,
			 (ntohl(ci->dmask) >> 16) & 0xff,
			 (ntohl(ci->dmask) >> 8) & 0xff,
			 (ntohl(ci->dmask) >> 0) & 0xff,
			 ci->op, ci->port));
	    *lp = ci;
	    lp = &ci->next;
	}
    }

    if (!ssl_socks_confs) {
	/* Empty file. Fix it for the user */
	BuildDefaultConfList();
    }
    return 0;
}

static int ChooseAddress(SSLSocket *ss, struct sockaddr_in *direct)
{
    unsigned long dstAddr;
    unsigned short dstPort;
    SocksConfItem *ci;
    int rv;

    if (!ssl_socks_confs) {
	rv = ReadConfFile();
	if (rv) {
	    return rv;
	}
    }

    /*
    ** Scan socks config info and look for a direct match or a force to
    ** use the sockd. Bail on first hit.
    */
    dstAddr = direct->sin_addr.s_addr;
    dstPort = ntohs(direct->sin_port);
    ci = ssl_socks_confs;
    while (ci) {
	SSL_TRC(10,
		("%d: SSL[%d]: match, direct=%d daddr=0x%x mask=0x%x op=%d port=%d",
		 SSL_GETPID(), ss->fd, ci->direct, ci->daddr,
		 ci->dmask, ci->op, ci->port));
	if ((ci->daddr & ci->dmask)  == (dstAddr & ci->dmask)) {
	    int portMatch = 0;
	    switch (ci->op) {
	      case OP_LESS:	portMatch = dstPort < ci->port; break;
	      case OP_EQUAL:	portMatch = dstPort == ci->port; break;
	      case OP_LEQUAL:	portMatch = dstPort <= ci->port; break;
	      case OP_GREATER:	portMatch = dstPort > ci->port; break;
	      case OP_NOTEQUAL:	portMatch = dstPort != ci->port; break;
	      case OP_GEQUAL:	portMatch = dstPort >= ci->port; break;
	      case OP_ALWAYS:	portMatch = 1; break;
	    }
	    if (portMatch) {
		SSL_TRC(10, ("%d: SSL[%d]: socks config match",
			     SSL_GETPID(), ss->fd));
		return ci->direct;
	    }
	}
	ci = ci->next;
    }
    SSL_TRC(10, ("%d: SSL[%d]: socks config: no match",
		 SSL_GETPID(), ss->fd));
    return 0;
}

/*
** Find port # and host # of socks daemon. Use info in ss->socks struct
** when valid. If not valid, try to figure it all out.
*/
static int FindDaemon(SSLSocket *ss, struct sockaddr_in *out)
{
    SSLSocksInfo *si;
    unsigned short port;
    unsigned long host;

    PORT_Assert(ss->socks != 0);
    si = ss->socks;

    /* For now, assume we are using the socks daemon */
    host = si->sockdHost;
    port = si->sockdPort;
#ifdef XP_UNIX
    if (!port) {
	static char firstTime = 1;
	static unsigned short sockdPort;

	if (firstTime) {
	    struct servent *sp;

	    firstTime = 0;
	    sp = getservbyname("socks", "tcp");
	    if (sp) {
		sockdPort = sp->s_port;
	    } else {
		SSL_TRC(10, ("%d: SSL[%d]: getservbyname of (socks,tcp) fails",
			     SSL_GETPID(), ss->fd));
	    }
	}
	port = sockdPort;
    }
#endif
    if (!port) {
	port = DEF_SOCKD_PORT;
    }
    if (host == 0) {
	SSL_TRC(10, ("%d: SSL[%d]: no socks server found",
		     SSL_GETPID(), ss->fd));
	PORT_SetError(XP_ERRNO_EINVAL);
	return -1;
    }

    /* We know the ip addr of the socks server */
    out->sin_family = AF_INET;
    out->sin_port = htons(port);
    out->sin_addr.s_addr = host;
    SSL_TRC(10, ("%d: SSL[%d]: socks server at %d.%d.%d.%d:%d",
		 SSL_GETPID(), ss->fd,
		 (ntohl(out->sin_addr.s_addr) >> 24) & 0xff,
		 (ntohl(out->sin_addr.s_addr) >> 16) & 0xff,
		 (ntohl(out->sin_addr.s_addr) >> 8) & 0xff,
		 (ntohl(out->sin_addr.s_addr) >> 0) & 0xff,
		 port));
    return 0;
}

/*
** Send our desired address and our user name to the socks daemon.
*/
static int SayHello(SSLSocket *ss, int cmd, struct sockaddr_in *sa, char *user)
{
    int rv, len;
    unsigned char msg[8];
    unsigned short port;
    unsigned long host;

    /* Send dst message to sockd */
    port = sa->sin_port;
    host = sa->sin_addr.s_addr;
    msg[0] = SOCKS_VERSION;
    msg[1] = cmd;
    PORT_Memcpy(msg+2, &port, 2);
    PORT_Memcpy(msg+4, &host, 4);
    SSL_TRC(10, ("%d: SSL[%d]: socks real dest=%d.%d.%d.%d:%d",
		 SSL_GETPID(), ss->fd, msg[4], msg[5], msg[6], msg[7],
		 port));

    rv = ssl_DefSend(ss, msg, sizeof(msg), 0);
    if (rv < 0) {
	goto io_error;
    }

    /* Send src-user message to sockd */
    len = strlen(user)+1;
    rv = ssl_DefSend(ss, user, len, 0);
    if (rv < 0) {
	goto io_error;
    }
    return 0;

  io_error:
    SSL_TRC(10, ("%d: SSL[%d]: socks, io error saying hello to sockd errno=%d",
		 SSL_GETPID(), ss->fd, PORT_GetError()));
    return -1;
}

static int GetDst(SSLSocket *ss)
{
    SSLGather *gs;
    unsigned char *msg, cmd;

    PORT_Assert(ss->gather != 0);
    gs = ss->gather;

    msg = gs->buf.buf;
    cmd = msg[1];
    SSL_TRC(10, ("%d: SSL[%d]: socks result: cmd=%d",
		 SSL_GETPID(), ss->fd, cmd));
    PORT_Memcpy(&ss->socks->destPort, msg+2, 2);
    PORT_Memcpy(&ss->socks->destHost, msg+4, 4);

    /* Check status back from sockd */
    switch (cmd) {
      case SOCKS_FAIL:
      case SOCKS_NO_IDENTD:
      case SOCKS_BAD_ID:
	SSL_DBG(("%d: SSL[%d]: sockd returns an error: %d",
		 SSL_GETPID(), ss->fd, cmd));
	PORT_SetError(XP_ERRNO_ECONNREFUSED);
	return -1;

      default:
	break;
    }

    /* All done */
    SSL_TRC(1, ("%d: SSL[%d]: using sockd at %d.%d.%d.%d",
		SSL_GETPID(), ss->fd,
		(ntohl(ss->socks->sockdHost) >> 24) & 0xff,
		(ntohl(ss->socks->sockdHost) >> 16) & 0xff,
		(ntohl(ss->socks->sockdHost) >> 8) & 0xff,
		(ntohl(ss->socks->sockdHost) >> 0) & 0xff));
    ss->handshake = 0;
    ss->nextHandshake = 0;
    ss->gather->recordLen = 0;
    return 0;
}

static int Gather(SSLSocket *ss)
{
    int rv;

    rv = ssl_GatherData(ss, ss->gather, 0);
    if (rv <= 0) {
	PORT_SetError(rv < 0 ? XP_SOCK_ERRNO : XP_ERRNO_EIO);
	return -1;
    }
    ss->handshake = 0;
    return 0;
}

static int StartGather(SSLSocket *ss)
{
    int rv;

    ss->handshake = Gather;
    ss->nextHandshake = GetDst;
    rv = ssl_StartGatherBytes(ss, ss->gather, 8);
    if (rv <= 0) {
	if (rv == 0) {
	    /* Unexpected EOF */
	    PORT_SetError(XP_ERRNO_EIO);
	    return -1;
	}
	return rv;
    }
    ss->handshake = 0;
    return 0;
}

/************************************************************************/


/* BSDI ain't got no cuserid() */
#ifdef __386BSD__
#include <pwd.h>
char *bsdi_cuserid(char *b)
{
    struct passwd *pw = getpwuid(getuid());

    if (!b) return pw ? pw->pw_name : NULL;

    if (!pw || !pw->pw_name)
	b[0] = '\0';
    else
	strcpy(b, pw->pw_name);
    return b;
}
#endif


int ssl_SocksConnect(SSLSocket *ss, const void *sa, int salen)
{
    int rv, err, direct;
    struct sockaddr_in daemon, *sip;
    char *user;

#ifdef __cplusplus
    salen = salen;
#endif

    /* Figure out where to connect to */
    rv = FindDaemon(ss, &daemon);
    if (rv) {
	return -1;
    }
    direct = ChooseAddress(ss, (struct sockaddr_in*)sa);
    if (direct) {
	sip = (struct sockaddr_in*) sa;
	ss->socks->direct = 1;
    } else {
	sip = &daemon;
	ss->socks->direct = 0;
    }
    SSL_TRC(10, ("%d: SSL[%d]: socks %s connect to %d.%d.%d.%d:%d",
		 SSL_GETPID(), ss->fd,
		 direct ? "direct" : "sockd",
		 (ntohl(sip->sin_addr.s_addr) >> 24) & 0xff,
		 (ntohl(sip->sin_addr.s_addr) >> 16) & 0xff,
		 (ntohl(sip->sin_addr.s_addr) >> 8) & 0xff,
		 ntohl(sip->sin_addr.s_addr) & 0xff,
		 ntohs(sip->sin_port)));

    /* Attempt first connection */
    rv = XP_SOCK_CONNECT(ss->fd, (struct sockaddr*)sip, sizeof(*sip));
    if (rv < 0) {
	err = XP_SOCK_ERRNO;
	PORT_SetError(err);
	if (err != XP_ERRNO_EISCONN) {
	    return rv;
	}
	/* Async connect finished */
    }

    /* If talking to sockd, do handshake */
    if (!direct) {
	/* Find user */
#ifdef XP_UNIX
#ifdef __386BSD__
	user = bsdi_cuserid(0);
#else
	user = cuserid(0);
#endif
	if (!user) {
	    PORT_SetError(XP_ERRNO_EINVAL);
	    SSL_DBG(("%d: SSL[%d]: cuserid fails, errno=%d",
		     SSL_GETPID(), ss->fd, PORT_GetError()));
	    return -1;
	}
#else
	user = "SSL";
#endif

	/* Send our message to it */
	rv = SayHello(ss, SOCKS_CONNECT, (struct sockaddr_in*) sa, user);
	if (rv) {
	    return rv;
	}

	ss->handshake = StartGather;
	ss->nextHandshake = 0;

	/* save up who we're really talking to so we can index the cache */
	ss->peer = ((struct sockaddr_in *)sa)->sin_addr.s_addr;
	ss->port = ((struct sockaddr_in *)sa)->sin_port;
    }
    return 0;
}

static int WaitForResponse(SSLSocket *ss)
{
    int rv;

    ss->handshake = StartGather;
    ss->nextHandshake = 0;

    /* Get response. Do it now, spinning if necessary (!) */
    for (;;) {
	rv = ssl_ReadHandshake(ss);
	if (rv == -2) {
#ifdef XP_UNIX
	    /*
	    ** Spinning is really evil under unix. Call select and
	    ** continue when a read select returns true. We only get
	    ** here if the socket was marked async before the bind
	    ** call.
	    */
	    fd_set spin;
	    PORT_Memset(&spin, 0, sizeof(spin));
	    FD_SET(ss->fd, &spin);
	    rv = select(ss->fd+1, &spin, 0, 0, 0);
	    if (rv < 0) {
		return rv;
	    }
	    if (rv == 0) {
		/* Buggy OS */
		PORT_SetError(XP_ERRNO_EIO);
		return -1;
	    }
#endif
	    continue;
	}
	break;
    }
    return rv;
}

int ssl_SocksBind(SSLSocket *ss, const void *sa, int salen)
{
    SSLSocksInfo *si;
    int rv, direct;
    struct sockaddr_in daemon;
    char *user;

    PORT_Assert(ss->socks != 0);
    si = ss->socks;

    /* Figure out where to connect to */
    rv = FindDaemon(ss, &daemon);
    if (rv) {
	return -1;
    }
    direct = ChooseAddress(ss, (struct sockaddr_in*)sa);
    if (direct) {
	ss->socks->direct = 1;
	rv = XP_SOCK_BIND(ss->fd, (struct sockaddr*) sa, salen);
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	}
	PORT_Memcpy(&ss->socks->bindAddr, sa, sizeof(struct sockaddr_in));
    } else {
	ss->socks->direct = 0;
	SSL_TRC(10, ("%d: SSL[%d]: socks sockd bind to %d.%d.%d.%d:%d",
		     SSL_GETPID(), ss->fd,
		     (ntohl(daemon.sin_addr.s_addr) >> 24) & 0xff,
		     (ntohl(daemon.sin_addr.s_addr) >> 16) & 0xff,
		     (ntohl(daemon.sin_addr.s_addr) >> 8) & 0xff,
		     ntohl(daemon.sin_addr.s_addr) & 0xff,
		     ntohs(daemon.sin_port)));

	/* First connect to socks daemon. ASYNC connects must be disabled! */
	rv = XP_SOCK_CONNECT(ss->fd, (struct sockaddr*)&daemon, sizeof(daemon));
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	    return rv;
	}

	/* Find user */
#ifdef XP_UNIX
#ifdef __386BSD__
	user = bsdi_cuserid(0);
#else
	user = cuserid(0);
#endif
	if (!user) {
	    SSL_DBG(("%d: SSL[%d]: cuserid fails, errno=%d",
		     SSL_GETPID(), ss->fd, PORT_GetError()));
	    PORT_SetError(XP_ERRNO_EINVAL);
	    return -1;
	}
#else
	user = "SSL";
#endif
	/* Send message to sockd */
	rv = SayHello(ss, SOCKS_BIND, (struct sockaddr_in*) sa, user);
	if (rv) {
	    return rv;
	}

	/* Gather up bind response from sockd */
	rv = WaitForResponse(ss);
	if (rv == 0) {
	    /* Done */
	    si->bindAddr.sin_family = AF_INET;
	    si->bindAddr.sin_port = si->destPort;
	    if (ntohl(si->destHost) == INADDR_ANY) {
		si->bindAddr.sin_addr.s_addr = daemon.sin_addr.s_addr;
	    } else {
		si->bindAddr.sin_addr.s_addr = si->destHost;
	    }
	}
    }
    si->didBind = 1;
    return rv;
}

int
SSL_BindForSockd(int s, const void *sa, int salen, long dsthost)
{
    SSLSocket *ss;
    SSLSocksInfo *si;
    int rv, direct;
    struct sockaddr_in daemon;
    char *user;
    struct sockaddr_in dst;

    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = dsthost;
    dst.sin_port = 0;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in bind", SSL_GETPID(), s));
	return -1;
    }

    si = ss->socks;
    if (si == NULL) {
	SSL_DBG(("%d: SSL[%d]: no socks information in bind", SSL_GETPID(), s));
	return -1;
    }

    /* Figure out where to connect to */
    rv = FindDaemon(ss, &daemon);
    if (rv) {
	return -1;
    }
    direct = ChooseAddress(ss, &dst);
    if (direct) {
        ss->socks->direct = 1;
        rv = XP_SOCK_BIND(ss->fd, (struct sockaddr*) sa, salen);
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	}
        PORT_Memcpy(&ss->socks->bindAddr, sa, sizeof(struct sockaddr_in));
    } else {
        ss->socks->direct = 0;

        /* First connect to socks daemon. ASYNC connects must be disabled! */
        rv = XP_SOCK_CONNECT(ss->fd, (struct sockaddr*)&daemon, sizeof(daemon));
        if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
            return rv;
        }

        /* Find user */
#ifdef XP_UNIX
#ifdef __386BSD__
        user = bsdi_cuserid(0);
#else
        user = cuserid(0);
#endif
        if (!user) {
	    SSL_DBG(("%d: SSL[%d]: cuserid fails, errno=%d",
		     SSL_GETPID(), ss->fd, PORT_GetError()));
	    PORT_SetError(XP_ERRNO_EINVAL);
            return -1;
	}
#else
	user = "SSL";
#endif
        /* Send message to sockd */
        rv = SayHello(ss, SOCKS_BIND, &dst, user);
        if (rv) {
            return rv;
        }

        /* Gather up bind response from sockd */
        rv = WaitForResponse(ss);

        if (rv == 0) {
            /* Done */
            si->bindAddr.sin_family = AF_INET;
            si->bindAddr.sin_port = si->destPort;
            if (ntohl(si->destHost) == INADDR_ANY) {
                si->bindAddr.sin_addr.s_addr = daemon.sin_addr.s_addr;
            } else {
                si->bindAddr.sin_addr.s_addr = si->destHost;
            }
        }
    }
    si->didBind = 1;
    return rv;
}

#if 0
static int SocksSend(SSLSocket *ss, unsigned char* bp, int len, int flags)
{
    int rv = XP_SOCK_SEND(ss->fd, bp, len, flags);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}
#endif

int ssl_SocksAccept(SSLSocket *ss, void *addr, int *addrlenp)
{
    SSLSocket *ns;
    SSLSocksInfo *si;
    int rv;

    PORT_Assert(ss->socks != 0);
    si = ss->socks;

    if (*addrlenp < sizeof(struct sockaddr_in)) {
	PORT_SetError(XP_ERRNO_EINVAL);
	return -1;
    }

    if (!si->didBind || si->direct) {
	/*
	** If we didn't do the bind yet this call will generate an error
	** from the OS. If we did do the bind then we must be direct and
	** let the OS do the accept.
	*/
	rv = (*ssl_accept_func)(ss->fd, (struct sockaddr*) addr, addrlenp);
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	}
	return rv;
    }

    /* Get next accept response from server */
    rv = WaitForResponse(ss);
    if (rv) {
	return rv;
    }

    /* Handshake finished. Give dest address back to caller */
    ((struct sockaddr_in*)addr)->sin_family = AF_INET;
    ((struct sockaddr_in*)addr)->sin_port = si->destPort;
    ((struct sockaddr_in*)addr)->sin_addr.s_addr = si->destHost;

    /* Dup the descriptor and return it */
    rv = XP_SOCK_DUP(ss->fd);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
	return rv;
    }

    /* Dup the socket structure */
    ns = ssl_DupSocket(ss, rv);
    if (!ns) {
	XP_SOCK_CLOSE(rv);
	return -1;
    }

    return rv;
}

int ssl_SocksListen(SSLSocket *ss, int backlog)
{
    int rv;

    PORT_Assert(ss->socks != 0);

    if (ss->socks->direct) {
	rv = XP_SOCK_LISTEN(ss->fd, backlog);
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	}
	return rv;
    }
    return 0;
}

int ssl_SocksGetsockname(SSLSocket *ss, void *name, int *namelenp)
{
    int rv;

    PORT_Assert(ss->socks != 0);
    if (!ss->socks->didBind || ss->socks->direct) {
	rv = XP_SOCK_GETSOCKNAME(ss->fd, (struct sockaddr *)name, namelenp);
	if (rv < 0) {
	    PORT_SetError(XP_SOCK_ERRNO);
	}
	return rv;
    }

    if (*namelenp < sizeof(struct sockaddr_in)) {
	PORT_SetError(XP_ERRNO_EINVAL);
	return -1;
    }
    *namelenp = sizeof(struct sockaddr_in);
    PORT_Memcpy(name, &ss->socks->bindAddr, sizeof(struct sockaddr_in));
    return 0;
}

int ssl_SocksRecv(SSLSocket *ss, void *buf, int len, int flags)
{
    int rv;

    PORT_Assert(ss->socks != 0);

    if (ss->handshake) {
	rv = ssl_ReadHandshake(ss);
	if (rv < 0) {
	    return rv;
	}
	if (ssl_SendSavedWriteData(ss, &ss->saveBuf, ssl_DefSend) < 0) {
	    return -1;
	}
    }

    rv = XP_SOCK_RECV(ss->fd, buf, len, flags);
    SSL_TRC(2, ("%d: SSL[%d]: recving %d bytes from sockd",
		SSL_GETPID(), ss->fd, rv));
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
    }
    return rv;
}

int ssl_SocksRead(SSLSocket *ss, void *buf, int len)
{
    return ssl_SocksRecv(ss, buf, len, 0);
}

int ssl_SocksSend(SSLSocket *ss, const void *buf, int len, int flags)
{
    int rv;

    PORT_Assert(ss->socks != 0);

    if (len == 0) return 0;
    if (ss->handshake) {
	rv = ssl_WriteHandshake(ss, buf, len);
	if (rv < 0) {
	    if (rv == -2) {
		return len;
	    }
	    return rv;
	}
	if (ssl_SendSavedWriteData(ss, &ss->saveBuf, ssl_DefSend) < 0) {
	    return -1;
	}
    }

    SSL_TRC(2, ("%d: SSL[%d]: sending %d bytes using socks",
		SSL_GETPID(), ss->fd, len));

    /* Send out the data */
    rv = ssl_DefSend(ss, (unsigned char*)buf, len, flags);
    return rv;
}

int ssl_SocksWrite(SSLSocket *ss, const void *buf, int len)
{
    return ssl_SocksSend(ss, buf, len, 0);
}

int SSL_CheckDirectSock(int s)
{
    SSLSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in CheckDirectSock", SSL_GETPID(), s));
	return -1;
    }

    if (ss->socks != NULL) {
	return ss->socks->direct;
    }
    return -1;
}


int SSL_ConfigSockd(int s, unsigned long host, short port)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in ConfigSocks", SSL_GETPID(), s));
	return -1;
    }

    /* Create socks info if not already done */
    rv = ssl_CreateSocksInfo(ss);
    if (rv) {
	return rv;
    }
    ss->socks->sockdHost = host;
    ss->socks->sockdPort = port;
    return 0;
}
