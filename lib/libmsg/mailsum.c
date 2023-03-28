/* -*- Mode: C; tab-width: 4 -*-
   mailsum.c --- parsing BSD-style mail folders.
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 10-May-95.
 */


#include "msg.h"
#include "mailsum.h"
#include "threads.h"
#include "newsrc.h"
#include "xp_md5.h"
#include "mime.h"
#include "xp_time.h"


extern int MK_OUT_OF_MEMORY;
extern int MK_MIME_ERROR_WRITING_FILE;


#ifdef XP_UNIX
extern FILE *real_stdout, *real_stderr;
#endif


typedef enum
{
  MBOX_PARSE_ENVELOPE,
  MBOX_PARSE_HEADERS,
  MBOX_PARSE_BODY
} MBOX_PARSE_STATE;


struct msg_FolderParseState
{
  MWContext* context;

  MBOX_PARSE_STATE state;
  uint32 position;
  uint32 envelope_pos;
  uint32 headerstartpos;

  char *headers;
  uint32 headers_fp;
  uint32 headers_size;

  char *envelope;
  uint32 envelope_fp;
  uint32 envelope_size;

  struct message_header message_id;
  struct message_header references;
  struct message_header in_reply_to;	/* jwz -- note In-Reply-To as well */
  struct message_header date;
  struct message_header from;
  struct message_header sender;
  struct message_header to;
  struct message_header cc;
  struct message_header newsgroups;
  struct message_header subject;
  struct message_header status;
  struct message_header mozstatus;

  struct message_header envelope_from;
  struct message_header envelope_date;

  uint16 body_lines;

  struct mail_FolderData *folder_data;

  /* used only for news parsing: */
  msg_NewsRCSet *newsrc_set;
  uint32 total_msgs;
  uint32 unread_msgs;
  uint32 first_msg_number;
  uint32 last_msg_number;
  uint32 last_processed_number;
  uint32 this_msg_number;	/* used only in non-XOVER mode */

  uint32 msg_count;
  struct MSG_ThreadEntry *msgs;

  XP_HashTable string_hash_table;
  uint16 string_table_fp;
  uint16 string_table_size;
  uint16 orig_string_table_size;

  XP_Bool should_free_folder_data;

  void* partialTimer;			/* Timer to go display a partial update of
								   the news listing. */
  XP_Bool partialDoit;			/* We should do a partial update of the news
								   listing next time it's convenient. */
};


#define msg_grow_headers(state, desired_size) \
  (((desired_size) >= (state)->headers_size) ? \
   msg_GrowBuffer ((desired_size), sizeof(char), 1024, \
				   &(state)->headers, &(state)->headers_size) \
   : 0)

#define msg_grow_envelope(state, desired_size) \
  (((desired_size) >= (state)->envelope_size) ? \
   msg_GrowBuffer ((desired_size), sizeof(char), 255, \
				   &(state)->envelope, &(state)->envelope_size) \
   : 0)


/* largely lifted from mimehtml.c, which does similar parsing, sigh...
 */
static int
mbox_parse_headers (struct msg_FolderParseState *state)
{
  char *buf = state->headers;
  char *buf_end = buf + state->headers_fp;
  while (buf < buf_end)
	{
	  char *colon = XP_STRCHR (buf, ':');
	  char *end;
	  char *value = 0;
	  struct message_header *header = 0;

	  if (! colon)
		break;

	  end = colon;
	  while (end > buf && (*end == ' ' || *end == '\t'))
		end--;

	  switch (buf [0])
		{
		case 'C': case 'c':
		  if (!strncasecomp ("CC", buf, end - buf))
			header = &state->cc;
		  break;
		case 'D': case 'd':
		  if (!strncasecomp ("Date", buf, end - buf))
			header = &state->date;
		  break;
		case 'F': case 'f':
		  if (!strncasecomp ("From", buf, end - buf))
			header = &state->from;
		  break;
		case 'I': case 'i':		/* jwz -- note In-Reply-To as well */
		  if (!strncasecomp ("In-Reply-To", buf, end - buf))
			header = &state->in_reply_to;
		  break;
		case 'M': case 'm':
		  if (!strncasecomp ("Message-ID", buf, end - buf))
			header = &state->message_id;
		  break;
		case 'N': case 'n':
		  if (!strncasecomp ("Newsgroups", buf, end - buf))
			header = &state->newsgroups;
		  break;
		case 'R': case 'r':
		  if (!strncasecomp ("References", buf, end - buf))
			header = &state->references;
		  break;
		case 'S': case 's':
		  if (!strncasecomp ("Subject", buf, end - buf))
			header = &state->subject;
		  else if (!strncasecomp ("Sender", buf, end - buf))
			header = &state->sender;
		  else if (!strncasecomp ("Status", buf, end - buf))
			header = &state->status;
		  break;
		case 'T': case 't':
		  if (!strncasecomp ("To", buf, end - buf))
			header = &state->to;
		  break;
		case 'X':
		  if (!strncasecomp(X_MOZILLA_STATUS, buf, end - buf))
			header = &state->mozstatus;
		  break;
		}

	  buf = colon + 1;
	  while (*buf == ' ' || *buf == '\t')
		buf++;

	  value = buf;
	  if (header)
		header->value = value;

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
		header->length = buf - header->value;

	  if (*buf == CR || *buf == LF)
		{
		  char *last = buf;
		  if (*buf == CR && buf[1] == LF)
			buf++;
		  buf++;
		  *last = 0;	/* short-circuit const, and null-terminate header. */
		}

	  if (header)
		{
		  /* More fucking const short-circuitry... */
		  /* strip leading whitespace */
		  while (XP_IS_SPACE (*header->value))
			header->value++, header->length--;
		  /* strip trailing whitespace */
		  while (header->length > 0 &&
				 XP_IS_SPACE (header->value [header->length - 1]))
			((char *) header->value) [--header->length] = 0;
		}
	}
  return 0;
}

static int
mbox_parse_envelope (const char *line, uint32 line_size,
					 struct msg_FolderParseState *state)
{
  const char *end;
  char *s;
  int status = 0;

  status = msg_grow_envelope (state, line_size + 1);
  if (status < 0) return status;
  XP_MEMCPY (state->envelope, line, line_size);
  state->envelope_fp = line_size;
  state->envelope [line_size] = 0;
  end = state->envelope + line_size;
  s = state->envelope + 5;

  while (s < end && XP_IS_SPACE (*s))
	s++;
  state->envelope_from.value = s;
  while (s < end && !XP_IS_SPACE (*s))
	s++;
  state->envelope_from.length = s - state->envelope_from.value;

  while (s < end && XP_IS_SPACE (*s))
	s++;
  state->envelope_date.value = s;
  state->envelope_date.length = line_size - (s - state->envelope);
  while (XP_IS_SPACE
		 (state->envelope_date.value [state->envelope_date.length - 1]))
	state->envelope_date.length--;

  /* #### short-circuit const */
  ((char *) state->envelope_from.value) [state->envelope_from.length] = 0;
  ((char *) state->envelope_date.value) [state->envelope_date.length] = 0;

  return 0;
}


static int
mbox_intern (struct msg_FolderParseState *state,
			 struct message_header *header,
			 XP_Bool hack_re)
{
  char *key;
  void *found;
  int position = -1;
  uint32 L;

  if (!header || header->length == 0)
	return 0;

  XP_ASSERT (header->length == XP_STRLEN (header->value));

  key = (char *) header->value;  /* #### const evilness */

  L = header->length;

  if (hack_re)
	{
	  /* I don't understand why this cast is necessary... */
	  if (msg_StripRE((const char **) &key, &L))
		state->msgs->flags |= MSG_FLAG_HAS_RE;
	}

  if (!*key) return 0; /* To catch a subject of "Re:" */

  found = XP_Gethash (state->string_hash_table, (void *) key, (void *) 0);

  if (found)
	{
	  position = (int) found;
	  XP_ASSERT (position > 0);
	}
  else
	{
	  int status;
	  char *new_string;
	  uint16 new_fp = state->string_table_fp + 1;

	  if (new_fp == 0) {
		PRINTF(("Yee-haw!  We have run out of space in the string table.\n"));
		return MK_OUT_OF_MEMORY;
	  }

	  /* Grow the string table to make room. */
	  if (state->string_table_fp + 1 >= state->string_table_size)
		{
		  uint32 size = state->string_table_size;
		  status = msg_GrowBuffer (state->string_table_fp + 1,
								   sizeof(char *), 255,
								   (char **) &state->folder_data->string_table,
								   &size);
		  state->string_table_size = (uint16) size;
		  if (status < 0)
			return status;
		}

	  new_string = XP_STRDUP (key);
	  if (! new_string)
		return MK_OUT_OF_MEMORY;

	  /* don't use slot 0 */
	  if (state->string_table_fp == 0)
		state->folder_data->string_table [state->string_table_fp++]
		  = XP_STRDUP ("");

	  /* Add this index to the hash table. */
	  XP_ASSERT(*new_string);
	  status = XP_Puthash (state->string_hash_table,
						   (void *) new_string,
						   (void *) ((uint32) state->string_table_fp));
	  if (status < 0)
		{
		  XP_FREE (new_string);
		  return status;
		}

	  /* Add this string to the table. */
	  position = state->string_table_fp++;
	  state->folder_data->string_table [position] = new_string;
	  state->folder_data->string_table [position + 1] = 0;

	  XP_ASSERT (mbox_intern (state, header, hack_re) == position);
	}

  return position;
}

/* Like mbox_intern() but for headers which contain email addresses:
   we extract the "name" component of the first address, and discard
   the rest. */
static int
mbox_intern_rfc822 (struct msg_FolderParseState *state,
					struct message_header *header)
{
  struct message_header dummy;
  int status;
  char *s;
  if (!header || header->length == 0)
	return 0;

  XP_ASSERT (header->length == XP_STRLEN (header->value));
  /* #### The mallocs here might be a performance bottleneck... */
  s = MSG_ExtractRFC822AddressName (header->value);
  if (! s)
	return MK_OUT_OF_MEMORY;
  dummy.value = s;
  dummy.length = XP_STRLEN (s);
  status = mbox_intern (state, &dummy, FALSE);
  XP_FREE (s);
  return status;
}

static int
mbox_intern_references (uint16 **references_listP,
						struct msg_FolderParseState *state,
						const struct message_header *header)
{
  int status = 0;
  int i = 0;
  int n_refs = 0;
  const char *end;
  const char *s;
  const char *start;
  
  if (!header || header->length == 0)
	return 0;

  end = header->value + header->length;

  for (s = header->value; s < end; s++)
	if (*s == '>')
	  n_refs++;

  if (*references_listP)
	XP_FREE (references_listP);
  *references_listP = (uint16 *) XP_ALLOC ((n_refs + 1) * sizeof (uint16));
  if (! *references_listP)
	return MK_OUT_OF_MEMORY;

  start = s = header->value;

  /* jwz: skip forward to "<", not merely over whitespace. */
  while (start < end && *start && *start != '<')
	start++;
  if (*start == '<') start++;
  s = start;
  while (s < end)
	if (*s == '>')
	  {
		struct message_header dummy;
		dummy.value = start;
		*((char *) s) = 0;  /* gag, and short-circuit const */
		dummy.length = s - start;
		s++;
		status = mbox_intern (state, &dummy, FALSE);
		if (status < 0)
		  return status;
		if (i < n_refs) {
		  (*references_listP) [i++] = status;
		}
		start = ++s;

        /* jwz: skip forward to "<", not merely over whitespace. */
		while (s < end && *s && *s != '<')
		  s++;
		if (*s == '<') s++;
		start = s;
	  }
	else
	  {
		s++;
	  }

  (*references_listP) [i++] = 0;   /* null-terminate it */

  return status;
}


