/*
 * PKCS #11 Wrapper functions which handles authenticating to the card's
 * choosing the best cards, etc.
 */
#ifndef _PK11FUNC_H_
#define _PK11FUNC_H_
#include "prarena.h"
#include "mcom_db.h"
#include "seccomon.h"
#include "secoidt.h"
#include "secdert.h"
#include "cryptot.h"
#include "keyt.h"
#include "certt.h"
#include "pkcs11t.h"
#include "secmodt.h"
#include "seccomon.h"

SEC_BEGIN_PROTOS

/************************************************************
 * Generic Slot Lists Management
 ************************************************************/
PK11SlotList * PK11_NewSlotList(void);
void PK11_FreeSlotList(PK11SlotList *list);
SECStatus PK11_AddSlotToList(PK11SlotList *list,PK11SlotInfo *slot);
SECStatus PK11_DeleteSlotFromList(PK11SlotList *list,PK11SlotListElement *le);
PK11SlotListElement * PK11_GetFirstSafe(PK11SlotList *list);
PK11SlotListElement *pk11_GetNextSafe(PK11SlotList *list, 
				PK11SlotListElement *le, PRBool restart);
PK11SlotListElement *PK11_FindSlotElement(PK11SlotList *list,
							PK11SlotInfo *slot);

/************************************************************
 * Generic Slot Management
 ************************************************************/
PK11SlotInfo *PK11_ReferenceSlot(PK11SlotInfo *slot);
void PK11_FreeSlot(PK11SlotInfo *slot);
SECStatus PK11_DestroyObject(PK11SlotInfo *slot,CK_OBJECT_HANDLE object);
SECStatus PK11_DestroyTokenObject(PK11SlotInfo *slot,CK_OBJECT_HANDLE object);
CK_OBJECT_HANDLE PK11_CopyKey(PK11SlotInfo *slot, CK_OBJECT_HANDLE srcObject);
SECStatus PK11_ReadAttribute(PK11SlotInfo *slot, CK_OBJECT_HANDLE id,
         CK_ATTRIBUTE_TYPE type, PRArenaPool *arena, SECItem *result);
PK11SlotInfo *PK11_GetInternalKeySlot(void);
char * PK11_MakeString(PRArenaPool *arena,char *space,char *staticSring,
								int stringLen);
int PK11_MapError(CK_RV error);
CK_SESSION_HANDLE PK11_GetRWSession(PK11SlotInfo *slot);
void PK11_RestoreROSession(PK11SlotInfo *slot,CK_SESSION_HANDLE rwsession);
PK11SlotInfo *PK11_NewSlotInfo(void);
SECStatus PK11_Logout(PK11SlotInfo *slot);
void PK11_LogoutAll(void);


/************************************************************
 *  Slot Password Management
 ************************************************************/
void PK11_SetSlotPWValues(PK11SlotInfo *slot,int askpw, int timeout);
void PK11_GetSlotPWValues(PK11SlotInfo *slot,int *askpw, int *timeout);
SECStatus PK11_CheckSSOPassword(PK11SlotInfo *slot, char *ssopw);
SECStatus PK11_CheckUserPassword(PK11SlotInfo *slot,char *pw);
SECStatus PK11_DoPassword(PK11SlotInfo *slot, void *wincx);
PRBool PK11_IsLoggedIn(PK11SlotInfo *slot);
SECStatus PK11_VerifyPW(PK11SlotInfo *slot,char *pw);
SECStatus PK11_InitPin(PK11SlotInfo *slot,char *ssopw, char *userpw);
SECStatus PK11_ChangePW(PK11SlotInfo *slot,char *oldpw, char *newpw);
void PK11_HandlePasswordCheck(PK11SlotInfo *slot,void *wincx);
void PK11_SetPasswordFunc(char *(*func)(PK11SlotInfo *,void *));


/************************************************************
 * Manage the built-In Slot Lists
 ************************************************************/
SECStatus PK11_InitSlotLists(void);
PK11SlotList *PK11_GetSlotList(CK_MECHANISM_TYPE type);
void PK11_LoadSlotList(PK11SlotInfo *slot, PK11PreSlotInfo *psi, int count);
void PK11_ClearSlotList(PK11SlotInfo *slot);
void PK11_ConfigurePKCS11(char *man, char *libdes, char *tokdes, char *ptokdes,
 char *slotdes, char *pslotdes, char *fslotdes, char *fpslotdes);
void PK11_ConfigureFIPS(char *slotdes, char *pslotdes);


/******************************************************************
 *           Slot initialization
 ******************************************************************/
