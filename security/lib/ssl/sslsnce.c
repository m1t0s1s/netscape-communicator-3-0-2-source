/* XXX rename this file */

/*
 * NOTE:  The contents of this file are NOT used by the client.
 * (They are part of the security library as a whole, but they are
 * NOT USED BY THE CLIENT.)  Do not change things on behalf of the
 * client (like localizing strings), or add things that are only
 * for the client (put them elsewhere).  Clearly we need a better
 * way to organize things, but for now this will have to do.
 */

#if defined(XP_UNIX) || defined(XP_WIN32)
#ifndef NADA_VERISON

#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"

#ifdef XP_UNIX
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#else /* XP_WIN32 */
#ifdef MC_HTTPD
#include <ereport.h>
#endif /* MC_HTTPD */
#include <wtypes.h>
#endif /* XP_WIN32 */
#include <sys/types.h>

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
#include "nspr/prmon.h"
static PRMonitor *cacheLock;
#endif

/*
** The server session-id cache uses a simple flat cache. The cache is
** sized during initialization. We hash the ip-address + session-id value
** into an index into the cache and do the lookup. No buckets, nothing
** fancy.
*/

#ifdef XP_UNIX
static int SIDCacheFD = -1;
static int certCacheFD = -1;
#else /* XP_WIN32 */
static HANDLE SIDCacheFD = INVALID_HANDLE_VALUE;
static HANDLE SIDCacheFDMAP = INVALID_HANDLE_VALUE;
static char *SIDCacheData = NULL;
static HANDLE SIDCacheSem = INVALID_HANDLE_VALUE;
static int SIDCacheOffset = 0;
static HANDLE certCacheFD = INVALID_HANDLE_VALUE;
static HANDLE certCacheFDMAP = INVALID_HANDLE_VALUE;
static char *certCacheData = NULL;
static HANDLE certCacheSem = INVALID_HANDLE_VALUE;
static int certCacheOffset = 0;
#endif /* XP_WIN32 */

static int numSIDCacheEntries = 10000;
static int numCertCacheEntries = 250;

#ifdef XP_UNIX
#define DEFAULT_CACHE_DIRECTORY "/tmp"
#else /* XP_WIN32 */
#define DEFAULT_CACHE_DIRECTORY "\\temp"
#endif /* XP_UNIX */

/*
** Format of a cache entry.
*/ 
typedef struct SIDCacheEntryStr SIDCacheEntry;
struct SIDCacheEntryStr {
    union {
	struct {
	    /* This is gross.  We have to have version and valid in both arms
	     * of the union for alignment reasons.  This probably won't work
	     * on a 64-bit machine. XXXX
	     */
	    uint16 version;
	    unsigned char valid;
	    unsigned char cipherType;
	    unsigned char sessionID[SSL_SESSIONID_BYTES];
	    unsigned char masterKey[SSL_MAX_MASTER_KEY_BYTES];
	    unsigned char cipherArg[SSL_MAX_CYPHER_ARG_BYTES];

	    unsigned char masterKeyLen;
	    unsigned char keyBits;

	    unsigned char secretKeyBits;
	    unsigned char cipherArgLen;
	} ssl2;
	struct {
	    uint16 version;
	    unsigned char valid;
	    uint8 sessionIDLength;
	    unsigned char sessionID[SSL3_SESSIONID_BYTES];
	    SSL3CipherSuite cipherSuite;
	    SSL3CompressionMethod compression;
	    SSL3MasterSecret masterSecret;
	    int16 certIndex;
	    PRBool resumable;
	} ssl3;
    } u;

    unsigned int addr : 32;
    unsigned int time : 32;

};


typedef struct CertCacheEntryStr CertCacheEntry;
struct CertCacheEntryStr {
    uint16 certLength;
    uint8 sessionIDLength;
    unsigned char sessionID[SSL3_SESSIONID_BYTES];
    unsigned char cert[SSL_MAX_CACHED_CERT_LEN];
};


static void IOError(int rv, char *type);
static unsigned long Offset(unsigned long addr, unsigned char *s, unsigned nl);
static void Invalidate(SIDCacheEntry *ce);


