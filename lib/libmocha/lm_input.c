/*
** Mocha input focus and event notifiers.
**
** Brendan Eich, 9/27/95
**
** XXX SIZE, MAXLENGTH attributes
*/
#include "lm.h"
#include "xp.h"
#include "fe_proto.h"
#include "lo_ele.h"
#include "pa_tags.h"
#include "layout.h"		/* XXX just for S_FORM_TYPE_... names */
#include "prmem.h"
#include "prprf.h"

enum input_slot {
    INPUT_TYPE              = -1,
    INPUT_NAME              = -2,
    INPUT_FORM              = -3,
    INPUT_VALUE             = -4,
    INPUT_DEFAULT_VALUE     = -5,
    INPUT_LENGTH            = -6,
    INPUT_OPTIONS           = -7,
    INPUT_SELECTED_INDEX    = -8,
    INPUT_STATUS            = -9,
    INPUT_DEFAULT_STATUS    = -10
};

static char lm_options_str[] = "options";

static MochaPropertySpec input_props[] = {
    {"type",                INPUT_TYPE,             MDF_ENUMERATE|MDF_READONLY},
    {"name",                INPUT_NAME,             MDF_ENUMERATE|MDF_TAINTED},
    {"form",                INPUT_FORM,             MDF_BACKEDGE|MDF_ENUMERATE|
                                                    MDF_READONLY},
    {"value",               INPUT_VALUE,            MDF_ENUMERATE|MDF_TAINTED},
    {"defaultValue",        INPUT_DEFAULT_VALUE,    MDF_ENUMERATE|MDF_TAINTED},
    {lm_length_str,         INPUT_LENGTH,           MDF_ENUMERATE},
    {lm_options_str,        INPUT_OPTIONS,          MDF_BACKEDGE|MDF_ENUMERATE|
                                                    MDF_READONLY},
    {"selectedIndex",       INPUT_SELECTED_INDEX,   MDF_ENUMERATE|MDF_TAINTED},
    {"status",              INPUT_STATUS,           MDF_TAINTED},
    {"defaultStatus",       INPUT_DEFAULT_STATUS,   MDF_TAINTED},
    {PARAM_CHECKED,         INPUT_STATUS,           MDF_ENUMERATE|MDF_TAINTED},
    {"defaultChecked",      INPUT_DEFAULT_STATUS,   MDF_ENUMERATE|MDF_TAINTED},
    {0}
};

/*
** Base input element type.
*/
typedef struct MochaInput {
    MochaInputHandler       handler;
    int32                   index;
} MochaInput;

#define input_decoder       handler.base_decoder
#define input_type          handler.base_type
#define input_object        handler.object
#define input_event_mask    handler.event_mask

/*
** Text and textarea input type.
*/
typedef struct MochaTextInput {
    MochaInput              input;
} MochaTextInput;

/*
** Select option tag reflected type.
*/
enum option_slot {
    OPTION_INDEX            = -1,
    OPTION_TEXT             = -2,
    OPTION_VALUE            = -3,
    OPTION_DEFAULT_SELECTED = -4,
    OPTION_SELECTED         = -5
};

static MochaPropertySpec option_props[] = {
    {"index",           OPTION_INDEX,            MDF_ENUMERATE|MDF_READONLY},
    {"text",            OPTION_TEXT,             MDF_ENUMERATE|MDF_TAINTED},
    {"value",           OPTION_VALUE,            MDF_ENUMERATE|MDF_TAINTED},
    {"defaultSelected", OPTION_DEFAULT_SELECTED, MDF_ENUMERATE|MDF_TAINTED},
    {"selected",        OPTION_SELECTED,         MDF_ENUMERATE|MDF_TAINTED},
    {0}
};

typedef struct MochaSelectOption {
    MochaDecoder            *decoder;
    MochaObject             *object;
    uint32                  index;
    int32                   indexInForm;
    lo_FormElementOptionData *data;
} MochaSelectOption;

static MochaBoolean
option_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		    MochaDatum *dp)
{
    MochaSelectOption *option;
    lo_FormElementOptionData *optionData;
    lo_FormElementSelectData *selectData;
    LO_FormElementStruct *form_element;
    enum option_slot option_slot;
    MochaAtom *atom;
    char *value;

    option = obj->data;
    optionData = option->data;
    if (optionData) {
	selectData = 0;
	form_element = 0;
    } else {
	if (!obj->parent)
	    return MOCHA_TRUE;
	form_element = lm_GetFormElementByIndex(obj->parent->parent,
						option->indexInForm);
	if (!form_element)
	    return MOCHA_TRUE;
	selectData = &form_element->element_data->ele_select;
    }
    option_slot = slot;
    switch (option_slot) {
      case OPTION_INDEX:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, option->index);
	break;

      case OPTION_TEXT:
      case OPTION_VALUE:
	if (selectData) {
	    PA_LOCK(optionData, lo_FormElementOptionData *,
		    selectData->options);
	}
	if (slot == OPTION_TEXT) {
	    value = (char *)optionData[option->index].text_value;
	    atom = MOCHA_Atomize(mc, value);
	} else {
	    value = (char *)optionData[option->index].value;
	    atom = MOCHA_Atomize(mc, value);
	}
	if (selectData)
	    PA_UNLOCK(selectData->options);
	if (!atom)
	    return MOCHA_FALSE;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
	break;

      case OPTION_DEFAULT_SELECTED:
      case OPTION_SELECTED:
	if (selectData) {
	    PA_LOCK(optionData, lo_FormElementOptionData *,
		    selectData->options);
	}
	MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, u.bval,
			 (option_slot == OPTION_DEFAULT_SELECTED)
			 ? optionData[option->index].def_selected
			 : optionData[option->index].selected);
	if (selectData)
	    PA_UNLOCK(selectData->options);
	break;

      default:
	/* Don't mess with a user-defined or method property. */
	break;
    }
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	return lm_GetTaint(mc, option->decoder, &dp->taint);
    return MOCHA_TRUE;
}

