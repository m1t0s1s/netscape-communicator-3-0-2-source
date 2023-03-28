/* -*- Mode: C; tab-width: 8 -*-
   commands.c --- menus and toolbar.
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 27-Jun-94.
 */

#include "mozilla.h"
#include "xfe.h"
#include "felocale.h"
#include "xlate.h"
#include "menu.h"
#include "net.h"
#include "msgcom.h"
#include "e_kit.h"
#include <libi18n.h>
#include <Xm/AtomMgr.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/utsname.h>

#include "outline.h"
#include "mailnews.h"
#ifdef EDITOR
#include "xeditor.h"
#endif /*EDITOR*/

#if defined(DEBUG_jwz) && defined(MOCHA)
# include "libmocha.h"
#endif /* DEBUG_jwz && MOCHA */


#include "mozjava.h"

#ifdef X_PLUGINS
#include "np.h"
#endif /* X_PLUGINS */

#include <Xm/CutPaste.h>	/* for PasteQuote clipboard hackery */

#include <Xm/RowColumn.h>
#include <XmL/Grid.h>

/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_SAVE_AS_TYPE_ENCODING;
extern int XFE_SAVE_AS_TYPE;
extern int XFE_SAVE_AS_ENCODING;
extern int XFE_SAVE_AS;
extern int XFE_ERROR_OPENING_FILE;
extern int XFE_ERROR_DELETING_FILE;
extern int XFE_LOG_IN_AS;
extern int XFE_OUT_OF_MEMORY_URL;
extern int XFE_FILE_OPEN;
extern int XFE_PRINTING_OF_FRAMES_IS_CURRENTLY_NOT_SUPPORTED;
extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_UNKNOWN_ESCAPE_CODE;
extern int XFE_COULDNT_FORK;
extern int XFE_EXECVP_FAILED;
extern int XFE_SAVE_FRAME_AS;
extern int XFE_SAVE_AS;
extern int XFE_PRINT_FRAME;
extern int XFE_PRINT;
extern int XFE_DOWNLOAD_FILE;
extern int XFE_COMPOSE_NO_SUBJECT;
extern int XFE_COMPOSE;
extern int XFE_NETSCAPE_UNTITLED;
extern int XFE_NETSCAPE;
extern int XFE_MAIL_FRAME;
extern int XFE_MAIL_DOCUMENT;
extern int XFE_NETSCAPE_MAIL;
extern int XFE_NETSCAPE_NEWS;
extern int XFE_BOOKMARKS;
extern int XFE_ADDRESS_BOOK;
extern int XFE_BACK;
#if 0		/* Disabling back/forward in frame - dp */
extern int XFE_BACK_IN_FRAME;
#endif /* 0 */
extern int XFE_FORWARD;
extern int XFE_FORWARD_IN_FRAME;
extern int XFE_CANNOT_SEE_FILE;
extern int XFE_CANNOT_READ_FILE;
extern int XFE_IS_A_DIRECTORY;
extern int XFE_REFRESH;
extern int XFE_REFRESH_FRAME;

extern char* help_menu_names[];
extern char* directory_menu_names[];

/* Local forward declarations */
static void
fe_mailcompose_cb(Widget widget, XtPointer closure, XtPointer call_data);

/* Externs from dialog.c: */
extern int fe_await_synchronous_url (MWContext *context);

/* Externs from mozilla.c */
extern char * fe_MakeSashGeometry(char *old_geom_str, int pane_config,
			unsigned int w, unsigned int h);

/* Externs from mailnews.c */
extern void fe_msgDoCommand(MWContext *context, MSG_CommandType command);
extern void fe_set_compose_wrap_state(MWContext *context, XP_Bool wrap_p);

static void
fe_text_insert (Widget text, const char *s, int16 charset)
{
  /* Gee, I wonder what other gratuitous junk I need to worry about? */
  Boolean pdel = True;
  XtVaGetValues (text, XmNpendingDelete, &pdel, 0);

  if (XmIsText (text))
    {
      char *s2 = strdup (s); /* This is nuts: it wants to modify the string! */
      char *loc;
      if (pdel) XmTextRemove (text);
      loc = (char *) fe_ConvertToLocaleEncoding (charset,
                                                 (unsigned char *) s2);

#ifdef DEBUG_jwz
        {
          /* On my Linux box, Motif freaks out if you try to insert a
             high-bit character into a text field.  So don't.
           */
          unsigned char *s = (unsigned char *) loc;
          if (s)
            while (*s)
              {
                if (*s & 0x80) *s &= 0x7F;
                s++;
              }
        }
#endif /* DEBUG_jwz */

      XmTextInsert (text, XmTextGetInsertionPosition (text), loc);
      if (loc != s2)
        {
          free (loc);
        }
      free (s2);
    }
  else if (XmIsTextField (text))
    {
      char *s2 = strdup (s); /* This is nuts: it wants to modify the string! */
      char *loc;
      if (pdel) XmTextFieldRemove (text);
      loc = (char *) fe_ConvertToLocaleEncoding (charset,
                                                 (unsigned char *) s2);
      XmTextFieldInsert (text, XmTextFieldGetInsertionPosition (text), loc);
      if (loc != s2)
        {
          free (loc);
        }
      free (s2);
    }
  else
    abort ();
}


/* This is completely disgusting.
   We want certain keys to be equivalent to menu items everywhere EXCEPT
   in text fields, where we would like them to do the obvious thing.
   This pile of shit is what we need to do to implement that!!
 */
static Boolean
fe_hack_self_inserting_accelerator (Widget widget, XtPointer closure,
				    XtPointer call_data)
{
#if 1

  /* But actually we're not using this any more. */
  return False;

#else /* 0 */

  XmPushButtonCallbackStruct *cd = (XmPushButtonCallbackStruct *) call_data;
  MWContext *context = (MWContext *) closure;
  Widget focus = XmGetFocusWidget (CONTEXT_WIDGET (context));
  Modifiers mods;
  KeySym k;
  char *s;

  if (!focus||
      (!XmIsText (focus) && !XmIsTextField (focus)))
    return False;

  if (!cd || !cd->event)	/* can this happen? */
    return False;

  if (cd->event->xany.type != KeyPress)
    return False;

  /* If any bits but control, shift, or lock are on, this can't be a
     self-inserting character. */
  if (cd->event->xkey.state & ~(ControlMask|ShiftMask|LockMask))
    return False;

  k = XtGetActionKeysym (cd->event, &mods);

  if (! k)
    return False;

  s = XKeysymToString (k);
  if (! s)
    return False;

  if (*s < ' ' || *s >= 127)  /* non-printing char */
    return False;

  if (s[1])		/* Not a self-inserting character - string is
			   probably "osfCancel" or some such nonsense. */
    return False;

  fe_text_insert (focus, s, CS_LATIN1);
  return True;

#endif /* 0 */
}


static void
fe_spacebar_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Widget sb = CONTEXT_DATA (context)->vscroll;
  XmScrollBarCallbackStruct cb;
  int pi = 0, v = 0, max = 1, min = 0;

#ifdef EDITOR
  if (context->is_editor) return;
#endif /* EDITOR */
    
  XP_ASSERT(sb);
  if (!sb) return;
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  XtVaGetValues (sb, XmNpageIncrement, &pi, XmNvalue, &v,
		 XmNmaximum, &max, XmNminimum, &min, 0);
  cb.reason = XmCR_PAGE_INCREMENT;
  cb.event = 0;
  cb.pixel = 0;
  cb.value = v + pi;
  if (cb.value > max - pi) cb.value = max - pi;
  if (cb.value < min) cb.value = min;

  if (cb.value == CONTEXT_DATA (context)->document_y
      && (context->type == MWContextMail || context->type == MWContextNews))
    {
      MSG_Command (context, MSG_NextMessage);
    }
  else
    {    
      XtCallCallbacks (sb, XmNvalueChangedCallback, &cb);
    }
}

static void
fe_page_forward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Widget sb = CONTEXT_DATA (context)->vscroll;
  XmScrollBarCallbackStruct cb;
  int pi = 0, v = 0, max = 1, min = 0;

  XP_ASSERT(sb);
  if (!sb) return;
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  XtVaGetValues (sb, XmNpageIncrement, &pi, XmNvalue, &v,
		 XmNmaximum, &max, XmNminimum, &min, 0);
  cb.reason = XmCR_PAGE_INCREMENT;
  cb.event = 0;
  cb.pixel = 0;
  cb.value = v + pi;
  if (cb.value > max - pi) cb.value = max - pi;
  if (cb.value < min) cb.value = min;
  XtCallCallbacks (sb, XmNvalueChangedCallback, &cb);
}


static void
fe_page_backward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Widget sb = CONTEXT_DATA (context)->vscroll;
  XmScrollBarCallbackStruct cb;
  int pi = 0, v = 0, min = 0;

  XP_ASSERT(sb);
  if (!sb) return;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  XtVaGetValues (sb, XmNpageIncrement, &pi, XmNvalue, &v, XmNminimum, &min, 0);
  cb.reason = XmCR_PAGE_INCREMENT;
  cb.event = 0;
  cb.pixel = 0;
  cb.value = v - pi;
  if (cb.value < min) cb.value = min;
  XtCallCallbacks (sb, XmNvalueChangedCallback, &cb);
}


/* DEBUG_jwz */
static void
fe_line_forward_1 (Widget widget, XtPointer closure, XtPointer call_data,
                   int lines)
{
  MWContext *context = (MWContext *) closure;
  Widget sb = CONTEXT_DATA (context)->vscroll;
  XmScrollBarCallbackStruct cb;
  int li = 0, v = 0, max = 1, min = 0;

#ifdef EDITOR
  if (context->is_editor) return;
#endif /* EDITOR */
    
  XP_ASSERT(sb);
  if (!sb) return;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  XtVaGetValues (sb, XmNincrement, &li, XmNvalue, &v,
		 XmNmaximum, &max, XmNminimum, &min, 0);
  cb.reason = XmCR_INCREMENT;
  cb.event = 0;
  cb.pixel = 0;
  cb.value = v + (li * lines);
  if (cb.value > max - li) cb.value = max - li;
  if (cb.value < min) cb.value = min;
  XtCallCallbacks (sb, XmNvalueChangedCallback, &cb);
}

#if 0
static void
fe_line_forward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  fe_line_forward_1 (widget, closure, call_data, 1);
}

static void
fe_line_backward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  fe_line_forward_1 (widget, closure, call_data, -1);
}
#endif


static void
fe_column_forward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Widget sb = CONTEXT_DATA (context)->hscroll;
  XmScrollBarCallbackStruct cb;
  int li = 0, v = 0, min = 0;

  XP_ASSERT(sb);
  if (!sb) return;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  XtVaGetValues (sb, XmNincrement, &li, XmNvalue, &v, XmNminimum, &min, 0);
  cb.reason = XmCR_INCREMENT;
  cb.event = 0;
  cb.pixel = 0;
  cb.value = v - li;
  if (cb.value < min) cb.value = min;
  XtCallCallbacks (sb, XmNvalueChangedCallback, &cb);
}


static void
fe_column_backward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Widget sb = CONTEXT_DATA (context)->hscroll;
  XmScrollBarCallbackStruct cb;
  int li = 0, v = 0, max = 1, min = 0;

  XP_ASSERT(sb);
  if (!sb) return;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  XtVaGetValues (sb, XmNincrement, &li, XmNvalue, &v,
		 XmNmaximum, &max, XmNminimum, &min, 0);
  cb.reason = XmCR_INCREMENT;
  cb.event = 0;
  cb.pixel = 0;
  cb.value = v + li;
  if (cb.value > max - li) cb.value = max - li;
  if (cb.value < min) cb.value = min;
  XtCallCallbacks (sb, XmNvalueChangedCallback, &cb);
}


/* File menu
 */
static void
fe_new_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  URL_Struct *url;
  MWContext *context = (MWContext *) closure;
  LO_Element *e;

  fe_UserActivity (context);

  /* Make sure we're using the top-level context; frame contexts
   * don't have chrome ...
   */
  context = fe_GridToTopContext (context);

  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  e = LO_XYToNearestElement (context,
			     CONTEXT_DATA (context)->document_x,
			     CONTEXT_DATA (context)->document_y);

  if (e)
      SHIST_SetPositionOfCurrentDoc (&context->hist, e->lo_any.ele_id);

  context = fe_MakeWindow (XtParent (CONTEXT_WIDGET (context)), context,
			   0, NULL, MWContextBrowser, FALSE);

  url = SHIST_CreateURLStructFromHistoryEntry (context,
					  SHIST_GetEntry (&context->hist, 0));
  if (!url)
    {
      if (fe_globalPrefs.home_document && *fe_globalPrefs.home_document)
        url = NET_CreateURLStruct (fe_globalPrefs.home_document, FALSE);
    }

  if (url)
    fe_GetURL (context, url, FALSE);
}

static void
fe_open_mail(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)), NULL,
		0, NULL, MWContextMail, FALSE);
}

static void
fe_open_news(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)), NULL,
		0, NULL, MWContextNews, FALSE);
}

static void
fe_open_url_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
#if defined(EDITOR) && defined(EDITOR_UI)
  fe_EditorOpenURLDialog(context);
#else
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_OpenURLCallback (widget, closure, call_data);
#endif
}

static void
fe_upload_file_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmString xm_title;
  char *title = 0;
  char *file, *msg;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  XtVaGetValues(widget, XmNlabelString, &xm_title, 0);
  XmStringGetLtoR (xm_title, XmFONTLIST_DEFAULT_TAG, &title);
  XmStringFree(xm_title);

  file = fe_ReadFileName (context, title, 0, True, 0);
  if (title) XP_FREE(title);

  /* validate filename */
  if (file) {
    if (!fe_isFileExist(file)) {
      msg = PR_smprintf( XP_GetString( XFE_CANNOT_SEE_FILE ), file);
      if (msg) {
	XFE_Alert(context, msg);
	XP_FREE(msg);
      }
      XP_FREE (file);
      file = NULL;
    }
    else if (!fe_isFileReadable(file)) {
      msg = PR_smprintf( XP_GetString( XFE_CANNOT_READ_FILE ) , file);
      if (msg) {
	XFE_Alert(context, msg);
	XP_FREE(msg);
      }
      XP_FREE (file);
      file = NULL;
    }
    else if (fe_isDir(file)) {
      msg = PR_smprintf( XP_GetString( XFE_IS_A_DIRECTORY ), file);
      if (msg) {
	  XFE_Alert(context, msg);
	  if (msg) XP_FREE(msg);
      }
      XP_FREE (file);
      file = NULL;
    }
  }

  if (file)
    {
      History_entry *he = SHIST_GetCurrent (&context->hist);
      URL_Struct *url;

      if (he && he->address && (XP_STRNCMP (he->address, "ftp://", 6) == 0)
	  && he->address [strlen (he->address)-1] == '/')
	{
	  url = NET_CreateURLStruct (he->address, NET_SUPER_RELOAD);
	  if (!url)
	    {
	      XP_FREE (file);
	      return;
	    }
	  url->method = URL_POST_METHOD;
	  url->files_to_post = XP_ALLOC (2);
	  if (!url->files_to_post)
	    {
	      XP_FREE (file);
	      return;
	    }

	  url->files_to_post [0] = XP_STRDUP ((const char *) file);
	  url->files_to_post [1] = 0;

	  fe_GetURL (context, url, FALSE);
	}

      XP_FREE (file);
    }
}

static void
fe_open_file_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char *title = 0;
  char *out;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  title = XP_GetString( XFE_FILE_OPEN );
  out = fe_ReadFileName (context, title, 0, True, 0);
  if (out)
    {
      char new [1024];
      URL_Struct *url_struct;
      PR_snprintf (new, sizeof (new), "file:%.900s", out); 
      free (out);
      url_struct = NET_CreateURLStruct (new, FALSE);
      fe_GetURL (context, url_struct, FALSE);
    }
}

static void
fe_reload_frame_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmPushButtonCallbackStruct *cd = (XmPushButtonCallbackStruct *) call_data;

  context = fe_GetFocusGridOfContext (context);

  if (!context) context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  
  if (cd && cd->event->xkey.state & ShiftMask)
    fe_ReLayout (context, NET_SUPER_RELOAD);
  else
    fe_ReLayout (context, NET_NORMAL_RELOAD);
}

static void
fe_reload_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmPushButtonCallbackStruct *cd = (XmPushButtonCallbackStruct *) call_data;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  
  if (cd && cd->event->xkey.state & ShiftMask)
    fe_ReLayout (context, NET_SUPER_RELOAD);
  else
    fe_ReLayout (context, NET_NORMAL_RELOAD);
}

static void
fe_abort_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_AbortCallback (widget, closure, call_data);
}


static void
fe_refresh_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Widget wid;
  Display *dpy;
  Window win;
  Dimension w = 0, h = 0;
  XGCValues gcv;
  GC gc;

  if (fe_IsGridParent (context)) context = fe_GetFocusGridOfContext (context);
  XP_ASSERT (context);
  if (!context) return;

  wid = CONTEXT_DATA (context)->drawing_area;
  if (wid == NULL) return;

  dpy = XtDisplay (wid);
  win = XtWindow (wid);

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  XtVaGetValues (wid, XmNbackground, &gcv.foreground,
		 XmNwidth, &w, XmNheight, &h, 0);
  gc = XCreateGC (dpy, win, GCForeground, &gcv);
  XFillRectangle (dpy, win, gc, 0, 0, w, h);
  XFreeGC (dpy, gc);

  fe_ClearArea (context, 0, 0, w, h);
  fe_RefreshArea (context,
		  CONTEXT_DATA (context)->document_x,
		  CONTEXT_DATA (context)->document_y, w, h);
}


char **fe_encoding_extensions = 0; /* gag.  used by mkcache.c. */


static struct save_as_data *
make_save_as_data (MWContext *context, Boolean allow_conversion_p,
		   int type, URL_Struct *url, const char *output_file)
{
  const char *file_name;
  struct save_as_data *sad;
  FILE *file;
  char *suggested_name = 0;
  Boolean use_dialog_p = False;
  const char *address = url ? url->address : 0;
  const char *content_type = url ? url->content_type : 0;
  const char *content_encoding = url ? url->content_encoding : 0;
  const char *title;
  char buf [255];

  if (url)
    {
      if (!url->content_name)
	url->content_name = MimeGuessURLContentName(context, url->address);
      if (url->content_name)
	address = url->content_name;
    }

  if (output_file)
    file_name = output_file;
  else
    {
      Boolean really_allow_conversion_p = allow_conversion_p;
      if (content_type &&
	  (!strcasecomp (content_type, UNKNOWN_CONTENT_TYPE) || 
	   !strcasecomp (content_type, INTERNAL_PARSER)))
	content_type = 0;

      if (content_type && !*content_type) content_type = 0;
      if (content_encoding && !*content_encoding) content_encoding = 0;
      if (content_type && content_encoding)
	PR_snprintf (buf, sizeof (buf), XP_GetString(XFE_SAVE_AS_TYPE_ENCODING),
		 content_type, content_encoding);
      else if (content_type)
	PR_snprintf (buf, sizeof (buf),
		XP_GetString(XFE_SAVE_AS_TYPE), content_type);
      else if (content_encoding)
	PR_snprintf (buf, sizeof (buf),
		XP_GetString(XFE_SAVE_AS_ENCODING), content_encoding);
      else
	PR_snprintf (buf, sizeof (buf), XP_GetString(XFE_SAVE_AS));
      title = buf;

      if (fe_encoding_extensions)
	{
	  int L = strlen (address);
	  int i = 0;
	  while (fe_encoding_extensions [i])
	    {
	      int L2 = strlen (fe_encoding_extensions [i]);
	      if (L2 < L &&
		  !strncmp (address + L - L2,
			    fe_encoding_extensions [i],
			    L2))
		{

		  /* The URL ends in ".Z" or ".gz" (or whatever.)
		     Take off that extension, and ask netlib what the type
		     of the resulting file is - if it is text/html or
		     text/plain (meaning it ends in ".html" or ".htm" or
		     ".txt" or god-knows-what-else) then (and only then)
		     strip off the .Z or .gz.  That we need to check the
		     extension like that is a kludge, but since we haven't
		     opened the URL yet, we don't have a content-type to
		     know for sure.  And it's just for the default file name
		     anyway...
		   */
#ifdef NEW_DECODERS
		  NET_cinfo *cinfo;

		  /* If the file ends in .html.Z, the suggested file
		     name is .html, but we will allow it to be saved
		     as .txt (uncompressed.)

		     If the file ends in .ps.Z, the suggested file name
		     is .ps.Z, and we will ignore a selection which says
		     to save it as text. */
		  really_allow_conversion_p = False;

		  suggested_name = strdup (address);
		  suggested_name [L - L2] = 0;
		  /* don't free cinfo. */
		  cinfo = NET_cinfo_find_type (suggested_name);
		  if (cinfo &&
		      cinfo->type &&
		      (!strcasecomp (cinfo->type, TEXT_PLAIN) ||
		       !strcasecomp (cinfo->type, TEXT_HTML) ||
		       !strcasecomp (cinfo->type, TEXT_MDL) ||
		       /* always treat unknown content types as text/plain */
		       !strcasecomp (cinfo->type, UNKNOWN_CONTENT_TYPE)))
		    {
		      /* then that's ok */
		      really_allow_conversion_p = allow_conversion_p;
		      break;
		    }

		  /* otherwise, continue. */
		  free (suggested_name);
		  suggested_name = 0;
#else  /* !NEW_DECODERS */
		  suggested_name = strdup (address);
		  suggested_name [L - L2] = 0;
		  break;
#endif /* !NEW_DECODERS */
		}
	      i++;
	    }
	}

      file_name = fe_ReadFileName (context, title,
				   (suggested_name ? suggested_name : address),
				   False,
				   (really_allow_conversion_p ? &type : 0));

      if (suggested_name)
	free ((char *) suggested_name);

      use_dialog_p = True;
    }

  if (! file_name)
    return 0;

  /* If the file exists, confirm overwriting it. */
  {
    XP_StatStruct st;
    if (!stat (file_name, &st))
      {
	char *bp;
	int size;
	size = XP_STRLEN (file_name) + 1;
	size += XP_STRLEN (fe_globalData.overwrite_file_message) + 1;
	bp = (char *) XP_ALLOC (size);
	if (!bp) return 0;
	PR_snprintf (bp, size,
			fe_globalData.overwrite_file_message, file_name);
	if (!FE_Confirm (context, bp))
	  {
	    XP_FREE (bp);
	    return 0;
	  }
	XP_FREE (bp);
      }
  }

  file = fopen (file_name, "w");
  if (! file)
    {
      char buf [1024];
      PR_snprintf (buf, sizeof (buf),
		    XP_GetString(XFE_ERROR_OPENING_FILE), file_name);
      fe_perror (context, buf);
      sad = 0;
    }
  else
    {
      sad = (struct save_as_data *) malloc (sizeof (struct save_as_data));
      sad->context = context;
      sad->name = (char *) file_name;
      sad->file = file;
      sad->type = type;
      sad->done = NULL;
      sad->insert_base_tag = FALSE;
      sad->use_dialog_p = use_dialog_p;
      sad->content_length = 0;
      sad->bytes_read = 0;
    }
  return sad;
}

static void
fe_save_as_complete (PrintSetup *ps)
{
  MWContext *context = (MWContext *) ps->carg;

  fclose(ps->out);

  fe_LowerSynchronousURLDialog (context);

  free (ps->filename);
  NET_FreeURLStruct (ps->url);
}

static void
fe_save_as_nastiness (MWContext *context, URL_Struct *url,
		      struct save_as_data *sad)
{
  assert (sad);
  if (! sad) return;

  /* make damn sure the form_data slot is zero'd or else all
   * hell will break loose
   */
  XP_MEMSET (&url->savedData, 0, sizeof (SHIST_SavedData));

  switch (sad->type)
    {
    case fe_FILE_TYPE_TEXT:
    case fe_FILE_TYPE_FORMATTED_TEXT:
      {
	PrintSetup p;
	fe_RaiseSynchronousURLDialog (context, CONTEXT_WIDGET (context),
				      "saving");
	XL_InitializeTextSetup (&p);
	p.out = sad->file;
	p.filename = sad->name;
	p.url = url;
	free(sad);
	p.completion = fe_save_as_complete;
	p.carg = context;
	XL_TranslateText (context, url, &p);
	fe_await_synchronous_url (context);
	break;
      }
    case fe_FILE_TYPE_PS:
      {
	PrintSetup p;
	fe_RaiseSynchronousURLDialog (context, CONTEXT_WIDGET (context),
				      "saving");
	XL_InitializePrintSetup (&p);
	p.out = sad->file;
	p.filename = sad->name;
	p.url = url;
	free (sad);
	p.completion = fe_save_as_complete;
	p.carg = context;

#ifdef DEBUG_jwz
        p.bigger = fe_globalPrefs.print_font_size;
        p.header = (fe_globalPrefs.print_header &&
                    *fe_globalPrefs.print_header
                    ? strdup (fe_globalPrefs.print_header)
                    : 0);
        p.footer = (fe_globalPrefs.print_footer &&
                    *fe_globalPrefs.print_footer
                    ? strdup (fe_globalPrefs.print_footer)
                    : 0);
        p.right = p.left = fe_globalPrefs.print_hmargin;
        p.bottom = p.top = fe_globalPrefs.print_vmargin;
#endif /* DEBUG_jwz */

	XL_TranslatePostscript (context, url, &p);
	fe_await_synchronous_url (context);
	break;
      }

    case fe_FILE_TYPE_HTML:
      {
	fe_GetSecondaryURL (context, url, FO_CACHE_AND_SAVE_AS, sad, FALSE);
	break;
      }
#ifdef SAVE_ALL
    case fe_FILE_TYPE_HTML_AND_IMAGES:
      {
	char *addr = strdup (url->address);
	char *filename = sad->name;
	FILE *file = sad->file;
	NET_FreeURLStruct (url);
	free (sad);
	SAVE_SaveTree (context, addr, filename, file);
	break;
      }
#endif /* SAVE_ALL */
    default:
      abort ();
    }
}


void
fe_SaveURL (MWContext *context, URL_Struct *url)
{
  struct save_as_data *sad;
  Boolean text_p = True;

  assert (url);
  if (!url) return;

#if 0
  if (url->content_type && *url->content_type)
    {
      if (! (!strcasecomp (url->content_type, TEXT_HTML) ||
	     !strcasecomp (url->content_type, TEXT_MDL) ||
	     !strcasecomp (url->content_type, TEXT_PLAIN)))
	text_p = False;
    }
#endif

  sad = make_save_as_data (context, text_p, fe_FILE_TYPE_HTML, url, 0);
  if (sad)
    fe_save_as_nastiness (context, url, sad);
  else
    NET_FreeURLStruct (url);
}

#ifdef EDITOR

Boolean
fe_SaveAsDialog(MWContext* context, char* buf, int type)
{
    URL_Struct*          url;
    struct save_as_data* sad;

    url = SHIST_CreateURLStructFromHistoryEntry(
					context,
					SHIST_GetCurrent(&context->hist)
					);

    if (url) {

	sad = make_save_as_data(context, FALSE, type, url, 0);

	if (sad) {
	
	    fclose(sad->file);

	    strcpy(buf, sad->name);

	    free(sad);

	    return TRUE;
	}
    }

    return FALSE;
}

/*
 *    Hack, hack, hack. These are in editor.c, please move the menu
 *    definition there.
 */
extern void fe_editor_view_source_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_edit_source_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_browse_doc_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_about_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_open_file_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_edit_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_paragraph_style_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_paragraph_style_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_new_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_new_from_wizard_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_new_from_template_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_open_file_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_save_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_save_as_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_edit_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_undo_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_redo_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_view_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_properties_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_hrule_properties_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_display_tables_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_paragraph_marks_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_link_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_target_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_image_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_hrule_dialog_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_html_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_line_break_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_non_breaking_space_cb(Widget,XtPointer,XtPointer);
extern void fe_editor_char_props_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_toggle_char_props_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_char_props_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_clear_char_props_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_font_size_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_font_size_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_paragraph_props_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_paragraph_props_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_indent_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_properties_dialog_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_delete_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_refresh_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_insert_hrule_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_cut_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_copy_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_paste_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_insert_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_row_insert_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_column_insert_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_cell_insert_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_delete_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_row_delete_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_column_delete_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_table_cell_delete_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_file_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_document_properties_dialog_cb(Widget, XtPointer,
						    XtPointer);
extern void fe_editor_preferences_dialog_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_target_properties_dialog_cb(Widget, XtPointer,
						    XtPointer);
extern void fe_editor_html_properties_dialog_cb(Widget, XtPointer,
						XtPointer);
extern void fe_editor_table_properties_dialog_cb(Widget, XtPointer,
						 XtPointer);
extern void fe_editor_publish_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_find_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_find_again_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_windows_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_select_all_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_delete_item_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_select_table_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_reload_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_set_colors_dialog_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_default_color_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_remove_links_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_open_url_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_show_paragraph_toolbar_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_show_character_toolbar_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_options_menu_cb(Widget, XtPointer, XtPointer);
extern void fe_editor_browse_publish_location_cb(Widget, XtPointer, XtPointer);

#endif /* EDITOR */

static void
fe_save_as_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  {
    MWContext *ctx = fe_GetFocusGridOfContext (context);
    if (ctx)
      context = ctx;
  }

  url =	SHIST_CreateWysiwygURLStruct (context,
                                      SHIST_GetCurrent (&context->hist));

  if (url)
    fe_SaveURL (context, url);
  else
    FE_Alert (context, fe_globalData.no_url_loaded_message);
}

