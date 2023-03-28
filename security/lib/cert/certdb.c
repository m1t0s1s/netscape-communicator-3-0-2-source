/*
 * Certificate handling code
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 */

#include "prtime.h"
#include "xp_reg.h"
#include "cert.h"
#include "secder.h"
#include "crypto.h"
#include "key.h"
#include "secitem.h"
#include "mcom_db.h"
#include "certdb.h"
#include "prprf.h"
#include "sechash.h"
#include "prlong.h"
#include "certxutl.h"

#if defined(SERVER_BUILD)
#define dbopen NS_dbopen
#endif

extern int SEC_ERROR_BAD_DATABASE;
extern int SEC_ERROR_BAD_DER;
extern int SEC_ERROR_BAD_SIGNATURE;
extern int SEC_ERROR_CA_CERT_INVALID;
extern int SEC_ERROR_CERT_NOT_VALID;
extern int SEC_ERROR_CERT_USAGES_INVALID;
extern int SEC_ERROR_CRL_BAD_SIGNATURE;
extern int SEC_ERROR_CRL_EXPIRED;
extern int SEC_ERROR_EXTENSION_NOT_FOUND;
extern int SEC_ERROR_EXPIRED_CERTIFICATE;
extern int SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE;
extern int SEC_ERROR_KRL_BAD_SIGNATURE;
extern int SEC_ERROR_KRL_EXPIRED;
extern int SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID;
extern int SEC_ERROR_REVOKED_CERTIFICATE;
extern int SEC_ERROR_REVOKED_KEY;
extern int SEC_ERROR_NO_KRL;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION;
extern int SEC_ERROR_UNKNOWN_ISSUER;
extern int SEC_ERROR_UNTRUSTED_CERT;
extern int SEC_ERROR_UNTRUSTED_ISSUER;

extern int SSL_ERROR_BAD_CERT_DOMAIN;
extern int SSL_ERROR_NO_CERTIFICATE;
extern int SEC_ERROR_INADEQUATE_KEY_USAGE;
extern int SEC_ERROR_INADEQUATE_CERT_TYPE;
extern int SEC_ERROR_CA_CERT_INVALID;

/*
 * Certificate database handling code
 */

DERTemplate CERTCertExtensionTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTCertExtension) },
    { DER_OBJECT_ID,
	  offsetof(CERTCertExtension,id) },
    { DER_OPTIONAL | DER_BOOLEAN,		/* XXX DER_DEFAULT */
	  offsetof(CERTCertExtension,critical), },
    { DER_OCTET_STRING,
	  offsetof(CERTCertExtension,value) },
    { 0, }
};



DERTemplate CERTCertificateTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTCertificate) },
    { DER_EXPLICIT | DER_OPTIONAL | DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 0,
						/* XXX DER_DEFAULT */
	  offsetof(CERTCertificate,version),
	  NULL, DER_INTEGER },
    { DER_INTEGER,
	  offsetof(CERTCertificate,serialNumber), },
    { DER_INLINE,
	  offsetof(CERTCertificate,signature),
	  SECAlgorithmIDTemplate, },
    { DER_DERPTR | DER_OUTER,
	  offsetof(CERTCertificate,derIssuer), },
    { DER_INLINE,
	  offsetof(CERTCertificate,issuer),
	  CERTNameTemplate, },
    { DER_INLINE,
	  offsetof(CERTCertificate,validity),
	  CERTValidityTemplate, },
    { DER_DERPTR | DER_OUTER,
	  offsetof(CERTCertificate,derSubject), },
    { DER_INLINE,
	  offsetof(CERTCertificate,subject),
	  CERTNameTemplate, },
    { DER_DERPTR | DER_OUTER,
	  offsetof(CERTCertificate,derPublicKey), },
    { DER_INLINE,
	  offsetof(CERTCertificate,subjectPublicKeyInfo),
	  CERTSubjectPublicKeyInfoTemplate, },
    { DER_OPTIONAL | DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 1,
	  offsetof(CERTCertificate,issuerID),
	  NULL, DER_OBJECT_ID },
    { DER_OPTIONAL | DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 2,
	  offsetof(CERTCertificate,subjectID),
	  NULL, DER_OBJECT_ID },
    { DER_EXPLICIT | DER_OPTIONAL | DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 3,
	  offsetof(CERTCertificate,extensions),
	  CERTCertExtensionTemplate, DER_INDEFINITE | DER_SEQUENCE },
    { 0, }
};

DERTemplate SECSignedCertificateTemplate[] =
{
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTCertificate) },
    { DER_DERPTR | DER_OUTER,
	  offsetof(CERTCertificate,signatureWrap.data), },
    { DER_INLINE,
	  0, CERTCertificateTemplate },
    { DER_INLINE,
	  offsetof(CERTCertificate,signatureWrap.signatureAlgorithm),
	  SECAlgorithmIDTemplate, },
    { DER_BIT_STRING,
	  offsetof(CERTCertificate,signatureWrap.signature), },
    { 0, }
};

/*
 * Find the subjectName in a DER encoded certificate
 */
DERTemplate SECCertSubjectTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(SECItem) },
    { DER_EXPLICIT | DER_OPTIONAL | DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 0,
	  0, NULL, DER_SKIP },	/* version */
    { DER_SKIP, },		/* serial number */
    { DER_SKIP, },		/* signature algorithm */
    { DER_SKIP, },		/* issuer */
    { DER_SKIP, },		/* validity */
    { DER_DERPTR | DER_OUTER, 0, },		/* subject */
    { 0, }
};

/*
 * Find the issuer and serialNumber in a DER encoded certificate.
 * This data is used as the database lookup key since its the unique
 * identifier of a certificate.
 */
DERTemplate CERTCertKeyTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTCertKey) },
    { DER_EXPLICIT | DER_OPTIONAL | DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 0,
	  0, NULL, DER_SKIP },	/* version */
    { DER_INTEGER,
	  offsetof(CERTCertKey,serialNumber), },
    { DER_SKIP, },		/* signature algorithm */
    { DER_DERPTR | DER_OUTER,
	  offsetof(CERTCertKey,derIssuer) },
    { 0, }
};


/*
 * Extract the subject name from a DER certificate
 */
SECStatus
CERT_NameFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	return(SECFailure);
    }
    
    rv = DER_Decode(arena, &sd, CERTSignedDataTemplate, derCert);
    
    if ( rv ) {
	goto loser;
    }
    
    rv = DER_Decode(arena, derName, SECCertSubjectTemplate, &sd.data);

    if ( rv ) {
	goto loser;
    }

    tmpptr = derName->data;
    derName->data = PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(derName->data, tmpptr, derName->len);
    
    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}

