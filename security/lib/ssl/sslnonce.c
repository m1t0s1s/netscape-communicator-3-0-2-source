#include "xp.h"
#include "cert.h"
#include "secitem.h"
#include "ssl.h"

#ifndef NADA_VERSION
#include "sslimpl.h"
#include "sslproto.h"
#ifdef FORTEZZA
/* for FortezzaFreeKey */
#include "fortezza.h"
#endif

/* XXX rename this file! */

time_t ssl_sid_timeout = 100;
time_t ssl3_sid_timeout = 86400L; /* 24 hours */

static SSLSessionID *cache;

static void
ssl_DestroySID(SSLSessionID *sid)
{
    SSL_TRC(8, ("SSL: destroy sid: sid=0x%x cached=%d", sid, sid->cached));
    PORT_Assert((sid->references == 0));
    if (sid->version == SSL_LIBRARY_VERSION_2) {
	SECITEM_ZfreeItem(&sid->u.ssl2.masterKey, PR_FALSE);
	SECITEM_ZfreeItem(&sid->u.ssl2.cipherArg, PR_FALSE);
#ifdef FORTEZZA
    } else {
	if (sid->u.ssl3.hasFortezza) {
	    if (sid->u.ssl3.clientWriteKey) {
		FortezzaFreeKey(sid->u.ssl3.clientWriteKey);
		FortezzaFreeKey(sid->u.ssl3.serverWriteKey);
		FortezzaFreeKey(sid->u.ssl3.tek);
	    }
	}
#endif
    }
    if (sid->peerID != NULL)
	PORT_Free(sid->peerID);

    if ( sid->peerCert ) {
	CERT_DestroyCertificate(sid->peerCert);
    }
    
    PORT_ZFree(sid, sizeof(SSLSessionID));
}

void
ssl_FreeSID(SSLSessionID *sid)
{
    PORT_Assert(sid->references >= 1);
    if (--sid->references == 0) {
	ssl_DestroySID(sid);
    }
}

/************************************************************************/

/*
** Code to manage a session-id cache that assumes that the caller is single
** threaded.
*/

SSLSessionID *
ssl_LookupSID(unsigned long addr, unsigned short port, char *peerID)
{
    SSLSessionID **sidp;
    SSLSessionID *sid;
    unsigned long now;

    now = PORT_Time();
    sidp = &cache;
    while ((sid = *sidp) != 0) {
	PORT_Assert(sid->cached);
	PORT_Assert(sid->references >= 1);
	SSL_TRC(8, ("SSL: Lookup1: sid=0x%x", sid));
	if (((sid->version == SSL_LIBRARY_VERSION_2) &&
	     (sid->time + ssl_sid_timeout < now)) ||
	    ((sid->version == SSL_LIBRARY_VERSION_3_0) &&
	     (sid->time + ssl3_sid_timeout < now))) {
	    /*
	    ** This session-id timed out. Don't even care who it belongs
	    ** to, blow it out of our cache.
	    */
	    SSL_TRC(7, ("SSL: lookup1, throwing sid out, age=%d refs=%d",
			now - sid->time, sid->references));
	    *sidp = sid->next;
	    ssl_FreeSID(sid);
	} else if (sid->addr == addr && sid->port == port &&
		   (((peerID == NULL) && (sid->peerID == NULL)) ||
		    ((peerID != NULL) && (sid->peerID != NULL) &&
		     PORT_Strcmp(sid->peerID, peerID) == 0)) &&
		   (sid->version == SSL_LIBRARY_VERSION_2 ||
		    sid->u.ssl3.resumable)) {
	    /* Hit */
	    sid->references++;
	    return sid;
	} else {
	    sidp = &sid->next;
	}
    }
    return sid;
}

/*
** Add an sid to the cache or return a cached entry to the cache.
*/
static void CacheSID(SSLSessionID *sid)
{
    SSL_TRC(8, ("SSL: Cache: sid=0x%x cached=%d addr=0x%08x port=0x%04x "
		"time=%x cached=%d",
		sid, sid->cached, sid->addr, sid->port, sid->time,
		sid->cached));
    if (sid->cached != never_cached)
	return;

    /* XXX should be different trace for version 2 vs. version 3 */
    if (sid->version == SSL_LIBRARY_VERSION_2) {
	PRINT_BUF(8, (0, "sessionID:",
		  sid->u.ssl2.sessionID, sizeof(sid->u.ssl2.sessionID)));
	PRINT_BUF(8, (0, "masterKey:",
		  sid->u.ssl2.masterKey.data, sid->u.ssl2.masterKey.len));
	PRINT_BUF(8, (0, "cipherArg:",
		  sid->u.ssl2.cipherArg.data, sid->u.ssl2.cipherArg.len));
    } else {
	if (sid->u.ssl3.sessionIDLength == 0) return;
	PRINT_BUF(8, (0, "sessionID:",
		      sid->u.ssl3.sessionID, sid->u.ssl3.sessionIDLength));
    }

    /*
     * Put sid into the cache. Set references to one to indicate that
     * cache is holding a reference. Uncache will reduce the cache
     * reference.
     */
    sid->references++;
    sid->cached = in_cache;
    sid->next = cache;
    cache = sid;
    sid->time = PORT_Time();
}

static void
UncacheSID(SSLSessionID *zap)
{
    SSLSessionID **sidp = &cache;
    SSLSessionID *sid;

    if (zap->cached != in_cache) {
	return;
    }

    SSL_TRC(8,("SSL: Uncache: zap=0x%x cached=%d addr=0x%08x port=0x%04x "
	       "time=%x cipher=%d",
	       zap, zap->cached, zap->addr, zap->port, zap->time,
	       zap->u.ssl2.cipherType));
    if (zap->version == SSL_LIBRARY_VERSION_2) {
	PRINT_BUF(8, (0, "sessionID:",
		      zap->u.ssl2.sessionID, sizeof(zap->u.ssl2.sessionID)));
	PRINT_BUF(8, (0, "masterKey:",
		      zap->u.ssl2.masterKey.data, zap->u.ssl2.masterKey.len));
	PRINT_BUF(8, (0, "cipherArg:",
		      zap->u.ssl2.cipherArg.data, zap->u.ssl2.cipherArg.len));
    }

    /* See if it's in the cache, if so nuke it */
    while ((sid = *sidp) != 0) {
	if (sid == zap) {
	    /*
	    ** Bingo. Reduce reference count by one so that when
	    ** everyone is done with the sid we can free it up.
	    */
	    *sidp = zap->next;
	    zap->cached = invalid_cache;
	    ssl_FreeSID(zap);
	    return;
	}
	sidp = &sid->next;
    }
}

void ssl_ChooseSessionIDProcs(SSLSecurityInfo *sec)
{
    if (sec->isServer) {
	sec->cache = ssl_sid_cache;
	sec->uncache = ssl_sid_uncache;
    } else {
	sec->cache = CacheSID;
	sec->uncache = UncacheSID;
    }
}


void
SSL_ClearSessionCache(void)
{
    while(cache != NULL)
	UncacheSID(cache);
}

#endif /* !NADA_VERSION */
