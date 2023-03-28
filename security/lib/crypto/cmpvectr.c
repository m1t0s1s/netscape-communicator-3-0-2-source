/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains routines that perform vector multiplication.
 */
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

#ifndef __VECTOR_ASSEMBLY
/* vectorB = (scaler * vectorA) + vectorB
     Begin vectorA at the indexA'th CMPWord and continue for lengthA CMPWords.
     Begin vectorB at the indexB'th CMPWord.
 */
void
CMP_VectorMultiply(CMPWord scaler, CMPInt *vectorA, int indexA, int lengthA,
		   CMPInt *vectorB, int indexB)
{
    register CMPWord carry, *valueA, *valueB;
    register CMPWord lowProd1, lowProd2, highProd1, highProd2;

    valueA = &(CMP_PINT_WORD (vectorA, indexA));
    valueB = &(CMP_PINT_WORD (vectorB, indexB));

    /* Initialize carry to 0. */
    carry = (CMPWord)0;

    if ((--lengthA) & 1) {
	CMP_MULT_HIGHLOW(scaler, *valueA, lowProd1, highProd1);
	valueA++;
	lowProd1 += *valueB;
	highProd1 += (lowProd1 < *valueB);
	lowProd1 += carry;
	highProd1 += (lowProd1 < carry);
	carry = highProd1;
	    
	*valueB = lowProd1;
	valueB++;
	lengthA--;
    }
    CMP_MULT_HIGHLOW(scaler, *valueA, lowProd1, highProd1);
    valueA++;
    while(lengthA) {
	CMP_MULT_HIGHLOW(scaler, *valueA, lowProd2, highProd2);
	valueA++;
	/* Inner product:
	   (highProd, lowProd) =
	   scaler * valueA[loopCount] + valueB[loopCount] + carry
	   Then set valueB[loopCount] to lowProd and carry (for the next
	   iteration) to highProd.
	   */
	lowProd1 += *valueB;
	highProd1 += (lowProd1 < *valueB);
	lowProd1 += carry;
	highProd1 += (lowProd1 < carry);
	carry = highProd1;
	    
	*valueB = lowProd1;
	valueB++;

	CMP_MULT_HIGHLOW(scaler, *valueA, lowProd1, highProd1);
	valueA++;
	/* Inner product:
	       (highProd, lowProd) =
	       scaler * valueA[loopCount] + valueB[loopCount] + carry
	       Then set valueB[loopCount] to lowProd and carry (for the next
	       iteration) to highProd.
	       */
	lowProd2 += *valueB;
	highProd2 += (lowProd2 < *valueB);
	lowProd2 += carry;
	highProd2 += (lowProd2 < carry);
	carry = highProd2;
	    
	*valueB = lowProd2;
	valueB++;
	lengthA -= 2;
    }
    lowProd1 += *valueB;
    highProd1 += (lowProd1 < *valueB);
    lowProd1 += carry;
    highProd1 += (lowProd1 < carry);
    carry = highProd1;
	    
    *valueB = lowProd1;
    valueB++;


    /* loopCount now = lengthA. */
    *valueB += carry;

    /* If adding in the final carry leads to a carry itself, then propagate.
     */
    if (*valueB < carry) {
	valueB++;
	while (*valueB == CMP_WORD_MASK) {
	    *valueB = 0;
	    valueB++;
	}
	(*valueB)++;
    }
    return;
}

/* Square each element of vectorA, adding the result to two elements
     of vectorB. This is used in squaring two CMPInt's.
 */
void
CMP_AddInTrace(CMPInt *vectorA, CMPInt *vectorB)
{
    int lenA;
    register CMPWord carry, lowProd, highProd, *valueA, *valueB;

    lenA = CMP_PINT_LENGTH(vectorA);
    valueA = CMP_PINT_VALUE (vectorA);
    valueB = CMP_PINT_VALUE (vectorB);
    carry = (CMPWord)0;

    /* Square one word of vectorA, then add the resulting two words to
       two words from montSquare. This should never overflow.
       */
    while(lenA--) {
	CMP_SQUARE_HIGHLOW(*valueA, lowProd, highProd);
	valueA++;

	lowProd += *valueB;
	if (lowProd < *valueB)
	    highProd++;

	lowProd += carry;
	if (lowProd < carry)
	    highProd++;

	*valueB = lowProd;
	valueB++;
	highProd += *valueB;
	carry = (highProd < *valueB) ? 1 : 0;
	*valueB = highProd;
	valueB++;
    }
    if (carry == 0)
	return;

    /* Propagate carry.
     */
    while (*valueB == CMP_WORD_MASK) {
	*valueB = 0;
	valueB++;
    }
    (*valueB)++;
    return;
}
#endif /* __VECTOR_ASSEMBLY */


