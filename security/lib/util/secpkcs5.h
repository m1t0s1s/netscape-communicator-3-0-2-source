#ifndef _SECPKCS5_H_
#define _SECPKCS5_H_

#include "secoid.h"
#include "secitem.h"
#include "prarena.h"

typedef SECItem * (* SEC_PKCS5GetPBEPassword)(void *arg);

extern const SEC_ASN1Template SEC_PKCS5PBEParameterTemplate[];
typedef struct SEC_PKCS5PBEParameterStr SEC_PKCS5PBEParameter;

struct SEC_PKCS5PBEParameterStr
{
    PRArenaPool *poolp;
    SECItem	salt;		/* octet string */
    SECItem	iteration;	/* integer */
    SECItem	keyLength;	/* optional key length */
    SECItem	iv;		/* optional iv */
    SECItem 	extendedParams;	/* optional additional params */

    /* used locally */
    SECOidTag	algorithm;
    int		iter;
};

/* Create a PKCS5 Algorithm ID
 * The algorithm ID is set up using the PKCS #5 parameter structure
 *  algorithm is the PBE algorithm ID for the desired algorithm
 *  salt can be specified or can be NULL, if salt is NULL then the
 *    salt is generated from random bytes
 *  iteration is the number of iterations for which to perform the
 *    hash prior to key and iv generation.
 * If an error occurs or the algorithm specified is not supported 
 * or is not a password based encryption algorithm, NULL is returned.
 * Otherwise, a pointer to the algorithm id is returned.
 */
extern SECAlgorithmID *
SEC_PKCS5CreateAlgorithmID(SECOidTag algorithm,
			   SECItem *salt, 
			   int iteration,
			   int keyLength,
			   SECItem *iv,
			   SECItem *param);

/* Get the initialization vector.  The password is passed in, hashing
 * is performed, and the initialization vector is returned.
 *  algid is a pointer to a PBE algorithm ID
 *  pwitem is the password
 * If an error occurs or the algorithm id is not a PBE algrithm,
 * NULL is returned.  Otherwise, the iv is returned in a secitem.
 */
extern SECItem *
SEC_PKCS5GetIV(SECAlgorithmID *algid, 
				SECItem *pwitem);

/* Get the key.  The password is passed in, hashing is performed,
 * and the key is returned.
 *  algid is a pointer to a PBE algorithm ID
 *  pwitem is the password
 * If an error occurs or the algorithm id is not a PBE algrithm,
 * NULL is returned.  Otherwise, the key is returned in a secitem.
 */
extern SECItem *
SEC_PKCS5GetKey(SECAlgorithmID *algid, 
		SECItem *pwitem);

/* Get PBE salt.  The salt for the password based algorithm is returned.
 *  algid is the PBE algorithm identifier
 * If an error occurs NULL is returned, otherwise the salt is returned
 * in a SECItem.
 */
extern SECItem *
SEC_PKCS5GetSalt(SECAlgorithmID *algid);

/* Encrypt/Decrypt data using password based encryption.  
 *  algid is the PBE algorithm identifier,
 *  pwitem is the password,
 *  src is the source for encryption/decryption,
 *  encrypt is PR_TRUE for encryption, PR_FALSE for decryption.
 * The key and iv are generated based upon PKCS #5 then the src
 * is either encrypted or decrypted.  If an error occurs, NULL
 * is returned, otherwise the ciphered contents is returned.
 */
extern SECItem *
SEC_PKCS5CipherData(SECAlgorithmID *algid, SECItem *pwitem,
		    SECItem *src, PRBool encrypt, PRBool *update);

/* Checks to see if algid algorithm is a PBE algorithm.  If
 * so, PR_TRUE is returned, otherwise PR_FALSE is returned.
 */
extern PRBool 
SEC_PKCS5IsAlgorithmPBEAlg(SECAlgorithmID *algid);

/* Destroys PBE parameter */
extern void
SEC_PKCS5DestroyPBEParameter(SEC_PKCS5PBEParameter *param);

/* Convert Algorithm ID to PBE parameter */
extern SEC_PKCS5PBEParameter *
SEC_PKCS5GetPBEParameter(SECAlgorithmID *algid);

/* Determine how large the key generated is */
extern int
SEC_PKCS5GetKeyLength(SECAlgorithmID *algid);

/* map crypto algorithm to pbe algorithm, assume sha 1 hashing for DES
 */
extern SECOidTag
SEC_PKCS5GetPBEAlgorithm(SECOidTag algTag, int keyLen);

/* return the underlying crypto algorithm */
extern SECOidTag
SEC_PKCS5GetCryptoAlgorithm(SECAlgorithmID *algid);
#endif
