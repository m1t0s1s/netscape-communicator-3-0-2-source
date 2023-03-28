/*
 * SSL3 stuff.
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: ssl3con.c,v 1.79.2.8 1997/05/24 00:24:37 jwz Exp $
 */

#include "xp.h"
#include "cert.h"
#include "ssl.h"
#include "crypto.h"
#include "key.h"
#include "secder.h"
#include "sechash.h"
#include "rsa.h"
#include "secrng.h"
#include "secitem.h"

#include "sslimpl.h"
#include "sslproto.h"
#include "sslerr.h"
#include "prtime.h"
#ifdef NSPR20
#include "prinrval.h"
#endif /* NSPR20 */

#include "pk11func.h"

extern int SEC_ERROR_IO;
extern int SEC_ERROR_INVALID_ARGS;
extern int SEC_ERROR_INVALID_KEY;
extern int SEC_ERROR_LIBRARY_FAILURE;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_BAD_KEY;
extern int SSL_ERROR_BAD_CERTIFICATE;
extern int SSL_ERROR_BAD_CLIENT;
extern int SSL_ERROR_BAD_SERVER;
extern int SSL_ERROR_EXPORT_ONLY_SERVER;
extern int SSL_ERROR_NO_CERTIFICATE;
extern int SSL_ERROR_NO_CYPHER_OVERLAP;
extern int SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE;
extern int SSL_ERROR_UNSUPPORTED_VERSION;
extern int SSL_ERROR_US_ONLY_SERVER;
extern int SSL_ERROR_BAD_MAC_READ;
extern int SSL_ERROR_BAD_MAC_ALERT;
extern int SSL_ERROR_BAD_CERT_ALERT;
extern int SSL_ERROR_REVOKED_CERT_ALERT;
extern int SSL_ERROR_EXPIRED_CERT_ALERT;
extern int XP_ERRNO_EIO;
extern int XP_ERRNO_EWOULDBLOCK;

/*
 * XXX 'sizeof' is most likely wrong in each of its uses below. It will
 * work on a 32 bit architecture (probably)
 */

static SECStatus ssl3_InitState(SSLSocket *ss);
static SECStatus ssl3_SendFinished(SSLSocket *ss);
static SECStatus ssl3_SendCertificate(SSLSocket *ss);
static SECStatus ssl3_SendCertificateRequest(SSLSocket *ss);
static SECStatus ssl3_SendServerKeyExchange(SSLSocket *ss);
static SECStatus ssl3_SendServerHelloDone(SSLSocket *ss);
static SECStatus SSL3_SendServerHello(SSLSocket *ss);
static SSLSessionID *ssl3_NewSessionID(SSLSocket *ss, PRBool is_server);
static SECStatus ssl3_HandshakeFailure(SSLSocket *ss);
static SECStatus ssl3_GenerateSessionKeys(SSLSocket *ss, SSL3CipherSpec *spec,
					 const SECItem *pms);

static SECStatus Null_Cipher(void *ctx, unsigned char *output,
			    unsigned int *outputLen, unsigned maxOutputLen,
			    unsigned char *input, unsigned inputLen);

#define MAX_SEND_BUF_LENGTH 32000 /* watch for 16-bit integer overflow */
#define MIN_SEND_BUF_LENGTH  4000

#define MAX_CIPHER_SUITES 20

/* This list of SSL3 cipher suites is sorted in descending order of 
 * precedence (desirability).  It only includes cipher suites we implement.
 */
static SSL3CipherSuiteCfg cipherSuites[] = {
#ifdef FORTEZZA
    { SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA, SSL_ALLOWED, PR_TRUE },
    { SSL_FORTEZZA_DMS_WITH_RC4_128_SHA,      SSL_ALLOWED, PR_TRUE },
#endif /* FORTEZZA */
    { SSL_RSA_WITH_RC4_128_MD5,               SSL_ALLOWED, PR_TRUE },
    { SSL_RSA_WITH_3DES_EDE_CBC_SHA,          SSL_ALLOWED, PR_TRUE },
    { SSL_RSA_WITH_DES_CBC_SHA,               SSL_ALLOWED, PR_TRUE },
    { SSL_RSA_EXPORT_WITH_RC4_40_MD5,         SSL_ALLOWED, PR_TRUE },
    { SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,     SSL_ALLOWED, PR_TRUE },
    { SSL_RSA_WITH_NULL_MD5,                  SSL_ALLOWED, PR_FALSE },
#ifdef FORTEZZA
    { SSL_FORTEZZA_DMS_WITH_NULL_SHA,         SSL_ALLOWED, PR_FALSE },
#endif /* FORTEZZA */
};

static int cipherSuiteCount = sizeof(cipherSuites) / sizeof(cipherSuites[0]);

static /*SSL3CompressionMethod*/ uint8 compressions [] = {
    compression_null
};

static int compressionMethodsCount =
		sizeof(compressions) / sizeof(compressions[0]);

static /*SSL3ClientCertificateType */ uint8 certificate_types [] = {
    ct_RSA_sign,
    ct_DSS_sign,
#ifdef FORTEZZA
    ct_Fortezza,
#endif
};
#if 0
static int certificateTypeCount =
    sizeof(certificate_types) / sizeof(certificate_types[0]);
#endif

#define EXPORT_RSA_KEY_LENGTH 64
/*
 * make sure there is room in the write buffer for padding and
 * other compression and cryptographic expansions
 */
#define SSL3_BUFFER_FUDGE     100

SECKEYPrivateKey *exportKey;
SECKEYPublicKey *exportPubKey;
static SECKEYPrivateKey *usKey;
#ifdef FORTEZZA
static SECKEYPrivateKey *fortezzaServerKey;
#endif

/* This is a hack to make sure we don't do double handshakes for US policy */
static PRBool policy_some_restricted = PR_FALSE;

CERTCertificateList *ssl3_server_cert_chain = NULL;
CERTDistNames *ssl3_server_ca_list = NULL;
#ifdef FORTEZZA
CERTCertificateList *ssl3_fortezza_server_cert_chain = NULL;
CERTDistNames *ssl3_fortezza_server_ca_list = NULL;
#endif

/* indexed by SSL3BulkCipher */
static const SSL3BulkCipherDef bulk_cipher_defs[] = {
    {cipher_null,      calg_null,      0,  0, type_stream,  0, 0, kg_null},
    {cipher_rc4,       calg_rc4,      16, 16, type_stream,  0, 0, kg_strong},
    {cipher_rc4_40,    calg_rc4,      16,  5, type_stream,  0, 0, kg_export},
    {cipher_rc2,       calg_rc2,      16, 16, type_block,   8, 8, kg_strong},
    {cipher_rc2_40,    calg_rc2,      16,  5, type_block,   8, 8, kg_export},
    {cipher_des,       calg_des,       8,  8, type_block,   8, 8, kg_strong},
    {cipher_3des,      calg_3des,     24, 24, type_block,   8, 8, kg_strong},
    {cipher_des40,     calg_des,       8,  5, type_block,   8, 8, kg_export},
    {cipher_idea,      calg_idea,     16, 16, type_block,   8, 8, kg_strong},
    {cipher_fortezza,  calg_fortezza, 12, 12, type_block,  24, 8, kg_null},
    {cipher_missing,   calg_null,      0,  0, type_stream,  0, 0, kg_null},
};

static const SSL3KEADef kea_defs[] = { /* indexed by SSL3KeyExchangeAlgorithm */
    {kea_null,           kt_null,     sign_null, PR_FALSE,   0},
    {kea_rsa,            kt_rsa,      sign_rsa,  PR_FALSE,   0},
    {kea_rsa_export,     kt_rsa,      sign_rsa,  PR_TRUE,  512},
    {kea_dh_dss,         kt_dh,       sign_dsa,  PR_FALSE,   0},
    {kea_dh_dss_export,  kt_dh,       sign_dsa,  PR_TRUE,  512},
    {kea_dh_rsa,         kt_dh,       sign_rsa,  PR_FALSE,   0},
    {kea_dh_rsa_export,  kt_dh,       sign_rsa,  PR_TRUE,  512},
    {kea_dhe_dss,        kt_dh,       sign_dsa,  PR_FALSE,   0},
    {kea_dhe_dss_export, kt_dh,       sign_dsa,  PR_TRUE,  512},
    {kea_dhe_rsa,        kt_dh,       sign_rsa,  PR_FALSE,   0},
    {kea_dhe_rsa_export, kt_dh,       sign_rsa,  PR_TRUE,  512},
    {kea_dh_anon,        kt_dh,       sign_null, PR_FALSE,   0},
    {kea_dh_anon_export, kt_dh,       sign_null, PR_TRUE,  512},
    {kea_fortezza,       kt_fortezza, sign_dsa,  PR_FALSE,   0},
};

static const SSL3MACDef mac_defs[] = { /* indexed by MACAlgorithm */
    {mac_null,  0},
    {mac_md5,  48},
    {mac_sha,  40},
};

/* must use ssl_LookupCipherSuiteDef to access */
static const SSL3CipherSuiteDef cipher_suite_defs[] = {
    {SSL_NULL_WITH_NULL_NULL,
     cipher_null,   mac_null, kea_null},
    {SSL_RSA_WITH_NULL_MD5,
     cipher_null,   mac_md5, kea_rsa},
    {SSL_RSA_WITH_NULL_SHA,
     cipher_null,   mac_sha, kea_rsa},
    {SSL_RSA_EXPORT_WITH_RC4_40_MD5,
     cipher_rc4_40, mac_md5, kea_rsa_export},
    {SSL_RSA_WITH_RC4_128_MD5,
     cipher_rc4,    mac_md5, kea_rsa},
    {SSL_RSA_WITH_RC4_128_SHA,
     cipher_rc4,    mac_sha, kea_rsa},
    {SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,
     cipher_rc2_40, mac_md5, kea_rsa_export},
#if 0 /* not implemented */
    {SSL_RSA_WITH_IDEA_CBC_SHA,
     cipher_idea,   mac_sha, kea_rsa},
    {SSL_RSA_EXPORT_WITH_DES40_CBC_SHA,
     cipher_des40,  mac_sha, kea_rsa_export},
#endif
    {SSL_RSA_WITH_DES_CBC_SHA,
     cipher_des,    mac_sha, kea_rsa},
    {SSL_RSA_WITH_3DES_EDE_CBC_SHA,
     cipher_3des,   mac_sha, kea_rsa},

#if 0 /* not implemented */
    {SSL_DH_DSS_EXPORT_WITH_DES40_CBC_SHA,
     cipher_des40,  mac_sha, kea_dh_dss_export},
    {SSL_DH_DSS_DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_dss},
    {SSL_DH_DSS_3DES_CBC_SHA,
     cipher_3des,   mac_sha, kea_dh_dss},
    {SSL_DH_RSA_EXPORT_WITH_DES40_CBC_SHA,
     cipher_des40,  mac_sha, kea_dh_rsa_export},
    {SSL_DH_RSA_DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_rsa},
    {SSL_DH_RSA_3DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_rsa},
    {SSL_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA,
     cipher_des40,  mac_sha, kea_dh_dss_export},
    {SSL_DHE_DSS_DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_dss},
    {SSL_DHE_DSS_3DES_CBC_SHA,
     cipher_3des,   mac_sha, kea_dh_dss},
    {SSL_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA,
     cipher_des40,  mac_sha, kea_dh_rsa_export},
    {SSL_DHE_RSA_DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_rsa},
    {SSL_DHE_RSA_3DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_rsa},
    {SSL_DH_ANON_EXPORT_RC4_40_MD5,
     cipher_rc4_40, mac_md5, kea_dh_anon_export},
    {SSL_DH_ANON_EXPORT_RC4_40_MD5,
     cipher_rc4,    mac_md5, kea_dh_anon_export},
    {SSL_DH_ANON_EXPORT_WITH_DES40_CBC_SHA,
     cipher_des40,  mac_sha, kea_dh_anon_export},
    {SSL_DH_ANON_DES_CBC_SHA,
     cipher_des,    mac_sha, kea_dh_anon},
    {SSL_DH_ANON_3DES_CBC_SHA,
     cipher_3des,   mac_sha, kea_dh_anon},
#endif

#ifdef FORTEZZA 
    {SSL_FORTEZZA_DMS_WITH_NULL_SHA,
     cipher_null,     mac_sha, kea_fortezza},
    {SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA,
     cipher_fortezza, mac_sha, kea_fortezza},
    {SSL_FORTEZZA_DMS_WITH_RC4_SHA,
     cipher_rc4,      mac_sha, kea_fortezza},
#endif
};

char *ssl3_cipherName[] = {
    "NULL",
    "RC4",
    "RC4-40",
    "RC2-CBC",
    "RC2-CBC-40",
    "DES-CBC",
    "3DES-EDE-CBC",
    "DES-CBC-40",
    "IDEA-CBC",
    "FORTEZZA",
    "missing"
};

static const SSL3CipherSuiteDef *
ssl_LookupCipherSuiteDef(SSL3CipherSuite suite)
{
    int cipher_suite_def_len =
	sizeof(cipher_suite_defs) / sizeof(cipher_suite_defs[0]);
    int i;

    for (i = 0; i < cipher_suite_def_len; i++) {
	if (cipher_suite_defs[i].cipher_suite == suite)
	    return &cipher_suite_defs[i];
    }
    PORT_Assert(PR_FALSE);  /* We should never get here. */
    return NULL;
}

/* Find the cipher configuration struct associate with suite */
static SSL3CipherSuiteCfg *
ssl_LookupCipherSuiteCfg(SSL3CipherSuite suite)
{
    int i;

    for (i = 0; i < cipherSuiteCount; i++) {
	if (cipherSuites[i].cipher_suite == suite)
	    return &cipherSuites[i];
    }
    PORT_Assert(PR_FALSE);  /* We should never get here. */
    return NULL;
}

/* return PR_TRUE if suite matches policy and enabled state */
static PRBool
config_match(SSL3CipherSuiteCfg *suite, int policy, PRBool enabled)
{
    if (policy == SSL_NOT_ALLOWED) {
	return ((suite->policy == policy) && (suite->enabled == enabled));
    } else {
	return ((suite->policy != SSL_NOT_ALLOWED) &&
		(suite->policy <= policy) && (suite->enabled == enabled));
    }
}

/* return number of cipher suites that match policy and enabled state */
static int
count_cipher_suites(int policy, PRBool enabled)
{
    int i, count = 0;

    for (i = 0; i < cipherSuiteCount; i++) {
	if (config_match(&cipherSuites[i], policy, enabled))
	    count++;
    }
    return count;
}

/*
 * Null compression, mac and encryption functions
 */

static SECStatus
Null_Cipher(void *ctx, unsigned char *output,
	    unsigned int *outputLen, unsigned maxOutputLen,
	    unsigned char *input, unsigned inputLen)
{
    *outputLen = inputLen;
    if (input != output)
	PORT_Memcpy(output, input, inputLen);
    return SECSuccess;
}


/*
 * SSL3 Utility functions
 */

static SECStatus
ssl3_GetNewRandom(SSL3Random *random)
{
#ifdef NSPR20
    PRIntervalTime gmt = PR_IntervalToSeconds(PR_IntervalNow());
#else
    int64 now = PR_NowS();
    uint32 gmt;
    LL_L2I(gmt, now);
#endif /* NSPR20 */

    random->rand[0] = (unsigned char)(gmt >> 24);
    random->rand[1] = (unsigned char)(gmt >> 16);
    random->rand[2] = (unsigned char)(gmt >>  8);
    random->rand[3] = (unsigned char)(gmt);

    /* first 4 bytes are reserverd for time */
    return PK11_GenerateRandom(&random->rand[4], SSL3_RANDOM_LENGTH - 4);
}

static SECStatus
ssl3_SignHashes(SSL3Hashes *hash, SECKEYPrivateKey *key, SECItem *buf)
{
    SECStatus rv;
    SECItem hashItem;

    switch (key->keyType) {
    case rsaKey: 
    	hashItem.data = (unsigned char *)hash;
    	hashItem.len = sizeof(SSL3Hashes);
	break;
    case dsaKey:
    case fortezzaKey:
	hashItem.data = (unsigned char *)hash->sha;
	hashItem.len = sizeof(hash->sha);
	break;
    default:
	PORT_SetError(SEC_ERROR_INVALID_KEY);
	return SECFailure;
    }
    rv = PK11_Sign(key, buf, &hashItem);
    PRINT_BUF(60, (NULL, "signed hashes", (unsigned char*)buf->data, buf->len));
    /*if (rv < 0) PORT_SetError(SEC_ERROR_LIBRARY_FAILURE); */
    return rv;
}


static SECStatus
ssl3_CheckSignedHashes(SSL3Hashes *hash, CERTCertificate *cert, SECItem *buf,
	void *wincx)
{
    int rv;
    SECKEYPublicKey *key;
    SECItem hashItem;


    PRINT_BUF(60, (NULL, "check signed hashes", (unsigned char*)buf->data,
		   buf->len));

    key = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if (key == NULL) return SECFailure;
   
    switch (key->keyType) {
    case rsaKey: 
    	hashItem.data = (unsigned char *)hash;
    	hashItem.len = sizeof(SSL3Hashes);
	break;
    case dsaKey:
    case fortezzaKey:
	hashItem.data = (unsigned char *)hash->sha;
	hashItem.len = sizeof(hash->sha);
	break;
    default:
    	SECKEY_DestroyPublicKey(key);
	PORT_SetError(SEC_ERROR_BAD_KEY);
	return SECFailure;
    }
    rv = PK11_Verify(key, buf, &hashItem, wincx);
    SECKEY_DestroyPublicKey(key);
    if (rv != SECSuccess) return SECFailure;

    return SECSuccess;
}

#ifdef notdef
static int
ssl_TmpEnableGroup(unsigned long *suites, int *count,int which,int on) {
    int i;
    long mask;
    SSL3KeyExchangeAlgorithm kea;
    const SSL3CipherSuiteDef *suite_def;

    PORT_Assert(cipherSuiteCount <= 32);
    for(i = 0; i < cipherSuiteCount; i++) {
	mask = 1L << i;
	spec = ssl_LookupCipherSuiteDef(i);
	if (spec == NULL)
	    return SECFailure;
	kea = suite_def->key_exchange_algorithm;
	if (((kea == kea_rsa) && (which & SSL_GroupRSA)) ||
	   ((kea == kea_dh) && (which & SSL_GroupDiffieHellman)) ||
	       ((kea == kea_fortezza) && (which & SSL_GroupFortezza))) {
	    if (on) {
		if (((*suites) & mask) == 0) {
		    (*count)++;
		    (*suites) |= mask;
		}
	    } else {
		if (((*suites) & mask) != 0) {
		    (*count)--;
		    (*suites) &= ~mask;
		}
	    }
	    return SECSuccess;
	}
    }
    return SECFailure;
}
#endif

