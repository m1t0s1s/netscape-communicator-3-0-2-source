#include "prarena.h"
#include "secpkcs7.h"
#include "p12local.h"
#include "crypto.h"
#include "secoid.h"
#include "secitem.h"
#include "secport.h"
#include "secasn1.h"
#include "secder.h"
#include "cert.h"
#ifndef XP_MAC
#include "../cert/certdb.h"	/* XXX remove when CERTDB_USER
				 * usage below is updated
				 */ 
#else
#include "certdb.h"
#endif
#include "pk11func.h"
#include "secpkcs5.h"
#include "p12plcy.h"

extern int SEC_ERROR_CERT_NICKNAME_COLLISION;
extern int SEC_ERROR_KEY_NICKNAME_COLLISION;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_PKCS12_DECODING_PFX;
extern int SEC_ERROR_PKCS12_INVALID_MAC;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE;
extern int SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_VERSION;
extern int SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT;
extern int SEC_ERROR_PKCS12_CERT_COLLISION;
extern int SEC_ERROR_USER_CANCELLED;
extern int SEC_ERROR_PKCS12_DUPLICATE_DATA;
extern int SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY;
extern int SEC_ERROR_PKCS12_IMPORTING_CERT_CHAIN;
extern int SEC_ERROR_BAD_EXPORT_ALGORITHM;

/* PFX extraction and validation routines */

/* decode the DER encoded PFX item.  if unable to decode, check to see if it
 * is an older PFX item.  If that fails, assume the file was not a valid
 * pfx file.
 * the returned pfx structure should be destroyed using SEC_PKCS12DestroyPFX
 */
static SEC_PKCS12PFXItem *
sec_pkcs12_decode_pfx(SECItem *der_pfx)
{
    SEC_PKCS12PFXItem *pfx;
    SECStatus rv;

    if(der_pfx == NULL) {
	return NULL;
    }

    /* allocate the space for a new PFX item */
    pfx = sec_pkcs12_new_pfx();
    if(pfx == NULL) {
	return NULL;
    }

    rv = SEC_ASN1DecodeItem(pfx->poolp, pfx, SEC_PKCS12PFXItemTemplate, 
    			    der_pfx);

    /* if a failure occurred, check for older version...
     * we also get rid of the old pfx structure, because we don't
     * know where it failed and what data in may contain
     */
    if(rv != SECSuccess) {
	SEC_PKCS12DestroyPFX(pfx);
	pfx = sec_pkcs12_new_pfx();
	if(pfx == NULL) {
	    return NULL;
	}
	rv = SEC_ASN1DecodeItem(pfx->poolp, pfx, SEC_PKCS12PFXItemTemplate_OLD, 
				der_pfx);
	if(rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_PKCS12_DECODING_PFX);
	    PORT_FreeArena(pfx->poolp, PR_TRUE);
	    return NULL;
	}
	pfx->old = PR_TRUE;
	SGN_CopyDigestInfo(pfx->poolp, &pfx->macData.safeMac, &pfx->old_safeMac);
	SECITEM_CopyItem(pfx->poolp, &pfx->macData.macSalt, &pfx->old_macSalt);
    } else {
	pfx->old = PR_FALSE;
    }

    /* convert bit string from bits to bytes */
    pfx->macData.macSalt.len /= 8;

    return pfx;
}

/* validate the integrity MAC used in the PFX.  The MAC is generated
 * per the PKCS 12 document.  If the MAC is incorrect, it is most likely
 * due to an invalid password.
 * pwitem is the integrity password
 * pfx is the decoded pfx item
 */
static PRBool 
sec_pkcs12_check_pfx_mac(SEC_PKCS12PFXItem *pfx, 
			 SECItem *pwitem)
{
    SECItem *key = NULL, *mac = NULL, *data = NULL;
    SECItem *vpwd = NULL;
    SECOidTag algorithm;
    PRBool ret = PR_FALSE;

    if(pfx == NULL) {
	return PR_FALSE;
    }

    algorithm = SECOID_GetAlgorithmTag(&pfx->macData.safeMac.digestAlgorithm);
    switch(algorithm) {
	/* only SHA1 hashing supported as a MACing algorithm */
	case SEC_OID_SHA1:
	    if(pfx->old == PR_FALSE) {
		pfx->swapUnicode == PR_FALSE;

recheckUnicodePassword:
		vpwd = sec_pkcs12_create_virtual_password(pwitem, 
	    					&pfx->macData.macSalt, 
						pfx->swapUnicode);
		if(vpwd == NULL) {
		    return PR_FALSE;
		}
	    }

	    key = sec_pkcs12_generate_key_from_password(algorithm,
						&pfx->macData.macSalt, 
						(pfx->old ? pwitem : vpwd));
	    /* free vpwd only for newer PFX */
	    if(vpwd) {
		SECITEM_ZfreeItem(vpwd, PR_TRUE);
	    }
	    if(key == NULL) {
		return PR_FALSE;
	    }

	    data = SEC_PKCS7GetContent(&pfx->authSafe);
	    if(data == NULL) {
		break;
	    }

	    /* check MAC */
	    mac = sec_pkcs12_generate_mac(key, data, pfx->old);
	    ret = PR_TRUE;
	    if(mac) {
		SECItem *safeMac = &pfx->macData.safeMac.digest;
		if(SECITEM_CompareItem(mac, safeMac) != SECEqual) {

		    /* if we encounter an invalid mac, lets invert the
		     * password in case of unicode changes 
		     */
		    if(((!pfx->old) && pfx->swapUnicode) || (pfx->old)){
			PORT_SetError(SEC_ERROR_PKCS12_INVALID_MAC);
			ret = PR_FALSE;
		    } else {
			SECITEM_ZfreeItem(mac, PR_TRUE);
			pfx->swapUnicode = PR_TRUE;
			goto recheckUnicodePassword;
		    }
		} 
		SECITEM_ZfreeItem(mac, PR_TRUE);
	    } else {
		ret = PR_FALSE;
	    }
	    break;
	default:
	    PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM);
	    ret = PR_FALSE;
	    break;
    }

    /* let success fall through */
    if(key != NULL)
	SECITEM_ZfreeItem(key, PR_TRUE);

    return ret;
}

/* check the validity of the pfx structure.  we currently only support
 * password integrity mode, so we check the MAC.
 */
static PRBool 
sec_pkcs12_validate_pfx(SEC_PKCS12PFXItem *pfx, 
			SECItem *pwitem)
{
    SECOidTag contentType;

    contentType = SEC_PKCS7ContentType(&pfx->authSafe);
    switch(contentType)
    {
	case SEC_OID_PKCS7_DATA:
	    return sec_pkcs12_check_pfx_mac(pfx, pwitem);
	    break;
	case SEC_OID_PKCS7_SIGNED_DATA:
	default:
	    PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE);
	    break;
    }

    return PR_FALSE;
}

