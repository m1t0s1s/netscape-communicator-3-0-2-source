/*
 * These functions keep track of the card state so we don't have to keep
 * querying the card.
 */
#include "sec.h"
#include "sslimpl.h"
#include "fortezza.h"
#include "fortdlgs.h"
#include "ssl.h"
#include "certdb.h"
#include "rsa.h" /* for big nums defines */
#include "prtime.h"
#ifdef USE_NSPR_MT
#include "nspr/prmon.h"
#endif
#if defined(SERVER_BUILD) && defined(XP_UNIX)
#undef PRIVATE
#include <sys/mman.h>
#endif

extern int SEC_ERROR_IO;
extern int XP_SEC_FORTEZZA_BAD_CARD;

FortezzaGlobal fortezzaGlobal = { 0 };

static void FortezzaRegisterFree(FortezzaPCMCIASocket *psock,FortezzaKey *key);
static int FortezzaFindCertIndexByNickname(FortezzaPCMCIASocket *p,char *c);

/*
 * These arrays are index by FortezzaCryptType. Changing the enum would
 * require changes to these arrays. Internal means save in the card's internal
 * storage. External means save to an external data area.
 */
unsigned int CI_External[] = {
	CI_ENCRYPT_EXT_TYPE,
	CI_DECRYPT_EXT_TYPE,
	CI_HASH_EXT_TYPE
};

/* return true if the cert for this bit is set in the cert flags */
static PRBool
FortezzaCertBits(unsigned char *bits, int cert) {
	int bitIndex = cert >> 3; /* cert div 8 */
	int bitLoc = 0x80 >> (cert & 0x7); /* cert mod 8 */

	return ((bits[bitIndex] & bitLoc) == bitLoc);
}

/* add a cert to the card */
static void
FortezzaSetCertBits(unsigned char *bits, int cert) {
	int bitIndex = cert >> 3; /* cert div 8 */
	int bitLoc = 0x80 >> (cert & 0x7); /* cert mod 8 */
	bits[bitIndex] |= bitLoc;
}

/* remove a cert from the card */
static void
FortezzaClrCertBits(unsigned char *bits, int cert) {
	int bitIndex = cert >> 3; /* cert div 8 */
	int bitLoc = 0x80 >> (cert & 0x7); /* cert mod 8 */
	bits[bitIndex] &= ~bitLoc;
}


/* Card Responses */
/* Library Errors */
static char *errorStr[] = {
	"CI_SRVR_ERROR",
	"CI_LIB_ALRDY_INIT",
	"CI_INVALID_FUNCTION",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"CI_NOT_A_CRYPTO_CARD",
	"CI_BAD_TUPLES",
	"CI_SOCKET_NOT_OPENED",
	"CI_NO_SOCKET",
	"CI_SOCKET_IN_USE",
	"CI_INV_SOCKET_INDEX",
	"Unknown",
	"Unknown",
	"CI_BAD_ADDR",
	"CI_BAD_IOSEEK",
	"CI_BAD_FLUSH",
	"CI_BAD_WRITE",
	"CI_BAD_SEEK",
	"CI_BAD_READ",
	"CI_BAD_IOCTL",
	"CI_BAD_CARD",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"CI_NO_SCTSRV",
	"CI_NO_CRDSRV",
	"CI_NO_DRIVER",
	"CI_NO_CARD",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"CI_OUT_OF_RESOURCES",
	"CI_BAD_PARAMETER",
	"CI_NO_EXECUTE",
	"CI_NO_ENCRYPT",
	"CI_NO_DECRYPT",
	"CI_BAD_SIZE",
	"CI_NULL_PTR",
	"CI_OUT_OF_MEMORY",
	"CI_TIME_OUT",
	"CI_CARD_IN_USE",
	"CI_CARD_NOT_READY",
	"CI_LIB_NOT_INIT",
	"CI_ERROR",
	"CI_OK",
	"CI_FAIL",
	"CI_CHECKWORD_FAIL",
	"CI_INV_TYPE",
	"CI_INV_MODE",
	"CI_INV_KEY_INDEX",
	"CI_INV_CERT_INDEX",
	"CI_INV_SIZE",
	"CI_INV_HEADER",
	"CI_INV_STATE",
	"CI_EXEC_FAIL",
	"CI_NO_KEY",
	"CI_NO_IV",
	"CI_NO_X",
	"Unknown",
	"CI_NO_SAVE",
	"CI_REG_IN_USE",
	"CI_INV_COMMAND",
	"CI_INV_POINTER",
	"CI_BAD_CLOCK",
	"CI_NO_DSA_PARMS",
};

static int errMax = sizeof(errorStr)/sizeof(errorStr[0]);
#define ERROFF 52

/*
 * Invalidate all the keys and context structs associated with a card that
 * has gone bad, or has been removed.
 *
 * Fortezza Error Must be called with CI_lock (to protect cx->state).
 * On servers FortezzaError logs error conditions.
 */
static void
FortezzaError(int error,FortezzaContext *cx,FortezzaPCMCIASocket *psock) {
	FortezzaContext 	*current;
	CI_PERSON		*personalityList;
	SECItem			*certList;
	CERTCertificate		*rootCert;
	HSESSION		hs = psock->maci_session;
	int i;

	/*
	 * card errors. often caused by bad data sent from
	 * our peer. Only reset our connection, not our card.
	 */
	if (error != CI_OK) PORT_SetError(SEC_ERROR_IO);
	if (error > 0) {
	    if (fortezzaGlobal.fortezzaAlert != NULL) {
	    	fortezzaGlobal.fortezzaAlert(fortezzaGlobal.fortezzaAlertArg,
			PR_FALSE,"Fortezza Card Error : %d (%s)\n", error,
		    	(error + ERROFF)<errMax ? errorStr[error+ERROFF]
			: "unknown");
	    }
	    /* SSL needs to free it */
	    if (cx) cx->state = BadCard;
	    return;
	}

	/* now handle library errors. These include errors about card
	 * insertion and what not. If these happen mark the whole socket
	 * as down */
	if (error < 0) {
	    if (fortezzaGlobal.fortezzaAlert != NULL) {
	        fortezzaGlobal.fortezzaAlert(fortezzaGlobal.fortezzaAlertArg,
			PR_FALSE,"Fortezza Library Error : %d (%s)\n", error,
				(error +ERROFF) > 0 ? errorStr[error+ERROFF]
				:  "unkown");
	    }
	}

	
	/* negative errors are library errors */
	/* tell everyone that we're gone */
	FORTEZZALOCK(psock->stateLock,psock->stateLockInit);
	psock->state = NoCard;
	PORT_Memset(psock->serial,0,sizeof(psock->serial));
	psock->load = 0;
	FORTEZZAUNLOCK(psock->stateLock);


	if (fortezzaGlobal.defaultSocket == psock) {
		fortezzaGlobal.defaultSocket = NULL;
	}

	MACI_Close(hs,CI_POWER_DOWN_FLAG,psock->socketId);
	/* ok, first we invalid all the keys */
	FORTEZZAPLOCK(psock->keyLock);
	for (i=0; i < psock->registerCount; i++) {
		if (psock->registers[i]) {
			FortezzaRegisterFree(psock,psock->registers[i]);
		}
		psock->registers[i] = NULL;
	}

	psock->registerCount = 0;
	FORTEZZAPUNLOCK(psock->keyLock);


	/* now dettach each context and mark them dead , as the
	 * ssl sockets they are attached to disappear, they will
	 * get freed */
	FORTEZZALOCK(psock->listLock,psock->listLockInit);
	for (current = psock->head; current; current = current->next) {
		current->state = BadCard;
	}
	psock->head=NULL;
	FORTEZZAUNLOCK(psock->listLock);


	/* we kill the cached certs and personalities */
	FORTEZZALOCK(psock->certLock,psock->certLockInit);
	certList = psock->certificateList;
	personalityList = psock->personalityList;
	rootCert = psock->rootCert;
	psock->certificateList = NULL;
	psock->personalityList = NULL;
	psock->rootCert = NULL;
	psock->certificateCount = 0;
	FORTEZZAUNLOCK(psock->certLock);
	if (rootCert) CERT_DestroyCertificate(rootCert);
	if (personalityList) PORT_Free(personalityList);
	if (certList) PORT_Free(certList);

	/* finally we decode the error */
        if (error != CI_OK) {
	    /* clear out XP_WouldBlock */
	    PORT_SetError(0); /* need to get and error code & map it*/
	}
}

/*
 * Save the current crypto state.  
 *
 * Environment: Caller must hold CI_LOCK();
 * 
 */
static SECStatus
FortezzaSaveState(FortezzaPCMCIASocket *psock,FortezzaContext *cx) {
	int error = CI_OK;
	HSESSION  hs = psock->maci_session;

	/*
	 * It looks like we need our personality set no matter what...
	 * so let's just set it
	 */
	error = MACI_Save(hs,CI_External[cx->type],cx->save);
	if (error != CI_OK) goto loser;

	cx->state = Saved;

	return SECSuccess;
loser:
	FortezzaError(error,cx,psock);
	return SECFailure;
}

/*
 * restore a saved state.  This routine is a lot simplier than it's
 * previous incarnation, which would keep track of which state is current.
 * It turns out that sneazing will trash your state, so we need to save
 * and restore our states early rather than delay.
 *
 * Environment: Caller must hold CI_LOCK();
 * 
 */
static SECStatus
FortezzaRestoreState(FortezzaPCMCIASocket *psock,FortezzaContext *cx) {
	int error = CI_OK;
	HSESSION  hs = psock->maci_session;
	FortezzaIV bogus_iv;

	if (cx->theKey == NULL) return SECFailure;

	/*
	 * It looks like we need our personality set no matter what...
	 * so let's just set it
	 */
	error = MACI_SetPersonality(hs,cx->certificate);
	if (error != CI_OK) goto loser;

	/* 
	 * We need to restore our state based on
	 * where it is. It can be in 1 of 5 places:
	 *	3) Saved - the state is saved in the context data structure.
	 *	4) Init - no state has been started yet.
	 */
	switch (cx->state) {
	case ServerInit:
		/* This is on purely to check Key escrow on server->client
		 * write */
		error = MACI_LoadIV(hs,cx->iv);
		if (error != CI_OK) {
			break;
		}
		if (cx->type == Encrypt) {
		    /* OK, now make Spyrus cards happy */
		    error = MACI_SetKey(hs,cx->theKey->keyRegister);
		    if (error != CI_OK) break;
		    error = MACI_GenerateIV(hs,cx->save);
		}
		break;
	case Saved:
		/*
		 * Sigh, the cards need to have some state bits set because
		 * save and restore don't necessarily save all the state.
		 * Instead of fixing the cards, they decided to change the
		 * protocol :(.
		 */
		if (cx->type == Encrypt) {
		    /* OK, now make Spyrus cards happy */
		    error = MACI_SetKey(hs,cx->theKey->keyRegister);
		    if (error != CI_OK) break;
		    error = MACI_GenerateIV(hs,bogus_iv);
		    if (error != CI_OK) break;
		} else if (cx->type == Decrypt) {
		    error = MACI_LoadIV(hs,cx->iv);
		    if (error != CI_OK) break;
		}
		error = MACI_Restore(hs,CI_External[cx->type],cx->save);
		break;
	case Init:
		error = MACI_LoadIV(hs,cx->iv);
		break;
	default:
		/* Shouldn't happen, set the error to something
		 * else that shouldn't happen as well.. */
		error = CI_INV_POINTER;
		break;
	}

	if (error != CI_OK) goto loser;
	return SECSuccess;

loser:
	FortezzaError(error,cx,psock);
	return SECFailure;
}


/*
 * pull more keys into the into the keyList.
 *  The keyListLock must  be held with this routine is called
 * NOTE: There is currently not Garbage collection. Once keys are allocated
 * they stay forever. Also NOTE: on the server (with shared memory) there is
 * no way to implement garbage collection. Once we touch a page of memory,
 * We can't un-touch it without freeing all of the shared memory segment.
 */
static FortezzaKey *FortezzaGetMoreKeys(unsigned char *mem_segment) {
	unsigned int count = (NEW_KEY_COUNT+1)*sizeof(FortezzaKey);
	unsigned char *maddr;
	int i;

#if defined(SERVER_BUILD) && defined(XP_UNIX)
	unsigned int len;

	len = (unsigned int) *(unsigned long *)mem_segment;;
	maddr = mem_segment + len;
	if (len + count >= FORTEZZA_MAX_SHARED_MEM) {
	    return NULL;
	}
	*(unsigned long *)mem_segment = (unsigned long)(len + count);
#else
	FORTEZZAPUNLOCK(fortezzaGlobal.keyListLock);
	maddr = PORT_Alloc(count);
	if (maddr == NULL) {
	    return NULL;
	}
	FORTEZZAPLOCK(fortezzaGlobal.keyListLock);
#endif

	for (i=0; i < NEW_KEY_COUNT; i++) {
	    FortezzaKey *key = (FortezzaKey *)maddr;
	    maddr += sizeof(FortezzaKey);
	    key->next = fortezzaGlobal.keyTop;
	    key->id = 0;
	    fortezzaGlobal.keyTop = key;
	}
	return (FortezzaKey *)maddr;
}


/*
 * Allocate a new key. 
 *  On multi-process servers key's need to be allocated out of shared memory.
 *  Since we need to allocate keys off lists anyway, we use the same structure
 *  for allocating keys for clients. Note: there is no garbage collection.
 *  Once memory has been put to getting keys, it will never be freed.
 */
FortezzaKey *FortezzaKeyAlloc(FortezzaKEAContext *cx) { 
        FortezzaKey *key;
	unsigned long id;

	FORTEZZAPLOCK(fortezzaGlobal.keyListLock);
	/* get a uniq non-zero keyid... depends on the fact that old keys
	 * will get freed before an unsigned long will wrap... only Servers
	 * actuall care about these */
	id = (*fortezzaGlobal.keyid)++;
	if (id == 0) id = (*fortezzaGlobal.keyid)++;

	/* now get a key off the list */
	key = fortezzaGlobal.keyTop;
	if (key) {
	     fortezzaGlobal.keyTop = key->next;
	     key->next = NULL;
	} else {
	     /* no key on the list, get more keys into the list */
	     key = FortezzaGetMoreKeys(fortezzaGlobal.memaddr);
	}
	FORTEZZAPUNLOCK(fortezzaGlobal.keyListLock);

        if (key) {
             key->keyRegister = KeyNotLoaded;
             key->hitCount = 0;
             key->keyType = NOKEY;
             key->ref_count = 1;
	     key->socket = cx->socket;
	     key->id = id;
	     key->cached = PR_FALSE;
        }
        return key;
}


/*
 * Tek's have extra fields to initialize. Most of the work is done by
 * FortezzaKeyAlloc.
 */
