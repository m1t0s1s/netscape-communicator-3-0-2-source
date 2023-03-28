/*
 * SSL stuff. XXX
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: sslcon.c,v 1.105.2.8 1997/05/24 00:24:38 jwz Exp $
 */

#include "xp.h"
#include "cert.h"
#include "secitem.h"
#include "secrng.h"
#include "sechash.h"
#include "crypto.h"
#include "key.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"
#include "ssl3prot.h"
#include "sslerr.h"
#include "pk11func.h"

extern int SEC_ERROR_INVALID_ARGS;
extern int SEC_ERROR_LIBRARY_FAILURE;
extern int SEC_ERROR_BAD_DATA;
extern int SSL_ERROR_BAD_CERTIFICATE;
extern int SSL_ERROR_BAD_CLIENT;
extern int SSL_ERROR_BAD_SERVER;
extern int SSL_ERROR_EXPORT_ONLY_SERVER;
extern int SSL_ERROR_NO_CERTIFICATE;
extern int SSL_ERROR_NO_CYPHER_OVERLAP;
extern int SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE;
extern int SSL_ERROR_UNSUPPORTED_VERSION;
extern int SSL_ERROR_US_ONLY_SERVER;
extern int SSL_ERROR_SSL2_DISABLED;
extern int SSL_ERROR_SSL_DISABLED;
extern int XP_ERRNO_EIO;

static PRBool policyWasSet;

/* This ordered list is indexed by XXXXXXXXXXXXX */
static unsigned char allCipherSuites[] = {
    0,						0,    0,
    SSL_CK_RC4_128_WITH_MD5,			0x00, 0x80,
    SSL_CK_RC4_128_EXPORT40_WITH_MD5,		0x00, 0x80,
    SSL_CK_RC2_128_CBC_WITH_MD5,		0x00, 0x80,
    SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5,	0x00, 0x80,
    SSL_CK_IDEA_128_CBC_WITH_MD5,		0x00, 0x80,
    SSL_CK_DES_64_CBC_WITH_MD5,			0x00, 0x40,
    SSL_CK_DES_192_EDE3_CBC_WITH_MD5,		0x00, 0xC0,
    0,						0,    0
};

static unsigned char *preferredCipher;



/*
** Put a string tag in the library so that we can examine an executable
** and see what kind of security it supports.
*/
char *ssl_version = "SECURITY_VERSION:"
			" +us"
			" +export"
#ifdef TRACE
			" +trace"
#endif
#ifdef DEBUG
			" +debug"
#endif
			;

char *ssl_cipherName[] = {
    "unknown",
    "RC4",
    "RC4-Export",
    "RC2-CBC",
    "RC2-CBC-Export",
    "IDEA-CBC",
    "DES-CBC",
    "DES-EDE3-CBC",
#ifdef FORTEZZA
    "unknown",
    "Fortezza",
#endif
};

static unsigned char *cipherSpecs = 0;
static unsigned sizeCipherSpecs = 0;

/* bit-masks, showing which SSLv2 suites are allowed */
static unsigned int	allowedByPolicy;
static unsigned int	maybeAllowedByPolicy;
static unsigned int	chosenPreference;

/* bit values for the above two bit masks */
#define SSL_CB_RC4_128_WITH_MD5              (1 << SSL_CK_RC4_128_WITH_MD5)
#define SSL_CB_RC4_128_EXPORT40_WITH_MD5     (1 << SSL_CK_RC4_128_EXPORT40_WITH_MD5)
#define SSL_CB_RC2_128_CBC_WITH_MD5          (1 << SSL_CK_RC2_128_CBC_WITH_MD5)
#define SSL_CB_RC2_128_CBC_EXPORT40_WITH_MD5 (1 << SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5)
#define SSL_CB_IDEA_128_CBC_WITH_MD5         (1 << SSL_CK_IDEA_128_CBC_WITH_MD5)
#define SSL_CB_DES_64_CBC_WITH_MD5           (1 << SSL_CK_DES_64_CBC_WITH_MD5)
#define SSL_CB_DES_192_EDE3_CBC_WITH_MD5     (1 << SSL_CK_DES_192_EDE3_CBC_WITH_MD5)

/* Must be in the USA if we can do triple DES. */
int
SSL_IsDomestic(void)
{
    if (! policyWasSet)
    	abort();
    return (0 != (allowedByPolicy & SSL_CB_DES_192_EDE3_CBC_WITH_MD5));
}

/* called for the URL "about:" (the about box) 
 * Taher says our RSA license agreement requires us to tell the world
 * what RSA ciphers we're using/shipping.  
 * Perhaps this should unconditionally list them all...
 */

unsigned long
SSL_SecurityCapabilities(void)
{
    unsigned long ret;
    unsigned int  allowed;

    if (! policyWasSet)
    	abort();

    ret = (SSL_SC_RSA | SSL_SC_MD2 | SSL_SC_MD5);

    allowed = allowedByPolicy;
    if (allowed & ( SSL_CB_RC2_128_CBC_WITH_MD5 |
		    SSL_CB_RC2_128_CBC_EXPORT40_WITH_MD5))
	ret |= SSL_SC_RC2_CBC;

    if (allowed & ( SSL_CB_RC4_128_WITH_MD5 |
    		    SSL_CB_RC4_128_EXPORT40_WITH_MD5 ))
	ret |= SSL_SC_RC4;

    if (allowed & SSL_CB_DES_64_CBC_WITH_MD5)
	ret |= SSL_SC_DES_CBC;

    if (allowed & SSL_CB_DES_192_EDE3_CBC_WITH_MD5)
	ret |= SSL_SC_DES_EDE3_CBC;

    if (allowed & SSL_CK_IDEA_128_CBC_WITH_MD5)
	ret |= SSL_SC_IDEA_CBC;

    return ret;
}


static SECStatus
ConstructCipherSpecs(SSLSocket *ss, unsigned char **cipherSpecs,
				    unsigned *sizeCipherSpecs)
{
    unsigned char *	cs	= NULL;
    unsigned int	allowed;
    unsigned int	count;
    unsigned int	kind;
    int 		ssl3_count;
    int 		final_count;
    SECStatus 		rv;

    /* First, count number of SSL2 ciphers enabled */
    count = 0;
    allowed = allowedByPolicy & chosenPreference;
    while (allowed) {
    	if (allowed & 1) 
	    ++count;
	allowed >>= 1;
    }

    /* ask SSL3 how many cipher suites it has. */
    rv = SSL3_ConstructV2CipherSpecsHack(ss, NULL, &ssl3_count);

    if (rv < 0) 
    	return rv;

    count += ssl3_count;

    /* Allocate memory to hold cipher specs */
    if (count > 0)
	cs = (unsigned char*) PORT_Alloc(count * 3);
    if (cs == NULL) 
    	return SECFailure;

    if (*cipherSpecs != NULL) {
	PORT_Free(*cipherSpecs);
    }
    *cipherSpecs = cs;
    *sizeCipherSpecs = count * 3;

    /* fill in cipher specs for SSL2 cipher suites */
    allowed = (allowedByPolicy & chosenPreference) >> 1;
    for (kind = 1; allowed; allowed >>= 1, ++kind) {
    	if (!(allowed & 1)) 
	    continue;

    	switch (kind) {
	case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5 :
	case SSL_CK_RC2_128_CBC_WITH_MD5 :
	case SSL_CK_RC4_128_EXPORT40_WITH_MD5 :
	case SSL_CK_RC4_128_WITH_MD5 :
	case SSL_CK_IDEA_128_CBC_WITH_MD5 :
	    *cs++ = kind;
	    *cs++ = 0x00;
	    *cs++ = 0x80;
	    break;

	case SSL_CK_DES_64_CBC_WITH_MD5 :
	    *cs++ = kind;
	    *cs++ = 0x00;
	    *cs++ = 0x40;
	    break;

	case SSL_CK_DES_192_EDE3_CBC_WITH_MD5 :
	    *cs++ = kind;
	    *cs++ = 0x00;
	    *cs++ = 0xC0;
	    break;
    	}
    }

    /* now have SSL3 add its suites onto the end */
    rv = SSL3_ConstructV2CipherSpecsHack(ss, cs, &final_count);
    
    /* adjust for any difference between first pass and second pass */
    *sizeCipherSpecs -= (ssl3_count - final_count) * 3;

    return rv;
}

SECStatus
SSL_SetPolicy(long which, int policy)
{
    unsigned int bitMask;
    SECStatus rv = SECSuccess;
    
    if ((which >> 8) != 0xFF) {
	rv = SSL3_SetPolicy(which, policy);
	goto done;
    }

    which &= 0x000f;
    bitMask = 1 << which;

    if (policy == SSL_ALLOWED) {
	allowedByPolicy 	|= bitMask;
	maybeAllowedByPolicy 	|= bitMask;
    } else if (policy == SSL_RESTRICTED) {
    	allowedByPolicy 	&= ~bitMask;
	maybeAllowedByPolicy 	|= bitMask;
    } else {
    	allowedByPolicy 	&= ~bitMask;
    	maybeAllowedByPolicy 	&= ~bitMask;
    }

done:
    policyWasSet = PR_TRUE;
    return rv;
}

/* called from secmoz.c whenever an SSL preference changes. */
SECStatus
SSL_EnableCipher(long which, int enabled)
{
    SECStatus 	 rv 	  = SECSuccess;
    unsigned int bitMask;
    
    if ((which >> 8) != 0xFF) {
	rv = SSL3_EnableCipher(which, enabled);
	goto done;
    }
    which &= 0x000f;
    bitMask = 1 << which;

    if (enabled)
	chosenPreference |= bitMask;
    else
    	chosenPreference &= ~bitMask;

done:
    
    if (cipherSpecs != NULL) {
	PORT_Free(cipherSpecs);
	cipherSpecs = NULL;
    }

    return rv;
}


/************************************************************************/

/*
** XXX this needs to deal with SOCKS! We need to know the address of who
** XXX we were reallying trying to connect to, not the address of the socks
** XXX daemon. This is easy for the client, but hard for the server.
*/
int
ssl_GetPeerInfo(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    struct sockaddr_in sin;
    int addrlen, rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    /* if we've got a better idea of who we're talking to use that */
    if ((ss->peer != 0) && (ss->port != 0)) {
	ci->peer = ss->peer;
	ci->port = ss->port;
	return 0;
    }

    addrlen = sizeof(sin);
    PORT_Memset(&sin, 0, sizeof(sin));
    rv = XP_SOCK_GETPEERNAME(ss->fd, (struct sockaddr*)&sin, &addrlen);
    if (rv < 0) {
	PORT_SetError(XP_SOCK_ERRNO);
	return -1;
    }
    if (addrlen != sizeof(sin)) {
	SSL_DBG(("%d: SSL[%d]: getpeername: expected %d, got %d",
		 SSL_GETPID(), ss->fd, sizeof(sin), addrlen));
	PORT_SetError(XP_ERRNO_EIO);
	return -1;
    }
    ci->peer = sin.sin_addr.s_addr;
    ci->port = sin.sin_port;
    return 0;
}

/************************************************************************/

static SECStatus
CreateMAC(SSLSecurityInfo *sec, SECItem *readKey,
	  SECItem *writeKey, unsigned char cipherChoice)
{
    switch (cipherChoice) {

      case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
      case SSL_CK_RC2_128_CBC_WITH_MD5:
      case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
      case SSL_CK_RC4_128_WITH_MD5:
      case SSL_CK_DES_64_CBC_WITH_MD5:
      case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	sec->hash = &SECHashObjects[HASH_AlgMD5];
	SECITEM_CopyItem(0, &sec->sendSecret, writeKey);
	SECITEM_CopyItem(0, &sec->rcvSecret, readKey);
	break;

      default:
	PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	return SECFailure;
    }
    sec->hashcx = (*sec->hash->create)();
    if (sec->hashcx == NULL)
	return SECFailure;
    return SECSuccess;
}

