/* -*- Mode: C; tab-width: 8 -*-
   xfe.h --- X-specific headers for the front end.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Jun-94.
 */
#ifndef _XFE_H_
#define _XFE_H_

#include <sys/errno.h>

#undef TRUE /* OSF1 version of X conflicts... */

#include <X11/Intrinsic.h>
/*
 *    X11/Xlib.h "define"s Bool to be int. This is really lame (that's
 *    what typedef is for losers). So.. Undef it here, so that the XP type
 *    gets used...djw
 */
#ifdef Bool
#undef Bool
#endif

#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/PanedW.h>
/*#include <Xm/Label.h>*/
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/List.h>
#include <Xm/FileSB.h>
#include <Xm/Frame.h>
#include <Xm/Protocols.h>


#ifndef TRUE /* OSF1 version of X conflicts... */
#define TRUE 1
#endif

#define NO_HELP

#define GRIDS

/* #define HAVE_SECURITY */ /* jwz -- moved this to myconfig.mk */
/* and again*/
#ifdef Bool
#undef Bool
#endif

#include "prefs.h"
#include "icondata.h"

#define BookmarkStruct HotlistStruct
#define HOT_GetBookmarkList HOT_GetHotlistList
#define HOT_SaveBookmark HOT_SaveHotlist
#define HOT_SearchBookmark HOT_SearchHotlist
#define HOT_ReadBookmarkFromDisk HOT_ReadHotlistFromDisk
#define HOT_FreeBookmarks HOT_FreeHotlist

#define PERFECT_SCROLL

extern Atom WM_SAVE_YOURSELF;

typedef struct fe_colormap fe_colormap;

/* if not found in binpath, _long will contain full-path exec name */ 
/* if FOUND in binpath, fe_progname_long == fe_progname */
extern const char *fe_progname_long; 
extern const char *fe_progname;
extern const char *fe_progclass;
extern const char fe_BuildDate[];
extern MWContext *someGlobalContext;
extern FILE *real_stderr, *real_stdout;
extern XtAppContext fe_XtAppContext;
extern Display *fe_display;
extern int fe_WindowCount;
extern XtResource fe_Resources[], fe_GlobalResources[];
extern Cardinal fe_ResourcesSize, fe_GlobalResourcesSize;
extern char *sploosh;
extern Boolean fe_ImagesCantWork;

extern const char fe_BuildConfiguration[];
extern const char fe_version[];
extern const char fe_long_version[];
extern char *fe_version_and_locale;
extern int fe_SecurityVersion;
extern int fe_HaveDNS;
extern int fe_VendorAnim;
extern char *fe_pidlock;

extern void fe_wm_save_self_cb(Widget w, XtPointer clientData, XtPointer callData);
extern Boolean fe_add_session_manager(MWContext *context);
extern char *xfe_get_netscape_home_page_url (void);

extern void fe_InitCommandActions (void);
extern void fe_InitMouseActions (void);
extern void fe_HackTranslations (MWContext *, Widget);
extern void fe_HackTextTranslations(Widget);
extern void fe_HackDialogTranslations(Widget);
extern void fe_EventLoop (void);
extern int fe_GetURL (MWContext *context, URL_Struct *url_struct,
			Boolean skip_get_url);
extern int fe_GetSecondaryURL (MWContext *context, URL_Struct *url_struct,
		       int output_format, void *call_data,
			       Boolean skip_get_url);
extern char *fe_GetURLAsText (MWContext *context, URL_Struct *url,
			      const char *prefix,
			      unsigned long *data_size_ret);
extern char *fe_GetURLAsPostscript (MWContext *context, URL_Struct *url,
				    unsigned long *data_size_ret);
extern char *fe_GetURLAsSource (MWContext *context, URL_Struct *url,
				unsigned long *data_size_ret);
extern void fe_SaveURL (MWContext *context, URL_Struct *url);
extern void fe_InitializeGlobalResources (Widget toplevel);
extern MWContext *fe_MakeWindow (Widget toplevel, MWContext *context_to_copy,
				 URL_Struct *url, char *window_name,
				 MWContextType type, Boolean skip_get_url);
extern MWContext * fe_MakeNewWindow(Widget toplevel, MWContext *context_to_copy,
                        URL_Struct *url, char *window_name, MWContextType type,
                        Boolean skip_get_url, Chrome *decor);
void fe_MakeChromeWidgets (Widget shell, MWContext *context);
extern void fe_MakeSaveToDiskContextWidgets (Widget toplevel,
						MWContext *context);
extern void fe_MakeWidgets (Widget toplevel, MWContext *context);
extern Widget fe_MakeMenubar (Widget parent, MWContext *context);
extern Widget fe_MakeToolbar (Widget parent, MWContext *context,
			      Boolean urls_p);
extern void fe_FixLogoMargins (MWContext *context);
extern void fe_RebuildWindow (MWContext *context);
extern void fe_BookmarkInit (MWContext *context);
extern void fe_GenerateBookmarkMenu (MWContext *context);
extern void fe_GenerateWindowsMenu (MWContext *context);
extern void fe_InvalidateAllBookmarkMenus (void);
extern Boolean fe_ImportBookmarks (char *filename);
extern Boolean fe_LoadBookmarks (char *filename);
extern Boolean fe_SaveBookmarks (void);
extern void fe_AddToBookmark (MWContext *, const char *title, URL_Struct *,
			      time_t time);
extern void fe_BookmarkAccessTimeChanged (MWContext *context,
					  BookmarkStruct *h);
extern Widget fe_MakeScrolledWindow (MWContext *, Widget, const char *);
extern void fe_SensitizeMenus (MWContext *context);
extern void fe_MsgSensitizeChildren(Widget, XtPointer, XtPointer);
extern void fe_DestroyContext (MWContext *context);
extern void fe_DestroySaveToDiskContext (MWContext *context);
extern void fe_DestroyLayoutData (MWContext *context);
extern void fe_LoadDelayedImages (MWContext *context);
extern void fe_LoadDelayedImage (MWContext *context, const char *url);
extern void fe_ReLayout (MWContext *context, NET_ReloadMethod force_reload);
extern void fe_AbortCallback (Widget, XtPointer, XtPointer);
extern void fe_QuitCallback (Widget, XtPointer, XtPointer);
extern void fe_SaveAsCallback (Widget, XtPointer, XtPointer);
extern void fe_OpenURLCallback (Widget, XtPointer, XtPointer);
extern void fe_NetscapeCallback (Widget, XtPointer, XtPointer);
#ifdef __sgi
extern void fe_SGICallback (Widget, XtPointer, XtPointer);
#endif /* __sgi */
extern void fe_RefreshAllAnchors (void);
extern void fe_ScrollTo (MWContext *context, unsigned long x, unsigned long y);
extern void fe_SetDocPosition (MWContext *, unsigned long x, unsigned long y);
extern void fe_SetCursor (MWContext *context, Boolean over_link_p);
extern void fe_EventLOCoords (MWContext *context, XEvent *event,
			      unsigned long *x, unsigned long *y);
extern void fe_SyncExposures(MWContext* context);
extern void fe_RefreshArea(MWContext*, int32 x, int32 y, uint32 w, uint32 h);

extern void fe_ScrollForms (MWContext *context, int x_off, int y_off);
extern void fe_GravityCorrectForms (MWContext *context, int x_off, int y_off);
extern void fe_SetFormsGravity (MWContext *context, int gravity);
extern void fe_NukeBackingStore (Widget widget);

extern void fe_StartProgressGraph (MWContext *context);
extern void fe_StopProgressGraph (MWContext *context);
extern void fe_SetURLString (MWContext *context, URL_Struct *url);
extern void fe_perror (MWContext *context, const char *message);
extern void fe_stderr (MWContext *context, const char *message);
extern void fe_Message (MWContext *context, const char *message);
extern char *fe_Basename (char *string);
extern XP_Bool fe_StrEndsWith(char *string, char *endString);
extern void fe_MidTruncatedProgress (MWContext *context, const char *message);
extern Boolean fe_ContextHasPopups(MWContext* context);
extern void *fe_dialog (Widget parent, const char *title,
			const char *message, XP_Bool question_p,
			const char *text, XP_Bool wait_p, XP_Bool select_p,
			char **passwd);
extern Boolean fe_Confirm_2 (Widget parent, const char *message);
extern void fe_Alert_2 (Widget parent, const char *message);
extern void fe_perror_2 (Widget parent, const char *message);
extern void fe_UpdateGraph (MWContext *context, Boolean update_text);
extern void fe_UpdateLED (MWContext *context, Boolean toggle_p);
extern void fe_UpdateSecurityBar (MWContext *context);
extern GC fe_GetGC (Widget, unsigned long flags, XGCValues *gcv);
extern GC fe_GetGCfromDW(Display*, Window, unsigned long, XGCValues*);
extern void fe_FlushGCCache (Widget widget, unsigned long flags);
extern void fe_GetMargin(MWContext*, int32 *marginw_ptr, int32 *marginh_ptr);

