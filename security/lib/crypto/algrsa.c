/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
extern int SEC_ERROR_BAD_KEY;
extern int SEC_ERROR_BAD_DATA;

#include "crypto.h"
#include "sechash.h"
#include "secrng.h"
#include "rsa.h"
#include "key.h"

#define RSA_BLOCK_MIN_PAD_LEN		8
#define RSA_BLOCK_FIRST_OCTET		0x00
#define RSA_BLOCK_PRIVATE0_PAD_OCTET	0x00
#define RSA_BLOCK_PRIVATE_PAD_OCTET	0xff
#define RSA_BLOCK_AFTER_PAD_OCTET	0x00

#define OAEP_SALT_LEN		8
#define OAEP_PAD_LEN		8
#define OAEP_PAD_OCTET		0x00

/*
 * Modify data by XORing it with a special hash of salt.
 */
static SECStatus
oaep_xor_with_h1(unsigned char *data, unsigned int datalen,
		 unsigned char *salt, unsigned int saltlen)
{
    SHA1Context *sha1cx;
    unsigned char *dp, *dataend;
    unsigned char end_octet;

    sha1cx = SHA1_NewContext();
    if (sha1cx == NULL) {
	return SECFailure;
    }

    /*
     * Get a hash of salt started; we will use it several times,
     * adding in a different end octet (x00, x01, x02, ...).
     */
    SHA1_Begin (sha1cx);
    SHA1_Update (sha1cx, salt, saltlen);
    end_octet = 0;

    dp = data;
    dataend = data + datalen;

    while (dp < dataend) {
	SHA1Context *sha1cx_h1;
	unsigned int sha1len, sha1off;
	unsigned char sha1[SHA1_LENGTH];

	/*
	 * Create hash of (salt || end_octet)
	 */
	sha1cx_h1 = SHA1_CloneContext (sha1cx);
	SHA1_Update (sha1cx_h1, &end_octet, 1);
	SHA1_End (sha1cx_h1, sha1, &sha1len, sizeof(sha1));
	SHA1_DestroyContext (sha1cx_h1, PR_TRUE);
	PORT_Assert (sha1len == SHA1_LENGTH);

	/*
	 * XOR that hash with the data.
	 * When we have fewer than SHA1_LENGTH octets of data
	 * left to xor, use just the low-order ones of the hash.
	 */
	sha1off = 0;
	if ((dataend - dp) < SHA1_LENGTH)
	    sha1off = SHA1_LENGTH - (dataend - dp);
	while (sha1off < SHA1_LENGTH)
	    *dp++ ^= sha1[sha1off++];

	/*
	 * Bump for next hash chunk.
	 */
	end_octet++;
    }

    return SECSuccess;
}

/*
 * Modify salt by XORing it with a special hash of data.
 */
static SECStatus
oaep_xor_with_h2(unsigned char *salt, unsigned int saltlen,
		 unsigned char *data, unsigned int datalen)
{
    unsigned char sha1[SHA1_LENGTH];
    unsigned char *psalt, *psha1, *saltend;
    SECStatus rv;

    /*
     * Create a hash of data.
     */
    rv = SHA1_HashBuf (sha1, data, datalen);
    if (rv != SECSuccess) {
	return rv;
    }

    /*
     * XOR the low-order octets of that hash with salt.
     */
    PORT_Assert (saltlen <= SHA1_LENGTH);
    saltend = salt + saltlen;
    psalt = salt;
    psha1 = sha1 + SHA1_LENGTH - saltlen;
    while (psalt < saltend) {
	*psalt++ ^= *psha1++;
    }

    return SECSuccess;
}

/*
 * Format one block of data for public/private key encryption using
 * the rules defined in PKCS #1.
 */
