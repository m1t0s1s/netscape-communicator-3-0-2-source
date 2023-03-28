/* -*- Mode: C; tab-width: 8 -*-
   outline.c --- the outline widget hack.
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 24-Jun-95.
 */

#include "mozilla.h"
#include "xfe.h"
#include <msgcom.h>

#include "outline.h"
#include "dragdrop.h"

#include <X11/IntrinsicP.h>	/* for w->core.height */

#ifndef _STDC_
#define _STDC_ 1
#define HACKED_STDC 1
#endif

#include <XmL/Grid.h>
#include <Xm/Label.h>		/* For the drag window pixmap label */

#ifdef HACKED_STDC
#undef HACKED_STDC
#undef _STDC_
#endif

#include "icons.h"
#include "icondata.h"

#ifdef FREEIF
#undef FREEIF
#endif
#define FREEIF(obj) do { if (obj) { XP_FREE (obj); obj = 0; }} while (0)

#define PIXMAP_WIDTH 18		/* #### */
#define PIXMAP_INDENT 10

#define FLIPPY_WIDTH 14		/* #### */

#define MIN_COLUMN_WIDTH 10	/* So that the user can manipulate it. */
#define MAX_COLUMN_WIDTH 10000	/* So that we don't overflow any 16-bit
				   numbers. */


static fe_OutlineDesc Data;
static int DataRow = -1;
static Widget DataWidget = NULL;

typedef struct fe_OutlineInfo {
  Widget widget;
  Widget drag_widget;
  int numcolumns;
  int numrows;
  fe_OutlineGetDataFunc datafunc;
  fe_OutlineClickFunc clickfunc;
  fe_OutlineIconClickFunc iconfunc;
  void* closure;
  fe_dnd_Type dragtype;
  int lastx;
  int lasty;
  Time lastdowntime;
  Time lastuptime;
  XP_Bool ignoreevents;
  EventMask activity;
  void* dragscrolltimer;
  int dragscrolldir;
  int dragrow;
  XP_Bool dragrowbox;
  GC draggc;
  XP_Bool allowiconresize;
  int iconcolumnwidth;
  int* columnwidths;
  int* columnIndex;		/* Which column to display where.  If
				   columnIndex[1] == 3, then display the third
				   column's data in column 1.  Column zero is
				   the icon column.*/
  char** posinfo;		/* Pointer to a string where we keep the
				   current column ordering and widths, in the
				   form to be saved in the preferences file.*/
  int dragcolumn;
  unsigned char clickrowtype;
  char** headers;		/* What headers to display on the columns.
				   Entry zero is for the icon column. */
  XP_Bool* headerhighlight;

  void* visibleTimer;
  int visibleLine;
} fe_OutlineInfo;


static fe_icon pixmapFlippyFolded = { 0 };
static fe_icon pixmapFlippyExpanded = { 0 };

static fe_icon pixmapFolderIcon = { 0 };
static fe_icon pixmapNewsgroupIcon = { 0 };
static fe_icon pixmapNewshostIcon = { 0 };
static fe_icon pixmapMessageIcon = { 0 };
static fe_icon pixmapArticleIcon = { 0 };

static fe_icon pixmapMarked = { 0 };
static fe_icon pixmapUnmarked = { 0 };
static fe_icon pixmapRead = { 0 };
static fe_icon pixmapUnread = { 0 };
static fe_icon pixmapSubscribed = { 0 };
static fe_icon pixmapUnsubscribed = { 0 };

static fe_icon pixmapBookmark = { 0 };
static fe_icon pixmapChangedBookmark = { 0 };
static fe_icon pixmapUnknownBookmark = { 0 };
static fe_icon pixmapClosedHeader = { 0 };
static fe_icon pixmapOpenedHeader = { 0 };
static fe_icon pixmapClosedHeaderDest = { 0 };
static fe_icon pixmapOpenedHeaderDest = { 0 };
static fe_icon pixmapClosedHeaderMenu = { 0 };
static fe_icon pixmapOpenedHeaderMenu = { 0 };
static fe_icon pixmapClosedHeaderMenuDest = { 0 };
static fe_icon pixmapOpenedHeaderMenuDest = { 0 };

static fe_icon pixmapAddress = { 0 };
static fe_icon pixmapClosedList = { 0 };
static fe_icon pixmapOpenedList = { 0 };


static fe_dnd_Source dragsource = {0};


static fe_icon* fe_outline_lookup_icon(fe_OutlineType icon)
{
  switch (icon) {
  case FE_OUTLINE_Marked:		return &pixmapMarked;
  case FE_OUTLINE_Unmarked:		return &pixmapUnmarked;
  case FE_OUTLINE_Read:			return &pixmapRead;
  case FE_OUTLINE_Unread:		return &pixmapUnread;
  case FE_OUTLINE_Subscribed:		return &pixmapSubscribed;
  case FE_OUTLINE_Unsubscribed:		return &pixmapUnsubscribed;
  case FE_OUTLINE_Article:		return &pixmapArticleIcon;
  case FE_OUTLINE_MailMessage:		return &pixmapMessageIcon;
  case FE_OUTLINE_Folder:		return &pixmapFolderIcon;
  case FE_OUTLINE_Newsgroup:		return &pixmapNewsgroupIcon;
  case FE_OUTLINE_Newshost:		return &pixmapNewshostIcon;
  case FE_OUTLINE_Bookmark:		return &pixmapBookmark;
  case FE_OUTLINE_ChangedBookmark:	return &pixmapChangedBookmark;
  case FE_OUTLINE_UnknownBookmark:	return &pixmapUnknownBookmark;
  case FE_OUTLINE_ClosedHeader:		return &pixmapClosedHeader;
  case FE_OUTLINE_OpenedHeader:		return &pixmapOpenedHeader;
  case FE_OUTLINE_ClosedHeaderDest:	return &pixmapClosedHeaderDest;
  case FE_OUTLINE_OpenedHeaderDest:	return &pixmapOpenedHeaderDest;
  case FE_OUTLINE_ClosedHeaderMenu:	return &pixmapClosedHeaderMenu;
  case FE_OUTLINE_OpenedHeaderMenu:	return &pixmapOpenedHeaderMenu;
  case FE_OUTLINE_ClosedHeaderMenuDest:	return &pixmapClosedHeaderMenuDest;
  case FE_OUTLINE_OpenedHeaderMenuDest:	return &pixmapOpenedHeaderMenuDest;
  case FE_OUTLINE_Address:		return &pixmapAddress;
  case FE_OUTLINE_ClosedList:		return &pixmapClosedList;
  case FE_OUTLINE_OpenedList:		return &pixmapOpenedList;
  default: return NULL;
  }
}

