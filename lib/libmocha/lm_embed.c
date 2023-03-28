/* -*- Mode: C; tab-width: 8; tabs-indent-mode: t -*- */

/*
** Mocha reflection of embeds in the current document.  The
** reflected object is the java object associated with the
** embed... if there is none then the reflection is MOCHA_null.
*/

/* Please leave outside of ifdef for windows precompiled headers */
#include "lm.h"

#ifdef JAVA

#include "xp.h"
#include "layout.h"
#include "np.h"
#include "nppriv.h"
#include "jri.h"
#include "mo_java.h"
#include "mocha.h"
#include "prlog.h"

enum embed_array_slot {
    EMBED_ARRAY_LENGTH = -1
};

static MochaPropertySpec embed_array_props[] = {
    {lm_length_str,     EMBED_ARRAY_LENGTH,     MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaEmbedArray {
    MochaDecoder        *decoder;
    uint		length;
} MochaEmbedArray;

static MochaBoolean
embed_array_get_property(MochaContext *mc, MochaObject *obj,
                         MochaSlot slot, MochaDatum *dp)
{
    MochaEmbedArray *array = obj->data;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;
    LO_EmbedStruct *embed_data;

    decoder = array->decoder;
    context = decoder->window_context;

    if (!context) return MOCHA_TRUE;

    switch (slot) {
      case EMBED_ARRAY_LENGTH:
	count = LO_EnumerateEmbeds(context);
	if (count > array->length)
	    array->length = count;
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, array->length);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}
	embed_data = LO_GetEmbedByIndex(context, (uint)slot);
	if (embed_data) {
            MochaObject *mo = LM_ReflectEmbed(context, embed_data,
                                              (uint)slot);
            MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, mo);
            XP_ASSERT((uint)slot < array->length);
	} else {
            MOCHA_ReportError(mc, "no embed with index %d\n");
            return MOCHA_FALSE;
        }
	break;
    }
    return MOCHA_TRUE;
}

static void
embed_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaEmbedArray *array = obj->data;

    LM_UNLINK_OBJECT(array->decoder, obj);
    MOCHA_free(mc, array);
}

static MochaClass embed_array_class = {
    "EmbedArray",
    embed_array_get_property, embed_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, embed_array_finalize
};

MochaObject *
lm_GetEmbedArray(MochaDecoder *decoder)
{
    MochaContext *mc = decoder->mocha_context;
    MochaObject *obj;
    MochaEmbedArray *array;

    obj = decoder->embeds;
    if (obj)
	return obj;

    array = MOCHA_malloc(mc, sizeof *array);
    if (!array)
        return 0;
    array->decoder = decoder;
    array->length = 0;

    obj = MOCHA_NewObject(mc, &embed_array_class, array, 0, 0, 
			  embed_array_props, 0);
    LM_LINK_OBJECT(decoder, obj, "embeds");
    decoder->embeds = MOCHA_HoldObject(mc, obj);
    return obj;
}

/* this calls MozillaEmbedContext to reflect the embed by
 * calling into mocha... yow! */
static MochaObject *
lm_ReallyReflectEmbed(MWContext *context, LO_EmbedStruct *lo_embed)
{
    MochaObject *obj;
    MochaDecoder *decoder;
    MochaContext *mc;
    jref jembed;
    NPEmbeddedApp* embed;
    PR_LOG(Moja, debug, ("really reflect embed 0x%x\n", lo_embed));

    if ((obj = lo_embed->mocha_object))
        return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    mc = decoder->mocha_context;

    /* set the element to something bad if we can't get the java obj */
    if (!MOCHA_JavaGlueEnabled()) {
        PR_LOG(Moja, debug, ("reflected embed 0x%x as null\n",
                             lo_embed));
        return lo_embed->mocha_object = lm_DummyObject;
    }

    embed = (NPEmbeddedApp*) lo_embed->FE_Data;
    if (embed) {
        np_data *ndata = (np_data*) embed->np_data;
        np_instance *instance;

        /* XXX this comes from npglue.c, it should be put
         * in one of the plugin header files */
        extern jref npn_getJavaPeer(NPP npp);

        if (!ndata || ndata->state == NPDataSaved) {
            PR_LOG(Moja, warn, ("embed 0x%x is missing or suspended\n",
                                lo_embed));
            return 0;
        }
        instance = ndata->instance;
	if (!instance) return NULL;
        jembed = npn_getJavaPeer(instance->npp);

        obj = MOCHA_ReflectJObjectToMObject(decoder->mocha_context,
                                            (HObject *)jembed);
        PR_LOG(Moja, debug, ("reflected embed 0x%x (java 0x%x) to 0x%x ok\n",
                             lo_embed, jembed, obj));

        return lo_embed->mocha_object = obj;
    } else {
        PR_LOG(Moja, warn, ("failed to reflect embed 0x%x\n", lo_embed));
        return 0;
    }
}


MochaObject *
LM_ReflectEmbed(MWContext *context, LO_EmbedStruct *lo_embed,
                 uint index)
{
    MochaObject *obj, *array_obj, *outer_obj;
    MochaDecoder *decoder;
    MochaEmbedArray *array;
    MochaDatum datum;
    MochaContext *mc;
    char *name;
    int i;

    obj = lo_embed->mocha_object;
    if (obj)
	return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    mc = decoder->mocha_context;

    /* get the name */
    name = 0;
    for (i = 0; i < lo_embed->attribute_cnt; i++) {
        if (!XP_STRCASECMP(lo_embed->attribute_list[i], "name")) {
            name = strdup(lo_embed->value_list[i]);
            break;
        }
    }

    array_obj = lm_GetEmbedArray(decoder);
    if (!array_obj) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    array = (MochaEmbedArray *) array_obj->data;

    /* XXX should pass thru ReallyReflectApplet to whatever calls NewObject */
    outer_obj = decoder->active_form ? decoder->active_form : decoder->document;

    /* this function does the real work */
    obj = lm_ReallyReflectEmbed(context, lo_embed);

    /* put it in the embed array */
    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, MDF_ENUMERATE|MDF_READONLY,
			  MOCHA_TAINT_IDENTITY, u.obj, obj);
    MOCHA_SetProperty(mc, decoder->embeds, name, index, datum);
    if (index >= array->length)
	array->length = index + 1;

    /* put it in the document scope */
    if (name && !MOCHA_DefineObject(mc, outer_obj, name, obj,
				    MDF_ENUMERATE | MDF_READONLY)) {
	PR_LOG(Moja, warn, ("failed to define embed 0x%x as %s\n",
			    lo_embed, name));
	/* XXX remove it altogether? */
    }

    /* cache it in layout data structure */
    lo_embed->mocha_object = obj;

    LM_PutMochaDecoder(decoder);
    return obj;
}

MochaObject *
LM_ReflectNamedEmbed(MWContext *context, lo_NameList *name_rec, uint index)
{
    return 0;
}

#endif /* JAVA */
