/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#include "crypto.h"
#include "secder.h"
#include "cmp.h"
#include "rsa.h"
#include "rsakeygn.h"
#include "prime.h"

extern int SEC_ERROR_NEED_RANDOM;
extern int SEC_ERROR_INPUT_LEN;
extern int SEC_ERROR_BAD_DATA;

static SECStatus RSAParameters(RSAKeyGenContext *context,
			       CMPInt *primeP, CMPInt *primeQ);
static SECStatus SetRSAKeyGenResult(CMPInt *cmpPrimeP, CMPInt *cmpPrimeQ,
				    RSAKeyGenContext *context,
				    SECKEYLowPrivateKey *result);

/* Initialize the RSA key generating process, constructing and setting
 * CMPInt elements of the context. If you call this routine, you
 * must call RSAKeyGenContextDestroy before exiting.
 */
RSAKeyGenContext *
RSA_CreateKeyGenContext (RSAKeyGenParams *params)
{
    CMPStatus status;
    int lsBit;
    RSAKeyGenContext *context;

    PORT_Assert(params != NULL);

    context = (RSAKeyGenContext *) PORT_ZAlloc(sizeof(RSAKeyGenContext));
    if (context == NULL)
	goto loser;

    CMP_Constructor (&context->cmpModulus);
    CMP_Constructor (&context->cmpCoefficient);
    CMP_Constructor (&context->cmpPublicExponent);
    CMP_Constructor (&context->cmpPrivateExponent);
    CMP_Constructor (&context->cmpPrime1);
    CMP_Constructor (&context->cmpPrime2);
    CMP_Constructor (&context->cmpExponentP);
    CMP_Constructor (&context->cmpExponentQ);

    context->modulusBits = params->modulusBits;

    /* Copy public exponent into CMPInt */
    status = CMP_OctetStringToCMPInt(params->publicExponent.data,
				     params->publicExponent.len,
				     &context->cmpPublicExponent);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Check that the public exponent is in bounds and odd.
	 */
    if (CMP_BitLengthOfCMPInt(&context->cmpPublicExponent) >=
	(int)context->modulusBits) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	goto loser;
    }

    status = CMP_GetBit(0, &context->cmpPublicExponent, &lsBit);
    if (status != CMP_SUCCESS) goto loser;
    if (lsBit == 0) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
	goto loser;
    }

    return context;

loser:
    if (context != NULL)
	RSA_DestroyKeyGenContext(context);
    return NULL;
}

/* This generates an RSA keypair of size modulusBits with the fixed
 *   publicExponent, pointing result to the resulting integers.  The
 *   resulting integer data is in the context, so that the values must be
 *   copied before the context is zeroized.
 * All integers are unsigned canonical byte arrays with the most significant
 *   byte first.
 * The randomBlock is of length randomBlockLen returned by RSAKeyGenQuery.
 * This assumes that the modulusBits size was checked by RSAKeyGenQuery.
 */
