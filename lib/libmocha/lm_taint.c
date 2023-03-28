/*
** Mocha/Mozilla data tainting support.
**
** Brendan Eich, 5/21/96
*/
#include "lm.h"
#include "xp.h"
#include "mkparse.h"	/* included via -I../libnet */
#include "mkutils.h"	/* for NET_URL_Type() */
#include "prclist.h"
#include "prhash.h"
#include "prmem.h"
#include "prprf.h"

MochaBoolean lm_data_tainting = MOCHA_FALSE;
uint16       lm_branch_taint  = MOCHA_TAINT_IDENTITY;

/* Taint origin-to-info cache. */
static PRHashTable  *origin_to_taint;
static PRCList      taint_freelist = PR_INIT_STATIC_CLIST(&taint_freelist);
static uint16       taint_freelist_len;
static uint16       taint_generator;

/* Cache at least 16 and at most 64 free taint entries on the freelist. */
#define TAINT_FREELIST_MIN 16
#define TAINT_FREELIST_MAX 64

typedef struct TaintEntry {
    PRCList         links;	/* LRU freelist linkage -- must be first */
    MochaRefCount   nrefs;	/* reference count */
    MochaRefCount   mcrefs;	/* references from Mocha contexts */
    MochaTaintInfo  info;	/* Mocha runtime taint info */
    const char      *origin;	/* origin URL prefix (http://server, e.g.) */
    const char      *index;	/* taint code pseudo-URL (taint:code) */
} TaintEntry;

#define APPEND_TO_TAINT_FREELIST(te) {                                        \
    XP_ASSERT(te->links.next == &te->links);                                  \
    PR_APPEND_LINK(&(te)->links, &taint_freelist);                            \
    taint_freelist_len++;                                                     \
}

#define REMOVE_FROM_TAINT_FREELIST(te) {                                      \
    XP_ASSERT(te->links.next != &te->links);                                  \
    PR_REMOVE_AND_INIT_LINK(&(te)->links);                                    \
    taint_freelist_len--;                                                     \
}

static char taint_index_format[] = "taint:%u";
static char file_url_prefix[]    = "file:";
static char unknown_origin_str[] = "[unknown origin]";

#define TAINT_INDEX_BUFFER_MAX  16
#define FILE_URL_PREFIX_LEN	(sizeof file_url_prefix - 1)

static TaintEntry *get_taint_entry(MochaContext *mc, const char *url_string);
static TaintEntry *set_taint_entry(MochaContext *mc, const char *url_string,
				   uint16 taint);

static TaintEntry *file_taint_entry;

static uint16 mix_taint(MochaContext *mc, uint16 accum, uint16 taint);
static void   hold_taint(MochaContext *mc, uint16 taint);
static void   drop_taint(MochaContext *mc, uint16 taint);

MochaBoolean
lm_InitTaint(MochaContext *mc)
{

#ifdef DEBUG_brendan
    if (!getenv("NS_DISABLE_TAINT"))
#else
    if (getenv("NS_ENABLE_TAINT"))
#endif
	lm_data_tainting = MOCHA_TRUE;
    if (!lm_data_tainting)
	return MOCHA_TRUE;

    origin_to_taint = PR_NewHashTable(2 * TAINT_FREELIST_MAX,
				      PR_HashString, PR_CompareStrings,
				      PR_CompareValues, 0, 0);
    if (!origin_to_taint)
	return MOCHA_FALSE;

    (void) set_taint_entry(mc, "[session history]", MOCHA_TAINT_SHIST);
    (void) set_taint_entry(mc, "[Java applets]",    MOCHA_TAINT_JAVA);
    (void) set_taint_entry(mc, unknown_origin_str, MOCHA_TAINT_MAX);
    file_taint_entry = get_taint_entry(mc, file_url_prefix);
    MOCHA_SetTaintCallbacks(hold_taint, drop_taint, mix_taint);
    return MOCHA_TRUE;
}