static MochaBoolean
option_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		    MochaDatum *dp)
{
    MochaSelectOption *option;
    lo_FormElementOptionData *optionData;
    lo_FormElementSelectData *selectData;
    LO_FormElementStruct *form_element;
    enum option_slot option_slot;
    MochaBoolean showChange;
    int32 i;

    option = obj->data;
    optionData = option->data;
    if (optionData) {
	selectData = 0;
	form_element = 0;
    } else {
	if (!obj->parent)
	    return MOCHA_TRUE;
	form_element = lm_GetFormElementByIndex(obj->parent->parent,
						option->indexInForm);
	if (!form_element)
	    return TRUE;
	selectData = &form_element->element_data->ele_select;
    }

    option_slot = slot;
    showChange = MOCHA_FALSE;
    switch (option_slot) {
      case OPTION_TEXT:
      case OPTION_VALUE:
	if (dp->tag != MOCHA_STRING &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	    return MOCHA_FALSE;
	}
	if (selectData) {
	    PA_LOCK(optionData, lo_FormElementOptionData *,
		    selectData->options);
	}
	if (option_slot == OPTION_TEXT) {
	    if (!lm_SaveParamString(mc, &optionData[option->index].text_value,
				    MOCHA_GetAtomName(mc, dp->u.atom))) {
		return MOCHA_FALSE;
	    }
	    showChange = MOCHA_TRUE;
	} else {
	    if (!lm_SaveParamString(mc, &optionData[option->index].value,
				    MOCHA_GetAtomName(mc, dp->u.atom))) {
		return MOCHA_FALSE;
	    }
	}
	if (selectData)
	    PA_UNLOCK(selectData->options);
	break;

      case OPTION_DEFAULT_SELECTED:
      case OPTION_SELECTED:
	if (dp->tag != MOCHA_BOOLEAN &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_BOOLEAN, dp)) {
	    return MOCHA_FALSE;
	}

	if (selectData) {
	    PA_LOCK(optionData, lo_FormElementOptionData *,
		    selectData->options);
	}
	if (option_slot == OPTION_DEFAULT_SELECTED)
	    optionData[option->index].def_selected = dp->u.bval;
	else
	    optionData[option->index].selected = dp->u.bval;
	if (selectData) {
	    if (dp->u.bval && !selectData->multiple) {
		/* Clear all the others. */
		for (i = 0; i < selectData->option_cnt; i++) {
		    if ((uint32)i == option->index)
			continue;
		    if (option_slot == OPTION_DEFAULT_SELECTED)
			optionData[i].def_selected = FALSE;
		    else
			optionData[i].selected = FALSE;
		}
	    }
	    PA_UNLOCK(selectData->options);
	}

	if (option_slot == OPTION_SELECTED)
	    showChange = MOCHA_TRUE;
	break;

      default:
	/* Don't mess with a user-defined property. */
	return TRUE;
    }

    if (showChange && option->decoder->window_context && form_element) {
	FE_ChangeInputElement(option->decoder->window_context,
			      (LO_Element *)form_element);
    }

    return option_get_property(mc, obj, slot, dp);
}

static void
option_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaSelectOption *option;
    lo_FormElementOptionData *optionData;

    option = obj->data;
    if (!option) return;
    LM_UNLINK_OBJECT(option->decoder, obj);
    optionData = option->data;
    if (optionData) {
	if (optionData->text_value)
	    MOCHA_free(mc, optionData->text_value);
	if (optionData->value)
	    MOCHA_free(mc, optionData->value);
	MOCHA_free(mc, optionData);
    }
    MOCHA_free(mc, option);
}

static MochaClass option_class = {
    "Option",
    option_get_property, option_set_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, option_finalize
};

/*
** Select option constructor, can be called any of these ways:
**  opt = new Option()
**  opt = new Option(text)
**  opt = new Option(text, value)
**  opt = new Option(text, value, defaultSelected)
**  opt = new Option(text, value, defaultSelected, selected)
** Where opt can be selectData.options[i] for any nonnegative integer i.
*/
static MochaBoolean
Option(MochaContext *mc, MochaObject *obj,
       uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaSelectOption *option;
    lo_FormElementOptionData *optionData;
    MochaAtom *atom;
    MochaBoolean bval;

    if (obj->clazz != &option_class) {
	*rval = MOCHA_null;
	return MOCHA_TRUE;
    }

    decoder = MOCHA_GetGlobalObject(mc)->data;
    option = MOCHA_malloc(mc, sizeof *option);
    if (!option)
	return MOCHA_FALSE;
    XP_BZERO(option, sizeof *option);
    obj->data = option;
    LM_LINK_OBJECT(decoder, obj, "anOption");

    optionData = MOCHA_malloc(mc, sizeof *optionData);
    if (!optionData)
	goto fail;
    XP_BZERO(optionData, sizeof *optionData);
    option->data = optionData;

    if (argc >= 4) {
	if (argv[3].tag != MOCHA_BOOLEAN &&
	    !MOCHA_DatumToBoolean(mc, argv[3], &bval)) {
	    goto fail;
	}
	optionData->selected = bval;
    }
    if (argc >= 3) {
	if (argv[2].tag != MOCHA_BOOLEAN &&
	    !MOCHA_DatumToBoolean(mc, argv[2], &bval)) {
	    goto fail;
	}
	optionData->def_selected = bval;
    }
    if (argc >= 2) {
	if (argv[1].tag == MOCHA_STRING)
	    atom = MOCHA_HoldAtom(mc, argv[1].u.atom);
	else if (!MOCHA_DatumToString(mc, argv[1], &atom))
	    goto fail;
	optionData->value =
	    (PA_Block)MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom));
	MOCHA_DropAtom(mc, atom); 
	if (!optionData->value)
	    goto fail;
    }
    if (argc >= 1) {
	if (argv[0].tag == MOCHA_STRING)
	    atom = MOCHA_HoldAtom(mc, argv[0].u.atom);
	else if (!MOCHA_DatumToString(mc, argv[0], &atom))
	    goto fail;
	optionData->text_value =
	    (PA_Block)MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom));
	MOCHA_DropAtom(mc, atom);
	if (!optionData->text_value)
	    goto fail;
    }

    option->decoder = decoder;
    option->object = obj;
    option->index = 0;		/* so option->data[option->index] works */
    option->indexInForm = -1;
    return MOCHA_TRUE;

fail:
    option_finalize(mc, obj);
    obj->data = 0;
    return MOCHA_FALSE;
}

static char *typenames[] = {
    "none",
    S_FORM_TYPE_TEXT,
    S_FORM_TYPE_RADIO,
    S_FORM_TYPE_CHECKBOX,
    S_FORM_TYPE_HIDDEN,
    S_FORM_TYPE_SUBMIT,
    S_FORM_TYPE_RESET,
    S_FORM_TYPE_PASSWORD,
    S_FORM_TYPE_BUTTON,
    S_FORM_TYPE_JOT,
    "select-one",
    "select-multiple",
    "textarea",
    "isindex",
    S_FORM_TYPE_IMAGE,
    S_FORM_TYPE_FILE,
    "keygen",
    S_FORM_TYPE_READONLY
};

