/* -*- Mode: C; tab-width: 8 -*-
   quickfile.c --- facilities for the quick file bookmark button/menu
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 10-Jun-96.
 */

#include "mozilla.h"
#include "xfe.h"
#include "quickfile.h"
#include "bkmks.h"
#include "felocale.h"

#include "xp_trace.h"

#include <X11/IntrinsicP.h>	/* for w->core.height */
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/MenuShell.h>
#include <Xm/MenuUtilP.h>
#include <Xm/RowColumn.h>
#include <XmL/Grid.h>

#ifndef _STDC_
#define _STDC_ 1
#define HACKED_STDC 1
#endif

#ifdef HACKED_STDC
#undef HACKED_STDC
#undef _STDC_
#endif

/* the event mask for the pointer grabs we do */
#define Events ((unsigned int) (ButtonPressMask | ButtonMotionMask | \
 			      ButtonReleaseMask | \
			      EnterWindowMask | LeaveWindowMask))

#include "icons.h"
#include "icondata.h"

#include <xpgetstr.h>
extern int XFE_BM_ALIAS;
extern int XFE_ESTIMATED_TIME_REMAINING;
extern int XFE_ESTIMATED_TIME_REMAINING_CHECKED;
extern int XFE_ESTIMATED_TIME_REMAINING_CHECKING;
extern int XFE_DONE_CHECKING_ETC;
extern int XFE_GG_EMPTY_LL;
extern int XFE_LOOK_FOR_DOCUMENTS_THAT_HAVE_CHANGED_ON;

extern MWContext* main_bm_context;

typedef struct qf_bookmark_struct qf_bookmark;
typedef struct qf_popup_struct qf_popup;

static void fe_qf_cell_draw_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void fe_qf_select_cell_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void fe_qf_activate_cell_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void fe_qf_initial_post_handler(Widget w, XtPointer client_data, XEvent *event, Boolean *cont);
static void fe_qf_button_release_handler(Widget w, XtPointer client_data, XEvent *event, Boolean *cont);
static qf_popup *fe_qf_popup_create(MWContext *context, Widget parent_shell, Arg *av, int ac);
static void fe_qf_unpost_submenus(qf_popup *popup);
static void fe_qf_unpost_all(qf_popup *popup);
static void fe_qf_unpost_submenu(qf_bookmark *bookmark);
static void fe_qf_post_submenu(qf_bookmark *bookmark);
static void fe_qf_generate_menu_hook(fe_QuickFile quickfile);
static void fe_qf_add_menu_items (qf_popup *popup, MWContext *bm_context, 
				  BM_Entry *entry);
static void fe_qf_add_child(qf_bookmark *bookmark, int position);

/* hooks used in traversing through the menus */
static void fe_qf_disarm_hook(qf_bookmark *bookmark);
static void fe_qf_arm_hook(qf_bookmark *bookmark);
static void fe_qf_activate_hook(qf_bookmark *bookmark);
static void fe_qf_fill_in_menu_hook(qf_bookmark *cascade);

/* The equivalent of the MenuShell/RowColumn combination in motif. */
struct qf_popup_struct {
  MWContext *context;    /* the context we're in               */
  Boolean popped_up;     /* whether we're popped up or not     */
  Widget shell;          /* our containing shell               */
  Widget rc;
  Widget grid;           /* the actual list of bookmarks       */
  qf_bookmark *bookmark; /* our parent in the cascade.  Useful
			    for unposting the entire cascade.  */
  
  qf_bookmark *active;   /* the currently active child. */
  int active_index;

  qf_bookmark **children;
  int num_children;
  int num_children_alloc;
};

