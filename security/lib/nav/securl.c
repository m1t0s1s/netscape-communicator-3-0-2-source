/* -*- Mode: C; tab-width: 8 -*-
 * securl.c --- the "about:security" URLs.
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: securl.c,v 1.51.20.1 1997/03/09 12:40:47 jwz Exp $
 */

#include "net.h"
#include "xp.h"
#include "xp_sec.h"
#include "secnav.h"
#ifdef XP_MAC
#include "certdb.h"
#else
#include "../cert/certdb.h"
#endif
#include "cert.h"
#include "secder.h"
#include "sechash.h"
#include "htmldlgs.h"

#include "secban.h"

extern int MK_OUT_OF_MEMORY;
extern int MK_MALFORMED_URL_ERROR;

/* subtypes of about:security URLs */
#define SEC_URL_EMPTY			0
#define SEC_URL_CERT_ISSUER_LOGO	5
#define SEC_URL_CERT_SUBJECT_LOGO	6
#define SEC_URL_BANNER_SECURE		7
#define SEC_URL_BANNER_INSECURE		8
#define SEC_URL_BANNER_MIXED		9
#define SEC_URL_ADVISOR			10

SECStatus
putItemToStream(NET_StreamClass *stream, SECItem *item)
{
    int rv;
    
    rv = (*(stream)->put_block)(stream->data_object, (char*)item->data, item->len);
    
    if ( rv < 0 ) {
	return(SECFailure);
    } else {
	return(SECSuccess);
    }
}

/*
 * determine the type of the given about:security url, and return a pointer
 * to the argument string.
 */
static int
sec_URLType(char *url, char **parg)
{
    int type;
    
    type = -1;
    *parg = 0;
    
    if ( url && ( *url ) ) {
        char *eq = PORT_Strchr(url, '=');
	int L = (eq ? eq - url : PORT_Strlen(url));
	*parg = url + L + (eq ? 1 : 0);
	switch(*url) {
	  case 'a':
	  case 'A':
	    if(L == 7 && !strncasecomp(url,"advisor",L)) {
	      type = SEC_URL_ADVISOR;
	    }
	    break;
	  case 'b':
	  case 'B':
	    if(L == 15 && !strncasecomp(url,"banner-insecure",L)) {
		type = SEC_URL_BANNER_INSECURE;
	    } else if(L == 13 && !strncasecomp(url,"banner-secure",L)) {
		type = SEC_URL_BANNER_SECURE;
	    } else if(L == 12 && !strncasecomp(url,"banner-mixed",L)) {
		type = SEC_URL_BANNER_MIXED;
	    }
	    break;
	  case 'i':
	  case 'I':
	    if(L == 11 && !strncasecomp(url,"issuer-logo",L)) {
	      type = SEC_URL_CERT_ISSUER_LOGO;
	    }
	    break;
	  case 's':
	  case 'S':
	    if(L == 12 && !strncasecomp(url,"subject-logo",L)) {
		type = SEC_URL_CERT_SUBJECT_LOGO;
	    }
	    break;
	  default:
	    break;
	}
    } else {
	type = SEC_URL_EMPTY;
    }
    
    return type;
}

/*
 * send the data for the given about:security url to the given stream
 */