/* decode and return the valid PFX.  if the PFX item is not valid,
 * NULL is returned.
 */
static SEC_PKCS12PFXItem *
sec_pkcs12_get_pfx(SECItem *pfx_data, 
		   SECItem *pwitem)
{
    SEC_PKCS12PFXItem *pfx;
    PRBool valid_pfx;

    if((pfx_data == NULL) || (pwitem == NULL)) {
	return NULL;
    }

    pfx = sec_pkcs12_decode_pfx(pfx_data);
    if(pfx == NULL) {
	return NULL;
    }

    valid_pfx = sec_pkcs12_validate_pfx(pfx, pwitem);
    if(valid_pfx != PR_TRUE) {
	SEC_PKCS12DestroyPFX(pfx);
	pfx = NULL;
    }

    return pfx;
}

/* authenticated safe decoding, validation, and access routines
 */

/* convert dogbert beta 3 authenticated safe structure to a post
 * beta three structure, so that we don't have to change more routines.
 */
static SECStatus
sec_pkcs12_convert_old_auth_safe(SEC_PKCS12AuthenticatedSafe *asafe)
{
    SEC_PKCS12Baggage *baggage;
    SEC_PKCS12BaggageItem *bag;
    SECStatus rv = SECSuccess;

    if(asafe->old_baggage.espvks == NULL) {
	/* XXX should the ASN1 engine produce a single NULL element list
	 * rather than setting the pointer to NULL?  
	 * There is no need to return an error -- assume that the list
	 * was empty.
	 */
	return SECSuccess;
    }

    baggage = sec_pkcs12_create_baggage(asafe->poolp);
    if(!baggage) {
	return SECFailure;
    }
    bag = sec_pkcs12_create_external_bag(baggage);
    if(!bag) {
	return SECFailure;
    }

    PORT_Memcpy(&asafe->baggage, baggage, sizeof(SEC_PKCS12Baggage));

    /* if there are shrouded keys, append them to the bag */
    rv = SECSuccess;
    if(asafe->old_baggage.espvks[0] != NULL) {
	int nEspvk = 0;
	rv = SECSuccess;
	while((asafe->old_baggage.espvks[nEspvk] != NULL) && 
		(rv == SECSuccess)) {
	    rv = sec_pkcs12_append_shrouded_key(bag, 
	    				asafe->old_baggage.espvks[nEspvk]);
	    nEspvk++;
	}
    }

    return rv;
}    

/* decodes the authenticated safe item.  a return of NULL indicates
 * an error.  however, the error will have occured either in memory
 * allocation or in decoding the authenticated safe.
 *
 * if an old PFX item has been found, we want to convert the
 * old authenticated safe to the new one.
 */
static SEC_PKCS12AuthenticatedSafe *
sec_pkcs12_decode_authenticated_safe(SEC_PKCS12PFXItem *pfx) 
{
    SECItem *der_asafe = NULL;
    SEC_PKCS12AuthenticatedSafe *asafe = NULL;
    SECStatus rv;

    if(pfx == NULL) {
	return NULL;
    }

    der_asafe = SEC_PKCS7GetContent(&pfx->authSafe);
    if(der_asafe == NULL) {
	/* XXX set error ? */
	goto loser;
    }

    asafe = sec_pkcs12_new_asafe(pfx->poolp);
    if(asafe == NULL) {
	goto loser;
    }

    if(pfx->old == PR_FALSE) {
	rv = SEC_ASN1DecodeItem(pfx->poolp, asafe, 
			 	SEC_PKCS12AuthenticatedSafeTemplate, 
			 	der_asafe);
	asafe->old = PR_FALSE;
	asafe->swapUnicode = pfx->swapUnicode;
    } else {
	/* handle beta exported files */
	rv = SEC_ASN1DecodeItem(pfx->poolp, asafe, 
				SEC_PKCS12AuthenticatedSafeTemplate_OLD,
				der_asafe);
	asafe->safe = &(asafe->old_safe);
	rv = sec_pkcs12_convert_old_auth_safe(asafe);
	asafe->old = PR_TRUE;
    }

    if(rv != SECSuccess) {
	goto loser;
    }

    asafe->poolp = pfx->poolp;
    
    return asafe;

loser:
    return NULL;
}

/* validates the safe within the authenticated safe item.  
 * in order to be valid:
 *  1.  the privacy salt must be present
 *  2.  the encryption algorithm must be supported (including
 *	export policy)
 * PR_FALSE indicates an error, PR_TRUE indicates a valid safe
 */
static PRBool 
sec_pkcs12_validate_encrypted_safe(SEC_PKCS12AuthenticatedSafe *asafe)
{
    PRBool valid = PR_FALSE;
    SECAlgorithmID *algid;

    if(asafe == NULL) {
	return PR_FALSE;
    }

    /* if mode is password privacy, then privacySalt is assumed
     * to be non-zero.
     */
    if(asafe->privacySalt.len != 0) {
	valid = PR_TRUE;
	asafe->privacySalt.len /= 8;
    } else {
	PORT_SetError(SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE);
	return PR_FALSE;
    }

    /* until spec changes, content will have between 2 and 8 bytes depending
     * upon the algorithm used if certs are unencrypted...
     * also want to support case where content is empty -- which we produce 
     */ 
    if(SEC_PKCS7IsContentEmpty(asafe->safe, 8) == PR_TRUE) {
	asafe->emptySafe = PR_TRUE;
	return PR_TRUE;
    }

    asafe->emptySafe = PR_FALSE;

    /* make sure that a pbe algorithm is being used */
    algid = SEC_PKCS7GetEncryptionAlgorithm(asafe->safe);
    if(algid != NULL) {
	if(SEC_PKCS5IsAlgorithmPBEAlg(algid)) {
	    valid = SEC_PKCS12DecryptionAllowed(algid);

	    if(valid == PR_FALSE) {
		PORT_SetError(SEC_ERROR_BAD_EXPORT_ALGORITHM);
	    }
	} else {
	    PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM);
	    valid = PR_FALSE;
	}
    } else {
	valid = PR_FALSE;
	PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM);
    }

    return valid;
}

/* validates authenticates safe:
 *  1.  checks that the version is supported
 *  2.  checks that only password privacy mode is used (currently)
 *  3.  further, makes sure safe has appropriate policies per above function
 * PR_FALSE indicates failure.
 */
