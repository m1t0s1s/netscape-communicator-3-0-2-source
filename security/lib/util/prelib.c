/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*- */

#include "sec.h"
#include "sslimpl.h"
#include "fortezza.h"
#include "preenc.h"

PEHeader *SSL_PreencryptedStreamToFile(int fd, PEHeader *header)
{
    FortezzaKey *key;
    FortezzaPCMCIASocket *psock;
    FortezzaKEAContext *kea_cx;
    SSLSocket *ss;
    
    if (fd < 0) {
        /* XXX set an error */
        return NULL;
    }
    
    ss = ssl_FindSocket(fd);
    if (ss == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    PORT_Assert(ss->ssl3 != NULL);
    
    /* get the kea context from the session */
    kea_cx = ss->ssl3->kea_context;
    if (kea_cx == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    key = FortezzaKeyAlloc(kea_cx);
    if (key == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    psock = (FortezzaPCMCIASocket *) kea_cx->socket;
    if (psock == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    key->keyRegister = KeyNotLoaded;
    key->hitCount = 0;
    key->keyType = TEKMEK;
    key->keyData.tek_mek.wrappingTEK = kea_cx->tek;
    PORT_Memcpy(key->keyData.tek_mek.TEK_M, header->u.fortezza.key,
              sizeof(FortezzaSymetric));
    (kea_cx->tek->ref_count)++;
    
    CI_LOCK(psock);
    FortezzaRestoreKey(psock, NULL, key);
    CI_UNLOCK(psock);
    
    PORT_Assert(key->keyType == MEK);
    
    /* overwrite the existing key in our header */
    PORT_Memcpy(header->u.fortezza.key, key->keyData.mek.Ks_M,
              sizeof(FortezzaSymetric));
    
    /* copy our local serial number into header */
    PORT_Memcpy(header->u.fortezza.serial, psock->serial,
              sizeof(CI_SERIAL_NUMBER));
    
    /* change type to file */
    PutInt2(header->type, PRE_FORTEZZA_FILE);
    
    FortezzaFreeKey(key);
    
    return(header);
}

PEHeader *SSL_PreencryptedFileToStream(int fd, PEHeader *header)
{
    FortezzaSymetric wrapped_key;
    FortezzaKEAContext *kea_cx;
    SSLSocket *ss;
	SECStatus rv;
    
    if (fd < 0) {
        /* XXX set an error */
        return NULL;
    }
    
    ss = ssl_FindSocket(fd);
    if (ss == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    PORT_Assert(ss->ssl3 != NULL);
    
    /* get the kea context from the session */
    kea_cx = ss->ssl3->kea_context;
    if (kea_cx == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    rv = FortezzaRewrapKey(kea_cx, (unsigned char *)header->u.fortezza.key,
						   wrapped_key);
    if (rv == SECFailure) {
        /* XXX set an error */
        return NULL;
    }
    
    /* overwrite the existing key in our header */
    PORT_Memcpy(header->u.fortezza.key, wrapped_key, sizeof(FortezzaSymetric));
    
    /* copy over our local serial number */
    PORT_Memset(header->u.fortezza.serial, 0, sizeof(CI_SERIAL_NUMBER));
    
    /* change type to stream */
    PutInt2(header->type, PRE_FORTEZZA_STREAM);
    
    return(header);
}


