/* -*- Mode: C; tab-width: 8 -*-
   editor.c --- XFE editor functions
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Spencer Murray <spence@netscape.com>, 11-Jan-96.
 */


#ifdef EDITOR

#include "mozilla.h"
#include "xfe.h"
#include <X11/keysym.h>   /* for editor key translation */
#include <Xm/CutPaste.h>   /* for editor key translation */

#include <xpgetstr.h>     /* for XP_GetString() */
#include <edttypes.h>
#include <edt.h>
#include <layout.h>
#include <secnav.h>

#include "menu.h"
#include "fonts.h"
#include "felocale.h"

#include "xeditor.h"

#define CB_STATIC          /* let commands.c see it */

#define XFE_EDITOR_IM_SUPPORT 1

/*  Our event loop doesn't have workprocs, so we can't have two modal
 *  dialog conditions (one from frontend, one from backend)
#define EVENT_LOOP_HAS_WORKPROC
*/

extern int XFE_WARNING_AUTO_SAVE_NEW_MSG;
extern int XFE_FILE_OPEN;
extern int XFE_EDITOR_ALERT_FRAME_DOCUMENT;
extern int XFE_EDITOR_ALERT_ABOUT_DOCUMENT;
extern int XFE_ALERT_SAVE_CHANGES;
extern int XFE_WARNING_SAVE_CHANGES;
extern int XFE_ERROR_GENERIC_ERROR_CODE;
extern int XFE_EDITOR_COPY_DOCUMENT_BUSY;
extern int XFE_EDITOR_COPY_SELECTION_EMPTY;
extern int XFE_EDITOR_COPY_SELECTION_CROSSES_TABLE_DATA_CELL;
extern int XFE_EDITOR_COPY_DOCUMENT_BUSY;
extern int XFE_COMMAND_EMPTY;
extern int XFE_EDITOR_HTML_EDITOR_COMMAND_EMPTY;
extern int XFE_EDITOR_IMAGE_EDITOR_COMMAND_EMPTY;
extern int XFE_ACTION_SYNTAX_ERROR;
extern int XFE_ACTION_WRONG_CONTEXT;
extern int XFE_EDITOR_WARNING_REMOVE_LINK;
extern int XFE_UNKNOWN;
extern int XFE_ERROR_COPYRIGHT_HINT;
extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_ERROR_READ_ONLY;
extern int XFE_ERROR_BLOCKED;
extern int XFE_ERROR_BAD_URL;
extern int XFE_ERROR_FILE_OPEN;
extern int XFE_ERROR_FILE_WRITE;
extern int XFE_ERROR_CREATE_BAKNAME;
extern int XFE_ERROR_DELETE_BAKFILE;
extern int XFE_ERROR_WRITING_FILE;
extern int XFE_ERROR_SRC_NOT_FOUND;
extern int XFE_WARNING_SAVE_CONTINUE;
extern int XFE_EDITOR_DOCUMENT_TEMPLATE_EMPTY;
extern int XFE_EDITOR_BROWSE_LOCATION_EMPTY;
extern int XFE_UPLOADING_FILE;
extern int XFE_SAVING_FILE;
extern int XFE_LOADING_IMAGE_FILE;
extern int XFE_FILE_N_OF_N;

static void
fe_Bell(MWContext* context)
{
    XBell(XtDisplay(CONTEXT_WIDGET(context)), 0);
}

/*
 *    Utility stuff that should be somewhere else.
 */
static Widget
fe_CreateFormDialog(MWContext* context, char* name)
{
  Widget mainw = CONTEXT_WIDGET(context);
  Widget form;
  Arg    av[20];
  int    ac;
  Visual* v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  /*
   *    Inherit MainW attributes.
   */
  XtVaGetValues(mainw, XtNvisual, &v, XtNcolormap, &cmap,
		XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg(av[ac], XmNvisual, v); ac++;
  XtSetArg(av[ac], XmNdepth, depth); ac++;
  XtSetArg(av[ac], XmNcolormap, cmap); ac++;
  XtSetArg(av[ac], XmNtransientFor, mainw); ac++;

  XtSetArg(av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg(av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg(av[ac], XmNautoUnmanage, False); ac++;

  form = XmCreateFormDialog(mainw, name, av, ac);

  return form;
}

static Widget
fe_CreateFramedToggle(
		      MWContext* context,
		      Widget     parent, 
		      char*      name,
		      Widget*    toggle_addr
		      )
{
  Arg      av[20];
  Cardinal ac;
  char     buf[64];
  Widget   title;
  Widget   frame;
  Widget   toggle;
  Widget   text;
  Widget   column;

  strcpy(buf, name);
  strcat(buf, "Frame");
  ac = 0;
  XtSetArg(av[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
  frame = XmCreateFrame(parent, buf, av, ac);

  strcpy(buf, name);
  strcat(buf, "Title");
  ac = 0;
  XtSetArg(av[ac], XmNlabelType, XmSTRING); ac++;
  XtSetArg(av[ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  XtSetArg(av[ac], XmNchildHorizontalAlignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg(av[ac], XmNchildVerticalAlignment, XmALIGNMENT_CENTER); ac++;
  title = XmCreateLabelGadget(frame, buf, av, ac);
  XtManageChild(title);

  strcpy(buf, name);
  strcat(buf, "RowColumn");
  ac = 0;
  XtSetArg(av[ac], XmNorientation, XmVERTICAL); ac++;
  XtSetArg(av[ac], XmNpacking, XmPACK_TIGHT); ac++;
  XtSetArg(av[ac], XmNchildType, XmFRAME_WORKAREA_CHILD); ac++;
  column = XmCreateRowColumn(frame, buf, av, ac);
  XtManageChild(column);

  strcpy(buf, name);
  strcat(buf, "Toggle");
  ac = 0;
  XtSetArg(av[ac], XmNindicatorType, XmN_OF_MANY); ac++;
  toggle = XmCreateToggleButtonGadget(column, buf, av, ac);
  XtManageChild(toggle);
  if (toggle_addr)
    *toggle_addr = toggle;

  strcpy(buf, name);
  strcat(buf, "Text");
  ac = 0;
  XtSetArg(av[ac], XmNlabelType, XmSTRING); ac++;
  text = XmCreateLabelGadget(column, buf, av, ac);
  XtManageChild(text);

  return frame;
}

static Widget
fe_CreateSaveCancelButtonGroup(
			   MWContext* context,
			   Widget     parent, 
			   char*      name,
			   Arg*       p_args,
			   Cardinal   np_args,
			   Widget*    save_addr,
			   Widget*    cancel_addr
			   )
{
  Arg      av[20];
  Cardinal ac;
  Widget   column;
  Widget   save_button;
  Widget   cancel_button;
  Widget   spacer_label;
  XmString xms;
  Widget   children[8];
  Cardinal nchildren = 0;

  for (ac = 0; ac < np_args; ac++) {
    XtSetArg(av[ac], p_args[ac].name, p_args[ac].value);
  }

  XtSetArg(av[ac], XmNorientation, XmVERTICAL); ac++;
  XtSetArg(av[ac], XmNisAligned, TRUE); ac++;
  XtSetArg(av[ac], XmNentryAlignment, XmALIGNMENT_CENTER); ac++;
  XtSetArg(av[ac], XmNpacking, XmPACK_TIGHT); ac++;
  column = XmCreateRowColumn(parent, name, av, ac);

  ac = 0;
  save_button = XmCreatePushButtonGadget(column, "save", av, ac);
  children[nchildren++] = save_button;
  if (save_addr)
    *save_addr = save_button;

  ac = 0;
  cancel_button = XmCreatePushButtonGadget(column, "cancel", av, ac);
  children[nchildren++] = cancel_button;
  if (cancel_addr)
    *cancel_addr = cancel_button;

  xms = XmStringCreateLtoR("", XmSTRING_DEFAULT_CHARSET);
  XtSetArg(av[ac], XmNlabelType, XmSTRING); ac++;
  XtSetArg(av[ac], XmNlabelString, xms); ac++;
  spacer_label = XmCreateLabelGadget(column, "spacer", av, ac);
  XmStringFree (xms);
  children[nchildren++] = spacer_label;

#ifndef NO_HELP
  ac = 0;
  XtSetArg(av[ac], XtNsensitive, False); ac++;
  children[nchildren++] = XmCreatePushButtonGadget(column, "help", av, ac);
#endif

  XtManageChildren(children, nchildren);

  return column;
}

static int fe_GetStandardPixmap_pixmapsInitialized;

static Pixmap
fe_GetStandardPixmap(MWContext* context, char type)
{
  Widget mainw = CONTEXT_WIDGET(context);
  Arg    av[20];
  int    ac;
  Cardinal depth;
  Screen* screen;
  Pixel   fg;
  Pixel   bg;
  char* name;
  Pixmap pixmap;

  switch (type) {
  case XmDIALOG_ERROR:       name = "xm_error";       break;
  case XmDIALOG_INFORMATION: name = "xm_information"; break;
  case XmDIALOG_QUESTION:    name = "xm_question";    break;
  case XmDIALOG_WARNING:     name = "xm_warning";     break;
  case XmDIALOG_WORKING:     name = "xm_working";     break;
  default:
    return XmUNSPECIFIED_PIXMAP;
  }

  /*
   *    This is so broken. Init MesageBox class, so the Motif
   *    standard icons get installed. YUCK..djw
   */
  if (!fe_GetStandardPixmap_pixmapsInitialized) {
    XtInitializeWidgetClass(xmMessageBoxWidgetClass);
    fe_GetStandardPixmap_pixmapsInitialized++;
  }

  /*
   *    Inherit MainW attributes.
   */
  ac = 0;
  XtSetArg(av[ac], XmNbackground, &bg); ac++;
  XtSetArg(av[ac], XmNforeground, &fg); ac++;
  XtSetArg(av[ac], XmNdepth, &depth); ac++;
  XtSetArg(av[ac], XmNscreen, &screen); ac++;
  XtGetValues(mainw, av, ac);

  pixmap = XmGetPixmapByDepth(screen, name, fg, bg, depth);
  if (pixmap == XmUNSPECIFIED_PIXMAP) {
    char default_name[256];

    strcpy(default_name, "default_");
    strcat(default_name, name);

    pixmap = XmGetPixmapByDepth(screen, default_name, fg, bg, depth);
  }

  return pixmap;
}

XtPointer
fe_GetUserData(Widget widget)
{
    void* rv;
    XtVaGetValues(widget, XmNuserData, &rv, 0);
    return rv;
}

static struct {
  unsigned type;
  char*    name;
} fe_CreateYesToAllDialog_button_data[] = {
  { XFE_DIALOG_YES_BUTTON,      "yes"      }, 
  { XFE_DAILOG_YESTOALL_BUTTON, "yesToAll" },
  { XFE_DIALOG_NO_BUTTON,       "no"       },
  { XFE_DIALOG_NOTOALL_BUTTON,  "noToAll"  },
  { XFE_DIALOG_CANCEL_BUTTON,   "cancel"   },
  { 0, NULL }
};



#define XFE_YESTOALL_YES_BUTTON      (fe_CreateYesToAllDialog_button_data[0].name)
#define XFE_YESTOALL_YESTOALL_BUTTON (fe_CreateYesToAllDialog_button_data[1].name)
#define XFE_YESTOALL_NO_BUTTON       (fe_CreateYesToAllDialog_button_data[2].name)
#define XFE_YESTOALL_NOTOALL_BUTTON  (fe_CreateYesToAllDialog_button_data[3].name)
#define XFE_YESTOALL_CANCEL_BUTTON   (fe_CreateYesToAllDialog_button_data[4].name)

typedef struct {
    Widget widget;
    char*  name;
} GetChildInfo;

static XtPointer
fe_YesToAllDialogGetChildMappee(Widget widget, XtPointer data)
{
    GetChildInfo* info = (GetChildInfo*)data;
    char*         name = XtName(widget);
    
    if (strcmp(info->name, name) == 0) {
	info->widget = widget;
	return (XtPointer)1; /* don't look any more */
    }
    return 0;
}

Widget
fe_YesToAllDialogGetChild(Widget parent, unsigned char type)
{
  char*        name;
  GetChildInfo info;

  switch (type) {
  case XFE_DIALOG_YES_BUTTON:
    name = XFE_YESTOALL_YES_BUTTON;
    break;
  case XFE_DAILOG_YESTOALL_BUTTON:
    name = XFE_YESTOALL_YESTOALL_BUTTON;
    break;
  case XFE_DIALOG_NO_BUTTON:
    name = XFE_YESTOALL_NO_BUTTON;
    break;
  case XFE_DIALOG_NOTOALL_BUTTON:
    name = XFE_YESTOALL_NOTOALL_BUTTON;
    break;
  case XFE_DIALOG_CANCEL_BUTTON:
    name = XFE_YESTOALL_CANCEL_BUTTON;
    break;
  default:
    return NULL; /* kill, kill, kill */
  }

  info.name = name;
  info.widget = NULL;

  fe_WidgetTreeWalk(parent, fe_YesToAllDialogGetChildMappee, &info);

  return info.widget;
}

static Widget
fe_CreateYesToAllDialog(MWContext* context, char* name, Arg* p_av, Cardinal p_ac)
{
  Arg      av[20];
  Cardinal ac;
  Widget   form;
  Widget   children[16];
  Cardinal nchildren = 0;
  Pixmap   pixmap;
  char* bname;
  Widget button;
  Widget icon;
  Widget text;
  Widget separator;
  Widget row;
  int i;
  XtCallbackRec* button_callback_rec = NULL;
  XmString msg_string = NULL;
  char namebuf[64];
  char pixmapType = XmDIALOG_WARNING;

  for (i = 0; i < p_ac; i++) {
      if (p_av[i].name == XmNarmCallback)
	  button_callback_rec = (XtCallbackRec*)p_av[i].value;
      else if (p_av[i].name == XmNmessageString)
	  msg_string = (XmString)p_av[i].value;
      else if (p_av[i].name == XmNdialogType)
	  pixmapType = (unsigned char)p_av[i].value;
  }

  form = fe_CreateFormDialog(context, name);

  strcpy(namebuf, name); strcat(namebuf, "Message");
  ac = 0;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNorientation, XmHORIZONTAL); ac++;
  XtSetArg(av[ac], XmNentryAlignment, XmALIGNMENT_CENTER); ac++;
  XtSetArg(av[ac], XmNisAligned, TRUE); ac++;
  XtSetArg(av[ac], XmNpacking, XmPACK_TIGHT); ac++;
  row = XmCreateRowColumn(form, namebuf, av, ac);

  nchildren = 0;

  /*
   *    Make pixmap label.
   */
  pixmap = fe_GetStandardPixmap(context, pixmapType);

  ac = 0;
  XtSetArg(av[ac], XmNlabelType, XmPIXMAP); ac++;
  XtSetArg(av[ac], XmNlabelPixmap, pixmap); ac++;
  icon = XmCreateLabelGadget(row, "icon", av, ac);
  children[nchildren++] = icon;

  /*
   *    Text.
   */
  ac = 0;
  if (msg_string)
    XtSetArg(av[ac], XmNlabelString, msg_string); ac++;
  XtSetArg(av[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg(av[ac], XmNlabelType, XmSTRING); ac++;
  text = XmCreateLabelGadget(row, "text", av, ac);
  children[nchildren++] = text;

  XtManageChildren(children, nchildren); /* children of row */

  nchildren = 0;
  children[nchildren++] = row;

  /*
   *    Separator.
   */
  ac = 0;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(av[ac], XmNtopWidget, row); ac++;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  separator = XmCreateSeparatorGadget(form, "separator", av, ac);
  children[nchildren++] = separator;
 
  /*
   *    Yes, YesToAll, No, NoToAll, Cancel.
   */
  strcpy(namebuf, name); strcat(namebuf, "Buttons");
  ac = 0;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(av[ac], XmNtopWidget, separator); ac++;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNorientation, XmHORIZONTAL); ac++;
  XtSetArg(av[ac], XmNentryAlignment, XmALIGNMENT_CENTER); ac++;
  XtSetArg(av[ac], XmNisAligned, TRUE); ac++;
  XtSetArg(av[ac], XmNpacking, XmPACK_COLUMN); ac++;
  row = XmCreateRowColumn(form, namebuf, av, ac);
  children[nchildren++] = row;

  XtManageChildren(children, nchildren);

  nchildren = 0; /* now do buttons */
  button = NULL;
  for (i = 0; (bname = fe_CreateYesToAllDialog_button_data[i].name); i++) {

    ac = 0;
    XtSetArg(av[ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
    XtSetArg(av[ac], XmNlabelType, XmSTRING); ac++;
    button = XmCreatePushButtonGadget(row, bname, av, ac);

    if (button_callback_rec)
      XtAddCallback(
		    button,
		    XmNactivateCallback,
		    button_callback_rec->callback,
		    button_callback_rec->closure
		    );
    children[nchildren++] = button;
  }

  XtManageChildren(children, nchildren);

  return form;
}

Boolean
fe_EditorDocumentIsSaved(MWContext* context)
{
    return (context->is_new_document == FALSE);
}


Boolean fe_SaveAsDialog(MWContext* context, char* buf, int type);

static Boolean
fe_editor_report_file_error(MWContext*   context,
			    ED_FileError code,
			    char*        filename, 
			    Boolean      ask_question)
{
    int   id;
    char* msg;
    char  buf[MAXPATHLEN];

    switch (code) {
    case ED_ERROR_READ_ONLY: /* File is marked read-only */
	id = XFE_ERROR_READ_ONLY;
	break;
    case ED_ERROR_BLOCKED: /* Can't write at this time, edit buffer blocked */
	id = XFE_ERROR_BLOCKED;
	break;
    case ED_ERROR_BAD_URL: /* URL was not a "file:" type or no string */
	id = XFE_ERROR_BAD_URL;
	break;
    case ED_ERROR_FILE_OPEN:
	id = XFE_ERROR_FILE_OPEN;
	break;
    case ED_ERROR_FILE_WRITE:
	id = XFE_ERROR_FILE_WRITE;
	break;
    case ED_ERROR_CREATE_BAKNAME:
    case ED_ERROR_FILE_RENAME_TO_BAK:
	id = XFE_ERROR_CREATE_BAKNAME;
	break;
    case ED_ERROR_DELETE_BAKFILE:
	id = XFE_ERROR_DELETE_BAKFILE;
	break;
    case ED_ERROR_SRC_NOT_FOUND:
	id = XFE_ERROR_SRC_NOT_FOUND;
	break;
    default:
	id = XFE_ERROR_GENERIC_ERROR_CODE; /* generic "...code = (%d)" */
	break;
    }

    msg = XP_GetString(XFE_ERROR_WRITING_FILE);
    sprintf(buf, msg, filename);
    strcat(buf, "\n\n");

    msg = XP_GetString(id);
    sprintf(&buf[strlen(buf)], msg, code);

    if (ask_question) {
	strcat(buf, "\n\n");
	strcat(buf, XP_GetString(XFE_WARNING_SAVE_CONTINUE));
	return XFE_Confirm(context, buf);
    } else {
	FE_Alert(context, buf);
	return TRUE;
    }
}

static Boolean
fe_editor_save_common(MWContext* context, char* target_url)
{
    History_entry* hist_ent;
    ED_FileError   code;
    Boolean        images;
    Boolean        links;
    Boolean        save_as;

    /* url->address is used for filename */
    hist_ent = SHIST_GetCurrent(&context->hist);

    if (!hist_ent)
	return FALSE;

    /*
     *    Has the title changed since it was last saved?
     */
    if (hist_ent->title && strcmp(hist_ent->title, hist_ent->address) == 0) {
	/*
	 *    BE will set this to the new file, if we don't zap it,
	 *    BE will retain the old name - yuck!
	 */
	XP_FREE(hist_ent->title);
	hist_ent->title = NULL;
    }

    if (target_url) { /* save as */
	save_as = TRUE;
	fe_EditorPreferencesGetLinksAndImages(context, &links, &images);
    } else { /* save me */ 
	target_url = hist_ent->address;
	save_as = FALSE;
	images = FALSE;
	links = FALSE;
    }

    /*
     *    EDT_SaveFile returns true if it completed immediately.
     *    savesas uses source and dest, save just uses 
     */
    code = EDT_SaveFile(context, hist_ent->address, target_url,
			save_as, images, links);

    /* here we spin and spin until all the savings of images
     * and stuff are done so that we get a "real" status back from
     * the backend.  This makes saving semi-synchronous, good for
     * status reporting
     */
    while(CONTEXT_DATA(context)->is_file_saving) {
        /* do nothing, we wait here */
        fe_EventLoop();
    }

    code = CONTEXT_DATA(context)->file_save_status;
    if (code != ED_ERROR_NONE && code != ED_ERROR_CANCEL) {
        fe_editor_report_file_error(context, code, 
				    CONTEXT_DATA(context)->save_file_url,
				    FALSE);
    }
    
    if (code != ED_ERROR_NONE) {
	/*
	 *    I'm sure if it is bad if the title is NULL, but
	 *    just in case..
	 */
	if ((hist_ent = SHIST_GetCurrent(&context->hist)) != NULL
	    && 
	    hist_ent->title == NULL) {
	    hist_ent->title = XP_STRDUP(hist_ent->address);
	}
	fe_editor_report_file_error(context, code, target_url, FALSE);
	return FALSE;
    }

    return TRUE;
}

Boolean
fe_EditorSaveAs(MWContext* context) /* launches dialog */
{
    char filebuf[MAXPATHLEN];

    if (fe_SaveAsDialog(context, filebuf, fe_FILE_TYPE_HTML)) {
	
	char urlbuf[MAXPATHLEN];
	    
	PR_snprintf(urlbuf, sizeof(urlbuf), "file:%.900s", filebuf);

	return fe_editor_save_common(context, urlbuf);
    }

    return FALSE;
}

Boolean
fe_EditorSave(MWContext* context)
{
    /* 
     *    Always ask the user for the filename once.
     *
     *    if (EDT_IS_NEW_DOCUMENT(context))
     */
    if (!fe_EditorDocumentIsSaved(context)) {
	return fe_EditorSaveAs(context);
    }

    return fe_editor_save_common(context, NULL);
}

CB_STATIC void
fe_editor_save_as_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;

    fe_UserActivity (context);
    
    {
	MWContext *ctx = fe_GetFocusGridOfContext(context);
	if (ctx)
	    context = ctx;
    }

    fe_EditorSaveAs(context);

    fe_EditorUpdateToolbar(context, 0);
}

CB_STATIC void
fe_editor_save_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;

    fe_UserActivity (context);
    
    {
	MWContext *ctx = fe_GetFocusGridOfContext(context);
	if (ctx)
	    context = ctx;
    }

    fe_EditorSave(context);

    fe_EditorUpdateToolbar(context, 0);
}

#if 0
static void
fe_editor_browse_doc_update_cb(Widget widget, XtPointer closure, XtPointer cd)
{
    Boolean sensitive = fe_EditorDocumentIsSaved((MWContext*)closure);

    fe_WidgetSetSensitive(widget, sensitive);
}

static void
fe_editor_publish_update_cb(Widget widget, XtPointer closure, XtPointer cd)
{
    Boolean sensitive = fe_EditorDocumentIsSaved((MWContext*)closure);

    fe_WidgetSetSensitive(widget, sensitive);
}

CB_STATIC void
fe_editor_file_menu_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context =  (MWContext*)closure;
    Arg        args[4];
    Cardinal   n;
    Widget     pulldown_menu;
    Widget*    children;
    Cardinal   nchildren;
    char*      name;
    Widget     button;
    int        i;

    /*
     *    Get the popup menu from the cascade.
     */
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, &pulldown_menu); n++;
    XtGetValues(widget, args, n);

    /*
     *    Get the children of the popup.
     */
    n = 0;
    XtSetArg(args[n], XmNchildren, &children); n++;
    XtSetArg(args[n], XmNnumChildren, &nchildren); n++;
    XtGetValues(pulldown_menu, args, n);

    for (i = 0; i < nchildren; i++) {

	button = children[i];
	name = XtName(button);
	
	if (strcmp(name, "browseDoc") == 0)
	    fe_editor_browse_doc_update_cb(button, (XtPointer)context, NULL);
	else if (strcmp(name, "publish") == 0)
	    fe_editor_publish_update_cb(button, (XtPointer)context, NULL);
    }
}
#endif

/*
 *    Editor dialogs.
 */
typedef struct SaveRemoteInfo
{
  Widget  widget;
  Boolean adjust_links;
  Boolean save_images;
  Boolean do_save;
  Boolean done;
} SaveRemoteInfo;

static void
fe_save_remote_save_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  SaveRemoteInfo* info = (SaveRemoteInfo*)closure;

  info->do_save = TRUE;
  info->done = TRUE;

  XtUnmanageChild(info->widget);
}

static void
fe_save_remote_cancel_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  SaveRemoteInfo* info = (SaveRemoteInfo*)closure;

  info->do_save = FALSE;
  info->done = TRUE;

  XtUnmanageChild(info->widget);
}

static void
fe_save_remote_images_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  SaveRemoteInfo* info = (SaveRemoteInfo*)closure;

  info->save_images = XmToggleButtonGadgetGetState(widget);
}

static void
fe_save_remote_links_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  SaveRemoteInfo* info = (SaveRemoteInfo*)closure;

  info->adjust_links = XmToggleButtonGadgetGetState(widget);
}


static Widget
fe_CreateSaveRemoteDialog(MWContext* context, SaveRemoteInfo* info)
{
  Arg      av[20];
  Cardinal ac;
  Widget   form;
  Widget   info_icon;
  Widget   info_text;
  Widget   links_frame;
  Widget   links_toggle;
  Widget   images_frame;
  Widget   images_toggle;
  Widget   buttons_column;
  Widget   save_button;
  Widget   cancel_button;
  Widget   children[16];
  Cardinal nchildren = 0;
  Pixmap   pixmap;
  Pixel    fg;
  Pixel    bg;
  Cardinal depth;
  Screen*  screen;

  form = fe_CreateFormDialog(context, "saveRemote");

  /*
   *   Info icon, info text,
   *   framed Links check box and text,
   *   framed Images check box and text,
   *   --------------------------------
   *   Save, Cancel, Help.
   */
#if 0
  pixmap = fe_ToolbarPixmap(context, 0, False, False); /*FIXME*/
#else
  ac = 0;
  XtSetArg(av[ac], XmNbackground, &bg); ac++;
  XtSetArg(av[ac], XmNforeground, &fg); ac++;
  XtSetArg(av[ac], XmNdepth, &depth); ac++;
  XtSetArg(av[ac], XmNscreen, &screen); ac++;
  XtGetValues(form, av, ac);

  /*
   *    This is so broken. Init MesageBox class, so the Motif
   *    standard icons get installed. YUCK..djw
   */
  XtInitializeWidgetClass(xmMessageBoxWidgetClass);

  pixmap = XmGetPixmapByDepth(screen, "xm_question", fg, bg, depth);
  if (pixmap == XmUNSPECIFIED_PIXMAP)
    pixmap = XmGetPixmapByDepth(screen, "default_xm_question", fg, bg, depth);
#endif
  ac = 0;
  XtSetArg(av[ac], XmNlabelType, XmPIXMAP); ac++;
  XtSetArg(av[ac], XmNlabelPixmap, pixmap); ac++;
  info_icon = XmCreateLabelGadget(form, "infoIcon", av, ac);
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  children[nchildren++] = info_icon;

  ac = 0;
  XtSetArg(av[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg(av[ac], XmNlabelType, XmSTRING); ac++;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(av[ac], XmNleftWidget, info_icon); ac++;
  info_text = XmCreateLabelGadget(form, "infoText", av, ac);
  children[nchildren++] = info_text;

  ac = 0;
  XtSetArg(av[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(av[ac], XmNleftWidget, info_text); ac++;
  XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  buttons_column = fe_CreateSaveCancelButtonGroup(
						  context,
						  form,
						  "rowColumn",
						  av,
						  ac,
						  &save_button,
						  &cancel_button
						  );
  children[nchildren++] = buttons_column;

  XtAddCallback(save_button, XmNactivateCallback, fe_save_remote_save_cb, info);
  XtAddCallback(cancel_button, XmNactivateCallback, fe_save_remote_cancel_cb, info);

  XtVaSetValues(form, XmNdefaultButton, save_button, 0);

  /*
   *    Links group.
   */
  links_frame = fe_CreateFramedToggle(
				       context,
				       form,
				       "links",
				       &links_toggle
				       );
  children[nchildren++] = links_frame;

  ac = 0;
  XtSetArg(av[ac], XmNset, ((info->adjust_links)? TRUE: FALSE)); ac++;
  XtSetValues(links_toggle, av, ac);
  XtAddCallback(links_toggle, XmNvalueChangedCallback, fe_save_remote_links_cb, info);

  ac = 0;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(av[ac], XmNtopWidget, buttons_column); ac++;
  XtSetValues(links_frame, av, ac);

  /*
   *    Images group.
   */
  images_frame = fe_CreateFramedToggle(
					context,
					form,
					"images",
					&images_toggle
				       );

  children[nchildren++] = images_frame;

  ac = 0;
  XtSetArg(av[ac], XmNset, ((info->save_images)? TRUE: FALSE)); ac++;
  XtSetValues(images_toggle, av, ac);
  XtAddCallback(images_toggle, XmNvalueChangedCallback, fe_save_remote_images_cb, info);

  ac = 0;
  XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg(av[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(av[ac], XmNtopWidget, links_frame); ac++;
  XtSetArg(av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetValues(images_frame, av, ac);

  XtManageChildren(children, nchildren);

  return form;
}

static void
fe_DoSaveRemoteDialog(
		      MWContext* context,
		      Boolean* save_rv, Boolean* links_rv, Boolean* images_rv
)
{
  SaveRemoteInfo info;

  info.save_images = *images_rv;;
  info.adjust_links = *links_rv;
  info.do_save = TRUE;
  info.done = FALSE;

  info.widget = fe_CreateSaveRemoteDialog(context, &info);
    
  XtManageChild(info.widget);

  fe_NukeBackingStore(info.widget);
  
  while (!info.done)
    fe_EventLoop();

  XtDestroyWidget(info.widget);

  *save_rv = info.do_save;
  *links_rv = info.adjust_links;
  *images_rv = info.save_images;
}

/*
 *    I18N input method support.
 */
#ifdef XFE_EDITOR_IM_SUPPORT
static void
fe_editor_im_deregister(Widget widget, XtPointer closure, XtPointer cb_data)
{
    XmImUnregister(widget);
}

static void
fe_editor_im_get_coords(MWContext* context, XPoint* point)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;
    
    if (data->running == FALSE && data->showing == FALSE) {
	point->x = 0;
	point->y = 16;
    } else {
	point->x = data->x + data->width - CONTEXT_DATA(context)->document_x;
	point->y = data->y - 2 - CONTEXT_DATA(context)->document_y +
			data->height;
    }
}

static void
fe_editor_im_init(MWContext* context)
{
    Widget     widget = CONTEXT_DATA(context)->drawing_area;
    Arg        args[8];
    Cardinal   n;
    XmFontList font_list;
    Pixel      bg_pixel;
    Pixel      fg_pixel;
    /* Pixmap     bg_pixmap; */
    XPoint     xmim_point;
    /* int        xmim_height; */
    
    XtVaSetValues(CONTEXT_WIDGET(context), XmNallowShellResize, TRUE, NULL);

    XmImRegister(widget, 0);
    XtAddCallback(widget, XmNdestroyCallback, fe_editor_im_deregister, NULL);

    /*
     *    Should these change dynamically with point?
     */
    font_list = fe_GetFont(context, 3, LO_FONT_FIXED);
    bg_pixel = CONTEXT_DATA(context)->bg_pixel;
    fg_pixel = CONTEXT_DATA(context)->fg_pixel;
    /* bg_pixmap = CONTEXT_DATA(context)->backdrop_pixmap; */

    fe_editor_im_get_coords(context, &xmim_point);

    n = 0;
    XtSetArg(args[n], XmNfontList, font_list); n++;
    XtSetArg(args[n], XmNbackground, bg_pixel); n++;
    XtSetArg(args[n], XmNforeground, fg_pixel); n++;
    /* XtSetArg(args[n], XmNbackgroundPixmap, bg_pixmap); n++; */
    XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
    /* XtSetArg(args[n], XmNlineSpace, xmim_height); n++; */
    XmImSetValues(widget, args, n);
}

static void
fe_editor_im_set_coords(MWContext* context)
{
    Widget   widget = CONTEXT_DATA(context)->drawing_area;
    Arg      args[8];
    Cardinal n;
    XPoint   xmim_point;
    /* int      xmim_height; */

    fe_editor_im_get_coords(context, &xmim_point);

    n = 0;
    XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
    /* XtSetArg(args[n], XmNlineSpace, xmim_height); n++; */
    XmImSetFocusValues(widget, args, n);
}

static void
fe_editor_im_focus_in(MWContext* context)
{
    fe_editor_im_set_coords(context);

    XtSetKeyboardFocus(CONTEXT_WIDGET(context),
			CONTEXT_DATA(context)->drawing_area);
}

static void
fe_editor_im_focus_out(MWContext* context)
{
    Widget widget = CONTEXT_DATA(context)->drawing_area;

    XmImUnsetFocus(widget);
}
#endif /*XFE_EDITOR_IM_SUPPORT*/

static void fe_caret_pause(MWContext* context);
static void fe_caret_unpause(MWContext* context);

static void
fe_losing_focus_eh(Widget w, XtPointer closure, XEvent* event, Boolean* cont)
{
    MWContext* context = (MWContext *)closure;

    *cont = TRUE;
    
    if (event->type == FocusIn) {
        fe_caret_unpause(context);
#ifdef XFE_EDITOR_IM_SUPPORT
	fe_editor_im_focus_in(context);
#endif /*XFE_EDITOR_IM_SUPPORT*/
    } else if (event->type == FocusOut) {
        fe_caret_pause(context);
#ifdef XFE_EDITOR_IM_SUPPORT
	fe_editor_im_focus_out(context);
#endif /*XFE_EDITOR_IM_SUPPORT*/
    }
}

/*
 *    Menu stuff.
 */
typedef MWContext MWEditorContext; /* place holder */

MWContext*
fe_EditorNew(MWContext* context, char* address)
{
    MWContext*  new_context;
    URL_Struct* url;
    Boolean     is_editor_save;

    fe_UserActivity(context);

#if 0
    /*
     *    It would be really nice to test for remote urls right here,
     *    but we don't always know (it may be an alias). So we have
     *    to load the doc first, the check in FE_EditorDocumentLoaded().
     *    Plus we need to pass links & images through to
     *    FE_EditorDocumentLoaded() - choke....djw
     */
    if (address != NULL	&& !NET_IsLocalFileURL(address)) {
	/* not a local url */

	fe_EditorPreferencesGetLinksAndImages(context, &links, &images);

	/*
	 *    Prompt for saving remote document.
	 */
	fe_DoSaveRemoteDialog(context, &save, &links, &images);

	if (!save)
	    return NULL; /* we got cancelled */
    }
#endif

    /*
     *    Make a new editor window. All this stuff with ->is_editor
     *    is to fake out the callees that want to see that set.
     */
    is_editor_save = context->is_editor;
    context->is_editor = TRUE;

    new_context = fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)),
				context,
				0,
				NULL,
				MWContextEditor,
				FALSE
				);
    context->is_editor = is_editor_save;
    new_context->is_editor = TRUE;

#if 0
    /*
     *    Just cannot get this to do what it is supposed to.
     */
    XtAddEventHandler(CONTEXT_WIDGET(new_context), PropertyChangeMask,
		      FALSE, (XtEventHandler)fe_property_notify_eh,
		      new_context);
#endif

    XtAddEventHandler(CONTEXT_WIDGET(new_context), FocusChangeMask,
		      FALSE, (XtEventHandler)fe_losing_focus_eh,
		      new_context);

    /*
     *    Register with input server.
     */
#ifdef XFE_EDITOR_IM_SUPPORT
    fe_editor_im_init(new_context);
#endif /*XFE_EDITOR_IM_SUPPORT*/
    
    if (!address)
	address = "about:editfilenew"; /* blank address */

    url = NET_CreateURLStruct(address, FALSE);

    fe_GetURL(new_context, url, FALSE);

    return new_context;
}

static MWContext*
fe_editor_find_context(char* address, int type)
{
    struct fe_MWContext_cons* rest;
    MWContext* context;
    History_entry* hist_entry;

    for (rest = fe_all_MWContexts; rest; rest = rest->next) {
        context = rest->context;
	if (context->type == type) {
	    hist_entry = SHIST_GetCurrent(&context->hist);

	    if (hist_entry != NULL
		&&
		hist_entry->address
		&&
		strcmp(hist_entry->address, address) == 0) {
	        return context;
	    }
	}
    }
    return NULL;
}

static void
fe_map_raise_window(MWContext* context)
{
    Widget widget = CONTEXT_WIDGET(context);
    Display* display = XtDisplay(widget);
    Window window = XtWindow(widget);

#if 1
    XMapWindow(display, window);
    XRaiseWindow(display, window);
#else
    XMapRaised(XtDisplay(CONTEXT_WIDGET(context)),
	       XtWindow(CONTEXT_WIDGET(context)));
#endif
}

static void
fe_map_raise_timeout(XtPointer closure, XtIntervalId* id)
{
    MWContext *context = (MWContext *)closure;

    fe_map_raise_window(context);
}

static void
fe_map_raise_window_when_idle(MWContext* context)
{
    /*
     *    Delay the raise until after any dialogs, etc have
     *    been unmanaged. Otherwise we bounce backwards and forwards,
     *    and sometimes the window is left unraised.
     */
    XtAppAddTimeOut(fe_XtAppContext, 0, fe_map_raise_timeout, (XtPointer)context);
}

MWContext*
fe_EditorGetURL(MWContext* context, char* address)
{
    MWContext* new_context = fe_editor_find_context(address, MWContextEditor);

    if (new_context) {
	fe_map_raise_window_when_idle(new_context);
	return new_context;
    } else {
	return fe_EditorNew(context, address);
    }
}

MWContext*
fe_BrowserGetURL(MWContext* context, char* address)
{
    return fe_GetURLInBrowser(context, NET_CreateURLStruct(address, FALSE));
}

CB_STATIC void
fe_editor_edit_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;
    History_entry* hist_ent;
    URL_Struct*    url;

    fe_UserActivity(context);

    hist_ent = SHIST_GetCurrent(&context->hist);

    if (hist_ent != NULL) {

	/* If this is a grid doc, bail ... */
	if (context->is_grid_cell || fe_IsGridParent (context)) {
	    FE_Alert(context, XP_GetString(XFE_EDITOR_ALERT_FRAME_DOCUMENT));
	    return;
	}

	if (strncmp(hist_ent->address, "about:", 6) == 0) {
	    FE_Alert(context, XP_GetString(XFE_EDITOR_ALERT_ABOUT_DOCUMENT));
	    return;
	}

	url = SHIST_CreateURLStructFromHistoryEntry(context, hist_ent);

	/* url->address is used for filename */
	if (url != NULL && hist_ent != NULL && hist_ent->address) {
	    fe_EditorGetURL(context, hist_ent->address);
	    return;
	}
    }

    /* fall thru for no url loaded */
    FE_Alert(context, fe_globalData.no_url_loaded_message);
}

CB_STATIC void
fe_editor_new_from_template_cb(Widget widget, XtPointer closure,
			       XtPointer call_data)
{
    MWContext* context = (MWContext *) closure;
    char*      template_url = fe_EditorPreferencesGetTemplate(context);
 
    if (template_url == NULL || template_url[0] == '\0') {
	char* msg = XP_GetString(XFE_EDITOR_DOCUMENT_TEMPLATE_EMPTY);

	/* The new document template location is not set. */
      	if (XFE_Confirm(context, msg)) {
	    fe_EditorPreferencesDialogDo(context,
				 XFE_EDITOR_DOCUMENT_PROPERTIES_GENERAL);
	}
	return;
    }

    fe_BrowserGetURL(context, template_url);
}

CB_STATIC void
fe_editor_new_from_wizard_cb(Widget widget, XtPointer closure,
			       XtPointer call_data)
{
    MWContext* context = (MWContext *) closure;
    char*      template_url = XFE_WIZARD_TEMPLATE_URL;

    fe_BrowserGetURL(context, template_url);
}

CB_STATIC void
fe_editor_new_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *) closure;

    fe_EditorNew(context, NULL);
}

CB_STATIC void
fe_editor_open_url_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *) closure;

    fe_EditorOpenURLDialog(context);
}