static int
fe_save_as_stream_write_method (void *closure, const char *str, int32 len)
{
  struct save_as_data *sad = (struct save_as_data *) closure;
  fwrite ((char *) str, 1, len, sad->file);

  sad->bytes_read += len;

  if (sad->content_length > 0)
    FE_SetProgressBarPercent (sad->context,
			      (sad->bytes_read * 100) /
			      sad->content_length);
#if 0
  /* Oops, this makes the size twice as large as it should be. */
  FE_GraphProgress (sad->context, 0 /* #### url_s */,
		    sad->bytes_read, len, sad->content_length);
#endif
  return 1;
}

static unsigned int
fe_save_as_stream_write_ready_method (void *closure)
{
  return(MAX_WRITE_READY);
}

static void
fe_save_as_stream_complete_method (void *closure)
{
  struct save_as_data *sad = (struct save_as_data *) closure;
  fclose (sad->file);
  if (sad->done)
    (*sad->done)(sad);
  sad->file = 0;
  if (sad->name)
    {
      free (sad->name);
      sad->name = 0;
    }
  if (sad->use_dialog_p)
    fe_LowerSynchronousURLDialog (sad->context);

  FE_GraphProgressDestroy (sad->context, 0 /* #### url */,
			   sad->content_length, sad->bytes_read);

  free (sad);
  return;
}

static void
fe_save_as_stream_abort_method (void *closure, int status)
{
  struct save_as_data *sad = (struct save_as_data *) closure;
  fclose (sad->file);
  sad->file = 0;
  if (sad->name)
    {
      if (!unlink (sad->name))
	{
#if 0
	  char buf [1024];
	  PR_snprintf (buf, sizeof (buf),
			XP_GetString(XFE_ERROR_DELETING_FILE), sad->name);
	  fe_perror (sad->context, buf);
#endif
	}
      free (sad->name);
      sad->name = 0;
    }
  if (sad->use_dialog_p)
    fe_LowerSynchronousURLDialog (sad->context);

  FE_GraphProgressDestroy (sad->context, 0 /* #### url */,
			   sad->content_length, sad->bytes_read);

  free (sad);
}

/* Creates and returns a stream object which writes the data read to a
   file.  If the file has not been prompted for / opened, it prompts the
   user.
 */
NET_StreamClass *
fe_MakeSaveAsStream (int format_out, void *data_obj,
		     URL_Struct *url_struct, MWContext *context)
{
  struct save_as_data *sad;
  NET_StreamClass* stream;

  if (url_struct->fe_data)
    {
      sad = url_struct->fe_data;
    }
  else
    {
      Boolean text_p = (url_struct->content_type &&
			(!strcasecomp (url_struct->content_type, TEXT_HTML) ||
			 !strcasecomp (url_struct->content_type, TEXT_MDL) ||
			 !strcasecomp (url_struct->content_type, TEXT_PLAIN)));
      sad = make_save_as_data (context, text_p, fe_FILE_TYPE_HTML, url_struct,
			       0);
      if (! sad) return 0;
    }

  url_struct->fe_data = 0;

  stream = (NET_StreamClass *) calloc (sizeof (NET_StreamClass), 1);
  if (!stream) return 0;

  stream->name           = "SaveAs";
  stream->complete       = fe_save_as_stream_complete_method;
  stream->abort          = fe_save_as_stream_abort_method;
  stream->put_block      = fe_save_as_stream_write_method;
  stream->is_write_ready = fe_save_as_stream_write_ready_method;
  stream->data_object    = sad;
  stream->window_id      = context;

  if (sad->insert_base_tag && XP_STRCMP(url_struct->content_type, TEXT_HTML)
      == 0)
    {
      /*
      ** This is here to that any relative URL's in the document
      ** will continue to work even though we are moving the document
      ** to another world.
      */
      fe_save_as_stream_write_method(sad, "<BASE HREF=", 11);
      fe_save_as_stream_write_method(sad, url_struct->address,
				     XP_STRLEN(url_struct->address));
      fe_save_as_stream_write_method(sad, ">\n", 2);
    }

  sad->content_length = url_struct->content_length;
  FE_SetProgressBarPercent (context, -1);
#if 0
  /* Oops, this makes the size twice as large as it should be. */
  FE_GraphProgressInit (context, url_struct, sad->content_length);
#endif

  if (sad->use_dialog_p)
    {
      /* make sure it is safe to open a new context */
      if (NET_IsSafeForNewContext(url_struct))
	{
	  MWContext *new_context;

	  /* create a new context here */
	  new_context = fe_MakeWindow(XtParent(CONTEXT_WIDGET (context)),
					context, url_struct, NULL,
					MWContextSaveToDisk, TRUE);
	  /* Set the values for location and filename */
	  if (url_struct->address) {
	    XmTextFieldSetString(CONTEXT_DATA(new_context)->url_text,
					url_struct->address);
	  }
	  if (sad && sad->name) {
	    XmTextFieldSetString(CONTEXT_DATA(new_context)->url_label,
					sad->name);
	  }

	  /* Set the url_count and enable all animation */
	  CONTEXT_DATA (new_context)->active_url_count = 1;
          fe_StartProgressGraph (new_context);
	  
	  /* register this new context associated with this stream
	   * with the netlib
	   */
	  NET_SetNewContext(url_struct, new_context, fe_url_exit);

	  /* Change the fe_data's context */
	  sad->context = new_context;
	}
      else
        fe_RaiseSynchronousURLDialog (context, CONTEXT_WIDGET (context),
					"saving");
   }

  return stream;
}

NET_StreamClass *
fe_MakeSaveAsStreamNoPrompt (int format_out, void *data_obj,
			     URL_Struct *url_struct, MWContext *context)
{
  struct save_as_data *sad;

  assert (context->prSetup);
  if (! context->prSetup)
    return 0;

  sad = (struct save_as_data *) malloc (sizeof (struct save_as_data));
  sad->context = context;
  sad->name = strdup (context->prSetup->filename);
  sad->file = context->prSetup->out;
  sad->type = fe_FILE_TYPE_HTML;
  sad->done = 0;
  sad->insert_base_tag = FALSE;
  sad->use_dialog_p = FALSE;

  url_struct->fe_data = sad;

  sad->content_length = url_struct->content_length;
  FE_GraphProgressInit (context, url_struct, sad->content_length);

  return fe_MakeSaveAsStream (format_out, data_obj, url_struct, context);
}


/* Viewing source
 */

struct view_source_data
{
  MWContext *context;
  Widget widget;
};

#if 0
static struct view_source_data *
make_view_source_data (MWContext *context, const char *title, const char *url)
{
  char buf [300];
  struct view_source_data *vsd;
  vsd = (struct view_source_data *) malloc (sizeof (struct view_source_data));
  vsd->context = context;
  if (title)
    PR_snprintf (buf, sizeof (buf), "%.290s", title);
  else
    *buf = 0;
  if (strlen (buf) > 4 && !strcmp ("...", buf + strlen(buf) - 3))
    buf [strlen(buf)-3] = 0;
  vsd->widget = fe_ViewSourceDialog (context, buf, url);
  return vsd;
}
#endif

static void
fe_view_source_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
#if 0
  struct view_source_data *vsd;
#endif
  History_entry *h;
  URL_Struct *url;
  char *new_url_add=0;

#if 0
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
#endif

  h = SHIST_GetCurrent (&context->hist);
  url = SHIST_CreateURLStructFromHistoryEntry(context, h);
  if (! url)
    {
      FE_Alert (context, fe_globalData.no_url_loaded_message);
      return;
    }

#if 0
  vsd = make_view_source_data (context, h->title, h->address);

  fe_GetSecondaryURL (context, url, FO_CACHE_AND_VIEW_SOURCE, vsd, FALSE);
#endif

  /* check to make sure it doesn't already have a view-source
   * prefix.  If it does then this window is already viewing
   * the source of another window.  In this case just
   * show the same thing by reloading the url...
   */
  if(strncmp(VIEW_SOURCE_URL_PREFIX, 
			 url->address, 
			 sizeof(VIEW_SOURCE_URL_PREFIX)-1))
    {
      /* prepend VIEW_SOURCE_URL_PREFIX to the url to
       * get the netlib to display the source view
       */
      StrAllocCopy(new_url_add, VIEW_SOURCE_URL_PREFIX);
      StrAllocCat(new_url_add, url->address);
      free(url->address);
      url->address = new_url_add;
    }

   /* make damn sure the form_data slot is zero'd or else all
    * hell will break loose
    */
   XP_MEMSET (&url->savedData, 0, sizeof (SHIST_SavedData));

#ifdef EDITOR
   if (EDT_IS_EDITOR(context) && !FE_CheckAndSaveDocument(context))
     return;
#endif

/*
  fe_GetSecondaryURL (context, url, FO_PRESENT, NULL, FALSE);
*/
  fe_GetURL (context, url, FALSE);

}

static void
fe_frame_source_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
#if 0
  struct view_source_data *vsd;
#endif

  /* Use the frame with focus if there is one */
  if (fe_IsGridParent (context))
    {
      MWContext *ctx = fe_GetFocusGridOfContext (context);
      if (ctx)
        context = ctx;
    }
  /* now we can use the standard view source routine */
  fe_view_source_cb (widget, (XtPointer) context, call_data);
}


static int
fe_view_source_stream_write_method (void *closure, const char *str, int32 len)
{
  struct view_source_data *vsd = (struct view_source_data *) closure;
  char buf [1024];
  XmTextPosition pos, cpos;

  if (!vsd || !vsd->widget) return -1;
  pos = XmTextGetLastPosition (vsd->widget);
  cpos = 0;
  XtVaGetValues (vsd->widget, XmNcursorPosition, &cpos, 0);

  /* We copy the data first because XmTextInsert() needs a null-terminated
     string, and there isn't necessarily room on the end of `str' for us
     to plop down a null. */
  while (len > 0)
    {
      int i;
      int L = (len > (sizeof(buf)-1) ? (sizeof(buf)-1) : len);
      memcpy (buf, str, L);
      buf [L] = 0;
      str += L;
      len -= L;
      /* Crockishly translate CR to LF for the Motif text widget... */
      for (i = 0; i < L; i++)
	if (buf[i] == '\r' && buf[i+1] != '\n')
	  buf[i] = '\n';
      XmTextInsert (vsd->widget, pos, buf);
      pos += L;
    }
  XtVaSetValues (vsd->widget, XmNcursorPosition, cpos, 0);
  return 1;
}

static void
fe_view_source_stream_complete_method (void *closure)
{
  struct view_source_data *vsd = (struct view_source_data *) closure;
  free (vsd);
}

static void
fe_view_source_stream_abort_method (void *closure, int status)
{
  struct view_source_data *vsd = (struct view_source_data *) closure;
  free (vsd);
}


/* Creates and returns a stream object which writes the data read into a
   text widget.
 */
NET_StreamClass *
fe_MakeViewSourceStream (int format_out, void *data_obj,
			 URL_Struct *url_struct, MWContext *context)
{
  struct view_source_data *vsd;
  NET_StreamClass* stream;

  if (url_struct->is_binary)
    {
      FE_Alert (context, fe_globalData.binary_document_message);
      return 0;
    }

  vsd = url_struct->fe_data;
  if (! vsd) abort ();
  url_struct->fe_data = 0;

  stream = (NET_StreamClass *) calloc (sizeof (NET_StreamClass), 1);
  if (!stream) return 0;

  stream->name           = "ViewSource";
  stream->complete       = fe_view_source_stream_complete_method;
  stream->abort          = fe_view_source_stream_abort_method;
  stream->put_block      = fe_view_source_stream_write_method;
  stream->is_write_ready = fe_save_as_stream_write_ready_method;
  stream->data_object    = vsd;
  stream->window_id      = context;
  return stream;
}


/* Mailing documents
 */

static void
fe_mailNew_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  MSG_Mail (context);
}


static void
fe_mailto_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

#ifdef EDITOR
  if (EDT_IS_EDITOR(context) && !FE_CheckAndSaveDocument(context))
    return;
#endif

  /*
   * You cannot mail a frameset, you must mail the frame child
   * with focus
   */
  if (fe_IsGridParent (context))
    {
      MWContext *ctx = fe_GetFocusGridOfContext (context);
      if (ctx)
        context = ctx;
    }
  MSG_MailDocument (context);
}


extern void fe_mailto_attach_dialog(MWContext* context);

static void
fe_mailto_attach_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_mailto_attach_dialog(context);
}

static void
fe_print_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

#ifdef EDITOR
  if (EDT_IS_EDITOR(context) && !FE_CheckAndSaveDocument(context))
      return;
#endif

  {
    MWContext *ctx = fe_GetFocusGridOfContext (context);
    if (ctx)
      context = ctx;
  }

  url = SHIST_CreateURLStructFromHistoryEntry (context, SHIST_GetCurrent(&context->hist));
  if (url) {
    /* Free the url struct we created */
    NET_FreeURLStruct(url);

#ifdef X_PLUGINS
    if (CONTEXT_DATA(context)->is_fullpage_plugin) {
      /* Full page plugins need a step here. We need to ask the plugin if
       * it wants to handle the printing.
       */
      NPPrint npprint;
      
      npprint.mode = NP_FULL;
      npprint.print.fullPrint.pluginPrinted = FALSE;
      npprint.print.fullPrint.printOne = TRUE;
      npprint.print.fullPrint.platformPrint = NULL;
      NPL_Print(context->pluginList, &npprint);
      if (npprint.print.fullPrint.pluginPrinted == TRUE)
	return;
    }
#endif /* X_PLUGINS */

    fe_PrintDialog (context);
  }
  else
    FE_Alert(context, fe_globalData.no_url_loaded_message);
}

static void
fe_delete_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  if (fe_WindowCount == 1)
    {
#if 1
      XBell (XtDisplay (widget), 0);
#else
      fe_QuitCallback (widget, closure, call_data);
#endif
    }
  else
    {
      XtDestroyWidget (CONTEXT_WIDGET (context));
    }
}

#define fe_quit_cb fe_QuitCallback
void
fe_QuitCallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  if (!fe_CheckUnsentMail ())
    return;
  if (!fe_CheckDeferredMail ())
    return;
#ifdef EDITOR
  if (!fe_EditorCheckUnsaved(context))
      return;
#endif /*EDITOR*/
  if (!CONTEXT_DATA (context)->confirm_exit_p ||
      FE_Confirm (context, fe_globalData.really_quit_message))
    {
      fe_AbortCallback (widget, closure, call_data);
      fe_Exit (0);
    }
}



/* Selections
 */

static Boolean
fe_selection_converter (Widget widget, Atom *selection, Atom *target,
			Atom *type_ret, XtPointer *value_ret,
			unsigned long *length_ret, int *format_ret)
{
  Display *dpy = XtDisplay (widget);
  XTextProperty tmp_prop;
  int status;
  Atom COMPOUND_TEXT = XmInternAtom(dpy, "COMPOUND_TEXT", False);
  MWContext *context = fe_WidgetToMWContext (widget);
  Atom XA_TARGETS   = XmInternAtom (dpy, "TARGETS", False);
  Atom XA_TIMESTAMP = XmInternAtom (dpy, "TIMESTAMP", False);
  Atom XA_TEXT      = XmInternAtom (dpy, "TEXT", False);
  Atom XA_LENGTH    = XmInternAtom (dpy, "LENGTH", False);
  Atom XA_FILE_NAME = XmInternAtom (dpy, "FILE_NAME", False);
  Atom XA_OWNER_OS  = XmInternAtom (dpy, "OWNER_OS", False);
  Atom XA_HOST_NAME = XmInternAtom (dpy, "HOST_NAME", False);
  Atom XA_USER      = XmInternAtom (dpy, "USER", False);
  Atom XA_CLASS     = XmInternAtom (dpy, "CLASS", False);
  Atom XA_NAME      = XmInternAtom (dpy, "NAME", False);
  Atom XA_CLIENT_WINDOW = XmInternAtom (dpy, "CLIENT_WINDOW", False);
  Atom XA_PROCESS   = XmInternAtom (dpy, "PROCESS", False);
#if 0
  Atom XA_CHARACTER_POSITION = XmInternAtom (dpy, "CHARACTER_POSITION", False);
#endif
  char *data = 0;
  Time time = 0;
  char *loc;

  XP_ASSERT (context);
  if (! context)
    {
      return False;
    }
  else if (*selection == XA_PRIMARY)
    {
      data = CONTEXT_DATA (context)->selection;
      time = CONTEXT_DATA (context)->selection_time;
    }
  else if (*selection == XmInternAtom (dpy, "CLIPBOARD", False))
    {
      data = CONTEXT_DATA (context)->clipboard;
      time = CONTEXT_DATA (context)->clipboard_time;
    }

  if (! data)
    {
      return False;
    }

  /* I can't fucking believe the contortions we need to go through here!! */

  if (*target == XA_TARGETS)
    {
      Atom *av = (Atom *) malloc (sizeof (Atom) * 20);
      int ac = 0;
      av [ac++] = XA_TARGETS;
      av [ac++] = XA_TIMESTAMP;
      av [ac++] = XA_TEXT;
      av [ac++] = XA_STRING;
      av [ac++] = XA_LENGTH;
#if 0
      av [ac++] = XA_CHARACTER_POSITION;
#endif
      av [ac++] = XA_FILE_NAME;
      av [ac++] = XA_OWNER_OS;
      av [ac++] = XA_HOST_NAME;
      av [ac++] = XA_USER;
      av [ac++] = XA_CLASS;
      av [ac++] = XA_NAME;
      av [ac++] = XA_CLIENT_WINDOW;
      av [ac++] = XA_PROCESS;
      av [ac++] = COMPOUND_TEXT;

      /* Other types that might be interesting (from the R6 ICCCM):

	 XA_MULTIPLE		(this is a pain in the ass and nobody uses it)
	 XA_LINE_NUMBER		start and end lines of the selection
	 XA_COLUMN_NUMBER	start and end column of the selection
	 XA_BACKGROUND		a list of Pixel values
	 XA_FOREGROUND		a list of Pixel values
	 XA_PIXMAP		a list of pixmap IDs (of the images?)
	 XA_DRAWABLE		a list of drawable IDs (?)

	 XA_COMPOUND_TEXT	for text

	 Hairy Image Conversions:
	 XA_APPLE_PICT		for images
	 XA_POSTSCRIPT
	 XA_ENCAPSULATED_POSTSCRIPT aka _ADOBE_EPS
	 XA_ENCAPSULATED_POSTSCRIPT_INTERCHANGE aka _ADOBE_EPSI
	 XA_ODIF		ISO Office Document Interchange Format
       */
      *value_ret = av;
      *length_ret = ac;
      *type_ret = XA_ATOM;
      *format_ret = 32;
      return True;
   }
  else if (*target == XA_TIMESTAMP)
    {
      Time *timestamp;
      timestamp = (Time *) malloc (sizeof (Time));
      *timestamp = time;
      *value_ret = (char *) timestamp;
      *length_ret = 1;
      *type_ret = XA_TIMESTAMP;
      *format_ret = 32;
      return True;
    }
  else if (((*target == XA_TEXT) && (context->win_csid == CS_LATIN1)) ||
           (*target == XA_STRING))
    {
      *value_ret = strdup (data);
      *length_ret = strlen (data);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_TEXT)
    {
      loc = (char *) fe_ConvertToLocaleEncoding (context->win_csid,
                                                 (unsigned char *) data);
      status = XmbTextListToTextProperty (dpy, &loc, 1, XStdICCTextStyle,
                                          &tmp_prop);
      if (loc != data)
        {
          XP_FREE (loc);
        }
      if (status == Success)
        {
          *value_ret = (XtPointer) tmp_prop.value;
          *length_ret = tmp_prop.nitems;
          *type_ret = tmp_prop.encoding;    /* STRING or COMPOUND_TEXT */
          *format_ret = tmp_prop.format;
        }
      else
        {
          *value_ret = NULL;
          *length_ret = 0;
          return False;
        }
      return True;
    }
  else if (*target == COMPOUND_TEXT)
    {
      loc = (char *) fe_ConvertToLocaleEncoding (context->win_csid,
                                                 (unsigned char *) data);
      status = XmbTextListToTextProperty (dpy, &loc, 1, XCompoundTextStyle,
                                          &tmp_prop);
      if (loc != data)
        {
          XP_FREE (loc);
        }
      if (status == Success)
        {
          *value_ret = (XtPointer) tmp_prop.value;
          *length_ret = tmp_prop.nitems;
          *type_ret = COMPOUND_TEXT;
          *format_ret = 8;
        }
      else
        {
          *value_ret = NULL;
          *length_ret = 0;
          return False;
        }
      return True;
    }
  else if (*target == XA_LENGTH)
    {
      int *len = (int *) malloc (sizeof (int));
      *len = strlen (data);
      *value_ret = len;
      *length_ret = 1;
      *type_ret = XA_INTEGER;
      *format_ret = 32;
      return True;
    }
#if 0
  else if (*target == XA_CHARACTER_POSITION)
    {
      int32 *ends = (int32 *) malloc (sizeof (int32) * 2);
      LO_Element *s, *e;
      /* oops, this doesn't actually give me character positions -
	 just LO_Elements and indexes into them. */
      LO_GetSelectionEndpoints (context, &s, &e, &ends[0], &ends[1]);
      *value_ret = ends;
      *length_ret = 2;
      *type_ret = XA_INTEGER;
      *format_ret = 32;
      return True;
    }
#endif
  else if (*target == XA_FILE_NAME)
    {
      History_entry *current = SHIST_GetCurrent (&context->hist);
      if (!current || !current->address)
	return False;
      *value_ret = strdup (current->address);
      *length_ret = strlen (current->address);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_OWNER_OS)
    {
      char *os;
      struct utsname uts;
      if (uname (&uts) < 0)
	os = "uname failed";
      else
	os = uts.sysname;
      *value_ret = strdup (os);
      *length_ret = strlen (os);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_HOST_NAME)
    {
      char name [255];
      if (gethostname (name, sizeof (name)))
	return False;
      *value_ret = strdup (name);
      *length_ret = strlen (name);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_USER)
    {
      struct passwd *pw = getpwuid (geteuid ());
      char *real_uid = (pw ? pw->pw_name : "");
      char *user_uid = getenv ("LOGNAME");
      if (! user_uid) user_uid = getenv ("USER");
      if (! user_uid) user_uid = real_uid;
      if (! user_uid)
	return False;
      *value_ret = strdup (user_uid);
      *length_ret = strlen (user_uid);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_CLASS)
    {
      *value_ret = strdup (fe_progclass);
      *length_ret = strlen (fe_progclass);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_NAME)
    {
      *value_ret = strdup (fe_progname);
      *length_ret = strlen (fe_progname);
      *type_ret = XA_STRING;
      *format_ret = 8;
      return True;
    }
  else if (*target == XA_CLIENT_WINDOW)
    {
      Window *window;
      window = (Window *) malloc (sizeof (Window));
      *window = XtWindow (CONTEXT_WIDGET (context));
      *value_ret = window;
      *length_ret = 1;
      *type_ret = XA_WINDOW;
      *format_ret = 32;
      return True;
    }
  else if (*target == XA_PROCESS)
    {
      pid_t *pid;
      pid = (pid_t *) malloc (sizeof (pid_t));
      *pid = getpid ();
      *value_ret = pid;
      *length_ret = 1;
      *type_ret = XA_INTEGER;
      *format_ret = 32;
      return True;
    }
  else if (*target == XA_COLORMAP)
    {
      Colormap *cmap;
      cmap = (Colormap *) malloc (sizeof (Colormap));
      *cmap = fe_cmap(context);
      *value_ret = cmap;
      *length_ret = 1;
      *type_ret = XA_COLORMAP;
      *format_ret = 32;
      return True;
    }

  return False;
}

static void
fe_selection_loser (Widget widget, Atom *selection)
{
  Display *dpy = XtDisplay (widget);
  MWContext *context = fe_WidgetToMWContext (widget);
  if (! context)
    ; /* Are you talking to me? */
  else if (*selection == XA_PRIMARY)
    fe_DisownSelection (context, 0, False);
  else if (*selection == XmInternAtom (dpy, "CLIPBOARD", False))
    fe_DisownSelection (context, 0, True);
}

void
fe_own_selection_1 (MWContext *context, Time time, Boolean clip_p, char *data)
{
  Display *dpy = XtDisplay (CONTEXT_WIDGET(context));
  Atom atom = (clip_p ? XmInternAtom (dpy, "CLIPBOARD", False) : XA_PRIMARY);
  if (clip_p)
    {
      CONTEXT_DATA (context)->clipboard = data;
      CONTEXT_DATA (context)->clipboard_time = time;
    }
  else
    {
      CONTEXT_DATA (context)->selection = data;
      CONTEXT_DATA (context)->selection_time = time;
    }

  if (data)
    XtOwnSelection (CONTEXT_WIDGET (context), atom, time,
		    fe_selection_converter,	/* conversion proc */
		    fe_selection_loser,		/* lost proc */
		    0);				/* ACK proc */
}

void
fe_OwnSelection (MWContext *context, Time time, Boolean clip_p)
{
  char *data = (char *) LO_GetSelectionText (context);
  fe_own_selection_1 (context, time, clip_p, data);
}


void
fe_DisownSelection (MWContext *context, Time time, Boolean clip_p)
{
  Display *dpy = XtDisplay (CONTEXT_WIDGET(context));
  Atom atom = (clip_p ? XmInternAtom (dpy, "CLIPBOARD", False) : XA_PRIMARY);
  XtDisownSelection (CONTEXT_WIDGET (context), atom, time);
  if (clip_p)
    {
      if (CONTEXT_DATA (context)->clipboard)
	free (CONTEXT_DATA (context)->clipboard);
      CONTEXT_DATA (context)->clipboard = 0;
      CONTEXT_DATA (context)->clipboard_time = time;
    }
  else
    {
      if (CONTEXT_DATA (context)->selection)
	free (CONTEXT_DATA (context)->selection);
      CONTEXT_DATA (context)->selection = 0;
      CONTEXT_DATA (context)->selection_time = time;
      LO_ClearSelection (context);
    }
}


/* Returns the current value of the clipboard as a string.
   Largely cribbed from XmTextPaste and XmTextFieldPaste,
   but only works with STRING, not COMPOUND_TEXT...
   #### i18n guys need to fix this...
 */
static char *
fe_get_clipboard (MWContext *context)
{
  Widget widget = CONTEXT_WIDGET(context);
  Display *dpy = XtDisplay(widget);
  Window window = XtWindow(widget);

  Boolean get_ct = False;		     /* COMPOUND_TEXT vs TEXT   */
  int status = 0;			     /* clipboard status	*/
  char *buffer;				     /* temporary text buffer	*/
  unsigned long length;			     /* length of buffer	*/
  unsigned long outlength = 0;		     /* length of bytes copied	*/
  long private_id = 0;			     /* id of item on clipboard */

  status = XmClipboardInquireLength(dpy, window, "STRING", &length);

  if (status == ClipboardNoData || length == 0)
    {
      status = XmClipboardInquireLength(dpy, window, "COMPOUND_TEXT",
					&length);
      if (status == ClipboardNoData ||
	  status == ClipboardLocked ||
	  length == 0)
	return 0;
      get_ct = True;
    }
  else if (status == ClipboardLocked)
    return 0;

  /* malloc length of clipboard data */
  buffer = XP_ALLOC(length);

  if (!get_ct)
    {
      status = XmClipboardRetrieve(dpy, window, "STRING", buffer,
				   length, &outlength, &private_id);
    }
  else
    {
      /* #### i18n: I don't understand how to deal with COMPOUND_TEXT */
      return 0;
    }

  if (status != ClipboardSuccess)
    {
      XmClipboardEndRetrieve(dpy, window);
      XP_FREE(buffer);
      return 0;
    }

  return buffer;
}


/* Motif sucks!  XmTextPaste() gets stuck if you're pasting from one
   widget to another in the same application! */
static void
fe_text_paste (MWContext *context, Widget text, XP_Bool quote_p)
{
  struct fe_MWContext_cons *rest;
  char *clip = 0;
  XP_Bool free_p = FALSE;
  int16 charset = CS_LATIN1;

  for (rest = fe_all_MWContexts; rest; rest = rest->next)
    if ((clip = CONTEXT_DATA (rest->context)->clipboard))
      {
        charset = rest->context->win_csid;
        break;
      }

  if (!clip && quote_p)
    {
      clip = fe_get_clipboard (context);
      free_p = TRUE;
    }

  if (clip && quote_p)
    {
      char *n = MSG_ConvertToQuotation (clip);
      XP_ASSERT(n);
      if (free_p) XP_FREE(clip);
      free_p = TRUE;
      clip = n;
      if (!clip) return;
    }

  if (clip)
    {
      fe_text_insert (text, clip, charset);
      if (free_p) XP_FREE(clip);
    }
  else if (XmIsText (text))
    XmTextPaste (text);
  else if (XmIsTextField (text))
    XmTextFieldPaste (text);
  else
    XBell (XtDisplay (CONTEXT_WIDGET (context)), 0);
}


typedef enum { fe_Cut, fe_Copy, fe_Paste, fe_PasteQuoted } fe_CCP;

static void
fe_ccp (MWContext *context, XEvent *event, fe_CCP ccp)
{
  Widget focus = XmGetFocusWidget (CONTEXT_WIDGET (context));
  Time time = (event && (event->type == KeyPress ||
			 event->type == KeyRelease)
	       ? event->xkey.time :
	       event && (event->type == ButtonPress ||
			 event->type == ButtonRelease)
	       ? event->xbutton.time :
	       XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context))));
  if (focus && CONTEXT_DATA (context)->selection && ccp == fe_Copy)
    {
      fe_OwnSelection (context, time, True);
    }
  else if (focus && XmIsText (focus))
    switch (ccp)
      {
      case fe_Cut:	   XmTextCut (focus, time); break;
      case fe_Copy:	   XmTextCopy (focus, time); break;
      case fe_Paste:	   fe_text_paste (context, focus, FALSE); break;
      case fe_PasteQuoted: fe_text_paste (context, focus, TRUE); break;
      default:		   XP_ASSERT(0); break;
      }
  else if (focus && XmIsTextField (focus))
    switch (ccp)
      {
      case fe_Cut:	   XmTextFieldCut (focus, time); break;
      case fe_Copy:	   XmTextFieldCopy (focus, time); break;
      case fe_Paste:	   fe_text_paste (context, focus, FALSE); break;
      case fe_PasteQuoted: fe_text_paste (context, focus, TRUE); break;
      default:		   XP_ASSERT(0); break;
      }
  else
    {
      XBell (XtDisplay (CONTEXT_WIDGET (context)), 0);
    }

}

