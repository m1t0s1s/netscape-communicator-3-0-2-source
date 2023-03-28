/*
 * @(#)outStr.c	1.2 95/06/14 Warren Harris
 *
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 *
 */

#include "nsn.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prtime.h"

#include "netscape_net_URLConnection.h"
#include "javaString.h"
#include "javaThreads.h"

/******************************************************************************/

PUBLIC void
netscape_net_URLConnection_pCreate(struct Hnetscape_net_URLConnection* this, 
				   struct Hjava_lang_String* urlString,
                                   struct Hjava_lang_String* addressString)
{
    Classnetscape_net_URLConnection* con = unhand(this);
    URL_Struct* url = NULL;
    char* str = NULL;
    char* addr = NULL;
    nsn_JavaStreamData* javaData = NULL;
    MWContext* javaContext;
    LJAppletData* ad;
    NET_ReloadMethod reloadMethod;

    con->pStreamData = 0;
    str = allocCString(urlString);	/* freed by netlib */
    addr = addressString ? allocCString(addressString) : NULL;	/* freed by netlib */
    PR_LOG_BEGIN(NSJAVA, debug,
		 ("URLConnection.pCreate(%s)", str));

    ad = nsn_GetCurrentAppletData();
    reloadMethod = (ad != NULL) ? ad->reloadMethod : NET_NORMAL_RELOAD;
    url = NET_CreateURLStruct(str, reloadMethod);
    if (addr && url) {
      if (NET_SetURLIPAddressString(url, addr)) {
        /* out of memory */
        NET_FreeURLStruct(url);
        url = NULL;
        }
      }
    free(addr);
    if (url == NULL) goto OutOfMemory;
    INCR(urlCount);

    javaData = PR_NEW(nsn_JavaStreamData);
    if (javaData == NULL) goto OutOfMemory;
    INCR(javaDataCount);

    javaData->state = OPENING;
    javaData->refcount = 2; /* Two users, netlib and java */
    javaData->errorStatus = 0;
    javaData->errorMessage = NULL;
    javaData->notifies = 0;
    javaData->netStream = NULL;
    javaData->realContext = nsn_GetCurrentContext();
    javaData->url = url;
    javaData->dataQueue.head = NULL;
    javaData->dataQueue.tail = NULL;

    javaContext = nsn_JavaContext_make(url, javaData);
    if (javaContext == NULL) goto OutOfMemory;

    javaData->javaContext = javaContext;
    url->fe_data = javaData;
    con->pStreamData = (long)javaData;

    PR_LOG_END(NSJAVA, debug,
	       ("URLConnection.pCreate(%s) connected", str));
    
    free(str);
    return;

  OutOfMemory:
    PR_LOG_END(NSJAVA, debug,
	       ("URLConnection.pCreate(%s) raising OutOfMemory", str));
    NSN_FREEIF(javaData);
    DECR(javaDataCount);
    if (url != NULL) {
	NET_FreeURLStruct(url);
	DECR(urlCount);
    }
    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
    free(str);
}

PUBLIC long
netscape_net_URLConnection_getContentLength0(struct Hnetscape_net_URLConnection* this)
{
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(this)->pStreamData;
    URL_Struct* url;
    long result = -1;

    ENTER_METHOD("URLConnection.getContentLength", SILENTLY_FAIL)

    switch (javaData->state) {
      case OPENED:
      case LIBNET_DONE:
	url = javaData->url;
	/* netlib returns 0 for unknown content length, but java wants -1: */
	result = (url->content_length == 0 ? -1 : url->content_length);
	break;
      default:
	break;
    }

    EXIT_METHOD("URLConnection.getContentLength")
    return result;
}

PUBLIC struct Hjava_lang_String*
netscape_net_URLConnection_getContentType0(struct Hnetscape_net_URLConnection* this)
{
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(this)->pStreamData;
    URL_Struct* url;
    struct Hjava_lang_String* result = NULL;

    ENTER_METHOD("URLConnection.getContentType", SILENTLY_FAIL)

    switch (javaData->state) {
      case OPENED:
      case LIBNET_DONE:
	url = javaData->url;
	result = makeJavaString(url->content_type, XP_STRLEN(url->content_type));
	break;
      default:
	break;
    }

    EXIT_METHOD("URLConnection.getContentType")
    return result;
}

