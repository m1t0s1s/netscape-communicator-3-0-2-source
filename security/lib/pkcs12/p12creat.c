#include "pkcs12.h"
#include "secitem.h"
#include "secport.h"
#include "secder.h"
#include "secoid.h"
#include "p12local.h"

extern int SEC_ERROR_NO_MEMORY;

/* allocate space for a PFX structure and set up initial
 * arena pool.  pfx structure is cleared and a pointer to
 * the new structure is returned.
 */
SEC_PKCS12PFXItem *
sec_pkcs12_new_pfx(void)
{
    SEC_PKCS12PFXItem   *pfx = NULL;
    PRArenaPool     *poolp = NULL;

    poolp = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);	/* XXX Different size? */
    if(poolp == NULL)
	goto loser;

    pfx = (SEC_PKCS12PFXItem *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12PFXItem));
    if(pfx == NULL)
	goto loser;
    pfx->poolp = poolp;

    return pfx;

loser:
    PORT_FreeArena(poolp, PR_TRUE);
    return NULL;
}

/* allocate space for a PFX structure and set up initial
 * arena pool.  pfx structure is cleared and a pointer to
 * the new structure is returned.
 */
SEC_PKCS12AuthenticatedSafe *
sec_pkcs12_new_asafe(PRArenaPool *poolp)
{
    SEC_PKCS12AuthenticatedSafe  *asafe = NULL;
    void *mark;

    mark = PORT_ArenaMark(poolp);
    asafe = (SEC_PKCS12AuthenticatedSafe *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12AuthenticatedSafe));
    if(asafe == NULL)
	goto loser;
    asafe->poolp = poolp;
    PORT_Memset(&asafe->old_baggage, 0, sizeof(SEC_PKCS7ContentInfo));

    return asafe;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}

/* create a new x509 certificate structure */
SEC_PKCS12X509CertCRL *
sec_pkcs12_new_x509_cert_crl(PRArenaPool *poolp)
{
    SEC_PKCS12X509CertCRL *x509 = NULL;
    void *mark;

    mark = PORT_ArenaMark(poolp);
    if(poolp != NULL)
    {
	x509 = (SEC_PKCS12X509CertCRL *)PORT_ArenaZAlloc(poolp, 
	    sizeof(SEC_PKCS12X509CertCRL));
	if(x509 == NULL)
	{
	    PORT_ArenaRelease(poolp, mark);
	    return NULL;
	}
	x509->poolp = poolp;
	return x509;
    }

    return NULL;
}

/* create an sdsi certificate structure */
SEC_PKCS12SDSICert *
sec_pkcs12_new_sdsi_cert(PRArenaPool *poolp)
{
    SEC_PKCS12SDSICert *sdsi;
    void *mark;

    mark = PORT_ArenaMark(poolp);

    if(poolp != NULL)
    {
	sdsi = (SEC_PKCS12SDSICert *)PORT_ArenaZAlloc(poolp, 
	    sizeof(SEC_PKCS12SDSICert));
        if(sdsi == NULL)
        {
	    PORT_ArenaRelease(poolp, mark);
	    return NULL;
	}
	sdsi->poolp = poolp;
	return sdsi;
    }

    return NULL;
}

/* create a new cert/crl of the appropriate type */
SEC_PKCS12CertAndCRL *
sec_pkcs12_new_cert_crl(PRArenaPool *poolp,
			SECOidTag certType)
{
    SEC_PKCS12CertAndCRL *certcrl = NULL;
    SECStatus rv;
    void *mark;
    SEC_PKCS12X509CertCRL *temp_x509;
    SEC_PKCS12SDSICert *temp_sdsi;

    mark = PORT_ArenaMark(poolp);

    if(poolp != NULL)
    {
	int size = sizeof(SEC_PKCS12CertAndCRL) + 10; /* slop due to ? */

	/* allocate the space */
	certcrl = (SEC_PKCS12CertAndCRL *)PORT_ArenaZAlloc(poolp, size);
	if(certcrl == NULL)
	    goto loser;
	certcrl->poolp = poolp;

	/* set object ID */
	certcrl->BagTypeTag = SECOID_FindOIDByTag(certType);
	rv = SECITEM_CopyItem(poolp, &(certcrl->BagID),
		&(certcrl->BagTypeTag->oid));
	if(rv != SECSuccess)
	    goto loser;

	switch(certType)
	{
	    case SEC_OID_PKCS12_X509_CERT_CRL_BAG:
		temp_x509 = sec_pkcs12_new_x509_cert_crl(poolp);
		if(temp_x509 == NULL)
		    goto loser;
		certcrl->value.x509 = temp_x509;
		break;
	    case SEC_OID_PKCS12_SDSI_CERT_BAG:
		temp_sdsi = sec_pkcs12_new_sdsi_cert(poolp);
		if(temp_sdsi == NULL)
		    goto loser;
		certcrl->value.sdsi = temp_sdsi;
		break;
	    default:
		goto loser;
	}
    }

    return certcrl;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}

