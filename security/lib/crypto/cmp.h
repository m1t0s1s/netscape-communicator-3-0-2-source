/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* Header file for CMP Library.
 */

#ifndef __cmp_h_
#define __cmp_h_

/* Definition of a CMPWord, the largest integer on the platform. */
typedef unsigned long CMPWord;

/* Error codes
 */
typedef enum CMPStatusEnum {
    CMP_SUCCESS             = 0,
    CMP_MEMORY              = -1,
    CMP_INVALID_ADDRESS     = -2,
    CMP_LENGTH              = -3,
    CMP_INVALID_VALUE       = -4,
    CMP_INVALID_ARGUMENT    = -5,
    CMP_INSUFFICIENT_SPACE  = -6,
    CMP_ZERO_DIVIDE         = -7,
    CMP_MODULUS             = -8,
    CMP_RANGE               = -9,
    CMP_NEGATIVE            = -10,
    CMP_DOMAIN              = -11,
    CMP_INVERSE             = -12,
    CMP_OUTPUT_LENGTH       = -13
} CMPStatus;


/* Definition of a CMPInt.
 */
typedef struct {
  int  space;      /* number of CMPWords currently allocated for the CMPInt */
  int  length;     /* length, in CMPWords, of the value */
  CMPWord *value;  /* value of the CMPInt, least significant CMPWord first */
} CMPInt;

/* Function definitions
 */

/* This routine constructs, or initializes a CMPInt to be 0 space allocated,
     0 length and a NULL for the value.
 */
extern void CMP_Constructor(CMPInt *theInt);

/* This routine destructs a CMPInt, zeroing out the value, freeing the
     memory and setting the length to 0.
 */
extern void CMP_Destructor(CMPInt *theInt);

/* CMP_OctetStringToCMPInt () copies an octet string vector into a CMPInt,
     changing the representation from most significant byte first to least
     significant CMPWord first.

  unsigned char *octetString        The octet string to be converted
  unsigned int octetStringLen       Length of the octet string, in bytes
  CMPInt *theInt                    The CMPInt taking on the value
*/
extern CMPStatus CMP_OctetStringToCMPInt(unsigned char *octetString,
					 unsigned int octetStringLen,
					 CMPInt *theInt);



/* CMP_CMPIntToOctetString () copies a CMPInt into an octet string vector,
     changing the representation from least significant CMPWord first to most
     significant octet first.

  CMPInt *theInt                       CMP integer to be converted
  unsigned int octetStringBufferSize   maximum number of output bytes
  unsigned int *octetStringLen         resulting number of bytes in output
  unsigned char *octetString           buffer for octet string representation
*/
extern CMPStatus CMP_CMPIntToOctetString(CMPInt *theInt,
					 unsigned int octetStringBufferSize,
					 unsigned int *octetStringLen,
					 unsigned char *octetString);


/* CMP_CMPIntToSignedOctetString () copies a CMPInt into an octet string vector,
     changing the representation from least significant CMPWord first to most
     significant octet first.  Makes sure that high bit of result is not set.

  CMPInt *theInt                       CMP integer to be converted
  unsigned int octetStringBufferSize   maximum number of output bytes
  unsigned int *octetStringLen         resulting number of bytes in output
  unsigned char *octetString           buffer for octet string representation
*/
extern CMPStatus CMP_CMPIntToSignedOctetString(CMPInt *theInt,
					       unsigned int octetStringBufferSize,
					       unsigned int *octetStringLen,
					       unsigned char *octetString);


/* This subroutine converts a CMPInt to an octet string of fixedLength
     bytes. If the CMPInt is not big enought to fill all the bytes, pad
     with leading zeros.

  CMPInt *theInt,               CMP integer to be converted
  unsigned int fixedLength,     fixed length size of the resultint octet string
  unsigned int bufferSize,      maximum number of output bytes
  unsigned int *outputLen,      resulting number of bytes in output
  unsigned char *output         buffer for octet string representation
*/
extern CMPStatus CMP_CMPIntToFixedLenOctetStr(CMPInt *theInt,
					      unsigned int fixedLength,
					      unsigned int bufferSize,
					      unsigned int *outputLen,
					      unsigned char *output);



