#ifndef _PKCS12_H_
#define _PKCS12_H_

#include "pkcs12t.h"
#include "secpkcs5.h"

typedef SECItem * (* SEC_PKCS12GetPassword)(void *arg);
typedef SECItem * (* SEC_PKCS12NicknameCollisionCallback)(SECItem *old_nickname,
							  PRBool *cancel,
							  void *arg);

/* Decode functions */
/* Import a PFX item.  
 *	der_pfx is the der-encoded pfx item to import.
 *	pbef, and pbefarg are used to retrieve passwords for the HMAC,
 *	    and any passwords needed for passing to PKCS5 encryption 
 *	    routines.
 *	algorithm is the algorithm by which private keys are stored in
 *	    the key database.  this could be a specific algorithm or could
 *	    be based on a global setting.
 *	slot is the slot to where the certificates will be placed.  if NULL,
 *	    the internal key slot is used.
 * If the process is successful, a SECSuccess is returned, otherwise
 * a failure occurred.
 */ 
SECStatus
SEC_PKCS12PutPFX(SECItem *der_pfx,
                 SEC_PKCS5GetPBEPassword pbef, void *pbearg,
		 SEC_PKCS12NicknameCollisionCallback ncCall,
		 PK11SlotInfo *slot,
		 void *wincx);

/* export */
/* Export a PFX item.
 *	nicknames is a null terminated list of nicknames
 *	keydb is the key database pointer
 *	pwfn, pwfnarg are the password and argument for the key database
 *	shroud_keys is PR_TRUE to shroud keys externally -- for export
 *	transportMode is the transportMode desired -- for us assumed to 
 *	    be offlineTransport.  
 * A return of NULL indicates an error.
 */
SECItem *SEC_PKCS12GetPFX(char **nicknames,
	CERTCertificate **ref_certs,
	PRBool shroud_keys, 
	SEC_PKCS5GetPBEPassword pbef, void *pbearg,
	SECOidTag transportMode, void *wincx);

/* check the first two bytes of a file to make sure that it matches
 * the desired header for a PKCS 12 file
 */
PRBool SEC_PKCS12ValidData(char *buf, int bufLen, long int totalLength);

#endif
