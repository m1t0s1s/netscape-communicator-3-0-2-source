#include "pkcs12.h"
#include "secpkcs7.h"
#include "secasn1.h"
#include "rsa.h"
#include "crypto.h"
#include "seccomon.h"
#include "secoid.h"
#include "sechash.h"
#include "secitem.h"
#include "secrng.h"
#include "p12local.h"
#include "alghmac.h"
#include "libi18n.h"

#define SALT_LENGTH	16

extern int SEC_ERROR_BAD_EXPORT_ALGORITHM;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME;

/* helper functions */
/* returns proper bag type template based upon object type tag */
const SEC_ASN1Template *
sec_pkcs12_choose_bag_type_old(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *template;
    SEC_PKCS12SafeBag *safebag;
    SECOidData *oiddata;

    if (src_or_dest == NULL) {
	return NULL;
    }

    safebag = src_or_dest;

    oiddata = safebag->safeBagTypeTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&safebag->safeBagType);
	safebag->safeBagTypeTag = oiddata;
    }

    switch (oiddata->offset) {
	default:
	    template = SEC_PointerToAnyTemplate;
	    break;
	case SEC_OID_PKCS12_KEY_BAG_ID:
	    template = SEC_PointerToPKCS12KeyBagTemplate;
	    break;
	case SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID:
	    template = SEC_PointerToPKCS12CertAndCRLBagTemplate_OLD;
	    break;
        case SEC_OID_PKCS12_SECRET_BAG_ID:
	    template = SEC_PointerToPKCS12SecretBagTemplate;
	    break;
    }
    return template;
}

const SEC_ASN1Template *
sec_pkcs12_choose_bag_type(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *template;
    SEC_PKCS12SafeBag *safebag;
    SECOidData *oiddata;

    if (src_or_dest == NULL) {
	return NULL;
    }

    safebag = src_or_dest;

    oiddata = safebag->safeBagTypeTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&safebag->safeBagType);
	safebag->safeBagTypeTag = oiddata;
    }

    switch (oiddata->offset) {
	default:
	    template = SEC_AnyTemplate;
	    break;
	case SEC_OID_PKCS12_KEY_BAG_ID:
	    template = SEC_PKCS12PrivateKeyBagTemplate;
	    break;
	case SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID:
	    template = SEC_PKCS12CertAndCRLBagTemplate;
	    break;
        case SEC_OID_PKCS12_SECRET_BAG_ID:
	    template = SEC_PKCS12SecretBagTemplate;
	    break;
    }
    return template;
}

/* returns proper cert crl template based upon type tag */
const SEC_ASN1Template *
sec_pkcs12_choose_cert_crl_type_old(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *template;
    SEC_PKCS12CertAndCRL *certbag;
    SECOidData *oiddata;

    if (src_or_dest == NULL) {
	return NULL;
    }

    certbag = src_or_dest;
    oiddata = certbag->BagTypeTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&certbag->BagID);
	certbag->BagTypeTag = oiddata;
    }

    switch (oiddata->offset) {
	default:
	    template = SEC_PointerToAnyTemplate;
	    break;
	case SEC_OID_PKCS12_X509_CERT_CRL_BAG:
	    template = SEC_PointerToPKCS12X509CertCRLTemplate_OLD;
	    break;
	case SEC_OID_PKCS12_SDSI_CERT_BAG:
	    template = SEC_PointerToPKCS12SDSICertTemplate;
	    break;
    }
    return template;
}

const SEC_ASN1Template *
sec_pkcs12_choose_cert_crl_type(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *template;
    SEC_PKCS12CertAndCRL *certbag;
    SECOidData *oiddata;

    if (src_or_dest == NULL) {
	return NULL;
    }

    certbag = src_or_dest;
    oiddata = certbag->BagTypeTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&certbag->BagID);
	certbag->BagTypeTag = oiddata;
    }

    switch (oiddata->offset) {
	default:
	    template = SEC_PointerToAnyTemplate;
	    break;
	case SEC_OID_PKCS12_X509_CERT_CRL_BAG:
	    template = SEC_PointerToPKCS12X509CertCRLTemplate;
	    break;
	case SEC_OID_PKCS12_SDSI_CERT_BAG:
	    template = SEC_PointerToPKCS12SDSICertTemplate;
	    break;
    }
    return template;
}

/* returns appropriate shroud template based on object type tag */
const SEC_ASN1Template *
sec_pkcs12_choose_shroud_type(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *template;
    SEC_PKCS12ESPVKItem *espvk;
    SECOidData *oiddata;

    if (src_or_dest == NULL) {
	return NULL;
    }

    espvk = src_or_dest;
    oiddata = espvk->espvkTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&espvk->espvkOID);
 	espvk->espvkTag = oiddata;
    }

    switch (oiddata->offset) {
	default:
	    template = SEC_PointerToAnyTemplate;
	    break;
	case SEC_OID_PKCS12_PKCS8_KEY_SHROUDING:
	   template = 
		SECKEY_PointerToEncryptedPrivateKeyInfoTemplate;
	    break;
    }
    return template;
}

/* temporary hack for export compliance, we can read any
 * encrypted private keys 
 */
PRBool sec_pkcs12_export_alg_read_espvk(SECOidTag algorithm, int keyLen)
{
    switch(algorithm) {
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC4:
	case SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC:
	    return PR_TRUE;
	default:
	    break;
    }
    return PR_FALSE;
}

/* we are only allowed to decrypt the safe if it is a 40 bit algorithm */
PRBool sec_pkcs12_export_alg_read_safe(SECOidTag algorithm, int keyLen)
{
    switch(algorithm) {
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4:
	case SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC:
	    PORT_SetError(SEC_ERROR_BAD_EXPORT_ALGORITHM);
	    return PR_FALSE;
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4:
	    return PR_TRUE;
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC4:
	    if(keyLen <= 40) {
		return PR_TRUE;
	    }
	    return PR_FALSE;
	default:
	    break;
    }

    return PR_FALSE;
}

/* generate SALT  placing it into the character array passed in.
 * it is assumed that salt_dest is an array of appropriate size
 * XXX We might want to generate our own random context
 */