unsigned char *
RSA_FormatOneBlock(unsigned modulusLen, RSA_BlockType blockType,
		   SECItem *data)
{
    unsigned char *block;
    unsigned char *bp;
    int padLen;
    RNGContext *rng;
    int i;

    block = (unsigned char *) PORT_Alloc(modulusLen);
    if (block == NULL)
	return NULL;

    bp = block;

    /*
     * All RSA blocks start with two octets:
     *	0x00 || BlockType
     */
    *bp++ = RSA_BLOCK_FIRST_OCTET;
    *bp++ = (unsigned char) blockType;

    switch (blockType) {

      /*
       * Blocks intended for private-key operation.
       */
      case RSA_BlockPrivate0: /* essentially unused */
      case RSA_BlockPrivate:	 /* preferred method */
	/*
	 * 0x00 || BT || Pad || 0x00 || ActualData
	 *   1      1   padLen    1      data->len
	 * Pad is either all 0x00 or all 0xff bytes, depending on blockType.
	 */
	padLen = modulusLen - data->len - 3;
	PORT_Assert (padLen >= RSA_BLOCK_MIN_PAD_LEN);
	PORT_Memset (bp,
		   blockType == RSA_BlockPrivate0
			? RSA_BLOCK_PRIVATE0_PAD_OCTET
			: RSA_BLOCK_PRIVATE_PAD_OCTET,
		   padLen);
	bp += padLen;
	*bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
	PORT_Memcpy (bp, data->data, data->len);
	break;

      /*
       * Blocks intended for public-key operation.
       */
      case RSA_BlockPublic:

	rng = RNG_CreateContext();
	if (rng == NULL) {
	    PORT_Free (block);
	    return NULL;
	}

	/*
	 * 0x00 || BT || Pad || 0x00 || ActualData
	 *   1      1   padLen    1      data->len
	 * Pad is all non-zero random bytes.
	 */
	padLen = modulusLen - data->len - 3;
	PORT_Assert (padLen >= RSA_BLOCK_MIN_PAD_LEN);
	for (i = 0; i < padLen; i++) {
	    /* Pad with non-zero random data. */
	    do {
		RNG_GenerateRandomBytes(rng, bp + i, 1);
	    } while (bp[i] == RSA_BLOCK_AFTER_PAD_OCTET);
	}
	bp += padLen;
	*bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
	PORT_Memcpy (bp, data->data, data->len);

	RNG_DestroyContext(rng, PR_TRUE);
	break;

      /*
       * Blocks intended for public-key operation, using
       * Optimal Asymmetric Encryption Padding (OAEP).
       */
      case RSA_BlockOAEP:
	/*
	 * 0x00 || BT || Modified2(Salt) || Modified1(PaddedData)
	 *   1      1     OAEP_SALT_LEN     OAEP_PAD_LEN + data->len [+ N]
	 *
	 * where:
	 *   PaddedData is "Pad1 || ActualData [|| Pad2]"
	 *   Salt is random data.
	 *   Pad1 is all zeros.
	 *   Pad2, if present, is random data.
	 *   (The "modified" fields are all the same length as the original
	 * unmodified values; they are just xor'd with other values.)
	 *
	 *   Modified1 is an XOR of PaddedData with a special octet
	 * string constructed of iterated hashing of Salt (see below).
	 *   Modified2 is an XOR of Salt with the low-order octets of
	 * the hash of Modified1 (see farther below ;-).
	 *
	 * Whew!
	 */

	rng = RNG_CreateContext();
	if (rng == NULL) {
	    PORT_Free (block);
	    return NULL;
	}

	/*
	 * Salt
	 */
	RNG_GenerateRandomBytes (rng, bp, OAEP_SALT_LEN);
	bp += OAEP_SALT_LEN;

	/*
	 * Pad1
	 */
	PORT_Memset (bp, OAEP_PAD_OCTET, OAEP_PAD_LEN);
	bp += OAEP_PAD_LEN;

	/*
	 * Data
	 */
	PORT_Memcpy (bp, data->data, data->len);
	bp += data->len;

	/*
	 * Pad2
	 */
	if (bp < (block + modulusLen))
	    RNG_GenerateRandomBytes (rng, bp, block - bp + modulusLen);

	RNG_DestroyContext (rng, PR_TRUE);

	/*
	 * Now we have the following:
	 * 0x00 || BT || Salt || PaddedData
	 * (From this point on, "Pad1 || Data [|| Pad2]" is treated
	 * as the one entity PaddedData.)
	 *
	 * We need to turn PaddedData into Modified1.
	 */
	if (oaep_xor_with_h1(block + 2 + OAEP_SALT_LEN,
			     modulusLen - 2 - OAEP_SALT_LEN,
			     block + 2, OAEP_SALT_LEN) != SECSuccess) {
	    PORT_Free (block);
	    return NULL;
	}

	/*
	 * Now we have:
	 * 0x00 || BT || Salt || Modified1(PaddedData)
	 *
	 * The remaining task is to turn Salt into Modified2.
	 */
	if (oaep_xor_with_h2(block + 2, OAEP_SALT_LEN,
			     block + 2 + OAEP_SALT_LEN,
			     modulusLen - 2 - OAEP_SALT_LEN) != SECSuccess) {
	    PORT_Free (block);
	    return NULL;
	}

	break;

      default:
	PORT_Assert (0);
	PORT_Free (block);
	return NULL;
    }

    return block;
}

