/*
** Image reflection and event notification
**
** Scott Furman, 3/30/96
**
*
* $Id: lm_img.c,v 1.24 1996/08/15 01:17:25 brendan Exp $
*/
#include "lm.h"
#include "xp.h"
#include "lo_ele.h"
#include "prmacros.h"

enum image_array_slot {
    IMAGE_ARRAY_LENGTH = -1
};

static MochaPropertySpec image_array_props[] = {
    {lm_length_str,     IMAGE_ARRAY_LENGTH,     MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaImageArray {
    MochaDecoder        *decoder;
    uint                length;
} MochaImageArray;

static MochaBoolean
image_array_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
			 MochaDatum *dp)
{
    MochaImageArray *array;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;
    LO_ImageStruct *image;

    array = obj->data;
    decoder = array->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    switch (slot) {
      case IMAGE_ARRAY_LENGTH:
	count = LO_EnumerateImages(context);
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
	image = LO_GetImageByIndex(context, (uint)slot);
	if (image) {
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
			     LM_ReflectImage(context, image, NULL, (uint)slot));
	}
	break;
    }
    return MOCHA_TRUE;
}

static void
image_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaImageArray *array = obj->data;

    LM_UNLINK_OBJECT(array->decoder, obj);
    MOCHA_free(mc, array);
}

static MochaClass image_array_class = {
    "ImageArray",
    image_array_get_property, image_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, image_array_finalize
};

enum image_slot {
    IMAGE_NAME              = -2,
    IMAGE_SRC               = -3,
    IMAGE_LOWSRC            = -4,
    IMAGE_HEIGHT            = -5,
    IMAGE_WIDTH             = -6,
    IMAGE_BORDER            = -7,
    IMAGE_VSPACE            = -8,
    IMAGE_HSPACE            = -9,
    IMAGE_COMPLETE          = -10
};

static MochaPropertySpec image_props[] = {
    {"name",                IMAGE_NAME,         MDF_ENUMERATE | MDF_READONLY},
    {"src",                 IMAGE_SRC,          MDF_ENUMERATE},
    {"lowsrc",              IMAGE_LOWSRC,       MDF_ENUMERATE},
    {"height",              IMAGE_HEIGHT,       MDF_ENUMERATE | MDF_READONLY},
    {"width",               IMAGE_WIDTH,        MDF_ENUMERATE | MDF_READONLY},
    {"border",              IMAGE_BORDER,       MDF_ENUMERATE | MDF_READONLY},
    {"vspace",              IMAGE_VSPACE,       MDF_ENUMERATE | MDF_READONLY},
    {"hspace",              IMAGE_HSPACE,       MDF_ENUMERATE | MDF_READONLY},
    {"complete",            IMAGE_COMPLETE,     MDF_ENUMERATE | MDF_READONLY},
    {0}
};

/*
** Base image element type.
*/
typedef struct MochaImage {
    MochaDecoder           *decoder;
    LO_ImageStruct         *image_data;     /* 0 unless made by new Image() */
    uint8                   pending_events;
    uint                    index;
    MochaBoolean            complete;       /* Finished loading or aborted */
    MochaAtom              *name;
} MochaImage;

#define GET_IMAGE_DATA(context, image)                                        \
    ((image)->image_data ? (image)->image_data                                \
     : LO_GetImageByIndex((context), (image)->index))

static MochaBoolean
image_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		   MochaDatum *dp)
{
    MochaImage *image;
    LO_ImageStruct *image_data;
    enum image_slot image_slot;
    MochaAtom *atom;

    image = obj->data;
    image_data = GET_IMAGE_DATA(image->decoder->window_context, image);
    if (!image_data)
	return MOCHA_TRUE;
    image_slot = slot;

    switch (image_slot) {
    case IMAGE_NAME:
	if (image->name)
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, image->name);
	else
	    *dp = MOCHA_null;
        break;

    case IMAGE_SRC:
	atom = MOCHA_Atomize(mc, (char*)image_data->image_url);
	if (!atom) return MOCHA_FALSE;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
	break;

    case IMAGE_LOWSRC:
	atom = MOCHA_Atomize(mc, (char*)image_data->lowres_image_url);
	if (!atom) return MOCHA_FALSE;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
	break;

    case IMAGE_HEIGHT:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, image_data->height);
        break;

    case IMAGE_WIDTH:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, image_data->width);
        break;

    case IMAGE_BORDER:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, image_data->border_width);
        break;

    case IMAGE_HSPACE:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval,
			 image_data->border_horiz_space);
        break;

    case IMAGE_VSPACE:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval,
			 image_data->border_vert_space);
        break;

    case IMAGE_COMPLETE:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, u.bval, image->complete);
        break;

    default:
        /* Don't mess with a user-defined or method property. */
        return MOCHA_TRUE;
    }

    return MOCHA_TRUE;
}