static void
fe_cut_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_ccp (context, (cb ? cb->event : 0), fe_Cut);
}

static void
fe_copy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_ccp (context, (cb ? cb->event : 0), fe_Copy);
}

static void
fe_paste_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_ccp (context, (cb ? cb->event : 0), fe_Paste);
}

static void
fe_paste_quoted_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_ccp (context, (cb ? cb->event : 0), fe_PasteQuoted);
}

static void
fe_find_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  MWContext *top_context;

  top_context = fe_GridToTopContext(context);
  if (!top_context) top_context = context;

  fe_UserActivity (top_context);
  if (fe_hack_self_inserting_accelerator (CONTEXT_WIDGET(top_context),
					  closure, call_data))
    return;
  fe_FindDialog (top_context, False);
}


static void
fe_netscape_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    char* netscape_url = xfe_get_netscape_home_page_url();
    MWContext* context = (MWContext*) closure;
 
    fe_UserActivity (context);
 
    fe_GetURL (context, NET_CreateURLStruct(netscape_url, FALSE), FALSE);
}


static void
fe_find_again_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  MWContext *top_context;
  fe_FindData *find_data;

  top_context = fe_GridToTopContext(context);
  if (!top_context) top_context = context;

  fe_UserActivity (top_context);
  if (fe_hack_self_inserting_accelerator (CONTEXT_WIDGET(top_context),
					  closure, call_data))
    return;

  find_data = CONTEXT_DATA(top_context)->find_data;

#ifdef DEBUG_jwz
    fe_FindDialog (top_context, True);
#else /* !DEBUG_jwz */
  if ((top_context->type == MWContextBrowser ||
       top_context->type == MWContextMail ||
       top_context->type == MWContextNews) &&
      find_data && find_data->string && find_data->string[0] != '\0')
    fe_FindDialog (top_context, True);
  else
    XBell (XtDisplay (widget), 0);
#endif /* !DEBUG_jwz */
}


static Boolean
fe_own_selection_p (MWContext *context, Boolean clip_p)
{
  Display *dpy = XtDisplay (CONTEXT_WIDGET (context));
  Atom atom = (clip_p ? XmInternAtom (dpy, "CLIPBOARD", False) : XA_PRIMARY);
  Window window = XGetSelectionOwner (dpy, atom);
  if ((clip_p
       ? CONTEXT_DATA (context)->clipboard
       : CONTEXT_DATA (context)->selection) ||
      (window && (clip_p || XtWindowToWidget (dpy, window))))
    return True;
  else
    return False;
}


/* Options menu
 */
static void
fe_general_prefs_cb (Widget widget,
			   XtPointer closure,
			   XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_GeneralPrefsDialog (context);
}

static void
fe_mailnews_prefs_cb (Widget widget,
			    XtPointer closure,
			    XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_MailNewsPrefsDialog (context);
}

static void
fe_network_prefs_cb (Widget widget,
		       XtPointer closure,
		       XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_NetworkPrefsDialog (context);
}

static void
fe_security_prefs_cb (Widget widget,
		       XtPointer closure,
		       XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_SecurityPrefsDialog (context);
}

static void
fe_save_options_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_ContextData* data = CONTEXT_DATA(context);
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  /* Flush this context's options into the global state, then save that. */
  if (context->type == MWContextBrowser) {
    fe_globalPrefs.show_toolbar_p = data->show_toolbar_p;
    fe_globalPrefs.toolbar_icons_p = data->show_toolbar_icons_p;
    fe_globalPrefs.toolbar_text_p  = data->show_toolbar_text_p;
    fe_globalPrefs.show_url_p = data->show_url_p;
    fe_globalPrefs.show_directory_buttons_p = data->show_directory_buttons_p;
    fe_globalPrefs.show_menubar_p = data->show_menubar_p;
    fe_globalPrefs.show_bottom_status_bar_p = data->show_bottom_status_bar_p;
    fe_globalPrefs.autoload_images_p = data->autoload_images_p;
    fe_globalPrefs.fancy_ftp_p = data->fancy_ftp_p;
    fe_globalPrefs.doc_csid = data->xfe_doc_csid;
#ifdef HAVE_SECURITY
    fe_globalPrefs.show_security_bar_p = data->show_security_bar_p;
#endif
  }
#ifdef EDITOR
  else if (context->type == MWContextEditor) {
    fe_globalPrefs.show_toolbar_p = data->show_toolbar_p;
    fe_globalPrefs.editor_character_toolbar = data->show_character_toolbar_p;
    fe_globalPrefs.editor_paragraph_toolbar = data->show_paragraph_toolbar_p;
    fe_globalPrefs.toolbar_icons_p = data->show_toolbar_icons_p;
    fe_globalPrefs.toolbar_text_p  = data->show_toolbar_text_p;
    fe_globalPrefs.show_menubar_p = data->show_menubar_p;
    fe_globalPrefs.show_bottom_status_bar_p = data->show_bottom_status_bar_p;
    fe_globalPrefs.autoload_images_p = data->autoload_images_p;
  }
#endif /*EDITOR*/
  else if ((context->type == MWContextMail) ||
	   (context->type == MWContextNews)) {
    /* Fix the sash geometry from the current settings */
    Dimension w, h;
    char *sash_geometry, *new_sash_geometry;
    int pane_style;

    if (context->type == MWContextMail) {
	sash_geometry = fe_globalPrefs.mail_sash_geometry;
	pane_style = fe_globalPrefs.mail_pane_style;
    }
    else {
	sash_geometry = fe_globalPrefs.news_sash_geometry;
	pane_style = fe_globalPrefs.news_pane_style;
    }
    
    switch (pane_style) {
	case FE_PANES_NORMAL:
	    XtVaGetValues(data->folderform, XmNwidth, &w, XmNheight, &h, 0);
	    break;
	case FE_PANES_HORIZONTAL:
	    XtVaGetValues(data->folderform, XmNwidth, &w, XmNheight, &h, 0);
	    break;
	case FE_PANES_STACKED:
	    XtVaGetValues(data->folderform, XmNheight, &w, 0);
	    XtVaGetValues(data->messageform, XmNheight, &h, 0);
	    break;
        case FE_PANES_TALL_FOLDERS:
	    XtVaGetValues(data->folderform, XmNwidth, &w, 0);
	    XtVaGetValues(data->messageform, XmNheight, &h, 0);
	    break;
        default:
            abort();
            break;
    }
    new_sash_geometry = fe_MakeSashGeometry(sash_geometry, pane_style, w, h);

    if (sash_geometry) XP_FREE(sash_geometry);

    if (context->type == MWContextMail)
	fe_globalPrefs.mail_sash_geometry = new_sash_geometry;
    else
	fe_globalPrefs.news_sash_geometry = new_sash_geometry;
  }

  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (context, XP_GetString( XFE_ERROR_SAVING_OPTIONS ) );
  else
    fe_Message (context, fe_globalData.options_saved_message);
}

/* Help menu
 */

/* I'm sick of hearing about brain surgeons like poskanzer going in and
   hacking the executable to change these URLs!  Damn it!  Grr! */
#define HTTP_OFF		42

	/* "about:document" */
#define DOCINFO			"about:document"

	/* ".netscape.com/" */
#define DOT_NETSCAPE_DOT_COM_SLASH \
				"\130\230\217\236\235\215\213\232\217\130\215"\
				"\231\227\131"

	/* "http://home.netscape.com/" */
#define HTTP_NETSCAPE		"\222\236\236\232\144\131\131\222\231\227\217"\
				"\130\230\217\236\235\215\213\232\217\130\215"\
				"\231\227\131"

	/* "http://cgi.netscape.com/cgi-bin/" */
#define HTTP_CGI		"\222\236\236\232\144\131\131\215\221\223\130"\
				"\230\217\236\235\215\213\232\217\130\215\231"\
				"\227\131\215\221\223\127\214\223\230\131"

        /* "http://home.netscape.com/home/" */
#define HTTP_HOME               "\222\236\236\232\144\131\131\222\231\227\217"\
                                "\130\230\217\236\235\215\213\232\217\130\215"\
                                "\231\227\131\222\231\227\217\131"

        /* "http://home.netscape.com/eng/mozilla/3.0/" */
#define HTTP_ENG                "\222\236\236\232\144\131\131\222\231\227\217"\
                                "\130\230\217\236\235\215\213\232\217\130\215"\
                                "\231\227\131\217\230\221\131\227\231\244\223"\
                		"\226\226\213\131\135\130\132\131"

        /* "http://home.netscape.com/info/" */
#define HTTP_INFO               "\222\236\236\232\144\131\131\222\231\227\217"\
                                "\130\230\217\236\235\215\213\232\217\130\215"\
                                "\231\227\131\223\230\220\231\131"

        /* "fishcam/" */
#define HTTP_FC                 HTTP_NETSCAPE \
                                "\220\223\235\222\215\213\227\131"

        /* "newsrc:" */
#define HTTP_NEWSRC             "\230\217\241\235\234\215\144"
 
        /* "whats-new.html" */
#define HTTP_WHATS_NEW          HTTP_HOME "\241\222\213\236\235\127\230\217"\
                                "\241\130\222\236\227\226"
 
        /* "whats-cool.html" */
#define HTTP_WHATS_COOL         HTTP_HOME "\241\222\213\236\235\127\215\231"\
                                "\231\226\130\222\236\227\226"
 
        /* "internet-directory.html" */
#define HTTP_INET_DIRECTORY     HTTP_HOME "\223\230\236\217\234\230\217\236"\
                                "\127\216\223\234\217\215\236\231\234\243\130"\
                                "\222\236\227\226"
 
        /* "internet-search.html" */
#define HTTP_INET_SEARCH        HTTP_HOME "\223\230\236\217\234\230\217\236"\
                                "\127\235\217\213\234\215\222\130\222\236\227"\
                                "\226"
        /* "internet-white-pages.html" */
#define HTTP_INET_WHITE         HTTP_HOME "\223\230\236\217\234\230\217\236"\
                                "\127\241\222\223\236\217\127\232\213\221\217"\
                                "\235\130\222\236\227\226"
 
        /* "about-the-internet.html" */
#define HTTP_INET_ABOUT         HTTP_HOME "\213\214\231\237\236\127\236\222"\
                                "\217\127\223\230\236\217\234\230\217\236\130"\
                                "\222\236\227\226"
 
        /* "comprod/upgrades/" */
#define HTTP_SOFTWARE           HTTP_NETSCAPE "\215\231\227\232\234\231\216"\
                                "\131\237\232\221\234\213\216\217\235"
 
        /* "how-to-create-web-services.html" */
#define HTTP_HOWTO              HTTP_HOME "\222\231\241\127\236\231\127\215"\
                                "\234\217\213\236\217\127\241\217\214\127\235"\
                                "\217\234\240\223\215\217\235\130\222\236\227"\
                                "\226"
        /* "netscape-galleria.html" */
#define HTTP_GALLERIA           HTTP_HOME "\230\217\236\235\215\213\232\217"\
                                "\127\221\213\226\226\217\234\223\213\130\222"\
                                "\236\227\226"
 
        /* "relnotes/unix-" */
# define HTTP_REL_VERSION_PREFIX HTTP_ENG "\234\217\226\230\231\236\217\235"\
                                "\131\237\230\223\242\127"
 
# define HTTP_BETA_VERSION_PREFIX H_REL_VERSION_PREFIX
 
        /* "reginfo-x.cgi" */
#define HTTP_REG		HTTP_CGI "\234\217\221\223\230\220\231\127"\
				"\242\130\215\221\223"

 
        /* "handbook/" */
#define HTTP_MANUAL             HTTP_ENG "\222\213\230\216\214\231\231\225\131"

       /* "http://help.netscape.com/faqs.html" */
#define HTTP_FAQ                "\222\236\236\232\144\131\131\222\217\226\232"\
                                "\130\230\217\236\235\215\213\232\217\130\215"\
                                "\231\227\131\220\213\233\235\130\222\236\227"\
                                "\226" 
 
        /* "security-doc.html" */
#define HTTP_SECURITY           HTTP_INFO "\235\217\215\237\234\223\236\243"\
                                "\127\216\231\215\130\222\236\227\226"
 
        /* "auto_bug.cgi" */
#define HTTP_FEEDBACK		HTTP_CGI "\213\237\236\231\211\214\237\221"\
				"\130\215\221\223"
 
        /* "http://help.netscape.com/" */
#define HTTP_SUPPORT            "\222\236\236\232\144\131\131\222\217\226\232"\
				"\130\230\217\236\235\215\213\232\217\130\215"\
				"\231\227\131"

        /* "news/news.html" */
#define HTTP_USENET             HTTP_ENG "\230\217\241\235\131\230\217\241"\
                                "\235\130\222\236\227\226"

        /* "about:plugins" */
#define HTTP_PLUGINS "\213\214\231\237\236\144\232\226\237\221\223\230\235"

        /* "escapes/index.html" */
#define HTTP_ESCAPES HTTP_NETSCAPE "\217\235\215\213\232\217\235\131\223\230" \
                                   "\216\217\242\130\222\236\227\226"

#ifdef __sgi

	/* "http://www.sgi.com/surfer/silicon_sites.html" */
# define HTTP_SGI_MENU "\222\236\236\232\144\131\131\241\241\241\130\235\221"\
		  "\223\130\215\231\227\131\235\237\234\220\217\234\131\235"\
		  "\223\226\223\215\231\230\211\235\223\236\217\235\130\222"\
		  "\236\227\226"

	/* "http://www.sgi.com" */
# define HTTP_SGI_BUTTON "\222\236\236\232\144\131\131\241\241\241\130\235"\
			 "\221\223\130\215\231\227"

	/* "file:/usr/local/lib/netscape/docs/Welcome.html" */
# define HTTP_SGI_WELCOME "\220\223\226\217\144\131\237\235\234\131\226\231"\
			  "\215\213\226\131\226\223\214\131\230\217\236\235"\
			  "\215\213\232\217\131\216\231\215\235\131\201\217"\
			  "\226\215\231\227\217\130\222\236\227\226"

	/* "http://www.adobe.com" */
# define HTTP_ADOBE_MENU "\222\236\236\232\144\131\131\241\241\241\130\213"\
			 "\216\231\214\217\130\215\231\227"

	/* "http://www.sgi.com/surfer/cool_sites.html" */
# define SGI_WHATS_COOL  "\222\236\236\232\144\131\131\241\241\241\130\235"\
			 "\221\223\130\215\231\227\131\235\237\234\220\217"\
			 "\234\131\215\231\231\226\211\235\223\236\217\235"\
			 "\130\222\236\227\226"

	/* "http://www.sgi.com/surfer/netscape/relnotes-" */
# define SGI_VERSION_PREFIX	"\222\236\236\232\144\131\131\241\241\241\130"\
				"\235\221\223\130\215\231\227\131\235\237\234"\
				"\220\217\234\131\230\217\236\235\215\213\232"\
				"\217\131\234\217\226\230\231\236\217\235\127"

#endif /* __sgi */


static char *
go_get_url(char *which)
{
	char		clas[128];
	char		name[128];
	char		*p;
	char		*ret;
	char		*type;
	XrmValue	value;

	PR_snprintf(clas, sizeof (clas), "%s.Url.Which", fe_progclass);
	PR_snprintf(name, sizeof (name), "%s.url.%s", fe_progclass, which);
	if (XrmGetResource(XtDatabase(fe_display), name, clas, &type, &value))
	{
		ret = strdup(value.addr);
		if (!ret)
		{
			return NULL;
		}
		p = ret;
		while (*p)
		{
			*p += HTTP_OFF;
			p++;
		}
		if (!strstr(ret, DOT_NETSCAPE_DOT_COM_SLASH))
		{
			free(ret);
			return NULL;
		}
		return ret;
	}

	return NULL;
}


#define GO_GET_URL(func, res)			\
	static char *				\
	func(char *builtin)			\
	{					\
		static char	*ret = NULL;	\
						\
		if (ret)			\
		{				\
			return ret;		\
		}				\
						\
		ret = go_get_url(res);		\
		if (ret)			\
		{				\
			return ret;		\
		}				\
						\
		return builtin;			\
	}


GO_GET_URL(go_get_url_plugins, "aboutplugins")
GO_GET_URL(go_get_url_whats_new, "whats_new")
GO_GET_URL(go_get_url_whats_cool, "whats_cool")
GO_GET_URL(go_get_url_inet_directory, "directory")
GO_GET_URL(go_get_url_inet_search, "search")
GO_GET_URL(go_get_url_inet_white, "white")
GO_GET_URL(go_get_url_inet_about, "about")
GO_GET_URL(go_get_url_software, "about")
GO_GET_URL(go_get_url_howto, "howto")
GO_GET_URL(go_get_url_netscape, "netscape")
GO_GET_URL(go_get_url_galleria, "galleria")
GO_GET_URL(go_get_url_rel_version_prefix, "rel_notes")
GO_GET_URL(go_get_url_reg, "reg")
GO_GET_URL(go_get_url_manual, "manual")
GO_GET_URL(go_get_url_faq, "faq")
GO_GET_URL(go_get_url_usenet, "usenet")
GO_GET_URL(go_get_url_security, "security")
GO_GET_URL(go_get_url_feedback, "feedback")
GO_GET_URL(go_get_url_support, "support")
GO_GET_URL(go_get_url_destinations, "destinations")


#define H_WHATS_NEW             go_get_url_whats_new(HTTP_WHATS_NEW)
#define H_WHATS_COOL            go_get_url_whats_cool(HTTP_WHATS_COOL)
#define H_INET_DIRECTORY        go_get_url_inet_directory(HTTP_INET_DIRECTORY)
#define H_INET_SEARCH           go_get_url_inet_search(HTTP_INET_SEARCH)
#define H_INET_WHITE            go_get_url_inet_white(HTTP_INET_WHITE)
#define H_INET_ABOUT            go_get_url_inet_about(HTTP_INET_ABOUT)
#define H_SOFTWARE              go_get_url_software(HTTP_SOFTWARE)
#define H_HOWTO                 go_get_url_howto(HTTP_HOWTO)
#define H_NETSCAPE              go_get_url_netscape(HTTP_NETSCAPE)
#define H_GALLERIA              go_get_url_galleria(HTTP_GALLERIA)
#define H_REL_VERSION_PREFIX    go_get_url_rel_version_prefix(HTTP_REL_VERSION_PREFIX)
#define H_REG                   go_get_url_reg(HTTP_REG)
#define H_MANUAL                go_get_url_manual(HTTP_MANUAL)
#define H_FAQ                   go_get_url_faq(HTTP_FAQ)
#define H_USENET                go_get_url_usenet(HTTP_USENET)
#define H_SECURITY              go_get_url_security(HTTP_SECURITY)
#define H_FEEDBACK              go_get_url_feedback(HTTP_FEEDBACK)
#define H_SUPPORT               go_get_url_support(HTTP_SUPPORT)


/*
 * EXPORT_URL creates a function that returns a localized URL.
 * It is needed by the e-kit in order to allow localized URL's.
 */
#define EXPORT_URL(func, builtin) char* xfe_##func(void) {return func(builtin);}

EXPORT_URL(go_get_url_plugins, HTTP_PLUGINS)
EXPORT_URL(go_get_url_whats_new, HTTP_WHATS_NEW)
EXPORT_URL(go_get_url_whats_cool, HTTP_WHATS_COOL)
EXPORT_URL(go_get_url_inet_directory, HTTP_INET_DIRECTORY)
EXPORT_URL(go_get_url_inet_search, HTTP_INET_SEARCH)
EXPORT_URL(go_get_url_inet_white, HTTP_INET_WHITE)
EXPORT_URL(go_get_url_inet_about, HTTP_INET_ABOUT)
EXPORT_URL(go_get_url_software, HTTP_SOFTWARE)
EXPORT_URL(go_get_url_howto, HTTP_HOWTO)
EXPORT_URL(go_get_url_netscape, HTTP_NETSCAPE)
EXPORT_URL(go_get_url_galleria, HTTP_GALLERIA)
EXPORT_URL(go_get_url_reg, HTTP_REG)
EXPORT_URL(go_get_url_manual, HTTP_MANUAL)
EXPORT_URL(go_get_url_faq, HTTP_FAQ)
EXPORT_URL(go_get_url_usenet, HTTP_USENET)
EXPORT_URL(go_get_url_security, HTTP_SECURITY)
EXPORT_URL(go_get_url_feedback, HTTP_FEEDBACK)
EXPORT_URL(go_get_url_support, HTTP_SUPPORT)
EXPORT_URL(go_get_url_destinations, HTTP_ESCAPES)

/*
 * xfe_go_get_url_relnotes
 */
char*
xfe_go_get_url_relnotes(void)
{
  static char* url = NULL;

  if ( url == NULL ) {
      char buf[1024];
      char* ptr;
      char* prefix = (strchr(fe_version, 'a') || strchr(fe_version, 'b'))
                     ? HTTP_BETA_VERSION_PREFIX
                     : H_REL_VERSION_PREFIX;

#ifdef GOLD
      sprintf(buf, "%sGold.html", fe_version);
#else
      sprintf(buf, "%s.html", fe_version);
#endif

      for ( ptr = buf; *ptr; ptr++ ) {
          *ptr+= HTTP_OFF;
      }
      url = (char*) malloc(strlen(prefix)+strlen(buf)+1);
      strcpy(url, prefix);
      strcat(url, buf);
  }

  return url;
}


#ifdef OLD_DOCINFO_WAY
static void
fe_docinfo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_DocInfoDialog (context);
}
#else
static void
fe_docinfo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = DOCINFO, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
    *out = 0;

#ifdef EDITOR
  if (EDT_IS_EDITOR(context) && !FE_CheckAndSaveDocument(context))
    return;
#endif
  /* @@@@ this is the proper way to do it but I dont'
   * know the encoding scheme
   * fe_GetURL (context,NET_CreateURLStruct(buf, FALSE), FALSE);
   */
  fe_GetURL (context,NET_CreateURLStruct(DOCINFO, FALSE), FALSE);

#ifdef EDITOR
  fe_EditorRefresh(context);
#endif
}

static void
fe_frame_info_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  MWContext *ctx = fe_GetFocusGridOfContext (context);
  if (ctx) context = ctx;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = DOCINFO, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
    *out = 0;

  /* @@@@ this is the proper way to do it but I dont'
   * know the encoding scheme
   * fe_GetURL (context,NET_CreateURLStruct(buf, FALSE), FALSE);
   */
  fe_GetURL (context,NET_CreateURLStruct(DOCINFO, FALSE), FALSE);
}
#endif


#ifdef DEBUG_jwz

void
fe_other_browser (MWContext *context, URL_Struct *url)
{
  char buf[1024];
  pid_t forked;

  if (!url ||
      !url->address ||
      !*url->address)
    return;

  if (!fe_globalPrefs.other_browser ||
      !*fe_globalPrefs.other_browser)
    {
      FE_Alert (context, "No other browser configured.");
      return;
    }

  switch ((int) (forked = fork ()))
    {
    case -1:
      {
        FE_Alert (context, "couldn't fork.");
        break;
      }
    case 0:
      {
        char *av[10];
        int ac = 0;
	Display *dpy = XtDisplay (CONTEXT_WIDGET (context));
        close (ConnectionNumber (dpy));

        av[ac++] = fe_globalPrefs.other_browser;
        av[ac++] = url->address;
        av[ac] = 0;

        execvp (av[0], av);			/* shouldn't return. */
        sprintf (buf, "running %s", av[0]);
        perror (buf);
        exit (1);                               /* exits fork */
        break;
      }
    default:
      {
        /* nothing to do */
        break;
      }
    }
}


static void
fe_other_browser_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  History_entry *h;
  URL_Struct *url;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  h = SHIST_GetCurrent (&context->hist);
  if (!h) return;
  url = SHIST_CreateURLStructFromHistoryEntry (context, h);
  if (!url) return;

  fe_other_browser (context, url);
  NET_FreeURLStruct (url);
}
#endif /* DEBUG_jwz */


char *
xfe_get_netscape_home_page_url()
{
  static char buf [128];
  char *in, *out;
  for (in = H_NETSCAPE, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
  *out = 0;
  return buf;
}

static void
fe_net_showstatus_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char *rv;

  fe_UserActivity (context);
  rv = NET_PrintNetlibStatus();
  NET_ToggleTrace();  /* toggle trace mode on and off */
  XFE_Alert (context, rv);
  free(rv);
}

static void
fe_fishcam_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = HTTP_FC, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
  *out = 0;
  fe_GetURL (context,NET_CreateURLStruct(buf, FALSE), FALSE);
}

void
fe_NetscapeCallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  if ( ekit_LogoUrl() ) {
    strcpy(buf, ekit_LogoUrl());
  } else {
    for (in = H_NETSCAPE, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
    *out = 0;
  }

#ifdef EDITOR
  if (EDT_IS_EDITOR(context))
      fe_GetURLInBrowser(context, NET_CreateURLStruct (buf, FALSE));
  else
#endif /*EDITOR*/
      fe_GetURL (context, NET_CreateURLStruct (buf, FALSE), FALSE);
}


#ifdef __sgi
static void
fe_sgi_menu_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = HTTP_SGI_MENU, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
  *out = 0;
  fe_GetURL (context, NET_CreateURLStruct (buf, FALSE), FALSE);
}

static void
fe_adobe_menu_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = HTTP_ADOBE_MENU, out = buf; *in; in++, out++)
    *out = *in - HTTP_OFF;
  *out = 0;
  fe_GetURL (context, NET_CreateURLStruct (buf, FALSE), FALSE);
}

void
fe_SGICallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = HTTP_SGI_BUTTON, out = buf; *in; in++, out++)
    *out = *in - HTTP_OFF;
  *out = 0;
  fe_GetURL (context, NET_CreateURLStruct (buf, FALSE), FALSE);
}

void
fe_sgi_welcome_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  char buf [1024], *in, *out;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  for (in = HTTP_SGI_WELCOME, out = buf; *in; in++, out++)
    *out = *in - HTTP_OFF;
  *out = 0;
  fe_GetURL (context, NET_CreateURLStruct (buf, FALSE), FALSE);
}
#endif /* __sgi */

static void
fe_toggle_loads_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean load_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &load_p, 0);
  CONTEXT_DATA (context)->autoload_images_p = load_p;
}

#if defined(DEBUG_jwz) && defined(MOCHA)
static void
fe_toggle_js_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean enable_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &enable_p, 0);
  fe_globalPrefs.disable_javascript = !enable_p;
  LM_SwitchMocha(enable_p);
}

#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
extern XP_Bool IL_AnimationLoopsEnabled;
extern XP_Bool IL_AnimationsEnabled;

static void fe_interrupt_images (MWContext *c);

static void
fe_toggle_anim_1 (Widget widget, MWContext *context, XmAnyCallbackStruct *cb,
                  XP_Bool all_anims_p)
{
  Boolean enable_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &enable_p, 0);
  if (all_anims_p)
    fe_globalPrefs.anim_p = enable_p;
  else
    fe_globalPrefs.anim_loop_p = enable_p;

  IL_AnimationLoopsEnabled = fe_globalPrefs.anim_loop_p;
  IL_AnimationsEnabled     = fe_globalPrefs.anim_p;

  if (!enable_p)
    {
      struct fe_MWContext_cons *rest;
      for (rest = fe_all_MWContexts; rest; rest = rest->next)
# if 1
        fe_interrupt_images (rest->context);
# else
      XP_InterruptContext (rest->context);
# endif
    }
}

static void
fe_interrupt_images (MWContext *c)
{
  if (IL_AreThereLoopingImagesForContext (c))
    {
      int i = 1;
      MWContext *child;
      IL_InterruptContext (c);
      while ((child = (MWContext*)
              XP_ListGetObjectNum (c->grid_children, i++)))
        fe_interrupt_images (child);
    }
}

static void
fe_toggle_anim_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  fe_toggle_anim_1 (widget, context, cb, TRUE);
}

static void
fe_toggle_loops_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  fe_toggle_anim_1 (widget, context, cb, FALSE);
}
#endif /* DEBUG_jwz */


#ifdef DEBUG_jwz

XP_Bool fe_ignore_font_size_changes_p = FALSE;

static void
fe_toggle_font_size_changes_enabled_cb (Widget widget, XtPointer closure,
                                        XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_ignore_font_size_changes_p = !fe_ignore_font_size_changes_p;
  fe_ReLayout (context, NET_RESIZE_RELOAD);
}

#endif /* DEBUG_jwz */


#if 0
static void
fe_toggle_fancy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean fancy_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &fancy_p, 0);
  CONTEXT_DATA (context)->fancy_ftp_p = fancy_p;
}


static void
fe_save_link_mode_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  CONTEXT_DATA (context)->save_next_mode_p = True;
  fe_SetCursor (context, False);
  XFE_Progress (context, fe_globalData.click_to_save_message);
}
#endif


static void
fe_toggle_compose_wrap_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean wrap_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &wrap_p, 0);
  fe_set_compose_wrap_state(context, wrap_p);
}



