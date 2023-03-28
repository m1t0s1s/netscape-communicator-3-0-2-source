/* -*- Mode: C; tab-width: 8 -*-
   xfe.c --- other junk specific to the X front end.
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Jun-94.
 */

#include "mozilla.h"
#include "xfe.h"
#include "fonts.h"
#include "felocale.h"
#include "il.h"                 /* Image library */
#include "java.h"

#include <X11/IntrinsicP.h>	/* Xt sucks */
#include <X11/ShellP.h>
#include <X11/Xatom.h>

#ifdef EDITOR
#include "xeditor.h"
#endif /* EDITOR */
#include <Xm/Label.h>

#ifdef AIXV3
#include <sys/select.h>
#endif /* AIXV3 */

#include <sys/time.h>

#include <np.h>

#include "msgcom.h"
#include "secnav.h"
#include "mozjava.h"
#ifdef MOCHA
#include "libmocha.h"
#endif

#if defined(_HPUX_SOURCE)
/* I don't know where this is coming from...  "ld -y Error" says
   /lib/libPW.a(xlink.o): Error is DATA UNSAT
   /lib/libPW.a(xmsg.o): Error is DATA UNSAT
 */
int Error = 0;
#endif

#include "libi18n.h"

#ifdef DEBUG_jwz
extern XP_Bool IL_AnimationsEnabled;
extern XP_Bool IL_AnimationLoopsEnabled;
extern XP_Bool fe_ignore_font_size_changes_p;
#endif /* DEBUG_jwz */



/* DEBUG_JWZ from mozilla 5 */
extern XP_Bool fe_UseAsyncDNS(void);


/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_DOCUMENT_DONE;
extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_STDERR_DIAGNOSTICS_HAVE_BEEN_TRUNCATED;
extern int XFE_ERROR_CREATING_PIPE;

int fe_WindowCount = 0;
MWContext *someGlobalContext = NULL;
static Boolean has_session_mgr= False;

struct fe_MWContext_cons *fe_all_MWContexts = 0;

static void fe_delete_cb (Widget, XtPointer, XtPointer);
static void fe_save_history_timer (XtPointer closure, XtIntervalId *id);
static void fe_initialize_stderr (void);
extern void fe_MakeBookmarkWidgets(Widget shell, MWContext *context);

#ifdef DEBUG_jwz
int
_Xsetlocale (void)   /* no fucking clue what this should do: stub it out */
{
  return 0;
}
#endif /* DEBUG_jwz */


void
XFE_EnableClicking (MWContext *context)
{
  MWContext *top = fe_GridToTopContext (context);
  XP_Bool running_p;

  running_p = XP_IsContextBusy (top);

  if (CONTEXT_DATA (top)->show_toolbar_p && CONTEXT_DATA (top)->toolbar) {
    if (CONTEXT_DATA (top)->abort_button)
      XtVaSetValues (CONTEXT_DATA (top)->abort_button,
		   XmNsensitive, XP_IsContextStoppable(top), 0);
  }
  if (CONTEXT_DATA (top)->show_menubar_p && CONTEXT_DATA (top)->menubar) {
    if (CONTEXT_DATA (top)->abort_menuitem)
      XtVaSetValues (CONTEXT_DATA (top)->abort_menuitem,
		   XmNsensitive, XP_IsContextStoppable(top), 0);

#if defined(DEBUG_jwz) && defined(MOCHA)
    /* this really doesn't belong here... */
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
      !fe_ignore_font_size_changes_p, 0);
#endif /* DEBUG_jwz */
  }

  if (! running_p)
    fe_StopProgressGraph (top);

  if (CONTEXT_DATA (context)->clicking_blocked)
    {
      CONTEXT_DATA (context)->clicking_blocked = False;
      /* #### set to link cursor if over link. */
      fe_SetCursor (context, False);
    }
}

void
FE_UpdateStopState(MWContext *context)
{
  MWContext *top = fe_GridToTopContext (context);

  if (context->type == MWContextPostScript) return;

  if (CONTEXT_DATA (top)->show_toolbar_p && CONTEXT_DATA (top)->toolbar) {
    if (CONTEXT_DATA (top)->abort_button)
      XtVaSetValues (CONTEXT_DATA (top)->abort_button,
		   XmNsensitive, XP_IsContextStoppable(top), 0);
  }
  if (CONTEXT_DATA (top)->show_menubar_p && CONTEXT_DATA (top)->menubar) {
    if (CONTEXT_DATA (top)->abort_menuitem)
      XtVaSetValues (CONTEXT_DATA (top)->abort_menuitem,
		   XmNsensitive, XP_IsContextStoppable(top), 0);

#if defined(DEBUG_jwz) && defined(MOCHA)
    /* this really doesn't belong here... */
    if (CONTEXT_DATA (context)->toggleJS_menuitem)
      XtVaSetValues (CONTEXT_DATA (context)->toggleJS_menuitem, XmNset,
                     !fe_globalPrefs.disable_javascript, 0);
#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
    /* this really doesn't belong here... */
    if (CONTEXT_DATA (context)->toggleAnim_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleAnim_menuitem, XmNset,
      IL_AnimationsEnabled, 0);
    if (CONTEXT_DATA (context)->toggleLoops_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleLoops_menuitem, XmNset,
      (IL_AnimationsEnabled && IL_AnimationLoopsEnabled), 0);
    if (CONTEXT_DATA (context)->toggleSizes_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleSizes_menuitem, XmNset,
      !fe_ignore_font_size_changes_p, 0);
#endif /* DEBUG_jwz */
  }
}

void
fe_url_exit (URL_Struct *url, int status, MWContext *context)
{
  assert (status == MK_INTERRUPTED ||
	  CONTEXT_DATA (context)->active_url_count > 0);

  if (CONTEXT_DATA (context)->active_url_count > 0)
    CONTEXT_DATA (context)->active_url_count--;

  /* We should be guarenteed that XFE_AllConnectionsComplete() will be called
     right after this, if active_url_count has just gone to 0. */
  if (status < 0 && url->error_msg)
    {
      FE_Alert (context, url->error_msg);
    }
#if defined(EDITOR) && defined(EDITOR_UI)
  /*
   *    Do stuff specific to the editor
   */
  if (context->type == MWContextEditor)
    fe_EditorGetUrlExitRoutine(context, url, status);
#endif
  if (status != MK_CHANGING_CONTEXT)
    {
      NET_FreeURLStruct (url);
    }
}

void
XFE_AllConnectionsComplete (MWContext *context)
{
  MWContext *top = fe_GridToTopContext (context);

  if (CONTEXT_DATA (context)->being_destroyed) return;

  /* #### ARRGH, when generating PostScript, this assert gets tripped!
     I guess this is because of the goofy games it plays with invoking
     callbacks in the "other" context...
  assert (CONTEXT_DATA (context)->active_url_count == 0);
  */

  /* If a title was never specified, clear the old one. */
  if ((context->type != MWContextBookmarks) &&
	(context->type != MWContextAddressBook) &&
	(CONTEXT_DATA (context)->active_url_count == 0) &&
	(context->title == 0))
    XFE_SetDocTitle (context, 0);

  /* This shouldn't be necessary, but it doesn't hurt. */
  XFE_EnableClicking (context);

  fe_RefreshAllAnchors ();

  /* If the user resized during layout, reload the document right now
     (it will come from the cache.) */

#if 0
printf ("ConnectionsComplete: 0x%x; grid: %d; busy: %d, relayout: %d\n",
	 context, context->is_grid_cell, XP_IsContextBusy (context),
	 CONTEXT_DATA (context)->relayout_required);
#endif

  if (CONTEXT_DATA (context)->relayout_required)
    {
      if (! XP_IsContextBusy (context))
	{
	  CONTEXT_DATA (context)->relayout_required = False;
	  fe_ReLayout (context, NET_RESIZE_RELOAD);
	}
    }
  else if (context->is_grid_cell && CONTEXT_DATA (top)->relayout_required)
    {
      if (! XP_IsContextBusy (top))
	{
	  CONTEXT_DATA (top)->relayout_required = False;
	  fe_ReLayout (top, NET_RESIZE_RELOAD);
	}
    }
  else
    {
      FE_Progress (top, XP_GetString(XFE_DOCUMENT_DONE));
    }

  if (context->type == MWContextMessageComposition) /* gag gag gag */
    MSG_MailCompositionAllConnectionsComplete (context);
  else if (context->type == MWContextSaveToDisk) /* gag gag gag */
    fe_DestroySaveToDiskContext (context);
}

extern int logomatic;

int
FE_GetURL (MWContext *context, URL_Struct *url)
{
  return fe_GetURL (context, url, FALSE);
}

/* View the URL in the given context.
   A pointer to url_string is not retained.
   Returns the status of NET_GetURL().
   */
int
fe_GetURL (MWContext *context, URL_Struct *url, Boolean skip_get_url)
{
  int32 x, y;
  LO_Element *e;
  History_entry *he;
  MWContext *new_context = NULL;

#ifdef JAVA
  /*
  ** This hack is to get around the crash when we have an
  ** auto-config proxy installed, and we come up in the mail
  ** window and get a message with an applet on it:
  */
  if (context->type == MWContextJava) {
      context = nsn_JavaContextToRealContext(context);
  }
#endif
  /* If this URL requires a particular kind of window, and this is not
     that kind of window, then we need to find or create one.
   */
  if (!skip_get_url && 
#ifdef EDITOR
      !(context->is_editor) &&
#endif
      MSG_NewWindowRequired (context, url->address))
    {
      XP_ASSERT (!MSG_NewWindowProhibited (context, url->address));
      if ( MSG_RequiresBrowserWindow(url->address) )
      {
        /* Find to see if there is a browser window */
        new_context = XP_FindContextOfType(context,MWContextBrowser );
      }
 
      if (!new_context )
      {
         context = fe_MakeWindow (XtParent (CONTEXT_WIDGET (context)), context,
                               url, NULL, MWContextBrowser, FALSE);
         return (context ? 0 : -1);
      }
      else
      {
	  /* If we find a new browser context, use it to display URL */
	  if (context != new_context)
	      /* If we have picked an existing context that isn't this
		 one in which to display this document, make sure that
		 context is uniconified and raised first. */
	      XMapRaised(XtDisplay(CONTEXT_WIDGET(new_context)),
			 XtWindow(CONTEXT_WIDGET(new_context)));
	  context = new_context;
	
      }
    }

  e = LO_XYToNearestElement (context,
			     CONTEXT_DATA (context)->document_x,
			     CONTEXT_DATA (context)->document_y);
  he = SHIST_GetCurrent (&context->hist);
  if (e)
    SHIST_SetPositionOfCurrentDoc (&context->hist, e->lo_any.ele_id);

  /* LOU: don't do this any more since Layout will do it
   * for you once the page starts loading
   *
   * Here is why we have to do this for now...
   * 
   * | This is important feedback on this new "feature" (or misfeature as
   * | it may turn out).
   * 
   * If this is the feature where the front end no longer calls
   * XP_InterruptContext() inside fe_GetURL(), but only when data from
   * the new URL arrives, then I think I understand what's going on.
   * 
   * | I didn't expect the images that are currently transfering to interfere
   * | significantly with the TCP connection setup,
   * 
   * They won't interfere within TCP or IP modules on intervening routers.
   * ISDN signalling guarantees perfect scheduling of the D-channel; modem
   * link layers are fair.
   * 
   * The problem is this: before, when fe_GetURL() called XP_InterruptContext(),
   * the connections for the old document were closed immediately.  Those TCP
   * closes turned into packets with the FIN flag set on the wire, which went to
   * the server over the fairly-scheduled link.  The client moved into TCP state
   * FIN_WAIT_1, indicating that it had sent a FIN but needed an ACK, and that
   * the server had not yet closed its half of the connection.  As soon as the
   * next TCP segment for this connection arrived at the client, the client TCP
   * noticed that the user process had closed, so it aborted the connection by
   * sending a RST.
   * 
   * Since the change to the front ends, it appears the old connections don't
   * (all) get closed until data on the new connection arrives.  The data for
   * those old connections that are not yet closed will still be queued in the
   * server's TCP, and sent as the client ACKs previous data and updates the
   * server's window.  There is no way for an HTTP daemon to cancel the write
   * or send syscalls that enqueued this data, once the syscall has returned.
   * Only a RST from the client for each old connection will cause the server
   * to kill its enqueued data.
   * 
   * I doubt there is much data in the fairly narrow pipe between server and
   * client.  Because the pipe is narrow, the server TCP has not opened its
   * congestion window, and there cannot be much data in flight.  The problem
   * is lack of abort because of lack of close() when the new URL is clicked.
   * 
   * | Perhaps there is some TCP magic that we can do
   * | to cause the new stream to have absolute priority over other TCP
   * | streams.
   * 
   * No such magic, nor any need for it in this scenario.
   * 
   * | I could also pause all the transfering connections while
   * | we are waiting.
   * 
   * Only a close, which guarantees an abort when more old data arrives, will
   * do the trick.  Anything else requires server mods as well as client.
   */
  if (!skip_get_url)
    XP_InterruptContext (context);

  if (CONTEXT_DATA (context)->refresh_url_timer)
    {
      XtRemoveTimeOut (CONTEXT_DATA (context)->refresh_url_timer);
      CONTEXT_DATA (context)->refresh_url_timer = 0;
    }
  if (CONTEXT_DATA (context)->refresh_url_timer_url)
    {
      free (CONTEXT_DATA (context)->refresh_url_timer_url);
      CONTEXT_DATA (context)->refresh_url_timer_url = 0;
    }

#if 0
  if (he && he->address && url && url->address
      && strcmp (he->address, url->address)	/* not just reloading. */
      && LO_LocateNamedAnchor (context, url, &x, &y))
    {
      /* This URL is an index into the current document, so we just
	 need to scroll instead of going through the network and cache.
       */
      fe_ScrollTo (context, 0, y);
      fe_SetURLString (context, url);
      fe_AddHistory (context, url, context->title);
      GH_UpdateGlobalHistory (url);
      NET_FreeURLStruct (url);
      fe_RefreshAllAnchors ();
      return 0;
    }
#else
  if (he && he->address && url && url->address
      && strcmp (he->address, url->address)	/* not just reloading. */
      && XP_FindNamedAnchor (context, url, &x, &y))
    {
      char *temp;
      URL_Struct *urlcp;
      
      fe_ScrollTo (context, 0, y);
      fe_SetURLString (context, url);

      /* Create URL from prev history entry to preserve security, etc. */
      urlcp = SHIST_CreateURLStructFromHistoryEntry(context, he);

      /*  Swap addresses. */
      temp = url->address;
      url->address = urlcp->address;
      urlcp->address = temp;
	
      /* set history_num correctly */
      urlcp->history_num = url->history_num;
      
      /*  Free old URL, and reassign. */
      NET_FreeURLStruct(url);
      url = urlcp;
      fe_AddHistory (context, url, context->title);
      NET_FreeURLStruct (url);
      fe_RefreshAllAnchors ();
      return 0;
    }
#endif
  else
    {
      char s[] = "\163\256\273\276\163";
      if (logomatic != 1 && url && url->address)
	{
	  char *s2;
	  logomatic = 0;
	  for (s2 = s; *s2; s2++) *s2 -= 68;
	  if ((s2 = strstr (url->address, s)))
	    {
	      char *a = strchr (url->address, '&');
	      char *b = strchr (url->address, '?');
	      if (!a || b < a) a = b;
	      if (!a || a > s2)
		{
		  char *host = NET_ParseURL (url->address, GET_HOST_PART);
		  if (host) {
		    for (s2 = host; *s2; s2++) *s2 += 67;
		    if (strstr (host, "\261\250\267\266\246\244\263\250\161"
				"\246\262\260") ||
			strstr (host, "\250\252\265\250\252\254\262\270\266"
				"\161\246\262\260") ||
			strstr (host, "\257\262\266\266\244\252\250\161\246"
				"\262\260") ||
			strstr (host, "\255\272\275\161\246\262\260"))
		      logomatic = 2;
		    free (host);
		  }
		}
	    }
	}

#ifdef EDITOR
      /*
       *    We have to pass "about:" docs through the old way,
       *    cause there's all this magic that happens! We do trap
       *    these in fe_EditorNew() so the user will not be allowed
       *    to edit them, but *we* do GetURLs on them as well.
       */
      if (EDT_IS_EDITOR(context) && strncasecmp(url->address, "about:", 6)!= 0)
	  return fe_GetSecondaryURL (context, url, FO_EDIT, 0,
				        skip_get_url);
      else
#endif
	  return fe_GetSecondaryURL (context, url, FO_CACHE_AND_PRESENT, 0,
					skip_get_url);
    }
}


/* Start loading a URL other than the main one (ie, an embedded image.)
   A pointer to url_string is not retained.
   Returns the status of NET_GetURL().
 */
