/*
** Mocha in the Navigator top-level stuff.
**
** Brendan Eich, 9/8/95
*/
#include "lm.h"
#include "xp.h"
#include "fe_proto.h"
#include "net.h"
#include "structs.h"
#include "ds.h"		/* XXX required by htmldlgs.h */
#include "htmldlgs.h"
#include "mkutils.h"	/* XXX for NET_URL_Type() */
#include "pa_tags.h"	/* included via -I../libparse */
#include "prmem.h"
#include "prprf.h"

char mocha_language_name[]  = "JavaScript";
char mocha_content_type[]   = APPLICATION_JAVASCRIPT;

char lm_onLoad_str[]        = PARAM_ONLOAD;
char lm_onUnload_str[]      = PARAM_ONUNLOAD;
char lm_onAbort_str[]       = PARAM_ONABORT;
char lm_onError_str[]       = PARAM_ONERROR;
char lm_onScroll_str[]      = PARAM_ONSCROLL;
char lm_onFocus_str[]       = PARAM_ONFOCUS;
char lm_onBlur_str[]        = PARAM_ONBLUR;
char lm_onSelect_str[]      = PARAM_ONSELECT;
char lm_onChange_str[]      = PARAM_ONCHANGE;
char lm_onReset_str[]       = PARAM_ONRESET;
char lm_onSubmit_str[]      = PARAM_ONSUBMIT;
char lm_onClick_str[]       = PARAM_ONCLICK;
char lm_onMouseOver_str[]   = PARAM_ONMOUSEOVER;
char lm_onMouseOut_str[]    = PARAM_ONMOUSEOUT;

char lm_focus_str[]         = "focus";
char lm_blur_str[]          = "blur";
char lm_select_str[]        = "select";
char lm_click_str[]         = "click";
char lm_scroll_str[]        = "scroll";
char lm_enable_str[]        = "enable";
char lm_disable_str[]       = "disable";

char lm_toString_str[]      = "toString";
char lm_length_str[]        = "length";
char lm_forms_str[]         = "forms";
char lm_links_str[]         = "links";
char lm_anchors_str[]       = "anchors";
char lm_plugins_str[]       = "plugins";
char lm_applets_str[]       = "applets";
char lm_embeds_str[]        = "embeds";
char lm_images_str[]        = "images";
char lm_location_str[]      = "location";

char lm_opener_str[]        = "opener";
char lm_closed_str[]        = "closed";
char lm_assign_str[]        = "assign";
char lm_reload_str[]        = "reload";
char lm_replace_str[]       = "replace";

MochaDecoder *lm_global_decoder;
MochaDecoder *lm_crippled_decoder;
MochaContext *lm_crippled_context;	/* exported to jsStubs.c */
MochaContext *lm_running_context;

static MochaClass lm_dummy_class = {
    "Dummy",
    MOCHA_PropertyStub, MOCHA_PropertyStub, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, MOCHA_FinalizeStub
};
MochaObject *lm_DummyObject;

