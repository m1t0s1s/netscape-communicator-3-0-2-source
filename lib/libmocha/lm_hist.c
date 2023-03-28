/*
** Mocha reflection of the Navigator Session History.
**
** Brendan Eich, 9/8/95
*/
#include "lm.h"
#include "xp.h"
#include "fe_proto.h"
#include "shist.h"
#include "structs.h"		/* for MWContext */

enum hist_slot {
    HIST_LENGTH     = -1,
    HIST_CURRENT    = -2,
    HIST_PREVIOUS   = -3,
    HIST_NEXT       = -4
};

static MochaPropertySpec hist_props[] = {
    {lm_length_str,     HIST_LENGTH,    MDF_ENUMERATE|MDF_READONLY},
    {"current",         HIST_CURRENT,   MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {"previous",        HIST_PREVIOUS,  MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {"next",            HIST_NEXT,      MDF_ENUMERATE|MDF_READONLY|MDF_TAINTED},
    {0}
};

typedef struct MochaHistory {
    MochaDecoder    *decoder;
    MochaObject     *object;
} MochaHistory;

/* XXX horrid forward decl of hist_go and early def of hist_forward to work
   XXX around a Win16 compiler bug! */
static MochaBoolean
hist_go(MochaContext *mc, MochaObject *obj,
	unsigned argc, MochaDatum *argv, MochaDatum *rval);

#define DEFER(fun, arg) FE_SetTimeout((void (*)(void *))fun, arg, 0)

static MochaBoolean
hist_forward(MochaContext *mc, MochaObject *obj,
	     unsigned argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaHistory *hist;
    MochaDecoder *decoder;
    MWContext *context;
    MochaDatum plus_one;

    hist = obj->data;
    decoder = hist->decoder;
    context = decoder->window_context;
    if (context->is_grid_cell) {
	while (context->grid_parent)
	    context = context->grid_parent;
	DEFER(LO_ForwardInGrid, context);
	return MOCHA_TRUE;
    }
    MOCHA_INIT_FULL_DATUM(mc, &plus_one, MOCHA_NUMBER, 0, MOCHA_TAINT_IDENTITY,
			  u.fval, 1);
    return hist_go(mc, obj, 1, &plus_one, rval);
}

static MochaBoolean
hist_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
		  MochaDatum *dp)
{
    MochaHistory *hist = obj->data;
    MochaDecoder *decoder;
    MWContext *context;
    History_entry *he, *he2;
    MochaAtom *atom;
    XP_List *histlist;

    decoder = hist->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_TRUE;

    atom = MOCHA_empty.u.atom;
    switch (slot) {
      case HIST_LENGTH:
	histlist = SHIST_GetList(context);
	MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, XP_ListCount(histlist));
	return MOCHA_TRUE;

      case HIST_CURRENT:
	if (!lm_data_tainting) break;
	he = SHIST_GetCurrent(&context->hist);
	if (he && he->address)
	    atom = MOCHA_Atomize(mc, he->address);
	break;

      case HIST_PREVIOUS:
	if (!lm_data_tainting) break;
	/* XXX why doesn't this function take (&context->hist)? */
	he2 = SHIST_GetPrevious(context);
	if (he2 && he2->address)
	    atom = MOCHA_Atomize(mc, he2->address);
	break;

      case HIST_NEXT:
	if (!lm_data_tainting) break;
	/* XXX why doesn't this function take (&context->hist)? */
	he2 = SHIST_GetNext(context);
	if (he2 && he2->address)
	    atom = MOCHA_Atomize(mc, he2->address);
	break;

      default:
	if (slot < 0) {
	    /* Don't mess with user-defined or method properties. */
	    return MOCHA_TRUE;
	}
	if (!lm_data_tainting) break;
	he = SHIST_GetObjectNum(&context->hist, 1 + slot);
	if (he && he->address) {
	    atom = MOCHA_Atomize(mc, he->address);
	    dp->flags |= MDF_TAINTED;
	}
	break;
    }

    /* Common tail code for string-type properties. */
    if (!atom)
	return MOCHA_FALSE;
    MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, atom);
    if (dp->flags & MDF_TAINTED)
	dp->taint = MOCHA_TAINT_SHIST;
    return MOCHA_TRUE;
}

static void
hist_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaHistory *hist = obj->data;

    LM_UNLINK_OBJECT(hist->decoder, obj);
    XP_DELETE(hist);
}

static MochaClass hist_class = {
    "History",
    hist_get_property, hist_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, hist_finalize
};

