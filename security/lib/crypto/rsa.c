/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
#include "crypto.h"
#include "keyt.h"
#include "cmp.h"
#include "rsa.h"
#include "rsaint.h"

extern int SEC_ERROR_INPUT_LEN;
extern int SEC_ERROR_OUTPUT_LEN;

/* RSA encryption/decryption with full exponent.
 */

static SECStatus
RSA_Operation(RSAPublicContext *, unsigned char *, unsigned int *, unsigned int,
	      unsigned char *);

/* Initializes the RSA process, constructing and setting CMPInt elements
     and allocating memory in context. If you call this routine, you
     must call RSADestroyContext before exiting.
 */
RSAPublicContext *
RSA_CreatePublicContext(SECKEYLowPublicKey *key)
{
    RSAPublicContext *context;
    SECStatus status;

    PORT_Assert(key->keyType == rsaKey); /* XXX RSA */

    context = (RSAPublicContext *) PORT_ZAlloc(sizeof(RSAPublicContext));
    if (context == NULL) goto loser;

    CMP_Constructor (&context->modulus);
    CMP_Constructor (&context->publicExponent);
    context->input = NULL;

    /* Convert modulus to CMP representation. */
    status = CMP_OctetStringToCMPInt(key->u.rsa.modulus.data,
				     key->u.rsa.modulus.len,
				     &context->modulus);
    if (status != SECSuccess) goto loser;

    /* Convert exponent to CMP representation. */
    status = CMP_OctetStringToCMPInt(key->u.rsa.publicExponent.data,
				     key->u.rsa.publicExponent.len,
				     &context->publicExponent);
    if (status != SECSuccess) goto loser;

    /* Set the block update blockLen to be big enough to hold the modulus. */
    context->blockLen = BITS_TO_LEN(CMP_BitLengthOfCMPInt (&context->modulus));

    /* Allocate space for the input block, as big as the modulus. */
    context->input = PORT_Alloc (context->blockLen);
    if (context->input == NULL)	goto loser;
    
    context->inputLen = 0;
    return context;

loser:
    if (context != NULL) RSA_DestroyPublicContext(context);
    return NULL;
}

/* Update the RSA process, if there is enough data, call RSA_Operation to
     perform the ModExp, if not, simply add it to the input buffer.
 */
SECStatus
RSA_PublicUpdate(RSAPublicContext *context, unsigned char *partOut,
		 unsigned int *partOutLen, unsigned int maxPartOutLen,
		 unsigned char *partIn, unsigned int partInLen)
{
    SECStatus status;
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
	 * encrypt from there (otherwise it's OK to encrypt straight from
	 * the partIn).
	 */
	partialLen = context->blockLen - context->inputLen;
	PORT_Memcpy((unsigned char *)(context->input + context->inputLen),
		    (unsigned char *)partIn, partialLen);
	partIn    += partialLen;
	partInLen -= partialLen;

	status = RSA_Operation(context, partOut, &localPartOutLen,
			       maxPartOutLen, context->input);
	if (status != SECSuccess) return SECFailure;

	(*partOutLen) += localPartOutLen;
	partOut       += localPartOutLen;
	maxPartOutLen -= localPartOutLen;
    }

  /* Encrypt as many blocks of input as provided. */
    while (partInLen >= context->blockLen) {
	status = RSA_Operation(context, partOut, &localPartOutLen,
			       maxPartOutLen, partIn);
	if (status != SECSuccess) return SECFailure;

	partIn        += context->blockLen;
	partInLen     -= context->blockLen;
	(*partOutLen) += localPartOutLen;
	partOut       += localPartOutLen;
	maxPartOutLen -= localPartOutLen;
    }
  
    /* Copy remaining input bytes to the context's input buffer. */
    PORT_Memcpy((unsigned char *)context->input, partIn,
		context->inputLen = partInLen);
    return SECSuccess;
}

/* Finalize the RSA process, check to make sure there is no more data
 * waiting in the input buffer.
 */
SECStatus
RSA_PublicEnd (RSAPublicContext *context)
{
    if (context->inputLen != 0) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }

    /* Restart context to accumulate a new block. */
    context->inputLen = 0;
    return SECSuccess;
}

/* There is enough data in the input data block, perform the ModExp. Input
 * length is context->blockLen.
 */
static SECStatus
RSA_Operation(RSAPublicContext *context, unsigned char *output,
	      unsigned int *outputLen, unsigned int maxOutputLen,
	      unsigned char *input)
{
    SECStatus status;
    unsigned int modulusLen;
    CMPInt cmpInputBuffer;
    CMPInt cmpOutputBuffer;

    CMP_Constructor (&cmpOutputBuffer);
    CMP_Constructor (&cmpInputBuffer);

    if ((*outputLen = context->blockLen) > maxOutputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	status = SECFailure;
	goto loser;
    }

    /* Convert input to CMPInt representation. */
    status = CMP_OctetStringToCMPInt(input, context->blockLen, &cmpInputBuffer);
    if (status != SECSuccess) goto loser;

    /* Exponentiate. */
    status = CMP_ModExp(&cmpInputBuffer, &context->publicExponent,
			&context->modulus, &cmpOutputBuffer);
    if (status != SECSuccess) goto loser;

    /* Convert back to an octet string. At this point, the result is a
     * cryptographic data unit, not a math unit. The buffer to return
     * must be the same size as the modulus, so if the actual value
     * of the cmpOutputBuffer is smaller, pad with leading zeros.
     */
    modulusLen = BITS_TO_LEN (CMP_BitLengthOfCMPInt (&context->modulus));
    status = CMP_CMPIntToFixedLenOctetStr(&cmpOutputBuffer, modulusLen,
					  maxOutputLen, outputLen,
					  output);
    if (status != SECSuccess) goto loser;

loser:
    CMP_Destructor (&cmpInputBuffer);
    CMP_Destructor (&cmpOutputBuffer);
    return status;
}

/* Destroy the CMPInt's and free up the memory of context.
 */
void
RSA_DestroyPublicContext (RSAPublicContext *context)
{
    CMP_Destructor (&context->modulus);
    CMP_Destructor (&context->publicExponent);

    if (context->input != NULL) {
	PORT_Memset((unsigned char *)(context->input), 0, context->blockLen);
	PORT_Free(context->input);
    }
}