SECItem *
sec_pkcs12_generate_salt(void)
{
    SECItem *salt;

    salt = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if(salt == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }
    salt->data = (unsigned char *)PORT_ZAlloc(sizeof(unsigned char) * 
					      SALT_LENGTH);
    salt->len = SALT_LENGTH;
    if(salt->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	SECITEM_ZfreeItem(salt, PR_TRUE);
	return NULL;
    }

    RNG_GenerateGlobalRandomBytes(salt->data, salt->len);

    return salt;
}

/* generate KEYS -- as per PKCS12 section 7.  
 * only used for MAC
 */
SECItem *
sec_pkcs12_generate_key_from_password(SECOidTag algorithm, 
				      SECItem *salt, 
				      SECItem *password) 
{
    unsigned char *pre_hash=NULL;
    unsigned char *hash_dest=NULL;
    SECStatus res;
    PRArenaPool *poolp;
    SECItem *key = NULL;
    int key_len = 0;

    if((salt == NULL) || (password == NULL)) {
	return NULL;
    }

    poolp = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if(poolp == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    pre_hash = (unsigned char *)PORT_ArenaZAlloc(poolp, sizeof(char) * 
						 (salt->len+password->len));
    if(pre_hash == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    hash_dest = (unsigned char *)PORT_ArenaZAlloc(poolp, 
					sizeof(unsigned char) * SHA1_LENGTH);
    if(hash_dest == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    PORT_Memcpy(pre_hash, salt->data, salt->len);
    /* handle password of 0 length case */
    if(password->len > 0) {
	PORT_Memcpy(&(pre_hash[salt->len]), password->data, password->len);
    }

    res = SHA1_HashBuf(hash_dest, pre_hash, (salt->len+password->len));
    if(res == SECFailure) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    switch(algorithm) {
	case SEC_OID_SHA1:
	    if(key_len == 0)
		key_len = 16;
	    key = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
	    if(key == NULL) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }
	    key->data = (unsigned char *)PORT_ZAlloc(sizeof(unsigned char) 
						     * key_len);
	    if(key->data == NULL) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }
	    key->len = key_len;
	    PORT_Memcpy(key->data, &hash_dest[SHA1_LENGTH-key->len], key->len);
	    break;
	default:
	    goto loser;
	    break;
    }

    PORT_FreeArena(poolp, PR_TRUE);
    return key;

loser:
    PORT_FreeArena(poolp, PR_TRUE);
    if(key != NULL) {
	SECITEM_ZfreeItem(key, PR_TRUE);
    }
    return NULL;
}

/* MAC is generated per PKCS 12 section 6.  It is expected that key, msg
 * and mac_dest are pre allocated, non-NULL arrays.  msg_len is passed in
 * because it is not known how long the message actually is.  String
 * manipulation routines will not necessarily work because msg may have
 * imbedded NULLs
 */
static SECItem *
sec_pkcs12_generate_old_mac(SECItem *key, 
			    SECItem *msg)
{
    SECStatus res;
    PRArenaPool *temparena = NULL;
    unsigned char *hash_dest=NULL, *hash_src1=NULL, *hash_src2 = NULL;
    int i;
    SECItem *mac = NULL;

    if((key == NULL) || (msg == NULL))
        goto loser;

    /* allocate return item */
    mac = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if(mac == NULL)
    	return NULL;
    mac->data = (unsigned char *)PORT_ZAlloc(sizeof(unsigned char)
    	* SHA1_LENGTH);
    mac->len = SHA1_LENGTH;
    if(mac->data == NULL)
	goto loser;

    /* allocate temporary items */
    temparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if(temparena == NULL)
	goto loser;

    hash_src1 = (unsigned char *)PORT_ArenaZAlloc(temparena,
    	sizeof(unsigned char) * (16+msg->len));
    if(hash_src1 == NULL)
        goto loser;

    hash_src2 = (unsigned char *)PORT_ArenaZAlloc(temparena,
   	sizeof(unsigned char) * (SHA1_LENGTH+16));
    if(hash_src2 == NULL)
        goto loser;

    hash_dest = (unsigned char *)PORT_ArenaZAlloc(temparena, 
   	sizeof(unsigned char) * SHA1_LENGTH);
    if(hash_dest == NULL)
        goto loser;

    /* perform mac'ing as per PKCS 12 */

    /* first round of hashing */
    for(i = 0; i < 16; i++)
	hash_src1[i] = key->data[i] ^ 0x36;
    PORT_Memcpy(&(hash_src1[16]), msg->data, msg->len);
    res = SHA1_HashBuf(hash_dest, hash_src1, (16+msg->len));
    if(res == SECFailure)
	goto loser;

    /* second round of hashing */
    for(i = 0; i < 16; i++)
	hash_src2[i] = key->data[i] ^ 0x5c;
    PORT_Memcpy(&(hash_src2[16]), hash_dest, SHA1_LENGTH);
    res = SHA1_HashBuf(mac->data, hash_src2, SHA1_LENGTH+16);
    if(res == SECFailure)
	goto loser;

    PORT_FreeArena(temparena, PR_TRUE);
    return mac;

loser:
    if(temparena != NULL)
	PORT_FreeArena(temparena, PR_TRUE);
    if(mac != NULL)
	SECITEM_ZfreeItem(mac, PR_TRUE);
    return NULL;
}

/* MAC is generated per PKCS 12 section 6.  It is expected that key, msg
 * and mac_dest are pre allocated, non-NULL arrays.  msg_len is passed in
 * because it is not known how long the message actually is.  String
 * manipulation routines will not necessarily work because msg may have
 * imbedded NULLs
 */
SECItem *
sec_pkcs12_generate_mac(SECItem *key, 
			SECItem *msg,
			PRBool old_method)
{
    SECStatus res = SECFailure;
    SECItem *mac = NULL;
    HMACContext *cx;

    if((key == NULL) || (msg == NULL)) {
	return NULL;
    }

    if(old_method == PR_TRUE) {
	return sec_pkcs12_generate_old_mac(key, msg);
    }

    /* allocate return item */
    mac = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if(mac == NULL) {
    	return NULL;
    }
    mac->data = (unsigned char *)PORT_ZAlloc(sizeof(unsigned char)
    					     * SHA1_LENGTH);
    mac->len = SHA1_LENGTH;
    if(mac->data == NULL) {
	PORT_Free(mac);
	return NULL;
    }

    /* compute MAC using HMAC */
    cx = HMAC_Create(SEC_OID_SHA1, key->data, key->len);
    if(cx != NULL) {
	HMAC_Begin(cx);
	HMAC_Update(cx, msg->data, msg->len);
	res = HMAC_Finish(cx, mac->data, &mac->len, SHA1_LENGTH);
	HMAC_Destroy(cx);
    }


    if(res != SECSuccess) {
	SECITEM_ZfreeItem(mac, PR_TRUE);
	mac = NULL;
    }

    return mac;
}

/* compute the thumbprint of the DER cert and create a digest info
 * to store it in and return the digest info.
 * a return of NULL indicates an error.
 */
SGNDigestInfo *
sec_pkcs12_compute_thumbprint(SECItem *der_cert)
{
    SGNDigestInfo *thumb = NULL;
    SECItem digest;
    PRArenaPool *temparena = NULL;
    SECStatus rv = SECFailure;

    if(der_cert == NULL)
	return NULL;

    temparena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(temparena == NULL) {
	return NULL;
    }

    digest.data = (unsigned char *)PORT_ArenaZAlloc(temparena,
						    sizeof(unsigned char) * 
						    SHA1_LENGTH);
    /* digest data and create digest info */
    if(digest.data != NULL) {
	digest.len = SHA1_LENGTH;
	rv = SHA1_HashBuf(digest.data, der_cert->data, der_cert->len);
	if(rv == SECSuccess) {
	    thumb = SGN_CreateDigestInfo(SEC_OID_SHA1, 
					 digest.data, 
					 digest.len);
	} else {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	}
    } else {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena(temparena, PR_TRUE);

    return thumb;
}

PRBool 
sec_pkcs12_compare_thumbprint(SGNDigestInfo *a, SGNDigestInfo *b)
{
    return (SGN_CompareDigestInfo(a, b) == SECEqual ? PR_TRUE : PR_FALSE);
}

/* convert a text item to unicode and return
 * NULL indicates an error 
 */
SECItem *
sec_pkcs12_convert_item_to_unicode(SECItem *textItem, PRBool swapUnicodeBytes)
{
    SECItem *uniItem;
    uint32 uniBufLen, retLen;

    if(textItem == NULL) {
	return NULL;
    }

    /* allocate space */
    uniItem = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if(uniItem == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }
    
    if(textItem->len == 0) {
	uniItem->data = PORT_ZAlloc(2);
	if(uniItem->data == NULL) {
	    goto loser;
	}

	uniItem->len = 2;
	return uniItem;
    }

    /* convert the text and allocate proper buffer */
    uniBufLen = INTL_TextToUnicodeLen(CS_ASCII, textItem->data, textItem->len);
    if(uniBufLen <= 0) {
	goto loser;
    }
    uniItem->data = (unsigned char *)PORT_ZAlloc(uniBufLen * 
    						 sizeof(unsigned short));
    uniItem->len = uniBufLen * 2;
    if(uniItem->data == NULL) {
	goto loser;
    }
    /* convert to unicode */
    retLen = INTL_TextToUnicode(CS_ASCII, textItem->data, textItem->len,
    				(unsigned short *)uniItem->data, uniBufLen);
    if(retLen+1 != uniBufLen) {
	PORT_Free(uniItem->data);
	goto loser;
    }

    if(swapUnicodeBytes) {
	if(sec_pkcs12_swap_unicode_bytes(uniItem) == SECFailure) {
	    PORT_Free(uniItem->data);
	    goto loser;
	}
    }
    
    return uniItem;

loser: 
    PORT_Free(uniItem);
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return NULL;
}

/* convert item from unicode */
SECItem *
sec_pkcs12_convert_item_from_unicode(SECItem *uniItem)
{
    SECItem *textItem;
    uint32 textBufLen;

    if(uniItem == NULL) {
	return NULL;
    }

    textItem = PORT_Alloc(sizeof(SECItem));
    if(textItem == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }
   
    /* get length & set up buffers */ 
    textBufLen = INTL_UnicodeToStrLen(CS_ASCII, (unsigned short *)uniItem->data, 
    					uniItem->len / 2);
    if(textBufLen <= 0) {
	goto loser;
    }
    textItem->data = (unsigned char *)PORT_ZAlloc(textBufLen * 
    						 sizeof(unsigned short));
    textItem->len = textBufLen;
    if(textItem->data == NULL) {
	goto loser;
    }

    /* convert text */
    INTL_UnicodeToStr(CS_ASCII, (INTL_Unicode*)uniItem->data, 
			uniItem->len / 2, textItem->data,
			textItem->len);
    
    return textItem;

loser: 
    PORT_Free(textItem);
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return NULL;
}

/* create a virtual password per PKCS 12, the password is converted
 * to unicode, the salt is prepended to it, and then the whole thing
 * is returned */
SECItem *
sec_pkcs12_create_virtual_password(SECItem *password, SECItem *salt,
				   PRBool swap)
{
    SECItem *uniPwd, *retPwd = NULL;

    if((password == NULL) || (salt == NULL)) {
	return NULL;
    }

    uniPwd = sec_pkcs12_convert_item_to_unicode(password, swap);
    if(uniPwd == NULL) {
	return NULL;
    }

    retPwd = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if(retPwd == NULL) {
	goto loser;
    }

    /* allocate space and copy proper data */
    retPwd->len = uniPwd->len + salt->len;
    retPwd->data = (unsigned char *)PORT_Alloc(retPwd->len);
    if(retPwd->data == NULL) {
	PORT_Free(retPwd);
	goto loser;
    }

    PORT_Memcpy(retPwd->data, salt->data, salt->len);
    PORT_Memcpy((retPwd->data + salt->len), uniPwd->data, uniPwd->len);

    SECITEM_ZfreeItem(uniPwd, PR_TRUE);

    return retPwd;

loser:
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    SECITEM_ZfreeItem(uniPwd, PR_TRUE);
    return NULL;
}

/* appends a shrouded key to a key bag.  this is used for exporting
 * to store externally wrapped keys.  it is used when importing to convert
 * old items to new
 */
SECStatus 
sec_pkcs12_append_shrouded_key(SEC_PKCS12BaggageItem *bag,
				SEC_PKCS12ESPVKItem *espvk)
{
    int size;
    void *mark = NULL, *dummy = NULL;

    if((bag == NULL) || (espvk == NULL))
	return SECFailure;

    mark = PORT_ArenaMark(bag->poolp);

    /* grow the list */
    size = (bag->nEspvks + 1) * sizeof(SEC_PKCS12ESPVKItem *);
    dummy = (SEC_PKCS12ESPVKItem **)PORT_ArenaGrow(bag->poolp,
	    				bag->espvks, size, 
	    				size + sizeof(SEC_PKCS12ESPVKItem *));

    if(dummy == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    bag->espvks[bag->nEspvks] = espvk;
    bag->nEspvks++;
    bag->espvks[bag->nEspvks] = NULL;

    return SECSuccess;

loser:
    PORT_ArenaRelease(bag->poolp, mark);
    return SECFailure;
}

/* search a certificate list for a nickname, a thumbprint, or both
 * within a certificate bag.  if the certificate could not be
 * found or an error occurs, NULL is returned;
 */
static SEC_PKCS12CertAndCRL *
sec_pkcs12_find_cert_in_certbag(SEC_PKCS12CertAndCRLBag *certbag,
				SECItem *nickname, SGNDigestInfo *thumbprint)
{
    PRBool search_both = PR_FALSE, search_nickname = PR_FALSE;
    int i, j;

    if((certbag == NULL) || ((nickname == NULL) && (thumbprint == NULL))) {
	return NULL;
    }

    if(thumbprint && nickname) {
	search_both = PR_TRUE;
    }

    if(nickname) {
	search_nickname = PR_TRUE;
    }

search_again:  
    i = 0;
    while(certbag->certAndCRLs[i] != NULL) {
	SEC_PKCS12CertAndCRL *cert = certbag->certAndCRLs[i];

	if(SECOID_FindOIDTag(&cert->BagID) == SEC_OID_PKCS12_X509_CERT_CRL_BAG) {

	    /* check nicknames */
	    if(search_nickname) {
		if(SECITEM_CompareItem(nickname, &cert->nickname) == SECEqual) {
		    return cert;
		}
	    } else {
	    /* check thumbprints */
		SECItem **derCertList;

		/* get pointer to certificate list, does not need to
		 * be freed since it is within the arena which will
		 * be freed later.
		 */
		derCertList = SEC_PKCS7GetCertificateList(&cert->value.x509->certOrCRL);
		j = 0;
		if(derCertList != NULL) {
		    while(derCertList[j] != NULL) {
			SECComparison eq;
			SGNDigestInfo *di;
			di = sec_pkcs12_compute_thumbprint(derCertList[j]);
			if(di) {
			    eq = SGN_CompareDigestInfo(thumbprint, di);
			    SGN_DestroyDigestInfo(di);
			    if(eq == SECEqual) {
				/* copy the derCert for later reference */
				cert->value.x509->derLeafCert = derCertList[j];
				return cert;
			    }
			} else {
			    /* an error occurred */
			    return NULL;
			}
			j++;
		    }
		}
	    }
	}

	i++;
    }

    if(search_both) {
	search_both = PR_FALSE;
	search_nickname = PR_FALSE;
	goto search_again;
    }

    return NULL;
}

/* search a key list for a nickname, a thumbprint, or both
 * within a key bag.  if the key could not be
 * found or an error occurs, NULL is returned;
 */
static SEC_PKCS12PrivateKey *
sec_pkcs12_find_key_in_keybag(SEC_PKCS12PrivateKeyBag *keybag,
			      SECItem *nickname, SGNDigestInfo *thumbprint)
{
    PRBool search_both = PR_FALSE, search_nickname = PR_FALSE;
    int i, j;

    if((keybag == NULL) || ((nickname == NULL) && (thumbprint == NULL))) {
	return NULL;
    }

    if(keybag->privateKeys == NULL) {
	return NULL;
    }

    if(thumbprint && nickname) {
	search_both = PR_TRUE;
    }

    if(nickname) {
	search_nickname = PR_TRUE;
    }

search_again:  
    i = 0;
    while(keybag->privateKeys[i] != NULL) {
	SEC_PKCS12PrivateKey *key = keybag->privateKeys[i];

	/* check nicknames */
	if(search_nickname) {
	    if(SECITEM_CompareItem(nickname, &key->pvkData.nickname) == SECEqual) {
		return key;
	    }
	} else {
	    /* check digests */
	    SGNDigestInfo **assocCerts = key->pvkData.assocCerts;
	    if((assocCerts == NULL) || (assocCerts[0] == NULL)) {
		return NULL;
	    }

	    j = 0;
	    while(assocCerts[j] != NULL) {
		SECComparison eq;
		eq = SGN_CompareDigestInfo(thumbprint, assocCerts[j]);
		if(eq == SECEqual) {
		    return key;
		}
		j++;
	    }
	}
	i++;
    }

    if(search_both) {
	search_both = PR_FALSE;
	search_nickname = PR_FALSE;
	goto search_again;
    }

    return NULL;
}

/* seach the safe first then try the baggage bag 
 *  safe and bag contain certs and keys to search
 *  objType is the object type to look for
 *  bagType is the type of bag that was found by sec_pkcs12_find_object
 *  index is the entity in safe->safeContents or bag->unencSecrets which
 *    is being searched
 *  nickname and thumbprint are the search criteria
 * 
 * a return of null indicates no match
 */
static void *
sec_pkcs12_try_find(SEC_PKCS12SafeContents *safe,
		  SEC_PKCS12BaggageItem *bag,
		  SECOidTag objType, SECOidTag bagType, int index,
		  SECItem *nickname, SGNDigestInfo *thumbprint)
{
    PRBool searchSafe;
    int i = index;

    if((safe == NULL) && (bag == NULL)) {
	return NULL;
    }

    searchSafe = (safe == NULL ? PR_FALSE : PR_TRUE);
    switch(objType) {
	case SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID:
	    if(objType == bagType) {
		SEC_PKCS12CertAndCRLBag *certBag;

		if(searchSafe) {
		    certBag = safe->contents[i]->safeContent.certAndCRLBag;
		} else {
		    certBag = bag->unencSecrets[i]->safeContent.certAndCRLBag;
		}
		return sec_pkcs12_find_cert_in_certbag(certBag, nickname, 
							thumbprint);
	    }
	    break;
	case SEC_OID_PKCS12_KEY_BAG_ID:
	    if(objType == bagType) {
		SEC_PKCS12PrivateKeyBag *keyBag;

		if(searchSafe) {
		    keyBag = safe->contents[i]->safeContent.keyBag;
		} else {
		    keyBag = bag->unencSecrets[i]->safeContent.keyBag;
		}
		return sec_pkcs12_find_key_in_keybag(keyBag, nickname, 
							 thumbprint);
	    }
	    break;
	default:
	    break;
    }

    return NULL;
}

/* searches both the baggage and the safe areas looking for
 * object of specified type matching either the nickname or the 
 * thumbprint specified.
 *
 * safe and baggage store certs and keys
 * objType is the OID for the bag type to be searched:
 *   SEC_OID_PKCS12_KEY_BAG_ID, or 
 *   SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID
 * nickname and thumbprint are the search criteria
 * 
 * if no match found, NULL returned and error set
 */
void *
sec_pkcs12_find_object(SEC_PKCS12SafeContents *safe,
			SEC_PKCS12Baggage *baggage,
			SECOidTag objType,
			SECItem *nickname,
			SGNDigestInfo *thumbprint)
{
    int i, j;
    PRBool found = PR_FALSE;
    PRBool problem = PR_FALSE;
    void *retItem;
   
    if(((safe == NULL) && (thumbprint == NULL)) ||
       ((nickname == NULL) && (thumbprint == NULL))) {
	return NULL;
    }    

    i = 0;
    if((safe != NULL) && (safe->contents != NULL)) {
	while(safe->contents[i] != NULL) {
	    SECOidTag bagType = SECOID_FindOIDTag(&safe->contents[i]->safeBagType);
	    retItem = sec_pkcs12_try_find(safe, NULL, objType, bagType, i,
	    				  nickname, thumbprint);
	    if(retItem != NULL) {
		return retItem;
	    }
	    i++;
	}
    }

    if((baggage != NULL) && (baggage->bags != NULL)) {
	i = 0;
	while(baggage->bags[i] != NULL) {
	    SEC_PKCS12BaggageItem *xbag = baggage->bags[i];
	    j = 0;
	    if(xbag->unencSecrets != NULL) {
		while(xbag->unencSecrets[j] != NULL) {
		    SECOidTag bagType;
		    bagType = SECOID_FindOIDTag(&xbag->unencSecrets[j]->safeBagType);
		    retItem = sec_pkcs12_try_find(NULL, xbag, objType, bagType,
		    				  j, nickname, thumbprint);
		    if(retItem != NULL) {
			return retItem;
		    }
		    j++;
		}
	    }
	    i++;
	}
    }

    PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME);
    return NULL;
}

SECStatus 
sec_pkcs12_copy_nickname(PRArenaPool *poolp, SECItem *a, SECItem *b)
{
    if((a == NULL) || (b == NULL) || (poolp == NULL)) {
	return SECFailure;
    }

   
    /* XXX this is added because routines using nickname do not
     * support know length unless string is null terminated
     */
    PORT_ArenaGrow(poolp, &b->data, b->len, b->len+1);
    b->data[b->len] = 0;

    if(a != b) {
	/* increment to copy the NULL */
	b->len++;
	SECITEM_CopyItem(poolp, a, b);
	b->len--;
	a->len--;
    }

    return SECSuccess;
}

SECStatus
sec_pkcs12_der_code_cert_crl(SEC_PKCS12CertAndCRL *cert, PRBool encode)
{
    void *dummy;
    const SEC_ASN1Template *template;
    void *src, *dest;
    SEC_PKCS12X509CertCRL *test_cert;
    PRArenaPool *arena;

    arena = PORT_NewArena(2048);
    test_cert = PORT_ArenaZAlloc(arena, sizeof(SEC_PKCS12X509CertCRL));

    if(cert == NULL) 
	return SECFailure;

    dest = &cert->derValue;
    switch(SECOID_FindOIDTag(&cert->BagID)) {
	case SEC_OID_PKCS12_X509_CERT_CRL_BAG:
	    src = cert->value.x509;
	    template = SEC_PKCS12X509CertCRLTemplate;
	    break;
	case SEC_OID_PKCS12_SDSI_CERT_BAG:
	    src = cert->value.sdsi;
	    template = SEC_PKCS12SDSICertTemplate;
	    break;
	default:
	    return SECFailure;
    }

    dummy = NULL;
    if(encode == PR_TRUE) {
	SEC_PKCS12CertAndCRL *ts = PORT_ArenaZAlloc(arena, sizeof(SEC_PKCS12CertAndCRL));
	dummy = SEC_ASN1EncodeItem(arena, dest, src, template);
    } else {
	SECStatus rv;
	rv = SEC_ASN1DecodeItem(cert->poolp, src, template, dest);
	if(rv == SECSuccess) {
	    dummy = src; 
	}
    }

    return (dummy ? SECSuccess : SECFailure);
}

SECStatus 
sec_pkcs12_der_code_cert_bag(SEC_PKCS12CertAndCRLBag *certbag, PRBool encode)
{
    int i;
    SECStatus rv;

    i = 0;
    if(certbag == NULL) {
	return SECFailure;
    }

    if(certbag->certAndCRLs == NULL) {
	/* if we encounter a NULL list on encoding -- there is an error.
	 * if we encounter a NULL list on decoding, that is the way the
	 * decoder is written...
	 */
	return (encode ? SECFailure : SECSuccess);
    }

    while(certbag->certAndCRLs[i] != NULL) {

	/* propagate the arena pool, just in case */
	certbag->certAndCRLs[i]->poolp = certbag->poolp;
	rv = sec_pkcs12_der_code_cert_crl(certbag->certAndCRLs[i], encode);

	if(rv != SECSuccess) {
	    return SECFailure;
	}

	i++;
    }

    return SECSuccess;
}

SECStatus
sec_pkcs12_der_code_safe_bag(SEC_PKCS12SafeBag *bag, PRBool encode)
{
    void *dummy = NULL;
    const SEC_ASN1Template *template;
    void *src, *dest;
    SECStatus rv;
    PRArenaPool *pool;
    SEC_PKCS12CertAndCRLBag *ts;

    pool = PORT_NewArena(2048);
    ts = PORT_ArenaZAlloc(pool, sizeof(SEC_PKCS12CertAndCRLBag));

    if(bag == NULL) 
	return SECFailure;

    rv = SECSuccess;
    dest = &bag->derSafeContent;
    switch(SECOID_FindOIDTag(&bag->safeBagType)) {
	case SEC_OID_PKCS12_KEY_BAG_ID:
	    src = bag->safeContent.keyBag;
	    template = SEC_PKCS12PrivateKeyBagTemplate;
	    break;
	case SEC_OID_PKCS12_SECRET_BAG_ID:
	    src = bag->safeContent.secretBag;
	    template = SEC_PKCS12SecretBagTemplate;
	    break;
	case SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID:
	    src = bag->safeContent.certAndCRLBag;
	    template = SEC_PKCS12CodedCertAndCRLBagTemplate;
	    bag->safeContent.certAndCRLBag->poolp = bag->poolp;
	    rv = sec_pkcs12_der_code_cert_bag(bag->safeContent.certAndCRLBag, encode);
	    break;
	default:
	    return SECFailure;
    }

    if(encode == PR_TRUE) {
	dummy = SEC_ASN1EncodeItem(bag->poolp, dest, src, template);
	SEC_ASN1DecodeItem(bag->poolp, src, template, dest);
    } else {
	SECStatus rv;
	rv = SEC_ASN1DecodeItem(bag->poolp, src, template, dest);
	if(rv == SECSuccess) {
	    dummy = src; 
	}
    }

    return (dummy ? SECSuccess : SECFailure);
}

SECStatus 
sec_pkcs12_prepare_for_der_code_safe(SEC_PKCS12SafeContents *safe, PRBool encode)
{
    int i;
    SECStatus rv;

    if(safe == NULL) {
	return SECFailure;
    }

    if(safe->contents == NULL) {
	/* if we encounter a NULL list on encoding -- there is an error.
	 * if we encounter a NULL list on decoding, that is the way the
	 * decoder is written...
	 */
	return (encode ? SECFailure : SECSuccess);
    }

    i = 0;
    while(safe->contents[i] != NULL) {
	SECOidTag bagType = SECOID_FindOIDTag(&safe->contents[i]->safeBagType);
	switch(bagType) {
	    case SEC_OID_PKCS12_KEY_BAG_ID:
	    case SEC_OID_PKCS12_SECRET_BAG_ID:
	    case SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID:
		/* propagate arena pool as necessary */
		safe->contents[i]->poolp = safe->poolp;

		rv = sec_pkcs12_der_code_safe_bag(safe->contents[i], encode);
		break;
	    default:
		return SECFailure;
	}
	i++;
    }

    return SECSuccess;
}

SECStatus 
sec_pkcs12_prepare_for_der_code_baggage(SEC_PKCS12Baggage *baggage, PRBool encode)
{
    int i, j;
    SECStatus rv;

    if(baggage == NULL) {
	return SECFailure;
    }

    if(baggage->bags == NULL) {
	/* if we encounter a NULL list on encoding -- there is an error.
	 * if we encounter a NULL list on decoding, that is the way the
	 * decoder is written...
	 */
	return (encode ? SECFailure : SECSuccess);
    }

    i = 0;
    while(baggage->bags[i] != NULL) {
	SEC_PKCS12BaggageItem *bag = baggage->bags[i];
	/* propagate arena pool, just in case */
	bag->poolp = baggage->poolp;

	if(bag->unencSecrets != NULL) {
	    j = 0;

	    while(bag->unencSecrets[j] != NULL) {
		/* propagate arena pool */
		bag->unencSecrets[j]->poolp = bag->poolp;
		rv = sec_pkcs12_der_code_safe_bag(bag->unencSecrets[j], encode);
		if(rv == SECFailure) {
		    return SECFailure;
		}
		j++;
	    }
	}
	i++;
    }

    return SECSuccess;
}

/* swap the byte order on a unicode string */
SECStatus
sec_pkcs12_swap_unicode_bytes(SECItem *uniItem)
{
    unsigned int i;
    unsigned char a;

    if((uniItem == NULL) || (uniItem->len % 2)) {
	return SECFailure;
    }

    for(i = 0; i < uniItem->len; i += 2) {
	a = uniItem->data[i];
	uniItem->data[i] = uniItem->data[i+1];
	uniItem->data[i+1] = a;
    }

    return SECSuccess;
}

/* converts a UniCode nickname to ascii and stores it in the requested
 * location.  
 */
SECStatus 
sec_pkcs12_copy_and_convert_unicode_string(PRArenaPool *poolp, SECItem *dest,
					   SECItem *src, PRBool swapUnicode)
{
    SECItem *textItem;
    SECStatus rv = SECSuccess;

    if((src == NULL) || (dest == NULL) || (src == dest)) {
	return SECFailure;
    }

    if(swapUnicode) {
	if(sec_pkcs12_swap_unicode_bytes(src) != SECSuccess) {
	    return SECFailure;
	}
    }

    textItem = sec_pkcs12_convert_item_from_unicode(src);
    if(textItem == NULL) {
	return SECFailure;
    }

    rv = SECITEM_CopyItem(poolp, dest, textItem);
    SECITEM_ZfreeItem(textItem, PR_TRUE);

    if(rv == SECFailure) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    return rv;
}

/* pkcs 12 templates */
static SEC_ChooseASN1TemplateFunc sec_pkcs12_shroud_chooser =
    sec_pkcs12_choose_shroud_type;

const SEC_ASN1Template SEC_PKCS12CodedSafeBagTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12SafeBag) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12SafeBag, safeBagType) },
    { SEC_ASN1_ANY, offsetof(SEC_PKCS12SafeBag, derSafeContent) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12CodedCertBagTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12CertAndCRL) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12CertAndCRL, BagID) },
    { SEC_ASN1_ANY, offsetof(SEC_PKCS12CertAndCRL, derValue) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12CodedCertAndCRLBagTemplate[] =
{
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12CertAndCRLBag, certAndCRLs),
	SEC_PKCS12CodedCertBagTemplate },
};