void
fe_RebuildWindow (MWContext *context)
{
  Widget shell = CONTEXT_WIDGET (context);
  History_entry *he = SHIST_GetCurrent (&context->hist);
  URL_Struct *url = NULL;

  if (he)
    url = SHIST_CreateURLStructFromHistoryEntry (context, he);

  /* We discard the document here since, with the advent of frames,
   * fe_MakeWidgets was wreaking havoc by destroying the frame
   * widgets out from under layout. When layout later called 
   * fe_DestroyContext in the process of re-laying out, freed
   * pointers were everywhere. See commands.c 1.393 for the old
   * mechanism.
   *
   * We cannot destroyt the document for the editor, because that
   * would destroy the user's work. We don't support frames so
   * things work out ok (I guess)...djw.
   */
  XP_InterruptContext (context);
#if defined(EDITOR) && defined(EDITOR_UI)
  if (!EDT_IS_EDITOR(context))
#endif
      LO_DiscardDocument (context);
  fe_MakeWidgets (shell, context);
  /* XXX Background get lost. Wonder if layout should do this. - dp */
  XFE_SetBackgroundColor(context, CONTEXT_DATA(context)->bg_red,
		CONTEXT_DATA(context)->bg_green,CONTEXT_DATA(context)->bg_blue);
  LO_RedoFormElements (context);
  fe_InitScrolling (context); /* big voodoo */
  CONTEXT_DATA (context)->bookmark_menu_up_to_date_p = False;
  fe_SensitizeMenus (context);
  fe_BookmarkInit (context);
  fe_RegenerateHistoryMenu (context);
  if (url) {
#if defined(EDITOR) && defined(EDITOR_UI)
  if (EDT_IS_EDITOR(context))
    fe_EditorRefresh(context);
  else
#endif
    fe_GetURL (context, url, FALSE);
  }
}

static void
fe_show_toolbar_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean toolbar_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &toolbar_p, 0);
  if (CONTEXT_DATA (context)->show_toolbar_p == toolbar_p)
    return;
  CONTEXT_DATA (context)->show_toolbar_p = toolbar_p;
  fe_RebuildWindow (context);
}

static void
fe_show_url_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean url_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &url_p, 0);
  if (CONTEXT_DATA (context)->show_url_p == url_p)
    return;
  CONTEXT_DATA (context)->show_url_p = url_p;
  fe_RebuildWindow (context);
}


static void
fe_show_directory_buttons_cb (Widget widget, XtPointer closure,
			      XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean directory_buttons_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &directory_buttons_p, 0);
  if (CONTEXT_DATA (context)->show_directory_buttons_p == directory_buttons_p)
    return;
  CONTEXT_DATA (context)->show_directory_buttons_p = directory_buttons_p;
  fe_RebuildWindow (context);
}

static void
fe_show_menubar_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean menubar_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &menubar_p, 0);
  if (CONTEXT_DATA (context)->show_menubar_p == menubar_p)
    return;
  CONTEXT_DATA (context)->show_menubar_p = menubar_p;
  fe_RebuildWindow (context);
}

#if 0 /* security people tomw@netscape.com wanted this off */
static void
fe_show_bottom_status_bar_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean bottom_status_bar_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &bottom_status_bar_p, 0);
  if (CONTEXT_DATA (context)->show_bottom_status_bar_p == bottom_status_bar_p)
    return;
  CONTEXT_DATA (context)->show_bottom_status_bar_p = bottom_status_bar_p;
  fe_RebuildWindow (context);
}
#endif

#if 0
#ifdef HAVE_SECURITY
static void
fe_show_security_bar_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  Boolean security_bar_p = False;
  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNset, &security_bar_p, 0);
  if (CONTEXT_DATA (context)->show_security_bar_p == security_bar_p)
    return;
  CONTEXT_DATA (context)->show_security_bar_p = security_bar_p;
  fe_RebuildWindow (context);
}
#endif /* HAVE_SECURITY */
#endif

#ifdef JAVA
static void
fe_show_java_console_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
#if 0
  MWContext *context = (MWContext *) closure;
#endif
  Boolean show_java_console_p = False;
  XtVaGetValues (widget, XmNset, &show_java_console_p, 0);
  if (show_java_console_p) {
      LJ_ShowConsole();
  } else {
      LJ_HideConsole();
  }
}
#endif

static void
fe_load_images_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_LoadDelayedImages (context);
}

/* Navigate menu
 */

static void
fe_back_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  MWContext *top_context = fe_GridToTopContext(context);
  URL_Struct *url;
#ifdef NEW_FRAME_HIST
  if (fe_IsGridParent(top_context))
    {
        if (LO_BackInGrid(top_context))
          {
            return;
          }
    }
#endif /* NEW_FRAME_HIST */
  fe_UserActivity (top_context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url = SHIST_CreateURLStructFromHistoryEntry (top_context,
					       SHIST_GetPrevious (top_context));
  if (url)
    {
      fe_GetURL (top_context, url, FALSE);
    }
  else
    {
      FE_Alert (top_context, fe_globalData.no_previous_url_message);
    }
}

static void
fe_forward_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  MWContext *top_context = fe_GridToTopContext(context);
  URL_Struct *url;
#ifdef NEW_FRAME_HIST
  if (fe_IsGridParent(top_context))
    {
        if (LO_ForwardInGrid(top_context))
          {
            return;
          }
    }
#endif /* NEW_FRAME_HIST */
  fe_UserActivity (top_context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url = SHIST_CreateURLStructFromHistoryEntry (top_context,
					       SHIST_GetNext (top_context));
  if (url)
    {
      fe_GetURL (top_context, url, FALSE);
    }
  else
    {
      FE_Alert (top_context, fe_globalData.no_next_url_message);
    }
}

static void
fe_home_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  URL_Struct *url;
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  if (!fe_globalPrefs.home_document || !*fe_globalPrefs.home_document)
    {
      FE_Alert (context, fe_globalData.no_home_url_message);
      return;
    }
  url = NET_CreateURLStruct (fe_globalPrefs.home_document, FALSE);
  fe_GetURL ((MWContext *) closure, url, FALSE);
}

static void
fe_history_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_HistoryDialog ((MWContext *) closure);
}

static void
fe_add_bookmark_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_AddBookmarkCallback (widget, closure, call_data);
}

static void
fe_view_bookmark_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_ViewBookmarkCallback(widget, closure, call_data);
}

static void
fe_view_addressbook_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  FE_GetAddressBook(context, True);
}

#if 0
static void
fe_goto_bookmark_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_GotoBookmarkCallback (widget, closure, call_data);
}
#endif /* 0 */


/* Help menu
 */
static void
fe_about_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_GetURL (context, NET_CreateURLStruct ("about:", FALSE), FALSE);
}

#ifdef X_PLUGINS
void
fe_aboutPlugins_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  fe_GetURL (context, NET_CreateURLStruct ("about:plugins", FALSE), FALSE);
}
#endif /* X_PLUGINS */

static void
fe_pulldown_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  Widget submenu;
  MWContext *context = (MWContext *) closure;
  fe_ContextData* data = CONTEXT_DATA(context);
  fe_UserActivity (context);

  XtVaGetValues(widget, XmNsubMenuId, &submenu, 0);

  /* Recreate the bookmark menu as necessary each time we popup the
     bookmarks menu from the menubar. 
  */
  if (submenu == data->bookmark_menu && !data->bookmark_menu_up_to_date_p)
    fe_GenerateBookmarkMenu (context);

#if 0
  /* Recreation of windows menu as necessary */
  if (submenu == data->windows_menu && !data->windows_menu_up_to_date_p)
    fe_GenerateWindowsMenu (context);
#else
  /* We need to handle some more conditions. For now since it is a
     not such an expensive operations, we will force recreation of
     windows menu.
  */
  if (submenu == data->windows_menu) {
     data->windows_menu_up_to_date_p = False;
    fe_GenerateWindowsMenu (context);
  }
#endif

  fe_SensitizeMenus (context);
}


/* Message composition stuff. */

static void
fe_mailcompose_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;
    XtPointer value;
    fe_MailComposeCallback cbid;

    XtVaGetValues(widget, XmNuserData, &value, 0);
    cbid = (fe_MailComposeCallback) value;

    fe_mailcompose_obeycb(context, cbid, call_data);
}

void
fe_mailcompose_obeycb(MWContext *context, fe_MailComposeCallback cbid,
			void *call_data)
{
    switch (cbid) {
	case ComposeClearAllText_cb: {
	    Widget text = CONTEXT_DATA (context)->mcBodyText;
	    XmTextSetString(text, "");
	    break;
	}
	case ComposeSelectAllText_cb: {
	    Widget text = CONTEXT_DATA (context)->mcBodyText;
	    XmPushButtonCallbackStruct *cb = (XmPushButtonCallbackStruct *)
						call_data;
	    if (cb)
		XmTextSetSelection(text, 0, XmTextGetLastPosition(text),
					cb->event->xkey.time);
	    break;
	}
    default:
      XP_ASSERT (0);
      break;
    }
}

void
fe_handle_delivery_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *) call_data;
  struct fe_MWContext_cons *cntx = fe_all_MWContexts;

  if (cb->reason != XmCR_VALUE_CHANGED) abort ();
  fe_UserActivity (context);

  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == CONTEXT_DATA (context)->deliverLater_menuitem)
    {
      XtVaSetValues (CONTEXT_DATA (context)->deliverNow_menuitem, XmNset, False, 0);
      fe_globalPrefs.queue_for_later_p = True;
    }
  else if (widget == CONTEXT_DATA (context)->deliverNow_menuitem)
    {
      XtVaSetValues (CONTEXT_DATA (context)->deliverLater_menuitem, XmNset, False, 0);
      fe_globalPrefs.queue_for_later_p = False;
    }
  else
    abort ();

  /* The mail compose toolbar of all open Mail Compose windows will change
     to send or send later icon depending on deliver_later */
  while (cntx) {
    if ((cntx->context->type == MWContextMessageComposition) &&
	(CONTEXT_DATA(cntx->context)->toolbar)) {
      Widget *buttons = 0;
      Cardinal nbuttons = 0;

      XtVaGetValues(CONTEXT_DATA(cntx->context)->toolbar,
		XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
      if (nbuttons >= 1) {
	/* Change the sensitive and insensitive pixmap and labelString */
	if (CONTEXT_DATA (context)->show_toolbar_icons_p) {
	  Pixmap p =  fe_ToolbarPixmap (context, 0, False, False);
	  Pixmap ip = fe_ToolbarPixmap (context, 0, True, False);
	  if (! ip) ip = p;
          XtVaSetValues (buttons[0],
	  		 XmNlabelPixmap, p,
	  		 XmNlabelInsensitivePixmap, ip, 0);
	}
	fe_MsgSensitizeChildren(CONTEXT_DATA(cntx->context)->toolbar,
			(XtPointer)cntx->context, 0);
      }
    }
    cntx = cntx->next;
  }

  /* The spec says changing mail delivery should be saved immediately */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));
}

void
FE_InsertMessageCompositionText(MWContext* context,
				const char* text,
				XP_Bool leaveCursorAtBeginning) {
  fe_ContextData* data = CONTEXT_DATA(context);
  XmTextPosition pos = 0, newpos = 0;
  unsigned char *loc;
  XtVaGetValues(data->mcBodyText, XmNcursorPosition, &pos, 0);
  loc = fe_ConvertToLocaleEncoding(INTL_DefaultWinCharSetID(context),
				   (unsigned char *) text);

#ifdef DEBUG_jwz
        {
          /* On my Linux box, Motif freaks out if you try to insert a
             high-bit character into a text field.  So don't.
           */
          unsigned char *s = (unsigned char *) loc;
          if (s)
            while (*s)
              {
                if (*s & 0x80) *s &= 0x7F;
                s++;
              }
        }
#endif /* DEBUG_jwz */

  XmTextInsert(data->mcBodyText, pos, (char*) loc);
  if (((char *) loc) != text) {
    XP_FREE(loc);
  }
  XtVaGetValues(data->mcBodyText, XmNcursorPosition, &newpos, 0);
  if (leaveCursorAtBeginning) {
    XtVaSetValues(data->mcBodyText, XmNcursorPosition, pos, 0);
  }
  else if (pos == newpos) {
    /* On some motif (eg. AIX), text insertion point is not moved after
     * inserted text. We depend on that here.
     *
     * WARNING: XXX I18N watch. The strlen might not be the right i18n way.
     */
    newpos = pos+strlen(text);
    XtVaSetValues(data->mcBodyText, XmNcursorPosition, newpos, 0);
  }
}


/* Document encoding stuff (internationalization) */

void
fe_doc_enc_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_button	*button;
	MWContext	*context;
	Widget		*kids;
	Cardinal	nkids;

	context = ((fe_button_closure *) closure)->context;
	button = ((fe_button_closure *) closure)->button;

	if (!widget)
	{
		if (button->var == (void *)(long) INTL_DefaultDocCharSetID(NULL))
		{
			*((Boolean *) call_data) = True;
		}
		else
		{
			*((Boolean *) call_data) = False;
		}
	}
	else if (button->var != (void *)(long) (context->fe.data->xfe_doc_csid))
	{
#ifdef EDITOR
		if (EDT_IS_EDITOR(context) &&
		    !FE_CheckAndSaveDocument(context)) {
		    XtVaSetValues(widget, XmNset, False, NULL);
		    return;
		}
#endif
		nkids = 0;
		XtVaGetValues(XtParent(widget), XmNchildren, &kids,
			XmNnumChildren, &nkids, 0);
		while (nkids-- > 0)
		{
			XtVaSetValues(kids[nkids], XmNset, False, NULL);
		}

		XtVaSetValues(widget, XmNset, True, NULL);

		context->fe.data->xfe_doc_csid = (int16) (int) button->var;
		context->win_csid =
			INTL_DocToWinCharSetID(context->fe.data->xfe_doc_csid);

		fe_ReLayout(context, 0);
	}
	else
	{
		XtVaSetValues(widget, XmNset, True, NULL);
	}
}


/* The menubar and toolbar descriptions.
 */

#undef OFF
#define OFF(x) ((void *) (XtOffset(fe_ContextData *, x)))

static struct fe_button fe_doc_encoding[] = 
{
    { "docEncMenu",	0,		SELECTABLE,	(void *) 0           },
    { "latin1",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_LATIN1   },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "latin2",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_LATIN2   },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "jaAuto",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_JIS_AUTO },
    { "jaSJIS",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_SJIS     },
    { "jaEUC",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_EUCJP    },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "twBig5",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_BIG5     },
    { "twEUC",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_CNS_8BIT },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "gbEUC",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_GB_8BIT  },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "krEUC",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_KSC_8BIT },
    { "2022kr",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_2022_KR  },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "koi8r",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_KOI8_R   },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "88595",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_8859_5   },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "greek",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_8859_7   },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "88599",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_8859_9   },
    { "",		0,		UNSELECTABLE,	(void *) 0           },
    { "other",		fe_doc_enc_cb,	TOGGLE,		(void *) CS_USRDEF2  },
    { 0 }
};