static MochaBoolean
lm_alert(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaDecoder *decoder;
    MochaAtom *arg;

    if (!MOCHA_InstanceOf(mc, obj, &lm_window_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    decoder = obj->data;
    if (decoder->window_context) {
	if (!MOCHA_DatumToString(mc, argv[0], &arg))
	    return MOCHA_FALSE;
	FE_Alert(decoder->window_context, MOCHA_GetAtomName(mc, arg));
	MOCHA_DropAtom(mc, arg);
    }
    return MOCHA_TRUE;
}

#ifdef DEBUG

#include "mo_cntxt.h"

static MochaBoolean
lm_tracing(MochaContext *mc, MochaObject *obj,
	   uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaBoolean bval;

    if (argc == 0) {
	MOCHA_INIT_DATUM(mc, rval, MOCHA_BOOLEAN, u.bval, (mc->tracefp != 0));
	return MOCHA_TRUE;
    }

    switch (argv[0].tag) {
      case MOCHA_NUMBER:
	bval = argv[0].u.fval != 0;
	break;
      case MOCHA_BOOLEAN:
	bval = argv[0].u.bval;
	break;
      default:
	return MOCHA_TRUE;	/* XXX should be error */
	break;
    }
    if (mc->tracefp)
	fclose(mc->tracefp);
    mc->tracefp = bval ? fopen("/dev/tty", "w") : 0;
    return MOCHA_TRUE;
}

#endif /* DEBUG */

static MochaFunctionSpec lm_global_functions[] = {
    {"alert",           lm_alert,       0},
#ifdef DEBUG
    {"tracing",         lm_tracing,     0},
#endif
    {0}
};

MochaBoolean
lm_BranchCallback(MochaContext *mc, MochaScript *script)
{
    MochaDecoder *decoder;
    char *message;
    MochaTaintInfo *info;
    MochaBoolean ok = MOCHA_TRUE;

    decoder = MOCHA_GetGlobalObject(mc)->data;
    if (decoder->window_context && ++decoder->branch_count == 1000000) {
	decoder->branch_count = 0;
	message = PR_smprintf("Lengthy %s still running.  Continue?",
			      mocha_language_name);
	if (message) {
	    ok = FE_Confirm(decoder->window_context, message);
	    XP_FREE(message);
	}
    }
    info = MOCHA_GetTaintInfo(mc);
    MOCHA_MIX_TAINT(mc, lm_branch_taint, info->accum);
    return ok;
}

#define ERROR_REPORT_LIMIT	10

static char lm_onerror_str[] = "onerror";

void
lm_ErrorReporter(MochaContext *mc, const char *message,
		 MochaErrorReport *report)
{
    MochaObject *obj;
    MochaDatum d, argv[3];
    char *last;
    MochaDecoder *decoder;
    int i, j, k, n;
    const char *s, *t;

    obj = MOCHA_GetGlobalObject(mc);
    decoder = obj->data;
    decoder->error_count++;

    if (MOCHA_LookupName(mc, obj, lm_onerror_str, &d)) {
	if (MOCHA_DATUM_IS_NULL(d)) {
	    /* User has set onerror to null, so cancel this report. */
	    return;
	}
	if (d.tag == MOCHA_FUNCTION) {
	    MOCHA_INIT_FULL_DATUM(mc, &argv[0], MOCHA_STRING,
				  0, MOCHA_TAINT_IDENTITY, u.atom,
				  MOCHA_Atomize(mc, message));
	    MOCHA_INIT_FULL_DATUM(mc, &argv[1], MOCHA_STRING,
				  0, MOCHA_TAINT_IDENTITY, u.atom,
				  MOCHA_Atomize(mc, report->filename));
	    MOCHA_INIT_FULL_DATUM(mc, &argv[2], MOCHA_NUMBER,
				  0, MOCHA_TAINT_IDENTITY, u.fval,
				  report->lineno);
	    if (MOCHA_CallMethod(mc, obj, lm_onerror_str, 3, argv, &d) &&
		d.tag == MOCHA_BOOLEAN && d.u.bval == MOCHA_TRUE) {
		/* True return value means the function handled this error. */
		return;
	    }
	}
    }

    if (decoder->error_count >= ERROR_REPORT_LIMIT) {
	if (decoder->error_count == ERROR_REPORT_LIMIT) {
	    last = PR_smprintf("too many %s errors", mocha_language_name);
	    if (last) {
		FE_Alert(decoder->window_context, last);
		XP_FREE(last);
	    }
	}
	return;
    }

    last = PR_sprintf_append(0, "<FONT SIZE=4>\n<B>%s Error:</B> ",
			     mocha_language_name);
    if (!report) {
	last = PR_sprintf_append(last, "<B>%s</B>\n", message);
    } else {
	if (report->filename)
	    last = PR_sprintf_append(last, "<A HREF=\"%s\">%s</A>, ",
				     report->filename, report->filename);
	if (report->lineno)
	    last = PR_sprintf_append(last, "<B>line %u:</B>", report->lineno);
	last = PR_sprintf_append(last,
				 "<BR><BR>%s.</FONT><PRE><FONT SIZE=4>",
				 message);
	if (report->linebuf) {
	    for (s = t = report->linebuf; *s != '\0'; s = t) {
		for (; t != report->tokenptr && *t != '<' && *t != '\0'; t++)
		    ;
		last = PR_sprintf_append(last, "%.*s", t - s, s);
		if (*t == '\0')
		    break;
		if (t == report->tokenptr) {
		    last = PR_sprintf_append(last,
					     "</FONT>"
					     "<FONT SIZE=4 COLOR=#ff2020>");
		}
		last = PR_sprintf_append(last, (*t == '<') ? "&lt;" : "%c", *t);
		t++;
	    }
	    last = PR_sprintf_append(last, "</FONT><FONT SIZE=4>\n");
	    n = report->tokenptr - report->linebuf;
	    for (i = j = 0; i < n; i++) {
		if (report->linebuf[i] == '\t') {
		    for (k = (j + 8) & ~7; j < k; j++)
			last = PR_sprintf_append(last, ".");
		    continue;
		}
		last = PR_sprintf_append(last, ".");
		j++;
	    }
	    last = PR_sprintf_append(last, "<B>^</B>");
	}
	last = PR_sprintf_append(last, "\n</FONT></PRE>");
    }

    if (last) {
	if (decoder->window_context)
	    XP_MakeHTMLAlert(decoder->window_context, last);
	XP_FREE(last);
    }
}

static MochaDecoder *
lm_new_decoder(MochaClass *clazz)
{
    MochaContext *mc;
    MochaDecoder *decoder;
    MochaObject *obj;

    decoder = XP_NEW_ZAP(MochaDecoder);
    if (!decoder)
	return 0;

/* XXX begin common */
    mc = MOCHA_NewContext(LM_STACK_SIZE);
    if (!mc) {
	XP_DELETE(decoder);
	return 0;
    }

    decoder->mocha_context = mc;
    MOCHA_SetBranchCallback(mc, lm_BranchCallback);
    MOCHA_SetErrorReporter(mc, lm_ErrorReporter);

    obj = MOCHA_NewObject(mc, clazz, decoder, 0, 0, 0, 0);
    if (!obj) {
	MOCHA_DestroyContext(mc);
	XP_DELETE(decoder);
	return 0;
    }

    decoder->window_context = 0;
    decoder->window_object = MOCHA_HoldObject(mc, obj);

    MOCHA_SetGlobalObject(mc, obj);
/* XXX end common */

    MOCHA_DefineFunctions(mc, obj, lm_global_functions);
    lm_DefineNavigator(decoder);

    return decoder;
}

/* XXX return boolean to propagate errors. */
void
LM_InitMocha(void)
{
    /* Initialize the global Mocha decoder used by proxy autoconfig. */
    lm_global_decoder = lm_new_decoder(&lm_window_class);

    /* Initialize a crippled decoder/context for use by Java. */
    lm_crippled_decoder = lm_new_decoder(&lm_dummy_class);
    lm_crippled_context = lm_crippled_decoder->mocha_context;

    /* Initialize a dummy object used for unreflectable applets and embeds. */
    lm_DummyObject = lm_crippled_decoder->window_object;

    /* Initialize data tainting using the crippled context. */
    (void) lm_InitTaint(lm_crippled_context);

    /* Associate a Mocha netlib "converter" with our mime type. */
    NET_RegisterContentTypeConverter(mocha_content_type, FO_PRESENT, 0,
				     NET_CreateMochaConverter);
}

void
LM_FinishMocha(void)
{
    MochaDecoder *decoder;

    decoder = lm_global_decoder;
    if (decoder) {
	MOCHA_DropObject(decoder->mocha_context, decoder->window_object);
	lm_global_decoder = 0;
    }

    decoder = lm_crippled_decoder;
    if (decoder) {
	MOCHA_DropObject(decoder->mocha_context, decoder->window_object);
	lm_crippled_decoder = 0;
    }
}

/*
** Enable or disable local Mocha decoding.
*/
static MochaBoolean lm_disabled = MOCHA_FALSE;

void
LM_SetMochaEnabled(MochaBoolean toggle)
{
    lm_disabled = !toggle;
}

MochaBoolean
LM_GetMochaEnabled(void)
{
    return !lm_disabled;
}

MochaDecoder *
LM_GetGlobalMochaDecoder(MWContext *context)
{
    MochaDecoder *decoder;

    decoder = lm_global_decoder;
    if (decoder) {
	decoder->window_context = context;
	MOCHA_HoldObject(decoder->mocha_context, decoder->window_object);
    }
    return decoder;
}

void
LM_PutGlobalMochaDecoder(MochaDecoder *decoder)
{
    XP_ASSERT(decoder == lm_global_decoder);
    decoder->window_context = 0;
    decoder->window_object = MOCHA_DropObject(decoder->mocha_context,
					      decoder->window_object);
}

MochaDecoder *
LM_GetMochaDecoder(MWContext *context)
{
    MochaContext *mc;
    MochaDecoder *decoder;

    if (lm_disabled)
	return 0;

    /* Get the context's Mocha decoder, creating one if necessary. */
    mc = context->mocha_context;
    if (mc) {
	decoder = MOCHA_GetGlobalObject(mc)->data;
    } else {
	decoder = lm_NewWindow(context);
	if (!decoder)
	    return 0;
	mc = decoder->mocha_context;
    }
    if (!decoder->document && !lm_InitWindowContent(decoder))
	return 0;
    MOCHA_HoldObject(mc, decoder->window_object);
    return decoder;
}

void
LM_PutMochaDecoder(MochaDecoder *decoder)
{
    MOCHA_DropObject(decoder->mocha_context, decoder->window_object);
}

const char *
LM_GetSourceURL(MochaDecoder *decoder)
{
    MochaContext *mc;
    
    if (!decoder->source_url) {
	mc = decoder->mocha_context;
	if (!decoder->origin_url && !lm_GetOriginURL(mc, decoder))
	    return 0;
	decoder->source_url = MOCHA_strdup(mc, decoder->origin_url);
	if (!decoder->source_url)
	    return 0;
    }
    return decoder->source_url;
}

MochaBoolean
LM_EvaluateBuffer(MochaDecoder *decoder, char *base, size_t length,
		  uint lineno, MochaDatum *result)
{
    MochaContext *mc, *save;
    const char *source_url;
    MochaBoolean ok;

    /* XXX if lineno == 0, do something smart */
    source_url = LM_GetSourceURL(decoder);
    if (!source_url)
	return MOCHA_FALSE;
    mc = decoder->mocha_context;
    save = lm_running_context;
    lm_running_context = mc;
    decoder->eval_level++;
    ok = MOCHA_EvaluateBuffer(mc, decoder->window_object,
                              base, length, source_url, lineno,
			      result);
    if (--decoder->eval_level == 0)
	decoder->branch_count = 0;
    lm_running_context = save;
    return ok;
}

char *
LM_EvaluateAttribute(MWContext *context, char *expr, uint lineno)
{
    char *string;
    MochaDecoder *decoder;
    MochaDatum result;
    MochaContext *mc;
    MochaAtom *atom;

    string = 0;
    if (!expr)
	return string;
    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return string;
    if (LM_EvaluateBuffer(decoder, expr, XP_STRLEN(expr), lineno, &result)) {
	mc = decoder->mocha_context;
	if (MOCHA_DatumToString(mc, result, &atom)) {
	    string = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom));
	    if (string)
		MOCHA_MIX_TAINT(mc, decoder->write_taint_accum, result.taint);
	    MOCHA_DropAtom(mc, atom);
	}
	MOCHA_DropRef(mc, &result);
    }
    LM_PutMochaDecoder(decoder);
    return string;
}