PRBool PK11_VerifyMechanism(PK11SlotInfo *slot,PK11SlotInfo *intern,
  CK_MECHANISM_TYPE mech, SECItem *data, SECItem *iv, SECItem *key);
PRBool PK11_VerifySlotMechanisms(PK11SlotInfo *slot);
SECStatus pk11_CheckVerifyTest(PK11SlotInfo *slot);
SECStatus PK11_InitToken(PK11SlotInfo *slot);
void PK11_InitSlot(SECMODModule *mod,CK_SLOT_ID slotID,PK11SlotInfo *slot);


/******************************************************************
 *           Slot info functions
 ******************************************************************/
PRBool PK11_IsReadOnly(PK11SlotInfo *slot);
PRBool PK11_IsInternal(PK11SlotInfo *slot);
char * PK11_GetTokenName(PK11SlotInfo *slot);
char * PK11_GetSlotName(PK11SlotInfo *slot);
PRBool PK11_NeedLogin(PK11SlotInfo *slot);
PRBool PK11_NeedUserInit(PK11SlotInfo *slot);
SECStatus PK11_GetSlotInfo(PK11SlotInfo *slot, CK_SLOT_INFO *info);
SECStatus PK11_GetTokenInfo(PK11SlotInfo *slot, CK_TOKEN_INFO *info);
PRBool PK11_IsDisabled(PK11SlotInfo *slot);
int PK11_GetDisabledReason(PK11SlotInfo *slot);
PRBool PK11_NeedPWInit(void);
/*Sigh*/
SECStatus PK11_GetModInfo(SECMODModule *mod, CK_INFO *info);


/*********************************************************************
 *            Slot mapping utility functions.
 *********************************************************************/
PRBool PK11_IsPresent(PK11SlotInfo *slot);
PRBool PK11_DoesMechanism(PK11SlotInfo *slot, CK_MECHANISM_TYPE type);
PK11SlotList * PK11_GetAllTokens(CK_MECHANISM_TYPE type,PRBool needRW) ;
PK11SlotList * PK11_GetPrivateKeyTokens(CK_MECHANISM_TYPE type,
						PRBool needRW,void *wincx);
PK11SlotInfo *PK11_GetBestSlot(CK_MECHANISM_TYPE type, void *wincx);

/*********************************************************************
 *       Mechanism Mapping functions
 *********************************************************************/
void PK11_AddMechanismEntry(CK_MECHANISM_TYPE type, CK_KEY_TYPE key,
		 	CK_MECHANISM_TYPE keygen, int ivLen, int blocksize);
CK_MECHANISM_TYPE PK11_GetKeyType(CK_MECHANISM_TYPE type,unsigned long len);
CK_MECHANISM_TYPE PK11_GetKeyGen(CK_MECHANISM_TYPE type);
int PK11_GetBlockSize(CK_MECHANISM_TYPE type,SECItem *params);
int PK11_GetIVLength(CK_MECHANISM_TYPE type);
SECItem *PK11_ParamFromIV(CK_MECHANISM_TYPE type,SECItem *iv);
SECItem * PK11_BlockData(SECItem *data,unsigned long size);

/* PKCS #11 to DER mapping functions */
SECItem *PK11_ParamFromAlgid(SECAlgorithmID *algid);
SECItem *PK11_GenerateNewParam(CK_MECHANISM_TYPE, PK11SymKey *);
CK_MECHANISM_TYPE PK11_AlgtagToMechanism(SECOidTag algTag);
SECStatus PK11_ParamToAlgid(SECOidTag algtag, SECItem *param,
                                   PRArenaPool *arena, SECAlgorithmID *algid);
SECStatus PK11_GenerateRandom(unsigned char *data,int len);
CK_RV PK11_MapPBEMechanismToCryptoMechanism(CK_MECHANISM_PTR pPBEMechanism,
					    CK_MECHANISM_PTR pCryptoMechanism,
					    SECItem *pbe_pwd);


/**********************************************************************
 *                   Symetric, Public, and Private Keys 
 **********************************************************************/
PK11SymKey *PK11_CreateSymKey(PK11SlotInfo *slot, 
					CK_MECHANISM_TYPE type, void *wincx);
void PK11_FreeSymKey(PK11SymKey *key);
PK11SymKey *PK11_ReferenceSymKey(PK11SymKey *symKey);
PK11SymKey *PK11_ImportSymKey(PK11SlotInfo *slot, CK_MECHANISM_TYPE type,
                     CK_ATTRIBUTE_TYPE operation, SECItem *key,void *wincx);
CK_OBJECT_HANDLE PK11_ImportPublicKey(PK11SlotInfo *slot, 
						SECKEYPublicKey *pubKey);
