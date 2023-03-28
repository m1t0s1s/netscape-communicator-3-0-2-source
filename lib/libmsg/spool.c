/* -*- Mode: C; tab-width: 4 -*-
   spool.c --- manipulation of mail files (adding and removing messages.)
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-May-95.

   This file contains functions for manipulating the contents of mail files.
   To this end, it contains Yet Another parser for BSD mail files, but only
   to the extent that the code in this file needs it; the full parser lives
   in mailsum.c.

   The somewhat-interrelated things which this file deals with are:

    - incorporating new messages into a folder from POP or movemail;
	- moving, deleting, or copying messages between folders;
	- emptying the trash;
	- compressing folders;
	- and taking messages from the "queue" folder and delivering them.
 */

#include "msg.h"
#include "msgcom.h"
#include "mailsum.h"
#include "msgundo.h"
#include "xp_str.h"
#include "merrors.h"
#include "xpgetstr.h"

extern int MK_MIME_ERROR_WRITING_FILE;
/* extern int MK_MSG_CANT_COPY_TO_DRAFTS_FOLDER; */
extern int MK_MSG_CANT_COPY_TO_QUEUE_FOLDER;
extern int MK_MSG_CANT_COPY_TO_SAME_FOLDER;
extern int MK_MSG_CANT_CREATE_INBOX;
extern int MK_MSG_ERROR_WRITING_MAIL_FOLDER;
extern int MK_MSG_FOLDER_SUMMARY_UNREADABLE;
extern int MK_MSG_FOLDER_UNREADABLE;
extern int MK_MSG_NON_MAIL_FILE_WRITE_QUESTION;
extern int MK_MSG_NO_POP_HOST;
extern int MK_OUT_OF_MEMORY;
extern int MK_UNABLE_TO_OPEN_FILE;
extern int MK_MSG_COULDNT_OPEN_FCC_FILE;  
extern int MK_MSG_COMPRESS_FOLDER_ETC;
extern int MK_MSG_COMPRESS_FOLDER_DONE;
extern int MK_MSG_CANT_OPEN;
extern int MK_MSG_BOGUS_QUEUE_MSG_1;
extern int MK_MSG_BOGUS_QUEUE_MSG_N;
extern int MK_MSG_BOGUS_QUEUE_REASON;
extern int MK_MSG_WHY_QUEUE_SPECIAL;
extern int MK_MSG_QUEUED_DELIVERY_FAILED;
extern int MK_MSG_DELIVERY_FAILURE_1;
extern int MK_MSG_DELIVERY_FAILURE_N;


/* #### Why is this alegedly netlib-specific?   What else am I to do? */
extern char *NET_ExplainErrorDetails (int code, ...);


/* First, some random utilities.
 */

XP_Bool
msg_ConfirmMailFile (MWContext *context, const char *file_name)
{
  XP_File in = XP_FileOpen (file_name, xpMailFolder, XP_FILE_READ_BIN);
  char buf[100];
  char *s = buf;
  int L;
  if (!in) return TRUE;
  L = XP_FileRead(buf, sizeof(buf)-2, in);
  XP_FileClose (in);
  if (L < 1) return TRUE;
  buf[L] = 0;
  while (XP_IS_SPACE(*s))
	s++, L--;
  if (L > 5 && msg_IsEnvelopeLine(s, L))
	return TRUE;
  PR_snprintf (buf, sizeof(buf),
			   XP_GetString (MK_MSG_NON_MAIL_FILE_WRITE_QUESTION), file_name);
  return FE_Confirm (context, buf);
}


/* If a mail context exists, make sure it lists the queue folder.
   The passed-in context may be of any type. */
void
msg_EnsureQueueFolderListed (MWContext* context)
{
  static MSG_Folder *folder = 0;
  if (!context || context->type != MWContextMail)
	context = XP_FindContextOfType (context, MWContextMail);
  if (context)
	folder = msg_FindFolderOfType(context, MSG_FOLDER_FLAG_QUEUE);
  if (folder)
	{
	  XP_StatStruct st;
	  if (XP_Stat(folder->data.mail.pathname, &st, xpMailFolder))
		{
		  /* File doesn't exist...  Create it, zero-length. */
		  XP_File f = XP_FileOpen(folder->data.mail.pathname, xpMailFolder,
								  XP_FILE_WRITE_BIN);
		  if (f)
			{
#ifdef XP_UNIX
			  /* When creating the queue file for the first time, make it
				 readable and writable only by owner. */
			  fchmod (fileno(f), (S_IWUSR | S_IRUSR));
#endif /* XP_UNIX */
			  XP_FileClose(f);
			}
		}
	}
}

static void
msg_open_first_unseen(URL_Struct *url, int status, MWContext *context)
{
  msg_GotoMessage (context, MSG_NextUnreadMessage);
}

/* Makes sure the "inbox" folder exists.  If it doesn't, creates it, and
   places an introductory message in it.  Returns a folder object if the
   folder was so created; returns 0 if it failed, or already existed. */
void
msg_EnsureInboxExists (MWContext *context)
{
  MSG_Folder* folder;
  XP_StatStruct st;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail)
	return;
  folder = msg_FindFolderOfType(context, MSG_FOLDER_FLAG_INBOX);
  if (XP_Stat(folder->data.mail.pathname, &st, xpMailFolder))
	{
	  /* File doesn't exist...  Create it. */
	  char *msg = 0;
	  char *s;
	  int32 L = 0;
	  void *fe_data;
	  XP_File file;
	  char *content_type;
	  char about_url[] = /* "mailintro" */
		"\262\246\256\261\256\263\271\267\264";

	  for (s = about_url; *s; s++)
		*s -= 69;

	  fe_data = FE_AboutData(about_url, &msg, &L, &content_type);
	  XP_ASSERT (msg);
	  if (!msg) return;

	  file = XP_FileOpen (folder->data.mail.pathname, xpMailFolder,
						  XP_FILE_WRITE_BIN);
	  if (!file)
		{
		  FE_Alert(context, XP_GetString (MK_MSG_CANT_CREATE_INBOX));
		}
	  else
		{
		  char* envelope = msg_GetDummyEnvelope();
		  /* No preceeding newline here because it's the first line of
			 the file. */
		  XP_FileWrite (envelope, XP_STRLEN(envelope), file);
		  s = msg;
		  while (L > 0)
			{
			  int wrote = XP_FileWrite (s, L, file);
			  if (wrote < L)
				{
				  XP_FileClose (file);
				  file = 0;   /* don't close it again */
				  folder = 0; /* don't open it */
				  XP_FileRemove (folder->data.mail.pathname, xpMailFolder);
				  break;
				}
			  L -= wrote;
			  s += wrote;
			}
		}
	  if (file) {
		if (XP_FileClose(file) != 0) {
		  XP_FileRemove(folder->data.mail.pathname, xpMailFolder);
		  folder = 0;
		}
	  }
		  
#ifdef XP_MAC
	  FE_FreeAboutData (fe_data, about_url);
#endif /* XP_MAC */

	  if (folder)
		/* Open up the new inbox folder, complete with magic introductory
		   message, and automatically show that message to the user. */
		msg_SelectAndOpenFolder(context, folder, msg_open_first_unseen);
	}
}


/* Return TRUE if some outside force seems to have done something to
   the current folder.  If so, turns on force_reparse_hack, so that if
   we save the summary file, we'll still reparse. */

static XP_Bool
msg_folder_mysteriously_changed(MWContext* context)
{
  XP_Bool result = FALSE;
  XP_StatStruct folderst;
  XP_ASSERT(context->msgframe != NULL);
  if (context->msgframe->opened_folder) {
	if (context->msgframe->msgs == NULL) result = TRUE;
	else {
	  if (XP_Stat((char*)context->msgframe->opened_folder->data.mail.pathname,
				  &folderst, xpMailFolder)) {
		result = TRUE;
	  } else {
		if (context->msgframe->msgs->folderdate != folderst.st_mtime ||
			context->msgframe->msgs->foldersize != folderst.st_size) {
		  result = TRUE;
		}
	  }
	}
  }
  if (result) {
	PRINTF(("The current folder seems to have mysteriously been changed.\n"));
	context->msgframe->force_reparse_hack = TRUE;
  }
  return result;
}



/* Writing messages to files - used for folder compression as well as
   incorporating or moving messages.
 */

typedef struct msg_write_state {
  XP_File fid;
  XP_Bool inhead;
  MSG_ThreadEntry* msg;
  uint32 position;
#ifdef EMIT_CONTENT_LENGTH
  uint32 header_bytes;
#endif /* EMIT_CONTENT_LENGTH */
  XP_Bool update_msg;
  char *uidl;
  int32 numskip;
  int32 (*writefunc)(char* line, uint32 length, void* closure);
  void* writeclosure;
} msg_write_state;


static int32
msg_writemsg_handle_line(char* line, uint32 length, void* closure)
{
  msg_write_state* state = (msg_write_state*) closure;
  char* buf;
  uint32 buflength;
  if (state->numskip > 0) {
	state->numskip--;
	return 0;
  }
  if (state->inhead) {

#ifdef EMIT_CONTENT_LENGTH
	/* Keep track of how many bytes the headers consume. */
	state->header_bytes += length;
#endif /* EMIT_CONTENT_LENGTH */

	if (line[0] == CR || line[0] == LF || length == 0) {
	  
	  /* If we're at the end of the headers block, maybe write out the
		 X-Mozilla-Status and X-UIDL headers.
	   */
	  uint16 flags = 0;
	  XP_Bool write_status_header = FALSE;

	  if (state->msg == NULL ||
		  state->position < state->msg->data.mail.file_index + 0xffff) {
		if (state->update_msg && state->msg) {
		  /* If we're compressing the folder, make sure we note where the
		     X-Mozilla-Status line is. */
		  state->msg->data.mail.status_offset =
			state->position - state->msg->data.mail.file_index;
		  XP_ASSERT(state->msg->data.mail.status_offset < 10000); /* ### Debugging hack */
		}

		if (state->msg)
		  flags = (state->msg->flags & ~MSG_FLAG_RUNTIME_ONLY);
		/* If there's a message, then get the flags from it and write them
		   out in the X-Mozilla-Status header. */
		write_status_header = TRUE;
	  }

	  if (state->uidl)
		{
		  /* If there's a UIDL, then this is a partial message.  Make sure
			 that the flags indicate this, and that the flags get written
			 out (there is only an X-UIDL header when there is also an
			 X-Mozilla-Status header.) */
		  write_status_header = TRUE;
		  flags |= MSG_FLAG_PARTIAL;
		  if (state->msg)
			state->msg->flags |= MSG_FLAG_PARTIAL;
		}

	  if (write_status_header)
		{
		  buf = PR_smprintf(X_MOZILLA_STATUS ": %04x" LINEBREAK, flags);
		  if (buf) {
			buflength = X_MOZILLA_STATUS_LEN + 6 + LINEBREAK_LEN;
			XP_ASSERT(buflength == XP_STRLEN(buf));
			/* don't increment state->header_bytes here; that applies to
			   the headers in the old file, not the new one. */
			if (XP_FileWrite(buf, buflength, state->fid) < buflength) {
			  XP_FREE(buf);
			  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
			}
			if (state->writefunc) {
			  (*state->writefunc)(buf, buflength, state->writeclosure);
			}
			state->position += buflength;
			XP_FREE(buf);
		  }
		}

	  if (state->uidl)
		{
		  buflength = X_UIDL_LEN + 2 + XP_STRLEN (state->uidl) + LINEBREAK_LEN;
		  buf = (char*) XP_ALLOC(buflength + 1);
		  if (buf) {
			XP_STRCPY (buf, X_UIDL ": ");
			XP_STRCAT (buf, state->uidl);
			XP_STRCAT (buf, LINEBREAK);
			XP_ASSERT(buflength == XP_STRLEN(buf));
			/* don't increment state->header_bytes here; that applies to
			   the headers in the old file, not the new one. */
			if (XP_FileWrite(buf, buflength, state->fid) < buflength) {
			  XP_FREE(buf);
			  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
			}
			if (state->writefunc) {
			  (*state->writefunc)(buf, buflength, state->writeclosure);
			}
			state->position += buflength;
			XP_FREE(buf);
		  }
		}

#ifdef EMIT_CONTENT_LENGTH
	  /* Now write a Content-Length header.
		 This must happen after all headers from the input file have been
		 seen (but needn't necessarily be the last header we emit here.)

		 Note that Content-Length should exist only in disk files -- it should
		 never be allowed to escape to the network, because it is sensitive to
		 LINEBREAK_LEN, which is different on different systems.  Of course,
		 this means that if a mail folder is transferred from one system to
		 another, and the linebreaks get translated, then the Content-Length
		 header will be incorrect, which is one of the reasons that
		 Content-Length is a fundamentally flawed idea.

		 The Content-Length is the number of bytes in the body of the message;
		 the body begins just after the blank line that terminates the headers,
		 and ends at the end of the message (which is presumably exactly at
		 EOF, or directly before the envelope of the next message.  Note that
		 in this case, newline before the "From " line is a part of the
		 envelope, not a part of the preceeding message.)
	   */
	  if (state->msg)
		{
		  int32 cl = (state->msg->data.mail.byte_length
					- state->header_bytes
					- LINEBREAK_LEN);

		  if (cl < 0)
			{
			  /* This shouldn't happen in a mail file which is to spec.
				 However, when we FCC a message with no body to a file,
				 we are inserting one too few newlines, thus the mail
				 folders we generate are out of spec.  This is what
				 #ifdef FIXED_SEPERATORS is supposed to fix.

				 Currently, we're only out of spec by one newline, so
				 I don't know of any cases where we're off by more than
				 one, so let's assert if we get any more-negative numbers
				 to see what turns up.
			   */
			  XP_ASSERT(cl >= -1);
			  cl = 0;
			}

		  buf = PR_smprintf(CONTENT_LENGTH ": %d" LINEBREAK, cl);
		  if (buf) {
			buflength = XP_STRLEN(buf);
			/* don't increment state->header_bytes here; that applies to
			   the headers in the old file, not the new one. */
			if (XP_FileWrite(buf, buflength, state->fid) < buflength) {
			  XP_FREE(buf);
			  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
			}
			if (state->writefunc) {
			  (*state->writefunc)(buf, buflength, state->writeclosure);
			}
			state->position += buflength;
			XP_FREE(buf);
		  }
		}

	  state->header_bytes = 0; /* done with them now */

#endif /* EMIT_CONTENT_LENGTH */

	  /* Now fall through to write out the blank line */
	  state->inhead = FALSE;
	} else {
	  /* If this is the X-Mozilla-Status header, don't write it (since we
		 will rewrite it again at the end of the header block.) */
	  if (!XP_STRNCMP(line, X_MOZILLA_STATUS, X_MOZILLA_STATUS_LEN)) return 0;

	  /* Likewise, if we are writing a UIDL header, discard an existing one.
		 (In the case of copying a `partial' message from one folder to
		 another, state->uidl will be false, so we will simply copy the
		 existing X-UIDL header like it was any other.)
	   */
	  if (state->uidl && !XP_STRNCMP(line, X_UIDL, X_UIDL_LEN)) return 0;

	  /* Likewise Content-Length.  But, if there's no state->msg, allow
		 the existing one to pass through. */
	  if (state->msg && !XP_STRNCMP(line, CONTENT_LENGTH, CONTENT_LENGTH_LEN))
		return 0;

	}
  }
  if (XP_FileWrite(line, length, state->fid) < length) {
	return MK_MSG_ERROR_WRITING_MAIL_FOLDER;	/* ### Need to make sure we
												   handle out of disk space
												   properly. */
  }
  if (state->writefunc) {
	(*state->writefunc)(line, length, state->writeclosure);
  }
  
  state->position += length;
  return 0;
}



