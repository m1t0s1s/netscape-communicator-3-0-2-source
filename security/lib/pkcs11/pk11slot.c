/*
 * Deal with PKCS #11 Slots.
 */
#include "seccomon.h"
#include "secmod.h"
#include "prlock.h"
#include "secmodi.h"
#include "pkcs11t.h"
#include "pk11func.h"
#include "cert.h"
#include "key.h"
#include "secitem.h"
#include "secder.h"
#include "secasn1.h"
#include "secoid.h"
#include "rsa.h"
#include "secpkcs5.h"

/* diable reason codes (from allxpstr.h) */
extern int PK11_COULD_NOT_INIT_TOKEN;
extern int PK11_USER_SELECTED;
extern int PK11_TOKEN_VERIFY_FAILED;
extern int PK11_TOKEN_NOT_PRESENT;

/*************************************************************
 * local static and global data
 *************************************************************/

/*
 * This array helps parsing between names, mechanisms, and flags.
 * to make the config files understand more entries, add them
 * to this table. (XXX need function to export this table and it's size)
 */
PK11DefaultArrayEntry PK11_DefaultArray[] = {
	{ "RSA", SECMOD_RSA_FLAG, CKM_RSA_PKCS },
	{ "DSA", SECMOD_DSA_FLAG, CKM_DSA },
	{ "DH", SECMOD_DH_FLAG, CKM_DH_PKCS_DERIVE },
	{ "RC2", SECMOD_RC2_FLAG, CKM_RC2_CBC },
	{ "RC4", SECMOD_RC4_FLAG, CKM_RC4 },
	{ "DES", SECMOD_DES_FLAG, CKM_DES_CBC },
	{ "SKIPJACK", SECMOD_FORTEZZA_FLAG, CKM_SKIPJACK_CBC64 },
	{ "RANDOM", SECMOD_RANDOM_FLAG, CKM_FAKE_RANDOM },
};

/*
 * These  slotlists are lists of modules which provide default support for
 *  a given algorithm or mechanism.
 */
static PK11SlotList pk11_desSlotList,
    pk11_rc4SlotList,
    pk11_rc2SlotList,
    pk11_rsaSlotList,
    pk11_dsaSlotList,
    pk11_dhSlotList,
    pk11_idea,
    pk11_random;

/*
 * Tables used for Extended mechanism mapping (currently not used)
 */
typedef struct {
	CK_MECHANISM_TYPE keyGen;
	CK_KEY_TYPE keyType;
	CK_MECHANISM_TYPE type;
	int blockSize;
	int iv;
} pk11MechanismData;
	
static pk11MechanismData pk11_default = 
			{ 0, CKK_GENERIC_SECRET, CKM_FAKE_RANDOM, 8, 8 };
static pk11MechanismData *pk11_MechanismTable = NULL;
static int pk11_MechTableSize = 0;
static int pk11_MechEntrySize = 0;

/*
 * This structure keeps track of status that spans all the Slots.
 * NOTE: This is a global data structure. It semantics expect thread crosstalk
 * be very afraid when you see it used. 
 *  It's major purpose in life is to allow the user to log in one PER 
 * Tranaction, even if a transaction spans threads. The problem is the user
 * may have to enter a password one just to be able to look at the 
 * personalities/certificates (s)he can use. Then if Auth every is one, they
 * may have to enter the password again to use the card. See PK11_StartTransac
 * and PK11_EndTransaction.
 */
static struct PK11GlobalStruct {
   int transaction;
   PRBool inTransaction;
   char *(*getPass)(PK11SlotInfo *,void *);
} PK11_Global = { 1, PR_FALSE, NULL };
 
/************************************************************
 * Generic Slot List and Slot List element manipulations
 ************************************************************/

/*
 * allocate a new list 
 */
PK11SlotList *
PK11_NewSlotList(void) {
    PK11SlotList *list;
 
    list = (PK11SlotList *)PORT_Alloc(sizeof(PK11SlotList));
    if (list == NULL) return NULL;
    list->head = NULL;
#ifdef PKCS11_USE_THREADS
    list->lock = PR_NewLock(0);
    if (list->lock == NULL) {
	PORT_Free(list);
	return NULL;
    }
#else
    list->lock = NULL;
#endif

    return list;
}

/*
 * free a list element when all the references go away.
 */
static void
pk11_FreeListElement(PK11SlotList *list, PK11SlotListElement *le) {
    PRBool freeit = PR_FALSE;

    PK11_USE_THREADS(PR_Lock((PRLock *)(list->lock));)
    if (le->refCount-- == 1) {
	freeit = PR_TRUE;
    }
    PK11_USE_THREADS(PR_Unlock((PRLock *)(list->lock));)
    if (freeit) {
    	PK11_FreeSlot(le->slot);
	PORT_Free(le);
    }
}

/*
 * if we are freeing the list, we must be the only ones with a pointer
 * to the list.
 */
void
PK11_FreeSlotList(PK11SlotList *list) {
    PK11SlotListElement *le, *next ;
    if (list == NULL) return;

    for (le = list->head ; le; le = next) {
	next = le->next;
	pk11_FreeListElement(list,le);
    }
    PK11_USE_THREADS(PR_DestroyLock((PRLock *)(list->lock));)
    PORT_Free(list);
}

/*
 * add a slot to a list
 */
SECStatus
PK11_AddSlotToList(PK11SlotList *list,PK11SlotInfo *slot) {
    PK11SlotListElement *le;

    le = (PK11SlotListElement *) PORT_Alloc(sizeof(PK11SlotListElement));
    if (le == NULL) return SECFailure;

    le->slot = PK11_ReferenceSlot(slot);
    le->prev = NULL;
    le->refCount = 1;
    PK11_USE_THREADS(PR_Lock((PRLock *)(list->lock));)
    if (list->head) list->head->prev = le;
    le->next = list->head;
    list->head = le;
    PK11_USE_THREADS(PR_Unlock((PRLock *)(list->lock));)

    return SECSuccess;
}

/*
 * remove a slot entry from the list
 */
SECStatus
PK11_DeleteSlotFromList(PK11SlotList *list,PK11SlotListElement *le) {
    PK11_USE_THREADS(PR_Lock((PRLock *)(list->lock));)
    if (le->prev) le->prev->next = le->next; else list->head = le->next;
    if (le->next) le->next->prev = le->prev;
    le->next = le->prev = NULL;
    PK11_USE_THREADS(PR_Unlock((PRLock *)(list->lock));)
    pk11_FreeListElement(list,le);
    return SECSuccess;
}

/*
 * get an element safely from the list. This just makes sure that if
 * this element is deleted while we deal with it.
 */
PK11SlotListElement *
PK11_GetFirstSafe(PK11SlotList *list) {
    PK11SlotListElement *le;

    PK11_USE_THREADS(PR_Lock((PRLock *)(list->lock));)
    le = list->head;
    if (le != NULL) (le)->refCount++;
    PK11_USE_THREADS(PR_Unlock((PRLock *)(list->lock));)
    return le;
}

/*
 * NOTE: if this element gets deleted, we can no longer safely traverse using
 * it's pointers. We can either terminate the loop, or restart from the
 * beginning. This is controlled by the restart option.
 */
PK11SlotListElement *
PK11_GetNextSafe(PK11SlotList *list, PK11SlotListElement *le, PRBool restart) {
    PK11SlotListElement *new_le;
    PK11_USE_THREADS(PR_Lock((PRLock *)(list->lock));)
    new_le = le->next;
    if (le->next == NULL) {
	/* if the prev and next fields are NULL then either this element
	 * has been removed and we need to walk the list again (if restart
	 * is true) or this was the only element on the list */
	if ((le->prev == NULL) && restart &&  (list->head != le)) {
	    new_le = list->head;
	}
    }
    if (new_le) new_le->refCount++;
    PK11_USE_THREADS(PR_Unlock((PRLock *)(list->lock));)
    pk11_FreeListElement(list,le);
    return new_le;
}


/*
 * Find the element that holds this slot
 */
PK11SlotListElement *
PK11_FindSlotElement(PK11SlotList *list,PK11SlotInfo *slot) {
    PK11SlotListElement *le;

    for (le = PK11_GetFirstSafe(list); le;
			 	le = PK11_GetNextSafe(list,le,PR_TRUE)) {
	if (le->slot == slot) return le;
    }
    return NULL;
}

/************************************************************
 * Generic Slot Utilities
 ************************************************************/
/*
 * Create a new slot structure
 */
PK11SlotInfo *
PK11_NewSlotInfo(void) {
    PK11SlotInfo *slot;

    slot = (PK11SlotInfo *)PORT_Alloc(sizeof(PK11SlotInfo));
    if (slot == NULL) return slot;

#ifdef PKCS11_USE_THREADS
    slot->refLock = PR_NewLock(0);
    if (slot->refLock == NULL) {
	PORT_Free(slot);
	return slot;
    }
#else
    slot->refLock = NULL;
#endif
    slot->functionList = NULL;
    slot->needTest = PR_TRUE;
    slot->isPerm = PR_FALSE;
    slot->isInternal = PR_FALSE;
    slot->disabled = PR_FALSE;
    slot->reason = 0;
    slot->readOnly = PR_TRUE;
    slot->needLogin = PR_FALSE;
    slot->hasRandom = PR_FALSE;
    slot->flags = 0;
    slot->session = CK_INVALID_SESSION;
    slot->slotID = 0;
    slot->defaultFlags = 0;
    slot->refCount = 1;
    slot->askpw = 0;
    slot->timeout = 0;
    slot->cert_array = NULL;
    slot->cert_count = 0;
    slot->slot_name[0] = 0;
    slot->token_name[0] = 0;
    slot->module = NULL;
    return slot;
}
    
/* create a new reference to a slot so it doesn't go away */
PK11SlotInfo *
PK11_ReferenceSlot(PK11SlotInfo *slot) {
    PK11_USE_THREADS(PR_Lock(slot->refLock);)
    slot->refCount++;
    PK11_USE_THREADS(PR_Unlock(slot->refLock);)
    return slot;
}

/* Destroy all info on a slot we have built up */
void
PK11_DestroySlot(PK11SlotInfo *slot) {
   int i;

   /* first free up all the sessions on this slot */
   if (slot->functionList) {
	PK11_GETTAB(slot)->C_CloseAllSessions(slot->slotID);
   }

   /* now free up all the certificates we grabbed on this slot */
   if (slot->cert_array) {
        for (i=0; i < slot->cert_count; i++) {
            CERT_DestroyCertificate(slot->cert_array[i]);
        }
        PORT_Free(slot->cert_array);
        slot->cert_count = 0;
	slot->cert_array = NULL;
   }

   /* finally Tell our parent module that we've gone away so it can unload */
   if (slot->module) {
	SECMOD_SlotDestroyModule(slot->module,PR_TRUE);
   }

   /* ok, well not quit finally... now we free the memory */
   PORT_Free(slot);
}


