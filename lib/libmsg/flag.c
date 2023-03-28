/* -*- Mode: C; tab-width: 4 -*-
   flag.c --- mail/news flag changing code
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 7-July-95.
 */


#include "msg.h"
#include "mailsum.h"
#include "newsrc.h"
#include "msgundo.h"


extern int MK_MSG_FOLDER_UNREADABLE;
extern int MK_OUT_OF_MEMORY;


static XP_Bool
msg_mark_expunged_1(MWContext* context,
					MSG_ThreadEntry** parent, MSG_ThreadEntry* msg)
{
  MSG_ThreadEntry* cur = *parent;
  for ( ; cur ; cur = cur->next) {
	if (cur == msg) {
	  if (msg->first_child) {
		*parent = msg->first_child;
		for (cur = msg->first_child ; cur->next ; cur = cur->next) ;
		cur->next = msg->next;
	  } else {
		*parent = msg->next;
	  }
	  if (context->msgframe->displayed_message == msg) {
		msg_ClearMessageArea(context);
		XP_ASSERT(context->msgframe->displayed_message == NULL);
		context->msgframe->displayed_message = NULL;
	  }
	  msg_RedisplayMessages(context);
	  return TRUE;
	}
	if (!(cur->flags & MSG_FLAG_ELIDED)) {
	  if (msg_mark_expunged_1(context, &(cur->first_child), msg)) {
		return TRUE;
	  }
	}
	parent = &(cur->next);
  }
  return FALSE;
}



void
msg_MarkExpunged(MWContext* context, struct mail_FolderData* msgs,
				 MSG_ThreadEntry* msg)
{
  XP_Bool result;
  MSG_ThreadEntry** prev;

  XP_ASSERT(msg != NULL);
  result = msg_mark_expunged_1(context, &(msgs->msgs), msg);
  if (result) {
	/* Check to see if there is now a dummy header which has no entries or
	   only one entry in it.  If so, nuke it.*/
	for (prev = &(msgs->msgs); *(prev); prev = &((*prev)->next)) {
	  if ((*prev)->flags & MSG_FLAG_EXPIRED) {
		if ((*prev)->first_child == NULL) {
		  if (((*prev)->flags & MSG_FLAG_SELECTED) &&
			  msgs == context->msgframe->msgs) {
			context->msgframe->num_selected_msgs--;
		  }
		  *prev = (*prev)->next;
		  msg_RedisplayMessages(context);
		  break;
		} else if ((*prev)->first_child->next == NULL &&
				   (*prev)->first_child->first_child == NULL) {
		  if (((*prev)->flags & MSG_FLAG_SELECTED) &&
			  msgs == context->msgframe->msgs) {
			context->msgframe->num_selected_msgs--;
		  }
		  (*prev)->first_child->next = (*prev)->next;
		  *prev = (*prev)->first_child;
		  msg_RedisplayMessages(context);
		  break;
		}
	  }
	}
  } else {
	for (prev = &(msgs->hiddenmsgs); (*prev); prev = &((*prev)->next)) {
	  if (*prev == msg) break;
	}
	XP_ASSERT(*prev == msg);
	*prev = msg->next;
  }
  msg->first_child = NULL;
  msg->next = msgs->expungedmsgs;
  msgs->expungedmsgs = msg;
  msg->flags &= ~MSG_FLAG_UPDATING;	/* If we ever undelete this message,
									   and this flag is set, then we get
									   unhappy.*/
}