FortezzaKey *FortezzaTEKAlloc(FortezzaKEAContext *cx) {
        FortezzaKey *tek;
 
        tek = FortezzaKeyAlloc(cx);
        if (tek) {
             tek->keyType = TEK;
             tek->keyData.tek.destroy_certificate = 0;
        }
        return tek;
}


/*
 * There are only a fixed number (usually 10) of key registers on the
 * fortezza card. We manage these registers based on an LRU sheme.
 * Each key structure carries enough information to reload itself if
 * necessary. FortezzaRegisterAlloc finds a free slot, or if there is
 * no free slot, throw out the oldest key.
 * 
 * NOTE: maci is also managing the keys, those keys we don't know anything
 * about. That's ok because there aren't likely going to be very many others
 * using the server's cards at the same time as the server!
 *
 * Environment: Caller must hold CI_LOCK();
 */
static int
FortezzaRegisterAlloc(FortezzaPCMCIASocket *psock,
					FortezzaContext *cx,FortezzaKey *key) {
	int i;
	uint32 minHitCount= 0xffffffff; /* MAX INT */
	FortezzaKeyRegister *registers;
	HSESSION  hs = psock->maci_session;
	FortezzaTEK	*tek;
	int candidate = 1;
	int tek_reg = 0;
	int error;

	/* paranoia */
	if (key == NULL) return KeyNotLoaded;

	/* register '0' is reserved, so we start by looking at register '1' */
	FORTEZZAPLOCK(psock->keyLock);
	registers = psock->registers;

	if (registers == NULL) {
		FORTEZZAPUNLOCK(psock->keyLock);
		return KeyNotLoaded;
	}
	for (i=1; i < psock->registerCount; i++) {

		/* Ok, we found an emply slot */
		if (registers[i] == NULL) {
			candidate = i;
			break;
		}

		/* If this one is the oldest, he may be the one we toss */
		if (registers[i]->hitCount < minHitCount) {
			minHitCount = registers[i]->hitCount;
			candidate = i;
		}
	}
	if (registers[candidate] != NULL) {
		registers[candidate]->keyRegister = KeyNotLoaded;
		registers[candidate] = NULL;
	}

	/* the key is loaded, update our data structures to reflect reality */
	key->keyRegister = candidate;
	registers[candidate] = key;

	/* definately have touched this key */
	key->hitCount = psock->hitCount++;
	FORTEZZAPUNLOCK(psock->keyLock);

	MACI_DeleteKey(hs,candidate);
	switch (key->keyType) {
	case TEKMEK:
		/* First make sure the tek is loaded */
		tek_reg = key->keyData.tek_mek.wrappingTEK->keyRegister; 
		if (tek_reg == KeyNotLoaded) {
		    tek_reg = FortezzaRegisterAlloc(psock,cx,
					key->keyData.tek_mek.wrappingTEK);
		}
		/* now unwrap the key */
		error = MACI_UnwrapKey(hs,tek_reg,candidate,
						key->keyData.tek_mek.TEK_M);
		if (error != CI_OK) {
			break;
		}

		/* now convert it to a Ks wrapped mek */
		FortezzaTEKFree(key->keyData.tek_mek.wrappingTEK);
		key->keyType = MEK;
		error = MACI_WrapKey(hs,0,candidate,key->keyData.mek.Ks_M);
		break;
		
	case MEK:
		error = MACI_UnwrapKey(hs,0,candidate,key->keyData.mek.Ks_M);
		break;
	case TEK:
		tek= &key->keyData.tek;
		error = MACI_SetPersonality(hs,tek->certificate);
		if (error == CI_OK) {
			error = MACI_GenerateTEK(hs,tek->flags,candidate,
			    tek->R_a,tek->R_b,tek->Y_bSize,tek->Y_b);
		}
		break;
	/* get a slot to Generate or load an MEK into */
	case NOKEY:
		error = CI_OK;
		break;
	default:
		error = -1;
		break;
	}
	if (error != CI_OK) {
		/* this clears out all the key structures */
		FortezzaError(error,cx,psock); 
		return KeyNotLoaded;
	}

	return candidate;
}

/*
 * When we're through with a key, free up the slot.
 *
 * Enviroment: Caller must hold psock->keylock.
 */
static void 
FortezzaRegisterFree(FortezzaPCMCIASocket *psock,FortezzaKey *key) {
	int reg = key->keyRegister;

	key->keyRegister = KeyNotLoaded;
	if (reg != KeyNotLoaded) {
		if (reg < psock->registerCount) {
			PORT_Assert(psock->registers[reg] == key);
			psock->registers[reg] = NULL;
		}
	}
}


/*
 * pre-encrypted files use this. It takes a K_s wrapped key (from some file)
 * and rewappes it with the session tek.
 */
SECStatus
FortezzaRewrapKey(FortezzaKEAContext *kea_cx, FortezzaSymetric inkey,
                  FortezzaSymetric wrapped_key)
{
    FortezzaPCMCIASocket *psock;
    int key_reg, tek_reg, error;
    FortezzaKey *key, *tek;
    HSESSION  hs;
   
    psock = (FortezzaPCMCIASocket *)kea_cx->socket;
    if (psock == NULL) goto loser;
 
    hs = psock->maci_session;

    tek = (FortezzaKey *)kea_cx->tek;
    if (tek == NULL) goto loser;

    /* allocate a key slot to use for unwrapping and wrapping */
    key = FortezzaKeyAlloc(kea_cx);
    if (key == NULL) goto loser;

    CI_LOCK(psock);
    key_reg = FortezzaRegisterAlloc(psock, NULL, key);
    if (key_reg == KeyNotLoaded) goto loser;
   
    FortezzaRestoreKey(psock, NULL, tek);

    tek_reg = tek->keyRegister;
    if (tek_reg == KeyNotLoaded) goto loser;

    /* unwrap the local Ks wrapped MEK */
    error = MACI_UnwrapKey(hs,0, key_reg, inkey);

    /* rewrap the MEK with the session TEK */
    error = MACI_WrapKey(hs,tek_reg, key_reg, wrapped_key);

    CI_UNLOCK(psock);

    FortezzaFreeKey(key);

    if (error != CI_OK) {
        FortezzaError(error, NULL, psock);
        return(SECFailure);
    }
    return(SECSuccess);

loser:
    CI_UNLOCK(psock);
    return SECFailure;
}

			
/*
 * restore the current key to a key register and set it.
 * Question, do I really need to send the whole context just to restore
 * a single key? Does it hurt?
 *
 * Environment: Caller must hold CI_LOCK()
 */
SECStatus
FortezzaRestoreKey(FortezzaPCMCIASocket *psock,FortezzaContext *cx,
							FortezzaKey *key) {
	int	keyRegister;
	int error;

	/* first make sure we have our key in place */
	keyRegister = key->keyRegister;
	if (keyRegister == KeyNotLoaded) {
		keyRegister = FortezzaRegisterAlloc(psock,cx,key);
		/* FortezzaRegisterAlloc has already loaded our key.
		 * if there was an error, he has already called 
		 * the fortezza error routine.
		 */
		if (keyRegister == KeyNotLoaded) {
			goto loser;
		}
	}

	/* it looks like the key setting can be trashed pretty easily,
	 * so for now we always just set the Key */
	error= MACI_SetKey(psock->maci_session,keyRegister);
	if (error != CI_OK) {
		FortezzaError(error,cx,psock);
		goto loser;
	}

	/* we've touched the key ... question is 32 bits long enough */
	key->hitCount = psock->hitCount++; 
	return SECSuccess;

loser:
	return SECFailure;
}

/*
 * this does an encrypt, decrypt, or hash depending on the value of in
 * the context.
 */
SECStatus Fortezza_Op(FortezzaContext *cx, unsigned char *out, unsigned *part, 
		unsigned int maxOut, unsigned char *in, unsigned inLen)
{

	FortezzaPCMCIASocket	*psock;
	int error = CI_OK;
	unsigned tmpSize = 0;
	unsigned char *tmp = NULL;
	unsigned char *loopin,*loopout;
	unsigned left;
	PRBool encrypt_hack = PR_FALSE;
	SECStatus rv;
	HSESSION hs;

static unsigned char pad[] = 
	{ 0xde, 0xad, 0xbe, 0xef,  0xde, 0xad, 0xbe, 0xef };

	
	/* 
	 * CI_LOCK locks the whole CI_Library. Since is is such a
	 * coarse grain lock, we use smaller grain locks on some of
	 * the key data structures. To prevent dead locks, you cannot
	 * call CI_LOCK() while you are already holding a finer grain
	 * lock.
	 */

	/* the card has been removed on this stream, no way to
	 * recover. Just keep yelling until the upper level
	 * code gives up and frees us.
	 */
	if ((cx->state == BadCard) || (cx->state == NotInit)) {
		if (part) *part = 0;
		return SECFailure;
	}

	psock = cx->socket;

	if (psock->state == NoCard) {
		if (part) *part = 0;
		return SECFailure;
	}

	hs = psock->maci_session;

	CI_LOCK(psock); 

#ifdef notdef
	/*
	 * Always force a select!
	 */
	error = MACI_Select(hs,psock->socketId);
	if (error != CI_OK) {
		FortezzaError(error,cx,psock);
		goto loser;
	}
#endif

	/*
	 * restore the key if necessary
	 */
	if (cx->type != Hash) {
	    if (FortezzaRestoreKey(psock,cx,cx->theKey) != SECSuccess) {
		goto loser;
	    }
	}

	/* update the key hit */
	cx->theKey->hitCount = psock->hitCount++;

	/* restore the state */
	if (cx->state == ServerInit) {
	    encrypt_hack = PR_TRUE;
	}
	if (FortezzaRestoreState(psock,cx) != SECSuccess) {
		goto loser;
	}

	/*
	 * sigh, save & restore is not working properly, so we let the
	 * first 8 bytes of the block get trashed.
	 */
	if (encrypt_hack) {
	   tmpSize = inLen+8;
	   tmp = PORT_Alloc(tmpSize);
	   if (tmp == NULL) { goto loser; }
	}

	/* OK, we're ready to encrypt, decrypt or hash */
	switch (cx->type) {
	case Encrypt:
		if (encrypt_hack) {
		    PORT_Memcpy(tmp,pad,8);
		    PORT_Memcpy(tmp+8,in,inLen);
		    inLen += 8;
		    loopin = tmp;
		    loopout = out;
		} else {
		    loopin = in;
		    loopout = out;
		}
		left = inLen;
		while((left > 0) && (error == CI_OK)) {
		    /* Hack, should look at card 's ramsize value */
		    unsigned current = left > 0x8000 ? 0x8000 : left;
		    error = MACI_Encrypt(hs,current,loopin,loopout);
		    loopin += current;
		    loopout += current;
		    left -= current;
		}
		break;
	case Decrypt:
		loopout = encrypt_hack ? tmp : out;
		loopin = in;
		left = inLen;
		while((left > 0) && (error == CI_OK)) {
		    /* Hack, should look at card 's ramsize value */
		    unsigned current = left > 0x8000 ? 0x8000 : left;
		    error = MACI_Decrypt(hs,current,loopin,loopout);
		    loopin += current;
		    loopout += current;
		    left -= current;
		}
		if (encrypt_hack) {
		    PORT_Assert(inLen >= 8);
		    if (inLen < 8) inLen = 8;
		    inLen -= 8;
		    PORT_Memcpy(out,tmp+8,inLen);
		}
		break;
	case Hash:
		/* if buffer is supplied, we return the hashed value */
		if (out) {
			error = MACI_GetHash(hs,inLen,in,out);
			inLen = 20;
		} else {
			error = MACI_Hash(hs,inLen,in);
		}
		break;
	default:
		error = CI_ERROR;
		break;
	}
	/* tmp had plain text, zero it on freeing */
	if (encrypt_hack) {
	    PORT_ZFree(tmp,tmpSize);
	}

	if (error != CI_OK) {
		FortezzaError(error,cx,psock);
		goto loser;
	}

	rv = FortezzaSaveState(psock,cx);
	if (rv < 0) goto loser;
	CI_UNLOCK(psock);


	if (part) *part = inLen;

	
	return SECSuccess;

loser:	
	CI_UNLOCK(psock);
	if (part) *part = 0;
	return SECFailure;
}

/*
 * We need a different routine to handle the premaster secret, as the premaster
 * secret does not use the trick of allowing the first 8 bytes to get trashed.
 * Actually we can now do without this, and use fortezza_op, but there are more
 * important things to clean up...
 */
SECStatus FortezzaCryptPremaster(FortezzaCryptType type, 
 FortezzaKEAContext *cx,FortezzaKey *fk, FortezzaIV iv, unsigned char *in, 
					unsigned char *out, int inLen) {

	FortezzaPCMCIASocket	*psock;
	HSESSION hs;
	int error;

	
	/* 
	 * CI_LOCK locks the whole CI_Library. Since is is such a
	 * coarse grain lock, we use smaller grain locks on some of
	 * the key data structures. To prevent dead locks, you cannot
	 * call CI_LOCK() while you are already holding a finer grain
	 * lock.
	 */
	PORT_Assert(cx->socketId == cx->socket->socketId) ;
	psock = cx->socket;

	hs = psock->maci_session;

	CI_LOCK(psock); 
	if (fk->keyType != TEK) goto loser;


	error = MACI_SetPersonality(hs,fk->keyData.tek.certificate);

	/*
	 * restore the key
	 */
	 if (FortezzaRestoreKey(psock,NULL,fk) != SECSuccess) {
		goto loser;
	 }


	/* OK, we're ready to encrypt, decrypt or hash */
	switch (type) {
	case Encrypt:
	    error = MACI_GenerateIV(hs,iv);
	    if (error == CI_OK) {
		error = MACI_Encrypt(hs,inLen,in,out);
	    }
	    break;
	case Decrypt:
	    error = MACI_LoadIV(hs,iv);
	    if (error == CI_OK) {
		error = MACI_Decrypt(hs,inLen,in,out);
	    }
	    break;
	default:
	    error = CI_ERROR;
	    break;
	}

	if (error != CI_OK) {
		FortezzaError(error,NULL,psock);
		goto loser;
	}
	CI_UNLOCK(psock);
	
	return SECSuccess;

loser:	
	CI_UNLOCK(psock);
	return SECFailure;
}



/*
 * create a new context and link it into the card's socket structure
 */
FortezzaContext *
FortezzaCreateContext(FortezzaKEAContext *template, FortezzaCryptType type) {
	FortezzaPCMCIASocket *psock = template->socket;
	FortezzaContext *cx;

	PORT_Assert(template->socketId == template->socket->socketId) ;

	cx = PORT_ZAlloc(sizeof(FortezzaContext));
	

	cx->state = NotInit;
	cx->type = type;
	cx->next = NULL;
	cx->theKey = NULL;
	cx->certificate = template->certificate;
	cx->socketId = template->socketId;

	FORTEZZALOCK(psock->listLock,psock->listLockInit);
	psock->load++;
	cx->socket = psock;
	if (psock->head) {
		cx->next = psock->head->next;
	} 
	psock->head = cx;
	FORTEZZAUNLOCK(psock->listLock);

	return cx;
}