static int
mbox_finalize_headers (struct msg_FolderParseState *state)
{
  struct MSG_ThreadEntry *msg;
  struct MSG_ThreadEntry *newm;
  int status = 0;
  struct message_header *sender;
  struct message_header *recipient;
  struct message_header *subject;
  struct message_header *id;
  struct message_header *references;
  struct message_header *in_reply_to;	/* jwz -- note In-Reply-To as well */
  struct message_header *date;
  struct message_header *statush;
  struct message_header *mozstatus;

  struct message_header md5_header;
  unsigned char md5_bin [16];
  char md5_data [50];

  const char *s;

#ifdef _USRDLL
  return(0);
#endif

  sender     = (state->from.length          ? &state->from :
				state->sender.length        ? &state->sender :
				state->envelope_from.length ? &state->envelope_from :
				0);
  recipient  = (state->to.length         ? &state->to :
				state->cc.length         ? &state->cc :
				state->newsgroups.length ? &state->newsgroups :
				sender);
  subject    = (state->subject.length    ? &state->subject    : 0);
  id         = (state->message_id.length ? &state->message_id : 0);
  references = (state->references.length ? &state->references : 0);
  in_reply_to= (state->in_reply_to.length? &state->in_reply_to: 0); /* jwz */
  statush    = (state->status.length     ? &state->status     : 0);
  mozstatus  = (state->mozstatus.length  ? &state->mozstatus  : 0);
  date       = (state->date.length       ? &state->date :
				state->envelope_date.length ? &state->envelope_date :
				0);

  newm = (struct MSG_ThreadEntry *)
	XP_AllocStructZero(&state->folder_data->msg_blocks);
  if (! newm)
	return MK_OUT_OF_MEMORY;
  newm->next = state->msgs;
  state->msgs = newm;
  msg = newm;

  status = mbox_intern_rfc822 (state, sender);
  if (status < 0) return status;
  msg->sender = status;

  if (recipient == &state->newsgroups)
	{
	  /* In the case where the recipient is a newsgroup, truncate the string
		 at the first comma.  This is used only for presenting the thread list,
		 and newsgroup lines tend to be long and non-shared, and tend to bloat
		 the string table.  So, by only showing the first newsgroup, we can
		 reduce memory and file usage at the expense of only showing the one
		 group in the summary list, and only being able to sort on the first
		 group rather than the whole list.  It's worth it. */
	  char *s;
	  XP_ASSERT (recipient->length == XP_STRLEN (recipient->value));
	  s = XP_STRCHR (recipient->value, ',');
	  if (s)
		{
		  *s = 0;
		  recipient->length = XP_STRLEN (recipient->value);
		}
	  status = mbox_intern (state, recipient, FALSE);
	}
  else
	{
	  status = mbox_intern_rfc822 (state, recipient);
	}
  if (status < 0) return status;
  msg->recipient = status;

  status = mbox_intern (state, subject, TRUE);
  if (status < 0) return status;
  msg->subject = status;

  if (! id)
	{
	  XP_Md5Binary (state->headers, (int) state->headers_fp, md5_bin);
	  PR_snprintf (md5_data, sizeof(md5_data),
				   "<md5:"
				   "%02X%02X%02X%02X%02X%02X%02X%02X"
				   "%02X%02X%02X%02X%02X%02X%02X%02X"
				   ">",
				   md5_bin[0], md5_bin[1], md5_bin[2], md5_bin[3],
				   md5_bin[4], md5_bin[5], md5_bin[6], md5_bin[7],
				   md5_bin[8], md5_bin[9], md5_bin[10],md5_bin[11],
				   md5_bin[12],md5_bin[13],md5_bin[14],md5_bin[15]);
	  md5_header.value = md5_data;
	  md5_header.length = XP_STRLEN (md5_data);
	  id = &md5_header;
	}

  /* Take off <> around message ID. */
  if (id->value[0] == '<')
	id->value++, id->length--;
  if (id->value[id->length-1] == '>')
	((char *) id->value) [id->length-1] = 0, id->length--; /* #### const */

  status = mbox_intern (state, id, FALSE);
  if (status < 0) return status;
  msg->id = status;
  XP_ASSERT(msg->id > 0);

  /* jwz -- if there is no References, check In-Reply-To. */
  if (references)
    status = mbox_intern_references (&msg->references, state, references);
  else
    status = mbox_intern_references (&msg->references, state, in_reply_to);

  if (status < 0) return status;

  if (date)
	{
	  msg->date = XP_ParseTimeString (date->value, FALSE);
	}

  if (mozstatus) {
	uint32 delta;
	if (strlen(mozstatus->value) == 4) {
#define UNHEX(C) \
	  ((C >= '0' && C <= '9') ? C - '0' : \
	   ((C >= 'A' && C <= 'F') ? C - 'A' + 10 : \
		((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)))
	  int i;
	  uint32 flags = 0;
	  for (i=0,s=mozstatus->value ; i<4 ; i++,s++) {
		flags = (flags << 4) | UNHEX(*s);
	  }
	  flags &= ~MSG_FLAG_RUNTIME_ONLY;
	  /* We trust the X-Mozilla-Status line to be the smartest in almost
		 all things.  One exception, however, is the HAS_RE flag.  Since
		 we just parsed the subject header anyway, we expect that parsing
		 to be smartest.  (After all, what if someone just went in and
		 edited the subject line by hand?) */
	  msg->flags = (flags & ~MSG_FLAG_HAS_RE) | (msg->flags & MSG_FLAG_HAS_RE);
	}
	delta = (state->headerstartpos +
			 (mozstatus->value - state->headers) -
			 (2 + X_MOZILLA_STATUS_LEN)		/* 2 extra bytes for ": ". */
			 ) - state->envelope_pos;
	if (delta < 0xffff) {		/* Only use if fits in 16 bits. */
	  msg->data.mail.status_offset = delta;
	  XP_ASSERT(msg->data.mail.status_offset < 10000); /* ### Debugging hack */
	}
  } else if (statush)
	{
	  /* Parse a little bit of the Berkeley Mail status header. */
	  for (s = statush->value; *s; s++)
		switch (*s)
		  {
		  case 'R': case 'r':
			msg->flags |= MSG_FLAG_READ;
			break;
		  case 'D': case 'd':
			/* msg->flags |= MSG_FLAG_EXPUNGED;  ### Is this reasonable? */
			break;
		  case 'N': case 'n':
		  case 'U': case 'u':
			msg->flags &= (~MSG_FLAG_READ);
			break;
		  }
	}

  if (!(msg->flags & MSG_FLAG_EXPUNGED)) {
	/* Assert checks that we don't have multiple messages with the same id
	   in news.  Not that we have any way of gracefully dealing with it if
	   a duplicate does exist, but this will help us notice bugs... */
	XP_ASSERT(state->context->type != MWContextNews ||
			  XP_Gethash(state->folder_data->message_id_table,
						 state->folder_data->string_table [msg->id], 0) == 0);
	/* Note that this stomps on any other existing entry with the same message
       id.  That's probably right; we want the last message in the inbox to
       win. */
	status = XP_Puthash(state->folder_data->message_id_table,
						state->folder_data->string_table [msg->id],
						msg);
  }

  return 0;
}



int32
msg_ParseFolderLine (char *line, uint32 line_size, void *closure)
{
  struct msg_FolderParseState *state = (struct msg_FolderParseState *) closure;
  int status = 0;
  if (line[0] == 'F' && msg_IsEnvelopeLine(line, line_size))
	{
	  XP_ASSERT (state->state == MBOX_PARSE_BODY); /* else folder corrupted */

	  if (state->msgs)
		{
		  struct MSG_ThreadEntry *msg = state->msgs;
		  msg->data.mail.file_index = state->envelope_pos;
		  msg->data.mail.byte_length = state->position - state->envelope_pos;
		  msg->lines = state->body_lines;

		  if (state->state == MBOX_PARSE_BODY)
			/* This will always happen, except in cases where the above ASSERT
			   was tripped.  In that case, we've gotten a "From " line within
			   the header block (perhaps two consecutive "From " lines with
			   no headers.)  So don't increment the pointer, just overwrite
			   the "partial" one. */
			state->msg_count++;

		  XP_ASSERT(msg->id > 0);

		  state->headers_fp = 0;
		  state->envelope_fp = 0;
		  state->message_id.length = 0;
		  state->references.length = 0;
		  state->in_reply_to.length = 0;	/* jwz */
		  state->date.length = 0;
		  state->from.length = 0;
		  state->sender.length = 0;
		  state->to.length = 0;
		  state->cc.length = 0;
		  state->newsgroups.length = 0;
		  state->subject.length = 0;
		  state->status.length = 0;
		  state->mozstatus.length = 0;
		  state->envelope_from.length = 0;
		  state->envelope_date.length = 0;
		  state->body_lines = 0;
		}

	  state->envelope_pos = state->position;
	  state->state = MBOX_PARSE_HEADERS;
	  state->headerstartpos = state->position + line_size;
	  status = mbox_parse_envelope (line, line_size, state);
	  if (status < 0)
		return status;
	}
  else if (state->state == MBOX_PARSE_HEADERS)
	{
	  if (line[0] == CR || line[0] == LF || line[0] == 0)
		{
		  /* End of headers.  Now parse them. */
		  status = mbox_parse_headers (state);
		  if (status < 0)
			return status;

		  status = mbox_finalize_headers (state);
		  if (status < 0)
			return status;
		  state->state = MBOX_PARSE_BODY;
		}
	  else
		{
		  /* Otherwise, this line belongs to a header.  So append it to the
			 header data, and stay in MBOX `MIME_PARSE_HEADERS' state.
		   */
		  status = msg_grow_headers (state, line_size + state->headers_fp + 1);
		  if (status < 0) return status;
		  XP_MEMCPY (state->headers + state->headers_fp, line, line_size);
		  state->headers_fp += line_size;
		}
	}
  else if (state->state == MBOX_PARSE_BODY)
	{
	  state->body_lines++;
	}

  state->position += line_size;

  return 0;
}


static void
msg_do_partial_timer(void* closure)
{
  struct msg_FolderParseState* state = (struct msg_FolderParseState*) closure;
  state->partialDoit = TRUE;
  state->partialTimer = NULL;
}

int
MSG_ProcessXOVER (MWContext *context, char *line, void **parse_data)
{
  int status = 0;
  struct msg_FolderParseState *state;
  struct MSG_ThreadEntry *msg;
  char *next;
  int32 message_number, lines;
  XP_Bool read_p = FALSE;

  XP_ASSERT (line && parse_data);
  if (!line || !parse_data)
	return -1;

  XP_ASSERT (context->msgframe &&
			 context->type == MWContextNews);
  if (!context->msgframe ||
	  context->type != MWContextNews)
	return -1;

  state = (struct msg_FolderParseState *) *parse_data;
  XP_ASSERT (state);
  if (! state) return -1;

  state->total_msgs++;

  next = line;

#define GET_TOKEN()								\
  line = next;									\
  next = (line ? XP_STRCHR (line, '\t') : 0);	\
  if (next) *next++ = 0

  GET_TOKEN ();											/* message number */
  message_number = atol (line);

  XP_ASSERT(message_number > state->last_processed_number ||
			message_number == 1);
  if (state->newsrc_set &&
	  message_number > state->last_processed_number + 1) {
	/* There are some articles that XOVER skipped; they must no longer
	   exist.  Mark them as read in the newsrc, so we don't include them
	   next time in our estimated number of unread messages. */
	if (msg_MarkRangeAsRead(state->newsrc_set,
							state->last_processed_number + 1,
							message_number - 1)) {
	  /* This isn't really an important enough change to warrant causing
		 the newsrc file to be saved; we haven't gathered any information
		 that won't also be gathered for free next time.
	  msg_NoteNewsrcChanged(context);
       */
	}
  }

  state->last_processed_number = message_number;
  if (context->msgframe->data.news.knownarts.set) {
	status = msg_MarkArtAsRead(context->msgframe->data.news.knownarts.set,
							   message_number);
	if (status < 0) return status;
  }

  if (message_number > state->last_msg_number)
	state->last_msg_number = message_number;
  else if (message_number < state->first_msg_number)
	state->first_msg_number = message_number;

  if (state->newsrc_set)
	read_p = msg_NewsRCIsRead (state->newsrc_set, message_number);

  if (!read_p)
	state->unread_msgs++;

  /* Update the thermometer with a percentage of articles retrieved.
   */
  if (state->last_msg_number > state->first_msg_number)
	{
	  int32 percent = 100 * (((double) (state->last_processed_number-
										state->first_msg_number)) /
							 ((double) (state->last_msg_number -
										state->first_msg_number)));
	  FE_SetProgressBarPercent (context, percent);
	}

  /* If this message is read, and we're only displaying unread messages,
	 then don't display it. */
  if (read_p && !context->msgframe->all_messages_visible_p)
	return 0;

  GET_TOKEN ();											/* subject */
  state->subject.value = line;
  state->subject.length = line ? XP_STRLEN (line) : 0;

  GET_TOKEN ();											/* sender */
  state->sender.value = line;
  state->sender.length = line ? XP_STRLEN (line) : 0;

  GET_TOKEN ();											/* date */
  state->date.value = line;
  state->date.length = line ? XP_STRLEN (line) : 0;

  GET_TOKEN ();											/* message id */
  state->message_id.value = line;
  state->message_id.length = line ? XP_STRLEN (line) : 0;

  GET_TOKEN ();											/* references */
  state->references.value = line;
  state->references.length = line ? XP_STRLEN (line) : 0;

  GET_TOKEN ();											/* bytes */
  /* ignored */

  GET_TOKEN ();											/* lines */
  lines = line ? atol (line) : 0;

  GET_TOKEN ();											/* xref */
  /* ignored here - we parse it from the actual headers as needed */

  if (state->message_id.value == NULL) {
	PRINTF(("Missing message id for article %d; ignoring it.\n",
			message_number));
	return 0;					/* Silently ignore this bogus thing. */
  }

  if (state->message_id.value[0] == '<') {
	state->message_id.value++;
	state->message_id.length--;
  }
  if (state->message_id.value[state->message_id.length-1] == '>') {
	((char *) state->message_id.value)[state->message_id.length-1] = 0;
								/* #### const */
	state->message_id.length--;
  }

  if (XP_Gethash(state->folder_data->message_id_table,
				 state->message_id.value, 0)) {
	PRINTF(("Duplicate message id for article %d; ignoring it.\n",
			message_number));
	return 0;					/* Silently ignore this bogus thing. */
  }


  status = mbox_finalize_headers (state);
  if (status < 0) return status;

  msg = state->msgs;
  state->msg_count++;

  msg->data.news.article_number = message_number;
  msg->lines = lines;

  if (read_p)
	msg->flags |= MSG_FLAG_READ;

  if (state->partialDoit) {
	/* save these so we can reinitialize. LJM */
	int32 first_msg_number = state->first_msg_number;
	int32 last_msg_number = state->last_msg_number;
	int32 last_processed_number = state->last_processed_number;
	msg_NewsRCSet *set = state->newsrc_set;
	state->last_msg_number = state->last_processed_number;

	MSG_FinishXOVER(context, parse_data, 0);
	*parse_data = msg_BeginParsingFolder(context, context->msgframe->msgs, 0);
	state = *parse_data;
	state->partialDoit = FALSE;
	/* reinitialize. LJM */
	state->first_msg_number = first_msg_number;
	state->last_msg_number = last_msg_number;
	state->last_processed_number = last_processed_number;
	state->newsrc_set = set;
  }
  if (!state->partialTimer) {
	state->partialTimer = FE_SetTimeout(msg_do_partial_timer, state, 2000);
  }

  return status;
}


/* When we don't have XOVER, but use HEAD, this is called instead.
   It reads lines until it has a whole header block, then parses the
   headers; then takes selected headers and creates an XOVER line
   from them.  This is more for simplicity and code sharing than
   anything else; it means we end up parsing some things twice.
   But if we don't have XOVER, things are going to be so horribly
   slow anyway that this just doesn't matter.
 */
int
MSG_ProcessNonXOVER (MWContext *context, char *line, void **parse_data)
{
  int status = 0;
  struct msg_FolderParseState *state;

  XP_ASSERT (line && parse_data);
  if (!line || !parse_data)
	return -1;

  XP_ASSERT (context->msgframe &&
			 context->type == MWContextNews);
  if (!context->msgframe ||
	  context->type != MWContextNews)
	return -1;

  state = (struct msg_FolderParseState *) *parse_data;
  XP_ASSERT (state);
  if (! state) return -1;

  if (line[0] == '.' && line[1] == 0)
	{
	  int32 size;
	  char *s, *s2;
	  XP_ASSERT (state->state == MBOX_PARSE_HEADERS); /* else data corrupted */

	  status = mbox_parse_headers (state);
	  if (status < 0) return status;

	  size = (state->subject.length +
			  state->sender.length +
			  state->date.length +
			  state->message_id.length +
			  state->references.length +
			  30);
	  status = msg_grow_envelope (state, size);
	  if (status < 0) return status;
	  s = state->envelope;
	  PR_snprintf (s, size, "%ld\t", state->this_msg_number);
	  s += XP_STRLEN (s);
# define PUSH_HDR(HDR) \
		    XP_MEMCPY (s, state->HDR.value, state->HDR.length); \
			s2 = s; \
		    s += state->HDR.length; \
			while (s2 < s) { /* take out tabs within this field */ \
			  if (*s2 == '\t') *s2 = ' '; \
			  s2++; } \
		    *s++ = '\t'

      PUSH_HDR (subject);
	  PUSH_HDR (sender);
	  PUSH_HDR (date);
	  PUSH_HDR (message_id);
	  PUSH_HDR (references);
	  *s++ = '\t'; /* bytes */
	  *s++ = '\t'; /* lines */
	  *s++ = '\t'; /* xref */
	  *s = 0;
# undef PUSH_HDR
	  status = MSG_ProcessXOVER (context, state->envelope, parse_data);
	  if (status < 0) return status;

      state = (struct msg_FolderParseState *) *parse_data;
      XP_ASSERT (state);
      if (! state) return -1;

	  state->headers_fp = 0;
	  state->envelope_fp = 0;
	  state->message_id.length = 0;
	  state->references.length = 0;
	  state->in_reply_to.length = 0;	/* jwz */
	  state->date.length = 0;
	  state->from.length = 0;
	  state->sender.length = 0;
	  state->to.length = 0;
	  state->cc.length = 0;
	  state->newsgroups.length = 0;
	  state->subject.length = 0;
	  state->status.length = 0;
	  state->mozstatus.length = 0;
	  state->envelope_from.length = 0;
	  state->envelope_date.length = 0;
	  state->body_lines = 0;

	  state->state = MBOX_PARSE_BODY;
	}
  else if (state->state == MBOX_PARSE_BODY)				/* initial state */
	{

	  state->this_msg_number = 0;
	  if (1 != sscanf (line, " %ld ", &state->this_msg_number))
		{
		  XP_ASSERT (0);
		  return -1;
		}
	  state->state = MBOX_PARSE_HEADERS;
	}
  else if (state->state == MBOX_PARSE_HEADERS)
	{
	  int32 line_size = XP_STRLEN (line);
	  status = msg_grow_headers (state, line_size + state->headers_fp + 4);
	  if (status < 0) return status;
	  XP_MEMCPY (state->headers + state->headers_fp, line, line_size);
	  state->headers_fp += line_size;
	  state->headers[state->headers_fp++] = CR;
	  state->headers[state->headers_fp++] = LF;
	}
  else
	{
	  XP_ASSERT (0);
	  return -1;
	}

  return status;
}


static int
mbox_cmp (const void *obj1, const void *obj2)
{
  XP_ASSERT (obj1 && obj2);
  return XP_STRCMP ((char *) obj1, (char *) obj2);
}


static MSG_ThreadEntry *
mbox_dummy_creator (void *arg)
{
  XP_AllocStructInfo *alloc_info = (XP_AllocStructInfo *) arg;
  struct MSG_ThreadEntry *dummy;

  dummy = (struct MSG_ThreadEntry *) XP_AllocStructZero (alloc_info);
  if (! dummy)
	return 0;

  return dummy;
}


static void
mbox_dummy_destroyer (struct MSG_ThreadEntry *dummy, void *arg)
{
  XP_AllocStructInfo *alloc_info = (XP_AllocStructInfo *) arg;
  XP_ASSERT (dummy);
  if (dummy)
	XP_FreeStruct (alloc_info, (void *) dummy);
}


static void
mbox_free_state (struct msg_FolderParseState *state)
{
  if (! state) return;

  FREEIF (state->headers);
  FREEIF (state->envelope);
  if (state->string_hash_table)
	{
	  XP_HashTableDestroy (state->string_hash_table);
	  state->string_hash_table = 0;
	}
  if (state->folder_data && state->folder_data->message_id_table)
	{
	  XP_HashTableDestroy (state->folder_data->message_id_table);
	  state->folder_data->message_id_table = 0;
	}

  if (state->folder_data) {
	if (state->folder_data->string_table) {
	  uint16 i;
	  for (i = state->orig_string_table_size ; i<state->string_table_fp ; i++){
		FREEIF(state->folder_data->string_table[i]);
	  }
	  XP_FREE(state->folder_data->string_table);
	  state->folder_data->string_table = 0;
	}
	if (state->should_free_folder_data) {
	  XP_FreeAllStructs (&state->folder_data->msg_blocks);
	  msg_FreeFolderData (state->folder_data);
	}
  }
  XP_FREE (state);
}

struct msg_FolderParseState *
msg_BeginParsingFolder (MWContext* context,
						struct mail_FolderData* folder_data,
						uint32 fileposition)
{
  struct msg_FolderParseState *state = XP_NEW (struct msg_FolderParseState);
  XP_Bool createdfolderdata = (folder_data == NULL);
  if (! state)
	return 0;

  XP_MEMSET (state, 0, sizeof (*state));
  state->context = context;
  state->state = MBOX_PARSE_BODY;
  state->position = fileposition;

  if (!folder_data) {
	/* Figure out how big the blocks we're allocating is.  This is
	   sleazy as hell, but lets us save 4 bytes per message in
	   newsgroups versus mail folders.  Similar code is in
	   mbox_read_summary_file(). */
	static struct MSG_ThreadEntry dummy;
	uint32 mail_size = ((uint8 *) &dummy.data.mail.sleazy_end_of_data -
						(uint8 *) &dummy);
#ifndef NDEBUG
	uint32 news_size = ((uint8 *) &dummy.data.news.sleazy_end_of_data -
						(uint8 *) &dummy);
	/* if this isn't actually doing anything, I'd like to know that... */
	XP_ASSERT (news_size < mail_size);
#endif
	folder_data = XP_NEW_ZAP(struct mail_FolderData);
	if (!folder_data) goto FAIL;
	XP_InitAllocStructInfo(&(folder_data->msg_blocks), mail_size);
	state->should_free_folder_data = TRUE;
	folder_data->message_id_table =
	  XP_HashTableNew (500, XP_StringHash, mbox_cmp);
	if (!folder_data->message_id_table) goto FAIL;
  }

  state->msg_count = folder_data->message_count;

  state->string_hash_table = XP_HashTableNew (500, XP_StringHash, mbox_cmp);
  if (!state->string_hash_table) goto FAIL;

  state->folder_data = folder_data;

  if (folder_data->string_table) {
	uint16 i = 1;
	char** table = folder_data->string_table + 1;
	int status = 0;
	for (; *table ; table++) {
	  XP_ASSERT(**table);
	  if (!**table)
		{
		  PRINTF(("Found extra \"\" in string table.  Discarding summary.\n"));
		  goto FAIL;
		}
	  XP_ASSERT(XP_Gethash(state->string_hash_table, (void*) *table,
						   (void*) 0) == 0);
	  if (status == 0) {
		status = XP_Puthash(state->string_hash_table, (void*) *table,
							(void*) ((uint32) i));
	  }
	  i++;
	}
	state->string_table_size = i;
	state->string_table_fp = i;
	state->orig_string_table_size = i;
	if (status < 0) goto FAIL;
  }

  return state;

FAIL:
  if (createdfolderdata) msg_FreeFolderData(folder_data);
  mbox_free_state(state);
  return 0;
}


struct mail_FolderData *
msg_DoneParsingFolder (struct msg_FolderParseState *state,
					   MSG_SORT_KEY sort_key, XP_Bool sort_forward_p,
					   XP_Bool thread_p, XP_Bool merge_before_sort)
{
  struct mail_FolderData *result;
  MSG_ThreadEntry* msg;
  MSG_ThreadEntry* prev;
  MSG_ThreadEntry* next;
  int flags;
  int num;
  int start;
  int i;

  XP_ASSERT (state);
  if (! state) return 0;

  result = state->folder_data;

  /* We've reached the end of the file; close off the last message.
   */
  if (state->state == MBOX_PARSE_BODY &&
	  state->position > 0 &&
	  state->msgs)
	{
	  msg = state->msgs;
	  msg->data.mail.file_index = state->envelope_pos;
	  msg->data.mail.byte_length = state->position - state->envelope_pos;
	  msg->lines = state->body_lines;
 	  state->msg_count++;
	}
  /* Shrink the string table and msg list to their minimal sizes.
   */
  if (state->string_table_size > state->string_table_fp + 1)
  {
	result->string_table =
	  (char **) XP_REALLOC (result->string_table,
							(state->string_table_fp + 1) * sizeof (char *));
  }
  if (result->string_table) {
	result->string_table [state->string_table_fp] = 0;
  }

  /* Now that we know how many messages there will be, allocate the
     msglist structure for it. */
  num = state->msg_count / mail_MSG_ARRAY_SIZE + 1;
  if (result->msglist) {
	result->msglist = (MSG_ThreadEntry***)
	  XP_REALLOC(result->msglist, num * sizeof(MSG_ThreadEntry**));
	start = result->message_count / mail_MSG_ARRAY_SIZE + 1;
  } else {
	result->msglist = (MSG_ThreadEntry***)
	  XP_ALLOC(num * sizeof(MSG_ThreadEntry**));
	start = 0;
  }
  if (result->msglist == NULL) {
	return 0;					/* ### */
  }
  for (i=start ; i<num ; i++) {
	result->msglist[i] = (MSG_ThreadEntry**)
	  XP_ALLOC(sizeof(MSG_ThreadEntry*) * mail_MSG_ARRAY_SIZE);
	if (!result->msglist[i]) {
	  return 0;					/* ### */
	}
  }

  /* Now go through all the messages, peel off any expunged or hidden
	 messages into their own lists, and put the messages into the msglist. */
  num = state->msg_count;
  flags = MSG_FLAG_EXPUNGED;
  if (state->context->msgframe &&
	  !state->context->msgframe->all_messages_visible_p) {
	flags |= MSG_FLAG_READ;
  }
  for (msg = state->msgs ; msg ; msg = next) {
	XP_ASSERT
	  (num == state->msg_count ||
	   state->context->type == MWContextNews ||
	   msg->data.mail.file_index <
	   (result->msglist[num/mail_MSG_ARRAY_SIZE][num%mail_MSG_ARRAY_SIZE]
		->data.mail.file_index));
	num--;
	XP_ASSERT(num >= 0);
	result->msglist[num/mail_MSG_ARRAY_SIZE][num%mail_MSG_ARRAY_SIZE] = msg;
	next = msg->next;
	if (msg->flags & flags) {
	  if (msg == state->msgs) state->msgs = msg->next;
	  else prev->next = msg->next;
	  if (msg->flags & MSG_FLAG_EXPUNGED) {
		msg->next = result->expungedmsgs;
		result->expungedmsgs = msg;
	  } else {
		XP_ASSERT(msg->flags & MSG_FLAG_READ);
		msg->next = result->hiddenmsgs;
		result->hiddenmsgs = msg;
	  }
	} else {
	  prev = msg;
	  result->total_messages++;
	  if (!(msg->flags & MSG_FLAG_READ)) result->unread_messages++;
	}
  }

  XP_ASSERT(num == result->message_count);
  result->message_count = state->msg_count;

  /* Now thread the messages. */
  if (state->msgs) {
	struct MSG_ThreadEntry *newm;
	if (merge_before_sort) {
	  newm = state->msgs;
	} else {
	  newm = 
		msg_RethreadMessages (state->context,
							  state->msgs,
							  result->message_count,
							  result->string_table,
							  FALSE, sort_key, sort_forward_p, thread_p,
							  mbox_dummy_creator,
							  mbox_dummy_destroyer,
							  (void *) &result->msg_blocks);
	}
	if (newm) {
	  if (result->msgs) {
		for (msg = result->msgs ; msg->next ; msg = msg->next)
		  ;
		msg->next = newm;
	  } else {
		result->msgs = newm;
	  }
	}
	if (merge_before_sort && result->msgs) {
	  result->msgs = 
		msg_RethreadMessages (state->context, result->msgs,
							  result->message_count,
							  result->string_table,
							  FALSE, sort_key, sort_forward_p, thread_p,
							  mbox_dummy_creator,
							  mbox_dummy_destroyer,
							  (void *) &result->msg_blocks);
	}
  }

  state->folder_data = 0;

  state->msgs = 0;

  mbox_free_state (state);

  return result;
}


static void
mbox_free_thread_references (struct MSG_ThreadEntry *msg)
{
 RECURSE:
  XP_ASSERT (msg);
  if (! msg) return;
  FREEIF (msg->references);
  if (msg->next && msg->first_child)
	{
	  /* #### toy-computer-danger, real recursion. */
	  mbox_free_thread_references (msg->first_child);
	  msg = msg->next;
	  goto RECURSE;
	}
  else if (msg->first_child)
	{
	  msg = msg->first_child;
	  goto RECURSE;
	}
  else if (msg->next)
	{
	  msg = msg->next;
	  goto RECURSE;
	}
}

void
msg_FreeFolderData (struct mail_FolderData *data)
{
  int i;
  if (! data) return;
  if (data->msglist) {
	for (i=data->message_count/mail_MSG_ARRAY_SIZE ; i>=0 ; i--) {
	  XP_FREE(data->msglist[i]);
	}
	XP_FREE(data->msglist);
  }
  if (data->message_id_table)
    XP_HashTableDestroy (data->message_id_table);

  i = 0;
  if (data->string_table)
	{
	  while (data->string_table[i])
		XP_FREE (data->string_table[i++]);
	  XP_FREE (data->string_table);
	}

  if (data->msgs)
	mbox_free_thread_references (data->msgs);
  if (data->expungedmsgs)
	mbox_free_thread_references (data->expungedmsgs);
  if (data->hiddenmsgs)
	mbox_free_thread_references (data->hiddenmsgs);
  XP_FreeAllStructs (&data->msg_blocks);

  XP_FREE (data);
}

int
MSG_ResetXOVER ( MWContext *context,
				 void **parse_data )
{
  int status = 0;
  struct msg_FolderParseState *state;

  if (!parse_data || !*parse_data) return status;

  state = (struct msg_FolderParseState *) *parse_data;
  state->last_msg_number = state->first_msg_number;
  state->last_processed_number = state->first_msg_number;

  return status;
}

int
MSG_InitXOVER (MWContext *context,
			   const char *host_and_port, XP_Bool secure_p,
			   const char *group_name,
			   uint32 first_msg, uint32 last_msg,
			   uint32 oldest_msg, uint32 youngest_msg,
			   void **parse_data)
{
  struct msg_FolderParseState *state;
  int status = 0;

  /* Front end should only run this kind of URL in a news window. */
  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;

  /* Consistancy checks, not that I know what to do if it fails (it will
	 probably handle it OK...) */
  XP_ASSERT(first_msg <= last_msg);
			 

  msg_DisableUpdates(context);


  status = msg_EnsureNewsgroupSelected (context, host_and_port, group_name,
										secure_p);
  if (status < 0) goto FAIL;

  state = (parse_data
		   ? (struct msg_FolderParseState *) *parse_data
		   : 0);

  if (state) {
	/* If any XOVER lines from the last time failed to come in, mark those
	   messages as read. */
	if (state->newsrc_set &&
        state->last_processed_number < state->last_msg_number) {
	  msg_MarkRangeAsRead(state->newsrc_set, state->last_processed_number + 1,
						  state->last_msg_number);
	}
  } else {
	state = msg_BeginParsingFolder (context, context->msgframe->msgs, 0);
	if (!state) {
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
  }

  state->first_msg_number = first_msg;
  state->last_msg_number = last_msg;
  state->last_processed_number = first_msg > 1 ? first_msg - 1 : 1;

  state->newsrc_set =
	msg_GetNewsRCSet (context, group_name,
					  context->msgframe->data.news.newsrc_file);

  *parse_data = state;

  msg_RedisplayOneFolder (context, context->msgframe->opened_folder, -1, -1,
						  TRUE);

  XP_ASSERT (context->msgframe->data.news.current_host);
  if (!context->msgframe->data.news.current_host) {
	status = -1;
	goto FAIL;
  }
FAIL:
  msg_EnableUpdates(context);
  return status;
}

static void
msg_count_unread_and_total(MSG_ThreadEntry* msg, int32* unread, int32* total)
{
  for (; msg ; msg = msg->next) {
	if (!(msg->flags & MSG_FLAG_EXPIRED)) {
	  (*total)++;
	  if (!(msg->flags & MSG_FLAG_READ)) (*unread)++;
	}
	if (msg->first_child) {
	  msg_count_unread_and_total(msg->first_child, unread, total);
	}
  }
}


int
MSG_FinishXOVER (MWContext *context, void **parse_data, int status)
{
  struct msg_FolderParseState *state;
  struct mail_FolderData *fd;
  struct MSG_NewsKnown* k;
  XP_Bool partialDoit = FALSE;
 
  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews) return -1;

  state = (parse_data
		   ? (struct msg_FolderParseState *) *parse_data
		   : 0);
  XP_ASSERT (state);
  if (! state) return -1;

  if (state->partialTimer) {
	FE_ClearTimeout(state->partialTimer);
	state->partialTimer = NULL;
  }

  /* If any XOVER lines from the last time failed to come in, mark those
	 messages as read. */

  if (state->newsrc_set && status >= 0 &&
      state->last_processed_number < state->last_msg_number) {
	msg_MarkRangeAsRead(state->newsrc_set, state->last_processed_number + 1,
						state->last_msg_number);
  }

  partialDoit = state->partialDoit;

  fd = msg_DoneParsingFolder (state,
							  context->msgframe->sort_key,
							  context->msgframe->sort_forward_p,
							  context->msgframe->thread_p,
							  TRUE);
  *parse_data = 0;
  /* after this point state is no longer valid; do NOT use it */
  /* it would be nice to set it to zero. but no one is using it why bother */
  /* state = 0; */

  if (! fd) return 0;

  if (context->msgframe->msgs && context->msgframe->msgs != fd)
	msg_FreeFolderData (context->msgframe->msgs);

  context->msgframe->msgs = fd;
  msg_ReCountSelectedMessages(context);

  msg_DisableUpdates(context);
  if (!msg_auto_expand_news_threads) {
	msg_ElideAllToplevelThreads(context);
  }
  msg_RedisplayMessages (context);
  msg_RedisplayOneMessage(context, context->msgframe->displayed_message,
						  -1, -1, TRUE);
  context->msgframe->cacheinfo.valid = FALSE;
  msg_UpdateToolbar(context);

  k = &(context->msgframe->data.news.knownarts);

  if (k->set) {
	int32 n = msg_FirstUnreadArt(k->set);
	if (n < k->first_possible || n > k->last_possible) {
	  /* We know we've gotten all there is to know.  Take advantage of that to
		 update our counts... */

	  if (context->msgframe->msgs) {
		int32 unread = 0;
		int32 total = 0;

		msg_count_unread_and_total(context->msgframe->msgs->msgs,
								   &unread, &total);
		if ((context->msgframe->opened_folder->total_messages < total) ||
			(context->msgframe->all_messages_visible_p &&
			 /* If we're inside the "partial" timer handler hack, don't
				update the "total" count since the current value in the
				folder is definitely more correct than the value we would
				give it. */
			 !partialDoit)) {
		  context->msgframe->opened_folder->total_messages = total;
		}
		context->msgframe->opened_folder->data.newsgroup.last_message =
		  k->last_possible;
		msg_UpdateNewsFolderMessageCount(context,
										 context->msgframe->opened_folder,
										 unread, FALSE);
	  }
	}
  }

  msg_EnableUpdates(context);
  return 0;
}


#ifdef _USRDLL
/* #### This isn't really how it should be done - it should be a stream.
 */
struct mail_FolderData *
MBOX_ParseFile (MWContext* context, XP_File file)
{
    struct mail_FolderData *result = 0;
    struct msg_FolderParseState *state = 0;
    int status = 0;
    char *ibuffer = 0;
    uint32 ibuffer_size = 0;
    uint32 ibuffer_fp = 0;

    int size = 10240;
    char *buffer = (char *) XP_ALLOC (size);

    if(! buffer)
        goto FAIL;

    state = msg_BeginParsingFolder (context, NULL, 0);
    if(! state)
        goto FAIL;

    do {
        status = XP_FileRead (buffer, size, file);
        if(status < 0)
            goto FAIL; /* yuck */

        status = msg_ParseFolderLine(buffer, size, state);

    } while(status > 0);

    result = msg_DoneParsingFolder (state,
									MSG_SortByMessageNumber, TRUE, FALSE,
									FALSE);
    state = NULL;

FAIL:
    FREEIF (buffer);
    FREEIF (ibuffer);
    if(state)
        mbox_free_state (state);
    return result;
}



#endif /* _USRDLL */




/* Figure out how many unseen and valid messages there are. */

static void
msg_recalculate_totals(struct mail_FolderData* data)
{
  uint32 num_valid = data->message_count;
  uint32 num_unread = 0;
  uint32 count = 0;
  int i, n;
  MSG_ThreadEntry* msg;
  MSG_ThreadEntry** array;
  for (n=0 ; n<data->message_count ; n += mail_MSG_ARRAY_SIZE) {
	int max = data->message_count - n < mail_MSG_ARRAY_SIZE ?
	  data->message_count - n : mail_MSG_ARRAY_SIZE;
	array = data->msglist[n / mail_MSG_ARRAY_SIZE];
	for (i=0 ; i<max ; i++) {
	  msg = array[i];
	  if (msg->flags & MSG_FLAG_EXPUNGED) {
		count += msg->data.mail.byte_length;
		num_valid--;
	  } else if (!(msg->flags & MSG_FLAG_READ)) {
		num_unread++;
	  }
	}
  }
  data->unread_messages = num_unread;
  data->total_messages = num_valid;
  data->expunged_bytes = count;
}






static char magic[] = mail_SUMMARY_MAGIC_NUMBER;



/* Given a folder, write a summary file.
   This does blocking writes.
 */
int
msg_WriteFolderSummaryFile (struct mail_FolderData *data, XP_File output)
{
  unsigned char *buf = 0;
  uint32 buf_fp = 0;
  uint32 buf_size = 0;
  int status;
  uint32 table_size = 0;
  uint32 table_bytes = 0;
  char **table;
  MSG_ThreadEntry* msg;
  MSG_ThreadEntry** array;
  uint32 refs_size = 0;
  uint16 *refs;
  uint32 flags;
  int i, n;  

  XP_ASSERT (data);
  if (! data) return -1;

  table = data->string_table;

# define msg_grow_buf(desired_size) \
  (((desired_size) >= buf_size) ? \
   msg_GrowBuffer ((desired_size), sizeof(*buf), 1024, \
				   (char **) &buf, &buf_size) \
   : 0)

  status = msg_grow_buf (1024);
  if (status < 0) return status;

  /* Write magic number
   */
  buf_fp = XP_STRLEN (magic);
  XP_MEMCPY (buf, magic, buf_fp);

  /* Write folder format version number.
   */
  buf[buf_fp++] = (mail_SUMMARY_VERSION >> 24) & 0xFF;
  buf[buf_fp++] = (mail_SUMMARY_VERSION >> 16) & 0xFF;
  buf[buf_fp++] = (mail_SUMMARY_VERSION >>  8) & 0xFF;
  buf[buf_fp++] = (mail_SUMMARY_VERSION      ) & 0xFF;

  /* Write folder size and date.
   */
  buf[buf_fp++] = (data->foldersize >> 24) & 0xFF;
  buf[buf_fp++] = (data->foldersize >> 16) & 0xFF;
  buf[buf_fp++] = (data->foldersize >>  8) & 0xFF;
  buf[buf_fp++] = (data->foldersize      ) & 0xFF;

  buf[buf_fp++] = (((uint32) data->folderdate) >> 24) & 0xFF;
  buf[buf_fp++] = (((uint32) data->folderdate) >> 16) & 0xFF;
  buf[buf_fp++] = (((uint32) data->folderdate) >>  8) & 0xFF;
  buf[buf_fp++] = (((uint32) data->folderdate)      ) & 0xFF;

  /* Write the "parsed_thru" number.  Since we know we've parsed everything,
	 this is the same as the folder size. */

  buf[buf_fp++] = (data->foldersize >> 24) & 0xFF;
  buf[buf_fp++] = (data->foldersize >> 16) & 0xFF;
  buf[buf_fp++] = (data->foldersize >>  8) & 0xFF;
  buf[buf_fp++] = (data->foldersize      ) & 0xFF;

  msg_recalculate_totals(data);

  buf[buf_fp++] = (data->message_count >> 24) & 0xFF;
  buf[buf_fp++] = (data->message_count >> 16) & 0xFF;
  buf[buf_fp++] = (data->message_count >>  8) & 0xFF;
  buf[buf_fp++] = (data->message_count      ) & 0xFF;

  buf[buf_fp++] = (data->total_messages >> 24) & 0xFF;
  buf[buf_fp++] = (data->total_messages >> 16) & 0xFF;
  buf[buf_fp++] = (data->total_messages >>  8) & 0xFF;
  buf[buf_fp++] = (data->total_messages      ) & 0xFF;

  buf[buf_fp++] = (data->unread_messages >> 24) & 0xFF;
  buf[buf_fp++] = (data->unread_messages >> 16) & 0xFF;
  buf[buf_fp++] = (data->unread_messages >>  8) & 0xFF;
  buf[buf_fp++] = (data->unread_messages      ) & 0xFF;

  buf[buf_fp++] = (data->expunged_bytes >> 24) & 0xFF;
  buf[buf_fp++] = (data->expunged_bytes >> 16) & 0xFF;
  buf[buf_fp++] = (data->expunged_bytes >>  8) & 0xFF;
  buf[buf_fp++] = (data->expunged_bytes      ) & 0xFF;

  if ((table = data->string_table))
	for (; *table; table++)
	  {
		table_bytes += XP_STRLEN (*table) + 1;
		table_size++;
	  }
  XP_ASSERT (table_size <= 0xFFFF);

  /* Write string table length
   */
  buf[buf_fp++] = (table_size >> 8) & 0xFF;
  buf[buf_fp++] = (table_size     ) & 0xFF;

  /* Write string table entries (null-terminated)
   */
  if ((table = data->string_table))
   for (; *table; table++)
	{
	  uint32 L = XP_STRLEN (*table);

	  XP_ASSERT ((L == 0) == (table == data->string_table)); /* only one "" */
	  status = msg_grow_buf (buf_fp + L + 10);
	  if (status < 0) goto FAIL;

	  XP_MEMCPY (buf + buf_fp, *table, L + 1);
	  buf_fp += L + 1;

	  if (buf_fp > 1024)
		{
		  /* The buffer is getting big; write out what we've got so far.
		   */
		  if (XP_FileWrite ((void*)buf, buf_fp, output) < buf_fp) {
			status = MK_MIME_ERROR_WRITING_FILE;
			goto FAIL;
		  }
		  buf_fp = 0;
		}
	}

  /* Write each message description
   */

  msg = NULL;
  for (n=0 ; n<data->message_count ; n += mail_MSG_ARRAY_SIZE) {
	int max = data->message_count - n < mail_MSG_ARRAY_SIZE ?
	  data->message_count - n : mail_MSG_ARRAY_SIZE;
	array = data->msglist[n / mail_MSG_ARRAY_SIZE];
	for (i=0 ; i<max ; i++) {
	  XP_ASSERT(msg == NULL ||
				msg->data.mail.file_index < array[i]->data.mail.file_index);
	  msg = array[i];
	  
	  XP_ASSERT(!(msg->flags & MSG_FLAG_EXPIRED));

	  refs_size = 0;
	  if (msg->references)
		for (refs = msg->references; *refs; refs++)
		  refs_size++;
	  XP_ASSERT (refs_size <= 0xFFFF);

	  if ((buf_fp + (refs_size * 2) + 100) > buf_size) {
		msg_grow_buf((buf_fp + (refs_size * 2) + 100));
	  }

	  buf[buf_fp++] = (msg->sender      >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->sender           ) & 0xFF;

	  buf[buf_fp++] = (msg->recipient   >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->recipient        ) & 0xFF;

	  buf[buf_fp++] = (msg->subject     >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->subject          ) & 0xFF;

	  buf[buf_fp++] = (msg->date        >> 24) & 0xFF;
	  buf[buf_fp++] = (msg->date        >> 16) & 0xFF;
	  buf[buf_fp++] = (msg->date        >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->date             ) & 0xFF;

	  flags = msg->flags & ~MSG_FLAG_RUNTIME_ONLY;

	  buf[buf_fp++] = (flags            >> 24) & 0xFF;
	  buf[buf_fp++] = (flags            >> 16) & 0xFF;
	  buf[buf_fp++] = (flags            >>  8) & 0xFF;
	  buf[buf_fp++] = (flags                 ) & 0xFF;

	  buf[buf_fp++] = (msg->data.mail.file_index  >> 24) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.file_index  >> 16) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.file_index  >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.file_index       ) & 0xFF;

	  buf[buf_fp++] = (msg->data.mail.byte_length >> 24) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.byte_length >> 16) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.byte_length >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.byte_length      ) & 0xFF;

	  XP_ASSERT(msg->data.mail.status_offset < 10000); /* ### Debugging hack */

	  buf[buf_fp++] = (msg->data.mail.status_offset >> 8) & 0xFF;
	  buf[buf_fp++] = (msg->data.mail.status_offset     ) & 0xFF;

	  buf[buf_fp++] = (msg->lines       >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->lines            ) & 0xFF;

	  XP_ASSERT(msg->id > 0);

	  buf[buf_fp++] = (msg->id          >>  8) & 0xFF;
	  buf[buf_fp++] = (msg->id               ) & 0xFF;

	  buf[buf_fp++] = (refs_size         >> 8) & 0xFF;
	  buf[buf_fp++] = (refs_size             ) & 0xFF;

	  if (msg->references) {
		for (refs = msg->references; *refs; refs++) {
		  buf[buf_fp++] = (*refs       >> 8) & 0xFF;
		  buf[buf_fp++] = (*refs           ) & 0xFF;
		}
	  }

	  if (buf_fp > 1024) {
		/* The buffer is getting big; write out what we've got so far.
		 */
		status = XP_FileWrite ( (void*)buf, buf_fp, output);
		if (status != buf_fp)
		  {
			status = MK_MIME_ERROR_WRITING_FILE;
			goto FAIL;
		  }
		buf_fp = 0;
	  }
	}
  }

  if (buf_fp > 0)
	{
	  /* Flush last buffer.
	   */
	  status = XP_FileWrite ((void*)buf, buf_fp, output);
	  if (status != buf_fp)
		{
		  status = MK_MIME_ERROR_WRITING_FILE;
		  goto FAIL;
		}
	  buf_fp = 0;
	}

