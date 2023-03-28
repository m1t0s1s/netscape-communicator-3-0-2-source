#include "xp_mcom.h"
#include "sec.h"
#include "rsabsafe.h"

#define FAILURE abort()

B_ALGORITHM_METHOD *chooser[] = {
    &AM_RC4_ENCRYPT, &AM_RC4_DECRYPT,
    &AM_MD2_RANDOM,
    &AM_MD5_RANDOM,
    0,
};

int main(int argc, char **argv)
{
    B_ALGORITHM_OBJ alg = 0;
    B_KEY_OBJ key = 0;
    ITEM it;
    unsigned char rc4key[256], *buffer;
    unsigned int bufferLen, partOut, kl;
    int rv;
    RNGContext *rng;

    kl = atoi(argv[1]);
    if (kl < 8) kl = 8;
    if (kl > 256*8) kl = 256*8;
    kl = (kl + 7) >> 3;

    bufferLen = atoi(argv[2]);
    if (bufferLen == 0) bufferLen = 1000000;
    buffer = (unsigned char*) PORT_Alloc(bufferLen);
    if (!buffer) {
	printf("Can't alloc %d bytes of memory\n", bufferLen);
	return -1;
    }

    printf("key length is %d bytes\n", kl);
    rng = SEC_NewRandom(0, 0);
    if (!rng) FAILURE;
    rv = RNG_GenerateRandomBytes(rng, rc4key, kl);
    if (rv) FAILURE;
    rv = RNG_GenerateRandomBytes(rng, buffer, bufferLen);
    if (rv) FAILURE;

    rv = B_CreateAlgorithmObject(&alg);
    if (rv) FAILURE;
    rv = B_SetAlgorithmInfo(alg, AI_RC4, 0);
    if (rv) FAILURE;

    it.data = rc4key;
    it.len = kl;
    rv = B_CreateKeyObject(&key);
    if (rv) FAILURE;
    rv = B_SetKeyInfo(key, KI_Item, (POINTER) &it);
    if (rv) FAILURE;

    rv = B_EncryptInit(alg, key, chooser, 0);
    if (rv) FAILURE;
    rv = B_EncryptUpdate(alg, buffer, &partOut, bufferLen,
			 buffer, bufferLen, 0, 0);
    if (rv) FAILURE;
    rv = B_EncryptFinal(alg, buffer+partOut, &partOut, bufferLen-partOut, 0, 0);
    if (rv) FAILURE;

    rv = B_DecryptInit(alg, key, chooser, 0);
    if (rv) FAILURE;
    rv = B_DecryptUpdate(alg, buffer, &partOut, bufferLen,
			 buffer, bufferLen, 0, 0);
    if (rv) FAILURE;
    rv = B_DecryptFinal(alg, buffer+partOut, &partOut, bufferLen-partOut, 0, 0);
    if (rv) FAILURE;

    return 0;
}
