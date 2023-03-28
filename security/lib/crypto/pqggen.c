/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
#include "crypto.h"
#include "cmp.h"
#include "dsa.h"
#include "pqggen.h"
#include "prime.h"
#include "secitem.h"

#define ROSTER_SIZE 500

extern int SEC_ERROR_NEED_RANDOM;

/* Initialize the process, allocating space for elements of the context.
 */
PQGParamGenContext *
PQG_CreateParamGenContext(unsigned int primeBits, unsigned int subPrimeBits)
{
    PQGParamGenContext *context;

    context = (PQGParamGenContext *) PORT_ZAlloc(sizeof(PQGParamGenContext));
    if (context == NULL)
	goto loser;

    /* Check lengths. */
    if (primeBits > MAX_PQG_PRIME_BITS || primeBits < MIN_PQG_PRIME_BITS)
	goto loser;

    if (subPrimeBits > MAX_PQG_SUBPRIME_BITS ||
	subPrimeBits < MIN_PQG_SUBPRIME_BITS)
	goto loser;

    if (subPrimeBits >= primeBits)
	goto loser;

    /* Copy generation parameters to context. */
    context->primeBits = primeBits;
    context->subPrimeBits = subPrimeBits;

    /* Mark as initialized. */
    context->initialized = 1;

    return context;
loser:
    if(context != NULL) PQG_DestroyParamGenContext(context);
    return NULL;
}

/* Generate p, q and g. randomBlock is a buffer containing
 *   pLen + qLen + (pLen - qLen) bytes.
 */
