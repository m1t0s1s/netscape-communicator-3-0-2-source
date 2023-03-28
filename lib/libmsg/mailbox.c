/* -*- Mode: C; tab-width: 4 -*-
   mailbox.c --- backend for the "mailbox:" URL type
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 29-Jun-95.
 */

#include "msg.h"
#include "mailsum.h"
#include "libmime.h"

/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_MSG_FOLDER_UNREADABLE;
extern int MK_MSG_ID_NOT_IN_FOLDER;
extern int MK_MSG_NON_MAIL_FILE_READ_QUESTION;
extern int MK_OUT_OF_MEMORY;
extern int MK_MSG_PARTIAL_MSG_FMT_1;
extern int MK_MSG_PARTIAL_MSG_FMT_2;
extern int MK_MSG_PARTIAL_MSG_FMT_3;


/* The "mailbox:" protocol module calls us in this way:

   MSG_BeginOpenFolderSock()
    makes a state object and (possibly) sets state->msgs.
    error if asking for a folder in a non-mail window.
	Also use this as an opportunity to make sure the current folder
	is being displayed as selected and is visible.

   MSG_FinishOpenFolderSock()
    parses the folder if necessary, creating a new summary file.
    may set state->msgs if that hasn't been done already.
    if we're opening a folder, not a message, installs the
    parsed data in the context and redisplays the list.

     MSG_OpenMessageSock
      opens the folder file if MSG_BeginOpenFolderSock didn't open it.
      does a seek().

     MSG_ReadMessageSock
      reads from the folder until enough has been read.
      (netlib feeds this data to layout.)

     MSG_CloseMessageSock
      nothing real to do, but use this as an opportunity to mark this
	  message as read, and make sure it's selected and visible in the
	  scrolling list.

   MSG_CloseFolderSock
     closes file, frees state.
 */

/*#define MBOX_DEBUG*/

#ifdef MBOX_DEBUG
extern FILE* real_stderr;
#endif

struct msg_mailbox_state
{
  XP_Bool mail_context_p;
  struct MSG_Folder *folder;
  struct mail_FolderData *msgs;
  struct MSG_ThreadEntry *msg;

  time_t folder_date;
  uint32 folder_size;

  XP_File file;
  struct mail_FolderData *flags_msgs; /* Kept around to let us copy flags from
										 out-of-date summary file into newly
										 created summary file. */

  char *obuffer;
  uint32 obuffer_size;

  char *ibuffer;
  uint32 ibuffer_size;
  uint32 ibuffer_fp;

  struct msg_FolderParseState *pstate;

  uint32 msg_bytes_remaining;

  XP_Bool discarded_envelope_p;
  XP_Bool wrote_fake_id_p;

  int32 graph_progress_total;
  int32 graph_progress_received;
};

static int
msg_cmp (const void *obj1, const void *obj2)
{
  XP_ASSERT (obj1 && obj2);
  return XP_STRCMP ((char *) obj1, (char *) obj2);
}

static void
msg_add_flags_to_hash_table(char** string_table, MSG_ThreadEntry* msg,
							XP_HashTable hash)
{
  for (; msg ; msg = msg->next) {
	XP_Puthash(hash, string_table[msg->id], (void*) ((uint32) msg->flags));
	msg_add_flags_to_hash_table(string_table, msg->first_child, hash);
  }
}  


static void
msg_apply_flags_from_hashtable(MWContext* context, mail_FolderData* msgs,
							   char** string_table, MSG_ThreadEntry* msg,
							   XP_HashTable hash)
{
  for (; msg ; msg = msg->next) {
	if (msg->flags == 0) {
	  /* Only get flags from the hash file if we didn't find a status */
	  /* line in the message itself. */
	  msg->flags = (uint16) (uint32) (XP_Gethash(hash, string_table[msg->id],
										(void*) ((uint32) msg->flags)));
	  if (msg->flags & MSG_FLAG_EXPUNGED) {
		/* Hoo boy.  The message was marked as deleted in the old summary file,
		   but we had lost that information in the new one.  Unfortunately,
		   the expunged flag has nasty side effects in the way things
		   are structured.  So, we'll call msg_MarkExpunged and have it
		   do that stuff for us. */
		msg_MarkExpunged(context, msgs, msg);
	  }
	}
	msg_apply_flags_from_hashtable(context, msgs, string_table,
								   msg->first_child, hash);
  }
}


