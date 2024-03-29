#ifndef libmocha_h___
#define libmocha_h___
/*
** Header file for Mocha in the Navigator (libmocha).
**
** Brendan Eich, 9/8/95
*/
#include "ntypes.h"
#include "prmacros.h"
#ifndef XP_MAC
#ifndef _mochaapi_h_
#include <mocha/mochaapi.h>
#endif
#else
#include "mochaapi.h"
#endif

NSPR_BEGIN_EXTERN_C

typedef struct MochaTimeout MochaTimeout;
typedef struct MochaNextURL MochaNextURL;

/*
** There exists one MochaDecoder per top-level MWContext that decodes Mocha,
** either from an HTML page or from a "mocha:[expr]" URL.
*/
typedef struct MochaDecoder {
    MochaContext    *mocha_context;
    MWContext       *window_context;
    MochaObject     *window_object;
    NET_StreamClass *stream;
    URL_Struct      *url_struct;
    MochaTimeout    *timeouts;
    MochaNextURL    *next_url_list;
    MochaNextURL    **url_list_tail;
    PRPackedBool    replace_location;
    PRPackedBool    resize_reload;
    PRPackedBool    load_event_sent;
    PRPackedBool    domain_set;
    PRPackedBool    visited;
    PRPackedBool    writing_input;
    PRPackedBool    in_window_quota;
    uint16          write_taint_accum;
    const char      *origin_url;
    const char      *source_url;
    const char      *nesting_url;
    char            *buffer;
    size_t          length;
    size_t          offset;
    uint32          branch_count;
    uint32          error_count;
    uint32          eval_level;
    uint32          event_mask;
    MochaObject     *opener;
    MochaObject     *form_prototype;
    MochaObject     *image_prototype;
    MochaObject     *input_prototype;
    MochaObject     *option_prototype;
    MochaObject     *url_prototype;
    MochaObject     *active_form;
    MochaObject     *document;
    MochaObject     *history;
    MochaObject     *location;
    MochaObject     *navigator;
    MochaObject     *forms;
    MochaObject     *links;
    MochaObject     *anchors;
    MochaObject     *applets;
    MochaObject	    *embeds;
    MochaObject     *images;

    /* XXX complete hack for anonymous image reparenting in frame resize */
    struct il_client_struct 
		    *anon_image_clients;
    void            *win_close_toid;
} MochaDecoder;

/*
** Public, well-known string constants.
*/
extern char mocha_language_name[];      /* "JavaScript" */
extern char mocha_content_type[];       /* "application/x-javascript" */

/*
** Initialize and finalize Mocha-in-the-client.
*/
extern void LM_InitMocha(void);
extern void LM_FinishMocha(void);

/*
** Enable/disable for Mocha-in-the-client.
*/
#define LM_SwitchMocha(toggle)	LM_SetMochaEnabled(toggle)

extern void
LM_SetMochaEnabled(MochaBoolean toggle);

extern MochaBoolean
LM_GetMochaEnabled(void);

/*
** Get (create if necessary) a MochaDecoder for context, adding a reference
** to its window_object.  Put drops the reference, destroying window_object
** when the count reaches zero.
*/
extern MochaDecoder *
LM_GetMochaDecoder(MWContext *context);

extern void
LM_PutMochaDecoder(MochaDecoder *decoder);

/*
** Called from NET_GetURL: check that this URL fetch does not convey tainted
** data via document.write() of a URL-fetching tag such as IMG or EMBED.
*/
extern MochaBoolean
LM_CheckGetURL(MWContext *context, URL_Struct *url_struct);

/*
** Get the source URL for script being loaded by document.  This URL will be
** the document's URL for inline script, or the SRC= URL for included script.
** The returned pointer is safe only within the extent of the function that
** calls LM_GetSourceURL().
*/
extern const char *
LM_GetSourceURL(MochaDecoder *decoder);

/*
** Evaluate the contents of a SCRIPT tag.
*/
extern MochaBoolean
LM_EvaluateBuffer(MochaDecoder *decoder, char *base, size_t length,
		  uint lineno, MochaDatum *result);

/*
** Evaluate an expression entity in an HTML attribute (WIDTH="&{height/2};").
** Returns null on error, otherwise a pointer to the malloc'd string result.
** The caller is responsible for freeing the string result.
*/
extern char *
LM_EvaluateAttribute(MWContext *context, char *expr, uint lineno);

/*
** Remove any MochaDecoder window_context pointer to an MWContext that's
** being destroyed.
*/
extern void
LM_RemoveWindowContext(MWContext *context);

extern void
LM_DropSavedWindow(MWContext *context, void *window);

/*
** Get and put the global, navigator MochaDecoder.  This decoder has access
** to all URL types, privileged data, etc.  It never runs code downloaded in
** a random HTML page.
*/
extern MochaDecoder *
LM_GetGlobalMochaDecoder(MWContext *context);

