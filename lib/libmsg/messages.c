/* -*- Mode: C; tab-width: 4 -*-
   messages.c --- commands that operate on messages or lists of messages
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-May-95.
 */

#include "msg.h"
#include "msgcom.h"
#include "libmime.h"
#include "mailsum.h"
#include "threads.h"
#include "newsrc.h"
#include "shist.h"
#include "mime.h"
#include "msgundo.h"
#include "libi18n.h"


extern int MK_OUT_OF_MEMORY;
extern int MK_MSG_SEARCH_FAILED;


char *msg_GetMessageURL (MWContext *context, MSG_ThreadEntry *msg);


static XP_Bool
msg_find_message_1(MSG_ThreadEntry* msg, MSG_ThreadEntry* search, int* line,
				   int* depth)
{
  for ( ; msg ; msg = msg->next) {
	if (msg == search) return TRUE;
	(*line)++;
	if (msg->first_child && (msg->flags & MSG_FLAG_ELIDED) == 0) {
	  (*depth)++;
	  if (msg_find_message_1(msg->first_child, search, line, depth)) {
		return TRUE;
	  }
	  (*depth)--;
	}
  }
  return FALSE;
}

int
msg_FindMessage(MWContext* context, MSG_ThreadEntry* search, int* depth)
{
  int line = 0;
  int d = 0;
  if (!msg_find_message_1(context->msgframe->msgs->msgs, search,
						  &line, &d)) return -1;
  if (depth) *depth = d;
  return line;
}



static void
msg_redisplay_1(MSG_ThreadEntry* msg, MSG_ThreadEntry* search,
				MSG_ThreadEntry* visible, int* index,
				int* cur, int* v, int** selected, XP_Bool isflush)
{
  for (; msg ; msg = msg->next) {
	if (isflush) {
	  msg->flags &= ~MSG_FLAG_UPDATING;
	}
	if (msg->flags & MSG_FLAG_SELECTED) {
	  if (*selected) {
		**selected = *cur;
		(*selected)++;
	  }
	}
	if (msg == search) {
	  *index = *cur;
	}
	if (msg == visible) {
	  *v = *cur;
	}
	(*cur)++;
	if (!(msg->flags & MSG_FLAG_ELIDED)) {
	  msg_redisplay_1(msg->first_child, search, visible, index, cur, v,
					  selected, isflush);
	}
  }
}
	


static void
msg_redisplay(MWContext* context, MSG_ThreadEntry* msg, int length,
			  MSG_ThreadEntry* visible, XP_Bool isflush)
{
  int index = 0;
  int total = 0;
  int v = -1;
  int* selected = NULL;
  int* tmp;
  if (!isflush) msg_FlushUpdateMessages(context);
  if (context->msgframe->num_selected_msgs) {
	selected = (int*)
	  XP_ALLOC(context->msgframe->num_selected_msgs * sizeof(int));
	if (!selected) return;
  } else {
	context->msgframe->lastSelectMsg = NULL;
  }
  tmp = selected;
  if (context->msgframe->msgs) {
	msg_redisplay_1(context->msgframe->msgs->msgs, msg, visible, &index,
					&total, &v, &tmp, isflush);
  }
  XP_ASSERT((tmp - selected) <= context->msgframe->num_selected_msgs);
  if (index + length > total) {
	length = total - index;
  }
  FE_UpdateThreadList(context, index, length, total, v,
					  selected, tmp - selected);
  FREEIF(selected);
}




void
msg_RedisplayMessagesFrom(MWContext* context, MSG_ThreadEntry* msg,
						  MSG_ThreadEntry* visible)
{
  msg_redisplay(context, msg, 32767, visible, FALSE);
}

  


void
msg_RedisplayMessages(MWContext* context)
{
  MSG_Frame* f = context->msgframe;
  f->update_all_messages = TRUE;
  if (f->disable_updates_depth == 0) {
	msg_FlushUpdateMessages(context);
  }
}







void
msg_RedisplayOneMessage(MWContext* context, MSG_ThreadEntry* msg,
						int line_number, int depth, XP_Bool makevisible)
{
  MSG_Frame* f = context->msgframe;
  if (!msg) return;
  if (makevisible) f->update_message_visible = msg;
  if (f->update_all_messages) return;
  if (msg->flags & MSG_FLAG_UPDATING) {
	XP_ASSERT(f->update_first_message != NULL);
	return;
  }
  if (line_number < 0 || depth < 0) {
	line_number = msg_FindMessage(context, msg, &depth);
	if (line_number < 0) return; /* Message isn't visible. */
  }
  if (f->update_first_message == NULL) {
	f->update_first_message = msg;
	f->update_last_message = msg;
	f->update_num_messages = 1;
	f->update_message_line = line_number;
	f->update_message_depth = depth;
  } else if (line_number == f->update_message_line - 1) {
	f->update_first_message = msg;
	f->update_num_messages++;
	f->update_message_line = line_number;
	f->update_message_depth = depth;
  } else if (line_number == f->update_message_line + f->update_num_messages) {
	f->update_last_message = msg;
	f->update_num_messages++;
  } else {
	msg_FlushUpdateMessages(context);
	msg_RedisplayOneMessage(context, msg, line_number, depth, makevisible);
	return;
  }
  msg_ChangeFlag(context, msg, MSG_FLAG_UPDATING, 0);
  if (f->disable_updates_depth == 0) {
	msg_FlushUpdateMessages(context);
  }
  if (f->update_num_messages > 20) {
	/* Boy, we're updating a lot of messages.  Let's just update
	 *everything*, and stop wasting all this time going into this
	 potentitally expensive routine. */
	msg_RedisplayMessages(context);
  }
}





void
msg_FlushUpdateMessages(MWContext* context)
{
  MSG_Frame* f = context->msgframe;
  if (f->update_all_messages) {
	f->update_first_message = context->msgframe->msgs ?
	  context->msgframe->msgs->msgs : NULL;
	f->update_num_messages = 32767;
	f->update_all_messages = FALSE;
  }
  if (f->update_num_messages > 0) {
	msg_redisplay(context, f->update_first_message, f->update_num_messages,
				  f->update_message_visible, TRUE);
	f->update_num_messages = 0;
  }
  f->update_first_message = NULL;
  f->update_message_visible = NULL;
}



static MSG_ThreadEntry*
msg_find_message_line(MSG_ThreadEntry* msg, int* find, int* depth)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (*find == 0) return msg;
	(*find)--;
	if (msg->first_child && !(msg->flags & MSG_FLAG_ELIDED)) {
	  if (depth) (*depth)++;
	  result = msg_find_message_line(msg->first_child, find, depth);
	  if (result) return result;
	  if (depth) (*depth)--;
	}
  }
  return NULL;
}


MSG_ThreadEntry*
msg_FindMessageLine(MWContext* context, int find, int* depth)
{
  if (depth) *depth = 0;
  
  XP_ASSERT( context );
  XP_ASSERT( context->msgframe );
  
  if ( !context->msgframe->msgs ) return NULL;
  
  return msg_find_message_line(context->msgframe->msgs->msgs, &find, depth);
}



static MSG_ThreadEntry*
msg_get_thread_line_2(MSG_ThreadEntry* msg, int* find, int* depth,
					  MSG_AncestorInfo** ancestor)
{
  MSG_ThreadEntry* result;
  int d = *depth;
  XP_Bool has_prev = FALSE;
  for ( ; msg ; msg = msg->next, has_prev = TRUE) {
	if (*find == 0) {
	  if (ancestor) {
		*ancestor = (MSG_AncestorInfo*)
		  XP_ALLOC((d + 1) * sizeof(MSG_AncestorInfo));
		if (*ancestor) {
		  (*ancestor)[d].has_next = (msg->next != NULL);
		  (*ancestor)[d].has_prev = has_prev;
		}
	  }
	  return msg;
	}
	(*find)--;
	if (msg->first_child && !(msg->flags & MSG_FLAG_ELIDED)) {
	  (*depth)++;
	  result = msg_get_thread_line_2(msg->first_child, find, depth, ancestor);
	  if (result) {
		if (ancestor && *ancestor) {
		  (*ancestor)[d].has_next = (msg->next != NULL);
		  (*ancestor)[d].has_prev = has_prev;
		}
		return result;
	  }
	  (*depth)--;
	}
  }
  return NULL;
}


static MSG_ThreadEntry*
msg_get_thread_line_1(MWContext* context, int find, int* depth,
					  MSG_AncestorInfo** ancestor)
{
  if (depth) *depth = 0;
  
  XP_ASSERT( context );
  XP_ASSERT( context->msgframe );
  
  if ( !context->msgframe->msgs ) return NULL;
  
  return msg_get_thread_line_2(context->msgframe->msgs->msgs, &find, depth,
							   ancestor);
}