/*
** Note early returns below, to avoid common string-valued property code at
** the bottom of the function.
*/
static MochaBoolean
input_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		   MochaDatum *dp)
{
    MochaInput *input;
    enum input_slot input_slot;
    LO_FormElementStruct *form_element;
    MochaObject *option_obj;
    MochaAtom *atom;

    input = obj->data;
    input_slot = slot;
    if (input_slot == INPUT_FORM) {
	/* Each input in a form has a back-pointer to its form. */
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, obj->parent);
	return MOCHA_TRUE;
    }

    form_element = lm_GetFormElementByIndex(obj->parent, input->index);
    if (!form_element)
	return MOCHA_TRUE;

    if (input_slot == INPUT_TYPE) {
	uint type_index;

	type_index = form_element->element_data->type;
	if (type_index >= sizeof typenames / sizeof typenames[0]) {
	    MOCHA_ReportError(mc, "unknown form element type %u", type_index);
	    return MOCHA_FALSE;
	}
	atom = MOCHA_Atomize(mc, typenames[type_index]);
	if (!atom) return MOCHA_FALSE;
	MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
	return MOCHA_TRUE;
    }

    switch (form_element->element_data->type) {
      case FORM_TYPE_TEXT:
      case FORM_TYPE_TEXTAREA:	/* XXX we ASSUME common struct prefixes */
      case FORM_TYPE_FILE:	/* XXX as above, also get-only, set is no-op */
      case FORM_TYPE_PASSWORD:
	{
	    lo_FormElementTextData *text;

	    text = &form_element->element_data->ele_text;
	    switch (input_slot) {
	      case INPUT_NAME:
		/* XXX should PA_LOCK/PA_UNLOCK but why bother? */
		atom = MOCHA_Atomize(mc, (char *)text->name);
		break;
	      case INPUT_VALUE:
		atom = MOCHA_Atomize(mc, (char *)text->current_text);
		break;
	      case INPUT_DEFAULT_VALUE:
		atom = MOCHA_Atomize(mc, (char *)text->default_text);
		break;
	      default:
		/* Don't mess with a user-defined property. */
		return MOCHA_TRUE;
	    }
	}
	break;

      case FORM_TYPE_SELECT_ONE:
      case FORM_TYPE_SELECT_MULT:
	{
	    lo_FormElementSelectData *selectData;
	    lo_FormElementOptionData *optionData;
	    int32 i;
	    MochaSelectOption *option;

	    selectData = &form_element->element_data->ele_select;
	    switch (input_slot) {
	      case INPUT_NAME:
		atom = MOCHA_Atomize(mc, (char *)selectData->name);
		break;

	      case INPUT_LENGTH:
		MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval,
				 selectData->option_cnt);
		return MOCHA_TRUE;

	      case INPUT_OPTIONS:
		MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
				 input->input_object);
		return MOCHA_TRUE;

	      case INPUT_SELECTED_INDEX:
		MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, -1);
		PA_LOCK(optionData, lo_FormElementOptionData *,
			selectData->options);
		for (i = 0; i < selectData->option_cnt; i++) {
		    if (optionData[i].selected) {
			dp->u.fval = i;
			break;
		    }
		}
		PA_UNLOCK(selectData->options);
		return MOCHA_TRUE;

	      default:
		if ((uint32)slot >= (uint32)selectData->option_cnt)
		    return MOCHA_TRUE;
		if (dp->tag == MOCHA_OBJECT && dp->u.obj) {
		    XP_ASSERT(dp->u.obj->clazz == &option_class);
		    return MOCHA_TRUE;
		}

		option = MOCHA_malloc(mc, sizeof *option);
		if (!option)
		    return MOCHA_FALSE;
		option_obj =
		    MOCHA_NewObject(mc, &option_class, option,
				    input->input_decoder->option_prototype, obj,
				    0, 0);
		if (!option_obj) {
		    MOCHA_free(mc, option);
		    return MOCHA_FALSE;
		}
		LM_LINK_OBJECT(input->input_decoder, option_obj, "anOption");
		option->decoder = input->input_decoder;
		option->object = option_obj;
		option->index = (uint32)slot;
		option->indexInForm = form_element->element_index;
		option->data = 0;
		MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, option_obj);
		return MOCHA_TRUE;
	    }
	}
	break;

      case FORM_TYPE_RADIO:
      case FORM_TYPE_CHECKBOX:
	{
	    lo_FormElementToggleData *toggle;

	    toggle = &form_element->element_data->ele_toggle;
	    switch (input_slot) {
	      case INPUT_NAME:
		atom = MOCHA_Atomize(mc, (char *)toggle->name);
		break;
	      case INPUT_VALUE:
		atom = MOCHA_Atomize(mc, (char *)toggle->value);
		break;
	      case INPUT_STATUS:
		MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, u.bval, toggle->toggled);
		return MOCHA_TRUE;
	      case INPUT_DEFAULT_STATUS:
		MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, u.bval,
				 toggle->default_toggle);
		return MOCHA_TRUE;
	      default:
		/* Don't mess with a user-defined property. */
		return MOCHA_TRUE;
	    }
	}
	break;

      default:
	{
	    lo_FormElementMinimalData *minimal;

	    minimal = &form_element->element_data->ele_minimal;
	    switch (input_slot) {
	      case INPUT_NAME:
		atom = MOCHA_Atomize(mc, (char *)minimal->name);
		break;
	      case INPUT_VALUE:
		atom = MOCHA_Atomize(mc, (char *)minimal->value);
		break;
	      default:
		/* Don't mess with a user-defined property. */
		return MOCHA_TRUE;
	    }
	}
	break;
    }

    if (!atom)
	return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	return lm_GetTaint(mc, input->input_decoder, &dp->taint);
    return MOCHA_TRUE;
}

static char *
lm_fix_newlines(MochaContext *mc, const char *value)
{
    size_t size;
    const char *cp;
    char *tp, *new_value;

#if defined XP_PC
    size = 1;
    for (cp = value; *cp != '\0'; cp++) {
	switch (*cp) {
	  case '\r':
	    if (cp[1] != '\n')
		size++;
	    break;
	  case '\n':
	    if (cp > value && cp[-1] != '\r')
		size++;
	    break;
	}
    }
    size += cp - value;
#else
    size = XP_STRLEN(value) + 1;
#endif
    new_value = MOCHA_malloc(mc, size);
    if (!new_value)
	return 0;
    for (cp = value, tp = new_value; *cp != '\0'; cp++) {
#if defined XP_MAC
	if (*cp == '\n') {
	    if (cp > value && cp[-1] != '\r')
		*tp++ = '\r';
	} else {
	    *tp++ = *cp;
	}
#elif defined XP_PC
	switch (*cp) {
	  case '\r':
	    *tp++ = '\r';
	    if (cp[1] != '\n')
		*tp++ = '\n';
	    break;
	  case '\n':
	    if (cp > value && cp[-1] != '\r')
		*tp++ = '\r';
	    *tp++ = '\n';
	    break;
	  default:
	    *tp++ = *cp;
	    break;
	}
#else /* XP_UNIX */
	if (*cp == '\r') {
	    if (cp[1] != '\n')
		*tp++ = '\n';
	} else {
	    *tp++ = *cp;
	}
#endif
    }
    *tp = '\0';
    return new_value;
}

