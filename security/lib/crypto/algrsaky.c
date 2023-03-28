#include "crypto.h"
#include "secder.h"
#include "secrng.h"
#include "key.h"
#include "rsa.h"
#include "rsakeygn.h"

extern int SEC_ERROR_INVALID_ARGS;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_NEED_RANDOM;

static SECItem defaultPublicExponent = {
    0,
    (unsigned char *)"\001\000\001",
    3
};

SECKEYLowPrivateKey *
RSA_NewKey(RNGContext *rng, int keySizeInBits, SECItem *pe)
{
    unsigned int randomBlockLen;
    RSAKeyGenContext *cx = NULL;
    SECKEYLowPrivateKey *pk = NULL;
    unsigned char *randomBlock = NULL;
    RSAKeyGenParams params;
    int i;

    if (pe == NULL) pe = &defaultPublicExponent;

    params.publicExponent.data = pe->data;
    params.publicExponent.len = pe->len;
    params.modulusBits = keySizeInBits;
    
    cx = RSA_CreateKeyGenContext(&params);
    if (!cx) return 0;

    randomBlockLen = keySizeInBits / 8 + 4;
    randomBlock = PORT_Alloc(randomBlockLen);
    if (randomBlock == NULL) goto loser;

    for (i = 0; i < RSA_KEYGEN_MAX_TRIES; i++) {
	RNG_GenerateRandomBytes(rng, randomBlock, randomBlockLen);
	pk = RSA_KeyGen(cx, randomBlock);
	if ((pk != NULL) || (PORT_GetError() != SEC_ERROR_NEED_RANDOM))
	    break;
    }
    RSA_DestroyKeyGenContext(cx);

    PORT_Free(randomBlock);
    return pk;

loser:
    if (randomBlock != NULL) PORT_Free(randomBlock);
    return NULL;
}
