/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains routines that perform modular exponentiation.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

/* Local support routines for ModExp.
 */
CMPWord CMP_GetBitsFromWord(CMPWord, int, int);
CMPWord CMP_GetBitsFromCMPInt(CMPInt *, int *, int);
CMPWord CMP_GetMSBitFromWord(CMPWord);
int CMP_ComputeLogOfTableSize(int);
int CMP_InitExponentTable(int, CMPInt *, CMPWord, CMPInt *, int **, CMPInt **);
void CMP_DestructExponentTable(int, int **, CMPInt **);
int CMP_GenerateNewExponent(CMPInt *, CMPWord, CMPWord, int *, CMPInt *);

/* Compute result <-- base ^ exponent mod modulus.
     Modular Exponentiation by using Montgomery product.
*/
CMPStatus
CMP_ModExp(CMPInt *base, CMPInt *exponent, CMPInt *modulus, CMPInt *result)
{
  CMPInt *expTab = (CMPInt *)NULL;
  CMPInt tmp;
  CMPWord n0Prime, power;
  register int i, unroll;
  int bitIndex;
  CMPStatus status;
  int *expExists = (int *)NULL;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (exponent) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (exponent) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (modulus) <= 0)
    return (CMP_LENGTH);
#endif

  CMP_Constructor (&tmp);

  do {
    /* If modulus is invalid, this subroutine will return the proper
         error code. */
    if ((status = CMP_ComputeMontCoeff (modulus, &n0Prime)) != 0)
      break;

    bitIndex = CMP_BitLengthOfCMPInt (exponent);
    unroll = CMP_ComputeLogOfTableSize (bitIndex);
    bitIndex--;

    /* If base >= modulus, this subroutine will return the proper
         error code. */
    if ((status = CMP_InitExponentTable
         (unroll, modulus, n0Prime, base, &expExists, &expTab)) != 0)
      break;

    power = CMP_GetBitsFromCMPInt (exponent, &bitIndex, unroll);
    if (expExists[(int)power] == 0) {
      if ((status = CMP_GenerateNewExponent
           (modulus, n0Prime, power, expExists, expTab)) != 0)
        break;
    }
    if ((status = CMP_Move
         (expTab + (int)power, result)) != 0)
      break;

    /* Compute exponent by successively squaring partial product "unroll"
         number of times and then by multiplying (base ^ power), where power
         is equal to the next segment of bits from the exponents. This loop
         unrolling saves on average
            exponentLengthInBits * (0.5 - 1 / unroll) - 2 ^ unroll
         MontProd operations. For example, if exponentLengthInBits = 512
         and unroll = 4, then average savings = 128 - 16 = 112.
      */
    while (bitIndex >= 0) {
      /* update result from p[k] to p[k] ^ (2 * unroll)
       */
      for (i = 1; i <= min (unroll, bitIndex + 1); ++i) {
        /* tmp = p[k] ^ (2i) */
        if ((status = CMP_MontSquare
             (result, modulus, n0Prime, &tmp)) != 0)
          break;

        /* result = p[k] ^ (2i) */
        if ((status = CMP_Move (&tmp, result)) != 0)
          break;
      }
      /* Did the above for loop finish because of an error? */
      if (status != 0)
        break;

      power = CMP_GetBitsFromCMPInt (exponent, &bitIndex, unroll);
      /* If there are no bits in this portion of the exponent, nothing to do */
      if (power == 0)
        continue;

      if (expExists[(int)power] == 0) {
        if ((status = CMP_GenerateNewExponent
             (modulus, n0Prime, power, expExists, expTab)) != 0)
          break;
      }

      /* tmp = p[k] ^ 2i * base ^ power = p[k + unroll] */
      if ((status = CMP_MontProduct
           (&expTab[(int)power], result, modulus, n0Prime, &tmp)) != 0)
        break;

      /* result = p[k + unroll] */
      if ((status = CMP_Move(&tmp, result)) != 0)
        break;      
    } /* end loop over bits of exponent, most significant bits first */
    /* Did the above while loop finish because of an error? */
    if (status != 0)
      break;

    /* Obtain natural representation of the Montgomery residue result.
     */
    if ((status = CMP_ConvertFromMont (result, modulus, n0Prime, &tmp)) != 0)
      break;
    if ((status = CMP_Move(&tmp, result)) != 0)
      break;
  } while (0);

  CMP_DestructExponentTable (unroll, &expExists, &expTab);
  CMP_Destructor (&tmp);
  return(status);
}

/* Returns a right adjusted bitfield of length n equal to bits pos, pos - 1,
     pos - 2, ..., pos - n + 1
 */