static MochaBoolean
input_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		   MochaDatum *dp)
{
    MochaInput *input;
    enum input_slot input_slot;
    const char *value;
    LO_FormElementStruct *form_element;
    MWContext *context;

    input = obj->data;
    input_slot = slot;
    switch (input_slot) {
      case INPUT_TYPE:
      case INPUT_FORM:
      case INPUT_OPTIONS:
	/* These are immutable. */
	break;
      case INPUT_NAME:
      case INPUT_VALUE:
      case INPUT_DEFAULT_VALUE:
	/* These are string-valued. */
	if (dp->tag != MOCHA_STRING &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	    return MOCHA_FALSE;
	}
	value = MOCHA_GetAtomName(mc, dp->u.atom);
	break;
      case INPUT_STATUS:
      case INPUT_DEFAULT_STATUS:
	/* These must be Booleans. */
	if (dp->tag != MOCHA_BOOLEAN &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_BOOLEAN, dp)) {
	    return MOCHA_FALSE;
	}
	break;
      case INPUT_LENGTH:
      case INPUT_SELECTED_INDEX:
	/* These should be a number. */
	if (dp->tag != MOCHA_NUMBER &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_NUMBER, dp)) {
	    return MOCHA_FALSE;
	}
	break;
    }

    form_element = lm_GetFormElementByIndex(obj->parent, input->index);
    if (!form_element)
	return MOCHA_TRUE;
    context = input->input_decoder->window_context;
    switch (form_element->element_data->type) {
      case FORM_TYPE_TEXT:
      case FORM_TYPE_TEXTAREA:	/* XXX we ASSUME common struct prefixes */
      case FORM_TYPE_PASSWORD:
	{
	    lo_FormElementTextData *text;
	    MochaBoolean ok;

	    text = &form_element->element_data->ele_text;
	    switch (input_slot) {
	      case INPUT_NAME:
		if (!lm_SaveParamString(mc, &text->name, value))
		    return MOCHA_FALSE;
		break;
	      case INPUT_VALUE:
	      case INPUT_DEFAULT_VALUE:
		value = lm_fix_newlines(mc, value);
		if (!value)
		    return MOCHA_FALSE;
		ok = (input_slot == INPUT_VALUE)
		     ? lm_SaveParamString(mc, &text->current_text, value)
		     : lm_SaveParamString(mc, &text->default_text, value);
		MOCHA_free(mc, (char *)value);
		if (!ok)
		    return MOCHA_FALSE;
		if (input_slot == INPUT_VALUE && context)
		    FE_ChangeInputElement(context, (LO_Element *)form_element);
		break;
	      default:
		/* Don't mess with option or user-defined property. */
		return MOCHA_TRUE;
	    }
	}
	break;

      case FORM_TYPE_SELECT_ONE:
      case FORM_TYPE_SELECT_MULT:
	{
	    lo_FormElementSelectData *selectData;
	    lo_FormElementOptionData *optionData;
	    MochaSelectOption *option;
	    int32 i, old_option_cnt;

	    selectData = &form_element->element_data->ele_select;
	    switch (slot) {
	      case INPUT_NAME:
		if (!lm_SaveParamString(mc, &selectData->name, value))
		    return MOCHA_FALSE;
		break;

	      case INPUT_LENGTH:
		old_option_cnt = selectData->option_cnt;
		selectData->option_cnt = (int32)dp->u.fval;
		if (!LO_ResizeSelectOptions(selectData)) {
		    selectData->option_cnt = old_option_cnt;
		    MOCHA_ReportOutOfMemory(mc);
		    return MOCHA_FALSE;
		}

		/* Remove truncated slots, or clear extended element data. */
		if (selectData->option_cnt < old_option_cnt) {
		    for (i = selectData->option_cnt; i < old_option_cnt; i++)
			MOCHA_RemoveSlot(mc, obj, (MochaSlot)i);
		} else if (selectData->option_cnt > old_option_cnt) {
		    PA_LOCK(optionData, lo_FormElementOptionData *,
			    selectData->options);
		    XP_BZERO(&optionData[old_option_cnt],
			     (selectData->option_cnt - old_option_cnt)
			     * sizeof *optionData);
		    PA_UNLOCK(selectData->options);
		}

		/* Tell the FE about it. */
		if (context)
		    FE_ChangeInputElement(context, (LO_Element *)form_element);
		break;

	      case INPUT_OPTIONS:
		break;

	      case INPUT_SELECTED_INDEX:
		PA_LOCK(optionData, lo_FormElementOptionData *,
			selectData->options);
		for (i = 0; i < selectData->option_cnt; i++)
		    optionData[i].selected = (i == (int32)dp->u.fval);
		PA_UNLOCK(selectData->options);

		/* Tell the FE about it. */
		if (context)
		    FE_ChangeInputElement(context, (LO_Element *)form_element);
		break;

	      default:
		if (slot < 0) {
		    /* Don't mess with a user-defined, named property. */
		    return MOCHA_TRUE;
		}

		/* The dp arg must refer to an object of the right class. */
		if (dp->tag != MOCHA_OBJECT &&
		    !MOCHA_ConvertDatum(mc, *dp, MOCHA_OBJECT, dp)) {
		    return MOCHA_FALSE;
		}

		if (!dp->u.obj) {
		    int32 count, limit;
		    MochaDatum datum;
		    MochaBoolean ok = MOCHA_TRUE;

		    if (slot >= selectData->option_cnt)
			return ok;

		    /* Clear the option and compress the options array. */
		    PA_LOCK(optionData, lo_FormElementOptionData *,
			    selectData->options);
		    count = selectData->option_cnt - (slot + 1);
		    if (count > 0) {
			/* Copy down option data above slot. */
			XP_MEMMOVE(&optionData[slot], &optionData[slot+1],
				   count * sizeof *optionData);

			/* And do the same for Mocha object reflections. */
			for (limit = slot + count; slot < limit; slot++) {
			    ok = MOCHA_GetSlot(mc, obj, slot + 1, &datum);
			    if (!ok)
				break;
			    MOCHA_SetSlot(mc, obj, slot, datum);

			    /* Fix each option's index-in-select property. */
			    XP_ASSERT(datum.tag == MOCHA_OBJECT);
			    option = datum.u.obj->data;
			    option->index = slot;
			}
			if (ok)
			    MOCHA_RemoveSlot(mc, obj, slot);
		    }
		    PA_UNLOCK(selectData->options);

		    /* Shrink the select element data's options array. */
		    if (ok) {
			selectData->option_cnt--;
			ok = LO_ResizeSelectOptions(selectData);
			if (!ok) {
			    MOCHA_ReportOutOfMemory(mc);
			} else if (context) {
			    FE_ChangeInputElement(context,
						  (LO_Element *)form_element);
			}
		    }
		    return ok;
		}

		if (dp->u.obj->clazz != &option_class) {
		    MOCHA_ReportError(mc, "cannot set %s.%s to incompatible %s",
				      obj->clazz->name, lm_options_str,
				      dp->u.obj->clazz->name);
		    return MOCHA_FALSE;
		}
		option = dp->u.obj->data;
		XP_ASSERT(option->data ||
			  option->index < (uint32)selectData->option_cnt);

		/* Grow the option array if necessary. */
		old_option_cnt = selectData->option_cnt;
		if (slot >= old_option_cnt) {
		    selectData->option_cnt = slot + 1;
		    if (!LO_ResizeSelectOptions(selectData)) {
			selectData->option_cnt = old_option_cnt;
			MOCHA_ReportOutOfMemory(mc);
			return MOCHA_FALSE;
		    }
		}

		/* Clear any option structs in the gap, then set slot. */
		PA_LOCK(optionData, lo_FormElementOptionData *,
			selectData->options);
		if (slot > old_option_cnt) {
		    XP_BZERO(&optionData[old_option_cnt],
			     (slot - old_option_cnt) * sizeof *optionData);
		}
		if (option->data) {
		    XP_MEMCPY(&optionData[slot], option->data,
			      sizeof(lo_FormElementOptionData));
		} else if ((uint32)slot != option->index) {
		    XP_MEMCPY(&optionData[slot], &optionData[option->index],
			      sizeof(lo_FormElementOptionData));
		}
		PA_UNLOCK(selectData->options);

		/* Update the option to point at its form and form element. */
		dp->u.obj->parent = obj;
		option->index = (uint32)slot;
		option->indexInForm = form_element->element_index;
		if (option->data) {
		    MOCHA_free(mc, option->data);
		    option->data = 0;
		}

		/* Tell the FE about it. */
		if (context)
		    FE_ChangeInputElement(context, (LO_Element *)form_element);
		break;
	    }
	}
	break;

      case FORM_TYPE_RADIO:
      case FORM_TYPE_CHECKBOX:
	{
	    lo_FormElementToggleData *toggle;

	    toggle = &form_element->element_data->ele_toggle;
	    switch (input_slot) {
	      case INPUT_NAME:
		if (!lm_SaveParamString(mc, &toggle->name, value))
		    return MOCHA_FALSE;
		break;
	      case INPUT_VALUE:
		if (!lm_SaveParamString(mc, &toggle->value, value))
		    return MOCHA_FALSE;
		break;
	      case INPUT_STATUS:
		if (dp->tag == MOCHA_BOOLEAN)
		    toggle->toggled = dp->u.bval;

		/* Tell the FE about it (the FE keeps radio-sets consistent). */
		if (context)
		    FE_ChangeInputElement(context, (LO_Element *)form_element);
		break;
	      case INPUT_DEFAULT_STATUS:
		if (dp->tag == MOCHA_BOOLEAN)
		    toggle->default_toggle = dp->u.bval;
		break;
	      default:
		/* Don't mess with a user-defined property. */
		return MOCHA_TRUE;
	    }
	}
	break;

      case FORM_TYPE_READONLY:
      case FORM_TYPE_FILE:
	/* Don't allow modification of readonly and file fields. */
	break;

      default:
	{
	    lo_FormElementMinimalData *minimal;

	    minimal = &form_element->element_data->ele_minimal;
	    switch (input_slot) {
	      case INPUT_NAME:
		if (!lm_SaveParamString(mc, &minimal->name, value))
		    return MOCHA_FALSE;
		break;
	      case INPUT_VALUE:
		if (!lm_SaveParamString(mc, &minimal->value, value))
		    return MOCHA_FALSE;
		if (context)
		    FE_ChangeInputElement(context, (LO_Element *)form_element);
		break;
	      default:
		/* Don't mess with a user-defined property. */
		return MOCHA_TRUE;
	    }
	}
	break;
    }

    return input_get_property(mc, obj, slot, dp);
}