/* We're all done with the slot, free it */
void
PK11_FreeSlot(PK11SlotInfo *slot) {
    PRBool freeit = PR_FALSE;

    PK11_USE_THREADS(PR_Lock(slot->refLock);)
    if (slot->refCount-- == 1) freeit = PR_TRUE;
    PK11_USE_THREADS(PR_Unlock(slot->refLock);)

    if (freeit) PK11_DestroySlot(slot);
}


/***********************************************************
 * Password Utilities
 ***********************************************************/
/*
 * Check the user's password. Log into the card if it's correct.
 * succede if the user is already logged in.
 */
SECStatus
pk11_CheckPassword(PK11SlotInfo *slot,char *pw) {
    int len = PORT_Strlen(pw);
    CK_RV crv;
    SECStatus rv;
    time_t currtime = XP_TIME();

    crv = PK11_GETTAB(slot)->C_Login(slot->session,CKU_USER,
						(unsigned char *)pw,len);
    switch (crv) {
    /* if we're already logged in, we're good to go */
    case CKR_OK:
	slot->authTransact = PK11_Global.transaction;
    case CKR_USER_ALREADY_LOGGED_IN:
	slot->authTime = currtime;
	rv = SECSuccess;
	break;
    case CKR_PIN_INCORRECT:
	rv = SECWouldBlock; /* everything else is ok, only the pin is bad */
	break;
    default:
	rv = SECFailure; /* some failure we can't fix by retrying */
    }
    return rv;
}

/*
 * Check the user's password. Logout before hand to make sure that
 * we are really checking the password.
 */
SECStatus
PK11_CheckUserPassword(PK11SlotInfo *slot,char *pw) {
    int len = PORT_Strlen(pw);
    CK_RV crv;
    SECStatus rv;
    time_t currtime = XP_TIME();

    /* force a logout */
    PK11_GETTAB(slot)->C_Logout(slot->session);

    crv = PK11_GETTAB(slot)->C_Login(slot->session,CKU_USER,
					(unsigned char *)pw,len);
    switch (crv) {
    /* if we're already logged in, we're good to go */
    case CKR_OK:
	slot->authTransact = PK11_Global.transaction;
	slot->authTime = currtime;
	rv = SECSuccess;
	break;
    case CKR_PIN_INCORRECT:
	rv = SECWouldBlock; /* everything else is ok, only the pin is bad */
	break;
    default:
	rv = SECFailure; /* some failure we can't fix by retrying */
    }
    return rv;
}

SECStatus
PK11_Logout(PK11SlotInfo *slot) {
    CK_RV crv;

    /* force a logout */
    crv = PK11_GETTAB(slot)->C_Logout(slot->session);
    return (crv == CKR_OK) ? SECSuccess : SECFailure;
}

/*
 * transaction stuff is for when we test for the need to do every
 * time auth to see if we already did it for this slot/transaction
 */
void PK11_StartAuthTransaction(void) {
PK11_Global.transaction++;
PK11_Global.inTransaction = PR_TRUE;
}

void PK11_EndAuthTransaction(void) {
PK11_Global.transaction++;
PK11_Global.inTransaction = PR_FALSE;
}

/*
 * before we do a private key op, we check to see if we
 * need to reauthenticate.
 */
void
PK11_HandlePasswordCheck(PK11SlotInfo *slot,void *wincx) {
    int askpw = slot->askpw;
    PRBool NeedAuth = PR_FALSE;

    if (!slot->needLogin) return;

    if ((slot->defaultFlags & PK11_OWN_PW_DEFAULTS) == 0) {
	PK11SlotInfo *def_slot = PK11_GetInternalKeySlot();

	askpw = def_slot->askpw;
    }

    /* timeouts are handled by isLoggedIn */
    if (!PK11_IsLoggedIn(slot)) {
	NeedAuth = PR_TRUE;
    } else if (slot->askpw == -1) {
	if (!PK11_Global.inTransaction	||
			 (PK11_Global.transaction != slot->authTransact)) {
	    PK11_GETTAB(slot)->C_Logout(slot->session);
	    NeedAuth = PR_TRUE;
	}
    }
    if (NeedAuth) PK11_DoPassword(slot,wincx);
}

void
PK11_SlotDBUpdate(PK11SlotInfo *slot) {
    SECMOD_AddPermDB(slot->module);
}

/*
 * set new askpw and timeout values
 */
void
PK11_SetSlotPWValues(PK11SlotInfo *slot,int askpw, int timeout) {
        slot->askpw = askpw;
        slot->timeout = timeout;
        slot->defaultFlags |= PK11_OWN_PW_DEFAULTS;
        PK11_SlotDBUpdate(slot);
}

/*
 * Get the askpw and timeout values for this slot
 */
void
PK11_GetSlotPWValues(PK11SlotInfo *slot,int *askpw, int *timeout) {
    *askpw = slot->askpw;
    *timeout = slot->timeout;

    if ((slot->defaultFlags & PK11_OWN_PW_DEFAULTS) == 0) {
	PK11SlotInfo *def_slot = PK11_GetInternalKeySlot();

	*askpw = def_slot->askpw;
	*timeout = def_slot->timeout;
    }
}

/*
 * notification stub. If we ever get interested in any events that
 * the pkcs11 functions may pass back to use, we can catch them here...
 * currently pdata is a slotinfo structure.
 */
CK_RV pk11_notify(CK_SESSION_HANDLE session, CK_NOTIFICATION event,
							 CK_VOID_PTR pdata) {
    return CKR_OK;
}


/*
 * grab a new RW session
 */
CK_SESSION_HANDLE PK11_GetRWSession(PK11SlotInfo *slot) {
    CK_SESSION_HANDLE rwsession;
    CK_RV crv;

    if (slot->defRWSession) return slot->session;

    crv = PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
				CKF_RW_SESSION|CKF_SERIAL_SESSION,
				  	  slot, pk11_notify,&rwsession);
    if (crv == CKR_SESSION_COUNT) {
	PK11_GETTAB(slot)->C_CloseSession(slot->session);
	slot->session = CK_INVALID_SESSION;
    	crv = PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
				CKF_RW_SESSION|CKF_SERIAL_SESSION,
				  	  slot,pk11_notify,&rwsession);
    }
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	if (slot->session == CK_INVALID_SESSION) {
    	    PK11_GETTAB(slot)->C_OpenSession(slot->slotID,CKF_SERIAL_SESSION,
				  	 slot,pk11_notify,&slot->session);
	}
	return CK_INVALID_SESSION;
    }

    return rwsession;
}

/*
 * close the rwsession and restore our readonly session
 */
void
PK11_RestoreROSession(PK11SlotInfo *slot,CK_SESSION_HANDLE rwsession) {
    if (slot->defRWSession) return;
    PK11_GETTAB(slot)->C_CloseSession(rwsession);
    if (slot->session == CK_INVALID_SESSION) {
    	 PK11_GETTAB(slot)->C_OpenSession(slot->slotID,CKF_SERIAL_SESSION,
				  	 slot,pk11_notify,&slot->session);
    }
}

/*
 * NOTE: this assumes that we are logged out of the card before hand
 */
SECStatus
PK11_CheckSSOPassword(PK11SlotInfo *slot, char *ssopw) {
    CK_SESSION_HANDLE rwsession;
    CK_RV crv;
    SECStatus rv = SECFailure;
    int len = PORT_Strlen(ssopw);

    /* get a rwsession */
    rwsession = PK11_GetRWSession(slot);
    if (rwsession == CK_INVALID_SESSION) return rv;

    /* check the password */
    crv = PK11_GETTAB(slot)->C_Login(rwsession,CKU_SO,
						(unsigned char *)ssopw,len);
    switch (crv) {
    /* if we're already logged in, we're good to go */
    case CKR_OK:
	rv = SECSuccess;
	break;
    case CKR_PIN_INCORRECT:
	rv = SECWouldBlock; /* everything else is ok, only the pin is bad */
	break;
    default:
	PORT_SetError(PK11_MapError(crv));
	rv = SECFailure; /* some failure we can't fix by retrying */
    }
    PK11_GETTAB(slot)->C_Logout(rwsession);
    /* release rwsession */
    PK11_RestoreROSession(slot,rwsession);
    return rv;
}

/*
 * make sure the password conforms to your token's requirements.
 */
SECStatus
PK11_VerifyPW(PK11SlotInfo *slot,char *pw) {
    int len = PORT_Strlen(pw);

    if ((slot->minPassword > len) || (slot->maxPassword < len)) {
	return SECFailure;
    }
    return SECSuccess;
}

/*
 * initialize a user PIN Value
 */
SECStatus
PK11_InitPin(PK11SlotInfo *slot,char *ssopw, char *userpw) {
    CK_SESSION_HANDLE rwsession = CK_INVALID_SESSION;
    CK_RV crv;
    SECStatus rv = SECFailure;
    int len;
    int ssolen;

    if (userpw == NULL) userpw = "";
    if (ssopw == NULL) ssopw = "";

    len = PORT_Strlen(userpw);
    ssolen = PORT_Strlen(ssopw);

    /* get a rwsession */
    rwsession = PK11_GetRWSession(slot);

    /* check the password */
    crv = PK11_GETTAB(slot)->C_Login(rwsession,CKU_SO, 
					  (unsigned char *)ssopw,ssolen);
    if (crv != CKR_OK) goto done;

    crv = PK11_GETTAB(slot)->C_InitPIN(rwsession,(unsigned char *)userpw,len);
    if (crv == CKR_OK) rv = SECSuccess;

done:
    PK11_GETTAB(slot)->C_Logout(rwsession);
    PK11_RestoreROSession(slot,rwsession);
    if (rv == SECSuccess) {
        /* update our view of the world */
        PK11_InitToken(slot);
    	PK11_GETTAB(slot)->C_Login(slot->session,CKU_USER,
						(unsigned char *)userpw,len);
    }
    return rv;
}

/*
 * Change an existing user password
 */