static struct fe_button fe_web_menubar[] = 
{
    { "file",		0,			SELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
#if defined(EDITOR) && defined(EDITOR_UI)
    { "editorNew",      0,                    CASCADE,                0 },
      { "editorNewBlank", fe_editor_new_cb,	SELECTABLE,	    	0 },
      { "editorNewTemplate", fe_editor_new_from_template_cb, SELECTABLE, 0 },
      { "editorNewWizard", fe_editor_new_from_wizard_cb, SELECTABLE, 0 },
      { 0 },
    FE_MENU_SEPARATOR,
    { "editDocument",   fe_editor_edit_cb,      SELECTABLE,      0        },
    FE_MENU_SEPARATOR,
    { "openURL",	fe_editor_open_url_cb,	SELECTABLE,		0 },
    { "openFile",	fe_open_file_cb,	SELECTABLE,		0 },
    { "editorOpenFile", fe_editor_open_file_cb, SELECTABLE,           0 },
    { "saveAs",		fe_save_as_cb,		SELECTABLE,		0,
      OFF(saveAs_menuitem) },
    { "uploadFile",	fe_upload_file_cb,	SELECTABLE,		0,
      OFF(uploadFile_menuitem) },
    FE_MENU_SEPARATOR,
    { "mailNew",	fe_mailNew_cb,		SELECTABLE,		0 },
    { "mailto",		fe_mailto_cb,		SELECTABLE,		0,
      OFF(mailto_menuitem) },
#else
    { "mailNew",	fe_mailNew_cb,		SELECTABLE,		0 },
    { "mailto",		fe_mailto_cb,		SELECTABLE,		0,
      OFF(mailto_menuitem) },
    FE_MENU_SEPARATOR,
    { "openURL",	fe_open_url_cb,		SELECTABLE,		0 },
    { "openFile",	fe_open_file_cb,	SELECTABLE,		0 },
    { "saveAs",		fe_save_as_cb,		SELECTABLE,		0,
      OFF(saveAs_menuitem) },
    { "uploadFile",	fe_upload_file_cb,	SELECTABLE,		0,
      OFF(uploadFile_menuitem) },
#endif /* EDITOR */
    FE_MENU_SEPARATOR,
    { "print",		fe_print_cb,		SELECTABLE,		0,
      OFF(print_menuitem)},
    FE_MENU_SEPARATOR,
    { "delete",		fe_delete_cb,		SELECTABLE,		0,
      OFF(delete_menuitem)},
    { "quit",		fe_QuitCallback,	SELECTABLE,		0 },
    { 0 },
    { "edit",   	0,			SELECTABLE,		0 },
    { "undo",		fe_undo_cb,		UNSELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "cut",		fe_cut_cb,		SELECTABLE,		0,
      OFF(cut_menuitem)},
    { "copy",		fe_copy_cb,		SELECTABLE,		0,
      OFF(copy_menuitem)},
    { "paste",		fe_paste_cb,		SELECTABLE,		0,
      OFF(paste_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
    { "find",		fe_find_cb,		SELECTABLE,		0 },
    { "findAgain",	fe_find_again_cb,	UNSELECTABLE,		0,
      OFF(findAgain_menuitem)},
    { 0 },
    { "view",   	 0,			SELECTABLE,		0 },
    { "reload",		fe_reload_cb,		SELECTABLE,		0 },
    { "reloadFrame",	fe_reload_frame_cb,	UNSELECTABLE,		0,
      OFF(reloadFrame_menuitem)},
    { "loadImages",	fe_load_images_cb,	SELECTABLE,		0,
      OFF(delayed_menuitem)},
    { "refresh",	fe_refresh_cb,		SELECTABLE,		0,
      OFF(refresh_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
    { "source",		fe_view_source_cb,	SELECTABLE,		0 },
    { "docInfo",	fe_docinfo_cb,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "frameSource",	fe_frame_source_cb,	SELECTABLE,		0,
      OFF(frameSource_menuitem)},
    { "frameInfo",	fe_frame_info_cb,	SELECTABLE,		0,
      OFF(frameInfo_menuitem)},
#ifdef JAVA_INFO
#ifdef JAVA
    { "javaInfo",	0,			UNSELECTABLE,		0 },
#endif
#endif /* JAVA_INFO */
#ifdef DEBUG_jwz
    { "",		0,			UNSELECTABLE,		0 },
    { "otherBrowser",	fe_other_browser_cb,	SELECTABLE,		0 },
#endif /* DEBUG_jwz */
    { 0 },
    { "go",	   	 0,			SELECTABLE,		0,
      OFF(history_menu)},
    { "back",		fe_back_cb,		UNSELECTABLE,		0,
      OFF(back_menuitem)},
    { "forward",	fe_forward_cb,		UNSELECTABLE,		0,
      OFF(forward_menuitem)},
    { "home",		fe_home_cb,		UNSELECTABLE,		0,
      OFF(home_menuitem)},
    { "abort",		fe_abort_cb,		UNSELECTABLE,		0,
      OFF(abort_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
#if defined(EDITOR) && defined(EDITOR_UI)
    { "browsePublishLocation", fe_editor_browse_publish_location_cb,
      SELECTABLE, 0 },
    { "",		0,			UNSELECTABLE,		0 },
#endif
    { 0 },
    { "bookmark",   	 0,			SELECTABLE,		0,
      OFF(bookmark_menu)},
    { "addBookmark",	 fe_add_bookmark_cb,	SELECTABLE,		0 },
#ifdef gone_for_beta2 /* talk to jean charles */
    { "gotoBookmarkHtml", fe_goto_bookmark_cb,	SELECTABLE,		0 },
#endif
    { 0 },
    { "options",   	 0,			SELECTABLE,		0 },
    { "generalPrefs",	 fe_general_prefs_cb,	SELECTABLE,		0 },
#if defined(EDITOR) && defined(EDITOR_UI)
    { "editorPrefs",	fe_editor_preferences_dialog_cb, SELECTABLE,    0 },
#endif /*EDITOR*/
    { "mailNewsPrefs",	 fe_mailnews_prefs_cb,	SELECTABLE,		0 },
    { "networkPrefs",	 fe_network_prefs_cb,	SELECTABLE,		0 },
#ifdef HAVE_SECURITY /* jwz */
    { "securityPrefs",	 fe_security_prefs_cb,	SELECTABLE,		0 },
#endif /* HAVE_SECURITY -- jwz */
    { "",		 0,			UNSELECTABLE,		0 },
    { "showMenubar",	fe_show_menubar_cb,	TOGGLE,	OFF(show_menubar_p)  },
    { "showToolbar",	fe_show_toolbar_cb,	TOGGLE,	OFF(show_toolbar_p)  },
    { "showURL",	fe_show_url_cb,		TOGGLE,	OFF(show_url_p)      },
    { "showDirectoryButtons", fe_show_directory_buttons_cb,
					        TOGGLE,
						OFF(show_directory_buttons_p)},
#if 0 /* security people tomw@netscape.com wanted this off */
    { "showBottomStatusbar",	fe_show_bottom_status_bar_cb,
						TOGGLE,
						OFF(show_bottom_status_bar_p)},
#endif /* 0 */
#ifdef JAVA
    { "showJavaConsole", fe_show_java_console_cb, TOGGLE,
      OFF(show_java_console_p), OFF(show_java) },
#endif

#if 0
#ifdef HAVE_SECURITY
    { "showSecurityBar", fe_show_security_bar_cb,TOGGLE,
						OFF(show_security_bar_p)  },
#endif
#endif
    { "",		 0,			UNSELECTABLE,		0 },
    { "autoLoadImages", fe_toggle_loads_cb,   TOGGLE, OFF(autoload_images_p) },
#if defined(DEBUG_jwz) && defined(MOCHA)
    { "toggleJS",   fe_toggle_js_cb,   TOGGLE, 0, OFF(toggleJS_menuitem) },
    { "toggleAnim", fe_toggle_anim_cb, TOGGLE, 0, OFF(toggleAnim_menuitem) },
    { "toggleLoops", fe_toggle_loops_cb, TOGGLE, 0,OFF(toggleLoops_menuitem) },
#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
    { "toggleSizes", fe_toggle_font_size_changes_enabled_cb, TOGGLE,
      0,OFF(toggleSizes_menuitem) },
#endif /* DEBUG_jwz */

#if 0
    { "fancyFTP", 	fe_toggle_fancy_cb,	TOGGLE,	OFF(fancy_ftp_p)     },
#endif
    { "",		 0,			UNSELECTABLE,		0 },
    { "docEncoding",	0,			INTL_CASCADE,
						(void *) fe_doc_encoding },
    { "",		 0,			UNSELECTABLE,		0 },
    { "saveOptions",    fe_save_options_cb,     SELECTABLE,		0 },

#if 0 /* This doesn't work real well - Motif bugs apparently. */
    { "",		 0,			UNSELECTABLE,		0 },
    { "changeVisual",	 fe_ChangeVisualCallback,SELECTABLE,		0 },
#endif
    { 0 },

    /*
     * Since customers can configure the directory menu using
     * e-kit, the directory menu is added by the e-kit at run-time.
     * See e_kit.c to add menu items, or e_kit.ad to change URLs.
     */
    { "directory", 0,			SELECTABLE,		0 },
    { 0 }, /* end of directory menu */

    { "windows",   	 0,			SELECTABLE,		0,
      OFF(windows_menu)},
/* Order matters here. Check with fe_GenerateWindowsMenu() */
    { "openMail",	fe_open_mail,		TOGGLE,			0 },
    { "openNews",	fe_open_news,		TOGGLE,			0 },
#ifdef DEBUG_jwz
    { "",		 0,			UNSELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
#endif /* DEBUG_jwz */
    { "",		 0,			UNSELECTABLE,		0 },
    { "editAddressBook",fe_view_addressbook_cb,	TOGGLE,			0 },
    { "viewBookmark",	fe_view_bookmark_cb,	TOGGLE,			0 },
    { "viewHistory",	fe_history_cb,		TOGGLE,			0 },
    { "",		 0,			UNSELECTABLE,		0 },
    { 0 },

    /*
     * Since customers can configure the help menu using
     * e-kit, the directory menu is added by the e-kit at run-time.
     * See e_kit.c to add menu items, or e_kit.ad to change URLs.
     * The 'about' item is left here, since that *always* appears in
     * the help menu.  The customer cannot make it disappear.
     */
    { "help",   	0,			SELECTABLE,		0 },
    { "about",		fe_about_cb,		SELECTABLE,		0 },
    { 0 },
    { 0 }
};

#ifdef EDITOR

struct fe_button fe_editor_menubar[] =
{
  /* File Menu */
  FE_MENU_BEGIN("file"),
  { "openBrowser",	fe_new_cb,	        SELECTABLE,		0 },
  { "editorNew",           0,                      CASCADE,                0 },
      { "editorNewBlank", fe_editor_new_cb,	SELECTABLE,	    	0 },
      { "editorNewTemplate", fe_editor_new_from_template_cb, SELECTABLE,0 },
      { "editorNewWizard", fe_editor_new_from_wizard_cb, SELECTABLE,	0 },
      { 0 },
  FE_MENU_SEPARATOR,
  { "editorBrowse",        fe_editor_browse_doc_cb, SELECTABLE,           0 },
  FE_MENU_SEPARATOR,
  { "openURL",          fe_editor_open_url_cb,	SELECTABLE,		0 },
  { "editorOpenFile",   fe_editor_open_file_cb, SELECTABLE,             0 },
  { "save",	        fe_editor_save_cb,      SELECTABLE,		0 },
  { "saveAs",     	fe_editor_save_as_cb,	SELECTABLE,             0,   
    OFF(saveAs_menuitem) },
  { "publish",          fe_editor_publish_cb,   SELECTABLE,             0 },
  FE_MENU_SEPARATOR,
  { "mailNew",	        fe_mailNew_cb,		SELECTABLE,		0 },
  { "mailto",		fe_mailto_cb,		SELECTABLE,		0,
    OFF(mailto_menuitem) },
  FE_MENU_SEPARATOR,
#if 0
  { "pageSetup",        FE_NOT_IMPLEMENTED,     UNSELECTABLE,           0 },
#endif
  { "print",		fe_print_cb,		SELECTABLE,		0,
    OFF(print_menuitem)},
#if 0
  { "printPreview",     FE_NOT_IMPLEMENTED,     UNSELECTABLE,           0 },
#endif
  FE_MENU_SEPARATOR,
  { "delete",		fe_editor_delete_cb,	SELECTABLE,		0,
    OFF(delete_menuitem)},
  { "quit",		fe_QuitCallback,	SELECTABLE,		0 },
  FE_MENU_END,

  /* Edit Menu */
  { "edit",   		fe_editor_edit_menu_cb,	SELECTABLE,		0 },
  { "undo",             fe_editor_undo_cb,      SELECTABLE,           0 },
  { "redo",             fe_editor_redo_cb,      SELECTABLE,           0 },
  FE_MENU_SEPARATOR,
  { "cut", 	        fe_editor_cut_cb, SELECTABLE, 0, OFF(cut_menuitem) },
  { "copy", 	        fe_editor_copy_cb, SELECTABLE, 0, OFF(copy_menuitem) },
  { "paste",	        fe_editor_paste_cb, SELECTABLE, 0,OFF(paste_menuitem)},
  { "deleteItem",	fe_editor_delete_item_cb, SELECTABLE,           0 },
  { "selectAll",        fe_editor_select_all_cb,  SELECTABLE,           0 },
  FE_MENU_SEPARATOR,
  { "selectTable",      fe_editor_select_table_cb, UNSELECTABLE,        0 },
  { "deleteTableMenu",  fe_editor_table_menu_cb, CASCADE, 0 },
    { "deleteTable",    fe_editor_table_delete_cb, UNSELECTABLE },
    { "deleteRow",      fe_editor_table_row_delete_cb, UNSELECTABLE },
    { "deleteColumn",   fe_editor_table_column_delete_cb, UNSELECTABLE },
    { "deleteCell",     fe_editor_table_cell_delete_cb, UNSELECTABLE },
    FE_MENU_END,
  FE_MENU_SEPARATOR,
  { "removeLinks",	fe_editor_remove_links_cb,     SELECTABLE,     0 },
  FE_MENU_SEPARATOR,
  { "find",		fe_editor_find_cb,	SELECTABLE,		0 },
  { "findAgain",	fe_editor_find_again_cb, UNSELECTABLE,		0,
      OFF(findAgain_menuitem)},
  FE_MENU_END,
  
  /* View Menu */
  { "view",   		fe_editor_view_menu_cb,	SELECTABLE,		0 },
  { "reload",		fe_editor_reload_cb,	SELECTABLE,		0 },
  { "loadImages",	fe_load_images_cb,	SELECTABLE,		0,
    OFF(delayed_menuitem)},
  { "refresh",          fe_editor_refresh_cb,	SELECTABLE,		0 },
  FE_MENU_SEPARATOR,
  { "source",           fe_editor_view_source_cb, SELECTABLE,		0 },
#ifdef DEBUG
  { "Browser Source",   fe_view_source_cb,	SELECTABLE,		0 },
#endif
  { "editSource",	fe_editor_edit_source_cb, SELECTABLE,		0 },
  { "docInfo",          fe_docinfo_cb,		SELECTABLE,		0 },
  FE_MENU_SEPARATOR,
  { "paragraphMarks",	fe_editor_paragraph_marks_cb, TOGGLE,           0 },
  { "displayTables",	fe_editor_display_tables_cb,  TOGGLE,           0 },
#if 0 /* do for Beta 5 */
  { "displayTableBoundaries", FE_NOT_IMPLEMENTED, UNSELECTABLE,     0 },
#endif
#ifdef JAVA_INFO
#ifdef JAVA
  { "javaInfo",	        0,			UNSELECTABLE,		0 },
#endif
#endif /* JAVA_INFO */
  FE_MENU_END,

  /* Insert Menu */
  { "insert",   	fe_editor_insert_menu_cb,   SELECTABLE,	0 },
  { "insertLink",	fe_editor_insert_link_cb,   SELECTABLE, 0 },
  { "insertTarget",	fe_editor_insert_target_cb, SELECTABLE, 0 },
  { "insertImage",	fe_editor_insert_image_cb,  SELECTABLE, 0 },
  { "insertHorizontalLine", fe_editor_insert_hrule_dialog_cb, SELECTABLE, 0 },
  { "insertHtmlTag",  	fe_editor_insert_html_cb,   SELECTABLE, 0 },
  FE_MENU_SEPARATOR,
  { "insertTableMenu",  fe_editor_table_menu_cb, CASCADE, 0 },
    { "insertTable",    fe_editor_table_insert_cb, SELECTABLE },
    { "insertRow",      fe_editor_table_row_insert_cb, UNSELECTABLE },
    { "insertColumn",   fe_editor_table_column_insert_cb, UNSELECTABLE },
    { "insertCell",     fe_editor_table_cell_insert_cb, UNSELECTABLE },
    FE_MENU_END,
  FE_MENU_SEPARATOR,
  { "insertNewLineBreak",	fe_editor_insert_line_break_cb, SELECTABLE,        0,
    /*offset*/0, (void*)ED_BREAK_NORMAL },
  { "insertBreakBelow",	fe_editor_insert_line_break_cb, SELECTABLE,        0,
    /*offset*/0, (void*)ED_BREAK_BOTH },
  { "insertNonBreaking",	fe_editor_insert_non_breaking_space_cb,SELECTABLE,0 },
  FE_MENU_END,

  /* Properties menu */
  { "properties",   	fe_editor_properties_menu_cb,	SELECTABLE,	0 },
  { "textProperties",	fe_editor_properties_dialog_cb, SELECTABLE, 0, 0,
    (void*)XFE_EDITOR_PROPERTIES_CHARACTER },
  { "linkProperties",	fe_editor_properties_dialog_cb, SELECTABLE, 0, 0,
    (void*)XFE_EDITOR_PROPERTIES_LINK },
  { "targetProperties",	fe_editor_target_properties_dialog_cb, SELECTABLE,0,0,
    (void*)XFE_EDITOR_PROPERTIES_TARGET },
  { "imageProperties",	fe_editor_properties_dialog_cb, SELECTABLE, 0, 0,
    (void*)XFE_EDITOR_PROPERTIES_IMAGE },
  { "tableProperties",	fe_editor_table_properties_dialog_cb, SELECTABLE,0,0,
    (void*)XFE_EDITOR_PROPERTIES_TABLE },
  { "hruleProperties",	fe_editor_properties_dialog_cb, SELECTABLE, 0, 0,
    (void*)XFE_EDITOR_PROPERTIES_HRULE },
  { "tagProperties",	fe_editor_html_properties_dialog_cb, SELECTABLE, 0,0,
    (void*)XFE_EDITOR_PROPERTIES_HTML_TAG },
  { "documentProperties",fe_editor_document_properties_dialog_cb,
    SELECTABLE, 0, 0, (void*)XFE_EDITOR_PROPERTIES_DOCUMENT },
  FE_MENU_SEPARATOR,
  { "charProperties",	fe_editor_char_props_menu_cb, CASCADE,          0 },
    { "bold",          fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_BOLD },
    { "italic",        fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_ITALIC },
    { "underline",     fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_UNDERLINE },
    { "fixed",         fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_FIXED },
    { "superscript",   fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_SUPER },
    { "subscript",     fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_SUB },
    { "strikethrough", fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_STRIKEOUT },
    { "blink",         fe_editor_toggle_char_props_cb, TOGGLE, 0, 0, (void*)TF_BLINK },
    FE_MENU_SEPARATOR,
    { "textColor",     fe_editor_set_colors_dialog_cb, SELECTABLE },
    { "defaultColor",  fe_editor_default_color_cb, SELECTABLE },
    FE_MENU_SEPARATOR,
    { "serverJavaScript", fe_editor_char_props_cb, SELECTABLE, 0, 0, (void*)TF_SERVER },
    { "clientJavaScript", fe_editor_char_props_cb, SELECTABLE, 0, 0, (void*)TF_SCRIPT },
    FE_MENU_SEPARATOR,
    { "clearAllStyles",fe_editor_clear_char_props_cb, SELECTABLE, 0, 0, (void*)TF_NONE },
    FE_MENU_END,
  { "fontSize",	       fe_editor_font_size_menu_cb, CASCADE,             0 },
    { "minusTwo",  fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_MINUS_TWO },
    { "minusOne",  fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_MINUS_ONE },
    { "plusZero",  fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_ZERO },
    { "plusOne",   fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_PLUS_ONE },
    { "plusTwo",   fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_PLUS_TWO },
    { "plusThree", fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_PLUS_THREE },
    { "plusFour",  fe_editor_font_size_cb, RADIO, 0, 0, (void*)ED_FONTSIZE_PLUS_FOUR },
    FE_MENU_END,
  { "paragraphProperties", fe_editor_paragraph_props_menu_cb, CASCADE,    0 },
    { "normal", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_PARAGRAPH },
    { "headingOne", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_HEADER_1 },
    { "headingTwo", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_HEADER_2  },
    { "headingThree", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_HEADER_3 },
    { "headingFour", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_HEADER_4 },
    { "headingFive", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_HEADER_5 },
    { "headingSix", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_HEADER_6 },
    { "address", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_ADDRESS },
    { "formatted", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_PREFORMAT },
    { "listItem", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_LIST_ITEM },
    { "descriptionItem", fe_editor_paragraph_props_cb, RADIO, 0, 0,(void*)P_DESC_TITLE },
    { "descriptionText", fe_editor_paragraph_props_cb, RADIO, 0, 0, (void*)P_DESC_TEXT },
    FE_MENU_SEPARATOR,
    { "indent", fe_editor_indent_cb, SELECTABLE, 0, 0, (XtPointer)TRUE },
    { "outdent", fe_editor_indent_cb, SELECTABLE, 0, 0, (XtPointer)FALSE },
    FE_MENU_END,
  FE_MENU_END,

  { "options",   	fe_editor_options_menu_cb,	SELECTABLE,	0 },
  { "generalPrefs",	fe_general_prefs_cb,	SELECTABLE,		0 },
  { "editorPrefs",	fe_editor_preferences_dialog_cb, SELECTABLE,    0 },
  { "mailNewsPrefs",	fe_mailnews_prefs_cb,	SELECTABLE,		0 },
  { "networkPrefs",	fe_network_prefs_cb,	SELECTABLE,		0 },
#ifdef HAVE_SECURITY /* jwz */
  { "securityPrefs",	fe_security_prefs_cb,	SELECTABLE,		0 },
#endif /* HAVE_SECURITY -- jwz */
  FE_MENU_SEPARATOR,
  { "showMenubar",	fe_show_menubar_cb,	TOGGLE,	OFF(show_menubar_p)  },
  { "showToolbar",	fe_show_toolbar_cb,	TOGGLE,	OFF(show_toolbar_p)  },
  { "showCharacter",	fe_editor_show_character_toolbar_cb, TOGGLE, 0 },
  { "showParagraph",    fe_editor_show_paragraph_toolbar_cb, TOGGLE, 0 },
#ifdef JAVA
  { "showJavaConsole",  fe_show_java_console_cb,TOGGLE,
    OFF(show_java_console_p), OFF(show_java) },
#endif
#if 0
#ifdef HAVE_SECURITY
  { "showSecurityBar",  fe_show_security_bar_cb,TOGGLE,
						OFF(show_security_bar_p)  },
#endif
#endif
  { "autoLoadImages",   fe_toggle_loads_cb,     TOGGLE,
    OFF(autoload_images_p)},
#if defined(DEBUG_jwz) && defined(MOCHA)
    { "toggleJS",   fe_toggle_js_cb,   TOGGLE, 0, OFF(toggleJS_menuitem) },
    { "toggleAnim", fe_toggle_anim_cb, TOGGLE, 0, OFF(toggleAnim_menuitem) },
    { "toggleLoops", fe_toggle_loops_cb, TOGGLE, 0,OFF(toggleLoops_menuitem) },
#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
    { "toggleSizes", fe_toggle_font_size_changes_enabled_cb, TOGGLE,
      0,OFF(toggleSizes_menuitem) },
#endif /* DEBUG_jwz */

  FE_MENU_SEPARATOR,
  { "docEncoding",	0,			INTL_CASCADE,
						(void *) fe_doc_encoding },
  FE_MENU_SEPARATOR,
  { "saveOptions",      fe_save_options_cb,     SELECTABLE,		0 },

#if 0 /* This doesn't work real well - Motif bugs apparently. */
  FE_MENU_SEPARATOR,
  { "changeVisual",	fe_ChangeVisualCallback,SELECTABLE,		0 },
#endif
  FE_MENU_END,

  /* Windows Menu */
  { "windows",   	fe_editor_windows_menu_cb, CASCADE,		0,
      OFF(windows_menu)},
  { "openMail",    	fe_open_mail,		TOGGLE,			0 },
  { "openNews",	        fe_open_news,		TOGGLE,			0 },
#if 0
  { "siteManager",	fe_open_news,		TOGGLE,			0 },
#endif
  FE_MENU_SEPARATOR,
  { "editAddressBook",  fe_view_addressbook_cb,	TOGGLE,			0 },
  { "viewBookmark",	fe_view_bookmark_cb,	TOGGLE,			0 },
  FE_MENU_SEPARATOR,
  /* Open windows list is added here */
  FE_MENU_END,

  /* Help Menu */
  FE_MENU_BEGIN("help"),
  { "about",		fe_editor_about_cb,	SELECTABLE,		0 },
  FE_MENU_END,

  /* End of Menu Bar */
  FE_MENU_END
};

#define EOFF(x) OFF(editor.x)

struct fe_button fe_editor_toolbar [] =
{
  { "editorNewBlank", fe_editor_new_cb, SELECTABLE,             0 },
  { "editorOpenFile", fe_editor_open_file_cb, SELECTABLE,             0 },
  { "save",	fe_editor_save_cb,      SELECTABLE,		0 },
  FE_MENU_SEPARATOR,
  { "editorBrowse",fe_editor_browse_doc_cb,SELECTABLE,0,EOFF(toolbar_browse) },
  FE_MENU_SEPARATOR,
  { "cut", 	fe_editor_cut_cb, UNSELECTABLE, 0, EOFF(toolbar_cut) },
  { "copy", 	fe_editor_copy_cb, UNSELECTABLE, 0, EOFF(toolbar_copy) },
  { "paste",	fe_editor_paste_cb, SELECTABLE, 0, EOFF(toolbar_paste)},
  FE_MENU_SEPARATOR,
  { "print",	fe_print_cb,		SELECTABLE,       	0 },
  { "find",	fe_editor_find_cb,	SELECTABLE,		0 },
  FE_MENU_SEPARATOR,
  { "publish",	fe_editor_publish_cb, SELECTABLE, 0, EOFF(toolbar_publish) }
};
#undef EOFF
#endif /* EDITOR */

/* Forward declaration: defined in mailnews.c */
extern void fe_MsgSensitizeSubmenu(Widget widget, XtPointer closure,
				   XtPointer call_data);

static struct fe_button fe_mail_menubar[] = 
{
    { "file",		0,			SELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
    { "mailNew",	fe_mailNew_cb,		SELECTABLE,		0 },
    { "newFolder",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "openFolder",	fe_msg_command,		SELECTABLE,		0 },
    { "saveMessageAs",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "getNewMail",	fe_msg_command,		SELECTABLE,		0 },
    { "deliverQueuedMessages",fe_msg_command,	SELECTABLE,		0 },
    { "emptyTrash",	fe_msg_command,		SELECTABLE,		0 },
    { "compressFolder",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
#ifdef DEBUG_jwz
    { "compressAllFolders",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
#endif
    { "print",		fe_print_cb,		SELECTABLE,		0,
      OFF(print_menuitem)},
/*    { "print",	fe_msg_command,		SELECTABLE,		0,
      OFF(print_menuitem)}, */
    { "",		0,			UNSELECTABLE,		0 },
    { "delete",		fe_delete_cb,		SELECTABLE,		0,
      OFF(delete_menuitem)},
    { "quit",		fe_QuitCallback,	SELECTABLE,		0 },

    { 0 },

    { "edit",   	0,			SELECTABLE,		0 },
    { "undo",		fe_undo_cb,		SELECTABLE,		0 },
    { "redo",		fe_redo_cb,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "cut",		fe_cut_cb,		SELECTABLE,		0,
      OFF(cut_menuitem)},
    { "copy",		fe_copy_cb,		SELECTABLE,		0,
      OFF(copy_menuitem)},
    { "paste",		fe_paste_cb,		SELECTABLE,		0,
      OFF(paste_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
    { "deleteMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "deleteFolder",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "selectThread",	fe_msg_command,		SELECTABLE,		0 },
    { "selectMarkedMessages", fe_msg_command,	SELECTABLE,		0 }, 
    { "selectAllMessages",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "find",		fe_find_cb,		SELECTABLE,		0 },
    { "findAgain",	fe_find_again_cb,	SELECTABLE,		0,
      OFF(findAgain_menuitem)},

    { 0 },

    { "view",   	0,			SELECTABLE,		0 },
    { "sort",fe_MsgSensitizeSubmenu,		CASCADE,		0 },
      { "reSort",	fe_msg_command,		SELECTABLE,	    	0 },
      { "",		0,			UNSELECTABLE,		0 },
      { "threadMessages",fe_msg_command,	TOGGLE,		    	0 },
      { "",		0,			UNSELECTABLE,		0 },
      { "sortBackward",	fe_msg_command,		TOGGLE,			0 },
      { "",		0,			UNSELECTABLE,		0 },
      { "sortByDate",	fe_msg_command,		RADIO,			0 },
      { "sortBySubject",fe_msg_command,		RADIO,			0 },
      { "sortBySender",	fe_msg_command,		RADIO,			0 },
      { "sortByMessageNumber",fe_msg_command,	RADIO,			0 },
      { 0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "rot13Message",	fe_msg_command,		TOGGLE,			0 },
    { "wrapLongLines",	fe_msg_command,		TOGGLE,			0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "reload",		fe_reload_cb,		SELECTABLE,		0 },
    { "loadImages",	fe_load_images_cb,	SELECTABLE,		0,
      OFF(delayed_menuitem)},
    { "refresh",	fe_refresh_cb,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "source",		fe_view_source_cb,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "attachmentsInline",  fe_msg_command,	RADIO,			0 },
    { "attachmentsAsLinks", fe_msg_command,	RADIO, 			0 },
    { 0 },

    { "message",   	0,			SELECTABLE,		0 },
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "replyToAll",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessageQuoted", fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "markSelectedRead",fe_msg_command,	SELECTABLE,		0 },
    { "markSelectedUnread",fe_msg_command,	SELECTABLE,		0 },
#ifdef DEBUG_jwz
    { "markThreadRead",	fe_msg_command,		SELECTABLE,		0 },
#endif /* DEBUG_jwz */
    { "",		0,			UNSELECTABLE,		0 },
    { "markSelectedMessages",	fe_msg_command,	SELECTABLE,		0 },
    { "unmarkSelectedMessages",	fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "editAddress",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "moveMessagesInto",0,			CASCADE,  		0,
      OFF(move_selected_to_folder)},
      { 0 },
    { "copyMessagesInto",0,			CASCADE,		0,
      OFF(copy_selected_to_folder)},
      { 0 },

    { 0 },

    { "go",	   	0,			SELECTABLE,		0 },
    { "nextMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "previousMessage",fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "firstUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "nextUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "previousUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "firstMarkedMessage",fe_msg_command,	SELECTABLE,		0 },
    { "nextMarkedMessage",fe_msg_command,	SELECTABLE,		0 },
    { "previousMarkedMessage",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "abort",		fe_abort_cb,		UNSELECTABLE,		0,
      OFF(abort_menuitem)},
    { 0 },

    { "options",	0,			SELECTABLE,		0 },
    { "generalPrefs",    fe_general_prefs_cb,   SELECTABLE,             0 },
    { "mailNewsPrefs",   fe_mailnews_prefs_cb,  SELECTABLE,             0 },
    { "networkPrefs",    fe_network_prefs_cb,   SELECTABLE,             0 },
#ifdef HAVE_SECURITY /* jwz */
    { "securityPrefs",   fe_security_prefs_cb,  SELECTABLE,             0 },
#endif /* HAVE_SECURITY -- jwz */
    { "",		0,			UNSELECTABLE,		0 },
    { "showAllMessages",fe_msg_command,		RADIO,			0 },
    { "showOnlyUnread",	fe_msg_command,		RADIO, 			0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "showHeaders",	fe_MsgSensitizeSubmenu,	CASCADE, 		0 },
      { "showAllHeaders", fe_msg_command,	RADIO,	  		0 },
      { "showSomeHeaders", fe_msg_command,	RADIO,	  		0 },
      { "showMicroHeaders", fe_msg_command,	RADIO,	  		0 },
      { 0 },

#ifdef DEBUG_jwz
    { "",		 0,			UNSELECTABLE,		0 },
    { "autoLoadImages", fe_toggle_loads_cb,   TOGGLE, OFF(autoload_images_p) },
# ifdef MOCHA
    { "toggleJS",   fe_toggle_js_cb,   TOGGLE, 0, OFF(toggleJS_menuitem) },
    { "toggleAnim", fe_toggle_anim_cb, TOGGLE, 0, OFF(toggleAnim_menuitem) },
    { "toggleLoops", fe_toggle_loops_cb, TOGGLE, 0,OFF(toggleLoops_menuitem) },
# endif /* MOCHA */
#endif /* DEBUG_jwz */

#ifdef DEBUG_jwz
    { "toggleSizes", fe_toggle_font_size_changes_enabled_cb, TOGGLE,
      0,OFF(toggleSizes_menuitem) },
#endif /* DEBUG_jwz */

    { "",		0,			UNSELECTABLE,		0 },
    { "docEncoding",	0,			INTL_CASCADE,
						(void *) fe_doc_encoding },
    { "",		0,			UNSELECTABLE,		0 },
    { "saveOptions",    fe_save_options_cb,     SELECTABLE,		0 },

    { 0 },

    { "windows",   	0,			SELECTABLE,		0,
      OFF(windows_menu)},
/* Order matters here. Check with fe_GenerateWindowsMenu() */
    { "openMail",	fe_open_mail,		TOGGLE,			0 },
    { "openNews",	fe_open_news,		TOGGLE,			0 },
#ifdef DEBUG_jwz
    { "",		 0,			UNSELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
#endif /* DEBUG_jwz */
    { "",		 0,			UNSELECTABLE,		0 },
    { "editAddressBook",fe_msg_command,		TOGGLE,			0 },
    { "viewBookmark",	fe_view_bookmark_cb,	TOGGLE,			0 },
    { "",		 0,			UNSELECTABLE,		0 },
    { 0 },

    /*
     * Since customers can configure the help menu using
     * e-kit, the directory menu is added by the e-kit at run-time.
     * See e_kit.c to add menu items, or e_kit.ad to change URLs.
     * The 'about' item is left here, since that *always* appears in
     * the help menu.  The customer cannot make it disappear.
     */
    { "help",   	0,			SELECTABLE,		0 },
    { "about",		fe_about_cb,		SELECTABLE,		0 },
    { 0 },
    { 0 }
};


static struct fe_button fe_news_menubar[] = 
{
    { "file",		0,			SELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
    { "mailNew",	fe_mailNew_cb,		SELECTABLE,		0 },
    { "postNew",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "saveMessageAs",	fe_msg_command,		SELECTABLE,		0 },
    { "openNewsHost",	fe_msg_command,		SELECTABLE,		0 },
    { "deleteNewsHost",	fe_msg_command,		SELECTABLE,		0 },
    { "addNewsgroup",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "getMoreMessages",fe_msg_command,		SELECTABLE,		0 },
#if NEWS_GET_ALL_MSGS
    { "getAllMessages",	fe_msg_command,		SELECTABLE,		0 },
#endif /* NEWS_GET_ALL_MSGS */
    { "deliverQueuedMessages",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "print",		fe_print_cb,		SELECTABLE,		0,
      OFF(print_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
    { "delete",		fe_delete_cb,		SELECTABLE,		0,
      OFF(delete_menuitem)},
    { "quit",		fe_QuitCallback,	SELECTABLE,		0 },

    { 0 },

    { "edit",   	0,			SELECTABLE,		0 },
    { "undo",		fe_undo_cb,		SELECTABLE,		0 },
    { "redo",		fe_redo_cb,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "cut",		fe_cut_cb,		SELECTABLE,		0,
      OFF(cut_menuitem)},
    { "copy",		fe_copy_cb,		SELECTABLE,		0,
      OFF(copy_menuitem)},
    { "paste",		fe_paste_cb,		SELECTABLE,		0,
      OFF(paste_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
    { "selectThread",	fe_msg_command,		SELECTABLE,		0 },
    { "selectMarkedMessages", fe_msg_command,	SELECTABLE,		0 }, 
    { "selectAllMessages",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "find",		fe_find_cb,		SELECTABLE,		0 },
    { "findAgain",	fe_find_again_cb,	SELECTABLE,		0,
      OFF(findAgain_menuitem)},
    { "",		0,			UNSELECTABLE,		0 },
    { "cancelMessage",	fe_msg_command,		SELECTABLE,		0 },

    { 0 },

    { "view",   	0,			SELECTABLE,		0 },
    { "sort",fe_MsgSensitizeSubmenu,		CASCADE,		0 },
      { "reSort",	fe_msg_command,		SELECTABLE,	    	0 },
      { "",		0,			UNSELECTABLE,		0 },
      { "threadMessages",fe_msg_command,	TOGGLE,		    	0 },
      { "",		0,			UNSELECTABLE,		0 },
      { "sortBackward",	fe_msg_command,		TOGGLE,			0 },
      { "",		0,			UNSELECTABLE,		0 },
      { "sortByDate",	fe_msg_command,		RADIO,			0 },
      { "sortBySubject",fe_msg_command,		RADIO,			0 },
      { "sortBySender",	fe_msg_command,		RADIO,			0 },
      { "sortByMessageNumber",fe_msg_command,	RADIO,			0 },
      { 0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "rot13Message",	fe_msg_command,		TOGGLE,			0 },
    { "wrapLongLines",	fe_msg_command,		TOGGLE,			0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "reload",		fe_reload_cb,		SELECTABLE,		0 },
    { "loadImages",	fe_load_images_cb,	SELECTABLE,		0,
      OFF(delayed_menuitem)},
    { "refresh",	fe_refresh_cb,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "source",		fe_view_source_cb,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "attachmentsInline",  fe_msg_command,	RADIO,			0 },
    { "attachmentsAsLinks", fe_msg_command,	RADIO, 			0 },
    { 0 },

    { "message",   	0,			SELECTABLE,		0 },
    { "postReply",	fe_msg_command,		SELECTABLE,		0 },
    { "postAndMailReply",fe_msg_command,	SELECTABLE,		0 },
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessageQuoted", fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "markSelectedRead",fe_msg_command,	SELECTABLE,		0 },
    { "markSelectedUnread",fe_msg_command,	SELECTABLE,		0 },
    { "markThreadRead",	fe_msg_command,		SELECTABLE,		0 },
    { "markAllRead",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "markSelectedMessages",	fe_msg_command,	SELECTABLE,		0 },
    { "unmarkSelectedMessages",	fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "editAddress",	fe_msg_command,		SELECTABLE,		0 },

    { 0 },

    { "go",	   	0,			SELECTABLE,		0 },
    { "nextMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "previousMessage",fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "firstUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "nextUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "previousUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "firstMarkedMessage",fe_msg_command,	SELECTABLE,		0 },
    { "nextMarkedMessage",fe_msg_command,	SELECTABLE,		0 },
    { "previousMarkedMessage",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "abort",		fe_abort_cb,		UNSELECTABLE,		0,
      OFF(abort_menuitem)},
    { 0 },

    { "options",	0,			SELECTABLE,		0 },
    { "generalPrefs",    fe_general_prefs_cb,   SELECTABLE,             0 },
    { "mailNewsPrefs",   fe_mailnews_prefs_cb,  SELECTABLE,             0 },
    { "networkPrefs",    fe_network_prefs_cb,   SELECTABLE,             0 },
#ifdef HAVE_SECURITY /* jwz */
    { "securityPrefs",   fe_security_prefs_cb,  SELECTABLE,             0 },
#endif /* HAVE_SECURITY -- jwz */
    { "",		0,			UNSELECTABLE,		0 },
    { "showSubscribedNewsGroups",fe_msg_command,RADIO,	 		0 },
    { "showActiveNewsGroups",fe_msg_command,	RADIO, 			0 },
    { "showAllNewsGroups",   fe_msg_command,	RADIO, 			0 },
    { "checkNewNewsGroups",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "showAllMessages",fe_msg_command,		RADIO, 			0 },
    { "showOnlyUnread",	fe_msg_command,		RADIO,	 		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "showHeaders",	fe_MsgSensitizeSubmenu,	CASCADE, 		0 },
      { "showAllHeaders", fe_msg_command,	RADIO,	  		0 },
      { "showSomeHeaders", fe_msg_command,	RADIO,	  		0 },
      { "showMicroHeaders", fe_msg_command,	RADIO,	  		0 },
      { 0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "addFromNewest",	fe_msg_command,		RADIO,			0 },
    { "addFromOldest",	fe_msg_command,		RADIO,			0 },

#ifdef DEBUG_jwz
    { "",		 0,			UNSELECTABLE,		0 },
    { "autoLoadImages", fe_toggle_loads_cb,   TOGGLE, OFF(autoload_images_p) },
# ifdef MOCHA
    { "toggleJS",   fe_toggle_js_cb,   TOGGLE, 0, OFF(toggleJS_menuitem) },
    { "toggleAnim", fe_toggle_anim_cb, TOGGLE, 0, OFF(toggleAnim_menuitem) },
    { "toggleLoops", fe_toggle_loops_cb, TOGGLE, 0,OFF(toggleLoops_menuitem) },
# endif /* MOCHA */
#endif /* DEBUG_jwz */

#ifdef DEBUG_jwz
    { "toggleSizes", fe_toggle_font_size_changes_enabled_cb, TOGGLE,
      0,OFF(toggleSizes_menuitem) },
#endif /* DEBUG_jwz */

    { "",		0,			UNSELECTABLE,		0 },
    { "docEncoding",	0,			INTL_CASCADE,
						(void *) fe_doc_encoding },
    { "",		0,			UNSELECTABLE,		0 },
    { "saveOptions",    fe_save_options_cb,     SELECTABLE,		0 },

    { 0 },

    { "windows",   	 0,			SELECTABLE,		0,
      OFF(windows_menu)},
/* Order matters here. Check with fe_GenerateWindowsMenu() */
    { "openMail",	fe_open_mail,		TOGGLE,			0 },
    { "openNews",	fe_open_news,		TOGGLE,			0 },
#ifdef DEBUG_jwz
    { "",		 0,			UNSELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
#endif /* DEBUG_jwz */
    { "",		 0,			UNSELECTABLE,		0 },
    { "editAddressBook",fe_msg_command,		TOGGLE,			0 },
    { "viewBookmark",	fe_view_bookmark_cb,	TOGGLE,			0 },
    { "",		 0,			UNSELECTABLE,		0 },
    { 0 },

    /*
     * Since customers can configure the help menu using
     * e-kit, the directory menu is added by the e-kit at run-time.
     * See e_kit.c to add menu items, or e_kit.ad to change URLs.
     * The 'about' item is left here, since that *always* appears in
     * the help menu.  The customer cannot make it disappear.
     */
    { "help",   	0,			SELECTABLE,		0 },
    { "about",		fe_about_cb,		SELECTABLE,		0 },
    { 0 },
    { 0 }
};


static struct fe_button fe_compose_menubar [] =
{
  { "file",		0,			SELECTABLE,		0 },
  { "sendOrSendLater",	fe_msg_command,		SELECTABLE,		0 },
  { "",		 	0,			UNSELECTABLE,		0 },
  { "attachFile",	fe_mailto_attach_cb,	SELECTABLE,		0 },
  { "quoteMessage",	fe_msg_command,		SELECTABLE,		0 },
  { "",		 	0,			UNSELECTABLE,		0 },
  { "saveMessageAs",	0,			UNSELECTABLE,		0 },
  { "",		 	0,			UNSELECTABLE,		0 },
  { "print",		fe_print_cb,		SELECTABLE,		0 },
  { "",		 	0,			UNSELECTABLE,		0 },
  { "delete",		fe_delete_cb,		SELECTABLE,		0,
    OFF(delete_menuitem)},
  { 0 },

  { "edit",   		0,			SELECTABLE,		0 },
  { "undo",		fe_undo_cb,		SELECTABLE,		0 },
  { "",			0,			UNSELECTABLE,		0 },
  { "cut",		fe_cut_cb,		SELECTABLE,		0,
    OFF(cut_menuitem)},
  { "copy",		fe_copy_cb,		SELECTABLE,		0,
    OFF(copy_menuitem)},
  { "paste",		fe_paste_cb,		SELECTABLE,		0,
    OFF(paste_menuitem)},
  { "pasteQuote",	fe_paste_quoted_cb,	SELECTABLE,		0,
    OFF(paste_quoted_menuitem) },
  { "clearAllText",	fe_mailcompose_cb,	SELECTABLE,		0,
	0,		(void *)ComposeClearAllText_cb },
  { "",			0,			UNSELECTABLE,		0 },
  { "selectAllText",	fe_mailcompose_cb,	SELECTABLE,		0,
	0,		(void *)ComposeSelectAllText_cb },
#ifdef COMPOSE_ROT13
  { "",			0,			UNSELECTABLE,		0 },
  { "rot13",		0,			UNSELECTABLE,		0 },
#endif /* COMPOSE_ROT13 */
  { 0 },

  { "view",	   	0,			SELECTABLE,		0 },
  { "showAllHeaders",	fe_msg_command,		TOGGLE,		  	0 },
  { "",			0,			UNSELECTABLE,		0 },
  { "showFrom",		fe_msg_command,		TOGGLE,			0 },
  { "showReplyTo",	fe_msg_command,		TOGGLE,			0 },
  { "showTo",		fe_msg_command,		TOGGLE,			0 },
  { "showCC",		fe_msg_command,		TOGGLE,			0 },
  { "showBCC",		fe_msg_command,		TOGGLE,			0 },
  { "showFCC",		fe_msg_command,		TOGGLE,			0 },
  { "showPostTo",	fe_msg_command,		TOGGLE,			0 },
  { "showFollowupTo",	fe_msg_command,		TOGGLE,			0 },
  { "showSubject",	fe_msg_command,		TOGGLE,			0 },
  { "showAttachments",	fe_msg_command,		TOGGLE,			0 },
  { "",			0,			UNSELECTABLE,		0 },
  { "wrapLines",	fe_toggle_compose_wrap_cb,	TOGGLE,
						OFF(compose_wrap_lines_p) },
  { 0 },

  { "options",	   	0,			SELECTABLE,		0 },
  { "mailNewsPrefs",	 fe_mailnews_prefs_cb,	SELECTABLE,		0 },
  { "",			0,			UNSELECTABLE,		0 },
  { "sendEncrypted",	fe_msg_command,		TOGGLE,			0 },
  { "sendSigned",	fe_msg_command,		TOGGLE,			0 },
  { "",			0,			UNSELECTABLE,		0 },
  { "deliverNow",	fe_handle_delivery_cb,	TOGGLE,		  	0,
    OFF(deliverNow_menuitem)},
  { "deliverLater",	fe_handle_delivery_cb,	TOGGLE,		  	0,
    OFF(deliverLater_menuitem)},
  { 0 },

  { "windows",   	 0,			SELECTABLE,		0,
    OFF(windows_menu)},
/* Order matters here. Check with fe_GenerateWindowsMenu() */
  { "openMail",		fe_open_mail,		TOGGLE,			0 },
  { "openNews",		fe_open_news,		TOGGLE,			0 },
#ifdef DEBUG_jwz
    { "",		 0,			UNSELECTABLE,		0 },
    { "openBrowser",	fe_new_cb,		SELECTABLE,		0 },
#endif /* DEBUG_jwz */
  { "",		 	0,			UNSELECTABLE,		0 },
  { "editAddressBook",	fe_msg_command,		TOGGLE,			0 },
  { "viewBookmark",	fe_view_bookmark_cb,	TOGGLE,			0 },
  { "",		 	0,			UNSELECTABLE,		0 },
  { 0 },
  { 0 }
};

static struct fe_button fe_web_toolbar [] = 
{
  { "back",		fe_back_cb,		UNSELECTABLE,	0 },
  { "forward",		fe_forward_cb,		UNSELECTABLE,	0 },
  { "home",		fe_home_cb,		UNSELECTABLE,	0 },
  { "",			0,			UNSELECTABLE,	0 },
#if defined(EDITOR) && defined(EDITOR_UI)
  { "editDocument",       fe_editor_edit_cb,      SELECTABLE,     0 },
  { "",                 0,                      UNSELECTABLE,	0 },
#endif /* EDITOR */
  { "reload",		fe_reload_cb,		SELECTABLE,	0 },
  { "loadImages",	fe_load_images_cb,	SELECTABLE,	0 },
  { "openURL",		fe_open_url_cb,		SELECTABLE,	0 },
  { "print",		fe_print_cb,		SELECTABLE,	0 },
  { "find",		fe_find_cb,		SELECTABLE,	0 },
  { "",			0,			UNSELECTABLE,	0 },
  { "abort",		fe_abort_cb,		UNSELECTABLE,	0 },
  { "Netscape",		fe_netscape_cb,		SELECTABLE,	0 },
};

 /*
  * fe_directory_buttons used to be here, but:
  * Since customers can configure the directory buttons using
  * e-kit, the directory buttons are added by the e-kit at run-time.
  * See e_kit.c to add directory buttons, or e_kit.ad to change URLs.
  */

static struct fe_button fe_mail_toolbar [] = 
{
    { "getNewMail",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "deleteMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "mailNew",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "replyToAll",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "previousUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "nextUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
#ifdef DEBUG_jwz
    { "markThreadRead",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
#endif /* DEBUG_jwz */
    { "print",		fe_print_cb,		SELECTABLE,		0 },
    { "abort",		fe_abort_cb,		UNSELECTABLE,		0 }
};


static struct fe_button fe_news_toolbar [] = 
{
    { "postNew",	fe_msg_command,		SELECTABLE,		0 },
    { "mailNew",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "postReply",	fe_msg_command,		SELECTABLE,		0 },
    { "postAndMailReply",fe_msg_command,	SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "previousUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "nextUnreadMessage",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "markThreadRead",	fe_msg_command,		SELECTABLE,		0 },
    { "markAllRead",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "print",		fe_print_cb,		SELECTABLE,		0 },
    { "abort",		fe_abort_cb,		UNSELECTABLE,		0 }
};

static struct fe_button fe_compose_toolbar [] = 
{
    { "sendOrSendLater",fe_msg_command,		SELECTABLE,		0 },
    { "quoteMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "attachFile",	fe_mailto_attach_cb,	SELECTABLE,		0 },
    { "editAddressBook",fe_msg_command,		SELECTABLE,		0 },
#ifndef NO_XFE_SA_ICON
    { "securityAdvisor",fe_msg_command,		SELECTABLE,		0 },
#endif /* !NO_XFE_SA_ICON */
    { "",		0,			UNSELECTABLE,		0 },
    { "abort",		fe_abort_cb,		UNSELECTABLE,		0 }
};


/* rlogin total crap
 */

void
FE_ConnectToRemoteHost (MWContext *context, int url_type, char *hostname,
			char *port, char *username)
{
  char *name;
  const char *command;
  const char *user_command;
  char *buf;
  const char *in;
  char *out;
  char **av, **ac, **ae;
  int na;
  Boolean cop_out_p = False;
  pid_t forked;

  switch (url_type)
    {
    case FE_TELNET_URL_TYPE:
      name = "telnet";
      command = fe_globalPrefs.telnet_command;
      user_command = 0 /* fe_globalPrefs.telnet_user_command */;
      break;
    case FE_TN3270_URL_TYPE:
      name = "tn3270";
      command = fe_globalPrefs.tn3270_command;
      user_command = 0 /* fe_globalPrefs.tn3270_user_command */;
      break;
    case FE_RLOGIN_URL_TYPE:
      name = "rlogin";
      command = fe_globalPrefs.rlogin_command;
      user_command = fe_globalPrefs.rlogin_user_command;
      break;
    default:
      abort ();
    }

  if (username && user_command && *user_command)
    command = user_command;
  else if (username)
    cop_out_p = True;

  buf = (char*) malloc( strlen(command) + 1
    + (hostname && *hostname ? strlen(hostname) : strlen("localhost"))
    + (username && *username ? strlen(username) : 0)
    + (port && *port ? strlen(port) : 0) );
  ac = av = (char**) malloc(10 * sizeof (char*));
  if (buf == NULL || av == NULL)
    goto malloc_lossage;
  ae = av + 10;
  in = command;
  *ac = out = buf;
  na = 0;
  while (*in)
    {
      if (*in == '%')
	{
	  in++;
	  if (*in == 'h')
	    {
	      char *s;
	      char *h = (hostname && *hostname ? hostname : "localhost");

	      /* Only allow the hostname to contain alphanumerics or
		 _ - and . to prevent people from embedding shell command
		 security holes in the host name. */
	      for (s = h; *s; s++)
		if (*s == '_' || (*s == '-' && s != h) || *s == '.' ||
		    isalpha(*s) || isdigit(*s))
		  *out++ = *s;
	    }
	  else if (*in == 'p')
	    {
	      if (port && *port)
		{
		  short port_num = atoi(port);

		  if (port_num > 0)
		    {
		      char buf1[6];
		      PR_snprintf (buf1, sizeof (buf1), "%.5d", port_num);
		      strcpy(out, buf1);
		      out += strlen(buf1);
		    }
		}
	    }
	  else if (*in == 'u')
	    {
	      char *s;
	      /* Only allow the user name to contain alphanumerics or
		 _ - and . to prevent people from embedding shell command
		 security holes in the host name. */
	      if (username && *username)
		{
		  for (s = username; *s; s++)
		    if (*s == '_' || (*s == '-' && s != username) ||
			*s == '.' || isalpha(*s) || isdigit(*s))
		      *out++ = *s;
		}
	    }
	  else if (*in == '%')
	    {
	      *out++ = '%';
	    }
	  else
	    {
	      char buf2 [255];
	      PR_snprintf (buf2, sizeof (buf2),
				 XP_GetString( XFE_UNKNOWN_ESCAPE_CODE ), name, *in);
	      FE_Alert (context, buf2);
	    }
	  if (*in)
	    in++;
	}
      else if (*in == ' ')
	{
	  if (out != *ac)
	    {
	      *out++ = '\0';
	      na++;
	      if (++ac == ae)
		{
		  av = (char**) realloc(av, (na + 10) * sizeof (char*));
		  if (av == NULL)
		    goto malloc_lossage;
		  ac = av + na;
		  ae = ac + 10;
		}
	      *ac = out;
	    }
	  in++;
	}
      else
	{
	  *out = *in;
	  out++;
	  in++;
	}
    }

  if (out != *ac)
    {
      *out = '\0';
      na++;
      ac++;
    }
  if (ac == ae)
    {
      av = (char**) realloc(av, (na + 1) * sizeof (char*));
      if (av == NULL)
	goto malloc_lossage;
      ac = av + na;
    }
  *ac = 0;

  if (cop_out_p)
    {
      char buf2 [1024];
      PR_snprintf (buf2, sizeof (buf2), XP_GetString(XFE_LOG_IN_AS), username);
      fe_Message (context, buf2);
    }

  switch (forked = fork ())
    {
    case -1:
      fe_perror (context, XP_GetString( XFE_COULDNT_FORK ) );
      break;
    case 0:
      {
	Display *dpy = XtDisplay (CONTEXT_WIDGET (context));
	close (ConnectionNumber (dpy));

	execvp (av [0], av);

	PR_snprintf (buf, sizeof (buf), XP_GetString( XFE_EXECVP_FAILED ),
			fe_progname, av[0]);
	perror (buf);
	exit (1);	/* Note that this only exits a child fork.  */
	break;
      }
    default:
      /* This is the "old" process (subproc pid is in `forked'.) */
      break;
    }
  free(av);
  free(buf);
  return;

malloc_lossage:
  if (av) free(av);
  if (buf) free(buf);
  fe_Message (context, XP_GetString(XFE_OUT_OF_MEMORY_URL));
}


/* Making the widgets.
 */

extern void fe_MsgSensitizeMenubar(Widget widget, XtPointer closure,
				   XtPointer call_data);

Widget
fe_MakeMenubar (Widget parent, MWContext *context)
{
  struct fe_button* description = 
    (context->type == MWContextBrowser ? fe_web_menubar :
     context->type == MWContextMail ? fe_mail_menubar :
     context->type == MWContextNews ? fe_news_menubar :
     context->type == MWContextMessageComposition ? fe_compose_menubar : 
#ifdef EDITOR
     context->type == MWContextEditor ? fe_editor_menubar :
#endif
     0);

  Widget menubar;

  if (!description) return 0;

  menubar =
    fe_PopulateMenubar(parent, description,
		       context, context, CONTEXT_DATA(context),
		       fe_pulldown_cb);

  if ( context->type == MWContextBrowser ) {
      ekit_AddDirectoryMenu(menubar);
  }

  if ( context->type == MWContextBrowser ||
#if defined(EDITOR) && defined(EDITOR_UI)
       context->type == MWContextEditor ||
#endif
       context->type == MWContextMail ||
       context->type == MWContextNews ) {
      ekit_AddHelpMenu(menubar, context->type);
  }

  if (CONTEXT_DATA (context)->back_menuitem)
    XtVaSetValues (CONTEXT_DATA (context)->back_menuitem,
#ifdef NEW_FRAME_HIST
		   XmNsensitive, SHIST_CanGoBack (context), 0);
#else
		   XmNsensitive, SHIST_CanGoBack (&context->hist), 0);
#endif /* NEW_FRAME_HIST */
  if (CONTEXT_DATA (context)->forward_menuitem)
    XtVaSetValues (CONTEXT_DATA (context)->forward_menuitem,
#ifdef NEW_FRAME_HIST
		   XmNsensitive, SHIST_CanGoForward (context), 0);
#else
		   XmNsensitive, SHIST_CanGoForward (&context->hist), 0);
#endif /* NEW_FRAME_HIST */
  if (CONTEXT_DATA (context)->abort_menuitem)
    XtVaSetValues (CONTEXT_DATA (context)->abort_menuitem, XmNsensitive,
		   XP_IsContextStoppable (context), 0);
  if (CONTEXT_DATA (context)->home_menuitem)
    XtVaSetValues (CONTEXT_DATA (context)->home_menuitem, XmNsensitive,
      (fe_globalPrefs.home_document && *fe_globalPrefs.home_document), 0);

#if defined(DEBUG_jwz) && defined(MOCHA)
    if (CONTEXT_DATA (context)->toggleJS_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleJS_menuitem, XmNset,
      !fe_globalPrefs.disable_javascript, 0);
#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
    if (CONTEXT_DATA (context)->toggleAnim_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleAnim_menuitem, XmNset,
      IL_AnimationsEnabled, 0);
    if (CONTEXT_DATA (context)->toggleLoops_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleLoops_menuitem, XmNset,
      (IL_AnimationsEnabled && IL_AnimationLoopsEnabled), 0);
    if (CONTEXT_DATA (context)->toggleSizes_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleSizes_menuitem, XmNset,
      fe_ignore_font_size_changes_p, 0);
#endif /* DEBUG_jwz */

  return menubar;
}


Widget
fe_MakeToolbar (Widget parent, MWContext *context, Boolean directory_buttons_p)
{
  Widget toolbar;
  Widget kids [40];
  Arg av[20];
  int ac;
  int i, j;
  int icon_number = 0;

  struct fe_button *tb;
  int tb_size;

  if (directory_buttons_p)
    {
      tb = ekit_GetDirectoryButtons();
      tb_size = ekit_NumDirectoryButtons();
    }
  else
    {
      switch (context->type)
	{
	case MWContextBrowser:
	  tb = fe_web_toolbar;
	  tb_size = countof (fe_web_toolbar);
          /* If no custom animation, get rid of co-brand button */
          if ( anim_custom_large == NULL && anim_custom_small == NULL ) {
            tb_size--;
          }
	  break;
	case MWContextMail:
	  tb = fe_mail_toolbar;
	  tb_size = countof (fe_mail_toolbar);
	  break;
	case MWContextNews:
	  tb = fe_news_toolbar;
	  tb_size = countof (fe_news_toolbar);
	  break;
	case MWContextMessageComposition:
	  tb = fe_compose_toolbar;
	  tb_size = countof (fe_compose_toolbar);
	  break;
#ifdef EDITOR
	case MWContextEditor:
	  tb = fe_editor_toolbar;
	  tb_size = countof (fe_editor_toolbar);
	  break;
#endif /*EDITOR*/
	default:
	  abort ();
	  break;
	}
    }

  ac = 0;
  XtSetArg (av[ac], XmNskipAdjust, True); ac++;
  XtSetArg (av[ac], XmNseparatorOn, False); ac++;
  XtSetArg (av[ac], XmNorientation, XmHORIZONTAL); ac++;
  XtSetArg (av[ac], XmNpacking, XmPACK_TIGHT); ac++;
  toolbar = XmCreateRowColumn (parent,
			       (directory_buttons_p ? "urlBar" : "toolBar"),
			       av,ac);

  for (i = 0, j = 0; i < tb_size; i++)
    {
      char *name = tb[i].name;
      Widget b;
      ac = 0;

#ifdef __sgi
	if (( fe_VendorAnim && name && !strcmp (name, "newsgroups")) ||
	    (!fe_VendorAnim && name && !strcmp (name, "welcome")))
	  {
	    icon_number++;
	    continue;
	  }
#endif /* __sgi */

      if (!*name)
        {
	  XmString xms = XmStringCreateLtoR ("", XmFONTLIST_DEFAULT_TAG);
          XtSetArg (av[ac], XmNlabelType, XmSTRING); ac++;
          XtSetArg (av[ac], XmNlabelString, xms); ac++;
	  b = XmCreateLabelGadget (toolbar, "spacer", av, ac);
	  XmStringFree (xms);
	}
      else
	{
#ifdef HAVE_URL_ICONS
          XtSetArg (av[ac], XmNlabelType,
		    ((directory_buttons_p ||
		      CONTEXT_DATA (context)->show_toolbar_icons_p)
		     ? XmPIXMAP : XmSTRING)); ac++;
#else
          XtSetArg (av[ac], XmNlabelType,
		    ((!directory_buttons_p &&
		      CONTEXT_DATA (context)->show_toolbar_icons_p)
		     ? XmPIXMAP : XmSTRING)); ac++;
#endif
	  if (CONTEXT_DATA (context)->show_toolbar_icons_p)
	    {
	      Pixmap p =  fe_ToolbarPixmap (context, icon_number, False,
					    directory_buttons_p);
	      Pixmap ip = fe_ToolbarPixmap (context, icon_number, True,
					    directory_buttons_p);
	      if (! ip) ip = p;
	      XtSetArg (av[ac], XmNlabelPixmap, p); ac++;
	      XtSetArg (av[ac], XmNlabelInsensitivePixmap, ip); ac++;
	    }

	  /* Install userData if available */
	  if (tb[i].userdata) {
	    XtSetArg(av[ac], XmNuserData, tb[i].userdata); ac++;
	  }

#if 0
	  if (CONTEXT_DATA (context)->show_toolbar_icons_p)
	    /* Apparently "*toolBar.back.labelPixmap:" gets ignored if it's
	       a gadget instead of a widget... */
	    b = XmCreatePushButton (toolbar, name, av, ac);
	  else
#endif
	    b = XmCreatePushButtonGadget (toolbar, name, av, ac);

	  XtAddCallback (b, XmNactivateCallback, tb[i].callback, context);

	  if (tb[i].type == UNSELECTABLE)
	    XtVaSetValues (b, XtNsensitive, False, 0);
	  else if (tb[i].type != SELECTABLE)
	    abort ();
	  icon_number++;
	}

      if (tb[j].offset) {
	fe_ContextData *data = CONTEXT_DATA (context);
	Widget* foo = (Widget*)
	  (((char*) data) + (int) (tb[j].offset));
	*foo = b;
      }

      kids [j++] = b;

      if (!directory_buttons_p) {
	if (!strcmp (name, "back"))
	  CONTEXT_DATA (context)->back_button = b;
	else if (!strcmp (name, "forward"))
	  CONTEXT_DATA (context)->forward_button = b;
	else if (!strcmp (name, "home"))
	  CONTEXT_DATA (context)->home_button = b;
	else if (!strcmp ("loadImages", name))
	  CONTEXT_DATA (context)->delayed_button = b;
	else if (!strcmp ("print", name))
	  CONTEXT_DATA (context)->print_button = b;
	else if (!strcmp ("abort", name))
	  CONTEXT_DATA (context)->abort_button = b;
      }
    }

  if (!directory_buttons_p) {
    if (CONTEXT_DATA (context)->back_button)
      XtVaSetValues (CONTEXT_DATA (context)->back_button,
#ifdef NEW_FRAME_HIST
		   XmNsensitive, SHIST_CanGoBack (context), 0);
#else
		   XmNsensitive, SHIST_CanGoBack (&context->hist), 0);
#endif /* NEW_FRAME_HIST */
    if (CONTEXT_DATA (context)->forward_button)
      XtVaSetValues (CONTEXT_DATA (context)->forward_button,
#ifdef NEW_FRAME_HIST
		   XmNsensitive, SHIST_CanGoForward (context), 0);
#else
		   XmNsensitive, SHIST_CanGoForward (&context->hist), 0);
#endif /* NEW_FRAME_HIST */
    if (CONTEXT_DATA (context)->home_button)
      XtVaSetValues (CONTEXT_DATA (context)->home_button, XmNsensitive,
      (fe_globalPrefs.home_document && *fe_globalPrefs.home_document), 0);
    if (CONTEXT_DATA (context)->delayed_button)
      XtVaSetValues (CONTEXT_DATA (context)->delayed_button,
		   XmNsensitive, CONTEXT_DATA (context)->delayed_images_p, 0);
    if (CONTEXT_DATA (context)->abort_button)
      XtVaSetValues (CONTEXT_DATA (context)->abort_button, XmNsensitive,
		   XP_IsContextStoppable (context), 0);
  }

  XtManageChildren (kids, j);
  return toolbar;
}

static void
fe_SensitizeForSelectedFrame(MWContext *context)
{
  fe_ContextData* data = CONTEXT_DATA(context);
  Boolean anySelectedFrame;
  char *s;

  anySelectedFrame = fe_GetFocusGridOfContext (context) ? True : False;
    
  if (data->saveAs_menuitem) {
    if (anySelectedFrame)
      s = XP_GetString( XFE_SAVE_FRAME_AS );
    else
      s = XP_GetString( XFE_SAVE_AS );
    fe_SetString(data->saveAs_menuitem, XmNlabelString, s);
  }

  if (data->mailto_menuitem) {
    if (anySelectedFrame)
      s = XP_GetString( XFE_MAIL_FRAME );
    else
      s = XP_GetString( XFE_MAIL_DOCUMENT );
    fe_SetString(data->mailto_menuitem, XmNlabelString, s);
  }

  if (data->print_menuitem) {
    if (anySelectedFrame)
      s = XP_GetString( XFE_PRINT_FRAME );
    else
      s = XP_GetString( XFE_PRINT );
    fe_SetString(data->print_menuitem, XmNlabelString, s);
  }

  if (data->refresh_menuitem) {
    if (anySelectedFrame)
      s = XP_GetString( XFE_REFRESH_FRAME );
    else
      s = XP_GetString( XFE_REFRESH );
    fe_SetString(data->refresh_menuitem, XmNlabelString, s);
  }

  if (data->reloadFrame_menuitem)
    XtVaSetValues(data->reloadFrame_menuitem, XmNsensitive,
		    anySelectedFrame, 0);

  if (data->frameSource_menuitem)
    XtVaSetValues(data->frameSource_menuitem, XmNsensitive,
		    anySelectedFrame, 0);

  if (data->frameInfo_menuitem)
    XtVaSetValues(data->frameInfo_menuitem, XmNsensitive,
		    anySelectedFrame, 0);
}

void
fe_SensitizeMenus (MWContext *context)
{
  Boolean s = fe_own_selection_p (context, False);
  Boolean c = fe_own_selection_p (context, True);
  
  fe_ContextData* data = CONTEXT_DATA(context);
  
  if (data->cut_menuitem) {
    XtVaSetValues(data->cut_menuitem,   XmNsensitive, s, 0);
  }
  if (data->copy_menuitem) {
    XtVaSetValues(data->copy_menuitem,  XmNsensitive, s, 0);
  }
  if (data->paste_menuitem) {
    XtVaSetValues(data->paste_menuitem, XmNsensitive, c, 0);
  }
  if (data->paste_quoted_menuitem) {
    XtVaSetValues(data->paste_quoted_menuitem, XmNsensitive, c, 0);
  }
  if (data->delayed_menuitem) {
    XtVaSetValues(data->delayed_menuitem,
		  XmNsensitive, data->delayed_images_p, 0);
  }
  /* Quit menu item is always available, but Delete is only available when
     there is more than one window. */
  if (data->delete_menuitem) {
    XtVaSetValues(data->delete_menuitem,
		  XmNsensitive, (fe_WindowCount > 1), 0);
  }

#if defined(DEBUG_jwz) && defined(MOCHA)
  if (data->toggleJS_menuitem)
    XtVaSetValues (data->toggleJS_menuitem, XmNset,
                   !fe_globalPrefs.disable_javascript, 0);
#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
    if (data->toggleAnim_menuitem)
      XtVaSetValues (data->toggleAnim_menuitem, XmNset,
                     IL_AnimationsEnabled, 0);
    if (data->toggleLoops_menuitem)
      XtVaSetValues (data->toggleLoops_menuitem, XmNset,
                     (IL_AnimationsEnabled && IL_AnimationLoopsEnabled), 0);
    if (data->toggleSizes_menuitem)
      XtVaSetValues (data->toggleSizes_menuitem, XmNset,
                     !fe_ignore_font_size_changes_p, 0);
#endif /* DEBUG_jwz */


  /* Frame related stuff */
  if (context->type == MWContextBrowser)
    fe_SensitizeForSelectedFrame(context);

  if (context->type == MWContextMail ||
      context->type == MWContextNews ||
      context->type == MWContextMessageComposition)
    fe_MsgSensitizeMenubar (data->menubar, (XtPointer) context, 0);

  /* FindAgain is sensitive only if a find was done earlier.
     WARNING: We need to do this after fe_MsgSensitizeMenubar() because
     the mail/news code will always desensitize findagain. */
  if ((context->type == MWContextBrowser || context->type == MWContextMail ||
       context->type == MWContextNews) && data->findAgain_menuitem) {
    fe_FindData *find_data = CONTEXT_DATA(context)->find_data;
    if (find_data && find_data->string && find_data->string[0] != '\0')
      XtVaSetValues(data->findAgain_menuitem, XmNsensitive, True, 0);
    else
      XtVaSetValues(data->findAgain_menuitem, XmNsensitive, False, 0);
  }

#if 0
  /* ### We're overriding libmsg because we're not using
   * it to do the searches. It'll never activate Find Again.
   */
  if ((context->type == MWContextBrowser || context->type == MWContextMail ||
       context->type == MWContextNews) && data->findAgain_menuitem &&
       last_find_junk && last_find_junk->string &&
       last_find_junk->string[0] != '\0')
    XtVaSetValues(data->findAgain_menuitem, XmNsensitive, True, 0);
#endif /* 0 */

  /* If we're looking at an ftp site, enable file upload */
  if (context->type == MWContextBrowser && data->uploadFile_menuitem)
    {
      History_entry *he = SHIST_GetCurrent (&context->hist);
      Boolean b = False;

      if (he && he->address && (XP_STRNCMP (he->address, "ftp://", 6) == 0)
      	&& (he->address[XP_STRLEN(he->address)-1] == '/'))
	b = True;
      XtVaSetValues (data->uploadFile_menuitem, XmNsensitive, b, 0);
    }

  /* If we're building a mail composition window, init the
   * delivery method menu options.
   */
  if (context->type == MWContextMessageComposition)
    {
      XtVaSetValues (CONTEXT_DATA (context)->deliverNow_menuitem, XmNset,
		     !fe_globalPrefs.queue_for_later_p, 0);
      XtVaSetValues (CONTEXT_DATA (context)->deliverLater_menuitem, XmNset,
		     fe_globalPrefs.queue_for_later_p, 0);
    }
}



/* The popup menu
 */

URL_Struct *fe_url_under_mouse = 0;
URL_Struct *fe_image_under_mouse = 0;

static void
fe_save_image_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmString xm_title = 0;
  char *title = 0;
  URL_Struct *url;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  XtVaGetValues (widget, XmNlabelString, &xm_title, 0);
  XmStringGetLtoR (xm_title, XmFONTLIST_DEFAULT_TAG, &title);
  XmStringFree(xm_title);

  url =	fe_image_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_image_message);
  if (title) free (title);
  if (url)
    fe_SaveURL (context, url);
  fe_image_under_mouse = 0; /* it will be freed in the exit routine. */
}


static void
fe_save_link_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmString xm_title = 0;
  char *title = 0;
  URL_Struct *url;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  XtVaGetValues (widget, XmNlabelString, &xm_title, 0);
  XmStringGetLtoR (xm_title, XmFONTLIST_DEFAULT_TAG, &title);
  XmStringFree(xm_title);

  url =	fe_url_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_link_message);
  if (title) free (title);
  if (url)
    fe_SaveURL (context, url);
  fe_url_under_mouse = 0; /* it will be freed in the exit routine. */
}

static void
fe_open_image_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url =	fe_image_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_image_message);
  else
    {
      fe_GetURL (fe_GridToTopContext(context), url, FALSE);
      fe_image_under_mouse = 0; /* it will be freed in the exit routine. */
    }
}

static void
fe_load_delayed_image_cb (Widget widget, XtPointer closure,
			  XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url =	fe_image_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_image_message);
  else
    {
      fe_LoadDelayedImage (context, url->address);
      NET_FreeURLStruct (fe_image_under_mouse);
      fe_image_under_mouse = 0;
    }
}

static void
fe_follow_link_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url =	fe_url_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_link_message);
  else
    {
      fe_GetURL (context, url, FALSE);
      fe_url_under_mouse = 0; /* it will be freed in the exit routine. */
    }
}

static void
fe_follow_link_new_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url =	fe_url_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_link_message);
  else
    {
      if (MSG_RequiresComposeWindow(url->address))
	fe_GetURL (context, url, FALSE);
      else
	fe_MakeWindow (XtParent (CONTEXT_WIDGET (context)), context, url, NULL,
		     MWContextBrowser, FALSE);
      fe_url_under_mouse = 0; /* it will be freed in the exit routine. */
    }
}

static void
fe_add_link_bookmark_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  url =	fe_url_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_link_message);
  else
    {
      fe_AddToBookmark (context, 0, url, 0);
      NET_FreeURLStruct (fe_url_under_mouse);
      fe_url_under_mouse = 0;
    }
}


#ifdef DEBUG_jwz
static void
fe_other_browser_link_cb (Widget widget, XtPointer closure,
                          XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;

  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;

  url =	fe_url_under_mouse;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_link_message);

  fe_other_browser (context, url);
}
#endif /* DEBUG_jwz */



static void
fe_clipboard_url_1 (Widget widget, XtPointer closure, XtPointer call_data,
		    URL_Struct *url)
{
  MWContext *context = (MWContext *) closure;
  XmPushButtonCallbackStruct *cb = (XmPushButtonCallbackStruct *) call_data;
  XEvent *event = (cb ? cb->event : 0);
  Time time = (event && (event->type == KeyPress ||
			 event->type == KeyRelease)
	       ? event->xkey.time :
	       event && (event->type == ButtonPress ||
			 event->type == ButtonRelease)
	       ? event->xbutton.time :
	       XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context))));
  fe_UserActivity (context);
  if (fe_hack_self_inserting_accelerator (widget, closure, call_data))
    return;
  if (! url)
    FE_Alert (context, fe_globalData.not_over_link_message);
  else
    {
      fe_own_selection_1 (context, time, True, strdup (url->address));
      fe_own_selection_1 (context, time, False, strdup (url->address));
      NET_FreeURLStruct (url);
    }
}

static void
fe_clipboard_link_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  fe_clipboard_url_1 (widget, closure, call_data, fe_url_under_mouse);
  fe_url_under_mouse = 0;
}

static void
fe_clipboard_image_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  fe_clipboard_url_1 (widget, closure, call_data, fe_image_under_mouse);
  fe_image_under_mouse = 0;
}

static void
fe_on_menubar_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  Boolean menubar_p = True;
  fe_UserActivity (context);
  context = fe_GridToTopContext(context);
  CONTEXT_DATA (context)->show_menubar_p = menubar_p;
  fe_RebuildWindow (context);
}


static struct fe_button fe_popup_all [] =
{
  { "back",		fe_back_cb,		UNSELECTABLE,	0,
  OFF(back_popupitem)},
  { "forward",		fe_forward_cb,		UNSELECTABLE,	0,
  OFF(forward_popupitem)},
/*  { "home",		fe_home_cb,		SELECTABLE,	0 } */
};

/* If the menubar if off, the popup can be used to turn it on */
static struct fe_button fe_popup_menubar [] =
{
  { "showMenubar",	fe_on_menubar_cb,		UNSELECTABLE,	0,
  OFF(show_menubar_p)},
};
int nfe_popup_menubar = countof(fe_popup_menubar);

/* These popup menus over links are used in mail/news body too */
struct fe_button fe_popup_link [] =
{
  { "openURL",		fe_follow_link_cb,	SELECTABLE,	0 },
  { "addURLBookmark",	fe_add_link_bookmark_cb,SELECTABLE,	0 },
  { "openURLNewWindow",	fe_follow_link_new_cb,	SELECTABLE,	0 },
  { "saveURL",		fe_save_link_cb,	SELECTABLE,	0 },
  { "copyURLToClip",	fe_clipboard_link_cb,	SELECTABLE,	0 }
#ifdef DEBUG_jwz
  ,{ "",		0,			UNSELECTABLE,	0 },
  { "otherBrowser",	fe_other_browser_link_cb, SELECTABLE,	0 }
#endif /* DEBUG_jwz */
};
int nfe_popup_link = countof(fe_popup_link);

struct fe_button fe_popup_image [] =
{
  { "openImage",	fe_open_image_cb,	SELECTABLE,	0 },
  { "saveImage",	fe_save_image_cb,	SELECTABLE,	0 },
  { "copyImageURLToClip",fe_clipboard_image_cb,	SELECTABLE,	0 }
};
int nfe_popup_image = countof(fe_popup_image);

static struct fe_button fe_popup_delayed_image [] =
{
  { "loadDelayedImage",	fe_load_delayed_image_cb,SELECTABLE,	0 }
};


static void
fe_popup_menu_popdown_cb (Widget widget, XtPointer closure,
			  XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UnProtectContext(context);
  if (fe_IsContextDestroyed(context)) {
    free(context);
    free(CONTEXT_DATA(context));
  }
  else {
    if (context) fe_SetCursor (context, False);
  }
  XtDestroyWidget (widget);
}


char *fe_last_popup_item = 0;

void
fe_remember_last_popup_item (Widget widget, XtPointer closure,
			     XtPointer call_data)
{
  if (fe_last_popup_item) free (fe_last_popup_item);
  fe_last_popup_item = (widget ? XtName (widget) : 0);
  if (fe_last_popup_item) fe_last_popup_item = strdup (fe_last_popup_item);
}


/* Enabling removing of elements from popup if they aren't relevant */
#define DYNAMIC_MENU

static void
fe_popup_menu_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *context, *top_context;
  Widget parent;
  Arg av [20];
  int ac;
  Widget menu, item;
  Widget kids [30];
  int i, j;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  unsigned long x, y;
  LO_Element *le;
  char buf [255];
  char *image_url;
  XmString padding, xmstring, xmstring2, xmstring3;
  History_entry *h;
  Boolean image_delayed_p = False;
  struct fe_Pixmap *fep;
  Widget last = 0;

