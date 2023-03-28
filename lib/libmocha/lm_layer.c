/*
 * Compositor layer reflection and event notification
 *
 * Scott Furman, 6/20/96
 *
 * $Id: lm_layer.c,v 1.1 1996/06/22 22:47:35 fur Exp $
 */

#ifdef LAYERS

#include "lm.h"
#include "xp.h"
#include "layers.h"

enum layer_array_slot {
    LAYER_ARRAY_LENGTH = -1
};

/* Native part of mocha object reflecting children layers of another layer */
typedef struct MochaLayerArray {
    MochaDecoder        *decoder;
    uint                 length; /* # of entries in array */
    CL_Layer            *parent_layer;
} MochaLayerArray;

static MochaBoolean
layer_array_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
			 MochaDatum *dp)
{
    MochaLayerArray *array;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;
    CL_Layer *layer, *parent_layer;

    array = obj->data;
    decoder = array->decoder;
    context = decoder->window_context;
    if (!context || !array) return MOCHA_TRUE; /* Paranoia */
    parent_layer = array->parent_layer;

    switch (slot) {
      case LAYER_ARRAY_LENGTH:
	count = CL_GetLayerChildCount(parent_layer);
	if (count > array->length)
	    array->length = count;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, fval, count);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}
	if ((uint)slot >= array->length)
	    array->length = slot + 1;
	layer = CL_GetLayerChildByIndex(parent_layer, slot);
	if (layer) {
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, obj,
			     LM_ReflectLayer(context, layer, parent_layer));
	}
	break;
    }
    return MOCHA_TRUE;
}

static void
layer_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaLayerArray *array = obj->data;

    if (array) {
	LM_UNLINK_OBJECT(array->decoder, obj);
	MOCHA_free(mc, array);
    }
}

static MochaObjectOps layer_array_ops = {
    layer_array_get_property, layer_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, layer_array_finalize
};

static MochaPropertySpec layer_array_props[] = {
    {lm_length_str,     LAYER_ARRAY_LENGTH,     MDF_TAINTED | MDF_READONLY},
    {0}
};


/* Native part of a mocha object that reflects a single compositor layer */
typedef struct MochaLayer {
    MochaDecoder           *decoder;
    CL_Layer               *layer;
    MochaAtom              *name;
    MochaObject            *child_layers_array_obj;
} MochaLayer;

static MochaClass layer_array_class = { "LayerArray", &layer_array_ops };

static MochaObject *
reflect_layer_array(MochaDecoder *decoder,
                    CL_Layer *parent_layer)
{
    MochaContext *mc;
    MochaLayer *mocha_layer_parent;
    MochaLayerArray *array;
    MochaObject *obj;
    MochaClass *clazz;

    obj = CL_GetLayerMochaObject(parent_layer);
    if (! obj)                  /* paranoia */
        return NULL;
    mocha_layer_parent = obj->data;
    obj = mocha_layer_parent->child_layers_array_obj;

    if (obj)                    /* Are layer children already reflected ? */
        return obj;
    
    clazz = &layer_array_class;
    
    mc = decoder->mocha_context;
    array = MOCHA_malloc(mc, sizeof *array);
    if (!array)
	return 0;
    XP_BZERO(array, sizeof *array);
    array->decoder = decoder;
/* FIXME use MOCHA_InitClass instead of this! */
    if (!clazz->atom)
        clazz->atom = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, clazz->name));
    
    obj = MOCHA_NewObject(mc, clazz, array, 0, 0, layer_array_props);
    if (!obj) {
	MOCHA_free(mc, array);
	return NULL;
    }
    array->parent_layer = parent_layer;
    LM_LINK_OBJECT(decoder, obj, "layers");
    return obj;
}

/* Given the mocha object reflecting a compositor layer, return
   the mocha object that reflects its child layers in an array. */
