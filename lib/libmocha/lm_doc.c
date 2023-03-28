/*
** Mocha reflection of the current Navigator Document.
**
** Brendan Eich, 9/8/95
*/
#include "lm.h"
#include "prtypes.h"
#include "prhash.h"
#include "prmem.h"
#include "prprf.h"
#include "prtime.h"
#include "xp.h"
#include "fe_proto.h"		/* just for FE_ClearView */
#include "lo_ele.h"
#include "shist.h"
#include "structs.h"		/* for MWContext */
#include "layout.h"		/* included via -I../layout */
#include "mkaccess.h"		/* from ../libnet */
#include "mkcache.h"		/* from ../libnet */
#include "pa_parse.h"		/* included via -I../libparse */
#include "pa_tags.h"		/* included via -I../libparse */

#if HAVE_SECURITY /* jwz */
#include "cert.h"		/* for CERT_DupCertificate */
#endif

enum doc_slot {
    DOC_LENGTH          = -1,
    DOC_ELEMENTS        = -2,
    DOC_FORMS           = -3,
    DOC_LINKS           = -4,
    DOC_ANCHORS         = -5,
    DOC_APPLETS         = -6,
    DOC_EMBEDS          = -7,
    DOC_IMAGES          = -8,
    DOC_TITLE           = -9,
    DOC_URL             = -10,
    DOC_REFERRER        = -11,
    DOC_LAST_MODIFIED   = -12,
    DOC_LOADED_DATE     = -13,
    DOC_COOKIE          = -14,
    DOC_DOMAIN          = -15,
    DOC_BG_COLOR        = -16,
    DOC_FG_COLOR        = -17,
    DOC_LINK_COLOR      = -18,
    DOC_VLINK_COLOR     = -19,
    DOC_ALINK_COLOR     = -20
};

