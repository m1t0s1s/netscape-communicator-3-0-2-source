/* -*- Mode: C; tab-width: 8 -*-
   thermo.c --- The thermometer and run light.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created from dialogs.c by Jamie Zawinski <jwz@netscape.com>, 14-Jun-94.
 */

#include "mozilla.h"
#include "xfe.h"
#include "xp_thrmo.h"

#include <X11/IntrinsicP.h>	/* for w->core.width, which is way faster */

/*#define DEBUG_THERMO*/
#define LO_THERMO


/* This is so that we (attempt) to update the display at least once a second,
   even if the network library isn't calling us (as when the remote host is
   being slow.) 
   */
static void
fe_progress_dialog_timer (XtPointer closure, XtIntervalId *id)
{
  MWContext *context = (MWContext *) closure;
  fe_UpdateGraph (context, True);
  /* If we don't know the total length (meaning cylon-mode) update every
     1/10th second to make the slider buzz along.  Otherwise, if we know
     the length, only update every 1/2 second.
   */
  CONTEXT_DATA (context)->thermo_timer_id =
    XtAppAddTimeOut (fe_XtAppContext,
		     (
#ifdef LO_THERMO
		       CONTEXT_DATA (context)->thermo_lo_percent <= 0
#else
		       CONTEXT_DATA (context)->thermo_total <= 0
#endif
		       ? 100
		       : 500),
		     fe_progress_dialog_timer, closure);
}

static void
fe_LED_dialog_timer (XtPointer closure, XtIntervalId *id)
{
  MWContext *context = (MWContext *) closure;
  fe_UpdateLED (context, True);
  CONTEXT_DATA (context)->LED_timer_id =
    XtAppAddTimeOut (fe_XtAppContext, CONTEXT_DATA (context)->busy_blink_rate,
		     fe_LED_dialog_timer, closure);
}

static void
fe_anim_timer (XtPointer closure, XtIntervalId *id)
{
  MWContext *context = (MWContext *) closure;
  int count = ((CONTEXT_DATA (context)->show_toolbar_p ? 1 : 0) +
	       (CONTEXT_DATA (context)->show_directory_buttons_p ? 1 : 0) +
	       (CONTEXT_DATA (context)->show_url_p ? 1 : 0));
  if (context->type == MWContextSaveToDisk) count = 2;
  fe_DrawLogo (context, True, (count > 1));
  CONTEXT_DATA (context)->anim_timer_id =
    XtAppAddTimeOut (fe_XtAppContext, CONTEXT_DATA (context)->anim_rate,
		     fe_anim_timer, closure);
}

/* Start blinking the light and drawing the thermometer.
   This is done before the first call to FE_GraphProgressInit()
   to make sure that we indicate that we are busy before the
   first connection has been established.
 */
void
fe_StartProgressGraph (MWContext *context)
{
  time_t now = time ((time_t *) 0);

  /* If thermo is not there, then we turn off both ICON animation and thermo */
  if (CONTEXT_DATA(context)->thermo == 0) return;

#ifdef DEBUG_THERMO
  fprintf (stderr, "FE_StartProgressGraph()\n");
#endif

  CONTEXT_DATA (context)->thermo_start_time = now;
  CONTEXT_DATA (context)->thermo_last_update_time = now;
  CONTEXT_DATA (context)->thermo_data_start_time = 0;

  CONTEXT_DATA (context)->thermo_size_unknown_count = 0;
  CONTEXT_DATA (context)->thermo_total = 0;
  CONTEXT_DATA (context)->thermo_current = 0;
  CONTEXT_DATA (context)->thermo_lo_percent = 0;

  CONTEXT_DATA (context)->LED_on_p = False; /* force it on */

  if (!CONTEXT_DATA (context)->thermo_timer_id)
    fe_progress_dialog_timer (context, 0);

  if (!CONTEXT_DATA (context)->LED_timer_id)
    fe_LED_dialog_timer (context, 0);

  if(!CONTEXT_DATA (context)->anim_timer_id)
    fe_anim_timer (context, 0);
}

/* Shut off the progress graph and blinking light completely.
 */