SECStatus
CERT_KeyFromIssuerAndSN(PRArenaPool *arena, SECItem *issuer, SECItem *sn,
			SECItem *key)
{
    key->len = sn->len + issuer->len;
    
    key->data = PORT_ArenaAlloc(arena, key->len);
    if ( !key->data ) {
	goto loser;
    }

    /* copy the serialNumber */
    PORT_Memcpy(key->data, sn->data, sn->len);

    /* copy the issuer */
    PORT_Memcpy(&key->data[sn->len], issuer->data, issuer->len);

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Generate a database key, based on serial number and issuer, from a
 * DER certificate.
 */
SECStatus
CERT_KeyFromDERCert(PRArenaPool *arena, SECItem *derCert, SECItem *key)
{
    int rv;
    CERTSignedData sd;
    CERTCertKey certkey;
    
    rv = DER_Decode(arena, &sd, CERTSignedDataTemplate, derCert);
    
    if ( rv ) {
	goto loser;
    }
    
    rv = DER_Decode(arena, &certkey, CERTCertKeyTemplate, &sd.data);

    if ( rv ) {
	goto loser;
    }

    return(CERT_KeyFromIssuerAndSN(arena, &certkey.derIssuer,
				   &certkey.serialNumber, key));
loser:
    return(SECFailure);
}

/*
 * fill in keyUsage field of the cert based on the cert extension
 * if the extension is not critical, then we allow all uses
 */
SECStatus
GetKeyUsage(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem tmpitem;
    PRBool critical;
    
    rv = CERT_FindKeyUsageExtension(cert, &tmpitem);
    if ( rv == SECSuccess ) {
	rv = CERT_GetExtenCriticality(cert->extensions, SEC_OID_X509_KEY_USAGE,
				      &critical);
	if ( rv != SECSuccess ) {
	    cert->keyUsage = 0;
	    cert->rawKeyUsage = 0;
	    cert->keyUsagePresent = PR_FALSE;
	    return(SECFailure);
	}

	/* remember the actual value of the extension */
	cert->rawKeyUsage = tmpitem.data[0];
	cert->keyUsagePresent = PR_TRUE;
	if ( critical ) {
	    cert->keyUsage = tmpitem.data[0];
	} else {
	    /* if it is not critical, then we allow all uses. */
	    cert->keyUsage = KU_ALL;
	}
    } else {
	/* if the extension is not present, then we allow all uses */
	cert->keyUsage = KU_ALL;
	cert->rawKeyUsage = KU_ALL;
	cert->keyUsagePresent = PR_FALSE;
    }

    if ( CERT_GovtApprovedBitSet(cert) ) {
	cert->keyUsage |= KU_NS_GOVT_APPROVED;
	cert->rawKeyUsage |= KU_NS_GOVT_APPROVED;
    }
    
    return(SECSuccess);
}

/*
 * fill in nsCertType field of the cert based on the cert extension
 */
SECStatus
GetCertType(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem tmpitem;
    PRBool critical;
    
    rv = CERT_FindNSCertTypeExtension(cert, &tmpitem);
    if ( rv == SECSuccess ) {
	cert->nsCertType = tmpitem.data[0];

	/*
	 * for this release, we will allow SSL certs with an email address
	 * to be used for email
	 */
	if ( ( cert->nsCertType & NS_CERT_TYPE_SSL_CLIENT ) &&
	    cert->emailAddr ) {
	    cert->nsCertType |= NS_CERT_TYPE_EMAIL;
	}
	/*
	 * for this release, we will allow SSL intermediate CAs to be
	 * email intermediate CAs too.
	 */
	if ( cert->nsCertType & NS_CERT_TYPE_SSL_CA ) {
	    cert->nsCertType |= NS_CERT_TYPE_EMAIL_CA;
	}
	
    } else {
	/* if no extension, then allow any application, but no CA */
	cert->nsCertType = NS_CERT_TYPE_APP;
    }

    return(SECSuccess);
}

/*
 * GetKeyID() - extract or generate the subjectKeyID from a certificate
 */
SECStatus
GetKeyID(CERTCertificate *cert)
{
    SECItem tmpitem;
    SECStatus rv;
    
    cert->subjectKeyID.len = 0;

    /* see of the cert has a key identifier extension */
    rv = CERT_FindSubjectKeyIDExten(cert, &tmpitem);
    if ( rv == SECSuccess ) {
	cert->subjectKeyID.data = PORT_ArenaAlloc(cert->arena, tmpitem.len);
	if ( cert->subjectKeyID.data != NULL ) {
	    PORT_Memcpy(cert->subjectKeyID.data, tmpitem.data, tmpitem.len);
	    cert->subjectKeyID.len = tmpitem.len;
	    cert->keyIDGenerated = PR_FALSE;
	}
	
	PORT_Free(tmpitem.data);
    }
    
    /* if the cert doesn't have a key identifier extension, then generate one*/
    if ( cert->subjectKeyID.len == 0 ) {
	unsigned char hashbuf[SHA1_LENGTH];
	/*
	 * pkix says that if the subjectKeyID is not present, then we should
	 * use the SHA-1 hash of the DER-encoded publicKeyInfo from the cert
	 */
	cert->subjectKeyID.data = PORT_ArenaAlloc(cert->arena, SHA1_LENGTH);
	if ( cert->subjectKeyID.data != NULL ) {
	    rv = SHA1_HashBuf(cert->subjectKeyID.data,
			      cert->derPublicKey.data,
			      cert->derPublicKey.len);
	    if ( rv == SECSuccess ) {
		cert->subjectKeyID.len = SHA1_LENGTH;
	    }
	}
    }

    if ( cert->subjectKeyID.len == 0 ) {
	return(SECFailure);
    }
    return(SECSuccess);

}

/*
 * take a DER certificate and decode it into a certificate structure
 */
CERTCertificate *
CERT_DecodeDERCertificate(SECItem *derSignedCert, PRBool copyDER,
			 char *nickname)
{
    CERTCertificate *cert;
    PRArenaPool *arena;
    PRBool freenn = PR_FALSE;
    void *data;
    int rv;
    int len;
    char *tmpname;
    
    /* make a new arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return 0;
    }

    /* allocate the certificate structure */
    cert = (CERTCertificate *)PORT_ArenaZAlloc(arena, sizeof(CERTCertificate));
    
    if ( !cert ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    cert->arena = arena;
    
    if ( copyDER ) {
	/* copy the DER data for the cert into this arena */
	data = (void *)PORT_ArenaAlloc(arena, derSignedCert->len);
	if ( !data ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	cert->derCert.data = (unsigned char *)data;
	cert->derCert.len = derSignedCert->len;
	PORT_Memcpy(data, derSignedCert->data, derSignedCert->len);
    } else {
	/* point to passed in DER data */
	cert->derCert = *derSignedCert;
    }

    /* decode the certificate info */
    rv = DER_Decode(arena, cert, SECSignedCertificateTemplate,
		    &cert->derCert);

    if ( rv ) {
	goto loser;
    }

    if (cert_HasUnknownCriticalExten (cert->extensions) == PR_TRUE) {
	PORT_SetError(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION);
	goto loser;
    }

    /* generate and save the database key for the cert */
    rv = CERT_KeyFromDERCert(arena, &cert->derCert, &cert->certKey);
    if ( rv ) {
	goto loser;
    }

    /* set the nickname */
    if ( nickname == NULL ) {
	cert->nickname = NULL;
    } else {
	/* copy and install the nickname */
	len = PORT_Strlen(nickname) + 1;
	cert->nickname = PORT_ArenaAlloc(arena, len);
	if ( cert->nickname == NULL ) {
	    goto loser;
	}

	PORT_Memcpy(cert->nickname, nickname, len);
    }

    /* set the email address */
    cert->emailAddr = CERT_GetCertEmailAddress(&cert->subject);
    
    /* initialize the subjectKeyID */
    rv = GetKeyID(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* initialize keyUsage */
    rv = GetKeyUsage(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* initialize the certType */
    rv = GetCertType(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    tmpname = CERT_NameToAscii(&cert->subject);
    if ( tmpname != NULL ) {
	cert->subjectName = PORT_ArenaStrdup(cert->arena, tmpname);
	PORT_Free(tmpname);
    }
    
    tmpname = CERT_NameToAscii(&cert->issuer);
    if ( tmpname != NULL ) {
	cert->issuerName = PORT_ArenaStrdup(cert->arena, tmpname);
	PORT_Free(tmpname);
    }
    
    cert->referenceCount = 1;
    cert->slot = NULL;
    cert->pkcs11ID = CK_INVALID_KEY;
    cert->dbnickname = NULL;
    
    return(cert);
    
loser:

    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(0);
}

/*
** Amount of time that a certifiate is allowed good before it is actually
** good. This is used for pending certificates, ones that are about to be
** valid. The slop is designed to allow for some variance in the clocks
** of the machine checking the certificate.
*/
#define PENDING_SLOP (24L*60L*60L)

SECStatus
CERT_GetCertTimes(CERTCertificate *c, int64 *notBefore, int64 *notAfter)
{
    int rv;
    
    /* convert DER not-before time */
    rv = DER_UTCTimeToTime(notBefore, &c->validity.notBefore);
    if (rv) {
	return(SECFailure);
    }
    
    /* convert DER not-after time */
    rv = DER_UTCTimeToTime(notAfter, &c->validity.notAfter);
    if (rv) {
	return(SECFailure);
    }

    return(SECSuccess);
}

/*
 * Check the validity times of a certificate
 */
SECCertTimeValidity
CERT_CheckCertValidTimes(CERTCertificate *c, int64 t)
{
    int64 notBefore, notAfter, pendingSlop;
    SECStatus rv;

#ifdef NOTDEF /* do we really want to do this here? */    
    /* if cert is already marked OK, then don't bother to check */
    if ( c->timeOK ) {
	return(SECSuccess);
    }
#endif
    
    rv = CERT_GetCertTimes(c, &notBefore, &notAfter);
    
    if (rv) {
	return(secCertTimeExpired); /*XXX is this the right thing to do here?*/
    }
    
    LL_I2L(pendingSlop, PENDING_SLOP);
    LL_SUB(notBefore, notBefore, pendingSlop);
    if ( LL_CMP( t, <, notBefore ) ) {
	PORT_SetError(SEC_ERROR_EXPIRED_CERTIFICATE);
	return(secCertTimeNotValidYet);
    }
    if ( LL_CMP( t, >, notAfter) ) {
	PORT_SetError(SEC_ERROR_EXPIRED_CERTIFICATE);
	return(secCertTimeExpired);
    }

    return(secCertTimeValid);
}

SECStatus
SEC_GetCrlTimes(CERTCrl *date, int64 *notBefore, int64 *notAfter)
{
    int rv;
    
    /* convert DER not-before time */
    rv = DER_UTCTimeToTime(notBefore, &date->lastUpdate);
    if (rv) {
	return(SECFailure);
    }
    
    /* convert DER not-after time */
    if (date->nextUpdate.data) {
	rv = DER_UTCTimeToTime(notAfter, &date->nextUpdate);
	if (rv) {
	    return(SECFailure);
	}
    }
    else {
	LL_I2L(*notAfter, 0L);
    }
    return(SECSuccess);
}

/* These routines should probably be combined with the cert
 * routines using an common extraction routine.
 */
SECCertTimeValidity
SEC_CheckCrlTimes(CERTCrl *crl, int64 t) {
    int64 notBefore, notAfter, pendingSlop;
    SECStatus rv;

    rv = SEC_GetCrlTimes(crl, &notBefore, &notAfter);
    
    if (rv) {
	return(secCertTimeExpired); 
    }

    LL_I2L(pendingSlop, PENDING_SLOP);
    LL_SUB(notBefore, notBefore, pendingSlop);
    if ( LL_CMP( t, <, notBefore ) ) {
	return(secCertTimeNotValidYet);
    }

    /* If next update is omitted and the test for notBefore passes, then
       we assume that the crl is up to date.
     */
    if ( LL_IS_ZERO(notAfter) ) {
	return(secCertTimeValid);
    }

    if ( LL_CMP( t, >, notAfter) ) {
	return(secCertTimeExpired);
    }

    return(secCertTimeValid);
}

PRBool
SEC_CrlIsNewer(CERTCrl *new, CERTCrl *old) {
    int64 newNotBefore, newNotAfter;
    int64 oldNotBefore, oldNotAfter;
    SECStatus rv;

    /* problems with the new CRL? reject it */
    rv = SEC_GetCrlTimes(new, &newNotBefore, &newNotAfter);
    if (rv) return PR_FALSE;

    /* problems with the old CRL? replace it */
    rv = SEC_GetCrlTimes(old, &oldNotBefore, &oldNotAfter);
    if (rv) return PR_TRUE;

    /* Question: what about the notAfter's? */
    return (LL_CMP(oldNotBefore, <, newNotBefore));
}
   

/*
 * WARNING - this function is depricated, and will either go away or have
 *		a new API in the near future.
 *
 * Check the validity times of a certificate
 */
SECStatus
CERT_CertTimesValid(CERTCertificate *c)
{
    int64 now, notBefore, notAfter, pendingSlop;
    SECStatus rv;
    
    /* if cert is already marked OK, then don't bother to check */
    if ( c->timeOK ) {
	return(SECSuccess);
    }
    
    /* get current UTC time */
    now = PR_Now();
    rv = CERT_GetCertTimes(c, &notBefore, &notAfter);
    
    if (rv) {
	return(SECFailure);
    }
    
    LL_I2L(pendingSlop, PENDING_SLOP);
    LL_SUB(notBefore, notBefore, pendingSlop);

    if (LL_CMP(now, <, notBefore) || LL_CMP(now, >, notAfter)) {
	PORT_SetError(SEC_ERROR_EXPIRED_CERTIFICATE);
	return(SECFailure);
    }

    return(SECSuccess);
}

/*
 * verify the signature of a signed data object with the given certificate
 */
SECStatus
CERT_VerifySignedData(CERTSignedData *sd, CERTCertificate *cert,
		      int64 t, void *wincx)
{
    SECItem sig;
    SECKEYPublicKey *pubKey = 0;
    SECStatus rv;
    SECCertTimeValidity validity;
    SECOidTag algid;

    /* check the certificate's validity */
    validity = CERT_CheckCertValidTimes(cert, t);
    if ( validity != secCertTimeValid ) {
	return(SECFailure);
    }

    /* get cert's public key */
    pubKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if ( !pubKey ) {
	return(SECFailure);
    }
    
    /* check the signature */
    sig = sd->signature;
    DER_ConvertBitString(&sig);

    algid = SECOID_GetAlgorithmTag(&sd->signatureAlgorithm);
    /* ANSI X9.57 specifies DSA signatures as DER encoded data */
    if (algid == SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST) {
	rv = DSAU_VerifyDerSignature(sd->data.data, sd->data.len, pubKey,
				     &sig, wincx);
    } else {
	rv = VFY_VerifyData(sd->data.data, sd->data.len, pubKey, &sig, wincx);
    }

    SECKEY_DestroyPublicKey(pubKey);

    if ( rv ) {
	return(SECFailure);
    }

    return(SECSuccess);
}

#define SEC_MAX_CERT_CHAIN 20

/*
 * This must only be called on a cert that is known to have an issuer
 * with an invalid time
 */
CERTCertificate *
CERT_FindExpiredIssuer(CERTCertDBHandle *handle, CERTCertificate *cert)
{
    SECStatus rv;
    CERTCertificate *issuerCert = NULL;
    CERTCertificate *subjectCert;
    
    subjectCert = CERT_DupCertificate(cert);
    if ( subjectCert == NULL ) {
	goto loser;
    }
    
    while ( 1 ) {
	/* find the certificate of the issuer */
	issuerCert = CERT_FindCertByName(handle, &subjectCert->derIssuer);
    
	if ( ! issuerCert ) {
	    goto loser;
	}

	rv = CERT_CertTimesValid(issuerCert);
	if ( rv == SECFailure ) {
	    /* this is the invalid issuer */
	    CERT_DestroyCertificate(subjectCert);
	    return(issuerCert);
	}

	CERT_DestroyCertificate(subjectCert);
	subjectCert = issuerCert;
    }

loser:
    if ( issuerCert ) {
	CERT_DestroyCertificate(issuerCert);
    }
    
    if ( subjectCert ) {
	CERT_DestroyCertificate(subjectCert);
    }
    
    return(NULL);
}

SECStatus
SEC_CheckKRL(CERTCertDBHandle *handle,SECKEYPublicKey *key,
	     CERTCertificate *rootCert, int64 t, void * wincx)
{
    CERTSignedCrl *crl = NULL;
    SECStatus rv = SECFailure;
    SECStatus rv2;
    CERTCrlEntry **crlEntry;
    SECCertTimeValidity validity;
    CERTCertificate *issuerCert = NULL;

    /* first look up the KRL */
    crl = SEC_FindCrlByName(handle,&rootCert->derSubject, SEC_KRL_TYPE);
    if (crl == NULL) {
	PORT_SetError(SEC_ERROR_NO_KRL);
	goto done;
    }

    /* get the issuing certificate */
    issuerCert = CERT_FindCertByName(handle, &crl->crl.derName);
    if (issuerCert == NULL) {
        PORT_SetError(SEC_ERROR_KRL_BAD_SIGNATURE);
        goto done;
    }


    /* now verify the KRL signature */
    rv2 = CERT_VerifySignedData(&crl->signatureWrap, issuerCert, t, wincx);
    if (rv2 != SECSuccess) {
	PORT_SetError(SEC_ERROR_KRL_BAD_SIGNATURE);
    	goto done;
    }

    /* Verify the date validity of the KRL */
    validity = SEC_CheckCrlTimes(&crl->crl, t);
    if (validity == secCertTimeExpired) {
	PORT_SetError(SEC_ERROR_KRL_EXPIRED);
	goto done;
    }

    /* now make sure the key in this cert is still valid */
    if (key->keyType != fortezzaKey) {
	PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
	goto done; /* This should be an assert? */
    }

    /* now make sure the key is not on the revocation list */
    for (crlEntry = crl->crl.entries; crlEntry && *crlEntry; crlEntry++) {
	if (PORT_Memcmp((*crlEntry)->serialNumber.data,
				key->u.fortezza.KMID,
				    (*crlEntry)->serialNumber.len)) {
	    PORT_SetError(SEC_ERROR_REVOKED_KEY);
	    goto done;
	}
    }
    rv = SECSuccess;

done:
    if (issuerCert) CERT_DestroyCertificate(issuerCert);
    if (crl) SEC_DestroyCrl(crl);
    return rv;
}

SECStatus
SEC_CheckCRL(CERTCertDBHandle *handle,CERTCertificate *cert,
	     CERTCertificate *caCert, int64 t, void * wincx)
{
    CERTSignedCrl *crl = NULL;
    SECStatus rv = SECSuccess;
    CERTCrlEntry **crlEntry;
    SECCertTimeValidity validity;

    /* first look up the CRL */
    crl = SEC_FindCrlByName(handle,&caCert->derSubject, SEC_CRL_TYPE);
    if (crl == NULL) {
	/* XXX for now no CRL is ok */
	goto done;
    }

    /* now verify the CRL signature */
    rv = CERT_VerifySignedData(&crl->signatureWrap, caCert, t, wincx);
    if (rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_CRL_BAD_SIGNATURE);
        rv = SECWouldBlock; /* Soft error, ask the user */
    	goto done;
    }

    /* Verify the date validity of the KRL */
    validity = SEC_CheckCrlTimes(&crl->crl,t);
    if (validity == secCertTimeExpired) {
	PORT_SetError(SEC_ERROR_CRL_EXPIRED);
        rv = SECWouldBlock; /* Soft error, ask the user */
    }

    /* now make sure the key is not on the revocation list */
    for (crlEntry = crl->crl.entries; crlEntry && *crlEntry; crlEntry++) {
	if (SECITEM_CompareItem(&(*crlEntry)->serialNumber,&cert->serialNumber) == SECEqual) {
	    PORT_SetError(SEC_ERROR_REVOKED_CERTIFICATE);
	    rv = SECFailure; /* cert is revoked */
	    goto done;
	}
    }

done:
    if (crl) SEC_DestroyCrl(crl);
    return rv;
}

/*
 * Find the issuer of a cert.  Use the authorityKeyID if it exists.
 */
CERTCertificate *
CERT_FindCertIssuer(CERTCertificate *cert)
{
    CERTAuthKeyID *authorityKeyID = NULL;  
    CERTCertificate *issuerCert = NULL;
    SECItem *caName;
    void *subjectMark = NULL;
    SECItem issuerCertKey;
    SECStatus rv;
    
    subjectMark = PORT_ArenaMark(cert->arena);
    authorityKeyID = CERT_FindAuthKeyIDExten(cert);

    if ( authorityKeyID != NULL ) {
	/* has the authority key ID extension */
	if ( authorityKeyID->keyID.data != NULL ) {
	    /* extension contains a key ID, so lookup based on it */
	    issuerCert = CERT_FindCertByKeyID(cert->dbhandle, &cert->derIssuer,
					      &authorityKeyID->keyID);
	    if ( issuerCert == NULL ) {
		PORT_SetError (SEC_ERROR_UNKNOWN_ISSUER);
		goto loser;
	    }
	    
	} else if ( authorityKeyID->authCertIssuer != NULL ) {
	    /* no key ID, so try issuer and serial number */
	    caName = CERT_GetGeneralNameByType(authorityKeyID->authCertIssuer,
					       certDirectoryName, PR_TRUE);

	    /*
	     * caName is NULL when the authCertIssuer field is not
	     * being used, or other name form is used instead.
	     * If the directoryName format and serialNumber fields are
	     * used, we use them to find the CA cert.
	     * Note:
	     *	By the time it gets here, we known for sure that if the
	     *	authCertIssuer exists, then the authCertSerialNumber
	     *	must also exists (CERT_DecodeAuthKeyID() ensures this).
	     *	We don't need to check again. 
	     */

	    if (caName != NULL) {
		rv = CERT_KeyFromIssuerAndSN(cert->arena, caName,
					     &authorityKeyID->authCertSerialNumber,
					     &issuerCertKey);
		if ( rv == SECSuccess ) {
		    issuerCert = CERT_FindCertByKey(cert->dbhandle,
						    &issuerCertKey);
		}
		
		if ( issuerCert == NULL ) {
		    PORT_SetError (SEC_ERROR_UNKNOWN_ISSUER);
		    goto loser;
		}
	    }
	}
    }
    if ( issuerCert == NULL ) {
	/* if there is not authorityKeyID, then just user the issuer */
	issuerCert = CERT_FindCertByName(cert->dbhandle, &cert->derIssuer);
    }

loser:
    if (subjectMark != NULL) {
	PORT_ArenaRelease(cert->arena, subjectMark);
	subjectMark = NULL;
    }

    return(issuerCert);
}

static void
AddToVerifyLog(CERTVerifyLog *log, CERTCertificate *cert, unsigned long error,
	       unsigned int depth, void *arg)
{
    CERTVerifyLogNode *node, *tnode;

    PORT_Assert(log != NULL);
    
    node = (CERTVerifyLogNode *)PORT_ArenaAlloc(log->arena,
						sizeof(CERTVerifyLogNode));
    if ( node != NULL ) {
	node->cert = CERT_DupCertificate(cert);
	node->error = error;
	node->depth = depth;
	node->arg = arg;
	
	if ( log->tail == NULL ) {
	    /* empty list */
	    log->head = log->tail = node;
	    node->prev = NULL;
	    node->next = NULL;
	} else if ( depth >= log->tail->depth ) {
	    /* add to tail */
	    node->prev = log->tail;
	    log->tail->next = node;
	    log->tail = node;
	    node->next = NULL;
	} else if ( depth < log->head->depth ) {
	    /* add at head */
	    node->prev = NULL;
	    node->next = log->head;
	    log->head->prev = node;
	    log->head = node;
	} else {
	    /* add in middle */
	    tnode = log->tail;
	    while ( tnode != NULL ) {
		if ( depth >= tnode->depth ) {
		    /* insert after tnode */
		    node->prev = tnode;
		    node->next = tnode->next;
		    tnode->next->prev = node;
		    tnode->next = node;
		    break;
		}

		tnode = tnode->prev;
	    }
	}

	log->count++;
    }
    return;
}

#define EXIT_IF_NOT_LOGGING(log) \
    if ( log == NULL ) { \
	goto loser; \
    }

#define LOG_ERROR_OR_EXIT(log,cert,depth,arg) \
    if ( log != NULL ) { \
	AddToVerifyLog(log, cert, PORT_GetError(), depth, (void *)arg); \
    } else { \
	goto loser; \
    }

#define LOG_ERROR(log,cert,depth,arg) \
    if ( log != NULL ) { \
	AddToVerifyLog(log, cert, PORT_GetError(), depth, (void *)arg); \
    }

SECStatus
CERT_VerifyCertChain(CERTCertDBHandle *handle, CERTCertificate *cert,
		     PRBool checkSig, SECCertUsage certUsage, int64 t,
		     void *wincx, CERTVerifyLog *log)
{
    SECTrustType trustType;
    CERTBasicConstraints basicConstraint;
    CERTCertificate *issuerCert = NULL;
    CERTCertificate *subjectCert;
    PRBool isca;
    PRBool isFortezza = PR_FALSE;
    SECStatus rv;
    SECComparison rvCompare;
    SECStatus rvFinal = SECSuccess;
    int count;
    int currentPathLen = -1;
    int flags;
    unsigned int caCertType;
    unsigned int requiredCAKeyUsage;
    unsigned int requiredFlags;
    
    enum { cbd_None, cbd_User, cbd_CA } last_type = cbd_None;
    SECKEYPublicKey *key;

    if ( certUsage == certUsageSSLServerWithStepUp ) {
	requiredCAKeyUsage = ( KU_KEY_CERT_SIGN | KU_NS_GOVT_APPROVED );
    } else {
	requiredCAKeyUsage = KU_KEY_CERT_SIGN;
    }
    
    switch ( certUsage ) {
      case certUsageSSLClient:
	requiredFlags = CERTDB_TRUSTED_CLIENT_CA;
	trustType = trustSSL;
	caCertType = NS_CERT_TYPE_SSL_CA;
        break;
      case certUsageSSLServer:
      case certUsageSSLCA:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustSSL;
	caCertType = NS_CERT_TYPE_SSL_CA;
        break;
      case certUsageSSLServerWithStepUp:
	requiredFlags = CERTDB_TRUSTED_CA | CERTDB_GOVT_APPROVED_CA;
	trustType = trustSSL;
	caCertType = NS_CERT_TYPE_SSL_CA;
        break;
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustEmail;
	caCertType = NS_CERT_TYPE_EMAIL_CA;
	break;
      case certUsageObjectSigner:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustObjectSigning;
	caCertType = NS_CERT_TYPE_OBJECT_SIGNING_CA;
	break;
      case certUsageVerifyCA:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = 0;
	caCertType = NS_CERT_TYPE_CA;
	break;
      default:
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredFlags = 0;
	trustType = 0;
	caCertType = 0;
    }
    
    subjectCert = CERT_DupCertificate(cert);
    if ( subjectCert == NULL ) {
	goto loser;
    }

    /* determine if the cert is fortezza. Getting the key is an easy
     * way to determine it, especially since we need to get the privillege
     * from the key anyway.
     */
    key = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);

    if (key != NULL) {
	isFortezza = (key->keyType == fortezzaKey);

	/* find out what type of cert we are starting with */
	if (isFortezza) {
	    unsigned char priv = 0;;

	    rv = SEC_CheckKRL(handle, key, NULL, t, wincx);
	    if (rv == SECFailure) {
		/* PORT_SetError is already set by SEC_Check_KRL */
		SECKEY_DestroyPublicKey(key);
		/* Bob - should we log and continue when logging? */
		goto loser;
	    }

	    if (key->u.fortezza.DSSpriviledge.len > 0) {
		priv = key->u.fortezza.DSSpriviledge.data[0];
	    }

	    last_type = (priv & 0x60) ? cbd_CA : cbd_User;
	}
		
	SECKEY_DestroyPublicKey(key);
    }

    
    for ( count = 0; count < SEC_MAX_CERT_CHAIN; count++ ) {
	/* find the certificate of the issuer */
	issuerCert = CERT_FindCertIssuer(subjectCert);
	if ( ! issuerCert ) {
	    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
	    LOG_ERROR(log,subjectCert,count,0);
	    goto loser;
	}

	/* verify the signature on the cert */
	if ( checkSig ) {
	    rv = CERT_VerifySignedData(&subjectCert->signatureWrap,
				       issuerCert, t, wincx);
    
	    if ( rv != SECSuccess ) {
		if ( PORT_GetError() == SEC_ERROR_EXPIRED_CERTIFICATE ) {
		    PORT_SetError(SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE);
		    LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
		} else {
		    PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		    LOG_ERROR_OR_EXIT(log,subjectCert,count,0);
		}
	    }
	}

	/*
	 * XXX - fortezza may need error logging stuff added
	 */
	if (isFortezza) {
	    unsigned char priv = 0;

	    /* read the key */
	    key = CERT_ExtractPublicKey(&issuerCert->subjectPublicKeyInfo);

	    /* Cant' get Key? fail. */
	    if (key == NULL) {
	    	PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
		goto loser;
	    }


	    /* if the issuer is not a fortezza cert, we bail */
	    if (key->keyType != fortezzaKey) {
	    	SECKEY_DestroyPublicKey(key);
	    	PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
		goto loser;
	    }

	    /* get the privilege mask */
	    if (key->u.fortezza.DSSpriviledge.len > 0) {
		priv = key->u.fortezza.DSSpriviledge.data[0];
	    }

	    /*
	     * make sure the CA's keys are OK
	     */
	    rv = SEC_CheckKRL(handle, key, NULL, t, wincx);
	    if (rv != SECSuccess) {
	    	SECKEY_DestroyPublicKey(key);
		goto loser;
	    }

	    SECKEY_DestroyPublicKey(key);

	    switch (last_type) {
	      case cbd_User:
		/* first check for subordination */
		/*rv = FortezzaSubordinateCheck(cert,issuerCert);*/
		rv = SECSuccess;

		/* now check for issuer privilege */
		if ((rv != SECSuccess) || ((priv & 0x40) == 0)) {
		    /* bail */
	    	    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
		    goto loser;
		}
		break;
	      case cbd_CA:
		if ((priv & 0x20) == 0) {
		    /* bail */
	    	    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
		    goto loser;
		}
		break;
	      default:
		/* bail */ /* shouldn't ever happen */
	    	PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
	        goto loser;
	    }

	    last_type =  cbd_CA;
	}

	/* XXX - the error logging may need to go down into CRL stuff at some
	 * point
	 */
	/* check revoked list (issuer) */
	rv = SEC_CheckCRL(handle, subjectCert, issuerCert, t, wincx);
	if (rv == SECFailure) {
	    LOG_ERROR_OR_EXIT(log,subjectCert,count,0);
	} else if (rv == SECWouldBlock) {
	    /* We found something fishy, so we intend to issue an
	     * error to the user, but the user may wish to continue
	     * processing, in which case we better make sure nothing
	     * worse has happened... so keep cranking the loop */
	    rvFinal = SECFailure;
	    LOG_ERROR(log,subjectCert,count,0);
	}
	
	if ( issuerCert->isperm ) {
	    /*
	     * check the trust parms of the issuer
	     */
	    if ( certUsage == certUsageVerifyCA ) {
		if ( subjectCert->nsCertType & NS_CERT_TYPE_EMAIL_CA ) {
		    trustType = trustEmail;
		} else if ( subjectCert->nsCertType & NS_CERT_TYPE_SSL_CA ) {
		    trustType = trustSSL;
		} else {
		    trustType = trustObjectSigning;
		}
	    }
	    
	    flags = SEC_GET_TRUST_FLAGS(issuerCert->trust, trustType);

	    if ( flags & CERTDB_VALID_CA ) {
		if ( ( flags & requiredFlags ) == requiredFlags ) {
		    /* we found a trusted one, so return */
		    rv = rvFinal; 
		    goto done;
		}
	    }
	}

	/* If the basicConstraint extension is included in an immediate CA
	 * certificate, make sure that the isCA flag is on.  If the
	 * pathLenConstraint component exists, it must be greater than the
	 * number of CA certificates we have seen so far.  If the extension
	 * is omitted, we will assume that this is a CA certificate with
	 * an unlimited pathLenConstraint (since it already passes the
	 * netscape-cert-type extension checking).
	 */
	rv = CERT_FindBasicConstraintExten(issuerCert, &basicConstraint);
	if ( rv != SECSuccess ) {
	    if (PORT_GetError() != SEC_ERROR_EXTENSION_NOT_FOUND) {
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
	    } else {
		currentPathLen = CERT_UNLIMITED_PATH_CONSTRAINT;
	    }
	    isca = PR_FALSE;
	} else  {
	    if ( basicConstraint.isCA == PR_FALSE ) {
		PORT_SetError (SEC_ERROR_CA_CERT_INVALID);
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
	    }
	    
	    /* make sure that the path len constraint is properly set.
	     */
	    if ( basicConstraint.pathLenConstraint ==
		CERT_UNLIMITED_PATH_CONSTRAINT ) {
		currentPathLen = CERT_UNLIMITED_PATH_CONSTRAINT;
	    } else if ( currentPathLen == CERT_UNLIMITED_PATH_CONSTRAINT ) {
		/* error if the previous CA's path length constraint is
		 * unlimited but its CA's path is not.
		 */
		PORT_SetError (SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID);
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,basicConstraint.pathLenConstraint);
	    } else if (basicConstraint.pathLenConstraint > currentPathLen) {
		currentPathLen = basicConstraint.pathLenConstraint;
	    } else {
		PORT_SetError (SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID);
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,basicConstraint.pathLenConstraint);
	    }

	    isca = PR_TRUE;
	}

	/*
	 * Make sure that if this is an intermediate CA in the chain that
	 * it was given permission by its signer to be a CA.
	 */
	if ( isca ) {
	    /*
	     * if basicConstraints says it is a ca, then we check the
	     * nsCertType.  If the nsCertType has any CA bits set, then
	     * it must have the right one.
	     */
	    if ( issuerCert->nsCertType & NS_CERT_TYPE_CA ) {
		if ( issuerCert->nsCertType & caCertType ) {
		    isca = PR_TRUE;
		} else {
		    isca = PR_FALSE;
		}
	    }
	} else {
	    if ( issuerCert->nsCertType & caCertType ) {
		isca = PR_TRUE;
	    } else {
		isca = PR_FALSE;
	    }
	}
	
	if ( ( !isFortezza ) && ( !isca ) ) {
	    PORT_SetError(SEC_ERROR_CA_CERT_INVALID);
	    LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
	}
	    
	/* make sure key usage allows cert signing */
	if ( ( issuerCert->keyUsage & requiredCAKeyUsage ) !=
	    requiredCAKeyUsage) {
	    PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
	    LOG_ERROR_OR_EXIT(log,issuerCert,count+1,requiredCAKeyUsage);
	}
	
	/* make sure that the issuer is not self signed.  If it is, then
	 * stop here to prevent looping.
	 */
	rvCompare = SECITEM_CompareItem(&issuerCert->derSubject,
				 &issuerCert->derIssuer);
	if (rvCompare == SECEqual) {
	    PORT_SetError(SEC_ERROR_UNTRUSTED_ISSUER);
	    LOG_ERROR(log,issuerCert,count+1,0);
	    goto loser;
	}

	CERT_DestroyCertificate(subjectCert);
	subjectCert = issuerCert;
    }

    subjectCert = NULL;
    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
    LOG_ERROR(log,issuerCert,count,0);
loser:
    rv = SECFailure;
done:

    if ( issuerCert ) {
	CERT_DestroyCertificate(issuerCert);
    }
    
    if ( subjectCert ) {
	CERT_DestroyCertificate(subjectCert);
    }
    
    return rv;
}
			
/*
 * verify a certificate by checking if its valid and that we
 * trust the issuer.
 * Note that this routine does not verify the signature of the certificate.
 */
SECStatus
CERT_VerifyCert(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertUsage certUsage, int64 t,
		void *wincx, CERTVerifyLog *log)
{
    SECStatus rv;
    unsigned int requiredKeyUsage;
    unsigned int requiredCertType;
    unsigned int flags;
    SECCertTimeValidity validity;
    unsigned int certType;
    
    /* make sure that the cert is valid at time t */
    validity = CERT_CheckCertValidTimes (cert, t);
    if ( validity != secCertTimeValid ) {
	LOG_ERROR_OR_EXIT(log,cert,0,validity);
    }

    /* check key usage and netscape cert type */
    certType = cert->nsCertType;
    switch ( certUsage ) {
      case certUsageSSLClient:
	requiredKeyUsage = KU_DIGITAL_SIGNATURE;
	requiredCertType = NS_CERT_TYPE_SSL_CLIENT;
	break;
      case certUsageSSLServer:
	requiredKeyUsage = KU_KEY_ENCIPHERMENT;
	requiredCertType = NS_CERT_TYPE_SSL_SERVER;
	break;
      case certUsageSSLServerWithStepUp:
	requiredKeyUsage = KU_KEY_ENCIPHERMENT | KU_NS_GOVT_APPROVED;
	requiredCertType = NS_CERT_TYPE_SSL_SERVER;
	break;
      case certUsageSSLCA:
	requiredKeyUsage = KU_KEY_CERT_SIGN;
	requiredCertType = NS_CERT_TYPE_SSL_CA;
        break;
      case certUsageEmailSigner:
	requiredKeyUsage = KU_DIGITAL_SIGNATURE;
	requiredCertType = NS_CERT_TYPE_EMAIL;
	break;
      case certUsageEmailRecipient:
	requiredKeyUsage = KU_KEY_ENCIPHERMENT;
	requiredCertType = NS_CERT_TYPE_EMAIL;
	break;
      case certUsageObjectSigner:
	requiredKeyUsage = KU_DIGITAL_SIGNATURE;
	requiredCertType = NS_CERT_TYPE_OBJECT_SIGNING;
	break;
      case certUsageVerifyCA:
	requiredKeyUsage = KU_KEY_CERT_SIGN;
	requiredCertType = NS_CERT_TYPE_CA;
	if ( ! ( certType & NS_CERT_TYPE_CA ) ) {
	    certType |= NS_CERT_TYPE_CA;
	}
	break;
      default:
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredKeyUsage = 0;
	requiredCertType = 0;
    }
    if ( ( cert->keyUsage & requiredKeyUsage ) != requiredKeyUsage ) {
	PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
	LOG_ERROR_OR_EXIT(log,cert,0,requiredKeyUsage);
    }
    if ( ! ( certType & requiredCertType ) ) {
	PORT_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE);
	LOG_ERROR_OR_EXIT(log,cert,0,requiredCertType);
    }

    /* check trust flags to see if this cert is directly trusted */
    if ( cert->trust ) { /* the cert is in the DB */
	switch ( certUsage ) {
	  case certUsageSSLClient:
	  case certUsageSSLServer:
	    flags = cert->trust->sslFlags;
	    
	    /* is the cert directly trusted or not trusted ? */
	    if ( flags & CERTDB_VALID_PEER ) {/*the trust record is valid*/
		if ( flags & CERTDB_TRUSTED ) {	/* trust this cert */
		    return(SECSuccess);
		} else { /* don't trust this cert */
		    PORT_SetError(SEC_ERROR_UNTRUSTED_CERT);
		    LOG_ERROR_OR_EXIT(log,cert,0,flags);
		}
	    }
	    break;
	  case certUsageSSLServerWithStepUp:
	    /* XXX - step up certs can't be directly trusted */
	    break;
	  case certUsageSSLCA:
	    break;
	  case certUsageEmailSigner:
	  case certUsageEmailRecipient:
	    flags = cert->trust->emailFlags;
	    
	    /* is the cert directly trusted or not trusted ? */
	    if ( ( flags & ( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) ==
		( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) {
		return(SECSuccess);
	    }
	    break;
	  case certUsageObjectSigner:
	    flags = cert->trust->objectSigningFlags;
	    
	    /* is the cert directly trusted or not trusted ? */
	    if ( ( flags & ( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) ==
		( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) {
		return(SECSuccess);
	    }
	    break;
	  case certUsageVerifyCA:
	    flags = cert->trust->sslFlags;
	    /* is the cert directly trusted or not trusted ? */
	    if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
		( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
		return(SECSuccess);
	    }
	    flags = cert->trust->emailFlags;
	    /* is the cert directly trusted or not trusted ? */
	    if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
		( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
		return(SECSuccess);
	    }
	    flags = cert->trust->objectSigningFlags;
	    /* is the cert directly trusted or not trusted ? */
	    if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
		( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
		return(SECSuccess);
	    }
	    break;
	}
    }

    rv = CERT_VerifyCertChain(handle, cert, checkSig, certUsage,
			      t, wincx, log);
    return (rv);

loser:
    rv = SECFailure;
    
    return(rv);
}

/*
 * verify a certificate by checking if its valid and that we
 * trust the issuer.  Verify time against now.
 */
SECStatus
CERT_VerifyCertNow(CERTCertDBHandle *handle, CERTCertificate *cert,
		   PRBool checkSig, SECCertUsage certUsage, void *wincx)
{
    return(CERT_VerifyCert(handle, cert, checkSig, 
		   certUsage, PR_Now(), wincx, NULL));
}

CERTCertificate *
CERT_DupCertificate(CERTCertificate *c)
{
    if (c) {
	++c->referenceCount;
    }
    return c;
}

/*
 * Allow use of default cert database, so that apps(such as mozilla) don't
 * have to pass the handle all over the place.
 */
static CERTCertDBHandle *default_cert_db_handle = 0;

void
CERT_SetDefaultCertDB(CERTCertDBHandle *handle)
{
    default_cert_db_handle = handle;
    
    return;
}

CERTCertDBHandle *
CERT_GetDefaultCertDB(void)
{
    return(default_cert_db_handle);
}

/*
 * Open volatile certificate database and index databases.  This is a
 * fallback if the real databases can't be opened or created.  It is only
 * resident in memory, so it will not be persistent.  We do this so that
 * we don't crash if the databases can't be created.
 */
SECStatus
CERT_OpenVolatileCertDB(CERTCertDBHandle *handle)
{
    /*
     * Open the memory resident perm cert database.
     */
    handle->permCertDB = dbopen( 0, O_RDWR | O_CREAT, 0600, DB_HASH, 0 );
    if ( !handle->permCertDB ) {
	goto loser;
    }

    /*
     * Open the memory resident decoded cert database.
     */
    handle->tempCertDB = dbopen( 0, O_RDWR | O_CREAT, 0600, DB_HASH, 0 );
    if ( !handle->tempCertDB ) {
	goto loser;
    }

    /* initialize the cert database */
    if ( CERT_InitCertDB(handle) ) {
	goto loser;
    }
    
    return (SECSuccess);
    
loser:

    PORT_SetError(SEC_ERROR_BAD_DATABASE);

    if ( handle->permCertDB ) {
	(* handle->permCertDB->close)(handle->permCertDB);
	handle->permCertDB = 0;
    }

    if ( handle->tempCertDB ) {
	(* handle->tempCertDB->close)(handle->tempCertDB);
	handle->tempCertDB = 0;
    }

    return(SECFailure);
}

/* XXX this would probably be okay/better as an xp routine? */
static void
sec_lower_string(char *s)
{
    if ( s == NULL ) {
	return;
    }
    
    while ( *s ) {
	*s = PORT_Tolower(*s);
	s++;
    }
    
    return;
}

/* Make sure that the name of the host we are connecting to matches the
 * name that is incoded in the common-name component of the certificate
 * that they are using.
 */
SECStatus
CERT_VerifyCertName(CERTCertificate *cert, char *hn)
{
    char *cn;
    int match;
    char *domain;
    char *hndomain;
    int regvalid;
    char *hostname;
    SECStatus rv;
    
    if ( cert->domainOK ) { /* user already said OK */
	return(SECSuccess);
    }

    hostname = PORT_Strdup(hn);
    if ( hostname == NULL ) {
	return(SECFailure);
    }

    sec_lower_string(hostname);
    
    /* try the cert extension first, then the common name */
    cn = CERT_FindNSStringExtension(cert, SEC_OID_NS_CERT_EXT_SSL_SERVER_NAME);
    if ( cn == NULL ) {
	cn = CERT_GetCommonName(&cert->subject);
    }
    
    sec_lower_string(cn);

    if ( cn ) {
	if ( ( hndomain = PORT_Strchr(hostname, '.') ) == NULL ) {
	    /* No domain in server name */
	    if ( ( domain = PORT_Strchr(cn, '.') ) != NULL ) {
		/* there is a domain in the cn string, so chop it off */
		*domain = '\0';
	    }
	}

	regvalid = XP_RegExpValid(cn);
	
	if ( regvalid == NON_SXP ) {
	    /* compare entire hostname with cert name */
	    if ( PORT_Strcmp(hostname, cn) == 0 ) {
		rv = SECSuccess;
		goto done;
	    }
	    
	    if ( hndomain ) {
		/* compare just domain name with cert name */
		if ( PORT_Strcmp(hndomain+1, cn) == 0 ) {
		    rv = SECSuccess;
		    goto done;
		}
	    }

	    PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
	    rv = SECFailure;
	    goto done;
	    
	} else {
	    /* try to match the shexp */
	    match = XP_RegExpCaseSearch(hostname, cn);

	    if ( match == 0 ) {
		rv = SECSuccess;
	    } else {
		PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
		rv = SECFailure;
	    }
	    goto done;
	}
    }

    PORT_SetError(SEC_ERROR_NO_MEMORY);
    rv = SECFailure;

done:
    /* free the common name */
    if ( cn ) {
	PORT_Free(cn);
    }
    
    if ( hostname ) {
	PORT_Free(hostname);
    }
    
    return(rv);
}

PRBool
CERT_CompareCerts(CERTCertificate *c1, CERTCertificate *c2)
{
    SECComparison comp;
    
    comp = SECITEM_CompareItem(&c1->derCert, &c2->derCert);
    if ( comp == SECEqual ) { /* certs are the same */
	return(PR_TRUE);
    } else {
	return(PR_FALSE);
    }
}

static SECStatus
StringsEqual(char *s1, char *s2) {
    if ( ( s1 == NULL ) || ( s2 == NULL ) ) {
	if ( s1 != s2 ) { /* only one is null */
	    return(SECFailure);
	}
	return(SECSuccess); /* both are null */
    }
	
    if ( PORT_Strcmp( s1, s2 ) != 0 ) {
	return(SECFailure); /* not equal */
    }

    return(SECSuccess); /* strings are equal */
}


PRBool
CERT_CompareCertsForRedirection(CERTCertificate *c1, CERTCertificate *c2)
{
    SECComparison comp;
    char *c1str, *c2str;
    SECStatus eq;
    
    comp = SECITEM_CompareItem(&c1->derCert, &c2->derCert);
    if ( comp == SECEqual ) { /* certs are the same */
	return(PR_TRUE);
    }
	
    /* check if they are issued by the same CA */
    comp = SECITEM_CompareItem(&c1->derIssuer, &c2->derIssuer);
    if ( comp != SECEqual ) { /* different issuer */
	return(PR_FALSE);
    }

    /* check country name */
    c1str = CERT_GetCountryName(&c1->subject);
    c2str = CERT_GetCountryName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }

    /* check locality name */
    c1str = CERT_GetLocalityName(&c1->subject);
    c2str = CERT_GetLocalityName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }
	
    /* check state name */
    c1str = CERT_GetStateName(&c1->subject);
    c2str = CERT_GetStateName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }

    /* check org name */
    c1str = CERT_GetOrgName(&c1->subject);
    c2str = CERT_GetOrgName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }

#ifdef NOTDEF	
    /* check orgUnit name */
    /* we are taking this out because the new certs for merchant.netscape.com
     * and transact.netscape.com have different orgUnit values.  We need
     * to revisit this and decide which fields should be allowed to be
     * different
     */
    c1str = CERT_GetOrgUnitName(&c1->subject);
    c2str = CERT_GetOrgUnitName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }
#endif

    return(PR_TRUE); /* all fields but common name are the same */
}


