/*
 * @(#)netStubs.c	1.2 95/06/14 Warren Harris
 *
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 *
 */

#include "nsn.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"

#undef MIN
#define MIN(a,b)((a)<(b)?(a):(b))

#include "netscape_net_URLStreamHandler.h"
#include "netscape_net_URLConnection.h"
#ifndef XP_MAC
#include "netscape_net_URLStreamHandlerFactory.h"
#else
#include "n_net_URLStreamHandlerFactory.h"
#endif

#include "javaString.h"

extern int MK_OUT_OF_MEMORY;

#ifdef DEBUG
/*
** These counters are used to keep track of whether we're freeing all the
** objects we're creating:
*/
nsn_Counters nsnCounters = { 0, 0, 0, 0, 0 };
#endif /* DEBUG */

/******************************************************************************/

#include "interpreter.h"
#include "javaThreads.h"
#include "java_lang_Thread.h"
#include "java_lang_ThreadGroup.h"
#define IMPLEMENT_netscape_applet_AppletThreadGroup
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#define IMPLEMENT_netscape_applet_EmbeddedAppletFrame
#ifndef XP_MAC
#include "netscape_applet_AppletThreadGroup.h"
#include "netscape_applet_EmbeddedAppletFrame.h"
#include "netscape_applet_MozillaAppletContext.h"
#else
#include "n_applet_AppletThreadGroup.h"
#undef DEBUG /* because javah generates extern symbols that are too long */
#include "n_applet_EmbeddedAppletFrame.h"
#include "n_applet_MozillaAppletContext.h"
#endif

static struct Hnetscape_applet_EmbeddedAppletFrame*
nsn_GetCurrentAppletFrame(void)
{
    TID current = threadSelf();
    HThreadGroup* group = unhand(current)->group;
    ExecEnv* ee = EE();
    ClassClass* classAppletThreadGroup =
	FindClass(ee, "netscape/applet/AppletThreadGroup", TRUE);
    struct Hnetscape_applet_EmbeddedAppletFrame* viewer;

    while (!is_instance_of((JHandle*)group, classAppletThreadGroup, ee)) {
	group = unhand(group)->parent;
	if (group == NULL) return NULL;
    }
    viewer = unhand((Hnetscape_applet_AppletThreadGroup*)group)->viewer;
    return viewer;
}

/*
** This function walks the chain of thread groups looking for an
** AppletThreadGroup. From there you can get to EmbeddedAppletFrame, and
** then the MozillaAppletContext to get the MWContext. If we can't find
** an AppletThreadGroup, we must not be in an applet, so we won't show
** any status information.
*/
MWContext*
nsn_GetCurrentContext(void)
{
    JRIEnv* env = JRI_GetCurrentEnv();
    struct netscape_applet_EmbeddedAppletFrame* viewer =
	(struct netscape_applet_EmbeddedAppletFrame*)
	nsn_GetCurrentAppletFrame();
    struct netscape_applet_MozillaAppletContext* appletContext;
    if (viewer == NULL)
	return NULL;
    /*
    ** We're mixing jdk and jri here -- This cast only works because we
    ** know the representation of jrefs:
    */
    appletContext = (struct netscape_applet_MozillaAppletContext*)
	get_netscape_applet_EmbeddedAppletFrame_context(env, viewer);
    return (MWContext*)
        get_netscape_applet_MozillaAppletContext_frameMWContext(
            env, appletContext);	/* whew! */
}

LJAppletData*
nsn_GetCurrentAppletData(void)
{
    JRIEnv* env = JRI_GetCurrentEnv();
    struct netscape_applet_EmbeddedAppletFrame* viewer =
	(struct netscape_applet_EmbeddedAppletFrame*)
	nsn_GetCurrentAppletFrame();
    if (viewer == NULL)
	return NULL;
    return (LJAppletData*)
	get_netscape_applet_EmbeddedAppletFrame_pData(env, viewer);
}

/******************************************************************************/

void
nsn_DataQueue_insert(nsn_DataQueue* queue, nsn_DataQueueElement* buf)
{
    if (queue->tail == NULL)
	queue->head = buf;
    else
	queue->tail->next = buf;
    queue->tail = buf;
}

/*
** Allocating buffers of length DEFAULT_DATA_LENGTH prevents us from
** dealing with a lot of 1 byte buffers:
*/
#define DEFAULT_DATA_LENGTH	256

