/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*-
 */
#ifdef MOCHA
/*
 * Mocha layout interface.
 */
#include "xp.h"
#include "lo_ele.h"
#include "layout.h"
#include "libmocha.h"
#include "pa_parse.h"
#ifdef JAVA
#include "mo_java.h"
#endif

void
lo_MarkMochaTopState(lo_TopState *top_state, MWContext *context)
{
    MochaDecoder *decoder;

    if (top_state == NULL || top_state->mocha_mark)
        return;
    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
        return;
    LM_PutMochaDecoder(decoder);
    top_state->mocha_mark = TRUE;
}

lo_TopState *
lo_GetTopState(MWContext *context)
{
    int32 doc_id;
    lo_TopState *top_state;

    doc_id = XP_DOCID(context);
    top_state = lo_FetchTopState(doc_id);
    if (top_state != NULL && top_state->doc_state == NULL)
        return NULL;
    return top_state;
}

lo_TopState *
lo_GetMochaTopState(MWContext *context)
{
    lo_TopState *top_state;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;
    lo_MarkMochaTopState(top_state, context);
    return top_state;
}

lo_FormData *
LO_GetFormDataByID(MWContext *context, intn form_id)
{
    lo_TopState *top_state;
    lo_FormData *form;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;

    for (form = top_state->form_list; form != NULL; form = form->next) {
        if (form->id == form_id)
            return form;
    }
    return NULL;
}

uint
LO_EnumerateForms(MWContext *context)
{
    lo_TopState *top_state;
    lo_FormData *form;

    top_state = lo_GetMochaTopState(context);
    if (top_state == NULL)
        return 0;

    /* Try to reflect all forms in case someone is enumerating. */
    for (form = top_state->form_list; form != NULL; form = form->next) {
        if (form->mocha_object == NULL)
            (void) LM_ReflectForm(context, form);
    }
    return top_state->current_form_num;
}

/*
 * Note that index is zero-based from the first form, and if in the second
 * form, is one more than the index of the last element in the first form.
 * So we subtract form's first element index before subscripting.
 */
LO_FormElementStruct *
LO_GetFormElementByIndex(lo_FormData *form, int32 index)
{
    LO_Element **ele_list;
    LO_FormElementStruct *form_element;

    if (form->form_elements == NULL)
        return NULL;
    PA_LOCK(ele_list, LO_Element **, form->form_elements);
    form_element = (LO_FormElementStruct *)ele_list[0];
    index -= form_element->element_index;
    if ((uint32)index < (uint32)form->form_ele_cnt)
        form_element = (LO_FormElementStruct *)ele_list[index];
    else
        form_element = NULL;
    PA_UNLOCK(form->form_elements);
    if (form_element == NULL || form_element->element_data == NULL)
        return NULL;
    return form_element;
}

uint
LO_EnumerateFormElements(MWContext *context, lo_FormData *form)
{
    LO_Element **ele_list;
    LO_FormElementStruct *form_element;
    uint i;

    /* Try to reflect all elements in case someone is enumerating. */
    PA_LOCK(ele_list, LO_Element **, form->form_elements);
    for (i = 0; i < (uint)form->form_ele_cnt; i++) {
        form_element = (LO_FormElementStruct *)ele_list[i];
        if (form_element->mocha_object == NULL)
            (void) LM_ReflectFormElement(context, form_element);
    }
    PA_UNLOCK(form->form_elements);
    return form->form_ele_cnt;
}

static Bool
lo_compile_mocha_param(MWContext *context, PA_Tag *tag, MochaObject *object,
                       char *name, PA_Block block)
{
    MochaDecoder *decoder;
    char *body;
    Bool ok;

    decoder = LM_GetMochaDecoder(context);
    if (!decoder)
        return FALSE;

    PA_LOCK(body, char *, block);
    ok = MOCHA_CompileMethod(decoder->mocha_context, object, name, 0,
                             body, XP_STRLEN(body), LM_GetSourceURL(decoder),
                             tag->newline_count);
    PA_UNLOCK(block);
    LM_PutMochaDecoder(decoder);
    return ok;
}

