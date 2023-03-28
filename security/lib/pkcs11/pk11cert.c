/*
 * This file implements the Symkey wrapper and the PKCS context
 * Interfaces.
 */
#include "seccomon.h"
#include "secmod.h"
#include "prlock.h"
#include "secmodi.h"
#include "pkcs11.h"
#include "pk11func.h"
#include "cert.h"
#include "secitem.h"
#include "key.h"
#ifdef XP_MAC
#include "certdb.h"
#else
#include "../cert/certdb.h"
#endif

extern int SEC_ERROR_ADDING_CERT;
extern int SEC_ERROR_FILING_KEY;
extern int SEC_ERROR_NO_MODULE;
extern int SEC_ERROR_LIBRARY_FAILURE;
extern int SSL_ERROR_NO_CERTIFICATE;

#define PK11_SEARCH_CHUNKSIZE 10

/*
 * return a list of attributes. Allocate space for them. If an arena is
 * provided, allocate space out of the arena.
 */
CK_RV
PK11_GetAttributes(PRArenaPool *arena,PK11SlotInfo *slot,
			CK_OBJECT_HANDLE obj,CK_ATTRIBUTE *attr, int count)
{
    int i;
    /* make pedantic happy... note that it's only used arena != NULL */ 
    void *mark = NULL; 
    CK_RV crv;

    /*
     * first get all the lengths of the parameters.
     */
    crv = PK11_GETTAB(slot)->C_GetAttributeValue(slot->session,obj,attr,count);
    if (crv != CKR_OK) {
	return crv;
    }

    if (arena) {
    	mark = PORT_ArenaMark(arena);
	if (mark == NULL) return CKR_HOST_MEMORY;
    }

    /*
     * now allocate space to store the results.
     */
    for (i=0; i < count; i++) {
	if (arena) {
	    attr[i].pValue = PORT_ArenaAlloc(arena,attr[i].ulValueLen);
	    if (attr[i].pValue == NULL) {
		PORT_ArenaRelease(arena,mark);
		return CKR_HOST_MEMORY;
	    }
	} else {
	    attr[i].pValue = PORT_Alloc(attr[i].ulValueLen);
	    if (attr[i].pValue == NULL) {
		int j;
		for (j= 0; j < i; j++) { PORT_Free(attr[j].pValue); }
		return CKR_HOST_MEMORY;
	    }
	}
    }

    /*
     * finally get the results.
     */
    crv = PK11_GETTAB(slot)->C_GetAttributeValue(slot->session,obj,attr,count);
    if (crv != CKR_OK) {
	if (arena) {
	    PORT_ArenaRelease(arena,mark);
	} else {
	    for (i= 0; i < count; i++) { PORT_Free(attr[i].pValue); }
	}
    }
    return crv;
}

/*
 * build a cert nickname based on the token name and the label of the 
 * certificate If the label in NULL, build a label based on the ID.
 */
static int toHex(int x) { return (x < 10) ? (x+'0') : (x+'a'-10); }
#define MAX_CERT_ID 4
#define DEFAULT_STRING "Cert ID "
static char *
pk11_buildNickname(PK11SlotInfo *slot,CK_ATTRIBUTE *cert_label,
			CK_ATTRIBUTE *key_label, CK_ATTRIBUTE *cert_id)
{
    int prefixLen = PORT_Strlen(slot->token_name);
    int suffixLen = 0;
    char *suffix = NULL;
    char buildNew[sizeof(DEFAULT_STRING)+MAX_CERT_ID*2];
    char *next,*nickname;

    if (slot->isInternal) {
	return NULL;
    }

    if ((cert_label) && (cert_label->pValue)) {
	suffixLen = cert_label->ulValueLen;
	suffix = cert_label->pValue;
    } else if (key_label && (key_label->pValue)) {
	suffixLen = key_label->ulValueLen;
	suffix = key_label->pValue;
    } else if ((cert_id) && cert_id->pValue) {
	int i,first = cert_id->ulValueLen - MAX_CERT_ID;
	int offset = sizeof(DEFAULT_STRING);
	char *idValue = (char *)cert_id->pValue;

	PORT_Memcpy(buildNew,DEFAULT_STRING,sizeof(DEFAULT_STRING)-1);
	next = buildNew + offset;
	if (first < 0) first = 0;
	for (i=first; i < (int) cert_id->ulValueLen; i++) {
		*next++ = toHex((idValue[i] >> 4) & 0xf);
		*next++ = toHex(idValue[i] & 0xf);
	}
	*next++ = 0;
	suffix = buildNew;
	suffixLen = PORT_Strlen(buildNew);
    } else {
	PORT_SetError( SEC_ERROR_LIBRARY_FAILURE );
	return NULL;
    }

    next = nickname = (char *)PORT_Alloc(prefixLen+1+suffixLen+1);
    if (nickname == NULL) return NULL;

    PORT_Memcpy(next,slot->token_name,prefixLen);
    next += prefixLen;
    *next++ = ':';
    PORT_Memcpy(next,suffix,suffixLen);
    next += suffixLen;
    *next++ = 0;
    return nickname;
}

/*
 * return the object handle that matches the template
 */
static CK_OBJECT_HANDLE
pk11_FindObjectByTemplate(PK11SlotInfo *slot,CK_ATTRIBUTE *template,int tsize)
{
    CK_OBJECT_HANDLE object;
    CK_RV crv;
    CK_ULONG objectCount;

    /*
     * issue the find
     */
    crv=PK11_GETTAB(slot)->C_FindObjectsInit(slot->session, template, tsize);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return CK_INVALID_KEY;
    }

    crv=PK11_GETTAB(slot)->C_FindObjects(slot->session,&object,1,&objectCount);
    PK11_GETTAB(slot)->C_FindObjectsFinal(slot->session);
    if ((crv != CKR_OK) || (objectCount < 1)) {
	/* sigh! shouldn't use SSL_ERROR... here */
	PORT_SetError( crv != CKR_OK ? PK11_MapError(crv) :
						  SSL_ERROR_NO_CERTIFICATE);
	return CK_INVALID_KEY;
    }

    /* blow up if the PKCS #11 module returns us and invalid object handle */
    XP_ASSERT(object != CK_INVALID_KEY);
    return object;
} 