static PRBool 
sec_pkcs12_validate_auth_safe(SEC_PKCS12AuthenticatedSafe *asafe)
{
    PRBool valid = PR_TRUE;
    SECOidTag safe_type;
    int version;

    if(asafe == NULL) {
	return PR_FALSE;
    }

    /* check version, since it is default it may not be present.
     * therefore, assume ok
     */
    if((asafe->version.len > 0) && (asafe->old == PR_FALSE)) {
	version = DER_GetInteger(&asafe->version);
	if(version > SEC_PKCS12_PFX_VERSION) {
	    PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_VERSION);
	    return PR_FALSE;
	}
    }

    /* validate password mode is being used */
    safe_type = SEC_PKCS7ContentType(asafe->safe);
    switch(safe_type)
    {
	case SEC_OID_PKCS7_ENCRYPTED_DATA:
	    valid = sec_pkcs12_validate_encrypted_safe(asafe);
	    break;
	case SEC_OID_PKCS7_ENVELOPED_DATA:
	default:
	    PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE);
	    valid = PR_FALSE;
	    break;
    }

    return valid;
}

/* retrieves the authenticated safe item from the PFX item
 *  before returning the authenticated safe, the validity of the
 *  authenticated safe is checked and if valid, returned.
 * a return of NULL indicates that an error occured.
 */
static SEC_PKCS12AuthenticatedSafe *
sec_pkcs12_get_auth_safe(SEC_PKCS12PFXItem *pfx)
{
    SEC_PKCS12AuthenticatedSafe *asafe;
    PRBool valid_safe;
    void *mark;

    if(pfx == NULL) {
	return NULL;
    }

    mark = PORT_ArenaMark(pfx->poolp);

    asafe = sec_pkcs12_decode_authenticated_safe(pfx);
    if(asafe == NULL) {
	return NULL;
    }

    valid_safe = sec_pkcs12_validate_auth_safe(asafe);
    if(valid_safe != PR_TRUE) {
	asafe = NULL;
    } else if(asafe) {
	asafe->baggage.poolp = asafe->poolp;
    }

    return asafe;
}

/* decrypts the authenticated safe.
 * a return of anything but SECSuccess indicates an error.  the
 * password is not known to be valid until the call to the 
 * function sec_pkcs12_get_safe_contents.  If decoding the safe
 * fails, it is assumed the password was incorrect and the error
 * is set then.  any failure here is assumed to be due to 
 * internal problems in SEC_PKCS7DecryptContents or below.
 */
static SECStatus
sec_pkcs12_decrypt_auth_safe(SEC_PKCS12AuthenticatedSafe *asafe, 
			     SECItem *pwitem,
			     void *wincx)
{
    SECStatus rv = SECFailure;
    SECItem *vpwd = NULL;

    if((asafe == NULL) || (pwitem == NULL)) {
	return SECFailure;
    }

    if(asafe->old == PR_FALSE) {
	vpwd = sec_pkcs12_create_virtual_password(pwitem, &asafe->privacySalt,
						  asafe->swapUnicode);
	if(vpwd == NULL) {
	    return SECFailure;
	}
    }

    rv = SEC_PKCS7DecryptContents(asafe->poolp, asafe->safe, 
    				  (asafe->old ? pwitem : vpwd), wincx);

    if(asafe->old == PR_FALSE) {
	SECITEM_ZfreeItem(vpwd, PR_TRUE);
    }

    return rv;
}

/* extract the safe from the authenticated safe.
 *  if we are unable to decode the safe, then it is likely that the 
 *  safe has not been decrypted or the password used to decrypt
 *  the safe was invalid.  we assume that the password was invalid and
 *  set an error accordingly.
 * a return of NULL indicates that an error occurred.
 */
static SEC_PKCS12SafeContents *
sec_pkcs12_get_safe_contents(SEC_PKCS12AuthenticatedSafe *asafe)
{
    SECItem *src = NULL;
    SEC_PKCS12SafeContents *safe = NULL;
    SECStatus rv = SECFailure;

    if(asafe == NULL) {
	return NULL;
    }

    safe = (SEC_PKCS12SafeContents *)PORT_ArenaZAlloc(asafe->poolp, 
	    					sizeof(SEC_PKCS12SafeContents));
    if(safe == NULL) {
	return NULL;
    }
    safe->poolp = asafe->poolp;
    safe->old = asafe->old;
    safe->swapUnicode = asafe->swapUnicode;

    src = SEC_PKCS7GetContent(asafe->safe);
    if(src != NULL) {
	const SEC_ASN1Template *template;
	if(asafe->old != PR_TRUE) {
	    template = SEC_PKCS12SafeContentsTemplate;
	} else {
	    template = SEC_PKCS12SafeContentsTemplate_OLD;
	}

	rv = SEC_ASN1DecodeItem(asafe->poolp, safe, template, src);

	/* if we could not decode the item, password was probably invalid */
	if(rv != SECSuccess) {
	    safe = NULL;
	    PORT_SetError(SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT);
	}
    } else {
	PORT_SetError(SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE);
	rv = SECFailure;
    }

    return safe;
}

/* validates that the key can be imported for the given certificate
 * derCert is the certificate for which the key will be imported
 * keyDupKey is a status indicator for a duplicate key already existing
 * keyProbCert is a status indicator indicating a problem w/ the cert
 * slot is the token to check against
 * wincx is a window handle pointer
 *
 * if an error occurs, SECFailure is returned 
 */
static SECStatus 
sec_pkcs12_validate_key_by_cert(SECItem *derCert, PRBool *keyDupKey,
				PRBool *keyProbCert, PK11SlotInfo *slot,
				void *wincx)
{
    CERTCertificate *leafCert;
    SECKEYPrivateKey *privk;

    if((derCert == NULL) || (slot == NULL) || 
   	(keyDupKey == NULL) || (keyProbCert == NULL)) {
   	return SECFailure;
    }

    leafCert = CERT_DecodeDERCertificate(derCert, PR_TRUE, NULL);
    if(leafCert == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	*keyProbCert = PR_TRUE;
	return SECFailure;
    }

    privk = PK11_FindPrivateKeyFromCert(slot, leafCert, wincx);
    if(privk) {
	*keyDupKey = PR_TRUE;
	SECKEY_DestroyPrivateKey(privk);
    } else {
	privk = PK11_FindKeyByDERCert(slot, leafCert, wincx);
	if(privk != NULL) {
	    *keyDupKey = PR_TRUE;
	    SECKEY_DestroyPrivateKey(privk);
	} else {
	    *keyDupKey = PR_FALSE;
	}
    }

    CERT_DestroyCertificate(leafCert);
    return SECSuccess;
}

/* validates that a cert nickname and subject are unique
 * derCert the certificate to validate
 * certName the certificate nickname
 * keyProbCert a status indicator
 * certDupCert duplicate certificate indicator
 * slot the token to check
 * poolp the memory pool for allocating space
 * ncCall certificate collision callback
 * wincx window handl pointer
 *
 * if an error occurs, SECFailure is returned and the error code set
 */