XP_Bool
MSG_DisplayingRecipients(MWContext* context)
{
  int flags = 0;
  XP_ASSERT(context && context->msgframe);
  if (context && context->msgframe && context->msgframe->opened_folder) {
	flags = context->msgframe->opened_folder->flags;
  }
  return 
   (flags & (MSG_FOLDER_FLAG_SENTMAIL |
			 MSG_FOLDER_FLAG_QUEUE |
			 MSG_FOLDER_FLAG_DRAFTS)) != 0 &&
   (flags & MSG_FOLDER_FLAG_INBOX) == 0;
}


XP_Bool
MSG_GetThreadLine(MWContext* context, int line, MSG_MessageLine* data,
				  MSG_AncestorInfo** ancestor)
{
  int depth;
  MSG_ThreadEntry* msg = msg_get_thread_line_1(context, line, &depth,
											   ancestor);

  if (!msg) return FALSE;

  XP_ASSERT(!(msg->flags & MSG_FLAG_EXPUNGED));
  data->flags = msg->flags;
  if (msg->first_child) {
	data->flippy_type =
	  (msg->flags & MSG_FLAG_ELIDED) ? MSG_Folded : MSG_Expanded;
  } else {
	data->flippy_type = MSG_Leaf;
  }
  if (msg->flags & MSG_FLAG_EXPIRED) {
	data->from = "";
	data->date = 0;
	if (msg->first_child) {
	  data->subject =
		context->msgframe->msgs->string_table[msg->first_child->subject];
	} else {
	  XP_ASSERT(0);				/* Why would we ever have a dummy message if
								   it didn't have any children? */
	  data->subject = "";
	}
  } else {
	XP_ASSERT(context->msgframe->opened_folder != NULL);
	if (MSG_DisplayingRecipients(context)) {
	  data->from = context->msgframe->msgs->string_table[msg->recipient];
	} else {
	  data->from = context->msgframe->msgs->string_table[msg->sender];
	}
	data->date = msg->date;
	data->subject = context->msgframe->msgs->string_table[msg->subject];
  }
  data->depth = depth;
  data->moreflags = 0;
  if (msg == context->msgframe->focusMsg) {
	data->moreflags |= MSG_MOREFLAG_LAST_SELECTED;
  }
  return TRUE;
}




int
msg_CountVisibleMessages(MSG_ThreadEntry* msg) {
  int result = 0;
  for ( ; msg ; msg = msg->next) {
	result++;
	if (!(msg->flags & MSG_FLAG_ELIDED)) {
	  result += msg_CountVisibleMessages(msg->first_child);
	}
  }
  return result;
}


int
msg_ElideAllToplevelThreads(MWContext* context)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe) return -1;
  if (!context->msgframe->msgs) return 0;
  msg_DisableUpdates(context);
  for (msg = context->msgframe->msgs->msgs ; msg ; msg = msg->next) {
	if (msg->first_child) {
	  msg_RedisplayMessages(context);
	  msg_ChangeFlag(context, msg, MSG_FLAG_ELIDED, 0);
	}
  }
  msg_EnableUpdates(context);
  return 0;
}


int
msg_Rot13Message (MWContext *context)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;
  msg = context->msgframe->displayed_message;
  if (!msg) return 0;
  context->msgframe->rot13_p = !context->msgframe->rot13_p;
  return msg_ReOpenMessage (context);
}

int
msg_AttachmentsInline (MWContext *context, XP_Bool inline_p)
{
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;
  if ((!context->msgframe->inline_attachments_p) == (!inline_p))
	return 0;
  context->msgframe->inline_attachments_p = (!!inline_p);
  if (!context->msgframe->displayed_message)
	return 0;
  return msg_ReOpenMessage (context);
}

static XP_Bool
msg_mark_messages_read_1(MWContext* context, MSG_ThreadEntry* msg,
						 void* closure)
{
  msg_ChangeFlag(context, msg, MSG_FLAG_READ, 0);
  return TRUE;
}


int
MSG_MarkMessagesRead (MWContext *context)
{
  msg_IterateOverSelectedMessages(context, msg_mark_messages_read_1, NULL);
  return 0;
}


static void
msg_mark_all_messages_read_1(MWContext* context, MSG_Folder* folder)
{
  for ( ; folder ; folder = folder->next) {
	if ((folder->flags & MSG_FOLDER_FLAG_SELECTED) &&
		(folder->flags & MSG_FOLDER_FLAG_NEWSGROUP)) {
	  msg_NewsRCSet* set =
		msg_GetNewsRCSet (context, folder->data.newsgroup.group_name,
						  context->msgframe->data.news.newsrc_file);
	  msg_PreserveSetInUndoHistory(context, set);
	  msg_MarkRangeAsRead(set, 1, folder->data.newsgroup.last_message);
	  msg_UpdateNewsFolderMessageCount(context, folder, 0, FALSE);
	}
	msg_mark_all_messages_read_1(context, folder->children);
  }
}

int
msg_MarkAllMessagesRead (MWContext *context)
{
  XP_ASSERT(context->type == MWContextNews);

  if (context->type != MWContextNews) return -1;

  msg_undo_StartBatch(context);
  if (context->msgframe->opened_folder &&
	  (context->msgframe->opened_folder->flags & MSG_FOLDER_FLAG_SELECTED)) {
	msg_IterateOverAllMessages(context, msg_mark_messages_read_1, NULL);
  }


  msg_mark_all_messages_read_1(context, context->msgframe->folders);

  /* Marking all read is like "leaving" the group, so now's a good time
	 to save, instead of waiting until the next group is selected. */
  msg_SaveNewsRCFile (context);
  msg_undo_EndBatch(context);

  return 0;
}



static XP_Bool
msg_is_descendent_1(MSG_ThreadEntry* msg, MSG_ThreadEntry* search)
{
  for ( ; msg ; msg = msg->next) {
	if (msg == search) return TRUE;
	if (msg_is_descendent_1(msg->first_child, search)) return TRUE;
  }
  return FALSE;
}

static XP_Bool
msg_is_descendent(MSG_ThreadEntry* ancestor, MSG_ThreadEntry* search)
{
  if (search == ancestor) return TRUE;
  return msg_is_descendent_1(ancestor->first_child, search);
}


static void
msg_set_thread_flag_1(MWContext* context, MSG_ThreadEntry* msg, uint16 flags)
{
  for ( ; msg ; msg = msg->next) {
	msg_ChangeFlag(context, msg, flags, 0);
	msg_set_thread_flag_1(context, msg->first_child, flags);
  }
}


static XP_Bool
msg_set_thread_flag(MWContext* context, MSG_ThreadEntry* msg, void* closure)
{
  MSG_ThreadEntry* head;
  uint16 flags = *((uint16*) closure);
  for (head = context->msgframe->msgs->msgs ; head ; head = head->next) {
	if (msg_is_descendent(head, msg)) {
	  msg_ChangeFlag(context, head, flags, 0);
	  msg_set_thread_flag_1(context, head->first_child, flags);
	  return TRUE;
	}
  }
  XP_ASSERT(0);
  return FALSE;
}

int
msg_MarkThreadsRead(MWContext* context)
{
  uint16 flags = MSG_FLAG_READ;
  msg_IterateOverSelectedMessages(context, msg_set_thread_flag, &flags);
  return msg_GotoMessage (context, MSG_NextUnreadMessage);
}



static MSG_ThreadEntry *
msg_find_next_1(MWContext* context, MSG_ThreadEntry* search,
				MSG_ThreadEntry* msg, XP_Bool* donext, XP_Bool marked_p)
{
  for ( ; msg ; msg = msg->next) {
	MSG_ThreadEntry *result;
	if (*donext &&
		(marked_p
		 ? ((msg->flags & MSG_FLAG_MARKED) && !(msg->flags & MSG_FLAG_EXPIRED))
		 : !(msg->flags & (MSG_FLAG_READ | MSG_FLAG_EXPIRED))))
	  return msg;

	if (msg == search) *donext = TRUE;
	result = msg_find_next_1(context, search, msg->first_child, donext,
									marked_p);
	if (result) return result;
	}
  return 0;
}

static MSG_ThreadEntry *
msg_find_next(MWContext* context, MSG_ThreadEntry* msg, XP_Bool marked_p)
{
  XP_Bool donext = (msg == NULL);
  return msg_find_next_1(context, msg, context->msgframe->msgs->msgs,
						 &donext, marked_p);
}

static XP_Bool
msg_find_prev_1(MWContext* context, MSG_ThreadEntry* search,
				MSG_ThreadEntry* msg, MSG_ThreadEntry** prev, XP_Bool marked_p)
{
  for ( ; msg ; msg = msg->next) {
	if (msg == search) return TRUE;
	if (marked_p
		? ((msg->flags & MSG_FLAG_MARKED) && !(msg->flags & MSG_FLAG_EXPIRED))
		: !(msg->flags & (MSG_FLAG_READ | MSG_FLAG_EXPIRED)))
	  *prev = msg;

	if (msg_find_prev_1(context, search, msg->first_child, prev, marked_p))
	  return TRUE;
  }
  return FALSE;
}

