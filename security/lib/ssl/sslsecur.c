#include "xp.h"
#include "cert.h"
#include "secitem.h"
#include "key.h"
#include "secrng.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"


extern int XP_ERRNO_EISCONN;
extern int XP_ERRNO_EWOULDBLOCK;
extern int XP_ERRNO_EBADF;
extern int SEC_ERROR_INVALID_ARGS;

#define MAX_BLOCK_CYPHER_SIZE	32

/*
** Maximum transmission amounts. These are tiny bit smaller than they
** need to be (they account for the MAC length plus some padding),
** assuming the MAC is 16 bytes long and the padding is a max of 7 bytes
** long. This gives an additional 9 bytes of slop to work within.
*/
#define MAX_STREAM_CYPHER_LEN	0x7fe0
#define MAX_BLOCK_CYPHER_LEN	0x3fe0

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
extern void RSA_InitBlinding(void);
#endif

/* XXX this is really really bad... */
SECKEYPrivateKey *ssl_server_key = NULL;
SECItem ssl_server_signed_certificate = {siBuffer, NULL, 0};
#ifdef FORTEZZA
/* ... and it is being perpetuatied */
SECItem ssl_fortezza_server_ca_list = {siBuffer, NULL, 0};
#endif

static unsigned char padbuf[MAX_BLOCK_CYPHER_SIZE];

int ssl_ReadHandshake(SSLSocket *ss)
{
    int rv = 0;

    PORT_Assert(ss->gather != 0);

    for (;;) {
	if (ss->handshake == 0) {
	    /* Previous handshake finished. Switch to next one */
	    ss->handshake = ss->nextHandshake;
	    ss->nextHandshake = 0;
	}
	if (ss->handshake == 0) {
	    /* Previous handshake finished. Switch to security handshake */
	    ss->handshake = ss->securityHandshake;
	    ss->securityHandshake = 0;
	}
	if (ss->handshake == 0) {
	    ss->gather->recordLen = 0;
	    SSL_TRC(3, ("%d: SSL[%d]: handshake is finished",
			SSL_GETPID(), ss->fd));
            /* Nasty kludge for 2.0 downgrade to call handshake callback */
	    if ( ( ss->sec != NULL ) &&
		( ss->sec->handshakeCallback != NULL ) &&
		( ss->connected == 0 ) ) {
		(ss->sec->handshakeCallback)(ss->fd, ss->sec->handshakeCallbackData);
	    }
	    ss->connected = 1;
	    ss->gather->writeOffset = 0;
	    ss->gather->readOffset = 0;
	    break;
	}
	rv = (*ss->handshake)(ss);
	if (rv < 0) {
	    break;
	}
    }
    return rv;
}

int ssl_WriteHandshake(SSLSocket *ss, const void *buf, int len)
{
    int rv;

    rv = ssl_ReadHandshake(ss);
    if (rv < 0) {
	if (rv == -2) {
	    if (ss->asyncWrites || (ss->saveBuf.len != 0)) {
		PORT_SetError(XP_ERRNO_EWOULDBLOCK);
		return -1;
	    }
	    if (ssl_SaveWriteData(ss, &ss->saveBuf, buf, len) < 0) {
		return -1;
	    }
	}
    }
    return rv;
}

int SSL_ResetHandshake(int s, int asServer)
{
#ifndef NADA_VERSION
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in ResetHandshake", SSL_GETPID(), s));
	return -1;
    }

    /* Don't waste my time */
    if (!ss->useSecurity) return 0;

    /* Reset handshake state */
    ss->connected = 0;
    ss->handshake = asServer ? ssl_BeginServerHandshake
	: ssl_BeginClientHandshake;
    ss->nextHandshake = 0;
    ss->securityHandshake = 0;
    ss->gather->state = GS_INIT;
    ss->gather->writeOffset = 0;
    ss->gather->readOffset = 0;

    /*
    ** Blow away old security state and get a fresh setup. This way if
    ** ssl was used to connect to the first point in communication, ssl
    ** can be used for the next layer.
    */
    if (ss->sec) {
	ssl_DestroySecurityInfo(ss->sec);
	ss->sec = 0;
    }
    rv = ssl_CreateSecurityInfo(ss);
    if (rv) return -1;
#endif
    return 0;
}

int
SSL_RedoHandshake(int fd)
{
    SSLSocket *ss;
    
    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in RedoHandshake", SSL_GETPID(), fd));
	PORT_SetError(XP_ERRNO_EBADF);
	return SECFailure;
    }

    if (!ss->useSecurity)
	return SECSuccess;
    
    /* XXX Not implemented for SSL v2. */
    if (ss->version == SSL_LIBRARY_VERSION_2) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    } else {
	return SSL3_RedoHandshake(ss);
    }
}

