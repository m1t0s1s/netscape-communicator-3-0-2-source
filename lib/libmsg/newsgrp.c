/* -*- Mode: C; tab-width: 4 -*-
   newsgrp.c --- commands relating to the newsgroup list window
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-May-95.
 */

#include "msg.h"
#include "mailsum.h"
#include "newsrc.h"
#include "msgundo.h"


/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_MSG_FILE_HAS_CHANGED;
extern int MK_MSG_OPEN_NEWS_HOST;
extern int MK_MIME_ERROR_WRITING_FILE;
extern int MK_MSG_ERROR_WRITING_MAIL_FOLDER;
extern int MK_MSG_ERROR_WRITING_NEWSRC;
extern int MK_MSG_MESSAGE_CANCELLED;
extern int MK_MSG_NEWSRC_UNPARSABLE;
extern int MK_MSG_NO_NEW_GROUPS;
extern int MK_MSG_N_NEW_GROUPS;
extern int MK_MSG_ONE_NEW_GROUP;
extern int MK_NNTP_TOO_MANY_ARTS;
extern int MK_OUT_OF_MEMORY;
extern int MK_UNABLE_TO_OPEN_NEWSRC;
extern int MK_NNTP_SERVER_NOT_CONFIGURED;
extern int XP_NEWS_PROMPT_ADD_NEWSGROUP;
extern int MK_MSG_REMOVE_HOST_CONFIRM;


#define MARK_SAVE_NEWS_MESSAGES_AS_READ


static int msg_sort_news_hosts (MWContext *context);


struct msg_collect_groups_data
{
  char **names;
  uint32 names_size;
  uint32 names_fp;
};

static int
msg_collect_groups_1 (struct msg_collect_groups_data *data, MSG_Folder *list)
{
  int status = 0;
  if (!list) return 0;
  while (list)
	{
	  XP_ASSERT (list->flags & MSG_FOLDER_FLAG_NEWSGROUP);
	  if (list->flags & MSG_FOLDER_FLAG_DIRECTORY)
		{
		  status = msg_collect_groups_1 (data, list->children);
		  if (status < 0) return status;
		}
	  else if (list->data.newsgroup.group_name &&
			   (list->unread_messages < 0 ||
				(list->unread_messages > 0 && list->total_messages <= 0)))
		/* Do a GROUP on this if the `unread' count is negative, or if
		   the `unread' count is non-negative, and the `total' count is
		   non-positive (this can happen when opening an unsubscribed
		   group explicitly, because of the funky ordering in that case.)
		 */
		{
		  if ((data->names_fp + 3) >= data->names_size)
			{
			  status = msg_GrowBuffer ((data->names_fp + 3),
									   sizeof(*data->names), 100,
									   (char **) &data->names,
									   &data->names_size);
			  if (status < 0) return status;
			}
		  data->names [data->names_fp++] = list->data.newsgroup.group_name;
		}
	  list = list->next;
	}
  return 0;

}

static char **
msg_collect_groups (MSG_Folder *host)
{
  int status;
  struct msg_collect_groups_data data;

  XP_ASSERT (host &&
			 (host->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
			 (host->flags & MSG_FOLDER_FLAG_DIRECTORY));
  XP_MEMSET (&data, 0, sizeof(data));

  status = msg_collect_groups_1 (&data, host->children);
  if (status < 0 || data.names_fp == 0)
	{
	  if (data.names) XP_FREE (data.names);
	  return 0;
	}
  else
	{
	  data.names [data.names_fp] = 0;
	  return data.names;
	}
}


struct MSG_Folder *
msg_NewsHostFolder (MWContext *context, MSG_NewsHost *host)
{
  struct MSG_Folder *folder;
  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews) return 0;
  for (folder = context->msgframe->folders; folder; folder = folder->next)
	{
	  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
				 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY));
	  if (folder->data.newshost.host == host)
		return folder;
	}
  return 0;
}

struct MSG_NewsHost *
msg_NewsFolderHost (MWContext *context, MSG_Folder *folder)
{
  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews) return 0;
  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
			 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY));
  if (! ((folder->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
		 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY)))
	return 0;
  XP_ASSERT (folder->data.newshost.host);
  return folder->data.newshost.host;
}


char **
MSG_GetNewsRCList (MWContext *context,
				   const char *host_and_port, XP_Bool secure_p)
{
  struct MSG_Folder *hfolder;
  struct MSG_NewsHost *host;
  int status;

  if (context->type != MWContextNews)
	return 0;

  status = msg_EnsureNewsgroupSelected (context, host_and_port, 0, secure_p);
  if (status < 0) return 0;

  host = context->msgframe->data.news.current_host;
  XP_ASSERT (host);
  if (!host) return 0;

  hfolder = msg_NewsHostFolder (context, host);
  XP_ASSERT (hfolder);
  if (!hfolder) return 0;

  return msg_collect_groups (hfolder);
}


/* The NNTP module asks the message library whether all newsgroup lines
   visible on the screen have their message counts displayed, and if not,
   will iterate over them to update them.  If this function returns false,
   then MSG_ArticleCountKnown will be called for each subscribed group.
   If that returns false, then MSG_DisplaySubscribedGroup will be called
   soon after (as soon as we get a response from the news server.)
 */
XP_Bool
MSG_ArticleCountsKnown (MWContext *context, const char *host)
{
  /* #### optimize this */

  FE_EnableClicking (context);

  return FALSE;
}


void
MSG_NewsGroupAndNumberOfID (MWContext *context, const char *message_id,
							const char **group, uint32 *message_number)
{
  struct MSG_ThreadEntry *msg;
  char *id, *id2;
  *group = 0;
  *message_number = 0;
  if (!message_id) return;

  /* If they gave us a non-news context, go find one - this is so that
	 we get the same behavior when retrieving articles for the message
	 composition window as for the news window.   Since this function
	 only looks in tables on the context and doesn't actually access
	 the network, this is ok.
   */
  if (!context || context->type != MWContextNews)
	context = XP_FindContextOfType (context, MWContextNews);
  if (!context || context->type != MWContextNews)
	return;

  if (!context->msgframe->msgs ||
	  !context->msgframe->msgs->msgs ||
	  !context->msgframe->opened_folder)
	return;

  if (! (context->msgframe->opened_folder->flags & MSG_FOLDER_FLAG_NEWSGROUP))
	return;

  id = XP_STRDUP (message_id);
  if (! id) return;
  id2 = id;
  if (*id2 == '<') id2++;
  if (id2[XP_STRLEN(id2)-1] == '>')
	id2[XP_STRLEN(id2)-1] = 0;

  msg = (struct MSG_ThreadEntry *)
	XP_Gethash (context->msgframe->msgs->message_id_table, id2, 0);
  if (msg)
	{
	  *group = context->msgframe->opened_folder->data.newsgroup.group_name;
	  *message_number = msg->data.news.article_number;
	}
  XP_FREE (id);
}


int
msg_ShowNewsGroups (MWContext *context,
					MSG_NEWSGROUP_DISPLAY_STYLE group_display_type)
{
  int status = 0;
  struct MSG_Folder *hfolder;
  XP_ASSERT (context && context->msgframe && context->type == MWContextNews);
  if (!context || !context->msgframe || context->type != MWContextNews)
	return -1;

  if (context->msgframe->data.news.group_display_type == group_display_type)
	return 0;

  if (!context->msgframe->data.news.current_host)
	/* no host currently selected */
	return 0;

  context->msgframe->max_depth = 0;
  hfolder = msg_NewsHostFolder (context,
								context->msgframe->data.news.current_host);

  XP_ASSERT (!hfolder || (hfolder->flags & MSG_FOLDER_FLAG_NEWS_HOST));

  /* close... */
  if (hfolder && !(hfolder->flags & MSG_FOLDER_FLAG_ELIDED))
	status = msg_ToggleFolderExpansion (context, hfolder, FALSE);
  if (status < 0) return status;

  context->msgframe->data.news.group_display_type = group_display_type;

  /* then reopen... */
  if (hfolder)
	status = msg_ToggleFolderExpansion (context, hfolder, TRUE);

  return status;
}

static void msg_new_groups_listed (URL_Struct *url, int status,
								   MWContext *context);

int
msg_CheckNewNewsGroups (MWContext *context)
{
  int status = 0;
  struct MSG_NewsHost *host;
  char *url;
  URL_Struct *url_struct;
  XP_ASSERT (context && context->msgframe && context->type == MWContextNews);
  if (!context || !context->msgframe || context->type != MWContextNews)
	return -1;

  host = context->msgframe->data.news.current_host;
  if (!host) return 0;

  /* If we're in "show all groups" mode, switch to "show subscribed groups"
	 mode. */
  if (context->msgframe->data.news.group_display_type == MSG_ShowAll)
	{
	  struct MSG_Folder *folder;

	  folder = msg_NewsHostFolder (context, host);
	  XP_ASSERT (folder);
	  if (folder)
		{
		  /* Turn off "show all" mode; the other two are ok. */
		  if (context->msgframe->data.news.group_display_type
			  == MSG_ShowAll)
			context->msgframe->data.news.group_display_type =
			  MSG_ShowSubscribedWithArticles;

		  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
					 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY));
		  /* Close it, then reopen it. */
		  if (!(folder->flags & MSG_FOLDER_FLAG_ELIDED))
			status = msg_ToggleFolderExpansion (context, folder, FALSE);
		  if (status < 0) return status;
		  status = msg_ToggleFolderExpansion (context, folder, FALSE);
		  if (status < 0) return status;
		}
	}

  url = msg_MakeNewsURL (context, host, "/?newgroups");
  if (!url) return MK_OUT_OF_MEMORY;
  url_struct = NET_CreateURLStruct (url, NET_DONT_RELOAD);
  XP_FREE (url);
  if (!url_struct) return MK_OUT_OF_MEMORY;
  url_struct->pre_exit_fn = msg_new_groups_listed;
  msg_GetURL (context, url_struct, FALSE);

  /* Scroll to the end of the list, which is where the new groups are
	 going to show up. */
  {
	MSG_Folder *hfolder = msg_NewsHostFolder (context, host);
	MSG_Folder *last;
	for (last = hfolder->children; last && last->next; last = last->next)
	  ;
	if (last)
	  msg_RedisplayOneFolder (context, last, -1, -1, TRUE);
  }

  return 0;
}