/* Folder compression
 */

typedef struct msg_compress_folder_state {
  MSG_Folder* folder;			/* Folder being compressed. */
  XP_Bool doall;				/* Whether we're to compress all the folders,
								   rather than just this one. */
  XP_File infile;				/* The original folder we're parsing. */
  XP_File outfile;				/* The new compressed folder. */
  char* tmpname;				/* Temporary filename of new folder. */
  XP_FileType tmptype;
  char *obuffer;
  uint32 obuffer_size;
  char *ibuffer;
  uint32 ibuffer_size;
  uint32 ibuffer_fp;
  struct msg_FolderParseState* parsestate;
  mail_FolderData* msgs;		/* Message info for this folder. */
  mail_FolderData* flags_msgs;	/* Message info from the out-of-date summary
								   file for this folder. */
  MSG_ThreadEntry* msg;			/* Current message. */
  int msgindex;					/* Original index number for this message. */
  int num;						/* New index number for this message. */
  msg_write_state writestate;
  XP_Bool needwritesummary;
  XP_StatStruct folderst;
  int32 position;
  int32 oldstart;				/* Where the current message started, as an
								   offset into the original folder. */

  int32 graphcur;
  int32 graphtotal;
} msg_compress_folder_state;



static void 
msg_compress_string_table(MWContext* context, mail_FolderData* msgs)
{
  int num;
  uint16* count;
  int i;
  int last;
  uint16* refs;
  MSG_ThreadEntry* msg;
  if (!msgs || !msgs->string_table)
	return;

  for (num=1 ; msgs->string_table[num] ; num++) ;

  count = (uint16*) XP_CALLOC(num, sizeof(uint16));
  if (!count) return;

  /* First go through all the messages, and fill "count" with the number
	 of references to the strings.  The elements of count which end up
	 being 0 indicate strings which are unused.
   */
  for (i=0 ; i<msgs->total_messages ; i++) {
	msg = msgs->msglist[i / mail_MSG_ARRAY_SIZE][i % mail_MSG_ARRAY_SIZE];
	XP_ASSERT(msg->sender < num);
	XP_ASSERT(msg->recipient < num);
	XP_ASSERT(msg->subject < num);
	XP_ASSERT(msg->id < num);
	count[msg->sender]++;
	count[msg->recipient]++;
	count[msg->subject]++;
	count[msg->id]++;
	if (msg->references) {
	  for (refs = msg->references ; *refs ; refs++) {
		XP_ASSERT(*refs < num);
		count[*refs]++;
	  }
	}
  }

  /* Now we re-use the "count" array as a table mapping the old indexes
	 to the new indexes: iterate through the array.  If a slot contains
	 a 0, free the string it indicates.  Otherwise, add the indicated
	 string to the new folder string table; and store the index at which
	 it has been added in "count".  So, at the end of this loop, the
	 string_table doesn't contain unused strings, and "count" contains
	 a mapping from old string table indexes to new ones.
   */

  count[0] = 0;
  last = 1;
  for (i=1 ; i<num ; i++) {
	XP_ASSERT (msgs->string_table[i] && *msgs->string_table[i]);
	if (count[i]) {
	  if (i != last) {
		XP_ASSERT(msgs->string_table[last] == NULL);
		msgs->string_table[last] = msgs->string_table[i];
		msgs->string_table[i] = NULL;
	  }
	  count[i] = last++;
	} else {
	  FREEIF(msgs->string_table[i]);
	  XP_ASSERT (count[i] == 0);
	  count[i] = 0;
	}
  }

  /* Now go through the messages again and map the old indexes to the new ones.
   */
  for (i=0 ; i<msgs->total_messages ; i++) {
	msg = msgs->msglist[i / mail_MSG_ARRAY_SIZE][i % mail_MSG_ARRAY_SIZE];
	msg->sender = count[msg->sender];
	msg->recipient = count[msg->recipient];
	msg->subject = count[msg->subject];
	msg->id = count[msg->id];
	if (msg->references) {
	  for (refs = msg->references ; *refs ; refs++) {
		*refs = count[*refs];
	  }
	}
  }
  XP_FREE(count);
}


static int
msg_compress_prepare_new_folder(MWContext* context,
								msg_compress_folder_state* state)
{
  const char* pathname = state->folder->data.mail.pathname;
  XP_File sumfid;
  XP_ASSERT(state->infile == NULL);
  XP_ASSERT(state->outfile == NULL);
  XP_ASSERT(state->tmpname == NULL);
  XP_ASSERT(state->msgs == NULL);
  XP_ASSERT(state->parsestate == NULL);
  XP_ASSERT(!state->needwritesummary);
  state->msgindex = -1;
  state->num = 0;
  state->msg = NULL;
  if (state->folder == context->msgframe->opened_folder &&
	  context->msgframe->msgs && !msg_folder_mysteriously_changed(context)) {
	state->msgs = context->msgframe->msgs;
  } else {
	if (XP_Stat(pathname, &state->folderst, xpMailFolder)) {
	  return MK_MSG_FOLDER_UNREADABLE;
	}
	sumfid = XP_FileOpen(pathname, xpMailFolderSummary, XP_FILE_READ_BIN);
	if (sumfid) {
	  state->msgs = msg_ReadFolderSummaryFile(context, sumfid, pathname, NULL);
	  XP_FileClose(sumfid);
	  if (state->msgs) {
		if (state->msgs->folderdate != state->folderst.st_mtime ||
			state->msgs->foldersize != state->folderst.st_size) {
		  /* The summary info is out of date.  Remember it so we can suck out
			 the flags after we've parsed the folder.*/
		  state->flags_msgs = state->msgs;
		  state->msgs = NULL;
		}
	  }
	}
	if (state->folder == context->msgframe->opened_folder) {
	  state->needwritesummary = TRUE; /* Something weird is going on, so
										 we'd better make sure to write out
										 all regenerated data. */
	}
  }
  state->infile = XP_FileOpen(pathname, xpMailFolder, XP_FILE_READ_BIN);
  if (!state->infile) return MK_MSG_FOLDER_UNREADABLE;
  if (state->msgs == NULL) {
	state->parsestate = msg_BeginParsingFolder(context, NULL, 0);
	if (!state->parsestate) return MK_OUT_OF_MEMORY;
  }
  state->writestate.position = 0;
  state->writestate.update_msg = TRUE;
  return 0;
}


static MSG_Folder*
msg_find_folder_after_1(MSG_Folder* folder, MSG_Folder* search,
						XP_Bool* doit)
{
  MSG_Folder* result;
  for ( ; folder ; folder = folder->next) {
	if (*doit && !(folder->flags & MSG_FOLDER_FLAG_DIRECTORY)) return folder;
	if (folder == search) *doit = TRUE;
	if (folder->children) {
	  result = msg_find_folder_after_1(folder->children, search, doit);
	  if (result) return result;
	}
  }
  return NULL;
}

static MSG_Folder* 
msg_find_folder_after(MSG_Folder* folder, MSG_Folder* search)
{
  XP_Bool doit = FALSE;
  return msg_find_folder_after_1(folder, search, &doit);
}