/* The equivalent of the cascade button for the quickfile menu. */
struct qf_bookmark_struct {
  Position x;
  Position y;
  Position width;
  Position height;
  char *name;               		/* the string we display               */
  qf_popup *parent_pane;       		/* the pane we appear in.              */
  qf_popup *submenu;           		/* the pane we pop up.                 */
  BM_Entry *entry;             		/* the entry associated with a submenu */
  void (*activate_hook)(qf_bookmark *); /* callback of sorts.                  */
  void (*arm_hook)(qf_bookmark *); 	/* callback of sorts.                  */
  void (*disarm_hook)(qf_bookmark *); 	/* callback of sorts.                  */
  void (*cascade_hook)(qf_bookmark *);  /* callback of sorts.                  */
};

/* This is the fe_QuickFile structure, typedef'ed in quickfile.h */
struct fe_qf_struct {
  MWContext *context;               /* the context we're in */
  Widget button;                    /* the button that starts it all. */
  qf_popup *initial_popup;          /* our toplevel menu.             */
  Boolean menu_up_to_date_p;        /* whether or not we should recreate the toplevel 
				       menu */
  XtIntervalId popdown_timer;       /* used when dragging             */

  void (*popup_hook)(fe_QuickFile); /* callback of sorts.             */
};

static fe_icon pixmapFolderClosed = { 0 };
static fe_icon pixmapFolderOpen = { 0 };
static fe_icon pixmapBookmark = { 0 };

static void
PixmapDraw(Widget w, Pixmap pixmap, Pixmap mask,
	   int pixmapWidth, int pixmapHeight, unsigned char alignment, GC gc,
	   XRectangle *rect, XRectangle *clipRect)
{
  Display *dpy;
  Window win;
  int needsClip;
  int x, y, width, height;

  if (pixmap == XmUNSPECIFIED_PIXMAP) return;
  if (rect->width <= 4 || rect->height <= 4) return;
  if (clipRect->width < 3 || clipRect->height < 3) return;
  if (!XtIsRealized(w)) return;
  dpy = XtDisplay(w);
  win = XtWindow(w);

  width = pixmapWidth;
  height = pixmapHeight;
  if (!width || !height) {
    alignment = XmALIGNMENT_TOP_LEFT;
    width = clipRect->width - 4;
    height = clipRect->height - 4;
  }

  if (alignment == XmALIGNMENT_TOP_LEFT ||
      alignment == XmALIGNMENT_LEFT ||
      alignment == XmALIGNMENT_BOTTOM_LEFT) {
    x = rect->x + 2;
  } else if (alignment == XmALIGNMENT_TOP ||
	     alignment == XmALIGNMENT_CENTER ||
	     alignment == XmALIGNMENT_BOTTOM) {
    x = rect->x + ((int)rect->width - width) / 2;
  } else {
    x = rect->x + rect->width - width - 2;
  }

  if (alignment == XmALIGNMENT_TOP ||
      alignment == XmALIGNMENT_TOP_LEFT ||
      alignment == XmALIGNMENT_TOP_RIGHT) {
    y = rect->y + 2;
  } else if (alignment == XmALIGNMENT_LEFT ||
	     alignment == XmALIGNMENT_CENTER ||
	     alignment == XmALIGNMENT_RIGHT) {
    y = rect->y + ((int)rect->height - height) / 2;
  } else {
    y = rect->y + rect->height - height - 2;
  }

  needsClip = 1;
  if (clipRect->x <= x &&
      clipRect->y <= y &&
      clipRect->x + clipRect->width >= x + width &&
      clipRect->y + clipRect->height >= y + height) {
    needsClip = 0;
  }

  if (needsClip) {
    XSetClipRectangles(dpy, gc, 0, 0, clipRect, 1, Unsorted);
  } else if (mask) {
    XSetClipMask(dpy, gc, mask);
    XSetClipOrigin(dpy, gc, x, y);
  }
  XSetGraphicsExposures(dpy, gc, False);
  XCopyArea(dpy, pixmap, win, gc, 0, 0, width, height, x, y);
  XSetGraphicsExposures(dpy, gc, True);
  if (needsClip || mask) {
    XSetClipMask(dpy, gc, None);
  }
}