/************************************************************************/

static int
GetSendBuffer(SSLSocket *ss, int len)
{
    SSLConnectInfo *ci;
    int rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    ci = ss->sec->ci;

    if (len < 128) {
	len = 128;
    }
    if (len > ci->sendBuf.space) {
	rv = ssl_GrowBuf(&ci->sendBuf, len);
	if (rv < 0) {
	    SSL_DBG(("%d: SSL[%d]: GetSendBuffer failed, tried to get %d bytes",
		     SSL_GETPID(), ss->fd, len));
	    return -1;
	}
    }
    return 0;
}

int
SSL_SendErrorMessage(SSLSocket *ss, int error)
{
    SSLSecurityInfo *sec;
    unsigned char msg[SSL_HL_ERROR_HBYTES];
    int rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;

    SSL_TRC(3, ("%d: SSL[%d]: sending error %d", SSL_GETPID(), ss->fd, error));

    msg[0] = SSL_MT_ERROR;
    msg[1] = MSB(error);
    msg[2] = LSB(error);
    rv = (*sec->send)(ss, msg, sizeof(msg), 0);
    if (rv >= 0) {
	rv = 0;
    }
    return rv;
}

static int
SendClientFinishedMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    unsigned char msg[1 + SSL_CONNECTIONID_BYTES];
    int rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    if (ci->sentFinished == 0) {
	ci->sentFinished = 1;

	SSL_TRC(3, ("%d: SSL[%d]: sending client-finished",
		    SSL_GETPID(), ss->fd));

	msg[0] = SSL_MT_CLIENT_FINISHED;
	PORT_Memcpy(msg+1, ci->connectionID, sizeof(ci->connectionID));

	DUMP_MSG(29, (ss, msg, 1 + sizeof(ci->connectionID)));
	rv = (*sec->send)(ss, msg, 1 + sizeof(ci->connectionID), 0);
	if (rv >= 0) {
	    rv = 0;
	}
	return rv;
    }
    return 0;
}

static int
SendServerVerifyMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    unsigned char *msg;
    int sendLen, rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    sendLen = 1 + SSL_CHALLENGE_BYTES;
    rv = GetSendBuffer(ss, sendLen);
    if (rv) {
	return rv;
    }

    msg = ci->sendBuf.buf;
    msg[0] = SSL_MT_SERVER_VERIFY;
    PORT_Memcpy(msg+1, ci->clientChallenge, SSL_CHALLENGE_BYTES);

    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*sec->send)(ss, msg, sendLen, 0);

    if (rv >= 0) {
	rv = 0;
    }
    return rv;
}

static int
SendServerFinishedMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLSessionID *sid;
    unsigned char *msg;
    int sendLen, rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    if (ci->sentFinished == 0) {
	ci->sentFinished = 1;
	PORT_Assert(ci->sid != 0);
	sid = ci->sid;

	SSL_TRC(3, ("%d: SSL[%d]: sending server-finished",
		    SSL_GETPID(), ss->fd));

	sendLen = 1 + sizeof(sid->u.ssl2.sessionID);
	rv = GetSendBuffer(ss, sendLen);
	if (rv) {
	    return rv;
	}

	msg = ci->sendBuf.buf;
	msg[0] = SSL_MT_SERVER_FINISHED;
	PORT_Memcpy(msg+1, sid->u.ssl2.sessionID, sizeof(sid->u.ssl2.sessionID));

	DUMP_MSG(29, (ss, msg, sendLen));
	rv = (*sec->send)(ss, msg, sendLen, 0);

	if (rv < 0) {
	    /* If send failed, it is now a bogus  session-id */
	    (*sec->uncache)(sid);
	} else if (!ss->noCache) {
	    /* Put the sid in session-id cache, (may already be there) */
	    (*sec->cache)(sid);
	    rv = 0;
	}
	ssl_FreeSID(sid);
	ci->sid = 0;
	return rv;
    }
    return 0;
}

static int
SendSessionKeyMessage(SSLSocket *ss, int cipher, int keySize,
		      unsigned char *ca, int caLen,
		      unsigned char *ck, int ckLen,
		      unsigned char *ek, int ekLen)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    unsigned char *msg;
    int rv, sendLen;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    sendLen = SSL_HL_CLIENT_MASTER_KEY_HBYTES + ckLen + ekLen + caLen;
    rv = GetSendBuffer(ss, sendLen);
    if (rv) return rv;

    SSL_TRC(3, ("%d: SSL[%d]: sending client-session-key",
		SSL_GETPID(), ss->fd));

    msg = ci->sendBuf.buf;
    msg[0] = SSL_MT_CLIENT_MASTER_KEY;
    msg[1] = cipher;
    msg[2] = MSB(keySize);
    msg[3] = LSB(keySize);
    msg[4] = MSB(ckLen);
    msg[5] = LSB(ckLen);
    msg[6] = MSB(ekLen);
    msg[7] = LSB(ekLen);
    msg[8] = MSB(caLen);
    msg[9] = LSB(caLen);
    PORT_Memcpy(msg+SSL_HL_CLIENT_MASTER_KEY_HBYTES, ck, ckLen);
    PORT_Memcpy(msg+SSL_HL_CLIENT_MASTER_KEY_HBYTES+ckLen, ek, ekLen);
    PORT_Memcpy(msg+SSL_HL_CLIENT_MASTER_KEY_HBYTES+ckLen+ekLen, ca, caLen);

    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*sec->send)(ss, msg, sendLen, 0);
    if (rv >= 0) {
	rv = 0;
    }
    return rv;
}

static int
SendCertificateRequestMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    unsigned char *msg;
    int rv, sendLen;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    sendLen = SSL_HL_REQUEST_CERTIFICATE_HBYTES + SSL_CHALLENGE_BYTES;
    rv = GetSendBuffer(ss, sendLen);
    if (rv) return rv;

    SSL_TRC(3, ("%d: SSL[%d]: sending certificate request",
		SSL_GETPID(), ss->fd));

    /* Generate random challenge for client to encrypt */
    RNG_GenerateGlobalRandomBytes(ci->serverChallenge, SSL_CHALLENGE_BYTES);

    msg = ci->sendBuf.buf;
    msg[0] = SSL_MT_REQUEST_CERTIFICATE;
    msg[1] = SSL_AT_MD5_WITH_RSA_ENCRYPTION;
    PORT_Memcpy(msg + SSL_HL_REQUEST_CERTIFICATE_HBYTES, ci->serverChallenge,
	      SSL_CHALLENGE_BYTES);

    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*sec->send)(ss, msg, sendLen, 0);
    if (rv >= 0) {
	rv = 0;
    }
    return rv;
}

static int
SendCertificateResponseMessage(SSLSocket *ss, SECItem *cert, SECItem *encCode)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    unsigned char *msg;
    int rv, sendLen;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    sendLen = SSL_HL_CLIENT_CERTIFICATE_HBYTES + encCode->len + cert->len;
    rv = GetSendBuffer(ss, sendLen);
    if (rv) return rv;

    SSL_TRC(3, ("%d: SSL[%d]: sending certificate response",
		SSL_GETPID(), ss->fd));

    msg = ci->sendBuf.buf;
    msg[0] = SSL_MT_CLIENT_CERTIFICATE;
    msg[1] = SSL_CT_X509_CERTIFICATE;/* XXX */
    msg[2] = MSB(cert->len);
    msg[3] = LSB(cert->len);
    msg[4] = MSB(encCode->len);
    msg[5] = LSB(encCode->len);
    PORT_Memcpy(msg + SSL_HL_CLIENT_CERTIFICATE_HBYTES, cert->data, cert->len);
    PORT_Memcpy(msg + SSL_HL_CLIENT_CERTIFICATE_HBYTES + cert->len,
	      encCode->data, encCode->len);

    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*sec->send)(ss, msg, sendLen, 0);
    if (rv >= 0) {
	rv = 0;
    }
    return rv;
}

/************************************************************************/

static int
GatherRecord(SSLSocket *ss)
{
    SSLConnectInfo *ci;
    int rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->gather != 0));
    ci = ss->sec->ci;

    /* See if we have a complete record */
    if (ss->version == SSL_LIBRARY_VERSION_3_0) {
	rv = ssl3_GatherHandshake(ss, 0);
    } else {
	rv = ssl_GatherRecord(ss, 0);
    }
    SSL_TRC(10, ("%d: SSL[%d]: handshake gathering, rv=%d",
		 SSL_GETPID(), ss->fd, rv));
    if (rv <= 0) {
	if (rv == -2) {
	    /* Would block, try again later */
	    SSL_TRC(10, ("%d: SSL[%d]: handshake blocked (need %d)",
			 SSL_GETPID(), ss->fd, ss->gather->remainder));
	    return rv;
	}
	if (rv == 0) {
	    /* EOF. Loser */
	    PORT_SetError(XP_ERRNO_EIO);
	    return -1;
	}
	return rv;
    }

    SSL_TRC(10, ("%d: SSL[%d]: got handshake record of %d bytes",
		 SSL_GETPID(), ss->fd, ss->gather->recordLen));
    ss->handshake = 0;
    return 0;
}

/************************************************************************/

static int
FillInSID(SSLSessionID *sid, int cipher,
	  unsigned char *keyData, int keyLen,
	  unsigned char *ca, int caLen,
	  int keyBits, int secretKeyBits)
{
    PORT_Assert(sid->references == 1);
    PORT_Assert(sid->cached == never_cached);
    PORT_Assert(sid->u.ssl2.masterKey.data == 0);
    PORT_Assert(sid->u.ssl2.cipherArg.data == 0);

    sid->version = SSL_LIBRARY_VERSION_2;

    sid->u.ssl2.cipherType = cipher;
    sid->u.ssl2.masterKey.data = (unsigned char*) PORT_Alloc(keyLen);
    if (!sid->u.ssl2.masterKey.data) {
	return -1;
    }
    PORT_Memcpy(sid->u.ssl2.masterKey.data, keyData, keyLen);
    sid->u.ssl2.masterKey.len = keyLen;
    sid->u.ssl2.keyBits = keyBits;
    sid->u.ssl2.secretKeyBits = secretKeyBits;

    if (caLen) {
	sid->u.ssl2.cipherArg.data = (unsigned char*) PORT_Alloc(caLen);
	if (!sid->u.ssl2.cipherArg.data) {
	    return -1;
	}
	sid->u.ssl2.cipherArg.len = caLen;
	PORT_Memcpy(sid->u.ssl2.cipherArg.data, ca, caLen);
    }
    return 0;
}

