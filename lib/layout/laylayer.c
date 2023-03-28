/* -*- Mode: C; tab-width: 4; tabs-indent-mode: t -*- */

#include "xp.h"
#include "pa_tags.h"
#include "layout.h"
#include "layers.h"

/**********************
 *
 * BLINK tag-related layering code
 *
 **********************/

#define LO_BLINK_RATE 750

typedef struct lo_BlinkStruct {
    MWContext *context;
    lo_DocState *state;
    LO_TextStruct *text;
} lo_BlinkStruct;

static void
lo_blink_destroy_func(CL_Layer *layer)
{
    CL_RemoveLayerFromGroup(layer->compositor, layer, "_BLINKGROUP");
    XP_FREE(layer->author_data);
}

static FE_Region 
lo_blink_painter_func(void *drawable, 
                      CL_Layer *layer, 
                      FE_Region update_region,
                      XP_Bool allow_transparency)
{
    lo_BlinkStruct *closure = (lo_BlinkStruct *)layer->author_data;
  
    /* This layer can't draw in the back-to-front phase */
    if (allow_transparency == FALSE)
        return NULL;

    /* Draw the text element associated with this layer */
    lo_DisplayText(closure->context, FE_VIEW, closure->text,
                   FALSE, TRUE);
}

static void
lo_blink_callback(void *closure)
{
    MWContext *context = (MWContext *)closure;

    if (context->blink_hidden == FALSE) {
        CL_HideLayerGroup(context->compositor, "_BLINKGROUP");
        context->blink_hidden = TRUE;
    }
    else {
        CL_UnhideLayerGroup(context->compositor, "_BLINKGROUP");
        context->blink_hidden = FALSE;
    }

    /* Set the next timeout */
    context->blink_timeout = FE_SetTimeout(lo_blink_callback,
                                           (void *)context,
                                           LO_BLINK_RATE);
}

void
lo_CreateBlinkLayer (MWContext *context, lo_DocState *state,
                     LO_TextStruct *text)
{
    CL_Layer *blink_layer, *parent;
    lo_BlinkStruct *closure;
    XP_Rect bbox;

    closure = XP_NEW_ZAP(lo_BlinkStruct);
    closure->context = context;
    closure->state = state;
    closure->text = text;
        
    bbox.left = text->x + text->x_offset;
    bbox.top = text->y + text->y_offset;
    bbox.right = bbox.left + text->width;
    bbox.bottom = bbox.top + text->height;
        
    blink_layer = CL_NewLayer(&bbox);
    blink_layer->direct = TRUE;
    blink_layer->painter_func = lo_blink_painter_func;
    blink_layer->destroy_func = lo_blink_destroy_func;
    blink_layer->author_data = (void *)closure;

    /* 
     * BUGBUG LAYERS: Right now we just use the _BASE layer. This 
     * code should find the layer associated with the text element.
     */
    parent = CL_FindLayer(context->compositor, "_BASE");
    CL_InsertChild(parent, blink_layer, NULL, CL_ABOVE);
    CL_AddLayerToGroup(context->compositor, blink_layer, "_BLINKGROUP");

    /* If noone has setup the callback yet, start it up */
    if (context->blink_timeout == NULL) {
        context->blink_hidden = FALSE;
        context->blink_timeout = FE_SetTimeout(lo_blink_callback,
                                               (void *)context,
                                               LO_BLINK_RATE);
    }
}

void
lo_DestroyBlinkers(MWContext *context)
{
    if (context->blink_timeout) {
        FE_ClearTimeout(context->blink_timeout);
        context->blink_timeout = NULL;
    }

#if 0
    CL_ForEachLayerInGroup(context->compositor, "_BLINKGROUP",
                           lo_blink_destroy_func, NULL);
#endif
}

/**********************
 *
 * HTML and background layer code
 *
 **********************/

/* Called for each rectangle in the update region */
static void
lo_html_rect_func(void *drawable,
				  XP_Rect *rect)
{
  MWContext *context = (MWContext *)drawable;

  LO_RefreshArea(context,
				 rect->left, rect->top,
				 (rect->right - rect->left),
				 (rect->bottom - rect->top));
}