Widget 
fe_QuickFile_Button(fe_QuickFile quickfile)
{
  return quickfile->button;
}

fe_QuickFile
fe_QuickFile_Create(MWContext *context,
		    Widget parent)
{
  fe_QuickFile quickfile = (fe_QuickFile)XtMalloc(sizeof(struct fe_qf_struct));

  quickfile->context = context;
  quickfile->button = XtVaCreateManagedWidget("QFile",
					      xmLabelWidgetClass,
					      parent,
					      NULL);

  XtAddEventHandler(quickfile->button, 
		    ButtonPressMask, 
		    False, 
		    fe_qf_initial_post_handler, 
		    quickfile);

  quickfile->initial_popup = fe_qf_popup_create(context, CONTEXT_WIDGET(context), NULL, 0);

  quickfile->menu_up_to_date_p = False; /* so the hook will generate the
					   menu the first time through */

  quickfile->popup_hook = fe_qf_generate_menu_hook;

  /* now we generate the pixmaps */

  if (!pixmapFolderOpen.pixmap) 
  {
      Pixel bg_pixel;
      XtVaGetValues(quickfile->initial_popup->grid, XmNbackground, &bg_pixel, NULL);

      fe_MakeIcon(context, bg_pixel, &pixmapFolderClosed, NULL,
		  HHdr.width, HHdr.height,
		  HHdr.mono_bits, HHdr.color_bits, HHdr.mask_bits,
		  FALSE);		  

      fe_MakeIcon(context, bg_pixel, &pixmapFolderOpen, NULL,
		  HHdrO.width, HHdrO.height,
		  HHdrO.mono_bits, HHdrO.color_bits, HHdrO.mask_bits,
		  FALSE);		  

      fe_MakeIcon(context, bg_pixel, &pixmapBookmark, NULL,
		  HBkm.width, HBkm.height,
		  HBkm.mono_bits, HBkm.color_bits, HBkm.mask_bits, 
		  FALSE);
  }

  return quickfile;
}

void
fe_QuickFile_Destroy(fe_QuickFile quickfile)
{
  /* do nothing for now.. -- major memory leak. FIX ME */
}

void
fe_QuickFile_Invalidate(fe_QuickFile quickfile)
{
  if (quickfile)
    quickfile->menu_up_to_date_p = 0;
}

/* This functions pops down _all_ popups on the screen. */
static void
fe_qf_unpost_all(qf_popup *popup)
{
  /* first we find the root */

  while (popup->bookmark)
  {
    popup = popup->bookmark->parent_pane;
  }
  
  /* then we popdown all our children. */
  fe_qf_unpost_submenus(popup);

  /* then we unpost the root pane. */
  XtPopdown(popup->shell);
  /* and marked it as being down. */
  popup->popped_up = False;

  /* disable selection in the grid widget */
  XtCallActionProc(popup->grid, 
		   "MenuDisarm",
		   NULL, NULL, 0);
}

/* This function pops down all the menu panes in the
   cascade below -- but not including -- the one 
   passed as the parameter. */
static void
fe_qf_unpost_submenus(qf_popup *popup)
{
  qf_bookmark *active_bookmark = popup->active;

  XP_ASSERT(popup != NULL);

  if (active_bookmark)
  {
    fe_qf_unpost_submenu(active_bookmark);
    popup->active = NULL;
  }
}

static void
fe_qf_unpost_submenu(qf_bookmark *bookmark)
{
  /* if we don't have a submenu. */
  if (!bookmark->submenu)
    return;

  /* if it's not popped up. */
  if (!bookmark->submenu->popped_up)
    return;

  /* pop down the submenus of this one. */
  fe_qf_unpost_submenus(bookmark->submenu);

  /* disable selection in the grid widget */
#ifdef DEBUG_toshok
  XP_Trace("   disabling selection in the popped up grid\n");
#endif
  XtCallActionProc(bookmark->submenu->grid, 
		   "MenuDisarm",
		   NULL, NULL, 0);

  /* then pop down this menu */
  XtPopdown(bookmark->submenu->shell);
  bookmark->submenu->popped_up = False;
}