/*
 * given a PKCS #11 object, match it's peer based on the KeyID. searchID
 * is typically a privateKey or a certificate while the peer is the opposite
 */
CK_OBJECT_HANDLE
PK11_MatchItem(PK11SlotInfo *slot, CK_OBJECT_HANDLE searchID,
				 		CK_OBJECT_CLASS matchclass)
{
    CK_ATTRIBUTE template[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    /* if you change the array, change the variable below as well */
    CK_ATTRIBUTE *keyclass = &template[1];
    int tsize = sizeof(template)/sizeof(template[0]);
    /* if you change the array, change the variable below as well */
    CK_OBJECT_HANDLE peerID;
    PRArenaPool *arena;
    CK_RV crv;

    /* now we need to create space for the public key */
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return CK_INVALID_KEY;

    crv = PK11_GetAttributes(arena,slot,searchID,template,tsize);
    if (crv != CKR_OK) {
	PORT_FreeArena(arena,PR_FALSE);
	PORT_SetError( PK11_MapError(crv) );
	return CK_INVALID_KEY;
    }

    /*
     * issue the find
     */
    *(CK_OBJECT_CLASS *)(keyclass->pValue) = matchclass;

    peerID = pk11_FindObjectByTemplate(slot,template,tsize);
    PORT_FreeArena(arena,PR_FALSE);

    return peerID;
}
     
/*
 * Build an CERTCertificate structure from a PKCS#11 object ID.... certID
 * Must be a CertObject. This code does not explicitly checks that.
 */
CERTCertificate *
PK11_MakeCertFromHandle(PK11SlotInfo *slot,CK_OBJECT_HANDLE certID,
						CK_ATTRIBUTE *privateLabel)
{
    CK_ATTRIBUTE certTemp[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_VALUE, NULL, 0 },
	{ CKA_LABEL, NULL, 0 }
    };
    CK_ATTRIBUTE *id = &certTemp[0];
    CK_ATTRIBUTE *certDER = &certTemp[1];
    CK_ATTRIBUTE *label = &certTemp[2];
    SECItem derCert;
    int csize = sizeof(certTemp)/sizeof(certTemp[0]);
    PRArenaPool *arena;
    CK_RV crv;
    char *nickname;
    CERTCertificate *cert;
    CERTCertTrust *trust;

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;
    /*
     * grab the der encoding
     */
    crv = PK11_GetAttributes(arena,slot,certID,certTemp,csize);
    if (crv != CKR_OK) {
	PORT_FreeArena(arena,PR_FALSE);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    /*
     * build a certificate out of it
     */
    derCert.data = certDER->pValue;
    derCert.len = certDER->ulValueLen;

    /* figure out the nickname.... */
    nickname = pk11_buildNickname(slot,label,privateLabel,id);
    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &derCert, nickname,
							PR_FALSE, PR_TRUE);
    PORT_FreeArena(arena,PR_FALSE);
    if (cert == NULL) {
	if (nickname) PORT_Free(nickname);
	return cert;
    }
	
    if (nickname) {
	if (cert->nickname != NULL) {
		cert->dbnickname = cert->nickname;
	} 
	cert->nickname = PORT_ArenaStrdup(cert->arena,nickname);
	PORT_Free(nickname);
    }

    /* remember where this cert came from.... If we have just looked
     * it up from the database and it already has a slot, don't add a new
     * one. */
    if (cert->slot == NULL) {
	cert->slot = PK11_ReferenceSlot(slot);
	cert->pkcs11ID = certID;
    }

    if (cert->trust == NULL) {
	trust = PORT_ArenaAlloc(cert->arena, sizeof(CERTCertTrust));
	if (trust == NULL) {
	    CERT_DestroyCertificate(cert);
	    return NULL;
	}

	PORT_Memset(trust,0, sizeof(CERTCertTrust));
	cert->trust = trust;
    } else  {
	trust = cert->trust;
    }

    if (PK11_MatchItem(slot, certID ,CKO_PRIVATE_KEY) != CK_INVALID_KEY) {
	trust->sslFlags |= CERTDB_USER;
	/* XXXX check for signing only here XXXXX */
	trust->emailFlags |= CERTDB_USER;
    }
    return cert;
}
	
/*
 * Build get a certificate from a private key
 */
CERTCertificate *
PK11_GetCertFromPrivateKey(SECKEYPrivateKey *privKey)
{
    PK11SlotInfo *slot = privKey->pkcs11Slot;
    CK_OBJECT_HANDLE certID = 
		PK11_MatchItem(slot,privKey->pkcs11ID,CKO_CERTIFICATE);
    SECStatus rv;
    CERTCertificate *cert;

    if (certID == CK_INVALID_KEY) {
	/* couldn't find it on the card, look in our data base */
	SECItem derSubject;

	rv = PK11_ReadAttribute(slot, privKey->pkcs11ID, CKA_SUBJECT, NULL,
					&derSubject);
	if (rv != SECSuccess) {
	    PORT_SetError(SSL_ERROR_NO_CERTIFICATE);
	    return NULL;
	}

	cert = CERT_FindCertByName(CERT_GetDefaultCertDB(),&derSubject);
	PORT_Free(derSubject.data);
	return cert;
    }
    cert = PK11_MakeCertFromHandle(slot,certID,NULL);
    return (cert);

}

/*
 * count the number of objects that match the template.
 */
int
PK11_NumberObjectsFor(PK11SlotInfo *slot, CK_ATTRIBUTE *findTemplate, 
							int templateCount)
{
    CK_OBJECT_HANDLE objID[PK11_SEARCH_CHUNKSIZE];
    int object_count = 0;
    CK_ULONG returned_count = 0;
    CK_RV crv;

    crv = PK11_GETTAB(slot)->C_FindObjectsInit(slot->session,
					findTemplate, templateCount);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return 0;
    }

    /*
     * collect all the Matching Objects
     */
    do {
    	crv = PK11_GETTAB(slot)->C_FindObjects(slot->session,
				objID,PK11_SEARCH_CHUNKSIZE,&returned_count);
	if (crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    break;
    	}
	object_count += returned_count;
    } while (returned_count == PK11_SEARCH_CHUNKSIZE);

    PK11_GETTAB(slot)->C_FindObjectsFinal(slot->session);
    return object_count;
}