static MochaPropertySpec doc_props[] = {
    {lm_length_str,  DOC_LENGTH,        MDF_READONLY|MDF_TAINTED},
    {"elements",     DOC_ELEMENTS,      MDF_READONLY|MDF_TAINTED},
    {lm_forms_str,   DOC_FORMS,         MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {lm_links_str,   DOC_LINKS,         MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {lm_anchors_str, DOC_ANCHORS,       MDF_ENUMERATE|MDF_READONLY},
    {lm_applets_str, DOC_APPLETS,       MDF_ENUMERATE|MDF_READONLY},
    {lm_embeds_str,  DOC_EMBEDS,        MDF_ENUMERATE|MDF_READONLY},
    {lm_plugins_str, DOC_EMBEDS,        MDF_READONLY},
    {lm_images_str,  DOC_IMAGES,        MDF_ENUMERATE|MDF_READONLY},
    {"title",        DOC_TITLE,         MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {"URL",          DOC_URL,           MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {"referrer",     DOC_REFERRER,      MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {"lastModified", DOC_LAST_MODIFIED, MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {"loadedDate",   DOC_LOADED_DATE,   MDF_READONLY},
    {"cookie",       DOC_COOKIE,        MDF_ENUMERATE|MDF_TAINTED},
    {"domain",       DOC_DOMAIN,        MDF_ENUMERATE|MDF_TAINTED},
    {"bgColor",      DOC_BG_COLOR,      MDF_ENUMERATE},
    {"fgColor",      DOC_FG_COLOR,      MDF_ENUMERATE},
    {"linkColor",    DOC_LINK_COLOR,    MDF_ENUMERATE},
    {"vlinkColor",   DOC_VLINK_COLOR,   MDF_ENUMERATE},
    {"alinkColor",   DOC_ALINK_COLOR,   MDF_ENUMERATE},
    {0}
};

#define LO_COLOR_TYPE(slot)                             \
    (((slot) == DOC_BG_COLOR) ? LO_COLOR_BG             \
     : ((slot) == DOC_FG_COLOR) ? LO_COLOR_FG           \
     : ((slot) == DOC_LINK_COLOR) ? LO_COLOR_LINK       \
     : ((slot) == DOC_VLINK_COLOR) ? LO_COLOR_VLINK     \
     : ((slot) == DOC_ALINK_COLOR) ? LO_COLOR_ALINK     \
     : -1)

typedef struct MochaDocument {
    MochaDecoder        *decoder;
    MochaObject         *object;
} MochaDocument;

static MochaBoolean
doc_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaDocument *doc;
    MochaDecoder *decoder;
    MWContext *context;
    History_entry *he;
    MochaAtom *atom;
    int64 prsec, scale, prusec;
    PRTime prtime;
    char *cookie, buf[100];
    const char *domain;
    uint32 type;
    LO_Color rgb;

    doc = obj->data;
    decoder = doc->decoder;
    if ((dp->flags & MDF_TAINTED) && !lm_CheckOrigins(mc, decoder))
	return MOCHA_FALSE;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    he = SHIST_GetCurrent(&context->hist);

    switch (slot) {
      case DOC_FORMS:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, lm_GetFormArray(decoder));
	(void) LO_EnumerateForms(context);
	return MOCHA_TRUE;

      case DOC_LINKS:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, lm_GetLinkArray(decoder));
	(void) LO_EnumerateLinks(context);
	return MOCHA_TRUE;

      case DOC_ANCHORS:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, lm_GetNameArray(decoder));
	(void) LO_EnumerateNamedAnchors(context);
	return MOCHA_TRUE;

      case DOC_APPLETS:
#ifdef JAVA
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, lm_GetAppletArray(decoder));
	(void) LO_EnumerateApplets(context);
#endif
	return MOCHA_TRUE;

      case DOC_EMBEDS:
#ifdef JAVA
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, lm_GetEmbedArray(decoder));
	(void) LO_EnumerateEmbeds(context);
#endif
	return MOCHA_TRUE;

      case DOC_IMAGES:
	MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, lm_GetImageArray(decoder));
	(void) LO_EnumerateImages(context);
	return MOCHA_TRUE;

      case DOC_TITLE:
	atom = MOCHA_Atomize(mc, he ? he->title : "");
	break;

      case DOC_URL:
	atom = MOCHA_Atomize(mc, he ? he->address : "");
	break;

      case DOC_REFERRER:
	atom = MOCHA_Atomize(mc, he ? he->referer : "");
	break;

      case DOC_LAST_MODIFIED:
	if (he) {
	    LL_I2L(prsec, he->last_modified);
	    LL_I2L(scale, 1000000);
	    LL_MUL(prusec, prsec, scale);
	    PR_ExplodeTime(&prtime, prusec);
	    PR_FormatTime(buf, sizeof buf, "%c", &prtime);
	} else {
	    buf[0] = '\0';
	}
	atom = MOCHA_Atomize(mc, buf);
	break;

      case DOC_LOADED_DATE:
	/* XXX todo */
	atom = MOCHA_empty.u.atom;
	break;

      case DOC_COOKIE:
	if (he && he->address) {
	    /* XXX overloaded return - can't distinguish "none" from "error" */
	    cookie = NET_GetCookie(context, he->address);
	} else {
	    cookie = 0;
	}
	atom = MOCHA_Atomize(mc, cookie);
	break;

      case DOC_DOMAIN:
	if (!decoder->origin_url && !lm_GetOriginURL(mc, decoder))
	    return MOCHA_FALSE;
	domain = NET_ParseURL(decoder->origin_url, GET_HOST_PART);
	if (!domain) {
	    MOCHA_ReportOutOfMemory(mc);
	    return MOCHA_FALSE;
	}
	atom = MOCHA_Atomize(mc, domain);
	XP_FREE((char *)domain);
	break;

      default:
	type = LO_COLOR_TYPE(slot);
	if (type >= LO_NCOLORS) {
	    /* Don't mess with a user-defined or method property. */
	    return MOCHA_TRUE;
	}
	LO_GetDocumentColor(context, type, &rgb);
	XP_SPRINTF(buf, "#%02x%02x%02x", rgb.red, rgb.green, rgb.blue);
	atom = MOCHA_Atomize(mc, buf);
	break;
    }

    /* Common tail code for string-type properties. */
    if (!atom)
	return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	return lm_GetTaint(mc, decoder, &dp->taint);
    return MOCHA_TRUE;
}