/*
** Construct session keys given the masterKey (tied to the session-id),
** the client's challenge and the server's nonce.
*/
static int
ProduceKeys(SSLSocket *ss, SECItem *readKey, SECItem *writeKey,
	    SECItem *masterKey, unsigned char *challenge,
	    unsigned char *nonce, int cipherType)
{
    unsigned char km[3*16];
    unsigned part, nkm = 0, nkd = 0, i, off;
    unsigned char countChar;
    int rv;

    MD5Context *cx = 0;
    readKey->data = 0;
    writeKey->data = 0;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));

    rv = 0;
    cx = MD5_NewContext();
    if (cx == NULL) {
	return 0;
    }

    switch (cipherType) {

    case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
    case SSL_CK_RC4_128_WITH_MD5:
    case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
    case SSL_CK_RC2_128_CBC_WITH_MD5:
	nkm = 2;
	nkd = 16;
	break;

    case SSL_CK_DES_64_CBC_WITH_MD5:
	nkm = 1;
	nkd = 8;
	break;


    case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	nkm = 3;
	nkd = 24;
	break;
    }

    readKey->data = (unsigned char*) PORT_Alloc(nkd);
    if (!readKey->data) goto loser;
    readKey->len = nkd;

    writeKey->data = (unsigned char*) PORT_Alloc(nkd);
    if (!writeKey->data) 
    	goto loser;
    writeKey->len = nkd;

    /* Produce key material */
    countChar = '0';
    for (i = 0, off = 0; i < nkm; i++, off += 16) {
	MD5_Begin(cx);
	MD5_Update(cx, masterKey->data, masterKey->len);
	MD5_Update(cx, &countChar, 1);
	MD5_Update(cx, challenge, SSL_CHALLENGE_BYTES);
	MD5_Update(cx, nonce, SSL_CONNECTIONID_BYTES);
	MD5_End(cx, km+off, &part, MD5_LENGTH);
	countChar++;
    }

    /* Produce keys */
    switch (cipherType) {
    case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
    case SSL_CK_RC2_128_CBC_WITH_MD5:
	PORT_Memcpy(readKey->data, km, 16);
	PORT_Memcpy(writeKey->data, km+16, 16);
	break;

    case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
    case SSL_CK_RC4_128_WITH_MD5:
	PORT_Memcpy(readKey->data, km, 16);
	PORT_Memcpy(writeKey->data, km+16, 16);
	break;

    case SSL_CK_DES_64_CBC_WITH_MD5:
	PORT_Memcpy(readKey->data, km, 8);
	PORT_Memcpy(writeKey->data, km+8, 8);
	break;

    case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	PORT_Memcpy(readKey->data, km, 24);
	PORT_Memcpy(writeKey->data, km+24, 24);
	break;
    }

loser:
    MD5_DestroyContext(cx, PR_TRUE);
    return rv;
}