void
fe_qf_post_submenu(qf_bookmark *bookmark)
{
  XRectangle rect;
  Dimension st = 0;

  /* if we don't have a submenu. */
  if (!bookmark->submenu)
    return;

  /* if it's not popped up already */
  if (bookmark->submenu->popped_up)
    return;

  /* save off the cascade that popped us off, used in the popup_hook. */
  bookmark->submenu->bookmark = bookmark;

  /* call the popup's hook */
  if (bookmark->cascade_hook)
    (*bookmark->cascade_hook)(bookmark);

  /* move the shell to the right place - we subtract the shadow
     thickness so the highlighted rectangles for the folder and the 
     first item in the submenu line up... you know, it's the little
     things that make the world go 'round */
  XmLGridRowColumnToXY(bookmark->parent_pane->grid,
		       XmCONTENT, bookmark->parent_pane->active_index, 
		       XmCONTENT, 0,
		       False, &rect);

  XtVaGetValues(bookmark->submenu->grid,
		XmNshadowThickness, &st,
		NULL);

  XtMoveWidget(bookmark->submenu->shell,
	       bookmark->parent_pane->shell->core.x + bookmark->parent_pane->shell->core.width,
	       bookmark->parent_pane->shell->core.y + rect.y - st);

  /* pop up the shell */
  XtPopup(bookmark->submenu->shell, XtGrabNonexclusive);
  bookmark->submenu->popped_up = True;

#ifdef DEBUG_toshok
  XP_Trace("   Enabling traversal in the popped up grid\n");
#endif
  XtCallActionProc(bookmark->submenu->grid, 
		   "MenuArm",
		   NULL, NULL, 0);

}

/* This method gets called just before the initial pane is posted,
   giving us time to generate the information that will go in it. */
static void 
fe_qf_generate_menu_hook(fe_QuickFile quickfile)
{
  qf_popup *popup;
  MWContext *bm_context;
  BM_Entry *entry;

  XP_ASSERT(quickfile->initial_popup != NULL);

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_generate_menu_hook()\n");
#endif
  if (quickfile->menu_up_to_date_p)
    return;

  popup = quickfile->initial_popup;
  
  bm_context = main_bm_context; /* ### */
  
  if (popup == NULL || quickfile->menu_up_to_date_p)
    return;

  /* first we need to destroy all the children off the popup */
#ifdef DEBUG_toshok
  XP_Trace("  Destroying current children of popup\n");
#endif
  if (popup->num_children)
  {
    /*    fe_qf_PopupDestroyChildren(popup);*/
  }

  entry = BM_GetMenuHeader(bm_context);
  if (entry)
  {
      /* this is a bit of a hack, but we don't want to create
       * a submenu for the top header, "Joe's Bookmarks". instead
       * we jump to the children.
       */
      if (BM_IsHeader(entry)) entry = BM_GetChildren(entry);
#ifdef DEBUG_toshok
      XP_Trace("  Adding new menu entries\n");
#endif
      if (entry)
	  fe_qf_add_menu_items (popup, bm_context, entry);
  }

  /*  XtRealizeWidget (popup);*/

#ifdef DEBUG_toshok
  XP_Trace("  Setting up to date flag to true\n");
#endif
  quickfile->menu_up_to_date_p = True;
}


