/* -*- Mode: C; tab-width: 8 -*-
   outline.h --- includes for the outline widget hack.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 24-Jun-95.
 */
#ifndef __xfe_outline_h_
#define __xfe_outline_h_

#include "msgcom.h"
#include "dragdrop.h"

typedef enum {
  FE_OUTLINE_String,		/* Draw the given string */
  FE_OUTLINE_ChoppedString,	/* Draw the given string, clipping out chars
				   from the middle */
  FE_OUTLINE_Marked,		/* The "is marked" icon. */
  FE_OUTLINE_Unmarked,		/* The "is not marked" icon. */
  FE_OUTLINE_Read,		/* The "is read" icon. */
  FE_OUTLINE_Unread,		/* The "is not read" icon. */
  FE_OUTLINE_Subscribed,	/* The "is subscribed" icon. */
  FE_OUTLINE_Unsubscribed,	/* The "is not subscribed" icon. */

  FE_OUTLINE_Article,
  FE_OUTLINE_MailMessage,
  FE_OUTLINE_Folder,
  FE_OUTLINE_Newsgroup,
  FE_OUTLINE_Newshost,

  FE_OUTLINE_Bookmark,
  FE_OUTLINE_ChangedBookmark,
  FE_OUTLINE_UnknownBookmark,
  FE_OUTLINE_ClosedHeader,
  FE_OUTLINE_OpenedHeader,
  FE_OUTLINE_ClosedHeaderDest,
  FE_OUTLINE_OpenedHeaderDest,
  FE_OUTLINE_ClosedHeaderMenu,
  FE_OUTLINE_OpenedHeaderMenu,
  FE_OUTLINE_ClosedHeaderMenuDest,
  FE_OUTLINE_OpenedHeaderMenuDest,

  FE_OUTLINE_Address,
  FE_OUTLINE_ClosedList,
  FE_OUTLINE_OpenedList
  
} fe_OutlineType;

typedef struct fe_OutlineDesc {
  MSG_FLIPPY_TYPE flippy; 
  int depth;
  fe_OutlineType icon;
  fe_OutlineType type[10];	/* What to draw in this column. */
  const char* str[10];		/* Used only if the corresponding entry in
				   the type array is FE_OUTLINE_String. */
  const char* tag;		/* What tag to use to paint any strings.
				   The datafunc *must* fill this in.  Choices
				   are (right now) XmFONTLIST_DEFAULT_TAG,
				   "BOLD", and "ITALIC". */
} fe_OutlineDesc;


typedef Boolean (*fe_OutlineGetDataFunc)(Widget, void* closure, int row,
					 fe_OutlineDesc* data);

typedef void (*fe_OutlineClickFunc)(Widget, void* closure, int row, int column,
				    char* header, /* Header for this column */
				    int button, int clicks, Boolean shift,
				    Boolean ctrl);

typedef void (*fe_OutlineIconClickFunc)(Widget, void* closure, int row);


/*
 * Be sure to pass fe_OutlineCreate() an ArgList that has at least 5 empty
 * slots that you aren't using, as it will.  Yick.  ###
 */
extern Widget fe_OutlineCreate(MWContext* context, Widget parent, String name,
			       ArgList av, Cardinal ac,
			       int maxindentdepth, /* Make the indentation
						      icon column deep enough
						      to initially show this
						      many levels. If zero,
						      don't let the user resize
						      it, and it will be
						      resized by calls to
						      fe_OutlineSetMaxDepth().
						      */
			       int numcolumns, int* columnwidths,
			       fe_OutlineGetDataFunc datafunc,
			       fe_OutlineClickFunc clickfunc,
			       fe_OutlineIconClickFunc iconfunc,
			       void* closure,
			       fe_dnd_Type dragtype,
			       char** posinfo);


/* Set the header strings for the given widget.  The outline code will keep the
   pointers you give it, so you'd better never touch them.  It will pass these
   same pointers back to the clickfunc, so you can write code that compares
   strings instead of remembering magic numbers.  Note that any column
   reordering that may get done by the user is completely hidden from the
   caller; the reordering is a display artifact only. */

extern void fe_OutlineSetHeaders(Widget widget, char** headers);

/* Tell the outline whether draw the given header in a highlighted way
   (currently, boldface). */

extern void fe_OutlineSetHeaderHighlight(Widget widget, const char* header,
					 XP_Bool value);


/* Change the text displayed for the given header.  Note the original header
   string is still used for calls that want to manipulate that header; this
   just changes what string is presented for the user.  If the label is
   NULL, then the header is reverted back to its original string. */

extern void fe_OutlineChangeHeaderLabel(Widget widget, const char* headername,
					const char* label);



/* Tells the outline widget that "length" rows starting at "first"
 have been changed.  The outline is now to consider itself to have
 "newnumrows" rows in it.*/

extern void fe_OutlineChange(Widget outline, int first, int length,
			     int newnumrows);

extern void fe_OutlineSelect(Widget outline, int row, Boolean exclusive);

extern void fe_OutlineUnselect(Widget outline, int row);

extern void fe_OutlineUnselectAll(Widget outline);

extern void fe_OutlineMakeVisible(Widget outline, int visible);


/* Tell the outline widget to set the maximum indentation depth.  This should
 only be called if the outline widget was created with maxindentdepth set to
 zero.*/
extern void fe_OutlineSetMaxDepth(Widget outline, int maxdepth);


/* Given an (x,y) in the root window, return which row it corresponds to in the
   outline (-1 if none). If nearbottom is given, then it is set to TRUE if the
   (x,y) is very near the bottom of the returned row (used by drag'n'drop to
   determine if the cursor should be considered to be between rows.) */
extern int fe_OutlineRootCoordsToRow(Widget outline, int x, int y,
				     XP_Bool* nearbottom);


/* Handles auto-scrolling during a drag operation.  If you call this, then
   you have to call this for every event on a particular drag.   This routine
   should be called if the given drag operation could possibly be destined
   for this widget. */
extern void fe_OutlineHandleDragEvent(Widget outline, XEvent* event,
				      fe_dnd_Event type,
				      fe_dnd_Source* source);


/* Helps provide feedback during drag operation.  Up to one row may have a drag
   highlight, which is either a box around it or a line under it.  Setting row
   to a negative value will turn it off entirely. */
extern void fe_OutlineSetDragFeedback(Widget outline, int row, XP_Bool usebox);


#endif /* __xfe_outline_h_ */
