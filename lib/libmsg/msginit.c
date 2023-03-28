/* -*- Mode: C; tab-width: 4 -*-
   init.c --- initialize mail/news contexts.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 19-Jun-95.
 */

#include "msg.h"
#include "xp_file.h"
#include "msgundo.h"
#include "mailsum.h"
#include "newsrc.h"


/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_MSG_CANT_CREATE_MAIL_DIR;
extern int MK_OUT_OF_MEMORY;
extern int MK_MSG_DIR_DOESNT_EXIST;


static void FreeFolders(MWContext* context, MSG_Folder* folder);

static int msg_folder_compare(const void* p1, const void* p2) 
{
  char *s1 = (*((MSG_Folder**) p1))->name;
  char *s2 = (*((MSG_Folder**) p2))->name;

#define MAGIC_FLAGS	(MSG_FOLDER_FLAG_INBOX | \
					 MSG_FOLDER_FLAG_TRASH | \
					 MSG_FOLDER_FLAG_QUEUE | \
					 MSG_FOLDER_FLAG_DRAFTS| \
					 MSG_FOLDER_FLAG_SENTMAIL)
  int magic1 = ((*((MSG_Folder**) p1))->flags) & MAGIC_FLAGS;
  int magic2 = ((*((MSG_Folder**) p2))->flags) & MAGIC_FLAGS;
#undef MAGIC_FLAGS

  /* First and foremost -- magic folders go first. */
  if (magic1 != magic2) {
	return magic2 - magic1;
  }

  /* Special hack - if one of them is a relative path and the other is not,
	 the non-relative path goes first, despite the fact that '/' < 'A'. */
  if (*s1 == '/' && *s2 != '/')
	return 1;
  else if (*s1 != '/' && *s2 == '/')
	return -1;

  return XP_FILENAMECMP (s1, s2);
}


/* Sorts the folders by name - does not recurse. */
int
msg_SortFolders(MSG_Folder **folder_list)
{
  int i, count;
  MSG_Folder **list;
  MSG_Folder *folder;
  if (!folder_list || !*folder_list)
	return 0;

  count = 0;
  for (folder = *folder_list; folder; folder = folder->next)
	count++;
  if (count < 2) return 0;

  list = (MSG_Folder**) XP_ALLOC(sizeof(MSG_Folder*) * count);
  if (!list) return MK_OUT_OF_MEMORY;

  for (folder = *folder_list,  i= 0; i < count ; folder = folder->next, i++)
	list[i] = folder;

  qsort (list, count, sizeof(MSG_Folder*), msg_folder_compare);

  for (i=count-2 ; i>=0 ; i--)
	list[i]->next = list[i+1];

  list[count-1]->next = NULL;

  *folder_list = list[0];
  XP_FREE(list);	
  return 0;
}


/*
 * Get the mail folders for the given directory.  This is a recursive
 * routine, but it only recurs the depth of the mail heirarchy, which had
 * better not be all that deep. In fact, we won't recur more than about
 * MAX_FOLDER_DEPTH times, so we won't get too screwed by self-referencing
 * symbolic links and nasty things like that.
 */

#define MAX_FOLDER_DEPTH		100