const SEC_ASN1Template SEC_PKCS12ESPVKItemTemplate_OLD[] = 
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12ESPVKItem) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12ESPVKItem, espvkOID) },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12ESPVKItem, espvkData),
	SEC_PKCS12PVKSupportingDataTemplate_OLD },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
	SEC_ASN1_DYNAMIC | 0, offsetof(SEC_PKCS12ESPVKItem, espvkCipherText),
	&sec_pkcs12_shroud_chooser },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12ESPVKItemTemplate[] = 
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12ESPVKItem) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12ESPVKItem, espvkOID) },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12ESPVKItem, espvkData),
	SEC_PKCS12PVKSupportingDataTemplate },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
	SEC_ASN1_DYNAMIC | 0, offsetof(SEC_PKCS12ESPVKItem, espvkCipherText),
	&sec_pkcs12_shroud_chooser },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12PVKAdditionalDataTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PVKAdditionalData) },
    { SEC_ASN1_OBJECT_ID, 
	offsetof(SEC_PKCS12PVKAdditionalData, pvkAdditionalType) },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	offsetof(SEC_PKCS12PVKAdditionalData, pvkAdditionalContent) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12PVKSupportingDataTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PVKSupportingData) },
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12PVKSupportingData, assocCerts),
	sgn_DigestInfoTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BOOLEAN, 
	offsetof(SEC_PKCS12PVKSupportingData, regenerable) },
    { SEC_ASN1_PRINTABLE_STRING, 
	offsetof(SEC_PKCS12PVKSupportingData, nickname) },
    { SEC_ASN1_ANY | SEC_ASN1_OPTIONAL,
	offsetof(SEC_PKCS12PVKSupportingData, pvkAdditionalDER) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12PVKSupportingDataTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PVKSupportingData) },
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12PVKSupportingData, assocCerts),
	sgn_DigestInfoTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BOOLEAN, 
	offsetof(SEC_PKCS12PVKSupportingData, regenerable) },
    { SEC_ASN1_BMP_STRING, 
	offsetof(SEC_PKCS12PVKSupportingData, uniNickName) },
    { SEC_ASN1_ANY | SEC_ASN1_OPTIONAL,
	offsetof(SEC_PKCS12PVKSupportingData, pvkAdditionalDER) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12BaggageItemTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12BaggageItem) },
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12BaggageItem, espvks),
	SEC_PKCS12ESPVKItemTemplate },
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12BaggageItem, unencSecrets),
	SEC_PKCS12SafeBagTemplate },
    /*{ SEC_ASN1_SET_OF, offsetof(SEC_PKCS12BaggageItem, unencSecrets),
	SEC_PKCS12CodedSafeBagTemplate }, */
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12BaggageTemplate[] =
{
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12Baggage, bags),
	SEC_PKCS12BaggageItemTemplate },
};