static int
is_taint_unique(PRHashEntry *he, int i, void *arg)
{
    TaintEntry *te = he->value;
    MochaBoolean *donep = arg;

    if (te->info.taint == taint_generator) {
	*donep = MOCHA_FALSE;
	return HT_ENUMERATE_STOP;
    }
    return HT_ENUMERATE_NEXT;
}

static uint16
generate_taint(void)
{
    MochaBoolean done;

    do {
	do {
	    taint_generator++;
	} while (taint_generator == MOCHA_TAINT_IDENTITY ||
		 taint_generator == MOCHA_TAINT_SHIST ||
		 taint_generator == MOCHA_TAINT_JAVA ||
		 taint_generator == MOCHA_TAINT_MAX);
	done = MOCHA_TRUE;
	PR_HashTableEnumerateEntries(origin_to_taint, is_taint_unique, &done);
    } while (!done);
    return taint_generator;
}

static void destroy_taint_entry(TaintEntry *te, MochaBoolean unhash);
static void free_taint_entry(TaintEntry *te, MochaBoolean unhash);
static void put_taint_entry(TaintEntry *te);

static TaintEntry *
new_taint_entry(MochaContext *mc, const char *origin, uint16 taint)
{
    TaintEntry *te;
    const char *index;

    if (taint_freelist_len < TAINT_FREELIST_MIN) {
	te = MOCHA_malloc(mc, sizeof *te);
	if (!te) {
	    MOCHA_ReportOutOfMemory(mc);
	    return 0;
	}
    } else {
	te = (TaintEntry *)taint_freelist.next;
	XP_ASSERT(&te->links != &taint_freelist);
	REMOVE_FROM_TAINT_FREELIST(te);
	free_taint_entry(te, MOCHA_TRUE);
    }

    if (taint == MOCHA_TAINT_IDENTITY) {
	switch (NET_URL_Type(origin)) {
	  case ABOUT_TYPE_URL:
	  case MOCHA_TYPE_URL:
	  case VIEW_SOURCE_TYPE_URL:
	    break;
	  default:
	    taint = generate_taint();
	    break;
	}
    }
    index = PR_smprintf(taint_index_format, taint);
    if (!index) {
	MOCHA_ReportOutOfMemory(mc);
	MOCHA_free(mc, te);
	return 0;
    }

    PR_INIT_CLIST(&te->links);
    te->nrefs = te->mcrefs = 0;
    te->info.taint = taint;
    te->info.accum = MOCHA_TAINT_IDENTITY;
    te->info.data = te;
    te->origin = origin;
    te->index = index;

    if (!PR_HashTableAdd(origin_to_taint, origin, te) ||
	!PR_HashTableAdd(origin_to_taint, index, te)) {
	MOCHA_ReportOutOfMemory(mc);
	destroy_taint_entry(te, MOCHA_TRUE);
	return MOCHA_FALSE;
    }
    return te;
}

static TaintEntry *
context_taint_entry(MochaContext *mc)
{
    return MOCHA_GetTaintInfo(mc)->data;
}

static void
switch_context_taint_entry(MochaContext *mc, TaintEntry *te)
{
    TaintEntry *cte;

    cte = context_taint_entry(mc);
    if (cte == te)
	return;
    if (te) {
	MOCHA_SetTaintInfo(mc, &te->info);
	te->nrefs++;
	te->mcrefs++;
    } else {
	MOCHA_SetTaintInfo(mc, 0);
    }
    if (cte) {
	XP_ASSERT(cte->nrefs > 0 && cte->mcrefs > 0);
	if (--cte->mcrefs == 0)
	    cte->info.accum = MOCHA_TAINT_IDENTITY;
	put_taint_entry(cte);
    }
}

static void
destroy_taint_entry(TaintEntry *te, MochaBoolean unhash)
{
    MochaContext *iter, *mc;
    TaintEntry *cte;

    XP_ASSERT(!unhash || te->nrefs == 0);
    iter = 0;
    while ((mc = MOCHA_ContextIterator(&iter)) != 0) {
	cte = context_taint_entry(mc);
	XP_ASSERT(cte != te);
	if (cte == te)
	    switch_context_taint_entry(mc, 0);
    }
    free_taint_entry(te, unhash);
    XP_FREE(te);
}

