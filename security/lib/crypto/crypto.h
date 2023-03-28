#ifndef _CRYPTO_H_
#define _CRYPTO_H_
/*
 * crypto.h - public data structures and prototypes for the crypto library
 *
 * $Id: crypto.h,v 1.10.2.3 1997/04/13 22:06:13 jwz Exp $
 */

#include "prarena.h"
#include "mcom_db.h"

#include "seccomon.h"
#include "secrngt.h"
#include "secoidt.h"
#include "secdert.h"
#include "cryptot.h"
#include "keyt.h"
#include "certt.h"
#include "pkcs11.h"     /* Required for pk11_fipsPowerUpSelfTest(). */
#include "prlog.h"	/* for PR_ASSERT */

/* XXX added as temporary fix */
extern const SEC_ASN1Template sgn_DigestInfoTemplate[];

SEC_BEGIN_PROTOS

/*
** RSA encryption/decryption. When encrypting/decrypting the output
** buffer must be at least the size of the public key modulus.
*/

/*
** Create a new RSA encryption context.
**	"pub" the public key to use (iff "priv" is zero)
*/
extern RSAPublicContext *RSA_CreatePublicContext(SECKEYLowPublicKey *pub);

/*
** Create a new RSA decryption context.
**	"priv" the private key to use (iff "pub" is zero)
*/
extern RSAPrivateContext *RSA_CreatePrivateContext(SECKEYLowPrivateKey *priv);

/*
** Destroy the given RSA encryption/decryption context.
**	"cx" the context
*/
extern void RSA_DestroyPublicContext(RSAPublicContext *cx);
extern void RSA_DestroyPrivateContext(RSAPrivateContext *cx);

/*
** Format some data into a PKCS#1 encryption block, preparing the
** data for RSA encryption.
**	"result" where the formatted block is stored (memory is allocated)
**	"modulusLen" the size of the formatted block
**	"blockType" what block type to use (SEC_RSABlock*)
**	"data" the data to format
*/
extern SECStatus RSA_FormatBlock(SECItem *result,
				 unsigned int modulusLen,
				 RSA_BlockType blockType,
				 SECItem *data);
/*
** Similar, but just returns a pointer to the allocated memory, *and*
** will *only* format one block, even if we (in the future) modify
** RSA_FormatBlock() to loop over multiples of modulusLen.
*/
extern unsigned char *RSA_FormatOneBlock(unsigned int modulusLen,
					 RSA_BlockType blockType,
					 SECItem *data);

/*
** Encrypt a block of data using RSA encryption. The caller must
** format the block for encryption first (see RSA_FormatBlock).
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
**
** NOTE: inputLen must equal the modulusLen of the RSA key
*/
extern SECStatus RSA_PublicUpdate(RSAPublicContext *cx,
				  unsigned char *output,
				  unsigned int *outputLen,
				  unsigned int maxOutputLen,
				  unsigned char *input,
				  unsigned int inputLen);

/*
** Decrypt a block of data using RSA decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
**
** NOTE: inputLen must equal the modulusLen of the RSA key
*/
extern SECStatus RSA_PrivateUpdate(RSAPrivateContext *cx,
				   unsigned char *output,
				   unsigned int *outputLen,
				   unsigned int maxOutputLen,
				   unsigned char *input,
				   unsigned int inputLen);

/*
** Finish RSA encryption/decryption. These verify that the encryption
** has finished and that the data was correctly blocked.
**	"cx" the context
*/
extern SECStatus RSA_PublicEnd(RSAPublicContext *cx);
extern SECStatus RSA_PrivateEnd(RSAPrivateContext *cx);

/*
** Generate and return a new RSA public and private key.
**	"cx" is the random number generator context
**	"keySizeInBits" is the size of the key to be generated, in bits.
**	   512, 1024, etc.
**	"publicExponent" when not NULL is a pointer to some data that
**	   represents the public exponent to use. The data is a byte
**	   encoded integer, in "big endian" order.
*/
extern SECKEYLowPrivateKey *RSA_NewKey(RNGContext *cx,
				    int keySizeInBits,
				    SECItem *publicExponent);

/*
** Perform a public-key operation (encryption or decryption both work!)
*/
extern SECStatus RSA_PublicKeyOp(SECKEYLowPublicKey *key,
				 unsigned char *output,
				 unsigned char *input,
				 unsigned int modulus_len);

/*
** Perform a private-key operation (encryption or decryption both work!)
*/
extern SECStatus RSA_PrivateKeyOp(SECKEYLowPrivateKey *key,
				  unsigned char *output,
				  unsigned char *input,
				  unsigned int modulus_len);

