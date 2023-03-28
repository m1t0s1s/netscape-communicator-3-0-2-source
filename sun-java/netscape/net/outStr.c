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

#include "netscape_net_URLConnection.h"
#include "netscape_net_URLOutputStream.h"
#include "javaString.h"
#include "javaThreads.h"

/******************************************************************************/

PUBLIC void
netscape_net_URLOutputStream_open(struct Hnetscape_net_URLOutputStream* this)
{
    Hnetscape_net_URLConnection* con = unhand(this)->connection;
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(con)->pStreamData;

    ENTER_METHOD("URLOutputStream.open", IO_EXCEPTION)

    /*
    ** Right now we don't really open a connection like we do with
    ** URLInputStream.open (i.e. a NET_GetURL) because we don't have a
    ** streamable posting mechanism. Instead we're going to gather all
    ** the data up in the DataBuffer and send it when we close this
    ** stream.
    */
    
    switch (javaData->state) {
      case OPENING:
	/*
	** If we're opening this stream, then everything's cool. But if
	** we're in any other state we have to stop what we were doing
	** before, i.e. close the stream.
	*/

	javaData->state = POSTING;
	break;
      case POSTING:
	/* Silently succeed */
	break;
      default:
	/*
	** This theoretically can't happen. The URLConnection code that
	** calls currentOutputStream.open should never be called if the
	** connection has already been connected. But throw an exception
	** just to be safe.
	*/
	SignalError(0, "java/io/IOException", "stream closed");
	break;
    }

    EXIT_METHOD("URLOutputStream.open")
}

PRIVATE void
nsn_URLOutputStream_writeBytes(struct Hnetscape_net_URLOutputStream* this,
			       char* bytes, long length)
{
    Hnetscape_net_URLConnection* con = unhand(this)->connection;
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)unhand(con)->pStreamData;

    ENTER_METHOD("URLOutputStream.writeBytes", IO_EXCEPTION)

    switch (javaData->state) {
      case POSTING: {
	  nsn_DataQueueElement* buf = 
	      nsn_DataQueue_writeBytes(&javaData->dataQueue, bytes, length);

	  if (buf == NULL) {
	      SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	      /* also do the exit monitor, below */
	  }
	  break;
      }
      default:
	PR_LOG(NSJAVA, debug,
	       ("nsn_URLOutputStream_writeBytes(%s), %d bytes -- stream closed!", 
		URL_ADDR(javaData), length));
	SignalError(0, "java/io/IOException", "stream closed");
	break;
    }

    EXIT_METHOD("URLOutputStream.writeBytes")
}

PUBLIC void
netscape_net_URLOutputStream_writeBytes(struct Hnetscape_net_URLOutputStream* this,
					HArrayOfByte* bytes, long offset, long length)
{
    char* body = &(unhand(bytes)->body)[offset];
    long datalen = obj_length(bytes);
    long remaining;
    
    if ((offset < 0) || (offset > datalen)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return;
    }
    if (offset + length > datalen) {
	length = datalen - offset;
    }
    remaining = length;
    
    nsn_URLOutputStream_writeBytes(this, body, remaining);
}

PUBLIC void
netscape_net_URLOutputStream_write(struct Hnetscape_net_URLOutputStream* this,
				   long data)
{
    char byte = (char)(data & 0xFF);
    nsn_URLOutputStream_writeBytes(this, &byte, 1);
}

PUBLIC void
netscape_net_URLOutputStream_pClose(struct Hnetscape_net_URLOutputStream* this)
{
    Hnetscape_net_URLConnection* con_h = unhand(this)->connection;
    Classnetscape_net_URLConnection* con = unhand(con_h);
    nsn_JavaStreamData* javaData = (nsn_JavaStreamData*)con->pStreamData;
    long existingLength, newLength;
    char* postData;
    long amountRead;

    ENTER_METHOD("URLOutputStream.close", IO_EXCEPTION)

    switch (javaData->state) {
      case POSTING:
	/* Write a final CRLF because the protocol needs it. */
	nsn_URLOutputStream_writeBytes(this, CRLF, 2);
	if (exceptionOccurred(EE()))
	    break;

	/* XXX Warning for Win16: this length might be > 64k */
	existingLength = javaData->url->post_data_size;
	newLength = nsn_DataQueue_length(&javaData->dataQueue);

	postData = malloc(newLength + existingLength);	/* ouch */

	if (postData == NULL) {
	    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	    break;
	}

	/*
	** Someday hopefully, we'll stream this data out as we fill up
	** a buffer somewhere. But for now we have to accumulate it
	** all up into one big string and hand it to netlib.
	*/
	if (existingLength > 0) {
	    XP_MEMCPY(postData, javaData->url->post_data, existingLength);
	    NSN_FREE(javaData->url->post_data);
	}
	amountRead = nsn_DataQueue_readBytes(&javaData->dataQueue, 
					     postData + existingLength,
					     newLength);
	PR_ASSERT(amountRead == newLength);
	
	/*
	** Store the post data and its length in the connection
	** object. The URLInputStream.open method will pick this up
	** and use it.
	*/
	javaData->url->post_data = postData;
	javaData->url->post_data_size = newLength + existingLength;
	
	/*
	** Mark the stream as closed to output, and ready to open for
	** input:
	*/
	javaData->state = OPENING;

	break;
      default:
	break;
    }
    EXIT_METHOD("URLOutputStream.close")
}

/******************************************************************************/