MochaBoolean
lm_SendEvent(MWContext *context, MochaObject *obj, char *name,
	     MochaDatum *result)
{
    MochaContext *mc, *save;
    MochaDecoder *decoder;
    MochaBoolean ok;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
	return MOCHA_FALSE;
    mc = decoder->mocha_context;
    if (MOCHA_LookupName(mc, obj, name, result) &&
	MOCHA_DATUM_IS_NULL(*result)) {
	return MOCHA_TRUE;
    }
    save = lm_running_context;
    lm_running_context = mc;
    decoder->eval_level++;
    ok = MOCHA_CallMethod(mc, obj, name, 0, 0, result);
    if (--decoder->eval_level == 0)
	decoder->branch_count = 0;
    lm_running_context = save;
    LM_PutMochaDecoder(decoder);
    return ok;
}

/*
** Wrapper for the common case of decoder's stream being set by code running
** on decoder's Mocha context.
*/
MochaBoolean
LM_SetDecoderStream(MochaDecoder *decoder, NET_StreamClass *stream,
		     URL_Struct *url_struct)
{
    return lm_SetInputStream(decoder->mocha_context, decoder, stream,
			     url_struct);
}

MochaBoolean
lm_SetInputStream(MochaContext *mc, MochaDecoder *decoder,
		  NET_StreamClass *stream, URL_Struct *url_struct)
{
    const char *origin_url;
    MochaDecoder *running;

    decoder->stream = stream;
    decoder->url_struct = url_struct;

    /* Don't update origin for <SCRIPT SRC="URL"> URLs. */
    if (decoder->nesting_url)
	return MOCHA_TRUE;

    /* Update the origin URL associated with the document in decoder. */
    if (!decoder->writing_input) {
	switch (NET_URL_Type(url_struct->address)) {
	  case ABOUT_TYPE_URL:
	  case MOCHA_TYPE_URL:
	    return MOCHA_TRUE;
	}
	origin_url = url_struct->address;
    } else {
	running = MOCHA_GetGlobalObject(mc)->data;
	if (!running)
	    return MOCHA_TRUE;
	if (!running->origin_url && !lm_GetOriginURL(mc, running))
	    return MOCHA_FALSE;
	origin_url = running->origin_url;
    }

    /* Change decoder->origin_url iff it is a different string. */
    if (!decoder->origin_url || XP_STRCMP(decoder->origin_url, origin_url)) {
	origin_url = lm_SetOriginURL(mc, decoder, origin_url);
	if (!origin_url)
	    return MOCHA_FALSE;
    }

    if (!lm_SetTaint(mc, decoder, origin_url))
	return MOCHA_FALSE;
    return MOCHA_TRUE;
}