static void
msg_remove_news_host_files (MWContext *context, MSG_NewsHost *host)
{
  MSG_Folder* folder;
  XP_ASSERT(context->type == MWContextNews && context->msgframe);
  if (context->type != MWContextNews || !context->msgframe) return;

  for (folder = context->msgframe->folders; folder; folder = folder->next)
	{
	  MSG_NewsHost *host2;
	  XP_ASSERT(folder->flags & MSG_FOLDER_FLAG_NEWS_HOST);
	  if (!(folder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) continue;
	  host2 = folder->data.newshost.host;

	  /* Skip over the folder of the target host. */
	  if (!host2 || host == host2)
		continue;

	  /* If these two hosts share the same newsrc file, don't delete it! */
	  if (host->secure_p == host2->secure_p &&
		  !XP_FILENAMECMP (host->newsrc_file_name,
						   host2->newsrc_file_name))
		return;

	  /* If these two hosts share the same newsgroups file, don't delete it! */
	  if (host->secure_p == host2->secure_p &&
		  !XP_FILENAMECMP (host->newsrc_file_name,
						   host2->newsrc_file_name))
		return;
	}

  /* We made it through the list of all hosts without finding an overlap
	 among the files we're about to delete; so nuke them.
   */
  XP_FileRemove(host->newsrc_file_name,
				(host->secure_p ? xpSNewsRC : xpNewsRC));
  XP_FileRemove(host->newsrc_file_name, 
				(host->secure_p ? xpSNewsgroups : xpNewsgroups));
}


int
msg_RemoveNewsHost (MWContext *context)
{
  XP_Bool confirm_p;
  struct MSG_NewsHost *host;
  struct MSG_NewsHost **h;
  MSG_Folder* folder;
  MSG_Folder* prev;
  char* buf;
  XP_ASSERT(context->type == MWContextNews && context->msgframe);
  if (context->type != MWContextNews || !context->msgframe) return -1;
RESTART:
  prev = NULL;
  for (folder = context->msgframe->folders;
	   folder;
	   prev = folder, folder = folder->next) {
	if (!(folder->flags & MSG_FOLDER_FLAG_SELECTED)) continue;
	XP_ASSERT(folder->flags & MSG_FOLDER_FLAG_NEWS_HOST);
	if (!(folder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) continue;
	host = folder->data.newshost.host;
	XP_ASSERT(host);
	if (!host) continue;
	buf = PR_smprintf(XP_GetString(MK_MSG_REMOVE_HOST_CONFIRM), host->name);
	if (!buf) return MK_OUT_OF_MEMORY;
	confirm_p = FE_Confirm(context, buf);
	XP_FREE(buf);
	if (!confirm_p) return 0;
	msg_DisableUpdates(context);

	msg_remove_news_host_files (context, host);

	if (!(folder->flags & MSG_FOLDER_FLAG_ELIDED)) {
	  msg_ToggleFolderExpansion(context, folder, FALSE);
	}
	if (prev) prev->next = folder->next;
	else context->msgframe->folders = folder->next;
	folder->next = NULL;
	XP_ASSERT(folder->children == NULL); /* If this fails, then we have
											a memory leak... */
	context->msgframe->num_selected_folders--;
	msg_FreeOneFolder(context, folder);
	for (h = &(context->msgframe->data.news.hosts) ; *h ; h = &((*h)->next)) {
	  if (*h == host) {
		*h = host->next;
		FREEIF(host->name);
		FREEIF(host->newsrc_file_name);
		XP_FREE(host);
		host = NULL;
		break;
	  }
	}
	XP_ASSERT(host == NULL);	/* If this fails, we never found it, which is
								   scary. */
	msg_RedisplayFolders(context);
	msg_EnableUpdates(context);
	goto RESTART;				/* Need to go back to the beginning; otherwise,
								   if we have multiple selected, we can skip
								   some as a side effect of having deleted
								   one. */
  }
  return 0;
}


/* Constructs a canonical "news:" or "snews:" URL for the given host, being
   careful to let the host name and port be unspecified defaults when
   appropriate.   `suffix' may be 0, a group name, or a message ID.
 */
char *
msg_MakeNewsURL (MWContext *context,
				 struct MSG_NewsHost *host, const char *suffix)
{
  char *url;
  const char *def;
  const char *name;
  const char *prefix;
  uint32 port;
  int length;
  XP_ASSERT (host);
  if (!host) return 0;
  def = NET_GetNewsHost (context, FALSE);
  name = host->name;
  port = host->port;
  XP_ASSERT (port != 0);
  if (port == (uint32) (host->secure_p ? SECURE_NEWS_PORT : NEWS_PORT))
	port = 0;
  if (!port && def && !XP_STRCMP (def, name))
	name = 0;
  prefix = (host->secure_p ? "snews:" : "news:");

  XP_ASSERT (name || port == 0);

  if (!host && !suffix)
	return (XP_STRDUP (prefix));

  length =
	(name ? XP_STRLEN(name) : 0) + (suffix ? XP_STRLEN(suffix) : 0) + 30;
  url = (char *) XP_ALLOC (length);
  if (!url) return 0;
  XP_STRCPY (url, prefix);
  if (name)
	{
	  XP_STRCAT (url, "//");
	  XP_STRCAT (url, name);
	  if (port)
		PR_snprintf (url + XP_STRLEN (url), length - XP_STRLEN(url),
					 ":%lu", (unsigned long) port);
	  XP_STRCAT (url, "/");
	}
  if (suffix)
	XP_STRCAT (url, suffix);
  return url;
}


static void
msg_check_newsrc_file_date (MWContext *context)
{
  XP_StatStruct st;
  struct MSG_NewsHost *host;
  XP_ASSERT (context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return;
  if (!context->msgframe->data.news.newsrc_file ||
	  !context->msgframe->data.news.newsrc_file_date)
	return;

  host = context->msgframe->data.news.current_host;
  XP_ASSERT (host);
  if (!host) return;

  if (!XP_Stat (host->newsrc_file_name, &st,
				(host->secure_p ? xpSNewsRC : xpNewsRC)) &&
	  st.st_mtime > 0 &&
	  st.st_mtime != context->msgframe->data.news.newsrc_file_date)
	{
	  XP_Bool discard_p = FALSE;

	  if (!context->msgframe->data.news.newsrc_changed_timer)
		{
		  /* It's always ok to just reload it from disk if there were no
			 unsaved modifications in memory. */
		  discard_p = TRUE;
		}
	  else
		{
		  /* But if there were changes, ask first.
		   */
		  char *buf;
		  char *name = WH_FileName (host->newsrc_file_name,
									host->secure_p ? xpSNewsRC : xpNewsRC);
		  buf = PR_smprintf(
				   XP_GetString(MK_MSG_FILE_HAS_CHANGED),
				   (name ? name : "?"));
		  FREEIF(name);
		  if (!buf) return;
		  discard_p = !FE_Confirm (context, buf);
		  XP_FREE(buf);
		}

	  if (discard_p)
		{
		  /* They don't want us to overwrite it - so discard our changes.
			 We will reload the file the next time we are interested in it.
			 */
		  msg_FreeNewsRCFile (context->msgframe->data.news.newsrc_file);
		  context->msgframe->data.news.newsrc_file = 0;
		  context->msgframe->data.news.newsrc_file_date = 0;
		  if (context->msgframe->data.news.newsrc_changed_timer) {
			FE_ClearTimeout(context->msgframe->data.news.newsrc_changed_timer);
			context->msgframe->data.news.newsrc_changed_timer = NULL;
		  }
		}
	}
}


static int
msg_save_newsrc_file(MWContext* context)
{
  int status = 0;
  XP_File out;
  struct MSG_NewsHost *host;

  host = context->msgframe->data.news.current_host;
  XP_ASSERT (host);
  if (!host) return -1;

  msg_check_newsrc_file_date (context);
  if (!context->msgframe->data.news.newsrc_file)
	return 0;

  out = XP_FileOpen (host->newsrc_file_name, xpTemporaryNewsRC,
					 XP_FILE_WRITE_BIN);
  if (!out)
	return MK_UNABLE_TO_OPEN_NEWSRC;

  /* #### We should check here that the newsrc file is writable, and warn.
	 (open it for append, then close it right away?)  Just checking 
	 S_IWUSR is not enough.
   */

#ifdef XP_UNIX
  /* Clone the permissions of the "real" newsrc file into the temp file,
	 so that when we rename the finished temp file to the real file, the
	 preferences don't appear to change. */
  {
	XP_StatStruct st;
	if (XP_Stat (host->newsrc_file_name, &st,
				 host->secure_p ? xpSNewsRC : xpNewsRC)
		== 0)
	  /* Ignore errors; if it fails, bummer. */
	  fchmod (fileno(out), (st.st_mode | S_IRUSR | S_IWUSR));
  }
#endif /* XP_UNIX */

  status = msg_WriteNewsRCFile (context,
								context->msgframe->data.news.newsrc_file,
								out);
  if (XP_FileClose(out) != 0) {
	if (status >= 0) status = MK_MSG_ERROR_WRITING_NEWSRC;
  }
  if (status < 0)
	{
	  XP_FileRemove (host->newsrc_file_name, xpTemporaryNewsRC);
	  return status;
	}

  /* We just wrote to ~/.newsrc-HOST:PORT.tmp
	 now rename it to ~/.newsrc-HOST:PORT
   */
  if (XP_FileRename (host->newsrc_file_name, xpTemporaryNewsRC,
					 host->newsrc_file_name,
					 host->secure_p ? xpSNewsRC : xpNewsRC)
	  < 0)
	{
	  XP_FileRemove (host->newsrc_file_name, xpTemporaryNewsRC);
	  status = MK_MSG_ERROR_WRITING_NEWSRC;
	}

  context->msgframe->data.news.newsrc_file_date = 0;
  if (status >= 0)
	{
	  /* Successfully wrote and renamed the file.
	   Notice what it's current write-date is. */
      XP_StatStruct st;
	  if (!XP_Stat (host->newsrc_file_name, &st,
					(host->secure_p ? xpSNewsRC : xpNewsRC)))
		context->msgframe->data.news.newsrc_file_date = st.st_mtime;
	  else
		/* couldn't stat the file we just wrote?? */
		XP_ASSERT (0);
	}

  return status;
}


int
msg_SaveNewsRCFile (MWContext *context)
{
  XP_ASSERT (context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return 0;

  if (!context->msgframe->data.news.newsrc_changed_timer ||
	  !context->msgframe->data.news.newsrc_file)
	return 0;

  FE_ClearTimeout(context->msgframe->data.news.newsrc_changed_timer);
  context->msgframe->data.news.newsrc_changed_timer = NULL;

  return msg_save_newsrc_file(context);
}


static void
msg_newsrc_timer(void* closure)
{
  MWContext* context = (MWContext*) closure;
  int status;
  XP_ASSERT (context->type == MWContextNews && context->msgframe);
  if (!context->msgframe || context->type != MWContextNews)
	return;
  context->msgframe->data.news.newsrc_changed_timer = NULL;
  status = msg_save_newsrc_file(context);
  if (status < 0) {
	FE_Alert(context, XP_GetString(status));
  }
}


void
msg_NoteNewsrcChanged(MWContext* context)
{
  XP_ASSERT (context->type == MWContextNews && context->msgframe);
  if (!context->msgframe || context->type != MWContextNews)
	return;
  if (context->msgframe->data.news.newsrc_changed_timer) {
	FE_ClearTimeout(context->msgframe->data.news.newsrc_changed_timer);
  }
  context->msgframe->data.news.newsrc_changed_timer =
	FE_SetTimeout(msg_newsrc_timer, context, 15 * 60 * 1000); /* ### Hard-coding. */
}




/* The newsgroup list window
   (For now the message list window is in mailsum.c)
 */

static MSG_Folder *
msg_make_newsgroup_folder (const char *prefix, const char *suffix,
						   int32 descendant_group_count)
{
  char *name;
  MSG_Folder *f;
  char name_suffix [50];
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

  f->flags |= MSG_FOLDER_FLAG_NEWSGROUP;

  if (descendant_group_count > 0)
	{
	  f->flags |= (MSG_FOLDER_FLAG_DIRECTORY|MSG_FOLDER_FLAG_ELIDED);
	  PR_snprintf (name_suffix, sizeof(name_suffix),
				   ".* (%lu groups)", (unsigned long) descendant_group_count);
	}
  else
	{
	  f->unread_messages = -1;
	  *name_suffix = 0;
	}

  *name = 0;
  if (prefix && *prefix)
	XP_STRCPY (name, prefix);
  if (prefix && *prefix && suffix && *suffix && name[XP_STRLEN(name)-1] != '.')
	XP_STRCAT (name, ".");
  if (suffix && *suffix == '.')
	suffix++;
  if (suffix)
	XP_STRCAT (name, suffix);

  f->data.newsgroup.group_name = name;

  f->name = (char *) XP_ALLOC (XP_STRLEN (name) + XP_STRLEN (name_suffix) + 1);
  if (!f->name)
	{
	  XP_FREE (f->data.newsgroup.group_name);
	  XP_FREE (f);
	  return 0;
	}
  XP_STRCPY (f->name, name);
  if (*name_suffix)
	XP_STRCAT (f->name, name_suffix);

  return f;
}



static struct MSG_Folder *
msg_push_newsgroup_line (MWContext* context,
						 const char *parent_name,
						 const char *child_name,
						 int32 descendant_group_count,
						 void *closure)
{
  struct MSG_Folder ***tail_ptrP = (struct MSG_Folder ***) closure;
  struct MSG_Folder *f;

  f = msg_make_newsgroup_folder (parent_name, child_name,
								 descendant_group_count);
  if (!f) return 0;

  if (context->msgframe->data.news.newsrc_file)
	{
	  msg_NewsRCSet *set =
		msg_GetNewsRCSet (context, f->data.newsgroup.group_name,
						  context->msgframe->data.news.newsrc_file);
	  if (set && msg_GetSubscribed(set))
		f->flags |= MSG_FOLDER_FLAG_SUBSCRIBED;
	}

  /* #### MSG_FOLDER_FLAG_MODERATED */

  **tail_ptrP = f;
  *tail_ptrP = &f->next;

  return f;
}

static int
msg_show_all_newsgroups_mapper (MWContext *context,
								const char *parent_name,
								const char *child_name,
								XP_Bool group_p,
								int32 descendant_group_count,
								void *closure)
{
  struct MSG_Folder *f;
  XP_ASSERT (group_p || descendant_group_count > 0);
  if (group_p)
	{
	  f = msg_push_newsgroup_line (context, parent_name, child_name, 0,
								   closure);
	  if (!f) return MK_OUT_OF_MEMORY;
	}
  if (descendant_group_count > 0)
	{
	  f = msg_push_newsgroup_line (context, parent_name, child_name,
								   descendant_group_count, closure);
	  if (!f) return MK_OUT_OF_MEMORY;
	}
  return 0;
}


static int
msg_show_subscribed_newsgroups_mapper (MWContext *context,
									   const char *group_name,
									   void *closure)
{
  struct MSG_Folder ***tail_ptrP = (struct MSG_Folder ***) closure;
  struct MSG_Folder *f = msg_make_newsgroup_folder (0, group_name, 0);
  if (!f) return MK_OUT_OF_MEMORY;
  f->flags |= MSG_FOLDER_FLAG_SUBSCRIBED;
  /* #### MSG_FOLDER_FLAG_MODERATED */
  **tail_ptrP = f;
  *tail_ptrP = &f->next;
  return 0;
}


static int
msg_compute_news_folder_name (MWContext *context, struct MSG_Folder *folder)
{
  const char *def = NET_GetNewsHost (context, FALSE);
  struct MSG_NewsHost *host;
  const char *suffix;
  char *new_name;
  uint32 port;
  int length;
  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;
  XP_ASSERT (folder && (folder->flags & MSG_FOLDER_FLAG_NEWS_HOST));
  if (!folder || !(folder->flags & MSG_FOLDER_FLAG_NEWS_HOST)) return -1;

  host = msg_NewsFolderHost (context, folder);
  if(!host) return -1;

  port = host->port;
  XP_ASSERT (port != 0);
  if (port == (uint32) (host->secure_p ? SECURE_NEWS_PORT : NEWS_PORT))
	port = 0;

  if (def && port == 0 && !XP_STRCMP (host->name, def))
	{
	  if (host->secure_p)
		suffix = " (default news host; secure)";
	  else
		suffix = " (default news host)";
	}
  else if (host->secure_p)
	suffix = " (secure)";
  else
	suffix = "";

  length = XP_STRLEN (host->name) + XP_STRLEN (suffix) + 20;
  new_name = (char *) XP_ALLOC (length);
  if (!new_name) return MK_OUT_OF_MEMORY;

  XP_STRCPY (new_name, host->name);
  if (port)
	PR_snprintf (new_name + XP_STRLEN (new_name), length - XP_STRLEN(new_name),
				 ":%lu", (unsigned long) port);
  XP_STRCAT (new_name, suffix);

  FREEIF (folder->name);
  folder->name = new_name;

  return 0;
}



struct MSG_Folder *
msg_make_news_folder_for_host (MWContext *context, struct MSG_NewsHost *host)
{
  struct MSG_Folder *folder = 0;
  int status;

  XP_ASSERT (host && host->name && host->newsrc_file_name);
  if (!host || !host->name || !host->newsrc_file_name)
	return 0;

  folder = XP_NEW (MSG_Folder);
  if (!folder) return 0;
  XP_MEMSET (folder, 0, sizeof(*folder));
  
  folder->flags = (MSG_FOLDER_FLAG_NEWS_HOST |
				   MSG_FOLDER_FLAG_DIRECTORY |
				   MSG_FOLDER_FLAG_ELIDED);
  folder->data.newshost.host = host;

  status = msg_compute_news_folder_name (context, folder);
  if (status < 0) return 0;

  return folder;
}


static struct MSG_NewsHost *
msg_make_host_and_folder_for_file (MWContext *context, const char *file,
								   const char *default_host);

/* Show all of the root-level groups on the given news host.
   The host may be a host name, or the name of a newsrc file
   (sans directory.)
 */
int
msg_ShowNewsHosts (MWContext *context)
{
  /* If the "default news host" isn't set in prefs, then NET_GetNewsHost()
	 will pop up a warning dialog, then return NULL. */
  const char *def = NET_GetNewsHost (context, TRUE);
  char **newsrcs = XP_GetNewsRCFiles();
  char **rest = newsrcs;
  int status = 0;
  XP_Bool got_default_p = FALSE;

  /* Make entries in the folder list for each news host.
	 If a host is there already, don't lose.
	 If other hosts are there already, still don't lose.
   */
  while (rest && *rest)
	{
	  struct MSG_NewsHost *h;

#ifndef NDEBUG
	  {
		/* Some versions of XP_ASSERT choke on embedded strings.  Sigh. */
		XP_Bool newsrc_name_ok = (!XP_STRCMP(*rest, "newsrc") ||
								  !XP_STRCMP(*rest, "snewsrc") ||
								  !XP_STRNCMP(*rest, "newsrc-", 7) ||
								  !XP_STRNCMP(*rest, "snewsrc-", 8));
		XP_ASSERT(newsrc_name_ok);
	  }
#endif

	  if (! (!XP_STRCMP(*rest, "newsrc") ||
			 !XP_STRCMP(*rest, "snewsrc") ||
			 !XP_STRNCMP(*rest, "newsrc-", 7) ||
			 !XP_STRNCMP(*rest, "snewsrc-", 8)))
		{
		  rest++;
		  continue;
		}

	  h = msg_make_host_and_folder_for_file (context, *rest, def);
	  if (!h)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}

	  if (!got_default_p)
		got_default_p = (def &&
						 h->name &&
						 !h->secure_p &&
						 h->port == NEWS_PORT &&
						 !strcasecomp (def, h->name));

	  rest++;
	}

  /* If the default news host is configured, but it wasn't in the list of
	 newsrc files (there was no ".newsrc-HOST" for it, and no ".newsrc"),
	 add it to the list.
   */
  if (def && !got_default_p)
	{
	  struct MSG_NewsHost *h =
		msg_make_host_and_folder_for_file (context, "newsrc", def);
	  if (! h)
		status = MK_OUT_OF_MEMORY;
	}

 FAIL:

  if (newsrcs)
	{
	  for (rest = newsrcs; *rest; rest++)
		XP_FREE (*rest);
	  XP_FREE (newsrcs);
	}

  if (status < 0)
	msg_sort_news_hosts (context);
  else
	status = msg_sort_news_hosts (context);

  return status;
}

static struct MSG_NewsHost *
msg_make_host_and_folder_for_file (MWContext *context, const char *file,
								   const char *default_host)
{
  char *name;
  uint32 port = 0;
  XP_Bool secure_p;
  int prefix_length;
  struct MSG_NewsHost *h;
  struct MSG_Folder *f;

  name = XP_STRCHR (file, '-');
  prefix_length = (name ? (name - file + 1) : XP_STRLEN (file));
  if (name)
	{
	  char *port_string;
	  name++;
	  name = XP_STRDUP (name);
	  if (!name)
		return 0;

	  port_string = XP_STRCHR (name, ':');
	  if (port_string)
		{
		  char dummy;
		  int n;
		  unsigned long pl = 0;
		  name [port_string - name] = 0;
		  port_string++;
		  port = 0;
		  n = sscanf (port_string, "%lu%c", &pl, &dummy);
		  port = pl;
		  XP_ASSERT (n == 1);
		  XP_ASSERT (port != 0);
		}
	}
  else
	{
	  if (!default_host)
		{
		  /* no default news host configured, so no host to map
			 the file ".newsrc" to... ignore it until one shows up.
		   */
		  return 0;
		}

	  name = XP_STRDUP (default_host);
	  if (!name)
		return 0;
	  port = 0;
	}

  secure_p = (*file == 's' || *file == 'S');

  if (port == 0)
	port = (secure_p ? SECURE_NEWS_PORT : NEWS_PORT);

  /* Look for an existing news host that matches this.
   */
  for (h = context->msgframe->data.news.hosts; h; h = h->next)
	if (h->port == port &&
		h->secure_p == secure_p &&
		!XP_STRCMP (h->name, name))
	  break;

  /* Create one if it isn't there.
   */
  if (h)
	{
	  XP_FREE (name);
	  name = 0;
	}
  else
	{
	  h = XP_NEW (struct MSG_NewsHost);
	  if (! h)
		{
		  XP_FREE (name);
		  name = 0;
		  return 0;
		}
	  XP_MEMSET (h, 0, sizeof(*h));
	  h->name = name;
	  h->port = port;
	  h->secure_p = secure_p;

	  h->next = context->msgframe->data.news.hosts;
	  context->msgframe->data.news.hosts = h;
	}

  /* If there was a host already, there's probably a folder. */
  if (h->newsrc_file_name)
	f = msg_NewsHostFolder (context, h);
  else
	f = 0; /* otherwise, we just created the host. */

  /* Store the (possibly new) newsrc file name into this host.
   */
  FREEIF (h->newsrc_file_name);
  h->newsrc_file_name = XP_STRDUP (file + prefix_length);
  if (!f)
	{
	  f = msg_make_news_folder_for_host (context, h);
	  if (!f)
		return 0;
	  f->next = context->msgframe->folders;
	  context->msgframe->folders = f;
	}

  return h;
}




static int
msg_update_article_counts (MWContext *context, struct MSG_NewsHost *host)
{
  char *newsrc_url;
  URL_Struct *url;
  XP_ASSERT (host);
  if (!host) return -1;
  newsrc_url = msg_MakeNewsURL (context, host, 0);
  if (!newsrc_url) return MK_OUT_OF_MEMORY;
  url = NET_CreateURLStruct (newsrc_url, NET_NORMAL_RELOAD);
  XP_FREE (newsrc_url);
  if (!url) return MK_OUT_OF_MEMORY;
  url->internal_url = TRUE;
  msg_GetURL (context, url, TRUE);
  return 0;
}

static void msg_newsgroup_list_read (URL_Struct *url, int status,
									 MWContext *context);


int
msg_ShowNewsgroupsOfFolder (MWContext *context, MSG_Folder *folder,
							XP_Bool update_article_counts_p)
{
  int status = 0;
  struct MSG_Folder **tail_ptr;
  struct MSG_NewsHost *host;
  
  XP_ASSERT (context->type == MWContextNews &&
			 folder &&
			 !folder->children &&
			 (folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
							   MSG_FOLDER_FLAG_NEWSGROUP)) &&
			 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY) &&
			 (folder->flags & MSG_FOLDER_FLAG_ELIDED));
  if (! (context->type == MWContextNews &&
		 folder &&
		 !folder->children &&
		 (folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
						   MSG_FOLDER_FLAG_NEWSGROUP)) &&
		 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY) &&
		 (folder->flags & MSG_FOLDER_FLAG_ELIDED)))
	return -1;

  tail_ptr = &folder->children;

  if (folder->flags & MSG_FOLDER_FLAG_NEWS_HOST)
	{
	  /* Opening a new news host - figure out which one. */
	  struct MSG_NewsHost *host;
	  status = msg_SaveNewsRCFile (context);
	  if (status < 0) return status;
	  if (context->msgframe->data.news.newsrc_file)
		{
		  msg_FreeNewsRCFile (context->msgframe->data.news.newsrc_file);
		  context->msgframe->data.news.newsrc_file = 0;
		  context->msgframe->data.news.newsrc_file_date = 0;
		  if (context->msgframe->data.news.newsrc_changed_timer) {
			FE_ClearTimeout(context->msgframe->data.news.newsrc_changed_timer);
			context->msgframe->data.news.newsrc_changed_timer = NULL;
		  }
		}
	  host = msg_NewsFolderHost (context, folder);
	  XP_ASSERT (host);
	  context->msgframe->data.news.current_host = host;
	}

  host = context->msgframe->data.news.current_host;
  XP_ASSERT (host && host->newsrc_file_name);

  if (!context->msgframe->data.news.newsrc_file)
	{
	  XP_File file = XP_FileOpen (host->newsrc_file_name,
								  (host->secure_p ? xpSNewsRC : xpNewsRC),
								  XP_FILE_READ_BIN);

	  if (file)
		{
		  XP_StatStruct st;
		  context->msgframe->data.news.newsrc_file_date = st.st_mtime = 0;
		  XP_Stat (host->newsrc_file_name, &st,
				   (host->secure_p ? xpSNewsRC : xpNewsRC));

		  context->msgframe->data.news.newsrc_file =
			msg_ReadNewsRCFile (context, file);

		  XP_FileClose (file);
		  if (!context->msgframe->data.news.newsrc_file)
			return MK_MSG_NEWSRC_UNPARSABLE;
		  context->msgframe->data.news.newsrc_file_date = st.st_mtime;
		}
	  else
		{
		  /* We are opening a host which has no newsrc file.

			 If this is the default host, then we want to subscribe the
			 user to some default groups, and show them right away.  To
			 do this, we need to create some dummy newsrc file data (but
			 don't need to create a newsrc file right away.)

			 If this is not the default host, then there's a reasonable
			 chance that it doesn't even have a full news feed, so we have
			 no way of knowing what groups are there.  In that case, why
			 don't we just switch to "all groups" mode...  This will download
			 the active file as the first action, giving the user that annoying
			 "this might take a while" dialog.  Sigh.
		   */
		  const char *def = NET_GetNewsHost (context, FALSE);
		  context->msgframe->data.news.newsrc_file_date = 0;
		  if (def && host->port == NEWS_PORT && !XP_STRCMP (def, host->name))
			{
			  context->msgframe->data.news.newsrc_file =
				msg_MakeDefaultNewsRCFileData (context, TRUE);
			}
		  else
			{
			  /* it's a non-default host with no newsrc. */
			  context->msgframe->data.news.group_display_type = MSG_ShowAll;
			  context->msgframe->data.news.newsrc_file =
				msg_MakeDefaultNewsRCFileData (context, FALSE);
			}
		}
	}

  if (context->msgframe->data.news.group_display_type != MSG_ShowAll)
	{
	  /* Populate the window with the contents of the newsrc file. */
	  status = msg_MapNewsRCGroupNames (context,
										msg_show_subscribed_newsgroups_mapper,
										(void *) &tail_ptr);
	}
  else
	{
	  char* newsrc_file_name = host->newsrc_file_name;
	  time_t t;
	  if (newsrc_file_name == NULL || *newsrc_file_name == '\0') {
		newsrc_file_name = (char*) NET_GetNewsHost(context, FALSE);
	  }
	  t = NET_NewsgroupsLastUpdatedTime (newsrc_file_name, host->secure_p);
	  if (!t && update_article_counts_p)
		{
		  char *url = msg_MakeNewsURL (context, host, "*");
		  URL_Struct *url_struct;
		  if (!url) return MK_OUT_OF_MEMORY;
		  url_struct = NET_CreateURLStruct (url, NET_DONT_RELOAD);
		  XP_FREE (url);
		  if (!url_struct) return MK_OUT_OF_MEMORY;
		  url_struct->pre_exit_fn = msg_newsgroup_list_read;
		  status = msg_GetURL (context, url_struct, FALSE);

		  /* Downloading the list will call into us again to do this. */
		  update_article_counts_p = FALSE;
		}
	  else if (t)
		{
		  /* Populate the window with the root of the news hierarchy. */
		  status =
			msg_MapNewsgroupNames (context,
								   newsrc_file_name,
								   host->secure_p,
								   ((folder->flags & MSG_FOLDER_FLAG_NEWS_HOST)
									? 0
									: folder->data.newsgroup.group_name),
								   msg_show_all_newsgroups_mapper,
								   (void *) &tail_ptr);
		}
	  /* Else, we don't have the newsgroup list on disk (t == 0) and we are
		 being called with update_article_counts_p == FALSE, meaning we can't
		 open a URL to go get them.  So...  we leave it in "show all groups"
		 mode, but with no groups showing (or at most, the one group which
		 was present in the URL if it was in "news://host/group" form.
	   */
	}

  if (status < 0) return status;

  /* Now go and fill in the article counts by talking to the server. */
  if (update_article_counts_p)
	status = msg_update_article_counts (context, host);

  folder->flags &= (~MSG_FOLDER_FLAG_ELIDED);
  return status;
}