SECStatus
PQG_ParamGen(PQGParamGenContext *context, PQGParams **params,
	     unsigned char *randomBlock)
{
    CMPStatus status;
    int bufferSize;
    unsigned int pLen, qLen, lengthInBits, rosterSize, i;
    CMPInt temp, primeP, primeQ, baseG, baseH;
    unsigned char primeRoster[ROSTER_SIZE];
    SECStatus passed;
    PQGParams *pqg;
    PRArenaPool *arena = NULL;

    /* Check if initialized. */
    if (context->initialized != 1)
	goto loser;

    CMP_Constructor (&temp);
    CMP_Constructor (&primeP);
    CMP_Constructor (&primeQ);
    CMP_Constructor (&baseG);
    CMP_Constructor (&baseH);

    /* Compute lengths. */
    pLen = CMP_BITS_TO_LEN(context->primeBits);
    qLen = CMP_BITS_TO_LEN(context->subPrimeBits);

    /* Find subprime primeQ, using RSA prime-finding routine checking
     * that gcd (primeQ, 1) = 1. Use baseG to hold the 1. This uses
     * the first qLen bytes of randomBlock.
     */
    status = CMP_CMPWordToCMPInt((CMPWord)1, &baseG);
    if (status != CMP_SUCCESS)
	goto loser;

    status = prm_PrimeFind(randomBlock, context->subPrimeBits, &baseG, &primeQ);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Convert next part of random block to CMPInt, use baseG */
    status = CMP_OctetStringToCMPInt(randomBlock + qLen, pLen - qLen, &baseG);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Create a CMPInt = to 2 ^ subPrimeBits, use baseH */
    status = CMP_PowerOfTwo(context->subPrimeBits, &baseH);
    if (status != CMP_SUCCESS)
	goto loser;

    /* p1 = random * 2 ^ subPrimeBits */
    status = CMP_Multiply(&baseG, &baseH, &primeP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Create a CMPInt = to 2 ^ primeBits, use baseG */
    status = CMP_PowerOfTwo(context->primeBits, &baseG);
    if (status != CMP_SUCCESS)
	goto loser;

    /* p2 = p1 mod 2 ^ primeBits. */
    status = CMP_ModularReduce(&primeP, &baseG, &baseH);
    if (status != CMP_SUCCESS)
	goto loser;

    /* p3 = p2 with a special bit set */
    status = CMP_SetBit(context->primeBits - 1, &baseH);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find 2q = 2 * primeQ */
    status = CMP_Add(&primeQ, &primeQ, &temp);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find p4 = p3 mod 2q */
    status = CMP_ModularReduce(&baseH, &temp, &primeP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find p5 = p3 - p4 */
    status = CMP_Subtract(&baseH, &primeP, &baseG);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find p6 = p5 + 2q */
    status = CMP_Add(&baseG, &temp, &primeP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find p7 = p6 + 1 */
    status = CMP_AddCMPWord((CMPWord)1, &primeP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find primeP, in steps of 2q. "Need random" error if not found. */
    rosterSize = ROSTER_SIZE;
    while (1) {
	passed = SECFailure;

	/* Using sieve method, generate list of potential primes.
	 * primeRoster[i] == 0 iff primeP + 2i * q is relatively prime
	 * to all primes less than 65,000. */
	status =  prm_GeneratePrimeRoster(&primeP, &temp, rosterSize,
					  primeRoster);
	if (status != CMP_SUCCESS)
	    goto loser;

	/* Iterate over the primeRoster array, testing only those
	 * candidates which remain.
	 */
	for (i = 0; i < rosterSize; ++i) {
	    lengthInBits = CMP_BitLengthOfCMPInt(&primeP);
	    if (lengthInBits > context->primeBits) {
		PORT_SetError(SEC_ERROR_NEED_RANDOM);
		goto loser;
	    }

	    if (primeRoster[i] == 0) {      
		status = prm_PseudoPrime(&primeP, &passed);
		if (status != CMP_SUCCESS) 
		    goto loser;
	    }
	    if (passed == SECSuccess)
		break;

	    /* Add 2q to current primeP */
	    status = CMP_Add(&temp, &primeP, &baseH);
	    if (status != CMP_SUCCESS)
		goto loser;
	    status = CMP_Move(&baseH, &primeP);
	    if (status != CMP_SUCCESS)
		goto loser;
	} /* End iteration over primeRoster candidates */
	if (passed == SECSuccess)
	    break;
    }

    /* Convert last part of random block to CMPInt */
    status = CMP_OctetStringToCMPInt(randomBlock + pLen, pLen, &temp);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Reduce mod primeP to get "pre-base" h. */
    status = CMP_ModularReduce(&temp, &primeP, &baseH);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Compute baseG, according to the equation
     * g = h^((p-1)/q) mod p.
     * "Need random" error if g = 0 or 1.
     */
    status = CMP_Divide(&primeP, &primeQ, &temp, &baseG);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_ModExp(&baseH, &temp, &primeP, &baseG);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Check for baseG = to 0 or 1 */
    if (CMP_BitLengthOfCMPInt (&baseG) <= 1) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	goto loser;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	goto loser;

    pqg = PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (pqg == NULL)
	goto loser;

    /* Convert results to canonical format. */
    bufferSize = CMP_OctetLengthOfCMPInt(&primeP);
    pqg->prime.data = PORT_ArenaAlloc(arena, bufferSize);
    if (pqg->prime.data == NULL)
	goto loser;
    status = CMP_CMPIntToOctetString(&primeP, bufferSize,
				     &pqg->prime.len,
				     pqg->prime.data);
    if (status != CMP_SUCCESS)
	goto loser;

    bufferSize = CMP_OctetLengthOfCMPInt(&primeQ);
    pqg->subPrime.data = PORT_ArenaAlloc(arena, bufferSize);
    if (pqg->subPrime.data == NULL)
	goto loser;
    status = CMP_CMPIntToOctetString(&primeQ, bufferSize,
				     &pqg->subPrime.len,
				     pqg->subPrime.data);
    if (status != CMP_SUCCESS)
	goto loser;

    bufferSize = CMP_OctetLengthOfCMPInt(&baseG);
    pqg->base.data = PORT_ArenaAlloc(arena, bufferSize);
    status = CMP_CMPIntToOctetString(&baseG, bufferSize,
				     &pqg->base.len,
				     pqg->base.data);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Copy parameters. */
    *params = pqg;

    CMP_Destructor (&temp);
    CMP_Destructor (&primeP);
    CMP_Destructor (&primeQ);
    CMP_Destructor (&baseG);
    CMP_Destructor (&baseH);
    return SECSuccess;

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);

    CMP_Destructor (&temp);
    CMP_Destructor (&primeP);
    CMP_Destructor (&primeQ);
    CMP_Destructor (&baseG);
    CMP_Destructor (&baseH);
    return SECFailure;
}

/* Free up memory allocated. */
void
PQG_DestroyParamGenContext(PQGParamGenContext *context)
{
    PORT_Assert(context != NULL);

    PORT_Memset(context, 0, sizeof(PQGParamGenContext));
    PORT_Free(context);
    return;
}

SECStatus
PQG_VerifyParams(PQGParams *params, SECStatus *result)
{
    unsigned int pLen, qLen;
    CMPStatus status;
    SECStatus secstatus;
    CMPInt one, n, prime, subPrime, base;

    CMP_Constructor(&one);
    CMP_Constructor(&n);
    CMP_Constructor(&prime);
    CMP_Constructor(&subPrime);
    CMP_Constructor(&base);

    status = CMP_OctetStringToCMPInt(params->prime.data,
				     params->prime.len, &prime);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_OctetStringToCMPInt(params->subPrime.data,
				     params->subPrime.len, &subPrime);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_OctetStringToCMPInt(params->base.data,
				     params->base.len, &base);
    if (status != CMP_SUCCESS)
	goto loser;

    pLen = CMP_BitLengthOfCMPInt(&prime);
    qLen = CMP_BitLengthOfCMPInt(&subPrime);

    if (qLen != DSA_SUBPRIME_BITS) {
	*result = SECFailure;
	goto done;
    }

    if ((pLen < 512) || (pLen > 1024) || ((pLen % 64) != 0)) {
	*result = SECFailure;
	goto done;
    }

    if (CMP_Compare(&base, &prime) != -1) {
	*result = SECFailure;
	goto done;
    }

    status = CMP_CMPWordToCMPInt((CMPWord)1, &one);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_ModularReduce(&prime, &subPrime, &n);
    if (status != CMP_SUCCESS)
	goto loser;

    if (CMP_Compare(&n, &one) != 0) {
	*result = SECFailure;
	goto done;
    }

    secstatus = prm_RabinTest(&subPrime, result);
    if (secstatus != SECSuccess)
	goto loser;
    if (*result != SECSuccess)
	goto done;

    secstatus = prm_RabinTest(&prime, result);
    if (secstatus != SECSuccess)
	goto loser;
    if (*result != SECSuccess)
	goto done;

    *result = SECSuccess;
done:
    CMP_Destructor(&one);
    CMP_Destructor(&n);
    CMP_Destructor(&prime);
    CMP_Destructor(&subPrime);
    CMP_Destructor(&base);
    return SECSuccess;

loser:
    *result = SECFailure;
    CMP_Destructor(&one);
    CMP_Destructor(&n);
    CMP_Destructor(&prime);
    CMP_Destructor(&subPrime);
    CMP_Destructor(&base);
    return SECFailure;
}

void
PQG_DestroyParams(PQGParams *params)
{
    if ((params != NULL) && (params->arena != NULL))
	PORT_FreeArena(params->arena, PR_FALSE);
}

PQGParams *
PQG_DupParams(PQGParams *src)
{
    PRArenaPool *arena;
    PQGParams *dest;
    SECStatus status;

    if (src == NULL)
	return NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	goto loser;

    dest = PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (dest == NULL)
	goto loser;

    status = SECITEM_CopyItem(arena, &dest->prime, &src->prime);
    if (status != SECSuccess)
	goto loser;

    status = SECITEM_CopyItem(arena, &dest->subPrime, &src->subPrime);
    if (status != SECSuccess)
	goto loser;

    status = SECITEM_CopyItem(arena, &dest->base, &src->base);
    if (status != SECSuccess)
	goto loser;

    return dest;

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}
