/*
** This file is Michael Toy's fault.  If you hate it or have troubles figuring
** it out, you should bother him about it.
**
** This file generates both the fields of the front end structure, and the
** fe specific prototypes for your front end implementations, AND the
** the code to fill in the structure in your context initilization code
**
** To use it:
**
**  #define MAKE_FE_FUNCS_STRUCT
**  #include "mk_cx_fn.h"
** This will generate the field definitions for the structure.
**
**  #define MAKE_FE_FUNCS_PREFIX(func) prefix_##func
**  #define MAKE_FE_FUNCS_ASSIGN cx->
**  #include "mk_cx_fn.h"
** Substitute your naming prefix for "prefix" (e.g XFE)
** This will generate the assignment statements to fill in the structure,
** the definition of MAKE_FE_FUNCS_ASSIGN will be the left hand side of the
** assignment statement.
**
**  #define MAKE_FE_FUNCS_PREFIX(func) prefix_##func
**  #define MAKE_FE_FUNCS_EXTERN
**  #include "mk_cx_fn.h"
** This will generate the prototypes for all your front end functions.
*/

#if defined(MAKE_FE_FUNCS_TYPES)
#define FE_DEFINE(func, returns, args) typedef returns (*MAKE_FE_TYPES_PREFIX(func)) args;
#elif defined(MAKE_FE_FUNCS_STRUCT)
#define FE_DEFINE(func, returns, args) returns (*func) args;
#elif defined(MAKE_FE_FUNCS_EXTERN)
#define FE_DEFINE(func, returns, args) extern returns MAKE_FE_FUNCS_PREFIX(func) args;
#elif defined(MAKE_FE_FUNCS_ASSIGN)
#define FE_DEFINE(func, returns, args) MAKE_FE_FUNCS_ASSIGN func = MAKE_FE_FUNCS_PREFIX(func);
#elif !defined(FE_DEFINE)
You;Should;Read;The;Header;For;This;File;Before;Including;Error;Error;Error;
#endif