/* Called as the pre-exit routine of the URL we loaded to download the
   active file.  Now that we have the active file, we can re-invoke
   msg_ShowNewsgroupsOfFolder() and it will actually work...
 */
static void
msg_newsgroup_list_read (URL_Struct *url, int status, MWContext *context)
{
  struct MSG_Folder *folder;
  if (status < 0) return;
  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews) return;
  XP_ASSERT (context->msgframe->data.news.current_host);
  if (!context->msgframe->data.news.current_host) return;

  /* #### Assumes the url points at the current_host */

  folder = msg_NewsHostFolder (context,
							   context->msgframe->data.news.current_host);
  XP_ASSERT (folder &&
			 (folder->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
			 (folder->flags & MSG_FOLDER_FLAG_DIRECTORY));
  if (!folder) return;
  XP_ASSERT (!folder->children);
  if (folder->children) return;
  folder->flags |= MSG_FOLDER_FLAG_ELIDED;
  msg_ShowNewsgroupsOfFolder (context, folder, TRUE);
  msg_RedisplayFolders (context);
}


static struct MSG_Folder *
msg_find_newsgroup_folder_1 (const char *group, MSG_Folder *list)
{
  if (!list) return 0;
  while (list)
	{
	  if (list->flags & MSG_FOLDER_FLAG_DIRECTORY)
		{
		  struct MSG_Folder *f =
			msg_find_newsgroup_folder_1 (group, list->children);
		  if (f) return f;
		}
	  else if (!XP_STRCMP (group, list->data.newsgroup.group_name))
		return list;

	  list = list->next;
	}
  return 0;
}

