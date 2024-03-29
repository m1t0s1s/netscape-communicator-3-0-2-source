/* -*- Mode:C; tab-width: 8 -*-
   dialogs.c --- General UI functions used elsewhere.
   Copyright � 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Jun-94.
 */

/* #define DOCINFO_SOURCE_TEXT */
#define DOCINFO_VISUAL_TEXT

#include "mozilla.h"
#include "net.h"             /* for fe_makeSecureTitle */
#include "xlate.h"
#include "xfe.h"
#include "felocale.h"
#include "outline.h"
#include "mailnews.h"

#include <X11/IntrinsicP.h>  /* for w->core.height */
#include <X11/CoreP.h>	     /* for w->core.widget_class->core_class.expose */
#include <X11/ShellP.h>      /* To access popped_up field */

#include <Xm/FileSBP.h>      /* for hacking FS lossage */

#include <Xm/XmAll.h>
#include <Xm/CascadeB.h>

#if 0
#include <Xfe/XfeP.h>			/* for xfe widgets and utilities */
#endif

#include "libi18n.h"
#if 0
#include "intl_csi.h"
#endif

#include "np.h"
#include "xp_trace.h"
#if 0
#include <layers.h>
#include "xeditor.h"
#include "xp_qsort.h"
#endif

/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_OPEN_FILE;
extern int XFE_ERROR_OPENING_FILE;
extern int XFE_ERROR_OPENING_PIPE;
extern int XFE_NO_SUBJECT;
extern int XFE_UNKNOWN_ERROR_CODE;
extern int XFE_INVALID_FILE_ATTACHMENT_DOESNT_EXIST;
extern int XFE_INVALID_FILE_ATTACHMENT_NOT_READABLE;
extern int XFE_INVALID_FILE_ATTACHMENT_IS_A_DIRECTORY;
extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_X_RESOURCES_NOT_INSTALLED_CORRECTLY;
extern int XFE_OUTBOX_CONTAINS_MSG;
extern int XFE_OUTBOX_CONTAINS_MSGS;
extern int XFE_CONTINUE_MOVEMAIL;
extern int XFE_CANCEL_MOVEMAIL;
extern int XFE_MOVEMAIL_EXPLANATION;
extern int XFE_SHOW_NEXT_TIME;
extern int XFE_MAIL_SPOOL_UNKNOWN;
extern int XFE_CANT_SAVE_PREFS;
#if 0
extern int XFE_JAVASCRIPT_APP;
extern int XFE_DIALOGS_PRINTING;
extern int XFE_DIALOGS_DEFAULT_VISUAL_AND_COLORMAP;
extern int XFE_DIALOGS_DEFAULT_VISUAL_AND_PRIVATE_COLORMAP;
extern int XFE_DIALOGS_NON_DEFAULT_VISUAL;
extern int XFE_DIALOGS_FROM_NETWORK;
extern int XFE_DIALOGS_FROM_DISK_CACHE;
extern int XFE_DIALOGS_FROM_MEMORY_CACHE;
extern int XFE_DIALOGS_DEFAULT;
#endif

#define XfeIsAlive(w) (!((w)->core.being_destroyed))
#define XfeNumPopups(w) ((w)->core.num_popups)
#define XfePopupListIndex(w,i) ((w)->core.popup_list[(i)])
#define XfeShellIsPoppedUp(w) (((ShellWidget)(w))->shell.popped_up)
#define XfeIsViewable(w) (XfeShellIsPoppedUp((w)))
#define _XfeWidth(w) ((w)->core.width)
#define _XfeHeight(w) ((w)->core.height)
#define _XfeBorderWidth(w) ((w)->core.border_width)
#define XfeWidth(w) _XfeWidth((w))
#define XfeHeight(w) _XfeHeight((w))
#define XfeBorderWidth(w) _XfeBorderWidth((w))
#define XP_QSORT qsort


#if XmVersion >= 2000
extern void _XmOSBuildFileList(String,String,unsigned char,String * *,
			       unsigned int *,unsigned int *);

extern char * _XmStringGetTextConcat(XmString);

extern int _XmOSFileCompare(XmConst void *,XmConst void *);

extern String _XmOSFindPatternPart(String);

extern void _XmOSQualifyFileSpec(String,String,String *,String *);

extern XmGeoMatrix _XmGeoMatrixAlloc(unsigned int,unsigned int,unsigned int);

extern Boolean _XmGeoSetupKid(XmKidGeometry,Widget);

extern void _XmMenuBarFix(XmGeoMatrix,int,XmGeoMajorLayout,XmKidGeometry);

extern void _XmSeparatorFix(XmGeoMatrix,int,XmGeoMajorLayout,XmKidGeometry);

extern void _XmDestroyParentCallback(Widget,XtPointer,XtPointer);

#endif /* XmVersion >= 2000 */


#define DOCINFO_CHARSET_TEXT

/* Kludge around conflicts between Motif and xp_core.h... */
#undef Bool
#define Bool char

typedef enum {
  Answer_Invalid = -1,
  Answer_Cancel = 0,
  Answer_OK,
  Answer_Apply,
  Answer_Destroy } Answers;

struct fe_confirm_data {
  MWContext *context;
  Answers answer;
  void *return_value;
  void *return_value_2;
  Widget widget;
  Widget text, text2;
  Boolean must_match;
};

#if 0
#ifdef MOZ_MAIL_NEWS
extern void fe_getMessageBody(MWContext *context, char **pBody, uint32 *body_size, MSG_FontCode **font_changes);
extern void fe_doneWithMessageBody(MWContext *context, char *pBody, uint32 body_size);
#endif
#endif

extern const char* FE_GetFolderDirectory(MWContext* context);

/*static void fe_confirm_cb (Widget, XtPointer, XtPointer);*/
static void fe_clear_text_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_ok_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_apply_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_cancel_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_finish_cb (Widget, XtPointer, XtPointer);

static void fe_destroy_snarf_text_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_snarf_pw_cb (Widget, XtPointer, XtPointer);
static void fe_destroy_snarf_pw2_cb (Widget, XtPointer, XtPointer);

void fe_browse_file_of_text (MWContext *context, Widget text_field, Boolean dirp);

extern Widget fe_toplevel; /* #### gag! */

#if 0
/*
 *    A real Info dialog - with ! instead of error icon.
 *    Now we don't have to have "error: no new messages on server"
 */
void
FE_Message(MWContext * context, const char* message)
{
	if (context && context->type != MWContextBiff)
		fe_Message(context, message);
}

#endif /* 0 */

/* FE_Alert is no longer defined in fe_proto.h */
void
FE_Alert (MWContext *context, const char *message)
{
  XP_ASSERT(context);
  if (context)
      XFE_Alert (context, message);
  else
    {
      /* Ok, we haven't got a context. This is a very bad
       * situation; we're gonna exit, but let's try and
       * find a context to use so we can notify the user.
       */
      if (fe_all_MWContexts && fe_all_MWContexts->context)
	XFE_Alert (fe_all_MWContexts->context, message);

      else if (fe_toplevel) /* You fucking pinheads. */
	fe_Alert_2 (fe_toplevel, message);
      else
	{
	  /* So that didn't even work. Write to stdout and
	   * exit.
	   */
	  XP_ABORT((message));
	}
    }
}

#if 0
/* Hack for 109371 */
void
FE_Alert_modal (MWContext *context, const char *message)
{
  if (context && context->type == MWContextBiff)
	return;

  if (context)
      fe_Alert_modal(CONTEXT_WIDGET (context), message);
  else
    {
      Widget toplevel = FE_GetToplevelWidget();
      if ( toplevel ) {
	      fe_Alert_2(toplevel, message);
      } else {
	  /* So that didn't even work. Write to stdout and
	   * exit.
	   */
	  XP_ABORT((message));
	}
    }
}

/* Lame hack to get the right title for javascript dialogs.
 * This should be instead added to the call arguments.
 */
static MWContext *javaScriptCallingContextHack = 0;
#endif /* 0 */

/* Display a message, and wait for the user to click "Ok".
   A pointer to the string is not retained.
   The call to FE_Alert() returns immediately - it does not
   wait for the user to click OK.
 */
void
XFE_Alert (MWContext *context, const char *message)
{
#if 0
  /* Keep the context around, so we can pull the domain name
   * full the dialog title.
   */
  if (context->bJavaScriptCalling)
	javaScriptCallingContextHack = context;
#endif /* 0 */

  fe_Alert_2(CONTEXT_WIDGET (context), message);
}

/* Just like XFE_Alert, but with a different dialog title. */
void
fe_stderr (MWContext *context, const char *message)
{
  (void) fe_dialog (CONTEXT_WIDGET (context),
		    (fe_globalData.stderr_dialog_p &&
		     fe_globalData.stdout_dialog_p
		     ? "stdout_stderr"
		     : fe_globalData.stderr_dialog_p ? "stderr"
		     : fe_globalData.stdout_dialog_p ?  "stdout"  : "???" ),
		    message, FALSE, 0, FALSE, FALSE, 0);
}


/* Let the user confirm or deny assertion `message' (returns True or False.)
   A pointer to the prompt-string is not retained.
 */
XP_Bool
XFE_Confirm (MWContext *context, const char *message)
{
  return fe_Confirm_2 (CONTEXT_WIDGET (context), message);
}

#if 0
/*
 * Yet Another front end function that
 * noone told us we were supposed to implement.
 */
PRBool
XP_Confirm(MWContext *c, const char *msg)
{
    return XFE_Confirm(c,msg);
}
#endif /* 0 */

Boolean
fe_Confirm_2 (Widget parent, const char *message)
{
  return (Bool) ((int) fe_dialog (parent, "question", message,
				  TRUE, 0, TRUE, FALSE, 0));
}

#if 0
void
fe_Alert_modal (Widget parent, const char *message)
{
  (void) fe_dialog (parent, "error", message, FALSE, 0, TRUE, FALSE, 0);
}
#endif /* 0 */

void
fe_Alert_2 (Widget parent, const char *message)
{
  (void) fe_dialog (parent, "error", message, FALSE, 0, FALSE, FALSE, 0);
}

#if 1
/* Prompt the user for a string.
   If the user selects "Cancel", 0 is returned.
   Otherwise, a newly-allocated string is returned.
   A pointer to the prompt-string is not retained.
 */
char *
XFE_Prompt (MWContext *context, const char *message, const char *deflt)
{
  return (char *) fe_dialog (CONTEXT_WIDGET (context), "prompt", message,
			     TRUE, (deflt ? deflt : ""), TRUE, FALSE, 0);
}

char *
FE_PromptMessageSubject (MWContext *context)
{
  const char *def = XP_GetString( XFE_NO_SUBJECT );
  return (char *) fe_dialog (CONTEXT_WIDGET (context), "promptSubject",
			     0, TRUE, def, TRUE, TRUE, 0);
}
#endif /* 1 */

#define HAVE_SYSERRLIST
#if !defined(HAVE_SYSERRLIST)
#include <sys/errno.h>
extern char *sys_errlist[];
extern int sys_nerr;
#endif

/* Like perror, but with a dialog.
 */
void
fe_perror (MWContext *context, const char *message)
{
  fe_perror_2 (CONTEXT_WIDGET (context), message);
}

void
fe_perror_2 (Widget parent, const char *message)
{
  int e = errno;
  const char *es = 0;
  char buf1 [2048];
  char buf2 [512];
  char *b = buf1;

#ifdef DEBUG_jwz  /* this is the modern way */
  if (e >= 0)
    es = strerror (e);
  if (!es || !*es)
#else /* !DEBUG_jwz */
  if (e >= 0 && e < sys_nerr)
    {
      es = sys_errlist [e];
    }
  else
#endif /* !DEBUG_jwz */
    {
      PR_snprintf (buf2, sizeof (buf2), XP_GetString( XFE_UNKNOWN_ERROR_CODE ),
		errno);
      es = buf2;
    }
  if (message)
    PR_snprintf (buf1, sizeof (buf1), "%.900s\n%.900s", message, es);
  else
    b = buf2;
  fe_Alert_2 (parent, b);
}

void
fe_UnmanageChild_safe (Widget w)
{
  if (w) XtUnmanageChild (w);
}


void
fe_NukeBackingStore (Widget widget)
{
  XSetWindowAttributes attrs;
  unsigned long attrmask;

  if (!XtIsTopLevelShell (widget))
    widget = XtParent (widget);
  XtRealizeWidget (widget);

  attrmask = CWBackingStore | CWSaveUnder;
  attrs.backing_store = NotUseful;
  attrs.save_under = False;
  XChangeWindowAttributes (XtDisplay (widget), XtWindow (widget),
			   attrmask, &attrs);
}


Widget

#ifdef OSF1
fe_CreateTextField (Widget parent, char *name, Arg *av, int ac)
#else
fe_CreateTextField (Widget parent, const char *name, Arg *av, int ac)
#endif
{
  Widget w;

#if 1
  w = XmCreateTextField (parent, (char *) name, av, ac);
#else
  XtSetArg (av[ac], XmNeditMode, XmSINGLE_LINE_EDIT); ac++;
  w = XmCreateTextField (parent, (char *) name, av, ac);
#endif

  fe_HackTextTranslations (w);
  return w;
}

Widget
fe_CreateText (Widget parent, const char *name, Arg *av, int ac)
{
  Widget w = XmCreateText (parent, (char *) name, av, ac);
  fe_HackTextTranslations (w);
  return w;
}


static void
fe_select_text(Widget text)
{
  XmTextPosition first = 0;
  XmTextPosition last = XmTextGetLastPosition (text);
  XP_ASSERT (XtIsRealized(text));
  XmTextSetSelection (text, first, last,
		      XtLastTimestampProcessed (XtDisplay (text)));
}


/* The purpose of this function is try disable all grabs and settle
 * focus issues. This will be called before we popup a dialog
 * (modal or non-modal). This needs to do all these:
 *	- if a menu was posted, unpost it.
 *	- if a popup menu was up, pop it down.
 *	- if an option menu, pop it down.
 */
void fe_fixFocusAndGrab(MWContext *context)
{
  Widget focus_w;
  Widget mainw;
  XEvent event;
  int i;

  mainw = CONTEXT_WIDGET(context);
  focus_w = XmGetFocusWidget(mainw);

  /* Unpost Menubar */
  if (focus_w && XmIsCascadeButton(focus_w) &&
      XmIsRowColumn(XtParent(focus_w))) {
    /* 1. Found the menubar. Unpost it.
     *		To do that, we makeup a dummy event and use it with the
     *		CleanupMenuBar() action.
     * WARNING: if focus_w was a XmCascadeButtonGadget we wont be able to
     *		do this.
     */
    event.xany.type = 0;
    event.xany.serial = 0;
    event.xany.send_event = 0;
    event.xany.display = fe_display;
    event.xany.window = XtWindow(focus_w);
    XtCallActionProc(focus_w, "CleanupMenuBar", &event, NULL, 0);
  }

  /* Identify and Popdown any OptionMenu that was active */
  if (focus_w && XmIsRowColumn(XtParent(focus_w))) {
    unsigned char type;
    Widget w;
    XtVaGetValues(XtParent(focus_w), XmNrowColumnType, &type, 0);
    if (type == XmMENU_OPTION) {
      XtVaGetValues(focus_w, XmNsubMenuId, &w, 0);
      if (w) XtUnmanageChild(w);
    }
  }
  
  /* Identify and Popdown any popup menus that were active */
  for (i=0; i < XfeNumPopups(mainw); i++) {
    Widget popup = XfePopupListIndex(mainw,i);
    if (XtIsShell(popup) && XmIsMenuShell(popup))
      if (XfeShellIsPoppedUp(popup)) {
#ifdef DEBUG_dora
	  printf("popdown... name %s shell is popped up\n", XtName(popup));
#endif
	XtPopdown(popup);
      }
  }
}

#if 0
static char *fe_makeSecureTitle( MWContext *context )
{
  History_entry *h;
  char *domain = 0;
  char *title = 0;

  if( !context )
	{
	  return title;
	}
  
  h = SHIST_GetCurrent (&context->hist);
  
  if (!h || !h->address) return title;
  
  domain = NET_ParseURL(h->address, GET_HOST_PART);

  if (domain) {
	title = PR_smprintf("%s - %s", domain, XP_GetString(XFE_JAVASCRIPT_APP));
	XP_FREE(domain);
  } else {
	title = PR_smprintf("%s", XP_GetString(XFE_JAVASCRIPT_APP));
  }
  
  return title;
}

void *
fe_prompt (MWContext *context, Widget mainw,
	   const char *title, const char *message,
	   XP_Bool question_p, const char *default_text,
	   XP_Bool wait_p, XP_Bool select_p,
	   char **passwd)
{
  /* Keep the context around, so we can pull the domain name
   * full the dialog title.
   */
  if (context->bJavaScriptCalling)
	javaScriptCallingContextHack = context;

  return fe_dialog(mainw, title, message, question_p,
				   default_text, wait_p, select_p, passwd);
}
#endif /* 0 */


/* This function is a complete piece of shit - it takes a billion flags
   and does everything in the world because somehow I thought it would save
   me a bunch of lines of code but it's just a MESS!
 */
void *
fe_dialog (Widget mainw,
	   const char *title, const char *message,
	   XP_Bool question_p, const char *default_text,
	   XP_Bool wait_p, XP_Bool select_p,
	   char **passwd)
{
  Widget shell, dialog;
  Widget text = 0, pwtext = 0;
  XmString xm_message = 0;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  char title2 [255];
  struct fe_MWContext_cons *cons;
  int i;
  char *javaScriptTitle = 0;  /* dialog title when javascript calls */
  XP_Bool javaScriptCalling = FALSE;
      
  strcpy (title2, title);
  strcat (title2, "_popup");

#if 0
  /* Use this global variable hackery to get the context */
  if (javaScriptCallingContextHack) {
	javaScriptTitle = fe_makeSecureTitle(javaScriptCallingContextHack);
	javaScriptCallingContextHack = NULL;
    javaScriptCalling = TRUE;
  }
#endif /* 0 */

  if (!mainw) {
    /* Yikes.  Well, this can happen if someone calls FE_Alert() on a biff
       context or some other context without a window.  We could ignore it,
       but I think it better to try and put the dialog up *someplace*.  So,
       look for some context with a reasonable window. */
    for (cons = fe_all_MWContexts ; cons && !mainw; cons = cons->next) {
      mainw = CONTEXT_WIDGET(cons->context);
    }
    if (!mainw) return NULL;
  }

  while(!XtIsWMShell(mainw) && (XtParent(mainw)!=0))
    mainw = XtParent(mainw);

#if 0
  /* so, if the context that is popping up the dialog is hidden, we need
     to pop it up _somewhere_, so we run down the list of contexts *again*,
     and pop it up over one of them.  This should work.  I think. */
  if (!XfeIsViewable(mainw))
    {
      Widget attempt = mainw;
      /* instead of popping up the dialog at (0,0) pop it up over
	 the active context */
      for (cons = fe_all_MWContexts ; 
	   cons && !XfeIsViewable(attempt); 
	   cons = cons->next) 
	{
	  attempt = CONTEXT_WIDGET(cons->context);

	  while(!XtIsWMShell(attempt) && (XtParent(attempt)!=0))
	    attempt = XtParent(attempt);
	}

      /* oh well, we tried. */
      if (attempt)
	mainw = attempt;
    }
#endif /* 0 */

  /* If any dialog is already up we will cascade these dialogs. Thus this
   * dialog will be the child of the last popped up dialog.
   */
  i = XfeNumPopups(mainw);
  while (i>0)
    if (XmIsDialogShell(XfePopupListIndex(mainw,i-1)) &&
	(XfeIsViewable(XfePopupListIndex(mainw,i-1)))) {
      /* Got a new mainw */
      Widget newmainw = XfePopupListIndex(mainw,i-1);
#ifdef DEBUG_dp
      fprintf(stderr, "Using mainw: %s[%x] (%s[%x] num_popup=%d)\n",
	      XtName(newmainw), newmainw,
	      XtName(mainw), mainw, i);
#endif /* DEBUG_dp */
      mainw = newmainw;
      i = XfeNumPopups(mainw);
    }
  else i--;

  /* Popdown any popup menu that was active. If not, this could get motif
   * motif really confused as to who has focus as the new dialog that we
   * are going to popup will undo a grab that the popup did without the
   * popup's knowledge.
   */
  for (cons = fe_all_MWContexts ; cons; cons = cons->next)
    fe_fixFocusAndGrab(cons->context);

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  shell = XmCreateDialogShell (mainw, title2, av, ac);

  /* If it is java script that is posting this, use this special title */
  if (javaScriptTitle) {
	XtVaSetValues (shell, XmNtitle, javaScriptTitle, 0);
	free(javaScriptTitle);
  }

  ac = 0;
  if (message)
    xm_message = XmStringCreateLtoR ((char *) message,
				     XmFONTLIST_DEFAULT_TAG);
  XtSetArg (av[ac], XmNdialogStyle, (wait_p
				     ? XmDIALOG_FULL_APPLICATION_MODAL
				     : XmDIALOG_MODELESS)); ac++;
  XtSetArg (av[ac], XmNdialogType,
	    (default_text
             ? XmDIALOG_MESSAGE
             : (question_p
                ? XmDIALOG_QUESTION
                : (javaScriptCalling
                   ? XmDIALOG_WARNING
                   : XmDIALOG_ERROR)))); ac++;
  if (xm_message) XtSetArg (av[ac], XmNmessageString, xm_message), ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  dialog = XmCreateMessageBox (shell, "dialog", av, ac);
  if (xm_message) XmStringFree (xm_message);

#ifdef NO_HELP
  fe_UnmanageChild_safe (XtNameToWidget (dialog, "*Help"));
#endif


  if (! question_p)
    {
      Widget cancel = 0;
      XtVaGetValues (dialog, XmNcancelButton, &cancel, 0);
      if (! cancel) abort ();
      XtUnmanageChild (cancel);
    }

  if (default_text)
    {
      Widget clear;
      Widget text_parent = dialog;
      Widget ulabel = 0, plabel = 0;

      text = 0;
      pwtext = 0;

      if (passwd && passwd != (char **) 1)
	{
	  ac = 0;
	  text_parent = XmCreateForm (dialog, "form", av, ac);
	  ulabel = XmCreateLabelGadget (text_parent, "userLabel", av, ac);
	  plabel = XmCreateLabelGadget (text_parent, "passwdLabel", av, ac);
	  XtVaSetValues (ulabel,
			 XmNtopAttachment, XmATTACH_FORM,
			 XmNbottomAttachment, XmATTACH_WIDGET,
			 XmNbottomWidget, plabel,
			 XmNleftAttachment, XmATTACH_FORM,
			 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			 XmNrightWidget, plabel,
			 0);
	  XtVaSetValues (plabel,
			 XmNtopAttachment, XmATTACH_NONE,
			 XmNbottomAttachment, XmATTACH_FORM,
			 XmNleftAttachment, XmATTACH_FORM,
			 XmNrightAttachment, XmATTACH_NONE,
			 0);
	}

      if (passwd != (char **) 1)
	{
	  ac = 0;
	  text = fe_CreateTextField (text_parent, "text", av, ac);
	  fe_SetTextField(text, default_text);
	  XtVaSetValues (text, XmNcursorPosition, 0, 0);
	}

      if (passwd)
	{
	  ac = 0;
	  pwtext = fe_CreateTextField (text_parent, "pwtext", av, ac);
	}

      if (text && pwtext)
	{
	  if (fe_globalData.nonterminal_text_translations)
	    XtOverrideTranslations (text, fe_globalData.
				    nonterminal_text_translations);
				    
	  XtVaSetValues (text,
			 XmNtopAttachment, XmATTACH_FORM,
			 XmNbottomAttachment, XmATTACH_WIDGET,
			 XmNbottomWidget, pwtext,
			 XmNleftAttachment, XmATTACH_WIDGET,
			 XmNleftWidget, ulabel,
			 XmNrightAttachment, XmATTACH_FORM,
			 0);
	  XtVaSetValues (pwtext,
			 XmNtopAttachment, XmATTACH_NONE,
			 XmNbottomAttachment, XmATTACH_FORM,
			 XmNleftAttachment, XmATTACH_WIDGET,
			 XmNleftWidget, plabel,
			 XmNrightAttachment, XmATTACH_FORM,
			 0);
	  XtManageChild (ulabel);
	  XtManageChild (plabel);
	}

      if (text)
	XtManageChild (text);
      if (pwtext)
	XtManageChild (pwtext);

      if (text && pwtext)
	XtManageChild (text_parent);

      XtVaSetValues (text_parent, XmNinitialFocus, (text ? text : pwtext), 0);
      XtVaSetValues (dialog, XmNinitialFocus, (text ? text : pwtext), 0);

      ac = 0;
      clear = XmCreatePushButtonGadget (dialog, "clear", av, ac);
      if (pwtext)
	XtAddCallback (clear, XmNactivateCallback, fe_clear_text_cb, pwtext);
      if (text)
	XtAddCallback (clear, XmNactivateCallback, fe_clear_text_cb, text);
      XtManageChild (clear);
    }

  if (! wait_p)
    {
      XtAddCallback (dialog, XmNokCallback, fe_destroy_cb, shell);
      XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cb, shell);
      XtManageChild (dialog);
      if (text && select_p)
	fe_select_text (text);
      return False;
    }
  else
    {
      struct fe_confirm_data data;
      void *ret_val = 0;
      
      /*      data.context = context; */
      data.answer = Answer_Invalid;
      data.widget = shell;
      data.text  = (text ? text : pwtext);
      data.text2 = ((text && pwtext) ? pwtext : 0);
      data.return_value = 0;
      data.return_value_2 = 0;

      XtVaSetValues (shell, XmNdeleteResponse, XmDESTROY, 0);
      XtAddCallback (dialog, XmNokCallback, fe_destroy_ok_cb, &data);
      XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cancel_cb, &data);
      XtAddCallback (dialog, XmNdestroyCallback, fe_destroy_finish_cb, &data);

      if (text)
	XtAddCallback (text, XmNdestroyCallback, fe_destroy_snarf_text_cb,
		       &data);

      if (text && pwtext)
	XtAddCallback (pwtext, XmNdestroyCallback, fe_destroy_snarf_pw2_cb,
		       &data);
      else if (pwtext)
	XtAddCallback (pwtext, XmNdestroyCallback, fe_destroy_snarf_pw_cb,
		       &data);

      if (pwtext)
	fe_SetupPasswdText (pwtext, 1024);

      fe_NukeBackingStore (dialog);
      XtManageChild (dialog);

      if (text)
	{
	  XtVaSetValues (text, XmNcursorPosition, 0, 0);
	  XtVaSetValues (text, XmNcursorPositionVisible, True, 0);
	  if (select_p)
	    fe_select_text (text);
	}

      while (data.answer == Answer_Invalid)
	fe_EventLoop ();

      if (default_text)
	{
	  if (data.answer == Answer_OK)	/* user clicked OK at the text */
	    {
	      if (text && pwtext)
		{
		  if (!data.return_value || !data.return_value_2) abort ();
		  *passwd = data.return_value_2;
		  ret_val = data.return_value;
		}
	      else if (text || pwtext)
		{
		  if (! data.return_value) abort ();
		  ret_val = data.return_value;
		}
	      else
		abort ();
	    }
	  else			/* user clicked cancel */
	    {
	      if (data.return_value)
		free (data.return_value);
	      if (data.return_value_2)
		free (data.return_value_2);
	      ret_val = 0;
	    }
	}
      else
	{
	  ret_val = (data.answer == Answer_OK
		     ? (void *) True
		     : (void *) False);
	}

      /* We need to suck the values out of the widgets before destroying them.
       */
      /* Ok, with the new way, it got destroyed by the callbacks on the
	 OK and Cancel buttons. */
      /* XtDestroyWidget (shell); */
      return ret_val;
    }
}


void
fe_Message (MWContext *context, const char *message)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget shell, dialog;
  XmString xm_message;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  shell = XmCreateDialogShell (mainw, "message_popup", av, ac);
  ac = 0;
  xm_message = XmStringCreateLtoR ((char *) message, XmFONTLIST_DEFAULT_TAG);
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_MESSAGE); ac++;
  XtSetArg (av[ac], XmNmessageString, xm_message); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  dialog = XmCreateMessageBox (shell, "message", av, ac);
  XmStringFree (xm_message);

#ifdef NO_HELP
  fe_UnmanageChild_safe (XtNameToWidget (dialog, "*Help"));
#endif

  {
    Widget cancel = 0;
    XtVaGetValues (dialog, XmNcancelButton, &cancel, 0);
    if (! cancel) abort ();
    XtUnmanageChild (cancel);
  }
  XtAddCallback (dialog, XmNokCallback, fe_destroy_cb, shell);
  XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cb, shell);
  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);
}

static void
fe_destroy_ok_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  data->answer = Answer_OK;
  XtDestroyWidget(data->widget);
}

static void
fe_destroy_cancel_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  data->answer = Answer_Cancel;
  XtDestroyWidget(data->widget);
}

static void
fe_destroy_apply_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  data->answer = Answer_Apply;
  XtDestroyWidget(data->widget);
}

static void
fe_destroy_finish_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  if (data->answer == Answer_Invalid)
    data->answer = Answer_Destroy;
}

static void
fe_destroy_snarf_text_cb (Widget widget, XtPointer closure,XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  Widget text = data->text;
  if (text)
    {
      char *plaintext = 0;
      data->return_value = 0;
      plaintext = fe_GetTextField(text);
      data->return_value = XP_STRDUP(plaintext ? plaintext : "");
    }
}

static void
fe_destroy_snarf_pw_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  Widget text = data->text;
  if (text)
    data->return_value = fe_GetPasswdText (text);
}

static void
fe_destroy_snarf_pw2_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  Widget text = data->text2;
  if (text)
    data->return_value_2 = fe_GetPasswdText (text);
}


/* Callback used by the clear button to nuke the contents of the text field. */
static void
fe_clear_text_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  Widget text = (Widget) closure;
  fe_SetTextField (text, "");
  /* Focus on the text widget after this, since otherwise you have to
     click again. */
  XmProcessTraversal (text, XmTRAVERSE_CURRENT);
}

/* When we are not blocking waiting for a response to a dialog, this is used
   by the buttons to make it get lost when no longer needed (the default would
   otherwise be to merely unmanage it instead of destroying it.) */
static void
fe_destroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
#if 0
  Widget w = (Widget) closure;
  Widget shell = XtParent (widget);
  if (!XtIsTopLevelShell (shell))
    shell = XtParent (shell);
  XtDestroyWidget (w);
#else
  Widget shell = XtParent (widget);
  while (!XmIsDialogShell (shell))
    shell = XtParent (shell);
  XtDestroyWidget (shell);
#endif
}



