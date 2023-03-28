/*
 * Code for dealing with X509.V3 extensions.
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: certv3.c,v 1.16.2.3 1997/03/22 23:38:37 jwz Exp $
 */

#include "cert.h"
#include "secitem.h"
#include "secoid.h"
#include "secder.h"
#include "secasn1.h"
#include "certxutl.h"

extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_EXTENSION_NOT_FOUND;
extern int SEC_ERROR_CERT_USAGES_INVALID;

SECStatus
CERT_FindCertExtensionByOID(CERTCertificate *cert, SECItem *oid,
			    SECItem *value)
{
    return (cert_FindExtensionByOID (cert->extensions, oid, value));
}
    

SECStatus
CERT_FindCertExtension(CERTCertificate *cert, int tag, SECItem *value)
{
    return (cert_FindExtension (cert->extensions, tag, value));
}


void *
CERT_StartCertExtensions(CERTCertificate *cert)
{
    return (cert_StartExtensions ((void *)cert, CertificateExtensions));
}

/* find the given extension in the certificate of the Issuer of 'cert' */
SECStatus
CERT_FindIssuerCertExtension(CERTCertificate *cert, int tag, SECItem *value)
{
    CERTCertificate *issuercert;
    SECStatus rv;

    issuercert = CERT_FindCertByName(cert->dbhandle, &cert->derIssuer);
    if ( issuercert ) {
	rv = cert_FindExtension(issuercert->extensions, tag, value);
	CERT_DestroyCertificate(issuercert);
    } else {
	rv = SECFailure;
    }
    
    return(rv);
}

/* find a URL extension in the cert or its CA
 * apply the base URL string if it exists
 */
