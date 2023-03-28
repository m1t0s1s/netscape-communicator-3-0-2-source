#ifndef _ALGHMAC_H_
#define _ALGHMAC_H_

#include "secoid.h"

typedef struct HMACContextStr HMACContext;

/* destroy HMAC context */
extern void
HMAC_Destroy(HMACContext *cx);

/* create HMAC context
 *  hash_alg	the algorithm with which the HMAC is performed.  This 
 *		should be, SEC_OID_MD5, SEC_OID_SHA1, or SEC_OID_MD2.
 *  secret	the secret with which the HMAC is performed.
 *  secret_len	the length of the secret, limited to at most 64 bytes.
 *
 * NULL is returned if an error occurs or the secret is > 64 bytes.
 */
extern HMACContext *
HMAC_Create(SECOidTag hash_alg, unsigned char *secret, unsigned int
	    secret_len);

/* reset HMAC for a fresh round */
extern void
HMAC_Begin(HMACContext *cx);

/* update HMAC 
 *  cx		HMAC Context
 *  data	the data to perform HMAC on
 *  data_len	the length of the data to process
 */
extern void 
HMAC_Update(HMACContext *cx, unsigned char *data, unsigned int data_len);

/* Finish HMAC -- place the results within result
 *  cx		HMAC context
 *  result	buffer for resulting hmac'd data
 *  result_len	where the resultant hmac length is stored
 *  max_result_len  maximum possible length that can be stored in result
 */
extern SECStatus
HMAC_Finish(HMACContext *cx, unsigned char *result, unsigned int *result_len,
	    unsigned int max_result_len);

/* clone a copy of the HMAC state.  this is usefult when you would
 * need to keep a running hmac but also need to extract portions 
 * partway through the process.
 */
extern HMACContext *
HMAC_Clone(HMACContext *cx);

#endif