/************************************************************************/

/*
** Reconstitute a cert from the cache
*/
static CERTCertificate *GetCertFromCache(SIDCacheEntry *sid)
{
    unsigned long offset = sid->u.ssl3.certIndex * sizeof(CertCacheEntry);
    int rv;
    CertCacheEntry ce;
    CERTCertificate *cert;
    SECItem derCert;

#ifdef XP_UNIX
    lseek(certCacheFD, offset, SEEK_SET);
    rv = read(certCacheFD, &ce, sizeof(CertCacheEntry));
    if (rv != sizeof(CertCacheEntry)) {
	/* ACK! File is damaged! */
	IOError(rv, "read");
	return NULL;
    }
#else /* XP_WIN32 */
    /* Use memory mapped I/O and just memcpy() the data */
    certCacheOffset = offset;
    CopyMemory(&ce, &certCacheData[offset], sizeof(CertCacheEntry));
    rv = sizeof(CertCacheEntry);
#endif /* XP_WIN32 */

    /* See if the session ID matches with that in the sid cache. */
    if((ce.sessionIDLength != sid->u.ssl3.sessionIDLength) ||
       PORT_Memcmp(ce.sessionID, sid->u.ssl3.sessionID, ce.sessionIDLength)) {
	return NULL;
    }

    derCert.len = ce.certLength;
    derCert.data = ce.cert;

    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &derCert, NULL,
				  PR_FALSE, PR_TRUE);

    return cert;
}

/* Put a certificate in the cache.  We assume that the certIndex in
** sid is valid.
*/
static void
CacheCert(CERTCertificate *cert, SIDCacheEntry *sid)
{
    CertCacheEntry ce;
    int16 index = sid->u.ssl3.certIndex;
    unsigned long offset = index * sizeof(CertCacheEntry);
    int rv;
    
    if (cert->derCert.len > SSL_MAX_CACHED_CERT_LEN)
	return;
    
    ce.sessionIDLength = sid->u.ssl3.sessionIDLength;
    PORT_Memcpy(ce.sessionID, sid->u.ssl3.sessionID, ce.sessionIDLength);
    ce.certLength = cert->derCert.len;
    PORT_Memcpy(ce.cert, cert->derCert.data, ce.certLength);

#ifdef XP_UNIX
    lseek(certCacheFD, offset, SEEK_SET);
    rv = write(certCacheFD, &ce, sizeof(CertCacheEntry));
    if (rv != sizeof(CertCacheEntry)) {
	/* ACK! File is damaged! */
	IOError(rv, "cert-write");
	Invalidate(sid);
    }
#else /* WIN32 */
    /* Use memory mapped I/O and just memcpy() the data */
    certCacheOffset = offset;
    CopyMemory(&certCacheData[offset], &ce, sizeof(CertCacheEntry));
#endif /* XP_UNIX */
    return;
}

