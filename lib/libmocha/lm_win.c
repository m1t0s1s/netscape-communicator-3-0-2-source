
/*
** Mocha reflection of the current Navigator Window.
**
** Brendan Eich, 9/8/95
*/
#include "lm.h"
#include "xp.h"
#include "fe_proto.h"
#include "structs.h"
#include "layout.h"		/* included via -I../layout */
#include "prprf.h"
#include "prtime.h"
#include "libi18n.h"

#undef FREE_AND_CLEAR		/* XXX over-including Mac compiled headers */

enum window_slot {
    WIN_LENGTH          = -1,
    WIN_FRAMES          = -2,
    WIN_PARENT          = -3,
    WIN_TOP             = -4,
    WIN_SELF            = -5,
    WIN_NAME            = -6,
    WIN_STATUS          = -7,
    WIN_DEFAULT_STATUS  = -8,
    WIN_OPENER          = -9,
    WIN_CLOSED          = -10
};

static MochaPropertySpec window_props[] = {
    {"length",        WIN_LENGTH,     MDF_ENUMERATE|MDF_READONLY},
    {"frames",        WIN_FRAMES,     MDF_BACKEDGE|MDF_ENUMERATE|MDF_READONLY},
    {"parent",        WIN_PARENT,     MDF_BACKEDGE|MDF_ENUMERATE|MDF_READONLY},
    {"top",           WIN_TOP,        MDF_BACKEDGE|MDF_ENUMERATE|MDF_READONLY},
    {"self",          WIN_SELF,       MDF_BACKEDGE|MDF_ENUMERATE|MDF_READONLY},
    {"window",        WIN_SELF,       MDF_READONLY},
    {"name",          WIN_NAME,       MDF_ENUMERATE},
    {"status",        WIN_STATUS,     MDF_ENUMERATE|MDF_TAINTED},
    {"defaultStatus", WIN_DEFAULT_STATUS, MDF_ENUMERATE|MDF_TAINTED},
    {lm_opener_str,   WIN_OPENER,     MDF_ENUMERATE},
    {lm_closed_str,   WIN_CLOSED,     MDF_ENUMERATE|MDF_READONLY},
    {0}
};

static MochaBoolean
win_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaDecoder *decoder;
    MWContext *context;
    enum window_slot window_slot;
    uint count;

    decoder = obj->data;
    context = decoder->window_context;
    if (!context) {
	if (slot == WIN_CLOSED)
	    MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, u.bval, MOCHA_TRUE);
	return MOCHA_TRUE;
    }

    if (mc != decoder->mocha_context &&
	!lm_MergeTaint(mc, decoder->mocha_context)) {
	return MOCHA_FALSE;
    }

    window_slot = slot;
    switch (window_slot) {
      case WIN_LENGTH:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval,
			 XP_ListCount(context->grid_children));
	break;

      case WIN_FRAMES:
      case WIN_SELF:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, decoder->window_object);
	break;

      case WIN_PARENT:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, decoder->window_object);
	if (context->grid_parent) {
	    decoder = LM_GetMochaDecoder(context->grid_parent);
	    if (decoder) {
		dp->u.obj = decoder->window_object;
		MOCHA_WeakenRef(mc, dp);
	    }
	}
	break;

      case WIN_TOP:
	while (context->grid_parent)
	    context = context->grid_parent;
	decoder = LM_GetMochaDecoder(context);
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
			 decoder ? decoder->window_object : 0);
	if (decoder)
	    MOCHA_WeakenRef(mc, dp);
	break;

      case WIN_NAME:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom,
			 MOCHA_Atomize(mc, context->name));
	break;

      case WIN_STATUS:
	return MOCHA_TRUE;	/* XXX can't get yet, return last known */

      case WIN_DEFAULT_STATUS:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom,
			 MOCHA_Atomize(mc, context->defaultStatus));
	break;

      case WIN_CLOSED:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_BOOLEAN, u.bval, MOCHA_FALSE);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}

	/* XXX silly xp_cntxt.c puts newer contexts at the front! fix. */
	count = XP_ListCount(context->grid_children);
	context = XP_ListGetObjectNum(context->grid_children, count - slot);
	if (context) {
	    decoder = LM_GetMochaDecoder(context);
	    if (decoder) {
		MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj,
				 decoder->window_object);
		LM_PutMochaDecoder(decoder);
	    } else {
		*dp = MOCHA_null;
	    }
	}
	break;
    }
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	return lm_GetTaint(mc, decoder, &dp->taint);
    return MOCHA_TRUE;
}

