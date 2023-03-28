/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
#include "secrng.h"
#include "crypto.h"
#include "cmp.h"
#include "cmpspprt.h"
#include "cmppriv.h"
#include "rsa.h"
#include "prime.h"

extern int SEC_ERROR_NEED_RANDOM;

#define SIZE_OF_PRIME_ROSTER     500
#define SIZE_OF_LIST_OF_PRIMES   53
#define SIEVE_INCREMENTS         512    
/* #define SIEVE_STEPS              ((65000 / SIEVE_INCREMENTS)) */
#define SIEVE_STEPS              126

static unsigned char listOfPrimes[SIZE_OF_LIST_OF_PRIMES] = {
      3,   5,   7,  11,  13,  17,  19,  23,  29,  31,
     37,  41,  43,  47,  53,  59,  61,  67,  71,  73,
     79,  83,  89,  97, 101, 103, 107, 109, 113, 127,
    131, 137, 139, 149, 151, 157, 163, 167, 173, 179,
    181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
    239, 241, 251
};

static SECStatus ByteVectorToCMPInt(unsigned char *, unsigned int, CMPInt *);
static SECStatus GeneratePrimeArray(int searchDepth,
				    unsigned int lowerLim,
				    unsigned int upperLim,
				    unsigned char *remainders,
				    unsigned char *sieve);
static SECStatus FilterCandidateRoster(CMPInt *start, CMPInt *step,
				       unsigned int test0,
				       unsigned int sieveSize,
				       unsigned char *sieveRoster,
				       unsigned int rosterSize,
				       unsigned char *roster);
static SECStatus ComputeInverse(long int r, long int prime, long int *rInv);

/* Prime finding routine. Find a prime number based on the randomBlock.
 */