char *
CERT_FindCertURLExtension(CERTCertificate *cert, int tag, int catag)
{
    SECStatus rv;
    SECItem urlitem;
    SECItem baseitem;
    SECItem urlstringitem;
    SECItem basestringitem;
    PRArenaPool *arena = NULL;
    PRBool hasbase;
    char *urlstring;
    char *str;
    int len;
    int i;
    
    urlstring = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( ! arena ) {
	goto loser;
    }
    
    hasbase = PR_FALSE;
    urlitem.data = NULL;
    baseitem.data = NULL;
    
    rv = cert_FindExtension(cert->extensions, tag, &urlitem);
    if ( rv == SECSuccess ) {
	rv = cert_FindExtension(cert->extensions, SEC_OID_NS_CERT_EXT_BASE_URL,
				   &baseitem);
	if ( rv == SECSuccess ) {
	    hasbase = PR_TRUE;
	}
	
    } else if ( catag ) {
	/* if the cert doesn't have the extensions, see if the issuer does */
	rv = CERT_FindIssuerCertExtension(cert, catag, &urlitem);
	if ( rv != SECSuccess ) {
	    goto loser;
	}	    
	rv = CERT_FindIssuerCertExtension(cert, SEC_OID_NS_CERT_EXT_BASE_URL,
					 &baseitem);
	if ( rv == SECSuccess ) {
	    hasbase = PR_TRUE;
	}
    } else {
	goto loser;
    }

    rv = DER_Decode(arena, &urlstringitem, SECIA5StringTemplate, &urlitem);

    if ( rv != SECSuccess ) {
	goto loser;
    }
    if ( hasbase ) {
	rv = DER_Decode(arena, &basestringitem, SECIA5StringTemplate,
			&baseitem);

	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    
    len = urlstringitem.len + ( hasbase ? basestringitem.len : 0 ) + 1;
    
    str = urlstring = (char *)PORT_Alloc(len);
    if ( urlstring == NULL ) {
	goto loser;
    }
    
    /* copy the URL base first */
    if ( hasbase ) {

	/* if the urlstring has a : in it, then we assume it is an absolute
	 * URL, and will not get the base string pre-pended
	 */
	for ( i = 0; i < urlstringitem.len; i++ ) {
	    if ( urlstringitem.data[i] == ':' ) {
		goto nobase;
	    }
	}
	
	PORT_Memcpy(str, basestringitem.data, basestringitem.len);
	str += basestringitem.len;
	
    }

nobase:
    /* copy the rest (or all) of the URL */
    PORT_Memcpy(str, urlstringitem.data, urlstringitem.len);
    str += urlstringitem.len;
    
    *str = '\0';
    goto done;
    
loser:
    if ( urlstring ) {
	PORT_Free(urlstring);
    }
    
    urlstring = NULL;
done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if ( baseitem.data ) {
	PORT_Free(baseitem.data);
    }
    if ( urlitem.data ) {
	PORT_Free(urlitem.data);
    }

    return(urlstring);
}

/*
 * get the value of the Netscape Certificate Type Extension
 */
SECStatus
CERT_FindNSCertTypeExtension(CERTCertificate *cert, SECItem *retItem)
{

    return (CERT_FindBitStringExtension
	    (cert->extensions, SEC_OID_NS_CERT_EXT_CERT_TYPE, retItem));    
}


/*
 * get the value of a string type extension
 */
char *
CERT_FindNSStringExtension(CERTCertificate *cert, int oidtag)
{
    SECItem wrapperItem, tmpItem;
    SECStatus rv;
    PRArenaPool *arena = NULL;
    char *retstring = NULL;
    
    wrapperItem.data = NULL;
    tmpItem.data = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	goto loser;
    }
    
    rv = cert_FindExtension(cert->extensions, oidtag,
			       &wrapperItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = DER_Decode(arena, &tmpItem, SECIA5StringTemplate, &wrapperItem);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    retstring = (char *)PORT_Alloc(tmpItem.len + 1 );
    if ( retstring == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(retstring, tmpItem.data, tmpItem.len);
    retstring[tmpItem.len] = '\0';

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    if ( wrapperItem.data ) {
	PORT_Free(wrapperItem.data);
    }

    return(retstring);
}

/*
 * get the value of the X.509 v3 Key Usage Extension
 */
SECStatus
CERT_FindKeyUsageExtension(CERTCertificate *cert, SECItem *retItem)
{

    return (CERT_FindBitStringExtension(cert->extensions,
					SEC_OID_X509_KEY_USAGE, retItem));    
}

/*
 * get the value of the X.509 v3 Key Usage Extension
 */
SECStatus
CERT_FindSubjectKeyIDExten(CERTCertificate *cert, SECItem *retItem)
{

    SECItem encodedValue;
    SECStatus rv;

    encodedValue.data = NULL;
    rv = cert_FindExtension
	 (cert->extensions, SEC_OID_X509_SUBJECT_KEY_ID, &encodedValue);
    if (rv != SECSuccess)
	return (rv);
    rv = SEC_ASN1DecodeItem (NULL, retItem, SEC_OctetStringTemplate,
			     &encodedValue);
    PORT_Free (encodedValue.data);

    return (rv);
}

SECStatus
CERT_FindBasicConstraintExten(CERTCertificate *cert,
			      CERTBasicConstraints *value)
{
    SECItem encodedExtenValue;
    SECStatus rv;

    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension(cert->extensions, SEC_OID_X509_BASIC_CONSTRAINTS,
			    &encodedExtenValue);
    if ( rv != SECSuccess ) {
	return (rv);
    }

    return (CERT_DecodeBasicConstraintValue (value, &encodedExtenValue));
}

CERTAuthKeyID *
CERT_FindAuthKeyIDExten (CERTCertificate *cert)
{
    SECItem encodedExtenValue;
    SECStatus rv;

    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension(cert->extensions, SEC_OID_X509_AUTH_KEY_ID,
			    &encodedExtenValue);
    if ( rv != SECSuccess ) {
	return (NULL);
    }

    return (CERT_DecodeAuthKeyID (cert->arena, &encodedExtenValue));
}

CERTCrlDistributionPoints *
CERT_FindCRLDistributionPoints (CERTCertificate *cert)
{
    SECItem encodedExtenValue;
    SECStatus rv;

    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension(cert->extensions, SEC_OID_X509_CRL_DIST_POINTS,
			    &encodedExtenValue);
    if ( rv != SECSuccess ) {
	return (NULL);
    }

    return (CERT_DecodeCRLDistributionPoints (cert->arena,
					      &encodedExtenValue));
}

SECStatus
CERT_CheckCertUsage(CERTCertificate *cert, unsigned char usage)
{
    PRBool critical;    
    SECItem keyUsage;
    SECStatus rv;

    /* There is no extension, v1 or v2 certificate */
    if (cert->extensions == NULL) {
	return (SECSuccess);
    }
    
    keyUsage.data = NULL;

    do {
	/* if the keyUsage extension exists and is critical, make sure that the
	   CA certificate is used for certificate signing purpose only. If the
	   extension does not exist, we will assum that it can be used for
	   certificate signing purpose.
	*/
	rv = CERT_GetExtenCriticality(cert->extensions,
				      SEC_OID_X509_KEY_USAGE,
				      &critical);
	if (rv == SECFailure) {
	    rv = (PORT_GetError () == SEC_ERROR_EXTENSION_NOT_FOUND) ?
		SECSuccess : SECFailure;
	    break;
	}

	if (critical == PR_FALSE) {
	    rv = SECSuccess;
	    break;
	}

	rv = CERT_FindKeyUsageExtension(cert, &keyUsage);
	if (rv != SECSuccess) {
	    break;
	}	
	if (!(keyUsage.data[0] & usage)) {
	    PORT_SetError (SEC_ERROR_CERT_USAGES_INVALID);
	    rv = SECFailure;
	}
    }while (0);
    PORT_Free (keyUsage.data);
    return (rv);
}