int
MSG_BeginCompressFolder(MWContext* context, URL_Struct* url,
						const char* foldername, void** closure)
{
  msg_compress_folder_state* state;
  int status;
  if (!context->type || !context->msgframe) {
	XP_ASSERT(0);
	return MK_CONNECTED;		/* ### Should really be some error code. */
  }
  state = XP_NEW_ZAP(msg_compress_folder_state);
  if (!state) return MK_OUT_OF_MEMORY;
  if (foldername && *foldername) {
	state->folder = msg_FindMailFolder(context, foldername, FALSE);
	if (state->folder->flags & MSG_FOLDER_FLAG_DIRECTORY) {
	  state->folder = NULL;		/* Uh, this is not a folder we can use! */
	}
  } else {
	state->folder = context->msgframe->folders;
	if (state->folder->flags & MSG_FOLDER_FLAG_DIRECTORY) {
	  /* The first folder is a directory; find the first non-directory
		 folder. */
	  state->folder = msg_find_folder_after(context->msgframe->folders,
											state->folder);
	}
	state->doall = TRUE;
  }
  if (!state->folder) {
	status = MK_CONNECTED;	/* ### Should really be some error code. */
	goto FAIL;
  }
  state->obuffer_size = 10240 * 2;
  while (!state->obuffer) {
	state->obuffer_size /= 2;
	state->obuffer = (char *) XP_ALLOC (state->obuffer_size);
	if (!state->obuffer && state->obuffer_size < 2) {
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
  }

  status = msg_compress_prepare_new_folder(context, state);

  if (status < 0) goto FAIL;

  *closure = state;
  return MK_WAITING_FOR_CONNECTION;

FAIL:
  XP_FREE(state);
  return status;
}


static void
msg_reopen_folder(URL_Struct *url, int status, MWContext *context)
{
  msg_SelectAndOpenFolder(context, context->msgframe->opened_folder,
						  msg_ScrolltoFirstUnseen);
}


int
MSG_FinishCompressFolder(MWContext* context, URL_Struct* url,
						 const char* foldername, void* closure)
{
  msg_compress_folder_state* state = (msg_compress_folder_state*) closure;
  const char* pathname = state->folder->data.mail.pathname;
  int status;
  XP_File sumfid;
  int32 length;
  int i;
  if (state->parsestate) {
	/* We're in the midst of parsing the folder.  Do a chunk. */
	status = XP_FileRead(state->obuffer, state->obuffer_size, state->infile);
	if (status < 0) return status;
	if (status == 0) {
	  if (state->ibuffer_fp > 0) {
		/* Flush out the last line. */
		msg_ParseFolderLine(state->ibuffer, state->ibuffer_fp,
							state->parsestate);
		state->ibuffer_fp = 0;
	  }
	  state->msgs = msg_DoneParsingFolder(state->parsestate,
										  MSG_SortByMessageNumber, TRUE,
										  FALSE, FALSE);
	  state->parsestate = NULL;
	  if (!state->msgs) return MK_OUT_OF_MEMORY; /* ### */
	  state->needwritesummary = TRUE;
	  if (state->flags_msgs) {
		status = msg_RescueOldFlags(context, state->flags_msgs, state->msgs);
		msg_FreeFolderData(state->flags_msgs);
		state->flags_msgs = NULL;
		if (status < 0) return status;
	  }
	} else {
	  status = msg_LineBuffer(state->obuffer, status,
							  &state->ibuffer, &state->ibuffer_size,
							  &state->ibuffer_fp, FALSE,
							  msg_ParseFolderLine, state->parsestate);
	  if (status < 0) return status;
	}
	return MK_WAITING_FOR_CONNECTION;
  }
  XP_ASSERT(state->msgs);
  if (!state->msgs) return -1;	/* ### */

  if (state->msg == NULL) {
	/* Need to find the next message. */
	state->msgindex++;

	if (state->msgindex == 0) {
	  /* First message.  Now's our first chance to look in the msgs structure,
		 and see if we have to compress this folder at all. */
	  if (state->msgs->expungedmsgs == NULL) {
		/* Nope, no need to do any work here. */
		state->msgindex = state->num = state->msgs->message_count;
	  } else {
		/* Yep.  Print a message, and open up the output file. */

		const char *fmt = XP_GetString(MK_MSG_COMPRESS_FOLDER_ETC); /* #### i18n */
		const char *f = pathname;
		char *s;
		if ((s = XP_STRRCHR (f, '/')))
		  f = s+1;
		s = PR_smprintf (fmt, f);
		if (s)
		  {
			FE_Progress(context, s);
			XP_FREE(s);
		  }

		
		XP_ASSERT(state->msgs->message_count > 0);
		if (state->msgs->message_count <= 0) return -1;

		state->graphtotal =
		  state->msgs->foldersize - state->msgs->expunged_bytes;
		if (state->graphtotal < 1) {
		  /* Pure paranoia. */
		  state->graphtotal = 1;
		}
		state->graphcur = 0;
		FE_SetProgressBarPercent(context, 0);


	    state->tmpname = FE_GetTempFileFor(context, pathname, xpMailFolder,
										   &state->tmptype);
		if (!state->tmpname) return MK_OUT_OF_MEMORY;	/* ### */
		state->outfile = XP_FileOpen(state->tmpname, xpMailFolder,
									 XP_FILE_WRITE_BIN);
		if (!state->outfile) return MK_UNABLE_TO_OPEN_FILE;
		state->writestate.fid = state->outfile;
		state->needwritesummary = TRUE;

		/* #### We should check here that the folder is writable, and warn.
		   (open it for append, then close it right away?)  Just checking
		   S_IWUSR is not enough.
		 */

#ifdef XP_UNIX
		/* Clone the permissions of the "real" folder file into the temp file,
		   so that when we rename the finished temp file to the real file, the
		   preferences don't appear to change. */
		{
		  XP_StatStruct st;
		  if (XP_Stat (pathname, &st, xpMailFolder) == 0)
			/* Ignore errors; if it fails, bummer. */
			fchmod (fileno(state->outfile), (st.st_mode | S_IRUSR | S_IWUSR));
		}
#endif /* XP_UNIX */
	  }
	}
	if (state->msgindex >= state->msgs->message_count) {
	  /* We've finished up this folder. */
	  XP_FileClose(state->infile);
	  state->infile = NULL;
	  if (state->outfile) {
		if (XP_FileClose(state->outfile) != 0) {
		  state->outfile = NULL;
		  return MK_MSG_ERROR_WRITING_MAIL_FOLDER;
		}
		state->outfile = NULL;
	  }
	  if (state->tmpname) {
		XP_FileRename(state->tmpname, state->tmptype, pathname, xpMailFolder);
		XP_FREE(state->tmpname);
		state->tmpname = NULL;
	  }
	  while (state->msgs->expungedmsgs) {
		MSG_ThreadEntry* msg = state->msgs->expungedmsgs->next;
		FREEIF(state->msgs->expungedmsgs->references);
		XP_FreeStruct(&state->msgs->msg_blocks, state->msgs->expungedmsgs);
		state->msgs->expungedmsgs = msg;
	  }
	  for (i=state->num/mail_MSG_ARRAY_SIZE + 1;
		   i<state->msgs->message_count/mail_MSG_ARRAY_SIZE + 1;
		   i++) {
		XP_FREE(state->msgs->msglist[i]);
	  }
	  state->msgs->message_count = state->num;

	  if (state->needwritesummary) {
		if (XP_Stat(pathname, &state->folderst, xpMailFolder)) {
		  return MK_MSG_FOLDER_UNREADABLE;
		}
		sumfid = XP_FileOpen(pathname, xpMailFolderSummary, XP_FILE_WRITE_BIN);
		if (!sumfid) {
		  return MK_MSG_FOLDER_SUMMARY_UNREADABLE;
		}

#ifdef XP_UNIX
		/* Clone the permissions of the folder file into the summary file.
		   But make sure it's readable/writable by owner.
		   Ignore errors; if it fails, bummer. */
		fchmod (fileno(sumfid), (state->folderst.st_mode | S_IRUSR | S_IWUSR));
#endif /* XP_UNIX */

		msg_compress_string_table(context, state->msgs);
		state->msgs->folderdate = state->folderst.st_mtime;
		state->msgs->foldersize = state->folderst.st_size;
		msg_WriteFolderSummaryFile(state->msgs, sumfid);
		XP_FileClose(sumfid);
		state->folder->unread_messages = state->msgs->unread_messages;
		state->folder->total_messages = state->msgs->total_messages;
		msg_RedisplayOneFolder(context, state->folder, -1, -1, FALSE);
		state->needwritesummary = FALSE;
	  }
	  if (state->msgs != context->msgframe->msgs) {
		msg_FreeFolderData(state->msgs);
		if (state->folder == context->msgframe->opened_folder) {
		  /* Hmm.  We must have earlier detected that the folder mysteriously
			 changed.  We'd better arrange to load up the message data we just
			 generated. */
		  url->pre_exit_fn = msg_reopen_folder;
		}
	  }
	  state->msgs = NULL;

	  /* If we don't print a "done" message, then it still says
		 "Compressing folder foo..." long after we've finished with it,
		 while we scan over all those other folders that don't need to
		 be compressed. */
	  {
		const char *fmt = XP_GetString(MK_MSG_COMPRESS_FOLDER_DONE); /* #### i18n */
		const char *f = pathname;
		char *s;
		if ((s = XP_STRRCHR (f, '/')))
		  f = s+1;
		s = PR_smprintf (fmt, f);
		if (s)
		  {
			FE_Progress(context, s);
			XP_FREE(s);
		  }
	  }

	  if (!state->doall) return MK_CONNECTED;
	  state->folder = msg_find_folder_after(context->msgframe->folders,
											state->folder);
	  if (!state->folder) return MK_CONNECTED;
	  msg_compress_prepare_new_folder(context, state);
	  return MK_WAITING_FOR_CONNECTION;
	}
	
	state->msg = state->msgs->msglist[state->msgindex/mail_MSG_ARRAY_SIZE]
	  [state->msgindex%mail_MSG_ARRAY_SIZE];
	if (state->msg->flags & MSG_FLAG_EXPUNGED) {
	  /* Skip this message; it's garbage that we need to compress away. */
	  state->msg = NULL;
	  return MK_WAITING_FOR_CONNECTION;
	}
	state->msgs->msglist[state->num/mail_MSG_ARRAY_SIZE]
	  [state->num%mail_MSG_ARRAY_SIZE] = state->msg;
	state->num++;
	if (state->position != state->msg->data.mail.file_index) {
	  state->position = state->msg->data.mail.file_index;
	  XP_FileSeek(state->infile, state->position, SEEK_SET);
	}
	state->oldstart = state->msg->data.mail.file_index;
	state->msg->data.mail.file_index = state->writestate.position;
	state->writestate.msg = state->msg;
	state->writestate.inhead = TRUE;
#ifdef EMIT_CONTENT_LENGTH
	state->writestate.header_bytes = 0;
#endif /* EMIT_CONTENT_LENGTH */
  }
  length = state->oldstart + state->msg->data.mail.byte_length
	- state->position;
  XP_ASSERT(length > 0);
  if (length <= 0) return -1;
  if (length > state->obuffer_size) length = state->obuffer_size;
  status = XP_FileRead(state->obuffer, length, state->infile);
  if (status < 0) return status;
  if (status == 0) return MK_MSG_FOLDER_UNREADABLE;
  state->position += status;
  state->graphcur += status;
  XP_ASSERT(state->graphtotal > 0);
  if (state->graphtotal > 0) {
	FE_SetProgressBarPercent(context,
							 state->graphcur * 100 / state->graphtotal);
  }
  status = msg_LineBuffer(state->obuffer, status, &state->ibuffer,
						  &state->ibuffer_size, &state->ibuffer_fp, FALSE,
						  msg_writemsg_handle_line, &state->writestate);
  if (status < 0) return status;
  if (state->position >= state->oldstart + state->msg->data.mail.byte_length) {
	/* Finished writing this message. */
	XP_ASSERT(state->position ==
			  state->oldstart + state->msg->data.mail.byte_length);
	if (state->ibuffer_fp > 0) {
	  /* Damn damn damn.  We didn't flush out the last line.  Almost certainly
	     this means we're on a mac, and msg_LineBuffer didn't flush out a
		 lone CR.  Anyway, flush it out now. */
	  msg_writemsg_handle_line(state->ibuffer, state->ibuffer_fp,
							   &state->writestate);
	  state->ibuffer_fp = 0;
	}
	state->msg->data.mail.byte_length =
	  state->writestate.position - state->msg->data.mail.file_index;
	state->msg = NULL;
  }
  return MK_WAITING_FOR_CONNECTION;
}


int
MSG_CloseCompressFolderSock(MWContext* context, URL_Struct* url,
							void* closure)
{
  msg_compress_folder_state* state = (msg_compress_folder_state*) closure;
  XP_File sumfid = 0;
  const char* pathname;
  if (!state) return 0;

  /* If there's still stuff in the buffer (meaning the last line had no
	 newline) push it out. */
  if (state->ibuffer_fp)
	msg_writemsg_handle_line (state->ibuffer, state->ibuffer_fp,
							  &state->writestate);

  if (state->infile) XP_FileClose(state->infile);
  if (state->outfile) XP_FileClose(state->outfile);
  if (state->tmpname) {
	XP_FileRemove(state->tmpname, state->tmptype);
	XP_FREE(state->tmpname);
  }
  FREEIF(state->obuffer);
  FREEIF(state->ibuffer);
  if (state->msgs) {
	msg_ClearMessageArea(context);
	msg_FreeFolderData(state->msgs);
	if (state->msgs == context->msgframe->msgs) {
	  context->msgframe->msgs = NULL;
	  /* Hoo, boy, we got canceled while compressing the current folder.
	     We'd better reload the summary file, if we can.  If we can't, then
		 just punt and close this folder. */
	  if (context->msgframe->opened_folder) {
		pathname = context->msgframe->opened_folder->data.mail.pathname;
	  }
	  if (pathname) {
		sumfid = XP_FileOpen(pathname, xpMailFolderSummary, XP_FILE_READ_BIN);
	  }
	  if (sumfid) {
		context->msgframe->msgs = msg_ReadFolderSummaryFile(context, sumfid,
															pathname, NULL);
		XP_FileClose(sumfid);
	  }
	  if (context->msgframe->msgs) {
		msg_ReSort(context);
	  } else {
		msg_ClearThreadList(context);
		context->msgframe->opened_folder = NULL;
	  }
	}
  }
  XP_FREE(state);
  msg_DisableUpdates(context);	/* Disgusting hack to force the status line */
  msg_EnableUpdates(context);	/* to update. */
  return 0;
}


int
msg_CompressFolder(MWContext* context)
{
  URL_Struct* url;
  char* buf;
  char* pathname;
  msg_undo_Initialize(context);
  msg_SaveMailSummaryChangesNow(context);
  if (!context->msgframe->opened_folder) return 0;
  pathname = context->msgframe->opened_folder->data.mail.pathname;
  buf = PR_smprintf("mailbox:%s?compress-folder", pathname);
  if (!buf) return MK_OUT_OF_MEMORY;
  url = NET_CreateURLStruct(buf, NET_NORMAL_RELOAD);
  XP_FREE(buf);
  if (!url) return MK_OUT_OF_MEMORY;
  url->internal_url = TRUE;
  msg_GetURL(context, url, FALSE);
  return 0;
}


int
msg_CompressAllFolders(MWContext* context)
{
  URL_Struct* url;
  msg_undo_Initialize(context);
  msg_SaveMailSummaryChangesNow(context);
  url = NET_CreateURLStruct("mailbox:?compress-folder", NET_NORMAL_RELOAD);
  if (!url) return MK_OUT_OF_MEMORY;
  url->internal_url = TRUE;
  msg_GetURL(context, url, FALSE);
  return 0;
}



/* Moving and copying messages
 */

typedef struct msg_move_state {
  XP_Bool ismove;
  XP_File infid;
  char* buf;
  uint32 size;  /* size of buf */
  MSG_Folder* destfolder;
  msg_write_state writestate;
  char *ibuffer;
  uint32 ibuffer_size;
  uint32 ibuffer_fp;
  int32 num;
  int32 numunread;
} msg_move_state;


static XP_Bool
msg_move_or_copy_1(MWContext* context, MSG_ThreadEntry* msg, void* closure)
{
  int32 length;
  msg_move_state* state = (msg_move_state*) closure;
  MSG_ThreadEntry msgcopy;
  int32 msgStartPosition = state->writestate.position;

  XP_FileSeek(state->infid, msg->data.mail.file_index, SEEK_SET);
  length = msg->data.mail.byte_length;

  /* We need to make a copy of the message structure, so that
     msg_writemsg_handle_line has something to look in and muck with.  We can't
     give it the original, because the original represents the message in the
     original folder, and that isn't being changed here. */

  XP_MEMCPY(&msgcopy, msg,
			((uint8 *) &msgcopy.data.mail.sleazy_end_of_data -
			 (uint8 *) &msgcopy));

  msgcopy.data.mail.file_index = state->writestate.position;

#ifdef EMIT_CONTENT_LENGTH
  state->writestate.header_bytes = 0;
#endif /* EMIT_CONTENT_LENGTH */
  state->writestate.inhead = TRUE;
  state->writestate.msg = &msgcopy;
  state->writestate.update_msg = FALSE;
  while (length > 0) {
	int32 l = XP_FileRead(state->buf,
						(length > state->size) ? state->size : length,
						state->infid);
	if (l <= 0) break;
	length -= l;
	l = msg_LineBuffer(state->buf, l, &(state->ibuffer),
					   &(state->ibuffer_size), &(state->ibuffer_fp),
					   FALSE, msg_writemsg_handle_line, &(state->writestate));
	if (l < 0) {
	  return FALSE;
	}
  }

  /* If there's still stuff in the buffer (meaning the last line had no
	 newline) push it out. */
  if (state->ibuffer_fp > 0) {
	if (msg_writemsg_handle_line(state->ibuffer, state->ibuffer_fp,
								 &(state->writestate)) < 0) {
	  return FALSE;
	}
  }

  if (state->destfolder) {
	msg_undo_LogFlagChange(context, state->destfolder,
						   msgStartPosition, MSG_FLAG_EXPUNGED, 0);
  }

  state->num++;
  if ((msg->flags & MSG_FLAG_READ) == 0) state->numunread++;
  return TRUE;
}


static XP_Bool
msg_move_or_copy_2(MWContext* context, MSG_ThreadEntry* msg, void* closure)
{
  msg_ChangeFlag(context, msg, MSG_FLAG_EXPUNGED, 0);
  return TRUE;
}


static int
msg_move_or_copy(MWContext *context, const char *file_name, XP_Bool ismove)
{
  int status = 0;
  msg_move_state state;
  XP_StatStruct folderst;
  MSG_ThreadEntry* msg;
  XP_Bool destsummaryvalid = FALSE;
  int32 startDestFolderLength = 0;

  XP_ASSERT (file_name);

  /* Make sure we're not trying to move messages into the folder from
	 which they came. */

  if (XP_FILENAMECMP(file_name,
				context->msgframe->opened_folder->data.mail.pathname) == 0) {
	/* This test is not sufficient in the face of symbolic links.  However,
	   the call at the end to msg_folder_mysteriously_changed() will
	   make things work OK if the user does something silly like that. */
	return MK_MSG_CANT_COPY_TO_SAME_FOLDER;
  }

  if (ismove)
	msg_ClearMessageArea(context);

  XP_MEMSET(&state, 0, sizeof(state));

  if (context->msgframe->num_selected_msgs <= 0) return 0;

  state.destfolder = msg_FindMailFolder(context, file_name, TRUE);
  if (!state.destfolder) return MK_OUT_OF_MEMORY;

  XP_ASSERT ((state.destfolder->flags & MSG_FOLDER_FLAG_MAIL) &&
			 !(state.destfolder->flags & MSG_FOLDER_FLAG_DIRECTORY));

  if (state.destfolder->flags & (MSG_FOLDER_FLAG_QUEUE
								 /* | MSG_FOLDER_FLAG_DRAFTS */))
	{
/*	  if (state.destfolder->flags & MSG_FOLDER_FLAG_QUEUE) */
		return MK_MSG_CANT_COPY_TO_QUEUE_FOLDER;
/*	  else
		return MK_MSG_CANT_COPY_TO_DRAFTS_FOLDER; */
	}

  if (!XP_Stat((char*) file_name, &folderst, xpMailFolder)) {
	state.writestate.position = startDestFolderLength = folderst.st_size;
	destsummaryvalid = msg_IsSummaryValid(file_name, &folderst);
  }

  if (!msg_ConfirmMailFile (context, file_name)) {
	/* The user has hit "cancel" in the dialog asking if he wants to write to
	   a non-mail file.  We don't want to return an error code, because we
	   don't want to pop up an error message; hitting "cancel" was enough
	   user interaction. */
	return 0;
  }

  msg_undo_StartBatch(context);

  state.ismove = ismove;
  state.writestate.fid = XP_FileOpen(file_name, xpMailFolder,
									 XP_FILE_APPEND_BIN);
  if (!state.writestate.fid) {
	status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	goto FAIL;
  }

  state.infid = XP_FileOpen(
						context->msgframe->opened_folder->data.mail.pathname,
							xpMailFolder, XP_FILE_READ_BIN);
  if (!state.infid) {
	status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	goto FAIL;
  }

  state.size = 512 * 2;
  do {
	state.size /= 2;
	state.buf = (char*)XP_ALLOC(state.size);
  } while (state.buf == NULL && state.size > 0);
  if (!state.buf) {
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
  }
  state.writestate.uidl = NULL;

  if (!msg_IterateOverSelectedMessages(context, msg_move_or_copy_1, &state)) {
	status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	goto FAIL;
  }

  /* Must close the file first so that msg_SetSummaryValid can correctly
	 stat it. */
  if (XP_FileClose(state.writestate.fid) != 0) {
	if (status >= 0) status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	state.writestate.fid = NULL;
	goto FAIL;
  } else {
	if (destsummaryvalid) {
	  msg_SetSummaryValid(file_name, state.num, state.numunread);
	}
  }
  state.writestate.fid = NULL;

  if (ismove) {
	msg = msg_FindNextUnselectedMessage(context);
	msg_IterateOverSelectedMessages(context, msg_move_or_copy_2, &state);
	msg_OpenMessage(context, msg);
	context->msgframe->focusMsg = msg;
  }

 FAIL:
  FREEIF(state.buf);
  FREEIF(state.ibuffer);
  if (state.infid) XP_FileClose(state.infid);
  if (state.writestate.fid) {
	if (XP_FileClose(state.writestate.fid) != 0) {
	  if (status >= 0) status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	}
  }
  if (status < 0) {
	/* We failed somewhere; make sure we didn't leave any dribbles in the
	   destination folder by truncating it back to its original length. */
	XP_FileTruncate(file_name, xpMailFolder, startDestFolderLength);
  }
  msg_undo_EndBatch(context);
  if (msg_folder_mysteriously_changed(context)) {
	msg_SelectAndOpenFolder(context, context->msgframe->opened_folder,
							msg_ScrolltoFirstUnseen);
	return 0;
  }
  return status;
}

/* Writes the selected mail messages to a mail folder.
   The context must be a mail context. */
int
msg_MoveOrCopySelectedMailMessages (MWContext *context, const char *file_name,
									XP_Bool ismove)
{
  return msg_move_or_copy(context, file_name, ismove);
}

void
MSG_MoveSelectedMessagesInto (MWContext *context, const char *folder_name)
{
  int status;
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail) return;
  XP_ASSERT (folder_name);
  if (!folder_name) return;
  msg_InterruptContext(context, FALSE);
  status = msg_move_or_copy(context, folder_name, TRUE);

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


/* Auto-filing of messages
 */

void
msg_FreeSortInfo(MWContext* context)
{
  msg_SortInfo* tmp;
  while (context->msgframe->data.mail.sort) {
	tmp = context->msgframe->data.mail.sort;
	context->msgframe->data.mail.sort = tmp->next;
	XP_FREE(tmp->pathname);
	XP_FREE(tmp->header);
	XP_FREE(tmp->match);
	XP_FREE(tmp);
  }
}


static void
msg_reload_sort_info(MWContext* context) 
{
  XP_StatStruct st;
  XP_File fid;
  msg_SortInfo* last = NULL;
  if (XP_Stat("", &st, xpMailSort)) {
	msg_FreeSortInfo(context);
	return;
  }
  if (st.st_mtime == context->msgframe->data.mail.sortfile_date &&
	  st.st_size == context->msgframe->data.mail.sortfile_size) {
	return;
  }
  msg_FreeSortInfo(context);
  fid = XP_FileOpen("", xpMailSort, XP_FILE_READ);
  if (fid) {
	char* buf = (char *) XP_ALLOC(1024);
	context->msgframe->data.mail.sortfile_date = st.st_mtime;
	context->msgframe->data.mail.sortfile_size = st.st_size;
	if (buf) {
	  while (XP_FileReadLine(buf, 1024, fid)) {
		char* ptr = buf;
		char* pathname;
		char* header;
		char* match;
		msg_SortInfo* tmp;
		while (*ptr == ' ' || *ptr == '\t') ptr++;
		if (*ptr == CR || *ptr == LF || *ptr == '#' || *ptr == '\0') continue;
		/* handle quoted directories for directories with spaces in the names */
		if (*ptr == '"')
		{
			pathname = XP_STRTOK(ptr, "\"" LINEBREAK);
			if (!pathname)
				continue;
		}
		else
		{
			pathname = XP_STRTOK(ptr, "\t " LINEBREAK);
		}
		if (!pathname) continue;
		header = XP_STRTOK(NULL, "\t " LINEBREAK);
		if (!header) continue;
		match = XP_STRTOK(NULL, LINEBREAK);
		if (!match) continue;
		while (*match == ' ' || *match == '\t') match++;
		ptr = match + XP_STRLEN(match) - 1;
		while (ptr >= match && (*ptr == '\t' || *ptr == ' ')) {
		  *ptr-- = '\0';
		}
		if (ptr < match) continue;
		tmp = XP_NEW_ZAP(msg_SortInfo);
		if (!tmp) break;
		if (XP_FileIsFullPath(pathname))
		{
			tmp->pathname = XP_STRDUP(pathname);
		}
		else
		{
			tmp->pathname = XP_STRDUP(FE_GetFolderDirectory(context));
			if (tmp->pathname[XP_STRLEN(tmp->pathname)-1] != '/')
			  tmp->pathname = StrAllocCat(tmp->pathname, "/");
			tmp->pathname = StrAllocCat(tmp->pathname, pathname);
		}
		tmp->header = XP_STRDUP(header);
		tmp->match = XP_STRDUP(match);
		tmp->next = NULL;
		if (!tmp->pathname || !tmp->header || !tmp->match) {
		  FREEIF(tmp->pathname);
		  FREEIF(tmp->header);
		  FREEIF(tmp->match);
		  XP_FREE(tmp);
		  break;
		}
		if (last) {
		  last->next = tmp;
		} else {
		  context->msgframe->data.mail.sort = tmp;
		}
		last = tmp;
	  }
	  XP_FREE(buf);
	}
	XP_FileClose(fid);
  }
}



/* Incorporating new messages from POP or movemail.
 */

typedef struct {
  MWContext* context;
  MSG_Folder* inbox;
  const char* dest;
  int32 start_length;
  msg_write_state writestate;
  int numdup;
  char *ibuffer;
  uint32 ibuffer_size;
  uint32 ibuffer_fp;
#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
  XP_Bool mangle_from;			/* True if "From " lines need to be subject
								   to the Usual Mangling Conventions.*/
#endif /* MANGLE_INTERNAL_ENVELOPE_LINES */
  char* headers;
  uint32 headers_length;
  uint32 headers_maxlength;
  XP_Bool gathering_headers;
  XP_Bool summary_valid;
  XP_Bool expect_multiple;
  XP_Bool expect_envelope;

  int status;
} msg_incorporate_state;


static int
msg_close_dest_folder(msg_incorporate_state* state)
{
  MWContext* context = state->context;
  int status = 0;
  if (state->dest == NULL) return 0;
  if (state->writestate.fid) {
	if (XP_FileClose(state->writestate.fid) != 0) {
	  status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	  state->status = status;
	}
	state->writestate.fid = 0;
  }

  if (state->writestate.writefunc) {
	state->writestate.writefunc = NULL;
  } else {
	/* Make sure the folder appears in our lists. */
	(void) msg_FindMailFolder(context, state->dest, TRUE);

	if (state->summary_valid) {
	  msg_SetSummaryValid(state->dest, 1, 1);
	}
  }
  state->dest = NULL;
  return status;
}


static int
msg_open_dest_folder(msg_incorporate_state* state)
{
  MWContext* context = state->context;
  int status;
  XP_StatStruct folderst;
  msg_SortInfo* info;
  status = msg_close_dest_folder(state);
  if (status < 0) return status;

  state->dest = state->inbox->data.mail.pathname;
  state->writestate.fid = NULL;

  if (context->msgframe->inc_uidl) {
	/* We're filling in a partial message.  No screwy destinations for us;
	   always go to the current folder. */
	goto FOUNDONE;
  }

  for (info = context->msgframe->data.mail.sort ; info ; info = info->next) {
	char *buf = state->headers;
	char *buf_end = buf + state->headers_length;
	while (buf < buf_end) {
	  char *end;
	  char *next;
	  char *colon = XP_STRCHR (buf, ':');
	  if (!colon) break;
	  end = colon;
	  while (end > buf && (*end == ' ' || *end == '\t'))
		end--;
	  next = colon + 1;
	KEEP_SEARCHING:
	  while (*next && *next != CR && *next != LF) next++;
	  if (next > buf_end - 3) {
		next = buf_end;
		XP_ASSERT(*next == '\0');
	  } else {
		if ((next[1] == CR || next[1] == LF) && next[1] != next[0]) {
		  next++;
		}
		next++;
		if (*next == ' ' || *next == '\t') goto KEEP_SEARCHING;
	  }
	  if (strncasecomp(info->header, buf, end - buf) == 0 &&
		  XP_STRLEN(info->header) == end - buf) {
		XP_Bool found;
		char c = *next;
		*next = '\0';
		found = (XP_STRSTR(colon + 1, info->match) != NULL);
		*next = c;
		if (found) {
		  state->writestate.fid = XP_FileOpen(info->pathname, xpMailFolder,
											  XP_FILE_APPEND_BIN);
		  if (!state->writestate.fid) {
			char* err;
			  /* ### This is wrong and evil.  Can someone help me understand
				 the rightous way to do this?  - Terry */
			  /* #### I sure can't. - jwz. */
			err = PR_smprintf(XP_GetString(MK_MSG_CANT_OPEN), info->pathname);/* ### I18n */
			if (err) {
			  FE_Alert(context, err);
			  XP_FREE(err);
			}
		  } else {
			state->dest = info->pathname;
			goto FOUNDONE;
		  }
		}
		/* jwz: don't break out here!  This cause us to only ever compare
		   the *first* occurence of a header, rather than comparing *all*
		   of them! */
/*		break;*/
	  }
	  buf = next;
	}
  }

FOUNDONE:

  if (!XP_Stat((char*) state->dest, &folderst, xpMailFolder)) {
	state->writestate.position = folderst.st_size;
  } else {
	state->writestate.position = 0;
  }
  state->start_length = state->writestate.position;
  if (state->writestate.fid == NULL) {
	state->writestate.fid = XP_FileOpen(state->dest,
										xpMailFolder, XP_FILE_APPEND_BIN);
  }
  if (!state->writestate.fid) {
	state->status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	return state->status;
  }

#ifdef FIXED_SEPERATORS
  /* #### This loses and I'm not entirely sure why; when you get mail from
	 POP3, new messages don't show up until some other message has been
	 selected, and after a few clicks you get messages about "Didn't find
	 X-Mozilla-Status where expected at position NNNN".  So something is
	 getting out of whack somewhere. */
  if (state->writestate.position > 0)
	{
	  /* We have just opened a mail folder for output (append) and that
		 file existed already.  In this case, write out two newlines right
		 away: the first to close off the last line of the previous message,
		 in case the file ended without a newline, and the second to insert a
		 blank line before the "From " line which we will eventually write,
		 since the file format requires a blank line before "From " except
		 at the first line of the file. */
	  if (XP_FileWrite (LINEBREAK LINEBREAK, LINEBREAK_LEN+LINEBREAK_LEN,
						state->writestate.fid)
		  < LINEBREAK_LEN+LINEBREAK_LEN) {
		state->status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
		return state->status;
	  }

	  /* Increment writestate.position by the data we just wrote, since those
		 are considered part of the previous message, not this one. */
	  state->writestate.position += (LINEBREAK_LEN+LINEBREAK_LEN);
	}
#endif /* FIXED_SEPERATORS */

  /* Now, if we're in the folder that's going to get this message, then set
	 things up so that as we write new messages out to it, we also send the
	 lines to the parsing code to update our message list. */
  if (XP_STRCMP(state->dest,
				context->msgframe->opened_folder->data.mail.pathname) == 0 &&
	  context->msgframe->msgs) {
	if (context->msgframe->incparsestate == NULL) {
	  context->msgframe->incparsestate =
		msg_BeginParsingFolder(context, context->msgframe->msgs,
							   state->writestate.position);
	}
	if (context->msgframe->incparsestate) {
	  state->writestate.writefunc = msg_ParseFolderLine;
	  state->writestate.writeclosure = context->msgframe->incparsestate;
	}
  } else {
	state->summary_valid = msg_IsSummaryValid(state->dest, &folderst);
  }
  return 0;
}


static int32
msg_incorporate_handle_line(char* line, uint32 length, void* closure)
{
  msg_incorporate_state* state = (msg_incorporate_state*) closure;
  char* line2 = NULL;
  int status;

  if (length > 5 && line[0] == 'F' && XP_STRNCMP(line, "From ", 5) == 0)
	{
	  if (
#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
		  !state->mangle_from ||
#endif /* MANGLE_INTERNAL_ENVELOPE_LINES */
		  state->expect_multiple)
		{
		  /* This is the envelope, and we should treat it as such. */
	  	  state->writestate.inhead = TRUE;
#ifdef EMIT_CONTENT_LENGTH
		  state->writestate.header_bytes = 0;
#endif /* EMIT_CONTENT_LENGTH */
		  if (!state->expect_multiple) state->mangle_from = TRUE;
		  state->expect_envelope = FALSE;
		}
#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
	  else
		{
		  /* Uh oh, we got a line back from the POP3 server that began with
			 "From ".  This must be a POP3 server that is not sendmail based
			 (maybe it's MMDF, or maybe it's non-Unix.)  We must follow the
			 Usual Mangling Conventions, since we're losers and are using the
			 BSD mailbox format, and if we let this line get out to the folder
			 like this, we (or other software) won't be able to parse it later.

			 Note: it is correct to mangle all lines beginning with "From ",
			 not just those that look like parsable message delimiters.
			 Though we might cope, other software won't.
		   */
		  line2 = (char *) XP_ALLOC (length + 2);
		  if (!line2) {
			status = MK_OUT_OF_MEMORY;
			goto FAIL;
		  }
		  line2[0] = '>';
		  XP_MEMCPY (line2+1, line, length);
		  line2[length+1] = 0;
		  line = line2;
		  length++;
		}
#endif /* MANGLE_INTERNAL_ENVELOPE_LINES */
	}

  if (state->expect_envelope) {
	/* Oh, bother.  We're doing movemail, we should have received an envelope
	   as the first line, and we didn't.  Fake one. */
	char *sep = msg_GetDummyEnvelope();
	status = msg_incorporate_handle_line(sep, XP_STRLEN(sep), state);
	if (status < 0) return status;
  }

  if (state->writestate.inhead) {
	status = msg_GrowBuffer(state->headers_length + length + 1, sizeof(char),
							1024, &(state->headers),
							&(state->headers_maxlength));
	if (status < 0) goto FAIL;
	XP_MEMCPY(state->headers + state->headers_length, line, length);
	state->headers_length += length;
	if (line[0] == CR || line[0] == LF || length == 0) {
	  char *ibuffer = NULL;
	  uint32 ibuffer_size = 0;
	  uint32 ibuffer_fp = 0;
	  state->headers[state->headers_length] = '\0';
	  status = msg_open_dest_folder(state);
	  if (status < 0) goto FAIL;
	  status = msg_LineBuffer(state->headers, state->headers_length, &ibuffer,
							  &ibuffer_size, &ibuffer_fp, FALSE,
							  msg_writemsg_handle_line, &(state->writestate));
	  if (status >= 0 && state->writestate.inhead) {
		/* Looks like that last blank line didn't make it to
           msg_writemsg_handle_line.  This can happen if msg_LineBuffer isn't
		   quite sure that a line ended yet and is waiting for more data.
		   We'll shove another blank line directly to msg_writemsg_handle_line,
		   which ought to fix things up. */
		XP_ASSERT(ibuffer_fp == 1); /* We do have some data that didn't make
									   it out, right? */
		msg_writemsg_handle_line(line, length, &(state->writestate));
		XP_ASSERT(!state->writestate.inhead); /* It got the blank line this
												 time, right? */
	  }
	  FREEIF(ibuffer);
	  FREEIF(state->headers);
	  state->headers_length = 0;
	  state->headers_maxlength = 0;
	}
  } else {
	status = msg_writemsg_handle_line(line, length, &(state->writestate));
  }
FAIL:
  FREEIF(line2);
  return status;
}


static int32
msg_incorporate_handle_pop_line(char* line, uint32 length, void* closure)
{
  if (length > 0 && line[0] == '.')
	{
	  /* Take off the first dot. */
	  line++;
	  length--;

	  if (length == LINEBREAK_LEN &&
		  !XP_STRNCMP (line, LINEBREAK, LINEBREAK_LEN))
		/* This line contained only a single dot, which means it was
		   the "end of message" marker.  This means this function will
		   not be called again for this message (it better not!  That
		   would mean something wasn't properly quoting lines.)
		 */
		return 0;
	}
  return msg_incorporate_handle_line(line, length, closure);
}


msg_incorporate_state *
msg_begin_incorporate (MWContext *context)
{
  msg_incorporate_state *state;
  XP_StatStruct folderst;
  MSG_Folder* inbox;

  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail)
	return 0;

  if (context->msgframe->inc_uidl) {
	/* We're filling in a partial message.  Treat the current folder as the
       "inbox", since that's the only folder we want to "inc" to now.  */
	inbox = context->msgframe->opened_folder;
  } else {
	inbox = msg_FindFolderOfType(context, MSG_FOLDER_FLAG_INBOX);
  }

  if (!inbox) return 0;

  state = XP_NEW_ZAP (msg_incorporate_state);
  if (!state) return 0;

  state->context = context;
  state->inbox = inbox;

  if (!XP_Stat(inbox->data.mail.pathname, &folderst, xpMailFolder)) {
	if (context->msgframe->first_incorporated_offset == 1) {
	  context->msgframe->first_incorporated_offset = folderst.st_size;
	}
  }

  XP_ASSERT(inbox == context->msgframe->opened_folder);

  return state;
}


