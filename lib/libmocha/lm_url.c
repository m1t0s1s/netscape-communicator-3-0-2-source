/*
** Mocha reflection of the current Navigator URL (Location) object.
**
** Brendan Eich, 9/8/95
*/
#include "lm.h"
#include "xp.h"
#include "net.h"		/* for URL_Struct */
#include "shist.h"		/* for session history stuff */
#include "structs.h"		/* for MWContext */
#include "fe_proto.h"
#include "layout.h"		/* included via -I../layout */
#include "mkparse.h"		/* included via -I../libnet */

/* NB: these named properties use non-negative slots; code below knows that. */
enum url_slot {
    URL_HREF,			/* the entire URL as a string */
    URL_PROTOCOL,		/* protocol:... */
    URL_HOST,			/* protocol://host/... */
    URL_HOSTNAME,		/* protocol://hostname:... */
    URL_PORT,			/* protocol://hostname:port/... */
    URL_PATHNAME,		/* protocol://host/pathname[#?...] */
    URL_HASH,			/* protocol://host/pathname#hash */
    URL_SEARCH,			/* protocol://host/pathname?search */
    URL_TARGET,			/* target window or null */
    LOC_ASSIGN  = -1,
    LOC_RELOAD  = -2,
    LOC_REPLACE = -3
};

static MochaPropertySpec url_props[] = {
    {"href",            URL_HREF,       MDF_ENUMERATE | MDF_TAINTED},
    {"protocol",        URL_PROTOCOL,   MDF_ENUMERATE | MDF_TAINTED},
    {"host",            URL_HOST,       MDF_ENUMERATE | MDF_TAINTED},
    {"hostname",        URL_HOSTNAME,   MDF_ENUMERATE | MDF_TAINTED},
    {"port",            URL_PORT,       MDF_ENUMERATE | MDF_TAINTED},
    {"pathname",        URL_PATHNAME,   MDF_ENUMERATE | MDF_TAINTED},
    {"hash",            URL_HASH,       MDF_ENUMERATE | MDF_TAINTED},
    {"search",          URL_SEARCH,     MDF_ENUMERATE | MDF_TAINTED},
    {"target",          URL_TARGET,     MDF_ENUMERATE},
    {lm_assign_str,     LOC_ASSIGN,     MDF_READONLY},
    {lm_reload_str,     LOC_RELOAD,     MDF_READONLY},
    {lm_replace_str,    LOC_REPLACE,    MDF_READONLY},
    {0}
};

#define ParseURL(url,part)	((url) ? NET_ParseURL(url,part) : 0)

static MochaBoolean
url_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaURL *url;
    MochaAtom *atom;
    char *str, *port;

    url = obj->data;
    atom = 0;
    str = 0;

    switch (slot) {
      case URL_HREF:
	atom = url->href;
	break;

      case URL_PROTOCOL:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_PROTOCOL_PART);
	break;

      case URL_HOST:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_HOST_PART);
	break;

      case URL_HOSTNAME:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_HOST_PART);
	if (str && (port = XP_STRCHR(str, ':')) != 0)
	    *port = '\0';
	break;

      case URL_PORT:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_HOST_PART);
	if (str && (port = XP_STRCHR(str, ':')) != 0)
	    port++;
	else
	    port = "";
	atom = MOCHA_Atomize(mc, port);
	break;

      case URL_PATHNAME:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_PATH_PART);
	break;

      case URL_HASH:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_HASH_PART);
	break;

      case URL_SEARCH:
	str = ParseURL(MOCHA_GetAtomName(mc, url->href), GET_SEARCH_PART);
	break;

      case URL_TARGET:
	if (!url->target) {
	    *dp = MOCHA_null;
	    return MOCHA_TRUE;
	}
	atom = url->target;
	break;

      default:
	/* Don't mess with user-defined or method properties. */
	return MOCHA_TRUE;
    }

    if (!atom && str)
	atom = MOCHA_Atomize(mc, str);
    if (str)
	XP_FREE(str);
    if (!atom)
	return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	dp->taint = url->taint;
    return MOCHA_TRUE;
}