int
SSL_HandshakeCallback(int fd, SSLHandshakeCallback cb, void *client_data)
{
    SSLSocket *ss;
    
    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d}: bad socket in HandshakeCallback",
		 SSL_GETPID(), fd));
	PORT_SetError(XP_ERRNO_EBADF);
	return SECFailure;
    }

    if (!ss->useSecurity) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    PORT_Assert(ss->sec);
    ss->sec->handshakeCallback = cb;
    ss->sec->handshakeCallbackData = client_data;
    return SECSuccess;
}

int
SSL_ForceHandshake(int fd)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in ForceHandshake",
		 SSL_GETPID(), fd));
	return -1;
    }

    /* Don't waste my time */
    if (!ss->useSecurity) return 0;

    rv = ssl_ReadHandshake(ss);
    return rv;
}

/************************************************************************/

/*
** Grow a buffer to hold newLen bytes of data.
*/
int ssl_GrowBuf(SSLBuffer *b, int newLen)
{
    if (newLen > b->space) {
	if (b->buf) {
	    b->buf = (unsigned char *) PORT_ReallocBlock(b->buf, newLen);
	} else {
	    b->buf = (unsigned char *) PORT_AllocBlock(newLen);
	}
	if (!b->buf) {
	    return -1;
	}
	SSL_TRC(10, ("%d: SSL: grow buffer from %d to %d",
		     SSL_GETPID(), b->space, newLen));
	b->space = newLen;
    }
    return 0;
}

/*
** Save away write data that is trying to be written before the security
** handshake has been completed. When the handshake is completed, we will
** flush this data out.
*/
int ssl_SaveWriteData(SSLSocket *ss, SSLBuffer *buf, const void *data, int len)
{
    int newlen, rv;

    newlen = buf->len + len;
    if (newlen > buf->space) {
	rv = ssl_GrowBuf(buf, newlen);
	if (rv) {
	    return rv;
	}
    }
    SSL_TRC(5, ("%d: SSL[%d]: saving %d bytes of data (%d total saved so far)",
		 SSL_GETPID(), ss->fd, len, newlen));
    PORT_Memcpy(buf->buf + buf->len, data, len);
    buf->len = newlen;
    return 0;
}

/*
** Send saved write data. This will flush out data sent prior to a
** complete security handshake. Hopefully there won't be too much of it.
*/
int ssl_SendSavedWriteData(SSLSocket *ss, SSLBuffer *buf, SSLSendProc send)
{
    int rv, len;

    rv = 0;
    len = buf->len;
    if (len != 0) {
	SSL_TRC(5, ("%d: SSL[%d]: sending %d bytes of saved data",
		     SSL_GETPID(), ss->fd, len));
	rv = (*send)(ss, buf->buf, len, 0);
	if (rv < 0) {
	    return rv;
	} else if (rv < len) {
	    PORT_Memmove(buf->buf, buf->buf + rv, len - rv);
	    buf->len = len - rv;
	    return buf->len;
	}
	
	buf->len = 0;
    }
    return rv;
}

/************************************************************************/

#ifndef NADA_VERSION

static void CalcMAC(unsigned char *result, SSLSecurityInfo *sec,
		    unsigned char *secret, unsigned secretLen,
		    unsigned char *data, unsigned dataLen,
		    unsigned paddingLen,
		    unsigned long sequenceNumber)
{
    unsigned char padding[32];/* XXX max blocksize? */
    unsigned char seq[4];
    unsigned nout;

    /* Reset hash function */
    (*sec->hash->begin)(sec->hashcx);


    /* Feed hash the data */
    (*sec->hash->update)(sec->hashcx, secret, secretLen);
    (*sec->hash->update)(sec->hashcx, data, dataLen);
    PORT_Memset(padding, paddingLen, paddingLen);
    (*sec->hash->update)(sec->hashcx, padding, paddingLen);

    seq[0] = (unsigned char) (sequenceNumber >> 24);
    seq[1] = (unsigned char) (sequenceNumber >> 16);
    seq[2] = (unsigned char) (sequenceNumber >> 8);
    seq[3] = (unsigned char) (sequenceNumber);

    PRINT_BUF(60, (0, "calc-mac secret:", secret, secretLen));
    PRINT_BUF(60, (0, "calc-mac data:", data, dataLen));
    PRINT_BUF(60, (0, "calc-mac padding:", padding, paddingLen));
    PRINT_BUF(60, (0, "calc-mac seq:", seq, 4));

    (*sec->hash->update)(sec->hashcx, seq, 4);

    /* Get result */
    (*sec->hash->end)(sec->hashcx, result, &nout, sec->hash->length);
}