  /* If this is a grid cell, set focus to it */
  context = fe_MotionWidgetToMWContext (widget);
  XP_ASSERT (context);
  if (context && context->is_grid_cell)
    fe_UserActivity (context);

  /* Now grab the toplevel context and go. */
  context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (! context) return;
  parent = CONTEXT_WIDGET (context);
  h = SHIST_GetCurrent (&context->hist);

  /* Popup menu valid only for MWContextBrower */
  if (context->type != MWContextBrowser) return;
  if (context->name)
	if (strcmp(context->name,"view-source")==0) return;
  top_context = fe_GridToTopContext(context);

  padding = XmStringCreateLtoR ("", XmFONTLIST_DEFAULT_TAG);

  if (fe_url_under_mouse)   NET_FreeURLStruct (fe_url_under_mouse);
  if (fe_image_under_mouse) NET_FreeURLStruct (fe_image_under_mouse);
  fe_url_under_mouse   = 0;
  fe_image_under_mouse = 0;

  /* Find the URLs under the mouse. */
  fe_EventLOCoords (context, event, &x, &y);
  le = LO_XYToElement (context, x, y);
  if (le)
    switch (le->type)
      {
      case LO_TEXT:
	if (le->lo_text.anchor_href)
	  fe_url_under_mouse =
	    NET_CreateURLStruct ((char *) le->lo_text.anchor_href->anchor, FALSE);
	break;
      case LO_IMAGE:
	if (le->lo_image.anchor_href)
	  fe_url_under_mouse =
	    NET_CreateURLStruct ((char *) le->lo_image.anchor_href->anchor, FALSE);

        /* If this is an internal-external-reconnect image, then the *real* url
           follows the "internal-external-reconnect:" prefix. Gag gag gag. */
        image_url = (char*)le->lo_image.image_url;
        if (image_url) {
            if (!XP_STRNCMP (image_url, "internal-external-reconnect:", 28))
                image_url += 28;
            fe_image_under_mouse =
                NET_CreateURLStruct (image_url, NET_DONT_RELOAD);
        }

	fep = (struct fe_Pixmap *) le->lo_image.FE_Data;
	image_delayed_p = (fep &&
			   fep->type == fep_icon &&
			   fep->data.icon_number == IL_IMAGE_DELAYED);

	if (fe_url_under_mouse &&
	    strncmp (fe_url_under_mouse->address, "about:", 6) &&
	    le->lo_image.image_attr->attrmask & LO_ATTR_ISMAP)
	  {
	    /* cribbed from fe_activate_link_action() */
	    long cx = event->xbutton.x + CONTEXT_DATA (context)->document_x;
	    long cy = event->xbutton.y + CONTEXT_DATA (context)->document_y;
	    long ix = le->lo_image.x + le->lo_image.x_offset;
	    long iy = le->lo_image.y + le->lo_image.y_offset;
	    long x = cx - ix - le->lo_image.border_width;
	    long y = cy - iy - le->lo_image.border_width;
	    NET_AddCoordinatesToURLStruct (fe_url_under_mouse,
						  ((x < 0) ? 0 : x),
						  ((y < 0) ? 0 : y));
	  }
	break;
      }