SECStatus
PK11_ChangePW(PK11SlotInfo *slot,char *oldpw, char *newpw) {
    CK_RV crv;
    SECStatus rv = SECFailure;
    int newLen;
    int oldLen;
    CK_SESSION_HANDLE rwsession;

    if (newpw == NULL) newpw = "";
    if (oldpw == NULL) oldpw = "";
    newLen = PORT_Strlen(newpw);
    oldLen = PORT_Strlen(oldpw);

    /* get a rwsession */
    rwsession = PK11_GetRWSession(slot);

    crv = PK11_GETTAB(slot)->C_SetPIN(rwsession,
		(unsigned char *)oldpw,oldLen,(unsigned char *)newpw,newLen);
    if (crv == CKR_OK) rv = SECSuccess;

    PK11_RestoreROSession(slot,rwsession);

    /* update our view of the world */
    PK11_InitToken(slot);
    return rv;
}



static char *
pk11_GetPassword(PK11SlotInfo *slot, void * wincx) {
    if (PK11_Global.getPass == NULL) return NULL;
    return (*PK11_Global.getPass)(slot,wincx);
}

void
PK11_SetPasswordFunc(char *(*func)(PK11SlotInfo *,void *)) {
    PK11_Global.getPass = func;
}


/*
 * authenticate to a slot. This loops until we can't recover, the user
 * gives up, or we succeed. If we're already logged in and this function
 * is called we will still prompt for a password, but we will probably
 * succeed no matter what the password was (depending on the implementation
 * of the PKCS 11 module.
 */
SECStatus
PK11_DoPassword(PK11SlotInfo *slot, void *wincx) {
    SECStatus rv = SECFailure;
    char * password;

    if (PK11_NeedUserInit(slot)) {
	/* XP_SetError() password not init */
	return SECFailure;
    }

    /* get the password. This can drop out of the while loop
     * for the following reasons:
     * 	(1) the user refused to enter a password. 
     *			(return error to caller)
     *	(2) the token user password is disabled [usually due to
     *	   too many failed authentication attempts].
     *			(return error to caller)
     *	(3) the password was successful.
     */
    while ((password = pk11_GetPassword(slot, wincx)) != NULL) {
	rv = pk11_CheckPassword(slot,password);
	PORT_Memset(password, 0, PORT_Strlen(password));
	PORT_Free(password);
	if (rv != SECWouldBlock) break;
    }
    return rv;
}

void PK11_LogoutAll(void) {
    SECMODListLock *lock = SECMOD_GetDefaultModuleListLock();
    SECMODModuleList *modList = SECMOD_GetDefaultModuleList();
    SECMODModuleList *mlp = NULL;
    int i;

    SECMOD_GetReadLock(lock);
    /* find the number of entries */
    for (mlp = modList; mlp != NULL; mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    PK11_Logout(mlp->module->slots[i]);
	}
    }

    SECMOD_ReleaseReadLock(lock);
}

/************************************************************
 * Manage the built-In Slot Lists
 ************************************************************/

/* Init the static built int slot list (should actually integrate
 * with PK11_NewSlotList */
static void
pk11_initSlotList(PK11SlotList *list) {
#ifdef PKCS11_USE_THREADS
    list->lock = PR_NewLock(0);
#else
    list->lock = NULL;
#endif
    list->head = NULL;
}

/* initialize the system slotlists */
SECStatus
PK11_InitSlotLists(void) {
    pk11_initSlotList(&pk11_rc4SlotList);
    pk11_initSlotList(&pk11_rc2SlotList);
    pk11_initSlotList(&pk11_rsaSlotList);
    pk11_initSlotList(&pk11_dsaSlotList);
    pk11_initSlotList(&pk11_dhSlotList);
    pk11_initSlotList(&pk11_idea);
    pk11_initSlotList(&pk11_random);
    return SECSuccess;
}

/* return a system slot list based on mechanism */
PK11SlotList *
PK11_GetSlotList(CK_MECHANISM_TYPE type) {
    switch (type) {
    case CKM_DES_CBC:
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_DES3_CBC:
	return &pk11_desSlotList;
    case CKM_RC4:
	return &pk11_rc4SlotList;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	return &pk11_rc2SlotList;
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	return &pk11_rsaSlotList;
    case CKM_DSA:
	return &pk11_dsaSlotList;
    case CKM_DH_PKCS_KEY_PAIR_GEN:
    case CKM_DH_PKCS_DERIVE:
	return &pk11_dhSlotList;
    case CKM_IDEA_CBC:
    case CKM_IDEA_ECB:
	return &pk11_idea;
    case CKM_FAKE_RANDOM:
	return &pk11_random;
    }
    return NULL;
}

/*
 * load the static SlotInfo structures used to select a PKCS11 slot.
 * preSlotInfo has a list of all the default flags for the slots on this
 * module.
 */
void
PK11_LoadSlotList(PK11SlotInfo *slot, PK11PreSlotInfo *psi, int count) {
    int i;

    for (i=0; i < count; i++) {
	if (psi[i].slotID == slot->slotID)
	    break;
    }

    if (i == count) return;

    slot->defaultFlags = psi[i].defaultFlags;
    slot->askpw = psi[i].askpw;
    slot->timeout = psi[i].timeout;

    /* if the slot is already disabled, don't load them into the
     * default slot lists. We get here so we can save the default
     * list value. */
    if (slot->disabled) return;

    /* if the user has disabled us, don't load us in */
    if (slot->defaultFlags & PK11_DISABLE_FLAG) {
	slot->disabled = PR_TRUE;
	slot->reason = PK11_USER_SELECTED;
	/* free up sessions and things?? */
	return;
    }

    for (i=0; i < sizeof(PK11_DefaultArray)/sizeof(PK11_DefaultArray[0]);
								i++) {
	if (slot->defaultFlags & PK11_DefaultArray[i].flag) {
	    CK_MECHANISM_TYPE mechanism = PK11_DefaultArray[i].mechanism;
	    PK11SlotList *slotList = PK11_GetSlotList(mechanism);

	    PK11_AddSlotToList(slotList,slot);
	}
    }

    return;
}

/*
 * clear a slot off of all of it's default list
 */
void
PK11_ClearSlotList(PK11SlotInfo *slot) {
    int i;

    if (slot->disabled) return;
    if (slot->defaultFlags == 0) return;

    for (i=0; i < sizeof(PK11_DefaultArray)/sizeof(PK11_DefaultArray[0]);
								i++) {
	if (slot->defaultFlags & PK11_DefaultArray[i].flag) {
	    CK_MECHANISM_TYPE mechanism = PK11_DefaultArray[i].mechanism;
	    PK11SlotList *slotList = PK11_GetSlotList(mechanism);
	    PK11SlotListElement *le = PK11_FindSlotElement(slotList,slot);

	    if (le) {
		PK11_DeleteSlotFromList(slotList,le);
		pk11_FreeListElement(slotList,le);
	    }
	}
    }
}


/******************************************************************
 *           Slot initialization
 ******************************************************************/
/*
 * turn a PKCS11 Static Label into a string
 */
char *
PK11_MakeString(PRArenaPool *arena,char *space,
					char *staticString,int stringLen) {
	int i;
	char *newString;
	for(i=(stringLen-1); i >= 0; i--) {
	  if (staticString[i] != ' ') break;
	}
	/* move i to point to the last space */
	i++;
	if (arena) {
	    newString = PORT_ArenaAlloc(arena,i+1 /* space for NULL */);
	} else if (space) {
	    newString = space;
	} else {
	    newString = PORT_Alloc(i+1 /* space for NULL */);
	}
	if (newString == NULL) return NULL;

	if (i) PORT_Memcpy(newString,staticString, i);
	newString[i] = 0;

	return newString;
}
/*
 * verify that slot implements Mechanism mech properly by checking against
 * our internal implementation
 */
PRBool
PK11_VerifyMechanism(PK11SlotInfo *slot,PK11SlotInfo *intern,
  CK_MECHANISM_TYPE mech, SECItem *data, SECItem *iv, SECItem *key) {
    PK11Context *test = NULL, *reference = NULL;
    SECItem *param = NULL;
    unsigned char encTest[8];
    unsigned char encRef[8];
    int outLenTest,outLenRef;
    SECStatus rv;

    /* initialize the mechanism parameter */
    param = PK11_ParamFromIV(mech,iv);
    if (param == NULL) goto loser;

    /* load the keys and contexts */
    test = PK11_CreateContextByRawKey(slot,mech,CKA_ENCRYPT,key,param,NULL);
    if (test == NULL) goto loser;
    reference = PK11_CreateContextByRawKey(intern,mech,
						CKA_ENCRYPT,key,param,NULL);
    if (reference == NULL) goto loser;
    SECITEM_FreeItem(param,PR_TRUE); param = NULL;

    /* encrypt the test data */
    rv = PK11_CipherOp(test,encTest,&outLenTest,sizeof(encTest),
							data->data,data->len);
    if (rv != SECSuccess) goto loser;
    rv = PK11_CipherOp(reference,encRef,&outLenRef,sizeof(encRef),
							data->data,data->len);
    if (rv != SECSuccess) goto loser;

    PK11_DestroyContext(reference,PR_TRUE); reference = NULL;
    PK11_DestroyContext(test,PR_TRUE); test = NULL;

    if (outLenTest != outLenRef) goto loser;
    if (PORT_Memcmp(encTest, encRef, outLenTest) != 0) goto loser;

    return PR_TRUE;

loser:
    if (test) PK11_DestroyContext(test,PR_TRUE);
    if (reference) PK11_DestroyContext(reference,PR_TRUE);
    if (param) SECITEM_FreeItem(param,PR_TRUE);

    return PR_FALSE;
}

/*
 * this code verifies that the advertised mechanisms are what they
 * seem to be.
 */
