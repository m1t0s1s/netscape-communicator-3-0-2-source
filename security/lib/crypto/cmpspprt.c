/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* Miscellaneous support functions which do not need to be visible by
     users of the CMP API.
*/
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"
#include "cmpspprt.h"

/* Adds two CMPInt's, placing the result into the second argument.
     base = base + increment
*/
int
CMP_AddInPlace(CMPInt *increment, CMPInt *base)
{
  register CMPWord *a1val, *a2val;
  register CMPWord intermediateSum, carry;
  register int wordCount;
  int status;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (increment) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (increment) <= 0)
    return (CMP_LENGTH);
#endif

  /* If base is shorter than increment, create a base as long as increment
       plus one CMPWord for possible overflow.
   */
  if (CMP_PINT_LENGTH (base) < CMP_PINT_LENGTH (increment)) {
    wordCount = CMP_PINT_LENGTH (increment);
    if (CMP_PINT_SPACE (base) < wordCount)
      if ((status = CMP_realloc (wordCount + 1, base)) != 0)
        return (status);

    /* Set all the extra CMPWords to 0. */
    PORT_Memset((unsigned char *)&(CMP_PINT_WORD(base, CMP_PINT_LENGTH (base))),
		0, (unsigned int)((wordCount - CMP_PINT_LENGTH (base)) *
				  sizeof (CMPWord)));

    /* The length of the result will be at least the length of increment. */
    CMP_PINT_LENGTH (base) = wordCount;
  }

  a1val = CMP_PINT_VALUE (base);
  a2val = CMP_PINT_VALUE (increment);

  /* Add all the words of the increment to the low order words of base.
   */
  carry = (CMPWord)0;
  for (wordCount = 0;
       wordCount < CMP_PINT_LENGTH (increment);
       ++wordCount, ++a1val, ++a2val) {
    intermediateSum = *a1val + *a2val;
    *a1val = intermediateSum + carry;
    carry = ( (intermediateSum < *a2val) || (*a1val < intermediateSum) ) ?
      (CMPWord)1 : (CMPWord)0;
  }

  /* If there is no more carry, we're done. */
  if (carry == (CMPWord)0)
    return (0);

  /* There's a carry, wordCount currently = length of increment, a1val
       points to the next CMPWord of base. Keep adding a carry until there
       is no more carry or no more words in base.
   */
  for ( ; wordCount < CMP_PINT_LENGTH (base); ++wordCount, ++a1val) {
    *a1val += carry;
    /* If *a1val now = 0, it was 0xff..f, so we still have a carry. If
         it is not zero, there is no more carry, done.
     */
    carry = (*a1val == 0) ? (CMPWord)1 : (CMPWord)0;
    if (carry == (CMPWord)0)
      break;
  }

  /* If there is no more carry, we're done. */
  if (carry == (CMPWord)0)
    return (0);

  /* There's still a carry but we ran out of CMPWords in base. We need one
       more CMPWord in base, and it is simply 1.
   */
  wordCount = CMP_PINT_LENGTH (base) + 1;
  if (CMP_PINT_SPACE (base) < wordCount)
    if ((status = CMP_realloc (wordCount, base)) != 0)
      return (status);

  CMP_PINT_WORD (base, wordCount - 1) = (CMPWord)1;
  CMP_PINT_LENGTH (base) = wordCount;
  return (0);
}

