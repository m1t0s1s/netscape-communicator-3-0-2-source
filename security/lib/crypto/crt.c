/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
#include "crypto.h"
#include "secrng.h"
#include "cmp.h"
#include "cmppriv.h"
#include "rsa.h"
#include "rsaint.h"

extern int SEC_ERROR_INPUT_LEN;
extern int SEC_ERROR_OUTPUT_LEN;
extern int SEC_ERROR_BAD_DATA;

/* RSA encryption/decryption with Chinese Remainder Theorem.
 */

static SECStatus RSA_CRT(RSAPrivateContext *, unsigned char *, unsigned int *,
			unsigned int, unsigned char *);

static SECStatus BlindingMultiply(RSAPrivateContext *, CMPInt *, CMPInt *);

/* This subroutine initializes the CRT Context, constructing the CMPInt's,
   setting them to the proper values, and allocating space for the input
   block buffer. If you call this routine you must call
   RSA_DestroyPrivateContext before exiting.
 */
RSAPrivateContext *
RSA_CreatePrivateContext(SECKEYLowPrivateKey *key)
{
    RSAPrivateContext *context;
    SECStatus status = SECSuccess;

    PORT_Assert(key->keyType == rsaKey); /* XXX RSA */

    context = (RSAPrivateContext *) PORT_ZAlloc(sizeof(RSAPrivateContext));
    if (context == NULL) goto loser;

    CMP_Constructor (&context->modulus);
    CMP_Constructor (&context->publicExponent);
    CMP_Constructor (&context->privateExponent);
    CMP_Constructor (&context->prime[0]);
    CMP_Constructor (&context->prime[1]);
    CMP_Constructor (&context->primeExponent[0]);
    CMP_Constructor (&context->primeExponent[1]);
    CMP_Constructor (&context->coefficient);
    context->mode = BLIND_BBT_MODE;

    context->input = NULL;
    context->inputLen = 0;

    /* Convert the modulus into a CMPInt. */
    status = CMP_OctetStringToCMPInt(key->u.rsa.modulus.data,
				     key->u.rsa.modulus.len,
				     &context->modulus);
    if (status != SECSuccess) goto loser;


    /* Set blockLen to be as many bytes as the modulus needs. */
    context->blockLen = BITS_TO_LEN (CMP_BitLengthOfCMPInt (&context->modulus));

    /* Allocate enough space for the input, it should never need more
       than blockLen bytes. */
    context->input = PORT_Alloc(context->blockLen);
    if (context->input == NULL) goto loser;

    /* Convert the CRT values to CMPInt.
     */
    status = CMP_OctetStringToCMPInt(key->u.rsa.publicExponent.data,
				     key->u.rsa.publicExponent.len,
				     &context->publicExponent);
    if (status != SECSuccess) goto loser;

    status = CMP_OctetStringToCMPInt(key->u.rsa.privateExponent.data,
				     key->u.rsa.privateExponent.len,
				     &context->privateExponent);
    if (status != SECSuccess) goto loser;

    status = CMP_OctetStringToCMPInt(key->u.rsa.prime[0].data,
				     key->u.rsa.prime[0].len,
				     &context->prime[0]);
    if (status != SECSuccess) goto loser;

    status = CMP_OctetStringToCMPInt(key->u.rsa.prime[1].data,
				     key->u.rsa.prime[1].len,
				     &context->prime[1]);
    if (status != SECSuccess) goto loser;

    status = CMP_OctetStringToCMPInt(key->u.rsa.primeExponent[0].data,
				     key->u.rsa.primeExponent[0].len,
				     &context->primeExponent[0]);
    if (status != SECSuccess) goto loser;

    status = CMP_OctetStringToCMPInt(key->u.rsa.primeExponent[1].data,
				     key->u.rsa.primeExponent[1].len,
				     &context->primeExponent[1]);
    if (status != SECSuccess) goto loser;

    status = CMP_OctetStringToCMPInt(key->u.rsa.coefficient.data,
				     key->u.rsa.coefficient.len,
				     &context->coefficient);
    if (status != SECSuccess) goto loser;

    return context;

loser:
    /* This does the right thing for freeing the context elements, even if
       they haven't been initialized yet. */
    if (context != NULL) RSA_DestroyPrivateContext(context);
    return NULL;
}

/* This subroutine updates the CRT process. It puts the input data into
     the input block buffer and if there is enough data there, it
     processes it by calling RSA_CRT.
 */