static SECStatus
ssl3_ComputeExportRSAKeyHash(SECItem modulus, SECItem publicExponent,
			     SSL3Random *client_rand, SSL3Random *server_rand,
			     SSL3Hashes *hashes)
{
    MD5Context *md5 = NULL;
    SHA1Context *sha = NULL;
    uint8 modulus_length[2];
    uint8 exponent_length[2];
    unsigned int outLen;
    SECStatus rv = SECSuccess;

    /*
     * OK, we really should use PKCS #11 for the whole thing, but
     * we do screwy things here, like statically allocate the SHA1 and
     * MD5 contexts, so we just make sure it's safe before we call
     * the sha/md5 stuff....
     */
    if (!PK11_HashOK(SEC_OID_MD5)) {
	rv = SECFailure;
        goto loser;
    }
    if (!PK11_HashOK(SEC_OID_SHA1)) {
	rv = SECFailure;
        goto loser;
    }

    md5 = MD5_NewContext();
    if (md5 == NULL) {
	rv = SECFailure;
	goto loser;
    }
    sha = SHA1_NewContext();
    if (sha == NULL) {
	rv = SECFailure;
	goto loser;
    }
    modulus_length[0] = (modulus.len >> 8) & 0xff;
    modulus_length[1] = (modulus.len) & 0xff;
    exponent_length[0] = (publicExponent.len >> 8) & 0xff;
    exponent_length[1] = (publicExponent.len) & 0xff;
    MD5_Begin(md5);
    SHA1_Begin(sha);
    MD5_Update(md5, (unsigned char *)client_rand, SSL3_RANDOM_LENGTH);
    SHA1_Update(sha, (unsigned char *)client_rand, SSL3_RANDOM_LENGTH);
    MD5_Update(md5, (unsigned char *)server_rand, SSL3_RANDOM_LENGTH);
    SHA1_Update(sha, (unsigned char *)server_rand, SSL3_RANDOM_LENGTH);
    MD5_Update(md5, modulus_length, 2);
    SHA1_Update(sha, modulus_length, 2);
    MD5_Update(md5, modulus.data, modulus.len);
    SHA1_Update(sha, modulus.data, modulus.len);
    MD5_Update(md5, exponent_length, 2);
    SHA1_Update(sha, exponent_length, 2);
    MD5_Update(md5, publicExponent.data, publicExponent.len);
    SHA1_Update(sha, publicExponent.data, publicExponent.len);
    MD5_End(md5, hashes->md5, &outLen, MD5_LENGTH);
    PORT_Assert(outLen == MD5_LENGTH);
    SHA1_End(sha, hashes->sha, &outLen, SHA1_LENGTH);
    PORT_Assert(outLen == SHA1_LENGTH);
loser:
    if (md5 != NULL) MD5_DestroyContext(md5, PR_TRUE);
    if (sha != NULL) SHA1_DestroyContext(sha, PR_TRUE);
    return rv;
}

static void
ssl3_BumpSequenceNumber(SSL3SequenceNumber *num)
{
    num->low++;
    if (num->low == 0)
	num->high++;
}


static void
ssl3_DestroyCipherSpec(SSL3CipherSpec *spec) {
    if (spec->destroy) {
	spec->destroy(spec->encodeContext,PR_TRUE);
	spec->destroy(spec->decodeContext,PR_TRUE);
	spec->encodeContext = NULL; /* paranoia */
	spec->decodeContext = NULL;
    }
    if (spec->hashContext != NULL) {
	spec->hash->destroy(spec->hashContext, PR_TRUE);
	spec->hashContext = NULL;
    }
    spec->destroy=NULL;
}

static SECStatus
ssl3_SetupPendingCipherSpec(SSLSocket *ss, SSL3State *ssl3)
{
    SSL3CipherSpec *spec = ssl3->pending_write;
    SSL3CipherSuite suite = ssl3->hs.cipher_suite;
    SSLSecurityInfo *sec = ss->sec;
    MACAlgorithm mac;
    SSL3BulkCipher cipher;
    SSL3KeyExchangeAlgorithm kea;
    const SSL3CipherSuiteDef *suite_def;

    PORT_Assert(ssl3->pending_write == ssl3->pending_read);

    suite_def = ssl_LookupCipherSuiteDef(suite);
    if (suite_def == NULL)
	return SECFailure;

    cipher = suite_def->bulk_cipher_algorithm;
    mac = suite_def->mac_algorithm;
    kea = suite_def->key_exchange_algorithm;

    ssl3->hs.suite_def = suite_def;
    ssl3->hs.kea_def = &kea_defs[kea];
    PORT_Assert(ssl3->hs.kea_def->kea == kea);
    spec->cipher_def = &bulk_cipher_defs[cipher];
    PORT_Assert(spec->cipher_def->cipher == cipher);
    spec->mac_def = &mac_defs[mac];
    PORT_Assert(spec->mac_def->alg == mac);


    sec->keyBits = spec->cipher_def->key_size * 8;
    sec->secretKeyBits = spec->cipher_def->secret_key_size * 8;
    sec->cipherType = cipher;

    /* XXX We should delete the old contexts first */
    /* No.. there shouldn't be any old contexts. SSL3spec does not
     * get reused. */
    spec->encodeContext = NULL;
    spec->decodeContext = NULL;
    
    switch (mac) {
    case mac_null:
	spec->hash = &SECHashObjects[HASH_AlgNULL];
	break;
    case mac_md5:
	spec->hash = &SECHashObjects[HASH_AlgMD5];
	break;
    case mac_sha:
	spec->hash = &SECHashObjects[HASH_AlgSHA1];
	break;
    default:
	PORT_Assert(0);		/* impossible mac algorithm */
    }

    spec->mac_size = spec->hash->length;

    return SECSuccess;
}

static SECStatus
InitPendingCipherSpec(SSLSocket *ss, SSL3State *ssl3, const SECItem *pms)
{
    SSLSecurityInfo *sec = ss->sec;
    SSL3CipherSpec *spec = ssl3->pending_write;
    const SSL3BulkCipherDef *cipher_def = spec->cipher_def;
    CK_MECHANISM_TYPE mechanism;
    PK11SlotInfo *slot = NULL;
    SECItem iv,key;
    SECItem *param;
    PK11Context *serverContext = NULL, *clientContext = NULL;
    int rv;

    rv = ssl3_GenerateSessionKeys(ss, ss->ssl3->pending_write, pms);
    if (rv < 0) return SECFailure;
    
    if (cipher_def->alg == calg_null) {
	spec->encode = Null_Cipher;
	spec->decode = Null_Cipher;
        spec->destroy = NULL;
	return SECSuccess;
    }

    mechanism = (CK_MECHANISM_TYPE) cipher_def->alg;

    slot = PK11_GetBestSlot(mechanism,ss->sec->getClientAuthDataArg);
    if (slot == NULL) return SECFailure;

    /* build the server context */
    iv.data = spec->server.write_iv;
    iv.len = cipher_def->iv_size;
    key.data = spec->server.write_key;
    key.len = cipher_def->key_size;
    param = PK11_ParamFromIV(mechanism,&iv);
    if (param == NULL) goto fail;
    serverContext = PK11_CreateContextByRawKey(slot,mechanism,
		(sec->isServer ? CKA_ENCRYPT : CKA_DECRYPT),
				   &key,param,ss->sec->getClientAuthDataArg);
    SECITEM_FreeItem(param,PR_TRUE);
    if (serverContext == NULL) goto fail;

    /* build the client context */
    iv.data = spec->client.write_iv;
    key.data = spec->client.write_key;;
    param = PK11_ParamFromIV(mechanism,&iv);
    if (param == NULL) goto fail;
    clientContext = PK11_CreateContextByRawKey(slot,mechanism,
		(sec->isServer ? CKA_DECRYPT : CKA_ENCRYPT),
				   &key,param,ss->sec->getClientAuthDataArg);
    SECITEM_FreeItem(param,PR_TRUE);
    if (clientContext == NULL) goto fail;

    PK11_FreeSlot(slot);

    spec->encodeContext = (sec->isServer) ? serverContext : clientContext;
    spec->decodeContext = (sec->isServer) ? clientContext : serverContext;
    spec->encode = (SSLCipher) PK11_CipherOp;
    spec->decode = (SSLCipher) PK11_CipherOp;
    spec->destroy = (SSLDestroy) PK11_DestroyContext;


    spec->hashContext = (*spec->hash->create)();
    if (spec->hashContext == NULL)  goto fail;
    return SECSuccess;

fail:
    if (slot != NULL) PK11_FreeSlot(slot);
    if (serverContext != NULL) PK11_DestroyContext(serverContext,PR_TRUE);
    if (clientContext != NULL) PK11_DestroyContext(clientContext,PR_TRUE);
    return SECFailure;
}

/*
 * 60 bytes is 3 times the maximum length MAC size that is supported.
 */
static unsigned char mac_pad_1 [60] = {
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36
};
static unsigned char mac_pad_2 [60] = {
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c
};

static void
SSL3_ComputeFragmentHash(
    SSL3CipherSpec *spec, SSL3Opaque *mac_secret, SSL3ContentType type,
    SSL3SequenceNumber seq_num, SSL3Opaque *input, int inputLength,
    unsigned char *outbuf, unsigned int *outLength)
{
    unsigned char temp[MAX_MAC_LENGTH];
    const SSL3MACDef *mac_def = spec->mac_def;

    spec->hash->begin(spec->hashContext);
    spec->hash->update(spec->hashContext, mac_secret, spec->mac_size);
    spec->hash->update(spec->hashContext, mac_pad_1, mac_def->pad_size);

    temp[0] = (unsigned char)(seq_num.high >> 24);
    temp[1] = (unsigned char)(seq_num.high >> 16);
    temp[2] = (unsigned char)(seq_num.high >>  8);
    temp[3] = (unsigned char)(seq_num.high >>  0);
    temp[4] = (unsigned char)(seq_num.low  >> 24);
    temp[5] = (unsigned char)(seq_num.low  >> 16);
    temp[6] = (unsigned char)(seq_num.low  >>  8);
    temp[7] = (unsigned char)(seq_num.low  >>  0);
    temp[8] = type;
    temp[9] = (inputLength >> 8) & 0xff;
    temp[10] = inputLength & 0xff;

    PRINT_BUF(95, (NULL, "COMPUTE FRAG HASH TEMP", temp, 11));
    PRINT_BUF(95, (NULL, "COMPUTE FRAG INPUT", input, inputLength));
    PRINT_BUF(95, (NULL, "COMPUTE FRAG SECRET", mac_secret, spec->mac_size));

    spec->hash->update(spec->hashContext, temp, 11);
    spec->hash->update(spec->hashContext, input, inputLength);
    spec->hash->end(spec->hashContext, temp, outLength, spec->hash->length);
    PORT_Assert(*outLength == spec->hash->length);

    PRINT_BUF(95, (NULL, "COMPUTE FRAG HASH TEMP", temp, *outLength));

    spec->hash->begin(spec->hashContext);
    spec->hash->update(spec->hashContext, mac_secret, spec->mac_size);
    spec->hash->update(spec->hashContext, mac_pad_2, mac_def->pad_size);
    spec->hash->update(spec->hashContext, temp, *outLength);
    spec->hash->end(spec->hashContext, outbuf, outLength, spec->hash->length);

    PRINT_BUF(95, (NULL, "COMPUTE FRAG HASH OUT", outbuf, *outLength));

    PORT_Assert(*outLength == spec->hash->length);
}

/*
 * process the plain text before sending it
 */
static SECStatus
ssl3_SendPlainText(SSLSocket *ss, SSL3ContentType type,
		   const SSL3Opaque *buf, int bytes, int flags)
{
    int rv;
    unsigned int hashBytes, cipherBytes, fragLen;
    SSL3CipherSpec *spec;
    SSLBuffer *write = &ss->sec->writeBuf;
    const SSL3BulkCipherDef *cipher_def;

    SSL_TRC(3, ("%d: SSL3[%d] SendPlainText type=%d bytes=%d",
		SSL_GETPID(), ss->fd, type, bytes));

    if (ss->ssl3 == NULL) {
	rv = ssl3_InitState(ss);
	if (rv == SECFailure) return rv;
    }
    spec = ss->ssl3->current_write;
    cipher_def = spec->cipher_def;

    while (bytes > 0) {
	if (bytes > MAX_FRAGMENT_LENGTH)
	    fragLen = MAX_FRAGMENT_LENGTH;
	else
	    fragLen = bytes;
	if (write->space < fragLen + SSL3_BUFFER_FUDGE) {
	    rv = ssl_GrowBuf(write, fragLen + SSL3_BUFFER_FUDGE);
	    if (rv < 0) {
		SSL_DBG(("%d: SSL3[%d]: SendPlainText, tried to get %d bytes",
			 SSL_GETPID(), ss->fd, fragLen + SSL3_BUFFER_FUDGE));
		return SECFailure;
	    }
	}

	/*
	 * null compression is easy to do
	 */
	PORT_Memcpy(write->buf+SSL3_RECORD_HEADER_LENGTH, buf, fragLen);
	buf += fragLen;
	bytes -= fragLen;

	/*
	 * Add the MAC
	 */
	SSL3_ComputeFragmentHash(
	    spec, (ss->sec->isServer) ?
	        spec->server.write_mac_secret : spec->client.write_mac_secret,
	    type, spec->write_seq_num, write->buf + SSL3_RECORD_HEADER_LENGTH,
	    fragLen,  write->buf + fragLen + SSL3_RECORD_HEADER_LENGTH,
	    &hashBytes);
	fragLen += hashBytes;	/* needs to be encrypted */
 
	/*
	 * Encrypt it
	 */
	if (cipher_def->type == type_block) {
	    int padding;
	    /* Assume blockSize is a power of two */
	    padding = cipher_def->block_size - 1 -
		((fragLen) & (cipher_def->block_size - 1));
	    /* XXX allow other side to see the padding bytes? */
	    fragLen += padding + 1;
	    PORT_Assert((fragLen % cipher_def->block_size) == 0);
	    write->buf[fragLen + SSL3_RECORD_HEADER_LENGTH - 1] = padding;
	}
	rv = spec->encode(
	    spec->encodeContext, write->buf + SSL3_RECORD_HEADER_LENGTH,
	    &cipherBytes, fragLen, write->buf + SSL3_RECORD_HEADER_LENGTH,
	    fragLen);
	if (rv < 0) { return rv; }

    /*
     * XXX should we zero out our copy of the buffer after compressing
     * and decryption
     */
	/* PORT_Assert(fragLen == cipherBytes); */
	write->len = cipherBytes + SSL3_RECORD_HEADER_LENGTH;
	write->buf[0] = type;
	write->buf[1] = MSB(SSL_LIBRARY_VERSION_3_0);
	write->buf[2] = LSB(SSL_LIBRARY_VERSION_3_0);
	write->buf[3] = MSB(cipherBytes);
	write->buf[4] = LSB(cipherBytes);

	PRINT_BUF(50, (ss, "send record data:", write->buf, write->len));

	if (ss->pendingBuf.len > 0)
	    rv = ssl_SendSavedWriteData(ss, &ss->pendingBuf, ssl_DefSend);
	if (ss->pendingBuf.len == 0)
	    rv = ssl_DefSend(ss, write->buf, write->len, flags);
	if(rv < 0) {
	    if (PORT_GetError() == XP_ERRNO_EWOULDBLOCK) {
		rv = ssl_SaveWriteData(ss, &ss->pendingBuf, write->buf,
				       write->len);
	    } else {
		return rv;
	    }
	} else if (rv < write->len) {
	    rv = ssl_SaveWriteData(ss, &ss->pendingBuf, write->buf + rv,
				   write->len - rv);
	}
	write->len = 0;
	ssl3_BumpSequenceNumber(&spec->write_seq_num);
    }
    return SECSuccess;
}

int
ssl3_SendApplicationData(SSLSocket *ss, const void *in, int len, int flags)
{
    SECStatus rv;
    rv = ssl3_SendPlainText(ss, content_application_data, in, len, flags);
    if (rv < 0)
	return rv;
    return len;
}

static SECStatus
ssl3_FlushHandshake(SSLSocket *ss)
{
    int rv;
    SSLConnectInfo *ci;

    PORT_Assert(ss->sec != NULL);
    ci = ss->sec->ci;

    rv = ssl3_SendPlainText(ss, content_handshake, ci->sendBuf.buf,
			    ci->sendBuf.len, 0);
    ci->sendBuf.len = 0;
    return rv;
}

/*
 * Alerts
 */

SECStatus
SSL3_SendAlert(
    SSLSocket *ss, SSL3AlertLevel level, SSL3AlertDescription desc)
{
    uint8 bytes[2];
    int rv;

    SSL_TRC(3, ("%d: SSL3[%d]: send alert level=%d desc=%d",
		SSL_GETPID(), ss->fd, level, desc));

    bytes[0] = level;
    bytes[1] = desc;
    if (level == alert_fatal) {
	if (ss->sec->ci->sid) {
	    ss->sec->uncache(ss->sec->ci->sid);
	}
    }
    rv = ssl3_FlushHandshake(ss);
    if (rv < 0)	return rv;
    rv = ssl3_SendPlainText(ss, content_alert, bytes, 2, 0);
    return rv;
}

static SECStatus
ssl3_HandshakeFailure(SSLSocket *ss)
{
    (void)SSL3_SendAlert(ss, alert_fatal, handshake_failure);
    PORT_SetError(
	ss->sec->isServer ? SSL_ERROR_BAD_SERVER : SSL_ERROR_BAD_CLIENT);
    return SECFailure;
}

SECStatus
SSL3_HandleAlert(SSLSocket *ss, SSLBuffer *buf)
{
    SSL3AlertLevel level;
    SSL3AlertDescription desc;
    int error;

    SSL_TRC(3, ("%d: SSL3[%d]: handle alert", SSL_GETPID(), ss->fd));

    if (buf->len != 2)
	return ssl3_HandshakeFailure(ss);
    level = buf->buf[0];
    desc = buf->buf[1];
    buf->len = 0;
    SSL_TRC(5, ("%d: SSL3[%d] received alert, level = %d, description = %d",
        SSL_GETPID(), ss->fd, level, desc));

    error = SEC_ERROR_IO;
    if (desc == bad_record_mac) error = SSL_ERROR_BAD_MAC_ALERT;
    else if ((desc == bad_certificate) ||
	     (desc == unsupported_certificate) ||
	     (desc == certificate_unknown)) error = SSL_ERROR_BAD_CERT_ALERT;
    else if (desc == certificate_revoked) error = SSL_ERROR_REVOKED_CERT_ALERT;
    else if (desc == certificate_expired) error = SSL_ERROR_EXPIRED_CERT_ALERT;
    
    if (level == alert_fatal) {
	ss->sec->uncache(ss->sec->ci->sid);
	if ((ss->ssl3->hs.ws == wait_server_hello) &&
	    (desc == handshake_failure)) {
	    /* XXX This is a hack.  We're assuming that any handshake failure
	     * XXX on the client hello is a failure to match
	     * XXX ciphers.
	     */
	    error = SSL_ERROR_NO_CYPHER_OVERLAP;
	}
	PORT_SetError(error);
	return SECFailure;
    }
    if ((desc == no_certificate) && (ss->ssl3->hs.ws == wait_client_cert)) {
	PORT_Assert(ss->sec->isServer);
	if (ss->sec->peerCert != NULL) {
	    if (ss->sec->peerKey != NULL) {
		SECKEY_DestroyPublicKey(ss->sec->peerKey);
		ss->sec->peerKey = NULL;
	    }
	    CERT_DestroyCertificate(ss->sec->peerCert);
	    ss->sec->peerCert = NULL;
	}
	ss->ssl3->hs.ws = wait_client_key;

	/* XXX If the server has client auth blindly turned on but doesn't
	 * XXX actually look at the certificate it won't know that no
	 * XXX certificate was presented so we close the socket to ensure
	 * XXX an error.  We really shouldn't do this, but there's not a
	 * XXX better way yet.  We only do this if we aren't connected because
	 * XXX if we're redoing the handshake we know the server is paying
	 * XXX attention to the certificate.
	 */
	if (!ss->connected) {
	    ss->sec->uncache(ss->sec->ci->sid);
	    PORT_SetError(SSL_ERROR_NO_CERTIFICATE);
	    SSL3_SendAlert(ss, alert_fatal, bad_certificate);
	    XP_SOCK_CLOSE(ss->fd);
	    return SECFailure;
	}
    }
    return SECSuccess;
}

