/* -*- Mode: C; tab-width: 4 -*-
   mailsum.h --- definitions for the mail summary file format.
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 10-May-95.
 */

#ifndef _MAILSUM_H_
#define _MAILSUM_H_

#include "xp_mcom.h"
#include "xp_core.h"
#include "xp_hash.h"
#include "xp_file.h"
#include "ntypes.h"


/* The summary file format is as follows:

   MAGIC_NUMBER VERSION FOLDER_SIZE FOLDER_DATE PARSED_THRU NMSGS NVALIDMSGS
    NUNREADMSGS EXPUNGED_BYTES STRING_TABLE [ MSG_DESC ]*

   MAGIC_NUMBER   := '# Netscape folder cache\r\n'
   VERSION        := uint32 (file format version number; this is version 4)

   FOLDER_SIZE    := uint32 (the size of the folder itself, so that we can
                     easily detect an out-of-date summary file)
   FOLDER_DATE    := uint32 (time_t of the folder; same purpose as above)
   PARSED_THRU	  := uint32 (the last position in the folder that we have
							 parsed data for.  Normally the same as 
							 FOLDER_SIZE, but if we append messages to a file,
							 we can just update FOLDER_SIZE and FOLDER_SIZE and
							 use this field to tell us what part of the
							 folder needs to be parsed.)
   NMSGS		  := uint32 (total number of messages in this folder, including
   							 expunged messages)
   NVALIDMSGS	  := uint32 (total number of non-expunged messages in this
   							 folder)
   NUNREADMSGS	  := uint32 (total number of unread messages in this folder)
   EXPUNGED_BYTES := uint32 (number of bytes of messages which have been
                     expunged from the summary, but not yet compacted out
                     of the folder file itself.  The messages in question
                     have the EXPUNGED bit set in their flags.)

   STRING_TABLE   := ST_LENGTH [ ST_STRING ]+
   ST_LENGTH      := uint16
   ST_STRING      := null terminated string

   MSG_DESC       := SENDER RECIPIENT SUBJECT DATE ID REFERENCES
                     FLAGS POS LENGTH STATUSINDEX
   SENDER         := uint16 (index into STRING_TABLE)
   RECIPIENT      := uint16 (index into STRING_TABLE)
   SUBJECT        := uint16 (index into STRING_TABLE)
   DATE           := uint32 (time_t)
   FLAGS          := uint32 (bitfield of other attributes of the message)
   POS            := uint32 (byte index into file)
   LENGTH         := uint32 (length in bytes from envelope to end of last line)
   STATUSOFFSET   := uint16 (Offset from beginning of message envelope for
							 X-Mozilla-Status header)
   LINES          := uint16 (length in lines)
   ID             := uint16 (index into STRING_TABLE)
   REFERENCES     := RLENGTH [ ID ]*
   RLENGTH        := uint16

   The basic idea here is that every string that we store in the summary
   information is stored exactly once, and we reference it with a 16 bit
   index.  The references field, since it refers to other message IDs, is
   a list of indexes into the table.  References to messages which are not
   in this folder are ignored, since we don't have a way of getting to those
   messages anyway (we have no way of knowing what folder they happen to be
   in.)

   This should be an extremely compact format.  The fact that we're using
   16 bit indexes should be ok, since any folder with more than sixty-five
   thousand messages is going to be too huge to deal with anyway.

   This data is used internally to the "mailbox" protocol module, whose
   URLs take a form analagous to "file" URLs:

  'mailbox:' [ '//' HOSTNAME ]? PATHNAME
             ['?' KEY '=' VALUE ['&' KEY '=' VALUE ]*]?

   for example,

       mailbox:/u/jwz/Mail/inbox?id=199505110554.WAA14855&num=5@netscape.com

   or

       mailbox://mail-server/jwz/inbox?id=199505110554.WAA14855@netscape.com

   Currently, the only things which may appear after the "?" is "id=" followed
   by a message ID in that folder, or "num=" followed by a message number in
   the folder.  It takes a key/value form to allow for future expansion
   (additional parameters, different index methods, whatever.)
 */

#define mail_SUMMARY_MAGIC_NUMBER "# Netscape folder cache" CRLF
#define mail_SUMMARY_VERSION 4L

#define mail_MSG_ARRAY_SIZE	128