static void
fe_make_outline_drag_widget (fe_OutlineInfo *info,
			     fe_OutlineDesc *data, int x, int y)
{
#if 0
  Display *dpy = XtDisplay (info->widget);
  XmString str;
  int str_w, str_h;
#endif
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget label;
  XmFontList fontList;
  fe_icon *icon;
  Widget shell;

  if (dragsource.widget) return;

  shell = info->widget;
  while (XtParent(shell) && !XtIsShell(shell)) {
    shell = XtParent(shell);
  }

  XtVaGetValues (shell, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);

  XtVaGetValues (info->widget, XmNfontList, &fontList, NULL);

/* ### free the string
  str = XmStringCreateLocalized(data->str[5]);
  str_w = XmStringWidth (fontList, str);
  str_h = XmStringHeight (fontList, str);
*/

  dragsource.type = info->clickrowtype == XmCONTENT ? info->dragtype :
    FE_DND_COLUMN;
  dragsource.closure = info->widget;

  if (dragsource.type == FE_DND_COLUMN) {
    icon = &pixmapArticleIcon;	/* ### Wrongwrongwrongwrongwrong */
  } else {

    icon = fe_outline_lookup_icon(data->icon);
    XP_ASSERT(icon != NULL);
    if (icon == NULL) return;
  }

  dragsource.widget = XtVaCreateWidget ("drag_win",
					overrideShellWidgetClass,
					info->widget,
					XmNwidth, icon->width,
					XmNheight, icon->height,
					XmNvisual, v,
					XmNcolormap, cmap,
					XmNdepth, depth,
					XmNborderWidth, 0,
					NULL);

  label = XtVaCreateManagedWidget ("label",
				   xmLabelWidgetClass,
				   dragsource.widget,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, icon->pixmap,
				   NULL);

  /* XmStringFree(str); */
}


static void
fe_destroy_outline_drag_widget (void)
{
  if (!dragsource.widget) return;
  XtDestroyWidget (dragsource.widget);
  dragsource.widget = NULL;
}

static fe_OutlineInfo*
fe_get_info(Widget outline)
{
  fe_OutlineInfo* result = NULL;
  XtVaGetValues(outline, XmNuserData, &result, 0);
  assert(result->widget == outline);
  return result;
}


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




extern XmString fe_StringChopCreate(char* message, char* tag,
				    XmFontList font_list, int maxwidth);

static void
fe_outline_celldraw(Widget widget, XtPointer clientData, XtPointer callData)
{
  fe_OutlineInfo* info = (fe_OutlineInfo*) clientData;
  XmLGridCallbackStruct* call = (XmLGridCallbackStruct*) callData;
  XmLGridDrawStruct *draw = call->drawInfo;
  XmLCGridRow *row;
  XmLCGridColumn *column;
  unsigned char alignment;
  Pixel fg;
  XmFontList fontList;
  XmString str = NULL;
  const char* ptr;
  fe_icon* data = NULL;
  XRectangle r;
  int sourcecol;
  if (call->rowType != XmCONTENT) return;
  row = XmLGridGetRow(widget, call->rowType, call->row);
  column = XmLGridGetColumn(widget, call->columnType, call->column);
  if (DataWidget != widget || call->row != DataRow) {
    if (!(*info->datafunc)(widget, info->closure, call->row, &Data)) {
      DataRow = -1;
      return;
    }
    DataWidget = widget;
    DataRow = call->row;
  }
  sourcecol = info->columnIndex[call->column];
  if (sourcecol == 0) {
    /* Draw the flippy, if any. */
    if (Data.flippy != MSG_Leaf) {
      switch (Data.flippy) {
      case MSG_Folded:		data = &pixmapFlippyFolded; break;
      case MSG_Expanded:	data = &pixmapFlippyExpanded; break;
      default:
	XP_ASSERT(0);
      }
      r = *draw->cellRect;
      r.width = data->width;
      PixmapDraw(widget, data->pixmap, data->mask, data->width, data->height,
		 XmALIGNMENT_LEFT, draw->gc, &r, call->clipRect);
    }

    /* Draw the indented icon */
    r = *draw->cellRect;
    data = fe_outline_lookup_icon(Data.icon);
    XP_ASSERT(data);
    if (!data) return;

    r.x += Data.depth * PIXMAP_INDENT + FLIPPY_WIDTH;
    r.width = data->width;
    if (r.x + r.width > draw->cellRect->x + draw->cellRect->width) {
      char buf[10];
      XRectangle rect;
      rect = *draw->cellRect;
      rect.width -= r.width;
      PR_snprintf(buf, sizeof (buf), "%d", Data.depth);
      XtVaGetValues(widget,
		    XmNrowPtr, row,
		    XmNcolumnPtr, column,
		    XmNcellForeground, &fg,
		    XmNcellFontList, &fontList,
		    NULL);
      str = XmStringCreate(buf, XmFONTLIST_DEFAULT_TAG);
      XSetForeground(XtDisplay(widget), draw->gc,
		     draw->isSelected ? draw->selectForeground : fg);
      XmLStringDraw(widget, str, draw->stringDirection, fontList,
		    XmALIGNMENT_RIGHT, draw->gc, &rect, &rect);
      r.x = draw->cellRect->x + draw->cellRect->width - r.width;
    }
    PixmapDraw(widget, data->pixmap, data->mask, data->width, data->height,
	       XmALIGNMENT_LEFT, draw->gc, &r, call->clipRect);
  } else {
    ptr = Data.str[sourcecol - 1];
    if (Data.type[sourcecol - 1] == FE_OUTLINE_String ||
	Data.type[sourcecol - 1] == FE_OUTLINE_ChoppedString) {
      XtVaGetValues(widget,
		    XmNrowPtr, row,
		    XmNcolumnPtr, column,
		    XmNcellForeground, &fg,
		    XmNcellAlignment, &alignment,
		    XmNcellFontList, &fontList,
		    NULL);
      if (call->clipRect->width > 4) {
	/* Impose some spacing between columns.  What a hack. ### */
	call->clipRect->width -= 4;
	if (Data.type[sourcecol - 1] == FE_OUTLINE_ChoppedString) {
	  str = fe_StringChopCreate((char*) ptr, (char*) Data.tag, fontList,
				    call->clipRect->width);
	} else {
	  str = XmStringCreate((char *) ptr, (char*) Data.tag);
	}
	XSetForeground(XtDisplay(widget), draw->gc,
		     draw->isSelected ? draw->selectForeground : fg);
	XmLStringDraw(widget, str, draw->stringDirection, fontList, alignment,
		      draw->gc, draw->cellRect, call->clipRect);
	call->clipRect->width += 4;
      }
    } else {
      switch (Data.type[sourcecol - 1]) {
      case FE_OUTLINE_Marked:		data = &pixmapMarked; break;
      case FE_OUTLINE_Unmarked:		data = &pixmapUnmarked; break;
      case FE_OUTLINE_Read:		data = &pixmapRead; break;
      case FE_OUTLINE_Unread:		data = &pixmapUnread; break;
      case FE_OUTLINE_Subscribed:	data = &pixmapSubscribed; break;
      case FE_OUTLINE_Unsubscribed:	data = &pixmapUnsubscribed; break;
      default: XP_ASSERT(0); return;
      }
      PixmapDraw(widget, data->pixmap, data->mask, data->width, data->height,
		 XmALIGNMENT_LEFT, draw->gc, draw->cellRect,
		 call->clipRect);
    }
  }
  if (call->row == info->dragrow && sourcecol > 0) {
    int y;
    int x2 = draw->cellRect->x + draw->cellRect->width - 1;
    XP_Bool rightedge = FALSE;
    if (call->column == info->numcolumns) {
      rightedge = TRUE;
      if (str) {
	int xx = draw->cellRect->x + XmStringWidth(fontList, str) + 4;
	if (x2 > xx) x2 = xx;
      }
    }
    if (info->draggc == NULL) {
      XGCValues gcv;
#if 0
      Pixel foreground;
#endif
      gcv.foreground = fg;
      info->draggc = XCreateGC(XtDisplay(widget), XtWindow(widget),
			       GCForeground, &gcv);
    }
    y = draw->cellRect->y + draw->cellRect->height - 1;
    XDrawLine(XtDisplay(widget), XtWindow(widget), info->draggc,
	      draw->cellRect->x, y, x2, y);
    if (info->dragrowbox) {
      int y0 = draw->cellRect->y;
      if (call->column == 1) {
	XDrawLine(XtDisplay(widget), XtWindow(widget), info->draggc,
		  draw->cellRect->x, y0, draw->cellRect->x, y);
      }
      if (rightedge) {
	XDrawLine(XtDisplay(widget), XtWindow(widget), info->draggc,
		  x2, y0, x2, y);
      }	
      XDrawLine(XtDisplay(widget), XtWindow(widget), info->draggc,
		draw->cellRect->x, y0, x2, y0);

    }
  }
  if (str) XmStringFree(str);
}