/*
 * KEA contexts are used by external code to represent sockets. It also is
 * used by SSL to store Key exchange specific contexts like the current 
 * session's tek, or the Server Randon number.
 */
FortezzaKEAContext *
FortezzaCreateKEAContext(FortezzaPCMCIASocket *psock) {
	FortezzaKEAContext *template = &psock->template;
	FortezzaKEAContext *cx;

	cx = PORT_ZAlloc(sizeof(FortezzaKEAContext));
	
	if (cx) {
	     cx->certificate = template->certificate;
	     cx->socket = psock;
	     cx->socketId = psock->socketId;
	     cx->tek = NULL;
	}

	return cx;
}

/*
 * get a context from a fortezza socket id
 */
FortezzaKEAContext *
FortezzaCreateKEAContextFromSocket(int socket) {
	FortezzaPCMCIASocket *psock;

	if ((socket <= 0) || (socket > fortezzaGlobal.socketCount)) {
		return NULL;
	}
	psock  = &fortezzaGlobal.sockets[socket-1];

	return(FortezzaCreateKEAContext(psock));
}

/* Destroy a kea context */
void
FortezzaDestroyKEAContext(FortezzaKEAContext *kcx,PRBool freeit) {
	PORT_Assert(kcx->socketId == kcx->socket->socketId) ;
	if (kcx->tek) {
	    FortezzaTEKFree(kcx->tek);
	    kcx->tek = NULL;
	}
	if (freeit) {
	    PORT_Free(kcx);
	}
	return; 
}


/* Destroy a crypto context */
void
FortezzaDestroyContext(FortezzaContext *cx,PRBool freeit) {
	FortezzaPCMCIASocket *psock = cx->socket;
	FortezzaContext **last,*current;

	cx->state = BadCard;
	if (cx->theKey) {
	    FortezzaFreeKey(cx->theKey);
	}

	FORTEZZALOCK(psock->listLock,psock->listLockInit);
	/* remove from the list */
	for (last = &psock->head, current=*last; *last; 
				last = &current->next,current = *last) {
	    if (current == cx) {
		*last = current->next;
		if (psock->load > 0) psock->load--;
		   break;
	    }
	}
	FORTEZZAUNLOCK(psock->listLock);

	if (freeit) PORT_Free(cx);
	return;
}


/*
 * set the current context key with tha given key.
 * (NOTE: context must not have a key already set! );
 */
SECStatus
FortezzaSetKey(FortezzaContext *cx, FortezzaKey *key) {
	FortezzaPCMCIASocket *psock = cx->socket;

	PORT_Assert(cx->theKey == NULL);
	cx->theKey = key;
	FORTEZZAPLOCK(psock->keyLock);
	key->ref_count++;
	FORTEZZAPUNLOCK(psock->keyLock);

	return SECSuccess;
}

/*
 * Generate a new MEK for this context and return it wrapped with the tek
 */
SECStatus
FortezzaGenerateKeys(FortezzaContext *cx, FortezzaKey *tek, 
					FortezzaSymetric wrapped_key) {
	FortezzaPCMCIASocket *psock = cx->socket;
	HSESSION  hs = psock->maci_session;
	int error,key_reg,tek_reg;

	CI_LOCK(psock);


	/* Get a Free Key Register to store the Key in */
	PORT_Assert(cx->theKey == NULL);
	cx->theKey = FortezzaKeyAlloc(&psock->template);
	if (cx->theKey == NULL) { goto loser; }
	key_reg = FortezzaRegisterAlloc(psock,cx,cx->theKey);
	if (key_reg == KeyNotLoaded) { goto loser; }

	/* make sure the tek is loaded */
	tek_reg = tek->keyRegister;
	if (tek_reg == KeyNotLoaded)  {
		tek_reg = FortezzaRegisterAlloc(psock,cx,tek);
		if (tek_reg == KeyNotLoaded) { goto loser; }
	}

	/* generate a new MEK */
	error = MACI_GenerateMEK(hs,key_reg,0);
	if (error != CI_OK) { goto process_loser; }

	/* Wrap that puppy with the tek for transport over the net */
	error = MACI_WrapKey(hs,tek_reg,key_reg,wrapped_key);
	if (error != CI_OK) { goto process_loser; }

	/* Wrap that puppy with Ks for local storage */
	error = MACI_WrapKey(hs,0,key_reg,cx->theKey->keyData.mek.Ks_M);
	if (error != CI_OK) { goto process_loser; }

	cx->theKey->keyType = MEK;

	CI_UNLOCK(psock);
	return SECSuccess;

process_loser:
	FortezzaError(error,cx,psock);
loser:
	CI_UNLOCK(psock);
	return SECFailure;
}

/*
 * load a tek wrapped MEK into this context
 */
SECStatus
FortezzaUnwrapKey(FortezzaContext *cx, FortezzaKey *tek, 
					FortezzaSymetric wrapped_key) {
	/*
	 * delay the unwrapping
	 * of the key until we load the context. This will limit the
	 * number of CI_LOCK calls needed on the server side to just
	 * '2' to create a connection (once to generate the server random
	 * and once to decrypt the pre-master secret). We already
	 * delay load IV to context loading time. NOTE: to do this we
	 * need to store the TEK in the FortezzaKey structure, and
	 * add a reference count to TEK.
	 */
	FortezzaPCMCIASocket *psock = cx->socket;

	PORT_Assert(cx->theKey == NULL);
	cx->theKey = FortezzaKeyAlloc(&cx->socket->template);
	if (cx->theKey == NULL) return SECFailure;

	if (tek != NULL) {
		cx->theKey->keyType = TEKMEK;
		cx->theKey->keyData.tek_mek.wrappingTEK = tek;
		FORTEZZAPLOCK(psock->keyLock);
		tek->ref_count++;
		FORTEZZAPUNLOCK(psock->keyLock);

		PORT_Memcpy(cx->theKey->keyData.tek_mek.TEK_M,wrapped_key,
					sizeof(FortezzaSymetric));
	} else {
		cx->theKey->keyType = MEK;
		PORT_Memcpy(cx->theKey->keyData.mek.Ks_M,wrapped_key,
					sizeof(FortezzaSymetric));
	}
	return SECSuccess;
}


/*
 * Generate a new iv for the key in the context.
 * This also changes the context state from NoInit to OnLine
 */
SECStatus
FortezzaGenerateIV(FortezzaContext *cx, FortezzaIV iv) {
	FortezzaPCMCIASocket *psock = cx->socket;
	HSESSION  hs = psock->maci_session;
	int error;
	SECStatus rv = SECSuccess;

	CI_LOCK(psock);


	/* load the key */
	rv = FortezzaRestoreKey(psock,cx,cx->theKey);
	if (rv < 0) goto loser;


	/* build the IV */
	error = MACI_GenerateIV(hs,iv);
	if (error != CI_OK) {
		FortezzaError(error,cx,psock);
		goto loser;
	}

	PORT_Memcpy(cx->iv,iv,sizeof(FortezzaIV));
	if (cx->type == Decrypt) {
	    cx->state = ServerInit;
	} else {
	    rv = FortezzaSaveState(psock,cx);
	    cx->state = Saved;
	}
	if (rv < 0) goto loser;


	CI_UNLOCK(psock);
	return SECSuccess;
loser:
	
	CI_UNLOCK(psock);
	return SECFailure;
}


/*
 * load's iv into the save area and sets the state to Init.
 * iv gets loaded when we decide to start an encrypt or decrypt
 */
SECStatus
FortezzaLoadIV(FortezzaContext *cx, FortezzaIV iv) {

	PORT_Memcpy(cx->iv,iv,sizeof(FortezzaIV));
	if (cx->type == Encrypt) {
	    cx->state = ServerInit;
        } else {
	    cx->state = Init;
	}

	return SECSuccess;
}

/*
 * force a reloaded decrypt context into ServerInit state.
 */
SECStatus
FortezzaSetServerInit(FortezzaContext *cx) {
    PORT_Assert(cx->type == Decrypt);
    cx->state = ServerInit;
    return SECSuccess;
}

SECStatus
FortezzaLoadSave(FortezzaContext *cx, FortezzaCryptSave save) {
	PORT_Memcpy(cx->save,save,sizeof(FortezzaCryptSave));

	cx->state = Saved;

	return SECSuccess;
}

/*
 * look up all the personalities that we have valid keys for
 * and return them in an array. (which needs to be freed below).
 */
SECStatus
FortezzaLookupPersonality(FortezzaPCMCIASocket *psock, 
		char ***personalities, int *found,int **index) {
    char **parray;
    int next = 0;
    int count,i;

    count =psock->certificateCount;
    /* define parray to manage the triple indirection better...*/
    (*personalities) = parray = (char **)
			           PORT_ZAlloc((count+1)*sizeof(char *));
    if (parray == NULL) {
	return SECFailure;
    }
    if (index) {
	(*index) = (int *) PORT_ZAlloc(count * sizeof(int));
	if (*index == NULL) {
	    PORT_Free(parray);
	    return SECFailure;
	}
    }


    for (i=1; i < count; i++) {
	int cert;
	CI_CERT_STR  name;

	FORTEZZALOCK(psock->certLock,psock->certLockInit);
	/* deal with the race condition */
	if ( i >= psock->certificateCount) {
	    FORTEZZAUNLOCK(psock->certLock);
	    break;
	}

	cert = (int)psock->personalityList[i].CertificateIndex;
	if (cert >= psock->certificateCount) {
	    FORTEZZAUNLOCK(psock->certLock);
	    continue;
	}

	if (!FortezzaCertBits(psock->certFlags, cert)) {
	    FORTEZZAUNLOCK(psock->certLock);
	    continue;
	}
 	PORT_Memcpy(name,psock->personalityList[i].CertLabel,
						sizeof(CI_CERT_STR));
	FORTEZZAUNLOCK(psock->certLock);

	/* Only use certificates we have keys for  */
	if (name[2]  != 'K' && name[3] != 'S' ) {
	    continue;
	}	
	parray[next] = PORT_Alloc(sizeof(CI_CERT_STR));
	if (parray[next] == NULL) {
	    FortezzaFreePersonality(parray);
	    if (index) PORT_Free(*index);
	    return SECFailure;
	    
	}
 	PORT_Memcpy(parray[next],&name[8], sizeof(CI_CERT_STR)-8);
	parray[next][sizeof(CI_CERT_STR)-8] = 0;
	if (index)  (*index)[next] = cert;
	next++;
    }
    *found = next;
    return SECSuccess;
}

/* Free a list of personalities found by lookup personalities */
void
FortezzaFreePersonality(char **personalities) {
	char **parray = personalities;

	while (*parray) PORT_Free(*parray++);
	PORT_Free(personalities);
}

/*
 * Find the "best" certificate in the card to use. If there is any
 * doubt, and we are on a client, ask the user.
 */
SECStatus
FortezzaSelectCertificate(SSLSocket *ss,FortezzaPCMCIASocket *psock,
							int *certificate) {
	SSLSecurityInfo *sec = NULL;
	SECStatus rv = SECSuccess;
	char **personalities;
	int *index;
	int count = 0;

	if (ss) sec = ss->sec;

        rv = FortezzaLookupPersonality(psock,&personalities,&count,&index);
	if (rv < 0) return rv;

	switch (count) {
	case 0:
	    /* No certificates, we fail */
	    *certificate= 0;
	    rv = SECFailure;
	    break;
	case 1:
	    /* Only one certificate, we use it */
	    *certificate = index[0];
	    break;
	default:
	    /* more than one and we're a client */
	    if (sec && sec->fortezzaCertificateSelect) {
		int cert;

		rv = (*sec->fortezzaCertificateSelect)
		 (sec->fortezzaCertificateArg,ss->fd,personalities,count,&cert);

		if (rv < 0) break;

		/* make sure we weren't a victim of FortezzaError. */
		if (index[cert-1] > psock->certificateCount) {
			*certificate = -1;
			rv = SECFailure;
			break;
		}		
		*certificate=index[cert-1];
	    /* more than one and we're a server with a specified cert */
	    } else if (fortezzaGlobal.serverCertificate) {
		*certificate = fortezzaGlobal.serverCertificate;
	    } else {
		/* we're on the server, and no cert has been selected, use
		 * the first valid one */
		*certificate=index[0];
	    }
	    break;
	}

	FortezzaFreePersonality(personalities);
	PORT_Free(index);
	
	return rv;
}

/*
 * check the pin. This returns 3 different types of values:
 *	1) SECSuccess... we have the correct pin.
 *	2) SECWouldBlock... we have the incorrect pin, but ths card is still
 *   functioning, try again with another pin.
 *	3) SECFailure... The card is out to lunch (probably because we tried to
 *   many invalid pin attempts.
 */
SECStatus
FortezzaPinCheck(FortezzaPCMCIASocket *psock,char *pin) {
	int error;
	HSESSION hs=psock->maci_session;

	CI_LOCK(psock);

	error = MACI_CheckPIN(hs,CI_USER_PIN,(unsigned char *)pin);
	CI_UNLOCK(psock);
	if (error != CI_OK) { 
	    /* failed because incorrect pin, try again */
	    if (error == CI_FAIL) {
		return SECWouldBlock;
	    }
	    /* failed because of library error or card problem */
	    FortezzaError(error,NULL,psock);
	    return SECFailure;
	}
	return SECSuccess;
}
	
/*
 * Fortezza certificates are 2048 bytes on the card, but most of those
 * bytes are '0'. We parse the starting sequence header to get the
 * true certificate length.
 */
static int
FortezzaCertLength(unsigned char *DerData) {
	int length = 0;
	unsigned char *dp = (unsigned char *)DerData;
	unsigned char tag = *dp++;
	int consumed;

	if (tag & 0x1f != 0x10 /* SEQUENCE */) {
		return 0;
	}

	length = *dp++;

	if (length & 0x80) {
	    int len_count = length & 0x7f;

	    length = 0;
	    while (len_count-- > 0) {
		length = (length << 8) | *dp++;
	    }
	}
	consumed = dp - (unsigned char *)DerData;
	length += consumed;

	/* Don't blow up on invalide certs */
	if (length > 2048) { 
	    length=2048;
	} else if (length < 0) {
	    length = 0;
	}
	return length;
}



/*
 * load up the certificates from the card into memory.
 */
