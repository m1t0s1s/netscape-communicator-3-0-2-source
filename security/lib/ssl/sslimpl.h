#ifndef __sslimpl_h_
#define __sslimpl_h_

/*
 * This file is PRIVATE to SSL and should be the first thing included by
 * any SSL implementation file.
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: sslimpl.h,v 1.65.2.4 1997/05/24 00:24:39 jwz Exp $
 */

#ifdef DEBUG
#undef NDEBUG
#else
#undef NDEBUG
#define NDEBUG
#endif
#include "xp_trace.h"
#include "xp_sock.h"
#include "ssl3prot.h"
#include "hasht.h"
#include "pkcs11t.h"

#ifdef FORTEZZA
/* sigh */
typedef struct FortezzaKeyStr FortezzaKey;
#endif

#ifndef NSPR20
#if defined(__sun)
# include "sunos4.h"
#endif /* __sun */
#endif /* NSPR20 */

#if defined(DEBUG) || defined(TRACE)
#define Debug 1
#else
#undef Debug
#endif
#if defined(DEBUG) && defined(XP_MAC)
#define TRACE
#endif

#ifdef TRACE
#define SSL_TRC(a,b) if (ssl_trace >= (a)) XP_TRACE(b)
#define PRINT_BUF(a,b) if (ssl_trace >= (a)) ssl_PrintBuf b
#define DUMP_MSG(a,b) if (ssl_trace >= (a)) ssl_DumpMsg b
#else
#define SSL_TRC(a,b)
#define PRINT_BUF(a,b)
#define DUMP_MSG(a,b)
#endif

#ifdef DEBUG
#define SSL_DBG(b) if (ssl_debug) XP_TRACE(b)
#ifndef XP_MAC
#define FAILURE abort()
#else
#define FAILURE assert(false)
#endif
#else
#define SSL_DBG(b)
#define FAILURE goto error
#endif

#define LSB(x) ((unsigned char) (x & 0xff))
#define MSB(x) ((unsigned char) (((unsigned)(x)) >> 8))

/************************************************************************/

typedef enum { SSLAppOpRead = 0,
	       SSLAppOpWrite,
	       SSLAppOpRDWR,
	       SSLAppOpPost,
	       SSLAppOpHeader
} SSLAppOperation;

#define SSL_MIN_MASTER_KEY_BYTES	5
#define SSL_MAX_MASTER_KEY_BYTES	64

#define SSL_SESSIONID_BYTES		16
#define SSL3_SESSIONID_BYTES		32

#define SSL_MIN_CHALLENGE_BYTES		16
#define SSL_MAX_CHALLENGE_BYTES		32
#define SSL_CHALLENGE_BYTES		16

#define SSL_CONNECTIONID_BYTES		16

#define SSL_MIN_CYPHER_ARG_BYTES	0
#define SSL_MAX_CYPHER_ARG_BYTES	32

#define SSL_MAX_MAC_BYTES		16

/* This makes the cert cache entry exactly 4k. */
#define SSL_MAX_CACHED_CERT_LEN		4060

typedef struct SSLBufferStr SSLBuffer;
typedef struct SSLConnectInfoStr SSLConnectInfo;
typedef struct SSLGatherStr SSLGather;
typedef struct SSLSecurityInfoStr SSLSecurityInfo;
typedef struct SSLSessionIDStr SSLSessionID;
typedef struct SSLSocketStr SSLSocket;
typedef struct SSLSocketOpsStr SSLSocketOps;
typedef struct SSLSocksInfoStr SSLSocksInfo;

typedef struct SSL3StateStr SSL3State;
typedef struct SSL3CertNodeStr SSl3CertNode;

typedef struct SSL3BulkCipherDefStr SSL3BulkCipherDef;
typedef struct SSL3MACDefStr SSL3MACDef;

struct SSL3CertNodeStr {
    struct SSL3CertNodeStr *next;
    CERTCertificate *cert;
};

typedef int (*SSLHandshakeProc)(SSLSocket *ss);
typedef int (*SSLSendProc)(SSLSocket *ss, const void *buf, int n, int flags);

typedef void (*SSLSessionIDCacheFunc)(SSLSessionID *sid);
typedef void (*SSLSessionIDUncacheFunc)(SSLSessionID *sid);
typedef SSLSessionID *(*SSLSessionIDLookupFunc)(unsigned long addr,
						unsigned char* sid,
						unsigned sidLen);