/*
** Receive some data on a socket. This reads records from the input
** stream, decrypts them and then copies them to the output buffer.
*/
static int DoRecv(SSLSocket *ss, unsigned char *out, int len, int flags)
{
    SSLSecurityInfo *sec;
    SSLGather *gs;
    int rv, amount, available;

    PORT_Assert((ss->sec != 0) && (ss->gather != 0));
    sec = ss->sec;
    gs = ss->gather;

    available = gs->writeOffset - gs->readOffset;
    if (available == 0) {
	/* Get some more data */
	if (ss->version == SSL_LIBRARY_VERSION_3_0) {
	    rv = ssl3_GatherRecord(ss, flags);
	} else {
	    rv = ssl_GatherRecord(ss, flags);
	}
	if (rv <= 0) {
	    if (rv == 0) {
		/* EOF */
		SSL_TRC(10, ("%d: SSL[%d]: ssl_recv EOF",
			     SSL_GETPID(), ss->fd));
		return rv;
	    }
	    if (rv != -2) {
		/* Some random error */
		return rv;
	    }

	    /*
	    ** Gather record is blocked waiting for more record data to
	    ** arrive. Try to process what we have already received
	    */
	} else {
	    /* Gather record has finished getting a complete record */
	}

	/* See if any clear data is now available */
	available = gs->writeOffset - gs->readOffset;
	if (available == 0) {
	    /*
	    ** No partial data is available. Force error code to
	    ** EWOULDBLOCK so that caller will try again later. Note
	    ** that the error code is probably EWOULDBLOCK already,
	    ** but if it isn't (for example, if we received a zero
	    ** length record) then this will force it to be correct.
	    */
	    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
	    return -1;
	}
	SSL_TRC(30, ("%d: SSL[%d]: partial data ready, available=%d",
		     SSL_GETPID(), ss->fd, available));
    }

    /* Dole out clear data to reader */
    amount = len;
    if (amount > available) {
	amount = available;
    }
    SSL_TRC(30, ("%d: SSL[%d]: amount=%d available=%d",
		 SSL_GETPID(), ss->fd, amount, available));
    PORT_Memcpy(out, gs->buf.buf + gs->readOffset, amount);
    gs->readOffset += amount;

    return amount;
}

/*
** Send some data in the clear. Package up data with the length header
** and send it.
*/
static int SendClear(SSLSocket *ss, const void *in, int len, int flags)
{
    SSLSecurityInfo *sec;
    int rv, amount, count;
    unsigned char aw;
    unsigned char *out;

    PORT_Assert(ss->sec != 0);
    sec = ss->sec;
    aw = ss->asyncWrites;
    ss->asyncWrites = 0;

    SSL_TRC(10, ("%d: SSL[%d]: sending %d bytes in the clear",
		 SSL_GETPID(), ss->fd, len));
    PRINT_BUF(50, (ss, "clear data:", (unsigned char*) in, len));

    count = 0;
    while (len) {
	amount = len;
	if (amount > MAX_STREAM_CYPHER_LEN) {
	    amount = MAX_STREAM_CYPHER_LEN;
	}
	if (amount + 2 > sec->writeBuf.space) {
	    rv = ssl_GrowBuf(&sec->writeBuf, amount + 2);
	    if (rv) {
		count = rv;
		goto done;
	    }
	}
	out = sec->writeBuf.buf;

	/*
	** Construct message.
	*/
	out[0] = 0x80 | MSB(amount);
	out[1] = LSB(amount);
	PORT_Memcpy(&out[2], in, amount);

	/* Now send the data */
	rv = ssl_DefSend(ss, out, amount + 2, flags);
	if (rv < 0) {
	    if (PORT_GetError() == XP_ERRNO_EWOULDBLOCK) {
		rv = 0;
	    } else {
		/* Return short write if some data already went out... */
		if (count == 0)
		    count = rv;
		goto done;
	    }
	}

	if (rv < (amount + 2)) {
	    /* Short write.  Save the data and return. */
	    if (ssl_SaveWriteData(ss, &ss->pendingBuf, out + rv,
				   amount + 2 - rv) == SECFailure) {
		count = -1;
	    }
	    sec->sendSequence++;
	    goto done;
	}

	sec->sendSequence++;
	in = (void*) (((unsigned char*)in) + amount);
	count += amount;
	len -= amount;
    }

  done:
    ss->asyncWrites = aw;
    return count;
}