SECStatus
FortezzaCertInit(FortezzaPCMCIASocket *psock) {
	SECItem *certListFree, *certificateList = NULL;
	CI_PERSON *personListFree, *personalityList = NULL;
	CI_STATUS status;
	CERTCertificate *rootCert = NULL, *rootCertFree;
	HSESSION hs=psock->maci_session;
	CERTCertTrust	trust;
	char *name;
	int error, i;
	SECStatus rv;

	/* now we read in the certificate list and personality list. Why
	 * didn't we when we initted the psock structure?? simple.. we needed
	 * the pin to read these...
	 */

	CI_LOCK(psock);

	/* then we get the current card state */
	error = MACI_GetStatus(hs,&status);
	if (error != CI_OK) { goto loser; }

	/* finally we read in the personalities */
	personalityList = (CI_PERSON *) PORT_ZAlloc
				(sizeof(CI_PERSON)*status.CertificateCount);
	if (personalityList == NULL) { goto loser; }

	error = MACI_GetPersonalityList(hs,status.CertificateCount,
							personalityList);
	if (error != CI_OK) { goto loser; }

	certificateList = (SECItem *) PORT_ZAlloc
				(sizeof(SECItem)*status.CertificateCount);
	if (certificateList == NULL) { goto loser; }

	for (i = 0; i < status.CertificateCount; i++) {
	    if (FortezzaCertBits(psock->certFlags, i)) {
		certificateList[i].data = (unsigned char *)PORT_ZAlloc(2048);
		if (certificateList[i].data == NULL) { goto loser; }
		error = MACI_GetCertificate(hs,i,certificateList[i].data);
		if (error != CI_OK) { goto loser; }
		certificateList[i].len=FortezzaCertLength(certificateList[i].data);
	    } else {
		certificateList[i].data = NULL;
		certificateList[i].len = 0;
	    }
	}
	CI_UNLOCK(psock);



	/* Store the Root Cert at trusted */

	/* create the root certificate */
	rootCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
		    &certificateList[0], NULL, PR_FALSE, PR_TRUE);
	if (rootCert == NULL) goto loser;

	/* make sure it's valid (self signed) */
	rv = CERT_VerifySignedData(&rootCert->signatureWrap, rootCert);
	if (rv < 0) goto loser;

	/* make it trusted */
	/* this code was swiped from certdlgs.c -- session option */
	if (!rootCert->isperm) {
	    name = CERT_GetCommonName(&rootCert->subject);
	    if (name == NULL) goto loser;


	    /*
	     * Should we make NS_TRUSTED_CA? Probably not we really
	     * didn't look at this cert, and it could be a rougue
	     * card.. Also, should we set emailFlags? rjr
	     */
	    trust.sslFlags = (CERTDB_VALID_CA | CERTDB_TRUSTED_CA );
	   	   /* CERTDB_NS_TRUSTED_CA ;*/
	    trust.emailFlags = 0;
	    trust.paymentFlags = 0;

	    rv = CERT_AddTempCertToPerm(rootCert,name,&trust);
	    PORT_Free(name);
	    if (rv < 0) goto loser;
	}

	FORTEZZALOCK(psock->certLock,psock->certLockInit);
	certListFree = psock->certificateList;
	personListFree = psock->personalityList;
	rootCertFree = psock->rootCert;

	psock->certificateList = certificateList;
	psock->personalityList = personalityList;
	psock->rootCert = rootCert;
	psock->certificateCount = status.CertificateCount;
	FORTEZZAUNLOCK(psock->certLock);

	if (rootCertFree) CERT_DestroyCertificate(rootCertFree);
	if (certListFree) PORT_Free(certListFree);
	if (personListFree) PORT_Free(personListFree);


	return SECSuccess;

loser:
	if (rootCert) CERT_DestroyCertificate(rootCert);
	if (personalityList) PORT_Free(personalityList);
	if (certificateList) PORT_Free(certificateList);
	CI_UNLOCK(psock);
	return SECFailure;
}

/*
 * initialize the pcmcia cards
 * 
 * This routine is called to initialize a fortezza structure. 
 *
 */
static void
FortezzaSocketInit(FortezzaPCMCIASocket *psock) {
	int 			error,i;
	CI_STATUS 		status;
	CI_PERSON		*personalityList;
	SECItem			*certList;
	CERTCertificate		*rootCert;
	FortezzaKeyRegister	*registers;
	HSESSION hs=psock->maci_session;

	if (psock->state != NoCard) {
		return;
	}

	/* first we open the socket */
	CI_LOCKA(psock); /* just do the local locking... */
	error = MACI_Open(hs,0,psock->socketId);

#ifdef XP_WIN
	/* for some reason Window's likes to shutdown the library on us
	 * if this happens we need to reopen the library.. */
	if (error == CI_LIB_NOT_INIT) {
	    int count;
	    error = MACI_Initialize(&count);
	    if (error == CI_OK) {
		error = MACI_Open(hs,0,psock->socketId);
	    }
	}
#endif
	   
	if (error != CI_OK) { goto loser; }


	/* then we get the current card state */
	error = MACI_GetStatus(hs,&status);
	if (error != CI_OK) { goto loser; }

	CI_UNLOCKA(psock);


	FORTEZZALOCK(psock->stateLock,psock->stateLockInit);

	/* check the state, it may have changed since we were called
	 * (someone may have called init between the time we checked
	 * and now).
	 */
	if (psock->state != NoCard) {
	    /* someone has come in and initialized the card for us already. */
	    FORTEZZAUNLOCK(psock->stateLock);
	    return;
	}

	/* update the serial number */
	PORT_Memcpy(psock->serial,status.SerialNumber,sizeof(psock->serial));
	psock->hitCount = 0;
	psock->load = 0;


	/* set the state to reflect our init values */
	PORT_Memcpy(psock->certFlags,status.CertificateFlags,
					sizeof(status.CertificateFlags));

	switch (status.CurrentState) {
	case CI_USER_INITIALIZED:
		psock->state = NeedPin;
	        FORTEZZAUNLOCK(psock->stateLock);
	        FORTEZZALOCK(psock->certLock,psock->certLockInit);
		personalityList = psock->personalityList;
		certList = psock->certificateList;
		rootCert= psock->rootCert;
		psock->certificateCount = 0;
		psock->certificateList = NULL;
		psock->personalityList = NULL;
		psock->rootCert = NULL;
	        FORTEZZAUNLOCK(psock->certLock);
		if (rootCert) CERT_DestroyCertificate(rootCert);
		if (personalityList) PORT_Free(personalityList);
		if (certList) PORT_Free(certList);
		break;
	case CI_STANDBY:
	case CI_READY:
		psock->state = NeedCert;
	        FORTEZZAUNLOCK(psock->stateLock);
		if (FortezzaCertInit(psock) != SECSuccess) {
			/* recover */
	        	FORTEZZALOCK(psock->certLock,psock->certLockInit);
			personalityList = psock->personalityList;
			certList = psock->certificateList;
			rootCert = psock->rootCert;
			psock->certificateCount = 0;
			psock->certificateList = NULL;
			psock->personalityList = NULL;
			psock->rootCert = NULL;
	        	FORTEZZAUNLOCK(psock->certLock);
			if (rootCert) CERT_DestroyCertificate(rootCert);
			if (personalityList) PORT_Free(personalityList);
			if (certList) PORT_Free(certList);
		}
		break;
	default:
	        FORTEZZAUNLOCK(psock->stateLock);
		if (fortezzaGlobal.fortezzaAlert != NULL) {
		    fortezzaGlobal.fortezzaAlert(
			fortezzaGlobal.fortezzaAlertArg,
			PR_TRUE,XP_GetString(XP_SEC_FORTEZZA_BAD_CARD),
			psock->socketId,NULL);
		}
		psock->state = NoCard; /* redundant */
		return;
	}

	FORTEZZAPLOCK(psock->keyLock);
	psock->registerCount = status.KeyRegisterCount;
	registers = psock->registers;
	for (i=0;i < status.KeyRegisterCount;i++) {
	    registers[i] = NULL;
	}
	FORTEZZAPUNLOCK(psock->keyLock);

	return;


loser:
	psock->state = NoCard;
	CI_UNLOCKA(psock);
	return;
}


/*
 * Create shared memory creates a memory region that can be addressed
 * any this process and any of it's decendants. This memory region
 * is used to manage the keys of the fortezza card. Other process access to
 * these keys are managed by MACI. If the application of OS does not create
 * additional processes, normal malloc may be used. (Thread or OK).
 */