const SEC_ASN1Template SEC_PKCS12BaggageTemplate_OLD[] =
{
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12Baggage_OLD, espvks),
	SEC_PKCS12ESPVKItemTemplate_OLD },
};

static SEC_ChooseASN1TemplateFunc sec_pkcs12_bag_chooser =
	sec_pkcs12_choose_bag_type;

static SEC_ChooseASN1TemplateFunc sec_pkcs12_bag_chooser_old =
	sec_pkcs12_choose_bag_type_old;

const SEC_ASN1Template SEC_PKCS12SafeBagTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12SafeBag) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12SafeBag, safeBagType) },
    { SEC_ASN1_DYNAMIC | SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT |
	SEC_ASN1_CONTEXT_SPECIFIC | 0,
        offsetof(SEC_PKCS12SafeBag, safeContent),
	&sec_pkcs12_bag_chooser_old },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SafeBagTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12SafeBag) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12SafeBag, safeBagType) },
    { SEC_ASN1_DYNAMIC | SEC_ASN1_POINTER,
        offsetof(SEC_PKCS12SafeBag, safeContent),
	&sec_pkcs12_bag_chooser },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BMP_STRING,
	offsetof(SEC_PKCS12SafeBag, uniSafeBagName) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SafeContentsTemplate_OLD[] =
{
    { SEC_ASN1_SET_OF,
	offsetof(SEC_PKCS12SafeContents, contents),
	SEC_PKCS12SafeBagTemplate_OLD }
};