int
msg_end_incorporate (msg_incorporate_state *state)
{
  const char* dest = state->dest;
  int status;
  status = msg_close_dest_folder(state);
  if (state->status < 0) status = state->status;

  if (status < 0 && dest) {
	MSG_Frame* f = state->context->msgframe;
	XP_FileTruncate(dest, xpMailFolder, state->start_length);
	if (f->incparsestate) {
	  msg_DoneParsingFolder(f->incparsestate, MSG_SortByMessageNumber,
							TRUE, FALSE, FALSE);
	  f->incparsestate = NULL;
	  f->force_reparse_hack = TRUE;
	  msg_SelectedMailSummaryFileChanged(state->context, NULL);
	  msg_SaveMailSummaryChangesNow(state->context);
	  f->first_incorporated_offset = 2; /* Magic value### */
	}
  }

  FREEIF (state->ibuffer);
  FREEIF (state->writestate.uidl);
  XP_FREE (state);

  return status;
}


XP_Bool
MSG_BeginMailDelivery(MWContext *context)
{
  /* #### Switch to the right folder or something; return false if we
	 can't accept new mail for some reason (other than disk space.)
   */
  return TRUE;
}

#ifndef _USRDLL

void
MSG_AbortMailDelivery (MWContext *context)
{
  /* Nothing to do here - we do it in url->pre_exit_fn */
}

