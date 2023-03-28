#include "xp.h"
#include "cert.h"
#include "key.h"
#include "secitem.h"
#include "secder.h"
#include "net.h"
#include "htmldlgs.h"
#include "secnav.h"
#include "xpgetstr.h"
#ifdef XP_MAC
#include "certdb.h"
#else
#include "../cert/certdb.h"
#endif
#include "pk11func.h"
#include "prtime.h"

extern int SEC_ERROR_DUPLICATE_CERT;
extern int SEC_ERROR_DUPLICATE_CERT_NAME;
extern int SEC_ERROR_ADDING_CERT;
extern int SEC_ERROR_FILING_KEY;
extern int SEC_ERROR_NO_KEY;
extern int SEC_ERROR_CERT_NOT_VALID;
extern int SEC_ERROR_CERT_VALID;
extern int SEC_ERROR_CERT_NO_RESPONSE;
extern int MK_OUT_OF_MEMORY;
extern int SEC_ERROR_CRL_INVALID;
extern int SEC_ERROR_CRL_BAD_SIGNATURE;
extern int SEC_ERROR_KRL_INVALID;
extern int SEC_ERROR_KRL_BAD_SIGNATURE;

typedef struct _certStream {
    NET_StreamClass *stream;
    SECItem derCert;
    int certClass;
    char *url; /* used by crl's and krl's */
} certStream;

static PRBool
SEC_CertificateConflict(MWContext *cx, SECItem *derCert,
			CERTCertDBHandle *handle)
{
    SECStatus rv;
    SECItem key;
    PRArenaPool *arena;
    PRBool ret;
    CERTCertificate *cert;
    
    arena = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( ! arena ) {
	goto loser;
    }

    /* get the key (issuer+cn) from the cert */
    rv = CERT_KeyFromDERCert(arena, derCert, &key);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* same cert already exists */
    cert = CERT_FindCertByKey(handle, &key);
    if ( cert ) {
	FE_Alert(cx,XP_GetString(SEC_ERROR_DUPLICATE_CERT));
	CERT_DestroyCertificate(cert);
	ret = PR_FALSE;
	goto done;
    }

    ret = PR_TRUE;
    goto done;
    
loser:
    ret = PR_FALSE;

done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(ret);
}

static void
handle_ca_cert( CERTCertificate *cert, CERTDERCerts *derCerts,
	       certStream *certstream )
{
    SECNAV_MakeCADownloadPanel(certstream->stream->window_id, cert, derCerts);
}

static void
handle_user_cert(CERTCertificate *cert, CERTDERCerts *derCerts,
		 certStream *certstream)
{
    PK11SlotInfo *slot;
    
    slot = PK11_KeyForCertExists(cert, NULL, certstream->stream->window_id);
    if ( slot == NULL ) {
	FE_Alert(certstream->stream->window_id, XP_GetString(SEC_ERROR_NO_KEY));

	CERT_DestroyCertificate(cert);
	
	return;
    }
    PK11_FreeSlot(slot);
    SECNAV_MakeUserCertDownloadDialog(certstream->stream->window_id, cert,
				      derCerts);
	
    return;
}

static SECStatus
collect_certs(void *arg, SECItem **certs, int numcerts)
{
    CERTDERCerts *collectArgs;
    SECItem *cert;
    SECStatus rv;
    
    collectArgs = (CERTDERCerts *)arg;
    
    collectArgs->numcerts = numcerts;
    collectArgs->rawCerts = PORT_ArenaZAlloc(collectArgs->arena,
					   sizeof(SECItem) * numcerts);
    
    if ( collectArgs->rawCerts == NULL ) {
	return(SECFailure);
    }
    
    cert = collectArgs->rawCerts;
    
    while ( numcerts-- ) {
	rv = SECITEM_CopyItem(collectArgs->arena, cert, *certs);
	if ( rv == SECFailure ) {
	    return(SECFailure);
	}
	
	cert++;
	certs++;
    }

    return(SECSuccess);
}

/*
 * complete the loading of the certificate
 * prompt the user for confirmation, then load it into the database
 */