void
lo_BeginReflectForm(MWContext *context, lo_DocState *doc_state, PA_Tag *tag,
                    lo_FormData *form_data)
{
    PA_Block name, onreset, onsubmit;
    MochaObject *form_object;
    MochaDecoder *decoder;

    name     = lo_FetchParamValue(context, tag, PARAM_NAME);
    onreset  = lo_FetchParamValue(context, tag, PARAM_ONRESET);
    onsubmit = lo_FetchParamValue(context, tag, PARAM_ONSUBMIT);
    if (name == NULL && onreset == NULL && onsubmit == NULL)
        return;
    lo_MarkMochaTopState(doc_state->top_state, context);
/* XXX begin common */
    form_object = LM_ReflectForm(context, form_data);
    if (form_object) {
        decoder = LM_GetMochaDecoder(context);
        if (decoder) {
            decoder->active_form = form_object;
            LM_PutMochaDecoder(decoder);
        }
/* XXX end common */
        if (onreset != NULL) {
            (void) lo_compile_mocha_param(context, tag, form_object,
                                          PARAM_ONRESET, onreset);
        }
        if (onsubmit != NULL) {
            (void) lo_compile_mocha_param(context, tag, form_object,
                                          PARAM_ONSUBMIT, onsubmit);
        }
    }
    if (name != NULL)
        PA_FREE(name);
    if (onreset != NULL)
        PA_FREE(onreset);
    if (onsubmit != NULL)
        PA_FREE(onsubmit);
}

LO_ImageStruct *
LO_GetImageByIndex(MWContext *context, intn index)
{
    lo_TopState *top_state;
    LO_ImageStruct *image;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;
    for (image = top_state->image_list; image != NULL;
         image = image->next_image) {
        if (!index--)
            return image;
    }
    return NULL;
}

uint
LO_EnumerateImages(MWContext *context)
{
    lo_TopState *top_state;
    uint image_num;
    LO_ImageStruct *image;

    top_state = lo_GetMochaTopState(context);
    if (top_state == NULL)
        return 0;

    /* Try to reflect all images in case someone is enumerating. */
    image_num = 0;
    for (image = top_state->image_list; image != NULL;
         image = image->next_image) {
        if (image->mocha_object == NULL)
            (void) LM_ReflectImage(context, image, NULL, image_num);
        image_num++;
    }
    return image_num;
}

void
lo_EndReflectForm(MWContext *context, lo_FormData *form_data)
{
    MochaDecoder *decoder;

    if (!context->mocha_context)
        return;
    decoder = LM_GetMochaDecoder(context);
    if (decoder) {
        XP_ASSERT(form_data->mocha_object == decoder->active_form);
        decoder->active_form = NULL;
        LM_PutMochaDecoder(decoder);
    }
}

