/* -*- Mode: C; tab-width: 4; indent-tabs-mode: f -*-
 */
#ifdef MOCHA
/*
 * SCRIPT tag support.
 */

#include "xp.h"
#include "libmocha.h"
#include "lo_ele.h"
#include "net.h"
#include "pa_parse.h"
#include "pa_tags.h"
#include "layout.h"

/* XXX borrowed from mkcache.h, should include that file */
extern int    NET_FindURLInCache(URL_Struct * URL_s, MWContext *ctxt);

typedef struct lo_ScriptState {
    lo_TopState         *top_state;
    NET_StreamClass     *parser_stream;
    URL_Struct          *url_struct;
    Net_GetUrlExitFunc  *pre_exit_fn;
    intn                blocked_tags;
} lo_ScriptState;

static int
lo_script_write(void *data_object, const char *str, int32 len)
{
    lo_ScriptState *script_state;
    NET_StreamClass *parser_stream;

    script_state = data_object;
    if (script_state == NULL)       /* XXX paranoia */
        return -1;
    parser_stream = script_state->parser_stream;
    return parser_stream->put_block(parser_stream->data_object, str, len);
}

static unsigned int
lo_script_write_ready(void *data_object)
{
    lo_ScriptState *script_state;
    NET_StreamClass *parser_stream;

    script_state = data_object;
    if (script_state == NULL)       /* XXX paranoia */
        return 0;
    parser_stream = script_state->parser_stream;
    return parser_stream->is_write_ready(parser_stream->data_object);
}

static void
lo_destroy_script_stream(lo_ScriptState *script_state)
{
    NET_StreamClass *parser_stream;
    MWContext *context;
    lo_TopState *top_state;

    /* Clear any MochaDecoder pointers to the parser stream. */
    parser_stream = script_state->parser_stream;
    context = parser_stream->window_id;
    if (context != NULL)
        LM_ClearContextStream(context);

    /* Clear pointers between top_state and stream-private data. */
    top_state = script_state->top_state;
    top_state->script_state = NULL;
    if (top_state->doc_data != NULL) {
        top_state->doc_data->layout_state = NULL;
        top_state->doc_data = NULL;
    }

    /* Free the cloned parser_stream's URL struct. */
    if (script_state->url_struct != NULL)
        NET_FreeURLStruct(script_state->url_struct);

    /* Free the cloned downward stream and our state record. */
    XP_DELETE(parser_stream);
    XP_DELETE(script_state);
}

/*
 * The reason for all this indirection: we must not call the parser stream's
 * complete function if there are blocked script tags (we have to presume that
 * they will write to this stream via document.write{,ln}).  In that case, the
 * complete function will be called later when lo_FlushBlockage processes all
 * the script tags and the blocked_tags counter goes to zero.
 */
static void
lo_script_complete(void *data_object)
{
    lo_ScriptState *script_state;
    NET_StreamClass *parser_stream;

    script_state = data_object;
    if (script_state == NULL)       /* XXX paranoia */
        return;
    parser_stream = script_state->parser_stream;
    if (script_state->blocked_tags == 0)
        parser_stream->complete(parser_stream->data_object);
    else
        script_state->top_state->nurl = NULL;
}

static void
lo_script_abort(void *data_object, int status)
{
    lo_ScriptState *script_state;
    NET_StreamClass *parser_stream;

    script_state = data_object;
    if (script_state == NULL)       /* XXX paranoia */
        return;
    parser_stream = script_state->parser_stream;
    parser_stream->abort(parser_stream->data_object, status);
    lo_destroy_script_stream(script_state);
}

static void
lo_switch_doc_data_url(URL_Struct *url_struct, int status, MWContext *context)
{
    lo_TopState *top_state;
    pa_DocData *doc_data;
    lo_ScriptState *script_state;

    top_state = lo_FetchTopState(XP_DOCID(context));
    if (top_state == NULL)
        return;
    doc_data = top_state->doc_data;
    if (doc_data == NULL)
        return;
    script_state = top_state->script_state;
    if (script_state == NULL)
        return;
    if (script_state->pre_exit_fn)
        script_state->pre_exit_fn(url_struct, status, context);
    url_struct = NET_CreateURLStruct(doc_data->url_struct->address,
                                     doc_data->url_struct->force_reload);
    if (url_struct == NULL) {
        top_state->out_of_memory = TRUE;
        return;
    }
    NET_FindURLInCache(url_struct, context);
    doc_data->url_struct = script_state->url_struct = url_struct;
}