nsn_DataQueueElement*
nsn_DataQueue_writeBytes(nsn_DataQueue* queue, const char* bytes, long length)
{
    nsn_DataQueueElement* buf = queue->head;

    /*
    ** First check if we can write some of this data to the end of the
    ** current DataQueueElement:
    */
    if (buf != NULL && buf->writeCursor < buf->dataLength) {
	long available = buf->dataLength - buf->writeCursor;
	long amount = MIN(length, available);
	XP_MEMCPY(&buf->data[buf->writeCursor], bytes, amount);
	buf->writeCursor += amount;
	bytes += amount;
	length -= amount;
    }

    /*
    ** And if there's more, create a new DataQueueElement:
    */
    if (length > 0) {
	long dataLength = MAX(length, DEFAULT_DATA_LENGTH);

	buf = PR_NEW(nsn_DataQueueElement);
	if (!buf) return NULL;
	INCR(bufCount);

	buf->next = NULL;
	buf->data = malloc(dataLength);
	if (!buf->data) {
	    NSN_FREE(buf);
	    DECR(bufCount);
	    return NULL;
	}

	XP_MEMCPY(buf->data, bytes, length);
	buf->dataLength = dataLength;
	buf->readCursor = 0;
	buf->writeCursor = length;

	nsn_DataQueue_insert(queue, buf);
    }

    return buf;
}

/* Returns the amount actually read: */
long
nsn_DataQueue_readBytes(nsn_DataQueue* queue, char* intoBuf, long bufLen)
{
    nsn_DataQueueElement* buf = queue->head;
    long remaining = bufLen;

    while (buf != NULL && remaining > 0) {
	nsn_DataQueueElement* next = buf->next;
	long consumed = buf->readCursor;
	long unconsumed = buf->writeCursor - consumed;
	long amount = MIN(remaining, unconsumed);
	XP_MEMCPY(intoBuf, &buf->data[consumed], amount);
	intoBuf += amount;
	remaining -= amount;
	if (amount == unconsumed) {
	    /* Can't call NSN_FREE on buf->data because sizeof can't 
	       tell us it's length: */
	    free((void*)buf->data);
	    NSN_FREE(buf);
	    DECR(bufCount);
	    buf = next;
	} else {
	    buf->readCursor += amount;
	    break;
	}
    }
    queue->head = buf;
    if (buf == NULL)
	queue->tail = NULL;
    return bufLen - remaining;
}

long
nsn_DataQueue_length(nsn_DataQueue* queue)
{
    long length = 0;
    nsn_DataQueueElement* buf = queue->head;
    while (buf != NULL) {
	length += buf->writeCursor - buf->readCursor;
	buf = buf->next;
    }
    return length;
}

void
nsn_DataQueue_clear(nsn_DataQueue* queue)
{
    nsn_DataQueueElement* buf = queue->head;
    while (buf != NULL) {
	nsn_DataQueueElement* next = buf->next;
	free(buf->data);
	NSN_FREE(buf);
	DECR(bufCount);
	buf = next;
    }
    queue->head = NULL;
    queue->tail = NULL;
}

/******************************************************************************/

void
nsn_JavaStream_abort(nsn_JavaStreamData* javaData, int status)
{
    /*
    ** This method is called whenever netlib initiates the closing of a
    ** connection.
    */

    ENTER_METHOD("JavaStream.abort", SILENTLY_FAIL)
    javaData->errorStatus = status;

    if (status < 0) {
        /*
        ** If we got back an error then close the stream in java
        ** land.
        */
        if (javaData->url->error_msg != NULL)
	    javaData->errorMessage = XP_STRDUP(javaData->url->error_msg);
    }

    EXIT_METHOD("JavaStream.abort")
}

/* XXX Move these later: */
#define HTTP_SERVER_STATUS_OK			200
#define HTTP_SERVER_STATUS_USE_LOCAL_COPY	304

PRIVATE void
nsn_JavaStream_complete(void* data)
{
    /* When the Java stream is complete (all data has arrived) we destroy
       the NET_StreamClass object, indicating that the net stream is finished.
       Whatever input is left in the JavaStreamData's DataQueue is left 
       for the reader. In the Java world, the stream is not considered 
       closed until all the data is read, or until it is explicitly closed
       or garbage collected. */

    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)data;
    int status;

    if (javaData == NULL) return;

    PR_LOG_BEGIN(NSJAVA, debug,
		 ("JavaStream.complete(%s)", URL_ADDR(javaData)));
    
    status = javaData->url->server_status;
    nsn_JavaStream_abort(javaData,
			 ((status == 0 ||
			   status == HTTP_SERVER_STATUS_OK ||
			   status == HTTP_SERVER_STATUS_USE_LOCAL_COPY)
			  ? 1 : -1));

    PR_LOG_END(NSJAVA, debug,
	       ("JavaStream.complete(%s)", URL_ADDR(javaData)));
}

PRIVATE unsigned int
nsn_JavaStream_writeReady(void* data)
{
    /* We always return MAX_WRITE_READY because we can consume it all
       by putting the data in the input buffer. */

    PR_LOG(NSJAVA, debug,
	   ("JavaStream.writeReady = %x", MAX_WRITE_READY));
    return MAX_WRITE_READY;	/* XXX should this be some max size for Windows 3.1? */
}

