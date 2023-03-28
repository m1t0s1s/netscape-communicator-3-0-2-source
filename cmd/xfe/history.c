/* -*- Mode: C; tab-width: 8 -*-
   history.c --- UI routines called by the history module.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Jun-94.
 */

#include "mozilla.h"
#include "xfe.h"
#include "felocale.h"

/* Enable the "Back" button and menu item.
 */
int
FE_EnableBackButton (MWContext *context)
{
  Widget w = CONTEXT_DATA (context)->back_button;
  if (CONTEXT_DATA (context)->show_toolbar_p && w)
    XtVaSetValues (w, XmNsensitive, True, 0);
  w = CONTEXT_DATA (context)->back_menuitem;
  if (w)
    XtVaSetValues (w, XmNsensitive, True, 0);
  return 0;
}

/* Disable the "Back" button and menu item.
 */
int
FE_DisableBackButton (MWContext *context)
{
  Widget w = CONTEXT_DATA (context)->back_button;
  if (CONTEXT_DATA (context)->show_toolbar_p && w)
    XtVaSetValues (w, XmNsensitive, False, 0);
  w = CONTEXT_DATA (context)->back_menuitem;
  if (w)
    XtVaSetValues (w, XmNsensitive, False, 0);
  return 0;
}

/* Enable the "Forward" button and menu item.
 */
int
FE_EnableForwardButton (MWContext *context)
{
  Widget w = CONTEXT_DATA (context)->forward_button;
  if (CONTEXT_DATA (context)->show_toolbar_p && w)
    XtVaSetValues (w, XmNsensitive, True, 0);
  w = CONTEXT_DATA (context)->forward_menuitem;
  if (w)
    XtVaSetValues (w, XmNsensitive, True, 0);
  return 0;
}

/* Disable the "Forward" button and menu item.
 */
int
FE_DisableForwardButton (MWContext *context)
{
  Widget w = CONTEXT_DATA (context)->forward_button;
  if (CONTEXT_DATA (context)->show_toolbar_p && w)
    XtVaSetValues (w, XmNsensitive, False, 0);
  w = CONTEXT_DATA (context)->forward_menuitem;
  if (w)
    XtVaSetValues (w, XmNsensitive, False, 0);
  return 0;
}



/* History popup
 */

static void
fe_update_history_time (MWContext *context, History_entry *entry,
			XP_List *hist)
{
  entry->last_access = time ((time_t *) 0);
  /* Flush this time into any bookmark entries that have the same URL. */
  if (hist)
    while ((hist = hist->next))
      {
	BookmarkStruct *h = hist->object;
	if (h->type == HOT_URLType &&
	    h->address && entry->address &&
	    !strcmp (h->address, entry->address))
	  {
	    h->last_visit = entry->last_access;
	    fe_BookmarkAccessTimeChanged (context, h);
	  }
	else if (h->type == HOT_HeaderType && h->children)
	  {
	    fe_update_history_time (context, entry, h->children);
	  }
      }
}

static void
fe_history_menu_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  int index = -1;
  History_entry *h;
  URL_Struct *url;
  XtVaGetValues (widget, XmNuserData, &index, 0);
  if (index < 0) return;
  h = SHIST_GetObjectNum (&context->hist, index);
  if (! h) return;
  url = SHIST_CreateURLStructFromHistoryEntry (context, h);
  fe_update_history_time (context, h, HOT_GetBookmarkList ());
  fe_GetURL (context, url, FALSE);
}

