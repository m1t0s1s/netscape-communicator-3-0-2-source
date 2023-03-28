/* -*- Mode: C; tab-width: 4 -*-
   msgutils.c --- various and sundry
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 6-Jun-95.
 */

#include "msg.h"
#include "mailsum.h"
#include "msgundo.h"
#include "xp_time.h"

extern int MK_OUT_OF_MEMORY;
#include "xplocale.h"

void
MSG_GetContextPrefs(MWContext *context,
					XP_Bool *all_headers_p,
					XP_Bool *micro_headers_p,
					XP_Bool *no_inline_p,
					XP_Bool *rot13_p,
					XP_Bool *wrap_long_lines_p)
{
  XP_ASSERT(context &&
			(context->type == MWContextMail ||
			 context->type == MWContextNews));
  *all_headers_p   = context->msgframe->all_headers_visible_p;
  *micro_headers_p = context->msgframe->micro_headers_p;
  *no_inline_p     = !context->msgframe->inline_attachments_p;
  *rot13_p         = context->msgframe->rot13_p;
  *wrap_long_lines_p = context->msgframe->wrap_long_lines_p;
}


int
msg_GetURL(MWContext* context, URL_Struct* url, XP_Bool issafe)
{
  XP_ASSERT(context);
  if (context && context->msgframe) {
	context->msgframe->safe_background_activity = issafe;
  }
  return FE_GetURL(context, url);
}


void
msg_InterruptContext(MWContext* context, XP_Bool safetoo)
{
  struct MSG_Frame *msg_frame;  
  if (!context) return;
  if (safetoo || !context->msgframe ||  
	  !context->msgframe->safe_background_activity) {
	/* save msg_frame in case context gets deleted on interruption */
	msg_frame = context->msgframe;
	XP_InterruptContext(context);
	if (msg_frame) {  
	  msg_frame->safe_background_activity = FALSE;
	}  
  }
}


/* Buffer management.
   Why do I feel like I've written this a hundred times before?
 */
int
msg_GrowBuffer (uint32 desired_size, uint32 element_size, uint32 quantum,
				char **buffer, uint32 *size)
{
  if (*size <= desired_size)
	{
	  char *new_buf;
	  uint32 increment = desired_size - *size;
	  if (increment < quantum) /* always grow by a minimum of N bytes */
		increment = quantum;

#ifdef TESTFORWIN16
	  if (((*size + increment) * (element_size / sizeof(char))) >= 64000)
		{
		  /* Make sure we don't choke on WIN16 */
		  XP_ASSERT(0);
		  return MK_OUT_OF_MEMORY;
		}
#endif /* DEBUG */

	  new_buf = (*buffer
				 ? (char *) XP_REALLOC (*buffer, (*size + increment)
										* (element_size / sizeof(char)))
				 : (char *) XP_ALLOC ((*size + increment)
									  * (element_size / sizeof(char))));
	  if (! new_buf)
		return MK_OUT_OF_MEMORY;
	  *buffer = new_buf;
	  *size += increment;
	}
  return 0;
}



/* Take the given buffer, tweak the newlines at the end if necessary, and
   send it off to the given routine.  We are guaranteed that the given
   buffer has allocated space for at least one more character at the end. */
static int
msg_convert_and_send_buffer(char* buf, int length, XP_Bool convert_newlines_p,
							int32 (*per_line_fn) (char *line,
												  uint32 line_length,
												  void *closure),
							void *closure)
{
  /* Convert the line terminator to the native form.
   */
  char* newline;

  XP_ASSERT(buf && length > 0);
  if (!buf || length <= 0) return -1;
  newline = buf + length;
  XP_ASSERT(newline[-1] == CR || newline[-1] == LF);
  if (newline[-1] != CR && newline[-1] != LF) return -1;

  if (!convert_newlines_p)
	{
	}
#if (LINEBREAK_LEN == 1)
  else if ((newline - buf) >= 2 &&
		   newline[-2] == CR &&
		   newline[-1] == LF)
	{
	  /* CRLF -> CR or LF */
	  buf [length - 2] = LINEBREAK[0];
	  length--;
	}
  else if (newline > buf + 1 &&
		   newline[-1] != LINEBREAK[0])
	{
	  /* CR -> LF or LF -> CR */
	  buf [length - 1] = LINEBREAK[0];
	}
#else
  else if (((newline - buf) >= 2 && newline[-2] != CR) ||
		   ((newline - buf) >= 1 && newline[-1] != LF))
	{
	  /* LF -> CRLF or CR -> CRLF */
	  length++;
	  buf[length - 2] = LINEBREAK[0];
	  buf[length - 1] = LINEBREAK[1];
	}
#endif

  return (*per_line_fn)(buf, length, closure);
}