  if (h && h->is_binary)
    {
      if (fe_image_under_mouse)
	NET_FreeURLStruct (fe_image_under_mouse);
      fe_image_under_mouse = NET_CreateURLStruct (h->address, FALSE);
    }

  if (fe_url_under_mouse &&
      !strncmp ("mailto:", fe_url_under_mouse->address, 7))
    {
      NET_FreeURLStruct (fe_url_under_mouse);
      fe_url_under_mouse = 0;
    }

  /* Add the referer to the URLs. */
  if (h && h->address)
    {
      if (fe_url_under_mouse)
	fe_url_under_mouse->referer = strdup (h->address);
      if (fe_image_under_mouse)
	fe_image_under_mouse->referer = strdup (h->address);
    }

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  menu = XmCreatePopupMenu (parent, "popup", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = XmCreateLabelGadget (menu, "title", av, ac);
  kids [i++] = XmCreateSeparatorGadget (menu, "titleSeparator", av, ac);

  for (j = 0; j < countof (fe_popup_all); j++)
    {
      item = XmCreatePushButtonGadget (menu, fe_popup_all[j].name, av, ac);
#if 0
      XtVaGetValues (item, XmNlabelString, &xmstring, 0);
      xmstring2 = XmStringConcat (padding, xmstring);
      XtVaSetValues (item, XmNlabelString, xmstring2, 0);
      XmStringFree (xmstring2);
      XmStringFree (xmstring);
#endif
      XtAddCallback (item, XmNactivateCallback, fe_remember_last_popup_item,0);
      XtAddCallback (item, XmNactivateCallback, fe_popup_all[j].callback,
		     context);

      if (!strcmp (fe_popup_all[j].name, "back") &&
#ifdef NEW_FRAME_HIST
	  !SHIST_CanGoBack (top_context)
#else
	  !SHIST_CanGoBack (&top_context->hist)
#endif /* NEW_FRAME_HIST */
	  )
	XtVaSetValues (item, XmNsensitive, False, 0);
      else if (!strcmp (fe_popup_all[j].name, "forward") &&
#ifdef NEW_FRAME_HIST
	       !SHIST_CanGoForward (top_context)
#else
	       !SHIST_CanGoForward (&top_context->hist)
#endif /* NEW_FRAME_HIST */
	       )
	XtVaSetValues (item, XmNsensitive, False, 0);

      if (fe_last_popup_item &&
	  !strcmp (fe_last_popup_item, fe_popup_all[j].name))
	last = item;

      if (fe_popup_all[j].offset) {
	fe_ContextData *data = CONTEXT_DATA (context);
	Widget* foo = (Widget*)
	  (((char*) data) + (int) (fe_popup_all[j].offset));
	*foo = item;
      }

      kids [i++] = item;
    }
#ifdef DYNAMIC_MENU
  if (!CONTEXT_DATA(top_context)->show_menubar_p)
#endif
    {
      kids [i++] = XmCreateSeparatorGadget (menu, "separator2", av, ac);
      kids [i++] = XmCreateSeparatorGadget (menu, "separator3", av, ac);
      for (j = 0; j < countof (fe_popup_menubar); j++)
	{
	  item = XmCreatePushButtonGadget (menu, fe_popup_menubar[j].name,
						av,ac);
	  if (fe_last_popup_item &&
		!strcmp (fe_last_popup_item, fe_popup_menubar[j].name))
	      last = item;

	  XtAddCallback (item, XmNactivateCallback,
			 fe_remember_last_popup_item, 0);
	  XtAddCallback (item, XmNactivateCallback,
			 fe_popup_menubar[j].callback, context);

	  kids [i++] = item;
	}
    }
#ifdef DYNAMIC_MENU
  if (fe_url_under_mouse)
#endif
    {
      kids [i++] = XmCreateSeparatorGadget (menu, "separator2", av, ac);
      kids [i++] = XmCreateSeparatorGadget (menu, "separator3", av, ac);

      for (j = 0; j < countof (fe_popup_link); j++)
	{
#ifdef DEBUG_jwz
          if (!*fe_popup_link[j].name)
            {
              item = XmCreateSeparatorGadget (menu, "separator", av, ac);
              kids [i++] = item;
              continue;
            }
#endif /* DEBUG_jwz */

          item = XmCreatePushButtonGadget (menu, fe_popup_link[j].name,av,ac);
	  XtVaGetValues (item, XmNlabelString, &xmstring, 0);
	  if (j == 0)
	    {
	      if (fe_url_under_mouse)
		{
		  const char *url = fe_url_under_mouse->address;
		  char *sug = MimeGuessURLContentName(context, url);
		  const char *s;
		  if (sug) url = sug;
		  s = strrchr (url, '/');
		  if (s)
		    s++;
		  else
		    s = url;
#if 0
		  PR_snprintf (buf, sizeof (buf), " (%.200s)", s);
#else
		  strcpy(buf, " (");
		  fe_MidTruncateString(s, buf + strlen(buf), 25);
		  strcat(buf, ")");
#endif
		  if (sug) free(sug);

		  xmstring2 = XmStringCreateLtoR(buf,XmFONTLIST_DEFAULT_TAG);
		  xmstring3 = XmStringConcat (xmstring, xmstring2);
		  XtVaSetValues (item, XmNlabelString, xmstring3, 0);
		  XmStringFree (xmstring2);
		  XmStringFree (xmstring3);
		}
	    }
	  else
	    {
	      xmstring2 = XmStringConcat (padding, xmstring);
	      XtVaSetValues (item, XmNlabelString, xmstring2, 0);
	      XmStringFree (xmstring2);
	    }
	  XmStringFree (xmstring);
	  XtAddCallback (item, XmNactivateCallback,
			 fe_remember_last_popup_item, 0);
	  XtAddCallback (item, XmNactivateCallback, fe_popup_link[j].callback,
			 context);
	  if (! fe_url_under_mouse)
	    XtVaSetValues (item, XmNsensitive, False, 0);

	  if (fe_last_popup_item &&
	      !strcmp (fe_last_popup_item, fe_popup_link[j].name))
	    last = item;

	  kids [i++] = item;
	}
    }
#ifdef DYNAMIC_MENU
  if (fe_image_under_mouse)
#endif
    {
      kids [i++] = XmCreateSeparatorGadget (menu, "separator2", av, ac);
      kids [i++] = XmCreateSeparatorGadget (menu, "separator3", av, ac);

      for (j = 0; j < countof (fe_popup_image); j++)
	{
	  item = XmCreatePushButtonGadget (menu, fe_popup_image[j].name,av,ac);
	  XtVaGetValues (item, XmNlabelString, &xmstring, 0);
	  if (j == 0)
	    {
	      if (fe_image_under_mouse)
		{
		  const char *url = fe_image_under_mouse->address;
		  char *sug = MimeGuessURLContentName(context, url);
		  const char *s;
		  if (sug) url = sug;
		  s = strrchr (url, '/');
		  if (s)
		    s++;
		  else
		    s = url;
#if 0
		  PR_snprintf (buf, sizeof (buf), " (%.200s)", s);
#else
		  strcpy(buf, " (");
		  fe_MidTruncateString(s, buf + strlen(buf), 25);
		  strcat(buf, ")");
#endif
		  if (sug) free(sug);

		  xmstring2 = XmStringCreateLtoR(buf,XmFONTLIST_DEFAULT_TAG);
		  xmstring3 = XmStringConcat (xmstring, xmstring2);
		  XtVaSetValues (item, XmNlabelString, xmstring3, 0);
		  XmStringFree (xmstring2);
		  XmStringFree (xmstring3);
		}
	    }
	  else
	    {
	      xmstring2 = XmStringConcat (padding, xmstring);
	      XtVaSetValues (item, XmNlabelString, xmstring2, 0);
	      XmStringFree (xmstring2);
	    }
	  XmStringFree (xmstring);
	  XtAddCallback (item, XmNactivateCallback,
			 fe_remember_last_popup_item, 0);
	  XtAddCallback (item, XmNactivateCallback, fe_popup_image[j].callback,
			 context);
	  if (! fe_image_under_mouse)
	    XtVaSetValues (item, XmNsensitive, False, 0);

	  if (fe_last_popup_item &&
	      !strcmp (fe_last_popup_item, fe_popup_image[j].name))
	    last = item;

	  kids [i++] = item;
	}

#ifdef DYNAMIC_MENU
      if (image_delayed_p)
#endif
	for (j = 0; j < countof (fe_popup_delayed_image); j++)
	  {
	    item = XmCreatePushButtonGadget (menu,
					     fe_popup_delayed_image[j].name,
					     av, ac);
	    XtVaGetValues (item, XmNlabelString, &xmstring, 0);
	    xmstring2 = XmStringConcat (padding, xmstring);
	    XtVaSetValues (item, XmNlabelString, xmstring2, 0);
	    XmStringFree (xmstring2);
	    XmStringFree (xmstring);
	    XtAddCallback (item, XmNactivateCallback,
			   fe_remember_last_popup_item, 0);
	    XtAddCallback (item, XmNactivateCallback,
			   fe_popup_delayed_image[j].callback, context);
	    if (!fe_image_under_mouse || !image_delayed_p)
	      XtVaSetValues (item, XmNsensitive, False, 0);

	    if (fe_last_popup_item &&
		!strcmp (fe_last_popup_item, fe_popup_delayed_image[j].name))
	      last = item;

	    kids [i++] = item;
	  }
    }

#if 0		/* Disabling back in frame and forward in frame - dp */

  /* In some cases, we need to change menu strings depending
   * prevailing winds ... in this case, if we've got frames
   * displayed, change the back and forward strings.
   */
  if (CONTEXT_DATA (context)->back_popupitem) {
    if (context->is_grid_cell)
      s = XP_GetString( XFE_BACK_IN_FRAME );
    else
      s = XP_GetString( XFE_BACK );
    fe_SetString(CONTEXT_DATA (context)->back_popupitem, XmNlabelString, s);
  }

  if (CONTEXT_DATA (context)->forward_popupitem) {
    if (context->is_grid_cell)
      s = XP_GetString( XFE_FORWARD_IN_FRAME );
    else
      s = XP_GetString( XFE_FORWARD );
    fe_SetString(CONTEXT_DATA (context)->forward_popupitem, XmNlabelString, s);
  }
#endif /* 0 */

#ifdef DYNAMIC_MENU
  if (fe_url_under_mouse || fe_image_under_mouse)
#endif
    kids [i++] = XmCreateSeparatorGadget (menu, "separator2", av, ac);

  XmStringFree (padding);

  XtAddCallback (XtParent (menu), XmNpopdownCallback,
		 fe_popup_menu_popdown_cb, context);

  /* The popdown_cb will UnProtect the context */
  fe_ProtectContext(context);

  XtManageChildren (kids, i);

  /* WHY THE FUCK DOESN'T THIS WORK??????? */
  if (last)
    XtVaSetValues (menu, XmNmenuHistory, last, 0);

  XmMenuPosition (menu, (XButtonPressedEvent *) event);

  XtManageChild (menu);
}


/*
 * For scrolling the window with the arrow keys, we need to
 * insert a lookahead key eater into each action routine,
 * else it is possible to queue up too many key events, and
 * screw up our careful scroll/expose event timing.
 */
static void
fe_eat_window_key_events(Display *display, Window window)
{
  XEvent event;

  XSync(display, FALSE);
#ifdef DEBUG_jwz
  /* also swallow mouse clicks, to avoid extreme typeahead on mouse
     wheel scrolling. */
  while (XCheckWindowEvent(display, window,
                           KeyPressMask|KeyReleaseMask|
                           ButtonPressMask|ButtonReleaseMask,
                           &event) == TRUE);
#else /* !DEBUG_jwz */
  while (XCheckTypedWindowEvent(display, window, KeyPress, &event) == TRUE);
#endif /* !DEBUG_jwz */
}


static void
fe_column_forward_action (Widget widget, XEvent *event,
			  String *av, Cardinal *ac)
{
  XKeyEvent *kev = (XKeyEvent *)event;
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;

  fe_column_forward_cb (widget, context, 0);
  fe_eat_window_key_events(XtDisplay(widget), kev->window);
}


static void
fe_column_backward_action (Widget widget, XEvent *event,
			  String *av, Cardinal *ac)
{
  XKeyEvent *kev = (XKeyEvent *)event;
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;

  fe_column_backward_cb (widget, context, 0);
  fe_eat_window_key_events(XtDisplay(widget), kev->window);
}


static void
fe_line_forward_command_1 (Widget widget, XEvent *event,
                           String *av, Cardinal *ac,
                           Bool invert_p)
{
  int i /* , j */;
  char c;
  XKeyEvent *kev = (XKeyEvent *)event;
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;

  /* take number-of-lines as an argument, e.g., LineUp(4) */
  i = 1;
  if (*ac > 0 && av[0])
    {
      if (1 != sscanf (av[0], " %d %c", &i, &c))
        i = 1;
    }

  if (invert_p)
    i = -i;

  fe_line_forward_1 (widget, context, 0, i);

  fe_eat_window_key_events(XtDisplay(widget), kev->window);

#if 0
  /* (this is only needed if we don't strip buttonpress events from
     the event queue). */
  j = CONTEXT_DATA (context)->line_height * ((i < 0 ? -i : i) + 1);
  if (i > 0)
    fe_RefreshArea (context,
                    CONTEXT_DATA (context)->document_x,
                    (CONTEXT_DATA (context)->document_y +
                     CONTEXT_DATA (context)->scrolled_height - j),
                    CONTEXT_DATA (context)->scrolled_width,
                    j);
  else
    fe_RefreshArea (context,
                    CONTEXT_DATA (context)->document_x,
                    CONTEXT_DATA (context)->document_y,
                    CONTEXT_DATA (context)->scrolled_width,
                    j);
#endif
}


static void
fe_line_forward_action (Widget widget, XEvent *event,
			  String *av, Cardinal *ac)
{
  fe_line_forward_command_1 (widget, event, av, ac, False);
}


static void
fe_line_backward_action (Widget widget, XEvent *event,
			  String *av, Cardinal *ac)
{
  fe_line_forward_command_1 (widget, event, av, ac, True);
}



/* Actions for use in translations tables. 
 */

#define DEFACTION(NAME) \
static void \
fe_##NAME##_action (Widget widget, XEvent *event, String *av, Cardinal *ac) \
{  \
  MWContext *context = fe_WidgetToMWContext (widget); \
  XP_ASSERT (context); \
  if (!context) return; \
  fe_##NAME##_cb (widget, context, 0); \
}

DEFACTION (new)
/*DEFACTION (open_url)*/
/*DEFACTION (open_file)*/
#ifdef EDITOR
/*DEFACTION (edit)*/
#endif /* EDITOR */
/*DEFACTION (save_as)*/
/*DEFACTION (mailto)*/
/*DEFACTION (print)*/
DEFACTION (docinfo)
/*DEFACTION (delete)*/
DEFACTION (quit)
DEFACTION (undo)
DEFACTION (redo)
DEFACTION (cut)
DEFACTION (copy)
DEFACTION (paste)
DEFACTION (paste_quoted)
/*DEFACTION (find)*/
DEFACTION (find_again)
DEFACTION (reload)
DEFACTION (load_images)
DEFACTION (refresh)
DEFACTION (view_source)
DEFACTION (back)
DEFACTION (forward)
DEFACTION (history)
DEFACTION (home)
DEFACTION (general_prefs)
DEFACTION (mailnews_prefs)
DEFACTION (network_prefs)
DEFACTION (security_prefs)
/*DEFACTION (show_toolbar)*/
/*DEFACTION (show_url)*/
/*DEFACTION (show_directory_buttons)*/
#ifdef HAVE_SECURITY
/*DEFACTION (show_security_bar)*/
#endif
/*DEFACTION (toggle_loads)*/
/*DEFACTION (toggle_fancy)*/
DEFACTION (save_options)
/*DEFACTION (add_bookmark)*/
DEFACTION (view_bookmark)
DEFACTION (fishcam)
DEFACTION (net_showstatus)
DEFACTION (abort)
DEFACTION (spacebar)
DEFACTION (page_forward)
DEFACTION (page_backward)
/*DEFACTION (line_forward)*/
/*DEFACTION (line_backward)*/
/*DEFACTION (column_forward)*/
/*DEFACTION (column_backward)*/

DEFACTION (save_image)
DEFACTION (open_image)
DEFACTION (save_link)
DEFACTION (follow_link)
DEFACTION (follow_link_new)
#undef DEFACTION

static void
fe_delete_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  if (!context) return;
  fe_UserActivity (context);
  if (CONTEXT_DATA(context)->hasChrome &&
	!CONTEXT_DATA(context)->chrome.allow_close) {
    XBell (XtDisplay (widget), 0);
  }
  else
    fe_delete_cb(widget, (XtPointer) context, 0);
}

static void
fe_exit_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  if (!context) return;
  fe_UserActivity (context);
  fe_QuitCallback(widget, (XtPointer)context, NULL);
}

static void
fe_open_url_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* openURL() pops up a file requestor.
     openURL(http:xxx) opens that URL.
     Really only one arg is meaningful, but we also accept the form
     openURL(http:xxx, remote) for the sake of remote.c, and treat
     openURL(remote) the same as openURL().
   */
  MWContext *context = fe_WidgetToMWContext (widget);
  MWContext *old_context = NULL;
  Boolean other_p = False;
  char *windowName = 0;
  XP_ASSERT (context);
  if (!context) return;
  context = fe_GridToTopContext (context);
  fe_UserActivity (context);

  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;

  if (*ac > 1 && av[*ac-1] )
  {
     if ( 
      (!strcasecomp (av[*ac-1], "new-window") ||
       !strcasecomp (av[*ac-1], "new_window") ||
       !strcasecomp (av[*ac-1], "newWindow") ||
       !strcasecomp (av[*ac-1], "new")))
    {
#ifdef DEBUG_jwz
      /* if this window was newly-created, then don't bother creating a
         second new one. */
      History_entry *he = SHIST_GetCurrent (&context->hist);
      if (he != 0)
#endif /* DEBUG_jwz */
      other_p = True;
      (*ac)--;
    }
    else if ( (old_context = XP_FindNamedContextInList(context, av[*ac-1])))
    {
	context = old_context;
	other_p = False;
        (*ac)--;
    }
    else 
    {
	StrAllocCopy(windowName, av[*ac-1]);
	other_p = True;
        (*ac)--;
    }

   }

  if (*ac == 1 && av[0])
    {
      URL_Struct *url_struct = NET_CreateURLStruct (av[0], FALSE);

      /* Dont create new windows for Compose. fe_GetURL will take care of it */
      if (MSG_RequiresComposeWindow(url_struct->address))
	other_p = False;

      if (other_p)
      {
	context = fe_MakeWindow (XtParent (CONTEXT_WIDGET (context)),
		       context, url_struct, windowName, 
			MWContextBrowser, FALSE);
        XP_FREE(windowName);
      }
      else
	fe_GetURL (context, url_struct, FALSE);

#ifdef DEBUG_jwz
      /* Make sure that the window we're actually drawing on gets raised
         (since fe_server_handle_command() no longer indiscriminately raises.)
       */
      {
        Display *dpy = XtDisplay (CONTEXT_WIDGET (context));
        Window window = XtWindow(CONTEXT_WIDGET (context));
        XMapRaised (dpy, window);
      }
#endif /* DEBUG_jwz */

    }
  else if (*ac > 1)
    {
      fprintf (stderr, "%s: usage: OpenURL(url [ , new-window|window-name ] )\n",
	       fe_progname);
    }
  else
    {
      fe_open_url_cb (widget, context, 0);
    }
}

static void
fe_open_file_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* See also fe_open_url_action() */
  MWContext *context = fe_WidgetToMWContext (widget);
  Boolean other_p = False;
  XP_ASSERT (context);
  if (!context) return;
  context = fe_GridToTopContext (context);
  fe_UserActivity (context);
  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;

  if (*ac && av[*ac-1] &&
      (!strcasecomp (av[*ac-1], "new-window") ||
       !strcasecomp (av[*ac-1], "new_window") ||
       !strcasecomp (av[*ac-1], "newWindow") ||
       !strcasecomp (av[*ac-1], "new")))
    {
      other_p = True;
      (*ac)--;
    }

  if (*ac == 1 && av[0])
    {
      char new [1024];
      URL_Struct *url_struct;
      PR_snprintf (new, sizeof (new), "file:%.900s", av[0]);
      url_struct = NET_CreateURLStruct (new, FALSE);
      if (other_p)
	fe_MakeWindow (XtParent (CONTEXT_WIDGET (context)),
		       context, url_struct, NULL, MWContextBrowser, FALSE);
      else
	fe_GetURL (context, url_struct, FALSE);
    }
  else if (*ac > 1)
    {
      fprintf (stderr, "%s: usage: OpenFile(file)\n", fe_progname);
    }
  else
    {
      fe_open_file_cb (widget, context, 0);
    }
}


