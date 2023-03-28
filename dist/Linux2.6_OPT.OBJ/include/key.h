#ifndef _KEY_H_
#define _KEY_H_
/*
 * key.h - public data structures and prototypes for the private key library
 *
 * $Id: key.h,v 1.6.2.4 1997/05/24 00:23:19 jwz Exp $
 */

#include "prarena.h"
#include "mcom_db.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secdert.h"
#include "cryptot.h"
#include "keyt.h"
#include "certt.h"
#include "secpkcs5.h"

SEC_BEGIN_PROTOS

typedef char * (* SECKEYDBNameFunc)(void *arg, int dbVersion);
    
/*
** Open a key database.
*/
extern SECKEYKeyDBHandle *SECKEY_OpenKeyDB(PRBool readonly,
					   SECKEYDBNameFunc namecb,
					   void *cbarg);

extern SECKEYKeyDBHandle *SECKEY_OpenKeyDBFilename(char *filename,
						   PRBool readonly);

/*
** Traverse the entire key database, and pass the nicknames and keys to a 
** user supplied function.
**      "f" is the user function to call for each key
**      "udata" is the user's data, which is passed through to "f"
*/
extern SECStatus SECKEY_TraverseKeys(SECKEYKeyDBHandle *handle, 
				SECKEYTraverseKeysFunc f,
				void *udata);

/*
** Update the database
*/
extern SECStatus SECKEY_UpdateKeyDBPass1(SECKEYKeyDBHandle *handle);
extern SECStatus SECKEY_UpdateKeyDBPass2(SECKEYKeyDBHandle *handle,
					 SECItem *pwitem);

/*
** Close the specified key database.
*/
extern void SECKEY_CloseKeyDB(SECKEYKeyDBHandle *handle);

/*
** Support a default key database.
*/
extern void SECKEY_SetDefaultKeyDB(SECKEYKeyDBHandle *handle);
extern SECKEYKeyDBHandle *SECKEY_GetDefaultKeyDB(void);

/* set the alg id of the key encryption algorithm */
extern void SECKEY_SetDefaultKeyDBAlg(SECOidTag alg);

/*
 * given a password and salt, produce a hash of the password
 */
extern SECItem *SECKEY_HashPassword(char *pw, SECItem *salt);

/*
** Delete a key from the database
*/
extern SECStatus SECKEY_DeleteKey(SECKEYKeyDBHandle *handle, 
				  SECItem *pubkey);

/*
** Store a key in the database, indexed by its public key modulus.
**	"pk" is the private key to store
**	"f" is a the callback function for getting the password
**	"arg" is the argument for the callback
*/
extern SECStatus SECKEY_StoreKeyByPublicKey(SECKEYKeyDBHandle *handle, 
					    SECKEYLowPrivateKey *pk,
					    SECItem *pubKeyData,
					    char *nickname,
					    SECKEYGetPasswordKey f, void *arg);

/* does the key for this cert exist in the database filed by modulus */
extern SECStatus SECKEY_KeyForCertExists(SECKEYKeyDBHandle *handle,
					 CERTCertificate *cert);

SECKEYLowPrivateKey *
SECKEY_FindKeyByCert(SECKEYKeyDBHandle *handle, CERTCertificate *cert,
		     SECKEYGetPasswordKey f, void *arg);

extern SECStatus SECKEY_HasKeyDBPassword(SECKEYKeyDBHandle *handle);
extern SECStatus SECKEY_SetKeyDBPassword(SECKEYKeyDBHandle *handle,
				     SECItem *pwitem);
extern SECStatus SECKEY_CheckKeyDBPassword(SECKEYKeyDBHandle *handle,
					   SECItem *pwitem);
extern SECStatus SECKEY_ChangeKeyDBPassword(SECKEYKeyDBHandle *handle,
					    SECItem *oldpwitem,
					    SECItem *newpwitem);

/*
** Destroy a private key object.
**	"key" the object
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SECKEY_DestroyPrivateKey(SECKEYPrivateKey *key);
extern void SECKEY_LowDestroyPrivateKey(SECKEYLowPrivateKey *key);

/*
** Destroy a public key object.
**	"key" the object
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SECKEY_DestroyPublicKey(SECKEYPublicKey *key);
extern void SECKEY_LowDestroyPublicKey(SECKEYLowPublicKey *key);

/*
** Destroy a subject-public-key-info object.
*/
extern void SECKEY_DestroySubjectPublicKeyInfo(CERTSubjectPublicKeyInfo *spki);