/* SI::BUFFERED-STREAM-MIXIN
   Why do I feel like I've written this a hundred times before?
 */


int
msg_LineBuffer (const char *net_buffer, int32 net_buffer_size,
				char **bufferP, uint32 *buffer_sizeP, uint32 *buffer_fpP,
				XP_Bool convert_newlines_p,
				int32 (*per_line_fn) (char *line, uint32 line_length,
									  void *closure),
				void *closure)
{
  int status = 0;
  if (*buffer_fpP > 0 && *bufferP && (*bufferP)[*buffer_fpP - 1] == CR &&
	  net_buffer_size > 0 && net_buffer[0] != LF) {
	/* The last buffer ended with a CR.  The new buffer does not start
	   with a LF.  This old buffer should be shipped out and discarded. */
	XP_ASSERT(*buffer_sizeP > *buffer_fpP);
	if (*buffer_sizeP <= *buffer_fpP) return -1;
	status = msg_convert_and_send_buffer(*bufferP, *buffer_fpP,
										 convert_newlines_p,
										 per_line_fn, closure);
	if (status < 0) return status;
	*buffer_fpP = 0;
  }
  while (net_buffer_size > 0)
	{
	  const char *net_buffer_end = net_buffer + net_buffer_size;
	  const char *newline = 0;
	  const char *s;


	  for (s = net_buffer; s < net_buffer_end; s++)
		{
		  /* Move forward in the buffer until the first newline.
			 Stop when we see CRLF, CR, or LF, or the end of the buffer.
			 *But*, if we see a lone CR at the *very end* of the buffer,
			 treat this as if we had reached the end of the buffer without
			 seeing a line terminator.  This is to catch the case of the
			 buffers splitting a CRLF pair, as in "FOO\r\nBAR\r" "\nBAZ\r\n".
		   */
		  if (*s == CR || *s == LF)
			{
			  newline = s;
			  if (newline[0] == CR)
				{
				  if (s == net_buffer_end - 1)
					{
					  /* CR at end - wait for the next character. */
					  newline = 0;
					  break;
					}
				  else if (newline[1] == LF)
					/* CRLF seen; swallow both. */
					newline++;
				}
			  newline++;
			  break;
			}
		}

	  /* Ensure room in the net_buffer and append some or all of the current
		 chunk of data to it. */
	  {
		const char *end = (newline ? newline : net_buffer_end);
		uint32 desired_size = (end - net_buffer) + (*buffer_fpP) + 1;

		if (desired_size >= (*buffer_sizeP))
		  {
			status = msg_GrowBuffer (desired_size, sizeof(char), 1024,
									 bufferP, buffer_sizeP);
			if (status < 0) return status;
		  }
		XP_MEMCPY ((*bufferP) + (*buffer_fpP), net_buffer, (end - net_buffer));
		(*buffer_fpP) += (end - net_buffer);
	  }

	  /* Now *bufferP contains either a complete line, or as complete
		 a line as we have read so far.

		 If we have a line, process it, and then remove it from `*bufferP'.
		 Then go around the loop again, until we drain the incoming data.
	   */
	  if (!newline)
		return 0;

	  status = msg_convert_and_send_buffer(*bufferP, *buffer_fpP,
										   convert_newlines_p,
										   per_line_fn, closure);
	  if (status < 0) return status;

	  net_buffer_size -= (newline - net_buffer);
	  net_buffer = newline;
	  (*buffer_fpP) = 0;
	}
  return 0;
}


/* The opposite of msg_LineBuffer(): takes small buffers and packs them
   up into bigger buffers before passing them along.

   Pass in a desired_buffer_size 0 to tell it to flush (for example, in
   in the very last call to this function.)
 */