extern Pixel fe_GetPermanentPixel (MWContext *context, int r, int g, int b);
extern Pixel fe_GetPixel (MWContext *context, int r, int g, int b);
extern Pixel fe_GetImagePixel (MWContext *context, int r, int g, int b);
extern void fe_QueryColor (MWContext *context, XColor *color);
extern Colormap fe_MakeNewColormap (MWContext *new_context,
				    MWContext *context_to_copy);
extern void fe_DisposeColormap(MWContext *context);
extern fe_colormap *fe_NewColormap(Screen *screen, Visual *visual,
                                    Colormap cmap, Boolean private_p);
extern Colormap fe_cmap(MWContext *context);
extern void fe_DefaultColormapAndVisual(Colormap *colormap, Visual *visual);
extern void fe_InitColormap (MWContext *context);
extern void fe_FreeTransientColors(MWContext *context);
extern Status fe_AllocColor(fe_colormap *colormap, XColor *color_in_out);
extern void fe_AllocClosestColor (fe_colormap *colormap,
                                  XColor *color_in_out);
extern void fe_SetColormapTransparencyColor(MWContext *context, int bg);
extern Pixel *fe_ColormapMapping(MWContext *context);

extern void fe_InitIconColors (MWContext *context);
extern void fe_InitIcons (MWContext *context);
extern void fe_IconSize (int icon_number, long *width, long *height);
extern Pixmap fe_LogoPixmap (MWContext *context, Dimension *w, Dimension *h,
			     Boolean large_p);
extern Pixmap fe_ToolbarPixmap (MWContext *context, int i, Boolean disabled_p,
				Boolean urls_p);
#ifdef HAVE_SECURITY
extern Pixmap fe_SecurityPixmap (MWContext *context,
				 Dimension *w, Dimension *h,
				 int type);
#endif
extern void fe_DrawLogo (MWContext *context, Boolean tick_p, Boolean large_p);
extern void fe_ResetLogo (MWContext *context, Boolean large_p);
extern Boolean plonk (MWContext *context);
extern void fe_RegisterConverters (void);
extern void fe_RegisterPluginConverters (void);
extern void fe_DrawShadows (MWContext *cx, Drawable draw, int x, int y,
			    int width, int height,
			    int shadow_width, int shadow_style);
extern void fe_AddHistory (MWContext *context, URL_Struct *url,
			   const char *doc_title);
extern void fe_HistoryDialog (MWContext *context);
extern void fe_RegenerateHistoryMenu (MWContext *context);
extern void fe_HistoryItemAction (Widget, XEvent *, String *, Cardinal *);
extern void fe_FindDialog (MWContext *context, Boolean really_do_it_p);
extern void fe_FindReset (MWContext *context);
extern void fe_PrintDialog (MWContext *context);
extern void fe_Print (MWContext *context, URL_Struct *url,
			Boolean toFile, char *filename);
extern void fe_DocInfoDialog (MWContext *context);
extern void fe_UpdateDocInfoDialog (MWContext *context);
extern void fe_AddBookmark (MWContext *context,
			   const char *url, const char *title);
extern void fe_AddBookmarkCallback (Widget, XtPointer, XtPointer);
extern void fe_ViewBookmarkCallback (Widget, XtPointer, XtPointer);
extern void fe_GotoBookmarkCallback (Widget, XtPointer, XtPointer);
extern void fe_PropertyTextDialog (MWContext *context);
extern void fe_PropertyImageDialog (MWContext *context);
extern void fe_GeneralPrefsDialog (MWContext *context);
extern void fe_MailNewsPrefsDialog (MWContext *context);
extern void fe_NetworkPrefsDialog (MWContext *context);
extern void fe_SecurityPrefsDialog (MWContext *context);
extern void fe_InstallPreferences (MWContext *context);
extern Widget fe_ViewSourceDialog (MWContext *context,
				   const char *title, const char *url);
extern void fe_LicenseDialog (MWContext *context);
extern void fe_url_exit (URL_Struct *url, int status, MWContext *context);
extern void fe_RaiseSynchronousURLDialog (MWContext *context,
					  Widget parent,
					  const char *title);
extern void fe_LowerSynchronousURLDialog (MWContext *context);
extern void fe_DefaultUserInfo (char **uid, char **name,
				Boolean really_default_p);
extern void fe_VerifyDiskCache (MWContext *context);
extern Boolean fe_CheckUnsentMail (void);
extern Boolean fe_CheckDeferredMail (void);
extern void fe_SetString(Widget widget, const char* propname, char* str);
extern int  fe_MoveMail(MWContext *context, char *from, char *to);

extern Widget fe_CreateTextField(Widget parent, const char *name,
				 Arg *av, int ac);
extern Widget fe_CreateText(Widget parent, const char *name, Arg *av, int ac);
extern Widget fe_CreateOptionMenu(Widget parent, char* name, Arg* p_argv, Cardinal p_argc);

extern Boolean fe_contextIsValid( MWContext *context );
extern void fe_SetGridFocus (MWContext *context);
extern MWContext *fe_GetFocusGridOfContext (MWContext *context);
extern MWContext *fe_GridToTopContext (MWContext *context);
extern Boolean fe_IsGridParent (MWContext *context);
extern void fe_forms_clean_text(MWContext *context, int charset,
				char *text, Boolean newlines_too_p);
extern Boolean fe_HandleHREF (MWContext *context, LO_Element *xref,
			   Boolean save_p, Boolean other_p,
			   XEvent *event, String *av, Cardinal *ac);
extern void fe_getVisualOfContext(MWContext *context, Visual **v_ptr,
				Colormap *cmap_ptr, Cardinal *depth_ptr);

/* File status routines */
extern XP_Bool fe_isFileChanged(char *filename, time_t mtime, time_t *new_mtime);
extern Boolean fe_isFileExist(char *filename);
extern Boolean fe_isFileReadable(char *filename);
extern Boolean fe_isDir(char *dirname);

extern Boolean fe_MovemailWarning(MWContext* context);

#ifdef DEBUG_jwz
Bool fe_any_anims_p (void);
#endif /* DEBUG_jwz */

#include "xp_str.h"


struct fe_file_type_data
{
  Widget options[10];
  int selected_option;
#ifdef NEW_DECODERS
  Boolean conversion_allowed_p;
  Widget fileb;
  const char *orig_url;
#endif /* NEW_DECODERS */
};

int16	XFE_GetDefaultCSID(void);

extern void fe_Exit (int status);
extern void fe_MinimalNoUICleanup(void);
extern char *fe_ReadFileName_2 (MWContext *context,
			      Widget parent,
			      Widget *filebp,
			      struct fe_file_type_data **ftdp,
			      const char *title,
			      const char *default_url,
			      Boolean must_exist,
			      int *save_as_type);
extern char *fe_ReadFileName (MWContext *context,
			      const char *title,
			      const char *default_url,
			      Boolean must_exist,
			      int *save_as_type);
extern Widget fe_CreateFileSelectionBox(Widget, char*, Arg*, Cardinal);
extern MWContext *fe_WidgetToMWContext (Widget widget);
extern MWContext *fe_MotionWidgetToMWContext (Widget widget);
extern void fe_UserActivity (MWContext *context);
extern void fe_DrawIcon (MWContext *context, LO_ImageStruct *lo_image,
			 int icon_number);
extern void fe_MakeLEDGCs (MWContext *context);

extern void fe_config_eh (Widget, XtPointer, XEvent *);
extern void fe_InitScrolling (MWContext *context);
extern void fe_DisableScrolling (MWContext *context);
extern void fe_SetGuffaw (MWContext *context, Boolean on);
extern void fe_ClearArea (MWContext *context,
			  int x, int y, unsigned int w, unsigned int h);
extern void fe_ClearAreaWithExposures(MWContext *context,
	      int x, int y, unsigned int w, unsigned int h, Boolean exposures);
extern int fe_WindowGravityWorks (Widget, Widget);
extern void fe_FormatDocTitle (const char *title, const char *url,
			       char *output, int size);
extern void fe_NeutralizeFocus (MWContext *context);

/* context layout functions
 */
#define MAKE_FE_FUNCS_PREFIX(func) XFE##_##func
#define MAKE_FE_FUNCS_EXTERN
#include "mk_cx_fn.h"

extern ContextFuncs * fe_BuildDisplayFunctionTable(void);

extern Visual *fe_ParseVisual (Screen *screen, const char *v);
extern char *fe_VisualDescription (Screen *screen, Visual *visual);
extern int fe_ScreenNumber (Screen *screen);
extern Visual *fe_ReadVisual (MWContext *context);
extern void fe_ChangeVisualCallback (Widget, XtPointer, XtPointer);
extern int fe_VisualDepth (Display *dpy, Visual *visual);
extern int fe_VisualPixmapDepth (Display *dpy, Visual *visual);
extern int fe_VisualCells (Display *dpy, Visual *visual);