extern int
msg_ChangeFlag(MWContext* context, MSG_ThreadEntry* msg,
			   uint16 flagson, uint16 flagsoff)
{
  XP_Bool needresort = FALSE;
  XP_Bool needrefresh;
  XP_Bool makevisible = FALSE;
  XP_Bool needrecount = FALSE;
  int status = 0;
  MSG_Folder *folder;

  XP_ASSERT (context->type == MWContextMail || context->type == MWContextNews);
  if (context->type != MWContextMail && context->type != MWContextNews)
	return -1;

  XP_ASSERT((flagson & flagsoff) == 0);

  if (!msg) return 0;

  context->msgframe->cacheinfo.valid = FALSE;

  flagson &= ~msg->flags;
  flagsoff &= msg->flags;

  folder = context->msgframe->opened_folder;

  /* If this message is a dummy parent message, don't allow it to 
	 have the various user-visible flags set on it. */
  if ((msg->flags & MSG_FLAG_EXPIRED) &&
	  (flagson & (MSG_FLAG_READ | MSG_FLAG_REPLIED | MSG_FLAG_MARKED)))
	flagson &= (~ (MSG_FLAG_READ | MSG_FLAG_REPLIED | MSG_FLAG_MARKED));


  needrefresh = ((flagson | flagsoff) & (MSG_FLAG_READ |
										 MSG_FLAG_REPLIED |
										 MSG_FLAG_MARKED |
										 MSG_FLAG_SELECTED |
										 MSG_FLAG_ELIDED)) != 0;

  /* Can only set flags on folders which aren't directory-like. */
  XP_ASSERT (! (folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								 MSG_FOLDER_FLAG_DIRECTORY)));

  if ((flagson | flagsoff) & MSG_FLAG_READ) {
	if ((folder->flags & MSG_FOLDER_FLAG_NEWSGROUP)) {
	  if (msg->data.news.article_number > 0 &&
		  (msg->flags & MSG_FLAG_EXPIRED) == 0) {
		int status;
		struct msg_NewsRCSet *newsrc =
		  msg_GetNewsRCSet(context, folder->data.newsgroup.group_name,
						   context->msgframe->data.news.newsrc_file);
		uint32 num = folder->unread_messages;
		if (flagson & MSG_FLAG_READ)
		  {
			if (!newsrc)
			  {
				/* There wasn't a newsrc entry for this group.  This probably
				   means that this group is both unsubscribed, and not listed
				   in the newsrc file.  So let's add one.  This will cause (at
				   the next save) there to be one more line in the newsrc file,
				   for this group in the unsubscribed state, with this article
				   marked as read.
				 */
				newsrc = msg_AddNewsRCSet (folder->data.newsgroup.group_name,
								   context->msgframe->data.news.newsrc_file);
				if (!newsrc) return MK_OUT_OF_MEMORY;
			  }
			status = msg_MarkArtAsRead(newsrc, msg->data.news.article_number);
			XP_ASSERT(num > 0);
			num--;
		  }
		else
		  {
			if (newsrc)
			  /* If there is no newsrc entry, then marking this message as
				 unread is a no-op, as that is the default.  It would be
				 bad to add a line to the newsrc file just for this. */
			  status = msg_MarkArtAsUnread(newsrc,
										   msg->data.news.article_number);

			num++;
		  }
		if (status < 0)
		  return MK_OUT_OF_MEMORY;
		else if (status > 0)
		  msg_NoteNewsrcChanged(context);
		msg_UpdateNewsFolderMessageCount(context, folder, num, FALSE);
	  }
	} else {
	  if (flagson & MSG_FLAG_READ) {
		XP_ASSERT(folder->unread_messages > 0);
		if (folder->unread_messages > 0)
		  folder->unread_messages--;
	  } else {
		folder->unread_messages++;
	  }
	  msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
	}
  }
  if (flagson & MSG_FLAG_EXPUNGED) {
	XP_ASSERT(!((flagson | flagsoff) & (MSG_FLAG_SELECTED | MSG_FLAG_EXPIRED |
										MSG_FLAG_ELIDED)));
	if (msg->flags & MSG_FLAG_SELECTED) {
	  msg_ChangeFlag(context, msg, 0, MSG_FLAG_SELECTED);
	}	  
	if (msg->flags & MSG_FLAG_EXPIRED) {
	  flagson &= ~MSG_FLAG_EXPUNGED;
	} else {
	  folder->total_messages--;
	  if (!(msg->flags & MSG_FLAG_READ)) folder->unread_messages--;
	  msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
	  msg_MarkExpunged(context, context->msgframe->msgs, msg);
	  if (XP_Gethash(context->msgframe->msgs->message_id_table,
					 context->msgframe->msgs->string_table[msg->id],
					 NULL) == msg) {
		status = XP_Remhash(context->msgframe->msgs->message_id_table,
							context->msgframe->msgs->string_table[msg->id]);
		XP_ASSERT(status > 0);
	  }
	}
  }
  if (flagsoff & MSG_FLAG_EXPUNGED) {
	MSG_ThreadEntry** prev;
	for (prev = &(context->msgframe->msgs->expungedmsgs);
		 (*prev); 
		 prev = &((*prev)->next)) {
	  if (*prev == msg) break;
	}
	XP_ASSERT(*prev == msg);
	*prev = msg->next;
	msg->first_child = NULL;
	msg->next = context->msgframe->msgs->msgs;
	context->msgframe->msgs->msgs = msg;
	folder->total_messages++;
	if (!(msg->flags & MSG_FLAG_READ)) folder->unread_messages++;
	msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
	if (XP_Gethash(context->msgframe->msgs->message_id_table,
				   context->msgframe->msgs->string_table[msg->id],
				   NULL) == NULL) {
	  XP_Puthash(context->msgframe->msgs->message_id_table,
				 context->msgframe->msgs->string_table[msg->id], msg);
	}
	needresort = TRUE;
  }
  if (flagson & MSG_FLAG_SELECTED) {
	context->msgframe->num_selected_msgs++;
	makevisible = TRUE;
	context->msgframe->lastSelectMsg = msg;
  }
  if (flagsoff & MSG_FLAG_SELECTED) {
	XP_ASSERT(context->msgframe->num_selected_msgs > 0);
	if (context->msgframe->num_selected_msgs > 0) {
	  context->msgframe->num_selected_msgs--;
	} else {
	  needrecount = TRUE;
	}
	if (context->msgframe->lastSelectMsg == msg) {
	  context->msgframe->lastSelectMsg = NULL;
	}
  }
  

  msg->flags = (msg->flags | flagson) & ~flagsoff;

  if ((flagson | flagsoff) & MSG_FLAG_ELIDED) {
	if (!msg->first_child) {
	  flagson &= ~MSG_FLAG_ELIDED;
	  flagsoff &= ~MSG_FLAG_ELIDED;
	} else {
	  msg_RedisplayMessages(context);
	}
  }

  if ((flagson | flagsoff) & (MSG_FLAG_READ)) {
	msg_UpdateToolbar(context);
  }

  flagson &= MSG_FLAG_UNDOABLE;
  flagsoff &= MSG_FLAG_UNDOABLE;
  if (flagson || flagsoff) {
	msg_undo_LogFlagChange(context, folder,
						   ((folder->flags & MSG_FOLDER_FLAG_MAIL)
							? msg->data.mail.file_index
							: msg->data.news.article_number),
						   flagsoff, flagson);
	if (folder->flags & MSG_FOLDER_FLAG_MAIL)
	  msg_SelectedMailSummaryFileChanged(context, msg);
  }
  if (needrecount) {
	msg_ReCountSelectedMessages(context);
	msg_RedisplayMessages(context);
  }
  if (needresort) {
	status = msg_ReSort(context);
  } else if (needrefresh) {
	msg_RedisplayOneMessage(context, msg, -1, -1, makevisible);
  }
  return status;
}