static MochaBoolean
image_set_src(MochaImage *image, const char *str, LO_ImageStruct *image_data)
{
    MochaDecoder *decoder;
    MWContext *context;
    PA_Block *image_urlp;

    decoder = image->decoder;
    context = decoder->window_context;
    image_urlp = &image_data->image_url;

    if (!lm_SaveParamString(decoder->mocha_context, image_urlp, str)) {
        XP_FREE((void *)str);
        return MOCHA_FALSE;
    }
    if (!context) return MOCHA_TRUE;

    if (image_data->il_image)
        FE_FreeImageElement(context, image_data);
    image->complete = MOCHA_FALSE;
    image_data->image_attr->attrmask |= LO_ATTR_MOCHA_IMAGE;
    IL_GetImage(str, context, image_data, NET_DONT_RELOAD);

    /* FE_DisplayImage() is how you tell the FE that the image
       coordinates are valid and that it can now draw the image.
       (There's no need to block on layout.  The image must have
       already been layed out if we can see it from JavaScript.) */

    if (! image->image_data)
        FE_DisplayImage(context, 0, image_data);
    return MOCHA_TRUE;
}

static MochaBoolean
image_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		   MochaDatum *dp)
{
    MochaBoolean ok;
    MochaImage *image;
    MochaDecoder *decoder;
    MWContext *context;
    LO_ImageStruct *image_data;
    enum image_slot image_slot;
    const char *str;

    image = obj->data;
    decoder = image->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    image_data = GET_IMAGE_DATA(context, image);
    if (!image_data)
	return MOCHA_TRUE;
    image_slot = slot;
    switch (image_slot) {
    case IMAGE_SRC:
    case IMAGE_LOWSRC:
        if (dp->tag != MOCHA_STRING &&
            !MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	    return MOCHA_FALSE;
        }

	str = MOCHA_GetAtomName(mc, dp->u.atom);
        str = lm_CheckURL(mc, str);
        if (! str)
            return MOCHA_FALSE;

        if (image_slot == IMAGE_SRC) {
            ok = image_set_src(image, str, image_data);
        } else {
	    ok = lm_SaveParamString(mc, &image_data->lowres_image_url, str);
        }

        XP_FREE((void *)str);
        if (!ok)
            return MOCHA_FALSE;
        break;

    case IMAGE_NAME:
    case IMAGE_HEIGHT:
    case IMAGE_WIDTH:
    case IMAGE_BORDER:
    case IMAGE_VSPACE:
    case IMAGE_HSPACE:
    case IMAGE_COMPLETE:
	/* These are immutable. */
	break;
    }

    return image_get_property(mc, obj, slot, dp);
}

static void
image_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaImage *image = obj->data;
    LO_ImageStruct *image_data;
    MWContext *context;

    if (!image) return;

    image_data = image->image_data;
    context = image->decoder->window_context;
    if (image_data) {
        if (context && image_data->il_image)
            FE_FreeImageElement(context, image_data);
        XP_FREE(image_data->image_attr);
        XP_FREE(image_data);
    } else {
	if (context) {
	    image_data = LO_GetImageByIndex(context, image->index);
	    if (image_data)
		image_data->mocha_object = NULL;
	}
    }

    MOCHA_DropAtom(mc, image->name);
    LM_UNLINK_OBJECT(image->decoder, obj);
    MOCHA_free(mc, image);
}

static MochaClass image_class = {
    "Image",
    image_get_property, image_set_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, image_finalize
};