/*
 * cert callback structure
 */
typedef struct pk11CertCallbackStr {
	SECStatus(* callback)(CERTCertificate*,void *);
	void *callbackArg;
} pk11CertCallback;

/*
 * callback to map object handles to certificate structures.
 */
SECStatus
pk11_DoCerts(PK11SlotInfo *slot, CK_OBJECT_HANDLE certID, void *arg)
{
    CERTCertificate *cert;
    pk11CertCallback *certcb = (pk11CertCallback *) arg;

    cert = PK11_MakeCertFromHandle(slot, certID, NULL);

    if (cert == NULL) {
	return SECFailure;
    }

    if (certcb && (certcb->callback)) {
	(*certcb->callback)(cert,certcb->callbackArg);
    }

    CERT_DestroyCertificate(cert);
	    
    return SECSuccess;
}


/*
 * key call back structure.
 */
typedef struct pk11KeyCallbackStr {
	SECStatus (* callback)(SECKEYPrivateKey *,void *);
	void *callbackArg;
	void *wincx;
} pk11KeyCallback;

/*
 * callback to map Object Handles to Private Keys;
 */
SECStatus
pk11_DoKeys(PK11SlotInfo *slot, CK_OBJECT_HANDLE keyHandle, void *arg)
{
    SECStatus rv = SECSuccess;
    SECKEYPrivateKey *privKey;
    pk11KeyCallback *keycb = (pk11KeyCallback *) arg;

    privKey = PK11_MakePrivKey(slot,nullKey,PR_TRUE,keyHandle,keycb->wincx);

    if (privKey == NULL) {
	return SECFailure;
    }

    if (keycb && (keycb->callback)) {
	rv = (*keycb->callback)(privKey,keycb->callbackArg);
    }

    SECKEY_DestroyPrivateKey(privKey);	    
    return rv;
}


/* Traverse slots callback */
typedef struct pk11TraverseSlotStr {
    SECStatus (*callback)(PK11SlotInfo *,CK_OBJECT_HANDLE, void *);
    void *callbackArg;
    CK_ATTRIBUTE *findTemplate;
    int templateCount;
} pk11TraverseSlotCert;

/*
 * Extract all the certs on a card from a slot.
 */
SECStatus
PK11_TraverseSlot(PK11SlotInfo *slot, void *arg)
{
    int i;
    CK_OBJECT_HANDLE *objID = NULL;
    int object_count = 0;
    CK_ULONG returned_count = 0;
    CK_RV crv;
    pk11TraverseSlotCert *slotcb = (pk11TraverseSlotCert *) arg;

    crv = PK11_GETTAB(slot)->C_FindObjectsInit(slot->session,
			slotcb->findTemplate, slotcb->templateCount);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }

    /*
     * collect all the Matching Objects
     */
    do {
	CK_OBJECT_HANDLE *oldObjID = objID;

	if (objID == NULL) {
    	    objID = (CK_OBJECT_HANDLE *) PORT_Alloc(sizeof(CK_OBJECT_HANDLE)*
				(object_count+ PK11_SEARCH_CHUNKSIZE));
	} else {
    	    objID = (CK_OBJECT_HANDLE *) PORT_Realloc(objID,
		sizeof(CK_OBJECT_HANDLE)*(object_count+PK11_SEARCH_CHUNKSIZE));
	}

	if (objID == NULL) {
	    if (oldObjID) PORT_Free(oldObjID);
	    break;
	}
    	crv = PK11_GETTAB(slot)->C_FindObjects(slot->session,
		&objID[object_count],PK11_SEARCH_CHUNKSIZE,&returned_count);
	if (crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    PORT_Free(objID);
	    objID = NULL;
	    break;
    	}
	object_count += returned_count;
    } while (returned_count == PK11_SEARCH_CHUNKSIZE);

    PK11_GETTAB(slot)->C_FindObjectsFinal(slot->session);
    if (objID == NULL) {
	return SECFailure;
    }

    /*Actually this isn't a failure... there just were no objs to be found*/
    if (object_count == 0) {
	PORT_Free(objID);
	return SECSuccess;
    }

    for (i=0; i < object_count; i++) {
	(*slotcb->callback)(slot,objID[i],slotcb->callbackArg);
    }
    PORT_Free(objID);
    return SECSuccess;
}

typedef struct pk11OldCertCallbackStr {
	SECStatus(* callback)(CERTCertificate*,SECItem *,void *);
	void *callbackArg;
} pk11OldCertCallback;

static SECStatus
pk11_SaveCert(CERTCertificate *cert, void *arg)
{
    pk11OldCertCallback *certcb = (pk11OldCertCallback *)arg;
    SECStatus rv = SECSuccess;

    cert->slot->cert_array[cert->slot->cert_count] = CERT_DupCertificate(cert);
    if (cert->slot->cert_array[cert->slot->cert_count] == NULL) {
	return SECFailure;
    }
    cert->slot->cert_count++;

    if (certcb->callback) {
	rv = (*certcb->callback)(cert, NULL, certcb->callbackArg);
    }
    return rv;
}


/*
 * Extract all the certs on a card from a slot.
 */
static SECStatus
pk11_ExtractCertsFromSlot(PK11SlotInfo *slot, void *arg)
{
    pk11TraverseSlotCert *slotcb = (pk11TraverseSlotCert *) arg;
    int object_count,i;

    if (slot->cert_array) {
	for (i=0; i < slot->cert_count; i++) {
	    CERT_DestroyCertificate(slot->cert_array[i]);
	}
	PORT_Free(slot->cert_array);
	slot->cert_array = NULL;
	slot->cert_count = 0;
    }

    object_count = PK11_NumberObjectsFor(slot,slotcb->findTemplate, 
						slotcb->templateCount);

    /*Actually this isn't a failure... there just were no certs to be found*/
    if (object_count == 0) {
	return SECSuccess;
    }

    slot->cert_array = (CERTCertificate **)
			PORT_Alloc(sizeof(CERTCertificate *)*object_count);
    if (slot->cert_array == NULL) {
	return SECFailure;
    }
    slot->cert_count = 0;
    PK11_TraverseSlot(slot,arg);

    return SECSuccess;
}