/* create a safe bag of the appropriate type */
SEC_PKCS12SafeBag *
sec_pkcs12_create_safe_bag(PRArenaPool *poolp,
			   SECOidTag bag_type)
{
    SEC_PKCS12SafeBag *bag = NULL;
    SECStatus rv;
    void *mark;

    mark = PORT_ArenaMark(poolp);

    /* allocate space */
    bag = (SEC_PKCS12SafeBag *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12SafeBag));
    if(bag == NULL)
	goto loser;
    bag->poolp = poolp;
    bag->uniSafeBagName.data = NULL;
    bag->uniSafeBagName.len = 0;

    /* set up object ID */
    bag->safeBagTypeTag = SECOID_FindOIDByTag(bag_type);
    rv = SECITEM_CopyItem(poolp, &(bag->safeBagType), 
	&(bag->safeBagTypeTag->oid));
    if(rv != SECSuccess)
	goto loser;

    /* create the guts */
    switch(bag_type)
    {
	case SEC_OID_PKCS12_KEY_BAG_ID:
	    bag->safeContent.keyBag = 
    		(SEC_PKCS12PrivateKeyBag *)PORT_ArenaZAlloc(poolp, 
		sizeof(SEC_PKCS12PrivateKeyBag));
	    if(bag->safeContent.keyBag == NULL)
		goto loser;
	    bag->safeContent.keyBag->poolp = poolp;
	    break;
	case SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID:
	    bag->safeContent.certAndCRLBag = 
		(SEC_PKCS12CertAndCRLBag *)PORT_ArenaZAlloc(poolp,
		    sizeof(SEC_PKCS12CertAndCRLBag));
	    if(bag->safeContent.certAndCRLBag == NULL)
		goto loser;
	    bag->safeContent.certAndCRLBag->poolp = poolp;
	    break;
	case SEC_OID_PKCS12_SECRET_BAG_ID:
	    bag->safeContent.secretBag = 
		(SEC_PKCS12SecretBag *)PORT_ArenaZAlloc(poolp,
		    sizeof(SEC_PKCS12SecretBag));
	    if(bag->safeContent.secretBag == NULL)
		goto loser;
	    bag->safeContent.secretBag->poolp = poolp;
	    break;
	default:
	    /* unsupported type */
	    goto loser;
    }

    return bag;

loser:
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}

/* create a safe contents structure with a list of
 * length 0 with the first element being NULL 
 */
SEC_PKCS12SafeContents *
sec_pkcs12_create_safe_contents(PRArenaPool *poolp)
{
    SEC_PKCS12SafeContents *safe;
    void *mark;

    if(poolp == NULL)
	return NULL;

    /* allocate structure */
    mark = PORT_ArenaMark(poolp);
    safe = (SEC_PKCS12SafeContents *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12SafeContents));
    if(safe == NULL)
    {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    /* init list */
    safe->contents = PORT_ArenaZAlloc(poolp, sizeof(SEC_PKCS12SafeBag *));
    if(safe->contents == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }
    safe->contents[0] = NULL;
    safe->poolp = poolp;
    safe->safe_size = 0;
    return safe;
}

/* create a new external bag which is appended onto the list
 * of bags in baggage.  the bag is created in the same arena
 * as baggage
 */