static MochaBoolean
url_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaURL *url;
    const char *href, *name, *checked_href;
    char *new_href;
    uint16 new_taint;
    MochaBoolean free_href;
    MochaDatum tmp;
    MochaAtom *atom;
    MWContext *context;
    LO_AnchorData *anchor_data;
    MochaBoolean ok;

    if (slot < 0) {
	/* Don't mess with user-defined or method properties. */
	return MOCHA_TRUE;
    }

    if (dp->tag != MOCHA_STRING &&
	!MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	return MOCHA_FALSE;
    }
    url = obj->data;
    ok = MOCHA_TRUE;

    switch (slot) {
      case URL_HREF:
	if (dp->u.atom != url->href) {
	    MOCHA_DropAtom(mc, url->href);
	    url->href = MOCHA_HoldAtom(mc, dp->u.atom);
	}
	url->taint = dp->taint;
	href = MOCHA_GetAtomName(mc, dp->u.atom);
	free_href = MOCHA_FALSE;
	break;

      case URL_PROTOCOL:
      case URL_HOST:
      case URL_HOSTNAME:
      case URL_PORT:
      case URL_PATHNAME:
      case URL_HASH:
      case URL_SEARCH:
	/* a component property changed -- recompute href and taint. */
	new_href = 0;
	new_taint = MOCHA_TAINT_IDENTITY;

#define GET_SLOT(aslot, ptmp) {                                               \
    if (aslot == slot) {                                                      \
	(ptmp)->u.atom = dp->u.atom;                                          \
	(ptmp)->taint = dp->taint;                                            \
    } else {                                                                  \
	if (!MOCHA_GetSlot(mc, obj, aslot, ptmp)) {                           \
	    if (new_href) XP_FREE(new_href);                                  \
	    return MOCHA_FALSE;                                               \
	}                                                                     \
    }                                                                         \
}

#define ADD_SLOT(aslot, separator) {                                          \
    GET_SLOT(aslot, &tmp);                                                    \
    name = MOCHA_GetAtomName(mc, tmp.u.atom);                                 \
    if (*name) {                                                              \
	if (separator) StrAllocCat(new_href, separator);                      \
	StrAllocCat(new_href, name);                                          \
	MOCHA_MIX_TAINT(mc, new_taint, tmp.taint);                            \
    }                                                                         \
}

	GET_SLOT(URL_PROTOCOL, &tmp);
	StrAllocCopy(new_href, MOCHA_GetAtomName(mc, tmp.u.atom));
	if (slot == URL_HOST) {
	    ADD_SLOT(URL_HOST, "//");
	} else {
	    ADD_SLOT(URL_HOSTNAME, "//");
	    ADD_SLOT(URL_PORT, ":");
	}
	ADD_SLOT(URL_PATHNAME, 0);
	ADD_SLOT(URL_HASH, "#");
	ADD_SLOT(URL_SEARCH, 0);

	if (!new_href) {
	    MOCHA_ReportOutOfMemory(mc);
	    return MOCHA_FALSE;
	}

	free_href = MOCHA_TRUE;
	href = new_href;
	atom = MOCHA_Atomize(mc, href);
	if (!atom) {
	    ok = MOCHA_FALSE;
	    goto out;
	}
	if (atom != url->href) {
	    MOCHA_DropAtom(mc, url->href);
	    url->href = MOCHA_HoldAtom(mc, atom);
	}
	url->taint = new_taint;
	break;

      case URL_TARGET:
	if (dp->u.atom != url->target) {
	    MOCHA_DropAtom(mc, url->target);
	    url->target = MOCHA_HoldAtom(mc, dp->u.atom);
	}
	if (url->index != URL_NOT_INDEXED) {
	    context = url->url_decoder->window_context;
	    if (context) {
		anchor_data = LO_GetLinkByIndex(context, url->index);
		if (anchor_data) {
		    name = MOCHA_GetAtomName(mc, dp->u.atom);
		    if (!lm_CheckWindowName(mc, name))
			return MOCHA_FALSE;
		    if (!lm_SaveParamString(mc, &anchor_data->target, name))
			return MOCHA_FALSE;
		}
	    }
	}
	/* Note early return, to bypass href update and freeing. */
	return MOCHA_TRUE;

      default:
	/* Don't mess with a user-defined property. */
	return ok;
    }

    if (url->index != URL_NOT_INDEXED) {
	context = url->url_decoder->window_context;
	if (context) {
	    anchor_data = LO_GetLinkByIndex(context, url->index);
	    if (anchor_data) {
		checked_href = lm_CheckURL(mc, href);
		if (!checked_href ||
		    !lm_SaveParamString(mc, &anchor_data->anchor,
					checked_href)) {
		    ok = MOCHA_FALSE;
		    goto out;
		}
		XP_FREE((char *)checked_href);
	    }
	}
    }