static void
fe_qf_fill_in_menu_hook(qf_bookmark *cascade)
{
  MWContext* bm_context = main_bm_context; /* ### */
  BM_Entry* entry = cascade->entry;

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_fill_in_menu_hook()\n");
#endif

  if (entry) 
  {
      XP_ASSERT(BM_IsHeader(entry));

      
      if (cascade->submenu)
      {
#ifdef DEBUG_toshok
	XP_Trace("  adding menu items to submenu\n");
#endif
	fe_qf_add_menu_items(cascade->submenu, bm_context,
			     BM_GetChildren(entry));
      }

#ifdef DEBUG_toshok
      XP_Trace("  setting entry to NULL\n");
#endif

      cascade->entry = NULL;
  }
}

static qf_popup *
fe_qf_popup_create(MWContext *context, 
			 Widget parent_shell,
			 Arg *av, int ac)
{
  qf_popup *new_popup = (qf_popup*)XtMalloc(sizeof(qf_popup));
  Arg *new_arglist;
  Arg av2[10];
  int ac2;
  Visual *v;
  Colormap cmap;
  Cardinal depth;

  new_popup->context = context;
  new_popup->popped_up = False;

  XtVaGetValues(CONTEXT_WIDGET(context),
		XmNdepth, &depth,
		XmNcolormap, &cmap,
		XmNvisual, &v,
		NULL);

  ac2 = 0;
  XtSetArg(av2[ac2], XmNdepth, depth); ac2++;
  XtSetArg(av2[ac2], XmNcolormap, cmap); ac2++;
  XtSetArg(av2[ac2], XmNvisual, v); ac2++;
  XtSetArg(av2[ac2], XmNwidth, 120); ac2++;
  XtSetArg(av2[ac2], XmNheight, 100); ac2++;
  XtSetArg(av2[ac2], XtNallowShellResize, True); ac2++;

  new_arglist = XtMergeArgLists(av, ac, av2, ac2);

  new_popup->shell = XtCreatePopupShell("quickfilePopup",
					xmMenuShellWidgetClass,
					parent_shell,
					new_arglist, ac + ac2);
  
  XtUninstallTranslations(new_popup->shell);

  new_popup->rc = XtVaCreateWidget("rc",
				   xmRowColumnWidgetClass,
				   new_popup->shell,
				   XmNmarginHeight, 0,
				   XmNmarginWidth, 0,
				   NULL);
  
  XtUnmanageChild(new_popup->rc);

  new_popup->grid = XtVaCreateManagedWidget("grid",
					    xmlGridWidgetClass,
					    new_popup->rc,
					    XmNuseTextWidget, False,
					    XmNrows, 0,
					    XmNcolumns, 2,
					    XmNsimpleWidths, "3c 30c",
					    XmNhorizontalSizePolicy, XmVARIABLE,
					    XmNverticalSizePolicy, XmVARIABLE,
					    XmNselectionPolicy, XmSELECT_BROWSE_ROW,
					    XmNautoSelect, False,
					    XmNshadowType, XmSHADOW_OUT,
					    NULL);

  /* now we set the cell defaults. */
  XtVaSetValues(new_popup->grid,
		XmNcellDefaults, True,
		XmNcellLeftBorderType, XmBORDER_NONE,
		XmNcellRightBorderType, XmBORDER_NONE,
		XmNcellTopBorderType, XmBORDER_NONE,
		XmNcellBottomBorderType, XmBORDER_NONE,
		XmNcellAlignment, XmALIGNMENT_LEFT,
		NULL);

  XtAddCallback(new_popup->grid, XmNcellDrawCallback, fe_qf_cell_draw_callback, new_popup);
  XtAddCallback(new_popup->grid, XmNselectCallback, fe_qf_select_cell_callback, new_popup);
  XtAddCallback(new_popup->grid, XmNactivateCallback, fe_qf_activate_cell_callback, new_popup);

  XtManageChild(new_popup->rc);

  new_popup->num_children = 0;
  new_popup->num_children_alloc = 4;

  new_popup->children = (qf_bookmark**)XtMalloc(sizeof(qf_bookmark*) 
						* new_popup->num_children_alloc);

  new_popup->bookmark = 
    new_popup->active = NULL;

  return new_popup;
}