/*
** CERT_CertChainFromCert
**
** Construct a CERTCertificateList consisting of the given certificate and all
** of the issuer certs until we either get to a self-signed cert or can't find
** an issuer.  Since we don't know how many certs are in the chain we have to
** build a linked list first as we count them.
*/

typedef struct certNode {
    struct certNode *next;
    CERTCertificate *cert;
} certNode;

CERTCertificateList *
CERT_CertChainFromCert(CERTCertDBHandle *handle, CERTCertificate *cert)
{
    CERTCertificateList *chain = NULL;
    CERTCertificate *c;
    SECItem *p;
    int rv, len = 0;
    PRArenaPool *tmpArena, *arena;
    certNode *head, *tail, *node;

    /*
     * Initialize stuff so we can goto loser.
     */
    head = NULL;
    arena = NULL;

    /* arena for linked list */
    tmpArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (tmpArena == NULL) goto no_memory;

    /* arena for SecCertificateList */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto no_memory;

    head = tail = PORT_ArenaZAlloc(tmpArena, sizeof(certNode));
    if (head == NULL) goto no_memory;

    /* put primary cert first in the linked list */
    head->cert = c = CERT_DupCertificate(cert);
    if (head->cert == NULL) goto loser;
    len++;

    /* add certs until we come to a self-signed one */
    while(SECITEM_CompareItem(&c->derIssuer, &c->derSubject) != SECEqual) {
	c = CERT_FindCertByName(handle, &tail->cert->derIssuer);
	if (c == NULL) break;

	tail->next = PORT_ArenaZAlloc(tmpArena, sizeof(certNode));
	tail = tail->next;
	if (tail == NULL) goto no_memory;
	
	tail->cert = c;
	len++;
    }

    /* now build the CERTCertificateList */
    chain = PORT_ArenaAlloc(arena, sizeof(CERTCertificateList));
    if (chain == NULL) goto no_memory;
    chain->certs = PORT_ArenaAlloc(arena, len * sizeof(SECItem));
    if (chain->certs == NULL) goto no_memory;
    
    for(node = head, p = chain->certs; node; node = node->next, p++) {
	rv = SECITEM_CopyItem(arena, p, &node->cert->derCert);
	CERT_DestroyCertificate(node->cert);
	node->cert = NULL;
	if (rv < 0) goto loser;
    }
    chain->len = len;
    chain->arena = arena;

    PORT_FreeArena(tmpArena, PR_FALSE);
    
    return chain;

no_memory:
    PORT_SetError(SEC_ERROR_NO_MEMORY);
loser:
    if (head != NULL) {
	for (node = head; node; node = node->next) {
	    if (node->cert != NULL)
		CERT_DestroyCertificate(node->cert);
	}
    }

    if (arena != NULL) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    if (tmpArena != NULL) {
	PORT_FreeArena(tmpArena, PR_FALSE);
    }

    return NULL;
}