int
msg_RescueOldFlags(MWContext* context, struct mail_FolderData* flags_msgs,
				   struct mail_FolderData* msgs)
{
  XP_HashTable table;
  table = XP_HashTableNew(flags_msgs->message_count, XP_StringHash, msg_cmp);
  if (!table) return MK_OUT_OF_MEMORY;
  msg_add_flags_to_hash_table(flags_msgs->string_table,
							  flags_msgs->expungedmsgs,
							  table);
  msg_add_flags_to_hash_table(flags_msgs->string_table, flags_msgs->hiddenmsgs,
							  table);
  msg_add_flags_to_hash_table(flags_msgs->string_table, flags_msgs->msgs,
							  table);
  msg_apply_flags_from_hashtable(context, msgs, msgs->string_table, msgs->msgs,
								 table);
  XP_HashTableDestroy(table);
  return 0;
}


int
msg_GetFolderData (MWContext* context, MSG_Folder* folder,
				   struct mail_FolderData **folder_ret)
{
  int status = 0;
  struct msg_FolderParseState *state = 0;
  mail_FolderData *msgs = 0;
  mail_FolderData *flags_msgs = 0;
  XP_StatStruct st;
  XP_File fid = 0;
  char *buf = 0;
  int size;
  char* ibuffer = 0;
  uint32 ibuffer_size = 0;
  uint32 ibuffer_fp = 0;
  const char* folder_name;

  XP_ASSERT(folder);
  if (!folder) return -1;
  XP_ASSERT(context->type != MWContextMail ||
			context->msgframe->opened_folder != folder);
								/* If this assert fails, best bet is to
								   fall through anyway...*/

  folder_name = folder->data.mail.pathname;

  if (XP_Stat(folder_name, &st, xpMailFolder))
	{
	  status = MK_MSG_FOLDER_UNREADABLE;
	  goto FAIL;
	}
  fid = XP_FileOpen(folder_name, xpMailFolderSummary, XP_FILE_READ_BIN);
  if (fid)
	{
	  msgs = msg_ReadFolderSummaryFile(context, fid, folder_name, NULL);
	  XP_FileClose(fid);
	  fid = 0;
	}

  if (!msgs ||
	  msgs->folderdate != st.st_mtime ||
	  msgs->foldersize != st.st_size)
	{
	  /* No summary file, or nothing in it, or it's out of date.
	   */

	  if (msgs) {
		flags_msgs = msgs;
		msgs = NULL;
	  }
	  fid = XP_FileOpen(folder_name, xpMailFolder, XP_FILE_READ_BIN);
	  if (!fid)
		{
		  status = MK_MSG_FOLDER_UNREADABLE;
		  goto FAIL;
		}
	  state = msg_BeginParsingFolder(context, NULL, 0);
	  if (!state)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}
	  size = 20 * 1024;
	  do {
		size /= 2;
		if (size == 0)
		  {
			status = MK_OUT_OF_MEMORY;
			goto FAIL;
		  }
		buf = (char*)XP_ALLOC(size);
	  } while (!buf);

	  for (;;)
		{
		  status = XP_FileRead(buf, size, fid);
		  if (status < 0) goto FAIL;
		  if (status == 0) break;
		  status = msg_LineBuffer(buf, status, &ibuffer, &ibuffer_size,
								  &ibuffer_fp, FALSE,
								  msg_ParseFolderLine, state);
		  if (status < 0) goto FAIL;
		}
	  XP_FileClose(fid);
	  if (ibuffer_fp > 0) {
		status = msg_ParseFolderLine(ibuffer, ibuffer_fp, state);
		ibuffer_fp = 0;
	  }
	  fid = 0;
	  msgs = msg_DoneParsingFolder(state, MSG_SortByMessageNumber, TRUE,
								   FALSE, FALSE);

	  state = NULL;
	  FREEIF(ibuffer);
	  if (!msgs)
		{
		  status = MK_OUT_OF_MEMORY; /* #### is this always the case??? */
		  goto FAIL;
		}
	  if (flags_msgs) {
		status = msg_RescueOldFlags(context, flags_msgs, msgs);
		if (status < 0) goto FAIL;
		msg_FreeFolderData(flags_msgs);
		flags_msgs = 0;
	  }
	}

  *folder_ret = msgs;
  return 0;

FAIL:
  FREEIF (buf);
  XP_ASSERT (!state);
  if (fid) XP_FileClose (fid);
  if (msgs) msg_FreeFolderData (msgs);
  if (flags_msgs) msg_FreeFolderData (flags_msgs);
  FREEIF(ibuffer);
  *folder_ret = 0;
  return status;
}