/*
** Convert memory based SID to file based one
*/
static void ConvertFromSID(SIDCacheEntry *to, SSLSessionID *from)
{
    to->u.ssl2.valid = 1;
    to->u.ssl2.version = from->version;
    to->addr = from->addr;
    to->time = from->time;

    if (from->version == SSL_LIBRARY_VERSION_2) {
	if ((from->u.ssl2.masterKey.len > SSL_MAX_MASTER_KEY_BYTES) ||
	    (from->u.ssl2.cipherArg.len > SSL_MAX_CYPHER_ARG_BYTES)) {
	    SSL_DBG(("%d: SSL: masterKeyLen=%d cipherArgLen=%d",
		     SSL_GETPID(), from->u.ssl2.masterKey.len,
		     from->u.ssl2.cipherArg.len));
	    to->u.ssl2.valid = 0;
	    return;
	}

	to->u.ssl2.cipherType = from->u.ssl2.cipherType;
	to->u.ssl2.masterKeyLen = from->u.ssl2.masterKey.len;
	to->u.ssl2.cipherArgLen = from->u.ssl2.cipherArg.len;
	to->u.ssl2.keyBits = from->u.ssl2.keyBits;
	to->u.ssl2.secretKeyBits = from->u.ssl2.secretKeyBits;
	PORT_Memcpy(to->u.ssl2.sessionID, from->u.ssl2.sessionID,
		  sizeof(to->u.ssl2.sessionID));
	PORT_Memcpy(to->u.ssl2.masterKey, from->u.ssl2.masterKey.data,
		  from->u.ssl2.masterKey.len);
	PORT_Memcpy(to->u.ssl2.cipherArg, from->u.ssl2.cipherArg.data,
		  from->u.ssl2.cipherArg.len);
#ifdef DEBUG
	PORT_Memset(to->u.ssl2.masterKey+from->u.ssl2.masterKey.len, 0,
		  sizeof(to->u.ssl2.masterKey) - from->u.ssl2.masterKey.len);
	PORT_Memset(to->u.ssl2.cipherArg+from->u.ssl2.cipherArg.len, 0,
		  sizeof(to->u.ssl2.cipherArg) - from->u.ssl2.cipherArg.len);
#endif
	SSL_TRC(8, ("%d: SSL: ConvertSID: masterKeyLen=%d cipherArgLen=%d "
		    "time=%d addr=0x%x cipherType=%d", SSL_GETPID(),
		    to->u.ssl2.masterKeyLen, to->u.ssl2.cipherArgLen,
		    to->time, to->addr, to->u.ssl2.cipherType));
    } else {
	to->u.ssl3.sessionIDLength = from->u.ssl3.sessionIDLength;
	to->u.ssl3.cipherSuite = from->u.ssl3.cipherSuite;
	to->u.ssl3.compression = from->u.ssl3.compression;
	to->u.ssl3.resumable = from->u.ssl3.resumable;
	PORT_Memcpy(to->u.ssl3.sessionID, from->u.ssl3.sessionID,
		  to->u.ssl3.sessionIDLength);
	PORT_Memcpy(to->u.ssl3.masterSecret, from->u.ssl3.masterSecret,
		  sizeof(to->u.ssl3.masterSecret));

	SSL_TRC(8, ("%d: SSL3: ConvertSID: time=%d addr=0x%x cipherSuite=%d",
		    SSL_GETPID(), to->time, to->addr, to->u.ssl3.cipherSuite));
    }
}

/*
** Convert file based cache-entry to memory based one
*/
static SSLSessionID *ConvertToSID(SIDCacheEntry *from)
{
    SSLSessionID *to;
    uint16 version = from->u.ssl2.version;

    to = (SSLSessionID*) PORT_ZAlloc(sizeof(SSLSessionID));
    if (!to) {
	return 0;
    }

    if (version == SSL_LIBRARY_VERSION_2) {
	/* This is an SSL v2 session */
	to->u.ssl2.masterKey.data =
	    (unsigned char*) PORT_Alloc(from->u.ssl2.masterKeyLen);
	if (!to->u.ssl2.masterKey.data) {
	    goto loser;
	}
	if (from->u.ssl2.cipherArgLen) {
	    to->u.ssl2.cipherArg.data = (unsigned char*)
		PORT_Alloc(from->u.ssl2.cipherArgLen);
	    if (!to->u.ssl2.cipherArg.data) {
		goto loser;
	    }
	    PORT_Memcpy(to->u.ssl2.cipherArg.data, from->u.ssl2.cipherArg,
		      from->u.ssl2.cipherArgLen);
	}

	to->u.ssl2.cipherType = from->u.ssl2.cipherType;
	to->u.ssl2.masterKey.len = from->u.ssl2.masterKeyLen;
	to->u.ssl2.cipherArg.len = from->u.ssl2.cipherArgLen;
	to->u.ssl2.keyBits = from->u.ssl2.keyBits;
	to->u.ssl2.secretKeyBits = from->u.ssl2.secretKeyBits;
	PORT_Memcpy(to->u.ssl2.sessionID, from->u.ssl2.sessionID,
		  sizeof(from->u.ssl2.sessionID));
	PORT_Memcpy(to->u.ssl2.masterKey.data, from->u.ssl2.masterKey,
		  from->u.ssl2.masterKeyLen);

	SSL_TRC(8, ("%d: SSL: ConvertToSID: masterKeyLen=%d cipherArgLen=%d "
		    "time=%d addr=0x%x cipherType=%d",
		    SSL_GETPID(), to->u.ssl2.masterKey.len,
		    to->u.ssl2.cipherArg.len, to->time, to->addr,
		    to->u.ssl2.cipherType));
    } else {
	/* This is an SSL v3 session */
	to->u.ssl3.sessionIDLength = from->u.ssl3.sessionIDLength;
	PORT_Memcpy(to->u.ssl3.sessionID, from->u.ssl3.sessionID,
		  to->u.ssl3.sessionIDLength);
	to->u.ssl3.cipherSuite = from->u.ssl3.cipherSuite;
	to->u.ssl3.compression = from->u.ssl3.compression;
	PORT_Memcpy(to->u.ssl3.masterSecret, from->u.ssl3.masterSecret,
		  sizeof(from->u.ssl3.masterSecret));
	to->u.ssl3.resumable = from->u.ssl3.resumable;

	if (from->u.ssl3.certIndex != -1) {
	    to->peerCert = GetCertFromCache(from);
	    if (to->peerCert == NULL)
		goto loser;
	}
    }

    to->version = from->u.ssl2.version;
    to->time = from->time;
    to->cached = in_cache;
    to->addr = from->addr;
    to->references = 1;
    
    return to;

  loser:
    Invalidate(from);
    if (to) {
	if (version == SSL_LIBRARY_VERSION_2) {
	    if (to->u.ssl2.masterKey.data)
		PORT_Free(to->u.ssl2.masterKey.data);
	    if (to->u.ssl2.cipherArg.data)
		PORT_Free(to->u.ssl2.cipherArg.data);
	}
	PORT_Free(to);
    }
    return NULL;
}