extern void fe_SetupPasswdText (Widget text_field, int max_length);
extern char *fe_GetPasswdText (Widget text_field);

extern char *fe_StringTrim (char *string);
extern void fe_MidTruncateString (const char *input, char *output,
				  int max_length);

extern void fe_OwnSelection (MWContext *context, Time time, Boolean clip_p);
extern void fe_DisownSelection (MWContext *context, Time time, Boolean clip_p);

extern void fe_InitRemoteServerWindow (MWContext *context);
extern void fe_InitRemoteServer (Display *dpy);
extern int fe_RemoteCommands (Display *dpy, Window window, char **commands);

extern XtActionsRec fe_CommandActions [], fe_MailNewsActions [];
extern int fe_CommandActionsSize, fe_MailNewsActionsSize;
extern void fe_UnmanageChild_safe (Widget w);
extern void fe_DestroyWidgetTree(Widget *kids, int nkids);

/*
 * Context protection/destruction releated routines
 */
void fe_ProtectContext(MWContext *context);
void fe_UnProtectContext(MWContext *context);
Boolean fe_IsContextProtected(MWContext *context);
Boolean fe_IsContextDestroyed(MWContext *context);

/*
 *    Tool tip support.
 */
typedef Boolean (*fe_ToolTipGadgetCheckProc)(Widget widget);
void    fe_ManagerAddGadgetToolTips(Widget manager, fe_ToolTipGadgetCheckProc);
void    fe_WidgetAddToolTips(Widget widget);
Widget  fe_CreateToolTipsDemoToggle(Widget, char*, Arg* args, Cardinal n);
Boolean fe_ManagerCheckGadgetToolTips(Widget, fe_ToolTipGadgetCheckProc);
void fe_WidgetAddDocumentString(MWContext *context, Widget widget);

/*
 *    Widget tree walking routine.
 */
typedef XtPointer (*fe_WidgetTreeWalkMappee)(Widget widget, XtPointer data);
XtPointer fe_WidgetTreeWalk(Widget widget, fe_WidgetTreeWalkMappee callback,
		  XtPointer data);
XtPointer fe_WidgetTreeWalkChildren(Widget widget,
				    fe_WidgetTreeWalkMappee callback,
				    XtPointer data);
Widget    fe_FindWidget(Widget top, char* name); /* find widget by name */
Boolean   fe_ManagerHasGadgets(Widget manager);
Boolean   fe_IsOptionMenu(Widget widget);
char*     fe_ResourceString(Widget widget, char* res_name, char* res_class);
char*     XtClassName(Widget w);

#define countof(x) (sizeof(x) / sizeof (*x))

/*
 * Drag'n'drop for Mail compose
 */
#include "dragdrop.h"
extern void
fe_mc_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		  fe_dnd_Source* source, XEvent* event);

/*
 * Mail compose: Attachments
 */
# define XFE_MAX_ATTACHMENTS	128
struct fe_mail_attach_data
{
  MWContext* context;
  Widget shell;
  Widget list;
  Widget attach_file, delete;
  Widget text_p, source_p, postscript_p;

  Widget file_shell;

  Widget location_shell;
  Widget location_text;

  int nattachments;
  struct MSG_AttachmentData attachments[XFE_MAX_ATTACHMENTS];
};

/*
 * Mail/News/Browser find
 */

typedef struct
{
  MWContext *context;
  MWContext *context_to_find;	/* used for which frame cell to find in. */
  Boolean find_in_headers;
  Boolean case_sensitive_p, backward_p;
  char *string;
  Widget shell, text, case_sensitive, backward, msg_head, msg_body;
  LO_Element *start_element, *end_element;
  int32 start_pos, end_pos;
} fe_FindData;


/*
 * Context data has subparts. Parts that only specific contexts will need.
 * fe_ContextData
 *	...all data common to most contexts...
 *	fe_NewsContextData *news;
 *	fe_MailContextData *news;
 *	fe_ComposeContextData *news;
 */

/* News context specific context data */
typedef struct fe_NewsContextData {
  /* For open news host dialog */
  Widget openNewsHost_shell;
  Widget openNewsHost_host;
  Widget openNewsHost_port;
  Widget openNewsHost_secure;
  ReadFileNameCallbackFunction openNewsHost_fn;
  void *openNewsHost_fnClosure;
} fe_NewsContextData;

typedef struct fe_caret_data
{
  int      x;
  int      y;
  unsigned width;
  unsigned height;
  int      time;
  XtIntervalId timer_id;
  Bool     showing;
  Bool     running;
  Pixmap   backing_store;
} fe_EditorCaretData;

typedef struct fe_EditorAscrollData
{
    XtIntervalId timer_id;
    int x;
    int y;
    int delta_x;
    int delta_y;
} fe_EditorAscrollData;

typedef struct fe_EditorContextData {
    fe_EditorCaretData caret_data; /* caret stuff */
    fe_EditorAscrollData ascroll_data;

    /*
     *    Toolbar widgets.
     */
    Widget toolbar_browse;
    Widget toolbar_publish;

    Widget toolbar_cut;
    Widget toolbar_copy;
    Widget toolbar_paste;

    Widget toolbar_smaller;
    Widget toolbar_bigger;
    Widget toolbar_size;

    Widget toolbar_bold;
    Widget toolbar_italic;
    Widget toolbar_fixed;
    Widget toolbar_color;
    Widget toolbar_link;
    Widget toolbar_plain;

    Widget toolbar_target;
    Widget toolbar_image;
    Widget toolbar_hrule;
    Widget toolbar_props;

    Widget toolbar_style;
    Widget toolbar_list;
    Widget toolbar_numbers;
    
    Widget toolbar_outdent;
    Widget toolbar_indent;

    Widget toolbar_left;
    Widget toolbar_center;
    Widget toolbar_right;

} fe_EditorContextData;

/*
 *   Simple dependency mechanism package...djw.
 */
typedef unsigned fe_Dependency;

#define FE_MAKE_DEPENDENCY(x) ((fe_Dependency)(x))

typedef struct fe_DependentList {
	struct fe_DependentList* next;

	Widget        widget;   /* that's me */
	XtCallbackRec callback;
	fe_Dependency mask;     /* call me back if match this mask */
} fe_DependentList;