static int
CreateSessionCypher(SSLSocket *ss, SSLSessionID *sid, PRBool isClient)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SECItem readKey, writeKey;
    SECItem *rk, *wk;
    int rv, blockSize, blockShift;
    PK11SlotInfo *slot = 0;
    CK_MECHANISM_TYPE mechanism;
    SECItem *param;

    void *readcx = 0;
    void *writecx = 0;
    readKey.data = 0;
    writeKey.data = 0;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->sec->ci->sid != 0));
    sec = ss->sec;
    ci = sec->ci;
    rk = isClient ? &readKey : &writeKey;
    wk = isClient ? &writeKey : &readKey;

    /* Produce the keys for this session */
    rv = ProduceKeys(ss, &readKey, &writeKey, &sid->u.ssl2.masterKey,
		     ci->clientChallenge, ci->connectionID,
		     sid->u.ssl2.cipherType);
    if (rv) goto loser;
    PRINT_BUF(7, (ss, "Session read-key: ", rk->data, rk->len));
    PRINT_BUF(7, (ss, "Session write-key: ", wk->data, wk->len));

    PORT_Memcpy(ci->readKey, readKey.data, readKey.len);
    PORT_Memcpy(ci->writeKey, writeKey.data, writeKey.len);
    ci->keySize = readKey.len;

    /* Setup the MAC */
    rv = CreateMAC(sec, rk, wk, sid->u.ssl2.cipherType);
    if (rv) 
    	goto loser;

    /* First create the session key object */
    SSL_TRC(3, ("%d: SSL[%d]: using %s", SSL_GETPID(), ss->fd,
	    ssl_cipherName[sid->u.ssl2.cipherType]));
    switch (sid->u.ssl2.cipherType) {


    case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
    case SSL_CK_RC4_128_WITH_MD5:
	blockSize = 1;
	blockShift = 0;
	mechanism = CKM_RC4;
	break;

    case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
    case SSL_CK_RC2_128_CBC_WITH_MD5:
	blockSize = 8;
	blockShift = 3;
	mechanism = CKM_RC2_CBC;
	break;

    case SSL_CK_DES_64_CBC_WITH_MD5:
	blockSize = 8;
	blockShift = 3;
	mechanism = CKM_DES_CBC;
	break;

    case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	blockSize = 8;
	blockShift = 3;
	mechanism = CKM_DES3_CBC;
	break;

    default:
	SSL_DBG(("%d: SSL[%d]: CreateSessionCypher: unknown cipher=%d",
		 SSL_GETPID(), ss->fd, sid->u.ssl2.cipherType));
	PORT_SetError(isClient ? SSL_ERROR_BAD_SERVER : SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    /* set destructer before we call loser... */
    sec->destroy = (void (*)(void*,PRBool)) PK11_DestroyContext;
    slot = PK11_GetBestSlot(mechanism,ss->sec->getClientAuthDataArg);
    if (slot == NULL) goto loser;

    param = PK11_ParamFromIV(mechanism,&sid->u.ssl2.cipherArg);
    if (param == NULL) goto loser;
    readcx = PK11_CreateContextByRawKey(slot,mechanism, CKA_DECRYPT,
				   rk,param,ss->sec->getClientAuthDataArg);
    SECITEM_FreeItem(param,PR_TRUE);
    if (readcx == NULL) goto loser;

    /* build the client context */
    param = PK11_ParamFromIV(mechanism,&sid->u.ssl2.cipherArg);
    if (param == NULL) goto loser;
    writecx = PK11_CreateContextByRawKey(slot,mechanism, CKA_ENCRYPT,
				   wk,param,ss->sec->getClientAuthDataArg);
    SECITEM_FreeItem(param,PR_TRUE);
    if (writecx == NULL) goto loser;
    PK11_FreeSlot(slot);

    rv = 0;
    sec->enc = (SSLCipher) PK11_CipherOp;
    sec->dec = (SSLCipher) PK11_CipherOp;
    sec->readcx = (void*) readcx;
    sec->writecx = (void*) writecx;
    sec->blockSize = blockSize;
    sec->blockShift = blockShift;
    sec->cipherType = sid->u.ssl2.cipherType;
    sec->keyBits = sid->u.ssl2.keyBits;
    sec->secretKeyBits = sid->u.ssl2.secretKeyBits;
    goto done;

  loser:
    if (readcx) (*sec->destroy)(readcx, PR_TRUE);
    if (writecx) (*sec->destroy)(writecx, PR_TRUE);
    if (slot) PK11_FreeSlot(slot);
    rv = -1;

  done:
    SECITEM_ZfreeItem(rk, PR_FALSE);
    SECITEM_ZfreeItem(wk, PR_FALSE);
    return rv;
}

/*
** Setup the server ciphers given information from a CLIENT-MASTER-KEY
** message.
** 	"ss" pointer to the ssl-socket object
** 	"cipher" the cipher type to use
** 	"keyBits" the size of the final cipher key
** 	"ck" the clear-key data
** 	"ckLen" the number of bytes of clear-key data
** 	"ek" the encrypted-key data
** 	"ekLen" the number of bytes of encrypted-key data
** 	"ca" the cipher-arg data
** 	"caLen" the number of bytes of cipher-arg data
**
** The MASTER-KEY is constructed by first decrypting the encrypted-key
** data. This produces the SECRET-KEY-DATA. The MASTER-KEY is composed by
** concatenating the clear-key data with the SECRET-KEY-DATA. This code
** checks to make sure that the client didn't send us an improper amount
** of SECRET-KEY-DATA (it restricts the length of that data to match the
** spec).
*/
static int
ServerSetupSessionCypher(SSLSocket *ss, int cipher, int keyBits,
			 unsigned char *ck, int ckLen,
			 unsigned char *ek, int ekLen,
			 unsigned char *ca, int caLen)
{
    unsigned char mkbuf[SSL_MAX_MASTER_KEY_BYTES], *kk;
    SSLSecurityInfo *sec;
    SSLSessionID *sid;
    int rv, keySize;
    unsigned el1;
    unsigned char *kbuf = 0;
    int modulus;

    PORT_Assert((ss->sec != 0) && (ssl_server_key != 0));
    sec = ss->sec;
    PORT_Assert((sec->ci != 0) && (sec->ci->sid != 0));
    sid = sec->ci->sid;

    keySize = (keyBits + 7) >> 3;
    if (keySize > SSL_MAX_MASTER_KEY_BYTES) {
	/* bummer */
	SSL_DBG(("%d: SSL[%d]: keySize=%d ckLen=%d max session key size=%d",
		 SSL_GETPID(), ss->fd, keySize, ckLen,
		 SSL_MAX_MASTER_KEY_BYTES));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }
    if ((cipher == SSL_CK_RC4_128_EXPORT40_WITH_MD5) ||
	(cipher == SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5)) {
	if (ckLen != (keySize - (40>>3))) {
	    SSL_DBG(("%d: SSL[%d]: odd secret key size, keySize=%d ckLen=%d!",
		     SSL_GETPID(), ss->fd, keySize, ckLen));
	    /* Somebody tried to sneak by a strange secret key */
	    PORT_SetError(SSL_ERROR_BAD_CLIENT);
	    goto loser;
	}
    } else {
	if ( ckLen != 0 ) {
	    PORT_SetError(SSL_ERROR_BAD_CLIENT);
	    goto loser;
	}
    }

    kbuf = (unsigned char*) PORT_Alloc(ekLen);
    if (!kbuf) {
	goto loser;
    }

    /*
    ** Decrypt encrypted half of the key. Note that encrypted half has
    ** been bloated to match the modulus size of our public key using
    ** PKCS#1. keySize is the real size of the data that is interesting.
    ** NOTE: PK11_PubDecryptRaw will barf on a non-RSA key. This is
    ** desired behavior here.
    */
    rv = PK11_PubDecryptRaw(ssl_server_key, kbuf, &el1, ekLen, ek, ekLen);
    if (rv != SECSuccess) goto loser;

    modulus = PK11_GetPrivateModulusLen(ssl_server_key);
    if (modulus == -1) {
	/* Paranoia.. if the key was really bad, then PK11_pubDecryptRaw
	 * would have failed, therefore the we must assume that the card
	 * is just being a pain and not giving us the modulus... but it
	 * should be the same size as the encrypted key length, so use it
	 * and keep cranking */
	modulus = ekLen;
    }
    /* Cheaply verify that PKCS#1 was used to format the encryption block */
    kk = kbuf + modulus - (keySize - ckLen);
    if ((kbuf[0] != 0x00) || (kbuf[1] != 0x02) || (kk[-1] != 0x00)) {
	/* Tsk tsk. */
	SSL_DBG(("%d: SSL[%d]: strange encryption block",
		 SSL_GETPID(), ss->fd));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    /* Make sure we're not subject to a version rollback attack. */
    if (ss->enableSSL3) {
	unsigned char threes[8] = { 0x03, 0x03, 0x03, 0x03,
				    0x03, 0x03, 0x03, 0x03 };
	
	if (PORT_Memcmp(kk - 8 - 1, threes, 8) == 0) {
	    PORT_SetError(SSL_ERROR_BAD_CLIENT);
	    goto loser;
	}
    }

    /*
    ** Construct master key out of the pieces.
    */
    if (ckLen) {
	PORT_Memcpy(mkbuf, ck, ckLen);
    }
    PORT_Memcpy(mkbuf+ckLen, kk, keySize-ckLen);

    /* Fill in session-id */
    rv = FillInSID(sid, cipher, mkbuf, keySize, ca, caLen,
		   keyBits, keyBits - (ckLen<<3));
    if (rv < 0) {
	goto loser;
    }

    /* Create session ciphers */
    rv = CreateSessionCypher(ss, sid, PR_FALSE);
    if (rv < 0) {
	goto loser;
    }

    SSL_TRC(1, ("%d: SSL[%d]: server, using %s cipher, clear=%d total=%d",
		SSL_GETPID(), ss->fd, ssl_cipherName[cipher],
		ckLen<<3, keySize<<3));
    rv = 0;
    goto done;

  loser:
    rv = -1;

  done:
    PORT_Free(kbuf);
    return rv;
}

/************************************************************************/

/*
** Rewrite the incoming cipher specs, eliminating anything in it we don't
** support.
*/
static int
QualifyCypherSpecs(SSLSocket *ss, unsigned char *cs, int csLen)
{
    unsigned char *ms, *hs;
    int i, hc;

    if (!cipherSpecs) {
	ConstructCipherSpecs(NULL, &cipherSpecs, &sizeCipherSpecs);
    }

    PRINT_BUF(10, (ss, "specs from client:", cs, csLen));
    hs = cs;
    hc = csLen / 3;
    csLen = 0;
    while (--hc >= 0) {
	for (i = 0, ms = cipherSpecs; i < sizeCipherSpecs; i += 3, ms += 3) {
	    if ((hs[0] == ms[0]) &&
		(hs[1] == ms[1]) &&
		(hs[2] == ms[2])) {
		/* Copy this cipher spec into the "keep" section */
		cs[0] = hs[0];
		cs[1] = hs[1];
		cs[2] = hs[2];
		cs += 3;
		csLen += 3;
		break;
	    }
	}
	hs += 3;
    }
    PRINT_BUF(10, (ss, "qualified specs from client:", cs-csLen, csLen));
    return csLen;
}

/*
** Pick the best cipher we can find, given the array of server cipher
** specs.
*/
static int
ChooseSessionCypher(SSLSocket *ss, int hc, unsigned char *hs, int *keyLenp)
{
    SSLConnectInfo *ci;
    int i, bestKeySize, bestRealKeySize, bestCypher, keySize, realKeySize;
    unsigned char *ms;

    unsigned char *ohs = hs;
    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    ci = ss->sec->ci;
    if (!cipherSpecs) {
	ConstructCipherSpecs(NULL, &cipherSpecs, &sizeCipherSpecs);
    }

    if (!preferredCipher) {
    	unsigned int allowed = allowedByPolicy & chosenPreference;
	preferredCipher = allCipherSuites;
	while (allowed) {
	    if (allowed & 1) 
	    	break;
	    allowed >>= 1;
	    preferredCipher += 3;
	}
    }
    /*
    ** Scan list of ciphers recieved from peer and look for a match in
    ** our list.
    */
    bestKeySize = bestRealKeySize = 0;
    bestCypher = -1;
    while (--hc >= 0) {
	for (i = 0, ms = cipherSpecs; i < sizeCipherSpecs; i += 3, ms += 3) {
	    if ((hs[0] == preferredCipher[0]) &&
		(hs[1] == preferredCipher[1]) &&
		(hs[2] == preferredCipher[2])) {
		/* Pick this cipher immediately! */
		/*
		** XXX this hack depends on key length being in bytes 1&2
		** XXX - which is not really what the spec allows!
		*/
		*keyLenp = (((hs[1] << 8) | hs[2]) + 7) >> 3;
		return hs[0];
	    }
	    if ((hs[0] == ms[0]) && (hs[1] == ms[1]) && (hs[2] == ms[2])) {
		/* Found a match */

		/* Use secret keySize to determine which cypher is best */
		/* XXX this is a hack that depends on old spec cipher-kind
		   formatting */
		realKeySize = (hs[1] << 8) | hs[2];
		switch (hs[0]) {
		  case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
		  case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
		    keySize = 40;
		    break;
		  default:
		    keySize = realKeySize;
		    break;
		}
		if (keySize > bestKeySize) {
		    bestCypher = hs[0];
		    bestKeySize = keySize;
		    bestRealKeySize = realKeySize;
		}
	    }
	}
	hs += 3;
    }
    if (bestCypher < 0) {
	/*
	** No overlap between server and client. Re-examine server list
	** to see what kind of ciphers it does support so that we can set
	** the error code appropriately.
	*/
	if ((ohs[0] == SSL_CK_RC4_128_WITH_MD5) ||
	    (ohs[0] == SSL_CK_RC2_128_CBC_WITH_MD5)) {
	    PORT_SetError(SSL_ERROR_US_ONLY_SERVER);
	} else if ((ohs[0] == SSL_CK_RC4_128_EXPORT40_WITH_MD5) ||
		   (ohs[0] == SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5)) {
	    PORT_SetError(SSL_ERROR_EXPORT_ONLY_SERVER);
	} else {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	}
	SSL_DBG(("%d: SSL[%d]: no cipher overlap", SSL_GETPID(), ss->fd));
	goto loser;
    }
    *keyLenp = (bestRealKeySize + 7) >> 3;
    return bestCypher;

  loser:
    return -1;
}

static int
ClientHandleServerCert(SSLSocket *ss, unsigned char *certData, int certLen)
{
    SECItem certItem;
    CERTCertificate *cert = NULL;

    PORT_Assert(ss->sec != 0);

    certItem.data = certData;
    certItem.len = certLen;

    /* decode the certificate */
    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &certItem, NULL,
				  PR_FALSE, PR_TRUE);
    
    if (cert == NULL) {
	SSL_DBG(("%d: SSL[%d]: decode of server certificate fails",
		 SSL_GETPID(), ss->fd));
	return SSL_ERROR_BAD_CERTIFICATE;
    }

#ifdef TRACE
    {
	if (ssl_trace >= 1) {
	    char *issuer;
	    char *subject;
	    issuer = CERT_NameToAscii(&cert->issuer);
	    subject = CERT_NameToAscii(&cert->subject);
	    SSL_TRC(1,("%d: server certificate issuer: '%s'",
		       SSL_GETPID(), issuer ? issuer : "OOPS"));
	    SSL_TRC(1,("%d: server name: '%s'",
		       SSL_GETPID(), subject ? subject : "OOPS"));
	    PORT_Free(issuer);
	    PORT_Free(subject);
	}
    }
#endif

    ss->sec->peerCert = cert;
    return 0;
}

/*
** Given the servers public key and cipher specs, generate a session key
** that is ready to use for encrypting/decrypting the byte stream. At
** the same time, generate the SSL_MT_CLIENT_MASTER_KEY message and
** send it to the server.
*/
static int
ClientSetupSessionCypher(SSLSocket *ss,
			 unsigned char *cs, int csLen)
{
    SSLConnectInfo *ci;
    SSLSessionID *sid;
    unsigned char *ca;
    int rv, cipher, keyLen, ckLen, caLen, nc;
    SECItem eblock, rek;
    unsigned char keyData[SSL_MAX_MASTER_KEY_BYTES];

    unsigned char iv[8];


    unsigned char *ekbuf = 0;
    CERTCertificate *cert = 0;
    SECKEYPublicKey *serverKey = 0;
    unsigned modulusLen = 0;
    eblock.data = 0;
    eblock.len = 0;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    ci = ss->sec->ci;
    PORT_Assert(ci->sid != 0);
    sid = ci->sid;

    cert = ss->sec->peerCert;
    
    serverKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if (!serverKey) {
	SSL_DBG(("%d: SSL[%d]: extract public key failed: error=%d",
		 SSL_GETPID(), ss->fd, PORT_GetError()));
	rv = SSL_ERROR_BAD_CERTIFICATE;
	goto loser2;
    }

    /* Choose a compatible cipher with the server */
    nc = csLen / 3;
    cipher = ChooseSessionCypher(ss, nc, cs, &keyLen);
    if (cipher < 0) {
	goto loser;
    }

    /* Generate the random keys */
    RNG_GenerateGlobalRandomBytes(keyData, sizeof(keyData));

    /*
    ** Next, carve up the keys into clear and encrypted portions. The
    ** clear data is taken from the start of keyData and the encrypted
    ** portion from the remainder. Note that each of these portions is
    ** carved in half, one half for the read-key and one for the
    ** write-key.
    */
    ca = 0;
    caLen = 0;
    switch (cipher) {

    case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
	ckLen = keyLen - (40>>3);
	RNG_GenerateGlobalRandomBytes(iv, 8);
	ca = iv;
	caLen = 8;
	break;

    case SSL_CK_RC2_128_CBC_WITH_MD5:
	ckLen = 0;
	RNG_GenerateGlobalRandomBytes(iv, 8);
	ca = iv;
	caLen = 8;
	break;

    case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
	ckLen = keyLen - (40>>3);
	break;

    case SSL_CK_RC4_128_WITH_MD5:
	ckLen = 0;
	break;

    case SSL_CK_DES_64_CBC_WITH_MD5:
    case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	ckLen = 0;
	RNG_GenerateGlobalRandomBytes(iv, 8);
	ca = iv;
	caLen = 8;
	break;

    default:
	SSL_DBG(("%d: SSL[%d]: ClientSetupSessionCypher: unknown cipher=%d",
		 SSL_GETPID(), ss->fd, cipher));
	PORT_SetError(SSL_ERROR_BAD_SERVER);
	goto loser;
    }

    /* Fill in session-id */
    rv = FillInSID(sid, cipher, keyData, keyLen,
		   ca, caLen, keyLen << 3, (keyLen - ckLen) << 3);
    if (rv < 0) {
	goto loser;
    }

    SSL_TRC(1, ("%d: SSL[%d]: client, using %s cipher, clear=%d total=%d",
		SSL_GETPID(), ss->fd, ssl_cipherName[cipher],
		ckLen<<3, keyLen<<3));

    /* Now setup read and write ciphers */
    rv = CreateSessionCypher(ss, sid, PR_TRUE);
    if (rv < 0) {
	goto loser;
    }

    /*
    ** Fill in the encryption buffer with some random bytes. Then 
    ** copy in the portion of the session key we are encrypting.
    */
    modulusLen = SECKEY_PublicKeyStrength(serverKey);
    rek.data = keyData + ckLen;
    rek.len = keyLen - ckLen;
    rv = RSA_FormatBlock(&eblock, modulusLen, RSA_BlockPublic, &rek);
    if (rv) goto loser;
    /* Set up the padding for version 2 rollback detection. */
    /* XXX We should really use defines here */
    if (ss->enableSSL3) {
	PORT_Assert((modulusLen - rek.len) > 12);
	PORT_Memset(eblock.data + modulusLen - rek.len - 8 - 1, 0x03, 8);
    }
    ekbuf = (unsigned char*) PORT_Alloc(modulusLen);
    if (!ekbuf) goto loser;
    PRINT_BUF(10, (ss, "master key encryption block:",
		   eblock.data, eblock.len));

    /* Encrypt ekitem */
    rv = PK11_PubEncryptRaw(serverKey, ekbuf, eblock.data, modulusLen,
						ss->sec->getClientAuthDataArg);
    if (rv) goto loser;

    if (eblock.len != modulusLen) {
	/* Something strange just happened */
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	goto loser;
    }

    /* GASP. Now we have everything ready to send */
    rv = SendSessionKeyMessage(ss, cipher, keyLen << 3, ca, caLen,
			       keyData, ckLen, ekbuf, modulusLen);
    if (rv < 0) {
	goto loser;
    }
    rv = 0;
    goto done;

  loser:
    rv = -1;

  loser2:
  done:
    PORT_Memset(keyData, 0, sizeof(keyData));
    PORT_ZFree(ekbuf, modulusLen);
    SECITEM_ZfreeItem(&eblock, PR_FALSE);
    SECKEY_DestroyPublicKey(serverKey);
    return rv;
}

/************************************************************************/

static void
ClientRegSessionID(SSLSocket *ss, unsigned char *s)
{
    SSLSecurityInfo *sec;
    SSLSessionID *sid;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci->sid != 0));
    sec = ss->sec;
    sid = sec->ci->sid;

    /* Record entry in nonce cache */
    if (sid->peerCert == NULL) {
	PORT_Memcpy(sid->u.ssl2.sessionID, s, sizeof(sid->u.ssl2.sessionID));
	sid->peerCert = CERT_DupCertificate(sec->peerCert);

    }
    if (!ss->noCache)
	(*sec->cache)(sid);
}