void
lo_ReflectImage(MWContext *context, lo_DocState *doc_state, PA_Tag *tag,
                LO_ImageStruct *image, Bool blocked)
{
    PA_Block name, onload, onabort, onerror;
    uint index;
    MochaObject *image_object;
    lo_TopState *top_state = doc_state->top_state;

    /* Reflect only IMG tags, not form INPUT tags of TYPE=IMAGE */
    if (tag->type != P_IMAGE && tag->type != P_NEW_IMAGE)
        return;

    if (image->mocha_object) {
        if (! blocked) {
            /* Flush any image events that were sent when the image
               was blocked */
            LM_SendImageEvent(context, image, LM_IMGUNBLOCK);
        }
        return;
    }

    /* Add image to global list of images for this document. */
    if (! (image->image_attr->attrmask & LO_ATTR_ON_IMAGE_LIST)) {
        image->image_attr->attrmask |= LO_ATTR_ON_IMAGE_LIST;
        if (top_state->last_image)
            top_state->last_image->next_image = image;
        else
            top_state->image_list = image;
        top_state->last_image = image;
        index = (uint)top_state->image_count++;
    }

    /* The image is reflected into a Javascript object lazily:
       Immediate reflection only occurs if there are Javascript-only
       attributes in the IMG tag */
    name     = lo_FetchParamValue(context, tag, PARAM_NAME);
    onload   = lo_FetchParamValue(context, tag, PARAM_ONLOAD);
    onabort  = lo_FetchParamValue(context, tag, PARAM_ONABORT);
    onerror  = lo_FetchParamValue(context, tag, PARAM_ONERROR);
    if (name == NULL && onload == NULL && onabort == NULL && onerror == NULL)
        return;

    lo_MarkMochaTopState(top_state, context);

    image_object = LM_ReflectImage(context, image, (char*)name, index);
    if (image_object) {
        if (onload != NULL)
            (void) lo_compile_mocha_param(context, tag, image_object,
                                          PARAM_ONLOAD, onload);
        if (onabort != NULL)
            (void) lo_compile_mocha_param(context, tag, image_object,
                                          PARAM_ONABORT, onabort);
        if (onerror != NULL)
            (void) lo_compile_mocha_param(context, tag, image_object,
                                          PARAM_ONERROR, onerror);

        /* Since the image is unblocked, there is no need to buffer
           any JavaScript image events.  Deliver them as they're generated. */
        if (! blocked)
            LM_SendImageEvent(context, image, LM_IMGUNBLOCK);
    }

    if (name != NULL)
        PA_FREE(name);
    if (onload != NULL)
        PA_FREE(onload);
    if (onabort != NULL)
        PA_FREE(onabort);
    if (onerror != NULL)
       PA_FREE(onerror);
}

void
lo_CompileEventHandler(MWContext *context, lo_DocState *doc_state, PA_Tag *tag,
                       LO_FormElementStruct *form_element, char *name)
{
    PA_Block block;
    MochaObject *element_object;
    lo_FormData *form_data;
    MochaObject *form_object;
    MochaDecoder *decoder;

    /* Don't reflect form elements twice, it makes arrays out of them! */
    if (doc_state->in_relayout)
        return;

    block = lo_FetchParamValue(context, tag, name);
    if (block == NULL)
        return;
    lo_MarkMochaTopState(doc_state->top_state, context);
    element_object = form_element->mocha_object;
    if (element_object == NULL) {
        form_data = doc_state->top_state->form_list;
        if (form_data != NULL) {
            /* XXX begin common */
            form_object = LM_ReflectForm(context, form_data);
            if (form_object) {
                decoder = LM_GetMochaDecoder(context);
                if (decoder) {
                    decoder->active_form = form_object;
                    LM_PutMochaDecoder(decoder);
                }
            }
            /* XXX end common */
        }
        element_object = LM_ReflectFormElement(context, form_element);
    }
    if (element_object != NULL) {
        (void) lo_compile_mocha_param(context, tag, element_object,
                                      name, block);
    }
    PA_FREE(block);
}

void
lo_ReflectNamedAnchor(MWContext *context, lo_DocState *doc_state, PA_Tag *tag,
                      lo_NameList *name_rec)
{
    uint count;
    MochaObject *object;

    lo_MarkMochaTopState(doc_state->top_state, context);
    count = LO_EnumerateNamedAnchors(context);
    object = LM_ReflectNamedAnchor(context, name_rec, count - 1);
    /* XXX */
}

lo_NameList *
LO_GetNamedAnchorByIndex(MWContext *context, uint index)
{
    lo_TopState *top_state;
    lo_DocState *doc_state;
    lo_NameList *name_rec;
    uint count;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;
    doc_state = top_state->doc_state;

    /* The list is in reverse-source order, so we have count anchors. */
    count = 0;
    for (name_rec = doc_state->name_list; name_rec != NULL;
         name_rec = name_rec->next) {
        count++;
    }
    if (count <= index)
        return NULL;

    count -= index;
    for (name_rec = doc_state->name_list; name_rec != NULL;
         name_rec = name_rec->next) {
        if (--count == 0)
            break;
    }
    return name_rec;
}

