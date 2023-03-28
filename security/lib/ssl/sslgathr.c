#include "xp.h"
#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"

extern int XP_ERRNO_EAGAIN;
extern int XP_ERRNO_EIO;
extern int XP_ERRNO_EWOULDBLOCK;
extern int SEC_ERROR_IO;
extern int SSL_ERROR_NO_CYPHER_OVERLAP;
extern int SSL_ERROR_BAD_MAC_READ;

extern int ssl_HandleClientHelloMessage(SSLSocket *ss);
extern int ssl_HandleServerHelloMessage(SSLSocket *ss);

#ifdef XP_WIN
SOCKET __ssl_last_read_fd;
#endif

/*
** Gather a single record of data from the receiving stream. This code
** first gathers the header (2 or 3 bytes long depending on the value of
** the most significant bit in the first byte) then gathers up the data
** for the record into the readBuf. This code handles non-blocking I/O
** and is to be called multiple times until sec->recordLen != 0.
*/
int ssl_GatherData(SSLSocket *ss, SSLGather *gs, int flags)
{
    SSLSecurityInfo *sec = ss->sec;
    unsigned char *bp, *lbp;
    unsigned char mac[SSL_MAX_MAC_BYTES], seq[4];
    uint32 sequenceNumber;
    int nb, err, rv;

    if (gs->state == GS_INIT) {
	/* Initialize gathering engine */
	gs->state = GS_HEADER;
	gs->remainder = 2;
	gs->count = 2;
	gs->offset = 0;
	gs->recordLen = 0;
	gs->recordPadding = 0;
	gs->hdr[2] = 0;

	gs->writeOffset = 0;
	gs->readOffset = 0;
    }
    if (gs->encrypted) {
	PORT_Assert(sec != 0);
    }

#ifdef XP_WIN
    __ssl_last_read_fd = ss->fd;
#endif
    lbp = gs->buf.buf;
    for (;;) {
	SSL_TRC(30, ("%d: SSL[%d]: gather state %d (need %d more)",
		     SSL_GETPID(), ss->fd, gs->state, gs->remainder));
	bp = ((gs->state != GS_HEADER) ? lbp : gs->hdr) + gs->offset;
	nb = XP_SOCK_RECV(ss->fd, (void *)bp, gs->remainder, flags);
	if (nb > 0) {
	    PRINT_BUF(60, (ss, "raw gather data:", bp, nb));
	}
	if (nb == 0) {
	    /* EOF */
	    SSL_TRC(30, ("%d: SSL[%d]: EOF", SSL_GETPID(), ss->fd));
	    rv = 0;
	    break;
	}
	if (nb < 0) {
	    err = XP_SOCK_ERRNO;
	    PORT_SetError(err);
	    if ((err == XP_ERRNO_EWOULDBLOCK) || (err == XP_ERRNO_EAGAIN)) {
		/* Try again later */
		SSL_TRC(30, ("%d: SSL[%d]: gather blocked",
			     SSL_GETPID(), ss->fd));
		rv = -2;
		break;
	    }
	    SSL_DBG(("%d: SSL[%d]: recv error %d", SSL_GETPID(), ss->fd, err));
	    rv = -1;
	    break;
	}

	/*
	** Try to make some data available to the reader immediately.
	*/
	/*
	 * XXX Should not give data to application until the MAC has
	 * has been verified.
	 * PK 7 Dec 95
	 */
	gs->offset += nb;
	gs->remainder -= nb;
	if (gs->encrypted) {
	    int available;
	    unsigned nout;
	    unsigned char *ibp;

	    switch (gs->state) {
	      case GS_PAD:
	      case GS_MAC:
		/*
		** Only process padding or MAC data when it completely
		** comes in
		*/
		if (gs->remainder) break;
		/* FALL THROUGH */

	      case GS_DATA:
		available = ((gs->offset - gs->writeOffset)
			     >> sec->blockShift) << sec->blockShift;
		if (available == 0) break;

		/* Decrypt the portion of data that we just recieved */
		ibp = lbp + gs->writeOffset;
		rv = (*sec->dec)(sec->readcx, ibp, &nout, available,
				 ibp, available);
		if (rv) {
		    return rv;
		}

		/* Advance write counts */
		gs->writeOffset += available;
		if (gs->state == GS_MAC) {
		    /* Don't give MAC to reader! */
		    gs->readOffset += available;
		} else {
		    (*sec->hash->update)(sec->hashcx, ibp, available);
		}
		if (gs->state == GS_PAD) {
		    /* Don't give padding to reader! */
		    gs->writeOffset -= gs->recordPadding;
		}
		break;
	    }
	}
	
	if (gs->remainder != 0) {
	    continue;
	}

	/* Probably finished this piece */
	switch (gs->state) {
	  case GS_HEADER: 
	    /* See if we need more header */
	    if (ss->enableSSL3) {
		if (gs->hdr[0] == content_handshake) {
		    if ((ss->nextHandshake == ssl_HandleClientHelloMessage) ||
			(ss->nextHandshake == ssl_HandleServerHelloMessage)) {
			rv = ssl_HandleV3Hello(ss);
			if (rv == -1) {
			    return -1;
			}
		    }
		    return -2;
		} else if (gs->hdr[0] == content_alert) {
		    if (ss->nextHandshake == ssl_HandleServerHelloMessage) {
			/* XXX This is a hack.  We're assuming that any failure
			 * XXX on the client hello is a failure to match
			 * XXX ciphers.
			 */
			PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
			return -1;
		    }
		}
	    }
	    if (gs->hdr[0] & 0x80) {
		/* All done getting header. Now get all of the data. */
		if (gs->hdr[2]) {
		    gs->count = ((gs->hdr[0] & 0x3f) << 8) | gs->hdr[1];
		} else {
		    gs->count = ((gs->hdr[0] & 0x7f) << 8) | gs->hdr[1];
		}
		gs->recordPadding = gs->hdr[2];
		if (gs->encrypted) {
		    if (gs->count & (sec->blockSize - 1)) {
			/* This is an error. Sender is misbehaving */
			SSL_DBG(("%d: SSL[%d]: sender, count=%d blockSize=%d",
				 SSL_GETPID(), ss->fd, gs->count,
				 sec->blockSize));
			PORT_SetError(XP_ERRNO_EIO);
			return -1;
		    }
		    gs->remainder = sec->hash->length;
		    gs->state = GS_MAC;
		    gs->recordLen = gs->count - gs->recordPadding
			- sec->hash->length;
		} else {
		    gs->remainder = gs->count;
		    gs->state = GS_DATA;
		    gs->recordLen = gs->count;
		}
		gs->offset = 0;
		if (gs->count > gs->buf.space) {
		    err = ssl_GrowBuf(&gs->buf, gs->count);
		    if (err) {
			return err;
		    }
		    lbp = gs->buf.buf;
		}
	    } else {
		/* Need one more byte of header */
		gs->remainder = 1;
		gs->hdr[0] |= 0x80;
		SSL_TRC(30, ("%d: SSL[%d]: gather record, another header byte",
			     SSL_GETPID(), ss->fd));
	    }
	    break;

	  case GS_MAC:
	    /* All done with MAC portion of record */
	    gs->state = GS_DATA;
	    gs->remainder = gs->count - sec->hash->length - gs->recordPadding;

	    /*
	    ** Prepare MAC by resetting it and feeding it the shared
	    ** secret
	    */
	    (*sec->hash->begin)(sec->hashcx);
	    (*sec->hash->update)(sec->hashcx, sec->rcvSecret.data,
				sec->rcvSecret.len);
	    break;

	  case GS_DATA:
	    /* All done with data portion of record */
	    if (gs->recordPadding) {
		gs->state = GS_PAD;
		gs->remainder = gs->recordPadding;
		break;
	    }
	    /* FALL THROUGH */

	  case GS_PAD:
	    /* All done with data/padding portion of record */
	    gs->state = GS_INIT;
	    gs->offset = 0;
	    gs->recordOffset = 0;
	    if (gs->encrypted) {
		unsigned ml = sec->hash->length;

		gs->recordOffset = ml;
		gs->recordPadding = 0;

		/*
		** Finish up MAC check by adding in the sequence
		** number. Padding was already added in above.
		*/
		sequenceNumber = sec->rcvSequence;
		seq[0] = (unsigned char) (sequenceNumber >> 24);
		seq[1] = (unsigned char) (sequenceNumber >> 16);
		seq[2] = (unsigned char) (sequenceNumber >> 8);
		seq[3] = (unsigned char) (sequenceNumber);
		(*sec->hash->update)(sec->hashcx, seq, 4);
		(*sec->hash->end)(sec->hashcx, mac, &ml, ml);
		if (PORT_Memcmp(mac, lbp, ml) != 0) {
		    /* MAC's didn't match... */
		    SSL_DBG(("%d: SSL[%d]: mac check failed, seq=%d",
			     SSL_GETPID(), ss->fd, sec->rcvSequence));
		    PRINT_BUF(0, (ss, "computed mac:", mac, ml));
		    PRINT_BUF(0, (ss, "received mac:", lbp, ml));
		    PORT_SetError(SSL_ERROR_BAD_MAC_READ);
		    return -1;
		}
	    }
	    if (sec) {
		/* sec might be NULL if gather is being used for socks... */
		sec->rcvSequence++;
	    }
	    PRINT_BUF(50, (ss, "recv clear record:", lbp +
			   (gs->encrypted ? sec->hash->length : 0),
			   gs->recordLen));
	    return 1;
	}
    }
    return rv;
}

