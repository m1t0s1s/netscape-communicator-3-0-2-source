/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains routines that perform Montgomery math.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

/* Convert a CMPInt to Montgomery representation. Actually, the Montgomery
     representation will reside in a CMPInt.
     montRep <- sourceCMP * R mod modulus,
     where R = 2 ^ (CMP_WORD_SIZE * (modulus->length + 1))
 */
int
CMP_ConvertToMont(CMPInt *sourceCMPInt, CMPInt *modulus, CMPInt *montRep)
{
    CMPInt srcXMod;
    int status;
    register int i, montRepSize, sizeShift, modLen, sourceLen;

#ifdef CMP_DEBUG
    if (CMP_PINT_VALUE (sourceCMPInt) == CMP_NULL_VALUE)
	return (CMP_INVALID_ADDRESS);
    if (CMP_PINT_LENGTH (sourceCMPInt) <= 0)
	return (CMP_LENGTH);
    if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
	return (CMP_INVALID_ADDRESS);
    if (CMP_PINT_LENGTH (modulus) <= 0)
	return (CMP_LENGTH);
#endif

    modLen = CMP_PINT_LENGTH (modulus);
    sourceLen = CMP_PINT_LENGTH (sourceCMPInt);

    /* sourceCMPInt must be less than modulus. Inline the compare function.
     */
    if (modLen < sourceLen)
	return (CMP_RANGE);
    if (modLen == sourceLen) {
	for (i = modLen - 1;
	     (i >= 0) &&
		 (CMP_PINT_WORD (modulus, i) == CMP_PINT_WORD (sourceCMPInt, i));
	     --i);

	/* If i < 0, we went through the loop and all words were equal. */
	if (i < 0)
	    return (CMP_RANGE);

	/* The ith word is the most significant word that differs. */
	if (CMP_PINT_WORD (modulus, i) < CMP_PINT_WORD (sourceCMPInt, i))
	    return (CMP_RANGE);
    }

    sizeShift = modLen - 1;
    montRepSize = (2 * modLen) + 1;
    CMP_Constructor (&srcXMod);

    do {
	if ((status = CMP_reallocNoCopy (montRepSize, &srcXMod)) != 0)
	    break;

	/* Initialize srcXMod = a * R;  R = w ^ nlength; w = 2 ^ CMP_WORD_SIZE
	 */
	if ((status = CMP_Move (sourceCMPInt, &srcXMod)) != 0)
	    break;

	if ((status = CMP_ShiftLeftByCMPWords (sizeShift + 1, &srcXMod)) != 0)
	    break;

	/* montRep = srcXMod mod (modulus) */
	if ((status = CMP_ModularReduce (&srcXMod, modulus, montRep)) != 0)
	    break;
    } while (0);

    CMP_Destructor (&srcXMod);
    return(status);
}

/* Convert a Montgomery representation integer back into normal format.
 */
int
CMP_ConvertFromMont(CMPInt *montRep, CMPInt *modulus, CMPWord n0Prime,
		    CMPInt *reducedCMP)
{
    CMPInt unit;
    int status;

#ifdef CMP_DEBUG
    if (CMP_PINT_VALUE (montRep) == CMP_NULL_VALUE)
	return (CMP_INVALID_ADDRESS);
    if (CMP_PINT_LENGTH (montRep) <= 0)
	return (CMP_LENGTH);
    if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
	return (CMP_INVALID_ADDRESS);
    if (CMP_PINT_LENGTH (modulus) <= 0)
	return (CMP_LENGTH);
#endif

    CMP_Constructor (&unit);

    do {
	/* Create a CMPInt equal to 1.
	 */
	if ((status = CMP_reallocNoCopy (1, &unit)) != 0)
	    break;
	CMP_INT_WORD (unit, 0) = (CMPWord)1;
	CMP_INT_LENGTH (unit) = 1;

	if ((status = CMP_MontProduct
	     (montRep, &unit, modulus, n0Prime, reducedCMP)) != 0)
	    break;
    } while (0);

    CMP_Destructor (&unit);  
    return(status);
}

/* Computes the least significant word of nPrime, the negative inverse of
     n mod r = 2^k. k is CMP_WORD_SIZE.
 */