void
fe_EditorDisplaySource(MWContext* context)
{
    EDT_DisplaySource(context);
}

CB_STATIC void
fe_editor_view_source_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *) closure;

    fe_EditorDisplaySource(context);
}

void
fe_EditorCleanup(MWContext* context)
{
    Boolean  as_enabled;
    unsigned as_time;

    /*
     *    If they have autosave on, try to do a save. Don't do it
     *    for a new doc, as that'll mean dialogs, and .....
     */
    if (!EDT_IS_NEW_DOCUMENT(context) &&
	(!EDT_IsBlocked(context) && EDT_DirtyFlag(context))) {
	fe_EditorPreferencesGetAutoSave(context, &as_enabled, &as_time);
	if (as_enabled)
	    fe_EditorSave(context);
    }
    
    FE_DestroyCaret(context);
    EDT_DestroyEditBuffer(context);
}

void
fe_EditorDelete(MWContext* context)
{
    /*
     *    We've already had this discussion....
     */
    EDT_SetDirtyFlag(context, FALSE);
    
    /*
     *    This will implicitly call fe_EditorCleanup().
     *    See fe_delete_cb() in xfe.c
     */
    XtDestroyWidget(CONTEXT_WIDGET(context));
}

char*
fe_EditorMakeName(MWContext* context, char* buf, unsigned maxsize)
{
    History_entry* entry;
    char*          local_name = NULL;

    if (!EDT_IS_NEW_DOCUMENT(context)
	&&
	(entry = SHIST_GetCurrent(&context->hist)) && (entry->address)) {

	local_name = NULL;
	if (XP_ConvertUrlToLocalFile(entry->address, &local_name)) {
	    FE_CondenseURL(buf, local_name, maxsize);
	    XP_FREE(local_name);
	} else {
	    FE_CondenseURL(buf, entry->address, maxsize);
	}
    } else {
	strcpy(buf, XFE_EDITOR_NEW_DOC_NAME);
    }
    return buf;
}

static Boolean
fe_named_question(MWContext* context, char* name, char* message)
{
    return (Boolean)((int)fe_dialog(CONTEXT_WIDGET (context),
				    name, message,
				    TRUE, 0, TRUE, FALSE, 0));
}

Boolean
fe_save_file_check(MWContext* context, Boolean file_must_exist, Boolean autoSaveNew)
{
    int rv;
    char filename[MAXPATHLEN];
    char buf[MAXPATHLEN];
    char dialogmessages[MAXPATHLEN+64];
    Boolean ok_cancel = (file_must_exist && EDT_IS_NEW_DOCUMENT(context));

    if (ok_cancel || EDT_DirtyFlag(context)) {
	fe_EditorMakeName(context, filename, sizeof(filename));
	
	if (ok_cancel) {
	    /* You must save <filename> as a local file */
	    sprintf(buf, XP_GetString(XFE_ALERT_SAVE_CHANGES), filename);
	    rv = (fe_named_question(context, "saveNewFile", buf))?
		XmDIALOG_OK_BUTTON: XmDIALOG_CANCEL_BUTTON;
	} else {
	    /* Save changes to <filename>? */
	    sprintf(buf, XP_GetString(XFE_WARNING_SAVE_CHANGES), filename);
	    if (autoSaveNew) {
            sprintf(dialogmessages,"%s\n\n%s", buf,
                    XP_GetString(XFE_WARNING_AUTO_SAVE_NEW_MSG));
			rv = fe_YesNoCancelDialog(context, "autoSaveNew", dialogmessages);
		} else
			rv = fe_YesNoCancelDialog(context, "saveFile", buf);
	}
	
	if (rv == XmDIALOG_OK_BUTTON)
	    return fe_EditorSave(context);
	else if (rv == XmDIALOG_CANCEL_BUTTON)
	    return FALSE;
    }
    return TRUE;
}

XP_Bool
FE_CheckAndSaveDocument(MWContext* context)
{
    return fe_save_file_check(context, TRUE, FALSE);
}

XP_Bool 
FE_CheckAndAutoSaveDocument(MWContext *context)
{
    return fe_save_file_check(context, FALSE, TRUE);
}



#if 0
/*
 *    Call this to check if we need to force saving file
 *    Conditions are New, unsaved document and when editing a remote file
 *    Returns TRUE for all cases except CANCEL by the user in any dialog
 */
XP_Bool
FE_SaveNonLocalDocument(MWContext* context, XP_Bool save_new_document) 
{
    History_entry* hist_entry;
    int type;
    Boolean links;
    Boolean images;
    Boolean save;
    char filebuf[MAXPATHLEN];
    char urlbuf[MAXPATHLEN];
    ED_FileError file_error;

    if (context == NULL || !EDT_IS_EDITOR(context)) {
        return TRUE;
    }
    
    hist_entry = SHIST_GetCurrent(&(context->hist));
    if (!hist_entry || !hist_entry->address)
        return TRUE;
    
    /*
     */
    type = NET_URL_Type(hist_entry->address);

    if ((type >  0 && type != FILE_TYPE_URL && type != FILE_CACHE_TYPE_URL &&
         type != MAILBOX_TYPE_URL && type != VIEW_SOURCE_TYPE_URL)
	||
	(save_new_document && EDT_IS_NEW_DOCUMENT(context)))
    {
	fe_EditorPreferencesGetLinksAndImages(context, &links, &images);
	fe_DoSaveRemoteDialog(context, &save, &links, &images);

	if (!save)
	    return FALSE;

#if 0
	if (!EDT_IS_NEW_DOCUMENT(context))
	    fe_EditorCopyrightWarningDialogDo();
#endif
	
	if (!fe_SaveAsDialog(context, filebuf, fe_FILE_TYPE_HTML))
	    return FALSE;

	/*
	 *    Grab the filename while we have it to form file: path.
	 */
	PR_snprintf(urlbuf, sizeof(urlbuf), "file:%.900s", filebuf);

	/* the c++ comments are here deliberately so that later
	 * on if we use this code we won't forget to modify it */

	/* this here is all disabled, we don't use it anymore but if later
	 * on we need to use it again, we should probably make it synchronous
	 */

	file_error = EDT_SaveFile(context,
				  hist_entry->address,
				  urlbuf,
				  TRUE,
				  images,
				  links);
	
    	if (file_error != ED_ERROR_NONE) {
	    fe_editor_report_file_error(context, file_error, urlbuf,
					FALSE);
	    return FALSE;
	}
    }
    return TRUE;
}
#endif

CB_STATIC void
fe_editor_delete_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *)closure;

    fe_UserActivity (context);

    if (fe_WindowCount == 1) {
#if 0
	fe_Bell(context);
#else
	fe_QuitCallback(widget, closure, call_data);
#endif
    }  else {
	if (fe_save_file_check(context, FALSE, FALSE))
	    fe_EditorDelete(context);
    }
}

void fe_editor_delete_response(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_editor_delete_cb(widget, closure, call_data);
}


Boolean
fe_EditorCheckUnsaved(MWContext* context)
{
    struct fe_MWContext_cons* rest;

    for (rest = fe_all_MWContexts; rest; rest = rest->next) {
	MWContext* context = rest->context;
	if (context->type == MWContextEditor) {
	    if (!fe_save_file_check(context, FALSE, FALSE)) {
		return FALSE;
	    }
	}
    }
    return TRUE;
}

/* this func is basically XP_FindContextOfType but does not return
 * view source type */

static MWContext*
fe_FindContextOfTypeNoViewSource(MWContext *context, MWContextType type)
{
	XP_List *context_list = XP_GetGlobalContextList();
	int i;

	/* The other types aren't "real" - they don't have windows.  (Actually,
   	 * neither do all of these, but it's damned useful to be able to find the
   	 * biff context...) */
	XP_ASSERT (type == MWContextBrowser ||
   			   type == MWContextMail ||
   			   type == MWContextNews ||
   			   type == MWContextMessageComposition ||
   			   type == MWContextBookmarks ||
   			   type == MWContextAddressBook ||
   			   type == MWContextBiff);

	/* If our current context has the right type, go there */
	if (context && context->type == type) {
		if (!context->name || strncmp(context->name,"view-source",11)!=0)
			return context;
	}

	/* otherwise, just get any other context */
	for (i=1; i<= XP_ListCount(context_list); i++) {
		MWContext * compContext = 
			(MWContext *)XP_ListGetObjectNum(context_list, i);
		if (compContext->type == type) {
			if (!compContext->name || 
				strncmp(compContext->name,"view-source",11)!=0)
				return compContext;
		}
	}
	return NULL;
}


MWContext*
fe_GetURLInBrowser(MWContext* context, URL_Struct *url)
{
    MWContext* new_context;
    MWContext* top_context;

	new_context = fe_FindContextOfTypeNoViewSource(context, MWContextBrowser);
 
    if (new_context) {
	top_context = fe_GridToTopContext(new_context);
	if (!top_context)
	    top_context = new_context;

	fe_GetURL(top_context, url, FALSE);
    } else {
	new_context = fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)),
				    context, url, NULL,
				    MWContextBrowser, FALSE);
    }

    if (new_context) {
	XMapRaised(XtDisplay(CONTEXT_WIDGET(new_context)),
		   XtWindow(CONTEXT_WIDGET(new_context)));
    }
    return new_context;
}


CB_STATIC void
fe_editor_browse_doc_cb(Widget widget, XtPointer closure, XtPointer cd)
{
    MWContext *context = (MWContext *)closure;
    URL_Struct *url;

    fe_UserActivity(context);

    if (!FE_CheckAndSaveDocument(context))
	return;

    url = SHIST_CreateURLStructFromHistoryEntry(context,
					SHIST_GetCurrent(&context->hist));
    fe_GetURLInBrowser(context, url);
}

CB_STATIC void
fe_editor_about_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;
    fe_UserActivity (context);
    fe_BrowserGetURL(context, "about:");
}
 
/*
 *    Crock timeout callback that is used by FE_EditorDocumentLoaded() to
 *    cancel an edit. See comments there.
 */
static void
fe_editor_exit_timeout(XtPointer closure, XtIntervalId* id)
{
    MWContext *context = (MWContext *)closure;

    fe_EditorDelete(context);
}

void
fe_EditorOpenFile(MWContext* context)
{
    char*      title;
    char*      out;

    title = XP_GetString(XFE_FILE_OPEN);
    out = fe_ReadFileName(context, title, 0, True, 0);

    if (out) {
      
	char buf[MAXPATHLEN];

	PR_snprintf(buf, sizeof(buf), "file:%.900s", out); 

	free(out);

	/*
	 *    Raise existing, or create a new window.
	 */
	fe_EditorGetURL(context, buf);
    }
}

CB_STATIC void
fe_editor_open_file_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *) closure;

    fe_UserActivity(context);

    fe_EditorOpenFile(context);
}

void
fe_EditorReload(MWContext* context, Boolean super_reload)
{
    if (EDT_IS_NEW_DOCUMENT(context))
	return; /* only from action */

    if (!FE_CheckAndSaveDocument(context))
	return;

#if 0 /* this does not exist! */
    EDT_Reload(context);
#else
    if (super_reload)
	fe_ReLayout (context, NET_SUPER_RELOAD);
    else
	fe_ReLayout (context, NET_NORMAL_RELOAD);
#endif
}

CB_STATIC void
fe_editor_reload_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    XmPushButtonCallbackStruct* cd = (XmPushButtonCallbackStruct *)call_data;

    fe_UserActivity(context);
    
    fe_EditorReload(context, (cd && cd->event->xkey.state & ShiftMask));
}

static void
fe_editor_reload_update_cb(Widget widget,
			   XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean sensitive = !EDT_IS_NEW_DOCUMENT(context);
    
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_cut_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    XmPushButtonCallbackStruct* info = (XmPushButtonCallbackStruct*)call_data;

    fe_EditorCut(context, info->event->xbutton.time);
}

static void
fe_editor_cut_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean sensitive = FALSE;

    if (fe_EditorCanCut(context))
	sensitive = TRUE;

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_copy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    XmPushButtonCallbackStruct* info = (XmPushButtonCallbackStruct*)call_data;

    fe_EditorCopy(context, info->event->xbutton.time);
}

static void
fe_editor_copy_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean sensitive = FALSE;

    if (fe_EditorCanCopy(context))
	sensitive = TRUE;

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_paste_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    XmPushButtonCallbackStruct* info = (XmPushButtonCallbackStruct*)call_data;

    fe_EditorPaste(context, info->event->xbutton.time);
}

static void
fe_editor_paste_update_cb(Widget widget,
			  XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean sensitive = FALSE;

    if (fe_EditorCanPaste(context))
	sensitive = TRUE;

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_undo_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorUndo(context);
}

static void
fe_editor_undo_update_cb(Widget widget,
			  XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean    sensitive = FALSE;
    XmString   string;
    Arg        args[8];
    Cardinal   n;

    if (fe_EditorCanUndo(context, &string))
	sensitive = TRUE;

    n = 0;
    XtSetArg(args[n], XtNsensitive, sensitive); n++;
    if (string) {
	XtSetArg(args[n], XmNlabelType, XmSTRING); n++;
	XtSetArg(args[n], XmNlabelString, string); n++;
    }
    XtSetValues(widget, args, n);
}

CB_STATIC void
fe_editor_redo_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorRedo(context);
}

static void
fe_editor_redo_update_cb(Widget widget,
			  XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean    sensitive = FALSE;
    XmString   string;
    Arg        args[8];
    Cardinal   n;

    if (fe_EditorCanRedo(context, &string))
	sensitive = TRUE;

    n = 0;
    XtSetArg(args[n], XtNsensitive, sensitive); n++;
    if (string) {
	XtSetArg(args[n], XmNlabelType, XmSTRING); n++;
	XtSetArg(args[n], XmNlabelString, string); n++;
    }
    XtSetValues(widget, args, n);
}

void
fe_EditorFind(MWContext* context)
{
    MWContext* top_context = fe_GridToTopContext(context);
    
    if (!top_context)
	top_context = context;

    fe_UserActivity(top_context);

    fe_FindDialog(top_context, False);
}

static Boolean
fe_EditorCanFindAgain(MWContext* context)
{
    fe_FindData* find_data = CONTEXT_DATA(context)->find_data;
    
    return ((find_data != NULL)
	    &&
	    (find_data->string != NULL)
	    &&
	    (find_data->string[0] != '\0'));
}

void
fe_EditorFindAgain(MWContext* context)
{
    MWContext*   top_context = fe_GridToTopContext(context);
    fe_FindData* find_data;

    if (!top_context)
	top_context = context;

    fe_UserActivity(top_context);

    find_data = CONTEXT_DATA(top_context)->find_data;

  if (fe_EditorCanFindAgain(top_context))
      fe_FindDialog(top_context, TRUE);
  else
      fe_Bell(context);
}

CB_STATIC void
fe_editor_find_cb(Widget widget,
		  XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;

    fe_EditorFind(context);
}

CB_STATIC void
fe_editor_find_again_cb(Widget widget,
			XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;

    fe_EditorFindAgain(context);
}

static void
fe_editor_find_again_update_cb(Widget widget,
			       XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;
    Boolean      sensitive = fe_EditorCanFindAgain(context);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_select_all_cb(Widget widget,
			XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;

    fe_EditorSelectAll(context);
}

CB_STATIC void
fe_editor_delete_item_cb(Widget widget,
			XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;

    fe_EditorDeleteItem(context);
}

CB_STATIC void
fe_editor_select_table_cb(Widget widget,
			XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;

    fe_EditorSelectTable(context);
}

static void
fe_editor_select_table_update_cb(Widget widget,
			       XtPointer closure, XtPointer call_data)
{
    MWContext*   context = (MWContext *)closure;
    Boolean      sensitive = fe_EditorTableCanDelete(context);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_remove_links_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    fe_EditorRemoveLinks((MWContext*)closure);
}

static void
fe_editor_remove_links_update_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    MWContext*     context = (MWContext*)closure;
    ED_ElementType type = EDT_GetCurrentElementType(context);
    Boolean        sensitive = FALSE;

    if ((type == ED_ELEMENT_TEXT ||
	 type == ED_ELEMENT_SELECTION ||
	 type == ED_ELEMENT_IMAGE)
	&&
	EDT_CanSetHREF(context)) {
	sensitive = TRUE;
    }

    XtVaSetValues(widget, XtNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_edit_menu_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext*         context =  (MWContext*)closure;

    Arg        args[4];
    Cardinal   n;
    Widget     pulldown_menu;
    Widget*    children;
    Cardinal   nchildren;
    char*      name;
    Widget     button;
    int        i;

    /*
     *    Get the popup menu from the cascade.
     */
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, &pulldown_menu); n++;
    XtGetValues(widget, args, n);

    /*
     *    Get the children of the popup.
     */
    n = 0;
    XtSetArg(args[n], XmNchildren, &children); n++;
    XtSetArg(args[n], XmNnumChildren, &nchildren); n++;
    XtGetValues(pulldown_menu, args, n);

    for (i = 0; i < nchildren; i++) {

	button = children[i];
	name = XtName(button);
	
	if (strcmp(name, "undo") == 0)
	    fe_editor_undo_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "redo") == 0)
	    fe_editor_redo_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "cut") == 0) 
	    fe_editor_cut_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "copy") == 0) 
	    fe_editor_copy_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "paste") == 0) 
	    fe_editor_paste_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "findAgain") == 0)
	    fe_editor_find_again_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "selectTable") == 0)
	    fe_editor_select_table_update_cb(button, (XtPointer)context, NULL);

	else if (strcmp(name, "removeLinks") == 0)
	    fe_editor_remove_links_update_cb(button, (XtPointer)context, NULL);
    }
}

CB_STATIC void
fe_editor_insert_link_cb(Widget widget, XtPointer closure,
			 XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    if (EDT_IS_NEW_DOCUMENT(context)) {
	if (!FE_CheckAndSaveDocument(context))
	    return;
    }

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_LINK_INSERT);
}

CB_STATIC void
fe_editor_insert_target_cb(Widget widget, XtPointer closure,
			   XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorTargetPropertiesDialogDo(context);
}

CB_STATIC void
fe_editor_insert_image_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    if (EDT_IS_NEW_DOCUMENT(context)) {
	if (!FE_CheckAndSaveDocument(context))
	    return;
    }

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_IMAGE_INSERT);
}

CB_STATIC void
fe_editor_insert_hrule_dialog_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorHorizontalRulePropertiesDialogDo(context);
}

CB_STATIC void
fe_editor_insert_html_cb(Widget widget, XtPointer closure,
			 XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorHtmlPropertiesDialogDo(context);
}

CB_STATIC void
fe_editor_insert_line_break_cb(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    XtPointer* userdata;

    XtVaGetValues(widget, XmNuserData, &userdata, 0);
    
    fe_EditorLineBreak(context, (ED_BreakType)userdata);
}

CB_STATIC void
fe_editor_insert_line_break_update_cb(Widget widget,
				      XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorLineBreakCanInsert(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_insert_non_breaking_space_cb(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorNonBreakingSpace(context);    
}

static XtPointer
fe_insert_menu_mappee(Widget widget, XtPointer data)
{
    MWContext* context = (MWContext *)data;
    char*      name = XtName(widget);
    
    if (strcmp(name, "insertNewLineBreak") == 0)
	fe_editor_insert_line_break_update_cb(widget, context, NULL);
    else if (strcmp(name, "insertBreakBelow") == 0)
	fe_editor_insert_line_break_update_cb(widget, context, NULL);

    return 0;
}

CB_STATIC void
fe_editor_insert_menu_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    Widget menu;

    XtVaGetValues(widget, XmNsubMenuId, &menu, 0);

    fe_WidgetTreeWalk(menu, fe_insert_menu_mappee, closure);
}

CB_STATIC void
fe_editor_display_paragraph_marks_update_cb(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean    set = fe_EditorParagraphMarksGetState(context);

    XmToggleButtonGadgetSetState(widget, set, FALSE);
}

CB_STATIC void
fe_editor_display_tables_cb(Widget widget, XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    set = fe_EditorDisplayTablesGet(context);
    fe_EditorDisplayTablesSet(context, !set);
}

CB_STATIC void
fe_editor_display_tables_update_cb(Widget widget,
				   XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    set = fe_EditorDisplayTablesGet(context);

    XmToggleButtonGadgetSetState(widget, set, FALSE);
}

static XtPointer
fe_view_menu_mappee(Widget widget, XtPointer data)
{
    MWContext* context = (MWContext *)data;
    char*      name = XtName(widget);
    
    if (strcmp(name, "paragraphMarks") == 0)
	fe_editor_display_paragraph_marks_update_cb(widget, context, NULL);
    else if (strcmp(name, "displayTables") == 0)
	fe_editor_display_tables_update_cb(widget, context, NULL);
    else if (strcmp(name, "reload") == 0)
	fe_editor_reload_update_cb(widget, context, NULL);

    return 0;
}

CB_STATIC void
fe_editor_view_menu_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    Widget menu;

    XtVaGetValues(widget, XmNsubMenuId, &menu, 0);

    fe_WidgetTreeWalk(menu, fe_view_menu_mappee, closure);
}

CB_STATIC void
fe_editor_windows_menu_cb(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
    MWContext*      context = (MWContext *)closure;
    fe_ContextData* data = CONTEXT_DATA(context);
    Widget          submenu;

    fe_UserActivity(context);

    XtVaGetValues(widget, XmNsubMenuId, &submenu, 0);

    data->windows_menu_up_to_date_p = False;
    fe_GenerateWindowsMenu(context);
}

CB_STATIC void
fe_editor_paragraph_marks_cb(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean    set;

    if (fe_EditorParagraphMarksGetState(context))
	set = FALSE; /* switch */
    else
	set = TRUE;

    fe_EditorParagraphMarksSetState(context, set);
    /* NOTE: view menu cascade callback sets button visual state */
}

#if 0
/* experimental, get rid of those fuckking switchs, and string switches */
typedef Widget (*fe_ToolbarWidgetCreateFunction)(
		  MWContext* context,
		  Widget     parent,
		  char*      name,
		  Arg*       args,
		  Cardinal   n
		  );
typedef struct fe_ToolbarWidgetClass {
    char*                          name;
    fe_ToolbarWidgetCreateFunction creator;
} fe_ToolbarWidgetClass;

static Widget
fe_CreateSizeGrowShrinkButton(
			      MWContext* context,
			      Widget     parent,
			      char*      name,
			      Arg*       args,
			      Cardinal   n
) {
    Widget button = fe_CreateToolbarButton(context, parent, name, args, n);
    XtAddCallback(buttons, XmNactivateCallback, fe_size_shrink_grow_cb, context);
}

static fe_ToolbarWidgetClass fe_SizeGrowShrinkButton = {
    "CharSizeGrowShrinkButton",
    fe_CreateSizeGrowShrinkButton
};

struct {
    fe_ToolbarWidgetClass* class;
    char*                  name; /* instance name */
    XtCallbackProc         callback;
    XtPointer              closure; /* passed as XmNuserdata */
} fe_editor_character_toolbar[] = {
  { fe_SizeGrowShrinkButton, "charSizeShrink", 0, (XtPointer)TRUE  },
  { fe_SizeGrowShrinkButton, "charSizeGrow",   0, (XtPointer)FALSE },
  { fe_SizeMenu,             "charSizeMenu" }
  { fe_SpacerLabel },
  { fe_TextAttributeButton,  "textBold",       0, (XtPointer)TF_BOLD },
  { fe_TextAttributeButton,  "textItalic",     0, (XtPointer)TF_ITALIC },
  { fe_TextAttributeButton,  "textFixed",      0,  (XtPointer)TF_FIXED },
  { fe_ToolbarButton,       "textColor",      color_callback }
  { fe_ToolbarButton,       "link",           link_callback }
  { fe_ToolbarButton,       "plain",          clear_callback }
  { fe_SpacerLabel },
  { fe_ToolbarButton,       "targetInsert",   target_callback }
  { fe_ToolbarButton,       "imageInsert",    image_callback }
  { fe_ToolbarButton,       "hruleInsert",    hrule_callback }
  { fe_SpacerLabel },
  { fe_ToolbarButton,       "properties",     properties_callback }
  { 0 }
};

Widget
fe_SizeMenuCreate(
		  MWContext* context,
		  Widget     parent,
		  char*      name
		  )
{
}
typedef Widget (*fe_ToolbarWidgetCreateFunction)(
		  MWContext* context,
		  Widget     parent,
		  char*      name
		  );
#endif

typedef struct fe_OptionMenuItemDescription {
    char* name;  /* of widget               */
    char* label; /* delete me */
    void* data;  /* gets passed as userdata */
} fe_OptionMenuItemDescription;

static Widget
create_option_menu(
		   MWContext* context,
		   Widget parent,
		   char* name,
		   fe_OptionMenuItemDescription* list,
		   Arg* p_argv, Cardinal p_argc
) {
  unsigned i;
  Widget   popup_menu;
  Widget   buttons[16];
  Widget   option_menu;
  Arg      argv[8];
  Cardinal argc;
  Visual*  v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtCallbackRec* button_callback_rec = NULL;

  for (i = 0; i < p_argc; i++) {
      if (p_argv[i].name == XmNactivateCallback)
	  button_callback_rec = (XtCallbackRec*)p_argv[i].value;
  }
  
  XtVaGetValues(CONTEXT_WIDGET (context), XtNvisual, &v, XtNcolormap, &cmap,
	XtNdepth, &depth, 0);

  argc = 0;
  XtSetArg (argv[argc], XmNvisual, v); argc++;
  XtSetArg (argv[argc], XmNdepth, depth); argc++;
  XtSetArg (argv[argc], XmNcolormap, cmap); argc++;
#if 0
  sprintf(buf, "%sPulldown", name);
#endif
  popup_menu = XmCreatePulldownMenu(parent, name, argv, argc);

  for (i = 0; list[i].name; i++) {
      argc = 0;
      XtSetArg(argv[argc], XmNuserData, (XtPointer)list[i].data); argc++;
      buttons[i] = XmCreatePushButtonGadget(popup_menu, list[i].name, argv, argc);
      
      if (button_callback_rec)
	  XtAddCallback(
			buttons[i],
			XmNactivateCallback,
			button_callback_rec->callback,
			(XtPointer)context
			);
  }

  XtManageChildren(buttons, i);

  argc = 0;
  XtSetArg(argv[argc], XmNsubMenuId, popup_menu); argc++;
  option_menu = fe_CreateOptionMenu(parent, name, argv, argc);

  /*
   *    Hide the label, as we don't use it.
   */
  fe_UnmanageChild_safe(XmOptionLabelGadget(option_menu));

  return option_menu;
}


/* -------------------------------------------------------------- */

/*
 *    Size Group.
 */
static fe_OptionMenuItemDescription fe_size_menu_items[] =
{
    { "minusTwo", "-2", (void*)ED_FONTSIZE_MINUS_TWO },
    { "minusOne", "-1", (void*)ED_FONTSIZE_MINUS_ONE },
    { "plusZero", "+0", (void*)ED_FONTSIZE_ZERO      },
    { "plusOne",  "+1", (void*)ED_FONTSIZE_PLUS_ONE  },
    { "plusTwo",  "+2", (void*)ED_FONTSIZE_PLUS_TWO  },
    { "plusThree","+3", (void*)ED_FONTSIZE_PLUS_THREE },
    { "plusFour", "+4", (void*)ED_FONTSIZE_PLUS_FOUR },
    { NULL }
};
#define FE_FONT_NSIZES (countof(fe_size_menu_items) - 1) /* -1 for null terminator */

void
fe_EditorFontSizeSet(MWContext* context, ED_FontSize edt_size)
{
    if (edt_size >= EDT_FONTSIZE_MIN && edt_size <= EDT_FONTSIZE_MAX) {
#if 1
	EDT_SetFontSize(context, edt_size);
#else
	/* this does not work */
	EDT_CharacterData* edt_cdata = EDT_GetCharacterData(context);
	edt_cdata->mask = TF_FONT_SIZE;
	edt_cdata->values = TF_FONT_SIZE;
	edt_cdata->iSize = edt_size;
	EDT_SetCharacterData(context, edt_cdata);
	EDT_FreeCharacterData(edt_cdata);
#endif
    } else {
	fe_Bell(context);
    }

    fe_EditorUpdateToolbar(context, TF_FONT_SIZE);
}

ED_FontSize
fe_EditorFontSizeGet(MWContext* context)
{
    EDT_CharacterData* edt_cdata = EDT_GetCharacterData(context);
    intn               edt_size = edt_cdata->iSize;

    if (edt_size < EDT_FONTSIZE_MIN)
	edt_size = ED_FONTSIZE_ZERO;
    
    EDT_FreeCharacterData(edt_cdata);

    return (ED_FontSize)edt_size;
}

static void
fe_size_button_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext*  context = (MWContext *)closure;
    unsigned    is_growser = (unsigned)fe_GetUserData(widget);
    ED_FontSize edt_size = fe_EditorFontSizeGet(context);

    if (is_growser) /* up */
	edt_size++;
    else
	edt_size--;

    if (edt_size >= EDT_FONTSIZE_MIN && edt_size <= EDT_FONTSIZE_MAX)
	fe_EditorFontSizeSet(context, edt_size);
}

CB_STATIC void
fe_editor_font_size_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    unsigned   edt_size = (unsigned)fe_GetUserData(widget);

    fe_EditorFontSizeSet(context, edt_size);
}