static qf_bookmark *
fe_qf_BookmarkCreate(qf_popup *parent,
		     char *name,
		     qf_popup *submenu,
		     BM_Entry *entry)
{
  qf_bookmark *new_bookmark = (qf_bookmark*)XtMalloc(sizeof(qf_bookmark));

  new_bookmark->x =
    new_bookmark->y =
    new_bookmark->width = 
    new_bookmark->height = -1;

  new_bookmark->activate_hook =
    new_bookmark->cascade_hook =
    new_bookmark->arm_hook =
    new_bookmark->disarm_hook = NULL;

  new_bookmark->name = strdup(name);
  new_bookmark->parent_pane = parent;
  new_bookmark->entry = entry;

  new_bookmark->submenu = submenu;

  return new_bookmark;
}

static void 
fe_qf_add_child(qf_bookmark *bookmark, int position)
{
  qf_popup *popup = bookmark->parent_pane;
  int rows;

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_add_child()\n");
#endif

  popup->children[popup->num_children++] = bookmark;

  if (popup->num_children == popup->num_children_alloc)
  {
    popup->num_children_alloc *= 2;
    popup->children = (qf_bookmark**)XtRealloc((char*)popup->children,
					       popup->num_children_alloc 
					       * sizeof(qf_bookmark*));
  }

#ifdef DEBUG_toshok
  XP_Trace("  Adding a new row\n");
#endif
  XmLGridAddRows(bookmark->parent_pane->grid,
		 XmCONTENT, position, 1);

  if (position == -1)
    position = popup->num_children - 1;

#ifdef DEBUG_toshok
  XP_Trace("  Setting the string '%s' at the position %d\n", 
	   bookmark->name, 
	   position);
#endif
  XmLGridSetStringsPos(bookmark->parent_pane->grid,
		       XmCONTENT, position, XmCONTENT,
		       1, bookmark->name);

}

static void
fe_qf_button_release_handler(Widget w,
		     XtPointer client_data,
		     XEvent *event,
		     Boolean *cont)
{
  fe_QuickFile quickfile = (fe_QuickFile)client_data;
  
#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_button_release_handler\n");

  XP_Trace("  popping down entire quickfile menu\n");
#endif
  fe_qf_unpost_all(quickfile->initial_popup);

  /* ungrab the pointer. */
  XtUngrabPointer(w, CurrentTime);
}

static void 
fe_qf_initial_post_handler(Widget w, 
			   XtPointer client_data, 
			   XEvent *event, 
			   Boolean *cont)
{
  fe_QuickFile quickfile = (fe_QuickFile)client_data;
  Position x_root, y_root;
  String param[1];

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_initial_post_handler()\n");
#endif
  /* we only do stuff on button 1 press. */
  if (event->xbutton.button != 1)
    return;

  /* There should always be a popup. */
  XP_ASSERT(quickfile->initial_popup != NULL);

  /* It should never be popped up if we're here. */
  XP_ASSERT(quickfile->initial_popup->popped_up == False);

  /* call the quickfile button's initial hook */
#ifdef DEBUG_toshok
  XP_Trace("   Calling popup_hook\n");
#endif
  if (quickfile->popup_hook)
    (*quickfile->popup_hook)(quickfile);

#ifdef DEBUG_toshok
  XP_Trace("   Calling XtTranslateCoords()\n");
#endif
  XtTranslateCoords(quickfile->button, 0, 0, &x_root, &y_root);

#ifdef DEBUG_toshok
  XP_Trace("   Calling XtMoveWidget()\n");
#endif
  XtMoveWidget(quickfile->initial_popup->shell,
	       x_root + quickfile->button->core.width,
	       y_root);

#ifdef DEBUG_toshok
  XP_Trace("  adding button release event handler\n");
#endif
  XtAddEventHandler(quickfile->initial_popup->shell,
		    ButtonReleaseMask, False, fe_qf_button_release_handler, 
		    (XtPointer) quickfile);

#ifdef DEBUG_toshok
  XP_Trace("   Calling XtPopupSpringLoaded()\n");
#endif
  /*  XtPopup(quickfile->initial_popup->shell, XtGrabExclusive);*/
  XtPopupSpringLoaded(quickfile->initial_popup->shell);

  _XmGrabPointer(quickfile->initial_popup->grid, True, Events,
		 GrabModeSync, GrabModeAsync, None, 
		 XmGetMenuCursor(XtDisplay(quickfile->initial_popup->grid)), 
		 CurrentTime);

  XAllowEvents(XtDisplay(quickfile->initial_popup->grid), SyncPointer, CurrentTime);

#ifdef DEBUG_toshok
  XP_Trace("   Setting popped_up to true\n");
#endif
  quickfile->initial_popup->popped_up = True;

#ifdef DEBUG_toshok
  XP_Trace("   Enabling traversal in the popped up grid\n");
#endif
  XtCallActionProc(quickfile->initial_popup->grid, 
		   "MenuArm",
		   NULL, NULL, 0);
}