/*
 * convenience wrappers for doing single RSA operations. They create the
 * RSA context internally and take care of the formatting
 * requirements. Blinding happens automagically within RSA_SignHash and
 * RSA_DecryptBlock.
 */

SECStatus RSA_Sign(SECKEYLowPrivateKey *key, unsigned char *output,
		       unsigned int *outputLen, unsigned int maxOutputLen,
		       unsigned char *input, int inputLen);

SECStatus RSA_CheckSign(SECKEYLowPublicKey *key, unsigned char *sign,
			    int signLength, unsigned char *hash,
			    int hashLength);

SECStatus RSA_CheckSignRecover(SECKEYLowPublicKey *key, unsigned char *data,
    			    int *data_len, int max_output_len, 
			    unsigned char *sign,int sign_len);

SECStatus RSA_EncryptBlock(SECKEYLowPublicKey *key, unsigned char *output,
			   unsigned int *outputLen, unsigned int maxOutputLen,
			   unsigned char *input, int inputLen);

SECStatus RSA_DecryptBlock(SECKEYLowPrivateKey *key, unsigned char *output,
			   unsigned int *outputLen, unsigned int maxOutputLen,
			   unsigned char *input, int inputLen);

/*
 * added to make pkcs #11 happy
 *   RAW is RSA_X_509
 */
SECStatus RSA_SignRaw( SECKEYLowPrivateKey *key, unsigned char *output,
			 unsigned int *output_len, unsigned int maxOutputLen,
			 unsigned char *input, int input_len);

SECStatus RSA_CheckSignRaw( SECKEYLowPublicKey *key, unsigned char *sign, 
			    int sign_len, unsigned char *hash, int hash_len);

SECStatus RSA_CheckSignRecoverRaw( SECKEYLowPublicKey *key,unsigned char *data,
			    int *data_len, int max_output_len,
			    unsigned char *sign,int sign_len);

SECStatus RSA_EncryptRaw( SECKEYLowPublicKey *key, unsigned char *output,
			    unsigned int *output_len,
			    unsigned int max_output_len, 
			    unsigned char *input, int input_len);

SECStatus RSA_DecryptRaw(SECKEYLowPrivateKey *key, unsigned char *output,
			     unsigned int *output_len,
    			     unsigned int max_output_len,
			     unsigned char *input, int input_len);

/******************************************/
/*
** PQG parameters
*/
void PQG_DestroyParams(PQGParams *params);
PQGParams *PQG_DupParams(PQGParams *src);


/******************************************/
/*
** DSA signing algorithm
*/

typedef struct DSAKeyGenContextStr DSAKeyGenContext;
typedef struct DSASignContextStr DSASignContext;
typedef struct DSAVerifyContextStr DSAVerifyContext;

DSAKeyGenContext *DSA_CreateKeyGenContext(PQGParams *params);
SECStatus DSA_KeyGen(DSAKeyGenContext *context, SECKEYLowPublicKey **publicKey,
		     SECKEYLowPrivateKey **privateKey,
		     unsigned char *randomBlock);
void DSA_DestroyKeyGenContext(DSAKeyGenContext *context);
/*
** Generate and return a new DSA public and private key pair.
**	"params" is a pointer to the PQG parameters for the domain
*/
SECStatus DSA_NewKey(PQGParams *params, SECKEYLowPublicKey **pubKey,
		     SECKEYLowPrivateKey **privKey);

DSASignContext *DSA_CreateSignContext(SECKEYLowPrivateKey *key);
SECStatus DSA_Presign(DSASignContext *context, unsigned char *randomBlock);
SECStatus DSA_Sign(DSASignContext *context, unsigned char *signature,
		   unsigned int *signLength, unsigned int maxSignLength,
		   unsigned char *digest, unsigned int maxDigestLength);
void DSA_DestroySignContext(DSASignContext *context,PRBool freeit);

DSAVerifyContext *DSA_CreateVerifyContext(SECKEYLowPublicKey *key);
SECStatus DSA_Verify(DSAVerifyContext *context, unsigned char *signature,
		    unsigned int signLength, unsigned char *digest, 
		    unsigned int hashLength);
void DSA_DestroyVerifyContext(DSAVerifyContext *context,PRBool freeit);

/* ANSI X9.57 defines DSA signatures as DER encoded data.  Our DSA code (and
 * most of the rest of the world) just generates 40 bytes of raw data.  These
 * functions convert between formats.
 */