/* Subtracts two CMPInt's, placing the result into the second argument.
     base = base - decrement
*/
int
CMP_SubtractInPlace(CMPInt *decrement, CMPInt *base)
{
  register CMPWord *dval, *bval;
  register CMPWord tempWord;
  register int i, j, carry;
  int dLen, bLen;

  bval = CMP_PINT_VALUE (base);
  dval = CMP_PINT_VALUE (decrement);
  bLen = CMP_PINT_LENGTH (base);
  dLen = CMP_PINT_LENGTH (decrement);

#ifdef CMP_DEBUG
  if (bval == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (bLen <= 0)
    return (CMP_LENGTH);
  if (dval == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (dLen <= 0)
    return (CMP_LENGTH);
#endif

  if (dLen > bLen)
    return (CMP_NEGATIVE);

  for (j = 0; j < dLen; ++j) {
    tempWord = bval[j];
    bval[j] = bval[j] - dval[j];
    carry = (bval[j] > tempWord) ? 1 : 0;
    if (carry != 0) {
      for (i = j + 1; i < bLen; ++i) {
        tempWord = bval[i];
        bval[i]--;
        carry = (bval[i] > tempWord) ? 1 : 0;
        if (carry == 0)
          break;
      }
    }
#ifdef CMP_DEBUG
    /* This will happen if the most significant word of a was 0 to begin
         with. */
    if (carry != 0)
      return (CMP_INVALID_ARGUMENT);
#endif
  }

  if (carry != 0)
    return (CMP_NEGATIVE);

  /* Make sure the MSW is not 0, unless the number is 0, in which case,
       the length should be 1.
   */
  while ( (CMP_PINT_WORD (base, CMP_PINT_LENGTH (base) - 1) == 0) &&
          (CMP_PINT_LENGTH (base) > 1) )
    CMP_PINT_LENGTH (base)--;

  return (0);
}

/* Shift base left by shiftCount CMPWords,
     base <--- base << (shiftCount * CMP_WORD_SIZE)
     equivalent to base <--- base * (w ^ shiftCount)
     where w = 2 ^ CMP_WORD_SIZE.
     For a representation of most significant word first (reading from
     left to right) shifting k words looks like the following.

     --------------------
     | msw ...  ... lsw |
     --------------------
     ------------------------------
     | msw ...  ... lsw 0 0 0 0 0 |
     ------------------------------
                        | k words |

     For a representation of _least_ significant word first it looks like
     the following.

     --------------------
     | lsw ...  ... msw |
     --------------------
     ------------------------------
     | 0 0 0 0 0 lsw ...  ... msw |
     ------------------------------
     | k words |

 */
int
CMP_ShiftLeftByCMPWords(int shiftCount, CMPInt *base)
{
  CMPInt oldBase;
  int status;
  register int totalLen, bLen;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  if (shiftCount <= 0)
    return (0);

  bLen = CMP_PINT_LENGTH(base);
  totalLen = bLen + shiftCount;

  /* Do we need a bigger base? */
  if (CMP_PINT_SPACE (base) < totalLen) {
    do {
      CMP_Constructor (&oldBase);
      if ((status = CMP_Move (base, &oldBase)) != 0)
        break;

      /* Allocate an extra word in case we need it later. */
      if ((status = CMP_reallocNoCopy (totalLen + 1, base)) != 0)
        break;

      /* Set the first shiftCount words (the least significant words)
           to be 0. */
      PORT_Memset((unsigned char *)(CMP_PINT_VALUE (base)), 0,
		  shiftCount * CMP_BYTES_PER_WORD);

      /* Copy the words from oldBase into base, beginning after the zeros. */
      PORT_Memcpy((unsigned char *)&(CMP_PINT_WORD (base, shiftCount)),
		  (unsigned char *)(CMP_PINT_VALUE (&oldBase)),
		  bLen * CMP_BYTES_PER_WORD);

      /* Make sure the MSW is not 0, unless the number is 0, in which case,
           the length should be 1. */
      while ( (CMP_PINT_WORD (base, totalLen - 1) == 0) && (totalLen > 1) )
        totalLen--;

      CMP_PINT_LENGTH (base) = totalLen;

    } while (0);

    CMP_Destructor (&oldBase);
    return (status);
  }

  /* Enough space, move the words to the proper position. */
  PORT_Memmove((unsigned char *)&(CMP_PINT_WORD (base, shiftCount)),
	       (unsigned char *)(CMP_PINT_VALUE (base)),
	       bLen * CMP_BYTES_PER_WORD);

  /* Set the first shiftCount words (the least significant words) to be 0. */
  PORT_Memset((unsigned char *)(CMP_PINT_VALUE (base)), 0,
	      shiftCount * CMP_BYTES_PER_WORD);

  /* Make sure the MSW is not 0, unless the number is 0, in which case,
       the length should be 1. */
  while ( (CMP_PINT_WORD (base, totalLen - 1) == 0) && (totalLen > 1) )
    totalLen--;

  CMP_PINT_LENGTH (base) = totalLen;

  return (0);
}

/* Shifts base leftShift bits to the left.
     The length of base will be adjusted if needed.
     Returns an error if potential reallocation fails.
*/
int
CMP_ShiftLeftByBits(int leftShift, CMPInt *base)
{
  register int count, rightShift;
  register CMPWord *bval, msw;
  int status;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  if (leftShift <= 0)
    return (0);

  /* If the shift count is large enough, shift by words.
   */
  if (leftShift >= CMP_WORD_SIZE) {
    if ((status = CMP_ShiftLeftByCMPWords
         (leftShift / CMP_WORD_SIZE, base)) != 0)
      return (status);

    leftShift = leftShift % CMP_WORD_SIZE;
    if (leftShift == 0)
      return (0);
  }

  /* Shift by bits.
   */
  count = CMP_PINT_LENGTH (base);
  rightShift = CMP_WORD_SIZE - leftShift;

  /* Shift the most significant word. */
  msw = (CMP_PINT_WORD (base, count - 1)) >> rightShift;

  /* If some of the msw bits shifted into a new word, make sure the
       CMPInt has enough space to expand.
   */
  if (msw != (CMPWord)0) {
    if (CMP_PINT_SPACE (base) < count + 1)
      if ((status = CMP_realloc (count + 2, base)) != 0)
        return (status);

    /* Set the new msw. */
    CMP_PINT_WORD (base, count) = msw;
    CMP_PINT_LENGTH (base)++;
  }

  /* Was the msw the only word?
   */
  bval = CMP_PINT_VALUE (base);
  if (--count == 0) {
    bval[0] = bval[0] << leftShift;
    return (0);
  }

  /* Move down the line, shifting each word left, OR'ing in the bits from
       the previous word that were shifted away when it was shifted left.
   */
  bval[count] = bval[count] << leftShift;
  while (count-- > 0) {
    bval[count + 1] |= bval[count] >> rightShift;
    bval[count]      = bval[count] << leftShift;
  }

  return (0);
}

/* Shift base right by shiftCount words,
     base <--- base >> (shiftCount * CMP_WORD_SIZE)
     Any words shifted off the end are lost.
     For a representation of most significant word first (reading from
     left to right) shifting k words looks like the following.

     --------------------
     | msw ...  ... lsw |
     --------------------
     |        ||        |
     |        | \        \
     |        |   \         \
     v        v    v         v
     ----------    -----------
     | msw ...|    | ... lsw |
     ----------    -----------
     | result |    | k words |
                   | discard |

     For a representation of _least_ significant word first it looks like
     the following.

     --------------------
     | lsw ...  ... msw |
     --------------------
     |         ||        |
     |         | \        \
     |         |   \         \
     v         v    v         v
     -----------    -----------
     | lsw ... |    | ... msw |
     -----------    -----------
     | k words |    | result  |
     | discard |

 */
int
CMP_ShiftRightByCMPWords(int shiftCount, CMPInt *base)
{
#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  /* The new length is the old length minus the number of words
       shifted away. */
  CMP_PINT_LENGTH (base) -= shiftCount;

  /* If the shift does not require shifting everything away ... */
  if (CMP_PINT_LENGTH (base) > 0) {
    /* Move new length number of words beginning at the first word after
         the shifted away words to the beginning. */
    PORT_Memmove((unsigned char *)(CMP_PINT_VALUE (base)),
		 (unsigned char *)&(CMP_PINT_WORD (base, shiftCount)),
		 CMP_PINT_LENGTH (base) * CMP_BYTES_PER_WORD);
    return (0);
  }

  /* The shiftCount is so large, everything shifts away, return 0. */
  CMP_PINT_LENGTH (base) = 1;
  CMP_PINT_WORD (base, 0) = (CMPWord)0;
  return (0);
}

/* Shifts base rightShift bits to the right. Any bits shifted off the end
     are lost forever.
 */
int
CMP_ShiftRightByBits(int rightShift, CMPInt *base)
{
  int status;
  register int count, length, leftShift;
  register CMPWord *bval;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  if (rightShift <= 0)
    return (0);

  /* If the shift count is large enough, do the shift by words.
   */
  if (rightShift >= CMP_WORD_SIZE) {
    if ((status = CMP_ShiftRightByCMPWords
         (rightShift / CMP_WORD_SIZE, base)) != 0)
      return (status);

    rightShift = rightShift % CMP_WORD_SIZE;
    if (rightShift == 0)
      return (0);
  }

  /* Shift by bits.
   */
  bval = CMP_PINT_VALUE (base);
  length = CMP_PINT_LENGTH (base) - 1;
  leftShift = CMP_WORD_SIZE - rightShift;

  /* Starting with the least significant word, shift the right bits off,
       then OR in the right bits from the next word up that will be
       shifted away anon.
   */
  for (count = 0; count < length; ++count)
    bval[count] = (bval[count] >> rightShift) | (bval[count + 1] << leftShift);

  /* The most significant word will be padded with zero(s) */
  bval[length] >>= rightShift;

  /* If the msw is 0, the length dropped by one. Unless length is zero,
       in which case, the result is 0. */
  if (bval[length] == 0)
    if (length != 0)
      CMP_PINT_LENGTH (base)--;

  return (0);
}

/* targIndex is the target index for the most significant word. If the
     (targIndex)th word of base is non zero, we met the target. If it is
     zero, check the next word. Keep checking until a non zero word is
     found or there are no more words, whichever comes first. Then, set
     the length of base appropriately.
 */
int
CMP_RecomputeLength(int targIndex, CMPInt *base)
{
  register CMPWord *bval;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_SPACE (base) < targIndex)
    return (CMP_INVALID_ARGUMENT);
#endif

  bval = CMP_PINT_VALUE (base);

  /* Continue checking the words of base until we find one that is non zero
       or we run out of words. */
  while ( (bval[targIndex] == 0) && (targIndex > 0) )
    targIndex--;

  /* Either targIndex is now 0 or is the index to the first non zero word.
       If 0, base is either 0 or a one word CMPInt. Indexing begins counting
       at 0 and length begins at 1, so the length will be targIndex + 1. */
  CMP_PINT_LENGTH (base) = targIndex + 1;

  return (0);
}

/* Gets the offset of the most significant bit of theInt from the top
     Returns error if most significant word is zero
 */
int
CMP_GetOffsetOfMSB(CMPInt *theInt, int *offset)
{
  register int shift = 0;
  register CMPWord mswOfTheInt;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (theInt) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (theInt) <=0)
    return (CMP_LENGTH);
#endif

  mswOfTheInt = CMP_PINT_WORD (theInt, CMP_PINT_LENGTH (theInt) - 1);
  if (mswOfTheInt != (CMPWord)0) {
    while (!(mswOfTheInt & CMP_MSB_MASK)) {
      mswOfTheInt <<= 1;
      shift++;
    }
    *offset = shift;
    return(0);
  }

  /* The most significant word of theInt is 0. */
  *offset = CMP_WORD_SIZE;
  return(0);
}