/* used for maintaining a list of sockets */
typedef struct SECSocketNode {
    SSLSocket *ss;
    struct SECSocketNode *next;
} SECSocketNode;
	
/* Socket ops */
struct SSLSocketOpsStr {
    int (*connect)(SSLSocket*, const void *, int);
    int (*accept)(SSLSocket*, void *, int *);
    int (*bind)(SSLSocket*, const void *, int);
    int (*listen)(SSLSocket*, int);
    int (*shutdown)(SSLSocket*, int);
    int (*ioctl)(SSLSocket*, int, void*);
    int (*close)(SSLSocket*);

    int (*recv)(SSLSocket*, void *, int, int);
    int (*send)(SSLSocket*, const void *, int, int);
    int (*read)(SSLSocket*, void *, int);
    int (*write)(SSLSocket*, const void *, int);

    int (*getpeername)(SSLSocket*, void *, int *);
    int (*getsockname)(SSLSocket*, void *, int *);
    int (*getsockopt)(SSLSocket*, int, int, void *, int *);
    int (*setsockopt)(SSLSocket*, int, int, const void *, int);
    int (*importfd)(SSLSocket*, int);
#ifdef XP_UNIX
    int (*dup2)(SSLSocket*, int);
#endif
};

/*
** A buffer object.
*/
struct SSLBufferStr {
    unsigned char *buf;
    int len;
    int space;
};

/*
** SSL Socket struct
*/
struct SSLSocketStr {
    SSLSocket *next;

    /* Pointer to operations vector for this socket */
    SSLSocketOps *ops;

    /* Actual underlying file descriptor */
    XP_SOCKET fd;

    /* State flags */
    unsigned char useSocks;
    unsigned char useSecurity;
    unsigned char requestCertificate;
    unsigned char requestPassword;/* XXX remove me*/
    unsigned char connected;
    unsigned char asyncWrites;
    unsigned char delayedHandshake;
    unsigned char handshakeAsClient;
    unsigned char handshakeAsServer;
    unsigned char enableSSL2;
    unsigned char enableSSL3;
    unsigned char clientAuthRequested;
    unsigned char noCache;

    /* state to avoid freeing when a dialog is up */
    PRBool beenFreed;  /* the owner has attempted to free the socket */
    PRBool dialogPending; /* there is a dialog with a pointer to this struct */

    /* version of the protocol to use */

    uint16 version;

    /* Non-zero if socks is enabled */
    SSLSocksInfo *socks;

    /* Non-zero if security is enabled */
    SSLSecurityInfo *sec;

    /* Gather object used for gathering data */
    SSLGather *gather;

    SSLHandshakeProc handshake;
    SSLHandshakeProc nextHandshake;
    SSLHandshakeProc securityHandshake;

    SSLBuffer saveBuf;
    SSLBuffer pendingBuf;
    long peer;
    int port;
    char *peerID;
#ifdef FORTEZZA
    /* sigh, we need to restart hello, but there are two different hello's
     * we may be using. (SSL2 or SSL3). We could be using either, even if
     * our version is set to ssl3!. So we record which on the way out
     * so we can restart the correct version!
     */
    int	reStartType;
#endif

    SSL3State *ssl3;
};

/*
** A gather object. Used to read some data until a count has been
** satisfied. Primarily for support of async sockets.
*/
struct SSLGatherStr {
    int state;
    int encrypted;

    SSLBuffer buf;
    unsigned char hdr[5];

    int offset;
    int remainder;
    int count;

    int recordLen;
    int recordPadding;
    int recordOffset;

    /* Spot where record reader will read next */
    int readOffset;

    /* Spot where record writer will write next */
    int writeOffset;

    /* Buffer for ssl3 to read data from the socket */
    SSLBuffer inbuf;
};

/* SSLGather.state */
#define GS_INIT		0
#define GS_HEADER	1
#define GS_MAC		2
#define GS_DATA		3
#define GS_PAD		4

struct SSLSocksInfoStr {
    int direct;
    int didBind;

    unsigned long sockdHost;
    unsigned short sockdPort;
    int (*handshake)(SSLSocket*);
    struct sockaddr_in bindAddr;

    /* Data returned by sockd */
    unsigned long destHost;
    unsigned short destPort;
};

typedef SECStatus (*SSLCipher)(void *, unsigned char *, unsigned *, unsigned,
			      unsigned char *, unsigned);