typedef struct fe_ContextData
{
  Widget widget;		/* The main shell widget for this window. */

  Widget url_label;		/* Label indicating what mode the url_text */
				/* is (either "Location:" or "Go To") */
  Widget url_text;		/* Text field displaying the current URL (or */
				/* a URL that the user is typing in) */
  Widget back_button;		/* Toolbar button to go back in history. */
  Widget forward_button;	/* Toolbar button to go forward in history. */
  Widget home_button;		/* Toolbar button to load home page. */

  Widget back_menuitem;		/* Menuitem to go back in history. */
  Widget forward_menuitem;	/* Menuitem to go foreward in history. */
  Widget home_menuitem;		/* Menuitem to load home page. */
  Widget delete_menuitem;	/* Menuitem to delete this window. */

  Widget back_popupitem;	/* Popup menuitem to go back in history. */
  Widget forward_popupitem;	/* Popup menuitem to go foreward in history. */

  Widget cut_menuitem, copy_menuitem, paste_menuitem, paste_quoted_menuitem;
				/* Menuitems to cut/copy/paste. */

  Widget findAgain_menuitem;	/* Menuitems to findAgain. */
  Widget reloadFrame_menuitem;	/* Menuitems to reload selected Frame. */
  Widget frameSource_menuitem;	/* Menuitem to view source of selected Frame. */
  Widget frameInfo_menuitem;	/* Menuitem to view info of selected Frame. */

  Widget mailto_menuitem;	/* Menuitem to mail this URL to someone. */
  Widget saveAs_menuitem;	/* Menuitem to save this URL as */
  Widget uploadFile_menuitem;   /* Menuitem to upload a file to an FTP site */
  Widget print_menuitem;	/* Menuitem to print this URL. */
  Widget refresh_menuitem;	/* Menuitem to refresh this URL. */
  Widget print_button;		/* Toolbar button to print this URL. */
  Widget logo;			/* The animating logo. */

  Widget bookmark_menu;		/* Menu containing the bookmark entries. */
  Widget windows_menu;		/* Menu containing the list of windows. */
  Widget history_menu;		/* Menu containing the URL history. */
  Widget delayed_menuitem;	/* Menuitem to load in delayed images. */
  Widget delayed_button;	/* Toolbar button to load in delayed images. */
  Widget abort_menuitem;	/* Menuitem to abort downloads. */
  Widget abort_button;		/* Toolbar button to abort downloads. */

#ifdef DEBUG_jwz
  Widget toggleJS_menuitem;	/* Menuitem to toggle javascript. */
  Widget toggleAnim_menuitem;	/* Menuitem to toggle GIF animations. */
  Widget toggleLoops_menuitem;	/* Menuitem to toggle GIF animation loops. */
  Widget toggleSizes_menuitem;	/* Menuitem to toggle font size changes. */
#endif /* DEBUG_jwz */


  Widget menubar;		/* Menubar, containing all menu buttons. */
  Widget top_area;		/* Form containing the toolbar, current URL */
				/* info, directory buttons, and logo. */
  Widget toolbar;		/* RowColumn containing the toolbar buttons. */
  Widget character_toolbar;	/* RowColumn containing editor char buttons. */
  Widget paragraph_toolbar;	/* RowColumn containing editor para buttons. */
  Widget dashboard;		/* Form containing the security/thermometer */
				/* info at bottom of window.*/
#ifdef LEDGES
  Widget top_ledge, bottom_ledge; /* Half-implemented fixed areas that */
				  /* display other URLs. */
#endif
#ifdef HAVE_SECURITY
  Widget security_bar;		/* Drawing area for colored security bar. */
  Widget security_logo;		/* Label gadget containing security "key" */
				/* icon.*/
#endif
#ifdef JAVA
  Widget show_java;
#endif
  Widget main_pane;		/* PanedWindow that contains the main
				   drawing area, as well as any scrolling
				   lists for mail or news. */
  Widget drawing_area;		/* The main drawing area */
  Widget scrolled;		/* scroller (see scroller.h) containing */
				/* the main drawing area and its scrollbars. */
  Widget hscroll, vscroll;	/* Scrollbars for the main drawining area. */
  Widget wholine;		/* Label at bottom of window displaying */
				/* info about current pointing URL, etc. */
  Widget thermo;		/* Drawing area for thermometer stuff. */
  Widget LED;			/* Drawing area for loading LED (used only */
				/* if no animating logo present.)*/
  Widget bifficon;		/* Button indicating whether we have new
				   mail. */
  GC thermo_gc;			/* GC used to help draw thermometer. */
  GC LED_gc;			/* GC used to help draw loading LED. */

  Widget folderform;		/* Form that holds the folderlist. */
  Widget messageform;		/* Form that holds the messagelist. */
  Widget folderlist;		/* List of mail/news folders. */
  Widget messagelist;		/* List of mail/news messages. */

  int hysteresis;		/* "stickiness" of mouse-selection. */

  /* Widgets for the mail folder menu */
  Widget foldermenu;
  Widget move_selected_to_folder;
  Widget copy_selected_to_folder;
  Boolean doingMove;			/* move and copy share the same
					   foldermenu */
  /* Widgets for mail popup menus */
  Widget mailPopupBody;
  Widget mailPopupMessage;
  Widget mailPopupFolder;

  /* Widgets and things that appear only in MessageComposition windows: */
  Widget mcFrom;
  Widget mcReplyTo;
  Widget mcTo;
  Widget mcCc;
  Widget mcBcc;
  Widget mcFcc;
  Widget mcNewsgroups;
  Widget mcFollowupTo;
  Widget mcSubject;
  Widget mcAttachments;
  Widget mcBodyText;
  Widget deliverNow_menuitem;
  Widget deliverLater_menuitem;
  XP_Bool mcCitedAndUnedited;
  XP_Bool mcEdited;
  Boolean compose_wrap_lines_p;
  struct fe_mail_attach_data *mad;


  /* Pixels allocated per window. */
  XColor *color_data;
  int color_size;
  int color_fp;

  /* Dialogs of which there is only one per context. */
  Widget history_dialog, file_dialog;
  struct fe_file_type_data *ftd;
  struct fe_source_data *sd;
  struct fe_docinfo_data *did;

  /* The dialog used when doing something synchronously per context,
     like downloading a file or delivering mail. */
  Widget synchronous_url_dialog;
  int synchronous_url_exit_status;

  Dimension sb_w, sb_h;		/* Width & height of the main scrollbars. */

  Pixel fg_pixel;
  Pixel bg_pixel;
  Pixel top_shadow_pixel;
  Pixel bottom_shadow_pixel;
  Pixel text_bg_pixel;          /* enabled text background color */

  int bg_red;                           /* Nominal background color */
  int bg_green;
  int bg_blue;

  Boolean icon_colors_initialized;
  struct fe_colormap *colormap;

  Pixmap backdrop_pixmap;

  int active_url_count;			/* Number of transfers in progress. */
  Boolean clicking_blocked;		/* Save me from myself! */
  Boolean save_next_mode_p;		/* "Save Next" prompt in progress */

  LO_Element *current_edge;		/* The grid edge being moved */
  Boolean focus_grid;			/* The grid with focus */
  int8 grid_scrolling;			/* Grid scrolling policy */

  unsigned long document_width;		/* Actual size of whole document. */
  unsigned long document_height;
  unsigned long scrolled_width;		/* drawing_area size @ last repaint */
  unsigned long scrolled_height;
  Boolean relayout_required;		/* Set when size change occurs before
					   document has been completely layed
					   out the first time. */
  const char *force_load_images;	/* Hack for "Load Images" command. */
  unsigned long document_x;
  unsigned long document_y;
  unsigned long line_height;
  time_t doc_size_last_update_time;	/* So FE_SetDocSize() can be lazy. */

  int16 xfe_doc_csid;

  Boolean bookmark_menu_up_to_date_p;
  Boolean windows_menu_up_to_date_p;

  int expose_x1;
  int expose_y1;
  int expose_x2;
  int expose_y2;
  Boolean held_expose;
  int expose_x_offset;
  int expose_y_offset;
  unsigned long expose_serial;

  /* Data used by thermo.c for status notification:
   */
  XtIntervalId thermo_timer_id;		/* timer running the animation */
  time_t thermo_start_time;		/* when transfer started (requested) */
  time_t thermo_data_start_time;	/* when transfer REALLY started */
  time_t thermo_last_update_time;	/* time we last printed text message */

  Boolean thermo_size_unknown_count;	/* The number of transfers in progress
					   whose content-length is not known */
  int thermo_current;			/* total bytes-so-far */
  int thermo_total;			/* total content-length of all docs
					   whose sizes are known. */
  int thermo_lo_percent;		/* percent of layout complete */

  int thermo_cylon;			/* if !thermo_size_known_p, this is the
					   last pixel position of the cylon
					   thingy.  It's negative if it is in
					   motion backwards. */

  XtIntervalId LED_timer_id;		/* timer running the light */
  XtIntervalId anim_timer_id;		/* timer running the animation */
  Boolean LED_on_p;			/* blink state */
  int logo_frame;			/* To keep the logo moving at a
					   reasonable clip, no matter how many
					   contexts are loading at once. */


  XtIntervalId blink_timer_id;		/* timer for blinking (gag) */
  struct fe_blinker *blinkers;
  Boolean blinking_enabled_p;

  Boolean delayed_images_p;

  /* Data for that FE_SetRefreshURLTimer() repulsive kludge. */
  XtIntervalId refresh_url_timer;
  uint32 refresh_url_timer_secs;
  char *refresh_url_timer_url;


  /* X Selection data
   */
  char *selection;
  char *clipboard;
  Time selection_time;
  Time clipboard_time;

  /* Things initialized from the resource database:
   */
  Pixel link_pixel;
  Pixel vlink_pixel;
  Pixel alink_pixel;
  Pixel select_fg_pixel;
  Pixel select_bg_pixel;
  Pixel default_fg_pixel;
  Pixel default_bg_pixel;
  Pixel thermo_pixel;
  Pixel LED_pixel;
#ifdef HAVE_SECURITY
  Pixel secure_document_pixel;
  Pixel insecure_document_pixel;
#endif

  char *default_background_image;

  XmString unedited_label_string;
  XmString edited_label_string;
  XmString netsite_label_string;

  Boolean confirm_exit_p;
  Boolean show_url_p;
  Boolean show_toolbar_p;
  Boolean show_toolbar_icons_p;
  Boolean show_toolbar_text_p;
  Boolean show_directory_buttons_p;
  Boolean show_menubar_p;
  Boolean show_bottom_status_bar_p;
#ifdef EDITOR
  Boolean show_character_toolbar_p;
  Boolean show_paragraph_toolbar_p;
#endif /* EDITOR */
  Boolean autoload_images_p;
  Boolean fancy_ftp_p;
#ifdef HAVE_SECURITY
  Boolean show_security_bar_p;
#endif
#ifdef JAVA
  Boolean show_java_console_p;
#endif

  int progress_interval;	/* How quickly %-done text updates */
  int busy_blink_rate;		/* How quickly light blinks (microseconds) */
  int anim_rate;		/* How quickly logo animates (microseconds) */
  Cursor link_cursor;
  Cursor busy_cursor;

  Cursor save_next_link_cursor;
  Cursor save_next_nonlink_cursor;
#ifdef EDITOR
  Cursor editable_text_cursor;     /* maybe others want to use this */
#endif /*EDITOR*/

  struct fe_bookmark_data* bmdata; /* Bookmark data (used only by hot.c) */

  Boolean hasChrome;
  Chrome chrome;

  /* a handle to the current pref dialog being displayed. this
     is useful if you need to create a modal html dialog as it's
     child, as in the case of the security prefs.
     */
  Widget currentPrefDialog;
  struct fe_prefs_data *fep;

  /* If this context is a full page plugin, then this flag gets set.
     Because of this, resize events are passed on to the plugin.
  */
  Boolean is_fullpage_plugin;

  Boolean being_destroyed;

  /* If dont_free_context_memory  is non-zero on the context,
   * fe_DestroyContext() will not release the
   * memory for the context although it will go ahead and do all other
   * destroy stuff like destroying the widgets etc. Just the memory for
   * both the context and the context-data is retained.
   *
   * delete_response is used to store the XmNdeleteResponse of the context
   * while the context is in a protected state. During protected state,
   * the context's XmNdeleteResponse will be forced to XmDO_NOTHING, indicating
   * that the user cannot destroy this context using the window manager
   * delete menu item. Once the context is completely unprotected, the
   * XmNdeleteResponse will be restored to the stored delete_response value.
   *
   *********** DO NOT ACCESS THESE VARAIBLES DIRECTLY.
   * WARNING * Use this via fe_ProtectContext(), fe_UnProtectContext() and
   *********** fe_IsContextProtected().
   */
  int dont_free_context_memory;
  unsigned char delete_response;

  /* If a context is destroyed and dont_free_context_memory is set,
   * then this will be set indicating that the context was destroyed but
   * the context and context-data memory was not reclaimed. It will be the
   * responsibility of the module that set dont_free_context_memory to
   * reclaim the context memory if this is set on the context.
   *
   * The situation this is happen is
   * 	1. when a synchronous_url_dialog is up
   *	2. when a popup on a say frame-context is up and before we can
   *	   bring the popup down, the frame-context gets destroyed with a
   *	   JavaScript timer.
   */
  Boolean destroyed;

  /* Mocha hack for changing input elements. XFE_GetFormElementInfo can
     be called with when the form is created or in FE_ChangeInputElement
     when the form is being changed using mocha. To distinguish these
     we use this flag that identifies if this was mocha changing the
     input element.
  */
  XP_Bool mocha_changing_input_element;
  fe_FindData *find_data;

  fe_NewsContextData *news_part;
#ifdef EDITOR
  fe_EditorContextData editor;     /* editor stuff */
  char *save_file_url;
  int  file_save_status;
  XP_Bool is_file_saving;
#endif /*EDITOR*/
  fe_DependentList* dependents;    /* dependency list */
  Widget            posted_msg_box;

} fe_ContextData;