#define MAX_MECH_LIST_SIZE 30	/* we only know of about 30 odd mechanisms */
PRBool
PK11_VerifySlotMechanisms(PK11SlotInfo *slot) {
    CK_MECHANISM_TYPE mechListArray[MAX_MECH_LIST_SIZE];
    CK_MECHANISM_TYPE *mechList = mechListArray;
    static SECItem data;
    static SECItem iv;
    static SECItem key;
    static unsigned char dataV[8];
    static unsigned char ivV[8];
    static unsigned char keyV[8];
    static PRBool generated = PR_FALSE;
    CK_ULONG count;
    int i;

    PRBool alloced = PR_FALSE;
    PK11SlotInfo *intern = SECMOD_GetInternalModule()->slots[0];

    /* first get the count of mechanisms */
    if (PK11_GETTAB(slot)->C_GetMechanismList(slot->slotID,NULL,&count)
								 != CKR_OK) {
	return PR_FALSE;
    }

    /* don't blow up just because the card supports more mechanisms than
     * we know about, just alloc space for them */
    if (count > MAX_MECH_LIST_SIZE) {
    	mechList = (CK_MECHANISM_TYPE *)
			    PORT_Alloc(count *sizeof(CK_MECHANISM_TYPE));
	alloced = PR_TRUE;
    }
    /* get the list */
    if (PK11_GETTAB(slot)->C_GetMechanismList(slot->slotID, mechList, &count) 
					!= CKR_OK) {
	if (alloced) PORT_Free(mechList);
	return PR_FALSE;
    }

    if (!generated) {
	data.data = dataV;
	data.len = sizeof(dataV);
	iv.data = ivV;
	iv.len = sizeof(ivV);
	key.data = keyV;
	key.len = sizeof(keyV);
	
	PK11_GETTAB(intern)->C_GenerateRandom(intern->session,
							data.data, data.len);
	PK11_GETTAB(intern)->C_GenerateRandom(intern->session,
							iv.data, iv.len);
	PK11_GETTAB(intern)->C_GenerateRandom(intern->session,
							key.data, key.len);
    }
    for (i=0; i < count; i++) {
	switch (mechList[i]) {
	case CKM_DES_CBC:
	case CKM_DES_ECB:
	case CKM_RC4:
	case CKM_RC2_CBC:
	case CKM_RC2_ECB:
	    if (!PK11_VerifyMechanism(slot,intern,mechList[i],&data,&iv,&key)){
		if (alloced) PORT_Free(mechList);
		return PR_FALSE;
	    }
	}
    }
    if (alloced) PORT_Free(mechList);
    return PR_TRUE;
}

/*
 * See if we need to run the verify test, do so if necessary. If we fail,
 * decouple the slot.
 */    
SECStatus
pk11_CheckVerifyTest(PK11SlotInfo *slot) {
    if (slot->needTest) {
	if (!PK11_VerifySlotMechanisms(slot)) {
	    (void)PK11_GETTAB(slot)->C_CloseSession(slot->session);
	    slot->session = CK_INVALID_SESSION;
	    PK11_ClearSlotList(slot);
	    slot->disabled = PR_TRUE;
	    slot->reason = PK11_TOKEN_VERIFY_FAILED;
	    return SECFailure;
	}
	slot->needTest = PR_FALSE; /* test completed */
    }
    return SECSuccess;
}

/*
 * initialize a new token
 */
SECStatus
PK11_InitToken(PK11SlotInfo *slot) {
    CK_TOKEN_INFO tokenInfo;
    CK_RV rv;
    char *tmp;

    /* set the slot flags to the current token values */
    if (PK11_GETTAB(slot)->C_GetTokenInfo(slot->slotID,&tokenInfo) != CKR_OK) {
	return SECFailure;
    }

    /* set the slot flags to the current token values */
    slot->flags = tokenInfo.flags;
    slot->needLogin = ((tokenInfo.flags & CKF_LOGIN_REQUIRED) ? 
							PR_TRUE : PR_FALSE);
    slot->readOnly = ((tokenInfo.flags & CKF_WRITE_PROTECTED) ? 
							PR_TRUE : PR_FALSE);
    slot->hasRandom = ((tokenInfo.flags & CKF_RNG) ? PR_TRUE : PR_FALSE);
    tmp = PK11_MakeString(NULL,slot->token_name,
			(char *)tokenInfo.label, sizeof(tokenInfo.label));
    slot->minPassword = tokenInfo.ulMinPinLen;
    slot->maxPassword = tokenInfo.ulMaxPinLen;

    slot->defRWSession = (!slot->readOnly) && 
					(tokenInfo.ulMaxSessionCount == 1);

    /* Make sure our session handle is valid */
    if (slot->session == CK_INVALID_SESSION) {
	/* we know we don't have a valid session, go get one */
	CK_SESSION_HANDLE session;

	/* session should be Readonly, serial */
	if (PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
	      (slot->defRWSession ? CKF_RW_SESSION : 0) | CKF_SERIAL_SESSION,
				  slot,pk11_notify,&session) != CKR_OK) {
	    return SECFailure;
	}
	slot->session = session;
    } else {
	/* The session we have may be defunct (the token associated with it)
	 * has been removed   */
	CK_SESSION_INFO sessionInfo;

	rv = PK11_GETTAB(slot)->C_GetSessionInfo(slot->session,&sessionInfo);
        if (rv == CKR_DEVICE_ERROR) {
	    PK11_GETTAB(slot)->C_CloseSession(slot->session);
	    rv = CKR_SESSION_CLOSED;
	}
	if ((rv == CKR_SESSION_CLOSED) || (rv == CKR_SESSION_HANDLE_INVALID)) {
	    if (PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
	      (slot->defRWSession ? CKF_RW_SESSION : 0) | CKF_SERIAL_SESSION,
					slot,pk11_notify,&slot->session)
								 != CKR_OK) {
		slot->session = CK_INVALID_SESSION;
		return SECFailure;
	    }
	}
    }

    if (!(slot->needLogin)) {
	return pk11_CheckVerifyTest(slot);
    }
    return SECSuccess;
}

/*
 * Initialize the slot (boy this is a useful comment (sigh))
 */
void
PK11_InitSlot(SECMODModule *mod,CK_SLOT_ID slotID,PK11SlotInfo *slot) {
    SECStatus rv;
    char *tmp;
    CK_SLOT_INFO slotInfo;

    slot->functionList = mod->functionList;
    slot->isInternal = mod->internal;
    slot->slotID = slotID;
    
    if (PK11_GETTAB(slot)->C_GetSlotInfo(slotID,&slotInfo) != CKR_OK) {
	slot->disabled = PR_TRUE;
	slot->reason = PK11_COULD_NOT_INIT_TOKEN;
	return;
    }

    /* test to make sure claimed mechanism work */
    slot->needTest = mod->internal ? PR_FALSE : PR_TRUE;
    slot->module = mod; /* NOTE: we don't make a reference here because
			 * modules have references to their slots. This
			 * works because modules keep implicit references
			 * from their slots, and won't unload and disappear
			 * until all their slots have been freed */
    tmp = PK11_MakeString(NULL,slot->slot_name,
	 (char *)slotInfo.slotDescription, sizeof(slotInfo.slotDescription));
    if ((slotInfo.flags & CKF_REMOVABLE_DEVICE) == 0) {
	slot->isPerm = PR_TRUE;
	/* permanment slots must have the token present always */
	if ((slotInfo.flags & CKF_TOKEN_PRESENT) == 0) {
	    slot->disabled = PR_TRUE;
	    slot->reason = PK11_TOKEN_NOT_PRESENT;
	    return; /* nothing else to do */
	}
    }
    /* if the token is present, initialize it */
    if ((slotInfo.flags & CKF_TOKEN_PRESENT) != 0) {
	rv = PK11_InitToken(slot);
	/* the only hard failures are on permanent devices, or function
	 * verify failures... function verify failures are already handled
	 * by tokenInit */
	if ((rv != SECSuccess) && (slot->isPerm) && (!slot->disabled)) {
	    slot->disabled = PR_TRUE;
	    slot->reason = PK11_COULD_NOT_INIT_TOKEN;
	}
    }
}

	

/*********************************************************************
 *            Slot mapping utility functions.
 *********************************************************************/

/*
 * determine if the token is present. If the token is present, make sure
 * we have a valid session handle. Also set the value of needLogin 
 * appropriately.
 */
PRBool
PK11_IsPresent(PK11SlotInfo *slot) {
    CK_SLOT_INFO slotInfo;

    if (slot->disabled) {
	return PR_FALSE;
    }
    if (slot->isPerm && (slot->session != CK_INVALID_SESSION)) {
	return PR_TRUE;
    }
    if (PK11_GETTAB(slot)->C_GetSlotInfo(slot->slotID,&slotInfo) != CKR_OK) {
	return PR_FALSE;
    }
    if ((slotInfo.flags & CKF_TOKEN_PRESENT) == 0) return PR_FALSE;

    if (PK11_InitToken(slot) != SECSuccess) {
	return PR_FALSE;
    }

    return PR_TRUE;
}

/* is the slot disabled? */
PRBool
PK11_IsDisabled(PK11SlotInfo *slot) {
    return slot->disabled;
}

/* and why? */
int
PK11_GetDisabledReason(PK11SlotInfo *slot) {
    return slot->reason;
}

/*
 * we can initialize the password if 1) The toke is not inited 
 * (need login == true and see need UserInit) or 2) the token has
 * a NULL password. (slot->needLogin = false & need user Init = false).
 */
PRBool PK11_NeedPWInit() {
    PK11SlotInfo *slot = PK11_GetInternalKeySlot();
    if (slot->needLogin && PK11_NeedUserInit(slot)) {
	return PR_TRUE;
    }
    if (!slot->needLogin && !PK11_NeedUserInit(slot)) {
	return PR_TRUE;
    }
    return PR_FALSE;
}

/*
 * The following wrapper functions allow us to export an opaque slot
 * function to the rest of libsec and the world... */
PRBool
PK11_IsReadOnly(PK11SlotInfo *slot) {
    return slot->readOnly;
}

PRBool
PK11_IsInternal(PK11SlotInfo *slot) {
    return slot->isInternal;
}

PRBool
PK11_NeedLogin(PK11SlotInfo *slot) {
    return slot->needLogin;
}


char *
PK11_GetTokenName(PK11SlotInfo *slot) {
     return slot->token_name;
}

char *
 PK11_GetSlotName(PK11SlotInfo *slot) {
     return slot->slot_name;
}

SECStatus
PK11_GetSlotInfo(PK11SlotInfo *slot, CK_SLOT_INFO *info) {
    CK_RV crv;

    crv = PK11_GETTAB(slot)->C_GetSlotInfo(slot->slotID,info);
    if (crv != CKR_OK) {
	return SECFailure;
    }
    return SECSuccess;
}

SECStatus
PK11_GetTokenInfo(PK11SlotInfo *slot, CK_TOKEN_INFO *info) {
    CK_RV crv = PK11_GETTAB(slot)->C_GetTokenInfo(slot->slotID,info);
    if (crv != CKR_OK) {
	return SECFailure;
    }
    return SECSuccess;
}

PRBool
PK11_NeedUserInit(PK11SlotInfo *slot) {
    return (slot->flags & CKF_USER_PIN_INITIALIZED) == 0;
}