/*
** Send some data, when using a stream cypher. Stream cyphers have a
** block size of 1. Package up the data with the length header
** and send it.
*/
static int SendStream(SSLSocket *ss, const void *in, int len, int flags)
{
    SSLSecurityInfo *sec;
    unsigned char aw, *out;
    int rv, amount, count, buflen;
    unsigned nout, ml;

    PORT_Assert(ss->sec != 0);
    sec = ss->sec;
    aw = ss->asyncWrites;
    ss->asyncWrites = 0;

    SSL_TRC(10, ("%d: SSL[%d]: sending %d bytes using stream cypher",
		 SSL_GETPID(), ss->fd, len));
    PRINT_BUF(50, (ss, "clear data:", (unsigned char*) in, len));

    count = 0;
    ml = sec->hash->length;
    while (len) {
	amount = len;
	if (amount > MAX_STREAM_CYPHER_LEN) {
	    amount = MAX_STREAM_CYPHER_LEN;
	}
	buflen = amount + 2 + ml;
	if (buflen > sec->writeBuf.space) {
	    rv = ssl_GrowBuf(&sec->writeBuf, buflen);
	    if (rv) {
		count = rv;
		goto done;
	    }
	}
	out = sec->writeBuf.buf;
	nout = amount + ml;
	out[0] = 0x80 | MSB(nout);
	out[1] = LSB(nout);

	/* Calculate MAC */
	CalcMAC(out+2, sec, sec->sendSecret.data, sec->sendSecret.len,
		(unsigned char*) in, amount, 0, sec->sendSequence);

	/* Encrypt MAC */
	rv = (*sec->enc)(sec->writecx, out+2, &nout, ml, out+2, ml);
	if (rv) goto loser;

	/* Encrypt data from caller */
	rv = (*sec->enc)(sec->writecx, out+2+ml, &nout, amount,
			 (unsigned char*) in, amount);
	if (rv) goto loser;

	PRINT_BUF(50, (ss, "encrypted data:", out, buflen));

	rv = ssl_DefSend(ss, out, buflen, flags);
	if (rv < 0) {
	    if (PORT_GetError() == XP_ERRNO_EWOULDBLOCK) {
		rv = 0;
	    } else {
		SSL_TRC(10, ("%d: SSL[%d]: send stream error %d",
			     SSL_GETPID(), ss->fd, PORT_GetError()));
		/* Return short write if some data already went out... */
		if (count == 0)
		    count = rv;
		goto done;
	    }
	}

	if (rv < buflen) {
	    /* Short write.  Save the data and return. */
	    if (ssl_SaveWriteData(ss, &ss->pendingBuf, out + rv,
				  buflen - rv) == SECFailure) {
		count = -1;
	    }
	    sec->sendSequence++;
	    goto done;
	}

	sec->sendSequence++;
	in = (void*) (((unsigned char*)in) + amount);
	count += amount;
	len -= amount;
    }

  done:
    ss->asyncWrites = aw;
    return count;

  loser:
    ss->asyncWrites = aw;
    return -1;
}

/*
** Send some data, when using a block cipher. Package up the data with
** the length header and send it.
*/
/* XXX assumes blocksize is > 7 */
static int SendBlock(SSLSocket *ss, const void *in, int len, int flags)
{
    SSLSecurityInfo *sec;
    unsigned char aw, *out, *op;
    int rv, amount, count, buflen;
    unsigned nout, padding, ml, hlen;

    PORT_Assert(ss->sec != 0);
    sec = ss->sec;
    aw = ss->asyncWrites;
    ss->asyncWrites = 0;

    SSL_TRC(10, ("%d: SSL[%d]: sending %d bytes using block cypher",
		 SSL_GETPID(), ss->fd, len));
    PRINT_BUF(50, (ss, "clear data:", (unsigned char*) in, len));

    count = 0;
    ml = sec->hash->length;
    while (len) {
	/* Figure out how much to send, including mac and padding */
	amount = len;
	if (amount > MAX_BLOCK_CYPHER_LEN) {
	    amount = MAX_BLOCK_CYPHER_LEN;
	}
	nout = amount + ml;
	padding = nout & (sec->blockSize - 1);
	if (padding) {
	    hlen = 3;
	    padding = sec->blockSize - padding;
	    nout += padding;
	} else {
	    hlen = 2;
	}
	buflen = hlen + nout;
	if (buflen > sec->writeBuf.space) {
	    rv = ssl_GrowBuf(&sec->writeBuf, buflen);
	    if (rv) {
		count = rv;
		goto done;
	    }
	}
	out = sec->writeBuf.buf;

	/* Construct header */
	op = out;
	if (padding) {
	    *op++ = MSB(nout);
	    *op++ = LSB(nout);
	    *op++ = padding;
	} else {
	    *op++ = 0x80 | MSB(nout);
	    *op++ = LSB(nout);
	}

	/* Calculate MAC */
	CalcMAC(op, sec, sec->sendSecret.data, sec->sendSecret.len,
		(unsigned char*) in, amount, padding, sec->sendSequence);
	op += ml;

	/* Copy in the input data */
	/* XXX could eliminate the copy by folding it into the encryption */
	PORT_Memcpy(op, in, amount);
	op += amount;
	if (padding) {
	    PORT_Memset(op, padding, padding);
	    op += padding;
	}

	/* Encrypt result */
	rv = (*sec->enc)(sec->writecx, out+hlen, &nout, buflen-hlen,
			 out+hlen, op - (out + hlen));
	if (rv) goto loser;

	PRINT_BUF(50, (ss, "final xmit data:", out, op - out));

	rv = ssl_DefSend(ss, out, op - out, flags);
	if (rv < 0) {
	    if (PORT_GetError() == XP_ERRNO_EWOULDBLOCK) {
		rv = 0;
	    } else {
		SSL_TRC(10, ("%d: SSL[%d]: send block error %d",
			     SSL_GETPID(), ss->fd, PORT_GetError()));
		/* Return short write if some data already went out... */
		if (count == 0)
		    count = rv;
		goto done;
	    }
	}

	if (rv < (op - out)) {
	    /* Short write.  Save the data and return. */
	    if (ssl_SaveWriteData(ss, &ss->pendingBuf, out + rv,
				  op - out - rv) == SECFailure) {
		count = -1;
	    }
	    sec->sendSequence++;
	    goto done;
	}

	sec->sendSequence++;
	in = (void*) (((unsigned char*)in) + amount);
	count += amount;
	len -= amount;
    }

  done:
    ss->asyncWrites = aw;
    return count;

  loser:
    ss->asyncWrites = aw;
    return -1;
}