extern CMPStatus CMP_CMPWordToCMPInt(CMPWord sourceWord, CMPInt *destInt);

/* Create a CMPInt 2 ^ exponent.
 */
extern CMPStatus CMP_PowerOfTwo(int exponent, CMPInt *theInt);

extern int CMP_BitLengthOfCMPInt(CMPInt *theInt);
/* This is guaranteed to return the size of a buffer that will hold theInt */
extern int CMP_OctetLengthOfCMPInt(CMPInt *theInt);
extern CMPStatus CMP_GetBit(int bitPosition, CMPInt *theInt, int *theBit);
extern CMPStatus CMP_SetBit(int bitPosition, CMPInt *theInt);
extern CMPStatus CMP_ClearBit(int bitPosition, CMPInt *theInt);

extern CMPStatus CMP_Move(CMPInt *source, CMPInt *destination);

/* Compares two CMPInt's. The return value is
       -1     firstInt < secondInt
       0      firstInt == secondInt
       1      firstInt > secondInt
*/
extern int CMP_Compare(CMPInt *firstInt, CMPInt *secondInt);

extern CMPStatus CMP_AddCMPWord(CMPWord increment, CMPInt *base);
extern CMPStatus CMP_SubtractCMPWord(CMPWord decrement, CMPInt *base);
extern CMPStatus CMP_Add(CMPInt *addend1, CMPInt *addend2, CMPInt *sum);
extern CMPStatus CMP_Subtract(CMPInt *minuend, CMPInt *sutrahend,
			      CMPInt *difference);
extern CMPStatus CMP_Multiply(CMPInt *multiplicand, CMPInt *multiplier,
			      CMPInt *product);
extern CMPStatus CMP_Divide(CMPInt *dividend, CMPInt *divisor, CMPInt *quotient,
			    CMPInt *remainder);


/* operand % modulus
     reducedValue = modulus - [operand * (operand / modulus)]
 */
extern CMPStatus CMP_ModularReduce(CMPInt *operand, CMPInt *modulus,
				   CMPInt *reducedValue);
extern CMPStatus CMP_CMPWordModularReduce(CMPInt *base, CMPWord modulus,
					  CMPWord *result);

extern CMPStatus CMP_ComputeGCD(CMPInt *u, CMPInt *v, CMPInt *gcd);
extern CMPStatus CMP_ModAdd(CMPInt *addend1, CMPInt *addend2, CMPInt *modulus,
			    CMPInt *sum);
extern CMPStatus CMP_ModSubtract(CMPInt *minuend, CMPInt *subtrahend,
				 CMPInt *modulus, CMPInt *difference);
extern CMPStatus CMP_ModMultiply(CMPInt *multiplicand, CMPInt *multiplier,
				 CMPInt *modulus, CMPInt *product);
extern CMPStatus CMP_ModInvert(CMPInt *operand, CMPInt *modulus,
			       CMPInt *inverse);
extern CMPStatus CMP_ModExp(CMPInt *base, CMPInt *exponent, CMPInt *modulus,
			    CMPInt *result);


/* (base ^ exponent) mod modulus, where

     exponent = primeP * primeQ
     baseP = base mod primeP
     baseQ = base mod primeQ
     exponentP = exponent mod (primeP - 1)
     exponentQ = exponent mod (primeQ - 1)
     resultP = (baseP ^ exponentP) mod primeP
     resultQ = (baseQ ^ exponentQ) mod primeQ
     crtCoeff = primeQ ^ (-1) mod primeP  --  inverse of primeQ mod primeP
*/
extern CMPStatus CMP_ModExpCRT(CMPInt *base, CMPInt *primeP, CMPInt *primeQ,
			       CMPInt *exponentP, CMPInt *exponentQ,
			       CMPInt *crtCoeff, CMPInt *result);


#endif    /* __cmp_h_ */