/* Invalidate a cache entry. */
static void Invalidate(SIDCacheEntry *ce)
{
    int rv;
    unsigned long offset;

    if (ce == NULL) return;

    if (ce->u.ssl2.version == SSL_LIBRARY_VERSION_2) {
	offset = Offset(ce->addr, ce->u.ssl2.sessionID,
			sizeof(ce->u.ssl2.sessionID)); 
    } else {
	offset = Offset(ce->addr, ce->u.ssl3.sessionID,
			ce->u.ssl3.sessionIDLength); 
    }
	
    ce->u.ssl2.valid = 0;
#ifdef XP_UNIX
    lseek(SIDCacheFD, -sizeof(ce), SEEK_CUR);
    rv = write(SIDCacheFD, ce, sizeof(SIDCacheEntry));
    if (rv != sizeof(SIDCacheEntry)) {
	/* ACK! File is damaged! */
	IOError(rv, "timeout-write");
    }
#else /* WIN32 */
    /* Use memory mapped I/O and just memcpy() the data */
    SIDCacheOffset = offset;
    CopyMemory(&SIDCacheData[offset], ce, sizeof(SIDCacheEntry));
#endif /* XP_UNIX */
}


static void IOError(int rv, char *type)
{
#ifdef XP_UNIX
    syslog(LOG_ALERT,
	   "SSL: %s error with session-id cache, pid=%d, rv=%d, error='%m'",
	   type, SSL_GETPID(), rv);
#else /* XP_WIN32 */
#ifdef MC_HTTPD 
	ereport(LOG_FAILURE, "%s error with session-id cache rv=%d\n",type, rv);
#endif /* MC_HTTPD */
#endif /* XP_UNIX */
}

static void lock_cache(void)
{
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    PR_EnterMonitor(cacheLock);
#endif
}

static void unlock_cache(void)
{
#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    PR_ExitMonitor(cacheLock);
#endif
}

/*
** Perform some mumbo jumbo on the ip-address and the session-id value to
** compute a hash value.
*/
static unsigned long Offset(unsigned long addr, unsigned char *s, unsigned nl)
{
    unsigned long rv;

    rv = addr ^ (((unsigned long)s[0] << 24) + ((unsigned long)s[1] << 16)
		 + (s[2] << 8) + s[nl-1]);
    return (rv % ((unsigned long)numSIDCacheEntries)) * sizeof(SIDCacheEntry);
}