uint
LO_EnumerateNamedAnchors(MWContext *context)
{
    lo_TopState *top_state;
    lo_DocState *doc_state;
    lo_NameList *name_rec;
    uint count;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return 0;
    doc_state = top_state->doc_state;

    count = 0;
    for (name_rec = doc_state->name_list; name_rec != NULL;
         name_rec = name_rec->next) {
        count++;
    }
    return count;
}

void
lo_ReflectLink(MWContext *context, lo_DocState *doc_state, PA_Tag *tag,
               LO_AnchorData *anchor_data, uint index)
{
    PA_Block onclick, onmouseover, onmouseout;
    MochaObject *object;

    onclick = lo_FetchParamValue(context, tag, PARAM_ONCLICK);
    onmouseover = lo_FetchParamValue(context, tag, PARAM_ONMOUSEOVER);
    onmouseout  = lo_FetchParamValue(context, tag, PARAM_ONMOUSEOUT);
    if (onclick || onmouseover || onmouseout) {
        lo_MarkMochaTopState(doc_state->top_state, context);
        object = LM_ReflectLink(context, anchor_data, index);
    } else {
        object = NULL;
    }
    if (onclick) {
        if (object) {
            (void) lo_compile_mocha_param(context, tag, object,
                                          PARAM_ONCLICK, onclick);
        }
        PA_FREE(onclick);
    }
    if (onmouseover) {
        if (object) {
            (void) lo_compile_mocha_param(context, tag, object,
                                          PARAM_ONMOUSEOVER, onmouseover);
        }
        PA_FREE(onmouseover);
    }
    if (onmouseout) {
        if (object) {
            (void) lo_compile_mocha_param(context, tag, object,
                                          PARAM_ONMOUSEOUT, onmouseout);
        }
        PA_FREE(onmouseout);
    }
}

LO_AnchorData *
LO_GetLinkByIndex(MWContext *context, uint index)
{
    lo_TopState *top_state;
    LO_AnchorData **anchor_array;
    LO_AnchorData *anchor_data;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;
    if (index >= (uint)top_state->url_list_len)
        return NULL;

    XP_LOCK_BLOCK(anchor_array, LO_AnchorData **, top_state->url_list);
    anchor_data = anchor_array[index];
    XP_UNLOCK_BLOCK(top_state->url_list);
    return anchor_data;
}

uint
LO_EnumerateLinks(MWContext *context)
{
    lo_TopState *top_state;
    uint count, index;
    LO_AnchorData **anchor_array;
    LO_AnchorData *anchor_data;

    top_state = lo_GetMochaTopState(context);
    if (top_state == NULL)
        return 0;

    /* Try to reflect all links in case someone is enumerating. */
    count = (uint)top_state->url_list_len;
    XP_LOCK_BLOCK(anchor_array, LO_AnchorData **, top_state->url_list);
    for (index = 0; index < count; index++) {
        anchor_data = anchor_array[index];
        if (anchor_data->mocha_object == NULL)
            (void) LM_ReflectLink(context, anchor_data, index);
    }
    XP_UNLOCK_BLOCK(top_state->url_list);
    return count;
}

#ifdef JAVA
LO_JavaAppStruct *
LO_GetAppletByIndex(MWContext *context, uint index)
{
    lo_TopState *top_state;
    LO_JavaAppStruct *applet;
    int i, count;

    /* XXX */
    if (!MOCHA_JavaGlueEnabled())
        return NULL;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;

    /* count 'em */
    count = 0;
    applet = top_state->applet_list;
    while (applet) {
        applet = applet->nextApplet;
        count++;
    }

    /* reverse order... */
    applet = top_state->applet_list;
    for (i = count-1; i >= 0; i--) {
        if (i == index)
            return applet;
        applet = applet->nextApplet;
    }
    return NULL;
}