void
MSG_EndMailDelivery (MWContext *context)
{
  /* Nothing to do here - we do it in url->pre_exit_fn */
}


int
MSG_IncorporateComplete (void *closure)
{
  msg_incorporate_state *state = (msg_incorporate_state *) closure;

  /* better have been a full CRLF on that last line... */
  XP_ASSERT(state && !state->ibuffer_fp);

  return msg_end_incorporate (state);
}

int
MSG_IncorporateAbort (void *closure, int status)
{
  msg_incorporate_state *state = (msg_incorporate_state *) closure;

  if (state->status >= 0) state->status = status;
  msg_end_incorporate (state);
  return 0;
}


int
MSG_IncorporateWrite (void *closure, const char *block, int32 length)
{
  msg_incorporate_state *state = (msg_incorporate_state *) closure;
  return msg_LineBuffer (block, length,
						 &state->ibuffer,
						 &state->ibuffer_size,
						 &state->ibuffer_fp,
						 TRUE, msg_incorporate_handle_pop_line, state);
}

/* This prepares us to receive a single new message.
   The message came from a mail server, and we need to make sure it gets
   to the local disk.

   If the `uidl' argument is set, then that means that this message has
   been (intentionally) truncated, because it was too long - we need to
   remember this uidl magic cookie to make it possible to retrieve the
   rest of the message at a later date.

   If we have the whole message, the uidl argument will be NULL.
 */
void *
MSG_IncorporateBegin(FO_Present_Types format_out,
					 char *pop3_uidl,
					 URL_Struct *url,
					 MWContext *context)
{
  char *sep = msg_GetDummyEnvelope();
  msg_incorporate_state *state;
  int status;
  state = msg_begin_incorporate (context);
  if (!state)
	{
	  return 0;
	}

  state->mangle_from = FALSE;
  state->writestate.inhead = FALSE;
  /* Write out a dummy mailbox (From_) line, since POP doesn't give us one. */
  status = msg_incorporate_handle_line (sep, XP_STRLEN (sep), state);
  if (status < 0)
	{
	  XP_FREE (state);
	  return 0;
	}
  XP_ASSERT(state->mangle_from && state->writestate.inhead);
  state->mangle_from = TRUE;
  state->writestate.inhead = TRUE;
  state->writestate.uidl = (pop3_uidl ? XP_STRDUP (pop3_uidl) : 0);

  return state;
}


char *msg_pop3_host = 0;

void
MSG_SetPopHost (const char *host)
{
  char *old = msg_pop3_host;
  char * at = NULL;
  if (host && old && !XP_STRCMP (host, old))
	return;

  /*
  ** If we are called with data like "fred@bedrock.com", then we will
  ** help the user by ignoring the stuff before the "@".  People with
  ** @ signs in their host names will be hosed.  They also can't possibly
  ** be current happy internet users.
  */
  if (host) at = XP_STRCHR(host, '@');
  if (at != NULL)  host = at + 1;
  if (host && !*host)
	host = 0;
  msg_pop3_host = host ? XP_STRDUP (host) : 0;
  FREEIF (old);
#ifdef _USRDLL
    XP_ASSERT(0);
#else
    NET_SetPopPassword (0);
#endif
}


static void
msg_get_new_mail_again(URL_Struct *url, int status, MWContext *context)
{
  if (msg_FindFolderOfType(context, MSG_FOLDER_FLAG_INBOX) ==
	  context->msgframe->opened_folder) {
	/* First make sure we're looking at the first unread message, just in
	   case there is no new mail to show. */
	MSG_ThreadEntry* msg = msg_FindNextMessage(context,
											   MSG_FirstUnreadMessage);
	if (!msg) msg = msg_FindNextMessage(context, MSG_LastMessage);
	msg_RedisplayOneMessage(context, msg, -1, -1, TRUE);

	msg_GetNewMail(context);
  }
}


int
msg_GetNewMail (MWContext *context)
{
  char *url;
  URL_Struct *url_struct;
  char *host = msg_pop3_host;
  MSG_Folder* folder;
  MSG_Frame* f = context->msgframe;
  
  XP_ASSERT (context->type == MWContextMail);
  if (context->type != MWContextMail)
	return -1;

  if (!host || !*host)
  {
#ifdef XP_MAC
	/*FE_EditPreference(PREF_PopHost); someone broke it*/
#endif
	return MK_MSG_NO_POP_HOST;
  }
  if (f->inc_uidl) {
	/* We're finishing loading a partial message.  So, we want to inc this
	   message into the current folder, 'cause that's where the partial is. */
	folder = f->opened_folder;
	if (folder == NULL) return -1;
	if (f->displayed_message == NULL ||
		!(f->displayed_message->flags & MSG_FLAG_PARTIAL)) {
	  return -1;
	}
  } else {
	folder = msg_FindFolderOfType(context, MSG_FOLDER_FLAG_INBOX);
  }
  if (folder != f->opened_folder ||
	  msg_folder_mysteriously_changed(context)) {
	msg_SelectAndOpenFolder(context, folder, msg_get_new_mail_again);
	return 0;
  }
  msg_reload_sort_info(context);
  msg_SaveMailSummaryChangesNow(context);
  url = (char *) XP_ALLOC (XP_STRLEN (host) + 10 +
						   (f->inc_uidl ? XP_STRLEN(f->inc_uidl) + 10 : 0));
  if (!url) return MK_OUT_OF_MEMORY;
  XP_STRCPY (url, "pop3://");
  XP_STRCAT (url, host);
  if (f->inc_uidl) {
	XP_STRCAT(url, "?uidl=");
	XP_STRCAT(url, f->inc_uidl);
  }
  url_struct = NET_CreateURLStruct (url, NET_NORMAL_RELOAD);
  XP_FREE (url);
  if (!url_struct) return MK_OUT_OF_MEMORY;
  url_struct->internal_url = TRUE;
  f->first_incorporated_offset = 1;	/* This turns out to never be a legal
									   value.*/
  url_struct->pre_exit_fn = msg_OpenFirstIncorporated;

  /* If there is a biff context around, interrupt it.  If we're in the
	 middle of doing a biff check, we don't want that to botch up our
	 getting of new mail. */
  msg_InterruptContext(XP_FindContextOfType(context, MWContextBiff), FALSE);

  msg_GetURL (context, url_struct, FALSE);
  return 0;
}



/* This stuff is for replacing `partial' messages with the full version. */

static void
msg_inc_with_uidl(URL_Struct *url, int status, MWContext *context)
{
  msg_GetNewMail(context);
}


void
MSG_PrepareToIncUIDL(MWContext* context, URL_Struct* url, const char* uidl)
{
  if (context->type == MWContextMail && context->msgframe) {
	FREEIF(context->msgframe->inc_uidl);
	context->msgframe->inc_uidl = XP_STRDUP(uidl);
	if (context->msgframe->inc_uidl) {
	  url->pre_exit_fn = msg_inc_with_uidl;
	}
  }
}



#ifdef XP_UNIX
static void
msg_incorporate_from_file_again(URL_Struct *url, int status,
								MWContext *context)
{
  if (msg_FindFolderOfType(context, MSG_FOLDER_FLAG_INBOX) ==
	  context->msgframe->opened_folder) {
	if (context->msgframe->inc_fid) {
	  /* First make sure we're looking at the first unread message, just in
		 case there is no new mail to show. */
	  MSG_ThreadEntry* msg = msg_FindNextMessage(context,
												 MSG_FirstUnreadMessage);
	  if (!msg) msg = msg_FindNextMessage(context, MSG_LastMessage);
	  msg_RedisplayOneMessage(context, msg, -1, -1, TRUE);

	  MSG_IncorporateFromFile(context, context->msgframe->inc_fid,
							  context->msgframe->inc_donefunc,
							  context->msgframe->inc_doneclosure);
	}
	context->msgframe->inc_fid = NULL;
	context->msgframe->inc_donefunc = NULL;
	context->msgframe->inc_doneclosure = NULL;
  }
}

void
MSG_IncorporateFromFile(MWContext* context, XP_File infid,
						void (*donefunc)(void*, XP_Bool),
						void* doneclosure)
{
  msg_incorporate_state *state;
  int32 size;
  char* buf;
  int32 l;
  MSG_Folder* folder;
  int status = 0;
  msg_InterruptContext(context, FALSE);
  folder = msg_FindFolderOfType(context, MSG_FOLDER_FLAG_INBOX);
  if (folder != context->msgframe->opened_folder ||
	  msg_folder_mysteriously_changed(context)) {
	context->msgframe->inc_fid = infid;
	context->msgframe->inc_donefunc = donefunc;
	context->msgframe->inc_doneclosure = doneclosure;
  	msg_SelectAndOpenFolder(context, folder, msg_incorporate_from_file_again);
	return;
  }


  msg_reload_sort_info(context);
  context->msgframe->first_incorporated_offset = 1;	/* This turns out to
													   never be a legal
													   value.*/

  msg_SaveMailSummaryChangesNow(context);

  state = msg_begin_incorporate (context);
  state->expect_multiple = TRUE;
  state->writestate.inhead = TRUE;
  state->expect_envelope = TRUE;

  size = 512 * 2;
  do {
	size /= 2;
	buf = (char*)XP_ALLOC(size);
  } while (buf == NULL && size > 0);
  if (!buf) {
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
  }

  for (;;) {
	l = XP_FileRead(buf, size, infid);
	if (l < 0) {
	  status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	  goto FAIL;
	}
	if (l == 0) break;

	/* Warning: don't call msg_incorporate_stream_write() here, because
	   that uses msg_incorporate_handle_pop_line() instead of
	   msg_incorporate_handle_line(), which will break -- in the pop
	   case, we hack '.' at the beginning of the line, and when
	   incorporating from a file, we don't.
	 */
	l = msg_LineBuffer (buf, l,
						&state->ibuffer,
						&state->ibuffer_size,
						&state->ibuffer_fp,
						TRUE, msg_incorporate_handle_line, state);

	if (l < 0) {
	  status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
	  goto FAIL;
	}
  }

FAIL:
  FREEIF(buf);
  if (status < 0) state->status = status;
  status = msg_end_incorporate (state);
  msg_OpenFirstIncorporated(NULL, 0, context);
  if (status < 0) {
	FE_Alert(context, XP_GetString(status));
  }
  (*donefunc)(doneclosure, status >= 0);
}
#endif /* XP_UNIX */

#endif /* _USRDLL */



/* Emptying the trash - a most magic process.
   It's tricky because if there are "partial" messages in the trash, then
   the act of emptying the trash should cause their counterparts on the
   POP server to be removed as well - which means we need to parse the
   trash folder as we empty it, and make a note of the POP-side objects
   which should be nuked at our next opportunity.
 */