static void
fe_set_toggle_state(MWContext* context, Widget cascade, unsigned match_userdata, Boolean radio)
{
    Arg        args[4];
    Cardinal   n;
    Widget     popup_menu;
    WidgetList children;
    Cardinal   nchildren;
    unsigned   userdata;
    Widget     widget;
    Boolean    set;
    int i;

    /*
     *    Get the pulldown menu from the cascade.
     */
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, &popup_menu); n++;
    XtGetValues(cascade, args, n);

    /*
     *    Get the children of the popup.
     */
    n = 0;
    XtSetArg(args[n], XmNchildren, &children); n++;
    XtSetArg(args[n], XmNnumChildren, &nchildren); n++;
    XtGetValues(popup_menu, args, n);

    /*
     *    Set the state of the buttons.
     */
    for (i = 0; i < nchildren; i++) {

	widget = children[i];
	if (XtIsSubclass(widget, xmToggleButtonGadgetClass)) {
	    userdata = (unsigned)fe_GetUserData(widget);

	    if ((radio && (userdata == match_userdata)) || (!radio && (userdata & match_userdata)))
		set = TRUE;
	    else
		set = FALSE;

	    XmToggleButtonGadgetSetState(widget, set, FALSE);
	}
    }
}

CB_STATIC void
fe_editor_font_size_menu_cb(Widget cascade, XtPointer closure, XtPointer call_data)
{
    MWContext*  context = (MWContext *)closure;
    ED_FontSize size = fe_EditorFontSizeGet(context);

    fe_set_toggle_state(context, cascade, (unsigned)size, TRUE);
}

Widget
fe_OptionMenuSetHistory(Widget menu, unsigned index)
{
    Arg        args[4];
    Cardinal   n;
    Widget     cascade;
    Widget     popup_menu;
    WidgetList children;
    Cardinal   nchildren;

    /*
     *   Update the label, and set the position of the popup.
     */
    cascade = XmOptionButtonGadget(menu);

    /*
     *    Get the popup menu from the cascade.
     */
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, &popup_menu); n++;
    XtGetValues(cascade, args, n);

    /*
     *    Get the children of the popup.
     */
    n = 0;
    XtSetArg(args[n], XmNchildren, &children); n++;
    XtSetArg(args[n], XmNnumChildren, &nchildren); n++;
    XtGetValues(popup_menu, args, n);
    if (index < nchildren) {
	/*
	 *    Finally, set the Nth button as history.
	 */
	n = 0;
	XtSetArg(args[n], XmNmenuHistory, children[index]); n++;
	/* NOTE: set it on the top level menu (strange) */
	XtSetValues(menu, args, n);
	
	return children[index];
    }
    return NULL;
}

static void
fe_SizeGroupUpdate(fe_EditorContextData* editor, int edt_size)
{
    Boolean sensitive;

    if (edt_size <= 0)
	edt_size = 3; /* + 0 */

    if (edt_size > EDT_FONTSIZE_MAX)
	edt_size = EDT_FONTSIZE_MAX;

    /*
     *    Set Shrink, grow buttons.
     */
    if (editor->toolbar_smaller) {
	if (edt_size > EDT_FONTSIZE_MIN)
	    sensitive = TRUE;
	else
	    sensitive = FALSE;
	XtVaSetValues(editor->toolbar_smaller, XtNsensitive, sensitive, 0);
    }

    if (editor->toolbar_bigger) {
	if (edt_size < EDT_FONTSIZE_MAX)
	    sensitive = TRUE;
	else
	    sensitive = FALSE;
	XtVaSetValues(editor->toolbar_bigger, XtNsensitive, sensitive, 0);
    }

    if (editor->toolbar_size) {
	fe_OptionMenuSetHistory(editor->toolbar_size, edt_size - 1);
    }
}

/* -------------------------------------------------------------- */
/*
 *    Text Attribute set.
 */
void
fe_EditorCharacterPropertiesSet(MWContext* context, ED_TextFormat values)
{
    EDT_CharacterData cdata;

    memset(&cdata, 0, sizeof(EDT_CharacterData));

    cdata.mask = TF_ALL_MASK;
    if (values == TF_NONE) {   /* pop everything */
	cdata.values = 0;
    } else if (values == (TF_SERVER|TF_SCRIPT)) {
	cdata.mask = TF_SERVER|TF_SCRIPT;
	cdata.values = 0;
    } else if (values == TF_SERVER || values == TF_SCRIPT) {
	cdata.mask = TF_SERVER|TF_SCRIPT;
	cdata.values = values;
    } else {
	values &= TF_ALL_MASK; /* don't let them shoot themselves */
	cdata.values = values;
    }

    EDT_SetCharacterData(context, &cdata);

    fe_EditorUpdateToolbar(context, values);
}

ED_TextFormat
fe_EditorCharacterPropertiesGet(MWContext* context)
{
    EDT_CharacterData* edt_cdata = EDT_GetCharacterData(context);
#if 0
    ED_TextFormat      values = (edt_cdata->values & TF_ALL_MASK);
#else
    ED_TextFormat      values = edt_cdata->values;
#endif

    EDT_FreeCharacterData(edt_cdata);
    
    return values;
}

static void
fe_EditorDoPoof(MWContext* context)
{
    Boolean            clear_link = TRUE;
    EDT_CharacterData* pData;
	
    if (EDT_SelectionContainsLink(context)) {
	/*Do you want to remove the link?*/
	if (!XFE_Confirm(context,
			 XP_GetString(XFE_EDITOR_WARNING_REMOVE_LINK)))
	    clear_link = FALSE;
    }

    if (clear_link) {
	EDT_FormatCharacter(context, TF_NONE);
    } else {
	pData = EDT_GetCharacterData(context);
        if (pData) {
            pData->mask = ~TF_HREF;
            pData->values = TF_NONE;
            EDT_SetCharacterData(context, pData);
            EDT_FreeCharacterData(pData);
        }
    }
    fe_EditorUpdateToolbar(context, 0);
}

CB_STATIC void
fe_editor_clear_char_props_cb(Widget widget, XtPointer closure,
			      XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorDoPoof(context);
}

CB_STATIC void
fe_editor_char_props_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext*     context = (MWContext *)closure;
    ED_ETextFormat mask = (ED_ETextFormat)fe_GetUserData(widget);

    fe_EditorCharacterPropertiesSet(context, mask);
}

CB_STATIC void
fe_editor_toggle_char_props_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext*     context = (MWContext *)closure;
    ED_ETextFormat mask = (ED_ETextFormat)fe_GetUserData(widget);
    ED_TextFormat  values = fe_EditorCharacterPropertiesGet(context);

    values ^= mask; /* toggle */

    /*
     *    Super and Sub should be mutually exclusive.
     */
    if (mask == TF_SUPER && (values & TF_SUPER) != 0)
	values &= ~TF_SUB;
    else if (mask == TF_SUB && (values & TF_SUB) != 0)
	values &= ~TF_SUPER;
    
    fe_EditorCharacterPropertiesSet(context, values);
}

CB_STATIC void
fe_editor_char_props_menu_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext*     context = (MWContext *)closure;
    ED_ETextFormat values = fe_EditorCharacterPropertiesGet(context);

    fe_set_toggle_state(context, widget, (unsigned)values, FALSE);
}

static void
fe_TextAttributeGroupUpdate(fe_EditorContextData* editor, ED_TextFormat attributes)
{
    XmToggleButtonGadgetSetState(editor->toolbar_bold, 
				 ((attributes & TF_BOLD) != 0),
				 FALSE);
    XmToggleButtonGadgetSetState(editor->toolbar_italic,
				 ((attributes & TF_ITALIC) != 0),
				 FALSE);
    XmToggleButtonGadgetSetState(editor->toolbar_fixed, 
				 ((attributes & TF_FIXED) != 0),
				 FALSE);
}

/* -------------------------------------------------------------- */
/*
 *    Horizontal Rule.
 */
CB_STATIC void
fe_editor_insert_hrule_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext*     context = (MWContext *)closure;

    /*
     *    Hrule is so simple, just use default values
     *   instead of bringing up properties dialog
     */
    EDT_HorizRuleData *pData = EDT_NewHorizRuleData();
    if (pData) {
        EDT_InsertHorizRule(context, pData);
	EDT_FreeHorizRuleData(pData);
    }
}

void
fe_EditorIndent(MWContext* context, Boolean is_indent)
{
    if (is_indent)
	EDT_Outdent(context);
    else
	EDT_Indent(context);
}

/* -------------------------------------------------------------- */
/*
 *    List set.
 */

/* -------------------------------------------------------------- */
/*
 *    Indent Set.
 */
CB_STATIC void
fe_editor_indent_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    unsigned   is_indent = (unsigned)fe_GetUserData(widget);

    fe_EditorIndent(context, !is_indent);
}

/* -------------------------------------------------------------- */
/*
 *    Align Set.
 */
void
fe_EditorAlignSet(MWContext* pMWContext, ED_Alignment align)
{
    ED_ElementType type = EDT_GetCurrentElementType(pMWContext);

    switch ( type ){
        case ED_ELEMENT_HRULE:
        {
            EDT_HorizRuleData* pData = EDT_GetHorizRuleData(pMWContext);
            if ( pData ){
                pData->align = align;
                EDT_SetHorizRuleData(pMWContext, pData);
            }
            break;
        }
       default: /* For Images, Text, or selection, this will do all: */
            EDT_SetParagraphAlign( pMWContext, align );
            break;
    }

    fe_EditorUpdateToolbar(pMWContext, 0);
}

ED_Alignment
fe_EditorAlignGet(MWContext* pMWContext)
{
   ED_ElementType type = EDT_GetCurrentElementType(pMWContext);
   EDT_HorizRuleData h_data;

   if (type == ED_ELEMENT_HRULE) {
       fe_EditorHorizontalRulePropertiesGet(pMWContext, &h_data);
       return h_data.align;
   } else { /* For Images, Text, or selection, this will do all: */
       return EDT_GetParagraphAlign(pMWContext);
   }
}

static void
fe_align_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext*     context = (MWContext *)closure;
    ED_Alignment   align = (ED_Alignment)fe_GetUserData(widget);
    
    fe_EditorAlignSet(context, align);
}

static void
fe_AlignGroupUpdate(fe_EditorContextData* editor, ED_Alignment align)
{
    unsigned mask;

    switch (align) {
    case ED_ALIGN_LEFT:  
    case ED_ALIGN_DEFAULT:mask = 0x1; break;
    case ED_ALIGN_ABSCENTER:
    case ED_ALIGN_CENTER: mask = 0x2; break;
    case  ED_ALIGN_RIGHT: mask = 0x4; break;
    default:              mask = 0;   break;
    }

    XmToggleButtonGadgetSetState(editor->toolbar_left,
				 ((mask & 0x1) != 0), FALSE);
    XmToggleButtonGadgetSetState(editor->toolbar_center,
				 ((mask & 0x2) != 0), FALSE);
    XmToggleButtonGadgetSetState(editor->toolbar_right,
				 ((mask & 0x4) != 0), FALSE);
}

/* -------------------------------------------------------------- */
/*
 *    List Set.
 */
static void
fe_toggle_list_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    intn       iTagType = (intn)fe_GetUserData(widget);

    EDT_ToggleList(context, iTagType);

    fe_EditorUpdateToolbar(context, 0);
}

static void
fe_toggle_list_update(fe_EditorContextData* editor, intn iTagType)
{
    unsigned mask;

    switch (iTagType) {
    case P_UNUM_LIST: mask = 0x1; break;
    case P_NUM_LIST:  mask = 0x2; break;
    default:          mask = 0;   break;
    }

    XmToggleButtonGadgetSetState(editor->toolbar_list,
				 (mask & 0x1)!=0, FALSE);
    XmToggleButtonGadgetSetState(editor->toolbar_numbers,
				 (mask & 0x2)!=0, FALSE);
}

/* -------------------------------------------------------------- */
/*
 *    Paragraph Styles Set.
 */
/*
 *   This is major bad non-NLS, but I just want to get teh look going....djw
 */
static fe_OptionMenuItemDescription fe_paragraph_style_menu_items[] = {
    { "normal",          "Normal",          (void*)P_PARAGRAPH  },
    { "headingOne",      "Heading 1",       (void*)P_HEADER_1   },
    { "headingTwo",      "Heading 2",       (void*)P_HEADER_2   },
    { "headingThree",    "Heading 3",       (void*)P_HEADER_3   },
    { "headingFour",     "Heading 4",       (void*)P_HEADER_4   },
    { "headingFive",     "Heading 5",       (void*)P_HEADER_5   },
    { "headingSix",      "Heading 6",       (void*)P_HEADER_6   },
    { "address",         "Address",         (void*)P_ADDRESS    },
    { "formatted",       "Formatted",       (void*)P_PREFORMAT  },
    { "listItem",        "List Item",       (void*)P_LIST_ITEM  },
    { "descriptionItem", "Description Item",(void*)P_DESC_TITLE },
    { "descriptionText", "Description Text",(void*)P_DESC_TEXT  },
    { 0 }
};

#define FE_PARAGRAPH_NSTYLES (countof(fe_paragraph_style_menu_items) - 1) /* -1 for null terminator */

TagType
fe_EditorParagraphPropertiesGet(MWContext* context)
{
    return EDT_GetParagraphFormatting(context);
}

void
fe_EditorParagraphPropertiesSet(MWContext* context, TagType type)
{
#if 1
    TagType paragraph_style = EDT_GetParagraphFormatting(context);

    if (type != paragraph_style) {
	EDT_MorphContainer(context, type);

	fe_EditorUpdateToolbar(context, 0);
    }
#else
    /*
     *    This seems like the correct code, as it would mean the toolbar
     *    menu and the properties dialog have the same effect when you set
     *    a list. But this would be different from the Windows version.
     *    Do above, the same as Windows.
     */
    if (type == P_LIST_ITEM) {
        EDT_ListData list_data;

        list_data.iTagType = P_UNUM_LIST;
        list_data.eType = ED_LIST_TYPE_DISC;
        list_data.bCompact = FALSE;

        fe_EditorParagraphPropertiesSetAll(context, type, &list_data,
					   ED_ALIGN_DEFAULT);
    } else {
        fe_EditorParagraphPropertiesSetAll(context, type, NULL,
					   ED_ALIGN_DEFAULT);
    }
#endif
}

CB_STATIC void
fe_editor_paragraph_props_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    unsigned   foo = (unsigned)fe_GetUserData(widget);
    TagType    new_paragraph_style = foo;

    fe_EditorParagraphPropertiesSet(context, new_paragraph_style);
}

CB_STATIC void
fe_editor_paragraph_props_menu_cb(Widget widget,
				  XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    TagType    paragraph_style = fe_EditorParagraphPropertiesGet(context);

    fe_set_toggle_state(context, widget, (unsigned)paragraph_style, TRUE);
}

static void
fe_editor_character_properties_dialog_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_CHARACTER);
}

static void
fe_editor_paragraph_properties_dialog_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_PARAGRAPH);
}

static void
fe_editor_image_properties_dialog_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_IMAGE);
}

static void
fe_editor_image_properties_dialog_update_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    sensitive;
    sensitive = (EDT_GetCurrentElementType(context) == ED_ELEMENT_IMAGE);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_editor_link_properties_dialog_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_LINK);
}

static void
fe_editor_link_properties_dialog_update_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    sensitive;
    sensitive = (EDT_CanSetHREF(context) != FALSE);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_editor_target_properties_dialog_update_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    sensitive;
    sensitive = (EDT_GetCurrentElementType(context) == ED_ELEMENT_TARGET);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_editor_hrule_properties_dialog_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;

    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_HRULE);
}

static void
fe_editor_hrule_properties_dialog_update_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    sensitive;

    sensitive = (EDT_GetCurrentElementType(context) == ED_ELEMENT_HRULE);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_properties_menu_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    Arg        args[4];
    Cardinal   n;
    Widget     pulldown_menu;
    Widget*    children;
    Cardinal   nchildren;
    char*      name;
    Widget     button;
    Boolean    sensitive;
    int        i;
    fe_EditorPropertiesDialogType type;

    /*
     *    Get the popup menu from the cascade.
     */
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, &pulldown_menu); n++;
    XtGetValues(widget, args, n);

    /*
     *    Get the children of the popup.
     */
    n = 0;
    XtSetArg(args[n], XmNchildren, &children); n++;
    XtSetArg(args[n], XmNnumChildren, &nchildren); n++;
    XtGetValues(pulldown_menu, args, n);

    for (i = 0; i < nchildren; i++) {

	button = children[i];
	name = XtName(button);

	if ( /* hack, hack, hack - we need dependencies!!!! */
	    strcmp(name, "linkProperties") == 0
	    ||
	    strcmp(name, "targetProperties") == 0
	    ||
	    strcmp(name, "imageProperties") == 0
	    ||
	    strcmp(name, "hruleProperties") == 0
	    ||
	    strcmp(name, "tagProperties") == 0
        ) {
	    type = (fe_EditorPropertiesDialogType)fe_GetUserData(button);
	    
	    if (fe_EditorPropertiesDialogCanDo(context, type))
		sensitive = TRUE;
	    else
		sensitive = FALSE;
	} else if (strcmp(name, "tableProperties") == 0) {
	    sensitive = fe_EditorTableCanDelete(context);
	} else {
	    continue; /* skip setval */
	}

	n = 0;
	XtSetArg(args[n], XtNsensitive, sensitive); n++;
	XtSetValues(button, args, n);
    }
}

CB_STATIC void
fe_editor_hrule_properties_cb(Widget widget, XtPointer closure,
			      XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    if (fe_EditorHorizontalRulePropertiesCanDo(context))
       fe_EditorHorizontalRulePropertiesDialogDo(context);
}

static void
fe_paragraph_style_toolbar_update(fe_EditorContextData* editor, TagType style)
{
    unsigned   index;

    if (editor->toolbar_style) {

	/*
	 *    Search through style table looking for match to this style.
	 */
	for (index = FE_PARAGRAPH_NSTYLES - 1; index > 0; index--) {
	    if ((TagType)(unsigned)fe_paragraph_style_menu_items[index].data == style)
		break;
	}

	fe_OptionMenuSetHistory(editor->toolbar_style, index);
    }
}

static Widget
fe_hack_out_editor_toggle_button(MWContext* context, Widget parent,
				 char* name, int icon_number)
{
    Pixmap   p;
    Arg      args[20];
    Cardinal n;

    p =  fe_ToolbarPixmap(context, icon_number, False, False);
      
    n = 0;
    XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
    XtSetArg(args[n], XmNlabelPixmap, p); n++;
    XtSetArg(args[n], XmNindicatorOn, FALSE); n++; /* Windows-esk */
    XtSetArg(args[n], XmNvisibleWhenOff, TRUE); n++;
    XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++; /* why do I need this? */
    return XmCreateToggleButtonGadget(parent, name, args, n);
}

static Widget
fe_hack_out_editor_push_button(MWContext* context, Widget parent,
				 char* name, int icon_number)
{
    Pixmap   p;
    Pixmap   ip;
    Arg      args[20];
    Cardinal n = 0;

    p =  fe_ToolbarPixmap(context, icon_number, False, False);
    ip = fe_ToolbarPixmap(context, icon_number, True, False);
      
    if (!ip)
	ip = p;
      
    XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
    XtSetArg(args[n], XmNlabelPixmap, p); n++;
    XtSetArg(args[n], XmNlabelInsensitivePixmap, ip); n++;

    return XmCreatePushButtonGadget(parent, name, args, n);
}

CB_STATIC void 
fe_editor_properties_dialog_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    fe_EditorPropertiesDialogType type;
    ED_ElementType e_type;

    type = (fe_EditorPropertiesDialogType)fe_GetUserData(widget);

    if (type == XFE_EDITOR_PROPERTIES_OOPS) {

	if (EDT_GetHREF(context)) {
	    type = XFE_EDITOR_PROPERTIES_LINK;
	} else {
	    e_type = EDT_GetCurrentElementType(context);

	    if (e_type == ED_ELEMENT_HRULE)
		type = XFE_EDITOR_PROPERTIES_HRULE;
	    else if (e_type == ED_ELEMENT_IMAGE)
		type = XFE_EDITOR_PROPERTIES_IMAGE;
	    else if (fe_EditorTableCanDelete(context))
		type = XFE_EDITOR_PROPERTIES_TABLE;
	    else
		type = XFE_EDITOR_PROPERTIES_CHARACTER;
	}
    }

    /*
     *    Validate that we can do this kind of dialog right now.
     */
    if (fe_EditorPropertiesDialogCanDo(context, type)) {
	/*
	 *   Yuck, ugly. User must save before inerting links
	 *   or images.
	 */
	if (EDT_IS_NEW_DOCUMENT(context)
	    &&
	    (type == XFE_EDITOR_PROPERTIES_IMAGE_INSERT
	     ||
	     type == XFE_EDITOR_PROPERTIES_LINK_INSERT)
	) {
	    if (!FE_CheckAndSaveDocument(context))
		return;
	}
	fe_EditorPropertiesDialogDo(context, type);
    } else {
	fe_Bell(context);
    }
}

CB_STATIC void 
fe_editor_document_properties_dialog_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorDocumentPropertiesDialogDo(context,
			XFE_EDITOR_DOCUMENT_PROPERTIES_APPEARANCE);
}

CB_STATIC void
fe_editor_target_properties_dialog_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorTargetPropertiesDialogDo(context);
}

CB_STATIC void
fe_editor_html_properties_dialog_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorHtmlPropertiesDialogDo(context);
}

static void
fe_editor_html_properties_dialog_update_cb(Widget widget,
					    XtPointer closure,
					    XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean    sensitive;
    sensitive = (EDT_GetCurrentElementType(context) == ED_ELEMENT_UNKNOWN_TAG);

    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void 
fe_editor_preferences_dialog_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorPreferencesDialogDo(context,
				  XFE_EDITOR_DOCUMENT_PROPERTIES_APPEARANCE);
}

static void
fe_editor_show_menubar_cb(Widget widget,
			  XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (!CONTEXT_DATA(context)->show_menubar_p) {
      CONTEXT_DATA(context)->show_menubar_p = True;
      fe_RebuildWindow(context);
  }
}

CB_STATIC void
fe_editor_show_character_toolbar_cb(Widget widget,
				    XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Boolean toolbar_p = False;

  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &toolbar_p, 0);
  if (CONTEXT_DATA (context)->show_character_toolbar_p == toolbar_p)
    return;
  CONTEXT_DATA (context)->show_character_toolbar_p = toolbar_p;
  fe_RebuildWindow (context);
}

static void
fe_editor_show_character_toolbar_update_cb(Widget widget,
				    XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean    set = CONTEXT_DATA (context)->show_character_toolbar_p;

    XtVaSetValues(widget, XmNset, set, 0);
}

CB_STATIC void
fe_editor_show_paragraph_toolbar_cb(Widget widget,
				    XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Boolean toolbar_p = False;

  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &toolbar_p, 0);
  if (CONTEXT_DATA (context)->show_paragraph_toolbar_p == toolbar_p)
    return;
  CONTEXT_DATA (context)->show_paragraph_toolbar_p = toolbar_p;
  fe_RebuildWindow (context);
}

static void
fe_editor_show_paragraph_toolbar_update_cb(Widget widget,
				    XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;
    Boolean    set = CONTEXT_DATA (context)->show_paragraph_toolbar_p;

    XtVaSetValues(widget, XmNset, set, 0);
}

CB_STATIC void
fe_editor_options_menu_cb(Widget cascade,
			  XtPointer closure, XtPointer call_data)
{
    Widget     pulldown;
    WidgetList children;
    Cardinal   nchildren;
    Cardinal i;
    Widget button;
    char* name;

    XtVaGetValues(cascade, XmNsubMenuId, &pulldown, 0);
    XtVaGetValues(pulldown, XmNchildren, &children,
		  XmNnumChildren, &nchildren, 0);

    for (i = 0; i < nchildren; i++) {

	button = children[i];
	name = XtName(button);
	/* update */

	if (strcmp(name, "showCharacter") == 0)
	    fe_editor_show_character_toolbar_update_cb(button, closure, 0);
	else if (strcmp(name, "showParagraph") == 0)
	    fe_editor_show_paragraph_toolbar_update_cb(button, closure, 0);
    }
}

CB_STATIC void
fe_editor_table_toolbar_cb(Widget widget,
			  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;

    /*
     *    Always do the insert dialog.
     */
    fe_EditorTableCreateDialogDo(context);
}

CB_STATIC void
fe_editor_set_colors_dialog_cb(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorSetColorsDialogDo(context);
}

CB_STATIC void
fe_editor_default_color_cb(Widget widget, XtPointer closure,
			   XtPointer call_data)
{
    MWContext* context = (MWContext *)closure;

    fe_EditorColorSet(context, NULL);
}

#define WOFF(x) ((void *) (XtOffset(fe_ContextData *, editor.x)))

static struct fe_button fe_character_toolbar[] =
{
  { "charSizeShrink", fe_size_button_cb, SELECTABLE, 0, WOFF(toolbar_smaller),(void*)FALSE      },
  { "charSizeGrow",   fe_size_button_cb, SELECTABLE, 0, WOFF(toolbar_bigger), (void*)TRUE       },
  { "fontSize",       fe_editor_font_size_cb,        CASCADE,   (void*)fe_size_menu_items, WOFF(toolbar_size) },
  FE_MENU_SEPARATOR,
  { "bold",       fe_editor_toggle_char_props_cb,   TOGGLE, 0, WOFF(toolbar_bold),   (void*)TF_BOLD    },
  { "italic",     fe_editor_toggle_char_props_cb,   TOGGLE, 0, WOFF(toolbar_italic), (void*)TF_ITALIC  },
  { "fixed",      fe_editor_toggle_char_props_cb,   TOGGLE, 0, WOFF(toolbar_fixed),  (void*)TF_FIXED   },
  { "textColor",      fe_editor_set_colors_dialog_cb,   SELECTABLE, 0 },
  { "makeLink",       fe_editor_properties_dialog_cb,   SELECTABLE,
    0, WOFF(toolbar_link), (void*)XFE_EDITOR_PROPERTIES_LINK_INSERT },
  { "clearAllStyles", fe_editor_clear_char_props_cb,   SELECTABLE, 0, WOFF(toolbar_plain),  (void*)TF_NONE },
  FE_MENU_SEPARATOR,
  { "insertTarget",   fe_editor_target_properties_dialog_cb, SELECTABLE, 0 },
  { "insertImage",    fe_editor_properties_dialog_cb,   SELECTABLE,
    0, 0, (void*)XFE_EDITOR_PROPERTIES_IMAGE_INSERT },
  { "insertHorizontalLine", fe_editor_insert_hrule_cb, SELECTABLE, 0,
    WOFF(toolbar_hrule) },
  { "insertTable",    fe_editor_table_toolbar_cb, SELECTABLE, 0 },
  FE_MENU_SEPARATOR,
  { "objectProperties",     fe_editor_properties_dialog_cb,SELECTABLE, 0 },
  { 0 }
};

static struct fe_button fe_paragraph_toolbar[] =
{
  { "paragraphProperties", fe_editor_paragraph_props_cb, CASCADE, (void*)fe_paragraph_style_menu_items, WOFF(toolbar_style) },
  FE_MENU_SEPARATOR,
  { "insertBulleted",  fe_toggle_list_cb,   TOGGLE, 0, WOFF(toolbar_list), (void*)P_UNUM_LIST },
  { "insertNumbered",  fe_toggle_list_cb,   TOGGLE, 0, WOFF(toolbar_numbers), (void*)P_NUM_LIST },
  FE_MENU_SEPARATOR,
  { "outdent",         fe_editor_indent_cb, SELECTABLE, 0, WOFF(toolbar_outdent), (void*)FALSE },
  { "indent",          fe_editor_indent_cb, SELECTABLE, 0, WOFF(toolbar_indent),  (void*)TRUE  },
  FE_MENU_SEPARATOR,
  { "alignLeft",       fe_align_cb,         TOGGLE, 0, WOFF(toolbar_left), (void*)ED_ALIGN_LEFT },
  { "alignCenter",     fe_align_cb,         TOGGLE, 0, WOFF(toolbar_center), (void*)ED_ALIGN_ABSCENTER },
  { "alignRight",      fe_align_cb,         TOGGLE, 0, WOFF(toolbar_right), (void*)ED_ALIGN_RIGHT },
  { 0 }
};
#undef WOFF

/*
 *    Special code to make editor toolbars.
 */
static void
do_work(MWContext* context, Widget toolbar, struct fe_button* tb, int icon_number)
{
  Widget kids[40];
  Arg    av[20];
  int ac;
  int i, j;
  char* name;
  int tb_size;
  Widget b;
  XtCallbackRec cb_data;

  for (tb_size = 0; tb[tb_size].name; tb_size++)
    ;

  for (i = 0, j = 0; i < tb_size; i++) {
    name = tb[i].name;
    ac = 0;

    if (name[0] == '\0') {
      XmString xms = XmStringCreateLtoR ("", XmSTRING_DEFAULT_CHARSET);
      XtSetArg (av[ac], XmNlabelType, XmSTRING); ac++;
      XtSetArg (av[ac], XmNlabelString, xms); ac++;
      b = XmCreateLabelGadget (toolbar, "spacer", av, ac);
      XmStringFree (xms);
    } else if (tb[i].type == CASCADE) {
	ac = 0;
	if (tb[i].callback) {
	    cb_data.callback = tb[i].callback;
	    cb_data.closure = context;
	    XtSetArg(av[ac], XmNactivateCallback, &cb_data); ac++;
	}
	b = create_option_menu(context, toolbar, name, tb[i].var, av, ac);
    } else if (tb[i].type == TOGGLE) {
	/* Install userData if available */
	b = fe_hack_out_editor_toggle_button(context, toolbar, name, icon_number++);
	if (tb[i].userdata) {
	    ac = 0;
	    XtSetArg(av[ac], XmNuserData, tb[i].userdata); ac++;
	    XtSetValues(b, av, ac);
	}
	
	if (tb[i].callback)
	    XtAddCallback(b, XmNvalueChangedCallback, tb[i].callback, context);
    } else {
	/* Install userData if available */
	b = fe_hack_out_editor_push_button(context, toolbar, name, icon_number++);
	ac = 0;
	if (tb[i].userdata) {
	    XtSetArg(av[ac], XmNuserData, tb[i].userdata); ac++;
	}
	if (tb[i].type == UNSELECTABLE) {
	    XtSetArg(av[ac], XtNsensitive, False); ac++;
	}
	if (ac > 0)
	    XtSetValues(b, av, ac);
	
	if (tb[i].callback)
	    XtAddCallback(b, XmNactivateCallback, tb[i].callback, context);
    }

    if (tb[i].offset) {
	fe_ContextData *data = CONTEXT_DATA (context);
	Widget* foo = (Widget*)(((char*) data) + (int)(tb[i].offset));
	*foo = b;
    }

    kids [j++] = b;
  }

  XtManageChildren (kids, j);
}

static Widget
fe_MakeCharacterToolbar(MWContext* context, Widget parent)
{
  Widget toolbar;
  Arg    av[8];
  int    ac;
  
  ac = 0;
  XtSetArg (av[ac], XmNskipAdjust, True); ac++;
  XtSetArg (av[ac], XmNseparatorOn, False); ac++;
  XtSetArg (av[ac], XmNorientation, XmHORIZONTAL); ac++;
  XtSetArg (av[ac], XmNpacking, XmPACK_TIGHT); ac++;
  toolbar = XmCreateRowColumn(parent, "characterToolbar", av, ac);

  do_work(context, toolbar, fe_character_toolbar, 10);
  
  return toolbar;
}

static Widget
fe_MakeParagraphToolbar(MWContext* context, Widget parent)
{
  Widget toolbar;
  Arg    av[8];
  int    ac;
  
  ac = 0;
  XtSetArg (av[ac], XmNskipAdjust, True); ac++;
  XtSetArg (av[ac], XmNseparatorOn, False); ac++;
  XtSetArg (av[ac], XmNorientation, XmHORIZONTAL); ac++;
  XtSetArg (av[ac], XmNpacking, XmPACK_TIGHT); ac++;
  toolbar = XmCreateRowColumn(parent, "paragraphToolbar", av, ac);

  do_work(context, toolbar, fe_paragraph_toolbar, 23);

  return toolbar;
}

Widget
fe_EditorCreatePropertiesToolbar(MWContext* context, Widget parent, char* name)
{
    fe_ContextData* data = CONTEXT_DATA(context);
    Widget form;
    Widget rc;
    Widget character_toolbar = 0;
    Widget paragraph_toolbar = 0;
    Arg    args[4];
    Cardinal n;

    if (data->show_character_toolbar_p && data->show_paragraph_toolbar_p) {
	n = 0;
	form = XmCreateForm(parent, name, args, n);
    } else {
	form = parent;
    }
	
    if (data->show_character_toolbar_p) {
	n = 0;
	XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
	character_toolbar = XmCreateFrame(form, "characterToolbarFrame",
					  args, n);
	XtManageChild(character_toolbar);
        rc = fe_MakeCharacterToolbar(context, character_toolbar);
	XtManageChild(rc);
    }

    if (data->show_paragraph_toolbar_p)	{
	n = 0;
	XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
	paragraph_toolbar = XmCreateFrame(form, "paragraphToolbarFrame",
					  args, n);
	XtManageChild(paragraph_toolbar);
	rc = fe_MakeParagraphToolbar(context, paragraph_toolbar);
	XtManageChild(rc);
    }

    if (character_toolbar && !paragraph_toolbar) {
	form = character_toolbar;
    } else if (!character_toolbar && paragraph_toolbar) {
	form = paragraph_toolbar;
    } else {
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(character_toolbar, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, character_toolbar); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetValues(paragraph_toolbar, args, n);
    }

    data->character_toolbar = character_toolbar;
    data->paragraph_toolbar = paragraph_toolbar;
    
    return form;
}

/*
 *    Caret handling stuff. The caret is just a timed draw onto the
 *    screen, it doesn't exist in the image.
 */
#define FE_CARET_DEFAULT_TIME 500
#define FE_CARET_FLAGS_BLANK  0x1
#define FE_CARET_FLAGS_DRAW   0x2
#define FE_CARET_FLAGS_XOR    0x4
#define FE_CARET_DEFAULT_WIDTH 5

static void
fe_caret_draw(MWContext *context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;
    Widget    drawing_area = CONTEXT_DATA(context)->drawing_area;
    Display*  dpy = XtDisplay(drawing_area);
    Window    win = XtWindow(drawing_area);
    GC        gc;
    XGCValues gcv;
    int       x = data->x;
    int       y = data->y;
    unsigned  width = data->width;
    unsigned  height = data->height;

    memset(&gcv, ~0, sizeof(gcv));
#if 0
    {
    LO_Color  text_color;
    LO_Color  bg_color;
    fe_EditorDocumentGetColors(context, NULL, &bg_color, &text_color,
			       NULL, NULL, NULL);
    gcv.foreground = fe_GetPixel(context,
				 text_color.red,
				 text_color.green,
				 text_color.blue);
    }
#else
    gcv.foreground = CONTEXT_DATA(context)->fg_pixel;
#endif
    gc = fe_GetGC(drawing_area, GCForeground, &gcv);

    x -= CONTEXT_DATA(context)->document_x;
    y -= CONTEXT_DATA(context)->document_y;

    if ((width & 0x1) != 1)
	width++;
	
    /*
     *    Hack, hack, hack. Do something pretty david!
     */
#if 1
    XDrawLine(dpy, win, gc, x, y, x + width - 1, y);
    XDrawLine(dpy, win, gc, x + (width/2), y, x + (width/2), y + height - 1);
    XDrawLine(dpy, win, gc, x, y + height - 1, x + width - 1, y + height - 1);
#else
    XDrawRectangle(dpy, win, gc, x, y, width-1, height-1);
#endif
}