static void
free_taint_entry(TaintEntry *te, MochaBoolean unhash)
{
    if (unhash) {
	PR_HashTableRemove(origin_to_taint, te->origin);
	PR_HashTableRemove(origin_to_taint, te->index);
    }
    XP_FREE((void *)te->origin);
    XP_FREE((void *)te->index);
    te->origin = te->index = 0;
}

static TaintEntry *
find_taint_entry(uint16 taint)
{
    char buf[TAINT_INDEX_BUFFER_MAX];

    PR_snprintf(buf, sizeof buf, taint_index_format, taint);
    return PR_HashTableLookup(origin_to_taint, buf);
}

#define HOLD_TAINT_ENTRY(te) {                                                \
    XP_ASSERT((te)->nrefs >= 0);                                              \
    if ((te)->nrefs == 0)                                                     \
	REMOVE_FROM_TAINT_FREELIST(te);                                       \
    (te)->nrefs++;                                                            \
}

static void
hold_taint(MochaContext *mc, uint16 taint)
{
    TaintEntry *te = find_taint_entry(taint);

    if (te)
	HOLD_TAINT_ENTRY(te);
}

static void
drop_taint(MochaContext *mc, uint16 taint)
{
    TaintEntry *te = find_taint_entry(taint);

    if (te)
	put_taint_entry(te);
}

static TaintEntry *
get_taint_entry(MochaContext *mc, const char *url_string)
{
    const char *origin;
    TaintEntry *te;

    if (!XP_STRCHR(url_string, '\n'))
	origin = lm_GetOrigin(mc, url_string);
    else
	origin = MOCHA_strdup(mc, url_string);
    if (!origin)
	return 0;

    te = PR_HashTableLookup(origin_to_taint, origin);
    if (te) {
	/* An entry already exists; share its info struct. */
	MOCHA_free(mc, (void *)origin);
	HOLD_TAINT_ENTRY(te);
    } else {
	/* Create a new entry and add it to the table. */
	te = new_taint_entry(mc, origin, MOCHA_TAINT_IDENTITY);
	if (!te) {
	    MOCHA_free(mc, (void *)origin);
	    return 0;
	}
	te->nrefs = 1;
    }
    return te;
}

static void
put_taint_entry(TaintEntry *te)
{
    TaintEntry *victim_te;

    XP_ASSERT(te->nrefs > 0);
    if (--te->nrefs == 0) {
	/* Evict the oldest taint entry if the freelist is full. */
	if (taint_freelist_len >= TAINT_FREELIST_MAX) {
	    victim_te = (TaintEntry *)taint_freelist.next;
	    REMOVE_FROM_TAINT_FREELIST(victim_te);
	    destroy_taint_entry(victim_te, MOCHA_TRUE);
	}
	APPEND_TO_TAINT_FREELIST(te);
    }
}

static TaintEntry *
set_taint_entry(MochaContext *mc, const char *origin, uint16 taint)
{
    TaintEntry *te;

    /* Create a new entry and add it to the table. */
    te = new_taint_entry(mc, origin, taint);
    if (!te) {
	XP_FREE((void *)origin);
	return 0;
    }
    te->nrefs = 1;
    return te;
}