static MSG_Folder *
msg_walk_folder_tree (MWContext *context, const char* directory,
					  int curdepth, XP_Bool* isdir)
{
  XP_Dir dir;
  XP_DirEntryStruct* entry;
  MSG_Folder* first = NULL;
  MSG_Folder* prev = NULL;
  MSG_Folder* cur = NULL;
  XP_Bool dirp;

  if (isdir) *isdir = FALSE;
  if (curdepth >= MAX_FOLDER_DEPTH) return 0;	/* ### Put up dialog? */

#ifdef XP_UNIX
  {
  /* BSDI is a steaming pile.  On every other platform, if you call opendir
	 with a file, it returns NULL.  But that doesn't happen on BSDI, so we'll
	 just stat the sucker first.  And, oh, what the heck, maybe some other
	 Unix platform loses in the same way, so we'll do this on all of them. */

	XP_StatStruct folderst;
	if (XP_Stat(directory, &folderst, xpMailFolder)) {
	  return NULL;
	}

	if (!S_ISDIR(folderst.st_mode)) {
	  return NULL;
	}
  }
#endif /* XP_UNIX */

  dir = XP_OpenDir(directory, xpMailFolder);
  if (!dir) return NULL;

  if (isdir) *isdir = TRUE;

  for (entry = XP_ReadDir(dir) ; entry ; entry = XP_ReadDir(dir)) {
	if (entry->d_name[0] == '.' ||
		entry->d_name[0] == '#' ||
		entry->d_name[XP_STRLEN (entry->d_name) - 1] == '~')
	  continue;

#if defined (XP_WIN) || defined (XP_MAC)
	/* don't add summary files to the list of folders;
	   don't add popstate files to the list either, or rules (sort.dat). */
	if ((XP_STRLEN(entry->d_name) > 4 &&
		 !strcasecomp(entry->d_name + XP_STRLEN(entry->d_name) - 4, ".snm")) ||
		!strcasecomp(entry->d_name, "popstate.dat") ||
		!strcasecomp(entry->d_name, "sort.dat"))
	  continue;
#ifdef XP_MAC
	/* Ignore Eudora's TOC files */
	if ((XP_STRLEN(entry->d_name) > 4 &&
		!strcasecomp(entry->d_name + XP_STRLEN(entry->d_name) - 4, ".toc")))
		continue;
#endif
#endif

	cur = msg_MakeMailFolder (context, directory, entry->d_name);
	if (!cur) break;			/* Out of memory; treat as no more folders. */

	cur->children = msg_walk_folder_tree (context, cur->data.mail.pathname,
										  curdepth + 1, &dirp);
	if (dirp)
	  {
		cur->flags |= MSG_FOLDER_FLAG_DIRECTORY;
		/* don't lose if they create a directory called "inbox" */
		cur->flags &= (~(MSG_FOLDER_FLAG_INBOX |
						 MSG_FOLDER_FLAG_TRASH |
/*						 MSG_FOLDER_FLAG_DRAFTS | */
						 MSG_FOLDER_FLAG_QUEUE));
	  }
	else
	  {
		cur->flags &= (~MSG_FOLDER_FLAG_DIRECTORY);
	  }

	if (cur->children)
	  cur->flags |= MSG_FOLDER_FLAG_ELIDED;
	else
	  cur->flags &= (~MSG_FOLDER_FLAG_ELIDED);

	cur->next = NULL;
	if (prev) prev->next = cur;
	else {
	  XP_ASSERT(first == NULL);
	  first = cur;
	}
	prev = cur;
  }
  XP_CloseDir(dir);
  
  if (first == NULL) return NULL;

  msg_SortFolders(&first);

  return first;
}


#if 0
static void 
DumpFolders(MSG_Folder* folder, int depth) {
	int i;
	if (!folder) return;
	for ( ; folder ; folder = folder->next) {
		for (i=0 ; i<depth ; i++) {
			fprintf(stderr, "  ");
		}
		fprintf(stderr, "%s -- %s\n", folder->name,
				folder->data.mail.pathname);
		DumpFolders(folder->children, depth + 1);
	}
}
#endif


static XP_Bool
msg_check_for_inbox (MWContext* context, const char* dir, MSG_Frame* frame)	
{
  XP_Bool bReturn = TRUE;

if (frame->folders == NULL) {
	XP_Dir direntry = XP_OpenDir(dir, xpMailFolder);
	if (direntry)
	  XP_CloseDir (direntry);
	else {
	  char* buf;
	  buf = PR_smprintf(XP_GetString(MK_MSG_DIR_DOESNT_EXIST),
						dir);	/* ### Fix i18n ### */
	  if ((bReturn = FE_Confirm(context, buf))) {
		if (XP_MakeDirectory(dir, xpMailFolder) < 0) {
		  FE_Alert(context, XP_GetString(MK_MSG_CANT_CREATE_MAIL_DIR));
		}
	  }
	  XP_FREE(buf);
	}
  }
  return bReturn;
}