int
msg_ReBuffer (const char *net_buffer, int32 net_buffer_size,
			  uint32 desired_buffer_size,
			  char **bufferP, uint32 *buffer_sizeP, uint32 *buffer_fpP,
			  int32 (*per_buffer_fn) (char *buffer, uint32 buffer_size,
									  void *closure),
			  void *closure)
{
  int status = 0;

  if (desired_buffer_size >= (*buffer_sizeP))
	{
	  status = msg_GrowBuffer (desired_buffer_size, sizeof(char), 1024,
							   bufferP, buffer_sizeP);
	  if (status < 0) return status;
	}

  do
	{
	  uint32 size = *buffer_sizeP - *buffer_fpP;
	  if (size > net_buffer_size)
		size = net_buffer_size;
	  if (size > 0)
		{
		  XP_MEMCPY ((*bufferP) + (*buffer_fpP), net_buffer, size);
		  (*buffer_fpP) += size;
		  net_buffer += size;
		  net_buffer_size -= size;
		}

	  if (*buffer_fpP > 0 &&
		  *buffer_fpP >= desired_buffer_size)
		{
		  status = (*per_buffer_fn) ((*bufferP), (*buffer_fpP), closure);
		  *buffer_fpP = 0;
		  if (status < 0) return status;
		}
	}
  while (net_buffer_size > 0);

  return 0;
}


struct msg_rebuffering_stream_data
{
  uint32 desired_size;
  char *buffer;
  uint32 buffer_size;
  uint32 buffer_fp;
  NET_StreamClass *next_stream;
};


int32
msg_rebuffering_stream_write_next_chunk (char *buffer, uint32 buffer_size,
										 void *closure)
{
  struct msg_rebuffering_stream_data *sd =
	(struct msg_rebuffering_stream_data *) closure;
  XP_ASSERT (sd);
  if (!sd) return -1;
  if (!sd->next_stream) return -1;
  return (*sd->next_stream->put_block) (sd->next_stream->data_object,
										buffer, buffer_size);
}

static int
msg_rebuffering_stream_write_chunk (void *closure,
									const char* net_buffer,
									int32 net_buffer_size)
{
  struct msg_rebuffering_stream_data *sd =
	(struct msg_rebuffering_stream_data *) closure;
  XP_ASSERT (sd);
  if (!sd) return -1;
  return msg_ReBuffer (net_buffer, net_buffer_size,
					   sd->desired_size,
					   &sd->buffer, &sd->buffer_size, &sd->buffer_fp,
					   msg_rebuffering_stream_write_next_chunk,
					   sd);
}

static void
msg_rebuffering_stream_abort (void *closure, int status)
{
  struct msg_rebuffering_stream_data *sd =
	(struct msg_rebuffering_stream_data *) closure;
  if (!sd) return;
  FREEIF (sd->buffer);
  if (sd->next_stream)
	{
	  (*sd->next_stream->abort) (sd->next_stream->data_object, status);
	  XP_FREE (sd->next_stream);
	}
  XP_FREE (sd);
}

static void
msg_rebuffering_stream_complete (void *closure)
{
  struct msg_rebuffering_stream_data *sd =
	(struct msg_rebuffering_stream_data *) closure;
  if (!sd) return;
  sd->desired_size = 0;
  msg_rebuffering_stream_write_chunk (closure, "", 0);
  FREEIF (sd->buffer);
  if (sd->next_stream)
	{
	  (*sd->next_stream->complete) (sd->next_stream->data_object);
	  XP_FREE (sd->next_stream);
	}
  XP_FREE (sd);
}

static unsigned int
msg_rebuffering_stream_write_ready (void *closure)
{
  struct msg_rebuffering_stream_data *sd =
	(struct msg_rebuffering_stream_data *) closure;
  if (sd && sd->next_stream)
    return ((*sd->next_stream->is_write_ready)
			(sd->next_stream->data_object));
  else
    return (MAX_WRITE_READY);
}