static void
input_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaInput *input = obj->data;
    LO_FormElementStruct *form_element;

    if (!input) return;
    form_element = lm_GetFormElementByIndex(obj->parent, input->index);
    if (form_element)
	form_element->mocha_object = 0;
    LM_UNLINK_OBJECT(input->input_decoder, obj);
    MOCHA_free(mc, input);
}

static MochaClass input_class = {
    "Input",
    input_get_property, input_set_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, input_finalize
};

static MochaBoolean
Input(MochaContext *mc, MochaObject *obj,
      uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
input_to_string(MochaContext *mc, MochaObject *obj,
		uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaInput *input;
    LO_FormElementStruct *form_element;
    uint type;
    char *typename, *string, *value;
    size_t length;
    long truelong;
    MochaDatum result;
    uint16 taint;
    MochaAtom *atom;

    if (!MOCHA_InstanceOf(mc, obj, &input_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    input = obj->data;
    form_element = lm_GetFormElementByIndex(obj->parent, input->index);
    if (!form_element) {
	*rval = MOCHA_empty;
	return MOCHA_TRUE;
    }

    type = form_element->element_data->type;
    if (type >= sizeof typenames / sizeof typenames[0]) {
	MOCHA_ReportError(mc, "unknown form element type %u", type);
	return MOCHA_FALSE;
    }
    typename = typenames[type];
    string = PR_sprintf_append(0, "<");
    switch (type) {
      case FORM_TYPE_TEXT:
	{
	    lo_FormElementTextData *text;

	    text = &form_element->element_data->ele_text;
	    string = PR_sprintf_append(string, "%s %s=\"%s\"",
				       PT_INPUT, PARAM_TYPE, typename);
	    if (text->name) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_NAME, (char *)text->name);
	    }
	    if (text->default_text) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_VALUE,
					   (char *)text->default_text);
	    }
	    if (text->size) {
		truelong = text->size;
		string = PR_sprintf_append(string, " %s=%ld\"",
					   PARAM_SIZE, truelong);
	    }
	    if (text->max_size) {
		truelong = text->max_size;
		string = PR_sprintf_append(string, " %s=%ld\"",
					   PARAM_MAXLENGTH, truelong);
	    }
	}
	break;

      case FORM_TYPE_TEXTAREA:	/* XXX we ASSUME common struct prefixes */
	{
	    lo_FormElementTextareaData *textarea;

	    textarea = &form_element->element_data->ele_textarea;
	    string = PR_sprintf_append(string, PT_TEXTAREA);
	    if (textarea->name) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_NAME, (char *)textarea->name);
	    }
	    if (textarea->default_text) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_VALUE,
					   (char *)textarea->default_text);
	    }
	    if (textarea->rows) {
		truelong = textarea->rows;
		string = PR_sprintf_append(string, " %s=%ld\"",
					   PARAM_SIZE, truelong);
	    }
	    if (textarea->cols) {
		truelong = textarea->cols;
		string = PR_sprintf_append(string, " %s=%ld\"",
					   PARAM_SIZE, truelong);
	    }
	    if (textarea->auto_wrap) {
		switch (textarea->auto_wrap) {
		  case TEXTAREA_WRAP_OFF:
		    value = "off";
		    break;
		  case TEXTAREA_WRAP_HARD:
		    value = "hard";
		    break;
		  case TEXTAREA_WRAP_SOFT:
		    value = "soft";
		    break;
		  default:
		    value = "unknown";
		    break;
		}
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_WRAP, value);
	    }
	}
	break;

      case FORM_TYPE_SELECT_ONE:
      case FORM_TYPE_SELECT_MULT:
	{
	    lo_FormElementSelectData *selectData;
	    lo_FormElementOptionData *optionData;
	    int32 i;

	    selectData = &form_element->element_data->ele_select;
	    string = PR_sprintf_append(string, PT_SELECT);
	    if (selectData->name) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_NAME,
					   (char *)selectData->name);
	    }
	    if (selectData->size) {
		truelong = selectData->size;
		string = PR_sprintf_append(string, " %s=%ld\"",
					   PARAM_SIZE, truelong);
	    }
	    if (selectData->multiple) {
		string = PR_sprintf_append(string, " %s", PARAM_MULTIPLE);
	    }

	    string = PR_sprintf_append(string, ">\n");
	    PA_LOCK(optionData, lo_FormElementOptionData *,
		    selectData->options);
	    for (i = 0; i < selectData->option_cnt; i++) {
		string = PR_sprintf_append(string, "<%s", PT_OPTION);
		if (optionData[i].value) {
		    string = PR_sprintf_append(string, " %s=\"%s\"",
					       PARAM_VALUE,
					       optionData[i].value);
		}
		if (optionData[i].def_selected)
		    string = PR_sprintf_append(string, " %s", PARAM_SELECTED);
		string = PR_sprintf_append(string, ">");
		if (optionData[i].text_value) {
		    string = PR_sprintf_append(string, "%s",
					       optionData[i].text_value);
		}
		string = PR_sprintf_append(string, "\n");
	    }
	    PA_UNLOCK(selectData->options);

	    string = PR_sprintf_append(string, "</%s", PT_SELECT);
	}
	break;

      case FORM_TYPE_RADIO:
      case FORM_TYPE_CHECKBOX:
	{
	    lo_FormElementToggleData *toggle;

	    toggle = &form_element->element_data->ele_toggle;
	    string = PR_sprintf_append(string, "%s %s=\"%s\"",
				       PT_INPUT, PARAM_TYPE, typename);
	    if (toggle->name) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_NAME, (char *)toggle->name);
	    }
	    if (toggle->value) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_VALUE, (char *)toggle->value);
	    }
	    if (toggle->default_toggle)
		string = PR_sprintf_append(string, " %s", PARAM_CHECKED);
	}
	break;

      default:
	{
	    lo_FormElementMinimalData *minimal;

	    minimal = &form_element->element_data->ele_minimal;
	    string = PR_sprintf_append(string, "%s %s=\"%s\"",
				       PT_INPUT, PARAM_TYPE, typename);
	    if (minimal->name) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_NAME, (char *)minimal->name);
	    }
	    if (minimal->value) {
		string = PR_sprintf_append(string, " %s=\"%s\"",
					   PARAM_VALUE, (char *)minimal->value);
	    }
	}
	break;
    }