static MochaBoolean
win_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaDecoder *decoder, *parent_decoder;
    MWContext *context;
    enum window_slot window_slot;
    const char *atom_name;
    char *str, *name, *old_name;

    decoder = obj->data;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    if (mc != decoder->mocha_context &&
	!lm_MergeTaint(mc, decoder->mocha_context)) {
	return MOCHA_FALSE;
    }

    window_slot = slot;
    switch (window_slot) {
      case WIN_NAME:
      case WIN_STATUS:
      case WIN_DEFAULT_STATUS:
	if (dp->tag != MOCHA_STRING &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	    return MOCHA_FALSE;
	}
	break;
      case WIN_OPENER:
	if (dp->tag != MOCHA_OBJECT &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_OBJECT, dp)) {
	    return MOCHA_FALSE;
	}
	break;
      default:;
    }

    switch (window_slot) {
      case WIN_NAME:
	/* Don't let rogue JS name a mail or news window and then close it. */
	if (context->type != MWContextBrowser)
	    return MOCHA_TRUE;
	atom_name = MOCHA_GetAtomName(mc, dp->u.atom);
	if (!lm_CheckWindowName(mc, atom_name))
	    return MOCHA_FALSE;
	name = MOCHA_strdup(mc, atom_name);
	if (!name)
	    return MOCHA_FALSE;
	old_name = context->name;
	if (old_name) {
	    /* If context is a frame, change its name in its parent's scope. */
	    if (context->grid_parent) {
		parent_decoder = LM_GetMochaDecoder(context->grid_parent);
		if (parent_decoder) {
		    MOCHA_RemoveProperty(mc, parent_decoder->window_object,
					 old_name);
		    LM_PutMochaDecoder(parent_decoder);
		}
	    }
	    XP_FREE(old_name);
	}
	context->name = name;
	break;

      case WIN_STATUS:
	FE_Progress(context, MOCHA_GetAtomName(mc, dp->u.atom));
	break;

      case WIN_DEFAULT_STATUS:
	str = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, dp->u.atom));
	if (!str)
	    return MOCHA_FALSE;
	if (context->defaultStatus)
	    XP_FREE(context->defaultStatus);
	context->defaultStatus = str;
	FE_Progress(context, 0);
	break;

      case WIN_OPENER:
	if (decoder->opener && !dp->u.obj) {
	    MOCHA_DropObject(mc, decoder->opener);
	    decoder->opener = 0;
	}
	break;

      default:;
    }
    return MOCHA_TRUE;
}

static MochaBoolean
win_list_properties(MochaContext *mc, MochaObject *obj)
{
    MochaDecoder *decoder;
    MWContext *context, *kid;
    XP_List *list;
    uint slot;
    MochaDatum d;

    decoder = obj->data;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;

    /* XXX silly xp_cntxt.c puts newer contexts at the front! fix. */
    list = context->grid_children;
    slot = XP_ListCount(list);
    while ((kid = XP_ListNextObject(list)) != 0) {
	slot--;
	d = MOCHA_null;
	d.flags |= MDF_ENUMERATE;
	if (!MOCHA_SetProperty(mc, decoder->window_object, kid->name, slot, d))
	    return MOCHA_FALSE;
	break;
    }
    return MOCHA_TRUE;
}

static MochaBoolean
win_resolve_name(MochaContext *mc, MochaObject *obj, const char *name)
{
    MochaDecoder *decoder;
    MWContext *context, *kid;
    XP_List *list;
    uint slot;
    MochaDatum d;

    decoder = obj->data;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;

    /* XXX silly xp_cntxt.c puts newer contexts at the front! fix. */
    list = context->grid_children;
    slot = XP_ListCount(list);
    while ((kid = XP_ListNextObject(list)) != 0) {
	slot--;
	if (kid->name && XP_STRCMP(kid->name, name) == 0) {
	    d = MOCHA_null;
	    d.flags |= MDF_ENUMERATE;
	    if (!MOCHA_SetProperty(mc, decoder->window_object, name, slot, d))
		return MOCHA_FALSE;
	    break;
	}
    }
    return MOCHA_TRUE;
}

static void
win_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaDecoder *decoder = obj->data;

    LM_UNLINK_OBJECT(decoder, obj);
    decoder->window_object = 0;
    lm_DestroyWindow(decoder);
}

MochaClass lm_window_class = {
    "Window",
    win_get_property, win_set_property, win_list_properties,
    win_resolve_name, MOCHA_ConvertStub, win_finalize
};

/*
** Alert and some simple dialogs.
*/
static MochaBoolean
win_alert(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaAtom *atom;
    char *message;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (!MOCHA_DatumToString(mc, argv[0], &atom))
        return MOCHA_FALSE;
    message = PR_smprintf("%s Alert:\n%s",
			  mocha_language_name, MOCHA_GetAtomName(mc, atom));
    MOCHA_DropAtom(mc, atom);
    if (message) {
	if (decoder->window_context)
	    FE_Alert(decoder->window_context, message);
	XP_FREE(message);
    }
    return MOCHA_TRUE;
}

static MochaBoolean
win_confirm(MochaContext *mc, MochaObject *obj,
	    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaAtom *atom;
    char *message;
    MochaBoolean ok;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (!MOCHA_DatumToString(mc, argv[0], &atom))
        return MOCHA_FALSE;
    message = PR_smprintf("%s Confirm:\n%s",
			  mocha_language_name, MOCHA_GetAtomName(mc, atom));
    MOCHA_DropAtom(mc, atom);
    if (!message) {
	*rval = MOCHA_false;
	return MOCHA_TRUE;
    }
    if (decoder->window_context)
	ok = FE_Confirm(decoder->window_context, message);
    else
	ok = MOCHA_FALSE;
    XP_FREE(message);
    MOCHA_INIT_DATUM(mc, rval, MOCHA_BOOLEAN, u.bval, ok);
    return MOCHA_TRUE;
}

