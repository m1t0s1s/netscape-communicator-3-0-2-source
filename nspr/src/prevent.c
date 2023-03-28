#include "prevent.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include <errno.h>
#include <stddef.h>

/*******************************************************************************
 * Event Queue Operations
 ******************************************************************************/

PR_PUBLIC_API(PREventQueue*)
PR_CreateEventQueue(char* name)
{
    int err;
    PREventQueue* self = NULL;
    PRMonitor* mon = NULL;

    self = PR_NEW(PREventQueue);
    if (self == NULL) return NULL;

    mon = PR_NewNamedMonitor(0, name);
    if (mon == NULL) goto error;

    err = PR_MakeMonitorSelectable(mon);
    if (err) goto error;

    self->name = name;
    self->monitor = mon;
    PR_INIT_CLIST(&self->queue);
    return self;

  error:
    if (mon != NULL)
	PR_DestroyMonitor(mon);
    PR_DELETE(self);
    return NULL;
}

void PR_CALLBACK _pr_destroyEvent(PREvent* event, void* data, PREventQueue* queue)
{
    PR_DestroyEvent(event);
}

PR_PUBLIC_API(void)
PR_DestroyEventQueue(PREventQueue* self)
{
    /* destroy undelivered events */
    PR_MapEvents(self, _pr_destroyEvent, NULL);

    /* destroying the monitor also destroys the name */
    PR_DestroyMonitor(self->monitor);
    PR_DELETE(self);
}

PR_PUBLIC_API(void)
PR_PostEvent(PREventQueue* self, PREvent* event)
{
    PR_EnterMonitor(self->monitor);

    /* insert event into thread's event queue: */
    if (event != NULL) {
	PR_APPEND_LINK(&event->link, &self->queue);
    }

    PR_SelectNotify(self->monitor);

    PR_ExitMonitor(self->monitor);
}

PR_PUBLIC_API(void*)
PR_PostSynchronousEvent(PREventQueue* self, PREvent* event)
{
    void* result;
    PR_CEnterMonitor(event);
    event->synchronousResult = (void*)PR_TRUE;
    PR_PostEvent(self, event);
    PR_CWait(event, LL_MAXINT);
    result = event->synchronousResult;
    event->synchronousResult = NULL;
    PR_CExitMonitor(event);
    return result;
}

PR_PUBLIC_API(PREvent*)
PR_GetEvent(PREventQueue* self)
{
    PREvent* event = NULL;

    PR_EnterMonitor(self->monitor);

    /* consume the byte NotifySelect put in our pipe: */
    PR_ClearSelectNotify(self->monitor);

    if (!PR_CLIST_IS_EMPTY(&self->queue)) {
	/* then grab the event and return it: */
	event = PR_EVENT_PTR(self->queue.next);
	PR_REMOVE_LINK(&event->link);
    }

    PR_ExitMonitor(self->monitor);
    return event;
}

PR_PUBLIC_API(PRBool)
PR_EventAvailable(PREventQueue* self)
{
    PRBool result = PR_FALSE;
    PR_EnterMonitor(self->monitor);

    if (!PR_CLIST_IS_EMPTY(&self->queue)) 
	result = PR_TRUE;

    PR_ExitMonitor(self->monitor);
    return result;
}

PR_PUBLIC_API(void)
PR_MapEvents(PREventQueue* self, PR_EventFunPtr fun, void* data)
{
    PRCList* qp = self->queue.next;
    while (qp != &self->queue) {
	PREvent* event = PR_EVENT_PTR(qp);
	qp = qp->next;
	(*fun)(event, data, self);
    }
}

#ifdef XP_UNIX

PR_PUBLIC_API(int)
PR_GetEventQueueSelectFD(PREventQueue* self)
{
    return PR_GetSelectFD(self->monitor);
}

#else 
#if !defined(XP_PC) && !defined(XP_MAC)

#error Mac guys!

#endif
#endif

/*******************************************************************************
 * Event Operations
 ******************************************************************************/

PR_PUBLIC_API(void)
PR_InitEvent(PREvent* self,
	     PRHandleEventProc handler,
	     PRDestroyEventProc destructor)
{
    PR_INIT_CLIST(&self->link);
    self->handler = handler;
    self->destructor = destructor;
    self->synchronousResult = NULL;
}

PR_PUBLIC_API(void)
PR_HandleEvent(PREvent* self)
{
    void* result;
    PRBool synchronous = (PRBool)self->synchronousResult;
    if (self != NULL)
	result = (*self->handler)(self);
    if (synchronous) {
	PR_CEnterMonitor(self);
	self->synchronousResult = result;
	PR_CNotify(self);
	PR_CExitMonitor(self);
    }
}

PR_PUBLIC_API(void)
PR_DestroyEvent(PREvent* self)
{
    if (self != NULL)
	(*self->destructor)(self);
}

PR_PUBLIC_API(void)
PR_DequeueEvent(PREvent* self, PREventQueue* queue)
{
    /*
    ** This implementation assumes that you know what you're doing and
    ** the event is actually in the queue. We could check, but we don't
    ** have time.
    */
    PR_REMOVE_LINK(&self->link);
}

/******************************************************************************/