int
fe_GetSecondaryURL (MWContext *context,
		    URL_Struct *url_struct,
		    int output_format,
		    void *call_data, Boolean skip_get_url)
{
  /* The URL will be freed when libnet calls the fe_url_exit() callback. */
  url_struct->fe_data = call_data;
  CONTEXT_DATA (context)->active_url_count++;
  if (CONTEXT_DATA (context)->active_url_count == 1)
    {
      CONTEXT_DATA (context)->clicking_blocked = True;

#ifdef GRIDS
      if (context->is_grid_cell)
        {
          MWContext *top_context = context;
          while ((top_context->grid_parent)&&
		(top_context->grid_parent->is_grid_cell))
            {
	      top_context = top_context->grid_parent;
            }
          fe_StartProgressGraph (top_context->grid_parent);
        }
      else
        {
	  fe_StartProgressGraph (context);
        }
#else
      fe_StartProgressGraph (context);
#endif /* GRIDS */

      fe_SetCursor (context, False);
    }

  if (CONTEXT_DATA (context)->show_toolbar_p)
    {
      if (CONTEXT_DATA (context)->abort_button)
        XtVaSetValues (CONTEXT_DATA (context)->abort_button,
		   XmNsensitive, True, 0);
    }
  if (CONTEXT_DATA (context)->show_menubar_p)
    {
      if (CONTEXT_DATA (context)->abort_menuitem)
        XtVaSetValues (CONTEXT_DATA (context)->abort_menuitem,
		   XmNsensitive, True, 0);

#if defined(DEBUG_jwz) && defined(MOCHA)
    /* this really doesn't belong here... */
    if (CONTEXT_DATA (context)->toggleJS_menuitem)
      XtVaSetValues (CONTEXT_DATA (context)->toggleJS_menuitem, XmNset,
                     !fe_globalPrefs.disable_javascript, 0);
#endif /* DEBUG_jwz && MOCHA */

#ifdef DEBUG_jwz
    /* this really doesn't belong here... */
    if (CONTEXT_DATA (context)->toggleAnim_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleAnim_menuitem, XmNset,
      IL_AnimationsEnabled, 0);
    if (CONTEXT_DATA (context)->toggleLoops_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleLoops_menuitem, XmNset,
      (IL_AnimationsEnabled && IL_AnimationLoopsEnabled), 0);
    if (CONTEXT_DATA (context)->toggleSizes_menuitem)
     XtVaSetValues (CONTEXT_DATA (context)->toggleSizes_menuitem, XmNset,
      !fe_ignore_font_size_changes_p, 0);
#endif /* DEBUG_jwz */
    }

  /* Pop down dialogs or whatever before disappearing down into
     the wilds of gethostbyname().  It'd be nice if we got to
     process all pending expose events here too, but that's hard. */
  XSync (XtDisplay (CONTEXT_WIDGET (context)), False);

  if (skip_get_url)
    {
      return(0);
    }

  return NET_GetURL (url_struct, output_format, context, fe_url_exit);
}



static XtInputId fe_fds_to_XtInputIds [FD_SETSIZE] = { 0, };

static void
fe_stream_callback (void *closure, int *source, XtInputId *id)
{
#ifdef JAVA /*  jwz */
    extern int _pr_x_locked;
    assert(PR_InMonitor(_pr_rusty_lock) && _pr_x_locked);
#endif

#ifdef QUANTIFY
quantify_start_recording_data();
#endif /* QUANTIFY */
  NET_ProcessNet (*source, NET_UNKNOWN_FD);
#ifdef QUANTIFY
quantify_stop_recording_data();
#endif /* QUANTIFY */
}


static void
fe_add_input (int fd, int mask)
{
  if (fd < 0 || fd >= countof (fe_fds_to_XtInputIds))
    abort ();

  LOCK_FDSET();

  if (fe_UseAsyncDNS() &&
      fe_fds_to_XtInputIds [fd])
    /* If we're already selecting input on this fd, don't select it again
       and lose our pointer to the XtInput object..  This shouldn't happen,
       but netlib does this when async DNS lookups are happening.  (This
       will lose if the `mask' arg has changed on two consecutive calls
       without an intervening call to `fe_remove_input', but that doesn't
       happen.)  -- jwz, 9-Jan-97.
     */
    goto DONE;

  fe_fds_to_XtInputIds [fd] = XtAppAddInput (fe_XtAppContext, fd,
					     (XtPointer) mask,
					     fe_stream_callback, 0);
#ifdef JAVA /*  jwz */
  if (PR_CurrentThread() != mozilla_thread) {
      /*
      ** Sometimes a non-mozilla thread will be using the netlib to fetch
      ** data. Because mozilla is the only thread that runs the netlib
      ** "select" code, we need to be able to kick mozilla and wake it up
      ** when the select set has changed.
      **
      ** A way to do this would be to have mozilla stop calling select
      ** and instead manually manipulate the idle' thread's select set,
      ** but there is yet to be an NSPR interface at that level.
      */
      PR_PostEvent(mozilla_event_queue, NULL);
  }
#endif


DONE:
  UNLOCK_FDSET();
}

static void
fe_remove_input (int fd)
{
  if (fd < 0 || fd >= countof (fe_fds_to_XtInputIds))
    return;	/* was abort() -- whh */

  LOCK_FDSET();

  if (fe_fds_to_XtInputIds [fd] != 0) {
      XtRemoveInput (fe_fds_to_XtInputIds [fd]);
      fe_fds_to_XtInputIds [fd] = 0;
  }

  UNLOCK_FDSET();
}

void
FE_SetReadSelect (MWContext *context, int fd)
{
  fe_add_input (fd, XtInputReadMask | XtInputExceptMask);
}

void
FE_SetConnectSelect (MWContext *context, int fd)
{
  fe_add_input (fd, (XtInputWriteMask | XtInputExceptMask));
}

void
FE_ClearReadSelect (MWContext *context, int fd)
{
  fe_remove_input (fd);
}

void
FE_ClearConnectSelect (MWContext *context, int fd)
{
  FE_ClearReadSelect (context, fd);
}

void
FE_ClearFileReadSelect (MWContext *context, int fd)
{
  FE_ClearReadSelect (context, fd);
}

void
FE_SetFileReadSelect (MWContext *context, int fd)
{
  FE_SetReadSelect (context, fd);
}


/* making contexts and windows */

static unsigned int
default_char_width (int charset, fe_Font font)
{
  XCharStruct overall;
  int ascent, descent;
  FE_TEXT_EXTENTS(charset, font, "n", 1, &ascent, &descent, &overall);
  return overall.width;
}


static void fe_pick_visual_and_colormap (Widget toplevel,
					 MWContext *new_context);
static void fe_get_context_resources (MWContext *context);
static void fe_get_final_context_resources (MWContext *context);

void
FE_DestroyWindow(MWContext *context)
{
    fe_delete_cb (0, context, NULL);
    return;
}

MWContext *
fe_MakeWindow(Widget toplevel, MWContext *context_to_copy,
			URL_Struct *url, char *window_name, MWContextType type,
			Boolean skip_get_url)
{
    return fe_MakeNewWindow(toplevel, context_to_copy, url, window_name, type,
			    skip_get_url, NULL);
}

static void
fe_position_download_context(MWContext *context)
{
  MWContext *active_context = NULL;
  Widget shell = CONTEXT_WIDGET(context);

  XP_ASSERT(context->type == MWContextSaveToDisk);

  /* Lets position this ourselves. If window manager interactive placement
     is on and this context doesn't exist for more than a few microsecs,
     then the user would see this as a outline, place it somewhere but
     wouldn't see a window at all.
  */
  if (fe_all_MWContexts->next)
    active_context = fe_all_MWContexts->next->context;
  if (active_context) {
    WMShellWidget wmshell = (WMShellWidget) shell;
    Widget parent = CONTEXT_WIDGET(active_context);
    Screen* screen = XtScreen (parent);
    Dimension screen_width = WidthOfScreen (screen);
    Dimension screen_height = HeightOfScreen (screen);
    Dimension parent_width = 0;
    Dimension parent_height = 0;
    Dimension child_width = 0;
    Dimension child_height = 0;
    Position x;
    Position y;
    XSizeHints size_hints;

    XtRealizeWidget (shell);  /* to cause its size to be computed */

    XtVaGetValues(shell,
		      XtNwidth, &child_width, XtNheight, &child_height, 0);
    XtVaGetValues (parent,
		       XtNwidth, &parent_width, XtNheight, &parent_height, 0);

    x = (((Position)parent_width) - ((Position)child_width)) / 2;
    y = (((Position)parent_height) - ((Position)child_height)) / 2;
    XtTranslateCoords (parent, x, y, &x, &y);

    if ((Dimension) (x + child_width) > screen_width)
      x = screen_width - child_width;
    if (x < 0)
      x = 0;

    if ((Dimension) (y + child_height) > screen_height)
      y = screen_height - child_height;
    if (y < 0)
      y = 0;

    XtVaSetValues (shell, XtNx, x, XtNy, y, 0);

    /* Horrific kludge because Xt sucks */
    wmshell->wm.size_hints.flags &= (~PPosition);
    wmshell->wm.size_hints.flags |= USPosition;
    if (XGetNormalHints (XtDisplay(shell), XtWindow(shell), &size_hints)) {
      size_hints.x = wmshell->wm.size_hints.x;
      size_hints.y = wmshell->wm.size_hints.y;
      size_hints.flags &= (~PPosition);
      size_hints.flags |= USPosition;
      XSetNormalHints (XtDisplay(shell), XtWindow(shell), &size_hints);
    }
  }
}

static void
fe_find_scrollbar_sizes(MWContext *context)
{
  /* Figure out how much space the horizontal and vertical scrollbars take up.
     It's basically impossible to determine this before creating them...
   */
  if (CONTEXT_DATA(context)->scrolled) {
    Dimension w1 = 0, w2 = 0;

    XtVaGetValues (CONTEXT_DATA (context)->scrolled, XmNwidth, &w1, 0);
    XtVaGetValues (CONTEXT_DATA (context)->drawing_area, XmNwidth, &w2, 0);
    CONTEXT_DATA (context)->scrolled_width = (unsigned long) w2;
    CONTEXT_DATA (context)->sb_w = w1 - w2;

    XtVaGetValues (CONTEXT_DATA (context)->scrolled, XmNheight, &w1, 0);
    XtVaGetValues (CONTEXT_DATA (context)->drawing_area, XmNheight, &w2, 0);
    CONTEXT_DATA (context)->scrolled_height = (unsigned long) w2;
    CONTEXT_DATA (context)->sb_h = w1 - w2;

    /* Now that we know, we don't need them any more. */
    XtUnmanageChild (CONTEXT_DATA (context)->hscroll);
    XtUnmanageChild (CONTEXT_DATA (context)->vscroll);
  }
}


static void
fe_load_default_font(MWContext *context)
{
  fe_Font font;
  int ascent;
  int descent;
  int16 charset;

  charset = CS_LATIN1;
  font = fe_LoadFont(context, &charset, 0, 3, 0);
  if (!font)
  {
	CONTEXT_DATA (context)->line_height = 17;
	return;
  }
  FE_FONT_EXTENTS(CS_LATIN1, font, &ascent, &descent);
  CONTEXT_DATA (context)->line_height = ascent + descent;
}

static void
fe_focus_notify_eh (Widget w, XtPointer closure, XEvent *ev, Boolean *cont)
{
  MWContext *context = (MWContext *) closure;

  TRACEMSG (("fe_focus_notify_eh\n"));
  switch (ev->type) {
#ifdef MOCHA
  case FocusIn:
    TRACEMSG (("focus in\n"));
    LM_SendOnFocus (context, NULL);
    break;
  case FocusOut:
    TRACEMSG (("focus out\n"));
    LM_SendOnBlur (context, NULL);
    break;
#endif /* MOCHA */
  }
}

static void
fe_map_notify_eh (Widget w, XtPointer closure, XEvent *ev, Boolean *cont)
{
#ifdef JAVA
    MWContext *context = (MWContext *) closure;
    switch (ev->type) {
    case MapNotify:
	LJ_UniconifyApplets(context);
	break;
    case UnmapNotify:
	LJ_IconifyApplets(context);
	break;
    }
#endif /* JAVA */
}

static void
fe_copy_context_settings(MWContext *to, MWContext *from)
{
  fe_ContextData* dto;

  if (!to) return;

  dto = CONTEXT_DATA(to);
  if (from) {
    fe_ContextData* dfrom = CONTEXT_DATA(from);
    dto->show_url_p		= dfrom->show_url_p;
    dto->show_toolbar_p		= dfrom->show_toolbar_p;
    dto->show_toolbar_icons_p	= dfrom->show_toolbar_icons_p;
    dto->show_toolbar_text_p	= dfrom->show_toolbar_text_p;
#ifdef EDITOR
    dto->show_character_toolbar_p = dfrom->show_character_toolbar_p;
    dto->show_paragraph_toolbar_p = dfrom->show_paragraph_toolbar_p;
#endif
    dto->show_directory_buttons_p = dfrom->show_directory_buttons_p;
    dto->show_menubar_p           = dfrom->show_menubar_p;
    dto->show_bottom_status_bar_p = dfrom->show_bottom_status_bar_p;
#ifdef HAVE_SECURITY
    dto->show_security_bar_p	= dfrom->show_security_bar_p;
#endif
    dto->autoload_images_p	= dfrom->autoload_images_p;
    dto->delayed_images_p	= dfrom->delayed_images_p;
    dto->force_load_images	= 0;
    dto->fancy_ftp_p		= dfrom->fancy_ftp_p;
    dto->xfe_doc_csid		= dfrom->xfe_doc_csid;
  }
  else {
    dto->show_url_p		= fe_globalPrefs.show_url_p;
    dto->show_toolbar_p		= fe_globalPrefs.show_toolbar_p;
#ifdef EDITOR
    dto->show_character_toolbar_p = fe_globalPrefs.editor_character_toolbar;
    dto->show_paragraph_toolbar_p = fe_globalPrefs.editor_paragraph_toolbar;
#endif
    dto->show_toolbar_icons_p	= fe_globalPrefs.toolbar_icons_p;
    dto->show_toolbar_text_p	= fe_globalPrefs.toolbar_text_p;
    dto->show_directory_buttons_p = fe_globalPrefs.show_directory_buttons_p;
    dto->show_menubar_p           = fe_globalPrefs.show_menubar_p;
    dto->show_bottom_status_bar_p = fe_globalPrefs.show_bottom_status_bar_p;
#ifdef HAVE_SECURITY
    dto->show_security_bar_p	= fe_globalPrefs.show_security_bar_p;
#endif
    dto->autoload_images_p	= fe_globalPrefs.autoload_images_p;
    dto->delayed_images_p	= False;
    dto->force_load_images	= 0;
    dto->fancy_ftp_p		= fe_globalPrefs.fancy_ftp_p;
    dto->xfe_doc_csid		= INTL_DefaultDocCharSetID(NULL);
  }
}

static Boolean
fe_respect_chrome(MWContext *context, Chrome *chrome)
{
  fe_ContextData* data = CONTEXT_DATA(context);
  Boolean changed = False;
#define UPDATE_CHANGED(dst, src) (changed = ((dst != src) ? ((dst = src), True) : changed));

  if (!chrome) return False;

  /* Update chrome into context */
  UPDATE_CHANGED(data->show_url_p, chrome->show_url_bar);
  UPDATE_CHANGED(data->show_toolbar_p, chrome->show_button_bar);
  UPDATE_CHANGED(data->show_directory_buttons_p, chrome->show_directory_buttons);
#ifdef HAVE_SECURITY
  UPDATE_CHANGED(data->show_security_bar_p, chrome->show_security_bar);
#endif
  UPDATE_CHANGED(data->show_menubar_p, chrome->show_menu);
  UPDATE_CHANGED(data->show_bottom_status_bar_p, chrome->show_bottom_status_bar);

#undef UPDATE_CHANGED

  return(changed);
}

/* #### mailnews.c */
extern void fe_set_compose_wrap_state(MWContext *context, XP_Bool wrap_p);


