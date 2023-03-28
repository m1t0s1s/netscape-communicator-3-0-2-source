#ifndef lm_h___
#define lm_h___
/*
** Mocha in the Navigator library-private interface.
*/
#include "xp.h"         /* for uint and PA_Block */
#include "libmocha.h"
#include "prlong.h"	/* for int64 time type used below */

/*
** Shared string constants for common property names.
*/
extern char lm_onLoad_str[];            /* "onLoad" */
extern char lm_onUnload_str[];          /* "onUnload" */
extern char lm_onAbort_str[];           /* "onAbort" */
extern char lm_onError_str[];           /* "onError" */
extern char lm_onScroll_str[];          /* "onScroll" */
extern char lm_onFocus_str[];           /* "onFocus" */
extern char lm_onBlur_str[];            /* "onBlur" */
extern char lm_onSelect_str[];          /* "onSelect" */
extern char lm_onChange_str[];          /* "onChange" */
extern char lm_onReset_str[];           /* "onReset" */
extern char lm_onSubmit_str[];          /* "onSubmit" */
extern char lm_onClick_str[];           /* "onClick" */
extern char lm_onMouseOver_str[];       /* "onMouseOver" */
extern char lm_onMouseOut_str[];        /* "onMouseOut" */

extern char lm_focus_str[];             /* "focus" */
extern char lm_blur_str[];              /* "blur" */
extern char lm_select_str[];            /* "select" */
extern char lm_click_str[];             /* "click" */
extern char lm_scroll_str[];            /* "scroll" */
extern char lm_enable_str[];            /* "enable" */
extern char lm_disable_str[];           /* "disable" */

extern char lm_toString_str[];          /* "toString" */
extern char lm_length_str[];            /* "length" */
extern char lm_forms_str[];             /* "forms" */
extern char lm_links_str[];             /* "links" */
extern char lm_anchors_str[];           /* "anchors" */
extern char lm_applets_str[];           /* "applets" */
extern char lm_embeds_str[];            /* "embeds" */
extern char lm_plugins_str[];           /* "plugins" */
extern char lm_images_str[];            /* "images" */
extern char lm_location_str[];          /* "location" */

extern char lm_opener_str[];            /* "opener" */
extern char lm_closed_str[];            /* "closed" */
extern char lm_assign_str[];            /* "assign" */
extern char lm_reload_str[];            /* "reload" */
extern char lm_replace_str[];           /* "replace" */

/*
** Timeout structure threaded on MochaDecoder.timeouts for cleanup.
*/
struct MochaTimeout {
    MochaDecoder        *self;          /* the initiating MochaDecoder */
    void                *toid;          /* timeout identifier */
    char                *expr;          /* the Mocha expression to evaluate */
    int64               when;           /* nominal time to run this timeout */
    uint16              taint;          /* taint from expr and timeout args */
    MochaTimeout        *next;          /* next timeout in list */
};

extern void             lm_ClearWindowTimeouts(MochaDecoder *decoder);

struct MochaNextURL {
    URL_Struct          *url_struct;
    MochaBoolean        replace;
    MochaNextURL        *next;
};

extern void             lm_ClearNextURLList(MochaDecoder *decoder);

/*
** Stack size per window context, plus one for the navigator.
*/
#define LM_STACK_SIZE   8192

extern MochaDecoder     *lm_global_decoder;
extern MochaDecoder     *lm_crippled_decoder;
extern MochaClass       lm_window_class;
extern MochaBoolean     lm_data_tainting;
extern uint16           lm_branch_taint;

/*
** A pointer to the current Mocha context, or null if Mocha is not running.
** We need this kludge because data-tainting checks from NET_GetURL and FE
** submit code (which calls LO_SendOnSubmit) are not fully parameterized.
*/
extern MochaContext     *lm_running_context;

extern MochaBoolean     lm_SaveParamString(MochaContext *mc, PA_Block *bp,
                                           const char *str);

extern MochaDecoder     *lm_NewWindow(MWContext *context);
extern void             lm_DestroyWindow(MochaDecoder *decoder);
extern MochaBoolean     lm_InitWindowContent(MochaDecoder *decoder);
extern void             lm_FreeWindowContent(MochaDecoder *decoder);
extern MochaBoolean     lm_SetInputStream(MochaContext *mc,
					  MochaDecoder *decoder,
					  NET_StreamClass *stream,
					  URL_Struct *url_struct);
extern MochaObject      *lm_DefineDocument(MochaDecoder *decoder);
extern MochaObject      *lm_DefineHistory(MochaDecoder *decoder);
extern MochaObject      *lm_DefineLocation(MochaDecoder *decoder);
extern MochaObject      *lm_DefineNavigator(MochaDecoder *decoder);
extern MochaObject      *lm_NewPluginList(MochaContext *mc);
extern MochaObject      *lm_NewMIMETypeList(MochaContext *mc);

/*
** Get (create if needed) decoder's form, link, and named anchor arrays.
*/
extern MochaObject      *lm_GetFormArray(MochaDecoder *decoder);
extern MochaObject      *lm_GetLinkArray(MochaDecoder *decoder);
extern MochaObject      *lm_GetNameArray(MochaDecoder *decoder);
extern MochaObject      *lm_GetAppletArray(MochaDecoder *decoder);
extern MochaObject      *lm_GetEmbedArray(MochaDecoder *decoder);
extern MochaObject      *lm_GetImageArray(MochaDecoder *decoder);

/*
** dummy object for applets and embeds that can't be reflected
*/
extern MochaObject *lm_DummyObject;
extern void lm_InitDummyObject(MochaContext *mc);

/*
** Base class of all Mocha input private object data structures.
*/
typedef struct MochaInputBase {
    MochaDecoder        *decoder;       /* this window's Mocha decoder */
    int32               type;           /* layout form element type */
} MochaInputBase;