void
MSG_InitializeMailContext(MWContext* context)
{
  MSG_Frame* frame;
  const char* dir;
  /* Make sure the FE has called XP_AddContextToList(). */
  XP_ASSERT (context == XP_FindContextOfType (0, MWContextMail));

  msg_InterruptContext(context, FALSE);

  dir = FE_GetFolderDirectory(context);

  context->msgframe = (MSG_Frame*) XP_CALLOC(sizeof(MSG_Frame), 1);
  frame = context->msgframe;
  if (!frame) return;
  if (msg_undo_Initialize(context) < 0) {
	XP_FREE(context->msgframe);
	context->msgframe = NULL;
	return;
  }
  context->type = MWContextMail;
  frame->sort_key = MSG_SortByMessageNumber;
  frame->sort_forward_p = TRUE;
  frame->thread_p = TRUE;
  frame->all_messages_visible_p = TRUE;
  frame->inline_attachments_p = TRUE;

  frame->folders = msg_walk_folder_tree (context, dir, 0, NULL);

  msg_UpdateToolbar(context);
  msg_RedisplayFolders(context);
  msg_RebuildFolderMenus(context);

  if (msg_check_for_inbox (context, dir, frame)) {
	msg_EnsureInboxExists (context);
  }
}


void
MSG_MailDirectoryChanged (MWContext *context)
{

  const char* dir;

  context = XP_FindContextOfType (context, MWContextMail);

  if (!context || !context->msgframe ||  context->type != MWContextMail) 
	return;

  msg_InterruptContext(context, FALSE);

  dir = FE_GetFolderDirectory(context);

  /* Free up the previous folders */
  if (context->msgframe->msgs)
	msg_FreeFolderData(context->msgframe->msgs);
  FreeFolders(context, context->msgframe->folders);
  context->msgframe->folders = NULL;
  msg_undo_Initialize(context);
  context->msgframe->msgs = NULL;
  context->msgframe->num_selected_folders = 0;
  context->msgframe->num_selected_msgs = 0;

  /* Rebuild the new folders */
  context->msgframe->folders = msg_walk_folder_tree (context, dir, 0, NULL);

  msg_UpdateToolbar(context);
  msg_RedisplayFolders(context);
  msg_RebuildFolderMenus(context);

  if (msg_check_for_inbox (context, dir, context->msgframe)) {
	msg_EnsureInboxExists (context);
  }
}



void
MSG_InitializeNewsContext(MWContext* context)
{
  MSG_Frame* frame;

  /* Make sure the FE has called XP_AddContextToList(). */
  XP_ASSERT (context == XP_FindContextOfType (0, MWContextNews));

  msg_InterruptContext(context, FALSE);

  context->msgframe = (MSG_Frame*) XP_CALLOC(sizeof(MSG_Frame), 1);
  frame = context->msgframe;
  if (!frame) return;
  if (msg_undo_Initialize(context) < 0) {
	XP_FREE(context->msgframe);
	context->msgframe = NULL;
	return;
  }
  context->type = MWContextNews;
  frame->sort_key = MSG_SortByDate;
  frame->sort_forward_p = TRUE;
  frame->thread_p = TRUE;
  frame->inline_attachments_p = TRUE;
  frame->data.news.group_display_type = MSG_ShowSubscribedWithArticles;
  msg_UpdateToolbar(context);
  msg_ShowNewsHosts (context);

  /* If there is exactly one news host, arrange to select it by default. */
  if (frame->folders && !frame->folders->next)
	{
	  XP_ASSERT ((frame->folders->flags & MSG_FOLDER_FLAG_ELIDED) &&
				 (frame->folders->flags & MSG_FOLDER_FLAG_NEWS_HOST));
	  if (frame->folders->flags & MSG_FOLDER_FLAG_ELIDED)
		MSG_ToggleFolderExpansion (context, 0);
	}
}


