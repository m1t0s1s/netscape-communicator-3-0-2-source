/*
 * Internal data structures used by pkcs11.c
 */
#ifndef _PKCS11I_H_
#define _PKCS11I_H_ 1

#include "prlock.h"

#ifdef PKCS11_USE_THREADS
#define PK11_USE_THREADS(x) x
#else
#define PK11_USE_THREADS(x) 
#endif

typedef struct PK11AttributeStr PK11Attribute;
typedef struct PK11ObjectListStr PK11ObjectList;
typedef struct PK11ObjectListElementStr PK11ObjectListElement;
typedef struct PK11ObjectStr PK11Object;
typedef struct PK11SessionStr PK11Session;
typedef struct PK11SlotStr PK11Slot;
typedef struct PK11SessionContextStr PK11SessionContext;
typedef struct PK11SearchResultsStr PK11SearchResults;

typedef void (*PK11Destroy)(void *, PRBool);
typedef SECStatus (*PK11Cipher)(void *,void *,unsigned int *,unsigned int,
					void *, unsigned int);
typedef SECStatus (*PK11Verify)(void *,void *,unsigned int,void *,unsigned int);
typedef void (*PK11Hash)(void *,void *,unsigned int);
typedef void (*PK11End)(void *,void *,unsigned int *,unsigned int);
typedef void (*PK11Free)(void *);

#define HASH_SIZE 32
#define SESSION_HASH_SIZE 64

/* Value to tell if an attribute is modifiable or not.
 *    NEVER: attribute is only set on creation.
 *    ONCOPY: attribute is set on creation and can only be changed on copy.
 *    SENSITIVE: attribute can only be changed to TRUE.
 *    ALWAYS: attribute can always be changed.
 */
typedef enum {
	PK11_NEVER = 0,
	PK11_ONCOPY = 1,
	PK11_SENSITIVE = 2,
	PK11_ALWAYS = 3
} PK11ModifyType;

/*
 * Free Status Enum... tell us more information when we think we're
 * deleting an object.
 */
typedef enum {
	PK11_DestroyFailure,
	PK11_Destroyed,
	PK11_Busy
} PK11FreeStatus;

/*
 * attribute values of an object.
 */
struct PK11AttributeStr {
    PK11Attribute  	*next;
    PK11Attribute  	*prev;
    int 		refCount;
    PRLock 		*refLock;
    /*must be called handle to make pk11queue_find work */
    CK_ATTRIBUTE_TYPE	handle;
    CK_ATTRIBUTE 	attrib;
};

struct PK11ObjectListStr {
    PK11ObjectList *next;
    PK11ObjectList *prev;
    PK11Object	   *parent;
};

/*
 * PKCS 11 crypto object structure
 */
struct PK11ObjectStr {
    PK11Object *next;
    PK11Object *prev;
    PK11ObjectList sessionList;
    CK_OBJECT_HANDLE handle;
    int refCount;
    PRLock 		*refLock;
    PRLock		*attributeLock;
    PK11Session   	*session;
    PK11Slot	   	*slot;
    CK_OBJECT_CLASS 	class;
    void 		*objectInfo;
    PK11Free 		infoFree;
    char		*label;
    PRBool		inDB;
    PK11Attribute 	*head[HASH_SIZE];
};

/*
 * struct to deal with a temparary list of objects
 */
struct PK11ObjectListElementStr {
    PK11ObjectListElement	*next;
    PK11Object 			*object;
};

/*
 * Area to hold Search results
 */
struct PK11SearchResultsStr {
    CK_OBJECT_HANDLE	*handles;
    int			size;
    int			index;
};


/* 
 * the universal crypto/hash/sign/verify context structure
 */
typedef enum {
    PK11_ENCRYPT,
    PK11_DECRYPT,
    PK11_HASH,
    PK11_SIGN,
    PK11_SIGN_RECOVER,
    PK11_VERIFY,
    PK11_VERIFY_RECOVER
} PK11ContextType;