void
lo_BlockScriptTag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
    lo_TopState *top_state;
    pa_DocData *doc_data;
    lo_ScriptState *script_state;
    NET_StreamClass *stream, *new_stream;
    URL_Struct *url_struct;
    MochaDecoder *decoder;

    /*
     * Find the parser stream from its private data, doc_data, pointed at by
     * a top_state member for just this situation.
     */
    top_state = state->top_state;
    doc_data = top_state->doc_data;
    XP_ASSERT(doc_data != NULL && doc_data->url_struct != NULL);
    if (doc_data == NULL || doc_data->url_struct == NULL)   /* XXX paranoia */
        return;

    /*
     * Count the bytes of HTML comments and heuristicly "lost" newlines in
     * tag so lo_ProcessScriptTag can recover it there later, in preference
     * to the (then far-advanced) value of doc_data->comment_bytes.  Ensure
     * that this hack is not confused for NULL by adding 1 here and taking
     * it away later.  Big XXX for future cleanup, needless to say.
     */
    tag->lo_data = (void *)(doc_data->comment_bytes + 1);

    /*
     * Check that we have not yet pushed our stream under the parser stream.
     * We do this by testing the top_state->script_state pointer.  If we have
     * already pushed a script-stream module, bump its blocked_tags counter
     * and we're done.
     */
    script_state = top_state->script_state;
    if (script_state) {
        script_state->blocked_tags++;
        return;
    }

    /*
     * Clone the parser stream so we can "push" a stream under it, and above
     * the netlib code that feeds us data when it reads from a socket or from
     * the cache.  Thus when the netlib sees a server close on the connection
     * (or EOF on the cache entry), we can intercept its call to the parser
     * stream's complete function, deferring stream tear-down until blocked
     * script tags are flushed.
     */
    stream = doc_data->parser_stream;
    XP_ASSERT(stream->window_id == context);
    new_stream = NET_NewStream(stream->name,
                               stream->put_block,
                               stream->complete,
                               stream->abort,
                               stream->is_write_ready,
                               stream->data_object,
                               context);
    if (new_stream == NULL) {
        top_state->out_of_memory = TRUE;
        return;
    }

    /*
     * Make a script state structure so we can tell what we're supposed to do
     * when in <SCRIPT SRC="url"> and blocked/flush-blockage layout states.
     */
    script_state = XP_NEW(lo_ScriptState);
    if (script_state == NULL) {
        XP_DELETE(new_stream);
        top_state->out_of_memory = TRUE;
        return;
    }
    script_state->top_state = top_state;
    script_state->parser_stream = new_stream;
    script_state->url_struct = NULL;
    url_struct = doc_data->url_struct;
    script_state->pre_exit_fn = url_struct->pre_exit_fn;
    url_struct->pre_exit_fn = lo_switch_doc_data_url;
    script_state->blocked_tags = 1;

    /*
     * Now rewrite the original stream to point to our script-stream functions.
     */
    stream->name           = "script-stream";
    stream->put_block      = lo_script_write;
    stream->complete       = lo_script_complete;
    stream->abort          = lo_script_abort;
    stream->is_write_ready = lo_script_write_ready;
    stream->data_object    = script_state;
    stream->window_id      = context;

    /*
     * Record the script_state pointer in top_state and switch the parser's
     * and the decoder's current stream pointer.
     */
    top_state->script_state = script_state;
    doc_data->parser_stream = new_stream;
    decoder = LM_GetMochaDecoder(context);
    if (decoder) {
        if (decoder->stream == stream)
            decoder->stream = new_stream;
        LM_PutMochaDecoder(decoder);
    }
}

void
lo_UnblockScriptTag(lo_DocState *state)
{
    lo_TopState *top_state;
    lo_ScriptState *script_state;

    if (state->in_relayout != FALSE)
        return;
    top_state = state->top_state;
    script_state = top_state->script_state;
    if ((script_state != NULL) &&
        (script_state->blocked_tags != 0)) {
        /*
         * We were blocked by some image or embed tag -- decrement the
         * blocked script tag counter, and call complete on the script
         * stream if the counter goes to zero *and* lo_script_complete
         * was already called by netlib.
         */
        XP_ASSERT(script_state->blocked_tags > 0);
        if ((--script_state->blocked_tags == 0) &&
            (top_state->nurl == NULL)) {
            lo_script_complete(script_state);
        }
    }
}