PK11SlotInfo *
PK11_GetInternalKeySlot(void) {
	return PK11_ReferenceSlot(SECMOD_GetInternalModule()->slots[1]);
}

PK11SlotInfo *
PK11_GetInternalSlot(void) {
	return PK11_ReferenceSlot(SECMOD_GetInternalModule()->slots[0]);
}

/*
 * Determine if the token is logged in. We have actually query the token,
 * because it's state can change without intervention from us.
 */
PRBool
PK11_IsLoggedIn(PK11SlotInfo *slot) {
    CK_SESSION_INFO sessionInfo;
    int askpw = slot->askpw;
    int timeout = slot->timeout;
    CK_RV crv;

    /* If we don't have our own password default values, use the system
     * ones */
    if ((slot->defaultFlags & PK11_OWN_PW_DEFAULTS) == 0) {
	PK11SlotInfo *def_slot = PK11_GetInternalKeySlot();

	askpw = def_slot->askpw;
	timeout = def_slot->timeout;
    }

    /* forget the password if we've been inactive too long */
    if (askpw == 1) {
	time_t currtime = XP_TIME();
	if (slot->authTime + (slot->timeout*60) < currtime) {
	    PK11_GETTAB(slot)->C_Logout(slot->session);
	} else {
	    slot->authTime = currtime;
	}
    }

    crv = PK11_GETTAB(slot)->C_GetSessionInfo(slot->session,&sessionInfo);
    /* if we can't get session info, something is really wrong */
    if (crv != CKR_OK) {
	slot->session = CK_INVALID_SESSION;
	return PR_FALSE;
    }

    switch (sessionInfo.state) {
    case CKS_RW_PUBLIC_SESSION:
    case CKS_RO_PUBLIC_SESSION:
    default:
	break; /* fail */
    case CKS_RW_USER_FUNCTIONS:
    case CKS_RW_SO_FUNCTIONS:
    case CKS_RO_USER_FUNCTIONS:
	return PR_TRUE;
    }
    return PR_FALSE; 
}

/*
 * check if a given slot supports the requested mechanism
 */
PRBool
PK11_DoesMechanism(PK11SlotInfo *slot, CK_MECHANISM_TYPE type) {
    CK_MECHANISM_TYPE mechListArray[MAX_MECH_LIST_SIZE];
    CK_MECHANISM_TYPE *mechList = mechListArray;
    PRBool alloced = PR_FALSE;
    CK_ULONG count;
    int i;

    /* CKM_FAKE_RANDOM is not a real PKCS mechanism. It's a marker to
     * tell us we're looking form someone that has implemented get
     * random bits */
    if (type == CKM_FAKE_RANDOM) {
	return slot->hasRandom;
    }

    if (PK11_GETTAB(slot)->C_GetMechanismList(slot->slotID,NULL,&count)
								 != CKR_OK) {
	return PR_FALSE;
    }

    /* don't blow up just because the card supports more mechanisms than
     * we know about, just alloc space for them */
    if (count > MAX_MECH_LIST_SIZE) {
    	mechList = (CK_MECHANISM_TYPE *)
			    PORT_Alloc(count *sizeof(CK_MECHANISM_TYPE));
	alloced = PR_TRUE;
    }
    if (PK11_GETTAB(slot)->C_GetMechanismList(slot->slotID, mechList, &count) 
					!= CKR_OK) {
	if (alloced) PORT_Free(mechList);
	return PR_FALSE;
    }
    for (i=0; i < count; i++) {
	if (mechList[i] == type) break;
    }
    if (alloced) PORT_Free(mechList);
    return (PRBool) (i < count); /*? PR_TRUE : PR_FALSE; */
}


/*
 * get all the currently available tokens in a list
 */
PK11SlotList *
PK11_GetAllTokens(CK_MECHANISM_TYPE type,PRBool needRW) {
    PK11SlotList *list = PK11_NewSlotList();
    SECMODModuleList *mlp;
    SECMODModuleList *modules = SECMOD_GetDefaultModuleList();
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();

    int i;

    SECMOD_GetReadLock(moduleLock);
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    PK11SlotInfo *slot = mlp->module->slots[i];
	    if (PK11_IsPresent(slot)) {
		if (needRW &&  slot->readOnly) continue;
		if ((type == CKM_INVALID_MECHANISM) 
					|| PK11_DoesMechanism(slot,type)) {
		    PK11_AddSlotToList(list,slot);
		}
	    }
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);

    return list;
}

/*
 * NOTE: This routine is working from a private List generated by 
 * PK11_GetAllTokens. That is why it does not need to lock.
 */
PK11SlotList *
PK11_GetPrivateKeyTokens(CK_MECHANISM_TYPE type,PRBool needRW,void *wincx) {
    PK11SlotList *list = PK11_GetAllTokens(type,needRW);
    PK11SlotListElement *le, *next ;
    SECStatus rv;

    if (list == NULL) return list;

    for (le = list->head ; le; le = next) {
	next = le->next; /* save the pointer here in case we have to 
			  * free the element later */
	if (le->slot->needLogin && !PK11_IsLoggedIn(le->slot)) {
	    /* card is weirded out, remove it from the list */
	    if (le->slot->session == CK_INVALID_SESSION) {
		PK11_DeleteSlotFromList(list,le);
	    }
	    rv = PK11_DoPassword(le->slot,wincx);
	    if (rv != SECSuccess) {
		PK11_DeleteSlotFromList(list,le);
		continue;
	    }
	    rv = pk11_CheckVerifyTest(le->slot);
	    if (rv != SECSuccess) {
		PK11_DeleteSlotFromList(list,le);
		continue;
	    }
	}
    }
    return list;
}

	
/*
 * find the best slot which supports the given
 * Mechanism. In normal cases this should grab the first slot on the list
 * with no fuss.
 */
PK11SlotInfo *
PK11_GetBestSlot(CK_MECHANISM_TYPE type, void *wincx) {
    PK11SlotList *list;
    PK11SlotListElement *le ;
    PK11SlotInfo *slot = NULL;
    PRBool freeit = PR_FALSE;
    SECStatus rv;


    /* always digest from the internal slto for now */
    if ((type == CKM_MD5) || (type == CKM_SHA_1) || (type == CKM_MD2)) {
	return PK11_GetInternalSlot();
    }

    list = PK11_GetSlotList(type);

    if (list == NULL) {
	/* sigh, we need to look up all the tokens for the mechanism */
	list = PK11_GetAllTokens(type,PR_FALSE);
	freeit = PR_TRUE;
    }

    /* no one can do it! */
    if (list == NULL) return NULL;

    for (le = PK11_GetFirstSafe(list); le;
			 	le = PK11_GetNextSafe(list,le,PR_TRUE)) {
	if (PK11_IsPresent(le->slot)) {
	    /* Random number generaters shouldn't need authentication */
	    if (type == CKM_FAKE_RANDOM) {
		/* if we think we have it, get it */
		if (!le->slot->hasRandom) continue;
	    /* neither should hashing functions */
	    } else if ((type == CKM_SHA_1) || (type == CKM_MD5) || 
							(type == CKM_MD2)) {
		slot = le->slot; /* fall through */
	    /* now deal with passwords and pins */
	    } else if (le->slot->needLogin && !PK11_IsLoggedIn(le->slot)) {
		/* card is weirded out, go to the next slot */
		if (le->slot->session == CK_INVALID_SESSION) {
		    continue;
		}
		rv = PK11_DoPassword(le->slot,wincx);
		if (rv != SECSuccess) continue;
		rv = pk11_CheckVerifyTest(le->slot);
		if (rv != SECSuccess) continue;
	    }
	    slot = le->slot;
	    PK11_ReferenceSlot(slot);
	    pk11_FreeListElement(list,le);
	    if (freeit) { PK11_FreeSlotList(list); }
	    return slot;
	}
    }
    if (freeit) { PK11_FreeSlotList(list); }
    return NULL;
}


/*********************************************************************
 *       Mechanism Mapping functions
 *********************************************************************/

/*
 * lookup an entry in the mechanism table. If none found, return the
 * default structure.
 */
static pk11MechanismData *
pk11_lookup(CK_MECHANISM_TYPE type) {
    int i;
    for (i=0; i < pk11_MechEntrySize; i++) {
	if (pk11_MechanismTable[i].type == type) {
	     return (&pk11_MechanismTable[i]);
	}
    }
    return &pk11_default;
}

/*
 * WARNING WARNING... not thread safe. Called at init time, and when loading
 * a new Entry. It is reasonably safe as long as it is not re-entered
 * (readers will always see a consistant table)
 *
 * This routint is called to add entries to the mechanism table, once there,
 * they can not be removed.
 */
void
PK11_AddMechanismEntry(CK_MECHANISM_TYPE type, CK_KEY_TYPE key,
		 	CK_MECHANISM_TYPE keyGen, int ivLen, int blockSize) {
    int tableSize = pk11_MechTableSize;
    int size = pk11_MechEntrySize;
    int entry = size++;
    pk11MechanismData *old = pk11_MechanismTable;
    pk11MechanismData *newt = pk11_MechanismTable;

	
    if (size > tableSize) {
	int oldTableSize = tableSize;
	tableSize += 10;
	newt = (pk11MechanismData *)
				PORT_Alloc(tableSize*sizeof(pk11MechanismData));
	if (newt == NULL) return;

	if (old) PORT_Memcpy(newt,old,oldTableSize*sizeof(pk11MechanismData));
    } else old = NULL;

    newt[entry].type = type;
    newt[entry].keyType = key;
    newt[entry].keyGen = keyGen;
    newt[entry].iv = ivLen;
    newt[entry].blockSize = blockSize;

    pk11_MechanismTable = newt;
    pk11_MechTableSize = tableSize;
    pk11_MechEntrySize = size;
    if (old) PORT_Free(old);
}

/*
 * Get the key type needed for the given mechanism
 */