static MSG_ThreadEntry*
msg_find_prev(MWContext* context, MSG_ThreadEntry* msg, XP_Bool marked_p)
{
  MSG_ThreadEntry* prev = NULL;
  msg_find_prev_1(context, msg, context->msgframe->msgs->msgs, &prev,
				  marked_p);
  return prev;
}


MSG_ThreadEntry *
msg_FindNextMessage (MWContext *context, MSG_MOTION_TYPE type)
{
  MSG_ThreadEntry* msg;
  int n;
  XP_ASSERT (context->msgframe);
  if (!context->msgframe ||
	  !context->msgframe->msgs ||
	  !context->msgframe->msgs->msgs)
	return 0;
  msg = context->msgframe->displayed_message;
  switch (type) {
  case MSG_FirstMessage:
	msg = NULL;
	/* Fall through. */
  case MSG_NextMessage:
	if (msg) {
	  n = msg_FindMessage(context, msg, NULL);
	} else {
	  n = -1;
	}
	do {
	  n++;
	  msg = msg_FindMessageLine(context, n, NULL);
	  if (msg && !(msg->flags & MSG_FLAG_EXPIRED)) {
		return msg;
	  }
	} while (msg);
	break;
  case MSG_LastMessage:
	msg = NULL;
	/* Fall through. */
  case MSG_PreviousMessage:
	if (msg) {
	  n = msg_FindMessage(context, msg, NULL);
	} else {
	  n = msg_CountVisibleMessages(context->msgframe->msgs->msgs);
	}
	do {
	  n--;
	  msg = msg_FindMessageLine(context, n, NULL);
	  if (msg && !(msg->flags & MSG_FLAG_EXPIRED)) {
		return msg;
	  }
	} while (msg);
	break;

  case MSG_FirstUnreadMessage:
	return msg_find_next(context, NULL, FALSE);
  case MSG_LastUnreadMessage:
	return msg_find_prev(context, NULL, FALSE);
  case MSG_NextUnreadMessage:
	return msg_find_next(context, msg, FALSE);
  case MSG_PreviousUnreadMessage:
	return msg_find_prev(context, msg, FALSE);

  case MSG_FirstMarkedMessage:
	return msg_find_next(context, NULL, TRUE);
  case MSG_LastMarkedMessage:
	return msg_find_prev(context, NULL, TRUE);
  case MSG_NextMarkedMessage:
	return msg_find_next(context, msg, TRUE);
  case MSG_PreviousMarkedMessage:
	return msg_find_prev(context, msg, TRUE);
  default:
	XP_ASSERT(0);
	return 0;
  }
  return 0;
}

int
msg_GotoMessage (MWContext *context, MSG_MOTION_TYPE type)
{
  MSG_ThreadEntry* msg = msg_FindNextMessage (context, type);
  if (msg) {
	context->msgframe->focusMsg = msg;
	return msg_OpenMessage(context, msg);
  } else {
	return 0;
  }
}


int
msg_SetHeaders (MWContext *context, MSG_CommandType which)
{
  XP_Bool m, a;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;

  m = context->msgframe->micro_headers_p;
  a = context->msgframe->all_headers_visible_p;
  switch (which)
	{
	case MSG_ShowMicroHeaders:
	  context->msgframe->micro_headers_p = TRUE;
	  context->msgframe->all_headers_visible_p = FALSE;
	  break;
	case MSG_ShowSomeHeaders:
	  context->msgframe->micro_headers_p = FALSE;
	  context->msgframe->all_headers_visible_p = FALSE;
	  break;
	case MSG_ShowAllHeaders:
	  context->msgframe->micro_headers_p = FALSE;
	  context->msgframe->all_headers_visible_p = TRUE;
	  break;
	default:
	  XP_ASSERT(0);
	  return -1;
	  break;
	}

  if (a == context->msgframe->all_headers_visible_p &&
	  m == context->msgframe->micro_headers_p)
	return 0;

  return msg_ReOpenMessage (context);
}


void
MSG_SetPaneConfiguration (MWContext *context, MSG_PANE_CONFIG type)
{
  msg_InterruptContext(context, FALSE);
  XP_ASSERT (0);
}


MSG_FONT MSG_CitationFont = MSG_ItalicFont;
MSG_CITATION_SIZE MSG_CitationSize = MSG_NormalSize;
char* MSG_CitationColor = NULL;

void
MSG_SetCitationStyle (MWContext *context,
					  MSG_FONT font,
					  MSG_CITATION_SIZE size,
					  const char *color)
{
  msg_InterruptContext(context, FALSE);
  if (font == MSG_CitationFont &&
	  size == MSG_CitationSize &&
	  (color == MSG_CitationColor || (color != NULL &&
									  MSG_CitationColor != NULL &&
									  XP_STRCMP(color, MSG_CitationColor)==0)))
	return;

  MSG_CitationFont = font;
  MSG_CitationSize = size;
  FREEIF(MSG_CitationColor);
  MSG_CitationColor = color ? XP_STRDUP(color) : NULL;
  
  /* Reload the current message, if there is one. */
  if (context &&
	  (context->type == MWContextMail || context->type == MWContextNews))
	{
	  MSG_ThreadEntry *msg = context->msgframe->displayed_message;
	  char *url = (msg ? msg_GetMessageURL (context, msg) : 0);
	  if (url)
		msg_GetURL(context, NET_CreateURLStruct (url, NET_DONT_RELOAD), FALSE);
	}
}


XP_Bool MSG_VariableWidthPlaintext = TRUE;

void 
MSG_SetPlaintextFont (MWContext *context, XP_Bool fixed_width_p)
{
  msg_InterruptContext(context, FALSE);
  if (!fixed_width_p == MSG_VariableWidthPlaintext)
	return;
  MSG_VariableWidthPlaintext = !fixed_width_p;

  /* Reload the current message, if there is one. */
  if (context &&
	  (context->type == MWContextMail || context->type == MWContextNews))
	{
	  MSG_ThreadEntry *msg = context->msgframe->displayed_message;
	  char *url = (msg ? msg_GetMessageURL (context, msg) : 0);
	  if (url)
		msg_GetURL(context, NET_CreateURLStruct (url, NET_DONT_RELOAD), FALSE);
	}
}



static char *
msg_make_article_url (MWContext *context, MSG_ThreadEntry* msg,
					  const char *message_id)
{
  char *url = 0;
  char *n, *id;
  int length;
  if (!message_id) return NULL;

  /* Take off bracketing <>, and quote special characters. */
  if (message_id[0] == '<')
	{
	  char *i2;
	  id = XP_STRDUP(message_id + 1);
	  if (!id) return 0;
	  if (*id && id[XP_STRLEN(id) - 1] == '>')
		{
		  id[XP_STRLEN(id) - 1] = '\0';
		}
	  i2 = NET_Escape (id, URL_XALPHAS);
	  XP_FREE (id);
	  id = i2;
	}
  else
	{
	  id = NET_Escape (message_id, URL_XALPHAS);
	}

  switch (context->type)
	{
	case MWContextMail:
	  if (!context->msgframe->opened_folder)
		break;
	  n = context->msgframe->opened_folder->data.mail.pathname;
	  XP_ASSERT(n);
	  if (!n) break;

#ifndef XP_MAC /* #### Giant Evil Mac Pathname Hack */
	  n = NET_Escape (n, URL_PATH);
	  if (!n) break;
#endif /* !XP_MAC */

	  length = XP_STRLEN(n) + XP_STRLEN(id) + 50;
	  url = (char*)XP_ALLOC(length);
	  if (!url)
		break;
	  XP_STRCPY (url, "mailbox:");
	  XP_STRCAT (url, n);
	  XP_STRCAT (url, "?id=");
	  XP_STRCAT (url, id);

#ifndef XP_MAC /* #### Giant Evil Mac Pathname Hack */
	  XP_FREE (n);
#endif /* !XP_MAC */

	  if (msg && context->msgframe->msgs)
		{
		  MSG_ThreadEntry*** msglist = context->msgframe->msgs->msglist;
		  int j;
		  for (j=context->msgframe->msgs->message_count-1 ; j>=0 ; j--) {
			if (msglist[j/mail_MSG_ARRAY_SIZE][j%mail_MSG_ARRAY_SIZE] == msg) {
			  PR_snprintf(url + XP_STRLEN(url), length - XP_STRLEN(url),
						  "&number=%d", j);
			  break;
			}
		  }
		}
	  break;

	case MWContextNews:
	  XP_ASSERT (context->msgframe->data.news.current_host);
	  if (! context->msgframe->data.news.current_host)
		url = 0;
	  else
		url = msg_MakeNewsURL (context,
							   context->msgframe->data.news.current_host,
							   id);
	  break;

	default:
	  XP_ASSERT(0);
	  break;
	}

  XP_FREE(id);

  return url;
}