#define FROB(param) {                                                         \
    if (!MOCHA_LookupName(mc, input->input_object, param, &result)) {         \
	PR_FREEIF(string);                                                    \
	return MOCHA_FALSE;                                                   \
    }                                                                         \
    if (result.tag == MOCHA_FUNCTION) {                                       \
	if (!MOCHA_DecompileFunctionBody(mc, result.u.fun, 0, &value)) {      \
	    PR_FREEIF(string);                                                \
	    return MOCHA_FALSE;                                               \
	}                                                                     \
	length = strlen(value);                                               \
	if (length && value[length-1] == '\n') length--;                      \
	string = PR_sprintf_append(string," %s='%.*s'", param, length, value);\
	MOCHA_free(mc, value);                                                \
    }                                                                         \
}

    FROB(lm_onFocus_str);
    FROB(lm_onBlur_str);
    FROB(lm_onSelect_str);
    FROB(lm_onChange_str);
    FROB(lm_onClick_str);
    FROB(lm_onScroll_str);
#undef FROB

    string = PR_sprintf_append(string, ">");
    if (!string) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }
    if (!lm_GetTaint(mc, input->input_decoder, &taint)) {
	XP_FREE(string);
	return MOCHA_FALSE;
    }
    atom = MOCHA_Atomize(mc, string);
    XP_FREE(string);
    if (!atom) return MOCHA_FALSE;
    MOCHA_INIT_FULL_DATUM(mc, rval, MOCHA_STRING, MDF_TAINTED, taint,
			  u.atom, atom);
    return MOCHA_TRUE;
}