/*
** Look something up in the cache. This will invalidate old entries
** in the process. Caller has locked the cache!
*/
static PRBool FindSID(unsigned long addr, unsigned char *sessionID,
		      unsigned sessionIDLength, SIDCacheEntry *ce)
{
    unsigned long offset;
    time_t now;
    int rv;

    /* Read in cache entry after hashing ip address and session-id value */
    offset = Offset(addr, sessionID, sessionIDLength); 
    now = PORT_Time();
#ifdef XP_UNIX
    lseek(SIDCacheFD, offset, SEEK_SET);
    rv = read(SIDCacheFD, ce, sizeof(SIDCacheEntry));
    if (rv != sizeof(SIDCacheEntry)) {
	/* ACK! File is damaged! */
	IOError(rv, "read");
	return PR_FALSE;
    }
#else /* XP_WIN32 */
    /* Use memory mapped I/O and just memcpy() the data */
    SIDCacheOffset = offset;
    CopyMemory(ce, &SIDCacheData[offset], sizeof(SIDCacheEntry));
    rv = sizeof(SIDCacheEntry);
#endif /* XP_WIN32 */

    if (!ce->u.ssl2.valid) {
	/* Entry is not valid */
	return PR_FALSE;
    }

    if (((ce->u.ssl2.version == SSL_LIBRARY_VERSION_2) &&
	 (now > ce->time + ssl_sid_timeout)) ||
	((ce->u.ssl2.version == SSL_LIBRARY_VERSION_3_0) &&
	 (now > ce->time + ssl3_sid_timeout))) {
	/* SessionID has timed out. Invalidate the entry. */
	SSL_TRC(7, ("%d: timed out sid entry addr=%08x now=%x time+=%x",
		    SSL_GETPID(), ce->addr, now, ce->time + ssl_sid_timeout));
	ce->u.ssl2.valid = 0;
#ifdef XP_UNIX
	lseek(SIDCacheFD, offset, SEEK_SET);
	rv = write(SIDCacheFD, ce, sizeof(SIDCacheEntry));
	if (rv != sizeof(SIDCacheEntry)) {
	    /* ACK! File is damaged! */
	    IOError(rv, "timeout-write");
	}
#else /* WIN32 */
	/* Use memory mapped I/O and just memcpy() the data */
	SIDCacheOffset = offset;
	CopyMemory(&SIDCacheData[offset], ce, sizeof(SIDCacheEntry));
#endif /* XP_UNIX */
	return PR_FALSE;
    }

    /*
    ** Finally, examine specific session-id/addr data to see if the cache
    ** entry matches our addr+session-id value
    */
    if ((ce->addr == addr) &&
	(PORT_Memcmp(ce->u.ssl2.sessionID, sessionID, sessionIDLength) == 0)) {
	/* Found it */
	return PR_TRUE;
    }
    return PR_FALSE;
}

/************************************************************************/


static SSLSessionID *ServerSessionIDLookup(unsigned long addr,
						     unsigned char *sessionID,
						     unsigned sessionIDLength)
{
    SIDCacheEntry ce;
    SSLSessionID *sid;

    sid = 0;
    lock_cache();
    if (FindSID(addr, sessionID, sessionIDLength, &ce)) {
	/* Found it. Convert file format to internal format */
	sid = ConvertToSID(&ce);
    }
    unlock_cache();
    return sid;
}