static MochaBoolean
doc_set_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		 MochaDatum *dp)
{
    MochaDocument *doc;
    MochaDecoder *decoder;
    MWContext *context;
    History_entry *he;
    char *cookie, *protocol, *pathname, *new_origin_url;
    const char *domain, *new_domain, *domain_suffix;
    size_t domain_len, new_domain_len;
    MochaBoolean ok;
    uint32 type, color;
    LO_Color rgb;

    doc = obj->data;
    decoder = doc->decoder;
    if ((dp->flags & MDF_TAINTED) && !lm_CheckOrigins(mc, decoder))
	return MOCHA_FALSE;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;

    switch (slot) {
      case DOC_COOKIE:
	if (!decoder->origin_url && !lm_GetOriginURL(mc, decoder))
	    return MOCHA_FALSE;
	if (!lm_CheckTaint(mc, dp->taint, "cookie for ", decoder->origin_url))
	    return MOCHA_FALSE;
	if (dp->tag != MOCHA_STRING &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	    return MOCHA_FALSE;
	}
	he = SHIST_GetCurrent(&context->hist);
	if (he && he->address) {
	    /* Must make a writable copy for netlib to munge gratuitously. */
	    cookie = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, dp->u.atom));
	    if (!cookie)
		return MOCHA_FALSE;
	    NET_SetCookieString(context, he->address, cookie);
	    MOCHA_free(mc, cookie);
	}
	break;

      case DOC_DOMAIN:
	if (!decoder->origin_url && !lm_GetOriginURL(mc, decoder))
	    return MOCHA_FALSE;
	domain = NET_ParseURL(decoder->origin_url, GET_HOST_PART);
	if (!domain) {
	    MOCHA_ReportOutOfMemory(mc);
	    return MOCHA_FALSE;
	}
	if (dp->tag != MOCHA_STRING &&
	    !MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp)) {
	    XP_FREE((char *)domain);
	    return MOCHA_FALSE;
	}
	new_domain = MOCHA_GetAtomName(mc, dp->u.atom);
	new_domain_len = XP_STRLEN(new_domain);
	domain_len = XP_STRLEN(domain);
	domain_suffix = domain + domain_len - new_domain_len;
	ok = (domain_len > new_domain_len &&
	      !XP_STRCASECMP(domain_suffix, new_domain) &&
	      (domain_suffix[-1] == '.' || domain_suffix[-1] == '/'));
	if (!ok) {
	    MOCHA_ReportError(mc, "illegal document.domain value %s",
			      new_domain);
	} else {
	    protocol = NET_ParseURL(decoder->origin_url, GET_PROTOCOL_PART);
	    pathname = NET_ParseURL(decoder->origin_url,
				    GET_PATH_PART | GET_HASH_PART |
				    GET_SEARCH_PART);
	    new_origin_url = PR_smprintf("%s//%s%s",
				         protocol, new_domain, pathname);
	    ok = (protocol && pathname && new_origin_url);
	    if (!ok) {
		MOCHA_ReportOutOfMemory(mc);
	    } else {
		ok = (lm_SetOriginURL(mc, decoder, new_origin_url) != 0);
		if (ok)
		    decoder->domain_set = MOCHA_TRUE;
	    }
	    PR_FREEIF(protocol);
	    PR_FREEIF(pathname);
	    PR_FREEIF(new_origin_url);
	}
	XP_FREE((char *)domain);
	return ok;

      case DOC_BG_COLOR:
      case DOC_FG_COLOR:
      case DOC_LINK_COLOR:
      case DOC_VLINK_COLOR:
      case DOC_ALINK_COLOR:
	type = LO_COLOR_TYPE(slot);
	if (type >= LO_NCOLORS)
	    break;

	switch (dp->tag) {
	  default:
	    if (!MOCHA_ConvertDatum(mc, *dp, MOCHA_STRING, dp))
		return MOCHA_FALSE;
	    /* FALL THROUGH */

	  case MOCHA_STRING:
	    /* XXX why not pass &rgb? */
	    if (LO_ParseRGB((char *)MOCHA_GetAtomName(mc, dp->u.atom),/*XXX*/
			    &rgb.red, &rgb.green, &rgb.blue)) {
		LO_SetDocumentColor(context, type, &rgb);
	    }
	    break;

	  case MOCHA_NUMBER:
	    color = (uint32)dp->u.fval;
	    if ((MochaFloat)color == dp->u.fval && (color >> 24) == 0) {
		rgb.red = color >> 16;
		rgb.green = (color >> 8) & 0xff;
		rgb.blue = color & 0xff;
		LO_SetDocumentColor(context, type, &rgb);
	    }
	    break;
	}
	break;
    }

    return doc_get_property(mc, obj, slot, dp);
}

static MochaBoolean
doc_list_properties(MochaContext *mc, MochaObject *obj)
{
#ifdef JAVA
    /* reflect applets eagerly, anything else? */
    MochaDocument *doc;
    MWContext *context;

    doc = obj->data;
    context = doc->decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    (void) LO_EnumerateApplets(context);
    (void) LO_EnumerateEmbeds(context);
#endif
    return MOCHA_TRUE;
}

