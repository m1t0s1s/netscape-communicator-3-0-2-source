/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contatins routines that perform CMP simple arithmatic,
     copy, compare, add and subtract.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"

/* Copies one CMPInt to another.
     destination <--- source
 */
CMPStatus
CMP_Move(CMPInt *source, CMPInt *destination)
{
  register int sourceLen, status;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (source) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (source) <= 0)
    return (CMP_LENGTH);
#endif

  /* Check to see if destination is has enough space to hold the value.
   */
  sourceLen = CMP_PINT_LENGTH (source);
  if (CMP_PINT_SPACE (destination) < sourceLen)
    if ((status = CMP_reallocNoCopy (sourceLen, destination)) != 0)
      return (status);

  /* Copy with PORT_Memcpy. */
  PORT_Memcpy((unsigned char *)(CMP_PINT_VALUE (destination)),
	      (unsigned char *)(CMP_PINT_VALUE (source)),
	      sourceLen * CMP_BYTES_PER_WORD);
  CMP_PINT_LENGTH (destination) = sourceLen;

  return (0);
}

/* Compares two CMPInt's. The return value is
       -1     firstInt < secondInt
       0      firstInt == secondInt
       1      firstInt > secondInt
     CMP_Compare (a, b) < 0
      means           a < b
     CMP_Compare (a, b) == 0
      means           a == b
     CMP_Compare (a, b) > 0
      means           a > b
 */
int
CMP_Compare(CMPInt *firstInt, CMPInt *secondInt)
{
  register int index, firstLen, secondLen;
  register CMPWord *firstValue, *secondValue;

  firstLen = CMP_PINT_LENGTH (firstInt);
  secondLen = CMP_PINT_LENGTH (secondInt);
  firstValue = CMP_PINT_VALUE (firstInt);
  secondValue = CMP_PINT_VALUE (secondInt);

#ifdef CMP_DEBUG
  if ( (firstValue == CMP_NULL_VALUE) ||
       (secondValue == CMP_NULL_VALUE) )
    return (CMP_INVALID_ADDRESS);
  if ( (firstLen <= 0) || (secondLen <= 0) )
    return (CMP_LENGTH);
#endif

  /* If the lengths are different, the one with the longer length is
       greater. If they are the same, compare word by word. XP_MEMCMP
       will not produce an equivalent answer.
   */
  if (firstLen == secondLen) {
    for (index = firstLen - 1;
         (index >= 0) && (firstValue[index] == secondValue[index]);
         --index) {}

    /* Did the loop finish because index went beyond 0 or because the
         values were unequal?
     */
    if (index < 0)
      return (0);
    if (firstValue[index] > secondValue[index])
      return (1);
    return (-1);
  }

  /* This is the code for when the lengths are different.
   */
  if (firstLen > secondLen) return (1);
  return (-1);
}