typedef SECStatus (*SSLDestroy)(void *, PRBool);


#ifdef FORTEZZA
/* uses void* for FortezzaCardInfo * to prevent having to include
 * fortezza.h everywhere...
 */
typedef SECStatus (*FortezzaCardSelect) (void *arg, int fd, void *info, 
						int cardmask,int *psockId);
typedef SECStatus (*FortezzaGetPin) (void *arg,void *psock);
typedef SECStatus (*FortezzaCertificateSelect) (void *arg,int fd,char **string, 
						   int certCount,int *pcert);

int Fortezza_CardSelectHook(int fd,FortezzaCardSelect func,void *arg);
int Fortezza_GetPinHook(int fd,FortezzaGetPin func,void *arg);
int Fortezza_CertificateSelectHook(int fd,FortezzaCertificateSelect func,void *arg);
int Fortezza_AlertHook(int fd,FortezzaAlert func,void *arg);

SECStatus SSL_RestartHandshakeAfterFortezza(SSLSocket *ss);
#endif

struct SSLSecurityInfoStr {
    SSLSendProc send;

    int isServer;

    SSLBuffer writeBuf;

    uint32 sendSequence;
    uint32 rcvSequence;

    /* Hash information; used for one-way-hash functions (MD2, MD5, etc.) */
    SECHashObject *hash;
    void *hashcx;

    SECItem sendSecret;
    SECItem rcvSecret;

    int cipherType;
    int keyBits;
    int secretKeyBits;
    CERTCertificate *peerCert;
    SECKEYPublicKey *peerKey;	/* used when server is anonymous or must
				   use shorter key length because of export
				   restrictions */
    char *url;
    SSLAppOperation app_operation; /* what type of operation is the app doing*/

    /* Session cypher contexts; one for each direction */
    void *readcx;
    void *writecx;
    SSLCipher enc, dec;
    void (*destroy)(void *, PRBool);

    /* Blocking information for the session cypher */
    int blockShift;
    int blockSize;

    /*
    ** Procs used for nonce management. Different implementations exist
    ** for clients/servers because servers are assumed to be
    ** multi-threaded and require nonce synchronization. The lookup proc
    ** is only used for servers.
    */
    SSLSessionIDCacheFunc cache;
    SSLSessionIDUncacheFunc uncache;

    /* These are used during a connection handshake */
    SSLConnectInfo *ci;

    SSLAuthCertificate authCertificate;
    void *authCertificateArg;
    SSLGetClientAuthData getClientAuthData;
    void *getClientAuthDataArg;
    SSLBadCertHandler handleBadCert;
    void *badCertArg;
    SSLHandshakeCallback handshakeCallback;
    void *handshakeCallbackData;
#ifdef FORTEZZA
    FortezzaCardSelect fortezzaCardSelect;
    void *fortezzaCardArg;
    FortezzaGetPin fortezzaGetPin;
    void *fortezzaPinArg;
    FortezzaCertificateSelect fortezzaCertificateSelect;
    void *fortezzaCertificateArg;
    FortezzaAlert fortezzaAlert;
    void *fortezzaAlertArg;
#endif
    PRBool post_ok; /* after post warning, user said it was ok */
};

struct SSLConnectInfoStr {
    SSLBuffer sendBuf;

    unsigned long peer;
    unsigned short port;
    SSLSessionID *sid;

    char elements;
    char requiredElements;
    char sentElements;
    char sentFinished;

    /* Length of server challenge.  Used by client when saving challenge */
    int serverChallengeLen;
    /* type of authentication requested by server */
    unsigned char authType;
    
    /* Challenge sent by client to server in client-hello message */
    unsigned char clientChallenge[SSL_MAX_CHALLENGE_BYTES];

    /* Connection-id sent by server to client in server-hello message */
    unsigned char connectionID[SSL_CONNECTIONID_BYTES];

    /* Challenge sent by server to client in request-certificate message */
    unsigned char serverChallenge[SSL_MAX_CHALLENGE_BYTES];

    /* Information kept to handle a request-certificate message */
    unsigned char readKey[SSL_MAX_MASTER_KEY_BYTES];
    unsigned char writeKey[SSL_MAX_MASTER_KEY_BYTES];
    unsigned keySize;
};