void ssl_ChooseProcs(SSLSocket *ss)
{
    SSLSecurityInfo *sec;

    PORT_Assert(ss->sec != 0);
    sec = ss->sec;
    PORT_Assert(sec->hashcx != 0);

    ss->gather->encrypted = 1;
    if (sec->blockSize > 1) {
	sec->send = SendBlock;
    } else {
	sec->send = SendStream;
    }
}

/************************************************************************/

/* XXX keep server key encrypted with an rc4 key that we generate on the fly */

int SSL_ConfigSecureServer(struct SECItemStr *signedCertItem,
			   struct SECKEYPrivateKeyStr *key,
			   struct CERTCertificateListStr *certChain,
			   struct CERTDistNamesStr *caNames)
{
    int rv;

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    RSA_InitBlinding();
#endif

    rv = SECITEM_CopyItem(0, &ssl_server_signed_certificate, signedCertItem);
    if (rv) {
	return rv;
    }

    ssl_server_key = SECKEY_CopyPrivateKey(key);
    if (!ssl_server_key) {
	return -1;
    }

    if (ssl3_server_cert_chain != NULL) {
	CERT_DestroyCertificateList(ssl3_server_cert_chain);
    }
    ssl3_server_cert_chain = certChain;
    if (ssl3_server_ca_list != NULL) {
	CERT_FreeDistNames(ssl3_server_ca_list);
    }
    ssl3_server_ca_list = caNames;
    
    SSL3_CreateExportRSAKeys(ssl_server_key);
    return 0;
}

/************************************************************************/

int ssl_CreateSecurityInfo(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLGather *gs;
    int rv;

    if (ss->sec) {
	return 0;
    }

    RNG_GenerateGlobalRandomBytes(padbuf, sizeof(padbuf));

    ss->sec = sec = (SSLSecurityInfo*) PORT_ZAlloc(sizeof(SSLSecurityInfo));
    if (!sec) {
	goto loser;
    }
    sec->ci = ci = (SSLConnectInfo*) PORT_ZAlloc(sizeof(SSLConnectInfo));
    if (!ci) {
	goto loser;
    }
    if ((gs = ss->gather) == 0) {
	ss->gather = gs = ssl_NewGather();
	if (!gs) {
	    goto loser;
	}
    }

    rv = ssl_GrowBuf(&sec->writeBuf, 4096);
    if (rv) {
	goto loser;
    }
    rv = ssl_GrowBuf(&gs->buf, 4096);
    if (rv) {
	goto loser;
    }

    sec->send = SendClear;
    sec->blockSize = 1;
    sec->blockShift = 0;

    /* Provide default implementation of hooks */
    sec->authCertificate = SSL_AuthCertificate;
    sec->authCertificateArg = (void *)CERT_GetDefaultCertDB();
    sec->getClientAuthData = NULL;
    sec->handleBadCert = NULL;
    sec->badCertArg = NULL;
#ifdef FORTEZZA
    sec->fortezzaCardSelect = NULL;
    sec->fortezzaCardArg = NULL;
    sec->fortezzaGetPin = NULL;
    sec->fortezzaPinArg = NULL;
    sec->fortezzaCertificateSelect = NULL;
    sec->fortezzaCertificateArg = NULL;
    sec->fortezzaAlert = NULL;
    sec->fortezzaAlertArg = NULL;
#endif
    sec->url = NULL;

    SSL_TRC(5, ("%d: SSL[%d]: security info created",
		SSL_GETPID(), ss->fd));
    return 0;

  loser:
    return -1;
}

