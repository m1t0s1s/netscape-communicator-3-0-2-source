/* -*- Mode: C; tab-width: 4 -*-
   folders.c --- commands relating to the folder list window
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-May-95.

   This file contains functions for dealing with the list of folders, and
   the list of threads -- searching, reordering, and redisplaying.  Basically,
   stuff for dealing with the structure of folders at the levels higher than
   that of the individual message, or the disk file.

   Most of the code in here deals with both mail and news "folders".

   Manipulation of mail folder disk files is in mailsum.c and spool.c.
 */

#include "msg.h"
#include "msgcom.h"
#include "mailsum.h"
#include "msgundo.h"
#include "xp_str.h"
#include "xpgetstr.h"
#include "bkmks.h"


extern int MK_MSG_CANT_CREATE_FOLDER;
extern int MK_MSG_CANT_DELETE_FOLDER;
extern int MK_MSG_FOLDER_ALREADY_EXISTS;
extern int MK_MSG_FOLDER_NOT_EMPTY;
extern int MK_OUT_OF_MEMORY;
extern int MK_MSG_OFFER_COMPRESS;
extern int MK_MSG_KBYTES_WASTED;
extern int MK_MSG_OPEN_FOLDER;
extern int MK_MSG_OPEN_FOLDER2;
extern int MK_MSG_ENTER_FOLDERNAME;
extern int MK_MSG_SAVE_MESSAGE_AS;
extern int MK_MSG_SAVE_MESSAGES_AS;
extern int MK_MSG_GET_NEW_MAIL;
extern int MK_MSG_DELIV_NEW_MSGS;
extern int MK_MSG_NEW_FOLDER;
extern int MK_MSG_COMPRESS_FOLDER;
extern int MK_MSG_COMPRESS_ALL_FOLDER;
extern int MK_MSG_OPEN_NEWS_HOST_ETC;
extern int MK_MSG_ADD_NEWS_GROUP;
extern int MK_MSG_EMPTY_TRASH_FOLDER;
extern int MK_MSG_PRINT;
extern int MK_MSG_UNDO;
extern int MK_MSG_REDO;
extern int MK_MSG_DELETE_SEL_MSGS;
extern int MK_MSG_DELETE_MESSAGE;
extern int MK_MSG_DELETE_FOLDER;
extern int MK_MSG_CANCEL_MESSAGE;
extern int MK_MSG_RMV_NEWS_HOST;
extern int MK_MSG_SUBSCRIBE;
extern int MK_MSG_UNSUBSCRIBE;
extern int MK_MSG_SELECT_THREAD;
extern int MK_MSG_SELECT_FLAGGED_MSG;
extern int MK_MSG_SELECT_ALL;
extern int MK_MSG_DESELECT_ALL_MSG;
extern int MK_MSG_FLAG_MESSAGE;
extern int MK_MSG_UNFLAG_MESSAGE;
extern int MK_MSG_AGAIN;
extern int MK_MSG_THREAD_MESSAGES;
extern int MK_MSG_BY_DATE;
extern int MK_MSG_BY_SENDER;
extern int MK_MSG_BY_SUBJECT;
extern int MK_MSG_BY_MESSAGE_NB;
extern int MK_MSG_UNSCRAMBLE;
extern int MK_MSG_ADD_FROM_NEW_MSG;
extern int MK_MSG_ADD_FROM_OLD_MSG;
extern int MK_MSG_GET_MORE_MSGS;
extern int MK_MSG_GET_ALL_MSGS;
extern int MK_MSG_WRAP_LONG_LINES;
extern int MK_MSG_ADDRESS_BOOK;
extern int MK_MSG_VIEW_ADDR_BK_ENTRY;
extern int MK_MSG_ADD_TO_ADDR_BOOK;
extern int MK_MSG_NEW_NEWS_MESSAGE;
extern int MK_MSG_POST_REPLY;
extern int MK_MSG_POST_MAIL_REPLY;
extern int MK_MSG_NEW_MAIL_MESSAGE;
extern int MK_MSG_REPLY;
extern int MK_MSG_REPLY_TO_ALL;
extern int MK_MSG_FWD_SEL_MESSAGES;
extern int MK_MSG_FORWARD;
extern int MK_MSG_FORWARD_QUOTED;
extern int MK_MSG_MARK_SEL_AS_READ;
extern int MK_MSG_MARK_AS_READ;
extern int MK_MSG_MARK_SEL_AS_UNREAD;
extern int MK_MSG_MARK_AS_UNREAD;
extern int MK_MSG_UNFLAG_ALL_MSGS;
extern int MK_MSG_COPY_SEL_MSGS;
extern int MK_MSG_COPY_ONE;
extern int MK_MSG_MOVE_SEL_MSG;
extern int MK_MSG_MOVE_ONE;
extern int MK_MSG_SAVE_SEL_MSGS;
extern int MK_MSG_SAVE_AS;
extern int MK_MSG_MOVE_SEL_MSG_TO;
extern int MK_MSG_MOVE_MSG_TO;
extern int MK_MSG_FIRST_MSG;
extern int MK_MSG_NEXT_MSG;
extern int MK_MSG_PREV_MSG;
extern int MK_MSG_LAST_MSG;
extern int MK_MSG_FIRST_UNREAD;
extern int MK_MSG_NEXT_UNREAD;
extern int MK_MSG_PREV_UNREAD;
extern int MK_MSG_LAST_UNREAD;
extern int MK_MSG_FIRST_FLAGGED;
extern int MK_MSG_NEXT_FLAGGED;
extern int MK_MSG_PREV_FLAGGED;
extern int MK_MSG_LAST_FLAGGED;
extern int MK_MSG_MARK_SEL_READ;
extern int MK_MSG_MARK_THREAD_READ;
extern int MK_MSG_MARK_NEWSGRP_READ;
extern int MK_MSG_SHOW_SUB_NEWSGRP;
extern int MK_MSG_SHOW_ACT_NEWSGRP;
extern int MK_MSG_SHOW_ALL_NEWSGRP;
extern int MK_MSG_CHECK_FOR_NEW_GRP;
extern int MK_MSG_SHOW_ALL_MSGS;
extern int MK_MSG_SHOW_UNREAD_ONLY;
extern int MK_MSG_SHOW_MICRO_HEADERS;
extern int MK_MSG_SHOW_SOME_HEADERS;
extern int MK_MSG_SHOW_ALL_HEADERS;
extern int MK_MSG_ALL_FIELDS;
extern int MK_MSG_QUOTE_MESSAGE;
extern int MK_MSG_FROM;
extern int MK_MSG_REPLY_TO;
extern int MK_MSG_MAIL_TO;
extern int MK_MSG_MAIL_CC;
extern int MK_MSG_MAIL_BCC;
extern int MK_MSG_FILE_CC;
extern int MK_MSG_POST_TO;
extern int MK_MSG_FOLLOWUPS_TO;
extern int MK_MSG_SUBJECT;
extern int MK_MSG_ATTACHMENT;
extern int MK_MSG_SEND_FORMATTED_TEXT;
extern int MK_MSG_SEND_ENCRYPTED;
extern int MK_MSG_SEND_SIGNED;
extern int MK_MSG_SECURITY_ADVISOR;
extern int MK_MSG_Q4_LATER_DELIVERY;
extern int MK_MSG_ATTACH_AS_TEXT;
extern int MK_MSG_MARK_MESSAGES;
extern int MK_MSG_UNMARK_MESSAGES;
extern int MK_MSG_SORT_BACKWARD;
extern int MK_MSG_FLAG_MESSAGES;
extern int MK_MSG_UNFLAG_MESSAGES;
extern int MK_MSG_LOADED_MESSAGES;

extern int MK_MSG_FIND_AGAIN;
extern int MK_MSG_SEND;
extern int MK_MSG_SEND_LATER;
extern int MK_MSG_ATTACH_ETC;
extern int MK_MSG_ATTACHMENTSINLINE;
extern int MK_MSG_ATTACHMENTSASLINKS;
extern int MK_MSG_DELETE_FOLDER_MESSAGES;

/* #### Why is this alegedly netlib-specific?   What else am I to do? */
extern char *NET_ExplainErrorDetails (int code, ...);


XP_Bool msg_auto_show_first_unread = FALSE;

XP_Bool msg_auto_expand_mail_threads = TRUE;
XP_Bool msg_auto_expand_news_threads = TRUE;

void
MSG_SetAutoShowFirstUnread(XP_Bool value)
{
  msg_auto_show_first_unread = value;
}

void MSG_SetExpandMessageThreads(XP_Bool isnews, XP_Bool value)
{
#if 0
  if (isnews) {
	msg_auto_expand_news_threads = value;
  } else {
	msg_auto_expand_mail_threads = value;
  }
#endif
}


static char *
msg_magic_folder_name (MWContext *context, int flag)
{
  const char *dir = FE_GetFolderDirectory(context);
  char *name;
  char *ptr;
  XP_ASSERT(dir);
  if (!dir) return 0;
  name = (char *) XP_ALLOC(XP_STRLEN(dir) + 30);
  if (!name) return 0;
  XP_STRCPY(name, dir);
  ptr = name + XP_STRLEN(name);
  if (ptr[-1] != '/')
	*ptr++ = '/';
	  
  XP_STRCPY(ptr,
			(flag == MSG_FOLDER_FLAG_INBOX  ? INBOX_FOLDER_NAME :
			 flag == MSG_FOLDER_FLAG_TRASH  ? TRASH_FOLDER_NAME :
			 flag == MSG_FOLDER_FLAG_QUEUE  ? QUEUE_FOLDER_NAME :
/*			 flag == MSG_FOLDER_FLAG_DRAFTS ? DRAFTS_FOLDER_NAME : */
			 "###"));
  return name;
}

/* returns a folder object of the given magic type, creating it if necessary.
   Context must be a mail window. */
MSG_Folder *
msg_FindFolderOfType(MWContext* context, int type)
{
  MSG_Folder* folder;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail)
	return 0;
  XP_ASSERT (type == MSG_FOLDER_FLAG_INBOX ||
			 type == MSG_FOLDER_FLAG_TRASH ||
/*			 type == MSG_FOLDER_FLAG_DRAFTS || */
			 type == MSG_FOLDER_FLAG_QUEUE);
  for (folder = context->msgframe->folders ; folder ; folder = folder->next)
	if (folder->flags & type)
	  break;

  if (!folder) {
	char *name = msg_magic_folder_name (context, type);
	if (!name) return 0;
	folder = msg_FindMailFolder(context, name, TRUE);
	XP_FREE(name);
	XP_ASSERT (folder &&
			   (folder->flags & type) &&
			   (folder->flags & MSG_FOLDER_FLAG_MAIL) &&
			   (! (folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
									MSG_FOLDER_FLAG_NEWS_HOST |
									MSG_FOLDER_FLAG_DIRECTORY |
									MSG_FOLDER_FLAG_ELIDED))));
  }
  return folder;
}


/* returns the name of the queue folder - returns a new string.
   Context may be any type. */
char *
msg_QueueFolderName(MWContext *context)
{
  return msg_magic_folder_name (context, MSG_FOLDER_FLAG_QUEUE);
}


static int
msg_count_all_folders(MSG_Folder* folder)
{
  int result = 0;
  for ( ; folder ; folder = folder->next) {
	result += 1 + msg_count_all_folders(folder->children);
  }
  return result;
}
	
