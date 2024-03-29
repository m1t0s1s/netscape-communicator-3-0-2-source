#include "crypto.h"
#include "secoid.h"
#include "secitem.h"
#include "secasn1.h"

extern int SEC_ERROR_INVALID_ALGORITHM;

/*
 * XXX OLD Template.  Once all uses have been switched over to new one,
 * remove this.
 */
DERTemplate SGNDigestInfoTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(SGNDigestInfo) },
    { DER_INLINE,
	  offsetof(SGNDigestInfo,digestAlgorithm),
	  SECAlgorithmIDTemplate, },
    { DER_OCTET_STRING,
	  offsetof(SGNDigestInfo,digest), },
    { 0, }
};

/* XXX See comment below about SGN_DecodeDigestInfo -- keep this static! */
/* XXX Changed from static -- need to change name? */
const SEC_ASN1Template sgn_DigestInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SGNDigestInfo) },
    { SEC_ASN1_INLINE,
	  offsetof(SGNDigestInfo,digestAlgorithm),
	  SECOID_AlgorithmIDTemplate },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(SGNDigestInfo,digest) },
    { 0 }
};

/*
 * XXX Want to have a SGN_DecodeDigestInfo, like:
 *	SGNDigestInfo *SGN_DecodeDigestInfo(SECItem *didata);
 * that creates a pool and allocates from it and decodes didata into
 * the newly allocated DigestInfo structure.  Then fix secvfy.c (it
 * will no longer need an arena itself) to call this and then call
 * DestroyDigestInfo when it is done, then can remove the old template
 * above and keep our new template static and "hidden".
 */

/*
 * XXX It might be nice to combine the following two functions (create
 * and encode).  I think that is all anybody ever wants to do anyway.
 */

SECItem *
SGN_EncodeDigestInfo(PRArenaPool *poolp, SECItem *dest, SGNDigestInfo *diginfo)
{
    return SEC_ASN1EncodeItem (poolp, dest, diginfo, sgn_DigestInfoTemplate);
}

SGNDigestInfo *
SGN_CreateDigestInfo(SECOidTag algorithm, unsigned char *sig, unsigned len)
{
    SGNDigestInfo *di;
    SECStatus rv;
    PRArenaPool *arena;

    switch (algorithm) {
      case SEC_OID_MD2:
      case SEC_OID_MD5:
      case SEC_OID_SHA1:
	break;
      default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return NULL;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	return NULL;
    }

    di = (SGNDigestInfo *) PORT_ArenaZAlloc(arena, sizeof(SGNDigestInfo));
    if (di == NULL) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }

    di->arena = arena;

    rv = SECOID_SetAlgorithmID(arena, &di->digestAlgorithm, algorithm, 0);
    if (rv != SECSuccess) {
	goto loser;
    }

    di->digest.data = (unsigned char *) PORT_ArenaAlloc(arena, len);
    if (di->digest.data == NULL) {
	goto loser;
    }

    di->digest.len = len;
    PORT_Memcpy(di->digest.data, sig, len);
    return di;

  loser:
    SGN_DestroyDigestInfo(di);
    return NULL;
}

SGNDigestInfo *
SGN_DecodeDigestInfo(SECItem *didata)
{
    PRArenaPool *arena;
    SGNDigestInfo *di;
    SECStatus rv = SECFailure;

    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(arena == NULL)
	return NULL;

    di = (SGNDigestInfo *)PORT_ArenaZAlloc(arena, sizeof(SGNDigestInfo));
    if(di != NULL)
    {
	di->arena = arena;
	rv = SEC_ASN1DecodeItem(arena, di, sgn_DigestInfoTemplate, didata);
    }
	
    if((di == NULL) || (rv != SECSuccess))
    {
	PORT_FreeArena(arena, PR_TRUE);
	di = NULL;
    }

    return di;
}

void
SGN_DestroyDigestInfo(SGNDigestInfo *di)
{
    if (di && di->arena) {
	PORT_FreeArena(di->arena, PR_FALSE);
    }

    return;
}

SECStatus 
SGN_CopyDigestInfo(PRArenaPool *poolp, SGNDigestInfo *a, SGNDigestInfo *b)
{
    SECStatus rv;
    void *mark;

    if((poolp == NULL) || (a == NULL) || (b == NULL))
	return SECFailure;

    mark = PORT_ArenaMark(poolp);
    a->arena = poolp;
    rv = SECOID_CopyAlgorithmID(poolp, &a->digestAlgorithm, 
	&b->digestAlgorithm);
    if(rv == SECSuccess)
	rv = SECITEM_CopyItem(poolp, &a->digest, &b->digest);

    if(rv != SECSuccess)
	PORT_ArenaRelease(poolp, mark);
		
    return rv;
}

SECComparison
SGN_CompareDigestInfo(SGNDigestInfo *a, SGNDigestInfo *b)
{
    SECComparison rv;

    /* Check signature algorithm's */
    rv = SECOID_CompareAlgorithmID(&a->digestAlgorithm, &b->digestAlgorithm);
    if (rv) return rv;

    /* Compare signature block length's */
    rv = SECITEM_CompareItem(&a->digest, &b->digest);
    return rv;
}