static SECStatus
sec_pkcs12_validate_cert_nickname(SECItem *derCert, SECItem *certName, 
				  PRBool *keyProbCert, PRBool *certDupCert,
				  PK11SlotInfo *slot, PRArenaPool *poolp, 
				  SEC_PKCS12NicknameCollisionCallback ncCall,
				  void *wincx)
{
    CERTCertificate *leafCert, *testCert;
    SECStatus rv;

    if((derCert == NULL) || (certName == NULL) || (keyProbCert == NULL) ||
    	(certDupCert == NULL) || (slot == NULL) || (poolp == NULL)) {
	return SECFailure;
    }

    /* decode the reference certificate */
    leafCert = CERT_DecodeDERCertificate(derCert, PR_TRUE, NULL);
    if(leafCert == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	*keyProbCert = PR_TRUE;
	return SECFailure;
    }

recheck_cert:

    /* certificate must have unique subject and nickname */
    testCert = PK11_FindCertFromDERSubjectAndNickname(slot, leafCert, 
    						   (char *)certName->data,
    						   wincx);

    /* if certificate not found through PKCS 11, lets check the
     * internal database, if the slot is an internal slot
     */
    if((testCert == NULL) && (PK11_IsInternal(slot) == PR_TRUE)) {
	CERTCertDBHandle *certdb = CERT_GetDefaultCertDB();
	if(certdb == NULL) {
	    /* XXX different error? */
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    return SECFailure;
	}
	testCert = CERT_FindCertByNickname(certdb, (char *)certName->data);
    }

    /* certificate exists, we need to check for collision */
    rv = SECSuccess;
    if(testCert) {
	SECComparison certEq;

	/* check subject */
	certEq = SECITEM_CompareItem(&testCert->derSubject, &leafCert->derSubject);
	CERT_DestroyCertificate(testCert);

	/* if collision occurred, prompt for new nickname */
	if(certEq == SECEqual) {
	    SECItem *newNick = NULL;
	    PRBool cancel = PR_FALSE;

	    newNick = (*ncCall)(certName, &cancel, wincx);
	   
	    rv = SECSuccess; 
	    if((cancel != PR_TRUE) && (newNick)) {
		rv = sec_pkcs12_copy_nickname(poolp, certName, newNick);
	    }

	    /* handle errors and if the user cancels */
	    if((rv == SECFailure) || (cancel == PR_TRUE) || (newNick == NULL)) {
		*keyProbCert = PR_TRUE;
		if(cancel == PR_TRUE) {
		    PORT_SetError(SEC_ERROR_USER_CANCELLED);
		} else {
		    PORT_SetError(SEC_ERROR_PKCS12_CERT_COLLISION);
		}
		rv = SECFailure;
		goto loser;
	    }

	    goto recheck_cert;
	}
    }

loser:
    CERT_DestroyCertificate(leafCert);
    return rv;
}

/* validates a certificate
 * derCert is the certificate to validate
 * certDupCert indicates a duplicate cert encountered 
 * keyProbCert indicates a problem with decoding the cert
 * certName is the certificate nickname
 * slot is the token to check against 
 * poolp is the memory pool for allocation of space 
 * ncCall is the nickname collision call back function
 * wincx is a window handle
 *
 * if an error occurs, the error code is set and SECFailure returned
 */
static SECStatus
sec_pkcs12_validate_cert(SECItem *derCert, PRBool *certDupCert, 
			 PRBool *keyProbCert, SECItem *certName,
			 PK11SlotInfo *slot, PRArenaPool *poolp,
			 SEC_PKCS12NicknameCollisionCallback ncCall,
			 void *wincx)
{
    CERTCertificate *leafCert, *testCert;
    SECStatus rv = SECFailure;

    if((derCert == NULL) || (certDupCert == NULL) || 
    	(keyProbCert == NULL) || (certName == NULL)) {
    	return SECFailure;
    }

    /* decode the certificate */
    leafCert = CERT_DecodeDERCertificate(derCert, PR_TRUE, NULL);
    if(leafCert == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	*keyProbCert = PR_TRUE;
	return SECFailure;
    }

    /* check for the existance of the certificate */
    testCert = PK11_FindCertFromDERCert(slot, leafCert, wincx);
    if((testCert == NULL) && (PK11_IsInternal(slot) == PR_TRUE)) {
	CERTCertDBHandle *certdb = CERT_GetDefaultCertDB();
	if(certdb == NULL) {
	    /* XXX different error? */
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}

	testCert = CERT_FindCertByDERCert(certdb, &leafCert->derCert);
    }

    /* a duplicate certificate exists */
    if(testCert != NULL) {
	CERT_DestroyCertificate(testCert);
	*certDupCert = PR_TRUE;
    } else {
	*certDupCert = PR_FALSE;
    }

    /* check for name and subject collisions */
    if(*certDupCert == PR_FALSE) {
	rv = sec_pkcs12_validate_cert_nickname(derCert, certName, keyProbCert,
    					   certDupCert, slot, poolp, ncCall, wincx);
    } else {
	rv = SECSuccess;
    }

    if(rv == SECFailure) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    if(*keyProbCert == PR_TRUE) {
	rv = SECFailure;
    }

loser:
    CERT_DestroyCertificate(leafCert);
    return rv;
}

/* validates a certificate and key pair
 * key is either a PrivateKey structure or a ESPVKItem
 * wrappedKey indicates which key type key is
 * initKey indicates that this is the first call to this function for
 *    the given key
 * cert is the certificate
 * dupCnt is a count of cert/key pair duplicates already in the slot
 * slot is the token in which the keys and certs are being imported 
 * poolp is the memory pool where allocations occur
 * ncCall is the certificate collision callback function
 * wincx is a window context pointer
 *
 * if an error occurs, the error code is set and SECFailure returned
 */