/* XXX avoid ntypes.h History typedef name clash */
static MochaBoolean
History_ctor(MochaContext *mc, MochaObject *obj,
	     uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
hist_back(MochaContext *mc, MochaObject *obj,
	  unsigned argc, MochaDatum *argv, MochaDatum *rval);

static MochaBoolean
hist_go(MochaContext *mc, MochaObject *obj,
	unsigned argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaHistory *hist;
    MochaDecoder *decoder;
    MWContext *context;
    History_entry *he;
    int index, delta;
    XP_List *histlist;
    URL_Struct *url_struct;

    if (!MOCHA_InstanceOf(mc, obj, &hist_class, argv[-1].u.fun))
        return MOCHA_FALSE;
    hist = obj->data;
    decoder = hist->decoder;
    context = decoder->window_context;
    if (!context) return MOCHA_FALSE;
    he = SHIST_GetCurrent(&context->hist);

    switch (argv[0].tag) {
      case MOCHA_NUMBER:
	delta = (int)argv[0].u.fval;
	if (context->is_grid_cell) {
	    if (delta == 1)
		return hist_forward(mc, obj, 0, 0, rval);
	    if (delta == -1)
		return hist_back(mc, obj, 0, 0, rval);
	}
	index = SHIST_GetIndex(&context->hist, he);
	index += delta;
	he = SHIST_GetObjectNum(&context->hist, index);
	break;

      case MOCHA_STRING:
	histlist = SHIST_GetList(context);
	if (histlist)
	    histlist = histlist->next;	/* to quote jwz: "How stupid!!!" */

	for (index = XP_ListCount(histlist); index > 0; index--) {
	    he = SHIST_GetObjectNum(&context->hist, index);
	    if (XP_STRSTR(he->address, MOCHA_GetAtomName(mc, argv[0].u.atom)) ||
		XP_STRSTR(he->title, MOCHA_GetAtomName(mc, argv[0].u.atom))) {
		goto out;
	    }
	}
	/* FALL THROUGH */

      default:
	he = 0;
	break;
    }

    if (!he)
	return MOCHA_TRUE;
out:
    url_struct = SHIST_CreateURLStructFromHistoryEntry(context, he);
    if (!url_struct) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }
    return lm_GetURL(mc, decoder, url_struct, MOCHA_TAINT_IDENTITY);
}

static MochaBoolean
hist_back(MochaContext *mc, MochaObject *obj,
	  unsigned argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaHistory *hist;
    MochaDecoder *decoder;
    MWContext *context;
    MochaDatum minus_one;

    hist = obj->data;
    decoder = hist->decoder;
    context = decoder->window_context;
    if (context->is_grid_cell) {
	while (context->grid_parent)
	    context = context->grid_parent;
	DEFER(LO_BackInGrid, context);
	return MOCHA_TRUE;
    }
    MOCHA_INIT_FULL_DATUM(mc, &minus_one, MOCHA_NUMBER, 0, MOCHA_TAINT_IDENTITY,
			  u.fval, -1);
    return hist_go(mc, obj, 1, &minus_one, rval);
}

static MochaBoolean
hist_to_string(MochaContext *mc, MochaObject *obj,
	       unsigned argc, MochaDatum *argv, MochaDatum *rval)
{
    MochaHistory *hist;
    MochaDecoder *decoder;
    MWContext *context;
    XP_List *histlist;
    History_entry *he;
    char *str;
    MochaAtom *atom;

    if (!MOCHA_InstanceOf(mc, obj, &hist_class, argv[-1].u.fun))
        return MOCHA_FALSE;

    if (!lm_data_tainting) {
	*rval = MOCHA_empty;
	return MOCHA_TRUE;
    }

    hist = obj->data;
    decoder = hist->decoder;
    context = decoder->window_context;
    if (!context)
	return MOCHA_TRUE;
    histlist = SHIST_GetList(context);

    str = 0;
    StrAllocCopy(str, "<TITLE>Window History</TITLE>"
		      "<TABLE BORDER=0 ALIGN=center VALIGN=top HSPACE=8>");
    while ((he = XP_ListNextObject(histlist)) != 0) {
	StrAllocCat(str, "<TR><TD VALIGN=top><STRONG>");
	StrAllocCat(str, he->title);
	StrAllocCat(str, "</STRONG></TD><TD>&nbsp;</TD>"
			 "<TD VALIGN=top><A HREF=\"");
	StrAllocCat(str, he->address);
	StrAllocCat(str, "\">");
	StrAllocCat(str, he->address);
	StrAllocCat(str, "</A></TD></TR>");
    }
    StrAllocCat(str, "</TABLE>");
    if (!str) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }

    atom = MOCHA_Atomize(mc, str);
    XP_FREE(str);
    if (!atom)
	return MOCHA_FALSE;
    MOCHA_INIT_FULL_DATUM(mc, rval, MOCHA_STRING,
			  MDF_TAINTED, MOCHA_TAINT_SHIST,
			  u.atom, atom);
    return MOCHA_TRUE;
}

static MochaFunctionSpec hist_methods[] = {
    {"go",              hist_go,                1},
    {"back",            hist_back,              0},
    {"forward",         hist_forward,           0},
    {lm_toString_str,   hist_to_string,         1},
    {0}
};

MochaObject *
lm_DefineHistory(MochaDecoder *decoder)
{
    MochaObject *obj;
    MochaContext *mc;
    MochaHistory *hist;

    obj = decoder->history;
    if (obj)
	return obj;

    mc = decoder->mocha_context;
    hist = MOCHA_malloc(mc, sizeof *hist);
    if (!hist)
	return 0;
    obj = MOCHA_InitClass(mc, decoder->window_object, &hist_class, hist,
			  History_ctor, 0, hist_props, hist_methods, 0, 0);
    if (!obj) {
	MOCHA_free(mc, hist);
	return 0;
    }
    if (!MOCHA_DefineObject(mc, decoder->window_object, "history", obj,
			    MDF_ENUMERATE | MDF_READONLY)) {
	return 0;
    }

    XP_BZERO(hist, sizeof *hist);
    hist->object = obj;
    hist->decoder = decoder;
    decoder->history = MOCHA_HoldObject(mc, obj);
    obj->parent = decoder->window_object;
    LM_LINK_OBJECT(decoder, obj, "history");
    return obj;
}