/*
** Place an sid into the cache, if it isn't already there. Note that if
** some other server process has replaced a session-id cache entry that has
** the same cache index as this sid, then all is ok. Somebody has to lose
** when this condition occurs, so it might as well be this sid.
*/
static void ServerSessionIDCache(SSLSessionID *sid)
{
    SIDCacheEntry ce;
    unsigned long offset;
    int rv;
    uint16 version = sid->version;

    if ((version == SSL_LIBRARY_VERSION_3_0) &&
	(sid->u.ssl3.sessionIDLength == 0)) {
	return;
    }

    if (sid->cached == never_cached) {
	lock_cache();

	sid->time = PORT_Time();
	if (version == SSL_LIBRARY_VERSION_2) {
	    SSL_TRC(8, ("%d: SSL: CacheMT: cached=%d addr=0x%08x time=%x "
			"cipher=%d", SSL_GETPID(), sid->cached, sid->addr,
			sid->time, sid->u.ssl2.cipherType));
	    PRINT_BUF(8, (0, "sessionID:", sid->u.ssl2.sessionID,
			  sizeof(sid->u.ssl2.sessionID)));
	    PRINT_BUF(8, (0, "masterKey:", sid->u.ssl2.masterKey.data,
			  sid->u.ssl2.masterKey.len));
	    PRINT_BUF(8, (0, "cipherArg:", sid->u.ssl2.cipherArg.data,
			  sid->u.ssl2.cipherArg.len));

	    /* Write out new cache entry */
	    offset = Offset(sid->addr, sid->u.ssl2.sessionID,
			    sizeof(sid->u.ssl2.sessionID)); 
	} else {
	    SSL_TRC(8, ("%d: SSL: CacheMT: cached=%d addr=0x%08x time=%x "
			"cipherSuite=%d", SSL_GETPID(), sid->cached, sid->addr,
			sid->time, sid->u.ssl3.cipherSuite));
	    PRINT_BUF(8, (0, "sessionID:", sid->u.ssl3.sessionID,
			  sid->u.ssl3.sessionIDLength));

	    offset = Offset(sid->addr, sid->u.ssl3.sessionID,
			    sid->u.ssl3.sessionIDLength);
	    
	}

	ConvertFromSID(&ce, sid);
	if (version == SSL_LIBRARY_VERSION_3_0) {
	    if (sid->peerCert == NULL) {
		ce.u.ssl3.certIndex = -1;
	    } else {
		ce.u.ssl3.certIndex = offset % numCertCacheEntries;
	    }
	}
	
#ifdef XP_UNIX
	lseek(SIDCacheFD, offset, SEEK_SET);
	rv = write(SIDCacheFD, &ce, sizeof(ce));
	if (rv != sizeof(ce)) {
	    IOError(rv, "update-write");
	}
#else /* WIN32 */
	SIDCacheOffset = offset;
	CopyMemory(&SIDCacheData[offset], &ce, sizeof(ce));
#endif /* XP_UNIX */

	if ((version == SSL_LIBRARY_VERSION_3_0) && (sid->peerCert != NULL)) {
	    CacheCert(sid->peerCert, &ce);
	}

	unlock_cache();
    }
}

static void ServerSessionIDUncache(SSLSessionID *sid)
{
    SIDCacheEntry ce;
    int rv;

    if (sid == NULL) return;
    
    lock_cache();
    if (sid->version == SSL_LIBRARY_VERSION_2) {
	SSL_TRC(8, ("%d: SSL: UncacheMT: valid=%d addr=0x%08x time=%x "
		    "cipher=%d", SSL_GETPID(), sid->cached, sid->addr,
		    sid->time, sid->u.ssl2.cipherType));
	PRINT_BUF(8, (0, "sessionID:", sid->u.ssl2.sessionID,
		      sizeof(sid->u.ssl2.sessionID)));
	PRINT_BUF(8, (0, "masterKey:", sid->u.ssl2.masterKey.data,
		      sid->u.ssl2.masterKey.len));
	PRINT_BUF(8, (0, "cipherArg:", sid->u.ssl2.cipherArg.data,
		      sid->u.ssl2.cipherArg.len));
	rv = FindSID(sid->addr, sid->u.ssl2.sessionID,
		     sizeof(sid->u.ssl2.sessionID), &ce);
    } else {
	SSL_TRC(8, ("%d: SSL3: UncacheMT: valid=%d addr=0x%08x time=%x "
		    "cipherSuite=%d", SSL_GETPID(), sid->cached, sid->addr,
		    sid->time, sid->u.ssl3.cipherSuite));
	PRINT_BUF(8, (0, "sessionID:", sid->u.ssl3.sessionID,
		      sid->u.ssl3.sessionIDLength));
	rv = FindSID(sid->addr, sid->u.ssl3.sessionID,
		     sid->u.ssl3.sessionIDLength, &ce);
    }
    
    if (rv) {
	/* Back up one entry */
#ifdef XP_UNIX
	lseek(SIDCacheFD, -sizeof(ce), SEEK_CUR);
	SSL_TRC(7, ("%d: SSL: uncaching session-id at offset %d",
		    SSL_GETPID(), lseek(SIDCacheFD, 0, SEEK_CUR)));
	ce.u.ssl2.valid = 0;
	rv = write(SIDCacheFD, &ce, sizeof(ce));
	if (rv != sizeof(ce)) {
	    IOError(rv, "remove-write");
	}
#else /* WIN32 */
	CopyMemory(&SIDCacheData[SIDCacheOffset], &ce, sizeof(ce));
#endif /* XP_UNIX */
    }
    unlock_cache();
}

