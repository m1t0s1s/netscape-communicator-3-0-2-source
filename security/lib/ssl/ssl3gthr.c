#include "xp.h"
#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "ssl3prot.h"

extern int XP_ERRNO_EAGAIN;
extern int XP_ERRNO_EIO;
extern int XP_ERRNO_EWOULDBLOCK;
extern int SEC_ERROR_IO;

int
ssl3_GatherData(SSLSocket *ss, SSLGather *gs, int flags)
{
    unsigned char *bp, *lbp;
    int nb, err, rv;

    if (gs->state == GS_INIT) {
	gs->state = GS_HEADER;
	gs->remainder = 5;
	gs->offset = 0;
	gs->writeOffset = 0;
	gs->readOffset = 0;
    }
    
#ifdef XP_WIN
    __ssl_last_read_fd = ss->fd;
#endif

    lbp = gs->inbuf.buf;
    for(;;) {
	SSL_TRC(30, ("%d: SSL3[%d]: gather state %d (need %d more)",
		SSL_GETPID(), ss->fd, gs->state, gs->remainder));
	bp = ((gs->state != GS_HEADER) ? lbp : gs->hdr) + gs->offset;
	nb = XP_SOCK_RECV(ss->fd, (void *)bp, gs->remainder, flags);
	if (nb > 0) {
	    PRINT_BUF(60, (ss, "raw gather data:", bp, nb));
	}
	if (nb == 0) {
	    /* EOF */
	    SSL_TRC(30, ("%d: SSL3[%d]: EOF", SSL_GETPID(), ss->fd));
	    rv = 0;
	    break;
	}
	if (nb < 0) {
	    err = XP_SOCK_ERRNO;
	    PORT_SetError(err);
	    if ((err == XP_ERRNO_EWOULDBLOCK) || (err == XP_ERRNO_EAGAIN)) {
		/* Try again later */
		SSL_TRC(30, ("%d: SSL3[%d]: gather blocked",
			     SSL_GETPID(), ss->fd));
		rv = -2;
		break;
	    }
	    SSL_DBG(("%d: SSL3[%d]: recv error %d", SSL_GETPID(), ss->fd, err));
	    rv = -1;
	    break;
	}

	gs->offset += nb;
	gs->inbuf.len += nb;
	gs->remainder -= nb;
	if (gs->remainder != 0) {
	    continue;
	}

	/* finished with this record */
	switch (gs->state) {
	  case GS_HEADER:
	    gs->remainder = (gs->hdr[3] << 8) | gs->hdr[4];
	    /* This is the max fragment length for an encrypted fragment
	     * plus the size of the record header.
	     */
	    if(gs->remainder > (MAX_FRAGMENT_LENGTH + 2048 + 5)) {
		SSL3_SendAlert(ss, alert_fatal, unexpected_message);
		gs->state = GS_INIT;
		PORT_SetError(SEC_ERROR_IO);
		return -1;
	    }
	    gs->state = GS_DATA;
	    gs->offset = 0;
	    gs->inbuf.len = 0;
	    if (gs->remainder > gs->inbuf.space) {
		err = ssl_GrowBuf(&gs->inbuf, gs->remainder);
		if (err) {
		    return err;
		}
		lbp = gs->inbuf.buf;
	    }
	    break;
	  case GS_DATA:
	    gs->state = GS_INIT;
	    return 1;
	}
    }

    return rv;
}


int
ssl3_GatherRecord(SSLSocket *ss, int flags)
{
    SSLGather *gs = ss->gather;
    SSL3Ciphertext cipher;
    int rv;

    rv = ssl3_GatherData(ss, gs, flags);
    if (rv <= 0) {
	return rv;
    }
    cipher.type = gs->hdr[0];
    cipher.version = (gs->hdr[1] << 8) | gs->hdr[2];
    cipher.buf = &gs->inbuf;
	
    rv = SSL3_HandleRecord(ss, &cipher, &gs->buf);
    if (rv < 0) {
	return rv;
    }
    if (gs->buf.len == 0) {
	return -2;
    }
    gs->readOffset = 0;
    gs->writeOffset = gs->buf.len;
    return 1;
}

int
ssl3_GatherHandshake(SSLSocket *ss, int flags)
{
    SSLGather *gs = ss->gather;
    SSL3Ciphertext cipher;
    int rv;

    rv = ssl3_GatherData(ss, gs, flags);
    if (rv <= 0) {
	return rv;
    }
    cipher.type = gs->hdr[0];
    cipher.version = (gs->hdr[1] << 8) | gs->hdr[2];
    cipher.buf = &gs->inbuf;
	
    rv = SSL3_HandleRecord(ss, &cipher, &gs->buf);
    if (rv < 0) {
	return rv;
    }
    if (ss->ssl3->hs.ws != idle_handshake)
	return -2;

    gs->readOffset = 0;
    gs->writeOffset = gs->buf.len;
    return 1;
}