/*
 * Change Cipher Specs
 */

static SECStatus
ssl3_SendChangeCipherSpecs(SSLSocket *ss)
{
    uint8 change = change_cipher_spec_choice;
    int rv;
    SSL3State *ssl3 = ss->ssl3;
    SSL3CipherSpec *spec;

    SSL_TRC(3, ("%d: SSL3[%d]: send change cipher specs",
		SSL_GETPID(), ss->fd));

    rv = ssl3_FlushHandshake(ss);
    if (rv < 0)	return rv;
    rv = ssl3_SendPlainText(ss, content_change_cipher_spec, &change, 1, 0);
    if (rv < 0) return rv;

    spec = ss->ssl3->pending_write;
    ssl3->pending_write = ssl3->current_write;
    ssl3->current_write = spec;
    spec->write_seq_num.high = spec->write_seq_num.low = 0;

    /* We need to free up the contexts, keys and certs ! */
    /* If we are really through with the old cipher spec 
     * (Both the read and write sides have changed) destroy it.
     */
    if (ss->ssl3->pending_read == ss->ssl3->pending_write) {
    	ssl3_DestroyCipherSpec(ss->ssl3->pending_write);
    }

    return SECSuccess;
}

static SECStatus
SSL3_HandleChangeCipherSpecs(SSLSocket *ss, SSLBuffer *buf)
{
    SSL3CipherSpec *spec;
    SSL3WaitState ws = ss->ssl3->hs.ws;
    int change;
    
    SSL_TRC(3, ("%d: SSL3[%d]: handle change cipher specs",
		SSL_GETPID(), ss->fd));

    if (ws != wait_change_cipher && ws != wait_cert_verify) {
	(void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	return SECFailure;
    }

    if(buf->len != 1) {
	return ssl3_HandshakeFailure(ss);
    }
    change = buf->buf[0];
    if(change != 1)
	return ssl3_HandshakeFailure(ss);
    buf->len = 0;

    spec = ss->ssl3->pending_read;
    ss->ssl3->pending_read = ss->ssl3->current_read;
    ss->ssl3->current_read = spec;
    spec->read_seq_num.high = spec->read_seq_num.low = 0;

    ss->ssl3->hs.ws = wait_finished;

    /* If we are really through with the old cipher spec 
     * (Both the read and write sides have changed) destroy it.
     */
    if (ss->ssl3->pending_read == ss->ssl3->pending_write) {
    	ssl3_DestroyCipherSpec(ss->ssl3->pending_read);
    }
    return SECSuccess;
}

/*
 * Key generation given pre master secret
 */
static char *mixers[] = { "A", "BB", "CCC", "DDDD", "EEEEE", "FFFFFF", "GGGGGGG"};
#define NUM_MIXERS 7

static SECStatus
ssl3_GenerateSessionKeys(SSLSocket *ss, SSL3CipherSpec *spec,
			 const SECItem *pms)
{
    MD5Context *md5 = NULL;
    SHA1Context *sha = NULL;
    int i;
    unsigned int outLen;
    SSL3Opaque sha_out[SHA1_LENGTH];
    SSL3Opaque key_block[NUM_MIXERS * MD5_LENGTH];
    unsigned char * cr = (unsigned char *)&ss->ssl3->hs.client_random;
    unsigned char * sr = (unsigned char *)&ss->ssl3->hs.server_random;
    const SSL3BulkCipherDef *cipher_def = spec->cipher_def;

    /*
     * OK, we really should use PKCS #11 for the whole thing, but
     * we do screwy things here, like statically allocate the SHA1 and
     * MD5 contexts, so we just make sure it's safe before we call
     * the sha/md5 stuff....
     */
    if (!PK11_HashOK(SEC_OID_MD5)) {
        goto loser;
    }
    if (!PK11_HashOK(SEC_OID_SHA1)) {
        goto loser;
    }

    md5 = MD5_NewContext();
    if (md5 == NULL) goto loser;
    sha = SHA1_NewContext();
    if (sha == NULL) goto loser;

    if ((pms != NULL) && (pms->data != NULL)) {
	/*
	 * generate the master secret if we only have the pre master secret
	 */
	for (i = 0; i < 3; i++) {
	    SHA1_Begin(sha);
	    SHA1_Update(sha, (unsigned char*) mixers[i], strlen(mixers[i]));
	    SHA1_Update(sha, pms->data, pms->len);
	    SHA1_Update(sha, cr, SSL3_RANDOM_LENGTH);
	    SHA1_Update(sha, sr, SSL3_RANDOM_LENGTH);
	    SHA1_End(sha, sha_out, &outLen, SHA1_LENGTH);
	    PORT_Assert(outLen == SHA1_LENGTH);
	    MD5_Begin(md5);
	    MD5_Update(md5, pms->data, pms->len);
	    MD5_Update(md5, sha_out, outLen);
	    MD5_End(md5, &spec->master_secret[i*MD5_LENGTH], &outLen,
		    MD5_LENGTH);
	    PORT_Assert(outLen == MD5_LENGTH);
	}
	PRINT_BUF(60, (ss, "master secret:", &spec->master_secret[0], 48));

#ifndef FORTEZZA
	/* get rid of the pms info before it gets swapped out */
	/* Well not really... we still need to encrypt it on the fortezza
	 * side!!! */
	PORT_Memset(pms->data, 0, pms->len);
#endif
    }


    /*
     * generate the key material
     */
    for (i = 0; i < NUM_MIXERS; i++) {
	SHA1_Begin(sha);
	SHA1_Update(sha, (unsigned char*) mixers[i], strlen(mixers[i]));
	SHA1_Update(sha, spec->master_secret, sizeof(SSL3MasterSecret));
	SHA1_Update(sha, sr, SSL3_RANDOM_LENGTH);
	SHA1_Update(sha, cr, SSL3_RANDOM_LENGTH);
	SHA1_End(sha, sha_out, &outLen, SHA1_LENGTH);
	PORT_Assert(outLen == SHA1_LENGTH);
	MD5_Begin(md5);
	MD5_Update(md5, spec->master_secret, sizeof(SSL3MasterSecret));
	MD5_Update(md5, sha_out, outLen);
	MD5_End(md5, &key_block[i*MD5_LENGTH], &outLen, MD5_LENGTH);
	PORT_Assert(outLen == MD5_LENGTH);
    }

    PRINT_BUF(60, (ss, "key block:", &key_block[0], NUM_MIXERS * MD5_LENGTH));

    /*
     * put the key material where it goes
     */
    i = 0;			/* now shows how much consumed */
    PORT_Memcpy(spec->client.write_mac_secret, &key_block[i], spec->mac_size);
    i += spec->mac_size;
    PORT_Memcpy(spec->server.write_mac_secret, &key_block[i], spec->mac_size);
    i += spec->mac_size;

#ifdef FORTEZZA
    /* we just need the pre-master secret. & mac Fortezza generates it's own
     * keys.
     */
    if (spec->bulk_cipher_algorithm == cipher_fortezza) {
	return SECSuccess;
    }
#endif

    if (cipher_def->keygen_mode == kg_strong) {
	PORT_Memcpy(spec->client.write_key, &key_block[i],
		    cipher_def->key_size);
	i += cipher_def->key_size;
	PORT_Memcpy(spec->server.write_key, &key_block[i],
		    cipher_def->key_size);
	i += cipher_def->key_size;
	PORT_Memcpy(spec->client.write_iv, &key_block[i], cipher_def->iv_size);
	i += cipher_def->iv_size;
	PORT_Memcpy(spec->server.write_iv, &key_block[i], cipher_def->iv_size);
	i += cipher_def->iv_size;
    } else {
	MD5_Begin(md5);
	MD5_Update(md5, &key_block[i], cipher_def->secret_key_size);
	MD5_Update(md5, cr, SSL3_RANDOM_LENGTH);
	MD5_Update(md5, sr, SSL3_RANDOM_LENGTH);
	MD5_End(md5, spec->client.write_key, &outLen, MD5_LENGTH);
	i += cipher_def->secret_key_size;
	MD5_Begin(md5);
	MD5_Update(md5, &key_block[i], cipher_def->secret_key_size);
	MD5_Update(md5, sr, SSL3_RANDOM_LENGTH);
	MD5_Update(md5, cr, SSL3_RANDOM_LENGTH);
	MD5_End(md5, spec->server.write_key, &outLen, MD5_LENGTH);
	i += cipher_def->secret_key_size;
	MD5_Begin(md5);
	MD5_Update(md5, cr, SSL3_RANDOM_LENGTH);
	MD5_Update(md5, sr, SSL3_RANDOM_LENGTH);
	MD5_End(md5, spec->client.write_iv, &outLen, MD5_LENGTH);
	MD5_Begin(md5);
	MD5_Update(md5, sr, SSL3_RANDOM_LENGTH);
	MD5_Update(md5, cr, SSL3_RANDOM_LENGTH);
	MD5_End(md5, spec->server.write_iv, &outLen, MD5_LENGTH);
    }

    MD5_DestroyContext(md5, PR_TRUE);
    SHA1_DestroyContext(sha, PR_TRUE);

    PRINT_BUF(60, (ss, "client write mac secret:",
		   spec->client.write_mac_secret, spec->mac_size));
    PRINT_BUF(60, (ss, "server write mac secret:",
		   spec->server.write_mac_secret, spec->mac_size));
    PRINT_BUF(60, (ss, "client write key:", spec->client.write_key,
		   cipher_def->key_size));
    PRINT_BUF(60, (ss, "server write key:", spec->server.write_key,
		   cipher_def->key_size));
    PRINT_BUF(60, (ss, "client write iv:", spec->client.write_iv,
		   cipher_def->iv_size));
    PRINT_BUF(60, (ss, "server write iv:", spec->server.write_iv,
		   cipher_def->iv_size));

    return SECSuccess;

loser:
    if (md5 != NULL) MD5_DestroyContext(md5, PR_TRUE);
    if (sha != NULL) SHA1_DestroyContext(sha, PR_TRUE);
    return SECFailure;
}

/*
 * Handshake messages
 */
static void
SSL3_UpdateHandshakeHashes(SSL3State *ssl3, unsigned char *b, unsigned int l)
{
    PRINT_BUF(90, (NULL, "XXX handshake hash input:", b, l));

    MD5_Update(ssl3->hs.md5, b, l);
    SHA1_Update(ssl3->hs.sha, b, l);
}

static SECStatus
ssl3_AppendHandshake(SSLSocket *ss, const void *void_src, int bytes)
{
    SSLSecurityInfo *sec = ss->sec;
    SSLConnectInfo *ci = sec->ci;
    unsigned char * src = (unsigned char *)void_src;
    int room;
    SECStatus rv;

    room = ci->sendBuf.space - ci->sendBuf.len;

    if (ci->sendBuf.space < MAX_SEND_BUF_LENGTH && room < bytes) {
	ssl_GrowBuf(
	    &ci->sendBuf,
#ifndef NSPR20
	    MAX(MIN_SEND_BUF_LENGTH,
		MIN(MAX_SEND_BUF_LENGTH, ci->sendBuf.len + bytes)));
#else
	    PR_MAX(MIN_SEND_BUF_LENGTH,
		   PR_MIN(MAX_SEND_BUF_LENGTH, ci->sendBuf.len + bytes)));
#endif /* NSPR20 */
	room = ci->sendBuf.space - ci->sendBuf.len;
    }

    PRINT_BUF(60, (ss, "", (unsigned char*)void_src, bytes));
    SSL3_UpdateHandshakeHashes(ss->ssl3, src, bytes);
    while (bytes > room) {
	if (room > 0)
	    PORT_Memcpy(ci->sendBuf.buf + ci->sendBuf.len, src, room);
	ci->sendBuf.len += room;
	rv = ssl3_FlushHandshake(ss);
	if (rv < 0) return rv;
	bytes -= room;
	src += room;
	room = ci->sendBuf.space;
	PORT_Assert(ci->sendBuf.len == 0);
    }
    PORT_Memcpy(ci->sendBuf.buf + ci->sendBuf.len, src, bytes);
    ci->sendBuf.len += bytes;
    return SECSuccess;
}

static SECStatus
ssl3_AppendHandshakeNumber(SSLSocket *ss, long num, int lenSize)
{
    uint8 b[4], *p = b;

    switch (lenSize) {
      case 4:
	*p++ = (num >> 24) & 0xff;
      case 3:
	*p++ = (num >> 16) & 0xff;
      case 2:
	*p++ = (num >> 8) & 0xff;
      case 1:
	*p = num & 0xff;
    }
    SSL_TRC(60, ("%d: number:", SSL_GETPID()));
    return ssl3_AppendHandshake(ss, &b[0], lenSize);
}

static SECStatus
ssl3_AppendHandshakeVariable(
    SSLSocket *ss, SSL3Opaque *src, long bytes, int lenSize)
{
    SECStatus rv;

    PORT_Assert((bytes < (1<<8) && lenSize == 1) ||
	      (bytes < (1L<<16) && lenSize == 2) ||
	      (bytes < (1L<<24) && lenSize == 3));

    SSL_TRC(60,("%d: append variable:", SSL_GETPID()));
    if((rv = ssl3_AppendHandshakeNumber(ss, bytes, lenSize)) != SECSuccess) {
	return rv;
    }
    SSL_TRC(60, ("data:"));
    return ssl3_AppendHandshake(ss, src, bytes);
}

static SECStatus
ssl3_AppendHandshakeHeader(SSLSocket *ss, SSL3HandshakeType t, uint32 length)
{
    int rv;

    SSL_TRC(30,("%d: append handshake header: %d", SSL_GETPID(), t));
    PRINT_BUF(60, (ss, "md5 handshake hash", (unsigned char*)ss->ssl3->hs.md5, 16));

    rv = ssl3_AppendHandshakeNumber(ss, t, 1);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshakeNumber(ss, length, 3);
    return rv;
}

static SECStatus
ssl3_ConsumeHandshake(
    SSLSocket *ss, void *v, int bytes, SSL3Opaque **b, long *length)
{
    if (bytes > *length)
	return ssl3_HandshakeFailure(ss);
    PORT_Memcpy(v, *b, bytes);
    PRINT_BUF(60, (ss, "consume bytes:", *b, bytes));
    *b += bytes;
    *length -= bytes;
    return SECSuccess;
}

static long
ssl3_ConsumeHandshakeNumber(
    SSLSocket *ss, int bytes, SSL3Opaque **b, long *length)
{
    long num = 0;
    int i;
    SECStatus status;
    uint8 buf[4];

    status = ssl3_ConsumeHandshake(ss, buf, bytes, b, length);
    if (status < 0)
	return ssl3_HandshakeFailure(ss);
    for (i = 0; i < bytes; i++)
	num = (num << 8) + buf[i];
    return num;
}

static SECStatus
ssl3_ConsumeHandshakeVariable(
    SSLSocket *ss, SECItem *i, int bytes, SSL3Opaque **b, long *length)
{
    long rv;

    PORT_Assert(bytes <= 3);
    i->len = 0;
    i->data = NULL;
    rv = ssl3_ConsumeHandshakeNumber(ss, bytes, b, length);
    if (rv < 0) return rv;
    if (rv > 0) {
	i->data = PORT_Alloc(rv);
	if (i->data == NULL)
	    return ssl3_HandshakeFailure(ss);
	i->len = rv;
	rv = ssl3_ConsumeHandshake(ss, i->data, i->len, b, length);
	if (rv < 0) {
	    PORT_Free(i->data);
	    i->data = NULL;
	    return ssl3_HandshakeFailure(ss);
	}
    }
    return SECSuccess;
}

/* Extract the hashes of handshake messages to this point */
static void
SSL3_ComputeHandshakeHashes(SSL3State *ssl3, SSL3CipherSpec *spec,
			    SSL3Hashes *hashes, uint32 sender)
{
    MD5Context *md5;
    SHA1Context *sha;
    SSL3Opaque md5_inner[MAX_MAC_LENGTH];
    SSL3Opaque sha_inner[MAX_MAC_LENGTH];
    unsigned int outLength;
    unsigned char s[4];

    /*
     * OK, we really should use PKCS #11 for the whole thing, but
     * we do screwy things here, like statically allocate the SHA1 and
     * MD5 contexts, so we just make sure it's safe before we call
     * the sha/md5 stuff....
     */
    if (!PK11_HashOK(SEC_OID_MD5)) {
        return; /* sigh. well we'll fail soon enough */
    }
    if (!PK11_HashOK(SEC_OID_SHA1)) {
        return; /* sigh. well we'll fail soon enough */
    }

    s[0] = (unsigned char)(sender >> 24);
    s[1] = (unsigned char)(sender >> 16);
    s[2] = (unsigned char)(sender >> 8);
    s[3] = (unsigned char)sender;

    PORT_Assert(mac_defs[mac_md5].alg == mac_md5);
    PORT_Assert(mac_defs[mac_sha].alg == mac_sha);

    md5 = MD5_CloneContext(ssl3->hs.md5);
    if (sender != 0) {
	MD5_Update(md5, s, 4);
    }
    MD5_Update(md5, spec->master_secret, sizeof(SSL3MasterSecret));
    MD5_Update(md5, mac_pad_1, mac_defs[mac_md5].pad_size);
    MD5_End(md5, md5_inner, &outLength, MD5_LENGTH);
    PORT_Assert(outLength == MD5_LENGTH);

    sha = SHA1_CloneContext(ssl3->hs.sha);
    if (sender != 0) {
	SHA1_Update(sha, s, 4);
    }
    SHA1_Update(sha, spec->master_secret, sizeof(SSL3MasterSecret));
    SHA1_Update(sha, mac_pad_1, mac_defs[mac_sha].pad_size);
    SHA1_End(sha, sha_inner, &outLength, SHA1_LENGTH);
    PORT_Assert(outLength == SHA1_LENGTH);

    MD5_Begin(md5);
    MD5_Update(md5, spec->master_secret, sizeof(SSL3MasterSecret));
    MD5_Update(md5, mac_pad_2, mac_defs[mac_md5].pad_size);
    MD5_Update(md5, md5_inner, MD5_LENGTH);
    MD5_End(md5, hashes->md5, &outLength, MD5_LENGTH);
    PORT_Assert(outLength == MD5_LENGTH);

    SHA1_Begin(sha);
    SHA1_Update(sha, spec->master_secret, sizeof(SSL3MasterSecret));
    SHA1_Update(sha, mac_pad_2, mac_defs[mac_sha].pad_size);
    SHA1_Update(sha, sha_inner, SHA1_LENGTH);
    SHA1_End(sha, hashes->sha, &outLength, SHA1_LENGTH);
    PORT_Assert(outLength == SHA1_LENGTH);

    MD5_DestroyContext(md5, PR_TRUE);
    SHA1_DestroyContext(sha, PR_TRUE);

    PRINT_BUF(60, (NULL, "handshake hashes", (unsigned char*)hashes,
		   sizeof(SSL3Hashes)));
}

/*
 * SSL 2 based implementations pass in the initial outbound buffer
 * so that the handshake hash can contain the included information
 */
