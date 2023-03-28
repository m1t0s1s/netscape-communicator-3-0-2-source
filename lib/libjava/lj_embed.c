/* -*- Mode: C; tab-width: 4; -*- */

/* From mozilla */
#include "lj.h"
#include "client.h"
#include "structs.h"
#include "shist.h"
#include "fe_proto.h"
#include "libmocha.h"
#include "layout.h"

/* From nspr */
#include "prfile.h"
#include "prlog.h"
#include "prmem.h"
#include "prmon.h"

#include "oobj.h"
#include "interpreter.h"		/* for CLASSPATH() */

#define IMPLEMENT_netscape_applet_MozillaAppletContext
#ifndef XP_MAC
#include "netscape_applet_MozillaAppletContext.h"
#else
#include "n_applet_MozillaAppletContext.h"
#endif

extern PRLogModule NSJAVALog;

int lj_init_failed = 0;
int lj_java_enabled = 1;

#ifdef xDEBUG
static void
DumpArgs(LO_JavaAppStruct *java)
{
    int i;
    char *cp;

    fprintf(stderr, "<applet ");

    PA_LOCK(cp, char*, java->attr_code);
    fprintf(stderr, "code=%s ", cp ? cp : "NULL");
    PA_UNLOCK(java->attr_code);

    PA_LOCK(cp, char*, java->attr_codebase);
    fprintf(stderr, "codebase=%s ", cp ? cp : "NULL");
    PA_UNLOCK(java->attr_codebase);

    PA_LOCK(cp, char*, java->attr_archive);
    fprintf(stderr, "archive=%s ", cp ? cp : "NULL");
    PA_UNLOCK(java->attr_archive);

    PA_LOCK(cp, char*, java->attr_name);
    fprintf(stderr, "name=%s ", cp ? cp : "NULL");
    PA_UNLOCK(java->attr_name);

    fprintf(stderr, ">\n");

    for(i=0; i<java->param_cnt; i++)
    {
        if(java->param_names[i] && java->param_values[i])
        {
            fprintf(stderr, "<param name=%s value=%s>\n", java->param_names[i],
					java->param_values[i]);
        }
    }

    fprintf(stderr, "</applet>\n");
}
#endif

void
LJ_ReportJavaStartupError(MWContext* cx, int rv)
{
    char *s;
    switch (rv) {
      case LJ_INIT_NO_CLASSES:
		lj_init_failed = 1;
		s = PR_smprintf(
			"Unable to start a java applet: Can't find \"%s\" in your\n"
			"CLASSPATH. Read the release notes and install \"%s\"\n"
			"properly before restarting.\n\nCurrent value of CLASSPATH:\n%s\n",
			lj_zip_name, lj_zip_name, (CLASSPATH() ? CLASSPATH() : "<none>")); /* i18n */
		if (s) {
			FE_Alert(cx, s);
			free(s);
		}
        return;

      case LJ_INIT_VERSION_WRONG:
		lj_init_failed = 1;
		if (lj_bad_version == 0) {
			s = PR_smprintf(
				"Unable to start a java applet: the zip file is so old that it\n"
				"doesn't have a version number.\n");
		} else {
			s = PR_smprintf(
				"Unable to start a java applet: the version number for your\n"
				"classes is wrong. The version number found was %d. The correct\n"
				"version number is %d.\n",
				lj_bad_version,
				lj_correct_version);
		}
		if (s) {
			FE_Alert(cx, s);
			free(s);
		}
        return;

	  case LJ_INIT_FAILED_LAST_TIME:
	  case LJ_INIT_OK:
      default:
		return;
    }
}

void
LJ_CreateApplet(LO_JavaAppStruct *java, MWContext* cx)
{
    LJAppletData *ad;
    History_entry *history_element;
    int rv;

    if (lj_init_failed || !lj_java_enabled) return;

#ifdef xDEBUG
    if (PR_LOG_TEST(NSJAVA, warn)) {
		PR_LOG(NSJAVA, warn, ("LJ_CreateApplet called: "));
		DumpArgs(java);
    }
#endif

    /* Make sure that this applet has the required parameters */
    if (!java->attr_code) {
		/* Malformed tag. */
		/* XXX need to reflect error back to user */
		return;
    }

    /* Provide missing defaults */
    if (!java->width) java->width = 50;
    if (!java->height) java->height = 50;

    /* Start up the runtime; if that doesn't work we are hosed */
    rv = LJ_StartupJava();
	if (rv != LJ_INIT_OK) {
		LJ_ReportJavaStartupError(cx, rv);
		return;
	}

    /*
    ** Create the applet data object to hold the browser side applet
    ** state
    */
    ad = (LJAppletData*) calloc(1, sizeof(LJAppletData));
    if (!ad) {
        if (ad) free(ad);
        return;
    }

    ad->context = cx;
	ad->reloadMethod = NET_NORMAL_RELOAD;

    history_element = SHIST_GetCurrent(&cx->hist);
    if (history_element) {
		ad->docID = history_element->unique_id;
    } else {
		/*
		** XXX What to do? This can happen for instance when printing a
		** mail message that contains an applet.
		*/
		static int32 unique_number;
		ad->docID = --unique_number;
    }

    ad->mayscript = java->may_script;

    /*
    ** Record the parent context. On the java side of the world all of
    ** the applets for a particular parent context are kept aggregated in
    ** history order so that they can be destroyed when resources get
    ** tight
    */
    if (cx->is_grid_cell) {
		MWContext* ctxt = cx;
		while (ctxt->is_grid_cell)
			ctxt = ctxt->grid_parent;
		ad->parentContext = ctxt;
    } else {
		ad->parentContext = cx;
    }

    java->session_data = ad;
}

