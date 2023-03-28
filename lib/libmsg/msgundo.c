/* -*- Mode: C; tab-width: 4 -*-
   msgundo.c --- mail/news undo facilities
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 7-July-95.
 */

#include "msg.h"
#include "undo.h"


extern int MK_OUT_OF_MEMORY;


#define MAX_UNDO	10

int
msg_undo_Initialize(MWContext* context)
{
  if (!context->msgframe->undo) {
    context->msgframe->undo = UNDO_Create(MAX_UNDO);
    if (!context->msgframe->undo) return MK_OUT_OF_MEMORY;
  }
  UNDO_DiscardAll(context->msgframe->undo);
  return 0;
}


void
msg_undo_Cleanup(MWContext* context)
{
  if (context->msgframe->undo) {
    UNDO_Destroy(context->msgframe->undo);
    context->msgframe->undo = NULL;
  }
}


typedef struct FlagInfo {
  MWContext* context;
  MSG_Folder* folder;
  unsigned int message_offset;
  uint16 flagson;
  uint16 flagsoff;
} FlagInfo;

static XP_AllocStructInfo FlagAlloc =
{ XP_INITIALIZE_ALLOCSTRUCTINFO(sizeof(FlagInfo)) };


static int
msg_undo_doflagchange(void* closure)
{
  FlagInfo* info = (FlagInfo*) closure;
  return msg_ChangeFlagAny(info->context, info->folder, info->message_offset,
						   info->flagson, info->flagsoff);
}

static void
msg_undo_freeflaginfo(void* closure)
{
  XP_FreeStruct(&FlagAlloc, closure);
}

void
msg_undo_LogFlagChange(MWContext* context, MSG_Folder* folder,
		       unsigned int message_offset,
		       uint16 flagson, uint16 flagsoff)
{
  FlagInfo* info;

  info = (FlagInfo*) XP_AllocStruct(&FlagAlloc);
  if (!info) {
    UNDO_DiscardAll(context->msgframe->undo);
    return;			/* ### Should propagate error. */
  }
  info->context = context;
  info->folder = folder;
  info->message_offset = message_offset;
  info->flagson = flagson;
  info->flagsoff = flagsoff;
  UNDO_LogEvent(context->msgframe->undo, msg_undo_doflagchange,
				msg_undo_freeflaginfo, info);
}


void
msg_undo_StartBatch(MWContext* context)
{
  UNDO_StartBatch(context->msgframe->undo);
  msg_DisableUpdates(context); /* This is an ugly place for this call, but it's
								  very convenient. */

}


void
msg_undo_EndBatch(MWContext* context)
{
  UNDO_EndBatch(context->msgframe->undo);
  msg_EnableUpdates(context);	/* This is an ugly place for this call, but
								   it's very convenient. */
}


int
msg_Undo(MWContext *context)
{
  return UNDO_DoUndo(context->msgframe->undo);
}

int
msg_Redo(MWContext *context)
{
  return UNDO_DoRedo(context->msgframe->undo);
}


XP_Bool
msg_CanUndo(MWContext* context)
{
  return UNDO_CanUndo(context->msgframe->undo);
}

XP_Bool
msg_CanRedo(MWContext* context)
{
  return UNDO_CanRedo(context->msgframe->undo);
}