#undef msg_grow_buf

 FAIL:
  FREEIF (buf);
  return status;
}


#if defined(DEBUG) && defined(XP_UNIX)
# define DEBUG_SUMMARIES
#endif

#ifdef DEBUG_SUMMARIES
static XP_Bool msg_dump_summaries = FALSE;
#endif


/* Read a summary file and return a folder object, or 0.
   This does blocking reads.
 */
struct mail_FolderData *
msg_ReadFolderSummaryFile (MWContext* context, XP_File input,
						   const char* pathname, XP_Bool* needs_save)
{
  struct mail_FolderData *data = 0;
  int32 i;
  uint32 L;
  uint32 parsed_thru;
  unsigned char *in = 0;
  unsigned char *buf = 0;
  uint32 buf_fp = 0;
  uint32 buf_size = 0;

  unsigned char *line_buf = 0;
  uint32 line_buf_fp = 0;
  uint32 line_buf_size = 0;

  int status = 0;
  uint32 table_size = 0;

  int num;

  XP_Bool hide_read_msgs =
	context && context->msgframe && !context->msgframe->all_messages_visible_p;

  XP_File fid = NULL;

  char* ibuffer = 0;
  uint32 ibuffer_size = 0;
  uint32 ibuffer_fp = 0;


# define msg_grow_buf(desired_size) \
  (((desired_size) >= buf_size) ? \
   msg_GrowBuffer ((desired_size), sizeof(*buf), 1024, \
				   (char **) &buf, &buf_size) \
   : 0)

# define msg_grow_line_buf(desired_size) \
  (((desired_size) >= line_buf_size) ? \
   msg_GrowBuffer ((desired_size), sizeof(*line_buf), 1024, \
				   (char **) &line_buf, &line_buf_size) \
   : 0)

# define GET_N_BYTES(N)											\
	do {														\
	  unsigned char *end = buf + buf_fp;						\
	  unsigned char *out = line_buf + line_buf_fp;				\
	  while (in < end && line_buf_fp < N)						\
		*out++ = *in++, line_buf_fp++;							\
	  if (line_buf_fp == N)										\
		{														\
		  line_buf_fp = 0;										\
		  break;												\
		}														\
	  /* get some more. */										\
	  status = XP_FileRead (buf, buf_size, input);				\
	  if (status <= 0) {										\
	    PRINTF(("Unexpectedly hit end-of-file.\n")); 			\
		goto FAIL;												\
	  }														\
	  buf_fp = status;											\
	  in = buf;													\
	  status = msg_grow_line_buf (line_buf_fp + buf_fp);		\
	  if (status < 0) goto FAIL;								\
	} while (1)

# define GET_SHORT(SLOT)													\
	do {																	\
	  GET_N_BYTES(2);														\
	  SLOT = ((((uint16) line_buf[0]) << 8) | ((uint16) line_buf[1]));		\
		} while (0)

# define GET_LONG(SLOT)														\
	do {																	\
	  GET_N_BYTES(4);														\
	  SLOT = ((((uint32) line_buf[0])<<24) | (((uint32) line_buf[1])<<16) |	\
			  (((uint32) line_buf[2])<< 8) |  ((uint32) line_buf[3]));		\
		} while (0)

  if (needs_save) *needs_save = FALSE;

  data = XP_NEW (struct mail_FolderData);
  if (! data) goto FAIL;

  XP_MEMSET (data, 0, sizeof(*data));

  /* Figure out how big the blocks we're allocating is.
	 This is sleazy as hell, but lets us save 4 bytes per message in
	 newsgroups versus mail folders.
	 Similar code is in msg_BeginParsingFolder().
   */
  {
	static struct MSG_ThreadEntry dummy;
	uint32 mail_size = ((uint8 *) &dummy.data.mail.sleazy_end_of_data -
						(uint8 *) &dummy);
#ifndef NDEBUG
	uint32 news_size = ((uint8 *) &dummy.data.news.sleazy_end_of_data -
						(uint8 *) &dummy);
	/* if this isn't actually doing anything, I'd like to know that... */
	XP_ASSERT (news_size < mail_size);
#endif
	XP_InitAllocStructInfo (&data->msg_blocks, mail_size);
  }

  /* Try to get a big buffer for reads. */
  status = msg_grow_buf(1024 * 20);
  if (status < 0) status = msg_grow_buf(1024 * 10);
  if (status < 0) status = msg_grow_buf(1024 * 5);
  if (status < 0) status = msg_grow_buf(1024);
  if (status < 0) status = msg_grow_buf(512);
  if (status < 0) goto FAIL;

  in = buf;

  L = XP_STRLEN (magic);

  /* Check magic number
   */
  GET_N_BYTES (L);
  if (XP_STRNCMP ((char*)line_buf, magic, L)) {
	PRINTF(("Bad magic at beginning of summary file.\n"));
	goto FAIL;
  }

  /* Read and check version number.
   */
  GET_LONG (L);
  if (L != mail_SUMMARY_VERSION) {
	PRINTF(("Wrong version for summary file.\n"));
	goto FAIL; /* #### */
  }

  /* Read and note the folder size and date.
   */
  GET_LONG (L);
  data->foldersize = L;

  GET_LONG (L);
  data->folderdate = (time_t) L;

  GET_LONG(parsed_thru);

  GET_LONG (L);
  data->message_count = L;

  GET_LONG (L);			/* Number of valid messages. */
  data->total_messages = L;

  GET_LONG (L);			/* Number of unread messages. */
  data->unread_messages = L;

  GET_LONG (L);
  data->expunged_bytes = L;

  /* Read string table length
   */
  GET_SHORT (table_size);

  data->string_table = (char **) XP_ALLOC ((table_size + 1) * sizeof (char *));
  if (!data->string_table)
	goto FAIL;
  XP_MEMSET (data->string_table, 0, (table_size + 1) * sizeof (char *));

#ifdef DEBUG_SUMMARIES
  if (msg_dump_summaries)
	{
	  fprintf (stderr, "\n");
	  fprintf (stderr, "        Name: %s\n", pathname);
	  fprintf (stderr, " Folder Size: %lu\n", data->foldersize);
	  fprintf (stderr, " Folder Date: %s", ctime (&data->foldersize));
	  fprintf (stderr, " Parsed Thru: %lu\n", parsed_thru);
	  fprintf (stderr, "   Msg Count: %d\n", data->message_count);
	  fprintf (stderr, "  Total Msgs: %lu\n", data->total_messages);
	  fprintf (stderr, " Unread Msgs: %lu\n", data->unread_messages);
	  fprintf (stderr, "   Exp Bytes: %lu\n", data->expunged_bytes);
	  fprintf (stderr, "  Table Size: %lu\n", table_size);
	}
#endif /* DEBUG_SUMMARIES */


  /* Read string table entries (null-terminated)
   */
  for (i = 0; i < table_size; i++)
	{
	  /* Read a null-terminated string into line_buf.
	   */
	  do {
		unsigned char *end = buf + buf_fp;
		unsigned char *out = line_buf + line_buf_fp;
		while (in < end && *in)
		  *out++ = *in++, line_buf_fp++;
		if (in < end)
		  {
			XP_ASSERT (!*in);
			in++;
			line_buf_fp = 0;
			*out++ = 0;
			break;
		  }
		/* get some more. */
		status = XP_FileRead (buf, buf_size, input);
		if (status <= 0) {
		  PRINTF(("Unexpectedly hit end-of-file.\n"));
		  goto FAIL;
		}
		buf_fp = status;
		in = buf;
		status = msg_grow_line_buf (line_buf_fp + buf_fp);
		if (status < 0) goto FAIL;
	  } while (1);

	  if ((i == 0) != (*line_buf == 0))
		{
		  PRINTF(("Found extra \"\" in string table.  Discarding summary.\n"));
		  goto FAIL;
		}

	  data->string_table [i] = XP_STRDUP ((char*)line_buf);
	  if (! data->string_table [i])
		goto FAIL;

#ifdef DEBUG_SUMMARIES
  if (msg_dump_summaries)
	{
	  fprintf (stderr, "        %4lu: \"%s\"\n", i, data->string_table[i]);
	}
#endif /* DEBUG_SUMMARIES */


#if 0 /* This check is interesting, but way too slow... */
#if defined(DEBUG) && !defined(NDEBUG)
	  {
		/* Do a little sanity checking to make sure duplicate strings
		   haven't found their way into the string table.
		 */
		int32 j;
		for (j = 0; j < i; j++)
		  {
			XP_ASSERT(data->string_table[j] &&
					  XP_STRCMP(data->string_table[j],
								data->string_table[i]));
		  }
	  }
#endif /* DEBUG && !NDEBUG */
#endif /* 0 */

	}
  data->string_table [i] = 0;

  /* Make the ID table. */
  data->message_id_table =
	XP_HashTableNew (data->message_count * 2, XP_StringHash, mbox_cmp);
  if (! data->message_id_table)
	{
	  /* Ok, try a smaller one. */
	  data->message_id_table =
		XP_HashTableNew (500, XP_StringHash, mbox_cmp);
	  if (! data->message_id_table)
		goto FAIL;
	}

  /* Allocate the bizarre message list structure. */
  num = (data->message_count / mail_MSG_ARRAY_SIZE) + 1;
  data->msglist = (MSG_ThreadEntry***)
	XP_ALLOC(num * sizeof(MSG_ThreadEntry**));
  if (!data->msglist) {
	goto FAIL;
  }
  for (i=0 ; i<num ; i++) {
	data->msglist[i] = (MSG_ThreadEntry**)
	  XP_ALLOC(mail_MSG_ARRAY_SIZE * sizeof(MSG_ThreadEntry*));
	if (!data->msglist[i]) {
	  for (i-- ; i>=0 ; i--) {
		XP_FREE(data->msglist[i]);
	  }
	  XP_FREE(data->msglist);
	  data->msglist = NULL;
	  goto FAIL;
	}
  }

#ifdef DEBUG_SUMMARIES
  if (msg_dump_summaries) fprintf (stderr, "\n");
#endif /* DEBUG_SUMMARIES */


  /* Read message descriptions
   */
  for (i = 0; i < data->message_count; i++)
	{
	  uint16 rlength;
	  struct MSG_ThreadEntry *msg = (struct MSG_ThreadEntry *)
		XP_AllocStructZero (&data->msg_blocks);
	  if (! msg) goto FAIL;
	  
	  GET_SHORT (msg->sender);
	  GET_SHORT (msg->recipient);
	  GET_SHORT (msg->subject);
	  GET_LONG  (msg->date);
	  GET_LONG  (msg->flags);
	  msg->flags &= ~MSG_FLAG_RUNTIME_ONLY;
	  GET_LONG  (msg->data.mail.file_index);
	  GET_LONG  (msg->data.mail.byte_length);
	  GET_SHORT (msg->data.mail.status_offset);
	  GET_SHORT (msg->lines);
	  GET_SHORT (msg->id);
	  GET_SHORT (rlength);

#ifdef DEBUG_SUMMARIES
	  if (msg_dump_summaries)
		{
		  fprintf (stderr, "        %4lu:  From: %4d: %s\n",
				   i, msg->sender, data->string_table[msg->sender]);
		  fprintf (stderr, "                 To: %4d: %s\n",
				   msg->recipient, data->string_table[msg->recipient]);
		  fprintf (stderr, "               Subj: %4d: %s\n",
				   msg->subject, data->string_table[msg->subject]);
		  fprintf (stderr, "                 ID: %4d: %s\n",
				   msg->id, data->string_table[msg->id]);
		  fprintf (stderr, "               Date: %s", ctime(&msg->date));
		  fprintf (stderr, "              Flags: %04x\n", msg->flags);
		  fprintf (stderr, "              Index: %lu\n",
				   msg->data.mail.file_index);
		  fprintf (stderr, "             Length: %lu\n",
				   msg->data.mail.byte_length);
		  fprintf (stderr, "              StOff: %d\n",
				   msg->data.mail.status_offset);
		  fprintf (stderr, "              Lines: %d\n", msg->lines);
		  fprintf (stderr, "               Refs: %d\n", rlength);
		}
#endif /* DEBUG_SUMMARIES */

#if 0
	  XP_ASSERT(msg->data.mail.status_offset < 10000); /* ### Debugging hack */
#endif

	  if (! (table_size > msg->sender &&
			 table_size > msg->recipient &&
			 table_size > msg->subject &&
			 table_size > msg->id))
		{
		  /* Summary badly corrupted... */
		  XP_ASSERT (0);
		  goto FAIL;
		}

	  if (!(msg->flags & MSG_FLAG_EXPUNGED)) {
		XP_ASSERT(*(data->string_table [msg->id]));
		status = XP_Puthash (data->message_id_table,
							 (void *) (data->string_table [msg->id]),
							 (void *) msg);
		if (status < 0) goto FAIL;
	  }

	  /* Now we need to read the references (rlength of them.)
	   */
	  if (rlength > 0)
		{
		  int j;
		  msg->references = (uint16 *) XP_ALLOC ((rlength+1) * sizeof(uint16));
		  if (! msg->references) goto FAIL;
		  for (j = 0; j < rlength; j++)
			{
			  GET_SHORT (msg->references[j]);
			  XP_ASSERT (table_size > msg->references[j]);
#ifdef DEBUG_SUMMARIES
			  if (msg_dump_summaries)
				{
				  fprintf (stderr, "               %4d: %4d: %s\n",
						   j, msg->references[j],
						   data->string_table[msg->references[j]]);
				}
#endif /* DEBUG_SUMMARIES */
			}
		  msg->references[j] = 0;
		}

	  data->msglist[i / mail_MSG_ARRAY_SIZE][i % mail_MSG_ARRAY_SIZE] = msg;
	  if (msg->flags & MSG_FLAG_EXPUNGED) {
		msg->next = data->expungedmsgs;
		data->expungedmsgs = msg;
	  } else if (hide_read_msgs && msg->flags & MSG_FLAG_READ) {
		msg->next = data->hiddenmsgs;
		data->hiddenmsgs = msg;
	  } else {
		msg->next = data->msgs;
		data->msgs = msg;
	  }
	}

# undef msg_grow_buf
# undef msg_grow_line_buf
# undef GET_N_BYTES
# undef GET_SHORT
# undef GET_LONG

  if (parsed_thru < data->foldersize) {
	XP_StatStruct folderst;
	if (parsed_thru == 0) {
	  goto FAIL;	/* Might as well let the caller parse the whole folder
					   instead of us doing it here; they might actually use
					   streams.  Also, we probably haven't initialized the
					   string table and things into the exact right empty state
					   that the folderparse code wants.*/
	}
	if (!XP_Stat((char*) pathname, &folderst, xpMailFolder)) {
	  if (folderst.st_size == data->foldersize &&
		  folderst.st_mtime == data->folderdate) {
		fid = XP_FileOpen(pathname, xpMailFolder, XP_FILE_READ_BIN);
		if (fid) {
		  struct msg_FolderParseState* state;
#ifdef DEBUG_terry
		  PRINTF(("Folder only partially parsed.  Had %d messages...\n",
				  data->message_count));
#endif /* DEBUG_terry */
		  XP_FileSeek(fid, parsed_thru, SEEK_SET);
		  state = msg_BeginParsingFolder(context, data, parsed_thru);
		  if (!state)
			{
			  status = -1;
			  goto FAIL;
			}
		  for (;;) {
			status = XP_FileRead(buf, buf_size, fid);
			if (status < 0) goto FAIL;
			if (status == 0) break;
			status = msg_LineBuffer((char*)buf, status, &ibuffer, &ibuffer_size,
									&ibuffer_fp, FALSE,
									msg_ParseFolderLine, state);
			if (status < 0) goto FAIL;
		  }
		  if (ibuffer_fp > 0) {
			/* Flush out any partial line. */
			msg_ParseFolderLine(ibuffer, ibuffer_fp, state);
			ibuffer_fp = 0;
		  }
		  data = msg_DoneParsingFolder(state, MSG_SortByMessageNumber, TRUE,
									   FALSE, FALSE);
		  if (!data) goto FAIL;
#ifdef DEBUG_terry
		  PRINTF(("... now have %d messages.\n", data->message_count));
#endif /* DEBUG_terry */
		  if (needs_save) *needs_save = TRUE;

		  /* The parsing code updates the totals on the data, but we should
			 already have numbers that include the newly parsed messages.  So,
			 now our totals are screwed up, so we just go count them again. */
		  msg_recalculate_totals(data);
		}
	  }
	}
  }
			


  goto CLEANUP;

 FAIL:
  if (data && !data->msglist) {
	/* We lost so early that we might as well not bother to keep anything. */
	msg_FreeFolderData(data);
	data = NULL;
  }
  if (data) {
	data->folderdate = 0;			/* Let the calling code know that this */
									/* data really doesn't match the folder */
									/* data. But we return what we did have, */
									/* so that the caller can try to extract */
									/* some data from it. */
	PRINTF(("Couldn't quite parse the summary file; using what we can...\n"));
  }

CLEANUP:
  if (fid) XP_FileClose(fid);
  FREEIF (buf);
  FREEIF (line_buf);
  FREEIF (ibuffer);

  return data;
}