static MSG_ThreadEntry*
msg_find_file_index_1(MSG_ThreadEntry* msg, uint32 num)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (msg->data.mail.file_index == num) {
	  return msg;
	}
	result = msg_find_file_index_1(msg->first_child, num);
	if (result) return result;
  }
  return NULL;
}


static MSG_ThreadEntry*
msg_find_file_index(mail_FolderData* msgs, uint32 num)
{
  MSG_ThreadEntry* result;
  result = msg_find_file_index_1(msgs->msgs, num);
  if (result) return result;
  result = msg_find_file_index_1(msgs->hiddenmsgs, num);
  if (result) return result;
  return msg_find_file_index_1(msgs->expungedmsgs, num);
}


static MSG_ThreadEntry*
msg_find_article_number_1(MSG_ThreadEntry* msg, uint32 num)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (msg->data.news.article_number == num) {
	  return msg;
	}
	result = msg_find_article_number_1(msg->first_child, num);
	if (result) return result;
  }
  return NULL;
}


static MSG_ThreadEntry*
msg_find_article_number(mail_FolderData* msgs, uint32 num)
{
  MSG_ThreadEntry* result;
  result = msg_find_article_number_1(msgs->msgs, num);
  if (result) return result;
  result = msg_find_article_number_1(msgs->hiddenmsgs, num);
  if (result) return result;
  return msg_find_article_number_1(msgs->expungedmsgs, num);
}