MochaObject *
lm_GetLayerArray(MochaDecoder *decoder,
                 MochaObject  *parent_mocha_layer_obj)
{
    MochaLayer *mocha_layer_parent;
    MochaObject *obj;
    MochaContext *mc;
    
    if (! parent_mocha_layer_obj)
        return NULL;
    mocha_layer_parent = parent_mocha_layer_obj->data;
    obj = mocha_layer_parent->child_layers_array_obj;
    if (obj)
        return obj;

    obj = reflect_layer_array(decoder, mocha_layer_parent->layer);
    mc = decoder->mocha_context;
    if (obj && MOCHA_DefineObject(mc, parent_mocha_layer_obj, "layers",
                                  obj, MDF_READONLY))
        mocha_layer_parent->child_layers_array_obj = obj;
    return obj;
}


/* The top-level document object contains the distinguished _DOCUMENT
   layer.  Reflect the array containing children of the _DOCUMENT layer. */
MochaObject *
lm_GetDocumentLayerArray(MochaDecoder *decoder)
{
    MochaLayerArray *array;
    MWContext *context;
    MochaObject *obj;

    obj = decoder->layers;
    if (obj)
	return obj;
    context = decoder->window_context;
    obj = reflect_layer_array(decoder,
                              CL_FindLayer(context->compositor, "_DOCUMENT"));
    decoder->layers = MOCHA_HoldObject(decoder->mocha_context, obj);
    return obj;
}

/* Mocha native method:

   Translate layer to given XY coordinates, e.g.
   document.layers[0].moveto(1, 3); */
static MochaBoolean
move_layer(MochaContext *mc, MochaObject *obj,
           uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaFloat x, y;
    MochaLayer *mocha_layer;
    
    mocha_layer = obj->data;
    if (argc != 2) {
        MOCHA_ReportError(mc, "incorrect number of arguments");
        MOCHA_ReportError(mc, "incorrect number of arguments");
        return MOCHA_FALSE;
    }
    if (!MOCHA_DatumToNumber(mc, argv[0], &x) ||
        !MOCHA_DatumToNumber(mc, argv[1], &y)) {
        return MOCHA_FALSE;
    }
    
    CL_MoveLayer(mocha_layer->layer, x, y);
    return MOCHA_TRUE;
}

/* Mocha native method:

   Translate layer by given XY offset, e.g.
   document.layers[0].offset(1, 3); */
static MochaBoolean
offset_layer(MochaContext *mc, MochaObject *obj,
           uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaFloat x, y;
    MochaLayer *mocha_layer;
    
    mocha_layer = obj->data;
    if (argc != 2) {
        MOCHA_ReportError(mc, "incorrect number of arguments");
        return MOCHA_FALSE;
    }
    if (!MOCHA_DatumToNumber(mc, argv[0], &x) ||
        !MOCHA_DatumToNumber(mc, argv[1], &y)) {
        return MOCHA_FALSE;
    }
    
    CL_OffsetLayer(mocha_layer->layer, x, y);
    return MOCHA_TRUE;
}

/* Mocha native method:

   Resize layer to given width and height, e.g.
   document.layers[0].resize(100, 200); */
static MochaBoolean
resize_layer(MochaContext *mc, MochaObject *obj,
             uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaFloat width, height;
    MochaLayer *mocha_layer;
    
    mocha_layer = obj->data;
    if (argc != 2) {
        MOCHA_ReportError(mc, "incorrect number of arguments");
        return MOCHA_FALSE;
    }
    if (!MOCHA_DatumToNumber(mc, argv[0], &width) ||
        !MOCHA_DatumToNumber(mc, argv[1], &height)) {
        return MOCHA_FALSE;
    }
    
    CL_ResizeLayer(mocha_layer->layer, width, height);
    return MOCHA_TRUE;
}

/* Static compositor layer property slots */
enum layer_slot {
    LAYER_NAME              = -2,
    LAYER_X                 = -3,
    LAYER_Y                 = -4,
    LAYER_HEIGHT            = -5,
    LAYER_WIDTH             = -6,
    LAYER_HIDDEN            = -7,
    LAYER_ABOVE             = -8,
    LAYER_BELOW             = -9,
    LAYER_GROUPS            = -10,
    LAYER_CHILDREN          = -11
};