SECStatus
prm_PrimeFind(unsigned char *randomBlock, unsigned int primeSizeBits,
	      CMPInt *expo, CMPInt *primeCMP)
{
    CMPStatus status;
    SECStatus dsrv;
    SECStatus testResult;
    unsigned char primeRoster[SIZE_OF_PRIME_ROSTER], sieve[SIEVE_INCREMENTS];
    unsigned int i, r, r1, s, offset, j, k, lowerLim, upperLim;
    int numPrimes = SIZE_OF_LIST_OF_PRIMES;
    unsigned char offsets[SIZE_OF_LIST_OF_PRIMES];
    CMPWord addendum;
    CMPWord remainder;
    CMPInt  gcd_p_1_ee, p_1, unit;

    CMP_Constructor (&p_1);
    CMP_Constructor (&gcd_p_1_ee);
    CMP_Constructor (&unit);

    /* unit = 1 */
    status = CMP_CMPWordToCMPInt ((CMPWord) 1, &unit);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Create a starting point for the prime from the random block.
     *   ByteVectorToCMPInt will produce a starting point exactly
     *   the same as BigNum based BSAFE. Otherwise, simply use
     *   CMP_OctetStringToCMPInt.
     */
    status = ByteVectorToCMPInt(randomBlock, primeSizeBits, primeCMP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Set two most significant bits */
    status = CMP_SetBit(primeSizeBits - 2, primeCMP);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_SetBit (primeSizeBits - 1, primeCMP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Clear bits beyond primeSizeBits until end of MSW. If you are using
     * ByteVectorToCMPInt, the algorithm is loading up 16 bit words
     * into the CMPInt, clear the last bits in the MSByte + 8 bits.
     */
    r = 8;
    r += BITS_TO_LEN (primeSizeBits) * BITS_PER_BYTE;
    r = (r <= (r1 = CMP_BitLengthOfCMPInt (primeCMP))) ? r : r1;
    for (i = primeSizeBits; i < r; ++i) {
	status = CMP_ClearBit (i, primeCMP);
	if (status != CMP_SUCCESS)
	    goto loser;
    }

    /* force p to be odd */
    status = CMP_SetBit (0, primeCMP);
    if (status != CMP_SUCCESS)
	goto loser;

    /* clear primeRoster to initialize all positions as possible primes. */
    PORT_Memset (primeRoster, 0, SIZE_OF_PRIME_ROSTER);

    /* Sieve is used to mark the position of plausible primes relative to
     * initial candidate computed from random block, ie, primeRoster[i] == 1
     * indicates that primeCMP + 2 * i is a composite number.
     */
    lowerLim = 3;
    upperLim = 3 + SIEVE_INCREMENTS;
    for (i = 0; i < SIEVE_STEPS; ++i) {
	/* Create sieve for purposes of eliminating composite numbers */
	dsrv = GeneratePrimeArray(numPrimes, lowerLim, upperLim, offsets,
				  sieve);
	if (dsrv != SECSuccess)
	    goto loser;

	for (k = 0; k < SIEVE_INCREMENTS; k += 2) {
	    if (sieve[k])
		continue;
	    /* sieve based on s */
	    s = k + lowerLim;

	    status = CMP_CMPWordModularReduce(primeCMP, (CMPWord) s,
					      &remainder);
	    if (status != CMP_SUCCESS)
		goto loser;

	    r = (unsigned int) remainder;
	    if ((r & 1) == 1) {
		offset = (s - r) / 2;
	    } else {
		if (r > 0)
		    offset = s - r / 2;
		else
		    offset = 0;
	    }

	    /* j = prime + n * s - r is divisible by s,
	     * so set PrimeRoster[j] = 1 */
	    for (j = offset; j < SIZE_OF_PRIME_ROSTER; j += s)
		primeRoster[j] = 1;
	}
	lowerLim = upperLim;
	upperLim += SIEVE_INCREMENTS;
    }

    /* check for primality of j = prime + 2 * i, where primeRoster[i] = 0 */
    testResult = SECFailure;
    for (i = 0; i < SIZE_OF_PRIME_ROSTER; i++) {
	/* accumulate addendum over consecutive 1's in PrimeRoster array */
	if (primeRoster[i] > 0) {
	    addendum = 0;
	    while (primeRoster[i]) { 
		addendum += 2;
		i++;
		if (i == SIZE_OF_PRIME_ROSTER) {
		    PORT_SetError(SEC_ERROR_NEED_RANDOM);
		    goto loser;
		}
	    }
	    status = CMP_AddCMPWord(addendum, primeCMP);
	    if (status != CMP_SUCCESS)
		goto loser;
	}    

	/* set p_1 = p - 1; Can subtract fearlessly since p > 0 */
	status = CMP_Move(primeCMP, &p_1);
	if (status != CMP_SUCCESS)
	    goto loser;
	
	status = CMP_SubtractCMPWord((CMPWord) 1, &p_1);
	if (status != CMP_SUCCESS)
	    goto loser;
	status = CMP_ComputeGCD(expo, &p_1, &gcd_p_1_ee);
	if (status != CMP_SUCCESS)
	    goto loser;

	/* Enforce constraint that p - 1 and encryption exponent have
	 * no common factors, ie,  gcd (p - 1, ee) == 1 . */
	if (CMP_Compare(&unit, &gcd_p_1_ee) != 0)
	    goto skip;

	/* check for pseudo primality */
	dsrv = prm_PseudoPrime(primeCMP, &testResult);
	if (dsrv != SECSuccess)
	    goto loser;
	if (testResult != SECSuccess)
	    goto skip;

	dsrv = prm_RabinTest(primeCMP, &testResult);
	if (dsrv != SECSuccess)
	    goto loser;
	if (testResult == SECSuccess)
	    goto found_prime;

    skip:
	status = CMP_AddCMPWord((CMPWord) 2, primeCMP);
	if (status != SECSuccess) 
	    goto loser;
    }

    /* Couldn't find a prime with the supplied random block, so ask
     * caller to generate another random block and try again. */
    PORT_SetError(SEC_ERROR_NEED_RANDOM);

loser:
    CMP_Destructor (&p_1);
    CMP_Destructor (&gcd_p_1_ee);
    CMP_Destructor (&unit);
    return SECFailure;

found_prime:
    CMP_Destructor (&p_1);
    CMP_Destructor (&gcd_p_1_ee);
    CMP_Destructor (&unit);
    return SECSuccess;
}

/* Pseudo-primality test.
 *   If pseudo prime, *testResult = SECSuccess, else *testResult = SECFailure.
 */
SECStatus
prm_PseudoPrime(CMPInt *proposedPrime, SECStatus *testResult)
{
    CMPStatus status;
    unsigned int i;
    CMPInt smallPrime, fermatValue;

    CMP_Constructor (&smallPrime);
    CMP_Constructor (&fermatValue);
  
    /* Prepare for setting base vector to the small prime.
     */
    for (i = 0; i < 4; ++i) {
	/* Fermat test. Compute
	 * remainder = base ^ proposedPrime mod proposedPrime
	 * and compare the base to the remainder.
	 */
	status = CMP_CMPWordToCMPInt((CMPWord) listOfPrimes[i], &smallPrime);
	if (status != CMP_SUCCESS)
	    goto loser;

	status = CMP_ModExp(&smallPrime, proposedPrime, proposedPrime,
			    &fermatValue);
	if (status != CMP_SUCCESS)
	    goto loser;

	if (CMP_Compare (&fermatValue, &smallPrime) != 0) {
	    *testResult = SECFailure;
	    return SECSuccess;
	}
    }

    /* Passed this test, set to succeed. */
    *testResult = SECSuccess;

    CMP_Destructor (&smallPrime);
    CMP_Destructor (&fermatValue);
    return SECSuccess;

loser:
    CMP_Destructor (&smallPrime);
    CMP_Destructor (&fermatValue);
    return SECFailure;
}

/* Sets sieve so that sieve[i] == 0 iff i + lowerLim is prime;
 *   Implements Eratosthenes sieve using listOfPrimes.
 *   Assumptions:
 *    First call must be made with lowerLim = 3 and upperLim >= biggest prime
 *     in listOfPrimes;
 *    lowerLim and UpperLim are odd numbers;
 *    remainders[i] = lowerLim % listOfPrimes;
 *    upperLim < min (2 ^ 32, p ^ 2) where p = the SIZE_OF_LIST_OF_PRIMES + 1
 *     prime.
 */   
static SECStatus
GeneratePrimeArray(int searchDepth, unsigned int lowerLim,
		   unsigned int upperLim, unsigned char *remainders,
		   unsigned char *sieve)
{
    int s, k, range;
    int prime, offset, remainder;

    /* Initialize sieve so that positions representing odd numbers are enabled
     * and corresponding even positions are disabled. 
     */
    range = upperLim - lowerLim;
    for (s = 0; s < range; s = s + 2) {
	sieve[s] = 0;
	sieve[s + 1] = 1;    
    }

    /* if this is first call to routine set remainders array appropriately */
    if (lowerLim == 3) {
	remainders[0] = 0;
	for (s = 1; s < searchDepth; ++s)
	    remainders[s] = 3;
    }

    /* Walk through sieve array progressively setting positions that represent
     * numbers divisible by the s-th odd prime
     */
    for (s = 0; s < searchDepth; ++s) {
	prime     = listOfPrimes[s];
	remainder = remainders[s];
	offset    = (remainder != 0) ? prime - remainder : 0;
	for (k = offset; k < range; k += prime)
	    sieve[k] = 1;
	remainders[s] = (unsigned char ) ((range == k) ? 0 : range + prime - k);
    }

    /* If initial call ensure that all prime positions indicate primeness */
    if (lowerLim == 3) {
	for (s = 0; s < searchDepth; ++s)
	    sieve[listOfPrimes[s] - 3] = 0;
    }
    return SECSuccess;
}

/* Uses a two-stage sieving process to produce a list of plausible primes.
 * One sieving process is used to generate consecutive primes that in
 * turn are used to test the candidate roster.
 */
SECStatus
prm_GeneratePrimeRoster(CMPInt *initPos, CMPInt *step, unsigned int rosterSize,
			unsigned char *roster)
{
    unsigned char offsets[SIZE_OF_LIST_OF_PRIMES];
    unsigned char sieve[SIEVE_INCREMENTS];
    int numPrimes = SIZE_OF_LIST_OF_PRIMES;
    unsigned int i, lowerLim, upperLim, sieveSize;    	
    SECStatus status;

    /* clear primeRoster to initialize all positions as possible primes. */
    PORT_Memset(roster, 0, rosterSize);

    /* Sieve is used to mark the position of plausible primes relative to
     * initial candidate computed from random block, ie, primeRoster[i] == 1
     * indicates that primeCMP + i * step is a composite number.
     */
    lowerLim  = 3;
    upperLim  = 3 + SIEVE_INCREMENTS;
    sieveSize = SIEVE_INCREMENTS;
    for (i = 0; i < SIEVE_STEPS; ++i) {
	/* Create sieve for purposes of eliminating composite candidate
	 * numbers */
	status = GeneratePrimeArray(numPrimes, lowerLim, upperLim, offsets,
				    sieve);
	if (status != SECSuccess)
	    goto loser;

	status = FilterCandidateRoster(initPos, step, lowerLim, sieveSize,
				       sieve, rosterSize, roster);
	if (status != SECSuccess)
	    goto loser;
	lowerLim = upperLim;
	upperLim += SIEVE_INCREMENTS;
    }

    return SECSuccess;
loser:
    return SECFailure;
}

/* FilterCandidateRoster is used as a means of progressively
 *   sieving through the list of numbers
 *   start, start + step, start + 2 * step, ..., start + k * step, ...,
 *   start + step * (rosterSize - 1).
 *   The sieving set has the representation {test0 + i[0], .., test0 + i[t]}
 *   where sieveRoster[i[j]] = 0.
 *   On return, roster[k] has been set to 1 if for some j, test0 + i[j]
 *   divides start + k * step.
 *
 *   Assumptions
 *    1) step is twice some prime number and so is prime
 *       to all other odd primes.
 *    2) sieveRoster[k] == 0 iff test0 + k is prime.
 *
 *   Suppose we wish to eliminate all the numbers in our candidate
 *   roster that are not relatively prime to a given prime number s.
 *   Let k[s] be the smallest index for which start + k[s] * step is
 *   divisible by s. If j > k[s] and start + j * step is divisible
 *   by s then it follows by subtraction that (j - k[s]) * step is divisible
 *   by s. Assumption (1) implies that s divides j - k[s].
 *
 *   If s divides start then k[s] = 0. Otherwise let r = start % s and
 *   rho = step % s > 0. Observe start + k[s] * step = 0 mod s, implying
 *   that k[s] * rho = s - r mod s . Hence k[s] = (s - r) * rho ^ -1 mod s.
 *   So k[s] is the smallest positive number which satisfies the previous
 *   equation.
 */
static SECStatus
FilterCandidateRoster(CMPInt *start, CMPInt *step, unsigned int test0,
		      unsigned int sieveSize, unsigned char *sieveRoster,
		      unsigned int rosterSize, unsigned char *roster)
{
    unsigned int j, k;
    CMPStatus status;
    SECStatus dsrv;
    CMPWord s, remainder, stepRemainder;
    long int inverse;

    for (j = 0; j < sieveSize; j += 2) {
	if (sieveRoster[j])
	    continue;

	s = (CMPWord) test0 + (CMPWord) j;
	status = CMP_CMPWordModularReduce(start, s, &remainder);
	if (status != CMP_SUCCESS)
	    goto loser;

	if (remainder == 0) {
	    k = 0;
	} else {
	    status = CMP_CMPWordModularReduce(step, s, &stepRemainder);
	    if (status != CMP_SUCCESS)
		goto loser;

	    dsrv = ComputeInverse((long int) stepRemainder, (long int) s,
				  &inverse);
	    if (dsrv != SECSuccess)
		goto loser;
	    k = (unsigned int) ((inverse * (s - remainder)) % s);
	}
	for (; k < rosterSize; k += (unsigned int) s)
	    roster[k] = 1;
    }
    return SECSuccess;
loser:
    return SECFailure;
}

/* ComputeInverse computes rInv, the positive multiplicative inverse
 *   of r mod prime, ie, rInv * r = 1 mod P and 0 < rInv < prime.
 *   Assume that 0 < r < prime < sqrt (MAX_CMPWORD) .
 *   Computes inverse of r by means of extended Euclidean algorithm.
 *   See Knuth Volume2 for details.
 */
static SECStatus
ComputeInverse(long int r, long int prime, long int *rInv)
{
    long int r0, r1, r2, u0, u1, u2, quotient;

    u0 = 0;
    u1 = 1;
    r0 = prime;
    r1 = r;
    quotient  = r0 / r1;
    r2 = r0 % r1;
    u2 = -quotient;
    while (r2 > 0) {
	r0 = r1;
	r1 = r2;
	u0 = u1;
	u1 = u2;
	quotient  = r0 / r1;
	r2 = r0 % r1;
	u2 = u0 - quotient * u1;
    }
    *rInv = (u1 > 0)? u1 : prime + u1;
    return SECSuccess;
}

/* Maps byteVector to destInt; Differs from octet to CMPInt routine in
 *   the way it treats the arithmetic valuation of the byte stream
 *   Here only to produce the exact same results as BigNum Algae.
 */
static SECStatus
ByteVectorToCMPInt(unsigned char *byteVector, unsigned int primeSizeBits,
		   CMPInt *destInt)
{
    CMPStatus status;
    int i, halfWords;
    unsigned char *octetString = NULL;

    halfWords = (int)((primeSizeBits + 15) / 16);

    octetString = PORT_Alloc ((halfWords * 2) + 1);
    if (octetString == NULL)
	goto loser;

    for (i = 0; i < halfWords * 2; i = i + 2) {
	octetString[(halfWords * 2) - 2 - i] = byteVector[i];
	octetString[(halfWords * 2) - 1 - i] = byteVector[i + 1];
    }

    status = CMP_OctetStringToCMPInt(octetString, (halfWords * 2), destInt);
    if (status != CMP_SUCCESS)
	goto loser;

    if (octetString != NULL) {
	PORT_Memset(octetString, 0, (halfWords * 2) + 1);
	PORT_Free(octetString);
    }
    return SECSuccess;

loser:
    if (octetString != NULL) {
	PORT_Memset(octetString, 0, (halfWords * 2) + 1);
	PORT_Free(octetString);
    }
    return SECFailure;
}

/* Implements the Rabin-Miller algorithm for probalistic primality detection.
 * The probability of a non-prime number passing this test is no greater
 * than (1/4)^iter.
 */
SECStatus
prm_RabinTest(CMPInt *proposedPrime, SECStatus *testResult)
{
    CMPInt p1, z, m, a, tmp, one;
    int j, b, bit;
    CMPStatus status;
    unsigned char *randomBlock;
    int primeBits, primeLen;
    char mask;
    int iter = 50;

    primeBits = CMP_BitLengthOfCMPInt(proposedPrime);
    primeLen = CMP_BITS_TO_LEN(primeBits);
    randomBlock = (unsigned char *)PORT_Alloc(primeLen);
    if (randomBlock == NULL)
	goto loser;
    mask = (0x100 >> ((primeLen * CMP_BITS_PER_BYTE) - primeBits)) - 1;

    if (primeBits >= 1024) {
	iter = 5;
    } else if (primeBits >= 512) {
	iter = 7;
    } else if (primeBits >= 384) {
	iter = 9;
    } else if (primeBits >= 256) {
	iter = 13;
    }

    CMP_Constructor(&p1);
    CMP_Constructor(&z);
    CMP_Constructor(&m);
    CMP_Constructor(&a);
    CMP_Constructor(&tmp);
    CMP_Constructor(&one);

    *testResult = SECSuccess;

    status = CMP_CMPWordToCMPInt((CMPWord)1, &one);
    if (status != CMP_SUCCESS)
	goto loser;

    /* p1 = proposedPrime - 1 */
    status = CMP_Move(proposedPrime, &p1);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_SubtractCMPWord((CMPWord)1, &p1);
    if (status != CMP_SUCCESS)
	goto loser;

    /* calculate b and m where p - 1 = 2^b * m.  assume p1 is even. */
    b = 0;
    do {
	b++;
	status = CMP_GetBit(b, &p1, &bit);
	if (status != CMP_SUCCESS)
	    goto loser;
    } while(bit == 0);
    
    status = CMP_Move(&p1, &m);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_ShiftRightByBits(b, &m);
    if (status != CMP_SUCCESS)
	goto loser;

    /* check for primality */
    while(iter--) {
	/* Generate a */
	do {
	    RNG_GenerateGlobalRandomBytes(randomBlock, primeLen);
	    randomBlock[0] &= mask;
	    CMP_OctetStringToCMPInt(randomBlock, primeLen, &a);
	} while((CMP_Compare(&a, proposedPrime) != -1) ||
		(CMP_Compare(&a, &one) != 1));
	
	status = CMP_ModExp(&a, &m, proposedPrime, &z);
	if (status != CMP_SUCCESS)
	    goto loser;

	/* if z == 1 succeed */
	if (CMP_Compare(&z, &one) == 0)
	    continue;

	/* if z == p - 1 succeed */
	if (CMP_Compare(&p1, &z) == 0)
	    continue;

	for (j = 1; j < b; j++) {
	    /* z = z^2 mod p */
	    status = CMP_ModMultiply(&z, &z, proposedPrime, &tmp);
	    if (status != CMP_SUCCESS)
		goto loser;

	    status = CMP_Move(&tmp, &z);
	    if (status != CMP_SUCCESS)
		goto loser;

	    /* if z == 1 fail */
	    if (CMP_Compare(&z, &one) == 0) {
		*testResult = SECFailure;
		goto done;
	    }

	    /* if z == p - 1 succeed */
	    if (CMP_Compare(&p1, &z) == 0)
		break;

	}
	/* if j == b and z != p - 1 fail */
	if (j == b && (CMP_Compare(&p1, &z) != 0)) {
	    *testResult = SECFailure;
	    goto done;
	}
    }

done:
    CMP_Destructor(&p1);
    CMP_Destructor(&z);
    CMP_Destructor(&m);
    CMP_Destructor(&a);
    CMP_Destructor(&tmp);
    CMP_Destructor(&one);
    return SECSuccess;

loser:
    CMP_Destructor(&p1);
    CMP_Destructor(&z);
    CMP_Destructor(&m);
    CMP_Destructor(&a);
    CMP_Destructor(&tmp);
    CMP_Destructor(&one);
    return SECFailure;
}
