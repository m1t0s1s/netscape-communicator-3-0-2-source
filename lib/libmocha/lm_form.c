/*
** Mocha reflection of the HTML FORM elements.
**
** Brendan Eich, 9/27/95
*/
#include "lm.h"
#include "xp.h"
#include "fe_proto.h"
#include "layout.h"	/* XXX for lo_FormData only */
#include "lo_ele.h"
#include "mkutils.h"	/* XXX for NET_URL_Type() etc. in form_submit() */

enum form_array_slot {
    FORM_ARRAY_LENGTH = -1
};

static MochaPropertySpec form_array_props[] = {
    {lm_length_str,     FORM_ARRAY_LENGTH,      MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaFormArray {
    MochaDecoder        *decoder;
    uint                length;
} MochaFormArray;

static MochaBoolean
form_array_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
			MochaDatum *dp)
{
    MochaFormArray *array;
    MochaDecoder *decoder;
    MWContext *context;
    uint count;
    lo_FormData *form_data;

    array = obj->data;
    decoder = array->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    switch (slot) {
      case FORM_ARRAY_LENGTH:
	count = LO_EnumerateForms(context);
	if (count > array->length)
	    array->length = count;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, array->length);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}
	if ((uint)slot >= array->length)
	    array->length = slot + 1;
	/* NB: form IDs start at 1, not 0. */
	form_data = LO_GetFormDataByID(context, slot + 1);
	if (form_data) {
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
			     LM_ReflectForm(context, form_data));
	}
	break;
    }
    return MOCHA_TRUE;
}

static void
form_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaFormArray *array = obj->data;

    if (!array) return;
    LM_UNLINK_OBJECT(array->decoder, obj);
    MOCHA_free(mc, array);
}

static MochaClass form_array_class = {
    "FormArray",
    form_array_get_property, form_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, form_array_finalize
};

