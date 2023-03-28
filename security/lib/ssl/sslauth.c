#include "xp.h"
#include "cert.h"
#include "secitem.h"
#include "key.h"
#include "crypto.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"
#include "pk11func.h"

CERTCertificate *SSL_PeerCertificate(int fd)
{
    SSLSocket *ss;
    SSLSecurityInfo *sec;

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in PeerCertificate",
		 SSL_GETPID(), fd));
	return 0;
    }
    sec = ss->sec;
    if (ss->useSecurity && sec && sec->peerCert) {
	return CERT_DupCertificate(sec->peerCert);
    }
    return 0;
}

int
SSL_SecurityStatus(int fd, int *op, char **cp, int *kp0, int *kp1,
		   char **ip, char **sp)
{
    SSLSocket *ss;
    SSLSecurityInfo *sec;
    char *cipherName;
    PRBool isDes = PR_FALSE;

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SecurityStatus",
		 SSL_GETPID(), fd));
	return -1;
    }

    if (cp) *cp = 0;
    if (kp0) *kp0 = 0;
    if (kp1) *kp1 = 0;
    if (ip) *ip = 0;
    if (sp) *sp = 0;
    if (op) {
	*op = SSL_SECURITY_STATUS_OFF;
    }

    if (ss->useSecurity && ss->connected) {
	PORT_Assert(ss->sec != 0);
	sec = ss->sec;

	if (ss->version == SSL_LIBRARY_VERSION_2) {
	    cipherName = ssl_cipherName[sec->cipherType];
	} else {
	    cipherName = ssl3_cipherName[sec->cipherType];
	}
	if (cipherName && PORT_Strstr(cipherName, "DES")) isDes = PR_TRUE;
    
	if (cp) {
	    *cp = PORT_Strdup(cipherName);
	}

	if (kp0) {
	    *kp0 = sec->keyBits;
	    if (isDes) *kp0 = (*kp0 * 7) / 8;
	}
	if (kp1) {
	    *kp1 = sec->secretKeyBits;
	    if (isDes) *kp1 = (*kp1 * 7) / 8;
	}
	*op = SSL_SECURITY_STATUS_ON_HIGH;
	/* XXX - jsw says this test is bogus.  It should compare keyBits
	 * to some absolute number (maybe 70-90 since that is what the crypto
	 * weenies say is necessary.
	 */
	if (sec->keyBits != sec->secretKeyBits) {
	    *op = SSL_SECURITY_STATUS_ON_LOW;
	}

#ifdef FORTEZZA
	if (ss->ssl3 && ss->ssl3->hs.kea == kea_fortezza) {
	    *op = SSL_SECURITY_STATUS_FORTEZZA;
	}
#endif

	if (sec->keyBits == 0) {
	    *op = SSL_SECURITY_STATUS_OFF;
	}

	if (ip || sp) {
	    CERTCertificate *cert;

	    cert = sec->peerCert;
	    if (cert) {
		if (ip) {
		    *ip = CERT_NameToAscii(&cert->issuer);
		}
		if (sp) {
		    *sp = CERT_NameToAscii(&cert->subject);
		}
	    } else {
		if (ip) {
		    *ip = PORT_Strdup("no certificate");
		}
		if (sp) {
		    *sp = PORT_Strdup("no certificate");
		}
	    }
	}
    }

    return 0;
}

/************************************************************************/

int
SSL_AuthCertificateHook(int s, SSLAuthCertificate func, void *arg)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in AuthCertificateHook",
		 SSL_GETPID(), s));
	return -1;
    }

#ifndef NADA_VERSION
    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return(rv);
    }
    ss->sec->authCertificate = func;
    ss->sec->authCertificateArg = arg;
#endif
    return(0);
}

int SSL_GetClientAuthDataHook(int s, SSLGetClientAuthData func, void *arg)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in GetClientAuthDataHook",
		 SSL_GETPID(), s));
	return -1;
    }

#ifndef NADA_VERSION
    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return rv;
    }
    ss->sec->getClientAuthData = func;
    ss->sec->getClientAuthDataArg = arg;
#endif
    return 0;
}

#ifndef NADA_VERSION

/*
** Encrypt the challenge data with the private key from the key
** file. Have to convert challenge data into something more encryptable
** for RSA encryption. Insert a 0 byte in front of the challenge data,
** and then pad it with 0xff after the challenge data.
*/
int SSL_EncryptChallenge(SECItem *encodedChallenge, SECItem *challenge,
			 SECKEYPrivateKey *privateKey)
{
    encodedChallenge->len = PK11_SignatureLen(privateKey);
    encodedChallenge->data = PORT_Alloc(encodedChallenge->len);
    if (encodedChallenge->data == NULL) {
	return SECFailure;
    }

    return PK11_Sign(privateKey,encodedChallenge,challenge);
}

int
SSL_AuthCertificate(void *arg, int fd, PRBool checkSig, PRBool isServer)
{
    SECStatus rv;
    CERTCertDBHandle *handle;
    SSLSocket *ss;
    SECCertUsage certUsage;
    
    ss = ssl_FindSocket(fd);
    
    handle = (CERTCertDBHandle *)arg;

    if ( isServer ) {
	certUsage = certUsageSSLClient;
    } else {
	certUsage = certUsageSSLServer;
    }
    
    rv = CERT_VerifyCertNow(handle, ss->sec->peerCert, checkSig, certUsage,
			    arg);

    return((int)rv);
}
#endif