NET_StreamClass * 
msg_MakeRebufferingStream (NET_StreamClass *next_stream,
						   URL_Struct *url,
						   MWContext *context)
{
  NET_StreamClass *stream;
  struct msg_rebuffering_stream_data *sd;

  XP_ASSERT (next_stream);

  TRACEMSG(("Setting up rebuffering stream. Have URL: %s\n", url->address));

  stream = XP_NEW (NET_StreamClass);
  if (!stream) return 0;

  sd = XP_NEW (struct msg_rebuffering_stream_data);
  if (! sd)
	{
	  XP_FREE (stream);
	  return 0;
	}

  XP_MEMSET (sd, 0, sizeof(*sd));
  XP_MEMSET (stream, 0, sizeof(*stream));

  sd->next_stream = next_stream;
  sd->desired_size = 4096;

  stream->name           = "ReBuffering Stream";
  stream->complete       = msg_rebuffering_stream_complete;
  stream->abort          = msg_rebuffering_stream_abort;
  stream->put_block      = msg_rebuffering_stream_write_chunk;
  stream->is_write_ready = msg_rebuffering_stream_write_ready;
  stream->data_object    = sd;
  stream->window_id      = context;

  return stream;
}



static XP_Bool
msg_iterate_selected_1(MWContext *context,
					   XP_Bool (*func)(MWContext* context,
									   MSG_ThreadEntry* msg,
									   void* closure),
					   MSG_ThreadEntry* msg,
					   void* closure)
{
  MSG_ThreadEntry* next = NULL;
  MSG_ThreadEntry* child = NULL;
  for ( ; msg ; msg = next) {
	next = msg->next;			/* Paranoia because the callback might */
	child = msg->first_child;	/* expunge the message, which will muck the */
								/* next and first_child pointers. */
	if (msg->flags & MSG_FLAG_SELECTED) {
	  assert (! (msg->flags & MSG_FLAG_EXPUNGED));
	  if (!(msg->flags & MSG_FLAG_EXPIRED)) {
		if (!(*func)(context, msg, closure)) return FALSE;
	  }
	}
	if (!msg_iterate_selected_1(context, func, child, closure)) {
	  return FALSE;
	}
  }
  return TRUE;
}

XP_Bool
msg_IterateOverSelectedMessages(MWContext* context,
								XP_Bool (*func)(MWContext* context,
												MSG_ThreadEntry* msg,
												void* closure),
								void* closure)
{
  XP_Bool result = TRUE;
  if (context && context->msgframe && context->msgframe->msgs) {
	msg_undo_StartBatch(context);
	result = msg_iterate_selected_1(context, func,
									context->msgframe->msgs->msgs, closure);
	msg_undo_EndBatch(context);
  }
  return result;
}


static XP_Bool
msg_iterate_all_1(MWContext *context,
				  XP_Bool (*func)(MWContext* context,
								  MSG_ThreadEntry* msg,
								  void* closure),
				  MSG_ThreadEntry* msg,
				  void* closure)
{
  MSG_ThreadEntry* next = NULL;
  MSG_ThreadEntry* child = NULL;
  for ( ; msg ; msg = next) {
	next = msg->next;			/* Paranoia because the callback might */
	child = msg->first_child;	/* expunge the message, which will muck the */
								/* next and first_child pointers. */
	assert (! (msg->flags & MSG_FLAG_EXPUNGED));
	if (!(msg->flags & MSG_FLAG_EXPIRED)) {
	  if (!(*func)(context, msg, closure)) return FALSE;
	}
	if (!msg_iterate_all_1(context, func, child, closure)) {
	  return FALSE;
	}
  }
  return TRUE;
}

void
msg_IterateOverAllMessages(MWContext* context,
						   XP_Bool (*func)(MWContext* context,
										   MSG_ThreadEntry* msg,
										   void* closure),
						   void* closure)
{
  if (context && context->msgframe && context->msgframe->msgs) {
	msg_undo_StartBatch(context);
	msg_iterate_all_1(context, func, context->msgframe->msgs->msgs, closure);
	msg_undo_EndBatch(context);
  }
}



/* Certain URLs must always be displayed in certain types of windows:
   for example, all "mailbox:" URLs must go in the mail window, and
   all "http:" URLs must go in a browser window.  These predicates
   look at the address and say which window type is required.
 */