static MSG_Folder**
msg_find_parent_folder(MSG_Folder** start, const char* pathname)
{
  MSG_Folder** cur;
  for (cur = start ; *cur ; cur = &((*cur)->next)) {
	int l = XP_STRLEN((*cur)->data.mail.pathname);
	if (XP_STRNCMP((*cur)->data.mail.pathname, pathname, l) == 0 &&
		pathname[l] == '/') {
	  return msg_find_parent_folder(&((*cur)->children), pathname);
	}
  }
  return start;
}


static struct MSG_Folder *
msg_find_folder_1 (const char *folder_name, struct MSG_Folder *folders)
{
  while (folders)
	{
	  if (!XP_FILENAMECMP (folder_name, folders->data.mail.pathname))
		return folders;
	  if (folders->children)
		{
		  struct MSG_Folder *f =
			msg_find_folder_1 (folder_name, folders->children);
		  if (f) return f;
		}
	  folders = folders->next;
	}
  return 0;
}

struct MSG_Folder *
msg_FindMailFolder (MWContext *context, const char *folder_name,
					XP_Bool create_p)
{
  struct MSG_Folder *folder;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return 0;
  if (!folder_name || !*folder_name)
	return 0;
  folder = msg_find_folder_1 (folder_name, context->msgframe->folders);
  if (folder || !create_p)
	return folder;
  else
	{
	  /* Didn't find it.  Create it and add it to the list. */

	  char* copy = XP_STRDUP(folder_name);
	  if (copy) {
		char* ptr = XP_STRRCHR(copy, '/');
		MSG_Folder** parent;
		if (ptr) {
		  *ptr++ = '\0';
		}
		folder = msg_MakeMailFolder (context, copy, ptr);
		XP_FREE(copy);
		if (!folder) return 0;

		parent = msg_find_parent_folder(&context->msgframe->folders,
										folder->data.mail.pathname);

		folder->next = *parent;
		*parent = folder;
		msg_SortFolders (parent);

		msg_RedisplayFolders (context);
		return folder;
	  }
	  return NULL;
	}
}



/* This function takes a folder name and begins the process of opening the
 * folder, creating all the information necessary for optimized access and
 * returning a folder structure that can be passed to MSG_OpenMessage.
 *
 * context:   	the netscape window context
 * folder_name: a text string representing the folder name
 * message_id:  a text string representing the message id that will be asked
 *				for later. If message_id is NULL then open message will not
 *				be called later and this function should begin the process of
 *				updating the mail window to show all the messeges in the
 *				folder.
 *
 * folder_ptr:  This is the address of a pointer that will be passed to
 *				open_message;
 * 
 * Return values:
 *      If the folder gets completely opened and MSG_FinishOpenFolder does
 *      not need to be called then return MK_CONNECTED.
 *
 *      If the folder is not finished getting opened return
 *      MK_WAITING_FOR_CONNECTION
 *
 *      If an error occurs return a negative error listed in merrors.h
 */
int
MSG_BeginOpenFolderSock (MWContext *context, 
						 const char *folder_name,
						 const char *message_id, int msgnum,
						 void **folder_ptr)
{
  struct msg_mailbox_state *state = (struct msg_mailbox_state *) *folder_ptr;
  XP_StatStruct folderst;
  MSG_Folder *folder = 0;
  XP_Bool mail_context_p = (context->msgframe &&
							context->type == MWContextMail);
  XP_Bool needs_save = FALSE;

  if (folder_name && !*folder_name)
	return -1;

  if (!message_id && msgnum < 0 && !mail_context_p)
	{
	  XP_ASSERT (0); /* can only open mail folders in a mail window */
	  return -1;
	}

  XP_ASSERT (!state);
  if (!state)
	state = XP_NEW (struct msg_mailbox_state);
  if (!state)
	return MK_OUT_OF_MEMORY;
  XP_MEMSET (state, 0, sizeof(*state));

  *folder_ptr = (void *) state;

  state->mail_context_p = mail_context_p;
  if (mail_context_p) {
	msg_SaveMailSummaryChangesNow(context);
	folder = msg_FindMailFolder (context, folder_name, TRUE);
  }

  if (!folder)
	{
	  if (mail_context_p)
		return MK_OUT_OF_MEMORY; /* #### only way this can happen? */

	  /* Create a dummy folder for the duration of the parsing. */
	  folder = msg_MakeMailFolder (context, 0, folder_name);
	  if (! folder) return MK_OUT_OF_MEMORY;
	}

  state->folder = folder;

  if ((message_id || msgnum >= 0) && mail_context_p &&
	  folder && folder == context->msgframe->opened_folder)
	/* In the case where the folder was already current, and we're looking
	   for a message, don't even bother to stat it.
	   We're already there... */
	{
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_BeginOpenFolderSock: have folder for msg\n");
#endif
	  XP_ASSERT (!state->msgs);
	  state->msgs = context->msgframe->msgs;
	  return (MK_CONNECTED);
	}

  if (XP_Stat (folder_name, &folderst, xpMailFolder)) {
	XP_FREE (state);
	*folder_ptr = 0;
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_BeginOpenFolderSock: couldn't stat folder\n");
#endif
	return MK_MSG_FOLDER_UNREADABLE;
  }

  if (mail_context_p && folder == context->msgframe->opened_folder)
	{
	  /* We're already there... */
	  XP_ASSERT (!state->msgs);
	  state->msgs = context->msgframe->msgs;
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_BeginOpenFolderSock: have folder\n");
#endif
	  return (MK_CONNECTED);
	}

  /* Else, we don't have this folder's messages loaded, so we need
	 to parse the summary file or the folder itself.
   */
  state->file = XP_FileOpen(folder->data.mail.pathname, xpMailFolderSummary,
							XP_FILE_READ_BIN);

  state->folder_date = folderst.st_mtime;
  state->folder_size = folderst.st_size;

  if (! state->file)
	{
	  /* No summary file; go parse the folder itself. */
	PRINTF(("MSG_BeginOpenFolderSock: No summary file for %s; parsing folder.\n", folder->name));
	  goto PARSE_FOLDER;
	}

#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_BeginOpenFolderSock: reading summary for %s\n",
			  folder_name);