FE_DEFINE(CreateNewDocWindow, MWContext*, (MWContext * calling_context,URL_Struct * URL))
FE_DEFINE(LayoutNewDocument, void, (MWContext *context, URL_Struct *url_struct, int32 *iWidth, int32 *iHeight, int32 *mWidth, int32 *mHeight))
FE_DEFINE(SetDocTitle,void, (MWContext * context, char * title))
FE_DEFINE(FinishedLayout,void, (MWContext *context))
FE_DEFINE(TranslateISOText,char *, (MWContext * context, int charset, char *ISO_Text))
FE_DEFINE(GetTextInfo,int, (MWContext * context, LO_TextStruct *text, LO_TextInfo *text_info))
FE_DEFINE(GetImageInfo,void, (MWContext * context, LO_ImageStruct *image_struct, NET_ReloadMethod force_reload))
FE_DEFINE(GetEmbedSize,void, (MWContext * context, LO_EmbedStruct *embed_struct, NET_ReloadMethod force_reload))
FE_DEFINE(GetJavaAppSize,void, (MWContext * context, LO_JavaAppStruct *java_struct, NET_ReloadMethod force_reload))
FE_DEFINE(GetFormElementInfo,void, (MWContext * context, LO_FormElementStruct * form_element))
FE_DEFINE(GetFormElementValue,void, (MWContext * context, LO_FormElementStruct * form_element, XP_Bool hide))
FE_DEFINE(ResetFormElement,void, (MWContext * context, LO_FormElementStruct * form_element))
FE_DEFINE(SetFormElementToggle,void, (MWContext * context, LO_FormElementStruct * form_element, XP_Bool toggle))
FE_DEFINE(FreeFormElement,void, (MWContext *context, LO_FormElementData *))
FE_DEFINE(FreeImageElement,void, (MWContext *context, LO_ImageStruct *))
FE_DEFINE(FreeEmbedElement,void, (MWContext *context, LO_EmbedStruct *))
FE_DEFINE(FreeJavaAppElement,void, (MWContext *context, LJAppletData *appletData))
FE_DEFINE(HideJavaAppElement,void, (MWContext *context, void*))
FE_DEFINE(FreeEdgeElement,void, (MWContext *context, LO_EdgeStruct *))
FE_DEFINE(FormTextIsSubmit,void, (MWContext * context, LO_FormElementStruct * form_element))
FE_DEFINE(DisplaySubtext,void, (MWContext * context, int iLocation, LO_TextStruct *text, int32 start_pos, int32 end_pos, XP_Bool need_bg))
FE_DEFINE(DisplayText,void, (MWContext * context, int iLocation, LO_TextStruct *text, XP_Bool need_bg))
FE_DEFINE(DisplayImage,void, (MWContext * context, int iLocation ,LO_ImageStruct *image_struct))
FE_DEFINE(DisplayEmbed,void, (MWContext * context, int iLocation ,LO_EmbedStruct *embed_struct))
FE_DEFINE(DisplayJavaApp,void, (MWContext * context, int iLocation ,LO_JavaAppStruct *java_struct))
FE_DEFINE(DisplaySubImage,void, (MWContext * context, int iLocation ,LO_ImageStruct *image_struct, int32 x, int32 y, uint32 width, uint32 height))
FE_DEFINE(DisplayEdge,void, (MWContext * context, int iLocation ,LO_EdgeStruct *edge_struct))
FE_DEFINE(DisplayTable,void, (MWContext * context, int iLocation ,LO_TableStruct *table_struct))
FE_DEFINE(DisplayCell,void, (MWContext * context, int iLocation ,LO_CellStruct *cell_struct))
FE_DEFINE(DisplaySubDoc,void, (MWContext * context, int iLocation ,LO_SubDocStruct *subdoc_struct))
FE_DEFINE(DisplayLineFeed,void, (MWContext * context, int iLocation , LO_LinefeedStruct *line_feed, XP_Bool need_bg))
FE_DEFINE(DisplayHR,void, (MWContext * context, int iLocation , LO_HorizRuleStruct *HR_struct))
FE_DEFINE(DisplayBullet,void, (MWContext *context, int iLocation, LO_BullettStruct *bullet))
FE_DEFINE(DisplayFormElement,void, (MWContext * context, int iLocation, LO_FormElementStruct * form_element))
FE_DEFINE(ClearView,void, (MWContext * context, int which))
FE_DEFINE(SetDocDimension,void, (MWContext *context, int iLocation, int32 iWidth, int32 iLength))
FE_DEFINE(SetDocPosition,void, (MWContext *context, int iLocation, int32 iX, int32 iY))
FE_DEFINE(GetDocPosition,void, (MWContext *context, int iLocation, int32 *iX, int32 *iY))
FE_DEFINE(BeginPreSection,void, (MWContext *context))
FE_DEFINE(EndPreSection,void, (MWContext *context))
FE_DEFINE(SetProgressBarPercent,void, (MWContext *context, int32 percent))
FE_DEFINE(SetBackgroundColor,void, (MWContext *context, uint8 red, uint8 green, uint8 blue))
FE_DEFINE(Progress, void, (MWContext * cx, const char *msg))
FE_DEFINE(Alert, void, (MWContext * cx, const char *msg))
FE_DEFINE(SetCallNetlibAllTheTime, void, (MWContext * win_id))
FE_DEFINE(ClearCallNetlibAllTheTime, void, (MWContext * win_id))
FE_DEFINE(GraphProgressInit, void, (MWContext *context, URL_Struct *URL_s, int32 content_length))
FE_DEFINE(GraphProgressDestroy, void, (MWContext *context, URL_Struct *URL_s, int32 content_length, int32 total_bytes_read))
FE_DEFINE(GraphProgress, void, (MWContext *context, URL_Struct *URL_s, int32 bytes_received, int32 bytes_since_last_time, int32 content_length))
FE_DEFINE(UseFancyFTP, XP_Bool, (MWContext * window_id))
FE_DEFINE(UseFancyNewsgroupListing, XP_Bool, (MWContext *window_id))
FE_DEFINE(FileSortMethod, int, (MWContext * window_id))
FE_DEFINE(ShowAllNewsArticles, XP_Bool, (MWContext *window_id))
FE_DEFINE(Confirm, XP_Bool,(MWContext * context, const char * Msg))
FE_DEFINE(Prompt,char*,(MWContext * context, const char * Msg, const char * dflt))
FE_DEFINE(PromptUsernameAndPassword, XP_Bool, (MWContext *,const char *,char **, char **))
FE_DEFINE(PromptPassword,char*,(MWContext * context, const char * Msg))
FE_DEFINE(EnableClicking,void,(MWContext*))
FE_DEFINE(ImageSize,int,(MWContext *, IL_ImageStatus , IL_Image *, void* ))
FE_DEFINE(ImageData,void,(MWContext *, IL_ImageStatus, IL_Image *, void*, int, int, XP_Bool update_image))
FE_DEFINE(ImageIcon,void,(MWContext *, int, void* ))
FE_DEFINE(ImageOnScreen,XP_Bool,(MWContext *, LO_ImageStruct *))
FE_DEFINE(SetColormap,int,(MWContext *, IL_IRGB *, int))
FE_DEFINE(AllConnectionsComplete,void,(MWContext * context))

#undef FE_DEFINE
#undef MAKE_FE_FUNCS_PREFIX
#undef MAKE_FE_FUNCS_ASSIGN
#undef MAKE_FE_FUNCS_EXTERN
#undef MAKE_FE_FUNCS_STRUCT
