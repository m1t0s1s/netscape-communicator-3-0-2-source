/*
 * X.509 v3 Subject Key Usage Extension 
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 */

#include "prtypes.h"
#include "mcom_db.h"
#include "seccomon.h"
#include "secdert.h"
#include "secoidt.h"
#include "secasn1t.h"
#include "secasn1.h"
#include "secport.h"
#include "certt.h"
#include "gename.h"

extern int SEC_ERROR_EXTENSION_VALUE_INVALID;
extern int SEC_ERROR_NO_MEMORY;

   
const SEC_ASN1Template CERTAuthKeyIDTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(CERTAuthKeyID) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	  offsetof(CERTAuthKeyID,keyID), SEC_OctetStringTemplate},
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC  | 1,
          offsetof(CERTAuthKeyID, DERAuthCertIssuer), CERT_GeneralNameTemplate},
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 2,
	  offsetof(CERTAuthKeyID,authCertSerialNumber), SEC_IntegerTemplate},
    { 0 }
};



SECStatus CERT_EncodeAuthKeyID (PRArenaPool *arena, CERTAuthKeyID *value, SECItem *encodedValue)
{
    SECStatus rv = SECFailure;
 
    PORT_Assert (value);
    PORT_Assert (arena);
    PORT_Assert (value->DERAuthCertIssuer == NULL);
    PORT_Assert (encodedValue);

    do {
	
	/* If both of the authCertIssuer and the serial number exist, encode
	   the name first.  Otherwise, it is an error if one exist and the other
	   is not.
	 */
	if (value->authCertIssuer && *(value->authCertIssuer)) {
	    if (!value->authCertSerialNumber.data) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		break;
	    }

	    rv = cert_EncodeGeneralName
		 (arena, value->authCertIssuer,
		  &value->DERAuthCertIssuer);
	    if (rv != SECSuccess || !value->DERAuthCertIssuer) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);	
		break;
	    }
	}
	else if (value->authCertSerialNumber.data) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		break;
	}

	if (SEC_ASN1EncodeItem (arena, encodedValue, value,
				CERTAuthKeyIDTemplate) == NULL)
	    break;
	rv = SECSuccess;

    } while (0);
     return(rv);
}

CERTAuthKeyID *CERT_DecodeAuthKeyID
   (PRArenaPool *arena, SECItem *encodedValue)
{
    CERTAuthKeyID *value = NULL;
    SECStatus rv = SECFailure;
    void *mark;

    PORT_Assert (arena);
   
    do {
	mark = PORT_ArenaMark (arena);
        value = PORT_ArenaZAlloc (arena, sizeof (*value));
	if (value == NULL)
	    break;
	rv = SEC_ASN1DecodeItem
	     (arena, value, CERTAuthKeyIDTemplate, encodedValue);
	if (rv != SECSuccess)
	    break;

        rv = cert_DecodeGeneralName (arena, &value->authCertIssuer, value->DERAuthCertIssuer);
	if (rv != SECSuccess)
	    break;
	
	/* what if the general name contains other format but not URI ?
	   hl
	 */
	if ((value->authCertSerialNumber.data && !value->authCertIssuer) ||
	    (!value->authCertSerialNumber.data && value->authCertIssuer)){
	    PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
	    break;
	}
    } while (0);

    if (rv != SECSuccess) {
	PORT_ArenaRelease (arena, mark);
	return ((CERTAuthKeyID *)NULL);	    
    }
    return (value);
}
