/* -*- Mode: C; tab-width: 4; -*- */

#ifndef fe_java_h___
#define fe_java_h___

#include "lo_ele.h"
#ifdef XP_MAC
#include "prevent.h"
#else
#include "nspr/prevent.h"
#endif
#include "jri.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type of data that hangs off of LO_JavaStruct->FE_Data */
typedef struct LJAppletDataStr LJAppletData;

typedef enum LJAppletState {
    LJAppletState_Unborn,
    LJAppletState_Initialized,
    LJAppletState_Running
} LJAppletState;

/*
** Java applet data (one per applet).
**
** Instances of this structure are pointed to by the
** LO_JavaAppStruct->FE_Data
*/
struct LJAppletDataStr {
    /* The name of the document URL that referenced the applet */
    char *documentURL;
	NET_ReloadMethod reloadMethod;

    MWContext* context;
    MWContext* parentContext;
    int32 docID;
    Bool mayscript;

    /* front-end specific data */
    void *fe_data;

    /* This stuff should probably be hung off fe_data, but oh well... */
    Bool running;	/* if running, this avoids recursive redisplays */
    void* window;	/* the dummy parent window used by awt */
    void* awtWindow;	/* the window created by awt that will get reparented */
};

/* type of a function used by a callback event */
typedef void (*LJCallback)(void*);

#define DOWNLOADABLE_ZIPFILE_PREFIX		"jzip"

/******************************************************************************/

extern int LJ_StartupJava(void);

extern void LJ_ShutdownJava(void);

/* Error codes returned by LJ_StartupJava: */
#define LJ_INIT_OK					 0
#define LJ_INIT_NO_CLASSES			-1
#define LJ_INIT_VERSION_WRONG		-2
#define LJ_INIT_FAILED_LAST_TIME	-3

/*
** This function can be used to report an error (put up a dialog) if java
** could not be initialized. The rv parameter is one of the error codes,
** returned by LJ_StartupJava.
*/
extern void LJ_ReportJavaStartupError(MWContext* cx, int rv);

/*
** Adds a directory to the classpath. This function will also add any zip
** files it finds in that directory to the classpath.
*/
extern void LJ_AddToClassPath(char* dirPath);

/*
** Make the java console window visible.
*/
extern void LJ_ShowConsole(void);

/*
** Hide the java console window.
*/
extern void LJ_HideConsole(void);

/*
** Predicate to see if the console window is currently visible.
*/
extern int LJ_IsConsoleShowing(void);

/*
** Set the callback function that tells whomever cares what
** the current state of the console's visibility is.
*/
extern void LJ_SetConsoleShowCallback(void (*func)(int on, void* a),
									  void *arg);

/************************************************************************/

/*
** Create a java applet data structure, filling out the session_data
** field of the LO_JavaAppStruct.
*/
extern void
LJ_CreateApplet(LO_JavaAppStruct *java, MWContext* cx);

/*
** This sends the applet the init message.
*/
extern void LJ_Applet_init(LO_JavaAppStruct *java_struct);

/*
** The FE calls this after LJ_Applet_init to ask the applet what size it
** would like to be.
*/
extern void LJ_Applet_getSize(LJAppletData* ad);

/*
** This sends the applet start message.
*/
extern void LJ_Applet_start(LJAppletData* ad);

/*
** This sends the applet stop message.
*/
extern void LJ_Applet_stop(LJAppletData* ad);

/*
** Called by the FE when the embedded java applet should be re-displayed.
** This is called during damage repair.
*/
extern void LJ_Applet_display(LJAppletData* ad, int iloc);

/*
** Called by the FE when the embedded java applet should be
** destroyed. This sends the applet the destroy message.
*/
extern void LJ_Applet_destroy(LJAppletData* ad);

/*
** Iconifies all the applets on a page.
*/
extern void LJ_IconifyApplets(MWContext* context);

/*
** Uniconifies all the applets on a page.
*/
extern void LJ_UniconifyApplets(MWContext* context);

/*
** Set the java enabled state bit. If "onoff" is non-zero then
** java is enabled, otherwise it's not.
*/
extern void LJ_SetJavaEnabled(int onoff);