static MochaBoolean
win_prompt(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaAtom *arg1, *arg2, *atom;
    char *message, *retval;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (!MOCHA_DatumToString(mc, argv[0], &arg1))
        return MOCHA_FALSE;
    message = PR_smprintf("%s Prompt:\n%s",
			  mocha_language_name, MOCHA_GetAtomName(mc, arg1));
    MOCHA_DropAtom(mc, arg1);
    if (!message) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }
    if (decoder->window_context) {
	if (!MOCHA_DatumToString(mc, argv[1], &arg2)) {
	    XP_FREE(message);
	    return MOCHA_FALSE;
	}
	retval = FE_Prompt(decoder->window_context, message,
			   MOCHA_GetAtomName(mc, arg2));
	MOCHA_DropAtom(mc, arg2);
    } else {
	retval = 0;
    }
    XP_FREE(message);
    if (!retval) {
	*rval = MOCHA_null;
	return MOCHA_TRUE;
    }
    atom = MOCHA_Atomize(mc, retval);
    XP_FREE(retval);
    if (!atom) return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, rval, MOCHA_STRING, u.atom, atom);
    return MOCHA_TRUE;
}

/*
** Open and close of a named window.
*/
MochaBoolean
lm_CheckWindowName(MochaContext *mc, const char *window_name)
{
    const char *cp;

    for (cp = window_name; *cp != '\0'; cp++) {
	if (!XP_IS_ALPHA(*cp) && !XP_IS_DIGIT(*cp) && *cp != '_') {
	    MOCHA_ReportError(mc,
		"illegal character '%c' ('\\%o') in window name %s",
		*cp, *cp, window_name);
	    return MOCHA_FALSE;
	}
    }
    return MOCHA_TRUE;
}

static int32
win_has_option(char *options, char *name)
{
    char *comma, *equal;
    int32 found = 0;

    for (;;) {
	comma = XP_STRCHR(options, ',');
	if (comma) *comma = '\0';
	equal = XP_STRCHR(options, '=');
	if (equal) *equal = '\0';
	if (XP_STRCASECMP(options, name) == 0) {
	    if (!equal || XP_STRCASECMP(equal + 1, "yes") == 0)
		found = 1;
	    else
		found = XP_ATOI(equal + 1);
	}
	if (equal) *equal = '=';
	if (comma) *comma = ',';
	if (found || !comma)
	    break;
	options = comma + 1;
    }
    return found;
}

static void
delete_window(void *closure)
{
    FE_DestroyWindow((MWContext *)closure);
}

/* NB: These apply only to top-level windows, not to frames. */
static uint lm_window_count = 0;
static uint lm_window_limit = 100;

/* static XXXsee doc_open */
MochaBoolean
win_open(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder, *new_decoder;
    URL_Struct *url_struct;
    uint16 url_taint;
    MochaAtom *atom, *window_name_atom;
    const char *url_string, *window_name;
    char *options;
    Chrome chrome;
    MWContext *old_context, *context;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;

    /* Make url_string absolute based on current document's base URL. */
    url_struct = 0;
    url_taint = MOCHA_TAINT_IDENTITY;
    if (argc > 0) {
	if (!MOCHA_DatumToString(mc, argv[0], &atom))
	    return MOCHA_FALSE;
	url_string = (char *) MOCHA_GetAtomName(mc, atom);
	if (*url_string != '\0') {
	    url_string = lm_CheckURL(mc, url_string);
	    if (url_string) {
		url_struct = NET_CreateURLStruct(url_string, NET_DONT_RELOAD);
		XP_FREE((char *)url_string);
		if (!url_struct) {
		    MOCHA_ReportOutOfMemory(mc);
		    MOCHA_DropAtom(mc, atom);
		    return MOCHA_FALSE;
		}
	    }
	}
	MOCHA_DropAtom(mc, atom);
	if (!url_string)
	    return MOCHA_FALSE;
	url_taint = argv[0].taint;
    }

    /* Set these to null so we can goto fail from here onward. */
    new_decoder = 0;
    window_name_atom = 0;

    /* Sanity-check the optional window_name argument. */
    if (argc > 1) {
	if (!MOCHA_DatumToString(mc, argv[1], &window_name_atom))
	    goto fail;
	window_name = MOCHA_GetAtomName(mc, window_name_atom);
	if (!lm_CheckWindowName(mc, window_name))
	    goto fail;
    } else {
	window_name = 0;
    }

    /* Check for window chrome options. */
    XP_BZERO(&chrome, sizeof chrome);
    if (argc > 2) {
	if (!MOCHA_DatumToString(mc, argv[2], &atom))
	    goto fail;
	options = (char *)MOCHA_GetAtomName(mc, atom);

	chrome.show_button_bar        = win_has_option(options, "toolbar");
	chrome.show_url_bar           = win_has_option(options, "location");
	chrome.show_directory_buttons = win_has_option(options, "directories");
	chrome.show_bottom_status_bar = win_has_option(options, "status");
	chrome.show_menu              = win_has_option(options, "menubar");
	chrome.show_security_bar      = FALSE;
	chrome.w_hint                 = win_has_option(options, "width");
	chrome.h_hint                 = win_has_option(options, "height");
	chrome.is_modal               = FALSE;
	chrome.show_scrollbar         = win_has_option(options, "scrollbars");
	chrome.allow_resize           = win_has_option(options, "resizable");
	chrome.allow_close            = TRUE;
	chrome.copy_history           = FALSE;	/* XXX need data tainting */

	/* Make sure windows are at least 100 by 100 pixels. */
	if (chrome.w_hint && chrome.w_hint < 100) chrome.w_hint = 100;
	if (chrome.h_hint && chrome.h_hint < 100) chrome.h_hint = 100;

	MOCHA_DropAtom(mc, atom);
	options = 0;
    } else {
	/* Make a fully chromed window, but don't copy history (XXX taint). */
	chrome.show_button_bar        = TRUE;
	chrome.show_url_bar           = TRUE;
	chrome.show_directory_buttons = TRUE;
	chrome.show_bottom_status_bar = TRUE;
	chrome.show_menu              = TRUE;
	chrome.show_security_bar      = FALSE;
	chrome.w_hint = chrome.h_hint = 0;
	chrome.is_modal               = FALSE;
	chrome.show_scrollbar         = TRUE;
	chrome.allow_resize           = TRUE;
	chrome.allow_close            = TRUE;
	chrome.copy_history           = FALSE;	/* XXX need data tainting */
    }

    old_context = decoder->window_context;
    if (!old_context) goto fail;
    if (window_name)
	context = XP_FindNamedContextInList(old_context, (char*)window_name);
    else
	context = 0;

    if (context) {
	new_decoder = LM_GetMochaDecoder(context);
	if (!new_decoder)
	    goto fail;
	if (url_struct && !lm_GetURL(mc, new_decoder, url_struct, url_taint)) {
	    url_struct = 0;
	    goto fail;
	}
	/* lm_GetURL() stashed a url_struct pointer, and owns it now. */
	url_struct = 0;
    } else {
	if (lm_window_count >= lm_window_limit)
	    goto fail;
	context = FE_MakeNewWindow(old_context, url_struct, (char*)window_name,
				   &chrome);
	if (!context)
	    goto fail;
	/* FE_MakeNewWindow() stashed a url_struct pointer, and owns it now. */
	url_struct = 0;
	new_decoder = LM_GetMochaDecoder(context);
	if (!new_decoder) {
	    (void) FE_SetTimeout(delete_window, context, 0);
	    goto fail;
	}
    }

    MOCHA_INIT_DATUM(mc, rval, MOCHA_OBJECT, u.obj, obj);
    if (!MOCHA_SetProperty(mc, new_decoder->window_object, lm_opener_str,
			   WIN_OPENER, *rval)) {
	goto fail;
    }
    new_decoder->opener = MOCHA_HoldObject(mc, obj);
    MOCHA_DropAtom(mc, window_name_atom);
    new_decoder->in_window_quota = MOCHA_TRUE;
    lm_window_count++;
    LM_PutMochaDecoder(new_decoder);
    MOCHA_INIT_DATUM(mc, rval, MOCHA_OBJECT, u.obj, new_decoder->window_object);
    return MOCHA_TRUE;

fail:
    MOCHA_DropAtom(mc, window_name_atom);
    if (new_decoder)
	LM_PutMochaDecoder(new_decoder);
    if (url_struct)
	NET_FreeURLStruct(url_struct);
    *rval = MOCHA_null;
    return MOCHA_TRUE;
}