#define CIS_HAVE_MASTER_KEY		0x01
#define CIS_HAVE_CERTIFICATE		0x02
#define CIS_HAVE_FINISHED		0x04
#define CIS_HAVE_VERIFY			0x08

/*
** SSL3State and CipherSpec structs
*/

/* The SSL bulk cipher definition */
typedef enum {
    cipher_null,
    cipher_rc4, cipher_rc4_40,
    cipher_rc2, cipher_rc2_40,
    cipher_des, cipher_3des, cipher_des40,
    cipher_idea, cipher_fortezza,
    cipher_missing              /* reserved for no such supported cipher */
} SSL3BulkCipher;

/* The specific cipher algorithm */
#ifndef XP_WIN16
typedef enum {
    calg_null = 0x8000, calg_rc4 = CKM_RC4, calg_rc2 = CKM_RC2_CBC,
    calg_des = CKM_DES_CBC, calg_3des = CKM_DES_CBC, calg_idea = CKM_IDEA_CBC,
    calg_fortezza = CKM_SKIPJACK_CBC64
} CipherAlgorithm;
#else
#define calg_null  0x80000000L
#define calg_rc4   CKM_RC4
#define calg_rc2   CKM_RC2_CBC
#define calg_des   CKM_DES_CBC
#define calg_3des   CKM_DES_CBC
#define calg_idea   CKM_IDEA_CBC
#define calg_fortezza   CKM_SKIPJACK_CBC64
typedef unsigned long CipherAlgorithm;
#endif

typedef enum { mac_null, mac_md5, mac_sha } MACAlgorithm;
typedef enum { type_stream, type_block } CipherType;

#define MAX_IV_LENGTH 64

/*
 * Do not depend upon 64 bit arithmetic in the underlying machine. Since
 * we simply add 1 from time to time, this straightforward implementation
 * will be more efficient for most current 32-bit architectures.
 */
typedef struct {
    uint32 high;
    uint32 low;
} SSL3SequenceNumber;

typedef struct {
    SSL3Opaque write_iv[MAX_IV_LENGTH];
    SSL3Opaque write_key[MAX_KEY_LENGTH];
    SSL3Opaque write_mac_secret[MAX_MAC_LENGTH];
} SSL3KeyMaterial;

typedef struct {
    const SSL3BulkCipherDef *cipher_def;
    const SSL3MACDef *mac_def;
    int mac_size;
    SSLCipher encode;
    void *encodeContext;
    SSLCipher decode;
    void *decodeContext;
    SSLDestroy destroy;
    SECHashObject *hash;
    void *hashContext;
    SSL3MasterSecret master_secret;
    SSL3KeyMaterial client;
    SSL3KeyMaterial server;
    SSL3SequenceNumber write_seq_num;
    SSL3SequenceNumber read_seq_num;
} SSL3CipherSpec;

typedef enum {never_cached, in_cache, invalid_cache} Cached;

struct SSLSessionIDStr {
    SSLSessionID *next;

    unsigned short port;
    unsigned long addr;
    char *peerID; /* client only */
    CERTCertificate *peerCert;

    uint16 version;

    time_t time;
    Cached cached;
    int references;

    union {
	struct {
	    /*
	     * the V2 code depends upon the size of sessionID.
	     */
	    unsigned char sessionID[SSL_SESSIONID_BYTES];

	    /* Stuff used to recreate key and read/write cipher objects */
	    SECItem masterKey;
	    int cipherType;
	    SECItem cipherArg;

	    int keyBits;
	    int secretKeyBits;
	} ssl2;
	struct {
	    uint8 sessionIDLength;
	    unsigned char sessionID[SSL3_SESSIONID_BYTES];
	    SSL3MasterSecret masterSecret;
	    SSL3CipherSuite cipherSuite;
	    SSL3CompressionMethod compression;
	    PRBool resumable;
	    int policy;
#ifdef FORTEZZA
	    PRBool hasFortezza;
 	    FortezzaKey *clientWriteKey;
	    FortezzaKey *serverWriteKey;
	    FortezzaKey *tek;
	    /* sigh... */
	    unsigned char clientWriteIV[24];
	    unsigned char serverWriteIV[24];
	    unsigned char clientWriteSave[28];
	    int fortezzaSocket;
#endif
	} ssl3;
    } u;
};

typedef struct {
    SSL3CipherSuite cipher_suite;
    int policy;
    PRBool enabled;
} SSL3CipherSuiteCfg;

