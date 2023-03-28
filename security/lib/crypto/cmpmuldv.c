/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains routines that perform multiplication, division and
     related operations, multiply, divide, modular reduction, GCD and
     extended GCD.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

/* Local definitions and support routines for GCD and extended GCD.
 */
#define U_BIGGER_THAN_V   1
#define U_SMALLER_THAN_V  0
int CMP_AbsDifference(CMPInt *, CMPInt *, int *, CMPInt *);
int CMP_UpdateCoefficients(CMPInt *, CMPInt *, CMPInt *, int *, int *,
			   CMPInt *, CMPInt *);

/* Local support routines for division and modular reduction.
 */
int CMP_AppendWord(CMPWord, CMPInt *);
int CMP_EstimateMSWQuotient(CMPInt *, CMPInt *, CMPInt *, CMPInt *);
void CMP_DivideTwoWordsByWord(CMPWord, CMPWord, CMPWord, CMPWord*, CMPWord *);
void CMP_DivOneAndHalfWordsByWord(CMPWord, CMPWord, CMPWord, CMPWord *,
				  CMPWord *);
int CMP_FullCMPWordModReduce(CMPInt *, CMPWord, CMPWord *);

/* Multiplies two CMPInt's.
     product <--- multiplicand * multiplier
 */
CMPStatus
CMP_Multiply(CMPInt *multiplicand, CMPInt *multiplier, CMPInt *product)
{
  register CMPWord *pval;
  register int index, candLength, plierLength, prodLength;
  int status;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (multiplicand) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (multiplicand) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (multiplier) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (multiplier) <= 0)
    return (CMP_LENGTH);
#endif

  candLength = CMP_PINT_LENGTH (multiplicand);
  plierLength = CMP_PINT_LENGTH (multiplier);

  /* Compute the number of words in the result.
   */
  prodLength = candLength + plierLength;
  if (CMP_PINT_SPACE (product) < prodLength)
    if ((status = CMP_reallocNoCopy (prodLength, product)) != 0)
      return (status);

  /* Initialize result CMPInt to all zeros so we can add in the
       the product of the two operands.
   */
  pval = CMP_PINT_VALUE (product);
  PORT_Memset((unsigned char *)pval, 0, prodLength * CMP_BYTES_PER_WORD);

  /* Computation loop. Multiply each element in the multiplier by each
       element of the multiplicand, adding in the result to the product.
       We already have an optimized subroutine that does this,
       CMP_VectorMultiply.
   */
  for (index = 0; index < plierLength; ++index)
    CMP_VectorMultiply
         (CMP_PINT_WORD (multiplier, index), multiplicand, 0,
          candLength, product, index);

  /* Compute the final length of the product.
   */
  while ( (prodLength > 0) && (pval[--prodLength] == 0) );
  CMP_PINT_LENGTH (product) = ++prodLength;

  return (0);
}

/* dividend / divisor
     dividend divided by divisor
     dividend = divisor * quotient + remainder, where remainder < divisor
 */
CMPStatus
CMP_Divide(CMPInt *dividend, CMPInt *divisor, CMPInt *quotient,
	   CMPInt *remainder)
{
  register int aLength, bLength, j;
  int shift, status;
  CMPInt qj, product;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (dividend) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (dividend) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (divisor) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (divisor) <= 0)
    return (CMP_LENGTH);