SECStatus
ssl3_StartHandshakeHash(SSLSocket *ss, unsigned char * buf, int length)
{
    SECStatus rv;

    rv = ssl3_InitState(ss);
    if (rv < 0) return rv;

    PORT_Memset(&ss->ssl3->hs.client_random, 0, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(
	&ss->ssl3->hs.client_random.rand[SSL3_RANDOM_LENGTH - SSL_CHALLENGE_BYTES],
	&ss->sec->ci->clientChallenge,
	SSL_CHALLENGE_BYTES);

    SSL3_UpdateHandshakeHashes(ss->ssl3, buf, length);
    /*
     * XXX The fragment sequence number went up, but there is no hash
     * yet, so we are sort of safe!
     */
    return SECSuccess;
}

SECStatus
SSL3_SendClientHello(SSLSocket *ss)
{
    SSL3ProtocolVersion version = SSL_LIBRARY_VERSION_3_0;
    SSLSecurityInfo *sec = ss->sec;
    SSLConnectInfo *ci = sec->ci;
    SSLSessionID *sid;
    SECStatus rv;
    int i, length, num_suites;
#ifdef FORTEZZA
    PRBool use_fortezza = PR_FALSE;
#endif

    SSL_TRC(3, ("%d: SSL3[%d]: send client hello", SSL_GETPID(), ss->fd));

    rv = ssl3_InitState(ss);
    if (rv < 0) return rv;

    /* Initialize the handshake hashes for the new handshake */
    /*
     * OK, we really should use PKCS #11 for the whole thing, but
     * we do screwy things here, like statically allocate the SHA1 and
     * MD5 contexts, so we just make sure it's safe before we call
     * the sha/md5 stuff....
     */
    if (!PK11_HashOK(SEC_OID_MD5)) {
        return SECFailure;
    }
    if (!PK11_HashOK(SEC_OID_SHA1)) {
        return SECFailure;
    }
    MD5_Begin(ss->ssl3->hs.md5);
    SHA1_Begin(ss->ssl3->hs.sha);

    PORT_Assert(sec && ci);

#ifdef FORTEZZA
    if (SSL_IsEnabledGroup(SSL_GroupFortezza)) {
	rv=FortezzaOpen(ss,(FortezzaKEAContext **)&ss->ssl3->kea_context);
	switch (rv) {
	case SECSuccess:
	    use_fortezza = PR_TRUE;
	    break;
	case SECFailure:
	    ssl_TmpEnableGroup(&localCipherSuites,&localCipherSuiteCount,
						SSL_GroupFortezza,PR_FALSE);
	    break;
	default:
	    ss->reStartType = 3;
	    return (rv); /* SECWouldBlock */
	}
    }
#endif

    if (ss->noCache) {
	sid = NULL;
    } else {
	sid = ssl_LookupSID(ci->peer, ci->port, ss->peerID);
    }

#ifdef FORTEZZA
    /* We can't resume based on a different card in Fortezza. If the sid
     * hasFortezza then it's most likely we will resume with fortezza (if
     * the sid doesn't have hasFortezza then the server won't resume fortezza
     * even if that's the protocol it chooses.
     * So if we try to resume on a different card, toss this sid and 
     *  renegotiate  */
    if (sid->u.ssl3.hasFortezza && ss->ssl3->kea_context) {
	FortezzaKEAContext *kea_context = 
			(FortezzaKEAContext *)ss->ssl3->kea_context;
	if (sid->u.ssl3.fortezzaSocket != kea_context->socketId) {
	    sid->u.ssl3.resumable = PR_FALSE;
	    (*ss->sec->uncache)(sid);
	    ssl_FreeSID(sid);
	    sid = NULL;
	}
    }
#endif

    if (sid) {
	PRINT_BUF(4, (ss, "client, found session-id:", sid->u.ssl3.sessionID,
		      sid->u.ssl3.sessionIDLength));
	ss->version = sid->version;
	ss->ssl3->policy = sid->u.ssl3.policy;
    } else {
	sid = ssl3_NewSessionID(ss, PR_FALSE);
	if (!sid) return SECFailure;
    }

    if (ci->sid != NULL) ssl_FreeSID(ci->sid);
    ci->sid = sid;
    sec->send = ssl3_SendApplicationData;

    num_suites = count_cipher_suites(ss->ssl3->policy, PR_TRUE);

    length = sizeof(SSL3ProtocolVersion) + SSL3_RANDOM_LENGTH +
	1 + ((sid == NULL) ? 0 : sid->u.ssl3.sessionIDLength) +
	2 + num_suites*sizeof(SSL3CipherSuite) +
	1 + compressionMethodsCount;

    rv = ssl3_AppendHandshakeHeader(ss, client_hello, length);
    if (rv < 0) return rv;

    rv = ssl3_AppendHandshakeNumber(ss, version, 2);
    if (rv < 0) return rv;
    rv = ssl3_GetNewRandom(&ss->ssl3->hs.client_random);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshake(
	ss, &ss->ssl3->hs.client_random, SSL3_RANDOM_LENGTH);
    if (rv < 0) return rv;
   
    if (sid)
	rv = ssl3_AppendHandshakeVariable(
	    ss, sid->u.ssl3.sessionID, sid->u.ssl3.sessionIDLength, 1);
    else
	rv = ssl3_AppendHandshakeVariable(ss, NULL, 0, 1);
    if (rv < 0) return rv;

    rv = ssl3_AppendHandshakeNumber(
	ss, num_suites*sizeof(SSL3CipherSuite), 2);
    if (rv < 0) return rv;
    for (i = 0; i < cipherSuiteCount; i++) {
	if (config_match(&cipherSuites[i], ss->ssl3->policy, PR_TRUE)) {
	    rv = ssl3_AppendHandshakeNumber(ss, cipherSuites[i].cipher_suite,
					    sizeof(SSL3CipherSuite));
	if (rv < 0) return rv;
	}
    }

    /* packing of compressions in array is unspecified */
    rv = ssl3_AppendHandshakeNumber(ss, compressionMethodsCount, 1);
    if (rv < 0) return rv;
    for (i = 0; i < compressionMethodsCount; i++) {
	rv = ssl3_AppendHandshakeNumber(ss, compressions[i], 1);
	if (rv < 0) return rv;
    }

    rv = ssl3_FlushHandshake(ss);
    if (rv < 0) return rv;

    ss->ssl3->hs.ws = wait_server_hello;
    return rv;
}

SECStatus
SSL3_HandleHelloRequest(SSLSocket *ss)
{
    SSLSessionID *sid = ss->sec->ci->sid;
    
    SSL_TRC(3, ("%d: SSL3[%d]: handle hello request", SSL_GETPID(), ss->fd));

    PORT_Assert(ss->ssl3);
    if (ss->ssl3->hs.ws == wait_server_hello)
	return SECSuccess;
    if (ss->ssl3->hs.ws != idle_handshake || ss->sec->isServer)
	return ssl3_HandshakeFailure(ss);
    if (sid) {
	ss->sec->uncache(sid);
	ssl_FreeSID(sid);
	ss->sec->ci->sid = NULL;
    }
    return SSL3_SendClientHello(ss);
}

SECStatus
SSL3_SendClientKeyExchange(SSLSocket *ss)
{
    SSL3RSAPreMasterSecret rsa_pms;
#ifdef FORTEZZA
    FortezzaKey		*tek;
    FortezzaKEAContext 	*kea_context;
    FortezzaContext 	*write_context,*read_context;
    FortezzaContext 	*tmp_write=NULL,*tmp_read=NULL;
    SSL3FortezzaKeys	fortezza_pms;
    int 	newpers;
    SSLSessionID *sid = ss->sec->ci->sid;
#endif
    CERTCertificate *cert = 0;
    SECKEYPublicKey *serverKey = 0;
    SECItem pms;
    SECItem enc_pms;
    SECStatus rv = SECFailure;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];
    PK11SymKey *symKey;
    PK11SlotInfo *slot;
    void *wincx;

    SSL_TRC(3, ("%d: SSL3[%d]: send client key exchange",
		SSL_GETPID(), ss->fd));

    cert = ss->sec->peerCert;
    if (ss->sec->peerKey == NULL)
	serverKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    else
	serverKey = ss->sec->peerKey;
    if (serverKey == NULL)
	return SECFailure;

    wincx = ss->sec->getClientAuthDataArg;
   
    /* 
     * in the future we need to do this whole Key exchange/Gen in the token
     *  thing here.
     */
    switch (ss->ssl3->hs.kea_def->alg) {
    case kt_rsa:
	rsa_pms.client_version[0] = MSB(SSL_LIBRARY_VERSION_3_0);
	rsa_pms.client_version[1] = LSB(SSL_LIBRARY_VERSION_3_0);
	rv = PK11_GenerateRandom(&rsa_pms.random[0], sizeof(rsa_pms.random));
	if (rv != SECSuccess) break;
	rv = SECFailure;

	pms.data = (unsigned char *)&rsa_pms;
	pms.len = sizeof(rsa_pms);
	enc_pms.data = buffer;
	enc_pms.len = sizeof(buffer);

	slot = PK11_GetBestSlot(CKM_RSA_PKCS,wincx);
	if (slot == NULL) break;
        symKey = PK11_ImportSymKey(slot,CKM_SSL3_PRE_MASTER_KEY_GEN,
						CKA_DERIVE,&pms,wincx);
	PK11_FreeSlot(slot);
	if (symKey == NULL) break;
	
	rv = PK11_PubWrapSymKey(CKM_RSA_PKCS,serverKey,symKey,&enc_pms);
	PK11_FreeSymKey(symKey);
	if (rv != SECSuccess) break;

	rv = InitPendingCipherSpec(ss, ss->ssl3, &pms);
	if (rv != SECSuccess) break;
	/*
	 * PMS must be cleared before we do anything else so that
	 * it probably will not get swapped to disk.
	 */
	PORT_Memset(&rsa_pms, 0, sizeof(rsa_pms));

	rv = ssl3_AppendHandshakeHeader(ss, client_key_exchange, enc_pms.len);
	if (rv != SECSuccess) break;
	rv = ssl3_AppendHandshake(ss, enc_pms.data, enc_pms.len);
	if (rv != SECSuccess) break;

	rv = SECSuccess;
	break;
#ifdef FORTEZZA
    case kt_fortezza:
	if (tek == NULL) goto fortezzaLoser;

	if (slot == NULL) break;

	/* if we don't have a certificate, we need to read out your public
	 * key. This changes for a bit when we need to deal with the
	 * PQG stuff */
	PORT_Memset(fortezza_pms.y_signature, 0,
					sizeof(fortezza_pms.y_signature));
	if (ss->ssl3->clientCertificate == NULL) {
	    pubkey = CERT_ExtractPublicKey(&ccert->subjectPublicKeyInfo);
	    if (pubkey == NULL) goto fortezza_oser;
	    PORT_Assert(pubkey->keyType == fortezzaKeys); /* paranoia */
	    /* send the protocol */
	    rv = ssl3_AppendHandshakeHeader(ss, client_key_exchange,
			 (sizeof(fortezza_pms)-sizeof(fortezza_pms.y_c)) 
				+ 1 + pubkey->fortezzaKeyData.KEAKey.len);
	    rv |= ssl3_AppendHandshakeVariable(ss, 
			pubkey->fortezzaKeyData.KEAKey.data, 
					pubkey->fortezzaKeyData.KEAKey.len,1);
	    SECKEY_DestroyPublicKey(pubkey);
	    if (rv < 0) goto fortezza_oser;
	} else {
		rv = ssl3_AppendHandshakeHeader(ss, client_key_exchange,
			 (sizeof(fortezza_pms)-sizeof(fortezza_pms.y_c)) + 1);
		rv = ssl3_AppendHandshakeVariable(ss,NULL,0,1);
	}

	/* create a pre-Master secret */
	/* We do this now so we can call InitPending Cipher to get our
	 * context's. That way we can properly load them up...
	 */
	pms.data = buffer;
	pms.len = sizeof(fortezza_pms.encrypted_preMasterSecret);
	rv = PK11_GenerateRandom(pms.data,pms.len);
	/* encrypt the pms with the TEK */
	mechanism.type = CKM_FORTEZZA_SKIPJACK;
	mechanism.pParameter = fortezza_pms.master_secret_iv;
	mechanism.usParameterLen = sizeof(fortezza_pms.master_secret_iv);
	rv = PK11_GETTAB(slot)->C_EncryptInit(slot->session,
						&mechanism,tek->objectID);
	enc_pms.data = fortezza_pms.encrpyted_preMasterSecret;
	enc_pms.len = sizeof(fortezza_pms.encrpyted_preMasterSecret);

	rv = InitPendingCipherSpec(ss, ss->ssl3, &pms);
	if (rv < 0) return SECFailure;

	/* copy the keys and IV's out now */
	fortezza_pms.wrapped_client_write_key
	fortezza_pms.wrapped_server_write_key;
	fortezza_pms.client_write_iv
	fortezza_pms.server_write_iv;
		
	if (rv < 0) { goto fortezza_loser; }

	/*
	 * now we initialize out contexts
	 */

	sid->u.ssl3.hasFortezza = PR_TRUE;
	sid->u.ssl3.fortezzaSocket = PK11_ReferenceSlot(slot);
	sid->u.ssl3.tek = PK11_ReferenceSymKey(tek);
	/* what to do about these??? */
	sid->u.ssl3.clientWriteKey = write_context->theKey;
	sid->u.ssl3.clientWriteKey->ref_count++;
	sid->u.ssl3.serverWriteKey = read_context->theKey;
	sid->u.ssl3.serverWriteKey->ref_count++;
	PORT_Memcpy(sid->u.ssl3.clientWriteIV,write_context->iv,
							sizeof(FortezzaIV));
	PORT_Memcpy(sid->u.ssl3.clientWriteSave,write_context->save,
						sizeof(FortezzaCryptSave));
	PORT_Memcpy(sid->u.ssl3.serverWriteIV,read_context->iv,
							sizeof(FortezzaIV));

	if (tmp_read) FortezzaDestroyContext(tmp_read,PR_TRUE);
	if (tmp_write) FortezzaDestroyContext(tmp_write,PR_TRUE);
	return SECSuccess;

fortezza_loser:
	if (tmp_read) FortezzaDestroyContext(tmp_read,PR_TRUE);
	if (tmp_write) FortezzaDestroyContext(tmp_write,PR_TRUE);
	return rv;
#endif
    default:
	break;
    }

    return rv;
}

SECStatus
SSL3_SendCertificateVerify(SSLSocket *ss)
{
    SSL3Hashes hashes;
    SECStatus rv;
    SECItem buf;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];
    SSL3State *ssl3 = ss->ssl3;

    buf.data = buffer;
    buf.len = MAX_RSA_MODULUS_LEN;

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate verify",
		SSL_GETPID(), ss->fd));

    SSL3_ComputeHandshakeHashes(ss->ssl3, ss->ssl3->pending_write,
				    &hashes, 0);
    rv = ssl3_SignHashes(&hashes, ss->ssl3->clientPrivateKey, &buf);
    /*
     * diffie-helman & fortezza need the client key for the key
     * exchange.
     */
    if (ssl3->hs.kea_def->alg == kt_rsa) {
	SECKEY_DestroyPrivateKey(ssl3->clientPrivateKey);
	ssl3->clientPrivateKey = NULL;
    }
    if (rv < 0) return SECFailure;
	
    rv = ssl3_AppendHandshakeHeader(ss, certificate_verify, buf.len + 2);
    if (rv < 0) return SECFailure;
    rv = ssl3_AppendHandshakeVariable(ss, buf.data, buf.len, 2);
    if (rv < 0) return SECFailure;

    return SECSuccess;
}

SECStatus
SSL3_HandleServerHello(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECStatus rv;
    int version, temp;		/* allow for consume number failure */
    SSLSessionID *sid = ss->sec->ci->sid;
    SECItem sidBytes = {siBuffer, NULL, 0};
    int i;
    PRBool can_resume = PR_TRUE;
    
    SSL_TRC(3, ("%d: SSL3[%d]: handle server hello", SSL_GETPID(), ss->fd));

    rv = ssl3_InitState(ss);
    if (rv < 0) goto loser;
    if (ss->ssl3->hs.ws != wait_server_hello) goto loser;

    version = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (version < 0) return version;
    if (version != SSL_LIBRARY_VERSION_3_0) /* only one supported for now */
	goto loser;

    rv = ssl3_ConsumeHandshake(
	ss, &ss->ssl3->hs.server_random, SSL3_RANDOM_LENGTH, &b, &length);
    if (rv < 0) return rv;

    rv = ssl3_ConsumeHandshakeVariable(ss, &sidBytes, 1, &b, &length);
    if (rv < 0) return rv;
    if (sidBytes.len > SSL3_SESSIONID_BYTES) goto loser;

    temp = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    for (i = 0; i < cipherSuiteCount; i++) {
	if ((temp == cipherSuites[i].cipher_suite) &&
	    (config_match(&cipherSuites[i], ss->ssl3->policy, PR_TRUE)))
	    break;
	if (i == cipherSuiteCount - 1)
	    goto loser;
    }
    ss->ssl3->hs.cipher_suite = temp;
    ss->ssl3->hs.suite_def = ssl_LookupCipherSuiteDef(temp);

    temp = ssl3_ConsumeHandshakeNumber(ss, 1, &b, &length);
    for (i = 0; i < compressionMethodsCount; i++) {
	if (temp == compressions[i]) break;
	if (i == compressionMethodsCount - 1)
	    goto loser;
    }
    ss->ssl3->hs.compression = temp;

    if (length != 0)
	goto loser;

    /*
     * we may or may not have sent a session id, we may get one back or
     * not and if so it may match the one we sent
     *  (fortezza is not resumable!)
     */
    can_resume = PR_TRUE;
#ifdef FORTEZZA
    if (ss->ssl3->hs.kea == kea_fortezza) can_resume = sid->u.ssl3.hasFortezza;
#endif
    if ((sidBytes.len > 0) && (sidBytes.len == sid->u.ssl3.sessionIDLength) &&
	(PORT_Memcmp(sid->u.ssl3.sessionID, sidBytes.data, sidBytes.len) == 0)
			&& can_resume) {
	/* Got a Match */
	ss->ssl3->hs.ws = wait_change_cipher;
	ss->ssl3->hs.isResuming = PR_TRUE;
	PORT_Memcpy(ss->ssl3->pending_read->master_secret,
		  sid->u.ssl3.masterSecret,
		  sizeof(ss->ssl3->pending_read->master_secret));
	/* copy the peer cert from the SID */
	if (sid->peerCert != NULL) {
	    ss->sec->peerCert = CERT_DupCertificate(sid->peerCert);
	}
    } else {
	/* throw the old one away, if any */
	sid->u.ssl3.resumable = PR_FALSE;
	(*ss->sec->uncache)(sid);
	ssl_FreeSID(sid);
	ss->sec->ci->sid = sid = ssl3_NewSessionID(ss, PR_FALSE);
	if (sid == NULL) goto loser;
	sid->u.ssl3.sessionIDLength = sidBytes.len;
	if (sidBytes.len > SSL3_SESSIONID_BYTES) goto loser;
	PORT_Memcpy(sid->u.ssl3.sessionID, sidBytes.data, sidBytes.len);
	ss->ssl3->hs.isResuming = PR_FALSE;
#if 0
	/* We should really use a table, or something.  Diffie-Helman XXXX */
	if (MSB(ss->ssl3->hs.cipher_suite) == 0x04) {
	    if (LSB(ss->ssl3->hs.cipher_suite <= 0x10)) {
		ss->ssl3->hs.ws = wait_server_key;
	    } else {
		goto loser;
	    }
	} else {
	    ss->ssl3->hs.ws = wait_server_cert;
	}
#else
	ss->ssl3->hs.ws = wait_server_cert;
#endif
    }
    
    ssl3_SetupPendingCipherSpec(ss, ss->ssl3);
    if (ss->ssl3->hs.isResuming == PR_TRUE) {
	rv = InitPendingCipherSpec(ss, ss->ssl3, NULL);
	if (rv < 0) goto loser;
#ifdef FORTEZZA
	if (ss->ssl3->hs.kea == kea_fortezza) {
	    FortezzaKEAContext *kea_context = 
				(FortezzaKEAContext *) ss->ssl3->kea_context;
	    FortezzaContext *write_context,*read_context;

	    kea_context->tek = sid->u.ssl3.tek;
	    kea_context->tek->ref_count++;	    
	    if (ss->ssl3->pending_write->bulk_cipher_algorithm == 
							cipher_fortezza) {
		write_context = ss->ssl3->pending_write->encodeContext;
		read_context = ss->ssl3->pending_read->decodeContext;
		if (write_context) {
		    rv = FortezzaSetKey(write_context,
						sid->u.ssl3.clientWriteKey);
		    if (rv < 0) goto loser;
		    rv = FortezzaLoadIV(write_context,
						sid->u.ssl3.clientWriteIV);
		    if (rv < 0) goto loser;
		    rv = FortezzaLoadSave(write_context,
					sid->u.ssl3.clientWriteSave);
		    if (rv < 0) goto loser;
		}
		if (read_context) {
		    rv = FortezzaSetKey(read_context,
						sid->u.ssl3.serverWriteKey);
		    if (rv < 0) goto loser;
		    rv = FortezzaLoadIV(read_context,sid->u.ssl3.serverWriteIV);
		    if (rv < 0) goto loser;
		    /* we need to know we're doing the 8 byte trash Hack */
		    rv = FortezzaSetServerInit(read_context);
		    if (rv < 0) goto loser;
		}
	    }
	}
#endif
    }

    return SECSuccess;
loser:
    if (sidBytes.data != NULL)
	PORT_ZFree(sidBytes.data, sidBytes.len);
    return ssl3_HandshakeFailure(ss);
}