/*
** Zero a file out to nb bytes
*/
#ifdef XP_UNIX
static SECStatus ZeroFile(int fd, int nb)
{
    char buf[16384];
    int amount, rv;

    PORT_Memset(buf, 0, sizeof(buf));
    (void) lseek(fd, 0, SEEK_SET);

    while (nb) {
	amount = nb;
	if (amount > sizeof(buf)) {
	    amount = sizeof(buf);
	}
	rv = write(fd, buf, amount);
	if (rv != amount) {
	    IOError(rv, "zero-write");
	    return SECFailure;
	}
	nb -= amount;
    }
    return SECSuccess;
}
#endif /* XP_UNIX */

static SECStatus InitSessionIDCache(int maxCacheEntries, time_t timeout,
				   time_t ssl3_timeout, const char *directory)
{
    char *cfn;
    int rv;
#ifdef XP_UNIX
    if (SIDCacheFD >= 0) {
	/* Already done */
	return SECSuccess;
    }
#else /* WIN32 */
	if(SIDCacheFD != INVALID_HANDLE_VALUE) {
	/* Already done */
	return SECSuccess;
	}
#endif /* XP_UNIX */


    /* Create file names */
    cfn = (char*) PORT_Alloc(PORT_Strlen(directory) + 100);
    if (!cfn) {
	return SECFailure;
    }
#ifdef XP_UNIX
    sprintf(cfn, "%s/.sslsidc.%d", directory, getpid());
#else /* XP_WIN32 */
	sprintf(cfn, "%s\\ssl.sidc.%d.%d", directory,
			GetCurrentProcessId(), GetCurrentThreadId());
#endif /* XP_WIN32 */

    /* Create session-id cache file */
#ifdef XP_UNIX
    (void) unlink(cfn);
    SIDCacheFD = open(cfn, O_CREAT|O_RDWR, 0600);
    if (SIDCacheFD < 0) {
	IOError(SIDCacheFD, "create");
	goto loser;
    }
    rv = unlink(cfn);
    if (rv < 0) {
	IOError(rv, "unlink");
	goto loser;
    }
#else  /* WIN32 */
    SIDCacheFDMAP = CreateFileMapping((HANDLE)0xffffffff, NULL, PAGE_READWRITE,
				      0, numSIDCacheEntries *
				      sizeof(SIDCacheEntry), NULL);
    if(!(SIDCacheFDMAP))
	goto loser;
    SIDCacheData = (char *) MapViewOfFile(SIDCacheFDMAP, FILE_MAP_READ |
				       FILE_MAP_WRITE, 0, 0, 
				       numSIDCacheEntries *
				       sizeof(SIDCacheEntry));
    if(!(SIDCacheData))
	goto loser;
#endif /* XP_UNIX */

#if defined(SERVER_BUILD) && defined(USE_NSPR_MT)
    cacheLock = PR_NewMonitor();
#endif

    if (maxCacheEntries) {
	numSIDCacheEntries = maxCacheEntries;
    }
    if (timeout) {   
	if (timeout > 100) {
	    timeout = 100;
	}
	if (timeout < 5) {
	    timeout = 5;
	}
	ssl_sid_timeout = timeout;
    }

    if (ssl3_timeout) {   
	if (ssl3_timeout > 86400L) {
	    ssl3_timeout = 86400L;
	}
	if (ssl3_timeout < 5) {
	    ssl3_timeout = 5;
	}
	ssl3_sid_timeout = ssl3_timeout;
    }

#ifdef XP_UNIX
    /* Initialize the files */
    if (ZeroFile(SIDCacheFD, numSIDCacheEntries * sizeof(SIDCacheEntry))) {
	/* Bummer */
	close(SIDCacheFD);
	SIDCacheFD = -1;
	goto loser;
    }
#else /* XP_WIN32 */
	ZeroMemory(SIDCacheData, numSIDCacheEntries * sizeof(SIDCacheEntry));
#endif /* XP_UNIX */
    PORT_Free(cfn);
    return SECSuccess;

  loser:
    PORT_Free(cfn);
    return SECFailure;
}