#if 0
static void
fe_outline_click(Widget widget, XtPointer clientData, XtPointer callData)
{
  fe_OutlineInfo* info = (fe_OutlineInfo*) clientData;
  XmLGridCallbackStruct* call = (XmLGridCallbackStruct*) callData;
  XEvent* event;
  unsigned int state;
  if (call->column < info->numiconcolumns) {
    if (DataWidget != widget || call->row != DataRow) {
      if (!(*info->datafunc)(widget, info->closure, call->row, &Data)) {
	return;
      }
      DataWidget = widget;
      DataRow = call->row;
    }
    if (call->column == Data.depth) {
      if (call->reason != XmCR_ACTIVATE) {
	(*info->iconfunc)(widget, info->closure, call->row);
      }
      return;
    }
  }

  event = call->event;
  if (!event)
    state = 0;
  else if (event->type == ButtonPress || event->type == ButtonRelease)
    state = event->xbutton.state;
  else if (event->type == KeyPress || event->type == KeyRelease)
    state = event->xkey.state;
  else
    state = 0;

  (*info->clickfunc)(widget, info->closure, call->row,
		     call->column - info->numiconcolumns,
		     1, call->reason == XmCR_ACTIVATE ? 2 : 1,
		     (state & ShiftMask) != 0,
		     (state & ControlMask) != 0);
}
#endif /* 0 */

static int last_motion_x = 0;
static int last_motion_y = 0;

static void
UpdateData (Widget widget, fe_OutlineInfo *info, int row)
{
  if (widget != DataWidget || row != DataRow) {
    if (!(*info->datafunc)(widget, info->closure, row, &Data)) {
      return;
    }
    DataWidget = widget;
    DataRow = row;
  }
}

static XP_Bool
RowIsSelected(fe_OutlineInfo* info, int row)
{
  int* position;
  int n = XmLGridGetSelectedRowCount(info->widget);
  XP_Bool result = FALSE;
  int i;
  if (n > 0) {
    position = XP_ALLOC(sizeof(int) * n);
    if (position) {
      XmLGridGetSelectedRows(info->widget, position, n);
      for (i=0 ; i<n ; i++) {
	if (position[i] == row) {
	  result = TRUE;
	  break;
	}
      }
      XP_FREE(position);
    }
  }
  return result;
}


static void
fe_outline_visible_timer(void* closure)
{
  fe_OutlineInfo* info = (fe_OutlineInfo*) closure;
  info->visibleTimer = NULL;
  if (info->visibleLine >= 0) {
    /* First and check that the user isn't still busy with his mouse.  If
       he is, then we'll do this stuff when activity goes to 0. */
    if (info->activity == 0) {
      fe_OutlineMakeVisible(info->widget, info->visibleLine);
      info->visibleLine = -1;
    }
  }
}


static void
SendClick(fe_OutlineInfo* info, XEvent* event, XP_Bool only_if_not_selected)
{
  int x = event->xbutton.x;
  int y = event->xbutton.y;
  int numclicks = 1;
  unsigned char rowtype;
  unsigned char coltype;
  int row;
  int column;
  unsigned int state = 0;

  if (!only_if_not_selected &&
      abs(x - info->lastx) < 5 && abs(y - info->lasty) < 5 &&
      (info->lastdowntime - info->lastuptime <=
       XtGetMultiClickTime(XtDisplay(info->widget)))) {
    numclicks = 2;		/* ### Allow triple clicks? */
  }
  info->lastx = x;
  info->lasty = y;

  if (XmLGridXYToRowColumn(info->widget, x, y,
			   &rowtype, &row, &coltype, &column) >= 0) {
    if (rowtype == XmHEADING) row = -1;
    if (coltype == XmCONTENT) {
      info->activity = ButtonPressMask;
      info->ignoreevents = True;
      if (column < 1 && !only_if_not_selected && row >= 0) {
	UpdateData(info->widget, info, row);
	if (1 /*### Pixel compare the click with where we drew icon*/) {
	  if (numclicks == 1) {
	    (*info->iconfunc)(info->widget, info->closure, row);
	  }
	}
	return;
      }
      state = event->xbutton.state;
      if (!only_if_not_selected || !RowIsSelected(info, row)) {
	int sourcecol = info->columnIndex[column];
	if (numclicks == 1) {
	  /* The user just clicked.  If he's in the middle of a double-click,
	     then we don't want calls to fe_OutlineMakeVisible to cause things
	     to scroll before the double-click finishes.  So, we set a
	     timer to go off; if fe_OutlineMakeVisible gets called before the
	     timer expires, we put off the call until the timer goes off. */
	  if (info->visibleTimer) {
	    FE_ClearTimeout(info->visibleTimer);
	  }
	  info->visibleTimer =
	    FE_SetTimeout(fe_outline_visible_timer, info,
			  XtGetMultiClickTime(XtDisplay(info->widget)) + 10);
	}	   
	(*info->clickfunc)(info->widget, info->closure, row, sourcecol - 1,
			   info->headers ? info->headers[sourcecol] : NULL,
			   event->xbutton.button, numclicks,
			   (state & ShiftMask) != 0,
			   (state & ControlMask) != 0);

      }
    }
  }
}