static MochaBoolean
split_sort_uniq(MochaContext *mc, const char *s, char c, const char ***vp)
{
    uint i, j, len;
    char **vec, **newvec;
    const char *p, *q;
    int compare;
    char *tmp;

    i = 0;
    vec = (char **) *vp;
    if (vec) {
	while (vec[i])
	    i++;
    }
    for (p = s; ; p = q + 1) {
	q = XP_STRCHR(p, c);
	if (!vec) {
	    vec = MOCHA_malloc(mc, 2 * sizeof *vec);
	    if (!vec)
		return MOCHA_FALSE;
	} else {
	    newvec = XP_REALLOC(vec, (i + 2) * sizeof *vec);
	    if (!newvec) {
		/* XXX add MOCHA_realloc and define its API carefully */
		MOCHA_ReportOutOfMemory(mc);
		goto bad;
	    }
	    vec = newvec;
	}
	len = q ? q - p : XP_STRLEN(p);
	vec[i] = MOCHA_malloc(mc, len + 1);
	if (!vec[i])
	    goto bad;
	XP_MEMCPY(vec[i], p, len);
	vec[i][len] = '\0';
	for (j = 0; j < i; j++) {
	    compare = XP_STRCMP(vec[i], vec[j]);
	    if (compare == 0) {
		MOCHA_free(mc, vec[i]);
		vec[i--] = 0;
		break;
	    }
	    if (compare < 0) {
		tmp = vec[j];
		vec[j] = vec[i];
		vec[i] = tmp;
	    }
	}
	i++;
	if (!q)
	    break;
    }
    vec[i] = 0;
    *vp = (const char **)vec;
    return MOCHA_TRUE;

bad:
    if (i) {
	do {
	    MOCHA_free(mc, vec[--i]);
	} while (i);
    }
    MOCHA_free(mc, vec);
    return MOCHA_FALSE;
}

static const char *
join_vec(MochaContext *mc, const char **vec, char c)
{
    char *s;
    uint i;

    s = 0;
    for (i = 0; vec[i]; i++)
	s = PR_sprintf_append(s, "%s%c", vec[i], c);
    if (i && !s) {
	MOCHA_ReportOutOfMemory(mc);
	return 0;
    }
    s[strlen(s)-1] = '\0';
    return s;
}

static void
free_vec(MochaContext *mc, const char **vec)
{
    uint i;

    for (i = 0; vec[i]; i++)
	MOCHA_free(mc, (void *)vec[i]);
    MOCHA_free(mc, vec);
}

static TaintEntry *
mix_taint_entries(MochaContext *mc, TaintEntry *te, TaintEntry *te2)
{
    const char **origin_vec, *mixture;
    TaintEntry *mixed_te;

    origin_vec = 0;
    if (!split_sort_uniq(mc, te->origin, '\n', &origin_vec))
	return 0;
    if (!split_sort_uniq(mc, te2->origin, '\n', &origin_vec))
	return 0;
    mixture = join_vec(mc, origin_vec, '\n');
    free_vec(mc, origin_vec);
    if (!mixture)
	return 0;
    mixed_te = get_taint_entry(mc, mixture);
    XP_FREE((void *)mixture);
    return mixed_te;
}

static uint16
mix_taint(MochaContext *mc, uint16 accum, uint16 taint)
{
    TaintEntry *accum_te, *taint_te, *mixed_te;
    uint16 mixed_taint;

    accum_te = find_taint_entry(accum);
    taint_te = find_taint_entry(taint);
    if (!accum_te || !taint_te)
	return MOCHA_TAINT_MAX;
    mixed_te = mix_taint_entries(mc, accum_te, taint_te);
    if (!mixed_te)
	return MOCHA_TAINT_MAX;
    mixed_taint = mixed_te->info.taint;
    put_taint_entry(mixed_te);
    return mixed_taint;
}

#define WYSIWYG_TYPE_LEN	10	/* wysiwyg:// */

const char *
LM_SkipWysiwygURLPrefix(const char *url_string)
{
    if (XP_STRLEN(url_string) < WYSIWYG_TYPE_LEN)
	return 0;
    url_string += WYSIWYG_TYPE_LEN;
    url_string = XP_STRCHR(url_string, '/');
    if (!url_string)
	return 0;
    return url_string + 1;
}