static char*
nsn_TimeString(time_t* time)
{
    #define TIME_LENGTH 64
    char* str = malloc(TIME_LENGTH);
    
    uint64	timeInUSec;
    uint64      secToUSec;
    PRTime	expandedTime;

    if (str == NULL) return NULL;
    LL_I2L(timeInUSec, *time);
    LL_I2L(secToUSec, PR_USEC_PER_SEC);
    LL_MUL(timeInUSec, timeInUSec, secToUSec);
#ifndef XP_MAC    
    timeInUSec = PR_ToGMT(timeInUSec);
#endif /* XP_MAC */
    PR_ExplodeTime(&expandedTime, timeInUSec);
    PR_FormatTimeUSEnglish(str, TIME_LENGTH, "%a %b %d %H:%M:%S GMT %Y", &expandedTime);

    return str;
}

PUBLIC struct Hjava_lang_String*
netscape_net_URLConnection_getHeaderField0(struct Hnetscape_net_URLConnection* this,
					   struct Hjava_lang_String* name)
{
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(this)->pStreamData;
    char* str;
    char* resultStr = NULL;
    struct Hjava_lang_String* result = NULL;
    PRBool freeStr = PR_FALSE;
    URL_Struct* url;

    ENTER_METHOD("URLConnection.getHeaderField", SILENTLY_FAIL)

    switch (javaData->state) {
      case OPENED:
      case LIBNET_DONE:
	str = makeCString(name);	/* freed by gc */
	url = javaData->url;

	if (XP_STRCASECMP(str, "Content-type") == 0)
	    resultStr = url->content_type;
	else if (XP_STRCASECMP(str, "Content-encoding") == 0)
	    resultStr = url->content_encoding;
	else if (XP_STRCASECMP(str, "charset") == 0)
	    resultStr = url->charset;
	else if (XP_STRCASECMP(str, "boundary") == 0)
	    resultStr = url->boundary;
	else if (XP_STRCASECMP(str, "Content-name") == 0)
	    resultStr = url->content_name;
	else if (XP_STRCASECMP(str, "Expires") == 0) {
	    resultStr = nsn_TimeString(&url->expires);
	    freeStr = PR_TRUE;
	}
	else if (XP_STRCASECMP(str, "Last-modified") == 0) {
	    resultStr = nsn_TimeString(&url->last_modified);
	    freeStr = PR_TRUE;
	}
	else if (XP_STRCASECMP(str, "Date") == 0) {
	    resultStr = nsn_TimeString(&url->server_date);
	    freeStr = PR_TRUE;
	}
	else if (XP_STRCASECMP(str, "Content-length") == 0) {
	    /* netlib returns 0 for unknown content length, but java wants -1: */
	    resultStr = PR_smprintf("%ld", (url->content_length == 0 ? -1 : url->content_length));
	    freeStr = PR_TRUE;
	}
	else if (XP_STRCASECMP(str, "Server-status") == 0) {
	    resultStr = PR_smprintf("%ld", url->server_status);
	    freeStr = PR_TRUE;
	}
        else {
            uint32 index=0;
            for (; index < url->all_headers.empty_index; index++) {
                if (XP_STRCASECMP(str, url->all_headers.key[index]) == 0) {
                    resultStr = url->all_headers.value[index];
                    break;
                 }
            }
        }

	if (resultStr != NULL)
	    result = makeJavaString(resultStr, XP_STRLEN(resultStr));
	if (freeStr)
	    NSN_FREE(resultStr);
	break;
      default:
	break;
    }

    EXIT_METHOD("URLConnection.getHeaderField")
    return result;
}

PUBLIC struct Hjava_lang_String*
netscape_net_URLConnection_getHeaderFieldKey0(struct Hnetscape_net_URLConnection* this,
					   long index)
{
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(this)->pStreamData;
    char* resultStr = NULL;
    struct Hjava_lang_String* result = NULL;
    URL_Struct* url;

    ENTER_METHOD("URLConnection.getHeaderFieldKey", SILENTLY_FAIL)

    switch (javaData->state) {
      case OPENED:
      case LIBNET_DONE:
	url = javaData->url;

        if (index < url->all_headers.empty_index)
	    resultStr = url->all_headers.key[index];
	if (resultStr != NULL)
	    result = makeJavaString(resultStr, XP_STRLEN(resultStr));
	break;
      default:
	break;
    }

    EXIT_METHOD("URLConnection.getHeaderFieldKey")
    return result;
}