static void
fe_qf_add_menu_items (qf_popup *popup,
		      MWContext *bm_context, 
		      BM_Entry *entry)
{
  Widget button;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_add_menu_items()\n");
#endif

  XtVaGetValues (popup->shell, XtNvisual, &v,
		 XtNcolormap, &cmap, XtNdepth, &depth, 0);

  while (entry)
  {
    qf_bookmark *bookmark;

    char *bm_name = BM_GetName(entry);

    if (BM_IsHeader(entry))
    {
      qf_popup *submenu;

#ifdef DEBUG_toshok
      XP_Trace("  Adding new folder\n");
#endif

      ac = 0;
      if (BM_GetChildren(entry))
      {
	ac = 0;
	XtSetArg (av[ac], XmNvisual, v); ac++;
	XtSetArg (av[ac], XmNcolormap, cmap); ac++;
	XtSetArg (av[ac], XmNdepth, depth); ac++;

	submenu = fe_qf_popup_create(popup->context, popup->shell, av, ac);
      }
      else
      {
	  submenu = 0;
      }

      bookmark = fe_qf_BookmarkCreate(popup,
				      bm_name,
				      submenu,
				      entry);

      bookmark->arm_hook = fe_qf_arm_hook;
      bookmark->disarm_hook = fe_qf_disarm_hook;
      bookmark->activate_hook = fe_qf_activate_hook;
      bookmark->cascade_hook = fe_qf_fill_in_menu_hook;

      fe_qf_add_child(bookmark, -1);
    }
    else if (BM_IsSeparator(entry))
    {
#ifdef DEBUG_toshok
      XP_Trace("  Adding new separator\n");
#endif

      bookmark = fe_qf_BookmarkCreate(popup,
				      "-------------------------",
				      NULL,
				      NULL);

      fe_qf_add_child(bookmark, -1);      
    }
    else
    {

#ifdef DEBUG_toshok
      XP_Trace("  Adding new bookmark\n");
#endif

      bookmark = fe_qf_BookmarkCreate(popup,
				      bm_name,
				      NULL,
				      entry);

      bookmark->arm_hook = fe_qf_arm_hook;
      bookmark->disarm_hook = fe_qf_disarm_hook;
      bookmark->activate_hook = fe_qf_activate_hook;

      fe_qf_add_child(bookmark, -1);

    }
    
    entry = BM_GetNext(entry);
  }
}