static MochaBoolean
win_close(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MWContext *context;
    char *message;
    MochaBoolean ok;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    context = decoder->window_context;
    if (!context || context->grid_parent)
	return MOCHA_TRUE;
    if (context->type != MWContextBrowser ||
	(!decoder->opener &&
	 (XP_ListCount(SHIST_GetList(context)) > 1 ||
	  decoder != MOCHA_GetGlobalObject(mc)->data))) {
	/*
	** Prevent this window.close() call if this window
	** - is not a browser window, or
	** - was not opened by javascript, and
	** - has session history other than the current document, or
	** - does not contain the script that is closing this window.
	*/
	message = PR_smprintf("%s Confirm:\nClose window%s%s?",
			      mocha_language_name,
			      context->name ? " " : "",
			      context->name ? context->name : "");
	ok = FE_Confirm(context, message);
	XP_FREE(message);
	if (!ok)
	    return MOCHA_TRUE;
    }
    if (!decoder->win_close_toid)
	decoder->win_close_toid = FE_SetTimeout(delete_window, context, 0);
    return MOCHA_TRUE;
}

/*
** Timeout support.
*/
static uint  lm_timeout_count = 0;
static uint  lm_timeout_limit = 1000;
static int32 lm_timeout_slack = 100000;	/* taint-free timeout slack in usec */

#define FREE_TIMEOUT(mc, timeout) {                                           \
    MOCHA_free(mc, (timeout)->expr);                                          \
    MOCHA_free(mc, (timeout));                                                \
    lm_timeout_count--;                                                       \
}

static void
win_run_timeout(void *closure)
{
    MochaTimeout *timeout, **top;
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaTaintInfo *info;
    int64 now, slack, later;
    MochaDatum result;

    /* Remove this timeout from the decoder's timeout list. */
    timeout = closure;
    decoder = timeout->self;
    for (top = &decoder->timeouts; *top != timeout; top = &(*top)->next)
	if (*top == 0)
	    return;
    *top = timeout->next;

    /* Propagate taint from invoking script to timeout. */
    mc = decoder->mocha_context;
    info = MOCHA_GetTaintInfo(mc);
    MOCHA_MIX_TAINT(mc, info->accum, timeout->taint);

    /* Close timing channels by attributing branch taint to tardy timeouts. */
    now = PR_Now();
    LL_I2L(slack, lm_timeout_slack);
    LL_ADD(later, timeout->when, slack);
    if (LL_CMP(now, >, later)) {
	MOCHA_MIX_TAINT(mc, info->accum, lm_branch_taint);
    }

    /* Evaluate the timeout expression. */
    if (LM_EvaluateBuffer(decoder, timeout->expr, XP_STRLEN(timeout->expr), 0,
			  &result)) {
	MOCHA_DropRef(decoder->mocha_context, &result);
    }

    /* Free timeout and drop lm_timeout_count *after*, to handle rearming. */
    FREE_TIMEOUT(mc, timeout);
}