XP_Bool
MSG_RequiresMailWindow (const char *url)
{
  if (!url) return FALSE;
  if (!strncasecomp (url, "pop3:", 5))
	return TRUE;
  if (!strncasecomp (url, "mailbox:", 8))
	{
	  /* If this is a reference to a folder, it requires a mail window.
		 If it is a reference to a message, it doesn't require one. */
	  char *q = XP_STRCHR (url, '?');
	  if (!q)
		return TRUE;
	  else if (XP_STRNCMP (q, "id=", 3) ||
			   XP_STRSTR (q, "&id="))
		return FALSE;
	  else
		return TRUE;
	}
  return FALSE;
}


/* in mknews.c - the proto is here because I don't feel like recompiling. */
extern XP_Bool NET_IsNewsMessageURL (const char *url);

XP_Bool
MSG_RequiresNewsWindow (const char *url)
{
  if (!url) return FALSE;
  if (*url == 's' || *url == 'S')
	url++;
  if (!strncasecomp (url, "news:", 5))
	{
	  /* If we're asking for a message ID, it's ok to display those in a
		 browser window.  Otherwise, a news window is required. */
	  return !NET_IsNewsMessageURL (url);
	}
  return FALSE;
}

XP_Bool
MSG_RequiresComposeWindow (const char *url)
{
  if (!url) return FALSE;
  if (!strncasecomp (url, "mailto:", 7))
	{
	  return TRUE;
	}
  return FALSE;
}


XP_Bool
MSG_RequiresBrowserWindow (const char *url)
{
  if (!url) return FALSE;
  if (MSG_RequiresNewsWindow (url) ||
	  MSG_RequiresMailWindow (url) ||
	  !strncasecomp (url, "about:", 6) ||
	  !strncasecomp (url, "mailto:", 7) ||
	  !strncasecomp (url, "view-source:", 12) ||
	  !strncasecomp (url, "internal-panel-handler:", 22) ||
	  !strncasecomp (url, "internal-dialog-handler", 23))
	return FALSE;

  else if (!strncasecomp (url, "news:", 5) ||
		   !strncasecomp (url, "snews:", 6) ||
		   !strncasecomp (url, "mailbox:", 8))
	{
	  /* Mail and news messages themselves don't require browser windows,
		 but their attachments do. */
	  if (XP_STRSTR(url, "?part=") || XP_STRSTR(url, "&part="))
		return TRUE;
	  else
		return FALSE;
	}
  else
	return TRUE;
}


/* If this URL requires a particular kind of window, and this is not
   that kind of window, then we need to find or create one.
 */
XP_Bool
MSG_NewWindowRequired (MWContext *context, const char *url)
{
  if (!context) return TRUE;
  if (/* This is not a mail window, and one is required. */
      (context->type != MWContextMail &&
       MSG_RequiresMailWindow (url)) ||

      /* This is not a news window, and one is required. */
      (context->type != MWContextNews &&
       MSG_RequiresNewsWindow (url)) ||

      /* This is not a browser window, and one is required. */
      (context->type != MWContextBrowser &&
       MSG_RequiresBrowserWindow (url)))
    return TRUE;
  else
    return FALSE;
}


/* If we're in a mail window, and clicking on a link which will itself
   require a mail window, then don't allow this to show up in a different
   window - since there can be only one mail window.
 */
XP_Bool
MSG_NewWindowProhibited (MWContext *context, const char *url)
{
  if (!context) return FALSE;
  if ((context->type == MWContextMail &&
       MSG_RequiresMailWindow (url)) ||
      (context->type == MWContextNews &&
       MSG_RequiresNewsWindow (url)) ||
      (MSG_RequiresComposeWindow (url)))
    return TRUE;
  else
    return FALSE;
}