char *
msg_GetMessageURL (MWContext *context, MSG_ThreadEntry *msg)
{
  char *id;
  if (! msg) return 0;

  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return 0;

  if (!context->msgframe->msgs) return 0;
  id = context->msgframe->msgs->string_table [msg->id];
  return msg_make_article_url (context, msg, id);
}


int
msg_OpenMessage (MWContext *context, MSG_ThreadEntry *msg)
{
  char *url;
  URL_Struct *url_struct;
  int status = 0;
  if (! msg) return 0;

  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;

  if (msg == context->msgframe->displayed_message) return 0;

  msg_ClearMessageArea(context);
  if (msg->flags & MSG_FLAG_EXPIRED) {
	context->msgframe->displayed_message = NULL;
	context->msgframe->cacheinfo.valid = FALSE;
	msg_UpdateToolbar(context);
	return 0;
  }	

  url = msg_GetMessageURL (context, msg);
  if (!url) return 0;

  url_struct = NET_CreateURLStruct(url, NET_DONT_RELOAD);
  XP_FREE (url);
  if (!url_struct)
	{
	  return MK_OUT_OF_MEMORY;
	}

  context->msgframe->displayed_message = msg;
  status = msg_GetURL (context, url_struct, FALSE);

  if (status >= 0) {
	msg_ChangeFlag(context, msg, MSG_FLAG_READ, 0);
	msg_EnsureSelected(context, msg);
  } else {
	msg_ClearMessageArea(context); /* Which incidentally clears out
									  displayed_message. */
  }
 
  return status;
}


int
msg_ReOpenMessage(MWContext* context)
{
  MSG_ThreadEntry* msg = context->msgframe->displayed_message;
  if (msg) {
	msg_ClearMessageArea(context);
	return msg_OpenMessage(context, msg);
  } else {
	return 0;
  }
}

void
MSG_ToggleThreadExpansion (MWContext *context, int line_number)
{
  int depth;
  MSG_ThreadEntry* msg;
  msg_InterruptContext(context, FALSE);
  msg = msg_FindMessageLine(context, line_number, &depth);
  if (!msg) return;
  if (msg->first_child) {
	msg_ToggleFlag(context, msg, MSG_FLAG_ELIDED);
  } else {
	msg_ClearMessageArea(context);
	msg_OpenMessage(context, msg);
  }
}



void
MSG_SelectMessage(MWContext* context, int line_number, XP_Bool exclusive)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  msg = msg_FindMessageLine(context, line_number, NULL);
  if (!msg) return;
  msg_DisableUpdates(context);
  context->msgframe->focusMsg = msg;
  if (exclusive) {
	msg_UnselectAllMessages(context);
  }
  msg_ChangeFlag(context, msg, MSG_FLAG_SELECTED, 0);
  msg_ClearMessageArea(context);
  if (exclusive) {
	msg_OpenMessage(context, msg);
  }
  msg_UpdateToolbar(context);
  msg_EnableUpdates(context);
}

void
MSG_UnselectMessage(MWContext* context, int line_number)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  msg = msg_FindMessageLine(context, line_number, NULL);
  if (!msg) return;
  context->msgframe->focusMsg = msg; /* ### Is this right? */
  msg_ChangeFlag(context, msg, 0, MSG_FLAG_SELECTED);
  msg_ClearMessageArea(context);
  msg_UpdateToolbar(context);
}


void
MSG_ToggleSelectMessage(MWContext* context, int line_number)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  msg = msg_FindMessageLine(context, line_number, NULL);
  if (!msg) return;
  context->msgframe->focusMsg = msg;
  msg_ToggleFlag(context, msg, MSG_FLAG_SELECTED);
  msg_ClearMessageArea(context);
  msg_UpdateToolbar(context);
}


void
msg_select_range_to_1(MWContext* context, int min, int max, int* cur,
					  MSG_ThreadEntry* msg)
{
  for ( ; msg ; msg = msg->next) {
	if (*cur >= min && *cur <= max) {
	  msg_ChangeFlag(context, msg, MSG_FLAG_SELECTED, 0);
	} else {
	  if (msg->flags & MSG_FLAG_SELECTED) {
		msg_ChangeFlag(context, msg, 0, MSG_FLAG_SELECTED);
	  }
	}
	(*cur)++;
	if (!(msg->flags & MSG_FLAG_ELIDED)) {
	  msg_select_range_to_1(context, min, max, cur, msg->first_child);
	}
  }
}


void
MSG_SelectRangeTo(MWContext* context, int line_number)
{
  MSG_ThreadEntry* first;
  MSG_ThreadEntry* last;
  int firstline;
  int min;
  int max;
  int cur = 0;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  first = context->msgframe->lastSelectMsg;
  last = msg_FindMessageLine(context, line_number, NULL);
  if (!last) return;
  msg_DisableUpdates(context);
  context->msgframe->focusMsg = last;
  if (first == NULL) first = last;
  firstline = msg_FindMessage(context, first, NULL);
  if (firstline < 0) {
	/* Apparently the last selection has been folded away.  The heck with
	   it; just select this line.*/
	first = last;
	firstline = line_number;
  }
  min = firstline < line_number ? firstline : line_number;
  max = firstline > line_number ? firstline : line_number;
  if (min + 20 < max) {
	/* We're selecting a "lot" (whatever that means); mark everything for
	   redisplay as an optimization.  What a hack.  ### */
	msg_RedisplayMessages(context);
  }
  msg_select_range_to_1(context, min, max, &cur,
						context->msgframe->msgs->msgs);
  /* Make sure we aren't trying to make some weird thing visible by telling
	 ourselves to make visible the one the user just clicked on. */
  msg_RedisplayOneMessage(context, last, -1, -1, TRUE);
  context->msgframe->lastSelectMsg = first;
  if (context->msgframe->num_selected_msgs > 1) {
	msg_ClearMessageArea(context);
  }
  msg_EnableUpdates(context);
}

	

static void
msg_unselect_all_messages_1(MWContext* context, MSG_ThreadEntry* msg)
{
  for ( ; msg ; msg = msg->next) {
	if (msg->flags & MSG_FLAG_SELECTED)
	  msg_ChangeFlag(context, msg, 0, MSG_FLAG_SELECTED);
	if (msg->first_child)
	  msg_unselect_all_messages_1(context, msg->first_child);
  }
}


/* Unselects all messages. */
void
msg_UnselectAllMessages(MWContext* context)
{
  msg_DisableUpdates(context);
  if (context->msgframe->num_selected_msgs > 20) {
	/* We're unselecting a "lot" (whatever that means); mark everything for
	   redisplay as an optimization.  What a hack.  ### */
	msg_RedisplayMessages(context);
  }
  msg_unselect_all_messages_1(context, context->msgframe->msgs->msgs);
  XP_ASSERT(context->msgframe->num_selected_msgs == 0);
  context->msgframe->num_selected_msgs = 0;
  msg_UpdateToolbar(context);
  msg_EnableUpdates(context);
}


static void
msg_select_all_messages_1(MWContext* context, MSG_ThreadEntry* msg)
{
  for ( ; msg ; msg = msg->next) {
	msg_ChangeFlag(context, msg, MSG_FLAG_SELECTED, 0);
	msg_select_all_messages_1(context, msg->first_child);
  }
}


/* Selects all messages. */
void
msg_SelectAllMessages(MWContext* context)
{
  msg_DisableUpdates(context);
  msg_RedisplayMessages(context);
  msg_select_all_messages_1(context, context->msgframe->msgs->msgs);
  msg_EnableUpdates(context);
}


static void
msg_select_thread_1(MWContext* context, MSG_ThreadEntry* msg) {
  uint16 flags = MSG_FLAG_SELECTED;
  for ( ; msg ; msg = msg->next) {
	if (msg->flags & MSG_FLAG_SELECTED) {
	  msg_set_thread_flag(context, msg, &flags);
	}
	msg_select_thread_1(context, msg->first_child);
  }
}

/* Selects the thread containing the selected messages.  Note that we want
   this to work even if it's a dummy message that is selected, so we can't use
   msg_IterateOverSelectedMessages. */
int
msg_SelectThread(MWContext* context)
{
  if (!context->msgframe->msgs) {
	return -1;
  }
  msg_select_thread_1(context, context->msgframe->msgs->msgs);
  if (context->msgframe->num_selected_msgs > 1) {
	msg_ClearMessageArea(context);
  }
  return 0;
}


static XP_Bool
msg_select_marked_messages_1(MWContext* context, MSG_ThreadEntry* msg,
							 void* closure)
{
  if (msg->flags & MSG_FLAG_MARKED) {
	msg_ChangeFlag(context, msg, MSG_FLAG_SELECTED, 0);
  }
  return TRUE;
}

/* Selects all the messages that have the mark set. */
int
msg_SelectMarkedMessages(MWContext* context)
{
  msg_UnselectAllMessages(context);
  msg_IterateOverAllMessages(context, msg_select_marked_messages_1, NULL);
  return 0;
}


