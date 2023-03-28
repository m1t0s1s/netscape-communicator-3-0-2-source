/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*-
 */

/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#ifdef MOCHA
#include <stddef.h>
#include <memory.h>
#include <time.h>
#include "net.h"
#include "xp.h"
#include "libmocha.h"
#include "mkgeturl.h"
#include "mkmocha.h"
#include "shist.h"

#define MOCHA_CACHES_WRITES     1       /* XXX dups ../layout/layout.h */

extern int MK_OUT_OF_MEMORY;
extern int MK_MALFORMED_URL_ERROR;

PRIVATE int
mocha_process(void *data_object, const char *str, int32 len)
{
    MochaDecoder *decoder = LM_GetMochaDecoder(data_object);

    if (!decoder) return -1;
    decoder->length += len;
    if (!decoder->buffer) {
        decoder->buffer = (char *)XP_ALLOC(decoder->length);
    } else {
        decoder->buffer = (char *)XP_REALLOC(decoder->buffer, decoder->length);
    }
    if (!decoder->buffer) {
        MOCHA_ReportOutOfMemory(decoder->mocha_context);
        return -1;
    }
    memcpy(decoder->buffer + decoder->offset, str, len);
    decoder->offset += len;
    LM_PutMochaDecoder(decoder);
    return len;
}

PRIVATE unsigned int
mocha_ready(void *data_object)
{
    return MAX_WRITE_READY;  /* always ready for writing */
}

PRIVATE void 
mocha_complete(void *data_object)
{
    MochaDecoder *decoder = LM_GetMochaDecoder(data_object);
    MochaContext *mc;
    MochaDatum result;
    MochaAtom *atom;
    char *str;

    if (!decoder) return;
    mc = decoder->mocha_context;

    if (LM_EvaluateBuffer(decoder, decoder->buffer, decoder->length, 0,
                          &result)) {
        if (decoder->stream &&
            !decoder->nesting_url &&
            result.tag != MOCHA_UNDEF &&
            MOCHA_DatumToString(mc, result, &atom)) {
            str = MOCHA_strdup(mc, MOCHA_GetAtomName(mc, atom));
            MOCHA_DropAtom(mc, atom);
            if (str) {
                (*decoder->stream->put_block)(decoder->stream->data_object,
                                              str, XP_STRLEN(str));
                MOCHA_free(mc, str);
            }
        }
        MOCHA_DropRef(mc, &result);
    }

    if (decoder->stream && !decoder->nesting_url) {
        (*decoder->stream->complete)(decoder->stream->data_object);
        XP_DELETE(decoder->stream);
        decoder->stream = 0;
    }
    LM_ClearDecoderStream(decoder);
    XP_FREE(decoder->buffer);
    decoder->buffer = 0;
    decoder->length = 0;
    decoder->offset = 0;
    LM_PutMochaDecoder(decoder);
}

PRIVATE void 
mocha_abort(void *data_object, int status)
{
    MochaDecoder *decoder = LM_GetMochaDecoder(data_object);

    if (!decoder) return;
    if (decoder->stream) {
        (*decoder->stream->abort)(decoder->stream->data_object, status);
        XP_DELETE(decoder->stream);
        decoder->stream = 0;
    }
    LM_ClearDecoderStream(decoder);
    XP_FREE(decoder->buffer);
    decoder->buffer = 0;
    decoder->length = 0;
    decoder->offset = 0;
    LM_PutMochaDecoder(decoder);
}

NET_StreamClass *
NET_CreateMochaConverter(FO_Present_Types format_out,
                         void             *data_object,
                         URL_Struct       *url_struct,
                         MWContext        *context)
{
    MochaDecoder *decoder;
    NET_StreamClass *stream, *html_stream;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
        return 0;

    stream = NET_NewStream("mocha", mocha_process, mocha_complete, mocha_abort,
                           mocha_ready, context, context); 
    if (!decoder->stream) {
        StrAllocCopy(url_struct->content_type, TEXT_HTML);
        html_stream = NET_StreamBuilder(FO_PRESENT, url_struct, context);
        if (html_stream)
            LM_SetDecoderStream(decoder, html_stream, url_struct);
    }
    LM_PutMochaDecoder(decoder);
    return stream;
}

/*
 * Mocha interpreter text widget.
 */