/* Create a XmString that will render in the given width or less.  If
   the given string is too big, then chop out stuff by clipping out the
   center and replacing it with "...". */
XmString
fe_StringChopCreate(char* message, char* tag, XmFontList font_list,
		    int maxwidth)
{
  XmString label = XmStringCreate ((char *) message, tag);
  int string_width;
/*  uint16 csid;*/

  if (!font_list) return label;

  string_width = XmStringWidth(font_list, label);
  if (string_width >= maxwidth) {
    /* The string is bigger than the label.  Mid-truncate it. */

    XmString try;
    int length = 0;
    int maxlength = XP_STRLEN(message);
    int tlen;
    int hlen;
    char* text = XP_ALLOC(maxlength + 10);
    char* end = message + maxlength;
    if (!text) return label;
      
    string_width = 0;
    while (string_width < maxwidth && length < maxlength) {
      length++;
      tlen = length / 2;
      hlen = length - tlen;
      strncpy(text, message, hlen);
      strncpy(text + hlen, "...", 3);
      strncpy(text + hlen + 3, end - tlen, tlen);
      text[length + 3] = '\0';
      try = XmStringCreate(text, tag);
      if (!try) break;
      string_width = XmStringWidth(font_list, try);
      if (string_width >= maxwidth) {
	XmStringFree(try);
      } else {
	XmStringFree(label);
	label = try;
      }
      try = 0;
    }

    free (text);
  }

  return label;
}

#if 1
/* Print a status message in the wholine.
   A pointer to the string is not retained.
 */
void
XFE_Progress (MWContext *context, const char *message)
{
  Widget w = CONTEXT_DATA (context)->wholine;
  XmString oldlabel = 0;
  char *oldlabelStr = NULL;
  XmString label;
  XP_Bool needsChange = True;

#ifdef GRIDS
  if (context->is_grid_cell)
  {
    MWContext *top_context = context;
    while ((top_context->grid_parent)&&
	   (top_context->grid_parent->is_grid_cell))
      {
	top_context = top_context->grid_parent;
      }
    if (top_context->grid_parent)
      w = CONTEXT_DATA (top_context->grid_parent)->wholine;
  }
#endif /* GRIDS */

  /* wholine wont be there if no decoration is available. */
  if (!w) return;

  if (message == 0 || *message == '\0') {
    message = context->defaultStatus;
    if (message == 0) message = "";
  }
  XtVaGetValues (w, XmNlabelString, &oldlabel, 0);
  if (XmStringGetLtoR(oldlabel, XmFONTLIST_DEFAULT_TAG, &oldlabelStr)) {
    if (oldlabelStr && !strcmp(oldlabelStr, (char *)message))
      needsChange = False;
    if (oldlabelStr) XtFree(oldlabelStr);
  }
  if (needsChange) {
    label = XmStringCreate ((char *) message, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues (w, XmNlabelString, label, 0);
    XmStringFree (label);
  }
  XmStringFree (oldlabel);
  XFlush(fe_display);

#if 0
  /* Fucking Motif clears out the label as soon as we change its value,
     and waits for a GraphicsExposure to come in before redrawing it.
     This means that if you set a label and don't return to the event
     loop for a while, the label will be blank for a long time.  So,
     invoke its redraw method by hand (you might think to cons up a
     fake Exposure event and just call XtDispatchEvent on it -- but NO!
     Because there's no way to process a single, particular event
     without also processing timer and file-descriptor events, which
     would be catastrophic to us!
   */
  {
    XExposeEvent event;
    Region region1, region2;
    XRectangle rect;
    XtExposeProc redisplay = w->core.widget_class->core_class.expose;
    if (redisplay == XtInheritExpose)
      abort();
    if (XmIsGadget(w))
      w = w->core.parent;
    event.type = Expose;
    event.serial = 0;
    event.send_event = 0;
    event.display = XtDisplay (w);
    event.window = XtWindow (w);
    event.x = 0;
    event.y = 0;
    event.width = w->core.width;
    event.height = w->core.height;
    event.count = 0;

    rect.x = event.x;
    rect.y = event.y;
    rect.width = event.height;
    rect.height = event.height;

    region1 = XCreateRegion();
    region2 = XCreateRegion();
    XUnionRectWithRegion (&rect, region1, region2);
    XDestroyRegion(region1);
    redisplay (w, &event, region2);
    XDestroyRegion(region2);
  }
#endif /* 0 */
}

void
fe_MidTruncatedProgress (MWContext *context, const char *message)
{
  Widget w = CONTEXT_DATA (context)->wholine;
  XmFontList font_list = 0;
  XmString oldlabel = 0;
  XmString label;
  Dimension widget_width;

#ifdef GRIDS
  if (context->is_grid_cell)
  {
    MWContext *top_context = context;
    while ((top_context->grid_parent)&&
	   (top_context->grid_parent->is_grid_cell))
      {
	top_context = top_context->grid_parent;
      }
    if (top_context->grid_parent)
      w = CONTEXT_DATA (top_context->grid_parent)->wholine;
  }
#endif /* GRIDS */
  /* wholine wont be there if no decoration is available. */
  if (!w) return;
  if (message == 0 || *message == '\0') {
    message = context->defaultStatus;
    if (message == 0) message = "";
  }
  XtVaGetValues (w, XmNfontList, &font_list, 0);
  widget_width = w->core.width - 20; /* #### random slop */
  label = fe_StringChopCreate((char*) message, XmFONTLIST_DEFAULT_TAG,
			      font_list, widget_width);

  XtVaGetValues (w, XmNlabelString, &oldlabel, 0);
  if (!XmStringCompare(label, oldlabel)) {
    XtVaSetValues (w, XmNlabelString, label, 0);
  }
  XmStringFree (label);
  XmStringFree (oldlabel);
}
#endif /* 1 */


/* Dealing with field/value pairs.

   This takes a "value" widget and an arbitrary number of "label" widgets.
   It finds the widest label, and attaches the "value" widget to the left
   edge of its parent with an offset such that it falls just beyond the
   right edge of the widest label widget.

   The argument list is expected to be NULL-terminated.
 */

/* #ifdef AIXV3 */
/*    I don't understand - this seems not to be broken any more, and now
      the "hack" way doesn't compile either.  Did someone upgrade me?? */
/* # define BROKEN_VARARGS */
/* #endif */

#ifdef BROKEN_VARARGS
/* Sorry, we can't be bothered to implement varargs correctly. */
# undef va_list
# undef va_start
# undef va_arg
# undef va_end
# define va_list int
# define va_start(list,first_arg) list = 0
# define va_arg(list, type) (++list, (type) (list == 1 ? varg0 : \
				             list == 2 ? varg1 : \
				             list == 3 ? varg2 : \
				             list == 4 ? varg3 : \
				             list == 5 ? varg4 : \
				             list == 6 ? varg5 : \
				             list == 7 ? varg6 : \
				             list == 8 ? varg7 : \
				             list == 9 ? varg8 : 0))
# define va_end(list)
#endif


void
#ifdef BROKEN_VARARGS
fe_attach_field_to_labels (Widget value_field,
			   void *varg0, void *varg1, void *varg2, void *varg3,
			   void *varg4, void *varg5, void *varg6, void *varg7,
			   void *varg8)
#else
fe_attach_field_to_labels (Widget value_field, ...)
#endif
{
  va_list vargs;
  Widget current;
  Widget widest = 0;
  Position left = 0;
  long max_width = -1;

  va_start (vargs, value_field);
  while ((current = va_arg (vargs, Widget)))
    {
      Dimension width = 0;
      Position left = 0;
      Position right = 10;

#ifdef Motif_doesnt_suck
	/* Getting the leftOffset of a Gadget may randomly dump core
	   (those fuckers). */
      if (XmIsGadget (current))
	abort ();
      XtVaGetValues (current,
		     XmNwidth, &width,
		     XmNleftOffset, &left,
/*		     XmNrightOffset, &right,*/
		     0);

#else
      width = XfeWidth(current);
#endif

      width += (left + right);
      if (((long) width) > max_width)
	{
	  widest = current;
	  max_width = (long) width;
	}
    }
  va_end (vargs);

  if (! widest)
    abort ();
#ifdef Motif_doesnt_suck
  XtVaGetValues (value_field, XmNleftOffset, &left, 0);
#endif
  XtVaSetValues (value_field,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, max_width + left,
		 0);
}


/* Files
 */
static XmString fe_file_selection_directory;

/*
 * This code tried to make some minor improvements to the standard
 * Motif 1.2* File Selection Box dialog. The code here attempts
 * to use the existing Motif code for the bulk of the work, and to
 * replace some parts of Motif's behavior with something simpler.
 * I've also added some nice things conceptually lifted from the Mac
 * file picker, and at the request of users. In hindsight it might
 * have been easier to just write a new file picker widget, preferably
 * one that is semantically compatible with the Motif widget. Next time.
 *
 * Anyway, this stuff is conditionally compiled. Most of this is
 * controlled by the USE_WINNING_FILE_SELECTION define. When
 * you turn this guy on, it enables a bunch of hacks to the standard
 * Motif behavior. Best to take a look, but below is a snippet from
 * the resource file. Note that some of the hacks (the simple layout
 * hacks) require Motif source knowledge because they must attack the
 * FSB class record directly. We've moved that code out (see the
 * define FEATURE_MOTIF_FSB_CLASS_HACKING) because we can't make the
 * Motif code public.
 *
 * Related to these hacks, but somewhat orthogonal to them, we try to
 * save the last accessed directory. This makes life a little easier
 * than always going back to (say) HOME.
 *
 * From the resource file:
 *
 * This resource enables some hacks to the File Selection Box
 * which use a simpler layout algorithm than the standard Motif
 * code. The directory and file listings are always maintained
 * as equal size. We don't do this for SGI, as they have solved
 * the problem. See *useEnhancedFSB above.
 * @NOTSGI@*nsMotifFSBHacks: True
 *
 * This resource enables the CDE mode of the File Selection Box.
 * You must be running a CDE enhanced Motif library for this
 * option to work. This resource will apply pathMode=1 to the
 * File Selection Box. If you specify nsMotifFSBHacks and
 * nsMotifFSBCdeMode, some of the keyboard and filter enhancements
 * of nsMotifFSBHacks will also be applied, the layout hacks will not.
 * *nsMotifFSBCdeMode: False
 *
 * ...djw
 */

#define USE_WINNING_FILE_SELECTION

#ifdef USE_WINNING_FILE_SELECTION

#include <Xm/XmP.h>
#include <Xm/XmosP.h>

static void
fe_force_configure(Widget widget)
{
    Dimension width = _XfeWidth(widget);
    Dimension height = _XfeHeight(widget);
    Dimension border_width = _XfeBorderWidth(widget);

    _XfeHeight(widget) += 1;
    XtResizeWidget(widget, width, height, border_width);
}

static void
fe_file_selection_get_dir_entries(String directory, String pattern,
				  unsigned type,
				  String** file_list_r, unsigned int* nfiles_r)
{
    unsigned int n;
    unsigned len = strlen(directory);

    *file_list_r = NULL;
#ifdef XM_GET_DIR_ENTRIES_OK
    _XmOSGetDirEntries(directory, pattern, type, FALSE, FALSE,
		       file_list_r, nfiles_r, &n);
#else
    _XmOSBuildFileList(directory, pattern, type,
		       file_list_r, nfiles_r, &n);

    for (n = 0; n < *nfiles_r; n++) {
        String old_s = (*file_list_r)[n];
	String new_s;

	if (strncmp(directory, old_s, len) == 0) {
	    new_s = (String)XtNewString(&old_s[len]);
	    XtFree(old_s);
	    (*file_list_r)[n] = new_s;
	}
    }
#endif
}

static void 
fe_file_selection_file_search_proc(Widget widget, XtPointer call_data)
{
    XmFileSelectionBoxCallbackStruct* search_data
	= (XmFileSelectionBoxCallbackStruct *)call_data;
    String    directory;
    String    pattern;
    String*   file_list;
    unsigned int nfiles;
    Arg       args[10];
    Cardinal  n;
    Cardinal  nn;
    XmString* xm_string_file_list;
    XmString  xm_directory;

    if (!(directory = _XmStringGetTextConcat(search_data->dir))) {
	return;
    }

    if (!(pattern = _XmStringGetTextConcat(search_data->pattern))) {
	XtFree(directory);
	return;
    }

    /*
     *    _XmOSGetDirEntries() is being really wierd! Use this guy
     *    instead..djw
     */
    file_list = NULL;
    fe_file_selection_get_dir_entries(directory, pattern, XmFILE_REGULAR,
				      &file_list, &nfiles);

    for (n = 0, nn = 0; n < nfiles; n++) {
		if (file_list[n][0] == '.') /* no more dot files ! */
			continue;
		
		file_list[nn++] = file_list[n];
	}
	nfiles = nn;
	
    if (nfiles != 0) {
	if (nfiles > 1)
	    XP_QSORT((void *)file_list, nfiles, sizeof(char *), _XmOSFileCompare);

	xm_string_file_list = (XmString*)XtMalloc(nfiles * sizeof(XmString));

	for (n = 0; n < nfiles; n++) {
	    xm_string_file_list[n] = XmStringLtoRCreate(file_list[n],
						XmFONTLIST_DEFAULT_TAG);
	}
    } else {
	xm_string_file_list = NULL;
    }

    /* The Motif book says update the XmNdirectory */
    xm_directory = XmStringLtoRCreate(directory, XmFONTLIST_DEFAULT_TAG);

    /* Update the list widget */
    n = 0;
    XtSetArg(args[n], XmNfileListItems, xm_string_file_list); n++;
    XtSetArg(args[n], XmNfileListItemCount, nfiles); n++;
    XtSetArg(args[n], XmNlistUpdated, TRUE); n++;
    XtSetArg(args[n], XmNdirectory, xm_directory); n++;
#if 0
	XtSetArg(args[n], XmNdirectoryValid, TRUE); n++;
#endif
    XtSetValues(widget, args, n);

#if 1
    fe_force_configure(widget);
#endif

    /* And save directory default for next time */
    if (fe_file_selection_directory)
	XmStringFree(fe_file_selection_directory);
    fe_file_selection_directory = xm_directory;

    if (nfiles != 0) {
	/*
	 *    Cleanup.
	 */
	for (n = 0; n < nfiles; n++) {
	    XmStringFree(xm_string_file_list[n]);
	    XtFree(file_list[n]);
	}
	XtFree((char *)xm_string_file_list);
    }

    XtFree((char *)directory);
    XtFree((char *)pattern);
    
    if (file_list)
	XtFree((char *)file_list);
}

static void 
fe_file_selection_dir_search_proc(Widget widget, XtPointer call_data)
{
    XmFileSelectionBoxCallbackStruct* search_data
	= (XmFileSelectionBoxCallbackStruct *)call_data;
    String          directory;
    Arg             args[10];
    Cardinal        n;
    Cardinal        m;
    String *        dir_list;
    unsigned int    ndirs;
    unsigned int    mdirs;
    XmString *      xm_string_dir_list;
    XmString        xm_directory;
    int attempts;
    char* p;

    XtVaGetValues(widget, XmNdirectory, &xm_directory, 0);

    if (XmStringByteCompare(xm_directory, search_data->dir) == True) {
	/* Update the list widget */
	n = 0;
	XtSetArg(args[n], XmNlistUpdated, FALSE); n++;
	XtSetArg(args[n], XmNdirectoryValid, TRUE); n++;
	XtSetValues(widget, args, n);
	return;
    }

    directory = _XmStringGetTextConcat(search_data->dir);

    ndirs = 0;
    for (attempts = 0; ndirs == 0; attempts++) {
	dir_list = NULL;
	fe_file_selection_get_dir_entries(directory, "*", XmFILE_DIRECTORY,
					  &dir_list, &ndirs);
	if (ndirs) {
	    if (attempts != 0) {
		XmFileSelectionBoxCallbackStruct new_data;
	        XmQualifyProc q_proc;

		/*
		 *    Cleanup old stuff that won't be re-used.
		 */
		XmStringFree(search_data->dir);
		if (search_data->mask)
		    XmStringFree(search_data->mask);

		memset(&new_data, 0, sizeof(XmFileSelectionBoxCallbackStruct));
		new_data.reason = search_data->reason;
		new_data.event = search_data->event;
		new_data.dir = XmStringLtoRCreate(directory,
						  XmFONTLIST_DEFAULT_TAG);
		new_data.dir_length = XmStringLength(new_data.dir);
		new_data.pattern = search_data->pattern;
		new_data.pattern_length = search_data->pattern_length;

		/*
		 *    Reset the spec, because we may get called 50M
		 *    times if this broken state.
		 */
		XtVaSetValues(widget,
			      XmNdirectory, new_data.dir,
			      XmNdirSpec, new_data.dir,
			      0);

		/*
		 *    Call qualify proc to install new directory
		 *    into widget.
		 */
		XtVaGetValues(widget, XmNqualifySearchDataProc, &q_proc, 0);
		(*q_proc)(widget, (XtPointer)&new_data,
			          (XtPointer)search_data);
	    }
	} else {
	    if (attempts == 0) {
#ifdef DEBUG
		char   buf[1024];
		
		sprintf(buf, "Unable to access directory:\n  %.900s\n",
			directory);
		fprintf(stderr, buf);
#endif
		XBell(XtDisplay(widget), 0); /* emulate Motif */
	    }

	    if (dir_list)
		XtFree((char *) dir_list);

	    if (directory[0] == '\0' || strcmp(directory, "/") == 0)
		return; /* I give in! */

	    while ((p = strrchr(directory, '/')) != NULL) {
		if (p[1] == '\0') { /* "/" at end */
		    p[0] = '\0';
		} else {
		    p[1] = '\0';
		    break;
		}
	    }
	}
    }

    if (ndirs > 1)
	XP_QSORT((void *)dir_list, ndirs, sizeof(char *), _XmOSFileCompare);
    
    xm_string_dir_list = (XmString *)XtMalloc(ndirs * sizeof(XmString));

    for (m = n = 0; n < ndirs; n++) {
	if (strcmp(dir_list[n], ".") == 0) /* don't want that */
	    continue;
	xm_string_dir_list[m++] = XmStringLtoRCreate(dir_list[n],
						     XmFONTLIST_DEFAULT_TAG);
    } 
    mdirs = m;

    /* Update the list widget */
    n = 0;
    XtSetArg(args[n], XmNdirListItems, xm_string_dir_list); n++;
    XtSetArg(args[n], XmNdirListItemCount, mdirs); n++;
    XtSetArg(args[n], XmNlistUpdated, TRUE); n++;
    XtSetArg(args[n], XmNdirectoryValid, TRUE); n++;
    XtSetValues(widget, args, n);

    /* And save directory default for next time */
    xm_directory = XmStringLtoRCreate(directory, XmFONTLIST_DEFAULT_TAG);
    if (fe_file_selection_directory)
	XmStringFree(fe_file_selection_directory);
    fe_file_selection_directory = xm_directory;

    /*
     *    Cleanup.
     */
    for (n = 0; n < mdirs; n++) {
	XmStringFree(xm_string_dir_list[n]);
    }    
    for (n = 0; n < ndirs; n++) {
	XtFree(dir_list[n]);
    } 

    XtFree((char *)xm_string_dir_list);
    XtFree((char *)directory);
}

static void
fe_file_selection_qualify_search_data_proc(Widget widget,
					   XtPointer sd, XtPointer qsd)
{
    XmFileSelectionBoxCallbackStruct* s_data
	= (XmFileSelectionBoxCallbackStruct *)sd;
    XmFileSelectionBoxCallbackStruct* qs_data
	= (XmFileSelectionBoxCallbackStruct *)qsd;
    String mask_string = _XmStringGetTextConcat(s_data->mask);
    String dir_string = _XmStringGetTextConcat(s_data->dir);
    String pattern_string = _XmStringGetTextConcat(s_data->pattern);
    String dir_part = NULL;
    String pattern_part = NULL;
    String q_dir_string = NULL;
    String q_mask_string = NULL;
    String q_pattern_string = NULL;
    String dir_free_string = NULL;
    String pattern_free_string = NULL;
    String p;
    XmString xm_directory;
    XmString xm_pattern;
    XmString xm_dir_spec;

    if (dir_string != NULL) {
	if (dir_string[0] == '/') {
	    dir_part = dir_string;
	} else {
	    XtVaGetValues(widget, XmNdirectory, &xm_directory, 0);
	    if (xm_directory != NULL) {
		dir_part = _XmStringGetTextConcat(xm_directory);
		p = (String)XtMalloc(strlen(dir_part) +
				     strlen(dir_string) + 2);
		strcpy(p, dir_part);
		strcat(p, dir_string);
		XtFree(dir_string);
		XtFree(dir_part);
		dir_part = dir_string = p;
	    }
	}
    } else {
	if (mask_string != NULL) { /* use filter value */
	    pattern_part = _XmOSFindPatternPart(mask_string);

	    if (pattern_part != mask_string) {
		pattern_part[-1] =  '\0'; /* zap dir trailing '/' */
		/* "" or "/" */
		if (*mask_string == '\0') {
		    dir_part = "/";
		} else if (*mask_string == '/' && mask_string[1] == '\0') {
		    dir_part = "//";
		} else {
		    dir_part = mask_string;
		}
	    }
	} else { /* use XmNdirectory */
	    XtVaGetValues(widget, XmNdirectory, &xm_directory, 0);
	    if (xm_directory != NULL) {
		dir_part = _XmStringGetTextConcat(xm_directory);
		dir_free_string = dir_part; /* to force free */
	    }
	}
    }

    if (pattern_string != NULL) {
	pattern_part = pattern_string;
    } else {
	if (mask_string != NULL) {
	    if (!pattern_part)
		pattern_part = _XmOSFindPatternPart(mask_string);
	} else {
	    XtVaGetValues(widget, XmNpattern, &xm_pattern, 0);
	    if (xm_pattern != NULL) {
		pattern_part = _XmStringGetTextConcat(xm_pattern);
		pattern_free_string = pattern_part; /* so it gets freed */
	    }
	}
    }

    /*
     *    It's ok for dir_part to be NULL, as _XmOSQualifyFileSpec
     *    will just use the cwd.
     */
    _XmOSQualifyFileSpec(dir_part, pattern_part,
			 &q_dir_string, &q_pattern_string);

    qs_data->reason = s_data->reason ;
    qs_data->event = s_data->event ;
    if (s_data->value) {
	qs_data->value = XmStringCopy(s_data->value);
    } else {
	XtVaGetValues(widget, XmNdirSpec, &xm_dir_spec, 0);
	qs_data->value = XmStringCopy(xm_dir_spec);
    }
    qs_data->length = XmStringLength(qs_data->value) ;
    q_mask_string = (String)XtMalloc(strlen(q_dir_string) +
				     strlen(q_pattern_string) + 1);
    strcpy(q_mask_string, q_dir_string);
    strcat(q_mask_string, q_pattern_string);
    qs_data->mask = XmStringLtoRCreate(q_mask_string, XmFONTLIST_DEFAULT_TAG);
    qs_data->mask_length = XmStringLength(qs_data->mask);
    qs_data->dir = XmStringLtoRCreate(q_dir_string, XmFONTLIST_DEFAULT_TAG);
    qs_data->dir_length = XmStringLength(qs_data->dir) ;
    qs_data->pattern = XmStringLtoRCreate(q_pattern_string,
					  XmFONTLIST_DEFAULT_TAG);
    qs_data->pattern_length = XmStringLength(qs_data->pattern);

    if (dir_free_string)
	XtFree(dir_free_string);
    if (pattern_free_string)
	XtFree(pattern_free_string);
    if (dir_string)
	XtFree(dir_string);
    if (pattern_string)
	XtFree(pattern_string);
    if (mask_string)
	XtFree(mask_string);
    if (q_dir_string)
	XtFree(q_dir_string);
    if (q_pattern_string)
	XtFree(q_pattern_string);
    if (q_mask_string)
	XtFree(q_mask_string);
}

static char* fe_file_selection_home = 0;

static char*
fe_file_selection_gethome()
{
  if (!fe_file_selection_home) {
    char* foo = getenv("HOME");
    char* p;

    if (!foo)
      foo = "/"; /* hah */

    fe_file_selection_home = XP_STRDUP(foo);
		
    p = strrchr(fe_file_selection_home, '/');
    if (p != NULL && p != fe_file_selection_home && p[1] == '\0')
      *p = '\0';
  }

  return fe_file_selection_home;
}

static void
fe_file_selection_dirspec_cb(Widget widget, XtPointer closure, XtPointer cb)
{
    XmTextVerifyCallbackStruct* vd = (XmTextVerifyCallbackStruct*)cb;
    Widget fsb = (Widget)closure;
    XmString xm_directory;
    String directory;
    String ptr;

    if (vd->startPos == 0 &&
	vd->text->ptr != NULL) {

      if (vd->text->ptr[0] == '~') { /* expand $HOME */

	char* home = fe_file_selection_gethome();

	ptr = (String)XtMalloc(strlen(home) + strlen(vd->text->ptr) + 2);

	strcpy(ptr, home);
	strcat(ptr, "/");
	if (vd->text->length > 1)
	  strcat(ptr, &vd->text->ptr[1]);

	XtFree(vd->text->ptr);
	vd->text->ptr = ptr;
	vd->text->length = strlen(ptr);
			
      } else if (vd->text->ptr[0] != '/') {
	XtVaGetValues(fsb, XmNdirectory, &xm_directory, 0);
	if (xm_directory != NULL) {
	  directory = _XmStringGetTextConcat(xm_directory);
				
	  ptr = (String)XtMalloc(strlen(directory) +
				 strlen(vd->text->ptr) + 2);
	  strcpy(ptr, directory);
	  strcat(ptr, vd->text->ptr);
				
	  XtFree(vd->text->ptr);
	  vd->text->ptr = ptr;
	  vd->text->length = strlen(ptr);
	}
      }
    }
}

static void
fe_file_selection_filter_cb(Widget apply, XtPointer closure, XtPointer cbd)
{
	Widget fsb = (Widget)closure;
	Widget dir = XmFileSelectionBoxGetChild(fsb, XmDIALOG_DIR_LIST);
	int*   position_list;
	int    position_count;

	if (XmListGetSelectedPos(dir, &position_list, &position_count)) {

		if (position_count == 1 && position_list[0] == 1)
			XmListSelectPos(dir, 1, True);
			
		if (position_count > 0 && position_list != NULL)
			XtFree((char*)position_list);
	}
}

static void
fe_file_selection_directory_up_action(Widget widget,
									  XEvent* event,
									  String* av, Cardinal* ac)
{
    XmString xm_directory;
    String   directory;
    String   p;

	while (!XtIsSubclass(widget, xmFileSelectionBoxWidgetClass)) {
		widget = XtParent(widget);
		if (!widget)
			return;
	}

	XtVaGetValues(widget, XmNdirectory, &xm_directory, 0);

	if (xm_directory != NULL) {
		int len;

		directory = _XmStringGetTextConcat(xm_directory);

		len = XP_STRLEN(directory);

		if (len > 0 && directory[len-1] == '/')
			directory[len-1] = '\0';

		if ((p = XP_STRRCHR(directory, '/')) != NULL) {
			p[1] = '\0';

			xm_directory = XmStringCreateSimple(directory);

			XtVaSetValues(widget, XmNdirectory, xm_directory, 0);

			XmStringFree(xm_directory);
		}
		XtFree(directory);
	}
}

static XtActionsRec fe_file_selection_actions[] =
{
	{ "FileSelectionBoxCdDotDot", fe_file_selection_directory_up_action },
};

#ifdef DEBUG_jwz
# define _XmConst const
#endif

static _XmConst char fe_file_selection_accelerators[] =
"~Alt Meta<Key>osfUp:   FileSelectionBoxCdDotDot()\n"
"Alt ~Meta<Key>osfUp:   FileSelectionBoxCdDotDot()";

static unsigned fe_file_selection_add_actions_done;

static XtPointer
fe_file_selection_register_actions_mappee(Widget widget, XtPointer data)
{
	XtOverrideTranslations(widget, (XtTranslations)data);
	return 0;
}

static void
fe_file_selection_register_actions(Widget widget)
{
	if (!fe_file_selection_add_actions_done) {
		XtAppAddActions(fe_XtAppContext,
						fe_file_selection_actions,
						countof(fe_file_selection_actions));
		fe_file_selection_add_actions_done++;
	}

	fe_WidgetTreeWalk(widget,
					  fe_file_selection_register_actions_mappee,
					  (XtPointer) XtParseTranslationTable(fe_file_selection_accelerators));
}

#if 0
/*
 *    Lifted from Motif 1.2.2. Copyright lots of people.
 */
#define IsButton(w) ( \
      XmIsPushButton(w)   || XmIsPushButtonGadget(w)   || \
      XmIsToggleButton(w) || XmIsToggleButtonGadget(w) || \
      XmIsArrowButton(w)  || XmIsArrowButtonGadget(w)  || \
      XmIsDrawnButton(w))

#define IsAutoButton(fsb, w) (                \
      w == SB_OkButton(fsb) ||                \
      w == SB_ApplyButton(fsb) ||     \
      w == SB_CancelButton(fsb) ||    \
      w == SB_HelpButton(fsb))

#define SetupWorkArea(fsb) \
    if (_XmGeoSetupKid (boxPtr, SB_WorkArea(fsb)))    \
    {                                                 \
        layoutPtr->space_above = vspace;              \
        vspace = BB_MarginHeight(fsb);                \
        boxPtr += 2 ;                                 \
        ++layoutPtr ;                                 \
    }
 
static Boolean 
FileSelectionBoxNoGeoRequest(
        XmGeoMatrix geoSpec )
{
/****************/

    if(    BB_InSetValues( geoSpec->composite)
        && (XtClass( geoSpec->composite) == xmFileSelectionBoxWidgetClass)    )
    {   
        return( TRUE) ;
        } 
    return( FALSE) ;
    }