/* Static compositor layer properties */
static MochaPropertySpec layer_props[] = {
    {"name",                LAYER_NAME,         MDF_TAINTED | MDF_READONLY},
    {"x",                   LAYER_X,            MDF_TAINTED},
    {"y",                   LAYER_Y,            MDF_TAINTED},
    {"height",              LAYER_HEIGHT,       MDF_TAINTED},
    {"width",               LAYER_WIDTH,        MDF_TAINTED},
    {"hidden",              LAYER_HIDDEN,       MDF_TAINTED},
    {"layers",              LAYER_CHILDREN,     MDF_TAINTED | MDF_READONLY},
    {"above",               LAYER_ABOVE,        MDF_TAINTED | MDF_READONLY}, /* FIXME - should be writeable */
    {"below",               LAYER_BELOW,        MDF_TAINTED | MDF_READONLY}, /* FIXME - should be writeable */
    {"groups",              LAYER_GROUPS,       MDF_TAINTED | MDF_READONLY}, /* FIXME - should be writeable */
    {0}
};

static MochaBoolean
layer_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		   MochaDatum *dp)
{
    MochaLayer *mocha_layer;
    CL_Layer *layer, *layer_above, *layer_below;
    enum layer_slot layer_slot;
    MochaAtom *atom;

    mocha_layer = obj->data;
    layer = mocha_layer->layer;
    layer_slot = slot;

    switch (layer_slot) {
    case LAYER_NAME:
	if (mocha_layer->name)
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, atom, mocha_layer->name);
	else
	    *dp = MOCHA_null;
        break;

    case LAYER_HIDDEN:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, bval, CL_GetLayerHidden(layer));
        break;

    case LAYER_X:
    case LAYER_Y:
    case LAYER_HEIGHT:
    case LAYER_WIDTH:
    {
        XP_Rect bbox;
        CL_GetLayerBbox(layer, &bbox);
            
        switch (layer_slot) {
        case LAYER_X:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, fval, bbox.left);
        break;
        
        case LAYER_Y:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, fval, bbox.top);
        break;

        case LAYER_HEIGHT:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, fval,
                             bbox.bottom - bbox.top + 1);
            break;

        case LAYER_WIDTH:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, fval,
                             bbox.right - bbox.left + 1);
            break;
        }
        break;
    }
     
    case LAYER_CHILDREN: 
        MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, obj,
                         lm_GetLayerArray(mocha_layer->decoder, obj));
        break;
        
    case LAYER_ABOVE:
        layer_above = CL_GetLayerAbove(layer);
        if (layer_above)
            MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, obj,
                             CL_GetLayerMochaObject(layer_above));
        break;

    case LAYER_BELOW:
        layer_below = CL_GetLayerBelow(layer);
        if (layer_below)
            MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, obj,
                             CL_GetLayerMochaObject(layer_below));
        break;

    case LAYER_GROUPS:

    default:
        /* Don't mess with a user-defined or method property. */
        return MOCHA_TRUE;
    }

    return MOCHA_TRUE;
}

static MochaBoolean
layer_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		   MochaDatum *dp)
{
    MochaBoolean hidden;
    MochaLayer *mocha_layer;
    MochaDecoder *decoder;
    MWContext *context;
    CL_Layer *layer;
    enum layer_slot layer_slot;
    const char *str;

    mocha_layer = obj->data;
    decoder = mocha_layer->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    layer = mocha_layer->layer;
    layer_slot = slot;
    switch (layer_slot) {
    case LAYER_HIDDEN:
        if (!MOCHA_DatumToBoolean(mc, *dp, &hidden))
            return MOCHA_FALSE;
        CL_SetLayerHidden(layer, hidden);
        break;
        
    case LAYER_X:
    case LAYER_Y:
    case LAYER_HEIGHT:
    case LAYER_WIDTH:
    {
        MochaFloat val;
        XP_Rect bbox;
        uint32 width, height;
        
        if (!MOCHA_DatumToNumber(mc, *dp, &val))
            return MOCHA_FALSE;
        CL_GetLayerBbox(layer, &bbox);
        width = bbox.right - bbox.left + 1;
        height = bbox.bottom - bbox.top + 1;

        switch (layer_slot) {
        case LAYER_X:
            CL_MoveLayer(layer, val, bbox.top);
            break;
            
        case LAYER_Y:
            CL_MoveLayer(layer, bbox.left, val);
            break;

        case LAYER_HEIGHT: 
            CL_ResizeLayer(mocha_layer->layer, width, val);
            break;

        case LAYER_WIDTH:
            CL_ResizeLayer(mocha_layer->layer, val, height);
            break;
        }
        break;
    }
     
    case LAYER_ABOVE:
    case LAYER_BELOW:
        /* FIXME - these should be writeable */
        break;

    /* These are immutable. */
    case LAYER_GROUPS:
    case LAYER_NAME:
    case LAYER_CHILDREN:

    default:
	break;
    }

    return layer_get_property(mc, obj, slot, dp);
}