#endif
  XP_ASSERT (!state->msgs);
  state->msgs = msg_ReadFolderSummaryFile(context, state->file, folder_name,
										  &needs_save);

  XP_FileClose(state->file);
  state->file = 0;

  if (!state->msgs)
	{
	  /* Got nothing from summary file?  Reparse. */
	  PRINTF(("MSG_BeginOpenFolderSock: %s's summary file seems empty; parsing folder.\n",
			  folder->name));
	  goto PARSE_FOLDER;
	}

  if (state->msgs->folderdate == state->folder_date &&
	  state->msgs->foldersize == state->folder_size) {
	/* The summary file is completely up-to-date.  Thread it and
	   use it. */
	if (mail_context_p)
	  {
#ifdef MBOX_DEBUG
		fprintf(real_stderr,
				"MSG_BeginOpenFolderSock: summary up to date; rethreading\n");
#endif
	    context->msgframe->opened_folder = state->folder;
								/* Be sure to set the opened_folder *before*
								   rethreading, so that the threading code
								   knows whether we're in the "sent" folder,
								   so that it can know whether SortBySender
								   really means sort by recipient. */
		msg_Rethread(context, state->msgs);
		if (context->msgframe->msgs)
		  msg_FreeFolderData(context->msgframe->msgs);
		context->msgframe->msgs = state->msgs;
		context->msgframe->num_selected_msgs = 0;
		context->msgframe->opened_folder->unread_messages =
		  state->msgs->unread_messages;
		context->msgframe->opened_folder->total_messages =
		  state->msgs->total_messages;
		msg_DisableUpdates(context);
	    msg_RedisplayOneFolder(context, context->msgframe->opened_folder,
							   -1, -1, TRUE);
		if (!msg_auto_expand_mail_threads) {
		  msg_ElideAllToplevelThreads(context);
		}
		msg_RedisplayMessages(context);
		msg_EnableUpdates(context);
		if (needs_save) {
		  msg_SelectedMailSummaryFileChanged(context, NULL);
		}
	  }
	return(MK_CONNECTED);
  }

  /* Otherwise, the summary file exists, but is out of date.  Remember the data
	 structure, so we can later suck the flags out of it, and use them in
	 messages which match.  */

  PRINTF(("MSG_BeginOpenFolderSock: Summary file for %s seems out-of-date.\nUsing flags and reparsing\n",
		  folder->name));
#ifdef MBOX_DEBUG
	  fprintf(real_stderr,
			  "MSG_BeginOpenFolderSock: summary out of date; saving flags\n");
#endif
  state->flags_msgs = state->msgs;
  state->msgs = 0;

 PARSE_FOLDER:

  if (state->mail_context_p) {
	context->msgframe->force_reparse_hack = FALSE;
  }
  state->file = XP_FileOpen(folder_name, xpMailFolder, XP_FILE_READ_BIN);
  if (!state->file) {
	state->folder = 0;
#ifdef MBOX_DEBUG
	fprintf(real_stderr, "MSG_BeginOpenFolderSock: couldn't open %s\n",
			folder_name);
#endif
	return MK_MSG_FOLDER_UNREADABLE;
  }

  /* The folder file is now open, and netlib will call us as it reads
	 chunks of it.   Set up the buffers, etc. */