/*
 * Extract all the certs on a card from a slot.
 */
static SECStatus
pk11_TraverseAllSlots(PRBool includeInternal,
	SECStatus (*callback)(PK11SlotInfo *,void *),void *arg,void *wincx) {

    PK11SlotList *list;
    PK11SlotListElement *le;
    SECStatus rv;

    /* get them all! */
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE);
    if (list == NULL) return SECFailure;

    /* look at each slot and authenticate as necessary */
    for (le = list->head ; le; le = le->next) {
	/* don't nab internal slots */
	if ((!includeInternal) && le->slot->isInternal == PR_TRUE) {
	    continue;
	}
	if (le->slot->needLogin && !PK11_IsLoggedIn(le->slot)) {
	    /* card is weirded out, remove it from the list */
	    if (le->slot->session == CK_INVALID_SESSION) {
		continue;
	    }
	    /* get the password. This can drop out of the while loop
	     * for the following reasons:
	     *  	(1) the user refused to enter a password. 
	     *			(continue to next card on the chain)
	     *	(2) the token user password is disabled [usually due to
	     *	   too many failed authentication attempts.
	     *			(continue to next card on the chain)
	     *	(3) the password was successful.
	     */
	    rv = PK11_DoPassword(le->slot,wincx);
	    if (rv != SECSuccess) {
		continue;
	    }
	    rv = pk11_CheckVerifyTest(le->slot);
	    if (rv != SECSuccess) {
		continue;
	    }
	}
	(*callback)(le->slot,arg);
    }

    PK11_FreeSlotList(list);

    return SECSuccess;
}

/*
 * Extract all the certs on a card from a slot.
 */
SECStatus
PK11_TraverseSlotCerts(SECStatus(* callback)(CERTCertificate*,SECItem *,void *),
						void *arg, void *wincx) {
    pk11OldCertCallback caller;
    pk11CertCallback saver;
    pk11TraverseSlotCert creater;
    CK_ATTRIBUTE template;
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;

    PK11_SETATTRS(&template, CKA_CLASS, &certClass, sizeof(certClass));

    caller.callback = callback;
    caller.callbackArg = arg;
    saver.callback = pk11_SaveCert;
    saver.callbackArg = (void *) & caller;
    creater.callback = pk11_DoCerts;
    creater.callbackArg = (void *) & saver;
    creater.findTemplate = &template;
    creater.templateCount = 1;

    return pk11_TraverseAllSlots(PR_FALSE, pk11_ExtractCertsFromSlot, 
							&creater, wincx);
}

CK_OBJECT_HANDLE
PK11_FindObjectFromNickname(char *nickname,PK11SlotInfo **slotptr,
				CK_OBJECT_CLASS objclass, void *wincx) {
    char *tokenName;
    char *delimit;
    PK11SlotInfo *slot;
    CK_OBJECT_HANDLE objID;
    CK_ATTRIBUTE findTemplate[] = {
	 { CKA_LABEL, NULL, 0},
	 { CKA_CLASS, NULL, 0},
    };
    int findCount = sizeof(findTemplate)/sizeof(findTemplate[0]);
    SECMODModuleList *mlp;
    SECMODModuleList *modules = SECMOD_GetDefaultModuleList();
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
    int i;
    SECStatus rv;
    PK11_SETATTRS(&findTemplate[1], CKA_CLASS, &objclass, sizeof(objclass));

    *slotptr = slot = NULL;
    /* first find the slot associated with this nickname */
    if ((delimit = PORT_Strchr(nickname,':')) != NULL) {
	int len = delimit - nickname;
	tokenName = PORT_Alloc(len+1);
	PORT_Memcpy(tokenName,nickname,len);
	tokenName[len] = 0;

	/* work through all the slots */
	SECMOD_GetReadLock(moduleLock);
	for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	    for (i=0; i < mlp->module->slotCount; i++) {
		slot = mlp->module->slots[i];
		if (PK11_IsPresent(slot)) {
		    if (PORT_Strcmp(slot->token_name,tokenName) == 0) {
			*slotptr = PK11_ReferenceSlot(slot);
			break;
		    }
		}
	    }
	    if (*slotptr != NULL) break;
	}
	SECMOD_ReleaseReadLock(moduleLock);
        PORT_Free(tokenName);
	nickname = delimit+1;
	if (*slotptr == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE);
	    return CK_INVALID_KEY;
	}
    } else {
	*slotptr = slot = PK11_GetInternalKeySlot();
    }

    if (PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
	rv = PK11_DoPassword(slot,wincx);
	if (rv != SECSuccess) return CK_INVALID_KEY;
    }

    findTemplate[0].pValue = nickname;
    findTemplate[0].ulValueLen = PORT_Strlen(nickname);
    objID = pk11_FindObjectByTemplate(slot,findTemplate,findCount);
    if (objID == CK_INVALID_KEY) {
	/* Sigh PKCS #11 isn't clear on whether or not the NULL is
	 * stored in the template.... try the find again with the
	 * full null terminated string. */
    	findTemplate[0].ulValueLen += 1;
        objID = pk11_FindObjectByTemplate(slot,findTemplate,findCount);
	if (objID == CK_INVALID_KEY) {
	    /* Well that's the best we can do. It's just not here */
	    /* what about faked nicknames? */
	    PK11_FreeSlot(slot);
	    *slotptr = NULL;
	}
    }

    return objID;
}

   
CERTCertificate *
PK11_FindCertFromNickname(char *nickname, void *wincx) {
    PK11SlotInfo *slot;
    CK_OBJECT_HANDLE certID = PK11_FindObjectFromNickname(nickname,&slot,
				 		CKO_CERTIFICATE, wincx);
    CERTCertificate *cert;

    if (certID == CK_INVALID_KEY) return NULL;
    cert = PK11_MakeCertFromHandle(slot,certID,NULL);
    PK11_FreeSlot(slot);
    return cert;
}