MWContext *
fe_MakeNewWindow(Widget toplevel, MWContext *context_to_copy,
			URL_Struct *url, char *window_name, MWContextType type,
			Boolean skip_get_url, Chrome *decor)
{
  MWContext* context;
  struct fe_MWContext_cons *cons;
  fe_ContextData *fec;
  Widget shell;
  char *shell_name;

  int delete_response;
  Boolean allow_resize;
  Boolean is_modal;
  Arg av[20];
  int ac;
  int16 new_charset;

  new_charset = CS_DEFAULT;

  /* Fix type */
  if (url && (type != MWContextSaveToDisk) && (type != MWContextBookmarks) &&
	(type != MWContextAddressBook) && (type != MWContextDialog)) {
    if (MSG_RequiresMailWindow (url->address))
      type = MWContextMail;
    else if (MSG_RequiresNewsWindow (url->address))
      type = MWContextNews;
    else if (MSG_RequiresBrowserWindow (url->address)) {
      if (type != MWContextEditor)
          type = MWContextBrowser;
    }
  }

  /* There will be only one Mail/News context */
  if (type == MWContextMail || type == MWContextNews) {
    for (cons = fe_all_MWContexts ; cons ; cons = cons->next) {
      if (cons->context->type == type) {
	XMapRaised(XtDisplay(CONTEXT_WIDGET(cons->context)),
		   XtWindow(CONTEXT_WIDGET(cons->context)));
	if (url) {
	    XP_MEMSET (&url->savedData, 0, sizeof (SHIST_SavedData));
	    fe_GetURL (cons->context, url, skip_get_url);
	}
	return cons->context;
      }
    }
  }

  /* Give the TopLevelShells different names so that the user can set
   * resources on them differently, for example,
   * "Netscape.Navigator.geometry" versus "Netscape.Mail.geometry"
   */
  switch (type) {
    case MWContextMail:		      shell_name = "Mail"; break;
    case MWContextNews:		      shell_name = "News"; break;
    case MWContextMessageComposition: shell_name = "Composition"; break;
    case MWContextSaveToDisk:	      shell_name = "Download"; break;
    case MWContextBookmarks:	      shell_name = "Bookmark"; break;
    case MWContextAddressBook:	      shell_name = "AddressBook"; break;
    case MWContextBrowser:	      shell_name = "Navigator"; break;
    case MWContextDialog:	      shell_name = "Dialog"; break;
    case MWContextEditor:             shell_name = "Editor"; break;
    default:	XP_ASSERT(0);
  }

  /* Allocate context required memory */
  context	= XP_NEW_ZAP (MWContext);
  if (context == NULL) return NULL;
  cons		= XP_NEW_ZAP(struct fe_MWContext_cons);
  if (cons == NULL) return NULL;
  fec		= XP_NEW_ZAP (fe_ContextData);
  if (fec == NULL) return NULL;

  /* Allocate context specific fe data */
  if (type == MWContextNews) {
    fec->news_part = XP_NEW_ZAP (fe_NewsContextData);
    if (fec->news_part == NULL) return NULL;
  }
  context->type = type;
  CONTEXT_DATA (context) = fec;

  fe_InitRemoteServer (XtDisplay (toplevel));

  /* add the layout function pointers */
  context->funcs = fe_BuildDisplayFunctionTable();
  context->convertPixX = context->convertPixY = 1;
#ifdef GRIDS
  context->is_grid_cell = FALSE;
  context->grid_parent = NULL;
#endif /* GRIDS */
  cons->context = context;
  cons->next = fe_all_MWContexts;
  fe_all_MWContexts = cons;

  if (window_name) context->name = strdup (window_name);
  XP_AddContextToList (context);

  fe_pick_visual_and_colormap (toplevel, context);
  fe_InitIconColors (context);

  /* Make a local copy of Chrome for calling the close callback with
     the arguments passed when a window is destroyed. Even though the
     URL_struct has the chrome, we might have a context with no URL and
     a Chrome */
  CONTEXT_DATA(context)->hasChrome = False;
  if (decor) {
    CONTEXT_DATA(context)->hasChrome = True;
    CONTEXT_DATA(context)->chrome = *decor;
    decor = &CONTEXT_DATA(context)->chrome;
  }

  /* resize ? default: True */
  allow_resize = True;
  if (decor)
    allow_resize = decor->allow_resize;

  /* modality ? default: False */
  is_modal = False;
  if (decor)
    if (decor->is_modal && (type == MWContextDialog) && context_to_copy)
      /* Modal contexts are implemented only for MWContextDialog for now.
	 And how modality is implemented is that the modal dialog is created
	 as a child of the context_to_copy. If there is no context_to_copy,
	 modality is ignored. */
      is_modal = True;
    else
      decor->is_modal = False;

  /* allow_close ? default: Yes (depending on context) */
  if (type == MWContextBookmarks || type == MWContextAddressBook)
    delete_response = XmUNMAP;
  else
    delete_response = XmDESTROY;
  if (decor)
    if (decor->allow_close)
      delete_response = XmDESTROY;
    else
      delete_response = XmDO_NOTHING;

  if (is_modal) {
    Widget mainw;
    Widget bulletinb;

    if (CONTEXT_DATA (context_to_copy)->currentPrefDialog)
      mainw = CONTEXT_DATA (context_to_copy)->currentPrefDialog;
    else
      mainw = CONTEXT_WIDGET (context_to_copy);

    ac = 0;
    XtSetArg (av[ac], XmNvisual, fe_globalData.default_visual); ac++;
    XtSetArg (av[ac], XmNdepth, fe_VisualDepth (XtDisplay (toplevel),
                                      fe_globalData.default_visual)); ac++;
    XtSetArg (av[ac], XmNcolormap, fe_cmap(context)); ac++;
                        /* Use colormap previously picked for context.      */
                        /* There are subsequent references to this colormap */
                        /* so do not use fe_cmap(context_to_copy).          */
    XtSetArg (av[ac], XmNallowShellResize, False); ac++;
    XtSetArg (av[ac], XmNdialogType, XmDIALOG_PROMPT); ac++;
    XtSetArg (av[ac], XmNdeleteResponse, delete_response); ac++;
    XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
    XtSetArg (av[ac], XmNborderWidth, 0); ac++;

    shell = XmCreateDialogShell (mainw, shell_name, av, ac);

    ac = 0;
    XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
    XtSetArg (av[ac], XmNborderWidth, 0); ac++;
    XtSetArg (av[ac], XmNmarginWidth, 0); ac++;
    XtSetArg (av[ac], XmNmarginHeight, 0); ac++;
    XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
    XtSetArg (av[ac], XmNnoResize, !allow_resize); ac++;
    XtSetArg (av[ac], XmNresizePolicy, XmRESIZE_ANY); ac++;
    bulletinb = XmCreateBulletinBoard (shell, "bulletinb", av, ac);
    shell = bulletinb;
  }
  else
  {
    ac = 0;
    XtSetArg (av[ac], XmNvisual, fe_globalData.default_visual); ac++;
    XtSetArg (av[ac], XmNdepth, fe_VisualDepth (XtDisplay (toplevel),
					fe_globalData.default_visual)); ac++;
    XtSetArg (av[ac], XmNcolormap, fe_cmap(context)); ac++;
    XtSetArg (av[ac], XmNallowShellResize, False); ac++;
    XtSetArg (av[ac], XmNdeleteResponse, delete_response); ac++;
    shell = XtCreatePopupShell (shell_name, topLevelShellWidgetClass,
				toplevel, av, ac);
  }

  XtGetApplicationResources (shell,	/* Resources to init for each window */
			     (XtPointer) CONTEXT_DATA (context),
			     fe_Resources, fe_ResourcesSize,
			     0, 0);

  /* Sigh. Grab the charset before we set context_to_copy to NULL. */
  if (context_to_copy && CONTEXT_DATA(context_to_copy)) {
    new_charset = CONTEXT_DATA(context_to_copy)->xfe_doc_csid;
  } else {
    new_charset = INTL_DefaultDocCharSetID(NULL);
  }

  if (context_to_copy) {
    context_to_copy = fe_GridToTopContext (context_to_copy);

#ifdef EDITOR
    /* Should the new context be an editor context? */
    if (type == MWContextEditor)
	context->is_editor = TRUE;
    else
	context->is_editor = FALSE;
#endif /* EDITOR */

    /* Dissimilar types of browser window don't share preferences. */
    if (type != context_to_copy->type)
      context_to_copy = 0;
  }

  switch (type) {
    case MWContextSaveToDisk:
        fe_WindowCount++;
	fe_MakeSaveToDiskContextWidgets (shell, context);
	fe_get_context_resources (context);  /* Do other resource db hackery. */
	fe_BookmarkInit (context);
	fe_LicenseDialog (context);
	XtAddCallback (CONTEXT_WIDGET (context), XtNdestroyCallback,
			fe_AbortCallback, context);
	XtAddEventHandler(CONTEXT_WIDGET (context), StructureNotifyMask,
		    FALSE, (XtEventHandler)fe_map_notify_eh, context);
	XtManageChild (CONTEXT_WIDGET (context));
	XtRealizeWidget (shell);
	fe_FixLogoMargins (context);
	fe_get_final_context_resources (context);
	fe_InitScrolling (context);
	fe_position_download_context (context);
	XtPopup (CONTEXT_WIDGET (context), XtGrabNone);
	XFE_SetDocTitle (context, url->address);
	fe_InitColormap (context);
	break;

    case MWContextBookmarks:	/* FALL THROUGH */
    case MWContextAddressBook:
	fe_MakeBookmarkWidgets (shell, context);
	XtAddCallback (CONTEXT_WIDGET (context), XtNdestroyCallback,
			fe_delete_cb, context);
	XtAddEventHandler(CONTEXT_WIDGET (context), StructureNotifyMask,
		    FALSE, (XtEventHandler)fe_map_notify_eh, context);
	fe_InitColormap (context);
	fe_InitIcons (context);
	break;

    case MWContextMail:		/* FALL THROUGH */
    case MWContextNews:
	fe_copy_context_settings(context, context_to_copy);
	CONTEXT_DATA (context)->show_url_p = False;
	CONTEXT_DATA (context)->show_directory_buttons_p = False;
	CONTEXT_DATA (context)->show_toolbar_p = True;
#ifdef EDITOR
	CONTEXT_DATA(context)->show_character_toolbar_p = False;
	CONTEXT_DATA(context)->show_paragraph_toolbar_p = False;
#endif /*EDITOR*/
	CONTEXT_DATA (context)->show_toolbar_icons_p =
	  fe_globalPrefs.toolbar_icons_p;
	CONTEXT_DATA (context)->show_toolbar_text_p =
	  TRUE /* #### fe_globalPrefs.toolbar_text_p */;
	CONTEXT_DATA (context)->show_menubar_p =
	  TRUE /* #### fe_globalPrefs.show_menubar_p */;
	CONTEXT_DATA (context)->show_bottom_status_bar_p =
	  TRUE /* #### fe_globalPrefs.show_bottom_status_bar_p */;
	context->win_csid =
	  INTL_DocToWinCharSetID (CONTEXT_DATA (context)->xfe_doc_csid);

        fe_WindowCount++;
	fe_MakeWidgets (shell, context);
	fe_get_context_resources (context);  /* Do other resource db hackery. */
	SHIST_InitSession (context);	/* Initialize the history library. */
	fe_load_default_font(context);
	fe_BookmarkInit (context);
	fe_LicenseDialog (context);
	XtAddCallback (CONTEXT_WIDGET (context), XtNdestroyCallback,
			fe_delete_cb, context);
	XtAddEventHandler(CONTEXT_WIDGET (context), StructureNotifyMask,
		    FALSE, (XtEventHandler)fe_map_notify_eh, context);
#ifdef MOCHA
	XtAddEventHandler(CONTEXT_WIDGET (context), FocusChangeMask,
			  FALSE, (XtEventHandler)fe_focus_notify_eh, context);
#endif /* MOCHA */
	XtManageChild (CONTEXT_WIDGET (context));
	fe_find_scrollbar_sizes (context);
	XtRealizeWidget (shell);
	fe_FixLogoMargins (context);
	fe_get_final_context_resources (context);
	fe_InitScrolling (context);
	fe_InitRemoteServerWindow (context);
	XtPopup (CONTEXT_WIDGET (context), XtGrabNone);
	fe_InitColormap (context);
	if (type == MWContextMail) {
	  MSG_InitializeMailContext(context);
	  fe_SetMailNewsSortBehavior(context,
				     fe_globalPrefs.mail_thread_p,
				     fe_globalPrefs.mail_sort_style);
	} else {
  	  MSG_InitializeNewsContext(context);
	  fe_SetMailNewsSortBehavior(context,
				     !fe_globalPrefs.no_news_thread_p,
				     fe_globalPrefs.news_sort_style);
	}
	fe_SensitizeMenus (context);	/* Finish initializing the menus. */
	break;

    case MWContextMessageComposition:
	fe_copy_context_settings(context, context_to_copy);
	CONTEXT_DATA (context)->show_url_p = False;
	CONTEXT_DATA (context)->show_directory_buttons_p = False;
	CONTEXT_DATA (context)->show_toolbar_p = True;
#ifdef EDITOR
	CONTEXT_DATA(context)->show_character_toolbar_p = False;
	CONTEXT_DATA(context)->show_paragraph_toolbar_p = False;
#endif /*EDITOR*/
	CONTEXT_DATA (context)->show_toolbar_icons_p =
	  fe_globalPrefs.toolbar_icons_p;
	CONTEXT_DATA (context)->show_toolbar_text_p =
	  TRUE /* #### fe_globalPrefs.toolbar_text_p */;

	CONTEXT_DATA(context)->compose_wrap_lines_p = True;

        fe_WindowCount++;

	fe_MakeWidgets (shell, context);
	fe_get_context_resources (context);  /* Do other resource db hackery. */

	fe_set_compose_wrap_state(context, TRUE);

	SHIST_InitSession (context);	/* Initialize the history library. */
	fe_BookmarkInit (context);
	fe_LicenseDialog (context);
	XtAddCallback (CONTEXT_WIDGET (context), XtNdestroyCallback,
			fe_delete_cb, context);
	XtAddEventHandler(CONTEXT_WIDGET (context), StructureNotifyMask,
		    FALSE, (XtEventHandler)fe_map_notify_eh, context);
	fe_load_default_font(context);
	XtManageChild (CONTEXT_WIDGET (context));
	fe_find_scrollbar_sizes (context);
	XtRealizeWidget (shell);
	fe_FixLogoMargins (context);
	fe_get_final_context_resources (context);
	fe_InitScrolling (context);
	fe_InitRemoteServerWindow (context);
	fe_InitColormap (context);
	fe_SensitizeMenus (context);	/* Finish initializing the menus. */
	break;

#ifdef EDITOR
    case MWContextEditor:
#endif /*EDITOR*/
    case MWContextBrowser:
	fe_copy_context_settings(context, context_to_copy);
	if (CONTEXT_DATA(context)->hasChrome)
	  fe_respect_chrome(context, &(CONTEXT_DATA(context)->chrome));
#ifdef HAVE_SECURITY
	/* Enable security bar for all browser windows. I had to do this
	   so that this would get focus whenever we set focus on the main
	   window. Thus all owr translations will work. Otherwise, if all
	   chrome is switched off, there will be no widget to set focus to
	*/
	CONTEXT_DATA(context)->show_security_bar_p = TRUE;
#endif
#ifdef EDITOR
	if (type == MWContextEditor)
	{
	  CONTEXT_DATA(context)->show_directory_buttons_p = False;
	  CONTEXT_DATA(context)->show_url_p = False;
	  if (window_name && strcmp(window_name, "view-source")==0)
		CONTEXT_DATA(context)->show_security_bar_p = False;
	  context->is_editor = TRUE;
	  context->type = MWContextEditor;
	  /* Sorry, I can't make blink work yet - fixed for #17391..djw. */
	  CONTEXT_DATA(context)->blinking_enabled_p = False;
	}
	else 
        {
	  CONTEXT_DATA(context)->show_character_toolbar_p = False;
	  CONTEXT_DATA(context)->show_paragraph_toolbar_p = False;
	  if (window_name && strcmp(window_name, "view-source")==0)
		CONTEXT_DATA(context)->show_security_bar_p = False;
	}
#endif /*EDITOR*/
        fe_WindowCount++;
	fe_MakeWidgets (shell, context);	
	fe_get_context_resources (context);  /* Do other resource db hackery. */

	if (decor) {
	  /* Set width and height. Do this after fe_get_context_resources()
	   * as it sets the width and height. */
	  if (decor->w_hint > 0)
	    XtVaSetValues(CONTEXT_DATA (context)->scrolled,
				XmNwidth, decor->w_hint, 0);
	  if (decor->h_hint > 0)
	    XtVaSetValues(CONTEXT_DATA (context)->scrolled,
				XmNheight, decor->h_hint, 0);
	}
	SHIST_InitSession (context);	/* Initialize the history library. */

	/* WARNING: We are assuming that a browser window get created first */
	fe_BookmarkInit (context);
	fe_LicenseDialog (context);
#ifdef EDITOR
	if (type == MWContextEditor) {
	  Atom WM_DELETE_WINDOW;

	  /*
	   *    Reset the delete handler, and add a WM callback.
	   */
	  XtVaSetValues(CONTEXT_WIDGET(context),
			XmNdeleteResponse,
			XmDO_NOTHING,
			0);

	  WM_DELETE_WINDOW = XmInternAtom(XtDisplay(CONTEXT_WIDGET(context)),
					  "WM_DELETE_WINDOW",
					  False);
	  XmAddWMProtocolCallback(shell,
				  WM_DELETE_WINDOW,
				  fe_editor_delete_response, 
				  context);
	}
#endif
	XtAddCallback (CONTEXT_WIDGET (context), XtNdestroyCallback,
			fe_delete_cb, context);
	XtAddEventHandler(CONTEXT_WIDGET (context), StructureNotifyMask,
		    FALSE, (XtEventHandler)fe_map_notify_eh, context);
#ifdef MOCHA
	XtAddEventHandler(CONTEXT_WIDGET (context), FocusChangeMask,
			  FALSE, (XtEventHandler)fe_focus_notify_eh, context);
#endif /* MOCHA */
	fe_load_default_font(context);
	XtManageChild (CONTEXT_WIDGET (context));
	fe_find_scrollbar_sizes (context);
	XtRealizeWidget (shell);
	fe_FixLogoMargins (context);
	fe_get_final_context_resources (context);
	fe_InitScrolling (context);
	if (context_to_copy
#ifdef EDITOR
	    && (type != MWContextEditor)
#endif /* EDITOR */
	    && (decor == NULL || decor->copy_history)) {
	  /* When creating a window, always copy the history list from
	     the previous window's history list.  This call makes the
	     current document be the oldest page in the history.
	  */
	  SHIST_CopySession (context, context_to_copy);

	  /* But if we're creating a new window but have a URL, position
	     the history to the newest document before loading that url.
	     If we left it at the beginning, then every new window created
	     by button2 would have exactly two documents - the oldest, and
	     the one clicked upon.
	  */
	  if (url) {
	    XP_List *history = SHIST_GetList (context);
	    SHIST_SetCurrent (&context->hist, XP_ListCount (history));
	  }
	}

	fe_InitRemoteServerWindow (context);
	XtPopup (CONTEXT_WIDGET (context), XtGrabNone);
	fe_InitColormap (context);
	fe_VerifyDiskCache (context);
	fe_SensitizeMenus (context);	/* Finish initializing the menus. */
	break;

    case MWContextDialog:
        fe_WindowCount++;
	/* Dialog context always load images */
	CONTEXT_DATA(context)->autoload_images_p = TRUE;
	CONTEXT_DATA(context)->xfe_doc_csid = new_charset;
	/* XXX This needs to become fe_MakeWidgets() Problems with this are
	 *	1. Lots of code gets visual, depth etc from the CONTEXT_WIDGET
	 *	   for ContextDialog, CONTEXT_WIDGET is the bulletin board that
	 *	   is the child of the shell. All these needs to change.
	 *	2. Menubar code needs to be told to use the browser menubar
	 *	   for ContextDialog too.
	 */
	fe_MakeChromeWidgets (shell, context);
	fe_get_context_resources (context);  /* Do other resource db hackery. */
	/* Set width and height. Do this after fe_get_context_resources()
	 * as it sets the width and height. */
	if (decor->w_hint > 0)
	  XtVaSetValues(CONTEXT_DATA (context)->scrolled,
				XmNwidth, decor->w_hint, 0);
	if (decor->h_hint > 0)
	  XtVaSetValues(CONTEXT_DATA (context)->scrolled,
				XmNheight, decor->h_hint, 0);

	XtAddCallback (CONTEXT_WIDGET (context), XtNdestroyCallback,
			fe_delete_cb, context);
	XtAddEventHandler(CONTEXT_WIDGET (context), StructureNotifyMask,
		    FALSE, (XtEventHandler)fe_map_notify_eh, context);
#ifdef MOCHA
	XtAddEventHandler(CONTEXT_WIDGET (context), FocusChangeMask,
			  FALSE, (XtEventHandler)fe_focus_notify_eh, context);
#endif /* MOCHA */

	fe_load_default_font(context);
	XtManageChild (CONTEXT_WIDGET (context));
	fe_find_scrollbar_sizes (context);
	XtRealizeWidget (shell);
	fe_get_final_context_resources (context);
	fe_InitScrolling (context);
	if (!XmIsDialogShell(XtParent(CONTEXT_WIDGET (context))))
	  XtPopup (CONTEXT_WIDGET (context), XtGrabNone);
	fe_InitColormap (context);
	break;


    default:	XP_ASSERT(0);
  }
  /* Register Session manager stuff before plonk.*/

  if ( !has_session_mgr)
  	has_session_mgr= fe_add_session_manager(context);

  if (plonk (context)) {
    url = 0;

    /* while in plonk the context might have got deleted. Validate context */
    if (!fe_contextIsValid(context)) return NULL;
  }

  fe_initialize_stderr ();

  if (!fe_VendorAnim)
    if (NET_CheckForTimeBomb (context))
      url = 0;

  if (url && (type != MWContextSaveToDisk)
     ) {
    /* #### This might not be right, or there might be more that needs
        to be done...   Note that url->history_num is bogus for the new
        context.  url->position_tag might also be context-specific.
    */
    XP_MEMSET (&url->savedData, 0, sizeof (SHIST_SavedData));
    fe_GetURL (context, url, skip_get_url);
  }
  
  /* Windows menu needs regeneration since we added a new context to the pool */
  {
    struct fe_MWContext_cons *cntx = fe_all_MWContexts;
    for (; cntx; cntx = cntx->next)
      CONTEXT_DATA(cntx->context)->windows_menu_up_to_date_p = False;
  }

  return context;
}