#ifdef DEBUG
static char lastquery_path[512];
static XP_Bool lastquery_result = FALSE;
#endif

#define GET_LONG() ((((uint32) ptr[0])<<24) | (((uint32) ptr[1])<<16) |	\
(((uint32) ptr[2])<< 8) | ((uint32) ptr[3]))


XP_Bool
msg_IsSummaryValid(const char* pathname, XP_StatStruct* folderst)
{
  XP_Bool result = FALSE;
  XP_File fid = XP_FileOpen(pathname, xpMailFolderSummary, XP_FILE_READ_BIN);
  if (fid) {
	int mlength = XP_STRLEN(magic);
	int length = mlength + 4 + 4 + 4;
	unsigned char * buf = (unsigned char*)XP_ALLOC(length);
	if (buf) {
	  int n = XP_FileRead(buf, length, fid);
	  if (n == length && XP_STRNCMP((char*)buf, (char*)magic, mlength) == 0) {
		unsigned char * ptr = buf + mlength;
		if (GET_LONG() == mail_SUMMARY_VERSION) {
		  ptr += 4;
		  if (GET_LONG() == folderst->st_size) {
			ptr += 4;
			if (GET_LONG() == folderst->st_mtime) {
			  result = TRUE;
			}
		  }
		}
	  }
	  XP_FREE(buf);
	}
	XP_FileClose(fid);
  }
  if (!result && folderst->st_size == 0) {
	/* OK, it doesn't have a valid summary file, but it has no data either, and
       so it is trivial to make a valid summary file.  Just do it. */
	fid = XP_FileOpen((char*) pathname, xpMailFolderSummary,
					  XP_FILE_WRITE_BIN);
	if (fid) {
	  mail_FolderData* data = XP_NEW_ZAP(mail_FolderData);
	  if (data) {
		data->foldersize = 0;
		data->folderdate = folderst->st_mtime;

#ifdef XP_UNIX
		/* Clone the permissions of the folder file into the summary
		   file (except make sure it's readable/writable by owner.)
		   Ignore errors; if it fails, bummer. */
		fchmod (fileno(fid), (folderst->st_mode | S_IRUSR | S_IWUSR));
#endif /* XP_UNIX */

		if (msg_WriteFolderSummaryFile(data, fid) >= 0) {
		  result = TRUE;
		}
		msg_FreeFolderData ( data );
	  }
	  XP_FileClose(fid);
	}
  }
#ifdef DEBUG
  XP_STRCPY(lastquery_path, pathname);
  lastquery_result = result;
#endif
  return result;
}