static XmGeoMatrix 
my_FileSBGeoMatrixCreate(
        Widget wid,
        Widget instigator,
        XtWidgetGeometry *desired )
{
    XmFileSelectionBoxWidget fsb = (XmFileSelectionBoxWidget) wid ;
    XmGeoMatrix     geoSpec ;
    register XmGeoRowLayout  layoutPtr ;
    register XmKidGeometry   boxPtr ;
    XmKidGeometry   firstButtonBox ; 
    Boolean         dirListLabelBox ;
    Boolean         listLabelBox ;
    Boolean         dirListBox ;
    Boolean         listBox ;
    Boolean         selLabelBox ;
    Boolean         filterLabelBox ;
    Dimension       vspace = BB_MarginHeight(fsb);
    int             i;

/*
 * Layout FileSelectionBox XmGeoMatrix.
 * Each row is terminated by leaving an empty XmKidGeometry and
 * moving to the next XmGeoRowLayout.
 */

    geoSpec = _XmGeoMatrixAlloc( XmFSB_MAX_WIDGETS_VERT,
                              fsb->composite.num_children, 0);
    geoSpec->composite = (Widget) fsb ;
    geoSpec->instigator = (Widget) instigator ;
    if(    desired    )
    {   geoSpec->instig_request = *desired ;
        } 
    geoSpec->margin_w = BB_MarginWidth( fsb) + fsb->manager.shadow_thickness ;
    geoSpec->margin_h = BB_MarginHeight( fsb) + fsb->manager.shadow_thickness ;
    geoSpec->no_geo_request = FileSelectionBoxNoGeoRequest ;

    layoutPtr = &(geoSpec->layouts->row) ;
    boxPtr = geoSpec->boxes ;

    /* menu bar */
 
    for (i = 0; i < fsb->composite.num_children; i++)
    {   Widget w = fsb->composite.children[i];

        if(    XmIsRowColumn(w)
            && ((XmRowColumnWidget)w)->row_column.type == XmMENU_BAR
            && w != SB_WorkArea(fsb)
            && _XmGeoSetupKid( boxPtr, w)    )
        {   layoutPtr->fix_up = _XmMenuBarFix ;
            boxPtr += 2;
            ++layoutPtr;
            vspace = 0;		/* fixup space_above of next row. */
            break;
            }
        }

    /* work area, XmPLACE_TOP */

    if (fsb->selection_box.child_placement == XmPLACE_TOP)
      SetupWorkArea(fsb);

    /* filter label */

    filterLabelBox = FALSE ;
    if(    _XmGeoSetupKid( boxPtr, FS_FilterLabel( fsb))    )
    {   
        filterLabelBox = TRUE ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(fsb);
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* filter text */

    if(    _XmGeoSetupKid( boxPtr, FS_FilterText( fsb))    )
    {   
        if(    !filterLabelBox    )
        {   layoutPtr->space_above = vspace;
            vspace = BB_MarginHeight(fsb);
            } 
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* dir list and file list labels */

    dirListLabelBox = FALSE ;
    if(    _XmGeoSetupKid( boxPtr, FS_DirListLabel( fsb))    )
    {   
        dirListLabelBox = TRUE ;
        ++boxPtr ;
        } 
    listLabelBox = FALSE ;
    if(    _XmGeoSetupKid( boxPtr, SB_ListLabel( fsb))    )
    {   
        listLabelBox = TRUE ;
        ++boxPtr ;
        } 
    if(    dirListLabelBox  ||  listLabelBox    )
    {
		/* hacked by djw */
		layoutPtr->fix_up = 0 ;
        layoutPtr->fit_mode = XmGEO_PROPORTIONAL ;
        layoutPtr->fill_mode = XmGEO_EXPAND ;
        layoutPtr->even_width = 1 ; /* special case _XmGeoBoxesSameWidth() */
		/* end hacked by djw */

        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(fsb);
        layoutPtr->space_between = BB_MarginWidth( fsb) ;

        if(    dirListLabelBox && listLabelBox    )
        {   layoutPtr->sticky_end = TRUE ;
            } 
        ++boxPtr ;
        ++layoutPtr ;
        } 

    /* dir list and file list */

    dirListBox = FALSE ;
    if(     FS_DirList( fsb)  &&  XtIsManaged( FS_DirList( fsb))
        &&  _XmGeoSetupKid( boxPtr, XtParent( FS_DirList( fsb)))    )
    {   
        dirListBox = TRUE ;
        ++boxPtr ;
        } 
    listBox = FALSE ;
    if(    SB_List( fsb)  &&  XtIsManaged( SB_List( fsb))
        && _XmGeoSetupKid( boxPtr, XtParent( SB_List( fsb)))    )
    {   
        listBox = TRUE ;
        ++boxPtr ;
        } 
    if(    dirListBox  || listBox    )
    {
		/* hacked by djw */
		layoutPtr->fix_up = 0 ;
        layoutPtr->fit_mode = XmGEO_PROPORTIONAL ;
        layoutPtr->fill_mode = XmGEO_EXPAND ;
        layoutPtr->even_width = 1 ; /* special case _XmGeoBoxesSameWidth() */
		/* end hacked by djw */

        layoutPtr->space_between = BB_MarginWidth( fsb) ;
        layoutPtr->stretch_height = TRUE ;
        layoutPtr->min_height = 70 ;
        layoutPtr->even_height = 1 ;
        if(    !listLabelBox  &&  !dirListLabelBox    )
        {   layoutPtr->space_above = vspace;
            vspace = BB_MarginHeight(fsb);
            } 
        ++boxPtr ;
        ++layoutPtr ;
        } 

    /* work area, XmPLACE_ABOVE_SELECTION */

    if (fsb->selection_box.child_placement == XmPLACE_ABOVE_SELECTION)
      SetupWorkArea(fsb)

    /* selection label */

    selLabelBox = FALSE ;
    if(    _XmGeoSetupKid( boxPtr, SB_SelectionLabel( fsb))    )
    {   selLabelBox = TRUE ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(fsb);
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* selection text */

    if(    _XmGeoSetupKid( boxPtr, SB_Text( fsb))    )
    {   
        if(    !selLabelBox    )
        {   layoutPtr->space_above = vspace;
            vspace = BB_MarginHeight(fsb);
            } 
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* work area, XmPLACE_BELOW_SELECTION */

    if (fsb->selection_box.child_placement == XmPLACE_BELOW_SELECTION)
      SetupWorkArea(fsb)

    /* separator */

    if(    _XmGeoSetupKid( boxPtr, SB_Separator( fsb))    )
    {   layoutPtr->fix_up = _XmSeparatorFix ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(fsb);
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* button row */

    firstButtonBox = boxPtr ;
    if(    _XmGeoSetupKid( boxPtr, SB_OkButton( fsb))    )
    {   ++boxPtr ;
        } 

    for (i = 0; i < fsb->composite.num_children; i++)
    {
      Widget w = fsb->composite.children[i];
      if (IsButton(w) && !IsAutoButton(fsb,w) && w != SB_WorkArea(fsb))
      {
          if (_XmGeoSetupKid( boxPtr, w))
          {   ++boxPtr ;
              } 
          }
      }

    if(    _XmGeoSetupKid( boxPtr, SB_ApplyButton( fsb))    )
    {   ++boxPtr ;
        } 
    if(    _XmGeoSetupKid( boxPtr, SB_CancelButton( fsb))    )
    {   ++boxPtr ;
        } 
    if(    _XmGeoSetupKid( boxPtr, SB_HelpButton( fsb))    )
    {   ++boxPtr ;
        } 
    if(    boxPtr != firstButtonBox    )
    {   
        layoutPtr->fill_mode = XmGEO_CENTER ;
        layoutPtr->fit_mode = XmGEO_WRAP ;
        if(    !(SB_MinimizeButtons( fsb))    )
        {   layoutPtr->even_width = 1 ;
            } 
        layoutPtr->space_above = vspace ;
        vspace = BB_MarginHeight(fsb) ;
        layoutPtr->even_height = 1 ;
	++layoutPtr ;
        } 

    /* the end. */

    layoutPtr->space_above = vspace ;
    layoutPtr->end = TRUE ;
    return( geoSpec) ;
    }
/****************************************************************/

/*
 *    Use a resource to tell if we should hack the class record's
 *    GeoMatrixCreate method. The Motif FSB's is completely broken,
 *    our's just makes directory and list windows the same size.
 *    But our's does not work on SGI (at least) because they fixed
 *    the Motif bugs. djw
 */
static void
fe_file_selection_hack_class_record(Widget parent)
{
	if (xmFileSelectionBoxClassRec.bulletin_board_class.geo_matrix_create
		!=
		my_FileSBGeoMatrixCreate) {
		xmFileSelectionBoxClassRec.bulletin_board_class.geo_matrix_create
			= my_FileSBGeoMatrixCreate;
	}
}
#endif /* 0 */


typedef struct {
	Boolean hack;
	Boolean cde;
} fe_fsb_res_st;

static XtResource fe_fsb_resources[] =
{
  {
    "nsMotifFSBHacks", XtCBoolean, XtRBoolean, sizeof(Boolean),
    XtOffset(fe_fsb_res_st *, hack), XtRImmediate,
#ifdef IRIX	
    /* Irix has the nice enhanced FSB, so they don't need it */
    (XtPointer)False
#else
    (XtPointer)True
#endif
  },
  {
    "nsMotifFSBCdeMode", XtCBoolean, XtRBoolean, sizeof(Boolean),
    XtOffset(fe_fsb_res_st *, cde), XtRImmediate, (XtPointer)False
  }
};

typedef enum {
	FSB_LOSING,
	FSB_HACKS,
	FSB_CDE,
	FSB_CDE_PLUS
} fe_fsb_mode;

static fe_fsb_mode
fe_file_selection_box_get_resources(Widget parent)
{
  static Boolean done;
  static fe_fsb_res_st result;

  if (!done) {
		
    while (!XtIsTopLevelShell(parent))
      parent = XtParent(parent);

    XtGetApplicationResources(parent,
			      (XtPointer)&result,
			      fe_fsb_resources, XtNumber(fe_fsb_resources),
			      0, 0);
    done++;
  }

  if (result.hack && result.cde)
    return FSB_CDE_PLUS;
  else if (result.hack && !result.cde)
    return FSB_HACKS;
  else if (!result.hack && result.cde)
    return FSB_CDE;
  else
    return FSB_LOSING;
}
#endif /*USE_WINNING_FILE_SELECTION*/

static void
fe_file_selection_save_directory_cb(Widget widget, XtPointer a, XtPointer b)
{
    XmString  xm_directory;
    XmString  xm_dirspec;
    /* And save directory default for next time */
    XtVaGetValues(widget,
		  XmNdirectory, &xm_directory,
		  XmNdirSpec, &xm_dirspec,
		  0);

    if (xm_dirspec != 0) {
      char* directory = _XmStringGetTextConcat(xm_dirspec);

      if (directory != NULL) {
	if (directory[0] == '/') {
				
	  char* end = strrchr(directory, '/');
				
	  if (end != NULL) {
	    if (end != directory)
	      *end = '\0';
					
	    if (xm_directory != NULL)
	      XmStringFree(xm_directory);
					
	    xm_directory = XmStringCreateLocalized(directory);
	  }
	}
	XtFree(directory);
      }
    }

    if (xm_directory != 0) {
	if (fe_file_selection_directory)
	    XmStringFree(fe_file_selection_directory);
	fe_file_selection_directory = xm_directory;
    }
}

Widget
fe_CreateFileSelectionBox(Widget parent, char* name,
			  Arg* p_args, Cardinal p_n)
{
    Arg      args[32];
    Cardinal n;
    Cardinal i;
    Widget   fsb;
    Boolean  directory_set = FALSE;
#ifdef USE_WINNING_FILE_SELECTION
    Boolean  dir_search_set = FALSE;
    Boolean  file_search_set = FALSE;
    Boolean  qualify_set = FALSE;
    Widget   dir_list;
    Widget   file_list;
    Widget   dirspec;
    Widget   filter;
	Widget   filter_button;
	fe_fsb_mode  mode = fe_file_selection_box_get_resources(parent);
#endif /*USE_WINNING_FILE_SELECTION*/

	for (n = 0, i = 0; i < p_n && i < 30; i++) {	
		if (p_args[i].name == XmNdirectory)
			directory_set = TRUE;

#ifdef USE_WINNING_FILE_SELECTION
		if (mode == FSB_HACKS) {
			if (p_args[i].name == XmNdirSearchProc)
				dir_search_set = TRUE;
			else if (p_args[i].name == XmNfileSearchProc)
				file_search_set = TRUE;
			else if (p_args[i].name == XmNqualifySearchDataProc)
				qualify_set = TRUE;
			else
				args[n++] = p_args[i];
		}
		else
#endif /*USE_WINNING_FILE_SELECTION*/
		{
			args[n++] = p_args[i];
		}
	}

	/*
	 *    Add last visited directory
	 */
    if (!directory_set) {
		XtSetArg(args[n], XmNdirectory, fe_file_selection_directory); n++;
    }

#ifdef USE_WINNING_FILE_SELECTION
    if (mode == FSB_CDE || mode == FSB_CDE_PLUS) {
		XtSetArg(args[n], "pathMode", 1); n++;
    }

	if (mode == FSB_HACKS) {

		if (!dir_search_set) {
			XtSetArg(args[n], XmNdirSearchProc,
					 fe_file_selection_dir_search_proc); n++;
		}
		
		if (!file_search_set) {
			XtSetArg(args[n], XmNfileSearchProc,
					 fe_file_selection_file_search_proc); n++;
		}
		
		if (!qualify_set) {
			XtSetArg(args[n], XmNqualifySearchDataProc,
					 fe_file_selection_qualify_search_data_proc); n++;
		}
    
#ifdef FEATURE_MOTIF_FSB_CLASS_HACKING
		/*
		 *    This routine is now in the Xm patches tree, as it requires
		 *    access to the Motif source...djw
		 */
		fe_FileSelectionBoxHackClassRecord(parent);
#endif /*FEATURE_MOTIF_FSB_CLASS_HACKING*/

		fsb = XmCreateFileSelectionBox(parent, name, args, n);
    
		/*
		 *    XmCreateFileSelectionBox() sets directory list
		 *    XmNscrollBarDisplayPolicy to STATIC which always shows
		 *    the horizontal scrollbar. Bye...
		 */
		dir_list = XmFileSelectionBoxGetChild(fsb, XmDIALOG_DIR_LIST);
		XtVaSetValues(dir_list, XmNscrollBarDisplayPolicy, XmAS_NEEDED, 0);
		file_list = XmFileSelectionBoxGetChild(fsb, XmDIALOG_LIST);
		XtVaSetValues(file_list, XmNscrollBarDisplayPolicy, XmAS_NEEDED, 0);
		dirspec = XmFileSelectionBoxGetChild(fsb, XmDIALOG_TEXT);
		XtAddCallback(dirspec, XmNmodifyVerifyCallback,
					  fe_file_selection_dirspec_cb, (XtPointer)fsb);

	filter = XmFileSelectionBoxGetChild(fsb, XmDIALOG_FILTER_TEXT);
		XtAddCallback(filter, XmNmodifyVerifyCallback,
					  fe_file_selection_dirspec_cb, (XtPointer)fsb);

	}
	else
#endif /*USE_WINNING_FILE_SELECTION*/
	{
		fsb = XmCreateFileSelectionBox(parent, name, args, n);
	}

#ifdef IRIX
#ifndef IRIX5_3
	/* FIX 98019
	 *
	 * For IRIX 6.2 and later:
	 *      Reset filetype mask to suit custom Sgm file dialog.
	 *      (otherwise directories don't show up in the list.)
	 */
	XtVaSetValues(fsb,XmNfileTypeMask,XmFILE_ANY_TYPE,NULL);
#endif
#endif


#ifdef IRIX
#ifndef IRIX5_3
	/* FIX 98019
	 *
	 * For IRIX 6.2 and later:
	 *	Reset filetype mask to suit custom Sgm file dialog.
	 *	(otherwise directories don't show up in the list.)
	 */
	XtVaSetValues(fsb,XmNfileTypeMask,XmFILE_ANY_TYPE,NULL);
#endif
#endif

#ifdef USE_WINNING_FILE_SELECTION
	if (mode == FSB_HACKS || mode == FSB_CDE_PLUS) {
		filter_button = XmFileSelectionBoxGetChild(fsb, XmDIALOG_APPLY_BUTTON);
		XtAddCallback(filter_button, XmNactivateCallback,
					  fe_file_selection_filter_cb, fsb);
		
		fe_file_selection_register_actions(fsb);
	}
#endif /*USE_WINNING_FILE_SELECTION*/

    XtAddCallback(fsb, XmNokCallback,
				  fe_file_selection_save_directory_cb, NULL);

    return fsb;
}

Widget 
fe_CreateFileSelectionDialog(Widget   parent,
			     String   name,
			     Arg*     fsb_args,
			     Cardinal fsb_n)
{   
  Widget  fsb;       /*  new fsb widget      */
  Widget  ds;        /*  DialogShell         */
  ArgList ds_args;   /*  arglist for shell  */
  char    ds_name[256];

  /*
   *    Create DialogShell parent.
   */
  XP_STRCPY(ds_name, name);
  XP_STRCAT(ds_name, "_popup"); /* motif compatible */

  ds_args = (ArgList)XtMalloc(sizeof(Arg) * (fsb_n + 1));
  XP_MEMCPY(ds_args, fsb_args, (sizeof(Arg) * fsb_n));
  XtSetArg(ds_args[fsb_n], XmNallowShellResize, True); 
  ds = XmCreateDialogShell(parent, ds_name, ds_args, fsb_n + 1);

  XtFree((char*)ds_args);

  /*
   *    Create FileSelectionBox.
   */
  fsb = fe_CreateFileSelectionBox(ds, name, fsb_args, fsb_n);
  XtAddCallback(fsb, XmNdestroyCallback, _XmDestroyParentCallback, NULL);

  return(fsb) ;
}

static void fe_file_cb (Widget, XtPointer, XtPointer);
static void fe_file_destroy_cb (Widget, XtPointer, XtPointer);
static void fe_file_type_cb (Widget, XtPointer, XtPointer);

#ifdef NEW_DECODERS
static void fe_file_type_hack_extension (struct fe_file_type_data *ftd);
#endif /* NEW_DECODERS */

/*
 * fe_ReadFilename_2
 * context:
 *	If context is non-null, it uses context's data to display the fsb.
 * iparent, filebp, ftdp:
 *	If context is NULL, then this uses these to create the file
 *	selection box.
 *	'iparent' is the parent shell under which the FSB will be created.
 *	'filebp' is a pointer to the file selection box widget. [CANT BE NULL]
 *	'ftdp' is a pointer to file type data used by FSB code. [CANT BE NULL]
 *		We allocate one and return it back to the caller.
 *		Everytime we are called with the same filebp, we need to
 * 		be called with the same ftdp.
 *
 * WARNING:	1. Allocates *ftdp
 *		2. filebp and ftdp go together. For every filebp we need to
 *		   use the ftdp that was created with it.
 *
 * Just another form of fe_ReadFileName() so that other parts of fe
 * eg. bookmarks can use this as they dont have a context and I
 * think making a dummy context is bad.
 * - dp
 *
 * Akkana note: bug 82924 was fixed in FE_PromptForFileName()
 * (in src/context_funcs.cpp), but has not yet been fixed here.
 * The three filename prompting routines need to be integrated into one;
 * see comment under FE_PromptForFileName().
 */
char *
fe_ReadFileName_2 (	MWContext* context,
			Widget iparent, Widget* filebp,
			struct fe_file_type_data **ftdp,
			const char *title, const char *default_url,
			Boolean must_match, int *save_as_type)
{
  Widget fileb;
  struct fe_file_type_data *ftd;
  struct fe_confirm_data data;
  Widget sabox, samenu, sabutton;

  if (context) {
    fileb = CONTEXT_DATA (context)->file_dialog;
    ftd = CONTEXT_DATA (context)->ftd;
  }
  else {
    fileb = *filebp;
    ftd = *ftdp;
  }

  if (! title) title = XP_GetString(XFE_OPEN_FILE);

  if (! fileb)
    {
      Arg av [20];
      int ac, i;
      Widget shell;
      Widget parent;
      Visual *v = 0;
      Colormap cmap = 0;
      Cardinal depth = 0;

      if (context) parent = CONTEXT_WIDGET (context);
      else parent = iparent;

      XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		     XtNdepth, &depth, 0);

      ac = 0;
      XtSetArg (av[ac], XmNvisual, v); ac++;
      XtSetArg (av[ac], XmNdepth, depth); ac++;
      XtSetArg (av[ac], XmNcolormap, cmap); ac++;
      XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
/*      XtSetArg (av[ac], XmNallowShellResize, True); ac++;*/
      shell = XmCreateDialogShell (parent, "fileSelector_popup", av, ac);
      ac = 0;
      XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
      ac++;
      ftd = (struct fe_file_type_data *)
	calloc (sizeof (struct fe_file_type_data), 1);
      if (context) CONTEXT_DATA (context)->ftd = ftd;
      else *ftdp = ftd;
      XtSetArg (av[ac], XmNuserData, ftd); ac++;
      fileb = fe_CreateFileSelectionBox (shell, "fileSelector", av, ac);

#ifdef NO_HELP
      fe_UnmanageChild_safe (XtNameToWidget (fileb, "*Help"));
#endif

      ac = 0;
      XtSetArg (av[ac], XmNvisual, v); ac++;
      XtSetArg (av[ac], XmNdepth, depth); ac++;
      XtSetArg (av[ac], XmNcolormap, cmap); ac++;
      sabox = XmCreateFrame (fileb, "frame", av, ac);
      samenu = XmCreatePulldownMenu (sabox, "formatType", av, ac);
      ac = 0;

#ifdef NEW_DECODERS
      ftd->fileb = fileb;
#endif /* NEW_DECODERS */
      ftd->options [fe_FILE_TYPE_TEXT] =
	XmCreatePushButtonGadget (samenu, "text", av, ac);
      ftd->options [fe_FILE_TYPE_HTML] =
	XmCreatePushButtonGadget (samenu, "html", av, ac);
#ifdef SAVE_ALL
      ftd->options [fe_FILE_TYPE_HTML_AND_IMAGES] =
	XmCreatePushButtonGadget (samenu, "tree", av, ac);
#endif /* SAVE_ALL */
      ftd->options [fe_FILE_TYPE_PS] =
	XmCreatePushButtonGadget (samenu, "ps", av, ac);
/*       ftd->selected_option = fe_FILE_TYPE_TEXT; */
	  /* Make the default "save as" type html (ie, source) */
      ftd->selected_option = fe_FILE_TYPE_HTML;
      XtVaSetValues (samenu, XmNmenuHistory,
		     ftd->options [ftd->selected_option], 0);
      ac = 0;
      XtSetArg (av [ac], XmNsubMenuId, samenu); ac++;
      sabutton = fe_CreateOptionMenu (sabox, "formatType", av, ac);
      for (i = 0; i < countof (ftd->options); i++)
	if (ftd->options[i])
	  {
	    XtAddCallback (ftd->options[i], XmNactivateCallback,
			   fe_file_type_cb, ftd);
	    XtManageChild (ftd->options[i]);
	  }
      XtManageChild (sabutton);

      if (context) CONTEXT_DATA (context)->file_dialog = fileb;
      else *filebp = fileb;

      fe_HackDialogTranslations (fileb);
    }
  else
    {
      /* Without this the "*fileSelector.width:" resource doesn't work
	 on subsequent invocations of this box.  Don't ask me why.  I
	 tried permutations of allowShellResize as well with no effect.
	 (It's better to keep the widgets themselves around, if not the
	 windows, so that the default text is persistent.)
       */
#if 0
      XtUnrealizeWidget (fileb);
#endif
      sabox = XtNameToWidget (fileb, "*frame");

	  /*
	   *    Restore the values saved in fe_file_selection_save_directory_cb()
	   */
	  if (fe_file_selection_directory != 0) {
		  XtVaSetValues(fileb, XmNdirectory, fe_file_selection_directory, 0);
	  }
    }

#ifdef NEW_DECODERS
  ftd->orig_url = default_url;
  ftd->conversion_allowed_p = (save_as_type != 0);
#endif /* NEW_DECODERS */

#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (fileb, XmDIALOG_HELP_BUTTON));
#endif

  XtRemoveAllCallbacks (fileb, XmNnoMatchCallback);
  XtRemoveAllCallbacks (fileb, XmNokCallback);
  XtRemoveAllCallbacks (fileb, XmNapplyCallback);
  XtRemoveAllCallbacks (fileb, XmNcancelCallback);
  XtRemoveAllCallbacks (fileb, XmNdestroyCallback);

  XtAddCallback (fileb, XmNnoMatchCallback, fe_file_cb, &data);
  XtAddCallback (fileb, XmNokCallback,      fe_file_cb, &data);
  XtAddCallback (fileb, XmNcancelCallback,  fe_file_cb, &data);
  XtAddCallback (fileb, XmNdestroyCallback, fe_file_destroy_cb, &data);

  XtVaSetValues (XtParent (fileb), XmNtitle, title, 0);
#if 0 /* mustMatch doesn't work! */
  XtVaSetValues (fileb, XmNmustMatch, must_match, 0);
#else
  XtVaSetValues (fileb, XmNmustMatch, False, 0);
  data.must_match = must_match;
#endif

  {
    String dirmask = 0;
    XmString xms;
    char *s, *tail;
    char buf [2048];
    const char *def;

    if (! default_url)
      def = 0;
    else if ((def = strrchr (default_url, '/')))
      def++;
    else
      def = default_url;

    XtVaGetValues (fileb, XmNdirSpec, &xms, 0);
    XmStringGetLtoR (xms, XmFONTLIST_DEFAULT_TAG, &s); /* s - is XtMalloc  */
    XmStringFree (xms);

    /* Take the file name part off of `s', leaving only the dir. */
    tail = strrchr (s, '/');
    if (tail) *tail = 0;

    PR_snprintf (buf, sizeof (buf), "%.900s/%.900s", s, (def ? def : ""));
    if (def)
      {
	/* Grab the file name part (it's sitting right after the end of `s')
	   and map out the characters which ought not go into file names.
	   Also, terminate the file name at ? or #, since those aren't
	   really a part of the URL's file, but are modifiers to it.
	 */
	for (tail = buf+strlen(s)+1; *tail; tail++)
	  if (*tail == '?' || *tail == '#')
	    *tail = 0;
	  else if (*tail < '+' || *tail > 'z' ||
		   *tail == ':' || *tail == ';' ||
		   *tail == '<' || *tail == '>' ||
		   *tail == '\\' || *tail == '^' ||
		   *tail == '\'' || *tail == '`' ||
		   *tail == ',')
	    *tail = '_';
      }
    XtFree (s);

    /* If the dialog already existed, its data is out of date.  Resync. */
    XtVaGetValues (fileb, XmNdirMask, &dirmask, 0);
    XmFileSelectionDoSearch (fileb, dirmask);

    /* Change the default file name. */
    xms = XmStringCreate (buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues (fileb, XmNdirSpec, xms, 0);
    XmStringFree (xms);

#ifdef NEW_DECODERS
    fe_file_type_hack_extension (ftd);
#endif /* NEW_DECODERS */
  }

  data.context = context;
  data.answer = Answer_Invalid;
  data.return_value = 0;

  if (save_as_type)
    XtManageChild (sabox);
  else
    XtUnmanageChild (sabox);

  fe_NukeBackingStore (fileb);
  XtManageChild (fileb);
  /* #### check for unmanagement here */
  while (data.answer == Answer_Invalid)
    fe_EventLoop ();

  if (data.answer != Answer_Destroy) {
    /* Call the ok -> save directory callback because it's been removed */
    fe_file_selection_save_directory_cb(fileb, NULL, NULL);
    XtUnmanageChild(fileb);
    XtRemoveCallback(fileb, XmNdestroyCallback, fe_file_destroy_cb, &data);
  }


  if (save_as_type)
    *save_as_type = ftd->selected_option;

  if (data.answer == Answer_Destroy) {
    if (!data.context) {
      XP_ASSERT(filebp && ftdp);
      *filebp = 0;
      *ftdp = 0;
    }
    if (ftd) {
      /*
       * If the context got destroyed, this could have been freed by the
       * context. Our destroy routine checks for the parent context being
       * freed and sets the return_value to -1 if the context was destroyed.
       */
      if (data.return_value != (void *)-1)
	XP_FREE(ftd);
    }
    data.answer = Answer_Cancel; /* Simulate CANCEL */
    data.return_value = 0;
  }

  return (char *) data.return_value;
}

char *
fe_ReadFileName (MWContext *context, const char *title,
		 const char *default_url,
		 Boolean must_match,
		 int *save_as_type)
{
  char *ret = NULL;

  fe_ProtectContext(context);
  ret = fe_ReadFileName_2( context, NULL, NULL, NULL,
			   title, default_url, must_match, save_as_type);
  fe_UnProtectContext(context);
  if (fe_IsContextDestroyed(context)) {
    free(CONTEXT_DATA(context));
    free(context);
  }
  return ret;
}


static void 
fe_file_destroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  if (data->answer == Answer_Invalid) {
    data->answer = Answer_Destroy;
    /* Reset the context file dialog storage. Be sure that this destroy isnt
     * a result of the context itself being destroyed.
     */
    if (data->context) {
      if (!XfeIsAlive(CONTEXT_WIDGET(data->context))) {
	/* Indicates the context was destroyed too */
	data->return_value = (void *)-1;
      }
      else {
	CONTEXT_DATA(data->context)->file_dialog = 0;
	CONTEXT_DATA(data->context)->ftd = 0;
      }
    }
  }
}

static void 
fe_file_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  XmFileSelectionBoxCallbackStruct *sbc =
    (XmFileSelectionBoxCallbackStruct *) call_data;

  switch (sbc->reason)
    {
    case XmCR_NO_MATCH:
      {
    NOMATCH:
	XBell (XtDisplay (widget), 0);
	break;
      }
    case XmCR_OK:
      {
	XmStringGetLtoR (sbc->value, XmFONTLIST_DEFAULT_TAG,
			 (char **) &data->return_value);
#if 1
	/* mustMatch doesn't work!! */
	{
	  struct stat st;
	  if (data->must_match &&
	      data->return_value &&
	      stat (data->return_value, &st))
	    {
	      free (data->return_value);
	      data->return_value = 0;
	      goto NOMATCH;
	    }
	}
#endif
	data->answer = Answer_OK;
	break;
      }
    case XmCR_CANCEL:
      {
	data->answer = Answer_Cancel;
	data->return_value = 0;
	break;
      }
    default:
      abort ();
    }
}

static void
fe_file_type_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_file_type_data *ftd = (struct fe_file_type_data *) closure;
  int i;
  ftd->selected_option = -1;
  for (i = 0; i < countof (ftd->options); i++)
    if (ftd->options [i] == widget)
      {
	ftd->selected_option = i;
	break;
      }
#ifdef NEW_DECODERS
  fe_file_type_hack_extension (ftd);
#endif /* NEW_DECODERS */
}

#ifdef NEW_DECODERS
static void
fe_file_type_hack_extension_1 (struct fe_file_type_data *ftd,
			       Boolean dirspec_p)
{
  int desired_type;
  const char *default_file_name = "index.html";
  XmString xm_file = 0;
  char *file = 0;
  char *name_part = 0;
  char *ext = 0;
  char *orig_ext = 0;
  const char *new_ext = 0;
  char *orig_url_copy = 0;

  if (!ftd || !ftd->fileb)
    return;

  if (ftd->conversion_allowed_p)
    desired_type = ftd->selected_option;
  else
    desired_type = fe_FILE_TYPE_HTML;

  /* Find a default file name.
     If this URL ends in a file part, use that, otherwise, assume "index.html".
     Then, once we've got a default file name, set the default extension to
     use for "Save As Source" from that.
   */
  if (ftd->orig_url)
    {
      const char *orig_name_part;
      char *s;

      orig_url_copy = strdup (ftd->orig_url);

      /* take off form and anchor data. */
      if ((s = strchr (orig_url_copy, '?')))
	*s = 0;
      if ((s = strchr (orig_url_copy, '#')))
	*s = 0;

      orig_name_part = strrchr (orig_url_copy, '/');
      if (! orig_name_part)
	orig_name_part = strrchr (orig_url_copy, ':');
      if (orig_name_part)
	orig_name_part++;
      else if (orig_url_copy && *orig_url_copy)
	orig_name_part = orig_url_copy;

      if (orig_name_part && *orig_name_part)
	default_file_name = orig_name_part;

      orig_ext = strrchr (default_file_name, '.');

      if (!orig_ext && !dirspec_p)
	{
	  /* If we're up in the filter area, and there is as yet no
	     orig_ext, then that means that there was file name without
	     an extension (as opposed to no file name, when we would
	     have defaulted to "index.html".)  So, for the purposes
	     of the filter, set the extension to ".*".
	   */
	  orig_ext = ".*";
	}
    }

  /* Get `file' from what's currently typed into the text field.
   */
  XtVaGetValues (ftd->fileb,
		 (dirspec_p ? XmNdirSpec : XmNdirMask), &xm_file, 0);
  if (! xm_file) return;
  XmStringGetLtoR (xm_file, XmFONTLIST_DEFAULT_TAG, &file); /* file- XtMalloc */
  XmStringFree (xm_file);

  /* If the file ends in "/" then stick the default file name on the end. */
  if (dirspec_p && file && *file && file [strlen (file) - 1] == '/')
    {
      char *file2 = (char *) malloc (strlen (file) +
				     strlen (default_file_name) + 1);
      strcpy (file2, file);
      strcat (file2, default_file_name);
      XtFree (file);
      file = file2;
    }

  name_part = strrchr (file, '/');
  if (! name_part)
    name_part = strrchr (file, ':');
  if (name_part)
    name_part++;

  if (!name_part || !*name_part)
    name_part = 0;

  if (name_part)
    ext = strrchr (name_part, '.');

  switch (desired_type)
    {
    case fe_FILE_TYPE_HTML:
    case fe_FILE_TYPE_HTML_AND_IMAGES:
      new_ext = orig_ext;
      break;
    case fe_FILE_TYPE_TEXT:
    case fe_FILE_TYPE_FORMATTED_TEXT:
      /* The user has selected "text".  Change the extension to "txt"
	 only if the original URL had extension "html" or "htm". */
      if (orig_ext &&
	  (!strcasecomp (orig_ext, ".html") ||
	   !strcasecomp (orig_ext, ".htm")))
	new_ext = ".txt";
      else
	new_ext = orig_ext;
      break;
    case fe_FILE_TYPE_PS:
      new_ext = ".ps";
      break;
    default:
      break;
    }

  if (ext && new_ext /* && !!strcasecomp (ext, new_ext) */ )
    {
      char *file2;
      *ext = 0;
      file2 = (char *) malloc (strlen (file) + strlen (new_ext) + 1);
      strcpy (file2, file);
      strcat (file2, new_ext);
      xm_file = XmStringCreateLtoR (file2, XmFONTLIST_DEFAULT_TAG);

      if (dirspec_p)
	{
	  XtVaSetValues (ftd->fileb, XmNdirSpec, xm_file, 0);
	}
      else
	{
	  XmString saved_value = 0;

	  /* Don't let what's currently typed into the `Selection' field
	     to change as a result of running this search again -- that's
	     totally bogus. */
	  XtVaGetValues (ftd->fileb, XmNdirSpec, &saved_value, 0);

	  XtVaSetValues (ftd->fileb, XmNdirMask, xm_file, 0);
	  XmFileSelectionDoSearch (ftd->fileb, xm_file);
	  if (saved_value)
	    {
	      XtVaSetValues (ftd->fileb, XmNdirSpec, saved_value, 0);
	      XmStringFree (saved_value);
	    }
	}

      XmStringFree (xm_file);
      free (file2);
    }

  if (orig_url_copy)
    free (orig_url_copy);

  XtFree (file);
}


static void
fe_file_type_hack_extension (struct fe_file_type_data *ftd)
{
  fe_file_type_hack_extension_1 (ftd, False);
  fe_file_type_hack_extension_1 (ftd, True);
}

#endif /* NEW_DECODERS */



/* Open URL - prompts asynchronously
 */

static char *last_open_url_text = 0;

struct fe_open_url_data {
  MWContext *context;
  Widget widget;
  Widget text;
#ifdef EDITOR
  Widget in_editor;
#endif
  Widget in_browser;
};

#if 0
static char*
cleanup_selection(char* target, char* source, unsigned max_size)
{
  char* p;
  char* q;
  char* end;

  for (p = source; isspace(*p); p++) /* skip beginning whitespace */
    ;

  end = &p[max_size-1];
  q = target;

  while (p < end) {
    /*
     *    Stop if we detect an unprintable, or newline.
     */
    if (!isprint(*p) || *p == '\n' || *p == '\r')
      break;

    if (isspace(*p))
      *q++ = ' ', p++;
    else
      *q++ = *p++;
  }
  /* strip trailing whitespace */
  while (q > target && isspace(q[-1]))
    q--;
	 
  *q = '\0';

  return target;
}
#endif

#if 0
static void
file_dialog_get_url(MWContext* context, char* address, MWContextType ge_type)
{
	if (address != NULL) {
		if (ge_type == MWContextBrowser) {
			fe_BrowserGetURL(context, address);
		}
#ifdef EDITOR
		else if (ge_type == MWContextEditor) {
			fe_EditorEdit(context, NULL, NULL, address); /* try to re-use */
		}
#endif
	}
}
#endif /* 0 */

#if 0
static void
fe_open_url_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
	struct fe_open_url_data *data = (struct fe_open_url_data *) closure;
	char*  text = NULL;
	MWContext* context;
	MWContextType ge_type = MWContextAny;

	if (call_data == NULL ||    /* when it's a destroy callback */
		!XfeIsAlive(widget) || /* on it's way */
		data == NULL)                         /* data from hell */
		return;
	
	context = data->context;

	if (widget == data->in_browser
#ifdef EDITOR
        || widget == data->in_editor
#endif
        ) {

		text = fe_GetTextField(data->text);

		if (! text)
			return;
		
		cleanup_selection(text, text, strlen(text)+1);

		if (*text != '\0') {

			if (last_open_url_text)
				XtFree(last_open_url_text);

			last_open_url_text = text;
            
            if (widget == data->in_browser) 
              ge_type = MWContextBrowser;
#ifdef EDITOR
            else if (widget == data->in_editor)
              ge_type = MWContextEditor;
#endif
		}
    }

	XtUnmanageChild(data->widget);
	XtDestroyWidget(data->widget);
	free(data);

	/*
	 *    Call common routine to divert the get_url()
	 */
	file_dialog_get_url(context, text, ge_type);
}
#endif /* 0 */

/* 3.x version */
static void
fe_open_url_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  char *text = 0;
  URL_Struct *url = 0;
  if (! cb) return;
  switch (cb->reason)
    {
    case XmCR_OK:
      if (! data->text) abort ();
      XtVaGetValues (data->text, XmNvalue, &text, 0);
      if (! text) abort ();
      if (last_open_url_text) free (last_open_url_text);
      last_open_url_text = strdup (text);
      url = NET_CreateURLStruct (text, FALSE);
      break;
    }
  XtDestroyWidget (data->widget);
  if (url)
    fe_GetURL (data->context, url, FALSE);
  free (data);
}