static void
fe_print_remote_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* Silent print for remote. Valid invocations are
     print(filename, remote)		: print URL of context to filename
     print(remote)			: print URL of context to printer
     print(remote)			: print URL of context to printer
  */
  MWContext *context = fe_WidgetToMWContext (widget);
  URL_Struct *url_struct = NULL;
  Boolean toFile = False;
  char *filename = NULL;

  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);

  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;
  else
    return;	/* this is valid only for remote */

  if (*ac && av[*ac-1]) {
    filename = av[*ac-1];
    if (*filename) toFile = True;
    (*ac)--;
  }

  if (*ac > 0)
    fprintf (stderr, "%s: usage: print([filename])\n", fe_progname);

  url_struct = SHIST_CreateWysiwygURLStruct (context,
                                             SHIST_GetCurrent (&context->hist));

  if (url_struct)
    {
      if (!toFile || filename)
        fe_Print (context, url_struct, toFile, filename);
      else
        NET_FreeURLStruct (url_struct);
    }
}

static void
fe_print_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);

  /* Print in mail/news should check validity */

  if ((context->type == MWContextMail) || (context->type == MWContextNews)) {
    int status;
    const char *string = 0;
    XP_Bool selectable_p = FALSE;
    MSG_COMMAND_CHECK_STATE selected_p = MSG_NotUsed;

    status = MSG_CommandStatus (context, MSG_Print, &selectable_p, &selected_p,
			      &string, 0);
    if ((status < 0) || !selectable_p) {
	/* We can only get here when invoked by an accelerator -- otherwise,
	   the menu item would have been unselectable. */
	XBell (XtDisplay (CONTEXT_WIDGET(context)), 0);
	return;
    }
  }

  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    fe_print_remote_action (widget, event, av, ac);
  else
    fe_print_cb (widget, context, 0);
}

#ifdef EDITOR

static void
fe_edit_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  printf ("fe_edit_action\n");
}

#endif /* EDITOR */

static void
fe_save_as_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* See also fe_open_url_action() */
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);
  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;
  if ((*ac == 1 || *ac == 2) && av[0])
    {
      URL_Struct *url;
      struct save_as_data *sad;
      char *file;
      int type = fe_FILE_TYPE_HTML;

      if (*ac == 2 && av[1])
	{
	  if (!strcasecmp (av[1], "source"))
	    type = fe_FILE_TYPE_HTML;
	  else if (!strcasecmp (av[1], "html"))
	    type = fe_FILE_TYPE_HTML;
#ifdef SAVE_ALL
	  else if (!strcasecmp (av[1], "tree"))
	    type = fe_FILE_TYPE_HTML_AND_IMAGES;
	  else if (!strcasecmp (av[1], "html-and-images"))
	    type = fe_FILE_TYPE_HTML_AND_IMAGES;
	  else if (!strcasecmp (av[1], "source-and-images"))
	    type = fe_FILE_TYPE_HTML_AND_IMAGES;
#endif /* SAVE_ALL */
	  else if (!strcasecmp (av[1], "text"))
	    type = fe_FILE_TYPE_TEXT;
	  else if (!strcasecmp (av[1], "formatted-text"))
	    type = fe_FILE_TYPE_FORMATTED_TEXT;
	  else if (!strcasecmp (av[1], "ps"))
	    type = fe_FILE_TYPE_PS;
	  else if (!strcasecmp (av[1], "postscript"))
	    type = fe_FILE_TYPE_PS;
	  else
	    {
	      fprintf (stderr, "%s: usage: SaveAS(file, output-data-type)\n",
		       fe_progname);
	      return;
	    }
	}

      url = SHIST_CreateWysiwygURLStruct (context,
                                          SHIST_GetCurrent (&context->hist));
      file = strdup (av[0]);
      sad = make_save_as_data (context, False, type, url, file);
      if (sad)
	fe_save_as_nastiness (context, url, sad);
      else
	NET_FreeURLStruct (url);
    }
  else if (*ac > 2)
    {
      fprintf (stderr, "%s: usage: SaveAS(file, [output-data-type])\n",
	       fe_progname);
    }
  else
    {
      fe_save_as_cb (widget, context, 0);
    }
}

static void
fe_mailto_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* See also fe_open_url_action() */
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);
  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;
  if (*ac >= 1)
    {
      char *to;
      int size = 0;
      int i;
      for (i = 0; i < *ac; i++)
	size += (strlen (av[i]) + 2);
      to = (char *) malloc (size + 1);
      *to = 0;
      for (i = 0; i < *ac; i++)
	{
	  strcat (to, av[i]);
	  if (i < (*ac)-1)
	    strcat (to, ", ");
	}

      MSG_ComposeMessage (context,
			  0, /* from */
			  0, /* reply_to */
			  to,
			  0, /* cc */
			  0, /* bcc */
			  0, /* fcc */
			  0, /* newsgroups */
			  0, /* followup_to */
			  0, /* organization */
			  0, /* subject */
			  0, /* references */
			  0, /* other_random_headers */
			  0, /* attachment */
			  0, /* newspost_url */
			  0, /* body */
			  FALSE, FALSE /* encrypt, sign -- let libmsg decide */
			  );
    }
  else if (*ac > 1)
    {
      fprintf (stderr, "%s: usage: mailto(address ...)\n", fe_progname);
    }
  else
    {
      fe_mailto_cb (widget, context, 0);
    }
}


static void
fe_find_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* See also fe_open_url_action() */
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);
  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;
#if 0
  /* #### this should search for the given string without a dialog. */
  if (*ac == 1 && av[0])
    {
      ...
    }
  else if (*ac > 1)
    {
      fprintf (stderr, "%s: usage: find(string)\n");
    }
  else
#endif /* 0 */
    {
      fe_find_cb (widget, context, 0);
    }
}


static void
fe_add_bookmark_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  /* See also fe_open_url_action() */
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);
  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote"))
    (*ac)--;
  if ((*ac == 1 || *ac == 2) && av[0])
    {
      URL_Struct *url = NET_CreateURLStruct (av[0], FALSE);
      char *title = ((*ac == 2 && av[1]) ? av[1] : 0);
      fe_AddToBookmark (context, title, url, 0);
    }
  else if (*ac > 2)
    {
      fprintf (stderr, "%s: usage: addBookmark(url, title)\n", fe_progname);
    }
  else
    {
      fe_add_bookmark_cb (widget, context, 0);
    }
}


static void
fe_new_msg_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);
  MSG_Mail (context);
}

static void
fe_html_help_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_Bool remote_p = False;
  char *map_file_url;
  char *id;
  char *search_text;

  XP_ASSERT (context);
  if (!context) return;
  fe_UserActivity (context);
  if (*ac && av[*ac-1] && !strcmp (av[*ac-1], "remote")) {
    remote_p = True;
    (*ac)--;
  }

  if (*ac == 3) {
    map_file_url = av[0];
    id = av[1];
    search_text = av[2];
    NET_GetHTMLHelpFileFromMapFile(context, map_file_url, id, search_text);
  }
  else {
      fprintf (stderr, "%s: usage: htmlHelp(map-file, id, search-text)\n",
	       fe_progname);
  }
}


static void
fe_deleteKey_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);

  XP_ASSERT(context);
  if (!context) return;

  if (context->type == MWContextMail || context->type == MWContextNews)
    fe_msgDoCommand(context, MSG_DeleteMessage);
  else
    fe_page_backward_action(widget, event, av, ac);
}

static void
fe_undefined_key_action (Widget widget, XEvent *event,
			 String *av, Cardinal *ac)
{
  XBell (XtDisplay (widget), 0);
}

XtActionsRec fe_CommandActions [] =
{
  { "new",		fe_new_action		},
  { "exit",		fe_exit_action		},
  { "openURL",		fe_open_url_action	},
  { "openFile",		fe_open_file_action	},
#ifdef EDITOR
  { "edit",		fe_edit_action	},
#endif /* EDITOR */
  { "saveAs",		fe_save_as_action	},
  { "mailto",		fe_mailto_action	},
  { "print",		fe_print_action		},
  { "docinfo",		fe_docinfo_action	},
  { "delete",		fe_delete_action	},
  { "quit",		fe_quit_action		},
  { "undo",		fe_undo_action		},
  { "redo",		fe_redo_action		},
  { "cut",		fe_cut_action		},
  { "copy",		fe_copy_action		},
  { "paste",		fe_paste_action		},
  { "pasteQuoted",	fe_paste_quoted_action	},
  { "find",		fe_find_action		},
  { "findAgain",	fe_find_again_action	},
  { "reload",		fe_reload_action	},
  { "loadImages",	fe_load_images_action	},
  { "refresh",		fe_refresh_action	},
  { "source",		fe_view_source_action	},
  { "viewSource",	fe_view_source_action	},
  { "back",		fe_back_action		},
  { "forward",		fe_forward_action	},
  { "home",		fe_home_action		},
  { "abort",		fe_abort_action		},
  { "viewHistory",	fe_history_action	},
  { "history",		fe_history_action	},
  { "historyItem",	fe_HistoryItemAction	},
  { "general_prefs",	fe_general_prefs_action	},
  { "mailnews_prefs",	fe_mailnews_prefs_action	},
  { "network_prefs",	fe_network_prefs_action	},
  { "security_prefs",	fe_security_prefs_action	},
/*{ "showToolbar",	fe_show_toolbar_action	}, */
/*{ "showURL",		fe_show_url_action	}, */
/*{ "showDirectoryButtons",fe_show_directory_buttons_action },*/
#ifdef HAVE_SECURITY
/*{ "showSecurityBar",	fe_show_security_bar_action }, */
#endif
/*{ "autoLoadImages",	fe_toggle_loads_action	}, */
/*{ "fancyFTP",		fe_toggle_fancy_action	}, */
  { "saveOptions",	fe_save_options_action	},
  { "addBookmark",	fe_add_bookmark_action	},
  { "viewBookmark",	fe_view_bookmark_action	},
  { "fishcam",		fe_fishcam_action	},
  { "mailNew",		fe_new_msg_action	},
  { "net_showstatus",	fe_net_showstatus_action	},
  { "Spacebar",	fe_spacebar_action	},
  { "PageDown",		fe_page_forward_action	},
  { "PageUp",		fe_page_backward_action	},
  { "LineDown",		fe_line_forward_action	},
  { "LineUp",		fe_line_backward_action	},
  { "ColumnLeft",	fe_column_forward_action  },
  { "ColumnRight",	fe_column_backward_action },

  { "Delete",		fe_deleteKey_action },
  { "Backspace",	fe_deleteKey_action },

  { "CommandMenu",	fe_popup_menu_action },
  { "SaveImage",	fe_save_image_action },
  { "OpenImage",	fe_open_image_action },
  { "SaveURL",		fe_save_link_action },
  { "OpenURL",		fe_follow_link_action },
  { "OpenURLNewWindow",	fe_follow_link_new_action },

  /* For Html Help */
  { "htmlHelp",		fe_html_help_action },

  { "undefined-key",	fe_undefined_key_action }

};

int fe_CommandActionsSize = countof (fe_CommandActions);


/**************************/
/* MessageCompose actions */
/* (Mail/News actions are */
/* defined in mailnews.c) */
/**************************/


void
fe_InitCommandActions ()
{
  XtAppAddActions(fe_XtAppContext, fe_CommandActions, fe_CommandActionsSize);
  XtAppAddActions(fe_XtAppContext, fe_MailNewsActions, fe_MailNewsActionsSize);
}


/* This installs all the usual translations on widgets which are a part
   of the main Netscape window -- it recursively decends the tree, 
   installing the appropriate translations everywhere.  This should
   not be called on popups/transients, but only on widgets which are
   children of the main Shell.
 */
void
fe_HackTranslations (MWContext *context, Widget widget)
{
  XtTranslations global_translations = 0;
  XtTranslations secondary_translations = 0;
  XP_Bool has_display_area = FALSE;

  if (XmIsGadget (widget))
    return;

 /* To prevent focus problems, dont enable translations on the menubar
    and its children. The problem was that when we had the translations
    in the menubar too, we could do a translation and popup a modal
    dialog when one of the menu's from the menubar was pulleddown. Now
    motif gets too confused about who holds pointer and keyboard focus.
 */

  if (XmIsRowColumn(widget)) {
    unsigned char type;
    XtVaGetValues(widget, XmNrowColumnType, &type, 0);
    if (type == XmMENU_BAR)
      return;
  }

  switch (context->type)
    {
#ifdef EDITOR
    case MWContextEditor:
      has_display_area = FALSE;
      global_translations = fe_globalData.global_translations;
      secondary_translations = fe_globalData.editor_global_translations;
      break;
#endif /*EDITOR*/

    case MWContextBrowser:
      has_display_area = TRUE;
      global_translations = fe_globalData.global_translations;
      secondary_translations = 0;
      break;

    case MWContextMail:
    case MWContextNews:
      has_display_area = TRUE;
      global_translations = fe_globalData.global_translations;
      secondary_translations = fe_globalData.mailnews_global_translations;
      break;

    case MWContextMessageComposition:
      has_display_area = FALSE;
      global_translations = 0;
      secondary_translations = fe_globalData.mailcompose_global_translations;
      break;

    case MWContextBookmarks:
    case MWContextAddressBook:
      has_display_area = FALSE;
      global_translations = 0;
      secondary_translations = fe_globalData.bm_global_translations;
      break;

    case MWContextDialog:
      has_display_area = TRUE;
      global_translations = 0;
      secondary_translations = fe_globalData.dialog_global_translations;
      break;

    default:
      break;
  }

  if (global_translations)
    XtOverrideTranslations (widget, global_translations);
  if (secondary_translations)
    XtOverrideTranslations (widget, secondary_translations);


  if (XmIsTextField (widget) || XmIsText (widget) || XmIsList(widget))
    {
      /* Set up the editing translations (all text fields, everywhere.) */
      if (XmIsTextField (widget) || XmIsText (widget))
	fe_HackTextTranslations (widget);

      /* Install globalTextFieldTranslations on single-line text fields in
	 windows which have an HTML display area (browser, mail, news) but
	 not in windows which don't have one (compose, download, bookmarks,
	 address book...)
       */
      if (has_display_area &&
	  XmIsTextField (widget) &&
	  fe_globalData.global_text_field_translations)
	XtOverrideTranslations (widget,
				fe_globalData.global_text_field_translations);
   }
  else
   {
       Widget *kids = 0;
       Cardinal nkids = 0;

      /* Not a Text or TextField.
       */
      /* Install globalNonTextTranslations on non-text widgets in windows which
	 have an HTML display area (browser, mail, news, view source) but not
	 in windows which don't have one (compose, download, bookmarks,
	 address book...)
       */
      if (has_display_area &&
	  fe_globalData.global_nontext_translations)
	XtOverrideTranslations (widget,
				fe_globalData.global_nontext_translations);

      /* Now recurse on the children.
       */
      XtVaGetValues (widget, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
      while (nkids--)
	  fe_HackTranslations (context, kids [nkids]);
    }
}

/* This installs all the usual translations on Text and TextField widgets.
 */
void
fe_HackTextTranslations (Widget widget)
{ 
  Widget parent = widget;

  for (parent=widget; parent && !XtIsWMShell (parent); parent=XtParent(parent))
    if (XmLIsGrid(parent))
      /* We shouldn't be messing with Grid widget and its children */
      return;

  if (XmIsTextField(widget))
    {
      if (fe_globalData.editing_translations)
	XtOverrideTranslations (widget, fe_globalData.editing_translations);
      if (fe_globalData.single_line_editing_translations)
	XtOverrideTranslations (widget,
			      fe_globalData.single_line_editing_translations);
    }
  else if (XmIsText(widget))
    {
      if (fe_globalData.editing_translations)
	XtOverrideTranslations (widget, fe_globalData.editing_translations);
      if (fe_globalData.multi_line_editing_translations)
	XtOverrideTranslations (widget,
			      fe_globalData.multi_line_editing_translations);
    }
  else
    {
      XP_ASSERT(0);
    }
}


/* This installs all the usual translations for popups/transients.
   All it does is recurse the tree and call fe_HackTextTranslations
   on any Text or TextField widgets it finds.
 */
void
fe_HackDialogTranslations (Widget widget)
{
  if (XmIsGadget (widget))
    return;
  else if (XmIsText (widget) || XmIsTextField (widget))
    fe_HackTextTranslations (widget);
  else
    {
      Widget *kids = 0;
      Cardinal nkids = 0;
      XtVaGetValues (widget, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
      while (nkids--)
	fe_HackDialogTranslations (kids [nkids]);
    }
}


void
fe_RegisterConverters (void)
{
#ifdef NEW_DECODERS
  NET_ClearAllConverters ();
#endif /* NEW_DECODERS */

  if (fe_encoding_extensions)
    {
      int i = 0;
      while (fe_encoding_extensions [i])
	free (fe_encoding_extensions [i++]);
      free (fe_encoding_extensions);
      fe_encoding_extensions = 0;
    }

  /* register X specific decoders
   */
  if (fe_globalData.encoding_filters)
    {
      char *copy = strdup (fe_globalData.encoding_filters);
      char *rest = copy;
      char *end = rest + strlen (rest);

      int exts_count = 0;
      int exts_size = 10;
      char **all_exts = (char **) malloc (sizeof (char *) * exts_size);

      while (rest < end)
	{
	  char *start;
	  char *eol, *colon;
	  char *input, *output, *extensions, *command;
	  eol = strchr (rest, '\n');
	  if (eol) *eol = 0;

	  rest = fe_StringTrim (rest);
	  if (! *rest)
	    /* blank lines are ok */
	    continue;

	  start = rest;

	  colon = strchr (rest, ':');
	  if (! colon) goto LOSER;
	  *colon = 0;
	  input = fe_StringTrim (rest);
	  rest = colon + 1;

	  colon = strchr (rest, ':');
	  if (! colon) goto LOSER;
	  *colon = 0;
	  output = fe_StringTrim (rest);
	  rest = colon + 1;

	  colon = strchr (rest, ':');
	  if (! colon) goto LOSER;
	  *colon = 0;
	  extensions = fe_StringTrim (rest);
	  rest = colon + 1;

	  command = fe_StringTrim (rest);
	  rest = colon + 1;
	  
	  if (*command)
	    {
	      /* First save away the extensions. */
	      char *rest = extensions;
	      while (*rest)
		{
		  char *start;
		  char *comma, *end;
		  while (isspace (*rest))
		    rest++;
		  start = rest;
		  comma = XP_STRCHR (start, ',');
		  end = (comma ? comma - 1 : start + strlen (start));
		  while (end >= start && isspace (*end))
		    end--;
		  if (comma) end++;
		  if (start < end)
		    {
		      all_exts [exts_count] =
			(char *) malloc (end - start + 1);
		      strncpy (all_exts [exts_count], start, end - start);
		      all_exts [exts_count][end - start] = 0;
		      if (++exts_count == exts_size)
			all_exts = (char **)
			  realloc (all_exts,
				   sizeof (char *) * (exts_size += 10));
		    }
		  rest = (comma ? comma + 1 : end);
		}
	      all_exts [exts_count] = 0;
	      fe_encoding_extensions = all_exts;

	      /* Now register the converter. */
	      NET_RegisterExternalDecoderCommand (input, output, command);
	    }
	  else
	    {
	  LOSER:
	      fprintf (stderr, "%s: unparsable encoding filter spec: %s\n",
		       fe_progname, start);
	    }
	  rest = (eol ? eol + 1 : end);
	}
      free (copy);
    }

  /* Register standard decoders
     This must come AFTER all calls to NET_RegisterExternalDecoderCommand(),
     (at least in the `NEW_DECODERS' world.)
   */
  NET_RegisterMIMEDecoders ();

  /* How to save to disk. */
  NET_RegisterContentTypeConverter ("*", FO_SAVE_AS, NULL,
				    fe_MakeSaveAsStream);

  /* Saving any binary format as type `text' should save as `source' instead.
   */
  NET_RegisterContentTypeConverter ("*", FO_SAVE_AS_TEXT, NULL,
				    fe_MakeSaveAsStreamNoPrompt);
  NET_RegisterContentTypeConverter ("*", FO_QUOTE_MESSAGE, NULL,
				    fe_MakeSaveAsStreamNoPrompt);

  /* default presentation converter - offer to save unknown types. */
  NET_RegisterContentTypeConverter ("*", FO_PRESENT, NULL,
				    fe_MakeSaveAsStream);

#if 0
  NET_RegisterContentTypeConverter ("*", FO_VIEW_SOURCE, NULL,
				    fe_MakeViewSourceStream);
#endif

#ifdef MOCHA /* added by jwz */
#ifndef NO_MOCHA_CONVERTER_HACK
  /* libmocha:LM_InitMocha() installs this convert. We blow away all
   * converters that were installed and hence these mocha default converters
   * dont get recreated. And mocha has no call to re-register them.
   * So this hack. - dp/brendan
   */
  NET_RegisterContentTypeConverter(APPLICATION_JAVASCRIPT, FO_PRESENT, 0,
				   NET_CreateMochaConverter);
#endif /* NO_MOCHA_CONVERTER_HACK */
#endif /* MOCHA -- added by jwz */

  /* Parse stuff out of the .mime.types and .mailcap files.
   * We dont have to check dates of files for modified because all that
   * would have been done by the caller. The only place time checking
   * happens is
   * (1) Helperapp page is created
   * (2) Helpers are being saved (OK button pressed on the General Prefs).
   */
  NET_InitFileFormatTypes (fe_globalPrefs.private_mime_types_file,
			   fe_globalPrefs.global_mime_types_file);
  fe_isFileChanged(fe_globalPrefs.private_mime_types_file, 0,
		   &fe_globalData.privateMimetypeFileModifiedTime);

  NET_RegisterConverters (fe_globalPrefs.private_mailcap_file,
			  fe_globalPrefs.global_mailcap_file);
  fe_isFileChanged(fe_globalPrefs.private_mailcap_file, 0,
		   &fe_globalData.privateMailcapFileModifiedTime);

  /* Plugins go on top of all this */
  fe_RegisterPluginConverters();
}

/* The Windows menu in the toolbar */

static void
fe_switch_context_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  struct fe_MWContext_cons *cntx = fe_all_MWContexts;

  /* We need to watchout here. If we were holding down the menu and
   * before we select, a context gets destroyed (ftp is done say), then
   * selecting on that item would give an invalid context here. Guard
   * against that case by validating the context.
   */
  
  while (cntx && (cntx->context != context))
    cntx = cntx->next;
  if (!cntx)
    /* Ah ha! invalid context. I told you! */
    return;
  
  XMapRaised(XtDisplay(CONTEXT_WIDGET(context)),
		   XtWindow(CONTEXT_WIDGET(context)));
  return;
}

void 
fe_SetString(Widget widget, const char* propname, char* str)
{
  XmString xmstr;
  xmstr = XmStringCreate(str, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(widget, propname, xmstr, 0);
  XmStringFree(xmstr);
}

static void
fe_addto_windows_menu(Widget w, MWContext *context, MWContextType lookfor)
{
  Widget menu = CONTEXT_DATA (context)->windows_menu;
  Widget button = w;
  char buf[1024], *title = 0, *s;
  struct fe_MWContext_cons *cntx = NULL;
  Boolean more = True;		/* some contexts are always single. In those
				   cases, this will become False and stop
				   further search */

  for (cntx = fe_all_MWContexts; more && cntx; cntx = cntx->next)
    if (cntx->context->type == lookfor) {
      /* Fix title */
      switch (lookfor) {
	case MWContextMail:
  	    title = XP_GetString( XFE_NETSCAPE_MAIL );
	    more = False;
	    break;
	case MWContextNews:
  	    title = XP_GetString( XFE_NETSCAPE_NEWS );
	    more = False;
	    break;
	case MWContextBookmarks:
  	    title = XP_GetString( XFE_BOOKMARKS );
	    more = False;
	    break;
	case MWContextAddressBook:
  	    title = XP_GetString( XFE_ADDRESS_BOOK );
	    more = False;
	    break;
	case MWContextSaveToDisk:
	    button = 0;
	    title =
		XmTextFieldGetString (CONTEXT_DATA(cntx->context)->url_label);
	    XP_ASSERT(title);
	    s = fe_Basename(title);
	    fe_MidTruncateString (s, s, 25);
	    PR_snprintf(buf, sizeof (buf), XP_GetString( XFE_DOWNLOAD_FILE ), s );
	    XtFree(title);
	    title = buf;
	    break;
	case MWContextMessageComposition:
	    button = 0;
	    title = cntx->context->title;
	    if (!title) {
		title = buf;
		PR_snprintf(buf, sizeof (buf), XP_GetString( XFE_COMPOSE_NO_SUBJECT ) );
	    }
	    else {
		s = XP_STRDUP(title);
	        fe_MidTruncateString (s, s, 25);
		PR_snprintf(buf, sizeof (buf), XP_GetString( XFE_COMPOSE ), s );
		XP_FREE(s);
		title = buf;
	    }
	    break;
	case MWContextBrowser:
	case MWContextEditor:
	    button = 0;
	    /* Dont process grid children */
	    if (cntx->context->is_grid_cell) continue;
	    title = cntx->context->title;
#if defined(EDITOR) && defined(EDITOR_UI)
	    if (!title || title[0] == '\0') {
	      History_entry *he = SHIST_GetCurrent(&cntx->context->hist);
	      char *url = (he && he->address ? he->address : 0);
	      if (url != NULL && strcmp(url, "file:///Untitled") != 0)
	        title = url;
	    }
#endif /*defined(EDITOR) && defined(EDITOR_UI)*/
	    if (!title || !(*title)) {
	      title = buf;
	      PR_snprintf(buf, sizeof (buf), XP_GetString( XFE_NETSCAPE_UNTITLED ) );
	    }
	    else {
		unsigned char *loc;

		s = strdup(title);
		title = buf;
		fe_MidTruncateString (s, s, 35);
		loc = fe_ConvertToLocaleEncoding(cntx->context->win_csid,
						 (unsigned char *) s);
		PR_snprintf(buf, sizeof (buf), XP_GetString( XFE_NETSCAPE ),
			    loc);
		if ((char *) loc != s) {
		    XP_FREE(loc);
		}
		XP_FREE(s);
	    }
	    break;
        default:
	  assert(0);
	  break;
      }
      if (!button)
	button = XmCreateToggleButtonGadget (menu, "windowButton", NULL, 0);
      XtVaSetValues(button, XmNset, (context == cntx->context),
			XmNvisibleWhenOff, False, 0);
      XtAddCallback (button, XmNvalueChangedCallback, fe_switch_context_cb,
			cntx->context);

#ifdef DEBUG_jwz
      {
	MWContext *cx = cntx->context;
	Widget w = CONTEXT_WIDGET(cx);
	Bool busy = XP_IsContextBusy(cx);
	Bool iconic = False;

	if (XtWindow(w) &&
	    cx->type != MWContextMail &&
	    cx->type != MWContextNews &&
	    cx->type != MWContextBookmarks &&
	    cx->type != MWContextAddressBook)
	  {
	    XWindowAttributes xgwa;
	    XGetWindowAttributes (XtDisplay(w), XtWindow(w), &xgwa);
	    iconic = (xgwa.map_state != IsViewable);
	  }

	if (busy || iconic)
	  {
	    const char *s = (iconic && busy		/* ####I18N */
			     ? " (iconic, busy)"
			     : (iconic
				? " (iconic)"
				: " (busy)"));
	    if (title != buf)
	      strcpy (buf, title);
	    title = buf;
	    strcat (title, s);
	  }
      }
#endif /* DEBUG_jwz */

      fe_SetString(button, XmNlabelString, title);
      XtManageChild(button);
    }
    /* If nothing found, turn button off */
    if (w && !cntx)
      XtVaSetValues(w, XmNset, False, XmNvisibleWhenOff, False, 0);
}

void
fe_GenerateWindowsMenu (MWContext *context)
{
  Widget menu = CONTEXT_DATA (context)->windows_menu;
  Widget *kids = 0;
  Cardinal nkids = 0, kid;

  if (menu == NULL || CONTEXT_DATA (context)->windows_menu_up_to_date_p)
    return;

  XtVaGetValues (menu, XmNchildren, &kids, XmNnumChildren, &nkids, 0);

  if (context->type == MWContextBrowser)
    XP_ASSERT(nkids >= 7);
  else
    XP_ASSERT(nkids >= 6);

  kid = 0;

  fe_addto_windows_menu(kids[kid++], context, MWContextMail);
  fe_addto_windows_menu(kids[kid++], context, MWContextNews);

  /* next is separator */
  kid++;

#ifdef DEBUG_jwz
  kid += 2;    /* "openBrowser" and <HR> */
#endif /* DEBUG_jwz */

  fe_addto_windows_menu(kids[kid++], context, MWContextAddressBook);
  fe_addto_windows_menu(kids[kid++], context, MWContextBookmarks);

  /* next is history for MWContextBrowser only */
  if (context->type == MWContextBrowser)
      XtVaSetValues(kids[kid++], XmNset, False, XmNvisibleWhenOff, False, 0);

  /* next is separator */
  kid++;

  /* Destroy the rest of the children buttons */
  XtUnmanageChildren (&kids[kid], nkids-kid);
  for(; kid < nkids; kid++) XtDestroyWidget(kids[kid]);

  fe_addto_windows_menu(0, context, MWContextBrowser);
#if defined(EDITOR) && defined(EDITOR_UI)
  fe_addto_windows_menu(0, context, MWContextEditor);
#endif
  fe_addto_windows_menu(0, context, MWContextMessageComposition);
  fe_addto_windows_menu(0, context, MWContextSaveToDisk);

  CONTEXT_DATA (context)->windows_menu_up_to_date_p = True;
}