void
fe_StopProgressGraph (MWContext *context)
{

#ifdef GRIDS
  if (context->is_grid_cell)
  {
    MWContext *top_context = context;
    while ((top_context->grid_parent)&&(top_context->grid_parent->is_grid_cell))
      {
	top_context = top_context->grid_parent;
      }
    context = top_context->grid_parent;
  }
#endif /* GRIDS */

  if (CONTEXT_DATA (context)->thermo_timer_id)
    {
      XtRemoveTimeOut (CONTEXT_DATA (context)->thermo_timer_id);
      CONTEXT_DATA (context)->thermo_timer_id = 0;
    }

  if (CONTEXT_DATA (context)->LED_timer_id)
    {
      XtRemoveTimeOut (CONTEXT_DATA (context)->LED_timer_id);
      CONTEXT_DATA (context)->LED_timer_id = 0;
    }
  if (CONTEXT_DATA (context)->anim_timer_id)
    {
      XtRemoveTimeOut (CONTEXT_DATA (context)->anim_timer_id);
      CONTEXT_DATA (context)->anim_timer_id = 0;
    }

    {
      /* Reset the animation to the first frame.
	 Logic kludgily duplicated from thermo.c */
      int count = ((CONTEXT_DATA (context)->show_toolbar_p ? 1 : 0) +
		   (CONTEXT_DATA (context)->show_directory_buttons_p ? 1 : 0) +
		   (CONTEXT_DATA (context)->show_url_p ? 1 : 0));
      if (context->type == MWContextSaveToDisk) count = 2;
      fe_ResetLogo (context, (count > 1));
    }

  /* Kludge to go out of "cylon mode" when we actually reach the end. */
  if (CONTEXT_DATA (context)->thermo_size_unknown_count > 0)
    {
      int size_guess = (CONTEXT_DATA (context)->thermo_total >
			CONTEXT_DATA (context)->thermo_current
			? CONTEXT_DATA (context)->thermo_total
			: CONTEXT_DATA (context)->thermo_current);
      CONTEXT_DATA (context)->thermo_size_unknown_count = 0;
      CONTEXT_DATA (context)->thermo_total = size_guess;
      CONTEXT_DATA (context)->thermo_current = size_guess;
    }
  /* Get the 100% message. */
  fe_UpdateGraph (context, True);

  /* Now clear the thermometer while not disturbing the text. */
#ifdef LO_THERMO
  CONTEXT_DATA (context)->thermo_lo_percent = 0;
#else
  CONTEXT_DATA (context)->thermo_total = 0;
#endif
  fe_UpdateGraph (context, False);

  CONTEXT_DATA (context)->LED_on_p = False; /* force it off */
  fe_UpdateLED (context, False);

#ifdef DEBUG_THERMO
  fprintf (stderr, "FE_StopProgressGraph: %d of %d (%d)\n",
	   CONTEXT_DATA (context)->thermo_total,
	   CONTEXT_DATA (context)->thermo_current,
	   CONTEXT_DATA (context)->thermo_size_unknown_count);
#endif
}

/* Inform the user that a document is being transferred.
   URL is the url to which this report relates;
   bytes_received is how many bytes have been read;
   content_length is how large this document is (0 if unknown.)
   This is called from netlib as soon as we know the content length
    (or know that we don't know.)  It is called only once per document.
 */
void
XFE_GraphProgressInit (MWContext *context,
		      URL_Struct *url_struct,
		      int32 content_length)
{
#ifdef DEBUG_THERMO
  fprintf (stderr, "FE_GraphProgressInit(%ld)\n", content_length);
#endif

  if (CONTEXT_DATA (context)->thermo == 0) return;

  if (!CONTEXT_DATA (context)->thermo_timer_id)
    /* Hey!  How did that get turned off?  Turn it back on. */
    fe_StartProgressGraph (context);

  if (content_length == 0)
    CONTEXT_DATA (context)->thermo_size_unknown_count++;
  else
    CONTEXT_DATA (context)->thermo_total += content_length;
}

/* Remove --one-- transfer from the progress graph.
 */
