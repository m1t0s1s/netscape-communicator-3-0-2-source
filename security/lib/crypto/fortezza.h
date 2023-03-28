/*
 * Fortezza Internal data structures
 */
#ifndef _FORTEZZA_H_
#define _FORTEZZA_H_ 1
#include "genci.h"
#include "prlong.h"


/* first deal with Threading vs no thread ing */
#ifdef USE_NSPR_MT

/* we are really multi-threaded, get real locks */
#define	FORTEZZAINITLOCK(l,i) if (!i) { l=PR_NewMonitor(); i++; }
#define FORTEZZALOCK(l,i) PORT_Assert(i); PR_EnterMonitor(l)
#define FORTEZZAUNLOCK(l) PR_ExitMonitor(l)
#define FORTEZZAWAKEUP(l) PR_Notify(l);
typedef PRMonitor FortezzaMonitor;

/* get multi-process locks .... 
 * These should all really be part of
 * NSPR, but it seems no one else needs to sychronize across
 * processes */
#if defined(SERVER_BUILD) && defined(XP_UNIX)
#ifdef AIX
#include <sys/mman.h>
#define FORTEZZAPLOCK(l) msem_lock(l,0)
#define FORTEZZAPUNLOCK(l) msem_unlock(1,0)
#define FORTEZZAINITPLOCK(l) msem_init(sem,MSEM_UNLOCKED)
#define FORTEZZA_HAS_PLOCK
typedef msemaphore FortezzaPlock;
#endif /* AIX */
#ifdef SOLARIS
#include <synch.h>
#define FORTEZZAPLOCK(l) mutex_lock(l)
#define FORTEZZAPUNLOCK(l) mutex_unlock(l)
#define FORTEZZAINITPLOCK(l) mutex_init(l,USYNC_PROCESS,NULL)
#define FORTEZZA_HAS_PLOCK
typedef mutex_t FortezzaPlock;
#endif /* SOLARIS */
/* If we don't have a a fast lock scheme, use a slow one */
#ifndef FORTEZZA_HAS_PLOCK
#define FORTEZZA_NEED_GROSS_LOCK
#define FORTEZZAPLOCK(l) FortezzaSetPLock(l,-1)
#define FORTEZZAPUNLOCK(l) FortezzaSetPLock(l,1)
#define FORTEZZAINITPLOCK(l) l = FortezzaInitPlock()
typedef struct FortezzaPlockStr FortezzaPlock;
struct FortezzaPlockStr {
	int semid;
	int sem_num;
};
FortezzaPlock *FortezzaInitPlock(void);
int FortezzaSetPLock(FortezzaPlock *,int);
void FortezzaDeletePlocks();
#endif /* OTHER */
#else /* No other processes, PLOCK == LOCK */
#define FORTEZZAPLOCK(l) PR_EnterMonitor(l)
#define FORTEZZAPUNLOCK(l) PR_ExitMonitor(l)
#define FORTEZZAINITPLOCK(l) l=PR_NewMonitor()
typedef PRMonitor FortezzaPlock;
#endif

/*
 * On UNIX, where we run full MACI we need to protect MACI based on
 * which socket we're using. On the other platforms, we are running
 * CI_lib emulation, so we need to protect all CI_lib calls.
 */
#ifdef XP_UNIX
#define CI_LOCKA(psock) \
    FORTEZZALOCK(psock->libLock,psock->libLockInit)
#define CI_UNLOCKA(psock) \
    FORTEZZAUNLOCK(psock->libLock)
#else
#define CI_LOCKA(psock) \
    FORTEZZALOCK(fortezzaGlobal.libLock,fortezzaGlobal.libLockInit)
#define CI_UNLOCKA(psock) \
    FORTEZZAUNLOCK(fortezzaGlobal.libLock)
#endif /* !XP_UNIX */

#else
/* not really multi-threaded, no need to lock */
#define	FORTEZZAINITLOCK(l,i) 
#define FORTEZZALOCK(l,i) 
#define FORTEZZAUNLOCK(l)
#define FORTEZZAWAKEUP(l) 
#define CI_LOCKA(psock)
#define CI_UNLOCKA(psock)
typedef int FortezzaMonitor;
typedef int FortezzaPlock;
#define FORTEZZAPLOCK(l)
#define FORTEZZAPUNLOCK(l)
#define FORTEZZAINITPLOCK(l)
#endif

