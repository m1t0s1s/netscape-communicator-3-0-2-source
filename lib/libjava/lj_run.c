/* From mozilla */
#include "lj.h"
#include "layout.h"
#include "structs.h"

/* From nspr */
#include "prmem.h"
#include "prlog.h"
#include "prthread.h"
#include "prmon.h"

#ifdef MOCHA
/* for Mocha glue */
#include "mo_java.h"
#include "libmocha.h"
#endif

#define IMPLEMENT_netscape_applet_MozillaAppletContext
#define IMPLEMENT_java_applet_Applet
#define IMPLEMENT_netscape_applet_EmbeddedAppletFrame

#include "java_applet_Applet.h"
#ifndef XP_MAC
#include "netscape_applet_EmbeddedAppletFrame.h"
#include "netscape_applet_MozillaAppletContext.h"
#else
#include "n_applet_EmbeddedAppletFrame.h"
#include "n_applet_MozillaAppletContext.h"
#endif


/*** XXX these two functions should be moved to jsStubs.c? */
jref
LJ_GetMochaWindow(MWContext* cx)
{
#ifdef MOCHA
    jref window;
    MochaDecoder *decoder; 

    if (!cx || cx->type != MWContextBrowser)
        return 0;	/* XXX exception? */

    decoder = LM_GetMochaDecoder(cx);

    /* if there is a decoder now, reflect the window */
    if (decoder && MOCHA_JavaGlueEnabled()) {
        /* XXX naughty way to get a jref */
        window = (jref)
          MOCHA_ReflectMObjectToJObject(decoder->mocha_context,
                                        decoder->window_object);
    }
    return window;
#else
    return 0;
#endif
}


/* THIS MUST RUN ON THE MOZILLA THREAD */
/*
** XXX This really needs to happen for the current applet rather than
** passing the applet.
*/
jref
LJ_GetAppletMochaWindow(JRIEnv *env, jref applet)
{
#ifdef MOCHA
    jref frame;
    LJAppletData *ad;
    MWContext *cx;

    /* XXX this is a hack so that use_blah only gets run in
     * the mozilla env */
    env = mozenv;

    use_java_applet_Applet(env);
    use_netscape_applet_EmbeddedAppletFrame(env);

    frame = get_java_applet_Applet_stub(env, applet);
    if (!frame) {
        PR_LOG(Moja, error,
               ("couldn't find applet stub for applet 0x%x", applet));
        goto except;
    }

    ad = (LJAppletData *)
      get_netscape_applet_EmbeddedAppletFrame_pData(env, frame);
    if (!ad) {
        PR_LOG(Moja, error,
               ("couldn't find LJAppletData for AppletFrame 0x%x", frame));
        goto except;
    }

    if (!ad->mayscript)
        goto except;

    cx = ad->context;

    return LJ_GetMochaWindow(cx);
#endif
  except:
    return 0;	/* XXX exception? */
}

Bool
LJ_MayScript(JRIEnv *env, jref frame)
{
#ifdef MOCHA
    LJAppletData *ad;

    ad = (LJAppletData *)
      get_netscape_applet_EmbeddedAppletFrame_pData(env, frame);
    if (!ad)
        return FALSE;

    return ad->mayscript;
#else
    return FALSE;
#endif
}

