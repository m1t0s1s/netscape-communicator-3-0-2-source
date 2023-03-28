#ifndef _P12PLCY_H_
#define _P12PLCY_H_

#include "secoid.h"
#include "sslproto.h"

/* for the algid specified, can we decrypt it ? */
extern PRBool SEC_PKCS12DecryptionAllowed(SECAlgorithmID *algid);

/* is encryption allowed? */
extern PRBool SEC_PKCS12IsEncryptionAllowed(void);

/* get the preferred encryption algorithm */
extern SECOidTag SEC_PKCS12GetPreferredEncryptionAlgorithm(void);

/* get the stronget crypto allowed (based on order in the table */
extern SECOidTag SEC_PKCS12GetStrongestAllowedAlgorithm(void);

/* enable a cipher for encryption/decryption */
extern SECStatus SEC_PKCS12EnableCipher(long which, int on);

/* return the preferred cipher for encryption */
extern SECStatus SEC_PKCS12SetPreferredCipher(long which, int on);

#endif