static MochaBoolean
doc_resolve_name(MochaContext *mc, MochaObject *obj, const char *name)
{
    if (!XP_STRCMP(name, lm_location_str)) {
	MochaDocument *doc = obj->data;

	return MOCHA_DefineObject(mc, obj, name, doc->decoder->location, 0);
    }
    return doc_list_properties(mc, obj);
}

static void
doc_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaDocument *doc = obj->data;

    LM_UNLINK_OBJECT(doc->decoder, obj);
    MOCHA_free(mc, doc);
}

static MochaClass doc_class = {
    "Document",
    doc_get_property, doc_set_property, doc_list_properties,
    doc_resolve_name, MOCHA_ConvertStub, doc_finalize
};

static MochaBoolean
Document(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
doc_to_string(MochaContext *mc, MochaObject *obj,
	      uint argc, MochaDatum *argv, MochaDatum *rval)
{
    *rval = MOCHA_empty;	/* XXX make tainted string of whole doc */
    return MOCHA_TRUE;
}

char *
LM_GetBaseHrefTag(MochaContext *mc)
{
    MochaDecoder *running;
    char *new_origin_url;
    const char *origin_url;
    const char *cp, *qp, *last_slash;
    char *tag;

    running = MOCHA_GetGlobalObject(mc)->data;
    if (!running->origin_url && !lm_GetOriginURL(mc, running))
	return 0;
    origin_url = running->origin_url;
    new_origin_url = 0;
    cp = origin_url;
    if ((qp = XP_STRCHR(cp, '"')) != 0) {
	do {
	    new_origin_url = PR_sprintf_append(new_origin_url, "%.*s%%%x",
					       qp - cp, cp, *qp);
	    cp = qp + 1;
	} while ((qp = XP_STRCHR(cp, '"')) != 0);
	new_origin_url = PR_sprintf_append(new_origin_url, "%s", cp);
	if (!new_origin_url) {
	    MOCHA_ReportOutOfMemory(mc);
	    return 0;
	}
	origin_url = new_origin_url;
    }
    last_slash = XP_STRRCHR(origin_url, '/');
    if (last_slash) {
	tag = PR_smprintf("<BASE HREF=\"%.*s/\">\n",
			  last_slash - origin_url, origin_url);
    } else {
	tag = PR_smprintf("<BASE HREF=\"%s\">\n", origin_url);
    }
    PR_FREEIF(new_origin_url);
    if (!tag)
	MOCHA_ReportOutOfMemory(mc);
    return tag;
}

static int
doc_write_stream(MochaContext *mc, MochaDecoder *decoder,
		 NET_StreamClass *stream, char *str, size_t len,
		 uint16 taint)
{
    MochaDecoder *running;
    MochaTaintInfo *info;
    MWContext *context;
    lo_TopState *top_state;
    int32 pre_doc_id;
    int status;

    /* XXX lacking MochaData everywhere, we must impute accumulated taint to
	   the generated document, and check for it in LM_CheckGetURL. */
    decoder->write_taint_accum = MOCHA_GetTaintInfo(mc)->accum;
    MOCHA_MIX_TAINT(mc, decoder->write_taint_accum, taint);
    running = MOCHA_GetGlobalObject(mc)->data;
    MOCHA_MIX_TAINT(mc, decoder->write_taint_accum, running->write_taint_accum);
    info = MOCHA_GetTaintInfo(decoder->mocha_context);
    MOCHA_MIX_TAINT(mc, info->accum, decoder->write_taint_accum);

    context = decoder->window_context;
    top_state = lo_GetMochaTopState(context);
    if (top_state)
	top_state->mocha_write_level++;
    pre_doc_id = XP_DOCID(context);
    status = (*stream->put_block)(stream->data_object, str, len);
    if (top_state && XP_DOCID(context) == pre_doc_id)
	top_state->mocha_write_level--;
    return status;
}

#ifdef MOCHA_CACHES_WRITES
static char *
context_pathname(MochaContext *mc, MWContext *context)
{
    MWContext *parent;
    int count, index;
    char *name = 0;

    while ((parent = context->grid_parent) != 0) {
	if (context->name) {
	    name = PR_sprintf_append(name, "%s.", context->name);
	} else {
	    /* XXX silly xp_cntxt.c puts newer contexts at the front! */
	    count = XP_ListCount(parent->grid_children);
	    index = XP_ListGetNumFromObject(parent->grid_children, context);
	    name = PR_sprintf_append(name, "%u.", count - index);
	}
	context = parent;
    }
    name = PR_sprintf_append(name, "%ld", (long)XP_DOCID(context));
    if (!name)
	MOCHA_ReportOutOfMemory(mc);
    return name;
}

/* Make a wysiwyg: URL containing the context and our origin. */
static char *
make_wysiwyg_url(MochaContext *mc, MochaDecoder *decoder)
{
    MochaDecoder *running;
    const char *pathname;
    char *url_string;

    running = MOCHA_GetGlobalObject(mc)->data;
    if (!running->origin_url && !lm_GetOriginURL(mc, running))
	return 0;
    pathname = context_pathname(mc, decoder->window_context);
    if (!pathname)
	return 0;
    url_string = PR_smprintf("wysiwyg://%s/%s", pathname, running->origin_url);
    XP_FREE((char *)pathname);
    if (!url_string) {
	MOCHA_ReportOutOfMemory(mc);
	return 0;
    }
    return url_string;
}

NET_StreamClass *
doc_cache_converter(MochaContext *mc, MochaDecoder *decoder,
		    URL_Struct *url_struct, char *wysiwyg_url)
{
    MWContext *context;
    lo_TopState *top_state;
    char *address;
    NET_StreamClass *cache_stream;
    History_entry *he;

    /* If we don't have a top state, layout must have run out of memory. */
    context = decoder->window_context;
    top_state = lo_GetMochaTopState(context);
    if (!top_state) {
	MOCHA_ReportOutOfMemory(mc);
	return 0;
    }

    /* Save a wysiwyg copy in the URL struct for resize-reloads. */
    url_struct->wysiwyg_url = XP_STRDUP(wysiwyg_url);
    if (!url_struct->wysiwyg_url) {
	MOCHA_ReportOutOfMemory(mc);
	return 0;
    }

    /* Then pass it via url_struct to create a cache converter stream. */
    address = url_struct->address;
    url_struct->address = url_struct->wysiwyg_url;
    cache_stream = NET_CacheConverter(FO_CACHE_ONLY,
				      (void *)1,    /* XXX don't hold url */
				      url_struct,
				      context);
    url_struct->address = address;

    /* Finally, keep a strong (freeing) link to it in our history entry. */
    he = SHIST_GetCurrent(&context->hist);
    if (he && cache_stream) {
	PR_FREEIF(he->wysiwyg_url);
	he->wysiwyg_url = wysiwyg_url;
    }

    top_state->mocha_write_stream = cache_stream;
    return cache_stream;
}

static char nscp_open_tag[] = "<" PT_NSCP_OPEN ">";

NET_StreamClass *
LM_WysiwygCacheConverter(MWContext *context, URL_Struct *url_struct,
			 NET_StreamClass *stream)
{
    MochaDecoder *decoder;
    MochaContext *mc;
    char *wysiwyg_url, *tag;
    NET_StreamClass *cache_stream;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return 0;

    /* Feed layout an internal tag so it will create a new top state. */
    if (stream) {
	stream->put_block(stream->data_object, nscp_open_tag,
			  sizeof nscp_open_tag - 1);
    }
    mc = decoder->mocha_context;
    wysiwyg_url = make_wysiwyg_url(mc, decoder);
    if (!wysiwyg_url) {
	cache_stream = 0;
    } else {
	cache_stream = doc_cache_converter(mc, decoder, url_struct,
					   wysiwyg_url);
	if (!cache_stream) {
	    XP_FREE(wysiwyg_url);
	} else {
	    /* Wysiwyg files need a base tag that removes that URL prefix. */
	    tag = LM_GetBaseHrefTag(mc);
	    if (tag) {
		(void) cache_stream->put_block(cache_stream->data_object, tag,
					       XP_STRLEN(tag));
		XP_FREE(tag);
	    }
	}
    }
    LM_PutMochaDecoder(decoder);
    return cache_stream;
}
#endif /* MOCHA_CACHES_WRITES */

static NET_StreamClass *
doc_open_stream(MochaContext *mc, MochaDecoder *decoder, const char *mime_type,
		MochaBoolean replace)
{
    MWContext *context;
    char *wysiwyg_url, *tag;
    URL_Struct *url_struct, *cached_url;
    MochaDecoder *running;
    History_entry *he;
    NET_StreamClass *stream;
    MochaBoolean is_text_html;

    context = decoder->window_context;
    if (!context) return 0;

    /* We must be called after any loading URL has finished. */
    XP_ASSERT(!decoder->url_struct);
    wysiwyg_url = make_wysiwyg_url(mc, decoder);
    if (!wysiwyg_url)
	return 0;
    url_struct = NET_CreateURLStruct(wysiwyg_url, NET_DONT_RELOAD);
    if (!url_struct) {
	XP_FREE(wysiwyg_url);
	MOCHA_ReportOutOfMemory(mc);
	return 0;
    }

    /* Set content type so it can be cached. */
    StrAllocCopy(url_struct->content_type, mime_type);
    url_struct->preset_content_type = TRUE;
    url_struct->must_cache = TRUE;

    /* Set these to null so we can goto bad from here onward. */
    stream = 0;
    cached_url = 0;

    /* If the writer is secure, pass its security info into the cache. */
    running = MOCHA_GetGlobalObject(mc)->data;
    he = SHIST_GetCurrent(&running->window_context->hist);
#if HAVE_SECURITY /* jwz */
    if (he && he->security_on) {
	/* Copy security stuff (checking for malloc failure) */
	url_struct->security_on = he->security_on;
	StrAllocCopy(url_struct->key_cipher, he->key_cipher);
	if (he->key_cipher && !url_struct->key_cipher)
	    goto bad;
	url_struct->key_size = he->key_size;
	url_struct->key_secret_size = he->key_secret_size;

	/* Make a url struct just to lookup a cached certificate. */
	cached_url = NET_CreateURLStruct(he->address, NET_DONT_RELOAD);
	if (!cached_url)
	    goto bad;
	(void) NET_FindURLInCache(cached_url, running->window_context);
	if (cached_url->certificate) {
	    url_struct->certificate
		= CERT_DupCertificate(cached_url->certificate);
	    if (!url_struct->certificate)
		goto bad;
	}
	NET_FreeURLStruct(cached_url);
	cached_url = 0;
    }
#endif /* HAVE_SECURITY */

    /* Free any old doc before stream building, which may discard too. */
    LO_DiscardDocument(context);
    if (replace) {
	/* If replacing, flag current entry and url_struct specially. */
	lm_ReplaceURL(context, url_struct);
    }

    /* We must be called after any open stream has been closed. */
    XP_ASSERT(!decoder->stream);
    stream = NET_StreamBuilder(FO_PRESENT, url_struct, context);
    if (!stream) {
	MOCHA_ReportError(mc, "cannot open document for %s", mime_type);
	goto bad;
    }

    /* Layout discards documents lazily, but we are now eager. */
    if (!decoder->document && !lm_InitWindowContent(decoder))
	goto bad;
    decoder->writing_input = MOCHA_TRUE;
    if (!lm_SetInputStream(mc, decoder, stream, url_struct))
	goto bad;

    is_text_html =  !XP_STRCMP(mime_type, TEXT_HTML);
#ifdef MOCHA_CACHES_WRITES
    if (is_text_html) {
	/* Feed layout an internal tag so it will create a new top state. */
	stream->put_block(stream->data_object, nscp_open_tag,
			  sizeof nscp_open_tag - 1);
    }

    if ((is_text_html || !XP_STRCMP(mime_type, TEXT_PLAIN)) &&
	!doc_cache_converter(mc, decoder, url_struct, wysiwyg_url)) {
	XP_FREE(wysiwyg_url);
	wysiwyg_url = 0;
    }
#else
    XP_FREE(wysiwyg_url);
    wysiwyg_url = 0;
#endif

    /* Auto-generate a BASE HREF= tag for generated HTML documents. */
    if (is_text_html) {
	tag = LM_GetBaseHrefTag(mc);
	if (!tag)
	    goto bad;
	(void) doc_write_stream(mc, decoder, stream, tag, XP_STRLEN(tag),
				MOCHA_TAINT_IDENTITY);
	XP_FREE(tag);
    }
    return stream;

bad:
    decoder->writing_input = MOCHA_FALSE;
    decoder->stream = 0;
    decoder->url_struct = 0;
    if (stream) {
	(*stream->abort)(stream->data_object, MK_UNABLE_TO_CONVERT);
	XP_DELETE(stream);
    }
    NET_FreeURLStruct(url_struct);
    if (cached_url)
	NET_FreeURLStruct(cached_url);
    PR_FREEIF(wysiwyg_url);
    return 0;
}

static MochaBoolean
doc_stream_active(MochaDecoder *decoder)
{
    MWContext *context;
    lo_TopState *top_state;

    if (!decoder->writing_input)
	return MOCHA_TRUE;
    context = decoder->window_context;
    if (context) {
	top_state = lo_GetMochaTopState(context);
	if (top_state && top_state->mocha_write_level)
	    return MOCHA_TRUE;
    }
    return MOCHA_FALSE;
}

/* XXX shared from lm_win.c for compatibility hack */
extern MochaBoolean
win_open(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval);

static MochaBoolean
doc_open(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDocument *doc;
    MochaDecoder *decoder;
    MochaAtom *atom, *atom2;
    const char *mime_type, *options;
    MochaBoolean ok, replace;
    NET_StreamClass *stream;

    if (!MOCHA_InstanceOf(mc, obj, &doc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    doc = obj->data;
    decoder = doc->decoder;

    if (argc > 2 ||
	(argv[0].tag == MOCHA_STRING && argv[0].u.atom == MOCHA_empty.u.atom)) {
	/* XXX hideous compatibility hack to call window.open */
	obj = MOCHA_HoldObject(mc, decoder->window_object);
	ok = win_open(mc, obj, argc, argv, rval);
	MOCHA_DropObject(mc, obj);
	return ok;
    }

    atom = atom2 = 0;
    mime_type = TEXT_HTML;
    replace = MOCHA_FALSE;
    if (argc >= 1) {
	if (!MOCHA_DatumToString(mc, argv[0], &atom))
	    return MOCHA_FALSE;
	mime_type = MOCHA_GetAtomName(mc, atom);
	if (argc >= 2) {
	    if (!MOCHA_DatumToString(mc, argv[1], &atom2)) {
		MOCHA_DropAtom(mc, atom);
		return MOCHA_FALSE;
	    }
	    options = MOCHA_GetAtomName(mc, atom2);
	    replace = !XP_STRCASECMP(options, "replace");
	}
    }
    stream = decoder->stream;
    if (stream) {
	if (doc_stream_active(decoder)) {
	    /* Don't close a stream being fed by netlib. */
	    *rval = MOCHA_null;
	    MOCHA_DropAtom(mc, atom);
	    MOCHA_DropAtom(mc, atom2);
	    return MOCHA_TRUE;
	}
	LM_ClearDecoderStream(decoder);
    }
    stream = doc_open_stream(mc, decoder, mime_type, replace);
    MOCHA_DropAtom(mc, atom);
    MOCHA_DropAtom(mc, atom2);
    if (!stream)
	return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, rval, MOCHA_OBJECT, u.obj, doc->object);
    return MOCHA_TRUE;
}

static MochaBoolean
doc_close(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDocument *doc;
    MochaDecoder *decoder;

    if (!MOCHA_InstanceOf(mc, obj, &doc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    doc = obj->data;
    decoder = doc->decoder;
    if (!decoder->stream || doc_stream_active(decoder)) {
	/* Don't close a closed stream, or a stream being fed by netlib. */
	*rval = MOCHA_false;
	return MOCHA_TRUE;
    }
    LM_ClearDecoderStream(decoder);
    *rval = MOCHA_true;
    return MOCHA_TRUE;
}

static MochaBoolean
doc_clear(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDocument *doc;
    MochaDecoder *decoder;
    MWContext *context;

    if (!MOCHA_InstanceOf(mc, obj, &doc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    doc = obj->data;
    decoder = doc->decoder;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    if (!doc_close(mc, obj, argc, argv, rval))
	return MOCHA_FALSE;
    /* XXX need to open and push a tag through layout? */
    FE_ClearView(context, FE_VIEW);
    return MOCHA_TRUE;
}

static NET_StreamClass *
doc_get_stream(MochaContext *mc, MochaDecoder *decoder)
{
    NET_StreamClass *stream;
    URL_Struct *url_struct;
    MWContext *context;
    lo_TopState *top_state;

    stream = decoder->stream;
    if (!stream) {
	stream = doc_open_stream(mc, decoder, TEXT_HTML, MOCHA_FALSE);
    } else {
	url_struct = decoder->url_struct;
	XP_ASSERT(url_struct);
	if (url_struct && !url_struct->wysiwyg_url) {
	    context = decoder->window_context;
	    if (context) {
		top_state = lo_GetMochaTopState(context);
		if (top_state && !top_state->mocha_write_stream) {
#ifdef MOCHA_CACHES_WRITES
		    top_state->mocha_write_stream
			= NET_CloneWysiwygCacheFile(context, url_struct,
					    (uint32)top_state->script_bytes);
#endif
		}
	    }
	}
    }
    return stream;
}

static MochaBoolean
doc_write(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDocument *doc;
    MochaDecoder *decoder;
    NET_StreamClass *stream;
    uint i;
    MochaAtom *atom;
    char *str;
    int status;

    if (!MOCHA_InstanceOf(mc, obj, &doc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    doc = obj->data;
    decoder = doc->decoder;
    stream = doc_get_stream(mc, decoder);
    if (!stream)
	return MOCHA_FALSE;
    for (i = 0; i < argc; i++) {
	if (!MOCHA_DatumToString(mc, argv[i], &atom))
	    return MOCHA_FALSE;
	/* XXX dup it just so libparse can gratuitously overwrite it */
	str = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom));
	MOCHA_DropAtom(mc, atom);
	if (!str)
	    return MOCHA_FALSE;
	status = doc_write_stream(mc, decoder, stream, str, XP_STRLEN(str),
				  argv[i].taint);
	MOCHA_free(mc, str);
	if (status < 0) {
	    *rval = MOCHA_false;
	    return MOCHA_TRUE;
	}
    }
    if (!decoder->document && !lm_InitWindowContent(decoder))
	return MOCHA_FALSE;
    *rval = MOCHA_true;
    return MOCHA_TRUE;
}

static MochaBoolean
doc_writeln(MochaContext *mc, MochaObject *obj,
	    uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDocument *doc;
    MochaDecoder *decoder;
    NET_StreamClass *stream;
    int status;
    static char eol[] = "\n";

    if (!MOCHA_InstanceOf(mc, obj, &doc_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    doc = obj->data;
    decoder = doc->decoder;
    stream = doc_get_stream(mc, decoder);
    if (!stream)
	return MOCHA_FALSE;
    if (!doc_write(mc, obj, argc, argv, rval))
	return MOCHA_FALSE;
    status = doc_write_stream(mc, decoder, stream, eol, sizeof eol - 1,
			      MOCHA_TAINT_IDENTITY);
    if (status < 0)
	rval->u.bval = MOCHA_FALSE;
    return MOCHA_TRUE;
}

static MochaFunctionSpec doc_methods[] = {
    {"clear",           doc_clear,      0},
    {"close",           doc_close,      0},
    {"open",            doc_open,       1},
    {lm_toString_str,   doc_to_string,  0},
    {"write",           doc_write,      0},
    {"writeln",         doc_writeln,    0},
    {0}
};

MochaObject *
lm_DefineDocument(MochaDecoder *decoder)
{
    MochaObject *obj;
    MochaContext *mc;
    MochaDocument *doc;

    obj = decoder->document;
    if (obj)
	return obj;
    mc = decoder->mocha_context;
    doc = MOCHA_malloc(mc, sizeof *doc);
    if (!doc)
	return 0;
    obj = MOCHA_InitClass(mc, decoder->window_object, &doc_class, doc,
			  Document, 0, doc_props, doc_methods, 0, 0);
    if (!obj) {
	MOCHA_free(mc, doc);
	return 0;
    }
    if (!MOCHA_DefineObject(mc, decoder->window_object, "document", obj,
			    MDF_ENUMERATE | MDF_READONLY)) {
	return 0;
    }
    XP_BZERO(doc, sizeof *doc);
    doc->object = obj;
    doc->decoder = decoder;
    decoder->document = MOCHA_HoldObject(mc, obj);
    obj->parent = decoder->window_object;
    LM_LINK_OBJECT(decoder, obj, "document");
    return obj;
}

void
LM_ReleaseDocument(MWContext *context, MochaBoolean resize_reload)
{
    MochaDecoder *decoder;
    MochaContext *mc;

    if (resize_reload)
	return;

    /* Be defensive about Mocha-unaware contexts. */
    mc = context->mocha_context;
    if (!mc)
	return;

    /* Hold the context's decoder rather than calling LM_GetMochaDecoder. */
    decoder = MOCHA_GetGlobalObject(mc)->data;
    MOCHA_HoldObject(mc, decoder->window_object);

    lm_FreeWindowContent(decoder);
    LM_PutMochaDecoder(decoder);
}