static int
TriggerNextMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    int rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    if ((ci->requiredElements & CIS_HAVE_CERTIFICATE) &&
	!(ci->sentElements & CIS_HAVE_CERTIFICATE)) {
	ci->sentElements |= CIS_HAVE_CERTIFICATE;
	rv = SendCertificateRequestMessage(ss);
	return rv;
    }
    return 0;
}

static int
TryToFinish(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    char e, ef;
    int rv;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    e = ci->elements;
    ef = e | CIS_HAVE_FINISHED;
    if ((ef & ci->requiredElements) == ci->requiredElements) {
	if (sec->isServer) {
	    /* Send server finished message if we already didn't */
	    rv = SendServerFinishedMessage(ss);
	} else {
	    /* Send client finished message if we already didn't */
	    rv = SendClientFinishedMessage(ss);
	}
	if (rv) {
	    return rv;
	}
	if ((e & ci->requiredElements) == ci->requiredElements) {
	    /* Totally finished */
	    ss->handshake = 0;
	    return 0;
	}
    }
    return 0;
}

static SECStatus
SignResponse(SSLSocket *ss,
	     SECKEYPrivateKey *key,
	     SECItem *response)
{
    SGNContext *sgn = NULL;
    SSLConnectInfo *ci;
    SSLSecurityInfo *sec;
    SECStatus rv;
    unsigned char *challenge;
    unsigned int len;
    
    sec = ss->sec;
    ci = sec->ci;
    challenge = ci->serverChallenge;
    len = ci->serverChallengeLen;
    
    /* Sign the expected data... */
    sgn = SGN_NewContext(SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION,key);
    if (!sgn) goto loser;
    rv = SGN_Begin(sgn);
    if (rv) goto loser;
    rv = SGN_Update(sgn, ci->readKey, ci->keySize);
    if (rv) goto loser;
    rv = SGN_Update(sgn, ci->writeKey, ci->keySize);
    if (rv) goto loser;
    rv = SGN_Update(sgn, challenge, len);
    if (rv) goto loser;
    rv = SGN_Update(sgn,
		    sec->peerCert->derCert.data, sec->peerCert->derCert.len);
    if (rv) goto loser;
    rv = SGN_End(sgn, response);
    if (rv) goto loser;

    SGN_DestroyContext(sgn, PR_TRUE);
    return(SECSuccess);

loser:
    SGN_DestroyContext(sgn, PR_TRUE);
    return(SECFailure);
}

/*
** Try to handle a request-certificate message. Get client's certificate
** and private key and sign a message for the server to see.
*/
static int
HandleRequestCertificate(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SECStatus rv;
    SECItem response;
    int ret;
    CERTCertificate *cert;
    SECKEYPrivateKey *key;
    unsigned char authType;


    /*
     * These things all need to be initialized before we can "goto loser".
     */
    cert = NULL;
    key = NULL;
    response.data = NULL;
    ret = 0;


    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    /* get challenge info from connectionInfo */
    authType = ci->authType;

    if (authType != SSL_AT_MD5_WITH_RSA_ENCRYPTION) {
	SSL_TRC(7, ("%d: SSL[%d]: unsupported auth type 0x%x",
		    SSL_GETPID(), ss->fd, authType));
	goto no_cert_error;
    }

    /* Get certificate and private-key from client */
    if (!sec->getClientAuthData) {
	SSL_TRC(7, ("%d: SSL[%d]: client doesn't support client-auth",
		    SSL_GETPID(), ss->fd));
	goto no_cert_error;
    }
    ret = (*sec->getClientAuthData)(sec->getClientAuthDataArg, ss->fd, NULL,
				    &cert, &key);
    if ( ret == -2 ) {
	goto done;
    }

    if (ret) {
	goto no_cert_error;
    }

    rv = SignResponse(ss, key, &response);
    if ( rv != SECSuccess ) {
	ret = -1;
	goto loser;
    }
    
    /* Send response message */
    ret = SendCertificateResponseMessage(ss, &cert->derCert, &response);
    goto done;

  no_cert_error:
    SSL_TRC(7, ("%d: SSL[%d]: no certificate (ret=%d)",
		SSL_GETPID(), ss->fd, ret));
    ret = SSL_SendErrorMessage(ss, SSL_PE_NO_CERTIFICATE);

  loser:
  done:
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    if ( key ) {
	SECKEY_DestroyPrivateKey(key);
    }
    if ( response.data ) {
	PORT_Free(response.data);
    }
    
    return ret;
}

static SECStatus
HandleClientCertificate(SSLSocket *ss, unsigned char certType,
			unsigned char *cd, unsigned cdLen,
			unsigned char *response,
			unsigned responseLen)
{
    SECItem certItem, rep;
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SECStatus rv;
    CERTCertificate *cert;
    SECKEYPublicKey *pubKey;
    VFYContext *vfy;


    /*
     * These things all need to be initialized before we can "goto loser".
     */
    rv = SECFailure;
    cert = NULL;
    pubKey = NULL;
    vfy = NULL;

    sec = ss->sec;
    ci = sec->ci;

    /* Extract the certificate */
    certItem.data = cd;
    certItem.len = cdLen;

    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &certItem, NULL,
			 	  PR_FALSE, PR_TRUE);
    if (cert == NULL) {
	goto loser;
    }

    /* save the certificate, since the auth routine will need it */
    sec->peerCert = cert;

    /* Extract the public key */
    pubKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if (!pubKey) goto loser;
    
    /* Verify the response data... */
    rep.data = response;
    rep.len = responseLen;
    vfy = VFY_CreateContext(pubKey, &rep, sec->getClientAuthData);
    if (!vfy) goto loser;
    rv = VFY_Begin(vfy);
    if (rv) goto loser;
    /*
    ** XXX This is wrong! should use key material as per spec. However,
    ** XXX it happens to work for DES.
    */
    rv = VFY_Update(vfy, ci->readKey, ci->keySize);
    if (rv) goto loser;
    rv = VFY_Update(vfy, ci->writeKey, ci->keySize);
    if (rv) goto loser;
    rv = VFY_Update(vfy, ci->serverChallenge, SSL_CHALLENGE_BYTES);
    if (rv) goto loser;
    rv = VFY_Update(vfy, ssl_server_signed_certificate.data,
		    ssl_server_signed_certificate.len);
    if (rv) goto loser;
    rv = VFY_End(vfy);
    if (rv) goto loser;

    /* Now ask application if it likes the certificate... */
    rv = (SECStatus) (*sec->authCertificate)(sec->authCertificateArg,
					    ss->fd, PR_TRUE, PR_TRUE);

    if (rv) goto loser;

    /* Hey, we liked it. Make a copy for SecurityStatus call... */
    if (rv) goto loser;
    goto done;

  loser:
    sec->peerCert = NULL;
    CERT_DestroyCertificate(cert);

  done:
    VFY_DestroyContext(vfy, PR_TRUE);
    SECKEY_DestroyPublicKey(pubKey);
    return (SECStatus) rv;
}

/*
** Handle remaining messages between client/server. Process finished
** messages from either side and any authentication requests.
*/
static int
HandleMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLGather *gs;
    unsigned char *data, *cid;
    unsigned len, certType, certLen, responseLen;
    int rv, rv2;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->gather != 0));
    sec = ss->sec;
    gs = ss->gather;
    ci = sec->ci;

    data = gs->buf.buf + gs->recordOffset;

    if (gs->recordLen < 1) {
	goto bad_peer;
    }
    SSL_TRC(3, ("%d: SSL[%d]: received %d message",
		SSL_GETPID(), ss->fd, data[0]));
    DUMP_MSG(29, (ss, data, gs->recordLen));

    switch (data[0]) {
      case SSL_MT_CLIENT_FINISHED:
	if (ci->elements & CIS_HAVE_FINISHED) {
	    SSL_DBG(("%d: SSL[%d]: dup client-finished message",
		     SSL_GETPID(), ss->fd));
	    goto bad_peer;
	}

	/* See if nonce matches */
	len = gs->recordLen - 1;
	cid = data + 1;
	if ((len != sizeof(ci->connectionID)) ||
	    (PORT_Memcmp(ci->connectionID, cid, len) != 0)) {
	    SSL_DBG(("%d: SSL[%d]: bad connection-id", SSL_GETPID(), ss->fd));
	    PRINT_BUF(5, (ss, "sent connection-id",
			  ci->connectionID, sizeof(ci->connectionID)));
	    PRINT_BUF(5, (ss, "rcvd connection-id", cid, len));
	    goto bad_peer;
	}

	SSL_TRC(5, ("%d: SSL[%d]: got client finished, waiting for 0x%d",
		    SSL_GETPID(), ss->fd, ci->requiredElements ^ ci->elements));
	ci->elements |= CIS_HAVE_FINISHED;
	break;

      case SSL_MT_SERVER_FINISHED:
	if (ci->elements & CIS_HAVE_FINISHED) {
	    SSL_DBG(("%d: SSL[%d]: dup server-finished message",
		     SSL_GETPID(), ss->fd));
	    goto bad_peer;
	}

	if (gs->recordLen - 1 != SSL_SESSIONID_BYTES) {
	    SSL_DBG(("%d: SSL[%d]: bad server-finished message, len=%d",
		     SSL_GETPID(), ss->fd, gs->recordLen));
	    goto bad_peer;
	}
	ClientRegSessionID(ss, data+1);
	SSL_TRC(5, ("%d: SSL[%d]: got server finished, waiting for 0x%d",
		    SSL_GETPID(), ss->fd, ci->requiredElements ^ ci->elements));
	ci->elements |= CIS_HAVE_FINISHED;
	break;

      case SSL_MT_REQUEST_CERTIFICATE:
	len = gs->recordLen - 2;
	if ((len != SSL_MIN_CHALLENGE_BYTES) ||
	    (len > SSL_MAX_CHALLENGE_BYTES)) {
	    /* Bad challenge */
	    SSL_DBG(("%d: SSL[%d]: bad cert request message: code len=%d",
		     SSL_GETPID(), ss->fd, len));
	    goto bad_peer;
	}
	
	/* save auth request info */
	ci->authType = data[1];
	ci->serverChallengeLen = len;
	PORT_Memcpy(ci->serverChallenge, data + 2, len);
	
	rv = HandleRequestCertificate(ss);
	if (rv == -2) {
	    SSL_TRC(3, ("%d: SSL[%d]: async cert request",
			SSL_GETPID(), ss->fd));
	    /* someone is handling this asynchronously */
	    return(-2);
	}
	if (rv) goto loser;
	break;

      case SSL_MT_CLIENT_CERTIFICATE:
	if (!sec->authCertificate) {
	    /* Server asked for authentication and can't handle it */
	    PORT_SetError(SSL_ERROR_BAD_SERVER);
	    goto loser;
	}
	if (gs->recordLen < SSL_HL_CLIENT_CERTIFICATE_HBYTES) goto loser;
	certType = data[1];
	certLen = (data[2] << 8) | data[3];
	responseLen = (data[4] << 8) | data[5];
	if (certType != SSL_CT_X509_CERTIFICATE) {
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE);
	    goto loser;
	}
	rv = HandleClientCertificate(ss, data[1],
		data + SSL_HL_CLIENT_CERTIFICATE_HBYTES,
		certLen,
		data + SSL_HL_CLIENT_CERTIFICATE_HBYTES + certLen,
		responseLen);
	if (rv) {
	    rv2 = SSL_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    goto loser;
	}
	ci->elements |= CIS_HAVE_CERTIFICATE;
	break;

      case SSL_MT_ERROR:
	rv = (data[1] << 8) | data[2];
	SSL_TRC(2, ("%d: SSL[%d]: got error message, error=0x%x",
		    SSL_GETPID(), ss->fd, rv));

	/* Convert protocol error number into API error number */
	switch (rv) {
	  case SSL_PE_NO_CYPHERS:
	    rv = SSL_ERROR_NO_CYPHER_OVERLAP;
	    break;
	  case SSL_PE_NO_CERTIFICATE:
	    rv = SSL_ERROR_NO_CERTIFICATE;
	    break;
	  case SSL_PE_BAD_CERTIFICATE:
	    rv = SSL_ERROR_BAD_CERTIFICATE;
	    break;
	  case SSL_PE_UNSUPPORTED_CERTIFICATE_TYPE:
	    rv = SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE;
	    break;
	  default:
	    goto bad_peer;
	}
	/* XXX make certificate-request optionally fail... */
	PORT_SetError(rv);
	goto loser;

      default:
	SSL_DBG(("%d: SSL[%d]: unknown message %d",
		 SSL_GETPID(), ss->fd, data[0]));
	goto loser;
    }

    SSL_TRC(3, ("%d: SSL[%d]: handled %d message, required=0x%x got=0x%x",
		SSL_GETPID(), ss->fd, data[0],
		ci->requiredElements, ci->elements));

    rv = TryToFinish(ss);
    if (rv) goto loser;
    if (ss->handshake == 0) {
	return 0;
    }

    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = HandleMessage;
    return TriggerNextMessage(ss);

  bad_peer:
    PORT_SetError(sec->isServer ? SSL_ERROR_BAD_CLIENT : SSL_ERROR_BAD_SERVER);
    /* FALL THROUGH */

  loser:
    return -1;
}

