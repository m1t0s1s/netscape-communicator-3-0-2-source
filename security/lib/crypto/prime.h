/* Copyright (C) RSA Data Security, Inc. created 1994.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef __prime_h_
#define __prime_h_ 1

#include "cmp.h"

SECStatus prm_PrimeFind(unsigned char *randomBlock, unsigned int primeSizeBits,
			CMPInt *expo, CMPInt *primeCMP);
SECStatus prm_GeneratePrimeRoster(CMPInt *initPos, CMPInt *step,
				  unsigned int rosterSize,
				  unsigned char *roster);
SECStatus prm_PseudoPrime(CMPInt *proposedPrime, SECStatus *testResult);
SECStatus prm_RabinTest(CMPInt *proposedPrime, SECStatus *testResult);

#endif /* __prime_h_ */
