/*
 * @(#)inStr.c	1.2 95/06/14 Warren Harris
 *
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 *
 */

#include "nsn.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prthread.h"

#include "netscape_net_URLConnection.h"
#include "netscape_net_URLInputStream.h"
#include "javaString.h"
#include "javaThreads.h"

nsn_TickleHookProcPtr	g_nsn_TickleHookProc = NULL;

/******************************************************************************/

PUBLIC void nsn_InstallTickleHookProc(nsn_TickleHookProcPtr newProc)
{
	g_nsn_TickleHookProc = newProc;
}

PRIVATE void
nsn_GetURL_exit(URL_Struct* url, int status, MWContext* window_id)
{
    /*
    ** This method is called whenever netlib initiates the closing of a
    ** connection.
    **
    ** When the Java stream is complete (all data has arrived) we destroy
    ** the NET_StreamClass object, indicating that the net stream is
    ** finished.  Whatever input is left in the JavaStreamData's
    ** DataQueue is left for the reader. In the Java world, the stream is
    ** not considered closed until all the data is read, or until it is
    ** explicitly closed or garbage collected.
    */
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)url->fe_data;


    ENTER_METHOD("GetURL_Exit", SILENTLY_FAIL)

    if( (javaData->errorStatus >= 0) && (status < 0) ) {
	/*
	** If we got back an error then remember it...
	*/
	if (url->error_msg != NULL)
	    javaData->errorMessage = XP_STRDUP(url->error_msg);

	javaData->errorStatus = status;
    }

    switch (javaData->state) {
      case OPENED:
	/* In this case, libnet is done with the stream first: */
	javaData->state = LIBNET_DONE;

	/* 
	** Important: this will close down the entire connection because the 
	** state is now LIBNET_DONE !! 
	*/
        if( javaData->errorStatus < 0 ) {
	    nsn_JavaStream_javaClose(javaData);
	}

	PR_LOG(NSJAVA, debug,
	       ("GetURL_exit(%s) libnet done, status=%d", 
		URL_ADDR(javaData), status));
	break;

      case LIBNET_DONE:
      case CLOSED:
	PR_LOG(NSJAVA, debug,
	       ("GetURL_exit(%s) stream already closed!", 
		URL_ADDR(javaData)));
	PR_ASSERT(0); /* Someone incorrectly advanced the state machine :-( */
	break;

      case OPENING:
	/*
	** If abort was called because the NET_GetURL immediately failed
	** (which is why we're still in the OPENING state)...
	*/
	PR_LOG(NSJAVA, debug,
	       ("GetURL_exit(%s) libnet opening", URL_ADDR(javaData)));
	nsn_JavaStream_javaClose(javaData);

	/* fall through */

      case JAVA_DONE:
	/*
	** In this case, java has already indicated it's desire to close
	** the stream, so libnet owns the url. 
	*/
	nsn_JavaStream_close(javaData);

	PR_LOG(NSJAVA, debug,
	       ("GetURL_exit(%s) libnet closed", URL_ADDR(javaData)));
	break;
      case POSTING:
      default:
	PR_ASSERT(0);	/* all bases should be covered, above */
	break;
    }

    (javaData->notifies)++;

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
	}

    /* Notify any waiting readers that the stream has ended: */
    PR_LOG(NSJAVA, debug,
	   ("(GetURL_exit%s) calling monitorNotifyAll",
	    URL_ADDR(javaData)));
    monitorNotifyAll(nsn_JavaStream_monitor(javaData));

    EXIT_METHOD("GetURL_exit")
}