out:
    if (free_href && href)
	XP_FREE((char *)href);
    return ok;
}

static void
url_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaURL *url = obj->data;
    MWContext *context;
    LO_AnchorData *anchor_data;

    if (!url) return;
    if (url->index != URL_NOT_INDEXED) {
	context = url->url_decoder->window_context;
	if (context) {
	    anchor_data = LO_GetLinkByIndex(context, url->index);
	    if (anchor_data)
		anchor_data->mocha_object = 0;
	}
    }
    MOCHA_DropAtom(mc, url->href);
    MOCHA_DropAtom(mc, url->target);
    LM_UNLINK_OBJECT(url->url_decoder, obj);
    XP_DELETE(url);
}

static MochaClass url_class = {
    "URL",
    url_get_property, url_set_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, url_finalize
};

static MochaBoolean
URL(MochaContext *mc, MochaObject *obj,
    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
url_to_string(MochaContext *mc, MochaObject *obj,
	      uint argc, MochaDatum *argv, MochaDatum *rval)
{
    rval->flags |= MDF_TAINTED;
    return url_get_property(mc, obj, URL_HREF, rval);
}

static MochaFunctionSpec url_methods[] = {
    {lm_toString_str,	url_to_string,	0},
    {0}
};

MochaURL *
lm_NewURL(MochaContext *mc, MochaDecoder *decoder, char *href, char *target)
{
    MochaObject *obj;
    MochaURL *url;
    MochaAtom *atom;

    if (!decoder->url_prototype) {
	obj = MOCHA_InitClass(mc, decoder->window_object, &url_class, 0,
			      URL, 0, url_props, 0, 0, 0);
	if (!obj)
	    return 0;
	decoder->url_prototype = MOCHA_HoldObject(mc, obj);
    }

    url = MOCHA_malloc(mc, sizeof *url);
    if (!url)
	return 0;
    XP_BZERO(url, sizeof *url);
    url->url_decoder = decoder;
    url->url_type = FORM_TYPE_NONE;
    url->index = URL_NOT_INDEXED;

    obj = MOCHA_NewObject(mc, &url_class, url, decoder->url_prototype,
			  decoder->active_form ? decoder->active_form
					       : decoder->document,
			  0, 0);
    if (!obj) {
	MOCHA_free(mc, url);
	return 0;
    }
    MOCHA_DefineFunctions(mc, obj, url_methods);
    url->url_object = obj;

    atom = MOCHA_Atomize(mc, href);
    if (!atom) {
	MOCHA_DestroyObject(mc, obj);
	return 0;
    }
    url->href = MOCHA_HoldAtom(mc, atom);
    if (!lm_GetTaint(mc, decoder, &url->taint)) {
	MOCHA_DestroyObject(mc, obj);
	return 0;
    }

    atom = MOCHA_Atomize(mc, target);
    if (!atom) {
	MOCHA_DestroyObject(mc, obj);
	return 0;
    }
    url->target = MOCHA_HoldAtom(mc, atom);
    LM_LINK_OBJECT(decoder, obj, href);
    return url;
}

/*
** Top-level window URL object, a reflection of the "Location:" GUI field.
*/
static MochaBoolean
loc_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaURL *url;
    MochaDecoder *decoder;
    MWContext *context;
    History_entry *he;
    const char *url_string;
    MochaAtom *atom;

    url = obj->data;
    decoder = url->url_decoder;
    if ((dp->flags & MDF_TAINTED) && !lm_CheckOrigins(mc, decoder))
	return MOCHA_FALSE;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;
    he = SHIST_GetCurrent(&context->hist);
    if (he) {
	url_string = he->address;
	if (NET_URL_Type(url_string) == WYSIWYG_TYPE_URL &&
	    !(url_string = LM_SkipWysiwygURLPrefix(url_string))) {
	    url_string = he->address;
	}
	atom = MOCHA_Atomize(mc, url_string);
	if (!atom)
	    return MOCHA_FALSE;
	if (atom != url->href) {
	    MOCHA_DropAtom(mc, url->href);
	    url->href = MOCHA_HoldAtom(mc, atom);
	}
	if (!lm_GetTaint(mc, decoder, &url->taint))
	    return MOCHA_FALSE;
    }
    return url_get_property(mc, obj, slot, dp);
}