static MochaBoolean
input_method(MochaContext *mc, MochaObject *obj, MochaDatum *argv,
	     uint32 event, void (*fefun)(MWContext *, LO_Element *))
{
    MochaInput *input;
    MWContext *context;
    LO_FormElementStruct *form_element;

    if (!MOCHA_InstanceOf(mc, obj, &input_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    input = obj->data;
    context = input->input_decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    form_element = lm_GetFormElementByIndex(obj->parent, input->index);
    if (!form_element)
	return MOCHA_TRUE;
    input->input_event_mask |= event;
    (*fefun)(context, (LO_Element *)form_element);
    input->input_event_mask &= ~event;
    return MOCHA_TRUE;
}

static MochaBoolean
input_focus(MochaContext *mc, MochaObject *obj,
	    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return input_method(mc, obj, argv, EVENT_FOCUS, FE_FocusInputElement);
}

static MochaBoolean
input_blur(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return input_method(mc, obj, argv, EVENT_BLUR, FE_BlurInputElement);
}

static MochaBoolean
input_select(MochaContext *mc, MochaObject *obj,
	     uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return input_method(mc, obj, argv, EVENT_SELECT, FE_SelectInputElement);
}

static MochaBoolean
input_click(MochaContext *mc, MochaObject *obj,
	     uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return input_method(mc, obj, argv, EVENT_CLICK, FE_ClickInputElement);
}

#ifdef NOTYET
static MochaBoolean
input_enable(MochaContext *mc, MochaObject *obj,
	     uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return input_method(mc, obj, argv, 0, FE_EnableInputElement);
}

static MochaBoolean
input_disable(MochaContext *mc, MochaObject *obj,
	      uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return input_method(mc, obj, argv, 0, FE_DisableInputElement);
}
#endif /* NOTYET */

static MochaFunctionSpec input_methods[] = {
    {lm_toString_str,   input_to_string,        0},
    {"focus",           input_focus,            0},
    {"blur",            input_blur,             0},
    {"select",          input_select,           0},
    {"click",           input_click,            0},
#ifdef NOTYET
    {"enable",          input_enable,           0},
    {"disable",         input_disable,          0},
#endif /* NOTYET */
    {0}
};

/*
** XXX move me somewhere else...
*/
enum input_array_slot {
    INPUT_ARRAY_LENGTH = -1
};

static MochaPropertySpec input_array_props[] = {
    {lm_length_str,     INPUT_ARRAY_LENGTH,     MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaInputArray {
    MochaInputBase      base;
    uint                length;
} MochaInputArray;

static MochaBoolean
input_array_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
			 MochaDatum *dp)
{
    MochaInputArray *array = obj->data;

    switch (slot) {
      case INPUT_ARRAY_LENGTH:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, array->length);
	break;
    }
    return MOCHA_TRUE;
}

static void
input_array_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaInputArray *array = obj->data;

    LM_UNLINK_OBJECT(array->base_decoder, obj);
    MOCHA_free(mc, array);
}

static MochaClass input_array_class = {
    "InputArray",
    input_array_get_property, input_array_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, input_array_finalize
};

#ifdef NOTYET
static MochaBoolean
InputArray(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}
#endif

#define ANTI_RECURSIVE_KLUDGE	((MochaObject *)1)

/*
** Reflect a bunch of different types of form elements into Mocha.
*/
MochaObject *
LM_ReflectFormElement(MWContext *context, LO_FormElementStruct *form_element)
{
    MochaObject *obj, *form_obj, *prototype, *old_obj, *array_obj;
    LO_FormElementData *data;
    MochaDecoder *decoder;
    MochaContext *mc;
    int32 type;
    char *name;
    MochaBoolean ok;
    size_t size;
    MochaInput *input;
    MochaInputBase *base;
    MochaInputArray *array;
    MochaDatum datum;
    static uint recurring;	/* XXX thread-unsafe */

    obj = form_element->mocha_object;
    if (obj) {
	if (obj == ANTI_RECURSIVE_KLUDGE)
	    return 0;
	return obj;
    }

    data = form_element->element_data;
    if (!data)
	return 0;
    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;
    form_obj = lm_GetFormObjectByID(decoder, (uint)form_element->form_id);
    if (!form_obj) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }

    mc = decoder->mocha_context;
    prototype = decoder->input_prototype;

    type = data->type;
    name = (char *)data->ele_minimal.name;
    switch (type) {
      case FORM_TYPE_TEXT:
      case FORM_TYPE_TEXTAREA:
	size = sizeof(MochaTextInput);
	break;

      case FORM_TYPE_RADIO:
	if (!recurring) {
	    recurring++;
	    ok = lm_ReflectRadioButtonArray(context, form_element->form_id,
					    name);
	    recurring--;
	    obj = form_element->mocha_object;
	    if (obj) {
		LM_PutMochaDecoder(decoder);
		return obj;
	    }
	}
	/* FALL THROUGH */

      default:
	size = sizeof(MochaInput);
	break;
    }

    input = MOCHA_malloc(mc, size);
    if (!input) {
	LM_PutMochaDecoder(decoder);
	return 0;
    }
    XP_BZERO(input, size);
    input->input_decoder = decoder;
    input->input_type = type;

    array_obj = 0;
    if (!name) {
	obj = MOCHA_NewObject(mc, &input_class, input, prototype, form_obj,
			      0, 0);
	LM_LINK_OBJECT(decoder, obj, "anInput");
    } else {
	form_element->mocha_object = ANTI_RECURSIVE_KLUDGE;
	ok = MOCHA_LookupName(mc, form_obj, name, &datum);
	form_element->mocha_object = 0;
	if (!ok) {
	    LM_PutMochaDecoder(decoder);
	    return 0;
	}
	old_obj = (datum.tag == MOCHA_OBJECT) ? datum.u.obj : 0;

	if (old_obj) {
	    datum.flags |= MDF_ENUMERATE;
	    base = old_obj->data;
	    if ((base->type & FORM_TYPE_ARRAY) == 0) {
		/* Make an array out of the previous element and this one. */
		array = MOCHA_malloc(mc, sizeof *array);
		if (!array) {
		    MOCHA_free(mc, input);
		    LM_PutMochaDecoder(decoder);
		    return 0;
		}
		XP_BZERO(array, sizeof *array);
		array->base_decoder = decoder;
		array->base_type = base->type | FORM_TYPE_ARRAY;

		/*
		** Hold old_obj temporarily until we remove it from form_obj
		** and add it as a property of the radio button array.
		*/
		old_obj = MOCHA_HoldObject(mc, old_obj);
		MOCHA_UndefineName(mc, form_obj, name);

/* XXX use MOCHA_InitClass instead of this! */
		array_obj = MOCHA_DefineNewObject(mc, form_obj, name,
						  &input_array_class, array,
						  0/*XXXproto*/, MDF_ENUMERATE,
						  input_array_props, 0);
		if (!array_obj) {
		    MOCHA_DropObject(mc, old_obj);
		    MOCHA_free(mc, array);
		    MOCHA_free(mc, input);
		    LM_PutMochaDecoder(decoder);
		    return 0;
		}

		/* Insert old_obj (referred to by datum) into the array. */
		LM_LINK_OBJECT(decoder, array_obj, "anInputArray");
		MOCHA_SetProperty(mc, array_obj, 0, array->length, datum);
		array->length++;

		/* Finally, we can drop old_obj because it's in array. */
		MOCHA_DropObject(mc, old_obj);
	    } else {
		array_obj = old_obj;
		array = (MochaInputArray *)base;
	    }

	    /* Clear name so we don't rebind it in lm_AddFormElement(). */
	    name = 0;

	    /* Finally, create obj and put it in the last slot in array. */
	    obj = MOCHA_NewObject(mc, &input_class, input, prototype, form_obj,
				  0, 0);
	    if (!obj) {
		MOCHA_free(mc, input);
		LM_PutMochaDecoder(decoder);
		return 0;
	    }
	    LM_LINK_OBJECT(decoder, obj, name);
	    datum.u.obj = obj;
	    MOCHA_SetProperty(mc, array_obj, 0, array->length, datum);
	    array->length++;
	} else {
	    obj = MOCHA_NewObject(mc, &input_class, input, prototype, form_obj,
				  0, 0);
	    LM_LINK_OBJECT(decoder, obj, name);
	}
    }

    if (!obj) {
	MOCHA_free(mc, input);
	LM_PutMochaDecoder(decoder);
	return 0;
    }

    input->input_object = obj;
    input->index = form_element->element_index;
    form_element->mocha_object = obj;

    if (!lm_AddFormElement(mc, form_obj, obj, name, input->index)) {
	/* XXX undefine name if it's non-null? */
    }
    LM_PutMochaDecoder(decoder);
    return obj;
}

MochaBoolean
lm_InitInputClasses(MochaDecoder *decoder)
{
    MochaContext *mc;
    MochaObject *prototype;

    mc = decoder->mocha_context;
    prototype = MOCHA_InitClass(mc, decoder->window_object, &input_class,
				0, Input, 0, input_props, input_methods, 0, 0);
    if (!prototype)
	return MOCHA_FALSE;
    decoder->input_prototype = MOCHA_HoldObject(mc, prototype);

    prototype = MOCHA_InitClass(mc, decoder->window_object, &option_class,
				0, Option, 0, option_props, 0, 0, 0);
    if (!prototype)
	return MOCHA_FALSE;
    decoder->option_prototype = MOCHA_HoldObject(mc, prototype);
    return MOCHA_TRUE;
}