MWContext *
FE_MakeNewWindow(MWContext *old_context, URL_Struct *url, char *window_name,
			Chrome *cr)
{
	MWContext *new_context;
	MWContextType type = MWContextBrowser;

	if ((old_context == NULL)||(CONTEXT_WIDGET (old_context) == NULL))
		return(NULL);

	if (cr && cr->type != 0) type = cr->type;

	new_context = fe_MakeNewWindow (
			XtParent(CONTEXT_WIDGET (old_context)),
			old_context, url,
			window_name, type, (url == NULL), cr);
	return(new_context);
}

MWContext *
FE_MakeBlankWindow(MWContext *old_context, URL_Struct *url, char *window_name)
{
	MWContext *new_context;

	if ((old_context == NULL)||(CONTEXT_WIDGET (old_context) == NULL))
	{
		return(NULL);
	}

	new_context = fe_MakeWindow (
		XtParent(CONTEXT_WIDGET (old_context)),
		old_context, NULL, window_name, MWContextBrowser, TRUE);
	return(new_context);
}


void
FE_SetWindowLoading(MWContext *context, URL_Struct *url,
	Net_GetUrlExitFunc **exit_func_p)
{
	if ((context != NULL)&&(url != NULL))
	{
		fe_GetURL (context, url, TRUE);
		*exit_func_p = (Net_GetUrlExitFunc *)fe_url_exit;
	}
}


Boolean
fe_IsGridParent (MWContext *context)
{
  return (context->grid_children && XP_ListTopObject (context->grid_children));
}


MWContext *
fe_GetFocusGridOfContext(MWContext *context)
{
  MWContext *child;
  int i = 1;

  if (context == NULL)
    return 0;

  while ((child = (MWContext*)XP_ListGetObjectNum (context->grid_children,
						   i++))) {
    if (CONTEXT_DATA (child)->focus_grid)
      return child;

    if (fe_IsGridParent (child)) {
      child = fe_GetFocusGridOfContext (child);
      if (child != NULL)
	return child;
    }
  }

  return 0;
}


void
fe_SetGridFocus (MWContext *context)
{
  Widget w;
  MWContext *top, *focus_grid;
  Dimension border_width;

  if (context == NULL)
    return;

  top = fe_GridToTopContext (context);
  if (top == NULL)
    return;

  if ((focus_grid = fe_GetFocusGridOfContext (top))) {
    CONTEXT_DATA (focus_grid)->focus_grid = False;
    w = CONTEXT_DATA (focus_grid)->main_pane;
    XtVaGetValues (w, XmNborderWidth, &border_width, 0);
    if (border_width)
      XtVaSetValues (w, XmNborderColor, 
		     (CONTEXT_DATA (context)->default_bg_pixel), 0);
  }

  /* Then indicate which cell has focus */
  TRACEMSG (("context: 0x%x has focus\n", context));
  CONTEXT_DATA (context)->focus_grid = True;
  w = CONTEXT_DATA (context)->main_pane;

  XtVaGetValues (w, XmNborderWidth, &border_width, 0);
  if (border_width)
    XtVaSetValues (w, XmNborderColor,
		   CONTEXT_DATA (context)->default_fg_pixel, 0);

  XFE_SetDocTitle (context, 0);
}

MWContext *
FE_MakeGridWindow (MWContext *old_context, void *history, int32 x, int32 y,
        int32 width, int32 height, char *url_str, char *window_name,
	int8 scrolling, NET_ReloadMethod force_reload, Bool no_edge)
{
#ifdef GRIDS
  Widget parent = CONTEXT_DATA (old_context)->drawing_area;
  MWContext *context = (MWContext *) calloc (sizeof (MWContext), 1);
  struct fe_MWContext_cons *cons = (struct fe_MWContext_cons *)
    malloc (sizeof (struct fe_MWContext_cons));
  fe_ContextData *fec = (fe_ContextData *) calloc (sizeof (fe_ContextData), 1);
  History_entry *he = (History_entry *)history;
  URL_Struct *url = NULL;

  CONTEXT_DATA (context) = fec;

  /* add the layout function pointers
   */
  context->funcs = fe_BuildDisplayFunctionTable();
  context->convertPixX = context->convertPixY = 1;
  context->is_grid_cell = TRUE;
  context->grid_parent = old_context;
  cons->context = context;
  cons->next = fe_all_MWContexts;
  fe_all_MWContexts = cons;

  SHIST_InitSession (context);		/* Initialize the history library. */
  SHIST_AddDocument(context, he);

  if (he)
    url = SHIST_CreateURLStructFromHistoryEntry (context, he);
  else
    url = NET_CreateURLStruct (url_str, FALSE);

  if (url) {
    MWContext *top = fe_GridToTopContext(old_context);
    History_entry *h = SHIST_GetCurrent (&top->hist);

    url->force_reload = force_reload;

    /* Set the referer field in the url to the document that refered to the
     * grid parent.
     */
    if (h && h->referer)
      url->referer = strdup(h->referer);
  }

  if (window_name)
    {
      context->name = strdup (window_name);
    }
  XP_AddContextToList (context);

  if (old_context)
    {
      CONTEXT_DATA (context)->autoload_images_p =
        CONTEXT_DATA (old_context)->autoload_images_p;
      CONTEXT_DATA (context)->delayed_images_p =
        CONTEXT_DATA (old_context)->delayed_images_p;
      CONTEXT_DATA (context)->force_load_images = 0;
      CONTEXT_DATA (context)->fancy_ftp_p =
        CONTEXT_DATA (old_context)->fancy_ftp_p;
      CONTEXT_DATA (context)->xfe_doc_csid =
        CONTEXT_DATA (old_context)->xfe_doc_csid;
    }
  CONTEXT_WIDGET (context) = CONTEXT_WIDGET (old_context);
  CONTEXT_DATA (context)->anim_rate = CONTEXT_DATA (old_context)->anim_rate;
  CONTEXT_DATA (context)->backdrop_pixmap = (Pixmap) ~0;
  CONTEXT_DATA (context)->grid_scrolling = scrolling;

#ifdef FRAMES_HAVE_THEIR_OWN_COLORMAP
  /* We have to go through this shit to get the toplevel widget */
  {
      MWContext *top_context = context;
      while ((top_context->grid_parent) &&
             (top_context->grid_parent->is_grid_cell))
          top_context = top_context->grid_parent;

      fe_pick_visual_and_colormap (XtParent(CONTEXT_WIDGET (top_context)),
                                   context);
  }
#else
  /* Inherit colormap from our parent */
  CONTEXT_DATA(context)->colormap = CONTEXT_DATA(old_context)->colormap;
#endif

#ifdef FRAMES_HAVE_THEIR_OWN_COLORMAP
  fe_InitColormap(context);
#endif

  XtGetApplicationResources (CONTEXT_WIDGET (old_context),
                             (XtPointer) CONTEXT_DATA (context),
                             fe_Resources, fe_ResourcesSize,
                             0, 0);

/* CONTEXT_DATA (context)->main_pane = parent; */

  /*
   * set the default coloring correctly into the new context.
   */
  {
    Pixel unused_select_pixel;
    XmGetColors (XtScreen (parent),
		 fe_cmap(context),
		 CONTEXT_DATA (context)->default_bg_pixel,
		 &(CONTEXT_DATA (context)->fg_pixel),
		 &(CONTEXT_DATA (context)->top_shadow_pixel),
		 &(CONTEXT_DATA (context)->bottom_shadow_pixel),
		 &unused_select_pixel);
  }

  /* ### Create a form widget to parent the scroller.
   *
   * This might keep the scroller from becoming smaller than
   * the cell size when there are no scrollbars.
   */

  {
    Arg av [50];
    int ac;
    Widget pane, mainw, scroller;
    int border_width = 0;

    if (no_edge)
      border_width = 0;
    else
      border_width = 2;

    ac = 0;
    XtSetArg (av[ac], XmNx, (Position)x); ac++;
    XtSetArg (av[ac], XmNy, (Position)y); ac++;
    XtSetArg (av[ac], XmNwidth, (Dimension)width - 2*border_width); ac++;
    XtSetArg (av[ac], XmNheight, (Dimension)height - 2*border_width); ac++;
    XtSetArg (av[ac], XmNborderWidth, border_width); ac++;
    mainw = XmCreateForm (parent, "form", av, ac);

    ac = 0;
    XtSetArg (av[ac], XmNborderWidth, 0); ac++;
    XtSetArg (av[ac], XmNmarginWidth, 0); ac++;
    XtSetArg (av[ac], XmNmarginHeight, 0); ac++;
    XtSetArg (av[ac], XmNborderColor,
		      CONTEXT_DATA (context)->default_bg_pixel); ac++;
    pane = XmCreatePanedWindow (mainw, "pane", av, ac);

    XtVaSetValues (pane,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);

    /* The actual work area */
    scroller = fe_MakeScrolledWindow (context, pane, "scroller");
    XtVaSetValues (CONTEXT_DATA (context)->scrolled,
		   XmNborderWidth, 0, 0);

    XtManageChild (scroller);
    XtManageChild (pane);
    XtManageChild (mainw);

    CONTEXT_DATA (context)->main_pane = mainw;
  }

  fe_load_default_font (context);
  fe_get_context_resources (context);   /* Do other resource db hackery. */

  /* Figure out how much space the horizontal and vertical scrollbars take up.
     It's basically impossible to determine this before creating them...
   */
  {
    Dimension w1 = 0, w2 = 0, h1 = 0, h2 = 0;

    XtManageChild (CONTEXT_DATA (context)->hscroll);
    XtManageChild (CONTEXT_DATA (context)->vscroll);
    XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
                                 XmNwidth, &w1,
                                 XmNheight, &h1,
                                 0);

    XtUnmanageChild (CONTEXT_DATA (context)->hscroll);
    XtUnmanageChild (CONTEXT_DATA (context)->vscroll);
    XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
                                 XmNwidth, &w2,
                                 XmNheight, &h2,
                                 0);

    CONTEXT_DATA (context)->sb_w = w2 - w1;
    CONTEXT_DATA (context)->sb_h = h2 - h1;

    /* Now that we know, we don't need to leave them managed. */
  }

  XtVaSetValues (CONTEXT_DATA (context)->scrolled, XmNinitialFocus,
                 CONTEXT_DATA (context)->drawing_area, 0);

  fe_SetGridFocus (context);  /* Give this grid focus */
  fe_InitScrolling (context); /* big voodoo */

  fe_InitColormap (context);

  if (url)
    {
      /* #### This might not be right, or there might be more that needs
         to be done...   Note that url->history_num is bogus for the new
         context.  url->position_tag might also be context-specific. */
      XP_MEMSET (&url->savedData, 0, sizeof (SHIST_SavedData));
      fe_GetURL (context, url, FALSE);
    }

  XFE_SetDocTitle (context, 0);

  return(context);
#else
  return(NULL);
#endif /* GRIDS */
}

void
FE_RestructureGridWindow (MWContext *context, int32 x, int32 y,
        int32 width, int32 height)
{
#ifdef GRIDS
  Widget mainw;

  /*
   * This comes from blank frames.  Maybe we should clear them
   * before we return?
   */
  if (!context ) return;

  /*
   * Basically we just set the new position and dimensions onto the
   * parent of the drawing area.  X will do the rest for us.
   * Because of the window gravity side effects of guffaws scrolling
   * The drawing area won't move with its parent unless we temporarily
   * turn off guffaws.
   */
  mainw = CONTEXT_DATA (context)->main_pane;
  fe_SetGuffaw(context, FALSE);
  XtVaSetValues (mainw,
		 XmNx, (Position)x,
		 XmNy, (Position)y,
		 XmNwidth, (Dimension)width - 4,   /* Adjust for focus border */
		 XmNheight, (Dimension)height - 4,
		 0);
  fe_SetGuffaw(context, TRUE);
#endif /* GRIDS */
}

/* Required to make custom colormap installation work.
   Really, not *every* context has a private colormap.
   Trivial contexts like address book, bookmarks, etc.
   share a colormap. */