void
XFE_GraphProgressDestroy (MWContext  *context, URL_Struct *url,
			 int32 content_length, int32 total_bytes_read)
{
#ifdef DEBUG_THERMO
  fprintf (stderr, "FE_GraphProgressDestroy(%ld, %ld)\n",
	   content_length, total_bytes_read);
#endif

  if (CONTEXT_DATA (context)->thermo == 0) return;

  if (content_length == 0)
    {
      /* Now that this transfer is done, we finally know how big it was.
	 This means that maybe we can go out of cylon mode and back into
	 thermometer mode. */
      CONTEXT_DATA (context)->thermo_size_unknown_count--;
      CONTEXT_DATA (context)->thermo_total += total_bytes_read;
    }
}


/* Inform the user of current progress, somehow.
   URL is the url to which this report relates;
   bytes_received is how many bytes have been read;
   content_length is how large this document is (0 if unknown.)
   This is called from netlib, and may be called very frequently.
 */
void
XFE_GraphProgress (MWContext *context,
		  URL_Struct *url_struct,
		  int32 bytes_received,
		  int32 bytes_since_last_time,
		  int32 content_length)
{
#ifdef DEBUG_THERMO
  fprintf (stderr, " FE_GraphProgress(%ld, +%ld, %ld)\n",
	   bytes_received, bytes_since_last_time, content_length);
#endif

/*  if (! CONTEXT_DATA (context)->show_status_p)
    return;*/

#ifdef GRIDS
  if (context->is_grid_cell)
  {
    MWContext *top_context = context;
    while ((top_context->grid_parent)&&(top_context->grid_parent->is_grid_cell))
      {
	top_context = top_context->grid_parent;
      }
    context = top_context->grid_parent;
  }
#endif /* GRIDS */

  if (CONTEXT_DATA (context)->thermo == 0) return;

  if (CONTEXT_DATA (context)->thermo_data_start_time <= 0)
    /* This is the first chunk of bits to arrive. */
    CONTEXT_DATA (context)->thermo_data_start_time = time ((time_t *) 0);

  CONTEXT_DATA (context)->thermo_current += bytes_since_last_time;
  fe_UpdateGraph (context, False);
}


/* Redraw the graph.  If text_too_p, then regenerate the textual message
   and/or move the cylon one tick if appropriate.  (We don't want to do
   this every time FE_GraphProgress() is called, just once a second or so.)
 */