static void
fe_open_url_browse_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	struct fe_open_url_data* data = (struct fe_open_url_data *)closure;

	fe_browse_file_of_text(data->context, data->text, False);
}

/* 3.x version */
void
fe_OpenURLCallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_confirm_data *data;
  MWContext *context = (MWContext *) closure;
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog, form, label, text, line;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  dialog = XmCreatePromptDialog (mainw, "openURLDialog", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  ac = 0;
  form = XmCreateForm (dialog, "form", av, ac);
  ac = 0;
  label = XmCreateLabelGadget (form, "openURLLabel", av, ac);

  if (! last_open_url_text)
    if (CONTEXT_DATA (context)->url_text)
      XtVaGetValues (CONTEXT_DATA (context)->url_text,
		     XmNvalue, &last_open_url_text, 0);

  ac = 0;
  XtSetArg (av [ac], XmNvalue, (last_open_url_text
				? last_open_url_text : "")); ac++;
  XtSetArg (av [ac], XmNeditable, True); ac++;
  XtSetArg (av [ac], XmNcursorPositionVisible, True); ac++;
  text = fe_CreateTextField (form, "openURLText", av, ac);
  ac = 0;
  line = XmCreateSeparator (form, "line", av, ac);

  XtVaSetValues (label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (text,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, label,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_WIDGET,
		 XmNbottomWidget, line,
		 0);
  XtVaSetValues (line,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
		 0);
  XtManageChild (label);
  XtManageChild (text);
  XtManageChild (line);
  XtManageChild (form);

  data = (struct fe_confirm_data *) calloc(sizeof (struct fe_confirm_data), 1);
  data->context = context;
  data->widget = dialog;
  data->text = text;

  XtAddCallback (dialog, XmNokCallback, fe_open_url_cb, data);
  XtAddCallback (dialog, XmNcancelCallback, fe_open_url_cb, data);
  XtAddCallback (dialog, XmNdestroyCallback, fe_open_url_cb, data);
  XtAddCallback (dialog, XmNapplyCallback, fe_clear_text_cb, text);

  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);
}



/* from 5.x editordialogs.c */
Widget
fe_CreatePromptDialog(MWContext *context, char* name,
		      Boolean ok, Boolean cancel, Boolean apply,
		      Boolean separator, Boolean modal)
{
  Widget mainw = CONTEXT_WIDGET(context);
  Widget dialog;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  if (modal) {
    XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  }
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;

  dialog = XmCreatePromptDialog (mainw, name, av, ac);

  if (!separator)
    fe_UnmanageChild_safe(XmSelectionBoxGetChild(dialog,
						 XmDIALOG_SEPARATOR));

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						 XmDIALOG_SELECTION_LABEL));
  if (!ok)
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
  if (!cancel)
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						   XmDIALOG_CANCEL_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  if (apply)
    XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));

  return dialog;
}


void
fe_OpenURLDialog(MWContext* context)
{
  struct fe_open_url_data *data;
  Widget dialog;
  Widget label;
  Widget text;
  Widget in_browser;
#ifdef EDITOR
  Widget in_editor;
#endif
  Widget form;
  Widget browse;
  Arg    args[20];
  int    n;
  char*  string;

  dialog = fe_CreatePromptDialog(context, "openURLDialog",
				 FALSE, TRUE, TRUE, TRUE, TRUE);

  n = 0;
  form = XmCreateForm(dialog, "form", args, n);
  XtManageChild(form);

  n = 0;
  XtSetArg(args [n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args [n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args [n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
  label = XmCreateLabelGadget(form, "label", args, n);
  XtManageChild(label);

  n = 0;
  XtSetArg(args [n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args [n], XmNtopWidget, label); n++;
  XtSetArg(args [n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args [n], XmNbottomAttachment, XmATTACH_FORM); n++;
  browse = XmCreatePushButtonGadget(form, "choose", args, n);
  XtManageChild(browse);

  string = last_open_url_text? last_open_url_text : "";

  n = 0;
  XtSetArg(args [n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args [n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args [n], XmNtopWidget, label); n++;
  XtSetArg(args [n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args [n], XmNrightWidget, browse); n++;
  XtSetArg(args [n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args [n], XmNeditable, True); n++;
  XtSetArg(args [n], XmNcursorPositionVisible, True); n++;
  text = fe_CreateTextField(form, "openURLText", args, n);
  fe_SetTextField(text, string);
  XtManageChild(text);

  n = 0;
#ifdef MOZ_MAIL_NEWS
  in_browser = XmCreatePushButtonGadget(dialog, "openInBrowser", args, n);
#else
  in_browser = XmCreatePushButtonGadget(dialog, "OK", args, n);
#endif
  XtManageChild(in_browser);

#ifdef EDITOR
  n = 0;
  XtSetArg(args[n], XmNsensitive, !fe_IsEditorDisabled()); n++;
  in_editor = XmCreatePushButtonGadget(dialog, "openInEditor", args, n);
  XtManageChild(in_editor);
#endif

  data = (struct fe_open_url_data *)calloc(sizeof(struct fe_open_url_data), 1);
  data->context = context;
  data->widget = dialog;
  data->text = text;
  data->in_browser = in_browser;
#ifdef EDITOR
  data->in_editor = in_editor;

  if (context->type == MWContextEditor)
    XtVaSetValues(dialog, XmNdefaultButton, in_editor, 0);
  else
#endif
    XtVaSetValues(dialog, XmNdefaultButton, in_browser, 0);

  XtAddCallback(browse, XmNactivateCallback,fe_open_url_browse_cb, data);
#ifdef EDITOR
  XtAddCallback(in_editor, XmNactivateCallback, fe_open_url_cb, data);
#endif
  XtAddCallback(in_browser, XmNactivateCallback, fe_open_url_cb, data);
  XtAddCallback(dialog, XmNcancelCallback, fe_open_url_cb, data);
  XtAddCallback(dialog, XmNdestroyCallback, fe_open_url_cb, data);
  XtAddCallback(dialog, XmNapplyCallback, fe_clear_text_cb, text);
	
  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);
}

#if 0
void
fe_OpenURLChooseFileDialog (MWContext *context)
{
  char *text;

  if (!context) return;

  text = fe_ReadFileName (context, 		
			  XP_GetString(XFE_OPEN_FILE),
			  last_open_url_text,
			  FALSE, /* must match */
			  0); /* save_as_type */

  file_dialog_get_url(context, text, context->type);
}
#endif




/* Prompt the user for their password.  The message string is displayed
 * to the user so they know what they are giving their password for.
 * The characters of the password are not echoed.
 */
char *
XFE_PromptPassword (MWContext *context, const char *message)
{
  return (char *) fe_dialog (CONTEXT_WIDGET (context),
			     "password", message, TRUE, "", TRUE, FALSE,
			     ((char **) 1));
}

#if 0
/*
 * Yet Another front end function that
 * noone told us we were supposed to implement.
 */
char *XP_PromptPassword(MWContext *context, const char *message)
{
    return XFE_PromptPassword(context, message);
}
#endif /* 0 */

/* Prompt for a  username and password
 *
 * message is a prompt message.
 *
 * if username and password are not NULL they should be used
 * as default values and NOT MODIFIED.  New values should be malloc'd
 * and put in their place.
 *
 * If the User hit's cancel FALSE should be returned, otherwise
 * TRUE should be returned.
 */
XP_Bool
XFE_PromptUsernameAndPassword (MWContext *context, 
			      const char *message,
			      char **username,
			      char **password)
{
  char *pw = "";
  char *un = (char *) fe_dialog (CONTEXT_WIDGET (context),
				 "password", message, TRUE,
				 (*username ? *username : ""), TRUE, FALSE,
				 &pw);
  if (pw)
    {
      *username = un;
      *password = XP_STRDUP(pw);
      return(TRUE);
    }
  else
    {
      *username = 0;
      *password = 0;
      return(FALSE);
    }
}


#if 0
/*
 * Yet Another front end function that
 * noone told us we were supposed to implement.
 */
PRBool XP_PromptUsernameAndPassword (MWContext *window_id,
                                      const char *message,
                                      char **username,
                                      char **password)
{
    return XFE_PromptUsernameAndPassword(window_id, message, username, password);
}
#endif /* 0 */


/* Prompting for visuals
 */

static void fe_visual_cb (Widget, XtPointer, XtPointer);

Visual *
fe_ReadVisual (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Display *dpy = XtDisplay (mainw);
  Screen *screen = XtScreen (mainw);
  Arg av [20];
  int ac;
  int i;
  Widget shell;
  XmString *items;
  int item_count;
  int default_item = 0;
  XVisualInfo vi_in, *vi_out;
  struct fe_confirm_data data;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  vi_in.screen = fe_ScreenNumber (screen);
  vi_out = XGetVisualInfo (dpy, VisualScreenMask, &vi_in, &item_count);
  if (! vi_out) item_count = 0;
  items = (XmString *) calloc (sizeof (XmString), item_count + 1);
  for (i = 0; i < item_count; i++)
    {
      char *vdesc = fe_VisualDescription (screen, vi_out [i].visual);
      items[i] = XmStringCreate (vdesc, XmFONTLIST_DEFAULT_TAG);
      free (vdesc);
      if (vi_out [i].visual == v)
	default_item = i;
    }

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
/*  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;*/
  XtSetArg (av[ac], XmNlistItems, items); ac++;
  XtSetArg (av[ac], XmNlistItemCount, item_count); ac++;
  shell = XmCreateSelectionDialog (mainw, "visual", av, ac);

  XtAddCallback (shell, XmNokCallback, fe_visual_cb, &data);
  XtAddCallback (shell, XmNcancelCallback, fe_visual_cb, &data);
  XtAddCallback (shell, XmNdestroyCallback, fe_destroy_finish_cb, &data);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_APPLY_BUTTON));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell,
						XmDIALOG_SELECTION_LABEL));

  {
    Widget list = XmSelectionBoxGetChild (shell, XmDIALOG_LIST);
    XtVaSetValues (list,
		   XmNselectedItems, (items + default_item),
		   XmNselectedItemCount, 1,
		   0);
  }

  data.context = context;
  data.widget = shell;
  data.answer = Answer_Invalid;
  data.return_value = 0;

  fe_NukeBackingStore (shell);
  XtManageChild (shell);

  /* #### check for destruction here */
  while (data.answer == Answer_Invalid)
    fe_EventLoop ();

  for (i = 0; i < item_count; i++)
    XmStringFree (items [i]);
  free (items);

  if (vi_out)
    {
      int index = (int) data.return_value;
      Visual *v = (data.answer == 1 ? vi_out [index].visual : 0);
      XFree ((char *) vi_out);
      return v;
    }
  else
    {
      return 0;
    }
}

static void fe_visual_cb (Widget widget, XtPointer closure,
			  XtPointer call_data)
{
  struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
  XmSelectionBoxCallbackStruct *cb =
    (XmSelectionBoxCallbackStruct *) call_data;
  XmString *items;
  int count;
  int i;
  XtVaGetValues (data->widget,
		 XmNlistItems, &items,
		 XmNlistItemCount, &count,
		 0);
  switch (cb->reason)
    {
    case XmCR_OK:
    case XmCR_APPLY:
      {
	data->answer = 1;
	data->return_value = (void *) -1;
	for (i = 0; i < count; i++)
	  if (XmStringCompare (items[i], cb->value))
	    data->return_value = (void *) i;
	if (data->return_value == (void *) -1)
	  abort ();
	break;
      }
    case XmCR_CANCEL:
      data->answer = 0;
      break;
    default:
      abort ();
      break;
    }
}


/* Displaying source
 */

struct fe_source_data
{
  Widget dialog;
  Widget name, url, text;
};

#if 0
static void
fe_close_source_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  if (! CONTEXT_DATA (context)->sd) return;
  XtUnmanageChild (CONTEXT_DATA (context)->sd->dialog);
}

static void
fe_source_save_as_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char *url = 0;
  URL_Struct *url_struct;
  if (! CONTEXT_DATA (context)->sd) return;
  url = fe_GetTextField (CONTEXT_DATA (context)->sd->url);
  if (! url) return;
  url_struct = NET_CreateURLStruct (url, FALSE);
  fe_SaveURL (context, url_struct);
}
#endif

#if 0	/* This is history. Should remove this code out later. */
Widget
fe_ViewSourceDialog (MWContext *context, const char *title, const char *url)
{
  Widget mainw = CONTEXT_WIDGET (context);
  struct fe_source_data *sd = CONTEXT_DATA (context)->sd;

  if (! sd)
    {
      Widget shell, form, text;
      Widget url_label, url_text, title_label, title_text;
      Widget ok_button, save_button;
      Arg av [20];
      int ac;
      Visual *v = 0;
      Colormap cmap = 0;
      Cardinal depth = 0;
      sd = (struct fe_source_data *) calloc(sizeof (struct fe_source_data), 1);

      XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		     XtNdepth, &depth, 0);
      ac = 0;
      XtSetArg (av[ac], XmNvisual, v); ac++;
      XtSetArg (av[ac], XmNdepth, depth); ac++;
      XtSetArg (av[ac], XmNcolormap, cmap); ac++;
      XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
      XtSetArg (av[ac], XmNtransientFor, mainw); ac++;

      XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;

      XtSetArg (av[ac], XmNdialogType, XmDIALOG_INFORMATION); ac++;
      XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
      shell = XmCreateTemplateDialog (mainw, "source", av, ac);

      fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_SEPARATOR));
      fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_OK_BUTTON));
      fe_UnmanageChild_safe (XmMessageBoxGetChild (shell,
						  XmDIALOG_CANCEL_BUTTON));
/*      fe_UnmanageChild_safe (XmMessageBoxGetChild (shell,
						  XmDIALOG_APPLY_BUTTON));*/
      fe_UnmanageChild_safe (XmMessageBoxGetChild (shell,
						  XmDIALOG_DEFAULT_BUTTON));
      fe_UnmanageChild_safe (XmMessageBoxGetChild (shell,
						  XmDIALOG_HELP_BUTTON));

      ac = 0;
      save_button = XmCreatePushButtonGadget (shell, "save", av, ac);
      ok_button = XmCreatePushButtonGadget (shell, "OK", av, ac);

      ac = 0;
      form = XmCreateForm (shell, "form", av, ac);

      ac = 0;
      title_label = XmCreateLabelGadget (form, "titleLabel", av, ac);
      url_label = XmCreateLabelGadget (form, "urlLabel", av, ac);
      ac = 0;
      XtSetArg (av [ac], XmNeditable, False); ac++;
      XtSetArg (av [ac], XmNcursorPositionVisible, False); ac++;
      title_text = fe_CreateTextField (form, "titleText", av, ac);
      url_text   = fe_CreateTextField (form, "urlText", av, ac);

      ac = 0;
      XtSetArg (av [ac], XmNeditable, False); ac++;
      XtSetArg (av [ac], XmNcursorPositionVisible, False); ac++;
      XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
      text = XmCreateScrolledText (form, "text", av, ac);
      fe_HackDialogTranslations (text);

      XtVaSetValues (title_label,
		     XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		     XmNtopWidget, title_text,
		     XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		     XmNbottomWidget, title_text,
		     XmNleftAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_WIDGET,
		     XmNrightWidget, title_text,
		     0);
      XtVaSetValues (url_label,
		     XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		     XmNtopWidget, url_text,
		     XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		     XmNbottomWidget, url_text,
		     XmNleftAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_WIDGET,
		     XmNrightWidget, url_text,
		     0);

      XtVaSetValues (title_text,
		     XmNtopAttachment, XmATTACH_FORM,
		     XmNbottomAttachment, XmATTACH_NONE,
		     XmNrightAttachment, XmATTACH_FORM,
		     0);
      XtVaSetValues (url_text,
		     XmNtopAttachment, XmATTACH_WIDGET,
		     XmNtopWidget, title_text,
		     XmNbottomAttachment, XmATTACH_NONE,
		     XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		     XmNleftWidget, title_text,
		     XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		     XmNrightWidget, title_text,
		     0);
      XtVaSetValues (XtParent (text),
		     XmNtopAttachment, XmATTACH_WIDGET,
		     XmNtopWidget, url_text,
		     XmNbottomAttachment, XmATTACH_FORM,
		     XmNleftAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_FORM,
		     0);

      fe_attach_field_to_labels (title_text, title_label, url_label, 0);

      XtAddCallback (save_button, XmNactivateCallback,
		     fe_source_save_as_cb, context);
      XtAddCallback (ok_button, XmNactivateCallback,
		     fe_close_source_cb, context);

      XtVaSetValues (shell, XmNdefaultButton, save_button, 0);

      XtManageChild (title_label);
      XtManageChild (title_text);
      XtManageChild (url_label);
      XtManageChild (url_text);
      XtManageChild (text);
      XtManageChild (form);
      XtManageChild (save_button);
      XtManageChild (ok_button);

      sd->dialog = shell;
      sd->text = text;
      sd->url = url_text;
      sd->name = title_text;
      CONTEXT_DATA (context)->sd = sd;
    }

  XtVaSetValues (sd->text, XmNcursorPosition, 0);
  fe_SetTextField (sd->text, "");
  XtVaSetValues (sd->name, XmNcursorPosition, 0, 0);
  fe_SetTextField (sd->name, (title ? title : ""));
  XtVaSetValues (sd->url, XmNcursorPosition, 0, 0);
  fe_SetTextField (sd->url, (url ? url : ""));

  fe_NukeBackingStore (sd->dialog);
  XtManageChild (sd->dialog);
  return sd->text;
}
#endif /* 0 */


/* User information
 */

#include <pwd.h>
#include <netdb.h>

void
fe_DefaultUserInfo (char **uid, char **name, Boolean really_default_p)
{
  struct passwd *pw = getpwuid (geteuid ());
  char *user_name, *tmp;
  char *real_uid = (pw ? pw->pw_name : "");
  char *user_uid = 0;

  if (really_default_p)
    {
      user_uid = real_uid;
    }
  else
    {
      user_uid = getenv ("LOGNAME");
      if (! user_uid) user_uid = getenv ("USER");
      if (! user_uid) user_uid = real_uid;

      /* If the env vars claim a different user, get the real name of
	 that user instead of the actual user. */
      if (strcmp (user_uid, real_uid))
	{
	  struct passwd *pw2 = getpwnam (user_uid);
	  if (pw2) pw = pw2;
	}
    }

  user_uid = strdup (user_uid);
  user_name = strdup ((pw && pw->pw_gecos ? pw->pw_gecos : "&"));

  /* Terminate the string at the first comma, to lose phone numbers and crap
     like that.  This may lose for "Jr."s, but who cares: Unix sucks. */
  if ((tmp = strchr (user_name, ',')))
    *tmp = 0;

  if ((tmp = strchr (user_name, '&')))
    {
      int i, j;
      char *new = (char *) malloc (strlen (user_name) + strlen (user_uid) + 1);
      for (i = 0, j = 0; user_name[i]; i++)
	if (tmp == user_name + i)
	  {
	    strcpy (new + j, user_uid);
	    new[j] = toupper (new[j]);
	    j += strlen (user_uid);
	    tmp = 0;
	  }
      else
	{
	  new[j++] = user_name[i];
	}
      free (user_name);
      user_name = new;
    }

  *uid = user_uid;
  *name = user_name;
}


/* Synchronous loading of a URL
   Internally, we use the URL mechanism to do some things during which time
   we want the interface to be blocked - for example, formatting a document
   for inclusion in a mail window, and printing, and so on.  This code lets
   us put up a dialog box with a status message and a cancel button, and
   bring it down when the event is over.
 */

/* #define SYNCHRONOUS_URL_DIALOG_WORKS */


#ifdef SYNCHRONOUS_URL_DIALOG_WORKS
static void
fe_synchronous_url_cancel_cb (Widget widget, XtPointer closure,
			      XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XP_InterruptContext (context);
}

/*
 * Make sure the the popped up modal dialog receives at least
 * on FocusIn event, because Motif seems to expect/require it.
 */
static void
fe_dialog_expose_eh (Widget widget, XtPointer closure, XEvent *event)
{
  MWContext *context = (MWContext *) closure;
  Widget dialog = CONTEXT_DATA (context)->synchronous_url_dialog;

  XtRemoveEventHandler(dialog, ExposureMask, FALSE,
        (XtEventHandler)fe_dialog_expose_eh, context);
  XSetInputFocus(XtDisplay (dialog), XtWindow(dialog),
        RevertToParent, CurrentTime);
  XSync (XtDisplay (dialog), False);
}
#endif /* SYNCHRONOUS_URL_DIALOG_WORKS */

void
fe_LowerSynchronousURLDialog (MWContext *context)
{
  if (CONTEXT_DATA (context)->synchronous_url_dialog)
    {
#ifdef SYNCHRONOUS_URL_DIALOG_WORKS
      Widget shell = XtParent (CONTEXT_DATA (context)->synchronous_url_dialog);

      /* Don't call XP_InterruptContext a (possibly) second time. */
      XtRemoveCallback (CONTEXT_DATA (context)->synchronous_url_dialog,
			XtNdestroyCallback,
			fe_synchronous_url_cancel_cb, context);
      XtUnmanageChild (CONTEXT_DATA (context)->synchronous_url_dialog);
      XSync (XtDisplay (shell), False);
      XtDestroyWidget (shell);
#endif /* SYNCHRONOUS_URL_DIALOG_WORKS */

      CONTEXT_DATA (context)->synchronous_url_dialog = 0;

      /* If this context was destroyed, do not proceed furthur */
      if (fe_IsContextDestroyed(context))
	return;

      assert (CONTEXT_DATA (context)->active_url_count > 0);
      if (CONTEXT_DATA (context)->active_url_count > 0)
	CONTEXT_DATA (context)->active_url_count--;
      if (CONTEXT_DATA (context)->active_url_count <= 0)
	XFE_AllConnectionsComplete (context);
      fe_SetCursor (context, False);
    }
}

static void
fe_synchronous_url_exit (URL_Struct *url, int status, MWContext *context)
{
  if (status == MK_CHANGING_CONTEXT)
    return;
  fe_LowerSynchronousURLDialog (context);
  if (status < 0 && url->error_msg)
    {
      FE_Alert (context, url->error_msg);
    }
  NET_FreeURLStruct (url);
  CONTEXT_DATA (context)->synchronous_url_exit_status = status;
}