const SEC_ASN1Template SEC_PKCS12SafeContentsTemplate[] =
{
    { SEC_ASN1_SET_OF,
	offsetof(SEC_PKCS12SafeContents, contents),
	SEC_PKCS12SafeBagTemplate }  /* here */
};

const SEC_ASN1Template SEC_PKCS12PrivateKeyTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PrivateKey) },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12PrivateKey, pvkData),
	SEC_PKCS12PVKSupportingDataTemplate },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12PrivateKey, pkcs8data),
	SECKEY_PrivateKeyInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12PrivateKeyBagTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PrivateKeyBag) },
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12PrivateKeyBag, privateKeys),
	SEC_PKCS12PrivateKeyTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12X509CertCRLTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12X509CertCRL) },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12X509CertCRL, certOrCRL),
	sec_PKCS7ContentInfoTemplate },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12X509CertCRL, thumbprint),
	sgn_DigestInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12X509CertCRLTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12X509CertCRL) },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12X509CertCRL, certOrCRL),
	sec_PKCS7ContentInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SDSICertTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12X509CertCRL) },
    { SEC_ASN1_IA5_STRING, offsetof(SEC_PKCS12SDSICert, value) },
    { 0 }
};

static SEC_ChooseASN1TemplateFunc sec_pkcs12_cert_crl_chooser_old =
	sec_pkcs12_choose_cert_crl_type_old;