PK11SymKey *PK11_KeyGen(PK11SlotInfo *slot,CK_MECHANISM_TYPE type,
						int keySize,void *wincx);
SECStatus PK11_PubWrapSymKey(CK_MECHANISM_TYPE type, SECKEYPublicKey *pubKey,
				PK11SymKey *symKey, SECItem *wrappedKey);
SECStatus PK11_WrapSymKey(CK_MECHANISM_TYPE type, PK11SymKey *wrappingKey,
				PK11SymKey *symKey, SECItem *wrappedKey);
PK11SymKey *PK11_DeriveSymKey( SECKEYPrivateKey *privKey, 
 SECKEYPublicKey *pubKey, PRBool isSender, SECItem *randomA, SECItem *randomB,
 CK_MECHANISM_TYPE derive, CK_MECHANISM_TYPE target,
						int keySize,void *wincx) ;
PK11SymKey *PK11_UnwrapSymKey(PK11SymKey *key, CK_MECHANISM_TYPE wraptype,
	SECItem *wrapppedKey,  CK_MECHANISM_TYPE target, int keySize);
PK11SymKey *PK11_PubUnwrapSymKey(SECKEYPrivateKey *key, SECItem *wrapppedKey,
				 CK_MECHANISM_TYPE target, int keySize);
/* size to hold key in bytes */
unsigned int PK11_GetKeyLength(PK11SymKey *key);
/* size of actual secret parts of key in bits */
unsigned int PK11_GetKeyStrength(PK11SymKey *key);
SECStatus PK11_ExtractKeyValue(PK11SymKey *symKey);
SECItem * PK11_GetKeyData(PK11SymKey *symKey);
SECKEYPrivateKey *PK11_GenerateKeyPair(PK11SlotInfo *slot,
   CK_MECHANISM_TYPE type, void *param, SECKEYPublicKey **pubk,
		 	    PRBool isPerm, PRBool isSensitive, void *wincx);
SECKEYPrivateKey *PK11_MakePrivKey(PK11SlotInfo *slot, KeyType keyType,
                        PRBool isTemp, CK_OBJECT_HANDLE privID, void *wincx);
SECKEYPrivateKey * PK11_FindPrivateKeyFromCert(PK11SlotInfo *slot,
				 	CERTCertificate *cert, void *wincx);
SECKEYPrivateKey * PK11_FindKeyByAnyCert(CERTCertificate *cert, void *wincx);
SECKEYPrivateKey * PK11_FindKeyByKeyID(PK11SlotInfo *slot, SECItem *keyID,
				       void *wincx);
CK_OBJECT_HANDLE PK11_FindObjectForCert(CERTCertificate *cert,
					void *wincx, PK11SlotInfo **pSlot);
int PK11_GetPrivateModulusLen(SECKEYPrivateKey *key); 
SECStatus PK11_PubDecryptRaw(SECKEYPrivateKey *key, unsigned char *data,
   unsigned *outLen, unsigned int maxLen, unsigned char *enc, unsigned encLen);
/* The encrypt version of the above function */
SECStatus PK11_PubEncryptRaw(SECKEYPublicKey *key, unsigned char *enc,
                unsigned char *data, unsigned dataLen, void *wincx);
SECStatus PK11_ImportPrivateKeyInfo(PK11SlotInfo *slot, 
		SECKEYPrivateKeyInfo *pki, SECItem *nickname, void *wincx);
SECStatus PK11_ImportEncryptedPrivateKeyInfo(PK11SlotInfo *slot, 
		SECKEYEncryptedPrivateKeyInfo *epki, SECItem *pwitem, 
		SECItem *nickname, void *wincx);
SECKEYPrivateKeyInfo *PK11_ExportPrivateKeyInfo(
		SECItem *nickname, CERTCertificate *cert, void *wincx);
SECKEYEncryptedPrivateKeyInfo *PK11_ExportEncryptedPrivateKeyInfo(
		PK11SlotInfo *slot, SECOidTag algTag, SECItem *pwitem,
		SECItem *nickname, CERTCertificate *cert, int iteration, 
		int keyLength, void *wincx);
SECKEYPrivateKey *PK11_FindKeyByDERCert(PK11SlotInfo *slot, 
					CERTCertificate *cert, void *wincx);

/**********************************************************************
 *                   Certs
 **********************************************************************/
CERTCertificate *PK11_GetCertFromPrivateKey(SECKEYPrivateKey *privKey);
SECStatus PK11_TraverseSlotCerts(
     SECStatus(* callback)(CERTCertificate*,SECItem *,void *),
                                                void *arg, void *wincx);