static int
msg_count_visible_folders(MSG_Folder* folder)
{
  int result = 0;
  for ( ; folder ; folder = folder->next) {
	result++;
	if (! (folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  result += msg_count_visible_folders(folder->children);
	}
  }
  return result;
}

static XP_Bool
msg_line_for_folder_1(MSG_Folder* folder, MSG_Folder* search, int* line,
					  int* depth)
{
  for (; folder ; folder = folder->next) {
	if (folder == search) return TRUE;
	(*line)++;
	if (folder->children && !(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  (*depth)++;
	  if (msg_line_for_folder_1(folder->children, search, line, depth)) {
		return TRUE;
	  }
	  (*depth)--;
	}
  }
  return FALSE;
}

static int
msg_line_for_folder(MWContext* context, MSG_Folder* folder, int *depth)
{
  int line = 0;
  int d = 0;
  if (!msg_line_for_folder_1(context->msgframe->folders, folder, &line, &d)) {
	return -1;
  }
  if (depth) *depth = d;
  return line;
}


static uint32
msg_addup_wasted_bytes(MSG_Folder* folder)
{
  uint32 result = 0;
  for (; folder ; folder = folder->next) {
	result += folder->data.mail.wasted_bytes;
	result += msg_addup_wasted_bytes(folder->children);
  }
  return result;
}


static void
msg_scan_folder_2(MWContext* context, MSG_Folder* folder)
{
  uint32 unread, total, wasted, bytes;
  if (msg_GetSummaryTotals(folder->data.mail.pathname, &unread, &total,
						   &wasted, &bytes)) {
	folder->unread_messages = unread;
	folder->total_messages = total;
	folder->data.mail.wasted_bytes = wasted;
    /* DEBUG_jwz: 10M instead of 50K */
	if (wasted * 5 > bytes && wasted > (10 * 1024 * 1024)) {
	  /* More than 20% of this folder is wasted, and that's more than 10M;
		 that's our heuristic to offer the user a chance to compress
		 everything. */
	  context->msgframe->data.mail.want_compress = TRUE;
	}
	msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
  } else {
	folder->unread_messages = -2;
  }
}

static XP_Bool
msg_scan_folder_1(MWContext* context, MSG_Folder* folder)
{
  for (; folder ; folder = folder->next) {
	if (folder->flags & MSG_FOLDER_FLAG_DIRECTORY) {
	  if (!(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
		if (msg_scan_folder_1(context, folder->children)) {
		  return TRUE;
		}
	  }
	} else if (folder->unread_messages == -1) {
	  msg_scan_folder_2(context, folder);
	  return TRUE;
	}
  }
  return FALSE;
}

static void
msg_scan_folder(void* closure)
{
  MWContext* context = (MWContext*) closure;
  MSG_Frame* f = context->msgframe;
  f->data.mail.scan_timer = NULL;
  if (msg_scan_folder_1(context, f->folders)) {
	if (f->data.mail.scan_timer == NULL) {
	  f->data.mail.scan_timer = FE_SetTimeout(msg_scan_folder, context, 1);
	}
  } else {
	if (f->data.mail.want_compress && !f->data.mail.offered_compress) {
	  char* buf;
	  if (NET_AreThereActiveConnectionsForWindow(context)) {
		/* Looks like we're busy doing something, probably incorporating
		   new mail.  Don't interrupt that; we can wait until we're idle
		   to ask our silly compression question.  Just try again in a
		   half-second. */
		f->data.mail.scan_timer = FE_SetTimeout(msg_scan_folder, context, 500);
		return;
	  }
	  buf = PR_smprintf(XP_GetString(MK_MSG_OFFER_COMPRESS),
						msg_addup_wasted_bytes(f->folders) / 1024);
	  if (buf) {
		f->data.mail.offered_compress = TRUE; /* Set it now, before FE_Confirm
												 gets called, as timers and
												 things can get called from
												 FE_Confirm, and we could end
												 up back here again. */
		if (FE_Confirm(context, buf)) {
		  msg_InterruptContext(context, FALSE);
		  msg_CompressAllFolders(context);
		}
		XP_FREE(buf);
	  }
	}
	f->data.mail.offered_compress = TRUE;
  }
}

static void
msg_update_folder_list_1(MSG_Folder* folder, int* line, int* selected, int max,
						 int* cur)
{
  for ( ; folder ; folder = folder->next) {
	if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	  if (*cur < max) selected[*cur] = *line;
	  (*cur)++;
	}
	(*line)++;
	if (folder->children && !(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  msg_update_folder_list_1(folder->children, line, selected, max, cur);
	}
  }
}

static void
msg_update_folder_list(MWContext* context, int index, int length,
					   int visibleline)
{
  int* selected = NULL;
  int cur = 0;
  int line = 0;
  MSG_Frame* f = context->msgframe;
  XP_ASSERT(f);
  if (!f) return;
  if (f->num_selected_folders > 0) {
	selected = (int*) XP_ALLOC(f->num_selected_folders * sizeof(int));
	if (!selected) return;
  }
  msg_update_folder_list_1(f->folders, &line, selected,
						   selected ? f->num_selected_folders : 0, &cur);
  XP_ASSERT(cur == f->num_selected_folders);
  if (cur > f->num_selected_folders) cur = f->num_selected_folders;
  if (length < 0) length = line - index;
  FE_UpdateFolderList(context, index, length, line, visibleline,
					  selected, cur);
  FREEIF(selected);
}


static void
msg_flush_update_folder(MWContext* context)
{
  MSG_Frame* f = context->msgframe;
  if (f->data.mail.scan_timer == NULL && context->type == MWContextMail) {
	/* What a disgusting way to do a background job.  ### */
	f->data.mail.scan_timer = FE_SetTimeout(msg_scan_folder, context, 1);
  }
  if (!f->update_folder) return;
  if (f->update_folder_line < 0 || f->update_folder_depth < 0) {
	f->update_folder_line = msg_line_for_folder(context, f->update_folder,
												&f->update_folder_depth);
  }
  msg_update_folder_list(context, f->update_folder_line, 1,
						 (f->update_folder_visible ? f->update_folder_line :
						  -1));
  f->update_folder = NULL;
}




void
msg_RedisplayFoldersFromLine(MWContext* context, int index)
{
  int num;

  msg_flush_update_folder(context);

  num = msg_count_visible_folders(context->msgframe->folders);

  msg_update_folder_list(context, index, -1, -1);
}


static void
msg_get_folder_line(MWContext* context, MSG_Folder* folder, int depth,
					MSG_FolderLine* data)
{
  data->depth = depth;
  data->name = folder->name;
  data->flags = folder->flags;
  data->moreflags = 0;
  if (folder == context->msgframe->focusFolder) {
	data->moreflags |= MSG_MOREFLAG_LAST_SELECTED;
  }
  if (folder->flags & MSG_FOLDER_FLAG_DIRECTORY) {
	data->flippy_type = ((folder->flags & MSG_FOLDER_FLAG_ELIDED)
						 ? MSG_Folded
						 : MSG_Expanded);
  } else {
	data->flippy_type = MSG_Leaf;
	data->unseen = folder->unread_messages;
	data->total = folder->total_messages;
  }
}


static void
msg_redisplay_folders_1(MWContext* context, MSG_Folder* folder,
						MSG_FolderLine** lines, int depth)
{
  for (; folder ; folder = folder->next) {
	msg_get_folder_line(context, folder, depth, *lines);
	(*lines)++;
	msg_redisplay_folders_1(context, folder->children, lines, depth+1);
  }
}

void
msg_RedisplayFolders(MWContext* context)
{
  int num;
  MSG_FolderLine* lines = NULL;
  msg_RedisplayFoldersFromLine(context, 0);
  if (context->type != MWContextMail) return;
  num = msg_count_all_folders(context->msgframe->folders);
  if (num > 0) {
	lines = (MSG_FolderLine*) XP_ALLOC(sizeof(MSG_FolderLine) * num);
	if (lines == NULL) num = 0;
  }
  if (num > 0) {
	MSG_FolderLine* tmp = lines;
	msg_redisplay_folders_1(context, context->msgframe->folders, &(tmp), 0);
	XP_ASSERT(tmp - lines == num);
  }
  FE_UpdateFolderMenus(context, lines, num);
  if (lines) XP_FREE(lines);
}


void
msg_RedisplayOneFolder(MWContext* context, MSG_Folder* folder, int line_number,
					   int depth, XP_Bool force_visible_p)
{
  if (folder == context->msgframe->update_folder) {
	if (line_number >= 0 && depth >= 0) {
	  context->msgframe->update_folder_line = line_number;
	  context->msgframe->update_folder_depth = depth;
	}
	if (force_visible_p) context->msgframe->update_folder_visible = TRUE;
	return;
  }
  msg_flush_update_folder(context);
  context->msgframe->update_folder = folder;
  context->msgframe->update_folder_line = line_number;
  context->msgframe->update_folder_depth = depth;
  context->msgframe->update_folder_visible = force_visible_p;
  if (context->msgframe->disable_updates_depth == 0) {
	msg_flush_update_folder(context);
  }
}
  
void
msg_UpdateToolbar(MWContext* context)
{
  if (!context->msgframe || context->msgframe->disable_updates_depth == 0) {
	FE_UpdateToolbar(context);
  } else {
	context->msgframe->need_toolbar_update = TRUE;
  }
}


static int
msg_UpdateStatusLine(MWContext* context)
{								/* fix i18n in this routine ### */
  MSG_Frame* f = context->msgframe;
  XP_ASSERT(f);
  if (!f) return -1;
  FREEIF(context->defaultStatus);
  switch (context->type) {
  case MWContextMail:
	if (f->opened_folder && f->msgs) {
	  int32 perc;
	  if (f->msgs->foldersize > 0) {
		perc = f->msgs->expunged_bytes * 100 / f->msgs->foldersize;
		if (perc > 100) perc = 100;
	  } else {
		perc = 100;
	  }
	  context->defaultStatus =
		PR_smprintf(XP_GetString(MK_MSG_KBYTES_WASTED),
					f->opened_folder->data.mail.pathname,
					f->msgs->expunged_bytes / 1024, perc);
	}
	break;
  case MWContextNews:
	if (f->opened_folder && f->msgs) {
	  int32 perc;
	  int32 num;
	  int32 total;
	  if (f->all_messages_visible_p) {
		num = f->msgs->total_messages;
		total = f->opened_folder->total_messages;
	  } else {
		num = f->msgs->unread_messages;
		total = f->opened_folder->unread_messages;
	  }
	  if (total) {
		perc = num * 100 / total;
		if (perc > 100) perc = 100;
	  } else {
		perc = 100;
	  }
	  context->defaultStatus =
		PR_smprintf(XP_GetString(MK_MSG_LOADED_MESSAGES),
					f->opened_folder->data.newsgroup.group_name, num, perc);
	}
	break;
  default:
	XP_ASSERT(0);
	return -1;
  }
  FE_Progress(context, context->defaultStatus);
  return 0;
}


void
msg_DisableUpdates(MWContext* context)
{
  if (context->msgframe) {
	context->msgframe->disable_updates_depth++;
  }
}

void 
msg_EnableUpdates(MWContext* context)
{
  if (context->msgframe) {
	XP_ASSERT(context->msgframe->disable_updates_depth > 0);
	context->msgframe->disable_updates_depth--;
	if (context->msgframe->disable_updates_depth == 0) {
	  msg_flush_update_folder(context);
	  msg_FlushUpdateMessages(context);
	  if (context->msgframe->need_toolbar_update) {
		msg_UpdateToolbar(context);
		context->msgframe->need_toolbar_update = FALSE;
	  }
	  msg_UpdateStatusLine(context);
	}
  }
}


static MSG_Folder*
msg_find_folder_line_1(MSG_Folder* folder, int* find, int* depth) 
{
  MSG_Folder* result;
  for ( ; folder ; folder = folder->next) {
	if (*find == 0) return folder;
	(*find)--;
	if (folder->children && !(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  if (depth) (*depth)++;
	  result = msg_find_folder_line_1(folder->children, find, depth);
	  if (result) return result;
	  if (depth) (*depth)--;
	}
  }
  return NULL;
}
  
static MSG_Folder*
msg_find_folder_line(MWContext* context, int line_number, int* depth) 
{
  if (depth) *depth = 0;
  return msg_find_folder_line_1(context->msgframe->folders, &line_number,
								depth);
}



static MSG_Folder* 
msg_find_folder_line_unfolded_1(MSG_Folder* folder, int* find)
{
  MSG_Folder* result;
  for ( ; folder ; folder = folder->next) {
	if (*find == 0) return folder;
	(*find)--;
	if (folder->children) {
	  result = msg_find_folder_line_unfolded_1(folder->children, find);
	  if (result) return result;
	}
  }
  return NULL;
}

static MSG_Folder*
msg_find_folder_line_unfolded(MWContext* context, int line_number)
{
  return msg_find_folder_line_unfolded_1(context->msgframe->folders,
										 &line_number);
}


static MSG_Folder*
msg_get_folder_line_1(MSG_Folder* folder, int* find, int* depth,
					  MSG_AncestorInfo** ancestor) {
  MSG_Folder* result;
  int d = *depth;
  XP_Bool has_prev = FALSE;
  for ( ; folder ; folder = folder->next, has_prev = TRUE) {
	if (*find == 0) {
	  if (ancestor) {
		*ancestor = (MSG_AncestorInfo*)
		  XP_ALLOC((d + 1) * sizeof(MSG_AncestorInfo));
		if (*ancestor) {
		  (*ancestor)[d].has_next = (folder->next != NULL);
		  (*ancestor)[d].has_prev = has_prev;
		}
	  }
	  return folder;
	}
	(*find)--;
	if (folder->children && !(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  (*depth)++;
	  result = msg_get_folder_line_1(folder->children, find, depth, ancestor);
	  if (result) {
		if (ancestor && *ancestor) {
		  (*ancestor)[d].has_next = (folder->next != NULL);
		  (*ancestor)[d].has_prev = has_prev;
		}
		return result;
	  }
	  (*depth)--;
	}
  }
  return NULL;
}
  


XP_Bool
MSG_GetFolderLine(MWContext* context, int line, MSG_FolderLine* data,
				  MSG_AncestorInfo** ancestor)
{
  int depth = 0;
  MSG_Folder* folder = msg_get_folder_line_1(context->msgframe->folders, &line,
											 &depth, ancestor);
  if (!folder) return FALSE;
  msg_get_folder_line(context, folder, depth, data);
  return TRUE;
}



static XP_Bool
msg_remove_folder_line_1(MSG_Folder* folder, MSG_Folder* search)
{
  for ( ; folder ; folder = folder->next) {
	if (folder->next == search) {
	  folder->next = search->next;
	  return TRUE;
	}
	if (folder->children == search) {
	  folder->children = search->next;
	  return TRUE;
	}
	if (msg_remove_folder_line_1(folder->children, search)) {
	  return TRUE;
	}
  }
  return FALSE;
}


void
msg_RemoveFolderLine (MWContext *context, MSG_Folder *folder)
{
  XP_Bool result;
  int line;
  XP_ASSERT (context->msgframe &&
			 !folder->children &&
			 !(folder->flags & MSG_FOLDER_FLAG_ELIDED) &&
			 !(folder->flags & MSG_FOLDER_FLAG_DIRECTORY));
  msg_flush_update_folder(context);
  line = msg_line_for_folder(context, folder, NULL);
  if (folder == context->msgframe->folders) {
	context->msgframe->folders = folder->next;
  } else {
	result = msg_remove_folder_line_1(context->msgframe->folders, folder);
	XP_ASSERT(result);
  }
  folder->next = NULL;
  if (line >= 0) {
	msg_RedisplayFoldersFromLine(context, line);
  }
}


static int
msg_get_folder_max_depth_1(MSG_Folder* folder)
{
  int result = 0;
  for ( ; folder ; folder = folder->next) {
	if (folder->children && !(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  int value = msg_get_folder_max_depth_1(folder->children);
	  if (result < value) result = value;
	}
  }
  return result + 1;
}

int
MSG_GetFolderMaxDepth(MWContext* context)
{
  MSG_Frame* f;
  XP_ASSERT(context && context->msgframe);
  if (!context || !context->msgframe) return -1;
  f = context->msgframe;
  if (f->max_depth) return f->max_depth;
  if (context->type == MWContextNews) {
	if (f->data.news.group_display_type != MSG_ShowAll) {
	  f->max_depth = 2;
	  return 2;
	}
  }
  f->max_depth = msg_get_folder_max_depth_1(f->folders);
  if (context->type == MWContextNews && f->max_depth < 2) {
	f->max_depth = 2;
  }
  return f->max_depth;
}


void
msg_RebuildFolderMenus(MWContext* context)
{
  /* #### Write me! */
}

char*
MSG_GetFolderName(MWContext* context, int line_number) 
{
  MSG_Folder* folder;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail)
	return 0;
  folder = msg_find_folder_line(context, line_number, NULL);
  if (!folder)
	return 0;
  else
	return folder->data.mail.pathname;
}


char*
MSG_GetFolderNameUnfolded(MWContext* context, int line_number) 
{
  MSG_Folder* folder;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail)
	return 0;
  folder = msg_find_folder_line_unfolded(context, line_number);
  if (!folder)
	return 0;
  else
	return folder->data.mail.pathname;
}



static void
msg_open_folder_cb (MWContext *context, char *file_name, void *closure)
{
  URL_Struct* url_struct;
  char *url;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return;
  if (!file_name) return;
  url = (char *) XP_ALLOC (XP_STRLEN (file_name) + 20);
  if (! url) return /* MK_OUT_OF_MEMORY */;
  XP_STRCPY (url, "mailbox:");
  /* Assumes file_name is in unix (url) form: /a/b/c/foo */
  XP_STRCAT (url, file_name);
  XP_FREE (file_name);
  url_struct = NET_CreateURLStruct(url, NET_DONT_RELOAD);
  XP_FREE (url);
  msg_GetURL (context, url_struct, FALSE);
}

int
msg_OpenFolder (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return -1;

  FE_PromptForFileName (context, XP_GetString(MK_MSG_OPEN_FOLDER), 0, /* default_path */
						TRUE, FALSE, msg_open_folder_cb, 0);
  return 0;
}

int
msg_NewFolder(MWContext* context)
{
  char* name;
  MSG_Folder* folder;
  XP_StatStruct folderst;
  XP_File file;
  int status = 0;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return -1;

  name =
	FE_Prompt(context,
			  XP_GetString(MK_MSG_ENTER_FOLDERNAME), /* ### Prompt should
														probably explain the
														expected syntax! */
			  NULL);
  if (!name || !XP_STRLEN(name)) return 0;
  folder = msg_MakeMailFolder(context, FE_GetFolderDirectory(context), name);
  if (!folder) return MK_OUT_OF_MEMORY;
  if (!XP_Stat(folder->data.mail.pathname, &folderst, xpMailFolder)) {
	status = MK_MSG_FOLDER_ALREADY_EXISTS;
	goto FAIL;
  } 
  file = XP_FileOpen(folder->data.mail.pathname, xpMailFolder,
					 XP_FILE_WRITE_BIN);
  if (!file) {
	status = MK_MSG_CANT_CREATE_FOLDER;
	goto FAIL;
  }
  XP_FileClose(file);

  /* What a silly hack.  msg_FindMailFolder and msg_MakeMailFolder need to
	 be rethought.  ###`*/
  msg_RedisplayOneFolder(context,
						 msg_FindMailFolder(context,
											folder->data.mail.pathname, TRUE),
						 -1, -1, TRUE);
  status = 0;
FAIL:
  msg_FreeOneFolder(context, folder);
  return status;
}


static MSG_Folder**
msg_find_folder_parent(MSG_Folder** start, MSG_Folder* find)
{
  MSG_Folder** result;
  for (; *start ; start = &((*start)->next)) {
	if (*start == find) return start;
	result = msg_find_folder_parent(&((*start)->children), find);
	if (result) return result;
  }
  return NULL;
}

static void msg_select_folder(MWContext* context, MSG_Folder* folder);
static void msg_open_first_unseen(URL_Struct *url, int status, MWContext *context);

static int
msg_delete_folder_2(MWContext* context, MSG_Folder* folder)
{
  MSG_Folder** parent;
  MSG_Folder* child;
  MSG_Folder* next;
  uint32 unread;
  uint32 total;
  int status = 0;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return -1;

  if (folder == NULL) return 0;
  if (folder->flags & MSG_FOLDER_FLAG_DIRECTORY) {
	/* Bloody hell, he wants to destroy a directory of folders.  Off we
	   go... */
	for (child = folder->children ; child ; child = next) {
	  next = child->next;
	  status = msg_delete_folder_2(context, child);
	  if (status < 0) return status;
	}
  }

  if (folder != context->msgframe->opened_folder) {
	/* It's tempting to just look at the "total_messages" field in the
	   folder, but that's wrong because the folder may have been modified
	   in the meantime by some external agency.  This is a routine we need
	   to be a little careful in... */
	if (msg_GetSummaryTotals(folder->data.mail.pathname, &unread, &total,
							 NULL, NULL)) {
	  if (total > 0) {
	    /* This code was supposed to allow deleting a folder that contains
	       messages. But it does not know how to handle the case of updated
	       folder summary file. So just return error for now.
            char *buf;
  	    buf = PR_smprintf(XP_GetString(MK_MSG_DELETE_FOLDER_MESSAGES),
              folder->name);
	    if (!FE_Confirm(context, buf))
              return MK_MSG_FOLDER_NOT_EMPTY;
	    sumfid = XP_FileOpen(folder->data.mail.pathname, xpMailFolderSummary,
			XP_FILE_READ_BIN);
	    if (sumfid) {
		context->msgframe->msgs = msg_ReadFolderSummaryFile(context, 
			sumfid, folder->data.mail.pathname, NULL);
		XP_FileClose(sumfid);
	    }
            context->msgframe->opened_folder = folder;
	    msg_SelectAllMessages(context);
	    if (msg_DeleteMessages(context))
	      return -1;
	    */
	    return MK_MSG_FOLDER_NOT_EMPTY;
	  }
	} else {
	  /* We're trying to delete a folder that isn't opened, and it doesn't have
		 a valid summary file.  We know the folder isn't strictly empty
		 (msg_GetSummaryTotals handles that case), but it might have only
		 expunged messages in it.  Therefore, we need to parse it. The thing
		 we should do is cause it to be compressed, and then we'll call this
		 routine again.  How disgusting.  For now, we'll just tell the
		 user that it isn't empty.  He'll be forced to open it, which will
		 reparse it, and then he can say "silly netscape, it is too empty", and
		 try again, and it will work.  This is even somewhat reflected in the
		 UI, in that the folder will have ??? for the total.  But it's still
		 a bug.  ### */
      return MK_MSG_FOLDER_NOT_EMPTY;
	}
  } else {
	if (context->msgframe->msgs == NULL) return -1; /* How can this happen? */
	
	if (context->msgframe->msgs->msgs ||
		context->msgframe->msgs->hiddenmsgs) {
	    /* This code was supposed to allow deleting a folder that contains
	       messages. But it does not know how to handle the case of updated
	       folder summary file. So just return error for now.
            char *buf;
  	    buf = PR_smprintf(XP_GetString(MK_MSG_DELETE_FOLDER_MESSAGES),
              folder->name);
	    if (!FE_Confirm(context, buf))
              return MK_MSG_FOLDER_NOT_EMPTY;
	    msg_SelectAllMessages(context);
	    if (msg_DeleteMessages(context))
	      return -1;
	    */
	    return MK_MSG_FOLDER_NOT_EMPTY;
	}
  }
  msg_undo_Initialize(context);
  if (XP_FileRemove(folder->data.mail.pathname, xpMailFolderSummary) < 0) {
	return MK_MSG_CANT_DELETE_FOLDER;
  }
  if (XP_FileRemove(folder->data.mail.pathname, xpMailFolder) < 0) {
	return MK_MSG_CANT_DELETE_FOLDER;
  }
  parent = msg_find_folder_parent(&(context->msgframe->folders), folder);
  if (!parent) return -1;
  
  if (context->msgframe->opened_folder == folder) {
	context->msgframe->opened_folder = NULL;
  }

  *parent = folder->next;

  if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	context->msgframe->num_selected_folders--;
  }
  msg_FreeOneFolder(context, folder);

  return 0;
}


static int
msg_delete_folder_1(MWContext* context, MSG_Folder* folder)
{
  int status = 0;
  MSG_Folder* next;
  for (; folder ; folder = next) {
	next = folder->next;		/* Necessary caution, since we're likely to
								   destroy this folder. */
	if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	  status = msg_delete_folder_2(context, folder);
	  if (status < 0) return status;
	} else {
	  status = msg_delete_folder_1(context, folder->children);
	  if (status < 0) return status;
	}
  }
  return status;
}

int
msg_DeleteFolder(MWContext* context)
{
  int status;
  MSG_Folder* tempfolder;
  MSG_Folder* selected = NULL;

  XP_ASSERT(context->msgframe);
  if (!context->msgframe) return -1;

  /* Find the folder prior to the first selection. */
  for (tempfolder = context->msgframe->folders; tempfolder; ) {
	if (tempfolder->flags & MSG_FOLDER_FLAG_SELECTED) 
	  break;
	selected = tempfolder;
	tempfolder = tempfolder->next;
  }

  status = msg_delete_folder_1(context, context->msgframe->folders);

  /* Reset focus */
  if ((context->msgframe->num_selected_folders == 0) && selected) {
    msg_DisableUpdates(context);
	msg_SelectAndOpenFolder(context, selected,
							(msg_auto_show_first_unread ?
							 msg_open_first_unseen :
							 msg_ScrolltoFirstUnseen));
    msg_EnableUpdates(context);
  }

  msg_RedisplayFolders(context);
  return status;
}



static XP_Bool
msg_find_next_unselected_message_1(MWContext* context,
								   MSG_ThreadEntry* msg,
								   XP_Bool* hitselected,
								   MSG_ThreadEntry** result)
{
  for (; msg ; msg = msg->next) {
	if (!(msg->flags & (MSG_FLAG_SELECTED | MSG_FLAG_EXPIRED))) {
	  *result = msg;
	  if (*hitselected) return TRUE;
	}
	if (!(*hitselected) && (msg->flags & MSG_FLAG_SELECTED)) {
	  *hitselected = TRUE;
	}
	if (msg->first_child && !(msg->flags & MSG_FLAG_ELIDED)) {
	  if (msg_find_next_unselected_message_1(context, msg->first_child,
											 hitselected, result)) {
		return TRUE;
	  }
	}
  }
  return FALSE;
}


/* Find the first message that is after a selected message but is not
   currently selected.  This is the message to view when a delete/move
   operation is complete. */
MSG_ThreadEntry *
msg_FindNextUnselectedMessage(MWContext* context)
{
  XP_Bool hitselected = FALSE;
  MSG_ThreadEntry* result = NULL;
  msg_find_next_unselected_message_1(context,
									 context->msgframe->msgs->msgs,
									 &hitselected,
									 &result);
  return result;
}
									 

void
MSG_CopySelectedMessagesInto (MWContext *context, const char *folder_name)
{
  int status;
  XP_ASSERT (context->type == MWContextMail || context->type == MWContextNews);
  if (context->type != MWContextMail && context->type != MWContextNews) return;
  XP_ASSERT (folder_name);
  if (!folder_name) return;
  msg_InterruptContext(context, FALSE);
  if (context->type == MWContextMail)
	status = msg_MoveOrCopySelectedMailMessages (context, folder_name, FALSE);
  else
	status = msg_SaveSelectedNewsMessages (context, folder_name);

  if (status < 0)
	{
	  char *alert = NET_ExplainErrorDetails(status);
	  if (alert)
		{
		  FE_Alert(context, alert);
		  XP_FREE(alert);
		}
	}
}


static void
msg_save_messages_as_cb (MWContext *context, char *file_name, void *closure)
{
  XP_Bool ismove = (closure != NULL);
  int status;
  XP_ASSERT (context->type == MWContextMail || context->type == MWContextNews);
  if (context->type != MWContextMail && context->type != MWContextNews)
	{
	  FREEIF (file_name);
	  return;
	}
  if (!file_name) return;
  if (context->type == MWContextMail)
	status = msg_MoveOrCopySelectedMailMessages (context, file_name, ismove);
  else
	status = msg_SaveSelectedNewsMessages (context, file_name);

  if (status < 0)
	{
	  char *alert = NET_ExplainErrorDetails(status);
	  if (alert)
		{
		  FE_Alert(context, alert);
		  XP_FREE(alert);
		}
	}
  FREEIF(file_name);
}


int 
msg_SaveMessagesAs (MWContext *context, XP_Bool ismove)
{
  const char *title;
  XP_ASSERT (context->type == MWContextMail || context->type == MWContextNews);
  if (context->type != MWContextMail && context->type != MWContextNews)
	return -1;
  title = (context->msgframe->num_selected_msgs > 1
		   ? XP_GetString(MK_MSG_SAVE_MESSAGE_AS)
		   : XP_GetString(MK_MSG_SAVE_MESSAGES_AS));
  FE_PromptForFileName (context, title, 0, /* default_path */
						FALSE, FALSE, msg_save_messages_as_cb,
						ismove ? context : NULL);
  return 0;
}



static MSG_ThreadEntry* 
msg_find_article_number(MSG_ThreadEntry* msg, uint32 num)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (msg->data.news.article_number == num) {
	  return msg;
	}
	result = msg_find_article_number(msg->first_child, num);
	if (result) return result;
  }
  return NULL;
}