CMPWord
CMP_GetBitsFromWord(CMPWord srcWord, int pos, int n)
{
  return ((srcWord >> (pos + 1 - n)) & ~(CMP_WORD_MASK << n));
}

/* Returns a right adjusted bitfield of length = min (n, *pos + 1) equal
     to bits pos, pos - 1, ..., min(0, pos - n + 1) from CMPInt x. For
     example, if n = 4, x = 0xF33 and *pos = 6, then *pos <-- 2, and 6
     (ie, 0110 base 2) is returned. Upon calling, *pos is the position
     within the CMPInt's value, it may be a number greater than the size
     of a CMPWord.
 */
CMPWord
CMP_GetBitsFromCMPInt(CMPInt *x, int *pos, int length)
{
  int bitPtr, bitIndex, wordIndex;
  CMPWord field;

  bitPtr      = *pos;
  length      = min (length, bitPtr + 1);
  *pos       -= length;
  wordIndex   = bitPtr / CMP_WORD_SIZE;
  bitIndex    = bitPtr % CMP_WORD_SIZE;

  if (bitIndex >= length - 1) {
    /* Enough bits may be obtained from wordIndex word */
    return (CMP_GetBitsFromWord
      (CMP_PINT_WORD (x, wordIndex), bitIndex, length));
  }

  /* Desired bit field straddles
       CMP_PINT_WORD (x, wordIndex) and CMP_PINT_WORD (x, wordIndex - 1)
   */
  else {
    /* Get the bits from CMP_PINT_WORD (x, wordIndex) */
    field = CMP_GetBitsFromWord
      (CMP_PINT_WORD (x, wordIndex), bitIndex, bitIndex + 1);

    /* Make those bits left justified to the position length - 1 */
    field <<= (length - bitIndex - 1);

    /* Now or in the proper bits from CMP_PINT_WORD (x, wordIndex - 1) */
    field |= CMP_GetBitsFromWord
      (CMP_PINT_WORD (x, wordIndex - 1), CMP_WORD_SIZE - 1,
       length - bitIndex - 1);
    return (field);
  }
}

/* Returns numerical value of most significant bit of srcWord. For example,
     if srcWord = 0xA, then 8 is returned
 */
CMPWord
CMP_GetMSBitFromWord(CMPWord srcWord)
{
  CMPWord msb;

  if (srcWord == (CMPWord)0)
    return ((CMPWord)0);

  srcWord >>= 1;
  msb  = 1;
  while (srcWord > 0) {
    msb  <<= 1;
    srcWord >>= 1;
  }
  return (msb);
}

/* Determines the logarithm of the size of the exponent table.
 */
int
CMP_ComputeLogOfTableSize(int lengthInBits)
{
  if (lengthInBits >= 64)
    return (4);
  if (lengthInBits >= 16)
    return (3);
  if (lengthInBits >= 4)
    return (2);
  return (1);
}

/* Sets expTab[2 ^ i] <-- base ^ (2 ^ i) (mod) n, i = 0, ..., logSize - 1 .
 *     Also initializes expExists table
 *
 *     int logSize;           log of table size                                
 *     CMPInt *n;             modulus used for computing modular exponents     
 *     CMPWord n0Prime;       inverse factor used in Montgomery computation    
 *     CMPInt *base;          value to be raised to the exponent               
 *     int **expExists;       expExists[n] == 1, iff base ^ n has been computed
 *     CMPInt **expTab;       Array of CMPInts: baseExpTab[n].value = base ^ n 
 */
int
CMP_InitExponentTable(int logSize, CMPInt *n, CMPWord n0Prime, CMPInt *base,
		      int **expExists, CMPInt **expTab)
{
  int i, status;
  int tableSize;
  CMPInt unit;
  CMPWord expo;

  /* The incoming table addresses must be NULLs.
   */
  if ( (*expExists != (int *)NULL) || (*expTab != (CMPInt *)NULL) )
    return (CMP_INVALID_ARGUMENT);

  tableSize = 1 << logSize;
  CMP_Constructor (&unit);
  do {
    *expExists = (int *)PORT_Alloc(tableSize * sizeof (int));
    if (*expExists == (int *)NULL) {
      status = CMP_MEMORY;
      break;
    }

    /* Allocate expTab as an array of CMPInts.
     */
    *expTab = (CMPInt *) PORT_Alloc(tableSize * sizeof (CMPInt));
    if (*expTab == (CMPInt *) NULL) {
      status = CMP_MEMORY;
      break;
    }

    /* Set all existence flags to zero, and initialize expTab array.
     */
    for (i = 0; i < tableSize; ++i) {
      (*expExists)[i] = 0;
      CMP_Constructor (*expTab + i);
    }

    /* Initialize expTab[0] and expTab[1] to be the Montgomery versions
         of 1 and base.
     */
    if ((status = CMP_reallocNoCopy (1, &unit)) != 0)
      break;
    CMP_INT_LENGTH (unit) = 1;
    CMP_INT_WORD (unit, 0) = (CMPWord)1;
    if ((status = CMP_ConvertToMont (&unit, n, *expTab)) != 0)
      break;
    (*expExists)[0] = 1;

    if ((status = CMP_ConvertToMont (base, n, *expTab + 1)) != 0)
      break;
    (*expExists)[1] = 1;

    /* Set *expTab[2 ^ i] <---- expTab[1] ^ i
     */
    expo = (CMPWord)1;
    for (i = 1; i < logSize; ++i) {
      expo <<= 1;
      if ((status = CMP_MontSquare
           (*expTab + (int)(expo / 2), n, n0Prime, *expTab + (int)expo)) != 0)
        break;
      (*expExists)[(int)expo] = 1;
    }
  } while (0);

  CMP_Destructor (&unit);
  return (status);
}