int
CMP_ComputeMontCoeff(CMPInt *n, CMPWord *n0Prime)
{
    register int i;
    register CMPWord powerOfTwo, modularMask, nTimesY, lswOfN, y;

#ifdef CMP_DEBUG
    if (CMP_PINT_VALUE (n) == CMP_NULL_VALUE)
	return (CMP_INVALID_ADDRESS);
    if (CMP_PINT_LENGTH (n) <= 0)
	return (CMP_LENGTH);
#endif

    lswOfN = CMP_PINT_WORD (n, 0);

    /* The modulus must be odd. */
    if ( (lswOfN & 1) == 0 )
	return (CMP_MODULUS);

    /* Find y[1] through y[Ln 2^k]. */

    /* y[1] = 1, then compute successive y's. */
    y = (CMPWord)1;

    /* powerOfTwo is 2^i. The loop begins with i = 2, so initialize
       accordingly. */
    powerOfTwo = CMP_LSB_MASK << 1;

    /* Each iteration of the loop will examine n * y[i - 1] mod 2^i. Anything
       mod 2^i is simply the last i bits, so simply mask. */
    modularMask = powerOfTwo | CMP_LSB_MASK;

    /* Each iteration examines n * y[i - 1] mod 2^i. For i = 2, y[i - 1] is 1,
       so the first nTimesY is simply the last two bits of n. */
    nTimesY = modularMask & lswOfN;

    /* Compare 2^i to n * y[i - 1]. If 2^i is less, y[i] = y[i - 1] + 2^i.
       Otherwise, y[i] = y[i - 1].
       */

    /* First iteration, i = 2. y is currently set to y[i - 1] */
    if (powerOfTwo < nTimesY)
	y += powerOfTwo;
    /* y is now y[i]. */

    for (i = 3; i <= CMP_WORD_SIZE; ++i) {
	powerOfTwo = powerOfTwo << 1;
	modularMask |= powerOfTwo;

	/* Incremented i, so y is currently y[i - 1]. */
	nTimesY = (lswOfN * y) & modularMask;

	if (powerOfTwo < nTimesY)
	    y += powerOfTwo;
	/* y is now y[i]. */
    }

    /* -n0Prime is the last y. */
    *n0Prime = ~y + 1;

    return (0);
}

/* If the assembly version exists, don't compile. */
#ifndef __MONT_SQUARE_ASSEMBLY

/* Use Dusse-Kaliski method for Montgomery squaring.
     montSquare <---  operand * operand * (R ^ (-1)) mod modulus.
 */
int
CMP_MontSquare(CMPInt *operand, CMPInt *modulus, CMPWord n0Prime,
	       CMPInt *montSquare)
{
    register int i;
  register CMPWord *sqval;
  register CMPWord m_i;
  register int oLength, mLength, mLength2, sqToMCompare;
    int status;

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

    mLength = CMP_PINT_LENGTH (modulus);
    mLength2 = 2 * mLength;
    oLength  = CMP_PINT_LENGTH (operand);

    /* Initialize montSquare to be mLength2 + 3 CMPWords long, all of them zero.
     */
    if (CMP_PINT_SPACE (montSquare) < (mLength2 + 3))
	if ((status = CMP_reallocNoCopy (mLength2 + 3, montSquare)) != 0)
	    return (status);
    sqval = CMP_PINT_VALUE (montSquare);
    PORT_Memset((unsigned char *)sqval, 0, (mLength2 + 3) * CMP_BYTES_PER_WORD);
    CMP_PINT_LENGTH (montSquare) = 1;

    /* For each CMPWord in operand except the last, compute
       montSquare = operand[i] * operand + montSquare
       except begin operand at the (i + 1)th CMPWord, and begin montSquare
       at the ((2 * i) + 1)th CMPWord. This is part of polynomial squaring.
       */
  for (i = 0; i < oLength - 1; ++i) 
    CMP_VectorMultiply
      (CMP_PINT_WORD (operand, i), operand, i + 1,
       CMP_PINT_LENGTH (operand) - (i + 1), montSquare, (2 * i) + 1);

    if ((status = CMP_RecomputeLength (mLength2, montSquare)) != 0)
	return (status);

    /* Find montSquare * 2, do this by shifting one bit to the left. */
    if ((status = CMP_ShiftLeftByBits (1, montSquare)) != 0)
	return (status);

    /* This routine squares each member of operand, adding the result to
       two words of montSquare. Part of the polynomial squaring process. */
    CMP_AddInTrace (operand, montSquare);

  for (i = 0; i < mLength; ++i) {
	/* m_i  <---  montSquare[i] * n0Prime mod w
	   The following multiplication assumes the result is the low 32
	   bits of the true product. If some platform/compiler does not
	   return the low 32 bits of the true product, use the macro
	   CMP_MULT_LOW (sqval[i], n0Prime, m_i) */
	m_i = sqval[i] * n0Prime;

	/* montSquare <--- m_i * modulus + montSquare */
	CMP_VectorMultiply (m_i, modulus, 0, mLength, montSquare, i);
    }
    CMP_PINT_LENGTH (montSquare) = mLength2 + 1;

    /* Extract upper half of result */
    CMP_ShiftRightByCMPWords (mLength, montSquare);
    CMP_RecomputeLength (mLength, montSquare); 

    /* Subtract modulus from montSquare if possible.
     */
    sqToMCompare = CMP_Compare (modulus, montSquare);
    if (sqToMCompare < 0){
	if ((status = CMP_SubtractInPlace (modulus, montSquare)) != 0)
	    return (status);
    }
    else {
	if (sqToMCompare == 0) {
	    /* the result is zero */
	    CMP_PINT_WORD (montSquare, 0) = 0;
	    CMP_PINT_LENGTH (montSquare) = 1;
	}
    }

    return (0);
}
#endif