static void
msg_open_by_article_number(URL_Struct *url, int status, MWContext *context)
{
  if (context->msgframe->msgs) {
	MSG_ThreadEntry* msg =
	  msg_find_article_number(context->msgframe->msgs->msgs,
							  context->msgframe->first_incorporated_offset);
	if (msg) {
	  msg_ChangeFlag(context, context->msgframe->displayed_message,
					 0, MSG_FLAG_READ);
	  msg_ClearMessageArea(context);
	  msg_OpenMessage(context, msg);
	}
  }
}


/* This creates a folder object, and initializes the important slots.
   Either of prefix and suffix may be NULL, but not both.  If both are
   provided, they are appended, with a "/" between them if there isn't
   one at the end of the first or beginning of the second already.

   This initalizes folder->data.mail.pathname, and sets folder->name
   to an abbreviation based on the user's mail directory.  It also
   checks the name and initalizes the INBOX, QUEUE, etc flags as
   appropriate.
 */
MSG_Folder *
msg_MakeMailFolder (MWContext *context, const char *prefix, const char *suffix)
{
  char *name;
  MSG_Folder *f;
  const char *dir;
  const char* fcc;
  XP_ASSERT ((prefix && *prefix) || (suffix && *suffix));
  name = (char *) XP_ALLOC ((prefix ? XP_STRLEN(prefix) : 0) +
							(suffix ? XP_STRLEN(suffix) : 0) + 3);
  if (!name) return 0;
  f = XP_NEW (MSG_Folder);
  if (!f)
	{
	  XP_FREE (name);
	  return 0;
	}
  XP_MEMSET (f, 0, sizeof(*f));

  f->unread_messages = -1;
  f->flags |= MSG_FOLDER_FLAG_MAIL;

  *name = 0;
  if (prefix && *prefix)
	XP_STRCPY (name, prefix);
  if (suffix && *suffix && *name && name[XP_STRLEN(name)-1] != '/') 
     XP_STRCAT(name, "/");
  if (*name && suffix && *suffix == '/')
	suffix++;
  if (suffix)
	XP_STRCAT (name, suffix);

  f->data.mail.pathname = name;

  fcc = msg_GetDefaultFcc(FALSE);
  if (fcc && XP_FILENAMECMP(name, fcc) == 0) {
	f->flags |= MSG_FOLDER_FLAG_SENTMAIL;
  }
  fcc = msg_GetDefaultFcc(TRUE);
  if (fcc && XP_FILENAMECMP(name, fcc) == 0) {
	f->flags |= MSG_FOLDER_FLAG_SENTMAIL;
  }

#ifdef DEBUG_jwz
  /* Augh!  I have multiple Inbox folders, and FCC points to them, so 
	 do a horrible hack: if the folder's name ends in "Inbox", then
	 it's not a SENTMAIL folder.
   */
  if (f->flags & MSG_FOLDER_FLAG_SENTMAIL)
	{
	  char *kludge = "Inbox";
	  int L1 = XP_STRLEN(name);
	  int L2 = XP_STRLEN(kludge);
	  if (L1 > L2 && !XP_FILENAMECMP(kludge, name + L1 - L2))
		f->flags &= (~MSG_FOLDER_FLAG_SENTMAIL);
	}
#endif


  dir = FE_GetFolderDirectory(context);
  if (dir && !XP_STRNCMP (name, dir, XP_STRLEN(dir)))
	{
	  /* It's in the folder directory. */
	  const char *s = f->data.mail.pathname + XP_STRLEN(dir);
	  if (*s == '/') 
	  	s++;
	  XP_ASSERT (*s);
	  if (suffix && *suffix && XP_STRLEN(suffix) < XP_STRLEN(s)) {
		f->name = XP_STRDUP(suffix);
	  } else {
		f->name = XP_STRDUP (s);
	  }

	  /* Certain folder names are magic when in the base directory. */
	  if (!XP_FILENAMECMP(s, INBOX_FOLDER_NAME))
		f->flags |= MSG_FOLDER_FLAG_INBOX;
	  else if (!XP_FILENAMECMP(s, TRASH_FOLDER_NAME))
	  	f->flags |= MSG_FOLDER_FLAG_TRASH;
	  else if (!XP_FILENAMECMP(s, QUEUE_FOLDER_NAME))
	  	f->flags |= MSG_FOLDER_FLAG_QUEUE;
/*	  else if (!XP_FILENAMECMP(s, DRAFTS_FOLDER_NAME))
	  	f->flags |= MSG_FOLDER_FLAG_DRAFTS; */
	}
  else
	{
	  /* It's not in the folder directory. */
	  f->name = XP_STRDUP (f->data.mail.pathname);
	}

  if (!f->name)
	{
	  XP_FREE (f->data.mail.pathname);
	  XP_FREE (f);
	  return 0;
	}
  return f;
}


static XP_Bool
msg_toggle_message_mark_1(MWContext* context, MSG_ThreadEntry* msg,
						  void* closure)
{
  if (msg->flags & MSG_FLAG_MARKED) {
	msg_ChangeFlag(context, msg, MSG_FLAG_MARKED, 0);
  } else {
	msg_ChangeFlag(context, msg, 0, MSG_FLAG_MARKED);
  }
  return TRUE;
}  

void
MSG_ToggleMessageMark (MWContext *context)
{
  msg_IterateOverSelectedMessages(context, msg_toggle_message_mark_1, NULL);
}

static void
msg_unmark_all_messages_1(MWContext* context, MSG_ThreadEntry* msg)
{
  for ( ; msg ; msg = msg->next) {
	if (msg->flags & MSG_FLAG_MARKED) {
	  msg_ChangeFlag(context, msg, 0, MSG_FLAG_MARKED);
	}
	msg_unmark_all_messages_1(context, msg->first_child);
  }
}

int
msg_UnmarkAllMessages (MWContext *context)
{
  msg_undo_StartBatch(context);
  msg_unmark_all_messages_1(context, context->msgframe->msgs->msgs);
  msg_undo_EndBatch(context);
  return 0;
}


static XP_Bool
msg_delete_messages_1(MWContext* context, MSG_ThreadEntry* msg, void* closure)
{
  msg_ChangeFlag(context, msg, MSG_FLAG_EXPUNGED, 0);
  return TRUE;
}

int
msg_DeleteMessages (MWContext *context)
{
  MSG_Folder* folder;
  MSG_Folder* trash_folder;
  const char *d = FE_GetFolderDirectory (context);

  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return -1;
  folder = context->msgframe->opened_folder;
  if (!folder) return 0;

  XP_ASSERT (d);
  if (!d) return -1;

  trash_folder = msg_FindFolderOfType (context, MSG_FOLDER_FLAG_TRASH);
  XP_ASSERT (trash_folder);
  if (!trash_folder) return -1;

  if (folder == trash_folder) {
	/* This is the trash folder. */
	MSG_ThreadEntry* msg = msg_FindNextUnselectedMessage(context);
	msg_ClearMessageArea(context);
	msg_IterateOverSelectedMessages(context, msg_delete_messages_1, NULL);
	msg_OpenMessage(context, msg);
  } else

#ifdef DEBUG_jwz
    /* When deleting unread messages, mark them as read first, so that
       they are marked as read by the time they hit the Trash folder. */
    msg_MarkSelectedRead (context, TRUE);
#endif

	/* This isn't the trash, so just move the messages there. */
	MSG_MoveSelectedMessagesInto (context, trash_folder->data.mail.pathname);

  return 0;
}


