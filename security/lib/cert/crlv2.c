/*
 * Code for dealing with x.509 v3 crl and crl entries extensions.
 *
 * Copyright Á 1997 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: crlv2.c,v 1.1 1997/02/21 00:52:06 hoa Exp $
 */

#include "cert.h"
#include "secitem.h"
#include "secoid.h"
#include "secoidt.h"
#include "secder.h"
#include "secasn1.h"
#include "certxutl.h"

SECStatus
CERT_FindCRLExtensionByOID(CERTCrl *crl, SECItem *oid, SECItem *value)
{
    return (cert_FindExtensionByOID (crl->extensions, oid, value));
}
    

SECStatus
CERT_FindCRLExtension(CERTCrl *crl, int tag, SECItem *value)
{
    return (cert_FindExtension (crl->extensions, tag, value));
}


void *
CERT_StartCRLExtensions(CERTCrl *crl)
{
    return (cert_StartExtensions ((void *)crl, CrlExtensions));
}


SECStatus CERT_FindCRLNumberExten (CERTCrl *crl, CERTCrlNumber *value)
{
    SECItem encodedExtenValue;
    SECStatus rv;

    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension
	 (crl->extensions, SEC_OID_X509_CRL_NUMBER, &encodedExtenValue);
    if ( rv != SECSuccess )
	return (rv);

    rv = SEC_ASN1DecodeItem (NULL, value, SEC_IntegerTemplate,
			     &encodedExtenValue);
    PORT_Free (encodedExtenValue.data);
    return (rv);
}

SECStatus CERT_FindCRLReasonExten (CERTCrl *crl, SECItem *value)
{
    return (CERT_FindBitStringExtension
	    (crl->extensions, SEC_OID_X509_REASON_CODE, value));    
}

SECStatus CERT_FindInvalidDateExten (CERTCrl *crl, int64 *value)
{
    SECItem encodedExtenValue;
    SECItem decodedExtenValue;
    SECStatus rv;

    encodedExtenValue.data = decodedExtenValue.data = NULL;
    encodedExtenValue.len = decodedExtenValue.len = 0;

    rv = cert_FindExtension
	 (crl->extensions, SEC_OID_X509_INVALID_DATE, &encodedExtenValue);
    if ( rv != SECSuccess )
	return (rv);

    rv = SEC_ASN1DecodeItem (NULL, &decodedExtenValue,
			     SEC_GeneralizedTimeTemplate, &encodedExtenValue);
    if (rv != SECSuccess)
	return (rv);
    rv = DER_GeneralizedTimeToTime(value, &encodedExtenValue);
    PORT_Free (decodedExtenValue.data);
    PORT_Free (encodedExtenValue.data);
    return (rv);
}