void 
LJ_Applet_getSize(LJAppletData* ad)
{
}

void
LJ_Applet_display(LJAppletData* ad, int iloc)
{
}

void
LJ_DeleteSessionData(MWContext* cx, void* sdata)
{
    LJAppletData* ad = (LJAppletData*)sdata;
    
    if (ad == NULL || lj_init_failed) return;
    if (cx != ad->context) return;

    FE_FreeJavaAppElement(ad->context, ad);
}

void
LJ_Applet_destroy(LJAppletData* ad)
{
    if (ad == NULL || lj_init_failed) return;

    PR_LOG(NSJAVA, warn, ("Destroy applet"));

    /* Don't check lj_java_enabled flag here because we need to
       be able to destroy an applet if we disable java and then
       leave a page that has an applet running */

    LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_destroyApplet_1,
					ad->docID, ad);
    ad->context = NULL;
    if (ad->documentURL) free(ad->documentURL);
    free(ad);
}

void
LJ_SetJavaEnabled(int onoff)
{
    if (!lj_init_failed && lj_java_enabled && !onoff) {
		/* Nuke the currently running applets */
		LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_destroyAll);
    }
    /*
    ** Don't actually turn it off until we've made it through the
    ** destroyAll, above.
    */
    lj_java_enabled = onoff;
}

Bool
LJ_GetJavaEnabled(void)
{
    if(lj_init_failed)
	   return FALSE;
	else
	  return lj_java_enabled;
}

/*******************************************************************************
 * Mozilla Events
 ******************************************************************************/

PR_LOG_DEFINE(Event);

typedef struct MozillaEvent_MochaOnLoad {
    MWContextEvent	ce;
    int             status;
} MozillaEvent_MochaOnLoad;

static void
lj_HandleEvent_MochaOnLoad(MozillaEvent_MochaOnLoad* e)
{
#ifdef MOCHA
  /* here we need only decrement the number of applets expected to load */
    int32 doc_id;
    lo_TopState *top_state;

    doc_id = XP_DOCID(e->ce.context);
    top_state = lo_FetchTopState(doc_id);
  
	/* only wait on applets if onload flag */
    if(top_state && top_state->mocha_has_onload
       && top_state->mocha_loading_applets_count) {
		top_state->mocha_loading_applets_count--;
		LM_SendLoadEvent(e->ce.context, LM_XFER_DONE);
    }
#endif /* MOCHA */
}

static void
lj_DestroyEvent_MochaOnLoad(MozillaEvent_MochaOnLoad* event)
{
    XP_FREE(event);
}

void
LJ_PostMochaOnLoad(MWContext* context,int status)
{
    MozillaEvent_MochaOnLoad* event = PR_NEW(MozillaEvent_MochaOnLoad);

	/* if unable to allocate event return, this could cause Navigator
	 * to hang on a page.
	 */
	if(event == NULL){
		/* make another attempt for the Mac */
		event = PR_NEW(MozillaEvent_MochaOnLoad);
		if(event == NULL)
			return;
	}

    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)lj_HandleEvent_MochaOnLoad,
				 (PRDestroyEventProc)lj_DestroyEvent_MochaOnLoad);
    event->ce.context = context;
	event->status     = status;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
}

/******************************************************************************/


typedef struct MozillaEvent_ShowStatus {
    MWContextEvent	ce;
    const char*		statusMessage;	/* string freed by receiver */
} MozillaEvent_ShowStatus;

void PR_CALLBACK
lj_HandleEvent_ShowStatus(MozillaEvent_ShowStatus* e)
{
    FE_Progress(e->ce.context, e->statusMessage);
}