void
fe_RaiseSynchronousURLDialog (MWContext *context, Widget parent,
			      const char *title)
{
#ifdef SYNCHRONOUS_URL_DIALOG_WORKS
  Widget shell, dialog;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  char title2 [255];
  Boolean popped_up;
#endif /* SYNCHRONOUS_URL_DIALOG_WORKS */

  CONTEXT_DATA (context)->active_url_count++;
  if (CONTEXT_DATA (context)->active_url_count == 1)
    {
      CONTEXT_DATA (context)->clicking_blocked = True;
      fe_StartProgressGraph (context);
      fe_SetCursor (context, False);
    }

#ifndef SYNCHRONOUS_URL_DIALOG_WORKS

  CONTEXT_DATA (context)->synchronous_url_dialog = (Widget) ~0;
  CONTEXT_DATA (context)->synchronous_url_exit_status = 0;

#else /* SYNCHRONOUS_URL_DIALOG_WORKS */

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  strcpy (title2, title);
  strcat (title2, "_popup");

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, parent); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  shell = XmCreateDialogShell (parent, title2, av, ac);
  ac = 0;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_WORKING); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  dialog = XmCreateMessageBox (shell, (char *) title, av, ac);

  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback (dialog, XmNokCallback, fe_destroy_cb, shell);
  XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cb, shell);
  XtAddCallback (dialog, XmNdestroyCallback, fe_synchronous_url_cancel_cb,
		 context);
  XtAddEventHandler(dialog, ExposureMask, FALSE,
		    (XtEventHandler)fe_dialog_expose_eh, context);

  CONTEXT_DATA (context)->synchronous_url_dialog = dialog;
  CONTEXT_DATA (context)->synchronous_url_exit_status = 0;

  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);
  XSync (XtDisplay (dialog), False);

  /*
   * We wait here until we KNOW the dialog is popped up,
   * otherwise Motif will misbehave.
   */
  popped_up = FALSE;
  while (!popped_up)
    {
      XEvent event;

      XtAppNextEvent(XtWidgetToApplicationContext(dialog), &event);
      if ((event.type == Expose)&&(event.xexpose.window == XtWindow(dialog)))
	{
	  popped_up = TRUE;
	}
      XtDispatchEvent(&event);
    }
#endif /* SYNCHRONOUS_URL_DIALOG_WORKS */

}


int
fe_await_synchronous_url (MWContext *context)
{
  /* Loop dispatching X events until the dialog box goes down as a result
     of the exit routine being called (which may be a result of the Cancel
     button being hit. */
  int status;
  
  fe_ProtectContext(context);
  while (CONTEXT_DATA (context)->synchronous_url_dialog)
    fe_EventLoop ();
  fe_UnProtectContext(context);
  status = CONTEXT_DATA (context)->synchronous_url_exit_status;
  if (fe_IsContextDestroyed(context)) {
    free(CONTEXT_DATA(context));
    free(context);
  }
  return (status);
}


int
fe_GetSynchronousURL (MWContext *context,
		      Widget widget_to_grab,
		      const char *title,
		      URL_Struct *url,
		      int output_format,
		      void *call_data)
{
  int status;
  url->fe_data = call_data;
  fe_RaiseSynchronousURLDialog (context, widget_to_grab, title);
  status = NET_GetURL (url, output_format, context, fe_synchronous_url_exit);
  if (status < 0)
    return status;
  else
    return fe_await_synchronous_url (context);
}


/* #if defined(MOZ_MAIL_NEWS) || defined(MOZ_MAIL_COMPOSE) */

/* Sending mail
 */

void fe_mail_text_modify_cb (Widget, XtPointer, XtPointer);

Boolean
fe_CheckDeferredMail (void)
{
/* #ifdef MOZ_MAIL_NEWS */
  struct fe_MWContext_cons* rest;
  Boolean haveQueuedMail = False;
  int numMsgs = 0;
  int row = 0;

  for (rest = fe_all_MWContexts; rest; rest = rest->next) {
    MWContext* context = rest->context;
/*    fe_MailNewsContextData* d = MAILNEWS_CONTEXT_DATA(context);*/
    if (context->type == MWContextMail) {
      MSG_CommandStatus (context, MSG_DeliverQueuedMessages,
			 &haveQueuedMail, NULL, NULL, NULL);
      if (haveQueuedMail) {
	MSG_FolderLine line;
	while (MSG_GetFolderLine (context, row, &line, NULL)) {
	  if (line.flags & MSG_FOLDER_FLAG_QUEUE) {
	    numMsgs = line.total;
	    break;
	  }
	  row++;
	}
	if (numMsgs) {
	  char buf [256];
	  void * sendNow = 0;
	  if (numMsgs == 1)
	    sendNow = fe_dialog (CONTEXT_WIDGET (fe_all_MWContexts->context),
				 "sendNow",
				 XP_GetString (XFE_OUTBOX_CONTAINS_MSG),
				 TRUE, 0, TRUE, FALSE, 0);
	  else {
	    PR_snprintf (buf, 256, XP_GetString (XFE_OUTBOX_CONTAINS_MSGS), numMsgs);
	    sendNow = fe_dialog (CONTEXT_WIDGET (fe_all_MWContexts->context),
				 "sendNow", buf, TRUE, 0, TRUE, FALSE, 0);
	  }
	  if (sendNow) {
	    MSG_Command (context, MSG_DeliverQueuedMessages);
	    return False;
	  }
	}
      }
    }
  }
/* #endif  MOZ_MAIL_NEWS */
  /* return True means no mail - weird, but consistent
   * with fe_CheckUnsentMail below. We can change them
   * later.
   */
  return True;
}

Boolean
fe_CheckUnsentMail (void)
{
  struct fe_MWContext_cons* rest;
  for (rest = fe_all_MWContexts; rest; rest = rest->next) {
    MWContext* context = rest->context;
    if (context->type == MWContextMessageComposition) {
      return XFE_Confirm(fe_all_MWContexts->context,
			 fe_globalData.unsent_mail_message);
    }
  }
  return TRUE;
}

static char* fe_last_attach_type = NULL;

static void
fe_del_attachment(struct fe_mail_attach_data *mad, int pos)
{
  if (pos > mad->nattachments) return;
  else XP_ASSERT(pos <= mad->nattachments);

  pos--;
  if (mad->attachments[pos].url)
    XP_FREE((char *)mad->attachments[pos].url);
  pos++;
  while (pos < mad->nattachments) {
    mad->attachments[pos-1] = mad->attachments[pos];
    pos++;
  }
  mad->nattachments--;
}

static void
fe_add_attachmentData(struct fe_mail_attach_data *mad,
			const struct MSG_AttachmentData *data)
{
  struct MSG_AttachmentData *m;
  char *name = (char *)data->url;
  XmString xmstr;

  if (!name || !*name) return;

  if (mad->nattachments >= XFE_MAX_ATTACHMENTS) return;
  else XP_ASSERT(mad->nattachments < XFE_MAX_ATTACHMENTS);

  xmstr = XmStringCreate(name, XmFONTLIST_DEFAULT_TAG);
  XmListAddItem(mad->list, xmstr, 0);

  m = &mad->attachments[mad->nattachments];
  *m = *data;
  m->url = XP_STRDUP(data->url);

  mad->nattachments++;
  if (mad->nattachments == 1)
    XmListSelectPos(mad->list, 1, True);

  XmListSelectItem(mad->list, xmstr, TRUE);
  XmStringFree(xmstr);
}


static void
fe_add_attachment(struct fe_mail_attach_data *mad, char *name)
{
  struct MSG_AttachmentData m = {0};
  Boolean b;
  XmString xmstr;

  if (!name || !*name) return;

  if(mad->nattachments >= XFE_MAX_ATTACHMENTS) return;
  else XP_ASSERT(mad->nattachments < XFE_MAX_ATTACHMENTS);

  xmstr = XmStringCreate(name, XmFONTLIST_DEFAULT_TAG);
  XmListAddItem(mad->list, xmstr, 0);

  XtVaGetValues (mad->text_p, XmNset, &b, 0);
  if (b)
    m.desired_type = TEXT_PLAIN;
  else {
    XtVaGetValues (mad->postscript_p, XmNset, &b, 0);
    if (b)
      m.desired_type = APPLICATION_POSTSCRIPT;
  }
  m.url = name;

  mad->attachments[mad->nattachments] = m;
  mad->nattachments++;

  XmListSelectItem(mad->list, xmstr, TRUE);
  XmStringFree(xmstr);
}

/***********************************
 * Location popup related routines *
 ***********************************/

static void
fe_locationOk_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  char *url = fe_GetTextField(mad->location_text);

  if (url && *url)
    fe_add_attachment(mad, url);
  else
    if (url) XtFree(url);
  XtUnmanageChild(mad->location_shell);
}
static void
fe_locationClear_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  fe_SetTextField (mad->location_text, "");
  /* Focus on the text widget after this, since otherwise you have to
     click again. */
  XmProcessTraversal (mad->location_text, XmTRAVERSE_CURRENT);
}

static void
fe_locationCancel_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  XtUnmanageChild(mad->location_shell);
}

static void
fe_attach_make_location(struct fe_mail_attach_data *mad)
{
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  Widget shell;
  Widget parent;
  Widget form;
  Widget label, location_label, location_text;
  Widget ok_button, clear_button, cancel_button;

  Widget kids [20];
  int i;

  if (mad->location_shell) return;

#ifdef dp_DEBUG
  fprintf(stderr, "Making attach_location widgets : fe_attach_make_location().\n");
#endif

  parent = CONTEXT_WIDGET(mad->context);

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
/*  XtSetArg (av[ac], XmNallowShellResize, True); ac++; */
/*  XtSetArg (av[ac], XmNtransientFor, parent); ac++; */
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  shell = XmCreateTemplateDialog (parent, "location_popup", av, ac);

  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_OK_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_CANCEL_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_DEFAULT_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_HELP_BUTTON));

  ac = 0;
  ok_button = XmCreatePushButtonGadget (shell, "OK", av, ac);
  clear_button = XmCreatePushButtonGadget (shell, "clear", av, ac);
  cancel_button = XmCreatePushButtonGadget (shell, "cancel", av, ac);

  ac = 0;
  form = XmCreateForm(shell, "form", av, ac);
  label = XmCreateLabelGadget( form, "label", av, ac);
  location_label = XmCreateLabelGadget( form, "locationLabel", av, ac);
  location_text = fe_CreateTextField( form, "locationText", av, ac);

  if (fe_globalData.nonterminal_text_translations)
    XtOverrideTranslations (location_text, fe_globalData.
				nonterminal_text_translations);

  XtVaSetValues (label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (location_label,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (location_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, location_label,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  _XfeHeight(location_label) = _XfeHeight(location_text);

  XtAddCallback(ok_button, XmNactivateCallback, fe_locationOk_cb, mad);
  XtAddCallback(clear_button, XmNactivateCallback, fe_locationClear_cb, mad);
  XtAddCallback(cancel_button, XmNactivateCallback, fe_locationCancel_cb, mad);

  {
    const char *str = MSG_GetAssociatedURL (mad->context);
    fe_SetTextFieldAndCallBack (location_text, (char *) (str ? str : ""));
  }

  fe_HackDialogTranslations (form);

  mad->location_shell = shell;
  i = 0;
  kids[i++] = mad->location_text = location_text;
  kids[i++] = location_label;
  kids[i++] = label;
  XtManageChildren(kids, i);
  i = 0;
  kids[i++] = ok_button;
  kids[i++] = clear_button;
  kids[i++] = cancel_button;
  XtManageChildren(kids, i);

  XtManageChild(form);
  XtManageChild(shell);
}

/*************************
 * File Attachment popup *
 *************************/
static void 
fe_attachFile_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data *) closure;
  XmFileSelectionBoxCallbackStruct *sbc =
    (XmFileSelectionBoxCallbackStruct *) call_data;
  char *file;
  char *msg;

  switch (sbc->reason) {
    case XmCR_NO_MATCH:
	XBell (XtDisplay (widget), 0);
	break;

    case XmCR_OK:
	XmStringGetLtoR (sbc->value, XmFONTLIST_DEFAULT_TAG, &file);
	if (!fe_isFileExist(file)) {
	    msg = PR_smprintf( XP_GetString( XFE_INVALID_FILE_ATTACHMENT_DOESNT_EXIST ), file);
	    if (msg) {
		fe_Alert_2(XtParent(mad->file_shell), msg);
		XP_FREE(msg);
	    }
	}
	else if (!fe_isFileReadable(file)) {
	    msg = PR_smprintf( XP_GetString( XFE_INVALID_FILE_ATTACHMENT_NOT_READABLE ) , file);
	    if (msg) {
		fe_Alert_2(XtParent(mad->file_shell), msg);
		XP_FREE(msg);
	    }
	}
	else if (fe_isDir(file)) {
	    msg = PR_smprintf( XP_GetString( XFE_INVALID_FILE_ATTACHMENT_IS_A_DIRECTORY ), file);
	    if (msg) {
		fe_Alert_2(XtParent(mad->file_shell), msg);
		if (msg) XP_FREE(msg);
	    }
	}
	else {
	  fe_add_attachment(mad, file);
	  XtUnmanageChild(mad->file_shell);
	}
	break;

    case XmCR_CANCEL:
	XtUnmanageChild(mad->file_shell);
	break;
    default:
      abort ();
  }
}

static void
fe_attach_make_file(struct fe_mail_attach_data *mad)
{
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget shell;
  Widget parent;
  Widget fileb;
  Boolean dirp = False;

  if (mad->file_shell) return;

#ifdef dp_DEBUG
  fprintf(stderr, "Making attach_file widgets : fe_attach_make_file().\n");
#endif

  parent = CONTEXT_WIDGET (mad->context);

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
/*  XtSetArg (av[ac], XmNallowShellResize, True); ac++;*/
  XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  shell = XmCreateDialogShell (parent, "fileBrowser_popup", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNfileTypeMask,
	    (dirp ? XmFILE_DIRECTORY : XmFILE_REGULAR)); ac++;
  fileb = fe_CreateFileSelectionBox (shell, "fileBrowser", av, ac);
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (fileb, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback(fileb, XmNnoMatchCallback, fe_attachFile_cb, mad);
  XtAddCallback(fileb, XmNokCallback,      fe_attachFile_cb, mad);
  XtAddCallback(fileb, XmNcancelCallback,  fe_attachFile_cb, mad);

  mad->file_shell = fileb;

  fe_HackDialogTranslations (fileb);

  fe_NukeBackingStore (fileb);
  XtManageChild (fileb);
}

static void
fe_attach_doc_type_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  int *poslist, npos;
  int attach_pos = -1;

  if (XmListGetSelectedPos(mad->list, &poslist, &npos)) {
    attach_pos = poslist[npos - 1] - 1;
    XP_FREE(poslist);
  }

  /*
   * my how intuitive, if the file is attach as source, desired type = NULL.
   */
  XtVaSetValues (widget, XmNset, True, 0);
  if (widget == mad->text_p) {
    XtVaSetValues (mad->source_p, XmNset, False, 0);
    XtVaSetValues (mad->postscript_p, XmNset, False, 0);
    if (attach_pos >= 0)
      mad->attachments[attach_pos].desired_type = TEXT_PLAIN;
  }
  else if (widget == mad->source_p) {
    XtVaSetValues (mad->text_p, XmNset, False, 0);
    XtVaSetValues (mad->postscript_p, XmNset, False, 0);
    if (attach_pos >= 0)
      mad->attachments[attach_pos].desired_type = NULL;
  }
  else if (widget == mad->postscript_p) {
    XtVaSetValues (mad->source_p, XmNset, False, 0);
    XtVaSetValues (mad->text_p, XmNset, False, 0);
    if (attach_pos >= 0)
      mad->attachments[attach_pos].desired_type = APPLICATION_POSTSCRIPT;
  }
  else
    abort ();
}

static void
fe_attachDestroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  int i;

#ifdef dp_DEBUG
  fprintf(stderr, "fe_attachDestroy_cb()\n");
  fprintf(stderr, "Destroying fe_mail_attach_data...\n");
#endif

  /* Free the list of attachments too */
  for(i=0; i<mad->nattachments; i++) {
    XP_FREE((char *)mad->attachments[i].url);
  }
  if (mad->location_shell)
    XtDestroyWidget(mad->location_shell);
  if (mad->file_shell)
    XtDestroyWidget(XtParent(mad->file_shell));
  CONTEXT_DATA(mad->context) ->mad = NULL;
  free (mad);
}

static void
fe_attachCancel_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;

  /* We dont need to delete all the attachments that we have as they will
     be deleted either the next time we show the attach window (or) when
     the message composition context gets destroyed */

  XtUnmanageChild(mad->shell);
  if (mad->location_shell)
    XtUnmanageChild(mad->location_shell);
  if (mad->file_shell)
    XtUnmanageChild(mad->file_shell);
}

static void
fe_attachOk_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  const char* ptr;

  mad->attachments[mad->nattachments].url = NULL;
  MSG_SetAttachmentList(mad->context, mad->attachments);
  ptr = MSG_GetAttachmentString(mad->context);
  fe_SetTextFieldAndCallBack(CONTEXT_DATA(mad->context)->mcAttachments,
		       ptr ? (char*) ptr : "");

  /* We dont need to delete all the attachments that we have as they will
     be deleted either the next time we show the attach window (or) when
     the message composition context gets destroyed */

  XtUnmanageChild(mad->shell);
  if (mad->location_shell)
    XtUnmanageChild(mad->location_shell);
  if (mad->file_shell)
    XtUnmanageChild(mad->file_shell);
}

static void
fe_attach_location_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;

  if (!mad->location_shell) fe_attach_make_location(mad);

  XtManageChild(mad->location_shell);
  XMapRaised(XtDisplay(mad->location_shell), XtWindow(mad->location_shell));
}

static void
fe_attach_file_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;

  if (!mad->file_shell) fe_attach_make_file(mad);

  XtManageChild(mad->file_shell);
  XMapRaised(XtDisplay(mad->file_shell), XtWindow(mad->file_shell));
}

static void
fe_attach_delete_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  int *poslist, npos;
  int i, pos;

  if (XmListGetSelectedPos(mad->list, &poslist, &npos)) {
    for(i=0; i<npos; i++) {
      XmListDeletePos(mad->list, poslist[i]);
      fe_del_attachment(mad, poslist[i]);
    }
    /*
     * After deleting an item from the list, select the
     * previous item in the list (or the last if it was
     * the first.)
     */
    pos = poslist[npos - 1] - 1;
    if (pos < 0)
      pos = 0;
    XmListSelectPos(mad->list, pos, TRUE);
    XP_FREE(poslist);
  }

  /*
   * If nothing left in the list selected, desensitize
   * the delete button.
   */
  if (!XmListGetSelectedPos(mad->list, &poslist, &npos)) {
    XtVaSetValues(mad->delete, XmNsensitive, False, 0);
  }
  else XP_FREE(poslist);
}

static void
fe_attach_select_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_mail_attach_data *mad = (struct fe_mail_attach_data  *) closure;
  XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;
  const char *s;
  Widget which_w = 0;

  if (cbs->item_position > mad->nattachments) return;
  else XP_ASSERT(cbs->item_position <= mad->nattachments);

  s = mad->attachments[cbs->item_position-1].desired_type;

  if (!s || !*s)
    which_w = mad->source_p;
  else if (!XP_STRCMP(s, APPLICATION_POSTSCRIPT))
    which_w = mad->postscript_p;
  else if (!XP_STRCMP(s, TEXT_PLAIN))
    which_w = mad->text_p;

  if (which_w == 0) return;
  else XP_ASSERT (which_w != 0);
  fe_attach_doc_type_cb(which_w, mad, NULL);

  XtVaSetValues(mad->delete, XmNsensitive, True, 0);
}

static void
fe_make_attach_dialog(MWContext* context)
{
  fe_ContextData* data = CONTEXT_DATA(context);
  Widget shell, form;
  Widget list;
  Widget messb;
  Widget   attach_location, attach_file, delete;
  Widget label;
  Widget   text_p, source_p, postscript_p;
  Widget ok_button, cancel_button;
  Widget kids [50];
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  struct fe_mail_attach_data *mad = data->mad;

  XP_ASSERT(context->type == MWContextMessageComposition);

  if (mad && mad->shell)
    return;

  XtVaGetValues (CONTEXT_WIDGET(context), XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, CONTEXT_WIDGET(context)); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  shell = XmCreateTemplateDialog (CONTEXT_WIDGET(context), "attach", av, ac);
/*  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_SEPARATOR)); */
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_OK_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_CANCEL_BUTTON));
/*  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_APPLY_BUTTON));*/
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_DEFAULT_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_HELP_BUTTON));

  ac = 0;
  ok_button = XmCreatePushButtonGadget (shell, "OK", av, ac);
  cancel_button = XmCreatePushButtonGadget (shell, "cancel", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form = XmCreateForm (shell, "form", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_WORK_AREA); ac++;
  XtSetArg (av[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
  messb = XmCreateMessageBox(form, "messagebox", av, ac);
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_OK_BUTTON));
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_CANCEL_BUTTON));
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_HELP_BUTTON));
  ac = 0;
  list = XmCreateList(messb, "list", av, ac);
  attach_location = XmCreatePushButtonGadget(messb, "attachLocation", av, ac);
  attach_file = XmCreatePushButtonGadget(messb, "attachFile", av, ac);
  ac = 0;
  XtSetArg (av[ac], XmNsensitive, False); ac++;
  delete = XmCreatePushButtonGadget(messb, "delete", av, ac);

  ac = 0;
  label = XmCreateLabelGadget (form, "label", av, ac);
  ac = 0;
  XtSetArg (av[ac], XmNset, False); ac++;
  source_p = XmCreateToggleButtonGadget (form, "sourceToggle", av, ac);
  text_p = XmCreateToggleButtonGadget (form, "textToggle", av, ac);
  postscript_p = XmCreateToggleButtonGadget (form, "postscriptToggle", av, ac);

  /* Making the attachments in such a way that the list would grow is
     the height of the dialog is increased */
  XtVaSetValues (messb,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (label,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (source_p,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, label,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, label,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (text_p,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, label,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, source_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (postscript_p,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, label,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, text_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

#ifdef dp_DEBUG
  fprintf(stderr, "Creating new fe_mail_attach_data...\n");
#endif
  data->mad = mad = (struct fe_mail_attach_data *)
    calloc (sizeof (struct fe_mail_attach_data), 1);
  mad->nattachments = 0;

  mad->context = context;
  mad->shell = shell;
  mad->list = list;
/*  mad->comppane = data->comppane;*/

  XtManageChild(list);
  ac = 0;
  kids [ac++] = attach_location;
  kids [ac++] = mad->attach_file = attach_file;
  kids [ac++] = mad->delete = delete;
  XtManageChildren (kids, ac);
  ac = 0;
  kids [ac++] = messb;
  kids [ac++] = label;
  kids [ac++] = mad->source_p = source_p;
  kids [ac++] = mad->text_p = text_p;
  kids [ac++] = mad->postscript_p = postscript_p;

  XtManageChildren (kids, ac);

  XtManageChild (form);
  XtManageChild (ok_button);
  XtManageChild (cancel_button);

  XtAddCallback (ok_button, XmNactivateCallback, fe_attachOk_cb, mad);
  XtAddCallback (cancel_button, XmNactivateCallback, fe_attachCancel_cb, mad);
  XtAddCallback (shell, XtNdestroyCallback, fe_attachDestroy_cb, mad);

  XtAddCallback (attach_location, XmNactivateCallback,
					fe_attach_location_cb, mad);
  XtAddCallback (attach_file, XmNactivateCallback, fe_attach_file_cb, mad);
  XtAddCallback (delete, XmNactivateCallback, fe_attach_delete_cb, mad);
  XtAddCallback (list, XmNbrowseSelectionCallback, fe_attach_select_cb, mad);

  XtAddCallback (text_p,   XmNvalueChangedCallback,
		 fe_attach_doc_type_cb, mad);
  XtAddCallback (source_p, XmNvalueChangedCallback,
		 fe_attach_doc_type_cb, mad);
  XtAddCallback (postscript_p, XmNvalueChangedCallback,
		 fe_attach_doc_type_cb, mad);

  /* Remember the last attachment typed used. */
  XtVaSetValues((fe_last_attach_type == NULL ? source_p :
		 strcmp(fe_last_attach_type, TEXT_PLAIN) == 0 ? text_p :
		 strcmp(fe_last_attach_type,
			APPLICATION_POSTSCRIPT) == 0 ? postscript_p :
		 source_p),
		XmNset, True, 0);

#if 0
  /* Decide whether to attach a file or a URL based on what the URL is.
     If there's something in the Attachement: field already, use that.
     Otherwise, use the document associated with this mail window.
   */
  {
    char *string = 0;
    const char *url = 0;
    const char *file;

    string = fe_GetTextField (data->mcAttachments);

    url = fe_StringTrim(string);
    if (url && !*url) {
      url = 0;
    }

    if (!url) {
      url = MSG_GetAssociatedURL(context);
    }

    if (! url)
      file = 0;
    else if (url[0] == '/')
      file = url;
    else if (!strncasecomp (url, "file://localhost/", 17))
      file = url + 16;
    else if (!strncasecomp (url, "file://", 7))
      file = 0;
    else if (!strncasecomp (url, "file:/", 6))
      file = url + 5;
    else
      file = 0;

    if (file)
      fe_SetTextField (file_text, file);

    XtVaSetValues (doc_p, XmNset, True, 0);
    fe_SetTextField (doc_text, url);
    if (string) free (string);
  }
#endif

  fe_NukeBackingStore (shell);
  XtManageChild (shell);
}


/* Prompts for attachment info.
 */
void
fe_mailto_attach_dialog(MWContext* context)
{
  struct fe_mail_attach_data *mad = CONTEXT_DATA(context)->mad;
  const struct MSG_AttachmentData *list;
  int i;

  XP_ASSERT(context->type == MWContextMessageComposition);

  if (!mad || !mad->shell) {
    fe_make_attach_dialog(context);
    mad = CONTEXT_DATA(context)->mad;
  }

  /* Free the existing list of attachments */
  XmListDeleteAllItems(mad->list);
  for(i=0; i<mad->nattachments; i++) {
    XP_FREE((char *)mad->attachments[i].url);
  }
  mad->nattachments = 0;
    
  /* Refresh the list of attachments */
  list = MSG_GetAttachmentList(context);
  while (list && list->url != NULL) {
    fe_add_attachmentData(mad, list);
    list++;
  }

  XtManageChild(mad->shell);
  XMapRaised(XtDisplay(mad->shell), XtWindow(mad->shell));
  return;
}


/* #### internal msglib thing... */
extern char **msg_GetSelectedMessageURLs (MWContext *context);

void
fe_attach_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		   fe_dnd_Source* source, XEvent* event)
{
  MWContext *compose_context;
  MWContext *src_context;
  const struct MSG_AttachmentData *old_list, *a;
  struct MSG_AttachmentData *new_list;
  Boolean sensitive_p = False;
  char **urls, **ss;
  const char* s;
  int old_count = 0;
  int new_count = 0;
  int i;

  if (type != FE_DND_DROP)
    return;

  compose_context = (MWContext *) closure;
  if (!compose_context) return;
  XP_ASSERT(compose_context->type == MWContextMessageComposition);
  if (compose_context->type != MWContextMessageComposition)
    return;

  XtVaGetValues(CONTEXT_DATA(compose_context)->mcAttachments,
		XmNsensitive, &sensitive_p, 0);
  if (!sensitive_p)
    {
      /* If the Attachments field is not sensitive, then that means that
	 an attachment (or delivery?) is in progress, and bad things would
	 happen were we to try and attach things right now.  So just beep.
       */
      XBell (XtDisplay (CONTEXT_WIDGET(compose_context)), 0);
      return;
    }

  src_context = fe_WidgetToMWContext((Widget) source->closure);
  if (!src_context) return;
  switch (src_context->type)
    {
    case MWContextMail:
    case MWContextNews:
      urls = msg_GetSelectedMessageURLs (src_context);
      break;
    case MWContextBookmarks:
      /* #### Get a list of all the selected URLs out of the bookmarks
	 window... */
      urls = 0;
      break;
    default:
      XP_ASSERT(0);
      urls = 0;
      break;
    }
  if (!urls)
    {
      XBell (XtDisplay (CONTEXT_WIDGET(compose_context)), 0);
      return;
    }

  new_count = 0;
  for (ss = urls; *ss; ss++)
    new_count++;
  XP_ASSERT(new_count > 0);
  if (new_count <= 0) return; /* #### leaks `urls'; but it already asserted. */

  old_list = MSG_GetAttachmentList(compose_context);
  old_count = 0;
  if (old_list)
    for (a = old_list; a->url; a++)
      old_count++;

  new_list = (struct MSG_AttachmentData *)
    XP_ALLOC(sizeof(struct MSG_AttachmentData) * (old_count + new_count + 1));

  for (i = 0; i < old_count; i++)
    {
      XP_MEMSET(&new_list[i], 0, sizeof(new_list[i]));
      if (old_list[i].url)
	new_list[i].url = XP_STRDUP(old_list[i].url);
      if (old_list[i].desired_type)
	new_list[i].desired_type = XP_STRDUP(old_list[i].desired_type);
      if (old_list[i].real_type)
	new_list[i].real_type = XP_STRDUP(old_list[i].real_type);
      if (old_list[i].real_encoding)
	new_list[i].real_encoding = XP_STRDUP(old_list[i].real_encoding);
      if (old_list[i].real_name)
	new_list[i].real_name = XP_STRDUP(old_list[i].real_name);
      if (old_list[i].description)
	new_list[i].description = XP_STRDUP(old_list[i].description);
      if (old_list[i].x_mac_type)
	new_list[i].x_mac_type = XP_STRDUP(old_list[i].x_mac_type);
      if (old_list[i].x_mac_creator)
	new_list[i].x_mac_creator = XP_STRDUP(old_list[i].x_mac_creator);
    }

  if (new_count > 0)
    XP_MEMSET(new_list + old_count, 0,
	      sizeof(struct MSG_AttachmentData) * (new_count + 1));

  i = old_count;
  for (ss = urls; *ss; ss++)
    new_list[i++].url = *ss;

  MSG_SetAttachmentList(compose_context, new_list);

  for (i = 0; i < old_count; i++)
    {
      if (new_list[i].url) XP_FREE((char*)new_list[i].url);
      if (new_list[i].desired_type) XP_FREE((char*)new_list[i].desired_type);
      if (new_list[i].real_type) XP_FREE((char*)new_list[i].real_type);
      if (new_list[i].real_encoding) XP_FREE((char*)new_list[i].real_encoding);
      if (new_list[i].real_name) XP_FREE((char*)new_list[i].real_name);
      if (new_list[i].description) XP_FREE((char*)new_list[i].description);
      if (new_list[i].x_mac_type) XP_FREE((char*)new_list[i].x_mac_type);
      if (new_list[i].x_mac_creator) XP_FREE((char*)new_list[i].x_mac_creator);
    }
  XP_FREE (new_list);
  for (ss = urls; *ss; ss++)
    XP_FREE(*ss);
  XP_FREE(urls);

  /* Now they're attached; update the display. */
  s = MSG_GetAttachmentString(compose_context);
  fe_SetTextFieldAndCallBack(CONTEXT_DATA(compose_context)->mcAttachments,
			     s ? (char*) s : "");
}


#if 1
MWContext *
FE_MakeMessageCompositionContext (MWContext *old_context)
{
  MWContext *new_context;

  new_context = fe_MakeWindow (XtParent(CONTEXT_WIDGET(old_context)), 0, 0, 0,
			MWContextMessageComposition, TRUE);

  /*
   * If the charset of the Motif text widget (the locale charset) can be
   * converted to our internal charset (win_csid), then inherit the default
   * and internal charsets from parent context. Otherwise, use the locale
   * charset. -- erik
   */
  if (INTL_DocToWinCharSetID(fe_LocaleCharSetID) == old_context->win_csid)
  {
    new_context->fe.data->xfe_doc_csid = old_context->fe.data->xfe_doc_csid;
    new_context->win_csid = old_context->win_csid;
  }
  else
  {
    new_context->fe.data->xfe_doc_csid = fe_LocaleCharSetID;
    new_context->win_csid = fe_LocaleCharSetID;
  }

  return new_context;
}

void
FE_InitializeMailCompositionContext (MWContext *context,
				     const char *from,
				     const char *reply_to,
				     const char *to,
				     const char *cc,
				     const char *bcc,
				     const char *fcc,
				     const char *newsgroups,
				     const char *followup_to,
				     const char *subject,
				     const char *attachment)
{
  fe_ContextData* data;
  XmString xmstr;
  Widget focusw;
  char *s;

  XP_ASSERT (context->type == MWContextMessageComposition);
  data = CONTEXT_DATA(context);

  data->mcCitedAndUnedited = False;
  data->mcEdited = False;

  /* #### data->mcUrl */
  /* #### data->mcNewsUrl */
  /* #### data->mcReferences */

  xmstr = XmStringCreate((/* FUCK */char *) from, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(data->mcFrom, XmNlabelString, xmstr, 0);
  XmStringFree(xmstr);

  /* #### warning this is cloned in mailnews.c (fe_set_compose_wrap_state) */
  XtAddCallback (data->mcBodyText, XmNmodifyVerifyCallback,
		 fe_mail_text_modify_cb, context);

  s = XP_STRDUP(subject);
  if (s)
    fe_forms_clean_text(context, context->win_csid, s, True);
  else
    /* No memory to do cleanup on subject. */
    s = (char *)subject;

  fe_SetTextFieldAndCallBack (data->mcReplyTo, reply_to);
  fe_SetTextFieldAndCallBack (data->mcTo, to);
  fe_SetTextFieldAndCallBack (data->mcCc, cc);
  fe_SetTextFieldAndCallBack (data->mcBcc, bcc);
  fe_SetTextFieldAndCallBack (data->mcFcc, fcc);
  fe_SetTextFieldAndCallBack (data->mcNewsgroups, newsgroups);
  fe_SetTextFieldAndCallBack (data->mcFollowupTo, followup_to);
  fe_SetTextFieldAndCallBack (data->mcSubject, s);
  fe_SetTextFieldAndCallBack (data->mcAttachments, attachment);

  XtVaSetValues (data->mcAttachments,
		 XmNsensitive, True,
		 XmNeditable, False,
		 XmNcursorPositionVisible, False,
		 0);

  /* All this focus stuff doesnt work too well yet. But this is better
     than not having this. */
  focusw = data->mcBodyText;
  if (newsgroups && *newsgroups) {
    if (!s || !*s) focusw = data->mcSubject;
  }
  else if (!to || !*to)
    focusw = data->mcTo;
  else if (!s || !*s)
    focusw = data->mcSubject;
    
  XtVaSetValues(data->main_pane, XmNinitialFocus, focusw, 0);
  XmProcessTraversal(focusw, XmTRAVERSE_CURRENT);

  if (data->show_toolbar_p && data->toolbar)	/* Sensitize the toolbars */
    fe_MsgSensitizeChildren(data->toolbar, (XtPointer)context, 0);

  /* ###  mtw->message_edited_p = False; */ /* after sig inserted */

  if (s && s != (char *)subject)
    XP_FREE(s);
}



void
FE_RaiseMailCompositionWindow (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  XtRealizeWidget (CONTEXT_WIDGET (context));
  XtManageChild (CONTEXT_WIDGET (context));
  XtPopup (CONTEXT_WIDGET (context), XtGrabNone);
}


void
FE_DestroyMailCompositionContext (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  XtDestroyWidget (CONTEXT_WIDGET (context));
}
#endif /* 1 */


int
FE_GetMessageBody (MWContext *context,
		   char **body,
		   uint32 *body_size,
		   MSG_FontCode **font_changes)
{
  fe_ContextData* data;
  Dimension columns = 0;
  char *loc;
  unsigned char *tmp;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  data = CONTEXT_DATA(context);

  *body = 0;
  loc = 0;
  tmp = 0;
  XtVaGetValues (data->mcBodyText, XmNvalue, &loc, XmNcolumns, &columns, 0);
  if (fe_LocaleCharSetID & MULTIBYTE) {
    columns *= 2;
  }
  if (loc) {
    tmp = fe_ConvertFromLocaleEncoding(INTL_DefaultWinCharSetID(context),
                                       (unsigned char *) loc);
  }
  if (tmp) {

#if 0  /* Old way: always wrap to window width */

    if (columns <= 0) columns = 79;
    *body = (char *) XP_WordWrap(INTL_DefaultWinCharSetID(context), tmp,
                                 columns, 1 /* look for '>' */);

#else /* New way: obey the ``Wrap Lines to 72'' toggle. */

    if (CONTEXT_DATA(context)->compose_wrap_lines_p)
      {
	columns = 72;
	*body = (char *) XP_WordWrap(INTL_DefaultWinCharSetID(context), tmp,
				     columns, 1 /* look for '>' */);
      }
    else
      {
	/* Else, don't wrap it at all. */
	*body = tmp;
	tmp = 0;
      }
#endif /* New way. */

    if (loc != (char *) tmp) {
      XP_FREE(tmp);
    }
  }
  *body_size = (*body ? strlen(*body) : 0);
  *font_changes = 0;
  return 0;
}

void
FE_DoneWithMessageBody(MWContext* context, char* body, uint32 body_size)
{
  if (body) {
    XP_FREE(body);
  }
}


void
fe_mail_text_modify_cb (Widget text, XtPointer client_data,
			XtPointer call_data)
{
  MWContext* context = (MWContext*) client_data;
  CONTEXT_DATA(context)->mcCitedAndUnedited = False;
  CONTEXT_DATA(context)->mcEdited = True;
  MSG_MessageBodyEdited(context);
}

/* #endif  MOZ_MAIL_NEWS || MOZ_MAIL_COMPOSE */

#define cite_abort 0
#define cite_protect_me_from_myself 1
#define cite_let_me_be_a_loser 2

#if 0
static int
FE_BogusQuotationQuery (MWContext *context, Boolean double_p)
{
  struct fe_confirm_data data;
  Widget dialog;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget parent = CONTEXT_WIDGET (context);
  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, parent); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  dialog = XmCreatePromptDialog (parent, (double_p ?
					  "citationQuery"
					  : "doubleCitationQuery")
				 , av, ac);

/*  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));*/
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtManageChild (dialog);

  XtAddCallback (dialog, XmNokCallback, fe_destroy_ok_cb, &data);
  XtAddCallback (dialog, XmNapplyCallback, fe_destroy_apply_cb, &data);
  XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cancel_cb, &data);
  XtAddCallback (dialog, XmNdestroyCallback, fe_destroy_finish_cb, &data);

  data.context = context;
  data.widget = dialog;
  data.answer = Answer_Invalid;

  while (data.answer == Answer_Invalid)
    fe_EventLoop ();

  return (data.answer == 0 ? cite_abort :
	  data.answer == 1 ? cite_protect_me_from_myself :
	  data.answer == 2 ? cite_let_me_be_a_loser : 99);
}
#endif


/* Print setup
 */

struct fe_print_data
{
  MWContext *context;
  History_entry *hist;
  Widget shell;
  Widget printer, file, command_text, file_text, browse;
  Widget first, last;
  Widget portrait, landscape;
  Widget grey, color;
  Widget letter, legal, exec, a4;
#ifdef DEBUG_jwz
  Widget small, med, large, huge;
  Widget header_toggle, footer_toggle;
  Widget header, footer;
  Widget hmargin, vmargin;
#endif /* DEBUG_jwz */
};

static void
fe_print_to_toggle_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fpd->printer)
    {
      XtVaSetValues (fpd->file, XmNset, False, 0);
      XtVaSetValues (fpd->browse, XmNsensitive, False, 0);
      XtVaSetValues (fpd->file_text, XmNsensitive, False, 0);
      XtVaSetValues (fpd->command_text, XmNsensitive, True, 0);
      XmProcessTraversal (fpd->command_text, XmTRAVERSE_CURRENT);
    }
  else if (widget == fpd->file)
    {
      XtVaSetValues (fpd->printer, XmNset, False, 0);
      XtVaSetValues (fpd->browse, XmNsensitive, True, 0);
      XtVaSetValues (fpd->file_text, XmNsensitive, True, 0);
      XtVaSetValues (fpd->command_text, XmNsensitive, False, 0);
      XmProcessTraversal (fpd->file_text, XmTRAVERSE_CURRENT);
    }
  else
    abort ();
}

