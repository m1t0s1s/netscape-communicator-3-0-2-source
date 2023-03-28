/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contatins function prototypes for various support routines.
     Any source code that includes this header file should also include
     cmp.h first.
 */

#ifndef __cmpspprt_h_
#define __cmpspprt_h_

int CMP_GetOffsetOfMSB(CMPInt *theInt, int *offset);
int CMP_ShiftLeftByCMPWords(int shiftCount, CMPInt *base);
int CMP_ShiftRightByCMPWords(int shiftCount, CMPInt *base);
int CMP_ShiftLeftByBits(int leftShift, CMPInt *base);
int CMP_ShiftRightByBits(int rightShift, CMPInt *base);

/* targIndex is the target index for the most significant word. If the
     (targIndex)th word of base is non zero, we met the target. If it is
     zero, check the next word. Keep checking until a non zero word is
     found or there are no more words, whichever comes first. Then, set
     the length of base appropriately.
 */
int CMP_RecomputeLength(int targIndex, CMPInt *base);

/* Operate on two CMPInt's, placing the result into the second argument. */
int CMP_AddInPlace(CMPInt *increment, CMPInt *base);
int CMP_SubtractInPlace(CMPInt *decrement, CMPInt *base);

/* Computes coeffA, coeffB, and gcd such that
 *    (coeffA * u) mod v = gcd(u,v) and
 *    (coeffB * v) mod u = gcd(u,v)
 */
int CMP_ComputeExtendedGCD(CMPInt *u, CMPInt *v, CMPInt *coeffA,
			   CMPInt *coeffB, CMPInt *gcd);

/* Convert a CMPInt to and from Montgomery representation. */
int CMP_ConvertToMont(CMPInt *sourceCMPInt, CMPInt *modulus, CMPInt *montRep);
int CMP_ConvertFromMont(CMPInt *montRep, CMPInt *modulus, CMPWord n0Prime,
			CMPInt *reducedCMP);

int CMP_MontSquare(CMPInt *operand, CMPInt *modulus, CMPWord n0Prime,
		   CMPInt *montSquare);
int CMP_MontProduct(CMPInt *multiplicand, CMPInt *multiplier, CMPInt *modulus,
		    CMPWord n0Prime, CMPInt *montProduct);
/* Compute n0Prime */
int CMP_ComputeMontCoeff(CMPInt *n, CMPWord *n0Prime);

/* vectorB = (scaler * vectorA[indexA ... indexA+lengthA]) +
 *                     vectorB[indexB ... indexB+lengthA]
 */
void CMP_VectorMultiply(CMPWord scaler, CMPInt *vectorA, int indexA,
			int lengthA, CMPInt *vectorB, int indexB);

/* This is used by the Montgomery code.  You don't want to know what it does. */
void CMP_AddInTrace(CMPInt *vectorA, CMPInt *vectorB);

/* Macros for multiplying and squaring two CMPWords, then either keeping
     both the high and low CMPWord result, or simply keeping the low
     CMPWord result.
 */
#if CMP_DEC_ALPHA == 1
#include <c_asm.h>
#define CMP_MULT_LOW(a , b, lowProd) {\
  lowProd = asm ("mulq %a0, %a1, %v0", a, b);\
  }

#define CMP_MULT_HIGHLOW(a , b, lowProd, highProd) {\
  lowProd  = asm ("mulq %a0, %a1, %v0", a, b);\
  highProd = asm ("umulh %a0, %a1, %v0", a, b);\
  }

#define CMP_SQUARE_HIGHLOW(a, lowProd, highProd) {\
  lowProd  = asm ("mulq %a0, %a0, %v0", a);\
  highProd = asm ("umulh %a0, %a0, %v0", a);\
  }

#else   /* CMP_DEC_ALPHA */
#define CMP_MULT_LOW(a , b, lowProd) {\
  CMPWord mixProd;\
  CMPWord lowWordA, lowWordB, highWordA, highWordB;\
  lowWordA  = CMP_LOW_WORD (a);\
  lowWordB  = CMP_LOW_WORD (b);\
  highWordA = CMP_HIGH_WORD (a);\
  highWordB = CMP_HIGH_WORD (b);\
  lowProd   = lowWordA * lowWordB;\
  mixProd   = lowWordA * highWordB + highWordA * lowWordB;\
  mixProd <<= CMP_HALF_WORD_SIZE;\
  lowProd  += mixProd;\
  }

#ifdef __GNUC__
#define CMP_MULT_HIGHLOW(a , b, lowProd, highProd) {\
  unsigned long long foo;\
  foo = ((unsigned long long)a) * ((unsigned long long)b);\
  lowProd = foo;\
  highProd = foo >> 32;\
}
#else
#define CMP_MULT_HIGHLOW(a , b, lowProd, highProd) {\
  CMPWord a1b0, a0b1, mixProd;\
  CMPWord lowWordA, lowWordB, highWordA, highWordB;\
  lowWordA  = CMP_LOW_WORD (a);\
  lowWordB  = CMP_LOW_WORD (b);\
  highWordA = CMP_HIGH_WORD (a);\
  highWordB = CMP_HIGH_WORD (b);\
  lowProd   = lowWordA * lowWordB;\
  highProd  = highWordA * highWordB;\
  a0b1      = lowWordA * highWordB;\
  a1b0      = lowWordB * highWordA;\
  mixProd   = a0b1 + a1b0;\
  if (mixProd < a0b1) highProd += CMP_SQRT_RADIX ;\
  highProd += CMP_HIGH_WORD (mixProd);\
  mixProd <<= CMP_HALF_WORD_SIZE;\
  lowProd  += mixProd;\
  if (lowProd < mixProd) highProd++;\
  }
#endif

#define CMP_SQUARE_HIGHLOW(a, lowProd, highProd) {\
  CMPWord a0a1, mixProd;\
  CMPWord lowHalfA, highHalfA;\
  lowHalfA  = CMP_LOW_WORD (a);\
  highHalfA = CMP_HIGH_WORD (a);\
  lowProd   = lowHalfA * lowHalfA;\
  highProd  = highHalfA * highHalfA;\
  a0a1      = lowHalfA * highHalfA;\
  mixProd   = a0a1 << 1;\
  if (mixProd < a0a1) highProd += CMP_SQRT_RADIX ;\
  highProd += CMP_HIGH_WORD (mixProd);\
  mixProd <<= CMP_HALF_WORD_SIZE;\
  lowProd  += mixProd;\
  if (lowProd < mixProd) highProd++;\
}
#endif    /* CMP_DEC_ALPHA */

#endif    /* __cmpspprt_h_ */