static void
ButtonEvent(Widget widget, XtPointer closure, XEvent* event, Boolean* c)
{
  fe_OutlineInfo* info = (fe_OutlineInfo*) closure;
  int x = event->xbutton.x;
  int y = event->xbutton.y;
  unsigned char rowtype;
  unsigned char coltype;
  int row;
  int column;

  info->ignoreevents = False;

  switch (event->type) {
  case ButtonPress:
    /* Always ignore btn3. Btn3 is for popups. - dp */
    if (event->xbutton.button == 3) break;

#ifdef DEBUG_jwz
    /* ignore all buttons except button1 -- e.g., 4 and 5 are mouse wheel */
    if (event->xbutton.button != 1) break;
#endif /* DEBUG_jwz */

    if (XmLGridXYToRowColumn(info->widget, x, y,
			     &rowtype, &row, &coltype, &column) >= 0) {
      if (rowtype == XmHEADING) {
	if (XmLGridPosIsResize(info->widget, x, y)) {
	  return;
	}
      }
      info->clickrowtype = rowtype;
      info->dragcolumn = column;
      info->activity |= ButtonPressMask;
      info->ignoreevents = True;
    }
    last_motion_x = x;
    last_motion_y = y;
    info->lastdowntime = event->xbutton.time;
    break;

  case ButtonRelease:

    /* fe_SetCursor (info->context, False); */

    if (info->activity & ButtonPressMask) {
      if (info->activity & PointerMotionMask) {
	/* handle the drop */

	fe_dnd_DoDrag(&dragsource, event, FE_DND_DROP);
	fe_dnd_DoDrag(&dragsource, event, FE_DND_END);

	fe_destroy_outline_drag_widget();

      } else {
	SendClick(info, event, FALSE);
      }
    }
    info->lastuptime = event->xbutton.time;
    info->activity = 0;
    if (info->visibleLine >= 0 && info->visibleTimer == NULL) {
      fe_OutlineMakeVisible(info->widget, info->visibleLine);
      info->visibleLine = -1;
    }

    break;

  case MotionNotify:
      if (!(info->activity & PointerMotionMask) &&
	  (abs(x - last_motion_x) < 5 && abs(y - last_motion_y) < 5)) {
	/* We aren't yet dragging, and the mouse hasn't moved enough for
	   this to be considered a drag. */
	break;
      }


      if (info->activity & ButtonPressMask) {
        /* ok, the pointer moved while a button was held.
         * we're gonna drag some stuff.
         */
	info->ignoreevents = True;
        if (!(info->activity & PointerMotionMask)) {
	  if (info->dragtype == FE_DND_NONE &&
	      info->clickrowtype == XmCONTENT) {
	    /* We don't do drag'n'drop in this context.  Do any any visibility
	       scrolling that we may have been putting off til the end of user
	       activity. */
	    info->activity = 0;
	    if (info->visibleLine >= 0 && info->visibleTimer == NULL) {
	      fe_OutlineMakeVisible(info->widget, info->visibleLine);
	      info->visibleLine = -1;
	    }
	    break;
	  }

	  /* First time.  If the item we're pointing at isn't
	     selected, deliver a click message for it (which ought to
	     select it.) */
	  
	  if (info->clickrowtype == XmCONTENT) {
	    /* Hack things so we click where the mouse down was, not where
	       the first notify event was.  Bleah. */
	    event->xbutton.x = last_motion_x;
	    event->xbutton.y = last_motion_y;
	    SendClick(info, event, TRUE);
	    event->xbutton.x = x;
	    event->xbutton.y = y;
	  }

	  /* Create a drag source. */
          fe_make_outline_drag_widget (info, &Data, x, y);
	  fe_dnd_DoDrag(&dragsource, event, FE_DND_START);
          info->activity |= PointerMotionMask;
        }
	
	fe_dnd_DoDrag(&dragsource, event, FE_DND_DRAG);

	/* Now, force all the additional mouse motion events that are
	   lingering around on the server to come to us, so that Xt can
	   compress them away.  Yes, XSync really does improve performance
	   in this case, not hurt it. */
	XSync(XtDisplay(info->widget), False);

      }
      last_motion_x = x;
      last_motion_y = y;
      break;
  }
  if (info->ignoreevents) *c = False;    
}



static void
fe_set_default_column_widths(Widget widget) {
  fe_OutlineInfo* info = fe_get_info(widget);
  int i;
  short avgwidth, avgheight;
  XmFontList fontList;
  XtVaGetValues(widget, XmNfontList, &fontList, NULL);
  XmLFontListGetDimensions(fontList, &avgwidth, &avgheight, TRUE);
  info->columnIndex[0] = 0;
  for (i=0 ; i<info->numcolumns ; i++) {
    int width = info->columnwidths[i] * avgwidth;
    if (width < MIN_COLUMN_WIDTH) width = MIN_COLUMN_WIDTH;
    if (width > MAX_COLUMN_WIDTH) width = MAX_COLUMN_WIDTH;
    info->columnIndex[i + 1] = i + 1;
    XtVaSetValues(widget,
		  XmNcolumn, i + 1,
		  XmNcolumnSizePolicy, XmCONSTANT,
		  XmNcolumnWidth, width,
		  0);
  }
}