static void
fe_print_order_toggle_cb (Widget widget, XtPointer closure,XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fpd->first)
    XtVaSetValues (fpd->last, XmNset, False, 0);
  else if (widget == fpd->last)
    XtVaSetValues (fpd->first, XmNset, False, 0);
  else
    abort ();
}

static void
fe_print_orientation_toggle_cb (Widget widget, XtPointer closure,
				XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fpd->portrait)
    XtVaSetValues (fpd->landscape, XmNset, False, 0);
  else if (widget == fpd->landscape)
    XtVaSetValues (fpd->portrait, XmNset, False, 0);
  else
    abort ();
}

static void
fe_print_color_toggle_cb (Widget widget, XtPointer closure,
			  XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fpd->grey)
    XtVaSetValues (fpd->color, XmNset, False, 0);
  else if (widget == fpd->color)
    XtVaSetValues (fpd->grey, XmNset, False, 0);
  else
    abort ();
}

static void
fe_print_paper_toggle_cb (Widget widget, XtPointer closure,
			  XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fpd->letter)
    {
      XtVaSetValues (fpd->legal,  XmNset, False, 0);
      XtVaSetValues (fpd->exec,   XmNset, False, 0);
      XtVaSetValues (fpd->a4,     XmNset, False, 0);
    }
  else if (widget == fpd->legal)
    {
      XtVaSetValues (fpd->letter, XmNset, False, 0);
      XtVaSetValues (fpd->exec,   XmNset, False, 0);
      XtVaSetValues (fpd->a4,     XmNset, False, 0);
    }
  else if (widget == fpd->exec)
    {
      XtVaSetValues (fpd->letter, XmNset, False, 0);
      XtVaSetValues (fpd->legal,  XmNset, False, 0);
      XtVaSetValues (fpd->a4,     XmNset, False, 0);
    }
  else if (widget == fpd->a4)
    {
      XtVaSetValues (fpd->letter, XmNset, False, 0);
      XtVaSetValues (fpd->legal,  XmNset, False, 0);
      XtVaSetValues (fpd->exec,   XmNset, False, 0);
    }
  else
    abort ();
}

#ifdef DEBUG_jwz
static void
fe_print_size_toggle_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fpd->small)
    {
      XtVaSetValues (fpd->med,   XmNset, False, 0);
      XtVaSetValues (fpd->large, XmNset, False, 0);
      XtVaSetValues (fpd->huge,  XmNset, False, 0);
    }
  else if (widget == fpd->med)
    {
      XtVaSetValues (fpd->small, XmNset, False, 0);
      XtVaSetValues (fpd->large, XmNset, False, 0);
      XtVaSetValues (fpd->huge,  XmNset, False, 0);
    }
  else if (widget == fpd->large)
    {
      XtVaSetValues (fpd->small, XmNset, False, 0);
      XtVaSetValues (fpd->med,   XmNset, False, 0);
      XtVaSetValues (fpd->huge,  XmNset, False, 0);
    }
  else if (widget == fpd->huge)
    {
      XtVaSetValues (fpd->small, XmNset, False, 0);
      XtVaSetValues (fpd->med,   XmNset, False, 0);
      XtVaSetValues (fpd->large, XmNset, False, 0);
    }
  else
    abort ();
}
#endif /* DEBUG_jwz */


void
fe_browse_file_of_text (MWContext *context, Widget text_field, Boolean dirp)
{
  char *text = 0;
  XmString xmpat, xmfile;
  char buf [1024];
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget shell;
  Widget parent;
  Widget fileb;
  struct fe_confirm_data data;
  data.context = context;

  /* Find the top level window of which this text field is a descendant,
     and make the file requester be a transient for that. */
  parent = text_field;
  while (parent && !XtIsShell (parent))
    parent = XtParent (parent);
  assert (parent);
  if (! parent)
    parent = CONTEXT_WIDGET (context);

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  text = fe_GetTextField(text_field);
  text = fe_StringTrim (text);


  if ( text && *text )
  	text = XP_STRTOK(text, " ");
 
  if (!text || !*text)
    {
      xmpat = 0;
      xmfile = 0;
    }
  else if (dirp)
    {
      if (text [strlen (text) - 1] == '/')
	text [strlen (text) - 1] = 0;
      PR_snprintf (buf, sizeof (buf), "%.900s/*", text);
      xmpat = XmStringCreateLtoR (buf, XmFONTLIST_DEFAULT_TAG);
      xmfile = XmStringCreateLtoR (text, XmFONTLIST_DEFAULT_TAG);
    }
  else
    {
      char *f;
      if (text [strlen (text) - 1] == '/')
	PR_snprintf (buf, sizeof (buf), "%.900s/*", text);
      else
	PR_snprintf (buf, sizeof (buf), "%.900s", text);
      xmfile = XmStringCreateLtoR (buf, XmFONTLIST_DEFAULT_TAG);
      if (text[0] == '/') /* only do this for absolute path */
	  f = strrchr (text, '/');
      else
	  f = NULL;
      if (f && f != text)
	*f = 0;
      if (f) {
        PR_snprintf (buf, sizeof (buf), "%.900s/*", text);
        xmpat = XmStringCreateLtoR (buf, XmFONTLIST_DEFAULT_TAG);
      }
      else {
	/* do not change dirmask and pattern if input is a file;
	 * otherwise, the text widget in the file selection box
	 * will insert the file name at the wrong position.
	 * Windows version has similar behavior if a relative file
	 * is entered.
	 */
	buf[0] = '\0';		/* input was just a file. no '/' in it. */
	xmpat = 0;
      }
    }
  if (text) free (text);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
/*  XtSetArg (av[ac], XmNallowShellResize, True); ac++;*/
  shell = XmCreateDialogShell (parent, "fileBrowser_popup", av, ac);
  ac = 0;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNfileTypeMask,
	    (dirp ? XmFILE_DIRECTORY : XmFILE_REGULAR)); ac++;
  fileb = fe_CreateFileSelectionBox (shell, "fileBrowser", av, ac);

#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (fileb, XmDIALOG_HELP_BUTTON));
#endif

  if (xmpat)
    {
      XtVaSetValues (fileb,
		     XmNdirMask, xmpat,
		     XmNpattern, xmpat, 0);
#if 0
      /*
       *    The XtVaSetValues on dirMask/pattern will cause this anyway.
       */
      XmFileSelectionDoSearch (fileb, xmpat);
#endif
      XtVaSetValues (fileb, XmNdirSpec, xmfile, 0);
      XmStringFree (xmpat);
      XmStringFree (xmfile);
    }

  XtAddCallback (fileb, XmNnoMatchCallback, fe_file_cb, &data);
  XtAddCallback (fileb, XmNokCallback,      fe_file_cb, &data);
  XtAddCallback (fileb, XmNcancelCallback,  fe_file_cb, &data);
  XtAddCallback (fileb, XmNdestroyCallback, fe_destroy_finish_cb, &data);

  data.answer = Answer_Invalid;
  data.return_value = 0;

  fe_HackDialogTranslations (fileb);

  fe_NukeBackingStore (fileb);
  XtManageChild (fileb);
  /* #### check for destruction here */
  while (data.answer == Answer_Invalid)
    fe_EventLoop ();

  if (data.answer == Answer_OK)
    if (data.return_value)
      {
	fe_SetTextField(text_field, data.return_value);
	free (data.return_value);
      }
  if (data.answer != Answer_Destroy)
    XtDestroyWidget(shell);
}

void
fe_browse_file_of_text_in_url (MWContext *context, Widget text_field, Boolean dirp)
{
  char *orig_text = 0;
  char *text = 0;
  XmString xmpat, xmfile;
  char buf [1024];
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget shell;
  Widget parent;
  Widget fileb;
  struct fe_confirm_data data;
  data.context = context;

  /* Find the top level window of which this text field is a descendant,
     and make the file requester be a transient for that. */

  parent = text_field;
  while (parent && !XtIsShell (parent))
    parent = XtParent (parent);
  assert (parent);
  if (! parent)
    parent = CONTEXT_WIDGET (context);

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  text = fe_GetTextField(text_field);
  orig_text = text;
  text = fe_StringTrim (text);

  if (!strncasecomp (text, "http://", 7)) {
    /* ignore url using http */
    free(orig_text);
    orig_text = 0;
    text = 0;
  }

  if (text) {
    if (!strncasecomp (text, "file://", 7)) {
      /* get to the absolute file path */
      text += 7;
    }
  }

  if ( text && *text )
    text = XP_STRTOK(text, " ");
 
  if (!text || !*text){
    xmpat = 0;
    xmfile = 0;
  }
  else if (dirp){
    if (text [strlen (text) - 1] == '/')
      text [strlen (text) - 1] = 0;
    PR_snprintf (buf, sizeof (buf), "%.900s/*", text);
    xmpat = XmStringCreateLtoR (buf, XmFONTLIST_DEFAULT_TAG);
    xmfile = XmStringCreateLtoR (text, XmFONTLIST_DEFAULT_TAG);
  }
  else {
    char *f;
    if (text [strlen (text) - 1] == '/')
      PR_snprintf (buf, sizeof (buf), "%.900s/*", text);
    else
      PR_snprintf (buf, sizeof (buf), "%.900s", text);
    xmfile = XmStringCreateLtoR (buf, XmFONTLIST_DEFAULT_TAG);
    if (text[0] == '/') /* only do this for absolute path */
      f = strrchr (text, '/');
    else
      f = NULL;
    if (f && f != text)
      *f = 0;
    if (f) {
      PR_snprintf (buf, sizeof (buf), "%.900s/*", text);
      xmpat = XmStringCreateLtoR (buf, XmFONTLIST_DEFAULT_TAG);
    }
    else {
      /* Do not change dirmask and pattern if input is a file;
       * otherwise, the text widget in the file selection box
       * will insert the file name at the wrong position.
       * Windows version has similar behavior if a relative file
       * is entered.
       */
      buf[0] = '\0';		/* input was just a file. no '/' in it. */
      xmpat = 0;
    }
  }
  if (orig_text) free (orig_text);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  /*  XtSetArg (av[ac], XmNallowShellResize, True); ac++;*/
  shell = XmCreateDialogShell (parent, "fileBrowser_popup", av, ac);
  ac = 0;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNfileTypeMask, (dirp ? XmFILE_DIRECTORY : XmFILE_REGULAR)); ac++;
  fileb = fe_CreateFileSelectionBox (shell, "fileBrowser", av, ac);

#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (fileb, XmDIALOG_HELP_BUTTON));
#endif

  if (xmpat) {
    XtVaSetValues (fileb,
		   XmNdirMask, xmpat,
		   XmNpattern, xmpat, 0);
#if 0
    /*
     *    The XtVaSetValues on dirMask/pattern will cause this anyway.
     */
    XmFileSelectionDoSearch (fileb, xmpat);
#endif
    XtVaSetValues (fileb, XmNdirSpec, xmfile, 0);
    XmStringFree (xmpat);
    XmStringFree (xmfile);
  }

  XtAddCallback (fileb, XmNnoMatchCallback, fe_file_cb, &data);
  XtAddCallback (fileb, XmNokCallback,      fe_file_cb, &data);
  XtAddCallback (fileb, XmNcancelCallback,  fe_file_cb, &data);
  XtAddCallback (fileb, XmNdestroyCallback, fe_destroy_finish_cb, &data);

  data.answer = Answer_Invalid;
  data.return_value = 0;

  fe_HackDialogTranslations (fileb);

  fe_NukeBackingStore (fileb);
  XtManageChild (fileb);
  /* #### check for destruction here */
  while (data.answer == Answer_Invalid)
    fe_EventLoop ();

  if (data.answer == Answer_OK)
    if (data.return_value) {
      /* prepend the answer with file url */
      char path[1025];
      sprintf(path, "file://%s", (char *) data.return_value);
      fe_SetTextField (text_field, path);
      free (data.return_value);
    }
  if (data.answer != Answer_Destroy)
    XtDestroyWidget(shell);
}



static void
fe_print_browse_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  fe_browse_file_of_text (fpd->context, fpd->file_text, False);
}


#define XFE_DEFAULT_PRINT_FILENAME	"netscape.ps"
static char *last_print_file_name = 0;
static Boolean last_to_file_p = False;

static void
ps_pipe_close (PrintSetup *p)
{
  fe_synchronous_url_exit (p->url, 0, (MWContext *) p->carg);
  pclose (p->out);
}

static void
ps_file_close (PrintSetup *p)
{
  fe_synchronous_url_exit (p->url, 0, (MWContext *) p->carg);
  fclose (p->out);
}

void
XFE_InitializePrintSetup (PrintSetup *p)
{
  XL_InitializePrintSetup (p);
  p->reverse = fe_globalPrefs.print_reversed;
  p->color = fe_globalPrefs.print_color;
  p->landscape = fe_globalPrefs.print_landscape;
/*  #### p->n_up = fe_globalPrefs.print_n_up;*/

  /* @@@ need to fix this -- erik */
  p->bigger = 0; /* -1 = small, 0 = medium, 1 = large, 2 = huge */

  if (fe_globalPrefs.print_paper_size == 0)
    {
      p->width = 8.5 * 72;
      p->height = 11  * 72;
    }
  else if (fe_globalPrefs.print_paper_size == 1)
    {
      p->width = 8.5 * 72;
      p->height = 14  * 72;
    }
  else if (fe_globalPrefs.print_paper_size == 2)
    {
      p->width = 7.5 * 72;
      p->height = 10  * 72;
    }
  else if (fe_globalPrefs.print_paper_size == 3)
    {
      p->width = 210 * 0.039 * 72;
      p->height = 297 * 0.039 * 72;
    }
#if 0
  p->paper_size = fe_globalPrefs.print_paper_size;

  /* initialize things related to other font to be NULL */
  for (i=0; i<N_FONTS; i++) {
      p->otherFontName[i] = NULL;
      p->otherFontInfo[i] = NULL;
  }
#endif /* 0 */

#ifdef DEBUG_jwz
  p->bigger = fe_globalPrefs.print_font_size;
  p->header = (fe_globalPrefs.print_header_p &&
               fe_globalPrefs.print_header && *fe_globalPrefs.print_header ?
               strdup (fe_globalPrefs.print_header) : 0);
  p->footer = (fe_globalPrefs.print_footer_p &&
               fe_globalPrefs.print_footer && *fe_globalPrefs.print_footer ?
               strdup (fe_globalPrefs.print_footer) : 0);
  p->right = p->left = fe_globalPrefs.print_hmargin;
  p->bottom = p->top = fe_globalPrefs.print_vmargin;
#endif /* DEBUG_jwz */

}


void
fe_Print(MWContext *context, URL_Struct *url, Boolean last_to_file_p,
		char *last_print_file_name)
{
/*  SHIST_SavedData saved_data;*/
  PrintSetup p;
  char *type = NULL;
  char name[1024];
  char clas[1024];
  char mimecharset[48];
  XrmValue value;
  XrmDatabase db = XtDatabase(XtDisplay(CONTEXT_WIDGET(context)));
/*  INTL_CharSetInfo c = LO_GetDocumentCharacterSetInfo(context);*/

  XFE_InitializePrintSetup (&p);

  if (last_to_file_p)
    {
      if (! *last_print_file_name)
	{
	  FE_Alert (context, fe_globalData.no_file_message);
	  return;
	}

      /* If the file exists, confirm overwriting it. */
      {
	XP_StatStruct st;
	char buf [2048];
	if (!stat (last_print_file_name, &st))
	  {
	    PR_snprintf (buf, sizeof (buf),
			fe_globalData.overwrite_file_message,
			last_print_file_name);
	    if (!FE_Confirm (context, buf))
	      return;
	  }
      }

      p.out = fopen (last_print_file_name, "w");
      if (! p.out)
	{
	  char buf [2048];
	  PR_snprintf (buf, sizeof (buf),
			XP_GetString(XFE_ERROR_OPENING_FILE),
	   		last_print_file_name);
	  fe_perror (context, buf);
	  return;
	}
    }
  else
    {
      fe_globalPrefs.print_command =
	fe_StringTrim (fe_globalPrefs.print_command);
      if (! *fe_globalPrefs.print_command)
	{
	  FE_Alert (context, fe_globalData.no_print_command_message);
	  return;
	}
      p.out = popen (fe_globalPrefs.print_command, "w");
      if (! p.out)
	{
	  char buf [2048];
	  PR_snprintf (buf, sizeof (buf),
		    XP_GetString(XFE_ERROR_OPENING_PIPE),
		    fe_globalPrefs.print_command);
	  fe_perror (context, buf);
	  return;
	}
    }

#if 0
  /* Make sure layout saves the current state. */
  LO_SaveFormData(context);

  /* Hold on to the saved data. */
  XP_MEMCPY(&saved_data, &url->savedData, sizeof(SHIST_SavedData));
#endif /* 0 */

  /* make damn sure the form_data slot is zero'd or else all
   * hell will break loose
   */
  XP_MEMSET (&url->savedData, 0, sizeof (SHIST_SavedData));
  NPL_PreparePrint(context, &url->savedData);

  INTL_CharSetIDToName(context->win_csid, mimecharset);

  PR_snprintf(clas, sizeof (clas),
		"%s.DocumentFonts.Charset.PSName", fe_progclass);
  PR_snprintf(name, sizeof (name),
		"%s.documentFonts.%s.psname", fe_progclass, mimecharset);
  if (XrmGetResource(db, name, clas, &type, &value))
      p.otherFontName = value.addr;
  else
      p.otherFontName = NULL;

  PR_snprintf(clas, sizeof (clas),
		"%s.DocumentFonts.Charset.PSCode", fe_progclass);
  PR_snprintf(name, sizeof (name),
		"%s.documentFonts.%s.pscode", fe_progclass, mimecharset);
  if (XrmGetResource(db, name, clas, &type, &value))
      p.otherFontCharSetID = INTL_CharSetNameToID(value.addr);

  PR_snprintf(clas, sizeof (clas),
		"%s.DocumentFonts.Charset.PSWidth", fe_progclass);
  PR_snprintf(name, sizeof (name),
		"%s.documentFonts.%s.pswidth", fe_progclass, mimecharset);
  if (XrmGetResource(db, name, clas, &type, &value))
      p.otherFontWidth = atoi(value.addr);

#if 0
  PR_snprintf(clas, sizeof (clas),
		"%s.DocumentFonts.Charset.PSAscent", fe_progclass);
  PR_snprintf(name, sizeof (name),
		"%s.documentFonts.%s.psascent", fe_progclass, mimecharset);
  if (XrmGetResource(db, name, clas, &type, &value))
      p.otherFontAscent = atoi(value.addr);
#endif /* 0 */

  if (last_to_file_p)
      p.completion = ps_file_close;
  else
      p.completion = ps_pipe_close;
  
  p.carg = context;
  fe_RaiseSynchronousURLDialog (context, CONTEXT_WIDGET (context),
				"printing");
  XL_TranslatePostscript (context, url, &p);
  fe_await_synchronous_url (context);

  /* XXX do we need to delete the URL ? */
}


static void
fe_print_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  if (!fpd) return;

  /* Remove this callback so that we make absolutely sure we wont
   * free the fpd again.
   */
  XtRemoveCallback(widget, XmNdestroyCallback, fe_print_destroy_cb, fpd);
  free(fpd);
}