static void
url_notify(URL_Struct *url_struct, int status, MWContext *context)
{
}

void
lm_ReplaceURL(MWContext *context, URL_Struct *url_struct)
{
    History_entry *he;

    he = SHIST_GetCurrent(&context->hist);
    if (!he)
	return;
    he->history_num = SHIST_GetIndex(&context->hist, he);
    he->replace = TRUE;
    url_struct->history_num = he->history_num;
}

static void
get_next_url(URL_Struct *url_struct, int status, MWContext *context)
{
    MochaDecoder *decoder;
    MochaNextURL *next_url;

    /* Notify anyone waiting on this URL. */
    url_notify(url_struct, status, context);

    /* Get the mocha decoder for this context.  Be paranoid. */
    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return;

    next_url = decoder->next_url_list;
    if (next_url) {
	/* The next url_struct belongs to the FE now... */
	url_struct = next_url->url_struct;
	decoder->next_url_list = next_url->next;
	if (next_url->replace)
	    lm_ReplaceURL(context, url_struct);
	if (!decoder->next_url_list) {
	    decoder->url_list_tail = &decoder->next_url_list;
	    url_struct->pre_exit_fn = url_notify;
	} else {
	    url_struct->pre_exit_fn = get_next_url;
	}
	MOCHA_free(decoder->mocha_context, next_url);
	FE_GetURL(context, url_struct);
    }
    LM_PutMochaDecoder(decoder);
}

void
lm_ClearNextURLList(MochaDecoder *decoder)
{
    MochaNextURL *next_url;

    while ((next_url = decoder->next_url_list) != 0) {
	NET_FreeURLStruct(next_url->url_struct);
	decoder->next_url_list = next_url->next;
	MOCHA_free(decoder->mocha_context, next_url);
    }
    decoder->url_list_tail = &decoder->next_url_list;
}

MochaBoolean
lm_GetURL(MochaContext *mc, MochaDecoder *decoder, URL_Struct *url_struct,
	  uint16 taint)
{
    MWContext *context;
    MochaNextURL *next_url;

    context = decoder->window_context;
    if (!context) {
	NET_FreeURLStruct(url_struct);
	return MOCHA_TRUE;
    }

    if (!lm_CheckTaint(mc, taint, lm_location_str, url_struct->address)) {
	NET_FreeURLStruct(url_struct);
	return MOCHA_FALSE;
    }

    if (decoder->url_struct) {
	/*
	** Because decoder->url_struct is non-null, we know we are still
	** loading from a URL containing Mocha, either embedded in HTML or
	** from a 'mocha:location = "http://foo.com/bar/baz"' URL.
	**
	** Set the currently-loading URL_Struct's pre_exit_fn to point to
	** a special FE_GetURL wrapper function so we can chain indirectly
	** from the "mocha:" URL to the new location.
	*/
	next_url = MOCHA_malloc(mc, sizeof *next_url);
	if (!next_url) {
	    NET_FreeURLStruct(url_struct);
	    return MOCHA_FALSE;
	}
	next_url->url_struct = url_struct;
	next_url->replace = decoder->replace_location;
	decoder->replace_location = MOCHA_FALSE;
	next_url->next = 0;
	*decoder->url_list_tail = next_url;
	decoder->url_list_tail = &next_url->next;
	decoder->url_struct->pre_exit_fn = get_next_url;
    } else {
	/*
	** No chaining required, we must be in a non-loading context.
	*/
	if (decoder->replace_location) {
	    decoder->replace_location = MOCHA_FALSE;
	    lm_ReplaceURL(context, url_struct);
	}
	url_struct->pre_exit_fn = url_notify;
	FE_GetURL(context, url_struct);
    }
    return MOCHA_TRUE;
}