struct MSG_Folder *
msg_FindNewsgroupFolder (MWContext *context, const char *group)
{
  struct MSG_Folder *folder;

  XP_ASSERT (context->msgframe &&
			 context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	{
	  XP_ASSERT (0); /* can only open newsrc: urls in a news window */
	  return 0;
	}

  if (!context->msgframe->data.news.current_host)
	return 0;

  folder = msg_NewsHostFolder (context,
							   context->msgframe->data.news.current_host);
  XP_ASSERT (folder);
  if (! folder) return 0;

  /* Find the folder line for this group. */
  /* #### This could maybe stand to be more efficient. */
  return msg_find_newsgroup_folder_1 (group, folder->children);
}


static struct MSG_Folder *
msg_find_news_folder (const char *group_name, struct MSG_Folder *folders)
{
  while (folders)
	{
	  if (!(folders->flags & MSG_FOLDER_FLAG_NEWS_HOST) &&
		  !XP_STRCMP (group_name, folders->data.newsgroup.group_name))
		return folders;
	  if (folders->children)
		{
		  struct MSG_Folder *f =
			msg_find_news_folder (group_name, folders->children);
		  if (f) return f;
		}
	  folders = folders->next;
	}
  return 0;
}

static int
msg_host_sort_fn (const void *a, const void *b)
{
  register struct MSG_Folder *ma = *(struct MSG_Folder **) a;
  register struct MSG_Folder *mb = *(struct MSG_Folder **) b;
  XP_ASSERT (ma && mb);
  return (strcasecomp (ma->name, mb->name));
}

static int
msg_sort_news_hosts (MWContext *context)
{
  int32 i, count = 0;
  int status = 0;
  struct MSG_Folder **folders, *rest;
  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;

  for (rest = context->msgframe->folders; rest; rest = rest->next)
	count++;
  if (count == 0)
	{
	  msg_RedisplayFolders (context);
	  return 0;
	}
  folders = (MSG_Folder **) XP_ALLOC ((count + 1) * sizeof(*folders));
  if (!folders) return MK_OUT_OF_MEMORY;

  for (i = 0, rest = context->msgframe->folders; rest; i++, rest = rest->next)
	{
	  folders[i] = rest;
	  /* In case the default host has changed, recompute the folder line. */
	  status = msg_compute_news_folder_name (context, rest);
	  if (status < 0) return status;
	}
  folders[i] = 0;
  qsort ((char *) folders, count, sizeof (struct MSG_Folder *),
		 msg_host_sort_fn);
  for (i = 0; i < count; i++)
	folders[i]->next = folders[i+1];
  folders[i] = 0;
  context->msgframe->folders = folders[0];
  XP_FREE (folders);
  msg_RedisplayFolders (context);
  return 0;
}


static int
msg_ensure_newsgroup_listed (MWContext *context,
							 const char *host_and_port,
							 const char *group_name,
							 XP_Bool secure_p,
							 XP_Bool sort_p,
							 XP_Bool select_host_p,
							 MSG_Folder **hfolderP,
							 MSG_Folder **gfolderP)
{
  char *host_name = NULL;
  char *colon, *at;
  uint32 port = 0;
  struct MSG_NewsHost *host;
  struct MSG_Folder *hfolder = NULL;
  struct MSG_Folder *gfolder = NULL;
  XP_Bool created_host_p = FALSE;
  int length;

  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;

  if (gfolderP) *gfolderP = 0;
  if (hfolderP) *hfolderP = 0;

  /* There may be authinfo at the front of the host - it could be of
	 the form "user:password@host:port", so take off everything before
	 the first at-sign.
   */
  if (host_and_port) {
	at = XP_STRCHR (host_and_port, '@');
	if (at) host_and_port = at + 1;

	host_name = XP_STRDUP (host_and_port);
	if (!host_name) return MK_OUT_OF_MEMORY;
  }

  /* Find the host that corresponds to this URL. */
  if (!host_name || !*host_name)
	{
	  const char* tmp;
	  FREEIF (host_name);
	  tmp = NET_GetNewsHost (context, FALSE);
	  if (!tmp) return MK_NNTP_SERVER_NOT_CONFIGURED;
	  host_name = XP_STRDUP(tmp);
	  if (!host_name) return MK_OUT_OF_MEMORY;
	}
  colon = XP_STRCHR (host_name, ':');
  if (colon)
	{
	  *colon = 0;
	  sscanf (colon+1, " %lu ", &port);
	}
  if (port == 0)
	port = (uint32) (secure_p ? SECURE_NEWS_PORT : NEWS_PORT);

  for (host = context->msgframe->data.news.hosts; host; host = host->next)
	if (host->port == port && !XP_STRCMP (host->name, host_name))
	  break;

  if (host)
	{
	  XP_FREE (host_name);
	  host_name = 0;
	}
  else
	{
	  /* No such host?  Add one! */
	  host = XP_NEW (struct MSG_NewsHost);
	  if (! host) return MK_OUT_OF_MEMORY;
	  XP_MEMSET (host, 0, sizeof(*host));
	  host->name = host_name;
	  host->port = port;
	  host->secure_p = secure_p;
	  length = XP_STRLEN (host_name) + 20;
	  host->newsrc_file_name = (char *) XP_ALLOC (length);
	  if (!host->newsrc_file_name)
		{
		  XP_FREE (host);
		  return MK_OUT_OF_MEMORY;
		}

	  /* Generate a newsrc file name for this host.

		 Sometimes the default news host can have a newsrc file name of ""
		 which corresponds to "~/.newsrc".  All other hosts have newsrc
		 files with host names and possibly ports in them.  The default
		 newsrc can be of the latter form too.

		 So, when we're in a situation where there is no newsrc file for a
		 host, we always create them in the latter form (even if it turns
		 out to be the default host.)  This means that, in the absence of
		 any newsrc files, ".newsrc" will never be created, but ".newsrc-news"
		 will be.  The user can rename it later if they care.

		 If the port being used is the default one for the protocol, we don't
		 put that on the file name.  A pre-existing file could have a name of
		 ".newsrc-news:119", and we would use it, but we would never generate
		 such a file unless it existed already.
	   */
	  *host->newsrc_file_name = 0;
	  XP_STRCPY (host->newsrc_file_name, host_name);
	  XP_ASSERT (host->port != 0);
	  if (host->port != (uint32) (host->secure_p ? SECURE_NEWS_PORT : NEWS_PORT))
		PR_snprintf (host->newsrc_file_name
					 + XP_STRLEN (host->newsrc_file_name),
					 length - XP_STRLEN (host->newsrc_file_name),
					 ":%lu", (unsigned long) host->port);
	  host->next = context->msgframe->data.news.hosts;
	  context->msgframe->data.news.hosts = host;
	  created_host_p = TRUE;
	}

  hfolder = msg_NewsHostFolder (context, host);
  XP_ASSERT (created_host_p || hfolder);

  if (!hfolder)
	{
	  /* No such folder?  Make one!
	   */
	  struct MSG_Folder *rest;
	  hfolder = msg_make_news_folder_for_host (context, host);
	  for (rest = context->msgframe->folders; rest && rest->next;
		   rest = rest->next)
		;
	  if (rest)
		rest->next = hfolder;
	  else
		context->msgframe->folders = hfolder;

	  created_host_p = TRUE;
	}

  /* If the host on which we are loading this newsgroup is not the host
	 that is currently selected, close that one and open this one. */
  if (select_host_p &&
	  context->msgframe->data.news.current_host != host)
	msg_ToggleFolderExpansion (context, hfolder, FALSE);


  if (!group_name)
	goto DONE;

  /* Now find the folder of this group.
   */
  gfolder = msg_find_news_folder (group_name, context->msgframe->folders);

  if (!gfolder)
	{
	  /* No such folder?  Make one!
		 (Must be an unsubscribed group.)
	   */
	  struct MSG_Folder *rest;

	  gfolder = msg_make_newsgroup_folder (0, group_name, 0);
	  if (!gfolder) return MK_OUT_OF_MEMORY;

	  /* #### if this unsubscribed group is listed in the .newsrc file, it
		 would be nice to insert it in the list in the same relative
		 position, instead of at the end. */
	  for (rest = hfolder->children; rest && rest->next; rest = rest->next)
		;
	  if (rest)
		rest->next = gfolder;
	  else
		hfolder->children = gfolder;

	  if (!created_host_p)
		/* #### redisplay more efficiently */
		msg_RedisplayFolders (context);
	}

  XP_ASSERT (gfolder);
  if (!gfolder) return -1;

 DONE:

  if (created_host_p && sort_p)
	msg_sort_news_hosts (context);

  if (hfolderP) *hfolderP = hfolder;
  if (gfolderP) *gfolderP = gfolder;

  return 0;
}

int
msg_EnsureNewsgroupSelected (MWContext *context,
							 const char *host_and_port,
							 const char *group_name,
							 XP_Bool secure_p)
{
  struct MSG_Folder *gfolder = 0;
  struct MSG_Folder *hfolder = 0;
  int status;

  status = msg_ensure_newsgroup_listed (context, host_and_port,
										group_name, secure_p, TRUE, TRUE,
										&hfolder, &gfolder);
  if (status < 0) return status;

  XP_ASSERT (hfolder);
  if (!hfolder) return -1;
  context->msgframe->data.news.current_host =
	msg_NewsFolderHost (context, hfolder);

  if (group_name)
	{
	  XP_ASSERT (gfolder);
	  if (!gfolder) return -1;
	  if (context->msgframe->opened_folder != gfolder) {
		msg_ClearThreadList(context);
		context->msgframe->opened_folder = gfolder;
	  }
	  if (!(gfolder->flags & MSG_FOLDER_FLAG_SELECTED) ||
		  context->msgframe->num_selected_folders != 1)
		{
		  msg_UnselectAllFolders(context);
		  gfolder->flags |= MSG_FOLDER_FLAG_SELECTED;
		  context->msgframe->num_selected_folders = 1;
		  msg_RedisplayOneFolder(context, gfolder, -1, -1, TRUE);
		}
	}
  return 0;
}


int
msg_UpdateNewsFolderMessageCount (MWContext *context,
								  struct MSG_Folder *folder,
								  uint32 unread_messages,
								  XP_Bool remove_if_empty)
{
  if (!folder) return -1;

  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews)
	return -1;
  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								MSG_FOLDER_FLAG_DIRECTORY |
								MSG_FOLDER_FLAG_MAIL)));

  folder->unread_messages = unread_messages;

  if (remove_if_empty &&
	  unread_messages == 0 &&
	  (context->msgframe->data.news.group_display_type ==
	   MSG_ShowSubscribedWithArticles) &&
	  /* Don't remove it if it's opened */
	  folder != context->msgframe->opened_folder &&
	  /* Don't remove it if it was just added by `Check New Groups' */
	  !(folder->flags & MSG_FOLDER_FLAG_NEW_GROUP))
	{
	  /* We now know that this group has no articles, and is uninteresting.
		 We can remove it from the list. */
	  XP_ASSERT (!folder->children &&
				 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
									MSG_FOLDER_FLAG_MAIL |
									MSG_FOLDER_FLAG_ELIDED |
									MSG_FOLDER_FLAG_DIRECTORY)));
	  msg_RemoveFolderLine (context, folder);
	  XP_ASSERT (!folder->next);
	  FREEIF (folder->name);
	  FREEIF (folder->data.newsgroup.group_name);
	  XP_FREE (folder);
	  return 0;
	}

  /* #### moderated_p */

  msg_RedisplayOneFolder (context, folder, -1, -1, FALSE);
  return 0;
}


