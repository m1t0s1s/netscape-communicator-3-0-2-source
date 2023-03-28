#include "crypto.h"
#include "cmp.h"
#include "dsa.h"
#include "secasn1.h"
#include "secitem.h"

typedef struct {
    SECItem r;
    SECItem s;
} DSA_ASN1Signature;

const SEC_ASN1Template DSA_SignatureTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(DSA_ASN1Signature) },
    { SEC_ASN1_INTEGER, offsetof(DSA_ASN1Signature,r) },
    { SEC_ASN1_INTEGER, offsetof(DSA_ASN1Signature,s) },
    { 0, }
};

SECStatus
DSAU_EncodeDerSig(SECItem *dest, SECItem *src)
{
    CMPInt r, s;
    CMPStatus rv;
    DSA_ASN1Signature sig;
    SECStatus status;
    SECItem *item;

    PORT_Memset(&sig, 0, sizeof(sig));
    CMP_Constructor(&r);
    CMP_Constructor(&s);

    rv = CMP_OctetStringToCMPInt(src->data, DSA_SUBPRIME_LEN, &r);
    if (rv != CMP_SUCCESS)
	goto loser;
    rv = CMP_OctetStringToCMPInt(src->data + DSA_SUBPRIME_LEN,
				 DSA_SUBPRIME_LEN, &s);
    if (rv != CMP_SUCCESS)
	goto loser;

    sig.r.data = PORT_Alloc(DSA_SUBPRIME_LEN + 1);
    if (sig.r.data == NULL)
	goto loser;
    sig.s.data = PORT_Alloc(DSA_SUBPRIME_LEN + 1);
    if (sig.s.data == NULL)
	goto loser;

    rv = CMP_CMPIntToSignedOctetString(&r, DSA_SUBPRIME_LEN + 1, &sig.r.len,
				       sig.r.data);
    if (rv != CMP_SUCCESS)
	goto loser;
    rv = CMP_CMPIntToSignedOctetString(&s, DSA_SUBPRIME_LEN + 1, &sig.s.len,
				       sig.s.data);
    if (rv != CMP_SUCCESS)
	goto loser;

    item = SEC_ASN1EncodeItem(NULL, dest, &sig, DSA_SignatureTemplate);
    if (item != NULL)
	goto loser;

done:
    if (sig.r.data != NULL)
	PORT_Free(sig.r.data);
    if (sig.s.data != NULL)
	PORT_Free(sig.s.data);

    CMP_Destructor(&s);
    CMP_Destructor(&r);

    return status;

loser:
    status = SECFailure;
    goto done;
}

SECItem *
DSAU_DecodeDerSig(SECItem *item)
{
    CMPInt r, s;
    CMPStatus rv;
    DSA_ASN1Signature sig;
    SECStatus status;
    SECItem *result = NULL;
    unsigned int len;

    CMP_Constructor(&r);
    CMP_Constructor(&s);

    PORT_Memset(&sig, 0, sizeof(sig));

    result = PORT_ZAlloc(sizeof(SECItem));
    if (result == NULL)
	goto loser;
    result->len = 2 * DSA_SUBPRIME_LEN;
    result->data = PORT_Alloc(2 * DSA_SUBPRIME_LEN);
    if (result->data == NULL)
	goto loser;

    status = SEC_ASN1DecodeItem(NULL, &sig, DSA_SignatureTemplate, item);
    if (status != SECSuccess)
	goto loser;

    rv = CMP_OctetStringToCMPInt(sig.r.data, sig.r.len, &r);
    if (rv != CMP_SUCCESS)
	goto loser;
    rv = CMP_OctetStringToCMPInt(sig.s.data, sig.s.len, &s);
    if (rv != CMP_SUCCESS)
	goto loser;

    rv = CMP_CMPIntToFixedLenOctetStr(&r, DSA_SUBPRIME_LEN, DSA_SUBPRIME_LEN,
				      &len, result->data);
    if (rv != CMP_SUCCESS)
	goto loser;
    rv = CMP_CMPIntToFixedLenOctetStr(&s, DSA_SUBPRIME_LEN, DSA_SUBPRIME_LEN,
				      &len, result->data + DSA_SUBPRIME_LEN);
    if (rv != CMP_SUCCESS)
	goto loser;
    goto done;

loser:
    if (result != NULL) {
	if (result->data != NULL) {
	    PORT_Free(result->data);
	}
	PORT_Free(result);
	result = NULL;
    }
done:

    if (sig.r.data != NULL)
	PORT_Free(sig.r.data);
    if (sig.s.data != NULL)
	PORT_Free(sig.s.data);

    CMP_Destructor(&s);
    CMP_Destructor(&r);

    return result;
}

SECStatus
DSAU_VerifyDerSignature(unsigned char *buf, int len, SECKEYPublicKey *key,
			SECItem *sig, void *wincx)
{
    SECItem *dsasig;
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == dsaKey);

    dsasig = DSAU_DecodeDerSig(sig);
    if (dsasig == NULL)
	return SECFailure;

    rv = VFY_VerifyData(buf, len, key, sig, wincx);

    SECITEM_FreeItem(dsasig, PR_TRUE);
    return rv;
}

SECStatus
DSAU_DerSignData(SECItem *res, unsigned char *buf, int len,
		 SECKEYPrivateKey *pk, SECOidTag algid)
{
    SECItem sig;
    SECStatus rv;

    rv = SEC_SignData(&sig, buf, len, pk, algid);
    if (rv != SECSuccess)
	goto loser;

    rv = DSAU_EncodeDerSig(res, &sig);
    if (rv != SECSuccess)
	goto loser;

loser:
    SECITEM_FreeItem(&sig, PR_FALSE);
    return rv;
}