static const char *
find_creator_url(MWContext *context)
{
    History_entry *he;
    const char *address;
    MochaContext *mc;
    MochaObject *obj;
    MochaDecoder *decoder;

    he = SHIST_GetCurrent(&context->hist);
    if (he && (address = he->address) != 0) {
	switch (NET_URL_Type(address)) {
	  case ABOUT_TYPE_URL:
	  case MOCHA_TYPE_URL:
	    /* These types cannot name the true origin (server) of JS code. */
	    break;
	  case WYSIWYG_TYPE_URL:
	    return LM_SkipWysiwygURLPrefix(address);
	  case VIEW_SOURCE_TYPE_URL:
	    XP_ASSERT(0);
	  default:
	    return address;
	}
    }

    if (context->grid_parent) {
	address = find_creator_url(context->grid_parent);
	if (address)
	    return address;
    }

    mc = context->mocha_context;
    if (mc) {
	obj = MOCHA_GetGlobalObject(mc);
	decoder = obj->data;
	if (decoder->opener) {
	    /* self.opener property is valid, check its MWContext. */
	    decoder = decoder->opener->data;
	    if (!decoder->visited) {
		decoder->visited = MOCHA_TRUE;
		address = find_creator_url(decoder->window_context);
		decoder->visited = MOCHA_FALSE;
		if (address)
		    return address;
	    }
	}
    }

    return 0;
}

static const char *
find_origin_url(MochaContext *mc, MochaDecoder *decoder)
{
    const char *url_string;
    MWContext *context;
    MochaDecoder *running;

    url_string = decoder->origin_url;
    if (url_string && decoder->domain_set)
	return url_string;
    context = decoder->window_context;
    if (!context) {
	MOCHA_ReportError(mc, "cannot determine origin of %s",
			  mocha_language_name);
	return 0;
    }

    url_string = find_creator_url(context);
    if (!url_string) {
	/* Must be a new, empty window?  Use running origin. */
	running = MOCHA_GetGlobalObject(mc)->data;
	if (running->origin_url)
	    url_string = running->origin_url;
	else
	    url_string = unknown_origin_str;
    }
    return lm_SetOriginURL(mc, decoder, url_string);
}

MochaBoolean
lm_GetTaint(MochaContext *mc, MochaDecoder *decoder, uint16 *taintp)
{
    MochaTaintInfo *info;
    uint16 taint;
    const char *save, *url_string;
    TaintEntry *te;

    if (!lm_data_tainting) {
	*taintp = MOCHA_TAINT_IDENTITY;
	return MOCHA_TRUE;
    }

    /*
    ** Compare old and new origin_url pointers to optimize for same taint.
    ** NB: this requires find_origin_url() to allocate a new origin_url before
    ** it frees the old string.  It means save may point at free space after
    ** the find_origin_url() call, but we don't refer to free memory, we only
    ** compare pointers.
    */
    info = MOCHA_GetTaintInfo(decoder->mocha_context);
    taint = info->taint;
    save = decoder->origin_url;
    url_string = find_origin_url(mc, decoder);
    if (!url_string)
	return MOCHA_FALSE;
    if (taint != MOCHA_TAINT_IDENTITY && url_string == save) {
	*taintp = taint;
	return MOCHA_TRUE;
    }

    te = get_taint_entry(mc, url_string);
    if (!te)
	return MOCHA_FALSE;
    if (&te->info != info) {
	MOCHA_MIX_TAINT(mc, te->info.accum, info->accum);
	switch_context_taint_entry(decoder->mocha_context, te);
    }
    *taintp = te->info.taint;
    put_taint_entry(te);
    return MOCHA_TRUE;
}

MochaBoolean
lm_SetTaint(MochaContext *mc, MochaDecoder *decoder, const char *url_string)
{
    TaintEntry *te;
    MochaTaintInfo *info;

    if (!lm_data_tainting)
	return MOCHA_TRUE;

    te = get_taint_entry(mc, url_string);
    if (!te)
	return MOCHA_FALSE;
    info = MOCHA_GetTaintInfo(decoder->mocha_context);
    if (&te->info != info)
	switch_context_taint_entry(decoder->mocha_context, te);
    put_taint_entry(te);
    return MOCHA_TRUE;
}