CK_MECHANISM_TYPE
PK11_GetKeyType(CK_MECHANISM_TYPE type,unsigned long len) {
    switch (type) {
    case CKM_DES_ECB:
    case CKM_DES_CBC:
	return CKK_DES;
    case CKM_DES3_ECB:
    case CKM_DES3_CBC:
	return (len == 128) ? CKK_DES2 : CKK_DES3;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	return CKK_RC2;
    /*case CKM_RC5_CBC:
	return CKK_RC5; */
    case CKM_RC4:
	return CKK_RC4;
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_SKIPJACK_WRAP:
	return CKK_SKIPJACK;
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_BATON_WRAP:
	return CKK_BATON;
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
    case CKM_JUNIPER_WRAP:
	return CKK_JUNIPER;
    case CKM_IDEA_CBC:
    case CKM_IDEA_ECB:
	return CKK_IDEA;
    case CKM_RSA_PKCS:
    case CKM_RSA_9796:
    case CKM_RSA_X_509:
	return CKK_RSA;
    case CKM_DSA:
	return CKK_DSA;
    case CKM_DH_PKCS_DERIVE:
	return CKK_DH;
    case CKM_KEA_KEY_DERIVE:
	return CKK_KEA;
    case CKM_ECDSA:
	return CKK_ECDSA;
    case CKM_MAYFLY_KEY_DERIVE:
	return CKK_MAYFLY;
    case CKM_SSL3_PRE_MASTER_KEY_GEN:
	return CKK_GENERIC_SECRET;
    default:
	return pk11_lookup(type)->keyType;
    }
}

/*
 * Get the key type needed for the given mechanism
 */
CK_MECHANISM_TYPE
PK11_GetKeyGen(CK_MECHANISM_TYPE type) {
    switch (type) {
    case CKM_DES_ECB:
    case CKM_DES_CBC:
	return CKM_DES_KEY_GEN;
    case CKM_DES3_ECB:
    case CKM_DES3_CBC:
	return CKM_DES3_KEY_GEN;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	return CKM_RC2_KEY_GEN;
    case CKM_RC4:
	return CKM_RC4_KEY_GEN;
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_SKIPJACK_WRAP:
	return CKM_SKIPJACK_KEY_GEN;
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_BATON_WRAP:
	return CKM_BATON_KEY_GEN;
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
    case CKM_JUNIPER_WRAP:
	return CKM_JUNIPER_KEY_GEN;
    case CKM_IDEA_CBC:
    case CKM_IDEA_ECB:
	return CKM_IDEA_KEY_GEN;
    case CKM_RSA_PKCS:
    case CKM_RSA_9796:
    case CKM_RSA_X_509:
	return CKM_RSA_PKCS_KEY_PAIR_GEN;
    case CKM_DSA:
	return CKM_DSA_KEY_PAIR_GEN;
    case CKM_DH_PKCS_DERIVE:
	return CKM_DH_PKCS_KEY_PAIR_GEN;
    case CKM_KEA_KEY_DERIVE:
	return CKM_KEA_KEY_PAIR_GEN;
    case CKM_ECDSA:
	return CKM_ECDSA_KEY_PAIR_GEN;
    case CKM_MAYFLY_KEY_DERIVE:
	return CKM_MAYFLY_KEY_PAIR_GEN;
    case CKM_PBE_MD2_DES_CBC:
    case CKM_PBE_MD5_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_RC4:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
    	return CKM_NETSCAPE_PBE_KEY_GEN;
    default:
	return pk11_lookup(type)->keyGen;
    }
}

/*
 * get the mechanism block size
 */
int
PK11_GetBlockSize(CK_MECHANISM_TYPE type,SECItem *params) {
    switch (type) {
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_RC2_ECB:
    case CKM_IDEA_ECB:
    case CKM_RC2_CBC:
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_DES_CBC:
    case CKM_DES3_CBC:
    case CKM_IDEA_CBC:
    case CKM_PBE_MD2_DES_CBC:
    case CKM_PBE_MD5_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
	return 8;
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
	return 4;
    case CKM_BATON_ECB128:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
	return 16;
    case CKM_BATON_ECB96:
	return 12;
    case CKM_RC4:
    case CKM_NETSCAPE_PBE_SHA1_RC4:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
	return 0;
    case CKM_RSA_PKCS:
    case CKM_RSA_9796:
    case CKM_RSA_X_509:
	/*actually it's the modulus length of the key!*/
	return MAX_RSA_MODULUS_LEN;
    default:
	return pk11_lookup(type)->blockSize;
    }
}

/*
 * get the iv length
 */
int
PK11_GetIVLength(CK_MECHANISM_TYPE type) {
    switch (type) {
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_RC2_ECB:
    case CKM_IDEA_ECB:
    case CKM_SKIPJACK_WRAP:
    case CKM_BATON_WRAP:
	return 0;
    case CKM_RC2_CBC:
    case CKM_DES_CBC:
    case CKM_DES3_CBC:
    case CKM_IDEA_CBC:
    case CKM_PBE_MD2_DES_CBC:
    case CKM_PBE_MD5_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
	return 8;
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
	return 24;
    case CKM_RC4:
    case CKM_RSA_PKCS:
    case CKM_RSA_9796:
    case CKM_RSA_X_509:
    case CKM_NETSCAPE_PBE_SHA1_RC4:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
	return 0;
    default:
	return pk11_lookup(type)->iv;
    }
}


/* These next two utilities are here to help facilitate future
 * Dynamic Encrypt/Decrypt symetric key mechanisms, and to allow functions
 * like SSL and S-MIME to automatically add them.
 */
SECItem *
PK11_ParamFromIV(CK_MECHANISM_TYPE type,SECItem *iv) {
    CK_RC2_CBC_PARAMS *rc2_params;
    SECItem *param;

    param = (SECItem *)PORT_Alloc(sizeof(SECItem));
    if (param == NULL) return NULL;
    param->data = NULL;
    param->len = 0;
    switch (type) {
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
    case CKM_RSA_9796:
    case CKM_IDEA_ECB:
    case CKM_RC4:
	break;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	rc2_params = (CK_RC2_CBC_PARAMS *)PORT_Alloc(sizeof(CK_RC2_CBC_PARAMS));
	if (rc2_params == NULL) break;
	/* XXX Maybe we should pass the key size in too to get this value? */
	rc2_params->ulEffectiveBits = 128;
	if (iv && iv->data)
	    PORT_Memcpy(rc2_params->iv,iv->data,sizeof(rc2_params->iv));
	param->data = (unsigned char *) rc2_params;
	param->len = sizeof(CK_RC2_CBC_PARAMS);
	break;
    case CKM_DES_CBC:
    case CKM_DES3_CBC:
    case CKM_IDEA_CBC:
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
	if ((iv == NULL) || (iv->data == NULL)) break;
	param->data = PORT_Alloc(iv->len);
	if (param->data != NULL) {
	    PORT_Memcpy(param->data,iv->data,iv->len);
	    param->len = iv->len;
	}
	break;
     /* unknown mechanism, pass IV in if it's there */
     default:
	if (pk11_lookup(type)->iv == 0) {
	    break;
	}
	if ((iv == NULL) || (iv->data == NULL)) {
	    break;
	}
	param->data = PORT_Alloc(iv->len);
	if (param->data != NULL) {
	    PORT_Memcpy(param->data,iv->data,iv->len);
	    param->len = iv->len;
	}
	break;
     }
     return param;
}


typedef struct sec_rc2cbcParameterStr {
    SECItem rc2ParameterVersion;
    SECItem iv;
} sec_rc2cbcParameter;

static const SEC_ASN1Template sec_rc2cbc_parameter_template[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(sec_rc2cbcParameter) },
    { SEC_ASN1_INTEGER,
          offsetof(sec_rc2cbcParameter,rc2ParameterVersion) },
    { SEC_ASN1_OCTET_STRING,
          offsetof(sec_rc2cbcParameter,iv) },
    { 0 }
};

/* sigh who the !#!@$$@! picked these *STUPID* values for keysize */
/* I do have a formula, but it ain't pretty, and it only works because you
 * can always match three points to a parabola:) */
static unsigned char  rc2_map(SECItem *version) {
    long x;

    x = DER_GetInteger(version);
    
    switch (x) {
        case 58: return 128;
        case 120: return 64;
        case 160: return 40;
    }
    return 128; /* sigh */
}

static unsigned long  rc2_unmap(unsigned long x) {
    switch (x) {
        case 128: return 58;
        case 64: return 120;
        case 40: return 160;
    }
    return 58; /* sigh */
}