PUBLIC void
netscape_net_URLInputStream_open(struct Hnetscape_net_URLInputStream* this)
{
    Hnetscape_net_URLConnection* con_h = unhand(this)->connection;
    Classnetscape_net_URLConnection* con = unhand(con_h);
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)con->pStreamData;
    FO_Present_Types presentType;
    URL_Struct* url;
    char* postHeaders;
    JRIEnv* env = JRI_GetCurrentEnv();

    PR_ASSERT(javaData != NULL);		/* javaData was garbage collected */
    /*
    ** XXX Currently we don't allow network activity from the mozilla
    ** thread. This is because we need to call wait to make this stuff
    ** work, but that locks out netlib (which runs on the mozilla
    ** thread). So for now, we'll throw an exception.
    */
    if (LJ_MozillaRunning(env) == PR_TRUE) {
      SignalError(0, "java/lang/InternalError",
		  "Can't call network functions from JavaScript yet.");
      return;
    }

    PR_LOG_BEGIN(NSJAVA, debug,
		 ("URLInputStream.open(%s)", URL_ADDR(javaData)));

    /* Enter the monitor to add the final touches to the post_headers */
    monitorEnter(nsn_JavaStream_monitor(javaData));

    switch (javaData->state) {
      case OPENING:
	/* The usual case */
	break;
      case OPENED:
      case LIBNET_DONE:
	/* Herb says to silently succeed. */
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) already opened: state %d",
		URL_ADDR(javaData), javaData->state));
	monitorExit(nsn_JavaStream_monitor(javaData));
	return;
      case POSTING:
      case JAVA_DONE:
      case CLOSED:
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) stream closed: state %d",
		URL_ADDR(javaData), javaData->state));
	SignalError(0, "java/io/IOException", "stream closed");
	monitorExit(nsn_JavaStream_monitor(javaData));
	return;
      default:
	PR_ASSERT(0);	/* should never get here */
	break;
    }

    presentType = con->useCaches ? FO_CACHE_AND_DO_JAVA : FO_DO_JAVA;
    url = javaData->url;
    postHeaders = makeCString(con->postHeaders); /* freed by gc */
    
    if (url->post_data != NULL) {
	/*
	** Add a Content-length. Subtract 2 for the size of the final
	** CRLF that we tacked on in netscape_net_URLOutputStream_pClose:
	*/
	url->post_headers =
	    PR_smprintf("%sContent-length: %ld" CRLF,
			postHeaders, url->post_data_size - 2);

	url->post_data_is_file = FALSE;
	url->method = URL_POST_METHOD;
	con->postHeaders = NULL;	/* zero it out for next time */

	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) calling NET_GetURL with post data",
		URL_ADDR(javaData)));
    }
    else {
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) calling NET_GetURL",
		URL_ADDR(javaData)));
    }

    monitorExit(nsn_JavaStream_monitor(javaData));

    /*
    ** Lou tells me that the status returned from NET_GetURL is not the
    ** error code, and that that gets passed to you through the abort
    ** routine. We're looking for that below in the case the stream was
    ** both created and terminated on our thread (we better get both, or
    ** we're hosed). So what's the result for?
    */
    (void)NET_GetURL(url, presentType, javaData->javaContext, nsn_GetURL_exit);
    /*
    ** Enter our monitor before checking the javaData state because
    ** nsn_JavaStream_make is going to change it or notify us when it's
    ** ready. We'd hate to get notified before we waited. Also, we can't
    ** enter the monitor before calling NET_GetURL because it will grab
    ** the netlib lock and cause a deadlock with the mozilla thread. The
    ** deadlock comes when mozilla wants to destroy the applet and its
    ** connection while we're still waiting to get connected.
    */
    monitorEnter(nsn_JavaStream_monitor(javaData));

    /*
    ** There are two cases for waiting for the make proc to be
    ** called. Sometimes the make proc is called by this thread during
    ** the call to NET_GetURL; sometimes its called by some other thread
    ** later. If it's called by ourselves then a useless notify will be
    ** done before we get to this line of code and the state will not be
    ** OPENING. If it's done by some other thread, then the other thread
    ** will block on the monitor we are holding until we release it, and
    ** therefore there is no race on the state change.
    */
    if (javaData->state == OPENING) {
	/*
	** Then wait for it to be created on the upcall:
	*/
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) monitorWait",
		URL_ADDR(javaData)));
	
	/* On the mac (and most likely win16), Net_ProcessNet and 
	** network tickling can only occur on the main thread.  
	** therefore, we provide a hook here for the case that the
	** main thread needs to tickle itself.  In this case,
	** we make sure that we give up the monitor so that the
	** tickle code can notify it without freezing.
	** dkc 2/13/96
	*/

	if (g_nsn_TickleHookProc != NULL) {
		monitorExit(nsn_JavaStream_monitor(javaData));
		while (1) {
			if ((*g_nsn_TickleHookProc)() || (javaData->state != OPENING))
				break;
		}
		monitorEnter(nsn_JavaStream_monitor(javaData));
	}
	
	/* If the stream still isn't open after tickling, then
	** wait on the monitor.
	*/
	
	if (javaData->state == OPENING)
		monitorWait(nsn_JavaStream_monitor(javaData), TIMEOUT_INFINITY);

	if (exceptionOccurred(EE())) {
	    /* we may get ThreadDeath */
	    PR_LOG(NSJAVA, debug,
		   ("URLInputStream.open(%s) exception occurred",
		    URL_ADDR(javaData)));
	    monitorExit(nsn_JavaStream_monitor(javaData));
	    return;
	}
	/*
	** We better should have either gotten an exception, or a
	** notification that we were leaving the OPENING state.
	*/
	PR_ASSERT(javaData->state != OPENING);
    }
    else {
	/*
	** Otherwise, we've already done the NET_GetURL on our thread, so
	** just check the error status (below).
	*/
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) monitorWait wasn't needed, status=%d: %s",
		URL_ADDR(javaData), javaData->errorStatus, FIX_NULL(javaData->errorMessage)));
    }
    if (javaData->errorStatus < 0) {
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.open(%s) raising IOException: %s",
		URL_ADDR(javaData), FIX_NULL(javaData->errorMessage)));
	monitorExit(nsn_JavaStream_monitor(javaData));
	SignalError(0, "java/io/IOException", FIX_NULL(javaData->errorMessage));
	return;
    }
    
    PR_LOG_END(NSJAVA, debug,
	       ("URLInputStream.open(%s)", URL_ADDR(javaData)));
    monitorExit(nsn_JavaStream_monitor(javaData));
}