typedef struct {
    SSL3CipherSuite cipher_suite;
    SSL3BulkCipher bulk_cipher_algorithm;
    MACAlgorithm mac_algorithm;
    SSL3KeyExchangeAlgorithm key_exchange_algorithm;
} SSL3CipherSuiteDef;

typedef struct {
    SSL3KeyExchangeAlgorithm kea;
    SSL3KEAType alg;
    SSL3SignType sign;
    PRBool is_limited;
    int key_size_limit;
} SSL3KEADef;

typedef enum { kg_null, kg_strong, kg_export } SSL3KeyGenMode;

struct SSL3BulkCipherDefStr {
    SSL3BulkCipher cipher;
    CipherAlgorithm alg;
    int key_size;
    int secret_key_size;
    CipherType type;
    int iv_size;
    int block_size;
    SSL3KeyGenMode keygen_mode;
};

struct SSL3MACDefStr {
    MACAlgorithm alg;
    int pad_size;
};

typedef enum {
    wait_client_hello, wait_client_cert, wait_client_key,
    wait_cert_verify, wait_change_cipher, wait_finished,
    wait_server_hello, wait_server_cert, wait_server_key,
    wait_cert_request, wait_hello_done,
    idle_handshake
} SSL3WaitState;

typedef struct {
    SSL3Random server_random;
    SSL3Random client_random;
    SSL3WaitState ws;
    MD5Context *md5;            /* handshake running hashes */
    SHA1Context *sha;
    const SSL3KEADef *kea_def;
    SSL3CipherSuite cipher_suite;
    const SSL3CipherSuiteDef *suite_def;
    SSL3CompressionMethod compression;
    SSLBuffer msg_body;      /* partial handshake message from record layer */
    unsigned int header_bytes; /* number of bytes consumed from handshake */
                               /* message for message type and header length */
    SSL3HandshakeType msg_type;
    unsigned long msg_len;
    SECItem ca_list;            /* used only by client */
    PRBool isResuming;          /* are we resuming a session */
    PRBool rehandshake;		/* immediately start another handshake when
				 * this one finishes */
    SSLBuffer msgState;         /* current state for handshake messages */
} SSL3HandshakeState;

struct SSL3StateStr {
    SSL3CipherSpec specs[2];
    SSL3HandshakeState hs;
    CERTCertificate *clientCertificate; /* used by client */
    SECKEYPrivateKey *clientPrivateKey;   /* used by client */
    CERTCertificateList *clientCertChain; /* used by client */
    SSL3CipherSpec *current_read;      /* points to one of state.specs[0..1] */
    SSL3CipherSpec *pending_read;
    SSL3CipherSpec *current_write;
    SSL3CipherSpec *pending_write;
    int policy; /* this says what cipher suites we can do, and should be
		 * either SSL_ALLOWED or SSL_RESTRICTED */

    PRArenaPool *peerCertArena;  /* These are used to keep track of the peer CA */
    void *peerCertChain;     /* chain while we are trying to validate it.   */
    CERTDistNames *ca_list; /* used by server.  trusted CAs for this socket. */
#ifdef FORTEZZA
    void *kea_context;
#endif
};

typedef struct {
    SSL3ContentType type;
    SSL3ProtocolVersion version;
    SSLBuffer *buf;
} SSL3Ciphertext;

/*
 * buffers piggy-backed from SSL 2 implementation
 *     writeBuf in the SecurityInfo maintained by sslsecur.c is used
 *              to hold the data just about to be passed to the kernel
 *     sendBuf in the ConnectInfo maintained by sslcon.c is used
 *              to hold handshake messages as they are accumulated
 */

extern char ssl_debug, ssl_trace;
extern SSLSocketOps ssl_default_ops;
extern SSLSocketOps ssl_socks_ops;
extern SSLSocketOps ssl_secure_ops;
extern SSLSocketOps ssl_secure_socks_ops;
extern SECKEYPrivateKey *ssl_server_key;
extern SECItem ssl_server_ca_list;
#ifdef FORTEZZA
extern SECItem ssl_fortezza_server_ca_list;
#endif
extern SECItem ssl_server_signed_certificate;
extern CERTCertificateList ssl_issuer_list;
extern SSLSessionIDLookupFunc ssl_sid_lookup;
extern SSLSessionIDCacheFunc ssl_sid_cache;
extern SSLSessionIDUncacheFunc ssl_sid_uncache;
extern time_t ssl_sid_timeout;
extern time_t ssl3_sid_timeout;
extern SSLAcceptFunc ssl_accept_func;
extern char *ssl_cipherName[];
extern char *ssl3_cipherName[];