SECStatus
SSL3_HandleServerKeyExchange(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECStatus rv;
    SECItem modulus = {siBuffer, NULL, 0};
    SECItem exponent = {siBuffer, NULL, 0};
    SECItem signature = {siBuffer, NULL, 0};
    SECKEYPublicKey *peerKey;
    PRArenaPool *arena;
    SSL3Hashes hashes;
#ifdef FORTEZZA
    FortezzaKEAContext *context;
#endif

    SSL_TRC(3, ("%d: SSL3[%d]: handle server key exchange",
		SSL_GETPID(), ss->fd));

    if (ss->ssl3->hs.ws != wait_server_key &&
	ss->ssl3->hs.ws != wait_server_cert) goto loser;
    if (ss->sec->peerCert == NULL) goto loser;


    switch (ss->ssl3->hs.kea_def->alg) {
    case kt_rsa:
    	rv = ssl3_ConsumeHandshakeVariable(ss, &modulus, 2, &b, &length);
    	if (rv < 0) goto loser;
    	rv = ssl3_ConsumeHandshakeVariable(ss, &exponent, 2, &b, &length);
    	if (rv < 0) goto loser;
    	rv = ssl3_ConsumeHandshakeVariable(ss, &signature, 2, &b, &length);
    	if (rv < 0) goto loser;
    	if (length != 0) goto loser;

    	/*
     	 *  check to make sure the hash is signed by right guy
     	 */
    
    	rv = ssl3_ComputeExportRSAKeyHash(
		modulus, exponent, &ss->ssl3->hs.client_random,
		&ss->ssl3->hs.server_random, &hashes);
        if (rv == SECFailure) goto loser;
        rv = ssl3_CheckSignedHashes( &hashes, ss->sec->peerCert, &signature,
					ss->sec->getClientAuthDataArg);
	if (rv == SECFailure)  goto loser;

	/*
	 * we really need to build a new key here because we can no longer
	 * ignore calling SECKEY_DestroyPublicKey. Using the key may allocate
	 * pkcs11 slots and ID's.
	 */
    	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (arena == NULL) goto loser;
    	ss->sec->peerKey = peerKey = (SECKEYPublicKey *)PORT_ArenaZAlloc(
		arena, sizeof(SECKEYPublicKey));
    	if (ss->sec->peerKey == NULL) goto loser;
	peerKey->arena = arena;
	peerKey->keyType = rsaKey;
	peerKey->pkcs11Slot = NULL;
	peerKey->pkcs11ID = CK_INVALID_KEY;
    	peerKey->u.rsa.modulus.data = PORT_ArenaAlloc(arena, modulus.len);
	if (peerKey->u.rsa.modulus.data == NULL) goto no_memory;
    	PORT_Memcpy(peerKey->u.rsa.modulus.data, modulus.data, modulus.len);
    	peerKey->u.rsa.modulus.len = modulus.len;

    	peerKey->u.rsa.publicExponent.data =
	    PORT_ArenaAlloc(arena, exponent.len);
        if (peerKey->u.rsa.publicExponent.data == NULL) goto no_memory;
    	PORT_Memcpy(peerKey->u.rsa.publicExponent.data,
		    exponent.data, exponent.len);
    	peerKey->u.rsa.publicExponent.len = exponent.len;

    	PORT_Free(modulus.data);
    	PORT_Free(exponent.data);
	PORT_Free(signature.data);
    	ss->ssl3->hs.ws = wait_cert_request;
    	return SECSuccess;
#ifdef FORTEZZA
    case kt_fortezza:
	PORT_Assert(ss->ssl3->kea_context);

	/* Fortezza need *BOTH* a server cert message and a server key
	 * exchange message.
 	 */
	if (ss->ssl3->hs.ws == wait_server_cert) goto loser;
	context = (FortezzaKEAContext *)ss->ssl3->kea_context;
    	rv = ssl3_ConsumeHandshake(
		ss, &ss->ssl3->R_s, sizeof(context->R_s), &b, &length);
    	if (rv < 0) goto loser;

    	ss->ssl3->hs.ws = wait_cert_request;
    	return SECSuccess;
#endif
    default:
	goto loser;
    }
loser:
    if (modulus.data != NULL) PORT_Free(modulus.data);
    if (exponent.data != NULL) PORT_Free(exponent.data);
    if (signature.data != NULL) PORT_Free(signature.data);
    return ssl3_HandshakeFailure(ss);
no_memory:
    if (modulus.data != NULL) PORT_Free(modulus.data);
    if (exponent.data != NULL) PORT_Free(exponent.data);
    if (signature.data != NULL) PORT_Free(signature.data);
    return SECFailure;
}


typedef struct dnameNode {
    struct dnameNode *next;
    SECItem name;
} dnameNode;

SECStatus
SSL3_HandleCertificateRequest(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECStatus rv;
    SSL3AlertDescription desc = handshake_failure;
    SECItem cert_types;
    SSLSecurityInfo *sec = ss->sec;
    SSL3State *ssl3 = ss->ssl3;
    CERTDistNames ca_list;
    PRArenaPool *arena = NULL;
    dnameNode *node;
    int i, remaining, len, nnames = 0;
    unsigned char *data;

    SSL_TRC(3, ("%d: SSL3[%d]: handle certificate request",
		SSL_GETPID(), ss->fd));

    if (ssl3->hs.ws != wait_cert_request) {
	desc = unexpected_message;
	goto loser;
    }
    rv = ssl3_ConsumeHandshakeVariable(ss, &cert_types, 1, &b, &length);
    if (rv < 0) goto loser;

    arena = ca_list.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto no_mem;

    remaining = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (remaining < 0) goto loser;

    desc = unexpected_message;
    ca_list.head = node = PORT_ArenaZAlloc(arena, sizeof(dnameNode));
    if (node == NULL) goto no_mem;

    while (remaining != 0) {
	if (remaining < 2) goto loser;

	len = node->name.len = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
	if (len < 0) goto loser;
	remaining -= 2;
	if (remaining < len) goto loser;

	data = node->name.data = PORT_ArenaAlloc(arena, len);
	if (data == NULL) goto no_mem;
	rv = ssl3_ConsumeHandshake(ss, data, len, &b, &length);
	if (rv < 0) goto loser;
	remaining -= len;
	nnames++;
	if (remaining == 0) break;

	node->next = PORT_ArenaZAlloc(arena, sizeof(dnameNode));
	node = node->next;
	if (node == NULL) goto no_mem;
    }

    ca_list.nnames = nnames;
    ca_list.names = PORT_ArenaAlloc(arena, nnames * sizeof(SECItem));
    if (ca_list.names == NULL) goto no_mem;

    for(i = 0, node = ca_list.head; i < nnames; i++, node = node->next) {
	ca_list.names[i] = node->name;
    }
        
    if (length != 0) goto loser;
    ssl3->hs.ws = wait_hello_done;

    if (sec->getClientAuthData == NULL) {
	rv = -1; /* force it to send a no_certificate alert */
    } else {
	rv = (*sec->getClientAuthData)(sec->getClientAuthDataArg, ss->fd,
				       &ca_list, &ssl3->clientCertificate,
				       &ssl3->clientPrivateKey);
    }
    if (rv < 0) {
	if (rv == SECWouldBlock) {
	    return SECWouldBlock;
	} else {
	    (void)SSL3_SendAlert(ss, alert_warning, no_certificate);
	}
    } else {
	ssl3->clientCertChain = CERT_CertChainFromCert(CERT_GetDefaultCertDB(),
						      ssl3->clientCertificate);
	if (ssl3->clientCertChain == NULL) {
	    if (ssl3->clientCertificate != NULL) {
		CERT_DestroyCertificate(ssl3->clientCertificate);
		ssl3->clientCertificate = NULL;
	    }
	    if (ssl3->clientPrivateKey != NULL) {
		SECKEY_DestroyPrivateKey(ssl3->clientPrivateKey);
		ssl3->clientPrivateKey = NULL;
	    }
	    (void)SSL3_SendAlert(ss, alert_warning, no_certificate);
	}
    }
    
    rv = SECSuccess;
    goto done;
    
no_mem:
    rv = SECFailure;
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    goto done;
loser:
    rv = SECFailure;
    (void)SSL3_SendAlert(ss, alert_fatal, desc);
done:
    if (arena != NULL) PORT_FreeArena(arena, PR_FALSE);
    return rv;
}

SECStatus
SSL3_HandleServerHelloDone(SSLSocket *ss)
{
    SECStatus rv;
    SSL3WaitState ws = ss->ssl3->hs.ws;
    PRBool send_verify = PR_FALSE;

    SSL_TRC(3, ("%d: SSL3[%d]: handle server hello done",
		SSL_GETPID(), ss->fd));

    if (ws != wait_hello_done && ws != wait_server_cert &&
	ws != wait_server_key && ws != wait_cert_request) {
	SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	return SECFailure;
    }
    if (ss->ssl3->clientCertChain != NULL &&
	ss->ssl3->clientPrivateKey != NULL) {
	send_verify = PR_TRUE;
	rv = ssl3_SendCertificate(ss);
	if (rv < 0) return rv;
    }

    rv = SSL3_SendClientKeyExchange(ss);
    if (rv < 0) return rv;

    if (send_verify) {
	rv = SSL3_SendCertificateVerify(ss);
	if (rv < 0) return rv;
    }
    rv = ssl3_SendChangeCipherSpecs(ss);
    if (rv < 0) return rv;
    rv = ssl3_SendFinished(ss);
    if (rv < 0) return rv;
    ss->ssl3->hs.ws = wait_change_cipher;
    return SECSuccess;
}

/*
 * Routines used by servers
 */
SECStatus
SSL3_SendHelloRequest(SSLSocket *ss)
{
    int rv;

    SSL_TRC(3, ("%d: SSL3[%d]: send hello request", SSL_GETPID(), ss->fd));

    rv = ssl3_AppendHandshakeHeader(ss, hello_request, 0);
    if (rv < 0) return rv;
    rv = ssl3_FlushHandshake(ss);
    if (rv < 0) return rv;
    ss->ssl3->hs.ws = wait_client_hello;
    return SECSuccess;
}

static SSLSessionID *
ssl3_NewSessionID(SSLSocket *ss, PRBool is_server)
{
    SSLSessionID *sid;

    sid = (SSLSessionID*) PORT_ZAlloc(sizeof(SSLSessionID));
    if (sid == NULL) return NULL;

    sid->addr = ss->sec->ci->peer;
    sid->port = ss->sec->ci->port;
    if (ss->peerID == NULL) {
	sid->peerID = NULL;
    } else {
	sid->peerID = PORT_Strdup(ss->peerID);
    }
    sid->u.ssl3.resumable = PR_TRUE;
    sid->u.ssl3.policy = SSL_ALLOWED;
    sid->references = 1;
    sid->cached = never_cached;
    sid->version = SSL_LIBRARY_VERSION_3_0;
#ifdef FORTEZZA
    sid->u.ssl3.hasFortezza = PR_FALSE;
    sid->u.ssl3.clientWriteKey = NULL;
    sid->u.ssl3.serverWriteKey = NULL;
    sid->u.ssl3.tek = NULL;
#endif

    if (is_server) {
	int pid = SSL_GETPID();
	sid->u.ssl3.sessionIDLength = SSL3_SESSIONID_BYTES;
	sid->u.ssl3.sessionID[0] = (pid >> 8) & 0xff;
	sid->u.ssl3.sessionID[1] = pid & 0xff;
	RNG_GenerateGlobalRandomBytes(
	    sid->u.ssl3.sessionID + 2, SSL3_SESSIONID_BYTES -2);
    }
    return sid;
}

static SECStatus
ssl3_SendServerHelloSequence(SSLSocket *ss, SSL3CipherSpec *spec)
{
    SECStatus rv;
    const SSL3KEADef *kea_def;

    SSL_TRC(3, ("%d: SSL3[%d]: begin server hello sequence",
		SSL_GETPID(), ss->fd));
    rv = SSL3_SendServerHello(ss);
    if (rv < 0) return rv;
    rv = ssl3_SendCertificate(ss);
    if (rv < 0) return rv;
    /* We have to do this here, because kea_def is not set up
     * until SSL3_SendServerHello(). */
    kea_def = ss->ssl3->hs.kea_def;
    /* XXX we should really check the key length against the limit */
    if (kea_def->is_limited && exportKey != NULL) {
	rv = ssl3_SendServerKeyExchange(ss);
	if (rv < 0) return rv;
    }
    else if (kea_def->alg == kt_fortezza) {
	rv = ssl3_SendServerKeyExchange(ss);
	if (rv < 0) return rv;
    }
    if (ss->requestCertificate) {
	rv = ssl3_SendCertificateRequest(ss);
	if (rv < 0) return rv;
    }
    rv = ssl3_SendServerHelloDone(ss);
    if (rv < 0) return rv;
    if (ss->requestCertificate) {
	ss->ssl3->hs.ws = wait_client_cert;
    } else {
	ss->ssl3->hs.ws = wait_client_key;
    }
    return SECSuccess;
}

SECStatus
SSL3_HandleClientHello(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECStatus rv;
    SSL3ProtocolVersion version;
    SECItem sidBytes = {siBuffer, NULL, 0};
    SSLSessionID *sid = NULL;
    SSL3State *ssl3 = ss->ssl3;
    int i, j;
    SECItem suites = {siBuffer, NULL, 0};
    SECItem comps = {siBuffer, NULL, 0};
    SSLConnectInfo *ci;
    SSL3CipherSpec *spec;
#ifdef FORTEZZA
    SSL3KeyExchangeAlgorithm kea;
    SSL3BulkCipher cipher;
    PRBool use_fortezza = PR_FALSE;
#endif

    SSL_TRC(3, ("%d: SSL3[%d]: handle client hello", SSL_GETPID(), ss->fd));

    /* Get peer name of client */
    rv = ssl_GetPeerInfo(ss);
    if (rv < 0) {
	goto loser;
    }

    rv = ssl3_InitState(ss);
    if (rv < 0) return SECFailure;
    if ((ss->ssl3->hs.ws != wait_client_hello) &&
	(ss->ssl3->hs.ws != idle_handshake)) {
	goto loser;
    }
    ci = ss->sec->ci;
    spec = ssl3->pending_read;

#ifdef FORTEZZA
    /* don't even try fortezza if it has been turned off */
    if (SSL_IsEnabledGroup(SSL_GroupFortezza)) {
    	rv = FortezzaOpen(ss,(FortezzaKEAContext **)&ssl3->kea_context);
	switch (rv) {
	case SECSuccess:
	    use_fortezza = PR_TRUE;
	    break;
	case SECFailure:
	    ssl_TmpEnableGroup(&localCipherSuites,&localCipherSuiteCount,
						SSL_GroupFortezza,PR_FALSE);
	    break; /*  Failed, don't use the Fortezza card */
	default:
	    return (rv); /* SECWouldBlock */
	}
    }
#endif

    version = (SSL3ProtocolVersion)ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (version < SSL_LIBRARY_VERSION_3_0)
	goto loser;
    if (version > SSL_LIBRARY_VERSION_3_0)
	version = SSL_LIBRARY_VERSION_3_0; /* nothing else supported yet */
    
    rv = ssl3_ConsumeHandshake(
	ss, &ss->ssl3->hs.client_random, SSL3_RANDOM_LENGTH, &b, &length);
    if (rv < 0) goto loser;

    rv = ssl3_ConsumeHandshakeVariable(ss, &sidBytes, 1, &b, &length);
    if (rv < 0) goto loser;

    if (sidBytes.len > 0) {
	SSL_TRC(7, ("%d: SSL3[%d]: server, lookup client session-id for 0x%08x",
                    SSL_GETPID(), ss->fd, ci->peer));
	sid = (*ssl_sid_lookup)(ci->peer, sidBytes.data, sidBytes.len);
    }
    SECITEM_FreeItem(&sidBytes, PR_FALSE);
    rv = ssl3_ConsumeHandshakeVariable(ss, &suites, 2, &b, &length);
    if (rv < 0) goto loser;

    rv = ssl3_ConsumeHandshakeVariable(ss, &comps, 1, &b, &length);
    if (rv < 0) goto loser;

    if (sid != NULL) {
	if ((sid->peerCert == NULL) && ss->requestCertificate) {
	    ss->sec->uncache(sid);
	    ssl_FreeSID(sid);
	    sid = NULL;
	}
    }
    
    for (i = 0; i < suites.len; i += 2) {
	for (j = 0; j < cipherSuiteCount; j++) {
	    if (config_match(&cipherSuites[j], ss->ssl3->policy, PR_TRUE) &&
		(suites.data[i] == (cipherSuites[j].cipher_suite>>8) & 0xff) &&
		(suites.data[i + 1] == cipherSuites[j].cipher_suite & 0xff)) {
		ss->ssl3->hs.cipher_suite = cipherSuites[j].cipher_suite;
		ss->ssl3->hs.suite_def =
		    ssl_LookupCipherSuiteDef(ss->ssl3->hs.cipher_suite);
		goto suite_found;
	    }
	}
    }
    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
    goto loser;

suite_found:
    for (i = 0; i < comps.len; i++) {
	for (j = 0; j < compressionMethodsCount; j++) {
	    if (comps.data[i] == compressions[j]) {
		ss->ssl3->hs.compression = compressions[j];
		goto compression_found;
	    }
	}
    }
    PORT_SetError(SSL_ERROR_BAD_CLIENT); /* null compression must be supported */
    goto loser;

compression_found:
    PORT_Free(suites.data);
    suites.data = NULL;
    PORT_Free(comps.data);
    comps.data = NULL;

    ss->sec->send = ssl3_SendApplicationData;

#ifdef FORTEZZA
    kea = ss->ssl3->hs.suite_def->key_exchange_algorithm;
    cipher = ss->ssl3->hs.suite_def->bulk_cipher_algorithm;
    /* can only resume fortezza if we have fortezza info */
    if (kea == kea_fortezza) {
	if (sid) {
	    /* this sid does not have enough fortezza info to resume */
	    if (!sid->u.ssl3.hasFortezza) {
		ss->sec->uncache(sid);
		ssl_FreeSID(sid);
		sid = NULL;
	    } else {
		/* force us to use the same card */
		FortezzaKEAContext *kea_context = 
				(FortezzaKEAContext *) ssl3->kea_context;
		if (kea_context->socketId != sid->u.ssl3.fortezzaSocket) {
			FortezzaDestroyKEAContext(kea_context,PR_TRUE);
			kea_context = FortezzaCreateKEAContextFromSocket
						  (sid->u.ssl3.fortezzaSocket);
			ssl3->kea_context = kea_context;
			if (kea_context == NULL) goto loser;
		}
	    }
	}
    }
#endif

    if (sid != NULL ) {
	if (ci->sid) {
	    ss->sec->uncache(ci->sid);
	    ssl_FreeSID(ci->sid);
	}
	ci->sid = sid;
	PORT_Memcpy(ssl3->pending_read->master_secret, sid->u.ssl3.masterSecret,
		  sizeof(ssl3->pending_read->master_secret));
	if (sid->peerCert != NULL) {
	    ss->sec->peerCert = CERT_DupCertificate(sid->peerCert);
	}
	/*
	 * XXX make sure cipher specs and compression still match
	 */
	ssl3->hs.isResuming = PR_TRUE;
	rv = SSL3_SendServerHello(ss);
	if (rv < 0) goto loser;
	rv = InitPendingCipherSpec(ss, ss->ssl3, NULL);
	if (rv < 0) goto loser;
#ifdef FORTEZZA
	if (kea == kea_fortezza) {
	    FortezzaKEAContext *kea_context = 
				(FortezzaKEAContext *) ssl3->kea_context;
	    FortezzaContext *write_context,*read_context;

	    kea_context->tek = sid->u.ssl3.tek;
	    kea_context->tek->ref_count++;	    
	    if (cipher == cipher_fortezza) {
		write_context = ssl3->pending_write->encodeContext;
		read_context = ssl3->pending_read->decodeContext;
		if (write_context) {
		    rv = FortezzaSetKey(write_context,
						sid->u.ssl3.serverWriteKey);
		    if (rv < 0) goto loser;
		    rv = FortezzaLoadIV(write_context,
						sid->u.ssl3.serverWriteIV);
		    if (rv < 0) goto loser;
		}
		if (read_context) {
		    rv = FortezzaSetKey(read_context,
						sid->u.ssl3.clientWriteKey);
		    if (rv < 0) goto loser;
		    rv = FortezzaLoadIV(read_context,sid->u.ssl3.clientWriteIV);
		    if (rv < 0) goto loser;
		}
	    }
	}
#endif
	rv = ssl3_SendChangeCipherSpecs(ss);
	if (rv < 0) goto loser;
	rv = ssl3_SendFinished(ss);
	ssl3->hs.ws = wait_change_cipher;
	if (rv < 0) goto loser;
    } else {
	sid = ssl3_NewSessionID(ss, PR_TRUE);
	if (sid == NULL) goto loser;
	ci->sid = sid;
	ssl3->hs.isResuming = PR_FALSE;
	rv = ssl3_SendServerHelloSequence(ss, spec);
	if (rv < 0) goto loser;
    }
    if (length != 0) goto loser;
    return SECSuccess;
loser:
    if (sidBytes.data != NULL) PORT_Free(sidBytes.data);
    if (suites.data != NULL) PORT_Free(suites.data);
    if (comps.data != NULL) PORT_Free(comps.data);
    return ssl3_HandshakeFailure(ss);
}