static void
fe_fill_history_selector (MWContext *context)
{
  XP_List *history = SHIST_GetList (context);
  XP_List *h;
  History_entry *current;
  History_entry *e;
  int i;
  int cpos = 0;
  int length = 0;
  int menu_length;
  XmString *items = 0;
  Dimension width = 0, height = 0;
  Widget *kids = 0;
  Cardinal nkids = 0;
  Arg av [20];
  int ac;
  Widget menu;
#if defined(EDITOR) && defined(EDITOR_UI)
  int fast_and_loose = 7;  /* Gold has browse publish location + separator */
#else
  int fast_and_loose = 5;  /* this is, uh, the top N items not to fuck with. */
#endif
  int max_length = 30;	   /* spurious statistics up 15% this year. */

  if (! history) return;

  length = XP_ListCount (history);

  history = history->next;	/* How stupid!!! */
  current = SHIST_GetCurrent (&context->hist);
  menu = CONTEXT_DATA (context)->history_menu;

  if (!menu) return;

  /* Kill off the old history stuff in the menu: skip past the first
     two items if they exist, otherwise create them.
   */
  XtVaGetValues (menu, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
  XtUnrealizeWidget (menu);
/*  if (nkids > fast_and_loose)
    XtUnmanageChildren (kids + fast_and_loose, nkids - fast_and_loose);*/
  if (nkids > fast_and_loose) {
    fe_DestroyWidgetTree(&kids[fast_and_loose], nkids-fast_and_loose);
    nkids = fast_and_loose;
  }

  if (nkids < fast_and_loose)
    abort ();

  if (length > max_length)
    menu_length = max_length;
  else
    menu_length = length;

  if (CONTEXT_DATA (context)->history_dialog)
    items = (XmString *) malloc (sizeof (XmString) * length);
  else
    items = 0;

  if (menu_length)
    while (nkids > 0)
      /* Force fucking Motif to recompute the width of the menu!! */
      XtVaSetValues (kids[--nkids], XmNwidth, 10, 0);

  if (menu_length)
    kids = malloc (sizeof (Widget) * menu_length);
  else
    kids = 0;

  h = history;
  /* i is the history index and starts at 1.
     pos is the index in the menu, with 0 being the first (topmost) item. */
  for (i = length; i > 0; i--)
    {
      int pos = length - i;
/*      int pos = i - 1 - (length - menu_length);*/
      XmString label;
      char *title, *url;
      char name [1024];
      e = SHIST_GetObjectNum (&context->hist, i);
      if (! e) abort ();
      title = e->title;
      url = e->address;

      if (title && !strcmp (title, "Untitled"))
	title = 0;   /* You losers, don't do that to me! */

      /* The dialog box */
      if (items)
	{
	  PR_snprintf (name, sizeof (name), "%-35.35s  %.900s",
		   (title && *title ? title : "Untitled"),
		   (url ? url : "???"));
	  items [length - i] = fe_ConvertToXmString (context->win_csid,
						     (unsigned char *) name);
	}

      /* The pulldown menu */
      if (pos == menu_length - 1 && menu_length < length)
	{
	  kids [pos] = XmCreateLabelGadget (menu, "historyTruncated", 0, 0);
	}
      else if (pos < menu_length)
	{
	  char buf [255];

	  fe_FormatDocTitle (title, url, name, 1024);
	  fe_MidTruncateString (name, name, 35);

	  label = fe_ConvertToXmString (context->win_csid,
					(unsigned char *) name);
	  ac = 0;
	  XtSetArg (av [ac], XmNlabelString, label); ac++;
	  XtSetArg (av [ac], XmNuserData, i); ac++;
	  PR_snprintf (buf, sizeof (buf), "historyItem%d", pos + 1);
	  if (e == current)
	    {
	      XtSetArg (av [ac], XmNset, True); ac++;
	      XtSetArg (av [ac], XmNvisibleWhenOff, False); ac++;
	      XtSetArg (av [ac], XmNindicatorType, XmN_OF_MANY); ac++;
	      kids[pos] = XmCreateToggleButtonGadget(menu, buf, av, ac);
	      XtAddCallback (kids[pos], XmNvalueChangedCallback,
			     fe_history_menu_cb, context);
	    }
	  else
	    {
	      kids[pos] = XmCreatePushButtonGadget (menu, buf, av, ac);
	      XtAddCallback (kids[pos], XmNactivateCallback,fe_history_menu_cb,
			     context);
	    }
	  XmStringFree (label);
	}

      if (e == current)
	cpos = length - i + 1;
      h = h->next;
    }

  if (menu_length)
    XtManageChildren (kids, menu_length);
  if (kids) free (kids);

  if (CONTEXT_DATA (context)->history_dialog)
    {
      XtVaGetValues (CONTEXT_DATA (context)->history_dialog,
		     XmNwidth, &width, XmNheight, &height, 0);
      XtVaSetValues (CONTEXT_DATA (context)->history_dialog,
		     XmNlistItemCount, length,
		     XmNlistItems, items,
		     XmNwidth, width,
		     XmNheight, height,
		     0);

      /* Update the current item. */
      if (cpos)
	{
	  Widget list = XtNameToWidget (CONTEXT_DATA (context)->history_dialog,
					"*ItemsList");
	  int top_pos, vis;
	  XmListSelectPos (list, cpos, False);
	  /* Scroll the list if necessary. */
	  XtVaGetValues (list,
			 XmNtopItemPosition, &top_pos,
			 XmNvisibleItemCount, &vis, 0);
	  if (cpos >= top_pos + vis)
	    XmListSetBottomPos (list, cpos);
	}

      for (i = 0; i < length; i++)
	XmStringFree (items [i]);
      free (items);
    }

  XtRealizeWidget (menu);
}

void
fe_HistoryItemAction (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  int n;
  char dummy;

  if (*ac < 1)
    fprintf (stderr, "%s: too few args (%d) to HistoryItem()\n",
	     fe_progname, *ac);
  else if (*ac > 2)
    fprintf (stderr, "%s: too many args (%d) to HistoryItem()\n",
	     fe_progname, *ac);
  else if (1 == sscanf (av[0], " %d %c", &n, &dummy))
    {
      Boolean done = False;
      MWContext *context = fe_WidgetToMWContext (widget);
      Widget menu;
      Widget *kids = 0;
      Cardinal nkids = 0;
      char buf [255];
      int i;
      Boolean is_remote = False;
      XP_ASSERT (context);
      if (!context) return;
      context = fe_GridToTopContext (context);

      /* Check if this command is invoked from -remote*/
      if ( !strcmp(av[*ac-1], "remote" ) )
      {
	is_remote = True;
      } 

      /* History Action can come from remote therefore browser is required.  
         Need to find the Browser context in order for history to work 
         If there is a browser, then apply the history action */
       if ( context->type != MWContextBrowser )
           context = XP_FindContextOfType(context,MWContextBrowser );
       
       if (!context) return;

      menu = CONTEXT_DATA (context)->history_menu;
      fe_UserActivity (context);
      if (!menu) return;



      PR_snprintf (buf, sizeof (buf), "historyItem%d", n);
      XtVaGetValues (menu, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
      for (i = 0; i < nkids; i++)
	if (!strcmp (buf, XtName (kids [i])))
	  {
	    XtCallCallbacks (kids [i],
			     (XmIsToggleButtonGadget (kids [i])
			      ? XmNvalueChangedCallback
			      : XmNactivateCallback),
			     0);
	    done = True;
	    break;
	  }
      if (! done)
	XBell (XtDisplay (widget), 0);

     /* If the command is from "-remote", make the browser window
        to be at the front */
     if ( is_remote )
     {
      XMapRaised(XtDisplay(CONTEXT_WIDGET(context)),
                   XtWindow(CONTEXT_WIDGET(context)));
     }
    }
  else
    fprintf (stderr, "%s: unknown parameter (%s) to HistoryItem()\n",
	     fe_progname, av[0]);
}


static void
fe_history_confirm_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XmSelectionBoxCallbackStruct *cb = (XmSelectionBoxCallbackStruct *)call_data;
  History_entry *h = 0;
  int count;
  int lower_p = 0;
  int hot_p = 0;
  XtVaGetValues (CONTEXT_DATA (context)->history_dialog,
		 XmNlistItemCount, &count, 0);
  switch (cb->reason)
    {
    case XmCR_APPLY:
      hot_p = 1;
      /* fall through */
    case XmCR_OK:
      {
	XP_List *history = (SHIST_GetList (context))->next; /* How stupid!! */
	int *positions = 0;
	int npositions = 0;
	int pos = 0;
	int count = 0;
	Boolean free_p = False;
	Widget list = XtNameToWidget (CONTEXT_DATA (context)->history_dialog,
				      "*ItemsList");
	int i;

	if (! list) abort ();

	XtVaGetValues (list, XmNitemCount, &count, 0);
	if (count <= 0)
  	  break;
	free_p = XmListGetSelectedPos (list, &positions, &npositions);
	if (npositions > 0)
	  pos = *positions;
	if (free_p && positions)
	  free (positions);

	if (pos <= 0)
	  break;
	for (i = count; (i > pos && history); i--)
	  history = history->next;
	if (! history)
	  break;
	h = history->object;
	break;
      }
    case XmCR_CANCEL:
      lower_p = 1;
      break;
    default:
      abort ();
      break;
    }

  if (lower_p)
    XtUnmanageChild (CONTEXT_DATA (context)->history_dialog);

  if (h)
    {
      URL_Struct *url = SHIST_CreateURLStructFromHistoryEntry (context, h);
      if (! url) return;
      if (hot_p)
	{
	  fe_AddToBookmark (context, h->title, url, h->last_access);
	}
      else
	{
	  fe_update_history_time (context, h, HOT_GetBookmarkList ());
	  fe_GetURL (context, url, FALSE);
	}
    }
}


static void
fe_make_history_dialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Arg av [20];
  int ac;
  Widget shell;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  shell = XmCreateSelectionDialog (mainw, "history", av, ac);

  XtVaSetValues (XtParent (shell), XmNallowShellResize, False, 0);

  CONTEXT_DATA (context)->history_dialog = shell;

  XtAddCallback (shell, XmNokCallback,     fe_history_confirm_cb, context);
  XtAddCallback (shell, XmNcancelCallback, fe_history_confirm_cb, context);
  XtAddCallback (shell, XmNapplyCallback,  fe_history_confirm_cb, context);

  XtUnmanageChild (XmSelectionBoxGetChild (shell, XmDIALOG_SEPARATOR));
  XtUnmanageChild (XmSelectionBoxGetChild (shell, XmDIALOG_TEXT));
  XtUnmanageChild (XmSelectionBoxGetChild (shell, XmDIALOG_SELECTION_LABEL));
  XtUnmanageChild (XmSelectionBoxGetChild (shell, XmDIALOG_LIST_LABEL));
#ifdef NO_HELP
  XtUnmanageChild (XmSelectionBoxGetChild (shell, XmDIALOG_HELP_BUTTON));
#endif

  fe_NukeBackingStore (shell);
  XtManageChild (shell);
}


void
fe_HistoryDialog (MWContext *context)
{
  if (!CONTEXT_DATA (context)->history_dialog)
    fe_make_history_dialog (context);
  fe_fill_history_selector (context);
  XtManageChild (CONTEXT_DATA (context)->history_dialog);
}


/* If url is 0, don't actually add anything to the history, but do
   update the history window if necessary.
 */
void
fe_AddHistory (MWContext *context, URL_Struct *url, const char *doc_title)
{
  if (! doc_title) doc_title = "";
  if (url)
    SHIST_AddDocument (context,
		       SHIST_CreateHistoryEntry (url, (char *) doc_title));
  fe_fill_history_selector (context);
}

void
fe_RegenerateHistoryMenu (MWContext *context)
{
  fe_fill_history_selector (context);
}