/******************************************************************************/

void
nsn_JavaStream_close(nsn_JavaStreamData* javaData)
{
    PR_LOG_BEGIN(NSJAVA, debug,
		 ("JavaStream.close(%s)", URL_ADDR(javaData)));
    /*
    ** Only when both parties have agreed that they're done can we
    ** destroy the url.
    */
    NET_FreeURLStruct(javaData->url);
    javaData->url = NULL;
    DECR(urlCount);
    
    nsn_DataQueue_clear(&javaData->dataQueue);
    javaData->state = CLOSED;
    
    PR_LOG_END(NSJAVA, debug,
	       ("JavaStream.close(%s)", URL_ADDR(javaData)));
}

void
nsn_JavaStream_javaClose(nsn_JavaStreamData* javaData)
{
    /*
    ** This method is called whenever java initiates the closing of a
    ** connection.
    */

    ENTER_METHOD("JavaStream.javaClose", CANT_HAPPEN)

    /*
    ** Be sure to drop the real context pointer when the connection is
    ** closed. This prevents status messages from being sent to a context
    ** that may already be destroyed.
    */
    javaData->realContext = NULL;

    switch (javaData->state) {
      case OPENED:
      case OPENING:
      case POSTING:
	/*
	** OPENING could happen if we've done the NET_GetURL and then
	** decided to close before netlib could construct the stream.
	*/
	javaData->state = JAVA_DONE;
	PR_LOG(NSJAVA, debug,
	       ("JavaStream.javaClose(%s) java done", URL_ADDR(javaData)));
	break;

      case JAVA_DONE:
      case CLOSED:
	PR_LOG(NSJAVA, debug,
	       ("JavaStream.javaClose(%s): stream already closed!",
		URL_ADDR(javaData)));
	break;

      case LIBNET_DONE:
	/*
	** In this case, libnet has already indicated it's desire to
	** close the stream:
	*/
	PR_LOG(NSJAVA, debug,
	       ("JavaStream.javaClose(%s) java closed",
		URL_ADDR(javaData)));

	nsn_JavaStream_close(javaData);
	break;
      default:
	PR_ASSERT(0);	/* should never get here */
	break;
    }

    /* We can't be needing the input anymore: */
    nsn_DataQueue_clear(&javaData->dataQueue);

    EXIT_METHOD("JavaStream.javaClose")
}

PUBLIC void
netscape_net_URLConnection_close(struct Hnetscape_net_URLConnection* this)
{
    Classnetscape_net_URLConnection* con = unhand(this);
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)con->pStreamData;

    ENTER_METHOD("URLConnection.close", IO_EXCEPTION)

    PR_ASSERT(javaData != NULL);		/* javaData was garbage collected */

    nsn_JavaStream_javaClose(javaData);

    con->currentInputStream = 0;
    con->connected = 0;

    EXIT_METHOD("URLConnection.close")
}

PUBLIC void
netscape_net_URLConnection_finalize(struct Hnetscape_net_URLConnection* this)
{
    /* Only called during garbage collection -- consequently no one else
       can be running a method on this connection. Therefore it's ok to finally
       delete the monitor and JavaStreamData object. */
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(this)->pStreamData;

    if (javaData) {
	PR_LOG_BEGIN(NSJAVA, debug,
		     ("URLConnection.pFinalize(%s)", URL_ADDR(javaData)));

	nsn_JavaStream_javaClose(javaData);
	
	/*	If we are done with the data data (no more references),
		dispose of it. */
	
	if (--(javaData->refcount) == 0) {
		if( javaData->url ) {
		    javaData->url->fe_data = NULL;
		}
		if( javaData->javaContext ) {
		    javaData->javaContext->javaContextStreamData = NULL;
		}
		NSN_FREEIF(javaData->errorMessage);
		NSN_FREE(javaData);
		DECR(javaDataCount);
		unhand(this)->pStreamData = 0;
	}

	PR_LOG_END(NSJAVA, debug, ("URLConnection.pFinalize"));
    }
    else {
	PR_LOG(NSJAVA, debug,
	       ("URLConnection.pFinalize: object finalized more than once"));
    }
}

/******************************************************************************/