#ifdef MBOX_DEBUG
	fprintf(real_stderr, "MSG_BeginOpenFolderSock: parsing %s\n", folder_name);
#endif
  state->obuffer_size = 10240;
  state->obuffer = (char *) XP_ALLOC (state->obuffer_size);
  if (! state->obuffer)
	{
	  state->folder = 0;
	  return MK_OUT_OF_MEMORY;
	}

  state->pstate = msg_BeginParsingFolder (context, NULL, 0);
  if (! state->pstate)
	{
	  state->folder = 0;
	  return MK_OUT_OF_MEMORY;
	}

  state->graph_progress_total = folderst.st_size;
  FE_GraphProgressInit (context, 0, state->graph_progress_total);

  return(MK_WAITING_FOR_CONNECTION);
}

/* This function finishes what MSG_BeginOpenFolder
 * starts.  This function can return MK_WAITING_FOR_CONNECTION
 * as many times as it needs and will be called again
 * after yeilding to user events until it returns
 * MK_CONNECTED or a negative error code.
 */
int
MSG_FinishOpenFolderSock (MWContext *context, 
						  const char *folder_name,
						  const char *message_id, int msgnum,
						  void **folder_ptr)
{
  struct msg_mailbox_state *state = (struct msg_mailbox_state *) *folder_ptr;
  int status;
  XP_ASSERT (state);
  if (! state) return -1;

  if (! state->file)
	{
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_FinishOpenFolderSock: no file??\n");
#endif
	  return MK_MSG_FOLDER_UNREADABLE;
	}

  /* Read the next chunk of data from the file. */
  status = XP_FileRead (state->obuffer, state->obuffer_size, state->file);
#ifdef MBOX_DEBUG
	fprintf(real_stderr, "MSG_FinishOpenFolderSock: parsed %d of %s\n",
			status, folder_name);
#endif

	if (status > 0 &&
		state->graph_progress_total > 0 &&
		state->graph_progress_received == 0)
	  {
		/* This is the first block from the file.  Check to see if this
		   looks like a mail file. */
		const char *s = state->obuffer;
		const char *end = s + state->obuffer_size;
		while (s < end && XP_IS_SPACE(*s))
		  s++;
		if ((end - s) < 20 || !msg_IsEnvelopeLine(s, end - s))
		  {
			char buf[500];
			PR_snprintf (buf, sizeof(buf),
						 XP_GetString(MK_MSG_NON_MAIL_FILE_READ_QUESTION),
						 folder_name);
			if (!FE_Confirm (context, buf))
			  return -1; /* #### NOT_A_MAIL_FILE */
		  }
	  }

	if (state->graph_progress_total > 0)
	  {
		int32 percent;
		if (status > 0)
		  state->graph_progress_received += status;
		percent = 100 * (((double) state->graph_progress_received) /
						 ((double) state->graph_progress_total));
		FE_SetProgressBarPercent (context, percent);
		FE_GraphProgress (context, 0,
						  state->graph_progress_received, status,
						  state->graph_progress_total);
	  }

  if (status < 0)
	{
	  state->folder = 0;
	  return status;
	}
  else if (status == 0)
	{
	  /* End of file.  Flush out any partial line remaining in the buffer. */
	  if (state->ibuffer_fp > 0) {
		msg_ParseFolderLine(state->ibuffer, state->ibuffer_fp, state->pstate);
		state->ibuffer_fp = 0;
	  }

	  /* Install the messages.
	   */
	  XP_ASSERT (state->pstate);
	  if (!state->pstate)
		{
		  state->folder = 0;
		  return MK_OUT_OF_MEMORY;
		}

	  XP_ASSERT (!state->msgs);
	  state->msgs = msg_DoneParsingFolder (state->pstate,
										   (context->msgframe ?
											context->msgframe->sort_key :
											MSG_SortByMessageNumber),
										   (context->msgframe ?
											context->msgframe->sort_forward_p :
											TRUE),
										   (context->msgframe ?
											context->msgframe->thread_p :
											FALSE),
										   FALSE);
	  state->pstate = 0;

	  if (state->flags_msgs)
		{
#ifdef MBOX_DEBUG
		  fprintf(real_stderr, "MSG_FinishOpenFolderSock: installing flags\n");
#endif
		  status = msg_RescueOldFlags(context, state->flags_msgs, state->msgs);
		  msg_FreeFolderData(state->flags_msgs);
		  state->flags_msgs = NULL;
		  if (status < 0) return status;
		}

	  if (state->msgs)
		{
		  XP_File summary;
#ifdef MBOX_DEBUG
		  fprintf(real_stderr, "MSG_FinishOpenFolderSock: saving summary\n");
#endif
		  summary = XP_FileOpen(state->folder->data.mail.pathname,
								xpMailFolderSummary, XP_FILE_WRITE_BIN);
		  if (summary)
			{
			  int status;
			  state->msgs->folderdate = state->folder_date;
			  state->msgs->foldersize = state->folder_size;

#ifdef XP_UNIX
			  {
				/* Clone the permissions of the folder file into the
				   summary file.  (Except make sure the summary is
				   readable/writable by owner.) */
				XP_StatStruct st;
				if (XP_Stat (state->folder->data.mail.pathname, &st,
							 xpMailFolder)
					== 0)
				  /* Ignore errors; if it fails, bummer. */
				  fchmod (fileno(summary), (st.st_mode | S_IRUSR | S_IWUSR));
			  }
#endif /* XP_UNIX */

			  status = msg_WriteFolderSummaryFile (state->msgs, summary);
			  XP_FileClose (summary);
			  if (status < 0) return status;
			}
		}

	  if (state->mail_context_p)
		{
		  XP_Bool folder_changed = (context->msgframe->opened_folder !=
									state->folder);
		  context->msgframe->opened_folder = state->folder;
		  if (context->msgframe->msgs != state->msgs)
			{
#ifdef MBOX_DEBUG
			  fprintf(real_stderr,
					  "MSG_FinishOpenFolderSock: installing folderData\n");
#endif
			  msg_FreeFolderData (context->msgframe->msgs);
			  context->msgframe->msgs = state->msgs;
			  context->msgframe->num_selected_msgs = 0;
			  folder_changed = TRUE;
			}
		  msg_SelectedMailSummaryFileChanged (context, NULL);

		  if (folder_changed || (message_id == NULL && msgnum < 0))
			{
#ifdef MBOX_DEBUG
			  fprintf(real_stderr,
					  "MSG_FinishOpenFolderSock: redisplaying threads\n");
#endif
			  context->msgframe->opened_folder->unread_messages =
				state->msgs->unread_messages;
			  context->msgframe->opened_folder->total_messages =
				state->msgs->total_messages;
			  msg_DisableUpdates(context);
			  msg_RedisplayOneFolder(context,
									 context->msgframe->opened_folder,
									 -1, -1, TRUE);
			  if (!msg_auto_expand_mail_threads) {
				msg_ElideAllToplevelThreads(context);
			  }
			  msg_RedisplayMessages(context);
			  msg_ClearMessageArea(context);
			  msg_EnableUpdates(context);
			}
		}

	  /* We're done reading the folder - we don't need these things
		 any more. */
	  FREEIF (state->ibuffer);
	  FREEIF (state->obuffer);

	  return MK_CONNECTED;
	}
  else
	{
	  status = msg_LineBuffer (state->obuffer, status,
							   &state->ibuffer, &state->ibuffer_size,
							   &state->ibuffer_fp, FALSE,
							   msg_ParseFolderLine, state->pstate);
	  if (status < 0)
		{
		  state->folder = 0;
		  return status;
		}
	}
  return(MK_WAITING_FOR_CONNECTION);
}


