/*
** Mocha reflection of the HREF and NAME Anchors in the current document.
**
** Brendan Eich, 9/8/95
*/
#include "lm.h"
#include "xp.h"
#include "layout.h"	/* XXX for lo_NameList only */

enum anchor_array_slot {
    ANCHOR_ARRAY_LENGTH = -1
};

static MochaPropertySpec anchor_array_props[] = {
    {lm_length_str,     ANCHOR_ARRAY_LENGTH,    MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaAnchorArray {
    MochaDecoder        *decoder;
    uint                length;
} MochaAnchorArray;

static MochaBoolean
link_array_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
			MochaDatum *dp)
{
    MochaAnchorArray *array = obj->data;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;
    LO_AnchorData *anchor_data;

    decoder = array->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    switch (slot) {
      case ANCHOR_ARRAY_LENGTH:
	count = LO_EnumerateLinks(context);
	if (count > array->length)
	    array->length = count;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, count);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}
	if ((uint)slot >= array->length)
	    array->length = slot + 1;
	anchor_data = LO_GetLinkByIndex(context, (uint)slot);
	if (anchor_data) {
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
			     LM_ReflectLink(context, anchor_data, (uint)slot));
	}
	break;
    }
    return MOCHA_TRUE;
}

static void
anchor_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaAnchorArray *array = obj->data;

    if (!array) return;
    LM_UNLINK_OBJECT(array->decoder, obj);
    MOCHA_free(mc, array);
}

static MochaClass link_array_class = {
    "LinkArray",
    link_array_get_property, link_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, anchor_array_finalize
};

static MochaBoolean
LinkArray(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
anchor_array_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
			  MochaDatum *dp)
{
    MochaAnchorArray *array = obj->data;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;

    decoder = array->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;
    switch (slot) {
      case ANCHOR_ARRAY_LENGTH:
	count = LO_EnumerateNamedAnchors(context);
	if (count > array->length)
	    array->length = count;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, count);
	break;

      default:
#ifdef NOTYET
	{
	    lo_NameList *name_rec;

	    name_rec = LO_GetNamedAnchorByIndex(context, (uint)slot);
	    if (name_rec) {
		MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
				 LM_ReflectNamedAnchor(context, name_rec,
						       (uint)slot));
	    }
	}
#else
	if ((uint)slot >= array->length)
	    array->length = slot + 1;
#endif
	break;
    }
    return MOCHA_TRUE;
}

static MochaClass anchor_array_class = {
    "AnchorArray",
    anchor_array_get_property, anchor_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, anchor_array_finalize
};

static MochaBoolean
AnchorArray(MochaContext *mc, MochaObject *obj,
	    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaObject *
reflect_anchor_array(MochaDecoder *decoder, MochaClass *clazz,
		     MochaNativeCall constructor)
{
    MochaContext *mc;
    MochaObject *obj, *prototype;
    MochaAnchorArray *array;

    mc = decoder->mocha_context;
    prototype = MOCHA_InitClass(mc, decoder->window_object, clazz, 0,
				constructor, 0, anchor_array_props, 0, 0, 0);
    if (!prototype)
	return 0;
    array = MOCHA_malloc(mc, sizeof *array);
    if (!array)
	return 0;
    obj = MOCHA_NewObject(mc, clazz, array, prototype, decoder->document, 0, 0);
    if (!obj) {
	MOCHA_free(mc, array);
	return 0;
    }
    XP_BZERO(array, sizeof *array);
    array->decoder = decoder;
    LM_LINK_OBJECT(decoder, obj, clazz->name);
    return obj;
}

MochaObject *
lm_GetLinkArray(MochaDecoder *decoder)
{
    MochaObject *obj;

    obj = decoder->links;
    if (obj)
	return obj;
    obj = reflect_anchor_array(decoder, &link_array_class, LinkArray);
    decoder->links = MOCHA_HoldObject(decoder->mocha_context, obj);
    return obj;
}

MochaObject *
lm_GetNameArray(MochaDecoder *decoder)
{
    MochaObject *obj;

    obj = decoder->anchors;
    if (obj)
	return obj;
    obj = reflect_anchor_array(decoder, &anchor_array_class, AnchorArray);
    decoder->anchors = MOCHA_HoldObject(decoder->mocha_context, obj);
    return obj;
}

MochaObject *
LM_ReflectLink(MWContext *context, LO_AnchorData *anchor_data, uint index)
{
    MochaObject *obj, *array_obj;
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaAnchorArray *array;
    char *anchor, *target;
    MochaURL *url;
    MochaDatum datum;

    obj = anchor_data->mocha_object;
    if (obj)
	return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    mc = decoder->mocha_context;

    array_obj = lm_GetLinkArray(decoder);
    if (!array_obj) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    array = array_obj->data;

    PA_LOCK(anchor, char *, anchor_data->anchor);
    PA_LOCK(target, char *, anchor_data->target);
    url = lm_NewURL(mc, decoder, anchor, target);
    PA_UNLOCK(anchor_data->anchor);
    PA_UNLOCK(anchor_data->target);
    if (!url) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    url->index = index;

    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, MDF_ENUMERATE|MDF_READONLY,
			  MOCHA_TAINT_IDENTITY, u.obj, url->url_object);
    MOCHA_SetProperty(mc, decoder->links, 0, index, datum);
    if (index >= array->length)
	array->length = index + 1;

    anchor_data->mocha_object = url->url_object;
    LM_PutMochaDecoder(decoder);
    return url->url_object;
}

MochaObject *
LM_ReflectNamedAnchor(MWContext *context, lo_NameList *name_rec, uint index)
{
    return 0;
}