/*
** Copy subject-public-key-info "src" to "dst". "dst" is filled in
** appropriately (memory is allocated for each of the sub objects).
*/
extern SECStatus SECKEY_CopySubjectPublicKeyInfo(PRArenaPool *arena,
					     CERTSubjectPublicKeyInfo *dst,
					     CERTSubjectPublicKeyInfo *src);

/*
** Return the modulus length of "pubKey".
*/
extern unsigned int SECKEY_LowPublicModulusLen(SECKEYLowPublicKey *pubKey);

/*
** Return the strength of the public key
*/
extern unsigned SECKEY_PublicKeyStrength(SECKEYPublicKey *pubk);


/*
** Return the modulus length of "privKey".
*/
extern unsigned int SECKEY_LowPrivateModulusLen(SECKEYLowPrivateKey *privKey);

/*
** Make a copy of the private key "privKey"
*/
extern SECKEYPrivateKey *SECKEY_CopyPrivateKey(SECKEYPrivateKey *privKey);

/*
** Make a copy of the public key "pubKey"
*/
extern SECKEYPublicKey *SECKEY_CopyPublicKey(SECKEYPublicKey *pubKey);

/*
** Convert a private key "privateKey" into a public key
*/
extern SECKEYPublicKey *SECKEY_ConvertToPublicKey(SECKEYPrivateKey *privateKey);

/*
** Convert a low private key "privateKey" into a public low key
*/
extern SECKEYLowPublicKey 
		*SECKEY_LowConvertToPublicKey(SECKEYLowPrivateKey *privateKey);

/*
 * create a new RSA key pair. The public Key is returned...
 */
SECKEYPrivateKey *SECKEY_CreateRSAPrivateKey(int keySizeInBits,
					   SECKEYPublicKey **pubk, void *cx);
/*
** Create a subject-public-key-info based on a public key.
*/
extern CERTSubjectPublicKeyInfo *
SECKEY_CreateSubjectPublicKeyInfo(SECKEYPublicKey *k);

/*
** Decode a DER encoded public key into an SECKEYPublicKey structure.
*/
extern SECKEYPublicKey *SECKEY_DecodeDERPublicKey(SECItem *pubkder);

/*
** Convert a base64 ascii encoded DER public key to our internal format.
*/
extern SECKEYPublicKey *SECKEY_ConvertAndDecodePublicKey(char *pubkstr);

/*
** Convert a base64 ascii encoded DER public key and challenge to spki,
** and verify the signature and challenge data are correct
*/
extern CERTSubjectPublicKeyInfo *
SECKEY_ConvertAndDecodePublicKeyAndChallenge(char *pkacstr, char *challenge,
								void *cx);

/*
** Decode a DER encoded subject public key info into a
** CERTSubjectPublicKeyInfo structure.
*/
extern CERTSubjectPublicKeyInfo *
SECKEY_DecodeDERSubjectPublicKeyInfo(SECItem *spkider);

/*
** Convert a base64 ascii encoded DER subject public key info to our
** internal format.
*/
extern CERTSubjectPublicKeyInfo *
SECKEY_ConvertAndDecodeSubjectPublicKeyInfo(char *spkistr);

/*
 * Set the Key Database password.
 *   handle is a handle to the key database
 *   pwitem is the new password
 *   algorithm is the algorithm by which the key database 
 *   	password is to be encrypted.
 * On failure, SECFailure is returned, otherwise SECSuccess is 
 * returned.
 */
extern SECStatus 
SECKEY_SetKeyDBPasswordAlg(SECKEYKeyDBHandle *handle,
			SECItem *pwitem, 
			SECOidTag algorithm);

/* Check the key database password.
 *   handle is a handle to the key database
 *   pwitem is the suspect password
 *   algorithm is the algorithm by which the key database 
 *   	password is to be encrypted.
 * The password is checked against plaintext to see if it is the
 * actual password.  If it is not, SECFailure is returned.
 */