/* Call CMP_Destructor for each of the CMPInt's in the exponent table.
 */
void
CMP_DestructExponentTable(int logSize, int **expExists, CMPInt **expTab)
{
  int i, tableSize;

  tableSize = 1 << logSize;

  /* If the incoming expExists table is NULL, do nothing, if not,
       zeroize the memory and free it up.
   */
  if (*expExists != (int *)NULL) {
    PORT_Memset((unsigned char *)*expExists, 0, tableSize * sizeof (int));
    PORT_Free((unsigned char *)(*expExists));
    *expExists = (int *)NULL;
  }

  /* If the incoming exponent table is NULL, do nothing, if not,
       destruct all the entries.
   */
  if (*expTab != (CMPInt *)NULL) {
    for (i = 0; i < tableSize; i++)
      CMP_Destructor (*expTab + i);
    PORT_Memset((unsigned char *)(*expTab), 0, tableSize * sizeof (CMPInt));
    PORT_Free((unsigned char *)(*expTab));
    *expTab = (CMPInt *)NULL;
  }
}

/* Sets expTab[expo] <-- (expTab[1] ^ expo) mod (n); expExists[expo] <-- 1.
 *     Explanation: if expo = e[n] *  2 ^ l + e[n-1] * 2 ^ l-1 .. + e[0], then
 *     the search procedes by successively computing (if necessary) the
 *     solution to  E[r] = e[n] *  2 ^ l + e[n-1] * 2 ^ l-1 .. + e[r] * 2 ^ r,
 *     and searching to see if the exponent D[r] = expo - E[r] can be computed
 *     by means of one multiplication of previously computed table entries
 *
 *     CMPInt *n;            modulus used for computing modular exponents
 *     CMPWord n0Prime;      inverse factor used in Montgomery computation
 *     CMPWord expo;         power which has yet to be generated
 *     int *expExists;       expExists[n] == 1, iff base ^ n has been computed
 *     CMPInt *expTab;       array of CMPInts: expTab[n].value = expTab[1] ^ n
 */
int
CMP_GenerateNewExponent(CMPInt *n, CMPWord n0Prime, CMPWord expo,
			int *expExists, CMPInt *expTab)
{
  CMPInt *resultP;
  CMPWord i, pow, msb, upperLim, lowerLim, upperExp;
  int status;

  expExists[(int)expo] = 1;
  resultP              = &(expTab[(int)expo]);
  pow                  = expo;
  upperExp             = 0;
  msb                  = CMP_GetMSBitFromWord (pow);
  lowerLim             = 1;

  while (pow > 0) {
    upperLim = pow >> 1;
    for (i = lowerLim; i <= upperLim; ++i) {
      if (expExists[(int)i] && expExists[(int)(pow - i)]) {
        expExists[(int)pow] = 1;
        if (pow == expo) {
          return (CMP_MontProduct
                   (&expTab[(int)i], &expTab[(int)(pow - i)], n,
                    n0Prime, resultP));
        }
        else {
          if ((status = CMP_MontProduct
               (&expTab[(int)i], &expTab[(int)(pow - i)], n, n0Prime,
                &expTab[(int)pow])) != 0)
            return (status);
          return (CMP_MontProduct
                  (&expTab[(int)pow], &expTab[(int)upperExp], n, n0Prime,
                   resultP));
        } 
      }
    }  /* End of search for previously computed co-factor (index i ) */

  /* Loop failed, so setup next iteration. Begin by checking to see if
       upper partial product has been precomputed, if not compute it and
       store it in appropriate cell of expTab.
   */
    if (pow != expo) {
      if (expExists[(int)(upperExp | msb)] == 0) {
        expExists[(int)(upperExp | msb)] = 1;
        if ((status = CMP_MontProduct
             (&expTab[(int)msb], &expTab[(int)upperExp], n, n0Prime,
              &expTab[(int)(upperExp | msb)])) != 0)
          return (status);
      }
    }

    /* Prepare for next iteration.
     */
    lowerLim  = 0;
    upperExp |= msb;      
    pow      &= ~msb;
    msb       = CMP_GetMSBitFromWord (pow);
  }  /* Loop over successive "suffixes of exp" */
  /* Is it possible to exit the loop? Or will it always execute
       one of the returns in the loop? */
  return (0);
}