SEC_PKCS12BaggageItem *
sec_pkcs12_create_external_bag(SEC_PKCS12Baggage *luggage)
{
    void *dummy, *mark;
    SEC_PKCS12BaggageItem *bag;

    if(luggage == NULL) {
	return NULL;
    }

    mark = PORT_ArenaMark(luggage->poolp);

    /* allocate space for null terminated bag list */
    if(luggage->bags == NULL) {
	luggage->bags = PORT_ArenaZAlloc(luggage->poolp, 
					sizeof(SEC_PKCS12BaggageItem *));
	if(luggage->bags == NULL) {
	    goto loser;
	}
	luggage->luggage_size = 0;
    }

    /* grow the list */    
    dummy = PORT_ArenaGrow(luggage->poolp, luggage->bags,
    			sizeof(SEC_PKCS12BaggageItem *) * (luggage->luggage_size + 1),
    			sizeof(SEC_PKCS12BaggageItem *) * (luggage->luggage_size + 2));
    if(dummy == NULL) {
	goto loser;
    }

    luggage->bags[luggage->luggage_size] = 
    		(SEC_PKCS12BaggageItem *)PORT_ArenaZAlloc(luggage->poolp,
    							sizeof(SEC_PKCS12BaggageItem));
    if(luggage->bags[luggage->luggage_size] == NULL) {
	goto loser;
    }

    /* create new bag and append it to the end */
    bag = luggage->bags[luggage->luggage_size];
    bag->espvks = (SEC_PKCS12ESPVKItem **)PORT_ArenaZAlloc(
    						luggage->poolp,
    						sizeof(SEC_PKCS12ESPVKItem *));
    bag->unencSecrets = (SEC_PKCS12SafeBag **)PORT_ArenaZAlloc(
    						luggage->poolp,
    						sizeof(SEC_PKCS12SafeBag *));
    if((bag->espvks == NULL) || (bag->unencSecrets == NULL)) {
	goto loser;
    }

    bag->poolp = luggage->poolp;
    luggage->luggage_size++;
    luggage->bags[luggage->luggage_size] = NULL;
    bag->espvks[0] = NULL;
    bag->unencSecrets[0] = NULL;
    bag->nEspvks = bag->nSecrets = 0;

    return bag;

loser:
    PORT_ArenaRelease(luggage->poolp, mark);
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return NULL;
}

/* creates a baggage witha NULL terminated 0 length list */
SEC_PKCS12Baggage *
sec_pkcs12_create_baggage(PRArenaPool *poolp)
{
    SEC_PKCS12Baggage *luggage;
    void *mark;

    if(poolp == NULL)
	return NULL;

    mark = PORT_ArenaMark(poolp);

    /* allocate bag */
    luggage = (SEC_PKCS12Baggage *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12Baggage));
    if(luggage == NULL)
    {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    /* init list */
    luggage->bags = (SEC_PKCS12BaggageItem **)PORT_ArenaZAlloc(poolp,
    					sizeof(SEC_PKCS12BaggageItem *));
    if(luggage->bags == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    luggage->bags[0] = NULL;
    luggage->luggage_size = 0;
    luggage->poolp = poolp;

    return luggage;
}

/* create an espvk item of the appropriate type -- currently only
 * pkcs 8 key shrouding 
 */
SEC_PKCS12ESPVKItem *
sec_pkcs12_create_espvk(PRArenaPool *poolp, 
			SECOidTag shroud_type)
{
    SEC_PKCS12ESPVKItem *espvk;
    void *mark;

    if(poolp == NULL)
	return NULL;

    mark = PORT_ArenaMark(poolp);

    espvk = (SEC_PKCS12ESPVKItem *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12ESPVKItem));
    if(espvk == NULL)
        goto loser;
    espvk->poolp = poolp;
    espvk->espvkTag = SECOID_FindOIDByTag(shroud_type);
    SECITEM_CopyItem(poolp, &espvk->espvkOID, &espvk->espvkTag->oid);

    return espvk;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}

/* initialize PVKSupportingData structure -- specifically the list of
 * associated certs
 */
SECStatus
sec_pkcs12_init_pvk_data(PRArenaPool *poolp, SEC_PKCS12PVKSupportingData *pvk)

{
    if((poolp == NULL) || (pvk == NULL))
	return SECFailure;

    pvk->poolp = poolp;
    pvk->assocCerts = (SGNDigestInfo **)PORT_ArenaZAlloc(poolp, 
    						sizeof(SGNDigestInfo *));
    if(pvk->assocCerts == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }

    pvk->assocCerts[0] = NULL;
    pvk->regenerable.data = pvk->pvkAdditionalDER.data = pvk->nickname.data = NULL;
    pvk->regenerable.len = pvk->pvkAdditionalDER.len = pvk->nickname.len = 0;

    return SECSuccess;
}
    
/* free pfx structure and associated items in the arena */
void 
SEC_PKCS12DestroyPFX(SEC_PKCS12PFXItem *pfx)
{
    if(pfx != NULL)
	if(pfx->poolp != NULL)
	{
	    PRArenaPool *poolp = pfx->poolp;
	    PORT_FreeArena(poolp, PR_TRUE);
	}
}