/* This function should close a folder opened
 * by MSG_BeginOpenFolder
 */
void
MSG_CloseFolderSock (MWContext *context, const char *folder_name,
					 const char *message_id, int msgnum, void *folder_ptr)
{
  struct msg_mailbox_state *state = (struct msg_mailbox_state *) folder_ptr;
  XP_ASSERT (state);
  if (! state)
	return;

  if (state->graph_progress_total > 0)
	FE_GraphProgressDestroy (context, 0,
							 state->graph_progress_total,
							 state->graph_progress_received);

  if (state->file)
	XP_FileClose(state->file);
  FREEIF (state->ibuffer);
  FREEIF (state->obuffer);
  if (state->flags_msgs)
	msg_FreeFolderData(state->flags_msgs);

  if (! state->mail_context_p)
	{
	  /* If this mailbox: url was opened in a non-mail context, then
		 we created a dummy folder object just for the duration of
		 this url; we should free it now (this implies that mailbox:
		 urls are more efficient in mail windows.  That's ok. */
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_CloseFolderSock: discarding tmp folder\n\n");
#endif
	  XP_ASSERT (message_id || msgnum >= 0);
	  if (state->msgs)
	  msg_FreeFolderData(state->msgs);
	  if (state->folder)
		{
		  FREEIF (state->folder->name);
		  FREEIF (state->folder->data.mail.pathname);
		  XP_FREE (state->folder);
		}
	}
  else
	{
	  if (context->msgframe) context->msgframe->cacheinfo.valid = FALSE;
	  msg_UpdateToolbar(context);
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_CloseFolderSock: freeing\n\n");
#endif
	}

  XP_FREE (state);
}