int ssl_GatherRecord(SSLSocket *ss, int flags)
{
    return ssl_GatherData(ss, ss->gather, flags);
}

int ssl_StartGatherBytes(SSLSocket *ss, SSLGather *gs, int count)
{
    int rv;

    gs->state = GS_DATA;
    gs->remainder = count;
    gs->count = count;
    gs->offset = 0;
    if (count > gs->buf.space) {
	rv = ssl_GrowBuf(&gs->buf, count);
	if (rv) {
	    return rv;
	}
    }
    return ssl_GatherData(ss, gs, 0);
}

SSLGather *ssl_NewGather(void)
{
    SSLGather *gs;

    gs = (SSLGather*) PORT_ZAlloc(sizeof(SSLGather));
    if (gs) {
	gs->state = GS_INIT;
    }
    return gs;
}

void ssl_DestroyGather(SSLGather *gs)
{
    if (gs->inbuf.buf != NULL) {
	PORT_Free(gs->inbuf.buf);
    }
    if (gs) {
	PORT_FreeBlock(gs->buf.buf);
	PORT_Free(gs);
    }
}

SECStatus
ssl_HandleV3Hello(SSLSocket *ss)
{
    SSLGather *gs = ss->gather;
    SSLSessionID *sid;
    extern ssl3_SendApplicationData();

    gs->remainder = 3;
    gs->count = 0;
    ss->nextHandshake = 0;
    ss->securityHandshake = 0;
    ss->version = SSL_LIBRARY_VERSION_3_0;
    ss->sec->send = ssl3_SendApplicationData;
    if(ss->sec && (sid = ss->sec->ci->sid)) {
	sid->version = SSL_LIBRARY_VERSION_3_0;
    }
    return SECSuccess;
}