SECStatus
RSA_FormatBlock(SECItem *result, unsigned modulusLen,
		RSA_BlockType blockType, SECItem *data)
{
    /*
     * XXX For now assume that the data length fits in a single
     * XXX encryption block; the ASSERTs below force this.
     * XXX To fix it, each case will have to loop over chunks whose
     * XXX lengths satisfy the assertions, until all data is handled.
     * XXX (Unless RSA has more to say about how to handle data
     * XXX which does not fit in a single encryption block?)
     * XXX And I do not know what the result is supposed to be,
     * XXX so the interface to this function may need to change
     * XXX to allow for returning multiple blocks, if they are
     * XXX not wanted simply concatenated one after the other.
     */

    switch (blockType) {
      case RSA_BlockPrivate0:
      case RSA_BlockPrivate:
      case RSA_BlockPublic:
	/*
	 * 0x00 || BT || Pad || 0x00 || ActualData
	 *
	 * The "3" below is the first octet + the second octet + the 0x00
	 * octet that always comes just before the ActualData.
	 */
	PORT_Assert (data->len <= (modulusLen - (3 + RSA_BLOCK_MIN_PAD_LEN)));

	result->data = RSA_FormatOneBlock(modulusLen, blockType, data);
	if (result->data == NULL) {
	    result->len = 0;
	    return SECFailure;
	}
	result->len = modulusLen;

	break;

      case RSA_BlockOAEP:
	/*
	 * 0x00 || BT || M1(Salt) || M2(Pad1||ActualData[||Pad2])
	 *
	 * The "2" below is the first octet + the second octet.
	 * (The other fields do not contain the clear values, but are
	 * the same length as the clear values.)
	 */
	PORT_Assert (data->len <= (modulusLen - (2 + OAEP_SALT_LEN
						 + OAEP_PAD_LEN)));

	result->data = RSA_FormatOneBlock(modulusLen, blockType, data);
	if (result->data == NULL) {
	    result->len = 0;
	    return SECFailure;
	}
	result->len = modulusLen;

	break;

      case RSA_BlockRaw:
	/*
	 * Pad || ActualData
	 * Pad is zeros. The application is responsible for recovering
	 * the actual data.
	 */
	result->data = PORT_ZAlloc(modulusLen);
	result->len = modulusLen;
	PORT_Memcpy(result->data+(modulusLen-data->len),data->data,data->len);
	break;

      default:
	PORT_Assert (0);
	result->data = NULL;
	result->len = 0;
	return SECFailure;
    }

    return SECSuccess;
}

/*
 * Takes a formatted block and returns the data part.
 * (This is the inverse of RSA_FormatOneBlock().)
 * In some formats the start of the data is ambiguous;
 * if it is non-zero, expectedLen will disambiguate.
 *
 * NOTE: this routine is not yet used/tested! (XXX please
 * remove this comment once that is no longer the case ;-)
 */