int
SECNAV_SecURLData(char *which, NET_StreamClass *stream, MWContext *cx)
{
    SECStatus status;
    int type;
    char *arg;
    CERTCertificate *cert;
    SECItem item;

    /* determine sub-type of URL */
    type = sec_URLType(which, &arg);

    switch(type) {
      case SEC_URL_CERT_ISSUER_LOGO:
	if ( *arg ) {
	    SECItem value;
	    
	    cert = CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(),
						      arg);
	    if ( !cert ) {
		return(SECFailure);
	    }

	    status = CERT_FindCertExtension(cert,
					   SEC_OID_NS_CERT_EXT_ISSUER_LOGO,
					   &value);

	    if ( status ) {
		CERT_DestroyCertificate(cert);
		break;
	    }
	    
	    status = putItemToStream(stream, &value);
	    PORT_Free(value.data);
	    CERT_DestroyCertificate(cert);
	    if ( status ) {
		return(SECFailure);
	    }
	    
	}
	break;
      case SEC_URL_CERT_SUBJECT_LOGO:
	if ( *arg ) {
	    SECItem value;
	    
	    cert = CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(),
						      arg);
	    if ( !cert ) {
		return(SECFailure);
	    }

	    status = CERT_FindCertExtension(cert,
					   SEC_OID_NS_CERT_EXT_SUBJECT_LOGO,
					   &value);
	    if ( status ) {
		CERT_DestroyCertificate(cert);
		break;
	    }
	    
	    status = putItemToStream(stream, &value);
	    PORT_Free(value.data);
	    CERT_DestroyCertificate(cert);
	    if ( status ) {
		return(SECFailure);
	    }
	    
	}
	break;

      case SEC_URL_BANNER_SECURE:
	item.data = BannerSecure;
	item.len = sizeof(BannerSecure);
	status = putItemToStream(stream, &item);
	break;
      case SEC_URL_BANNER_INSECURE:
	item.data = BannerInsecure;
	item.len = sizeof(BannerInsecure);
	status = putItemToStream(stream, &item);
	break;
      case SEC_URL_BANNER_MIXED:
	item.data = BannerMixed;
	item.len = sizeof(BannerMixed);
	status = putItemToStream(stream, &item);
	break;
      case SEC_URL_ADVISOR:
	XP_ASSERT(0);  /* not reached */
	return -1;
      default:
	break;
    }
    
    return(SECSuccess);
}

char *
SECNAV_SecURLContentType(char *which)
{
    int type;
    char *arg;
    char *contenttype;
    
    type = sec_URLType(which, &arg);
    
    switch(type) {
      case SEC_URL_CERT_ISSUER_LOGO:
      case SEC_URL_CERT_SUBJECT_LOGO:
      case SEC_URL_BANNER_SECURE:
      case SEC_URL_BANNER_INSECURE:
      case SEC_URL_BANNER_MIXED:
	contenttype = IMAGE_GIF;
	break;
      case SEC_URL_ADVISOR:
	contenttype = "advisor";
	break;
      default:
        contenttype = 0;  /* Let netlib return MK_MALFORMED_URL_ERROR */
	break;
    }

    return(contenttype);
}


int
SECNAV_SecHandleSecurityAdvisorURL(MWContext *cx, const char *which)
{
  char *equal = 0;
  URL_Struct *url_struct = 0;
  XP_Bool legal = TRUE;
  char *s;

  equal = which ? XP_STRCHR(which, '=') : 0;
  if (equal)
    {
      int i;
      char *url = XP_STRDUP(equal + 1);
      if (!url) return MK_OUT_OF_MEMORY;
      NET_UnEscape(url);
      url_struct = NET_CreateURLStruct(url, NET_NORMAL_RELOAD);
      for (s = url; *s; s++) *s += 23;
      if (!XP_STRCMP (url, "\170\171\206\214\213\121\204\206"
		      "\221\200\203\203\170"))
	legal = FALSE;
      XP_FREE(url);

      if (!url_struct)
	return MK_OUT_OF_MEMORY;
    }

  if (legal)
    SECNAV_SecurityAdvisor(cx, url_struct);
  else
    {
      const char *s2 = "\130\212\067\220\206\214\211\067\203\170\216"
	"\220\174\211\103\067\140\067\170\173\215\200\212\174\067\220"
	"\206\214\067\213\206\067\212\203\206\216\067\173\206\216\205"
	"\103\067\204\170\205\070";
      char *s3 = XP_ALLOC((url_struct ? XP_STRLEN(url_struct->address) : 0)
			  + XP_STRLEN(s2) + 10);
      if (s3)
	{
	  XP_STRCPY(s3, s2);
	  for (s = s3; *s; s++) *s -= 23;
	  FE_Alert(cx, s3);
	  XP_FREE(s3);
	}
    }

  if (url_struct)
    NET_FreeURLStruct(url_struct);

  return 0;
}