int ssl_CopySecurityInfo(SSLSocket *ss, SSLSocket *os)
{
    SSLSecurityInfo *sec, *osec;
    int rv;

    rv = ssl_CreateSecurityInfo(ss);
    if (rv < 0) {
	goto loser;
    }
    sec = ss->sec;
    osec = os->sec;

    sec->send = osec->send;
    sec->isServer = osec->isServer;

    sec->sendSequence = osec->sendSequence;
    sec->rcvSequence = osec->rcvSequence;

    if (osec->hash && osec->hashcx) {
	sec->hash = osec->hash;
	sec->hashcx = osec->hash->clone(osec->hashcx);
    } else {
	sec->hash = NULL;
	sec->hashcx = NULL;
    }

    SECITEM_CopyItem(0, &sec->sendSecret, &osec->sendSecret);
    SECITEM_CopyItem(0, &sec->rcvSecret, &osec->rcvSecret);

    sec->keyBits = osec->keyBits;
    sec->secretKeyBits = osec->secretKeyBits;
    sec->peerCert = CERT_DupCertificate(osec->peerCert);

    sec->readcx = osec->readcx;/* XXX */
    sec->writecx = osec->writecx;/* XXX */
    sec->enc = osec->enc;
    sec->dec = osec->dec;
    sec->destroy = 0;/* XXX */

    sec->blockShift = osec->blockShift;
    sec->blockSize = osec->blockSize;

    sec->cache = osec->cache;
    sec->uncache = osec->uncache;

    sec->authCertificate = osec->authCertificate;
    sec->authCertificateArg = osec->authCertificateArg;
    sec->getClientAuthData = osec->getClientAuthData;
    sec->getClientAuthDataArg = osec->getClientAuthDataArg;
    sec->handleBadCert = osec->handleBadCert;
    sec->badCertArg = osec->badCertArg;
    sec->handshakeCallback = osec->handshakeCallback;
    sec->handshakeCallbackData = osec->handshakeCallbackData;
#ifdef FORTEZZA
    sec->fortezzaCardSelect = osec->fortezzaCardSelect;
    sec->fortezzaCardArg = osec->fortezzaCardArg;
    sec->fortezzaGetPin = osec->fortezzaGetPin;
    sec->fortezzaPinArg = osec->fortezzaPinArg;
    sec->fortezzaCertificateSelect = osec->fortezzaCertificateSelect;
    sec->fortezzaCertificateArg = osec->fortezzaCertificateArg;
    sec->fortezzaAlert = osec->fortezzaAlert;
    sec->fortezzaAlertArg = osec->fortezzaAlertArg;
#endif
    
    return 0;

  loser:
    return -1;
}

void ssl_DestroySecurityInfo(SSLSecurityInfo *sec)
{
    SSLConnectInfo *ci;

    if (sec != 0) {
	/* Destroy MAC */
	if (sec->hash && sec->hashcx) {
	    (*sec->hash->destroy)(sec->hashcx, PR_TRUE);
	    sec->hashcx = 0;
	}
	SECITEM_ZfreeItem(&sec->sendSecret, PR_FALSE);
	SECITEM_ZfreeItem(&sec->rcvSecret, PR_FALSE);

	/* Destroy ciphers */
	if (sec->destroy) {
	    (*sec->destroy)(sec->readcx, PR_TRUE);
	    (*sec->destroy)(sec->writecx, PR_TRUE);
	} else {
	    PORT_Assert(sec->readcx == 0);
	    PORT_Assert(sec->writecx == 0);
	}

	/* etc. */
	PORT_FreeBlock(sec->writeBuf.buf);
	CERT_DestroyCertificate(sec->peerCert);
	ci = sec->ci;
	if (ci != 0) {
	    ssl_DestroyConnectInfo(sec);
	    sec->ci = 0;
	}
	sec->readcx = 0;
	sec->writecx = 0;
	sec->writeBuf.buf = 0;
	if ( sec->url != NULL ) {
	    PORT_Free(sec->url);
	}
	
	PORT_Free(sec);
    }
}

/************************************************************************/

int ssl_SecureConnect(SSLSocket *ss, const void *sa, int namelen)
{
    int rv;

    PORT_Assert(ss->sec != 0);

    /* First connect to server */
    rv = XP_SOCK_CONNECT(ss->fd, (struct sockaddr*) sa, namelen);
    if (rv < 0) {
	int olderrno = XP_SOCK_ERRNO;
	PORT_SetError(olderrno);
	SSL_DBG(("%d: SSL[%d]: connect failed, errno=%d",
		 SSL_GETPID(), ss->fd, olderrno));
	if (olderrno == XP_ERRNO_EISCONN) {
	    /*
	    ** Connected after all. Caller was Using a non-blocking
	    ** connect. Go ahead and establish secure connection.
	    */
	} else
	    return rv;
    }

    /* Create security data */
    SSL_TRC(5, ("%d: SSL[%d]: secure connect completed, starting handshake",
		SSL_GETPID(), ss->fd));

    if ( ss->handshakeAsServer ) {
	ss->securityHandshake = ssl_BeginServerHandshake;
    } else {
	ss->securityHandshake = ssl_BeginClientHandshake;
    }
    
    if (ss->delayedHandshake == 0) {
	rv = ssl_ReadHandshake(ss);
	if (rv < 0) {
	    if (rv != -2) {
		return rv;
	    }
	}
    }
    return 0;
}

