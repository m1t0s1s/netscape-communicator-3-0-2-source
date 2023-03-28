/*
 * Definition of Security Module Data Structure. There is a separate data
 * structure for each loaded PKCS #11 module.
 */
#ifndef _SECMODT_H_
#define _SECMODT_H_ 1

/* PKCS11 needs to be included */
typedef struct SECMODModuleStr SECMODModule;
typedef struct SECMODModuleListStr SECMODModuleList;
typedef struct SECMODListLockStr SECMODListLock; /* defined in secmodi.h */
typedef struct PK11SlotInfoStr PK11SlotInfo; /* defined in secmodti.h */
typedef struct PK11PreSlotInfoStr PK11PreSlotInfo; /* defined in secmodti.h */
typedef struct PK11SymKeyStr PK11SymKey; /* defined in secmodti.h */
typedef struct PK11ContextStr PK11Context; /* defined in secmodti.h */
typedef struct PK11SlotListStr PK11SlotList;
typedef struct PK11SlotListElementStr PK11SlotListElement;
typedef struct PK11RSAGenParamsStr PK11RSAGenParams;

struct SECMODModuleStr {
    PRArenaPool	*arena;
    PRBool	internal;	/* true of internally linked modules, false
				 * for the loaded modules */
    PRBool	loaded;		/* Set to true if module has been loaded */
    PRBool	isFIPS;		/* Set to true if module is finst internal */
    char	*dllName;	/* name of the shared library which implements
				 * this module */
    char	*commonName;	/* name of the module to display to the user */
    void	*library;	/* pointer to the library. opaque. used only by
				 * pk11load.c */
    void	*functionList; /* The PKCS #11 function table */
    void	*refLock;	/* only used pk11db.c */
    int		refCount;	/* Module reference count */
    PK11SlotInfo **slots;	/* array of slot points attatched to this mod*/
    int		slotCount;	/* count of slot in above array */
    PK11PreSlotInfo *slotInfo;	/* special info about slots default settings */
    int		slotInfoCount;  /* count */
    unsigned long ssl[2];	/* SSL cipher enable flags */
};

struct SECMODModuleListStr {
    SECMODModuleList	*next;
    SECMODModule	*module;
};

struct PK11SlotListStr {
    PK11SlotListElement *head;
    void *lock;
};

struct PK11SlotListElementStr {
    PK11SlotListElement *next;
    PK11SlotListElement *prev;
    PK11SlotInfo *slot;
    int refCount;
};

struct PK11RSAGenParamsStr {
    int keySizeInBits;
    unsigned long pe;
};

#define SECMOD_RSA_FLAG 	0x00000001
#define SECMOD_DSA_FLAG 	0x00000002
#define SECMOD_RC2_FLAG 	0x00000004
#define SECMOD_RC4_FLAG 	0x00000008
#define SECMOD_DES_FLAG 	0x00000010
#define SECMOD_DH_FLAG	 	0x00000020
#define SECMOD_FORTEZZA_FLAG	0x00000040
#define SECMOD_RANDOM_FLAG	0x80000000L
/* sigh need to make SECMOD and PK11 prefixes consistant!! */
#define PK11_OWN_PW_DEFAULTS 0x20000000L
#define PK11_DISABLE_FLAG    0x40000000L

/* FAKE PKCS #11 defines */
#define CKM_FAKE_RANDOM       0x80000efeL
#define CKM_INVALID_MECHANISM 0xffffffffL
#define CKA_DIGEST            0x81000000L
#define CK_INVALID_KEY 0
#define CK_INVALID_SESSION 0

/* Cryptographic module types */
#define SECMOD_EXTERNAL	0	/* external module */
#define SECMOD_INTERNAL 1	/* internal default module */
#define SECMOD_FIPS	2	/* internal fips module */

#endif /*_SECMODT_H_ */