void
MSG_NewsDirectoryChanged ( MWContext *context )
{
  MSG_NewsHost* host;
  MSG_NewsHost* next;
 
  context = XP_FindContextOfType ( context, MWContextNews );

  if ( !context || !context->msgframe || context->type != MWContextNews )
	return;

  msg_InterruptContext ( context, FALSE );
	
	/* Free up the previous folders */
  if (context->msgframe->msgs)
	msg_FreeFolderData(context->msgframe->msgs);
  FreeFolders(context, context->msgframe->folders);
  context->msgframe->folders = NULL;
  msg_undo_Initialize(context);
  context->msgframe->msgs = NULL;
  context->msgframe->num_selected_folders = 0;
  context->msgframe->num_selected_msgs = 0;

  msg_FreeNewsrcSet(context->msgframe->data.news.knownarts.set);
  context->msgframe->data.news.knownarts.set = NULL;
  msg_SaveNewsRCFile (context);
  msg_FreeNewsRCFile (context->msgframe->data.news.newsrc_file);
  context->msgframe->data.news.newsrc_file = NULL;
  host = context->msgframe->data.news.hosts;
  while (host) {
	next = host->next;
	FREEIF(host->name);
	FREEIF(host->newsrc_file_name);
	XP_FREE(host);
	host = next;
  }
  context->msgframe->data.news.hosts = NULL;
  context->msgframe->data.news.current_host = NULL;

  msg_UpdateToolbar(context);
  msg_ShowNewsHosts (context);

  /* If there is exactly one news host, arrange to select it by default. */
  if (context->msgframe->folders && !context->msgframe->folders->next)
	{
	  XP_ASSERT ((context->msgframe->folders->flags & MSG_FOLDER_FLAG_ELIDED) &&
				 (context->msgframe->folders->flags & MSG_FOLDER_FLAG_NEWS_HOST));
	  if (context->msgframe->folders->flags & MSG_FOLDER_FLAG_ELIDED)
		MSG_ToggleFolderExpansion (context, 0);
	}
}

void
msg_FreeOneFolder(MWContext* context, MSG_Folder* folder)
{
  if (folder->flags & MSG_FOLDER_FLAG_NEWSGROUP) {
	FREEIF(folder->data.newsgroup.group_name);
  } else if (folder->flags & MSG_FOLDER_FLAG_MAIL) {
	FREEIF(folder->data.mail.pathname);
  }
  FREEIF(folder->name);
  XP_FREE(folder);
}

static void 
FreeFolders(MWContext* context, MSG_Folder* folder)
{
  MSG_Folder* next;
  for ( ; folder ; folder = next) {
	next = folder->next;
	FreeFolders(context, folder->children);
	msg_FreeOneFolder(context, folder);
  }
}


static void
msg_cleanup_context(MWContext* context)
{
  msg_FreeFolderData(context->msgframe->msgs);
  FreeFolders(context, context->msgframe->folders);
  msg_undo_Cleanup(context);
  FREEIF (context->msgframe->mail_to_sender_url);
  FREEIF (context->msgframe->mail_to_all_url);
  FREEIF (context->msgframe->post_reply_url);
  FREEIF (context->msgframe->post_and_mail_url);
  FREEIF (context->msgframe->displayed_message_subject);
  FREEIF (context->msgframe->displayed_message_id);
  FREEIF (context->msgframe->searchstring);
  XP_FREE(context->msgframe);
  context->msgframe = NULL;
}  

void
MSG_CleanupMailContext(MWContext* context)
{
  msg_InterruptContext(context, TRUE);
  msg_SaveMailSummaryChangesNow(context);

  if (context->msgframe->data.mail.scan_timer) {
	FE_ClearTimeout(context->msgframe->data.mail.scan_timer);
	context->msgframe->data.mail.scan_timer = NULL;
  }

#if 0
  if (context->msgframe->data.mail.biff_timer) {
	FE_ClearTimeout(context->msgframe->data.mail.biff_timer);
	context->msgframe->data.mail.biff_timer = NULL;
  }
#endif

  msg_FreeSortInfo(context);

  msg_cleanup_context(context);
}



void
MSG_CleanupNewsContext(MWContext* context)
{
  MSG_NewsHost* host;
  MSG_NewsHost* next;
  msg_InterruptContext(context, TRUE);
  msg_FreeNewsrcSet(context->msgframe->data.news.knownarts.set);
  msg_SaveNewsRCFile (context);
  msg_FreeNewsRCFile (context->msgframe->data.news.newsrc_file);
  host = context->msgframe->data.news.hosts;
  while (host) {
	next = host->next;
	FREEIF(host->name);
	FREEIF(host->newsrc_file_name);
	XP_FREE(host);
	host = next;
  }
  context->msgframe->data.news.hosts = NULL;
  msg_cleanup_context(context);
  NET_CleanTempXOVERCache();
}
