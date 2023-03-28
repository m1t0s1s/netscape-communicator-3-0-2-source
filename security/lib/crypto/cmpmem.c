/* Copyright (C) RSA Data Security, Inc. created 1995.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* This file contains code that constructs, destructs, mallocs, reallocs
     and frees CMPInt's.
 */
#include "crypto.h"
#include "cmp.h"
#include "cmppriv.h"

/* This routine constructs, or initializes a CMPInt to be 0 space allocated,
     0 length and a NULL for the value.
 */
void
CMP_Constructor(CMPInt *theInt)
{
  CMP_PINT_SPACE (theInt) = 0;
  CMP_PINT_LENGTH (theInt) = 0;
  CMP_PINT_VALUE (theInt) = CMP_NULL_VALUE;
}

/* This routine destructs a CMPInt, zeroing out the value, freeing the
     memory and setting the length to 0.
 */
void
CMP_Destructor(CMPInt *theInt)
{
  if (CMP_PINT_VALUE (theInt) != CMP_NULL_VALUE) {
    PORT_Memset((unsigned char *)CMP_PINT_VALUE (theInt), 0,
		(unsigned int)(CMP_PINT_SPACE (theInt) * sizeof (CMPWord)));
    PORT_Free((unsigned char *)CMP_PINT_VALUE (theInt));
    CMP_PINT_VALUE (theInt) = CMP_NULL_VALUE;
  }
  CMP_PINT_SPACE (theInt) = 0;
  CMP_PINT_LENGTH (theInt) = 0;
}

/* This routine checks to see if enough space is allocated. If not, it
     allocates new space, copying the old into the new.
 */
int
CMP_realloc(int wordsNeeded, CMPInt *theInt)
{
  register unsigned int bytesNeeded;
  register CMPWord *newValue;

  /* If the amount of space already allocated is sufficient, do nothing. */
  if (CMP_PINT_SPACE (theInt) >= wordsNeeded)
    return (0);

  bytesNeeded = (unsigned int)(wordsNeeded * sizeof (CMPWord));

  /* If value is a null pointer, this is a new CMPInt. If PORT_Alloc is
       successful, theInt is set, if not, make sure the space indicates 0
       bytes allocated and return the error code.
   */
  if (CMP_PINT_VALUE (theInt) == CMP_NULL_VALUE) {
    CMP_PINT_SPACE (theInt) = wordsNeeded;
    if ((CMP_PINT_VALUE (theInt) = (CMPWord *)PORT_Alloc(bytesNeeded))
         != (CMPWord *)NULL)
      return (0);
    CMP_PINT_SPACE (theInt) = 0;
    return (CMP_MEMORY);
  }

  /* There is space allocated, but it is not enough, allocate a new buffer,
       copy the old buffer into the new, zero out the old and free it up.
   */
  if ((newValue = (CMPWord *)PORT_Alloc(bytesNeeded)) != (CMPWord *)NULL) {
    PORT_Memcpy((unsigned char *)newValue,
	       (unsigned char *)CMP_PINT_VALUE (theInt),
	       (unsigned int)(CMP_PINT_LENGTH (theInt) * sizeof (CMPWord)));
    PORT_Memset((unsigned char *)CMP_PINT_VALUE (theInt), 0,
		(unsigned int)(CMP_PINT_SPACE (theInt) * sizeof (CMPWord)));
    PORT_Free((unsigned char *)CMP_PINT_VALUE (theInt));
    CMP_PINT_SPACE (theInt) = wordsNeeded;
    CMP_PINT_VALUE (theInt) = newValue;
    return (0);
  }

  /* Could not allocate the space, zero out the old buffer and return
       the error code.
   */
  PORT_Memset((unsigned char *)CMP_PINT_VALUE (theInt), 0,
	      (unsigned int)(CMP_PINT_SPACE (theInt) * sizeof (CMPWord)));
  PORT_Free((unsigned char *)CMP_PINT_VALUE (theInt));
  CMP_PINT_SPACE (theInt) = 0;
  CMP_PINT_LENGTH (theInt) = 0;
  CMP_PINT_VALUE (theInt) = CMP_NULL_VALUE;
  return (CMP_MEMORY);
}

/* This routine checks to see if enough space is allocated. If not, it
     allocates, but does not copy the old into the new.
 */
int
CMP_reallocNoCopy(int wordsNeeded, CMPInt *theInt)
{
  register unsigned int bytesNeeded;
  register CMPWord *newValue;

  /* If the amount of space already allocated is sufficient, do nothing. */
  if (CMP_PINT_SPACE (theInt) >= wordsNeeded)
    return (0);

  bytesNeeded = (unsigned int)(wordsNeeded * sizeof (CMPWord));

  /* If value is a null pointer, this is a new CMPInt. If PORT_Alloc is
       successful, theInt is set, if not, make sure the space indicates 0
       bytes allocated and return the error code.
   */
  if (CMP_PINT_VALUE (theInt) == CMP_NULL_VALUE) {
    CMP_PINT_SPACE (theInt) = wordsNeeded;
    if ((CMP_PINT_VALUE (theInt) = (CMPWord *)PORT_Alloc (bytesNeeded))
         != (CMPWord *)NULL)
      return (0);
    CMP_PINT_SPACE (theInt) = 0;
    return (CMP_MEMORY);
  }

  /* There is space allocated, but it is not enough, allocate a new buffer.
       Zero out the old buffer and free it up.
   */
  if ((newValue = (CMPWord *)PORT_Alloc (bytesNeeded)) != (CMPWord *)NULL) {
    PORT_Memset((unsigned char *)CMP_PINT_VALUE (theInt), 0,
		(unsigned int)(CMP_PINT_SPACE (theInt) * sizeof (CMPWord)));
    PORT_Free((unsigned char *)CMP_PINT_VALUE (theInt));
    CMP_PINT_SPACE (theInt) = wordsNeeded;
    CMP_PINT_LENGTH (theInt) = 0;
    CMP_PINT_VALUE (theInt) = newValue;
    return (0);
  }

  /* Could not allocate the space, zero out the old buffer and return
       the error code.
   */
  PORT_Memset((unsigned char *)CMP_PINT_VALUE (theInt), 0,
	      (unsigned int)(CMP_PINT_SPACE (theInt) * sizeof (CMPWord)));
  PORT_Free((unsigned char *)CMP_PINT_VALUE (theInt));
  CMP_PINT_SPACE (theInt) = 0;
  CMP_PINT_LENGTH (theInt) = 0;
  CMP_PINT_VALUE (theInt) = CMP_NULL_VALUE;
  return (CMP_MEMORY);
}

/* This subroutine frees up memory allocated for a CMPInt and zeros out
     the value.
 */
void
CMP_free(CMPInt *theInt)
{
  if (CMP_PINT_VALUE (theInt) != CMP_NULL_VALUE) {
    PORT_Memset((unsigned char *)CMP_PINT_VALUE (theInt), 0,
		(unsigned int)(CMP_PINT_SPACE (theInt) * sizeof (CMPWord)));
    PORT_Free((unsigned char *)CMP_PINT_VALUE (theInt));
    CMP_PINT_VALUE (theInt) = CMP_NULL_VALUE;
  }
  CMP_PINT_SPACE (theInt) = 0;
  CMP_PINT_LENGTH (theInt) = 0;
}