/************************************************************************/

static int
KeepOnGoing(SSLSocket *ss)
{
    int rv;

    rv = TryToFinish(ss);
    if (rv) return -1;
    if (ss->handshake == 0) {
	return 0;
    }
    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = HandleMessage;
    return 0;
}

static int
HandleVerifyMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLGather *gs;
    unsigned char *data;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->gather != 0));
    sec = ss->sec;
    ci = sec->ci;
    gs = ss->gather;

    data = gs->buf.buf + gs->recordOffset;
    DUMP_MSG(29, (ss, data, gs->recordLen));
    if ((gs->recordLen != 1 + SSL_CHALLENGE_BYTES) ||
	(data[0] != SSL_MT_SERVER_VERIFY) ||
	PORT_Memcmp(data+1, ci->clientChallenge, SSL_CHALLENGE_BYTES)) {
	/* Bad server */
	PORT_SetError(SSL_ERROR_BAD_SERVER);
	goto loser;
    }
    ci->elements |= CIS_HAVE_VERIFY;

    SSL_TRC(5, ("%d: SSL[%d]: got server-verify, required=0x%d got=0x%x",
		SSL_GETPID(), ss->fd, ci->requiredElements, ci->elements));

    return KeepOnGoing(ss);

  loser:
    return -1;
}

int
ssl_HandleServerHelloMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLGather *gs;
    SSLSessionID *sid;
    int rv, needed, sidHit, certLen, csLen, cidLen, certType, err;
    unsigned char *cert, *cs, *data;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->gather != 0));
    sec = ss->sec;
    ci = sec->ci;
    gs = ss->gather;
    PORT_Assert(ci->sid != 0);

    if (!ss->enableSSL2) {
	PORT_SetError(SSL_ERROR_SSL2_DISABLED);
	return -1;
    }

    data = gs->buf.buf + gs->recordOffset;
    DUMP_MSG(29, (ss, data, gs->recordLen));

    /* Make sure first message has some data and is the server hello message */
    if ((gs->recordLen < SSL_HL_SERVER_HELLO_HBYTES)
	|| (data[0] != SSL_MT_SERVER_HELLO)) {
	if ((data[0] == SSL_MT_ERROR) && (gs->recordLen == 3)) {
	    err = (data[1] << 8) | data[2];
	    if (err == SSL_PE_NO_CYPHERS) {
		PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
		goto loser;
	    }
	}
	goto bad_server;
    }

    sidHit = data[1];
    certType = data[2];
    ss->version = (data[3] << 8) | data[4];
    certLen = (data[5] << 8) | data[6];
    csLen = (data[7] << 8) | data[8];
    cidLen = (data[9] << 8) | data[10];
    cert = data + SSL_HL_SERVER_HELLO_HBYTES;
    cs = cert + certLen;

    SSL_TRC(5,
	    ("%d: SSL[%d]: server-hello, hit=%d vers=%x certLen=%d csLen=%d cidLen=%d",
	     SSL_GETPID(), ss->fd, sidHit, ss->version, certLen,
	     csLen, cidLen));
    if (ss->version != SSL_LIBRARY_VERSION_2) {
        if (ss->version < SSL_LIBRARY_VERSION_2) {
	  SSL_TRC(3, ("%d: SSL[%d]: demoting self (%x) to server version (%x)",
		      SSL_GETPID(), ss->fd, SSL_LIBRARY_VERSION_2,
		      ss->version));
	} else {
	  SSL_TRC(1, ("%d: SSL[%d]: server version is %x (we are %x)",
		    SSL_GETPID(), ss->fd, ss->version, SSL_LIBRARY_VERSION_2));
	  /* server claims to be newer but does not follow protocol */
	  PORT_SetError(SSL_ERROR_UNSUPPORTED_VERSION);
	  goto loser;
	}
    }

    /* Save connection-id for later */
    PORT_Memcpy(ci->connectionID, cs + csLen, sizeof(ci->connectionID));

    /* See if session-id hit */
    needed = CIS_HAVE_MASTER_KEY | CIS_HAVE_FINISHED | CIS_HAVE_VERIFY;
    if (sidHit) {
	if (certLen || csLen) {
	    /* Uh oh - bogus server */
	    SSL_DBG(("%d: SSL[%d]: client, huh? hit=%d certLen=%d csLen=%d",
		     SSL_GETPID(), ss->fd, sidHit, certLen, csLen));
	    goto bad_server;
	}

	/* Total winner. */
	SSL_TRC(1, ("%d: SSL[%d]: client, using nonce for peer=0x%08x "
		    "port=0x%04x",
		    SSL_GETPID(), ss->fd, ci->peer, ci->port));
	sec->peerCert = CERT_DupCertificate(ci->sid->peerCert);
	rv = CreateSessionCypher(ss, ci->sid, PR_TRUE);
	if (rv < 0) {
	    goto loser;
	}
    } else {
	if (certType != SSL_CT_X509_CERTIFICATE) {
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE);
	    goto loser;
	}
	if (csLen == 0) {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	    SSL_DBG(("%d: SSL[%d]: no cipher overlap",
		     SSL_GETPID(), ss->fd));
	    goto loser;
	}
	if (certLen == 0) {
	    SSL_DBG(("%d: SSL[%d]: client, huh? certLen=%d csLen=%d",
		     SSL_GETPID(), ss->fd, certLen, csLen));
	    goto bad_server;
	}

	sid = ci->sid;
	if (sid->cached) {
	    /* Forget our session-id - server didn't like it */
	    SSL_TRC(7, ("%d: SSL[%d]: server forgot me, uncaching session-id",
			SSL_GETPID(), ss->fd));
	    (*sec->uncache)(sid);
	    ssl_FreeSID(sid);
	    ci->sid = sid = (SSLSessionID*) PORT_ZAlloc(sizeof(SSLSessionID));
	    if (!sid) {
		goto loser;
	    }
	    sid->references = 1;
	    sid->addr = ci->peer;
	    sid->port = ci->port;
	}

	/* decode the server's certificate */
	rv = ClientHandleServerCert(ss, cert, certLen);
	if (rv < 0) {
	    if (rv == SSL_ERROR_BAD_CERTIFICATE) {
		(void) SSL_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    }
	    goto loser;
	}

	/* Setup new session cipher */
	rv = ClientSetupSessionCypher(ss, cs, csLen);
	if (rv < 0) {
	    if (rv == SSL_ERROR_BAD_CERTIFICATE) {
		(void) SSL_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    }
	    goto loser;
	}
    }

    /* Build up final list of required elements */
    ci->elements = CIS_HAVE_MASTER_KEY;
    ci->requiredElements = needed;

    if ( sidHit ) {
	/* verify the certificate, don't check signatures */
	rv = (int)(* ss->sec->authCertificate)(ss->sec->authCertificateArg,
					       ss->fd, PR_FALSE, PR_FALSE);
    } else {
	/* verify the certificate, check signatures */
	rv = (int)(* ss->sec->authCertificate)(ss->sec->authCertificateArg,
					       ss->fd, PR_TRUE, PR_FALSE);
    }
    
    if (rv) {
	if ( ss->sec->handleBadCert ) {
	    rv = (* ss->sec->handleBadCert )(ss->sec->badCertArg,
					     ss->fd);
	    if ( rv ) {
		if ( rv == -2 ) {
		    /* someone will handle this connection asynchronously*/
		    /* WARNING - I RETURN HERE */

		    SSL_DBG(("%d: SSL[%d]: go to async cert handler",
			     SSL_GETPID(), ss->fd));
		    return -2;
		}
		/* cert is bad */
		SSL_DBG(("%d: SSL[%d]: server certificate is no good: error=%d",
			 SSL_GETPID(), ss->fd, PORT_GetError()));
		goto loser;

	    }
	    /* cert is good */
	} else {
	    SSL_DBG(("%d: SSL[%d]: server certificate is no good: error=%d",
		     SSL_GETPID(), ss->fd, PORT_GetError()));
	    goto loser;
	}
    }

    /*
    ** At this point we have a completed session key and our session
    ** cipher is setup and ready to go. Switch to encrypted write routine
    ** as all future message data is to be encrypted.
    */
    ssl_ChooseProcs(ss);

    rv = TryToFinish(ss);
    if (rv) goto loser;
    if (ss->handshake == 0) {
	return 0;
    }

    SSL_TRC(5, ("%d: SSL[%d]: got server-hello, required=0x%d got=0x%x",
		SSL_GETPID(), ss->fd, ci->requiredElements, ci->elements));
    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = HandleVerifyMessage;
    return 0;

  bad_server:
    PORT_SetError(SSL_ERROR_BAD_SERVER);
    /* FALL THROUGH */

  loser:
    return -1;
}