/* CMP_ModExpCRT - decrypt ciphertext c into message m using Chinese
     Remainder Theorem.

   Discussion: We wish to compute m = c ^ d mod (pp * qq) where
     dp = d mod p-1 and dq = mod q-1 . We reduce the overall computation by
     approximately a factor of two by computing
       1) m mod p == a
       2) m mod q == b
       3) m = ((a - b mod p) mod p * cr) * q  + b

     The computation of a (b) can be further reduced by applying the primality
     of p and Fermat's theorem :
       4) a ^ f mod p = (a ^ (f mod p - 1)) mod p
     Equation 4) implies
       5) a == m mod p = c ^ dp mod p
       6) b == m mod q = c ^ dq mod q

     In computing 3) if a < b mod p, then it is necessary to compute the
     additive inverse modp of b mod p - a  = p - (b mod p - a)

     Derivation of Equation 3.
     Suppose we wish to compute x mod pq having previously computed
     a = x mod p and b = mod q. For purposes of derivation we assume
     that p < q. Let's try a linear type interpolation
     x = (a - b) * u + b. Observe that b = x mod q implies that
     (a - b) * u mod q = 0. Hence u = y * q. Hence we may re-express x as
     follows x = (a - b) * y * q + b. Now applying a = x mod p we see after
     simplification that  (a - b mod p) * q mod p * mod y  = a - b mod p.
     After dividing through mod p we see that y = q ^ -1  mod p. It is easy
     to deduce that 3) guarantees that 0 <= x <= p * q. 
 */

/* (base ^ exponent) mod modulus, where

     exponent = primeP * primeQ
     baseP = base mod primeP
     baseQ = base mod primeQ
     exponentP = exponent mod (primeP - 1)
     exponentQ = exponent mod (primeQ - 1)
     resultP = (baseP ^ exponentP) mod primeP
     resultQ = (baseQ ^ exponentQ) mod primeQ
     crtCoeff = primeQ ^ (-1) mod primeP  --  inverse of primeQ mod primeP

     answer =
       { [ (resultP - resultQ) mod primeP ] * (crtCoeff) } mod primeP
       * primeQ
       + resultQ
 */
CMPStatus
CMP_ModExpCRT(CMPInt *base, CMPInt *primeP, CMPInt *primeQ, CMPInt *exponentP,
	      CMPInt *exponentQ, CMPInt *crtCoeff, CMPInt *result)
{
  CMPStatus status;
  CMPInt temp, resultQ;

  CMP_Constructor (&temp);
  CMP_Constructor (&resultQ);

  do {
    /* resultP = [ (base mod primeP) ^ exponentP ] mod primeP
         place resultP into "temp"
         use the CMPInt "result" to hold intermediate results of operations
     */
    if ((status  = CMP_ModularReduce (base, primeP, result)) != 0)
      break;
    if ((status = CMP_ModExp
         (result, exponentP, primeP, &temp)) != 0)
      break;

    /* resultQ = [ (base mod primeQ) ^ exponentQ ] mod primeQ
         once again, use the CMPInt "result" to hold intermediate results
         of operations.
     */
    if ((status  = CMP_ModularReduce (base, primeQ, result)) != 0)
      break;
    if ((status = CMP_ModExp
         (result, exponentQ, primeQ, &resultQ)) != 0)
      break;

    /* Compute (resultP - resultQ) mod primeP
         remember, resultP is in "temp"
         still using the CMPInt "result" to hold intermediate results of
         operations. */
    if ((status = CMP_ModSubtract
         (&temp, &resultQ, primeP, result)) != 0)
      break;

    /* Multiply, mod primeP, by the crtCoeff -- primeQ ^ (-1) */
    if ((status = CMP_ModMultiply
         (result, crtCoeff, primeP, &temp)) != 0)
      break;

    /* Multiply by primeQ. */
    if ((status = CMP_Multiply (&temp, primeQ, result)) != 0)
      break;

    /* Add resultQ */
    status = CMP_AddInPlace (&resultQ, result);

  } while (0);

  CMP_Destructor (&temp);
  CMP_Destructor (&resultQ);

  return (status);
}