static MochaBoolean
FormArray(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

MochaObject *
lm_GetFormArray(MochaDecoder *decoder)
{
    MochaObject *obj, *prototype;
    MochaContext *mc;
    MochaFormArray *array;

    obj = decoder->forms;
    if (obj)
	return obj;
    mc = decoder->mocha_context;
    prototype = MOCHA_InitClass(mc, decoder->window_object, &form_array_class,
				0, FormArray, 0, form_array_props, 0, 0, 0);
    if (!prototype)
	return 0;

    array = MOCHA_malloc(mc, sizeof *array);
    if (!array)
	return 0;
    XP_BZERO(array, sizeof *array);
    array->decoder = decoder;

    obj = MOCHA_NewObject(mc, &form_array_class, array, prototype,
			  decoder->document, 0, 0);
    if (!obj) {
	MOCHA_free(mc, array);
	return 0;
    }
    LM_LINK_OBJECT(decoder, obj, "forms");
    decoder->forms = MOCHA_HoldObject(mc, obj);
    return obj;
}

/*
** Forms can be treated as arrays of their elements, so all named properties
** have negative slot numbers < -1.
*/
enum form_slot {
    FORM_LENGTH         = -1,
    FORM_NAME           = -2,
    FORM_ELEMENTS       = -3,
    FORM_METHOD         = -4,
    FORM_ACTION         = -5,
    FORM_ENCODING       = -6,
    FORM_TARGET         = -7
};

static char form_action_str[] = "action";

static MochaPropertySpec form_props[] = {
    {"length",          FORM_LENGTH,    MDF_ENUMERATE | MDF_READONLY},
    {"name",            FORM_NAME,      MDF_ENUMERATE},
    {"elements",        FORM_ELEMENTS,  MDF_BACKEDGE | MDF_ENUMERATE |
					MDF_READONLY},
    {"method",          FORM_METHOD,    MDF_ENUMERATE},
    {form_action_str,   FORM_ACTION,    MDF_ENUMERATE|MDF_TAINTED},
    {"encoding",        FORM_ENCODING,  MDF_ENUMERATE},
    {"target",          FORM_TARGET,    MDF_ENUMERATE},
    {0}
};

typedef struct MochaForm {
    MochaInputHandler   handler;
    intn                form_id;
    uint                length;
    MochaAtom           *name;
} MochaForm;

#define form_decoder	handler.base_decoder
#define form_type       handler.base_type
#define form_object	handler.object
#define form_event_mask	handler.event_mask

typedef struct FormMethodMap {
    char                *name;
    uint                code;
} FormMethodMap;

static FormMethodMap form_method_map[] = {
    {"get",             FORM_METHOD_GET},
    {"post",            FORM_METHOD_POST},
    {0}
};

static char *
form_method_name(uint code)
{
    FormMethodMap *mm;

    for (mm = form_method_map; mm->name; mm++)
	if (mm->code == code)
	    return mm->name;
    return "unknown";
}

static int
form_method_code(const char *name)
{
    FormMethodMap *mm;

    for (mm = form_method_map; mm->name; mm++)
	if (XP_STRCASECMP(mm->name, name) == 0)
	    return mm->code;
    return -1;
}

static MochaBoolean
form_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		  MochaDatum *dp)
{
    MochaForm *form;
    MWContext *context;
    lo_FormData *form_data;
    uint count;
    MochaAtom *atom;
    LO_Element **ele_list;
    LO_FormElementStruct *form_ele;

    form = obj->data;
    context = form->form_decoder->window_context;
    if (!context) return MOCHA_TRUE;
    form_data = LO_GetFormDataByID(context, form->form_id);
    if (!form_data)
	return MOCHA_TRUE;

    switch (slot) {
      case FORM_LENGTH:
	count = LO_EnumerateFormElements(context, form_data);
	if (count > form->length)
	    form->length = count;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, form->length);
	return MOCHA_TRUE;

      case FORM_NAME:
	atom = MOCHA_Atomize(mc, (char *)form_data->name);
	break;

      case FORM_ELEMENTS:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
			 form->form_object);
	return MOCHA_TRUE;

      case FORM_METHOD:
	atom = MOCHA_Atomize(mc, form_method_name(form_data->method));
	break;

      case FORM_ACTION:
	atom = MOCHA_Atomize(mc, (char *)form_data->action);
	break;

      case FORM_ENCODING:
	atom = MOCHA_Atomize(mc, (char *)form_data->encoding);
	break;

      case FORM_TARGET:
	atom = MOCHA_Atomize(mc, (char *)form_data->window_target);
	break;

      default:
	if ((uint)slot >= (uint)form_data->form_ele_cnt) {
	    /* Don't mess with a user-defined or method property. */
	    return MOCHA_TRUE;
	}

	PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
	form_ele = (LO_FormElementStruct *)ele_list[slot];
	if (form_ele) {
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
			     LM_ReflectFormElement(context, form_ele));
	} else {
	    *dp = MOCHA_null;
	}
	PA_UNLOCK(form_data->form_elements);
	return MOCHA_TRUE;
    }

    /* Common tail code for string-type properties. */
    if (!atom)
	return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	return lm_GetTaint(mc, form->form_decoder, &dp->taint);
    return MOCHA_TRUE;
}

static MochaBoolean
form_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		  MochaDatum *dp)
{
    MochaForm *form;
    MWContext *context;
    lo_FormData *form_data;
    const char *value;

    if (slot < FORM_TARGET || 0 <= slot) {
	/* Don't mess with a user-defined property. */
	return MOCHA_TRUE;
    }

    form = obj->data;
    context = form->form_decoder->window_context;
    if (!context) return MOCHA_TRUE;
    form_data = LO_GetFormDataByID(context, form->form_id);
    if (!form_data)
	return MOCHA_TRUE;

    if (dp->tag != MOCHA_STRING &&
	!MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	return MOCHA_FALSE;
    }

    value = MOCHA_GetAtomName(mc, dp->u.atom);
    switch (slot) {
      case FORM_METHOD:
	form_data->method = form_method_code(value);
	break;

      case FORM_ACTION:
	value = lm_CheckURL(mc, value);
	if (!value)
	    return MOCHA_FALSE;
	if (!lm_SaveParamString(mc, &form_data->action, value))
	    return MOCHA_FALSE;
	XP_FREE((char *)value);
	break;

      case FORM_ENCODING:
	if (!lm_SaveParamString(mc, &form_data->encoding, value))
	    return MOCHA_FALSE;
	break;

      case FORM_TARGET:
	if (!lm_CheckWindowName(mc, value))
	    return MOCHA_FALSE;
	if (!lm_SaveParamString(mc, &form_data->window_target, value))
	    return MOCHA_FALSE;
	break;
    }

    return form_get_property(mc, obj, slot, dp);
}