unsigned char *
RSA_DecodeOneBlock(unsigned char *data,
		   unsigned int modulusLen,
		   unsigned int expectedLen,
		   RSA_BlockType *pResultType,
		   unsigned int *pResultLen)
{
    RSA_BlockType blockType;
    unsigned char *dp, *res;
    unsigned int len, padLen;
    int i;

    dp = data;
    if (*dp++ != RSA_BLOCK_FIRST_OCTET) {
	PORT_SetError (SEC_ERROR_BAD_DATA);
	return NULL;
    }

    blockType = *dp++;
    switch (blockType) {
      case RSA_BlockPrivate0:
	if (expectedLen) {
	    padLen = modulusLen - expectedLen - 3;
	    PORT_Assert (padLen >= RSA_BLOCK_MIN_PAD_LEN);
	    for (i = 0; i < padLen; i++) {
		if (*dp++ != RSA_BLOCK_PRIVATE0_PAD_OCTET)
		    break;
	    }
	    if ((i != padLen) || (*dp != RSA_BLOCK_AFTER_PAD_OCTET)) {
		PORT_SetError (SEC_ERROR_BAD_DATA);
		return NULL;
	    }
	    dp++;
	    len = expectedLen;
	} else {
	    for (i = 0; i < modulusLen; i++) {
		if (*dp++ != RSA_BLOCK_PRIVATE0_PAD_OCTET)
		    break;
	    }
	    if (i == modulusLen) {
		PORT_SetError (SEC_ERROR_BAD_DATA);
		return NULL;
	    }
	    if (RSA_BLOCK_PRIVATE0_PAD_OCTET == RSA_BLOCK_AFTER_PAD_OCTET)
		dp--;
	    padLen = dp - data - 2;
	    if ((padLen < RSA_BLOCK_MIN_PAD_LEN)
		|| (*dp != RSA_BLOCK_AFTER_PAD_OCTET)) {
		PORT_SetError (SEC_ERROR_BAD_DATA);
		return NULL;
	    }
	    dp++;
	    len = modulusLen - (dp - data);
	}
	res = (unsigned char *) PORT_Alloc(len);
	if (res == NULL) {
	    return NULL;
	}
	PORT_Memcpy (res, dp, len);
	break;

      case RSA_BlockPrivate:
	for (i = 0; i < modulusLen; i++) {
	    if (*dp++ != RSA_BLOCK_PRIVATE_PAD_OCTET)
		break;
	}
	if ((i == modulusLen) || (*dp != RSA_BLOCK_AFTER_PAD_OCTET)) {
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return NULL;
	}
	padLen = dp - data - 2;
	dp++;
	len = modulusLen - (dp - data);
	if ((padLen < RSA_BLOCK_MIN_PAD_LEN) || (expectedLen
						 && (expectedLen != len))) {
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return NULL;
	}
	res = (unsigned char *) PORT_Alloc(len);
	if (res == NULL) {
	    return NULL;
	}
	PORT_Memcpy (res, dp, len);
	break;

      case RSA_BlockPublic:
	for (i = 0; i < modulusLen; i++) {
	    if (*dp++ == RSA_BLOCK_AFTER_PAD_OCTET)
		break;
	}
	if (i == modulusLen) {
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return NULL;
	}
	padLen = dp - data - 2;
	dp++;
	len = modulusLen - (dp - data);
	if ((padLen < RSA_BLOCK_MIN_PAD_LEN) || (expectedLen
						 && (expectedLen != len))) {
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return NULL;
	}
	res = (unsigned char *) PORT_Alloc(len);
	if (res == NULL) {
	    return NULL;
	}
	PORT_Memcpy (res, dp, len);
	break;

      case RSA_BlockOAEP:
	{
	    unsigned char *salt, *tmp_res;
	    SECStatus rv;

	    len = modulusLen - 2 - OAEP_SALT_LEN;
	    /*
	     * dp points to:
	     *	Modified2(Salt) || Modified1(PaddedData)
	     * To recover Salt we need to XOR it with the low-order hash
	     * of Modified1.
	     */
	    salt = (unsigned char *) PORT_Alloc(OAEP_SALT_LEN);
	    if (salt == NULL) {
		return NULL;
	    }
	    PORT_Memcpy (salt, dp, OAEP_SALT_LEN);
	    dp += OAEP_SALT_LEN;
	    rv = oaep_xor_with_h2 (salt, OAEP_SALT_LEN, dp, len);
	    if (rv != SECSuccess) {
		PORT_Free (salt);
		return NULL;
	    }
	    if (expectedLen) {
		PORT_Assert (expectedLen <= len);
		len = expectedLen;
	    }
	    tmp_res = (unsigned char *) PORT_Alloc(len);
	    if (tmp_res == NULL) {
		PORT_Free (salt);
		return NULL;
	    }
	    PORT_Memcpy (tmp_res, dp, len);
	    rv = oaep_xor_with_h1 (tmp_res, len, salt, OAEP_SALT_LEN);
	    PORT_Free (salt);
	    if (rv != SECSuccess) {
		return NULL;
	    }
	    for (i = 0; i < OAEP_PAD_LEN; i++) {
		if (tmp_res[i] != OAEP_PAD_OCTET) {
		    PORT_SetError (SEC_ERROR_BAD_DATA);
		    PORT_Free (tmp_res);
		    return NULL;
		}
	    }
	    len -= OAEP_PAD_LEN;
	    res = (unsigned char *) PORT_Alloc(len);
	    if (res == NULL) {
		PORT_Free (tmp_res);
		return NULL;
	    }
	    PORT_Memcpy (res, tmp_res + OAEP_PAD_LEN, len);
	    PORT_Free (tmp_res);
	}
	break;

      default:
	PORT_SetError (SEC_ERROR_BAD_DATA);
	return NULL;
    }

    PORT_Assert (res != NULL);
    *pResultLen = len;
    *pResultType = blockType;
    return res;
}