int
ssl_SecureAccept(SSLSocket *ss, void *addr, int* addrlenp)
{
    SSLSocket *ns;
    int rv, newfd;

    /* First accept connection */
    newfd = (*ssl_accept_func)(ss->fd, (struct sockaddr *) addr, addrlenp);
    if (newfd < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
	SSL_DBG(("%d: SSL[%d]: accept failed, errno=%d",
		 SSL_GETPID(), ss->fd, PORT_GetError()));
	return newfd;
    }

    /* Create new socket */
    ns = ssl_DupSocket(ss, newfd);
    if (!ns) {
	XP_SOCK_CLOSE(newfd);
	return -1;
    }

    /* Now start server connection handshake with client */
    if ( ns->handshakeAsClient ) {
	ns->handshake = ssl_BeginClientHandshake;
    } else {
	ns->handshake = ssl_BeginServerHandshake;
    }
    if (ns->delayedHandshake == 0) {
	rv = ssl_ReadHandshake(ns);
	if (rv < 0) {
	    if (rv != -2) {
		goto lossage;
	    }
	}
    }
    return newfd;

  lossage:
    (*ns->ops->close)(ns);
    return rv;
}

int
ssl_SecureSocksConnect(SSLSocket *ss, const void *sa, int salen)
{
    int rv;

    PORT_Assert((ss->socks != 0) && (ss->sec != 0));

    /* First connect to socks daemon */
    rv = ssl_SocksConnect(ss, sa, salen);
    if (rv < 0) {
	return rv;
    }

    if ( ss->handshakeAsServer ) {
	ss->securityHandshake = ssl_BeginServerHandshake;
    } else {
	ss->securityHandshake = ssl_BeginClientHandshake;
    }
    
    if (ss->delayedHandshake == 0) {
	rv = ssl_ReadHandshake(ss);
	if (rv < 0) {
	    if (rv != -2) {
		return rv;
	    }
	}
    }
    return 0;
}

int ssl_SecureSocksAccept(SSLSocket *ss, void *addr, int *addrlenp)
{
    SSLSocket *ns;
    int rv, newfd;

    newfd = ssl_SocksAccept(ss, addr, addrlenp);
    if (newfd < 0) {
	return newfd;
    }

    /* Create new socket */
    ns = ssl_FindSocket(newfd);
    PORT_Assert(ns != NULL);

    if ( ns->handshakeAsClient ) {
	ns->handshake = ssl_BeginClientHandshake;
    } else {
	ns->handshake = ssl_BeginServerHandshake;
    }
    if (ns->delayedHandshake == 0) {
	rv = ssl_ReadHandshake(ns);
	if (rv < 0) {
	    if (rv != -2) {
		goto lossage;
	    }
	}
    }
    return newfd;

  lossage:
    (*ns->ops->close)(ns);
    return rv;
}

int
ssl_SecureImportFd(SSLSocket *ss, int fd)
{
    SSLSocket *ns;
    int rv;

    ns = ssl_DupSocket(ss, fd);
    if (!ns) {
	return -1;
    }
    
    if ( ns->handshakeAsClient ) {
	ns->securityHandshake = ssl_BeginClientHandshake;
    } else {
	ns->securityHandshake = ssl_BeginServerHandshake;
    }
    if(ns->delayedHandshake == 0) {
	rv = ssl_ReadHandshake(ns);
	if (rv < 0) {
	    if (rv != -2) {
		goto lossage;
	    }
	}
    }
    return fd;
    
  lossage:
    return rv;
}

int
ssl_SecureClose(SSLSocket *ss)
{
    if (ss->version == SSL_LIBRARY_VERSION_3_0) {
	(void) SSL3_SendAlert(ss, alert_warning, close_notify);
    }
    return ssl_DefClose(ss);
}

/************************************************************************/

int
ssl_SecureRecv(SSLSocket *ss, void *buf, int len, int flags)
{
    SSLSecurityInfo *sec;
    int rv;

    PORT_Assert(ss->sec != 0);
    sec = ss->sec;
    
    if (ss->pendingBuf.len != 0) {
	rv = ssl_SendSavedWriteData(ss, &ss->pendingBuf, ssl_DefSend);
	if ((rv < 0) && (PORT_GetError() != XP_ERRNO_EWOULDBLOCK)) {
	    return -1;
	}
    }
    
    if (ss->handshake || ss->nextHandshake || ss->securityHandshake) {
	rv = ssl_ReadHandshake(ss);
	if (rv < 0) {
	    if (rv == -2) {
		rv = -1;
		PORT_SetError(XP_ERRNO_EWOULDBLOCK);
	    }
	    return rv;
	}
	rv = ssl_SendSavedWriteData(ss, &ss->saveBuf, sec->send);
	if (rv < 0) {
	    return -1;
	}
	PORT_SetError(XP_ERRNO_EWOULDBLOCK);
	return -1;
    }
    if (len == 0) return 0;

    rv = DoRecv(ss, (unsigned char*) buf, len, flags);
    SSL_TRC(2, ("%d: SSL[%d]: recving %d bytes securely (errno=%d)",
		SSL_GETPID(), ss->fd, rv, PORT_GetError()));
    return rv;
}

