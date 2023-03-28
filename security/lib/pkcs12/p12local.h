#ifndef _P12LOCAL_H_
#define _P12LOCAL_H_

#include "prarena.h"
#include "secoidt.h"
#include "secasn1.h"
#include "secder.h"
#include "certt.h"
#include "crypto.h"
#include "key.h"
#include "hasht.h"
#include "secpkcs7.h"
#include "pkcs12.h"
#include "prcpucfg.h"

/* helper functions */
extern const SEC_ASN1Template *
sec_pkcs12_choose_bag_type(void *src_or_dest, PRBool encoding);
extern const SEC_ASN1Template *
sec_pkcs12_choose_cert_crl_type(void *src_or_dest, PRBool encoding);
extern const SEC_ASN1Template *
sec_pkcs12_choose_shroud_type(void *src_or_dest, PRBool encoding);
extern SECItem *sec_pkcs12_generate_salt(void);
extern SECItem *sec_pkcs12_generate_key_from_password(SECOidTag algorithm, 
	SECItem *salt, SECItem *password);
extern SECItem *sec_pkcs12_generate_mac(SECItem *key, SECItem *msg, 
					PRBool old_method);
extern SGNDigestInfo *sec_pkcs12_compute_thumbprint(SECItem *der_cert);
extern PRBool sec_pkcs12_compare_thumbprint(SGNDigestInfo *a, SGNDigestInfo *b);
extern SECItem *sec_pkcs12_create_virtual_password(SECItem *password, 
						   SECItem *salt, 
						   PRBool swapUnicodeBytes);
extern SECStatus sec_pkcs12_append_shrouded_key(SEC_PKCS12BaggageItem *bag,
						 SEC_PKCS12ESPVKItem *espvk);
extern SECItem *sec_pkcs12_convert_item_to_unicode(SECItem *textItem, 
						   PRBool swapUnicodeBytes);
extern SECItem *sec_pkcs12_convert_item_from_unicode(SECItem *uniItem);
extern SECStatus sec_pkcs12_copy_nickname(PRArenaPool *poolp, SECItem *a,
					  SECItem *b);
extern PRBool sec_pkcs12_export_alg_read_safe(SECOidTag algorithm, int keyLen);
extern PRBool sec_pkcs12_export_alg_read_espvk(SECOidTag algorithm, int keyLen);
extern SECStatus sec_pkcs12_init_pvk_data(PRArenaPool *poolp, 
					  SEC_PKCS12PVKSupportingData *pvk);
extern void *sec_pkcs12_find_object(SEC_PKCS12SafeContents *safe,
				SEC_PKCS12Baggage *baggage, SECOidTag objType,
				SECItem *nickname, SGNDigestInfo *thumbprint);
extern SECStatus sec_pkcs12_prepare_for_der_code_baggage(SEC_PKCS12Baggage *baggage, PRBool encode);
extern SECStatus sec_pkcs12_prepare_for_der_code_safe(SEC_PKCS12SafeContents *safe, PRBool encode);
extern SECStatus sec_pkcs12_copy_and_convert_unicode_string(PRArenaPool *poolp, 
						SECItem *dest, SECItem *src,
						PRBool swapUnicodeBytes);
extern SECStatus sec_pkcs12_swap_unicode_bytes(SECItem *src);

/* create functions */
extern SEC_PKCS12PFXItem *sec_pkcs12_new_pfx(void);
extern SEC_PKCS12X509CertCRL *sec_pkcs12_new_x509_cert_crl(PRArenaPool *poolp);
extern SEC_PKCS12SDSICert *sec_pkcs12_new_sdsi_cert(PRArenaPool *poolp);
extern SEC_PKCS12CertAndCRL *sec_pkcs12_new_cert_crl(PRArenaPool *poolp,
    SECOidTag certType);
extern SEC_PKCS12SafeBag *sec_pkcs12_create_safe_bag(PRArenaPool *poolp,
    SECOidTag bag_type);
extern SEC_PKCS12SafeContents *sec_pkcs12_create_safe_contents(
	PRArenaPool *poolp);
extern SEC_PKCS12Baggage *sec_pkcs12_create_baggage(PRArenaPool *poolp);
extern SEC_PKCS12BaggageItem *sec_pkcs12_create_external_bag(SEC_PKCS12Baggage *luggage);
extern SEC_PKCS12ESPVKItem *sec_pkcs12_create_espvk(PRArenaPool *poolp,
	SECOidTag shroud_type);
extern void SEC_PKCS12DestroyPFX(SEC_PKCS12PFXItem *pfx);
extern SEC_PKCS12AuthenticatedSafe *sec_pkcs12_new_asafe(PRArenaPool *poolp);
	
#endif