static SECStatus 
sec_pkcs12_validate_cert_and_key(PRBool wrappedKey, PRBool initKey,
				 void *key, SEC_PKCS12CertAndCRL *cert,
				 int *dupCnt, PK11SlotInfo *slot, 
				 PRArenaPool *poolp, 
				 SEC_PKCS12NicknameCollisionCallback ncCall,
				 void *wincx)
{
    PRBool *keyDupKey = NULL, *keyProbCert = NULL, *certDupCert = NULL;
    int *keyNCerts = NULL;
    SEC_PKCS12PVKSupportingData *supData = NULL;
    SECItem *derCert = NULL, *keyName = NULL, *certName = NULL;
    PRBool keyDone;
    SECStatus rv;


    if((key == NULL) || (cert == NULL) || (poolp == NULL)) {
	return SECFailure;
    }

    /* get key information */
    if(wrappedKey == PR_TRUE) {
	SEC_PKCS12ESPVKItem *espvk = (SEC_PKCS12ESPVKItem *)key;
	keyDupKey = &espvk->duplicate;
	keyProbCert = &espvk->problem_cert;
	keyNCerts = &espvk->nCerts;
	supData = &espvk->espvkData;
	keyName = &supData->nickname;
    } else {
	SEC_PKCS12PrivateKey *pvk = (SEC_PKCS12PrivateKey *)key;
	keyDupKey = &pvk->duplicate;
	keyProbCert = &pvk->problem_cert;
	keyNCerts = &pvk->nCerts;
	supData = &pvk->pvkData;
	keyName = &supData->nickname;
    }

    /* get certificate information */
    derCert = cert->value.x509->derLeafCert;
    certDupCert = &cert->duplicate;
    certName = &cert->nickname;

    /* key must have associated certs */
    if(supData->assocCerts == NULL) {
	PORT_SetError(SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE);
	return SECFailure;
    }

    /* copy the nickname, including a NULL if necessary */
    rv = sec_pkcs12_copy_nickname(poolp, certName, keyName);
    if(rv != SECSuccess) {
	return SECFailure;
    }

    /* update state if this is the inital key */
    if(initKey) {
	*keyNCerts = 0;
	*keyDupKey = *keyProbCert = *certDupCert = PR_FALSE;
	if(supData->assocCerts[*keyNCerts]) {
	   while(supData->assocCerts[*keyNCerts] != NULL) {
		(*keyNCerts)++;
	   }
	}
    }

    keyDone = PR_FALSE;
    /* validate the key first */
    rv = sec_pkcs12_validate_key_by_cert(derCert, keyDupKey, keyProbCert, 
    					 slot, wincx);
    if(rv == SECSuccess) {
	keyDone = PR_TRUE;
	/* validate the certificate */
	rv = sec_pkcs12_validate_cert(derCert, certDupCert, keyProbCert, 
				      certName, slot, poolp, ncCall, wincx);
	if((rv == SECSuccess) && 
		(SECITEM_CompareItem(certName, keyName) != SECEqual)) {
	    rv = sec_pkcs12_copy_nickname(poolp, certName, keyName);
	}
    }

    if(rv != SECSuccess) {
	if(keyDone == PR_FALSE) {
	    PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY);
	}
	/* otherwise error already set */
    } else if((*keyDupKey == PR_TRUE) && (*certDupCert == PR_TRUE)) {
	(*dupCnt)++;
    }

    return rv;
}

static PRBool
sec_pkcs12_validate_safe_keys(SEC_PKCS12SafeContents *safe, 
			      SEC_PKCS12Baggage *baggage,
			      SEC_PKCS12NicknameCollisionCallback ncCall,
			      PK11SlotInfo *slot,
			      void *wincx, int *keyCount)
{
    SEC_PKCS12PrivateKeyBag *keyBag;
    int i, j;
    SEC_PKCS12PrivateKey *pk;
    SEC_PKCS12CertAndCRL *cert;
    SECStatus rv;
    int nKeys, dupKeys;;

    /* nothing in safe contents */
    if((safe == NULL) || (safe->contents == NULL)) {
	return PR_TRUE;
    }

    i = 0;
    nKeys = dupKeys = 0;
    while(safe->contents[i] != NULL) {
	SECOidTag bagType = SECOID_FindOIDTag(&safe->contents[i]->safeBagType);

    	/* only look through key bags */
	if(bagType == SEC_OID_PKCS12_KEY_BAG_ID) {
	    j = 0;
	    keyBag = safe->contents[i]->safeContent.keyBag;

	    /* search through the keys */
	    while(keyBag->privateKeys[j] != NULL) {
		pk = keyBag->privateKeys[j];

		/* copy the nickname over */
		if(safe->old == PR_FALSE) {
		    PRBool swapDone = PR_FALSE;

redoUnicodeConversion:
		    rv = sec_pkcs12_copy_and_convert_unicode_string(safe->poolp,
		    				&pk->pvkData.nickname, 
						&pk->pvkData.uniNickName,
						safe->swapUnicode);
		    if(rv != SECSuccess) {
			return PR_FALSE;
		    /* a null password may have resulted in an inability
		     * to detect unicode conversion, try recheck here...
		     */
		    } else if(safe->possibleSwapUnicode && !swapDone) {
			if((pk->pvkData.nickname.data[0] == '?') &&
		    	   ((pk->pvkData.uniNickName.data[0] != '?') && 
		    	    (pk->pvkData.uniNickName.data[1] != '?'))) {
			    swapDone = PR_TRUE;
			    safe->swapUnicode = !safe->swapUnicode;
			    goto redoUnicodeConversion;
			}
		    /* a null password may have resulted in an inability
		     * to detect unicode conversion, if recheck also fails,
		     * assume no conversion needed.
		     */
		    } else if(safe->possibleSwapUnicode && swapDone) {
			if(pk->pvkData.nickname.data[0] == '?') {
			    safe->possibleSwapUnicode = PR_FALSE;
			    safe->swapUnicode = !safe->swapUnicode;
			    goto redoUnicodeConversion;
			}
		    }

		}

		/* tack on an extra NULL */
		sec_pkcs12_copy_nickname(safe->poolp, &pk->pvkData.nickname,
					 &pk->pvkData.nickname);

		nKeys++;

		/* tack on a NULL */
		/* check for associated certs */ 
		if(pk->pvkData.assocCerts == NULL) {
		    pk->duplicate = PR_TRUE;
		    continue;
		    /* XXX should the ASN1 decoder create single item (NULL)
		     * lists rather than setting the list to NULL? 
		     * for now, mark the key as duplicate so it will not be
		     * installed.  keys and certs should be associated.
		     */
		}

		cert = sec_pkcs12_find_object(safe, baggage, 
					    SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID,
					    NULL, pk->pvkData.assocCerts[0]);
		if(cert != NULL) {
		    rv = sec_pkcs12_validate_cert_and_key(PR_FALSE, PR_TRUE,
		    				pk, cert, &dupKeys, slot, safe->poolp,
		    				ncCall, wincx);
		} else {
		    rv = SECFailure;
		}

		if(rv == SECFailure) {
		    return PR_FALSE;
		}

		j++;
	    }
	}

	i++;
    }

    if((nKeys > 0) && (nKeys == dupKeys)) {
	PORT_SetError(SEC_ERROR_PKCS12_DUPLICATE_DATA);
	return PR_FALSE;
    }

    *keyCount += nKeys;
    return PR_TRUE;
}