/* Whenever we lock, we need to lock our local thread (to protect the MACI
 * library, then lock the card using MACI_Lock(). In some cases we need to
 * do a CI_Select() first (those cases we aren't using full MACI).
 */
#define CI_LOCK(psock) CI_LOCKA(psock); \
	MACI_SEL(psock->socketId); \
	MACI_Lock(psock->maci_session,CI_BLOCK_LOCK_FLAG)
#define CI_UNLOCK(psock) MACI_Unlock(psock->maci_session); CI_UNLOCKA(psock)

/*
 * key defines an preallocations stuff 
 */
/* number of new keys to allocate when we grab new keys */
#define NEW_KEY_COUNT 10
/* maximum number of registers we can handle */
#define FORTEZZA_MAX_REG  50
/* number of keys to initially allocate */
#define FORTEZZA_INIT_KEYS 30
/* size of Shared memory segment to allocate */
#define FORTEZZA_MAX_SHARED_MEM (8*1024*1024)

/*
 * Structure aliases and common fortezza string types
 */
typedef struct FortezzaTEKStr FortezzaTEK;
typedef struct FortezzaMEKStr FortezzaMEK;
typedef struct FortezzaTEKMEKStr FortezzaTEKMEK;
/* find the below definition sslimpl.h (SIGH) */
/*typedef struct FortezzaKeyStr FortezzaKey;*/
typedef struct FortezzaContextStr FortezzaContext;
typedef struct FortezzaKEAContextStr FortezzaKEAContext;
typedef struct FortezzaPCMCIASocketStr FortezzaPCMCIASocket;
typedef struct FortezzaGlobalStr FortezzaGlobal;
typedef struct FortezzaCardInfoStr FortezzaCardInfo;
typedef unsigned char FortezzaPublic[128];/* public key, p, g and Rx lengths */
typedef unsigned char FortezzaSymetric[12]; /* symetric key lengths */
typedef unsigned char FortezzaIV[24];
typedef unsigned char FortezzaCryptSave[28];

typedef struct SSL3CertNodeStr SSL3CertNode;

/*
 * internal callback routines...
 */
typedef void (*FortezzaMarkSocket)(SSLSocket *ss);

/* Token Encryption Key */
struct FortezzaTEKStr {
	FortezzaPublic		Y_b;
	FortezzaPublic		R_a;
	FortezzaPublic		R_b;
	int			certificate;  /* to get X_a, & PQG */
	int			flags;
	int			Y_bSize;
	PRBool			destroy_certificate;
};

/* Message Encryption Key */
struct FortezzaMEKStr {
	FortezzaSymetric	Ks_M;         /* Ks Wrapped MEK */
};

/* Message Encryption Key wrapped with a TEK*/
struct FortezzaTEKMEKStr {
	FortezzaKey		*wrappingTEK;
	FortezzaSymetric	TEK_M;         /* TEK Wrapped MEK */
};

typedef enum { NOKEY, TEK, MEK, TEKMEK } FortezzaKeyType;

/*
 * unified key data structure. Has use count, and current fortezza
 * register.
 */
struct FortezzaKeyStr {
	FortezzaKey	*next;
	int		keyRegister;
	uint32  	hitCount;
	FortezzaKeyType	keyType;
	int		ref_count;
	FortezzaPCMCIASocket *socket;
	unsigned long	id;
	PRBool		cached;
	union {
		FortezzaTEKMEK	tek_mek;
		FortezzaMEK	mek;
		FortezzaTEK	tek;
	} keyData;
};

#define KeyNotLoaded	-1


/*
 * for now we just save onto and off of the card. Later we can get
 * fancy and store internal as well
 */
typedef enum { BadCard, Saved, ServerInit, Init, NotInit } 
							FortezzaCryptState;

/* changing the following enum require changes arrays in algfor.c */
typedef enum { Encrypt=0 , Decrypt=1 , Hash=2 } FortezzaCryptType;
struct FortezzaContextStr {
	FortezzaContext	*next;
	FortezzaKey	*theKey;
	FortezzaPCMCIASocket	*socket;
	FortezzaCryptType	type;
	FortezzaCryptState	state;
	FortezzaCryptSave	save;
	FortezzaIV		iv;
	int		certificate;
	int		socketId;
};