SECKEYLowPrivateKey *
RSA_KeyGen(RSAKeyGenContext *context, unsigned char *randomBlock)
{
    CMPInt *cmpPrimeP, *cmpPrimeQ;
    SECStatus dsrv;
    unsigned int primeSizeBits, primeWords;
    SECKEYLowPrivateKey *key;
    PRArenaPool *arena;

    PORT_Assert(context != NULL);
    PORT_Assert(randomBlock != NULL);

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL )
	goto loser;

    key = (SECKEYLowPrivateKey *)PORT_ArenaZAlloc(arena, 
					sizeof(SECKEYLowPrivateKey));
    if (key == NULL)
	goto loser;
    key->arena = arena;
    key->keyType = rsaKey;

    /* prime size is half modulus size
     */
    primeSizeBits = RSA_PRIME_BITS (context->modulusBits);
    primeWords = (primeSizeBits / 16) + 1;

    /* Fish for cmpPrime1 and cmpPrime2 that are compatible with supplied
     * publicExponent.
     * The randomBlock holds random bytes for two primes.
     */
    dsrv = prm_PrimeFind(randomBlock, primeSizeBits,
			 &context->cmpPublicExponent,
			 &context->cmpPrime1);
    if (dsrv != SECSuccess)
	goto loser;

    /* should add (primeSizeBits + 7) / 8 to randomBlock, for now, make
     * sure it behaves same as old algae. */
    dsrv = prm_PrimeFind(randomBlock + (2 * primeWords),
			 context->modulusBits - primeSizeBits,
			 &context->cmpPublicExponent, &context->cmpPrime2);
    if (dsrv != SECSuccess)
	goto loser;

    /* Set cmpPrimeP to the larger of cmpPrime1 and cmpPrime2 and set
     * cmpPrimeQ to the smaller. */
    if (CMP_Compare (&context->cmpPrime1, &context->cmpPrime2) > 0) {
	cmpPrimeP = &context->cmpPrime1;
	cmpPrimeQ = &context->cmpPrime2;
    } else {
	cmpPrimeP = &context->cmpPrime2;
	cmpPrimeQ = &context->cmpPrime1;
    }

    /* Calculate the rest of the key components */
    dsrv = RSAParameters(context, cmpPrimeP, cmpPrimeQ);
    if (dsrv != SECSuccess)
	goto loser;

    /* Copy key components into canonical buffers which are at the
     * end of the context. */
    dsrv = SetRSAKeyGenResult(cmpPrimeP, cmpPrimeQ, context, key);
    if (dsrv != SECSuccess)
	goto loser;

    return key;

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_TRUE);
    return NULL;
}

/* This routine takes publicExponent, primeP and primeQ and computes the
 * rest of the RSA keypair parameters.
 */
static SECStatus
RSAParameters(RSAKeyGenContext *context, CMPInt *primeP, CMPInt *primeQ)
{
    CMPStatus status;
    CMPInt phiOfModulus;

    PORT_Assert(context != NULL);
    PORT_Assert(primeP != NULL);
    PORT_Assert(primeQ != NULL);

    /* Construct the CMPInt variable */
    CMP_Constructor (&phiOfModulus);

    /* modulus = p * q */
    status = CMP_Multiply (primeP, primeQ, &context->cmpModulus);
    if (status != CMP_SUCCESS)
	goto loser;

    /*  Find p - 1 and q - 1 */
    status = CMP_SubtractCMPWord ((CMPWord)1, primeP);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_SubtractCMPWord ((CMPWord)1, primeQ);
    if (status != CMP_SUCCESS)
	goto loser;

    /* phi = phi (modulus), where phi = Euler totient function,
     * See Knuth Vol 2 */
    status = CMP_Multiply (primeP, primeQ, &phiOfModulus);
    if (status != CMP_SUCCESS)
	goto loser;

    /* compute decryption exponent */
    status = CMP_ModInvert(&context->cmpPublicExponent, &phiOfModulus,
			   &context->cmpPrivateExponent);
    if (status != CMP_SUCCESS)
	goto loser;

    /* calc exponentP = inv (publicExponent) [mod (p - 1)]
     * and exponentQ = inv (publicExponent) [mod (q - 1)] */
    status = CMP_ModularReduce(&context->cmpPrivateExponent, primeP,
			       &context->cmpExponentP);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_ModularReduce(&context->cmpPrivateExponent, primeQ,
			       &context->cmpExponentQ);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Restore p and q; q++ & p++ */
    status = CMP_AddCMPWord ((CMPWord)1, primeP);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_AddCMPWord ((CMPWord)1, primeQ);
    if (status != CMP_SUCCESS)
	goto loser;

    /* calculate coefficient = (inv (Q) [modP]) */
    status = CMP_ModInvert(primeQ, primeP, &context->cmpCoefficient);
    if (status != CMP_SUCCESS)
	goto loser;

    CMP_Destructor (&phiOfModulus);

    return SECSuccess;

loser:
    if (status == CMP_INVERSE)
	/* CMP_ModInvert() couldn't find an inverse.  try again. */
	PORT_SetError(SEC_ERROR_NEED_RANDOM);

    return SECFailure;
}

/* Put the elements of the RSA key into a private key struct.
 */