int
MSG_DisplaySubscribedGroup (MWContext *context, const char *group,
							uint32 oldest_message,
							uint32 youngest_message,
							uint32 total_messages,
							XP_Bool nowvisiting)
{
  struct MSG_Folder *folder = msg_FindNewsgroupFolder (context, group);
  struct msg_NewsRCSet *newsrc;
  uint32 unread_messages;
  if (! folder)
	return 0;

  FE_EnableClicking (context);

  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								MSG_FOLDER_FLAG_MAIL |
								MSG_FOLDER_FLAG_DIRECTORY |
								MSG_FOLDER_FLAG_ELIDED)));

  folder->data.newsgroup.last_message = youngest_message;

  newsrc = msg_GetNewsRCSet (context, folder->data.newsgroup.group_name,
							 context->msgframe->data.news.newsrc_file);
  if (newsrc)
	{
	  /* First, mark all of the articles now known to be expired as read. */
	  int status = 0;
	  if (oldest_message > 1)
		status = msg_MarkRangeAsRead (newsrc, 0, oldest_message - 1);
	  if (status < 0)
		return status;
	  /* This isn't really an important enough change to warrant causing
		 the newsrc file to be saved; we haven't gathered any information
		 that won't also be gathered for free next time.
	  else if (status > 0)
		msg_NoteNewsrcChanged(context);
       */

	  /* Now search the newsrc line and figure out how many of these
		 messages are marked as unread. */
	  unread_messages = msg_NewsRCUnreadInRange (newsrc, oldest_message,
												 youngest_message);
	  /*XP_ASSERT (unread_messages >= 0);		it's unsigned */
	  if (unread_messages > total_messages)
		/* This can happen when the newsrc file shows more unread than exist
		   in the group (total_messages is not necessarily `end - start'.) */
		unread_messages = total_messages;
	}
  else
	{
	  unread_messages = total_messages;
	}

  folder->total_messages = total_messages;
  return msg_UpdateNewsFolderMessageCount(context, folder, unread_messages,
										  !nowvisiting &&
										  (context->msgframe->
										   data.news.group_display_type
										   == MSG_ShowSubscribedWithArticles));
}


