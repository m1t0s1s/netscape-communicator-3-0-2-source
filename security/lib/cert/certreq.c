#include "cert.h"
#include "secder.h"
#include "key.h"
#include "secitem.h"

DERTemplate CERTCertificateRequestTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTCertificateRequest) },
    { DER_INTEGER,
	  offsetof(CERTCertificateRequest,version), },
    { DER_INLINE,
	  offsetof(CERTCertificateRequest,subject),
	  CERTNameTemplate, },
    { DER_INLINE,
	  offsetof(CERTCertificateRequest,subjectPublicKeyInfo),
	  CERTSubjectPublicKeyInfoTemplate, },
    { DER_CONSTRUCTED | DER_CONTEXT_SPECIFIC | 0,
	  offsetof(CERTCertificateRequest,attributes), NULL, DER_ANY },
    { 0, }
};

CERTCertificate *
CERT_CreateCertificate(unsigned long serialNumber,
		      CERTName *issuer,
		      CERTValidity *validity,
		      CERTCertificateRequest *req)
{
    CERTCertificate *c;
    int rv;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	return(0);
    }

    c = (CERTCertificate *)PORT_ArenaZAlloc(arena, sizeof(CERTCertificate));
    
    if (c) {
	c->referenceCount = 1;
	c->arena = arena;

	/*
	 * Default is a plain version 1.
	 * If extensions are added, it will get changed as appropriate.
	 */
	rv = DER_SetUInteger(arena, &c->version, SEC_CERTIFICATE_VERSION_1);
	if (rv) goto loser;

	rv = DER_SetUInteger(arena, &c->serialNumber, serialNumber);
	if (rv) goto loser;

	rv = CERT_CopyName(arena, &c->issuer, issuer);
	if (rv) goto loser;

	rv = CERT_CopyValidity(arena, &c->validity, validity);
	if (rv) goto loser;

	rv = CERT_CopyName(arena, &c->subject, &req->subject);
	if (rv) goto loser;
	rv = SECKEY_CopySubjectPublicKeyInfo(arena, &c->subjectPublicKeyInfo,
					  &req->subjectPublicKeyInfo);
	if (rv) goto loser;
    }
    return c;

  loser:
    CERT_DestroyCertificate(c);
    return 0;
}

/************************************************************************/

CERTCertificateRequest *
CERT_CreateCertificateRequest(CERTName *subject,
			     CERTSubjectPublicKeyInfo *spki,
			     SECItem *attributes)
{
    CERTCertificateRequest *certreq;
    PRArenaPool *arena;
    SECStatus rv;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return NULL;
    }
    
    certreq = (CERTCertificateRequest *)
			PORT_ArenaZAlloc(arena, sizeof(CERTCertificateRequest));

    if (certreq != NULL) {
	certreq->arena = arena;
	
	rv = DER_SetUInteger(arena, &certreq->version,
			     SEC_CERTIFICATE_REQUEST_VERSION);
	if (rv != SECSuccess)
	    goto loser;

	rv = CERT_CopyName(arena, &certreq->subject, subject);
	if (rv != SECSuccess)
	    goto loser;

	rv = SECKEY_CopySubjectPublicKeyInfo(arena,
					  &certreq->subjectPublicKeyInfo,
					  spki);
	if (rv != SECSuccess)
	    goto loser;

	/* Copy over attribute information */
	if (attributes) {
	    rv = SECITEM_CopyItem(arena, &certreq->attributes, attributes);
	    if (rv != SECSuccess)
		goto loser;
	} else {
	    /*
	     ** Invent empty attribute information. According to the
	     ** pkcs#10 spec, attributes has this ASN.1 type:
	     **
	     ** attributes [0] IMPLICIT Attributes
	     **
	     ** Which means that when no attributes exist, we need to
	     ** encode tag type [0] with the IMPLICIT bit in constructed
	     ** form, and set the length value to zero.
	     */
	    certreq->attributes.len = 0;
	    certreq->attributes.data = (unsigned char *) "";
	} 
    } else {
	PORT_FreeArena(arena, PR_FALSE);
    }

    return certreq;

loser:
    CERT_DestroyCertificateRequest(certreq);
    return NULL;
}

void
CERT_DestroyCertificateRequest(CERTCertificateRequest *req)
{
    if (req && req->arena) {
	PORT_FreeArena(req->arena, PR_FALSE);
    }
    return;
}

