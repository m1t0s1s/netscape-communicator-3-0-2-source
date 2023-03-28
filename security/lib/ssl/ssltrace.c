#include "xp.h"
#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"

#if defined(DEBUG) || defined(TRACE)
static char *hex = "0123456789abcdef";

void ssl_PrintBuf(SSLSocket *ss, char *msg, unsigned char *cp, int len)
{
    char buf[80];
    char *bp;

    if (ss) {
	XP_TRACE(("%d: SSL[%d]: %s", SSL_GETPID(), ss->fd, msg));
    } else {
	XP_TRACE(("%d: SSL: %s", SSL_GETPID(), msg));
    }
    bp = buf;
    while (--len >= 0) {
	unsigned char ch = *cp++;
	*bp++ = hex[(ch >> 4) & 0xf];
	*bp++ = hex[ch & 0xf];
	*bp++ = ' ';
	if (bp + 4 > buf + 80) {
	    *bp = 0;
	    XP_TRACE(("%s", buf));
	    bp = buf;
	}
    }
    if (bp > buf) {
	*bp = 0;
	XP_TRACE(("%s", buf));
    }
}

#define LEN(cp)		(((cp)[0] << 8) | ((cp)[1]))

static void PrintType(SSLSocket *ss, char *msg)
{
    if (ss) {
	XP_TRACE(("%d: SSL[%d]: dump-msg: %s", SSL_GETPID(), ss->fd, msg));
    } else {
	XP_TRACE(("%d: SSL: dump-msg: %s", SSL_GETPID(), msg));
    }
}

static void PrintInt(SSLSocket *ss, char *msg, unsigned v)
{
    if (ss) {
	XP_TRACE(("%d: SSL[%d]:           %s=%u", SSL_GETPID(), ss->fd,
		  msg, v));
    } else {
	XP_TRACE(("%d: SSL:           %s=%u", SSL_GETPID(), msg, v));
    }
}

static void PrintBuf(SSLSocket *ss, char *msg, unsigned char *cp, int len)
{
    char buf[80];
    char *bp;

    if (ss) {
	XP_TRACE(("%d: SSL[%d]:           %s", SSL_GETPID(), ss->fd, msg));
    } else {
	XP_TRACE(("%d: SSL:           %s", SSL_GETPID(), msg));
    }
    bp = buf;
    while (--len >= 0) {
	unsigned char ch = *cp++;
	*bp++ = hex[(ch >> 4) & 0xf];
	*bp++ = hex[ch & 0xf];
	*bp++ = ' ';
	if (bp + 4 > buf + 50) {
	    *bp = 0;
	    if (ss) {
		XP_TRACE(("%d: SSL[%d]:             %s",
			  SSL_GETPID(), ss->fd, buf));
	    } else {
		XP_TRACE(("%d: SSL:             %s", SSL_GETPID(), buf));
	    }
	    bp = buf;
	}
    }
    if (bp > buf) {
	*bp = 0;
	if (ss) {
	    XP_TRACE(("%d: SSL[%d]:             %s",
		      SSL_GETPID(), ss->fd, buf));
	} else {
	    XP_TRACE(("%d: SSL:             %s", SSL_GETPID(), buf));
	}
    }
}

void ssl_DumpMsg(SSLSocket *ss, unsigned char *bp, unsigned len)
{
    switch (bp[0]) {
      case SSL_MT_ERROR:
	PrintType(ss, "Error");
	PrintInt(ss, "error", LEN(bp+1));
	break;

      case SSL_MT_CLIENT_HELLO:
	{
	    unsigned lcs = LEN(bp+3);
	    unsigned ls = LEN(bp+5);
	    unsigned lc = LEN(bp+7);
	    PrintType(ss, "Client-Hello");
	    PrintInt(ss, "version", LEN(bp+1));
	    PrintInt(ss, "cipher-specs-length", lcs);
	    PrintInt(ss, "session-id-length", ls);
	    PrintInt(ss, "challenge-length", lc);
	    PrintBuf(ss, "cipher-specs", bp+9, lcs);
	    PrintBuf(ss, "session-id", bp+9+lcs, ls);
	    PrintBuf(ss, "challenge", bp+9+lcs+ls, lc);
	}
	break;
      case SSL_MT_CLIENT_MASTER_KEY:
	{
	    unsigned lck = LEN(bp+4);
	    unsigned lek = LEN(bp+6);
	    unsigned lka = LEN(bp+8);
	    PrintType(ss, "Client-Master-Key");
	    PrintInt(ss, "cipher-choice", bp[1]);
	    PrintInt(ss, "key-length", LEN(bp+2));
	    PrintInt(ss, "clear-key-length", lck);
	    PrintInt(ss, "encrypted-key-length", lek);
	    PrintInt(ss, "key-arg-length", lka);
	    PrintBuf(ss, "clear-key", bp+10, lck);
	    PrintBuf(ss, "encrypted-key", bp+10+lck, lek);
	    PrintBuf(ss, "key-arg", bp+10+lck+lek, lka);
	}
	break;
      case SSL_MT_CLIENT_FINISHED:
	PrintType(ss, "Client-Finished");
	PrintBuf(ss, "connection-id", bp+1, len-1);
	break;
      case SSL_MT_SERVER_HELLO:
	{
	    unsigned lc = LEN(bp+5);
	    unsigned lcs = LEN(bp+7);
	    unsigned lci = LEN(bp+9);
	    PrintType(ss, "Server-Hello");
	    PrintInt(ss, "session-id-hit", bp[1]);
	    PrintInt(ss, "certificate-type", bp[2]);
	    PrintInt(ss, "server-version", LEN(bp+3));
	    PrintInt(ss, "certificate-length", lc);
	    PrintInt(ss, "cipher-specs-length", lcs);
	    PrintInt(ss, "connection-id-length", lci);
	    PrintBuf(ss, "certificate", bp+11, lc);
	    PrintBuf(ss, "cipher-specs", bp+11+lc, lcs);
	    PrintBuf(ss, "connection-id", bp+11+lc+lcs, lci);
	}
	break;
      case SSL_MT_SERVER_VERIFY:
	PrintType(ss, "Server-Verify");
	PrintBuf(ss, "challenge", bp+1, len-1);
	break;
      case SSL_MT_SERVER_FINISHED:
	PrintType(ss, "Server-Finished");
	PrintBuf(ss, "session-id", bp+1, len-1);
	break;
      case SSL_MT_REQUEST_CERTIFICATE:
	PrintType(ss, "Request-Certificate");
	PrintInt(ss, "authentication-type", bp[1]);
	PrintBuf(ss, "certificate-challenge", bp+2, len-2);
	break;
      case SSL_MT_CLIENT_CERTIFICATE:
	{
	    unsigned lc = LEN(bp+2);
	    unsigned lr = LEN(bp+4);
	    PrintType(ss, "Client-Certificate");
	    PrintInt(ss, "certificate-type", bp[1]);
	    PrintInt(ss, "certificate-length", lc);
	    PrintInt(ss, "response-length", lr);
	    PrintBuf(ss, "certificate", bp+6, lc);
	    PrintBuf(ss, "response", bp+6+lc, lr);
	}
	break;
      default:
	ssl_PrintBuf(ss, "sending *unknown* message type", bp, len);
	return;
    }
}
#endif