static SECStatus
SetRSAKeyGenResult(CMPInt *cmpPrimeP, CMPInt *cmpPrimeQ,
		   RSAKeyGenContext *context, SECKEYLowPrivateKey *key)
{
    CMPStatus status;
    unsigned int primeLen, modulusLen;
    unsigned char *resultBuffer;
    SECStatus dsrv;

    PORT_Assert( cmpPrimeP != NULL);
    PORT_Assert( cmpPrimeQ != NULL);
    PORT_Assert( context != NULL);
    PORT_Assert( key != NULL);
    PORT_Assert(key->keyType == rsaKey); /* XXX RSA */

    modulusLen = BITS_TO_LEN (context->modulusBits) + 1;
    primeLen   = RSA_PRIME_LEN (context->modulusBits) + 1;
    context->bufSize = (3 * modulusLen + 5 * primeLen) *
	sizeof (unsigned char);

    resultBuffer = PORT_ArenaAlloc(key->arena, context->bufSize);
    if (resultBuffer == NULL)
	goto loser;

    /* Set up data pointers using locations in resultBuffer array */
    key->u.rsa.modulus.data = resultBuffer;
    key->u.rsa.publicExponent.data = key->u.rsa.modulus.data + modulusLen;
    key->u.rsa.privateExponent.data
	= key->u.rsa.publicExponent.data + modulusLen;
    key->u.rsa.prime[0].data
	= key->u.rsa.privateExponent.data + modulusLen;
    key->u.rsa.prime[1].data = key->u.rsa.prime[0].data + primeLen;
    key->u.rsa.primeExponent[0].data = key->u.rsa.prime[1].data + primeLen;
    key->u.rsa.primeExponent[1].data
	= key->u.rsa.primeExponent[0].data + primeLen;
    key->u.rsa.coefficient.data = key->u.rsa.primeExponent[1].data + primeLen;

    status = CMP_CMPIntToSignedOctetString(&context->cmpModulus,
					   modulusLen,
					   &key->u.rsa.modulus.len,
					   key->u.rsa.modulus.data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(&context->cmpPublicExponent,
					   modulusLen,
					   &key->u.rsa.publicExponent.len,
					   key->u.rsa.publicExponent.data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(&context->cmpPrivateExponent,
					   modulusLen,
					   &key->u.rsa.privateExponent.len,
					   key->u.rsa.privateExponent.data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(cmpPrimeP, primeLen,
					   &key->u.rsa.prime[0].len,
					   key->u.rsa.prime[0].data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(cmpPrimeQ, primeLen,
					   &key->u.rsa.prime[1].len,
					   key->u.rsa.prime[1].data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(&context->cmpExponentP, primeLen,
					   &key->u.rsa.primeExponent[0].len,
					   key->u.rsa.primeExponent[0].data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(&context->cmpExponentQ, primeLen,
					   &key->u.rsa.primeExponent[1].len,
					   key->u.rsa.primeExponent[1].data);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_CMPIntToSignedOctetString(&context->cmpCoefficient, primeLen,
					   &key->u.rsa.coefficient.len,
					   key->u.rsa.coefficient.data);
    if (status != CMP_SUCCESS)
	goto loser;
    dsrv = DER_SetUInteger(key->arena, &key->u.rsa.version,
			  SEC_PRIVATE_KEY_VERSION);
    if (dsrv != SECSuccess)
	goto loser;

    return SECSuccess;
   
loser:
    return SECFailure;
}

/* Destroy the CMPInt elements of the context.
 */
void RSA_DestroyKeyGenContext(RSAKeyGenContext *context)
{
    PORT_Assert(context != NULL);

    CMP_Destructor (&context->cmpModulus);
    CMP_Destructor (&context->cmpCoefficient);
    CMP_Destructor (&context->cmpPublicExponent);
    CMP_Destructor (&context->cmpPrivateExponent);
    CMP_Destructor (&context->cmpPrime1);
    CMP_Destructor (&context->cmpPrime2);
    CMP_Destructor (&context->cmpExponentP);
    CMP_Destructor (&context->cmpExponentQ);

    PORT_Free(context);
    return;
}