Widget
fe_OutlineCreate(MWContext* context, Widget parent, String name,
		 ArgList av, Cardinal ac,
		 int maxindentdepth,
		 int numcolumns, int* columnwidths,
		 fe_OutlineGetDataFunc datafunc,
		 fe_OutlineClickFunc clickfunc,
		 fe_OutlineIconClickFunc iconfunc,
		 void* closure, fe_dnd_Type dragtype,
		 char** posinfo)
{
  Widget result;
  fe_OutlineInfo* info;

  XP_ASSERT(numcolumns >= 0);

  info = XP_NEW_ZAP(fe_OutlineInfo);
  if (info == NULL) return NULL; /* ### */

  XtSetArg(av[ac], XmNuserData, info); ac++;

  info->numcolumns = numcolumns;

  info->datafunc = datafunc;
  info->clickfunc = clickfunc;
  info->iconfunc = iconfunc;
  info->closure = closure;
  info->dragtype = dragtype;
  info->dragrow = -1;
  info->posinfo = posinfo;
  info->visibleLine = -1;
  DataRow = -1;

  info->lastx = -999;
  info->columnwidths = columnwidths;

  columnwidths[numcolumns - 1] = 9999; /* Make the last column always really
					  wide, so we always are ready to
					  show something no matter how wide
					  the window gets. */

  XtSetArg(av[ac], XmNselectionPolicy, XmSELECT_NONE); ac++;
  if ((context->type == MWContextMail &&
       (fe_globalPrefs.mail_pane_style == FE_PANES_HORIZONTAL ||
	fe_globalPrefs.mail_pane_style == FE_PANES_TALL_FOLDERS)) ||
      ((context->type == MWContextNews &&
	(fe_globalPrefs.news_pane_style == FE_PANES_HORIZONTAL ||
	 fe_globalPrefs.news_pane_style == FE_PANES_TALL_FOLDERS)))) {
      XtSetArg(av[ac], XmNhorizontalSizePolicy, XmCONSTANT); ac++;
  }
  else {
      XtSetArg(av[ac], XmNhorizontalSizePolicy, XmVARIABLE); ac++;
  }

  /* ### Need to add deletion callback to clean up info rec! */

  result = XmLCreateGrid(parent, name, av, ac);
  info->widget = result;

  XtVaSetValues(result,
		XmNcellDefaults, True,
		XmNcellLeftBorderType, XmBORDER_NONE,
		XmNcellRightBorderType, XmBORDER_NONE,
		XmNcellTopBorderType, XmBORDER_NONE,
		XmNcellBottomBorderType, XmBORDER_NONE,
		XmNcellAlignment, XmALIGNMENT_LEFT,
		0);

  XmLGridAddColumns(result, XmCONTENT, -1, info->numcolumns + 1);

  XtVaSetValues(result, XmNcolumn, 0,
		XmNcolumnSizePolicy, XmCONSTANT,
		XmNcolumnWidth,
		(maxindentdepth - 1) * PIXMAP_INDENT + PIXMAP_WIDTH,
		0);

  info->allowiconresize = (maxindentdepth > 0);

  info->columnIndex = (int*) XP_ALLOC(sizeof(int) * (info->numcolumns + 1));
  info->columnIndex[0] = 0;

  fe_set_default_column_widths(result);

  XtInsertEventHandler(result, ButtonPressMask | ButtonReleaseMask
		       | PointerMotionMask, False,
		       ButtonEvent, info, XtListHead);

  XtAddCallback(result, XmNcellDrawCallback, fe_outline_celldraw, info);
/*   XtAddCallback(result, XmNselectCallback, fe_outline_click, info); */
/*   XtAddCallback(result, XmNactivateCallback, fe_outline_click, info); */

  if (!pixmapMessageIcon.pixmap) {
    Pixel pixel;
    XtVaGetValues(result, XmNbackground, &pixel, NULL);

    fe_MakeIcon(context, pixel, &pixmapMessageIcon, NULL,
		HMsg.width, HMsg.height,
		HMsg.mono_bits, HMsg.color_bits, HMsg.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapArticleIcon, NULL,
		HArt.width, HArt.height,
		HArt.mono_bits, HArt.color_bits, HArt.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapFlippyFolded, NULL,
		HFolder.width, HFolder.height,
		HFolder.mono_bits, HFolder.color_bits, HFolder.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapFlippyExpanded, NULL,
		HFolderO.width, HFolderO.height,
		HFolderO.mono_bits, HFolderO.color_bits, HFolderO.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapNewsgroupIcon, NULL,
		HGroup.width, HGroup.height,
		HGroup.mono_bits, HGroup.color_bits, HGroup.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapNewshostIcon, NULL,
		HHost.width, HHost.height,
		HHost.mono_bits, HHost.color_bits, HHost.mask_bits, FALSE);

    fe_MakeIcon(context, pixel, &pixmapMarked, NULL,
		HMarked.width, HMarked.height,
		HMarked.mono_bits, HMarked.color_bits, HMarked.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapUnmarked, NULL,
		HUMarked.width, HUMarked.height,
		HUMarked.mono_bits, HUMarked.color_bits, HUMarked.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapRead, NULL,
		HRead.width, HRead.height,
		HRead.mono_bits, HRead.color_bits, HRead.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapUnread, NULL,
		HURead.width, HURead.height,
		HURead.mono_bits, HURead.color_bits, HURead.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapSubscribed, NULL,
		HSub.width, HSub.height,
		HSub.mono_bits, HSub.color_bits, HSub.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapUnsubscribed, NULL,
		HUSub.width, HUSub.height,
		HUSub.mono_bits, HUSub.color_bits, HUSub.mask_bits, FALSE);

    fe_MakeIcon(context, pixel, &pixmapBookmark, NULL,
		HBkm.width, HBkm.height,
		HBkm.mono_bits, HBkm.color_bits, HBkm.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapChangedBookmark, NULL,
		HBkmCh.width, HBkmCh.height,
		HBkmCh.mono_bits, HBkmCh.color_bits, HBkmCh.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapUnknownBookmark, NULL,
		HBkmUnknown.width, HBkmUnknown.height,
		HBkmUnknown.mono_bits, HBkmUnknown.color_bits,
		HBkmUnknown.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapClosedHeader, NULL,
		HHdr.width, HHdr.height,
		HHdr.mono_bits, HHdr.color_bits, HHdr.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapOpenedHeader, NULL,
		HHdrO.width, HHdrO.height,
		HHdrO.mono_bits, HHdrO.color_bits, HHdrO.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapClosedHeaderDest, NULL,
		HHdrDest.width, HHdrDest.height,
		HHdrDest.mono_bits, HHdrDest.color_bits, HHdrDest.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapOpenedHeaderDest, NULL,
		HHdrDestO.width, HHdrDestO.height,
		HHdrDestO.mono_bits, HHdrDestO.color_bits, HHdrDestO.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapClosedHeaderMenu, NULL,
		HHdrMenu.width, HHdrMenu.height,
		HHdrMenu.mono_bits, HHdrMenu.color_bits, HHdrMenu.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapOpenedHeaderMenu, NULL,
		HHdrMenuO.width, HHdrMenuO.height,
		HHdrMenuO.mono_bits, HHdrMenuO.color_bits, HHdrMenuO.mask_bits,
		FALSE);
    fe_MakeIcon(context, pixel, &pixmapClosedHeaderMenuDest, NULL,
		HHdrMenuDest.width, HHdrMenuDest.height,
		HHdrMenuDest.mono_bits, HHdrMenuDest.color_bits,
		HHdrMenuDest.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapOpenedHeaderMenuDest, NULL,
		HHdrMenuDestO.width, HHdrMenuDestO.height,
		HHdrMenuDestO.mono_bits, HHdrMenuDestO.color_bits,
		HHdrMenuDestO.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapAddress, NULL,
		HAddr.width, HAddr.height,
		HAddr.mono_bits, HAddr.color_bits, HAddr.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapClosedList, NULL,
		HList.width, HList.height,
		HList.mono_bits, HList.color_bits, HList.mask_bits, FALSE);
    fe_MakeIcon(context, pixel, &pixmapOpenedList, NULL,
		HListO.width, HListO.height,
		HListO.mono_bits, HListO.color_bits, HListO.mask_bits, FALSE);


    pixmapNewsgroupIcon = pixmapArticleIcon;
    pixmapFolderIcon = pixmapMessageIcon;
    
  }

  return result;
}