void
LM_ClearDecoderStream(MochaDecoder *decoder)
{
    NET_StreamClass *stream;
    URL_Struct *url_struct;

    stream = decoder->stream;
    url_struct = decoder->url_struct;
    decoder->stream = 0;
    decoder->url_struct = 0;
    if (decoder->writing_input) {
	decoder->writing_input = MOCHA_FALSE;
	if (stream) {
	    /*
	     * Complete the stream before freeing the URL struct to which it
	     * may hold a private pointer.
	     */
	    (*stream->complete)(stream->data_object);
	    XP_DELETE(stream);
	}
	if (url_struct) {
	    if (url_struct->pre_exit_fn && decoder->window_context) {
		/* Chain to the next URL, which was blocked by this one. */
		url_struct->pre_exit_fn(url_struct, 0, decoder->window_context);
	    }
	    NET_FreeURLStruct(url_struct);
	}
    }
}

void
LM_ClearContextStream(MWContext *context)
{
    MochaDecoder *decoder;

    if (!context->mocha_context) {
	/* Don't impose cost on non-Mocha contexts. */
	return;
    }
    decoder = LM_GetMochaDecoder(context);
    if (!decoder) {
	/* XXX how can this happen?  If lm_InitWindow fails or if the user
	       turns off JS in the middle of a load.  Return false! */
	return;
    }
    LM_ClearDecoderStream(decoder);
    LM_PutMochaDecoder(decoder);
}