static MochaBoolean
form_list_properties(MochaContext *mc, MochaObject *obj)
{
    MochaForm *form;
    MWContext *context;
    lo_FormData *form_data;

    form = obj->data;
    context = form->form_decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    form_data = LO_GetFormDataByID(context, form->form_id);
    if (!form_data)
	return MOCHA_TRUE;
    /* XXX should return -1 on reflection error */
    (void) LO_EnumerateFormElements(context, form_data);
    return MOCHA_TRUE;
}

static MochaBoolean
form_resolve_name(MochaContext *mc, MochaObject *obj, const char *name)
{
    MochaForm *form;
    MWContext *context;
    lo_FormData *form_data;
    LO_Element **ele_list;
    LO_FormElementStruct *form_ele;
    lo_FormElementMinimalData *min_data;
    int32 i;

    form = obj->data;
    context = form->form_decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    form_data = LO_GetFormDataByID(context, form->form_id);
    if (!form_data)
	return MOCHA_TRUE;

    PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
    for (i = 0; i < form_data->form_ele_cnt; i++) {
	if (ele_list[i]->type != LO_FORM_ELE)
	    continue;
	form_ele = (LO_FormElementStruct *)ele_list[i];
	if (!form_ele->element_data)
	    continue;
	min_data = &form_ele->element_data->ele_minimal;
	if (min_data->name && XP_STRCMP((char *)min_data->name, name) == 0)
	    (void) LM_ReflectFormElement(context, form_ele);
    }
    PA_UNLOCK(form_data->form_elements);
    return MOCHA_TRUE;
}

static void
form_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaForm *form;

    form = obj->data;
    if (!form) return;
    LM_UNLINK_OBJECT(form->form_decoder, obj);
    MOCHA_free(mc, form);
}

static MochaClass form_class = {
    "Form",
    form_get_property, form_set_property, form_list_properties,
    form_resolve_name, MOCHA_ConvertStub, form_finalize
};