static void
fe_print_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_print_data *fpd = (struct fe_print_data *) closure;
  URL_Struct *url;
  Boolean b;

  /*
   * Pop down the print dialog immediately.
   */
  fe_UnmanageChild_safe (fpd->shell);

  XtVaGetValues (fpd->printer, XmNset, &b, 0);
  last_to_file_p = !b;
  if (fe_globalPrefs.print_command) free (fe_globalPrefs.print_command);
  XtVaGetValues (fpd->command_text, XmNvalue, &fe_globalPrefs.print_command,0);
  if (last_print_file_name) free (last_print_file_name);
  last_print_file_name = fe_GetTextField(fpd->file_text);
  XtVaGetValues (fpd->portrait, XmNset, &b, 0);
  fe_globalPrefs.print_landscape = !b;
  XtVaGetValues (fpd->last, XmNset, &b, 0);
  fe_globalPrefs.print_reversed = b;
  XtVaGetValues (fpd->grey, XmNset, &b, 0);
  fe_globalPrefs.print_color = !b;
  XtVaGetValues (fpd->letter, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_paper_size = 0;
  XtVaGetValues (fpd->legal, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_paper_size = 1;
  XtVaGetValues (fpd->exec, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_paper_size = 2;
  XtVaGetValues (fpd->a4, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_paper_size = 3;

#ifdef DEBUG_jwz
  XtVaGetValues (fpd->small, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_font_size = -1;
  XtVaGetValues (fpd->med, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_font_size = 0;
  XtVaGetValues (fpd->large, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_font_size = 1;
  XtVaGetValues (fpd->huge, XmNset, &b, 0);
  if (b) fe_globalPrefs.print_font_size = 2;

  XtVaGetValues (fpd->header_toggle, XmNset, &b, 0);
  fe_globalPrefs.print_header_p = b;
  XtVaGetValues (fpd->footer_toggle, XmNset, &b, 0);
  fe_globalPrefs.print_footer_p = b;

  {
    char *s;
    float f;
    char c;

    s = fe_GetTextField (fpd->header);
    if (fe_globalPrefs.print_header) free (fe_globalPrefs.print_header);
    s = fe_StringTrim (s);
    if (!*s) free (s), s = 0;
    fe_globalPrefs.print_header = s;

    s = fe_GetTextField (fpd->footer);
    if (fe_globalPrefs.print_footer) free (fe_globalPrefs.print_footer);
    s = fe_StringTrim (s);
    if (!*s) free (s), s = 0;
    fe_globalPrefs.print_footer = s;

    s = fe_GetTextField (fpd->hmargin);
    if (1 == sscanf (s, " %f %c", &f, &c))
      fe_globalPrefs.print_hmargin = (int) (f * 72);

    s = fe_GetTextField (fpd->vmargin);
    if (1 == sscanf (s, " %f %c", &f, &c))
      fe_globalPrefs.print_vmargin = (int) (f * 72);
  }
#endif /* DEBUG_jwz */


  url = SHIST_CreateWysiwygURLStruct (fpd->context, fpd->hist);
  if (url) {
    last_print_file_name = fe_StringTrim (last_print_file_name);
    
    fe_Print(fpd->context, url, last_to_file_p, last_print_file_name);
  } else {
    FE_Alert(fpd->context, fe_globalData.no_url_loaded_message);
  }
  
  /* We need this check to see if this widget is being_destroyed
   * or not in its ok_callback because fe_Print() goes off and
   * calls fe_await_synchronous_url() while has a mini eventloop
   * in it. This could cause a destroy of this widget. Motif, in
   * all its smartness, keeps this widget's memory around. This
   * is the only think we can check. context, shell, fpd all might
   * have been destroyed and deallocated.
   */
  if (XfeIsAlive(widget)) {
    XtDestroyWidget (fpd->shell);
  }
}


void
fe_PrintDialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget kids [100];
  Arg av [20];
  int ac, i /*, labels_width */;
  Widget shell, form;
  Widget print_to_label, print_command_label;
  Widget print_command_text, file_name_label, file_name_text;
  Widget browse_button, line, print_label;
  Widget first_first_toggle, last_first_toggle, orientation_label;
  Widget portrait_toggle, landscape_toggle;
  Widget print_color_label, greyscale_toggle;
  Widget color_toggle, paper_size_label, paper_size_radio_box, letter_toggle;
  Widget legal_toggle, executive_toggle, a4_toggle;
  Widget to_printer_toggle, to_file_toggle;

#ifdef DEBUG_jwz
  Widget line2;
  Widget font_size_label, small_toggle, med_toggle, large_toggle, huge_toggle;
  Widget header_label, header_text;
  Widget footer_label, footer_text;
  Widget header_doc_1, header_doc_2;
  Widget margins_label, hmargin_text, vmargin_text;
#endif /* DEBUG_jwz */

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  struct fe_print_data *fpd = (struct fe_print_data *)
    calloc (sizeof (struct fe_print_data), 1);

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNuserData, False); ac++;
  shell = XmCreatePromptDialog (mainw, "printSetup", av, ac);

  XtAddCallback (shell, XmNokCallback, fe_print_cb, fpd);
  XtAddCallback (shell, XmNcancelCallback, fe_destroy_cb, shell);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell,
						XmDIALOG_SELECTION_LABEL));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_HELP_BUTTON));
#endif
  XtAddCallback (shell, XmNdestroyCallback, fe_print_destroy_cb, fpd);

  i = 0;
  ac = 0;
  form = XmCreateForm (shell, "form", av, ac);
  kids [i++] = print_to_label =
    XmCreateLabelGadget (form, "printToLabel", av, ac);
  kids [i++] = to_printer_toggle =
    XmCreateToggleButtonGadget (form, "toPrinterToggle", av, ac);
  kids [i++] = to_file_toggle =
    XmCreateToggleButtonGadget (form, "toFileToggle", av, ac);
  kids [i++] = print_command_label =
    XmCreateLabelGadget (form, "printCommandLabel", av, ac);
  kids [i++] = print_command_text =
    fe_CreateTextField (form, "printCommandText", av, ac);
  kids [i++] = file_name_label =
    XmCreateLabelGadget (form, "fileNameLabel", av, ac);
  kids [i++] = file_name_text =
    fe_CreateTextField (form, "fileNameText", av, ac);
  kids [i++] = browse_button =
    XmCreatePushButtonGadget (form, "browseButton", av, ac);
  kids [i++] = line = XmCreateSeparator (form, "line", av, ac);
  kids [i++] = print_label = XmCreateLabelGadget (form, "printLabel", av, ac);
  kids [i++] = first_first_toggle =
    XmCreateToggleButtonGadget (form, "firstFirstToggle", av, ac);
  kids [i++] = last_first_toggle =
    XmCreateToggleButtonGadget (form, "lastFirstToggle", av, ac);
  kids [i++] = orientation_label =
    XmCreateLabelGadget (form, "orientationLabel", av, ac);
  kids [i++] = portrait_toggle =
    XmCreateToggleButtonGadget (form, "portraitToggle", av, ac);
  kids [i++] = landscape_toggle =
    XmCreateToggleButtonGadget (form, "landscapeToggle", av, ac);
  kids [i++] = print_color_label =
    XmCreateLabelGadget (form, "printColorLabel", av, ac);
  kids [i++] = greyscale_toggle =
    XmCreateToggleButtonGadget (form, "greyscaleToggle", av, ac);
  kids [i++] = color_toggle =
    XmCreateToggleButtonGadget (form, "colorToggle", av, ac);
  kids [i++] = paper_size_label =
    XmCreateLabelGadget (form, "paperSizeLabel", av, ac);
  kids [i++] = paper_size_radio_box =
    XmCreateRadioBox (form, "paperSizeRadioBox", av, ac);
  kids [i++] = letter_toggle =
    XmCreateToggleButtonGadget (form, "letterToggle", av, ac);
  kids [i++] = legal_toggle =
    XmCreateToggleButtonGadget (form, "legalToggle", av, ac);
  kids [i++] = executive_toggle =
    XmCreateToggleButtonGadget (form, "executiveToggle", av, ac);
  kids [i++] = a4_toggle =
    XmCreateToggleButtonGadget (form, "a4Toggle", av, ac);

#ifdef DEBUG_jwz
  kids [i++] = line2 = XmCreateSeparator (form, "line", av, ac);
  kids [i++] = font_size_label =
    XmCreateLabelGadget (form, "fontSizeLabel", av, ac);
  kids [i++] = small_toggle =
    XmCreateToggleButtonGadget (form, "smallToggle", av, ac);
  kids [i++] = med_toggle =
    XmCreateToggleButtonGadget (form, "mediumToggle", av, ac);
  kids [i++] = large_toggle =
    XmCreateToggleButtonGadget (form, "largeToggle", av, ac);
  kids [i++] = huge_toggle =
    XmCreateToggleButtonGadget (form, "hugeToggle", av, ac);

  kids [i++] = header_label =
/*    XmCreateLabelGadget (form, "headerLabel", av, ac);*/
    XmCreateToggleButtonGadget (form, "headerLabel", av, ac);
  kids [i++] = header_text =
    fe_CreateTextField (form, "headerText", av, ac);
  kids [i++] = footer_label =
/*    XmCreateLabelGadget (form, "footerLabel", av, ac);*/
    XmCreateToggleButtonGadget (form, "footerLabel", av, ac);
  kids [i++] = footer_text =
    fe_CreateTextField (form, "footerText", av, ac);

  kids [i++] = header_doc_1 =
    XmCreateLabelGadget (form, "headerDoc1", av, ac);
  kids [i++] = header_doc_2 =
    XmCreateLabelGadget (form, "headerDoc2", av, ac);

  kids [i++] = margins_label =
    XmCreateLabelGadget (form, "marginsLabel", av, ac);
  kids [i++] = hmargin_text =
    fe_CreateTextField (form, "hmarginText", av, ac);
  kids [i++] = vmargin_text =
    fe_CreateTextField (form, "vmarginText", av, ac);

#endif /* DEBUG_jwz */


#if 0
  labels_width = XfeVaGetWidestWidget(print_to_label, print_command_label,
			     file_name_label, 0);
#endif /* 0 */

  XtVaSetValues (print_to_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, to_printer_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, to_printer_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, to_printer_toggle,
		 0);
  XtVaSetValues (to_printer_toggle,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (to_file_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, to_printer_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, to_printer_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, to_printer_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (print_command_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, print_command_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, print_command_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, print_command_text,
		 0);
  XtVaSetValues (print_command_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, to_printer_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, to_printer_toggle,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (file_name_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, file_name_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, file_name_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, file_name_text,
		 0);
  XtVaSetValues (file_name_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, print_command_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, print_command_text,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, browse_button,
		 0);
  XtVaSetValues (browse_button,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, print_command_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  fe_attach_field_to_labels (to_printer_toggle,
			     print_to_label, print_command_label,
#ifdef DEBUG_jwz
                             margins_label,
#endif /* DEBUG_jwz */
			     file_name_label, 0);

  XtVaSetValues (browse_button, XmNheight, file_name_text->core.height, 0);

  XtVaSetValues (line,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, file_name_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (print_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, first_first_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, first_first_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, first_first_toggle,
		 0);
  XtVaSetValues (first_first_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, line,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, print_command_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (last_first_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, first_first_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, first_first_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, first_first_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (orientation_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, portrait_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, portrait_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, portrait_toggle,
		 0);
  XtVaSetValues (portrait_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, first_first_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, first_first_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (landscape_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, portrait_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, portrait_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, portrait_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (print_color_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, greyscale_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, greyscale_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, greyscale_toggle,
		 0);
  XtVaSetValues (greyscale_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, portrait_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, portrait_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (color_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, greyscale_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, greyscale_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, greyscale_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (paper_size_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, letter_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, letter_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, letter_toggle,
		 0);
  XtVaSetValues (letter_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, greyscale_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, greyscale_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (legal_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, letter_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, letter_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, letter_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (executive_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, letter_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, letter_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (a4_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, executive_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, executive_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, executive_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

#ifdef DEBUG_jwz
  XtVaSetValues (line2,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, executive_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (font_size_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, small_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, small_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, small_toggle,
		 0);
  XtVaSetValues (small_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, line2,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, executive_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (med_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, small_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, small_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, small_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (large_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, med_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, med_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, med_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (huge_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, large_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, large_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, large_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (header_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, header_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, header_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, header_text,
		 0);
  XtVaSetValues (header_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, small_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, small_toggle,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (footer_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, footer_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, footer_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, footer_text,
		 0);
  XtVaSetValues (footer_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, header_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, header_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (header_doc_1,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, footer_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, footer_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (header_doc_2,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, header_doc_1,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, header_doc_1,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (margins_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, hmargin_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, hmargin_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, hmargin_text,
		 0);
  XtVaSetValues (hmargin_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, header_doc_2,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, header_doc_2,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (vmargin_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, hmargin_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, hmargin_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, hmargin_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#endif /* DEBUG_jwz */


  XtAddCallback (to_printer_toggle, XmNvalueChangedCallback,
		 fe_print_to_toggle_cb, fpd);
  XtAddCallback (to_file_toggle, XmNvalueChangedCallback,
		 fe_print_to_toggle_cb, fpd);

  XtAddCallback (browse_button, XmNactivateCallback,
		 fe_print_browse_cb, fpd);

  XtAddCallback (first_first_toggle, XmNvalueChangedCallback,
		 fe_print_order_toggle_cb, fpd);
  XtAddCallback (last_first_toggle, XmNvalueChangedCallback,
		 fe_print_order_toggle_cb, fpd);

  XtAddCallback (portrait_toggle, XmNvalueChangedCallback,
		 fe_print_orientation_toggle_cb, fpd);
  XtAddCallback (landscape_toggle, XmNvalueChangedCallback,
		 fe_print_orientation_toggle_cb, fpd);

  XtAddCallback (greyscale_toggle, XmNvalueChangedCallback,
		 fe_print_color_toggle_cb, fpd);
  XtAddCallback (color_toggle, XmNvalueChangedCallback,
		 fe_print_color_toggle_cb, fpd);

  XtAddCallback (letter_toggle, XmNvalueChangedCallback,
		 fe_print_paper_toggle_cb, fpd);
  XtAddCallback (legal_toggle, XmNvalueChangedCallback,
		 fe_print_paper_toggle_cb, fpd);
  XtAddCallback (executive_toggle, XmNvalueChangedCallback,
		 fe_print_paper_toggle_cb, fpd);
  XtAddCallback (a4_toggle, XmNvalueChangedCallback,
		 fe_print_paper_toggle_cb, fpd);

#ifdef DEBUG_jwz
  XtAddCallback (small_toggle, XmNvalueChangedCallback,
		 fe_print_size_toggle_cb, fpd);
  XtAddCallback (med_toggle, XmNvalueChangedCallback,
		 fe_print_size_toggle_cb, fpd);
  XtAddCallback (large_toggle, XmNvalueChangedCallback,
		 fe_print_size_toggle_cb, fpd);
  XtAddCallback (huge_toggle, XmNvalueChangedCallback,
		 fe_print_size_toggle_cb, fpd);
#endif /* DEBUG_jwz */


  XtVaSetValues (print_command_text, XmNvalue, fe_globalPrefs.print_command,0);
  if (!last_print_file_name || !*last_print_file_name) {
	/* Use a default file name. We need to strdup here as we free this
	   later. */
	last_print_file_name = strdup( XFE_DEFAULT_PRINT_FILENAME );
  }
  fe_SetTextField(file_name_text, last_print_file_name);
  if (last_to_file_p)
    {
      XtVaSetValues (to_file_toggle, XmNset, True, 0);
      XtVaSetValues (print_command_text, XmNsensitive, False, 0);
      XtVaSetValues (shell, XmNinitialFocus, file_name_text, 0);
    }
  else
    {
      XtVaSetValues (to_printer_toggle, XmNset, True, 0);
      XtVaSetValues (file_name_text, XmNsensitive, False, 0);
      XtVaSetValues (browse_button, XmNsensitive, False, 0);
      XtVaSetValues (shell, XmNinitialFocus, print_command_text, 0);
    }
  XtVaSetValues ((fe_globalPrefs.print_reversed
		  ? last_first_toggle : first_first_toggle),
		 XmNset, True, 0);
  XtVaSetValues ((fe_globalPrefs.print_landscape
		  ? landscape_toggle : portrait_toggle),
		 XmNset, True, 0);
  XtVaSetValues ((fe_globalPrefs.print_color
		  ? color_toggle : greyscale_toggle),
		 XmNset, True, 0);
  XtVaSetValues ((fe_globalPrefs.print_paper_size == 0 ? letter_toggle :
		  fe_globalPrefs.print_paper_size == 1 ? legal_toggle :
		  fe_globalPrefs.print_paper_size == 2 ? executive_toggle :
		  a4_toggle),
		 XmNset, True, 0);

#ifdef DEBUG_jwz
  XtVaSetValues ((fe_globalPrefs.print_font_size == -1 ? small_toggle :
		  fe_globalPrefs.print_font_size ==  1 ? large_toggle :
		  fe_globalPrefs.print_font_size ==  2 ? huge_toggle :
                  med_toggle),
		 XmNset, True, 0);
  XtVaSetValues (header_text, XmNvalue, fe_globalPrefs.print_header, 0);
  XtVaSetValues (footer_text, XmNvalue, fe_globalPrefs.print_footer, 0);

  XtVaSetValues (header_label, XmNset, fe_globalPrefs.print_header_p, 0);
  XtVaSetValues (footer_label, XmNset, fe_globalPrefs.print_footer_p, 0);

  {
    char s[30];
    sprintf (s, "%.1f", (float) (fe_globalPrefs.print_hmargin / 72.0));
    XtVaSetValues (hmargin_text, XmNvalue, s, 0);
    sprintf (s, "%.1f", (float) (fe_globalPrefs.print_vmargin / 72.0));
    XtVaSetValues (vmargin_text, XmNvalue, s, 0);
  }
#endif /* DEBUG_jwz */


  XtManageChildren (kids, i);
  XtManageChild (form);

  fpd->context = context;
  fpd->hist = SHIST_GetCurrent (&context->hist);
  fpd->shell = shell;
  fpd->printer = to_printer_toggle;
  fpd->file = to_file_toggle;
  fpd->command_text = print_command_text;
  fpd->file_text = file_name_text;
  fpd->browse = browse_button;
  fpd->first = first_first_toggle;
  fpd->last = last_first_toggle;
  fpd->portrait = portrait_toggle;
  fpd->landscape = landscape_toggle;
  fpd->grey = greyscale_toggle;
  fpd->color = color_toggle;
  fpd->letter = letter_toggle;
  fpd->legal = legal_toggle;
  fpd->exec = executive_toggle;
  fpd->a4 = a4_toggle;

#ifdef DEBUG_jwz
  fpd->small = small_toggle;
  fpd->med = med_toggle;
  fpd->large = large_toggle;
  fpd->huge = huge_toggle;
  fpd->header = header_text;
  fpd->footer = footer_text;
  fpd->header_toggle = header_label;
  fpd->footer_toggle = footer_label;
  fpd->hmargin = hmargin_text;
  fpd->vmargin = vmargin_text;
#endif /* DEBUG_jwz */

  fe_NukeBackingStore (shell);
  XtManageChild (shell);
}



/* Find dialog. */

static void
fe_find_refresh_data(fe_FindData *find_data)
{
  MWContext *focus_context;

  if (!find_data)
    return;

  /* Decide which context to search in */
  focus_context = fe_GetFocusGridOfContext(find_data->context);
  if (focus_context) {
    if (focus_context != find_data->context_to_find)
      fe_FindReset(find_data->context);
  }
  find_data->context_to_find = focus_context;

  if (find_data->shell) {
    unsigned char *loc;

    if (find_data->string)
      XP_FREE (find_data->string);
    find_data->string = 0;
    XtVaGetValues (find_data->text, XmNvalue, &loc, 0);
    find_data->string = (char *) fe_ConvertFromLocaleEncoding (
					find_data->context->win_csid, loc);
    if (find_data->string != (char *) loc) {
      free (loc);
    }
    XtVaGetValues (find_data->case_sensitive, XmNset,
		   &find_data->case_sensitive_p, 0);
    XtVaGetValues (find_data->backward,
		   XmNset, &find_data->backward_p, 0);
    /* For mail/news contexts, the Search in Header/Body is loaded into the
       fe_FindData in the valueChangeCallback */
  }
}

#define FE_FIND_FOUND			0
#define FE_FIND_NOTFOUND		1
#define FE_FIND_CANCEL			2
#define FE_FIND_HEADER_FOUND		3
#define FE_FIND_HEADER_NOTFOUND		4
#define FE_FIND_NOSTRING		5


static char *last_find_string = 0;


static int
fe_find (fe_FindData *find_data)
{
  Widget mainw;
  MWContext *context_to_find;

  XP_ASSERT(find_data);

  /* Find a shell to use with all subsequent dialogs */
  if (find_data->shell && XtIsManaged(find_data->shell))
    mainw = find_data->shell;
  else
    mainw = CONTEXT_WIDGET(find_data->context);
  while(!XtIsWMShell(mainw) && (XtParent(mainw)!=0))
    mainw = XtParent(mainw);

  /* reload search parameters */
  fe_find_refresh_data(find_data);

  context_to_find = find_data->context;
  if (find_data->context_to_find)
    context_to_find = find_data->context_to_find;

  if (!find_data->string || !find_data->string[0]) {
    fe_Alert_2(mainw, fe_globalData.no_search_string_message);
    return (FE_FIND_NOSTRING);
  }

  if (last_find_string)
    free (last_find_string);
  last_find_string = strdup (find_data->string);

  if (find_data->find_in_headers) {
    XP_ASSERT(find_data->context->type == MWContextMail ||
	      find_data->context->type == MWContextNews);
    if (find_data->context->type == MWContextMail ||
	find_data->context->type == MWContextNews) {
      int status = MSG_DoFind(find_data->context, find_data->string,
			      find_data->case_sensitive_p);
      if (status < 0) {
	/* mainw could be the find_data->shell. If status < 0 (find failed)
	 * backend will bring the find window down. So use the context to
	 * display the error message here.
	 */
	FE_Alert(find_data->context, XP_GetString(status));
	return(FE_FIND_HEADER_NOTFOUND);
      }
      return (FE_FIND_HEADER_FOUND);
    }
  }

/*#ifdef EDITOR*/
  /* but I think you will want this in non-Gold too! */
  /*
   *    Start the search from the current selection location. Bug #29854.
   */
  LO_GetSelectionEndpoints(context_to_find,
			   &find_data->start_element,
			   &find_data->end_element,
			   &find_data->start_pos,
			   &find_data->end_pos);
/*#endif*/
  AGAIN:

  if (LO_FindText (context_to_find, find_data->string,
		   &find_data->start_element, &find_data->start_pos, 
		   &find_data->end_element, &find_data->end_pos,
		   find_data->case_sensitive_p, !find_data->backward_p))
    {
      int32 x, y;
      LO_SelectText (context_to_find,
		     find_data->start_element, find_data->start_pos,
		     find_data->end_element, find_data->end_pos,
		     &x, &y);

      /* If the found item is not visible on the screen, scroll to it.
	 If we need to scroll, attempt to position the destination
	 coordinate in the middle of the window.
	 */
      if (x >= CONTEXT_DATA (context_to_find)->document_x &&
	  x <= (CONTEXT_DATA (context_to_find)->document_x +
		CONTEXT_DATA (context_to_find)->scrolled_width))
	x = CONTEXT_DATA (context_to_find)->document_x;
      else
	x = x - (CONTEXT_DATA (context_to_find)->scrolled_width / 2);

      if (y >= CONTEXT_DATA (context_to_find)->document_y &&
	  y <= (CONTEXT_DATA (context_to_find)->document_y +
		CONTEXT_DATA (context_to_find)->scrolled_height))
	y = CONTEXT_DATA (context_to_find)->document_y;
      else
	y = y - (CONTEXT_DATA (context_to_find)->scrolled_height / 2);

      if (x + CONTEXT_DATA (context_to_find)->scrolled_width
	  > CONTEXT_DATA (context_to_find)->document_width)
	x = (CONTEXT_DATA (context_to_find)->document_width -
	     CONTEXT_DATA (context_to_find)->scrolled_width);

      if (y + CONTEXT_DATA (context_to_find)->scrolled_height
	  > CONTEXT_DATA (context_to_find)->document_height)
	y = (CONTEXT_DATA (context_to_find)->document_height -
	     CONTEXT_DATA (context_to_find)->scrolled_height);

      if (x < 0) x = 0;
      if (y < 0) y = 0;

      fe_ScrollTo (context_to_find, x, y);
    }
  else
    {
      if (fe_Confirm_2 (mainw,
			(find_data->backward_p
			 ? fe_globalData.wrap_search_backward_message
			 : fe_globalData.wrap_search_message)))	{
	find_data->start_element = 0;
	find_data->start_pos = 0;
	goto AGAIN;
      }
      else
	return (FE_FIND_CANCEL);
    }
  return(FE_FIND_FOUND);
}

static void
fe_find_destroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_FindData *find_data = CONTEXT_DATA(context)->find_data;
  if (find_data) {
    /* Before we destroy, load all the search parameters */
    fe_find_refresh_data(find_data);
    find_data->shell = 0;
  }
  /* find_data will be freed when the context is deleted */
}


static void
fe_find_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  MWContext *context = (MWContext *) closure;
  fe_FindData *find_data = CONTEXT_DATA(context)->find_data;
  int result;

  XP_ASSERT(find_data && find_data->shell);

  switch (cb->reason) {
  case XmCR_OK:		/* ok */
    result = fe_find(find_data);
    if (result == FE_FIND_HEADER_FOUND ||
	result == FE_FIND_HEADER_NOTFOUND)
      XtUnmanageChild(find_data->shell);
    break;
  case XmCR_APPLY:	/* clear */
    XtVaSetValues (find_data->text, XmNcursorPosition, 0, 0);
    XtVaSetValues (find_data->text, XmNvalue, "", 0);
    XmProcessTraversal (find_data->text, XmTRAVERSE_CURRENT);
    break;
  case XmCR_CANCEL:	/* cancel */
  default:
    XtUnmanageChild(find_data->shell);
  }
}

static void
fe_find_head_or_body_changed(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
  fe_FindData *find_data = (fe_FindData *) closure;
  XP_ASSERT(find_data);
  if (widget == find_data->msg_body) {
    find_data->find_in_headers = FALSE;
  } else {
    XP_ASSERT(widget == find_data->msg_head);
    find_data->find_in_headers = TRUE;
  }
  XmToggleButtonGadgetSetState(find_data->msg_body, !find_data->find_in_headers, FALSE);
  XmToggleButtonGadgetSetState(find_data->msg_head, find_data->find_in_headers, FALSE);
  XtVaSetValues(find_data->backward, XmNsensitive, !find_data->find_in_headers, 0);
}
  

void
fe_FindDialog (MWContext *context, Boolean again)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget kids [50];
  Arg av [20];
  int ac;
  Widget shell, form, find_label, find_text;
  Widget findin = 0;
  Widget msg_head = 0;
  Widget msg_body = 0;
  Widget case_sensitive_toggle, backwards_toggle;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  fe_FindData *find_data = CONTEXT_DATA(context)->find_data;
  char *loc;

  /* Force Find if this was the first time */
  if (!find_data && again)
      again = False;

  if (again) {
    if (find_data->find_in_headers) {
      XP_ASSERT(find_data->context->type == MWContextMail ||
		find_data->context->type == MWContextNews);
      if (find_data->context->type == MWContextMail ||
	  find_data->context->type == MWContextNews) {
	MSG_Command(find_data->context, MSG_FindAgain);
	return;
      }
    }
    fe_find (find_data);
    return;
  }
  else if (find_data && find_data->shell) {
    /* The find dialog is already there. Use it. */
    XtManageChild(find_data->shell);
    XMapRaised(XtDisplay(find_data->shell),
	       XtWindow(find_data->shell));
    return;
  }

  /* Create the find dialog */
  if (!find_data) {
    find_data = (fe_FindData *) XP_NEW_ZAP (fe_FindData);
    CONTEXT_DATA(context)->find_data = find_data;
    find_data->case_sensitive_p = False;
    if (context->type == MWContextMail || context->type == MWContextNews)
      find_data->find_in_headers = True;
    else
      find_data->find_in_headers = False;
  }

  fe_getVisualOfContext(context, &v, &cmap, &depth);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNuserData, find_data); ac++;	/* this must go */
  shell = XmCreatePromptDialog (mainw, "findDialog", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell,
						XmDIALOG_SELECTION_LABEL));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_TEXT));
  XtManageChild (XmSelectionBoxGetChild (shell, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_HELP_BUTTON));
#endif

  ac = 0;
  form = XmCreateForm (shell, "form", av, ac);

  if (context->type == MWContextMail || context->type == MWContextNews) {
    ac = 0;
    findin = XmCreateLabelGadget(form, "findInLabel", av, ac);
    ac = 0;
    msg_head = XmCreateToggleButtonGadget(form, "msgHeaders", av, ac);
    ac = 0;
    msg_body = XmCreateToggleButtonGadget(form, "msgBody", av, ac);
    XtAddCallback(msg_body, XmNvalueChangedCallback,
		  fe_find_head_or_body_changed, find_data);
    XtAddCallback(msg_head, XmNvalueChangedCallback,
		  fe_find_head_or_body_changed, find_data);
  }

  ac = 0;
  find_label = XmCreateLabelGadget (form, "findLabel", av, ac);
  ac = 0;


  /* default to the last thing searched for, already! */
  if (!find_data->string && last_find_string)
    find_data->string = strdup(last_find_string);


  loc = (char *) fe_ConvertToLocaleEncoding (context->win_csid,
				    (unsigned char *) find_data->string);
  XtSetArg (av [ac], XmNvalue, loc ? loc : ""); ac++;
  find_text = fe_CreateTextField (form, "findText", av, ac);
  if (loc && (loc != find_data->string)) {
    free (loc);
  }
  ac = 0;
  XtSetArg (av [ac], XmNset, find_data->case_sensitive_p); ac++;
  case_sensitive_toggle = XmCreateToggleButtonGadget (form, "caseSensitive",
						      av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNset, find_data->backward_p); ac++;
  backwards_toggle = XmCreateToggleButtonGadget (form, "backwards", av, ac);

  XtAddCallback (shell, XmNdestroyCallback, fe_find_destroy_cb, context);
  XtAddCallback (shell, XmNokCallback, fe_find_cb, context);
  XtAddCallback (shell, XmNcancelCallback, fe_find_cb, context);
  XtAddCallback (shell, XmNapplyCallback, fe_find_cb, context);

  find_data->msg_head = msg_head;
  find_data->msg_body = msg_body;
  find_data->context = context;
  find_data->context_to_find = fe_GetFocusGridOfContext(context);
  find_data->shell = shell;
  find_data->text = find_text;
  find_data->case_sensitive = case_sensitive_toggle;
  find_data->backward = backwards_toggle;

  if (findin) {
    XtVaSetValues(findin,
		  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNtopWidget, msg_head,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_WIDGET,
		  XmNrightWidget, msg_head,
		  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNbottomWidget, msg_head,
		  0);
    XtVaSetValues(msg_head,
		  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNtopWidget, msg_body,
		  XmNrightAttachment, XmATTACH_WIDGET,
		  XmNrightWidget, msg_body,
		  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNbottomWidget, msg_body,
		  0);
    XtVaSetValues(msg_body,
		  XmNtopAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_NONE,
		  0);
  }

  XtVaSetValues (find_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, find_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, find_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, find_text,
		 0);
  XtVaSetValues (find_text,
		 XmNtopAttachment, msg_body ? XmATTACH_WIDGET : XmATTACH_FORM,
		 XmNtopWidget, msg_body,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 10,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (case_sensitive_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, find_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, find_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (backwards_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, case_sensitive_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, case_sensitive_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, case_sensitive_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  ac = 0;
  if (findin) {
    fe_find_head_or_body_changed(find_data->find_in_headers ? msg_head : msg_body,
				 find_data, 0);
    kids[ac++] = findin;
    kids[ac++] = msg_head;
    kids[ac++] = msg_body;
  }
  kids[ac++] = find_label;
  kids[ac++] = find_text;
  kids[ac++] = case_sensitive_toggle;
  kids[ac++] = backwards_toggle;
  XtManageChildren (kids, ac);
  ac = 0;
  XtManageChild (form);
  XtVaSetValues (form, XmNinitialFocus, find_text, 0);

  fe_NukeBackingStore (shell);
  XtManageChild (shell);
}

/* When a new layout begins, the find data is invalid.
 */
void
fe_FindReset (MWContext *context)
{
  fe_FindData *find_data;
  MWContext *top_context = fe_GridToTopContext(context);

  find_data = CONTEXT_DATA(top_context)->find_data;
  
  if (!find_data) return;

  if (find_data->context_to_find == context ||
      find_data->context == context) {
    find_data->start_element = 0;
    find_data->end_element = 0;
    find_data->start_pos = 0;
    find_data->end_pos = 0;
  }
}


/* Streaming audio
 */

int
XFE_AskStreamQuestion (MWContext *context)
{
  struct fe_confirm_data data;
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  dialog = XmCreatePromptDialog (mainw, "streamingAudioQuery", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);

  XtAddCallback (dialog, XmNokCallback, fe_destroy_ok_cb, &data);
  XtAddCallback (dialog, XmNapplyCallback, fe_destroy_apply_cb, &data);
  XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cancel_cb, &data);
  XtAddCallback (dialog, XmNdestroyCallback, fe_destroy_finish_cb, &data);

  data.context = context;
  data.widget = dialog;
  data.answer = Answer_Invalid;

  while (data.answer == Answer_Invalid)
    fe_EventLoop ();

  if (data.answer == Answer_Cancel || data.answer == Answer_Destroy)
    return -1;
  else if (data.answer == Answer_OK)
    return 1;
  else if (data.answer == Answer_Apply)
    return 0;
  else
    abort ();
}


/* Care and feeding of lawyers and other parasites */

#if 0
static XtErrorHandler old_xt_warning_handler = NULL;

static void 
xt_warning_handler(String msg)
{
  if (!msg)
    {
      return;
    }

#ifndef DEBUG
  /* Ignore warnings that contain "Actions not found" (non debug only) */
  if (XP_STRSTR(msg,"Actions not found"))
    {
      return;
    }
#endif
	
  (*old_xt_warning_handler)(msg);
}
#endif /* 0 */

void
fe_LicenseDialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget form, label1, text, label2;
  Widget accept, reject;

  char crockery [1024];
  PR_snprintf (crockery, sizeof (crockery), "%d ", getuid ());
  strcat (crockery, fe_version);

  if (fe_VendorAnim)
    return;

  if (fe_globalPrefs.license_accepted &&
      !strcmp (crockery, fe_globalPrefs.license_accepted))
    return;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNdefaultButton, 0); ac++;
  dialog = XmCreateTemplateDialog (mainw, "licenseDialog", av, ac);

  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
/*  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));*/
  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog,XmDIALOG_DEFAULT_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));

  ac = 0;
  accept = XmCreatePushButtonGadget (dialog, "accept", av, ac);
  reject = XmCreatePushButtonGadget (dialog, "reject", av, ac);
  form = XmCreateForm (dialog, "form", av, ac);
  label1 = XmCreateLabelGadget (form, "label1", av, ac);

  XtSetArg (av [ac], XmNeditable, False); ac++;
  XtSetArg (av [ac], XmNcursorPositionVisible, False); ac++;
  XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
  text = XmCreateScrolledText (form, "text", av, ac);
  fe_HackDialogTranslations (text);
  ac = 0;
  label2 = XmCreateLabelGadget (form, "label2", av, ac);

  XtVaSetValues (label1,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (XtParent (text),
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, label1,
		 XmNbottomAttachment, XmATTACH_WIDGET,
		 XmNbottomWidget, label2,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (label2,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  {
    char *license = 0;
    int32 length = 0;
    char *ctype = 0;
    void *data = FE_AboutData ("license", &license, &length, &ctype);
    if (!license || length < 300)
      {
        license = "Please fill in";
        /* abort (); */
      }
    XtVaSetValues (text, XmNvalue, license, 0);
    if (data) free (data);
  }

  XtManageChild (label1);
  XtManageChild (text);
  XtManageChild (XtParent (text));
  XtManageChild (label2);
  XtManageChild (form);
  XtManageChild (accept);
  XtManageChild (reject);

  {
    struct fe_confirm_data data;
    XtVaSetValues (dialog, XmNdefaultButton, accept, 0);
    XtAddCallback (accept, XmNactivateCallback, fe_destroy_ok_cb, &data);
    XtAddCallback (reject, XmNactivateCallback, fe_destroy_cancel_cb, &data);
    XtAddCallback (dialog, XmNdestroyCallback, fe_destroy_finish_cb, &data);

    data.context = context;
    data.widget = dialog;
    data.answer = Answer_Invalid;

    fe_NukeBackingStore (dialog);
#if 0
    /* Install a warning handler that ignores translation warnings */
    old_xt_warning_handler = XtAppSetWarningHandler(fe_XtAppContext,
						    xt_warning_handler);
#endif /* 0 */

    XtManageChild (dialog);

#if 0
    /* Restore the original warning handler after the dialog is managed */
    XtAppSetWarningHandler(fe_XtAppContext,old_xt_warning_handler);
#endif /* 0 */

    while (data.answer == Answer_Invalid)
      fe_EventLoop ();

    if (data.answer == Answer_Cancel || data.answer == Answer_Destroy)
      {
	if (fe_pidlock) unlink (fe_pidlock);
	exit (0);
      }
    else if (data.answer == Answer_OK)
      {
	/* Store crockery into preferences, and save. */
	StrAllocCopy (fe_globalPrefs.license_accepted, crockery);
	if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file,
			    &fe_globalPrefs))
	  fe_perror (context, XP_GetString(XFE_CANT_SAVE_PREFS));
	return;
      }
    else
      {
	abort ();
      }
  }
}

#ifdef OLD_DOCINFO_WAY
/* Document info
 */

struct fe_docinfo_data
{
  Widget dialog;
  Widget title_text, url_text, modified_text, source_text;
  Widget charset_text, dpy_text;
  Widget key_desc_label;
  Widget certificate_label, certificate_text;
  Widget desc_label;
};

static void
fe_close_docinfo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  if (! CONTEXT_DATA (context)->did) return;
  XtUnmanageChild (CONTEXT_DATA (context)->did->dialog);
}

static void
fe_make_docinfo_dialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  struct fe_docinfo_data *did = CONTEXT_DATA (context)->did;
  Widget shell, form;
  Widget url_label, url_text;
  Widget title_label, title_text;
  Widget mod_label, mod_text;
#ifdef DOCINFO_SOURCE_TEXT
  Widget source_label, source_text;
#endif
#ifdef DOCINFO_CHARSET_TEXT
  Widget charset_label, charset_text;
#endif
#ifdef DOCINFO_VISUAL_TEXT
  Widget dpy_label, dpy_text;
#endif
  Widget sec_frame;
  Widget  sec_box_label;
  Widget  sec_box;
  Widget   key_desc_label;
  Widget   certificate_label;
  Widget   certificate_text;
  Widget   sec_label;
  Widget ok_button;
  Arg av [20];
  int ac;
  Widget kids [30];
  int i;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  if (did)
    return;

  did = (struct fe_docinfo_data *) calloc (sizeof (struct fe_docinfo_data), 1);

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;

  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;

  XtSetArg (av[ac], XmNdialogType, XmDIALOG_INFORMATION); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  shell = XmCreateTemplateDialog (mainw, "docinfo", av, ac);
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_OK_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_CANCEL_BUTTON));
/*  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_APPLY_BUTTON));*/
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_DEFAULT_BUTTON));
  fe_UnmanageChild_safe (XmMessageBoxGetChild (shell, XmDIALOG_HELP_BUTTON));

  ac = 0;
  ok_button = XmCreatePushButtonGadget (shell, "OK", av, ac);

  ac = 0;
  form = XmCreateForm (shell, "form", av, ac);

  ac = 0;
  title_label  = XmCreateLabelGadget (form, "titleLabel", av, ac);
  url_label    = XmCreateLabelGadget (form, "urlLabel", av, ac);
  mod_label    = XmCreateLabelGadget (form, "modifiedLabel", av, ac);
#ifdef DOCINFO_SOURCE_TEXT
  source_label = XmCreateLabelGadget (form, "sourceLabel", av, ac);
#endif
#ifdef DOCINFO_CHARSET_TEXT
  charset_label= XmCreateLabelGadget (form, "charsetLabel", av, ac);
#endif
#ifdef DOCINFO_VISUAL_TEXT
  dpy_label    = XmCreateLabelGadget (form, "dpyLabel", av, ac);
#endif
  ac = 0;
  XtSetArg (av [ac], XmNeditable, False); ac++;
  XtSetArg (av [ac], XmNcursorPositionVisible, False); ac++;
  title_text  = fe_CreateTextField (form, "titleText", av, ac);
  url_text    = fe_CreateTextField (form, "urlText", av, ac);
  mod_text    = fe_CreateTextField (form, "modifiedText", av, ac);
#ifdef DOCINFO_SOURCE_TEXT
  source_text = fe_CreateTextField (form, "sourceText", av, ac);
#endif
#ifdef DOCINFO_CHARSET_TEXT
  charset_text= fe_CreateTextField (form, "charsetText", av, ac);
#endif
#ifdef DOCINFO_VISUAL_TEXT
  XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
  dpy_text   = fe_CreateText (form, "dpyText", av, ac);
#endif

  ac = 0;
#ifndef HAVE_SECURITY
  XtSetArg (av [ac], XmNsensitive, False); ac++;
#endif /* !HAVE_SECURITY */
  sec_frame = XmCreateFrame (form, "securityFrame", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  sec_box_label = XmCreateLabelGadget (sec_frame, "label", av, ac);

  ac = 0;
  sec_box = XmCreateForm (sec_frame, "securityBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNeditable, False); ac++;
  XtSetArg (av [ac], XmNcursorPositionVisible, True); ac++;
  XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
  ac = 0;
  key_desc_label = XmCreateLabelGadget (sec_box, "keyDescLabel", av, ac);
  certificate_label = XmCreateLabelGadget (sec_box, "certificateLabel", av,ac);

  ac = 0;
  XtSetArg (av [ac], XmNeditable, False); ac++;
  XtSetArg (av [ac], XmNcursorPositionVisible, False); ac++;
  XtSetArg (av [ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
  certificate_text = XmCreateScrolledText (sec_box, "certificateText", av, ac);
  fe_HackDialogTranslations (certificate_text);

  ac = 0;
  sec_label = XmCreateLabelGadget (sec_box, "securityLabel", av, ac);

  XtVaSetValues (title_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, title_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, title_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, title_text,
		 0);
  XtVaSetValues (url_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, url_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, url_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, url_text,
		 0);
  XtVaSetValues (mod_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mod_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mod_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, mod_text,
		 0);
#ifdef DOCINFO_SOURCE_TEXT
  XtVaSetValues (source_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, source_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, source_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, source_text,
		 0);
#endif
#ifdef DOCINFO_CHARSET_TEXT
  XtVaSetValues (charset_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, charset_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, charset_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, charset_text,
		 0);
#endif
#ifdef DOCINFO_VISUAL_TEXT
  XtVaSetValues (dpy_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, dpy_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, dpy_text,
		 0);
#endif

  XtVaSetValues (title_text,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (url_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, title_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, title_text,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, title_text,
		 XmNrightOffset, 0,
		 0);
  XtVaSetValues (mod_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, url_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, url_text,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, url_text,
		 XmNrightOffset, 0,
		 0);
#ifdef DOCINFO_SOURCE_TEXT
  XtVaSetValues (source_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mod_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mod_text,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, mod_text,
		 XmNrightOffset, 0,
		 0);
#endif
#ifdef DOCINFO_CHARSET_TEXT
  XtVaSetValues (charset_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftOffset, 0,
		 XmNrightOffset, 0,
# ifdef DOCINFO_SOURCE_TEXT
		 XmNtopWidget, source_text,
		 XmNleftWidget, source_text,
		 XmNrightWidget, source_text,
# else
		 XmNtopWidget, mod_text,
		 XmNleftWidget, mod_text,
		 XmNrightWidget, mod_text,
# endif
		 0);
#endif
#ifdef DOCINFO_VISUAL_TEXT
  XtVaSetValues (dpy_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftOffset, 0,
		 XmNrightOffset, 0,
# ifdef DOCINFO_CHARSET_TEXT
		 XmNtopWidget, charset_text,
		 XmNleftWidget, charset_text,
		 XmNrightWidget, charset_text,
# elif defined(DOCINFO_SOURCE_TEXT)
		 XmNtopWidget, source_text,
		 XmNleftWidget, source_text,
		 XmNrightWidget, source_text,
# else
		 XmNtopWidget, mod_text,
		 XmNleftWidget, mod_text,
		 XmNrightWidget, mod_text,
# endif
		 0);
#endif

  fe_attach_field_to_labels (title_text,
			     title_label, url_label, mod_label,
#ifdef DOCINFO_SOURCE_TEXT
			     source_label,
#endif
#ifdef DOCINFO_CHARSET_TEXT
			     charset_label,
#endif
#ifdef DOCINFO_VISUAL_TEXT
			     dpy_label,
#endif
			     0);

  XtVaSetValues (sec_frame,
		 XmNtopAttachment, XmATTACH_WIDGET,
#ifdef DOCINFO_VISUAL_TEXT
		 XmNtopWidget, dpy_text,
#else
# ifdef DOCINFO_SOURCE_TEXT
		 XmNtopWidget, source_text,
# else
		 XmNtopWidget, mod_text,
# endif
#endif
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (key_desc_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (certificate_label,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, key_desc_label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (XtParent (certificate_text),
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, certificate_label,
		 XmNbottomAttachment, XmATTACH_WIDGET,
		 XmNbottomWidget, sec_label,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (sec_label,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtManageChild (certificate_text);

  i = 0;
  kids [i++] = key_desc_label;
  kids [i++] = certificate_label;
  kids [i++] = XtParent (certificate_text);
  kids [i++] = sec_label;
  XtManageChildren (kids, i);

  i = 0;
  kids [i++] = sec_box_label;
  kids [i++] = sec_box;
  XtManageChildren (kids, i);

  i = 0;
  kids [i++] = title_label;
  kids [i++] = title_text;
  kids [i++] = url_label;
  kids [i++] = url_text;
  kids [i++] = mod_label;
  kids [i++] = mod_text;
#ifdef DOCINFO_SOURCE_TEXT
  kids [i++] = source_label;
  kids [i++] = source_text;
#endif
#ifdef DOCINFO_CHARSET_TEXT
  kids [i++] = charset_label;
  kids [i++] = charset_text;
#endif
#ifdef DOCINFO_VISUAL_TEXT
  kids [i++] = dpy_label;
  kids [i++] = dpy_text;
#endif
  kids [i++] = sec_frame;
  XtManageChildren (kids, i);
  XtManageChild (form);
  XtManageChild (ok_button);

  XtVaSetValues (shell, XmNdefaultButton, ok_button, 0);
  XtAddCallback (ok_button, XmNactivateCallback, fe_close_docinfo_cb, context);

  did->dialog = shell;
  did->title_text = title_text;
  did->url_text = url_text;
  did->modified_text = mod_text;
#ifdef DOCINFO_SOURCE_TEXT
  did->source_text = source_text;
#endif
#ifdef DOCINFO_CHARSET_TEXT
  did->charset_text = charset_text;
#endif
#ifdef DOCINFO_VISUAL_TEXT
  did->dpy_text = dpy_text;
#endif
  did->key_desc_label = key_desc_label;
  did->certificate_text = certificate_text;

  CONTEXT_DATA (context)->did = did;
}

void
fe_UpdateDocInfoDialog (MWContext *context)
{
  struct fe_docinfo_data *did = CONTEXT_DATA (context)->did;

  History_entry *h;
  XmString xmstring;
  int security_level = 0;
#ifdef DOCINFO_CHARSET_TEXT
  char *charset = 0;
#endif
  char *cert = 0;
  char *cipher = 0;
  String sec;
  time_t mod = 0;
  char *mods;
#ifdef DOCINFO_SOURCE_TEXT
  char *source_text;
#endif
#ifdef DOCINFO_VISUAL_TEXT
  char dpy_text [1024];
#endif

  if (! did)
    return;

  h = SHIST_GetCurrent (&context->hist);

  if (h)
    {
#ifdef DOCINFO_CHARSET_TEXT
      charset = INTL_CharSetDocInfo (context);
#endif
      mod = h->last_modified;
      security_level = h->security_on;
      cipher = XP_PrettySecurityStatus(security_level,
				       h->key_cipher,
				       h->key_size,
				       h->key_secret_size);
      cert = XP_PrettyCertInfo (h->certificate);
    }

  if (mod)
    {
      mods = ctime (&mod);
      mods [strlen (mods) - 1] = 0; /* lose newline */
    }
  else
    {
      mods = "";
    }

  
#ifdef DOCINFO_VISUAL_TEXT
  {
    extern char * IL_ContextInfo(MWContext *cx);

    Visual *visual = fe_globalData.default_visual;
    Screen *screen = XtScreen (did->dialog);
    char *vi = fe_VisualDescription (screen, visual);
    char *im = IL_ContextInfo(context);
    strcpy (dpy_text, vi);
    if (visual == DefaultVisualOfScreen (screen))
      {
	if (CONTEXT_DATA (context)->cmap == DefaultColormapOfScreen (screen))
	  strcat (dpy_text, "\nThis is the default visual and color map.");
	else
	  strcat (dpy_text,"\nThis is the default visual with a private map.");
      }
    else
      {
	strcat (dpy_text, "\nThis is a non-default visual.");
      }
    free (vi);
    if (im)
      {
	strcat (dpy_text, "\n");
	strcat (dpy_text, im);
	free (im);
      }
  }
#endif

#ifdef DOCINFO_SOURCE_TEXT
  source_text = (!h ? "" :
		 h->transport_method == SHIST_CAME_FROM_NETWORK
		 ? "from network" :
		 h->transport_method == SHIST_CAME_FROM_DISK_CACHE
		 ? "from disk cache" :
		 h->transport_method == SHIST_CAME_FROM_MEMORY_CACHE
		 ? "from memory cache" : "");
#endif

  XtVaSetValues (did->title_text,
		 XmNcursorPosition, 0, XmNvalue, (h ? h->title : ""), 0);
  XtVaSetValues (did->url_text,
		 XmNcursorPosition, 0, XmNvalue, (h ? h->address : ""), 0);
  XtVaSetValues (did->modified_text, XmNcursorPosition, 0, XmNvalue, mods, 0);
#ifdef DOCINFO_SOURCE_TEXT
  XtVaSetValues (did->source_text, XmNcursorPosition, 0,
		 XmNvalue, source_text, 0);
#endif
#ifdef DOCINFO_VISUAL_TEXT
  XtVaSetValues (did->dpy_text, XmNcursorPosition, 0, XmNvalue, dpy_text, 0);
#endif

#ifdef DOCINFO_CHARSET_TEXT
  XtVaSetValues (did->charset_text,
		 XmNvalue, (charset && *charset ? charset : "default"), 0);
  if (charset) free (charset);
#endif

#ifdef HAVE_SECURITY
  sec = cipher;
#else
  /* This is really stupid, but it's compatible...
     --kipp
     */
  {
    XtResource r;
    r.resource_name = r.resource_class = "noSecurityMessage";
    r.resource_type = XtRString;
    r.resource_size = sizeof (String);
    r.resource_offset = 0;
    r.default_type = XtRImmediate;
    r.default_addr = 0;
    XtGetApplicationResources (did->dialog, &sec, &r, 1, 0, 0);
    if (! sec)
      sec = XP_GetString( XFE_X_RESOURCES_NOT_INSTALLED_CORRECTLY );
  }
#endif

  xmstring = XmStringCreateLtoR (sec, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues (did->key_desc_label, XmNlabelString, xmstring, 0);
  XmStringFree (xmstring);

  XtVaSetValues (did->certificate_text,
		 XmNcursorPosition, 0,
		 XmNvalue, (cert ? cert : ""),
		 0);
  if (cert) free (cert);
  if (cipher) free (cipher);
}

void
fe_DocInfoDialog (MWContext *context)
{
  struct fe_docinfo_data *did;
  fe_make_docinfo_dialog (context);
  fe_UpdateDocInfoDialog (context);
  did = CONTEXT_DATA (context)->did;
  fe_NukeBackingStore (did->dialog);
  XtManageChild (did->dialog);
  XMapRaised (XtDisplay (did->dialog), XtWindow (did->dialog));
}
#else /* else !OLD_DOCINFO_WAY */

void
fe_UpdateDocInfoDialog (MWContext *context)
{
	/* nada for now */
}

#endif /*  OLD_DOCINFO_WAY */


static void
fe_security_dialog_toggle (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  XP_Bool *prefs_toggle = (XP_Bool *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  *prefs_toggle = cb->set;
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (fe_all_MWContexts->context,
               XP_GetString( XFE_ERROR_SAVING_OPTIONS ) );
}

#if 0
static void
fe_movemail_dialog_toggle (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  XP_Bool *prefs_toggle = (XP_Bool *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  *prefs_toggle = cb->set;
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (fe_all_MWContexts->context,
               XP_GetString( XFE_ERROR_SAVING_OPTIONS ) );
}
#endif /* 0 */


/* #ifdef MOZ_MAIL_NEWS */
/*
 * fe_MovemailWarning
 * Stolen from FE_SecurityDialog
 */
extern Boolean
fe_MovemailWarning(MWContext *context)
{
  if ( fe_globalPrefs.movemail_warn == True ) {
    Widget mainw = CONTEXT_WIDGET (context);
    Widget dialog;
    Widget form;
    Widget toggle;
    Arg av [20];
    int ac;
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    XmString ok_label;
    XmString cancel_label;
    XmString selection_label;
    XmString toggle_label;
    struct fe_confirm_data data;
    char buf[256];
    char* from;
    const char* to;
 
    from = fe_mn_getmailbox();
    to   = FE_GetFolderDirectory(context);
 
    /*
     * This dialog should probably be popped up in fe_mn_getmailbox()
     * instead.  Otherwise, we have to check everywhere it's used.
     * (Only two places at this point, though.)
     */
    if ( from == NULL ) {
        FE_Alert(context, XP_GetString(XFE_MAIL_SPOOL_UNKNOWN));
        return False;
    }

    PR_snprintf(buf, sizeof(buf), XP_GetString(XFE_MOVEMAIL_EXPLANATION),
                from, to, from);
 
    ok_label = XmStringCreateLtoR(XP_GetString(XFE_CONTINUE_MOVEMAIL),
                                  XmFONTLIST_DEFAULT_TAG);
    cancel_label = XmStringCreateLtoR(XP_GetString(XFE_CANCEL_MOVEMAIL),
                                      XmFONTLIST_DEFAULT_TAG);
    selection_label = XmStringCreateLtoR(buf,
                                         XmFONTLIST_DEFAULT_TAG);
    toggle_label = XmStringCreateLtoR(XP_GetString(XFE_SHOW_NEXT_TIME),
                                      XmFONTLIST_DEFAULT_TAG);
 
    XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                   XtNdepth, &depth, 0);
    ac = 0;
    XtSetArg (av[ac], XmNvisual, v); ac++;
    XtSetArg (av[ac], XmNdepth, depth); ac++;
    XtSetArg (av[ac], XmNcolormap, cmap); ac++;
    XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
    XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
    XtSetArg (av[ac], XmNdialogStyle,
              XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
    XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
    XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
    XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
    XtSetArg (av[ac], XmNchildPlacement, XmPLACE_BELOW_SELECTION); ac++;
    XtSetArg (av[ac], XmNokLabelString, ok_label); ac++;
    XtSetArg (av[ac], XmNcancelLabelString, cancel_label); ac++;
    XtSetArg (av[ac], XmNselectionLabelString, selection_label); ac++;
    dialog = XmCreatePromptDialog (mainw, "movemailWarnDialog", av, ac);
 
    if ( ok_label ) XmStringFree(ok_label);
    if ( cancel_label ) XmStringFree(cancel_label);
    if ( selection_label ) XmStringFree(selection_label);
 
    XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_SELECTION_LABEL));
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                  XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                  XmDIALOG_HELP_BUTTON));
#endif
 
    ac = 0;
    form = XmCreateForm (dialog, "form", av, ac);
    ac = 0;
    XtSetArg (av[ac], XmNset, fe_globalPrefs.movemail_warn); ac++;
    XtSetArg (av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (av[ac], XmNrightAttachment, XmATTACH_NONE); ac++;
    XtSetArg (av[ac], XmNlabelString, toggle_label); ac++;
    toggle = XmCreateToggleButtonGadget (form, "toggle", av, ac);
    if ( toggle_label ) XmStringFree(toggle_label);
 
    XtAddCallback (toggle, XmNvalueChangedCallback,
               fe_security_dialog_toggle, &fe_globalPrefs.movemail_warn);
    XtManageChild (toggle);
    XtManageChild (form);
 
    fe_NukeBackingStore (dialog);
    XtManageChild (dialog);
 
    XtAddCallback (dialog, XmNokCallback, fe_destroy_ok_cb, &data);
    XtAddCallback (dialog, XmNapplyCallback, fe_destroy_apply_cb, &data);
    XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cancel_cb, &data);
    XtAddCallback (dialog, XmNdestroyCallback, fe_destroy_finish_cb,&data);
 
    data.context = context;
    data.widget = dialog;
    data.answer = Answer_Invalid;
 
    while (data.answer == Answer_Invalid)
      fe_EventLoop ();

    if (data.answer == Answer_Cancel || data.answer == Answer_Destroy)
      return False;
    else if (data.answer == Answer_OK)
      return True;
    else
      abort ();
  }
 
  return True;
}
/* #endif  MOZ_MAIL_NEWS */


extern Bool
FE_SecurityDialog (MWContext *context, int state /* , XP_Bool *prefs_toggle */)
{
  char *name;
  XP_Bool *prefs_toggle = 0;
  Bool cancel_p = True;
  Bool cancel_default_p = False;
  switch (state)
    {
    case SD_INSECURE_POST_FROM_SECURE_DOC:
      name = "insecurePostFromSecureDocDialog";
      prefs_toggle = 0;
      cancel_p = True;
      cancel_default_p = True;
      break;
    case SD_INSECURE_POST_FROM_INSECURE_DOC:
      name = "insecurePostFromInsecureDocDialog";
      prefs_toggle = &fe_globalPrefs.submit_warn;
      cancel_p = True;
      break;
    case SD_ENTERING_SECURE_SPACE:
      name = "enteringSecureDialog";
      prefs_toggle = &fe_globalPrefs.enter_warn;
      cancel_p = False;
      break;
    case SD_LEAVING_SECURE_SPACE:
      name = "leavingSecureDialog";
      prefs_toggle = &fe_globalPrefs.leave_warn;
      cancel_p = True;
      break;
    case SD_INSECURE_DOCS_WITHIN_SECURE_DOCS_NOT_SHOWN:
      name = "mixedSecurityDialog";
      prefs_toggle = &fe_globalPrefs.mixed_warn;
      cancel_p = False;
      break;
    case SD_REDIRECTION_TO_INSECURE_DOC:
      name = "redirectionToInsecureDialog";
      prefs_toggle = 0;
      cancel_p = True;
      break;
    case SD_REDIRECTION_TO_SECURE_SITE:
      name = "redirectionToSecureDialog";
      prefs_toggle = 0;
      cancel_p = True;
      break;
    default:
      abort ();
    }

  if (prefs_toggle && !*prefs_toggle)
    return True;

  {
    Widget mainw = CONTEXT_WIDGET (context);
    Widget dialog;
    Arg av [20];
    int ac;
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		   XtNdepth, &depth, 0);
    ac = 0;
    XtSetArg (av[ac], XmNvisual, v); ac++;
    XtSetArg (av[ac], XmNdepth, depth); ac++;
    XtSetArg (av[ac], XmNcolormap, cmap); ac++;
    XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
    XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
    XtSetArg (av[ac], XmNdialogStyle,
	      XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
    if (cancel_p)
      {
	XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
      }
    else
      {
	XtSetArg (av[ac], XmNdialogType, XmDIALOG_INFORMATION); ac++;
      }
    XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
    XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
    XtSetArg (av[ac], XmNchildPlacement, XmPLACE_BELOW_SELECTION); ac++;
    dialog = XmCreatePromptDialog (mainw, name, av, ac);

    XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_SELECTION_LABEL));
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						  XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
    fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						  XmDIALOG_HELP_BUTTON));
#endif
    if (! cancel_p)
      fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						    XmDIALOG_CANCEL_BUTTON));

    if (prefs_toggle)
      {
	Widget form, toggle;
	ac = 0;
	form = XmCreateForm (dialog, "form", av, ac);
	ac = 0;
	XtSetArg (av[ac], XmNset, *prefs_toggle); ac++;
	XtSetArg (av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg (av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg (av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (av[ac], XmNrightAttachment, XmATTACH_NONE); ac++;
	toggle = XmCreateToggleButtonGadget (form, "toggle", av, ac);
	XtAddCallback (toggle, XmNvalueChangedCallback,
		       fe_security_dialog_toggle, prefs_toggle);
	XtManageChild (toggle);
	XtManageChild (form);
      }

    if (cancel_default_p)
      {
	Widget c = XmSelectionBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON);
	if (!c) abort ();
	XtVaSetValues (dialog, XmNdefaultButton, c, XmNinitialFocus, c, 0);
      }

    fe_NukeBackingStore (dialog);
    XtManageChild (dialog);

    if (cancel_p)
      {
	struct fe_confirm_data data;
	XtAddCallback (dialog, XmNokCallback, fe_destroy_ok_cb, &data);
	XtAddCallback (dialog, XmNapplyCallback, fe_destroy_apply_cb, &data);
	XtAddCallback (dialog, XmNcancelCallback, fe_destroy_cancel_cb, &data);
	XtAddCallback (dialog, XmNdestroyCallback, fe_destroy_finish_cb,&data);

	data.context = context;
	data.widget = dialog;
	data.answer = Answer_Invalid;

	while (data.answer == Answer_Invalid)
	  fe_EventLoop ();

	if (data.answer == Answer_Cancel || data.answer == Answer_Destroy)
	  return False;
	else if (data.answer == Answer_OK)
	  return True;
	else
	  abort ();
      }
    else
      {
	XtAddCallback (dialog, XmNokCallback, fe_destroy_cb, dialog);
	return True;
      }
  }
}

#if 0
/* Temporary stub. */
XP_Bool 
XFE_SelectDialog(MWContext  *pContext,
				 const char *pMessage,
				 const char **pList,
				 int16      *pCount) 
{
    int i;
    char *message = 0;
    for (i = 0; i < *pCount; i++) {
	StrAllocCopy(message, pMessage);
        StrAllocCat(message, " = ");
	StrAllocCat(message, pList[i]);
	if (FE_Confirm(pContext, message)) {
	    /* user selected this one */
	    XP_FREE(message);
	    *pCount = i;
	    return TRUE;
	}
    }

    /* user rejected all */
    XP_FREE(message);
    return FALSE;
}

/*
 * temporary UI until FE implements this function as a single dialog box
 */
XP_Bool
XFE_CheckConfirm(MWContext  *pContext,
		 const char *pConfirmMessage,
		 const char *pCheckMessage,
		 const char *pOKMessage,     /* text on the OK button */
		 const char *pCancelMessage, /* text on the cancel button */
		 XP_Bool    *pChecked)
{
  Bool userHasAccepted = FE_Confirm(pContext, pConfirmMessage);
  *pChecked = FE_Confirm (pContext, pCheckMessage);
  return userHasAccepted;
}
#endif /* 0 */