static SECStatus InitCertCache(const char *directory)
{
    char *cfn;
    int rv;
#ifdef XP_UNIX
    if (certCacheFD >= 0) {
	/* Already done */
	return SECSuccess;
    }
#else /* WIN32 */
	if(certCacheFD != INVALID_HANDLE_VALUE) {
	/* Already done */
	return SECSuccess;
	}
#endif /* XP_UNIX */


    /* Create file names */
    cfn = (char*) PORT_Alloc(PORT_Strlen(directory) + 100);
    if (!cfn) {
	return SECFailure;
    }
#ifdef XP_UNIX
    sprintf(cfn, "%s/.sslcertc.%d", directory, getpid());
#else /* XP_WIN32 */
	sprintf(cfn, "%s\\ssl.certc.%d.%d", directory,
			GetCurrentProcessId(), GetCurrentThreadId());
#endif /* XP_WIN32 */

    /* Create certificate cache file */
#ifdef XP_UNIX
    (void) unlink(cfn);
    certCacheFD = open(cfn, O_CREAT|O_RDWR, 0600);
    if (certCacheFD < 0) {
	IOError(certCacheFD, "create");
	goto loser;
    }
    rv = unlink(cfn);
    if (rv < 0) {
	IOError(rv, "unlink");
	goto loser;
    }
#else  /* WIN32 */
    certCacheFDMAP = CreateFileMapping((HANDLE)0xffffffff, NULL, PAGE_READWRITE,
				       0, numCertCacheEntries *
				       sizeof(CertCacheEntry), NULL);
    if(!(certCacheFDMAP))
	goto loser;
    certCacheData = (char *) MapViewOfFile(certCacheFDMAP, FILE_MAP_READ |
					   FILE_MAP_WRITE, 0, 0, 
					   numCertCacheEntries *
					   sizeof(CertCacheEntry));
    if(!(certCacheData))
	goto loser;
#endif /* XP_UNIX */

#ifdef XP_UNIX
    /* Initialize the files */
    if (ZeroFile(certCacheFD, numCertCacheEntries * sizeof(CertCacheEntry))) {
	/* Bummer */
	close(certCacheFD);
	certCacheFD = -1;
	goto loser;
    }
#else /* XP_WIN32 */
	ZeroMemory(certCacheData, numCertCacheEntries * sizeof(CertCacheEntry));
#endif /* XP_UNIX */
    PORT_Free(cfn);
    return SECSuccess;

  loser:
    PORT_Free(cfn);
    return SECFailure;
}

int SSL_ConfigServerSessionIDCache(int maxCacheEntries, time_t timeout,
				   time_t ssl3_timeout, const char *directory)
{
    SECStatus rv;

    PORT_Assert(sizeof(SIDCacheEntry) == 128);

    if (!directory) {
	directory = DEFAULT_CACHE_DIRECTORY;
    }
    rv = InitSessionIDCache(maxCacheEntries, timeout, ssl3_timeout, directory);
    if (rv) return -1;
    rv = InitCertCache(directory);
    if (rv) return -1;

    ssl_sid_lookup = ServerSessionIDLookup;
    ssl_sid_cache = ServerSessionIDCache;
    ssl_sid_uncache = ServerSessionIDUncache;
    return 0;
}

#endif /* NADA_VERISON */
#endif /* XP_UNIX || XP_WIN32 */