/* save kea exchange info before we have crypto context's */
struct FortezzaKEAContextStr {
	FortezzaPCMCIASocket	*socket;
	FortezzaPublic		R_s;
	FortezzaKey	*tek;
	int		certificate;
	int		socketId;
};

typedef FortezzaKey *FortezzaKeyRegister;

typedef enum { NoCard, Ready, NeedPin, NeedCert, InOpen } FortezzaCardState;
struct FortezzaPCMCIASocketStr {
	int			socketId;
	CI_SERIAL_NUMBER	serial;
	FortezzaCardState	state;	
	int			stateLockInit;
	FortezzaMonitor		*stateLock;
  	int64			lasttime;
	uint32			load;
	int			libLockInit;
	FortezzaMonitor		*libLock;
	HSESSION		maci_session;

	/* linked list of contexts using this card */
	FortezzaContext		*head;	 
	/* locks for the linked list */
	int			listLockInit;
	FortezzaMonitor		*listLock;

	/* key register managment */
	int			registerCount;
	FortezzaKeyRegister	*registers;
	uint32			hitCount;
	/* locks for the key register structures */
	int			keyLockInit;
	FortezzaPlock		*keyLock;

	/* number of certificates on card */
	int			certLockInit;
	FortezzaMonitor		*certLock;
	int			certificateCount;
	int			personality;
	CI_PERSON		*personalityList;
	SECItem			*certificateList;
	CERTCertificate		*rootCert;
	CI_CERT_FLAGS		certFlags;

	/* general template for this PCMCIA socket */
	FortezzaKEAContext	template;
};

/* the following states control fortezza dialogs. GlobalOK means that
 * the path is clear, no dialogs are in progress. GlobalOpen means that
 * there is a fortezza dialog pending. GlobalSkip is set if a dialog
 * was cancelled... the user evidently does not want to use fortezza,
 * continue making the connection with the other options.
 */
typedef enum { GlobalOK, GlobalOpen, GlobalSkip } FortezzaGlobalState;
struct FortezzaGlobalStr {
	int			socketCount;
	int			libLockInit;
	FortezzaMonitor		*libLock;
	int			paramLockInit;
	FortezzaMonitor		*paramLock;
	FortezzaPlock		*keyListLock;
	FortezzaKey		*keyTop;	/* top of the free keyList*/
	unsigned long		*keyid;		/* next id (in shared mem) */
	FortezzaPCMCIASocket	*sockets;
	FortezzaPCMCIASocket	*defaultSocket;
	int			serverCertificate;
	FortezzaGlobalState	state;
	SECSocketNode		*sslList;
	unsigned char		*memaddr;
	FortezzaMarkSocket	fortezzaMarkSocket;
	FortezzaAlert		fortezzaAlert;
	void			*fortezzaAlertArg;
	uint32			mask;
	int64			timeout;
};

extern FortezzaGlobal fortezzaGlobal;

struct FortezzaCardInfoStr {
	CI_SERIAL_NUMBER	serial;
	FortezzaCardState	state;
};

/*
 * do an encrypt, decrypt or hash depending on the context type
 */
SECStatus Fortezza_Op(FortezzaContext *cx, unsigned char *out, unsigned *part,
		unsigned int maxOut, unsigned char *in, unsigned inLen);

FortezzaContext *FortezzaCreateContext(FortezzaKEAContext *template, 
						FortezzaCryptType type);
FortezzaKEAContext *FortezzaCreateKEAContext(FortezzaPCMCIASocket *);
FortezzaKEAContext *FortezzaCreateKEAContextFromSocket(int socket);
void FortezzaDestroyContext(FortezzaContext *cx,PRBool freeit);
void FortezzaDestroyKEAContext(FortezzaKEAContext *cx,PRBool freeit);
SECStatus FortezzaSetKey(FortezzaContext *cx, FortezzaKey *key);
SECStatus FortezzaSetServerIniit(FortezzaContext *cx);
void FortezzaFreeKey(FortezzaKey *key);
SECStatus FortezzaGenerateKeys(FortezzaContext *cx, FortezzaKey *tek, 
						FortezzaSymetric wrapped_key);
SECStatus FortezzaRewrapKey(FortezzaKEAContext *kea_cx, FortezzaSymetric inkey,
						FortezzaSymetric wrapped_key);
SECStatus FortezzaUnwrapKey(FortezzaContext *cx, FortezzaKey *tek, 
						FortezzaSymetric wrapped_key);