/*
 * SSL3_HandleV2ClientHello is used when a V2 formatted hello comes
 * in asking to use the V3 handshake.
 */
SECStatus
SSL3_HandleV2ClientHello(SSLSocket *ss, unsigned char *buffer, int length)
{
    SSLSessionID *sid = NULL;
    int i, j;
    SECStatus rv;
    SSL3CipherSpec *spec;
    SSLConnectInfo *ci = ss->sec->ci;
    SSL3ProtocolVersion version;
    int sid_length, suite_length, rand_length;
    unsigned char *suites, *random;

    SSL_TRC(3, ("%d: SSL3[%d]: handle v2 client hello", SSL_GETPID(), ss->fd));

    rv = ssl3_InitState(ss);
    if (rv < 0) return SECFailure;

    if (ss->ssl3->hs.ws != wait_client_hello) goto loser;

#ifdef FORTEZZA
    /* don't even try fortezza if it has been turned off */
    if (SSL_IsEnabledGroup(SSL_GroupFortezza)) {
    	rv = FortezzaOpen(ss,(FortezzaKEAContext **)&ss->ssl3->kea_context);
	switch (rv) {
	case SECSuccess:
	    break;
	case SECFailure:
	    ssl_TmpEnableGroup(&localCipherSuites,&localCipherSuiteCount,
						SSL_GroupFortezza,PR_FALSE);
	    break; /*  Failed, don't use the Fortezza card */
	default:
	    return (rv); /* SECWouldBlock */
	}
    }
#endif

    version = (buffer[1] << 8) | buffer[2];
    suite_length = (buffer[3] << 8) | buffer[4];
    sid_length = (buffer[5] << 8) | buffer[6];
    rand_length = (buffer[7] << 8) | buffer[8];
    if ((sid_length > 0) || /* known session => v3 client */
	(length != SSL_HL_CLIENT_HELLO_HBYTES + suite_length + rand_length)) {
	SSL_DBG(("%d: SSL3[%d]: bad v2 client hello message, len=%d should=%d",
		 SSL_GETPID(), ss->fd, length,
		 SSL_HL_CLIENT_HELLO_HBYTES + suite_length + rand_length));
	goto loser;
    }
    suites = buffer + SSL_HL_CLIENT_HELLO_HBYTES;
    random = suites + suite_length;

    spec = ss->ssl3->pending_read;

    if (rand_length < SSL_MIN_CHALLENGE_BYTES ||
	rand_length > SSL_MAX_CHALLENGE_BYTES) goto loser;

    PORT_Assert(SSL_MAX_CHALLENGE_BYTES == SSL3_RANDOM_LENGTH);

    PORT_Memset(&ss->ssl3->hs.client_random, 0, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(
	&ss->ssl3->hs.client_random.rand[SSL3_RANDOM_LENGTH - rand_length],
	random, rand_length);

    PRINT_BUF(60, (ss, "client random:", &ss->ssl3->hs.client_random.rand[0],
		   SSL3_RANDOM_LENGTH));

    /* XXX
     * Should  we get our favorite, rather than the client's favorite
     */
    for (j = 0; j < cipherSuiteCount; j++) {
	for (i = 0; i < suite_length; i += 3) {
	    if (config_match(&cipherSuites[j], ss->ssl3->policy, PR_TRUE) &&
		(suites[i] == 0) &&
		(suites[i+1] == ((cipherSuites[j].cipher_suite >> 8) & 0xff)) &&
		(suites[i + 2] == (cipherSuites[j].cipher_suite & 0xff))) {
		ss->ssl3->hs.cipher_suite = cipherSuites[j].cipher_suite;
		ss->ssl3->hs.suite_def =
		    ssl_LookupCipherSuiteDef(ss->ssl3->hs.cipher_suite);
		goto suite_found;
	    }
	}
    }
    SSL3_SendAlert(ss, alert_fatal, handshake_failure);
    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
    return SECFailure;
suite_found:

    ss->ssl3->hs.compression = compression_null;
    ss->sec->send = ssl3_SendApplicationData;

    sid = ssl3_NewSessionID(ss, PR_TRUE);
    if (sid == NULL) goto loser;
    ci->sid = sid;

    /* We have to update the handshake hashes before we can send stuff */
    SSL3_UpdateHandshakeHashes(ss->ssl3, buffer, length);

    /* do not worry about memory leak of sid since it now belongs to ci */
    rv = ssl3_SendServerHelloSequence(ss, spec);
    if (rv < 0) goto loser;

    /* We need to return SECWouldBlock so that the server doesn't block. */
    return SECWouldBlock;
loser:
    PORT_SetError(SSL_ERROR_BAD_CLIENT);
    /* what kind of error should be sent back XXX */
    return SECFailure;
}

static SECStatus
SSL3_SendServerHello(SSLSocket *ss)
{
    SSL3ProtocolVersion version = SSL_LIBRARY_VERSION_3_0;
    SSLSessionID *sid;
    SECStatus rv;
    long length;

    SSL_TRC(3, ("%d: SSL3[%d]: send server hello", SSL_GETPID(), ss->fd));

    PORT_Assert(ss->sec && ss->sec->ci);

    sid = ss->sec->ci->sid;
    length = sizeof(SSL3ProtocolVersion) + SSL3_RANDOM_LENGTH +
	1 + ((sid == NULL) ? 0: SSL3_SESSIONID_BYTES) +
	sizeof(SSL3CipherSuite) + 1;
    rv = ssl3_AppendHandshakeHeader(ss, server_hello, length);
    if (rv < 0) return rv;

    rv = ssl3_AppendHandshakeNumber(ss, version, 2);
    if (rv < 0) return rv;
    rv = ssl3_GetNewRandom(&ss->ssl3->hs.server_random);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshake(
	ss, &ss->ssl3->hs.server_random, SSL3_RANDOM_LENGTH);
    if (rv < 0) return rv;
    
    if (sid)
	rv = ssl3_AppendHandshakeVariable(
	    ss, sid->u.ssl3.sessionID, sid->u.ssl3.sessionIDLength, 1);
    else
	rv = ssl3_AppendHandshakeVariable(ss, NULL, 0, 1);
    if (rv < 0) return rv;

    rv = ssl3_AppendHandshakeNumber(ss, ss->ssl3->hs.cipher_suite, 2);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshakeNumber(ss, ss->ssl3->hs.compression, 1);
    if (rv < 0) return rv;
    ssl3_SetupPendingCipherSpec(ss, ss->ssl3);

    return SECSuccess;
}


static SECStatus
ssl3_SignExportRSAKeyHash(SECKEYPrivateKey *server_key, SSL3Hashes *hashes,
			  SECItem *signed_hash)
{
    int rv;
    
    PORT_Assert(signed_hash != NULL);
    PORT_Assert(signed_hash->data == NULL);
    signed_hash->data = PORT_Alloc(MAX_RSA_MODULUS_LEN);
    if (signed_hash->data == NULL) return SECFailure;
    rv = ssl3_SignHashes(hashes, server_key, signed_hash);
    if (rv == SECFailure) {
	PORT_Free(signed_hash->data);
	return SECFailure;
    }
    return SECSuccess;
}


static SECStatus
ssl3_SendServerKeyExchange(SSLSocket *ss)
{
    SECStatus rv = SECFailure;
    int length;
    SSL3Hashes hashes;
    SECItem signed_hash = {siBuffer, NULL, 0};
    const SSL3KEADef *kea_def = ss->ssl3->hs.kea_def;
#ifndef FORTEZZA
    /* XXX only works for RSA */ 
#else
    FortezzaKEAContext *kea_context;
#endif    

    SSL_TRC(3, ("%d: SSL3[%d]: send server key exchange",
		SSL_GETPID(), ss->fd));

    switch (kea_def->alg) {
    case kt_rsa:
    	rv = ssl3_ComputeExportRSAKeyHash(exportPubKey->u.rsa.modulus,
					  exportPubKey->u.rsa.publicExponent,
	    &ss->ssl3->hs.client_random,&ss->ssl3->hs.server_random,&hashes);
        if (rv == SECFailure) return rv;
	rv = ssl3_SignExportRSAKeyHash(usKey, &hashes, &signed_hash);
        if (rv == SECFailure) return rv;
	if (signed_hash.data == NULL) goto loser;
	length = 2 + exportPubKey->u.rsa.modulus.len +
	    2 + exportPubKey->u.rsa.publicExponent.len +
	    2 + signed_hash.len;
   
	rv = ssl3_AppendHandshakeHeader(ss, server_key_exchange, length);
	if (rv < 0) goto loser;

	rv = ssl3_AppendHandshakeVariable(ss, exportPubKey->u.rsa.modulus.data,
					  exportPubKey->u.rsa.modulus.len, 2);
	if (rv < 0) goto loser;

	rv = ssl3_AppendHandshakeVariable(ss,
				 exportPubKey->u.rsa.publicExponent.data, 
				 exportPubKey->u.rsa.publicExponent.len,
					  2);
	if (rv < 0) goto loser;

	rv = ssl3_AppendHandshakeVariable(
	     ss, signed_hash.data, signed_hash.len, 2);
	if (rv < 0) goto loser;
	PORT_Free(signed_hash.data);
	return SECSuccess;
	break;
    case kt_fortezza:
#ifdef FORTEZZA
	rv = PK11_FortezzaGenerateRa(fortezzaServerKey,ssl->ssl3->R_s);
	if (rv < 0) return rv;

	/* don't waste time signing the random number */
	length = sizeof (ssl->ssl3->R_s) /*+ 2 + signed_hash.len*/;

	rv = ssl3_AppendHandshakeHeader(ss, server_key_exchange, length);
	if (rv < 0) goto loser;
    
	rv = ssl3_AppendHandshake( ss, &ssl->ssl3->R_s, 
						sizeof(ssl->ssl3->R_s));
	if (rv < 0) goto loser;
	return SECSuccess;
#endif
    case kt_null:
    case kt_dh:
	break;
    }
loser:
    if (signed_hash.data != NULL) PORT_Free(signed_hash.data);
    return SECFailure;
}


SECStatus
ssl3_SendCertificateRequest(SSLSocket *ss)
{
    SECStatus rv;
    int length, i, calen = 0;
    SECItem *name;
    CERTDistNames *ca_list;
    SECItem *names = NULL;
    int nnames = 0;

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate request",
		SSL_GETPID(), ss->fd));

    if (ss->ssl3->ca_list != NULL) {
	ca_list = ss->ssl3->ca_list;
    } else {
	ca_list = ssl3_server_ca_list;
    }

    if (ca_list != NULL) {
	names = ca_list->names;
	nnames = ca_list->nnames;
    }

    for(i = 0, name = names; i < nnames; i++, name++) {
	calen += 2 + name->len;
    }
    length = 1 + sizeof(certificate_types) + 2 + calen;

    rv = ssl3_AppendHandshakeHeader(ss, certificate_request, length);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshakeVariable(
	ss, certificate_types, sizeof(certificate_types), 1);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshakeNumber(ss, calen, 2);
    if (rv < 0) return rv;
    for (i = 0, name = names; i < nnames; i++, name++) {
	rv = ssl3_AppendHandshakeVariable(ss, name->data, name->len, 2);
	if (rv < 0) return rv;
    }

    return SECSuccess;
}

static SECStatus
ssl3_SendServerHelloDone(SSLSocket *ss)
{
    SECStatus rv;

    SSL_TRC(3, ("%d: SSL3[%d]: send server hello done", SSL_GETPID(), ss->fd));

    rv = ssl3_AppendHandshakeHeader(ss, server_hello_done, 0);
    if (rv < 0) return rv;
    rv =  ssl3_FlushHandshake(ss);
    if (rv < 0) return rv;
    return SECSuccess;
}

static SECStatus
ssl3_HandleCertificateVerify(SSLSocket *ss, SSL3Opaque *b, long length,
			     SSL3Hashes *hashes)
{
    SECItem signed_hash = {siBuffer, NULL, 0};
    SECStatus rv;

    SSL_TRC(3, ("%d: SSL3[%d]: handle certificate verify",
		SSL_GETPID(), ss->fd));

    if (ss->ssl3->hs.ws != wait_cert_verify) goto loser;
    if (ss->sec->peerCert == NULL) goto loser;

    rv = ssl3_ConsumeHandshakeVariable(ss, &signed_hash, 2, &b, &length);
    if (rv < 0) goto loser;

    /* verify that the key & kea match */
    rv = ssl3_CheckSignedHashes(hashes, ss->sec->peerCert, &signed_hash,
					ss->sec->getClientAuthDataArg);
    if (rv < 0) goto loser;

    PORT_Free(signed_hash.data);
    signed_hash.data = NULL;

    if (length != 0) goto loser;
    ss->ssl3->hs.ws = wait_change_cipher;
    return SECSuccess;
loser:
    if (signed_hash.data != NULL) PORT_Free(signed_hash.data);
    return ssl3_HandshakeFailure(ss);
}