static void
fe_outline_remember_columns(Widget widget)
{
  fe_OutlineInfo* info = fe_get_info(widget);
  XmLCGridColumn* col;
  int i;
  Dimension width;
  char* ptr;
  int length = 0;
  FREEIF(*(info->posinfo));
  for (i=0 ; i <= info->numcolumns ; i++) {
    length += strlen(info->headers[i]) + 14;
  }
  *(info->posinfo) = XP_ALLOC(length);
  ptr = *(info->posinfo);
  for (i=0 ; i<=info->numcolumns ; i++) {
    col = XmLGridGetColumn(widget, XmCONTENT, i);
    XtVaGetValues(widget, XmNcolumnPtr, col, XmNcolumnWidth, &width, 0);
    PR_snprintf(ptr, *(info->posinfo) + length - ptr,
		"%s:%d;", info->headers[info->columnIndex[i]],
		(int) width);
    ptr += strlen(ptr);
  }
  if (ptr > *(info->posinfo)) ptr[-1] = '\0';
}


static void
fe_outline_resize(Widget widget, XtPointer clientData, XtPointer callData)
{
  /* The user has resized a column.  Unfortunately, if they resize it
     to width 0, they will never be able to grab it to resize it
     bigger.  So, we will implement a minimum width per column. */

  fe_OutlineInfo* info = (fe_OutlineInfo*) clientData;
  XmLGridCallbackStruct* call = (XmLGridCallbackStruct*) callData;
  XmLCGridColumn* col;
  Dimension width;
  if (call->reason != XmCR_RESIZE_COLUMN) return;
  if (!info->allowiconresize && call->column == 0) {
    XtVaSetValues(widget, XmNcolumn, call->column, XmNcolumnWidth,
		  info->iconcolumnwidth, 0);
  } else {
    col = XmLGridGetColumn(widget, call->columnType, call->column);
    XtVaGetValues(widget, XmNcolumnPtr, col, XmNcolumnWidth, &width, 0);
    if (width < MIN_COLUMN_WIDTH) {
      XtVaSetValues(widget, XmNcolumn, call->column,
		    XmNcolumnWidth, MIN_COLUMN_WIDTH, 0);
    }
  }
  fe_outline_remember_columns(widget);
}

void
fe_OutlineSetMaxDepth(Widget outline, int maxdepth)
{
  fe_OutlineInfo* info = fe_get_info(outline);
  int value = (maxdepth - 1) * PIXMAP_INDENT + PIXMAP_WIDTH + FLIPPY_WIDTH;
  XP_ASSERT(!info->allowiconresize);
  XP_ASSERT(maxdepth > 0);
  if (value != info->iconcolumnwidth) {
    info->iconcolumnwidth = value;
    XtVaSetValues(outline, XmNcolumn, 0, XmNcolumnWidth, value, 0);
  }
}


static XmString
fe_outline_get_header(char *widget, char *header, XP_Bool highlight)
{
	char		clas[512];
	XrmDatabase	db;
	char		name[512];
	char		*type;
	XrmValue	value;
	XmString	xms;

	db = XtDatabase(fe_display);
	(void) PR_snprintf(clas, sizeof(clas), "%s.MailNewsColumns.Pane.Column",
		fe_progclass);
	(void) PR_snprintf(name, sizeof(name), "%s.mailNewsColumns.%s.%s",
		fe_progclass, widget, header);
	if (XrmGetResource(db, name, clas, &type, &value))
	{
		xms = XmStringCreate((char *) value.addr,
			highlight ? "BOLD" : XmFONTLIST_DEFAULT_TAG);
	}
	else
	{
		xms = XmStringCreate(header,
			highlight ? "BOLD" : XmFONTLIST_DEFAULT_TAG);
	}

	return xms;
}


