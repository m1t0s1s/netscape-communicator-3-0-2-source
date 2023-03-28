/* -*- Mode: C; tab-width: 4 -*-
   msgundo.h --- internal defs for the msg library, dealing with undo stuff
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 7-July-95.
 */

#ifndef _MSGUNDO_H_
#define _MSGUNDO_H_


XP_BEGIN_PROTOS


/* Initialize the undo state.  This must be called when the context is
   initialized for mail/news.  It should also be called later whenever
   the undo state should be thrown away (because the user has just
   done an non-undoable operation. If this returns a negative value,
   then we failed big-time (ran out of memory), and we should abort
   everything.*/
extern int msg_undo_Initialize(MWContext* context);


/* Throw away the undo state, as part of destroying the mail/news context. */
extern void msg_undo_Cleanup(MWContext* context);



/* Log an "flag change" event.  The given flag bits indicate what to do to
   execute the undo. */

extern void msg_undo_LogFlagChange(MWContext* context, MSG_Folder* folder,
								   unsigned int message_offset,
								   uint16 flagson, uint16 flagsoff);


/* Mark the beginning of a bunch of actions that should be undone as
   one user action.  Should always be matched by a later call to
   msg_undo_EndBatch().  These calls can nest. */

extern void msg_undo_StartBatch(MWContext* context);

extern void msg_undo_EndBatch(MWContext* context);


extern XP_Bool msg_CanUndo(MWContext* context);
extern XP_Bool msg_CanRedo(MWContext* context);

XP_END_PROTOS

#endif /* !_MSGUNDO_H_ */