static MochaBoolean
Form(MochaContext *mc, MochaObject *obj,
     uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
form_native_prolog(MochaContext *mc, MochaObject *obj, MochaDatum *argv,
		   MochaForm **formp, lo_FormData **form_datap)
{
    MochaForm *form;
    MochaDecoder *decoder;
    lo_FormData *form_data;

    if (!MOCHA_InstanceOf(mc, obj, &form_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    form = obj->data;
    decoder = form->form_decoder;
    form_data = decoder->window_context
		? LO_GetFormDataByID(decoder->window_context, form->form_id)
		: 0;
    *formp = form;
    *form_datap = form_data;
    return MOCHA_TRUE;
}

static MochaBoolean
form_reset(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaForm *form;
    lo_FormData *form_data;
    LO_Element **ele_list;

    if (!form_native_prolog(mc, obj, argv, &form, &form_data))
        return MOCHA_FALSE;
    if (form_data && form_data->form_ele_cnt > 0) {
	/* There is no form LO_Element; use the first thing in the form. */
	PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
	LO_ResetForm(form->form_decoder->window_context,
		     (LO_FormElementStruct *)ele_list[0]);
	PA_UNLOCK(form_data->form_elements);
    }
    return MOCHA_TRUE;
}

static MochaBoolean
form_check_taint(MochaContext *mc, lo_FormData *form_data)
{
    MochaObject *obj;
    MochaDatum datum;
    const char *action;
    MochaDecoder *decoder;
    uint16 accum, taint;
    LO_Element **ele_list;
    int32 i;
    MochaBoolean ok;

    obj = form_data->mocha_object;
    if (obj) {
	/* Check the action property only if it was set by Mocha. */
	if (!MOCHA_LookupName(mc, obj, form_action_str, &datum))
	    return MOCHA_FALSE;
	XP_ASSERT(datum.tag == MOCHA_STRING);
	action = MOCHA_GetAtomName(mc, datum.u.atom);
	if (!lm_CheckTaint(mc, datum.taint, "form.action property", action))
	    return MOCHA_FALSE;
    } else {
	PA_LOCK(action, const char *, form_data->action);
    }

    decoder = MOCHA_GetGlobalObject(mc)->data;
    accum = decoder->write_taint_accum;
    PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
    for (i = 0; i < form_data->form_ele_cnt; i++) {
	taint = lm_GetFormElementTaint(mc, (LO_FormElementStruct *)ele_list[i]);
	MOCHA_MIX_TAINT(mc, accum, taint);
    }
    PA_UNLOCK(form_data->form_elements);
    ok = action ? lm_CheckTaint(mc, accum, "form submission to", action)
		: MOCHA_TRUE;
    if (!obj)
	PA_UNLOCK(form_data->action);
    return ok;
}

static MochaBoolean
form_submit(MochaContext *mc, MochaObject *obj,
	    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaForm *form;
    lo_FormData *form_data;
    const char *action;
    MochaBoolean ok;
    MochaDecoder *decoder;
    MWContext *context;
    LO_Element **ele_list, *element;
    LO_FormSubmitData *submit;
    URL_Struct *url_struct;
    History_entry *he;

    if (!form_native_prolog(mc, obj, argv, &form, &form_data))
        return MOCHA_FALSE;
    if (!form_data)
        return MOCHA_TRUE;

    PA_LOCK(action, const char *, form_data->action);
    switch (NET_URL_Type(action)) {
      case MAILTO_TYPE_URL:
      case NEWS_TYPE_URL:
	ok = MOCHA_FALSE;
	break;
      default:
	ok = MOCHA_TRUE;
	break;
    }
    PA_UNLOCK(form_data->action);
    if (!ok) {
	/* XXX silently fail this mailto: or news: form post. */
	return MOCHA_TRUE;
    }

    if (!form_check_taint(mc, form_data))
	return MOCHA_FALSE;

    if (form_data->form_ele_cnt > 0) {
	decoder = form->form_decoder;
	context = decoder->window_context;
	PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
	element = ele_list[0];
	PA_UNLOCK(form_data->form_elements);

/* XXX begin FE_SubmitInputElement, but use lm_GetURL to avoid reentrancy */
	submit = LO_SubmitForm(context, (LO_FormElementStruct *) element);
	if (!submit) {
	    MOCHA_ReportOutOfMemory(mc);
	    return MOCHA_FALSE;
	}

	/* Create the URL to load. */
	url_struct = NET_CreateURLStruct((char *)submit->action,
					 NET_DONT_RELOAD);
	if (!url_struct) {
	    MOCHA_ReportOutOfMemory(mc);
	    ok = MOCHA_FALSE;
	} else {
	    /* Add the form data. */
	    NET_AddLOSubmitDataToURLStruct(submit, url_struct);

	    /* Set the referrer field if there is one. */
	    he = SHIST_GetCurrent(&context->hist);
	    if (he && he->address) {
		url_struct->referer = MOCHA_strdup(mc, he->address);
		if (!url_struct->referer)
		    ok = MOCHA_FALSE;
	    }

	    /* Get the URL to submit the form. */
	    if (ok) {
		ok = lm_GetURL(mc, decoder, url_struct,
			       MOCHA_TAINT_IDENTITY);
	    }
	}
	LO_FreeSubmitData(submit);
/* XXX end FE_SubmitInputElement */
    }
    return ok;
}

static MochaFunctionSpec form_methods[] = {
    {"reset",	form_reset,	0},
    {"submit",	form_submit,	0},
    {0}
};

static MochaBoolean
form_event(MWContext *context, LO_Element *element, uint32 event, char *method,
	   lo_FormData **form_datap)
{
    lo_FormData *form_data;
    MochaObject *obj;
    MochaForm *form;
    MochaContext *mc;
    MochaBoolean ok;
    MochaDatum rval;

    if (element->type != LO_FORM_ELE &&
	(element->type != LO_IMAGE || !element->lo_image.image_attr)) {
	*form_datap = 0;
	return MOCHA_TRUE;
    }
    form_data = LO_GetFormDataByID(context,
				   (element->type == LO_IMAGE)
				   ? element->lo_image.image_attr->form_id
				   : element->lo_form.form_id);
    *form_datap = form_data;
    if (!form_data || !(obj = form_data->mocha_object))
	return MOCHA_TRUE;
    form = obj->data;
    if (form->form_event_mask & event)
	return MOCHA_TRUE;

    mc = form->form_decoder->mocha_context;
    MOCHA_HoldObject(mc, obj);
    form->form_event_mask |= event;
    ok = lm_SendEvent(context, obj, method, &rval);
    form->form_event_mask &= ~event;
    MOCHA_DropObject(mc, obj);
    if (ok && rval.tag == MOCHA_BOOLEAN && rval.u.bval == MOCHA_FALSE)
	return MOCHA_FALSE;
    return MOCHA_TRUE;
}

MochaBoolean
LM_SendOnReset(MWContext *context, LO_Element *element)
{
    lo_FormData *form_data;
    MochaDecoder *decoder;
    LO_Element **ele_list;
    int32 i;

    if (!context->mocha_context)
	return MOCHA_TRUE;
    if (!form_event(context, element, EVENT_RESET, lm_onReset_str, &form_data))
	return MOCHA_FALSE;
    if (form_data) {
	decoder = LM_GetMochaDecoder(context);
	if (!decoder)
	    return MOCHA_FALSE;
	PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
	for (i = 0; i < form_data->form_ele_cnt; i++) {
	    lm_ResetFormElementTaint(decoder->mocha_context,
				     (LO_FormElementStruct *)ele_list[i]);
	}
	PA_UNLOCK(form_data->form_elements);
	LM_PutMochaDecoder(decoder);
    }
    return MOCHA_TRUE;
}

MochaBoolean
LM_SendOnSubmit(MWContext *context, LO_Element *element)
{
    lo_FormData *form_data;
    MochaContext *mc;

    if (!form_event(context, element, EVENT_SUBMIT, lm_onSubmit_str,
		    &form_data)) {
	return MOCHA_FALSE;
    }
    if (!form_data)
	return MOCHA_TRUE;

    mc = lm_running_context;
    if (!mc) {
	mc = context->mocha_context;
	if (!mc)
	    return MOCHA_TRUE;
    }
    return form_check_taint(mc, form_data);
}

#ifdef DEBUG_brendan
static char *
form_name(lo_FormData *form_data)
{
    static char buf[20];

    if (form_data->name)
	return (char *)form_data->name;
    PR_snprintf(buf, sizeof buf, "$form%d", form_data->id - 1);
    return buf;
}
#endif

MochaObject *
LM_ReflectForm(MWContext *context, lo_FormData *form_data)
{
    MochaObject *obj, *array_obj, *prototype;
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaFormArray *array;
    MochaForm *form;
    char *name;
    MochaBoolean ok;
    MochaDatum datum;

    obj = form_data->mocha_object;
    if (obj)
	return obj;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    array_obj = lm_GetFormArray(decoder);
    if (!array_obj) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    array = array_obj->data;

    mc = decoder->mocha_context;
    prototype = decoder->form_prototype;
    if (!prototype) {
	prototype =  MOCHA_InitClass(mc, decoder->window_object, &form_class,
				     0, Form, 0, form_props, form_methods,
				     0, 0);
	if (!prototype) {
	    LM_PutMochaDecoder(decoder);
	    return 0;
	}
	decoder->form_prototype = MOCHA_HoldObject(mc, prototype);
    }

    form = MOCHA_malloc(mc, sizeof *form);
    if (!form) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    XP_BZERO(form, sizeof *form);
    form->form_decoder = decoder;
    form->form_type = FORM_TYPE_NONE;

    obj = MOCHA_NewObject(mc, &form_class, form, prototype,
			  decoder->document, 0, 0);
    if (!obj) {
	MOCHA_free(mc, form);
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    if (form_data->name) {
	PA_LOCK(name, char *, form_data->name);
	ok = MOCHA_DefineObject(mc, decoder->document, name, obj,
				MDF_ENUMERATE | MDF_READONLY);
	PA_UNLOCK(form_data->name);
	if (!ok) {
	    MOCHA_DestroyObject(mc, obj);
	    LM_PutMochaDecoder(decoder);
	    return 0;
	}
    }

    if (form_data->name) {
	PA_LOCK(name, char *, form_data->name);
    } else {
	name = 0;
    }
    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, MDF_ENUMERATE|MDF_READONLY,
			  MOCHA_TAINT_IDENTITY, u.obj, obj);
    MOCHA_SetProperty(mc, decoder->forms, name, form_data->id - 1, datum);
    if (form_data->name)
	PA_UNLOCK(form_data->name);
    if ((uint)form_data->id > array->length)
	array->length = form_data->id;

    form->form_object = obj;
    form->form_id = form_data->id;
    form_data->mocha_object = obj;

    LM_LINK_OBJECT(decoder, obj, form_name(form_data));
    LM_PutMochaDecoder(decoder);
    return obj;
}

MochaObject *
lm_GetFormObjectByID(MochaDecoder *decoder, uint form_id)
{
    MWContext *context;
    lo_FormData *form_data;

    context = decoder->window_context;
    if (!context)
	return 0;
    form_data = LO_GetFormDataByID(context, form_id);
    if (!form_data)
	return 0;
    return form_data->mocha_object;
}

LO_FormElementStruct *
lm_GetFormElementByIndex(MochaObject *form_obj, int32 index)
{
    MochaForm *form;
    MWContext *context;
    lo_FormData *form_data;

    if (!form_obj)
	return 0;
    form = form_obj->data;
    context = form->form_decoder->window_context;
    if (!context)
	return 0;
    form_data = LO_GetFormDataByID(context, form->form_id);
    if (!form_data)
	return 0;
    return LO_GetFormElementByIndex(form_data, index);
}

MochaBoolean
lm_AddFormElement(MochaContext *mc, MochaObject *form_obj, MochaObject *ele_obj,
		  char *name, uint index)
{
    MochaForm *form;
    MWContext *context;
    lo_FormData *form_data;
    LO_Element **ele_list;
    LO_FormElementStruct *first_ele;
    uint first_index;
    MochaDatum datum;

    form = form_obj->data;
    context = form->form_decoder->window_context;
    if (!context)
	return MOCHA_FALSE;
    form_data = LO_GetFormDataByID(context, form->form_id);
    if (!form_data)
	return MOCHA_FALSE;

    /* XXX confine this to laymocha.c where LO_GetFormByElementIndex() lives */
    if (form_data->form_ele_cnt == 0) {
	index = 0;
    } else {
	PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
	first_ele = (LO_FormElementStruct *)ele_list[0];
	first_index = (uint)first_ele->element_index;
	PA_UNLOCK(form_data->form_elements);
	XP_ASSERT(index >= first_index);
	if (index < first_index)
	    return MOCHA_FALSE;
	index -= first_index;
    }

    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, MDF_ENUMERATE|MDF_READONLY,
			  MOCHA_TAINT_IDENTITY, u.obj, ele_obj);
    if (!MOCHA_SetProperty(mc, form_obj, name, index, datum))
	return MOCHA_FALSE;

    if (index >= form->length)
	form->length = index + 1;
    return MOCHA_TRUE;
}

MochaBoolean
lm_ReflectRadioButtonArray(MWContext *context, intn form_id, const char *name)
{
    lo_FormData *form_data;
    MochaBoolean ok;
    LO_Element **ele_list;
    int32 i;
    LO_FormElementStruct *form_ele;
    LO_FormElementData *data;

    form_data = LO_GetFormDataByID(context, form_id);
    if (!form_data)
	return MOCHA_FALSE;
    ok = MOCHA_TRUE;
    PA_LOCK(ele_list, LO_Element **, form_data->form_elements);
    for (i = 0; i < form_data->form_ele_cnt; i++) {
	form_ele = (LO_FormElementStruct *)ele_list[i];
	if ((data = form_ele->element_data) &&
	    data->ele_minimal.name &&
	    !XP_STRCMP((char *)data->ele_minimal.name, name) &&
	    form_ele->mocha_object == NULL) {
	    ok = (LM_ReflectFormElement(context, form_ele) != 0);
	    if (!ok)
		break;
	}
    }
    PA_UNLOCK(form_data->form_elements);
    return ok;
}