int
msg_EmptyTrash (MWContext *context)
{
  URL_Struct* url;
  msg_undo_Initialize(context);
  msg_SaveMailSummaryChangesNow(context);
  url = NET_CreateURLStruct("mailbox:?empty-trash", NET_NORMAL_RELOAD);
  if (!url) return MK_OUT_OF_MEMORY;
  url->internal_url = TRUE;
  msg_GetURL(context, url, FALSE);
  return 0;
}


typedef struct msg_empty_trash_state {
  MSG_Folder* folder;			/* The trash folder. */
  XP_File infile;				/* The trash folder we're parsing. */
  XP_File outfile;				/* The popstate file. */
  char *obuffer;
  uint32 obuffer_size;

  char *ibuffer;
  uint32 ibuffer_size;
  uint32 ibuffer_fp;

  XP_Bool inhead;
  char* uidl;
  uint32 flags;

  int32 graphcur;
  int32 graphtotal;
} msg_empty_trash_state;

int
MSG_BeginEmptyTrash(MWContext* context, URL_Struct* url, void** closure)
{
  msg_empty_trash_state* state;
  MSG_Folder* folder;
  if (!context->msgframe || context->type != MWContextMail) return -1;
  for (folder = context->msgframe->folders ; folder ; folder = folder->next) {
	if (folder->flags & MSG_FOLDER_FLAG_TRASH) {
	  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_MAIL) &&
				 !(folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
									MSG_FOLDER_FLAG_NEWS_HOST |
									MSG_FOLDER_FLAG_DIRECTORY |
									MSG_FOLDER_FLAG_ELIDED)));
	  state = XP_NEW_ZAP(msg_empty_trash_state);
	  if (!state) return MK_OUT_OF_MEMORY;
	  *closure = state;
	  state->folder = folder;
	  return (MK_WAITING_FOR_CONNECTION);
	}
  }
  /* Gee, no trash folder.  We're done. */
  return MK_CONNECTED;
}