#endif

  CMP_Constructor (&qj);
  CMP_Constructor (&product);

  do { 
    /* Initialize the quotient to be zero.
     */
    if (CMP_PINT_SPACE (quotient) < 1)
      if ((status = CMP_reallocNoCopy (2, quotient)) != 0)
        break;

    CMP_PINT_WORD (quotient, 0) = (CMPWord)0;
    CMP_PINT_LENGTH (quotient) = 1;

    /* if dividend < divisor, quotient = 0 and remainder = dividend.
     */
    if (CMP_Compare (dividend, divisor) < 0) {
      status = CMP_Move (dividend, remainder);
      break;
    }

    /* Compute left shift that will make the most significant bit of the
         most significant word of the divisor 1. Then shift divisor and
         dividend accordingly
     */
    if ((status = CMP_GetOffsetOfMSB (divisor, &shift)) != 0)
      break;

    /* If shift is CMP_WORD_SIZE, the most significant word of divisor is
         zero, meaning it is zero, an error.
     */
    if (shift == CMP_WORD_SIZE) {
      status = CMP_ZERO_DIVIDE;
      break;
    }

    if (shift > 0) {
      if ((status = CMP_ShiftLeftByBits (shift, divisor)) != 0)
        break;
      if ((status = CMP_ShiftLeftByBits (shift, dividend)) != 0)
        break;
    }

    /* Since dividend >= divisor, aLength >= bLength */
    aLength = CMP_PINT_LENGTH (dividend);
    bLength = CMP_PINT_LENGTH (divisor);

    /* Set up remainder so that it contains the first bLength + 1 most
         significant words of dividend.
     */
    if ((status = CMP_Move (dividend, remainder)) != 0)
      break;
    if (aLength > bLength) {
       if ((status = CMP_ShiftRightByCMPWords
            (aLength - bLength, remainder)) != 0)
         break;
    }
    /* Since dividend >= divisor, aLength >= bLength. To reach this else
         clause, aLength is not > bLength, so they are equal. In this case,
         quotient is 1 and remainder is dividend - divisor.
     */
    else {
      CMP_PINT_WORD (quotient, 0) = (CMPWord)1;
      CMP_PINT_LENGTH (quotient) = 1;
      if ((status = CMP_SubtractInPlace (divisor, remainder)) != 0)
        break;
    }

    if ((status = CMP_reallocNoCopy (aLength, &product)) != 0)
      break;

    if ((status = CMP_reallocNoCopy (2, &qj)) != 0)
      break;

    for (j = aLength - bLength - 1; j >= 0; --j) {
      /* If the current remainder is > divisor, add 1 to the quotient and
           subtract divisor from the remainder.
       */
      if (CMP_Compare (remainder, divisor) >= 0) {
         if ((status = CMP_SubtractInPlace (divisor, remainder)) != 0)
           break;
         if ((status = CMP_AddCMPWord ((CMPWord)1, quotient)) != 0)
           break;
      }

      /* Create new partial dividend by concatenating the next word of the
           original dividend.
       */
      if ((status = CMP_AppendWord
           (CMP_PINT_WORD (dividend, j), remainder)) != 0)
        break;
      if ((status = CMP_ShiftLeftByCMPWords (1, quotient)) != 0)
        break;
      if (CMP_PINT_LENGTH (remainder) <= bLength)
        continue;

      if ((status = CMP_EstimateMSWQuotient
           (remainder, divisor, &qj, &product)) != 0)
        break;

      /* Adjust qj and the product so that it is less than the new partial
           divisor.
       */
      while (CMP_Compare (&product, remainder) > 0) {
        if ((status = CMP_SubtractCMPWord ((CMPWord)1, &qj)) != 0)
          break;
        if ((status = CMP_SubtractInPlace (divisor, &product)) != 0)
          break;
      }
      if (status != 0)
        break;

      if ((status = CMP_AddCMPWord (CMP_INT_WORD (qj, 0), quotient)) != 0)
        break;
      if ((status = CMP_SubtractInPlace (&product, remainder)) != 0)
        break;
    }
    if (status != 0)
      break;

    /* Shift remainder, a, b to the right to compensate for initial left
         shift
     */
    if (shift > 0) {
      CMP_ShiftRightByBits (shift, remainder);
      CMP_ShiftRightByBits (shift, dividend); 
      CMP_ShiftRightByBits (shift, divisor);
    }

    if (CMP_Compare (remainder, divisor) >= 0) {
      if ((status = CMP_SubtractInPlace (divisor, remainder)) != 0)
        break;
      if ((status = CMP_AddCMPWord ((CMPWord) 1, quotient)) != 0)
        break;
    }
  } while (0);

  CMP_Destructor (&qj);
  CMP_Destructor (&product);

  return (status);
}

/* operand % modulus
     reducedValue = modulus - [operand * (operand / modulus)]
 */
CMPStatus
CMP_ModularReduce(CMPInt *operand, CMPInt *modulus, CMPInt *reducedValue)
{
  register int aLength, bLength, j;
  int shift, status;
  CMPInt qj, product;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (operand) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (operand) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (modulus) <= 0)
    return (CMP_LENGTH);