/* This function opens a message and returns a handle to that message in the
 * msg_ptr pointer.
 *
 * The message handle will be passed to MSG_ReadMessage and MSG_CloseMessage
 * to read data and to close the message
 *
 * Return values: return a negative return value listed in merrors.h to
 * signify an error.  return zero (0) on success.
 *
 * !Set message_ptr to NULL on error!
 */
int
MSG_OpenMessageSock (MWContext *context, const char *folder_name,
					 const char *msg_id, int msgnum,
					 void *folder_ptr, void **message_ptr,
					 int32 *content_length)
{
  struct msg_mailbox_state *state = (struct msg_mailbox_state *) folder_ptr;
  XP_ASSERT (state);
  if (! state) return -1;

  *message_ptr = folder_ptr;

  XP_ASSERT (state->msgs);
  if (! state->msgs)
	{
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_OpenMessageSock: no messages!\n");
#endif
	  return MK_MSG_FOLDER_UNREADABLE;
	}

  XP_ASSERT(state->msg == NULL);
  if (msgnum >= 0 && msgnum < state->msgs->message_count) {
	state->msg = state->msgs->msglist
	  [msgnum/mail_MSG_ARRAY_SIZE][msgnum%mail_MSG_ARRAY_SIZE];
	if (msg_id &&
		XP_STRCMP(msg_id, state->msgs->string_table[state->msg->id]) != 0) {
	  state->msg = NULL;
	}
  }
  if (state->msg == NULL && msg_id) {
	state->msg = (MSG_ThreadEntry *)
	  XP_Gethash (state->msgs->message_id_table, msg_id, 0);
  }

  if (! state->msg)	/* id not in folder */
	{
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_OpenMessageSock: no id %s in %s!\n",
			  msg_id, folder_name);
#endif
	  return MK_MSG_ID_NOT_IN_FOLDER;
	}

  state->msg_bytes_remaining = state->msg->data.mail.byte_length;

  if (! state->file)
	{
	  /* It won't be open if this folder was selected already, or if
		 we got this folder by reading the summary file. */
	  state->file = XP_FileOpen(folder_name, xpMailFolder, XP_FILE_READ_BIN);
	  if (!state->file) {
		state->folder = 0;
#ifdef MBOX_DEBUG
		fprintf(real_stderr, "MSG_OpenMessageSock: couldn't open %s\n",
				folder_name);
#endif
		return MK_MSG_FOLDER_UNREADABLE;
	  }
#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_OpenMessageSock: opened %s\n", folder_name);
#endif
	}

  *content_length = state->msg->data.mail.byte_length;

  /* #### does this return a status code? */
  XP_FileSeek (state->file, state->msg->data.mail.file_index, SEEK_SET);

#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_OpenMessageSock: seek\n");
#endif

  return 0;
}

/* this function should work just like UNIX read(3)
 *
 * "buffer" should be filled up to the size of "buffer_size"
 * with message data.
 *
 * Return values
 *	Return the number of bytes put into "buffer", or
 *  Return zero(0) at end of message, or
 *  Return a negative error value from merrors.h or sys/errno.h
 */
