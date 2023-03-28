/* -*- Mode: C; tab-width: 4; -*- */
/*
 * @(#)javactxt.c	1.2 95/06/14 Warren Harris
 *
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 *
 */

#include "nsn.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prtime.h"
#include "prdump.h"

PR_LOG_DEFINE(JavaContext);

/******************************************************************************/
/*
** Here's what's going on here: We need a context to invoke NET_GetURL,
** but if we use the one available to us relative to the current thread
** (found via nsn_GetCurrentContext) we may deadlock or crash. This is
** because in some cases NET_GetURL does its work on the current thread
** rather than the mozilla thread causing the two to contend for the same
** dpy.
**
** To get around this, we make a dummy MWContext object (called a
** JavaContext) and set its fe callback functions to deliver events
** (defined in java.h) back to the mozilla thread to get the work
** done. We do this for things like GraphProgress, etc.
*/

typedef struct MozillaEvent_GraphProgressInit {
    MWContextEvent	ce;
    URL_Struct*		URL_s;
    int32			content_length;
} MozillaEvent_GraphProgressInit;

PR_STATIC_CALLBACK(void)
nsn_HandleEvent_GraphProgressInit(MozillaEvent_GraphProgressInit* e)
{
    e->ce.context->numberOfSimultaneousShowers++;
    PR_LOG(JavaContext, debug,
		   ("$$$ MOZILLAEVENT_GRAPHPROGRESSINIT: numberOfSimultaneousShowers = %d",
			e->ce.context->numberOfSimultaneousShowers));
    if (e->ce.context->numberOfSimultaneousShowers == 1) {
		e->ce.context->timeOfFirstMeteorShower = PR_Now();
    }
}