void
lm_ClearWindowTimeouts(MochaDecoder *decoder)
{
    MochaTimeout *timeout, *next;

    for (timeout = decoder->timeouts; timeout; timeout = next) {
	next = timeout->next;
	FE_ClearTimeout(timeout->toid);
	FREE_TIMEOUT(decoder->mocha_context, timeout);
    }
    decoder->timeouts = 0;
}

static MochaBoolean
win_set_timeout(MochaContext *mc, MochaObject *obj,
		uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaFloat interval;
    MochaAtom *atom;
    char *expr;
    MochaTimeout *timeout;
    int64 now, delta;

    if (lm_timeout_count >= lm_timeout_limit) {
	MOCHA_ReportError(mc, "too many timeouts");
	return MOCHA_FALSE;
    }

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (!lm_CheckOrigins(mc, decoder))
	return MOCHA_FALSE;

    if (!MOCHA_DatumToNumber(mc, argv[1], &interval))
	return MOCHA_FALSE;

    if (!MOCHA_DatumToString(mc, argv[0], &atom))
	return MOCHA_FALSE;
    expr = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom));
    MOCHA_DropAtom(mc, atom);
    if (!expr)
	return MOCHA_FALSE;

    timeout = MOCHA_malloc(mc, sizeof *timeout);
    if (!timeout) {
	MOCHA_free(mc, expr);
	return MOCHA_TRUE;
    }
    timeout->self = decoder;
    timeout->expr = expr;
    timeout->taint = MOCHA_GetTaintInfo(mc)->accum;
    MOCHA_MIX_TAINT(mc, timeout->taint, argv[0].taint);
    MOCHA_MIX_TAINT(mc, timeout->taint, argv[1].taint);
    now = PR_Now();
    LL_D2L(delta, (double)interval);
    LL_ADD(timeout->when, now, delta);
    timeout->next = decoder->timeouts;
    decoder->timeouts = timeout;

    timeout->toid = FE_SetTimeout(win_run_timeout, timeout, (uint32)interval);
    if (!timeout->toid) {
	MOCHA_free(mc, expr);
	MOCHA_free(mc, timeout);
	return MOCHA_FALSE;
    }
    if (lm_timeout_count++ == 0)
	lm_branch_taint = MOCHA_TAINT_IDENTITY;
    MOCHA_INIT_DATUM(mc, rval, MOCHA_INTERNAL, u.ptr, timeout->toid);
    return MOCHA_TRUE;
}

static MochaBoolean
win_clear_timeout(MochaContext *mc, MochaObject *obj,
		  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    void *toid;
    MochaTimeout **top, *timeout;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (argv[0].tag != MOCHA_INTERNAL)
	return MOCHA_TRUE;
    toid = argv[0].u.ptr;
    for (top = &decoder->timeouts; (timeout = *top); top = &timeout->next) {
	if (timeout->toid == toid) {
	    *top = timeout->next;
	    FE_ClearTimeout(timeout->toid);
	    FREE_TIMEOUT(mc, timeout);
	    break;
	}
    }
    return MOCHA_TRUE;
}

static MochaBoolean
win_escape(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaFloat fval;
    MochaAtom *atom;
    char *str;
    int mask;

    if (argc > 1) {
	if (!MOCHA_DatumToNumber(mc, argv[1], &fval))
	    return MOCHA_FALSE;
	mask = (int)fval;
	if (mask & ~(URL_XALPHAS | URL_XPALPHAS | URL_PATH)) {
	    MOCHA_ReportError(mc, "invalid string escape mask %x", mask);
	    return MOCHA_FALSE;
	}
    } else {
	mask = URL_XALPHAS | URL_XPALPHAS | URL_PATH;
    }
    if (!MOCHA_DatumToString(mc, argv[0], &atom))
	return MOCHA_FALSE;
    str = NET_Escape(MOCHA_GetAtomName(mc, atom), mask);
    MOCHA_DropAtom(mc, atom);
    if (!str) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }

    atom = MOCHA_Atomize(mc, str);
    XP_FREE(str);
    if (!atom) return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, rval, MOCHA_STRING, u.atom, atom);
    return MOCHA_TRUE;
}

static MochaBoolean
win_unescape(MochaContext *mc, MochaObject *obj,
	     uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaAtom *atom;
    char *str;

    if (!MOCHA_DatumToString(mc, argv[0], &atom))
	return MOCHA_FALSE;
    if (!(str = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom))))
	return MOCHA_FALSE;
    NET_UnEscape(str);
    atom = MOCHA_Atomize(mc, str);
    XP_FREE(str);
    if (!atom) return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, rval, MOCHA_STRING, u.atom, atom);
    return MOCHA_TRUE;
}

static MochaBoolean
win_focus(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (decoder->window_context)
	FE_FocusInputElement(decoder->window_context, 0);
    return MOCHA_TRUE;
}

static MochaBoolean
win_blur(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (decoder->window_context)
	FE_BlurInputElement(decoder->window_context, 0);
    return MOCHA_TRUE;
}