uint
LO_EnumerateApplets(MWContext *context)
{
    lo_TopState *top_state;
    int count, index;
    LO_JavaAppStruct *applet;

    /* XXX */
    if (!MOCHA_JavaGlueEnabled())
        return 0;

    top_state = lo_GetMochaTopState(context);
    if (top_state == NULL)
        return 0;

    /* count 'em */
    count = 0;
    applet = top_state->applet_list;
    while (applet) {
        applet = applet->nextApplet;
        count++;
    }

    /* reflect all applets in reverse order */
    applet = top_state->applet_list;
    for (index = count-1; index >= 0; index--) {
        if (applet->mocha_object == NULL) {
            (void) LM_ReflectApplet(context, applet, index);
        }
        applet = applet->nextApplet;
    }

    return count;
}

LO_EmbedStruct *
LO_GetEmbedByIndex(MWContext *context, uint index)
{
    lo_TopState *top_state;
    LO_EmbedStruct *embed;
    int i, count;

    /* XXX */
    if (!MOCHA_JavaGlueEnabled())
        return NULL;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return NULL;

    /* count 'em */
    count = 0;
    embed = top_state->embed_list;
    while (embed) {
        embed = embed->nextEmbed;
        count++;
    }

    /* reverse order... */
    embed = top_state->embed_list;
    for (i = count-1; i >= 0; i--) {
        if (i == index)
            return embed;
        embed = embed->nextEmbed;
    }
    return NULL;
}

uint
LO_EnumerateEmbeds(MWContext *context)
{
    lo_TopState *top_state;
    int count, index;
    LO_EmbedStruct *embed;

    /* XXX */
    if (!MOCHA_JavaGlueEnabled())
        return 0;

    top_state = lo_GetMochaTopState(context);
    if (top_state == NULL)
        return 0;

    /* count 'em */
    count = 0;
    embed = top_state->embed_list;
    while (embed) {
        embed = embed->nextEmbed;
        count++;
    }

    /* reflect all embeds in reverse order */
    embed = top_state->embed_list;
    for (index = count-1; index >= 0; index--) {
        if (embed->mocha_object == NULL) {
            (void) LM_ReflectEmbed(context, embed, index);
        }
        embed = embed->nextEmbed;
    }

    return count;
}
#endif /* JAVA */


/* XXX lo_DocState should use a colors[LO_NCOLORS] array to shrink code here
       and in layout.c.
 */
void
LO_GetDocumentColor(MWContext *context, int type, LO_Color *color)
{
    lo_TopState *top_state;
    lo_DocState *doc_state;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return;
    doc_state = top_state->doc_state;

    switch (type) {
      case LO_COLOR_BG:
        *color = doc_state->text_bg;
        break;
      case LO_COLOR_FG:
        *color = doc_state->text_fg;
        break;
      case LO_COLOR_LINK:
        *color = doc_state->anchor_color;
        break;
      case LO_COLOR_VLINK:
        *color = doc_state->visited_anchor_color;
        break;
      case LO_COLOR_ALINK:
        *color = doc_state->active_anchor_color;
        break;
      default:
        break;
    }
}

void
LO_SetDocumentColor(MWContext *context, int type, LO_Color *color)
{
    lo_TopState *top_state;
    lo_DocState *doc_state;

    top_state = lo_GetTopState(context);
    if (top_state == NULL)
        return;
    doc_state = top_state->doc_state;
    if (color == NULL)
        color = &lo_master_colors[type];

    switch (type) {
      case LO_COLOR_BG:
        doc_state->text_bg = *color;
        top_state->doc_specified_bg = TRUE;
        lo_RecolorText(doc_state);
        FE_SetBackgroundColor(context, color->red, color->green, color->blue);
        break;
      case LO_COLOR_FG:
        doc_state->text_fg = *color;
        lo_RecolorText(doc_state);
        break;
      case LO_COLOR_LINK:
        doc_state->anchor_color = *color;
        break;
      case LO_COLOR_VLINK:
        doc_state->visited_anchor_color = *color;
        break;
      case LO_COLOR_ALINK:
        doc_state->active_anchor_color = *color;
        break;
      default:;
    }
}