void
CERT_DestroyCertificateList(CERTCertificateList *list)
{
    PORT_FreeArena(list->arena, PR_FALSE);
}


CERTIssuerAndSN *
CERT_GetCertIssuerAndSN(PRArenaPool *arena, CERTCertificate *cert)
{
    CERTIssuerAndSN *result;
    SECStatus rv;

    if ( arena == NULL ) {
	arena = cert->arena;
    }
    
    result = PORT_ArenaZAlloc(arena, sizeof(*result));
    if (result == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    rv = SECITEM_CopyItem(arena, &result->derIssuer, &cert->derIssuer);
    if (rv != SECSuccess)
	return NULL;

    rv = CERT_CopyName(arena, &result->issuer, &cert->issuer);
    if (rv != SECSuccess)
	return NULL;

    rv = SECITEM_CopyItem(arena, &result->serialNumber, &cert->serialNumber);
    if (rv != SECSuccess)
	return NULL;

    return result;
}

char *
CERT_MakeCANickname(CERTCertificate *cert)
{
    char *firstname = NULL;
    char *org = NULL;
    char *nickname = NULL;
    int count;
    CERTCertificate *dummycert;
    CERTCertDBHandle *handle;
    
    handle = cert->dbhandle;
    
    firstname = CERT_GetCommonName(&cert->subject);
    if ( firstname == NULL ) {
	firstname = CERT_GetOrgUnitName(&cert->subject);
    }

    org = CERT_GetOrgName(&cert->issuer);
    if ( org == NULL ) {
	goto loser;
    }
    
    count = 1;
    while ( 1 ) {

	if ( firstname ) {
	    if ( count == 1 ) {
		nickname = PR_smprintf("%s - %s", firstname, org);
	    } else {
		nickname = PR_smprintf("%s - %s #%d", firstname, org, count);
	    }
	} else {
	    if ( count == 1 ) {
		nickname = PR_smprintf("%s", org);
	    } else {
		nickname = PR_smprintf("%s #%d", org, count);
	    }
	}
	if ( nickname == NULL ) {
	    goto loser;
	}

	/* look up the nickname to make sure it isn't in use already */
	dummycert = CERT_FindCertByNickname(handle, nickname);

	if ( dummycert == NULL ) {
	    goto done;
	}
	
	/* found a cert, destroy it and loop */
	CERT_DestroyCertificate(dummycert);

	/* free the nickname */
	PORT_Free(nickname);

	count++;
    }
    
loser:
    if ( nickname ) {
	PORT_Free(nickname);
    }

    nickname = "";
    
done:
    if ( firstname ) {
	PORT_Free(firstname);
    }
    if ( org ) {
	PORT_Free(org);
    }
    
    return(nickname);
}

SECStatus
CERT_ImportCAChain(SECItem *certs, int numcerts, SECCertUsage certUsage)
{
    SECStatus rv;
    SECItem *derCert;
    SECItem certKey;
    PRArenaPool *arena;
    CERTCertificate *cert = NULL;
    CERTCertificate *newcert = NULL;
    CERTCertDBHandle *handle;
    int64 newtime, oldtime;
    CERTCertTrust trust;
    PRBool notca;
    char *nickname;
    
    handle = CERT_GetDefaultCertDB();
    
    arena = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( ! arena ) {
	goto loser;
    }

    while (numcerts--) {
	derCert = certs;
	certs++;
	
	/* get the key (issuer+cn) from the cert */
	rv = CERT_KeyFromDERCert(arena, derCert, &certKey);
	if ( rv != SECSuccess ) {
	    goto loser;
	}

	/* same cert already exists in the database, don't need to do
	 * anything more with it
	 */
	cert = CERT_FindCertByKey(handle, &certKey);
	if ( cert ) {
	    CERT_DestroyCertificate(cert);
	    cert = NULL;
	    continue;
	}

	/* decode my certificate */
	newcert = CERT_DecodeDERCertificate(derCert, PR_FALSE, NULL);
	if ( newcert == NULL ) {
	    goto loser;
	}

	/* make sure that cert is valid */
	rv = CERT_CertTimesValid(newcert);
	if ( rv == SECFailure ) {
	    goto endloop;
	}

	/* does it have the CA extension */
	
	/*
	 * Make sure that if this is an intermediate CA in the chain that
	 * it was given permission by its signer to be a CA.
	 */
	if ( certUsage == certUsageSSLCA ) {
	    /* cert is being used for SSL */
	    if ( newcert->nsCertType & NS_CERT_TYPE_SSL_CA ) {
		notca = PR_FALSE;
	    } else {
		notca = PR_TRUE;
	    }
	} else {
	    /* only deal with SSL usage for now */
	    notca = PR_TRUE;
	}

	/* if it doesn't have the right CA extension, then dump it */
	if ( notca ) {
	    goto endloop;
	}

	/* cert of the same name already exists */
	cert = CERT_FindCertByName(handle, &newcert->derSubject);
	if ( cert ) { /* different cert with same name - yuck!! */

	    /* get the expiration time of the new cert being downloaded */
	    rv = DER_UTCTimeToTime(&newtime, &newcert->validity.notAfter);
	    if ( rv ) {
		goto loser;
	    }
	    
	    /* get the expiration time of the old cert with the same name */
	    rv = DER_UTCTimeToTime(&oldtime, &cert->validity.notAfter);
	    if ( rv ) {
		goto loser;
	    }
	    
	    if ( LL_CMP(newtime, >, oldtime) ) {
		/* newly downloaded cert is newer */
		/* XXX - need to implement this */
	    }
	    
	    CERT_DestroyCertificate(cert);
	    cert = NULL;

	    goto endloop;
	}

	/* it passed all of the tests, so lets add it to the database */
	/* mark it as a CA */
	PORT_Memset((void *)&trust, 0, sizeof(trust));
	trust.sslFlags = CERTDB_VALID_CA;
	
	cert = CERT_NewTempCertificate(handle, derCert, NULL, PR_FALSE, PR_TRUE);
	if ( cert == NULL ) {
	    goto loser;
	}
	
	/* get a default nickname for it */
	nickname = CERT_MakeCANickname(cert);

	rv = CERT_AddTempCertToPerm(cert, nickname, &trust);
	/* free the nickname */
	if ( nickname ) {
	    PORT_Free(nickname);
	}

	CERT_DestroyCertificate(cert);
	cert = NULL;
	
	if ( rv != SECSuccess ) {
	    goto loser;
	}

endloop:
	if ( newcert ) {
	    CERT_DestroyCertificate(newcert);
	    newcert = NULL;
	}
	
    }

    rv = SECSuccess;
    goto done;
loser:
    rv = SECFailure;
done:
    
    if ( newcert ) {
	CERT_DestroyCertificate(newcert);
	newcert = NULL;
    }
    
    if ( cert ) {
	CERT_DestroyCertificate(cert);
	cert = NULL;
    }
    
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    return(rv);
}

void
CERT_DestroyCrl (CERTSignedCrl *crl)
{
    SEC_DestroyCrl (crl);
}

/*
 * Does a cert belong to a CA?  We decide based on perm database trust
 * flags, Netscape Cert Type Extension, and KeyUsage Extension.
 */
PRBool
CERT_IsCACert(CERTCertificate *cert)
{
    CERTCertTrust *trust;
    SECStatus rv;
    
    if ( cert->isperm ) {
	trust = cert->trust;
	if ( ( ( trust->sslFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
	    ( ( trust->emailFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
	    ( ( trust->objectSigningFlags & CERTDB_VALID_CA ) ==
	     CERTDB_VALID_CA ) ){
	    return(PR_TRUE);
	}
    } else {
	if ( cert->nsCertType &
	    ( NS_CERT_TYPE_SSL_CA | NS_CERT_TYPE_EMAIL_CA |
	     NS_CERT_TYPE_OBJECT_SIGNING_CA ) ) {
	    return(PR_TRUE);
	} else {
	    CERTBasicConstraints constraints;
	    rv = CERT_FindBasicConstraintExten(cert, &constraints);
	    if ( rv == SECSuccess ) {
		if ( constraints.isCA ) {
		    return(PR_TRUE);
		}
	    }
	}
    }
    return(PR_FALSE);
}

/*
 * is certa newer than certb?  If one is expired, pick the other one.
 */
PRBool
CERT_IsNewer(CERTCertificate *certa, CERTCertificate *certb)
{
    int64 notBeforeA, notAfterA, notBeforeB, notAfterB, now;
    SECStatus rv;
    PRBool newerbefore, newerafter;
    
    rv = CERT_GetCertTimes(certa, &notBeforeA, &notAfterA);
    if ( rv != SECSuccess ) {
	return(PR_FALSE);
    }
    
    rv = CERT_GetCertTimes(certb, &notBeforeB, &notAfterB);
    if ( rv != SECSuccess ) {
	return(PR_TRUE);
    }

    newerbefore = PR_FALSE;
    if ( LL_CMP(notBeforeA, >, notBeforeB) ) {
	newerbefore = PR_TRUE;
    }

    newerafter = PR_FALSE;
    if ( LL_CMP(notAfterA, >, notAfterB) ) {
	newerafter = PR_TRUE;
    }
    
    if ( newerbefore && newerafter ) {
	return(PR_TRUE);
    }
    
    if ( ( !newerbefore ) && ( !newerafter ) ) {
	return(PR_FALSE);
    }

    /* get current UTC time */
    now = PR_Now();

    if ( newerbefore ) {
	/* cert A was issued after cert B, but expires sooner */
	/* if A is expired, then pick B */
	if ( LL_CMP(notAfterA, <, now ) ) {
	    return(PR_FALSE);
	}
	return(PR_TRUE);
    } else {
	/* cert B was issued after cert A, but expires sooner */
	/* if B is expired, then pick A */
	if ( LL_CMP(notAfterB, <, now ) ) {
	    return(PR_TRUE);
	}
	return(PR_FALSE);
    }
}

void
CERT_DestroyCertArray(CERTCertificate **certs, unsigned int ncerts)
{
    unsigned int i;
    
    if ( certs ) {
	for ( i = 0; i < ncerts; i++ ) {
	    if ( certs[i] ) {
		CERT_DestroyCertificate(certs[i]);
	    }
	}

	PORT_Free(certs);
    }
    
    return;
}

char *
CERT_FixupEmailAddr(char *emailAddr)
{
    char *retaddr;
    char *str;

    if ( emailAddr == NULL ) {
	return(NULL);
    }
    
    /* copy the string */
    str = retaddr = PORT_Strdup(emailAddr);
    if ( str == NULL ) {
	return(NULL);
    }
    
    /* make it lower case */
    while ( *str ) {
	*str = tolower( *str );
	str++;
    }
    
    return(retaddr);
}

/*
 * NOTE - don't allow encode of govt-approved or invisible bits
 */
SECStatus
CERT_DecodeTrustString(CERTCertTrust *trust, char *trusts)
{
    int i;
    unsigned int *pflags;
    
    trust->sslFlags = 0;
    trust->emailFlags = 0;
    trust->objectSigningFlags = 0;

    pflags = &trust->sslFlags;
    
    for (i=0; i < PORT_Strlen(trusts); i++) {
	switch (trusts[i]) {
	  case 'p':
	      *pflags = *pflags | CERTDB_VALID_PEER;
	      break;

	  case 'P':
	      *pflags = *pflags | CERTDB_TRUSTED | CERTDB_VALID_PEER;
	      break;

	  case 'w':
	      *pflags = *pflags | CERTDB_SEND_WARN;
	      break;

	  case 'c':
	      *pflags = *pflags | CERTDB_VALID_CA;
	      break;

	  case 'T':
	      *pflags = *pflags | CERTDB_TRUSTED_CLIENT_CA | CERTDB_VALID_CA;
	      break;

	  case 'C' :
	      *pflags = *pflags | CERTDB_TRUSTED_CA | CERTDB_VALID_CA;
	      break;

	  case 'u':
	      *pflags = *pflags | CERTDB_USER;
	      break;
	  case ',':
	      if ( pflags == &trust->sslFlags ) {
		  pflags = &trust->emailFlags;
	      } else {
		  pflags = &trust->objectSigningFlags;
	      }
	      break;
	  default:
	      return SECFailure;
	}
    }

    return SECSuccess;
}

static void
EncodeFlags(char *trusts, unsigned int flags)
{
    if (flags & CERTDB_VALID_CA)
	if (!(flags & CERTDB_TRUSTED_CA) &&
	    !(flags & CERTDB_TRUSTED_CLIENT_CA))
	    XP_STRCAT(trusts, "c");
    if (flags & CERTDB_VALID_PEER)
	if (!(flags & CERTDB_TRUSTED))
	    XP_STRCAT(trusts, "p");
    if (flags & CERTDB_TRUSTED_CA)
	XP_STRCAT(trusts, "C");
    if (flags & CERTDB_TRUSTED_CLIENT_CA)
	XP_STRCAT(trusts, "T");
    if (flags & CERTDB_TRUSTED)
	XP_STRCAT(trusts, "P");
    if (flags & CERTDB_USER)
	XP_STRCAT(trusts, "u");
    if (flags & CERTDB_SEND_WARN)
	XP_STRCAT(trusts, "w");
    if (flags & CERTDB_INVISIBLE_CA)
	XP_STRCAT(trusts, "I");
    if (flags & CERTDB_GOVT_APPROVED_CA)
	XP_STRCAT(trusts, "G");
    return;
}

char *
CERT_EncodeTrustString(CERTCertTrust *trust)
{
    char tmpTrustSSL[32];
    char tmpTrustEmail[32];
    char tmpTrustSigning[32];
    char *retstr = NULL;

    if ( trust ) {
	tmpTrustSSL[0] = '\0';
	tmpTrustEmail[0] = '\0';
	tmpTrustSigning[0] = '\0';
    
	EncodeFlags(tmpTrustSSL, trust->sslFlags);
	EncodeFlags(tmpTrustEmail, trust->emailFlags);
	EncodeFlags(tmpTrustSigning, trust->objectSigningFlags);
    
	retstr = PR_smprintf("%s,%s,%s", tmpTrustSSL, tmpTrustEmail,
			     tmpTrustSigning);
    }
    
    return(retstr);
}

SECStatus
CERT_ImportCerts(CERTCertDBHandle *certdb, SECCertUsage usage,
		 unsigned int ncerts, SECItem **derCerts,
		 CERTCertificate ***retCerts, PRBool keepCerts,
		 PRBool caOnly, char *nickname)
{
    int i;
    CERTCertificate **certs = NULL, *cert = NULL;
    SECStatus rv;

    if ( ncerts ) {
	certs = PORT_Alloc(sizeof(CERTCertificate *) * ncerts );
	if ( certs == NULL ) {
	    return(SECFailure);
	}
    
	/* decode all of the certs into the temporary DB */
	for ( i = 0; i < ncerts; i++ ) {
	    certs[i] = CERT_NewTempCertificate(certdb, derCerts[i], NULL,
					       PR_FALSE, PR_TRUE);
	}

	if ( keepCerts ) {
	    for ( i = 0; i < ncerts; i++ ) {
		rv = CERT_SaveImportedCert(certs[i], usage, caOnly, nickname);
		/* don't care if it fails - keep going */
	    }
	}
    }

    if ( retCerts ) {
	*retCerts = certs;
    } else {
	CERT_DestroyCertArray(certs, ncerts);
    }

    return(SECSuccess);
    
loser:
    if ( retCerts ) {
	*retCerts = NULL;
    }
    if ( certs ) {
	CERT_DestroyCertArray(certs, ncerts);
    }    
    return(SECFailure);

}