CERTCertificate * PK11_FindCertFromNickname(char *nickname, void *wincx);
SECKEYPrivateKey * PK11_FindPrivateKeyFromNickname(char *nickname, void *wincx);
PK11SlotInfo *PK11_ImportCertForKey(CERTCertificate *cert, char *nickname,
								void *wincx);
CK_OBJECT_HANDLE PK11_FindObjectFromNickname(char *nickname,
		PK11SlotInfo **slotptr, CK_OBJECT_CLASS class,void *wincx);
PK11SlotInfo *PK11_KeyForCertExists(CERTCertificate *cert,
					CK_OBJECT_HANDLE *keyPtr, void *wincx);
CK_OBJECT_HANDLE PK11_MatchItem(PK11SlotInfo *slot,CK_OBJECT_HANDLE peer,
						CK_OBJECT_CLASS class); 
CERTCertificate * PK11_FindCertByIssuerAndSN(PK11SlotInfo **slot,
					CERTIssuerAndSN *sn, void *wincx);
CK_BBOOL PK11_HasAttributeSet( PK11SlotInfo *slot,
			       CK_OBJECT_HANDLE id,
			       CK_ATTRIBUTE_TYPE type );
CK_RV PK11_GetAttributes(PRArenaPool *arena,PK11SlotInfo *slot,
			 CK_OBJECT_HANDLE obj,CK_ATTRIBUTE *attr, int count);
int PK11_NumberCertsForCertSubject(CERTCertificate *cert);
SECStatus PK11_TraverseCertsForSubject(CERTCertificate *cert, 
	SECStatus(*callback)(CERTCertificate *, void *), void *arg);
CERTCertificate *PK11_FindCertFromDERCert(PK11SlotInfo *slot, 
					  CERTCertificate *cert, void *wincx);
CERTCertificate *PK11_FindCertFromDERSubjectAndNickname(
					PK11SlotInfo *slot, 
					CERTCertificate *cert, char *nickname,
					void *wincx);
SECStatus PK11_ImportCertForKeyToSlot(PK11SlotInfo *slot, CERTCertificate *cert,
					char *nickname, void *wincx);

/**********************************************************************
 *                   Sign/Verify 
 **********************************************************************/
int PK11_SignatureLen(SECKEYPrivateKey *key);
SECStatus PK11_Sign(SECKEYPrivateKey *key, SECItem *sig, SECItem *hash);
SECStatus PK11_VerifyRecover(SECKEYPublicKey *key, SECItem *sig,
						 SECItem *dsig, void * wincx);
SECStatus PK11_Verify(SECKEYPublicKey *key, SECItem *sig, 
						SECItem *hash, void *wincx);



/**********************************************************************
 *                   Crypto Contexts
 **********************************************************************/
void PK11_DestroyContext(PK11Context *context, PRBool freeit);
PK11Context * PK11_CreateContextByRawKey(PK11SlotInfo *slot, 
    CK_MECHANISM_TYPE type, CK_ATTRIBUTE_TYPE operation, SECItem *key, 
						SECItem *param, void *wincx);
PK11Context *PK11_CreateContextBySymKey(CK_MECHANISM_TYPE type,
	CK_ATTRIBUTE_TYPE operation, PK11SymKey *symKey, SECItem *param);
PK11Context *PK11_CreateDigestContext(SECOidTag hashAlg);
PK11Context *PK11_CloneContext(PK11Context *old);
SECStatus PK11_DigestBegin(PK11Context *cx);
SECStatus PK11_HashBuf(SECOidTag hashAlg, unsigned char *out, unsigned char *in,
					int32 len);
SECStatus PK11_DigestOp(PK11Context *context, unsigned char *in, unsigned len);
SECStatus PK11_CipherOp(PK11Context *context, unsigned char * out, int *outlen, 
				int maxout, unsigned char *in, int inlen);
SECStatus PK11_Finalize(PK11Context *context);
SECStatus PK11_DigestFinal(PK11Context *context, unsigned char *data, 
				unsigned int *outLen, unsigned int length);
PRBool PK11_HashOK(SECOidTag hashAlg);


/**********************************************************************
 *                   PBE functions 
 **********************************************************************/
SECAlgorithmID *
PK11_CreatePBEAlgorithmID(SECOidTag algorithm, int iteration, SECItem *salt,
	int keyLength, SECItem *iv, SECItem *op_param);
PK11SymKey *
PK11_PBEKeyGen(PK11SlotInfo *slot, SECAlgorithmID *algid,  SECItem *pwitem,
	void *wincx);
SECItem *
PK11_GetPBEIV(SECAlgorithmID *algid, SECItem *pwitem);

SEC_END_PROTOS

#endif