extern int
msg_ChangeFlagAny(MWContext* context, MSG_Folder* folder,
				  uint32 message_offset, uint16 flagson, uint16 flagsoff)
{
  int status = 0;
  MSG_ThreadEntry* msg = 0;
  mail_FolderData* msgs = 0;

  XP_ASSERT (folder);
  if (!folder) return -1;

  /* Can only set flags on non-directory-like folders. */
  XP_ASSERT (!(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								MSG_FOLDER_FLAG_DIRECTORY |
								MSG_FOLDER_FLAG_ELIDED)));

  switch (context->type) {
  case MWContextMail:
	XP_ASSERT (folder->flags & MSG_FOLDER_FLAG_MAIL);
	if (folder == context->msgframe->opened_folder)
	  return msg_ChangeFlag(context,
							msg_find_file_index(context->msgframe->msgs,
												message_offset),
							flagson, flagsoff);
	flagson &= MSG_FLAG_UNDOABLE;
	flagsoff &= MSG_FLAG_UNDOABLE;

	if (!flagson && !flagsoff) return 0;

	status = msg_GetFolderData (context, folder, &msgs);
	if (status < 0)
	  break;
	XP_ASSERT (msgs);
	if (!msgs) return -1;

	msg = msg_find_file_index(msgs, message_offset);
	if (!msg)
	  {
		status = -1; /* #### */
		break;
	  }
	msg->flags = (msg->flags | flagson) & ~flagsoff;
	msg_MailSummaryFileChanged(context, folder, msgs, msg);
	folder->unread_messages = msgs->unread_messages;
	folder->total_messages = msgs->total_messages;
	msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
	msg_FreeFolderData(msgs);
	break;

  case MWContextNews:
	XP_ASSERT (folder->flags & MSG_FOLDER_FLAG_NEWSGROUP);
	if (folder == context->msgframe->opened_folder) {
	  msg = msg_find_article_number(context->msgframe->msgs, message_offset);
	  if (msg)
		return msg_ChangeFlag(context, msg, flagson, flagsoff);
	}
	flagson &= MSG_FLAG_UNDOABLE;
	flagsoff &= MSG_FLAG_UNDOABLE;

	if (((flagson | flagsoff) & MSG_FLAG_READ) && message_offset > 0) {
	  struct msg_NewsRCSet* newsrc =
		msg_GetNewsRCSet(context,
						 folder->data.newsgroup.group_name,
						 context->msgframe->data.news.newsrc_file);
	  uint32 num = folder->unread_messages;
	  if (!newsrc)
		/* There is a folder, but there is no no newsrc set.  This can happen
		   if we're in "all groups" mode, with groups X and Y visible, reading
		   a message in group X which is crossposted to previously-unknown
		   group Y.  So in that case, don't bother adding group Y to the
		   newsrc file, since we may never actually read that group explicitly.
		 */
		;
	  else if (flagson & MSG_FLAG_READ) {
		status = msg_MarkArtAsRead(newsrc, message_offset);
		if (status > 0) {
		  /* XP_ASSERT(num > 0); */
		  /* Num can be 0 if group A is listed as having 0 articles;
			 and group B is selected, and has an article crossposted
			 to A - but this article arrived between the time group
			 A's count was retrieved and when group B was selected. */
		  if (num > 0) num--;
		}
	  } else {
		status = msg_MarkArtAsUnread(newsrc, message_offset);
		if (status > 0) {
		  num++;
		}
	  }
	  msg_UpdateNewsFolderMessageCount(context, folder, num, FALSE);
	  if (status < 0) /* status is from msg_MarkArtAsRead() */
		return MK_OUT_OF_MEMORY;
	  else if (status > 0)
		msg_NoteNewsrcChanged(context);
	}
	break;
  default:
	XP_ASSERT(0);
	return -1;
  }
  msg_undo_LogFlagChange(context, folder, message_offset, flagsoff, flagson);
  return status;
}


extern void
msg_ToggleFlag(MWContext* context, MSG_ThreadEntry* msg, uint16 flag)
{
  if (!msg) return;
  if (msg->flags & flag) {
	msg_ChangeFlag(context, msg, 0, flag);
  } else {
	msg_ChangeFlag(context, msg, flag, 0);
  }
}