void
MSG_ToggleSubscribed(MWContext* context, int line_number)
{
  int depth = 0;
  MSG_Folder* folder;
  if (context->type != MWContextNews) return;
  /* Don't call msg_InterruptContext(); this is a real harmless operation. */
  folder = msg_find_folder_line(context, line_number, &depth);
  if (folder) {
	msg_DisableUpdates(context);
	msg_ToggleSubscribedGroup(context, folder);
	msg_RedisplayOneFolder(context, folder, line_number, depth, TRUE);
	msg_EnableUpdates(context);
  }
}


static void
msg_open_first_unseen(URL_Struct *url, int status, MWContext *context)
{
  MSG_ThreadEntry* msg = msg_FindNextMessage(context, MSG_FirstUnreadMessage);
  if (msg) {
	msg_OpenMessage (context, msg);
  } else {
	/* Well, scroll down to the end then. */
	msg = msg_FindNextMessage(context, MSG_LastMessage);
	if (msg) msg_RedisplayOneMessage(context, msg, -1, -1, TRUE);
  }
}


void
msg_ScrolltoFirstUnseen(URL_Struct *url, int status, MWContext *context)
{
  MSG_ThreadEntry* msg;
  XP_ASSERT(context->msgframe);
  if (!context->msgframe) return;
  msg = context->msgframe->displayed_message;
  if (!msg) msg = msg_FindNextMessage(context, MSG_FirstUnreadMessage);
  if (!msg) msg = msg_FindNextMessage(context, MSG_LastMessage);
  if (msg) {
	/* Scroll this message to be visible.  Maybe we should select it? ### */
	msg_RedisplayOneMessage(context, msg, -1, -1, TRUE);
  }
}


static void
msg_close_opened_folder(MWContext* context)
{
  XP_ASSERT(context && context->msgframe);
  if (context && context->msgframe) {
	msg_ClearThreadList(context);
	context->msgframe->opened_folder = NULL;
  }
}


static void
msg_select_folder(MWContext* context, MSG_Folder* folder)
{
  if (folder) {
	if (!(folder->flags & MSG_FOLDER_FLAG_SELECTED)) {
	  folder->flags |= MSG_FOLDER_FLAG_SELECTED;
	  context->msgframe->num_selected_folders++;
	  msg_RedisplayOneFolder(context, folder, -1, -1, TRUE);
	}
  }
}


static void
msg_unselect_folder(MWContext* context, MSG_Folder* folder)
{
  if (folder) {
	if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	  folder->flags &= ~MSG_FOLDER_FLAG_SELECTED;
	  XP_ASSERT(context->msgframe->num_selected_folders > 0);
	  if (context->msgframe->num_selected_folders > 0) {
		context->msgframe->num_selected_folders--;
	  }
	  msg_RedisplayOneFolder(context, folder, -1, -1, TRUE);
	}
  }
}




void
MSG_SelectFolder(MWContext *context, int line_number, XP_Bool exclusive_p)
{
  int depth = 0;
  MSG_Folder* folder;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return;
  msg_InterruptContext(context, FALSE);
  folder = msg_find_folder_line(context, line_number, &depth);
  XP_ASSERT(folder);
  if (!folder) return;
  context->msgframe->focusFolder = folder;
  msg_DisableUpdates(context);
  if (exclusive_p) {
	msg_SelectAndOpenFolder(context, folder,
							(msg_auto_show_first_unread ?
							 msg_open_first_unseen :
							 msg_ScrolltoFirstUnseen));
  } else {
	msg_close_opened_folder(context);
	msg_select_folder(context, folder);
  }
  msg_EnableUpdates(context);
}



static MSG_Folder*
msg_validate_select_folder(MWContext* context, MSG_Folder* folder, int* line)
{
  MSG_Folder* result;
  for (; folder ; folder = folder->next) {
	if (folder == context->msgframe->lastSelectFolder) return folder;
	(*line)++;
	if (!(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  result = msg_validate_select_folder(context, folder->children, line);
	  if (result) return result;
	}
  }
  return NULL;
}

static void
msg_select_folder_range_1(MWContext* context, MSG_Folder* folder,
						  int min, int max, int line, int* cur)
{
  for (; folder ; folder = folder->next) {
	if (*cur == line) context->msgframe->focusFolder = folder;
	if (*cur >= min) {
	  if (*cur > max) return;
	  msg_select_folder(context, folder);
	}
	(*cur)++;
	if (!(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  msg_select_folder_range_1(context, folder->children, min, max,
								line, cur);
	}
  }
}

void
MSG_SelectFolderRangeTo(MWContext* context, int line)
{
  MSG_Frame* f;
  int min = 0;
  int max = 0;
  int cur = 0;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return;
  msg_InterruptContext(context, FALSE);
  f = context->msgframe;

  /* Be very paranoid, and make sure that lastSelectFolder still points to
	 a valid and selected folder before we use it.  At the same time, we'll
	 figure out what line number it is on. */
  f->lastSelectFolder = msg_validate_select_folder(context, f->folders,
												   &min);
  if (!f->lastSelectFolder) {
	MSG_SelectFolder(context, line, TRUE);
	return;
  }
  max = line;
  if (min > max) {
	int tmp = min;
	min = max;
	max = tmp;
  }
  msg_DisableUpdates(context);
  msg_close_opened_folder(context);
  msg_UnselectAllFolders(context);
  msg_select_folder_range_1(context, f->folders, min, max, line, &cur);
  msg_EnableUpdates(context);
}

void
MSG_UnselectFolder(MWContext* context, int line)
{
  int depth = 0;
  MSG_Folder* folder;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  folder = msg_find_folder_line(context, line, &depth);
  XP_ASSERT(folder);
  if (!folder) return;
  context->msgframe->focusFolder = folder; /* ### Is this right? */
  msg_DisableUpdates(context);
  msg_close_opened_folder(context);
  msg_unselect_folder(context, folder);
  msg_EnableUpdates(context);
}

void
MSG_ToggleSelectFolder(MWContext* context, int line)
{
  int depth = 0;
  MSG_Folder* folder;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe) return;
  msg_InterruptContext(context, FALSE);
  folder = msg_find_folder_line(context, line, &depth);
  XP_ASSERT(folder);
  if (!folder) return;
  context->msgframe->focusFolder = folder;
  msg_DisableUpdates(context);
  msg_close_opened_folder(context);
  if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	msg_unselect_folder(context, folder);
  } else {
	msg_select_folder(context, folder);
  }
  msg_EnableUpdates(context);
}





static void
msg_unselect_all_folders_1(MWContext* context, MSG_Folder* folder)
{
  for ( ; folder ; folder = folder->next) {
	if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	  folder->flags &= ~MSG_FOLDER_FLAG_SELECTED;
	  context->msgframe->num_selected_folders--;
	  msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
	}
	msg_unselect_all_folders_1(context, folder->children);
  }
}

void
msg_UnselectAllFolders(MWContext* context)
{
  msg_DisableUpdates(context);
  msg_unselect_all_folders_1(context, context->msgframe->folders);
  XP_ASSERT(context->msgframe->num_selected_folders == 0);
  context->msgframe->num_selected_folders = 0;
  msg_EnableUpdates(context);
}

int
msg_SelectAndOpenFolder(MWContext *context, MSG_Folder *folder,
						Net_GetUrlExitFunc* func)
{
  char *url = 0;
  URL_Struct *url_struct;
  int status = 0;

  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;

  if (!folder) return 0;

  msg_UnselectAllFolders(context);

  msg_select_folder(context, folder);

  context->msgframe->lastSelectFolder = folder;
  context->msgframe->focusFolder = folder;

  if (folder->flags & MSG_FOLDER_FLAG_DIRECTORY) {
	msg_ClearThreadList(context);
	return 0;
  }

  switch (context->type)
    {
    case MWContextMail:
	  if (folder == context->msgframe->opened_folder) {
		msg_SaveMailSummaryChangesNow(context);
		context->msgframe->opened_folder = NULL; /* Force reload. */
	  }

	  XP_ASSERT ((folder->flags && MSG_FOLDER_FLAG_MAIL) &&
				 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
									MSG_FOLDER_FLAG_NEWSGROUP |
									MSG_FOLDER_FLAG_DIRECTORY |
									MSG_FOLDER_FLAG_ELIDED)));

	  url = (char *) XP_ALLOC (XP_STRLEN (folder->data.mail.pathname) + 20);
	  if (!url) return MK_OUT_OF_MEMORY;
	  XP_STRCPY (url, "mailbox:");
	  XP_STRCAT (url, folder->data.mail.pathname);
	  msg_ClearThreadList (context);
	  break;
    case MWContextNews:
	  XP_ASSERT (context->msgframe->data.news.current_host);
	  if (! context->msgframe->data.news.current_host)
		return -1;

	  /* When opening a new newsgroup, save any changes made by reading
		 the last group. */
	  status = msg_SaveNewsRCFile (context);
	  if (status < 0) return status;

	  XP_ASSERT ((folder->flags && MSG_FOLDER_FLAG_NEWSGROUP) &&
				 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
									MSG_FOLDER_FLAG_MAIL |
									MSG_FOLDER_FLAG_DIRECTORY |
									MSG_FOLDER_FLAG_ELIDED)));
	  url = msg_MakeNewsURL (context,
							 context->msgframe->data.news.current_host,
							 folder->data.newsgroup.group_name);
	  if (!url) return MK_OUT_OF_MEMORY;
	  break;
	default:
	  XP_ASSERT (0);
	  return -1;
	}

  url_struct = NET_CreateURLStruct(url, NET_DONT_RELOAD);
  url_struct->pre_exit_fn = func;
  msg_GetURL (context, url_struct, FALSE);
  XP_FREE (url);
  return status;
}


void
msg_SaveMailSummaryChangesNow(MWContext* context)
{
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return;
  if (context->msgframe->data.mail.save_timer) {
	FE_ClearTimeout(context->msgframe->data.mail.save_timer);
	context->msgframe->data.mail.save_timer = NULL;
  }
  if (context->msgframe->data.mail.dirty) {
	msg_MailSummaryFileChanged(context, context->msgframe->opened_folder,
							   context->msgframe->msgs, NULL);
	context->msgframe->data.mail.dirty = FALSE;
  }
}


static void
msg_save_timer(void* closure) 
{
  MWContext* context = (MWContext*) closure;
  context->msgframe->data.mail.save_timer = NULL;
  msg_SaveMailSummaryChangesNow(context);
}


void
msg_SelectedMailSummaryFileChanged(MWContext* context, MSG_ThreadEntry* msg)
{
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return;
  if (msg) msg->flags |= MSG_FLAG_DIRTY;
  context->msgframe->data.mail.dirty = TRUE;
  if (context->msgframe->data.mail.save_timer) {
	FE_ClearTimeout(context->msgframe->data.mail.save_timer);
  }
  context->msgframe->data.mail.save_timer =
	FE_SetTimeout(msg_save_timer, context, 15000); /* ### Hard-coding... */
  if (!context->msgframe->data.mail.save_timer) {
	msg_SaveMailSummaryChangesNow(context);
  }
}


static void
msg_summary_file_changed_any_1(MWContext* context, MSG_Folder* folder,
							   MSG_ThreadEntry* msg, XP_File* fid)
{
  static char buf[30];

  for ( ; msg ; msg = msg->next) {
	if (msg->flags & MSG_FLAG_DIRTY) {
	  if (msg->data.mail.status_offset > 0) {
		if (*fid == NULL) {
		  *fid = XP_FileOpen(folder->data.mail.pathname, xpMailFolder,
							 XP_FILE_UPDATE_BIN);
		}
		if (*fid) {
		  uint32 position =
			msg->data.mail.status_offset + msg->data.mail.file_index;
		  XP_ASSERT(msg->data.mail.status_offset < 10000);
		  XP_FileSeek(*fid, position, SEEK_SET);
		  buf[0] = '\0';
		  if (XP_FileReadLine(buf, sizeof(buf), *fid)) {
			if (strncmp(buf, X_MOZILLA_STATUS, X_MOZILLA_STATUS_LEN) == 0 &&
				strncmp(buf + X_MOZILLA_STATUS_LEN, ": ", 2) == 0 &&
				strlen(buf) > X_MOZILLA_STATUS_LEN + 6) {
			  XP_FileSeek(*fid, position, SEEK_SET);
			  XP_FilePrintf(*fid, "%s: %04x", X_MOZILLA_STATUS,
							(msg->flags & ~MSG_FLAG_RUNTIME_ONLY));
			} else {
			  PRINTF(("Didn't find %s where expected at position %d\n"
					  "instead, found %s.\n",
					  X_MOZILLA_STATUS, position, buf));
			  msg->data.mail.status_offset = 0;
			}			
		  } else {
			PRINTF(("Couldn't read old status line at all at position %d\n",
					position));
			msg->data.mail.status_offset = 0;
		  }
		}
		if (msg->data.mail.status_offset == 0) {
		  context->msgframe->force_reparse_hack = TRUE;
		}
	  }
	  msg->flags &= ~MSG_FLAG_DIRTY;
	}
	msg_summary_file_changed_any_1(context, folder, msg->first_child, fid);
  }
}

void
msg_MailSummaryFileChanged(MWContext* context, MSG_Folder* folder,
						   struct mail_FolderData* msgs, MSG_ThreadEntry* msg)
{
  XP_File file = NULL;
  XP_StatStruct folderst;
  char *fname;

  if (context->type == MWContextNews) {
	/* #### update newsrc */
	return;
  }

  if (!folder || !msgs) return;
  fname = folder->data.mail.pathname;
  XP_ASSERT (fname);
  if (!fname) return;

  if (msg) msg->flags |= MSG_FLAG_DIRTY;

  msg_summary_file_changed_any_1(context, folder, msgs->msgs, &file);
  msg_summary_file_changed_any_1(context, folder, msgs->hiddenmsgs, &file);
  msg_summary_file_changed_any_1(context, folder, msgs->expungedmsgs, &file);

  folderst.st_mode = 0;
  if (file) {
	XP_FileClose(file);
	if (!XP_Stat(folder->data.mail.pathname, &folderst, xpMailFolder)) {
	  msgs->folderdate =
		context->msgframe->force_reparse_hack ? 0 : folderst.st_mtime;
	  msgs->foldersize =
		context->msgframe->force_reparse_hack ? 0 : folderst.st_size;
	  context->msgframe->force_reparse_hack = FALSE;
	}
  }
  
  file = XP_FileOpen(folder->data.mail.pathname, xpMailFolderSummary,
					  XP_FILE_WRITE_BIN);
  if (file) {

#ifdef XP_UNIX
	/* Clone the permissions of the folder file into the summary file.
	   (except make sure the summary is readable/writable by owner.) */
	if (!folderst.st_mode)
	  /* Stat it if we haven't already. */
	  XP_Stat(folder->data.mail.pathname, &folderst, xpMailFolder);
	if (folderst.st_mode)
	  /* Ignore errors; if it fails, bummer. */
	  fchmod (fileno(file), (folderst.st_mode | S_IWUSR | S_IRUSR));
#endif /* XP_UNIX */

	msg_WriteFolderSummaryFile(msgs, file);
	XP_FileClose(file);
  }
}  