static MochaBoolean
Image(MochaContext *mc, MochaObject *obj,
      unsigned argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaFloat width, height;
    MochaImage *image;
    LO_ImageStruct *image_data;

    /* Check arguments first. */
    if (obj->clazz != &image_class)
	return MOCHA_TRUE;
    if (argc > 0) {
	if (argc != 2) {
	    MOCHA_ReportError(mc, "incorrect number of arguments");
	    return MOCHA_FALSE;
	}
        if (!MOCHA_DatumToNumber(mc, argv[0], &width) ||
            !MOCHA_DatumToNumber(mc, argv[1], &height)) {
            return MOCHA_FALSE;
        }
    }

    /* Allocate dummy layout structure.  This is not really
       used by layout, but the front-ends and the imagelib
       need it as a handle on an image instance. */
    image_data = XP_NEW_ZAP(LO_ImageStruct);
    if (!image_data) {
	MOCHA_ReportOutOfMemory(mc);
        return MOCHA_FALSE;
    }
    image_data->image_attr = XP_NEW_ZAP(LO_ImageAttr);
    if (!image_data->image_attr) {
        XP_FREE(image_data);
	MOCHA_ReportOutOfMemory(mc);
        return MOCHA_FALSE;
    }

    /* Fake layout ID, guaranteed not to match any real layout element */
    image_data->ele_id = -1;

    image = MOCHA_malloc(mc, sizeof *image);
    if (!image) {
	XP_FREE(image_data->image_attr);
	XP_FREE(image_data);
        return MOCHA_FALSE;
    }
    XP_BZERO(image, sizeof *image);

    image->image_data = image_data;
    image->decoder = MOCHA_GetGlobalObject(mc)->data;

    /* Events are never blocked for anonymous images
       since there is no associated layout. */
    image->pending_events = PR_BIT(LM_IMGUNBLOCK);

    obj->clazz = &image_class;
    obj->data = image;

    /* Process arguments */

    /* Width & Height */
    if (argc == 2) {
        image_data->width  = (int)width;
        image_data->height = (int)height;
    }
    image_data->mocha_object = obj;

    LM_LINK_OBJECT(image->decoder, obj, "anAnonymousImage");
    return MOCHA_TRUE;
}

#ifdef NOTYET

/* Make code like
     my_image = other_image;
   behave equivalently to
     my_image.src = other_image.src;

   It's not clear we want to do this because in some far-flung future
   where layout can dynamically be altered under control of JavaScript,
   all image properties should be copied.
   */

static MochaBoolean
image_assign(MochaContext *mc, MochaObject *obj,
             uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaDatum d;
    MWContext *context;
    MochaImage *image, *image2;

    if (!MOCHA_InstanceOf(mc, obj, &image_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    image = obj->data;
    decoder = image->decoder;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;

    XP_ASSERT(argc == 1);       /* rvalue */

    d = argv[0];
    *rval = d;

    if (d.tag != MOCHA_OBJECT && !MOCHA_ConvertDatum(mc, d, MOCHA_OBJECT, &d))
        return MOCHA_FALSE;

    if (d.u.obj->clazz != &image_class) {
        MOCHA_ReportError(mc, "incompatible right-hand side of assignment");
        return MOCHA_FALSE;
    }

    image2 = d.u.obj->data;
    return image_set_src(image, (const char *)image2->image_data->image_url);
}

static MochaFunctionSpec image_methods[] = {
    {"assign",	image_assign,	1},
    {0}
};

#endif /* NOTYET */

static MochaObject *
reflect_image_array(MochaDecoder *decoder, MochaClass *clazz)
{
    MochaContext *mc;
    MochaImageArray *array;
    MochaObject *obj;

    mc = decoder->mocha_context;
    array = MOCHA_malloc(mc, sizeof *array);
    if (!array)
	return 0;
    XP_BZERO(array, sizeof *array);
    array->decoder = decoder;

/* XXX use MOCHA_InitClass! */
    obj = MOCHA_NewObject(mc, clazz, array, 0, 0, image_array_props, 0);
    if (!obj) {
	MOCHA_free(mc, array);
	return NULL;
    }
    LM_LINK_OBJECT(decoder, obj, "images");
    return obj;
}

MochaObject *
lm_GetImageArray(MochaDecoder *decoder)
{
    MochaObject *obj;

    obj = decoder->images;
    if (obj)
	return obj;
    obj = reflect_image_array(decoder, &image_array_class);
    decoder->images = MOCHA_HoldObject(decoder->mocha_context, obj);
    return obj;
}

MochaObject *
LM_ReflectImage(MWContext *context, LO_ImageStruct *image_data, char *name,
		uint index)
{
    MochaObject *obj, *array_obj, *outer_obj;
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaImageArray *array;
    MochaImage *image;
    MochaDatum datum;

    obj = image_data->mocha_object;
    if (obj)
	return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    mc = decoder->mocha_context;

    array_obj = lm_GetImageArray(decoder);
    if (!array_obj) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    array = array_obj->data;

    image = MOCHA_malloc(mc, sizeof *image);
    if (!image) {
	LM_PutMochaDecoder(decoder);
	return NULL;
    }
    XP_BZERO(image, sizeof *image);

    outer_obj = decoder->active_form ? decoder->active_form : decoder->document;
    obj = MOCHA_NewObject(mc, &image_class, image,
			  decoder->image_prototype, outer_obj, 0, 0);
    if (!obj) {
	MOCHA_free(mc, image);
	LM_PutMochaDecoder(decoder);
	return NULL;
    }
    if (name && !MOCHA_DefineObject(mc, outer_obj, name, obj,
				    MDF_ENUMERATE | MDF_READONLY)) {
	MOCHA_DestroyObject(mc, obj);
	LM_PutMochaDecoder(decoder);
	return NULL;
    }

    image->decoder = decoder;
    image->index = index;
    image->name = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, name));

    image_data->mocha_object = obj;

    /* Put obj in the document.images array. */
    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, MDF_ENUMERATE|MDF_READONLY,
			  MOCHA_TAINT_IDENTITY, u.obj, obj);
    MOCHA_SetProperty(mc, array_obj, name, index, datum);
    if (index >= array->length)
	array->length = index + 1;

    LM_LINK_OBJECT(decoder, obj, name ? name : "anImage");
    LM_PutMochaDecoder(decoder);
    return obj;
}