#define UNHEX(C) \
	  ((C >= '0' && C <= '9') ? C - '0' : \
	   ((C >= 'A' && C <= 'F') ? C - 'A' + 10 : \
		((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)))

extern char *msg_pop3_host;

static int32
msg_finish_empty_trash_1(char* line, uint32 length, void* closure)
{
  msg_empty_trash_state* state = (msg_empty_trash_state*) closure;
  int i;
  if (length > 5 && msg_IsEnvelopeLine(line, length)) {
	state->inhead = TRUE;
  } else if (state->inhead && length > 0 && line[0] == 'X') {
	if (XP_STRNCMP(line, X_MOZILLA_STATUS, X_MOZILLA_STATUS_LEN) == 0) {
	  line += X_MOZILLA_STATUS_LEN + 1;
	  while (*line == ' ' || *line == '\t') line++;
	  state->flags = 0;
	  for (i=0 ; i<4 ; i++) {
		state->flags = (state->flags << 4) | UNHEX(*line);
		line++;
	  }
	} else if (XP_STRNCMP(line, X_UIDL, X_UIDL_LEN) == 0) {
	  line += X_UIDL_LEN + 1;
	  length -= X_UIDL_LEN + 1;
	  while (*line == ' ' || *line == '\t') {
		line++;
		length--;
	  }
	  while (length > 0 && (line[length-1] == CR || line[length-1] == LF)) {
		length--;
	  }
	  FREEIF(state->uidl);
	  state->uidl = (char*) XP_ALLOC(length + 1);
	  if (state->uidl) {
		XP_MEMCPY(state->uidl, line, length);
		state->uidl[length] = '\0';
	  }
	}
  } else if (state->inhead && (length == 0 ||
							   line[0] == CR || line[0] == LF)) {
	state->inhead = FALSE;
	if (state->uidl) {
	  if ((state->flags & MSG_FLAG_PARTIAL) &&
		   !(state->flags & MSG_FLAG_EXPUNGED)) {
		/* Note in the popstate file that the next time we try to get mail,
		   we will delete this message that we partially loaded and the user
		   decided to delete without fully loading. */
		if (state->outfile == NULL) {
		  state->outfile = XP_FileOpen("", xpMailPopState, XP_FILE_APPEND_BIN);
		}
		if (state->outfile) {
		  XP_FilePrintf(state->outfile, "*%s %s" LINEBREAK "d %s" LINEBREAK,
						msg_pop3_host, NET_GetPopUsername(), state->uidl);
		}
	  }
	  XP_FREE(state->uidl);
	  state->uidl = NULL;
	  state->flags = 0;
	}
  }
  return 0;
}


int
MSG_FinishEmptyTrash(MWContext* context, URL_Struct* url, void* closure)
{
  msg_empty_trash_state* state = (msg_empty_trash_state*) closure;
  XP_StatStruct folderst;
  mail_FolderData* msgs;
  XP_File file = NULL;
  MSG_ThreadEntry* msg;
  int status;
  if (state->obuffer == NULL) {
	/* First time through. */

	FE_Progress(context, "Mail: Emptying trash..."); /* ### Fix i18n */

	if (NET_GetPopMsgSizeLimit() < 0) {
	  /* No need to do anything fancy.  We can just truncate the folder
		 and be done. */
	  goto ALLDONE;
	}
	if (XP_Stat ((char *) state->folder->data.mail.pathname, &folderst,
				 xpMailFolder)) {
	  /* Can't stat the file; must not have any trash. */
	  goto ALLDONE;
	}

	state->graphtotal = folderst.st_size;
	if (state->graphtotal <= 0) {
	  /* Trash is empty?  All done. */
	  goto ALLDONE;
	}

	/* Optimization: if we have a valid summary file, we can check to
	   see if any messages in it has the PARTIAL flag set.  If none are
	   set, then we can skip parsing through the folder. */

	file = XP_FileOpen(state->folder->data.mail.pathname, xpMailFolderSummary,
					   XP_FILE_READ_BIN);
	if (file) {
	  msgs = msg_ReadFolderSummaryFile(context, file,
									   state->folder->data.mail.pathname,
									   NULL);
	  XP_FileClose(file);
	  file = NULL;
	  if (msgs) {
		XP_Bool must_parse = TRUE;
		if (msgs->folderdate == folderst.st_mtime &&
			msgs->foldersize == folderst.st_size) {
		  must_parse = FALSE;
		  for (msg = msgs->msgs ; msg ; msg = msg->next) {
			if (msg->flags & MSG_FLAG_PARTIAL) {
			  must_parse = TRUE;
			  break;
			}
			XP_ASSERT(msg->first_child == NULL);
		  }
		}
		msg_FreeFolderData(msgs);
		if (!must_parse) goto ALLDONE;
	  }
	}

	/* We have to do things the hard way, and parse the silly trash folder. */

	state->obuffer_size = 10240 * 2;
	while (!state->obuffer) {
	  state->obuffer_size /= 2;
	  state->obuffer = (char *) XP_ALLOC (state->obuffer_size);
	  if (!state->obuffer && state->obuffer_size < 2) {
		status = MK_OUT_OF_MEMORY;
		goto FAIL;
	  }
	}

	state->infile = XP_FileOpen(state->folder->data.mail.pathname,
								xpMailFolder, XP_FILE_READ_BIN);
	if (!state->infile) goto ALLDONE;

	return MK_WAITING_FOR_CONNECTION;
  }

  /* We're in the middle of parsing the folder.  Do a chunk. */
  status = XP_FileRead(state->obuffer, state->obuffer_size, state->infile);

  if (status < 0) {
	goto FAIL;
  } else if (status == 0) {
	/* Finally finished parsing.  Close and clean up. */
	goto ALLDONE;
  } else {
	state->graphcur += status;
	XP_ASSERT(state->graphtotal > 0);
	if (state->graphtotal > 0) {
	  FE_SetProgressBarPercent(context,
							   state->graphcur * 100 / state->graphtotal);
	}
	status = msg_LineBuffer (state->obuffer, status,
							 &state->ibuffer, &state->ibuffer_size,
							 &state->ibuffer_fp, FALSE,
							 msg_finish_empty_trash_1, state);
	if (status < 0) goto FAIL;
  }
  return MK_WAITING_FOR_CONNECTION;


ALLDONE:
  if (state->infile) {
	XP_FileClose(state->infile);
	state->infile = NULL;
  }

  /* Actually truncate the trash file. */
  XP_FileClose(XP_FileOpen(state->folder->data.mail.pathname,
						   xpMailFolder, XP_FILE_WRITE_BIN));

  state->folder->unread_messages = 0;
  state->folder->total_messages = 0;
  msg_RedisplayOneFolder(context, state->folder, -1, -1, FALSE);
  if (state->folder == context->msgframe->opened_folder) {
	/* We've been displaying the trash; better display it again. */
	url->pre_exit_fn = msg_reopen_folder;
  }
  status = MK_CONNECTED;

FAIL:
  return status;
}


int MSG_CloseEmptyTrashSock(MWContext* context, URL_Struct* url,
							void* closure)
{
  msg_empty_trash_state* state = (msg_empty_trash_state*) closure;
  if (!state) return 0;

  /* better have been a full CRLF on that last line... */
  XP_ASSERT(!state->ibuffer_fp);

  FREEIF(state->ibuffer);
  FREEIF(state->obuffer);
  if (state->infile) XP_FileClose(state->infile);
  if (state->outfile) XP_FileClose(state->outfile);
  XP_FREE(state);
  return 0;
}


/* Delivering queued messages - a most magic process.
 */

int
msg_DeliverQueuedMessages (MWContext *context)
{
  URL_Struct* url;
  url = NET_CreateURLStruct("mailbox:?deliver-queued", NET_NORMAL_RELOAD);
  if (!url) return MK_OUT_OF_MEMORY;
  url->internal_url = TRUE;
  msg_GetURL(context, url, FALSE);
  return 0;
}

typedef struct msg_deliver_queued_state {
  MWContext *context;
  char *folder_name;			/* The full path of the queue folder. */
  char *tmp_name;				/* The full path of the tmp file. */
  XP_File infile;				/* The queue file we're parsing. */
  XP_File outfile;				/* A tmp file for each message in turn. */

  char *to, *newsgroups, *fcc;	/* The parsed mail and news recipients */
  char *newshost;				/* Optional host on which the groups reside. */
  uint16 flags;					/* Flags from the X-Mozilla-Status header */
  int status;
  XP_Bool done;

  XP_Bool awaiting_delivery;	/* This is set while we are waiting for
								   another URL to finish - it is used to
								   make intervening calls to
								   MSG_FinishDeliverQueued() to no-op.
								 */
  XP_Bool delivery_failed;		/* Set when delivery of one message failed,
								   but we want to continue on with the next.
								 */
  int delivery_failure_count;	/* So that we can tell whether it's ok to
								   zero the file at the end.
								 */
  int bogus_message_count;		/* If there are messages in the queue file
								   which oughtn't be there, this counts them.
								 */

  int32 unread_messages;	/* As we walk through the folder, we regenerate */
  int32 total_messages;		/* the summary info. */

  XP_Bool inhead;
  uint32 position;
  uint32 headers_position;
  uint32 flags_position;

  char *obuffer;
  uint32 obuffer_size;

  char *ibuffer;
  uint32 ibuffer_size;
  uint32 ibuffer_fp;

  char *headers;
  uint32 headers_size;
  uint32 headers_fp;

  /* Thermo stuff
   */
  int32 msg_number;				/* The ordinal of the message in the file
								   (not counting expunged messages.) */
  int32 folder_size;			/* How big the queue folder is. */
  int32 bytes_read;				/* How many bytes into the file we are. */

} msg_deliver_queued_state;


int
MSG_BeginDeliverQueued(MWContext *context, URL_Struct *url, void **closure)
{
  msg_deliver_queued_state* state;
  XP_ASSERT(context->type == MWContextMail || context->type == MWContextNews);
  if (context->type != MWContextMail && context->type != MWContextNews)
	return -1;
  if (!context->msgframe) return -1;

  state = XP_NEW_ZAP(msg_deliver_queued_state);
  if (!state) return MK_OUT_OF_MEMORY;
  *closure = state;
  state->context = context;
  return (MK_WAITING_FOR_CONNECTION);
}


static int msg_deliver_queued_message_done(msg_deliver_queued_state *);
static int msg_deliver_queued_message_as_news(msg_deliver_queued_state *);
static int msg_deliver_queued_message_as_mail(msg_deliver_queued_state *);
static int msg_deliver_queued_message_do_fcc(msg_deliver_queued_state *);
static void msg_deliver_queued_message_as_news_exit(URL_Struct *url,
													int status,
													MWContext *context);
static void msg_deliver_queued_message_as_mail_exit(URL_Struct *url,
													int status,
													MWContext *context);
static void msg_deliver_queued_error(msg_deliver_queued_state *,
									 const char *error_msg);

int
msg_deliver_queued_message (msg_deliver_queued_state *state)
{
  XP_ASSERT (state->outfile && state->tmp_name);
  if (!state->outfile || !state->tmp_name) return -1;

  XP_FileClose (state->outfile);
  state->outfile = 0;

  if ((! (state->flags & MSG_FLAG_EXPUNGED)) &&
	  (state->flags & MSG_FLAG_QUEUED))
  	{
	  /* If this message is marked to be delivered, and does not have its
		 expunged bit set (meaning it has not yet been successfully delivered)
		 then deliver it now.
  	   */

	  state->total_messages++;
	  if (! (state->flags & MSG_FLAG_READ))
		state->unread_messages++;

#ifdef XP_UNIX
	  state->status = msg_DeliverMessageExternally(state->context,
												   state->tmp_name);
	  if (state->status != 0)
		{
		  if (state->status >= 0)
			state->status = msg_deliver_queued_message_do_fcc (state);
		  else
			msg_deliver_queued_message_done (state);
		  return state->status;
		}
#endif /* XP_UNIX */

	  msg_deliver_queued_message_as_news (state);
	}
  else
	{
	  /* Just to delete the temp file. */
		msg_deliver_queued_message_done (state);
	}

  return state->status;
}


static int
msg_deliver_queued_message_done (msg_deliver_queued_state *state)
{
  int local_status = 0;
  if (state->status >= 0 &&
	  !state->delivery_failed &&
	  state->flags_position > 0 &&
	  (state->flags & MSG_FLAG_QUEUED) &&
	  (! (state->flags & MSG_FLAG_EXPUNGED)))
	{
	  /* If delivery was successful, mark the message as expunged in the file
		 right now - we do this by opening the file a second time, updating it,
		 and closing it, to make sure that this informationn gets flushed to
		 disk as quickly as possible.  If we make it through the entire folder
		 and deliver them all, we will delete the whole file - but if we crash
		 in the middle, it's important that we not re-transmit all the already-
		 sent messages next time.

		 The message should have an X-Mozilla-Status header that is suitable
		 for us overwriting, but if it doesn't, then we will simply not mark
		 it as expunged - and should delivery fail, it will get sent again.
	   */
	  XP_File out;
	  char buf[50];
#if 0
	  XP_Bool summary_valid = FALSE;
	  XP_StatStruct st;
#endif

	  XP_Bool was_read = (state->flags & MSG_FLAG_READ);
	  state->flags |= MSG_FLAG_EXPUNGED;
	  state->flags &= (~MSG_FLAG_QUEUED);
	  /* it's pretty important that this take up the same space...
		 We can't write out a line terminator because at this point,
		 we don't know what the file had in it before.  (Safe to
		 assume LINEBREAK?) */
	  PR_snprintf(buf, sizeof(buf), X_MOZILLA_STATUS ": %04x", state->flags);

#if 0
	  XP_Stat (state->folder_name, &st, xpURL); /* must succeed */
	  summary_valid = msg_IsSummaryValid (state->folder_name, &st);
#endif

	  out = XP_FileOpen (state->folder_name, xpMailFolder,
						 XP_FILE_UPDATE_BIN);
	  if (!out)
		{
		  local_status = state->status = MK_MSG_ERROR_WRITING_MAIL_FOLDER;
		  goto FAIL;
		}
	  XP_FileSeek (out, state->flags_position, SEEK_SET);
	  state->flags_position = 0; /* Just in case we get called again... */
	  state->status = XP_FileWrite (buf, XP_STRLEN(buf), out);
	  XP_FileClose (out);
	  if (state->status < XP_STRLEN(buf)) {
		local_status = state->status = MK_MIME_ERROR_WRITING_FILE;
		goto FAIL;
	  }

#if 0
	  /* Now that we've marked this message as deleted in the file itself,
		 reduce the message count in its summary file, and in the mail
		 window display (if there is one.) */
	  if (summary_valid)
		msg_SetSummaryValid (state->folder_name, -1,
							 (was_read ? -1 : 0));
#else
	  XP_FileRemove (state->folder_name, xpMailFolderSummary);
#endif

	  XP_ASSERT(state->total_messages > 0);
	  if (state->total_messages > 0)
		state->total_messages--;

	  if (!was_read)
		{
		  XP_ASSERT(state->unread_messages > 0);
		  if (state->unread_messages > 0)
			state->unread_messages--;
		}
	}

 FAIL:
  /* Now we can delete the temp file too. */
  if (state->tmp_name)
	{
	  int status2 = XP_FileRemove (state->tmp_name, xpFileToPost);
	  if (local_status >= 0)
		local_status = state->status = status2;
	  XP_FREE (state->tmp_name);
	  state->tmp_name = 0;
	}

#if 1
  /* If there is a summary file, it is now out of date.  Rather than keeping
	 it up to date, just nuke it, and let it be updated again later, if at
	 all. */
  if (state->folder_name)
	XP_FileRemove (state->folder_name, xpMailFolderSummary);
#endif

  state->awaiting_delivery = FALSE;

  if (state->delivery_failed || state->status < 0)
	state->delivery_failure_count++;
  state->delivery_failed = FALSE;

  if (local_status < 0)
	/* If local_status is set, then it's an error that occurred in this
	   routine, not earlier -- so it wasn't a problem with delivery, but
	   rather, with the post-delivery stuff like updating the summary info.
	   (This would actually be a rather large problem, as now the message
	   has been sent, but we were unable to mark it as sent...)
	 */
	{
	  char *alert = NET_ExplainErrorDetails(local_status,0,0,0,0);
	  if (!alert) alert = XP_STRDUP("Unknown error!"); /* ####i18n */
	  if (alert)
		{
		  FE_Alert(state->context, alert);
		  XP_FREE(alert);
		}
	}

  return state->status;
}


static int
msg_deliver_queued_message_as_news (msg_deliver_queued_state *state)
{
  URL_Struct *url;
  char *ng = state->newsgroups;
  char *host_and_port = state->newshost;
  XP_Bool secure_p = FALSE;
  char *buf;
  char *fn = 0;

  if (ng) while (XP_IS_SPACE(*ng)) ng++;

  if (!ng || !*ng)
	return msg_deliver_queued_message_as_mail (state);

  state->awaiting_delivery = TRUE;

  if (host_and_port && *host_and_port)
	{
	  char *slash = XP_STRRCHR (host_and_port, '/');
	  if (slash && !strcasecomp (slash, "/secure"))
		{
		  *slash = 0;
		  secure_p = TRUE;
		}
	}
  if (host_and_port && !*host_and_port)
	host_and_port = 0;

  buf = (char *) XP_ALLOC (30 + (host_and_port ? XP_STRLEN(host_and_port) :0));
  if (!buf) return MK_OUT_OF_MEMORY;
  XP_STRCPY(buf, secure_p ? "snews:" : "news:");
  if (host_and_port)
	{
	  XP_STRCAT (buf, "//");
	  XP_STRCAT (buf, host_and_port);
	}

  fn = XP_STRDUP(state->tmp_name);
  if (!fn)
	{
	  XP_FREE (buf);
	  return MK_OUT_OF_MEMORY;
	}

  url = NET_CreateURLStruct (buf, NET_NORMAL_RELOAD);
  XP_FREE (buf);
  if (!url)
	{
	  XP_FREE (fn);
	  return MK_OUT_OF_MEMORY;
	}

  url->post_data = fn;
  url->post_data_size = XP_STRLEN(fn);
  url->post_data_is_file = TRUE;
  url->method = URL_POST_METHOD;
  url->fe_data = state;
  url->internal_url = TRUE;

  {
	char buf[100];
#ifdef XP_WIN16
        const char *fmt = "Mail: delivering message %ld to %s..."; /* #### i18n */
#else
	const char *fmt = "Mail: delivering message %d to %s..."; /* #### i18n */
#endif
	PR_snprintf(buf, sizeof(buf)-1, fmt, state->msg_number, ng);
	FE_Progress(state->context, buf);
  }

  /* We can ignore the return value of NET_GetURL() because we have
	 handled the error in msg_deliver_queued_message_as_news_exit(). */
  NET_GetURL (url, FO_CACHE_AND_PRESENT, state->context,
			  msg_deliver_queued_message_as_news_exit);
  return 0;
}


static int
msg_deliver_queued_message_as_mail (msg_deliver_queued_state *state)
{
  URL_Struct *url;
  char *to = state->to;
  char *buf;
  char *fn = 0;

  if (to) while (XP_IS_SPACE(*to)) to++;

  if (!to || !*to)
	return msg_deliver_queued_message_do_fcc (state);

  state->awaiting_delivery = TRUE;

  buf = (char *) XP_ALLOC(30 + XP_STRLEN(to));
  if (!buf) return MK_OUT_OF_MEMORY;
  XP_STRCPY(buf, "mailto:");
  XP_STRCAT (buf, to);

  fn = XP_STRDUP(state->tmp_name);
  if (!fn)
	{
	  XP_FREE (buf);
	  return MK_OUT_OF_MEMORY;
	}

  url = NET_CreateURLStruct (buf, NET_NORMAL_RELOAD);
  XP_FREE (buf);
  if (!url)
	{
	  XP_FREE (fn);
	  return MK_OUT_OF_MEMORY;
	}

  url->post_data = fn;
  url->post_data_size = XP_STRLEN(fn);
  url->post_data_is_file = TRUE;
  url->method = URL_POST_METHOD;
  url->fe_data = state;
  url->internal_url = TRUE;

  {
	char buf[100];
#ifdef XP_WIN16
        const char *fmt = "Mail: delivering message %ld to %s..."; /* #### i18n */
#else
	const char *fmt = "Mail: delivering message %d to %s..."; /* #### i18n */
#endif
	PR_snprintf(buf, sizeof(buf)-1, fmt, state->msg_number, to);
	FE_Progress(state->context, buf);
  }

  /* We can ignore the return value of NET_GetURL() because we have
	 handled the error in msg_deliver_queued_message_as_mail_exit(). */
  NET_GetURL (url, FO_CACHE_AND_PRESENT, state->context,
			  msg_deliver_queued_message_as_mail_exit);
  return 0;
}


static void
msg_deliver_queued_message_as_news_exit (URL_Struct *url, int status,
										 MWContext *context)
{
  msg_deliver_queued_state *state = (msg_deliver_queued_state *) url->fe_data;

  url->fe_data = 0;
  if (state->status >= 0)
	state->status = status;

  if (status < 0 && url->error_msg)
	msg_deliver_queued_error (state, url->error_msg);

  NET_FreeURLStruct (url);

  if (state->status >= 0 && !state->delivery_failed)
	state->status = msg_deliver_queued_message_as_mail (state);
  else
	msg_deliver_queued_message_done (state);
}


static void
msg_deliver_queued_message_as_mail_exit (URL_Struct *url, int status,
										 MWContext *context)
{
  msg_deliver_queued_state *state = (msg_deliver_queued_state *) url->fe_data;

  url->fe_data = 0;
  if (state->status >= 0)
	state->status = status;

  if (status < 0 && url->error_msg)
	msg_deliver_queued_error (state, url->error_msg);

  NET_FreeURLStruct (url);

  if (state->status >= 0 && !state->delivery_failed)
	state->status = msg_deliver_queued_message_do_fcc (state);
  else
	msg_deliver_queued_message_done (state);
}


static int
msg_deliver_queued_message_do_fcc (msg_deliver_queued_state *state)
{
  const char *bcc = 0 /* #### state->bcc */;
  const char *fcc = state->fcc;
  if (bcc) while (XP_IS_SPACE(*bcc)) bcc++;
  if (fcc) while (XP_IS_SPACE(*fcc)) fcc++;

  if (fcc && *fcc)
	{
	  int status = msg_DoFCC (state->context,
							  state->tmp_name, xpFileToPost,
							  fcc, xpMailFolder,
							  bcc, fcc);
	  if (status < 0)
		msg_deliver_queued_error (state,
								  /* #### slightly wrong error */
								  XP_GetString (MK_MSG_COULDNT_OPEN_FCC_FILE));
	}

  state->status = msg_deliver_queued_message_done (state);
  return state->status;
}

static void
msg_deliver_queued_error(msg_deliver_queued_state *state,
						 const char *error_msg)
{
  XP_ASSERT(state && error_msg);
  if (!state) return;

  state->delivery_failed = TRUE;

  if (!error_msg)
	{
	  state->done = TRUE;
	}
  else
	{
	  char *s, *s2;
	  int32 L;
	  XP_Bool cont = FALSE;

	  s2 = XP_GetString(MK_MSG_QUEUED_DELIVERY_FAILED);			
			
	  L = XP_STRLEN(error_msg) + XP_STRLEN(s2) + 1;
	  s = XP_ALLOC(L);
	  if (!s)
		{
		  state->done = TRUE;
		  return;
		}
	  PR_snprintf (s, L, s2, error_msg);
	  cont = FE_Confirm (state->context, s);
	  XP_FREE(s);

	  if (cont)
		state->status = 0;
	  else
		state->done = TRUE;
	}
}

/* This function parses the headers, and also deletes from the header block
   any headers which should not be delivered in mail, regardless of whether
   they were present in the queue file.  Such headers include: BCC, FCC,
   Sender, X-Mozilla-Status, X-Mozilla-News-Host, and Content-Length.
   (Content-Length is for the disk file only, and must not be allowed to
   escape onto the network, since it depends on the local linebreak
   representation.  Arguably, we could allow Lines to escape, but it's not
   required by NNTP.)
 */
static int32
msg_deliver_queued_hack_headers (msg_deliver_queued_state *state)
{
  char *buf = state->headers;
  char *buf_end = buf + state->headers_fp;

  FREEIF(state->to);
  FREEIF(state->newsgroups);
  FREEIF(state->newshost);
  state->flags = 0;

  while (buf < buf_end)
	{
	  XP_Bool prune_p = FALSE;
	  XP_Bool do_flags_p = FALSE;
	  char *colon = XP_STRCHR (buf, ':');
	  char *end;
	  char *value = 0;
	  char **header = 0;
	  char *header_start = buf;

	  if (! colon)
		break;

	  end = colon;
	  while (end > buf && (*end == ' ' || *end == '\t'))
		end--;

	  switch (buf [0])
		{
		case 'B': case 'b':
		  if (!strncasecomp ("BCC", buf, end - buf))
			{
			  header = &state->to;
			  prune_p = TRUE;
			}
		  break;
		case 'C': case 'c':
		  if (!strncasecomp ("CC", buf, end - buf))
			header = &state->to;
		  else if (!strncasecomp (CONTENT_LENGTH, buf, end - buf))
			prune_p = TRUE;
		  break;
		case 'F': case 'f':
		  if (!strncasecomp ("FCC", buf, end - buf))
			{
			  header = &state->fcc;
			  prune_p = TRUE;
			}
		  break;
		case 'L': case 'l':
		  if (!strncasecomp ("Lines", buf, end - buf))
			prune_p = TRUE;
		  break;
		case 'N': case 'n':
		  if (!strncasecomp ("Newsgroups", buf, end - buf))
			header = &state->newsgroups;
		  break;
		case 'S': case 's':
		  if (!strncasecomp ("Sender", buf, end - buf))
			prune_p = TRUE;
		  break;
		case 'T': case 't':
		  if (!strncasecomp ("To", buf, end - buf))
			header = &state->to;
		  break;
		case 'X': case 'x':
		  if (!strncasecomp(X_MOZILLA_STATUS, buf, end - buf))
			prune_p = do_flags_p = TRUE;
		  else if (!strncasecomp(X_MOZILLA_NEWSHOST, buf, end - buf))
			{
			  prune_p = TRUE;
			  header = &state->newshost;
			}
		  break;
		}

	  buf = colon + 1;
	  while (*buf == ' ' || *buf == '\t')
		buf++;

	  value = buf;

	SEARCH_NEWLINE:
	  while (*buf != 0 && *buf != CR && *buf != LF)
		buf++;

	  if (buf+1 >= buf_end)
		;
	  /* If "\r\n " or "\r\n\t" is next, that doesn't terminate the header. */
	  else if (buf+2 < buf_end &&
			   (buf[0] == CR  && buf[1] == LF) &&
			   (buf[2] == ' ' || buf[2] == '\t'))
		{
		  buf += 3;
		  goto SEARCH_NEWLINE;
		}
	  /* If "\r " or "\r\t" or "\n " or "\n\t" is next, that doesn't terminate
		 the header either. */
	  else if ((buf[0] == CR  || buf[0] == LF) &&
			   (buf[1] == ' ' || buf[1] == '\t'))
		{
		  buf += 2;
		  goto SEARCH_NEWLINE;
		}

	  if (header)
		{
		  int32 L = buf - value;
		  if (*header)
			{
			  char *newh = (char*) XP_REALLOC ((*header),
											   XP_STRLEN(*header) + L + 10);
			  if (!newh) return MK_OUT_OF_MEMORY;
			  *header = newh;
			  newh = (*header) + XP_STRLEN (*header);
			  *newh++ = ',';
			  *newh++ = ' ';
			  XP_MEMCPY (newh, value, L);
			  newh [L] = 0;
			}
		  else
			{
			  *header = (char *) XP_ALLOC(L+1);
			  if (!*header) return MK_OUT_OF_MEMORY;
			  XP_MEMCPY ((*header), value, L);
			  (*header)[L] = 0;
			}
		}
	  else if (do_flags_p)
		{
		  int i;
		  char *s = value;
		  XP_ASSERT(*s != ' ' && *s != '\t');
		  state->flags = 0;
		  for (i=0 ; i<4 ; i++) {
			state->flags = (state->flags << 4) | UNHEX(*s);
			s++;
		  }
		}

	  if (*buf == CR || *buf == LF)
		{
		  if (*buf == CR && buf[1] == LF)
			buf++;
		  buf++;
		}

	  if (prune_p)
		{
		  char *to = header_start;
		  char *from = buf;
		  while (from < buf_end)
			*to++ = *from++;
		  buf = header_start;
		  buf_end = to;
		  state->headers_fp = buf_end - state->headers;
		}
	}

  /* Print a message if this message isn't already expunged. */
  if ((state->flags & MSG_FLAG_QUEUED) &&
	  (! (state->flags & MSG_FLAG_EXPUNGED)))
	{
	  char buf[100];
	  const char *fmt = "Mail: delivering message %d..."; /* #### i18n */
	  state->msg_number++;
	  PR_snprintf(buf, sizeof(buf)-1, fmt, state->msg_number);
	  FE_Progress(state->context, buf);
	}
  else if ((! (state->flags & MSG_FLAG_QUEUED)) &&
		   (! (state->flags & MSG_FLAG_EXPUNGED)))
	{
	  /* Ugh!  a non-expunged message without its queued bit set!
		 How did that get in here?  Danger, danger. */
	  state->bogus_message_count++;
	}

  /* Now state->headers contains only the headers we should ship.
	 Write them to the file;
   */
  state->headers [state->headers_fp++] = CR;
  state->headers [state->headers_fp++] = LF;
  if (XP_FileWrite (state->headers, state->headers_fp,
					state->outfile) < state->headers_fp) {
	return MK_MIME_ERROR_WRITING_FILE;
  }
  return 0;
}


#define msg_grow_headers(state, desired_size) \
  (((desired_size) >= (state)->headers_size) ? \
   msg_GrowBuffer ((desired_size), sizeof(char), 1024, \
				   &(state)->headers, &(state)->headers_size) \
   : 0)

static int32
msg_deliver_queued_line(char *line, uint32 length, void *closure)
{
  uint32 flength = length;
  msg_deliver_queued_state *state = (msg_deliver_queued_state*) closure;

  state->bytes_read += length;
  FE_SetProgressBarPercent(state->context,
						   ((state->bytes_read * 100)
							/ state->folder_size));

  /* convert existing newline to CRLF */
  if (length > 0 &&
	  (line[length-1] == CR ||
	   (line[length-1] == LF &&
		(length < 2 || line[length-2] != CR))))
	{
	  line[length-1] = CR;
	  line[length++] = LF;
	}

  if (line[0] == 'F' && msg_IsEnvelopeLine(line, length))
	{
	  XP_ASSERT (!state->inhead); /* else folder corrupted */

	  /* Make sure there are no mystery deletions floating around... */
	  {
		MWContext *mc = XP_FindContextOfType(0, MWContextMail);
		if (mc) msg_SaveMailSummaryChangesNow(mc);
	  }

	  if (!state->inhead &&
		  state->headers_fp > 0)
		{
		  state->status = msg_deliver_queued_message (state);
		  if (state->status < 0) return state->status;
		}
	  state->headers_fp = 0;
	  state->headers_position = 0;
	  state->inhead = TRUE;
	}
  else if (state->inhead)
	{
	  if (state->headers_position == 0)
		{
		  /* This line is the first line in a header block.
			 Remember its position. */
		  state->headers_position = state->position;

		  /* Also, since we're now processing the headers, clear out the
			 slots which we will parse data into, so that the values that
			 were used the last time around do not persist.

			 We must do that here, and not in the previous clause of this
			 `else' (the "I've just seen a `From ' line clause") because
			 that clause happens before delivery of the previous message is
			 complete, whereas this clause happens after the previous msg
			 has been delivered.  If we did this up there, then only the
			 last message in the folder would ever be able to be both
			 mailed and posted (or fcc'ed.)
		   */
		  FREEIF(state->to);
		  FREEIF(state->newsgroups);
		  FREEIF(state->newshost);
		  FREEIF(state->fcc);
		}

	  if (line[0] == CR || line[0] == LF || line[0] == 0)
		{
		  /* End of headers.  Now parse them; open the temp file;
			 and write the appropriate subset of the headers out. */
		  state->inhead = FALSE;

		  state->tmp_name = WH_TempName (xpFileToPost, "nsqmail");
		  if (!state->tmp_name)
			return MK_OUT_OF_MEMORY;
		  state->outfile = XP_FileOpen (state->tmp_name, xpFileToPost,
										XP_FILE_WRITE_BIN);
		  if (!state->outfile)
			return MK_MIME_ERROR_WRITING_FILE;

		  state->status = msg_deliver_queued_hack_headers (state);
		  if (state->status < 0) return state->status;
		}
	  else
		{
		  /* Otherwise, this line belongs to a header.  So append it to the
			 header data. */

		  if (!strncasecomp (line, X_MOZILLA_STATUS, X_MOZILLA_STATUS_LEN))
			/* Notice the position of the flags. */
			state->flags_position = state->position;
		  else if (state->headers_fp == 0)
			state->flags_position = 0;

		  state->status = msg_grow_headers (state,
											length + state->headers_fp + 10);
		  if (state->status < 0) return state->status;

		  XP_MEMCPY (state->headers + state->headers_fp, line, length);
		  state->headers_fp += length;
		}
	}
  else
	{
	  /* This is a body line.  Write it to the file. */
	  XP_ASSERT (state->outfile);
	  if (state->outfile)
		{
		  state->status = XP_FileWrite (line, length, state->outfile);
		  if (state->status < length) return MK_MIME_ERROR_WRITING_FILE;
		}
	}

  state->position += flength;
  return 0;
}


int
MSG_FinishDeliverQueued(MWContext *context, URL_Struct *url, void *closure)
{
  msg_deliver_queued_state* state = (msg_deliver_queued_state*) closure;
  XP_StatStruct folderst;
  char *line;

  MSG_Folder *folder = 0;
  MWContext *mc = 0;

  if (state->done)
	goto ALLDONE;

  if (state->awaiting_delivery)
	/* If we're waiting for our post to a "mailto:" or "news:" URL to
	   complete, just return.  We'll be called again after the current
	   batch of stuff is serviced, and eventually this will go FALSE.

	   #### I think this means that, though we will be servicing user
	   events, netlib will be running full-tilt until the message is
	   delivered, calling this function as frequently as it can.
	   That's kind of bad...
	 */
	return MK_WAITING_FOR_CONNECTION;

  if (state->obuffer == NULL) {
	/* First time through. */

	if (!mc)
	  mc = XP_FindContextOfType(0, MWContextMail);

	/* If there is a mail context around, grab the Queue folder out of it. */
	if (mc && !folder)
	  {
		for (folder = mc->msgframe->folders ; folder; folder = folder->next)
		  if (folder->flags & MSG_FOLDER_FLAG_QUEUE) {
			XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_MAIL) &&
					   !(folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
										  MSG_FOLDER_FLAG_NEWS_HOST |
										  MSG_FOLDER_FLAG_DIRECTORY |
										  MSG_FOLDER_FLAG_ELIDED)));
			break;
		  }
	  }

	if (folder)
	  state->folder_name = XP_STRDUP (folder->data.mail.pathname);
	else
	  {
		const char *dir = FE_GetFolderDirectory(context);
		state->folder_name = (char*)XP_ALLOC (XP_STRLEN(dir) + 100);
		if (state->folder_name)
		  {
			char *s;
			XP_STRCPY(state->folder_name, dir);
			s = state->folder_name + XP_STRLEN(state->folder_name);
			if (s[-1] != '/') *s++ = '/';
			XP_STRCPY(s, QUEUE_FOLDER_NAME);
		  }
	  }
	if (!state->folder_name)
	  return MK_OUT_OF_MEMORY;

	if (XP_Stat (state->folder_name, &folderst, xpMailFolder)) {
	  /* Can't stat the file; must not have any queued messages. */
	  goto ALLDONE;
	}

	state->folder_size = folderst.st_size;
	if (state->folder_size)
	  FE_GraphProgressInit(state->context, url, state->folder_size);

	state->obuffer_size = 2048;
	while (!state->obuffer) {
	  state->obuffer_size /= 2;
	  state->obuffer = (char *) XP_ALLOC (state->obuffer_size);
	  if (!state->obuffer && state->obuffer_size < 2) {
		state->status = MK_OUT_OF_MEMORY;
		goto FAIL;
	  }
	}

	state->infile = XP_FileOpen(state->folder_name, xpMailFolder,
								XP_FILE_READ_BIN);
	if (!state->infile) goto ALLDONE;

	return MK_WAITING_FOR_CONNECTION;
  }

  if (state->infile)
	{
	  /* We're in the middle of parsing the folder.  Do a line.
		 (It would be nice to use msg_LineBuffer() here, but because
		 of the goofy flow of control, that won't work.)
	   */
	  line = XP_FileReadLine(state->obuffer, state->obuffer_size,
							 state->infile);
	  if (!line)
		/* Finally finished parsing.  Close and clean up. */
		goto ALLDONE;

	  state->status = msg_deliver_queued_line (line, XP_STRLEN(line), state);
	  if (state->status < 0) goto FAIL;

	  return MK_WAITING_FOR_CONNECTION;
	}

 ALLDONE:
  if (state->infile)
	{
	  XP_FileClose(state->infile);
	  state->infile = NULL;

	  if (!state->inhead &&
		  state->headers_fp > 0 &&
		  state->status >= 0)
		{
		  /* Ah, not really done now, are we? */
		  state->status = msg_deliver_queued_message (state);
		  if (state->awaiting_delivery)
			return MK_WAITING_FOR_CONNECTION;
		}
	}

  /* Actually truncate the queue file if all messages in it have been
	 delivered successfully. */
  if (state->status >= 0 &&
	  state->delivery_failure_count == 0 &&
	  state->bogus_message_count == 0)
	XP_FileClose(XP_FileOpen(state->folder_name, xpMailFolder,
							 XP_FILE_WRITE_BIN));