void 
fe_OutlineSetHeaders(Widget widget, char** headers)
{				/* Fix i18n in here ### */
  fe_OutlineInfo* info = fe_get_info(widget);
  int i;
  XmString str;
  char* ptr;
  char* ptr2;
  char* ptr3;
  int value;
  int width;


  ptr = *info->posinfo;
  for (i=0 ; i<=info->numcolumns ; i++) {
    if (ptr == NULL) {
      FREEIF(*info->posinfo);
      break;
    }
    ptr2 = XP_STRCHR(ptr, ';');
    if (ptr2) *ptr2 = '\0';
    ptr3 = XP_STRCHR(ptr, ':');
    if (!ptr3) {
      FREEIF(*info->posinfo);
      break;
    }
    *ptr3 = '\0';
    for (value = 0 ; value <= info->numcolumns ; value++) {
      if (strcmp(headers[value], ptr) == 0) break;
    }
    if (value > info->numcolumns) {
      FREEIF(*info->posinfo);
      break;
    }
    info->columnIndex[i] = value;
    width = atoi(ptr3 + 1);
    *ptr3 = ':';
    if (i == info->numcolumns) width = MAX_COLUMN_WIDTH; /* Last column is
							    always huge.*/
    if (width < MIN_COLUMN_WIDTH) width = MIN_COLUMN_WIDTH;
    if (width > MAX_COLUMN_WIDTH) width = MAX_COLUMN_WIDTH;
    XtVaSetValues(widget,
		  XmNcolumn, i,
		  XmNcolumnSizePolicy, XmCONSTANT,
		  XmNcolumnWidth, width,
		  0);
    if (ptr2) *ptr2++ = ';';
    ptr = ptr2;
  }

  if (*info->posinfo) {
    /* Check that every column was mentioned, and no duplicates occurred. */
    int* check = (int*) XP_ALLOC(sizeof(int) * (info->numcolumns + 1));
    for (i=0 ; i<=info->numcolumns ; i++) check[i] = 0;
    for (i=0 ; i<=info->numcolumns ; i++) {
      int w = info->columnIndex[i];
      if (w < 0 || w > info->numcolumns || check[w]) {
	FREEIF(*info->posinfo);
	break;
      }
      check[w] = 1;
    }
    XP_FREE(check);
  }

  if (!*info->posinfo) fe_set_default_column_widths(widget);

  info->headers = headers;
  info->headerhighlight = (XP_Bool*)
    XP_ALLOC((info->numcolumns + 1) * sizeof(XP_Bool));
  XP_MEMSET(info->headerhighlight, 0,
	    (info->numcolumns + 1) * sizeof(XP_Bool));
  XmLGridAddRows(widget, XmHEADING, 0, 1);
  for (i=0 ; i<=info->numcolumns ; i++) {
    char* name = headers[info->columnIndex[i]];
    fe_OutlineType type = FE_OUTLINE_String;
    if (strcmp(name, "Flag") == 0) {
      type = FE_OUTLINE_Marked;
    } else if (strcmp(name, "UnreadMsg") == 0) {
      type = FE_OUTLINE_Unread;
    } else if (strcmp(name, "Sub") == 0) {
      type = FE_OUTLINE_Subscribed;
    } else if (strcmp(name, "Depth") == 0) {
      name = " ";
    }
    if (type == FE_OUTLINE_String) {
      str = fe_outline_get_header(XtName(widget), name, 0);
      XtVaSetValues(widget, XmNcolumn, i, XmNrowType, XmHEADING, XmNrow, 0,
		    XmNcellLeftBorderType, XmBORDER_LINE,
		    XmNcellRightBorderType, XmBORDER_LINE,
		    XmNcellTopBorderType, XmBORDER_LINE,
		    XmNcellBottomBorderType, XmBORDER_LINE,
		    XmNcellString, str, 0);
      XmStringFree(str);
    } else {
      fe_icon* icon = fe_outline_lookup_icon(type);
      XtVaSetValues(widget, XmNcolumn, i, XmNrowType, XmHEADING, XmNrow, 0,
		    XmNcellLeftBorderType, XmBORDER_LINE,
		    XmNcellRightBorderType, XmBORDER_LINE,
		    XmNcellTopBorderType, XmBORDER_LINE,
		    XmNcellBottomBorderType, XmBORDER_LINE,
		    XmNcellType, XmPIXMAP_CELL, XmNcellPixmap, icon->pixmap,
		    0);
    }
  }
  XtVaSetValues(widget, XmNallowColumnResize, True, 0);
  XtAddCallback(widget, XmNresizeCallback, fe_outline_resize, info);
}


