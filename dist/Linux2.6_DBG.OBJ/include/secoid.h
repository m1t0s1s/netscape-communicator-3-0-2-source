#ifndef _SECOID_H_
#define _SECOID_H_
/*
 * secoid.h - public data structures and prototypes for ASN.1 OID functions
 *
 * $Id: secoid.h,v 1.4.2.1 1997/04/15 23:08:39 jwz Exp $
 */

#include "prarena.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secasn1t.h"

extern const SEC_ASN1Template SECOID_AlgorithmIDTemplate[];

SEC_BEGIN_PROTOS

/*
 * OID handling routines
 */
extern SECOidData *SECOID_FindOID(SECItem *oid);
extern SECOidTag SECOID_FindOIDTag(SECItem *oid);
extern SECOidData *SECOID_FindOIDByTag(SECOidTag tagnum);

/****************************************/
/*
** Algorithm id handling operations
*/

/*
** Fill in an algorithm-ID object given a tag and some parameters.
** 	"aid" where the DER encoded algorithm info is stored (memory
**	   is allocated)
**	"tag" the tag defining the algorithm (SEC_OID_*)
**	"params" if not NULL, the parameters to go with the algorithm
*/
extern SECStatus SECOID_SetAlgorithmID(PRArenaPool *arena, SECAlgorithmID *aid,
				   SECOidTag tag, SECItem *params);

/*
** Copy the "src" object to "dest". Memory is allocated in "dest" for
** each of the appropriate sub-objects. Memory in "dest" is not freed
** before memory is allocated (use SECOID_DestroyAlgorithmID(dest, PR_FALSE)
** to do that).
*/
extern SECStatus SECOID_CopyAlgorithmID(PRArenaPool *arena, SECAlgorithmID *dest,
				    SECAlgorithmID *src);

/*
** Get the SEC_OID_* tag for the given algorithm-id object.
*/
extern SECOidTag SECOID_GetAlgorithmTag(SECAlgorithmID *aid);

/*
** Destroy an algorithm-id object.
**	"aid" the certificate-request to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SECOID_DestroyAlgorithmID(SECAlgorithmID *aid, PRBool freeit);

/*
** Compare two algorithm-id objects, returning the difference between
** them.
*/
extern SECComparison SECOID_CompareAlgorithmID(SECAlgorithmID *a,
					   SECAlgorithmID *b);

extern PRBool SECOID_KnownCertExtenOID (SECItem *extenOid);

/* Given a SEC_OID_* tag, return a string describing it.
 */
extern const char *SECOID_FindOIDTagDescription(SECOidTag tagnum);


SEC_END_PROTOS

#endif /* _SECOID_H_ */