static void
msg_free_folder_children (MWContext *context, MSG_Folder *folder)
{
  MSG_Folder *rest, *next;
  for (rest = folder->children; rest; rest = next)
	{
	  next = rest->next;

	  if (rest->flags & MSG_FOLDER_FLAG_SELECTED) {
		XP_ASSERT(context->msgframe->num_selected_folders > 0);
		if (context->msgframe->num_selected_folders > 0) {
		  context->msgframe->num_selected_folders--;
		}
	  }

	  FREEIF (rest->name);

	  if (rest->flags & MSG_FOLDER_FLAG_NEWSGROUP)
		FREEIF (rest->data.newsgroup.group_name);
	  else if (rest->flags & MSG_FOLDER_FLAG_MAIL)
		FREEIF (rest->data.mail.pathname);

	  if (rest->children)
		msg_free_folder_children (context, rest);
	  XP_FREE (rest);
	}
  folder->children = 0;
  folder->flags |= MSG_FOLDER_FLAG_ELIDED;
  context->msgframe->max_depth = 0;
}


static void
msg_toggle_folder_expansion_1(MWContext* context, MSG_Folder* folder)
{
  for ( ; folder ; folder = folder->next) {
	if (folder->flags & MSG_FOLDER_FLAG_SELECTED) {
	  folder->flags &= ~MSG_FOLDER_FLAG_SELECTED;
	  context->msgframe->num_selected_folders--;
	}
	if (folder == context->msgframe->opened_folder) {
	  msg_close_opened_folder(context);
	}
	msg_unselect_all_folders_1(context, folder->children);
  }
}


static int
msg_toggle_folder_expansion (MWContext *context, MSG_Folder *folder,
							 int line_number, int depth,
							 XP_Bool update_article_counts_p)
{
  int status;
  XP_ASSERT(folder);
  if (!folder) return -1;
  XP_ASSERT (folder->flags & MSG_FOLDER_FLAG_DIRECTORY);
  XP_ASSERT (context->msgframe);
  if (! context->msgframe) return -1;

  if (line_number < 0 || depth < 0)
	line_number = msg_line_for_folder(context, folder, &depth);

  context->msgframe->max_depth = 0;

  if (!(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	/* If any subfolders of this folder we're closing are selected, unselect
	   them.  If we're displaying the messages of any of those folders,
	   stop doing so. */
	msg_toggle_folder_expansion_1(context, folder->children);
  }

  switch (context->type)
    {
    case MWContextMail:
	  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_MAIL) &&
				 !(folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
									MSG_FOLDER_FLAG_NEWS_HOST)));
	  if (folder->children != NULL) {
		if (folder->flags & MSG_FOLDER_FLAG_ELIDED)
		  folder->flags &= (~MSG_FOLDER_FLAG_ELIDED);
		else
		  folder->flags |= MSG_FOLDER_FLAG_ELIDED;
		msg_RedisplayFoldersFromLine(context, line_number);
	  }
	  break;

    case MWContextNews:
	  XP_ASSERT ((folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
								   MSG_FOLDER_FLAG_NEWS_HOST)) &&
				 !(folder->flags & MSG_FOLDER_FLAG_MAIL));
	  if (depth == 0 && (folder->flags & MSG_FOLDER_FLAG_ELIDED))
		{
		  MSG_Folder *rest;
		  msg_ClearThreadList(context);
		  /* Opening a new news host... first close the others that
			 are open (there should be only one at most.)
		   */
		  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
					 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY));
		  for (rest = context->msgframe->folders; rest; rest = rest->next)
			if (!(rest->flags & MSG_FOLDER_FLAG_ELIDED))
			  {
				int n = msg_count_visible_folders (rest->children);
				XP_ASSERT (rest->flags & MSG_FOLDER_FLAG_DIRECTORY);
				status = msg_SaveNewsRCFile (context);
				if (status < 0) return status;
				msg_free_folder_children (context, rest);

				/* must throw away the folder data too. */
				context->msgframe->opened_folder = 0;
				if (context->msgframe->msgs)
				  msg_FreeFolderData(context->msgframe->msgs);
				context->msgframe->msgs = 0;
				context->msgframe->num_selected_msgs = 0;

				XP_ASSERT (rest->flags & MSG_FOLDER_FLAG_ELIDED);
				/* Too hard to figure out line number; redisplay them all. */
				/* FE_UpdateFolderList (context, line_number+1, n, NULL, 0); */
				msg_RedisplayFolders (context);
				if (line_number > n)
				  line_number -= n;
			  }

		  /* Now fall through, and open this one. */
		}

	  if (folder->flags & MSG_FOLDER_FLAG_ELIDED)
		{
		  /* Opening a news host or a newsgroup sub-tree.
		     When a sub-tree of a newsgroup tree is not displayed, it hasn't
			 actually been allocated.  Go get it. */
		  XP_ASSERT (folder->flags & MSG_FOLDER_FLAG_DIRECTORY);
		  status = msg_ShowNewsgroupsOfFolder (context, folder,
											   update_article_counts_p);
		  XP_ASSERT (status < 0 || !(folder->flags & MSG_FOLDER_FLAG_ELIDED));
		  status = 0;
		  msg_RedisplayFoldersFromLine (context, line_number + 1); 
		}
	  else
		{
		  /* Closing a news host or a newsgroup sub-tree.
			 When folding a sub-tree, we actually free the structures. */
/*		  int n = msg_count_visible_folders(folder->children);*/
		  XP_ASSERT (folder->flags & MSG_FOLDER_FLAG_DIRECTORY);

		  /* Closing a news host or closing a folder of newsgroups (in view all
			 groups mode) cannot be undone, because the undo state keeps
			 pointers to the MSG_Folder structures, but closing a folder
			 deletes those structures, making the undo state point into free'd
			 memory. */
		  status = msg_undo_Initialize(context);
		  if (status < 0) return status;

		  status = msg_SaveNewsRCFile (context);
		  if (status < 0) return status;
		  msg_free_folder_children (context, folder);

		  /* bug # 27248 -- 7-31-96 */
		  context->msgframe->opened_folder = 0;

		  XP_ASSERT (folder->flags & MSG_FOLDER_FLAG_ELIDED);
		  msg_RedisplayFoldersFromLine(context, line_number + 1);
		  if (depth == 0)
			{
			  msg_InterruptContext(context, TRUE); /* Interrupt any 'safe'
													  counting of articles
													  that might be going
													  on in the background.*/
			  context->msgframe->data.news.current_host = 0;

			  if (context->msgframe->data.news.group_display_type
				  == MSG_ShowAll)
				context->msgframe->data.news.group_display_type =
				  MSG_ShowSubscribedWithArticles;
			}
		}
	  break;

	default:
	  XP_ASSERT (0);
	  break;
	}
  msg_RedisplayOneFolder(context, folder, line_number, depth, TRUE);
  return 0;
}

int
msg_ToggleFolderExpansion (MWContext *context, MSG_Folder *folder,
						   XP_Bool update_article_counts_p)
{
  return msg_toggle_folder_expansion (context, folder, -1, -1,
									  update_article_counts_p);
}

void
MSG_ToggleFolderExpansion (MWContext *context, int line_number)
{
  int depth = 0;
  MSG_Folder* folder = msg_find_folder_line(context, line_number, &depth);
  XP_ASSERT(folder != NULL);
  if (folder == NULL) return;
  msg_InterruptContext(context, FALSE);
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe) return;

  /* If this is a news window, and the line being toggled is a news host,
	 then turn off "show all newsgroups" mode before opening/closing it.
	 This is so that new hosts are never opened in "show all" mode.
   */
  if (context->type == MWContextNews &&
	  depth == 0 &&
	  context->msgframe->data.news.group_display_type == MSG_ShowAll) {
	context->msgframe->data.news.group_display_type =
	  MSG_ShowSubscribedWithArticles;
	context->msgframe->max_depth = 0;
  }

  msg_toggle_folder_expansion (context, folder, line_number, depth, TRUE);
}


int
msg_SetThreading (MWContext *context, XP_Bool thread_p)
{
  context->msgframe->thread_p = thread_p;
  return msg_ReSort(context);
}

int
msg_SetSortKey (MWContext *context, MSG_SORT_KEY key)
{
  context->msgframe->sort_key = key;
  return msg_ReSort(context);
}


int
msg_SetSortForward(MWContext* context, XP_Bool forward_p)
{
  context->msgframe->sort_forward_p = forward_p;
  return msg_ReSort(context);
}

int
msg_ReSort(MWContext* context)
{
  int status;
  status = msg_Rethread(context, context->msgframe->msgs);
  msg_ReCountSelectedMessages(context);	/* In case the rethreading lost some
										   dummy messages that were selected.*/
  msg_RedisplayMessages(context);
  msg_RedisplayOneMessage(context, context->msgframe->displayed_message,
						  -1, -1, TRUE);
  context->msgframe->cacheinfo.valid = FALSE;
  msg_UpdateToolbar(context);
  return status;
}

static MSG_ThreadEntry*
msg_find_file_index(MSG_ThreadEntry* msg, uint32 num)
{
  MSG_ThreadEntry* result;
  for ( ; msg ; msg = msg->next) {
	if (msg->data.mail.file_index == num) {
	  return msg;
	}
	result = msg_find_file_index(msg->first_child, num);
	if (result) return result;
  }
  return NULL;
}



static MSG_ThreadEntry**
msg_find_parent(MSG_ThreadEntry** start, MSG_ThreadEntry* find)
{
  MSG_ThreadEntry** result;
  for (; *start ; start = &((*start)->next)) {
	if (*start == find) return start;
	result = msg_find_parent(&((*start)->first_child), find);
	if (result) return result;
  }
  return NULL;
}


/* A callback to be used as a url pre_exit_fn just after message incorporation
   has happened; this notices which message was the first one incorporated, and
   selects it.  It also does cleanup from the incorporate; in particular, it
   finishes the process of parsing new messages into the opened folder.  Some
   magic to do with replacement of `partial' messages happens as well. */
void
msg_OpenFirstIncorporated(URL_Struct *url, int status, MWContext *context)
{
  MSG_Frame* f = context->msgframe;
  MSG_ThreadEntry* msg;
  MSG_ThreadEntry** parent;
  MSG_ThreadEntry** parent2;
  MSG_Folder* folder;
  XP_StatStruct folderst;
  XP_Bool had_inc_uidl = FALSE;
  XP_ASSERT(f);
  if (!f) return;

  folder = f->opened_folder;
  if (!folder) return;

  if (f->inc_uidl) {
      /* The incorporate we just finished has to do with filling out a partial
         message.  Make a note of that fact, and then clear out the inc_uidl
         field.  It's important to clear it out early; if we hit some failure
         condition (like, the full text of the partial message is no longer on
         the server), we want to make sure that we've cleared out the inc_uidl
         field so that the next real GetNewMail really will work. */
      had_inc_uidl = TRUE;
      XP_FREE(f->inc_uidl);
      f->inc_uidl = NULL;
  }

  if (f->incparsestate) {
	/* We parsed some new messages into the opened_folder; finish up that
	   process. */
	msg_DoneParsingFolder(f->incparsestate, MSG_SortByMessageNumber,
						  TRUE, FALSE, FALSE);
	f->incparsestate = NULL;
	folder->unread_messages = context->msgframe->msgs->unread_messages;
	folder->total_messages = context->msgframe->msgs->total_messages;
	if (!XP_Stat(folder->data.mail.pathname, &folderst, xpMailFolder)) {
	  context->msgframe->msgs->folderdate = folderst.st_mtime;
	  context->msgframe->msgs->foldersize = folderst.st_size;
	}
	msg_SelectedMailSummaryFileChanged(context, NULL);
	msg_RedisplayOneFolder(context, folder, -1, -1, TRUE);
  }
  if (f && f->msgs) {
	msg = msg_find_file_index(f->msgs->msgs, f->first_incorporated_offset);
	if (msg) {
	  if (had_inc_uidl && msg->next == NULL && msg->first_child == NULL) {
		/* This is the case where the user was looking at a `partial' message,
		   and we have just downloaded the `full' version of that message.
		   At this point, the new message is the last message displayed in the
		   folder.  The stub message is the currently viewed one.  Rearrange
		   the message tree so that the stub message becomes deleted, and the
		   new one is in its place in the tree. Yow. */
		parent = msg_find_parent(&(f->msgs->msgs), msg);
		parent2 = msg_find_parent(&(f->msgs->msgs), f->displayed_message);
		if (parent && parent2) {
		  MSG_ThreadEntry* next;
		  *parent = NULL;
		  next = f->displayed_message->next;
		  msg->flags = (f->displayed_message->flags &
						~(MSG_FLAG_RUNTIME_ONLY | MSG_FLAG_PARTIAL));
		  if (f->displayed_message->first_child) {
			msg->first_child = f->displayed_message->first_child;
			f->displayed_message->first_child = NULL;
		  }
		  msg_ChangeFlag(context, f->displayed_message, MSG_FLAG_EXPUNGED, 0);
		  *parent2 = msg;
		  msg->next = next;
		}
	  }
	  msg_RedisplayMessages(context); /* Make sure the FE knows about all
										 the new messages. */
	  msg_ClearMessageArea(context);
	  msg_OpenMessage(context, msg);
	  context->msgframe->focusMsg = msg;
	} else {
	  if (f->first_incorporated_offset == 2) {
		/* Special magic value###; means that we had better go reparse this
		   folder cause what we got in memory now is caca. */
		msg_close_opened_folder(context);
		msg_SelectAndOpenFolder(context, folder, msg_ScrolltoFirstUnseen);
	  }
	}
  }
}



int
msg_ShowMessages (MWContext *context, XP_Bool all_p)
{
  Net_GetUrlExitFunc* func = NULL;
  MSG_ThreadEntry* msg;
  XP_ASSERT (context && context->msgframe);
  if (!context || !context->msgframe)
	return -1;
  if (context->msgframe->all_messages_visible_p == all_p)
	return 0;
  msg = context->msgframe->displayed_message;
  context->msgframe->all_messages_visible_p = all_p;
  if (msg) {
	if (!all_p) {
	  /* What a hack.  We know that we're about to read this article, and thus
		 mark it as read, so we make sure that it's not read now, so that we
		 will be sure it will appear in the new article list.  Hack### */
	  msg_ChangeFlag(context, msg, 0, MSG_FLAG_READ);
	}
	switch (context->type) {
	case MWContextMail:
	  context->msgframe->first_incorporated_offset = msg->data.mail.file_index;
	  func = msg_OpenFirstIncorporated;
	  break;
	case MWContextNews:
	  context->msgframe->first_incorporated_offset =
		msg->data.news.article_number;
	  func = msg_open_by_article_number;
	  break;
	default:
	  XP_ASSERT (0);
	  return -1;
	}
  }
  msg_ClearThreadList(context);
  return msg_SelectAndOpenFolder(context, context->msgframe->opened_folder,
								 func);
}


static XP_Bool msg_count_selected_1(MWContext*, MSG_ThreadEntry*, void*);
static XP_Bool msg_gather_selected_urls_1(MWContext*, MSG_ThreadEntry*, void*);

char **
msg_GetSelectedMessageURLs (MWContext *context)
{
  int32 count;
  char **result, **out;
  XP_ASSERT (context->msgframe);
  if (!context->msgframe)
	return 0;

  count = 0;
  msg_IterateOverSelectedMessages(context, msg_count_selected_1, &count);
  if (count <= 0) return 0;

  result = (char **) XP_ALLOC (sizeof(char*) * (count + 1));
  if (!result) return 0;

  out = result;
  msg_IterateOverSelectedMessages(context, msg_gather_selected_urls_1, &out);
  *out = 0;
  return result;
}


static XP_Bool
msg_count_selected_1 (MWContext* context, MSG_ThreadEntry *msg, void *closure)
{
  int32 *countP = (int32 *) closure;
  (*countP)++;
  return TRUE;
}

static XP_Bool
msg_gather_selected_urls_1 (MWContext* context, MSG_ThreadEntry *msg,
							void *closure)
{
  char ***outP = (char ***) closure;
  char *url;
  MSG_Folder *folder = context->msgframe->opened_folder;

  XP_ASSERT (folder);
  if (!folder) return FALSE;
  url = msg_GetMessageURL (context, msg);
  if (!url) return FALSE;

  **outP = url;
  (*outP)++;
  
  return TRUE;
}


/* This is as good a place as any for this monster... */


void 

