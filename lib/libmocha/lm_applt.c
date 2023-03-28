/* -*- Mode: C; tab-width: 8; tabs-indent-mode: t -*- */

/*
** Mocha reflection of applets in the current document
*/

/* Please leave outside of ifdef for windows precompiled headers */
#include "lm.h"

#ifdef JAVA

#include "xp.h"
#include "layout.h"
#include "java.h"
#include "lj.h"		/* for LJ_InvokeMethod */
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#ifdef XP_MAC
#include "n_applet_MozillaAppletContext.h"
#else
#include "netscape_applet_MozillaAppletContext.h"
#endif
#include "mo_java.h"
#include "mocha.h"
#include "prlog.h"

enum applet_array_slot {
    APPLET_ARRAY_LENGTH = -1
};

static MochaPropertySpec applet_array_props[] = {
    {lm_length_str,     APPLET_ARRAY_LENGTH,    MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaAppletArray {
    MochaDecoder        *decoder;
    uint		length;
} MochaAppletArray;

static MochaBoolean
applet_array_get_property(MochaContext *mc, MochaObject *obj,
			  MochaSlot slot, MochaDatum *dp)
{
    MochaAppletArray *array = obj->data;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;
    LO_JavaAppStruct *applet_data;

    decoder = array->decoder;
    context = decoder->window_context;

    if (!context) return MOCHA_TRUE;

    switch (slot) {
      case APPLET_ARRAY_LENGTH:
	count = LO_EnumerateApplets(context);
	if (count > array->length)
	    array->length = count;
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, array->length);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}
	applet_data = LO_GetAppletByIndex(context, (uint)slot);
	if (applet_data) {
            MochaObject *mo = LM_ReflectApplet(context, applet_data,
                                               (uint)slot);
            MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, mo);

            if ((uint)slot >= array->length)
                array->length = slot + 1;
	} else {
            MOCHA_ReportError(mc, "no applet with index %d\n");
            return MOCHA_FALSE;
        }
	break;
    }
    return MOCHA_TRUE;
}

static void
applet_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaAppletArray *array = obj->data;

    LM_UNLINK_OBJECT(array->decoder, obj);
    MOCHA_free(mc, array);
}

static MochaClass applet_array_class = {
    "AppletArray",
    applet_array_get_property, applet_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, applet_array_finalize
};

MochaObject *
lm_GetAppletArray(MochaDecoder *decoder)
{
    MochaContext *mc = decoder->mocha_context;
    MochaObject *obj;
    MochaAppletArray *array;

    obj = decoder->applets;
    if (obj)
	return obj;

    array = MOCHA_malloc(mc, sizeof *array);
    if (!array)
        return 0;
    array->decoder = decoder;
    array->length = 0;

    obj = MOCHA_NewObject(mc, &applet_array_class, array, 0, 0,
			  applet_array_props, 0);
    LM_LINK_OBJECT(decoder, obj, "applets");
    decoder->applets = MOCHA_HoldObject(mc, obj);
    return obj;
}

/* this calls MozillaAppletContext to reflect the applet by
 * calling into mocha... yow! */
static MochaObject *
lm_ReallyReflectApplet(MWContext *context, LO_JavaAppStruct *lo_applet)
{
    MochaObject *obj;
    MochaDecoder *decoder;
    MochaContext *mc;
    LJAppletData *ad;
    jref japplet;

    PR_LOG(Moja, debug, ("really reflect applet 0x%x\n", lo_applet));

    if ((obj = lo_applet->mocha_object))
        return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    mc = decoder->mocha_context;

    /* set the element to something bad if we can't get the java obj */
    if (!MOCHA_JavaGlueEnabled()) {
        PR_LOG(Moja, debug, ("reflected applet 0x%x as null\n",
                             lo_applet));
        return lo_applet->mocha_object = lm_DummyObject;
    }

    ad = (LJAppletData *) lo_applet->session_data;

    if (ad) {
        /* XXX probably need some work here to get the MochaContext
         * which is used in the reflection callback, and store it
         * in lj_mozilla_ee for the duration */
        /* XXX actually, this should be done in LJ_InvokeMethod */
        /*
          MochaContext *saved_context = lj_mozilla_ee->mocha_context;
          lj_mozilla_ee->mocha_context = decoder->mocha_context
         */

        /* MozillaAppletContext.reflectApplet adds it to the array from java */
        japplet = LJ_InvokeMethod(
	    methodID_netscape_applet_MozillaAppletContext_reflectApplet_1,
	    ad->docID, ad);

        obj = MOCHA_ReflectJObjectToMObject(decoder->mocha_context,
                                            (HObject *)japplet);
        /*
          lj_mozilla_ee->mocha_context = saved_context;
         */

        PR_LOG(Moja, debug, ("reflected applet 0x%x (java 0x%x) to 0x%x ok\n",
                             lo_applet, japplet, obj));

        return lo_applet->mocha_object = obj;
    } else {
        PR_LOG(Moja, warn, ("failed to reflect applet 0x%x\n", lo_applet));
        return 0;
    }
}


/* XXX what happens if this is called before java starts up?
 * or if java is disabled? */

MochaObject *
LM_ReflectApplet(MWContext *context, LO_JavaAppStruct *applet_data,
                 uint index)
{
    MochaObject *obj, *array_obj, *outer_obj;
    MochaDecoder *decoder;
    MochaAppletArray *array;
    MochaDatum datum;
    MochaContext *mc;
    char *name;

    obj = applet_data->mocha_object;
    if (obj)
	return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    mc = decoder->mocha_context;

    /* get the name */
    if (applet_data->attr_name) {
        char *tmp;
        PA_LOCK(tmp, char *, applet_data->attr_name);
        name = MOCHA_strdup(mc, tmp);
        PA_UNLOCK(applet_data->attr_name);
    } else {
        name = 0;
    }

    array_obj = lm_GetAppletArray(decoder);
    if (!array_obj) {
        obj = 0;
        goto out;
    }
    array = (MochaAppletArray *) array_obj->data;

    /* XXX should pass thru ReallyReflectApplet to whatever calls NewObject */
    outer_obj = decoder->active_form ? decoder->active_form : decoder->document;

    /* this function does the real work */
    obj = lm_ReallyReflectApplet(context, applet_data);

    if (!obj) {
        /* reflection failed.  this usually means that javascript tried
         * to talk to the applet before it was ready.  there's not much
         * we can do about it */

        MOCHA_ReportError(mc, "Can't reflect applet \"%s\": not loaded yet", 
                          name);
        obj = 0;
        goto out;
    }

    /* put it in the applet array */
    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, MDF_ENUMERATE|MDF_READONLY,
			  MOCHA_TAINT_IDENTITY, u.obj, obj);
    MOCHA_SetProperty(mc, decoder->applets, name, index, datum);
    if (index >= array->length)
	array->length = index + 1;

    /* put it in the document scope */
    if (name) {
        if (!MOCHA_DefineObject(mc, outer_obj, name, obj,
				MDF_ENUMERATE | MDF_READONLY)) {
            PR_LOG(Moja, warn, ("failed to define applet 0x%x as %s\n",
                                applet_data, name));
	    /* XXX remove it altogether? */
        }
	MOCHA_free(mc, name);
    }

    /* cache it in layout data structure XXX lm_ReallyReflectApplet did this */
    XP_ASSERT(applet_data->mocha_object == obj);

  out:
    LM_PutMochaDecoder(decoder);
    return obj;
}

MochaObject *
LM_ReflectNamedApplet(MWContext *context, lo_NameList *name_rec, uint index)
{
    return 0;
}

#endif /* JAVA */
