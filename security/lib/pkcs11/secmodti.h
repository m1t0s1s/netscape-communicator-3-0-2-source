/*
 * Internal header file included only by files in pkcs11 dir, or in
 * pkcs11 specific client and server files.
 */

#ifdef NSPR20
#include "prmon.h"
#include "prtypes.h"
#endif /* NSPR20 */

typedef struct PK11DefaultArrayEntryStr PK11DefaultArrayEntry;

/* internal data structures */
struct SECMODListLockStr {
    PRLock	*mutex;
    PRMonitor	*monitor;
    int		state;
    int		count;
};

/* represet a pkcs#11 slot */
struct PK11SlotInfoStr {
    /* the PKCS11 function list for this slot */
    void *functionList;
    SECMODModule *module; /* our parent module */
    /* Boolean to indicate the current state of this slot */
    PRBool needTest;
    PRBool isPerm;
    PRBool isInternal;
    PRBool disabled;
    int	   reason; 	/* Why this slot is disabled */
    PRBool readOnly;
    PRBool needLogin;
    PRBool hasRandom;
    PRBool defRWSession;
    /* The actual flags (many of which are distilled into the above PRBools */
    CK_FLAGS flags;
    /* a default session handle to do quick and dirty functions */
    CK_SESSION_HANDLE session;
    /* our ID */
    CK_SLOT_ID slotID;
    /* persistant flags saved from startup to startup */
    unsigned long defaultFlags;
    /* keep track of who is using us so we don't accidently get freed while
     * still in use */
    int refCount;
    PRLock *refLock;
    /* Password control functions for this slot */
    int askpw;
    int timeout;
    int authTransact;
    time_t authTime;
    int minPassword;
    int maxPassword;
    /* cache the certificates stored on the token of this slot */
    CERTCertificate **cert_array;
    int cert_count;
    /* since these are odd sizes, keep them last */
    char slot_name[65];
    char token_name[33];
};

/* hold slot default flags until we initialize a slot */
struct PK11PreSlotInfoStr {
    CK_SLOT_ID slotID;  	/* slot these flags are for */
    unsigned long defaultFlags; /* bit mask of default implementation this slot
				 * provides */
    int askpw;
    int timeout;
};

/* Symetric Key structure */
struct PK11SymKeyStr {
    CK_MECHANISM_TYPE type;	/* type of operation this key was created for*/
    CK_OBJECT_HANDLE  objectID; /* object id of this key in the slot */
    PK11SlotInfo      *slot;    /* Slot this key is loaded into */
    void	      *cx;	/* window context in case we need to loggin */
    SECItem	data;		/* raw key data if available */
    int		refCount;	/* number of references to this key */
    PRLock	*refLock;
    int		size;		/* key size in bytes */
};

struct PK11ContextStr {
    CK_ATTRIBUTE_TYPE	operation; /* type of operation this context is doing*/
    PK11SymKey  	*key;	   /* symetric key used in this context */
    PK11SlotInfo	*slot;	   /* slot this context is operationing on */
    CK_SESSION_HANDLE	session;   /* session this context is using */
    PRBool		ownSession;/* do we own the session? */
    void 		*cx;	   /* window context in case we need to loggin*/
    void		*savedData;/* save data when we are multiplexing on a
				    * single context */
    unsigned long	savedLength;
    SECItem		*param;
    PRBool		init;
    CK_MECHANISM_TYPE	type;
};

struct PK11DefaultArrayEntryStr {
    char *name;
    unsigned long flag;
    CK_MECHANISM_TYPE mechanism;
};