extern SECStatus 
SECKEY_CheckKeyDBPasswordAlg(SECKEYKeyDBHandle *handle,
				SECItem *pwitem, 
				SECOidTag algorithm);

/* Change the key database password and/or algorithm by which
 * the password is stored with.  
 *   handle is a handle to the key database
 *   old_pwitem is the current password
 *   new_pwitem is the new password
 *   old_algorithm is the algorithm by which the key database 
 *   	password is currently encrypted.
 *   new_algorithm is the algorithm with which the new password
 *      is to be encrypted.
 * A return of anything but SECSuccess indicates failure.
 */
extern SECStatus 
SECKEY_ChangeKeyDBPasswordAlg(SECKEYKeyDBHandle *handle,
			      SECItem *oldpwitem, SECItem *newpwitem,
			      SECOidTag old_algorithm);

/* Store key by modulus and specify an encryption algorithm to use.
 *   handle is the pointer to the key database,
 *   privkey is the private key to be stored,
 *   f and arg are the function and arguments to the callback
 *       to get a password,
 *   algorithm is the algorithm which the privKey is to be stored.
 * A return of anything but SECSuccess indicates failure.
 */
extern SECStatus 
SECKEY_StoreKeyByPublicKeyAlg(SECKEYKeyDBHandle *handle, 
			      SECKEYLowPrivateKey *privkey, 
			      SECItem *pubKeyData,
			      char *nickname,
			      SECKEYGetPasswordKey f, void *arg,
			      SECOidTag algorithm); 

/* Find key by modulus.  This function is the inverse of store key
 * by modulus.  An attempt to locate the key with "modulus" is 
 * performed.  If the key is found, the private key is returned,
 * else NULL is returned.
 *   modulus is the modulus to locate
 */
extern SECKEYLowPrivateKey *
SECKEY_FindKeyByPublicKey(SECKEYKeyDBHandle *handle, SECItem *modulus, 
			  SECKEYGetPasswordKey f, void *arg);

/* Destroy and zero out a private key info structure.  for now this
 * function zero's out memory allocated in an arena for the key 
 * since PORT_FreeArena does not currently do this.  
 *
 * NOTE -- If a private key info is allocated in an arena, one should 
 * not call this function with freeit = PR_FALSE.  The function should 
 * destroy the arena.  
 */
extern void
SECKEY_DestroyPrivateKeyInfo(SECKEYPrivateKeyInfo *pvk, PRBool freeit);

/* Destroy and zero out an encrypted private key info.
 *
 * NOTE -- If a encrypted private key info is allocated in an arena, one should 
 * not call this function with freeit = PR_FALSE.  The function should 
 * destroy the arena.  
 */
extern void
SECKEY_DestroyEncryptedPrivateKeyInfo(SECKEYEncryptedPrivateKeyInfo *epki,
				      PRBool freeit);

/* Copy private key info structure.  
 *  poolp is the arena into which the contents of from is to be copied.
 *	NULL is a valid entry.
 *  to is the destination private key info
 *  from is the source private key info
 * if either from or to is NULL or an error occurs, SECFailure is 
 * returned.  otherwise, SECSuccess is returned.
 */
extern SECStatus
SECKEY_CopyPrivateKeyInfo(PRArenaPool *poolp,
			  SECKEYPrivateKeyInfo *to,
			  SECKEYPrivateKeyInfo *from);

/* Copy encrypted private key info structure.  
 *  poolp is the arena into which the contents of from is to be copied.
 *	NULL is a valid entry.
 *  to is the destination encrypted private key info
 *  from is the source encrypted private key info
 * if either from or to is NULL or an error occurs, SECFailure is 
 * returned.  otherwise, SECSuccess is returned.
 */
extern SECStatus
SECKEY_CopyEncryptedPrivateKeyInfo(PRArenaPool *poolp,
				   SECKEYEncryptedPrivateKeyInfo *to,
				   SECKEYEncryptedPrivateKeyInfo *from);


/* Make a copy of a low private key in it's own arena.
 * a return of NULL indicates an error.
 */
extern SECKEYLowPrivateKey *
SECKEY_CopyLowPrivateKey(SECKEYLowPrivateKey *privKey);


SEC_END_PROTOS

#endif /* _KEY_H_ */