int
MSG_ReadMessageSock (MWContext *context, const char *folder_name,
					 void *message_ptr, const char *message_id, int msgnum,
					 char * buffer, int32 buffer_size)
{
  struct msg_mailbox_state *state = (struct msg_mailbox_state *) message_ptr;
  int L;
  XP_ASSERT (state);
  if (! state) return -1;

  XP_ASSERT (state->msg && state->file);
  if (!state->msg || !state->file)
	return -1;

  if (state->msg_bytes_remaining == 0)
	return 0;

  if (!state->discarded_envelope_p &&
	  !state->wrote_fake_id_p)
	{
	  /* Before emitting any of the `real' data, emit a dummy Message-ID
		 header if this was an IDless message.  This is so that the MIME
		 parsing code will call MSG_ActivateReplyOptions() with an ID
		 that it can use when generating (among other things) the URL
		 to be used to forward this message to another user.
	   */
	  char *id = state->msgs->string_table[state->msg->id];
	  state->wrote_fake_id_p = TRUE;
	  if (id && !XP_STRNCMP ("md5:", id, 4))
		{
		  XP_ASSERT (buffer_size > (XP_STRLEN(id) + 40));
		  XP_STRCPY (buffer, "Message-ID: <");
		  XP_STRCAT (buffer, id);
		  XP_STRCAT (buffer, ">" LINEBREAK);
		  return XP_STRLEN (buffer);
		}
	}

  L = XP_FileRead (buffer,
				   (state->msg_bytes_remaining <= buffer_size
					? state->msg_bytes_remaining
					: buffer_size),
				   state->file);
  if (L > 0)
	state->msg_bytes_remaining -= L;

  if (L > 0 && !state->discarded_envelope_p)
	{
	  char *s;
	  for (s = buffer; s < buffer + L; s++)
		if (*s == CR || *s == LF)
		  {
			if (*s == CR && *(s+1) == LF)
			  s++;
			s++;
			break;
		  }
	  if (s != buffer)
		{
		  /* Take the first line off the front of the buffer */
		  uint32 off = s - buffer;
		  L -= off;
		  for (s = buffer; s < buffer + L; s++)
			*s = *(s+off);
		  state->discarded_envelope_p = TRUE;
		}
	  else
		{
		  /* discard this whole buffer */
		  L = 0;
		}
	}

#ifdef MBOX_DEBUG
	  fprintf(real_stderr, "MSG_ReadMessageSock: read %d\n", L);
#endif

  return L;
}

/* This function should close a message opened
 * by MSG_OpenMessage
 */
void
MSG_CloseMessageSock (MWContext *context, const char *folder_name,
					  const char *message_id, int msgnum, void *message_ptr)
{
  struct msg_mailbox_state *state = (struct msg_mailbox_state *) message_ptr;
  if (state->msg && context->msgframe && context->msgframe->opened_folder) {
	msg_DisableUpdates(context);
	msg_ChangeFlag(context, state->msg, MSG_FLAG_READ, 0);
	context->msgframe->displayed_message = state->msg;
	msg_EnsureSelected(context, state->msg); /* Select if not already
												selected. */
	msg_RedisplayOneMessage(context, state->msg, -1, 0, TRUE); /* Scroll to
																  this msg. */
	context->msgframe->cacheinfo.valid = FALSE;
	msg_UpdateToolbar(context);
	msg_EnableUpdates(context);
  }
}


/* Partial messages
 */


char *
MSG_GeneratePartialMessageBlurb (MWContext *context,
								 URL_Struct *url, void *closure,
								 MimeHeaders *headers)
{
  char *stuff = 0;
  char *new_url = 0, *quoted = 0;
  unsigned long flags = 0;
  char dummy = 0;
  char *uidl = 0, *xmoz = 0;

  /* The message is partial if it has an X-UIDL header field, and it has
	 an X-Mozilla-Status header field which contains the `partial' bit.
   */

  uidl = MimeHeaders_get(headers, HEADER_X_UIDL, FALSE, FALSE);
  if (!uidl) goto DONE;
  xmoz = MimeHeaders_get(headers, HEADER_X_MOZILLA_STATUS, FALSE, FALSE);
  if (!xmoz) goto DONE;

  if (1 != sscanf (xmoz, " %lx %c", &flags, &dummy))
	goto DONE;
	  
  if (! (flags & MSG_FLAG_PARTIAL))
	goto DONE;

  quoted = NET_Escape (uidl, URL_XALPHAS);
  if (!quoted) goto DONE;
  FREEIF(uidl);

  new_url = (char *) XP_ALLOC (XP_STRLEN (url->address) +
							   XP_STRLEN (quoted) + 20);
  if (!new_url) goto DONE;

  XP_STRCPY (new_url, url->address);
  XP_ASSERT (XP_STRCHR (new_url, '?'));
  if (XP_STRCHR (new_url, '?'))
	XP_STRCAT (new_url, "&uidl=");
  else
	XP_STRCAT (new_url, "?uidl=");
  XP_STRCAT (new_url, quoted);
  FREEIF(quoted);

  stuff = PR_smprintf ("%s%s%s%s",
					   XP_GetString(MK_MSG_PARTIAL_MSG_FMT_1),
					   XP_GetString(MK_MSG_PARTIAL_MSG_FMT_2),
					   new_url,
					   XP_GetString(MK_MSG_PARTIAL_MSG_FMT_3));

DONE:
  FREEIF(new_url);
  FREEIF(quoted);
  FREEIF(uidl);
  FREEIF(xmoz);

  return stuff;
}