MochaBoolean
lm_SaveParamString(MochaContext *mc, PA_Block *bp, const char *str)
{
    PA_Block block;
    char *newstr;

    /* XXX intimate with layout's allocation and ownership model */
    block = PA_ALLOC((XP_STRLEN(str) + 1) * sizeof(char));
    if (!block) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }
    if (*bp)
	PA_FREE(*bp);
    PA_LOCK(newstr, char *, block);
    XP_STRCPY(newstr, str);
    *bp = block;
    PA_UNLOCK(block);
    return MOCHA_TRUE;
}

#ifdef DEBUG_brendan
#include <stdio.h>
#include "prclist.h"

typedef struct ContextList {
    PRCList         clist;
    MochaContext    *context;
    PRCList         objects;
    int32           count;
} ContextList;

#define CL(cl)	((ContextList *)(cl))

typedef struct ObjectList {
    PRCList         clist;
    MochaObject     *object;
    const char      *name;
} ObjectList;

#define OL(ol)	((ObjectList *)(ol))

static PRCList lm_context_list = PR_INIT_STATIC_CLIST(&lm_context_list);

void
lm_LinkObject(MochaDecoder *decoder, MochaObject *obj, const char *name)
{
    MochaContext *mc = decoder->mocha_context;
    PRCList *cl, *ol;

    for (cl = lm_context_list.next; ; cl = cl->next) {
	if (CL(cl)->context == mc)
	    break;
	if (cl == &lm_context_list) {
	    cl = (PRCList *)PR_NEW(ContextList);
	    PR_APPEND_LINK(cl, &lm_context_list);
	    CL(cl)->context = mc;
	    PR_INIT_CLIST(&CL(cl)->objects);
	    CL(cl)->count = 0;
	    break;
	}
    }
    ol = (PRCList *)PR_NEW(ObjectList);
    PR_APPEND_LINK(ol, &CL(cl)->objects);
    OL(ol)->object = obj;
    OL(ol)->name = name;
    CL(cl)->count++;
}