int
MSG_DisplayNewNewsGroup (MWContext *context, const char *host_and_port,
						 XP_Bool secure_p, const char *group_name,
						 int32 oldest_message, int32 youngest_message)
{
  struct MSG_Folder *hfolder = 0;
  struct MSG_Folder *gfolder = 0;
  int status;
  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;

  status = msg_ensure_newsgroup_listed (context, host_and_port,
										group_name, secure_p, TRUE, TRUE,
										&hfolder, &gfolder);
  if (status < 0) return status;
  XP_ASSERT (hfolder && gfolder);
  if (!hfolder || !gfolder) return -1;
  XP_ASSERT ((gfolder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			 !(gfolder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								 MSG_FOLDER_FLAG_MAIL |
								 MSG_FOLDER_FLAG_DIRECTORY |
								 MSG_FOLDER_FLAG_ELIDED)));
  gfolder->flags |= MSG_FOLDER_FLAG_NEW_GROUP;
  gfolder->data.newsgroup.last_message = youngest_message;
  gfolder->total_messages = youngest_message - oldest_message;
  if (gfolder->unread_messages < 0)
	gfolder->unread_messages = gfolder->total_messages;
  return msg_UpdateNewsFolderMessageCount(context, gfolder,
										  gfolder->unread_messages,
										  FALSE);
}


/* Called as the pre-exit routine of the URL we loaded to download the
   new newsgroups.  Now that we have them, we can tell the user what
   we did.
 */
static void
msg_new_groups_listed (URL_Struct *url, int status, MWContext *context)
{
  struct MSG_Folder *hfolder;
  struct MSG_Folder *rest;
  int32 count = 0;
  char *fmt;
  char *buf;
  if (status < 0) return;
  hfolder = msg_NewsHostFolder (context,
								context->msgframe->data.news.current_host);
  for (rest = hfolder->children; rest; rest = rest->next)
	{
	  XP_ASSERT ((rest->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
				 !(rest->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								  MSG_FOLDER_FLAG_MAIL |
								  MSG_FOLDER_FLAG_DIRECTORY |
								  MSG_FOLDER_FLAG_ELIDED)));
	  if (rest->flags & MSG_FOLDER_FLAG_NEW_GROUP)
		{
		  /* The NEW_GROUP flag was added when this group was inserted
			 into the list; now that we're done, count up the groups
			 that have that flag, and remove the flag so that the
			 count is right the next time around (mark/sweep :-)) */
		  rest->flags &= ~MSG_FOLDER_FLAG_NEW_GROUP;
		  count++;
		}
	}

  if (count <= 0)
	fmt = XP_GetString (MK_MSG_NO_NEW_GROUPS);
  else if (count == 1)
	fmt = XP_GetString (MK_MSG_ONE_NEW_GROUP);
  else
	fmt = XP_GetString (MK_MSG_N_NEW_GROUPS);

  XP_ASSERT (fmt);
  if (fmt)
	{
	  buf = PR_smprintf (fmt, count);
	  if (!buf) return;
	  FE_Alert (context, buf);
	  XP_FREE (buf);
	}
}


/* When the user changes their default news host in preferences,
   the FE should call this on any existing news window to ensure
   that that host is in the list.
 */
int
MSG_NewsHostChanged (MWContext *context, const char *host_and_port)
{
  int status;
  XP_ASSERT (context && context->msgframe && context->type == MWContextNews);
  if (!context || !context->msgframe ||  context->type != MWContextNews) {
	return -1;
  }
  msg_InterruptContext(context, FALSE);
  status = msg_ensure_newsgroup_listed (context, host_and_port, 0,
										FALSE, FALSE, FALSE, 0, 0);
  if (status < 0) return status;
  status = msg_ShowNewsHosts (context);
  return status;
}


static void
msg_open_news_host_cb (MWContext *context, char *url, void *closure)
{
  URL_Struct* url_struct;
  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews) return;
  if (!url) return;
  url_struct = NET_CreateURLStruct(url, NET_NORMAL_RELOAD);
  XP_FREE (url);
  msg_GetURL (context, url_struct, FALSE);
}

int
msg_OpenNewsHost (MWContext *context)
{
  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews) return -1;
  return FE_PromptForNewsHost (context, XP_GetString(MK_MSG_OPEN_NEWS_HOST),
							   msg_open_news_host_cb, 0);
}


int
msg_AddNewsGroup(MWContext* context)
{
  char* groupname;
  char* url;
  URL_Struct *url_struct;
  groupname = FE_Prompt(context, XP_GetString(XP_NEWS_PROMPT_ADD_NEWSGROUP),
						"");
  if (!groupname) return 0;		/* User cancelled. */

  url = msg_MakeNewsURL(context, context->msgframe->data.news.current_host,
						groupname);
  if (!url) return MK_OUT_OF_MEMORY;
  url_struct = NET_CreateURLStruct (url, NET_NORMAL_RELOAD);
  XP_FREE(url);
  if (!url_struct) return MK_OUT_OF_MEMORY;
  msg_GetURL(context, url_struct, FALSE);
  return 0;
}


static void
msg_cancel_done (URL_Struct *url, int status, MWContext *context)
{
  if (status >= 0)
	FE_Alert (context, XP_GetString (MK_MSG_MESSAGE_CANCELLED));
  /* else, the FE exit routine will present the error message. */
}

int
msg_CancelMessage (MWContext *context)
{
  /* Get the message ID of the current message, and open a URL of the form
	 "news://host/message@id?cancel" and mknews.c will do all the work. */
  struct MSG_ThreadEntry *msg;
  struct MSG_NewsHost *host;
  URL_Struct *url_struct;
  char *id;
  char *url;
  char *suffix;

  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;

  msg = context->msgframe->displayed_message;
  if (!msg) return -1; /* ??? */

  host = context->msgframe->data.news.current_host;
  XP_ASSERT (host);
  if (!host) return -1;


  id = context->msgframe->msgs->string_table [msg->id];
  if (!id) return -1;  /* ??? */

  id = NET_Escape (id, URL_XALPHAS);
  if (!id) return MK_OUT_OF_MEMORY;

  suffix = (char *) XP_ALLOC (XP_STRLEN (id) + 30);
  if (!suffix) return MK_OUT_OF_MEMORY;
  XP_STRCPY (suffix, id);
  XP_STRCAT (suffix, "?cancel");
  XP_FREE (id);

  url = msg_MakeNewsURL (context, host, suffix);
  XP_FREE (suffix);
  if (!url) return MK_OUT_OF_MEMORY;

  /* use NET_NORMAL_RELOAD so we don't fail to
     cancel messages that are in the cache */
  url_struct = NET_CreateURLStruct (url, NET_NORMAL_RELOAD);
  XP_FREE (url);
  if (!url_struct) return MK_OUT_OF_MEMORY;
  url_struct->internal_url = TRUE;
  url_struct->pre_exit_fn = msg_cancel_done;
  return msg_GetURL (context, url_struct, FALSE);
}


/* Updating the newsrc
 */