PUBLIC long
netscape_net_URLInputStream_read(struct Hnetscape_net_URLInputStream* this,
				 HArrayOfByte* bytes, long offset, long length)
{
    Hnetscape_net_URLConnection* con = unhand(this)->connection;
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(con)->pStreamData;
    long datalen;
    long remaining;
    long result = -1;	/* java EOF */
    int tickleAbort;

    ENTER_METHOD("URLInputStream.read", IO_EXCEPTION)

    switch (javaData->state) {
      case OPENED:
      case LIBNET_DONE:
	datalen = obj_length(bytes);
	if ((offset < 0) || (offset > datalen)) {
	    SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	    break;
	}
	if (offset + length > datalen) {
	    length = datalen - offset;
	}
	remaining = length;

	/* Check if there's buffered input to consume: */
	while (javaData->dataQueue.head == NULL) {

	    if (javaData->state == LIBNET_DONE) {
		/* If there's no buffered input, and libnet is done,
		   there's not going to be anymore: */
		PR_LOG(NSJAVA, debug,
		       ("URLInputStream.read(%s), %d bytes, => EOF",
			URL_ADDR(javaData), length));
		goto done;
	    }

	    /* Wait for input: */
	    PR_LOG(NSJAVA, debug,
		   ("URLInputStream.read(%s) monitorWait",
		    URL_ADDR(javaData)));
		/* If there is a tickle proc installed, then we should call
		** it instead of mon-waiting immediately in case we are being
		** called by the main thread.  This will prevent deadlock
		** where the main thread would wait forever to be notified
		** by itself (via NET_ProcessNET.  If the tickle procedure  
		** returns false (continue tickling), then we repeat the tickle 
		** tickle until the stream is aborted or a write notifies,
		** which we can tell from the counter javaData->notifies 
		** incrementing. 
		*/
		tickleAbort = 1;
	    if (g_nsn_TickleHookProc != NULL) {
			int beforeNotifies = javaData->notifies;
		    while (1) {
		    	tickleAbort = (*g_nsn_TickleHookProc)();
		        if (tickleAbort || (javaData->notifies != beforeNotifies))
		            break;
		    }
	    }
	    if (tickleAbort)
		    monitorWait(nsn_JavaStream_monitor(javaData), TIMEOUT_INFINITY);
	    PR_LOG(NSJAVA, debug,
		   ("URLInputStream.read(%s) monitorWait",
		    URL_ADDR(javaData)));
	    if (exceptionOccurred(EE())) {
		/* we may get ThreadDeath */
		PR_LOG(NSJAVA, debug,
		       ("URLInputStream.read(%s), %d bytes, => exception occurred",
			URL_ADDR(javaData), length));
		goto done;
	    }

	    /* When we return from Wait, the stream may have been closed. Check: */
	    if (javaData->state != OPENED && javaData->state != LIBNET_DONE) {
		PR_LOG(NSJAVA, debug,
		       ("URLInputStream.read(%s), %d bytes, => stream closed",
			URL_ADDR(javaData), length));
		SignalError(0, "java/io/IOException", javaData->errorMessage);
		goto done;
	    }
	}

	/* Snarf whatever data's available: */
	result = nsn_DataQueue_readBytes(&javaData->dataQueue,
					 &(unhand(bytes)->body)[offset],
					 remaining);
	break;
      default:
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.read(%s), %d bytes, => stream not opened",
		URL_ADDR(javaData), length));
	SignalError(0, "java/io/IOException",
		    (javaData->errorMessage ? javaData->errorMessage : "stream closed"));
	break;
    }
  done:
    EXIT_METHOD("URLInputStream.read")
    return result;
}

PUBLIC long
netscape_net_URLInputStream_available(struct Hnetscape_net_URLInputStream* this)
{
    Hnetscape_net_URLConnection* con = unhand(this)->connection;
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(con)->pStreamData;
    long result = 0;

    ENTER_METHOD("URLInputStream.available", IO_EXCEPTION)

    switch (javaData->state) {
      case OPENED:
      case LIBNET_DONE:
	result = nsn_DataQueue_length(&javaData->dataQueue);
	break;
      default:
	PR_LOG(NSJAVA, debug,
	       ("URLInputStream.available(%s), raising IOException",
		URL_ADDR(javaData)));
	SignalError(0, "java/io/IOException",
		    (javaData->errorMessage ? javaData->errorMessage : "stream closed"));
	break;
    }

    EXIT_METHOD("URLInputStream.available")
    return result;
}

/******************************************************************************/