SECStatus
RSA_PrivateKeyOp(SECKEYLowPrivateKey *key, unsigned char *output,
		 unsigned char *input, unsigned int modulus_len)
{
    RSAPrivateContext *rsa;
    SECStatus rv;
    unsigned int out_len;

    /*
     * Create a blinding context if the private key doesn't have one, then 
     * do a blinded RSA operation.
     */
    rsa = RSA_CreatePrivateContext(key);
    if (rsa == NULL) goto loser;

    rv = RSA_PrivateUpdate(rsa, output, &out_len, modulus_len, input,
			  modulus_len);
    if (rv != SECSuccess) goto loser;

    rv = RSA_PrivateEnd(rsa);
    if (rv != SECSuccess) goto loser;

    PORT_Assert(out_len == modulus_len);

done:
    if (rsa != NULL) RSA_DestroyPrivateContext(rsa);
    return rv;
loser:
    rv = SECFailure;
    goto done;
}

SECStatus
RSA_PublicKeyOp(
    SECKEYLowPublicKey *key, unsigned char *output, unsigned char *input,
    unsigned int modulus_len)
{
    SECStatus rv;
    RSAPublicContext *rsa = NULL;
    unsigned int out_len;

    rsa = RSA_CreatePublicContext(key);
    if (rsa == NULL) goto loser;

    rv = RSA_PublicUpdate(rsa, output, &out_len, modulus_len, input,
			  modulus_len);
    if (rv != SECSuccess) goto loser;

    rv = RSA_PublicEnd(rsa);
    if (rv != SECSuccess) goto loser;

    PORT_Assert(out_len == modulus_len);

done:
    if (rsa != NULL) RSA_DestroyPublicContext(rsa);
    return rv;
loser:
    rv = SECFailure;
    goto done;
}

SECStatus
RSA_Sign(
    SECKEYLowPrivateKey *key, unsigned char *output, unsigned int *output_len,
    unsigned int maxOutputLen, unsigned char *input, int input_len)
{
    SECStatus rv = SECSuccess;
    int modulus_len;
    SECItem formatted, unformatted;

    modulus_len = SECKEY_LowPrivateModulusLen(key);
    if (maxOutputLen < modulus_len) return SECFailure;

    unformatted.len = input_len;
    unformatted.data = input;
    formatted.data = NULL;
    rv = RSA_FormatBlock(&formatted, modulus_len, RSA_BlockPrivate,
			 &unformatted);
    if (rv < 0) goto loser;

    rv = RSA_PrivateKeyOp(key, output, formatted.data, modulus_len);
    *output_len = modulus_len;

    goto done;

loser:
    rv = SECFailure;
done:
    if (formatted.data != NULL) PORT_ZFree(formatted.data, modulus_len);
    return rv;
}