static void
fe_caret_undraw(MWContext *context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;
    Widget    drawing_area = CONTEXT_DATA (context)->drawing_area;
    Display*  dpy = XtDisplay(drawing_area);
    Window    win = XtWindow(drawing_area);
    GC        gc;
    XGCValues gcv;
    Visual*   visual;
    int       visual_depth;
    int       x = data->x;
    int       y = data->y;
    unsigned  width = data->width;
    unsigned  height = data->height;

    if (data->backing_store == 0) {
	/* screen co-ords */
	fe_ClearArea(context,
		     x - CONTEXT_DATA(context)->document_x,
		     y - CONTEXT_DATA(context)->document_y,
		     width + 1, height + 1);

	/* doc co-ords */ /* refreshArea seems to have off my one bugs */
	LO_RefreshArea(context, x, y, width + 1, height + 1);
	XFlush(dpy);

	XtVaGetValues(CONTEXT_WIDGET(context), XmNvisual, &visual, 0);
	if (!visual)
	    visual = fe_globalData.default_visual;
	
	visual_depth = fe_VisualDepth(dpy, visual);

	if (!height)
	    height++;

	data->backing_store = XCreatePixmap(dpy, win,
					    width, height, visual_depth);
	    
	x -= CONTEXT_DATA(context)->document_x;
	y -= CONTEXT_DATA(context)->document_y;

	memset(&gcv, ~0, sizeof(gcv));
	gc = fe_GetGC(drawing_area, 0, &gcv);
	XCopyArea(dpy, win, data->backing_store, gc,
		  x, y, width, height, 0, 0);
    } else {
	x -= CONTEXT_DATA(context)->document_x;
	y -= CONTEXT_DATA(context)->document_y;

	memset(&gcv, ~0, sizeof(gcv));
	gc = fe_GetGC(drawing_area, 0, &gcv);
	XCopyArea(dpy, data->backing_store, win, gc,
		  0, 0, width, height, x, y);
    }
}

static void
fe_caret_update(MWContext* context, XtIntervalId *id)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;

    if (data->showing) {
	fe_caret_undraw(context);
	data->showing = FALSE;
    } else {
	fe_caret_draw(context);
	data->showing = TRUE;
    }

    if (data->running) {
	data->timer_id = XtAppAddTimeOut(fe_XtAppContext,
					 data->time,
					 (XtTimerCallbackProc)fe_caret_update,
					 context);
    }
}

static void
fe_caret_cancel(MWContext* context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;
  
    if (data->running)
	XtRemoveTimeOut(data->timer_id);
    data->running = FALSE;

    if (data->showing)
	fe_caret_undraw(context);
    data->showing = FALSE;
    
    if (data->backing_store) {
	XFreePixmap(XtDisplay(CONTEXT_DATA(context)->drawing_area),
		    data->backing_store);
	data->backing_store = 0;
    }
}

static void
fe_caret_pause(MWContext* context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;

    if (data->running == FALSE) /* already paused or never started */
        return;

    XtRemoveTimeOut(data->timer_id);

    data->running = FALSE;

    if (data->showing == FALSE) /* always pause showing */
        fe_caret_update(context, NULL);
}

static void
fe_caret_unpause(MWContext* context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;

    if (data->running == TRUE || data->showing == FALSE) /* not paused */
        return;

    data->showing = FALSE;
    data->running = TRUE;

    /*
     *    Draw and set timer
     */
    fe_caret_update(context, NULL);
}

static void
fe_caret_begin(MWContext* context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;

    data->showing = FALSE;
    data->time = FE_CARET_DEFAULT_TIME;
    data->running = TRUE;

    fe_caret_update(context, NULL);

#ifdef XFE_EDITOR_IM_SUPPORT
    fe_editor_im_set_coords(context);
#endif /* XFE_EDITOR_IM_SUPPORT */
}

void
fe_EditorRefreshArea(MWContext* context, int x, int y, unsigned w, unsigned h)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;

    if (
#if 0
	/*
	 *    Don't use x, as LO_RefreshArea tends to redraw full width
	 *    anyway.
	 */
	((x < data->x && x + w >= data->x) ||
	 (x < data->x + data->width && x + w >= data->x + data->width))
	&&
#endif
	((y < data->y && y + h >= data->y) ||
	 (y < data->y + data->height && y + h >= data->y + data->height))) {

        if (data->backing_store) {
	    XFreePixmap(XtDisplay(CONTEXT_DATA(context)->drawing_area),
			data->backing_store);
	    data->backing_store = 0;
	}

	if (data->running) {
	    XtRemoveTimeOut(data->timer_id);
	    data->showing = TRUE; /* force restart of sequence next */
	}
	    
	if (data->showing) {
	    data->showing = FALSE;
	    fe_caret_update(context, NULL);
	}
    }
}

static void
fe_editor_keep_cursor_visible(MWContext* context)
{
    fe_EditorCaretData* c_data = &EDITOR_CONTEXT_DATA(context)->caret_data;
    int x = CONTEXT_DATA(context)->document_x;
    int y = CONTEXT_DATA(context)->document_y;
    unsigned win_height = CONTEXT_DATA(context)->scrolled_height;
    unsigned win_width = CONTEXT_DATA(context)->scrolled_width;
    Boolean  coffee_scroll = FALSE;

    if (c_data->y + c_data->height > y + win_height - 20) {
	y = c_data->y + c_data->height - win_height + 20; /* fudge */
	if (y + win_height > CONTEXT_DATA(context)->document_height)
	    y = CONTEXT_DATA(context)->document_height - win_height;
	coffee_scroll = TRUE;
    } else if (c_data->y < y + 10) {
	y = c_data->y - 10;
	if (y < 0)
	    y = 0;
	coffee_scroll = TRUE;
    }

     if (c_data->x + c_data->width > x + win_width - 20) {
	x = c_data->x + c_data->width - win_width + 20;
	if (x + win_width > CONTEXT_DATA(context)->document_width)
	    x = CONTEXT_DATA(context)->document_width - win_width;
	coffee_scroll = TRUE;
    } else if (c_data->x < x + 10) {
	x = c_data->x - 10;
	if (x < 0)
	    x = 0;
	coffee_scroll = TRUE;
    }

    if (coffee_scroll) {
        /*
	 *    Collect all pending exposures before we scroll.
	 */
	fe_SyncExposures(context);

	fe_ScrollTo(context, x, y);
    }
}

static void
fe_editor_keep_pointer_visible_autoscroll(XtPointer, XtIntervalId*);

static void
fe_editor_keep_pointer_visible(MWContext* context, int p_x, int p_y)
{
    fe_ContextData* data = CONTEXT_DATA(context);
    fe_EditorAscrollData* ascroll_data
	= &EDITOR_CONTEXT_DATA(context)->ascroll_data;
    Boolean coffee_scroll = FALSE;
    int x = data->document_x;
    int y = data->document_y;
    int delta = 0;

    if (ascroll_data->timer_id)
	XtRemoveTimeOut(ascroll_data->timer_id);

    ascroll_data->delta_x = 0;
    ascroll_data->delta_y = 0;

    if (p_y < data->document_y) {
	coffee_scroll = TRUE;
	y = p_y;
	delta = (data->document_y - p_y);
	ascroll_data->y = p_y;
	ascroll_data->delta_y = -delta;
    }

    if (p_y > data->document_y + data->scrolled_height) {
	coffee_scroll = TRUE;
	y = p_y - data->scrolled_height;
	delta = p_y - (data->document_y + data->scrolled_height);
	ascroll_data->y = p_y;
	ascroll_data->delta_y = delta;
    }

    if (coffee_scroll) {
        /*
	 *    Collect all pending exposures before we scroll.
	 */
	fe_SyncExposures(context);

	fe_ScrollTo(context, x, y);

	ascroll_data->timer_id = XtAppAddTimeOut(fe_XtAppContext,
				 (100),
				 fe_editor_keep_pointer_visible_autoscroll,
				 (XtPointer)context);
    }
}

static void
fe_editor_keep_pointer_visible_autoscroll(XtPointer closure, XtIntervalId* id)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorAscrollData* ascroll_data;

    ascroll_data = &EDITOR_CONTEXT_DATA(context)->ascroll_data;
    ascroll_data->timer_id = 0; /* because we won't pop again */
  
    ascroll_data->x += ascroll_data->delta_x;
    ascroll_data->y += ascroll_data->delta_y;

    EDT_ExtendSelection(context, ascroll_data->x, ascroll_data->y);

    fe_editor_keep_pointer_visible(context, ascroll_data->x, ascroll_data->y);
}

static void
fe_caret_set(MWContext* context, int x, int y, unsigned w, unsigned h)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;

    if (x != data->x || y != data->y || w != data->width || h != data->height)
	fe_caret_cancel(context);

    if (data->running)
	XtRemoveTimeOut(data->timer_id);

    data->x = x;
    data->y = y;
    data->width = w;
    data->height = h;

    fe_editor_keep_cursor_visible(context);
}

Boolean 
FE_GetCaretPosition(
		    MWContext *context,
		    LO_Position* where,
		    int32* caretX, int32* caretYLow, int32* caretYHigh
) {
  int32 xVal;
  int32 yVal;
  int32 yValHigh;

#if 0
  (void) fprintf(real_stderr, "FE_GetCaretPosition\n");
#endif

  if (!context || !where->element)
    return FALSE;
  
  xVal = where->element->lo_any.x + where->element->lo_any.x_offset;
  yVal = where->element->lo_any.y;
  yValHigh = yVal + where->element->lo_any.height;

  switch (where->element->type) {
  case LO_TEXT: {
    LO_TextStruct* text_data = & where->element->lo_text;
    LO_TextInfo    text_info;
    PA_Block       text_save = text_data->text;
    int            len_save = text_data->text_len;

    if (!text_data->text_attr)
      return FALSE;

    if (where->position <= text_data->text_len) {
      text_data->text += where->position;
      text_data->text_len -= where->position;
    }
    XFE_GetTextInfo(context, text_data, &text_info);
    text_data->text = text_save;
    text_data->text_len = len_save;
    
    xVal += text_info.max_width - 1;
  } break;

  case LO_IMAGE: {
    LO_ImageStruct *pLoImage = & where->element->lo_image;
    if (where->position == 0) {
      xVal -= 1;
    } else {
      xVal += pLoImage->width + 2 * pLoImage->border_width;
    }
  } break;
    
  default: {
    LO_Any *any = &where->element->lo_any;
    if (where->position == 0) {
      xVal -= 1;
    } else {
      xVal += any->width;
    }
  }
  }

  *caretX = xVal;
  *caretYLow = yVal;
  *caretYHigh = yValHigh;

  return TRUE;
}

/*
 *    char_offset is in characters!!!
 */
PUBLIC void
FE_DisplayTextCaret(MWContext* context, int iLocation, LO_TextStruct* text,
		    int char_offset)
{
  int       x;
  int       y;
  unsigned  width;
  unsigned  height;
  LO_TextInfo text_info;
  int16     save_len;

  /*
   *    Get info extent info about the first <char_offset> characters of
   *    text.
   */
  save_len = text->text_len;
  text->text_len = char_offset;
  XFE_GetTextInfo(context, text, &text_info);
  x = text->x + text->x_offset + text_info.max_width;
  y = text->y + text->y_offset;
  height =  text_info.ascent + text_info.descent;
  
  text->text_len = save_len;

#if 0
  /*
   *    Now get extent info about the next character, because we'll draw
   *    it's size as the cursor.
   */
  if (char_offset < save_len) {
  } else {
    width = 4;
  }
#endif

  width = FE_CARET_DEFAULT_WIDTH;
  x -= (FE_CARET_DEFAULT_WIDTH/2) + 1; /* middle of char and back */

  fe_caret_set(context, x, y, width, height);
  fe_caret_begin(context);
}

void 
FE_DisplayImageCaret(
		     MWContext*             context, 
		     LO_ImageStruct*        image,
		     ED_CaretObjectPosition caretPos)
{
#if 0
  fprintf (real_stderr,"FE_DisplayImageCaret\n");
#endif

  int       x;
  int       y;
  unsigned  width;
  unsigned  height;
  
  /*
   *    Get info extent info about the first <char_offset> characters of
   *    text.
   */
  x = image->x + image->x_offset;
  y = image->y + image->y_offset;
  width = FE_CARET_DEFAULT_WIDTH;
  height = image->height + (2 * image->border_width);

  if (caretPos == ED_CARET_BEFORE) {
    x -= 1;
  } else if (caretPos == ED_CARET_AFTER) {
    x += image->width + (2 * image->border_width);
  } else {
    width = image->width + (2 * image->border_width);
  }

  fe_caret_set(context, x, y, width, height);
  fe_caret_begin(context);
}

void FE_DisplayGenericCaret(MWContext * context, LO_Any * image,
                        ED_CaretObjectPosition caretPos )
{
#if 0
  fprintf (real_stderr,"FE_DisplayGenericCaret\n");
#endif
  int       x;
  int       y;
  unsigned  width;
  unsigned  height;
  
  /*
   *    Get info extent info about the first <char_offset> characters of
   *    text.
   */
  x = image->x + image->x_offset;
  y = image->y + image->y_offset;
  width = FE_CARET_DEFAULT_WIDTH;
  height = image->line_height;

  if (caretPos == ED_CARET_BEFORE) {
    x -= 1;
  } else if (caretPos == ED_CARET_AFTER) {
    x += image->width;
  } else {
    width = image->width;
  }

  fe_caret_set(context, x, y, width, height);
  fe_caret_begin(context);
}

PUBLIC void
FE_DestroyCaret(MWContext * context)
{
#if 0
  (void) fprintf(real_stderr, "FE_DestroyCaret\n");
#endif
  fe_caret_cancel(context);
}

PUBLIC void
FE_ShowCaret(MWContext * context)
{
#if 0
  (void) fprintf(real_stderr, "FE_ShowCaret\n");
#endif
  fe_caret_begin(context);
}

#if 0
void FE_DisplaySource(MWContext * context, char * source)
{
	(void) fprintf(real_stderr, "FE_DisplaySource\n");
}
#endif

/*
 *    Note all x, y co-ordinates are document relative.
 */
PUBLIC void 
FE_DocumentChanged(MWContext* context, int32 p_y, int32 p_height)
{
    int32  redraw_x = 0;
    int32  redraw_y;
    uint32 redraw_width = CONTEXT_DATA(context)->document_width;
    uint32 redraw_height;
    int32  win_y;
    uint32 win_height;
    int32  win_x;
#if 0
    uint32 win_width;
#endif
    int32  margin_height;
    int32  margin_width;

#if 0
    fprintf(
	    real_stderr,
	    "FE_DocumentChanged(%d,%d) "
	    "[doc=(0,0,%d,%d)] [win=(%d,%d,%d,%d)] ",
	    p_y, p_height,        /* passed in */
	    CONTEXT_DATA(context)->document_width,
	    CONTEXT_DATA(context)->document_height,
	    CONTEXT_DATA(context)->document_x,
	    CONTEXT_DATA(context)->document_y,
	    CONTEXT_DATA(context)->scrolled_width,
	    CONTEXT_DATA(context)->scrolled_height
	    );
#endif

    win_y = CONTEXT_DATA(context)->document_y;
    win_height = CONTEXT_DATA(context)->scrolled_height;

    win_x = CONTEXT_DATA(context)->document_x;
#if 0
    win_width = CONTEXT_DATA(context)->scrolled_width;
#endif

    /*
     *    Ok, it seems like layout doesn't take into account margins.
     *    If there was stuff in the margin before (because we *were*
     *    displaying something big, which was scrolled up), but now we
     *    are not, we must clear the margin. This code attempts
     *    to detect when we are close to the margin, and say, clear
     *    the whole thing. This seems kinda bogus, the back-end
     *    should know about margins (no?), but... djw
     */
    fe_GetMargin(context, &margin_width, &margin_height);
    
    if (p_y <= margin_height) {
        p_y = 0;
	if (p_height != -1)
	    p_height += margin_height;
    }

    redraw_y = p_y;

    if (p_height < 0) {
	redraw_height = CONTEXT_DATA(context)->document_height;
	/* make sure full repaint goes to end of window at least. */
	if (win_height > redraw_height)
	    redraw_height = win_height;
    } else {
	redraw_height = p_height;
    }

    /*
     *    Is doc change area above or below displayed region?
     */
    if (redraw_y > win_y + win_height || redraw_y + redraw_height < win_y)
	return; /* nothing to do */

    /*
     *    Clip redraw area.
     */
    if (redraw_y < win_y) {
	redraw_height -= (win_y - redraw_y);
	redraw_y = win_y;
    }

    if (redraw_y + redraw_height > win_y + win_height) {
	redraw_height = win_y + win_height - redraw_y;
    }

    /*
     *    Redraw background, and generate expose.
     */
    fe_ClearAreaWithExposures(context, redraw_x - win_x, redraw_y - win_y,
			      redraw_width, redraw_height, TRUE);
    
#if 0
    fprintf(
	    real_stderr,
	    "=> (%d, %d, %d, %d)\n",
	    redraw_x, redraw_y, redraw_width, redraw_height /* we did    */
	    );
#endif
}

void FE_GetDocAndWindowPosition(MWContext * context, int32 *pX, int32 *pY, 
    int32 *pWidth, int32 *pHeight )
{
#if 0
  (void) fprintf(real_stderr, "FE_GetDocAndWindowPosition\n");
#endif
  *pX = CONTEXT_DATA (context)->document_x;
  *pY = CONTEXT_DATA (context)->document_y;
  *pWidth = CONTEXT_DATA (context)->scrolled_width;
  *pHeight = CONTEXT_DATA (context)->scrolled_height;
}

void
FE_SetNewDocumentProperties(MWContext* context)
{

    LO_Color bg_color;
    LO_Color normal_color;
    LO_Color link_color;
    LO_Color active_color;
    LO_Color followed_color;
    char background_image[MAXPATHLEN];
    XP_Bool use_custom;
    Boolean keep_images;

    /*
     *    Set keep images state from prefs. 
     */
    fe_EditorPreferencesGetLinksAndImages(context, NULL, &keep_images);
    fe_EditorDocumentSetImagesWithDocument(context, keep_images);

    /*
     *    Get editor defaults.
     */
    use_custom = fe_EditorPreferencesGetColors(context,
					       background_image,
					       &bg_color,
					       &normal_color,
					       &link_color,
					       &active_color,
					       &followed_color);

    /*
     *    Apply.
     */
    if (use_custom) {
	fe_EditorDocumentSetColors(context,
				   background_image,
				   &bg_color,
				   &normal_color,
				   &link_color,
				   &active_color,
				   &followed_color);
    } else {
	fe_EditorDocumentSetColors(context,
				   background_image, /*independent of colors*/
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   NULL);
    }

    /*
     *    Now do other stuff.
     */
    /* Title */
    fe_EditorDocumentSetTitle(context, NULL); /* will show Untitled */

    /* Add fixed MetaData items for author, others?? */
    fe_EditorDocumentSetMetaData(context,
				 "Author",
				 fe_EditorPreferencesGetAuthor(context));
    fe_EditorDocumentSetMetaData(context,
				 "GENERATOR",
				 "Mozilla/X");
}

void
fe_EditorNotifyBackgroundChanged(MWContext* context)
{
    fe_EditorCaretData* data = &EDITOR_CONTEXT_DATA(context)->caret_data;
    
    if (data->backing_store) {
	XFreePixmap(XtDisplay(CONTEXT_DATA(context)->drawing_area),
		    data->backing_store);
	data->backing_store = 0;
    }
}

char* 
FE_URLToLocalName(char *name)
{
  char* qm;
  char* hm;
  char* begin;
  char* end;
  char* rv;
  char* nameSave;
  int   len;
  int   nameLen;

#if 0
  fprintf(real_stderr, "FE_URLToLocalName: in = %s\n", name);
#endif

  /*
   *    Save a copy of the name as we might drop characters after '?' or '#'
   */

  nameLen = strlen(name);
  nameSave = XP_ALLOC(nameLen + 1);
  memcpy(nameSave, name, nameLen);
  nameSave[nameLen] = '\0';

  /*
   *    Drop everything after a '?' or '#' character.
   */
  qm = strchr(nameSave, '?');
  hm = strchr(nameSave, '#');

  if (qm && hm) {
    end = (qm > hm)? qm: hm;
  } else if (qm) {
    end = qm;
  } else if (hm) {
    end = hm;
  } else {
    end = nameSave + nameLen;
  }

  *end = '\0';

  /*
   *    Then get the basename of what's left.
   */
  if ((begin = strrchr(nameSave, '/')) != NULL && begin[1] != '\0') {
    begin++;
  } else {
    begin = nameSave;
  }
    
  len = end - begin;
  rv = malloc(len + 1);
  memcpy(rv, begin, len);
  rv[len] = '\0';

#if 0
  fprintf(real_stderr, "FE_URLToLocalName: out = %s\n", rv);
#endif
  
  XP_FREE(nameSave);
  return rv;
}

Boolean FE_EditorPrefConvertFileCaseOnWrite(void)
{
  fprintf (real_stderr,"FE_EditorPrefConvertFileCaseOnWrite\n");
  return TRUE;
}

void 
FE_FinishedSave(MWContext* context, int status,
		char *pDestURL, int iFileNumber)
{
}

static void
fe_editor_copyright_hint_dialog(MWContext* context)
{
    if (!fe_globalPrefs.editor_copyright_hint)
	return;

    if (fe_HintDialog(context, XP_GetString(XFE_ERROR_COPYRIGHT_HINT))) {
	fe_globalPrefs.editor_copyright_hint = FALSE;
	
	/*
	 *    Save options.
	 */
	if (!XFE_SavePrefs((char *)fe_globalData.user_prefs_file,
			   &fe_globalPrefs)) {
	    fe_perror(context, XP_GetString(XFE_ERROR_SAVING_OPTIONS));
	}
    }
}


void
fe_EditorGetUrlExitRoutine(MWContext* context, URL_Struct* url, int status)
{
    if (status < 0) {
	/*
	 *    Make sure that it's not a publish. This code is only
	 *    meant for fe_GetURL() calls. Make sure the user gets to see
	 *    the error dialog, then request a delete on the editor.
	 */
	if (url != NULL && url->files_to_post == NULL) { /* not publish */

	   /*
	    *    Wait for popup menus to go away.
	    */
	    /* we need to check context->type because under certain cases we 
	     * reach this loop after the editor has been killed so context is
	     * garbage and if we don't check then ContextHasPopups fail
	     */
	    while (context && context->type == MWContextEditor && 
		   fe_ContextHasPopups(context))
    		fe_EventLoop();
	    
	    /*
	     *    Then request a dissapearing act.
	     */
	    XtAppAddTimeOut(fe_XtAppContext, 0, fe_editor_exit_timeout,
			    (XtPointer)context);
	}
    }
}


/*
 * Editor calls us when we are finished loading
 * We force saving document if not editing local file
 */
void
FE_EditorDocumentLoaded(MWContext* context)
{
    History_entry* hist_ent;
    Bool           do_save_dialog;
    Boolean        save = FALSE;
    Boolean        links = TRUE;
    Boolean        images = TRUE;
    char filebuf[MAXPATHLEN];
    char urlbuf[MAXPATHLEN];
    unsigned as_time;
    Boolean  as_enable;
    ED_FileError file_error;
    
    if (!context) {
	return;
    }
    
    /*
     *    Windows does a lot more gymnastics here to determine which
     *    docs should or should not be saved (especially new ones).
     *    Please look into this and match up.
     */
    /*FIXME*/
    do_save_dialog = FALSE;
    if ((hist_ent = SHIST_GetCurrent(&context->hist)) && hist_ent->address) {
	do_save_dialog = !NET_IsLocalFileURL(hist_ent->address);
    }
    
    if (!EDT_IS_NEW_DOCUMENT(context) && do_save_dialog) {
	
	/*
	 *    Prompt for saving remote document.
	 */
	fe_EditorPreferencesGetLinksAndImages(context, &links, &images);
	fe_DoSaveRemoteDialog(context, &save, &links, &images);

	/*
	 *    Tongue in cheek....
	 */
	if (save)
	    fe_editor_copyright_hint_dialog(context);

	if (save && !fe_SaveAsDialog(context, filebuf, fe_FILE_TYPE_HTML))
	    save = FALSE;

	if (save) {
	    /*
	     *    Grab the filename while we have it to form file: path.
	     */
	    PR_snprintf(urlbuf, sizeof(urlbuf), "file:%.900s", filebuf);
	
	    file_error = EDT_SaveFile(context,
				      hist_ent->address,
				      urlbuf,
				      TRUE,
				      images,
				      links);

	    if (file_error != ED_ERROR_NONE && file_error != ED_ERROR_CANCEL) {
		fe_editor_report_file_error(context, file_error, urlbuf,
					    FALSE);
	    }

		/* here we spin and spin until all the savings of images
	     * and stuff are done so that we get a "real" status back from
	     * the backend.  This makes saving semi-synchronous, good for
	     * status reporting
		 */
	    while(CONTEXT_DATA(context)->is_file_saving) {
		/* do nothing, we wait here */
		fe_EventLoop();
	    }

		/* now that we get the real status message, do stuff with it...*/
	    file_error = CONTEXT_DATA(context)->file_save_status;
	    if (file_error != ED_ERROR_NONE && file_error != ED_ERROR_CANCEL) {
	    	fe_editor_report_file_error(context, file_error, 
					    CONTEXT_DATA(context)->save_file_url,
					    FALSE);

	    } 
	    if (file_error != ED_ERROR_NONE)
		save = FALSE;
	}

	if (save) {
	    /*
	     *    Save the keep images with link thingy.
	     */
	    fe_EditorDocumentSetImagesWithDocument(context, images);

	} else { /* this means there was an error, or the user punted */

	    /*
	     *    Danger, danger Will Robinson. This is a crock.
	     *    The user cancelled the load (actually the load has
	     *    already happened, what a joke, so I guess they are
	     *    cancelling the edit), so we just need to delete
	     *    this context (closing the window, etc). We cannot
	     *    just call delete, because the editor lib, and 
	     *    net lib above us (our caller), still has a bunch of
	     *    pointers and goop into us (so why are they calling
	     *    ...Load<ed>), anyway, we need to setup a call back
	     *    to call back into the delete code after our callers
	     *    are done. Sounds like a job for a work proc huh?
	     *    Right you are Will, but Navigator's event loop
	     *    doesn't do work procs (I guess they would subvert
	     *    Netlib's hegemony), so I set a timeout (of zero)
	     *    which will be called as soon as we get back in the
	     *    event loop...djw
	     */
	    XtAppAddTimeOut(fe_XtAppContext, 0, fe_editor_exit_timeout,
			    (XtPointer)context);
	    return; /* out of here */
	}
    }

    /*
     *    Set autosave period.
     */
    fe_EditorPreferencesGetAutoSave(context, &as_enable, &as_time);
    EDT_SetAutoSavePeriod(context, as_time);

    fe_EditorUpdateToolbar(context, 0);
}

static char fe_SaveDialogSaveName[] = "saveMessageDialog";
static char fe_SaveDialogUploadName[] = "uploadMessageDialog";
static char fe_SaveDialogImageLoadName[] = "imageLoadMessageDialog";

typedef enum fe_SaveDialogType
{
    XFE_SAVE_DIALOG_SAVE,
    XFE_SAVE_DIALOG_UPLOAD,
    XFE_SAVE_DIALOG_IMAGELOAD
} fe_SaveDialogType;

typedef struct fe_SaveDialogInfo
{
    unsigned          nfiles;
    unsigned          current;
    fe_SaveDialogType type;
} fe_SaveDialogInfo;

static void
fe_save_dialog_info_init(Widget msg_box, fe_SaveDialogType type, unsigned n)
{
    fe_SaveDialogInfo* info = (fe_SaveDialogInfo*)XtNew(fe_SaveDialogInfo);
    
    if (info == NULL)
	return;

    info->nfiles = n;
    info->type = type;
    info->current = 0;

    XtVaSetValues(msg_box, XmNuserData, (XtPointer)info, 0);
}

#define XFE_SAVE_DIALOG_NEXT (-1)

static void
fe_save_dialog_info_set(Widget msg_box, char* filename, int index)
{
    XtPointer          user_data;
    fe_SaveDialogInfo* info;
    char               buf[MAXPATHLEN];
    int                id;
    char*              string;
    XmString           xm_string;

    XtVaGetValues(msg_box, XmNuserData, &user_data, 0);
    if (user_data != 0) {
	info = (fe_SaveDialogInfo*)user_data;

	if (index == XFE_SAVE_DIALOG_NEXT)
	    info->current++;
	else
	    info->current = index;

	if (info->current > info->nfiles)
	    info->nfiles = info->current;

	if (info->type == XFE_SAVE_DIALOG_IMAGELOAD)
	    id = XFE_LOADING_IMAGE_FILE;
	else if (info->type == XFE_SAVE_DIALOG_UPLOAD)
	    id = XFE_UPLOADING_FILE;
	else
	    id = XFE_SAVING_FILE;

	string = XP_GetString(id);

	if (!filename)
	    filename = "";
	sprintf(buf, string, filename);
	
	if (info->nfiles > 1) {
	    strcat(buf, "\n");
	    string = XP_GetString(XFE_FILE_N_OF_N);
	    sprintf(&buf[strlen(buf)], string, info->current, info->nfiles);
	}
	
	xm_string = XmStringCreateLocalized(buf);
    } else {
	xm_string = XmStringCreateLocalized(filename);
    }
    XtVaSetValues(msg_box, XmNmessageString, xm_string, 0);
    XmStringFree(xm_string);
}

static Boolean
fe_SaveDialogIsSave(Widget msg_box)
{
    char* name = XtName(msg_box);

    return (strcmp(name, fe_SaveDialogSaveName) == 0);
}

static void
fe_save_dialog_cancel_cb(Widget msg_box, XtPointer closure, XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;

    if (fe_SaveDialogIsSave(msg_box)) {
        EDT_SaveCancel(context);
    } else {
        NET_InterruptWindow(context);
    }
}

static Widget
fe_save_dialog_create(MWContext* context, char* name)
{
    Widget mainw = CONTEXT_WIDGET(context);
    Widget msg_box;
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    Arg args [20];
    Cardinal n;

    if (CONTEXT_DATA(context)->posted_msg_box != NULL)
	return CONTEXT_DATA(context)->posted_msg_box;

    XtVaGetValues(mainw, XtNvisual, &v, XtNcolormap, &cmap,
		  XtNdepth, &depth, 0);

    n = 0;
    XtSetArg(args[n], XmNvisual, v); n++;
    XtSetArg(args[n], XmNdepth, depth); n++;
    XtSetArg(args[n], XmNcolormap, cmap); n++;
    XtSetArg(args[n], XmNallowShellResize, TRUE); n++;
    XtSetArg(args[n], XmNtransientFor, mainw); n++;
    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_MESSAGE); n++;
    XtSetArg(args[n], XmNdeleteResponse, XmDESTROY); n++;
    XtSetArg(args[n], XmNautoUnmanage, FALSE); n++;
    XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;

    msg_box = XmCreateMessageDialog(mainw, name, args, n);

    fe_UnmanageChild_safe(XmMessageBoxGetChild(msg_box, XmDIALOG_OK_BUTTON));
    fe_UnmanageChild_safe(XmMessageBoxGetChild(msg_box, XmDIALOG_HELP_BUTTON));

    XtAddCallback(msg_box, XmNcancelCallback, fe_save_dialog_cancel_cb,
		  (XtPointer)context);

    CONTEXT_DATA(context)->posted_msg_box = msg_box;

    return msg_box;
}

static void 
fe_save_dialog_destroy(Widget msg_box)
{
    XtPointer  user_data;
    
    XtVaGetValues(msg_box, XmNuserData, &user_data, 0);
    if (user_data != NULL)
	XtFree(user_data);

    XtDestroyWidget(msg_box);

}

/*
 *    Dialog to give feedback and allow canceling, overwrite protection
 *    when downloading remote files 
 *
 *    put up a save dialog that says "i'm saving foo right now", cancel?
 *    and return right away. 
 */
void 
FE_SaveDialogCreate(MWContext* context, int nfiles, Bool is_upload)
{
    Widget msg_box;
    char*  name;
    fe_SaveDialogType type;

    if (CONTEXT_DATA(context)->posted_msg_box != NULL)
	return;

    if (is_upload) {
	name = fe_SaveDialogUploadName;
	type = XFE_SAVE_DIALOG_UPLOAD;
    } else {
	name = fe_SaveDialogSaveName;
	type = XFE_SAVE_DIALOG_SAVE;
    }

    msg_box = fe_save_dialog_create(context, name);

    fe_save_dialog_info_init(msg_box, type, nfiles);

    CONTEXT_DATA(context)->posted_msg_box = msg_box;

	/* this is here so that we know when the file saying is over */
    CONTEXT_DATA(context)->is_file_saving = True;

    /*
     *    Postpone until we have something to say..
    XtManageChild(msg_box);
     */
}

void 
FE_SaveDialogSetFilename(MWContext* context, char* filename) 
{
    Widget msg_box = CONTEXT_DATA(context)->posted_msg_box;
    
    if (!msg_box)
	return;
    
    fe_save_dialog_info_set(msg_box, filename, XFE_SAVE_DIALOG_NEXT);
    
    if (!XtIsManaged(msg_box))
	XtManageChild(msg_box);
}

void 
FE_SaveDialogDestroy(MWContext* context, int status, char* file_url)
{
    Widget msg_box = CONTEXT_DATA(context)->posted_msg_box;
    
    if (msg_box == NULL)
	return;

    if (fe_SaveDialogIsSave(msg_box)) {
	CONTEXT_DATA(context)->save_file_url = file_url;
	CONTEXT_DATA(context)->file_save_status = status;
    } else { /* upload */
	/*
	 *    Do stuff for publish location.
	 */
    }
    
    fe_save_dialog_destroy(msg_box);
    CONTEXT_DATA(context)->posted_msg_box = NULL;
	/* this is here so that we know when the file saying is over */
    CONTEXT_DATA(context)->is_file_saving = False;
}

void
FE_ImageLoadDialog(MWContext* context)
{
    Widget msg_box;

    if (CONTEXT_DATA(context)->posted_msg_box != NULL)
	return;

    msg_box = fe_save_dialog_create(context, fe_SaveDialogImageLoadName);

    fe_save_dialog_info_init(msg_box, XFE_SAVE_DIALOG_IMAGELOAD, 1);
    CONTEXT_DATA(context)->posted_msg_box = msg_box;

    fe_save_dialog_info_set(msg_box, "", 1);

    XtManageChild(msg_box);
}