int
ssl_BeginClientHandshake(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLSessionID *sid;
    int i, rv, sendLen, sidLen;
    unsigned char *msg, *cp;
    unsigned char *localCipherSpecs = NULL;
    unsigned localCipherSize;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;
    sec->isServer = 0;
    ssl_ChooseSessionIDProcs(sec);
    sec->sendSequence = 0;
    sec->rcvSequence = 0;

    if (!cipherSpecs && (!!ss->enableSSL2 || !!ss->enableSSL3)) {
	ConstructCipherSpecs(NULL, &cipherSpecs, &sizeCipherSpecs);
    }

    if ((!ss->enableSSL2 && !ss->enableSSL3) || !cipherSpecs) {
	SSL_DBG(("%d: SSL[%d]: Can't handshake! both v2 and v3 disabled.",
		 SSL_GETPID(), ss->fd));
	PORT_SetError(SSL_ERROR_SSL_DISABLED);
	return SECFailure;
    }
    
    /* Get peer name of server */
    rv = ssl_GetPeerInfo(ss);
    if (rv < 0) {
	goto loser;
    }

    SSL_TRC(3, ("%d: SSL[%d]: sending client-hello", SSL_GETPID(), ss->fd));

    /* Try to find server in our session-id cache */
    if (ss->noCache) {
	sid = NULL;
    } else {
	sid = ssl_LookupSID(ci->peer, ci->port, ss->peerID);
    }
    if (sid) {
	if (((sid->version == SSL_LIBRARY_VERSION_2) && !ss->enableSSL2) ||
	    ((sid->version == SSL_LIBRARY_VERSION_3_0) && !ss->enableSSL3)) {
	    sec->uncache(sid);
	    goto invalid;
	}
	if (ss->enableSSL2) {
	    if (sid->version == SSL_LIBRARY_VERSION_2) {
		for (i = 0; i < sizeCipherSpecs; i += 3) {
		    if (cipherSpecs[i] == sid->u.ssl2.cipherType)
			goto sid_cipher_match;
		}
		goto invalid;
	    }
	}
    sid_cipher_match:
	sidLen = sizeof(sid->u.ssl2.sessionID);
	PRINT_BUF(4, (ss, "client, found session-id:", sid->u.ssl2.sessionID,
		      sidLen));
	ss->version = sid->version;
    } else {
    invalid:
	sidLen = 0;
	sid = (SSLSessionID*) PORT_ZAlloc(sizeof(SSLSessionID));
	if (!sid) {
	    goto loser;
	}
	sid->references = 1;
	sid->addr = ci->peer;
	sid->port = ci->port;
	if (ss->peerID == NULL) {
	    sid->peerID = NULL;
	} else {
	    sid->peerID = PORT_Strdup(ss->peerID);
	}
    }
    ci->sid = sid;

    PORT_Assert(sid != NULL);

    if (sid->version == SSL_LIBRARY_VERSION_3_0) {
	PORT_Assert(ss->gather != NULL);
	ss->gather->state = GS_INIT;
	ss->handshake = GatherRecord;
	ss->version = SSL_LIBRARY_VERSION_3_0;
	return SSL3_SendClientHello(ss);
    }
   
#ifdef FORTEZZA
    /* 
     * if SSL3 was turned off, don't use fortezza!
     */
    if (!ss->enableSSL3) {
    	if (!cipherSpecs) {
	    ConstructCipherSpecs(NULL, &cipherSpecs, &sizeCipherSpecs);
    	}
	localCipherSpecs = cipherSpecs;
	localCipherSize = sizeCipherSpecs;
    } else {
        rv = ConstructCipherSpecs(ss, &localCipherSpecs, &localCipherSize);
	if (rv < 0) return rv;
    }
#else 
    if (!cipherSpecs) {
	ConstructCipherSpecs(NULL, &cipherSpecs, &sizeCipherSpecs);
    }
    localCipherSpecs = cipherSpecs;
    localCipherSize = sizeCipherSpecs;
#endif
    sendLen = SSL_HL_CLIENT_HELLO_HBYTES + localCipherSize + sidLen +
	SSL_CHALLENGE_BYTES;
    rv = GetSendBuffer(ss, sendLen);
    if (rv) goto loser;

    /* Generate challenge bytes for server */
    RNG_GenerateGlobalRandomBytes(ci->clientChallenge, SSL_CHALLENGE_BYTES);

    /* Construct client-hello message */
    cp = msg = ci->sendBuf.buf;
    msg[0] = SSL_MT_CLIENT_HELLO;
    if ( ss->enableSSL3 ) {
	msg[1] = MSB(SSL_LIBRARY_VERSION_3_0);
	msg[2] = LSB(SSL_LIBRARY_VERSION_3_0);
    } else {
	msg[1] = MSB(SSL_LIBRARY_VERSION_2);
	msg[2] = LSB(SSL_LIBRARY_VERSION_2);
    }
    
    msg[3] = MSB(localCipherSize);
    msg[4] = LSB(localCipherSize);
    msg[5] = MSB(sidLen);
    msg[6] = LSB(sidLen);
    msg[7] = MSB(SSL_CHALLENGE_BYTES);
    msg[8] = LSB(SSL_CHALLENGE_BYTES);
    cp += SSL_HL_CLIENT_HELLO_HBYTES;
    PORT_Memcpy(cp, localCipherSpecs, localCipherSize);
    cp += localCipherSize;
    if (sidLen) {
	PORT_Memcpy(cp, sid->u.ssl2.sessionID, sidLen);
	cp += sidLen;
    }
    PORT_Memcpy(cp, ci->clientChallenge, SSL_CHALLENGE_BYTES);

    /* Send it to the server */
    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*sec->send)(ss, msg, sendLen, 0);
    if (rv < 0) {
	goto loser;
    }

    ssl3_StartHandshakeHash(ss, msg, sendLen);

    /* Setup to receive servers hello message */
    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = ssl_HandleServerHelloMessage;
    return 0;

  loser:
    return -1;
}

/************************************************************************/

static int
HandleClientSessionKeyMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLGather *gs;
    int rv, cipher, keySize, ckLen, ekLen, caLen;
    unsigned char *data;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->gather != 0));
    sec = ss->sec;
    gs = ss->gather;
    ci = sec->ci;

    data = gs->buf.buf + gs->recordOffset;
    DUMP_MSG(29, (ss, data, gs->recordLen));

    if ((gs->recordLen < SSL_HL_CLIENT_MASTER_KEY_HBYTES)
	|| (data[0] != SSL_MT_CLIENT_MASTER_KEY)) {
	goto bad_client;
    }
    cipher = data[1];
    keySize = (data[2] << 8) | data[3];
    ckLen = (data[4] << 8) | data[5];
    ekLen = (data[6] << 8) | data[7];
    caLen = (data[8] << 8) | data[9];

    SSL_TRC(5, ("%d: SSL[%d]: session-key, cipher=%d keySize=%d ckLen=%d ekLen=%d caLen=%d",
		SSL_GETPID(), ss->fd, cipher, keySize, ckLen, ekLen,
		caLen));

    if (gs->recordLen < SSL_HL_CLIENT_MASTER_KEY_HBYTES + ckLen
	+ ekLen + caLen) {
	SSL_DBG(("%d: SSL[%d]: protocol size mismatch dataLen=%d",
		 SSL_GETPID(), ss->fd, gs->recordLen));
	goto bad_client;
    }

    /* Use info from client to setup session key */
    /* XXX should validate cipher&keySize are in our array */
    rv = ServerSetupSessionCypher(ss, cipher, keySize,
		data+SSL_HL_CLIENT_MASTER_KEY_HBYTES, ckLen,
		data+SSL_HL_CLIENT_MASTER_KEY_HBYTES+ckLen, ekLen,
		data+SSL_HL_CLIENT_MASTER_KEY_HBYTES+ckLen+ekLen,
		caLen);
    if (rv < 0) {
	goto loser;
    }
    ci->elements |= CIS_HAVE_MASTER_KEY;
    ssl_ChooseProcs(ss);

    /* Send server verify message now that key's are established */
    rv = SendServerVerifyMessage(ss);
    if (rv) goto loser;

    rv = TryToFinish(ss);
    if (rv) goto loser;
    if (ss->handshake == 0) {
	return 0;
    }

    SSL_TRC(5, ("%d: SSL[%d]: server: waiting for elements=0x%d",
		SSL_GETPID(), ss->fd, ci->requiredElements ^ ci->elements));
    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = HandleMessage;
    return TriggerNextMessage(ss);

  bad_client:
    PORT_SetError(SSL_ERROR_BAD_CLIENT);
    /* FALLTHROUGH */

  loser:
    return -1;
}

/*
 * attempt to restart the handshake after asynchronously handling
 * a request for the client's certificate.
 */
int
SSL_RestartHandshakeAfterCertReq(SSLSocket *ss,
				CERTCertificate *cert, SECKEYPrivateKey *key,
						CERTCertificateList *certChain)
{
    int ret;
    SECStatus rv;
    
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SECItem response;

    if (ss->version == SSL_LIBRARY_VERSION_3_0) {
	if (ss->handshake != 0) {
	    ss->handshake = GatherRecord;
	    ss->ssl3->clientCertificate = cert;
	    ss->ssl3->clientCertChain = certChain;
	    if (key == NULL) {
		(void)SSL3_SendAlert(ss, alert_warning, no_certificate);
		ss->ssl3->clientPrivateKey = NULL;
	    } else {
		ss->ssl3->clientPrivateKey = SECKEY_CopyPrivateKey(key);
	    }
	    if (ss->ssl3->hs.msgState.buf != NULL) {
		rv = SSL3_HandleRecord(ss, NULL, &ss->gather->buf);
		return rv;
	    }
	}
	return 0;
    }

    response.data = NULL;
    
    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;

    /* generate error if no cert or key */
    if ( ( cert == NULL ) || ( key == NULL ) ) {
	goto no_cert;
    }
    
    /* generate signed response to the challenge */
    rv = SignResponse(ss, key, &response);
    if ( rv != SECSuccess ) {
	goto no_cert;
    }
    
    /* Send response message */
    ret = SendCertificateResponseMessage(ss, &cert->derCert, &response);
    if (ret) {
	goto no_cert;
    }

    /* try to finish the handshake */
    ret = TryToFinish(ss);
    if (ret) {
	goto loser;
    }
    
    /* done with handshake */
    if (ss->handshake == 0) {
	ret = 0;
	goto done;
    }

    /* continue handshake */
    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = HandleMessage;
    ret = TriggerNextMessage(ss);
    goto done;
    
no_cert:
    /* no cert - send error */
    ret = SSL_SendErrorMessage(ss, SSL_PE_NO_CERTIFICATE);
    goto done;
    
loser:
    ret = -1;
done:
    /* free allocated data */
    if ( response.data ) {
	PORT_Free(response.data);
    }
    
    return(ret);
}

#ifdef FORTEZZA
/* restart an SSL connection that we stopped to run certificate dialogs */
SECStatus
SSL_RestartHandshakeAfterFortezza(SSLSocket *ss)
{
    int rv;
    
    /*
    ** At this point we have a completed session key and our session
    ** cipher is setup and ready to go. Switch to encrypted write routine
    ** as all future message data is to be encrypted.
    */

    if (ss->reStartType == 3) {
	rv = SSL3_SendClientHello(ss);
    } else {
	rv = ssl_BeginClientHandshake(ss);
    }
    return rv;
}
#endif