SECStatus
RSA_CheckSign(
    SECKEYLowPublicKey *key,
    unsigned char *sign, int sign_len, unsigned char *hash, int hash_len)
{
    SECStatus rv;
    int modulus_len, i;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];

    modulus_len = SECKEY_LowPublicModulusLen(key);
    if (sign_len != modulus_len) return SECFailure;
    if (hash_len > modulus_len - 8) return SECFailure;

    rv = RSA_PublicKeyOp(key, buffer, sign, modulus_len);
    if (rv != SECSuccess)
	return SECFailure;

    /*
     * check the padding that was used
     */
    if (buffer[0] != 0) return SECFailure;
    if (buffer[1] != 1) return SECFailure;
    for (i = 2; i < modulus_len - hash_len - 1; i++) {
	if (buffer[i] == 0) break;
	if (buffer[i] != 0xff) return SECFailure;
    }

    /*
     * make sure we get the same results
     */
    if (PORT_Memcmp(buffer + modulus_len - hash_len, hash, hash_len) != 0)
	return SECFailure;

    return SECSuccess;
}

SECStatus
RSA_CheckSignRecover(SECKEYLowPublicKey *key,unsigned char *data,
    int *data_len, int max_output_len, unsigned char *sign,int sign_len)
{
    SECStatus rv;
    int modulus_len, i;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];

    modulus_len = SECKEY_LowPublicModulusLen(key);
    if (sign_len != modulus_len) return SECFailure;

    rv = RSA_PublicKeyOp(key, buffer, sign, modulus_len);
    *data_len = 0;

    /*
     * check the padding that was used
     */
    if (buffer[0] != 0) return SECFailure;
    if (buffer[1] != 1) return SECFailure;
    for (i = 2; i < modulus_len; i++) {
	if (buffer[i] == 0) {
	    *data_len = modulus_len - i - 1;
	    break;
	}
	if (buffer[i] != 0xff) return SECFailure;
    }
    if (*data_len == 0) return SECFailure;
    if (*data_len > max_output_len) return SECFailure;

    /*
     * make sure we get the same results
     */
    PORT_Memcpy(data,buffer + modulus_len - *data_len, *data_len);

    return SECSuccess;
}

SECStatus
RSA_EncryptBlock(
    SECKEYLowPublicKey *key, unsigned char *output, unsigned int *output_len,
    unsigned int max_output_len, unsigned char *input, int input_len)
{
    SECStatus rv;
    int modulus_len;
    SECItem formatted, unformatted;

    modulus_len = SECKEY_LowPublicModulusLen(key);
    if (max_output_len < modulus_len) return SECFailure;

    unformatted.len = input_len;
    unformatted.data = input;
    formatted.data = NULL;
    rv = RSA_FormatBlock(&formatted, modulus_len, RSA_BlockPublic,
			 &unformatted);
    if (rv < 0) {
	if (formatted.data != NULL) PORT_ZFree(formatted.data, modulus_len);
	return SECFailure;
    }

    rv = RSA_PublicKeyOp(key, output, formatted.data, modulus_len);
    PORT_ZFree(formatted.data, modulus_len);
    if (rv < 0) return SECFailure;

    *output_len = modulus_len;
    return SECSuccess;
}

SECStatus
RSA_DecryptBlock(
    SECKEYLowPrivateKey *key, unsigned char *output, unsigned int *output_len,
    unsigned int max_output_len, unsigned char *input, int input_len)
{
    SECStatus rv;
    int modulus_len, i;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];

    modulus_len = SECKEY_LowPrivateModulusLen(key);

    rv = RSA_PrivateKeyOp(key, buffer, input, modulus_len);
    if (rv < 0) return SECFailure;
    if (buffer[0] != 0) return SECFailure;
    if (buffer[1] != 2) return SECFailure;
    *output_len = 0;
    for (i = 2; i < modulus_len; i++) {
	if (buffer[i] == 0) {
	    *output_len = modulus_len - i - 1;
	    break;
	}
    }
    if (*output_len == 0) return SECFailure;
    if (*output_len > max_output_len) return SECFailure;

    PORT_Memcpy(output, buffer + modulus_len - *output_len, *output_len);

    return SECSuccess;
}

/*
 * added to make pkcs #11 happy
 *   RAW is RSA_X_509
 */