int ssl_SecureRead(SSLSocket *ss, void *buf, int len)
{
    return ssl_SecureRecv(ss, buf, len, 0);
}

int ssl_SecureSend(SSLSocket *ss, const void *buf, int len, int flags)
{
    SSLSecurityInfo *sec;
    int rv;

    PORT_Assert(ss->sec != 0);
    sec = ss->sec;

    if (len == 0) return 0;
    PORT_Assert(buf != NULL);
    
    if (ss->pendingBuf.len != 0) {
	PORT_Assert(ss->pendingBuf.len > 0);
	rv = ssl_SendSavedWriteData(ss, &ss->pendingBuf, ssl_DefSend);
	if (rv < 0) {
	    return rv;
	}
	if (ss->pendingBuf.len != 0) {
	    PORT_Assert(ss->pendingBuf.len > 0);
	    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
	    return -1;
	}
    }

    if (ss->handshake || ss->nextHandshake || ss->securityHandshake) {
	rv = ssl_WriteHandshake(ss, buf, len);
	if (rv < 0) {
	    if (rv == -2) {
		return len;
	    }
	    return rv;
	}
	rv = ssl_SendSavedWriteData(ss, &ss->saveBuf, sec->send);
	if (rv < 0) {
	    return rv;
	}
    }

    SSL_TRC(2, ("%d: SSL[%d]: SecureSend: sending %d bytes",
		SSL_GETPID(), ss->fd, len));

    /* Send out the data */
    return (*sec->send)(ss, (unsigned char*)buf, len, flags);
}

int ssl_SecureWrite(SSLSocket *ss, const void *buf, int len)
{
    return ssl_SecureSend(ss, buf, len, 0);
}
#endif /* !NADA_VERSION */

int
SSL_BadCertHook(int fd, SSLBadCertHandler f, void *arg)
{
    SSLSocket *ss;
    int rv;
    
    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SSLBadCertHook",
		 SSL_GETPID(), fd));
	return -1;
    }

#ifndef NADA_VERSION
    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return(rv);
    }
    ss->sec->handleBadCert = f;
    ss->sec->badCertArg = arg;
#endif
    return(0);
}

/*
 * Allow the application to pass the url or hostname into the SSL library
 * so that we can do some checking on it.
 */
int
SSL_SetURL(int fd, char *url)
{
    SSLSocket *ss;
    ss = ssl_FindSocket(fd);
    
    if ( ss->sec ) {
	if ( ss->sec->url ) {
	    PORT_Free(ss->sec->url);
	}

	ss->sec->url = PORT_Strdup(url);
	if ( ss->sec->url == NULL ) {
	    return(-1);
	}
    }

    return(0);
}

int
SSL_DataPending(int fd)
{
    SSLSocket *ss;
    ss = ssl_FindSocket(fd);

    if (ss && ss->useSecurity) {
	return ss->gather->writeOffset - ss->gather->readOffset;
    }

    return 0;
}

int
SSL_InvalidateSession(int fd)
{
    SSLSocket *ss;
    ss = ssl_FindSocket(fd);

    if (ss && ss->sec && ss->sec->ci && ss->sec->ci->sid) {
	ss->sec->uncache(ss->sec->ci->sid);
	return SECSuccess;
    }
    return SECFailure;
}

SECItem *
SSL_GetSessionID(int fd)
{
  SSLSocket *ss;
  SECItem *item;
  SSLSessionID *sid;

  ss = ssl_FindSocket(fd);

  if (ss && ss->useSecurity && ss->connected && ss->sec && ss->sec->ci &&
      ss->sec->ci->sid) {
    sid = ss->sec->ci->sid;
    item = (SECItem *)PORT_Alloc(sizeof(SECItem));
    if (sid->version == SSL_LIBRARY_VERSION_2) {
      item->len = SSL_SESSIONID_BYTES;
      item->data = PORT_Alloc(item->len);
      PORT_Memcpy(item->data, sid->u.ssl2.sessionID, item->len);
    } else {
      item->len = sid->u.ssl3.sessionIDLength;
      item->data = PORT_Alloc(item->len);
      PORT_Memcpy(item->data, sid->u.ssl3.sessionID, item->len);
    }
    return item;
  } else {
    return NULL;
  }
}