static void
cert_complete(void *data)
{
    certStream *certstream;
    CERTCertificate *cert;
    PRBool download;
    CERTDERCerts *collectArgs;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    certstream = (certStream *)data;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    collectArgs = (CERTDERCerts *)PORT_ArenaZAlloc(arena,
						   sizeof(CERTDERCerts));
    if ( collectArgs == NULL ) {
	goto loser;
    }

    collectArgs->arena = arena;
    
    /* decode the accumulated certificate */
    rv = CERT_DecodeCertPackage((char *)certstream->derCert.data,
				certstream->derCert.len,
				collect_certs, (void *)collectArgs);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* check first cert */
    download = SEC_CertificateConflict(certstream->stream->window_id,
				       collectArgs->rawCerts,
				       CERT_GetDefaultCertDB());

    if ( !download ) {
	goto loser;
    }
    
    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
				   collectArgs->rawCerts,
				   NULL, PR_FALSE, PR_TRUE);

    if ( !cert ) {
	goto loser;
    }

    switch(certstream->certClass) {
      case SEC_CERT_CLASS_CA:
	handle_ca_cert(cert, collectArgs, certstream); /* dialog handler destroys cert */
	break;
      case SEC_CERT_CLASS_SERVER:
	CERT_DestroyCertificate(cert);
	break;
      case SEC_CERT_CLASS_USER:
	handle_user_cert(cert, collectArgs, certstream); /* dialog handler destroys cert */
	break;
      case SEC_CERT_CLASS_EMAIL:
	SECNAV_MakeEmailCertDownloadDialog(certstream->stream->window_id,
					   cert, collectArgs);
	break;
      default:
	break;
    }

loser:
    
    PORT_Free(certstream->derCert.data);
    PORT_Free(certstream);
    
    return;
}

/*
 * abort the transfer of the certificate
 */
static void
cert_abort(void *data, int status)
{
    certStream *certstream;
    
    certstream = (certStream *)data;

    PORT_Free(certstream->derCert.data);
    if (certstream->url) PORT_Free(certstream->url);
    PORT_Free(certstream);
    
    return;
}

/*
 * don't accept data until password is initialized to avoid race with
 * password setup dialog
 */
static unsigned int
cert_is_write_ready(void *data)
{
    certStream *certstream;
    SECKEYKeyDBHandle *keydb;
    
    keydb = SECKEY_GetDefaultKeyDB();

    certstream = (certStream *)data;

    if ( ( certstream->certClass != SEC_CERT_CLASS_USER ) ||
	( keydb->dialog_pending == PR_FALSE ) ) {
	return( MAX_WRITE_READY );
    } else {
	return(0);
    }
}

/*
 * read certificate data from the stream
 */
static int
cert_write(void *data, const char *str, int32 len)
{
    certStream *certstream;
    unsigned char *tmpdata;
    
    certstream = (certStream *)data;
    
    if ( certstream->derCert.data ) {
	/* realloc the buffer */
	tmpdata = (unsigned char *)PORT_Realloc(certstream->derCert.data,
					      certstream->derCert.len + len);
	if ( tmpdata == NULL ) {
	    return(MK_OUT_OF_MEMORY);
	}
	
	/* copy new stuff into it */
	PORT_Memcpy(tmpdata + certstream->derCert.len, str, len);

	/* update the secitem */
	certstream->derCert.data = tmpdata;
	certstream->derCert.len += len;
    } else {
	/* alloc the buffer */
	tmpdata = (unsigned char *)PORT_Alloc(len);

	if ( tmpdata == NULL ) {
	    return(MK_OUT_OF_MEMORY);
	}

	/* copy stuff into it */
	PORT_Memcpy(tmpdata, str, len);

	/* update the secitem */
	certstream->derCert.data = tmpdata;
	certstream->derCert.len = len;
    }

    return(1);
}

/*
 * set up a stream for downloading certificates
 */
