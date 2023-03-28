/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains routines that manipulate bits.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

/* This routine determines how long, in bits, a CMPInt is.
 */
int
CMP_BitLengthOfCMPInt(CMPInt *theInt)
{
  register int bitIndex, mswIndex;
  register CMPWord temp;

#ifdef CMP_DEBUG
  if (CMP_PINT_LENGTH (theInt) <=0)
    return (CMP_LENGTH);
#endif

  if (CMP_PINT_VALUE (theInt) != CMP_NULL_VALUE) {
    bitIndex = 1;
    mswIndex = CMP_PINT_LENGTH (theInt) - 1;
    temp = CMP_PINT_WORD (theInt, mswIndex) >> 1;  
    while (temp > 0) {
      bitIndex++;
      temp >>= 1;
    }
    return ((mswIndex * CMP_WORD_SIZE) + bitIndex);
  }
  /* If theInt is a constructed CMPInt, but has nothing in it,
       return 0. */
  return (0);
}

/* Gets the bitPosition-th bit of theInt, setting the return variable
     theBit to 1 or 0, depending on whether the bitPosition-th bit is
     set or not.
 */
CMPStatus
CMP_GetBit(int bitPosition, CMPInt *theInt, int *theBit)
{
  int wordIndex, bitIndex;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (theInt) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (theInt) <= 0)
    return (CMP_LENGTH);
  if (bitPosition < 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  wordIndex = bitPosition / CMP_WORD_SIZE;
  bitIndex  = bitPosition % CMP_WORD_SIZE;

  if (wordIndex < CMP_PINT_LENGTH (theInt)) {
    *theBit = (int)((CMP_PINT_WORD (theInt, wordIndex) >> bitIndex) & 1);
    return(CMP_SUCCESS);
  }

  /* Getting a bit beyond the current most significant word of theInt,
       by definition, such a bit is 0. */
  *theBit = 0;
  return (CMP_SUCCESS);
}

/* Clears (sets to 0) the bitPosition-th bit of theInt.
 */
CMPStatus
CMP_ClearBit(int bitPosition, CMPInt *theInt)
{
  int wordIndex, bitIndex;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (theInt) != CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (theInt) < 0)
    return (CMP_LENGTH);
  if (bitPosition < 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  wordIndex = bitPosition / CMP_WORD_SIZE;
  bitIndex  = bitPosition % CMP_WORD_SIZE;

  /* If the length is not greater than the wordIndex, we're trying to
       clear a bit beyond the ms word.
   */
  if (CMP_PINT_LENGTH (theInt) > wordIndex) {
    CMP_PINT_WORD (theInt, wordIndex) &=
      CMP_WORD_MASK ^ ((CMPWord)1 << bitIndex);
    CMP_RecomputeLength (CMP_PINT_LENGTH (theInt) - 1, theInt);
    return(CMP_SUCCESS);
  }

  /* Clearing a bit beyond the ms word of theInt, such a bit, by
       definition, is already clear. */
  return (CMP_SUCCESS);
}

/* Sets (to 1) the bitPosition-th bit of theInt.
 */
CMPStatus
CMP_SetBit(int bitPosition, CMPInt *theInt)
{
  int wordIndex, bitIndex;
  CMPStatus status;

#ifdef CMP_DEBUG
  if (CMP_PINT_LENGTH (theInt) <= 0)
    return (CMP_LENGTH);
  if (bitPosition < 0)
    return (CMP_INVALID_ARGUMENT);
#endif

  wordIndex = bitPosition / CMP_WORD_SIZE;
  bitIndex  = bitPosition % CMP_WORD_SIZE;

  if (wordIndex < CMP_PINT_LENGTH (theInt)) {
    CMP_PINT_WORD (theInt, wordIndex) |= ((CMPWord)1 << bitIndex);
    return(CMP_SUCCESS);
  }

  /* Setting a bit beyond the current most significant word of theInt.
   */
  if (CMP_PINT_SPACE (theInt) < wordIndex + 1)
    if ((status = CMP_realloc (wordIndex + 2, theInt)) != 0)
      return (status);

  /* Set all the new words to 0. */
  PORT_Memset((unsigned char *)&CMP_PINT_WORD(theInt, CMP_PINT_LENGTH (theInt)),
	      0, (CMP_PINT_SPACE (theInt) - CMP_PINT_LENGTH (theInt)) *
	      CMP_BYTES_PER_WORD);

  /* Set the bit. */
  CMP_PINT_WORD (theInt, wordIndex) = ((CMPWord)1 << bitIndex);
  CMP_RecomputeLength (wordIndex, theInt);

  return (CMP_SUCCESS);
}