extern void
LM_PutGlobalMochaDecoder(MochaDecoder *decoder);

/*
** Set and clear the HTML stream and URL for the given MochaDecoder.
*/
extern MochaBoolean
LM_SetDecoderStream(MochaDecoder *decoder, NET_StreamClass *stream,
		    URL_Struct *url_struct);

extern void
LM_ClearDecoderStream(MochaDecoder *decoder);

extern void
LM_ClearContextStream(MWContext *context);

/*
** Start caching HTML or plain text generated by document.write() where the
** script is running on mc, the document is being generated into decoder's
** window, and url_struct tells about the generator.
*/
extern NET_StreamClass *
LM_WysiwygCacheConverter(MWContext *context, URL_Struct *url_struct,
			 NET_StreamClass *stream);

/*
** Skip over the "wysiwyg://docid/" in url_string and return a pointer to the
** real URL hidden after the prefix.
**
** Returns null if url_string does not start with "wysiwyg://docid/" and have
** more characters before the end of string.
*/
extern const char *
LM_SkipWysiwygURLPrefix(const char *url_string);

/*
** Return a pointer to a malloc'd string of the form "<BASE HREF=...>" where
** the "..." URL is the directory of mc's origin URL.  Such a base URL is the
** default base for relative URLs in generated HTML.
*/
extern char *
LM_GetBaseHrefTag(MochaContext *mc);

/*
** XXX Make these public LO_... typedefs in lo_ele.h/ntypes.h?
*/
struct lo_FormData_struct;
struct lo_NameList_struct;

/*
** Reflect the form described by form_data into Mocha.
*/
extern MochaObject *
LM_ReflectForm(MWContext *context, struct lo_FormData_struct *form_data);

extern struct lo_FormData_struct *
LO_GetFormDataByID(MWContext *context, intn form_id);

extern uint
LO_EnumerateForms(MWContext *context);

/*
** Reflect HTML images into Mocha.
*/
extern MochaObject *
LM_ReflectImage(MWContext *context, struct LO_ImageStruct_struct *image,
                char *name, uint index);

extern struct LO_ImageStruct_struct *
LO_GetImageByIndex(MWContext *context, intn image_id);

extern uint
LO_EnumerateImages(MWContext *context);

/*
** Conditionally create a MochaObject that reflects form_element as a
** property of the current form.
*/
extern MochaObject *
LM_ReflectFormElement(MWContext *context, LO_FormElementStruct *form_element);

extern LO_FormElementStruct *
LO_GetFormElementByIndex(struct lo_FormData_struct *form_data, int32 index);

extern uint
LO_EnumerateFormElements(MWContext *context,
			 struct lo_FormData_struct *form_data);

/*
** Reflect a named anchor into Mocha.
*/
extern MochaObject *
LM_ReflectNamedAnchor(MWContext *context, struct lo_NameList_struct *name_rec,
		      uint index);

/*
** Layout helper function to find a named anchor by its index in the
** document.anchors[] array.
*/
extern struct lo_NameList_struct *
LO_GetNamedAnchorByIndex(MWContext *context, uint index);

extern uint
LO_EnumerateNamedAnchors(MWContext *context);

/*
** Reflect a hyperlink into the document.links[] object, indexed by its
** order number within the document.
*/
extern MochaObject *
LM_ReflectLink(MWContext *context, LO_AnchorData *anchor_data, uint index);

/*
** Layout Mocha helper function to find an HREF Anchor by its index in the
** document.links[] array.
*/
extern LO_AnchorData *
LO_GetLinkByIndex(MWContext *context, uint index);

extern uint
LO_EnumerateLinks(MWContext *context);

/*
** Reflect an applet into Mocha.
*/
extern MochaObject *
LM_ReflectApplet(MWContext *context, LO_JavaAppStruct *lo_applet, uint index);

extern LO_JavaAppStruct *
LO_GetAppletByIndex(MWContext *context, uint index);

extern uint
LO_EnumerateApplets(MWContext *context);

/*
** Reflect an embed into Mocha.
*/
extern MochaObject *
LM_ReflectEmbed(MWContext *context, LO_EmbedStruct *lo_embed, uint index);

extern LO_EmbedStruct *
LO_GetEmbedByIndex(MWContext *context, uint index);

extern uint
LO_EnumerateEmbeds(MWContext *context);

/*
** Get and set a color attribute in the current document state.
*/
extern void
LO_GetDocumentColor(MWContext *context, int type, LO_Color *color);

extern void
LO_SetDocumentColor(MWContext *context, int type, LO_Color *color);

/*
** Layout function to reallocate the lo_FormElementOptionData array pointed at
** by lo_FormElementSelectData's options member to include space for the number
** of options given by selectData->option_cnt.
*/
extern XP_Bool
LO_ResizeSelectOptions(lo_FormElementSelectData *selectData);