void
msg_SetSummaryValid(const char* pathname, int num, int numunread)
{
  XP_StatStruct folderst;
  MWContext* context;
  MSG_Frame* f;
  char* buffer;
  char* ibuffer = 0;
  uint32 ibuffer_size = 0;
  uint32 ibuffer_fp = 0;
  uint32 position;
  int l;
  struct msg_FolderParseState* state;
  MSG_ThreadEntry* lastmsg;
  MSG_Folder* folder;

#ifdef DEBUG
  XP_ASSERT(lastquery_result && XP_FILENAMECMP(pathname, lastquery_path) == 0);
#endif

  if (!XP_Stat((char*) pathname, &folderst, xpMailFolder)) {
	XP_File fid = XP_FileOpen((char*) pathname, xpMailFolderSummary,
							  XP_FILE_UPDATE_BIN);
	if (fid) {
	  unsigned char buf[8];
	  unsigned char * ptr = buf;
	  XP_FileSeek(fid, XP_STRLEN(magic) + 4, SEEK_SET);
	  *ptr++ = (folderst.st_size >> 24) & 0xFF;
	  *ptr++ = (folderst.st_size >> 16) & 0xFF;
	  *ptr++ = (folderst.st_size >>  8) & 0xFF;
	  *ptr++ = (folderst.st_size	  ) & 0xFF;
	  *ptr++ = (folderst.st_mtime >> 24) & 0xFF;
	  *ptr++ = (folderst.st_mtime >> 16) & 0xFF;
	  *ptr++ = (folderst.st_mtime >>  8) & 0xFF;
	  *ptr++ = (folderst.st_mtime	   ) & 0xFF;
	  XP_FileWrite(buf, 8, fid);
	  position = XP_STRLEN(magic) + (5 * 4);
	  XP_FileSeek(fid, position, SEEK_SET);
	  if (XP_FileRead(buf, 8, fid) == 8) {
		ptr = buf;
		num += GET_LONG();
		if (num < 0) num = 0; /* can this ever happen? */
		ptr += 4;
		numunread += GET_LONG();
		if (numunread < 0) numunread = 0;
		ptr = buf;
		*ptr++ = (num >> 24) & 0xFF;
		*ptr++ = (num >> 16) & 0xFF;
		*ptr++ = (num >>  8) & 0xFF;
		*ptr++ = (num	   ) & 0xFF;
		*ptr++ = (numunread >> 24) & 0xFF;
		*ptr++ = (numunread >> 16) & 0xFF;
		*ptr++ = (numunread >>  8) & 0xFF;
		*ptr++ = (numunread		 ) & 0xFF;
		XP_FileSeek(fid, position, SEEK_SET);
		XP_FileWrite(buf, 8, fid);
	  }
	  XP_FileClose(fid); 
	  context = XP_FindContextOfType(0, MWContextMail);
	  if (context == NULL) return;
	  f = context->msgframe;
	  if (f == NULL) return;
	  folder = msg_FindMailFolder(context, pathname, FALSE);
	  if (folder == NULL) return;
	  folder->unread_messages = numunread;
	  folder->total_messages = num;
	  msg_RedisplayOneFolder(context, folder, -1, -1, FALSE);

	  if (f->msgs == NULL || f->opened_folder != folder) return;

	  /* Oh boy, the folder that just got stuff appended to it is the folder
		 that the user is looking at.  Try to parse the new messages and
		 display them. */

	  fid = XP_FileOpen(pathname, xpMailFolderSummary, XP_FILE_READ_BIN);
	  if (!fid) return;
	  XP_FileSeek(fid, XP_STRLEN(magic) + 4 + 4 + 4, SEEK_SET);
	  XP_FileRead(buf, 4, fid);
	  XP_FileClose(fid);
	  position = ((((uint32) buf[0])<<24) | (((uint32) buf[1])<<16) |
				  (((uint32) buf[2])<< 8) | ((uint32) buf[3]));
	  l = f->msgs->message_count - 1;
	  lastmsg =
		f->msgs->msglist[l / mail_MSG_ARRAY_SIZE][l % mail_MSG_ARRAY_SIZE];
	  if (position !=
		  lastmsg->data.mail.file_index + lastmsg->data.mail.byte_length) {
		return;
	  }
	  fid = XP_FileOpen(pathname, xpMailFolder, XP_FILE_READ_BIN);
	  if (!fid) return;
	  buffer = (char*)XP_ALLOC(512);
	  if (buffer) {
		XP_FileSeek(fid, position, SEEK_SET);
		state = msg_BeginParsingFolder(context, f->msgs, position);
		if (state) {
		  for (;;) {
			l = XP_FileRead(buffer, 512, fid);
			if (l <= 0) break;
			l = msg_LineBuffer(buffer, l, &ibuffer, &ibuffer_size, &ibuffer_fp,
							   FALSE, msg_ParseFolderLine, state);
			if (l < 0) break;
		  }
		  if (l == 0) {
			if (ibuffer_fp > 0) {
			  msg_ParseFolderLine(ibuffer, ibuffer_fp, state);
			  ibuffer_fp = 0;
			}
			msg_DoneParsingFolder(state, MSG_SortByMessageNumber, TRUE, FALSE,
								  FALSE);
			f->msgs->folderdate = folderst.st_mtime;
			f->msgs->foldersize = folderst.st_size;
			msg_SelectedMailSummaryFileChanged(context, NULL);
		  }
		  msg_RedisplayMessages(context);
		  msg_UpdateToolbar(context);
		}
		XP_FREE(buffer);
		FREEIF(ibuffer);
	  }
	  XP_FileClose(fid);
	}
  }
}