typedef struct mail_FolderData
{
  char **string_table;					/* All of the strings */
  XP_HashTable message_id_table;		/* Hashes message ID strings into
										   indexes into the `msgs' array. */

  /* `message_count' is the number of messages in the folder, including
	 any expunged messages.

	 During the process of threading, we sometimes need to manufacture
	 placeholder messages to represent expired (or otherwise
	 unavailable) parents of siblings of the same thread.  Those are
	 not included in 'message_count'. */

  uint16 message_count;

  time_t folderdate;			/* The date we believe that the folder */
								/* was last changed.  If this date ever */
								/* differs from the actual date of the */
								/* folder, we had better reparse the */
								/* folder! */

  long foldersize;				/* The number of bytes in the folder. */
								/* Again, if this ever differs from the */
								/* actual folder size, we'd better reparse */
								/* it! */

  uint32 expunged_bytes;		/* Number of bytes of messages which have been
								   expunged from the summary, but not yet
								   compacted out of the folder file itself.
								   The messages in question have the EXPUNGED
								   bit set in their flags. */

  uint32 unread_messages;		/* How many messages there are in the folder
								   without their READ flag set.*/
  uint32 total_messages;		/* How many messages in the folder that
								   are not expunged. */

  struct MSG_ThreadEntry *msgs;

  struct MSG_ThreadEntry* expungedmsgs;	/* All the messages that have the */
										/* expunged bit on in their flags.*/

  struct MSG_ThreadEntry* hiddenmsgs; /* The messages that are in the folder
										 but that we're not showing right now
										 (because, for example, we are not
										 showing any messages that have been
										 read) */

  XP_AllocStructInfo msg_blocks; /* Used to allocate all the MSG_ThreadEntry*
									entries referenced by this struct. */

  /* The below is used only for mail. */

  MSG_ThreadEntry*** msglist;	/* Array of arrays of every message in this
								   structure in message number order, including
								   hidden or expunged messages, but not
								   including expired messages. Each array
								   contains mail_MSG_ARRAY_SIZE slots; there
								   are a total of
								   (message_count/mail_MSG_ARRAY_SIZE)+1 arrays
								   present.  (This structure was chosen to use
								   memory in an efficient and non-fragmenting
								   way). */


} mail_FolderData;


XP_BEGIN_PROTOS

/* Initializes and returns an opaque object representing the parse
   state of a mailbox file.  If folder_data is NULL (the usual case),
   then it will create a new mail_FolderData structure; otherwise, it
   will append the new messages to the given folder_data structure.
   The first message parsed is considered to be at the given
   fileposition; fileposition should be the byte position in the file
   at which the first line of the first message appears.  */
extern struct msg_FolderParseState *
msg_BeginParsingFolder (MWContext* context,
						struct mail_FolderData* folder_data,
						uint32 fileposition);

/* Pass each line of the folder to this, along with the "state" object.
 */
extern int32 msg_ParseFolderLine (char *line, uint32 line_size, void *closure);



/* Done parsing a mailbox file - frees the state object and returns a folder.
 */
extern struct mail_FolderData *
msg_DoneParsingFolder (struct msg_FolderParseState *state,
					   MSG_SORT_KEY sort_key, XP_Bool sort_forward_p,
					   XP_Bool thread_p,
					   XP_Bool merge_before_sort /* If TRUE, then new messages
													are merged in with the
													old ones and the combo
													is sorted.  If FALSE,
													then the new messages
													are sorted separately and
													then tacked onto the
													end of the old ones.*/
					   );



/* Frees a struct mail_FolderData structure and all objects pointed to
   by it, including its string table and MSG_ThreadEntry structures.
 */
extern void msg_FreeFolderData (struct mail_FolderData *data);

/* Read a summary file and return a folder object, or 0.
   This does blocking reads.  Note that it is also passed the filename
   of the folder, in case the summary file contains only part of the
   data.  If the returned data contains more info than is on disk in the
   summary file (because we parsed part of the folder), then the
   provided needs_save flag is set to TRUE.
 */
extern struct mail_FolderData *msg_ReadFolderSummaryFile (MWContext* context,
														  XP_File input,
														  const char* fname,
														  XP_Bool* needs_save);

/* Given a folder, write a summary file.
   This does blocking writes.
 */
extern int msg_WriteFolderSummaryFile (struct mail_FolderData *data,
									   XP_File output);


/* Returns whether the summary file for the given folder is considered
   to have up-to-date info.  The given XP_StatStruct must be filled by
   a very recent call to XP_FileStat on the folder itself. */

extern XP_Bool msg_IsSummaryValid(const char* pathname,
								  XP_StatStruct* folderst);


/* Set the summary for given folder as having up-to-date info.  (This call
   should only be made if a recent call to msg_IsSummaryValid() returned TRUE,
   and if the only changes since made to the folder were appends.)  "num" new
   messages must have been appended, with "numunread" of them not having the
   read bit set on them.  If the given folder is being shown in the mail
   context, then the context's display and data structures will be updated with
   the new messages. */
extern void msg_SetSummaryValid(const char* pathname, int num, int numunread);


/* Get the unread and total message counts for the given folder.  If
   the summary file isn't valid, returns FALSE. */
extern XP_Bool msg_GetSummaryTotals(const char* pathname,
									uint32* unread, uint32* total,
									uint32* wasted, uint32* bytes);


/* Rethread the messages in the given structure according to the current
   sort order for the context.
 */
extern int msg_Rethread (MWContext* context, struct mail_FolderData* data);


XP_END_PROTOS

#endif /* _MAILSUM_H_ */
