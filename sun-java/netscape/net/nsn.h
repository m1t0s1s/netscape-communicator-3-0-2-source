/*
 * @(#)nsn.h	1.2 95/06/14 Warren Harris
 *
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 *
 */

#ifndef _NSN_H_
#define _NSN_H_

#include "java.h"
#include "xp.h"
#include "net.h"
#include "java.h"

/******************************************************************************/

#ifdef DEBUG
#define NSN_CLEAR(x)	do { if (x) { XP_MEMSET(x, 0x01, sizeof(*(x))); }} while (0)
#else
#define NSN_CLEAR(x)	
#endif

#define NSN_FREE(x)	do { NSN_CLEAR(x); free(x); } while (0)
#define NSN_FREEIF(x)	if ((x) != NULL) NSN_FREE(x)

#undef  MAX
#define MAX(x, y)	(((x) > (y)) ? (x) : (y))

/******************************************************************************/

typedef struct nsn_DataQueueElement {
    struct nsn_DataQueueElement*	next;
    char*				data;
    int32				dataLength;
    int32				readCursor;
    int32				writeCursor;
} nsn_DataQueueElement;

typedef struct nsn_DataQueue {
    nsn_DataQueueElement*	head;
    nsn_DataQueueElement*	tail;
} nsn_DataQueue;

extern void
nsn_DataQueue_insert(nsn_DataQueue* queue, nsn_DataQueueElement* buf);

extern nsn_DataQueueElement*
nsn_DataQueue_writeBytes(nsn_DataQueue* queue, const char* bytes, long length);

/* Returns the amount actually read: */
extern long
nsn_DataQueue_readBytes(nsn_DataQueue* queue, char* intoBuf, long bufLen);

extern long
nsn_DataQueue_length(nsn_DataQueue* queue);

extern void
nsn_DataQueue_clear(nsn_DataQueue* queue);

/******************************************************************************/

typedef enum nsn_JavaStreamState {
    OPENING,	/* just created */
    POSTING,	/* accumulating data to post (opened for output) */
    OPENED,	/* opened for input */
    JAVA_DONE,	/* java's closed the stream, and netlib should clean it up */
    LIBNET_DONE,/* netlib closed the stream, and java should clean it up */
    CLOSED	/* really closed */
} nsn_JavaStreamState;

/* The JavaStreamData struct is protected by the monitor it contains. It is
   illegal to access any field of this struct without first entering the 
   monitor. */
typedef struct nsn_JavaStreamData {
    nsn_JavaStreamState	state;
    URL_Struct*		url;
    MWContext*		javaContext;
    MWContext*		realContext;
    NET_StreamClass*	netStream;
    nsn_DataQueue	dataQueue;
    int			refcount;
    int			notifies;
    int			errorStatus;
    char*		errorMessage;
} nsn_JavaStreamData;

/*
** Let the JavaStreamData structure be the key to the monitor cache:
*/
#define nsn_JavaStream_monitor(javaData)	((uint32)javaData)

/*
** This method is called whenever netlib initiates the closing of a
** connection.
*/
extern void
nsn_JavaStream_abort(nsn_JavaStreamData* javaData, int status);

/*
** This method is called whenever java initiates the closing of a
** connection.
*/
extern void
nsn_JavaStream_javaClose(nsn_JavaStreamData* javaData);

/*
** This method is called whenever both sides have decided to close.  It
** destroys the JavaStreamData's internal state.
*/
extern void
nsn_JavaStream_close(nsn_JavaStreamData* javaData);

/*
** This method is called to install a tickle hook for certain network
** calls on the mac to avoid deadlock.
*/

typedef int (* nsn_TickleHookProcPtr)(void);

extern void 
nsn_InstallTickleHookProc(nsn_TickleHookProcPtr newProc);

/******************************************************************************/

extern MWContext*
nsn_GetCurrentContext(void);

extern LJAppletData*
nsn_GetCurrentAppletData(void);

extern MWContext*
nsn_JavaContext_make(URL_Struct* url, struct nsn_JavaStreamData* javaData);

extern void
nsn_JavaContext_destroy(MWContext* context);

/******************************************************************************/
/* Message formatting things */
 
#define FIX_NULL(s)		(s == NULL ? "<null>" : s)

#define URL_ADDR(javaData)	((javaData)->url ? (javaData)->url->address : "?")

/******************************************************************************/
/*
** Helpful macros for entering and exiting native functions while
** synchronizing on the javaData structure. These macros assume there is
** a structure named javaData
*/

#define ENTER_METHOD(message, unconnectedAction)	  \
{							  \
    if (javaData == NULL) { unconnectedAction; }	  \
    monitorEnter(nsn_JavaStream_monitor(javaData));	  \
    PR_LOG_BEGIN(NSJAVA, debug,				  \
		 (message "(%s)\n", URL_ADDR(javaData))); \
    {							  \

/* Possible unconnectedActions: */
#define SILENTLY_FAIL					  \
    goto doneWithMethod;				  \

#define IO_EXCEPTION					  \
    SignalError(0, "java/io/IOException",		  \
		"connnection not established");		  \
    goto doneWithMethod;				  \

#define CANT_HAPPEN					  \
    PR_ASSERT(javaData != NULL);			  \
    goto doneWithMethod;	/* for non-debug */	  \

#define EXIT_METHOD(message)				  \
        ; /* in case there's a label before the macro */  \
    }							  \
    PR_LOG_END(NSJAVA, debug,				  \
	       (message "(%s)\n", URL_ADDR(javaData)));	  \
    monitorExit(nsn_JavaStream_monitor(javaData));	  \
  doneWithMethod: ;					  \
}							  \

/******************************************************************************/

#ifdef DEBUG

/*
** These counters are used to keep track of whether we're freeing all the
** objects we're creating:
*/
typedef struct nsn_Counters {
    int javaDataCount;
    int urlCount;
    int contextCount;
    int streamCount;
    int bufCount;
} nsn_Counters;

extern nsn_Counters nsnCounters;

#define INCR(counter)	(nsnCounters.counter++)
#define DECR(counter)	(nsnCounters.counter--)

#else

#define INCR(counter)	((void)0)
#define DECR(counter)	((void)0)

#endif /* DEBUG */

/******************************************************************************/

#endif /* _NSN_H_ */