/*
** Scrolling support.
*/
static MochaBoolean
win_scroll(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    int32 x, y;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (argc != 2 ||
	argv[0].tag != MOCHA_NUMBER ||
	argv[1].tag != MOCHA_NUMBER ||
	!decoder->window_context) {
	return MOCHA_TRUE;
    }
    x = (int) argv[0].u.fval;
    y = (int) argv[1].u.fval;
    FE_ScrollDocTo(decoder->window_context, 0, x, y);
    return MOCHA_TRUE;
}

/*
** Run load events from the timeout loop to avoid freeing contexts in use,
** e.g., parent.document.write() in a frame's BODY onLoad= attribute.
*/
static void
load_event(void *closure)
{
    MochaDecoder *decoder;
    MWContext *context;
    MochaDatum rval;

    decoder = closure;
    context = decoder->window_context;
    if (context) {
	/* Can't call if the window was closed. */
	(void) lm_SendEvent(context, decoder->window_object, lm_onLoad_str,
			    &rval);
    }
    LM_PutMochaDecoder(decoder);
}

static MochaBoolean
is_context_really_busy(MWContext *context)
{
    lo_TopState *top_state;
    int i;
    XP_List *kids;
    MWContext *kid;
    
    if (!context)
	return MOCHA_FALSE;

    if ((top_state = lo_FetchTopState(XP_DOCID(context))) &&
	top_state->mocha_has_onload &&
	(top_state->mocha_loading_applets_count ||
	 top_state->mocha_loading_embeds_count)) {
	return MOCHA_TRUE;
    }

    if ((kids = context->grid_children)) {
	for (i = 1; (kid = XP_ListGetObjectNum(kids, i)); i++) {
	    if (is_context_really_busy(kid))
		return MOCHA_TRUE;
	}
    }
    return MOCHA_FALSE;
}

MochaBoolean
is_context_busy(MWContext *context)
{
    if (XP_IsContextBusy(context))
	return MOCHA_TRUE;
    return is_context_really_busy(context);
}

static void
try_load_event(MWContext *context, MochaDecoder *decoder)
{
    MochaContext *mc;

    /*
    ** Don't do anything if we are waiting for URLs to load, or if applets
    ** have not been loaded and initialized.
    */
    if (is_context_busy(context))
	return;

    /*
    ** Send a load event at most once.  Also, if we can't create a timeout,
    ** we give up rather than risk flaming death.
    */
    if (decoder && !decoder->load_event_sent) {
	decoder->load_event_sent = MOCHA_TRUE;
	mc = decoder->mocha_context;
	MOCHA_HoldObject(mc, decoder->window_object);
	if (!FE_SetTimeout(load_event, decoder, 0))
	    MOCHA_DropObject(mc, decoder->window_object);
    }

    /*
    ** Now that we may have scheduled a load event for this context, send
    ** a transfer-done event to our parent frameset, if any.
    */
    if (context->grid_parent)
	LM_SendLoadEvent(context->grid_parent, LM_XFER_DONE);
}

/*
** Entry point for the netlib to notify Mocha of load and unload events.
*/
void
LM_SendLoadEvent(MWContext *context, LM_LoadEvent event)
{
    MochaDecoder *decoder;
    MochaDatum rval;

    decoder = context->mocha_context ? LM_GetMochaDecoder(context) : 0;
    if (decoder && decoder->resize_reload)
	goto out;

    switch (event) {
      case LM_LOAD:
	if (decoder && decoder->mocha_context) {
	    if (context->win_csid & MULTIBYTE)
		MOCHA_SetCharFilter(decoder->mocha_context, (MochaCharFilter) INTL_IsLeadByte, (void *) context->win_csid);
	    else
		MOCHA_SetCharFilter(decoder->mocha_context, NULL, NULL);
	}
	if (decoder) {
	    if (decoder->url_struct && decoder->url_struct->resize_reload) {
		decoder->resize_reload = MOCHA_TRUE;
		goto out;
	    }
	    decoder->event_mask |= EVENT_LOAD;
	}
	try_load_event(context, decoder);
	break;

      case LM_UNLOAD:
	if (decoder && decoder->mocha_context) {
	    if (context->win_csid & MULTIBYTE)
		MOCHA_SetCharFilter(decoder->mocha_context, (MochaCharFilter) INTL_IsLeadByte, (void *) context->win_csid);
	    else
		MOCHA_SetCharFilter(decoder->mocha_context, NULL, NULL);
	}
	if (decoder) {
	    decoder->event_mask &= ~EVENT_LOAD;
	    decoder->load_event_sent = MOCHA_FALSE;
	    (void) lm_SendEvent(context, decoder->window_object,
				lm_onUnload_str, &rval);
	}
	break;

      case LM_XFER_DONE:
	if (decoder && (decoder->event_mask & EVENT_LOAD))
	    try_load_event(context, decoder);
	if (context->grid_parent)
	    LM_SendLoadEvent(context->grid_parent, LM_XFER_DONE);
	break;

      default:;
    }

out:
    if (decoder)
	LM_PutMochaDecoder(decoder);
    return;
}

/*
** Entry point for front-ends to notify Mocha code of scroll events.
*/
void
LM_SendOnScroll(MWContext *context, int32 x, int32 y)
{
    MochaDecoder *decoder;
    MochaDatum rval;

    if (!context->mocha_context)
	return;
    decoder = LM_GetMochaDecoder(context);
    if (!(decoder->event_mask & EVENT_SCROLL)) {
	decoder->event_mask |= EVENT_SCROLL;
	(void) lm_SendEvent(context, decoder->window_object, lm_onScroll_str,
			    &rval);
	decoder->event_mask &= ~EVENT_SCROLL;
    }
    LM_PutMochaDecoder(decoder);
}