SECStatus
SSL3_HandleClientKeyExchange(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECKEYPrivateKey *serverKey = NULL;
    SECItem enc_pms;
    PK11SymKey *symKey;
    int rv;

    SSL_TRC(3, ("%d: SSL3[%d]: handle client key exchange",
		SSL_GETPID(), ss->fd));

    if (ss->ssl3->hs.ws != wait_client_key) {
	SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	return SECFailure;
    }

    if (exportKey != NULL && ss->ssl3->hs.kea_def->is_limited) {
	serverKey = exportKey;
    } else {
	serverKey = usKey;
    }

    switch (ss->ssl3->hs.kea_def->alg) {
    case kt_rsa:
	/*
	 * decrypt out of the incoming buffer
	 */
	enc_pms.data = b;
	enc_pms.len= length;
	symKey = PK11_PubUnwrapSymKey(serverKey, &enc_pms, 
					CKM_SSL3_PRE_MASTER_KEY_GEN, 0);
	if (symKey == NULL) return SECFailure;

	rv = PK11_ExtractKeyValue(symKey);
	if (rv != SECSuccess) {
	    PK11_FreeSymKey(symKey);
	    return SECFailure;
	}

	PRINT_BUF(60, (ss, "decrypted premaster secret:",
		 PK11_GetKeyData(symKey)->data,PK11_GetKeyData(symKey)->len));

	rv = InitPendingCipherSpec(ss, ss->ssl3, PK11_GetKeyData(symKey));
	PK11_FreeSymKey(symKey);
	if (rv < 0) return SECFailure;
	break;

#ifdef FORTEZZA
    case kt_fort:

	/* get the structure */
	kea_context = (FortezzaKEAContext *)ss->ssl3->kea_context;

	tek = FortezzaTEKAlloc(kea_context);
	kea_context->tek = tek; /* Adopt the allocated TEK */
	if (tek == NULL) return SECFailure;

    	rv = ssl3_ConsumeHandshakeVariable(ss,&fortezza_pms.y_c, 1,&b,&length);
    	if (rv < 0) return rv;
    	rv = ssl3_ConsumeHandshake( ss, &fortezza_pms.r_c, 
		sizeof(fortezza_pms)-sizeof(fortezza_pms.y_c), &b, &length);
    	if (rv < 0) return rv;


	/* build a Token Encryption key (tek) TEK's can never be unloaded
	 * from the card, but given these parameters, and *OUR* fortezza
	 * card, we can always regenerate the same one on the fly. */
	tek->keyRegister = KeyNotLoaded;
	tek->keyType = TEK;
	/* get the clients public key */
        server_cert = FortezzaGetCertificate(
		fortezzaServerKey->fortezzaKeyData.socket,
		     fortezzaServerKey->fortezzaKeyData.certificate,
				fortezzaServerKey->fortezzaKeyData.serial);
	if (server_cert == NULL) return SECFailure;
    	if (ss->sec->peerCert != NULL && 
	     (FortezzaComparePQG(ss->sec->peerCert,server_cert) == 0)) {
		CERTCertificate *cert = ss->sec->peerCert;
		SECKEYPublicKey *pubkey;

		pubkey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
		if (pubkey == NULL) {
		    CERT_DestroyCertificate(server_cert);
		    return SECFailure;
		}
		PORT_Assert(pubkey->keyType == fortezzaKeys); /* paranoia */
		PORT_Memcpy(tek->keyData.tek.Y_b,
				pubkey->fortezzaKeyData.KEAKey.data, 
					pubkey->fortezzaKeyData.KEAKey.len);
		tek->keyData.tek.Y_bSize = pubkey->fortezzaKeyData.KEAKey.len;
		SECKEY_DestroyPublicKey(pubkey);
	} else if (fortezza_pms.y_c.len != 0) {
		PORT_Memcpy(tek->keyData.tek.Y_b,fortezza_pms.y_c.data,
							fortezza_pms.y_c.len);
		tek->keyData.tek.Y_bSize = fortezza_pms.y_c.len;

		/* if we have client auth on, check the signature */
		if (ss->sec->peerCert) {
		    unsigned char hash[20];
		    SECKEYPublicKey *pubkey;

		    pubkey = CERT_ExtractPublicKey(
				&ss->sec->peerCert->subjectPublicKeyInfo);
		    if (pubkey == NULL) {
		    CERT_DestroyCertificate(server_cert);
			return SECFailure;
		    }
		    PORT_Assert(pubkey->keyType == fortezzaKeys); /* paranoia */

		    ssl3_ComputeFortezzaPublicKeyHash(
						tek->keyData.tek.Y_b,hash);
		    if (DSAVerify(hash, fortezza_pms.y_signature, pubkey) 
								== PR_FALSE) {
			/* probably not the correct allert */
			SSL3_SendAlert(ss,alert_fatal,illegal_parameter);
			CERT_DestroyCertificate(server_cert);
			SECKEY_DestroyPublicKey(pubkey);
			return SECFailure;
		    }
		    SECKEY_DestroyPublicKey(pubkey);
		}
	} else {
		SSL3_SendAlert(ss,alert_fatal,illegal_parameter);
		CERT_DestroyCertificate(server_cert);
		return SECFailure;
	}
	CERT_DestroyCertificate(server_cert);
	tek->keyData.tek.certificate = 
				fortezzaServerKey->fortezzaKeyData.certificate;
	PORT_Memcpy(tek->keyData.tek.R_b,kea_context->R_s,
						sizeof(tek->keyData.tek.R_b));
	PORT_Memcpy(tek->keyData.tek.R_a,fortezza_pms.r_c,
						sizeof(tek->keyData.tek.R_a));
	tek->keyData.tek.flags = CI_RECIPIENT_FLAG;

	
	/* decrypt the pre-Master secret with the TEK */
	rv = FortezzaCryptPremaster(Decrypt,kea_context,tek,
		fortezza_pms.master_secret_iv,
		fortezza_pms.encrypted_preMasterSecret, buffer,
			sizeof(fortezza_pms.encrypted_preMasterSecret));
	if (rv < 0) { goto fortezza_loser; }

	pms.data = buffer;
	pms.len = sizeof(fortezza_pms.encrypted_preMasterSecret);
	rv = InitPendingCipherSpec(ss, ss->ssl3, &pms);
	PORT_Memset(buffer, 0, sizeof(fortezza_pms.encrypted_preMasterSecret));
	if (rv < 0) { goto fortezza_loser; }

	/* get the context pointers (to save on long code and protect against
	 * long dereferences in stupid compiliers) */
	if (ss->ssl3->pending_write->bulk_cipher_algorithm == cipher_fortezza) {
		write_context = ss->ssl3->pending_write->encodeContext;
		read_context = ss->ssl3->pending_read->decodeContext;
	} else {
		/* fortezza NULL_SHA  or RC4 don't have fortezza contexts...
		 * create some temporary ones.
		 */
	    tmp_write = FortezzaCreateContext(kea_context,Encrypt);
	    write_context = tmp_write;
	    tmp_read= FortezzaCreateContext(kea_context,Decrypt);
	    read_context = tmp_read;
	}

	/* UnwrapKey used the key to unwrap the key into a free storeage,
	 * then stores a Ks Wrapped MEK in the context so we can load it.
	 * at will without the TEK.
	 */
	rv = FortezzaUnwrapKey(write_context,tek,
			fortezza_pms.wrapped_server_write_key);
	if (rv < 0) { goto fortezza_loser; }

	/*
	 * Load IV it also activates our context (until now we couldn't
	 * issue an operation on the context. After this call we can).
	 */
	rv = FortezzaLoadIV(write_context,fortezza_pms.server_write_iv);
	if (rv < 0) { goto fortezza_loser; }

	/* now get the client write (our read) keys */
	rv = FortezzaUnwrapKey(read_context,tek,
			fortezza_pms.wrapped_client_write_key);
	if (rv < 0) { goto fortezza_loser; }
	rv = FortezzaLoadIV(read_context,fortezza_pms.client_write_iv);
	if (rv < 0) { goto fortezza_loser; }

	sid->u.ssl3.hasFortezza = PR_TRUE;
	sid->u.ssl3.fortezzaSocket = kea_context->socketId;
	sid->u.ssl3.tek = kea_context->tek;
	sid->u.ssl3.tek->ref_count++;
	sid->u.ssl3.clientWriteKey = read_context->theKey;
	FortezzaKeyReference(sid->u.ssl3.clientWriteKey);
	sid->u.ssl3.serverWriteKey = write_context->theKey;
	FortezzaKeyReference(sid->u.ssl3.serverWriteKey);
	PORT_Memcpy(sid->u.ssl3.clientWriteIV,read_context->iv,
							sizeof(FortezzaIV));
	PORT_Memcpy(sid->u.ssl3.serverWriteIV,write_context->iv,
							sizeof(FortezzaIV));
	if (tmp_read) FortezzaDestroyContext(tmp_read,PR_TRUE);
	if (tmp_write) FortezzaDestroyContext(tmp_write,PR_TRUE);
	break;

fortezza_loser:
	if (tmp_read) FortezzaDestroyContext(tmp_read,PR_TRUE);
	if (tmp_write) FortezzaDestroyContext(tmp_write,PR_TRUE);
	return rv;
#endif
    default:
	return ssl3_HandshakeFailure(ss);
    }
    if (ss->sec->peerCert != NULL)
        ss->ssl3->hs.ws = wait_cert_verify;
    else
        ss->ssl3->hs.ws = wait_change_cipher;
    return SECSuccess;
}

/*
 * used by both
 */
SECStatus
ssl3_SendCertificate(SSLSocket *ss)
{
    int rv;
    CERTCertificateList *certChain;
    int len = 0;
    int i;

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate", SSL_GETPID(), ss->fd));

#ifdef FORTEZZA
    certChain = (ss->sec->isServer) ? 
	((ss->ssl3->hs.kea == kea_fortezza) ?
 		ssl3_fortezza_server_cert_chain : ssl3_server_cert_chain) :
	ss->ssl3->clientCertChain;
#else
    certChain = (ss->sec->isServer) ? ssl3_server_cert_chain :
	ss->ssl3->clientCertChain;
#endif

    for (i = 0; i < certChain->len; i++) {
	len += certChain->certs[i].len + 3;
    }

    rv = ssl3_AppendHandshakeHeader(ss, certificate, len + 3);
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshakeNumber(ss, len, 3);
    if (rv < 0) return rv;
    for (i = 0; i < certChain->len; i++) {
	rv = ssl3_AppendHandshakeVariable(ss, certChain->certs[i].data,
					  certChain->certs[i].len, 3);
	if (rv < 0) return rv;
    }
    
    return SECSuccess;
}

#ifndef FORTEZZA
typedef struct certNode {
    struct certNode *next;
    CERTCertificate *cert;
} SSL3CertNode;
#endif

SECStatus
SSL3_HandleCertificate(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECItem certItem;
    SSL3CertNode *certs = NULL, *c;
    PRArenaPool *arena = NULL;
    SSL3AlertDescription desc = bad_certificate;
    SECStatus rv;
    long remaining;
    long size;
    SSL3State *ssl3 = ss->ssl3;
    SSLSecurityInfo *sec = ss->sec;
    PRBool isServer;
    CERTCertificate *cert;
    
    SSL_TRC(3, ("%d: SSL3[%d]: handle certificate", SSL_GETPID(), ss->fd));

    if (ssl3->hs.ws != wait_server_cert && ssl3->hs.ws != wait_client_cert) {
	desc = unexpected_message;
	goto loser;
    }

    PORT_Assert(ssl3->peerCertArena == NULL);
    
    if (sec->peerCert != NULL) {
	if (sec->peerKey) {
	    SECKEY_DestroyPublicKey(sec->peerKey);
	    sec->peerKey = NULL;
	}
	CERT_DestroyCertificate(sec->peerCert);
	sec->peerCert = NULL;
    }

    /* XXX need to make sure everything gets freed up when we lose */

    remaining = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
    if (remaining < 0) goto bad_cert;

    ssl3->peerCertArena = arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* First get the peer cert. */
    remaining -= 3;
    if (remaining < 0) goto bad_cert;
    size = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
    if (size < 0) goto bad_cert;

    remaining -= size;
    if (remaining < 0) goto bad_cert;
    
    certItem.data = PORT_ArenaAlloc(arena, size);
    if (certItem.data == NULL) goto loser;

    certItem.len = size;
    rv = ssl3_ConsumeHandshake(ss, certItem.data, certItem.len, &b,
			       &length);
    if (rv < 0) goto bad_cert;
	
    ss->sec->peerCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
					       &certItem, NULL, PR_FALSE,
					       PR_TRUE);
    if (ss->sec->peerCert == NULL) goto bad_cert;
    
    /* Now get all of the CA certs. */
    while(remaining != 0) {
	remaining -= 3;
	if (remaining < 0) goto bad_cert;
	size = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
	if (size < 0) goto bad_cert;

	remaining -= size;
	if (remaining < 0) goto bad_cert;

	certItem.data = PORT_ArenaAlloc(arena, size);
	if (certItem.data == NULL) goto loser;

	certItem.len = size;
	rv = ssl3_ConsumeHandshake(ss, certItem.data, certItem.len, &b,
				   &length);
	if (rv < 0) goto bad_cert;

	c = PORT_ArenaAlloc(arena, sizeof(SSL3CertNode));
	if (c == NULL) goto loser;
	c->cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
					 &certItem, NULL, PR_FALSE, PR_TRUE);
	if (c->cert == NULL) goto loser;
	c->next = certs;
	certs = c;
    }

#ifdef FORTEZZA
    if (remaining != 0) goto bad_cert;

    FortezzaPQGUpdate(certs,ss->sec->peerCert);
#endif

    if ( ss->sec->isServer ) {
	isServer = PR_TRUE;
    } else {
	isServer = PR_FALSE;
    }
    
    rv = (int)(* ss->sec->authCertificate)(ss->sec->authCertificateArg,
					   ss->fd, PR_TRUE, isServer);
    if (rv) {
	if ( ss->sec->handleBadCert ) {
	    rv = (* ss->sec->handleBadCert )(ss->sec->badCertArg, ss->fd);
	    if ( rv ) {
		if ( rv == -2 ) {
		    /* someone will handle this connection asynchronously*/
		    SSL_DBG(("%d: SSL3[%d]: go to async cert handler",
			     SSL_GETPID(), ss->fd));
		    ssl3->peerCertChain = certs;
		    certs = NULL;
		    goto cert_block;
		}
		/* cert is bad */
		goto bad_cert;
	    }
	    /* cert is good */
	} else {
	    goto bad_cert;
	}
    }

    cert = ss->sec->peerCert;
    if (policy_some_restricted && (ssl3->policy == SSL_ALLOWED) &&
	(CERT_VerifyCert(cert->dbhandle, cert, PR_FALSE,
			 certUsageSSLServerWithStepUp,
			 PR_Now(), ss->sec->authCertificateArg,
			 NULL) == SECSuccess)) {
	ssl3->policy = SSL_RESTRICTED;
	ssl3->hs.rehandshake = PR_TRUE;
    }

    /* We don't need the CA certs now that we've authenticated the peer cert. */
    for (; certs; certs = certs->next) {
	CERT_DestroyCertificate(certs->cert);
    }
    PORT_FreeArena(arena, PR_FALSE);
    ssl3->peerCertArena = NULL;
    ssl3->peerCertChain = NULL;

    ss->sec->ci->sid->peerCert = CERT_DupCertificate(ss->sec->peerCert);
    
cert_block:    
    if (ss->sec->isServer) {
	ssl3->hs.ws = wait_client_key;
    } else {
	ssl3->hs.ws = wait_cert_request;
	if(ssl3->hs.kea_def->is_limited) {
	    SECKEYPublicKey *key =
		CERT_ExtractPublicKey(&ss->sec->peerCert->subjectPublicKeyInfo);
	    int keyLen = SECKEY_PublicKeyStrength(key);
	    SECKEY_DestroyPublicKey(key);

	    if(keyLen > EXPORT_RSA_KEY_LENGTH) {
		ssl3->hs.ws = wait_server_key;
	    }
	}
#ifdef FORTEZZA
	 else if (ss->ssl3->hs.kea == kea_fortezza) {
		ss->ssl3->hs.ws = wait_server_key;
	 }
#endif
    }

    /* rv must normally be equal to SECSuccess here.  If we called
     * handleBadCert, it can also be SECWouldBlock.
     */
    return rv;

bad_cert:
    desc = bad_certificate;
    SSL_DBG(("%d: SSL3[%d]: peer certificate is no good: error=%d",
	     SSL_GETPID(), ss->fd, PORT_GetError()));

loser:
    if ( certs != NULL ) {
	for (; certs; certs = certs->next) {
	    CERT_DestroyCertificate(certs->cert);
	}
    }
    
    if (arena != NULL) {
	PORT_FreeArena(arena, PR_FALSE);
	ssl3->peerCertArena = NULL;
	ssl3->peerCertChain = NULL;
    }
    if (sec->peerCert != NULL) {
	CERT_DestroyCertificate(sec->peerCert);
	sec->peerCert = NULL;
    }
	
    (void)SSL3_SendAlert(ss, alert_fatal, desc);
    return SECFailure;
}

void
SSL3_CleanupPeerCerts(SSL3State *ssl3)
{
    PRArenaPool *arena = ssl3->peerCertArena;
    SSL3CertNode *certs = (SSL3CertNode *)ssl3->peerCertChain;
    
    for (; certs; certs = certs->next) {
	CERT_DestroyCertificate(certs->cert);
    }
    if (arena) PORT_FreeArena(arena, PR_FALSE);
    ssl3->peerCertArena = NULL;
    ssl3->peerCertChain = NULL;
}


static SECStatus
ssl3_SendFinished(SSLSocket *ss)
{
    SSL3Hashes hashes;
    SECStatus rv;
    SSL3Sender sender = ss->sec->isServer ? sender_server : sender_client;

    SSL_TRC(3, ("%d: SSL3[%d]: send finished", SSL_GETPID(), ss->fd));

    SSL3_ComputeHandshakeHashes(ss->ssl3, ss->ssl3->current_write,
				&hashes, sender);
    rv = ssl3_AppendHandshakeHeader(ss, finished, sizeof(SSL3Hashes));
    if (rv < 0) return rv;
    rv = ssl3_AppendHandshake(ss, &hashes, sizeof(SSL3Hashes));
    rv = ssl3_FlushHandshake(ss);
    if (rv < 0) return rv;

    return SECSuccess;
}

SECStatus
SSL3_HandleFinished(SSLSocket *ss, SSL3Opaque *b, long length,
		    const SSL3Hashes *hashes)
{
    SSLSecurityInfo *sec = ss->sec;
    SSL3State *ssl3 = ss->ssl3;
    SSLSessionID *sid = sec->ci->sid;
    
    SSL_TRC(3, ("%d: SSL3[%d]: handle finished", SSL_GETPID(), ss->fd));

    if (ssl3->hs.ws != wait_finished)
	return ssl3_HandshakeFailure(ss);

    if (length != sizeof(SSL3Hashes) || (PORT_Memcmp(hashes, b, length) != 0))
	return ssl3_HandshakeFailure(ss);

    if ((sec->isServer && !ssl3->hs.isResuming) ||
	(!sec->isServer && ssl3->hs.isResuming)) {
	SECStatus rv;

	rv = ssl3_SendChangeCipherSpecs(ss);
	if (rv < 0) return SECFailure;
	rv = ssl3_SendFinished(ss);
	if (rv < 0) return SECFailure;
	if (sec->isServer || !ssl3->hs.rehandshake) {
	    ss->handshake = NULL;
	    ss->connected = 1;
	    ss->gather->writeOffset = 0;
	    ss->gather->readOffset = 0;
	}
    }

    PORT_Memcpy(sid->u.ssl3.masterSecret, ssl3->current_read->master_secret,
	      sizeof(sid->u.ssl3.masterSecret));
    sid->u.ssl3.cipherSuite = ssl3->hs.cipher_suite;
    sid->u.ssl3.compression = ssl3->hs.compression;
    sid->u.ssl3.policy = ssl3->policy;

    if (!sec->isServer && ssl3->hs.rehandshake) {
	ssl_FreeSID(sid);
	ss->sec->ci->sid = NULL;
	ssl3->hs.rehandshake = PR_FALSE;
	return SSL3_SendClientHello(ss);
    }

    if (!ss->noCache)
	(*sec->cache)(sid);
    ss->ssl3->hs.ws = idle_handshake;
    if (sec->handshakeCallback != NULL) {
	(sec->handshakeCallback)(ss->fd, ss->sec->handshakeCallbackData);
    }

    return SECSuccess;
}