/*
** Get the java enabled state as a Bool TRUE or FALSE.
*/
extern Bool LJ_GetJavaEnabled(void);

/*
** Returns the Mocha window for a context as a reflected JSObject.
** Returns NULL if Mocha isn't enabled.
*/
extern jref LJ_GetMochaWindow(MWContext *cx);

/*
** Returns the Mocha window for an applet as a reflected JSObject.
** Returns NULL if Mocha isn't enabled or if the MAYSCRIPT attribute
** isn't present in the applet tag.
*/
extern jref LJ_GetAppletMochaWindow(JRIEnv *env, jref applet);

/*
** Returns TRUE if the MAYSCRIPT attribute was present for the
** applet in the given EmbeddedAppletFrame.
*/
extern Bool LJ_MayScript(JRIEnv *env, jref frame);

#ifdef XP_UNIX

/*
** Tell libjava where the program that is using java was run from.  This
** way it can add that directory (and maybe some of the related ones) to
** the CLASSPATH.
*/
extern void LJ_SetProgramName(const char *name);

#endif /* XP_UNIX */

/* 
 * Called by layout: 
 */
extern void LJ_DeleteSessionData(MWContext* cx, void* sdata);

/*******************************************************************************
 * Mozilla Events
 ******************************************************************************/

#define NEW_FE_CONTEXT_FUNCS
#include "fe_proto.h"

/*
** Process events sent to the Mozilla thread.
*/
extern void
LJ_ProcessEvent(void);

/*
** Discard all events in the mozilla_event_queue for a particular
** context.
*/
extern void
LJ_DiscardEventsForContext(MWContext* context);

/*
** All events sent to the Mozilla thread must subclass this (include this
** structure as its first field).
*/

extern PREventQueue* mozilla_event_queue;

typedef struct MWContextEvent {
    PREvent			event;
    MWContext*		context;
} MWContextEvent;

/******************************************************************************/
/* Event posting routines for netscape/applet */

/*
** Posts a MochaOnLoad event to the mozilla_event_queue.
*/
extern void
LJ_PostMochaOnLoad(MWContext* context,int status);

/*
** Posts a SHOWSTATUS event to the mozilla_event_queue.
*/
extern void
LJ_PostShowStatus(MWContext* context, const char* message);

/*
** Posts a SHOWDOCUMENT event to the mozilla_event_queue.
*/
extern void
LJ_PostShowDocument(MWContext* context, char* url, char* addr, char* target);

/*
** Posts a SETCONSOLESTATE event to the mozilla_event_queue
*/
extern void LJ_PostSetConsoleState(int state);

/******************************************************************************/
/* Stuff for netscape/net */

/*
** This function is used to register converters for java.
*/
extern void NSN_RegisterJavaConverter(void);

/*
** This private function is used to close down a java network
** connection. The java context must be one created by netStubs.
*/
extern void nsn_CloseJavaConnection(MWContext* javaContext);

/*
** This function is used to obtain the MWContext for the window in which
** a java context is to be displayed.
*/
extern MWContext *nsn_JavaContextToRealContext(MWContext *javaContext);

/*
** This private function is used to destroy the relationship
** between a real context and the pseudo contexts created for
** network connections.
*/
extern void nsn_JavaContext_breakLinkage(MWContext *realContext);

/******************************************************************************/
/* Applet Trimming */

/*
** Allows the applet trimThreshold to be set. This count is the number of
** applets that are allowed to be around at any given time. Applets which
** are in a stopped state (in the history) will be trimmed if this
** threshold is exceeded. The trimThreshold can be exceeded if the
** visible pages have more than that number of applets. Also sets whether
** messages should be printed to the console during trimming.
*/
extern void LJ_SetAppletTrimParams(jint threshold, jbool noisy);

/*
** Returns the total of number of applets which have been created and not
** yet destroyed. This includes running and stopped applets.
*/
extern jint LJ_GetTotalApplets(void);

/*
** Forces applets to be trimmed. The number of applets actually trimmed
** is returned.
*/
extern jint LJ_TrimApplets(jint count, jbool trimOnlyStoppedApplets);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* fe_java_h___ */
