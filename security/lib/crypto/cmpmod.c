/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains routines that perform modular arithmatic. Modular
     reduction resides in the multiply divide file because it is based
     on and uses the local support routines of divide.
 */
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

/* sum = (addend1 + addend2) mod modulus
 */
CMPStatus
CMP_ModAdd(CMPInt *addend1, CMPInt *addend2, CMPInt *modulus, CMPInt *sum)
{
  int status;
  CMPInt reducedSum;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (addend1) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (addend1) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (addend2) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (addend2) <= 0)
    return (CMP_LENGTH);
#endif

  CMP_Constructor (&reducedSum);

  do {
    if ((status = CMP_Add (addend1, addend2, sum)) != 0)
      break;

    /* If the result of the addition is < modulus, done.
     */
    if (CMP_Compare (sum, modulus) < 0)
      break;

    /* Otherwise, reduce.
     */
    if ((status = CMP_SubtractInPlace (modulus, sum)) != 0)
      break;

    /* One subtraction was probably all it took ... */
    if (CMP_Compare (sum, modulus) < 0)
      break;

    /* ... if not, though, reduce. */
    if ((status = CMP_ModularReduce (sum, modulus, &reducedSum)) != 0)
      break;
    status = CMP_Move (&reducedSum, sum);
  } while (0);

  CMP_Destructor (&reducedSum);

  return (status);
}

/* Modular subtraction routine.
     if (minuend >= subtrahend) then
       difference = (minuend - subtrahend) mod modulus
     but if (subtrahend > minuend
       difference = modulus - [ (subtrahend - minuend) mod modulus ]
 */
CMPStatus
CMP_ModSubtract(CMPInt *minuend, CMPInt *subtrahend, CMPInt *modulus,
		CMPInt *difference)
{
  int status;
  CMPInt reducedDifference;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (minuend) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (minuend) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (subtrahend) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (subtrahend) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (modulus) <= 0)
    return (CMP_LENGTH);
#endif

  CMP_Constructor (&reducedDifference);

  do {
    if (CMP_Compare (minuend, subtrahend) >= 0) {
      /* If the minuend >= subtrahend, do regular subtraction followed by
           modular reduction if necessary.
       */
      if ((status = CMP_Subtract
           (minuend, subtrahend, difference)) != 0)
        break;

      /* If the result of that subtraction is < modulus, done.
       */
      if (CMP_Compare (difference, modulus) < 0)
        break;

      /* Otherwise, reduce.
       */
      if ((status = CMP_ModularReduce
           (difference, modulus, &reducedDifference)) != 0)
        break;

      status = CMP_Move (&reducedDifference, difference);
      break;
    }

    /* subtrahend > minuend, first find subtrahend - minuend.
     */
    if ((status = CMP_Subtract
         (subtrahend, minuend, &reducedDifference)) != 0)
      break;

    /* If the difference <= modulus, the answer is modulus - difference.
     */
    if (CMP_Compare (&reducedDifference, modulus) <= 0) {
      status = CMP_Subtract (modulus, &reducedDifference, difference);
    }

    /* (subtrahend - minuend) > modulus, find
          answer = modulus - [(subtrahend - minuend) mod modulus]
     */
    else {
      if ((status = CMP_Move (&reducedDifference, difference)) != 0)
        break;
      if ((status = CMP_ModularReduce
           (difference, modulus, &reducedDifference)) != 0)
        break;

      /* If the original subtrahend = minuend + (k * modulus), at this
           point, reducedDifference = 0 and the result should be 0. We
           know that difference has space of at least 1.
       */
      if ( (CMP_INT_LENGTH (reducedDifference) == 1) &&
           (CMP_INT_WORD (reducedDifference, 0) == (CMPWord)0) ) {
        CMP_PINT_LENGTH (difference) = 1;
        CMP_PINT_WORD (difference, 0) = (CMPWord)0;
      }
      else {
        status = CMP_Subtract
          (modulus, &reducedDifference, difference);
      }
    }
  } while (0);

  CMP_Destructor (&reducedDifference);

  return (status);
}

/*  product = (multiplicand * multiplier) mod modulus
 */
CMPStatus
CMP_ModMultiply(CMPInt *multiplicand, CMPInt *multiplier, CMPInt *modulus,
		CMPInt *product)
{
  int status;
  CMPInt intermediateProduct;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (multiplicand) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (multiplicand) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (multiplier) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (multiplier) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (modulus) <= 0)
    return (CMP_LENGTH);
#endif

  CMP_Constructor (&intermediateProduct);

  do {
    if ((status = CMP_Multiply
         (multiplicand, multiplier, &intermediateProduct)) != 0)
      break;

    status = CMP_ModularReduce (&intermediateProduct, modulus, product);
  } while (0);

  CMP_Destructor (&intermediateProduct);

  return(status);
}

/* Computes modular inverse.
*/
CMPStatus
CMP_ModInvert(CMPInt *operand, CMPInt *modulus, CMPInt *inverse)
{
  int status;
  CMPInt coeff2, gcd;

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

  CMP_Constructor (&coeff2);
  CMP_Constructor (&gcd);

  do {
    /* If the operand >= modulus, error. */
    if (CMP_Compare (operand, modulus) >= 0) {
      if (CMP_PINT_WORD (modulus, CMP_PINT_LENGTH (modulus) - 1) == (CMPWord)0)
        status = CMP_MODULUS;
      else
        status = CMP_RANGE;
      break;
    }

    if ((status = CMP_ComputeExtendedGCD
         (operand, modulus, inverse, &coeff2, &gcd)) != 0)
      break;

    if ( (CMP_INT_LENGTH (gcd) != 1) ||
         (CMP_INT_WORD (gcd, 0) != (CMPWord)1) )
      status = CMP_INVERSE;

  } while (0);

  CMP_Destructor (&coeff2);
  CMP_Destructor (&gcd);

  return (status);
}