char *
MSG_ConvertToQuotation (const char *string)
{
  int32 column = 0;
  int32 newlines = 0;
  int32 chars = 0;
  const char *in;
  char *out;
  char *new_string;

  if (! string) return 0;

  /* First, count up the lines in the string. */
  for (in = string; *in; in++)
    {
	  chars++;
      if (*in == CR || *in == LF)
		{
		  if (in[0] == CR && in[1] == LF) {
		 	 in++;
		 	 chars++;
		  }
		  newlines++;
		  column = 0;
		}
      else
		{
		  column++;
		}
    }
  /* If the last line doesn't end in a newline, pretend it does. */
  if (column != 0)
    newlines++;

  /* 2 characters for each '> ', +1 for '\0', and + potential linebreak */
  new_string = (char *) XP_ALLOC (chars + (newlines * 2) + 1 + LINEBREAK_LEN);
  if (! new_string)
	return 0;

  column = 0;
  out = new_string;

  /* Now copy. */
  for (in = string; *in; in++)
    {
	  if (column == 0)
		{
		  *out++ = '>';
		  *out++ = ' ';
		}
		
	  *out++ = *in;
      if (*in == CR || *in == LF)
		{
		  if (in[0] == CR && in[1] == LF)
			*out++ = *++in;
		  newlines++;
		  column = 0;
		}
      else
		{
		  column++;
		}
    }

 /* If the last line doesn't end in a newline, add one. */
	if (column != 0)
  	{
		XP_STRCPY (out, LINEBREAK);
		out += LINEBREAK_LEN;
  	}
  
 	*out = 0;

  return new_string;
}


/* Given a string and a length, removes any "Re:" strings from the front.
   It also deals with that dumbass "Re[2]:" and "Re(2):" thing that some
   losing mailers do.

   Returns TRUE if it made a change, FALSE otherwise.

   The string is not altered: the pointer to its head is merely advanced,
   and the length correspondingly decreased.
 */
XP_Bool
msg_StripRE(const char **stringP, uint32 *lengthP)
{
  const char *s, *s_end;
  const char *last;
  uint32 L;
  XP_Bool result = FALSE;
  XP_ASSERT(stringP && lengthP);
  if (!stringP || !lengthP) return FALSE;
  s = *stringP;
  L = *lengthP;

  s_end = s + L;
  last = s;

 AGAIN:

  while (s < s_end && XP_IS_SPACE(*s))
	s++;

  if (s < (s_end-2) &&
	  (s[0] == 'r' || s[0] == 'R') &&
	  (s[1] == 'e' || s[1] == 'E'))
	{
	  if (s[2] == ':')
		{
		  s = s+3;			/* Skip over "Re:" */
		  result = TRUE;	/* Yes, we stripped it. */
		  goto AGAIN;		/* Skip whitespace and try again. */
		}
	  else if (s[2] == '[' || s[2] == '(')
		{
		  const char *s2 = s+3;		/* Skip over "Re[" or "Re(" */

		  /* Skip forward over digits after the "[" or "(". */
		  while (s2 < (s_end-2) && XP_IS_DIGIT(*s2))
			s2++;

		  /* Now ensure that the following thing is "]:" or "):"
			 Only if it is do we alter `s'.
		   */
		  if ((s2[0] == ']' || s2[0] == ')') &&
			  s2[1] == ':')
			{
			  s = s2+2;			/* Skip over "]:" */
			  result = TRUE;	/* Yes, we stripped it. */
			  goto AGAIN;		/* Skip whitespace and try again. */
			}
		}
	}

  /* Decrease length by difference between current ptr and original ptr.
	 Then store the current ptr back into the caller. */
  *lengthP -= (s - (*stringP));
  *stringP = s;

  return result;
}



const char*
MSG_FormatDate(MWContext* context, time_t date)
{

  /* fix i18n.  Well, maybe.  Isn't strftime() supposed to be i18n? */
  /* ftong- Well.... strftime() in Mac and Window is not really i18n 		*/
  /* We need to use XP_StrfTime instead of strftime 						*/
  static char result[40];	/* 30 probably not enough */
  time_t now = time ((time_t *) 0);

  int32 offset = XP_LocalZoneOffset() * 60; /* Number of seconds between
											 local and GMT. */

  int32 secsperday = 24 * 60 * 60;

  int32 nowday = (now + offset) / secsperday;
  int32 day = (date + offset) / secsperday;

  if (day == nowday) {
	XP_StrfTime(context, result, sizeof(result), XP_TIME_FORMAT, localtime(&date));
  } else if (day < nowday && day > nowday - 7) {
	XP_StrfTime(context, result, sizeof(result), XP_WEEKDAY_TIME_FORMAT, localtime(&date));
  } else {
	XP_StrfTime(context, result, sizeof(result), XP_DATE_TIME_FORMAT, localtime(&date));
  }

  return result;
}
  