PRIVATE int
nsn_JavaStream_write(void* data, const char* str, int32 length)
{
    /*
    ** JavaStream_Write puts the data in the JavaStreamData's
    ** DataQueueElement:
    */
    int err = MK_UNABLE_TO_CONVERT; /* will cause an abort */
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)data;

    ENTER_METHOD("JavaStream.write", SILENTLY_FAIL)

    switch (javaData->state) {
      case OPENED: {
	  nsn_DataQueueElement* buf = 
	      nsn_DataQueue_writeBytes(&javaData->dataQueue, str, length);

	  if (buf != NULL) {
	      err = 0;	/* succeeded: clear the error */
	  }
	  /* also do the notify and exit monitor, below */
	  break;
      }
      default:
	PR_LOG(NSJAVA, debug,
	       ("JavaStream.write(%s), %d bytes -- stream closed!", 
		URL_ADDR(javaData), length));
	break;
    }

    (javaData->notifies)++;

    /* Notify any waiting readers that new data is here: */
    PR_LOG(NSJAVA, debug,
	   ("JavaStream.write(%s) calling monitorNotify", 
	    URL_ADDR(javaData)));
    monitorNotifyAll(nsn_JavaStream_monitor(javaData));

    EXIT_METHOD("JavaStream.write")
    return err;
}

PRIVATE NET_StreamClass*
nsn_JavaStream_make(int formatOut,
		    void* data,
		    URL_Struct* url,
		    MWContext* window_id)
{
    NET_StreamClass* netStream;
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)url->fe_data;

    if (javaData == NULL) return NULL;		/* javaData was garbage collected */

    /* Java may have closed this stream before netlib could open it: */
    if (javaData->state != OPENING)
	return NULL;

    PR_LOG_BEGIN(NSJAVA, debug,
		 ("JavaStream.make(%s)", url->address));

    netStream = PR_NEW(NET_StreamClass);
    if (netStream == NULL) 
	return NULL;
    
    XP_MEMSET(netStream, 0, sizeof(NET_StreamClass));

    netStream->name           = "Java Stream";
    netStream->complete       = nsn_JavaStream_complete;
    netStream->abort          = (MKStreamAbortFunc)nsn_JavaStream_abort;
    netStream->put_block      = nsn_JavaStream_write;
    netStream->is_write_ready = nsn_JavaStream_writeReady;
    netStream->data_object    = url->fe_data;	/* the javaData struct */
    netStream->window_id      = window_id;

    PR_LOG(NSJAVA, debug,
	   ("JavaStream.make(%s) entering monitor", url->address));
    monitorEnter(nsn_JavaStream_monitor(javaData));

    /* We must be in the monitor to manipulate the javaData object: */
    javaData->netStream = netStream;
    javaData->state = OPENED;

    (javaData->notifies)++;

    /*
    ** Signal to netscape_net_URLConnection_pConnect that the netStream
    ** is created and that it may be retrieved from the url:
    */
    PR_LOG(NSJAVA, debug,
	   ("JavaStream.make(%s) calling monitorNotify", url->address));
    monitorNotify(nsn_JavaStream_monitor(javaData));

    INCR(streamCount);
    monitorExit(nsn_JavaStream_monitor(javaData));

    PR_LOG_END(NSJAVA, debug,
	       ("JavaStream.make(%s)", url->address));
    return netStream;
}

PUBLIC void
NSN_RegisterJavaConverter(void)
{
    NET_RegisterContentTypeConverter("*", FO_DO_JAVA, NULL,
				     nsn_JavaStream_make);
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_DO_JAVA, NULL,
				     NET_CacheConverter);
}

/******************************************************************************/

PUBLIC void
netscape_net_URLStreamHandlerFactory_pInit(
    struct Hnetscape_net_URLStreamHandlerFactory* bogus)
{
    PR_LOG_BEGIN(NSJAVA, debug, ("netscape_net_URLStreamHandlerFactory_pInit"));
    PR_LOG_END(NSJAVA, debug, ("netscape_net_URLStreamHandlerFactory_pInit"));
}

extern int NET_URL_Type(const char* url);	/* should be in a header file */

PUBLIC long
netscape_net_URLStreamHandlerFactory_pSupportsProtocol(
    struct Hnetscape_net_URLStreamHandlerFactory* factory,
    struct Hjava_lang_String* protocol)
{
    #define BUF_SIZE 128
    char str[BUF_SIZE];
    int32 len;
    int type;

    javaString2CString(protocol, str, BUF_SIZE);
    len = javaStringLength(protocol);
    if (len+3 < BUF_SIZE) {
	str[len+0] = ':';
        str[len+1] = '/';
	str[len+2] = '\0';
	type = NET_URL_Type(str);
	return (type != 0);
    } else
	return 0;
}

/******************************************************************************/

PUBLIC void
nsn_CloseJavaConnection(MWContext* javaContext)
{
    nsn_JavaStreamData* javaData = javaContext->javaContextStreamData;

    if (javaData == NULL) return;		/* javaData was garbage collected */

    nsn_JavaStream_javaClose(javaData);
}

PUBLIC MWContext *
nsn_JavaContextToRealContext(MWContext *cx)
{
    if ( ( cx == NULL ) || ( cx->type != MWContextJava ) ||
	( cx->javaContextStreamData == NULL ) ) {
	return(cx);
    }

    return ( cx->javaContextStreamData->realContext );
}

/******************************************************************************/