static MochaFunctionSpec lm_window_methods[] = {
    {"alert",           win_alert,              1},
    {"clearTimeout",    win_clear_timeout,      1},
    {"close",           win_close,              0},
    {"confirm",         win_confirm,            1},
    {"open",            win_open,               1},
    {"prompt",          win_prompt,             2},
    {"setTimeout",      win_set_timeout,        2},
    {"escape",          win_escape,             2},
    {"unescape",        win_unescape,           1},
    {lm_blur_str,       win_blur,               0},
    {lm_focus_str,      win_focus,              0},
    {lm_scroll_str,     win_scroll,             2},
    {0}
};

MochaDecoder *
lm_NewWindow(MWContext *context)
{
    History_entry *he;
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaObject *obj;

    switch (context->type) {
      case MWContextBrowser:
	if (EDT_IS_EDITOR(context))
	    return 0; /* No Mocha for editor contexts. */
	/* FALL THROUGH */
      case MWContextDialog:
      case MWContextMail:
      case MWContextNews:
	break;
      default:
	return 0;
    }

    /* If this is a (resizing) frame, get its decoder from session history. */
    he = context->is_grid_cell ? SHIST_GetCurrent(&context->hist) : 0;
    if (he && he->savedData.Window) {
	decoder = he->savedData.Window;
	he->savedData.Window = 0;
	mc = decoder->mocha_context;
	obj = decoder->window_object;
	decoder->window_context = context;
	context->mocha_context = mc;
	if (!lm_RestoreAnonymousImages(context, decoder)) {
	    MOCHA_DropObject(mc, obj);
	    return 0;
	}
	return decoder;
    }

    /* Otherwise, make a new decoder/context/object for this window. */
    decoder = XP_NEW_ZAP(MochaDecoder);
    if (!decoder)
	return 0;
    mc = MOCHA_NewContext(LM_STACK_SIZE);
    if (!mc) {
	XP_DELETE(decoder);
	return 0;
    }
    obj = MOCHA_NewObject(mc, &lm_window_class, decoder, 0, 0, 0, 0);
    if (!obj) {
	MOCHA_DestroyContext(mc);
	XP_DELETE(decoder);
	return 0;
    }

    /* MWContext holds window_object even though it points at decoder */
    decoder->window_object = MOCHA_HoldObject(mc, obj);
    decoder->window_context = context;
    decoder->mocha_context = mc;
    context->mocha_context = mc;

    if (!lm_InitWindowContent(decoder)) {
	/* This drop will cause finalization and call lm_DestroyWindow(). */
	MOCHA_DropObject(mc, obj);
	return 0;
    }

    LM_LINK_OBJECT(decoder, obj, context->name);
    decoder->url_list_tail = &decoder->next_url_list;
    decoder->write_taint_accum = MOCHA_TAINT_IDENTITY;
    MOCHA_SetBranchCallback(mc, lm_BranchCallback);
    MOCHA_SetErrorReporter(mc, lm_ErrorReporter);
    return decoder;
}

void
lm_DestroyWindow(MochaDecoder *decoder)
{
    MochaContext *mc;

    /* We must not have an MWContext at this point. */
    XP_ASSERT(!decoder->window_context);

    /* Drop decoder refs to window prototypes and sub-objects. */
    lm_FreeWindowContent(decoder);

    /* The opener property could be set even if no document was loaded. */
    mc = decoder->mocha_context;
    MOCHA_DropObject(mc, decoder->opener);
    if (decoder->in_window_quota)
	lm_window_count--;

    /* Free Mocha context and decoder struct, window is gone. */
    MOCHA_DestroyContext(mc);
    XP_DELETE(decoder);
}

MochaBoolean
lm_InitWindowContent(MochaDecoder *decoder)
{
    MochaContext *mc;
    MochaObject *obj;
    MochaDatum d;

    mc = decoder->mocha_context;
    obj = decoder->window_object;
    /* XXX add properties first, else standard methods use their slots! */
    if (!MOCHA_SetProperties(mc, obj, window_props))
	return MOCHA_FALSE;
    if (!MOCHA_SetGlobalObject(mc, obj))
	return MOCHA_FALSE;
    if (decoder->opener) {
	MOCHA_INIT_FULL_DATUM(mc, &d, MOCHA_OBJECT, MDF_ENUMERATE,
			      MOCHA_TAINT_IDENTITY, u.obj, decoder->opener);
	if (!MOCHA_SetProperty(mc, obj, lm_opener_str, WIN_OPENER, d))
	    return MOCHA_FALSE;
    }
    return MOCHA_DefineFunctions(mc, obj, lm_window_methods) &&
	   lm_DefineDocument(decoder) &&
	   lm_DefineHistory(decoder) &&
	   lm_DefineLocation(decoder) &&
	   lm_DefineNavigator(decoder) &&
	   lm_InitImageClass(decoder) &&
	   lm_InitInputClasses(decoder);
}