#define EVERY_CONTEXT_HAS_PRIVATE_COLORMAP

static void
fe_pick_visual_and_colormap (Widget toplevel, MWContext *new_context)
{
  Screen *screen = XtScreen (toplevel);
  Display *dpy = XtDisplay (toplevel);
  Colormap cmap;
  Visual *v;
  fe_colormap *colormap;

  v = fe_globalData.default_visual;
  if (!v)
  {
    String str = 0;
    /* "*visualID" is special for a number of reasons... */
    static XtResource res = { "visualID", "VisualID",
			      XtRString, sizeof (String),
			      0, XtRString, "default" };
    XtGetSubresources (toplevel, &str, (char *) fe_progname, "TopLevelShell",
		       &res, 1, 0, 0);
    v = fe_ParseVisual (screen, str);
    fe_globalData.default_visual = v;
  }

  {
    String str = 0;
    static XtResource res = { "installColormap", XtCString, XtRString,
			      sizeof (String), 0, XtRString, "guess" };
    XtGetApplicationResources (toplevel, &str, &res, 1, 0, 0);
    if (!str || !*str || !strcasecmp (str, "guess"))
#if 0
      /* If the server is capable of installing multiple hardware colormaps
	 simultaniously, take one for ourself by default.  Otherwise, don't. */
      fe_globalData.always_install_cmap = (MaxCmapsOfScreen (screen) > 1);
#else
      {
	/* But everybody lies about this value, fuck it. */
	char *vendor = XServerVendor (XtDisplay (toplevel));
	fe_globalData.always_install_cmap =
	  !strcmp (vendor, "Silicon Graphics");
      }
#endif
    else if (!strcasecmp (str, "yes") || !strcasecmp (str, "true"))
      fe_globalData.always_install_cmap = True;
    else if (!strcasecmp (str, "no") || !strcasecmp (str, "false"))
      fe_globalData.always_install_cmap = False;
    else
      {
	fprintf (stderr,
		 "%s: installColormap: %s must be yes, no, or guess.\n",
		 fe_progname, str);
	fe_globalData.always_install_cmap = False;
      }
  }

  /* Don't allow colormap flashing on a deep display */
  if (v != DefaultVisualOfScreen (screen))
    fe_globalData.always_install_cmap = True;
  
  if (!fe_globalData.default_colormap)
    {
      cmap = DefaultColormapOfScreen (screen);
      fe_globalData.default_colormap =
          fe_NewColormap(screen,  DefaultVisualOfScreen (screen), cmap, False);
    }

  colormap = fe_globalData.common_colormap;

  if (!colormap)
    {
      if (fe_globalData.always_install_cmap)
        {
          /* Create a colormap for "simple" contexts 
             like bookmarks, address book, etc */
          cmap = XCreateColormap (dpy, RootWindowOfScreen (screen),
                                  v, AllocNone);
          colormap = fe_NewColormap(screen, v, cmap, True);
        }
      else
        {
          /* Use the default colormap for all contexts. */
          colormap = fe_globalData.default_colormap;
        }
      fe_globalData.common_colormap = colormap;
    }
  
      
#ifdef EVERY_CONTEXT_HAS_PRIVATE_COLORMAP
  if (fe_globalData.always_install_cmap)
    {
      /* Even when installing "private" colormaps for every window,
         "simple" contexts, which have fixed color composition, share
         a single colormap. */
      MWContextType type = new_context->type;
      if ((type == MWContextBrowser) ||
          (type == MWContextEditor)  ||
          (type == MWContextNews)    ||
          (type == MWContextMail))
        {
          cmap = XCreateColormap (dpy, RootWindowOfScreen (screen),
                                  v, AllocNone);
          colormap = fe_NewColormap(screen, v, cmap, True);
        }
    }
#endif  /* !EVERY_CONTEXT_HAS_PRIVATE_COLORMAP */

  CONTEXT_DATA (new_context)->colormap = colormap;
}

static XP_Bool
xfe_openwound_running_p(Widget toplevel)
{
  Display *dpy = XtDisplay(toplevel);
  int result;
  Atom actual_type = 0;
  int actual_format = 0;
  unsigned long nitems = 0, bytes_after = 0;
  unsigned char *data = 0;

  Atom proto_atom = XInternAtom(dpy, "_SUN_WM_PROTOCOLS", True);

  if (proto_atom == None)
    /* If the atom isn't interned, OLWM can't be running. */
    return FALSE;

  result = XGetWindowProperty (dpy,
                             RootWindowOfScreen(DefaultScreenOfDisplay(dpy)),
                             proto_atom,
                             0, 0,            /* read 0 bytes */
                             False,           /* don't delete */
                             AnyPropertyType, /* type expected */
                             &actual_type,    /* returned values */
                             &actual_format,
                             &nitems,
                             &bytes_after,
                             &data);

  if (data)
    /* At most, this will be 1 allocated byte. */
    free(data);

  if (result != Success ||            /* no such property */
      actual_type != XA_ATOM ||               /* didn't contain type Atom */
      bytes_after <= 0)                       /* zero length */
    /* If any of those are true, then OLWM isn't running. */
    return FALSE;

  /* Otherwise, it sure looks like OLWM is running.  If it's not, then it's
     some other program that is imitating it well. */
  return TRUE;
}

void
fe_InitializeGlobalResources (Widget toplevel)
{
  XtGetApplicationResources (toplevel,
			     (XtPointer) &fe_globalData,
			     fe_GlobalResources, fe_GlobalResourcesSize,
			     0, 0);

  /*
   *    And then there was Sun. Try to detect losing olwm,
   *    and default to mono desktop icons.
   */
  if (fe_globalData.wm_icon_policy == NULL) { /* not set */
      if (xfe_openwound_running_p(toplevel))
	  fe_globalData.wm_icon_policy = "mono";
      else
	  fe_globalData.wm_icon_policy = "color";
  }
    
  /* Add a timer to periodically flush out the global history and bookmark. */
  fe_save_history_timer ((XtPointer) ((int) True), 0);

  /* #### move to prefs */
  LO_SetUserOverride (!fe_globalData.document_beats_user_p);
}


/* This initializes resources which must be set up BEFORE the widget is
   realized or managed (sizes and things). */
static void
fe_get_context_resources (MWContext *context)
{
  fe_ContextData *fec = CONTEXT_DATA (context);
  if (fec->drawing_area) {
    XtVaGetValues (fec->drawing_area,
		   XmNbackground, &fec->bg_pixel, 0);
  } /* else??? ### */

  /* If the selection colors ended up mapping to the same pixel values,
     invert them. */
  if (CONTEXT_DATA (context)->select_fg_pixel ==
      CONTEXT_DATA (context)->fg_pixel &&
      CONTEXT_DATA (context)->select_bg_pixel ==
      CONTEXT_DATA (context)->bg_pixel)
    {
      CONTEXT_DATA (context)->select_fg_pixel =
	CONTEXT_DATA (context)->bg_pixel;
      CONTEXT_DATA (context)->select_bg_pixel =
	CONTEXT_DATA (context)->fg_pixel;
    }

  /* Tell layout about the default colors and background.
   */
  {
    XColor c[5];
    c[0].pixel = CONTEXT_DATA (context)->link_pixel;
    c[1].pixel = CONTEXT_DATA (context)->vlink_pixel;
    c[2].pixel = CONTEXT_DATA (context)->alink_pixel;
    c[3].pixel = CONTEXT_DATA (context)->default_fg_pixel;
    c[4].pixel = CONTEXT_DATA (context)->default_bg_pixel;
    XQueryColors (XtDisplay (CONTEXT_WIDGET (context)),
		  fe_cmap(context),
		  c, 5);
    LO_SetDefaultColor (LO_COLOR_LINK,
			c[0].red >> 8, c[0].green >> 8, c[0].blue >> 8);
    LO_SetDefaultColor (LO_COLOR_VLINK,
			c[1].red >> 8, c[1].green >> 8, c[1].blue >> 8);
    LO_SetDefaultColor (LO_COLOR_ALINK,
			c[2].red >> 8, c[2].green >> 8, c[2].blue >> 8);
    LO_SetDefaultColor (LO_COLOR_FG,
			c[3].red >> 8, c[3].green >> 8, c[3].blue >> 8);
    LO_SetDefaultColor (LO_COLOR_BG,
			c[4].red >> 8, c[4].green >> 8, c[4].blue >> 8);

    if (CONTEXT_DATA (context)->default_background_image &&
	*CONTEXT_DATA (context)->default_background_image)
      {
	char *bi = CONTEXT_DATA (context)->default_background_image;
	if (bi[0] == '/')
	  {
	    char *s = (char *) malloc (strlen (bi) + 6);
	    strcpy (s, "file:");
	    strcat (s, bi);
	    LO_SetDefaultBackdrop (s);
	    free (s);
	  }
	else
	  {
	    LO_SetDefaultBackdrop (bi);
	  }
      }
  }

  /* Set the default size of the scrolling area based on the size of the
     default font.  (This can be overridden by a -geometry argument.)
     */
  {
    int16 charset = CS_LATIN1;
    fe_Font font = fe_LoadFont (context, &charset, 0, 3, 0);
    Widget scrolled = XtNameToWidget (CONTEXT_WIDGET (context), "*scroller");
    unsigned int cw = (font ? default_char_width (CS_LATIN1, font) : 12);
#if 0
    /* It's a totally losing proposition to try and figure out how much space
       the scrollbar and associated chrome is going to take up... */
    Dimension bw1 = 0, bw2 = 0;
    Dimension w, h;
    XtVaGetValues (scrolled, XmNverticalScrollBar, &vscroll, 0);
    XtVaGetValues (vscroll, XmNwidth, &bw1, XmNscrolledWindowMarginWidth, &bw2,
		   0);
    w = (cw * 80) + bw1 + bw2;
    h = w;
#else
    /* So just add 10% or so and hope it all fits, dammit. */

# ifdef DEBUG_jwz
    Dimension w = (cw * 100);  /* the web is wider these days. */
    Dimension h = (w * 1.2);
# else /* !DEBUG_jwz */
    Dimension w = (cw * 90);
    Dimension h = w;
# endif /* !DEBUG_jwz */
#endif

    if (context->type == MWContextNews || context->type == MWContextMail) {
      Dimension folderh = 0, messageh = 0;
      int pane_style;

      if (context->type == MWContextNews) {
	pane_style = fe_globalPrefs.news_pane_style;
      }
      else {
	pane_style = fe_globalPrefs.mail_pane_style;
      }

      /* XXX Need to revisit this height stuff - dp */

      /* This height is a hack. It is calculated based on the fact
         that the panes 200 high and above the html area */
      h = (h * 3) / 4;

      /* Here is the history behind this. The pane that separates the list
	 of folders/messages and the html display area is resizable.
	 Initially before this change the folderlist was 200 high. After
	 we fixed it to be saved and restored, the html area's height
	 should depend on the height of the folder list */
      switch (pane_style) {
        case FE_PANES_NORMAL:   
        case FE_PANES_TALL_FOLDERS:   
	  XtVaGetValues(CONTEXT_DATA(context)->messageform,
			XmNheight, &messageh, 0);
	  h -= (messageh - 200);
	  break;
        case FE_PANES_HORIZONTAL:       
	  h = (h * 4) / 3 + 20;
          break;
        case FE_PANES_STACKED:  
	  XtVaGetValues(CONTEXT_DATA(context)->folderform,
			XmNheight, &folderh, 0);
	  XtVaGetValues(CONTEXT_DATA(context)->messageform,
			XmNheight, &messageh, 0);
	  h -= (folderh + messageh - 200);
          break;
      }
    }

    if (scrolled) {
      /* We don't want the default window size to be bigger than the screen.
	 So don't make the default height of the scrolling area be more than
	 90% of the height of the screen.  This is pretty pseudo-nebulous,
	 but again, getting exact numbers here is a royal pain in the ass.
       */
      int pseudo_max_height = (HeightOfScreen (XtScreen (scrolled))
			       * 0.70);
      if (h > pseudo_max_height)
	h = pseudo_max_height;

      XtVaSetValues (scrolled, XmNwidth, w, XmNheight, h, 0);
    }
  }
}


static void
fe_save_history_timer (XtPointer closure, XtIntervalId *id)
{
  Boolean init_only_p = (Boolean) ((int) closure);
  if (! init_only_p) {
    fe_SaveBookmarks ();
    GH_SaveGlobalHistory ();
    NET_WriteCacheFAT (0, False);
    NET_SaveCookies(NULL);
  }
  /* Re-add the timer. */
  fe_globalData.save_history_id =
    XtAppAddTimeOut (fe_XtAppContext,
		     fe_globalData.save_history_interval * 1000,
		     fe_save_history_timer, (XtPointer) ((int) False));
}


static void
fe_refresh_url_timer (XtPointer closure, XtIntervalId *id)
{
  MWContext *context = (MWContext *) closure;
  URL_Struct *url;
  assert (CONTEXT_DATA (context)->refresh_url_timer_url);
  if (! CONTEXT_DATA (context)->refresh_url_timer_url)
    return;
  url = NET_CreateURLStruct (CONTEXT_DATA (context)->refresh_url_timer_url,
			     FALSE);
  url->force_reload = NET_NORMAL_RELOAD;
  fe_GetURL (context, url, FALSE);
}

void
FE_SetRefreshURLTimer (MWContext *context, uint32 secs, char *url)
{
  if(context->type != MWContextBrowser)
	return;

  if (CONTEXT_DATA (context)->refresh_url_timer)
    XtRemoveTimeOut (CONTEXT_DATA (context)->refresh_url_timer);
  if (CONTEXT_DATA (context)->refresh_url_timer_url)
    free (CONTEXT_DATA (context)->refresh_url_timer_url);
  CONTEXT_DATA (context)->refresh_url_timer = 0;
  CONTEXT_DATA (context)->refresh_url_timer_secs = secs;
  CONTEXT_DATA (context)->refresh_url_timer_url = strdup (url);
  if (secs <= 0)
    fe_refresh_url_timer (context, 0);
  else
    CONTEXT_DATA (context)->refresh_url_timer =
      XtAppAddTimeOut (fe_XtAppContext, secs * 1000,
		       fe_refresh_url_timer, (XtPointer) context);
}


void
fe_MakeLEDGCs (MWContext *context)
{
  fe_ContextData *fec = CONTEXT_DATA (context);
  XGCValues gcv;
  Widget widget = CONTEXT_DATA (context)->drawing_area;
  Window w;
  if (!widget) widget = CONTEXT_DATA (context)->widget;
  if (!widget) return;

  w = XtWindow (widget);
  
  /* "*thermo.slider.foreground" and "*power.LED.foreground" are slightly
     special, since I didn't bother to make subclasses for them... */

  if (! w) abort ();

  if (fec->thermo && !fec->thermo_gc)
    {
      static XtResource res = { XtNforeground, XtCForeground,
				XtRPixel, sizeof (String),
				XtOffset (fe_ContextData *, thermo_pixel),
				XtRString, XtDefaultForeground };
      XtGetApplicationResources (fec->thermo, (XtPointer) fec, &res, 1, 0, 0);
      gcv.foreground = fec->thermo_pixel;
      fec->thermo_gc = XCreateGC (XtDisplay (widget), w, GCForeground, &gcv);
    }

  if (fec->LED && !fec->LED_gc)
    {
      static XtResource res = { XtNforeground, XtCForeground,
				XtRPixel, sizeof (String),
				XtOffset (fe_ContextData *, LED_pixel),
				XtRString, XtDefaultForeground };
      XtGetApplicationResources (fec->LED, (XtPointer) fec, &res, 1, 0, 0);
      gcv.foreground = fec->LED_pixel;
      fec->LED_gc = XCreateGC (XtDisplay (widget), w, GCForeground, &gcv);
    }
}


/* This initializes resources which must be set up AFTER the widget is
   realized (meaning we need a window of the correct depth.) */
static void
fe_get_final_context_resources (MWContext *context)
{
  static Boolean guffaws_done = False;

  fe_MakeLEDGCs (context);
  fe_InitIcons (context);

  if (!guffaws_done)
    {
      Widget widget = CONTEXT_DATA (context)->drawing_area;
      if (widget) {
	if (! XtIsRealized (widget)) abort ();
	guffaws_done = True;
	fe_globalData.fe_guffaw_scroll =
	  fe_WindowGravityWorks (CONTEXT_WIDGET(context), widget);
      }
    }
}

void
fe_DestroySaveToDiskContext(MWContext *context)
{
  fe_delete_cb (0, (XtPointer) context, 0);
}

static void fe_cleanup_tooltips(MWContext *context);