static void
fe_qf_activate_hook (qf_bookmark *bookmark)
{
  MWContext *context = bookmark->parent_pane->context;
  BM_Entry *entry = bookmark->entry;
  URL_Struct *url;

  /* pop everything down */
  fe_qf_unpost_all(bookmark->parent_pane);

  if (bookmark->submenu == NULL)  
  {
      fe_UserActivity(context);
      if (!entry || !(BM_IsAlias(entry) || BM_IsUrl(entry)))
	  return;
      url = NET_CreateURLStruct (BM_GetAddress(entry), FALSE);
      /* item->last_visit = time ((time_t *) 0); ### */
      /* fe_BookmarkAccessTimeChanged (context, item); ### */
      fe_GetURL (context, url, FALSE);
  }
  else 
  {
      /* do special stuff if we're in a cascade button -- tear off thingy */
  }

}

static void
fe_qf_arm_hook(qf_bookmark *bookmark)
{
  MWContext *context = bookmark->parent_pane->context;
  BM_Entry *entry = bookmark->entry;

  fe_UserActivity(context);

  /* if we're a folder, we post our submenu when we're armed. */
  fe_qf_post_submenu(bookmark);
  
  if (!entry || !(BM_IsAlias(entry) || BM_IsUrl(entry)))
    return;

  XFE_Progress(context, BM_GetAddress(entry));
}

static void
fe_qf_disarm_hook(qf_bookmark *bookmark)
{
  MWContext *context = bookmark->parent_pane->context;

  /* if we're a folder, we unpost our submenu when we're disarmed. */
  fe_qf_unpost_submenu(bookmark);

  XFE_Progress(context, "");
}


static void 
fe_qf_cell_draw_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  qf_popup *popup = (qf_popup*)client_data;
  XmLGridCallbackStruct* call = (XmLGridCallbackStruct*) call_data;
  XmLGridDrawStruct *draw = call->drawInfo;
  Boolean drawSelected = draw->drawSelected;
  qf_bookmark *bookmark = popup->children[call->row];
  fe_icon* data = NULL;
  XRectangle r;

  if (call->column == 0) /* here we're drawing the icon. */
  {
      if (bookmark->submenu)
      {
	if (drawSelected)
	  data = &pixmapFolderOpen;
	else
	  data = &pixmapFolderClosed;
      }
      else
      {
	data = &pixmapBookmark;
      }

      r = *draw->cellRect;
      r.width = data->width;
      PixmapDraw(w, data->pixmap, data->mask, data->width, data->height,
		 XmALIGNMENT_LEFT, draw->gc, &r, call->clipRect);
  }
}

static void 
fe_qf_activate_cell_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  qf_popup *popup = (qf_popup*)client_data;
  XmLGridCallbackStruct *cbs = (XmLGridCallbackStruct*)call_data;

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_activate_cell_callback()\n");
#endif

  popup->active = popup->children[cbs->row];
  popup->active_index = cbs->row;

  /*  and call it's arm hook if it has one. */
#ifdef DEBUG_toshok
  XP_Trace("  calling activate_hook.\n");
#endif
  if (popup->active
      && popup->active->activate_hook)
    popup->active->activate_hook(popup->active);
}

static void 
fe_qf_select_cell_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  qf_popup *popup = (qf_popup*)client_data;
  XmLGridCallbackStruct *cbs = (XmLGridCallbackStruct*)call_data;

#ifdef DEBUG_toshok
  XP_Trace("Inside fe_qf_select_cell_callback()\n");
#endif

  /* first we disarm the currently active bookmark, if there is one. */
#ifdef DEBUG_toshok
  XP_Trace("  calling disarm_hook.\n");
#endif
  if (popup->active
      && popup->active->disarm_hook)
    popup->active->disarm_hook(popup->active);
  
  /* then switch to the new item. */
  popup->active = popup->children[cbs->row];
  popup->active_index = cbs->row;

  /*  and call it's arm hook if it has one. */
#ifdef DEBUG_toshok
  XP_Trace("  calling arm_hook\n");
#endif
  if (popup->active
      && popup->active->arm_hook)
    popup->active->arm_hook(popup->active);
}