/* painter_func for HTML document layers */
FE_Region 
lo_html_painter_func(void *drawable, 
					 CL_Layer *layer, 
					 FE_Region update_region,
					 XP_Bool allow_transparency)
{
  /* If this is the front-to-back pass, we can't draw here */
  if (!allow_transparency)
	return NULL;

  FE_ForEachRectInRegion(update_region, 
						 (FE_RectInRegionFunc)lo_html_rect_func,
						 (void *)drawable);

  return update_region;
}


/* Called for each rectangle in the update region */
static void
lo_bgcolor_rect_func(void *drawable,
					 XP_Rect *rect)
{
  MWContext *context = (MWContext *)drawable;

  FE_EraseBackground(context, FE_VIEW, rect->left, rect->top,
					 (rect->right - rect->left), (rect->bottom - rect->top));
}

/* painter_func for a HTML background layer */
FE_Region
lo_html_bgcolor_painter_func(void *drawable, 
							 CL_Layer *layer, 
							 FE_Region update_region,
							 XP_Bool allow_transparency)
{
  FE_ForEachRectInRegion(update_region, 
						 (FE_RectInRegionFunc)lo_bgcolor_rect_func,
						 (void *)drawable);

  return update_region;
}

/* 
 * Creates a HTML layer (either the _BASE layer or one created
 * using the <LAYER> tag.
 */
CL_Layer *
lo_CreateHTMLLayer(MWContext *context, char *name, int32 x, int32 y,
                   uint32 width, uint32 height, Bool scrolling,
                   Bool hidden, Bool direct)
{
    CL_Layer *layer;
    XP_Rect bbox;
    
    bbox.left = x;
    bbox.top = y;
    bbox.right = x + width;
    bbox.bottom = y + width;
    
    layer = CL_NewLayer(&bbox);
    layer->name = name;
    layer->direct = direct;
    layer->scrolling = scrolling;
    layer->hidden = hidden;
    /* Use the default HTML layer func */
    layer->painter_func = lo_html_painter_func;
    layer->author_data = (void *)context;
    
    return layer;
}

/* Creates a HTML background layer */
CL_Layer *
lo_CreateHTMLBackgroundLayer(MWContext *context)
{
    CL_Layer *layer;
    XP_Rect bbox;
    
    bbox.left = bbox.right = bbox.top = bbox.bottom = 0;
    layer = CL_NewLayer(&bbox);
    layer->name = "_BGCOLOR";
    layer->direct = TRUE;
    layer->painter_func = lo_html_bgcolor_painter_func;
    layer->author_data = (void *)context;

    return layer;
}

/* Creates the _BASE and _BGCOLOR layers and adds them to the layer tree */
void
lo_CreateDefaultLayers(MWContext *context)
{
    CL_Layer *base_layer, *bg_layer, *doc_layer;
    
    doc_layer = CL_FindLayer(context->compositor, "_DOCUMENT");
    
    bg_layer = lo_CreateHTMLBackgroundLayer(context);
    CL_InsertChild(doc_layer, bg_layer, NULL, CL_ABOVE);

    base_layer = lo_CreateHTMLLayer(context, "_BASE", 0, 0, 0, 0,
                                    FALSE, FALSE, TRUE);
    CL_InsertChild(doc_layer, base_layer, NULL, CL_ABOVE);
}

static void
lo_destroy_layer_recursive(CL_Layer *layer, void *arg)
{
    CL_Layer *parent = (CL_Layer *)arg;
    
    CL_RemoveChild(parent, layer);
    CL_DestroyLayerTree(layer);
}

/* Destroy all HTML-created layers */
void
lo_DestroyLayers(MWContext *context)
{
    CL_Layer *doc_layer;
    
    doc_layer = CL_FindLayer(context->compositor, "_DOCUMENT");

    /* BUGBUG LAYERS For now, destroy all children of the _DOCUMENT layer */
    CL_ForEachChildOfLayer(doc_layer, lo_destroy_layer_recursive, 
                           (void *)doc_layer);
}