MochaBoolean
lm_MergeTaint(MochaContext *mc, MochaContext *mc2)
{
    MochaTaintInfo *info, *info2;
    TaintEntry *merged_te;

    info = MOCHA_GetTaintInfo(mc);
    if (!info->data)
	return MOCHA_TRUE;
    info2 = MOCHA_GetTaintInfo(mc2);
    if (info == info2)
	return MOCHA_TRUE;
    if (!info2->data) {
	merged_te = info->data;
	HOLD_TAINT_ENTRY(merged_te);
    } else {
	merged_te = mix_taint_entries(mc, info->data, info2->data);
	if (!merged_te)
	    return MOCHA_FALSE;
    }
    MOCHA_MIX_TAINT(mc, merged_te->info.accum, info->accum);
    MOCHA_MIX_TAINT(mc, merged_te->info.accum, info2->accum);
    switch_context_taint_entry(mc, merged_te);
    switch_context_taint_entry(mc2, merged_te);
    put_taint_entry(merged_te);
    return MOCHA_TRUE;
}

void
lm_ClearTaint(MochaContext *mc)
{
    switch_context_taint_entry(mc, 0);
}

MochaBoolean
lm_CheckTaint(MochaContext *mc, uint16 taint, const char *url_desc,
	      const char *url_string)
{
    TaintEntry *te;
    uint16 accum, script_taint, server_taint;
    char *message;
    MochaDecoder *decoder;
    MochaBoolean ok;

    if (!lm_data_tainting)
	return MOCHA_TRUE;

    te = get_taint_entry(mc, url_string);
    if (!te)
	return MOCHA_FALSE;
    server_taint = te->info.taint;
    put_taint_entry(te);
    if (server_taint == MOCHA_TAINT_IDENTITY ||
	server_taint == file_taint_entry->info.taint) {
	return MOCHA_TRUE;
    }

    script_taint = MOCHA_GetTaintInfo(mc)->accum;
    accum = taint;
    MOCHA_MIX_TAINT(mc, accum, script_taint);
    MOCHA_MIX_TAINT(mc, accum, server_taint);
    if (accum == server_taint) {
	/* data, script, and server have compatible taint. */
	return MOCHA_TRUE;
    }

    /* XXX refer users to the handbook for more on tainting. */
    message = PR_smprintf("%s Confirm:\n"
			  "The %s%s%s\n"
			  "may contain sensitive or private data gathered\n",
			  mocha_language_name,
			  url_desc,
			  (XP_STRLEN(url_desc) + XP_STRLEN(url_string) > 40)
			      ? "\n" : " ",
			  url_string);

    te = find_taint_entry((taint != MOCHA_TAINT_IDENTITY &&
			   taint != MOCHA_TAINT_MAX &&
			   taint != server_taint)
			   ? taint
			   : script_taint);
    if (te) {
	message = PR_sprintf_append(message,
				    "from the following location(s):\n%s\n",
				    te->origin);
    }

    message =
	PR_sprintf_append(message,
			  "and encoded by JavaScript.\n\n"
			  "You may click OK to continue this operation,\n"
			  "or Cancel to abort it.\n");
    if (!message) {
	MOCHA_ReportOutOfMemory(mc);
	return MOCHA_FALSE;
    }

    decoder = MOCHA_GetGlobalObject(mc)->data;
    ok = FE_Confirm(decoder->window_context, message);
    XP_FREE(message);
    return ok;
}

MochaBoolean
LM_CheckGetURL(MWContext *context, URL_Struct *url_struct)
{
    MochaDecoder *decoder;
    MochaContext *mc;
    MochaBoolean ok;

    if (url_struct->force_reload != NET_DONT_RELOAD) {
	/* We're just reloading the same document: allow it. */
	return MOCHA_TRUE;
    }

    /* Handle document.write("<frame src='cgi/steal?"+encode(history)+"'>") */
    while (!context->mocha_context) {
	context = context->grid_parent;
	if (!context)
	    return MOCHA_TRUE;
    }
    decoder = LM_GetMochaDecoder(context);
    if (!decoder) {
	/* Perhaps JavaScript was disabled after some use of it. */
	return MOCHA_TRUE;
    }

    /*
    ** Use the executing Mocha context, if Mocha is running.  Otherwise use
    ** decoder's context.  We've already checked in lm_GetURL for a tainted
    ** URL string, and in LM_SendOnSubmit for a tainted form action URL, and
    ** for tainted form element data.
    */
    mc = lm_running_context;
    if (!mc)
	mc = decoder->mocha_context;

    /*
    ** Mix taint conveyed via document.write() in with accumulated taint.
    ** Over-conservative, but necessary in lieu of a <TAINT CODE=%u> tag
    ** tag that we might add around the written strings, and process in
    ** layout so as to taint its tag structures.
    */
    ok = lm_CheckTaint(mc, decoder->write_taint_accum, lm_location_str,
		       url_struct->address);
    LM_PutMochaDecoder(decoder);
    return ok;
}