/*
 * extract a key ID for a certificate...
 * DANGER, DANGER, WE call this function from PKCS11.c If we ever use
 * pkcs11 to extract the public key (we currently do not), this will break.
 */
SECItem *
PK11_GetPubIndexKeyID(CERTCertificate *cert) {
    SECKEYPublicKey *pubk;
    SECItem *newItem = NULL;

    pubk = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if (pubk == NULL) return NULL;

    switch (pubk->keyType) {
    case rsaKey:
	newItem = SECITEM_DupItem(&pubk->u.rsa.modulus);
	break;
    case dsaKey:
        newItem = SECITEM_DupItem(&pubk->u.dsa.publicValue);
	break;
    case dhKey:
    case fortezzaKey:
    default:
	newItem = NULL; /* Fortezza Fix later... */
    }
    SECKEY_DestroyPublicKey(pubk);
    /* make hash of it */
    return newItem;
}

SECItem *
pk11_mkcertKeyID(CERTCertificate *cert) {
    SECItem *pubKeyData = PK11_GetPubIndexKeyID(cert) ;
#ifdef notdef
    PK11Context *context;
    SECItem *certCKA_ID;
    SECStatus rv;

    if (pubKeyData == NULL) return NULL;

    context = PK11_CreateDigestContext(SEC_OID_SHA1);
    if (context == NULL) {
    	SECITEM_FreeItem(pubKeyData,PR_TRUE);
	return NULL;
    }

    rv = PK11_DigestBegin(context);
    if (rv == SECSuccess) {
    	rv = PK11_DigestOp(context,pubKeyData->data,pubkeyData->len);
    }
    SECITEM_FreeItem(pubKeyData,PR_TRUE);
    if (rv != SECSucces) {
	PK11_DestroyContext(context);
	return NULL;
    }

    certCKA_ID = (SecItem *)PORT_Alloc(sizeof(SECItem));
    if (certCKA_ID == NULL) {
	PK11_DestroyContext(context);
	return NULL;
    }

    certCKA_ID->len = SHA1_LENGTH;
    certCKA_ID->data = PORT_Alloc(certCKA_ID->len);
    if (certCKA_ID->data == NULL) {
	PORT_Free(certCKA_ID);
	PK11_DestroyContext(context);
        return NULL;
    }

    rv = PK11_DigestFinal(context,certCKA_ID->data,certCKA_ID->len);
    PK11_DestroyContext(context);
    if (rv != SECSuccess) {
    	SECITEM_FreeItem(certCKA_ID,PR_TRUE);
	return NULL;
    }

    return certCKA_ID;
#else
    return pubKeyData;
#endif
}

/*
 * Write the cert into the token.
 */
SECStatus
PK11_ImportCert(PK11SlotInfo *slot, CERTCertificate *cert, 
				CK_OBJECT_HANDLE key, char *nickname) {
    int len = PORT_Strlen(nickname);
    SECItem *keyID = pk11_mkcertKeyID(cert);
    CK_ATTRIBUTE keyAttrs[] = {
	{ CKA_LABEL, NULL, 0},
	{ CKA_SUBJECT, NULL, 0},
    };
    CK_OBJECT_CLASS certc = CKO_CERTIFICATE;
    CK_CERTIFICATE_TYPE certType = CKC_X_509;
    CK_OBJECT_HANDLE certID;
    CK_SESSION_HANDLE rwsession;
    CK_BBOOL cktrue = TRUE;
    SECStatus rv = SECFailure;
    CK_ATTRIBUTE certAttrs[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_LABEL, NULL, 0},
	{ CKA_CLASS,  NULL, 0},
	{ CKA_TOKEN,  NULL, 0},
	{ CKA_CERTIFICATE_TYPE, NULL, 0},
	{ CKA_SUBJECT, NULL, 0},
	{ CKA_ISSUER, NULL, 0},
	{ CKA_SERIAL_NUMBER,  NULL, 0},
	{ CKA_VALUE,  NULL, 0},
    };
    int certCount = sizeof(certAttrs)/sizeof(certAttrs[0]);
    CK_ATTRIBUTE *attrs;
    CK_RV crv;

    if (keyID == NULL) {
	PORT_SetError(SEC_ERROR_ADDING_CERT);
	return rv;
    }
    attrs = certAttrs;
    PK11_SETATTRS(attrs,CKA_ID, keyID->data, keyID->len); attrs++;
    PK11_SETATTRS(attrs,CKA_LABEL, nickname, len ); attrs++;
    PK11_SETATTRS(attrs,CKA_CLASS, &certc, sizeof(certc) ); attrs++;
    PK11_SETATTRS(attrs,CKA_TOKEN, &cktrue, sizeof(cktrue) ); attrs++;
    PK11_SETATTRS(attrs,CKA_CERTIFICATE_TYPE, &certType,
						sizeof(certType)); attrs++;
    PK11_SETATTRS(attrs,CKA_SUBJECT, cert->derSubject.data,
					 cert->derSubject.len ); attrs++;
    PK11_SETATTRS(attrs,CKA_ISSUER, cert->derIssuer.data,
					 cert->derIssuer.len ); attrs++;
    PK11_SETATTRS(attrs,CKA_SERIAL_NUMBER, cert->serialNumber.data,
					 cert->serialNumber.len); attrs++;
    PK11_SETATTRS(attrs,CKA_VALUE, cert->derCert.data, cert->derCert.len);
    attrs = keyAttrs;
    PK11_SETATTRS(attrs,CKA_LABEL, nickname, len ); attrs++;
    PK11_SETATTRS(attrs,CKA_SUBJECT, cert->derSubject.data,
					 cert->derSubject.len ); 

    rwsession = PK11_GetRWSession(slot);

    crv = PK11_GETTAB(slot)->C_SetAttributeValue(rwsession,key,keyAttrs,2);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	goto done;
    }

    crv = PK11_GETTAB(slot)->
			C_CreateObject(rwsession,certAttrs,certCount,&certID);
    if (crv == CKR_OK) {
	rv = SECSuccess;
    } else {
	PORT_SetError( PK11_MapError(crv) );
    }