void PR_CALLBACK
lj_DestroyEvent_ShowStatus(MozillaEvent_ShowStatus* event)
{
    XP_FREE((char*)event->statusMessage);
    XP_FREE(event);
}

void
LJ_PostShowStatus(MWContext* context, const char* message)
{
    MozillaEvent_ShowStatus* event = PR_NEW(MozillaEvent_ShowStatus);
	if (event == NULL) return;
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)lj_HandleEvent_ShowStatus,
				 (PRDestroyEventProc)lj_DestroyEvent_ShowStatus);
    event->ce.context = context;
    event->statusMessage = strdup(message);
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
}

/******************************************************************************/

typedef struct MozillaEvent_ShowDocument {
    MWContextEvent	ce;
    char*			url;	/* string freed by receiver */
    char*			addr;	/* string freed by receiver */
    char*			target;	/* string freed by receiver */
} MozillaEvent_ShowDocument;


void PR_CALLBACK
lj_HandleEvent_ShowDocument(MozillaEvent_ShowDocument* e)
{
    URL_Struct* url_s;
    MWContext* targetContext;
		
    url_s = NET_CreateURLStruct(e->url, NET_NORMAL_RELOAD);
    if (e->addr && url_s) {
        if (NET_SetURLIPAddressString(url_s, e->addr)) {
            /* out of memory */
            NET_FreeURLStruct(url_s);
            url_s = NULL;
        }
    }
    if (url_s == NULL)
		return;

    targetContext = XP_FindNamedContextInList(e->ce.context, e->target);
    if (targetContext == NULL) {
		targetContext = FE_MakeNewWindow(e->ce.context,
										 url_s, e->target, NULL);
		if (targetContext == NULL) {
			NET_FreeURLStruct(url_s);
			return;
		}
    }
	else
		FE_GetURL(targetContext, url_s);
}

void PR_CALLBACK
lj_DestroyEvent_ShowDocument(MozillaEvent_ShowDocument* event)
{
    XP_FREE((char*)event->url);
    XP_FREE((char*)event->addr);
    XP_FREE((char*)event->target);
    XP_FREE(event);
}

void
LJ_PostShowDocument(MWContext* context, char* url, char* addr, char* target)
{
    MozillaEvent_ShowDocument* event = PR_NEW(MozillaEvent_ShowDocument);
	if (event == NULL) return;
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)lj_HandleEvent_ShowDocument,
				 (PRDestroyEventProc)lj_DestroyEvent_ShowDocument);
    event->ce.context = context;
    event->url = strdup(url);
    event->addr = addr ? strdup(addr) : NULL;
    event->target = strdup(target);
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
}

/******************************************************************************/

void
LJ_ProcessEvent()
{
    PREvent* event;

    for (;;) {
        PR_LOG_BEGIN(Event, debug, ("$$$ getting event\n"));
    
        event = PR_GetEvent(mozilla_event_queue);
        if (event == NULL) {
            return;
        }
    
        PR_HandleEvent(event);
        PR_LOG_END(Event, debug, ("$$$ done with event\n"));
        PR_DestroyEvent(event);
    }
}

void PR_CALLBACK
lj_DiscardEventForContext(MWContextEvent* ce, MWContext* context,
			  PREventQueue* queue)
{
    if (ce->context == context) {
		PR_LOG(Event, debug,
			   ("$$$ \tdropping event %d for context %d\n",
				ce, SHIST_GetCurrent(&ce->context->hist)));
		PR_DequeueEvent(&ce->event, queue);
		PR_DestroyEvent(&ce->event);
    }
    else {
		PR_LOG(Event, debug,
			   ("$$$ \tskipping event %d for context %d\n",
				ce, SHIST_GetCurrent(&ce->context->hist)));
    }
}

void
LJ_DiscardEventsForContext(MWContext* context)
{
    PR_LOG_BEGIN(Event, debug, ("$$$ destroying context %d\n", 
								SHIST_GetCurrent(&context->hist)));

    /*
    ** First we enter the monitor so that no one else can post any events
    ** to the queue:
    */
    PR_EnterMonitor(mozilla_event_queue->monitor);
    PR_LOG(Event, debug,
		   ("$$$ destroying context %d, entered monitor\n",
			SHIST_GetCurrent(&context->hist)));

    /*
    ** Discard any pending events for this context:
    */
    PR_MapEvents(mozilla_event_queue,
				 (PR_EventFunPtr)lj_DiscardEventForContext,
				 context);

    nsn_JavaContext_breakLinkage(context);

    PR_ExitMonitor(mozilla_event_queue->monitor);

    PR_LOG_END(Event, debug,
			   ("$$$ destroying context %d, exited monitor\n", 
				SHIST_GetCurrent(&context->hist)));
}

/******************************************************************************/