/* Generate a mechaism param from a type, and iv. */
SECItem *
PK11_ParamFromAlgid(SECAlgorithmID *algid) {
    CK_RC2_CBC_PARAMS *rc2_params;
    CK_PBE_PARAMS *pbe_params = NULL;
    CK_NETSCAPE_PBE_PARAMS *ns_pbe_params;
    SECItem iv;
    sec_rc2cbcParameter rc2;
    SEC_PKCS5PBEParameter *p5_param;
    SECItem *mech;
    CK_MECHANISM_TYPE type;
    SECOidTag algtag;
    SECStatus rv;
    SECItem p5_misc;

    algtag = SECOID_GetAlgorithmTag(algid);
    type = PK11_AlgtagToMechanism(algtag);

    mech = (SECItem *) PORT_Alloc(sizeof(SECItem));
    if (mech == NULL) return NULL;


    switch (type) {
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
        rv = SEC_ASN1DecodeItem(NULL, &rc2 ,sec_rc2cbc_parameter_template,
							&(algid->parameters));
	if (rv != SECSuccess) { 
	    PORT_Free(mech);
	    return NULL;
	}
	rc2_params = (CK_RC2_CBC_PARAMS *)PORT_Alloc(sizeof(CK_RC2_CBC_PARAMS));
	if (rc2_params == NULL) {
	    PORT_Free(rc2.iv.data);
	    PORT_Free(rc2.rc2ParameterVersion.data);
	    PORT_Free(mech);
	    return NULL;
	}
	rc2_params->ulEffectiveBits = rc2_map(&rc2.rc2ParameterVersion);
	PORT_Free(rc2.rc2ParameterVersion.data);
	PORT_Memcpy(rc2_params->iv,rc2.iv.data,sizeof(rc2_params->iv));
	PORT_Free(rc2.iv.data);
	mech->data = (unsigned char *) rc2_params;
	mech->len = sizeof(CK_RC2_CBC_PARAMS);
	return mech;
    /* case CKM_RC5_XXX */
    /*  break;		*/
    }

    switch (type) {
    case CKM_PBE_MD2_DES_CBC:
    case CKM_PBE_MD5_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_RC4:
	p5_param = SEC_PKCS5GetPBEParameter(algid);
	if(p5_param != NULL) {
	    if(p5_param->keyLength.data != NULL) {
	    	ns_pbe_params = (CK_NETSCAPE_PBE_PARAMS *)PORT_ZAlloc(
	    		sizeof(CK_NETSCAPE_PBE_PARAMS));
	    } else {
		pbe_params = (CK_PBE_PARAMS *)PORT_ZAlloc(sizeof(CK_PBE_PARAMS));
		ns_pbe_params = (CK_NETSCAPE_PBE_PARAMS *)pbe_params;
	    }

	    if(ns_pbe_params == NULL) {
		SEC_PKCS5DestroyPBEParameter(p5_param);
		PORT_Free(mech);
		return NULL;
	    }

	    /* get salt */
	    p5_misc = p5_param->salt;
	    ns_pbe_params->pSalt = (CK_CHAR_PTR)PORT_ZAlloc(p5_misc.len);
	    if(ns_pbe_params->pSalt != NULL_PTR) {
		PORT_Memcpy(ns_pbe_params->pSalt, p5_misc.data, p5_misc.len);
		ns_pbe_params->ulSaltLen = (CK_ULONG)p5_misc.len;
		rv = SECSuccess;
	    } else {
		rv = SECFailure;
	    }

	    /* get iteration count */
	    if(rv != SECFailure) {
		p5_misc = p5_param->iteration;
		ns_pbe_params->ulIteration = (CK_ULONG)DER_GetInteger(&p5_misc);
	    }

	    /* get optional key length */
	    if((rv == SECSuccess) && (p5_param->keyLength.data != NULL)) {
		p5_misc = p5_param->keyLength;
		ns_pbe_params->pSalt = (CK_CHAR_PTR)PORT_ZAlloc(p5_misc.len);
		if(ns_pbe_params->pSalt != NULL_PTR) {
		    PORT_Memcpy(ns_pbe_params->pSalt, p5_misc.data, 
			p5_misc.len);
		    ns_pbe_params->ulSaltLen = (CK_ULONG)p5_misc.len;
		    rv = SECSuccess;
		} else {
		    rv = SECFailure;
		}
	    }

	    /* get optional init vector */
	    if((rv == SECSuccess) && (p5_param->iv.data != NULL)) {
		p5_misc = p5_param->keyLength;
		ns_pbe_params->pInitVector = (CK_CHAR_PTR)PORT_ZAlloc(
			p5_misc.len);
		if(ns_pbe_params->pSalt != NULL_PTR) {
		    PORT_Memcpy(ns_pbe_params->pInitVector, p5_misc.data, 
			p5_misc.len);
		    rv = SECSuccess;
		} else {
		    rv = SECFailure;
		}
	    }

	    if(rv == SECFailure) {
		if(ns_pbe_params->pSalt != NULL_PTR) {
		    PORT_Free(ns_pbe_params->pSalt);
		}
		if(ns_pbe_params->pInitVector != NULL_PTR) {
		    PORT_Free(ns_pbe_params->pInitVector);
		}
		if(p5_param->keyLength.data != NULL) {
		    PORT_Free(ns_pbe_params);
		} else {
		    PORT_Free(pbe_params);
		}
		PORT_Free(mech);
		mech = NULL;
	    } else {
		mech->data = (unsigned char *)ns_pbe_params;
		if(p5_param->keyLength.data != NULL) {
		    mech->len = sizeof(CK_NETSCAPE_PBE_PARAMS);
		} else {
		    mech->len = sizeof(CK_PBE_PARAMS);
		}
	    }
	    SEC_PKCS5DestroyPBEParameter(p5_param);
	} else {
	    PORT_Free(mech);
	    mech = NULL;
	}

	return mech;
    }


    rv = SEC_ASN1DecodeItem(NULL, &iv, SEC_OctetStringTemplate, 
							&(algid->parameters));
    if (rv !=  SECSuccess) {
	iv.data = NULL;
	iv.len = 0;
    }

    rv = SECSuccess;
    switch (type) {
    case CKM_RC4:
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_IDEA_ECB:
	mech->data = NULL;
	mech->len = 0;
	break;
    default:
	if (pk11_lookup(type)->iv == 0) {
	    mech->data = NULL;
	    mech->len = 0;
	    break;
	}
    case CKM_DES_CBC:
    case CKM_DES3_CBC:
    case CKM_IDEA_CBC:
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
	if (iv.data ==  NULL) {
	    rv = SECFailure;
	    break;
	}
	mech->data = PORT_Alloc(iv.len);
	if (mech->data == NULL) {
	    rv = SECFailure;
	    break;
	}
	PORT_Memcpy(mech->data,iv.data,iv.len);
	mech->len = iv.len;
	break;
    }
    if (iv.data) PORT_Free(iv.data);
    if (rv !=  SECSuccess) {
	SECITEM_FreeItem(mech,PR_TRUE);
	return NULL;
    }
    return mech;
}

SECStatus
PK11_GenerateRandom(unsigned char *data,int len) {
    PK11SlotInfo *slot;
    CK_RV crv;

    slot = PK11_GetBestSlot(CKM_FAKE_RANDOM,NULL);
    if (slot == NULL) return SECFailure;

    crv = PK11_GETTAB(slot)->C_GenerateRandom(slot->session,data, 
							(CK_ULONG)len);
    if (crv != CKR_OK) return SECFailure;

    return SECSuccess;
}

/*
 * Generate an IV for the given mechanism 
 */
static SECStatus
pk11_GenIV(CK_MECHANISM_TYPE type, SECItem *iv) {
    int iv_size = PK11_GetIVLength(type);
    SECStatus rv;

    iv->len = iv_size;
    if (iv_size == 0) { 
	iv->data = NULL;
	return SECSuccess;
    }

    iv->data = (unsigned char *) PORT_Alloc(iv_size);
    if (iv->data == NULL) {
	iv->len = 0;
	return SECFailure;
    }

    rv = PK11_GenerateRandom(iv->data,iv->len);
    if (rv != SECSuccess) {
	PORT_Free(iv->data);
	iv->data = NULL; iv->len = 0;
	return SECFailure;
    }
    return SECSuccess;
}


/*
 * create a new paramter block from the passed in algorithm ID and the
 * key.
 */
SECItem *
PK11_GenerateNewParam(CK_MECHANISM_TYPE type, PK11SymKey *key) { 
    CK_RC2_CBC_PARAMS *rc2_params;
    SECItem *mech;
    SECItem iv;
    SECStatus rv;


    mech = (SECItem *) PORT_Alloc(sizeof(SECItem));
    if (mech == NULL) return NULL;

    rv = SECSuccess;
    switch (type) {
    case CKM_RC4:
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_IDEA_ECB:
	mech->data = NULL;
	mech->len = 0;
	break;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	rv = pk11_GenIV(type,&iv);
	if (rv != SECSuccess) {
	    break;
	}
	rc2_params = (CK_RC2_CBC_PARAMS *)PORT_Alloc(sizeof(CK_RC2_CBC_PARAMS));
	if (rc2_params == NULL) {
	    PORT_Free(iv.data);
	    rv = SECFailure;
	    break;
	}
	/* NOTE PK11_GetKeyLength can return -1 if the key isn't and RC2, RC5,
	 *   or RC4 key. Of course that wouldn't happen here doing RC2:).*/
	rc2_params->ulEffectiveBits = PK11_GetKeyLength(key)*8;
	if (iv.data)
	    PORT_Memcpy(rc2_params->iv,iv.data,sizeof(rc2_params->iv));
	mech->data = (unsigned char *) rc2_params;
	mech->len = sizeof(CK_RC2_CBC_PARAMS);
	PORT_Free(iv.data);
	break;
    /* case CKM_RC5_XXX */

    default:
	if (pk11_lookup(type)->iv == 0) {
	    mech->data = NULL;
	    mech->len = 0;
	    break;
	}
    case CKM_DES_CBC:
    case CKM_DES3_CBC:
    case CKM_IDEA_CBC:
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
	rv = pk11_GenIV(type,&iv);
	if (rv != SECSuccess) {
	    break;
	}
	mech->data = PORT_Alloc(iv.len);
	if (mech->data == NULL) {
	    PORT_Free(iv.data);
	    rv = SECFailure;
	    break;
	}
	PORT_Memcpy(mech->data,iv.data,iv.len);
	mech->len = iv.len;
        PORT_Free(iv.data);
	break;
    }
    if (rv !=  SECSuccess) {
	SECITEM_FreeItem(mech,PR_TRUE);
	return NULL;
    }
    return mech;

}