static PRBool
sec_pkcs12_validate_baggage_keys(SEC_PKCS12SafeContents *safe, 
				 SEC_PKCS12Baggage *baggage,
				 SEC_PKCS12NicknameCollisionCallback ncCall,
				 PK11SlotInfo *slot,
				 void *wincx, int *keyCount)
{
    int i, j;
    SEC_PKCS12CertAndCRL *cert;
    SEC_PKCS12ESPVKItem *espvk;
    SEC_PKCS12BaggageItem *bag;
    SECStatus rv;
    int nKeys, dupKeys;

    i = 0;
    nKeys = dupKeys = 0;

    if((baggage == NULL) || (slot == NULL)) {
	return PR_FALSE;
    }

    /* step through keys stored in baggage */
    while(baggage->bags[i] != NULL) {

	/* want to skip over bags that have no keys */
	if(baggage->bags[i]->espvks == NULL) {
	    i++;
	    continue;
	}

	bag = baggage->bags[i];
	j = 0;

	/* go through each shrouded key */
	while(bag->espvks[j] != NULL) {
	    espvk = bag->espvks[j];
	    nKeys++;

	    if(safe->old == PR_FALSE) {
		PRBool swapDone = PR_FALSE;
redoUnicodeConversion:
		rv = sec_pkcs12_copy_and_convert_unicode_string(baggage->poolp,
		    			&espvk->espvkData.nickname, 
		    			&espvk->espvkData.uniNickName,
					safe->swapUnicode);
		if(rv != SECSuccess) {
		    return PR_FALSE;
		} else if(safe->possibleSwapUnicode && !swapDone) {
		    /* a null password may have resulted in an inability
		     * to detect unicode conversion, try recheck here...
		     */
		    if((espvk->espvkData.nickname.data[0] == '?') &&
		       ((espvk->espvkData.uniNickName.data[0] != '?') && 
		        (espvk->espvkData.uniNickName.data[1] != '?'))) {
			swapDone = PR_TRUE;
			safe->swapUnicode = !safe->swapUnicode;
			goto redoUnicodeConversion;
		    }
		} else if(safe->possibleSwapUnicode && swapDone) {

		   /* if the check results in a '?' as well, then we might
		    * as well assume no conversion needed 
		    */
		   if(espvk->espvkData.nickname.data[0] == '?') {
			safe->possibleSwapUnicode = PR_FALSE;
			safe->swapUnicode = !safe->swapUnicode;
			goto redoUnicodeConversion;
		   }
		}
	    }

	    /* tack on a null */
	    sec_pkcs12_copy_nickname(baggage->poolp, &espvk->espvkData.nickname,
	    			     &espvk->espvkData.nickname);

	    if(espvk->espvkData.assocCerts == NULL) {
		espvk->duplicate = PR_TRUE;
		continue;
		/* XXX should the ASN1 decoder create single item (NULL)
		 * lists rather than setting the list to NULL? 
		 * for now, mark the key as duplicate so it will not be
		 * installed.  keys and certs should be associated.
		 */
	    }

	    if(SECOID_FindOIDTag(&espvk->espvkOID) == 
			SEC_OID_PKCS12_PKCS8_KEY_SHROUDING) {
		SECAlgorithmID *algid;
		
		algid = &(espvk->espvkCipherText.pkcs8KeyShroud->algorithm);
		if(!SEC_PKCS5IsAlgorithmPBEAlg(algid)) {
		    PORT_SetError(SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM);
		    return PR_FALSE;
		}
	    } else {
		PORT_SetError(SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE);
		return PR_FALSE;
	    }

	    cert = sec_pkcs12_find_object(safe, baggage, 
					 SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID,
					 NULL, espvk->espvkData.assocCerts[0]);
	    if(cert != NULL) {
		rv = sec_pkcs12_validate_cert_and_key(PR_TRUE, PR_TRUE, espvk, cert,
		    					&dupKeys, slot, baggage->poolp,
		    					ncCall, wincx);
		if(rv == SECFailure) {
		    return PR_FALSE;
		}
	    } else {
		return PR_FALSE;
	    }
	    j++;
	}

	i++;
    }

    if((nKeys > 0) && (nKeys == dupKeys)) {
	PORT_SetError(SEC_ERROR_PKCS12_DUPLICATE_DATA);
	return PR_FALSE;
    }

    *keyCount += nKeys;
    return PR_TRUE;
}

static PRBool 
sec_pkcs12_validate(SEC_PKCS12SafeContents *safe,
		    SEC_PKCS12Baggage *baggage,
		    SEC_PKCS12NicknameCollisionCallback ncCall,
		    PK11SlotInfo *slot,
		    void *wincx)
{
    PRBool safe_valid = PR_FALSE, bag_valid = PR_FALSE;
    int keyCount = 0;

    /* try validating only if safe is non NULL, assume empty safe and 
     * success otherwise.
     */
    safe_valid = sec_pkcs12_validate_safe_keys(safe, baggage, ncCall, slot, 
    					       wincx, &keyCount);
    if(safe_valid == PR_TRUE) {
	if(baggage->bags != NULL) {
	    bag_valid = sec_pkcs12_validate_baggage_keys(safe, baggage, 
	    						 ncCall, slot, 
	    						 wincx, &keyCount);
	} else {
	    bag_valid = PR_TRUE;
	}
    }

    if((bag_valid != PR_TRUE) || (safe_valid != PR_TRUE)) {
	return PR_FALSE;
    }

    /* want to handle the case where there are no keys to be
     * imported.
     */
    if(keyCount == 0) {
	PORT_SetError(SEC_ERROR_PKCS12_DECODING_PFX);
	return PR_FALSE;
    }

    return PR_TRUE;
}

static SECStatus
sec_pkcs12_add_certificate_for_key(SEC_PKCS12CertAndCRL *p12cert,
				   PK11SlotInfo *slot,
				   void *wincx)
{
    CERTCertificate *cert = NULL;
    CERTCertDBHandle *certdb = NULL;
    SECItem **der_certs;
    SECStatus rv;
    unsigned int ncerts;

    if((p12cert == NULL) || (slot == NULL)) {
	return SECFailure;
    }

    if(p12cert->duplicate == PR_TRUE) {
	return SECSuccess;
    }

    certdb = CERT_GetDefaultCertDB();
    if(certdb == NULL) {
	return SECFailure;
    }

    /* if certs are null, error occurred.  
     */
    der_certs = SEC_PKCS7GetCertificateList(&p12cert->value.x509->certOrCRL);
    if(der_certs == NULL) {
	return SECFailure;
    }

    /* count certs in chain */
    ncerts = 0;
    while(der_certs[ncerts] != NULL) {
	ncerts++;
    }

    /* store CA chain into cert database */
    rv = CERT_ImportCerts(certdb, certUsageUserCertImport, ncerts, der_certs, 
    			  NULL, PR_TRUE, PR_TRUE, 
    			  (char *)p12cert->nickname.data);
    if(rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_PKCS12_IMPORTING_CERT_CHAIN);
	return SECFailure;
    }

    cert = CERT_NewTempCertificate(certdb, p12cert->value.x509->derLeafCert,
    				   NULL, PR_FALSE, PR_TRUE);
    if(cert == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }

    rv = PK11_ImportCertForKeyToSlot(slot, cert, (char *)p12cert->nickname.data, wincx);
    if(PK11_IsInternal(slot)) {
	rv = CERT_ChangeCertTrustByUsage(certdb, cert, certUsageUserCertImport);
    }

    if(cert != NULL) {
	CERT_DestroyCertificate(cert);
    }

    return rv;
}
    