extern SECStatus DSAU_EncodeDerSig(SECItem *dest, SECItem *src);
extern SECItem *DSAU_DecodeDerSig(SECItem *item);
/* This is equivalent to VFY_VerifyData() for DER-encoded DSA signatures */
extern SECStatus DSAU_VerifyDerSignature(unsigned char *buf, int len,
					 SECKEYPublicKey *key, SECItem *sig,
					 void *wincx);
/* This is equivalent to SEC_SignData() for DER-encoded DSA signatures */
extern SECStatus DSAU_DerSignData(SECItem *res, unsigned char *buf, int len,
				  SECKEYPrivateKey *pk, SECOidTag algid);


/******************************************/
/*
** RC4 symmetric stream cypher
*/

/*
** clone an existing RC4 context
**	"rc4" is the context to clone
*/
extern RC4Context *RC4_DupContext(RC4Context *rc4);

/*
** Create a new RC4 context suitable for RC4 encryption/decryption.
**	"key" raw key data
**	"len" the number of bytes of key data
*/
extern RC4Context *RC4_CreateContext(unsigned char *key, int len);

/*
** Create a new RC4 context suitable for RC4 encryption/decryption.
** "password" is used to produce the eventual RC4 key by MD5 hashing.
*/
extern RC4Context *RC4_MakeKey(unsigned char *password);

/*
** Destroy an RC4 encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void RC4_DestroyContext(RC4Context *cx, PRBool freeit);

/*
** Perform RC4 encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC4_Encrypt(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/*
** Perform RC4 decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC4_Decrypt(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/******************************************/
/*
** RC2 symmetric block cypher
*/

/*
** Create a new RC2 context suitable for RC2 encryption/decryption.
** 	"key" raw key data
** 	"len" the number of bytes of key data
** 	"iv" is the CBC initialization vector (if mode is SEC_RC2_CBC)
** 	"mode" one of SEC_RC2 or SEC_RC2_CBC
**	"effectiveKeyLen" is some RSA thing that pkcs7 and pkcs11 know
**	   about. For most applications, it should be the same as len.
**
** When mode is set to SEC_RC2_CBC the RC2 cipher is run in "cipher block
** chaining" mode.
*/
extern RC2Context *RC2_CreateContext(unsigned char *key, unsigned int len,
		     unsigned char *iv, int mode, unsigned effectiveKeyLen);

/*
** Destroy an RC2 encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void RC2_DestroyContext(RC2Context *cx, PRBool freeit);

/*
** Perform RC2 encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC2_Encrypt(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    unsigned char *input, unsigned int inputLen);

/*
** Perform RC2 decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC2_Decrypt(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    unsigned char *input, unsigned int inputLen);

/******************************************/
/*
** DES symmetric block cypher
*/

/*
** Create a new DES context suitable for DES encryption/decryption.
** 	"key" raw key data
** 	"len" the number of bytes of key data
** 	"iv" is the CBC initialization vector (if mode is SEC_DES_CBC or
** 	   mode is DES_EDE3_CBC)
** 	"mode" one of SEC_DES, SEC_DES_CBC, SEC_DES_EDE3 or SEC_DES_EDE3_CBC
**	"encrypt" is PR_TRUE if the context will be used for encryption
**
** When mode is set to SEC_DES_CBC or SEC_DES_EDE3_CBC then the DES
** cipher is run in "cipher block chaining" mode.
*/
extern DESContext *DES_CreateContext(unsigned char *key, unsigned char *iv,
				     int mode, PRBool encrypt);

/*
** Destroy an DES encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void DES_DestroyContext(DESContext *cx, PRBool freeit);

/*
** Perform DES encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
**
** NOTE: the inputLen must be a multiple of DES_KEY_LENGTH
*/
extern SECStatus DES_Encrypt(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    unsigned char *input, unsigned int inputLen);

/*
** Perform DES decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
**
** NOTE: the inputLen must be a multiple of DES_KEY_LENGTH
*/
extern SECStatus DES_Decrypt(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    unsigned char *input, unsigned int inputLen);

/*
** Prepare a buffer for DES encryption, growing to the appropriate boundary,
** filling with the appropriate padding.
**
** NOTE: If arena is non-NULL, we re-allocate from there, otherwise
** we assume (and use) XP memory (re)allocation.
*/
extern unsigned char *DES_PadBuffer(PRArenaPool *arena,
				    unsigned char *inbuf, unsigned int inlen,
				    unsigned int *outlen);

/******************************************/
/*
** RC5 symmetric block cypher -- 64-bit block size
*/