void
lo_ProcessContextEventHandlers(MWContext *context, lo_DocState *doc_state,
                               PA_Tag *tag)
{
    PA_Block onload, onunload, onfocus, onblur;
    MochaDecoder *decoder;
    lo_TopState *top_state;

    onload = lo_FetchParamValue(context, tag, PARAM_ONLOAD);
    onunload = lo_FetchParamValue(context, tag, PARAM_ONUNLOAD);
    onfocus = lo_FetchParamValue(context, tag, PARAM_ONFOCUS);
    onblur = lo_FetchParamValue(context, tag, PARAM_ONBLUR);
    if (onload == NULL && onunload == NULL && onfocus == NULL && onblur == NULL)
        return;

    decoder = LM_GetMochaDecoder(context);
    if (decoder != NULL) {
        top_state = doc_state->top_state;
        lo_MarkMochaTopState(top_state, context);

        if (onload != NULL) {
            (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                          PARAM_ONLOAD, onload);
            if (tag->type == P_GRID) {
                top_state->savedData.OnLoad = onload;
                onload = NULL;
            } else if (tag->type == P_BODY) {
                top_state->mocha_has_onload = TRUE;
            }
        }

        if (onunload != NULL) {
            (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                          PARAM_ONUNLOAD, onunload);
            if (tag->type == P_GRID) {
                top_state->savedData.OnUnload = onunload;
                onunload = NULL;
            }
        }

        if (onfocus != NULL) {
            (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                          PARAM_ONFOCUS, onfocus);
            if (tag->type == P_GRID) {
                top_state->savedData.OnFocus = onfocus;
                onfocus = NULL;
            }
        }

        if (onblur != NULL) {
            (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                          PARAM_ONBLUR, onblur);
            if (tag->type == P_GRID) {
                top_state->savedData.OnBlur = onblur;
                onblur = NULL;
            }
        }
        LM_PutMochaDecoder(decoder);
    }

    if (onload != NULL)
        PA_FREE(onload);
    if (onunload != NULL)
        PA_FREE(onunload);
    if (onfocus != NULL)
        PA_FREE(onfocus);
    if (onblur != NULL)
        PA_FREE(onblur);
}

void
lo_RestoreContextEventHandlers(MWContext *context, lo_DocState *doc_state,
                               PA_Tag *tag, SHIST_SavedData *saved_data)
{
    PA_Block onload, onunload, onfocus, onblur;
    MochaDecoder *decoder;
    lo_TopState *top_state;

    onload = saved_data->OnLoad;
    onunload = saved_data->OnUnload;
    onfocus = saved_data->OnFocus;
    onblur = saved_data->OnBlur;
    if (onload == NULL && onunload == NULL && onfocus == NULL && onblur == NULL)
        return;

    decoder = LM_GetMochaDecoder(context);
    if (decoder == NULL)
        return;

    top_state = doc_state->top_state;
    lo_MarkMochaTopState(top_state, context);

    if (onload != NULL) {
        (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                      PARAM_ONLOAD, onload);
        top_state->savedData.OnLoad = onload;
    }

    if (onunload != NULL) {
        (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                      PARAM_ONUNLOAD, onunload);
        top_state->savedData.OnUnload = onunload;
    }

    if (onfocus != NULL) {
        (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                      PARAM_ONFOCUS, onfocus);
        top_state->savedData.OnFocus = onfocus;
    }

    if (onblur != NULL) {
        (void) lo_compile_mocha_param(context, tag, decoder->window_object,
                                      PARAM_ONBLUR, onblur);
        top_state->savedData.OnBlur = onblur;
    }

    LM_PutMochaDecoder(decoder);
}

#endif /* MOCHA */