/*
** Discard the current document and all its subsidiary objects.
*/
extern void
LM_ReleaseDocument(MWContext *context, MochaBoolean resize_reload);

/*
** Send a load or unload event to context.
*/
typedef enum LM_LoadEvent {
    LM_ABORT,                   /* document load was aborted */
    LM_LOAD,                    /* document load has completed */
    LM_UNLOAD,                  /* document unload has completed */
    LM_XFER_DONE                /* inline image, etc., transfer done */
} LM_LoadEvent;

extern void LM_SendLoadEvent(MWContext *context, LM_LoadEvent event);

/*
** Scroll a window to the given point.
*/
extern void LM_SendOnScroll(MWContext *context, int32 x, int32 y);

/*
** The FE should call this when the following get input focus:
**  - INPUT fields of all types
**  - TEXTAREA input fields
**  - HREF Anchors
**
** These elements cannot be done easily -- on some platforms, all events in
** a popup are handled by the native window system.
**  - OPTION elements in a SELECT form element
*/
extern void LM_SendOnFocus(MWContext *context, LO_Element *element);

/*
** The FE should call this when the following lose input focus:
**  - INPUT fields of all types
**  - OPTION elements in a SELECT form element
**  - TEXTAREA input fields
**  - HREF Anchors
*/
extern void LM_SendOnBlur(MWContext *context, LO_Element *element);

/*
** The FE should call this when the following are selected:
**  - INPUT fields of all types
**  - OPTION elements in a SELECT form element
**  - TEXTAREA input fields
*/
extern void LM_SendOnSelect(MWContext *context, LO_Element *element);

/*
** The FE should call this when the following are modified:
**  - INPUT fields of all types
**  - TEXTAREA input fields
*/
extern void LM_SendOnChange(MWContext *context, LO_Element *element);

/*
** Call this function from LO_ResetForm() to let the form's onReset attribute
** event handler run and do specialized form element data reinitialization.
** Returns MOCHA_TRUE if the form reset should continue, MOCHA_FALSE if it
** should be aborted.
*/
extern MochaBoolean LM_SendOnReset(MWContext *context, LO_Element *element);

/*
** This one should be called when a form is submitted.  If there is an
** INPUT tag of type "submit", the Mocha author could write an "onClick"
** handler property of the submit button, but for forms that auto-submit
** we want an "onSubmit" form property handler.
**
** Returns MOCHA_TRUE if the submit should continue, MOCHA_FALSE if it
** should be aborted.
*/
extern MochaBoolean LM_SendOnSubmit(MWContext *context, LO_Element *element);

/*
** The FE should call this when the following are clicked:
**  - INPUT elements of all clickable types:
**    - button
**    - checkbox
**    - radio
**    - reset
**    - submit
**  - HREF Anchors
*/
extern MochaBoolean LM_SendOnClick(MWContext *context, LO_Element *element);
extern MochaBoolean
LM_SendOnClickToAnchor(MWContext *context, LO_AnchorData *anchor);


/*
** The FE should call this when the user mouses over a link, passing in the
** context of the closest enclosing frame or window, and the LO_TEXT element
** that contains the anchor text and the anchor_href pointer.
**
** If this function returns MOCHA_FALSE, the FE should update the status bar
** from the anchor's HREF attribute.  If this function returns MOCHA_TRUE,
** the FE should leave the status bar alone, because a 'return true' in the
** onMouseOver event handler means some Mocha script updated status.
*/
extern MochaBoolean
LM_SendMouseOver(MWContext *context, LO_Element *element);

extern MochaBoolean
LM_SendMouseOverAnchor(MWContext *context, LO_AnchorData *anchor);

/*
** The FE should call this when the user mouses out of an area in a client
** side image map, passing in the context of the enclosing frame or window,
** and the anchor_data pointer for the area.
**
** XXX AREA tags do not have LO_Element structs of type LO_AREA, rather, they
** XXX have private lo_MapAreaRec structs containing LO_AnchorData pointers
*/
extern void
LM_SendMouseOut(MWContext *context, LO_Element *element);

extern void
LM_SendMouseOutOfAnchor(MWContext *context, LO_AnchorData *anchor);

/*
** Send a load or abort event for an image to a callback.
*/
typedef enum LM_ImageEvent {
    LM_IMGUNBLOCK   = 0,
    LM_IMGLOAD      = 1,
    LM_IMGABORT     = 2,
    LM_IMGERROR     = 3,
    LM_LASTEVENT    = 3
} LM_ImageEvent;

extern void
LM_SendImageEvent(MWContext *context, LO_ImageStruct *image_data,
		  LM_ImageEvent event);

NSPR_END_EXTERN_C

#endif /* libmocha_h___ */