void
lo_ClearInputStream(MWContext *context, lo_TopState *top_state)
{
    lo_ScriptState *script_state;

    script_state = top_state->script_state;
    if (script_state) {
        if (script_state->blocked_tags == 0)
            lo_destroy_script_stream(script_state);
    } else {
        LM_ClearContextStream(context);

        /* Clear pointers between top_state and stream-private data. */
        if (top_state->doc_data != NULL) {
            top_state->doc_data->layout_state = NULL;
            top_state->doc_data = NULL;
        }
    }
}

static void
lo_script_src_exit_fn(URL_Struct *url_struct, int status, MWContext *context)
{
    int32 doc_id;
    lo_TopState *top_state;
    lo_DocState *state;
    MochaDecoder *decoder;
    LO_Element *script_ele;

    /* XXX need state to be subdoc state? */
    doc_id = XP_DOCID(context);
    top_state = lo_FetchTopState(doc_id);
    if ((top_state == NULL) ||
        ((state = top_state->doc_state) == NULL)) {
        return;
    }

    /* We should get a Mocha decoder, because lo_ProcessScriptTag() did. */
    decoder = LM_GetMochaDecoder(context);
    XP_ASSERT(decoder != NULL);
    if (decoder != NULL) {
        XP_FREE((char *)decoder->nesting_url);
        decoder->nesting_url = 0;
        LM_PutMochaDecoder(decoder);
    }

    /* Flush tags blocked by this <SCRIPT SRC="URL"> tag. */
    script_ele = top_state->layout_blocking_element;
    top_state->layout_blocking_element = NULL;
    lo_FreeElement(context, script_ele, FALSE);
    lo_FlushBlockage(context, state, state);

    /* Free the URL struct allocated before NET_GetURL below. */
    NET_FreeURLStruct(url_struct);
}

static char script_suppress_tag[]  = "<" PT_SCRIPT " " PARAM_LANGUAGE
                                         "=suppress>";
static char script_end_tag[]       = "</" PT_SCRIPT ">";