int 
msg_MarkNewsArticleRead (MWContext *context,
						 const char *message_id,
						 const char *xref)
{
  struct MSG_ThreadEntry *msg = 0;
  struct msg_NewsRCSet *newsrc = 0;
  struct MSG_Folder *folder = 0;
  XP_Bool marked_read = FALSE;
  int status = 0;
  if (!message_id ||
	  !context->msgframe ||
	  context->type != MWContextNews ||
	  !context->msgframe->msgs ||
	  !context->msgframe->msgs->message_id_table)
	return 0;

  if (!context->msgframe->data.news.newsrc_changed_timer)
	{
	  /* Only stat the file at first change, not every change. */
	  msg_check_newsrc_file_date (context);
	  if (!context->msgframe->data.news.newsrc_file)
		/* #### should probably reload data now? */
		return 0;
	}

  msg_undo_StartBatch(context);

  folder = context->msgframe->opened_folder;

  /* Even if the file doesn't exist on disk, we should have memory data
	 for it.  When connecting to a new news host, we don't actually write
	 a newsrc file on disk until we have article data to put in it. */
  XP_ASSERT (context->msgframe->data.news.newsrc_file);

  if (folder && context->msgframe->data.news.newsrc_file)
	{
	  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
				 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
									MSG_FOLDER_FLAG_MAIL |
									MSG_FOLDER_FLAG_DIRECTORY |
									MSG_FOLDER_FLAG_ELIDED)));
	  newsrc = msg_GetNewsRCSet (context, folder->data.newsgroup.group_name,
								 context->msgframe->data.news.newsrc_file);
	}

  if (folder && !newsrc && context->msgframe->data.news.newsrc_file)
	{
	  /* There wasn't a newsrc entry for this group.  This probably means
		 that this group is both unsubscribed, and not listed in the newsrc
		 file.  So let's add one.  This will cause (at the next save) there
		 to be one more line in the newsrc file, for this group in the
		 unsubscribed state.
	   */
	  newsrc = msg_AddNewsRCSet (folder->data.newsgroup.group_name,
								 context->msgframe->data.news.newsrc_file);
	}

  if (newsrc)
	{
	  /* Find the ThreadEntry with this ID. */
	  char *id = XP_STRDUP (message_id);
	  char *id2;
	  if (! id) {
		status = MK_OUT_OF_MEMORY;
		goto FAIL;
	  }
	  id2 = id;
	  if (*id2 == '<') id2++;
	  if (id2[XP_STRLEN(id2)-1] == '>')
		id2[XP_STRLEN(id2)-1] = 0;

	  msg = (struct MSG_ThreadEntry *)
		XP_Gethash (context->msgframe->msgs->message_id_table, id2, 0);

	  XP_FREE (id);

	  if (msg)
		{
		  status = msg_MarkArtAsRead (newsrc, msg->data.news.article_number);
		  if (status < 0)
			goto FAIL;
		  else if (status > 0)
			{
			  marked_read = TRUE;
			  msg_NoteNewsrcChanged(context);
			  /* Update the summary line. */
			  msg_ChangeFlag(context, msg, MSG_FLAG_READ, 0);
			}
		}
	}

  if (msg)
	{
	  context->msgframe->displayed_message = msg;
	  msg_EnsureSelected(context, msg); /* Select if not already. */
	  msg_RedisplayOneMessage(context, msg, -1, 0, TRUE); /* Scroll to it. */
	}
  else
	{
	  /* This can happen if we were reading a group normally, and then clicked
		 on a "references" link, or a "news:" link, to some random message for
		 which we do not have a ThreadEntry. */
	  msg_UnselectAllMessages (context);
	}

  /* Now let's go through that xref field, and mark the crossposts
	 as read as well. */
  if (xref)
	{
	  const char *rest = xref;
	  const char *group_start = 0, *group_end = 0;
	  while (XP_IS_SPACE (*rest))
		rest++;
	  group_start = rest;
	  for (;;) {
		char* name;
		uint32 n;
		switch (*rest) {
		case ':':
		  group_end = rest;
		  rest++;
		  break;

		case ',':
		case ' ':
		case 0:
		  n = 0;
#ifdef BUG_22516
		  if (group_end) n = XP_ATOI(group_end + 1);
#else
		  if (group_end) sscanf ( group_end+1, "%lu", &n);
#endif /* BUG_22516 */
		  if (n > 0) {
			MSG_Folder* folder;
			name = (char *) XP_ALLOC (group_end - group_start + 1);
			if (! name) {
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
			XP_MEMCPY (name, group_start, group_end - group_start);
			name[group_end - group_start] = 0;

			folder = msg_FindNewsgroupFolder(context, name);
			if (folder) {
			  msg_ChangeFlagAny(context, folder, n, MSG_FLAG_READ, 0);
			} else {
			  newsrc =
				msg_GetNewsRCSet(context, name,
								 context->msgframe->data.news.newsrc_file);
			  if (newsrc) {
				status = msg_MarkArtAsRead (newsrc, n);
				if (status < 0) {
				  goto FAIL;
				} else if (status > 0) {
				  msg_NoteNewsrcChanged(context);
				}
			  }
			}
			XP_FREE (name);
		  }

		  if (*rest == 0) goto DONE;
		  rest++;
		  group_start = rest;
		  group_end = 0;
		  break;
		default:
		  rest++;
		  break;
		}
	  }
	}

DONE:

  status = 0;

FAIL:
  msg_undo_EndBatch(context);
  return status;
}



static void
msg_subscribe_group_1(MWContext* context, MSG_Folder* folder,
					  int on_or_off_or_toggle)
{
  msg_NewsRCSet* set;
  XP_ASSERT (context->type == MWContextNews);
  if (context->type != MWContextNews) return;
  XP_ASSERT (folder &&
			 (folder->flags & MSG_FOLDER_FLAG_NEWSGROUP) &&
			 !(folder->flags & (MSG_FOLDER_FLAG_NEWS_HOST |
								MSG_FOLDER_FLAG_MAIL |
								MSG_FOLDER_FLAG_DIRECTORY |
								MSG_FOLDER_FLAG_ELIDED)));
  if (!folder) return;
  if (! (folder->flags & MSG_FOLDER_FLAG_NEWSGROUP)) return;
  set = msg_GetNewsRCSet(context, folder->data.newsgroup.group_name,
						 context->msgframe->data.news.newsrc_file);
  if (!set) {
	set = msg_AddNewsRCSet(folder->data.newsgroup.group_name,
						   context->msgframe->data.news.newsrc_file);
  }
  if (set) {
	if (on_or_off_or_toggle == 0)
	  msg_SetSubscribed(set, !msg_GetSubscribed(set));
	else if (on_or_off_or_toggle == -1)
	  msg_SetSubscribed(set, FALSE);
	else if (on_or_off_or_toggle == 1)
	  msg_SetSubscribed(set, TRUE);
	else
	  XP_ASSERT(0);

	msg_NoteNewsrcChanged(context);
	if (msg_GetSubscribed(set)) {
	  folder->flags |= MSG_FOLDER_FLAG_SUBSCRIBED;
	} else {
	  folder->flags &= ~MSG_FOLDER_FLAG_SUBSCRIBED;
	}
	msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);
  }
}

void
msg_ToggleSubscribedGroup(MWContext* context, MSG_Folder* folder)
{
  msg_subscribe_group_1(context, folder, 0);
}



static int
msg_subscribe_selected_groups_1(MWContext* context, MSG_Folder* folder,
								XP_Bool subscribe_p)
{
  int status;
  for ( ; folder ; folder = folder->next) {
	if ((folder->flags & MSG_FOLDER_FLAG_SELECTED) && 
		(folder->flags & MSG_FOLDER_FLAG_NEWSGROUP)) {
	  msg_subscribe_group_1(context, folder, (subscribe_p ? 1 : -1));
	}
	status = msg_subscribe_selected_groups_1(context, folder->children,
											 subscribe_p);
	if (status < 0) return status;
  }
  return 0;
}


int
msg_SubscribeSelectedGroups (MWContext *context, XP_Bool subscribe_p)
{
  XP_ASSERT (context->type == MWContextNews && context->msgframe);
  if (context->type != MWContextNews || !context->msgframe) return -1;
  return msg_subscribe_selected_groups_1(context, context->msgframe->folders,
										 subscribe_p);
}



/* Saving news messages
 */

struct msg_news_save_state
{
  char *file_name;
  char **urls;
  char **urls_remaining;
  XP_File out_fd;
  XP_Bool existed_p;
  XP_Bool wrote_any_p;
  XP_Bool summary_valid_p;
  int numwrote;
  Net_GetUrlExitFunc *final_exit;

#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
  char *obuffer;
  uint32 obuffer_size;
  uint32 obuffer_fp;
#endif /* MANGLE_INTERNAL_ENVELOPE_LINES */
};

static void msg_save_next_news_exit (URL_Struct *, int, MWContext *);

static int
msg_save_next_news_pop (MWContext *context, struct msg_news_save_state *state,
						XP_Bool first_time_p)
{
  URL_Struct *url_struct;
  char *url = *state->urls_remaining++;
  state->numwrote++;
  url_struct = NET_CreateURLStruct (url, NET_DONT_RELOAD);
  if (!url_struct) return MK_OUT_OF_MEMORY;
  if (first_time_p)
	{
	  XP_ASSERT (!state->final_exit);
	  FE_SetWindowLoading (context, url_struct, &state->final_exit);
	  XP_ASSERT (state->final_exit);
	}
  url_struct->fe_data = (void *) state;
  state->wrote_any_p = FALSE;
  NET_GetURL (url_struct, FO_CACHE_AND_APPEND_TO_FOLDER,
			  context, msg_save_next_news_exit);
  return 0;
}


static unsigned int
msg_append_to_folder_stream_write_ready (void *closure)
{
  return MAX_WRITE_READY;
}

/* #### move this to a header file -- defined in compose.c */
extern int32 msg_do_fcc_handle_line(char* line, uint32 length, void* closure);