const char *
lm_CheckURL(MochaContext *mc, const char *url_string)
{
    char *protocol, *absolute;
    MochaDecoder *decoder;
    lo_TopState *top_state;

    protocol = NET_ParseURL(url_string, GET_PROTOCOL_PART);
    if (!protocol || *protocol == '\0') {
	decoder = MOCHA_GetGlobalObject(mc)->data;
	top_state = lo_GetMochaTopState(decoder->window_context);
	if (top_state && top_state->base_url) {
	    absolute = NET_MakeAbsoluteURL(top_state->base_url,
					   (char *)url_string);	/*XXX*/
	} else {
	    absolute = 0;
	}
	if (absolute) {
	    if (protocol) XP_FREE(protocol);
	    protocol = NET_ParseURL(absolute, GET_PROTOCOL_PART);
	}
    } else {
	absolute = MOCHA_strdup(mc, url_string);
	if (!absolute) {
	    XP_FREE(protocol);
	    return 0;
	}
    }

    if (absolute) {
	if (mc != lm_global_decoder->mocha_context) {
	    /* Make sure it's a safe URL type. */
	    switch (NET_URL_Type(protocol)) {
	      case FILE_TYPE_URL:
	      case FTP_TYPE_URL:
	      case GOPHER_TYPE_URL:
	      case HTTP_TYPE_URL:
	      case MAILTO_TYPE_URL:
	      case NEWS_TYPE_URL:
	      case RLOGIN_TYPE_URL:
	      case TELNET_TYPE_URL:
	      case TN3270_TYPE_URL:
	      case WAIS_TYPE_URL:
	      case SECURE_HTTP_TYPE_URL:
	      case URN_TYPE_URL:
	      case MOCHA_TYPE_URL:
	      case VIEW_SOURCE_TYPE_URL:
		/* These are "safe". */
		break;
	      case ABOUT_TYPE_URL:
		if (XP_STRCASECMP(absolute, "about:blank") == 0)
		    break;
		/* FALL THROUGH */
	      default:
		/* All others are naughty. */
		XP_FREE(absolute);
		absolute = 0;
		break;
	    }
	}
    }

    if (!absolute) {
	MOCHA_ReportError(mc, "illegal URL method '%s'",
			  protocol && *protocol ? protocol : "unknown");
    }
    if (protocol)
	XP_FREE(protocol);
    return absolute;
}

static MochaBoolean
url_load(MochaContext *mc, MochaObject *obj, NET_ReloadMethod reload_how)
{
    MochaURL *url;
    const char *url_string;
    URL_Struct *url_struct;

    url = obj->data;
    url_string = MOCHA_GetAtomName(mc, url->href);
    url_struct = NET_CreateURLStruct(url_string, reload_how);
    if (!url_struct) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }
    return lm_GetURL(mc, url->url_decoder, url_struct, url->taint);
}