/*
** Create a new RC5 context suitable for RC5 encryption/decryption.
** 	"key" raw key data
** 	"len" the number of bytes of key data
** 	"iv" is the CBC initialization vector (if mode is SEC_RC5_CBC)
** 	"mode" one of SEC_RC5 or SEC_RC5_CBC
**
** When mode is set to SEC_RC5_CBC the RC5 cipher is run in "cipher block
** chaining" mode.
*/
extern RC5Context *RC5_CreateContext(SECItem *key, unsigned int rounds,
				     unsigned char *iv, int mode);

/*
** Destroy an RC5 encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void RC5_DestroyContext(RC5Context *cx, PRBool freeit);

/*
** Perform RC5 encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC5_Encrypt(RC5Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    unsigned char *input, unsigned int inputLen);

/*
** Perform RC5 decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/

extern SECStatus RC5_Decrypt(RC5Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    unsigned char *input, unsigned int inputLen);


/****************************************/
/*
** RSA Key functions
*/

/*
** Create a new RSA private key (and public key). The key will be
** "keySizeInBits" bits long.
*/
extern SECKEYLowPrivateKey *RSA_CreatePrivateKey(int keySizeInBits);

/****************************************/
/*
** Digest-info functions
*/

/*
** Create a new digest-info object
** 	"algorithm" one of SEC_OID_MD2, SEC_OID_MD5, or SEC_OID_SHA1
** 	"sig" the raw signature data (from MD2 or MD5)
** 	"sigLen" the length of the signature data
**
** NOTE: this is a low level routine used to prepare some data for PKCS#1
** digital signature formatting.
**
** XXX It might be nice to combine the create and encode functions.
** I think that is all anybody ever wants to do anyway.
*/
extern SGNDigestInfo *SGN_CreateDigestInfo(SECOidTag algorithm,
					   unsigned char *sig,
					   unsigned int sigLen);

/*
** Destroy a digest-info object
*/
extern void SGN_DestroyDigestInfo(SGNDigestInfo *info);

/*
** Encode a digest-info object
**	"poolp" is where to allocate the result from; it can be NULL in
**		which case generic heap allocation (XP_ALLOC) will be used
**	"dest" is where to store the result; it can be NULL, in which case
**		it will be allocated (from poolp or heap, as explained above)
**	"diginfo" is the object to be encoded
** The return value is NULL if any error occurred, otherwise it is the
** resulting SECItem (either allocated or the same as the "dest" parameter).
**
** XXX It might be nice to combine the create and encode functions.
** I think that is all anybody ever wants to do anyway.
*/
extern SECItem *SGN_EncodeDigestInfo(PRArenaPool *poolp, SECItem *dest,
				     SGNDigestInfo *diginfo);

/*
** Decode a DER encoded digest info objct.
**  didata is thr source of the encoded digest.  
** The return value is NULL if an error occurs.  Otherwise, a
** digest info object which is allocated within it's own
** pool is returned.  The digest info should be deleted
** by later calling SGN_DestroyDigestInfo.
*/
extern SGNDigestInfo *SGN_DecodeDigestInfo(SECItem *didata);


/*
** Copy digest info.
**  poolp is the arena to which the digest will be copied.
**  a is the destination digest, it must be non-NULL.
**  b is the source digest
** This function is for copying digests.  It allows digests
** to be copied into a specified pool.  If the digest is in
** the same pool as other data, you do not want to delete
** the digest by calling SGN_DestroyDigestInfo.  
** A return value of SECFailure indicates an error.  A return
** of SECSuccess indicates no error occured.
*/
extern SECStatus  SGN_CopyDigestInfo(PRArenaPool *poolp,
					SGNDigestInfo *a, 
					SGNDigestInfo *b);

/*
** Compare two digest-info objects, returning the difference between
** them.
*/
extern SECComparison SGN_CompareDigestInfo(SGNDigestInfo *a, SGNDigestInfo *b);

/****************************************/
/*
** Signature creation operations
*/

/*
** Create a new signature context used for signing a data stream.
**	"alg" the signature algorithm to use (e.g. SEC_OID_RSA_WITH_MD5)
**	"privKey" the private key to use
*/
extern SGNContext *SGN_NewContext(SECOidTag alg, SECKEYPrivateKey *privKey);

/*
** Destroy a signature-context object
**	"key" the object
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SGN_DestroyContext(SGNContext *cx, PRBool freeit);

/*
** Reset the signing context "cx" to its initial state, preparing it for
** another stream of data.
*/
extern SECStatus SGN_Begin(SGNContext *cx);