static const char *
strip_file_double_slash(MochaContext *mc, const char *url_string)
{
    char *new_url_string;

    if (!XP_STRNCASECMP(url_string, file_url_prefix, FILE_URL_PREFIX_LEN) &&
	url_string[FILE_URL_PREFIX_LEN + 0] == '/' &&
	url_string[FILE_URL_PREFIX_LEN + 1] == '/') {
	new_url_string = PR_smprintf("%s%s",
				     file_url_prefix,
				     url_string + FILE_URL_PREFIX_LEN + 2);
	if (!new_url_string)
	    MOCHA_ReportOutOfMemory(mc);
    } else {
	new_url_string = MOCHA_strdup(mc, url_string);
    }
    return new_url_string;
}

const char *
lm_GetOrigin(MochaContext *mc, const char *url_string)
{
    const char *origin, *new_origin;

    if (NET_URL_Type(url_string) == WYSIWYG_TYPE_URL)
	url_string = LM_SkipWysiwygURLPrefix(url_string);
    origin = NET_ParseURL(url_string, GET_PROTOCOL_PART | GET_HOST_PART);
    if (!origin) {
	MOCHA_ReportOutOfMemory(mc);
	return 0;
    }
    new_origin = strip_file_double_slash(mc, origin);
    XP_FREE((char *)origin);
    return new_origin;
}

const char *
lm_GetOriginURL(MochaContext *mc, MochaDecoder *decoder)
{
    return find_origin_url(mc, decoder);
}

const char *
lm_SetOriginURL(MochaContext *mc, MochaDecoder *decoder,
		const char *url_string)
{
    const char *origin_url, *new_origin_url;

    origin_url = decoder->origin_url;
    if (origin_url && !XP_STRCMP(origin_url, url_string))
	return origin_url;
    new_origin_url = strip_file_double_slash(mc, url_string);
    if (new_origin_url) {
	decoder->origin_url = new_origin_url;
	PR_FREEIF((char *)origin_url);
    }
    return new_origin_url;
}

MochaBoolean
lm_CheckOrigins(MochaContext *mc, MochaDecoder *decoder)
{
    MochaDecoder *running;
    const char *org1, *org2;
    MochaBoolean ok;

    if (lm_data_tainting)
	return MOCHA_TRUE;

    /* Get non-null origin_url pointers for running and decoder. */
    running = MOCHA_GetGlobalObject(mc)->data;
    if (running == decoder)
	return MOCHA_TRUE;
    if (!find_origin_url(mc, running) || !find_origin_url(mc, decoder))
	return MOCHA_FALSE;

    /* Now see whether the origin methods and servers match. */
    if (!XP_STRCMP(running->origin_url, decoder->origin_url))
	return MOCHA_TRUE;
    org1 = lm_GetOrigin(mc, running->origin_url);
    org2 = lm_GetOrigin(mc, decoder->origin_url);
    if (org1 && org2) {
	ok = !XP_STRCMP(org1, org2);
	if (!ok) {
	    MOCHA_ReportError(mc,
		"access disallowed from scripts at %s to documents at %s",
		LM_GetSourceURL(running), LM_GetSourceURL(decoder));
	}
    } else {
	/* lm_GetOrigin already reported the error. */
	ok = MOCHA_FALSE;
    }
    PR_FREEIF((char *)org1);
    PR_FREEIF((char *)org2);
    return ok;
}