char*
msg_GetDummyEnvelope(void)
{
  static char result[75];
  time_t now = time ((time_t *) 0);
  char *ct = ctime(&now);
  XP_ASSERT(ct[24] == CR || ct[24] == LF);
  ct[24] = 0;
  /* This value must be in ctime() format, with English abbreviations.
	 strftime("... %c ...") is no good, because it is localized. */
  XP_STRCPY(result, "From - ");
  XP_STRCPY(result + 7, ct);
  XP_STRCPY(result + 7 + 24, LINEBREAK);
  return result;
}

/* #define STRICT_ENVELOPE */

XP_Bool
msg_IsEnvelopeLine(const char *buf, int32 buf_size)
{
#ifdef STRICT_ENVELOPE
  /* The required format is
	   From jwz  Fri Jul  1 09:13:09 1994
	 But we should also allow at least:
	   From jwz  Fri, Jul 01 09:13:09 1994
	   From jwz  Fri Jul  1 09:13:09 1994 PST
	   From jwz  Fri Jul  1 09:13:09 1994 (+0700)

	 We can't easily call XP_ParseTimeString() because the string is not
	 null terminated (ok, we could copy it after a quick check...) but
	 XP_ParseTimeString() may be too lenient for our purposes.

	 DANGER!!  The released version of 2.0b1 was (on some systems,
	 some Unix, some NT, possibly others) writing out envelope lines
	 like "From - 10/13/95 11:22:33" which STRICT_ENVELOPE will reject!
   */
  const char *date, *end;

  if (buf_size < 29) return FALSE;
  if (*buf != 'F') return FALSE;
  if (XP_STRNCMP(buf, "From ", 5)) return FALSE;

  end = buf + buf_size;
  date = buf + 5;

  /* Skip horizontal whitespace between "From " and user name. */
  while ((*date == ' ' || *date == '\t') && date < end)
	date++;

  /* If at the end, it doesn't match. */
  if (XP_IS_SPACE(*date) || date == end)
	return FALSE;

  /* Skip over user name. */
  while (!XP_IS_SPACE(*date) && date < end)
	date++;

  /* Skip horizontal whitespace between user name and date. */
  while ((*date == ' ' || *date == '\t') && date < end)
	date++;

  /* Don't want this to be localized. */
# define TMP_ISALPHA(x) (((x) >= 'A' && (x) <= 'Z') || \
						 ((x) >= 'a' && (x) <= 'z'))

  /* take off day-of-the-week. */
  if (date >= end - 3)
	return FALSE;
  if (!TMP_ISALPHA(date[0]) || !TMP_ISALPHA(date[1]) || !TMP_ISALPHA(date[2]))
	return FALSE;
  date += 3;
  /* Skip horizontal whitespace (and commas) between dotw and month. */
  if (*date != ' ' && *date != '\t' && *date != ',')
	return FALSE;
  while ((*date == ' ' || *date == '\t' || *date == ',') && date < end)
	date++;

  /* take off month. */
  if (date >= end - 3)
	return FALSE;
  if (!TMP_ISALPHA(date[0]) || !TMP_ISALPHA(date[1]) || !TMP_ISALPHA(date[2]))
	return FALSE;
  date += 3;
  /* Skip horizontal whitespace between month and dotm. */
  if (date == end || (*date != ' ' && *date != '\t'))
	return FALSE;
  while ((*date == ' ' || *date == '\t') && date < end)
	date++;

  /* Skip over digits and whitespace. */
  while (((*date >= '0' && *date <= '9') || *date == ' ' || *date == '\t') &&
		 date < end)
	date++;
  /* Next character should be a colon. */
  if (date >= end || *date != ':')
	return FALSE;

  /* Ok, that ought to be enough... */

# undef TMP_ISALPHA

#else  /* !STRICT_ENVELOPE */

  if (buf_size < 5) return FALSE;
  if (*buf != 'F') return FALSE;
  if (XP_STRNCMP(buf, "From ", 5)) return FALSE;

#endif /* !STRICT_ENVELOPE */

  return TRUE;
}
