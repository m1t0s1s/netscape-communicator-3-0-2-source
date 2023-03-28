/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains conversion routines.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"

/* CMP_OctetStringToCMPInt () copies an octet string vector into a CMPInt,
     changing the representation from most significant byte first to least
     significant CMPWord first.
  */
CMPStatus
CMP_OctetStringToCMPInt(unsigned char *octetString, unsigned int octetStringLen,
			CMPInt *theInt)
{
  int theIntLen, fullWords;
  CMPStatus status;
  register int i, j;
  register CMPWord *wordPtr;
  register unsigned char *bytePtr;

#ifdef CMP_DEBUG
  if (octetString == (unsigned char *)NULL)
    return (CMP_INVALID_ARGUMENT);
  if (octetStringLen <= 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  fullWords = octetStringLen / CMP_BYTES_PER_WORD;
  theIntLen = (octetStringLen + CMP_BYTES_PER_WORD - 1) / CMP_BYTES_PER_WORD;

  if (CMP_PINT_SPACE (theInt) < theIntLen)
    if ((status = CMP_reallocNoCopy (theIntLen + 1, theInt)) != 0)
      return (status);

  CMP_PINT_LENGTH (theInt) = theIntLen;
  wordPtr = CMP_PINT_VALUE (theInt);

  /* Start at the end of the octetString. */
  bytePtr = octetString + octetStringLen - 1;

  /* Place the last bytes of octetString -- the least significant -- at the
       beginning of theInt.
   */
  for (i = 0; i < fullWords; ++i) {
    *wordPtr = (CMPWord)(*bytePtr);
    for (j = 1; j < CMP_BYTES_PER_WORD; ++j)
      *wordPtr |= (CMPWord)(*(bytePtr - j)) << (j * 8);
    bytePtr = bytePtr - CMP_BYTES_PER_WORD;
    wordPtr++;
  }

  /* If the number of input bytes is not a multiple of CMP_BYTES_PER_WORD
       copy the last few bytes
   */
  if ((int)octetStringLen > (i = (fullWords * CMP_BYTES_PER_WORD)) ) {
    *wordPtr = (CMPWord)(*bytePtr);
    for (j = 1; j < ((int)octetStringLen - i); ++j)
      *wordPtr |= (CMPWord)(*(bytePtr - j)) << (j * 8);
  }

  /* Make sure the MSW is not 0, unless the number is 0, in which case,
       the length should be 1.
   */
  while ( (CMP_PINT_WORD (theInt, CMP_PINT_LENGTH (theInt) - 1) == 0) &&
          (CMP_PINT_LENGTH (theInt) > 1) )
    CMP_PINT_LENGTH (theInt)--;

  return (CMP_SUCCESS);
}

/* This subroutine converts a CMPInt to an octet string of fixedLength
     bytes. If the CMPInt is not big enought to fill all the bytes, pad
     with leading zeros.
 */
CMPStatus
CMP_CMPIntToFixedLenOctetStr(CMPInt *theInt, unsigned int fixedLength,
			     unsigned int bufferSize, unsigned int *outputLen,
			     unsigned char *output)
{
  int i;
  unsigned int cmpIntLen;

  /* Check to make sure the buffer size is big enough to hold the value. */
  if (bufferSize < fixedLength)
    return (CMP_OUTPUT_LENGTH);

  /* Find the length of the CMPInt. */
  cmpIntLen = CMP_BITS_TO_LEN (CMP_BitLengthOfCMPInt (theInt));

  /* If the length is too long, return an error. */
  if (cmpIntLen > fixedLength)
    return (CMP_OUTPUT_LENGTH);

  /* If the length is less than the fixedLength, put in the leading zeros.
   */
  if (cmpIntLen < fixedLength) {
    for (i = 0; i < (int)(fixedLength - cmpIntLen); ++i)
      output[i] = 0;

    *outputLen = fixedLength;
    return (CMP_CMPIntToOctetString
            (theInt, bufferSize, &cmpIntLen, &(output[i])));
  }

  /* If the length is the same as the fixedLength, simply convert. */
  return (CMP_CMPIntToOctetString (theInt, bufferSize, outputLen, output));
}

/* CMP_CMPIntToOctetString () copies a CMPInt into an octet string vector,
     changing the representation from least significant CMPWord first to most
     significant octet first.
 */
CMPStatus
CMP_CMPIntToOctetString(CMPInt *theInt, unsigned int octetStringBufferSize,
			unsigned int *octetStringLen,
			unsigned char *octetString)
{
  register int theIntLen;
  register int i, j;
  char firstByte;
  register CMPWord wordTmp;
  register CMPWord *placePtr;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (theInt) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (theInt) <= 0)
    return (CMP_LENGTH);
  if (octetString == (unsigned char *)NULL)
    return (CMP_INVALID_ARGUMENT);
  if (octetStringBufferSize <= 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  /* Start at the end of the CMPInt.
   */
  theIntLen = CMP_PINT_LENGTH (theInt);
  *octetStringLen = (theIntLen * CMP_BYTES_PER_WORD);
  placePtr = CMP_PINT_VALUE (theInt) + (theIntLen - 1);
  wordTmp = *placePtr;

  /* Look at the first byte, if it is zero, ignore it.
   */
  i = CMP_BYTES_PER_WORD - 1;
  firstByte = (char)(wordTmp >> (i * 8));
  while ( (i > 0) && (firstByte == 0) ) {
    (*octetStringLen)--;
    i--;
    firstByte = (char)(wordTmp >> (i * 8));
  }

  /* If the first byte is still 0, the CMPInt may be 0, or there is
       an error.
   */
  if (firstByte == 0) {
    if (*octetStringLen == 1) {
      if (*octetStringLen > octetStringBufferSize)
        return (CMP_INSUFFICIENT_SPACE);
      *octetString = (char)0;
      return (CMP_SUCCESS);
    }
    /* The MSW of theInt is 0, but there are more words, this is an
         error. */
    return (CMP_INVALID_ARGUMENT);
  }

  if (*octetStringLen > octetStringBufferSize)
    return (CMP_INSUFFICIENT_SPACE);

  /* Now place the first word into octetString.
   */
  *(octetString++) = firstByte;
  for (j = i - 1; j >= 0; --j)
    *(octetString++) = (char)((wordTmp >> (j * 8)) & 0xff);

  /* Place the rest of the words into octetString.
   */
  for (i = 1; i < theIntLen; ++i){
    wordTmp = *(placePtr - i);
    for (j = CMP_BYTES_PER_WORD - 1; j >= 0; --j) {
      *(octetString + j) = (char)wordTmp;
      wordTmp >>= 8;
    }
    octetString += CMP_BYTES_PER_WORD;
  }

  return (CMP_SUCCESS);
}

/* CMP_CMPIntToSignedOctetString () copies a CMPInt into an octet string
     vector, changing the representation from least significant CMPWord first
     to most significant octet first.  Makes sure that the high bit is not set.
 */
CMPStatus
CMP_CMPIntToSignedOctetString(CMPInt *theInt,
			      unsigned int octetStringBufferSize,
			      unsigned int *octetStringLen,
			      unsigned char *octetString)
{
  register int theIntLen;
  register int i, j;
  char firstByte;
  register CMPWord wordTmp;
  register CMPWord *placePtr;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (theInt) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (theInt) <= 0)
    return (CMP_LENGTH);
  if (octetString == (unsigned char *)NULL)
    return (CMP_INVALID_ARGUMENT);
  if (octetStringBufferSize <= 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  /* Start at the end of the CMPInt.
   */
  theIntLen = CMP_PINT_LENGTH (theInt);
  *octetStringLen = (theIntLen * CMP_BYTES_PER_WORD);
  placePtr = CMP_PINT_VALUE (theInt) + (theIntLen - 1);
  wordTmp = *placePtr;

  /* Look at the first byte, if it is zero, ignore it.
   */
  i = CMP_BYTES_PER_WORD - 1;
  firstByte = (char)(wordTmp >> (i * 8));
  while ( (i > 0) && (firstByte == 0) ) {
    (*octetStringLen)--;
    i--;
    firstByte = (char)(wordTmp >> (i * 8));
  }

  /* If the high bit of the first byte is set, reserve space for an extra 0x00
   * at the beginning.
   */
  if (firstByte & 0x80) {
      (*octetStringLen)++;
  }

  /* If the first byte is still 0, the CMPInt may be 0, or there is
       an error.
   */
  if (firstByte == 0) {
    if (*octetStringLen == 1) {
      if (*octetStringLen > octetStringBufferSize)
        return (CMP_INSUFFICIENT_SPACE);
      *octetString = (char)0;
      return (CMP_SUCCESS);
    }
    /* The MSW of theInt is 0, but there are more words, this is an
         error. */
    return (CMP_INVALID_ARGUMENT);
  }

  if (*octetStringLen > octetStringBufferSize)
    return (CMP_INSUFFICIENT_SPACE);

  /* If the high bit of the first byte is set, insert an extra 0x00 at the
   * beginning of the octet string.
   */
  if (firstByte & 0x80) {
      *(octetString++) = 0x00;
  }

  /* Now place the first word into octetString.
   */
  *(octetString++) = firstByte;
  for (j = i - 1; j >= 0; --j)
    *(octetString++) = (char)((wordTmp >> (j * 8)) & 0xff);

  /* Place the rest of the words into octetString.
   */
  for (i = 1; i < theIntLen; ++i){
    wordTmp = *(placePtr - i);
    for (j = CMP_BYTES_PER_WORD - 1; j >= 0; --j) {
      *(octetString + j) = (char)wordTmp;
      wordTmp >>= 8;
    }
    octetString += CMP_BYTES_PER_WORD;
  }

  return (CMP_SUCCESS);
}

/* Convert a CMPWord to CMPInt.
 */
CMPStatus
CMP_CMPWordToCMPInt(CMPWord sourceWord, CMPInt *destInt)
{
  CMPStatus status;

  if (CMP_PINT_SPACE (destInt) < 1)
    if ((status = CMP_reallocNoCopy (2, destInt)) != 0)
      return (status);

  CMP_PINT_WORD (destInt, 0) = sourceWord;
  CMP_PINT_LENGTH (destInt) = 1;

  return (CMP_SUCCESS);
}

/* Create a CMPInt 2 ^ exponent.
 */
CMPStatus
CMP_PowerOfTwo(int exponent, CMPInt *theInt)
{
  CMPStatus status;
  register int wordIndex, bitIndex;
  register CMPWord targetBit;

  if (exponent < 0)
    return (CMP_INVALID_ARGUMENT);

  wordIndex = exponent / CMP_WORD_SIZE;
  bitIndex = exponent - (wordIndex * CMP_WORD_SIZE);

  /* Since indexing begins counting at 0 and length begins counting at 1,
       we need at least wordIndex + 1 CMPWords for the number.
   */
  if (CMP_PINT_SPACE (theInt) < (wordIndex + 1))
    if ((status = CMP_reallocNoCopy (wordIndex + 2, theInt)) != 0)
      return (status);
  CMP_PINT_LENGTH (theInt) = wordIndex + 1;

  /* Set all but the most significant word to 0. */
  PORT_Memset((unsigned char *)(CMP_PINT_VALUE (theInt)), 0,
	      wordIndex * CMP_BYTES_PER_WORD);

  /* Now set the most significant word. */
  targetBit  = ((CMPWord) 1) << bitIndex;
  CMP_PINT_WORD (theInt, wordIndex) = targetBit;

  return (CMP_SUCCESS);
}

int
CMP_OctetLengthOfCMPInt(CMPInt *theInt)
{
    int theIntLen, octetStringLen;
    int i;
    CMPWord word;
    char firstByte;

    theIntLen = CMP_PINT_LENGTH(theInt);
    octetStringLen = (theIntLen * CMP_BYTES_PER_WORD);

    /* Look at the first byte, if it is zero, ignore it. */

    word = *(CMP_PINT_VALUE(theInt) + (theIntLen - 1));
    i = CMP_BYTES_PER_WORD - 1;
    firstByte = (char)(word >> (i * 8));
    while((i > 0) && (firstByte == 0)) {
	octetStringLen--;
	i--;
	firstByte = (char)(word >> (i * 8));
    }
    return octetStringLen;
}