void
fe_DestroyContext (MWContext *context)
{
  Widget w = CONTEXT_WIDGET (context);
  struct fe_MWContext_cons *rest, *prev;

  /* This is a hack. If the mailcompose window is going away and 
   * a tooltip was still there (or) was armed (a timer was set for it)
   * for a widget in the mailcompose window, then the destroying
   * mailcompose context would cause a core dump when the tooltip
   * timer hits. This could happen to a javascript window with toolbars
   * too if it is being closed on a timer.
   *
   * In this critical time of akbar ship, we are fixing this by
   * always killing any tooltip that was present anywhere and
   * remove the timer for any armed tooltop anywhere in the
   * navigator when any context is going away.
   *----
   * The proper thing to do would be
   *  - to check if the fe_tool_tips_widget is a child of the
   *    CONTEXT_WIDGET(context) and if it is, then cleanup the tooltip.
   */
  fe_cleanup_tooltips(context);
 

  if (context->is_grid_cell)
    CONTEXT_DATA (context)->being_destroyed = True;

  if (context->type == MWContextSaveToDisk) {
    /* We might be in an extend text selection on the text widgets.
       If we destroy this now, we will dump core. So before destroying
       we will disable extend of the selection. */
    XtCallActionProc(CONTEXT_DATA (context)->url_label, "extend-end",
				NULL, NULL, 0);
    XtCallActionProc(CONTEXT_DATA (context)->url_text, "extend-end",
				NULL, NULL, 0);
  }

#ifdef EDITOR
  if (context->type == MWContextEditor)
      fe_EditorCleanup(context);
#endif /*EDITOR*/

#ifdef GRIDS
  if (context->is_grid_cell)
    w = CONTEXT_DATA (context)->main_pane;
#endif /* GRIDS */
  XP_InterruptContext (context);
  if (CONTEXT_DATA (context)->refresh_url_timer)
    XtRemoveTimeOut (CONTEXT_DATA (context)->refresh_url_timer);
  if (CONTEXT_DATA (context)->refresh_url_timer_url)
    free (CONTEXT_DATA (context)->refresh_url_timer_url);
  fe_DisposeColormap(context);

  /*
  ** We have to destroy the layout before calling XtUnmanageChild so that
  ** we have a chance to reparent the applet windows to a safe
  ** place. Otherwise they'll get destroyed.
  */
  fe_DestroyLayoutData (context);
  XtUnmanageChild (w);

  IL_ScourContext(context);
  fe_StopProgressGraph (context);
  fe_FindReset (context);
  SHIST_EndSession (context);
  if (CONTEXT_DATA (context)->thermo_gc)
    XFreeGC (XtDisplay (w), CONTEXT_DATA (context)->thermo_gc);
  if (CONTEXT_DATA (context)->LED_gc)
    XFreeGC (XtDisplay (w), CONTEXT_DATA (context)->LED_gc);
  fe_DisownSelection (context, 0, True);
  fe_DisownSelection (context, 0, False);
  if (context->type == MWContextMail) {
    MSG_CleanupMailContext(context);
  }
  if (context->type == MWContextNews) {
    MSG_CleanupNewsContext(context);
  }
  if (context->type == MWContextMessageComposition) {
    MSG_CleanupMessageCompositionContext(context);
  }

  if (! context->is_grid_cell) {
    if (context->type == MWContextSaveToDisk)
      XtRemoveCallback (w, XtNdestroyCallback, fe_AbortCallback, context);
    else
      XtRemoveCallback (w, XtNdestroyCallback, fe_delete_cb, context);

    XtRemoveEventHandler (w, StructureNotifyMask, False, fe_map_notify_eh,
			  context);
#ifdef MOCHA
    if (context->type == MWContextBrowser ||
	context->type == MWContextEditor ||
	context->type == MWContextDialog ||
	context->type == MWContextMail ||
	context->type == MWContextNews) {
      XtRemoveEventHandler (w, FocusChangeMask, False, fe_focus_notify_eh, 
			 context);
    }
#endif /* MOCHA */
  }
  XtDestroyWidget (w);

  MimeDestroyContextData(context);

  if (CONTEXT_DATA (context)->ftd) free (CONTEXT_DATA (context)->ftd);
  if (CONTEXT_DATA (context)->sd) free (CONTEXT_DATA (context)->sd);
  if (CONTEXT_DATA(context)->find_data)
    XP_FREE(CONTEXT_DATA(context)->find_data);
  if (context->funcs) free (context->funcs);

#if 0
  {
    int i = 0;
    printf("Before: destroying 0x%x\n", context);
    for (rest = fe_all_MWContexts; rest; rest = rest->next)
      printf("  0x%x; %d\n", rest->context, ++i);
  }
#endif /* 0 */

  for (prev = 0, rest = fe_all_MWContexts;
       rest;
       prev = rest, rest = rest->next)
    if (rest->context == context)
      break;
  if (! rest) abort ();
  if (prev)
    prev->next = rest->next;
  else
    fe_all_MWContexts = rest->next;
  free (rest);

#if 0
  {
    int i = 0;
    printf("After: destroying 0x%x\n", context);
    for (rest = fe_all_MWContexts; rest; rest = rest->next)
      printf("  0x%x; %d\n", rest->context, ++i);
    printf("\n");
  }
#endif /* 0 */

  /* Some window disappears. So we need to recreate windows menu of all
     contexts availables. Mark so. */
  for( rest=fe_all_MWContexts; rest; rest=rest->next )
    CONTEXT_DATA(rest->context)->windows_menu_up_to_date_p = False;

  if (context->title) free (context->title);
  context->title = 0;
  if (NEWS_CONTEXT_DATA (context)) {
    XP_ASSERT(context->type == MWContextNews);
    free(NEWS_CONTEXT_DATA (context));
  }
  XP_RemoveContextFromList(context);
#ifdef JAVA  /* jwz */
  LJ_DiscardEventsForContext(context);
#endif /* JAVA */

  /* If a synchronous url dialog is up, dont free the context. Once the
   * synchronous url dialog is over, we will free it.
   */
  if (fe_IsContextProtected(context)) {
    CONTEXT_DATA(context)->destroyed = 1;
  }
  else {
    free (CONTEXT_DATA (context));
    free (context);
  }
}


/* This is called any time the user performs an action.
   It reorders the list of contexts so that the most recently
   used one is at the front.
 */
void
fe_UserActivity (MWContext *context)
{
  struct fe_MWContext_cons *rest, *prev;

  for (prev = 0, rest = fe_all_MWContexts;
       rest;
       prev = rest, rest = rest->next)
    if (rest->context == context)
      break;
  if (! rest) abort ();		/* not found?? */

  if (context->is_grid_cell) fe_SetGridFocus (context);

  if (! prev) return;		/* it was already first. */

  prev->next = rest->next;
  rest->next = fe_all_MWContexts;
  fe_all_MWContexts = rest;
}


void
fe_RefreshAllAnchors ()
{
  struct fe_MWContext_cons *rest;
  for (rest = fe_all_MWContexts; rest; rest = rest->next)
    LO_RefreshAnchors (rest->context);
}

/*
 * XXX Need to make all contexts be deleted through this. - dp
 */
static void
fe_delete_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;

  if (fe_WindowCount == 1)
    {
      /* Unmap the window right away to give feedback that a delete
	 is in progress. */
      Widget widget = CONTEXT_WIDGET(context);
      Window window = (widget ? XtWindow(widget) : 0);
      if (window)
	XUnmapWindow (XtDisplay(widget), window);

      /* Now save files and exit. */
      fe_Exit (0);
    }
  else
    {
      if ( someGlobalContext == context )
      {
        fe_DestroyContext (context);
        fe_WindowCount--;
 
        someGlobalContext = XP_FindContextOfType(NULL, MWContextBrowser);
        if (!someGlobalContext)
              someGlobalContext = XP_FindContextOfType(NULL, MWContextMail);
        if (!someGlobalContext)
              someGlobalContext = XP_FindContextOfType(NULL, MWContextNews);
        if (!someGlobalContext)
              someGlobalContext = fe_all_MWContexts->context;

        XmAddWMProtocols(CONTEXT_WIDGET(someGlobalContext), 
				&WM_SAVE_YOURSELF,1);
        XmAddWMProtocolCallback(CONTEXT_WIDGET(someGlobalContext),
                WM_SAVE_YOURSELF,
                fe_wm_save_self_cb, someGlobalContext);
     }
     else
     {
        fe_DestroyContext (context);
        fe_WindowCount--;
 
     }

     if (fe_WindowCount <= 0)
	abort ();
    }
}

MWContext *
fe_WidgetToMWContext (Widget widget)
{
  struct fe_MWContext_cons *rest;
  Widget transient_for = NULL;
  while (!XtIsWMShell (widget))
    widget = XtParent (widget);
  if (XtIsSubclass(widget, transientShellWidgetClass))
      XtVaGetValues(widget, XmNtransientFor, &transient_for, 0);
  for (rest = fe_all_MWContexts; rest; rest = rest->next)
    if ((CONTEXT_WIDGET (rest->context) == widget) ||
	(XtParent(CONTEXT_WIDGET (rest->context)) == widget) ||
	(transient_for != NULL && CONTEXT_WIDGET(rest->context) == transient_for))
      return rest->context;
  return 0;
}

/*
 * Now that we have grids, you can't just walk up to the parent shell
 * to find the context for a widget.  We are assuming here that the
 * motion event was always delivered to the drawingarea widget, I hope
 * that is correct --ejb
 */
MWContext *
fe_MotionWidgetToMWContext (Widget widget)
{
  struct fe_MWContext_cons *rest;
  for (rest = fe_all_MWContexts; rest; rest = rest->next)
    if (CONTEXT_DATA (rest->context)->drawing_area == widget)
      return rest->context;
  /* There is no parent -- hope the caller can deal with a null */
  return 0;
}


void
fe_Exit (int status)
{
  struct fe_MWContext_cons *cons;

  /* Try to do these in decreasing order of importance, in case the user
     gets impatient and kills us harder. */

  /* My theory for putting this first is that if the user's keys get
   * trashed they are really fucked, and if their certs get trashed
   * they may have to pay $$$ to get new ones.  If you disagree,
   * lets talk about it.  --jsw
   */
#ifdef HAVE_SECURITY /* added by jwz */
  SECNAV_Shutdown();
#endif /* !HAVE_SECURITY -- added by jwz */

  fe_SaveBookmarks ();
  GH_SaveGlobalHistory ();

  for (cons = fe_all_MWContexts; cons; cons = cons->next)
    {
      switch (cons->context->type)
	{
	case MWContextMail:
	  MSG_CleanupMailContext (cons->context);  /* save summary files */
	  break;
	case MWContextNews:
	  MSG_CleanupNewsContext (cons->context);  /* save newsrc file */
	  break;
	case MWContextMessageComposition:
	  MSG_CleanupMessageCompositionContext (cons->context); /* tmp files */
	  break;
#ifdef EDITOR
	case MWContextEditor:
	  fe_EditorCleanup(cons->context);
	  break;
#endif /*EDITOR*/
	default:
	  break;
	}
#ifdef MOCHA
      LM_RemoveWindowContext (cons->context);
#endif
    }
#ifdef MOCHA
  LM_FinishMocha ();
#endif
#ifdef JAVA
  LJ_ShutdownJava();
#endif
  NET_CleanupCacheDirectory (FE_CacheDir, "cache");
  GH_SaveGlobalHistory ();
  GH_FreeGlobalHistory ();
  NET_ShutdownNetLib ();
  NPL_Shutdown ();
  HOT_FreeBookmarks ();
  GH_FreeGlobalHistory ();

  /* Mailcap and MimeType have been written out when the prefdialog was closed */
  /* We don't need to write again. Just, Clean up these cached mailcap */
  /* and mimetype link list */
  NET_CleanupFileFormat(NULL);
  NET_CleanupMailCapList(NULL);

  if (fe_pidlock) unlink (fe_pidlock);
  fe_pidlock = 0;
  exit (status);
}

/* fe_MimimalNoUICleanup
 *
 * This does a cleanup of the only the absolute essential stuff.
 * - saves bookmarks, addressbook
 * - saves global history
 * - saves cookies
 * (since all these saves are protected by flags anyway, we wont endup saving
 *  again if all these happened before.)
 * - remove lock file if existent
 *
 * This will be called at the following points:
 * 1. when the x server dies [x_fatal_error_handler()]
 * 2. when window manages says 'SAVE_YOURSELF'
 * 3. at_exit_handler()
 */
void
fe_MinimalNoUICleanup()
{
  fe_SaveBookmarks ();
  GH_SaveGlobalHistory ();
  NET_SaveCookies(NULL);
}

/* If there is whitespace at the beginning or end of string, removes it.
   The string is modified in place.
 */
char *
fe_StringTrim (char *string)
{
  char *orig = string;
  char *new;
  if (! string) return 0;
  new = string + strlen (string) - 1;
  while (new >= string && XP_IS_SPACE (new [0]))
    *new-- = 0;
  new = string;
  while (XP_IS_SPACE (*new))
    new++;
  if (new == string)
    return string;
  while (*new)
    *string++ = *new++;
  *string = 0;
  return orig;
}

/*
 * fe_StrEndsWith(char *s, char *endstr)
 *
 * returns TRUE if string 's' ends with string 'endstr'
 * else returns FALSE
 */
XP_Bool
fe_StrEndsWith(char *s, char *endstr)
{
  int l, lend;
  XP_Bool retval = FALSE;

  if (!endstr)
    /* All strings ends in NULL */
    return(TRUE);
  if (!s)
    /* NULL strings will never have endstr at its end */
    return(FALSE);

  lend = strlen(endstr);
  l = strlen(s);
  if (l >= lend && !strcmp(s+l-lend, endstr))
    retval = TRUE;
  return (retval);
}

char *
fe_Basename (char *s)
{
    int len;
    char *p;

    if (!s) return (s);
    len = strlen(s);
    p = &s[len-1];
    
    while(--len > 0 && *p != '/') p--;
    if (*p == '/') p++;
    return (p);
}

void
fe_MidTruncateString (const char *input, char *output, int max_length)
{
  int L = strlen (input);
  if (L <= max_length)
    {
      strncpy (output, input, L+1);
    }
  else
    {
      int mid = (max_length - 3) / 2;
      char *tmp = 0;
      if (input == output)
	{
	  tmp = output;
	  output = (char *) malloc (max_length + 1);
	}
      strncpy (output, input, mid);
      strncpy (output + mid, "...", 3);
      strncpy (output + mid + 3, input + L - mid, mid + 1);

      if (tmp)
	{
	  strncpy (tmp, output, max_length + 1);
	  free (output);
	}
    }
}


/* Mail stuff */

const char *
FE_UsersMailAddress (void)
{
  static char *cached_uid = 0;
  char *uid, *name;
  if (fe_globalPrefs.email_address && *fe_globalPrefs.email_address)
    {
      if (cached_uid) free (cached_uid);
      cached_uid = 0;
      return fe_globalPrefs.email_address;
    }
  else if (cached_uid)
    {
      return cached_uid;
    }
  else
    {
      fe_DefaultUserInfo (&uid, &name, False);
      free (name);
      cached_uid = uid;
      return uid;
    }
}


const char *
FE_UsersRealMailAddress (void)
{
#ifdef DEBUG_jwz
  /* I am who I say I am, piss off. */
  return FE_UsersMailAddress ();
#else /* !DEBUG_jwz */
  static char *cached_uid = 0;
  if (cached_uid)
    {
      return cached_uid;
    }
  else
    {
      char *uid, *name;
      fe_DefaultUserInfo (&uid, &name, True);
      free (name);
      cached_uid = uid;
      return uid;
    }
#endif /* !DEBUG_jwz */
}


const char *
FE_UsersFullName (void)
{
  static char *cached_name = 0;
  char *uid, *name;
  if (fe_globalPrefs.real_name && *fe_globalPrefs.real_name)
    {
      if (cached_name) free (cached_name);
      cached_name = 0;
      return fe_globalPrefs.real_name;
    }
  else if (cached_name)
    {
      return cached_name;
    }
  else
    {
      fe_DefaultUserInfo (&uid, &name, False);
      free (uid);
      cached_name = name;
      return name;
    }
}


const char *
FE_UsersOrganization (void)
{
  static char *cached_org = 0;
  if (cached_org)
    free (cached_org);
  cached_org = strdup (fe_globalPrefs.organization
		       ? fe_globalPrefs.organization
		       : "");
  return cached_org;
}


