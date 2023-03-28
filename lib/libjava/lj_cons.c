/* -*- Mode: C; tab-width: 4; -*- */

#include "lj.h"
#include "prmem.h"
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#ifndef XP_MAC
#include "netscape_applet_MozillaAppletContext.h"
#else
#include "n_applet_MozillaAppletContext.h"
#endif

void (*lj_console_callback)(int state, void *arg);
void *lj_console_callback_arg;
int lj_console_showing;
/*
** Make the java console window visible.
*/
void LJ_ShowConsole(void)
{
    if( !lj_java_enabled )
	    return;
  
    if( ! lj_is_started && ! lj_init_failed ) {
        int rv;
	rv = LJ_StartupJava();
	if (rv != LJ_INIT_OK) {
	    lj_init_failed = 1;
	    return;
	}
    }

    if( lj_is_started ) {
        LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_showConsole);
        lj_console_showing = 1;
    }
}

/*
** Hide the java console window.
*/
void LJ_HideConsole(void)
{
    if( lj_is_started ) {
        LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_hideConsole);
        lj_console_showing = 0;
    }
}

/*
** Called by java when the console state actually changes.
*/
void _LJ_SetConsoleState(int state)
{
}

/*
** Predicate to see if the console window is currently visible.
*/
int LJ_IsConsoleShowing(void)
{
    return lj_console_showing;
}

/*
** Set the callback function that tells whomever cares what
** the current state of the console's visibility is.
*/
void LJ_SetConsoleShowCallback(void (*func)(int on, void* a), void *arg)
{
    lj_console_callback = func;
    lj_console_callback_arg = arg;
}

/******************************************************************************/

typedef struct MozillaEvent_SetConsoleState {
    MWContextEvent	ce;
    int32			state;
} MozillaEvent_SetConsoleState;

void PR_CALLBACK
lj_HandleEvent_SetConsoleState(MozillaEvent_SetConsoleState* e)
{
    lj_console_showing = e->state;
    if (lj_console_callback) {
		(*lj_console_callback)(e->state, lj_console_callback_arg);
    }
}

void PR_CALLBACK
lj_DestroyEvent_SetConsoleState(MozillaEvent_SetConsoleState* event)
{
    XP_FREE(event);
}

void
LJ_PostSetConsoleState(int state)
{
    MozillaEvent_SetConsoleState* event =
		PR_NEW(MozillaEvent_SetConsoleState);
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)lj_HandleEvent_SetConsoleState,
				 (PRDestroyEventProc)lj_DestroyEvent_SetConsoleState);
    event->ce.context = 0;
    event->state = state;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
}

/******************************************************************************/