static XP_Bool
msg_mark_selected_messages_1(MWContext* context, MSG_ThreadEntry* msg,
							 void* closure)
{
  if (*((XP_Bool*) closure)) {
	msg_ChangeFlag(context, msg, MSG_FLAG_MARKED, 0);
  } else {
	msg_ChangeFlag(context, msg, 0, MSG_FLAG_MARKED);
  }
  return TRUE;
}


/* Marks all the messages that are selected. */
int
msg_MarkSelectedMessages(MWContext* context, XP_Bool mark)
{
  msg_IterateOverSelectedMessages(context, msg_mark_selected_messages_1,
								  &mark);
  return 0;
}

int
MSG_GetNumSelectedMessages(MWContext* context)
{
  return context->msgframe->num_selected_msgs;
}




void
MSG_ToggleMark(MWContext* context, int line_number)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  msg = msg_FindMessageLine(context, line_number, NULL);
  msg_ToggleFlag(context, msg, MSG_FLAG_MARKED);
}

int
msg_MarkMessage (MWContext *context, XP_Bool marked_p)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;
  msg = context->msgframe->displayed_message;
  if (!msg) return -1;
  if (marked_p)
	return msg_ChangeFlag(context, msg, MSG_FLAG_MARKED, 0);
  else
	return msg_ChangeFlag(context, msg, 0, MSG_FLAG_MARKED);
}



static XP_Bool
msg_mark_selected_read_1(MWContext* context, MSG_ThreadEntry* msg,
						 void* closure)
{
  if (*((XP_Bool*)closure)) {
	msg_ChangeFlag(context, msg, MSG_FLAG_READ, 0);
  } else {
	msg_ChangeFlag(context, msg, 0, MSG_FLAG_READ);
  }
  return TRUE;
}


int
msg_MarkSelectedRead (MWContext *context, XP_Bool read_p)
{
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;
  msg_IterateOverSelectedMessages(context, msg_mark_selected_read_1, &read_p);
  return 0;
}


void
MSG_ToggleRead(MWContext* context, int line_number)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  msg = msg_FindMessageLine(context, line_number, NULL);
  msg_ToggleFlag(context, msg, MSG_FLAG_READ);
}


static XP_Bool
msg_ensure_unelided_1(MWContext* context, MSG_ThreadEntry* search,
					  MSG_ThreadEntry* msg, int depth, XP_Bool visible,
					  MSG_ThreadEntry*** list) 
{
  for ( ; msg ; msg = msg->next) {
	if (msg == search) {
	  XP_ASSERT(*list == NULL);
	  if (!visible) {
		*list = (MSG_ThreadEntry**) XP_ALLOC((depth + 1) *
											  sizeof(MSG_ThreadEntry*));
		if (*list) (*list)[depth] = NULL;
	  }
	  return TRUE;
	}
	if (msg_ensure_unelided_1(context, search, msg->first_child, depth + 1,
							  visible && ((msg->flags & MSG_FLAG_ELIDED) == 0),
							  list)) {
	  if (*list) (*list)[depth] = msg;
	  return TRUE;
	}
  }
  return FALSE;
}


static void
msg_ensure_unelided(MWContext* context, MSG_ThreadEntry* msg)
{
  MSG_ThreadEntry** list = NULL;
  XP_ASSERT(context->msgframe && context->msgframe->msgs);
  if (!context->msgframe || !context->msgframe->msgs) return;
  msg_ensure_unelided_1(context, msg, context->msgframe->msgs->msgs,
						0, TRUE, &list);
  if (list) {
	MSG_ThreadEntry** tmp;
	for (tmp = list ; *tmp ; tmp++) {
	  msg_ChangeFlag(context, msg, 0, MSG_FLAG_ELIDED);
	}
	XP_FREE(list);
  }
}



void
msg_EnsureSelected(MWContext* context, MSG_ThreadEntry* msg)
{
  if (!msg) return;
  
  msg_ensure_unelided(context, msg);

  if (!(msg->flags & MSG_FLAG_SELECTED)) {
	msg_UnselectAllMessages(context);
	msg_ChangeFlag(context, msg, MSG_FLAG_SELECTED, 0);
	msg_OpenMessage(context, msg);
	msg_UpdateToolbar(context);
  }
}
	

int
msg_MarkMailArticleRead (MWContext *context, const char *message_id)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT (context && context->msgframe && context->type == MWContextMail
			 && message_id);
  if (!context || !context->msgframe || context->type != MWContextMail ||
	  !message_id)
	return -1;

  if (!context->msgframe->msgs)
	return 0;

  msg = (MSG_ThreadEntry*)XP_Gethash (context->msgframe->msgs->message_id_table, message_id, 0);
  if (!msg)
	return 0;

  msg_ChangeFlag(context, msg, MSG_FLAG_READ, 0);

  return 0;
}


int 
MSG_MarkMessageIDRead (MWContext *context, const char *message_id,
					   const char *xref)
{
  switch (context->type)
    {
    case MWContextMail:
	  return msg_MarkMailArticleRead (context, message_id);
	  break;
	case MWContextNews:
	  return msg_MarkNewsArticleRead (context, message_id, xref);
	  break;
	case MWContextBrowser:
	  return 0;
	  break;
	default:
	  XP_ASSERT (0);
	  return -1;
	  break;
	}
}


void
msg_ClearMessageArea (MWContext *context)
{
#ifndef _USRDLL
  /* This is a moderately nasty kludge, and I'm not totally convinced
	 it won't break something...  Open a stream to layout, give it
	 whitespace (it needs something, or it bombs) and then close it
	 right away.  This has the effect of getting the front end to
	 clear out the HTML display area.
   */
  NET_StreamClass *stream;
  static PA_InitData data;
  URL_Struct *url;
  if (!context || !context->msgframe)
	return;
  context->msgframe->displayed_message = 0;
  context->msgframe->cacheinfo.valid = FALSE;

  data.output_func = LO_ProcessTag;
  url = NET_CreateURLStruct ("", NET_NORMAL_RELOAD);
  if (!url) return;
  stream = PA_BeginParseMDL (FO_PRESENT, &data, url, context);
  if (stream)
	{
	  char buf[] = "\n";
	  int status = (*stream->put_block) (stream->data_object, buf, 1);
	  if (status < 0)
		(*stream->abort) (stream->data_object, status);
	  else
		(*stream->complete) (stream->data_object);
	  XP_FREE (stream);
	}
  NET_FreeURLStruct (url);
  FE_SetProgressBarPercent (context, 0); /* ugh... */

  MSG_ActivateReplyOptions (context, 0);
  msg_UpdateToolbar(context);

#endif
}

void
msg_ClearThreadList (MWContext *context)
{
  if (!context || !context->msgframe)
	return;
  msg_ClearMessageArea (context);
  if (context->type == MWContextMail) {
	msg_SaveMailSummaryChangesNow(context);
  } else if (context->type == MWContextNews) {
	FREEIF(context->msgframe->data.news.knownarts.host_and_port);
  } else {
	XP_ASSERT(0);
  }
  if (context->msgframe->msgs)
	{
	  msg_FreeFolderData (context->msgframe->msgs);
	  context->msgframe->msgs = 0;
	  context->msgframe->num_selected_msgs = 0;
	  msg_RedisplayMessages (context);
	}
  msg_UpdateToolbar(context);
}


/* After a message has been displayed (by mimehtml.c) this is called with
   the header contents so that we can put the appropriate actions in the
   Message menu.
 */


#ifdef XP_MAC
#pragma push
#pragma global_optimizer on
#endif

#ifdef DEBUG_jwz
const char * FE_UsersMailAddresses (void);
#endif