#ifndef __MONT_PRODUCT_ASSEMBLY
/* Use Dusse-Kaliski method for Montgomery product.
     montProduct = multiplicand * multiplier * (R ^ (-1)) mod modulus.
 */
int
CMP_MontProduct(CMPInt *multiplicand, CMPInt *multiplier, CMPInt *modulus,
		CMPWord n0Prime, CMPInt *montProduct)
{
    register int i, aLength, mLength, mLength2, pToMCompare;
    register CMPWord *aval, *pval, m_i;
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
    if (CMP_PINT_VALUE (modulus) == CMP_NULL_VALUE)
	return (CMP_INVALID_ADDRESS);
    if (CMP_PINT_LENGTH (modulus) <= 0)
	return (CMP_LENGTH);
#endif

    mLength = CMP_PINT_LENGTH (modulus);
    mLength2 = 2 * mLength;
 
    /* Initialize mLength2 + 3 CMPWords of montProduct to be zero.
     */
    if (CMP_PINT_SPACE (montProduct) < (mLength2 + 3))
	if ((status = CMP_reallocNoCopy (mLength2 + 3, montProduct)) != 0)
	    return (status);
    pval = CMP_PINT_VALUE (montProduct);
    PORT_Memset((unsigned char *)pval, 0, (mLength2 + 3) * CMP_BYTES_PER_WORD);

    CMP_PINT_LENGTH (montProduct) = 1;
    aval    = CMP_PINT_VALUE (multiplicand);
    aLength = CMP_PINT_LENGTH (multiplicand);

    for (i = 0; i < aLength; ++i) {
	/* montProduct <--- multiplicand[i] * multiplier + montProduct */
	CMP_VectorMultiply
	    (aval[i], multiplier, 0, CMP_PINT_LENGTH (multiplier), montProduct, i);

	/* m_i  <---  montProduct[i] * n0Prime mod w
	   The following multiplication assumes the result is the low 32
	   bits of the true product. If some platform/compiler does not
	   return the low 32 bits of the true product, use the macro
	   CMP_MULT_LOW (pval[i], n0Prime, m_i) */
	m_i = pval[i] * n0Prime;

	/* montProduct <--- m_i * modulus + montProduct */
	CMP_VectorMultiply (m_i, modulus, 0, mLength, montProduct, i);
    }

    for (; i < mLength; ++i) {
	/* m_i  <---  montProduct[i] * n0Prime mod w
	   The following multiplication assumes the result is the low 32
	   bits of the true product. If some platform/compiler does not
	   return the low 32 bits of the true product, use the macro
	   CMP_MULT_LOW (pval[i], n0Prime, m_i) */
	m_i = pval[i] * n0Prime;

	/* montProduct <--- m_i * modulus + montProduct */
	CMP_VectorMultiply (m_i, modulus, 0, mLength, montProduct, i);
    }
  
    CMP_PINT_LENGTH (montProduct) = mLength2 + 1;

    /* Extract upper half of result */
    CMP_ShiftRightByCMPWords (mLength, montProduct);
    CMP_RecomputeLength (mLength, montProduct); 

    /* Subtract modulus from montProduct if possible
     */
    pToMCompare = CMP_Compare (montProduct, modulus);
    if (pToMCompare >= 0){
	if ((status = CMP_SubtractInPlace (modulus, montProduct)) != 0)
	    return (status);
    }

    return (0);
}
#endif