/* install all the certificates associated with a given key
 * wrappedKey inidicates if the key is encrypted
 * key is the key data,
 * safe_contents and baggage are used to look up the certificates 
 * pwitem is the password for encrypted keys 
 * slot is the destination token
 * wincx is the window handle
 * 
 * if an error occurs, SECFailure is returned and an error code set
 */
static SECStatus
sec_pkcs12_install_key_and_certs(PRBool wrappedKey, void *key,
				 SEC_PKCS12SafeContents *safe_contents,
				 SEC_PKCS12Baggage *baggage,
				 SECItem *pwitem,
				 PK11SlotInfo *slot,
				 void *wincx)
{
    SECItem *nickname = NULL;
    SGNDigestInfo **assocCerts;
    SECStatus rv = SECFailure; 
    SEC_PKCS12CertAndCRL *cert;
    int i;

    if(key == NULL) {
	return SECFailure;
    }

    /* import key */
    if(wrappedKey == PR_TRUE) {
	SEC_PKCS12ESPVKItem *espvk = (SEC_PKCS12ESPVKItem *)key;
	nickname = &espvk->espvkData.nickname;
	assocCerts = espvk->espvkData.assocCerts;
	/* do not want to install duplicate keys */
	if(espvk->duplicate != PR_TRUE) {
	    rv = PK11_ImportEncryptedPrivateKeyInfo(slot, 
					espvk->espvkCipherText.pkcs8KeyShroud,
					pwitem, nickname, wincx);
	} else {
	    rv = SECSuccess;
	}
    } else {
	SEC_PKCS12PrivateKey *pk = (SEC_PKCS12PrivateKey *)key;
	nickname = &pk->pvkData.nickname;
	assocCerts = pk->pvkData.assocCerts;
	/* do not want to install duplicate keys */
	if(pk->duplicate != PR_TRUE) {
	    rv = PK11_ImportPrivateKeyInfo(slot, &pk->pkcs8data, nickname, wincx);
	} else {
	    rv = SECSuccess;
	}
	/* zero out unencrypted private key */
	PORT_Memset((char *)pk->pkcs8data.privateKey.data, 0, 
		    pk->pkcs8data.privateKey.len);
    }

    if(rv == SECFailure) {
	PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY);
	return SECFailure;
    }

    i = 0;

    /* loop through all certificates and install them */
    /* XXX artificially only install one certificate -- 
     * check what needs to be done to support more than one
     */
    while((assocCerts[i] != NULL) && (i == 0)) {
	cert = sec_pkcs12_find_object(safe_contents, baggage, 
				    SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID,
				    NULL, assocCerts[i]); 
	if(cert == NULL) {
	    return SECFailure;
	}
	rv = sec_pkcs12_add_certificate_for_key(cert, slot, wincx);
	if(rv == SECFailure) {
	    return SECFailure;
	}
	i++;
    }

    return SECSuccess;
}

/* install keys and certs where the keys are stored in the 
 * safe contents area
 * safe_contents -- package of certs and keys
 * baggage -- package of external keys and other secrets 
 * slot -- destination token
 * wincx -- window handle
 * 
 * if an error occurs, SECFailure returned
 */
static SECStatus
sec_pkcs12_install_safe_keys_and_certs(SEC_PKCS12SafeContents *safe_contents,
				       SEC_PKCS12Baggage *baggage,
				       PK11SlotInfo *slot,
			               void *wincx)
{
    SEC_PKCS12PrivateKeyBag *keyBag;
    int i, j;
    SEC_PKCS12PrivateKey *pk;
    SECStatus rv;

    if(safe_contents == NULL) {
	return SECFailure;
    }

    i = 0;
    if(safe_contents->contents == NULL) {
	/* XXX should the decode produce a single element NULL list? 
	 * for now assume no.  if the answer becomes yes, the two lines 
	 * should be added.
	PORT_SetError(SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE);
	return SECFailure;
	 */
    } else {
	while(safe_contents->contents[i] != NULL) {

	    /* find key bag */
	    if(SECOID_FindOIDTag(&safe_contents->contents[i]->safeBagType) ==
		SEC_OID_PKCS12_KEY_BAG_ID) {

		j = 0;
		keyBag = safe_contents->contents[i]->safeContent.keyBag;

		/* install all keys in bag */
		while(keyBag->privateKeys[j] != NULL) {
		    pk = keyBag->privateKeys[j];
		    rv = sec_pkcs12_install_key_and_certs(PR_FALSE, pk, safe_contents,
		    					  baggage, NULL, slot, wincx);
		    if(rv == SECFailure) {
			return SECFailure;
		    }
		    j++;
		}
	    }
	    i++;
	}
    }

    return SECSuccess;
}

/* install keys and certs stored in external shrouding
 * safe_contents -- package of certs and secrets
 * baggage -- contains external keys 
 * pwitem -- password
 * slot -- destination tokey
 * wincx -- window handle 
 *
 * if an error occurs, return SECFailure and set error 
 */
static SECStatus
sec_pkcs12_install_baggage_keys_and_certs(SEC_PKCS12SafeContents *safe_contents,
					  SEC_PKCS12Baggage *baggage,
					  SECItem *pwitem,
					  PK11SlotInfo *slot,
					  void *wincx)
{
    SEC_PKCS12ESPVKItem *espvk;
    SEC_PKCS12BaggageItem *bag;
    int i, j;
    PRBool problem = PR_FALSE;
    SECStatus rv;

    if(baggage == NULL) {
	return SECFailure;
    }

    i = 0;
    if(baggage->bags == NULL) {
	/* XXX should the decode produce a single element NULL list? 
	 * for now assume no.  if the answer becomes yes, the two lines 
	 * should be added.
	PORT_SetError(SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE);
	return SECFailure;
	 */
    } else {
	/* look through each bag */
	while(baggage->bags[i] != NULL) {
	    bag = baggage->bags[i];
	    if(bag->espvks == NULL) {
		i++;
		continue;
		/* XXX see comment above */
	    }

	    j = 0;
	    /* install keys */
	    while(bag->espvks[j] != NULL) {
		espvk = bag->espvks[j];
		rv = sec_pkcs12_install_key_and_certs(PR_TRUE, espvk, safe_contents,
							baggage, pwitem, slot, wincx);
		if(rv == SECFailure) {
		    return SECFailure;
		}
		j++;
	    }
	    i++;
	}
    }

    return SECSuccess;
}