/************************************************************************/

SEC_BEGIN_PROTOS

/* Implementation of ops for default (non socks, non secure) case */
extern int ssl_DefConnect(SSLSocket*, const void *, int);
extern int ssl_DefAccept(SSLSocket*, void *, int*);
extern int ssl_DefBind(SSLSocket*, const void *, int);
extern int ssl_DefListen(SSLSocket*, int);
extern int ssl_DefShutdown(SSLSocket*, int);
extern int ssl_DefIoctl(SSLSocket*, int, void*);
extern int ssl_DefClose(SSLSocket*);
extern int ssl_DefRecv(SSLSocket*, void *, int, int);
extern int ssl_DefSend(SSLSocket*, const void *, int, int);
extern int ssl_DefRead(SSLSocket*, void *, int);
extern int ssl_DefWrite(SSLSocket*, const void *, int);
extern int ssl_DefGetpeername(SSLSocket*, void *, int *);
extern int ssl_DefGetsockname(SSLSocket*, void *, int *);
extern int ssl_DefGetsockopt(SSLSocket*, int, int, void *, int *);
extern int ssl_DefSetsockopt(SSLSocket*, int, int, const void *, int);
extern int ssl_DefDup2(SSLSocket*, int);
extern int ssl_DefImportFd(SSLSocket*, int);

/* Implementation of ops for socks only case */
extern int ssl_SocksConnect(SSLSocket*, const void *, int);
extern int ssl_SocksAccept(SSLSocket*, void *, int*);
extern int ssl_SocksBind(SSLSocket*, const void *, int);
extern int ssl_SocksListen(SSLSocket*, int);
extern int ssl_SocksGetsockname(SSLSocket*, void *, int *);
extern int ssl_SocksRecv(SSLSocket*, void *, int, int);
extern int ssl_SocksSend(SSLSocket*, const void *, int, int);
extern int ssl_SocksRead(SSLSocket*, void *, int);
extern int ssl_SocksWrite(SSLSocket*, const void *, int);

/* Implementation of ops for secure only case */
extern int ssl_SecureConnect(SSLSocket*, const void *, int);
extern int ssl_SecureAccept(SSLSocket*, void *, int*);
extern int ssl_SecureRecv(SSLSocket*, void *, int, int);
extern int ssl_SecureSend(SSLSocket*, const void *, int, int);
extern int ssl_SecureRead(SSLSocket*, void *, int);
extern int ssl_SecureWrite(SSLSocket*, const void *, int);
extern int ssl_SecureImportFd(SSLSocket*, int);
extern int ssl_SecureClose(SSLSocket *ss);

/* Implementation of ops for secure socks case */
extern int ssl_SecureSocksConnect(SSLSocket*, const void *, int);
extern int ssl_SecureSocksAccept(SSLSocket*, void *, int*);

extern SSLSocket *ssl_FindSocket(int fd);
extern SSLGather *ssl_NewGather(void);
extern void ssl_DestroyGather(SSLGather *gs);
extern int ssl_GatherData(SSLSocket *ss, SSLGather *gs, int flags);
extern int ssl_GatherRecord(SSLSocket *ss, int flags);
extern int ssl_StartGatherBytes(SSLSocket *ss, SSLGather *gs, int count);

extern int ssl_CreateSecurityInfo(SSLSocket *ss);
extern int ssl_CopySecurityInfo(SSLSocket *ss, SSLSocket *os);
extern void ssl_DestroySecurityInfo(SSLSecurityInfo *sec);

extern int ssl_CreateSocksInfo(SSLSocket *ss);
extern int ssl_CopySocksInfo(SSLSocket *ss, SSLSocket *os);
extern void ssl_DestroySocksInfo(SSLSocksInfo *si);

extern SSLSocket *ssl_NewSocket(int fd);
extern SSLSocket *ssl_DupSocket(SSLSocket *old, int newfd);

extern void ssl_PrintBuf(SSLSocket *ss, char *msg, unsigned char *cp, int len);
extern void ssl_DumpMsg(SSLSocket *ss, unsigned char *bp, unsigned len);