/* restart an SSL connection that we stopped to run certificate dialogs */
int
SSL_RestartHandshakeAfterServerCert(SSLSocket *ss)
{
    int rv;
    
    /*
    ** At this point we have a completed session key and our session
    ** cipher is setup and ready to go. Switch to encrypted write routine
    ** as all future message data is to be encrypted.
    */

    if (ss->version == SSL_LIBRARY_VERSION_3_0) {
	if (ss->handshake != 0) {
	    ss->handshake = GatherRecord;
	    SSL3_CleanupPeerCerts(ss->ssl3);
	    ss->sec->ci->sid->peerCert = CERT_DupCertificate(ss->sec->peerCert);

	    if (ss->ssl3->hs.msgState.buf != NULL) {
		rv = SSL3_HandleRecord(ss, NULL, &ss->gather->buf);
		return rv;
	    }
	}
	return 0;
    }
    
    ssl_ChooseProcs(ss);

    rv = TryToFinish(ss);
    if (rv) {
	return -1;
    }
    
    if (ss->handshake == 0) {
	return 0;
    }

    SSL_TRC(5, ("%d: SSL[%d]: got server-hello, required=0x%d got=0x%x",
		SSL_GETPID(), ss->fd, ss->sec->ci->requiredElements,
		ss->sec->ci->elements));
    ss->gather->recordLen = 0;

    ss->handshake = GatherRecord;
    ss->nextHandshake = HandleVerifyMessage;
    return 0;
}

/*
** Handle the initial hello message from the client
*/
int
ssl_HandleClientHelloMessage(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;
    SSLGather *gs;
    SSLSessionID *sid = NULL;
    int needed, rv, hit, csLen, sendLen, sdLen, certLen, pid;
    unsigned challengeLen;
    unsigned char *msg, *data, *cs, *sd, *cert = 0, *challenge;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0) && (ss->gather != 0));
    sec = ss->sec;
    gs = ss->gather;
    ci = sec->ci;

    data = gs->buf.buf + gs->recordOffset;
    DUMP_MSG(29, (ss, data, gs->recordLen));

    /* Make sure first message has some data and is the client hello message */
    if ((gs->recordLen < SSL_HL_CLIENT_HELLO_HBYTES)
	|| (data[0] != SSL_MT_CLIENT_HELLO)) {
	goto bad_client;
    }

    /* Get peer name of client */
    rv = ssl_GetPeerInfo(ss);
    if (rv < 0) {
	goto loser;
    }

    /* Examine version information */
    /*
     * See if this might be a V3 client hello or a V2 client hello
     * asking to use the V3 protocol
     */
    if ((data[0] == SSL_MT_CLIENT_HELLO) && (data[1] == 3) && ss->enableSSL3) {
	rv = SSL3_HandleV2ClientHello(ss, data, gs->recordLen);
	if (rv != SECFailure) {
	    ss->handshake = NULL;
	    ss->nextHandshake = GatherRecord;
	    ss->securityHandshake = NULL;
	    ss->gather->state = GS_INIT;
	    ss->version = SSL_LIBRARY_VERSION_3_0;
	    ss->sec->ci->sid->version = SSL_LIBRARY_VERSION_3_0;
	}
	return rv;
    }
    if (!ss->enableSSL2) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
	return SECFailure;
    }

    /* Extract info from message */
    ss->version = (data[1] << 8) | data[2];
    if (ss->version >= SSL_LIBRARY_VERSION_3_0) {
	ss->version = SSL_LIBRARY_VERSION_2;
    }
    
    csLen = (data[3] << 8) | data[4];
    sdLen = (data[5] << 8) | data[6];
    challengeLen = (data[7] << 8) | data[8];
    cs = data + SSL_HL_CLIENT_HELLO_HBYTES;
    sd = cs + csLen;
    challenge = sd + sdLen;
    PRINT_BUF(7, (ss, "server, client session-id value:", sd, sdLen));

    if (gs->recordLen != SSL_HL_CLIENT_HELLO_HBYTES+csLen+sdLen+challengeLen) {
	SSL_DBG(("%d: SSL[%d]: bad client hello message, len=%d should=%d",
		 SSL_GETPID(), ss->fd, gs->recordLen,
		 SSL_HL_CLIENT_HELLO_HBYTES+csLen+sdLen+challengeLen));
	goto bad_client;
    }

    SSL_TRC(3, ("%d: SSL[%d]: client version is %x",
		SSL_GETPID(), ss->fd, ss->version));
    if (ss->version != SSL_LIBRARY_VERSION_2) {
	if (ss->version > SSL_LIBRARY_VERSION_2) {
	    /*
	    ** Newer client than us. Things are ok because new clients
	    ** are required to be backwards compatible with old servers.
	    ** Change version number to our version number so that client
	    ** knows whats up.
	    */
	    ss->version = SSL_LIBRARY_VERSION_2;
	} else {
	    /*
	    ** XXX here is where server backwards compatability with old
	    ** XXX clients code belongs
	    */
	    SSL_TRC(1, ("%d: SSL[%d]: client version is %x (we are %x)",
		SSL_GETPID(), ss->fd, ss->version, SSL_LIBRARY_VERSION_2));
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_VERSION);
	    goto loser;
	}
    }

    /* Qualify cipher specs before returning them to client */
    csLen = QualifyCypherSpecs(ss, cs, csLen);
    if (csLen == 0) {
	SSL_SendErrorMessage(ss, SSL_PE_NO_CYPHERS);
	PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	goto loser;
    }

    /* Squirrel away the challenge for later */
    PORT_Memcpy(ci->clientChallenge, challenge, challengeLen);

    /* Examine message and see if session-id is good */
    needed = 0;
    ci->elements = 0;
    if (ss->noCache) {
	sid = NULL;
    } else if (sdLen) {
	SSL_TRC(7, ("%d: SSL[%d]: server, lookup client session-id for 0x%08x",
		    SSL_GETPID(), ss->fd, ci->peer));
	sid = (*ssl_sid_lookup)(ci->peer, sd, sdLen);
    }
    if (sid) {
	/* Got a good session-id. Short cut! */
	SSL_TRC(1, ("%d: SSL[%d]: server, using session-id for 0x%08x (age=%d)",
		    SSL_GETPID(), ss->fd, ci->peer,
		    XP_TIME() - sid->time));
	PRINT_BUF(1, (ss, "session-id value:", sd, sdLen));
	ci->sid = sid;
	ci->elements = CIS_HAVE_MASTER_KEY;
	hit = 1;
	certLen = 0;
	csLen = 0;
	rv = CreateSessionCypher(ss, sid, PR_FALSE);
	if (rv < 0) {
	    goto loser;
	}
    } else {
	SSL_TRC(7, ("%d: SSL[%d]: server, lookup nonce missed",
		    SSL_GETPID(), ss->fd));
	hit = 0;
	sid = (SSLSessionID*) PORT_ZAlloc(sizeof(SSLSessionID));
	if (!sid) {
	    goto loser;
	}
	sid->references = 1;
	sid->addr = ci->peer;
	sid->port = ci->port;

	/* Invent a session-id */
	ci->sid = sid;
	RNG_GenerateGlobalRandomBytes(
	    sid->u.ssl2.sessionID+2, SSL_SESSIONID_BYTES-2);

	pid = SSL_GETPID();
	sid->u.ssl2.sessionID[0] = MSB(pid);
	sid->u.ssl2.sessionID[1] = LSB(pid);
	cert = ssl_server_signed_certificate.data;
	certLen = ssl_server_signed_certificate.len;
    }

    /* Build up final list of required elements */
    ci->requiredElements = CIS_HAVE_MASTER_KEY | CIS_HAVE_FINISHED;
    if (ss->requestCertificate) {
	ci->requiredElements |= CIS_HAVE_CERTIFICATE;
    }
    ci->sentElements = 0;

    /* Send hello message back to client */
    sendLen = SSL_HL_SERVER_HELLO_HBYTES + certLen + csLen
	+ SSL_CONNECTIONID_BYTES;
    rv = GetSendBuffer(ss, sendLen);
    if (rv < 0) {
	goto loser;
    }

    SSL_TRC(3, ("%d: SSL[%d]: sending server-hello (%d)",
		SSL_GETPID(), ss->fd, sendLen));

    msg = ci->sendBuf.buf;
    msg[0] = SSL_MT_SERVER_HELLO;
    msg[1] = hit;
    msg[2] = SSL_CT_X509_CERTIFICATE;/* XXX */
    msg[3] = MSB(ss->version);
    msg[4] = LSB(ss->version);
    msg[5] = MSB(certLen);
    msg[6] = LSB(certLen);
    msg[7] = MSB(csLen);
    msg[8] = LSB(csLen);
    msg[9] = MSB(SSL_CONNECTIONID_BYTES);
    msg[10] = LSB(SSL_CONNECTIONID_BYTES);
    if (certLen) {
	PORT_Memcpy(msg+SSL_HL_SERVER_HELLO_HBYTES, cert, certLen);
    }
    if (csLen) {
	PORT_Memcpy(msg+SSL_HL_SERVER_HELLO_HBYTES+certLen, cs, csLen);
    }
    PORT_Memcpy(msg+SSL_HL_SERVER_HELLO_HBYTES+certLen+csLen, ci->connectionID,
	      SSL_CONNECTIONID_BYTES);

    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*sec->send)(ss, msg, sendLen, 0);
    if (rv < 0) {
	goto loser;
    }
    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    if (hit) {
	/* Session key is good. Go encrypted */
	ssl_ChooseProcs(ss);

	/* Send server verify message now that key's are established */
	rv = SendServerVerifyMessage(ss);
	if (rv) goto loser;

	ss->nextHandshake = HandleMessage;
	return TriggerNextMessage(ss);
    }
    ss->nextHandshake = HandleClientSessionKeyMessage;
    return 0;

  bad_client:
    PORT_SetError(SSL_ERROR_BAD_CLIENT);
    /* FALLTHROUGH */

  loser:
    SSL_TRC(10, ("%d: SSL[%d]: server, wait for client-hello lossage",
		 SSL_GETPID(), ss->fd));
    return -1;
}

int
ssl_BeginServerHandshake(SSLSocket *ss)
{
    SSLSecurityInfo *sec;
    SSLConnectInfo *ci;

    PORT_Assert((ss->sec != 0) && (ss->sec->ci != 0));
    sec = ss->sec;
    ci = sec->ci;
    sec->isServer = 1;
    ssl_ChooseSessionIDProcs(sec);
    sec->sendSequence = 0;
    sec->rcvSequence = 0;

    if (!cipherSpecs && (!!ss->enableSSL2 || !!ss->enableSSL3)) {
	ConstructCipherSpecs(NULL, &cipherSpecs, &sizeCipherSpecs);
    }

    if ((!ss->enableSSL2 && !ss->enableSSL3) || !cipherSpecs) {
	SSL_DBG(("%d: SSL[%d]: Can't handshake! both v2 and v3 disabled.",
		 SSL_GETPID(), ss->fd));
	return SECFailure;
    }

#ifndef FORTEZZA
    /* you don't need these keys if fortezza is defined  */
    if (!ssl_server_key || !ssl_server_signed_certificate.data) {
	/* Need to have these defined before you can be a secure server */
	PORT_SetError(SSL_ERROR_BAD_SERVER);
	return -1;
    }
#endif

    /*
    ** Generate connection-id. Always do this, even if things fail
    ** immediately. This way the random number generator is always
    ** rolling around, every time we get a connection.
    */
    RNG_GenerateGlobalRandomBytes(ci->connectionID, sizeof(ci->connectionID));

    ss->gather->recordLen = 0;
    ss->handshake = GatherRecord;
    ss->nextHandshake = ssl_HandleClientHelloMessage;
    return 0;
}

void
ssl_DestroyConnectInfo(SSLSecurityInfo *sec)
{
    SSLConnectInfo *ci;
    SSLSessionID *sid;

    ci = sec->ci;
    if (ci) {
	PORT_FreeBlock(ci->sendBuf.buf);
	if ((sid = ci->sid) != NULL) {
	    ssl_FreeSID(sid);
	}
	PORT_ZFree(ci, sizeof(SSLConnectInfo));
	sec->ci = NULL;
    }
}