static char *
make_mailto(const char *to, const char *cc, const char *newsgroups,
			const char *subject, const char *references,
			const char *attachment, const char *host_data,
			XP_Bool encrypt_p, XP_Bool sign_p)
{
  char *to2 = 0, *cc2 = 0;
  char *out, *head;
  char *qto, *qcc, *qnewsgroups, *qsubject, *qreferences;
  char *qattachment, *qhost_data;
  char *url;
  char *me = MIME_MakeFromField();
  char *to_plus_me = 0;

#ifdef DEBUG_jwz
  /* This massive suckage is so that I can have N different email addresses,
	 and have both of them exhibit the "don't reply to myself" behavior of
	 Reply-All described below.
   */
  const char *me_too = FE_UsersMailAddresses();
# define KLUDGE XP_STRLEN(me_too)
#else  /* !DEBUG_jwz */
# define KLUDGE 0
#endif /* !DEBUG_jwz */

  to2 = MSG_RemoveDuplicateAddresses (to, ((cc && *cc) ? me : 0));
  if (to2 && !*to2) XP_FREE (to2), to2 = 0;

  /* This to_plus_me business is so that, in reply-to-all of a message
	 to which I was a recipient, I don't go into the CC field (that's
	 what BCC/FCC are for.) */
  if (to2 &&
#ifndef DEBUG_jwz
      cc &&
#endif /* !DEBUG_jwz */
      me)
	to_plus_me = (char *) XP_ALLOC(XP_STRLEN(to2) + XP_STRLEN(me) +
								   KLUDGE + 10);
  if (to_plus_me)
	{
	  XP_STRCPY(to_plus_me, me);
	  XP_STRCAT(to_plus_me, ", ");
#ifdef DEBUG_jwz
	  XP_STRCAT(to_plus_me, me_too);
	  XP_STRCAT(to_plus_me, ", ");
#endif /* DEBUG_jwz */
	  XP_STRCAT(to_plus_me, to2);
	}
  FREEIF(me);

  cc2 = MSG_RemoveDuplicateAddresses (cc, (to_plus_me ? to_plus_me : to2));
  if (cc2 && !*cc2) XP_FREE (cc2), cc2 = 0;

  FREEIF(to_plus_me);

  /* Catch the case of "Reply To All" on a message that was from me.
	 In that case, we've got an empty To: field at this point.
	 What we should do is, promote the first CC address to the To:
	 field.  But I'll settle for promoting all of them.
   */
  if (cc2 && *cc2 && (!to2 || !*to2))
	{
	  FREEIF(to2);
	  to2 = cc2;
	  cc2 = 0;
	}

  qto		  = to2		   ? NET_Escape (to2, URL_XALPHAS)		  : 0;
  qcc		  = cc2		   ? NET_Escape (cc2, URL_XALPHAS)		  : 0;
  qnewsgroups = newsgroups ? NET_Escape (newsgroups, URL_XALPHAS) : 0;
  qsubject	  = subject    ? NET_Escape (subject, URL_XALPHAS)    : 0;
  qreferences = references ? NET_Escape (references, URL_XALPHAS) : 0;
  qattachment = attachment ? NET_Escape (attachment, URL_XALPHAS) : 0;
  qhost_data  = host_data  ? NET_Escape (host_data, URL_XALPHAS)  : 0;

  url = (char *)
	XP_ALLOC ((qto         ? XP_STRLEN(qto)			+ 15 : 0) +
			  (qcc         ? XP_STRLEN(qcc)			+ 15 : 0) +
			  (qnewsgroups ? XP_STRLEN(qnewsgroups) + 15 : 0) +
			  (qsubject    ? XP_STRLEN(qsubject)	+ 15 : 0) +
			  (qreferences ? XP_STRLEN(qreferences) + 15 : 0) +
			  (qhost_data  ? XP_STRLEN(qhost_data)  + 15 : 0) +
			  (qattachment ? XP_STRLEN(qattachment) + 15 : 0) +
			  60);
  if (!url) goto FAIL;
  XP_STRCPY (url, "mailto:");
  head = url + XP_STRLEN (url);
  out = head;
# define PUSH_STRING(S) XP_STRCPY(out, S), out += XP_STRLEN(S)
# define PUSH_PARM(prefix,var)			\
  if (var)								\
	{									\
	  if (out == head)					\
		*out++ = '?';					\
	  else								\
		*out++ = '&';					\
	  PUSH_STRING (prefix);				\
	  *out++ = '=';						\
	  PUSH_STRING (var);				\
	}
  PUSH_PARM("to", qto);
  PUSH_PARM("cc", qcc);
  PUSH_PARM("newsgroups", qnewsgroups);
  PUSH_PARM("subject", qsubject);
  PUSH_PARM("references", qreferences);
  PUSH_PARM("attachment", qattachment);
  PUSH_PARM("newshost", qhost_data);
  if (encrypt_p)
	PUSH_PARM("encrypt", "true");
  if (sign_p)
	PUSH_PARM("sign", "true");
# undef PUSH_PARM
# undef PUSH_STRING

 FAIL:
  FREEIF (to2);
  FREEIF (cc2);
  FREEIF (qto);
  FREEIF (qcc);
  FREEIF (qnewsgroups);
  FREEIF (qsubject);
  FREEIF (qreferences);
  FREEIF (qattachment);
  FREEIF (qhost_data);

  return url;
}

#ifdef XP_MAC
#pragma pop
#endif



static char *
msg_compute_newshost_arg (MWContext *context)
{
  /* Compute a value for the "?newshost=" parameter.  This gets passed in with
	 all of the mailto's, even the ones that don't have to do with posting,
	 because the user might decide later to type in some newsgroups, and we
	 need to know which host to send them to.  Perhaps the UI should have a
	 way of selecting the host - say you wanted to forward a message frome
	 one news server to another.  But it doesn't now.
   */
  uint32 port;
  char *host;
  int length;
  if (!context->msgframe ||
	  context->type != MWContextNews ||
	  !context->msgframe->data.news.current_host)
	return 0;

  port = context->msgframe->data.news.current_host->port;
  if (port == (context->msgframe->data.news.current_host->secure_p
			   ? SECURE_NEWS_PORT : NEWS_PORT))
	port = 0;

  length = XP_STRLEN(context->msgframe->data.news.current_host->name) + 20;
  host = (char *) XP_ALLOC (length);
  if (!host)
	return 0; /* #### MK_OUT_OF_MEMORY */
  XP_STRCPY (host, context->msgframe->data.news.current_host->name);
  if (port)
	PR_snprintf (host + XP_STRLEN(host), length - XP_STRLEN(host),
				 ":%lu", port);
  if (context->msgframe->data.news.current_host->secure_p)
	XP_STRCAT (host, "/secure");
  return host;
}


static char *
msg_get_1522_header(MWContext *context, MimeHeaders *headers,
					const char *name, XP_Bool all_p)
{
  char *contents = MimeHeaders_get (headers, name, FALSE, all_p);
  if (contents && *contents)
	{
	  char *converted = IntlDecodeMimePartIIStr(contents, context->win_csid,
												FALSE);
	  if (converted)
		{
		  XP_FREE(contents);
		  contents = converted;
		}
	}
  return contents;
}