#if 1
  /* If there is a summary file, it is now out of date.  Rather than keeping
	 it up to date, just nuke it, and let it be updated again later, if at
	 all. */
  XP_FileRemove (state->folder_name, xpMailFolderSummary);
#endif

  if (!mc)
	mc = XP_FindContextOfType(0, MWContextMail);

  /* If there is a mail context around, grab the Queue folder out of it. */
  if (mc && !folder)
	{
	  for (folder = mc->msgframe->folders ; folder; folder = folder->next)
		if (folder->flags & MSG_FOLDER_FLAG_QUEUE) {
		  XP_ASSERT ((folder->flags & MSG_FOLDER_FLAG_MAIL) &&
					 !(folder->flags & (MSG_FOLDER_FLAG_NEWSGROUP |
										MSG_FOLDER_FLAG_NEWS_HOST |
										MSG_FOLDER_FLAG_DIRECTORY |
										MSG_FOLDER_FLAG_ELIDED)));
		  break;
		}
	}

  if (folder)
	{
	  folder->unread_messages = state->unread_messages;
	  folder->total_messages = state->total_messages;
	  msg_RedisplayOneFolder(mc, folder, -1, -1, FALSE);
	  if (folder == mc->msgframe->opened_folder) {
		/* We've been displaying the queue folder; better display it again. */
		url->pre_exit_fn = msg_reopen_folder;
	  }
	}

  state->status = MK_CONNECTED;

  /* Warn about failed delivery.
   */
  if (state->delivery_failure_count > 0)
	{
	  char *fmt, *buf;
	  int32 L;
	  if (state->delivery_failure_count == 1)
		fmt = XP_GetString (MK_MSG_DELIVERY_FAILURE_1);
	  else
		fmt = XP_GetString (MK_MSG_DELIVERY_FAILURE_N);

	  L = XP_STRLEN(fmt) + 30;
	  buf = (char *) XP_ALLOC (L);
	  if (buf)
		{
		  PR_snprintf(buf, L-1, fmt, state->delivery_failure_count);
		  FE_Alert(state->context, buf);
		  XP_FREE(buf);
		}
	}

  /* Warn about bogus messages in the Outbox.
   */
  else if (state->bogus_message_count)
	{
	  char *msg1, *msg2, *msg3;
	  char *buf1, *buf2;

	  if (state->bogus_message_count == 1)
		{
		  buf1 = NULL;
		  msg1 = XP_GetString(MK_MSG_BOGUS_QUEUE_MSG_1);
		}
	  else
		{
		  buf1 = PR_smprintf(XP_GetString(MK_MSG_BOGUS_QUEUE_MSG_N),
							 state->bogus_message_count);
		  msg1 = buf1;
		}

	  msg2 = XP_GetString(MK_MSG_BOGUS_QUEUE_REASON);
	  msg3 = XP_GetString(MK_MSG_WHY_QUEUE_SPECIAL);

	  if (msg1 && msg2 && msg3)
		{
		  buf2 = PR_smprintf("%s%s%s", msg1, msg2, msg3);
		  if (buf2)
			{
			  FE_Alert(state->context, buf2);
			  XP_FREE(buf2);
			}
		}

	  if (buf1)
		XP_FREE(buf1);
	}

FAIL:
  return state->status;
}


int
MSG_CloseDeliverQueuedSock(MWContext *context, URL_Struct *url, void *closure)
{
  msg_deliver_queued_state* state = (msg_deliver_queued_state*) closure;
  if (!state) return 0;
  if (state->infile) XP_FileClose(state->infile);
  if (state->outfile) XP_FileClose(state->outfile);
  FREEIF(state->to);
  FREEIF(state->newsgroups);
  FREEIF(state->newshost);
  FREEIF(state->ibuffer);
  FREEIF(state->obuffer);
  FREEIF(state->headers);
  FREEIF(state->folder_name);
  if (state->tmp_name)
	{
	  XP_FileRemove(state->tmp_name, xpFileToPost);
	  XP_FREE(state->tmp_name);
	}

  if (state->folder_size)
	FE_GraphProgressDestroy(state->context, url,
							state->folder_size, state->bytes_read);

  XP_FREE(state);
  return 0;
}