MSG_Command (MWContext *context, MSG_CommandType command)
{
  int status = 0;

  XP_ASSERT (context &&
			 (context->type == MWContextMail ||
			  context->type == MWContextNews ||
			  context->type == MWContextMessageComposition));
  if (! (context &&
		 (context->type == MWContextMail ||
		  context->type == MWContextNews ||
		  context->type == MWContextMessageComposition)))
	return;

# define MSG_ASSERT_MAIL(C) \
    XP_ASSERT (C->type == MWContextMail); \
	if (C->type != MWContextMail) break
# define MSG_ASSERT_NEWS(C) \
    XP_ASSERT (C->type == MWContextNews); \
	if (C->type != MWContextNews) break
# define MSG_ASSERT_COMPOSITION(C) \
    XP_ASSERT (C->type == MWContextMessageComposition); \
	if (C->type != MWContextMessageComposition) break
# define MSG_ASSERT_MAIL_OR_NEWS(C) \
    XP_ASSERT (C->type == MWContextMail || C->type == MWContextNews); \
	if (C->type != MWContextMail && C->type != MWContextNews) break

  msg_InterruptContext(context, FALSE);

  msg_DisableUpdates(context);

  switch (command) {

  /* FILE MENU
	 =========
   */
  case MSG_GetNewMail:
	MSG_ASSERT_MAIL(context);
    status = msg_GetNewMail(context);
    break;
  case MSG_DeliverQueuedMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_DeliverQueuedMessages(context);
    break;
  case MSG_OpenFolder:
	MSG_ASSERT_MAIL(context);
	status = msg_OpenFolder(context);
	break;
  case MSG_NewFolder:
	MSG_ASSERT_MAIL(context);
	status = msg_NewFolder(context);
	break;
  case MSG_CompressFolder:
	MSG_ASSERT_MAIL(context);
    status = msg_CompressFolder(context);
    break;
  case MSG_CompressAllFolders:
	MSG_ASSERT_MAIL(context);
    status = msg_CompressAllFolders(context);
    break;
  case MSG_OpenNewsHost:
	MSG_ASSERT_NEWS(context);
	status = msg_OpenNewsHost(context);
	break;
  case MSG_AddNewsGroup:
	MSG_ASSERT_NEWS(context);
	status = msg_AddNewsGroup(context);
	break;
  case MSG_EmptyTrash:
	MSG_ASSERT_MAIL(context);
	status = msg_EmptyTrash(context);
    break;
  case MSG_Print:
	XP_ASSERT(0);
	status = -1;
    break;

  /* EDIT MENU
	 =========
   */
  case MSG_Undo:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_Undo(context);
    break;
  case MSG_Redo:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_Redo(context);
    break;
  case MSG_DeleteMessage:
	MSG_ASSERT_MAIL(context);
	status = msg_DeleteMessages(context);
    break;
  case MSG_DeleteFolder:
	MSG_ASSERT_MAIL(context);
	status = msg_DeleteFolder(context);
	break;
  case MSG_CancelMessage:
	MSG_ASSERT_NEWS(context);
	status = msg_CancelMessage(context);
    break;
  case MSG_DeleteNewsHost:
	MSG_ASSERT_NEWS(context);
	status = msg_RemoveNewsHost(context);
	break;
  case MSG_SubscribeSelectedGroups:
	MSG_ASSERT_NEWS(context);
	status = msg_SubscribeSelectedGroups(context, TRUE);
	break;
  case MSG_UnsubscribeSelectedGroups:
	MSG_ASSERT_NEWS(context);
	status = msg_SubscribeSelectedGroups(context, FALSE);
	break;
  case MSG_SelectThread:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_SelectThread(context);
	break;
  case MSG_SelectMarkedMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_SelectMarkedMessages(context);
	break;
  case MSG_SelectAllMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	msg_SelectAllMessages(context);
    break;
  case MSG_UnselectAllMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	msg_UnselectAllMessages(context);
    break;
  case MSG_MarkSelectedMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_MarkSelectedMessages(context, TRUE);
	break;
  case MSG_UnmarkSelectedMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_MarkSelectedMessages(context, FALSE);
	break;
  case MSG_FindAgain:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_Find(context, FALSE);
	break;

  /* VIEW/SORT MENUS
	 ===============
   */
  case MSG_ReSort:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_ReSort(context);
	break;
  case MSG_ThreadMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_SetThreading(context, !context->msgframe->thread_p);
    break;
  case MSG_SortBackward:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_SetSortForward(context, !context->msgframe->sort_forward_p);
	break;
  case MSG_SortByDate:
  case MSG_SortBySubject:
  case MSG_SortBySender:
  case MSG_SortByMessageNumber:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_SetSortKey(context, command);
    break;
  case MSG_Rot13Message:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_Rot13Message(context);
    break;
  case MSG_AttachmentsInline:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_AttachmentsInline(context, TRUE);
    break;
  case MSG_AttachmentsAsLinks:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_AttachmentsInline(context, FALSE);
    break;
  case MSG_AddFromNewest:
	MSG_ASSERT_NEWS(context);
	if (context->msgframe->data.news.knownarts.shouldGetOldest) {
	  context->msgframe->data.news.knownarts.shouldGetOldest = FALSE;
	  msg_ClearThreadList(context);
	}
	msg_SelectAndOpenFolder(context, context->msgframe->opened_folder,
							msg_ScrolltoFirstUnseen);
	break;
  case MSG_AddFromOldest:
	MSG_ASSERT_NEWS(context);
	if (!context->msgframe->data.news.knownarts.shouldGetOldest) {
	  context->msgframe->data.news.knownarts.shouldGetOldest = TRUE;
	  msg_ClearThreadList(context);
	}
	msg_SelectAndOpenFolder(context, context->msgframe->opened_folder,
							msg_ScrolltoFirstUnseen);
	break;
  case MSG_GetMoreMessages:
	msg_SelectAndOpenFolder(context, context->msgframe->opened_folder,
							msg_ScrolltoFirstUnseen);
	break;
  case MSG_GetAllMessages:
	XP_ASSERT(0);				/* ### Write me! */

  case MSG_WrapLongLines:
	  context->msgframe->wrap_long_lines_p =
		  ! context->msgframe->wrap_long_lines_p;
	  msg_ReOpenMessage(context);
	  break;	  

  /* MESSAGE MENU
	 ============
   */
  case MSG_EditAddressBook:
	(void) FE_GetAddressBook(context, TRUE);
	break;
  case MSG_EditAddress:
	if (context->msgframe->mail_to_sender_url) {
	  MWContext* addressbook_context = FE_GetAddressBook(context, FALSE);
	  if (addressbook_context) {
		BM_EditAddress(addressbook_context,
					   context->msgframe->mail_to_sender_url);
	  }
	}
	break;
  case MSG_PostNew:
  case MSG_PostReply:
  case MSG_PostAndMailReply:
	MSG_ASSERT_NEWS(context);
	status = msg_ComposeMessage(context, command);
    break;
  case MSG_MailNew:
  case MSG_ReplyToSender:
  case MSG_ForwardMessage:
  case MSG_ForwardMessageQuoted:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_ComposeMessage(context, command);
    break;
  case MSG_ReplyToAll:
	MSG_ASSERT_MAIL(context);
	status = msg_ComposeMessage(context, command);
    break;
  case MSG_MarkSelectedRead:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_MarkSelectedRead(context, TRUE);
    break;
  case MSG_MarkSelectedUnread:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_MarkSelectedRead(context, FALSE);
    break;
  case MSG_MarkMessage:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	if (context->msgframe->displayed_message)
	  status = msg_MarkMessage(context,
							   !(context->msgframe->displayed_message->flags &
								 MSG_FLAG_MARKED));
    break;
  case MSG_UnmarkMessage:
	XP_ASSERT(0); /* #### */
	break;
  case MSG_UnmarkAllMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_UnmarkAllMessages(context);
    break;
  case MSG_CopyMessagesInto:
  case MSG_MoveMessagesInto:
	XP_ASSERT(0);
	return;
  case MSG_SaveMessagesAs:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_SaveMessagesAs (context, FALSE);
	break;
  case MSG_SaveMessagesAsAndDelete:
	MSG_ASSERT_MAIL(context);
	status = msg_SaveMessagesAs (context, TRUE);
	break;

  /* GO MENU
	 =======
   */
  case MSG_FirstMessage:
  case MSG_NextMessage:
  case MSG_PreviousMessage:
  case MSG_LastMessage:
  case MSG_FirstUnreadMessage:
  case MSG_NextUnreadMessage:
  case MSG_PreviousUnreadMessage:
  case MSG_LastUnreadMessage:
  case MSG_FirstMarkedMessage:
  case MSG_NextMarkedMessage:
  case MSG_PreviousMarkedMessage:
  case MSG_LastMarkedMessage:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_GotoMessage(context, command);
    break;
  case MSG_MarkThreadRead:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_MarkThreadsRead(context);
    break;
  case MSG_MarkAllRead:
	MSG_ASSERT_MAIL_OR_NEWS(context);
	status = msg_MarkAllMessagesRead(context);
    break;

  /* OPTIONS MENU
	 ============
   */
  case MSG_ShowSubscribedNewsGroups:
	MSG_ASSERT_NEWS(context);
    status = msg_ShowNewsGroups(context, MSG_ShowSubscribed);
	break;
  case MSG_ShowActiveNewsGroups:
	MSG_ASSERT_NEWS(context);
    status = msg_ShowNewsGroups(context, MSG_ShowSubscribedWithArticles);
	break;
  case MSG_ShowAllNewsGroups:
	MSG_ASSERT_NEWS(context);
    status = msg_ShowNewsGroups(context, MSG_ShowAll);
    break;
  case MSG_CheckNewNewsGroups:
	MSG_ASSERT_NEWS(context);
    status = msg_CheckNewNewsGroups(context);
    break;
  case MSG_ShowAllMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_ShowMessages(context, TRUE);
    break;
  case MSG_ShowOnlyUnreadMessages:
	MSG_ASSERT_MAIL_OR_NEWS(context);
    status = msg_ShowMessages(context, FALSE);
    break;
  case MSG_ShowMicroHeaders:
  case MSG_ShowSomeHeaders:
	status = msg_SetHeaders(context, command);
	break;
  case MSG_ShowAllHeaders:
    if (context->type != MWContextMessageComposition)
	  status = msg_SetHeaders(context, command);
	else
	  /* In the composition window, it's still a toggle. */
	  status = msg_ConfigureCompositionHeaders(context,
								!msg_ShowingAllCompositionHeaders(context));
    break;

  /* COMPOSITION FILE MENU
	 =====================
   */
  case MSG_SendMessage:
	MSG_ASSERT_COMPOSITION(context);
	status = msg_DeliverMessage(context, FALSE);
	break;
  case MSG_SendMessageLater:
	MSG_ASSERT_COMPOSITION(context);
	status = msg_DeliverMessage(context, TRUE);
	break;

  case MSG_Attach:
	XP_ASSERT(0);
	break;

  case MSG_QuoteMessage:
	MSG_ASSERT_COMPOSITION(context);
	status = msg_QuoteMessage(context);
	break;

  /* COMPOSITION VIEW MENU
	 =====================
   */
  case MSG_ShowFrom:
	status = msg_ToggleCompositionHeader(context, MSG_FROM_HEADER_MASK);
	break;
  case MSG_ShowReplyTo:
	status = msg_ToggleCompositionHeader(context, MSG_REPLY_TO_HEADER_MASK);
	break;
  case MSG_ShowTo:
	status = msg_ToggleCompositionHeader(context, MSG_TO_HEADER_MASK);
	break;
  case MSG_ShowCC:
	status = msg_ToggleCompositionHeader(context, MSG_CC_HEADER_MASK);
	break;
  case MSG_ShowBCC:
	status = msg_ToggleCompositionHeader(context, MSG_BCC_HEADER_MASK);
	break;
  case MSG_ShowFCC:
	status = msg_ToggleCompositionHeader(context, MSG_FCC_HEADER_MASK);
	break;
  case MSG_ShowPostTo:
	status = msg_ToggleCompositionHeader(context, MSG_NEWSGROUPS_HEADER_MASK);
	break;
  case MSG_ShowFollowupTo:
	status = msg_ToggleCompositionHeader(context, MSG_FOLLOWUP_TO_HEADER_MASK);
	break;
  case MSG_ShowSubject:
	status = msg_ToggleCompositionHeader(context, MSG_SUBJECT_HEADER_MASK);
	break;
  case MSG_ShowAttachments:
	status = msg_ToggleCompositionHeader(context, MSG_ATTACHMENTS_HEADER_MASK);
	break;
	
  /* COMPOSITION OPTIONS MENU
	 ========================
   */
  case MSG_SendFormattedText:
	MSG_ASSERT_COMPOSITION(context);
	status = msg_SetCompositionFormattingStyle(context, FALSE /* #### */);
    break;
  case MSG_SendEncrypted:
	MSG_ASSERT_COMPOSITION(context);
	status = msg_ToggleCompositionEncrypted(context);
    break;
  case MSG_SendSigned:
	MSG_ASSERT_COMPOSITION(context);
	status = msg_ToggleCompositionSigned(context);
	break;
  case MSG_SecurityAdvisor:
	MSG_ASSERT_COMPOSITION(context);
	msg_GetURL (context,
				NET_CreateURLStruct("about:security?advisor", NET_DONT_RELOAD),
				FALSE);
	break;

  /* For the mail attach dialog */
  case MSG_AttachAsText:
	MSG_ASSERT_COMPOSITION(context);
/*####    status = msg_AttachAsText(context, bool);*/
	break;
  default:
    XP_ASSERT(0);
    break;
  }

# undef MSG_ASSERT_MAIL
# undef MSG_ASSERT_NEWS
# undef MSG_ASSERT_COMPOSITION
# undef MSG_ASSERT_MAIL_OR_NEWS

  msg_UpdateToolbar(context);
  msg_EnableUpdates(context);

  if (status < 0)
	{
	  char *alert = NET_ExplainErrorDetails(status);
	  if (alert)
		{
		  FE_Alert(context, alert);
		  XP_FREE(alert);
		}
	}
}


static MSG_ThreadEntry*
msg_find_last_message(MWContext* context)
{
  MSG_ThreadEntry* msg = context->msgframe->msgs->msgs;
  if (!msg) return NULL;
  while (msg->next || msg->first_child) {
	while (msg->next) msg = msg->next;
	if (msg->first_child) msg = msg->first_child;
  }
  return msg;
}


static XP_Bool
msg_any_messages_queued_p (MWContext *context)
{
  static MSG_Folder *folder = 0;
  if (!context || context->type != MWContextMail)
	context = XP_FindContextOfType (context, MWContextMail);
  if (context)
	{
	  for (folder = context->msgframe->folders; folder; folder = folder->next)
		{
		  if (folder->flags & MSG_FOLDER_FLAG_QUEUE)
			{
			  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_MAIL) &&
						 !(folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
											MSG_FOLDER_FLAG_NEWS_HOST |
											MSG_FOLDER_FLAG_DIRECTORY |
											MSG_FOLDER_FLAG_ELIDED)));
			  break;
			}
		}
	}
  else
	{
	  /* If a mail context doesn't exist, we still want deferred
	   * news posts to work. So say yes. */
	  return TRUE;
	}
  if (folder)
	{
	  if (folder->total_messages != 0)
		/* If it has messages, well there you go. */
		return TRUE;
	  if (folder->unread_messages < 0)
		/* If we don't know how many unread messages there are, then that
		   implies that there are non-0 messages.  So there you go again. */
		return TRUE;
	}
  return FALSE;
}