XP_Bool
msg_GetSummaryTotals(const char* pathname, uint32* unread, uint32* total,
					 uint32* wasted, uint32* bytes)
{
  XP_StatStruct folderst;
  XP_Bool result = FALSE;
  XP_File fid;
  if (XP_Stat((char*) pathname, &folderst, xpMailFolder) ||
	  folderst.st_size == 0) {
	if (unread) *unread = 0;
	if (total) *total = 0;
	if (wasted) *wasted = 0;
	if (bytes) *bytes = 0;
	return TRUE;
  }
  fid = XP_FileOpen(pathname, xpMailFolderSummary, XP_FILE_READ_BIN);
  if (fid) {
	int mlength = XP_STRLEN(magic);
	int length = mlength + (8 * 4);
	unsigned char * buf = (unsigned char*)XP_ALLOC(length);
	if (buf) {
	  int n = XP_FileRead(buf, length, fid);
	  if (n == length && XP_STRNCMP((char*)buf, magic, mlength) == 0) {
		unsigned char * ptr = buf + mlength;
		if (GET_LONG() == mail_SUMMARY_VERSION) {
		  ptr += 4;
		  if (GET_LONG() == folderst.st_size) {
			ptr += 4;
			if (GET_LONG() == folderst.st_mtime) {
			  result = TRUE;
			  ptr += 4 + 4 + 4;
			  if (total) {
				*total = GET_LONG();
			  }
			  ptr += 4;
			  if (unread) {
				*unread = GET_LONG();
			  }
			  ptr += 4;
			  if (wasted) {
				*wasted = GET_LONG();
			  }
			  if (bytes) {
				*bytes = (uint32) folderst.st_size;
			  }
			}
		  }
		}
	  }
	  XP_FREE(buf);
	}
	XP_FileClose(fid);
  }
  return result;
}