done:
    SECITEM_FreeItem(keyID,PR_TRUE);
    PK11_RestoreROSession(slot,rwsession);
    return rv;

}

/*
 * return the private key From a given Cert
 */
SECKEYPrivateKey *
PK11_FindPrivateKeyFromCert(PK11SlotInfo *slot, CERTCertificate *cert,
								 void *wincx) {
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE template[] = {
	{ CKA_VALUE, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    /* if you change the array, change the variable below as well */
    int tsize = sizeof(template)/sizeof(template[0]);
    CK_OBJECT_HANDLE certh;
    CK_OBJECT_HANDLE keyh;
    CK_ATTRIBUTE *attrs = template;
    SECStatus rv;

    PK11_SETATTRS(attrs, CKA_VALUE, cert->derCert.data, 
						cert->derCert.len); attrs++;
    PK11_SETATTRS(attrs, CKA_CLASS, &certClass, sizeof(certClass));

    /*
     * issue the find
     */
    if (PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
	rv = PK11_DoPassword(slot,wincx);
	if (rv != SECSuccess) return NULL;
    }

    if (cert->slot == slot) {
	certh = cert->pkcs11ID;
    } else {
    	certh = pk11_FindObjectByTemplate(slot,template,tsize);
    }
    if (certh == CK_INVALID_KEY) {
	return NULL;
    }
    keyh = PK11_MatchItem(slot,certh,CKO_PRIVATE_KEY);
    if (keyh == CK_INVALID_KEY) { return NULL; }
    return PK11_MakePrivKey(slot, nullKey, PR_TRUE, keyh, wincx);
} 


/*
 * return the private key with the given ID
 */
static CK_OBJECT_HANDLE
pk11_FindPrivateKeyFromCertID(PK11SlotInfo *slot, SECItem *keyID)  {
    CK_OBJECT_CLASS privKey = CKO_PRIVATE_KEY;
    CK_ATTRIBUTE template[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_CLASS, NULL, 0 },
    };
    /* if you change the array, change the variable below as well */
    int tsize = sizeof(template)/sizeof(template[0]);
    CK_ATTRIBUTE *attrs = template;

    PK11_SETATTRS(attrs, CKA_ID, keyID->data, keyID->len ); attrs++;
    PK11_SETATTRS(attrs, CKA_CLASS, &privKey, sizeof(privKey));

    return pk11_FindObjectByTemplate(slot,template,tsize);
} 

/*
 * import a cert for a private key we have already generated. Set the label
 * on both to be the nickname.
 */
PK11SlotInfo *
PK11_KeyForCertExists(CERTCertificate *cert, CK_OBJECT_HANDLE *keyPtr, 
								void *wincx) {
    PK11SlotList *list;
    PK11SlotListElement *le;
    PK11SlotList *needLoginList = PK11_NewSlotList();
    SECItem *keyID;
    CK_OBJECT_HANDLE key;
    PK11SlotInfo *slot = NULL;

    keyID = pk11_mkcertKeyID(cert);
    /* get them all! */
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE);
    if ((keyID == NULL) || (list == NULL) || (needLoginList == NULL)) {
	if (keyID) SECITEM_FreeItem(keyID,PR_TRUE);
	if (list) PK11_FreeSlotList(list);
	if (needLoginList) PK11_FreeSlotList(needLoginList);
    	return NULL;
    }


    /* Look for the slot that holds the Key */
    for (le = list->head ; le; le = le->next) {
	if (le->slot->needLogin && !PK11_IsLoggedIn(le->slot)) {
	    /* card is weirded out, remove it from the list */
	    if (le->slot->session == CK_INVALID_SESSION) {
		continue;
	    }
	    PK11_AddSlotToList(needLoginList,le->slot);
	    continue;
	}
	key = pk11_FindPrivateKeyFromCertID(le->slot,keyID);
	if (key != CK_INVALID_KEY) {
	    slot = PK11_ReferenceSlot(le->slot);
	    if (keyPtr) *keyPtr = key;
	    break;
	}
    }

    if (slot == NULL) {
	for (le = needLoginList->head; le; le = le->next) {
	    SECStatus rv = PK11_DoPassword(le->slot,wincx);
	    if (rv != SECSuccess) {
		continue;
	    }
	    rv = pk11_CheckVerifyTest(le->slot);
	    if (rv != SECSuccess) {
		continue;
	    }
	    key = pk11_FindPrivateKeyFromCertID(le->slot,keyID);
	    if (key != CK_INVALID_KEY) {
		slot = PK11_ReferenceSlot(le->slot);
	        if (keyPtr) *keyPtr = key;
		break;
	    }
	}
    }

    SECITEM_FreeItem(keyID,PR_TRUE);
    PK11_FreeSlotList(list);
    PK11_FreeSlotList(needLoginList);
    return slot;

}

PK11SlotInfo *
PK11_ImportCertForKey(CERTCertificate *cert, char *nickname,void *wincx) {
    PK11SlotInfo *slot = NULL;
    CK_OBJECT_HANDLE key;

    slot = PK11_KeyForCertExists(cert,&key,wincx);

    if (slot) {
	if (PK11_ImportCert(slot,cert,key,nickname) != SECSuccess) {
	    PK11_FreeSlot(slot);
	    slot = NULL;
	}
    } else {
	PORT_SetError(SEC_ERROR_ADDING_CERT);
    }

    return slot;
}