static SEC_ChooseASN1TemplateFunc sec_pkcs12_cert_crl_chooser =
	sec_pkcs12_choose_cert_crl_type;

const SEC_ASN1Template SEC_PKCS12CertAndCRLTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12CertAndCRL) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12CertAndCRL, BagID) },
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_EXPLICIT |
	SEC_ASN1_DYNAMIC | SEC_ASN1_CONSTRUCTED | 0,
	offsetof(SEC_PKCS12CertAndCRL, value),
	&sec_pkcs12_cert_crl_chooser_old },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12CertAndCRLTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12CertAndCRL) },
    { SEC_ASN1_OBJECT_ID, offsetof(SEC_PKCS12CertAndCRL, BagID) },
    { SEC_ASN1_DYNAMIC | SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT |
	SEC_ASN1_CONTEXT_SPECIFIC | 0, 
	offsetof(SEC_PKCS12CertAndCRL, value),
	&sec_pkcs12_cert_crl_chooser },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12CertAndCRLBagTemplate[] =
{
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12CertAndCRLBag, certAndCRLs),
	SEC_PKCS12CertAndCRLTemplate },
};

const SEC_ASN1Template SEC_PKCS12CertAndCRLBagTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12CertAndCRLBag) },
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12CertAndCRLBag, certAndCRLs),
	SEC_PKCS12CertAndCRLTemplate_OLD },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SecretAdditionalTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12SecretAdditional) },
    { SEC_ASN1_OBJECT_ID,
	offsetof(SEC_PKCS12SecretAdditional, secretAdditionalType) },
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_EXPLICIT,
	offsetof(SEC_PKCS12SecretAdditional, secretAdditionalContent) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SecretTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12Secret) },
    { SEC_ASN1_BMP_STRING, offsetof(SEC_PKCS12Secret, uniSecretName) },
    { SEC_ASN1_ANY, offsetof(SEC_PKCS12Secret, value) },
    { SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL,
	offsetof(SEC_PKCS12Secret, secretAdditional),
	SEC_PKCS12SecretAdditionalTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SecretItemTemplate[] = 
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12Secret) },
    { SEC_ASN1_INLINE | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	offsetof(SEC_PKCS12SecretItem, secret), SEC_PKCS12SecretTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	offsetof(SEC_PKCS12SecretItem, subFolder), SEC_PKCS12SafeBagTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12SecretBagTemplate[] =
{
    { SEC_ASN1_SET_OF, offsetof(SEC_PKCS12SecretBag, secrets),
	SEC_PKCS12SecretItemTemplate },
};

const SEC_ASN1Template SEC_PKCS12MacDataTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PFXItem) },
    { SEC_ASN1_INLINE, offsetof(SEC_PKCS12MacData, safeMac),
	sgn_DigestInfoTemplate },
    { SEC_ASN1_BIT_STRING, offsetof(SEC_PKCS12MacData, macSalt) },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12PFXItemTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PFXItem) },
    { SEC_ASN1_OPTIONAL |
	SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0, 
	offsetof(SEC_PKCS12PFXItem, macData), SEC_PKCS12MacDataTemplate },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1, 
	offsetof(SEC_PKCS12PFXItem, authSafe), 
	sec_PKCS7ContentInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12PFXItemTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12PFXItem) },
    { SEC_ASN1_OPTIONAL |
	SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0, 
	offsetof(SEC_PKCS12PFXItem, old_safeMac), sgn_DigestInfoTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BIT_STRING,
	offsetof(SEC_PKCS12PFXItem, old_macSalt) },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1, 
	offsetof(SEC_PKCS12PFXItem, authSafe), 
	sec_PKCS7ContentInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12AuthenticatedSafeTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12AuthenticatedSafe) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_INTEGER, 
	offsetof(SEC_PKCS12AuthenticatedSafe, version) }, 
    { SEC_ASN1_OPTIONAL | SEC_ASN1_OBJECT_ID,
	offsetof(SEC_PKCS12AuthenticatedSafe, transportMode) },
    { SEC_ASN1_BIT_STRING | SEC_ASN1_OPTIONAL,
	offsetof(SEC_PKCS12AuthenticatedSafe, privacySalt) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_SET_OF, 
	offsetof(SEC_PKCS12AuthenticatedSafe, baggage.bags), 
	SEC_PKCS12BaggageItemTemplate },
    { SEC_ASN1_POINTER,
	offsetof(SEC_PKCS12AuthenticatedSafe, safe),
	sec_PKCS7ContentInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PKCS12AuthenticatedSafeTemplate_OLD[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS12AuthenticatedSafe) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_INTEGER, 
	offsetof(SEC_PKCS12AuthenticatedSafe, version) }, 
    { SEC_ASN1_OPTIONAL | SEC_ASN1_INTEGER,
	offsetof(SEC_PKCS12AuthenticatedSafe, transportMode) },
    { SEC_ASN1_BIT_STRING,
	offsetof(SEC_PKCS12AuthenticatedSafe, privacySalt) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
    	SEC_ASN1_CONTEXT_SPECIFIC | 0, 
	offsetof(SEC_PKCS12AuthenticatedSafe, old_baggage), 
	SEC_PKCS12BaggageTemplate_OLD },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	offsetof(SEC_PKCS12AuthenticatedSafe, old_safe),
	sec_PKCS7ContentInfoTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_PointerToPKCS12KeyBagTemplate[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12PrivateKeyBagTemplate }
};

const SEC_ASN1Template SEC_PointerToPKCS12CertAndCRLBagTemplate_OLD[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12CertAndCRLBagTemplate_OLD }
};

const SEC_ASN1Template SEC_PointerToPKCS12CertAndCRLBagTemplate[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12CertAndCRLBagTemplate }
};

const SEC_ASN1Template SEC_PointerToPKCS12SecretBagTemplate[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12SecretBagTemplate }
};

const SEC_ASN1Template SEC_PointerToPKCS12X509CertCRLTemplate_OLD[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12X509CertCRLTemplate_OLD }
};

const SEC_ASN1Template SEC_PointerToPKCS12X509CertCRLTemplate[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12X509CertCRLTemplate }
};

const SEC_ASN1Template SEC_PointerToPKCS12SDSICertTemplate[] =
{
    { SEC_ASN1_POINTER, 0, SEC_PKCS12SDSICertTemplate }
};