#define EDITOR_CONTEXT_DATA(context)	((&CONTEXT_DATA(context)->editor))


/* Magic value used in fe_ContextData->force_load_images to mean "load all." */
#define FORCE_LOAD_ALL_IMAGES ((char *)1)


typedef struct
{
  /* Stuff from the resource database that is not per-window. */

  XtIntervalId save_history_id;		/* timer for periodically saving
					   the history and bookmark files. */
  int save_history_interval;		/* Every N seconds. */

  Visual *default_visual;
  struct fe_colormap *common_colormap; /* If private colormap, colormap for
                                          "simple" contexts, else common
                                          colormap for all contexts. */

  struct fe_colormap *default_colormap; /* fe wrapper around X default
                                           colormap */

  Boolean always_install_cmap;
#ifdef USE_NONSHARED_COLORMAPS
  Boolean windows_share_cmap;
#endif
  Boolean force_mono_p;
  char*   wm_icon_policy;

  Boolean document_beats_user_p;	/* #### move to prefs */

#ifdef __sgi
  Boolean sgi_mode_p;
#endif /* __sgi */

  int max_image_colors;		/* Max color cells to gobble up. */

  int fe_guffaw_scroll;		/* Brokenness of server WindowGravity */

  const char *user_prefs_file;

  Boolean stderr_dialog_p;
  Boolean stdout_dialog_p;

  const char *encoding_filters;

  XtTranslations terminal_text_translations;
  XtTranslations nonterminal_text_translations;
  XtTranslations global_translations;
  XtTranslations global_text_field_translations;
  XtTranslations global_nontext_translations;

  XtTranslations editing_translations;
  XtTranslations single_line_editing_translations;
  XtTranslations multi_line_editing_translations;

  XtTranslations bm_global_translations;
  XtTranslations mailnews_global_translations;
  XtTranslations mailcompose_global_translations;
  XtTranslations dialog_global_translations;
  XtTranslations editor_global_translations;

  /* Random error messages and things.
   */
  const char *options_saved_message;
  const char *click_to_save_message;
  const char *click_to_save_cancelled_message;
  const char *no_url_loaded_message;
  const char *no_next_url_message;
  const char *no_previous_url_message;
  const char *no_home_url_message;
  const char *not_over_image_message;
  const char *not_over_link_message;
  const char *no_search_string_message;
  const char *wrap_search_message;
  const char *wrap_search_backward_message;
  const char *no_addresses_message;
  const char *no_file_message;
  const char *no_print_command_message;
  const char *bookmarks_changed_message;
  const char *bookmark_conflict_message;
  const char *bookmarks_no_forms_message;
  const char *create_cache_dir_message;
  const char *created_cache_dir_message;
  const char *cache_not_dir_message;
  const char *cache_suffix_message;
  const char *cube_too_small_message;
  const char *really_quit_message;
  const char *double_inclusion_message;
  const char *expire_now_message;
  const char *clear_mem_cache_message;
  const char *clear_disk_cache_message;
  const char *rename_files_message;
  const char *overwrite_file_message;
  const char *unsent_mail_message;
  const char *binary_document_message;
  const char *empty_message_message;
  const char *default_mailto_text;
  const char *default_url_charset;
  const char *helper_app_delete_message; /* For Helper App Delete */

  /*
   * Enterprise Kit stuff
   */
  #define MAX_DIRECTORY_BUTTONS    6
  #define MAX_DIRECTORY_MENU_ITEMS 25
  #define MAX_HELP_MENU_ITEMS      25

  char* homePage;
  int numDirectoryButtons;
  int numDirectoryMenuItems;
  int numHelpMenuItems;
  char* directoryButtonUrls[MAX_DIRECTORY_BUTTONS];
  char* directoryMenuUrls[MAX_DIRECTORY_MENU_ITEMS];
  char* helpMenuUrls[MAX_HELP_MENU_ITEMS];

  /*
   * Enterprise Kit Proxy information is put in fe_globalPrefs
   */

  /* More global data that aren't resource related */
  time_t privateMimetypeFileModifiedTime;
  time_t privateMailcapFileModifiedTime;

} fe_GlobalData;

extern fe_GlobalData   fe_globalData;
extern XFE_GlobalPrefs fe_globalPrefs;
extern XFE_GlobalPrefs fe_defaultPrefs;


struct fe_MWContext_cons
{
  MWContext *context;
  struct fe_MWContext_cons *next;
};

extern struct fe_MWContext_cons *fe_all_MWContexts;


#define CONTEXT_WIDGET(context)	((CONTEXT_DATA (context))->widget)
#define CONTEXT_DATA(context)	((context)->fe.data)
#define NEWS_CONTEXT_DATA(context) ((fe_ContextData *)CONTEXT_DATA(context))->news_part


#define fe_FILE_TYPE_NONE		1
#define fe_FILE_TYPE_TEXT		2
#define fe_FILE_TYPE_FORMATTED_TEXT	3
#define fe_FILE_TYPE_HTML		4
#define fe_FILE_TYPE_HTML_AND_IMAGES	5
#define fe_FILE_TYPE_PS			6
#define fe_FILE_TYPE_URL_ONLY		7

/* Saving to disk
 */

struct save_as_data
{
  MWContext *context;
  char *name;
  FILE *file;
  int type;
  int insert_base_tag;
  Boolean use_dialog_p;
  void (*done)(struct save_as_data *);
  void* data;
  int content_length;
  int bytes_read;
};


/* icons */

extern Pixel fe_gray_map [256];

struct fe_Pixmap
{
  enum { fep_nothing, fep_image, fep_icon } type;
  union { IL_Image *il_image; int icon_number; } data;
  IL_ImageStatus status;
  Boolean image_coords_valid;
  Boolean image_display_begun;
  Boolean bits_creator_p;
};

extern struct fe_Pixmap * fe_new_fe_Pixmap(void);

/*
 * Mocha can destroy the document while we do a LM_Send call.
 * We protect againsta this by storing the doc_id before the call
 * and checking this against the new doc_id after the call.
 *
 * In general any call to Mocha to send events (LM_Send*()) should
 * be like this:
 *	CALLING_MOCHA(context);
 *	LM_Send*(context, form);
 *	if (IS_DOCUMENT_DESTROYED(context)) return FALSE/TRUE/what-ever;
 *
 */