static CK_OBJECT_HANDLE
pk11_FindCertObjectByTemplate(PK11SlotInfo **slotPtr, 
		CK_ATTRIBUTE *searchTemplate, int count, void *wincx) {
    PK11SlotList *list;
    PK11SlotListElement *le;
    PK11SlotList *needLoginList = PK11_NewSlotList();
    CK_OBJECT_HANDLE certHandle = CK_INVALID_KEY;
    PK11SlotInfo *slot = NULL;

    *slotPtr = NULL;

    /* get them all! */
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE);
    if ((list == NULL) || (needLoginList == NULL)) {
	if (list) PK11_FreeSlotList(list);
	if (needLoginList) PK11_FreeSlotList(needLoginList);
    	return CK_INVALID_KEY;
    }


    /* Look for the slot that holds the Key */
    for (le = list->head ; le; le = le->next) {
	if (le->slot->needLogin && !PK11_IsLoggedIn(le->slot)) {
	    /* card is weirded out, remove it from the list */
	    if (le->slot->session == CK_INVALID_SESSION) {
		continue;
	    }
	    PK11_AddSlotToList(needLoginList,le->slot);
	    continue;
	}
	certHandle = pk11_FindObjectByTemplate(le->slot,searchTemplate,count);
	if (certHandle != CK_INVALID_KEY) {
	    slot = PK11_ReferenceSlot(le->slot);
	    break;
	}
    }

    if (slot == NULL) {
	for (le = needLoginList->head; le; le = le->next) {
	    SECStatus rv = PK11_DoPassword(le->slot,wincx);
	    if (rv != SECSuccess) {
		continue;
	    }
	    rv = pk11_CheckVerifyTest(le->slot);
	    if (rv != SECSuccess) {
		continue;
	    }
	    certHandle = pk11_FindObjectByTemplate(le->slot,
							searchTemplate,count);
	    if (certHandle != CK_INVALID_KEY) {
		slot = PK11_ReferenceSlot(le->slot);
		break;
	    }
	}
    }

    PK11_FreeSlotList(list);
    PK11_FreeSlotList(needLoginList);

    if (slot == NULL) {
	return CK_INVALID_KEY;
    }
    *slotPtr = slot;
    return certHandle;
}

CERTCertificate *
PK11_FindCertByIssuerAndSN(PK11SlotInfo **slotPtr, CERTIssuerAndSN *issuerSN,
							 void *wincx) {
    CK_OBJECT_HANDLE certHandle;
    CERTCertificate *cert = NULL;
    CK_ATTRIBUTE searchTemplate[] = {
	{ CKA_ISSUER, NULL, 0 },
	{ CKA_SERIAL_NUMBER, NULL, 0}
    };
    int count = sizeof(searchTemplate)/sizeof(CK_ATTRIBUTE);
    CK_ATTRIBUTE *attrs = searchTemplate;

    PK11_SETATTRS(attrs, CKA_ISSUER, issuerSN->derIssuer.data, 
					issuerSN->derIssuer.len); attrs++;
    PK11_SETATTRS(attrs, CKA_SERIAL_NUMBER, issuerSN->serialNumber.data, 
						issuerSN->serialNumber.len);

    certHandle = pk11_FindCertObjectByTemplate
					(slotPtr,searchTemplate,count,wincx);
    if (certHandle == CK_INVALID_KEY) {
	return NULL;
    }
    cert = PK11_MakeCertFromHandle(*slotPtr,certHandle,NULL);
    if (cert == NULL) {
	PK11_FreeSlot(*slotPtr);
	return NULL;
    }
    return cert;
}

CK_OBJECT_HANDLE
PK11_FindObjectForCert(CERTCertificate *cert, void *wincx, PK11SlotInfo **pSlot)
{
    CK_OBJECT_HANDLE certHandle;
    CK_ATTRIBUTE searchTemplate	= { CKA_VALUE, NULL, 0 };
    SECStatus rv;
    
    PK11_SETATTRS(&searchTemplate, CKA_VALUE, cert->derCert.data,
		  cert->derCert.len);

    if (cert->slot) {
	if (PK11_NeedLogin(cert->slot) && !PK11_IsLoggedIn(cert->slot)) {
	    rv = PK11_DoPassword(cert->slot,wincx);
	    if (rv != SECSuccess) return CK_INVALID_KEY;
	}
	*pSlot = PK11_ReferenceSlot(cert->slot);
	return cert->pkcs11ID;
    }

    certHandle = pk11_FindCertObjectByTemplate(pSlot,&searchTemplate,1,wincx);
    if (certHandle != CK_INVALID_KEY) {
	cert->slot = PK11_ReferenceSlot(*pSlot);
	cert->pkcs11ID = certHandle;
    }

    return(certHandle);
}

SECKEYPrivateKey *
PK11_FindKeyByAnyCert(CERTCertificate *cert, void *wincx)
{
    CK_OBJECT_HANDLE certHandle;
    CK_OBJECT_HANDLE keyHandle;
    PK11SlotInfo *slot = NULL;
    SECKEYPrivateKey *privKey;

    certHandle = PK11_FindObjectForCert(cert, wincx, &slot);
    if (certHandle == CK_INVALID_KEY) {
	 return NULL;
    }
    keyHandle = PK11_MatchItem(slot,certHandle,CKO_PRIVATE_KEY);
    if (keyHandle == CK_INVALID_KEY) { 
	PK11_FreeSlot(slot);
	return NULL;
    }
    privKey =  PK11_MakePrivKey(slot, nullKey, PR_TRUE, keyHandle, wincx);
    PK11_FreeSlot(slot);
    return privKey;
}

SECKEYPrivateKey *
PK11_FindKeyByKeyID(PK11SlotInfo *slot, SECItem *keyID, void *wincx)
{
    CK_OBJECT_HANDLE keyHandle;
    SECKEYPrivateKey *privKey;

    keyHandle = pk11_FindPrivateKeyFromCertID(slot, keyID);
    if (keyHandle == CK_INVALID_KEY) { 
	return NULL;
    }
    privKey =  PK11_MakePrivKey(slot, nullKey, PR_TRUE, keyHandle, wincx);
    return privKey;
}

/*
 * find the number of certs in the slot with the same subject name
 */
int
PK11_NumberCertsForCertSubject(CERTCertificate *cert)
{
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE template[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 },
    };
    CK_ATTRIBUTE *attr = template;
   int templateSize = sizeof(template)/sizeof(template[0]);

    PK11_SETATTRS(attr,CKA_CLASS, &certClass, sizeof(certClass)); attr++;
    PK11_SETATTRS(attr,CKA_SUBJECT,cert->derSubject.data,cert->derSubject.len);

    if ((cert->slot == NULL) || (cert->slot->isInternal)) {
	return 0;
    }

    return PK11_NumberObjectsFor(cert->slot,template,templateSize);
}