void
MSG_ActivateReplyOptions (MWContext *context, MimeHeaders *headers)
{
  char *host = 0;
  char *to_and_cc = 0;
  char *re_subject = 0;
  char *new_refs = 0;

  char *from = 0;
  char *repl = 0;
  char *subj = 0;
  char *id = 0;
  char *refs = 0;
  char *to = 0;
  char *cc = 0;
  char *grps = 0;
  char *foll = 0;

  XP_Bool encrypt_p = FALSE;	/* initialized later */
  XP_Bool sign_p    = FALSE;	/* initialized later */

  if (!context || !context->msgframe)
	/* This happens when a mail or news message is displayed in
	   a web browser window (which is legal.) */
	return;

  FREEIF (context->msgframe->mail_to_sender_url);
  FREEIF (context->msgframe->mail_to_all_url);
  FREEIF (context->msgframe->post_reply_url);
  FREEIF (context->msgframe->post_and_mail_url);
  FREEIF (context->msgframe->displayed_message_subject);
  FREEIF (context->msgframe->displayed_message_id);

  if (headers)
	{
	  from = msg_get_1522_header (context, headers, HEADER_FROM,     FALSE);
	  repl = msg_get_1522_header (context, headers, HEADER_REPLY_TO, FALSE);
	  subj = msg_get_1522_header (context, headers, HEADER_SUBJECT,  FALSE);
	  to   = msg_get_1522_header (context, headers, HEADER_TO,       TRUE);
	  cc   = msg_get_1522_header (context, headers, HEADER_CC,       TRUE);

	  /* These headers should not be RFC-1522-decoded. */
	  grps = MimeHeaders_get (headers, HEADER_NEWSGROUPS,  FALSE, TRUE);
	  foll = MimeHeaders_get (headers, HEADER_FOLLOWUP_TO, FALSE, TRUE);
	  id   = MimeHeaders_get (headers, HEADER_MESSAGE_ID,  FALSE, FALSE);
	  refs = MimeHeaders_get (headers, HEADER_REFERENCES,  FALSE, TRUE);

	  if (repl)
		{
		  FREEIF(from);
		  from = repl;
		  repl = 0;
		}

	  if (foll)
		{
		  FREEIF(grps);
		  grps = foll;
		  foll = 0;
		}

	  /* Set defaults for encryption and signing based on the type of the
		 message.
	   */
	  if (!grps || !*grps)				/* it's a mail message */
		{
#ifdef HAVE_SECURITY /* added by jwz */
		  if (! encrypt_p) encrypt_p = MSG_GetMailEncryptionPreference();
		  if (! sign_p)    sign_p    = MSG_GetMailSigningPreference();
#endif /* !HAVE_SECURITY -- added by jwz */
		}
	  else								/* it's a news message */
		{
		  encrypt_p = FALSE;
#ifdef HAVE_SECURITY /* added by jwz */
		  if (! sign_p) sign_p = MSG_GetNewsSigningPreference();
#endif /* !HAVE_SECURITY -- added by jwz */
		}


	  /* Find out whether the message to which we're replying was itself signed
		 and/or encrypted.  If the message was encrypted, then default to
		 sending an encrypted reply (regardless of what the preference for the
		 default state for encryption is.)

		 However, don't do the same thing for signing: while an encrypted
		 message deserves an encrypted reply, a signed message doesn't
		 necessarily deserve a signed reply (especially given that signed
		 messages could turn out to be legally binding.)
	   */
	  {
		XP_Bool msg_encrypted_p = FALSE;
		XP_Bool msg_signed_p = FALSE;
		MIME_GetMessageCryptoState (context, &msg_signed_p, &msg_encrypted_p,
									0, 0);
		if (msg_encrypted_p)
		  encrypt_p = TRUE;
	  }


	  /* Actually, I've decided that the following is not the right thing to
		 do.  If someone gets an encrypted message, and hits reply, the default
		 should be to try and send an encrypted reply.  If, for some reason, an
		 encrypted reply cannot be sent, then the user should be informed of
		 that, and given a chance to fix the problem.
	   */
#if 0
	  /* Find out if we're able to sign outgoing mail; if we're not, then don't
		 try to sign this message, regardless of what the preferences are.

		 Find out if we're able to encrypt to the set of recipients of this
		 message (assuming `Reply All') and don't try to encrypt this message
		 if we can't, regardless of what the preferences are.
	   */
	  if (sign_p || encrypt_p)
		{
		  XP_Bool can_encrypt_p = FALSE;
		  XP_Bool can_sign_p = FALSE;
		  SEC_GetMessageCryptoViability(from,
										0, /* #### find *my* reply_to
											  (what FE called
											  MSG_SetDefaultHeaderContents
											  with)
											*/
										to,
										cc,
										0, /* #### find *my* bcc
											  (what FE called
											  MSG_SetDefaultHeaderContents
											  with)
											*/
										grps,
										0, /* followup_to: no default value */
										&can_sign_p,
										&can_encrypt_p);
		  if (!can_sign_p)
			sign_p = FALSE;
		  if (!can_encrypt_p)
			encrypt_p = FALSE;
		}
#endif /* 0 */
	}

  if (context->type == MWContextNews)
	{
	  /* Decide whether cancelling this message should be allowed. */
	  context->msgframe->data.news.cancellation_allowed_p = FALSE;
	  if (from)
		{
		  char *us = MSG_MakeFullAddress (NULL, FE_UsersMailAddress ());
		  char *them = MSG_ExtractRFC822AddressMailboxes (from);
		  context->msgframe->data.news.cancellation_allowed_p =
			(us && them && !strcasecomp (us, them));
		  FREEIF(us);
		  FREEIF(them);
		}
	}

  if (!headers)
	return;

  if (subj) context->msgframe->displayed_message_subject = XP_STRDUP(subj);
  if (id)   context->msgframe->displayed_message_id      = XP_STRDUP(id);

  host = msg_compute_newshost_arg (context);

  if (to || cc)
	{
	  to_and_cc = (char *)
		XP_ALLOC ((to ? XP_STRLEN (to) : 0) +
				  (cc ? XP_STRLEN (cc) : 0) +
				  5);
	  if (!to_and_cc) goto FAIL;
	  *to_and_cc = 0;
	  if (to) XP_STRCPY (to_and_cc, to);
	  if (to && *to &&
		  cc && *cc)
		XP_STRCAT (to_and_cc, ", ");
	  if (cc) XP_STRCAT (to_and_cc, cc);
	}

  if (id || refs)
	{
	  new_refs = (char *)
		XP_ALLOC ((id   ? XP_STRLEN (id)   : 0) +
				  (refs ? XP_STRLEN (refs) : 0) +
				  5);
	  if (!new_refs) goto FAIL;
	  *new_refs = 0;
	  if (refs) XP_STRCPY (new_refs, refs);
	  if (refs && id)
		XP_STRCAT (new_refs, " ");
	  if (id) XP_STRCAT (new_refs, id);
	}

  if (from || grps || to_and_cc)
	{
	  const char *s = subj;
	  if (s)
		{
		  uint32 L = XP_STRLEN(s);
		  msg_StripRE(&s, &L);
		  XP_ASSERT(L == XP_STRLEN(s));
		}
	  re_subject = (char *) XP_ALLOC((s ? XP_STRLEN (s) : 0) + 10);
	  if (!re_subject) goto FAIL;
	  XP_STRCPY (re_subject, "Re: ");
	  if (s) XP_STRCAT (re_subject, s);
	}

  if (from)
	context->msgframe->mail_to_sender_url =
	  make_mailto (from, 0, 0, re_subject, new_refs, 0, host,
				   encrypt_p, sign_p);

  if (from || to_and_cc)
	context->msgframe->mail_to_all_url =
	  make_mailto (from, to_and_cc, 0, re_subject, new_refs, 0, host,
				   encrypt_p, sign_p);

  if (grps)
	{
	  if (!strcasecomp (grps, "poster"))
		{
		  /* "Followup-to: poster" makes "post reply" act the same as
			 "reply to sender". */
		  if (context->msgframe->mail_to_sender_url)
			context->msgframe->post_reply_url =
			  XP_STRDUP (context->msgframe->mail_to_sender_url);
		}
	  else
		context->msgframe->post_reply_url =
		  make_mailto (0, 0, grps, re_subject, new_refs, 0, host,
					   encrypt_p, sign_p);
	}

  if (from || grps)
	{
	  if (grps && !strcasecomp (grps, "poster"))
		{
		  /* "Followup-to: poster" makes "post and mail reply" act the
			 same as "reply to sender". */
		  if (context->msgframe->mail_to_sender_url)
			context->msgframe->post_and_mail_url =
			  XP_STRDUP (context->msgframe->mail_to_sender_url);
		}
	  else
		context->msgframe->post_and_mail_url =
		  make_mailto (from, 0, grps, re_subject, new_refs, 0, host,
					   encrypt_p, sign_p);
	}

 FAIL:
  FREEIF (host);
  FREEIF (to_and_cc);
  FREEIF (re_subject);
  FREEIF (new_refs);
  FREEIF(from);
  FREEIF(repl);
  FREEIF(subj);
  FREEIF(id);
  FREEIF(refs);
  FREEIF(to);
  FREEIF(cc);
  FREEIF(grps);
  FREEIF(foll);
}


static XP_Bool
msg_find_first_message_to_forward_1(MWContext* context, MSG_ThreadEntry *msg,
									void *closure)
{
  MSG_ThreadEntry **msgP = (MSG_ThreadEntry **) closure;
  *msgP = msg;
  return FALSE;
}

static MSG_ThreadEntry *
msg_find_first_message_to_forward(MWContext *context)
{
  MSG_ThreadEntry *msg = 0;
  msg_IterateOverSelectedMessages (context,
								   msg_find_first_message_to_forward_1, &msg);
  return msg;
}


static char *
make_forward_message_url(MWContext *context, XP_Bool quote_original)
{
  char *url = 0;
  char *host = msg_compute_newshost_arg (context);
  char *fwd_subject = 0;
  const char *id = 0;
  const char *subject = 0;
  char *conv_subject = 0;
  MSG_ThreadEntry *msg = msg_find_first_message_to_forward (context);

#ifdef HAVE_SECURITY /* added by jwz */
  XP_Bool encrypt_p = MSG_GetMailEncryptionPreference();
  XP_Bool sign_p    = MSG_GetMailSigningPreference();
#else
  XP_Bool encrypt_p = FALSE;
  XP_Bool sign_p    = FALSE;
#endif /* !HAVE_SECURITY -- added by jwz */

  /* #### if the message which we are forwarding was encrypted, then
	 #### encrypt_p = TRUE; */


  if (msg)
	{
	  subject = context->msgframe->msgs->string_table [msg->subject];
	  id = context->msgframe->msgs->string_table [msg->id];
	}
  else
	{
	  subject = context->msgframe->displayed_message_subject;
	  id = context->msgframe->displayed_message_id;
	  if (subject)
		while (XP_IS_SPACE(*subject))
		  subject++;
	}

  conv_subject = IntlDecodeMimePartIIStr(subject, context->win_csid, FALSE);
  if (conv_subject == NULL)
  	  conv_subject = (char *) subject;
  fwd_subject = (char *) XP_ALLOC((conv_subject ? XP_STRLEN(conv_subject) : 0) + 20);
  if (!fwd_subject) goto FAIL;

  XP_STRCPY (fwd_subject, "[Fwd: ");
  if (msg && (msg->flags & MSG_FLAG_HAS_RE))
	XP_STRCAT (fwd_subject, "Re: ");
  if (conv_subject)
	XP_STRCAT (fwd_subject, conv_subject);
  XP_STRCAT (fwd_subject, "]");

  if (context->msgframe->num_selected_msgs > 1)
	{
	  url = make_mailto (0, 0, 0, fwd_subject, 0, "(digest)", host,
						 encrypt_p, sign_p);
	}
  else
	{
      char *attachment = 0;
      if (id)
	    attachment = msg_make_article_url (context, msg, id);

      /* if we are quoting the original document, slip a cookie string in
         at the beginning of the attachment field.  This will be removed
         before processing. */

      if (quote_original && attachment) 
        {
           char * new_attachment = PR_smprintf(MSG_FORWARD_COOKIE "%s",attachment);
           FREEIF(attachment);
           attachment = new_attachment;
        }

	  url = make_mailto (0, 0, 0, fwd_subject, 0, attachment, host,
						 encrypt_p, sign_p);
	  FREEIF (attachment);
	}

 FAIL:
  FREEIF (host);
  FREEIF (fwd_subject);
  if (conv_subject != subject)
  	FREEIF(conv_subject);

  return url;
}


