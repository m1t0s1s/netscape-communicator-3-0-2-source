#include "cert.h"
#include "secpkcs7.h"
#include "base64.h"
#include "secitem.h"
#include "secder.h"
#include "secoid.h"

SECStatus
SEC_ReadPKCS7Certs(SECItem *pkcs7Item, CERTImportCertificateFunc f, void *arg)
{
    SEC_PKCS7ContentInfo *contentInfo = NULL;
    SECStatus rv;
    SECItem **certs;
    int count;

    contentInfo = SEC_PKCS7DecodeItem(pkcs7Item, NULL, NULL, NULL, NULL);
    if ( contentInfo == NULL ) {
	goto loser;
    }

    if ( SEC_PKCS7ContentType (contentInfo) != SEC_OID_PKCS7_SIGNED_DATA ) {
	goto loser;
    }

    certs = contentInfo->content.signedData->rawCerts;
    if ( certs ) {
	count = 0;
	
	while ( *certs ) {
	    count++;
	    certs++;
	}
	rv = (* f)(arg, contentInfo->content.signedData->rawCerts, count);
    }
    
    rv = SECSuccess;
    
    goto done;
loser:
    rv = SECFailure;
    
done:
    if ( contentInfo ) {
	SEC_PKCS7DestroyContentInfo(contentInfo);
    }

    return(rv);
}

DERTemplate SECCertSequenceTemplate[] = {
    { DER_INDEFINITE | DER_SEQUENCE,
	  0, SECAnyTemplate, sizeof(SECItem *) }
};

SECStatus
SEC_ReadCertSequence(SECItem *certsItem, CERTImportCertificateFunc f, void *arg)
{
    SECStatus rv;
    SECItem **certs;
    int count;
    SECItem **rawCerts;
    PRArenaPool *arena;
    SEC_PKCS7ContentInfo *contentInfo = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	return SECFailure;
    }

    contentInfo = SEC_PKCS7DecodeItem(certsItem, NULL, NULL, NULL, NULL);
    if ( contentInfo == NULL ) {
	goto loser;
    }

    if ( SEC_PKCS7ContentType (contentInfo) != SEC_OID_NS_TYPE_CERT_SEQUENCE ) {
	goto loser;
    }


    rv = DER_Decode(arena, &rawCerts, SECCertSequenceTemplate,
		    contentInfo->content.data);

    if (rv != SECSuccess) {
	goto loser;
    }

    certs = rawCerts;
    if ( certs ) {
	count = 0;
	
	while ( *certs ) {
	    count++;
	    certs++;
	}
	rv = (* f)(arg, rawCerts, count);
    }
    
    rv = SECSuccess;
    
    goto done;
loser:
    rv = SECFailure;
    
done:
    if ( contentInfo ) {
	SEC_PKCS7DestroyContentInfo(contentInfo);
    }

    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(rv);
}

CERTCertificate *
CERT_ConvertAndDecodeCertificate(char *certstr)
{
    CERTCertificate *cert;
    SECStatus rv;
    SECItem der;

    rv = ATOB_ConvertAsciiToItem(&der, certstr);
    if (rv != SECSuccess)
	return NULL;

    cert = CERT_DecodeDERCertificate(&der, PR_TRUE, NULL);

    PORT_Free(der.data);
    return cert;
}

#define NS_CERT_HEADER "-----BEGIN CERTIFICATE-----"
#define NS_CERT_TRAILER "-----END CERTIFICATE-----"

#define CERTIFICATE_TYPE_STRING "certificate"
#define CERTIFICATE_TYPE_LEN (sizeof(CERTIFICATE_TYPE_STRING)-1)