void
LM_SendImageEvent(MWContext *context, LO_ImageStruct *image_data,
                  LM_ImageEvent event)
{
    char *str;
    uint event_mask;
    MochaObject *obj;
    MochaDatum result;
    MochaImage *image;

    obj = image_data->mocha_object;
    if (!obj)
        return;
    image = obj->data;
    image->pending_events |= PR_BIT(event);

    /* Special event used to trigger deferred events */
    if (! (image->pending_events & PR_BIT(LM_IMGUNBLOCK)))
        return;

    for (event = LM_IMGLOAD; event <= LM_LASTEVENT; event++) {
        event_mask = PR_BIT(event);
        if (image->pending_events & event_mask) {
            image->pending_events &= ~event_mask;
            switch (event) {
            case LM_IMGLOAD:
                str = lm_onLoad_str;
                image->complete = MOCHA_TRUE;
                break;
            case LM_IMGABORT:
                str = lm_onAbort_str;
                image->complete = MOCHA_TRUE;
                break;
            case LM_IMGERROR:
                str = lm_onError_str;
                image->complete = MOCHA_TRUE;
                break;
            default:
                XP_ABORT(("Unknown image event"));
            }

            lm_SendEvent(context, obj, str, &result);
        }
    }
}

MochaBoolean
lm_InitImageClass(MochaDecoder *decoder)
{
    MochaContext *mc;
    MochaObject *prototype;

    mc = decoder->mocha_context;
    prototype = MOCHA_InitClass(mc, decoder->window_object, &image_class, 0,
				Image, 2, image_props, 0, 0, 0);
    if (!prototype)
	return MOCHA_FALSE;
    decoder->image_prototype = MOCHA_HoldObject(mc, prototype);
    return MOCHA_TRUE;
}

/* XXX fur is disgusted */
#include "if.h"

#define GET_IC_BACKPTR(c) \
    ((il_container *)((LO_ImageStruct *)(c)->client)->alt)
#define SET_IC_BACKPTR(c, ic) \
    (((LO_ImageStruct *)(c)->client)->alt = (PA_Block)(ic))

void
lm_SaveAnonymousImages(MWContext *context, MochaDecoder *decoder)
{
    il_container_list **iclp, *icl;
    il_container *ic;
    il_client **cp, *c;
    LO_ImageStruct *image_data;

    iclp = &context->imageList;
    while ((icl = *iclp) != 0) {
	ic = icl->ic;
	cp = &ic->clients;
	while ((c = *cp) != 0) {
	    image_data = c->client;
	    if (c->cx == context &&
		(image_data->image_attr->attrmask & LO_ATTR_MOCHA_IMAGE)) {
		*cp = c->next;
		c->cx = 0;
		SET_IC_BACKPTR(c, ic);
		c->next = decoder->anon_image_clients;
		decoder->anon_image_clients = c;
		continue;
	    }
	    cp = &c->next;
	}
	for (c = ic->clients; c; c = c->next)
	    if (c->cx == context)
		break;
	if (!c) {
	    *iclp = icl->next;
	    XP_DELETE(icl);
	    context->images--;
	    continue;
	}
	iclp = &icl->next;
    }
}

MochaBoolean
lm_RestoreAnonymousImages(MWContext *context, MochaDecoder *decoder)
{
    il_client **cp, *c;
    il_container_list *icl;
    il_container *ic;

    cp = &decoder->anon_image_clients;
    while ((c = *cp) != 0) {
	ic = GET_IC_BACKPTR(c);
	for (icl = context->imageList; icl; icl = icl->next) {
	    if (icl->ic == ic)
		goto found;
	}
	icl = XP_NEW(il_container_list);
	if (!icl) {
	    MOCHA_ReportOutOfMemory(decoder->mocha_context);
	    return MOCHA_FALSE;
	}
	icl->ic = ic;
	icl->next = context->imageList;
	context->imageList = icl;
	context->images++;
    found:
	SET_IC_BACKPTR(c, 0);
	*cp = c->next;
	c->next = ic->clients;
	ic->clients = c;
	c->cx = context;
    }
    return MOCHA_TRUE;
}