/* Adds two CMPInt's.
     sum = addend1 + addend2
*/
CMPStatus
CMP_Add(CMPInt *addend1, CMPInt *addend2, CMPInt *sum)
{
  register CMPWord *a1val, *a2val, *sval;
  register int wordCount, a1Len, a2Len;
  register CMPWord intermediateSum, carry;
  int status;

  /* Set a1Len to be the max length, a2Len to be the min length, and
       a1val and a2val the corresponding value pointers.
   */
  if (CMP_PINT_LENGTH (addend1) >= CMP_PINT_LENGTH (addend2)) {
    a1val = CMP_PINT_VALUE (addend1);
    a1Len = CMP_PINT_LENGTH (addend1);
    a2val = CMP_PINT_VALUE (addend2);
    a2Len = CMP_PINT_LENGTH (addend2);
  }
  else {
    a1val = CMP_PINT_VALUE (addend2);
    a1Len = CMP_PINT_LENGTH (addend2);
    a2val = CMP_PINT_VALUE (addend1);
    a2Len = CMP_PINT_LENGTH (addend1);
  }

#ifdef CMP_DEBUG
  if (a1val == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (a1Len <= 0)
    return (CMP_LENGTH);
  if (a2val == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (a2Len <= 0)
    return (CMP_LENGTH);
#endif

  /* Check to see if sum is large enough to hold the answer. If it is not
       big enough, might as well allocate an extra word in case of
       overflow.
   */
  if (CMP_PINT_SPACE (sum) < a1Len)
    if ((status = CMP_reallocNoCopy (a1Len + 1, sum)) != 0)
      return (status);

  sval = CMP_PINT_VALUE (sum);

  /* Add all the words of the shorter CMPInt to the low order words of
       the longer.
   */
  carry = 0;
  for (wordCount = 0;
       wordCount < a2Len;
       ++wordCount, ++a1val, ++a2val, ++sval) {
    intermediateSum = *a2val + *a1val;
    *sval = intermediateSum + carry;
    carry = ( (intermediateSum < *a2val) || (*sval < intermediateSum) ) ?
            (CMPWord)1 : (CMPWord)0;
  }

  /* Now add the carry (0 or 1) to the remaining words of the longer.
   */
  for (/* wordCount = a2Len */;
       wordCount < a1Len;
       ++wordCount, ++sval, ++a1val) {
    *sval = *a1val + carry;
    carry = (*sval < *a1val) ? (CMPWord)1 : (CMPWord)0;
  }

  CMP_PINT_LENGTH (sum) = wordCount;

  /* If there is no more carry, we're done, wordCount is now equal to a1Len
   */
  if (carry == (CMPWord)0)
    return (CMP_SUCCESS);

  /* If there is a carry, need one more word in sum, wordCount = a1Len
   */
  if (CMP_PINT_SPACE (sum) < ++wordCount)
    if ((status = CMP_realloc (wordCount, sum)) != 0)
      return (status);
  CMP_PINT_LENGTH (sum) = wordCount;
  CMP_PINT_WORD (sum, a1Len) = 1;
  return (CMP_SUCCESS);
}

/* Increments a CMPInt by the amount given in a CMPWord.
     base = base + increment
*/
CMPStatus
CMP_AddCMPWord(CMPWord increment, CMPInt *base)
{
  register int pos, length;
  CMPStatus status;
  register CMPWord *bval;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  /* Add the increment to the first word. */
  bval = CMP_PINT_VALUE (base);
  *bval += increment;

  /* If there was no carry, done. */
  if (*bval >= increment)
    return (CMP_SUCCESS);

  /* Add the carry to the next word. If the result of adding a carry is 0,
       that means we added 1 to 0xffffffff. therefore, there is a carry
       into the word following the one we just added to.
   */
  length = CMP_PINT_LENGTH (base);
  for (pos = 1; pos < length; ++pos) {
    bval[pos]++;
    if (bval[pos] != 0)
      return (CMP_SUCCESS);
  }

  /* The loop ended but we still have a carry. Put it into a new word.
       That means the length will have to increase by one. We may need
       to reallocate.
   */
  length++;
  if ((CMP_PINT_SPACE (base)) < length)
    if ((status = CMP_realloc (length + 1, base)) != 0)
      return (status);

  CMP_PINT_WORD (base, pos) = 1;
  CMP_PINT_LENGTH (base) = length;
  return (CMP_SUCCESS);
}

/*  difference <----  minuend - subtrahend
 */
CMPStatus
CMP_Subtract(CMPInt *minuend, CMPInt *subtrahend, CMPInt *difference)
{
  register CMPWord *mval, *sval, *dval;
  register int i, j, borrow;
  register int sLen, dLen;
  CMPStatus status;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (minuend) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (minuend) <= 0)
    return (CMP_LENGTH);
  if (CMP_PINT_VALUE (subtrahend) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (subtrahend) <= 0)
    return (CMP_LENGTH);
#endif

  sval = CMP_PINT_VALUE (subtrahend);
  sLen = CMP_PINT_LENGTH (subtrahend);
  mval = CMP_PINT_VALUE (minuend);
  dLen = CMP_PINT_LENGTH (minuend);
  if (sLen > dLen)
    return (CMP_NEGATIVE);

  if (CMP_PINT_SPACE (difference) < dLen)
    if ((status = CMP_reallocNoCopy (dLen + 1, difference)) != 0)
      return (status);
  dval = CMP_PINT_VALUE (difference);

  dval[0] = mval[0] - sval[0];
  borrow = (dval[0] > mval[0]) ? 1 : 0;
  for (j = 1; j < sLen; ++j) {
    dval[j] = mval[j] - sval[j];
    if (borrow == 0)
      borrow = (dval[j] > mval[j]) ? 1 : 0;
    else {
      dval[j]--;
      borrow = (dval[j] >= mval[j]) ? 1 : 0;
    }
  }

  /* If the length of minuend is the same as the length of subtrahend,
       we're done.
   */
  if (j == dLen) {
    if (borrow != 0)
      return (CMP_NEGATIVE);

    CMP_PINT_LENGTH (difference) = dLen;
    return (CMP_SUCCESS);
  }

  /* Copy the remaining CMPWords from minuend into the result, subtract
       if there's a borrow.
   */
  if (borrow == 0) {
    PORT_Memcpy((unsigned char *)(dval + j), (unsigned char *)(mval + j),
		(dLen - j) * CMP_BYTES_PER_WORD);
    CMP_PINT_LENGTH (difference) = dLen;
    return (CMP_SUCCESS);
  }

  for (i = j; i < dLen; ++i) {
    dval[i] = mval[i] - (CMPWord)borrow;
    borrow = (dval[i] > mval[i]) ? 1 : 0;
  }
  if (borrow != 0)
    return (CMP_NEGATIVE);

  /* Make sure the MSW is not 0, unless the number is 0, in which case,
       the length should be 1.
   */
  while ( (CMP_PINT_WORD (difference, dLen - 1) == 0) && (dLen > 1) )
    dLen--;

  CMP_PINT_LENGTH (difference) = dLen;
  return (CMP_SUCCESS);
}

/* Decrements one CMPWord from a CMPInt.
 */
CMPStatus
CMP_SubtractCMPWord(CMPWord decrement, CMPInt *base)
{
  register int pos, mswIndex;
  register CMPWord *bval;

#ifdef CMP_DEBUG
  if (CMP_PINT_VALUE (base) == CMP_NULL_VALUE)
    return (CMP_INVALID_ADDRESS);
  if (CMP_PINT_LENGTH (base) <= 0)
    return (CMP_LENGTH);
#endif

  /* If the first word of base is greater than or equal to the decrement,
       then subtract and we're done.
   */
  bval = CMP_PINT_VALUE (base);
  if (*bval >= decrement) {
    *bval -= decrement;
    return (CMP_SUCCESS);
  }

  /* If the first word of base is less than decrement, subtract that first
       word, then decrement the borrow from the next word.
   */
  *bval -= decrement;
  mswIndex = CMP_PINT_LENGTH (base) - 1;
  for (pos = 1; pos <= mswIndex; ++pos) {
    bval[pos]--;
    /* If the word had been 0, the result is a word with every bit set
         and we need to decrement the borrow from the next word. Otherwise,
         we're done with the loop. */
    if (bval[pos] != CMP_WORD_MASK)
      break;
  }

  /* Done with the loop. If the current pos is less than mswIndex, we
       had broken out before reaching the most significant word, so
       we're done. */
  if (pos < mswIndex)
    return (CMP_SUCCESS);

  /* If pos > mswIndex, either base is a one CMPWord long CMPInt, so mswIndex
       is 0 and pos was initialized to 1, or we went through the loop,
       subracted the borrow from the most significant word and still needed
       to borrow some more. So either the decrement was greater than the
       base, or the most significant word was 0. */
  if (pos > mswIndex)
    return (CMP_NEGATIVE);

  /* pos equals mswIndex, if bval[pos] is 0, the length of the
       CMPInt went down by one and we're done. If bval[pos] is not 0,
       the length stays the same and we're done.
   */
  if (bval[pos] == 0)
    (CMP_PINT_LENGTH (base))--;
  return (CMP_SUCCESS);
}