void
lm_UnlinkObject(MochaDecoder *decoder, MochaObject *obj)
{
    MochaContext *mc = decoder->mocha_context;
    PRCList *cl, *ol;

    for (cl = lm_context_list.next; ; cl = cl->next) {
	if (cl == &lm_context_list)
	    return;
	if (CL(cl)->context == mc)
	    break;
    }
    for (ol = CL(cl)->objects.next; ; ol = ol->next) {
	if (ol == &CL(cl)->objects)
	    return;
	if (OL(ol)->object == obj)
	    break;
    }
    PR_REMOVE_LINK(ol);
    PR_DELETE(ol);
    if (--CL(cl)->count == 0) {
	PR_REMOVE_LINK(cl);
	PR_DELETE(cl);
    }
}

void
lm_DumpLiveObjects(MochaDecoder *decoder)
{
    MochaContext *mc = decoder->mocha_context;
    PRCList *cl, *ol;
    MWContext *context;
    MochaObject *obj;

    for (cl = lm_context_list.next; ; cl = cl->next) {
	if (cl == &lm_context_list)
	    return;
	if (CL(cl)->context == mc)
	    break;
    }
    context = decoder->window_context;
    if (context && context->name) {
	fprintf(stderr, "========== Object dump for window %s ==========\n",
		context->name);
    } else {
	fprintf(stderr, "========== Object dump for context %#x ==========\n",
		mc);
    }
    for (ol = CL(cl)->objects.next; ol != &CL(cl)->objects; ol = ol->next) {
	obj = OL(ol)->object;
	fprintf(stderr,
		"%s (%#x) nrefs %d clazz %s (%#x) data %#x scope (%#x)\n",
		OL(ol)->name ? OL(ol)->name : "[unnamed]",
		obj, obj->nrefs, obj->clazz->name, obj->clazz, obj->data,
		obj->scope);
    }
    fprintf(stderr, "==========\n");
}
#endif