struct PK11SessionContextStr {
    PK11ContextType	type;
    PRBool		multi; /* is multipart */
    void		*cipherInfo;
    unsigned int	cipherInfoLen;
    CK_MECHANISM_TYPE	currentMech;
    PK11Cipher		update;
    PK11Hash		hashUpdate;
    PK11End		end;
    PK11Destroy		destroy;
    PK11Verify		verify;
    unsigned int	maxLen;
};

/*
 * Sessions (have objects)
 */
struct PK11SessionStr {
    PK11Session        *next;
    PK11Session        *prev;
    CK_SESSION_HANDLE	handle;
    int			refCount;
    PRLock 		*refLock;
    PRLock		*objectLock;
    int			objectIDCount;
    CK_SESSION_INFO	info;
    CK_NOTIFY		notify;
    CK_VOID_PTR		appData;
    PK11Slot		*slot;
    PK11SearchResults	*search;
    PK11SessionContext	*context;
    PK11ObjectList	*objects[1];
};

/*
 * slots (have sessions and objects)
 */
struct PK11SlotStr {
    CK_SLOT_ID		slotID;
    PRLock		*sessionLock;
    PRLock		*objectLock;
    SECItem		*password;
    PRBool		hasTokens;
    PRBool		isLoggedIn;
    PRBool		ssoLoggedIn;
    PRBool		needLogin;
    PRBool		DB_loaded;
    int			sessionIDCount;
    int			sessionCount;
    int			rwSessionCount;
    int			tokenIDCount;
    PK11Object		*tokObjects[HASH_SIZE];
    PK11Session		*head[SESSION_HASH_SIZE];
};

/*
 * session handle modifiers
 */
#define PK11_PRIVATE_KEY_FLAG	0x80000000L

/*
 * object handle modifiers
 */
#define PK11_TOKEN_MASK		0x80000000L
#define PK11_TOKEN_MAGIC	0x80000000L
#define PK11_TOKEN_TYPE_MASK	0x70000000L
#define PK11_TOKEN_TYPE_CERT	0x00000000L
#define PK11_TOKEN_TYPE_PRIV	0x10000000L

/* how big a password/pin we can deal with */
#define PK11_MAX_PIN	255

/* slot helper macros */
#define pk11_SlotFromSessionHandle(handle) (((handle) & PK11_PRIVATE_KEY_FLAG)\
			 ? &pk11_slot[1] : &pk11_slot[0])
#define PK11_TOSLOT1(handle) handle &= ~PK11_PRIVATE_KEY_FLAG
#define PK11_TOSLOT2(handle) handle |= PK11_PRIVATE_KEY_FLAG
#define pk11_SlotFromSession(sp) ((sp)->slot)
#define pk11_SlotFromID(id) ((id) == NETSCAPE_SLOT_ID ? \
	&pk11_slot[0] : (((id) == PRIVATE_KEY_SLOT_ID) ? &pk11_slot[1] : NULL))
#define pk11_isToken(id) (((id) & PK11_TOKEN_MASK) == PK11_TOKEN_MAGIC)

/* queueing helper macros */
#define pk11_hash(value,size) ((value) & (size-1))/*size must be a power of 2*/
#define pk11queue_add(element,id,head,hash_size) \
	{ int tmp = pk11_hash(id,hash_size); \
	(element)->next = (head)[tmp]; \
	(element)->prev = NULL; \
	if ((head)[tmp]) (head)[tmp]->prev = (element); \
	(head)[tmp] = (element); }
#define pk11queue_find(element,id,head,hash_size) \
	for( (element) = (head)[pk11_hash(id,hash_size)]; (element) != NULL; \
					 (element) = (element)->next) { \
	    if ((element)->handle == (id)) { break; } }
#define pk11queue_delete(element,id,head,hash_size) \
	if ((element)->next) (element)->next->prev = (element)->prev; \
	if ((element)->prev) (element)->prev->next = (element)->next; \
	   else (head)[pk11_hash(id,hash_size)] = ((element)->next); \
	(element)->next = NULL; \
	(element)->prev = NULL; \

/* expand an attribute & secitem structures out */
#define pk11_attr_expand(ap) (ap)->type,(ap)->pValue,(ap)->ulValueLen
#define pk11_item_expand(ip) (ip)->data,(ip)->len

#endif
	