void
lo_ProcessScriptTag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
    lo_TopState *top_state;
    pa_DocData *doc_data;
    MochaDecoder *decoder;

    top_state = state->top_state;
    doc_data = (pa_DocData *)top_state->doc_data;
    XP_ASSERT(doc_data != NULL || state->in_relayout);

    if (tag->is_end == FALSE) {
        PA_Block buff;
        char *str, *url;

        /* Controversial default language value. */
        top_state->in_script = SCRIPT_TYPE_MOCHA;

        /* XXX account for HTML comment bytes and  "lost" newlines */
        top_state->script_bytes = top_state->layout_bytes - tag->true_len;
        if (tag->lo_data != NULL) {
            top_state->script_bytes += (int32)tag->lo_data - 1;
            tag->lo_data = NULL;
        } else if (doc_data != NULL) {
            top_state->script_bytes += doc_data->comment_bytes;
        } else {
            XP_ASSERT(state->in_relayout);
        }

        buff = lo_FetchParamValue(context, tag, PARAM_LANGUAGE);
        if (buff != NULL) {
            PA_LOCK(str, char *, buff);
            if ((XP_STRCASECMP(str, mocha_language_name) == 0) ||
                (XP_STRCASECMP(str, "JavaScript1.1") == 0) ||
                (XP_STRCASECMP(str, "LiveScript") == 0) ||
                (XP_STRCASECMP(str, "Mocha") == 0)) {
                top_state->in_script = SCRIPT_TYPE_MOCHA;
            } else {
                top_state->in_script = SCRIPT_TYPE_UNKNOWN;
            }
            PA_UNLOCK(buff);
            PA_FREE(buff);
        }

        /*
         * If in Mocha, record an allocation state watermark in top_state.
         */
        if (top_state->in_script == SCRIPT_TYPE_MOCHA)
            lo_MarkMochaTopState(top_state, context);

        /*
         * Flush the line buffer so we can start storing Mocha script
         * source lines in there.
         */
        if ((state->cur_ele_type == LO_TEXT) &&
            (state->line_buf_len != 0)) {
            lo_FlushLineBuffer(context, state);
        }

        /*
         * Set text_divert so we know to accumulate text in line_buf
         * without interpretation.
         */
        state->text_divert = tag->type;

        /*
         * XXX need to stack these to handle blocked SCRIPT tags
         */
        top_state->script_lineno = tag->newline_count;

        /*
         * Now look for a SRC="url" attribute for known languages.
         * If found, synchronously load the url.
         */
        url = NULL;
        if (top_state->in_script != SCRIPT_TYPE_NOT) {
            buff = lo_FetchParamValue(context, tag, PARAM_SRC);  /* XXX overloaded rv */
            if (buff != NULL) {
                PA_LOCK(str, char *, buff);
                url = NET_MakeAbsoluteURL(top_state->base_url, str);
                PA_UNLOCK(buff);
                PA_FREE(buff);
                if (url == NULL) {
                    top_state->out_of_memory = TRUE;
                    return;
                }
            }
        }
        if (url != NULL) {
            if ((doc_data != NULL) &&
                (state->in_relayout == FALSE) &&
                (top_state->resize_reload == FALSE) &&
                (decoder = LM_GetMochaDecoder(context)) != NULL) {
                URL_Struct *url_struct;
                int status;

                url_struct = NET_CreateURLStruct(url, NET_NORMAL_RELOAD);
                if (url_struct == NULL) {
                    top_state->out_of_memory = TRUE;
                    LM_PutMochaDecoder(decoder);
                } else {
                    url_struct->must_cache = TRUE;
                    url_struct->preset_content_type = TRUE;
                    StrAllocCopy(url_struct->content_type, mocha_content_type);

                    decoder->nesting_url = XP_STRDUP(url_struct->address);
                    if (LM_SetDecoderStream(decoder,
                                            doc_data->parser_stream,
                                            url_struct)) {
                        status = NET_GetURL(url_struct, FO_CACHE_AND_PRESENT,
                                            context, lo_script_src_exit_fn);
                    } else {
                        status = -1;
                    }
                    if (status < 0) {
                        XP_FREE((char *)decoder->nesting_url);
                        decoder->nesting_url = 0;
                    } else {
                        /* Ignore contents of <SCRIPT SRC="url">...</SCRIPT>. */
                        top_state->in_script = SCRIPT_TYPE_UNKNOWN;

                        top_state->layout_blocking_element
                            = lo_NewElement(context, state, LO_SCRIPT, NULL, 0);
                        if (top_state->layout_blocking_element == NULL) {
                            top_state->out_of_memory = TRUE;
                        } else {
                            top_state->layout_blocking_element->lo_any.ele_id
                                = NEXT_ELEMENT;
                            tag = pa_CreateMDLTag(doc_data,
                                                  script_end_tag,
                                                  sizeof script_end_tag - 1);
                            if (tag != NULL) {
                                tag->next =
                                    pa_CreateMDLTag(doc_data,
                                                    script_suppress_tag,
                                                    sizeof script_suppress_tag
                                                    - 1);
                            }
                            if (tag == NULL) {
                                top_state->out_of_memory = TRUE;
                            } else {
                                if (top_state->tags == NULL)
                                    top_state->tags_end = &tag->next->next;
                                else
                                    tag->next->next = top_state->tags;
                                top_state->tags = tag;
                                top_state->mocha_write_point = &tag->next;
                                tag->true_len = tag->next->true_len = 0;
                                lo_BlockScriptTag(context, state, tag);
                                lo_BlockScriptTag(context, state, tag->next);
                            }
                        }
                    }

                    LM_PutMochaDecoder(decoder);
                }
            }
            XP_FREE(url);
        }
    } else {
        size_t line_buf_len;
        intn script_type;

        /*
         * Reset these before potentially recursing indirectly through
         * the document.write() built-in function, which writes to the
         * very same doc_data->parser_stream that this <SCRIPT> tag
         * came in on.
         */
        state->text_divert = P_UNKNOWN;
        line_buf_len = state->line_buf_len;
        state->line_buf_len = 0;
        script_type = top_state->in_script;
        top_state->in_script = SCRIPT_TYPE_NOT;

        if (script_type == SCRIPT_TYPE_MOCHA) {
            if ((doc_data != NULL) &&
                (state->in_relayout == FALSE) &&
                (top_state->resize_reload == FALSE)) {
                decoder = LM_GetMochaDecoder(context);
                if (decoder) {
                    MochaDatum result;

                    if (LM_SetDecoderStream(decoder,
                                            doc_data->parser_stream,
                                            doc_data->url_struct) &&
                        LM_EvaluateBuffer(decoder,
                                          (char *)state->line_buf,
                                          line_buf_len,
                                          top_state->script_lineno,
                                          &result)) {
                        MOCHA_DropRef(decoder->mocha_context, &result);
                    }

                    LM_PutMochaDecoder(decoder);
                }
            }
        }
    }
}

#endif /* MOCHA */