static void
msg_get_cache_info_1(MWContext* context, struct MSG_CacheInfo* info,
					 MSG_ThreadEntry* msg)
{
  for (; msg ; msg = msg->next) {
	if (msg == context->msgframe->displayed_message) {
	  info->valid = TRUE;		/* Temporarily used to indicate that we
								   have gone past the displayed message. */
	}
	if (!(msg->flags & MSG_FLAG_EXPIRED)) {
	  if (msg->flags & MSG_FLAG_MARKED) {
		info->any_marked_msgs = TRUE;
		if (msg != context->msgframe->displayed_message) {
		  info->any_marked_notdisplayed = TRUE;
		  if (info->valid) info->future_marked = TRUE;
		  else info->past_marked = TRUE;
		}
	  }
	  if (!(msg->flags & MSG_FLAG_READ)) {
		if (msg != context->msgframe->displayed_message) {
		  info->any_unread_notdisplayed = TRUE;
		  if (info->valid) info->future_unread = TRUE;
		  else info->past_unread = TRUE;
		}
	  }
	  if (msg->flags & MSG_FLAG_SELECTED) {
		if (info->selected_msgs_p) {
		  info->possibly_plural_p = TRUE; /* Multiple selected. */
		}
		info->selected_msgs_p = TRUE;
		if (msg->flags & MSG_FLAG_MARKED) {
		  info->any_marked_selected = TRUE;
		} else {
		  info->any_unmarked_selected = TRUE;
		}
		if (msg->flags & MSG_FLAG_READ) {
		  info->any_read_selected = TRUE;
		} else {
		  info->any_unread_selected = TRUE;
		}
	  }
	}
	msg_get_cache_info_1(context, info, msg->first_child);
  }
}

#ifdef BUG20390

/*
 * This routine returns true if we have a newsgroup selected.
 */
static XP_Bool
msg_get_cache_info_2(MWContext* context,
					 MSG_Folder* folder)
{
	struct MSG_Folder* curFolder;

    /* check if we ended up here with no folder */
	if (!folder) {
		return FALSE;
	}
		
	/* check if any first level children are selected newsgroups */
	for (curFolder = folder->children; curFolder ; curFolder = curFolder->next) {
		if ((curFolder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_DIRECTORY) &&
			(curFolder->flags & MSG_FOLDER_FLAG_SELECTED)&&
			!(curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) {
				return TRUE;
		}
		/* check within a directory */
		if ((curFolder->flags & MSG_FOLDER_FLAG_DIRECTORY) &&
			(curFolder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_ELIDED) &&
			!(curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) {
		  /*
		   * BUG here -- we are dropping one level of children; should be curFolder
		   * 5-20-96 -- jefft 
		   */
				if (msg_get_cache_info_2(context, curFolder->children))
					return TRUE;
		}
	}

	/* go to the next host if we need to */
	for (curFolder = folder->next; curFolder; curFolder = curFolder->next) {
		if 	(!(curFolder->flags & MSG_FOLDER_FLAG_ELIDED) &&
			(curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST))
			if (msg_get_cache_info_2(context, curFolder))
				return TRUE;
	}

  	return FALSE;
}

#else

/*
 * This routine returns true if we have a newsgroup selected.
 */
static XP_Bool
msg_get_cache_info_2 (MWContext* context,
					  MSG_Folder* folder)
{
	struct MSG_Folder* curFolder;

    /* check if we ended up here with no folder */
	if (!folder) {
		return FALSE;
	}

	for ( curFolder = folder; curFolder; curFolder = curFolder->next ) {
	  /* 
	   * if this is a host and not collapsed process the children
	   */
	  if ( curFolder->flags & MSG_FOLDER_FLAG_NEWS_HOST ) {
		if ( ! (curFolder->flags & MSG_FOLDER_FLAG_ELIDED) ) {
		  if ( msg_get_cache_info_2(context, curFolder->children) )
				return TRUE;
		}
	  }
	  else if ( curFolder->flags & MSG_FOLDER_FLAG_NEWSGROUP ) {
		/*
		 * if this is a newsgroup
		 */
		if ( curFolder->flags & MSG_FOLDER_FLAG_DIRECTORY ) {
		  /*
		   * if it's a directory and not collapsed process the children
		   */
		  if ( ! (curFolder->flags & MSG_FOLDER_FLAG_ELIDED) ) {
			if ( msg_get_cache_info_2(context, curFolder->children) )
				return TRUE;
		  }
		}
		else if ( curFolder->flags &  MSG_FOLDER_FLAG_SELECTED ) {
		  /*
		   * if it is selected return TRUE
		   */
		  return TRUE;
		}
	  }
	}

  	return FALSE;
}


#endif /* BUG20390 */



static struct MSG_CacheInfo*
msg_get_cache_info(MWContext* context)
{
  struct MSG_CacheInfo* info;
  if (!context->msgframe) {
	static struct MSG_CacheInfo silly = { 0 };
	return &silly;				/* So we can use this stuff from msg
								   composition windows and not have to add
								   additional checks for "info != NULL" */
  }
  info = &(context->msgframe->cacheinfo);
  if (!info->valid) {
	XP_MEMSET(info, 0, sizeof(*info));
	if (context->msgframe->msgs) {
	  msg_get_cache_info_1(context, info, context->msgframe->msgs->msgs);
	  if (!info->valid) {
		/* Never found the current message; must not be displaying any. */
		XP_ASSERT(context->msgframe->displayed_message == NULL);
		info->future_marked = info->past_marked;
		info->future_unread = info->past_unread;
	  }
	}
	info->valid = TRUE;
  }
  return info;
}



static XP_Bool
msg_newshost_selected_p(MWContext* context)
{
  MSG_Folder* folder;
  for (folder = context->msgframe->folders ; folder ; folder = folder->next) {
	XP_ASSERT(folder->flags & MSG_FOLDER_FLAG_NEWS_HOST);
	if (folder->flags & MSG_FOLDER_FLAG_SELECTED) return TRUE;
  }
  return FALSE;
}


int
MSG_CommandStatus (MWContext *context,
				   MSG_CommandType command,
				   XP_Bool *selectable_pP,
				   MSG_COMMAND_CHECK_STATE *selected_pP,
				   const char **display_stringP,
				   XP_Bool *plural_pP)
{
  const char *display_string = 0;
  struct MSG_CacheInfo *info = msg_get_cache_info(context);
  XP_Bool plural_p = FALSE;
  XP_Bool selectable_p = TRUE;
  XP_Bool selected_p = FALSE;
  XP_Bool selected_used_p = FALSE;
  XP_Bool selected_newsgroups_p = FALSE;
  XP_Bool mail_p = (context->type == MWContextMail);
  XP_Bool news_p = (context->type == MWContextNews);
  XP_Bool send_p = (context->type == MWContextMessageComposition);
  XP_Bool msg_p;
  XP_Bool any_msgs_p;
  XP_Bool folder_open_p;
  XP_Bool folder_selected_p;
  MWContext* addressbook_context = NULL;

  XP_ASSERT (mail_p || news_p || send_p);
  if (! (mail_p || news_p || send_p))
	return -1;

  msg_p = ((mail_p || news_p) && context->msgframe->displayed_message);
  any_msgs_p = ((mail_p || news_p) &&
				context->msgframe->msgs &&
				context->msgframe->msgs->msgs);
  folder_open_p = ((mail_p || news_p) &&
				   context->msgframe->opened_folder);
  folder_selected_p = ((mail_p || news_p) &&
	context->msgframe->num_selected_folders > 0);
  if (news_p && folder_selected_p) 
		selected_newsgroups_p = msg_get_cache_info_2(context, context->msgframe->folders);	


  /* If multiple messages are selected, set msg_p to FALSE.
	 This means that commands which operate only on the current
	 message, and not on selected messages, will not be available
	 when multiple messages are selected - only when just the
	 displayed message is selected.  Commands which operate on
	 any number of messages will look at `info->selected_msgs_p'
	 instead of `msg_p' and will be selectable because of that.
   */
  if (info && info->possibly_plural_p)
	msg_p = FALSE;

  switch (command) {

  /* FILE MENU
	 =========
   */
  case MSG_GetNewMail:
	display_string = XP_GetString(MK_MSG_GET_NEW_MAIL);
	selectable_p = mail_p; /* #### && inbox_folder_p ? */
    break;
  case MSG_DeliverQueuedMessages:
	display_string = XP_GetString(MK_MSG_DELIV_NEW_MSGS);
	selectable_p = msg_any_messages_queued_p (context);
    break;
  case MSG_OpenFolder:
	display_string = XP_GetString(MK_MSG_OPEN_FOLDER2);
	selectable_p = mail_p;
	break;
  case MSG_NewFolder:
	display_string = XP_GetString(MK_MSG_NEW_FOLDER);
	selectable_p = mail_p;
	break;
  case MSG_CompressFolder:
	display_string = XP_GetString(MK_MSG_COMPRESS_FOLDER);
	selectable_p = mail_p && folder_selected_p;
    break;
  case MSG_CompressAllFolders:
	display_string = XP_GetString(MK_MSG_COMPRESS_ALL_FOLDER);
	selectable_p = mail_p; /* #### && any_folder_needs_compression */
    break;
  case MSG_OpenNewsHost:
	display_string = XP_GetString(MK_MSG_OPEN_NEWS_HOST_ETC);
	selectable_p = news_p;
	break;
  case MSG_AddNewsGroup:
	display_string = XP_GetString(MK_MSG_ADD_NEWS_GROUP);
	selectable_p = news_p && context->msgframe->data.news.current_host != NULL;
	break;
  case MSG_EmptyTrash:
	display_string = XP_GetString(MK_MSG_EMPTY_TRASH_FOLDER);
	selectable_p = mail_p; /* #### && trash_needs_emptied */
    break;
  case MSG_Print:
	display_string = XP_GetString(MK_MSG_PRINT);
	selectable_p = msg_p;
    break;

  /* EDIT MENU
	 =========
   */
  case MSG_Undo:
	display_string = XP_GetString(MK_MSG_UNDO);
	selectable_p = (mail_p || news_p) && msg_CanUndo(context);
    break;
  case MSG_Redo:
	display_string = XP_GetString(MK_MSG_REDO);
	selectable_p = (mail_p || news_p) && msg_CanRedo(context);
    break;
  case MSG_DeleteMessage:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_DELETE_SEL_MSGS)  :
					  XP_GetString(MK_MSG_DELETE_MESSAGE));
	selectable_p = mail_p && info->selected_msgs_p;
    break;
  case MSG_DeleteFolder:
	display_string = XP_GetString(MK_MSG_DELETE_FOLDER);
	selectable_p = mail_p && folder_selected_p;
	break;
  case MSG_CancelMessage:
	display_string = XP_GetString(MK_MSG_CANCEL_MESSAGE);
	selectable_p = (news_p && msg_p &&
					context->msgframe->data.news.cancellation_allowed_p);
    break;
  case MSG_DeleteNewsHost:
	display_string = XP_GetString(MK_MSG_RMV_NEWS_HOST);
	selectable_p = news_p && msg_newshost_selected_p(context);
	break;
  case MSG_SubscribeSelectedGroups:
	display_string = XP_GetString(MK_MSG_SUBSCRIBE);
	selectable_p = news_p && folder_selected_p;
	break;
  case MSG_UnsubscribeSelectedGroups:
	display_string = XP_GetString(MK_MSG_UNSUBSCRIBE);
	selectable_p = news_p && folder_selected_p;
	break;
  case MSG_SelectThread:
	display_string = XP_GetString(MK_MSG_SELECT_THREAD);
	selectable_p = (context->msgframe->num_selected_msgs == 1);
	/* Don't use "msg_p"; we want this to work even if a dummy message is
	   selected. */
	break;
  case MSG_SelectMarkedMessages:
	display_string = XP_GetString(MK_MSG_SELECT_FLAGGED_MSG);
	selectable_p = info->any_marked_msgs;
	break;
  case MSG_SelectAllMessages:
	display_string = XP_GetString(MK_MSG_SELECT_ALL);
	selectable_p = any_msgs_p;
	break;
  case MSG_UnselectAllMessages:
	display_string = XP_GetString(MK_MSG_DESELECT_ALL_MSG);
	selectable_p = info->selected_msgs_p;
	break;
  case MSG_MarkSelectedMessages:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_FLAG_MESSAGES) : XP_GetString(MK_MSG_FLAG_MESSAGE));
	selectable_p = info->any_unmarked_selected;
	break;
  case MSG_UnmarkSelectedMessages:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_UNFLAG_MESSAGES) : XP_GetString(MK_MSG_UNFLAG_MESSAGE));
	selectable_p = info->any_marked_selected;
	break;
  case MSG_FindAgain:
	display_string = XP_GetString(MK_MSG_FIND_AGAIN);
	selectable_p = (context->msgframe->searchstring != NULL);
	break;


  /* VIEW/SORT MENUS
	 ===============
   */
  case MSG_ReSort:
	display_string = XP_GetString(MK_MSG_AGAIN);
	selectable_p = any_msgs_p; /* #### && !sorted_p */
	break;
  case MSG_ThreadMessages:
	display_string = XP_GetString(MK_MSG_THREAD_MESSAGES);
	selectable_p = mail_p || news_p;
	selected_p = ((mail_p || news_p) && context->msgframe->thread_p);
    selected_used_p = TRUE;
    break;
  case MSG_SortBackward:
	display_string = XP_GetString(MK_MSG_SORT_BACKWARD);
	selectable_p = mail_p || news_p;
	selected_p = ((mail_p || news_p) && !context->msgframe->sort_forward_p);
    selected_used_p = TRUE;
	break;
  case MSG_SortByDate:
	display_string = XP_GetString(MK_MSG_BY_DATE);
	selectable_p = mail_p || news_p;
	selected_p =((mail_p || news_p) && context->msgframe->sort_key == command);
    selected_used_p = TRUE;
    break;
  case MSG_SortBySender:
	display_string = XP_GetString(MK_MSG_BY_SENDER);
	selectable_p = mail_p || news_p;
	selected_p =((mail_p || news_p) && context->msgframe->sort_key == command);
    selected_used_p = TRUE;
    break;
  case MSG_SortBySubject:
	display_string = XP_GetString(MK_MSG_BY_SUBJECT);
	selectable_p = mail_p || news_p;
	selected_p =((mail_p || news_p) && context->msgframe->sort_key == command);
    selected_used_p = TRUE;
    break;
  case MSG_SortByMessageNumber:
	display_string = XP_GetString(MK_MSG_BY_MESSAGE_NB);
	selectable_p = mail_p || news_p;
	selected_p =((mail_p || news_p) && context->msgframe->sort_key == command);
    selected_used_p = TRUE;
    break;
  case MSG_Rot13Message:
	display_string = XP_GetString(MK_MSG_UNSCRAMBLE);
	selectable_p = msg_p;
	selected_p = ((mail_p || news_p) && context->msgframe->rot13_p);
    selected_used_p = TRUE;
    break;
  case MSG_AttachmentsInline:
	display_string = XP_GetString(MK_MSG_ATTACHMENTSINLINE);
	selectable_p = (mail_p || news_p);
	selected_p = context->msgframe->inline_attachments_p;
	selected_used_p = TRUE;
	break;
  case MSG_AttachmentsAsLinks:
	display_string = XP_GetString(MK_MSG_ATTACHMENTSASLINKS);
	selectable_p = (mail_p || news_p);
	selected_p = !context->msgframe->inline_attachments_p;
	selected_used_p = TRUE;
	break;
  case MSG_AddFromNewest:
	display_string = XP_GetString(MK_MSG_ADD_FROM_NEW_MSG);
	selectable_p = news_p;
	selected_p = news_p &&
	  !context->msgframe->data.news.knownarts.shouldGetOldest;
	selected_used_p = TRUE;
	break;
  case MSG_AddFromOldest:
	display_string = XP_GetString(MK_MSG_ADD_FROM_OLD_MSG);
	selectable_p = news_p;
	selected_p = news_p &&
	  context->msgframe->data.news.knownarts.shouldGetOldest;
	selected_used_p = TRUE;
	break;
  case MSG_GetMoreMessages:
	display_string = XP_GetString(MK_MSG_GET_MORE_MSGS);
	selectable_p = news_p;
	break;
  case MSG_GetAllMessages:
	display_string = XP_GetString(MK_MSG_GET_ALL_MSGS);
	selectable_p = /*news_p*/ FALSE;   /* ### NYI! */
	break;
  case MSG_WrapLongLines:
	display_string = XP_GetString(MK_MSG_WRAP_LONG_LINES);
	selectable_p = msg_p;
	selected_p = ((mail_p || news_p) && context->msgframe->wrap_long_lines_p);
	selected_used_p = TRUE;
	break;

  /* MESSAGE MENU
	 ============
   */
  case MSG_EditAddressBook:
	display_string = XP_GetString(MK_MSG_ADDRESS_BOOK);
	selectable_p = TRUE;
	break;
  case MSG_EditAddress:
	selectable_p = msg_p && context->msgframe->mail_to_sender_url;
	if (selectable_p) {
	  addressbook_context = FE_GetAddressBook(context, FALSE);
	  if (!addressbook_context) selectable_p = FALSE;
	}
	display_string = (selectable_p &&
					  BM_FindAddress(addressbook_context,
									 context->msgframe->mail_to_sender_url))
	  ? XP_GetString(MK_MSG_VIEW_ADDR_BK_ENTRY) : XP_GetString(MK_MSG_ADD_TO_ADDR_BOOK);
	break;
  case MSG_PostNew:
	display_string = XP_GetString(MK_MSG_NEW_NEWS_MESSAGE);