SECStatus
RSA_PrivateUpdate(RSAPrivateContext *context, unsigned char *partOut,
		  unsigned int *partOutLen, unsigned int maxPartOutLen,
		  unsigned char *partIn, unsigned int partInLen)
{
    SECStatus status = SECSuccess;
    unsigned int partialLen, localPartOutLen;

    /* Initialize partOutLen to zero. */
    *partOutLen = 0;

    if (context->inputLen + partInLen < context->blockLen) {
	/* Not enough to encrypt - just accumulate. */
	PORT_Memcpy((unsigned char *)(context->input + context->inputLen),
		    (unsigned char *)partIn, partInLen);
	context->inputLen += partInLen;
	return SECSuccess;
    }
  
    if (context->inputLen > 0) {
	/* Need to accumulate the rest of the block bytes into the input and
	   encrypt from there (otherwise it's OK to encrypt straight from
	   the partIn). */
	partialLen = context->blockLen - context->inputLen;
	PORT_Memcpy((unsigned char *)(context->input + context->inputLen),
		  (unsigned char *)partIn, partialLen);
	partIn += partialLen;
	partInLen -= partialLen;
    
	status = RSA_CRT(context, partOut, &localPartOutLen, maxPartOutLen,
			      context->input);
	if (status != SECSuccess) return SECFailure;

	(*partOutLen) += localPartOutLen;
	partOut += localPartOutLen;
	maxPartOutLen -= localPartOutLen;
    }

    /* Encrypt as many blocks of input as provided. */
    while (partInLen >= context->blockLen) {
	status = RSA_CRT(context, partOut, &localPartOutLen, maxPartOutLen,
			 partIn);
	if (status != SECSuccess) return SECFailure;
    
	partIn        += context->blockLen;
	partInLen     -= context->blockLen;
	(*partOutLen) += localPartOutLen;
	partOut       += localPartOutLen;
	maxPartOutLen -= localPartOutLen;
    }
  
    /* Copy remaining input bytes to the context's input buffer. */
    PORT_Memcpy((unsigned char *)context->input, partIn, partInLen);
    context->inputLen = partInLen;
    return SECSuccess;
}

/* This subroutine finalizes the CRT process, checking to make sure
 * all the data has been processed.
 */
SECStatus
RSA_PrivateEnd(RSAPrivateContext *context)
{
    if (context->inputLen != 0) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
  
    /* Restart context to accumulate a new block. */
    context->inputLen = 0;
    return SECSuccess;
}

/* This subroutine destroys the CMPInt's in the context and frees up
 * memory allocated. If you call RSA_CreatePrivateContext, you must call this
 * routine before exiting.
 */
void
RSA_DestroyPrivateContext(RSAPrivateContext *context)
{
    PORT_Assert(context != NULL);
    if (context == NULL) return;

    CMP_Destructor (&context->modulus);
    CMP_Destructor (&context->publicExponent);
    CMP_Destructor (&context->privateExponent);
    CMP_Destructor (&context->prime[0]);
    CMP_Destructor (&context->prime[1]);
    CMP_Destructor (&context->primeExponent[0]);
    CMP_Destructor (&context->primeExponent[1]);
    CMP_Destructor (&context->coefficient);

    if (context->input != NULL) {
	PORT_Memset(context->input, 0, context->blockLen);
	PORT_Free(context->input);
    }
    PORT_Free(context);
    return;
}

/* Perform RSA CRT.
 */