#define FORM_TYPE_ARRAY 0x100

/*
** Base class of input event-handling elements like anchors and form inputs.
*/
typedef struct MochaInputHandler {
    MochaInputBase      base;           /* decoder and type */
    MochaObject         *object;        /* this input handler's Mocha object */
    uint32              event_mask;     /* mask of events in progress */
} MochaInputHandler;

#define base_decoder    base.decoder
#define base_type       base.type

/* Event bits stored in the low end of decoder->event_mask. */
#define EVENT_FOCUS     0x00000001      /* input focus event in progress */
#define EVENT_BLUR      0x00000002      /* loss of focus event in progress */
#define EVENT_SELECT    0x00000004      /* input field selection in progress */
#define EVENT_CHANGE    0x00000008      /* field value change in progress */
#define EVENT_RESET     0x00000010      /* form submit in progress */
#define EVENT_SUBMIT    0x00000020      /* form submit in progress */
#define EVENT_CLICK     0x00000040      /* input element click in progress */
#define EVENT_SCROLL    0x00000080      /* window is being scrolled */
#define EVENT_MOUSEOVER 0x00000100      /* user is mousing over a link */
#define EVENT_MOUSEOUT  0x00000200      /* user is mousing out of a link */
#define EVENT_LOAD      0x00000400      /* layout parsed a loaded document */

/*
** Mocha URL object.
**
** Location is a special URL: when you set one of its properties, your client
** window goes to the newly formed address.
*/
typedef struct MochaURL {
    MochaInputHandler   handler;
    uint32              index;
    uint16              taint;
    uint16              spare;
    MochaAtom           *href;
    MochaAtom           *target;
} MochaURL;

#define URL_NOT_INDEXED ((uint32)-1)

#define url_decoder     handler.base_decoder
#define url_type        handler.base_type
#define url_object      handler.object
#define url_event_mask  handler.event_mask

extern MochaURL *
lm_NewURL(MochaContext *mc, MochaDecoder *decoder, char *href, char *target);

extern void
lm_ReplaceURL(MWContext *context, URL_Struct *url_struct);

extern MochaBoolean
lm_GetURL(MochaContext *mc, MochaDecoder *decoder, URL_Struct *url_struct,
          uint16 taint);

extern const char *
lm_CheckURL(MochaContext *mc, const char *url_string);

extern MochaBoolean
lm_CheckWindowName(MochaContext *mc, const char *window_name);

extern MochaBoolean
lm_InitTaint(MochaContext *mc);

extern MochaBoolean
lm_GetTaint(MochaContext *mc, MochaDecoder *decoder, uint16 *taintp);

extern MochaBoolean
lm_SetTaint(MochaContext *mc, MochaDecoder *decoder, const char *url_string);

extern MochaBoolean
lm_MergeTaint(MochaContext *mc, MochaContext *mc2);

extern void
lm_ClearTaint(MochaContext *mc);

extern MochaBoolean
lm_CheckTaint(MochaContext *mc, uint16 taint, const char *url_desc,
	      const char *url_string);

extern const char *
lm_GetOrigin(MochaContext *mc, const char *url_string);

extern const char *
lm_GetOriginURL(MochaContext *mc, MochaDecoder *decoder);

extern const char *
lm_SetOriginURL(MochaContext *mc, MochaDecoder *decoder,
		const char *url_string);

extern MochaBoolean
lm_CheckOrigins(MochaContext *mc, MochaDecoder *decoder);

extern MochaBoolean
lm_BranchCallback(MochaContext *mc, MochaScript *script);

extern void
lm_ErrorReporter(MochaContext *mc, const char *message,
                 MochaErrorReport *report);

extern MochaObject *
lm_GetFormObjectByID(MochaDecoder *decoder, uint form_id);

extern LO_FormElementStruct *
lm_GetFormElementByIndex(MochaObject *form_obj, int32 index);

extern MochaBoolean
lm_AddFormElement(MochaContext *mc, MochaObject *form, MochaObject *obj,
                  char *name, uint index);

extern MochaBoolean
lm_ReflectRadioButtonArray(MWContext *context, intn form_id, const char *name);

extern uint16
lm_GetFormElementTaint(MochaContext *mc, LO_FormElementStruct *form_element);

extern void
lm_ResetFormElementTaint(MochaContext *mc, LO_FormElementStruct *form_element);

extern MochaBoolean
lm_SendEvent(MWContext *context, MochaObject *obj, char *name,
             MochaDatum *result);

/*
** Class initializers (the wave of the future).
*/
extern MochaBoolean
lm_InitImageClass(MochaDecoder *decoder);

extern MochaBoolean
lm_InitInputClasses(MochaDecoder *decoder);

/* XXX save and restore anonymous images across frame resize-reload */
extern void
lm_SaveAnonymousImages(MWContext *context, MochaDecoder *decoder);

extern MochaBoolean
lm_RestoreAnonymousImages(MWContext *context, MochaDecoder *decoder);

/*
** Debugging hooks.
*/
#ifdef DEBUG_brendan
#define LM_LINK_OBJECT(decoder, obj, name)  lm_LinkObject(decoder, obj, name)
#define LM_UNLINK_OBJECT(decoder, obj)      lm_UnlinkObject(decoder, obj)

extern void
lm_LinkObject(MochaDecoder *decoder, MochaObject *obj, const char *name);

extern void
lm_UnlinkObject(MochaDecoder *decoder, MochaObject *obj);

extern void
lm_DumpLiveObjects(MochaDecoder *decoder);
#else
#define LM_LINK_OBJECT(decoder, obj, name)
#define LM_UNLINK_OBJECT(decoder, obj)
#endif

#endif /* lm_h___ */
