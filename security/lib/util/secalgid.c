#include "secoid.h"
#include "secder.h"	/* XXX remove this when remove the DERTemplate */
#include "secasn1.h"
#include "secitem.h"

extern int SEC_ERROR_INVALID_ALGORITHM;

/* XXX Old template; want to expunge it eventually. */
DERTemplate SECAlgorithmIDTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(SECAlgorithmID) },
    { DER_OBJECT_ID,
	  offsetof(SECAlgorithmID,algorithm), },
    { DER_OPTIONAL | DER_ANY,
	  offsetof(SECAlgorithmID,parameters), },
    { 0, }
};

const SEC_ASN1Template SECOID_AlgorithmIDTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECAlgorithmID) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SECAlgorithmID,algorithm), },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY,
	  offsetof(SECAlgorithmID,parameters), },
    { 0, }
};

SECOidTag
SECOID_GetAlgorithmTag(SECAlgorithmID *id)
{
    if (id == NULL || id->algorithm.data == NULL)
	return SEC_OID_UNKNOWN;

    return SECOID_FindOIDTag (&(id->algorithm));
}

SECStatus
SECOID_SetAlgorithmID(PRArenaPool *arena, SECAlgorithmID *id, SECOidTag which,
		   SECItem *params)
{
    unsigned char *oid;
    unsigned oidLen;
    SECOidData *oiddata;
    
    oiddata = SECOID_FindOIDByTag(which);
    if ( !oiddata ) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }

    oid = oiddata->oid.data;
    oidLen = oiddata->oid.len;

    id->algorithm.data = (unsigned char*) PORT_ArenaAlloc(arena, oidLen);
    if (!id->algorithm.data) {
	return SECFailure;
    }
    id->algorithm.len = oidLen;
    PORT_Memcpy(id->algorithm.data, oid, oidLen);
    if (params) {
	if (SECITEM_CopyItem(arena, &id->parameters, params)) {
	    return SECFailure;
	}
    } else {
	id->parameters.data = (unsigned char*) PORT_ArenaAlloc(arena, 2);
	if (!id->parameters.data) {
	    return SECFailure;
	}
	id->parameters.len = 2;
	id->parameters.data[0] = DER_NULL;/* XXX DER_CONTEXT_SPECIFIC? */
	id->parameters.data[1] = 0;
    }
    return SECSuccess;
}

SECStatus
SECOID_CopyAlgorithmID(PRArenaPool *arena, SECAlgorithmID *to, SECAlgorithmID *from)
{
    SECStatus rv;

    rv = SECITEM_CopyItem(arena, &to->algorithm, &from->algorithm);
    if (rv) return rv;
    rv = SECITEM_CopyItem(arena, &to->parameters, &from->parameters);
    return rv;
}

void SECOID_DestroyAlgorithmID(SECAlgorithmID *algid, PRBool freeit)
{
    SECITEM_FreeItem(&algid->parameters, PR_FALSE);
    SECITEM_FreeItem(&algid->algorithm, PR_FALSE);
    if(freeit == PR_TRUE)
        PORT_Free(algid);
}

SECComparison
SECOID_CompareAlgorithmID(SECAlgorithmID *a, SECAlgorithmID *b)
{
    SECComparison rv;

    rv = SECITEM_CompareItem(&a->algorithm, &b->algorithm);
    if (rv) return rv;
    rv = SECITEM_CompareItem(&a->parameters, &b->parameters);
    return rv;
}