static MochaBoolean
loc_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    const char *url_string;
    MochaAtom *atom;
    MochaDatum datum;

    /* Setting these properties should not cause a FE_GetURL. */
    if (slot < 0 || slot == URL_TARGET)
        return url_set_property(mc, obj, slot, dp);

    /* Make sure dp is a string. */
    if (dp->tag != MOCHA_STRING &&
	!MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	return MOCHA_FALSE;
    }

    /* Two cases: setting href vs. setting a component property. */
    if (slot == URL_HREF || slot == URL_PROTOCOL) {
	/* Make sure the URL is absolute and sanity-check its protocol. */
	url_string = MOCHA_GetAtomName(mc, dp->u.atom);
	url_string = lm_CheckURL(mc, url_string);
	if (!url_string)
	    return MOCHA_FALSE;
	atom = MOCHA_Atomize(mc, url_string);
	XP_FREE((char *)url_string);
	if (!atom) return MOCHA_FALSE;
	MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_STRING, 0,
			      MOCHA_TAINT_IDENTITY, u.atom, atom);
	dp = &datum;
    } else {
	/* Get href from session history before setting a piece of it. */
	if (!loc_get_property(mc, obj, URL_HREF, &datum))
	    return MOCHA_FALSE;
    }

    /* Set slot's property. */
    if (!url_set_property(mc, obj, slot, dp))
	return MOCHA_FALSE;
    return url_load(mc, obj, NET_DONT_RELOAD);
}

static MochaClass loc_class = {
    "Location",
    loc_get_property, loc_set_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, url_finalize
};

static MochaBoolean
Location(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
loc_to_string(MochaContext *mc, MochaObject *obj,
	      uint argc, MochaDatum *argv, MochaDatum *rval)
{
    rval->flags |= MDF_TAINTED;
    return loc_get_property(mc, obj, URL_HREF, rval);
}

static MochaBoolean
loc_assign(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDatum d;
    MochaURL *url;

    if (!MOCHA_InstanceOf(mc, obj, &loc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    d = argv[0];
    if (!loc_set_property(mc, obj, URL_HREF, &d))
	return MOCHA_FALSE;
    url = obj->data;
    MOCHA_INIT_DATUM(mc, rval, MOCHA_OBJECT, u.obj, url->url_object);
    return MOCHA_TRUE;
}

static MochaBoolean
loc_reload(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    if (!MOCHA_InstanceOf(mc, obj, &loc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    return url_load(mc, obj,
		    (argv[0].tag == MOCHA_BOOLEAN && argv[0].u.bval)
		    ? NET_SUPER_RELOAD
		    : NET_NORMAL_RELOAD);
}

static MochaBoolean
loc_replace(MochaContext *mc, MochaObject *obj,
	    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaURL *url;

    if (!MOCHA_InstanceOf(mc, obj, &loc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    url = obj->data;
    url->url_decoder->replace_location = MOCHA_TRUE;
    return loc_assign(mc, obj, argc, argv, rval);
}

static MochaFunctionSpec loc_methods[] = {
    {lm_toString_str,   loc_to_string,  0},
    {lm_assign_str,     loc_assign,     1},
    {lm_reload_str,     loc_reload,     1},
    {lm_replace_str,    loc_replace,    1},
    {0}
};

MochaObject *
lm_DefineLocation(MochaDecoder *decoder)
{
    MochaObject *obj;
    MochaContext *mc;
    MochaURL *url;

    obj = decoder->location;
    if (obj)
	return obj;

    mc = decoder->mocha_context;
    url = MOCHA_malloc(mc, sizeof *url);
    if (!url)
	return 0;
    obj = MOCHA_InitClass(mc, decoder->window_object, &loc_class, url,
			  Location, 0, url_props, loc_methods, 0, 0);
    if (!obj) {
	MOCHA_free(mc, url);
	return 0;
    }
    if (!MOCHA_DefineObject(mc, decoder->window_object, lm_location_str, obj,
			    MDF_ENUMERATE | MDF_READONLY)) {
	return 0;
    }

    /* Define the Location object (the current URL). */
    XP_BZERO(url, sizeof *url);
    url->url_decoder = decoder;
    url->url_type = FORM_TYPE_NONE;
    url->url_object = obj;
    url->index = URL_NOT_INDEXED;
    decoder->location = MOCHA_HoldObject(mc, obj);
    obj->parent = decoder->window_object;
    LM_LINK_OBJECT(decoder, obj, lm_location_str);
    return obj;
}
