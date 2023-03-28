/*
 * Code for dealing with x.509 v3 CRL Distribution Point extension.
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 */
#include "gename.h"
#include "certt.h"

extern int SEC_ERROR_EXTENSION_VALUE_INVALID;
extern int SEC_ERROR_NO_MEMORY;

extern void PrepareBitStringForEncoding (SECItem *bitMap, SECItem *value);

static const SEC_ASN1Template FullNameTemplate[] = {
    {SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 0,
	offsetof (CRLDistributionPoint,derFullName), CERT_GeneralNameTemplate}
};

static const SEC_ASN1Template RelativeNameTemplate[] = {
    {SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 1, 
	offsetof (CRLDistributionPoint,distPoint.relativeName), CERT_RDNTemplate}
};
	 
static const SEC_ASN1Template CRLDistributionPointTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(CRLDistributionPoint) },
	{ SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC |
	    SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT | 0,
	    offsetof(CRLDistributionPoint,derDistPoint), SEC_AnyTemplate},
	{ SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	    offsetof(CRLDistributionPoint,bitsmap), SEC_BitStringTemplate},
	{ SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC |
	    SEC_ASN1_CONSTRUCTED | 2,
	    offsetof(CRLDistributionPoint, derCrlIssuer), CERT_GeneralNameTemplate},
    { 0 }
};

const SEC_ASN1Template CERTCRLDistributionPointsTemplate[] = {
    {SEC_ASN1_SEQUENCE_OF, 0, CRLDistributionPointTemplate}
};

SECStatus
CERT_EncodeCRLDistributionPoints (PRArenaPool *arena, CERTCrlDistributionPoints *value,
				  SECItem *derValue)
{
    CRLDistributionPoint **pointList, *point;
    PRArenaPool *ourPool = NULL;
    SECStatus rv = SECSuccess;

    PORT_Assert (derValue);
    PORT_Assert (value && value->distPoints);

    do {
	ourPool = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
	if (ourPool == NULL) {
	    rv = SECFailure;
	    break;
	}    
	
	pointList = value->distPoints;
	while (*pointList) {
	    point = *pointList;
	    point->derFullName = NULL;
	    point->derDistPoint.data = NULL;

	    if (point->distPointType == generalName) {
		rv = cert_EncodeGeneralName
		     (ourPool, point->distPoint.fullName,
		      &point->derFullName);
		
		if (rv == SECSuccess && point->derFullName)
		    rv = (SEC_ASN1EncodeItem (ourPool, &point->derDistPoint,
			  point, FullNameTemplate) == NULL) ? SECFailure : SECSuccess;
	    }
	    else if (point->distPointType == relativeDistinguishedName) {
		if (SEC_ASN1EncodeItem
		     (ourPool, &point->derDistPoint, 
		      point, RelativeNameTemplate) == NULL) 
		    rv = SECFailure;
	    }
	    /* distributionPointName is omitted */
	    else if (point->distPointType != 0) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		rv = SECFailure;
	    }
	    if (rv != SECSuccess)
		break;

	    if (point->reasons.data)
		PrepareBitStringForEncoding (&point->bitsmap, &point->reasons);

	    if (point->crlIssuer) {
		rv = cert_EncodeGeneralName
		     (ourPool, point->crlIssuer, &point->derCrlIssuer);
		if (rv != SECSuccess)
		    break;
	    }
	    
	    ++pointList;
	}
	if (rv != SECSuccess)
	    break;
	if (SEC_ASN1EncodeItem
	     (arena, derValue, value, CERTCRLDistributionPointsTemplate) == NULL) {
	    rv = SECFailure;
	    break;
	}
    } while (0);
    PORT_FreeArena (ourPool, PR_FALSE);
    return (rv);
}

CERTCrlDistributionPoints *
CERT_DecodeCRLDistributionPoints (PRArenaPool *arena, SECItem *encodedValue)
{
   CERTCrlDistributionPoints *value = NULL;    
   CRLDistributionPoint **pointList, *point;    
   SECStatus rv;

   PORT_Assert (arena);
   do {
	value = PORT_ArenaZAlloc (arena, sizeof (*value));
	if (value == NULL) {
	    rv = SECFailure;
	    break;
	}
	    
	rv = SEC_ASN1DecodeItem
	     (arena, &value->distPoints, CERTCRLDistributionPointsTemplate,
	      encodedValue);
	if (rv != SECSuccess)
	    break;

	pointList = value->distPoints;
	while (*pointList) {
	    point = *pointList;

	    /* get the data if the distributionPointName is not omitted */
	    if (point->derDistPoint.data != NULL) {
		point->distPointType = (point->derDistPoint.data[0] & 0x1f) +1;
		if (point->distPointType == generalName) {
		    SECItem innerDER;
		
		    innerDER.data = NULL;
		    rv = SEC_ASN1DecodeItem
			 (arena, point, FullNameTemplate, &(point->derDistPoint));
		    if (rv != SECSuccess)
			break;
		    rv = cert_DecodeGeneralName
		     (arena, &point->distPoint.fullName, point->derFullName);
		    if (rv != SECSuccess)
			break;
		}
		else if ( relativeDistinguishedName) {
		    rv = SEC_ASN1DecodeItem
			 (arena, point, RelativeNameTemplate, &(point->derDistPoint));
		    if (rv != SECSuccess)
			break;
		}
		else {
		    PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		    break;
		}
	    }

	    /* Get the reason code if it's not omitted in the encoding */
	    if (point->bitsmap.data != NULL) {
		point->reasons.data = PORT_ArenaAlloc
				      (arena, (point->bitsmap.len + 7) >> 3);
		if (!point->reasons.data) {
		    rv = SECFailure;
		    break;
		}
		PORT_Memcpy (point->reasons.data, point->bitsmap.data,
			     point->reasons.len = ((point->bitsmap.len + 7) >> 3));
	    }

	    /* Get the crl issuer name if it's not omitted in the encoding */
	    if (point->derCrlIssuer != NULL) {
		rv = cert_DecodeGeneralName
		     (arena, &point->crlIssuer, point->derCrlIssuer);
		if (rv != SECSuccess)
		    break;
	    }
	    ++pointList;
	}
   } while (0);
   return (rv == SECSuccess ? value : NULL);
}