static unsigned char *FortezzaSharedMemory(int len, unsigned char **addr){
#if defined(SERVER_BUILD) && defined(XP_UNIX)
	/* we use mmap instead of sys V shared memory so the system
	 * can clean up after us (sys v shared memory stays around *FOREVER*
	 * mmap'ed unlinked files disappear as soon as all references leave
	 * them.
	 * system V shared memory, we should use mmap.
	 * NOTE: Most systems can't extend their shared memory segments
	 * on the fly (sigh), so we preallocate a *LARGE* shareed memory
	 * segment. We don't actually touch that memory in case the system
	 * uses late swap allocation.. we won't actually be using 
	 * any resources associated with that memory. Note also that this
	 * backing store is saved in the specified TMP filesystem */
	int fd;
	char *tmp;
	char *tmpdir=getenv("TMPDIR");
#define TMP_NAME   "fortezza_keys"

	if (tmpdir == NULL) {
	   tmpdir = "/tmp";
	}

	tmp = PORT_Alloc(PORT_Strlen(tmpdir)+PORT_Strlen(TMP_NAME)+30);
	sprintf(tmp,"%s/%s%d",tmpdir,TMP_NAME,getpid());

	fd = open(tmp,O_CREAT|O_RDWR|O_EXCL,0600);
	if (fd < 0) {
	    PORT_Free(tmp);
	    return NULL;
	}
	/* Only we need it.. make it go way when we're through */
	unlink(tmp);
	PORT_Free(tmp);
	/* make the file big enough */
	lseek(fd,FORTEZZA_MAX_SHARED_MEM-1, SEEK_SET);
	write(fd,"\0",1);

	*addr = mmap(NULL,FORTEZZA_MAX_SHARED_MEM,
					PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if (*addr == NULL) {
		return NULL;
	}
	*(unsigned long *)(*addr) = len + sizeof(unsigned long);
	return (*addr+sizeof(unsigned long));
	
#else 
	return PORT_Alloc(len);
#endif
}


/*
 * initialize the global fields
 */
void
FortezzaGlobalInit(void) {
    if (fortezzaGlobal.socketCount == 0) {
	int socketCount,i;
	int error;
	unsigned char * mem_addr;

	fortezzaGlobal.state = GlobalOK;
	fortezzaGlobal.sslList = NULL; /* paranoia */

	error = MACI_Initialize(&socketCount);
	if (error != CI_OK) {
		return;
	}

	fortezzaGlobal.timeout = LL_ZERO;/* by default, set no timeout value */

	fortezzaGlobal.sockets = (FortezzaPCMCIASocket *)
			PORT_ZAlloc(sizeof(FortezzaPCMCIASocket)*socketCount);

	if (fortezzaGlobal.sockets == NULL) {
	    return;
	}

	fortezzaGlobal.socketCount = socketCount;
	fortezzaGlobal.defaultSocket = NULL; /* used by clients */
	/* 
	 * CI_Lock currently does not really lock the library,
	 * but the current library is single threaded, so we need
	 * to protect the library calls with a lock. That's what
	 * this lock is for...
	 */
	FORTEZZAINITLOCK(fortezzaGlobal.libLock,
						fortezzaGlobal.libLockInit);
	FORTEZZAINITLOCK(fortezzaGlobal.paramLock,
					        fortezzaGlobal.paramLockInit);

	fortezzaGlobal.fortezzaMarkSocket = NULL;
	fortezzaGlobal.fortezzaAlert = NULL;
	fortezzaGlobal.fortezzaAlertArg = NULL;
	fortezzaGlobal.mask = 0xffffffff;

	/*
	 * Create shared memory for key allocation
	 * XXX The following really should be an NSPR call!!
	 */
	mem_addr= FortezzaSharedMemory( fortezzaGlobal.socketCount*
			((FORTEZZA_MAX_REG*sizeof(FortezzaKey *))
				+sizeof(FortezzaPlock))+
				 sizeof(FortezzaPlock)+sizeof(unsigned long) +
			FORTEZZA_INIT_KEYS*sizeof(FortezzaKey),
						&fortezzaGlobal.memaddr);
	if (mem_addr == NULL) {
	    fortezzaGlobal.socketCount = 0;
	    PORT_Free(fortezzaGlobal.sockets);
	    return;
	}

	/*
	 * initalize the global shared memory fields
	 */
	/* Key id is to handle some races in sslnonce. It makes sure the
	 * key we have is the key we started with. */
	fortezzaGlobal.keyid = (unsigned long *)mem_addr;
	mem_addr += sizeof(unsigned long);
	*(fortezzaGlobal.keyid) = 1;
	/* keyList is the lock to avoid contention on the list of free keys */
	fortezzaGlobal.keyListLock = (FortezzaPlock *)mem_addr;
	mem_addr += sizeof(FortezzaPlock);
	FORTEZZAINITPLOCK(fortezzaGlobal.keyListLock);

	for (i=0; i < socketCount; i++) {
	    FortezzaPCMCIASocket *psock=&fortezzaGlobal.sockets[i];

	    psock->socketId = i+1;
	    /*
	     * look at all these "baby"-locks. The rule:
	     *  1) you can't grab CI_LOCK if you already hold
	     *    a baby-lock (you can grab a baby-lock if you
	     *    already hold CI_LOCK).
	     *  2) you can't grab more than one baby-lock at a
	     *   time.
	     *  3) Don't make CI_Calls, UI-calls, or other
	     *    potentially long calls with holding a baby-lock;
	     * See the other comments in this file for why we
	     * bother with these finer grain locks.
	     */
	    /*FORTEZZAINITLOCK(psock->keyLock,psock->keyLockInit);*/
	    /* the key data structures are shared by all processes.. so we
	     * have to lock them among all processes */
	    psock->keyLock = (FortezzaPlock *)mem_addr;
	    mem_addr += sizeof(FortezzaPlock);
	    FORTEZZAINITPLOCK(psock->keyLock);
	    FORTEZZAINITLOCK(psock->listLock,psock->listLockInit);
	    FORTEZZAINITLOCK(psock->stateLock,psock->stateLockInit);
	    FORTEZZAINITLOCK(psock->certLock,psock->certLockInit);
	    FORTEZZAINITLOCK(psock->libLock,psock->libLockInit);

	    error = MACI_GetSessionID(&psock->maci_session);
    
	    psock->state = NoCard;
	    /*registers allocation is shared globally among all the processes*/
	    psock->registers = (FortezzaKey **)mem_addr;
	    mem_addr += FORTEZZA_MAX_REG*sizeof(FortezzaKey *);
	    psock->head = NULL;
	    psock->certificateList = NULL;
	    psock->personalityList = NULL;
	    psock->rootCert = NULL;
	    psock->certificateCount = 0;
    
	    psock->template.socket = psock;
	    /* paranoia to help verify stuff */
	    psock->template.socketId = psock->socketId;
	    psock->template.certificate = -1;
	    FortezzaSocketInit(psock);
	}
    
	/*
	 * OK now build the list of keys.
	 */
	fortezzaGlobal.keyTop = NULL;
	for (i=0; i < FORTEZZA_INIT_KEYS; i++) {
	    FortezzaKey *next = (FortezzaKey *)mem_addr;
	    mem_addr += sizeof(FortezzaKey);

	    next->next = fortezzaGlobal.keyTop;
	    fortezzaGlobal.keyTop = next;
	}
    }
    return;
}

/* log out of all the fortezza cards */
void
FortezzaLogout() {
    int i;
    for (i=0; i < fortezzaGlobal.socketCount; i++) {
	FortezzaPCMCIASocket *psock=&fortezzaGlobal.sockets[i];
	/* reset all the cards */
	FortezzaError(CI_OK,NULL,psock); 
    }
}

/*
 * initialize the pcmcia cards
 * 
 * This routine is called to initialize a fortezza structure. 
 *
 */
static PRBool
FortezzaSocketVerify(FortezzaPCMCIASocket *psock) {
	int error;
	CI_STATUS status;
	HSESSION hs = psock->maci_session;
	PRBool ret;

	/* 
	 * CI_LOCK locks the whole CI_Library. Since is is such a
	 * coarse grain lock, we use smaller grain locks on some of
	 * the key data structures. To prevent dead locks, you cannot
	 * call CI_LOCK() while you are already holding a finer grain
	 * lock.
	 */
	PORT_Memset(&status,0,sizeof(CI_STATUS));
	CI_LOCK(psock);
	/*  What we are trying to detect if the card
	 * has popped in or out. If it has, select should fail, and we
	 * go off and clear off any existing context's then re-init the
	 * socket. However it looks like select succedes even if there 
	 * is no card, so we issue a CI_GetStatus call. This better fail if
	 * there is no card.
	 */
	error = MACI_GetStatus(hs,&status);
	if ((error != CI_OK) || (status.CurrentSocket != psock->socketId) ||
	 (PORT_Memcmp(psock->serial,
			status.SerialNumber,sizeof(psock->serial))!=0)) {
	    /* If this is the default card, then it has changed */
		if (fortezzaGlobal.defaultSocket == psock) {
			fortezzaGlobal.defaultSocket = NULL;
		}
		FortezzaError(CI_OK,NULL,psock); /* don't throw an alert */
		CI_UNLOCK(psock);
		FortezzaSocketInit(psock);
		return (psock->state != NoCard);
	}
	CI_UNLOCK(psock);

	FORTEZZALOCK(psock->stateLock,psock->stateLockInit);
	/* if we're not already in ready state, someone
	 * is dealing with getting us there...
	 */
	if ((psock->state != Ready) && (psock->state != NeedCert)) {
		FORTEZZAUNLOCK(psock->stateLock);
		return PR_TRUE;
	}
	ret = PR_TRUE;
	switch (status.CurrentState) {
	case CI_USER_INITIALIZED:
		/* some one pulled the card, we need a new pin */
		psock->state = NeedPin;
		break;
	case CI_STANDBY:
	case CI_READY:
		/* The card is there, we don't set the ready state because
		 * we may be in NeedCert state.. Since the card is initialized,
		 * psock->state must already be correct. */
		/*psock->state = Ready; */
		break;
	default:
		if (fortezzaGlobal.fortezzaAlert != NULL) {
		    fortezzaGlobal.fortezzaAlert(
			fortezzaGlobal.fortezzaAlertArg,
			PR_TRUE,XP_GetString(XP_SEC_FORTEZZA_BAD_CARD),
			psock->socketId,NULL);
		}
		psock->state = NoCard; /* redundant */
		ret = PR_FALSE;
		break;
	}
	if (ret && !LL_IS_ZERO(fortezzaGlobal.timeout) && 
						(psock->state != NeedPin)) {
	    /* time out fortezza cards */
	    int64 timeout;
	    int64 now = PR_NowS();
	    LL_ADD(timeout,psock->lasttime,fortezzaGlobal.timeout);
	    if (LL_CMP(now, > , timeout)) {
		psock->state = NeedPin;
	    }
	}
 	FORTEZZAUNLOCK(psock->stateLock);

	return ret;
}

/*
 * get a list of sockets and cards on the system
 */
SECStatus
FortezzaLookupCard(FortezzaCardInfo **info, uint32 *mask, int *count,
    int *found,FortezzaPCMCIASocket **min, FortezzaPCMCIASocket **last,
							 PRBool isServer) {
	int i;
	FortezzaPCMCIASocket *last_sock = NULL;
	FortezzaPCMCIASocket *min_sock = NULL;
	uint32 minload = 0xffffffff /* MAXUINT */;

	*mask = 0;
	*found = 0;

	PORT_Assert(fortezzaGlobal.socketCount <= 32);

	if (info) {
	    *info = PORT_Alloc(sizeof(FortezzaCardInfo)
					*fortezzaGlobal.socketCount);
	    if (*info == NULL) return SECFailure;
	}

	if (count != NULL) {
	    *count = fortezzaGlobal.socketCount;
	}

	for (i=0; i < fortezzaGlobal.socketCount; i++) {
	    FortezzaPCMCIASocket *psock = &fortezzaGlobal.sockets[i];

	    /* only tweak the cards in the mask */
	    if ((fortezzaGlobal.mask & (1 << i)) == 0) {
		if (psock->state != NoCard) {
		    /* close the current socket down */
		    FortezzaError(CI_OK,NULL,psock); 
		}
		continue;
	    }

	    /* look for and existing card */
	    if (psock->state != NoCard)   {
		/* don't verify on servers, let the connection fail if
		 * the fortezza card was pulled out! */
		if (isServer || FortezzaSocketVerify(psock)) {
		    last_sock = psock;
		    (*mask) |= ((uint32)1 << i);
		    (*found)++;
		    if ((unsigned)psock->load < minload) {
			minload = psock->load;
			min_sock = psock;
		    }
		} else {
		    /* card has been moved */
		    if (fortezzaGlobal.defaultSocket == psock) {
			fortezzaGlobal.defaultSocket = NULL;
		    }
		}
	    } else {
		/* try to init one */
		FortezzaSocketInit(psock);
		if (psock->state != NoCard) {
		    last_sock = psock;
		    (*mask) |= (1 << i);
		    (*found)++;
		    if ((unsigned)psock->load < minload) {
			minload = psock->load;
			min_sock = psock;
		    }
		}
	    }

	    /* add vendor? */
	    if (info) {
	    	PORT_Memcpy((*info)[i].serial,psock->serial,
							sizeof(psock->serial));
	    	(*info)[i].state = psock->state;
	    }
	}

	if (last) *last = last_sock;
	if (min) *min = min_sock;

	return SECSuccess;
}

/*
 * this routine finds the first available fortezza card.
 * In the future this routine could get a lot smarter, and look for
 * user selected cards, and/or spread out load over multiple cards on
 * a server.
 */
static SECStatus
FortezzaFindCard(SSLSocket *ss, FortezzaPCMCIASocket **ppsock) {
	SSLSecurityInfo *sec = NULL;
	int rv;
	uint32 cards = 0;
	int count=0;
	FortezzaPCMCIASocket *last_sock = NULL;
	FortezzaPCMCIASocket *min_sock = NULL;
	FortezzaCardInfo *info = NULL;
	PRBool isServer = PR_TRUE;

        if (ss) {
	    sec=ss->sec;
	    if (sec->fortezzaCardSelect != NULL) isServer=PR_FALSE;
	}

	/*
	 * we're waiting for a popup to complete
	 */
        if (fortezzaGlobal.state == GlobalOpen) {
	    if (fortezzaGlobal.fortezzaMarkSocket != NULL) {
		fortezzaGlobal.fortezzaMarkSocket(ss);
	    }
	    return SECWouldBlock;
	}

	/*
	 * the popup was cancelled, continue processing without the
	 * fortezza card.
	 */
	if (fortezzaGlobal.state == GlobalSkip) {
	    return SECFailure;
	}

	/* hack -- not thread safe... */
	if (sec) fortezzaGlobal.fortezzaAlertArg = sec->fortezzaAlertArg;
	rv = FortezzaLookupCard(isServer? NULL : &info,&cards,NULL,
					&count,&min_sock,&last_sock,isServer);
	if (rv < 0) return SECFailure;


	switch (count) {
	case 0:
		*ppsock = NULL;
		if (info) PORT_Free(info);
		return SECFailure; /* none found */
	case 1:
		/* only one card available, no need to ask the user
		 * anything */
		*ppsock = last_sock;
		fortezzaGlobal.defaultSocket = last_sock;
		if (info) PORT_Free(info);
		return SECSuccess; 
	}
	/* default fall-through */
	
	
	if (sec && sec->fortezzaCardSelect) {
	    int psockID;
	    SECStatus rv;

	    /* if we have already selected a socket, and we haven't
	     * added new sockets or removed the selected socket ..
	     * use the selected socket */
	    if (fortezzaGlobal.defaultSocket) {
		*ppsock = fortezzaGlobal.defaultSocket;
		if (info) PORT_Free(info);
		return SECSuccess;
	    }
	    rv=sec->fortezzaCardSelect
		(sec->fortezzaCardArg,ss->fd,info,cards,&psockID);
	    if (info) PORT_Free(info);
	    if (rv == SECSuccess) {
		PORT_Assert((psockID > 0)
				&&(psockID <= fortezzaGlobal.socketCount));
		*ppsock = &fortezzaGlobal.sockets[psockID-1];
		fortezzaGlobal.defaultSocket = *ppsock;
	    }
	    return rv;
	}
	if (info) PORT_Free(info);

	/* for the server we grab the least busy card */
	*ppsock = min_sock;
	fortezzaGlobal.defaultSocket = min_sock;
	return SECSuccess;
}

/*
 * this routine finds the first available fortezza card.
 * In the future this routine will get a lot smarter, and look for
 * user selected cards, and/or spread out load over multiple cards on
 * a server.
 */
static SECStatus
FortezzaFindOpenCard(FortezzaPCMCIASocket **ppsock) {
	int i;
	unsigned long cards;
	FortezzaPCMCIASocket *last_sock = NULL;
	FortezzaPCMCIASocket *min_sock = NULL;
	unsigned long minload = 0xffffffff /* MAXUINT */;
	int count=0;


	PORT_Assert(fortezzaGlobal.socketCount <= 32);
	for (i=0; i < fortezzaGlobal.socketCount; i++) {
	    FortezzaPCMCIASocket *psock = &fortezzaGlobal.sockets[i];

	    /* look for and existing card */
	    /* Any card that's been initialized with a pin will work */
	    if ((psock->state == Ready) || (psock->state == NeedCert)) {
		    last_sock = psock;
		    cards |= (1 << i);
			count++;
		    if ((unsigned)psock->load < minload) {
			minload = psock->load;
			min_sock = psock;
		    }
	    } 
	}

	switch (count) {
	case 0:
		*ppsock = NULL;
		return SECFailure; /* none found */
	case 1:
		/* only one card available, no need to ask the user
		 * anything */
		*ppsock = last_sock;
		return SECSuccess; 
	}
	/* default fall-through */

	/* for the server we grab the least busy card */
	*ppsock = min_sock;
	return SECSuccess;
}

/*
 * the goal with FortezzaOpenCard is to get through the an already opened
 * adapter without having to do a CI_LOCK or a CI_ call. This is most
 * important in the server, which will never call CI_LOCK in open (all
 * the fortezza control on the server happen "off-line".
 */
static SECStatus
FortezzaOpenCard(SSLSocket *ss,FortezzaKEAContext **cx,
			FortezzaPCMCIASocket *psock,PRBool needCert) {
	SSLSecurityInfo *sec = NULL;

	if (ss) sec = ss->sec;

	FORTEZZALOCK(psock->stateLock,psock->stateLockInit);

	if (psock->state == NoCard) {
	    FORTEZZAUNLOCK(psock->stateLock);
	    return SECFailure;
	}

	/* see if we can get out of here without issuing a 
	 * CI command! */

	psock->lasttime = PR_NowS();

	if (psock->state == Ready) {
	    FORTEZZAUNLOCK(psock->stateLock);
	    if (cx) *cx = FortezzaCreateKEAContext(psock);
	    return SECSuccess;
	}

	/*
	 * Nope, we are going to need to get the pin.
	 * So now we tell everyone else interested in openning this socket
	 * go go away
	 */


	if (sec && sec->fortezzaGetPin) {
	    SECStatus rv;

	    FORTEZZAUNLOCK(psock->stateLock); 
	    /* does *NOT* return SECWouldBlock! */
	    if (psock->state == NeedPin) {
	    	rv = (sec->fortezzaGetPin)(sec->fortezzaPinArg,psock);
	    } else {
		rv = SECSuccess;
	    }
	    if (rv == SECSuccess) {
		/* assumes assignment to int is atomic ! */
		psock->state = NeedCert;
		*cx = FortezzaCreateKEAContext(psock);
		FORTEZZAWAKEUP(psock->stateLock);
		rv = FortezzaCertInit(psock);
		if (rv < 0) return rv;

		if (needCert) {			
			rv = FortezzaSelectCertificate(ss,psock,
					&psock->template.certificate);
			(*cx)->certificate = psock->template.certificate;
			if (rv == SECSuccess) psock->state = Ready;
		}
		return rv;

	    }
	    psock->state = NoCard;
	    FORTEZZAWAKEUP(psock->stateLock);
	    return rv;
	}
	psock->state = NoCard; /* don't keep putting up dialogs right away */
	/* no need to wakeup here, no one could have been put to sleep on
	 * this path */
	FORTEZZAUNLOCK(psock->stateLock); 

	return SECFailure;
}


/*
 * the goal with FortezzaOpenCard is to get through the an already opened
 * adapter without having to do a CI_LOCK or a CI_ call. This is most
 * important in the server, which will never call CI_LOCK in open (all
 * the fortezza control on the server happen "off-line".
 */
SECStatus
FortezzaOpen(SSLSocket *ss,FortezzaKEAContext **cx) {
   FortezzaPCMCIASocket *psock;
   SECStatus rv;

   rv = FortezzaFindCard(ss,&psock);
   if (rv == SECSuccess) {
	rv=FortezzaOpenCard(ss,cx,psock,PR_TRUE);
   }
   return (rv);
}


/*
 * Servers are configured once on startup. Dynamic changing of cards
 * does not happen in the server case! If a card goes off line, it's offline
 * for the rest to the life of the server.
 */
SECStatus
FortezzaConfigureServer(FortezzaPinFunc getPin,uint32 mask, char *cert_name,
				FortezzaAlert func, void *arg, int *error) {
    char *password;
    FortezzaPCMCIASocket *psock;
    SECStatus rv = SECFailure;
    FortezzaKEAContext *cx = NULL;
    SECKEYPrivateKey *key;
    uint32 cards = 0;
    int i,count=0;
    int first = 0;
    *error = FORTEZZA_BADCARD;

    fortezzaGlobal.mask = mask;
    fortezzaGlobal.fortezzaAlert = func;
    fortezzaGlobal.fortezzaAlertArg = arg;

    rv = FortezzaLookupCard(NULL,&cards,NULL,&count,NULL,NULL,PR_TRUE);
    password = NULL;
    fortezzaGlobal.mask = cards;
    if (rv == SECSuccess) {
	/* initialize all the cards the server is going to use */
	for (i=0; i < fortezzaGlobal.socketCount; i++) {
	    /* skip cards slots that have been reserved */
	    if ((cards & (1<<i)) == 0) {
		continue;
	    }

	    psock = &fortezzaGlobal.sockets[i];

	    /* pin initialize all the cards */
	    if (password == NULL) {
		/* prompt for the pin of the first card */
		do {
	    	    if (password) PORT_ZFree(password, PORT_Strlen(password));
	    	    if ((*getPin)(&password) == NULL) {
			*error = FORTEZZA_BADPASSWD;
			return SECFailure;
	    	    }
	    	    if ((password == NULL) || (*password == '\0')) {
			rv = SECFailure;
			break;
	   	    }
	    	    rv = FortezzaPinCheck(psock,password);
		} while (rv == SECWouldBlock);
		if (rv < 0) {
	    	    *error = FORTEZZA_BADPASSWD;
	    	    if (password) PORT_ZFree(password, PORT_Strlen(password));
	    	    return rv;
		}
	    } else {
		/* now do a pin check on the subsequent cards. If they
		 * fail, just disable them */
	    	rv = FortezzaPinCheck(psock,password);
		if (rv < 0) {
		     fortezzaGlobal.mask &= ~(1<<i);
		     FortezzaError(CI_OK,NULL,psock); 
		     func(arg,PR_TRUE,
			"Card in socket %d failed pin check.. disabling\n",
								i+1,NULL);
		     continue;
		}
	    }
	    psock->state = Ready; /* assumes single threaded at init time */
	    rv=FortezzaOpenCard(NULL,&cx,psock,PR_TRUE);
	    if (rv < 0) {
		fortezzaGlobal.mask &= ~(1<<i);
		FortezzaError(CI_OK,NULL,psock); 
		continue;
	    }
		
	    rv = FortezzaCertInit(psock);
	    if (rv < 0) {
		fortezzaGlobal.mask &= ~(1<<i);
		FortezzaError(CI_OK,NULL,psock); 
		FortezzaDestroyKEAContext(cx,PR_TRUE);
		continue;
	    }

	    /* use the first one to initialize the fortezza keys structure */
	    if (first == 0) {
		CERTCertificate *certificate = NULL;

		/* now look up the certificate by name */
		if (cert_name) {
		    psock->template.certificate = 
			    FortezzaFindCertIndexByNickname(psock,cert_name);
		} else psock->template.certificate = -1;
		if (psock->template.certificate < 0) {
	    	    rv = FortezzaSelectCertificate(NULL,psock,
					&psock->template.certificate);
		}
		if (rv < 0) {
		    fortezzaGlobal.mask &= ~(1<<i);
		    FortezzaError(CI_OK,NULL,psock); 
		    FortezzaDestroyKEAContext(cx,PR_TRUE);
		    continue;
		}
	    	cx->certificate = psock->template.certificate;
	    	rv=FortezzaGetCertList(cx,
			&ssl3_fortezza_server_cert_chain,&key,&certificate);
	    	if (rv < 0) {
		    fortezzaGlobal.mask &= ~(1<<i);
		    FortezzaError(CI_OK,NULL,psock); 
		    FortezzaDestroyKEAContext(cx,PR_TRUE);
		    continue;
		}
		CERT_DestroyCertificate(certificate);
		first++;
            	SSL3_SetFortezzaKeys(key);
		fortezzaGlobal.serverCertificate=psock->template.certificate;
	    } else {
		psock->template.certificate = fortezzaGlobal.serverCertificate;
	    }
   	    FortezzaDestroyKEAContext(cx,PR_TRUE);
	}
    }

    if (password) PORT_ZFree(password, PORT_Strlen(password));

    rv = SECFailure;
    if (first) {
       *error = FORTEZZA_OK;
       rv = SECSuccess;
    }

    return (rv);
}	

/*
 * turn the 4 byte BCD index in the personality label to an
 * integer we can use to look up the parent
 */
static int
FortezzaDecodeIndex(unsigned char *label) {
	int i,c;
	int value = 0;

	label +=4;

	for (i=0; i < 4; i++) {
		c = label[i];
		if (c < '0' || c > '9') return -1;
		value = value*10 + label[i] - '0';
	}

	return value;
}

/*
 * return configuration and status info
 */
SECStatus
FortezzaGetConfig(FortezzaPCMCIASocket *psock,CI_CONFIG *config,
							 CI_STATUS *status) {
    int error;
    HSESSION hs;

    if (psock == NULL || (psock->state == NoCard)) goto loser;

    hs = psock->maci_session;

    CI_LOCK(psock);

    error = MACI_GetConfiguration(hs,config);
    if (error != CI_OK) {
	goto loser;
    	CI_UNLOCK(psock);
    }

    error = MACI_GetStatus(hs,status);
    if (error != CI_OK) {
    	PORT_Memset(status, 0, sizeof(CI_STATUS));
    }
    CI_UNLOCK(psock);
    return SECSuccess;

loser:
    PORT_Memset(config, 0, sizeof(CI_CONFIG));
    PORT_Memset(status, 0, sizeof(CI_STATUS));
    return SECFailure;
}

/*
 * do the actual lookup... used below.
 */
static CERTCertificate *
FortezzaCertLookUp(FortezzaPCMCIASocket *psock, int certID, int *nextCert) {
    int i;
    char nickname[25];


    FORTEZZALOCK(psock->certLock,psock->certLockInit);
    if (certID > psock->certificateCount) {
	goto loser;
    }
    if (psock->certificateList == NULL) {
	goto loser;
    }
    if (psock->certificateList[certID].data == NULL) {
	goto loser;
    }

    /*
     * Look up 1) our parent certificate and 2) our nickname.
     * We get this from the personalityList record. The personality
     * List record has a CertificateIndex and a Label. The certificate
     * index tells which certificate this belongs to, and the Label
     * is a string with the following format:
     *  Bytes 0-3	Certificate type
     *  Bytes 4-7	BCD encoded parent certificate index
     *  Bytes 8-31	Name of the certificate (nickname)
     */
    if (certID != 0) {
      for (i=0; i < psock->certificateCount; i++) {
	/* here's the personality that matches our cert */
	if (psock->personalityList[i].CertificateIndex == certID) {
	    /* copy the name field	*/
	    PORT_Memcpy(nickname,&psock->personalityList[i].CertLabel[8],24);
	    nickname[24] = 0;
	    /* grab the parent cert */
	    *nextCert = 
		    FortezzaDecodeIndex(psock->personalityList[i].CertLabel);
	    break;
	}
      }
    } else {
	i = 0;
        *nextCert  = -1;
	nickname[0] = 0;
    } 
    if (i == psock->certificateCount) {
    	FORTEZZAUNLOCK(psock->certLock);
	/*should do something more drastic, like invalidate the card!*/
	return NULL;
    }

    FORTEZZAUNLOCK(psock->certLock);

    /* look up the next cert and add it to the chain (special
     * case the first try) */
    return CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
	&psock->certificateList[certID],nickname, PR_FALSE, PR_TRUE);
loser:
    FORTEZZAUNLOCK(psock->certLock);
    return NULL;
}
		    