/* install all keys and certs 
 * safe_contens and baggage are cert and key containers
 * pwitem is the password prtecting external keys
 * slot is the destination token
 * wincx is the window handler pointer
 *
 * on failure, an error is set and SECFailure returned
 */
static SECStatus
sec_pkcs12_install(SEC_PKCS12SafeContents *safe_contents,
		   SEC_PKCS12Baggage *baggage,
		   SECItem *pwitem,
		   PK11SlotInfo *slot,
		   void *wincx)
{
    SECStatus rv;

    /* one source must not be NULL */
    if((safe_contents == NULL) && (baggage == NULL)) {
	return SECFailure;
    }

    /* install keys stored in safe */
    if(safe_contents != NULL) {
	rv = sec_pkcs12_install_safe_keys_and_certs(safe_contents, 
						    baggage, slot, wincx);
    } else {
	rv = SECSuccess;
    }
   
    /* install keys stored in external baggage */ 
    if((rv == SECSuccess) && (baggage != NULL)) {
	if(baggage->bags != NULL) {
	    rv = sec_pkcs12_install_baggage_keys_and_certs(safe_contents,
	    						   baggage, 
	    						   pwitem,
	    						   slot, wincx);
	} else {
	    rv = SECSuccess;
	}
    }

    return rv;
}

/* process certs and keys -- validate them then install them
 * safe_contents and baggage are containers for certs and keys
 * pwitem is password protecting data and integrity
 * ncCall is nickname collision callback function
 * slot is destination token
 * wincx window handler 
 *
 * error is set and SECFailure returned when error occurs
 */
static SECStatus
sec_pkcs12_process(SEC_PKCS12SafeContents *safe_contents,
		   SEC_PKCS12Baggage *baggage,
		   SECItem *pwitem,
		   SEC_PKCS12NicknameCollisionCallback ncCall,
		   PK11SlotInfo *slot,
		   void *wincx)
{
    SECStatus rv;
    PRBool valid = PR_FALSE;

    /* validate then install */
    valid = sec_pkcs12_validate(safe_contents, baggage, ncCall, slot, wincx);
    if(valid == PR_TRUE) {
	rv = sec_pkcs12_install(safe_contents, baggage, pwitem, slot, wincx);
    } else {
	rv = SECFailure;
    }

    return rv;
}

/* import PFX item 
 * der_pfx is the der encoded pfx strcture
 * pbef and pbearg are the integrity/encryption password call back
 * ncCall is the nickname collision calllback
 * slot is the destination token
 * wincx window handler
 *
 * on error, error code set and SECFailure returned 
 */
SECStatus
SEC_PKCS12PutPFX(SECItem *der_pfx, 
		 SEC_PKCS5GetPBEPassword pbef, void *pbearg,
		 SEC_PKCS12NicknameCollisionCallback ncCall,
		 PK11SlotInfo *slot,
		 void *wincx)
{
    SEC_PKCS12PFXItem *pfx;
    SEC_PKCS12AuthenticatedSafe *asafe;
    SEC_PKCS12SafeContents *safe_contents = NULL;
    SECStatus rv;
    SECItem *pwitem = NULL;

    if(der_pfx == NULL) {
	return SECFailure;
    }

    /* if NULL slot, then install locally */
    if(slot == NULL) {
	slot = PK11_GetInternalKeySlot();
	if(slot == NULL) {
	    return SECFailure;
	}
    }

    /* get passwords */
    pwitem = (*pbef)(pbearg);
    if(pwitem == NULL) {
	return SECFailure;
    }

    /* decode and validate each section */
    rv = SECFailure;

    pfx = sec_pkcs12_get_pfx(der_pfx, pwitem);
    if(pfx != NULL) {
	asafe = sec_pkcs12_get_auth_safe(pfx);
	if(asafe != NULL) {

	    /* decrypt safe -- only if not empty */
	    if(asafe->emptySafe != PR_TRUE) {
		rv = sec_pkcs12_decrypt_auth_safe(asafe, pwitem, wincx);
		if(rv == SECSuccess) {
		    safe_contents = sec_pkcs12_get_safe_contents(asafe);
		    if(safe_contents == NULL) {
			rv = SECFailure;
		    }
		}
	    } else {
		safe_contents = sec_pkcs12_create_safe_contents(asafe->poolp);
		safe_contents->swapUnicode = pfx->swapUnicode;
		if(safe_contents == NULL) {
		    rv = SECFailure;
		} else {
		    rv = SECSuccess;
		}
	    }

	    /* get safe contents and begin import */
	    if(rv == SECSuccess) {
		if(pwitem->len == 0) {
		    safe_contents->possibleSwapUnicode = PR_TRUE;
		} else {
		    safe_contents->possibleSwapUnicode = PR_FALSE;
		}

		rv = sec_pkcs12_process(safe_contents, 
					&asafe->baggage, 
					pwitem,
					ncCall, slot, wincx);
	    }

	}
	SEC_PKCS12DestroyPFX(pfx);
    }

    SECITEM_ZfreeItem(pwitem, PR_TRUE);

    return rv;
}

PRBool 
SEC_PKCS12ValidData(char *buf, int bufLen, long int totalLength)
{
    long int innerLen = 0, objectLen = 0;
    int lengthLength;

    PRBool valid = PR_FALSE;

    if(buf == NULL) {
	return PR_FALSE;
    }

    /* check for constructed sequence identifier tag */
    if(*buf == (SEC_ASN1_CONSTRUCTED | SEC_ASN1_SEQUENCE)) {
	totalLength--;   /* header byte taken care of */
	buf++;

	lengthLength = (long int)SEC_ASN1LengthLength(totalLength - 1);
	if(totalLength > 0x7f) {
	    lengthLength--;
	    *buf &= 0x7f;  /* remove bit 8 indicator */
	    if((*buf - (char)lengthLength) == 0) {
		valid = PR_TRUE;
	    }
	} else {
	    lengthLength--;
	    if((*buf - (char)lengthLength) == 0) {
		valid = PR_TRUE;
	    }
	}
    }

    return valid;
}