extern int32 _doc_id_before_calling_mocha;
#define CALLING_MOCHA(context) \
		(_doc_id_before_calling_mocha = XP_DOCID(context))
#define IS_FORM_DESTROYED(context) \
		(_doc_id_before_calling_mocha != XP_DOCID(context))

/*
 * Mail Compose Callbacks in XFE
 */
typedef enum {
    ComposeSelectAllText_cb,
    ComposeClearAllText_cb
} fe_MailComposeCallback;

void
fe_mailcompose_obeycb(MWContext *context, fe_MailComposeCallback cbid,
			void *call_data);

void
fe_key_input_cb (Widget widget, XtPointer closure, XtPointer call_data);

#define IL_ICON_BACK		 (IL_SA_LAST+1)
#define IL_ICON_BACK_GREY	 (IL_ICON_BACK+1)
#define IL_ICON_FWD		 (IL_ICON_BACK+2)
#define IL_ICON_FWD_GREY	 (IL_ICON_BACK+3)
#define IL_ICON_HOME		 (IL_ICON_BACK+4)
#define IL_ICON_HOME_GREY	 (IL_ICON_BACK+5)

#define IL_ICON_RELOAD		 (IL_ICON_HOME_GREY+1)
#define IL_ICON_RELOAD_GREY	 (IL_ICON_RELOAD+1)
#define IL_ICON_LOAD		 (IL_ICON_RELOAD+2)
#define IL_ICON_LOAD_GREY	 (IL_ICON_RELOAD+3)
#define IL_ICON_OPEN		 (IL_ICON_RELOAD+4)
#define IL_ICON_OPEN_GREY	 (IL_ICON_RELOAD+5)
#define IL_ICON_PRINT	 	 (IL_ICON_RELOAD+6)
#define IL_ICON_PRINT_GREY	 (IL_ICON_RELOAD+7)
#define IL_ICON_FIND		 (IL_ICON_RELOAD+8)
#define IL_ICON_FIND_GREY	 (IL_ICON_RELOAD+9)

#define IL_ICON_STOP		 (IL_ICON_FIND_GREY+1)
#define IL_ICON_STOP_GREY	 (IL_ICON_STOP+1)

#define IL_ICON_NETSCAPE	 (IL_ICON_STOP_GREY+1)

#define IL_ICON_BACK_PT		 (IL_ICON_NETSCAPE+1)
#define IL_ICON_BACK_PT_GREY	 (IL_ICON_BACK_PT+1)
#define IL_ICON_FWD_PT		 (IL_ICON_BACK_PT+2)
#define IL_ICON_FWD_PT_GREY	 (IL_ICON_BACK_PT+3)
#define IL_ICON_HOME_PT		 (IL_ICON_BACK_PT+4)
#define IL_ICON_HOME_PT_GREY	 (IL_ICON_BACK_PT+5)

#define IL_ICON_RELOAD_PT	 (IL_ICON_HOME_PT_GREY+1)
#define IL_ICON_RELOAD_PT_GREY	 (IL_ICON_RELOAD_PT+1)
#define IL_ICON_LOAD_PT		 (IL_ICON_RELOAD_PT+2)
#define IL_ICON_LOAD_PT_GREY	 (IL_ICON_RELOAD_PT+3)
#define IL_ICON_OPEN_PT		 (IL_ICON_RELOAD_PT+4)
#define IL_ICON_OPEN_PT_GREY	 (IL_ICON_RELOAD_PT+5)
#define IL_ICON_PRINT_PT 	 (IL_ICON_RELOAD_PT+6)
#define IL_ICON_PRINT_PT_GREY	 (IL_ICON_RELOAD_PT+7)
#define IL_ICON_FIND_PT		 (IL_ICON_RELOAD_PT+8)
#define IL_ICON_FIND_PT_GREY	 (IL_ICON_RELOAD_PT+9)

#define IL_ICON_STOP_PT		 (IL_ICON_FIND_PT_GREY+1)
#define IL_ICON_STOP_PT_GREY	 (IL_ICON_STOP_PT+1)

#define IL_ICON_NETSCAPE_PT      (IL_ICON_STOP_PT_GREY+1)

#define IL_ICON_SECURITY_ON	 (IL_ICON_NETSCAPE_PT+1)
#define IL_ICON_SECURITY_OFF	 (IL_ICON_SECURITY_ON+1)
#define IL_ICON_SECURITY_HIGH	 (IL_ICON_SECURITY_OFF+1)
#ifndef FORTEZZA
#define IL_ICON_SECURITY_MIXED   (IL_ICON_SECURITY_HIGH+1)
#else
#define IL_ICON_SECURITY_FORTEZZA (IL_ICON_SECURITY_HIGH+1)
#define IL_ICON_SECURITY_MIXED   (IL_ICON_SECURITY_FORTEZZA+1)
#endif

#define IL_DIRECTORY_ICON_FIRST  (IL_ICON_SECURITY_MIXED+1)
#define IL_ICON_TOUR		 (IL_DIRECTORY_ICON_FIRST)
#define IL_ICON_WHATS_NEW	 (IL_ICON_TOUR+1)
#define IL_ICON_INET_INDEX	 (IL_ICON_TOUR+2)
#define IL_ICON_INET_SEARCH	 (IL_ICON_TOUR+3)
#define IL_ICON_FAQ		 (IL_ICON_TOUR+4)
#define IL_ICON_NEWSRC		 (IL_ICON_TOUR+5)
#define IL_DIRECTORY_ICON_LAST   (IL_ICON_NEWSRC)

#define IL_ICON_DESKTOP_NAVIGATOR	(IL_DIRECTORY_ICON_LAST+1)
#define IL_ICON_DESKTOP_ABOOK	 	(IL_ICON_DESKTOP_NAVIGATOR+1)
#define IL_ICON_DESKTOP_BOOKMARK	(IL_ICON_DESKTOP_ABOOK+1)
#define IL_ICON_DESKTOP_NEWS	 	(IL_ICON_DESKTOP_BOOKMARK+1)
#define IL_ICON_DESKTOP_NOMAIL	 	(IL_ICON_DESKTOP_NEWS+1)
#define IL_ICON_DESKTOP_YESMAIL	 	(IL_ICON_DESKTOP_NOMAIL+1)
#define IL_ICON_DESKTOP_COMPOSE	 	(IL_ICON_DESKTOP_YESMAIL+1)
#define IL_ICON_DESKTOP_EDITOR	 	(IL_ICON_DESKTOP_COMPOSE+1)

#define IL_MSG_HIER_ARTICLE		(IL_ICON_DESKTOP_EDITOR+1)
#define IL_MSG_HIER_FOLDER_CLOSED	(IL_MSG_HIER_ARTICLE+1)
#define IL_MSG_HIER_FOLDER_OPEN		(IL_MSG_HIER_FOLDER_CLOSED+1)
#define IL_MSG_HIER_NEWSGROUP		(IL_MSG_HIER_FOLDER_OPEN+1)
#define IL_MSG_HIER_MESSAGE		(IL_MSG_HIER_NEWSGROUP+1)

#define IL_MSG_HIER_SUBSCRIBED		(IL_MSG_HIER_MESSAGE+1)
#define IL_MSG_HIER_UNSUBSCRIBED	(IL_MSG_HIER_SUBSCRIBED+1)
#define IL_MSG_HIER_READ		(IL_MSG_HIER_UNSUBSCRIBED+1)
#define IL_MSG_HIER_UNREAD		(IL_MSG_HIER_READ+1)
#define IL_MSG_HIER_MARKED		(IL_MSG_HIER_UNREAD+1)
#define IL_MSG_HIER_UNMARKED		(IL_MSG_HIER_MARKED+1)