void
FE_ImageLoadDialogDestroy(MWContext* context)
{
    Widget msg_box = CONTEXT_DATA(context)->posted_msg_box;

    if (msg_box == NULL)
	return;

    fe_save_dialog_destroy(msg_box);

    CONTEXT_DATA(context)->posted_msg_box = NULL;
}

typedef struct {
  Widget  form;
  Widget  widget;
  unsigned response;
  Boolean done;
} YesToAllInfo;

static void
fe_yes_to_all_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  YesToAllInfo* info = (YesToAllInfo*)closure;
  int i;
  char* name;
  char* widget_name = XtName(widget);

  for (i = 0; (name = fe_CreateYesToAllDialog_button_data[i].name); i++) {
    if (strcmp(name, widget_name) == 0) {
      info->response = fe_CreateYesToAllDialog_button_data[i].type;
      break;
    }
  }

  info->widget = widget;
  info->done = TRUE;

  XtUnmanageChild(info->form);
}

#if 0
static unsigned
fe_OverWriteFileQuestionDialog(MWContext* context, char* filename)
{
  char* msg;
  int   size;
  Bool  rv = FALSE;

  size = XP_STRLEN(filename) + 1 /* slash*/
       + XP_STRLEN(fe_globalData.overwrite_file_message) + 1; /*nul*/

  msg = (char *)XP_ALLOC(size);
  if (!msg)
    return False;

  PR_snprintf(msg, size, fe_globalData.overwrite_file_message, filename);

  if (FE_Confirm(context, msg))
    rv = TRUE;

  XP_FREE(msg);

  return rv;
}
#endif

static unsigned
fe_DoYesToAllDialog(MWContext* context, char* filename)
{
  YesToAllInfo info;
  Arg      av[20];
  Cardinal ac;
  XtCallbackRec cb_data;
  XmString msg_string;
  char* msg;
  int size;
  Widget form;
  Widget default_button;

  size = XP_STRLEN(filename) + 1 /* slash*/
       + XP_STRLEN(fe_globalData.overwrite_file_message) + 1; /*nul*/

  msg = (char *)XP_ALLOC(size);
  if (!msg)
    return XFE_DIALOG_CANCEL_BUTTON;

  PR_snprintf(msg, size, fe_globalData.overwrite_file_message, filename);
  msg_string = XmStringCreateLocalized(msg);
  XP_FREE(msg);

  cb_data.callback = fe_yes_to_all_cb;
  cb_data.closure = &info;

  ac = 0;
  XtSetArg(av[ac], XmNarmCallback, &cb_data); ac++;
  XtSetArg(av[ac], XmNmessageString, msg_string); ac++;
  form = fe_CreateYesToAllDialog(context, "confirmSaveFiles", av, ac);

  default_button = fe_YesToAllDialogGetChild(form, XFE_DAILOG_YESTOALL_BUTTON);
  ac = 0;
  XtSetArg(av[ac], XmNdefaultButton, default_button); ac++;
#if 0 /* why doesn't this work? */
  XtSetArg(av[ac], XmNinitialFocus, default_button); ac++;
#endif
  XtSetValues(form, av, ac);

  info.response = XFE_DIALOG_CANCEL_BUTTON;
  info.done = FALSE;
  info.widget = NULL;
  info.form = form;

  XmStringFree(msg_string);
    
  XtManageChild(form);

  fe_NukeBackingStore(form);
  while (!info.done)
    fe_EventLoop();

  XtDestroyWidget(form);

  return info.response;
}

/*
typedef enum {
    ED_SAVE_OVERWRITE_THIS,
    ED_SAVE_OVERWRITE_ALL,
    ED_SAVE_DONT_OVERWRITE_THIS,
    ED_SAVE_DONT_OVERWRITE_ALL,
    ED_SAVE_CANCEL
} ED_SaveOption;
*/

ED_SaveOption
FE_SaveFileExistsDialog(MWContext* context, char* filename) 
{
#if 0
  fprintf(real_stderr,"FE_SaveFileExistsDialog\n");
#endif

  switch (fe_DoYesToAllDialog(context, filename)) {
  case XFE_DIALOG_YES_BUTTON:
    return ED_SAVE_OVERWRITE_THIS;
  case XFE_DAILOG_YESTOALL_BUTTON:
    return ED_SAVE_OVERWRITE_ALL;
  case XFE_DIALOG_NO_BUTTON:
    return ED_SAVE_DONT_OVERWRITE_THIS;
  case XFE_DIALOG_NOTOALL_BUTTON:
    return ED_SAVE_DONT_OVERWRITE_ALL;
  default:
    return ED_SAVE_CANCEL;
  }
}

#if 0 /* I have no idea... */
/*
 *    lifted from winfe/edview.cpp
 */
void 
TellSiteManagerFileSaved( char *szFile )
{
    if ( talkhandle != NULL ) {
        TALK_HApp sitemanager = TALK_GetFirstApp( talk_TAppSiteManager );
        if ( sitemanager ) {
	    /* Send the URL address as the DataIn. No return data needed */
            TALK_SendMessage(sitemanager, lvp_NG_HaveSavedUrl, 
                             szFile,                /* DataIn */
                             XP_STRLEN(szFile)+1,   /* Size */
                             NULL, 0 );             /* DataOut and size */
        }
    }
}
#endif

Boolean
FE_SaveErrorContinueDialog(MWContext*   context,
			   char*        filename, 
			   ED_FileError error)
{
  if (error != ED_ERROR_NONE) {
      /* There was a an error while saving on:\n%s */
      /* The error code = (%d).*/
      return fe_editor_report_file_error(context, error, filename, TRUE);
  } else {
      return TRUE;
  }
}

Boolean XP_ConvertUrlToLocalFile (const char *url, char **localName)
{
  Boolean filefound = FALSE;
  char *path = NULL;
  char *file = NULL;
  XP_StatStruct statinfo;

#if 0
  fprintf (real_stderr,"XP_ConvertURLToLocalFile: %s\n", url);
#endif

  if (localName)
    {
      *localName = NULL;
    }

  /* Must have "file:" url type and at least 1 character after '//' */
  if (!url || !NET_IsLocalFileURL((char *) url))
    {
	return FALSE; 
    }

  /* Extract file path from url: e.g. "/cl/foo/file.html" */
  path = NET_ParseURL (url, GET_PATH_PART);
  if (!path || XP_STRLEN(path) < 1) {
    return FALSE;
  }

#if 0
  /* Get filename - skip over initial "/" */
  file = WH_FileName (path+1, xpURL);
  XP_FREE (path);
#endif
  file = XP_STRDUP(path);
 
  /* Test if file exists */
  if (XP_Stat (file, &statinfo, xpURL) >= 0 &&
      statinfo.st_mode & S_IFREG)
    {
      filefound = TRUE;
    }
 
  if (localName)
    {
      /* Pass string to caller; we didn't change it, so there's
         no need to strdup
         */
      *localName = file;
    }

  return filefound;
}

char *
XP_BackupFileName (const char *url)
{
  char *filename = NULL;
  char* path;

#if 0
  fprintf (real_stderr,"XP_BackupFileName: %s\n", url);
#endif
  /* Must have a "file:" url and at least 1 character after the "//" */
  if (!url || !NET_IsLocalFileURL((char *) url))
    {
      return FALSE;
    }

  /* Extract file path from url: e.g. "/cl/foo/file.html" */
  path = NET_ParseURL (url, GET_PATH_PART);
  if (!path || XP_STRLEN(path) < 1) {
    return FALSE;
  }

  filename = (char *) XP_ALLOC ((XP_STRLEN (path) + 5) * sizeof (char));
  if (!filename)
    {
      return NULL;
    }

  /* Get filename but ignore "file://" */
  {
      char* filename2 = WH_FileName (path, xpURL);
      if (!filename2) return NULL;
      XP_STRCPY (filename, filename2);
      XP_FREE(filename2);
  }

  /* Add extension to the filename */
  XP_STRCAT (filename, "~");
#if 0
  fprintf (real_stderr,"XP_BackupFileName is: %s\n", filename);
#endif
  return filename;
}

static void
fe_EditorUpdateToolbarTextAttributes(
				     MWEditorContext* context,
				     EDT_CharacterData* cdata
) {
    ED_TextFormat attributes = cdata->values;
    int           size = cdata->iSize;
    fe_EditorContextData* editor = EDITOR_CONTEXT_DATA(context);

    /*FIXME*/
    /*
     *    This function should do some kind of check to see if the widgets
     *    need to be updated. Like:
    if (editor->last_size == size && editor->last_attributes == attributes)
	return;
     */
    if ((attributes & TF_FONT_SIZE) == 0) {
	size = 0; /* default */
    }
    
    fe_SizeGroupUpdate(editor, size);
    fe_TextAttributeGroupUpdate(editor, attributes);
}

void
fe_EditorUpdateToolbar(MWEditorContext* context, ED_TextFormat unused_for_now)
{
    fe_EditorContextData* editor = EDITOR_CONTEXT_DATA(context);
    ED_ElementType type;
    TagType nParagraphFormat;
    intn iTagType;

    if (CONTEXT_DATA(context)->show_toolbar_p) {
        /*
	 *    Cut/Copy/Paste updates. These are setup for the dependency
	 *    model used in the dialogs, whcih I would like to
	 *   introduce here.
	 */
        fe_editor_cut_update_cb(editor->toolbar_cut, (XtPointer)context, NULL);
	fe_editor_copy_update_cb(editor->toolbar_copy, (XtPointer)context,
				 NULL);
#if 0
	fe_editor_paste_update_cb(editor->toolbar_paste, (XtPointer)context, 
				  NULL);
#endif
#if 0
	fe_editor_browse_doc_update_cb(editor->toolbar_browse, 
				       (XtPointer)context, NULL);
	fe_editor_publish_update_cb(editor->toolbar_publish, 
				    (XtPointer)context, NULL);
#endif
    }

    if (CONTEXT_DATA(context)->show_character_toolbar_p) {

        type = EDT_GetCurrentElementType(context);
	switch (type) {
	case ED_ELEMENT_TEXT:
	case ED_ELEMENT_SELECTION:{
	    EDT_CharacterData* edt_cdata = EDT_GetCharacterData(context);
	    if (edt_cdata) {
	        fe_EditorUpdateToolbarTextAttributes(context, edt_cdata);
		EDT_FreeCharacterData(edt_cdata);
	    }
	}
	    break;
	default:
	    break;
	}
    }

    if (CONTEXT_DATA(context)->show_paragraph_toolbar_p) {

        fe_AlignGroupUpdate(editor, fe_EditorAlignGet(context));
	/* lifted from editor.cpp, sort of */
	nParagraphFormat = EDT_GetParagraphFormatting(context);
	fe_paragraph_style_toolbar_update(editor, nParagraphFormat);

	iTagType = 0;

	if (nParagraphFormat == P_LIST_ITEM) {
	    EDT_ListData* pData = EDT_GetListData(context);
	    if (pData) {
	        iTagType = pData->iTagType;
		EDT_FreeListData(pData);
	    }

	    fe_toggle_list_update(editor, iTagType);
	} else {
	    fe_toggle_list_update(editor, -1);
	}
    }

    fe_EditorNotifyBackgroundChanged(context);
}

Boolean
fe_EditorLineBreakCanInsert(MWContext* context)
{
   return (EDT_IsSelected(context) == FALSE);
}

/*
 *    type             description
 *    ED_BREAK_NORMAL  insert line break, ignore images.
 *    ED_BREAK_LEFT    Break so it passes the image on the left
 *    ED_BREAK_RIGHT   Break past the right image.
 *    ED_BREAK_BOTH    Break below image(s).
 */
void
fe_EditorLineBreak(MWContext* context, ED_BreakType type)
{
    if (!fe_EditorLineBreakCanInsert(context))
        return;
    EDT_InsertBreak(context, type);
    fe_EditorUpdateToolbar(context, 0);
}

void
fe_EditorNonBreakingSpace(MWContext* context)
{
    EDT_InsertNonbreakingSpace(context);
    fe_EditorUpdateToolbar(context, 0);
}

void
fe_EditorParagraphMarksSetState(MWContext* context, Boolean display)
{
    EDT_SetDisplayParagraphMarks(context, (Bool)display);
}

Boolean
fe_EditorParagraphMarksGetState(MWContext* context)
{
    return (Boolean)EDT_GetDisplayParagraphMarks(context);
}

char*
fe_EditorDefaultGetTemplate()
{
    return XFE_EDITOR_DEFAULT_DOCUMENT_TEMPLATE_URL;
}

void
fe_EditorDefaultGetColors(LO_Color*  bg_color,
			  LO_Color*  normal_color,
			  LO_Color*  link_color,
			  LO_Color*  active_color,
			  LO_Color*  followed_color)
{
    if (bg_color)
        *bg_color = lo_master_colors[LO_COLOR_BG];

    if (normal_color)
        *normal_color = lo_master_colors[LO_COLOR_FG];

    if (link_color)
        *link_color = lo_master_colors[LO_COLOR_LINK];

    if (active_color)
        *active_color = lo_master_colors[LO_COLOR_ALINK];

    if (followed_color)
        *followed_color = lo_master_colors[LO_COLOR_VLINK];
}

static char* fe_last_publish_location;
static char* fe_last_publish_username;
static char* fe_last_publish_password;

#define SAFE_STRDUP(x) ((x)? XP_STRDUP(x): NULL)

Boolean
fe_EditorDefaultGetLastPublishLocation(MWContext* context,
				       char** location,
				       char** username,
				       char** password)
{
    if (location)
        *location = SAFE_STRDUP(fe_last_publish_location);
    if (username)
        *username = SAFE_STRDUP(fe_last_publish_username);
    if (password)
        *password = SECMOZ_UnMungeString(fe_last_publish_password);

    return fe_last_publish_password != NULL;
}

void
fe_EditorDefaultSetLastPublishLocation(MWContext* context,
				       char* location,
				       char* username,
				       char* password)
{
    /* NOTE: this function is not MT safe */
    if (fe_last_publish_location)
        XP_FREE(fe_last_publish_location);
    if (fe_last_publish_username)
        XP_FREE(fe_last_publish_username);
    if (fe_last_publish_password)
        XP_FREE(fe_last_publish_password);

    fe_last_publish_location = NULL;
    fe_last_publish_username = NULL;
    fe_last_publish_password = NULL;

    if (location) {
        fe_last_publish_location = XP_STRDUP(location);

	if (username)
	    fe_last_publish_username = XP_STRDUP(username);

	if (password)
  	    fe_last_publish_password = SECMOZ_MungeString(password);
    }
}

void
fe_EditorPreferencesGetAutoSave(MWContext* context,
				Boolean*   enable,
				unsigned*  time)
{
    if (fe_globalPrefs.editor_autosave_period == 0) {
	*time = 0;
	*enable = FALSE;
    } else {
	*time = fe_globalPrefs.editor_autosave_period;
	*enable = TRUE;
    }
}

void
fe_EditorPreferencesSetAutoSave(MWContext* context,
				Boolean    enable,
				unsigned   time)
{
    struct fe_MWContext_cons *rest;

    if (enable) {
	fe_globalPrefs.editor_autosave_period = time;
    } else {
	fe_globalPrefs.editor_autosave_period = 0;
	time = 0;
    }

    /*
     *    Walk over the contexts, and tell the BE to reset the
     *    autosave.
     */
    for (rest = fe_all_MWContexts; rest; rest = rest->next) {
	if (rest->context->type == MWContextEditor) {
	    EDT_SetAutoSavePeriod(rest->context, time);
	}
    }
}

void
fe_EditorPreferencesGetLinksAndImages(MWContext* context,
				      Boolean*   links, 
				      Boolean*   images)
{
    if (links)
	*links = fe_globalPrefs.editor_maintain_links;
    if (images)
	*images = fe_globalPrefs.editor_keep_images;
}

void
fe_EditorPreferencesSetLinksAndImages(MWContext* context,
				      Boolean    links, 
				      Boolean    images)
{
    fe_globalPrefs.editor_maintain_links = links;
    fe_globalPrefs.editor_keep_images = images;
}

Boolean
fe_EditorPreferencesGetPublishLocation(MWContext* context,
				       char** location,
				       char** username,
				       char** password)
{
    if (location)
        *location = SAFE_STRDUP(fe_globalPrefs.editor_publish_location);
    if (username)
        *username = SAFE_STRDUP(fe_globalPrefs.editor_publish_username);
    if (password)
        *password =
	  SECMOZ_UnMungeString(fe_globalPrefs.editor_publish_password);

    return (fe_globalPrefs.editor_publish_password != NULL);
}

void
fe_EditorPreferencesSetPublishLocation(MWContext* context,
				       char* location,
				       char* username,
				       char* password)
{
    if (fe_globalPrefs.editor_publish_location)
        XP_FREE(fe_globalPrefs.editor_publish_location);
    if (fe_globalPrefs.editor_publish_username)
        XP_FREE(fe_globalPrefs.editor_publish_username);
    if (fe_globalPrefs.editor_publish_password)
        XP_FREE(fe_globalPrefs.editor_publish_password);

    fe_globalPrefs.editor_publish_location = NULL;
    fe_globalPrefs.editor_publish_username = NULL;
    fe_globalPrefs.editor_publish_password = NULL;

    if (location) {
        fe_globalPrefs.editor_publish_location = XP_STRDUP(location);

	if (username)
	    fe_globalPrefs.editor_publish_username = XP_STRDUP(username);

	if (password)
	    fe_globalPrefs.editor_publish_password
	      = SECMOZ_MungeString(password);
    }
    fe_globalPrefs.editor_save_publish_password = (password != NULL);
}

char*
fe_EditorPreferencesGetBrowseLocation(MWContext* context)
{
    return SAFE_STRDUP(fe_globalPrefs.editor_browse_location);
}

void
fe_EditorPreferencesSetBrowseLocation(MWContext* context, char* location)
{
    if (fe_globalPrefs.editor_browse_location)
        XP_FREE(fe_globalPrefs.editor_browse_location);
    fe_globalPrefs.editor_browse_location = NULL;

    if (location)
	fe_globalPrefs.editor_browse_location = XP_STRDUP(location);
}

char*
fe_EditorPreferencesGetAuthor(MWContext* context)
{
    if (fe_globalPrefs.editor_author_name)
	return fe_globalPrefs.editor_author_name;
    else
	return "";
}

void
fe_EditorPreferencesSetAuthor(MWContext* context, char* name)
{
    if (fe_globalPrefs.editor_author_name)
	XtFree(fe_globalPrefs.editor_author_name);
    fe_globalPrefs.editor_author_name = XtNewString(name);
}

char*
fe_EditorPreferencesGetTemplate(MWContext* context)
{
    if (fe_globalPrefs.editor_document_template)
	return fe_globalPrefs.editor_document_template;
    else
	return NULL;
}

void
fe_EditorPreferencesSetTemplate(MWContext* context, char* name)
{
    if (fe_globalPrefs.editor_document_template)
	XtFree(fe_globalPrefs.editor_document_template);
    fe_globalPrefs.editor_document_template = XtNewString(name);
}

void
fe_EditorPreferencesGetEditors(MWContext* context, char** html, char** image)
{
    if (html != NULL) {
	if (fe_globalPrefs.editor_html_editor)
	    *html = fe_globalPrefs.editor_html_editor;
	else
	    *html = NULL;
    }

    if (image != NULL) {
	if (fe_globalPrefs.editor_image_editor)
	    *image = fe_globalPrefs.editor_image_editor;
	else
	    *image = NULL;
    }
}

void
fe_EditorPreferencesSetEditors(MWContext* context, char* html, char* image)
{
    if (html != NULL) {
	if (fe_globalPrefs.editor_html_editor)
	    XtFree(fe_globalPrefs.editor_html_editor);
	fe_globalPrefs.editor_html_editor = XtNewString(html);
    }

    if (image != NULL) {
	if (fe_globalPrefs.editor_image_editor)
	    XtFree(fe_globalPrefs.editor_image_editor);
	fe_globalPrefs.editor_image_editor = XtNewString(image);
    }
}

Boolean
fe_EditorPreferencesGetColors(MWContext* context,
			      char*      background_image,
			      LO_Color*  bg_color,
			      LO_Color*  normal_color,
			      LO_Color*  link_color,
			      LO_Color*  active_color,
			      LO_Color*  followed_color)
{
    if (bg_color)
	*bg_color = fe_globalPrefs.editor_background_color;
    if (normal_color)
	*normal_color = fe_globalPrefs.editor_normal_color;
    if (link_color)
	*link_color = fe_globalPrefs.editor_link_color;
    if (active_color)
	*active_color = fe_globalPrefs.editor_active_color;
    if (followed_color)
	*followed_color = fe_globalPrefs.editor_followed_color;

    if (background_image) {
	if (fe_globalPrefs.editor_background_image)
	    strcpy(background_image,  fe_globalPrefs.editor_background_image);
	else
	    background_image[0] = '\0';
    }

    return fe_globalPrefs.editor_custom_colors;
}

void
fe_EditorPreferencesSetColors(MWContext* context,
			      char*      background_image,
			      LO_Color*  bg_color,
			      LO_Color*  normal_color,
			      LO_Color*  link_color,
			      LO_Color*  active_color,
			      LO_Color*  followed_color)
{
    XP_Bool set = FALSE;

    if (bg_color) {
	fe_globalPrefs.editor_background_color = *bg_color;
	set = TRUE;
    }
    if (normal_color) {
	fe_globalPrefs.editor_normal_color = *normal_color;
	set = TRUE;
    }
    if (link_color) {
	fe_globalPrefs.editor_link_color = *link_color;
	set = TRUE;
    }
    if (active_color) {
	fe_globalPrefs.editor_active_color = *active_color;
	set = TRUE;
    }
    if (followed_color) {
	fe_globalPrefs.editor_followed_color = *followed_color;
	set = TRUE;
    }

    fe_globalPrefs.editor_custom_colors = set;

    if (fe_globalPrefs.editor_background_image)
	XtFree(fe_globalPrefs.editor_background_image);
    if (background_image && background_image[0] != '\0')
	fe_globalPrefs.editor_background_image = XtNewString(background_image);
    else
	fe_globalPrefs.editor_background_image = NULL;
}

Boolean
fe_EditorDocumentGetImagesWithDocument(MWContext* context)
{
    Boolean keep;
    EDT_PageData* page_data = EDT_GetPageData(context);

    if (page_data == NULL) {
	fe_EditorPreferencesGetLinksAndImages(context, NULL, &keep);
    } else {
	keep = page_data->bKeepImagesWithDoc;
	EDT_FreePageData(page_data);
    }

    return keep;
}

void
fe_EditorDocumentSetImagesWithDocument(MWContext* context, Boolean keep)
{
    EDT_PageData* page_data = EDT_GetPageData(context);

    if (page_data != NULL) {
	page_data->bKeepImagesWithDoc = keep;
	EDT_SetPageData(context, page_data);
	EDT_FreePageData(page_data);
    }
}

char*
fe_EditorDocumentLocationGet(MWContext* context)
{
    History_entry* hist_ent;

    /* get the location */
    hist_ent = SHIST_GetCurrent(&context->hist);
    if (hist_ent)
	return hist_ent->address;
    else
	return NULL;
}

void
fe_EditorDocumentSetTitle(MWContext* context, char* name)
{
    EDT_PageData* page_data;

    page_data = EDT_GetPageData(context);
    if (page_data->pTitle)
	XP_FREE(page_data->pTitle);

    if (name && name[0] != '\0')
	page_data->pTitle = XP_STRDUP(name);
    else
	page_data->pTitle = NULL;

    EDT_SetPageData(context, page_data);

    EDT_FreePageData(page_data);
}

static char* fe_EditorDocumentGetTitle_value;

char*
fe_EditorDocumentGetTitle(MWContext* context)
{
    EDT_PageData* page_data;

    /* free the old */
    if (fe_EditorDocumentGetTitle_value)
	XtFree(fe_EditorDocumentGetTitle_value);

    fe_EditorDocumentGetTitle_value = NULL;

    /* get the title */
    page_data = EDT_GetPageData(context);
    if (page_data->pTitle)
	fe_EditorDocumentGetTitle_value = XtNewString(page_data->pTitle);

    EDT_FreePageData(page_data);

    return fe_EditorDocumentGetTitle_value;

}
void
fe_EditorDocumentSetMetaData(MWContext* context, char* name, char* value)
{
    EDT_MetaData *pData = EDT_NewMetaData();
    XP_Bool dirty = FALSE;
    if ( pData ) {
        pData->bHttpEquiv = FALSE;
        if ( name && XP_STRLEN(name) > 0 ) {
            pData->pName = XP_STRDUP(name);
            if ( value && XP_STRLEN(value) > 0 ) {
                pData->pContent = XP_STRDUP(value);
                EDT_SetMetaData(context, pData);
		dirty = TRUE;
            } else {
                /* (Don't really need to do this) */
		pData->pContent = NULL; 
                /* Remove the item */
		EDT_DeleteMetaData(context, pData);
		dirty = TRUE;
            }
	}
	EDT_FreeMetaData(pData);
    }
    if (dirty)
	EDT_SetDirtyFlag(context, dirty);
}

char*
fe_EditorDocumentGetMetaData(MWContext* context, char* name)
{
    EDT_MetaData* pData;
    int count;
    int i;

    /* lifted from winfe/edprops.cpp */
    count = EDT_MetaDataCount(context);
    for (i = 0; i < count; i++ ) {
        pData = EDT_GetMetaData(context, i);
        if (!pData->bHttpEquiv) {
            if (strcasecmp(pData->pName, name) == 0) {
		return pData->pContent;
	    }
	}
    }
    return NULL;
}

static char* fe_editor_known_meta_data_names[] = {
    "Author",
    "Description",
#if 0 /* this one need to end up in user variables */
    "Generator",
#endif
    "Last-Modified",
    "Created",
    "Classification",
    "Keywords",
    NULL
};

static Boolean
fe_is_known_meta_data_name(char* name)
{
    unsigned i;

    for (i = 0; fe_editor_known_meta_data_names[i] != NULL; i++) {
	if (strcasecmp(fe_editor_known_meta_data_names[i], name) == 0)
	    return TRUE;
    }
    return FALSE;
}

fe_NameValueItem*
fe_EditorDocumentGetHttpEquivMetaDataList(MWContext* context)
{
    int count = EDT_MetaDataCount(context);
    int i;
    EDT_MetaData* pData;
    fe_NameValueItem* items;
    unsigned nitems = 0;

    items = (fe_NameValueItem*)XtMalloc(sizeof(fe_NameValueItem)*(count+1));

    for (i = 0; i < count; i++) {
	pData = EDT_GetMetaData(context, i);

        if ((pData->bHttpEquiv)) {
	    items[nitems].name = XtNewString(pData->pName);
	    items[nitems].value = XtNewString(pData->pContent);
	    nitems++;
	}
    }
    items[nitems].name = NULL;
    items[nitems].value = NULL;

    items = (fe_NameValueItem*)XtRealloc((XtPointer)items,
					 sizeof(fe_NameValueItem)*(nitems+1));

    return items;
}

void
fe_EditorDocumentSetHttpEquivMetaDataList(MWContext* context,
					  fe_NameValueItem* list)
{
    fe_NameValueItem* old_list;
    int i;
    int j;
    EDT_MetaData *meta_data;
    XP_Bool dirty = FALSE;

    old_list = fe_EditorDocumentGetHttpEquivMetaDataList(context);

    /* first delete items no longer in set */
    for (i = 0; old_list[i].name != NULL; i++) {

	for (j = 0; list[j].name != NULL; j++) {
	    if (strcasecmp(old_list[i].name, list[j].name) == 0) /* match */
		break;
	}

	/* I'm sure all this thrashing of the heap can't be required ?? */
	if (list[j].name == NULL) { /* it's gone now */
	    meta_data = EDT_NewMetaData();
	    meta_data->pName = old_list[i].name;
	    meta_data->pContent = NULL;
	    EDT_DeleteMetaData(context, meta_data);
	    EDT_FreeMetaData(meta_data);
	    dirty = TRUE;
	}
    }

    /* now set the ones that have changed */
    for (j = 0; list[j].name != NULL; j++) {
	meta_data = EDT_NewMetaData();
	meta_data->pName = list[j].name;
	meta_data->pContent = list[j].value;
	meta_data->bHttpEquiv = TRUE;
	EDT_SetMetaData(context, meta_data);
	EDT_FreeMetaData(meta_data);
	dirty = TRUE;
    }
    if (dirty)
	EDT_SetDirtyFlag(context, dirty);
}

fe_NameValueItem*
fe_EditorDocumentGetAdvancedMetaDataList(MWContext* context)
{
    int count = EDT_MetaDataCount(context);
    int i;
    EDT_MetaData* pData;
    fe_NameValueItem* items;
    unsigned nitems = 0;

    items = (fe_NameValueItem*)XtMalloc(sizeof(fe_NameValueItem)*(count+1));

    for (i = 0; i < count; i++) {
	pData = EDT_GetMetaData(context, i);

	/* is it one of the ones handled in document general */
        if ((pData->bHttpEquiv) || fe_is_known_meta_data_name(pData->pName))
	    continue;
	
	items[nitems].name = XtNewString(pData->pName);
	items[nitems].value = XtNewString(pData->pContent);
	nitems++;
    }
    items[nitems].name = NULL;
    items[nitems].value = NULL;

    items = (fe_NameValueItem*)XtRealloc((XtPointer)items,
					 sizeof(fe_NameValueItem)*(nitems+1));

    return items;
}

void
fe_EditorDocumentSetAdvancedMetaDataList(MWContext* context,
					 fe_NameValueItem* list)
{
    fe_NameValueItem* old_list;
    int i;
    int j;
    EDT_MetaData *meta_data;
    XP_Bool dirty = FALSE;

    old_list = fe_EditorDocumentGetAdvancedMetaDataList(context);

    /* first delete items no longer in set */
    for (i = 0; old_list[i].name != NULL; i++) {

	for (j = 0; list[j].name != NULL; j++) {
	    if (strcasecmp(old_list[i].name, list[j].name) == 0) /* match */
		break;
	}

	/* I'm sure all this thrashing of the heap can't be required ?? */
	if (list[j].name == NULL) { /* it's gone now */
	    meta_data = EDT_NewMetaData();
	    meta_data->pName = old_list[i].name;
	    meta_data->pContent = NULL;
	    EDT_DeleteMetaData(context, meta_data);
	    EDT_FreeMetaData(meta_data);
	    dirty = TRUE;
	}
    }

    /* now set the ones that have changed */
    for (j = 0; list[j].name != NULL; j++) {
	meta_data = EDT_NewMetaData();
	meta_data->pName = list[j].name;
	meta_data->pContent = list[j].value;
	EDT_SetMetaData(context, meta_data);
	EDT_FreeMetaData(meta_data);
	dirty = TRUE;
    }
    if (dirty)
	EDT_SetDirtyFlag(context, dirty);
}

Boolean
fe_EditorDocumentGetColors(MWContext* context,
			   char*      background_image,
			   LO_Color*  bg_color,
			   LO_Color*  normal_color,
			   LO_Color*  link_color,
			   LO_Color*  active_color,
			   LO_Color*  followed_color)
{
    EDT_PageData* page_data = EDT_GetPageData(context);
    Boolean set = FALSE;

    fe_EditorDefaultGetColors(bg_color,
			      normal_color,
			      link_color, 
			      active_color,
			      followed_color);

    if (background_image != NULL) {
        if (page_data->pBackgroundImage != NULL) {
	    strcpy(background_image, page_data->pBackgroundImage);
	} else {
	    background_image[0] = '\0';
	}
    }

    if (bg_color) {
        if (page_data->pColorBackground) {
	    *bg_color = *page_data->pColorBackground;
	    set = TRUE;
	}
    }

    if (normal_color) {
        if (page_data->pColorText) {
	    *normal_color = *page_data->pColorText;
	    set = TRUE;
	}
    }

    if (link_color) {
        if (page_data->pColorLink) {
	    *link_color = *page_data->pColorLink;
	    set = TRUE;
	}
    }

    if (active_color) {
        if (page_data->pColorActiveLink) {
	    *active_color = *page_data->pColorActiveLink;
	    set = TRUE;
	}
    }

    if (followed_color) {
        if (page_data->pColorFollowedLink) {
	    *followed_color = *page_data->pColorFollowedLink;
	    set = TRUE;
	}
    }

    EDT_FreePageData(page_data);

    return set;
}

void
fe_EditorDocumentSetColors(MWContext* context,
			   char*      background_image,
			   LO_Color*  bg_color,
			   LO_Color*  normal_color,
			   LO_Color*  link_color,
			   LO_Color*  active_color,
			   LO_Color*  followed_color)
{
    EDT_PageData  save_data;
    EDT_PageData* page_data;
    
    if (background_image != NULL && background_image[0] == '\0')
	background_image = NULL;
    
    page_data = EDT_GetPageData(context);
    memcpy(&save_data, page_data, sizeof(EDT_PageData));
    
    page_data->pBackgroundImage = background_image;
    page_data->pColorBackground = bg_color;
    page_data->pColorText = normal_color;
    page_data->pColorLink = link_color;
    page_data->pColorActiveLink = active_color;
    page_data->pColorFollowedLink = followed_color;
    
    EDT_SetPageData(context, page_data);
    memcpy(page_data, &save_data, sizeof(EDT_PageData));
    EDT_FreePageData(page_data);
}

typedef struct fe_EditorCommandDescription {
    intn  id;
    char* name;
} fe_EditorCommandDescription;

/*
 *    Yuck lifted from editor.h
 */
#define kNullCommandID 0
#define kTypingCommandID 1
#define kAddTextCommandID 2
#define kDeleteTextCommandID 3
#define kCutCommandID 4
#define kPasteTextCommandID 5
#define kPasteHTMLCommandID 6
#define kPasteHREFCommandID 7
#define kChangeAttributesCommandID 8
#define kIndentCommandID 9
#define kParagraphAlignCommandID 10
#define kMorphContainerCommandID 11
#define kInsertHorizRuleCommandID 12
#define kSetHorizRuleDataCommandID 13
#define kInsertImageCommandID 14
#define kSetImageDataCommandID 15
#define kInsertBreakCommandID 16
#define kChangePageDataCommandID 17
#define kSetMetaDataCommandID 18
#define kDeleteMetaDataCommandID 19
#define kInsertTargetCommandID 20
#define kSetTargetDataCommandID 21
#define kInsertUnknownTagCommandID 22
#define kSetUnknownTagDataCommandID 23
#define kGroupOfChangesCommandID 24
#define kSetListDataCommandID 25

#define kInsertTableCommandID 26
#define kDeleteTableCommandID 27
#define kSetTableDataCommandID 28

#define kInsertTableCaptionCommandID 29
#define kSetTableCaptionDataCommandID 30
#define kDeleteTableCaptionCommandID 31

