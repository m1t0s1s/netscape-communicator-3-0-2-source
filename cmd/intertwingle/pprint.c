/* -*- Mode: C; tab-width: 4 -*-
   leaf.c --- extract text from MIME objects.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-Jul-97.
   This takes a MIME object and pulls the leaf-nodes out of it, via libmime.
 */

#include <stdio.h>
#include "config.h"
#include "libmime.h"	/* for MimeHeaders, HEADER_FROM, etc */
#include "mimehdrs.h"	/* for MimeHeaders_map_headers() */
#include "xp_time.h"	/* for XP_ParseTimeString() */
#include "leaf.h"
#include "pprint.h"

extern int MK_OUT_OF_MEMORY;

static int
pprint_header_mapper (const char *name, const char *value, void *closure)
{
  FILE *out = (FILE *) closure;
  const char *s;
  int i;
  HeaderType type = header_type (name);

  if (type == HeaderIgnore)
	return 0;

  if (type == HeaderURL)
	return 0;


  /* Write out the key, downcased.
   */
  fputs ("\n  (:header-field (:key \"", out);
  for (s = name; *s; s++)
	fputc (tolower(*s), out);
  fputs ("\")", out);

  if (type == HeaderID)
	{
	  const char *s = value;
	  while (*s)
		{
		  const char *end;
		  while (*s && *s != '<') s++;
		  for (end = s; *end && *end != '>'; end++)
			;
		  if (end > s+1)
			{
			  fputs ("\n                 (:id \"", out);
			  fwrite (s+1, 1, end-s-1, out);
			  fputs ("\")", out);
			}
		  s = (*end ? end+1 : end);
		}
	}
  else if (type == HeaderAddress)
	{
	  char *names = 0;
	  char *values = 0;
	  int n = MSG_ParseRFC822Addresses(value, &names, &values);
	  if (n > 0)
		{
		  char *na = names;
		  char *va = values;
		  while (n > 0)
			{
			  fprintf (out, "\n                 (:addr \"%s\" \"%s\")",
					   na, va);
			  na += strlen (na) + 1;
			  va += strlen (va) + 1;
			  n--;
			}
		}
	  if (names) free (names);
	  if (values) free (values);
	}
  else if (type == HeaderNewsgroup)
	{
	  char *s = strdup (value);
	  char *s2;
	  if (!s) return MK_OUT_OF_MEMORY;
	  s2 = strtok (s, " ,;");
	  do {
		fprintf (out, "\n                 (:news \"%s\")", s2);
	  } while ((s2 = strtok (0, " ,;")));
	  free (s);
	}
  else if (type == HeaderDate)
	{
	  time_t t = XP_ParseTimeString (value, TRUE);
	  fprintf (out, "(:time_t %lu)", (unsigned long) t);
	}
  else
	{
	  XP_ASSERT(type == HeaderText);
	  fputs (" (:text \"", out);
	  i = 0;
	  for (s = value; *s; s++, i++)
		if (i >= 27)
		  {
			fputs ("...", out);
			break;
		  }
		else if (*s == '\n')
		  fputs ("\\n", out);
		else if (*s == '\"')
		  fputs ("\\\"", out);
		else
		  fputc (*s, out);

	  fputs ("\")", out);
	}

  fputs (")", out);
  return 1;
}



int
pprint_begin (int32 db_id,
			  MimeHeaders *headers,
			  const char *filename,
			  int32 file_position,
			  const char *urls,
			  const char *addrs_or_ids,
			  void *closure)
{
  FILE *out = (FILE *) closure;
  int status = 0;

  fprintf (out, "\n(:message");
  fprintf (out, "\n  (:db-id %d)", db_id);
  fprintf (out, "\n  (:file \"%s\" %ld)", filename, file_position);

  status = MimeHeaders_map_headers (headers, pprint_header_mapper, closure);
  if (status < 0) return status;

  if (urls)
	{
	  const char *s = urls;
	  while (*s)
		{
		  fprintf (out, "\n  (:link \"%s\")", s);
		  s += strlen(s) + 1;
		}
	}
  if (addrs_or_ids)
	{
	  const char *s = addrs_or_ids;
	  while (*s)
		{
		  fprintf (out, "\n  (:addr-or-id \"%s\")", s);
		  s += strlen(s) + 1;
		}
	}
  return 0;
}

int
pprint_attachment (const char *name, const char *type, const char *desc,
				   const char *location,
				   const char *urls,
				   const char *addrs_or_ids,
				   const char *text, int32 text_length,
				   int32 sub_message_db_id,
				   void *closure)
{
  FILE *out = (FILE *) closure;

  fprintf (out, "\n  (:attachment");
  if (name && *name)
	fprintf (out, "\n    (:name \"%s\")", name);
  fprintf (out, "\n    (:type \"%s\")", type);
  if (desc && *desc)
	fprintf (out, "\n    (:description \"%s\")", desc);
  if (location && *location)
	fprintf (out, "\n    (:location \"%s\")", location);

  if (text)
	{
	  char *s;
	  if (text_length >= 59)
		strcpy (((char*)text)+59, "...");	/* #### shorting const */
	  else
		((char*)text)[text_length] = 0;		/* #### shorting const */
	  for (s = (char*)text; *s; s++)		/* #### shorting const */
		if (*s == '\n' || *s == '\"')
		  *s = ' ';
	  fprintf (out, "\n    (:text \"%s\")", text);
	  XP_ASSERT(!sub_message_db_id);
	}
  else if (sub_message_db_id)
	fprintf (out, "\n    (:value (:msg-ptr %d))", sub_message_db_id);

  if (urls)
	{
	  const char *s = urls;
	  while (*s)
		{
		  fprintf (out, "\n    (:link \"%s\")", s);
		  s += strlen(s) + 1;
		}
	}
  if (addrs_or_ids)
	{
	  const char *s = addrs_or_ids;
	  while (*s)
		{
		  fprintf (out, "\n    (:addr-or-id \"%s\")", s);
		  s += strlen(s) + 1;
		}
	}

  fprintf (out, ")");
  return 0;
}

int
pprint_end (void *closure)
{
  FILE *out = (FILE *) closure;
  fprintf (out, ")");
  return 0;
}



/* Horrible fucking kludge -- xp_time.c calls this.  Ignore it. */
void *FE_SetTimeout (TimeoutCallbackFunction x, void *y, uint32 z)
{ return 0; }