SECStatus FortezzaGenerateIV(FortezzaContext *cx, FortezzaIV iv);
SECStatus FortezzaLoadIV(FortezzaContext *cx, FortezzaIV iv);
SECStatus FortezzaLoadSave(FortezzaContext *cx, FortezzaCryptSave save);
void FortezzaGlobalInit(void);
SECStatus FortezzaOpen(SSLSocket *ss,FortezzaKEAContext **cx);
CERTCertificate *FortezzaGetCertificate(int sockID,int certID,unsigned char *serial);
SECStatus FortezzaPQGUpdate(SSL3CertNode *certs,CERTCertificate *);
SECStatus FortezzaSign(unsigned char *digest,unsigned char *signature, SECKEYPrivateKey *key);
PRBool DSAVerify(unsigned char *digest,unsigned char *signature, SECKEYPublicKey *key);
SECStatus FortezzaDecodeCertKey(PRArenaPool *arena, SECKEYPublicKey *pubk, 
		SECItem *rawkey, SECItem *params);
SECStatus FortezzaBuildParams(SECItem *params,SECKEYPublicKey *key);
SECStatus FortezzaEncodeCertKey(PRArenaPool *arena,SECItem *encodeKey,
							SECKEYPublicKey *key);
SECStatus FortezzaGenerateRa(FortezzaKEAContext *cx,unsigned char *);
SECStatus FortezzaPinCheck(FortezzaPCMCIASocket *psock, char *pin);
SECKEYPrivateKey *FortezzaGetPrivateKey(FortezzaKEAContext *cx);
SECStatus FortezzaGetCertList(FortezzaKEAContext *cx,CERTCertificateList **chain,
				SECKEYPrivateKey **key, CERTCertificate **cert);
SECStatus FortezzaSelectCertificate(SSLSocket *ss, FortezzaPCMCIASocket *psock,
				   int *certificate);
SECStatus FortezzaCryptPremaster(FortezzaCryptType type,FortezzaKEAContext *cx,
  FortezzaKey *fk, FortezzaIV iv, unsigned char *in, 
						unsigned char *out, int size);
SECStatus FortezzaLookupCard(FortezzaCardInfo **info, uint32 *mask,
 int *tcount, int *found, FortezzaPCMCIASocket **min,
				 FortezzaPCMCIASocket **last, PRBool isServer);
SECStatus FortezzaLookupPersonality(FortezzaPCMCIASocket *psock, char ***parray,
				int *found, int **index);
void FortezzaFreePersonality(char **personalities);
SECStatus FortezzaCertInit(FortezzaPCMCIASocket *psock);
SECStatus FortezzaGetConfig(FortezzaPCMCIASocket *psock,
					CI_CONFIG *config,CI_STATUS *status);
int FortezzaComparePQG(CERTCertificate *c1,CERTCertificate *c2);
SECStatus FortezzaGenerateNewPersonality(FortezzaKEAContext *cx,int *certIndex,
                                FortezzaPublic y, CERTCertificate *cert) ;
SECStatus FortezzaDeletePersonality(FortezzaKEAContext *cx,int cert);
void FortezzaLogout();
CERTCertificate * FortezzaFindCertByNickname(char *certname);
void FortezzaTEKFree(FortezzaKey *tek);
FortezzaKey *FortezzaTEKAlloc(FortezzaKEAContext *cx);
FortezzaKey *FortezzaKeyAlloc(FortezzaKEAContext *cx);
SECStatus FortezzaRestoreKey(FortezzaPCMCIASocket *psock,FortezzaContext *cx,
							FortezzaKey *key);
FortezzaKEAContext *SECMOZ_FindCardBySerial(void *wcx,unsigned char *serial);
#ifdef USE_CARD_RANDOM
void Fortezza_GenerateRandom(unsigned char *ptr,int len);
#else
#define Fortezza_GenerateRandom RNG_GenerateRandomBytes 
#endif

unsigned long FortezzaKeyID(FortezzaKey *);
void FortezzaKeyUnlock(int socket);
void FortezzaKeyLock(int socket);
void FortezzaKeyReference(FortezzaKey *);
void FortezzaFreeCachedKey(FortezzaKey *,unsigned long id);
void FortezzaCachedKey(FortezzaKey *);
SECStatus FortezzaSetServerInit(FortezzaContext *);

#endif /* _FORTEZZA_H_ */