#ifdef bug_22998 
	selectable_p = news_p && selected_newsgroups_p;
#else
	selectable_p = news_p && ( selected_newsgroups_p || 
							   context->msgframe->data.news.current_host != NULL );
#endif
    break;
  case MSG_PostReply:
	display_string = XP_GetString(MK_MSG_POST_REPLY);
	selectable_p = news_p && msg_p;
    break;
  case MSG_PostAndMailReply:
	display_string = XP_GetString(MK_MSG_POST_MAIL_REPLY);
	selectable_p = news_p && msg_p;
    break;
  case MSG_MailNew:
	display_string = XP_GetString(MK_MSG_NEW_MAIL_MESSAGE);
	selectable_p = TRUE;
    break;
  case MSG_ReplyToSender:
	display_string = XP_GetString(MK_MSG_REPLY);
	selectable_p = msg_p;
    break;
  case MSG_ReplyToAll:
	display_string = XP_GetString(MK_MSG_REPLY_TO_ALL);
	selectable_p = mail_p && msg_p;
    break;
  case MSG_ForwardMessageQuoted:
    display_string = XP_GetString(MK_MSG_FORWARD_QUOTED);
	selectable_p = msg_p;
    break;
  case MSG_ForwardMessage:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_FWD_SEL_MESSAGES) :
					  XP_GetString(MK_MSG_FORWARD));
	/* This is available even if no lines are selected, since the news window
	   might be displaying a message for which no thread line exists (we can
	   get there by chasing references or other links.) */
	selectable_p = msg_p || info->selected_msgs_p;
    break;
  case MSG_MarkSelectedRead:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_MARK_SEL_AS_READ) :
					  XP_GetString(MK_MSG_MARK_AS_READ));
	selectable_p = info->any_unread_selected;
    break;
  case MSG_MarkSelectedUnread:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_MARK_SEL_AS_UNREAD) :
					  XP_GetString(MK_MSG_MARK_AS_UNREAD));
	selectable_p = info->any_read_selected;
    break;
  case MSG_MarkMessage:
	display_string = XP_GetString(MK_MSG_FLAG_MESSAGE);
	selectable_p = msg_p;
	selected_p = (msg_p &&
				  (context->msgframe->displayed_message->flags &
				   MSG_FLAG_MARKED));
    selected_used_p = TRUE;
    break;
  case MSG_UnmarkMessage:
	display_string = XP_GetString(MK_MSG_UNFLAG_MESSAGE);
	selectable_p = FALSE; /* #### */
    break;
  case MSG_UnmarkAllMessages:
	display_string = XP_GetString(MK_MSG_UNFLAG_ALL_MSGS);
	selectable_p = (any_msgs_p && info->any_marked_msgs);
    break;
  case MSG_CopyMessagesInto:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_COPY_SEL_MSGS) :
					  XP_GetString(MK_MSG_COPY_ONE));
	selectable_p = info->selected_msgs_p;
	break;
  case MSG_MoveMessagesInto:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_MOVE_SEL_MSG) :
					  XP_GetString(MK_MSG_MOVE_ONE));
	selectable_p = info->selected_msgs_p;
	break;
  case MSG_SaveMessagesAs:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_SAVE_SEL_MSGS) :
					  XP_GetString(MK_MSG_SAVE_AS));
	selectable_p = info->selected_msgs_p;
	break;
  case MSG_SaveMessagesAsAndDelete:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_MOVE_SEL_MSG_TO) :
					  XP_GetString(MK_MSG_MOVE_MSG_TO));
	selectable_p = mail_p && info->selected_msgs_p;
	break;

  /* GO MENU
	 =======
   */
  case MSG_FirstMessage:
	display_string = XP_GetString(MK_MSG_FIRST_MSG);
	selectable_p = any_msgs_p;
    break;
  case MSG_NextMessage:
	display_string = XP_GetString(MK_MSG_NEXT_MSG);
	selectable_p = any_msgs_p &&
	  (context->msgframe->displayed_message != msg_find_last_message(context));
    break;
  case MSG_PreviousMessage:
	display_string = XP_GetString(MK_MSG_PREV_MSG);
	/* We can go backwards so long as there are any messages; and
	   there is no displayed message at all; or it is not the first message
	   and the first message is not a dummy, or if msg_FindNextMessage can
	   find a previous message. */
  selectable_p =
	(any_msgs_p &&
	 context->msgframe->displayed_message != context->msgframe->msgs->msgs && 
	 (!context->msgframe->displayed_message ||
	  !(context->msgframe->msgs->msgs->flags & MSG_FLAG_EXPIRED) ||
	  msg_FindNextMessage(context, MSG_PreviousMessage) != NULL));
    break;
  case MSG_LastMessage:
	display_string = XP_GetString(MK_MSG_LAST_MSG);
	selectable_p = any_msgs_p;
    break;
  case MSG_FirstUnreadMessage:
	display_string = XP_GetString(MK_MSG_FIRST_UNREAD);

	selectable_p = info->any_unread_notdisplayed;
    break;
  case MSG_NextUnreadMessage:
	display_string = XP_GetString(MK_MSG_NEXT_UNREAD);
	selectable_p = info->future_unread;
    break;
  case MSG_PreviousUnreadMessage:
	display_string = XP_GetString(MK_MSG_PREV_UNREAD);
	selectable_p = info->past_unread;
    break;
  case MSG_LastUnreadMessage:
	display_string = XP_GetString(MK_MSG_LAST_UNREAD);
	selectable_p = info->any_unread_notdisplayed;
    break;
  case MSG_FirstMarkedMessage:
	display_string = XP_GetString(MK_MSG_FIRST_FLAGGED);
	selectable_p = info->any_marked_notdisplayed;
    break;
  case MSG_NextMarkedMessage:
	display_string = XP_GetString(MK_MSG_NEXT_FLAGGED);
	selectable_p = info->future_marked;
    break;
  case MSG_PreviousMarkedMessage:
	display_string = XP_GetString(MK_MSG_PREV_FLAGGED);
	selectable_p = info->past_marked;
    break;
  case MSG_LastMarkedMessage:
	display_string = XP_GetString(MK_MSG_LAST_FLAGGED);
	selectable_p = info->any_marked_notdisplayed;
    break;
  case MSG_MarkThreadRead:
	plural_p = info->possibly_plural_p;
	display_string = (plural_p ? XP_GetString(MK_MSG_MARK_SEL_READ) :
					  XP_GetString(MK_MSG_MARK_THREAD_READ));
	selectable_p = info->selected_msgs_p;
    break;
  case MSG_MarkAllRead:
	display_string = XP_GetString(MK_MSG_MARK_NEWSGRP_READ);
	selectable_p = news_p && folder_selected_p && selected_newsgroups_p;
    break;

  /* OPTIONS MENU
	 ============
   */
  case MSG_ShowSubscribedNewsGroups:
	display_string = XP_GetString(MK_MSG_SHOW_SUB_NEWSGRP);
	selectable_p  = news_p && context->msgframe->data.news.current_host;
	selected_p = (news_p &&
				  context->msgframe->data.news.group_display_type
				  == MSG_ShowSubscribed);
	selected_used_p = TRUE;
    break;
  case MSG_ShowActiveNewsGroups:
	display_string = XP_GetString(MK_MSG_SHOW_ACT_NEWSGRP);
	selectable_p  = news_p && context->msgframe->data.news.current_host;
	selected_p = (news_p &&
				  context->msgframe->data.news.group_display_type
				  == MSG_ShowSubscribedWithArticles);
	selected_used_p = TRUE;
    break;
  case MSG_ShowAllNewsGroups:
	display_string = XP_GetString(MK_MSG_SHOW_ALL_NEWSGRP);
	selectable_p  = news_p && context->msgframe->data.news.current_host;
	selected_p = (news_p &&
				  context->msgframe->data.news.group_display_type
				  == MSG_ShowAll);
	selected_used_p = TRUE;
    break;
  case MSG_CheckNewNewsGroups:
	display_string = XP_GetString(MK_MSG_CHECK_FOR_NEW_GRP);
	selectable_p = news_p && context->msgframe->data.news.current_host;
    break;
  case MSG_ShowAllMessages:
	display_string = XP_GetString(MK_MSG_SHOW_ALL_MSGS);
	selectable_p = mail_p || news_p;
	selected_p = ((mail_p || news_p) &&
				  context->msgframe->all_messages_visible_p);
    selected_used_p = TRUE;
    break;
  case MSG_ShowOnlyUnreadMessages:
	display_string = XP_GetString(MK_MSG_SHOW_UNREAD_ONLY);
	selectable_p = mail_p || news_p;
	selected_p = ((mail_p || news_p) &&
				  !context->msgframe->all_messages_visible_p);
    selected_used_p = TRUE;
    break;
  case MSG_ShowMicroHeaders:
	display_string = XP_GetString(MK_MSG_SHOW_MICRO_HEADERS);
	selectable_p = mail_p || news_p;
	selected_p = (!context->msgframe->all_headers_visible_p &&
				  context->msgframe->micro_headers_p);
    selected_used_p = TRUE;
    break;
  case MSG_ShowSomeHeaders:
	display_string = XP_GetString(MK_MSG_SHOW_SOME_HEADERS);
	selectable_p = mail_p || news_p;
	selected_p = (!context->msgframe->all_headers_visible_p &&
				  !context->msgframe->micro_headers_p);
    selected_used_p = TRUE;
    break;
  case MSG_ShowAllHeaders:
	display_string = XP_GetString(send_p ? MK_MSG_ALL_FIELDS :
								  MK_MSG_SHOW_ALL_HEADERS);
	selectable_p = mail_p || news_p || send_p;
	selected_p = (((mail_p || news_p) &&
				   context->msgframe->all_headers_visible_p) ||
				  (send_p && msg_ShowingAllCompositionHeaders(context)));
    selected_used_p = TRUE;
    break;

  /* COMPOSITION FILE MENU
	 =====================
   */
  case MSG_SendMessage:
	display_string = XP_GetString(MK_MSG_SEND);
	break;
  case MSG_SendMessageLater:
	display_string = XP_GetString(MK_MSG_SEND_LATER);
	break;

  case MSG_Attach:
	display_string = XP_GetString(MK_MSG_ATTACH_ETC);
	break;

  case MSG_QuoteMessage:
	display_string = XP_GetString(MK_MSG_QUOTE_MESSAGE);
	selectable_p = (send_p && MSG_GetAssociatedURL(context) != NULL);
	break;

  /* COMPOSITION VIEW MENU
	 =====================
   */
  case MSG_ShowFrom:
	display_string = XP_GetString(MK_MSG_FROM);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_FROM_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowReplyTo:
	display_string = XP_GetString(MK_MSG_REPLY_TO);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_REPLY_TO_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowTo:
	display_string = XP_GetString(MK_MSG_MAIL_TO);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_TO_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowCC:
	display_string = XP_GetString(MK_MSG_MAIL_CC);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_CC_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowBCC:
	display_string = XP_GetString(MK_MSG_MAIL_BCC);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_BCC_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowFCC:
	display_string = XP_GetString(MK_MSG_FILE_CC);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_FCC_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowPostTo:
	display_string = XP_GetString(MK_MSG_POST_TO);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_NEWSGROUPS_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowFollowupTo:
	display_string = XP_GetString(MK_MSG_FOLLOWUPS_TO);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_FOLLOWUP_TO_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowSubject:
	display_string = XP_GetString(MK_MSG_SUBJECT);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_SUBJECT_HEADER_MASK);
    selected_used_p = TRUE;
	break;
  case MSG_ShowAttachments:
	display_string = XP_GetString(MK_MSG_ATTACHMENT);
	selected_p = msg_ShowingCompositionHeader(context,
											  MSG_ATTACHMENTS_HEADER_MASK);
    selected_used_p = TRUE;
	break;

  /* COMPOSITION OPTIONS MENU
	 ========================
   */
  case MSG_SendFormattedText:
	display_string = XP_GetString(MK_MSG_SEND_FORMATTED_TEXT);
	selectable_p = send_p;
	selected_p = FALSE; /* #### */
    selected_used_p = TRUE;
    break;
  case MSG_SendEncrypted:
	display_string = XP_GetString(MK_MSG_SEND_ENCRYPTED);
	selectable_p = TRUE;
	selected_p = msg_CompositionSendingEncrypted(context);
    selected_used_p = TRUE;
    break;
  case MSG_SendSigned:
	selectable_p = TRUE;
	display_string = XP_GetString(MK_MSG_SEND_SIGNED);
	selected_p = msg_CompositionSendingSigned(context);
    selected_used_p = TRUE;
	break;
  case MSG_SecurityAdvisor:
	display_string = XP_GetString(MK_MSG_SECURITY_ADVISOR);
	break;
  case MSG_AttachAsText:
	display_string = XP_GetString(MK_MSG_ATTACH_AS_TEXT);
	selectable_p = send_p;
	selected_p = FALSE; /* #### */
    selected_used_p = TRUE;
    break;
  default:
    XP_ASSERT(0);
    break;
  }


  /* Disable all commands if this is a composition window, and a delivery
	 is in progress. */
  if (send_p && msg_DeliveryInProgress(context))
	selectable_p = FALSE;


  if (selectable_pP)
	*selectable_pP = selectable_p;
  if (selected_pP)
  {
    if (selected_used_p)
    {
      if (selected_p)
        *selected_pP = MSG_Checked;
      else
        *selected_pP = MSG_Unchecked;
    }
    else
    {
      *selected_pP = MSG_NotUsed;
    }
  }
  if (display_stringP)
	*display_stringP = display_string;
  if (plural_pP)
	*plural_pP = plural_p;

  return 0;
}



MSG_COMMAND_CHECK_STATE
MSG_GetToggleStatus(MWContext* context, MSG_CommandType command)
{
  MSG_COMMAND_CHECK_STATE result = FALSE;
  if (MSG_CommandStatus(context, command, NULL, &result, NULL, NULL) < 0) {
	return MSG_NotUsed;
  }
  return result;
}


int
MSG_SetToggleStatus(MWContext* context, MSG_CommandType command,
					MSG_COMMAND_CHECK_STATE value)
{
  MSG_COMMAND_CHECK_STATE old = MSG_GetToggleStatus(context, command);
  if (old == MSG_NotUsed) return -1;
  if (old != value) {
	MSG_Command(context, command);
	if (MSG_GetToggleStatus(context, command) != value) {
	  XP_ASSERT(0);
	  return -1;
	}
  }
  return 0;
}

#ifdef DEBUG_jwz
const char *
MSG_SelectedFolderName (MWContext *context)
{
  if (context->type != MWContextMail)
    return 0;
  else
    {
      MSG_Frame *f = context->msgframe;
      struct MSG_Folder *of = (f ? f->opened_folder : 0);
      return (of ? of->data.mail.pathname : 0);
    }
}
#endif