void
fe_OutlineChangeHeaderLabel(Widget widget, const char* headername,
			    const char* label)
{
  fe_OutlineInfo* info = fe_get_info(widget);
  int i;
  int j;
  XmString str;
  if (label == NULL) label = headername;
  for (i=0 ; i<info->numcolumns ; i++) {
    unsigned char celltype;
    XtVaGetValues(widget, XmNcellType, &celltype, 0);

    if (XP_STRCMP(info->headers[i], headername) == 0) {
      for (j=0 ; j<info->numcolumns ; j++) {
	if (info->columnIndex[j] != i) continue;
	str = XmStringCreate((char*/*FUCK*/)label,
			     info->headerhighlight[i] ? "BOLD"
			     : XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(widget, XmNcolumn, j, XmNrowType, XmHEADING, XmNrow, 0,
		      XmNcellString, str, XmNcellType, XmTEXT_CELL, 0);
	XmStringFree(str);
	return;
      }
    }
  }
  XP_ASSERT(0);
}


void
fe_OutlineSetHeaderHighlight(Widget widget, const char* header, XP_Bool value)
{
  fe_OutlineInfo* info = fe_get_info(widget);
  int i, j;
  XmString str;
  for (i=0 ; i<=info->numcolumns ; i++) {
    if (XP_STRCMP(info->headers[i], header) == 0) {
      if (info->headerhighlight[i] == value) return;
      info->headerhighlight[i] = value;
      for (j=0 ; j<=info->numcolumns ; j++) {
	if (info->columnIndex[j] == i) {
	  str = fe_outline_get_header(XtName(widget), info->headers[i], value);
	  XtVaSetValues(widget, XmNcolumn, j, XmNrowType, XmHEADING,
			XmNrow, 0, XmNcellString, str, 0);
	  XmStringFree(str);
	  return;
	}
      }
      XP_ASSERT(0);
    }
  }
  XP_ASSERT(0);
}



void
fe_OutlineChange(Widget widget, int first, int length, int newnumrows)
{
  fe_OutlineInfo* info = fe_get_info(widget);
  int i;
  DataRow = -1;
  if (newnumrows != info->numrows) {
    if (newnumrows > info->numrows) {
      XmLGridAddRows(widget, XmCONTENT, -1, newnumrows - info->numrows);
    } else {
      XmLGridDeleteRows(widget, XmCONTENT, newnumrows,
			info->numrows - newnumrows);
    }
    info->numrows = newnumrows;
    length = newnumrows - first;
  }
  if (first == 0 && length == newnumrows) {
    XmLGridRedrawAll(widget);
  } else {
    for (i=first ; i<first + length ; i++) {
      XmLGridRedrawRow(widget, XmCONTENT, i);
    }
  }
  XFlush(XtDisplay(widget));
}


void
fe_OutlineSelect(Widget widget, int row, Boolean exclusive)
{
  if (exclusive) XmLGridDeselectAllRows(widget, False);
  XmLGridSelectRow(widget, row, False);
}


void
fe_OutlineUnselect(Widget widget, int row)
{
  XmLGridDeselectRow(widget, row, False);
}

void
fe_OutlineUnselectAll(Widget widget)
{
  XmLGridDeselectAllRows(widget, False);
}


void
fe_OutlineMakeVisible(Widget widget, int visible)
{
  fe_OutlineInfo* info = fe_get_info(widget);
  int firstrow, lastrow;
  XRectangle rect;
  Dimension height, shadowthickness;
  if (visible < 0) return;
  if (info->visibleTimer) {
    info->visibleLine = visible;
    return;
  }
  XtVaGetValues(widget, XmNscrollRow, &firstrow, XmNheight, &height,
		XmNshadowThickness, &shadowthickness, NULL);
  height -= shadowthickness;
  for (lastrow = firstrow + 1 ; ; lastrow++) {
    if (XmLGridRowColumnToXY(widget, XmCONTENT, lastrow, XmCONTENT, 0,
			     &rect) < 0) {
      break;
    }
    if (rect.y + rect.height >= height) break;
  }
  if (visible >= firstrow && visible < lastrow) return;
  firstrow = visible - ((lastrow - firstrow) / 2);
  if (firstrow < 0) firstrow = 0;
  XtVaSetValues(widget, XmNscrollRow, firstrow, 0);
}


int
fe_OutlineRootCoordsToRow(Widget outline, int x, int y, XP_Bool* nearbottom)
{
  Position rootx;
  Position rooty;
  int row;
  int column;
  unsigned char rowtype;
  unsigned char coltype;
  XtTranslateCoords(outline, 0, 0, &rootx, &rooty);
  x -= rootx;
  y -= rooty;
  if (x < 0 || y < 0 ||
      x >= outline->core.width || y >= outline->core.height) {
    return -1;
  }
  if (XmLGridXYToRowColumn(outline, x, y, &rowtype, &row,
			   &coltype, &column) < 0) {
    return -1;
  }
  if (rowtype != XmCONTENT || coltype != XmCONTENT) return -1;
  if (nearbottom) {
    int row2;
    *nearbottom = (XmLGridXYToRowColumn(outline, x, y + 5, &rowtype, &row2,
					&coltype, &column) >= 0 &&
		   row2 > row);
  }
  return row;
}


#define SCROLLMARGIN 50
#define INITIALWAIT 500
#define REPEATINTERVAL 100

static void
fe_outline_drag_scroll(void* closure)
{
  fe_OutlineInfo* info = (fe_OutlineInfo*) closure;
  int row;
  info->dragscrolltimer = FE_SetTimeout(fe_outline_drag_scroll, info,
					REPEATINTERVAL);
  XtVaGetValues(info->widget, XmNscrollRow, &row, 0);
  row += info->dragscrolldir;
  XtVaSetValues(info->widget, XmNscrollRow, row, 0);
}

void
fe_OutlineHandleDragEvent(Widget outline, XEvent* event, fe_dnd_Event type,
			  fe_dnd_Source* source)
{
  fe_OutlineInfo* info = fe_get_info(outline);
  XP_Bool doscroll = FALSE;
  Position rootx;
  Position rooty;
  int x, y;
  unsigned char rowtype;
  unsigned char coltype;
  int row;
  int column;
  XmLCGridColumn* col;
  Dimension width;
  int i;
  int delta;
  int tmp;
  int total;

  if (source->type == FE_DND_COLUMN) {
    if (type == FE_DND_DROP && source->closure == outline) {
      XtTranslateCoords(outline, 0, 0, &rootx, &rooty);
      x = event->xbutton.x_root - rootx;
      y = event->xbutton.y_root - rooty;
      if (XmLGridXYToRowColumn(info->widget, x, y,
			       &rowtype, &row, &coltype, &column) >= 0) {
	if (column != info->dragcolumn) {
	  /* Get rid of the hack that makes the last column really wide.  Make
             it be exactly as wide as it appears, so that if it no longer
	     ends up as the last column, it has a reasonable width. */
	  total = outline->core.width;
	  for (i=0 ; i<info->numcolumns ; i++) {
	    col = XmLGridGetColumn(outline, XmCONTENT, i);
	    XtVaGetValues(outline,
			  XmNcolumnPtr, col,
			  XmNcolumnWidth, &width, 0
			  );
	    total -= ((int) width); /* Beware any unsigned lossage... */
	  }
	  if (total < MIN_COLUMN_WIDTH) total = MIN_COLUMN_WIDTH;
	  width = total;
	  if (width > MAX_COLUMN_WIDTH) width = MAX_COLUMN_WIDTH;
	  XtVaSetValues(outline, XmNcolumn, info->numcolumns,
			XmNcolumnWidth, width, 0);

	  if (info->dragcolumn < column) {
	    delta = 1;
	  } else {
	    delta = -1;
	  }
	  tmp = info->columnIndex[info->dragcolumn];
	  for (i=info->dragcolumn ; i != column ; i += delta) {
	    info->columnIndex[i] = info->columnIndex[i + delta];
	  }
	  info->columnIndex[column] = tmp;

	  XmLGridMoveColumns(outline, column, info->dragcolumn, 1);

	  /* Now restore the hack of having the last column be wide. */
	  XtVaSetValues(outline, XmNcolumn, info->numcolumns,
			XmNcolumnWidth, MAX_COLUMN_WIDTH, 0);

	  fe_outline_remember_columns(outline);
	}
      }
    }
    return;
  }

  if (type != FE_DND_DRAG && type != FE_DND_END) return;
  if (type == FE_DND_DRAG) {
    XtTranslateCoords(outline, 0, 0, &rootx, &rooty);
    x = event->xbutton.x_root - rootx;
    y = event->xbutton.y_root - rooty;
    doscroll = (x >= 0 && x < outline->core.width &&
		((y < 0 && y >= -SCROLLMARGIN) ||
		 (y >= outline->core.height &&
		  y < outline->core.height + SCROLLMARGIN)));
    info->dragscrolldir = y > outline->core.height / 2 ? 1 : -1;
  }
  if (!doscroll) {
    if (info->dragscrolltimer) {
      FE_ClearTimeout(info->dragscrolltimer);
      info->dragscrolltimer = NULL;
    }
  } else {
    if (!info->dragscrolltimer) {
      info->dragscrolltimer = FE_SetTimeout(fe_outline_drag_scroll, info,
					    INITIALWAIT);
    }
  }
}


void
fe_OutlineSetDragFeedback(Widget outline, int row, XP_Bool usebox)
{
  fe_OutlineInfo* info = fe_get_info(outline);
  int old = info->dragrow;
  if (old == row && info->dragrowbox == usebox) return;
  info->dragrowbox = usebox;
  info->dragrow = row;
  if (old >= 0) XmLGridRedrawRow(outline, XmCONTENT, old);
  if (row >= 0 && row != old) XmLGridRedrawRow(outline, XmCONTENT, row);
}