/* Rethread the messages in the given structure into the given order.  */
int
msg_RethreadInOrder(MWContext* context, struct mail_FolderData* data,
					MSG_SORT_KEY sort_key, XP_Bool sort_forward_p,
					XP_Bool thread_p)
{
  if (data && data->msgs) {
	struct MSG_ThreadEntry *newm = 
	  msg_RethreadMessages (context,
							data->msgs,
							data->message_count,
							data->string_table,
							FALSE,
							sort_key,
							sort_forward_p,
							thread_p,
							mbox_dummy_creator,
							mbox_dummy_destroyer,
							(void *) &data->msg_blocks);
	if (newm)
	  {
		data->msgs = newm;
		return 0;
	  }
	else
	  {
		return MK_OUT_OF_MEMORY;
	  }
  }
  return 0;
}





/* Rethread the messages currently loaded in the context.
 */
int
msg_Rethread (MWContext* context, struct mail_FolderData* data)
{
  return msg_RethreadInOrder(context, data, 
							 context->msgframe->sort_key,
							 context->msgframe->sort_forward_p,
							 context->msgframe->thread_p);
}



#if defined(DEBUG) && !defined(XP_WIN) && !defined(XP_MAC)

void
msg_print_thread_entry (struct MSG_ThreadEntry *msg, char **string_table,
						uint32 depth)
{
  int i;
  char subj [37];
  char *s = subj;
  int L;

/* XP_ASSERT (depth == msg->depth); */
  for (i = 0; i < depth; i++)
	fprintf (real_stdout, "  .");

  if (msg->flags & MSG_FLAG_HAS_RE)
	{
	  XP_STRCPY (subj, "Re: ");
	  s += 4;
	}
  L = XP_STRLEN (string_table [msg->subject]);
  if (L > ((subj + sizeof(subj) - 1) - s))
	L = ((subj + sizeof(subj) - 1) - s);
  XP_MEMCPY (s, string_table [msg->subject], L);
  s [L] = 0;

  if (msg->flags & MSG_FLAG_EXPIRED)
	{
	  fprintf (real_stdout, "                 |                 |     | %s\n", subj);
	}
  else
	{
	  char buf [255];
	  char date [20];
	  char *d = date;
	  char *ct = ctime (&msg->date);
	  *d++ = ct[11]; *d++ = ct[12]; *d++ = ct[13];
	  *d++ = ct[14]; *d++ = ct[15]; *d++ = ' ';
	  *d++ = ct[8]; *d++ = ct[9]; *d++ = '-';
	  *d++ = ct[4]; *d++ = ct[5]; *d++ = ct[6]; *d++ = '-'; 
	  *d++ = ct[22]; *d++ = ct[23];
	  *d++ = 0;
	  PR_snprintf (buf, sizeof(buf), "%-200s", string_table [msg->sender]);
	  if ((depth * 3) > 15)
		buf[0] = 0;
	  else
		buf [15 - (depth * 3)] = 0;
	  fprintf (real_stdout, " %s | %s | %3d | %s\n", buf, date, msg->lines, subj);
	}
}