uint16
lm_GetFormElementTaint(MochaContext *mc, LO_FormElementStruct *form_element)
{
    uint16 taint;
    MochaObject *obj, *option_obj;
    LO_FormElementData *data;
    MochaPropertySpec *ps;
    MochaDatum datum;
    MochaSlot length, slot;

    taint = MOCHA_TAINT_IDENTITY;
    obj = form_element->mocha_object;
    data = form_element->element_data;
    if (!obj || !data)
	return taint;
    for (ps = input_props; ps->name; ps++) {
	if ((ps->flags & MDF_TAINTED) &&
	    MOCHA_LookupName(mc, obj, ps->name, &datum)) {
	    MOCHA_MIX_TAINT(mc, taint, datum.taint);
	}
    }
    switch (data->type) {
      case FORM_TYPE_SELECT_ONE:
      case FORM_TYPE_SELECT_MULT:
	length = data->ele_select.option_cnt;
	for (slot = 0; slot < length; slot++) {
	    if (!MOCHA_GetSlot(mc, obj, slot, &datum))
		continue;
	    if (datum.tag != MOCHA_OBJECT || !(option_obj = datum.u.obj))
		continue;
	    for (ps = option_props; ps->name; ps++) {
		if ((ps->flags & MDF_TAINTED) &&
		    MOCHA_LookupName(mc, option_obj, ps->name, &datum)) {
		    MOCHA_MIX_TAINT(mc, taint, datum.taint);
		}
	    }
	}
	break;
      default:;
    }
    return taint;
}

void
lm_ResetFormElementTaint(MochaContext *mc, LO_FormElementStruct *form_element)
{
    MochaObject *obj, *option_obj;
    LO_FormElementData *data;
    MochaPropertySpec *ps;
    MochaSlot length, slot;
    MochaDatum datum;

    obj = form_element->mocha_object;
    data = form_element->element_data;
    if (!obj || !data)
	return;
    for (ps = input_props; ps->name; ps++) {
	if (ps->flags & MDF_TAINTED)
	    MOCHA_SetPropertyTaint(mc, obj, ps->name, MOCHA_TAINT_IDENTITY);
    }
    switch (data->type) {
      case FORM_TYPE_SELECT_ONE:
      case FORM_TYPE_SELECT_MULT:
	length = data->ele_select.option_cnt;
	for (slot = 0; slot < length; slot++) {
	    if (!MOCHA_GetSlot(mc, obj, slot, &datum))
		continue;
	    if (datum.tag != MOCHA_OBJECT || !(option_obj = datum.u.obj))
		continue;
	    for (ps = option_props; ps->name; ps++) {
		if (ps->flags & MDF_TAINTED) {
		    MOCHA_SetPropertyTaint(mc, option_obj, ps->name,
					   MOCHA_TAINT_IDENTITY);
		}
	    }
	}
	break;
      default:;
    }
}

static MochaBoolean
input_event(MWContext *context, LO_Element *element, uint32 event, char *method,
	    MochaDatum *rval)
{
    MochaContext *mc;
    MochaDecoder *decoder;
    MochaBoolean ok;
    LO_AnchorData *anchor;
    MochaObject *obj;
    MochaInputHandler *handler;

    *rval = MOCHA_void;
    mc = context->mocha_context;
    if (!mc)
	return MOCHA_TRUE;

    if (!element) {
	decoder = LM_GetMochaDecoder(context);
	if (!decoder)
	    return MOCHA_FALSE;
	if (decoder->event_mask & event) {
	    ok = MOCHA_TRUE;
	} else {
	    decoder->event_mask |= event;
	    ok = lm_SendEvent(context, decoder->window_object, method, rval);
	    decoder->event_mask &= ~event;
	}
	LM_PutMochaDecoder(decoder);
	return ok;
    }

    switch (element->type) {
      case LO_TEXT:
	anchor = element->lo_text.anchor_href;
	if (!anchor)
	    return MOCHA_FALSE;
	obj = anchor->mocha_object;
	break;
      case LO_IMAGE:
	anchor = element->lo_image.anchor_href;
	if (!anchor)
	    return MOCHA_FALSE;
	obj = anchor->mocha_object;
	break;
      case LO_FORM_ELE:
	obj = element->lo_form.mocha_object;
	break;
      default:
	return MOCHA_FALSE;
    }
    if (!obj)
	return MOCHA_FALSE;

    handler = obj->data;

    if (!handler)
      return MOCHA_FALSE;	/* jwz -- guessing... */

    if (handler->event_mask & event)
	return MOCHA_FALSE;
    MOCHA_HoldObject(mc, obj);
    handler->event_mask |= event;
    ok = lm_SendEvent(context, obj, method, rval);
    handler->event_mask &= ~event;
    MOCHA_DropObject(mc, obj);
    return ok;
}

void
LM_SendOnFocus(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    (void) input_event(context, element, EVENT_FOCUS, lm_onFocus_str, &rval);
}

void
LM_SendOnBlur(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    (void) input_event(context, element, EVENT_BLUR, lm_onBlur_str, &rval);
}

void
LM_SendOnSelect(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    (void) input_event(context, element, EVENT_SELECT, lm_onSelect_str, &rval);
}

void
LM_SendOnChange(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    (void) input_event(context, element, EVENT_CHANGE, lm_onChange_str, &rval);
}

MochaBoolean
LM_SendOnClick(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    if (input_event(context, element, EVENT_CLICK, lm_onClick_str, &rval) &&
	rval.tag == MOCHA_BOOLEAN && rval.u.bval == MOCHA_FALSE) {
	/* User wants to cancel this click gesture. */
	return MOCHA_FALSE;
    }
    return MOCHA_TRUE;
}

MochaBoolean
LM_SendOnClickToAnchor(MWContext *context, LO_AnchorData *anchor)
{
    LO_Element element;

    element.lo_text.type = LO_TEXT;
    element.lo_text.anchor_href = anchor;
    return LM_SendOnClick(context, &element);
}

MochaBoolean
LM_SendMouseOver(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    if (input_event(context, element, EVENT_MOUSEOVER, lm_onMouseOver_str,
		    &rval) &&
	rval.tag == MOCHA_BOOLEAN && rval.u.bval == MOCHA_TRUE) {
	/* User wants to manage the status bar. */
	return MOCHA_TRUE;
    }
    return MOCHA_FALSE;
}

MochaBoolean
LM_SendMouseOverAnchor(MWContext *context, LO_AnchorData *anchor)
{
    LO_Element element;

    element.lo_text.type = LO_TEXT;
    element.lo_text.anchor_href = anchor;
    return LM_SendMouseOver(context, &element);
}

void
LM_SendMouseOut(MWContext *context, LO_Element *element)
{
    MochaDatum rval;

    (void) input_event(context, element, EVENT_MOUSEOUT, lm_onMouseOut_str,
		       &rval);
}

void
LM_SendMouseOutOfAnchor(MWContext *context, LO_AnchorData *anchor)
{
    LO_Element element;

    element.lo_text.type = LO_TEXT;
    element.lo_text.anchor_href = anchor;
    LM_SendMouseOut(context, &element);
}
