#include "alghmac.h"
#include "sechash.h"
#include "secoid.h"
#include "secport.h"

#define HMAC_PAD_SIZE 64

struct HMACContextStr {
    void *hash;
    SECHashObject *hashobj;
    unsigned char ipad[HMAC_PAD_SIZE];
    unsigned char opad[HMAC_PAD_SIZE];
};

void
HMAC_Destroy(HMACContext *cx)
{
    if (cx == NULL)
	return;

    if (cx->hash != NULL)
	cx->hashobj->destroy(cx->hash, PR_TRUE);
    PORT_ZFree(cx, sizeof(HMACContext));
}

HMACContext *
HMAC_Create(SECOidTag hash_alg, unsigned char *secret, unsigned int secret_len)
{
    HMACContext *cx;
    int i;

    cx = PORT_ZAlloc(sizeof(HMACContext));
    if (cx == NULL)
	return NULL;

    switch (hash_alg) {
      case SEC_OID_MD5:
	cx->hashobj = &SECRawHashObjects[HASH_AlgMD5];
	break;
      case SEC_OID_MD2:
	cx->hashobj = &SECRawHashObjects[HASH_AlgMD2];
	break;
      case SEC_OID_SHA1:
	cx->hashobj = &SECRawHashObjects[HASH_AlgSHA1];
	break;
      default:
	goto loser;
    }

    cx->hash = cx->hashobj->create();
    if (cx->hash == NULL)
	goto loser;

    if (secret_len > HMAC_PAD_SIZE) /* XXX We don't handle large secrets. */
	goto loser;

    PORT_Memset(cx->ipad, 0x36, sizeof(cx->ipad));
    PORT_Memset(cx->opad, 0x5c, sizeof(cx->opad));

    /* fold secret into padding */
    for (i = 0; i < secret_len; i++) {
	cx->ipad[i] ^= secret[i];
	cx->opad[i] ^= secret[i];
    }
    return cx;

loser:
    HMAC_Destroy(cx);
    return NULL;
}

void
HMAC_Begin(HMACContext *cx)
{
    /* start inner hash */
    cx->hashobj->begin(cx->hash);
    cx->hashobj->update(cx->hash, cx->ipad, sizeof(cx->ipad));
}

void
HMAC_Update(HMACContext *cx, unsigned char *data, unsigned int data_len)
{
    cx->hashobj->update(cx->hash, data, data_len);
}

SECStatus
HMAC_Finish(HMACContext *cx, unsigned char *result, unsigned int *result_len,
	    unsigned int max_result_len)
{
    if (max_result_len < cx->hashobj->length)
	return SECFailure;

    cx->hashobj->end(cx->hash, result, result_len, max_result_len);
    if (*result_len != cx->hashobj->length)
	return SECFailure;

    cx->hashobj->begin(cx->hash);
    cx->hashobj->update(cx->hash, cx->opad, sizeof(cx->opad));
    cx->hashobj->update(cx->hash, result, *result_len);
    cx->hashobj->end(cx->hash, result, result_len, max_result_len);
    return SECSuccess;
}

HMACContext *
HMAC_Clone(HMACContext *cx)
{
    HMACContext *newcx;

    newcx = PORT_ZAlloc(sizeof(HMACContext));
    if (newcx == NULL)
	goto loser;

    newcx->hashobj = cx->hashobj;
    newcx->hash = cx->hashobj->clone(cx->hash);
    if (newcx->hash == NULL)
	goto loser;
    PORT_Memcpy(newcx->ipad, cx->ipad, sizeof(cx->ipad));
    PORT_Memcpy(newcx->opad, cx->opad, sizeof(cx->opad));
    return newcx;

loser:
    HMAC_Destroy(newcx);
    return NULL;
}