void
lm_FreeWindowContent(MochaDecoder *decoder)
{
    MochaContext *mc;
    MochaObject *obj;

    /* Clear any stream that the user forgot to close. */
    LM_ClearDecoderStream(decoder);

    /* Clear any pending timeouts and URL fetches. */
    lm_ClearWindowTimeouts(decoder);
    lm_ClearNextURLList(decoder);

    /* These flags should never be set, but if any are, we'll cope. */
    decoder->replace_location = MOCHA_FALSE;
    decoder->resize_reload    = MOCHA_FALSE;
    decoder->load_event_sent  = MOCHA_FALSE;
    decoder->domain_set       = MOCHA_FALSE;
    decoder->visited          = MOCHA_FALSE;
    decoder->writing_input    = MOCHA_FALSE;

    /* Reset written taint so lm_CheckGetURL doesn't misfire. */
    decoder->write_taint_accum = MOCHA_TAINT_IDENTITY;

#define FREE_AND_CLEAR(p)	if (p) { XP_FREE((char *)p); p = 0; }

    FREE_AND_CLEAR(decoder->origin_url);
    FREE_AND_CLEAR(decoder->source_url);
    FREE_AND_CLEAR(decoder->nesting_url);
    FREE_AND_CLEAR(decoder->buffer);

#undef  FREE_AND_CLEAR

    /* Forgive and forget all excessive errors. */
    decoder->error_count = 0;

    /* Clear the event mask. */
    decoder->event_mask = 0;

    mc = decoder->mocha_context;

#define DROP_AND_CLEAR(obj)	MOCHA_DropObject(mc, obj); obj = 0

    /* Clear all object prototype refs. */
    DROP_AND_CLEAR(decoder->form_prototype);
    DROP_AND_CLEAR(decoder->image_prototype);
    DROP_AND_CLEAR(decoder->input_prototype);
    DROP_AND_CLEAR(decoder->option_prototype);
    DROP_AND_CLEAR(decoder->url_prototype);

    /* Clear the active form weak link in case we're still in a form. */
    decoder->active_form = 0;

    /* Drop and clear window sub-object refs. */
    DROP_AND_CLEAR(decoder->document);
    DROP_AND_CLEAR(decoder->history);
    DROP_AND_CLEAR(decoder->location);
    DROP_AND_CLEAR(decoder->navigator);

    /* Drop and clear document sub-object refs. */
    DROP_AND_CLEAR(decoder->forms);
    DROP_AND_CLEAR(decoder->links);
    DROP_AND_CLEAR(decoder->anchors);
    DROP_AND_CLEAR(decoder->applets);
    DROP_AND_CLEAR(decoder->embeds);
    DROP_AND_CLEAR(decoder->images);

#undef DROP_AND_CLEAR

    obj = decoder->window_object;
    if (obj) {
	MOCHA_ClearScope(mc, obj);
	(void) MOCHA_SetProperty(mc, obj, lm_closed_str, WIN_CLOSED,
				 MOCHA_false);
    }

    /* Clear this context's taint; taint for its origin may linger, however */
    lm_ClearTaint(mc);

    /* XXX fix this please! */
    decoder->anon_image_clients = 0;

#ifdef DEBUG_brendan
    lm_DumpLiveObjects(decoder);
#endif
}

/* XXX prototype in layout.h please! */
extern lo_GridCellRec *
lo_ContextToCell(MWContext *context, lo_GridRec **grid_ptr);

void
LM_RemoveWindowContext(MWContext *context)
{
    MochaDecoder *decoder;
    MochaContext *mc;

    /* Do work only if this context has a Mocha decoder. */
    mc = context->mocha_context;
    if (!mc)
	return;
    decoder = MOCHA_GetGlobalObject(mc)->data;

    /* Sever the bidirectional connection between context and decoder. */
    XP_ASSERT(decoder->window_context == context);
    context->mocha_context = 0;
    decoder->window_context = 0;
    LM_ClearDecoderStream(decoder);

    /*
     * Frames are special, because their contexts are destroyed and recreated
     * when they're reloaded, even when resizing.
     */
    if (context->is_grid_cell) {
	lo_TopState *top_state;

	top_state = lo_FetchTopState(XP_DOCID(context));
	if (top_state && top_state->resize_reload) {
	    lo_GridRec *grid = 0;
	    lo_GridCellRec *rec = lo_ContextToCell(context, &grid);

	    if (rec && rec->history) {
		History_entry *he = (History_entry *)rec->history;

		/*
		 * Set current history entry's saved window from its layout cell.
		 * We need to do this rather than SHIST_SetCurrentDocWindowData()
		 * because FE_FreeGridWindow (who calls us indirectly) has already
		 * "stolen" session history for return to layout, who saves it in
		 * parent session history in lo_InternalDiscardDocument().
		 */
		he->savedData.Window = decoder;
		lm_SaveAnonymousImages(context, decoder);
		return;
	    }
	    if (context->hist.list_ptr) {
		/*
		 * XXX Somehow the context's history list still exists (see
		 * SHIST_EndSession() and FE_FreeGridWindow() for two places
		 * that null it; but note unfixed shist.c bug: EndSession does
		 * not null cur_doc_ptr, which SHIST_GetCurrent() returns).
		 *
		 * This case "can't happen" (ha ha).  But if it does, save the
		 * decoder in the intact context history list.
		 */
		SHIST_SetCurrentDocWindowData(context, decoder);
		lm_SaveAnonymousImages(context, decoder);
		return;
	    }
	    /* Else we must free the window object tree now, rather than leak. */
	}
    }

    /* This might call lm_DestroyWindow(decoder), so do it last. */
    MOCHA_DropObject(mc, decoder->window_object);
}

extern void
LM_DropSavedWindow(MWContext *context, void *window)
{
    MochaDecoder *decoder = window;

    MOCHA_DropObject(decoder->mocha_context, decoder->window_object);
}