static int
msg_append_to_folder_stream_write (void *closure,
								   const char *block, int32 length)
{
  struct msg_news_save_state *state = (struct msg_news_save_state *) closure;

  /* We implicitly assume that the data we're getting is already following
	 the local line termination conventions; the NNTP module has already
	 converted from CRLF to the local linebreak for us. */

  if (!state->wrote_any_p)
	{
	  const char *envelope = msg_GetDummyEnvelope();
	  int32 L;

#ifdef FIXED_SEPERATORS
	  if (state->existed_p)
		/* If the file existed, preceed it by a linebreak: the BSD mailbox file
		   format wants a *blank* line before all "From " lines except the
		   first.  This assumes that the previous message in the file was
		   properly closed, that is, that the last line in the file ended with
		   a linebreak. */
		L = LINEBREAK_LEN;
		if (L != XP_FileWrite (LINEBREAK, L, state->out_fd))
		  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
		state->wrote_any_p = TRUE;
#endif /* FIXED_SEPERATORS */

	  state->existed_p = TRUE;

	  L = XP_STRLEN (envelope);
	  if (L != XP_FileWrite (envelope, L, state->out_fd))
		  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;

	  state->wrote_any_p = TRUE;

	  /* Now write out an X-Mozilla-Status header to the folder. */
	  {
		uint16 flags = 0;
		char buf[X_MOZILLA_STATUS_LEN + 50];

#ifdef MARK_SAVE_NEWS_MESSAGES_AS_READ
		/* #### should only do this if the message is currently marked
		   as read (it usually is, but not always.) */
		flags |= MSG_FLAG_READ;
#endif /* MARK_SAVE_NEWS_MESSAGES_AS_READ */

		/* #### how to tell if the news message was MSG_FLAG_MARKED? */

		/* #### for consistency, it would be better if we put the
		   X-Mozilla-Status header at the *end* of the header block
		   rather than the beginning. */
		XP_SPRINTF(buf, X_MOZILLA_STATUS ": %04x" LINEBREAK, flags);

		L = XP_STRLEN (buf);
		if (L != XP_FileWrite (buf, L, state->out_fd))
		  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	  }
	}

#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
  /* Since we're writing a mail folder, we must follow the Usual Mangling
	 Conventions.
   */
  {

	int n = msg_LineBuffer (block, length,
							&state->obuffer,
							&state->obuffer_size,
							&state->obuffer_fp,
							TRUE, msg_do_fcc_handle_line,
							state->out_fd);
	if (n < 0) /* write failed */
	  return MK_MIME_ERROR_WRITING_FILE;
  }
#else  /* !MANGLE_INTERNAL_ENVELOPE_LINES */

  if (XP_FileWrite (block, length, state->out_fd) < length)
	return MK_MIME_ERROR_WRITING_FILE;

#endif /* !MANGLE_INTERNAL_ENVELOPE_LINES */

  return 1;
}

static void
msg_append_to_folder_stream_complete (void *closure)
{
  /* Nothing to do here - the URL exit method does our cleanup. */
}

static void
msg_append_to_folder_stream_abort (void *closure, int status)
{
  /* Nothing to do here - the URL exit method does our cleanup. */
}


NET_StreamClass *
msg_MakeAppendToFolderStream (int format_out, void *closure,
							  URL_Struct *url, MWContext *context)
{
  NET_StreamClass *stream;
    
  TRACEMSG(("Setting up attachment stream. Have URL: %s\n", url->address));

  stream = XP_NEW (NET_StreamClass);
  if (!stream) return 0;
  XP_MEMSET (stream, 0, sizeof (NET_StreamClass));

  stream->name           = "Folder Append Stream";
  stream->complete       = msg_append_to_folder_stream_complete;
  stream->abort          = msg_append_to_folder_stream_abort;
  stream->put_block      = msg_append_to_folder_stream_write;
  stream->is_write_ready = msg_append_to_folder_stream_write_ready;
  stream->data_object    = url->fe_data;
  stream->window_id      = context;

  TRACEMSG(("Returning stream from msg_MakeAppendToFolderStream"));

  return stream;
}


static void
msg_save_next_news_exit (URL_Struct *url, int status, MWContext *context)
{
  struct msg_news_save_state *state =
	(struct msg_news_save_state *) url->fe_data;
  XP_ASSERT (state);
  if (!state) return;

  if (status < 0 || !*state->urls_remaining)
	{
	FAIL:

#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
	  if (status >= 0 && state->obuffer_fp > 0)
		{
		  /* Flush out any partial line. */
		  msg_do_fcc_handle_line(state->obuffer, state->obuffer_fp, state);
		}

	  FREEIF(state->obuffer);
#endif /* !MANGLE_INTERNAL_ENVELOPE_LINES */

	  if (state->urls)
		{
		  char **rest = state->urls;
		  while (*rest)
			XP_FREE (*rest++);
		  XP_FREE (state->urls);
		}
	  if (XP_FileClose(state->out_fd) != 0) {
		if (status >= 0) status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	  }
	  if (state->final_exit)
		state->final_exit (url, status, context);
	  if (state->summary_valid_p)
		msg_SetSummaryValid (state->file_name, state->numwrote,
#ifdef MARK_SAVE_NEWS_MESSAGES_AS_READ
							 0
#else  /* !MARK_SAVE_NEWS_MESSAGES_AS_READ */
							 state->numwrote
#endif /* !MARK_SAVE_NEWS_MESSAGES_AS_READ */
);

	  FREEIF (state->file_name);
	  XP_FREE (state);
	}
  else
	{
	  status = msg_save_next_news_pop (context, state, FALSE);
	  if (status < 0) goto FAIL;
	}
}

int
msg_SaveSelectedNewsMessages (MWContext *context, const char *file_name)
{
  struct msg_news_save_state *state;
  XP_StatStruct st;
  state = XP_NEW (struct msg_news_save_state);
  if (!state) return MK_OUT_OF_MEMORY;

  XP_MEMSET (state, 0, sizeof(*state));
  if (!XP_Stat (file_name, &st, xpMailFolder))
	state->existed_p = (st.st_size > 0);

  if (state->existed_p && !msg_ConfirmMailFile (context, file_name))
	return -1; /* #### NOT_A_MAIL_FILE */

  state->out_fd = XP_FileOpen (file_name, xpMailFolder, XP_FILE_APPEND_BIN);
  if (!state->out_fd)
	{
	  XP_FREE (state);
	  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	}

  state->urls = msg_GetSelectedMessageURLs (context);
  if (!state->urls)
	{
	  XP_FileClose (state->out_fd);
	  XP_FREE (state);
	  return 0;
	}
  state->urls_remaining = state->urls;

  if (state->existed_p)
	state->summary_valid_p = msg_IsSummaryValid (file_name, &st);

  state->file_name = XP_STRDUP (file_name);

  return msg_save_next_news_pop (context, state, TRUE);
}



int 
MSG_AddToKnownArticles(MWContext* context, const char* host_and_port,
					   XP_Bool secure_p, const char* group_name,
					   int32 first, int32 last)
{
  struct MSG_NewsKnown* k;
  XP_ASSERT(context->type == MWContextNews && context->msgframe);
  if (context->type != MWContextNews || context->msgframe == NULL) return -1;

  k = &(context->msgframe->data.news.knownarts);
  if (k->host_and_port == NULL ||
	  XP_STRCMP(k->host_and_port, host_and_port) != 0 ||
	  k->secure_p != secure_p ||
	  k->group_name == NULL ||
	  XP_STRCMP(k->group_name, group_name) != 0 ||
	  !k->set) {
	msg_ClearThreadList(context);
	FREEIF(k->host_and_port);
	k->host_and_port = XP_STRDUP(host_and_port);
	k->secure_p = secure_p;
	FREEIF(k->group_name);
	k->group_name = XP_STRDUP(group_name);
	msg_FreeNewsrcSet(k->set);
	k->set = msg_MakeNewsrcSet();

	if (!k->host_and_port || !k->group_name || !k->set) {
	  return MK_OUT_OF_MEMORY;
	}

  }

  return msg_MarkRangeAsRead(k->set, first, last);
}



/* Figure out what articles to get from the news server.  Takes into account
   what articles we already know about (if we're already displaying this
   group), and what articles have already been read (if we're only looking for
   unread articles). */

int
MSG_GetRangeOfArtsToDownload(MWContext* context, const char* host_and_port,
							 XP_Bool secure_p, const char* group_name,
							 int32 first_possible,
							 int32 last_possible,
							 int32 maxextra,
							 int32* first,
							 int32* last)
{
  struct MSG_NewsKnown* k;
  msg_NewsRCSet *set;
  int status = 0;
  XP_Bool emptyGroup_p = FALSE;

  XP_ASSERT(first && last);
  if (!first || !last) return -1;

  *first = 0;
  *last = 0;

  XP_ASSERT(context && context->msgframe && context->type == MWContextNews);
  if (!context || !context->msgframe || context->type != MWContextNews) {
	return -1;
  }

  if (maxextra <= 0 || last_possible < first_possible || last_possible < 1) {
	  emptyGroup_p = TRUE;
  }

  k = &(context->msgframe->data.news.knownarts);
  if (k->host_and_port == NULL ||
	  XP_STRCMP(k->host_and_port, host_and_port) != 0 ||
	  k->secure_p != secure_p ||
	  k->group_name == NULL ||
	  XP_STRCMP(k->group_name, group_name) != 0 ||
	  !k->set) {
	/* We're displaying some other group.  Clear out that display, and set up
	   everything to return the proper first chunk. */
	status = msg_EnsureNewsgroupSelected(context, host_and_port, group_name,
										 secure_p);
	if (status < 0) return status;
	if (emptyGroup_p)
	  return 0;
	status = MSG_AddToKnownArticles(context, host_and_port, secure_p,
									group_name, 0, 0);
	if (status < 0) return status;

	if (!context->msgframe->all_messages_visible_p) {
	  set = msg_GetNewsRCSet(context, group_name,
							 context->msgframe->data.news.newsrc_file);
	  if (set) status = msg_CopySet(k->set, set);
	  if (status < 0) return status;
	  if (first_possible > 1) {
		msg_MarkRangeAsRead(k->set, 1, first_possible - 1);
			/* Help makes later calls to msg_FirstUnreadArt be more useful. */
	  }
	}
  }
  else
  {
	if (emptyGroup_p)
	  return 0;
  }

  k->first_possible = first_possible;
  k->last_possible = last_possible;

  /* Determine if we want to get articles towards the end of the range,
	 or towards the beginning.  If there are new articles at the end
	 we haven't read, we always want to get those first.  Otherwise, it
	 depends on the shouldGetOldest flag. */
  
  if (k->shouldGetOldest && msg_NewsRCIsRead(k->set, last_possible)) {
	status = msg_FirstUnreadRange(k->set, first_possible, last_possible,
								  first, last);
	if (status < 0) return status;
	if (*first > 0 && *last - *first >= maxextra) {
	  *last = *first + maxextra - 1;
	}
  } else {
	status = msg_LastUnreadRange(k->set, first_possible, last_possible,
								 first, last);
	if (status < 0) return status;
	if (*first > 0 && *last - *first >= maxextra) {
	  *first = *last - maxextra + 1;
	}
  }

  return 0;
}