#endif

  CMP_Constructor (&qj);
  CMP_Constructor (&product);

  do { 
    /* if operand < modulus, reducedValue = operand.
     */
    if (CMP_Compare (operand, modulus) < 0) {
      status = CMP_Move (operand, reducedValue);
      break;
    }

    /* Compute left shift that will make the most significant bit of the
         most significant word of the modulus 1. Then shift modulus and
         operand accordingly
     */
    if ((status = CMP_GetOffsetOfMSB (modulus, &shift)) != 0)
      break;

    /* If shift is CMP_WORD_SIZE, the most significant word of modulus is
         zero, meaning it is zero, an error.
     */
    if (shift == CMP_WORD_SIZE) {
      status = CMP_MODULUS;
      break;
    }

    if (shift > 0) {
      if ((status = CMP_ShiftLeftByBits (shift, modulus)) != 0)
        break;
      if ((status = CMP_ShiftLeftByBits (shift, operand)) != 0)
        break;
    }

    /* Since operand >= modulus, aLength >= bLength */
    aLength = CMP_PINT_LENGTH (operand);
    bLength = CMP_PINT_LENGTH (modulus);

    /* Set up reducedValue so that it contains the first bLength + 1 most
         significant words of operand.
     */
    if ((status = CMP_Move (operand, reducedValue)) != 0)
      break;
    if (aLength > bLength) {
       if ((status = CMP_ShiftRightByCMPWords
            (aLength - bLength, reducedValue)) != 0)
         break;
    }
    /* Since operand >= modulus, aLength >= bLength. To reach this else
         clause, aLength is not > bLength, so they are equal. In this case,
         reducedValue is operand - modulus.
     */
    else {
      if ((status = CMP_SubtractInPlace (modulus, reducedValue)) != 0)
        break;
    }

    if ((status = CMP_reallocNoCopy (aLength, &product)) != 0)
      break;

    if ((status = CMP_reallocNoCopy (2, &qj)) != 0)
      break;

    for (j = aLength - bLength - 1; j >= 0; --j) {
      /* If the current reducedValue is > modulus, subtract modulus from
           the reducedValue.
       */
      if (CMP_Compare (reducedValue, modulus) >= 0) {
         if ((status = CMP_SubtractInPlace (modulus, reducedValue)) != 0)
           break;
      }

      /* Create new partial operand by concatenating the next word of the
           original operand.
       */
      if ((status = CMP_AppendWord
           (CMP_PINT_WORD (operand, j), reducedValue)) != 0)
        break;
      if (CMP_PINT_LENGTH (reducedValue) <= bLength)
        continue;

      if ((status = CMP_EstimateMSWQuotient
           (reducedValue, modulus, &qj, &product)) != 0)
        break;

      /* Adjust the product so that it is less than the new partial modulus.
       */
      while (CMP_Compare (&product, reducedValue) > 0) {
        if ((status = CMP_SubtractInPlace (modulus, &product)) != 0)
          break;
      }
      if (status != 0)
        break;

      if ((status = CMP_SubtractInPlace (&product, reducedValue)) != 0)
        break;
    }
    if (status != 0)
      break;

    /* Shift reducedValue, a, b to the right to compensate for initial left
         shift
     */
    if (shift > 0) {
      CMP_ShiftRightByBits (shift, reducedValue);
      CMP_ShiftRightByBits (shift, operand); 
      CMP_ShiftRightByBits (shift, modulus);
    }

    if (CMP_Compare (reducedValue, modulus) >= 0) {
      if ((status = CMP_SubtractInPlace (modulus, reducedValue)) != 0)
        break;
    }
  } while (0);

  CMP_Destructor (&qj);
  CMP_Destructor (&product);

  return (status);
}

/* Bump each CMPWord of a up 1, then set the least significant word to x.
     a <--- a * rho + x, where rho = 2 ^ CMP_WORD_SIZE
     In a representation of most significant word first, it would be
          ---- ---- ---- ----
           /    /    /    /
         /    /    /    /
        v    v    v    v   x
     ---- ---- ---- ---- ----

     In a representation of least significant word first, it would be
      ---- ---- ---- ----
       \    \    \    \
         \    \    \    \
      x   v    v    v    v
     ---- ---- ---- ---- ----
 */