CERTPackageType
CERT_CertPackageType(SECItem *package, SECItem *certitem)
{
    unsigned char *cp;
    int seqLen, seqLenLen;
    SECItem oiditem;
    SECOidData *oiddata;
    CERTPackageType type = certPackageNone;
    
    cp = package->data;

    /* is a DER encoded certificate of some type? */
    if ( ( *cp  & 0x1f ) == DER_SEQUENCE ) {
	cp++;
	
	if ( *cp & 0x80) {
	    /* Multibyte length */
	    seqLenLen = cp[0] & 0x7f;
	    
	    switch (seqLenLen) {
	      case 4:
		seqLen = ((unsigned long)cp[1]<<24) |
		    ((unsigned long)cp[2]<<16) | (cp[3]<<8) | cp[4];
		break;
	      case 3:
		seqLen = ((unsigned long)cp[1]<<16) | (cp[2]<<8) | cp[3];
		break;
	      case 2:
		seqLen = (cp[1]<<8) | cp[2];
		break;
	      case 1:
		seqLen = cp[1];
		break;
	      default:
		/* indefinite length */
		seqLen = 0;
	    }
	    cp += ( seqLenLen + 1 );

	} else {
	    seqLenLen = 0;
	    seqLen = *cp;
	    cp++;
	}

	/* check entire length if definite length */
	if ( seqLen || seqLenLen ) {
	    if ( package->len != ( seqLen + seqLenLen + 2 ) ) {
		/* not a DER package */
		return(type);
	    }
	}
	
	/* check the type string */
	/* netscape wrapped DER cert */
	if ( ( cp[0] == DER_OCTET_STRING ) &&
	    ( cp[1] == CERTIFICATE_TYPE_LEN ) &&
	    ( PORT_Strcmp((char *)&cp[2], CERTIFICATE_TYPE_STRING) ) ) {
	    
	    cp += ( CERTIFICATE_TYPE_LEN + 2 );

	    /* it had better be a certificate by now!! */
	    if ( certitem ) {
		certitem->data = cp;
		certitem->len = package->len -
		    ( cp - (unsigned char *)package->data );
	    }
	    type = certPackageNSCertWrap;
	    
	} else if ( cp[0] == DER_OBJECT_ID ) {
	    /* XXX - assume DER encoding of OID len!! */
	    oiditem.len = cp[1];
	    oiditem.data = (unsigned char *)&cp[2];
	    oiddata = SECOID_FindOID(&oiditem);
	    if ( oiddata == NULL ) {
		/* failure */
		return(type);
	    }

	    if ( certitem ) {
		certitem->data = package->data;
		certitem->len = package->len;
	    }
	    
	    switch ( oiddata->offset ) {
	      case SEC_OID_PKCS7_SIGNED_DATA:
		type = certPackagePKCS7;
		break;
	      case SEC_OID_NS_TYPE_CERT_SEQUENCE:
		type = certPackageNSCertSeq;
		break;
	      default:
		break;
	    }
	    
	} else {
	    /* it had better be a certificate by now!! */
	    if ( certitem ) {
		certitem->data = package->data;
		certitem->len = package->len;
	    }
	    
	    type = certPackageCert;
	}
    }

    return(type);
}

/*
 * read an old style ascii or binary certificate chain
 */