MODULE_PRIVATE int
net_OpenMochaURL(ActiveEntry *ae)
{
    MWContext *context;
    MochaDecoder *decoder;
    MochaContext *mc;
    char *what, *prefix;
    MochaAtom *atom;
    Bool eval_what, single_shot;
    URL_Struct *save_url;
    MochaBoolean ok;
    MochaDatum result;
    Bool first_time;
    NET_StreamClass *stream;
    int status;

    context = ae->window_id;
    decoder = LM_GetMochaDecoder(context);
    if (!decoder) {
        ae->status = MK_OUT_OF_MEMORY;
        return -1;
    }
    XP_ASSERT(decoder->window_context == ae->window_id);
    mc = decoder->mocha_context;

    what = XP_STRCHR(ae->URL_s->address, ':');
    XP_ASSERT(what);
    what++;
    eval_what = FALSE;
    single_shot = (*what != '?');

    if (single_shot) {
        /* Don't use an existing Mocha output window's stream. */
        stream = 0;
        if (*what == '\0') {
            /* Generate two grid cells, one for output and one for input. */
            what = PR_smprintf("<frameset rows=\"75%%,25%%\">\n"
                               "<frame name=MochaOutput src=about:blank>\n"
                               "<frame name=MochaInput src=%.*s#input>\n"
                               "</frameset>",
                               what - ae->URL_s->address,
                               ae->URL_s->address);
        } else if (!XP_STRCMP(what, "#input")) {
            /* The input cell contains a form with one magic isindex field. */
            what = PR_smprintf("<b>%.*s typein</b>\n"
                               "<form action=%.*s target=MochaOutput"
                               " onsubmit='this.isindex.focus()'>\n"
                               "<input name=isindex size=60>\n"
                               "</form>",
                               what - ae->URL_s->address - 1,
                               ae->URL_s->address,
                               what - ae->URL_s->address,
                               ae->URL_s->address);
            ae->URL_s->internal_url = TRUE;
        } else {
            eval_what = TRUE;
        }
    } else {
        /* Use the Mocha output window if it exists. */
        stream = decoder->stream;
        ae->URL_s->auto_scroll = 1000;

        /* Skip the leading question-mark and clean up the string. */
        what++;
        NET_PlusToSpace(what);
        NET_UnEscape(what);
        eval_what = TRUE;
    }

    /* If there's a Mocha expression after the :, evaluate it. */
    if (what && eval_what) {
        save_url = decoder->url_struct;
        if (!save_url)
            decoder->url_struct = ae->URL_s;
        ok = LM_EvaluateBuffer(decoder, what, XP_STRLEN(what), 0, &result);
        if (!save_url)
            decoder->url_struct = 0;
        if (!ok) {
            LM_PutMochaDecoder(decoder);
            ae->status = MK_MALFORMED_URL_ERROR;
            return -1;
        }
        if (result.tag == MOCHA_UNDEF ||
            !MOCHA_DatumToString(mc, result, &atom)) {
            MOCHA_DropRef(mc, &result);
            LM_PutMochaDecoder(decoder);
            ae->status = MK_DATA_LOADED;
            return -1;
        }
        MOCHA_MIX_TAINT(mc, decoder->write_taint_accum, result.taint);
        what = XP_STRDUP(MOCHA_GetAtomName(mc, atom));
        MOCHA_DropAtom(mc, atom);
        MOCHA_DropRef(mc, &result);
    }

    /* The what string must end in a newline so the parser will flush it.  */
    StrAllocCat(what, "\n");
    if (!what) {
        LM_PutMochaDecoder(decoder);
        ae->status = MK_OUT_OF_MEMORY;
        return -1;
    }

    first_time = !stream;
    if (first_time) {
        StrAllocCopy(ae->URL_s->content_type, TEXT_HTML);
        ae->format_out = CLEAR_CACHE_BIT(ae->format_out);
        ae->socket = ae->con_sock = SOCKET_INVALID;

        stream = NET_StreamBuilder(ae->format_out, ae->URL_s, ae->window_id);
        if (!stream) {
            XP_FREE(what);
            LM_PutMochaDecoder(decoder);
            ae->status = MK_UNABLE_TO_CONVERT;
            return -1;
        }
    }

    status = 0;
    if (first_time && eval_what &&
        ae->format_out != FO_INTERNAL_IMAGE &&
        ae->format_out != FO_MULTIPART_IMAGE &&
        ae->format_out != FO_EMBED &&
        ae->format_out != FO_PLUGIN) {
#ifdef MOCHA_CACHES_WRITES
        (void) LM_WysiwygCacheConverter(context, ae->URL_s, stream);
#endif
        if (*what == '<') {
            prefix = LM_GetBaseHrefTag(mc);
        } else {
            prefix = 0;
            StrAllocCopy(prefix, "<TITLE>");
            StrAllocCat(prefix, ae->URL_s->address);
            StrAllocCat(prefix, "</TITLE><PLAINTEXT>");
            if (!prefix)
                MOCHA_ReportOutOfMemory(mc);
        }
        if (!prefix) {
            status = -1;
        } else {
            status = (*stream->put_block)(stream->data_object, prefix,
                                          XP_STRLEN(prefix));
            XP_FREE(prefix);
        }
    }

    if (status >= 0) {
        status = (*stream->put_block)(stream->data_object, what,
                                      XP_STRLEN(what));
    }
    XP_FREE(what);

    if (single_shot && status >= 0)
        (*stream->complete)(stream->data_object);

    if (!single_shot && status >= 0) {
        if (first_time) {
            LM_SetDecoderStream(decoder, stream, ae->URL_s);
            LM_PutMochaDecoder(decoder);
            return 0;
        }
    } else {
        if (status < 0)
            (*stream->abort)(stream->data_object, status);
        if (!first_time)
            LM_ClearDecoderStream(decoder);
        XP_DELETE(stream);
    }

    LM_PutMochaDecoder(decoder);
    ae->status = MK_DATA_LOADED;
    return -1;
}

MODULE_PRIVATE void
net_AbortMochaURL(ActiveEntry *ae)
{
    MWContext *context;
    MochaDecoder *decoder;
    NET_StreamClass *stream;

    context = ae->window_id;
    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
        return;
    XP_ASSERT(decoder->window_context == ae->window_id);
    stream = decoder->stream;
    if (stream) {
        LM_ClearDecoderStream(decoder);
        (*stream->abort)(stream->data_object, MK_INTERRUPTED);
        FREE(stream);
    }
    LM_PutMochaDecoder(decoder);
}

MODULE_PRIVATE int
net_InterruptMocha(ActiveEntry *ae)
{
    return ae->status = MK_INTERRUPTED;
}
#endif /* MOCHA */