int
CMP_AppendWord(CMPWord x, CMPInt *a)
{
  int status, aLength;

  aLength = CMP_PINT_LENGTH (a);

#ifdef CMP_DEBUG
  if (aLength <= 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  /* Check to see if a == 0 */
  if (CMP_PINT_WORD (a, aLength - 1) != (CMPWord)0) {
    /* Is the CMPInt large enough to take on a new word? */
    if (CMP_PINT_SPACE (a) < aLength + 1) {
      if ((status = CMP_realloc (aLength + 2, a)) != 0)
        return (status);
    }

    /* Move the words "up" one. a[0] to a[1], etc. */
    PORT_Memmove((unsigned char *)&(CMP_PINT_WORD (a, 1)),
		 (unsigned char *)CMP_PINT_VALUE (a),
		 aLength * CMP_BYTES_PER_WORD);
    CMP_PINT_WORD (a, 0)  = x;
    CMP_PINT_LENGTH (a)++;
    return (0);
  }

  /* The most significant word of a is 0.
   */
  CMP_PINT_WORD (a, 0) = x;
  CMP_PINT_LENGTH (a) = 1;

  return (0);
}

/* Returns an estimate of most significant word of the quotient of
     numerator / divisor. Estimate is based on two most significant words
     of divisor and three most significant words of numerator.
     COMMENT: Might be preferable to separate this into two different
              subroutines, one where denominator is at least two words,
              and the other where the denominator is a single word.
 */
int
CMP_EstimateMSWQuotient(CMPInt *numerator, CMPInt *divisor, CMPInt *quotient,
			CMPInt *product)
{
  CMPWord q0, r;
  CMPWord divisorMSW, numeratorMSW, numeratorMSW1;
  CMPInt partNum, partDiv;
  int numeratorLength, divisorLength, status;

  CMP_Constructor (&partNum);
  CMP_Constructor (&partDiv);

  do {
    divisorLength   = CMP_PINT_LENGTH (divisor);
    numeratorLength = CMP_PINT_LENGTH (numerator);

    if (divisorLength >= numeratorLength) {
      /* Set product and quotient to 0.
       */
      if (CMP_PINT_SPACE (product) < 1)
        if ((status = CMP_reallocNoCopy (2, product)) != 0)
          break;
      CMP_PINT_WORD (product, 0) = (CMPWord)0;
      CMP_PINT_LENGTH (product) = 1;

      if (CMP_PINT_SPACE (quotient) < 1)
        if ((status = CMP_reallocNoCopy (2, quotient)) != 0)
          break;
      CMP_PINT_WORD (quotient, 0) = (CMPWord)0;
      CMP_PINT_LENGTH (quotient) = 1;
      status = 0;
      break;
    }

    /* Initialize partial numerator and partial divisor.
     */
    if ((status = CMP_Move (numerator, &partNum)) != 0)
      break;
    if ((status = CMP_Move (divisor, &partDiv)) != 0)
      break;

    CMP_ShiftRightByCMPWords (max (numeratorLength - 3, 0), &partNum);
    CMP_ShiftRightByCMPWords (max (divisorLength - 2, 0), &partDiv);
    divisorMSW    = CMP_INT_WORD (partDiv, CMP_INT_LENGTH (partDiv) - 1);
    numeratorMSW  = CMP_INT_WORD (partNum, CMP_INT_LENGTH (partNum) - 1);
    numeratorMSW1 = CMP_INT_WORD (partNum, CMP_INT_LENGTH (partNum) - 2);    

    CMP_DivideTwoWordsByWord
      (divisorMSW, numeratorMSW, numeratorMSW1, &q0, &r);
    CMP_PINT_WORD (quotient, 0) = q0;
    CMP_PINT_LENGTH (quotient) = 1;

    /* With this new quotient, compute the product. */
    if ((status == CMP_Multiply (quotient, divisor, product)) != 0)
      break;
  } while (0);

  CMP_Destructor (&partNum);
  CMP_Destructor (&partDiv);  
  return (status);
}

/* computes q <--- (u0 * w + u1) / v; r <--- (u0 * w + u1) % v
 */
void
CMP_DivideTwoWordsByWord(CMPWord v, CMPWord u0, CMPWord u1, CMPWord *q,
			 CMPWord *r)
{
  CMPWord qUpper, rUpper, qLower, rLower;

  /* This routine is to be called only if u0 <= v. If u0 < v, then
       the result of the division is one word.
   */
  if (u0 < v) {
    CMP_DivOneAndHalfWordsByWord (v, u0, u1, &qUpper, &rUpper);
    CMP_DivOneAndHalfWordsByWord
      (v, rUpper, (CMPWord)(CMP_LOW_WORD (u1) << CMP_HALF_WORD_SIZE),
       &qLower, &rLower);

    *q = (qUpper << CMP_HALF_WORD_SIZE) + qLower;
    *r = rLower;
    return;
  }

  /* If u0 == v, then the result would normally be two words, the first
       of which is 1. Except u0 and u1 are the first two words of a number,
       N, that is less than a number, D, which has as its first word, v.
       Numerator < Denominator, compute N' = N * 2^w, find q s.t.
       q * D <= N', but (q + 1) * D > N'. Since N < D and N' = N * 2^w,
       if q >= 2^w, q * D would be > N'. Hence q < 2^w. How much less?
       If u0 == v, q is 2^w - 1. The rest of the proof is left as an
       exercise for the reader.
   */
  *q = CMP_WORD_MASK;
  *r = 0;
}

/* Assumptions: (u0 >> (CMP_WORD_SIZE - 1) = 1. 
     w = 2 ^ CMP_WORD_SIZE, rho = sqrt (w).
     Computes q : q * v * rho < u0 * w + u1, and
     r = (u0 * w + u1 - q * v * rho) < w * rho .
     r = (u0 * w + u1 - q * v * rho) >> CMP_HALF_WORD_SIZE

     Discussion: Set
       U  = u0 * rho + u1 / rho,
       v0 = v / rho .
       We then compute U / v and U % v as follows.
       a: Compute q0 = u0 / v0  (Note q0 >= U / v)
       b: Compute q0 * v
       c: While ( q0 * v  > U) {
          q0--;
          Update q0 * v by subtracting v from previous product
          }
 */
void
CMP_DivOneAndHalfWordsByWord(CMPWord v, CMPWord u0, CMPWord u1, CMPWord *q,
			     CMPWord *r)
{
  register CMPWord q0, v0, uUpper, uLower, q0vUpper, q0vLower;

  uUpper = CMP_HIGH_WORD (u0);
  uLower = (CMP_LOW_WORD (u0) << CMP_HALF_WORD_SIZE) | CMP_HIGH_WORD (u1);

  v0     = CMP_HIGH_WORD (v);
  q0     = u0 / v0;
  CMP_MULT_HIGHLOW (q0 , v, q0vLower, q0vUpper);

  while (q0vUpper > uUpper || (q0vUpper == uUpper && q0vLower > uLower)) {
    q0--;
    if (q0vLower >= v)
      q0vLower -= v;
    else {
      q0vLower = ~(v - q0vLower) + 1;
      q0vUpper--;
    }
  }
 *r = (uLower >= q0vLower) ? uLower - q0vLower : ~(q0vLower - uLower) + 1;
 *q = q0;
}

/* This routine computes result = base mod modulus when modulus is a
     CMPWord. If modulus is less than or equal to max half word, that is,
     no upper half bits are set, use the algorithm in this routine, if
     not, call another subroutine.
 */
CMPStatus
CMP_CMPWordModularReduce(CMPInt *base, CMPWord modulus, CMPWord *result)
{
  register int i;
  register CMPWord upperHalf, lowerHalf, currWord, scale;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  if (modulus == (CMPWord)0)
    return (CMP_MODULUS);

  /* If there are any bits in the upper half of the modulus, can still
       optimize, but not as much, call another special subroutine. */
  if ( (modulus & CMP_UPPER_WORD_MASK) != 0 )
    return (CMP_FullCMPWordModReduce (base, modulus, result));

  scale = CMP_SQRT_RADIX % modulus;

  /* The first iteration of the loop is
       *result = [(0 * scale) + upperHalf (msw (base))] mod modulus
       *result = [(*result * scale) + lowerHalf (msw (base))] mod modulus
   */
  currWord = CMP_PINT_WORD (base, CMP_PINT_LENGTH (base) - 1);
  upperHalf = (currWord & CMP_UPPER_WORD_MASK) >> CMP_HALF_WORD_SIZE;
  lowerHalf = currWord & CMP_HALF_WORD_MASK;
  *result = upperHalf % modulus;
  *result = (*result * scale) + lowerHalf;
  *result = *result % modulus;

  for (i = CMP_PINT_LENGTH (base) - 2; i >= 0; --i) {
    currWord = CMP_PINT_WORD (base, i);
    upperHalf = (currWord & CMP_UPPER_WORD_MASK) >> CMP_HALF_WORD_SIZE;
    lowerHalf = currWord & CMP_HALF_WORD_MASK;
    *result = (*result * scale) + upperHalf;
    *result = *result % modulus;
    *result = (*result * scale) + lowerHalf;
    *result = *result % modulus;
  }
  return (0);
}

/* This routine computes result = base mod modulus when modulus is greater
     than the max half CMPWord, that is, the upper half is not all zeros.
 */
int
CMP_FullCMPWordModReduce(CMPInt *base, CMPWord modulus, CMPWord *result)
{
  int status;
  register int i;
  register CMPWord scaleAsWord;
  CMPInt modAsCMPInt, scale, intermediate, temp;

  CMP_Constructor (&modAsCMPInt);
  CMP_Constructor (&scale);
  CMP_Constructor (&intermediate);
  CMP_Constructor (&temp);

  /* The scale is (WORD_MASK + 1) mod modulus.
   */
  scaleAsWord = CMP_WORD_MASK - (modulus - 1);
  if (scaleAsWord >= modulus)
    scaleAsWord = scaleAsWord % modulus;

  do {
    /* Get the modulus into a CMPInt.
     */
    if ((status = CMP_reallocNoCopy (1, &modAsCMPInt)) != 0)
      break;
    CMP_INT_WORD (modAsCMPInt, 0) = modulus;
    CMP_INT_LENGTH (modAsCMPInt) = 1;

    /* Get the scale int a CMPInt.
     */
    if ((status = CMP_reallocNoCopy (1, &scale)) != 0)
      break;
    CMP_INT_WORD (scale, 0) = scaleAsWord;
    CMP_INT_LENGTH (scale) = 1;

    /* temp will be the result of multiplying a one CMPWord CMPInt by
         another, so its max size will be 2 CMPWords. Initialize it to
         the most significant word of base.
     */
    if ((status = CMP_reallocNoCopy (2, &temp)) != 0)
      break;
    CMP_INT_WORD (temp, 0) = CMP_PINT_WORD (base, CMP_PINT_LENGTH (base) - 1);
    CMP_INT_LENGTH (temp) = 1;

    /* The first iteration of the loop is
         intermediate = [(0 * scale) + msw (base)] mod modulus.
     */
    if ((status = CMP_ModularReduce
         (&temp, &modAsCMPInt, &intermediate)) != 0)
      break;

    /* Loop on each CMPWord of base
         intermediate = [(intermediate * scale) + nextWord (base)] mod modulus
     */
    for (i = CMP_PINT_LENGTH (base) - 2; i >= 0; --i) {
      if ((status = CMP_Multiply (&scale, &intermediate, &temp)) != 0)
        break;
      if ((status = CMP_AddCMPWord
           (CMP_PINT_WORD (base, i), &temp)) != 0)
        break;
      if ((status = CMP_ModularReduce
           (&temp, &modAsCMPInt, &intermediate)) != 0)
        break;
    }
  } while (0);

  /* The answer is the last value intermediate took on. */
  if (status == 0)
    *result = CMP_INT_WORD (intermediate, 0);

  CMP_Destructor (&modAsCMPInt);
  CMP_Destructor (&scale);
  CMP_Destructor (&intermediate);
  CMP_Destructor (&temp);

  return (status);
}

CMPStatus
CMP_ComputeGCD(CMPInt *u, CMPInt *v, CMPInt *gcd)
{
  int status, gcdFlag;
  CMPInt temp1, temp2;

  CMP_Constructor (&temp1);
  CMP_Constructor (&temp2);

  do {
    /* Initialize temp1 to be the larger of the two operands, and gcd to be
         the smaller.
     */
    if (CMP_Compare (u, v) >= 0) {
      if ((status = CMP_Move (u, &temp1)) != 0)
        break;
      if ((status = CMP_Move (v, gcd)) != 0)
        break;
    }
    else {
      if ((status = CMP_Move (v, &temp1)) != 0)
        break;
      if ((status = CMP_Move (u, gcd)) != 0)
        break;
    }

    /* If the program ends now, one of the arguments was 0. If the other
         argument is not 0, temp1 contains the answer, if the other argument
         is also 0, this is an error. Set gcdFlag to 3. */
    gcdFlag = 3;

    /* Reduce the larger mod the smaller, then repeat until the result of
         the reduction is 0. At that point, the answer is the smaller.
     */
    while (CMP_PINT_WORD (gcd, CMP_PINT_LENGTH (gcd) - 1) != (CMPWord)0) {
      if ((status = CMP_ModularReduce (&temp1, gcd, &temp2)) != 0)
        break;

      /* If the result of the reduction is 0, gcd contains the answer.
       */
      gcdFlag = 0;
      if (CMP_INT_WORD (temp2, CMP_INT_LENGTH (temp2) - 1) == (CMPWord)0)
        break;

      /* gcd now contains the larger, temp2 the smaller. */
      if ((status = CMP_ModularReduce (gcd, &temp2, &temp1)) != 0)
        break;

      /* If the result of the reduction is 0, temp2 contains the answer.
       */
      gcdFlag = 2;
      if (CMP_INT_WORD (temp1, CMP_INT_LENGTH (temp1) - 1) == (CMPWord)0)
        break;

      /* temp2 now contains the larger, temp1 the smaller. */
      if ((status = CMP_ModularReduce (&temp2, &temp1, gcd)) != 0)
        break;

      /* If the result of the reduction is 0, temp1 contains the answer.
       */
      gcdFlag = 1;
    }
    if (status != 0)
      break;

    switch (gcdFlag) {
      case 1:
        status = CMP_Move (&temp1, gcd);
	break;

      case 2:
        status = CMP_Move (&temp2, gcd);
	break;

      /* The smaller of the two incoming operands was 0, check the larger
           (which is in temp1). If it is not zero, the answer is the larger,
           if it is zero, return an error.
       */
      case 3:
        if (CMP_INT_WORD (temp1, CMP_INT_LENGTH (temp1) - 1) != 0)
          status = CMP_Move (&temp1, gcd);
        else
          status = CMP_DOMAIN;

      default:
        ;
    }

  } while (0);

  CMP_Destructor (&temp1);
  CMP_Destructor (&temp2);
  return (status);
}

/* CMP_ComputeExtendedGCD computes coeffA, coeffB and the gcd
     For a and b, find
       a*u + b*v = gcd
     but this would mean either a or b is negative, so this routine
     actually computes
       coeffA = aPrime mod v
       coeffB = bPrime mod u
     where
       (aPrime * u) + (bPrime * v) = gcd
     Recall that for a negative x, x mod n = (n + x) mod n

     Skipping some proofs,
       (coeffA * u) mod v = gcd
       (coeffB * v) mod u = gcd

     If u and v are relatively prime, then the gcd is 1 and
       (coeffA * u) mod v = 1
       (coeffB * v) mod u = 1
     So
       coeffA = inverse (u) mod v
       coeffB = inverse (v) mod u

     (see KNUTH vol 2)
  */
 /* Explanation:
      Without loss of generality assume that u >= v.
      Let u[0] = u, u[1] = v, and in general
      set u[n] = u[n - 2] mod u[n - 1],
      and q[n] = u[n - 2] div u[n - 1].
      By induction it may be shown that the greatest common denominator
      of u and v equals u[n0] : u[n0 - 1] mod u[n0] == 0. Moreover for
      each n >= 0 there exist integers such that
      u[n] = a[n] * u[0] + b[n] * u[1]. Therefore
      gcd = a[n0] * u + b[n0] * v . By definition a[0] = 1 and a[1] = 0;
      To get the iterative formulas for a[n] observe that

      u[n + 1] = u[n - 2] - q[n + 1] * u[n - 1]

      Using the inductive hypothesis yields

      u[n + 1] = a[n - 2] * u[0] + b[n - 2] * u[1] 
                 - q[n + 1] * (a[n - 1] * u[0] + b[n - 1] * u[1])

      or that
      u[n + 1] = (a[n - 2] - q[n + 1] * a[n - 1]) * u[0] +
                 (b[n - 2] - q[n + 1] * b[n - 1]) * u[1]

      Now notice that the computation of a[n] is independent of b[n], thus
      we may avoid the computation of the intermediate values of b[n] and
      compute only the required b by using the equation
      b = (gcd - a * u) / v

      Ignoring the obvious complication that we need to implement this
      in unsigned arithmetic this is what this algorithm does. One final
      complication is that we are returning not a[n0] and b[n0] but their
      mod u and mod v positive, respectively, analogues.
  */
int
CMP_ComputeExtendedGCD(CMPInt *u, CMPInt *v, CMPInt *coeffA, CMPInt *coeffB,
		       CMPInt *gcd)
{
  CMPInt *biggerCMP, *smallerCMP;
  CMPInt dividendP, divisorP, prevCoeffA, temp1, temp2;
  int status, uComparedToV, divideFlag;
  int signCoeffA     = 1;
  int signPrevCoeffA = 1;

  CMP_Constructor (&dividendP);
  CMP_Constructor (&divisorP);
  CMP_Constructor (&prevCoeffA);
  CMP_Constructor (&temp1);
  CMP_Constructor (&temp2);

  do {
    /* Set dividendP to larger of u and v, and divisorP to smaller of u and v */
    if (CMP_Compare (u, v) >= 0) {
      biggerCMP  = u;
      smallerCMP = v;
      if ((status = CMP_Move (u, &dividendP)) != 0)
        break;
      if ((status = CMP_Move (v, &divisorP)) != 0)
        break;
      uComparedToV = U_BIGGER_THAN_V;
    } else {
      biggerCMP  = v;
      smallerCMP = u;      
      if ((status = CMP_Move (v, &dividendP)) != 0)
        break;
      if ((status = CMP_Move (u, &divisorP)) != 0)
        break;       
      uComparedToV = U_SMALLER_THAN_V;
    }

    /* Initialize prevCoeffA to 1. */
    if ((status = CMP_reallocNoCopy (1, &prevCoeffA)) != 0)
      break;
    CMP_INT_LENGTH (prevCoeffA) = 1;
    CMP_INT_WORD (prevCoeffA, 0) = (CMPWord)1;

    /* Initialize coeffA to 0. */
    if (CMP_PINT_SPACE (coeffA) < 1)
      if ((status = CMP_reallocNoCopy (1, coeffA)) != 0)
        break;
    CMP_PINT_LENGTH (coeffA) = 1;
    CMP_PINT_WORD (coeffA, 0) = (CMPWord)0;

    /* If the smaller operand is 0, coeffA is 0 and coeffB is 1. Unless the
         bigger operand is also 0, in which case, this is an error.
     */
    if (CMP_PINT_WORD (smallerCMP, CMP_PINT_LENGTH (smallerCMP) - 1) == 0) {
      if (CMP_PINT_WORD (biggerCMP, CMP_PINT_LENGTH (biggerCMP) - 1) == 0) {
        status = CMP_DOMAIN;
	break;
      }

      /* Set gcd to the biggerCMP, coeffA is already 0, set coeffB to 1.
       */
      if ((status = CMP_Move (biggerCMP, gcd)) != 0)
        break;
      if (CMP_PINT_SPACE (coeffB) < 1)
        if ((status = CMP_reallocNoCopy (1, coeffB)) != 0)
          break;
      CMP_PINT_LENGTH (coeffB) = 1;
      CMP_PINT_WORD (coeffB, 0) = (CMPWord)1;
      break;
    }

    /* find dividendP / divisorP (the quotient) and dividendP % divisorP
         (the remainder). We need a CMPInt to hold the quotient, use coeffB.
     */
    if ((status = CMP_Divide
         (&dividendP, &divisorP, coeffB, gcd)) != 0)
      break;

    /* If the remainder (sitting in gcd) is 0 after this first divide,
         this is a special case.
     */
    if (CMP_PINT_WORD (gcd, CMP_PINT_LENGTH (gcd) - 1) == (CMPWord)0) {
      /* Set coeffA to 0 and coeffB to 1. Actually, coeffA already
           contains a zero. The gcd is min (u, v).
       */
      CMP_PINT_LENGTH (coeffB) = 1;
      CMP_PINT_WORD (coeffB, 0) = (CMPWord)1;
      status = CMP_Move (smallerCMP, gcd);
      break;
    }

    /* Iterate until the remainder is 0.
     */
    do {
      if ((status = CMP_UpdateCoefficients
           (&temp1, &temp2, coeffB, &signPrevCoeffA, &signCoeffA,
            &prevCoeffA, coeffA)) != 0)
        break;

      /* What was the divisor becomes the dividend, what was the remainder
           becomes the divisor.
       */
      if ((status = CMP_Divide
           (&divisorP, gcd, coeffB, &dividendP)) != 0)
        break;

      /* divideFlag of 1 means the last divisor is in gcd. */
      divideFlag = 1;
      if (CMP_INT_WORD (dividendP, CMP_INT_LENGTH (dividendP) - 1) ==
          (CMPWord)0)
        break;

      if ((status = CMP_UpdateCoefficients
           (&temp1, &temp2, coeffB, &signPrevCoeffA, &signCoeffA,
            &prevCoeffA, coeffA)) != 0)
        break;

      /* What was the divisor becomes the dividend, what was the remainder
           becomes the divisor.
       */
      if ((status = CMP_Divide
           (gcd, &dividendP, coeffB, &divisorP)) != 0)
        break;

      /* divideFlag of 2 means the last divisor is in dividendP. */
      divideFlag = 2;
      if (CMP_INT_WORD (divisorP, CMP_INT_LENGTH (divisorP) - 1) ==
          (CMPWord)0)
        break;

      if ((status = CMP_UpdateCoefficients
           (&temp1, &temp2, coeffB, &signPrevCoeffA, &signCoeffA,
            &prevCoeffA, coeffA)) != 0)
        break;

      /* What was the divisor becomes the dividend, what was the remainder
           becomes the divisor.
       */
      if ((status = CMP_Divide
           (&dividendP, &divisorP, coeffB, gcd)) != 0)
        break;

      /* divideFlag of 0 means the last divisor is in divisorP. */
      divideFlag = 0;
    } while (CMP_PINT_WORD (gcd, CMP_PINT_LENGTH (gcd) - 1) != (CMPWord)0);
    if (status != 0)
      break;

    /* The last divisor (which may be sitting in divisorP, dividendP or gcd)
         is the greatest common divisor.
     */
    switch (divideFlag) {
      case 0:
        status = CMP_Move (&divisorP, gcd);
        break;

      case 2:
        status = CMP_Move (&dividendP, gcd);

      default:
        ;
    }

    /* coeffB <---  (biggerCMP * coeffA - signCoeffA * gcd) / smallerCMP
     */
    if ((status = CMP_Multiply (coeffA, biggerCMP, &temp1)) != 0)
      break;

    if (signCoeffA > 0) {
      if ((status = CMP_Subtract (&temp1, gcd, &temp2)) != 0)
        break;
    }
    else {
      if ((status = CMP_Add (&temp1, gcd, &temp2)) != 0)
        break;
    }

    if ((status = CMP_Divide (&temp2, smallerCMP, coeffB, &temp1)) != 0)
      break;

    /* if coeffA > 0 then coeffB < 0 so then coeffB <--- -coeffB mod biggerCMP
     */
    if (signCoeffA > 0) {
      if ((status = CMP_Subtract (biggerCMP, coeffB, &temp2)) != 0)
        break;
      if ((status = CMP_Move (&temp2, coeffB)) != 0)
        break;
    }
    /* if coeffB > 0 then coeffA < 0 so then coeffA <--- -coeffA mod smallerCMP
     */
    else {
      if ((status = CMP_Subtract (smallerCMP, coeffA, &temp2)) != 0)
        break;
      if ((status = CMP_Move (&temp2, coeffA)) != 0)
        break;
    }

  } while (0);

  /* if u is smaller than v then swap coeffA and coeffB
   */
  if ( (uComparedToV == U_SMALLER_THAN_V) && (status == 0) ) {
    do {
      if ((status = CMP_Move (coeffA, &temp1)) != 0)
        break;
      if ((status = CMP_Move (coeffB, coeffA)) != 0)
        break;
      if ((status = CMP_Move (&temp1, coeffB)) != 0)      
        break;
    } while (0);
  }

  CMP_Destructor (&dividendP);
  CMP_Destructor (&divisorP);
  CMP_Destructor (&prevCoeffA);
  CMP_Destructor (&temp1);
  CMP_Destructor (&temp2);

  return (status);
}      

/* an1     <--- an - (q * an1)
   an      <--- an1
   sign_n  <--- sign_n1;
   sign_n1 <--- sign [ (sign_n * an) - (sign_n1 * q * an1) ];
 */
int
CMP_UpdateCoefficients(CMPInt *an2, CMPInt *an1Xq, CMPInt *q, int *sign_n,
		       int *sign_n1, CMPInt *an, CMPInt *an1)
{
  int anSign, signDiff, status;

  /* Save current sign of an, but set new sign of an to sign of an1. */
  anSign = *sign_n;
  *sign_n = *sign_n1;

  do {
    if ((status = CMP_Multiply (q, an1, an1Xq)) != 0)
      break;

    /* If an and an1 are of different signs, add to find the difference.
     */
    if (*sign_n1 != anSign) {
      if ((status = CMP_Add (an, an1Xq, an2)) != 0)
        break;
      *sign_n1 = -(*sign_n1);
    }
    /* If an and an1 are the same sign, find the absolute value of their
         difference.
     */
    else {
      if ((status != CMP_AbsDifference (an, an1Xq, &signDiff, an2)) != 0)
        break;
      *sign_n1 = *sign_n1 * signDiff;
    }

    /* an <-- an1,   an1 <-- an2
     */
    if ((status = CMP_Move (an1, an)) != 0)
      break;
    if ((status = CMP_Move (an2, an1)) != 0)
      break;  
  } while (0);

  return (status);
}

/* Compute the absolute value of (a - b), setting the sign of the difference.
 */
int
CMP_AbsDifference(CMPInt *a, CMPInt *b, int *signDiff, CMPInt *difference)
{
  if (CMP_Compare (a, b) >= 0) {
    *signDiff = 1;
    return (CMP_Subtract (a, b, difference));
  }
  else {
    *signDiff = -1;
    return (CMP_Subtract (b, a, difference));
  }
}