#define kInsertTableRowCommandID 32
#define kSetTableRowDataCommandID 33
#define kDeleteTableRowCommandID 34

#define kInsertTableColumnCommandID 35
#define kSetTableCellDataCommandID 36
#define kDeleteTableColumnCommandID 37

#define kInsertTableCellCommandID 38
#define kDeleteTableCellCommandID 39

static fe_EditorCommandDescription fe_editor_commands[] = {
    { kNullCommandID,             "Null"              },
    { kTypingCommandID,           "Typing"            },
    { kAddTextCommandID,          "AddText"           },
    { kDeleteTextCommandID,       "DeleteText"        },
    { kCutCommandID,              "Cut"               },
    { kPasteTextCommandID,        "PasteText"         },
    { kPasteHTMLCommandID,        "PasteHTML"         },
    { kPasteHREFCommandID,        "PasteHREF"         },
    { kChangeAttributesCommandID, "ChangeAttributes"  },
    { kIndentCommandID,           "Indent"            },
    { kParagraphAlignCommandID,   "ParagraphAlign"    },
    { kMorphContainerCommandID,   "MorphContainer"    },
    { kInsertHorizRuleCommandID,  "InsertHorizRule"   },
    { kSetHorizRuleDataCommandID, "SetHorizRuleData"  },
    { kInsertImageCommandID,      "InsertImage"       },
    { kSetImageDataCommandID,     "SetImageData"      },
    { kInsertBreakCommandID,      "InsertBreak"       },
    { kChangePageDataCommandID,   "ChangePageData"    },
    { kSetMetaDataCommandID,      "SetMetaData"       },
    { kDeleteMetaDataCommandID,   "DeleteMetaData"    },
    { kInsertTargetCommandID,     "InsertTarget"      },
    { kSetTargetDataCommandID,    "SetTargetData"     },
    { kInsertUnknownTagCommandID, "InsertUnknownTag"  },
    { kSetUnknownTagDataCommandID,"SetUnknownTagData" },
    { kGroupOfChangesCommandID,   "GroupOfChanges"    },
    { kSetListDataCommandID,      "SetListData"       },
    { kInsertTableCommandID,      "InsertTable"       },
    { kDeleteTableCommandID,      "DeleteTable"       },
    { kSetTableDataCommandID,     "SetTableData"      },
    { kInsertTableCaptionCommandID,  "InsertTableCaption"  },
    { kSetTableCaptionDataCommandID, "SetTableCaptionData" },
    { kDeleteTableCaptionCommandID,  "DeleteTableCaption"  },
    { kInsertTableRowCommandID,    "InsertTableRow"   },
    { kSetTableRowDataCommandID,   "SetTableRowData"  },
    { kDeleteTableRowCommandID,    "DeleteTableRow"   },
    { kInsertTableColumnCommandID, "InsertTableColumn" },
    { kSetTableCellDataCommandID,  "SetTableCellData"  },
    { kDeleteTableColumnCommandID, "DeleteTableColumn" },
    { kInsertTableCellCommandID,   "InsertTableCell"  },
    { kDeleteTableCellCommandID,   "DeleteTableCell"  },

    { -1, NULL }
};

static XmString
resource_motif_string(Widget widget, char* name)
{
  XtResource resource;
  XmString result = 0;
  
  resource.resource_name = XmNlabelString;
  resource.resource_class = XmCXmString;
  resource.resource_type = XmRXmString;
  resource.resource_size = sizeof (XmString);
  resource.resource_offset = 0;
  resource.default_type = XtRImmediate;
  resource.default_addr = 0;

  XtGetSubresources(widget, (XtPointer)&result, name,
		     "UndoItem", &resource, 1, NULL, 0);
  if (!result)
      result = XmStringCreateLocalized(name);

  return result;
}

static XmString string_cache;

static XmString
fe_editor_undo_get_label(MWContext* context, intn id, Boolean is_undo)
{
    char     buf[64];
    XmString string;
    char* name;
    int n;

    name = NULL;
    for (n = 0; fe_editor_commands[n].name != NULL; n++) {
	if (fe_editor_commands[n].id == id) {
	    name = fe_editor_commands[n].name;
	    break;
	}
    }

    /*
     *    For ids we don't know yet, just show "Undo" or "Redo"
     */
    if (!name)
	name = "";

    /*
     *    Look for it in the cache here!!!!
     */
    /*FIXME*/
    if (string_cache)
	XmStringFree(string_cache);

    if (is_undo)
	XP_STRCPY(buf, "undo");
    else
	XP_STRCPY(buf, "redo");
    XP_STRCAT(buf, name);

    /*
     *    Resource the string.
     */
    /*
     *    Fuck these stupid Xt resource shit. Why the fuck is
     *    is it so fucking hard to get a string! Like, what,
     *    I'd never want to do that. Dickheads.
     */
    string = resource_motif_string(CONTEXT_DATA(context)->menubar, buf);
    string_cache = string;

    return string;
}

Boolean
fe_EditorCanUndo(MWContext* context, XmString* label_return)
{
    intn command_id = EDT_GetUndoCommandID(context, 0);

    if (label_return) {
	*label_return = fe_editor_undo_get_label(context,
						 command_id,
						 True);
    }
    
    if (command_id != CEDITCOMMAND_ID_NULL)
	return True;
    else
	return FALSE;
}

Boolean
fe_EditorCanRedo(MWContext* context, XmString* label_return)
{
    intn command_id = EDT_GetRedoCommandID(context, 0);
    
    if (label_return) {
	*label_return = fe_editor_undo_get_label(context, 
						 command_id,
						 False);
    }
    
    if (command_id != CEDITCOMMAND_ID_NULL)
	return True;
    else
	return FALSE;
}

void
fe_EditorUndo(MWContext* context)
{
    if (EDT_GetUndoCommandID(context, 0) != CEDITCOMMAND_ID_NULL) {
	EDT_Undo(context);
	fe_EditorUpdateToolbar(context, 0);
    } else {
	fe_Bell(context);
    }
}

void
fe_EditorRedo(MWContext* context)
{
    if (EDT_GetRedoCommandID(context, 0) != CEDITCOMMAND_ID_NULL) {
	EDT_Redo(context);
	fe_EditorUpdateToolbar(context, 0);
    } else {
	fe_Bell(context);
    }
}

Boolean
fe_EditorCanCut(MWContext* context)
{
    return (EDT_COP_OK == EDT_CanCut(context, FALSE));
}

Boolean
fe_EditorCanCopy(MWContext* context)
{
    return (EDT_COP_OK == EDT_CanCopy(context, TRUE));
}

static fe_editor_ccp_clipboard_initialized;

static char fe_editor_html_name[] = "NETSCAPE_HTML";
static char fe_editor_text_name[] = "STRING";
static char fe_editor_app_name[] = "MOZILLA";

#define XFE_CCP_TEXT (fe_editor_text_name)
#define XFE_CCP_HTML (fe_editor_html_name)
#define XFE_CCP_NAME (fe_editor_app_name)

#if 0
/*
 *    Cannot get this to work.
 */
static void
fe_property_notify_eh(Widget w, XtPointer closure, XEvent *ev, Boolean *cont)
{
    MWContext* context = (MWContext *)closure;
    Display*   display = XtDisplay(CONTEXT_WIDGET(context));
    Atom       cb_atom = XmInternAtom(display, "CLIPBOARD", False);
    Atom       string_atom = XmInternAtom(display, XFE_CCP_TEXT, False);
    Atom       html_atom = XmInternAtom(display, XFE_CCP_HTML, False);
    Atom       long_string_atom = XmInternAtom(display, "_MOTIF_CLIP_FORMAT_" "STRING", False);
    Atom       long_html_atom = XmInternAtom(display, "_MOTIF_CLIP_FORMAT_" "NETSCAPE_HTML", False);
    fe_EditorContextData* editor;

#if 0
    if (ev->xproperty.atom == XA_CUT_BUFFER0) {
	editor = EDITOR_CONTEXT_DATA(context);
	fe_editor_paste_update_cb(editor->toolbar_paste,
				  (XtPointer)context, NULL);
    }
#endif

#if 0
    if (ev->xproperty.atom ==  XmInternAtom (dpy, "CLIPBOARD", False)XA_CUT_BUFFER0)
#endif
}
#endif

static void
fe_editor_initalize_clipboard(MWContext* context)
{
    Widget   mainw;
    Display* display;

    if (!fe_editor_ccp_clipboard_initialized) {
	mainw = CONTEXT_WIDGET(context);
	display = XtDisplay(mainw);

	XmClipboardRegisterFormat(display, XFE_CCP_HTML, 8);
	fe_editor_ccp_clipboard_initialized = TRUE;
    }
}

Boolean
fe_EditorCanPasteHtml(MWContext* context, int32* r_length)
{
    Widget   mainw = CONTEXT_WIDGET(context);
    Display* display = XtDisplay(mainw);
    Window   window = XtWindow(mainw);
    int      cb_result;
    unsigned long length;

    fe_editor_initalize_clipboard(context);

    cb_result = XmClipboardInquireLength(display, window,
					 XFE_CCP_HTML, &length);

    if (cb_result ==  ClipboardSuccess && length > 0) {
	*r_length = length;
	return TRUE;
    } else {
	*r_length = 0;
	return FALSE;
    }
}

Boolean
fe_EditorCanPasteText(MWContext* context, int32* r_length)
{
    Widget   mainw = CONTEXT_WIDGET(context);
    Display* display = XtDisplay(mainw);
    Window   window = XtWindow(mainw);
    int      cb_result;
    unsigned long length;

    cb_result = XmClipboardInquireLength(display, window,
					 XFE_CCP_TEXT, &length);

    if (cb_result ==  ClipboardSuccess && length > 0) {
	*r_length = length;
	return TRUE;
    } else {
	*r_length = 0;
	return FALSE;
    }
}

Boolean
fe_EditorCanPasteLocal(MWContext* context, int32* r_length)
{
    struct fe_MWContext_cons *rest;

    fe_editor_initalize_clipboard(context);

    /*
     *    The browser maintains it's own internal clipboard.
     *    Clear it.
     */
    for (rest = fe_all_MWContexts; rest; rest = rest->next) {
	if (CONTEXT_DATA(rest->context)->clipboard != NULL) {
	    *r_length = strlen(CONTEXT_DATA(rest->context)->clipboard);
	    return TRUE;
	}
    }
    return FALSE;
}

Boolean
fe_EditorCanPaste(MWContext* context)
{
    int32 len;

    /*
     *    We have to check if we can do a local paste first,
     *    because we can hang the server if we try to re-own
     *    the selection. This is all very hokey. I need to
     *    talk to jamie.
     */
    if (
	fe_EditorCanPasteLocal(context, &len)
	||
	fe_EditorCanPasteHtml(context, &len)
	|| 
	fe_EditorCanPasteText(context, &len))
    {
	return TRUE;
    } else {
	return FALSE;
    }
	
}

static int
fe_clipboard_copy_work(MWContext* context,
		       char* cut_text, unsigned long cut_text_len,
		       char* cut_html, unsigned long cut_html_len,
		       Time timestamp) {
    Widget   mainw = CONTEXT_WIDGET(context);
    Display* display = XtDisplay(mainw);
    Window   window = XtWindow(mainw);
    int      cb_result;
    long     item_id;
    XmString label;
    long private_id = 0;
    long data_id;
    struct fe_MWContext_cons *rest;

    fe_editor_initalize_clipboard(context);

    /*
     *    The browser maintains it's own internal clipboard.
     *    Clear it.
     */
    for (rest = fe_all_MWContexts; rest; rest = rest->next) {
	if (CONTEXT_DATA(rest->context)->clipboard != NULL) {
	    fe_DisownSelection(rest->context, timestamp, TRUE);
	}
    }
    
    label = XmStringCreateLocalized(XFE_CCP_NAME);
    
    cb_result = XmClipboardStartCopy(display, window,
				     label,
				     timestamp,
				     mainw,
				     NULL,
				     &item_id);

    XmStringFree(label);

    if (cb_result != ClipboardSuccess)
	return cb_result;
    
    if (cut_text && cut_text_len > 0) {
	cb_result = XmClipboardCopy(display, window,
				    item_id,
				    XFE_CCP_TEXT,
				    (XtPointer)cut_text,
				    cut_text_len,
				    private_id,
				    &data_id);

	if (cb_result != ClipboardSuccess)
	    return cb_result;
    }
				     
    if (cut_html && cut_html_len > 0) {
	cb_result = XmClipboardCopy(display, window,
				    item_id,
				    XFE_CCP_HTML,
				    (XtPointer)cut_html,
				    cut_html_len,
				    private_id,
				    &data_id);

	if (cb_result != ClipboardSuccess)
	    return cb_result;
    }
				     
    return XmClipboardEndCopy(display, window, item_id);
}

void
fe_editor_report_copy_error(MWContext* context, EDT_ClipboardResult code)
{
    char* msg;
    int id;

    switch (code) {
    case EDT_COP_DOCUMENT_BUSY: /* IDS_DOCUMENT_BUSY */
	id = XFE_EDITOR_COPY_DOCUMENT_BUSY;
	/* Cannot copy or cut at this time, try again later. */
	break;
    case EDT_COP_SELECTION_EMPTY: /* IDS_SELECTION_EMPTY */
	id = XFE_EDITOR_COPY_SELECTION_EMPTY;
	/* Nothing is selected. */
	break;
    case EDT_COP_SELECTION_CROSSES_TABLE_DATA_CELL:
	id = XFE_EDITOR_COPY_SELECTION_CROSSES_TABLE_DATA_CELL;
	/* The selection includes a table cell boundary. */
	/* Deleting and copying are not allowed.         */
	break;
    default:
	return;
    }

    msg = XP_GetString(id);
    FE_Alert(context, msg);
}

Boolean
fe_EditorCut(MWContext* context, Time timestamp)
{
    char* cut_text;
    int32 cut_text_len;
    char* cut_html;
    int32 cut_html_len;
    EDT_ClipboardResult cut_result;
    int cb_result;

    if (!fe_EditorCanCut(context)) {
	fe_Bell(context);
	return FALSE;
    }
    
    /* cut/copy from edt to Xm clipboard */
    cut_result = EDT_CutSelection(context, 
				  &cut_text, &cut_text_len,
				  &cut_html, &cut_html_len);
    if (cut_result != EDT_COP_OK) {
	fe_editor_report_copy_error(context, cut_result);
	return FALSE;
    }

    if ((!cut_text || cut_text_len <= 0) && (!cut_html || cut_html_len <= 0))
	return FALSE;
    
    cb_result = fe_clipboard_copy_work(context,
				       cut_text, cut_text_len,
				       cut_html, cut_html_len,
				       timestamp);

    fe_EditorUpdateToolbar(context, 0);

    if (cb_result != ClipboardSuccess)
	return FALSE;

    return TRUE;
}

Boolean
fe_EditorCopy(MWContext* context, Time timestamp)
{
    char* cut_text;
    int32 cut_text_len;
    char* cut_html;
    int32 cut_html_len;
    EDT_ClipboardResult cut_result;
    int cb_result;

    if (!fe_EditorCanCopy(context)) {
	fe_Bell(context);
	return FALSE;
    }
    
    cut_result = EDT_CopySelection(context, 
				  &cut_text, &cut_text_len,
				  &cut_html, &cut_html_len);
    if (cut_result != EDT_COP_OK) {
	fe_editor_report_copy_error(context, cut_result);
	return FALSE;
    }

    if ((!cut_text || cut_text_len <= 0) && (!cut_html || cut_html_len <= 0))
	return FALSE;
    
    cb_result = fe_clipboard_copy_work(context,
				       cut_text, cut_text_len,
				       cut_html, cut_html_len,
				       timestamp);

    fe_EditorUpdateToolbar(context, 0);

    if (cb_result != ClipboardSuccess)
	return FALSE;
    return TRUE;
}

int
fe_clipboard_retrieve_work(MWContext* context,
			   char* name,
			   char* buf, unsigned long* buf_len_a,
			   Time timestamp) {

    Widget   mainw = CONTEXT_WIDGET(context);
    Display* display = XtDisplay(mainw);
    Window   window = XtWindow(mainw);
    int      cb_result;
    long private_id;

    fe_editor_initalize_clipboard(context);

    cb_result = XmClipboardStartRetrieve(display, window, timestamp);

    if (cb_result != ClipboardSuccess)
	return cb_result;
    
    cb_result = XmClipboardRetrieve(display, window,
				    name,
				    (XtPointer)buf,
				    *buf_len_a,
				    buf_len_a,
				    &private_id);

    if (cb_result != ClipboardSuccess)
	return cb_result;
    
    return XmClipboardEndRetrieve(display, window);
}

static char fe_editor_local_name[] = "LOCAL";

#define XFE_CCP_LOCAL (fe_editor_local_name)

Boolean
fe_EditorPaste(MWContext* context, Time timestamp)
{
    int32   length;
    char*   clip_data = NULL;
    char*   clip_name = NULL; /* none */
    int     cb_result;
    Boolean rv = FALSE;
    Widget   mainw = CONTEXT_WIDGET(context);
    Display* display = XtDisplay(mainw);
    Window   window = XtWindow(mainw);
    struct fe_MWContext_cons *rest;

    /*
     *    The browser maintains it's own internal clipboard.
     *    There seems to be a good reason for it. Anyway,
     *    check on that clipboard list first.
     */
    for (rest = fe_all_MWContexts; rest; rest = rest->next) {
	if ((clip_data = CONTEXT_DATA(rest->context)->clipboard)) {
	    clip_name = XFE_CCP_LOCAL;
	    break;
	}
    }
    
    if (!clip_data) { /* go look on real CLIPBOARD */

	/*
	 *    Need to lock the clipboard while we ask if there is something
	 *    and subsequently get it.
	 */
	XmClipboardLock(display, window);
	
	if (fe_EditorCanPasteHtml(context, &length)) {
	    clip_data = XP_ALLOC(length);
	    clip_name = XFE_CCP_HTML;
	} else if (fe_EditorCanPasteText(context, &length)) {
	    clip_data = XP_ALLOC(length);
	    clip_name = XFE_CCP_TEXT;
	}

	if (clip_data) {

	    cb_result = fe_clipboard_retrieve_work(context,
						   clip_name,
						   clip_data, &length,
						   timestamp);
	    
	    if (cb_result == ClipboardSuccess) {
		clip_data[length] = '\0';
	    } else {
		XP_FREE(clip_data);
		clip_data = NULL;
		clip_name = NULL;
	    }
	}

	XmClipboardUnlock(display, window, TRUE);
    } 

    if (clip_name == XFE_CCP_HTML) {
	rv = (EDT_PasteHTML(context, clip_data) == EDT_COP_OK);
	XP_FREE(clip_data);
    } else if (clip_name == XFE_CCP_TEXT) {
	rv = (EDT_PasteText(context, clip_data) == EDT_COP_OK);
	XP_FREE(clip_data);
    } else if (clip_name == XFE_CCP_LOCAL) {
	rv = (EDT_PasteText(context, clip_data) == EDT_COP_OK);
    } else {
	fe_Bell(context);
    }

    fe_EditorUpdateToolbar(context, 0);

    return rv;
}

void
fe_EditorSelectAll(MWContext* context)
{
    EDT_SelectAll(context);  
    fe_EditorUpdateToolbar(context, 0);
}

void
fe_EditorDeleteItem(MWContext* context)
{
    EDT_ClipboardResult result;

    if ((result = EDT_DeleteChar(context)) != EDT_COP_OK) {
	fe_editor_report_copy_error(context, result);
    }
    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorHorizontalRulePropertiesCanDo(MWContext* context)
{
    ED_ElementType type = EDT_GetCurrentElementType(context);

    if  (type == ED_ELEMENT_HRULE)
	return TRUE;
    else
	return FALSE;
}

void
fe_EditorHorizontalRulePropertiesSet(MWContext* context, EDT_HorizRuleData* data)
{
    if (fe_EditorHorizontalRulePropertiesCanDo(context)) {
	EDT_SetHorizRuleData(context, data);
    } else {
	EDT_InsertHorizRule(context, data);
    }
}

void
fe_EditorHorizontalRulePropertiesGet(MWContext* context, EDT_HorizRuleData* data)
{
    EDT_HorizRuleData* foo;

    if (fe_EditorHorizontalRulePropertiesCanDo(context)) {
	foo = EDT_GetHorizRuleData(context);
	memcpy(data, foo, sizeof(EDT_HorizRuleData));
	EDT_FreeHorizRuleData(foo);
    } else {
	data->size = 2;
	data->iWidth = 100;
	data->bWidthPercent = TRUE;
	data->align = ED_ALIGN_CENTER;
	data->bNoShade = FALSE;
    }
}

void
fe_EditorRefresh(MWContext* context)
{
    EDT_RefreshLayout(context);    
}


CB_STATIC void
fe_editor_refresh_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    fe_EditorRefresh((MWContext*)closure);
}

void
fe_EditorHrefInsert(MWContext* context, char* p_text, char* p_url)
{
    char* text;
    char* url;

    if (!EDT_CanSetHREF(context)) { /* should not exist already */

	if (EDT_IS_NEW_DOCUMENT(context)) {
	    if (!FE_CheckAndSaveDocument(context))
		return;
	}
	
	if (p_url && p_url[0] != '\0')
	    url = XP_STRDUP(p_url);
	else
	    return; /* no url specified */

	if (p_text && p_text[0] != '\0')
	    text = XP_STRDUP(p_text);
	else
	    text = XP_STRDUP(p_url);

	
	EDT_PasteHREF(context, &url, &text, 1);
    }
}

Boolean
fe_EditorGetHref(MWContext *context) {
    char*         p;

    p = (char *) LO_GetSelectionText(context);
    if (p == NULL) return False;
    else
	return EDT_GetHREF(context);
}


Boolean
fe_EditorHrefGet(MWContext* context, char** text, char** url)
{
    EDT_HREFData* href_data = EDT_GetHREFData(context);
    Bool          is_selected = EDT_IsSelected(context);
    char*         p;

    p = (char *) LO_GetSelectionText(context);

    if (is_selected && p != NULL) {
	*text = XtNewString(p);
	XP_FREE(p);
    } else if (p == NULL) {   /* this fixes #27762 */
	*text = NULL;
    } else if ((p = EDT_GetHREFText(context)) != NULL) { 
			/* charlie's new get the text of a link thing */
	*text = p;
    } else {
	*text = NULL;
    }
    
    if (href_data != NULL && (p = href_data->pURL) != NULL) {
	*url = XtNewString(p);
    } else {
	*url = NULL;
    }

    if (href_data != NULL)
	EDT_FreeHREFData(href_data);

    return EDT_CanSetHREF(context);
}

void
fe_EditorHrefSetUrl(MWContext* context, char* url)
{
    EDT_HREFData* href_data;

    if (EDT_CanSetHREF(context)) {

	href_data = EDT_GetHREFData(context);

	if (!href_data)
	    href_data = EDT_NewHREFData();

	if (href_data->pURL)
	    XP_FREE(href_data->pURL);
	
	if (url)
	    href_data->pURL = XP_STRDUP(url);
	else
	    href_data->pURL = NULL; /* clear the href */

	EDT_SetHREFData(context, href_data);

	EDT_FreeHREFData(href_data);
    }
}

void
fe_EditorHrefClearUrl(MWContext* context)
{
    fe_EditorHrefSetUrl(context, NULL);
}

void
fe_EditorRemoveLinks(MWContext* context)
{
    /*
     *    Remove a single link or all links within selected region.
     */
    if (EDT_CanSetHREF(context)) {
        EDT_SetHREF(context, NULL);
    }
}

static void
fe_editor_link_browse_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    MWContext* context = (MWContext*)closure;

    EDT_HREFData* href_data = EDT_GetHREFData(context);
    
    if (href_data != NULL) {
	if (href_data->pURL != NULL) {
	    fe_GetURLInBrowser(context,
			       NET_CreateURLStruct(href_data->pURL, FALSE));
	}
	EDT_FreeHREFData(href_data);
    }
}

static void
fe_editor_link_edit_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    MWContext* context = (MWContext*)closure;
    EDT_HREFData* href_data = EDT_GetHREFData(context);
    
    if (href_data != NULL) {
	if (href_data->pURL != NULL) {
	    fe_EditorNew(context, href_data->pURL);
	}
	EDT_FreeHREFData(href_data);
    }
}

static void
fe_editor_link_add_bookmark_cb(Widget widget, XtPointer closure,
			  XtPointer call_data)
{
    MWContext* context = (MWContext*)closure;
    EDT_HREFData* href_data = EDT_GetHREFData(context);
    
    if (href_data != NULL) {
	if (href_data->pURL != NULL) {
	    fe_AddToBookmark(context, 0,
			     NET_CreateURLStruct(href_data->pURL, FALSE), 0);
	}
	EDT_FreeHREFData(href_data);
    }
}

static Time
fe_getTimeFromEvent(MWContext* context, XEvent* event)
{
  Time time;
  time = (event && (event->type == KeyPress ||
		    event->type == KeyRelease)
	       ? event->xkey.time :
	       event && (event->type == ButtonPress ||
			 event->type == ButtonRelease)
	       ? event->xbutton.time :
	       XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context))));
  return time;
}

static void
fe_editor_link_copy_link_cb(Widget widget, XtPointer closure,
			    XtPointer cd)
{
    MWContext* context = (MWContext*)closure;
    EDT_HREFData* href_data = EDT_GetHREFData(context);
    XmPushButtonCallbackStruct* cb_data = (XmPushButtonCallbackStruct*)cd;
    
    if (href_data != NULL) {
	if (href_data->pURL != NULL) {
	    fe_clipboard_copy_work(context,
			   NULL, 0,
			   href_data->pURL, strlen(href_data->pURL),
			   fe_getTimeFromEvent(context, cb_data->event));
	}
	EDT_FreeHREFData(href_data);
    }
}

#if 0
Boolean
fe_EditorImageDataGet(MWContext* context, EDT_ImageData* idata)
{
    EDT_ImageData* foo;

    if (EDT_GetCurrentElementType(context) != ED_ELEMENT_IMAGE)
	return FALSE;

    foo = EDT_GetImageData(context);
    memcpy(idata, foo, sizeof(EDT_ImageData));

    /*
     *    Caller owns this memory.
     */
#define SAFE_DUP(x) if (foo->(x)) idata->(x) = XP_STRDUP(foo->(x))
    SAFE_DUP(pUseMap);
    SAFE_DUP(pSrc);
    SAFE_DUP(pLowSrc);
    SAFE_DUP(pName);
    SAFE_DUP(pAlt);
#undef SAFE_DUP

    EDT_FreeImageData(foo);

    return TRUE;
}

void
fe_EditorImageDataSet(MWContext* context, EDT_ImageData* idata, Boolean copy)
{
    EDT_ImageData* foo;
    EDT_ImageData  save;

    if (EDT_GetCurrentElementType(context) != ED_ELEMENT_IMAGE)
	return;

    foo = EDT_GetImageData(context);
    memcpy(&save, foo, sizeof(EDT_ImageData));

    

}

void
fe_EditorImageInsert(MWContext* context, EDT_ImageData* idata)
{
}
#endif

Boolean
fe_EditorPropertiesDialogCanDo(MWContext* context,
			       fe_EditorPropertiesDialogType d_type)
{
    ED_ElementType e_type;

    if (
	d_type == XFE_EDITOR_PROPERTIES_IMAGE_INSERT
	||
	d_type == XFE_EDITOR_PROPERTIES_LINK_INSERT
	||
	d_type == XFE_EDITOR_PROPERTIES_CHARACTER
	||
	d_type == XFE_EDITOR_PROPERTIES_PARAGRAPH
	||
	d_type == XFE_EDITOR_PROPERTIES_TABLE
	|| 
	d_type == XFE_EDITOR_PROPERTIES_DOCUMENT
    )
	return TRUE; /* always ok to do */

    e_type = EDT_GetCurrentElementType(context);

    if (d_type == XFE_EDITOR_PROPERTIES_IMAGE) {
	if (e_type == ED_ELEMENT_IMAGE)
	    return TRUE;
	else
	    return FALSE;

    } else if (d_type == XFE_EDITOR_PROPERTIES_HRULE) {
	if (e_type == ED_ELEMENT_HRULE)
	    return TRUE;
	else
	    return FALSE;

    } else if (d_type == XFE_EDITOR_PROPERTIES_HTML_TAG) {
	if (e_type == ED_ELEMENT_UNKNOWN_TAG)
	    return TRUE;
	else
	    return FALSE;

    } else if (d_type == XFE_EDITOR_PROPERTIES_TARGET) {
	if (e_type == ED_ELEMENT_TARGET)
	    return TRUE;
	else
	    return FALSE;

    } else if (d_type == XFE_EDITOR_PROPERTIES_LINK) {
	if (EDT_GetHREF(context))
	    return TRUE;
	else
	    return FALSE;
    } 
    return FALSE;
}

/*
 *    This stuff is so fucked.
 *
 */