void 
fe_UpdateGraph (MWContext *context, Boolean text_too_p)
{
  Widget thermo;
  Display *dpy;
  Window thermo_win;
  double ratio_done;
  Dimension w = 0, h = 0;
  int x;
  time_t now;
  int total_bytes, bytes_received;
  Boolean size_known_p;

  thermo = CONTEXT_DATA (context)->thermo;
  if (!thermo) return;

  dpy = XtDisplay (thermo);
  thermo_win = XtWindow (thermo);
  now = time ((time_t *) 0);
  total_bytes     = CONTEXT_DATA (context)->thermo_total;
  bytes_received  = CONTEXT_DATA (context)->thermo_current;
  size_known_p    = CONTEXT_DATA (context)->thermo_size_unknown_count <= 0;

/*  if (! CONTEXT_DATA (context)->show_status_p)
    return;*/

  if (size_known_p && total_bytes > 0 && bytes_received > total_bytes)
    {
#if 0
      /* Netlib doesn't take into account the size of the headers, so this
	 can actually go a bit over 100% (not by much, though.)  Prevent
	 the user from seeing this bug...   But DO print a warning if we're
	 way, way over the limit - more than 1k probably means the server
	 is messed up. */
      if (bytes_received > total_bytes + 1024)
	fprintf (stderr, "%s: received %d bytes but only expected %d??\n",
		 fe_progname, bytes_received, total_bytes);
#endif
      bytes_received = total_bytes;
    }

  if (! XtIsRealized (thermo))
    return;

#if 0
  XtVaGetValues (thermo, XtNwidth, &w, XtNheight, &h, 0);
#else
  w = thermo->core.width;
  h = thermo->core.height;
#endif
  if (w == 0 || h == 0) abort ();

  ratio_done = (size_known_p && total_bytes > 0
		? (((double) bytes_received) / ((double) total_bytes))
		: 0);

#ifdef DEBUG_THERMO
  fprintf (stderr, " fe_UpdateGraph: %d/%d=%f (%d/%d) %s  -or-  %d%%\n",
	   bytes_received, total_bytes,
	   ratio_done,
	   CONTEXT_DATA (context)->thermo_size_unknown_count,
	   CONTEXT_DATA (context)->active_url_count,
	   text_too_p ? "text" : "notext",
	   CONTEXT_DATA (context)->thermo_lo_percent);
#endif

  /* Update the thermo each time we're called. */

#ifdef LO_THERMO
  if (CONTEXT_DATA (context)->thermo_lo_percent >= 0)
#else
  if (size_known_p)
#endif
    {
#ifdef LO_THERMO
      x = (w * CONTEXT_DATA (context)->thermo_lo_percent) / 100;
#else
      x = w * ratio_done;
#endif

      /* The thermometer-like case - draw some mercury. */
      if (x != w)
	XClearArea (dpy, thermo_win, x, 0, w - x, h, False);
      if (x != 0)
	XFillRectangle (dpy, thermo_win, CONTEXT_DATA (context)->thermo_gc,
			0, 0, x, h);
    }
  else
    {
      /* If we don't know the length, do a random lightshow. */
      int sw, sw2;
      GC gc = CONTEXT_DATA (context)->thermo_gc;
      int down_p;

      sw = h * 3;
      sw2 = h;
      x = CONTEXT_DATA (context)->thermo_cylon;
      if ((down_p = (x < 0))) x = -x;
      if (x != 0)
	XClearArea (dpy, thermo_win, 0, 0, x, h, False);
      XFillRectangle (dpy, thermo_win, gc, x, 0, sw, h);
      if (w > (x + sw))
	XClearArea (dpy, thermo_win, x + sw, 0, w - (x + sw), h, False);

      if (text_too_p)  /* move the slider */
	{
	  CONTEXT_DATA (context)->thermo_cylon += sw2;
	  if (down_p)
	    {
	      if (CONTEXT_DATA (context)->thermo_cylon > 0)
		CONTEXT_DATA (context)->thermo_cylon = 0;
	    }
	  else
	    {
	      if (CONTEXT_DATA (context)->thermo_cylon > (w - (sw2 + sw)))
		CONTEXT_DATA (context)->thermo_cylon = -(w - sw);
	    }
	}
    }

  /* Only update text if a second or more has elapsed since last time.
     Unlike the cylon, which we update each time we are called with
     text_too_p == True (which is 4x a second or so.)
   */
  if (text_too_p && now >= (CONTEXT_DATA (context)->thermo_last_update_time +
			    CONTEXT_DATA (context)->progress_interval))
    {
      const char *msg = XP_ProgressText ((size_known_p ? total_bytes : 0),
					 bytes_received, 
				    CONTEXT_DATA (context)->thermo_start_time,
					 now);

      CONTEXT_DATA (context)->thermo_last_update_time = now;

      if (msg && *msg)
	FE_Progress (context, msg);

#ifdef DEBUG_THERMO
      fprintf (stderr, "====== %s\n", (msg ? msg : ""));
#endif

      /* if (msg) free (msg); */
    }
}

void 
fe_UpdateLED (MWContext *context, Boolean toggle_p)
{
  Widget LED = CONTEXT_DATA (context)->LED;
  Display *dpy;
  Window win;
  GC gc;
  if (!LED || !XtIsRealized (LED))
    return;
  dpy = XtDisplay (LED);
  win = XtWindow (LED);
  gc = CONTEXT_DATA (context)->LED_gc;
  if (toggle_p)
    CONTEXT_DATA (context)->LED_on_p = !CONTEXT_DATA (context)->LED_on_p;
  if (CONTEXT_DATA (context)->LED_on_p)
    XFillRectangle (dpy, win, gc, 0, 0, 9999, 9999);
  else
    XClearWindow (dpy, win);
}

#ifdef EDITOR
void fe_UpdateEditorBar (MWContext *context);
#endif /* EDITOR */