/*
** Update the signing context with more data to sign.
**	"cx" the context
**	"input" the input data to sign
**	"inputLen" the length of the input data
*/
extern SECStatus SGN_Update(SGNContext *cx, unsigned char *input,
			   unsigned int inputLen);

/*
** Finish the signature process. Use either k0 or k1 to sign the data
** stream that was input using SGN_Update. The resulting signature is
** formatted using PKCS#1 and then encrypted using RSA private or public
** encryption.
**	"cx" the context
**	"result" the final signature data (memory is allocated)
*/
extern SECStatus SGN_End(SGNContext *cx, SECItem *result);

/*
** Sign a single block of data using private key encryption and given
** signature/hash algorithm.
**	"result" the final signature data (memory is allocated)
**	"buf" the input data to sign
**	"len" the amount of data to sign
**	"pk" the private key to encrypt with
**	"algid" the signature/hash algorithm to sign with 
**		(must be compatible with the key type).
*/
extern SECStatus SEC_SignData(SECItem *result, unsigned char *buf, int len,
			     SECKEYPrivateKey *pk, SECOidTag algid);

/*
** Sign a pre-digested block of data using private key encryption, encoding
**  The given signature/hash algorithm.
**	"result" the final signature data (memory is allocated)
**	"digest" the digest to sign
**	"pk" the private key to encrypt with
**	"algtag" The algorithm tag to encode (need for RSA only)
*/
extern SECStatus SGN_Digest(SECKEYPrivateKey *privKey,
                SECOidTag algtag, SECItem *result, SECItem *digest);

/*
** DER sign a single block of data using private key encryption and the
** MD5 hashing algorithm. This routine first computes a digital signature
** using SEC_SignData, then wraps it with an CERTSignedData and then der
** encodes the result.
**	"arena" is the memory arena to use to allocate data from
** 	"result" the final der encoded data (memory is allocated)
** 	"buf" the input data to sign
** 	"len" the amount of data to sign
** 	"pk" the private key to encrypt with
*/
extern SECStatus SEC_DerSignData(PRArenaPool *arena, SECItem *result,
				unsigned char *buf, int len,
				SECKEYPrivateKey *pk, SECOidTag algid);

/*
** Destroy a signed-data object.
**	"sd" the object
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SEC_DestroySignedData(CERTSignedData *sd, PRBool freeit);

/****************************************/
/*
** Signature verification operations
*/

/*
** Create a signature verification context.
**	"key" the public key to verify with
**	"sig" the encrypted signature data
**	"wincx" void pointer to the window context
*/
extern VFYContext *VFY_CreateContext(SECKEYPublicKey *key, SECItem *sig,
						void *wincx);

/*
** Destroy a verification-context object.
**	"cx" the context to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void VFY_DestroyContext(VFYContext *cx, PRBool freeit);

extern SECStatus VFY_Begin(VFYContext *cx);

/*
** Update a verification context with more input data. The input data
** is fed to a secure hash function (depending on what was in the
** encrypted signature data).
**	"cx" the context
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus VFY_Update(VFYContext *cx, unsigned char *input,
			    unsigned int inputLen);

/*
** Finish the verification process. The return value is a status which
** indicates success or failure. On success, the SECSuccess value is
** returned. Otherwise, SECFailure is returned and the error code found
** using PORT_GetError() indicates what failure occurred.
** 	"cx" the context
*/
extern SECStatus VFY_End(VFYContext *cx);

/*
** Verify the signature on a block of data for which we already have
** the digest. The signature data is an RSA private key encrypted
** block of data formatted according to PKCS#1.
** 	"dig" the digest
** 	"key" the public key to check the signature with
** 	"sig" the encrypted signature data
**/
extern SECStatus VFY_VerifyDigest(SECItem *dig, SECKEYPublicKey *key,
				  SECItem *sig, void *wincx);

/*
** Verify the signature on a block of data. The signature data is an RSA
** private key encrypted block of data formatted according to PKCS#1.
** 	"buf" the input data
** 	"len" the length of the input data
** 	"key" the public key to check the signature with
** 	"sig" the encrypted signature data
*/
extern SECStatus VFY_VerifyData(unsigned char *buf, int len,
				SECKEYPublicKey *key,
				SECItem *sig, void *wincx);

/****************************************/
/*
** PKCS #11 FIPS Power-Up SelfTests
*/

/*
** Power-Up selftests required for FIPS and invoked only
** under PKCS #11 FIPS mode.
*/
extern CK_RV pk11_fipsPowerUpSelfTest( void );

SEC_END_PROTOS

#endif /* _CRYPTO_H_ */