const char *
FE_UsersSignature (void)
{
  static char *signature = NULL;
  XP_File file;
  time_t sig_date = 0;
	
  if (signature)
    XP_FREE (signature);
		
  file = XP_FileOpen (fe_globalPrefs.signature_file,
		      xpSignature, XP_FILE_READ);
	
  if (file)
    {
      struct stat st;
      char buf [1024];
      char *s = buf;
	
      int left = sizeof (buf) - 2;
      int size;
      *s = 0;

      if (!fstat (fileno (file), &st))
	sig_date = st.st_mtime;

      while ((size = XP_FileRead (s, left, file)) && left > 0)
	{
	  left -= size;
	  s += size;
	}

      *s = 0;

      /* take off all trailing whitespace */
      s--;
      while (s >= buf && isspace (*s))
	*s-- = 0;
      /* terminate with a single newline. */
      s++;
      *s++ = '\n';
      *s++ = 0;
      XP_FileClose (file);
      if ( !strcmp (buf, "\n"))
	signature = NULL;
      else
	signature = strdup (buf);
    }
  else
    signature = NULL;

  /* The signature file date has changed - check the contents of the file
     again, and save that date to the preferences file so that it is checked
     only when the file changes, even if Netscape has been restarted in the
     meantime. */
  if (fe_globalPrefs.signature_date != sig_date)
    {
      MWContext *context =
	XP_FindContextOfType(0, MWContextMessageComposition);
      if (!context) context = fe_all_MWContexts->context;
      MISC_ValidateSignature (context, signature);
      fe_globalPrefs.signature_date = sig_date;

      if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file,
			  &fe_globalPrefs))
	fe_perror (context, XP_GetString( XFE_ERROR_SAVING_OPTIONS ) );
    }

  return signature;
}


/* stderr hackery - Why Unix Sucks, reason number 32767.
 */

#include <fcntl.h>

static char stderr_buffer [1024];
static char *stderr_tail = 0;
static time_t stderr_last_read = 0;
static XtIntervalId stderr_dialog_timer = 0;

extern FILE *real_stderr;
/* jwz: can't use stdout as a top-level lvalue in RH 6.2 */
/* FILE *real_stdout = stdout; */
FILE *real_stdout = 0;

static void
fe_stderr_dialog_timer (XtPointer closure, XtIntervalId *id)
{
  char *s = fe_StringTrim (stderr_buffer);
  if (*s)
    {
      /* If too much data was printed, then something has gone haywire,
	 so truncate it. */
      char *trailer = XP_GetString(XFE_STDERR_DIAGNOSTICS_HAVE_BEEN_TRUNCATED);
      int max = sizeof (stderr_buffer) - strlen (trailer) - 5;
      if (strlen (s) > max)
	strcpy (s + max, trailer);

      /* Now show the user.
	 Possibly we should do something about not popping this up
	 more often than every few seconds or something.
       */
      fe_stderr (fe_all_MWContexts->context, s);
    }

  stderr_tail = stderr_buffer;
  stderr_dialog_timer = 0;
}


static void
fe_stderr_callback (XtPointer closure, int *fd, XtIntervalId *id)
{
  char *s;
  int left;
  int size;
  int read_this_time = 0;

  if (stderr_tail == 0)
    stderr_tail = stderr_buffer;

  left = ((sizeof (stderr_buffer) - 2)
	  - (stderr_tail - stderr_buffer));

  s = stderr_tail;
  *s = 0;

  /* Read as much data from the fd as we can, up to our buffer size. */
  if (left > 0)
    {
#if 1
      while ((size = read (*fd, (void *) s, left)) > 0)
	{
	  left -= size;
	  s += size;
	  read_this_time += size;
	}
#else
      size = read (*fd, (void *) s, left);
      left -= size;
      s += size;
      read_this_time += size;
#endif

      *s = 0;
    }
  else
    {
      char buf2 [1024];
      /* The buffer is full; flush the rest of it. */
      while (read (*fd, (void *) buf2, sizeof (buf2)) > 0)
	;
    }

  stderr_tail = s;
  stderr_last_read = time ((time_t *) 0);

  /* Now we have read some data that we would like to put up in a dialog
     box.  But more data may still be coming in - so don't pop up the
     dialog right now, but instead, start a timer that will pop it up
     a second from now.  Should more data come in in the meantime, we
     will be called again, and will reset that timer again.  So the
     dialog will only pop up when a second has elapsed with no new data
     being written to stderr.

     However, if the buffer is full (meaning lots of data has been written)
     then we don't reset the timer.
   */
  if (read_this_time > 0)
    {
      if (stderr_dialog_timer)
	XtRemoveTimeOut (stderr_dialog_timer);

      stderr_dialog_timer =
	XtAppAddTimeOut (fe_XtAppContext, 1 * 1000,
			 fe_stderr_dialog_timer, 0);
    }
}

static void
fe_initialize_stderr (void)
{
  static Boolean done = False;
  int fds [2];
  int in, out;
  int new_stdout, new_stderr;
  int stdout_fd = 1;
  int stderr_fd = 2;
  int flags;

  if (done) return;

  real_stderr = stderr;
  real_stdout = stdout;

  if (!fe_globalData.stderr_dialog_p && !fe_globalData.stdout_dialog_p)
    return;

  done = True;

  if (pipe (fds))
    {
      fe_perror (fe_all_MWContexts->context,
                 XP_GetString( XFE_ERROR_CREATING_PIPE) );
      return;
    }

  in = fds [0];
  out = fds [1];

# ifdef O_NONBLOCK
  flags = O_NONBLOCK;
# else
#  ifdef O_NDELAY
  flags = O_NDELAY;
#  else
  ERROR!! neither O_NONBLOCK nor O_NDELAY are defined.
#  endif
# endif

    /* Set both sides of the pipe to nonblocking - this is so that
       our reads (in fe_stderr_callback) will terminate, and so that
       out writes (in net_ExtViewWrite) will silently fail when the
       pipe is full, instead of hosing the program. */
  if (fcntl (in, F_SETFL, flags) != 0)
    {
      fe_perror (fe_all_MWContexts->context, "fcntl:");
      return;
    }
  if (fcntl (out, F_SETFL, flags) != 0)
    {
      fe_perror (fe_all_MWContexts->context, "fcntl:");
      return;
    }

  if (fe_globalData.stderr_dialog_p)
    {
      FILE *new_stderr_file;
      new_stderr = dup (stderr_fd);
      if (new_stderr < 0)
	{
	  fe_perror (fe_all_MWContexts->context, "could not dup() a stderr:");
	  return;
	}
      if (! (new_stderr_file = fdopen (new_stderr, "w")))
	{
	  fe_perror (fe_all_MWContexts->context,
		     "could not fdopen() the new stderr:");
	  return;
	}
      real_stderr = new_stderr_file;

      close (stderr_fd);
      if (dup2 (out, stderr_fd) < 0)
	{
	  fe_perror (fe_all_MWContexts->context,
		     "could not dup() a new stderr:");
	  return;
	}
    }

  if (fe_globalData.stdout_dialog_p)
    {
      FILE *new_stdout_file;
      new_stdout = dup (stdout_fd);
      if (new_stdout < 0)
	{
	  fe_perror (fe_all_MWContexts->context, "could not dup() a stdout:");
	  return;
	}
      if (! (new_stdout_file = fdopen (new_stdout, "w")))
	{
	  fe_perror (fe_all_MWContexts->context,
		     "could not fdopen() the new stdout:");
	  return;
	}
      real_stdout = new_stdout_file;

      close (stdout_fd);
      if (dup2 (out, stdout_fd) < 0)
	{
	  fe_perror (fe_all_MWContexts->context,
		     "could not dup() a new stdout:");
	  return;
	}
    }

  XtAppAddInput (fe_XtAppContext, in,
		 (XtPointer) (XtInputReadMask /* | XtInputExceptMask */),
		 fe_stderr_callback, 0);
}



#if 0
Boolean
fe_DisplayIsLocal (MWContext *context)
{
  Display *display = XtDisplay (CONTEXT_WIDGET (context));
  int not_on_console = 1;
  char *dpy = DisplayString (display);
  char *tail = (char *) strchr (dpy, ':');
  if (! tail || strncmp (tail, ":0", 2))
    not_on_console = 1;
  else
    {
      char dpyname[255], localname[255];
      strncpy (dpyname, dpy, tail-dpy);
      dpyname [tail-dpy] = 0;
      if (!*dpyname ||
	  !strcmp(dpyname, "unix") ||
	  !strcmp(dpyname, "localhost"))
	not_on_console = 0;
      else if (gethostname (localname, sizeof (localname)))
	not_on_console = 1;  /* can't find hostname? */
      else
	{
	  /* We have to call gethostbyname() on the result of gethostname()
	     because the two aren't guarenteed to be the same name for the
	     same host: on some losing systems, one is a FQDN and the other
	     is not.  Here in the wide wonderful world of Unix it's rocket
	     science to obtain the local hostname in a portable fashion.
	     
	     And don't forget, gethostbyname() reuses the structure it
	     returns, so we have to copy the fucker before calling it again.
	     Thank you master, may I have another.
	   */
	  struct hostent hpbuf, *h = PR_gethostbyname (dpyname, &hpbuf);
	  if (!h)
	    not_on_console = 1;
	  else
	    {
	      char hn [255];
	      struct hostent *l;
	      strcpy (hn, h->h_name);
	      l = PR_n_console = (!l || !!(strcmp (l->h_name, hn)));
	    }
	}
    }
  return !not_on_console;
}
#endif


int32
FE_GetContextID (MWContext * window_id)
{
   return((int32) window_id);
}

XP_Bool
XFE_UseFancyFTP (MWContext *context)
{
  return CONTEXT_DATA (context)->fancy_ftp_p;
}

XP_Bool
XFE_UseFancyNewsgroupListing (MWContext * window_id)
{
  return False;
}

/* FE_ShowAllNewsArticles
 *
 * Return true if the user wants to see all newsgroup  
 * articles and not have the number restricted by
 * .newsrc entries
 */
XP_Bool
XFE_ShowAllNewsArticles (MWContext *window_id)
{
    return(FALSE);  /* temporary LJM */
}

int 
XFE_FileSortMethod (MWContext * window_id)
{
   return (SORT_BY_NAME);
}

int16
INTL_DefaultDocCharSetID(MWContext *cxt)
{
	int16	csid;

	if (cxt)
	{
		if (cxt->doc_csid)
		{
			csid = cxt->doc_csid;
		}
		else if (cxt->fe.data && cxt->fe.data->xfe_doc_csid)
		{
			csid = cxt->fe.data->xfe_doc_csid;
		}
		else
		{
			csid = fe_globalPrefs.doc_csid;
		}
	}
	else
	{
		csid = fe_globalPrefs.doc_csid;
	}

#if 0
	{
		char	charset[128];
		int16	id;

		if (cxt && cxt->fe.data)
		{
			id = cxt->fe.data->xfe_doc_csid;
			INTL_CharSetIDToName(id & ~CS_AUTO, charset);
			(void) fprintf(real_stderr,
				"context doc charset: \"%s\" (%s)", charset,
				(id & CS_AUTO) ? "auto" : "not auto");
		}
		else
		{
			(void) fprintf(real_stderr,
				"context doc charset: NULL");
		}
		INTL_CharSetIDToName(fe_globalPrefs.doc_csid, charset);
		(void) fprintf(real_stderr,
			"; global doc charset: \"%s\" (%s)\n", charset,
			(fe_globalPrefs.doc_csid & CS_AUTO) ? "auto" :
			"not auto");
	}
#endif /* 0 */

	return csid;
}

char *
INTL_ResourceCharSet(void)
{
	return fe_LocaleCharSetName;
}

void
INTL_Relayout(MWContext *pContext)
{
	fe_ReLayout(pContext, 0);
}



typedef struct fe_timeout {
  TimeoutCallbackFunction func;
  void* closure;
  XtIntervalId timer;
} fe_timeout;


static void
fe_do_timeout(XtPointer p, XtIntervalId* id)
{
  fe_timeout* timer = (fe_timeout*) p;
  XP_ASSERT(timer->timer == *id);
  (*timer->func)(timer->closure);
  XP_FREE(timer);
}

void* 
FE_SetTimeout(TimeoutCallbackFunction func, void* closure, uint32 msecs)
{
  fe_timeout* timer;
  timer = XP_NEW(fe_timeout);
  if (!timer) return NULL;
  timer->func = func;
  timer->closure = closure;
  timer->timer = XtAppAddTimeOut(fe_XtAppContext, msecs, fe_do_timeout, timer);
  return timer;
}


void 
FE_ClearTimeout(void* timer_id)
{
  fe_timeout* timer = (fe_timeout*) timer_id;
  XtRemoveTimeOut(timer->timer);
  XP_FREE(timer);
}

void
FE_UpdateChrome(MWContext *context, Chrome *chrome)
{
    if (!chrome || !context || context->type != MWContextBrowser)
	return;

    if (fe_respect_chrome(context, chrome))
	fe_RebuildWindow(context);
}

char *
FE_GetCipherPrefs(void)
{
    if (fe_globalPrefs.cipher == NULL)
	return NULL;
    return(strdup(fe_globalPrefs.cipher));
}

void
FE_SetCipherPrefs(MWContext *context, char *cipher)
{
  if (fe_globalPrefs.cipher) {
    if (!strcmp(fe_globalPrefs.cipher, cipher))
      return;
    XP_FREE(fe_globalPrefs.cipher);
  }
  fe_globalPrefs.cipher = strdup(cipher);
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
  {
      if (context == NULL) {
	  MWContext *someContext;
	  /* Type to find a context */
	  someContext = XP_FindContextOfType(NULL, MWContextBrowser);
	  if (!someContext)
	      someContext = XP_FindContextOfType(NULL, MWContextMail);
	  if (!someContext)
	      someContext = XP_FindContextOfType(NULL, MWContextNews);
	  if (!someContext)
	      someContext = fe_all_MWContexts->context;
	  context = someContext;
      }
      if (context != NULL)
	  fe_perror (context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));
  }
}


/************************
 * File status routines *
 ************************/
/*
 * File changes since last seen.
 * For error case of file not present, it returns FALSE and doesn't change
 *	return value new_mtime.
 * If file has changed, returns TRUE and new_mtime if not null is updated
 *	to the new modified time.
 * If file has not changed, return FALSE and doesn't change new_mtime.
 */
XP_Bool
fe_isFileChanged(char *name, time_t mtime, time_t *new_mtime)
{
    XP_StatStruct st;
    XP_Bool ret = FALSE;
    
    if (name && *name && !stat(name, &st))
      if (st.st_mtime != mtime) ret = TRUE;

    if (ret && new_mtime)
      *new_mtime = st.st_mtime;

    return (ret);
}

/*
 * File exists
 */
Boolean
fe_isFileExist(char *name)
{
    XP_StatStruct st;
    if (!name || !*name) return (False);
    if (!stat (name, &st))
	return (True);
    else
	return (False);
}

/*
 * File exists and is readable.
 */
Boolean
fe_isFileReadable(char *name)
{
    FILE *fp;
    if (!name || !*name) return (False);
    fp = fopen(name, "r");
    if (fp) {
	fclose(fp);
	return (True);
    }
    else
	return (False);
}

/*
 * File is a directory
 */
Boolean
fe_isDir(char *name)
{
    XP_StatStruct st;
    if (!name || !*name) return (False);
    if (!stat (name, &st) && S_ISDIR(st.st_mode))
	return (True);
    else
	return (False);
}

/*
 *    Walks over a tree, calling mappee callback on each widget.
 */
XtPointer
fe_WidgetTreeWalk(Widget widget, fe_WidgetTreeWalkMappee callback,
		  XtPointer data)
{
  Arg       av[8];
  Cardinal  ac;
  Widget*   children;
  Cardinal  nchildren;
  Cardinal  i;
  XtPointer rv;

  if (widget == NULL || callback == NULL)
      return 0;

  if (XtIsSubclass(widget, compositeWidgetClass)) {

      ac = 0;
      XtSetArg(av[ac], XmNchildren, &children); ac++;
      XtSetArg(av[ac], XmNnumChildren, &nchildren); ac++;
      XtGetValues(widget, av, ac);
      
      for (i = 0; i < nchildren; i++) {
	  rv = fe_WidgetTreeWalk(children[i], callback, data);
	  if (rv != 0)
	      return rv;
      }
  }

  return (callback)(widget, data);
}

/*
 * fe_WidgetTreeWalkChildren
 *
 * Intension here is to call the mappee callback for all children in the
 * tree taking into account the cascade menus.
 */
XtPointer
fe_WidgetTreeWalkChildren(Widget widget, fe_WidgetTreeWalkMappee callback,
			  XtPointer closure)
{
  Widget *buttons = 0, menu = 0;
  Cardinal nbuttons = 0;
  int i;
  XtPointer ret = 0;
  XtVaGetValues (widget, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i = 0; ret == 0 && i < nbuttons; i++)
    {
      Widget item = buttons[i];
      if (XmIsToggleButton(item) || XmIsToggleButtonGadget(item) ||
	      XmIsPushButton(item) || XmIsPushButtonGadget(item) ||
	      XmIsCascadeButton(item) || XmIsCascadeButtonGadget(item))
	ret = (callback) (item, closure);
      if (ret != 0) break;
      if (XmIsCascadeButton(item) || XmIsCascadeButtonGadget(item)) {
	XtVaGetValues (item, XmNsubMenuId, &menu, 0);
	if (menu)
	  ret = fe_WidgetTreeWalkChildren(menu, callback, closure);
      }
    }
  return(ret);
}