#ifdef HAVE_SECURITY
void 
fe_UpdateSecurityBar (MWContext *context)
{
/*  History_entry *h = SHIST_GetCurrent (&context->hist);*/
  Widget LED = CONTEXT_DATA (context)->security_bar;

  /* The colorbar
   */

#ifdef EDITOR
  if (context->is_editor) {
    fe_UpdateEditorBar (context);
    return;
  }
#endif /* EDITOR */

  if (LED && XtIsRealized (LED))
    {
      Display *dpy = XtDisplay (LED);
      Window win = XtWindow (LED);
      XGCValues gcv;
      GC gc;
      memset (&gcv, ~0, sizeof (gcv));
      gcv.foreground = XP_GetSecurityStatus(context)
	? CONTEXT_DATA (context)->secure_document_pixel
	: CONTEXT_DATA (context)->insecure_document_pixel;
      gc = fe_GetGC (LED, GCForeground, &gcv);
      XFillRectangle (dpy, win, gc, 0, 0, 9999, 9999);
    }

  /* The footer logo
   */
  if (CONTEXT_DATA (context)->security_logo)
    {
      Widget w = CONTEXT_DATA (context)->security_logo;
      Pixmap p1 = 0, p2 = 0;
      int state = XP_GetSecurityStatus(context);
      if (state == SSL_SECURITY_STATUS_OFF &&
	  (context->type == MWContextMail || context->type == MWContextNews))
	{
	  /* Check to see if this is an S/MIME message. */
	  XP_Bool encrypted = FALSE;
	  XP_Bool encrypted_ok = FALSE;
	  MIME_GetMessageCryptoState(context,
				     0, &encrypted,
				     0, &encrypted_ok);
	  if (encrypted && encrypted_ok)
	    state = SSL_SECURITY_STATUS_ON_LOW;	/* or HIGH? or what? */
	}

      XtVaGetValues (w, XmNlabelPixmap, &p1, 0);
      p2 = fe_SecurityPixmap (context, 0, 0, state);
      if (p1 != p2)
	XtVaSetValues (w, XmNlabelPixmap, p2, 0);
    }
}
#endif /* !HAVE_SECURITY */

void
XFE_SetProgressBarPercent (MWContext *context, int32 percent)
{
#ifdef DEBUG_THERMO
  fprintf (stderr, " FE_SetProgressBarPercent(%ld)\n", percent);
#endif

#ifdef EDITOR
  if (context->is_editor) {
    fe_UpdateEditorBar (context);
    /* the editor tells us 100% when it means 0 */
    if (percent == 100)
      percent = 0;
  }
#endif /* EDITOR */

  /* If thermo is not there, then we turn off both ICON animation and thermo */
  if (CONTEXT_DATA(context)->thermo == 0) return;

#ifdef GRIDS
  if (context->is_grid_cell)
  {
    MWContext *top_context = context;
    while ((top_context->grid_parent)&&(top_context->grid_parent->is_grid_cell))
      {
	top_context = top_context->grid_parent;
      }
    context = top_context->grid_parent;
  }
#endif /* GRIDS */

#ifndef LOU_OR_ERIC_REMOVED_EXPLICIT_CALL_TO_XFE_SETPROGRESSBAR_FROM_LAYOUT
  if (CONTEXT_DATA(context) == NULL)
	return; /* Called during layout for print, do nothing */
#endif
/*  if (! CONTEXT_DATA (context)->show_status_p)
    return;*/

  CONTEXT_DATA (context)->thermo_lo_percent = percent;
#ifdef LO_THERMO
  fe_UpdateGraph (context, False);
#endif
}

#ifdef EDITOR
void
fe_UpdateEditorBar (MWContext *context)
{
  Widget LED = CONTEXT_DATA (context)->security_bar;

  /* The colorbar
   */
  if (LED && XtIsRealized (LED))
    {
      Display *dpy = XtDisplay (LED);
      Window win = XtWindow (LED);
      XGCValues gcv;
      GC gc;
      memset (&gcv, ~0, sizeof (gcv));
      gcv.foreground = 0xbf;
      gc = fe_GetGC (LED, GCForeground, &gcv);
      XFillRectangle (dpy, win, gc, 0, 0, 9999, 9999);
    }
}

#endif /* EDITOR */
