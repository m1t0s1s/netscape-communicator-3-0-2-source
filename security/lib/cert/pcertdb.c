/*
 * Permanent Certificate database handling code
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 * 
 * $Id: pcertdb.c,v 1.40.2.9 1997/05/24 00:23:05 jwz Exp $
 */
#include "prtime.h"

#include "cert.h"
#include "mcom_db.h"
#include "certdb.h"
#include "secitem.h"
#include "secder.h"
#include "pk11func.h"
#include "secasn1.h"

#if defined(SERVER_BUILD)
#define dbopen NS_dbopen
#endif

extern int SEC_ERROR_BAD_DATABASE;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_CRL_INVALID;
extern int SEC_ERROR_CRL_EXPIRED;
extern int SEC_ERROR_KRL_INVALID;
extern int SEC_ERROR_KRL_EXPIRED;
extern int SEC_ERROR_OLD_CRL;

static SECStatus
DeleteDBEntry(CERTCertDBHandle *handle, certDBEntryType type, SECItem *dbkey)
{
    DBT key;
    int ret;

    /* init the database key */
    key.data = dbkey->data;
    key.size = dbkey->len;
    
    dbkey->data[0] = (unsigned char)type;

    /* delete entry from database */
    ret = (* handle->permCertDB->del)(handle->permCertDB, &key, 0 );
    if ( ret != 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    ret = (* handle->permCertDB->sync)(handle->permCertDB, 0);
    if ( ret ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    return(SECSuccess);
    
loser:
    return(SECFailure);
}

static SECStatus
ReadDBEntry(CERTCertDBHandle *handle, certDBEntryCommon *entry,
	    SECItem *dbkey, SECItem *dbentry, PRArenaPool *arena)
{
    DBT data, key;
    int ret;
    unsigned char *buf;
    
    /* init the database key */
    key.data = dbkey->data;
    key.size = dbkey->len;
    
    dbkey->data[0] = (unsigned char)entry->type;

    /* read entry from database */
    ret = (* handle->permCertDB->get)(handle->permCertDB, &key, &data, 0 );
    if ( ret != 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* validate the entry */
    if ( data.size < SEC_DB_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    buf = (unsigned char *)data.data;
    if ( buf[0] != (unsigned char)CERT_DB_FILE_VERSION ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    if ( buf[1] != (unsigned char)entry->type ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    /* copy out header information */
    entry->version = (unsigned int)buf[0];
    entry->type = (certDBEntryType)buf[1];
    entry->flags = (unsigned int)buf[2];
    
    /* format body of entry for return to caller */
    dbentry->len = data.size - SEC_DB_ENTRY_HEADER_LEN;
    if ( dbentry->len ) {
	dbentry->data = (unsigned char *)PORT_ArenaAlloc(arena, dbentry->len);
	if ( dbentry->data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
    
	PORT_Memcpy(dbentry->data, &buf[SEC_DB_ENTRY_HEADER_LEN],
		  dbentry->len);
    } else {
	dbentry->data = NULL;
    }
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/**
 ** Implement low level database access
 **/
static SECStatus
WriteDBEntry(CERTCertDBHandle *handle, certDBEntryCommon *entry,
	     SECItem *dbkey, SECItem *dbentry)
{
    int ret;
    DBT data, key;
    unsigned char *buf;
    
    data.data = dbentry->data;
    data.size = dbentry->len;
    
    buf = data.data;
    
    buf[0] = (unsigned char)entry->version;
    buf[1] = (unsigned char)entry->type;
    buf[2] = (unsigned char)entry->flags;
    
    key.data = dbkey->data;
    key.size = dbkey->len;
    
    dbkey->data[0] = (unsigned char)entry->type;

    /* put the record into the database now */
    ret = (* handle->permCertDB->put)(handle->permCertDB, &key,
				      &data, 0);

    if ( ret != 0 ) {
	goto loser;
    }

    ret = (* handle->permCertDB->sync)( handle->permCertDB, 0 );
    
    if ( ret ) {
	goto loser;
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * encode a database cert record
 */
static SECStatus
EncodeDBCertEntry(certDBEntryCert *entry, PRArenaPool *arena, SECItem *dbitem)
{
    unsigned int nnlen;
    unsigned char *buf;
    char *nn;
    char zbuf = 0;
    
    if ( entry->nickname ) {
	nn = entry->nickname;
    } else {
	nn = &zbuf;
    }
    nnlen = PORT_Strlen(nn) + 1;
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->derCert.len + nnlen + DB_CERT_ENTRY_HEADER_LEN +
	SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->trust.sslFlags >> 8 ) & 0xff;
    buf[1] = entry->trust.sslFlags & 0xff;
    buf[2] = ( entry->trust.emailFlags >> 8 ) & 0xff;
    buf[3] = entry->trust.emailFlags & 0xff;
    buf[4] = ( entry->trust.objectSigningFlags >> 8 ) & 0xff;
    buf[5] = entry->trust.objectSigningFlags & 0xff;
    buf[6] = ( entry->derCert.len >> 8 ) & 0xff;
    buf[7] = entry->derCert.len & 0xff;
    buf[8] = ( nnlen >> 8 ) & 0xff;
    buf[9] = nnlen & 0xff;
    
    PORT_Memcpy(&buf[DB_CERT_ENTRY_HEADER_LEN], entry->derCert.data,
	      entry->derCert.len);

    PORT_Memcpy(&buf[DB_CERT_ENTRY_HEADER_LEN + entry->derCert.len],
	      nn, nnlen);

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * encode a database key for a cert record
 */
static SECStatus
EncodeDBCertKey(SECItem *certKey, PRArenaPool *arena, SECItem *dbkey)
{
    dbkey->len = certKey->len + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN],
	      certKey->data, certKey->len);
    dbkey->data[0] = certDBEntryTypeCert;

    return(SECSuccess);
loser:
    return(SECFailure);
}

static SECStatus
EncodeDBGenericKey(SECItem *certKey, PRArenaPool *arena, SECItem *dbkey, 
				certDBEntryType entryType)
{
    dbkey->len = certKey->len + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN],
	      certKey->data, certKey->len);
    dbkey->data[0] = (unsigned char) entryType;

    return(SECSuccess);
loser:
    return(SECFailure);
}

static SECStatus
DecodeDBCertEntry(certDBEntryCert *entry, SECItem *dbentry)
{
    unsigned int nnlen;
    int headerlen;
    int lenoff;

    /* allow updates of old versions of the database */
    switch ( entry->common.version ) {
      case 5:
	headerlen = DB_CERT_V5_ENTRY_HEADER_LEN;
	lenoff = 3;
	break;
      case 6:
	/* should not get here */
	PORT_Assert(0);
	headerlen = DB_CERT_V6_ENTRY_HEADER_LEN;
	lenoff = 3;
	break;
      case 7:
	headerlen = DB_CERT_ENTRY_HEADER_LEN;
	lenoff = 6;
	break;
      default:
	/* better not get here */
	PORT_Assert(0);
	headerlen = DB_CERT_V5_ENTRY_HEADER_LEN;
	lenoff = 3;
	break;
    }
    
    /* is record long enough for header? */
    if ( dbentry->len < headerlen ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->derCert.len = ( ( dbentry->data[lenoff] << 8 ) |
			  dbentry->data[lenoff+1] );
    nnlen = ( ( dbentry->data[lenoff+2] << 8 ) | dbentry->data[lenoff+3] );
    if ( ( entry->derCert.len + nnlen + headerlen )
	!= dbentry->len) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the dercert */
    entry->derCert.data = (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
							   entry->derCert.len);
    if ( entry->derCert.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->derCert.data, &dbentry->data[headerlen],
	      entry->derCert.len);

    /* copy the nickname */
    if ( nnlen > 1 ) {
	entry->nickname = (char *)PORT_ArenaAlloc(entry->common.arena, nnlen);
	if ( entry->nickname == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->nickname,
		    &dbentry->data[headerlen +
				   entry->derCert.len],
		    nnlen);
    } else {
	entry->nickname = NULL;
    }
    
    if ( entry->common.version < 7 ) {
	/* allow updates of v5 db */
	entry->trust.sslFlags = dbentry->data[0];
	entry->trust.emailFlags = dbentry->data[1];
	entry->trust.objectSigningFlags = dbentry->data[2];
    } else {
	entry->trust.sslFlags = ( dbentry->data[0] << 8 ) | dbentry->data[1];
	entry->trust.emailFlags = ( dbentry->data[2] << 8 ) | dbentry->data[3];
	entry->trust.objectSigningFlags =
	    ( dbentry->data[4] << 8 ) | dbentry->data[5];
    }
    
    return(SECSuccess);
loser:
    return(SECFailure);
}


/*
 * Create a new certDBEntryCert from existing data
 */
static certDBEntryCert *
NewDBCertEntry(SECItem *derCert, char *nickname,
	       CERTCertTrust *trust, int flags)
{
    certDBEntryCert *entry;
    PRArenaPool *arena = NULL;
    int nnlen;
    
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if ( !arena ) {
	goto loser;
    }
	
    entry = (certDBEntryCert *)PORT_ArenaZAlloc(arena, sizeof(certDBEntryCert));

    if ( entry == NULL ) {
	goto loser;
    }
    
    /* fill in the dbCert */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeCert;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;
    
    if ( trust ) {
	entry->trust = *trust;
    }

    entry->derCert.data = (unsigned char *)PORT_ArenaAlloc(arena, derCert->len);
    if ( !entry->derCert.data ) {
	goto loser;
    }
    entry->derCert.len = derCert->len;
    PORT_Memcpy(entry->derCert.data, derCert->data, derCert->len);
    
    nnlen = ( nickname ? strlen(nickname) + 1 : 0 );
    
    if ( nnlen ) {
	entry->nickname = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( !entry->nickname ) {
	    goto loser;
	}
	PORT_Memcpy(entry->nickname, nickname, nnlen);
	
    } else {
	entry->nickname = 0;
    }

    return(entry);

loser:
    
    /* allocation error, free arena and return */
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return(0);
}

/*
 * Decode a version 4(cheddar) DBCert from the byte stream database format
 * and construct a current database entry struct
 */
static certDBEntryCert *
DecodeV4DBCertEntry(unsigned char *buf, int len)
{
    certDBEntryCert *entry;
    int certlen;
    int nnlen;
    PRArenaPool *arena;
    
    /* make sure length is at least long enough for the header */
    if ( len < DBCERT_V4_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	return(0);
    }

    /* get other lengths */
    certlen = buf[3] << 8 | buf[4];
    nnlen = buf[5] << 8 | buf[6];
    
    /* make sure DB entry is the right size */
    if ( ( certlen + nnlen + DBCERT_V4_HEADER_LEN ) != len ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	return(0);
    }

    /* allocate arena */
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if ( !arena ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return(0);
    }
	
    /* allocate structure and members */
    entry = (certDBEntryCert *)  PORT_ArenaAlloc(arena, sizeof(certDBEntryCert));

    if ( !entry ) {
	goto loser;
    }

    entry->derCert.data = (unsigned char *)PORT_ArenaAlloc(arena, certlen);
    if ( !entry->derCert.data ) {
	goto loser;
    }
    entry->derCert.len = certlen;
    
    if ( nnlen ) {
	entry->nickname = (char *) PORT_ArenaAlloc(arena, nnlen);
	if ( !entry->nickname ) {
	    goto loser;
	}
    } else {
	entry->nickname = 0;
    }

    entry->common.arena = arena;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.type = certDBEntryTypeCert;
    entry->common.flags = 0;
    entry->trust.sslFlags = buf[0];
    entry->trust.emailFlags = buf[1];
    entry->trust.objectSigningFlags = buf[2];

    PORT_Memcpy(entry->derCert.data, &buf[DBCERT_V4_HEADER_LEN], certlen);
    PORT_Memcpy(entry->nickname, &buf[DBCERT_V4_HEADER_LEN + certlen], nnlen);

    return(entry);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return(0);
}

/*
 * Encode a Certificate database entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBCertEntry(CERTCertDBHandle *handle, certDBEntryCert *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECItem tmpitem;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBCertEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* get the database key and format it */
    rv = CERT_KeyFromDERCert(tmparena, &entry->derCert, &tmpitem);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = EncodeDBCertKey(&tmpitem, tmparena, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }
    
    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}


/*
 * delete a certificate entry
 */
static SECStatus
DeleteDBCertEntry(CERTCertDBHandle *handle, SECItem *certKey)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBCertKey(certKey, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = DeleteDBEntry(handle, certDBEntryTypeCert, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a certificate entry
 */
static certDBEntryCert *
ReadDBCertEntry(CERTCertDBHandle *handle, SECItem *certKey)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryCert *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryCert *)PORT_ArenaAlloc(arena, sizeof(certDBEntryCert));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeCert;

    rv = EncodeDBCertKey(certKey, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = DecodeDBCertEntry(entry, &dbentry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * encode a database cert record
 */
static SECStatus
EncodeDBCrlEntry(certDBEntryRevocation *entry, PRArenaPool *arena, SECItem *dbitem)
{
    unsigned int nnlen = 0;
    unsigned char *buf;
  
    if (entry->url) {  
	nnlen = PORT_Strlen(entry->url) + 1;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->derCrl.len + nnlen 
		+ SEC_DB_ENTRY_HEADER_LEN + DB_CRL_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->derCrl.len >> 8 ) & 0xff;
    buf[1] = entry->derCrl.len & 0xff;
    buf[2] = ( nnlen >> 8 ) & 0xff;
    buf[3] = nnlen & 0xff;
    
    PORT_Memcpy(&buf[DB_CRL_ENTRY_HEADER_LEN], entry->derCrl.data,
	      entry->derCrl.len);

    if (nnlen != 0) {
	PORT_Memcpy(&buf[DB_CRL_ENTRY_HEADER_LEN + entry->derCrl.len],
	      entry->url, nnlen);
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

static SECStatus
DecodeDBCrlEntry(certDBEntryRevocation *entry, SECItem *dbentry)
{
    unsigned int nnlen;
    
    /* is record long enough for header? */
    if ( dbentry->len < DB_CRL_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->derCrl.len = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    nnlen = ( ( dbentry->data[2] << 8 ) | dbentry->data[3] );
    if ( ( entry->derCrl.len + nnlen + DB_CRL_ENTRY_HEADER_LEN )
	!= dbentry->len) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the dercert */
    entry->derCrl.data = (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
							 entry->derCrl.len);
    if ( entry->derCrl.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->derCrl.data, &dbentry->data[DB_CRL_ENTRY_HEADER_LEN],
	      entry->derCrl.len);

    /* copy the url */
    entry->url = NULL;
    if (nnlen != 0) {
	entry->url = (char *)PORT_ArenaAlloc(entry->common.arena, nnlen);
	if ( entry->url == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->url,
	      &dbentry->data[DB_CERT_ENTRY_HEADER_LEN + entry->derCrl.len],
	      nnlen);
    }
    
    return(SECSuccess);
loser:
    return(SECFailure);
}

/*
 * Create a new certDBEntryRevocation from existing data
 */
static certDBEntryRevocation *
NewDBCrlEntry(SECItem *derCrl, char * url, certDBEntryType crlType, int flags)
{
    certDBEntryRevocation *entry;
    PRArenaPool *arena = NULL;
    int nnlen;
    
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if ( !arena ) {
	goto loser;
    }
	
    entry = (certDBEntryRevocation*)
			PORT_ArenaZAlloc(arena, sizeof(certDBEntryRevocation));

    if ( entry == NULL ) {
	goto loser;
    }
    
    /* fill in the dbRevolcation */
    entry->common.arena = arena;
    entry->common.type = crlType;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;
    

    entry->derCrl.data = (unsigned char *)PORT_ArenaAlloc(arena, derCrl->len);
    if ( !entry->derCrl.data ) {
	goto loser;
    }

    if (url) {
	nnlen = PORT_Strlen(url) + 1;
	entry->url  = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( !entry->url ) {
	    goto loser;
	}
	PORT_Memcpy(entry->url, url, nnlen);
    } else {
	entry->url = NULL;
    }

	
    entry->derCrl.len = derCrl->len;
    PORT_Memcpy(entry->derCrl.data, derCrl->data, derCrl->len);

    return(entry);

loser:
    
    /* allocation error, free arena and return */
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return(0);
}


static SECStatus
WriteDBCrlEntry(CERTCertDBHandle *handle, certDBEntryRevocation *entry )
{
    SECItem dbkey;
    PRArenaPool *tmparena = NULL;
    SECItem tmpitem,encodedEntry;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }

    /* get the database key and format it */
    rv = CERT_KeyFromDERCrl(tmparena, &entry->derCrl, &tmpitem);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = EncodeDBCrlEntry(entry, tmparena, &encodedEntry);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = EncodeDBGenericKey(&tmpitem, tmparena, &dbkey, entry->common.type);
    if ( rv == SECFailure ) {
	goto loser;
    }
    
    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &encodedEntry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}
/*
 * delete a crl entry
 */
static SECStatus
DeleteDBCrlEntry(CERTCertDBHandle *handle, SECItem *crlKey, 
						certDBEntryType crlType)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBGenericKey(crlKey, arena, &dbkey, crlType);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = DeleteDBEntry(handle, crlType, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a certificate entry
 */
static certDBEntryRevocation *
ReadDBCrlEntry(CERTCertDBHandle *handle, SECItem *certKey,
						certDBEntryType crlType)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryRevocation *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryRevocation *)
			PORT_ArenaAlloc(arena, sizeof(certDBEntryRevocation));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = crlType;

    rv = EncodeDBGenericKey(certKey, tmparena, &dbkey, crlType);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = DecodeDBCrlEntry(entry, &dbentry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * destroy a database entry
 */
void
DestroyDBEntry(certDBEntry *entry)
{
    PORT_FreeArena(entry->common.arena, PR_FALSE);

    return;
}

/*
 * Encode a database nickname record
 */
static SECStatus
EncodeDBNicknameEntry(certDBEntryNickname *entry, PRArenaPool *arena,
		      SECItem *dbitem)
{
    unsigned char *buf;
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->subjectName.len + DB_NICKNAME_ENTRY_HEADER_LEN +
	SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->subjectName.len >> 8 ) & 0xff;
    buf[1] = entry->subjectName.len & 0xff;
    
    PORT_Memcpy(&buf[DB_NICKNAME_ENTRY_HEADER_LEN], entry->subjectName.data,
	      entry->subjectName.len);

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Encode a database key for a nickname record
 */
static SECStatus
EncodeDBNicknameKey(char *nickname, PRArenaPool *arena,
		    SECItem *dbkey)
{
    unsigned int nnlen;
    
    nnlen = PORT_Strlen(nickname) + 1; /* includes null */

    /* now get the database key and format it */
    dbkey->len = nnlen + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN], nickname, nnlen);
    dbkey->data[0] = certDBEntryTypeNickname;

    return(SECSuccess);

loser:
    return(SECFailure);
}

static SECStatus
DecodeDBNicknameEntry(certDBEntryNickname *entry, SECItem *dbentry)
{
    /* is record long enough for header? */
    if ( dbentry->len < DB_NICKNAME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->subjectName.len = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    if (( entry->subjectName.len + DB_NICKNAME_ENTRY_HEADER_LEN ) !=
	dbentry->len ){
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the certkey */
    entry->subjectName.data =
	(unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					 entry->subjectName.len);
    if ( entry->subjectName.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->subjectName.data,
	      &dbentry->data[DB_NICKNAME_ENTRY_HEADER_LEN],
	      entry->subjectName.len);
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new nickname entry
 */
static certDBEntryNickname *
NewDBNicknameEntry(char *nickname, SECItem *subjectName, unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntryNickname *entry;
    int nnlen;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntryNickname *)PORT_ArenaAlloc(arena,
						 sizeof(certDBEntryNickname));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* init common fields */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeNickname;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    /* copy the nickname */
    nnlen = PORT_Strlen(nickname) + 1;
    
    entry->nickname = PORT_ArenaAlloc(arena, nnlen);
    if ( entry->nickname == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(entry->nickname, nickname, nnlen);
    
    rv = SECITEM_CopyItem(arena, &entry->subjectName, subjectName);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * delete a nickname entry
 */
static SECStatus
DeleteDBNicknameEntry(CERTCertDBHandle *handle, char *nickname)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    SECItem dbkey;
    
    if ( nickname == NULL ) {
	return(SECSuccess);
    }
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBNicknameKey(nickname, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = DeleteDBEntry(handle, certDBEntryTypeNickname, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a nickname entry
 */
static certDBEntryNickname *
ReadDBNicknameEntry(CERTCertDBHandle *handle, char *nickname)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryNickname *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryNickname *)PORT_ArenaAlloc(arena,
						 sizeof(certDBEntryNickname));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeNickname;

    rv = EncodeDBNicknameKey(nickname, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    /* is record long enough for header? */
    if ( dbentry.len < DB_NICKNAME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    rv = DecodeDBNicknameEntry(entry, &dbentry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a nickname entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBNicknameEntry(CERTCertDBHandle *handle, certDBEntryNickname *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBNicknameEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = EncodeDBNicknameKey(entry->nickname, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
    
}

/*
 * Encode a database smime record
 */
static SECStatus
EncodeDBSMimeEntry(certDBEntrySMime *entry, PRArenaPool *arena,
		   SECItem *dbitem)
{
    unsigned char *buf;
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->subjectName.len + entry->smimeOptions.len +
	entry->optionsDate.len +
	DB_SMIME_ENTRY_HEADER_LEN + SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->subjectName.len >> 8 ) & 0xff;
    buf[1] = entry->subjectName.len & 0xff;
    buf[2] = ( entry->smimeOptions.len >> 8 ) & 0xff;
    buf[3] = entry->smimeOptions.len & 0xff;
    buf[4] = ( entry->optionsDate.len >> 8 ) & 0xff;
    buf[5] = entry->optionsDate.len & 0xff;

    /* if no smime options, then there should not be an options date either */
    PORT_Assert( ! ( ( entry->smimeOptions.len == 0 ) &&
		    ( entry->optionsDate.len != 0 ) ) );
    
    PORT_Memcpy(&buf[DB_SMIME_ENTRY_HEADER_LEN], entry->subjectName.data,
	      entry->subjectName.len);
    if ( entry->smimeOptions.len ) {
	PORT_Memcpy(&buf[DB_SMIME_ENTRY_HEADER_LEN+entry->subjectName.len],
		    entry->smimeOptions.data,
		    entry->smimeOptions.len);
	PORT_Memcpy(&buf[DB_SMIME_ENTRY_HEADER_LEN + entry->subjectName.len +
			 entry->smimeOptions.len],
		    entry->optionsDate.data,
		    entry->optionsDate.len);
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Encode a database key for a SMIME record
 */
static SECStatus
EncodeDBSMimeKey(char *emailAddr, PRArenaPool *arena,
		 SECItem *dbkey)
{
    unsigned int addrlen;
    
    addrlen = PORT_Strlen(emailAddr) + 1; /* includes null */

    /* now get the database key and format it */
    dbkey->len = addrlen + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN], emailAddr, addrlen);
    dbkey->data[0] = certDBEntryTypeSMimeProfile;

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Decode a database SMIME record
 */
static SECStatus
DecodeDBSMimeEntry(certDBEntrySMime *entry, SECItem *dbentry, char *emailAddr)
{
    /* is record long enough for header? */
    if ( dbentry->len < DB_SMIME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->subjectName.len = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    entry->smimeOptions.len = ( ( dbentry->data[2] << 8 ) | dbentry->data[3] );
    entry->optionsDate.len = ( ( dbentry->data[4] << 8 ) | dbentry->data[5] );
    if (( entry->subjectName.len + entry->smimeOptions.len +
	 entry->optionsDate.len + DB_SMIME_ENTRY_HEADER_LEN ) != dbentry->len){
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the subject name */
    entry->subjectName.data =
	(unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					 entry->subjectName.len);
    if ( entry->subjectName.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->subjectName.data,
	      &dbentry->data[DB_SMIME_ENTRY_HEADER_LEN],
	      entry->subjectName.len);

    /* copy the smime options */
    if ( entry->smimeOptions.len ) {
	entry->smimeOptions.data =
	    (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					     entry->smimeOptions.len);
	if ( entry->smimeOptions.data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->smimeOptions.data,
		    &dbentry->data[DB_SMIME_ENTRY_HEADER_LEN +
				   entry->subjectName.len],
		    entry->smimeOptions.len);
    }
    if ( entry->optionsDate.len ) {
	entry->optionsDate.data =
	    (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					     entry->optionsDate.len);
	if ( entry->optionsDate.data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->optionsDate.data,
		    &dbentry->data[DB_SMIME_ENTRY_HEADER_LEN +
				   entry->subjectName.len +
				   entry->smimeOptions.len],
		    entry->optionsDate.len);
    }

    /* both options and options date must either exist or not exist */
    if ( ( ( entry->optionsDate.len == 0 ) ||
	  ( entry->smimeOptions.len == 0 ) ) &&
	entry->smimeOptions.len != entry->optionsDate.len ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    entry->emailAddr = (char *)PORT_Alloc(PORT_Strlen(emailAddr)+1);
    if ( entry->emailAddr ) {
	PORT_Strcpy(entry->emailAddr, emailAddr);
    }
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new SMIME entry
 */
static certDBEntrySMime *
NewDBSMimeEntry(char *emailAddr, SECItem *subjectName, SECItem *smimeOptions,
		SECItem *optionsDate, unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntrySMime *entry;
    int addrlen;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntrySMime *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntrySMime));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* init common fields */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSMimeProfile;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    /* copy the email addr */
    addrlen = PORT_Strlen(emailAddr) + 1;
    
    entry->emailAddr = PORT_ArenaAlloc(arena, addrlen);
    if ( entry->emailAddr == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(entry->emailAddr, emailAddr, addrlen);
    
    /* copy the subject name */
    rv = SECITEM_CopyItem(arena, &entry->subjectName, subjectName);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* copy the smime options */
    if ( smimeOptions ) {
	rv = SECITEM_CopyItem(arena, &entry->smimeOptions, smimeOptions);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	PORT_Assert(optionsDate == NULL);
	entry->smimeOptions.data = NULL;
	entry->smimeOptions.len = 0;
    }

    /* copy the options date */
    if ( optionsDate ) {
	rv = SECITEM_CopyItem(arena, &entry->optionsDate, optionsDate);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	PORT_Assert(smimeOptions == NULL);
	entry->optionsDate.data = NULL;
	entry->optionsDate.len = 0;
    }
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * delete a SMIME entry
 */
static SECStatus
DeleteDBSMimeEntry(CERTCertDBHandle *handle, char *emailAddr)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    SECItem dbkey;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBSMimeKey(emailAddr, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = DeleteDBEntry(handle, certDBEntryTypeSMimeProfile, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a SMIME entry
 */
static certDBEntrySMime *
ReadDBSMimeEntry(CERTCertDBHandle *handle, char *emailAddr)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntrySMime *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntrySMime *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntrySMime));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSMimeProfile;

    rv = EncodeDBSMimeKey(emailAddr, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    /* is record long enough for header? */
    if ( dbentry.len < DB_SMIME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    rv = DecodeDBSMimeEntry(entry, &dbentry, emailAddr);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a SMIME entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBSMimeEntry(CERTCertDBHandle *handle, certDBEntrySMime *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBSMimeEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = EncodeDBSMimeKey(entry->emailAddr, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
    
}

/*
 * Encode a database subject record
 */
static SECStatus
EncodeDBSubjectEntry(certDBEntrySubject *entry, PRArenaPool *arena,
		     SECItem *dbitem)
{
    unsigned char *buf;
    int len;
    unsigned int ncerts;
    unsigned int i;
    unsigned char *tmpbuf;
    unsigned int nnlen = 0;
    unsigned int eaddrlen = 0;
    int keyidoff;
    
    if ( entry->nickname ) {
	nnlen = PORT_Strlen(entry->nickname) + 1;
    }
    if ( entry->emailAddr ) {
	eaddrlen = PORT_Strlen(entry->emailAddr) + 1;
    }
    
    ncerts = entry->ncerts;
    
    /* compute the length of the entry */
    keyidoff = DB_SUBJECT_ENTRY_HEADER_LEN + nnlen + eaddrlen;
    len = keyidoff + 4 * ncerts;
    for ( i = 0; i < ncerts; i++ ) {
	len += entry->certKeys[i].len;
	len += entry->keyIDs[i].len;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = len + SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( ncerts >> 8 ) & 0xff;
    buf[1] = ncerts & 0xff;
    buf[2] = ( nnlen >> 8 ) & 0xff;
    buf[3] = nnlen & 0xff;
    buf[4] = ( eaddrlen >> 8 ) & 0xff;
    buf[5] = eaddrlen & 0xff;

    XP_MEMCPY(&buf[DB_SUBJECT_ENTRY_HEADER_LEN], entry->nickname, nnlen);
    XP_MEMCPY(&buf[DB_SUBJECT_ENTRY_HEADER_LEN+nnlen], entry->emailAddr,
	      eaddrlen);
    
    for ( i = 0; i < ncerts; i++ ) {
	buf[keyidoff+i*2] = ( entry->certKeys[i].len >> 8 ) & 0xff;
	buf[keyidoff+1+i*2] = entry->certKeys[i].len & 0xff;
	buf[keyidoff+ncerts*2+i*2] = ( entry->keyIDs[i].len >> 8 ) & 0xff;
	buf[keyidoff+1+ncerts*2+i*2] = entry->keyIDs[i].len & 0xff;
    }
    
    /* temp pointer used to stuff certkeys and keyids into the buffer */
    tmpbuf = &buf[keyidoff+ncerts*4];

    for ( i = 0; i < ncerts; i++ ) {
	PORT_Memcpy(tmpbuf, entry->certKeys[i].data, entry->certKeys[i].len);
	tmpbuf = tmpbuf + entry->certKeys[i].len;
    }
    
    for ( i = 0; i < ncerts; i++ ) {
	PORT_Memcpy(tmpbuf, entry->keyIDs[i].data, entry->keyIDs[i].len);
	tmpbuf = tmpbuf + entry->keyIDs[i].len;
    }

    PORT_Assert(tmpbuf == &buf[len]);
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Encode a database key for a subject record
 */
static SECStatus
EncodeDBSubjectKey(SECItem *derSubject, PRArenaPool *arena,
		   SECItem *dbkey)
{
    dbkey->len = derSubject->len + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN], derSubject->data,
	      derSubject->len);
    dbkey->data[0] = certDBEntryTypeSubject;

    return(SECSuccess);

loser:
    return(SECFailure);
}

static SECStatus
DecodeDBSubjectEntry(certDBEntrySubject *entry, SECItem *dbentry,
		     SECItem *derSubject)
{
    unsigned int ncerts;
    PRArenaPool *arena;
    unsigned int len, itemlen;
    unsigned char *tmpbuf;
    unsigned int i;
    SECStatus rv;
    unsigned int keyidoff;
    unsigned int nnlen, eaddrlen;
    
    arena = entry->common.arena;

    rv = SECITEM_CopyItem(arena, &entry->derSubject, derSubject);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* is record long enough for header? */
    if ( dbentry->len < DB_SUBJECT_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    entry->ncerts = ncerts = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    nnlen = ( ( dbentry->data[2] << 8 ) | dbentry->data[3] );
    eaddrlen = ( ( dbentry->data[4] << 8 ) | dbentry->data[5] );
    if ( dbentry->len < ( ncerts * 4 + DB_SUBJECT_ENTRY_HEADER_LEN +
			 nnlen + eaddrlen) ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    entry->certKeys = (SECItem *)PORT_ArenaAlloc(arena,
						 sizeof(SECItem) * ncerts);
    entry->keyIDs = (SECItem *)PORT_ArenaAlloc(arena,
					       sizeof(SECItem) * ncerts);

    if ( ( entry->certKeys == NULL ) || ( entry->keyIDs == NULL ) ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    if ( nnlen > 1 ) { /* null terminator is stored */
	entry->nickname = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( entry->nickname == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->nickname,
		    &dbentry->data[DB_SUBJECT_ENTRY_HEADER_LEN],
		    nnlen);
    } else {
	entry->nickname = NULL;
    }
    
    if ( eaddrlen > 1 ) { /* null terminator is stored */
	entry->emailAddr = (char *)PORT_ArenaAlloc(arena, eaddrlen);
	if ( entry->emailAddr == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->emailAddr,
		    &dbentry->data[DB_SUBJECT_ENTRY_HEADER_LEN+nnlen],
		    eaddrlen);
    } else {
	entry->emailAddr = NULL;
    }
    
    /* collect the lengths of the certKeys and keyIDs, and total the
     * overall length.
     */
    keyidoff = DB_SUBJECT_ENTRY_HEADER_LEN + nnlen + eaddrlen;
    len = keyidoff + 4 * ncerts;

    tmpbuf = &dbentry->data[0];
    
    for ( i = 0; i < ncerts; i++ ) {

	itemlen = ( tmpbuf[keyidoff + 2*i] << 8 ) | tmpbuf[keyidoff + 1 + 2*i] ;
	len += itemlen;
	entry->certKeys[i].len = itemlen;

	itemlen = ( tmpbuf[keyidoff + 2*ncerts + 2*i] << 8 ) |
	    tmpbuf[keyidoff + 1 + 2*ncerts + 2*i] ;
	len += itemlen;
	entry->keyIDs[i].len = itemlen;
    }
    
    /* is database entry correct length? */
    if ( len != dbentry->len ){
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    tmpbuf = &tmpbuf[keyidoff + 4*ncerts];
    for ( i = 0; i < ncerts; i++ ) {
	entry->certKeys[i].data =
	    (unsigned char *)PORT_ArenaAlloc(arena, entry->certKeys[i].len);
	if ( entry->certKeys[i].data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->certKeys[i].data, tmpbuf, entry->certKeys[i].len);
	tmpbuf = &tmpbuf[entry->certKeys[i].len];
    }

    for ( i = 0; i < ncerts; i++ ) {
	entry->keyIDs[i].data =
	    (unsigned char *)PORT_ArenaAlloc(arena, entry->keyIDs[i].len);
	if ( entry->keyIDs[i].data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->keyIDs[i].data, tmpbuf, entry->keyIDs[i].len);
	tmpbuf = &tmpbuf[entry->keyIDs[i].len];
    }
    
    PORT_Assert(tmpbuf == &dbentry->data[dbentry->len]);
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new subject entry with a single cert
 */
static certDBEntrySubject *
NewDBSubjectEntry(SECItem *derSubject, SECItem *certKey,
		  SECItem *keyID, char *nickname, char *emailAddr,
		  unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntrySubject *entry;
    SECStatus rv;
    unsigned int i;
    unsigned int nnlen;
    unsigned int eaddrlen;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntrySubject *)PORT_ArenaAlloc(arena,
						  sizeof(certDBEntrySubject));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* init common fields */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSubject;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    /* copy the subject */
    rv = SECITEM_CopyItem(arena, &entry->derSubject, derSubject);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    entry->ncerts = 1;
    /* copy nickname */
    if ( nickname && ( *nickname != '\0' ) ) {
	nnlen = PORT_Strlen(nickname) + 1;
	entry->nickname = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( entry->nickname == NULL ) {
	    goto loser;
	}
						  
	PORT_Memcpy(entry->nickname, nickname, nnlen);
    } else {
	entry->nickname = NULL;
    }
    
    /* copy email addr */
    if ( emailAddr && ( *emailAddr != '\0' ) ) {
	emailAddr = CERT_FixupEmailAddr(emailAddr);
	if ( emailAddr == NULL ) {
	    entry->emailAddr = NULL;
	    goto loser;
	}
	
	eaddrlen = PORT_Strlen(emailAddr) + 1;
	entry->emailAddr = (char *)PORT_ArenaAlloc(arena, eaddrlen);
	if ( entry->emailAddr == NULL ) {
	    PORT_Free(emailAddr);
	    goto loser;
	}
	
	PORT_Memcpy(entry->emailAddr, emailAddr, eaddrlen);
	PORT_Free(emailAddr);
    } else {
	entry->emailAddr = NULL;
    }
    
    /* allocate space for certKeys and keyIDs */
    entry->certKeys = (SECItem *)PORT_ArenaAlloc(arena, sizeof(SECItem));
    entry->keyIDs = (SECItem *)PORT_ArenaAlloc(arena, sizeof(SECItem));
    if ( ( entry->certKeys == NULL ) || ( entry->keyIDs == NULL ) ) {
	goto loser;
    }

    /* copy the certKey and keyID */
    rv = SECITEM_CopyItem(arena, &entry->certKeys[0], certKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    rv = SECITEM_CopyItem(arena, &entry->keyIDs[0], keyID);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * delete a subject entry
 */
static SECStatus
DeleteDBSubjectEntry(CERTCertDBHandle *handle, SECItem *derSubject)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBSubjectKey(derSubject, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = DeleteDBEntry(handle, certDBEntryTypeSubject, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read the subject entry
 */
static certDBEntrySubject *
ReadDBSubjectEntry(CERTCertDBHandle *handle, SECItem *derSubject)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntrySubject *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntrySubject *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntrySubject));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSubject;

    rv = EncodeDBSubjectKey(derSubject, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = DecodeDBSubjectEntry(entry, &dbentry, derSubject);
    if ( rv == SECFailure ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a subject name entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBSubjectEntry(CERTCertDBHandle *handle, certDBEntrySubject *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBSubjectEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = EncodeDBSubjectKey(&entry->derSubject, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
    
}

static SECStatus
UpdateSubjectWithEmailAddr(CERTCertificate *cert, char *emailAddr)
{
    CERTSubjectList *subjectList;
    PRBool save = PR_FALSE, delold = PR_FALSE;
    certDBEntrySubject *entry;
    SECStatus rv;
    
    emailAddr = CERT_FixupEmailAddr(emailAddr);
    if ( emailAddr == NULL ) {
	return(SECFailure);
    }
    
    subjectList = cert->subjectList;
    PORT_Assert(subjectList != NULL);
    
    if ( subjectList->emailAddr ) {
	if ( PORT_Strcmp(subjectList->emailAddr, emailAddr) != 0 ) {
	    save = PR_TRUE;
	    delold = PR_TRUE;
	}
    } else {
	save = PR_TRUE;
    }

    if ( delold ) {
	/* delete the old smime entry, because this cert now has a new
	 * smime entry pointing to it
	 */
	PORT_Assert(save);
	PORT_Assert(subjectList->emailAddr != NULL);
	DeleteDBSMimeEntry(cert->dbhandle, subjectList->emailAddr);
    }

    if ( save ) {
	unsigned int len;
	
	entry = subjectList->entry;

	PORT_Assert(entry != NULL);
	len = PORT_Strlen(emailAddr) + 1;
	entry->emailAddr = (char *)PORT_ArenaAlloc(entry->common.arena, len);
	if ( entry->emailAddr == NULL ) {
	    goto loser;
	}
	PORT_Memcpy(entry->emailAddr, emailAddr, len);
	
	/* delete the subject entry */
	DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);

	/* write the new one */
	rv = WriteDBSubjectEntry(cert->dbhandle, entry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }

    PORT_Free(emailAddr);
    return(SECSuccess);

loser:
    PORT_Free(emailAddr);
    return(SECFailure);
}

/*
 * create a new version entry
 */
static certDBEntryVersion *
NewDBVersionEntry(unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntryVersion *entry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntryVersion *)PORT_ArenaAlloc(arena,
					       sizeof(certDBEntryVersion));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeVersion;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Read the version entry
 */
static certDBEntryVersion *
ReadDBVersionEntry(CERTCertDBHandle *handle)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryVersion *entry;
    SECItem dbkey;
    SECItem dbentry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryVersion *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntryVersion));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeVersion;

    /* now get the database key and format it */
    dbkey.len = SEC_DB_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_VERSION_KEY,
	      SEC_DB_VERSION_KEY_LEN);

    ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);

    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}


/*
 * Encode a version entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBVersionEntry(CERTCertDBHandle *handle, certDBEntryVersion *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem.len = SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbitem.len);
    if ( dbitem.data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* now get the database key and format it */
    dbkey.len = SEC_DB_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_VERSION_KEY,
	      SEC_DB_VERSION_KEY_LEN);

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}

/*
 * create a new version entry
 */
static certDBEntryContentVersion *
NewDBContentVersionEntry(unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntryContentVersion *entry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntryContentVersion *)
	PORT_ArenaAlloc(arena, sizeof(certDBEntryContentVersion));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeContentVersion;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    entry->contentVersion = CERT_DB_CONTENT_VERSION;
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Read the version entry
 */
static certDBEntryContentVersion *
ReadDBContentVersionEntry(CERTCertDBHandle *handle)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryContentVersion *entry;
    SECItem dbkey;
    SECItem dbentry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryContentVersion *)
	PORT_ArenaAlloc(arena, sizeof(certDBEntryContentVersion));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeContentVersion;

    /* now get the database key and format it */
    dbkey.len = SEC_DB_CONTENT_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_CONTENT_VERSION_KEY,
		SEC_DB_CONTENT_VERSION_KEY_LEN);

    ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);

    if ( dbentry.len != 1 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    entry->contentVersion = dbentry.data[0];
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a version entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBContentVersionEntry(CERTCertDBHandle *handle,
			   certDBEntryContentVersion *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem.len = SEC_DB_ENTRY_HEADER_LEN + 1;
    
    dbitem.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbitem.len);
    if ( dbitem.data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    dbitem.data[SEC_DB_ENTRY_HEADER_LEN] = entry->contentVersion;
    
    /* now get the database key and format it */
    dbkey.len = SEC_DB_CONTENT_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_CONTENT_VERSION_KEY,
		SEC_DB_CONTENT_VERSION_KEY_LEN);

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}

/*
 * delete a content version entry
 */
static SECStatus
DeleteDBContentVersionEntry(CERTCertDBHandle *handle)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    /* now get the database key and format it */
    dbkey.len = SEC_DB_CONTENT_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_CONTENT_VERSION_KEY,
		SEC_DB_CONTENT_VERSION_KEY_LEN);
    
    rv = DeleteDBEntry(handle, certDBEntryTypeContentVersion, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Routines and datastructures to manage the list of certificates for a
 * particular subject name.
 */

/*
 * Create a new certificate subject list.  If entry exists, then populate
 * the list with the entries from the permanent database.
 */
CERTSubjectList *
NewSubjectList(certDBEntrySubject *entry)
{
    PRArenaPool *permarena;
    unsigned int i;
    CERTSubjectList *subjectList;
    CERTSubjectNode *node;
    SECStatus rv;
    unsigned int nnlen;
    
    permarena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( permarena == NULL ) {
	goto loser;
    }
    subjectList = (CERTSubjectList *)PORT_ArenaAlloc(permarena,
						     sizeof(CERTSubjectList));
    if ( subjectList == NULL ) {
	goto loser;
    }

    subjectList->arena = permarena;
    subjectList->ncerts = 0;
    subjectList->head = NULL;
    subjectList->tail = NULL;
    subjectList->entry = entry;
    subjectList->emailAddr = NULL;
    if ( entry ) {
	
	/* initialize the list with certs from database entry */
	for ( i = 0; i < entry->ncerts; i++ ) {
	    /* Init the node */
	    node = (CERTSubjectNode *)PORT_ArenaAlloc(permarena,
						      sizeof(CERTSubjectNode));
	    if ( node == NULL ) {
		goto loser;
	    }

	    /* copy certKey and keyID to node */
	    rv = SECITEM_CopyItem(permarena, &node->certKey,
				  &entry->certKeys[i]);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	    rv = SECITEM_CopyItem(permarena, &node->keyID,
				  &entry->keyIDs[i]);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }

	    /* the certs are already in order, so just add them
	     * to the tail.
	     */
	    node->next = NULL;
	    if ( subjectList->tail == NULL ) {
		/* first in list */
		subjectList->head = node;
		subjectList->tail = node;
		node->prev = NULL;
	    } else {
		/* add to end of list */
		node->prev = subjectList->tail;
		subjectList->tail = node;
		node->prev->next = node;
	    }
	    subjectList->ncerts++;
	}
    }
    
    return(subjectList);

loser:
    PORT_FreeArena(permarena, PR_FALSE);
    return(NULL);
}

/*
 * Find the Subject entry in the temp database.  It it is not in the
 * temp database, then get it from the perm DB.  It its not there either,
 * then create a new one.
 */
CERTSubjectList *
FindSubjectList(CERTCertDBHandle *handle, SECItem *subject, PRBool create)
{
    PRArenaPool *arena = NULL;
    SECItem keyitem;
    SECStatus rv;
    DBT namekey;
    DBT tmpdata;
    int ret;
    CERTSubjectList *subjectList = NULL;
    certDBEntrySubject *entry;
    PRArenaPool *permarena = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBSubjectKey(subject, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    namekey.data = keyitem.data;
    namekey.size = keyitem.len;
    
    /* lookup in the temporary database */
    ret = (* handle->tempCertDB->get)(handle->tempCertDB, &namekey,
				      &tmpdata, 0);

    /* error accessing the database */
    if ( ret < 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    if ( ret == 0 ) {	/* found in temp database */
	if ( tmpdata.size != sizeof(CERTCertificate *) ) {
	    PORT_SetError(SEC_ERROR_BAD_DATABASE);
	    goto loser;
	}

	/* copy pointer out of database */
	PORT_Memcpy(&subjectList, tmpdata.data, tmpdata.size);
    } else {		/* not found in temporary database */
	entry = ReadDBSubjectEntry(handle, subject);
	if ( entry || create ) {
	    /* decode or create new subject list */
	    subjectList = NewSubjectList(entry);

	    /* put it in the temp database */
	    if ( subjectList ) {
		tmpdata.data = (unsigned char *)(&subjectList);
		tmpdata.size = sizeof(subjectList);
		ret = (* handle->tempCertDB->put)(handle->tempCertDB, &namekey,
						  &tmpdata, R_NOOVERWRITE);
		if ( ret ) {
		    goto loser;
		}
	    }
	}
    }

    goto done;

loser:
    subjectList = NULL;
    
done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(subjectList);
}

/*
 * Add a temp cert to the temp subject list
 */
SECStatus
AddTempCertToSubjectList(CERTCertificate *cert)
{
    CERTSubjectList *subjectList;
    CERTSubjectNode *node, *newnode;
    CERTCertificate *cmpcert;
    PRBool newer;
    SECStatus rv;
    
    PORT_Assert(cert->isperm == PR_FALSE);
    PORT_Assert(cert->subjectList == NULL);
    
    subjectList = FindSubjectList(cert->dbhandle, &cert->derSubject, PR_TRUE);
    
    if ( subjectList == NULL ) {
	goto loser;
    }

    newnode = (CERTSubjectNode*)PORT_ArenaAlloc(subjectList->arena,
						sizeof(CERTSubjectNode));
    /* copy certKey and keyID to node */
    rv = SECITEM_CopyItem(subjectList->arena, &newnode->certKey,
			  &cert->certKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    rv = SECITEM_CopyItem(subjectList->arena, &newnode->keyID,
			  &cert->subjectKeyID);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    node = subjectList->head;

    if ( node ) {
	/* list is not empty */
	while ( node ) {
	    cmpcert = CERT_FindCertByKey(cert->dbhandle, &node->certKey);
	    if ( cmpcert ) {
		
		newer =  CERT_IsNewer(cert, cmpcert);
		CERT_DestroyCertificate(cmpcert);
		if ( newer ) {
		    /* insert before this cert */
		    newnode->next = node;
		    newnode->prev = node->prev;
		    if ( newnode->prev ) {
			newnode->prev->next = newnode;
		    } else {
			/* at the head of the list */
			subjectList->head = newnode;
		    }
		    node->prev = newnode;
		    goto done;
		}
	    }
	    node = node->next;
	}
	/* if we get here, we add the node to the end of the list */
	newnode->prev = subjectList->tail;
	newnode->next = NULL;
	subjectList->tail->next = newnode;
	subjectList->tail = newnode;
    } else {
	/* this is a new/empty list */
	newnode->next = NULL;
	newnode->prev = NULL;
	subjectList->head = newnode;
	subjectList->tail = newnode;
    }
    
done:
    subjectList->ncerts++;
    cert->subjectList = subjectList;
    return(SECSuccess);
    
loser:
    return(SECFailure);
}

/*
 * Find the node in a subjectList that belongs a cert
 */
CERTSubjectNode *
FindCertSubjectNode(CERTCertificate *cert)
{
    CERTSubjectList *subjectList;
    CERTSubjectNode *node = NULL;
    
    PORT_Assert(cert->subjectList);
    
    subjectList = cert->subjectList;
    
    if ( subjectList ) {
	node = subjectList->head;
    }
    
    while ( node ) {
	if ( SECITEM_CompareItem(&node->certKey, &cert->certKey) == SECEqual ){
	    return(node);
	    break;
	}
	
	node = node->next;
    }

    return(NULL);
}

/*
 * Remove a temp cert from the temp subject list
 */
SECStatus
RemoveTempCertFromSubjectList(CERTCertificate *cert)
{
    CERTSubjectList *subjectList;
    CERTSubjectNode *node;
    SECItem keyitem;
    DBT namekey;
    SECStatus rv;
    int ret;
    CERTCertDBHandle *handle;
    
    PORT_Assert(cert->subjectList);

    /* don't remove perm certs */
    if ( cert->isperm ) {
	return(SECSuccess);
    }
    
    subjectList = cert->subjectList;

    node = FindCertSubjectNode(cert);
    
    if ( node ) {
	/* found it, unlink it */
	if ( node->next ) {
	    node->next->prev = node->prev;
	} else {
	    /* removing from tail of list */
	    subjectList->tail = node->prev;
	}
	if ( node->prev ) {
	    node->prev->next = node->next;
	} else {
	    /* removing from head of list */
	    subjectList->head = node->next;
	}

	subjectList->ncerts--;
	
	/* dont need to free the node, because it is from subjectList
	 * arena.
	 */
	
	/* remove reference from cert */
	cert->subjectList = NULL;
	
	/* if the list is now empty, remove the list from the db and free it */
	if ( subjectList->head == NULL ) {
	    PORT_Assert(subjectList->ncerts == 0);
	    rv = EncodeDBSubjectKey(&cert->derSubject, subjectList->arena,
				    &keyitem);
	    if ( rv == SECSuccess ) {
		namekey.data = keyitem.data;
		namekey.size = keyitem.len;

		handle = cert->dbhandle;
		
		ret = (* handle->tempCertDB->del)(handle->tempCertDB,
						  &namekey, 0);
		/* keep going if it fails */

		if ( cert->dbnickname ) {
		    rv = SEC_DeleteTempNickname(handle, cert->dbnickname);
		} else if ( cert->nickname ) {
		    rv = SEC_DeleteTempNickname(handle, cert->nickname);
		}
		
		/* keep going if it fails */
	    }

	    PORT_FreeArena(subjectList->arena, PR_FALSE);
	}
    }

    PORT_Assert(cert->subjectList == NULL);
    
    if ( cert->subjectList != NULL ) {
	return(SECFailure);
    }

    return(SECSuccess);
}

/*
 * cert is no longer a perm cert, but will remain a temp cert
 */
SECStatus
RemovePermSubjectNode(CERTCertificate *cert)
{
    CERTSubjectList *subjectList;
    certDBEntrySubject *entry;
    unsigned int i;
    SECStatus rv;

    PORT_Assert(cert->isperm);
    if ( !cert->isperm ) {
	return(SECFailure);
    }
    
    subjectList = cert->subjectList;
    PORT_Assert(subjectList);
    if ( subjectList == NULL ) {
	return(SECFailure);
    }
    entry = subjectList->entry;
    PORT_Assert(entry);
    if ( entry == NULL ) {
	return(SECFailure);
    }

    PORT_Assert(entry->ncerts);
    rv = SECFailure;
    
    if ( entry->ncerts > 1 ) {
	for ( i = 0; i < entry->ncerts; i++ ) {
	    if ( SECITEM_CompareItem(&entry->certKeys[i], &cert->certKey) ==
		SECEqual ) {
		/* copy rest of list forward one entry */
		for ( i = i + 1; i < entry->ncerts; i++ ) {
		    entry->certKeys[i-1] = entry->certKeys[i];
		    entry->keyIDs[i-1] = entry->keyIDs[i];
		}
		entry->ncerts--;
		DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);
		rv = WriteDBSubjectEntry(cert->dbhandle, entry);
		break;
	    }
	}
    } else {
	/* no entries left, delete the perm entry in the DB */
	if ( subjectList->entry->emailAddr ) {
	    /* if the subject had an email record, then delete it too */
	    DeleteDBSMimeEntry(cert->dbhandle, subjectList->entry->emailAddr);
	}
	
	DestroyDBEntry((certDBEntry *)subjectList->entry);
	subjectList->entry = NULL;
	DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);
    }

    return(rv);
}

/*
 * add a cert to the perm subject list
 */
static SECStatus
AddPermSubjectNode(CERTCertificate *cert, char *nickname)
{
    CERTSubjectList *subjectList;
    certDBEntrySubject *entry;
    SECItem *newCertKeys, *newKeyIDs;
    int i;
    SECStatus rv;
    CERTCertificate *cmpcert;
    unsigned int nnlen;
    int ncerts;
    
    subjectList = cert->subjectList;
    
    PORT_Assert(subjectList);
    if ( subjectList == NULL ) {
	return(SECFailure);
    }

    entry = subjectList->entry;
    
    if ( entry ) {
	ncerts = entry->ncerts;
	
	if ( nickname && entry->nickname ) {
	    /* nicknames must be the same */
	    PORT_Assert(PORT_Strcmp(nickname, entry->nickname) == 0);
	}

	if ( ( entry->nickname == NULL ) && ( nickname != NULL ) ) {
	    /* copy nickname into the entry */
	    nnlen = PORT_Strlen(nickname) + 1;
	    entry->nickname = (char *)PORT_ArenaAlloc(entry->common.arena,
						      nnlen);
	    if ( entry->nickname == NULL ) {
		return(SECFailure);
	    }
	    PORT_Memcpy(entry->nickname, nickname, nnlen);
	}
	
	/* a DB entry already exists, so add this cert */
	newCertKeys = (SECItem *)PORT_ArenaAlloc(entry->common.arena,
						 sizeof(SECItem) *
						 ( ncerts + 1 ) );
	newKeyIDs = (SECItem *)PORT_ArenaAlloc(entry->common.arena,
					       sizeof(SECItem) *
					       ( ncerts + 1 ) );

	if ( ( newCertKeys == NULL ) || ( newKeyIDs == NULL ) ) {
	    return(SECFailure);
	}

	for ( i = 0; i < ncerts; i++ ) {
	    cmpcert = CERT_FindCertByKey(cert->dbhandle, &entry->certKeys[i]);
	    PORT_Assert(cmpcert);
	    
	    if ( CERT_IsNewer(cert, cmpcert) ) {
		/* insert before cmpcert */
		rv = SECITEM_CopyItem(entry->common.arena, &newCertKeys[i],
				      &cert->certKey);
		if ( rv != SECSuccess ) {
		    return(SECFailure);
		}
		rv = SECITEM_CopyItem(entry->common.arena, &newKeyIDs[i],
				      &cert->subjectKeyID);
		if ( rv != SECSuccess ) {
		    return(SECFailure);
		}
		/* copy the rest of the entry */
		for ( ; i < ncerts; i++ ) {
		    newCertKeys[i+1] = entry->certKeys[i];
		    newKeyIDs[i+1] = entry->keyIDs[i];
		}

		/* update certKeys and keyIDs */
		entry->certKeys = newCertKeys;
		entry->keyIDs = newKeyIDs;
		
		/* increment count */
		entry->ncerts++;
		break;
	    }
	    /* copy this cert entry */
	    newCertKeys[i] = entry->certKeys[i];
	    newKeyIDs[i] = entry->keyIDs[i];
	}

	if ( entry->ncerts == ncerts ) {
	    /* insert new one at end */
	    rv = SECITEM_CopyItem(entry->common.arena, &newCertKeys[ncerts],
				  &cert->certKey);
	    if ( rv != SECSuccess ) {
		return(SECFailure);
	    }
	    rv = SECITEM_CopyItem(entry->common.arena, &newKeyIDs[ncerts],
				  &cert->subjectKeyID);
	    if ( rv != SECSuccess ) {
		return(SECFailure);
	    }

	    /* update certKeys and keyIDs */
	    entry->certKeys = newCertKeys;
	    entry->keyIDs = newKeyIDs;
		
	    /* increment count */
	    entry->ncerts++;
	}
    } else {
	/* need to make a new DB entry */
	entry = NewDBSubjectEntry(&cert->derSubject, &cert->certKey,
				  &cert->subjectKeyID, nickname,
				  NULL, 0);
	cert->subjectList->entry = entry;
    }
    if ( entry ) {
	DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);
	rv = WriteDBSubjectEntry(cert->dbhandle, entry);
    } else {
	rv = SECFailure;
    }
    
    return(rv);
}

SECStatus
CERT_TraversePermCertsForSubject(CERTCertDBHandle *handle, SECItem *derSubject,
				 CERTCertCallback cb, void *cbarg)
{
    certDBEntrySubject *entry;
    int i;
    CERTCertificate *cert;
    SECStatus rv;
    
    entry = ReadDBSubjectEntry(handle, derSubject);

    if ( entry == NULL ) {
	return(SECFailure);
    }
    
    for( i = 0; i < entry->ncerts; i++ ) {
	cert = CERT_FindCertByKey(handle, &entry->certKeys[i]);
	rv = (* cb)(cert, cbarg);
	CERT_DestroyCertificate(cert);
	if ( rv == SECFailure ) {
	    break;
	}
    }

    DestroyDBEntry((certDBEntry *)entry);

    return(rv);
}

int
CERT_NumPermCertsForSubject(CERTCertDBHandle *handle, SECItem *derSubject)
{
    certDBEntrySubject *entry;
    int ret;
    
    entry = ReadDBSubjectEntry(handle, derSubject);

    if ( entry == NULL ) {
	return(SECFailure);
    }

    ret = entry->ncerts;
    
    DestroyDBEntry((certDBEntry *)entry);
    
    return(ret);
}

SECStatus
CERT_TraversePermCertsForNickname(CERTCertDBHandle *handle, char *nickname,
				  CERTCertCallback cb, void *cbarg)
{
    certDBEntryNickname *nnentry = NULL;
    certDBEntrySMime *smentry = NULL;
    SECStatus rv;
    SECItem *derSubject = NULL;
    
    nnentry = ReadDBNicknameEntry(handle, nickname);
    if ( nnentry ) {
	derSubject = &nnentry->subjectName;
    } else {
	smentry = ReadDBSMimeEntry(handle, nickname);
	if ( smentry ) {
	    derSubject = &smentry->subjectName;
	}
    }
    
    if ( derSubject ) {
	rv = CERT_TraversePermCertsForSubject(handle, derSubject,
					      cb, cbarg);
    } else {
	rv = SECFailure;
    }

    if ( nnentry ) {
	DestroyDBEntry((certDBEntry *)nnentry);
    }
    if ( smentry ) {
	DestroyDBEntry((certDBEntry *)smentry);
    }
    
    return(rv);
}

int
CERT_NumPermCertsForNickname(CERTCertDBHandle *handle, char *nickname)
{
    certDBEntryNickname *entry;
    int ret;
    
    entry = ReadDBNicknameEntry(handle, nickname);
    
    if ( entry ) {
	ret = CERT_NumPermCertsForSubject(handle, &entry->subjectName);
	DestroyDBEntry((certDBEntry *)entry);
    } else {
	ret = 0;
    }
    return(ret);
}

int
CERT_NumCertsForCertSubject(CERTCertificate *cert)
{
    int ret = 0;
    
    if ( cert->subjectList ) {
	ret = cert->subjectList->ncerts;
    }
    return(ret);
}

int
CERT_NumPermCertsForCertSubject(CERTCertificate *cert)
{
    int ret = 0;
    
    if ( cert->subjectList ) {
	if ( cert->subjectList->entry ) {
	    ret = cert->subjectList->entry->ncerts;
	}
    }
    return(ret);
}

SECStatus
CERT_TraverseCertsForSubject(CERTCertDBHandle *handle,
			     CERTSubjectList *subjectList,
			     CERTCertCallback cb, void *cbarg)
{
    CERTSubjectNode *node;
    CERTCertificate *cert;
    SECStatus rv;
    
    node = subjectList->head;
    while ( node ) {

	cert = CERT_FindCertByKey(handle, &node->certKey);

	PORT_Assert(cert != NULL);

	if ( cert != NULL ) {
	    rv = (* cb)(cert, cbarg);
	    CERT_DestroyCertificate(cert);
	    if ( rv == SECFailure ) {
		break;
	    }
	}

	node = node->next;
    }

    return(rv);
}

static certDBEntryCert *
AddCertToPermDB(CERTCertDBHandle *handle, CERTCertificate *cert,
		char *nickname, CERTCertTrust *trust)
{
    certDBEntryCert *certEntry = NULL;
    certDBEntryNickname *nicknameEntry = NULL;
    certDBEntrySubject *subjectEntry = NULL;
    int state = 0;
    SECStatus rv;
    PRBool donnentry = PR_FALSE;

    if ( nickname ) {
	donnentry = PR_TRUE;
    }
	
    if ( cert->subjectList != NULL ) {
	if ( cert->subjectList->entry != NULL ) {
	    if ( cert->subjectList->entry->ncerts > 0 ) {
		/* of other certs with same subject exist, then they already
		 * have a nickname, so don't add a new one.
		 */
		donnentry = PR_FALSE;
		nickname = cert->subjectList->entry->nickname;
	    }
	}
    }
    
    certEntry = NewDBCertEntry(&cert->derCert, nickname, trust, 0);
    if ( certEntry == NULL ) {
	goto loser;
    }
    
    if ( donnentry ) {
	nicknameEntry = NewDBNicknameEntry(nickname, &cert->derSubject, 0);
	if ( nicknameEntry == NULL ) {
	    goto loser;
	}
    }
    
    rv = WriteDBCertEntry(handle, certEntry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    state = 1;
    
    if ( nicknameEntry ) {
	rv = WriteDBNicknameEntry(handle, nicknameEntry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    
    state = 2;
    
    /* add to or create new subject entry */
    if ( cert->subjectList ) {
	rv = AddPermSubjectNode(cert, nickname);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	/* make a new subject entry - this case is only used when updating
	 * an old version of the database.  This is OK because the oldnickname
	 * db format didn't allow multiple certs with the same subject.
	 */
	subjectEntry = NewDBSubjectEntry(&cert->derSubject, &cert->certKey,
					 &cert->subjectKeyID, nickname,
					 NULL, 0);
	if ( subjectEntry == NULL ) {
	    goto loser;
	}
	rv = WriteDBSubjectEntry(handle, subjectEntry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    
    state = 3;
    
    if ( nicknameEntry ) {
	DestroyDBEntry((certDBEntry *)nicknameEntry);
    }
    
    if ( subjectEntry ) {
	DestroyDBEntry((certDBEntry *)subjectEntry);
    }

    return(certEntry);

loser:
    /* don't leave partial entry in the database */
    if ( state > 0 ) {
	rv = DeleteDBCertEntry(handle, &cert->certKey);
    }
    if ( ( state > 1 ) && donnentry ) {
	rv = DeleteDBNicknameEntry(handle, nickname);
    }
    if ( state > 2 ) {
	rv = DeleteDBSubjectEntry(handle, &cert->derSubject);
    }
    if ( certEntry ) {
	DestroyDBEntry((certDBEntry *)certEntry);
    }
    if ( nicknameEntry ) {
	DestroyDBEntry((certDBEntry *)nicknameEntry);
    }
    if ( subjectEntry ) {
	DestroyDBEntry((certDBEntry *)subjectEntry);
    }

    return(NULL);
}

/*
 * NOTE - Version 6 DB did not go out to the real world in a release,
 * so we can remove this function in a post-dogbert release.
 * If I get this checked in for B3, then we can remove it for B4.
 */
static SECStatus
UpdateV6DB(CERTCertDBHandle *handle, DB *updatedb)
{
    int ret;
    DBT key, data;
    unsigned char *buf, *tmpbuf = NULL;
    certDBEntryType type;
    certDBEntryNickname *nnEntry = NULL;
    certDBEntrySubject *subjectEntry = NULL;
    certDBEntrySMime *emailEntry = NULL;
    char *nickname;
    char *emailAddr;
    SECStatus rv;
    
    /*
     * Sequence through the old database and copy all of the entries
     * to the new database.  Subject name entries will have the new
     * fields inserted into them (with zero length).
     */
    ret = (* updatedb->seq)(updatedb, &key, &data, R_FIRST);
    if ( ret ) {
	return(SECFailure);
    }

    do {
	buf = (unsigned char *)data.data;
	
	if ( data.size >= 3 ) {
	    if ( buf[0] == 6 ) { /* version number */
		type = buf[1];
		if ( type == certDBEntryTypeSubject ) {
		    /* expando subjecto entrieo */
		    tmpbuf = (unsigned char *)PORT_Alloc(data.size + 4);
		    if ( tmpbuf ) {
			/* copy header stuff */
			PORT_Memcpy(tmpbuf, buf, SEC_DB_ENTRY_HEADER_LEN + 2);
			/* insert 4 more bytes of zero'd header */
			PORT_Memset(&tmpbuf[SEC_DB_ENTRY_HEADER_LEN + 2],
				    0, 4);
			/* copy rest of the data */
			PORT_Memcpy(&tmpbuf[SEC_DB_ENTRY_HEADER_LEN + 6],
				    &buf[SEC_DB_ENTRY_HEADER_LEN + 2],
				    data.size - (SEC_DB_ENTRY_HEADER_LEN + 2));

			data.data = (void *)tmpbuf;
			data.size += 4;
			buf = tmpbuf;
		    }
		} else if ( type == certDBEntryTypeCert ) {
		    /* expando certo entrieo */
		    tmpbuf = (unsigned char *)PORT_Alloc(data.size + 3);
		    if ( tmpbuf ) {
			/* copy header stuff */
			PORT_Memcpy(tmpbuf, buf, SEC_DB_ENTRY_HEADER_LEN);

			/* copy trust flage, setting msb's to 0 */
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN] = 0;
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+1] =
			    buf[SEC_DB_ENTRY_HEADER_LEN];
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+2] = 0;
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+3] =
			    buf[SEC_DB_ENTRY_HEADER_LEN+1];
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+4] = 0;
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+5] =
			    buf[SEC_DB_ENTRY_HEADER_LEN+2];
			
			/* copy rest of the data */
			PORT_Memcpy(&tmpbuf[SEC_DB_ENTRY_HEADER_LEN + 6],
				    &buf[SEC_DB_ENTRY_HEADER_LEN + 3],
				    data.size - (SEC_DB_ENTRY_HEADER_LEN + 3));

			data.data = (void *)tmpbuf;
			data.size += 3;
			buf = tmpbuf;
		    }

		}

		/* update the record version number */
		buf[0] = CERT_DB_FILE_VERSION;

		/* copy to the new database */
		ret = (* handle->permCertDB->put)(handle->permCertDB, &key,
						  &data, 0);
		if ( tmpbuf ) {
		    PORT_Free(tmpbuf);
		    tmpbuf = NULL;
		}
	    }
	}
    } while ( (* updatedb->seq)(updatedb, &key, &data, R_NEXT) == 0 );

    ret = (* handle->permCertDB->sync)(handle->permCertDB, 0);

    ret = (* updatedb->seq)(updatedb, &key, &data, R_FIRST);
    if ( ret ) {
	return(SECFailure);
    }

    do {
	buf = (unsigned char *)data.data;
	
	if ( data.size >= 3 ) {
	    if ( buf[0] == CERT_DB_FILE_VERSION ) { /* version number */
		type = buf[1];
		if ( type == certDBEntryTypeNickname ) {
		    nickname = &((char *)key.data)[1];

		    /* get the matching nickname entry in the new DB */
		    nnEntry = ReadDBNicknameEntry(handle, nickname);
		    if ( nnEntry == NULL ) {
			goto endloop;
		    }
		    
		    /* find the subject entry pointed to by nickname */
		    subjectEntry = ReadDBSubjectEntry(handle,
						      &nnEntry->subjectName);
		    if ( subjectEntry == NULL ) {
			goto endloop;
		    }
		    
		    subjectEntry->nickname =
			(char *)PORT_ArenaAlloc(subjectEntry->common.arena,
						key.size - 1);
		    if ( subjectEntry->nickname ) {
			PORT_Memcpy(subjectEntry->nickname, nickname,
				    key.size - 1);
			rv = WriteDBSubjectEntry(handle, subjectEntry);
		    }
		} else if ( type == certDBEntryTypeSMimeProfile ) {
		    emailAddr = &((char *)key.data)[1];

		    /* get the matching smime entry in the new DB */
		    emailEntry = ReadDBSMimeEntry(handle, emailAddr);
		    if ( emailEntry == NULL ) {
			goto endloop;
		    }
		    
		    /* find the subject entry pointed to by nickname */
		    subjectEntry = ReadDBSubjectEntry(handle,
						      &emailEntry->subjectName);
		    if ( subjectEntry == NULL ) {
			goto endloop;
		    }
		    
		    subjectEntry->nickname =
			(char *)PORT_ArenaAlloc(subjectEntry->common.arena,
						key.size - 1);
		    if ( subjectEntry->emailAddr ) {
			PORT_Memcpy(subjectEntry->emailAddr, emailAddr,
				    key.size - 1);
			rv = WriteDBSubjectEntry(handle, subjectEntry);
		    }
		}
		
endloop:
		if ( subjectEntry ) {
		    DestroyDBEntry((certDBEntry *)subjectEntry);
		    subjectEntry = NULL;
		}
		if ( nnEntry ) {
		    DestroyDBEntry((certDBEntry *)nnEntry);
		    nnEntry = NULL;
		}
		if ( emailEntry ) {
		    DestroyDBEntry((certDBEntry *)emailEntry);
		    emailEntry = NULL;
		}
	    }
	}
    } while ( (* updatedb->seq)(updatedb, &key, &data, R_NEXT) == 0 );

    ret = (* handle->permCertDB->sync)(handle->permCertDB, 0);

    (* updatedb->close)(updatedb);
    return(SECSuccess);
}


static SECStatus
updateV5Callback(CERTCertificate *cert, SECItem *k, void *pdata)
{
    CERTCertDBHandle *handle;
    certDBEntryCert *entry;
    CERTCertTrust *trust;
    
    handle = (CERTCertDBHandle *)pdata;
    trust = &cert->dbEntry->trust;

    /* SSL user certs can be used for email if they have an email addr */
    if ( cert->emailAddr && ( trust->sslFlags & CERTDB_USER ) &&
	( trust->emailFlags == 0 ) ) {
	trust->emailFlags = CERTDB_USER;
    }
    
    entry = AddCertToPermDB(handle, cert, cert->dbEntry->nickname,
			    &cert->dbEntry->trust);
    if ( entry ) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    
    return(SECSuccess);
}

static SECStatus
UpdateV5DB(CERTCertDBHandle *handle, DB *updatedb)
{
    CERTCertDBHandle updatehandle;
    SECStatus rv;
    
    updatehandle.permCertDB = updatedb;
    
    rv = SEC_TraversePermCerts(&updatehandle, updateV5Callback,
			       (void *)handle);
    
    return(rv);
}

static SECStatus
UpdateV4DB(CERTCertDBHandle *handle, DB *updatedb)
{
    DBT key, data;
    certDBEntryCert *entry, *entry2;
    SECItem derSubject, certKey;
    int ret;
    SECStatus rv;
    PRArenaPool *arena = NULL;
    CERTCertificate *cert;

    ret = (* updatedb->seq)(updatedb, &key, &data, R_FIRST);

    if ( ret ) {
	return(SECFailure);
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	return(SECFailure);
    }
    
    do {
	if ( data.size != 1 ) { /* skip version number */

	    /* decode the old DB entry */
	    entry = DecodeV4DBCertEntry(data.data, data.size);
	    derSubject.data = NULL;
	    
	    if ( entry ) {
		cert = CERT_DecodeDERCertificate(&entry->derCert, PR_TRUE,
						 entry->nickname);

		if ( cert != NULL ) {
		    /* add to new database */
		    entry2 = AddCertToPermDB(handle, cert, entry->nickname,
					     &entry->trust);
		    
		    CERT_DestroyCertificate(cert);
		    if ( entry2 ) {
			DestroyDBEntry((certDBEntry *)entry2);
		    }
		}
		DestroyDBEntry((certDBEntry *)entry);
	    }
	}
    } while ( (* updatedb->seq)(updatedb, &key, &data, R_NEXT) == 0 );

    PORT_FreeArena(arena, PR_FALSE);
    (* updatedb->close)(updatedb);
    return(SECSuccess);
}

/*
 * return true if a database key conflict exists
 */
PRBool
SEC_CertDBKeyConflict(SECItem *derCert, CERTCertDBHandle *handle)
{
    SECStatus rv;
    DBT tmpdata;
    DBT namekey;
    int ret;
    SECItem keyitem;
    PRArenaPool *arena = NULL;
    SECItem derKey;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    /* get the db key of the cert */
    rv = CERT_KeyFromDERCert(arena, derCert, &derKey);
    if ( rv != SECSuccess ) {
        goto loser;
    }

    rv = EncodeDBCertKey(&derKey, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    namekey.data = keyitem.data;
    namekey.size = keyitem.len;
    
    /* lookup in the temporary database */
    ret = (* handle->tempCertDB->get)(handle->tempCertDB, &namekey,
				     &tmpdata, 0);

    if ( ret == 0 ) {	/* found in temp database */
	goto loser;
    } else {		/* not found in temporary database */
	ret = (* handle->permCertDB->get)(handle->permCertDB, &namekey,
					  &tmpdata, 0);
	if ( ret == 0 ) {
	    goto loser;
	}
    }

    PORT_FreeArena(arena, PR_FALSE);
    
    return(PR_FALSE);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(PR_TRUE);
}

#ifdef NOTDEF
/*
 * return true if a subject name conflict exists
 * NOTE: caller must have already made sure that this exact cert
 * doesn't exist in the DB
 */
PRBool
SEC_CertSubjectConflict(SECItem *derCert, CERTCertDBHandle *handle)
{
    SECStatus rv;
    DBT tmpdata;
    DBT namekey;
    int ret;
    SECItem keyitem;
    PRArenaPool *arena = NULL;
    SECItem derName;
    
    derName.data = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    /* get the subject name of the cert */
    rv = CERT_NameFromDERCert(derCert, &derName);
    if ( rv != SECSuccess ) {
        goto loser;
    }

    rv = EncodeDBSubjectKey(&derName, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    namekey.data = keyitem.data;
    namekey.size = keyitem.len;
    
    /* lookup in the temporary database */
    ret = (* handle->tempCertDB->get)(handle->tempCertDB, &namekey,
				     &tmpdata, 0);

    if ( ret == 0 ) {	/* found in temp database */
	return(PR_TRUE);
    } else {		/* not found in temporary database */
	ret = (* handle->permCertDB->get)(handle->permCertDB, &namekey,
					  &tmpdata, 0);
	if ( ret == 0 ) {
	    return(PR_TRUE);
	}
    }

    PORT_FreeArena(arena, PR_FALSE);
    PORT_Free(derName.data);
    
    return(PR_FALSE);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if ( derName.data ) {
	PORT_Free(derName.data);
    }
    
    return(PR_TRUE);
}
#endif

/*
 * return true if a nickname conflict exists
 * NOTE: caller must have already made sure that this exact cert
 * doesn't exist in the DB
 */
PRBool
SEC_CertNicknameConflict(char *nickname, SECItem *derSubject,
			 CERTCertDBHandle *handle)
{
    SECStatus rv;
    certDBEntryNickname *entry;
    
    if ( nickname == NULL ) {
	return(PR_FALSE);
    }
    
    entry = ReadDBNicknameEntry(handle, nickname);

    if ( entry == NULL ) {
	/* no entry for this nickname, so no conflict */
	return(PR_FALSE);
    }

    rv = PR_TRUE;
    if ( SECITEM_CompareItem(derSubject, &entry->subjectName) == SECEqual ) {
	/* if subject names are the same, then no conflict */
	rv = PR_FALSE;
    }

    DestroyDBEntry((certDBEntry *)entry);
    return(rv);
}

/*
 * Open the certificate database and index databases.  Create them if
 * they are not there or bad.
 */
SECStatus
SEC_OpenPermCertDB(CERTCertDBHandle *handle, PRBool readonly,
		   CERTDBNameFunc namecb, void *cbarg)
{
    SECStatus rv;
    int openflags;
    certDBEntryVersion *versionEntry = NULL;
    DB *updatedb = NULL;
    char *tmpname;
    char *certdbname;
    PRBool updated = PR_FALSE;
    
    certdbname = (* namecb)(cbarg, CERT_DB_FILE_VERSION);
    if ( certdbname == NULL ) {
	return(SECFailure);
    }
    
    if ( readonly ) {
	openflags = O_RDONLY;
    } else {
	openflags = O_RDWR;
    }
    
    /*
     * first open the permanent file based database.
     */
    handle->permCertDB = dbopen( certdbname, openflags, 0600, DB_HASH, 0 );

    /* check for correct version number */
    if ( handle->permCertDB ) {
	versionEntry = ReadDBVersionEntry(handle);

	if ( versionEntry == NULL ) {
	    /* no version number */
	    (* handle->permCertDB->close)(handle->permCertDB);
	    handle->permCertDB = 0;
	} else if ( versionEntry->common.version != CERT_DB_FILE_VERSION ) {
	    /* wrong version number, can't update in place */
	    DestroyDBEntry((certDBEntry *)versionEntry);
	    return(SECFailure);
	}

    }


    /* if first open fails, try to create a new DB */
    if ( handle->permCertDB == NULL ) {

	/* don't create if readonly */
	if ( readonly ) {
	    goto loser;
	}
	
	handle->permCertDB = dbopen(certdbname,
				    O_RDWR | O_CREAT | O_TRUNC,
				    0600, DB_HASH, 0);

	/* if create fails then we lose */
	if ( handle->permCertDB == 0 ) {
	    goto loser;
	}

	versionEntry = NewDBVersionEntry(0);
	if ( versionEntry == NULL ) {
	    goto loser;
	}
	
	rv = WriteDBVersionEntry(handle, versionEntry);

	DestroyDBEntry((certDBEntry *)versionEntry);

	if ( rv != SECSuccess ) {
	    goto loser;
	}

	/* try to upgrade old db here */
	tmpname = (* namecb)(cbarg, 6);	/* get v6 db name */
	if ( tmpname ) {
	    updatedb = dbopen( tmpname, O_RDONLY, 0600, DB_HASH, 0 );
	    PORT_Free(tmpname);
	    if ( updatedb ) {
		rv = UpdateV6DB(handle, updatedb);
		if ( rv != SECSuccess ) {
		    goto loser;
		}
		updated = PR_TRUE;
	    } else { /* no v6 db, so try v5 db */
		tmpname = (* namecb)(cbarg, 5);	/* get v5 db name */
		if ( tmpname ) {
		    updatedb = dbopen( tmpname, O_RDONLY, 0600, DB_HASH, 0 );
		    PORT_Free(tmpname);
		    if ( updatedb ) {
			rv = UpdateV5DB(handle, updatedb);
			if ( rv != SECSuccess ) {
			    goto loser;
			}
			updated = PR_TRUE;
		    } else { /* no v5 db, so try v4 db */
			/* try to upgrade v4 db */
			tmpname = (* namecb)(cbarg, 4);	/* get v4 db name */
			if ( tmpname ) {
			    updatedb = dbopen( tmpname, O_RDONLY, 0600,
					      DB_HASH, 0 );
			    PORT_Free(tmpname);
			    if ( updatedb ) {
				rv = UpdateV4DB(handle, updatedb);
				if ( rv != SECSuccess ) {
				    goto loser;
				}
				updated = PR_TRUE;
			    }
			}
		    }
		}
	    }
	}

	/* initialize the database with our well known certificates
	 * or in the case of update, just fall down to CERT_AddNewCerts()
	 * below.
	 */
	if ( ! updated ) {
	    rv = CERT_InitCertDB(handle);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	}
    }

    rv = CERT_AddNewCerts(handle);
    
    return (SECSuccess);
    
loser:

    PORT_SetError(SEC_ERROR_BAD_DATABASE);
    
    if ( handle->permCertDB ) {
	(* handle->permCertDB->close)(handle->permCertDB);
	handle->permCertDB = 0;
    }

    return(SECFailure);
}

/*
 * delete all DB records associated with a particular certificate
 */
static SECStatus
DeletePermCert(CERTCertificate *cert)
{
    SECStatus rv;
    SECStatus ret;

    ret = SECSuccess;
    
    rv = DeleteDBCertEntry(cert->dbhandle, &cert->certKey);
    if ( rv != SECSuccess ) {
	ret = SECFailure;
    }
    
    if ( cert->nickname ) {
	rv = DeleteDBNicknameEntry(cert->dbhandle, cert->nickname);
	if ( rv != SECSuccess ) {
	    ret = SECFailure;
	}
    }
    
    rv = RemovePermSubjectNode(cert);

    return(ret);
}

/*
 * Delete a certificate from the permanent database.
 */
SECStatus
SEC_DeletePermCertificate(CERTCertificate *cert)
{
    SECStatus rv;
    
    if ( !cert->isperm ) {
	return(SECSuccess);
    }

    /* delete the records from the permanent database */
    rv = DeletePermCert(cert);
    
    /* no longer permanent */
    cert->isperm = PR_FALSE;

    /* get rid of dbcert and stuff pointing to it */
    DestroyDBEntry((certDBEntry *)cert->dbEntry);
    cert->dbEntry = NULL;
    cert->trust = NULL;

    /* delete it from the temporary database too.  It will remain in
     * memory until all references go away.
     */
    rv = CERT_DeleteTempCertificate(cert);

    return(rv);
}

/*
 * Lookup a certificate in the databases.
 */
certDBEntryCert *
SEC_FindPermCertByKey(CERTCertDBHandle *handle, SECItem *key)
{
    return(ReadDBCertEntry(handle, key));
}

/*
 * Lookup a certificate in the database by name
 */
certDBEntryCert *
SEC_FindPermCertByName(CERTCertDBHandle *handle, SECItem *derSubject)
{
    certDBEntrySubject *subjectEntry;
    certDBEntryCert *certEntry;
    
    subjectEntry = ReadDBSubjectEntry(handle, derSubject);
    
    if ( subjectEntry == NULL ) {
	goto loser;
    }

    certEntry = ReadDBCertEntry(handle, &subjectEntry->certKeys[0]);
    DestroyDBEntry((certDBEntry *)subjectEntry);
    
    return(certEntry);

loser:
    return(NULL);
}

/*
 * Lookup a certificate in the database by nickname
 */
certDBEntryCert *
SEC_FindPermCertByNickname(CERTCertDBHandle *handle, char *nickname)
{
    certDBEntryNickname *nicknameEntry;
    certDBEntryCert *certEntry;
    
    nicknameEntry = ReadDBNicknameEntry(handle, nickname);
    
    if ( nicknameEntry == NULL ) {
	goto loser;
    }

    certEntry = SEC_FindPermCertByName(handle, &nicknameEntry->subjectName);
    DestroyDBEntry((certDBEntry *)nicknameEntry);
    
    return(certEntry);

loser:
    return(NULL);
}

/*
 * Traverse all of the entries in the database of a particular type
 * call the given function for each one.
 */
SECStatus
SEC_TraverseDBEntries(CERTCertDBHandle *handle,
		      certDBEntryType type,
		      SECStatus (* callback)(SECItem *data, SECItem *key,
					    certDBEntryType type, void *pdata),
		      void *udata )
{
    DBT data;
    DBT key;
    SECStatus rv;
    int ret;
    SECItem dataitem;
    SECItem keyitem;
    unsigned char *buf;
    unsigned char *keybuf;
    
    ret = (* handle->permCertDB->seq)(handle->permCertDB, &key, &data, R_FIRST);

    if ( ret ) {
	return(SECFailure);
    }
    
    do {
	buf = (unsigned char *)data.data;
	
	if ( buf[1] == (unsigned char)type ) {
	    dataitem.len = data.size;
	    dataitem.data = buf;
	    keyitem.len = key.size - SEC_DB_KEY_HEADER_LEN;
	    keybuf = (unsigned char *)key.data;
	    keyitem.data = &keybuf[SEC_DB_KEY_HEADER_LEN];
	    
	    rv = (* callback)(&dataitem, &keyitem, type, udata);
	    if ( rv != SECSuccess ) {
		return(rv);
	    }
	}
    } while ( (* handle->permCertDB->seq)(handle->permCertDB, &key,
					  &data, R_NEXT) == 0 );

    return(SECSuccess);
}

typedef struct {
    PermCertCallback certfunc;
    void *data;
} PermCertCallbackState;

/*
 * traversal callback to decode certs and call callers callback
 */
static SECStatus
certcallback(SECItem *dbdata, SECItem *dbkey, certDBEntryType type, void *data)
{
    PermCertCallbackState *mystate;
    SECStatus rv;
    certDBEntryCert entry;
    SECItem entryitem;
    CERTCertificate *cert;
    PRArenaPool *arena = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    mystate = (PermCertCallbackState *)data;
    entry.common.version = (unsigned int)dbdata->data[0];
    entry.common.type = (certDBEntryType)dbdata->data[1];
    entry.common.flags = (unsigned int)dbdata->data[2];
    entry.common.arena = arena;
    
    entryitem.len = dbdata->len - SEC_DB_ENTRY_HEADER_LEN;
    entryitem.data = &dbdata->data[SEC_DB_ENTRY_HEADER_LEN];
    
    rv = DecodeDBCertEntry(&entry, &entryitem);
    if (rv != SECSuccess ) {
	goto loser;
    }
    
    cert = CERT_DecodeDERCertificate(&entry.derCert, PR_FALSE,
				    entry.nickname);
    cert->dbEntry = &entry;
    cert->trust = &entry.trust;

    rv = (* mystate->certfunc)(cert, dbkey, mystate->data);

    /* arena destroyed by SEC_DestroyCert */
    CERT_DestroyCertificate(cert);

    return(rv);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return(SECFailure);
}

/*
 * Traverse all of the certificates in the permanent database and
 * call the given function for each one.
 */
SECStatus
SEC_TraversePermCerts(CERTCertDBHandle *handle,
		      SECStatus (* certfunc)(CERTCertificate *cert, SECItem *k,
					    void *pdata),
		      void *udata )
{
    SECStatus rv;
    PermCertCallbackState mystate;

    mystate.certfunc = certfunc;
    mystate.data = udata;
    
    rv = SEC_TraverseDBEntries(handle, certDBEntryTypeCert, certcallback,
			       (void *)&mystate);
    
    return(rv);
}

/*
 * Close the database
 */
void
CERT_ClosePermCertDB(CERTCertDBHandle *handle)
{
    int rv;
    
    if ( handle ) {
	if ( handle->permCertDB ) {
	    rv = (* handle->permCertDB->close)( handle->permCertDB );
	}
    }
    return;
}

/*
 * Get the trust attributes from a certificate
 */
SECStatus
CERT_GetCertTrust(CERTCertificate *cert, CERTCertTrust *trust)
{
    if ( cert->trust == NULL ) {
	return(SECFailure);
    }

    *trust = *cert->trust;
    
    return(SECSuccess);
}

/*
 * Change the trust attributes of a certificate and make them permanent
 * in the database.
 */
SECStatus
CERT_ChangeCertTrust(CERTCertDBHandle *handle, CERTCertificate *cert,
		    CERTCertTrust *trust)
{
    certDBEntryCert *entry;
    int rv;
    
    /* only set the trust on permanent certs */
    if ( cert->trust == NULL ) {
	return(SECFailure);
    }

    *cert->trust = *trust;
    if ( cert->dbEntry == NULL ) {
	return(SECSuccess); /* not in permanent database */
    }
    
    entry = cert->dbEntry;
    entry->trust = *trust;
    
    rv = WriteDBCertEntry(handle, entry);
    if ( rv ) {
	goto loser;
    }

    return(SECSuccess);
    
loser:
    return(SECFailure);
}

typedef struct stringNode {
    struct stringNode *next;
    char *string;
} stringNode;
    
static SECStatus
CollectNicknames( CERTCertificate *cert, SECItem *k, void *data)
{
    CERTCertNicknames *names;
    PRBool saveit = PR_FALSE;
    CERTCertTrust *trust;
    stringNode *node;
    int len;
    
    names = (CERTCertNicknames *)data;
    
    if ( cert->dbEntry && cert->dbEntry->nickname ) {
	trust = &cert->dbEntry->trust;
	
	switch(names->what) {
	  case SEC_CERT_NICKNAMES_ALL:
	    if ( ( trust->sslFlags & (CERTDB_VALID_CA|CERTDB_VALID_PEER) ) ||
	      ( trust->emailFlags & (CERTDB_VALID_CA|CERTDB_VALID_PEER) ) ||
	      ( trust->objectSigningFlags & (CERTDB_VALID_CA|CERTDB_VALID_PEER) ) ) {
		saveit = PR_TRUE;
	    }
	    
	    break;
	  case SEC_CERT_NICKNAMES_USER:
	    if ( ( trust->sslFlags & CERTDB_USER ) ||
		( trust->emailFlags & CERTDB_USER ) ||
		( trust->objectSigningFlags & CERTDB_USER ) ) {
		saveit = PR_TRUE;
	    }
	    
	    break;
	  case SEC_CERT_NICKNAMES_SERVER:
	    if ( trust->sslFlags & CERTDB_VALID_PEER ) {
		saveit = PR_TRUE;
	    }
	    
	    break;
	  case SEC_CERT_NICKNAMES_CA:
	    if ( ( ( trust->sslFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
		( ( trust->emailFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
		( ( trust->objectSigningFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ) {
		saveit = PR_TRUE;
	    }
	    break;
	}
    }

    if ( saveit ) {
	/* allocate the node */
	node = PORT_ArenaAlloc(names->arena, sizeof(stringNode));
	if ( node == NULL ) {
	    return(SECFailure);
	}

	/* copy the string */
	len = PORT_Strlen(cert->dbEntry->nickname) + 1;
	node->string = PORT_ArenaAlloc(names->arena, len);
	if ( node->string == NULL ) {
	    return(SECFailure);
	}
	PORT_Memcpy(node->string, cert->dbEntry->nickname, len);

	/* link it into the list */
	node->next = (stringNode *)names->head;
	names->head = (void *)node;

	/* bump the count */
	names->numnicknames++;
    }
    
    return(SECSuccess);
}


CERTCertNicknames *
CERT_GetCertNicknames(CERTCertDBHandle *handle, int what, void *wincx)
{
    PRArenaPool *arena;
    CERTCertNicknames *names;
    int i;
    SECStatus rv;
    stringNode *node;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return(NULL);
    }
    
    names = PORT_ArenaAlloc(arena, sizeof(CERTCertNicknames));
    if ( names == NULL ) {
	goto loser;
    }

    names->arena = arena;
    names->head = NULL;
    names->numnicknames = 0;
    names->nicknames = NULL;
    names->what = what;
    names->totallen = 0;
    
    rv = SEC_TraversePermCerts(handle, CollectNicknames, (void *)names);
    if ( rv ) {
	goto loser;
    }

    if ( wincx != NULL ) {
	rv = PK11_TraverseSlotCerts(CollectNicknames, (void *)names, wincx);
	if ( rv ) {
	    goto loser;
	}
    }

    if ( names->numnicknames ) {
	names->nicknames = PORT_ArenaAlloc(arena,
					 names->numnicknames * sizeof(char *));

	if ( names->nicknames == NULL ) {
	    goto loser;
	}
    
	node = (stringNode *)names->head;
	
	for ( i = 0; i < names->numnicknames; i++ ) {
	    PORT_Assert(node != NULL);
	    
	    names->nicknames[i] = node->string;
	    names->totallen += PORT_Strlen(node->string);
	    node = node->next;
	}

	PORT_Assert(node == NULL);
    }

    return(names);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(NULL);
}

void
CERT_FreeNicknames(CERTCertNicknames *nicknames)
{
    PORT_FreeArena(nicknames->arena, PR_FALSE);
    
    return;
}

typedef struct dnameNode {
    struct dnameNode *next;
    SECItem name;
} dnameNode;

void
CERT_FreeDistNames(CERTDistNames *names)
{
    PORT_FreeArena(names->arena, PR_FALSE);
    
    return;
}

static SECStatus
CollectDistNames( CERTCertificate *cert, SECItem *k, void *data)
{
    CERTDistNames *names;
    PRBool saveit = PR_FALSE;
    CERTCertTrust *trust;
    dnameNode *node;
    int len;
    
    names = (CERTDistNames *)data;
    
    if ( cert->trust ) {
	trust = cert->trust;
	
	if ( ( ( trust->sslFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
	    ( ( trust->emailFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
	    ( ( trust->objectSigningFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ){
	    saveit = PR_TRUE;
	}
    }

    if ( saveit ) {
	/* allocate the node */
	node = PORT_ArenaAlloc(names->arena, sizeof(dnameNode));
	if ( node == NULL ) {
	    return(SECFailure);
	}

	/* copy the name */
	node->name.len = len = cert->derSubject.len;
	node->name.data = PORT_ArenaAlloc(names->arena, len);
	if ( node->name.data == NULL ) {
	    return(SECFailure);
	}
	PORT_Memcpy(node->name.data, cert->derSubject.data, len);

	/* link it into the list */
	node->next = (dnameNode *)names->head;
	names->head = (void *)node;

	/* bump the count */
	names->nnames++;
    }
    
    return(SECSuccess);
}

/*
 * Return all of the CAs that are "trusted" for SSL.
 */
CERTDistNames *
CERT_GetSSLCACerts(CERTCertDBHandle *handle)
{
    PRArenaPool *arena;
    CERTDistNames *names;
    int i;
    SECStatus rv;
    dnameNode *node;
    
    /* allocate an arena to use */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return(NULL);
    }
    
    /* allocate the header structure */
    names = (CERTDistNames *)PORT_ArenaAlloc(arena, sizeof(CERTDistNames));
    if ( names == NULL ) {
	goto loser;
    }

    /* initialize the header struct */
    names->arena = arena;
    names->head = NULL;
    names->nnames = 0;
    names->names = NULL;
    
    /* collect the names from the database */
    rv = SEC_TraversePermCerts(handle, CollectDistNames, (void *)names);
    if ( rv ) {
	goto loser;
    }

    /* construct the array from the list */
    if ( names->nnames ) {
	names->names = PORT_ArenaAlloc(arena, names->nnames * sizeof(SECItem));

	if ( names->names == NULL ) {
	    goto loser;
	}
    
	node = (dnameNode *)names->head;
	
	for ( i = 0; i < names->nnames; i++ ) {
	    PORT_Assert(node != NULL);
	    
	    names->names[i] = node->name;
	    node = node->next;
	}

	PORT_Assert(node == NULL);
    }

    return(names);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(NULL);
}

CERTDistNames *
CERT_DistNamesFromNicknames(CERTCertDBHandle *handle, char **nicknames,
			   int nnames)
{
    CERTDistNames *dnames = NULL;
    PRArenaPool *arena;
    int i, rv;
    SECItem *names = NULL;
    CERTCertificate *cert = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto loser;
    dnames = PORT_Alloc(sizeof(CERTDistNames));
    if (dnames == NULL) goto loser;

    dnames->arena = arena;
    dnames->nnames = nnames;
    dnames->names = names = PORT_Alloc(nnames * sizeof(SECItem));
    if (names == NULL) goto loser;
    
    for (i = 0; i < nnames; i++) {
	cert = CERT_FindCertByNicknameOrEmailAddr(handle, nicknames[i]);
	if (cert == NULL) goto loser;
	rv = SECITEM_CopyItem(arena, &names[i], &cert->derSubject);
	if (rv == SECFailure) goto loser;
	CERT_DestroyCertificate(cert);
    }
    return dnames;
    
loser:
    if (cert != NULL)
	CERT_DestroyCertificate(cert);
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}

SECStatus
CERT_AddTempCertToPerm(CERTCertificate *cert, char *nickname,
		       CERTCertTrust *trust)
{
    char *oldnn;
    certDBEntryCert *entry;
    SECStatus rv;
    SECItem cpdata;	/* database item containing certificate pointer */
    PRBool conflict;
    
    PORT_Assert(cert->istemp);
    PORT_Assert(!cert->isperm);
    PORT_Assert(cert->dbhandle);
    PORT_Assert(!cert->dbEntry);

    /* don't add a conflicting nickname */
    conflict = SEC_CertNicknameConflict(nickname, &cert->derSubject,
					cert->dbhandle);
    if ( conflict ) {
	return(SECFailure);
    }
    
    /* save old nickname so that we can delete it */
    oldnn = cert->nickname;

    entry = AddCertToPermDB(cert->dbhandle, cert, nickname, trust);
    
    if ( entry == NULL ) {
	return(SECFailure);
    }
    
    cert->nickname = entry->nickname;
    cert->trust = &entry->trust;
    cert->isperm = PR_TRUE;
    cert->dbEntry = entry;

    if ( nickname && oldnn && ( PORT_Strcmp(nickname, oldnn) != 0 ) ) {
	/* only delete the old one if they are not the same */
	/* delete old nickname from temp database */
	rv = SEC_DeleteTempNickname(cert->dbhandle, oldnn);
	if ( rv != SECSuccess ) {
	    /* do we care?? */
	}
    }
    
    /* add new nickname to temp database */
    if ( cert->nickname ) {
	rv = SEC_AddTempNickname(cert->dbhandle, cert->nickname,
				 &cert->derSubject);
	if ( rv != SECSuccess ) {
	    return(SECFailure);
	}
    }
    
    return(SECSuccess);
}

/*
 * Open the certificate database and index databases.  Create them if
 * they are not there or bad.
 */
SECStatus
CERT_OpenCertDB(CERTCertDBHandle *handle, PRBool readonly,
		CERTDBNameFunc namecb, void *cbarg)
{
    int rv;
    
    /*
     * Open the memory resident decoded cert database.
     */
    handle->tempCertDB = dbopen( 0, O_RDWR | O_CREAT, 0600, DB_HASH, 0 );
    if ( !handle->tempCertDB ) {
	goto loser;
    }

    rv = SEC_OpenPermCertDB(handle, readonly, namecb, cbarg);
    if ( rv ) {
	goto loser;
    }

    return (SECSuccess);
    
loser:

    PORT_SetError(SEC_ERROR_BAD_DATABASE);
    
    if ( handle->tempCertDB ) {
	(* handle->tempCertDB->close)(handle->tempCertDB);
	handle->tempCertDB = 0;
    }

    return(SECFailure);
}

static char *
certDBFilenameCallback(void *arg, int dbVersion)
{
    return((char *)arg);
}

SECStatus
CERT_OpenCertDBFilename(CERTCertDBHandle *handle, char *certdbname,
			PRBool readonly)
{
    return(CERT_OpenCertDB(handle, readonly, certDBFilenameCallback,
			   (void *)certdbname));
}

/*
 * Add a nickname to the temp database
 */
SECStatus
SEC_AddTempNickname(CERTCertDBHandle *handle, char *nickname,
		    SECItem *subjectName)
{
    DBT namekey;
    int ret;
    SECItem nameitem;
    SECStatus rv;
    DBT keydata;
    PRArenaPool *arena = NULL;
    SECItem tmpitem;
    
    PORT_Assert(nickname != NULL);
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    rv = EncodeDBNicknameKey(nickname, arena, &nameitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    namekey.data = nameitem.data;
    namekey.size = nameitem.len;

    /* see if an entry already exists */
    ret = (* handle->tempCertDB->get)(handle->tempCertDB, &namekey,
				     &keydata, 0);

    if ( ret == 0 ) {
	/* found in temp database */
	tmpitem.data = keydata.data;
	tmpitem.len = keydata.size;

	if ( SECITEM_CompareItem(subjectName, &tmpitem) == SECEqual ) {
	    /* same subject name */
	    goto done;
	} else {
	    /* different subject name is an error */
	    goto loser;
	}
    }
    
    keydata.data = subjectName->data;
    keydata.size = subjectName->len;
    
    /* put into temp byname index */
    ret = (* handle->tempCertDB->put)(handle->tempCertDB, &namekey,
				     &keydata, R_NOOVERWRITE);

    if ( ret ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

done:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return(SECFailure);
}

SECStatus
SEC_DeleteTempNickname(CERTCertDBHandle *handle, char *nickname)
{
    DBT namekey;
    SECStatus rv;
    PRArenaPool *arena = NULL;
    SECItem nameitem;
    int ret;
    
    PORT_Assert(nickname != NULL);
    if ( nickname == NULL ) {
	return(SECSuccess);
    }
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* format a database key based on the nickname */
    if ( nickname ) {
	rv = EncodeDBNicknameKey(nickname, arena, &nameitem);
	if ( rv != SECSuccess ) {
	    goto loser;
	}

	namekey.data = nameitem.data;
	namekey.size = nameitem.len;

	ret = (* handle->tempCertDB->del)(handle->tempCertDB,
						   &namekey, 0);
	if ( ret ) {
	    goto loser;
	}
    }

    PORT_FreeArena(arena, PR_FALSE);
    
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return(SECFailure);
}

/*
 * Decode a certificate and enter it into the temporary certificate database.
 * Deal with nicknames correctly
 *
 * nickname is only used if isperm == PR_TRUE
 */
CERTCertificate *
CERT_NewTempCertificate(CERTCertDBHandle *handle, SECItem *derCert,
			char *nickname, PRBool isperm, PRBool copyDER)
{
    DBT key;
    DBT data;
    int status;
    CERTCertificate *cert = NULL;
    PRBool promoteError = PR_TRUE;
    PRArenaPool *arena = NULL;
    SECItem keyitem, dataitem;
    SECStatus rv;
    PRBool conflict;
    
    if ( isperm == PR_FALSE ) {
	cert = CERT_FindCertByDERCert(handle, derCert);
	if ( cert ) {
	    return(cert);
	}

	nickname = NULL;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    cert = CERT_DecodeDERCertificate(derCert, copyDER, nickname );
    
    if ( cert == NULL ) {
	/* We want to save the decoding error here */
	promoteError = PR_FALSE;
	goto loser;
    }

    cert->dbhandle = handle;

    /* only save pointer to cert in database */
    data.data = &cert;
    data.size = sizeof(cert);

    /* if this is a perm cert, then it is already in the subject db */
    if ( isperm == PR_FALSE ) {
	/* enter into the subject index */
	rv = AddTempCertToSubjectList(cert);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	cert->subjectList = FindSubjectList(cert->dbhandle, &cert->derSubject,
					    PR_FALSE);
    }
    
    rv = EncodeDBCertKey(&cert->certKey, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    key.data = keyitem.data;
    key.size = keyitem.len;
    
    /* enter into main db */
    status = (* handle->tempCertDB->put)(handle->tempCertDB, &key,
					 &data, R_NOOVERWRITE);
    if ( status ) {
	goto loser;
    }

    if ( cert->nickname ) {
	status = SEC_AddTempNickname(handle, cert->nickname,
				     &cert->derSubject);
	if ( status ) {
	    promoteError = PR_FALSE;
	    goto loser;
	}
    }

    cert->isperm = isperm;
    cert->istemp = PR_TRUE;
    
    PORT_FreeArena(arena, PR_FALSE);
    return(cert);

loser:
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    if ( promoteError ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
    }
    return(0);
}

/*
 * Decode a permanent certificate and enter it into the temporary certificate
 * database.
 */
static CERTCertificate *
SEC_AddPermCertToTemp(CERTCertDBHandle *handle, certDBEntryCert *entry)
{
    CERTCertificate *cert;


    cert = CERT_NewTempCertificate(handle, &entry->derCert, entry->nickname,
				  PR_TRUE, PR_TRUE);
    if ( !cert ) {
	return(0);
    }

    cert->dbEntry = entry;

    cert->trust = &entry->trust;
    
    return(cert);
}

SECStatus
CERT_DeleteTempCertificate(CERTCertificate *cert)
{
    SECStatus rv;
    DBT nameKey;
    CERTCertDBHandle *handle;
    SECItem keyitem;
    PRArenaPool *arena;
    int ret;
    
    handle = cert->dbhandle;

    if ( !cert->istemp ) {
	return(SECSuccess);
    }
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
   
    if (cert->slot) {
	PK11_FreeSlot(cert->slot);
	cert->slot = NULL;
	cert->pkcs11ID = CK_INVALID_KEY;
    }
    
    /* delete from subject list (also takes care of nickname */
    rv = RemoveTempCertFromSubjectList(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = EncodeDBCertKey(&cert->certKey, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    nameKey.data = keyitem.data;
    nameKey.size = keyitem.len;

    /* delete the cert */
    ret = (* handle->tempCertDB->del)(handle->tempCertDB,
					       &nameKey, 0);
    if ( ret ) {
	goto loser;
    }

    cert->istemp = PR_FALSE;

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    return(SECFailure);
}

/*
 * Lookup a certificate in the databases.
 */
CERTCertificate *
CERT_FindCertByKey(CERTCertDBHandle *handle, SECItem *certKey)
{
    DBT tmpdata;
    int ret;
    SECItem keyitem;
    DBT key;
    SECStatus rv;
    CERTCertificate *cert = NULL;
    PRArenaPool *arena = NULL;
    certDBEntryCert *entry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBCertKey(certKey, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    key.data = keyitem.data;
    key.size = keyitem.len;
    
    /* lookup in the temporary database */
    ret = (* handle->tempCertDB->get)( handle->tempCertDB, &key, &tmpdata, 0 );

    /* error accessing the database */
    if ( ret < 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    if ( ret == 0 ) {	/* found in temp database */
	if ( tmpdata.size != sizeof(CERTCertificate *) ) {
	    PORT_SetError(SEC_ERROR_BAD_DATABASE);
	    goto loser;
	}
	
	PORT_Memcpy(&cert, tmpdata.data, tmpdata.size);
	cert->referenceCount++;
    } else {		/* not found in temporary database */

	/* find in perm database */
	entry = SEC_FindPermCertByKey(handle, certKey);
	
	if ( entry == NULL ) {
	    goto loser;
	}
    
	cert = SEC_AddPermCertToTemp(handle, entry);
    }

    return(cert);

loser:
    return(0);
}

/*
 * Generate a key from an issuerAndSerialNumber, and find the
 * associated cert in the database.
 */
CERTCertificate *
CERT_FindCertByIssuerAndSN(CERTCertDBHandle *handle, CERTIssuerAndSN *issuerAndSN)
{
    SECItem certKey;
    CERTCertificate *cert;
    
    certKey.len = issuerAndSN->serialNumber.len + issuerAndSN->derIssuer.len;
    certKey.data = PORT_Alloc(certKey.len);
    
    if ( certKey.data == NULL ) {
	return(0);
    }

    /* copy the serialNumber */
    PORT_Memcpy(certKey.data, issuerAndSN->serialNumber.data,
	      issuerAndSN->serialNumber.len);

    /* copy the issuer */
    PORT_Memcpy( &certKey.data[issuerAndSN->serialNumber.len],
	      issuerAndSN->derIssuer.data, issuerAndSN->derIssuer.len);

    cert = CERT_FindCertByKey(handle, &certKey);
    
    PORT_Free(certKey.data);
    
    return(cert);
}

/*
 * Lookup a certificate in the database by name
 */
CERTCertificate *
CERT_FindCertByName(CERTCertDBHandle *handle, SECItem *name)
{
    CERTCertificate *cert = NULL;
    CERTSubjectList *subjectList;
    
    subjectList = FindSubjectList(handle, name, PR_FALSE);

    if ( subjectList ) {
	PORT_Assert(subjectList->head);
	cert = CERT_FindCertByKey(handle, &subjectList->head->certKey);
    }

    return(cert);
}

/*
 * Lookup a certificate in the database by name
 */
CERTCertificate *
CERT_FindCertByNameString(CERTCertDBHandle *handle, char *nameStr)
{
    CERTName *name;
    SECItem *nameItem;
    CERTCertificate *cert = NULL;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( arena == NULL ) {
	goto loser;
    }
    
    name = CERT_AsciiToName(nameStr);
    
    if ( name ) {
	nameItem = SEC_ASN1EncodeItem (arena, NULL, (void *)name,
				       CERT_NameTemplate);
	if ( nameItem == NULL ) {
	    goto loser;
	}

	cert = CERT_FindCertByName(handle, nameItem);
	CERT_DestroyName(name);
    }

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(cert);
}

/*
 * Lookup a certificate in the database by name and key ID
 */
CERTCertificate *
CERT_FindCertByKeyID(CERTCertDBHandle *handle, SECItem *name, SECItem *keyID)
{
    CERTCertificate *cert = NULL;
    CERTSubjectList *subjectList;
    CERTSubjectNode *node;

    /* find the list of certs for the given subject */
    subjectList = FindSubjectList(handle, name, PR_FALSE);

    if ( subjectList ) {
	PORT_Assert(subjectList->head);
	node = subjectList->head;

	/* walk through the certs until we find one with a matching key ID */
	while ( node ) {
	    if ( SECITEM_CompareItem(keyID, &node->keyID) == SECEqual ) {
		cert = CERT_FindCertByKey(handle, &node->certKey);
	    }
	    node = node->next;
	}
    }

    return(cert);
}

/*
 * look up a cert by its nickname string
 */
CERTCertificate *
CERT_FindCertByNickname(CERTCertDBHandle *handle, char *nickname)
{
    DBT tmpdata;
    DBT namekey;
    CERTCertificate *cert;
    SECStatus rv;
    int ret;
    SECItem keyitem;
    PRArenaPool *arena = NULL;
    certDBEntryCert *entry;
    
    PORT_Assert(nickname != NULL);
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBNicknameKey(nickname, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    namekey.data = keyitem.data;
    namekey.size = keyitem.len;
    
    /* lookup in the temporary database */
    ret = (* handle->tempCertDB->get)(handle->tempCertDB, &namekey,
				     &tmpdata, 0);

    /* error accessing the database */
    if ( ret < 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    if ( ret == 0 ) {	/* found in temp database */
	SECItem nameitem;
	
	nameitem.len = tmpdata.size;
	nameitem.data = (unsigned char *)PORT_Alloc(tmpdata.size);
	if ( nameitem.data == NULL ) {
	    goto loser;
	}
	PORT_Memcpy(nameitem.data, tmpdata.data, nameitem.len);
	cert = CERT_FindCertByName(handle, &nameitem);
	PORT_Free(nameitem.data);
    } else {		/* not found in temporary database */

	entry = SEC_FindPermCertByNickname(handle, nickname);
	
	if ( entry == NULL ) {
	    goto loser;
	}
    
	cert = SEC_AddPermCertToTemp(handle, entry);
    }

    PORT_FreeArena(arena, PR_FALSE);

    return(cert);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(0);
}

/*
 * look for the given DER certificate in the database
 */
CERTCertificate *
CERT_FindCertByDERCert(CERTCertDBHandle *handle, SECItem *derCert)
{
    PRArenaPool *arena;
    SECItem certKey;
    SECStatus rv;
    CERTCertificate *cert = NULL;
    
    /* create a scratch arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return(NULL);
    }
    
    /* extract the database key from the cert */
    rv = CERT_KeyFromDERCert(arena, derCert, &certKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* find the certificate */
    cert = CERT_FindCertByKey(handle, &certKey);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(cert);
}

void
CERT_DestroyCertificate(CERTCertificate *cert)
{
    certDBEntryCert *entry;
    
    if ( cert ) {
	if (--cert->referenceCount <= 0) {
	    if ( !cert->keepSession ) {
		entry = cert->dbEntry;
		if ( cert->istemp ) {
		    CERT_DeleteTempCertificate(cert);
		}
		if ( entry ) {
		    DestroyDBEntry((certDBEntry *)entry);
		}
	
		PORT_FreeArena(cert->arena, PR_FALSE);
	    }
	}
    }

    return;
}


/*
 * cache a CRL from the permanent database into the temporary database
 */
CERTSignedCrl *
SEC_AddPermCrlToTemp(CERTCertDBHandle *handle, certDBEntryRevocation *entry)
{
    CERTSignedCrl *crl = NULL;
    DBT key;
    DBT data;
    int status;
    PRArenaPool *arena = NULL;
    SECItem keyitem;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    crl = CERT_DecodeDERCrl(NULL, &entry->derCrl, 
       entry->common.type == certDBEntryTypeRevocation ? 
						SEC_CRL_TYPE : SEC_KRL_TYPE);
    
    if ( crl == NULL ) {
	goto loser;
    }

    /* only save pointer to cert in database */
    data.data = &crl;
    data.size = sizeof(crl);


    rv = EncodeDBGenericKey(&(crl->crl.derName), arena, 
						&keyitem, entry->common.type);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    if (entry->url) {
	int nnlen = PORT_Strlen(entry->url) + 1;
	crl->url  = (char *)PORT_ArenaAlloc(crl->arena, nnlen);
	if ( !crl->url ) {
	    goto loser;
	}
	PORT_Memcpy(crl->url, entry->url, nnlen);
    } else {
	crl->url = NULL;
    }
    
    key.data = keyitem.data;
    key.size = keyitem.len;
    
    /* enter into main db */
    status = (* handle->tempCertDB->put)(handle->tempCertDB, &key,
					 &data, R_NOOVERWRITE);
    if ( status ) {
	goto loser;
    }

    crl->istemp = PR_TRUE;
    crl->isperm = PR_TRUE;    
    crl->dbhandle = handle;
    crl->dbEntry = entry;

    
    PORT_FreeArena(arena, PR_FALSE);

    crl->keep = PR_TRUE;
    return(crl);

loser:
    if ( crl ) {
	SEC_DestroyCrl(crl);
    }
    
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    PORT_SetError(SEC_ERROR_BAD_DATABASE);
    return(0);
}

SECStatus
SEC_DeleteTempCrl(CERTSignedCrl *crl)
{
    SECStatus rv;
    DBT nameKey;
    CERTCertDBHandle *handle;
    SECItem keyitem;
    PRArenaPool *arena;
    int ret;
    
    handle = crl->dbhandle;

    if ( !crl->istemp ) {
	return(SECSuccess);
    }
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBGenericKey
	 (&crl->crl.derName, arena, &keyitem, certDBEntryTypeRevocation);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    nameKey.data = keyitem.data;
    nameKey.size = keyitem.len;

    /* delete the cert */
    ret = (* handle->tempCertDB->del)(handle->tempCertDB,
					       &nameKey, 0);
    if ( ret ) {
	goto loser;
    }

    crl->istemp = PR_FALSE;

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    return(SECFailure);
}

/*
 * Lookup a CRL in the databases. We mirror the same fast caching data base
 *  caching stuff used by certificates....?
 */
CERTSignedCrl *
SEC_FindCrlByKey(CERTCertDBHandle *handle, SECItem *crlKey, int type)
{
    DBT tmpdata;
    int ret;
    SECItem keyitem;
    DBT key;
    SECStatus rv;
    CERTSignedCrl *crl = NULL;
    PRArenaPool *arena = NULL;
    certDBEntryRevocation *entry;
    certDBEntryType crlType = (type == SEC_CRL_TYPE) ?
	 certDBEntryTypeRevocation : certDBEntryTypeKeyRevocation;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBGenericKey(crlKey, arena, &keyitem, crlType);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    key.data = keyitem.data;
    key.size = keyitem.len;
    
    /* lookup in the temporary database */
    ret = (* handle->tempCertDB->get)( handle->tempCertDB, &key, &tmpdata, 0 );

    /* error accessing the database */
    if ( ret < 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    if ( ret == 0 ) {	/* found in temp database */
	if ( tmpdata.size != sizeof(CERTSignedCrl *) ) {
	    PORT_SetError(SEC_ERROR_BAD_DATABASE);
	    goto loser;
	}
	
	PORT_Memcpy(&crl, tmpdata.data, tmpdata.size);
	crl->referenceCount++;
    } else {		/* not found in temporary database */

	/* find in perm database */
	entry = ReadDBCrlEntry(handle, crlKey, crlType);
	
	if ( entry == NULL ) {
	    goto loser;
	}
    
	crl = SEC_AddPermCrlToTemp(handle, entry);
    }

    return(crl);

loser:
    return(0);
}


CERTSignedCrl *
SEC_FindCrlByName(CERTCertDBHandle *handle, SECItem *crlKey, int type)
{
	return SEC_FindCrlByKey(handle,crlKey,type);
}

CERTSignedCrl *
SEC_FindCrlByDERCert(CERTCertDBHandle *handle, SECItem *derCrl, int type)
{
    PRArenaPool *arena;
    SECItem crlKey;
    SECStatus rv;
    CERTSignedCrl *crl = NULL;
    
    /* create a scratch arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return(NULL);
    }
    
    /* extract the database key from the cert */
    rv = CERT_KeyFromDERCrl(arena, derCrl, &crlKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* find the crl */
    crl = SEC_FindCrlByKey(handle, &crlKey, type);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(crl);
}


SECStatus
SEC_DestroyCrl(CERTSignedCrl *crl)
{
    if (crl) {
	if (crl->referenceCount-- <= 1) {
	    if (!crl->keep) {
		SEC_DeleteTempCrl(crl);
		if (crl->dbEntry) {
		    DestroyDBEntry((certDBEntry *)crl->dbEntry);
		}
		PORT_FreeArena(crl->arena, PR_FALSE);
	    }
	}
    }
    return SECSuccess;
}

CERTSignedCrl *
cert_DBInsertCRL (CERTCertDBHandle *handle, char *url,
		  CERTSignedCrl *newCrl, SECItem *derCrl, int type)
{
    CERTSignedCrl *oldCrl = NULL, *crl = NULL;
    certDBEntryRevocation *entry = NULL;
    PRArenaPool *arena = NULL;
    SECCertTimeValidity validity;
    certDBEntryType crlType = (type == SEC_CRL_TYPE) ?
	 certDBEntryTypeRevocation : certDBEntryTypeKeyRevocation;
    SECStatus rv;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto done;

    validity = SEC_CheckCrlTimes(&newCrl->crl,PR_Now());
    if ( validity == secCertTimeExpired) {
	if (type == SEC_CRL_TYPE) {
	    PORT_SetError(SEC_ERROR_CRL_EXPIRED);
	} else {
	    PORT_SetError(SEC_ERROR_KRL_EXPIRED);
	}
	goto done;
    }

    oldCrl = SEC_FindCrlByKey(handle, &newCrl->crl.derName, type);

    /* if there is an old crl, make sure the one we are installing
     * is newer. If not, exit out, otherwise delete the old crl.
     */
    if (oldCrl != NULL) {
	if (!SEC_CrlIsNewer(&newCrl->crl,&oldCrl->crl)) {
	    PORT_SetError (SEC_ERROR_OLD_CRL);
	    goto done;
	}

	/* if we have a url in the database, use that one */
	if (oldCrl->url) {
	    int nnlen = PORT_Strlen(oldCrl->url) + 1;
	    url  = (char *)PORT_ArenaAlloc(arena, nnlen);
	    if ( !url ) {
	        PORT_Memcpy(url, oldCrl->url, nnlen);
	    }
	}

	/* really destroy this crl */
	/* first drum it out of the permanment Data base */
	rv = DeleteDBCrlEntry(handle, &newCrl->crl.derName, crlType);
	if (rv != SECSuccess) goto done;

	/* now force it to be freed when all the reference counts go */
	oldCrl->keep = PR_FALSE;
	/* force it out of the temporary data base */
	SEC_DeleteTempCrl(oldCrl);
	/* then get rid of our reference to it... */
	SEC_DestroyCrl(oldCrl);
	oldCrl = NULL;
    }

    /* Write the new entry into the data base */
    entry = NewDBCrlEntry(derCrl, url, crlType, 0);
    if (entry == NULL) goto done;

    rv = WriteDBCrlEntry(handle, entry);
    if (rv != SECSuccess) goto done;

    crl = SEC_AddPermCrlToTemp(handle, entry);
    if (crl) entry = NULL;
    crl->isperm = PR_TRUE;

done:
    if (entry) DestroyDBEntry((certDBEntry *)entry);
    if (arena) PORT_FreeArena(arena, PR_FALSE);
    if (oldCrl) SEC_DestroyCrl(oldCrl);

    return crl;
}


/*
 * create a new CRL from DER material.
 *
 * The signature on this CRL must be checked before you
 * load it. ???
 */
CERTSignedCrl *
SEC_NewCrl(CERTCertDBHandle *handle, char *url, SECItem *derCrl, int type)
{
    CERTSignedCrl *newCrl = NULL, *crl = NULL;

    /* make this decode dates! */
    newCrl = CERT_DecodeDERCrl(NULL, derCrl, type);
    if (newCrl == NULL) {
	if (type == SEC_CRL_TYPE) {
	    PORT_SetError(SEC_ERROR_CRL_INVALID);
#ifdef FORTEZZA
	} else {
	    PORT_SetError(SEC_ERROR_KRL_INVALID);
#endif
	}
	goto done;
    }

    crl = cert_DBInsertCRL (handle, url, newCrl, derCrl, type);


done:
    if (newCrl) PORT_FreeArena(newCrl->arena, PR_FALSE);

    return crl;
}

/*
 * collect a linked list of CRL's
 */
static SECStatus CollectCrls(SECItem *dbdata, SECItem *dbkey, 
					certDBEntryType type, void *data) {
    SECStatus rv;
    certDBEntryRevocation entry;
    SECItem entryitem;
    PRArenaPool *arena = NULL;
    CERTCrlHeadNode *head;
    CERTCrlNode *new_node;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    head = (CERTCrlHeadNode *)data;
    entry.common.version = (unsigned int)dbdata->data[0];
    entry.common.type = (certDBEntryType)dbdata->data[1];
    entry.common.flags = (unsigned int)dbdata->data[2];
    entry.common.arena = arena;
    
    entryitem.len = dbdata->len - SEC_DB_ENTRY_HEADER_LEN;
    entryitem.data = &dbdata->data[SEC_DB_ENTRY_HEADER_LEN];
    
    rv = DecodeDBCrlEntry(&entry, &entryitem);
    if (rv != SECSuccess ) {
	goto loser;
    }

    new_node = (CERTCrlNode *)PORT_ArenaAlloc(head->arena, sizeof(CERTCrlNode));
    if (new_node == NULL) {
	goto loser;
    }

    new_node->type = (entry.common.type == certDBEntryTypeRevocation) ? 
						SEC_CRL_TYPE : SEC_KRL_TYPE;
    new_node->crl=CERT_DecodeDERCrl(head->arena,&entry.derCrl,new_node->type);

    if (entry.url) {
	int nnlen = PORT_Strlen(entry.url) + 1;
	new_node->crl->url  = (char *)PORT_ArenaAlloc(head->arena, nnlen);
	if ( !new_node->crl->url ) {
	    goto loser;
	}
	PORT_Memcpy(new_node->crl->url, entry.url, nnlen);
    } else {
	new_node->crl->url = NULL;
    }
    

    new_node->next = NULL;
    if (head->last) {
	head->last->next = new_node;
	head->last = new_node;
    } else {
	head->first = head->last = new_node;
    }
    return (SECSuccess);
    
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return(SECFailure);
}

	
SECStatus
SEC_LookupCrls(CERTCertDBHandle *handle, CERTCrlHeadNode **nodes, int type)
{
    CERTCrlHeadNode *head;
    PRArenaPool *arena = NULL;
    int rv;

    *nodes = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return SECFailure;
    }

    /* build a head structure */
    head = (CERTCrlHeadNode *)PORT_ArenaAlloc(arena, sizeof(CERTCrlHeadNode));
    head->arena = arena;
    head->first = NULL;
    head->last = NULL;


    /* Look up the proper crl types */
    *nodes = head;
    switch (type) {
    case SEC_CRL_TYPE:
	rv = SEC_TraverseDBEntries(handle, certDBEntryTypeRevocation,
						CollectCrls, (void *)head);
	break;
#ifdef FORTEZZA
    case SEC_KRL_TYPE:
	rv = SEC_TraverseDBEntries(handle, certDBEntryTypeKeyRevocation,
					CollectCrls, (void *)head);
	break;
    case -1:
	rv = SEC_TraverseDBEntries(handle, certDBEntryTypeKeyRevocation,
						CollectCrls, (void *)head);
	if (rv != SECSuccess) break;
#else
    case -1:
#endif
	rv = SEC_TraverseDBEntries(handle, certDBEntryTypeRevocation,
					CollectCrls, (void *)head);
	break;
    default:
	rv = SECFailure;
	break;
    }


    if (rv != SECSuccess) {
	if ( arena ) {
	    PORT_FreeArena(arena, PR_FALSE);
	    *nodes = NULL;
	}
    }

    return rv;
}
	
/*
 * Delete a certificate from the permanent database.
 */
SECStatus
SEC_DeletePermCRL(CERTSignedCrl *crl)
{
    SECStatus rv;

    if ( !crl->isperm ) {
	return(SECSuccess);
    }

    PORT_Assert (crl->dbEntry);
    
    /* delete the records from the permanent database */
    rv = DeleteDBCrlEntry(crl->dbhandle, &crl->crl.derName, crl->dbEntry->common.type);

    if (rv == SECSuccess) {
	/* no longer permanent */
	crl->isperm = PR_FALSE;

	/* get rid of dbcert and stuff pointing to it */
	DestroyDBEntry((certDBEntry *)crl->dbEntry);	
	crl->dbEntry = NULL;

	/* delete it from the temporary database too.  It will remain in
	 * memory until all references go away.
	 */
	rv = SEC_DeleteTempCrl(crl);
    }

    return(rv);
}

/*
 * This function has the logic that decides if another person's cert and
 * email profile from an S/MIME message should be saved.  It can deal with
 * the case when there is no profile.
 */
SECStatus
CERT_SaveSMimeProfile(CERTCertificate *cert, SECItem *emailProfile,
		      SECItem *profileTime)
{
    certDBEntrySMime *entry, *oldentry;
    PRBool newer;
    int64 oldtime;
    int64 newtime;
    SECStatus rv;
    CERTCertificate *oldcert = NULL;
    PRBool saveit;
    CERTCertTrust trust;
    CERTCertTrust tmptrust;
    char *emailAddr;
    
    emailAddr = cert->emailAddr;
    
    PORT_Assert(emailAddr);
    if ( emailAddr == NULL ) {
	goto loser;
    }
    
    entry = NULL;
    saveit = PR_FALSE;
    
    oldcert = CERT_FindCertByEmailAddr(cert->dbhandle, emailAddr);
    
    /* both profileTime and emailProfile have to exist or not exist */
    if ( emailProfile == NULL ) {
	profileTime = NULL;
    } else if ( profileTime == NULL ) {
	emailProfile = NULL;
    }
    
    /* see if there is an entry already */
    oldentry = ReadDBSMimeEntry(cert->dbhandle, emailAddr);
    
    if ( oldentry == NULL ) {
	/* no old entry for this address */
	PORT_Assert(oldcert == NULL);
	saveit = PR_TRUE;
    } else {
	/* there was already a profile for this email addr */
	if ( profileTime ) {
	    /* we have an old and new profile - save whichever is more recent*/
	    if ( oldentry->optionsDate.len == 0 ) {
		/* always replace if old entry doesn't have a time */
		oldtime = LL_MININT;
	    } else {
		rv = DER_UTCTimeToTime(&oldtime, &oldentry->optionsDate);
		if ( rv != SECSuccess ) {
		    goto loser;
		}
	    }
	    
	    rv = DER_UTCTimeToTime(&newtime, profileTime);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	
	    if ( LL_CMP(newtime, >, oldtime ) ) {
		/* this is a newer profile, save it and cert */
		saveit = PR_TRUE;
	    }
	} else {
	    /* we don't have a new profile or time */

	    if ( oldentry->optionsDate.len == 0 ) {
		/* the old entry doesn't have a time either, so compare certs*/
		PORT_Assert(oldcert != NULL);
		if ( oldcert == NULL ) {
		    goto loser;
		}
		if ( CERT_IsNewer(cert, oldcert) ) {
		    /* new cert is newer, use it instead */
		    saveit = PR_TRUE;
		}
	    } else {
		/* if the old entry has a time, then don't replace it */
	    }
	}
    }

    if ( saveit ) {
	if ( oldcert && ( oldcert != cert ) ) {
	    /* old cert is different from new cert */
	    if ( PORT_Memcmp(oldcert->trust, &trust, sizeof(trust)) == 0 ) {
		/* old cert is only for e-mail, so delete it */
		SEC_DeletePermCertificate(oldcert);
	    } else {
		/* old cert is for other things too, so just change trust */
		tmptrust = *oldcert->trust;
		tmptrust.emailFlags &= ( ~CERTDB_VALID_PEER );
		rv = CERT_ChangeCertTrust(oldcert->dbhandle, oldcert,
					  &tmptrust);
		if ( rv != SECSuccess ) {
		    goto loser;
		}
	    }
	}

	/* now save the entry */
	entry = NewDBSMimeEntry(emailAddr, &cert->derSubject, emailProfile,
				profileTime, 0);
	if ( entry == NULL ) {
	    goto loser;
	}

	rv = DeleteDBSMimeEntry(cert->dbhandle, emailAddr);
	/* if delete fails, try to write new entry anyway... */
	
	rv = WriteDBSMimeEntry(cert->dbhandle, entry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}

	/* link subject entry back here */
	rv = UpdateSubjectWithEmailAddr(cert, emailAddr);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }

    rv = SECSuccess;
    goto done;
    
loser:
    rv = SECFailure;
    
done:
    if ( oldcert ) {
	CERT_DestroyCertificate(oldcert);
    }
    
    if ( entry ) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    if ( oldentry ) {
	DestroyDBEntry((certDBEntry *)oldentry);
    }
    
    return(rv);
}

CERTCertificate *
CERT_FindCertByEmailAddr(CERTCertDBHandle *handle, char *emailAddr)
{
    certDBEntrySMime *entry;
    CERTCertificate *cert = NULL;

    emailAddr = CERT_FixupEmailAddr(emailAddr);
    if ( emailAddr == NULL ) {
	return(NULL);
    }
    
    entry = ReadDBSMimeEntry(handle, emailAddr);

    /* XXX - this will have to change when multiple certs per subject
     * are allowed
     */
    if ( entry != NULL ) {
	cert = CERT_FindCertByName(handle, &entry->subjectName);
    }

    if ( entry ) {
	DestroyDBEntry((certDBEntry *)entry);
    }

    PORT_Free(emailAddr);
    
    return(cert);
}

/*
 * find the smime symmetric capabilities profile for a given cert
 */
SECItem *
CERT_FindSMimeProfile(CERTCertificate *cert)
{
    certDBEntrySMime *entry;
    SECItem *retitem = NULL;
    
    PORT_Assert(cert->emailAddr != NULL);
    
    if ( cert->emailAddr == NULL ) {
	return(NULL);
    }
    
    entry = ReadDBSMimeEntry(cert->dbhandle, cert->emailAddr);

    if ( entry ) {
	retitem = SECITEM_DupItem(&entry->smimeOptions);
	DestroyDBEntry((certDBEntry *)entry);
    }

    return(retitem);
}

CERTCertificate *
CERT_FindCertByNicknameOrEmailAddr(CERTCertDBHandle *handle, char *name)
{
    CERTCertificate *cert;
    cert = CERT_FindCertByNickname(handle, name);
    if ( cert == NULL ) {
	cert = CERT_FindCertByEmailAddr(handle, name);
    }

    return(cert);
}

PRBool
CERT_IsCertRevoked(CERTCertificate *cert)
{
    return(PR_FALSE);
}

CERTCertificate *
CERT_NextSubjectCert(CERTCertificate *cert)
{
    CERTSubjectNode *node;
    CERTCertificate *retcert = NULL;
    
    node = FindCertSubjectNode(cert);
    PORT_Assert(node != NULL);
    
    if ( node->next != NULL ) {
	retcert = CERT_FindCertByKey(cert->dbhandle, &node->next->certKey);
    }

    return(retcert);
}

CERTCertificate *
CERT_PrevSubjectCert(CERTCertificate *cert)
{
    CERTSubjectNode *node;
    CERTCertificate *retcert = NULL;
    
    node = FindCertSubjectNode(cert);
    PORT_Assert(node != NULL);
    
    if ( node->prev != NULL ) {
	retcert = CERT_FindCertByKey(cert->dbhandle, &node->prev->certKey);
    }

    return(retcert);
}

SECStatus
CERT_SaveImportedCert(CERTCertificate *cert, SECCertUsage usage,
		      PRBool caOnly, char *nickname)
{
    SECStatus rv;
    PRBool saveit;
    CERTCertTrust trust;
    CERTCertTrust tmptrust;
    PRBool isCA;
    unsigned int certtype;
    PRBool freeNickname = PR_FALSE;
    
    isCA = CERT_IsCACert(cert);
    if ( caOnly && ( !isCA ) ) {
	return(SECSuccess);
    }
    
    saveit = PR_TRUE;
    
    PORT_Memset((void *)&trust, 0, sizeof(trust));

    certtype = cert->nsCertType;

    /* if no CA bits in cert type, then set all CA bits */
    if ( isCA && ( ! ( certtype & NS_CERT_TYPE_CA ) ) ) {
	certtype |= NS_CERT_TYPE_CA;
    }

    /* if no app bits in cert type, then set all app bits */
    if ( ( !isCA ) && ( ! ( certtype & NS_CERT_TYPE_APP ) ) ) {
	certtype |= NS_CERT_TYPE_APP;
    }

    if ( isCA ) {
	nickname = CERT_MakeCANickname(cert);
	if ( nickname != NULL ) {
	    freeNickname = PR_TRUE;
	}
    }
    
    switch ( usage ) {
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
	if ( isCA ) {
	    if ( certtype & NS_CERT_TYPE_EMAIL_CA ) {
		trust.emailFlags = CERTDB_VALID_CA;
	    }
	} else {
	    PORT_Assert(nickname == NULL);
	    if ( certtype & NS_CERT_TYPE_EMAIL ) {
		trust.emailFlags = CERTDB_VALID_PEER;
		if ( ! ( cert->rawKeyUsage & KU_KEY_ENCIPHERMENT ) ) {
		    /* don't save it if KeyEncipherment is not allowed */
		    saveit = PR_FALSE;
		}
	    }
	}
	break;
      case certUsageUserCertImport:
	if ( isCA ) {
	    if ( certtype & NS_CERT_TYPE_SSL_CA ) {
		trust.sslFlags = CERTDB_VALID_CA;
	    }
	    
	    if ( certtype & NS_CERT_TYPE_EMAIL_CA ) {
		trust.emailFlags = CERTDB_VALID_CA;
	    }
	    
	    if ( certtype & NS_CERT_TYPE_OBJECT_SIGNING_CA ) {
		trust.objectSigningFlags = CERTDB_VALID_CA;
	    }
	    
	} else {
	    if ( certtype & NS_CERT_TYPE_SSL_CLIENT ) {
		trust.sslFlags = CERTDB_VALID_PEER;
	    }
	    
	    if ( certtype & NS_CERT_TYPE_EMAIL ) {
		trust.emailFlags = CERTDB_VALID_PEER;
	    }
	    
	    if ( certtype & NS_CERT_TYPE_OBJECT_SIGNING ) {
		trust.objectSigningFlags = CERTDB_VALID_PEER;
	    }
	}
	break;
    }

    if ( (trust.sslFlags | trust.emailFlags | trust.objectSigningFlags) == 0 ){
	saveit = PR_FALSE;
    }

    if ( saveit ) {
	if ( cert->isperm ) {
	    /* Cert already in the DB.  Just adjust flags */
	    tmptrust = *cert->trust;
	    tmptrust.sslFlags |= trust.sslFlags;
	    tmptrust.emailFlags |= trust.emailFlags;
	    tmptrust.objectSigningFlags |= trust.objectSigningFlags;
	    
	    rv = CERT_ChangeCertTrust(cert->dbhandle, cert,
				      &tmptrust);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	} else {
	    /* Cert not already in the DB.  Add it */
	    rv = CERT_AddTempCertToPerm(cert, nickname, &trust);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	}
    }

    rv = SECSuccess;
    goto done;

loser:
    rv = SECFailure;
done:

    if ( freeNickname ) {
	PORT_Free(nickname);
    }
    
    return(rv);
}

SECStatus
CERT_ChangeCertTrustByUsage(CERTCertDBHandle *certdb,
			    CERTCertificate *cert, SECCertUsage usage)
{
    SECStatus rv;
    CERTCertTrust trust;
    CERTCertTrust tmptrust;
    unsigned int certtype;
    PRBool saveit;
    
    saveit = PR_TRUE;
    
    PORT_Memset((void *)&trust, 0, sizeof(trust));

    certtype = cert->nsCertType;

    /* if no app bits in cert type, then set all app bits */
    if ( ! ( certtype & NS_CERT_TYPE_APP ) ) {
	certtype |= NS_CERT_TYPE_APP;
    }

    switch ( usage ) {
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
	if ( certtype & NS_CERT_TYPE_EMAIL ) {
	     trust.emailFlags = CERTDB_VALID_PEER;
	     if ( ! ( cert->rawKeyUsage & KU_KEY_ENCIPHERMENT ) ) {
		/* don't save it if KeyEncipherment is not allowed */
		saveit = PR_FALSE;
	    }
	}
	break;
      case certUsageUserCertImport:
	if ( certtype & NS_CERT_TYPE_EMAIL ) {
	    trust.emailFlags = CERTDB_VALID_PEER;
	}
	break;
    }

    if ( (trust.sslFlags | trust.emailFlags | trust.objectSigningFlags) == 0 ){
	saveit = PR_FALSE;
    }

    if ( saveit && cert->isperm ) {
	/* Cert already in the DB.  Just adjust flags */
	tmptrust = *cert->trust;
	tmptrust.sslFlags |= trust.sslFlags;
	tmptrust.emailFlags |= trust.emailFlags;
	tmptrust.objectSigningFlags |= trust.objectSigningFlags;
	    
	rv = CERT_ChangeCertTrust(cert->dbhandle, cert,
				  &tmptrust);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }

    rv = SECSuccess;
    goto done;

loser:
    rv = SECFailure;
done:

    return(rv);
}

int
CERT_GetDBContentVersion(CERTCertDBHandle *handle)
{
    certDBEntryContentVersion *entry;
    int ret;
    
    entry = ReadDBContentVersionEntry(handle);
    
    if ( entry == NULL ) {
	return(0);
    }

    ret = entry->contentVersion;

    DestroyDBEntry((certDBEntry *)entry);

    return(ret);
}

void
CERT_SetDBContentVersion(int version, CERTCertDBHandle *handle)
{
    SECStatus rv;
    certDBEntryContentVersion *entry;
    
    entry = NewDBContentVersionEntry(0);
    
    if ( entry == NULL ) {
	return;
    }
    
    rv = DeleteDBContentVersionEntry(handle);
    rv = WriteDBContentVersionEntry(handle, entry);
    
    DestroyDBEntry((certDBEntry *)entry);
    
    return;
}