void
LJ_Applet_init(LO_JavaAppStruct *java)
{
    LJAppletData *ad = (LJAppletData *)java->session_data;
    char *cp;
    char buf[50];
    int i, argc;
    jref hs;

    if (lj_init_failed || !lj_java_enabled) return;

    /*
    ** Make a copy of the document's url
    */
    PA_LOCK(cp, char*, java->base_url);
    ad->documentURL = strdup(cp);
    PA_UNLOCK(java->base_url);

    /*
    ** Allocate an array of strings for each argument we will pass it.
    **
    ** XXX Count the real arguments (defensive code in case layout
    ** bothces things)
    */
    argc = 7;
    if (java->attr_codebase) argc++;
    if (java->attr_archive) argc++;
    if (java->attr_name) argc++;
    if (java->may_script) argc++;
    for (i = 0; i < java->param_cnt; i++) {
	if (java->param_names[i]) {
	    argc++;
	}
    }

    hs = LJ_MakeArrayOfString(argc);

    if (hs == 0) {
	return;
    }

    /*
    ** Make java strings out of each of the arguments. argv[0] will be
    ** the URL of the document the applet came from.
    */
    argc = 0;
    if (!LJ_SetStringArraySlot(hs, argc++, ad->documentURL, 0, 0 )) {
	return;
    }

    /* applet attribute */
    PA_LOCK(cp, char*, java->attr_code);
    if (!LJ_SetStringArraySlot(hs, argc++, "code", cp, 0)) {
	return;
    }
    PA_UNLOCK(java->attr_code); 

    /* codebase attribute (optional) */
    if (java->attr_codebase) {
	PA_LOCK(cp, char*, java->attr_codebase);
	if (!LJ_SetStringArraySlot(hs, argc++, "codebase", cp, 0)) {
	    return;
	}
	PA_UNLOCK(java->attr_codebase);
    }

    /* archive attribute (optional) */
    if (java->attr_archive) {
	PA_LOCK(cp, char*, java->attr_archive);
	if (!LJ_SetStringArraySlot(hs, argc++, "archive", cp, 0)) {
	    return;
	}
	PA_UNLOCK(java->attr_archive);
    }

    /* name attribute (optional) */
    if (java->attr_name) {
	PA_LOCK(cp, char*, java->attr_name);
	if (!LJ_SetStringArraySlot(hs, argc++, "name", cp, 0)) {
	    return;
	}
	PA_UNLOCK(java->attr_name);
    }

    /* width attribute */
    sprintf(buf, "%d", java->width);
    if (!LJ_SetStringArraySlot(hs, argc++, "width", buf, 0)) {
	return;
    }

    /* height attribute */
    sprintf(buf, "%d", java->height);
    if (!LJ_SetStringArraySlot(hs, argc++, "height", buf, 0)) {
	return;
    }

    /* alignment attribute */
    switch (java->alignment) {
      case LO_ALIGN_CENTER:		cp = "abscenter"; break;
      case LO_ALIGN_LEFT:		cp = "left"; break;
      case LO_ALIGN_RIGHT:		cp = "right"; break;
      case LO_ALIGN_TOP:		cp = "texttop"; break;
      case LO_ALIGN_BOTTOM:		cp = "absbottom"; break;
      case LO_ALIGN_NCSA_CENTER:	cp = "center"; break;
      case LO_ALIGN_NCSA_BOTTOM:	cp = "bottom"; break;
      case LO_ALIGN_NCSA_TOP:		cp = "top"; break;
      default:				cp = "baseline"; break;
    }
    if (!LJ_SetStringArraySlot(hs, argc++, "align", cp , 0)) {
	return;
    }

    /* vspace attribute */
    sprintf(buf, "%d", java->border_vert_space);
    if (!LJ_SetStringArraySlot(hs, argc++, "vspace", buf, 0)) {
	return;
    }

    /* hspace attribute */
    sprintf(buf, "%d", java->border_horiz_space);
    if (!LJ_SetStringArraySlot(hs, argc++, "hspace", buf, 0)) {
	return;
    }

    /* mayscript attribute */
    if (java->may_script) {
        if (!LJ_SetStringArraySlot(hs, argc++, "mayscript", "true", 0)) {
            return;
        }
    }

    argc = 7;
    if (java->attr_codebase) argc++;
    if (java->attr_archive) argc++;
    if (java->attr_name) argc++;
    if (java->may_script) argc++;
    for (i = 0; i < java->param_cnt; i++) {
	if (!java->param_names[i]) {
	    continue;
	}
	if (!LJ_SetStringArraySlot(hs, argc, java->param_names[i],
			   java->param_values[i], ad->context->win_csid)) {
	    return;
	}
	argc++;
    }

    LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_initApplet_1,
		    ad->parentContext, ad->context, ad->docID, ad, hs);
}

void
LJ_Applet_start(LJAppletData* ad)
{
    if (ad == NULL || lj_init_failed || !lj_java_enabled) 
	return;
    LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_startApplet_1,
		    ad->docID, ad, ad->context);
}

void
LJ_Applet_stop(LJAppletData* ad)
{
    if (ad == NULL || lj_init_failed || !lj_java_enabled) 
	return;
    LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_stopApplet_1,
		    ad->docID, ad);
}

void
lj_MapContextTree(MWContext* context, JRIMethodID operation)
{
    MWContext *child;
    int i = 1;
    History_entry* currentPage = context->hist.cur_doc_ptr;
    if (currentPage != NULL) 
	LJ_InvokeMethod(operation, currentPage->unique_id);

    while ((child = (MWContext*)XP_ListGetObjectNum(context->grid_children,
						    i++))) {
	lj_MapContextTree(child, operation);
    }
}

void
LJ_IconifyApplets(MWContext* context)
{
    if (lj_init_failed || !lj_java_enabled)
	return;

    lj_MapContextTree(context, 
	methodID_netscape_applet_MozillaAppletContext_iconifyApplets_1);
}

void
LJ_UniconifyApplets(MWContext* context)
{
    if (lj_init_failed || !lj_java_enabled)
	return;

    lj_MapContextTree(context, 
	methodID_netscape_applet_MozillaAppletContext_uniconifyApplets_1);
}

void
LJ_DestroyAppletContext(MWContext* context)
{
    if (lj_init_failed || !lj_java_enabled)
	return;

    PR_LOG(NSJAVA, warn, ("Destroy applets for context %x", context));

    lj_MapContextTree(context, 
	methodID_netscape_applet_MozillaAppletContext_destroyApplets_1);
}

/******************************************************************************/

int
LJ_CurrentDocID(MWContext* context)
{
    while (context->is_grid_cell)
	context = context->grid_parent;
    fprintf(stderr, "CurrentDocID = %d\t%s\n",
	    context->hist.cur_doc_ptr->address,
	    context->hist.cur_doc_ptr->title);
    return (int)context->hist.cur_doc_ptr->address;	/* need something that changes with the current doc */
}