NET_StreamClass *
SECNAV_CertificateStream(FO_Present_Types format_out, void *data,
			 URL_Struct *urls, MWContext *cx)
{
    NET_StreamClass *stream;
    certStream *certstream;
    
    stream = (NET_StreamClass *)PORT_ZAlloc(sizeof(NET_StreamClass));
    
    if ( stream == NULL ) {
	return(NULL);
    }

    /* allocate our data object */
    certstream = (certStream *)PORT_ZAlloc(sizeof(certStream));
    if ( certstream == NULL ) {
	PORT_Free(stream);
	return(NULL);
    }
    
    certstream->stream = stream;
    certstream->certClass = (int)data;
    certstream->url = NULL;
    
    stream->name = "certificate stream";
    stream->complete = cert_complete;
    stream->abort = cert_abort;
    stream->is_write_ready = cert_is_write_ready;
    stream->data_object = (void *)certstream;
    stream->window_id = cx;
    stream->put_block = (MKStreamWriteFunc)cert_write;
    
    return(stream);
}

typedef struct _revokeStream {
    NET_StreamClass *stream;
    PRBool revoked;
    PRBool got_it;
    URL_Struct *url;
    MWContext *cx;
} revokeStream;

/*
 * complete the loading of the certificate revocation response
 * display the results to the user
 */
static void
revoke_complete(void *data)
{
    revokeStream *revokestream;
    
    revokestream = (revokeStream *)data;

    if ( revokestream->got_it ) {
	if ( revokestream->revoked ) {
	    FE_Alert(revokestream->cx, XP_GetString(SEC_ERROR_CERT_NOT_VALID));
	} else {
	    FE_Alert(revokestream->cx, XP_GetString(SEC_ERROR_CERT_VALID));
	}
    } else {
	FE_Alert(revokestream->cx, XP_GetString(SEC_ERROR_CERT_NO_RESPONSE));
    }
    
    PORT_Free(revokestream);
    
    return;
}

/*
 * abort the transfer of the certificate revocation information
 */
static void
revoke_abort(void *data, int status)
{
    revokeStream *revokestream;
    
    revokestream = (revokeStream *)data;

    PORT_Free(revokestream);
    
    return;
}

/*
 * always ready
 */
static unsigned int
revoke_is_write_ready(void *data)
{
    return( MAX_WRITE_READY );
}

/*
 * read revocation data from the stream
 */
static int
revoke_write(void *data, const char *str, int32 len)
{
    revokeStream *revokestream;
    
    revokestream = (revokeStream *)data;

    /* if we already got a 1 or 0, then just skip the data */
    if ( ! revokestream->got_it ) {
	while ( len ) {

	    /* ignore everything but a 0 or 1 */
	    if ( *str == '0' ) {
		revokestream->got_it = PR_TRUE;
		revokestream->revoked = PR_FALSE;
		break;
	    } else if ( *str == '1' ) {
		revokestream->got_it = PR_TRUE;
		revokestream->revoked = PR_TRUE;
		break;
	    }
	    len--;
	    str++;
	}
    }

    return(1);
}

/*
 * set up a stream for downloading revocation information
 */
NET_StreamClass *
SECNAV_RevocationStream(FO_Present_Types format_out, void *data,
		     URL_Struct *urls, MWContext *cx)
{
    NET_StreamClass *stream;
    revokeStream *revokestream;
    
    stream = (NET_StreamClass *)PORT_ZAlloc(sizeof(NET_StreamClass));
    
    if ( stream == NULL ) {
	return(NULL);
    }

    /* allocate our data object */
    revokestream = (revokeStream *)PORT_ZAlloc(sizeof(revokeStream));
    if ( revokestream == NULL ) {
	PORT_Free(stream);
	return(NULL);
    }
    
    revokestream->stream = stream;
    revokestream->url = urls;
    revokestream->revoked = PR_FALSE;
    revokestream->got_it = PR_FALSE;
    revokestream->cx = cx;

    stream->name = "revocation stream";
    stream->complete = revoke_complete;
    stream->abort = revoke_abort;
    stream->is_write_ready = revoke_is_write_ready;
    stream->data_object = (void *)revokestream;
    stream->window_id = cx;
    stream->put_block = (MKStreamWriteFunc)revoke_write;
    
    return(stream);
}

