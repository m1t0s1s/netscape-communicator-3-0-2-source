/* -*- Mode: C; tab-width: 8 -*-
   msgwin.c --- mail message window
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 23-Jul-96
 */

#include "mozilla.h"
#include "xfe.h"
#include <msgcom.h>
#include "outline.h"
#include "mailnews.h"
#include "menu.h"
#include "felocale.h"
#include "xpgetstr.h"
#include "libi18n.h"

#include <Xm/Label.h>

#include <X11/IntrinsicP.h>     /* for w->core.height */
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>

static void
fe_msgwin_do_command(MWContext *context, MSG_CommandType command)
{
  MSG_ViewIndex index;
  MessageKey key;
  MSG_FolderInfo *folder;

  if (!context)
    return;

  XP_ASSERT(context->type == MWContextMailMsg);

  fe_UserActivity(context);

  if (context->type != MWContextMailMsg)
    {
      /* It's illegal to call MSG_CommandStatus() on a context not of the
	 above types.  We can only get here when invoked by an accelerator:
	 otherwise, the menu item wouldn't have been present at all. */
      XBell(XtDisplay (CONTEXT_WIDGET(context)), 0);
      return;
    }

  if (command == (MSG_CommandType) ~0)
    return;


  MSG_GetCurMessage(MSG_CONTEXT_DATA(context)->messagepane,
		    &folder, &key, &index);

  if (folder == NULL || key == MSG_MESSAGEKEYNONE)
    return; /* should this be an error, since a message window _must_ be
	       displaying a message? */


  switch(command) {
    /* all the commands that can be issued which act on 
       the message presently displayed. */
  case MSG_DeleteMessage:
  case MSG_ReplyToSender:
  case MSG_ReplyToAll:
  case MSG_ForwardMessage:
  case MSG_ForwardMessageQuoted:
  case MSG_Rot13Message:
  case MSG_AttachmentsInline:
  case MSG_AttachmentsAsLinks:
  case MSG_EditAddressBook:
  case MSG_EditAddress:
  case MSG_AddSender:
  case MSG_AddAll:
  case MSG_MailNew:
  case MSG_MarkMessagesRead:
  case MSG_MarkMessagesUnread:
  case MSG_ToggleMessageRead:
  case MSG_SaveMessagesAs:
  case MSG_SaveMessagesAsAndDelete:
    /*  case MSG_SaveMessagesAsAndDelete: */
  case MSG_MarkThreadRead:
  case MSG_ShowMicroHeaders:
  case MSG_ShowSomeHeaders:
  case MSG_ShowAllHeaders:
    break;

  default:
    /* this is an invalid action on a message window -- I think. */
    XP_ASSERT(0);
    break;
  }

  MSG_Command (MSG_CONTEXT_DATA(context)->messagepane, command, &index, 1);
}

/* called for items in the toolbar. */
void 
fe_msgwin_cb(Widget widget, 
	     XtPointer closure, 
	     XtPointer calldata)
{
  MWContext *context = closure;

  XP_ASSERT(widget);
  XP_ASSERT(context);

  if (!widget || !closure) return;
  fe_msgwin_do_command(context,
		       fe_widget_name_to_command(XtName(widget)));
}

/* called for menu items. */
void
fe_msgwin_action(Widget widget, XEvent *event, String *av, Cardinal *ac)
{
}