extern int ssl_SendSavedWriteData(SSLSocket *ss, SSLBuffer *buf,
				  SSLSendProc fp);
extern int ssl_SaveWriteData(SSLSocket *ss, SSLBuffer *buf, const void* p,
			     int l);
extern int ssl_BeginClientHandshake(SSLSocket *ss);
extern int ssl_BeginServerHandshake(SSLSocket *ss);
extern int ssl_ReadHandshake(SSLSocket *ss);
extern int ssl_WriteHandshake(SSLSocket *ss, const void *buf, int len);

extern void ssl_DestroyConnectInfo(SSLSecurityInfo *sec);

extern int ssl_GrowBuf(SSLBuffer *b, int newLen);

extern void ssl_ChooseProcs(SSLSocket *ss);
extern void ssl_ChooseSessionIDProcs(SSLSecurityInfo *sec);

extern SSLSessionID *ssl_LookupSID(unsigned long addr, unsigned short port,
				   char *peerID);
extern void ssl_FreeSID(SSLSessionID *sid);

extern int ssl_UnderlyingAccept(int fd, struct sockaddr *a, int *ap);

extern int SSL_RestartHandshakeAfterServerCert(struct SSLSocketStr *ss);
extern int SSL_RestartHandshakeAfterCertReq(struct SSLSocketStr *ss,
					    CERTCertificate *cert,
					    SECKEYPrivateKey *key,
					    CERTCertificateList *certChain);
extern int SSL_SendErrorMessage(struct SSLSocketStr *ss, int error);
extern void ssl_FreeSocket(struct SSLSocketStr *ssl);

/*
 * for dealing with SSL 3.0 clients sending SSL 2.0 format hellos
 */
extern SECStatus SSL3_HandleV2ClientHello(
    SSLSocket *ss, unsigned char *buffer, int length);
extern SECStatus ssl_HandleV3Hello(struct SSLSocketStr *ss);
extern SECStatus ssl3_StartHandshakeHash(
    SSLSocket *ss, unsigned char *buf, int length);

/*
 * SSL3 specific routines
 */
SECStatus SSL3_SendClientHello(SSLSocket *ss);

/*
 * input into the SSL3 machinery from the actualy network reading code
 */
SECStatus SSL3_HandleRecord(
    SSLSocket *ss, SSL3Ciphertext *cipher, SSLBuffer *out);

int ssl3_GatherRecord(SSLSocket *ss, int flags);
int ssl3_GatherHandshake(SSLSocket *ss, int flags);
/*
 * US severs must use a self-signed, smaller key when talking to export
 * clients.  Generate that key pair and keep it around.
 */
extern void SSL3_CreateExportRSAKeys(SECKEYPrivateKey *key);
#ifdef FORTEZZA
void SSL3_SetFortezzaKeys(SECKEYPrivateKey *server_key);
#endif


extern SECStatus SSL3_SendAlert(
    SSLSocket *ss, SSL3AlertLevel level, SSL3AlertDescription desc);

extern SECStatus SSL3_EnableCipher(SSL3CipherSuite which, int enabled);
extern SECStatus SSL3_SetPolicy(SSL3CipherSuite which, int policy);

extern SECStatus SSL3_ConstructV2CipherSpecsHack(SSLSocket *ss,
						 unsigned char *cs, int *size);

extern SECStatus SSL3_RedoHandshake(SSLSocket *ss);

/* This is used to delete the CA certificates in the peer certificate chain
 * from the cert database after they've been validated.
 */
extern void SSL3_CleanupPeerCerts(SSL3State *ssl3);

extern void ssl3_DestroySSL3Info(SSL3State *ssl3);

extern int ssl_GetPeerInfo(SSLSocket *ss);

/*
 * initialize the lock that guards the hashing tables that
 * map an FDs to am SSLSocket*
 */
extern void SSL_InitHashLock(void);

SEC_END_PROTOS

extern char *ssl3_cipherName[];

extern CERTCertificateList *ssl3_server_cert_chain;
extern CERTDistNames *ssl3_server_ca_list;
#ifdef FORTEZZA
extern CERTDistNames *ssl3_fortezza_server_ca_list;
extern CERTCertificateList *ssl3_fortezza_server_cert_chain;
#endif

#ifdef XP_UNIX
#define SSL_GETPID() getpid()
#else
#define SSL_GETPID() 0
#endif

#endif /* __sslimpl_h_ */