/*
 *  Walk all the certs with the same subject
 */
SECStatus
PK11_TraverseCertsForSubject(CERTCertificate *cert,
	SECStatus(* callback)(CERTCertificate*, void *), void *arg)
{
    pk11CertCallback caller;
    pk11TraverseSlotCert callarg;
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE template[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 },
    };
    CK_ATTRIBUTE *attr = template;
   int templateSize = sizeof(template)/sizeof(template[0]);

    PK11_SETATTRS(attr,CKA_CLASS, &certClass, sizeof(certClass)); attr++;
    PK11_SETATTRS(attr,CKA_SUBJECT,cert->derSubject.data,cert->derSubject.len);

    if ((cert->slot == NULL) || (cert->slot->isInternal)) {
	return SECSuccess;
    }
    caller.callback = callback;
    caller.callbackArg = arg;
    callarg.callback = pk11_DoCerts;
    callarg.callbackArg = (void *) & caller;
    callarg.findTemplate = template;
    callarg.templateCount = templateSize;
    
    return PK11_TraverseSlot(cert->slot, &callarg);
}

/*
 * return the certificate associated with a derCert 
 */
CERTCertificate *
PK11_FindCertFromDERCert(PK11SlotInfo *slot, CERTCertificate *cert,
								 void *wincx) {
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE template[] = {
	{ CKA_VALUE, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    /* if you change the array, change the variable below as well */
    int tsize = sizeof(template)/sizeof(template[0]);
    CK_OBJECT_HANDLE certh;
    CK_ATTRIBUTE *attrs = template;
    SECStatus rv;

    PK11_SETATTRS(attrs, CKA_VALUE, cert->derCert.data, 
						cert->derCert.len); attrs++;
    PK11_SETATTRS(attrs, CKA_CLASS, &certClass, sizeof(certClass));

    /*
     * issue the find
     */
    if (PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
	rv = PK11_DoPassword(slot,wincx);
	if (rv != SECSuccess) return NULL;
    }

    if (cert->slot == slot) {
	certh = cert->pkcs11ID;
    } else {
    	certh = pk11_FindObjectByTemplate(slot,template,tsize);
    }
    if (certh == CK_INVALID_KEY) {
	return NULL;
    }
    return PK11_MakeCertFromHandle(slot, certh, NULL);
} 

/*
 * return the certificate associated with a derCert 
 */
CERTCertificate *
PK11_FindCertFromDERSubjectAndNickname(PK11SlotInfo *slot, 
					CERTCertificate *cert, 
					char *nickname, void *wincx) {
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE template[] = {
	{ CKA_SUBJECT, NULL, 0 },
	{ CKA_LABEL, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    /* if you change the array, change the variable below as well */
    int tsize = sizeof(template)/sizeof(template[0]);
    CK_OBJECT_HANDLE certh;
    CK_ATTRIBUTE *attrs = template;
    SECStatus rv;

    PK11_SETATTRS(attrs, CKA_SUBJECT, cert->derSubject.data, 
						cert->derSubject.len); attrs++;
    PK11_SETATTRS(attrs, CKA_LABEL, nickname, PORT_Strlen(nickname));
    PK11_SETATTRS(attrs, CKA_CLASS, &certClass, sizeof(certClass));

    /*
     * issue the find
     */
    if (PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
	rv = PK11_DoPassword(slot,wincx);
	if (rv != SECSuccess) return NULL;
    }

    if (cert->slot == slot) {
	certh = cert->pkcs11ID;
    } else {
    	certh = pk11_FindObjectByTemplate(slot,template,tsize);
    }
    if (certh == CK_INVALID_KEY) {
	return NULL;
    }

    return PK11_MakeCertFromHandle(slot, certh, NULL);
}

/*
 * import a cert for a private key we have already generated. Set the label
 * on both to be the nickname.
 */
static CK_OBJECT_HANDLE 
pk11_findKeyObjectByDERCert(PK11SlotInfo *slot, CERTCertificate *cert, 
								void *wincx) {
    SECItem *keyID;
    CK_OBJECT_HANDLE key;

    if((slot == NULL) || (cert == NULL)) {
	return CK_INVALID_KEY;
    }

    keyID = pk11_mkcertKeyID(cert);
    if(keyID == NULL) {
	return CK_INVALID_KEY;
    }

    key = CK_INVALID_KEY;

    if(slot->needLogin && !PK11_IsLoggedIn(slot)) {
	SECStatus rv = PK11_DoPassword(slot, wincx);
	if(rv != SECSuccess) {
	    goto loser;
	}
    }

    key = pk11_FindPrivateKeyFromCertID(slot, keyID);

loser:
    SECITEM_ZfreeItem(keyID, PR_TRUE);
    return key;
}

SECKEYPrivateKey *
PK11_FindKeyByDERCert(PK11SlotInfo *slot, CERTCertificate *cert, 
								void *wincx) {
    CK_OBJECT_HANDLE keyHandle;

    if((slot == NULL) || (cert == NULL)) {
	return NULL;
    }

    keyHandle = pk11_findKeyObjectByDERCert(slot, cert, wincx);
    if(keyHandle == CK_INVALID_KEY) {
	return NULL;
    }

    return PK11_MakePrivKey(slot,nullKey,PR_TRUE,keyHandle,wincx);
}

SECStatus
PK11_ImportCertForKeyToSlot(PK11SlotInfo *slot, CERTCertificate *cert, 
						char *nickname, void *wincx) {
    CK_OBJECT_HANDLE keyHandle;

    if((slot == NULL) || (cert == NULL) || (nickname == NULL)) {
	return SECFailure;
    }

    keyHandle = pk11_findKeyObjectByDERCert(slot, cert, wincx);
    if(keyHandle == CK_INVALID_KEY) {
	return SECFailure;
    }

    return PK11_ImportCert(slot, cert, keyHandle, nickname);
}   