static void 
crl_alert_invalid(MWContext *cx, int type) {
    int error = (type == SEC_CRL_TYPE) ? SEC_ERROR_CRL_INVALID :
				SEC_ERROR_KRL_INVALID;
    FE_Alert(cx,XP_GetString(error));
}

/*
 * complete the loading of the crl/krl
 */
static void
crl_complete(void *data)
{
    certStream *certstream;
    CERTCertificate *caCert = NULL;
    CERTSignedCrl *crl;
    SECItem derName,*derCrl;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    CERTCertDBHandle *handle = CERT_GetDefaultCertDB();
    CERTSignedData sd;
    int type, error;
    
    certstream = (certStream *)data;
    type = certstream->certClass;
    derCrl = &certstream->derCert;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto loser;

    /* find the CACert */
    rv = CERT_KeyFromDERCrl(arena,derCrl,&derName);
    if (rv != SECSuccess) {
	crl_alert_invalid(certstream->stream->window_id,type);
	goto loser;
    }
    caCert = CERT_FindCertByName(handle,&derName);
    if (caCert == NULL) {
	if (type == SEC_KRL_TYPE) {
	    FE_Alert(certstream->stream->window_id, 
			"Couldn't find Root certificate for KRL\n");
	    goto loser;
	}
    } else {
    	/* check the signature */
	rv = DER_Decode(arena,&sd, CERTSignedDataTemplate, derCrl);
	if (rv != SECSuccess) {
	    crl_alert_invalid(certstream->stream->window_id,type);
	    goto loser;
	}
	rv = CERT_VerifySignedData(&sd, caCert, PR_Now(),
				   certstream->stream->window_id);
	if (rv != SECSuccess) {
	    error = (type == SEC_CRL_TYPE ? SEC_ERROR_CRL_BAD_SIGNATURE :
				SEC_ERROR_KRL_BAD_SIGNATURE);
	    FE_Alert(certstream->stream->window_id,XP_GetString(error));

	}

    }
  

    /* load it into the data base */
    crl = SEC_NewCrl(handle,certstream->url,derCrl,type);

    if ( !crl ) {
	int error = PORT_GetError();

	if (error) { 
	    FE_Alert(certstream->stream->window_id,XP_GetString(error));
	}
	goto loser;
    }

    SEC_DestroyCrl(crl);


loser:
    if (caCert) CERT_DestroyCertificate(caCert); 
    if (certstream->url) PORT_Free(certstream->url);
    PORT_Free(certstream->derCert.data);
    PORT_Free(certstream);
    
    return;
}

/*
 * don't accept data until password is initialized to avoid race with
 * password setup dialog
 */
static unsigned int
crl_is_write_ready(void *data)
{
    return( MAX_WRITE_READY );
}


/*
 * set up a stream for downloading certificates
 */
NET_StreamClass *
SECNAV_CrlStream(FO_Present_Types format_out, void *data,
		      URL_Struct *urls, MWContext *cx)
{
    NET_StreamClass *stream;
    certStream *certstream;
    
    stream = (NET_StreamClass *)PORT_ZAlloc(sizeof(NET_StreamClass));
    
    if ( stream == NULL ) {
	return(NULL);
    }

    /* allocate our data object */
    certstream = (certStream *)PORT_ZAlloc(sizeof(certStream));
    if ( certstream == NULL ) {
	PORT_Free(stream);
	return(NULL);
    }
    
    certstream->stream = stream;
    certstream->certClass = (int)data;
    certstream->url = PORT_Strdup(urls->address);
    
    stream->name = "crl stream";
    stream->complete = crl_complete;
    stream->abort = cert_abort;
    stream->is_write_ready = crl_is_write_ready;
    stream->data_object = (void *)certstream;
    stream->window_id = cx;
    stream->put_block = (MKStreamWriteFunc)cert_write;
    
    return(stream);
}