/*
 * grab a certificate from the "card" based on socketID, serial number, and
 * certificate index. Future enhancements may be to search all to sockets
 * for the serial number? It wouldn't be to much code, but will it ever be
 * useful? Fortezza Certificates really come in chains. You pass the whole
 * chain around (except the root, which everyone has..).
 */
CERTCertificate *
FortezzaGetCertificate(int sockID,int certID,unsigned char *serial) {
	FortezzaPCMCIASocket *psock;
	unsigned char nickname[25];
	int nextCert;
	CERTCertificate *current = NULL, *certificate = NULL;
	SSL3CertNode *certs = NULL, *cnode;
	PRArenaPool *arena = NULL;

	if ((sockID <= 0) || (sockID > fortezzaGlobal.socketCount)) {
	    return NULL;
	}


	psock = &fortezzaGlobal.sockets[sockID-1];
	nickname[0] = 0;

	FORTEZZALOCK(psock->stateLock,psock->stateLockInit);
	if (PORT_Memcmp(psock->serial,serial,sizeof(psock->serial)) != 0) {
	    FORTEZZAUNLOCK(psock->stateLock);
	    return NULL; /* Well maybe we should go looking for it? */
	}

	FORTEZZAUNLOCK(psock->stateLock);

	/* ok we have a valid card, and we have a valid serial, lets
	 * go look up the certificate...
	 */
	certificate = FortezzaCertLookUp(psock,certID,&nextCert);
	current = certificate;

	if (certificate == NULL) return NULL;
	
	/*
	 * a fortezza certificate by itself is not meaningful
	 * we need a chain of certificates. NOTE: multiple users certificates
	 * can all point to the same LA or CA certificates. We send all but
	 * the root certificate, since all fortezza cards must have the same
	 * root certificate in their card.
	 */
	for (certID = nextCert; certID != -1 && 
          certificate->subjectPublicKeyInfo.algorithm.parameters.len == 0;
							 certID = nextCert) {

	    if (arena == NULL) {
		arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
		if (arena == NULL) goto loser;
	    }
	    /* unashamedly plagerized from ssl3con.c
	     * we keep track of the certs in a link list because we need to
	     * walk the array backwards to fill in the PQG parameters at the
	     * end... */
	    cnode = PORT_ArenaAlloc(arena, sizeof(SSL3CertNode));
	    if (cnode == NULL) goto loser;

	    cnode->cert = FortezzaCertLookUp(psock,certID,&nextCert);
	    if (cnode->cert == NULL) goto loser;

	    cnode->next = certs;
	    certs = cnode;
	}

	/* make each cert independent from each other */
	if (certs) {
	    FortezzaPQGUpdate(certs,certificate);
	    /* destroy the other certs */

	    for (cnode = certs; cnode != NULL; cnode = cnode->next) {
		CERT_DestroyCertificate(cnode->cert);
	    }
	    PORT_FreeArena(arena, PR_FALSE);
	}

	return certificate;

loser:
	if (certificate) { CERT_DestroyCertificate(certificate); }
	if (certs) {
	    for (cnode = certs; cnode != NULL; cnode = cnode->next) {
		CERT_DestroyCertificate(cnode->cert);
	    }
	}
	if (arena) { PORT_FreeArena(arena, PR_FALSE); }
	return NULL;
}

/*
 * Grab A cert by it's nickname
 */
static int FortezzaFindCertIndexByNickname(
			FortezzaPCMCIASocket *psock,char *certname) {
	int len = PORT_Strlen(certname);
	char **personalities;
	int 	*indexes;
	int count,i;
	int found = -1;
	SECStatus rv;

	if (psock == NULL) {
	    return -1;
	}

	rv = FortezzaLookupPersonality(psock,&personalities,&count,&indexes);

	if (rv < 0 ) return -1;

	for (i=0; i < count ; i++) {
	    if (XP_STRNCMP(personalities[i],certname,len) == 0) {
		found = indexes[i];
		break;
	    }
	}
	FortezzaFreePersonality(personalities);
	PORT_Free(indexes);

	return found;

}

/*
 * look up a cert based on the nickname in the personality list field
 */
CERTCertificate * FortezzaFindCertByNickname(char *certname) {
	FortezzaPCMCIASocket *psock = fortezzaGlobal.defaultSocket;
	int found;

	found = FortezzaFindCertIndexByNickname(psock,certname);

	if (found < 0) {
	    return NULL;
	}
	
	return FortezzaGetCertificate(psock->socketId,found,psock->serial);
}



/*
 * build an SECKEYPrivateKey from a fortezza context.
 */
SECKEYPrivateKey *
FortezzaGetPrivateKey(FortezzaKEAContext *cx) {
    PRArenaPool *arena;
    SECKEYPrivateKey *privkey;

    PORT_Assert(cx->socketId == cx->socket->socketId) ;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	return NULL;
    }
    privkey = (SECKEYPrivateKey *)PORT_ArenaZAlloc(arena,sizeof(SECKEYPrivateKey));
    if (privkey == NULL) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }
    privkey->arena = arena;
    privkey->keyType = fortezzaKeys;
    privkey->fortezzaKeyData.certificate = cx->certificate;
    PORT_Memcpy(privkey->fortezzaKeyData.serial,
		cx->socket->serial,sizeof(cx->socket->serial));
    privkey->fortezzaKeyData.socket =  cx->socket->socketId;

    return privkey;
}


/*
 * build a certificate list and a certificat DER list from a
 * context (get the key as well).
 */
SECStatus
FortezzaGetCertList(FortezzaKEAContext *cx,CERTCertificateList **chain,
			SECKEYPrivateKey **key, CERTCertificate **certificate) {
	FortezzaPCMCIASocket *psock = cx->socket;
	unsigned char nickname[25];
	int nextCert;
	int certID = cx->certificate;
	SSL3CertNode *certs = NULL, *cnode;
	PRArenaPool *arena = NULL;

    	PORT_Assert(cx->socketId == cx->socket->socketId) ;

	*key = NULL;
	*chain = NULL;
	*certificate = NULL;

	nickname[0] = 0;


	/* ok we have a valid card, and we have a valid serial, lets
	 * go look up the certificate...
	 */
	*certificate = FortezzaCertLookUp(psock,certID,&nextCert);

	if (*certificate == NULL) return SECFailure;
	
	/*
	 * a fortezza certificate by itself is not meaningful
	 * we need a chain of certificates. 
	 */
	for (certID = nextCert; certID != 0 ; certID = nextCert) {

	    if (arena == NULL) {
		arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
		if (arena == NULL) goto loser;
	    }
	    /* unashamedly plagerized from ssl3con.c
	     * we keep track of the certs in a link list because we need to
	     * walk the array backwards to fill in the PQG parameters at the
	     * end... */
	    cnode = PORT_ArenaAlloc(arena, sizeof(SSL3CertNode));
	    if (cnode == NULL) goto loser;

	    cnode->cert = FortezzaCertLookUp(psock,certID,&nextCert);
	    if (cnode->cert == NULL) goto loser;

	    cnode->next = certs;
	    certs = cnode;
	}


	/* make each cert independent from each other */
	if (certs) {
	    FortezzaPQGUpdate(certs,*certificate);
	}

	*chain = CERT_CertChainFromCert(CERT_GetDefaultCertDB(),*certificate);

	/* destroy the certs now */
	if (certs) {
	    for (cnode = certs; cnode != NULL; cnode = cnode->next) {
		CERT_DestroyCertificate(cnode->cert);
	    }
	    PORT_FreeArena(arena, PR_FALSE);
	}

	*key = FortezzaGetPrivateKey(cx);
	if ((key != NULL) && (*chain != NULL)) {
		return SECSuccess;
	}

	if (*key) { SECKEY_DestroyPrivateKey(*key); }
	if (*chain)  { CERT_DestroyCertificateList(*chain); }
	CERT_DestroyCertificate(*certificate);

	*certificate =  NULL;
	*key = NULL;
	*chain = NULL;

	return SECFailure;

loser:
	if (*certificate) { CERT_DestroyCertificate(*certificate); }
	if (certs) {
	    for (cnode = certs; cnode != NULL; cnode = cnode->next) {
		CERT_DestroyCertificate(cnode->cert);
	    }
	}
	if (arena) { PORT_FreeArena(arena, PR_FALSE); }
	return SECFailure;
}
    