static char*
msg_find_selected_newsgroups(MWContext* context, MSG_Folder* folder, char *url)
{
	struct MSG_Folder* curFolder = folder;

    /* check if we ended up here with no folder */
	if (!folder) {
		return url;
	}

	/* check if any first level children are selected newsgroups */
	for (curFolder = folder->children; curFolder ; curFolder = curFolder->next) {
		if ((curFolder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_DIRECTORY) &&
			(curFolder->flags & MSG_FOLDER_FLAG_SELECTED) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) {
                if (url) 
                    url = XP_AppendStr (url, ",");
				url = XP_AppendStr (url, curFolder->data.newsgroup.group_name);
		}
		if ((curFolder->flags & MSG_FOLDER_FLAG_DIRECTORY) &&
			(curFolder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_ELIDED) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) {
				url = msg_find_selected_newsgroups(context, curFolder, url);
		}

	}

	/* go to the next host if we need to */
	for (curFolder = folder->next; curFolder; curFolder = curFolder->next) {
		if 	(!(curFolder->flags & MSG_FOLDER_FLAG_ELIDED) &&
			(curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST))
			url = msg_find_selected_newsgroups(context, curFolder, url);
	}  
	return url;
}



int
msg_ComposeMessage (MWContext *context, MSG_CommandType type)
{
  char *host = 0;
  char *url = 0;
  char *group = 0;
  XP_Bool free_url_p = FALSE;
  URL_Struct *url_struct = 0;
  const char *current_newsgroup = 0;

  XP_Bool encrypt_p = FALSE;	/* initialized later */
  XP_Bool sign_p    = FALSE;	/* initialized later */

  if (context && context->msgframe)
	switch (type)
	  {
	  case MSG_MailNew:
		host = msg_compute_newshost_arg (context);
#ifdef HAVE_SECURITY /* added by jwz */
		encrypt_p = MSG_GetMailEncryptionPreference();
		sign_p    = MSG_GetMailSigningPreference();
#endif /* !HAVE_SECURITY -- added by jwz */
		url = make_mailto (0, 0, 0, 0, 0, 0, host, encrypt_p, sign_p);
		free_url_p = TRUE;
		break;
	  case MSG_PostNew:
		if (context->type != MWContextNews)
		  break;
		  current_newsgroup =
			msg_find_selected_newsgroups(context,context->msgframe->folders, group);

		host = msg_compute_newshost_arg (context);
		encrypt_p = FALSE;
#ifdef HAVE_SECURITY /* added by jwz */
		sign_p    = MSG_GetNewsSigningPreference();
#endif /* !HAVE_SECURITY -- added by jwz */
		if ( current_newsgroup ) {
		  url = make_mailto (0, 0, current_newsgroup, 0, 0, 0, host,
							 encrypt_p, sign_p);
		}
		else {
		  url = make_mailto (0, 0, "", 0, 0, 0, host, encrypt_p, sign_p);
		}
		free_url_p = TRUE;
		break;
	  case MSG_ReplyToSender:
		url = context->msgframe->mail_to_sender_url;
		break;
	  case MSG_ReplyToAll:
		url = context->msgframe->mail_to_all_url;
		break;
	  case MSG_PostReply:
		url = context->msgframe->post_reply_url;
		break;
	  case MSG_PostAndMailReply:
		url = context->msgframe->post_and_mail_url;
		break;
	  case MSG_ForwardMessage:
		url = make_forward_message_url (context, FALSE);
		free_url_p = TRUE;
		break;
      case MSG_ForwardMessageQuoted:
        /* last flag indicates to quote the original message. */
		url = make_forward_message_url (context, TRUE);
		free_url_p = TRUE;
		break;
	  default:
		XP_ASSERT (0);
		break;
	  }

  FREEIF (host);
  if (!url)
	{
	  url = "mailto:";
	  free_url_p = FALSE;
	}
  url_struct = NET_CreateURLStruct (url, NET_NORMAL_RELOAD);
  if (free_url_p && url)
	XP_FREE (url);
  if (!url_struct) return MK_OUT_OF_MEMORY;
  url_struct->internal_url = TRUE;
  msg_GetURL (context, url_struct, FALSE);

  return 0;
}



static void
msg_recount_selected_messages_1(MWContext* context, MSG_ThreadEntry* msg)
{
  for (; msg ; msg = msg->next) {
	if (msg->flags & MSG_FLAG_SELECTED) {
	  context->msgframe->num_selected_msgs++;
	}
	msg_recount_selected_messages_1(context, msg->first_child);
  }
}

void
msg_ReCountSelectedMessages(MWContext* context)
{
  XP_ASSERT(context && context->msgframe);
  if (context && context->msgframe && context->msgframe->msgs) {
	context->msgframe->num_selected_msgs = 0;
	msg_recount_selected_messages_1(context, context->msgframe->msgs->msgs);
  }
}


static XP_Bool
msg_find_check(MWContext* context, MSG_ThreadEntry* msg)
{
  char** table = context->msgframe->msgs->string_table;
  const char* search = context->msgframe->searchstring;
  if (msg->flags & MSG_FLAG_EXPIRED) return FALSE;
  
  return (
  	INTL_FindMimePartIIStr(context->win_csid, context->msgframe->searchcasesensitive, 
  		table[msg->subject], search) ||
	INTL_FindMimePartIIStr(context->win_csid, context->msgframe->searchcasesensitive, 
		table[msg->sender], search) ||
	INTL_FindMimePartIIStr(context->win_csid, context->msgframe->searchcasesensitive, 
		table[msg->recipient], search) 
  );
}

MSG_ThreadEntry*
msg_find_pass1(MWContext* context, MSG_ThreadEntry* msg, XP_Bool* passedcur,
			   XP_Bool doOpenedFirst)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (!*passedcur) {
	  if (msg == context->msgframe->displayed_message) {
		*passedcur = TRUE;
		if (doOpenedFirst) {
		  if (msg_find_check(context, msg)) return msg;
		}

	  }
	} else {
	  if (msg_find_check(context, msg)) return msg;
	}
	result = msg_find_pass1(context, msg->first_child, passedcur,
							doOpenedFirst);
	if (result) return result;
  }
  return NULL;
}


MSG_ThreadEntry*
msg_find_pass2(MWContext* context, MSG_ThreadEntry* msg)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (msg == context->msgframe->displayed_message) {
	  return msg;				/* Special case, really means we hit the end
								   of the search. */
	}
	if (msg_find_check(context, msg)) return msg;
	result = msg_find_pass2(context, msg->first_child);
	if (result) return result;
  }
  return NULL;
}


int
msg_Find(MWContext* context, XP_Bool doOpenedFirst)
{
  XP_Bool passedcur = FALSE;
  MSG_ThreadEntry* msg = NULL;
  if (context->msgframe->msgs && context->msgframe->searchstring) {
	if (context->msgframe->displayed_message) {
	  msg = msg_find_pass1(context, context->msgframe->msgs->msgs, &passedcur,
						   doOpenedFirst);
	}
	if (!msg) {
	  msg = msg_find_pass2(context, context->msgframe->msgs->msgs);
	}
	if (msg) {
	  /* Check to see if we merely hit the special termination case. */
	  if (!msg_find_check(context, msg)) msg = NULL;
	}
	if (msg) {
	  return msg_OpenMessage(context, msg);
	}
  }
  return MK_MSG_SEARCH_FAILED;
}


int
MSG_DoFind(MWContext* context, const char* searchstring, XP_Bool casesensitive)
{
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return -1;
  XP_ASSERT(searchstring);
  if (!searchstring) return -1;
  msg_InterruptContext(context, FALSE);
  FREEIF(context->msgframe->searchstring);
  context->msgframe->searchstring = XP_STRDUP(searchstring);
  if (!context->msgframe->searchstring) return MK_OUT_OF_MEMORY;
  context->msgframe->searchcasesensitive = casesensitive;
  return msg_Find(context, TRUE);
}


