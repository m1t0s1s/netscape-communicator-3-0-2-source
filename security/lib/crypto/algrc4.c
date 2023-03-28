#include "crypto.h"

/* Copyright (C) RSA Data Security, Inc. created 1987. This is an
   unpublished work protected as such under copyright law. This work
   contains proprietary, confidential, and trade secret information
   of RSA Data Security, Inc. Use, disclosure or reproduction without
   the express written authorization of RSA Data Security, Inc., is
   prohibited.
 */
extern int SEC_ERROR_INVALID_ARGS;
extern int SEC_ERROR_OUTPUT_LEN;

struct RC4ContextStr {
    int encrypt;
    int initialized;
    unsigned int i;
    unsigned int j;
    unsigned char table[256];
};

RC4Context *
RC4_DupContext(RC4Context *rc4)
{
    RC4Context *ret;
    ret = (RC4Context *)PORT_Alloc(sizeof(RC4Context));
    if ( ret ) {
	*ret = *rc4;
    }
    
    return(ret);
}

RC4Context *
RC4_CreateContext(unsigned char *key, int keyLen)
{
  unsigned char *table, ti;
  unsigned int i, j, k;
  RC4Context *rc4Context;

  if (keyLen < 1 || keyLen > 256) {
      PORT_SetError(SEC_ERROR_INVALID_ARGS);
      return 0;
  }

  rc4Context = (RC4Context *) PORT_Alloc(sizeof(RC4Context));
  if (!rc4Context) {
      return 0;
  }
  table = rc4Context->table;

  /* Initialize table.
   */
  for (i = 0; i < 256; i++)
    table[i] = (unsigned char)i;

  /* Permute table according to key.
   */
  j = 0;
  for (i = 0, k = 0; i < 256; i++, k = (k + 1) % keyLen) {
    ti = table[i];
    j = (j + ti + key[k]) & 255;
    table[i] = table[j];
    table[j] = ti;
  }

  /* Initialize indices.
   */
  rc4Context->i = rc4Context->j = 0;

  rc4Context->initialized = 1;

  return rc4Context;
}

void
RC4_DestroyContext(RC4Context *rc4, PRBool freeit)
{
    if (freeit) {
	PORT_Free(rc4);
    }
}

SECStatus
RC4_Encrypt(RC4Context *rc4Context, unsigned char *output,
	    unsigned int *outputLen, unsigned maxOutputLen,
	    const unsigned char *input, unsigned inputLen)
{
  unsigned char *table, ti, tj;
  unsigned int i, j, k;
  
  if (maxOutputLen < inputLen) {
      PORT_SetError(SEC_ERROR_OUTPUT_LEN);
      return SECFailure;
  }
  
  i = rc4Context->i;
  j = rc4Context->j;
  table = rc4Context->table;
  
  for (k = 0; k < inputLen; k++) {
    i = (i + 1) & 255;
    ti = table[i];
    j = (j + ti) & 255;
    tj = table[j];
    table[i] = tj;
    table[j] = ti;
    output[k] = input[k] ^ table[(ti + tj) & 255];
  }
  
  *outputLen = inputLen;
  
  /* Restore indices.
   */
  rc4Context->i = i;
  rc4Context->j = j;
  
  return SECSuccess;
}

SECStatus
RC4_Decrypt(RC4Context *rc4Context, unsigned char *output,
	    unsigned int *outputLen, unsigned maxOutputLen,
	    const unsigned char *input, unsigned inputLen)
{
    return RC4_Encrypt(rc4Context, output, outputLen, maxOutputLen,
		       input, inputLen);
}