#define IL_MSG_DELETE			(IL_MSG_HIER_UNMARKED+1)
#define IL_MSG_DELETE_GREY		(IL_MSG_DELETE+1)
#define IL_MSG_DELETE_PT		(IL_MSG_DELETE_GREY+1)
#define IL_MSG_DELETE_PT_GREY		(IL_MSG_DELETE_PT+1)
#define IL_MSG_GET_MAIL			(IL_MSG_DELETE_PT_GREY+1)
#define IL_MSG_GET_MAIL_GREY		(IL_MSG_GET_MAIL+1)
#define IL_MSG_GET_MAIL_PT		(IL_MSG_GET_MAIL_GREY+1)
#define IL_MSG_GET_MAIL_PT_GREY		(IL_MSG_GET_MAIL_PT+1)
#define IL_MSG_MARK_ALL_READ		(IL_MSG_GET_MAIL_PT_GREY+1)
#define IL_MSG_MARK_ALL_READ_GREY	(IL_MSG_MARK_ALL_READ+1)
#define IL_MSG_MARK_ALL_READ_PT		(IL_MSG_MARK_ALL_READ_GREY+1)
#define IL_MSG_MARK_ALL_READ_PT_GREY	(IL_MSG_MARK_ALL_READ_PT+1)
#define IL_MSG_MARK_THREAD_READ		(IL_MSG_MARK_ALL_READ_PT_GREY+1)
#define IL_MSG_MARK_THREAD_READ_GREY	(IL_MSG_MARK_THREAD_READ+1)
#define IL_MSG_MARK_THREAD_READ_PT	(IL_MSG_MARK_THREAD_READ_GREY+1)
#define IL_MSG_MARK_THREAD_READ_PT_GREY	(IL_MSG_MARK_THREAD_READ_PT+1)
#define IL_MSG_FORWARD_MSG		(IL_MSG_MARK_THREAD_READ_PT_GREY+1)
#define IL_MSG_FORWARD_MSG_GREY		(IL_MSG_FORWARD_MSG+1)
#define IL_MSG_FORWARD_MSG_PT		(IL_MSG_FORWARD_MSG_GREY+1)
#define IL_MSG_FORWARD_MSG_PT_GREY	(IL_MSG_FORWARD_MSG_PT+1)
#define IL_MSG_NEW_MSG			(IL_MSG_FORWARD_MSG_PT_GREY+1)
#define IL_MSG_NEW_MSG_GREY		(IL_MSG_NEW_MSG+1)
#define IL_MSG_NEW_MSG_PT		(IL_MSG_NEW_MSG_GREY+1)
#define IL_MSG_NEW_MSG_PT_GREY		(IL_MSG_NEW_MSG_PT+1)
#define IL_MSG_NEW_POST			(IL_MSG_NEW_MSG_PT_GREY+1)
#define IL_MSG_NEW_POST_GREY		(IL_MSG_NEW_POST+1)
#define IL_MSG_NEW_POST_PT		(IL_MSG_NEW_POST_GREY+1)
#define IL_MSG_NEW_POST_PT_GREY		(IL_MSG_NEW_POST_PT+1)
#define IL_MSG_FOLLOWUP			(IL_MSG_NEW_POST_PT_GREY+1)
#define IL_MSG_FOLLOWUP_GREY		(IL_MSG_FOLLOWUP+1)
#define IL_MSG_FOLLOWUP_PT		(IL_MSG_FOLLOWUP_GREY+1)
#define IL_MSG_FOLLOWUP_PT_GREY		(IL_MSG_FOLLOWUP_PT+1)
#define IL_MSG_FOLLOWUP_AND_REPLY	(IL_MSG_FOLLOWUP_PT_GREY+1)
#define IL_MSG_FOLLOWUP_AND_REPLY_GREY	(IL_MSG_FOLLOWUP_AND_REPLY+1)
#define IL_MSG_FOLLOWUP_AND_REPLY_PT	(IL_MSG_FOLLOWUP_AND_REPLY_GREY+1)
#define IL_MSG_FOLLOWUP_AND_REPLY_PT_GREY (IL_MSG_FOLLOWUP_AND_REPLY_PT+1)
#define IL_MSG_REPLY_TO_SENDER		(IL_MSG_FOLLOWUP_AND_REPLY_PT_GREY+1)
#define IL_MSG_REPLY_TO_SENDER_GREY	(IL_MSG_REPLY_TO_SENDER+1)
#define IL_MSG_REPLY_TO_SENDER_PT	(IL_MSG_REPLY_TO_SENDER_GREY+1)
#define IL_MSG_REPLY_TO_SENDER_PT_GREY	(IL_MSG_REPLY_TO_SENDER_PT+1)
#define IL_MSG_REPLY_TO_ALL		(IL_MSG_REPLY_TO_SENDER_PT_GREY+1)
#define IL_MSG_REPLY_TO_ALL_GREY	(IL_MSG_REPLY_TO_ALL+1)
#define IL_MSG_REPLY_TO_ALL_PT		(IL_MSG_REPLY_TO_ALL_GREY+1)
#define IL_MSG_REPLY_TO_ALL_PT_GREY	(IL_MSG_REPLY_TO_ALL_PT+1)
#define IL_MSG_NEXT_UNREAD		(IL_MSG_REPLY_TO_ALL_PT_GREY+1)
#define IL_MSG_NEXT_UNREAD_GREY		(IL_MSG_NEXT_UNREAD+1)
#define IL_MSG_NEXT_UNREAD_PT		(IL_MSG_NEXT_UNREAD_GREY+1)
#define IL_MSG_NEXT_UNREAD_PT_GREY	(IL_MSG_NEXT_UNREAD_PT+1)
#define IL_MSG_PREV_UNREAD		(IL_MSG_NEXT_UNREAD_PT_GREY+1)
#define IL_MSG_PREV_UNREAD_GREY		(IL_MSG_PREV_UNREAD+1)
#define IL_MSG_PREV_UNREAD_PT		(IL_MSG_PREV_UNREAD_GREY+1)
#define IL_MSG_PREV_UNREAD_PT_GREY	(IL_MSG_PREV_UNREAD_PT+1)

#define IL_COMPOSE_SEND		 	(IL_MSG_PREV_UNREAD_PT_GREY+1)
#define IL_COMPOSE_SEND_GREY	 	(IL_COMPOSE_SEND+1)
#define IL_COMPOSE_SEND_PT		(IL_COMPOSE_SEND_GREY+1)
#define IL_COMPOSE_SEND_PT_GREY	 	(IL_COMPOSE_SEND_PT+1)
#define IL_COMPOSE_ATTACH		(IL_COMPOSE_SEND_PT_GREY+1)
#define IL_COMPOSE_ATTACH_GREY	 	(IL_COMPOSE_ATTACH+1)
#define IL_COMPOSE_ATTACH_PT	 	(IL_COMPOSE_ATTACH_GREY+1)
#define IL_COMPOSE_ATTACH_PT_GREY	(IL_COMPOSE_ATTACH_PT+1)
#define IL_COMPOSE_ADDRESSBOOK		(IL_COMPOSE_ATTACH_PT_GREY+1)
#define IL_COMPOSE_ADDRESSBOOK_GREY	(IL_COMPOSE_ADDRESSBOOK+1)
#define IL_COMPOSE_ADDRESSBOOK_PT	(IL_COMPOSE_ADDRESSBOOK_GREY+1)
#define IL_COMPOSE_ADDRESSBOOK_PT_GREY	(IL_COMPOSE_ADDRESSBOOK_PT+1)
#define IL_COMPOSE_QUOTE		(IL_COMPOSE_ADDRESSBOOK_PT_GREY+1)
#define IL_COMPOSE_QUOTE_GREY		(IL_COMPOSE_QUOTE+1)
#define IL_COMPOSE_QUOTE_PT		(IL_COMPOSE_QUOTE_GREY+1)
#define IL_COMPOSE_QUOTE_PT_GREY	(IL_COMPOSE_QUOTE_PT+1)
#define IL_COMPOSE_SENDLATER		(IL_COMPOSE_QUOTE_PT_GREY+1)
#define IL_COMPOSE_SENDLATER_GREY	(IL_COMPOSE_SENDLATER+1)
#define IL_COMPOSE_SENDLATER_PT		(IL_COMPOSE_SENDLATER_GREY+1)
#define IL_COMPOSE_SENDLATER_PT_GREY	(IL_COMPOSE_SENDLATER_PT+1)

