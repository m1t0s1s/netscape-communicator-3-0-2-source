/* -*- Mode: C; tab-width: 4 -*-
   mimetpla.c --- definition of the MimeInlineTextPlain class (see mimei.h)
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#include "mimetpla.h"

#define MIME_SUPERCLASS mimeInlineTextClass
MimeDefClass(MimeInlineTextPlain, MimeInlineTextPlainClass,
			 mimeInlineTextPlainClass, &MIME_SUPERCLASS);

static int MimeInlineTextPlain_parse_begin (MimeObject *);
static int MimeInlineTextPlain_parse_line (char *, int32, MimeObject *);
static int MimeInlineTextPlain_parse_eof (MimeObject *, XP_Bool);

static int
MimeInlineTextPlainClassInitialize(MimeInlineTextPlainClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  oclass->parse_begin = MimeInlineTextPlain_parse_begin;
  oclass->parse_line  = MimeInlineTextPlain_parse_line;
  oclass->parse_eof   = MimeInlineTextPlain_parse_eof;
  return 0;
}

static int
MimeInlineTextPlain_parse_begin (MimeObject *obj)
{
  int status = 0;

  status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_begin(obj);
  if (status < 0) return status;

  if (!obj->output_p) return 0;

#ifdef MIME_TO_HTML
  if (obj->options &&
	  obj->options->write_html_p &&
	  obj->options->output_fn)
	{
	  char* strs[4];
	  char* s;
	  strs[0] = "<PRE>";
	  strs[1] = "<PRE VARIABLE>";
	  strs[2] = "<PRE WRAP>";
	  strs[3] = "<PRE VARIABLE WRAP>";
	  s = XP_STRDUP(strs[(obj->options->variable_width_plaintext_p ? 1 : 0) +
						(obj->options->wrap_long_lines_p ? 2 : 0)]);
	  if (!s) return MK_OUT_OF_MEMORY;
	  status = MimeObject_write(obj, s, XP_STRLEN(s), FALSE);
	  XP_FREE(s);
	  if (status < 0) return status;

	  /* text/plain objects always have separators before and after them.
		 Note that this is not the case for text/enriched objects. */
	  status = MimeObject_write_separator(obj);
	  if (status < 0) return status;
	}
#endif /* MIME_TO_HTML */

  return 0;
}

static int
MimeInlineTextPlain_parse_eof (MimeObject *obj, XP_Bool abort_p)
{
  int status;
  if (obj->closed_p) return 0;
  
  /* Run parent method first, to flush out any buffered data. */
  status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_eof(obj, abort_p);
  if (status < 0) return status;

  if (!obj->output_p) return 0;

#ifdef MIME_TO_HTML
  if (obj->options &&
	  obj->options->write_html_p &&
	  obj->options->output_fn &&
	  !abort_p)
	{
	  char s[] = "</PRE>";
	  status = MimeObject_write(obj, s, XP_STRLEN(s), FALSE);
	  if (status < 0) return status;

	  /* text/plain objects always have separators before and after them.
		 Note that this is not the case for text/enriched objects.
	   */
	  status = MimeObject_write_separator(obj);
	  if (status < 0) return status;
	}
#endif /* MIME_TO_HTML */

  return 0;
}


static int
MimeInlineTextPlain_parse_line (char *line, int32 length, MimeObject *obj)
{
  int status;

  XP_ASSERT(length > 0);
  if (length <= 0) return 0;

  status = MimeObject_grow_obuffer (obj, length * 2 + 40);
  if (status < 0) return status;

#ifdef MIME_LEAF_TEXT
  if (obj->options && obj->options->write_leaf_p)
	{
	  XP_ASSERT(obj->options->state->current_leaf_closure);
	  status = obj->options->leaf_write (line, length,
								   obj->options->state->current_leaf_closure);
	  return status;
	}
#endif /* MIME_LEAF_TEXT */


#ifdef MIME_TO_HTML
  /* Copy `line' to `out', quoting HTML along the way.
	 Note: this function does no charset conversion; that has already
	 been done.
   */
  *obj->obuffer = 0;
  status = NET_ScanForURLs (
#ifndef AKBAR
							(obj->options ? obj->options->pane : 0),
#endif /* !AKBAR */
							line, length, obj->obuffer, obj->obuffer_size - 10,
#ifdef AKBAR
							FALSE
#else  /* !AKBAR */
							(obj->options
							 ? obj->options->dont_touch_citations_p
							 : FALSE)
#endif /* !AKBAR */
							);
  if (status < 0) return status;
  XP_ASSERT(*line == 0 || *obj->obuffer);
  return MimeObject_write(obj, obj->obuffer, XP_STRLEN(obj->obuffer), TRUE);
#else  /* !MIME_TO_HTML */
  return 0;
#endif /* !MIME_TO_HTML */
}
