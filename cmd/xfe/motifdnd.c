/* -*- Mode: C; tab-width: 8 -*-
   motifdnd.c --- facilities for using motif's drag and drop in 
                  mozilla.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 15-Jun-96.
 */


#include "prlog.h"
#include "mozilla.h"
#include "xfe.h"
#include "motifdnd.h"

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeBP.h>
#include <Xm/CascadeBGP.h>
#include <Xm/RowColumnP.h>
#include <Xm/AtomMgr.h>

#ifndef _STDC_
#define _STDC_ 1
#define HACKED_STDC 1
#endif

#ifdef HACKED_STDC
#undef HACKED_STDC
#undef _STDC_
#endif

Atom NS_DND_URL = 0;
Atom NS_DND_BOOKMARK = 0;
Atom NS_DND_ADDRESS = 0;
Atom NS_DND_HTMLTEXT = 0;
Atom NS_DND_FOLDER = 0;
Atom NS_DND_MESSAGE = 0;
Atom NS_DND_COLUMN = 0;
Atom NS_DND_COMPOUNDTEXT = 0;
Atom _SGI_ICON = 0;

static void dnd_location_drag_start(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void dnd_handle_browser_drop(Widget w, XtPointer client_data, XtPointer call_data);

static void dnd_browser_import_proc(Widget w, XtPointer closure, Atom *seltype, Atom *type, XtPointer value, unsigned long *length, int *format);

void
DND_initialize_atoms(Widget w)
{
  /* first we initialize all the atoms we need. */

  if (NS_DND_URL == 0) 
  {
      NS_DND_URL = XmInternAtom(XtDisplay(w),
				"NS_DND_URL",
				False);
      
      NS_DND_BOOKMARK = XmInternAtom(XtDisplay(w),
				     "NS_DND_BOOKMARK",
				     False);
      
      NS_DND_ADDRESS = XmInternAtom(XtDisplay(w),
				    "NS_DND_ADDRESS",
				    False);
      
      NS_DND_HTMLTEXT = XmInternAtom(XtDisplay(w),
				     "NS_DND_HTMLTEXT",
				     False);
      
      NS_DND_FOLDER = XmInternAtom(XtDisplay(w),
				   "NS_DND_FOLDER",
				   False);
      
      NS_DND_MESSAGE = XmInternAtom(XtDisplay(w),
				    "NS_DND_MESSAGE",
				    False);
      
      NS_DND_COLUMN = XmInternAtom(XtDisplay(w),
				   "NS_DND_COLUMN",
				   False);

      NS_DND_COMPOUNDTEXT = XmInternAtom(XtDisplay(w),
					 "COMPOUND_TEXT",
					 False);

      NS_DND_COMPOUNDTEXT = XmInternAtom(XtDisplay(w),
					 "_SGI_ICON",
					 False);
  }
}

void
DND_install_location_drag(Widget location_label)
{
  String location_drag_trans = "<Btn1Down>:  StartLocationDrag()";
  XtActionsRec actions[] = {
        {"StartLocationDrag", dnd_location_drag_start}
  };
  XtTranslations trans = XtParseTranslationTable(location_drag_trans);
  static Boolean actions_added = False;

  /* add the translations to the label. */
  XtOverrideTranslations(location_label,
			 trans);

  /* if it hasn't been already, add the appropriate action
     to the application context. */
  if (!actions_added)
  {
    XtAppAddActions(XtWidgetToApplicationContext(location_label),
		    actions, 
		    XtNumber(actions));

    actions_added = True;
  }

  XtFree((char*)trans);
}

void
DND_install_browser_drop(MWContext *context)
{
  Arg av[10];
  int ac;
  Atom import_targets[3];

  /* we only allow this on browser contexts */
  XP_ASSERT(context->type == MWContextBrowser);

  /* set our import list. */
  import_targets[0] = NS_DND_URL;
  import_targets[1] = _SGI_ICON;

  /* then set the resources for the drop site */
  ac = 0;
  XtSetArg(av[ac], XmNimportTargets, import_targets); ac++;
  XtSetArg(av[ac], XmNnumImportTargets, 2); ac++;
  XtSetArg(av[ac], XmNdropProc, dnd_handle_browser_drop); ac++;
  XtSetArg(av[ac], XmNdropSiteOperations, XmDROP_COPY); ac++;
  XtSetArg(av[ac], XmNdropSiteType, XmDROP_SITE_COMPOSITE); ac++;

  XmDropSiteRegister(CONTEXT_WIDGET(context), av, ac);
}

static void
dnd_handle_browser_drop(Widget w,
			XtPointer client_data,
			XtPointer call_data)
{
  XmDropProcCallback dropdata = (XmDropProcCallback)call_data;
  XmDropTransferEntryRec transfer_entries[1];
  Atom *export_list;
  int num_export_list, i;
  Arg av[10];
  int ac;
  int url_found = 0,
    sgi_icon_found = 0;

  ac = 0;

  if (dropdata->dropAction != XmDROP
      || dropdata->operation != XmDROP_COPY)
  {
    XtSetArg(av[ac], XmNtransferStatus, XmTRANSFER_FAILURE); ac++;
  }
  else
  {
    XtVaGetValues(dropdata->dragContext,
		  XmNexportTargets, &export_list,
		  XmNnumExportTargets, &num_export_list,
		  NULL);

    for (i=0; i < num_export_list; i++)
    {
      /* we want URL's first. */
      if (export_list[i] == NS_DND_URL)
      {
	url_found = 1;
	break;
      }
      /* and we want to support webjumpers. */
      else if (export_list[i] == _SGI_ICON) 
      {
	sgi_icon_found = 1;
	break;
      }
    }

    if (url_found
	|| sgi_icon_found)
    {
      if (url_found)
	transfer_entries[0].target = NS_DND_URL;
      else if (sgi_icon_found)
	transfer_entries[0].target = _SGI_ICON;

      transfer_entries[0].client_data = NULL; /* fill this in with something useful.  Ok? */

      XtSetArg(av[ac], XmNtransferProc, dnd_browser_import_proc); ac++;
      XtSetArg(av[ac], XmNdropTransfers, transfer_entries); ac++;
      XtSetArg(av[ac], XmNnumDropTransfers, 1); ac++;
    }
    else 
    {
      XtSetArg(av[ac], XmNtransferStatus, XmTRANSFER_FAILURE); ac++;
    }
  }

  XmDropTransferStart(dropdata->dragContext, av, ac);
}

static void 
dnd_location_drag_start(Widget w, 
			XEvent *event,
			String *params,
			Cardinal *num_params)
{
    int ac;
    Arg av[10];
    Atom export_targets[2];
    Widget dragContext;

    export_targets[0] = NS_DND_BOOKMARK;
    export_targets[1] = NS_DND_URL;

    ac = 0;
    XtSetArg(av[ac], XmNexportTargets, &export_targets); ac++;
    XtSetArg(av[ac], XmNnumExportTargets, 2); ac++;
    XtSetArg(av[ac], XmNdragOperations, XmDROP_COPY); ac++;

    dragContext = XmDragStart(w, event, av, ac);
}

static void 
dnd_browser_import_proc(Widget w, 
		       XtPointer closure, 
		       Atom *seltype, 
		       Atom *type, 
		       XtPointer value, 
		       unsigned long *length, 
		       int *format)
{
  if (*type == NS_DND_URL)
  {
  }
  else if (*type == _SGI_ICON)
  {
    /* first we check if it's a webjumper.
       webjumpers always have the string 'Type:WebJumpsite' 
       somewhere in the value. */
    char *string_value = (char*)value;

    if (strstr(string_value, "Type:WebJumpsite") == NULL)
    {
      XtVaSetValues(w, 
		    XmNtransferStatus, XmTRANSFER_FAILURE,
		    NULL);
    }
    else 
    {
      XP_Trace("You dropped a webjumper on me!\n");
    }
  }
}
