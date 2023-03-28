/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* Header file for RSA Data Security's implementation of the CMP library.
 */

#ifndef __cmppriv_h_
#define __cmppriv_h_

#define CMP_INT_VALUE(m)     ((m).value)
#define CMP_PINT_VALUE(m)    ((m)->value)
#define CMP_INT_SPACE(m)     ((m).space)
#define CMP_PINT_SPACE(m)    ((m)->space)
#define CMP_INT_LENGTH(m)    ((m).length)
#define CMP_PINT_LENGTH(m)   ((m)->length)
#define CMP_INT_WORD(m, k)   ((m).value[(k)])
#define CMP_PINT_WORD(m, k)  ((m)->value[(k)])

#define CMP_NULL_VALUE       ((CMPWord *)0)

#define CMP_BITS_PER_BYTE    8
#define CMP_BYTES_PER_WORD   (sizeof (CMPWord))
#define CMP_WORD_SIZE        (sizeof (CMPWord) * CMP_BITS_PER_BYTE)

#define CMP_BITS_TO_LEN(modulusBits)     \
  (((modulusBits) + CMP_BITS_PER_BYTE - 1) / CMP_BITS_PER_BYTE)
#define CMP_HALF_WORD_SIZE   (CMP_WORD_SIZE >> 1)
#define CMP_HALF_WORD_MASK   ((CMPWord)(~0) >> (CMP_WORD_SIZE / 2))
#define CMP_UPPER_WORD_MASK  ((CMPWord)(~0) << (CMP_WORD_SIZE / 2))
#define CMP_WORD_MASK        ((CMPWord)(~0))
#define CMP_MSB_MASK         ((CMPWord)1 << (CMP_WORD_SIZE - 1))
#define CMP_LSB_MASK         (CMPWord)1
#define CMP_SQRT_RADIX       ((CMPWord) 1 << CMP_HALF_WORD_SIZE)
#define CMP_LOW_WORD(x)      ((x) & CMP_HALF_WORD_MASK)
#define CMP_HIGH_WORD(x)     ((x) >> CMP_HALF_WORD_SIZE)

#if !defined(max)
#define max(a,b)       ( (a)>(b) ? (a) : (b) )
#endif
#if !defined(min)
#define min(a,b)       ( (a)<(b) ? (a) : (b) )
#endif

/* Support routines
 */
int CMP_realloc(int wordsNeeded, CMPInt *theInt);
/* Doesn't necessarily preserve old data */
int CMP_reallocNoCopy(int wordsNeeded, CMPInt *theInt);
void CMP_free(CMPInt *theInt);

#endif    /* __cmppriv_h_ */