/* turn a parameter into an algid */
SECStatus
PK11_ParamToAlgid(SECOidTag algTag, SECItem *param, 
				PRArenaPool *arena, SECAlgorithmID *algid) {
    CK_RC2_CBC_PARAMS *rc2_params;
    sec_rc2cbcParameter rc2;
    CK_MECHANISM_TYPE type = PK11_AlgtagToMechanism(algTag);
    SECItem *newParams = NULL;
    SECStatus rv = SECFailure;
    unsigned long rc2version;
    CK_NETSCAPE_PBE_PARAMS *ns_pbe_params = NULL;
    SECAlgorithmID *pbe_algid = NULL;
    SECItem pbe_salt, pbe_iv, pbe_param;

    rv = SECSuccess;
    switch (type) {
    case CKM_RC4:
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_IDEA_ECB:
	newParams = NULL;
	rv = SECSuccess;
	break;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	rc2_params = (CK_RC2_CBC_PARAMS *)param->data;
	rc2version = rc2_unmap(rc2_params->ulEffectiveBits);
	if (SEC_ASN1EncodeUnsignedInteger (NULL, &(rc2.rc2ParameterVersion),
					   rc2version) == NULL)
	    break;
	rc2.iv.data = rc2_params->iv;
	rc2.iv.len = sizeof(rc2_params->iv);
	newParams = SEC_ASN1EncodeItem (NULL, NULL, &rc2,
                                         sec_rc2cbc_parameter_template);
	if (newParams == NULL)
	    break;
	rv = SECSuccess;
	break;

    /* case CKM_RC5_XXX */

    case CKM_PBE_MD2_DES_CBC:
    case CKM_PBE_MD5_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_RC4:
	ns_pbe_params = (CK_NETSCAPE_PBE_PARAMS *)param->data;
	pbe_salt.data = (unsigned char *)ns_pbe_params->pSalt;
	pbe_salt.len = 8;
	pbe_iv.data = pbe_param.data = NULL;
	pbe_iv.len = pbe_param.len = 0;
	ns_pbe_params = (CK_NETSCAPE_PBE_PARAMS *)param->data;
	if(param->len == sizeof(CK_NETSCAPE_PBE_PARAMS)) {
	    pbe_iv.data = (unsigned char *)ns_pbe_params->pInitVector;
	    if(pbe_iv.data != NULL) {
	    	pbe_iv.len = 0;
		pbe_algid = SEC_PKCS5CreateAlgorithmID(algTag,
				&pbe_salt,
				(int)ns_pbe_params->ulIteration,
				(int)ns_pbe_params->ulEffectiveBits,
				&pbe_iv,
				NULL);
	    } else {
		pbe_algid = SEC_PKCS5CreateAlgorithmID(algTag,
				&pbe_salt,
				(int)ns_pbe_params->ulIteration,
				(int)ns_pbe_params->ulEffectiveBits,
				NULL,
				NULL);
	    }
	} else {
	    pbe_algid = SEC_PKCS5CreateAlgorithmID(algTag,
				&pbe_salt,
				(int)ns_pbe_params->ulIteration,
				0,
				NULL,
				NULL);
	}

	if(pbe_algid != NULL) {
	    rv = SECSuccess;
	} else {
	    rv = SECFailure;
	}
	break;

    default:
	if (pk11_lookup(type)->iv == 0) {
	    rv = SECSuccess;
	    newParams = NULL;
	    break;
	}
    case CKM_DES_CBC:
    case CKM_DES3_CBC:
    case CKM_IDEA_CBC:
    case CKM_SKIPJACK_CBC64:
    case CKM_SKIPJACK_ECB64:
    case CKM_SKIPJACK_OFB64:
    case CKM_SKIPJACK_CFB64:
    case CKM_SKIPJACK_CFB32:
    case CKM_SKIPJACK_CFB16:
    case CKM_SKIPJACK_CFB8:
    case CKM_BATON_ECB128:
    case CKM_BATON_ECB96:
    case CKM_BATON_CBC128:
    case CKM_BATON_COUNTER:
    case CKM_BATON_SHUFFLE:
    case CKM_JUNIPER_ECB128:
    case CKM_JUNIPER_CBC128:
    case CKM_JUNIPER_COUNTER:
    case CKM_JUNIPER_SHUFFLE:
	newParams = SEC_ASN1EncodeItem(NULL,NULL,param,
						SEC_OctetStringTemplate);
	rv = SECSuccess;
	break;
    }
    if (rv !=  SECSuccess) {
	if (newParams) SECITEM_FreeItem(newParams,PR_TRUE);
	return rv;
    }

    if(pbe_algid != NULL) {
	rv = SECOID_CopyAlgorithmID(arena, algid, pbe_algid);
	SECOID_DestroyAlgorithmID(pbe_algid, PR_TRUE);
    } else {
	rv = SECOID_SetAlgorithmID(arena, algid, algTag, newParams);
    }
    SECITEM_FreeItem(newParams,PR_TRUE);
    return rv;
}

CK_MECHANISM_TYPE
PK11_AlgtagToMechanism(SECOidTag algTag) {
    SECOidData *oid = SECOID_FindOIDByTag(algTag);

    if (oid) return (CK_MECHANISM_TYPE) oid->mechanism;
    return CKM_INVALID_MECHANISM;
}

/*
 * Build a block big enough to hold the data
 */
SECItem *
PK11_BlockData(SECItem *data,unsigned long size) {
    SECItem *newData;

    newData = (SECItem *)PORT_Alloc(sizeof(SECItem));
    if (newData == NULL) return NULL;

    newData->len = (data->len + (size-1))/size;
    newData->len *= size;

    newData->data = (unsigned char *) PORT_ZAlloc(newData->len);
    if (newData->data == NULL) {
	PORT_Free(newData);
	return NULL;
    }
    PORT_Memcpy(newData->data,data->data,data->len);
    return newData;
}


SECStatus
PK11_DestroyObject(PK11SlotInfo *slot,CK_OBJECT_HANDLE object) {
    CK_RV crv;

    crv = PK11_GETTAB(slot)->C_DestroyObject(slot->session,object);
    if (crv != CKR_OK) {
	return SECFailure;
    }
    return SECSuccess;
}

SECStatus
PK11_DestroyTokenObject(PK11SlotInfo *slot,CK_OBJECT_HANDLE object) {
    CK_RV crv;
    SECStatus rv = SECSuccess;
    CK_SESSION_HANDLE rwsession;

    
    rwsession = PK11_GetRWSession(slot);

    crv = PK11_GETTAB(slot)->C_DestroyObject(rwsession,object);
    if (crv != CKR_OK) {
	rv = SECFailure;
	PORT_SetError(PK11_MapError(crv));
    }
    PK11_RestoreROSession(slot,rwsession);
    return rv;
}

SECStatus
PK11_ReadAttribute(PK11SlotInfo *slot, CK_OBJECT_HANDLE id,
	 CK_ATTRIBUTE_TYPE type, PRArenaPool *arena, SECItem *result) {
    CK_ATTRIBUTE attr = { 0, NULL, 0 };
    CK_RV crv;

    attr.type = type;

    crv = PK11_GETTAB(slot)->C_GetAttributeValue(slot->session,id,&attr,1);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    if (arena) {
    	attr.pValue = PORT_ArenaAlloc(arena,attr.ulValueLen);
    } else {
    	attr.pValue = PORT_Alloc(attr.ulValueLen);
    }
    if (attr.pValue == NULL) return SECFailure;
    crv = PK11_GETTAB(slot)->C_GetAttributeValue(slot->session,id,&attr,1);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	if (!arena) PORT_Free(attr.pValue);
	return SECFailure;
    }

    result->data = attr.pValue;
    result->len = attr.ulValueLen;

    return SECSuccess;
}

static SECOidTag
pk11_MapPBEMechanismTypeToAlgtag(CK_MECHANISM_TYPE mech)
{
    switch(mech) {
	case CKM_PBE_MD2_DES_CBC:
	    return SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC;
	case CKM_PBE_MD5_DES_CBC:
	    return SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC;
	case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
	    return SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC;
	case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC;
	case CKM_NETSCAPE_PBE_SHA1_RC4:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC4;
	case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4;
	case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4;
	case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC2_CBC;
	case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC;
	case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
	    return SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC;
	default:
	    return SEC_OID_UNKNOWN;
    }
}

CK_RV
PK11_MapPBEMechanismToCryptoMechanism(CK_MECHANISM_PTR pPBEMechanism, 
				      CK_MECHANISM_PTR pCryptoMechanism,
				      SECItem *pbe_pwd)
{
    int iv_len = 0;
    CK_NETSCAPE_PBE_PARAMS_PTR pPBEparams;
    CK_RC2_CBC_PARAMS_PTR rc2_params;
    CK_ULONG rc2_key_len;
    SECStatus rv = SECFailure;
    SECAlgorithmID temp_algid;
    SECItem param, *iv;

    if((pPBEMechanism == NULL_PTR) || (pCryptoMechanism == NULL_PTR)) {
	return CKR_HOST_MEMORY;
    }

    pPBEparams = pPBEMechanism->pParameter;
    iv_len = PK11_GetIVLength(pPBEMechanism->mechanism);

    if(pPBEparams->pInitVector == NULL_PTR) {
	pPBEparams->pInitVector = (CK_CHAR_PTR)PORT_ZAlloc(iv_len);
	if(pPBEparams->pInitVector == NULL) {
	    return CKR_HOST_MEMORY;
	}
	param.data = pPBEMechanism->pParameter;
	param.len = pPBEMechanism->ulParameterLen;
	rv = PK11_ParamToAlgid(pk11_MapPBEMechanismTypeToAlgtag(
				  pPBEMechanism->mechanism),
				&param, NULL, &temp_algid);
	if(rv != SECSuccess) {
	    SECOID_DestroyAlgorithmID(&temp_algid, PR_FALSE);
	    return CKR_HOST_MEMORY;
	} else {
	    iv = SEC_PKCS5GetIV(&temp_algid, pbe_pwd);
	    if((iv == NULL) && (iv_len != 0)) {
		return CKR_HOST_MEMORY;
		SECOID_DestroyAlgorithmID(&temp_algid, PR_FALSE);
	    }
	    SECOID_DestroyAlgorithmID(&temp_algid, PR_FALSE);
	    if(iv != NULL) {
		PORT_Memcpy((char *)pPBEparams->pInitVector,
	    		    (char *)iv->data,
	    		    iv->len);
		SECITEM_ZfreeItem(iv, PR_TRUE);
	    }
	}
    }

    switch(pPBEMechanism->mechanism) {
	case CKM_PBE_MD2_DES_CBC:
	case CKM_PBE_MD5_DES_CBC:
	case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
	    pCryptoMechanism->mechanism = CKM_DES_CBC;
	    goto have_crypto_mechanism;
	case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
	    pCryptoMechanism->mechanism = CKM_DES3_CBC;
have_crypto_mechanism:
	    pCryptoMechanism->pParameter = PORT_Alloc(iv_len);
	    pCryptoMechanism->ulParameterLen = (CK_ULONG)iv_len;
	    if(pCryptoMechanism->pParameter == NULL) {
		return CKR_HOST_MEMORY;
	    }
	    PORT_Memcpy((unsigned char *)(pCryptoMechanism->pParameter),
			(unsigned char *)(pPBEparams->pInitVector),
			iv_len);
	    break;
	case CKM_NETSCAPE_PBE_SHA1_RC4:
	case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
	case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
	    pCryptoMechanism->mechanism = CKM_RC4;
	    pCryptoMechanism->ulParameterLen = 0;
	    pCryptoMechanism->pParameter = NULL_PTR;
	    break;
	case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
	    rc2_key_len = pPBEparams->ulEffectiveBits;
	    goto have_key_len;
	case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
	    rc2_key_len = 40;
	    goto have_key_len;
	case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
	    rc2_key_len = 128;
have_key_len:
	    pCryptoMechanism->mechanism = CKM_RC2_CBC;
	    pCryptoMechanism->ulParameterLen = (CK_ULONG)sizeof(CK_RC2_CBC_PARAMS);
	    pCryptoMechanism->pParameter = 
		(CK_RC2_CBC_PARAMS_PTR)PORT_ZAlloc(sizeof(CK_RC2_CBC_PARAMS));
	    if(pCryptoMechanism->pParameter == NULL) {
		return CKR_HOST_MEMORY;
	    }
	    rc2_params = (CK_RC2_CBC_PARAMS_PTR)pCryptoMechanism->pParameter;
	    PORT_Memcpy((unsigned char *)rc2_params->iv,
	    		(unsigned char *)pPBEparams->pInitVector,
	    		iv_len);
	    rc2_params->ulEffectiveBits = rc2_key_len;
	    break;
	default:
	    return CKR_MECHANISM_INVALID;
    }

    return CKR_OK;
}