PRIVATE void
nsn_GraphProgressInit(MWContext  *context, 
		      URL_Struct *URL_s, 
		      int32       content_length)
{
    MozillaEvent_GraphProgressInit* event;
    MWContext* realContext;

    /* Don't look at the realContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    PR_ASSERT(context->type == MWContextJava);

    if (context->javaContextStreamData == NULL
		|| (realContext = context->javaContextStreamData->realContext) == NULL)
		goto done;
	
    event = PR_NEW(MozillaEvent_GraphProgressInit);
	if (event == NULL) return;
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)nsn_HandleEvent_GraphProgressInit,
				 (PRDestroyEventProc)free);
    event->ce.context = realContext;
    event->URL_s = /* URL_s */ NULL;
    event->content_length = content_length;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
    
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

typedef struct MozillaEvent_GraphProgressDestroy {
    MWContextEvent	ce;
    URL_Struct*		URL_s;
    int32			content_length;
    int32			total_bytes_read;
} MozillaEvent_GraphProgressDestroy;

PR_STATIC_CALLBACK(void)
nsn_HandleEvent_GraphProgressDestroy(MozillaEvent_GraphProgressDestroy* e)
{
    /*
    ** e->URL_s is ALWAYS NULL.  This is because the URL was created by a 
    ** different thread. So by the time the event is processed, the URL may have
    ** been destroyed.
    */
    e->ce.context->numberOfSimultaneousShowers--;
    PR_LOG(JavaContext, debug,
		   ("$$$ MOZILLAEVENT_GRAPHPROGRESSDESTROY: numberOfSimultaneousShowers = %d",
			e->ce.context->numberOfSimultaneousShowers));
    PR_ASSERT(e->ce.context->numberOfSimultaneousShowers >= 0);
    if (e->ce.context->numberOfSimultaneousShowers == 0 && e->ce.context->displayingMeteors) {
		e->ce.context->displayingMeteors = PR_FALSE;
		PR_LOG(JavaContext, debug, ("GraphProgressDestroy"));
		FE_GraphProgressDestroy(e->ce.context, /* e->URL_s */ NULL,
								e->content_length,
								e->total_bytes_read);
		/*
		** And it seems that if we don't call FE_EnableClicking about
		** now, those meteors never will stop tumbling.
		*/
		FE_EnableClicking(e->ce.context);
    }
}

PRIVATE void
nsn_GraphProgressDestroy(MWContext  *context, 
			 URL_Struct *URL_s, 
			 int32       content_length,
			 int32       total_bytes_read)
{
    MozillaEvent_GraphProgressDestroy* event;
    MWContext* realContext;

    /* Don't look at the realContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    PR_ASSERT(context->type == MWContextJava);

    if (context->javaContextStreamData == NULL
		|| (realContext = context->javaContextStreamData->realContext) == NULL)
		goto done;

    event = PR_NEW(MozillaEvent_GraphProgressDestroy);
	if (event == NULL) return;
    PR_InitEvent(&event->ce.event, 
				 (PRHandleEventProc)nsn_HandleEvent_GraphProgressDestroy,
				 (PRDestroyEventProc)free); 
    event->ce.context = realContext;
    event->URL_s = /* URL_s */ NULL;
    event->content_length = content_length;
    event->total_bytes_read = total_bytes_read;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

typedef struct MozillaEvent_GraphProgress {
    MWContextEvent	ce;
    URL_Struct*		URL_s;
    int32			bytes_received;
    int32       	bytes_since_last_time;
    int32       	content_length;
} MozillaEvent_GraphProgress;

PR_STATIC_CALLBACK(void)
nsn_HandleEvent_GraphProgress(MozillaEvent_GraphProgress* e)
{
    /*
    ** I think I figured this out. For some reason, lots of
    ** subsystems called from netlib (mail, news, ftp, etc.)  will
    ** call FE_SetProgressBarPercent -- but not http, and this is
    ** the main one we care about for java's netlib stubs. So we'll
    ** just fake it out by calculating our own percentage and
    ** setting that. Why FE_GraphProgress just doen't work this way
    ** to begin with will remain a complete and total mystery.
    **
    ** e->URL_s is ALWAYS NULL.  This is because the URL was created by a 
    ** different thread. So by the time the event is processed, the URL may have
    ** been destroyed.
    */
    int32 percent;
    if (e->content_length == 0) return;	/* it happens */
    percent = e->bytes_received * 100 / e->content_length;
    PR_LOG(JavaContext, debug, ("$$$ MOZILLAEVENT_GRAPHPROGRESS: percent = %d", percent));
    if (!e->ce.context->displayingMeteors) {
		/*
		** Heuristics... ohmygod. I'm only looking at the 10-70%
		** case because I'm trying to filter off initial transients
		** and the case where we're almost done anyway.
		*/
		if (percent > 10 && percent < 70) {
			int64 now = PR_Now();
			int64 delta, tmp, dpct, eta, waitTime;
				
			LL_SUB(delta, now, e->ce.context->timeOfFirstMeteorShower);
			LL_I2L(tmp, 100);
			LL_MUL(dpct, delta, tmp);	/* dpct = delta*100 */
			LL_I2L(tmp, percent);
			LL_DIV(eta, dpct, tmp);	/* eta = dpct/percent */
			LL_I2L(waitTime, 1000000L);	/* 1 sec */
#ifdef DEBUG_GRAPH
			{
				char buf[1024];
				size_t buflen = 1024;
				size_t count = 0;
				count += PR_snprintf(buf[count], buflen-count,
									 "GraphProgress: count=%d, %%=%d, delta=", 
									 e->ce.context->numberOfSimultaneousShowers, percent);
				count += PR_PrintMicrosecondsToBuf(buf[count], buflen-count, delta);
				count += PR_snprintf(buf[count], buflen-count, ", eta=");
				count += PR_PrintMicrosecondsToBuf(buf[count], buflen-count, eta);
				PR_LOG(JavaContext, debug, (buf));
			}
#endif /* DEBUG_GRAPH */
			if (LL_CMP(eta, >, waitTime)) {
				PR_LOG(JavaContext, debug, ("GraphProgressInit"));
				FE_GraphProgressInit(e->ce.context, /* e->URL_s */ NULL, e->content_length);
				e->ce.context->displayingMeteors = PR_TRUE;
			}
		}
    }
    else {
		FE_SetProgressBarPercent(e->ce.context, percent);
		FE_GraphProgress(e->ce.context, /* e->URL_s */ NULL, e->bytes_received,
						 e->bytes_since_last_time, e->content_length);
    }
}

PRIVATE void
nsn_GraphProgress(MWContext  *context, 
		  URL_Struct *URL_s, 
		  int32       bytes_received,
		  int32       bytes_since_last_time,
		  int32       content_length)
{
    MozillaEvent_GraphProgress* event;
    MWContext* realContext;

    /* Don't look at the realContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    PR_ASSERT(context->type == MWContextJava);

    if (context->javaContextStreamData == NULL
		|| (realContext = context->javaContextStreamData->realContext) == NULL)
		goto done;

    event = PR_NEW(MozillaEvent_GraphProgress);
	if (event == NULL) return;
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)nsn_HandleEvent_GraphProgress,
				 (PRDestroyEventProc)free); 
    event->ce.context = realContext;
    event->URL_s = /* URL_s */ NULL;
    event->bytes_received = bytes_received;
    event->bytes_since_last_time = bytes_since_last_time;
    event->content_length = content_length;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

PRIVATE void
nsn_Progress(MWContext *context, const char* msg)
{
    MWContext* realContext;

    /* Don't look at the realContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    PR_ASSERT(context->type == MWContextJava);

    if (context->javaContextStreamData == NULL
	|| (realContext = context->javaContextStreamData->realContext) == NULL)
	goto done;

    LJ_PostShowStatus(realContext, (char*)msg);
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

typedef struct MozillaEvent_SetProgressBarPercent {
    MWContextEvent	ce;
    int32			percent;
} MozillaEvent_SetProgressBarPercent;

PR_STATIC_CALLBACK(void)
nsn_HandleEvent_SetProgressBarPercent(MozillaEvent_SetProgressBarPercent* e)
{
    FE_SetProgressBarPercent(e->ce.context, e->percent);
}

PRIVATE void
nsn_SetProgressBarPercent(MWContext *context, int32 percent)
{
    MozillaEvent_SetProgressBarPercent* event;
    MWContext* realContext;

    /* Don't look at the realContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    PR_ASSERT(context->type == MWContextJava);

    if (context->javaContextStreamData == NULL
	|| (realContext = context->javaContextStreamData->realContext) == NULL)
	goto done;

    event = PR_NEW(MozillaEvent_SetProgressBarPercent);
	if (event == NULL) return;
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)nsn_HandleEvent_SetProgressBarPercent,
				 (PRDestroyEventProc)free); 
    event->ce.context = realContext;
    event->percent = percent;
    PR_PostEvent(mozilla_event_queue, &event->ce.event);
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

PR_STATIC_CALLBACK(void)
nsn_AllConnectionsComplete(MWContext* context)
{
    nsn_JavaContext_destroy(context);
    DECR(streamCount);
}

/*
** And the rest of them are no-ops. Define them with Michael's fancy
** mk_cx_fn.h header:
*/

PRIVATE int
nsn_noop(int x, ...)
{
    return 0;
}

#define MAKE_FE_TYPES_PREFIX(func)	func##_t
#define MAKE_FE_FUNCS_TYPES
#include "java.h"
#include "mk_cx_fn.h"
#undef MAKE_FE_FUNCS_TYPES

#define nsn_CreateNewDocWindow		(CreateNewDocWindow_t)nsn_noop
#define nsn_LayoutNewDocument		(LayoutNewDocument_t)nsn_noop
#define nsn_SetDocTitle			(SetDocTitle_t)nsn_noop
#define nsn_FinishedLayout		(FinishedLayout_t)nsn_noop
#define nsn_TranslateISOText		(TranslateISOText_t)nsn_noop
#define nsn_GetTextInfo			(GetTextInfo_t)nsn_noop
#define nsn_GetImageInfo		(GetImageInfo_t)nsn_noop
#define nsn_GetEmbedSize		(GetEmbedSize_t)nsn_noop
#define nsn_GetJavaAppSize		(GetJavaAppSize_t)nsn_noop
#define nsn_GetFormElementInfo		(GetFormElementInfo_t)nsn_noop
#define nsn_GetFormElementValue		(GetFormElementValue_t)nsn_noop
#define nsn_ResetFormElement		(ResetFormElement_t)nsn_noop
#define nsn_SetFormElementToggle	(SetFormElementToggle_t)nsn_noop
#define nsn_FreeFormElement		(FreeFormElement_t)nsn_noop
#define nsn_FreeImageElement		(FreeImageElement_t)nsn_noop
#define nsn_FreeEmbedElement		(FreeEmbedElement_t)nsn_noop
#define nsn_FreeJavaAppElement		(FreeJavaAppElement_t)nsn_noop
#define nsn_HideJavaAppElement		(HideJavaAppElement_t)nsn_noop
#define nsn_FreeEdgeElement		(FreeEdgeElement_t)nsn_noop
#define nsn_FormTextIsSubmit		(FormTextIsSubmit_t)nsn_noop
#define nsn_DisplaySubtext		(DisplaySubtext_t)nsn_noop
#define nsn_DisplayText			(DisplayText_t)nsn_noop
#define nsn_DisplayImage		(DisplayImage_t)nsn_noop
#define nsn_DisplayEmbed		(DisplayEmbed_t)nsn_noop
#define nsn_DisplayJavaApp		(DisplayJavaApp_t)nsn_noop
#define nsn_DisplaySubImage		(DisplaySubImage_t)nsn_noop
#define nsn_DisplayEdge			(DisplayEdge_t)nsn_noop
#define nsn_DisplayTable		(DisplayTable_t)nsn_noop
#define nsn_DisplayCell			(DisplayCell_t)nsn_noop
#define nsn_DisplaySubDoc		(DisplaySubDoc_t)nsn_noop
#define nsn_DisplayLineFeed		(DisplayLineFeed_t)nsn_noop
#define nsn_DisplayHR			(DisplayHR_t)nsn_noop
#define nsn_DisplayBullet		(DisplayBullet_t)nsn_noop
#define nsn_DisplayFormElement		(DisplayFormElement_t)nsn_noop
#define nsn_ClearView			(ClearView_t)nsn_noop
#define nsn_SetDocDimension		(SetDocDimension_t)nsn_noop
#define nsn_SetDocPosition		(SetDocPosition_t)nsn_noop
#define nsn_GetDocPosition		(GetDocPosition_t)nsn_noop
#define nsn_BeginPreSection		(BeginPreSection_t)nsn_noop
#define nsn_EndPreSection		(EndPreSection_t)nsn_noop
#if 0
#define nsn_SetProgressBarPercent	(SetProgressBarPercent_t)nsn_noop
#endif
#define nsn_SetBackgroundColor		(SetBackgroundColor_t)nsn_noop
#if 0	/* above */
#define nsn_Progress			(Progress_t)nsn_noop
#endif
#define nsn_Alert			(Alert_t)nsn_noop
#define nsn_SetCallNetlibAllTheTime	(SetCallNetlibAllTheTime_t)nsn_noop
#define nsn_ClearCallNetlibAllTheTime	(ClearCallNetlibAllTheTime_t)nsn_noop
#if 0	/* above */
#define nsn_GraphProgressInit		(GraphProgressInit_t)nsn_noop
#define nsn_GraphProgressDestroy	(GraphProgressDestroy_t)nsn_noop
#define nsn_GraphProgress		(GraphProgress_t)nsn_noop
#endif
#define nsn_UseFancyFTP			(UseFancyFTP_t)nsn_noop
#define nsn_UseFancyNewsgroupListing	(UseFancyNewsgroupListing_t)nsn_noop
#define nsn_FileSortMethod		(FileSortMethod_t)nsn_noop
#define nsn_ShowAllNewsArticles		(ShowAllNewsArticles_t)nsn_noop
#define nsn_Confirm			(Confirm_t)nsn_noop
#define nsn_Prompt			(Prompt_t)nsn_noop
#define nsn_PromptUsernameAndPassword	(PromptUsernameAndPassword_t)nsn_noop
#define nsn_PromptPassword		(PromptPassword_t)nsn_noop
#define nsn_EnableClicking		(EnableClicking_t)nsn_noop
#define nsn_ImageSize			(ImageSize_t)nsn_noop
#define nsn_ImageData			(ImageData_t)nsn_noop
#define nsn_ImageIcon			(ImageIcon_t)nsn_noop
#define nsn_ImageOnScreen		(ImageOnScreen_t)nsn_noop
#define nsn_SetColormap			(SetColormap_t)nsn_noop
#if 0	/* above */
#define nsn_AllConnectionsComplete	(AllConnectionsComplete_t)nsn_noop
#endif

/* Just reuse the same set of context functions: */
ContextFuncs nsn_JavaContextFuncs;

#define JCX_LOCK() PR_CEnterMonitor(&nsn_JavaContextFuncs)
#define JCX_UNLOCK() PR_CExitMonitor(&nsn_JavaContextFuncs)

/******************************************************************************/

MWContext*
nsn_JavaContext_make(URL_Struct* url, nsn_JavaStreamData* javaData)
{
    MWContext* context;
    static PRBool funcsInitialized = PR_FALSE;

    if (!funcsInitialized) {
	/* Initialize our statically allocated context funcs table: */
	#define MAKE_FE_FUNCS_PREFIX(f) nsn_##f
	#define MAKE_FE_FUNCS_ASSIGN nsn_JavaContextFuncs.
	#include "mk_cx_fn.h"

	funcsInitialized = PR_TRUE;
    }

    context = PR_NEW(MWContext);
    if (!context) return NULL;
    XP_MEMSET(context, 0, sizeof(MWContext));
    context->type = MWContextJava;
    context->prInfo = NULL;
    context->prSetup = NULL;
    context->funcs = &nsn_JavaContextFuncs;

    context->convertPixX = 1;	/* XXX needed? */
    context->convertPixY = 1;	/* XXX needed? */

    context->is_grid_cell = FALSE;

    context->javaContextStreamData = javaData;

    PR_INIT_CLIST(&context->javaContexts);
    /*
    ** Link us into the javaContexts so that if the realContext goes
    ** away, we can cope (see structs.h and lj_embed.c). The realContext
    ** might be null if we were not in an AppletThreadGroup
    ** (e.g. AWT-Motif).
    */
    if (javaData->realContext != NULL) {
	JCX_LOCK();
	PR_APPEND_LINK(&context->javaContexts, &javaData->realContext->javaContexts);
	JCX_UNLOCK();
    }

    INCR(contextCount);
    return context;
}

void
nsn_JavaContext_destroy(MWContext* context)
{
    
    PR_EnterMonitor(mozilla_event_queue->monitor);
    JCX_LOCK();
    PR_ASSERT(context->type == MWContextJava);
    PR_REMOVE_AND_INIT_LINK(&context->javaContexts);
    if( context->javaContextStreamData ) {
        context->javaContextStreamData->javaContext = NULL;
    }
    JCX_UNLOCK();
    NSN_FREE(context);
    DECR(contextCount);
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/*
** Break the links between the real window contexts and
** dummy contexts created for java's network connections. This
** prevents subsequent Progress events from happening on this context
** that's going away.
*/
void nsn_JavaContext_breakLinkage(MWContext *realContext)
{
    JCX_LOCK();
	PR_ASSERT(realContext->type != MWContextJava);
    if (realContext->javaContexts.next == NULL) {
	/*
	** Defensive: in case we get here before the history code has
	** initialized the linkage. This can happen on certain kinds
	** of contexts (e.g. a ShowDocument window).
	*/
    } else {
	while (!PR_CLIST_IS_EMPTY(&realContext->javaContexts)) {
	    MWContext* javaContext = 
		MWCONTEXT_PTR(PR_LIST_HEAD(&realContext->javaContexts));
	    PR_REMOVE_AND_INIT_LINK(&javaContext->javaContexts);
	    JCX_UNLOCK();
	    nsn_CloseJavaConnection(javaContext);
	    JCX_LOCK();
	}
    }
    JCX_UNLOCK();
}

/******************************************************************************/