/*FIXME need to do greys */
#define IL_EDITOR_GROUP         (IL_COMPOSE_SENDLATER_PT_GREY+1)
#define IL_EDITOR_NEW		(IL_EDITOR_GROUP+0)
#define IL_EDITOR_NEW_GREY	(IL_EDITOR_GROUP+1)
#define IL_EDITOR_NEW_PT	(IL_EDITOR_GROUP+2)
#define IL_EDITOR_NEW_PT_GREY	(IL_EDITOR_GROUP+3)
#define IL_EDITOR_OPEN		(IL_EDITOR_GROUP+4)
#define IL_EDITOR_OPEN_GREY	(IL_EDITOR_GROUP+5)
#define IL_EDITOR_OPEN_PT	(IL_EDITOR_GROUP+6)
#define IL_EDITOR_OPEN_PT_GREY	(IL_EDITOR_GROUP+7)
#define IL_EDITOR_SAVE		(IL_EDITOR_GROUP+8)
#define IL_EDITOR_SAVE_GREY	(IL_EDITOR_GROUP+9)
#define IL_EDITOR_SAVE_PT	(IL_EDITOR_GROUP+10)
#define IL_EDITOR_SAVE_PT_GREY	(IL_EDITOR_GROUP+11)
#define IL_EDITOR_BROWSE	(IL_EDITOR_GROUP+12)
#define IL_EDITOR_BROWSE_GREY	(IL_EDITOR_GROUP+13)
#define IL_EDITOR_BROWSE_PT	(IL_EDITOR_GROUP+14)
#define IL_EDITOR_BROWSE_PT_GREY (IL_EDITOR_GROUP+15)
#define IL_EDITOR_CUT		(IL_EDITOR_GROUP+16)
#define IL_EDITOR_CUT_GREY	(IL_EDITOR_GROUP+17)
#define IL_EDITOR_CUT_PT	(IL_EDITOR_GROUP+18)
#define IL_EDITOR_CUT_PT_GREY	(IL_EDITOR_GROUP+19)
#define IL_EDITOR_COPY		(IL_EDITOR_GROUP+20)
#define IL_EDITOR_COPY_GREY		(IL_EDITOR_GROUP+21)
#define IL_EDITOR_COPY_PT	(IL_EDITOR_GROUP+22)
#define IL_EDITOR_COPY_PT_GREY	(IL_EDITOR_GROUP+23)
#define IL_EDITOR_PASTE		(IL_EDITOR_GROUP+24)
#define IL_EDITOR_PASTE_GREY		(IL_EDITOR_GROUP+25)
#define IL_EDITOR_PASTE_PT	(IL_EDITOR_GROUP+26)
#define IL_EDITOR_PASTE_PT_GREY	(IL_EDITOR_GROUP+27)
#define IL_EDITOR_PRINT		(IL_EDITOR_GROUP+28)
#define IL_EDITOR_PRINT_GREY		(IL_EDITOR_GROUP+29)
#define IL_EDITOR_PRINT_PT	(IL_EDITOR_GROUP+30)
#define IL_EDITOR_PRINT_PT_GREY	(IL_EDITOR_GROUP+31)
#define IL_EDITOR_FIND		(IL_EDITOR_GROUP+32)
#define IL_EDITOR_FIND_GREY		(IL_EDITOR_GROUP+33)
#define IL_EDITOR_FIND_PT	(IL_EDITOR_GROUP+34)
#define IL_EDITOR_FIND_PT_GREY	(IL_EDITOR_GROUP+35)
#define IL_EDITOR_PUBLISH	(IL_EDITOR_GROUP+36)
#define IL_EDITOR_PUBLISH_GREY	(IL_EDITOR_GROUP+37)
#define IL_EDITOR_PUBLISH_PT	(IL_EDITOR_GROUP+38)
#define IL_EDITOR_PUBLISH_PT_GREY	(IL_EDITOR_GROUP+39)
#define IL_EDITOR_EDIT          (IL_EDITOR_GROUP+40)
#define IL_EDITOR_EDIT_GREY          (IL_EDITOR_GROUP+41)
#define IL_EDITOR_EDIT_PT	(IL_EDITOR_GROUP+42)
#define IL_EDITOR_EDIT_PT_GREY	(IL_EDITOR_GROUP+43)

#define IL_EDITOR_CHARACTER_TOOLBAR_ID 44
#define IL_EDITOR_OTHER_GROUP   (IL_EDITOR_EDIT_PT_GREY+1)
#define IL_EDITOR_CHARACTER_GROUP (IL_EDITOR_OTHER_GROUP)
#define IL_EDITOR_SHRINK        (IL_EDITOR_CHARACTER_GROUP+0)
#define IL_EDITOR_SHRINK_GREY   (IL_EDITOR_CHARACTER_GROUP+1)
#define IL_EDITOR_GROW          (IL_EDITOR_CHARACTER_GROUP+2)
#define IL_EDITOR_GROW_GREY     (IL_EDITOR_CHARACTER_GROUP+3)
#define IL_EDITOR_BOLD          (IL_EDITOR_CHARACTER_GROUP+4)
#define IL_EDITOR_BOLD_GREY     (IL_EDITOR_CHARACTER_GROUP+5)
#define IL_EDITOR_ITALIC        (IL_EDITOR_CHARACTER_GROUP+6)
#define IL_EDITOR_ITALIC_GREY   (IL_EDITOR_CHARACTER_GROUP+7)
#define IL_EDITOR_FIXED         (IL_EDITOR_CHARACTER_GROUP+8)
#define IL_EDITOR_FIXED_GREY    (IL_EDITOR_CHARACTER_GROUP+9)
#define IL_EDITOR_COLOR         (IL_EDITOR_CHARACTER_GROUP+10)
#define IL_EDITOR_COLOR_GREY    (IL_EDITOR_CHARACTER_GROUP+11)
#define IL_EDITOR_LINK          (IL_EDITOR_CHARACTER_GROUP+12)
#define IL_EDITOR_LINK_GREY     (IL_EDITOR_CHARACTER_GROUP+13)
#define IL_EDITOR_CLEAR         (IL_EDITOR_CHARACTER_GROUP+14)
#define IL_EDITOR_CLEAR_GREY    (IL_EDITOR_CHARACTER_GROUP+15)
#define IL_EDITOR_TARGET        (IL_EDITOR_CHARACTER_GROUP+16)
#define IL_EDITOR_TARGET_GREY   (IL_EDITOR_CHARACTER_GROUP+17)
#define IL_EDITOR_IMAGE         (IL_EDITOR_CHARACTER_GROUP+18)
#define IL_EDITOR_IMAGE_GREY    (IL_EDITOR_CHARACTER_GROUP+19)
#define IL_EDITOR_HRULE         (IL_EDITOR_CHARACTER_GROUP+20)
#define IL_EDITOR_HRULE_GREY    (IL_EDITOR_CHARACTER_GROUP+21)
#define IL_EDITOR_TABLE         (IL_EDITOR_CHARACTER_GROUP+22)
#define IL_EDITOR_TABLE_GREY    (IL_EDITOR_CHARACTER_GROUP+23)
#define IL_EDITOR_PROPS         (IL_EDITOR_CHARACTER_GROUP+24)
#define IL_EDITOR_PROPS_GREY    (IL_EDITOR_CHARACTER_GROUP+25)

/* why? #define IL_EDITOR_PARAGRAPH_TOOLBAR_ID 70 */
#define IL_EDITOR_PARAGRAPH_GROUP (IL_EDITOR_CHARACTER_GROUP+26)
#define IL_EDITOR_BULLET        (IL_EDITOR_PARAGRAPH_GROUP+0)
#define IL_EDITOR_BULLET_GREY   (IL_EDITOR_PARAGRAPH_GROUP+1)
#define IL_EDITOR_NUMBER        (IL_EDITOR_PARAGRAPH_GROUP+2)
#define IL_EDITOR_NUMBER_GREY   (IL_EDITOR_PARAGRAPH_GROUP+3)
#define IL_EDITOR_OUTDENT       (IL_EDITOR_PARAGRAPH_GROUP+4)
#define IL_EDITOR_OUTDENT_GREY  (IL_EDITOR_PARAGRAPH_GROUP+5)
#define IL_EDITOR_INDENT        (IL_EDITOR_PARAGRAPH_GROUP+6)
#define IL_EDITOR_INDENT_GREY   (IL_EDITOR_PARAGRAPH_GROUP+7)
#define IL_EDITOR_LEFT          (IL_EDITOR_PARAGRAPH_GROUP+8)
#define IL_EDITOR_LEFT_GREY     (IL_EDITOR_PARAGRAPH_GROUP+9)
#define IL_EDITOR_CENTER        (IL_EDITOR_PARAGRAPH_GROUP+10)
#define IL_EDITOR_CENTER_GREY   (IL_EDITOR_PARAGRAPH_GROUP+11)
#define IL_EDITOR_RIGHT         (IL_EDITOR_PARAGRAPH_GROUP+12)
#define IL_EDITOR_RIGHT_GREY    (IL_EDITOR_PARAGRAPH_GROUP+13)

#define IL_EDITOR_ALIGN_GROUP   (IL_EDITOR_PARAGRAPH_GROUP+14)
#define IL_ALIGN1_RAISED        (IL_EDITOR_ALIGN_GROUP+0)
#define IL_ALIGN1_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+1)
#define IL_ALIGN2_RAISED        (IL_EDITOR_ALIGN_GROUP+2)
#define IL_ALIGN2_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+3)
#define IL_ALIGN3_RAISED        (IL_EDITOR_ALIGN_GROUP+4)
#define IL_ALIGN3_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+5)
#define IL_ALIGN4_RAISED        (IL_EDITOR_ALIGN_GROUP+6)
#define IL_ALIGN4_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+7)
#define IL_ALIGN5_RAISED        (IL_EDITOR_ALIGN_GROUP+8)
#define IL_ALIGN5_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+9)
#define IL_ALIGN6_RAISED        (IL_EDITOR_ALIGN_GROUP+10)
#define IL_ALIGN6_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+11)
#define IL_ALIGN7_RAISED        (IL_EDITOR_ALIGN_GROUP+12)
#define IL_ALIGN7_DEPRESSED     (IL_EDITOR_ALIGN_GROUP+13)

#define IL_ICON_LOGO		(IL_ALIGN7_DEPRESSED+1)

#endif /* _XFE_H_ */