SECStatus
RSA_SignRaw(
    SECKEYLowPrivateKey *key, unsigned char *output, unsigned int *output_len,
    unsigned int maxOutputLen, unsigned char *input, int input_len)
{
    SECStatus rv = SECSuccess;
    int modulus_len;
    SECItem formatted, unformatted;

    modulus_len = SECKEY_LowPrivateModulusLen(key);
    if (maxOutputLen < modulus_len) return SECFailure;

    unformatted.len = input_len;
    unformatted.data = input;
    formatted.data = NULL;
    rv = RSA_FormatBlock(&formatted, modulus_len, RSA_BlockRaw, &unformatted);
    if (rv < 0) goto loser;

    rv = RSA_PrivateKeyOp(key, output, formatted.data, modulus_len);
    *output_len = modulus_len;

    goto done;

loser:
    rv = SECFailure;
done:
    if (formatted.data != NULL) PORT_ZFree(formatted.data, modulus_len);
    return rv;
}

SECStatus
RSA_CheckSignRaw(
    SECKEYLowPublicKey *key,
    unsigned char *sign, int sign_len, unsigned char *hash, int hash_len)
{
    SECStatus rv;
    int modulus_len;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];

    modulus_len = SECKEY_LowPublicModulusLen(key);
    if (sign_len != modulus_len) return SECFailure;
    if (hash_len > modulus_len) return SECFailure;

    rv = RSA_PublicKeyOp(key, buffer, sign, modulus_len);
    if (rv != SECSuccess)
	return SECFailure;

    /*
     * make sure we get the same results
     */
    /* NOTE: should we verify the leading zeros? */
    if (PORT_Memcmp(buffer + (modulus_len-hash_len), hash, hash_len) != 0)
	return SECFailure;

    return SECSuccess;
}

SECStatus
RSA_CheckSignRecoverRaw( SECKEYLowPublicKey *key,unsigned char *data,
    int *data_len, int max_output_len, unsigned char *sign,int sign_len)
{
    SECStatus rv;
    int modulus_len;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];

    modulus_len = SECKEY_LowPublicModulusLen(key);
    if (sign_len != modulus_len) return SECFailure;
    if (max_output_len < modulus_len) return SECFailure;

    rv = RSA_PublicKeyOp(key, buffer, sign, modulus_len);
    if (rv != SECSuccess)
	return SECFailure;

    *data_len = modulus_len;

    /*
     * make sure we get the same results
     */
    PORT_Memcpy(data, buffer, *data_len);

    return SECSuccess;
}


SECStatus
RSA_EncryptRaw(
    SECKEYLowPublicKey *key, unsigned char *output, unsigned int *output_len,
    unsigned int max_output_len, unsigned char *input, int input_len)
{
    SECStatus rv;
    int modulus_len;
    SECItem formatted, unformatted;

    modulus_len = SECKEY_LowPublicModulusLen(key);
    if (max_output_len < modulus_len) return SECFailure;

    unformatted.len = input_len;
    unformatted.data = input;
    formatted.data = NULL;
    rv = RSA_FormatBlock(&formatted, modulus_len, RSA_BlockRaw, &unformatted);
    if (rv < 0) {
	if (formatted.data != NULL) PORT_ZFree(formatted.data, modulus_len);
	return SECFailure;
    }

    rv = RSA_PublicKeyOp(key, output, formatted.data, modulus_len);
    PORT_ZFree(formatted.data, modulus_len);
    if (rv < 0) return SECFailure;

    *output_len = modulus_len;
    return SECSuccess;
}

SECStatus
RSA_DecryptRaw(
    SECKEYLowPrivateKey *key, unsigned char *output, unsigned int *output_len,
    unsigned int max_output_len, unsigned char *input, int input_len)
{
    SECStatus rv;
    int modulus_len;
    unsigned char buffer[MAX_RSA_MODULUS_LEN];

    modulus_len = SECKEY_LowPrivateModulusLen(key);

    rv = RSA_PrivateKeyOp(key, buffer, input, modulus_len);
    *output_len = modulus_len;
    if (*output_len == 0) return SECFailure;
    if (*output_len > max_output_len) return SECFailure;

    PORT_Memcpy(output, buffer, *output_len);

    return SECSuccess;
}