static XtPointer
fe_find_widget_mappee(Widget widget, XtPointer data)
{
    char* match_name = (char*)data;
    char* name = XtName(widget);

    if (strcmp(name, match_name) == 0) {
	return widget; /* non-zero, will force termination of walk */
    } else {
	return 0;
    }
}

Widget
fe_FindWidget(Widget top, char* name)
{
    XtPointer rv;

    rv = fe_WidgetTreeWalk(top, fe_find_widget_mappee, (XtPointer)name);
    
    return (Widget)rv;
}

Boolean
fe_IsOptionMenu(Widget widget)
{
    unsigned char type;
    if (XtClass(widget) == xmRowColumnWidgetClass) {
	XtVaGetValues(widget, XmNrowColumnType, &type, 0);
	if (type == XmMENU_OPTION)
	    return TRUE;
    }
    return FALSE;
}

char*
XtClassName(Widget w) {
  char* class_name = XtClass(w)->core_class.class_name;

  return class_name;
}

char*
fe_ResourceString(Widget widget, char* res_name, char* res_class)
{
    XtResource resource;
    XmString   result = 0;

    resource.resource_name = res_name;
    resource.resource_class = res_class;
    resource.resource_type = XmRString;
    resource.resource_size = sizeof(String);
    resource.resource_offset = 0;
    resource.default_type = XtRImmediate;
    resource.default_addr = 0;

    XtGetSubresources(XtParent(widget), (XtPointer)&result, XtName(widget),
		      XtClassName(widget), &resource, 1, NULL, 0);
    
    return result;
}

/*
 *    Tool tips.
 */
static Widget       fe_tool_tips_widget;   /* current hot widget */
static XtIntervalId fe_tool_tips_timer_id; /* timer id for non-movement */
static Widget       fe_tool_tips_shell;    /* posted tips shell */

static void
fe_cleanup_tooltips(MWContext *context)
{
    fe_tool_tips_widget = NULL;

    /*
     *    Stage two? Any event should zap that.
     */
    if (fe_tool_tips_shell) {
	XtDestroyWidget(fe_tool_tips_shell);
	fe_tool_tips_shell = NULL;
    }
    
    /*
     *   Stage one?
     */
    if (fe_tool_tips_timer_id) {
	XtRemoveTimeOut(fe_tool_tips_timer_id);
	fe_tool_tips_timer_id = 0;
    }
}

static XP_Bool
fe_display_docString(MWContext *context, Widget widget, Boolean erase)
{
    char *s;

    if (!erase) {
	s = fe_ResourceString(widget,
			       "documentationString", "DocumentationString");
	if (s == NULL) return(FALSE);
      }
    else
	s = "";

#ifdef DEBUG
    if (s == NULL) {
        static char buf[64];
	s = buf;
	sprintf(s, "Debug: no documentationString resource for widget %s",
		XtName(widget));
    }
#endif /*DEBUG*/

    if (context != NULL && s != NULL)
        XFE_Progress(context, s);

    return(TRUE);
}

static void
fe_tooltips_display_stage_one(Widget widget, Boolean begin)
{
    MWContext* context = fe_WidgetToMWContext(widget);
    if (context)
        fe_display_docString(context, widget, !begin);
}

static Widget
fe_tooltip_create_effects(Widget parent, char* name, char* string)
{
    Widget shell;
    Widget label;
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    XmFontList fontList;
    XmString xm_string;

    shell = parent;
    while (XtParent(shell) && !XtIsShell(shell)) {
	shell = XtParent(shell);
    }

    if (shell == NULL || XtParent(shell) == NULL)
        return NULL;

    XtVaGetValues(shell, XtNvisual, &v, XtNcolormap, &cmap,
		   XtNdepth, &depth, 0);

    XtVaGetValues(parent, XmNfontList, &fontList, NULL);

    shell = XtVaCreateWidget(name,
			     overrideShellWidgetClass,
			     XtParent(shell), /* the app */
			     XmNvisual, v,
			     XmNcolormap, cmap,
			     XmNdepth, depth,
			     XmNborderWidth, 1,
			     NULL);

    xm_string = XmStringCreateLocalized(string);

    label = XtVaCreateManagedWidget("tipLabel",
				    xmLabelWidgetClass,
				    shell,
				    XmNlabelType, XmSTRING,
				    XmNlabelString, xm_string,
				    NULL);
    XmStringFree(xm_string);
    XtManageChild(label);

    return label;
}

static Widget
fe_tooltips_display_stage_two(Widget widget)
{
    Widget parent;
    Widget label;
    Dimension width;
    Dimension border_width;
    Dimension height;
    Position x_root;
    Position y_root;
    Position y_root_orig;
    Screen* screen;
    char* s = fe_ResourceString(widget, "tipString", "TipString");

#ifdef DEBUG
    if (s == NULL) {
        static char buf[64];
	s = buf;
	sprintf(s, "Debug: no tipString resource for widget %s\n"
		   "This message only appears in a DEBUG build",
		XtName(widget));
    }
#endif /*DEBUG*/

    if (s == NULL)
      return NULL;

    parent = XtParent(widget);

    label = fe_tooltip_create_effects(parent, "tipShell", s);
    if (label == NULL)
        return NULL;

    parent = XtParent(label);

    XtVaGetValues(widget, XmNwidth, &width, XmNheight, &height, 0);

    XtTranslateCoords(widget, 0, 0, &x_root, &y_root_orig);
    x_root += (width/2); /* positon in center of button */
    y_root = y_root_orig + height + 5;

    /*
     *    Make sure it fits on screen.
     */
    XtVaGetValues(parent, XmNborderWidth, &border_width, 0);
    XtVaGetValues(label, XmNwidth, &width, XmNheight, &height, 0);
    screen = XtScreen(label);

    height += (2*border_width);
    width += (2*border_width);

    if (x_root + width > WidthOfScreen(screen))
	x_root = WidthOfScreen(screen) - width;
    else if (x_root < 0)
	x_root = 0;
    if (y_root + height > HeightOfScreen(screen))
	y_root = y_root_orig - height - 5;
    else if (y_root < 0)
	y_root = 0;

    XtVaSetValues(parent, XmNx, x_root, XmNy, y_root, 0);

    /*
     *    Make sure the user cannot shoot themselves with a random
     *    geometry spec. No more attack of the killer tomatoes...djw
     */
    {
    char buf[128];
    sprintf(buf, "%dx%d", width, height);
    XtVaSetValues(parent, XmNwidth, width, XmNheight, height,
		  XmNgeometry, buf, 0);
    }

    XtPopup(parent, XtGrabNone);

    return parent;
}

static void
fe_tooltips_stage_two_timeout(XtPointer closure, XtIntervalId *id)
{
    Widget     widget = (Widget)closure;
    Widget     shell;

    /* Clear the timer id that we store in the context as once the timeout
     * has triggered (that is why we are here), the timeout is automatically
     * removed. Else our event handler will go and remove the timeout again.
     */
    fe_tool_tips_timer_id = 0;

    if (fe_tool_tips_shell == NULL) {
	shell = fe_tooltips_display_stage_two(widget);
	fe_tool_tips_shell = shell;
    }

}

static Boolean
fe_manager_tt_demo_check(Widget widget)
{
    return TRUE;
}

extern Widget _XmInputInGadget(Widget, int x, int y); /* in Xm */

static void
fe_tooltips_event_handler(Widget widget, XtPointer closure, XEvent* event,
			Boolean* keep_going)
{
    fe_ToolTipGadgetCheckProc check = (fe_ToolTipGadgetCheckProc)closure;
    Boolean enter;
    Boolean leave;
    Widget  child;
    Boolean demo;
    Boolean tips_enabled;

    *keep_going = TRUE;

    if (check == fe_manager_tt_demo_check) {
	demo = TRUE;
	check = NULL;
    } else {
	demo = FALSE;
    }

    if (event->type == MotionNotify) {
	enter = FALSE;
	leave = FALSE;
	
	if (check != NULL) { /* gadgets */
	    child = _XmInputInGadget(widget,
				     event->xmotion.x, event->xmotion.y);
	    
	    if (child != NULL && (*check)(child) == TRUE) {
		widget = child;
		if (fe_tool_tips_widget == NULL) {
		    leave = FALSE;
		    enter = TRUE;
		} else if (fe_tool_tips_widget != widget) {
		    leave = TRUE;
		    enter = TRUE;
		}
	    } else {
		if (fe_tool_tips_widget != NULL) {
		    leave = TRUE;
		    enter = FALSE;
		}
	    }
	} else { /* motion in non-manager widget */
	    if (fe_tool_tips_widget == NULL) {
		leave = FALSE;
		enter = TRUE;
	    } else if (fe_tool_tips_widget != widget) {
		leave = TRUE;
		enter = TRUE;
	    }
	}

	if (leave == FALSE && enter == FALSE) /* motion */
	    return;

    } else if (event->type == EnterNotify) {
	enter = TRUE;
	leave = FALSE;
    } else if (event->type == LeaveNotify){
	enter = FALSE;
	leave = TRUE;
    } else { /* clicks, pops, squeaks, and other non-motionals */
	enter = FALSE;
	leave = FALSE;
    }

    /*
     *    Stage two? Any event should zap that.
     */
    if (fe_tool_tips_shell) {
	XtDestroyWidget(fe_tool_tips_shell);
	fe_tool_tips_shell = NULL;
    }
    
    /*
     *   Stage one?
     */
    if (fe_tool_tips_timer_id) {
	XtRemoveTimeOut(fe_tool_tips_timer_id);
	fe_tool_tips_timer_id = 0;
    }
    
    if (leave == TRUE) {
	fe_tooltips_display_stage_one(widget, FALSE); /* clear */
	fe_tool_tips_widget = NULL;
    }
    
    if (enter == TRUE) {
	if (demo) {
	    tips_enabled = XmToggleButtonGetState(widget);
	} else {
	    tips_enabled  = fe_globalPrefs.toolbar_tips_p;
	}

	if (tips_enabled) {
	    fe_tool_tips_timer_id = XtAppAddTimeOut(fe_XtAppContext,
						500, /* whatever */
						fe_tooltips_stage_two_timeout,
						widget);
	}
	fe_tooltips_display_stage_one(widget, TRUE); /* msg */
	fe_tool_tips_widget = widget;
    }
}

static Boolean
fe_manager_tt_default_check(Widget widget)
{
    return (XtIsSubclass(widget, xmPushButtonGadgetClass)
	    ||
	    XtIsSubclass(widget, xmToggleButtonGadgetClass)
	    ||
	    XtIsSubclass(widget, xmCascadeButtonGadgetClass));
}

void
fe_ManagerAddGadgetToolTips(Widget manager, fe_ToolTipGadgetCheckProc do_class)
{
    if (!do_class)
	do_class = fe_manager_tt_default_check;

#if 0
    XtAddEventHandler(manager,
		      PointerMotionMask|LeaveWindowMask|ButtonPressMask,
		      FALSE, fe_tooltips_event_handler, do_class); 
#else
    XtInsertEventHandler(manager,
			 PointerMotionMask|LeaveWindowMask| \
			 ButtonPressMask|ButtonReleaseMask| \
			 KeyPressMask|KeyReleaseMask,
			 FALSE, fe_tooltips_event_handler,
			 do_class, XtListHead);
#endif
}

/* Document String Only */
static void fe_docString_disarm_cb(Widget, XtPointer, XtPointer);
static void fe_docString_arm_cb(Widget, XtPointer, XtPointer);

static void
fe_docString_disarm_cb(Widget w, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;
    fe_display_docString(context, w, TRUE);
}

static void
fe_docString_arm_cb(Widget w, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;

    if (!fe_display_docString(context, w, FALSE)) {
	/* Arm failed to get any string. Dont waste time from now on for
	 * trying to show the docString everytime we arm this widget.
	 */
	XtRemoveCallback(w, XmNarmCallback, fe_docString_arm_cb, context);
	XtRemoveCallback(w, XmNdisarmCallback, fe_docString_disarm_cb,context);
    }

}

void
fe_WidgetAddDocumentString(MWContext *context, Widget widget)
{
    XtAddCallback(widget, XmNarmCallback, fe_docString_arm_cb, context);
    /* Add the disarm callback to erase the document string */
    XtAddCallback(widget, XmNdisarmCallback, fe_docString_disarm_cb, context);
}


void
fe_WidgetAddToolTips(Widget widget)
{
#if 0
    XtAddEventHandler(widget,
		      PointerMotionMask|LeaveWindowMask|ButtonPressMask,
		      FALSE, fe_tooltips_event_handler, NULL);
#else
    XtInsertEventHandler(widget,
			 PointerMotionMask|EnterWindowMask|LeaveWindowMask| \
			 ButtonPressMask|ButtonReleaseMask| \
			 KeyPressMask|KeyReleaseMask,
			 FALSE, fe_tooltips_event_handler,
			 NULL, XtListHead);
#endif
}



Widget
fe_CreateToolTipsDemoToggle(Widget parent, char* name, Arg* args, Cardinal n)
{
    Widget widget;

    widget = XmCreateToggleButton(parent, name, args, n);

    XtInsertEventHandler(widget,
			 PointerMotionMask|EnterWindowMask|LeaveWindowMask| \
			 ButtonPressMask|ButtonReleaseMask| \
			 KeyPressMask|KeyReleaseMask,
			 FALSE, fe_tooltips_event_handler,
			 fe_manager_tt_demo_check, XtListHead);
    return widget;
}

Boolean
fe_ManagerCheckGadgetToolTips(Widget manager, fe_ToolTipGadgetCheckProc check)
{
    Cardinal   nchildren;
    WidgetList children;
    Widget     widget;
    int        i;

    if (!check)
	check = fe_manager_tt_default_check;

    XtVaGetValues(manager,
		  XmNchildren, &children, XmNnumChildren, &nchildren, 0);

    for (i = 0; i < nchildren; i++) {
	widget = children[i];
	if ((*check)(widget) == TRUE)
	    return TRUE;
    }
    return FALSE;
}

Boolean
fe_ContextHasPopups(MWContext* context)
{
    Widget shell = CONTEXT_WIDGET(context);

    return (shell->core.num_popups > 0);
}

/*
 * Motif 1.2 core dumps if right/left arrow keys are pressed while an option
 * menu is active. So always use fe_CreateOptionMenu() instead of
 * XmCreateOptionMenu(). This will create one and set the traversal off
 * on the popup submenu.
 * - dp
 */
Widget
fe_CreateOptionMenu(Widget parent, char* name, Arg* p_argv, Cardinal p_argc)
{
	Widget menu;
	Widget submenu;

	menu = XmCreateOptionMenu(parent, name, p_argv, p_argc);

	XtVaGetValues(menu, XmNsubMenuId, &submenu, 0);
	if (submenu)
	  XtVaSetValues(submenu, XmNtraversalOn, False, 0);

	return menu;
}


/*
 * fe_ProtectContext()
 *
 * Sets the dont_free_context_memory variable in the CONTEXT_DATA. This
 * will prevent the memory for the context and CONTEXT_DATA to be freed
 * even if the context was destroyed.
 */
void
fe_ProtectContext(MWContext *context)
{
  unsigned char del = XmDO_NOTHING;

  if (!CONTEXT_DATA(context)->dont_free_context_memory)
    {
      /* This is the first person trying to protect this context. 
       * Dont allow the user to destroy this context using the windowmanger
       * delete menu item.
       */
      XtVaGetValues(CONTEXT_WIDGET(context), XmNdeleteResponse, &del, 0);
      CONTEXT_DATA(context)->delete_response = del;
      XtVaSetValues(CONTEXT_WIDGET(context),
		    XmNdeleteResponse, XmDO_NOTHING, 0);
    }
  CONTEXT_DATA(context)->dont_free_context_memory++;
}

/*
 * fe_UnProtectContext()
 *
 * Undo what fe_ProtectContext() does. Unsets dont_free_context_memory
 * variable in the context_data.
 */
void
fe_UnProtectContext(MWContext *context)
{
  XP_ASSERT(CONTEXT_DATA(context)->dont_free_context_memory);
  if (CONTEXT_DATA(context)->dont_free_context_memory)
    {
      CONTEXT_DATA(context)->dont_free_context_memory--;
      if (!CONTEXT_DATA(context)->dont_free_context_memory) {
	/* This is the last person unprotecting this context. 
	 * Set the delete_response to what it was before.
	 */
	XtVaSetValues(CONTEXT_WIDGET(context), XmNdeleteResponse,
		      CONTEXT_DATA(context)->delete_response, 0);
      }
    }
}

/*
 * fe_IsContextProtected()
 *
 * Return the protection state of the context.
 */
Boolean
fe_IsContextProtected(MWContext *context)
{
  return (CONTEXT_DATA(context)->dont_free_context_memory);
}

/*
 * fe_IsContextDestroyed()
 *
 * Return if the context was destroyed. This is valid only for protected
 * contexts.
 */
Boolean
fe_IsContextDestroyed(MWContext *context)
{
  return (CONTEXT_DATA(context)->destroyed);
}