static void
layer_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaLayer *mocha_layer = obj->data;
    CL_Layer *layer = mocha_layer->layer;

    if (!mocha_layer) return;
    CL_SetLayerMochaObject(layer, NULL);
    LM_UNLINK_OBJECT(mocha_layer->decoder, obj);

    MOCHA_DropAtom(mc, mocha_layer->name);
    MOCHA_free(mc, mocha_layer);
}

static MochaObjectOps layer_ops = {
    layer_get_property, layer_set_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, layer_finalize
};

static MochaClass layer_class = { "Layer", &layer_ops };

static MochaFunctionSpec layer_methods[] = {
    {"offset",	offset_layer,	2},
    {"moveto",	move_layer,	2},
    {"resize",	resize_layer,	2},
    {0}
};

MochaObject *
LM_ReflectLayer(MWContext *context, CL_Layer *layer, CL_Layer *parent_layer)
{
    MochaObject *obj, *array_obj;
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaLayerArray *array;
    MochaLayer *mocha_layer;
    char *name = CL_GetLayerName(layer);

    obj = CL_GetLayerMochaObject(layer);
    if (obj)                    /* Already reflected ? */
	return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)               /* Paranoia */
	return 0;
    mc = decoder->mocha_context;

    /* Layer is accessible by name in child array of parent layer.
       For example a layer named "Fred" could be accessed as
       document.layers["Fred"] if it is a child of the distinguished
       _DOCUMENT layer. */
    if (parent_layer) {
        array_obj =
            lm_GetLayerArray(decoder, CL_GetLayerMochaObject(parent_layer));
        if (!array_obj) {
            LM_PutMochaDecoder(decoder);
            return 0;
        }
    }
    
    mocha_layer = MOCHA_malloc(mc, sizeof *mocha_layer);
    if (!mocha_layer) {
	LM_PutMochaDecoder(decoder);
	return NULL;
    }
    XP_BZERO(mocha_layer, sizeof *mocha_layer);

    obj = MOCHA_NewObject(mc, &layer_class, mocha_layer,
			  decoder->layer_prototype, array_obj, 0);
    if (!obj) {
	MOCHA_free(mc, mocha_layer);
	LM_PutMochaDecoder(decoder);
	return NULL;
    }
    if (name && parent_layer && 
        !MOCHA_DefineObject(mc, array_obj, name, obj, MDF_READONLY)) {
	MOCHA_DestroyObject(mc, obj);
	LM_PutMochaDecoder(decoder);
	return NULL;
    }

    MOCHA_DefineFunctions(mc, obj, layer_methods);

    mocha_layer->decoder = decoder;
    mocha_layer->layer = layer;
    mocha_layer->name = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, name));

    CL_SetLayerMochaObject(layer, obj);

    LM_LINK_OBJECT(decoder, obj, name ? name : "aLayer");
    LM_PutMochaDecoder(decoder);
    return obj;
}

MochaBoolean
lm_InitLayerClass(MochaDecoder *decoder)
{
    MochaContext *mc;
    MochaObject *prototype;

    mc = decoder->mocha_context;
    prototype = MOCHA_InitClass(mc, decoder->window_object, &layer_class, 0,
                                NULL, 0, layer_props, 0);
    if (!prototype)
	return MOCHA_FALSE;
    decoder->layer_prototype = MOCHA_HoldObject(mc, prototype);
    return MOCHA_TRUE;
}

#endif /* LAYERS */