/*
 * Not all Fortezza certificates carry their PQG parameters.
 * Users inherit their parameters from their "parent" certificate.
 * When we get into the bowels of key extraction code, you don't have
 * any information about what the parameters are, which we need to set
 * in our public key structure. If we ignore this step, we won't be able
 * to check parameter mis-matches when verifying signatures.
 */
SECStatus
FortezzaPQGUpdate(SSL3CertNode *certs,CERTCertificate *tipCert) {
	int tag;
	SECOidData *oid;
	SECItem parameters;
	SSL3CertNode *cnode;
	CERTSubjectPublicKeyInfo *spki;

	/* we only need to do this update on fortezza certificates */
	oid = SECOID_FindOID(&tipCert->subjectPublicKeyInfo.algorithm.algorithm);
	if (oid == NULL) return SECSuccess; /* or should we fail? */
	tag = oid->offset;
	if ( (tag != SEC_OID_MISSI_KEA_DSS_OLD) &&
	    (tag != SEC_OID_MISSI_DSS_OLD) && (tag != SEC_OID_MISSI_KEA_DSS)
					&& (tag != SEC_OID_MISSI_DSS) ) {
		/* this is not a fortezza certificate, we're done */
		return SECSuccess;
	}

	/* initialize the paramters from the global root parameters,
	 * This requires that we have loaded at least on fortezza card once
	 * sometime in the past. If we haven't, we shouldn't be gettting
	 * MISSI formated certificates! This also assumes a single
	 * fortezza root for all cards! 
	 *
	 * NOTE: this locking is broken. parameters.data may get freed
	 *  out from under us. It works for now because 1) the client
	 *  is not multi-threaded, and 2) the server never frees
	 *  it's parameters.data structure? */
	parameters.data = NULL;
	parameters.len  = 0;
	FORTEZZAUNLOCK(fortezzaGlobal.paramLock);

	for (cnode = certs; cnode != NULL; cnode = cnode->next) {
	    CERTCertificate *cert = cnode->cert;

	    spki=&cert->subjectPublicKeyInfo;
	    if (spki->algorithm.parameters.len == 0) {
		SECStatus rv;
		if (parameters.len != 0) {
		    rv  = SECITEM_CopyItem(cert->arena, 
			&spki->algorithm.parameters, &parameters);
		    if (rv < 0) return rv;
		}
	    } else {
		parameters.data = spki->algorithm.parameters.data;
		parameters.len = spki->algorithm.parameters.len;
	    }
	}

	spki=&tipCert->subjectPublicKeyInfo;
	if (parameters.len != 0 && spki->algorithm.parameters.len == 0) {
	   SECStatus rv = SECITEM_CopyItem(tipCert->arena, 
			&spki->algorithm.parameters, &parameters);
	    if (rv < 0) return rv;
	} 

	return SECSuccess;
}

/*
 * make sure the PQG parameters for any two certs match
 */
int
FortezzaComparePQG(CERTCertificate *cert1, CERTCertificate *cert2) {
    CERTSubjectPublicKeyInfo *spki1 = &cert1->subjectPublicKeyInfo;
    CERTSubjectPublicKeyInfo *spki2 = &cert1->subjectPublicKeyInfo;

    return SECITEM_CompareItem(&spki1->algorithm.parameters,
						&spki2->algorithm.parameters);
}

static int
FortezzaFindBlankPersonality(FortezzaPCMCIASocket *psock) {
    int i,count = psock->certificateCount;
    int certID = -1;

    for (i=0; i < count; i++) {
	FORTEZZALOCK(psock->certLock,psock->certLockInit);
	/* deal with the race condition */
	if ( i >= psock->certificateCount) {
	    FORTEZZAUNLOCK(psock->certLock);
	    break;
	}

	if (!FortezzaCertBits(psock->certFlags, i)) {
	    certID = i;
	    FortezzaSetCertBits(psock->certFlags, i);
	    FORTEZZAUNLOCK(psock->certLock);
	}
    }
    return certID;
}

/*
 * Generate a new key based on the PQG parameters of the server's certificate
 */
SECStatus
FortezzaGenerateNewPersonality(FortezzaKEAContext *cx, int *certIndex,
				FortezzaPublic y, CERTCertificate *cert) {
    SECKEYPublicKey *key=NULL;
    FortezzaPCMCIASocket *psock = cx->socket; 
    HSESSION hs = psock->maci_session;
    int error;

    PORT_Assert(cx->socketId == cx->socket->socketId) ;
    /*
     * Extracting the key is the easiest way to find the pqg
     * parameters.
     */
    key = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if (key == NULL) {
	return SECFailure;
    }

    /*
     * RestoreState saves the current state as well as restoring our
     * state. For signatures we have no state to restore... we just do
     * it. That's why we pass it a NULL. We call restore state so that
     * we don't trash any running encryption/decryption.
     */	
    CI_LOCK(psock);

    *certIndex = FortezzaFindBlankPersonality(psock);
    if (*certIndex < 0) { goto loser; }

    error = MACI_GenerateX(hs,*certIndex,CI_KEA_TYPE,key->fortezzaKeyData.p.len,
		key->fortezzaKeyData.q.len,key->fortezzaKeyData.p.data,
		key->fortezzaKeyData.q.data,key->fortezzaKeyData.g.data,
					 sizeof(FortezzaPublicKey), y);
    if (error != CI_OK) { goto loser; }
    /* Load a fake certificate??! */
    CI_UNLOCK(psock);

    SECKEY_DestroyPublicKey(key);
    return SECSuccess;

loser:
    CI_UNLOCK(psock);
    SECKEY_DestroyPublicKey(key);
    return SECFailure;
}

/*
 * Delete a generated personality
 */
SECStatus FortezzaDeletePersonality(FortezzaKEAContext *cx,int cert) {
    FortezzaPCMCIASocket *psock = cx->socket; 
    HSESSION hs = psock->maci_session;

    PORT_Assert(cx->socketId == cx->socket->socketId) ;
    CI_LOCK(psock);
    /*FortezzaRestoreState(psock,NULL);*/
    MACI_DeleteCertificate(hs,cert);
    CI_UNLOCK(psock);
    FORTEZZALOCK(psock->certLock,psock->certLockInit);
    FortezzaClrCertBits(psock->certFlags, cert);
    FORTEZZAUNLOCK(psock->certLock);

    return SECSuccess;
}
	   

/*
 * use the card to Sign something
 */
SECStatus
FortezzaSign(unsigned char *digest,unsigned char *signature, SECKEYPrivateKey *key) {
	FortezzaPCMCIASocket *psock; 
        HSESSION hs;
	int error;
	int sockID = key->fortezzaKeyData.socket;

	PORT_Assert( key->keyType == fortezzaKeys  );

	/* paranoia */
	if ((sockID <= 0) || (sockID > fortezzaGlobal.socketCount)) {
	    return SECFailure;
	}

	psock = &fortezzaGlobal.sockets[key->fortezzaKeyData.socket-1];

        hs = psock->maci_session;
	CI_LOCK(psock); 

	if (psock->state == NoCard) {
	    goto loser;
	}

	if (PORT_Memcmp(psock->serial,key->fortezzaKeyData.serial,
						sizeof(psock->serial)) != 0) {
	    goto loser;
	}


	/*
	 * RestoreState saves the current state as well as restoring our
	 * state. For signatures we have no state to restore... we just do
	 * it. That's why we pass it a NULL. We call restore state so that
	 * we don't trash any running encryption/decryption.
	 */	
#ifdef notdef
	error = MACI_Select(hs,psock->socketId);
	if (error != CI_OK) { goto loser; }
#endif

	error = MACI_SetPersonality(hs,key->fortezzaKeyData.certificate);
	if (error != CI_OK) { goto loser; }

	error = MACI_Sign(hs,digest,signature);
	if (error != CI_OK) { goto loser; }

	CI_UNLOCK(psock);
	return SECSuccess;
loser:
	CI_UNLOCK(psock);
	return SECFailure;
}

/*
 * General DSA signature verify routine. This routine will work with
 * any DSA signatures, as long as the SECKEYPublicKey has the fortezza
 * DSA key material in them.
 */
#ifndef FORTEZZA_SOFTWARE
#define DSA_WORDS 11
#define MAX_DSA_KEY 65
PRBool
DSAVerify(unsigned char *digest,unsigned char *signature, SECKEYPublicKey *key) {


	FortezzaPCMCIASocket *psock; 
        HSESSION hs;
	int error;
	SECStatus rv;

	rv = FortezzaFindOpenCard(&psock);
	if (rv < 0) return PR_FALSE;

	PORT_Assert( key->keyType == fortezzaKeys  );

        hs = psock->maci_session;

	CI_LOCK(psock); 
	if (psock->state == NoCard) {
	    goto loser;
	}
	

	/*
	 * RestoreState saves the current state as well as restoring our
	 * state. For signatures we have no state to restore... we just do
	 * it. That's why we pass it a NULL. We call restore state so that
	 * we don't trash any running encryption/decryption.
	 */	
#ifdef notdef
	error = MACI_Select(hs,psock->socketId);
	if (error != CI_OK) { goto loser; }
#endif


	error = MACI_LoadDSAParameters(hs,key->fortezzaKeyData.p.len,
			 key->fortezzaKeyData.q.len,key->fortezzaKeyData.p.data,
			 key->fortezzaKeyData.q.data,key->fortezzaKeyData.g.data);
	if (error != CI_OK) { goto loser; }

	error = MACI_VerifySignature(hs,digest,
				key->fortezzaKeyData.DSSKey.len,
				key->fortezzaKeyData.DSSKey.data,signature);	   
	if (error != CI_OK) { goto loser; }

	CI_UNLOCK(psock);
	return PR_TRUE;
loser:
	CI_UNLOCK(psock);
	return PR_FALSE;

	
}
#endif

/*
 * set up the fortezza hooks. This code was plagerized from sslauth.c
 */
int
Fortezza_CardSelectHook(int s, FortezzaCardSelect func, void *arg)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in AuthCertificateHook",
		 SSL_GETPID(), s));
	return -1;
    }

    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return(rv);
    }
    ss->sec->fortezzaCardSelect = func;
    ss->sec->fortezzaCardArg = arg;
    return(0);
}


/*
 * Error Logging or client notification.
 */
int
Fortezza_AlertHook(int s, FortezzaAlert func, void *arg)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in AuthCertificateHook",
		 SSL_GETPID(), s));
	return -1;
    }

    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return(rv);
    }
    ss->sec->fortezzaAlert = func;
    ss->sec->fortezzaAlertArg = arg;

    if (fortezzaGlobal.fortezzaAlert == NULL) {
    	fortezzaGlobal.fortezzaAlert = func;
    	fortezzaGlobal.fortezzaAlertArg = arg;
    }
    return(0);
}

/*
 * finding a new certificate
 */
int
Fortezza_CertificateSelectHook(int s, FortezzaCertificateSelect func, void *arg)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in AuthCertificateHook",
		 SSL_GETPID(), s));
	return -1;
    }

    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return(rv);
    }
    ss->sec->fortezzaCertificateSelect = func;
    ss->sec->fortezzaCertificateArg = arg;
    return(0);
}

/* Getting a pin */
int
Fortezza_GetPinHook(int s, FortezzaGetPin func, void *arg)
{
    SSLSocket *ss;
    int rv;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in AuthCertificateHook",
		 SSL_GETPID(), s));
	return -1;
    }

    if ((rv = ssl_CreateSecurityInfo(ss)) != 0) {
	return(rv);
    }
    ss->sec->fortezzaGetPin = func;
    ss->sec->fortezzaPinArg = arg;
    return(0);
}

/* 
 * template for unloading Fortezza pqg parameters
 */
DERTemplate FortezzaPreParamTemplate[] = {
	{ DER_BOOLEAN|DER_CONSTRUCTED|DER_CONTEXT_SPECIFIC, 0, NULL,
		sizeof(SECItem) },
	{ 0 },
};
DERTemplate FortezzaParameterTemplate[] = {
   	{ DER_SEQUENCE,  0, NULL, sizeof(SECKEYPublicKey) },
	{ DER_OCTET_STRING, offsetof(SECKEYPublicKey,fortezzaKeyData.p), },
	{ DER_OCTET_STRING, offsetof(SECKEYPublicKey,fortezzaKeyData.q), },
	{ DER_OCTET_STRING, offsetof(SECKEYPublicKey,fortezzaKeyData.g), },
	{ 0 },
};

/*
 * NOTE: this only decodes KEA_DSS type keys, not DSS only type keys.
 */