static void
msg_print_thread (struct MSG_ThreadEntry *msg, char **string_table,
				  uint32 depth)
{
  while (msg)
	{
	  msg_print_thread_entry (msg, string_table, depth);
	  if (msg->first_child)
		msg_print_thread (msg->first_child, string_table, depth + 1);
	  msg = msg->next;
	}
}

static void
msg_compare_trees (struct MSG_ThreadEntry *a, struct MSG_ThreadEntry *b)
{
  uint16 *ra, *rb;
 RECURSE:
  XP_ASSERT (!a == !b);
  if (!a) return;
  XP_ASSERT (a->sender == b->sender);
  XP_ASSERT (a->recipient == b->recipient);
  XP_ASSERT (a->subject == b->subject);
  XP_ASSERT (a->lines == b->lines);
  XP_ASSERT (a->id == b->id);
  XP_ASSERT (a->flags == b->flags);
  XP_ASSERT (a->date == b->date);
  XP_ASSERT (a->data.mail.file_index == b->data.mail.file_index);
  XP_ASSERT (a->data.mail.byte_length == b->data.mail.byte_length);
  XP_ASSERT (!a->references == !b->references);
  XP_ASSERT (!a->first_child == !b->first_child);
  XP_ASSERT (!a->next == !b->next);
  ra = a->references;
  rb = b->references;
  if (ra)
	{
	  while (*ra)
		{
		  XP_ASSERT (*ra == *rb);
		  ra++;
		  rb++;
		}
	  XP_ASSERT (!*rb);
	}
  msg_compare_trees (a->first_child, b->first_child);
  a = a->next;
  b = b->next;
  goto RECURSE;
}

#endif /* DEBUG */
