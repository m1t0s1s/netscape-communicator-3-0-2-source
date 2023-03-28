/*
 * seccomon.h - common data structures for security libraries
 *
 * This file should have lowest-common-denominator datastructures
 * for security libraries.  It should not be dependent on any other
 * headers, and should not require linking with any libraries.
 *
 * NOTE - Don't change this file without first consulting with Jeff (jsw)
 * or Lisa (repka)!!!
 *
 * $Id: seccomon.h,v 1.3.18.1 1997/04/05 04:10:18 jwz Exp $
 */

#ifndef _SECCOMMON_H_
#define _SECCOMMON_H_

#include "prtypes.h"

/* I'm not sure yet if this is the right thing to do... jsw */
#include "secport.h"

#if defined(XP_CPLUSPLUS)
# define SEC_BEGIN_PROTOS extern "C" {
# define SEC_END_PROTOS }
#else
# define SEC_BEGIN_PROTOS
# define SEC_END_PROTOS
#endif

typedef enum {
    siBuffer,
    siClearDataBuffer,
    siCipherDataBuffer,
    siDERCertBuffer,
    siEncodedCertBuffer,
    siDERNameBuffer,
    siEncodedNameBuffer,
    siAsciiNameString,
    siAsciiString,
    siDEROID
} SECItemType;

typedef struct SECItemStr SECItem;

struct SECItemStr {
    SECItemType type;
    unsigned char *data;
    unsigned int len;
};

/*
** A status code. Status's are used by procedures that return status
** values. Again the motivation is so that a compiler can generate
** warnings when return values are wrong. Correct testing of status codes:
**
**	SECStatus rv;
**	rv = some_function (some_argument);
**	if (rv != SECSuccess)
**		do_an_error_thing();
**
*/
typedef enum _SECStatus {
    SECWouldBlock = -2,
    SECFailure = -1,
    SECSuccess = 0
} SECStatus;

/*
** A comparison code. Used for procedures that return comparision
** values. Again the motivation is so that a compiler can generate
** warnings when return values are wrong.
*/
typedef enum _SECComparison {
    SECLessThan = -1,
    SECEqual = 0,
    SECGreaterThan = 1
} SECComparison;

#endif /* _SECCOMMON_H_ */