SECStatus
FortezzaDecodeCertKey(PRArenaPool *arena, SECKEYPublicKey *pubk, SECItem *rawkey,
							SECItem *params) {
	unsigned char *rawptr = rawkey->data;
	unsigned char *end = rawkey->data + rawkey->len;
	unsigned char *clearptr;
	SECItem preParams;
	SECStatus rv;

	/* first march down and decode the raw key data */

	/* version */
	pubk->fortezzaKeyData.KEAversion = *rawptr++;
	if (*rawptr++ != 0x01) {
		return SECFailure;
	}

	/* KMID */
	PORT_Memcpy(pubk->fortezzaKeyData.KMID,rawptr,
				sizeof(pubk->fortezzaKeyData.KMID));
	rawptr += sizeof(pubk->fortezzaKeyData.KMID);

	/* clearance (the string up to the first byte with the hi-bit on */
	clearptr = rawptr;
	while ((rawptr < end) && (*rawptr++ & 0x80));

	if (rawptr >= end) { return SECFailure; }
	pubk->fortezzaKeyData.clearance.len = rawptr - clearptr;
	pubk->fortezzaKeyData.clearance.data = 
		PORT_ArenaZAlloc(arena,pubk->fortezzaKeyData.clearance.len);
	if (pubk->fortezzaKeyData.clearance.data == NULL) {
		return SECFailure;
	}
	PORT_Memcpy(pubk->fortezzaKeyData.clearance.data,clearptr,
					pubk->fortezzaKeyData.clearance.len);

	/* KEAPrivilege (the string up to the first byte with the hi-bit on */
	clearptr = rawptr;
	while ((rawptr < end) && (*rawptr++ & 0x80));
	if (rawptr >= end) { return SECFailure; }
	pubk->fortezzaKeyData.KEApriviledge.len = rawptr - clearptr;
	pubk->fortezzaKeyData.KEApriviledge.data = 
		PORT_ArenaZAlloc(arena,pubk->fortezzaKeyData.KEApriviledge.len);
	if (pubk->fortezzaKeyData.KEApriviledge.data == NULL) {
		return SECFailure;
	}
	PORT_Memcpy(pubk->fortezzaKeyData.KEApriviledge.data,clearptr,
				pubk->fortezzaKeyData.KEApriviledge.len);


	/* now copy the key. The next to bytes are the key length, and the
	 * key follows */
	pubk->fortezzaKeyData.KEAKey.len = (*rawptr << 8) | rawptr[1];

	rawptr += 2;
	if (rawptr+pubk->fortezzaKeyData.KEAKey.len > end) { return SECFailure; }
	pubk->fortezzaKeyData.KEAKey.data = 
			PORT_ArenaZAlloc(arena,pubk->fortezzaKeyData.KEAKey.len);
	if (pubk->fortezzaKeyData.KEAKey.data == NULL) {
		return SECFailure;
	}
	PORT_Memcpy(pubk->fortezzaKeyData.KEAKey.data,rawptr,
					pubk->fortezzaKeyData.KEAKey.len);
	rawptr += pubk->fortezzaKeyData.KEAKey.len;

	/* shared key */
	if (rawptr >= end) {
	    pubk->fortezzaKeyData.DSSKey.len = pubk->fortezzaKeyData.KEAKey.len;
	    /* this depends on the fact that we are going to get freed with an
	     * ArenaFree call. We cannot free DSSKey and KEAKey separately */
	    pubk->fortezzaKeyData.DSSKey.data=
					pubk->fortezzaKeyData.KEAKey.data;
	    pubk->fortezzaKeyData.DSSpriviledge.len = 
				pubk->fortezzaKeyData.KEApriviledge.len;
	    pubk->fortezzaKeyData.DSSpriviledge.data =
			pubk->fortezzaKeyData.DSSpriviledge.data;
	    goto done;
	}
		

	/* DSS Version is next */
	pubk->fortezzaKeyData.DSSversion = *rawptr++;

	if (*rawptr++ != 2) {
		return SECFailure;
	}

	/* DSSPrivilege (the string up to the first byte with the hi-bit on */
	clearptr = rawptr;
	while ((rawptr < end) && (*rawptr++ & 0x80));
	if (rawptr >= end) { return SECFailure; }
	pubk->fortezzaKeyData.DSSpriviledge.len = rawptr - clearptr;
	pubk->fortezzaKeyData.DSSpriviledge.data = 
		PORT_ArenaZAlloc(arena,pubk->fortezzaKeyData.DSSpriviledge.len);
	if (pubk->fortezzaKeyData.DSSpriviledge.data == NULL) {
		return SECFailure;
	}
	PORT_Memcpy(pubk->fortezzaKeyData.DSSpriviledge.data,clearptr,
				pubk->fortezzaKeyData.DSSpriviledge.len);

	/* finally copy the DSS key. The next to bytes are the key length,
	 *  and the key follows */
	pubk->fortezzaKeyData.DSSKey.len = (*rawptr << 8) | rawptr[1];

	rawptr += 2;
	if (rawptr+pubk->fortezzaKeyData.DSSKey.len > end){ return SECFailure; }
	pubk->fortezzaKeyData.DSSKey.data = 
			PORT_ArenaZAlloc(arena,pubk->fortezzaKeyData.DSSKey.len);
	if (pubk->fortezzaKeyData.DSSKey.data == NULL) {
		return SECFailure;
	}
	PORT_Memcpy(pubk->fortezzaKeyData.DSSKey.data,rawptr,
					pubk->fortezzaKeyData.DSSKey.len);

	/* ok, now we decode the parameters */
done:
	rv = DER_Decode(arena, &preParams, FortezzaPreParamTemplate, params);
	if (rv < 0) return rv;

	return DER_Decode(arena,pubk, FortezzaParameterTemplate, &preParams);
}

/*
 * decode PQG Parameters.
 */
SECStatus
FortezzaBuildParams(SECItem *params,SECKEYPublicKey *key) {

	return DER_Encode(NULL, params, FortezzaParameterTemplate, key);
}

/*
 * create a der encoded version of a public key
 */
SECStatus
FortezzaEncodeCertKey(PRArenaPool *arena,SECItem *encodeKey,SECKEYPublicKey *pubk) {
	unsigned char *rawptr;

	encodeKey->len = 2 /* versions */ +
		 sizeof(pubk->fortezzaKeyData.KMID) +
		pubk->fortezzaKeyData.clearance.len +
		pubk->fortezzaKeyData.KEApriviledge.len +
		pubk->fortezzaKeyData.DSSpriviledge.len +
		pubk->fortezzaKeyData.KEAKey.len + 2 /* KEA Length */ +
		pubk->fortezzaKeyData.DSSKey.len + 2 /* DSS Length */;

	rawptr = (unsigned char *)PORT_ArenaAlloc(arena,encodeKey->len);
	if (rawptr == NULL) {
	    return SECFailure;
	}
	encodeKey->data = rawptr;

	/* march down and encode the raw key data */

	/* version */
	*rawptr++ = pubk->fortezzaKeyData.KEAversion;

	/* KMID */
	PORT_Memcpy(rawptr,pubk->fortezzaKeyData.KMID,
				sizeof(pubk->fortezzaKeyData.KMID));
	rawptr += sizeof(pubk->fortezzaKeyData.KMID);

	/* clearance (assumes the high bit is already on)  */
	PORT_Memcpy(rawptr,pubk->fortezzaKeyData.clearance.data,
					pubk->fortezzaKeyData.clearance.len);
	rawptr += pubk->fortezzaKeyData.clearance.len;

	/* KEAPrivilege */
	PORT_Memcpy(rawptr,pubk->fortezzaKeyData.KEApriviledge.data,
				pubk->fortezzaKeyData.KEApriviledge.len);
	rawptr += pubk->fortezzaKeyData.KEApriviledge.len;


	/* now copy the key. */
	*rawptr++=(unsigned char)((pubk->fortezzaKeyData.KEAKey.len>>8)&0xff);
	*rawptr++ = (unsigned char)(pubk->fortezzaKeyData.KEAKey.len & 0xff);
	PORT_Memcpy(rawptr,pubk->fortezzaKeyData.KEAKey.data,
				pubk->fortezzaKeyData.KEAKey.len);
	rawptr += pubk->fortezzaKeyData.KEAKey.len;

	/* DSS Version is next */
	*rawptr++=pubk->fortezzaKeyData.DSSversion;

	/* DSSPrivilege */
	PORT_Memcpy(rawptr,pubk->fortezzaKeyData.DSSpriviledge.data,
				pubk->fortezzaKeyData.DSSpriviledge.len);
	rawptr += pubk->fortezzaKeyData.DSSpriviledge.len;

	/* finally copy the DSS key. */
	*rawptr++=(unsigned char)((pubk->fortezzaKeyData.DSSKey.len>>8)&0xff);
	*rawptr++ = (unsigned char)(pubk->fortezzaKeyData.DSSKey.len & 0xff);
	PORT_Memcpy(rawptr,pubk->fortezzaKeyData.DSSKey.data,
				pubk->fortezzaKeyData.DSSKey.len);

	return SECSuccess;
}

/*
 * Generate a Key Exchange Random number. 
 */
SECStatus
FortezzaGenerateRa(FortezzaKEAContext *cx,unsigned char *ra) {
	int error;

	PORT_Assert(cx->socketId == cx->socket->socketId) ;
	CI_LOCK(cx->socket);

	error = MACI_SetPersonality(cx->socket->maci_session,cx->certificate);
	if (error != CI_OK) goto loser;

	error = MACI_GenerateRa(cx->socket->maci_session,ra);
	if (error != CI_OK) goto loser;

	CI_UNLOCK(cx->socket);
	return SECSuccess;

loser:
	FortezzaError(error,NULL,cx->socket);
	CI_UNLOCK(cx->socket);
	return SECFailure;
}


/*
 * When we're through with a tek, free up the slot... 
 * It's the same as FortezzaFreeKey, except it makes sure the key is a tek.
 * this is becaus fortezzaFreeKey call it, an this will prevent infinite
 * recursion.
 */
void
FortezzaTEKFree(FortezzaKey *tek) {
    /* avoid recursion... especially in the case of TEKMEK! */
    if (tek->keyType != TEK) return;

    FortezzaFreeKey(tek);
   return;
}

/*
 * Free a key and everything associated with it.
 */
void 
FortezzaFreeKey(FortezzaKey *key) {
	FortezzaPCMCIASocket *psock = key->socket;

	FORTEZZAPLOCK(psock->keyLock);
	if (key->ref_count == 1) {
	    /* get rid of any register references */
	    key->id = 0;
	    FortezzaRegisterFree(psock,key);
	    FORTEZZAPUNLOCK(psock->keyLock);

	    /* handle key specific stuff */
	    switch (key->keyType) {
	    case TEKMEK:
		FortezzaTEKFree(key->keyData.tek_mek.wrappingTEK);
		break;
	    case TEK:
        	if (key->keyData.tek.destroy_certificate) {
           	     FortezzaDeletePersonality(&psock->template,
						key->keyData.tek.certificate);
        	}
		break;
	    default:
		break;
	    }
	    /* OK, now free the key */
	    FORTEZZAPLOCK(fortezzaGlobal.keyListLock);
	    key->next = fortezzaGlobal.keyTop;
	    fortezzaGlobal.keyTop = key;
	    FORTEZZAPUNLOCK(fortezzaGlobal.keyListLock);
	    /* PORT_Free(key); */
	    return;
	}
	FORTEZZAPUNLOCK(psock->keyLock);
	return;
}

#ifdef SUNOS4
/* Sigh make maci happy */
#undef strerror
char *strerror(unsigned e) {
	return sys_errlist[((unsigned)(e) < sys_nerr) ? e : 0];
}
#endif

unsigned long FortezzaKeyID(FortezzaKey *key) { return key->id; }

void FortezzaKeyLock(int socket) {
	FortezzaPCMCIASocket *psock;

	if ((socket <= 0) || (socket > fortezzaGlobal.socketCount)) {
		return;
	}
	psock  = &fortezzaGlobal.sockets[socket-1];
	FORTEZZAPLOCK(psock->keyLock);
}

void FortezzaKeyUnlock(int socket) {
	FortezzaPCMCIASocket *psock;

	if ((socket <= 0) || (socket > fortezzaGlobal.socketCount)) {
		return;
	}
	psock  = &fortezzaGlobal.sockets[socket-1];
	FORTEZZAPUNLOCK(psock->keyLock);
}

/* reference the key (already have the lock) */
void FortezzaKeyReferenceLocked(FortezzaKey *key) { key->ref_count++; }

/* normal Fortezza key Reference */
void FortezzaKeyReference(FortezzaKey *key) {
    FortezzaPCMCIASocket *psock = key->socket;
    FORTEZZAPLOCK(psock->keyLock);
    key->ref_count++;

    FORTEZZAPUNLOCK(psock->keyLock);
}

/* Cached keys may have a race which causes them to free twice.
 * Since a key is unique to a session, we know it can only be
 * cached once, so only ree it if it was cached.
 */
void FortezzaFreeCachedKey(FortezzaKey *key,unsigned long id) {
    FortezzaPCMCIASocket *psock = key->socket;

    FORTEZZAPLOCK(psock->keyLock);
    if ((key->id == id) && (key->cached)) {
	key->cached = PR_FALSE;
	FORTEZZAPUNLOCK(psock->keyLock);
	FortezzaFreeKey(key);
    }
    FORTEZZAPUNLOCK(psock->keyLock);
    return;
}

/*
 * mark a key as cached before we save it.
 */
void FortezzaCachedKey(FortezzaKey *key) {
    FortezzaPCMCIASocket *psock = key->socket;
    FORTEZZAPLOCK(psock->keyLock);
    key->cached = PR_TRUE;
    key->ref_count++;
    FORTEZZAPUNLOCK(psock->keyLock);
}

/*
 * set the timeout value  (called from FE's
 */
void
FortezzaSetTimeout(int time) {
/* time comes in in minutes, we store it as seconds */
#ifdef HAVE_LONG_LONG
    LL_MUL(fortezzaGlobal.timeout,(int64)time,(int64)60);
#else
    LL_MUL32(fortezzaGlobal.timeout,time,60);
#endif
}
	


#ifdef FORTEZZA_NEED_GROSS_LOCK
/*
 * again this should be NSPR routines. It needs.
 * if you don't have any real mutexes, we use system V
 * semaphores (slow). Only Sunos uses them.
 */
#include <sys/ipc.h>
#include <sys/sem.h>

/*
 * first we initialize the semaphore....
 */
static int fsemid = 0;
FortezzaPlock *
FortezzaInitPlock(void) {
	static int init= 0;
	FortezzaPlock *lock;

	lock = (FortezzaPlock *) PORT_Alloc(sizeof(FortezzaPlock));
	if (lock == NULL) return NULL;

	/* get our semaphore if we haven't got it yet */
	if (init == 0) {
		union semun arg;
		int i;

		fsemid=semget(IPC_PRIVATE, 10, 0600);
		if (fsemid < 0) {
			PORT_Free(lock);
			return NULL;
		}
		/* initialize the semaphores to 1 */
		arg.val = 1;
		for (i=0; i < 10; i++) {
			semctl(fsemid,i,SETVAL,&arg);
		}
	}

	/* initialize the Plock structure */
	lock->semid = fsemid;
	if (init < 10) {
		lock->sem_num = init++;
	} else {
		PORT_Free(lock);
		lock= NULL;
	}
	return lock;
}

int
FortezzaSetPLock(FortezzaPlock *lock,int x) {
	struct sembuf sops;

	PORT_Assert(lock != NULL);
	if (lock == NULL) return -1;
	sops.sem_num = lock->sem_num;
	sops.sem_op = x;
	sops.sem_flg = SEM_UNDO;
	return semop(lock->semid,&sops,1);
}

void
FortezzaDeletePlocks() {
	if (fsemid > 0) semctl(fsemid,0,IPC_RMID,NULL);
}
#endif


/*
 * When we have semaphores we need to clean then up before
 * we die. This cleanup function is tied to the watchdog deamon.
 */
void
fortezza_cleanup_init(void) {
#ifdef FORTEZZA_NEED_GROSS_LOCK
	on_exit(FortezzaDeletePlocks,NULL);
#endif
}