SECStatus
CERT_DecodeCertPackage(char *certbuf,
		       int certlen,
		       CERTImportCertificateFunc f,
		       void *arg)
{
    unsigned char *cp;
    int seqLen, seqLenLen;
    int cl;
    unsigned int part1, part2;
    int asciilen;
    unsigned char *bincert = NULL, *certbegin = NULL, *certend = NULL;
    ATOBContext *atob = NULL;
    CERTCertificate *cert;
    SECItem certitem, oiditem;
    int ret;
    SECStatus rv;
    SECOidData *oiddata;
    SECItem *pcertitem = &certitem;
    
    if ( certbuf == NULL ) {
	return(SECFailure);
    }
    
    cert = 0;
    cp = (unsigned char *)certbuf;

    /* is a DER encoded certificate of some type? */
    if ( ( *cp  & 0x1f ) == DER_SEQUENCE ) {
	cp++;
	
	if ( *cp & 0x80) {
	    /* Multibyte length */
	    seqLenLen = cp[0] & 0x7f;
	    
	    switch (seqLenLen) {
	      case 4:
		seqLen = ((unsigned long)cp[1]<<24) |
		    ((unsigned long)cp[2]<<16) | (cp[3]<<8) | cp[4];
		break;
	      case 3:
		seqLen = ((unsigned long)cp[1]<<16) | (cp[2]<<8) | cp[3];
		break;
	      case 2:
		seqLen = (cp[1]<<8) | cp[2];
		break;
	      case 1:
		seqLen = cp[1];
		break;
	      default:
		/* indefinite length */
		seqLen = 0;
	    }
	    cp += ( seqLenLen + 1 );

	} else {
	    seqLenLen = 0;
	    seqLen = *cp;
	    cp++;
	}

	/* check entire length if definite length */
	if ( seqLen || seqLenLen ) {
	    if ( certlen != ( seqLen + seqLenLen + 2 ) ) {
		goto notder;
	    }
	}
	
	/* check the type string */
	/* netscape wrapped DER cert */
	if ( ( cp[0] == DER_OCTET_STRING ) &&
	    ( cp[1] == CERTIFICATE_TYPE_LEN ) &&
	    ( PORT_Strcmp((char *)&cp[2], CERTIFICATE_TYPE_STRING) ) ) {
	    
	    cp += ( CERTIFICATE_TYPE_LEN + 2 );

	    /* it had better be a certificate by now!! */
	    certitem.data = cp;
	    certitem.len = certlen - ( cp - (unsigned char *)certbuf );
	    
	    rv = (* f)(arg, &pcertitem, 1);
	    
	    return(rv);
	} else if ( cp[0] == DER_OBJECT_ID ) {
	    /* XXX - assume DER encoding of OID len!! */
	    oiditem.len = cp[1];
	    oiditem.data = (unsigned char *)&cp[2];
	    oiddata = SECOID_FindOID(&oiditem);
	    if ( oiddata == NULL ) {
		return(SECFailure);
	    }

	    certitem.data = (unsigned char*)certbuf;
	    certitem.len = certlen;
	    
	    switch ( oiddata->offset ) {
	      case SEC_OID_PKCS7_SIGNED_DATA:
		return(SEC_ReadPKCS7Certs(&certitem, f, arg));
		break;
	      case SEC_OID_NS_TYPE_CERT_SEQUENCE:
		return(SEC_ReadCertSequence(&certitem, f, arg));
		break;
	      default:
		break;
	    }
	    
	} else {
	    /* it had better be a certificate by now!! */
	    certitem.data = (unsigned char*)certbuf;
	    certitem.len = certlen;
	    
	    rv = (* f)(arg, &pcertitem, 1);
	    return(rv);
	}
    }

    /* now look for a netscape base64 ascii encoded cert */
notder:
    cp = (unsigned char *)certbuf;
    cl = certlen;
    certbegin = 0;
    certend = 0;

    /* find the beginning marker */
    while ( cl > sizeof(NS_CERT_HEADER) ) {
	if ( !PORT_Strncasecmp((char *)cp, NS_CERT_HEADER,
			     sizeof(NS_CERT_HEADER)-1) ) {
	    cp = cp + sizeof(NS_CERT_HEADER);
	    certbegin = cp;
	    break;
	}
	
	/* skip to next eol */
	do {
	    cp++;
	    cl--;
	} while ( ( *cp != '\n') && cl );

	/* skip all blank lines */
	while ( ( *cp == '\n') && cl ) {
	    cp++;
	    cl--;
	}
    }

    if ( certbegin ) {

	/* find the ending marker */
	while ( cl > sizeof(NS_CERT_TRAILER) ) {
	    if ( !PORT_Strncasecmp((char *)cp, NS_CERT_TRAILER,
				 sizeof(NS_CERT_TRAILER)-1) ) {
		certend = (unsigned char *)cp;
		break;
	    }

	    /* skip to next eol */
	    do {
		cp++;
		cl--;
	    } while ( ( *cp != '\n') && cl );

	    /* skip all blank lines */
	    while ( ( *cp == '\n') && cl ) {
		cp++;
		cl--;
	    }
	}
    }

    if ( certbegin && certend ) {

	/* allocate a temporary buffer */
	asciilen = certend - certbegin;
	bincert = (unsigned char *)PORT_Alloc(asciilen);
	if ( !bincert ) {
	    return(SECFailure);
	}
	
	/* convert to binary */
	atob = ATOB_NewContext();
	if ( !atob ) {
	    rv = SECFailure;
	    goto loser;
	}

	ATOB_Begin(atob);
	ret = ATOB_Update(atob, bincert, &part1, asciilen, certbegin, asciilen);
	if ( ret ) {
	    rv = SECFailure;
	    goto loser;
	}
	
	ret = ATOB_End(atob, bincert+part1, &part2, asciilen-part1);
	if ( ret ) {
	    rv = SECFailure;
	    goto loser;
	}

	/* now recurse to decode the binary */
	rv = CERT_DecodeCertPackage((char *)bincert, part1 + part2, f, arg);
	
    } else {
	rv = SECFailure;
    }

loser:
    if ( atob ) {
	ATOB_DestroyContext(atob);
    }

    if ( bincert ) {
	PORT_Free(bincert);
    }

    return(rv);
}

typedef struct {
    PRArenaPool *arena;
    SECItem cert;
} collect_args;

static SECStatus
collect_certs(void *arg, SECItem **certs, int numcerts)
{
    SECStatus rv;
    collect_args *collectArgs;
    
    collectArgs = (collect_args *)arg;
    
    rv = SECITEM_CopyItem(collectArgs->arena, &collectArgs->cert, *certs);

    return(rv);
}


/*
 * read an old style ascii or binary certificate
 */
CERTCertificate *
CERT_DecodeCertFromPackage(char *certbuf, int certlen)
{
    collect_args collectArgs;
    SECStatus rv;
    CERTCertificate *cert = NULL;
    
    collectArgs.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    rv = CERT_DecodeCertPackage(certbuf, certlen, collect_certs,
				(void *)&collectArgs);
    if ( rv == SECSuccess ) {
	cert = CERT_DecodeDERCertificate(&collectArgs.cert, PR_TRUE, NULL);
    }
    
    PORT_FreeArena(collectArgs.arena, PR_FALSE);
    
    return(cert);
}