static SECStatus
ssl3_HandleHandshakeMessage(SSLSocket *ss, SSL3Opaque *b, long length)
{
    SECStatus rv = SECSuccess;
    SSL3HandshakeType type = ss->ssl3->hs.msg_type;
    SSL3Hashes hashes;
    char hdr[4];

    /*
     * We have to compute the hashes before we update them with the
     * current message.
     */
    if((type == finished) || (type == certificate_verify)) {
	SSL3Sender sender = 0;
	SSL3CipherSpec *spec = ss->ssl3->pending_read;

	if (type == finished) {
	    sender = ss->sec->isServer ? sender_client : sender_server;
	    spec = ss->ssl3->current_read;
	}
	SSL3_ComputeHandshakeHashes(ss->ssl3, spec, &hashes, sender);
    }
    SSL_TRC(30,("%d: handle handshake message: %d", SSL_GETPID(),
		ss->ssl3->hs.msg_type));
    PRINT_BUF(60, (ss, "md5 handshake hash", (unsigned char*)ss->ssl3->hs.md5, 16));

    hdr[0] = ss->ssl3->hs.msg_type;
    hdr[1] = (length >> 16) & 0xff;
    hdr[2] = (length >> 8) & 0xff;
    hdr[3] = (length) & 0xff;

    /* Start new handshake hashes when we start a new handshake */
    if (ss->ssl3->hs.msg_type == client_hello) {
	/*
	 * OK, we really should use PKCS #11 for the whole thing, but
 	 * we do screwy things here, like statically allocate the SHA1 and
	 * MD5 contexts, so we just make sure it's safe before we call
	 * the sha/md5 stuff....
	 */
	if (!PK11_HashOK(SEC_OID_MD5)) {
            return SECFailure;
	}
	if (!PK11_HashOK(SEC_OID_SHA1)) {
            return SECFailure;
	}
	MD5_Begin(ss->ssl3->hs.md5);
	SHA1_Begin(ss->ssl3->hs.sha);
    }
    /* We should not include hello_request messages in the handshake hashes */
    if (ss->ssl3->hs.msg_type != hello_request) {
	SSL3_UpdateHandshakeHashes(ss->ssl3, (unsigned char*) hdr, 4);
	SSL3_UpdateHandshakeHashes(ss->ssl3, b, length);
    }
    switch (ss->ssl3->hs.msg_type) {
    case hello_request:
	if (length != 0 || ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleHelloRequest(ss);
	break;
    case client_hello:
	if (!ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleClientHello(ss, b, length);
	break;
    case server_hello:
	if (ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleServerHello(ss, b, length);
	break;
    case certificate:
	rv = SSL3_HandleCertificate(ss, b, length);
	break;
    case server_key_exchange:
	if (ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleServerKeyExchange(ss, b, length);
	break;
    case certificate_request:
	if (ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleCertificateRequest(ss, b, length);
	break;
    case server_hello_done:
	if (length != 0 || ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleServerHelloDone(ss);
	break;
    case certificate_verify:
	if (!ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = ssl3_HandleCertificateVerify(ss, b, length, &hashes);
	break;
    case client_key_exchange:
	if (!ss->sec->isServer)
	    return ssl3_HandshakeFailure(ss);
	rv = SSL3_HandleClientKeyExchange(ss, b, length);
	break;
    case finished:
        rv = SSL3_HandleFinished(ss, b, length, &hashes);
	break;
    default:
	return ssl3_HandshakeFailure(ss);
    }
    return rv;
}

SECStatus
SSL3_HandleHandshake(SSLSocket *ss, SSLBuffer *origBuf)
{
    /*
     * There may be a partial handshake message already in the handshake
     * state. The incoming buffer may contain another portion, or a
     * complete message or several messages followed by another portion.
     *
     * Each message is made contiguous before being passed to the actual
     * message parser.
     */
    SSL3State *ssl3 = ss->ssl3;
    SSLBuffer *buf = &ssl3->hs.msgState; /* do not lose the original buffer pointer */
    SECStatus rv;

    if (buf->buf == NULL) {
	*buf = *origBuf;
    }
    while (buf->len > 0) {
	while (ssl3->hs.header_bytes < 4) {
	    uint8 t;
	    t = *(buf->buf++);
	    buf->len--;
	    if (ssl3->hs.header_bytes++ == 0)
		ssl3->hs.msg_type = t;
	    else
		ssl3->hs.msg_len = (ssl3->hs.msg_len << 8) + t;
	    if (ssl3->hs.header_bytes == 4) {
		/*
		 * XXX For now we do not support anybody who sends a
		 * handshake message that is more than 32K bytes. It
		 * should only happen with grotesquely sized certificate
		 * chains which we should not see in the near future. The
		 * guard is against 16 bit windows implementations that
		 * would now have to deal with potentially large blocks.
		 */
		if (ssl3->hs.msg_len > 0x7fff)
		    return ssl3_HandshakeFailure(ss);
	    }
	    if (buf->len == 0 && ssl3->hs.msg_len > 0) {
		buf->buf = NULL;
		return SECSuccess;
	    }
	}

	/*
	 * Header has been gathered and there is at least one byte of new
	 * data available for this message. If it can be done right out
	 * of the original buffer, then use it from there.
	 */
	if (ssl3->hs.msg_body.len == 0 && buf->len >= ssl3->hs.msg_len) {
	    /* handle it from input buffer */
	    rv = ssl3_HandleHandshakeMessage(ss, buf->buf, ssl3->hs.msg_len);
	    if (rv == SECFailure) return rv;
	    buf->buf += ssl3->hs.msg_len;
	    buf->len -= ssl3->hs.msg_len;
	    ssl3->hs.msg_len = 0;
	    ssl3->hs.header_bytes = 0;
	    if (rv < 0) return rv;
	} else {
	    /* must be copied to msg_body and dealt with from there */
	    long bytes;
	    if (buf->len < ssl3->hs.msg_len)
		bytes = buf->len;
	    else
		bytes = buf->len;
	    /* Grow the buffer if needed */
	    if (bytes > ssl3->hs.msg_body.space - ssl3->hs.msg_body.len) {
		rv = ssl_GrowBuf(
		    &ssl3->hs.msg_body, ssl3->hs.msg_body.len + bytes);
		if (rv < 0) return SECFailure;
	    }
	    PORT_Memcpy(ssl3->hs.msg_body.buf + ssl3->hs.msg_body.len,
		      buf->buf, buf->len);
	    buf->buf += bytes;
	    buf->len -= bytes;
	    /* should not be more that a message in msg_body */
	    PORT_Assert(ssl3->hs.msg_body.len <= ssl3->hs.msg_len);
	    /* if we have a whole message, do it */
	    if (ssl3->hs.msg_body.len == ssl3->hs.msg_len) {
		rv = ssl3_HandleHandshakeMessage(
		    ss, ssl3->hs.msg_body.buf, ssl3->hs.msg_len);
		if (rv < 0) return rv;
		ssl3->hs.msg_body.len = 0;
		ssl3->hs.msg_len = 0;
		ssl3->hs.header_bytes = 0;
	    } else {
		PORT_Assert(buf->len == 0);
		break;
	    }
	}
	
    }
    origBuf->len = 0;
    buf->buf = NULL;
    return SECSuccess;
}


SECStatus
SSL3_HandleRecord(SSLSocket *ss, SSL3Ciphertext *cipher, SSLBuffer *databuf)
{
    long rv;
    SSL3State *ssl3 = ss->ssl3;
    SSL3CipherSpec *spec;
    SSL3Opaque hash[MAX_MAC_LENGTH];
    unsigned int hashBytes;
    unsigned int padding;
    SSL3AlertDescription alert = unexpected_message;
    const SSL3BulkCipherDef *cipher_def;

    if(ssl3 == NULL) {
	rv = ssl3_InitState(ss);
	if (rv < 0) return rv;
    }

    ssl3 = ss->ssl3;
    spec = ssl3->current_read;
    cipher_def = spec->cipher_def;

    if (cipher == NULL) {
	SSL_DBG(("%d: SSL3[%d]: HandleRecord, resuming handshake",
		 SSL_GETPID(), ss->fd));
	rv = SSL3_HandleHandshake(ss, databuf);
	return rv;
    }

    databuf->len = 0;
    if (databuf->space < MAX_FRAGMENT_LENGTH) {
	rv = ssl_GrowBuf(databuf, MAX_FRAGMENT_LENGTH + 2048);
	if (rv < 0) {
	    SSL_DBG(("%d: SSL3[%d]: HandleRecord, tried to get %d bytes",
		     SSL_GETPID(), ss->fd, MAX_FRAGMENT_LENGTH + 2048));
	    return SECFailure;
	}
    }

    PRINT_BUF(80, (ss, "ciphertext:", cipher->buf->buf, cipher->buf->len));

    rv = spec->decode(
	spec->decodeContext, databuf->buf, (unsigned *)&databuf->len,
	databuf->space, cipher->buf->buf, cipher->buf->len);

    PRINT_BUF(80, (ss, "cleartext:", databuf->buf, databuf->len));
    if (rv < 0) return SECFailure;

    if (cipher_def->type == type_block) {
	padding = *(databuf->buf + databuf->len - 1);
	if (padding >= cipher_def->block_size) goto bad_msg;
	databuf->len -= padding + 1;
	if (databuf->len <= 0) goto bad_msg;
    }
    
    databuf->len -= spec->mac_size;
    SSL3_ComputeFragmentHash(
	spec, (ss->sec->isServer) ?
	    spec->client.write_mac_secret : spec->server.write_mac_secret,
	cipher->type, spec->read_seq_num, databuf->buf, databuf->len,
	hash, &hashBytes);

    if (hashBytes != spec->mac_size ||
	PORT_Memcmp(databuf->buf + databuf->len, hash, spec->mac_size) != 0) {
	SSL_DBG(("%d: SSL3[%d]: mac check failed",
		 SSL_GETPID(), ss->fd));
	PORT_SetError(SSL_ERROR_BAD_MAC_READ);
	SSL3_SendAlert(ss, alert_fatal, bad_record_mac);
	return SECFailure;
    }

    ssl3_BumpSequenceNumber(&spec->read_seq_num);

    /*
     * the null decompression routine is right here
     */

    switch (cipher->type) {
    case content_change_cipher_spec:
	rv = SSL3_HandleChangeCipherSpecs(ss, databuf);
	break;
    case content_alert:
	rv = SSL3_HandleAlert(ss, databuf);
	break;
    case content_handshake:
	rv = SSL3_HandleHandshake(ss, databuf);
	break;
    case content_application_data:
	break;
    default:
	SSL_DBG(("%d: SSL3[%d]: bogus content type=%d",
		 SSL_GETPID(), ss->fd, cipher->type));
	return SECFailure;
    }
	
    return rv;
    
bad_msg:
    PORT_SetError(XP_ERRNO_EIO);
    SSL3_SendAlert(ss, alert_fatal, alert);
    return SECFailure;
}

/*
 * Initialization functions
 */

static void
ssl3_InitCipherSpec(SSL3CipherSpec *s)
{
    s->cipher_def = &bulk_cipher_defs[cipher_null];
    PORT_Assert(s->cipher_def->cipher == cipher_null);
    s->mac_def = &mac_defs[mac_null];
    PORT_Assert(s->mac_def->alg == mac_null);
    s->hashContext = NULL;
    s->hash = &SECHashObjects[HASH_AlgNULL];
    s->encode = Null_Cipher;
    s->decode = Null_Cipher;
    s->destroy = NULL;
    s->mac_size = 0;
    s->write_seq_num.high = s->write_seq_num.low = 0;
    s->read_seq_num.high = s->read_seq_num.low = 0;
}

static SECStatus
ssl3_InitState(SSLSocket *ss)
{
    SSL3State *ssl3 = NULL;
    MD5Context *md5 = NULL;
    SHA1Context *sha = NULL;

    /* reinitialization for renegotiated sessions XXX */
    if (ss->ssl3 != NULL) return 0;

    ssl3 = (SSL3State*) PORT_ZAlloc(sizeof(SSL3State)); /* zero on purpose */
    if (ssl3 == NULL) return SECFailure;

    /* note that entire HandshakeState is zero, including the buffer */
    ssl3->policy = SSL_ALLOWED;
    ssl3->current_read = ssl3->current_write = &ssl3->specs[0];
    ssl3->pending_read = ssl3->pending_write = &ssl3->specs[1];
    ssl3->hs.rehandshake = PR_FALSE;
    ssl3_InitCipherSpec(ssl3->current_read);
    ssl3_InitCipherSpec(ssl3->pending_read);
#ifdef FORTEZZA
    ssl3->kea_context = NULL; /* paranoia */
#endif

    ss->ssl3 = ssl3;

    if (ss->sec->isServer)
	ssl3->hs.ws = wait_client_hello;
    else
	ssl3->hs.ws = wait_server_hello;

    /*
     * OK, we really should use PKCS #11 for the whole thing, but
     * we do screwy things here, like statically allocate the SHA1 and
     * MD5 contexts, so we just make sure it's safe before we call
     * the sha/md5 stuff....
     */
    if (!PK11_HashOK(SEC_OID_MD5)) {
	goto loser;
    }
    if (!PK11_HashOK(SEC_OID_SHA1)) {
        goto loser;
    }
    md5 = ssl3->hs.md5 = MD5_NewContext();
    if (md5 == NULL) goto loser;
    MD5_Begin(ssl3->hs.md5);
    sha = ssl3->hs.sha = SHA1_NewContext();
    if (sha == NULL) goto loser;
    SHA1_Begin(ssl3->hs.sha);
    return SECSuccess;

loser:
    if (md5 != NULL) MD5_DestroyContext(md5, PR_TRUE);
    if (sha != NULL) SHA1_DestroyContext(sha, PR_TRUE);
    if (ssl3 != NULL) PORT_Free(ssl3);
    return SECFailure;
}

void
SSL3_Init(void)
{
}

void
SSL3_CreateExportRSAKeys(SECKEYPrivateKey *server_key)
{
    usKey = server_key;
    /* Sigh, should have a get key strength call */
    if (PK11_GetPrivateModulusLen(server_key) > EXPORT_RSA_KEY_LENGTH) {
	/* need to ask for the key size in bits */
	exportKey = SECKEY_CreateRSAPrivateKey(EXPORT_RSA_KEY_LENGTH * 8,
						&exportPubKey, NULL);
    } else {
	exportKey = NULL;
    }
}

#ifdef FORTEZZA
void
SSL3_SetFortezzaKeys(SECKEYPrivateKey *server_key)
{
    PORT_Assert(server_key->keyType == fortezzaKeys);
    fortezzaServerKey = server_key;
}
#endif

/* record the export policy for this cipher suite */
SECStatus
SSL3_SetPolicy(SSL3CipherSuite which, int policy)
{
    SSL3CipherSuiteCfg *suite;

    if (policy == SSL_RESTRICTED) {
	policy_some_restricted = PR_TRUE;
    }
    
    suite = ssl_LookupCipherSuiteCfg(which);
    if (suite == NULL)
	return SECFailure;

    suite->policy = policy;
    return SECSuccess;
}

/* record the user preference for this suite */
SECStatus
SSL3_EnableCipher(SSL3CipherSuite which, int enabled)
{
    SSL3CipherSuiteCfg *suite;

    suite = ssl_LookupCipherSuiteCfg(which);
    if (suite == NULL)
	return SECFailure;

    suite->enabled = enabled;
    return SECSuccess;
}

SECStatus
SSL3_ConstructV2CipherSpecsHack(SSLSocket *ss, unsigned char *cs, int *size)
{
    int i, count = 0;

    if (cs == NULL) {
	*size = count_cipher_suites(SSL_ALLOWED, PR_TRUE);
	return SECSuccess;
    }

#ifdef FORTEZZA
    if (ss) {
        SECStatus rv;
	rv = ssl3_InitState(ss);
	if (rv < 0) return rv;

    	if (SSL_IsEnabledGroup(SSL_GroupFortezza)) {
	    rv=FortezzaOpen(ss,(FortezzaKEAContext **)&ss->ssl3->kea_context);
	    switch (rv) {
	    case SECSuccess:
		break;
	    case SECFailure:
	    	ssl_TmpEnableGroup(&localCipherSuites,&localCipherSuiteCount,
						SSL_GroupFortezza,PR_FALSE);
		break;
	    default:
	        ss->reStartType = 2;
		return(rv); /* SECWouldBlock.. waiting for user input */
	    }
	}
    } else {
	/* if we're this far into the SSL2 exchange, we can't do FORTEZZA */
	ssl_TmpEnableGroup(&localCipherSuites,&localCipherSuiteCount,
						SSL_GroupFortezza,PR_FALSE);
    }
#endif


    for(i = 0; i < cipherSuiteCount; i++) {
	if (config_match(&cipherSuites[i], SSL_ALLOWED, PR_TRUE)) {
	    if (cs != NULL) {
		*cs++ = 0x00;
		*cs++ = (cipherSuites[i].cipher_suite >> 8) & 0xFF;
		*cs++ = cipherSuites[i].cipher_suite & 0xFF;
	    }
	    count++;
	}
    }
    *size = count;
    return SECSuccess;
}

SECStatus
SSL3_RedoHandshake(SSLSocket *ss)
{
    SSLSecurityInfo *sec = ss->sec;
    SSLSessionID *sid = ss->sec->ci->sid;
    
    if (!ss->connected)
	return SECFailure;

    if (sid) {
	sec->uncache(sid);
	ssl_FreeSID(sid);
	ss->sec->ci->sid = NULL;
    }
    
    if (sec->isServer) {
	return SSL3_SendHelloRequest(ss);
    } else {
	return SSL3_SendClientHello(ss);
    }
}

void
ssl3_DestroySSL3Info(SSL3State *ssl3)
{
    if (ssl3 == NULL)
	return;
    
    if (ssl3->clientCertificate != NULL)
	CERT_DestroyCertificate(ssl3->clientCertificate);

    if (ssl3->clientPrivateKey != NULL)
	SECKEY_DestroyPrivateKey(ssl3->clientPrivateKey);
    
    if (ssl3->peerCertArena != NULL)
	SSL3_CleanupPeerCerts(ssl3);

#ifdef FORTEZZA
    if (ssl3->kea_context) {
	FortezzaDestroyKEAContext(ssl3->kea_context,PR_TRUE);
    }
#endif

    /* clean up handshake */
    if (ssl3->hs.md5) {
	MD5_DestroyContext(ssl3->hs.md5,PR_TRUE);
    }
    if (ssl3->hs.sha) {
	SHA1_DestroyContext(ssl3->hs.sha,PR_TRUE);
    }
    /* free the SSL3Buffer (msg_body) */
    PORT_FreeBlock(ssl3->hs.msg_body.buf);

    /* free up the CipherSpecs */
    ssl3_DestroyCipherSpec(&ssl3->specs[0]);
    ssl3_DestroyCipherSpec(&ssl3->specs[1]);

    PORT_Free(ssl3);
}

#ifdef FORTEZZA
int
SSL_IsEnabledGroup(int which) {
    int i;
    long mask;
    SSL3KeyExchangeAlgorithm kea;
    const SSL3CipherSuiteDef *suite_def;

    PORT_Assert(cipherSuiteCount <= 32);
    for(i = 0; i < cipherSuiteCount; i++) {
	mask = 1L << i;

	suite_def = ssl_LookupCipherSuiteDef(i);
	if (suite_def == NULL) continue;
	kea = suite_def->key_exchange_algorithm;

	if (((kea == kea_rsa) && (which & SSL_GroupRSA)) ||
	   ((kea == kea_dh) && (which & SSL_GroupDiffieHellman)) ||
	       ((kea == kea_fortezza) && (which & SSL_GroupFortezza))) {
	    if ((enabledCipherSuites & mask) != 0) {
		return PR_TRUE;
	    }
	}
    }
    return PR_FALSE;
}

int
SSL_EnableGroup(int which,int on) {
    int i;
    long mask;
    SSL3KeyExchangeAlgorithm kea;
    const SSL3CipherSuiteDef *suite_def;

    PORT_Assert(cipherSuiteCount <= 32);
    for(i = 0; i < cipherSuiteCount; i++) {
	mask = 1L << i;

	suite_def = ssl_LookupCipherSuiteDef(i);
	if (suite_def == NULL) continue;
	kea = suite_def->key_exchange_algorithm;

	if (((kea == kea_rsa) && (which & SSL_GroupRSA)) ||
	   ((kea == kea_dh) && (which & SSL_GroupDiffieHellman)) ||
	       ((kea == kea_fortezza) && (which & SSL_GroupFortezza))) {
	    if (on) {
		if ((enabledCipherSuites & mask) == 0) {
		    enabledCipherSuiteCount++;
		    enabledCipherSuites |= mask;
		}
	    } else {
		if ((enabledCipherSuites & mask) != 0) {
		    enabledCipherSuiteCount--;
		    enabledCipherSuites &= ~mask;
		}
	    }
	    return SECSuccess;
	}
    }
    return SECFailure;
}
#endif