void
fe_EditorParagraphPropertiesSetAll(MWContext*    context,
				   TagType       paragragh_type,
				   EDT_ListData* list_data,
				   ED_Alignment  align)
{
    TagType       old_paragraph_type = EDT_GetParagraphFormatting(context);
    EDT_ListData* old_list_data = EDT_GetListData(context);
    Boolean       do_set;

    EDT_BeginBatchChanges(context);

    /*
     *    Windows has this kludge, so...
     */
    if (align == ED_ALIGN_LEFT)
	align = ED_ALIGN_DEFAULT;

    EDT_SetParagraphAlign(context, align);

    /*
     *    If there was a list before, and now there is none,
     *    remove the list.
     */
    if (old_list_data && !list_data) {
	for ( ; old_list_data; old_list_data = EDT_GetListData(context)) {
	    EDT_FreeListData(old_list_data);
	    EDT_Outdent(context);
	}
	old_list_data = NULL;
    }
    
    /*
     *    If the style has changed.
     */
    if (paragragh_type != old_paragraph_type)
	EDT_MorphContainer(context, paragragh_type);

    /*
     *    If there was no list, and now there is..
     */
    if (!old_list_data && list_data) {
	EDT_Indent(context);
	old_list_data = EDT_GetListData(context);
    }

    /*
     *    The BE is losing. It cannot deal with lists in selections,
     *    it returns no list data. So, we protect oursleves and
     *    end up doing nothing, just like windows.
     */
    if (old_list_data != NULL) {

	do_set = TRUE;

	/*
	 *    If this is a block quote..
	 */
	if (list_data != NULL && list_data->iTagType == P_BLOCKQUOTE) {
	    old_list_data->iTagType = P_BLOCKQUOTE;
	    old_list_data->eType = ED_LIST_TYPE_DEFAULT;
	}

	/*
	 *    If it's a list.
	 */
	else if (list_data != NULL) {
	    old_list_data->iTagType = list_data->iTagType;
	    old_list_data->eType = ED_LIST_TYPE_DEFAULT;
#if 0
	    /*
	     *    We have no UI for this, but maybe it's in  the author's
	     *    HTML. Don't change it. Actually, I have no idea.
	     */
	    old_list_data->bCompact = FALSE;
#endif
	    switch (list_data->iTagType) {
	    case P_UNUM_LIST:
		if (list_data->eType == ED_LIST_TYPE_DISC   ||
		    list_data->eType == ED_LIST_TYPE_SQUARE	||
		    list_data->eType == ED_LIST_TYPE_CIRCLE)
		    old_list_data->eType = list_data->eType;
		break;
	    case P_NUM_LIST:
		if (list_data->eType == ED_LIST_TYPE_DIGIT       ||
		    list_data->eType == ED_LIST_TYPE_BIG_ROMAN   ||
		    list_data->eType == ED_LIST_TYPE_SMALL_ROMAN ||
		    list_data->eType == ED_LIST_TYPE_BIG_LETTERS ||
		    list_data->eType == ED_LIST_TYPE_SMALL_LETTERS)
		    old_list_data->eType = list_data->eType;
		if (list_data->iStart > 0)
		    old_list_data->iStart = list_data->iStart;
		else
		    old_list_data->iStart = 1;
		break;
	    case P_DIRECTORY:
	    case P_MENU:
	    case P_DESC_LIST:
		break;
	    default: /* garbage, don't set */
		do_set = FALSE;
		break;
	    }
	}

	if (do_set)
	    EDT_SetListData(context, old_list_data);
    }

    /*
     *    Else it's just an ordinary old jo.
     */
    /* nothing to do */
 
    if (old_list_data != NULL)
	EDT_FreeListData(old_list_data);

    EDT_EndBatchChanges(context);
    
    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorParagraphPropertiesGetAll(MWContext*    context,
				   TagType*      paragragh_type,
				   EDT_ListData* list_data,
				   ED_Alignment* align)
{
    TagType       old_paragraph_type = EDT_GetParagraphFormatting(context);
    EDT_ListData* old_list_data = EDT_GetListData(context);

    if (paragragh_type)
	*paragragh_type = old_paragraph_type;

    if (list_data && old_list_data)
	*list_data = *old_list_data;

    if (align)
	*align = EDT_GetParagraphAlign(context);

    if (old_list_data != NULL)
	EDT_FreeListData(old_list_data);

    return (old_list_data != NULL);
}

void
fe_EditorColorSet(MWContext* context, LO_Color* color)
{
    EDT_CharacterData cdata;

    memset(&cdata, 0, sizeof(EDT_CharacterData));

    cdata.mask = TF_FONT_COLOR;
    cdata.pColor = color;

    if (color)
	cdata.values = TF_FONT_COLOR;
    else
	cdata.values = 0;

    EDT_SetCharacterData(context, &cdata);
}

Boolean
fe_EditorColorGet(MWContext* context, LO_Color* color)
{
    EDT_CharacterData* cdata = EDT_GetCharacterData(context);
    Boolean rv;

    if ((cdata->values & TF_FONT_COLOR) != 0) { /* custom */
	*color = *cdata->pColor;
	rv = TRUE;
    } else {
	fe_EditorDocumentGetColors(context,
				   NULL, /* bg image */
				   NULL, /* bg color */
				   color,/* normal color */
				   NULL, /* link color */
				   NULL, /* active color */
				   NULL); /* followed color */
	rv = FALSE;
    }

    EDT_FreeCharacterData(cdata);

    return rv;
}

extern int XFE_COULDNT_FORK;
extern int XFE_EXECVP_FAILED;

void
fe_ExecCommand(MWContext* context, char** argv)
{
    char buf[MAXPATHLEN];
    int forked;

    switch (forked = fork()) {
    case -1:
	fe_perror(context, XP_GetString(XFE_COULDNT_FORK));
	break;
    case 0: {
	Display *dpy = XtDisplay(CONTEXT_WIDGET(context));
	close(ConnectionNumber(dpy));

	execvp(argv[0], argv);

	PR_snprintf(buf, sizeof(buf), XP_GetString(XFE_EXECVP_FAILED),
			fe_progname, argv[0]);
	perror(buf);
	exit(3);	/* Note that this only exits a child fork.  */
	break;
    }
    default:
	/* block */
	break;
    }
}

static char**
fe_editor_parse_argv(char* string, char* filename)
{
    char     buf[MAXPATHLEN];
    unsigned size = 32;
    unsigned argc = 0;
    char**   argv = (char**)malloc(sizeof(char*) * size);
    char*    p;
    char*    q;
    char*    r;

    for (p = string; *p != '\0'; ) {

	q = buf;

	/* skip leading white */
	while (isspace(*p))
	    p++;

	while (!isspace(*p) && *p != '\0' && ((q - buf) < sizeof(buf))) {
	    if (*p == '%' && p[1] == 'f') {
		for (r = filename; *r != '\0'; ) {
		    *q++ = *r++;
		}
		p += 2;
	    } else {
		*q++ = *p++;
	    }
	}
	*q = '\0';

	if (q > buf)
	    argv[argc++] = strdup(buf);

	if (argc == size) {
	    size = argc + 32;
	    argv = (char**)realloc((void*)argv, sizeof(char*) * size);
	}
    }

    if (argc > 0) {
	argv[argc] = NULL;
	return argv;
    } else {
	free(argv);
	return NULL;
    }
}

void
fe_editor_free_argv(char** argv)
{
    unsigned i;
    for (i = 0; argv[i]; i++)
	free(argv[i]);
    free(argv);
}

void
fe_editor_exec_command(MWContext* context, char* command, char* filename)
{
    char** argv;

    if ((argv = fe_editor_parse_argv(command, filename)) != NULL) {

	fe_ExecCommand(context, argv);

	fe_editor_free_argv(argv);
    } else {
	/* Empty command specified! */
	FE_Alert(context, XP_GetString(XFE_COMMAND_EMPTY));
    }
}

void
fe_EditorEditSource(MWContext* context)
{
    char* html_editor;
    char* local_name;
    History_entry* hist_entry;

    if (!FE_CheckAndSaveDocument(context))
	return;
    fe_EditorPreferencesGetEditors(context, &html_editor, NULL);

    if (html_editor == NULL || html_editor[0] == '\0') {
	/*
	 * "Please specify an editor in Editor Preferences.\n"
	 * "Specify the file argument with %f. Netscape will\n"
	 * "replace %f with the correct file name. Examples:\n"
	 * "             xterm -e vi %f\n"
	 * "             xgifedit %f\n"
	 */
	char* msg = XP_GetString(XFE_EDITOR_HTML_EDITOR_COMMAND_EMPTY);
	
      	if (XFE_Confirm(context, msg)) {
	    fe_EditorPreferencesDialogDo(context,
				 XFE_EDITOR_DOCUMENT_PROPERTIES_GENERAL);
	}

	return;
    } 

    hist_entry = SHIST_GetCurrent(&context->hist);
    if (hist_entry && hist_entry->address) {
	if (XP_ConvertUrlToLocalFile(hist_entry->address, &local_name)) {
	    fe_editor_exec_command(context, html_editor, local_name);
	    XP_FREE(local_name);
	}
    }
}

CB_STATIC void
fe_editor_edit_source_cb(Widget widget, XtPointer closure, XtPointer cb)
{
    MWContext* context = (MWContext*)closure;

    fe_UserActivity(context);

    fe_EditorEditSource(context);
}

void
fe_EditorEditImage(MWContext* context, char* file)
{
    char* image_editor;
    char* local_name = NULL;
    char* full_file;
    char* directory;
    char* p;
    History_entry* hist_entry;

    fe_EditorPreferencesGetEditors(context, NULL, &image_editor);

    if (image_editor == NULL || image_editor[0] == '\0') {
	/*
	 * "Please specify an editor in Editor Preferences.\n"
	 * "Specify the file argument with %f. Netscape will\n"
	 * "replace %f with the correct file name. Examples:\n"
	 * "             xterm -e vi %f\n"
	 * "             xgifedit %f\n"
	 */
	char* msg = XP_GetString(XFE_EDITOR_IMAGE_EDITOR_COMMAND_EMPTY);

      	if (XFE_Confirm(context, msg)) {
	    fe_EditorPreferencesDialogDo(context,
				 XFE_EDITOR_DOCUMENT_PROPERTIES_GENERAL);
	}

	return;
    }

    while (isspace(*file))
	file++;

    if (file[0] != '/') { /* not absolute */
	hist_entry = SHIST_GetCurrent(&context->hist);
	if (hist_entry && hist_entry->address) {
	    if (XP_ConvertUrlToLocalFile(hist_entry->address, &local_name)) {
		p = strrchr(local_name, '/'); /* find last slash */
		if (p) {
		    *p = '\0';
		    directory = local_name;
		} else {
		    directory = ".";
		}
		full_file = (char*)XtMalloc(strlen(directory) +
					    strlen(file) + 2);
		sprintf(full_file, "%s/%s", directory, file);
		XP_FREE(local_name);

		fe_editor_exec_command(context, image_editor, full_file);
		XtFree(full_file);
	    }
	}
    } else {
	fe_editor_exec_command(context, image_editor, file);
    }
}

CB_STATIC void
fe_editor_browse_publish_location_cb(Widget widget,
				     XtPointer closure, XtPointer cb_data)
{
    MWContext* context = (MWContext*)closure;
    char*      location = fe_EditorPreferencesGetBrowseLocation(context);

    if (location == NULL || location[0] == '\0') {
	/*
	 *    "The default browse location is not set.\n"
	 *    "Would you like to enter a value in Editor Preferences now?")
	 */
	char* msg = XP_GetString(XFE_EDITOR_BROWSE_LOCATION_EMPTY);
	
      	if (XFE_Confirm(context, msg)) {
	    fe_EditorPreferencesDialogDo(context, 3); /* yea, I know */
	}
	
	return;
    }

    fe_GetURLInBrowser(context, NET_CreateURLStruct(location, FALSE));
}

#define COPY_SAVE_EXTRA(b, a) \
{ char* xx_extra = (b)->pExtra; *(b) = *(a); (b)->pExtra = xx_extra; }

#define COPY_SAVE_BGCOLOR(b, a) \
{ char* xx_bgcolor = (b)->pColorBackground; *(b) = *(a); (b)->pColorBackground = xx_bgcolor; }

static LO_Color*
fe_dup_lo_color(LO_Color* color)
{
    LO_Color* new_color;

    if (color != NULL && (new_color = (LO_Color*)XtNew(LO_Color)) != NULL) {
	*new_color = *color;
	return new_color;
    } else {
	return NULL;
    }
}

#define DUP_BGCOLOR(b, a) \
{ (b)->pColorBackground = fe_dup_lo_color((a)->pColorBackground); }

/*
 *    Table support.
 */
Boolean
fe_EditorTableCanInsert(MWContext* context)
{
    return !EDT_IsJavaScript(context);
}

void
fe_EditorTableInsert(MWContext* context, EDT_AllTableData* all_data)
{
    EDT_TableData* new_data = EDT_NewTableData();
    EDT_TableCaptionData* caption_data;
    
    if (all_data) {
	EDT_TableData* data = &all_data->td;
	COPY_SAVE_EXTRA(new_data, data);
	DUP_BGCOLOR(new_data, data);
    } else {
	new_data->iBorderWidth = 1;
	new_data->iRows = 2;
	new_data->iColumns = 2;
    }
    EDT_InsertTable(context, new_data);
    EDT_FreeTableData(new_data);

    if (all_data != NULL && all_data->has_caption) {
	caption_data = EDT_NewTableCaptionData();
	caption_data->align = all_data->cd.align;
	EDT_InsertTableCaption(context, caption_data);
    }

    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorTableCanDelete(MWContext* context)
{
    return (EDT_IsInsertPointInTable(context) != 0);
}

void
fe_EditorTableDelete(MWContext* context)
{
    if (EDT_IsInsertPointInTable(context) != 0) {
	EDT_DeleteTable(context);
	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableGetData(MWContext* context, EDT_AllTableData* all_data)
{
    Boolean in_table = FALSE;
    EDT_TableData* table_data = EDT_GetTableData(context);
    EDT_TableData* pt_data = &all_data->td;
    EDT_TableCaptionData* caption_data;

    if (table_data) {
	*pt_data = *table_data;
	if (table_data->pColorBackground != NULL) {
	    static LO_Color color_buf;
	    color_buf = *table_data->pColorBackground;
	    pt_data->pColorBackground = &color_buf; /*caller cannot hang on*/
	}
	
	EDT_FreeTableData(table_data);
	in_table = TRUE;
    } else {
#if 0
	/*
	 *    Set defaults.
	 */
	pt_data->iRows = 0;
	pt_data->iColumns = 0;
	pt_data->BorderWidth = 0;
	pt_data->iCellSpacing = 0;
	pt_data->iCellPadding = 0;
	pt_data->bWidthDefined = FALSE;
	pt_data->bWidthPercent = FALSE;
	pt_data->iWidth = 0;
	pt_data->bHeightDefined = FALSE;
	pt_data->bHeightPercent = FALSE;
	pt_data->iHeight = 0;
	pt_data->pColorBackground = NULL;
#endif
	return FALSE;
    }

    if (EDT_IsInsertPointInTableCaption(context)) {
	caption_data = EDT_GetTableCaptionData(context);
	all_data->has_caption = TRUE;
	all_data->cd.align = caption_data->align;
    } else {
	all_data->has_caption = FALSE;
	all_data->cd.align = ED_ALIGN_TOP;
    }
    return TRUE;
}

void
fe_EditorTableSetData(MWContext* context, EDT_AllTableData* all_data)
{
    Boolean has_caption;
    Boolean wants_caption;
    EDT_TableData* table_data = EDT_GetTableData(context);
    EDT_TableCaptionData* caption_data;

    if (table_data) {
	/* get the basic table data */
	COPY_SAVE_EXTRA(table_data, &all_data->td);
	DUP_BGCOLOR(table_data, &all_data->td);

	EDT_SetTableData(context, table_data);

	caption_data = NULL;
	has_caption = EDT_IsInsertPointInTableCaption(context);
	wants_caption = all_data->has_caption;

	if (has_caption == TRUE && wants_caption == FALSE) {
	    EDT_DeleteTableCaption(context);
	} else if (has_caption == FALSE && wants_caption == TRUE) {
	    caption_data = EDT_NewTableCaptionData();
	    caption_data->align = all_data->cd.align;
	    EDT_InsertTableCaption(context, caption_data);
	} else if (has_caption) {
	    caption_data = EDT_GetTableCaptionData(context);
	    if (caption_data->align != all_data->cd.align) {
		caption_data->align = all_data->cd.align;
		EDT_SetTableCaptionData(context, caption_data);
	    }
	}

	if (caption_data)
	    EDT_FreeTableCaptionData(caption_data);

	EDT_FreeTableData(table_data);

	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableRowCanInsert(MWContext* context)
{
    return (EDT_IsInsertPointInTableRow(context) != 0);
}

void
fe_EditorTableRowInsert(MWContext* context, EDT_TableRowData* data)
{
    EDT_TableRowData* new_data;

    if (!fe_EditorTableRowCanInsert(context)) {
	fe_Bell(context);
	return;
    }

    new_data = EDT_GetTableRowData(context);

    if (data) {
	new_data->align = data->align;
	new_data->valign = data->valign;
	DUP_BGCOLOR(new_data, data);
    } 
    EDT_InsertTableRows(context, new_data, TRUE, 1);
    EDT_FreeTableRowData(new_data);
    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorTableRowCanDelete(MWContext* context)
{
    return (EDT_IsInsertPointInTableRow(context) != 0);
}

void
fe_EditorTableRowDelete(MWContext* context)
{
    if (EDT_IsInsertPointInTable(context) != 0) {
	EDT_DeleteTableRows(context, 1);
	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableRowGetData(MWContext* context, EDT_TableRowData* p_data)
{
    EDT_TableRowData* row_data = EDT_GetTableRowData(context);

    if (row_data) {
	*p_data = *row_data;
	if (row_data->pColorBackground != NULL) {
	    static LO_Color color_buf;
	    color_buf = *row_data->pColorBackground;
	    p_data->pColorBackground = &color_buf; /*caller cannot hang on*/
	}
	EDT_FreeTableRowData(row_data);
	return TRUE;
    } else {
	return FALSE;
    }
}

void
fe_EditorTableRowSetData(MWContext* context, EDT_TableRowData* p_data)
{
    EDT_TableRowData* row_data = EDT_GetTableRowData(context);

    if (row_data) {
	COPY_SAVE_EXTRA(row_data, p_data);
	DUP_BGCOLOR(row_data, p_data);
	EDT_SetTableRowData(context, row_data);	
	EDT_FreeTableRowData(row_data);
	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableCaptionCanInsert(MWContext* context)
{
    return (EDT_IsInsertPointInTable(context) != 0);
}

void
fe_EditorTableCaptionInsert(MWContext* context, EDT_TableCaptionData* data)
{
    EDT_TableCaptionData* new_data = EDT_NewTableCaptionData();
    EDT_InsertTableCaption(context, new_data);
    EDT_FreeTableCaptionData(new_data);
    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorTableCaptionCanDelete(MWContext* context)
{
    return (EDT_IsInsertPointInTableCaption(context) != 0);
}

void
fe_EditorTableCaptionDelete(MWContext* context)
{
    if (EDT_IsInsertPointInTableCaption(context) != 0) {
	EDT_DeleteTableCaption(context);
	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableColumnCanInsert(MWContext* context)
{
    return (EDT_IsInsertPointInTableRow(context) != 0);
}

void
fe_EditorTableColumnInsert(MWContext* context, EDT_TableCellData* data)
{
    EDT_TableCellData* new_data = EDT_NewTableCellData();
    EDT_InsertTableColumns(context, new_data, TRUE, 1);
    EDT_FreeTableCellData(new_data);

    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorTableColumnCanDelete(MWContext* context)
{
    return (EDT_IsInsertPointInTableRow(context) != 0);
}

void
fe_EditorTableColumnDelete(MWContext* context)
{
    if (EDT_IsInsertPointInTableRow(context) != 0) {
	EDT_DeleteTableColumns(context, 1);
	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableCellCanInsert(MWContext* context)
{
    return (EDT_IsInsertPointInTableRow(context) != 0);
}

void
fe_EditorTableCellInsert(MWContext* context, EDT_TableCellData* data)
{
    EDT_TableCellData* new_data = EDT_NewTableCellData();
    EDT_InsertTableCells(context, new_data, TRUE, 1);
    EDT_FreeTableCellData(new_data);

    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorTableCellCanDelete(MWContext* context)
{
    return (EDT_IsInsertPointInTableCell(context) != 0);
}

void
fe_EditorTableCellDelete(MWContext* context)
{
    if (EDT_IsInsertPointInTableRow(context) != 0) {
	EDT_DeleteTableCells(context, 1);
	fe_EditorUpdateToolbar(context, 0);
    }
}

Boolean
fe_EditorTableCellGetData(MWContext* context, EDT_TableCellData* p_data)
{
    EDT_TableCellData* cell_data = EDT_GetTableCellData(context);
    if (cell_data) {
	*p_data = *cell_data;
	if (cell_data->pColorBackground != NULL) {
	    static LO_Color color_buf;
	    color_buf = *cell_data->pColorBackground;
	    p_data->pColorBackground = &color_buf; /*caller cannot hang on*/
	}
	EDT_FreeTableCellData(cell_data);
	return TRUE;
    } else {
	return FALSE;
    }
}

void
fe_EditorTableCellSetData(MWContext* context, EDT_TableCellData* p_data)
{
    EDT_TableCellData* cell_data = EDT_GetTableCellData(context);
    if (cell_data) {
	COPY_SAVE_EXTRA(cell_data, p_data);
	DUP_BGCOLOR(cell_data, p_data);
	EDT_SetTableCellData(context, cell_data);
	EDT_FreeTableCellData(cell_data);
	fe_EditorUpdateToolbar(context, 0);
    }
}

void
fe_EditorSelectTable(MWContext* context)
{
    EDT_SelectTable(context);  
    fe_EditorUpdateToolbar(context, 0);
}

void
fe_EditorPublish(MWContext* context)
{
    History_entry* hist_entry;

#if 0
    /*FIXME*/
    /* why would we be here anyway??...djw */
    /* Force saving (or don't publish) if new document,*/
    /*  or ask user to save first */
    if ((context->is_new_document &&
	 !FE_SaveNonLocalDocument(context, TRUE)) ||
        !FE_CheckAndSaveDocument(context) ) {
	return;
    }
#endif
    if (context->is_new_document)
        return;

    hist_entry = SHIST_GetCurrent(&context->hist);
    if( !hist_entry || !hist_entry->address ||
	!NET_IsLocalFileURL(hist_entry->address) ){
	return;
    }

    /* Get the destination and what files to include from the user:*/
    fe_EditorPublishDialogDo(context);
}

CB_STATIC void
fe_editor_publish_cb(Widget widget, XtPointer closure, XtPointer cd)
{
    MWContext* context = (MWContext*)closure;

    fe_UserActivity(context);

    if (!FE_CheckAndSaveDocument(context))
	return;

    fe_EditorPublish(context);
}

CB_STATIC void
fe_editor_table_properties_dialog_cb(Widget widget,
			  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;

    fe_EditorTablePropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_TABLE);
}

CB_STATIC void
fe_editor_table_properties_dialog_update_cb(Widget widget,
			  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;

    Boolean sensitive = fe_EditorTableCanDelete(context);

    XtVaSetValues(widget, XtNsensitive, sensitive, 0);
}


CB_STATIC void
fe_editor_table_insert_cb(Widget widget,
			  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
#if 1
    fe_EditorTableCreateDialogDo(context);
#else
    fe_EditorTableInsert(context, NULL);
#endif
}

CB_STATIC void
fe_editor_table_insert_update_cb(Widget widget,
			      XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableCanInsert(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_row_insert_cb(Widget widget,
			      XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableRowInsert(context, NULL);
}

CB_STATIC void
fe_editor_table_row_insert_update_cb(Widget widget,
			      XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableRowCanInsert(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_column_insert_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableColumnInsert(context, NULL);
}

CB_STATIC void
fe_editor_table_column_insert_update_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableColumnCanInsert(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_caption_insert_cb(Widget widget,
				  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableCaptionInsert(context, NULL);
}

CB_STATIC void
fe_editor_table_caption_insert_update_cb(Widget widget,
				  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableCaptionCanInsert(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_cell_insert_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableCellInsert(context, NULL);
}

CB_STATIC void
fe_editor_table_cell_insert_update_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableCellCanInsert(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_delete_cb(Widget widget,
			  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableDelete(context);
}

CB_STATIC void
fe_editor_table_delete_update_cb(Widget widget,
			  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableCanDelete(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_row_delete_cb(Widget widget,
			      XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableRowDelete(context);
}

CB_STATIC void
fe_editor_table_row_delete_update_cb(Widget widget,
			      XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableRowCanDelete(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_column_delete_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableColumnDelete(context);
}

CB_STATIC void
fe_editor_table_column_delete_update_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableColumnCanDelete(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_caption_delete_cb(Widget widget,
				  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableCaptionDelete(context);
}

CB_STATIC void
fe_editor_table_caption_delete_update_cb(Widget widget,
				  XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableCaptionCanDelete(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_cell_delete_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    fe_EditorTableCellDelete(context);
}

CB_STATIC void
fe_editor_table_cell_delete_update_cb(Widget widget,
				 XtPointer closure, XtPointer c_data)
{
    MWContext* context = (MWContext*)closure;
    Boolean sensitive = FALSE;
    if (fe_EditorTableCellCanDelete(context))
	sensitive = TRUE;
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

CB_STATIC void
fe_editor_table_menu_cb(Widget cascade, XtPointer closure, XtPointer c_data)
{
    Widget     pulldown;
    WidgetList children;
    Cardinal   nchildren;
    Cardinal i;
    Widget button;
    char* name;

    XtVaGetValues(cascade, XmNsubMenuId, &pulldown, 0);
    XtVaGetValues(pulldown, XmNchildren, &children,
		  XmNnumChildren, &nchildren, 0);

    for (i = 0; i < nchildren; i++) {

	button = children[i];
	name = XtName(button);
	/* update */

	if (strcmp(name, "insertTable") == 0)
	  fe_editor_table_insert_update_cb(button, closure, 0);
	else if (strcmp(name, "deleteTable") == 0)
	  fe_editor_table_delete_update_cb(button, closure, 0);
	else if (strcmp(name, "insertRow") == 0) 
	  fe_editor_table_row_insert_update_cb(button, closure, 0);
	else if (strcmp(name, "deleteRow") == 0) 
	  fe_editor_table_row_delete_update_cb(button, closure, 0);
	else if (strcmp(name, "insertColumn") == 0) 
	  fe_editor_table_column_insert_update_cb(button, closure, 0);
	else if (strcmp(name, "deleteColumn") == 0) 
	  fe_editor_table_column_delete_update_cb(button, closure, 0);
	else if (strcmp(name, "insertCell") == 0) 
	  fe_editor_table_cell_insert_update_cb(button, closure, 0);
	else if (strcmp(name, "deleteCell") == 0) 
	  fe_editor_table_cell_delete_update_cb(button, closure, 0);
	else if (strcmp(name, "insertCaption") == 0) 
	  fe_editor_table_caption_insert_update_cb(button, closure, 0);
	else if (strcmp(name, "deleteCaption") == 0) 
	  fe_editor_table_caption_delete_update_cb(button, closure, 0);
    }
}

void
fe_EditorDisplayTablesSet(MWContext* context, Boolean display)
{
    EDT_SetDisplayTables(context, display == TRUE);
    fe_EditorUpdateToolbar(context, 0);
}

Boolean
fe_EditorDisplayTablesGet(MWContext* context)
{
    return (EDT_GetDisplayTables(context) == TRUE);
}

/*
 *    Popup menu handling.
 */
Widget
fe_CreatePopupMenu(MWContext* context,
		   Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    Arg args[8];
    Cardinal n;
    Widget menu;
    Widget mainw = CONTEXT_WIDGET(context);

    XtVaGetValues(mainw, XtNvisual, &v, XtNcolormap, &cmap,
		  XtNdepth, &depth, 0);
    
    n = 0;
    XtSetArg(args[n], XmNvisual, v); n++;
    XtSetArg(args[n], XmNcolormap, cmap); n++;
    XtSetArg(args[n], XmNdepth, depth); n++;
    
    menu = XmCreatePopupMenu(parent, name, args, n);
    
    return menu;
}

Widget
fe_CreatePulldownMenu(MWContext* context,
		   Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    Arg args[8];
    Cardinal n;
    Widget menu;
    Widget mainw = CONTEXT_WIDGET(context);

    XtVaGetValues(mainw, XtNvisual, &v, XtNcolormap, &cmap,
		  XtNdepth, &depth, 0);
    
    n = 0;
    XtSetArg(args[n], XmNvisual, v); n++;
    XtSetArg(args[n], XmNcolormap, cmap); n++;
    XtSetArg(args[n], XmNdepth, depth); n++;
    
    menu = XmCreatePulldownMenu(parent, name, args, n);
    
    return menu;
}

typedef Widget (*fe_PopupMenuCreate) (MWContext* context,
				      Widget parent,
				      char*  name,  /* instance name */
				      Arg*   args,
				      Cardinal n);
typedef struct fe_PopupMenuDescription
{
    char*              name;     /* name of the menu item */
    fe_PopupMenuCreate create;
    XtCallbackProc     update;   /* update sensitivity callback */
    XtPointer          user_data;
} fe_PopupMenuDescription;

static void
fe_PopulateMenuFromDescription(MWContext* context, Widget menu,
			       fe_PopupMenuDescription* description)
{
    Arg    args[10];
    Cardinal n;
    unsigned i;
    Widget button;
    char* button_name;
    fe_PopupMenuCreate create;

    for (i = 0; (button_name = description[i].name) != NULL; i++) {
	create = description[i].create;
	if (!create)
	    continue;
	n = 0;
	XtSetArg(args[0], XmNuserData, description[i].user_data); n++;
	button = (create)(context, menu, button_name, args, n);
	if (description[i].update)
	    (description[i].update)(button, context, NULL);

	XtManageChild(button);
    }
}

static Widget
fe_menu_button(MWContext* context, Widget parent, char* name,
	       Arg* p_args, Cardinal p_n)
{
    Arg args[2];
    Cardinal n;
    XtCallbackProc callback = (XtCallbackProc)p_args[0].value;
    Widget widget;

    n = 0;
    widget = XmCreatePushButtonGadget(parent, name, args, n);

    if (callback) {
	XtAddCallback(widget, XmNactivateCallback, callback, context);
	fe_WidgetAddDocumentString(context, widget);
    }
    return widget;
}

#if 0
static void
fe_menu_menu_cascading_cb(Widget widget, XtPointer closure, XtPointer cb)
{
    fe_PopupMenuDescription* description = (fe_PopupMenuDescription*)closure;
    Widget     menu;
    WidgetList children;
    Cardinal   nchildren;
    Cardinal i;
    Widget button;
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    XtVaGetValues(widget, XmNsubMenuId, &menu, 0);
    XtVaGetValues(menu, XmNchildren, &children,
		  menu, &nchildren, 0);

    for (i = 0; i < nchildren && description[i].name != NULL; i++) {
	button = children[i];

	if (description[i].update)
	    (description[i].update)(button, context, NULL);
    }
}
#endif

static Widget
fe_menu_menu(MWContext* context, Widget parent, char* name,
	     Arg* p_args, Cardinal p_n)
{
    Arg args[2];
    Cardinal n;
    fe_PopupMenuDescription* description;
    Widget menu;
    Widget cascade;

    description = (fe_PopupMenuDescription*)p_args[0].value;

    n = 0;
    menu = fe_CreatePulldownMenu(context, parent, name, args, 0);
    fe_PopulateMenuFromDescription(context, menu, description);

    n = 0;
    XtSetArg(args[n], XmNsubMenuId, menu); n++;
    cascade = XmCreateCascadeButtonGadget(parent, name, args, n);

#if 0
    XtAddCallback(cascade, XmNcascadingCallback,
		  fe_menu_menu_cascading_cb, description);
#endif

    return cascade;
}

static Widget
fe_menu_separator(MWContext* context, Widget parent, char* name,
	       Arg* p_args, Cardinal p_n)
{
    Arg args[2];
    Cardinal n;
    Widget widget;
    unsigned s_type = (unsigned)p_args[0].value;

    n = 0;
    XtSetArg(args[n], XmNseparatorType, s_type); n++;
    widget = XmCreateSeparatorGadget(parent, name, args, n);

    return widget;
}

static Widget
fe_menu_label(MWContext* context, Widget parent, char* name,
	       Arg* p_args, Cardinal p_n)
{
    Arg args[2];
    Cardinal n;
    Widget widget;

    n = 0;
    widget = XmCreateLabelGadget(parent, name, args, n);

    return widget;
}

static fe_PopupMenuDescription fe_editor_document_popups[] =
{
    { "documentProperties", fe_menu_button,
      NULL,
      fe_editor_document_properties_dialog_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_character_popups[] =
{
    { "textProperties", fe_menu_button,
      NULL,
      fe_editor_character_properties_dialog_cb },
    { "paragraphProperties", fe_menu_button,
      NULL,
      fe_editor_paragraph_properties_dialog_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_image_popups[] =
{
    { "imageProperties", fe_menu_button,
      fe_editor_image_properties_dialog_update_cb,
      fe_editor_image_properties_dialog_cb },
    { "paragraphProperties", fe_menu_button,
      NULL,
      fe_editor_paragraph_properties_dialog_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_tag_popups[] =
{
    { "tagProperties", fe_menu_button,
      fe_editor_html_properties_dialog_update_cb,
      fe_editor_html_properties_dialog_cb },
    { "paragraphProperties", fe_menu_button,
      NULL,
      fe_editor_paragraph_properties_dialog_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_target_popups[] =
{
    { "targetProperties", fe_menu_button,
      fe_editor_target_properties_dialog_update_cb,
      fe_editor_target_properties_dialog_cb },
    { "paragraphProperties", fe_menu_button,
      NULL,
      fe_editor_paragraph_properties_dialog_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_hrule_popups[] =
{
    { "hruleProperties", fe_menu_button,
      fe_editor_hrule_properties_dialog_update_cb,
      fe_editor_hrule_properties_dialog_cb },
    { "paragraphProperties", fe_menu_button,
      NULL,
      fe_editor_paragraph_properties_dialog_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_table_insert_popups[] =
{
    { "insertTable", fe_menu_button, fe_editor_table_insert_update_cb,
      fe_editor_table_insert_cb },
    { "insertRow",   fe_menu_button, fe_editor_table_row_insert_update_cb,
      fe_editor_table_row_insert_cb },
    { "insertColumn",fe_menu_button, fe_editor_table_column_insert_update_cb,
      fe_editor_table_column_insert_cb },
    { "insertCell",  fe_menu_button, fe_editor_table_cell_insert_update_cb,
      fe_editor_table_cell_insert_cb },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_table_delete_popups[] =
{
    { "deleteTable",  fe_menu_button, fe_editor_table_delete_update_cb,
      fe_editor_table_delete_cb },
    { "deleteRow",    fe_menu_button, fe_editor_table_row_delete_update_cb,
      fe_editor_table_row_delete_cb },
    { "deleteColumn", fe_menu_button, fe_editor_table_column_delete_update_cb,
      fe_editor_table_column_delete_cb },
    { "deleteCell",   fe_menu_button, fe_editor_table_cell_delete_update_cb,
      fe_editor_table_cell_delete_cb },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_table_popups[] =
{
    { "tableProperties", fe_menu_button,
      fe_editor_table_properties_dialog_update_cb,
      fe_editor_table_properties_dialog_cb },
    { "insert", fe_menu_menu,
      NULL, fe_editor_table_insert_popups },
    { "delete", fe_menu_menu,
      fe_editor_table_delete_update_cb, fe_editor_table_delete_popups },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_link_popups[] =
{
    { "linkProperties", fe_menu_button,
      fe_editor_link_properties_dialog_update_cb,
      fe_editor_link_properties_dialog_cb },
    { "browseLink", fe_menu_button, NULL, fe_editor_link_browse_cb },
    { "editLink", fe_menu_button, NULL, fe_editor_link_edit_cb },
    { "bookmarkLink", fe_menu_button, NULL, fe_editor_link_add_bookmark_cb },
    { "copyLink", fe_menu_button, NULL, fe_editor_link_copy_link_cb },
    { "removeLink", fe_menu_button, NULL, fe_editor_remove_links_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_insert_link_popups[] =
{
    { "insertLink", fe_menu_button, NULL, fe_editor_insert_link_cb },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_edit_popups[] =
{
    { "cut",   fe_menu_button, fe_editor_cut_update_cb, fe_editor_cut_cb },
    { "copy",  fe_menu_button, fe_editor_copy_update_cb, fe_editor_copy_cb },
    { "paste", fe_menu_button, NULL, fe_editor_paste_cb
      /*fe_editor_paste_update_cb*/ },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_title_popups[] =
{
    { "title", fe_menu_label, NULL },
    { "separator", fe_menu_separator, NULL, (XtPointer)XmDOUBLE_LINE },
    { NULL }
};

static fe_PopupMenuDescription fe_editor_show_options_popups[] =
{
    { "separator", fe_menu_separator, NULL, (XtPointer)XmSHADOW_ETCHED_IN },
    { "showMenubar", fe_menu_button, NULL, fe_editor_show_menubar_cb },
    { NULL }
};

static XtPointer
fe_editor_destroy_mappee(Widget widget, XtPointer data)
{
    Widget menu;

    if (XtIsSubclass(widget, xmCascadeButtonGadgetClass)) {
	XtVaGetValues(widget, XmNsubMenuId, &menu, 0);
	if (menu != 0)
	    fe_WidgetTreeWalk(menu, fe_editor_destroy_mappee, NULL);
    }

    XtDestroyWidget(widget);

    return 0;
}

static void
fe_popup_menu_popdown_cb(Widget menu, XtPointer closure, XtPointer cb)
{
    MWContext *context = (MWContext *) closure;

    fe_WidgetTreeWalk(menu, fe_editor_destroy_mappee, NULL);
    fe_SetCursor(context, False);
}

static unsigned
fe_add_menu(fe_PopupMenuDescription* tgt, fe_PopupMenuDescription* src)
{
    unsigned i;

    for (i = 0; src[i].name; i++)
	tgt[i] = src[i];

    return i;
}

static Widget
fe_EditorCreatePopupMenu(MWContext* context)
{
    fe_PopupMenuDescription items[32]; /* better not exceed */
    fe_PopupMenuDescription* list;
    unsigned nitems = 0;
    Widget parent = CONTEXT_WIDGET(context);
    Widget menu;
    ED_ElementType e_type;

    nitems += fe_add_menu(&items[nitems], fe_editor_title_popups);

    e_type = EDT_GetCurrentElementType(context);

    switch (e_type) {
    case ED_ELEMENT_IMAGE:       list = fe_editor_image_popups;  break;
    case ED_ELEMENT_UNKNOWN_TAG: list = fe_editor_tag_popups;    break;
    case ED_ELEMENT_TARGET:      list = fe_editor_target_popups; break;
    case ED_ELEMENT_HRULE:       list = fe_editor_hrule_popups;  break;
    case ED_ELEMENT_TEXT:
    default:                     list = fe_editor_character_popups; break;
    }
    nitems += fe_add_menu(&items[nitems], list);

    /*
     *    Document Props..
     */
    nitems += fe_add_menu(&items[nitems], fe_editor_document_popups);

    /*
     *    Table support.
     */
    if (EDT_IsInsertPointInTable(context) != 0) {
	nitems += fe_add_menu(&items[nitems], fe_editor_table_popups);
    }

    /*
     *    Link stuff.
     */
    if (EDT_GetHREF(context)) { 
	nitems += fe_add_menu(&items[nitems], fe_editor_link_popups);
    } else {
	nitems += fe_add_menu(&items[nitems], fe_editor_insert_link_popups);
    }


    /*
     *    Add cut&paste menu.
     */
    nitems += fe_add_menu(&items[nitems], fe_editor_edit_popups);

    /*
     *    If they accidently deleted the menubar, help them out.
     */
    if (!CONTEXT_DATA(context)->show_menubar_p)
        nitems += fe_add_menu(&items[nitems], fe_editor_show_options_popups);
    
    items[nitems].name = NULL;
    
    menu = fe_CreatePopupMenu(context, parent, "popup", NULL, 0);
    fe_PopulateMenuFromDescription(context, menu, items);
    
    /*
     *    Add destruction callback.
     */
    XtAddCallback(XtParent(menu), XmNpopdownCallback,
		  fe_popup_menu_popdown_cb, context);
    
    return menu;
}

static char*
fe_action_name(char* buf,
	       XtActionsRec* action_list,
	       Cardinal      action_list_size,
	       XtActionProc  action_proc,
	       String *av, Cardinal *ac)
{
    int i;
    char* name = XP_GetString(XFE_UNKNOWN); /* "<unknown>" */

    if (action_list_size == 0)
	action_list_size = 100000; /* look for null termination */

    /*
     *    Look for action name in the action_list.
     */
    for (i = 0; i < action_list_size && action_list[i].string != NULL; i++) {
	if (action_list[i].proc == action_proc) {
	    name = action_list[i].string;
	    break;
	}
    }

    strcpy(buf, name);
    strcat(buf, "(");
    for (i = 0; i < *ac; i++) {
	if (i != 0)
	    strcat(buf, ",");
	strcat(buf, av[i]);
    }
    strcat(buf, ")");

    return buf;
}


/*
 *    Editor specific action handlers.
 */
static void
fe_ActionSyntaxAlert(MWContext*, XtActionProc, String*, Cardinal*);
static void
fe_ActionWrongContextAlert(MWContext*, XtActionProc, String*, Cardinal*);

#define FE_ACTION_VALIDATE(proc)                             \
if (!(context) || (context)->type != MWContextEditor) {      \
    fe_ActionWrongContextAlert(context,                      \
			       (proc),                       \
			       av, ac);                      \
    return;                                                  \
}

#define FE_SYNTAX_ERROR(proc) fe_ActionSyntaxAlert(context, (proc), av, ac)

/*=========================== ACTIONS =================================*/
static void
fe_editor_insert_line_break_action(Widget widget, XEvent *event,
				   String *av, Cardinal *ac)
{
    MWContext*   context = (MWContext *)fe_WidgetToMWContext(widget);
    ED_BreakType type;

    FE_ACTION_VALIDATE(fe_editor_insert_line_break_action);

    if (*ac == 0 || (*ac == 1 && strcmp(av[0], "normal") == 0))
	type = ED_BREAK_NORMAL;
    else if (*ac == 1 && strcmp(av[0], "left") == 0)
	type = ED_BREAK_LEFT;
    else if (*ac == 1 && strcmp(av[0], "right") == 0)
	type = ED_BREAK_RIGHT;
    else if (*ac == 1 && strcmp(av[0], "both") == 0)
	type = ED_BREAK_BOTH;
    else { /* junk */
	FE_SYNTAX_ERROR(fe_editor_insert_line_break_action);
	return;
    }

    fe_EditorLineBreak(context, type);
}

static void /* my entrant for longest function name context..djw */
fe_editor_insert_non_breaking_space_action(Widget widget, XEvent *event,
				   String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_insert_non_breaking_space_action);
    
    fe_EditorNonBreakingSpace(context);
}

struct parse_tokens_st {
    char*    name;
    unsigned bits;
};
static struct parse_tokens_st parse_character_tokens[] = {
    { "bold", TF_BOLD },
    { "italic", TF_ITALIC },
    { "underline", TF_UNDERLINE },
    { "fixed",  TF_FIXED },
    { "super",  TF_SUPER },
    { "sub",    TF_SUB   },
    { "strikethrough", TF_STRIKEOUT },
    { "blink",  TF_BLINK },
    { "none",   TF_NONE } /* probably does nothing, but TF_NONE may change */
};

static ED_TextFormat
parse_character_types(char* s)
{
    ED_TextFormat bits = 0;
    char *p;
    char *q;
    int i;

    s = strdup(s); /* lazy */

    for (p = s; *p; ) {
	q = strchr(p, '|'); /* look for or */

	if (q)
	    *q++ = '\0'; /* punch out or */
	else
	    q = &p[strlen(p)];

	/*
	 *    Check the token.
	 */
	for (i = 0; i < countof(parse_character_tokens); i++) {
	    if (strcmp(p, parse_character_tokens[i].name) == 0) {
		bits |= parse_character_tokens[i].bits;
		break;
	    }
	}
	/* Should generate an error for no match */
	/*FIXME*/ 
	p = q;
    }

    free(s);

    return bits;
}

/*
 *    syntax: set-character-properties(type1|type2|type3....);
 *            set-character-properties(none);
 */
static void
fe_editor_set_character_properties_action(Widget widget, XEvent *event,
					  String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    ED_TextFormat bits;

    FE_ACTION_VALIDATE(fe_editor_set_character_properties_action);

    if (*ac == 1) {
	if (strcmp(av[0], "none") == 0) {
	    fe_EditorDoPoof(context);
	} else {
	    bits = parse_character_types(av[0]);
	    fe_EditorCharacterPropertiesSet(context, bits);
	}
	return;
    }

    FE_SYNTAX_ERROR(fe_editor_set_character_properties_action);
}

static void
fe_editor_toggle_character_properties_action(Widget widget, XEvent *event,
					  String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    ED_TextFormat bits;
    ED_TextFormat value;

    FE_ACTION_VALIDATE(fe_editor_toggle_character_properties_action);

    if (*ac == 1) {
	value = fe_EditorCharacterPropertiesGet(context);

	bits = parse_character_types(av[0]);

	value ^= bits;

	fe_EditorCharacterPropertiesSet(context, value);
	return;
    }

    FE_SYNTAX_ERROR(fe_editor_toggle_character_properties_action);
}

/*
 *    syntax: set-paragraph-properties(type);
 */
static void
fe_editor_set_paragraph_properties_action(Widget widget, XEvent *event,
					  String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    TagType    tag;
    int i;

    FE_ACTION_VALIDATE(fe_editor_set_paragraph_properties_action);

    if (*ac == 1) {

	for (i = 0; fe_paragraph_style_menu_items[i].name; i++) {
	    if (strcmp(av[0], fe_paragraph_style_menu_items[i].name) == 0) {
		tag = (TagType)(unsigned)fe_paragraph_style_menu_items[i].data;
		fe_EditorParagraphPropertiesSet(context, tag);
		return;
	    }
	}
    }

    FE_SYNTAX_ERROR(fe_editor_set_paragraph_properties_action);
}

static void
fe_editor_set_font_size_action(Widget widget, XEvent *event,
			       String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    ED_FontSize size;

    FE_ACTION_VALIDATE(fe_editor_set_font_size_action);

    if (*ac == 1) {
	if (strcmp(av[0], "-2") == 0)
	    size = ED_FONTSIZE_MINUS_TWO;
	else if (strcmp(av[0], "-1") == 0)
	    size = ED_FONTSIZE_MINUS_ONE;
	else if (strcmp(av[0], "+0") == 0)
	    size = ED_FONTSIZE_ZERO;
	else if (strcmp(av[0], "+1") == 0)
	    size = ED_FONTSIZE_PLUS_ONE;
	else if (strcmp(av[0], "+2") == 0)
	    size = ED_FONTSIZE_PLUS_TWO;
	else if (strcmp(av[0], "+3") == 0)
	    size = ED_FONTSIZE_PLUS_THREE;
	else if (strcmp(av[0], "+4") == 0)
	    size = ED_FONTSIZE_PLUS_FOUR;
	else if (strcmp(av[0], "default") == 0)
	    size = ED_FONTSIZE_ZERO;
	else if (strcmp(av[0], "increase") == 0)
	    size = fe_EditorFontSizeGet(context) + 1;
	else if (strcmp(av[0], "decrease") == 0)
	    size = fe_EditorFontSizeGet(context) - 1;
	else { /* junk */
	    FE_SYNTAX_ERROR(fe_editor_set_font_size_action);
	    return;
	}

	fe_EditorFontSizeSet(context, size);
	return;
    }

    FE_SYNTAX_ERROR(fe_editor_set_font_size_action);
}

static void
fe_editor_increase_font_size_action(Widget widget, XEvent *event,
			       String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    String   arg = "increase";
    Cardinal argc = 1;

    FE_ACTION_VALIDATE(fe_editor_increase_font_size_action);

    fe_editor_set_font_size_action(widget, event, &arg, &argc);
}

static void
fe_editor_decrease_font_size_action(Widget widget, XEvent *event,
			       String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    String   arg = "decrease";
    Cardinal argc = 1;

    FE_ACTION_VALIDATE(fe_editor_decrease_font_size_action);

    fe_editor_set_font_size_action(widget, event, &arg, &argc);
}

#define XFE_EDITOR_MAX_INSERT_SIZE 512

static void
fe_editor_key_input_action(Widget widget,
			   XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    char       buf[XFE_EDITOR_MAX_INSERT_SIZE + 1];
    KeySym     keysym;
    Status     status_return;
    int        len;
    int        n;
    EDT_ClipboardResult result;

    FE_ACTION_VALIDATE(fe_editor_key_input_action);

    if (widget != CONTEXT_DATA(context)->drawing_area) {
#ifdef DEBUG
	(void) fprintf(real_stderr,
		"fe_editor_key_input_action got wrong widget (%s)\n",
		XtName(widget));
#endif /* DEBUG */
	XtSetKeyboardFocus(CONTEXT_WIDGET(context),
				CONTEXT_DATA(context)->drawing_area);
	return;
    }
    len = XmImMbLookupString(widget,
			     (XKeyEvent *)event,
			     buf,
			     XFE_EDITOR_MAX_INSERT_SIZE, 
			     &keysym,
			     &status_return);
    
    if (status_return == XBufferOverflow || len > XFE_EDITOR_MAX_INSERT_SIZE)
        return;

    if (len > 0) {
	char *str;

	buf[len] = 0;
	str = (char *) fe_ConvertFromLocaleEncoding(context->win_csid,
		(unsigned char *) buf);
	if (str != buf) {
	    len = strlen(str);
	}
        for (n = 0; n < len; n++) {
	    if ((result = EDT_KeyDown(context, str[n], 0, 0)) != EDT_COP_OK) {
	        fe_editor_report_copy_error(context, result);
	    }
	}
	if (str != buf) {
	    XP_FREE(str);
	}
    }
}

static void
fe_editor_beginning_of_line_action(Widget widget,XEvent *event,String *av,Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

  FE_ACTION_VALIDATE(fe_editor_beginning_of_line_action);

  EDT_BeginOfLine(context, FALSE);
  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_end_of_line_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

  FE_ACTION_VALIDATE(fe_editor_end_of_line_action);

  EDT_EndOfLine(context, FALSE);
  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_backward_character_action(Widget widget, XEvent *event,
				    String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

  FE_ACTION_VALIDATE(fe_editor_backward_character_action);

  EDT_PreviousChar(context, FALSE);
  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_forward_character_action(Widget widget, XEvent *event,
				   String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

  FE_ACTION_VALIDATE(fe_editor_forward_character_action);

  EDT_NextChar(context, FALSE);
  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_delete_previous_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
  EDT_ClipboardResult result;
  
  FE_ACTION_VALIDATE(fe_editor_delete_previous_action);

  if ((result = EDT_DeletePreviousChar(context)) != EDT_COP_OK)
      fe_editor_report_copy_error(context, result);

  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_delete_next_action(Widget widget, XEvent *event,
			     String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
  
    FE_ACTION_VALIDATE(fe_editor_delete_next_action);

    fe_EditorDeleteItem(context);
}

static void
fe_editor_process_up_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
  
  FE_ACTION_VALIDATE(fe_editor_process_up_action);

  EDT_Up(context, FALSE);
  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_process_down_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
  
  FE_ACTION_VALIDATE(fe_editor_process_down_action);

  EDT_Down(context, FALSE);

  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_process_return_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
  
  FE_ACTION_VALIDATE(fe_editor_process_return_action);

  EDT_ReturnKey(context);

  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_update_toolbar_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
  
  FE_ACTION_VALIDATE(fe_editor_update_toolbar_action);

  fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_grab_focus_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = fe_MotionWidgetToMWContext(widget);
  unsigned long x, y;
  Time time;

  FE_ACTION_VALIDATE(fe_editor_grab_focus_action);

  fe_UserActivity(context); /* tell the app who has focus */

  /* don't do this -- Japanese input method requires focus
  fe_NeutralizeFocus(context);
  */

  time = fe_getTimeFromEvent(context, event); /* get the time */

  fe_EventLOCoords(context, event, &x, &y); /* get the doc co-ords */

  EDT_StartSelection(context, x, y);
}

static void
fe_editor_extend_start_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = fe_MotionWidgetToMWContext (widget);
    unsigned long x, y;
    Time time;

    FE_ACTION_VALIDATE(fe_editor_extend_start_action);

    fe_UserActivity (context);  

    /* don't do this -- Japanese input method requires focus
    fe_NeutralizeFocus (context);
    */

    if (CONTEXT_DATA(context)->editor.ascroll_data.timer_id)
	XtRemoveTimeOut(CONTEXT_DATA(context)->editor.ascroll_data.timer_id);

    if (
	CONTEXT_DATA(context)->clicking_blocked
	||
	CONTEXT_DATA (context)->synchronous_url_dialog)
	{
	    fe_Bell(context);
	    return;
	}

    time = fe_getTimeFromEvent(context, event); /* get the time */
  
    fe_EventLOCoords(context, event, &x, &y);
#if 1
    /* need to impliment disown */
    EDT_StartSelection(context, x, y);
#else
    fe_DisownSelection(context, time, False);
    LO_StartSelection(context, x, y);
#endif
}

static void
fe_editor_extend_adjust_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = fe_MotionWidgetToMWContext(widget);
    Time       time;
    unsigned long x;
    unsigned long y;

    FE_ACTION_VALIDATE(fe_editor_extend_adjust_action);

    time = fe_getTimeFromEvent(context, event); /* get the time */

    fe_UserActivity(context);

    /* don't do this -- Japanese input method requires focus
    fe_NeutralizeFocus(context);
    */

    fe_EventLOCoords (context, event, &x, &y);

    if (CONTEXT_DATA(context)->editor.ascroll_data.timer_id)
	XtRemoveTimeOut(CONTEXT_DATA(context)->editor.ascroll_data.timer_id);
    fe_editor_keep_pointer_visible(context, x, y);

#if 1
    /* need to impliment own */
    EDT_ExtendSelection(context, x, y);
#else
    LO_ExtendSelection (context, x, y);
    fe_OwnSelection (context, time, False);
#endif

#if 0
    /* Making a selection turns off "Save Next" mode. */
    if (CONTEXT_DATA (context)->save_next_mode_p)
    {
      XBell (XtDisplay (widget), 0);
      CONTEXT_DATA (context)->save_next_mode_p = False;
      fe_SetCursor (context, False);
      XFE_Progress (context, fe_globalData.click_to_save_cancelled_message);
    }
#endif
}

static void
fe_editor_extend_end_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext *context = fe_MotionWidgetToMWContext (widget);
    Time time;
    unsigned long x;
    unsigned long y;

    FE_ACTION_VALIDATE(fe_editor_extend_end_action);

    fe_UserActivity(context);
    time = fe_getTimeFromEvent(context, event); /* get the time */

    fe_EventLOCoords (context, event, &x, &y);

    if (CONTEXT_DATA(context)->editor.ascroll_data.timer_id)
	XtRemoveTimeOut(CONTEXT_DATA(context)->editor.ascroll_data.timer_id);

#if 1
    /* need to impliment own */
    EDT_EndSelection(context, x, y);
#else
    LO_EndSelection(context);
    fe_OwnSelection(context, time, False);
#endif
    fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_undo_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_undo_action);

    fe_EditorUndo(context);
}

static void
fe_editor_redo_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

  fe_EditorRedo(context);
}

static void
fe_editor_refresh_action(Widget widget, XEvent *event, String *av,
			 Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_refresh_action);

    fe_EditorRefresh(context);
}

static void
fe_editor_cut_action(Widget widget, XEvent *event, String *av,
			 Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_cut_action);

    fe_EditorCut(context, fe_getTimeFromEvent(context, event));
}

static void
fe_editor_copy_action(Widget widget, XEvent *event, String *av,
			 Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_copy_action);

    fe_EditorCopy(context, fe_getTimeFromEvent(context, event));
}

static void
fe_editor_paste_action(Widget widget, XEvent *event, String *av,
			 Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_paste_action);

    fe_EditorPaste(context, fe_getTimeFromEvent(context, event));
}

static void
fe_editor_find_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_find_action);

    fe_EditorFind(context);
}

static void
fe_editor_find_again_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_find_again_action);

    fe_EditorFindAgain(context);
}

static void
fe_editor_reload_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    Boolean super_reload = FALSE;

    FE_ACTION_VALIDATE(fe_editor_reload_action);

    if (*ac == 1 && strcmp(av[0], "super") == 0)
	super_reload = TRUE;

    fe_EditorReload(context, super_reload);
}

static void
fe_editor_open_file_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_open_file_action);

    fe_EditorOpenFile(context);
}

static void
fe_editor_open_url_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_open_url_action);

    fe_EditorOpenURLDialog(context);
}

static void
fe_editor_new_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_new_action);

    fe_EditorNew(context, NULL);
}

static void
fe_editor_delete_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_delete_action);

    if (!fe_save_file_check(context, FALSE, FALSE))
	return;

    fe_EditorDelete(context);
}

static void
fe_editor_save_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_save_action);

    fe_EditorSave(context);
}

static void
fe_editor_select_all_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_select_all_action);

    fe_EditorSelectAll(context);
}

static void
fe_editor_edit_source_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_edit_source_action);

    fe_EditorEditSource(context);
}

static void
fe_editor_select_word_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    unsigned long x;
    unsigned long y;

    FE_ACTION_VALIDATE(fe_editor_select_word_action);

    fe_EventLOCoords(context, event, &x, &y);

    EDT_DoubleClick(context, x, y); /* this takes screen co-ords?? */
#if 0
    /* can't do EDT_GetCurrentElementType() after EDT_DoubleClick()?? */
    fe_EditorUpdateToolbar(context, 0);
#endif
}

static void
fe_editor_dialog_action(Widget widget, XEvent *event, String *av,
				  Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_dialog_action);
    
    if (*ac == 1) {
	if (strcasecmp(av[0], "text") == 0)
	    fe_EditorPropertiesDialogDo(context,
					XFE_EDITOR_PROPERTIES_CHARACTER);
	else if (strcasecmp(av[0], "link") == 0)
	    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_LINK);
	else if (strcasecmp(av[0], "target") == 0)
	    fe_EditorTargetPropertiesDialogDo(context);
	else if (strcasecmp(av[0], "image") == 0)
	    fe_EditorPropertiesDialogDo(context, XFE_EDITOR_PROPERTIES_IMAGE);
	else if (strcasecmp(av[0], "hrule") == 0)
	    fe_EditorHorizontalRulePropertiesDialogDo(context);
	else if (strcasecmp(av[0], "tag") == 0)
	    fe_EditorHtmlPropertiesDialogDo(context);
	else if (strcasecmp(av[0], "document") == 0)
	    fe_EditorDocumentPropertiesDialogDo(context, 0); /* arg */
	else if (strcasecmp(av[0], "table") == 0)
	    fe_EditorTablePropertiesDialogDo(context, 0);
	else if (strcasecmp(av[0], "table-insert") == 0)
	    fe_EditorTableCreateDialogDo(context);
	else if (strcasecmp(av[0], "color") == 0)
	    fe_EditorSetColorsDialogDo(context);
	else if (strcasecmp(av[0], "publish") == 0)
	    fe_EditorPublishDialogDo(context);
	else
	    FE_SYNTAX_ERROR(fe_editor_dialog_action);

	return;
    }
    FE_SYNTAX_ERROR(fe_editor_dialog_action);
}

static char*
fe_editor_element_type_name(MWContext* context)
{
    ED_ElementType e_type; /* jag */

    e_type = EDT_GetCurrentElementType(context);

    switch (e_type) {
    case ED_ELEMENT_TEXT:        return "text";
    case ED_ELEMENT_IMAGE:       return "image";
    case ED_ELEMENT_UNKNOWN_TAG: return "tag";
    case ED_ELEMENT_TARGET:      return "target";
    case ED_ELEMENT_HRULE:       return "hrule";
    default:                     return "unknown";
    }
}

static fe_editor_click_action_last; /* this is very lazy */

static void
fe_editor_click_action(Widget widget, XEvent *event, String *av,
			    Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    char*      e_type;
    char*      c_type;
    char*      p_name = NULL;
    int        n;
    int        m;
    Time time;
    unsigned n_clicks = 1;
    unsigned c_clicks = 1;

    FE_ACTION_VALIDATE(fe_editor_click_action);

    time = fe_getTimeFromEvent(context, event);

    if (time - fe_editor_click_action_last <
	XtGetMultiClickTime(XtDisplay(widget)))
	n_clicks = 2;

    fe_editor_click_action_last = time;

    e_type = fe_editor_element_type_name(context);

    for (n = 0; n + 1 < *ac; n++) {

	if (strcasecmp(av[n], "double") == 0) {
	    c_clicks = 2;
	    n++;
	} else if (strcasecmp(av[n], "single") == 0) {
	    c_clicks = 1;
	    n++;
	} else {
	    c_clicks = 1;
	}
	    
	c_type = av[n++];
	
	if ((strcmp(c_type, "*") == 0 || strcasecmp(c_type, e_type) == 0)
	    &&
	    c_clicks == n_clicks) {
	    p_name = av[n++]; /* action name */
	}

	/* skip until "" or end */
	for (m = n; m < *ac && av[m] != NULL && av[m][0] != '\0'; m++)
	    ;

	if (p_name != NULL && p_name[0] != '\0') {
	    XtCallActionProc(widget, p_name, event, &av[n], m - n);
	    break;
	}
	n = m;
    }

    fe_EditorUpdateToolbar(context, 0);
}

static void
fe_editor_indent_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_indent_action);
    
    if (*ac == 0)
	fe_EditorIndent(context, FALSE);
    else if (*ac == 1 && strcasecmp(av[0], "in") == 0)
	fe_EditorIndent(context, FALSE);
    else if (*ac == 1 && strcasecmp(av[0], "out") == 0)
	fe_EditorIndent(context, TRUE);
    else
	FE_SYNTAX_ERROR(fe_editor_indent_action);
}

static void
fe_get_selected_text_rect(MWContext* context,
			  LO_TextStruct* text,
			  int32*         rect_x,
			  int32*         rect_y,
			  int32*         rect_width,
			  int32*         rect_height)
{
    PA_Block    text_save = text->text;
    int         len_save = text->text_len;
    LO_TextInfo info;
    
    text->text_len = text->sel_start;
    XFE_GetTextInfo(context, text, &info);

    *rect_x = text->x + text->x_offset + info.max_width;
    *rect_y = text->y + text->y_offset;

    text->text = (PA_Block)((char*)text->text + text->sel_start);
    text->text_len = text->sel_end - text->sel_start;
    
    XFE_GetTextInfo(context, text, &info);

    *rect_width = info.max_width;
    *rect_height = info.ascent + info.descent;

    text->text = text_save;
    text->text_len = len_save;
}

static Boolean
fe_editor_selection_contains_point(MWContext* context, int32 x, int32 y)
{
    int32 start_selection;
    int32 end_selection;
    LO_Element* start_element = NULL;
    LO_Element* end_element = NULL;
    LO_Element* lo_element;
    int32 rect_x;
    int32 rect_y;
    int32 rect_width;
    int32 rect_height;

    if (!EDT_IsSelected(context))
	return FALSE;

    LO_GetSelectionEndpoints(context,
			     &start_element,
			     &end_element,
			     &start_selection,
			     &end_selection);

    if (start_element == NULL)
	return FALSE;

    for (lo_element = start_element;
	 lo_element != NULL;
	 lo_element = ((LO_Any *)lo_element)->next) {

	if (lo_element->type == LO_TEXT &&
	    (lo_element == start_element || lo_element == end_element)) {
	    LO_TextStruct* text = (LO_TextStruct*)lo_element;

	    if (text->text == NULL) {
		if (text->prev != NULL && text->prev->type == LO_TEXT) {
		    text = (LO_TextStruct*)text->prev;
		} else {
		    text = (LO_TextStruct*)text->next;
		}
	    }
	    
	    if (text->text == NULL)
		continue;
	    
	    fe_get_selected_text_rect(context, text,
				      &rect_x,
				      &rect_y,
				      &rect_width,
				      &rect_height);
	    
	} else if (lo_element->type == LO_LINEFEED) {
	    continue;
	    
	} else {
	    LO_Any* lo_any = (LO_Any*)lo_element;
	    rect_x = lo_any->x + lo_any->x_offset;
	    rect_y = lo_any->y + lo_any->y_offset;
	    rect_width = lo_any->width;
	    rect_height = lo_any->height;
	}
	
	if (x > rect_x && y > rect_y &&
	    x < (rect_x + rect_width) && y < (rect_y + rect_height))
	    return TRUE;

	if (lo_element == end_element)
	    break;
    }

    return FALSE;
}

static void
fe_editor_popup_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);
    Widget menu;
    unsigned long x;
    unsigned long y;

    FE_ACTION_VALIDATE(fe_editor_popup_action);

    fe_UserActivity(context); /* tell the app who has focus */

    fe_EventLOCoords(context, event, &x, &y); /* get the doc co-ords */

    if (!fe_editor_selection_contains_point(context, x, y))
	EDT_SelectObject(context, x, y);
    
    menu = fe_EditorCreatePopupMenu(context);

    XmMenuPosition(menu, (XButtonPressedEvent *) event);

    XtManageChild(menu);
}

static char*
fe_lo_image_anchor(LO_ImageStruct* image,
		     long x, long y,
		     MWContext* context)
{
    struct fe_Pixmap *fep = (struct fe_Pixmap *) image->FE_Data;

    if (fep &&
	fep->type == fep_icon &&
	fep->data.icon_number == IL_IMAGE_DELAYED) { /* delayed image */

	long width, height;

	fe_IconSize(IL_IMAGE_DELAYED, &width, &height);

	if (image->alt &&
	    image->alt_len &&
	    (x > image->x + image->x_offset + 1 + 4 + width)) {
	    if (image->anchor_href) {
		return (char *)image->anchor_href->anchor;
	    }
	} /* else fall to end */
    } else if (image->image_attr->attrmask & LO_ATTR_ISFORM) { /* form */
	return "";
    }
    /*
     *    Internal editor image maybe?
     */
    else if ((image->image_attr->attrmask & LO_ATTR_INTERNAL_IMAGE) &&
	       EDT_IS_EDITOR(context) &&
	       image->alt != NULL) {
	return (char*)image->alt;
    }
    /*
     * This would be a client-side usemap image.
     */
    else if (image->image_attr->usemap_name != NULL) {
	
	LO_AnchorData *anchor_href;
	long ix = image->x + image->x_offset;
	long iy = image->y + image->y_offset;
	long mx = x - ix - image->border_width;
	long my = y - iy - image->border_width;
	
	anchor_href = LO_MapXYToAreaAnchor(context,
					   (LO_ImageStruct *)image,
					   mx, my);
	if (anchor_href) {
	    return (char *)anchor_href->anchor;
	} /* else fall to end */
    } else { /* some old random image */
	if (image->anchor_href) {
	    return (char *)image->anchor_href->anchor;
	}
    }

    return NULL;
}

static LO_Element*   fe_editor_motion_action_last_le;
static Boolean       fe_editor_motion_action_last_le_is_image;

static void
fe_editor_motion_action(Widget widget, XEvent *event,
			String *av, Cardinal *ac)
{
    MWContext*    context = fe_MotionWidgetToMWContext(widget);
    LO_Element*   le;
    unsigned long x, y;
    Cursor        cursor = None;
    char*         progress_string = NULL;
    char buf[80];
    char num[32];

    FE_ACTION_VALIDATE(fe_editor_motion_action);

    fe_EventLOCoords(context, event, &x, &y); /* get the doc co-ords */

    le = LO_XYToElement(context, (int32)x, (int32)y);

    if (le == fe_editor_motion_action_last_le &&
	!fe_editor_motion_action_last_le_is_image) {
	return; /* no change */
    }

    fe_editor_motion_action_last_le_is_image = FALSE;
    fe_editor_motion_action_last_le = le;
    
    if (le != NULL) {
	/*
	 *    Are we over text? Special cursor.
	 */
	if (le->type == LO_TEXT) {
	    cursor = CONTEXT_DATA(context)->editable_text_cursor;
	    if (le->lo_text.anchor_href) { /* link */
		progress_string = (char *)le->lo_text.anchor_href->anchor;
	    }
        /*
	 *    Over image, special progress.
	 */
	} else if (le->type == LO_IMAGE) {
	    progress_string = fe_lo_image_anchor(&le->lo_image, /* link ? */
						 x, y,
						 context);
	    if (progress_string != NULL && progress_string[0] == '\0')
		progress_string = NULL;
	    
	    if (progress_string == NULL) {

		if (le->lo_image.image_url) {
		    progress_string = (char*)le->lo_image.image_url;
		} else if (le->lo_image.lowres_image_url) {
		    progress_string = (char*)le->lo_image.lowres_image_url;
		}

		if (progress_string != NULL) {
		    sprintf(num, ",%d,%d",
			    x - le->lo_image.x, y - le->lo_image.y);
		    FE_CondenseURL(buf, progress_string,
				   sizeof(buf) - 1 - strlen(num));
		    strcat(buf, num);
		    progress_string = buf;
		    fe_editor_motion_action_last_le_is_image = TRUE;
		}
	    }
	}
    }

    if (CONTEXT_DATA(context)->clicking_blocked ||
	CONTEXT_DATA(context)->synchronous_url_dialog) {
	cursor = CONTEXT_DATA(context)->busy_cursor;
    }

    if (CONTEXT_DATA(context)->drawing_area) {
	XDefineCursor(XtDisplay(CONTEXT_DATA(context)->drawing_area),
		      XtWindow(CONTEXT_DATA(context)->drawing_area),
		      cursor);
    }

    if (progress_string == NULL)
	progress_string = "";

    if (CONTEXT_DATA(context)->active_url_count == 0) {
	/* If there are transfers in progress, don't document the URL under
	   the mouse, since that message would interfere with the transfer
	   messages.  Do change the cursor, however. */
	fe_MidTruncatedProgress(context, progress_string);
    }
}

static void
fe_editor_debug_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

#if 1
    FE_Alert(context, "I see no bugs here!!!");
#endif
}

static void
fe_editor_unimplemented_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
    MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

    FE_ACTION_VALIDATE(fe_editor_unimplemented_action);

    FE_Alert(context, "unimplimented action");
}


#define EA_PREFIX "editor-"

static XtActionsRec fe_editor_actions [] =
{
  { EA_PREFIX "debug",	                 fe_editor_debug_action },
  
  { EA_PREFIX "new",	                 fe_editor_new_action },
  { EA_PREFIX "delete",	                 fe_editor_delete_action },
  { EA_PREFIX "save",	                 fe_editor_save_action },
  { EA_PREFIX "reload",	                 fe_editor_reload_action },
  { EA_PREFIX "open-file",	         fe_editor_open_file_action },
  { EA_PREFIX "open-location",	         fe_editor_open_url_action },
  { EA_PREFIX "edit-source",	         fe_editor_edit_source_action },
  { EA_PREFIX "dialog",	                 fe_editor_dialog_action },
  { EA_PREFIX "popup-menu",	         fe_editor_popup_action },

  { EA_PREFIX "self-insert",	         fe_editor_key_input_action },
  { EA_PREFIX "beginning-of-line",       fe_editor_beginning_of_line_action },
  { EA_PREFIX "end-of-line",             fe_editor_end_of_line_action },
  { EA_PREFIX "backward-character",      fe_editor_backward_character_action },
  { EA_PREFIX "forward-character",       fe_editor_forward_character_action },
  { EA_PREFIX "delete-previous-character", fe_editor_delete_previous_action },
  { EA_PREFIX "delete-next-character",   fe_editor_delete_next_action },
  { EA_PREFIX "process-up",              fe_editor_process_up_action }, 
  { EA_PREFIX "process-down",            fe_editor_process_down_action }, 
  { EA_PREFIX "process-return",          fe_editor_process_return_action },
  { EA_PREFIX "grab-focus",              fe_editor_grab_focus_action },
  { EA_PREFIX "extend-start",            fe_editor_extend_start_action },
  { EA_PREFIX "extend-adjust",           fe_editor_extend_adjust_action },
  { EA_PREFIX "extend-end",              fe_editor_extend_end_action },
  { EA_PREFIX "motion",                  fe_editor_motion_action },

  { EA_PREFIX "undo",                    fe_editor_undo_action },
  { EA_PREFIX "redo",                    fe_editor_redo_action },
  { EA_PREFIX "insert-line-break",       fe_editor_insert_line_break_action },
  { EA_PREFIX "insert-non-breaking-space", 
                            fe_editor_insert_non_breaking_space_action },
  { EA_PREFIX "toggle-character-properties", 
                           fe_editor_toggle_character_properties_action },
  { EA_PREFIX "set-character-properties", 
                              fe_editor_set_character_properties_action },
  { EA_PREFIX "set-paragraph-properties",
                              fe_editor_set_paragraph_properties_action },
  { EA_PREFIX "set-font-size",           fe_editor_set_font_size_action },
  { EA_PREFIX "increase-font-size",      fe_editor_increase_font_size_action },
  { EA_PREFIX "decrease-font-size",      fe_editor_decrease_font_size_action },
  { EA_PREFIX "refresh",                 fe_editor_refresh_action },
  { EA_PREFIX "cut",                     fe_editor_cut_action },
  { EA_PREFIX "copy",                    fe_editor_copy_action },
  { EA_PREFIX "paste",                   fe_editor_paste_action },
  { EA_PREFIX "select-all",              fe_editor_select_all_action },
  { EA_PREFIX "select-word",             fe_editor_select_word_action },
  { EA_PREFIX "click",                   fe_editor_click_action },
  { EA_PREFIX "indent",                  fe_editor_indent_action },

  { EA_PREFIX "process-cancel",          fe_editor_unimplemented_action }, 
  { EA_PREFIX "set-anchor",              fe_editor_unimplemented_action },
  { EA_PREFIX "backward-word",           fe_editor_unimplemented_action },
  { EA_PREFIX "forward-word",            fe_editor_unimplemented_action },
  { EA_PREFIX "copy-clipboard",          fe_editor_unimplemented_action },

  { EA_PREFIX "find",                    fe_editor_find_action },
  { EA_PREFIX "find-again",              fe_editor_find_again_action },

  { EA_PREFIX "update-toolbar",          fe_editor_update_toolbar_action }
};

static void
fe_ActionSyntaxAlert(MWContext*    context,
		     XtActionProc  action_proc,
		     String* av, Cardinal* ac)
{
    char buf[1024];
    char name[1024];

    fe_action_name(name,
		   fe_editor_actions, countof(fe_editor_actions),
		   action_proc, av, ac);
    sprintf(buf, XP_GetString(XFE_ACTION_SYNTAX_ERROR), name);

    FE_Alert(context, buf);
}

static void
fe_ActionWrongContextAlert(MWContext*    context,
			   XtActionProc  action_proc,
			   String* av, Cardinal* ac)
{
    char buf[1024];
    char name[1024];

    fe_action_name(name,
		   fe_editor_actions, countof(fe_editor_actions),
		   action_proc, av, ac);
    sprintf(buf, XP_GetString(XFE_ACTION_WRONG_CONTEXT), name);

    if (context)
	FE_Alert(context, buf);
    else
	fprintf(real_stderr, buf);
}

void
fe_EditorInitActions()
{
  XtAppAddActions(
		  fe_XtAppContext,
		  fe_editor_actions,
		  countof(fe_editor_actions)
		  );
}

#endif /* EDITOR */