static SECStatus
RSA_CRT(RSAPrivateContext *context, unsigned char *output,
	unsigned int *outputLen, unsigned int maxOutputLen,
	unsigned char *input)
{
    SECStatus status = SECSuccess;
    unsigned int modulusLen;
    CMPInt inputCMP, outputCMP, blindingInverse;
    
    CMP_Constructor (&inputCMP);
    CMP_Constructor (&outputCMP);
    CMP_Constructor (&blindingInverse);

    /* Check to see if the output buffer is big enough? */
    if (context->blockLen > maxOutputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	status = SECFailure;
	goto loser;
    }

    /* Get the input into a CMPInt. */
    status = CMP_OctetStringToCMPInt(input, context->blockLen, &inputCMP);
    if (status != SECSuccess) goto loser;

    /* The input must be less than the modulus. */
    if (CMP_Compare (&inputCMP, &context->modulus) >= 0) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
	status = SECFailure;
	goto loser;
    }

    /* If the mode is blinding, multiply by the blinding factor computed
       from the input and context. */
    if (context->mode == BLIND_BBT_MODE) {
	status = BlindingMultiply(context, &blindingInverse, &inputCMP);
	if (status != SECSuccess) goto loser;
    }

    /* Chinese remainder exponentiation. */
    status = CMP_ModExpCRT(&inputCMP, &context->prime[0], &context->prime[1],
			   &context->primeExponent[0],
			   &context->primeExponent[1],
			   &context->coefficient, &outputCMP);
    if (status != SECSuccess) goto loser;

    /* If the mode is blinding, multiply by the inverse blinding factor. */
    if (context->mode == BLIND_BBT_MODE) {
	status = CMP_ModMultiply(&outputCMP, &blindingInverse,
				 &(context->modulus), &inputCMP);
	if (status != SECSuccess) goto loser;

	/* Convert back to an octet string. At this point, the result is a
	 * cryptographic data unit, not a math unit. The buffer to return
	 * must be the same size as the modulus, so if the actual value
	 * of the outputCMP is smaller, pad with leading zeros.
	 */
	modulusLen = BITS_TO_LEN (CMP_BitLengthOfCMPInt (&context->modulus));
	status = CMP_CMPIntToFixedLenOctetStr(&inputCMP, modulusLen,
					      maxOutputLen, outputLen,
					      output);
	if (status != SECSuccess) goto loser;
    } else {
	/* Convert back to an octet string. At this point, the result is a
	 * cryptographic data unit, not a math unit. The buffer to return
	 * must be the same size as the modulus, so if the actual value
	 * of the outputCMP is smaller, pad with leading zeros.
	 */
	modulusLen = BITS_TO_LEN (CMP_BitLengthOfCMPInt (&context->modulus));
	status = CMP_CMPIntToFixedLenOctetStr(&outputCMP, modulusLen,
					      maxOutputLen, outputLen,
					      output);
	if (status != SECSuccess) goto loser;
    }

loser:
    CMP_Destructor (&inputCMP);
    CMP_Destructor (&outputCMP);
    CMP_Destructor (&blindingInverse);
    return status;
}

/* This routine takes the the information in the context to build a random
 * number x. It then creates blinding factor r = x^e mod n, where e is the
 * public exponent and n the modulus in the context. The routine then
 * computes inputCMP = inputCMP * r mod n. THis is the new value to be
 * used as the input for ModExpCRT. Finally, the routine finds
 * rInverse = inverse (r) mod n, to be used after ModExpCRT to construct
 * the "real," unblinded result.
 */
static SECStatus
BlindingMultiply(RSAPrivateContext *context, CMPInt *blindingInverse,
		 CMPInt *inputCMP)
{
    SECStatus status = SECSuccess;
    unsigned int modulusLen;
    unsigned char *tempBuffer = NULL;
    CMPInt randomCMP, blindingValue;

    modulusLen = CMP_INT_LENGTH(context->modulus) * CMP_BYTES_PER_WORD;
    
    CMP_Constructor (&randomCMP);
    CMP_Constructor (&blindingValue);

    tempBuffer = PORT_Alloc(modulusLen);
    if (tempBuffer == NULL) goto loser;

    RNG_GenerateGlobalRandomBytes(tempBuffer, modulusLen);

    /* Make sure this random value is less than the modulus by setting
     * the most significant byte to 0.
     */
    tempBuffer[0] = 0;

    /* Place this random value into a CMPInt. */
    status = CMP_OctetStringToCMPInt(tempBuffer, modulusLen, &randomCMP);
    if (status != SECSuccess) goto loser;

    /* Compute blinding factor = random ^ e mod n. */
    status = CMP_ModExp(&randomCMP, &(context->publicExponent),
			&(context->modulus), &blindingValue);
    if (status != SECSuccess) goto loser;

    /* Now find the inverse of random mod n. */
    status = CMP_ModInvert(&randomCMP, &(context->modulus), blindingInverse);
    if (status != SECSuccess) goto loser;

    /* The input is converted, inputCMP = (blindingValue * inputCMP) mod n.
     * Use randomCMP as a temp CMPInt.
     */
    status = CMP_ModMultiply(&blindingValue, inputCMP, &(context->modulus),
			     &randomCMP);
    if (status != SECSuccess) goto loser;

    status = CMP_Move (&randomCMP, inputCMP);
    if (status != SECSuccess) goto loser;

loser:
    /* Zeroize the buffers. */
    if (tempBuffer != NULL) {
	PORT_Memset(tempBuffer, 0, modulusLen);
	PORT_Free(tempBuffer);
    }
    CMP_Destructor (&randomCMP);
    CMP_Destructor (&blindingValue);
    
    return status;
}
