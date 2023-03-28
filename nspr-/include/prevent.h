#ifndef prevent_h___
#define prevent_h___

#include "prclist.h"
#include "prtypes.h"

NSPR_BEGIN_EXTERN_C

/*******************************************************************************
 * Event Queue Operations
 ******************************************************************************/

/*
** Creates a new event queue. Returns NULL on failure.
*/
extern PR_PUBLIC_API(PREventQueue*)
PR_CreateEventQueue(char* name);

/*
** Destroys an event queue.
*/
extern PR_PUBLIC_API(void)
PR_DestroyEventQueue(PREventQueue* self);

/*
** Posts an event to an event queue, waking up any threads waiting for an
** event. If event is NULL, notification still occurs, but no event will
** be available.
*/
extern PR_PUBLIC_API(void)
PR_PostEvent(PREventQueue* self, PREvent* event);

/*
** Like PR_PostEvent, this routine posts an event to the event handling
** thread, but does so synchronously, waiting for the result. The result
** which is the value of the handler routine is returned.
*/
extern PR_PUBLIC_API(void*)
PR_PostSynchronousEvent(PREventQueue* self, PREvent* event);

/*
** Gets an event from an event queue. Returns NULL if no event is
** available.
*/
extern PR_PUBLIC_API(PREvent*)
PR_GetEvent(PREventQueue* self);

/*
** Returns true if there is an event available for PR_GetEvent.
*/
extern PR_PUBLIC_API(PRBool)
PR_EventAvailable(PREventQueue* self);

/*
** This is the type of the function that must be passed to PR_MapEvents
** (see description below).
*/
typedef void 
(PR_CALLBACK *PR_EventFunPtr)(PREvent* event, void* data, PREventQueue* queue);

/*
** Applies a function to every event in the event queue. This can be used
** to selectively handle, filter, or remove events. The data pointer is
** passed to each invocation of the function fun.
*/
extern PR_PUBLIC_API(void)
PR_MapEvents(PREventQueue* self, PR_EventFunPtr fun, void* data);

#ifdef XP_UNIX

/*
** This Unix-specific routine allows you to grab the file descriptor
** associated with an event queue and use it in the readFD set of select.
*/
extern PR_PUBLIC_API(int)
PR_GetEventQueueSelectFD(PREventQueue* self);

#endif

/*******************************************************************************
 * Event Operations
 ******************************************************************************/

/*
** The type of an event handler function. This function is passed as an
** initialization argument to PR_InitEvent, and called by
** PR_HandleEvent. If the event is called synchronously, a void* result 
** may be returned (otherwise any result will be ignored).
*/
typedef void*
(*PRHandleEventProc)(PREvent* self);

/*
** The type of an event destructor function. This function is passed as
** an initialization argument to PR_InitEvent, and called by
** PR_DestroyEvent.
*/
typedef void
(*PRDestroyEventProc)(PREvent* self);

/*
** Initializes an event. Usually events are embedded in a larger event
** structure which holds event-specific data, so this is an initializer
** for that embedded part of the structure.
*/
extern PR_PUBLIC_API(void)
PR_InitEvent(PREvent* self,
	     PRHandleEventProc handler,
	     PRDestroyEventProc destructor);

/*
** Handles an event, calling the event's handler routine.
*/
extern PR_PUBLIC_API(void)
PR_HandleEvent(PREvent* self);

/*
** Destroys an event, calling the event's destructor.
*/
extern PR_PUBLIC_API(void)
PR_DestroyEvent(PREvent* self);

/*
** Removes an event from an event queue.
*/
extern PR_PUBLIC_API(void)
PR_DequeueEvent(PREvent* self, PREventQueue* queue);

/*
** Returns the type of an event.
*/
#define PR_EventType(self)	((self)->type)

/*******************************************************************************
 * Private Stuff
 ******************************************************************************/

struct PREventQueueStr {
    char*	name;
    PRMonitor*	monitor;
    PRCList	queue;
};

struct PREventStr {
    PRCList			link;
    PRHandleEventProc		handler;
    PRDestroyEventProc		destructor;
    void*			synchronousResult;
    /* other fields follow... */
};

#define PR_EVENT_PTR(_qp) \
    ((PREvent*) ((char*) (_qp) - offsetof(PREvent,link)))

/******************************************************************************/

NSPR_END_EXTERN_C

#endif /* prevent_h___ */
