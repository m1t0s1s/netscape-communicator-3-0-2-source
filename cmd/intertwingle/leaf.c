/* -*- Mode: C; tab-width: 4 -*-
   leaf.c --- extract text from MIME objects.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-Jul-97.
   This takes a MIME object and pulls the leaf-nodes out of it, via libmime.
 */

#include <stdio.h>
#include "config.h"
#include "libmime.h"	/* for MimeHeaders */
#include "mimemsg.h"	/* for MimeObject, mime_new() */
#include "mimehdrs.h"	/* for MimeHeaders_map_headers() */

#include "leaf.h"
#include "html.h"

extern int MK_OUT_OF_MEMORY;

#define HEADER_CONTENT_LOCATION	"Content-Location"

typedef struct Leaf Leaf;

struct LeafParseObject {
  MimeObject *obj;
  MimeDisplayOptions opt;
  int leaf_count;
  Leaf *open_leaves;
  Leaf *toplevel_leaves;
  char *filename;
  int32 start_pos;

  int (*write_message_begin) (int32 db_id,
							  MimeHeaders *headers,
							  const char *filename,
							  int32 file_position,
							  const char *urls,
							  const char *addrs_or_ids,
							  void *closure);
  int (*write_attachment) (const char *name,
						   const char *type,
						   const char *description,
						   const char *location,
						   const char *urls,
						   const char *addrs_or_ids,
						   const char *text,
						   int32 text_length,
						   int32 sub_message_db_id,
						   void *closure);
  int (*write_message_end) (void *closure);
  void *write_closure;
};

struct Leaf {
  int unique_id;
  int32 pos;
  char *content_type;
  MimeHeaders *leaf_headers;
  MimeHeaders *container_headers;
  LeafParseObject *pobj;
  char *body;
  int32 body_size, body_fp;
  char *location;
  char *urls;
  char *addrs_or_ids;
  Leaf *kids;
  Leaf *next;
};


static int global_leaf_ids = 0;

static void *
mime_leaf_begin (const char *type,
				 MimeHeaders *leaf_headers,
				 MimeHeaders *container_headers,
				 void *leaf_closure)
{
  LeafParseObject *pobj = (LeafParseObject *) leaf_closure;
  Leaf *leaf = XP_NEW_ZAP(Leaf);
  char *s;
  if (!leaf) return 0;

  if (container_headers)
	leaf->unique_id = ++global_leaf_ids;

  if (!type) type = "UNKNOWN";
  leaf->content_type = strdup(type);
  if (leaf_headers)
	leaf->leaf_headers = MimeHeaders_copy (leaf_headers);
  if (container_headers)
	leaf->container_headers = MimeHeaders_copy (container_headers);
  leaf->pobj = pobj;

  for (s = leaf->content_type; *s; s++)
	*s = tolower(*s);

  leaf->next = pobj->open_leaves;
  pobj->open_leaves = leaf;

  return leaf;
}

static int
mime_leaf_write (char *buf, int32 size, void *leaf_closure)
{
  Leaf *leaf = (Leaf *) leaf_closure;

  if (leaf->body_size < leaf->body_fp + size + 1)
	{
	  int status = msg_GrowBuffer (((leaf->body_fp + size) * 1.1),
								   sizeof(char), 512,
								   &leaf->body, &leaf->body_size);
	  if (status < 0) return status;
	}
  memcpy (leaf->body + leaf->body_fp, buf, size);
  leaf->body_fp += size;
  return 0;
}

static void decode_location_header (char *value);
static int scan_text_headers (Leaf *leaf);

static int
mime_leaf_end (void *leaf_closure, XP_Bool error_p)
{
  int status = 0;
  Leaf *leaf = (Leaf *) leaf_closure;

  /* Take this leaf off the list of open leaves... */
  XP_ASSERT (leaf == leaf->pobj->open_leaves);
  if (leaf != leaf->pobj->open_leaves) return -1;
  leaf->pobj->open_leaves = leaf->next;
  leaf->next = 0;

  /* If there is an open parent leaf, put this leaf on its list of kids.
	 Else, put it on the toplevel list.
   */
  if (leaf->pobj->open_leaves)
	{
	  leaf->next = leaf->pobj->open_leaves->kids;
	  leaf->pobj->open_leaves->kids = leaf;
	}
  else
	{
	  leaf->next = leaf->pobj->toplevel_leaves;
	  leaf->pobj->toplevel_leaves = leaf;
	}

  if (leaf->body_fp)
	{
	  if (!strcasecmp (leaf->content_type, TEXT_HTML) ||
		  !strcasecmp (leaf->content_type, TEXT_ENRICHED) ||
		  !strcasecmp (leaf->content_type, TEXT_RICHTEXT))
		{
		  char *def_base = 0;
		  char *base = 0;
		  if (leaf->leaf_headers)
			{
			  def_base = MimeHeaders_get(leaf->leaf_headers,
										 HEADER_CONTENT_BASE,
										 TRUE, FALSE);
			  if (!def_base)
				def_base = MimeHeaders_get(leaf->leaf_headers,
										   HEADER_CONTENT_LOCATION,
										   TRUE, FALSE);

			  if (def_base)
				decode_location_header (def_base);

			  base = def_base;
			}
		  status = html_to_plaintext (leaf->body, &leaf->body_fp, &base,
									  &leaf->urls);
		  if (base != def_base)
			free (def_base);
		  leaf->location = base;
		  if (status < 0) return status;
		}
	  else
		{
		  int32 size = 0, fp = 0;

		  status = strip_overstriking (leaf->body, &leaf->body_fp);
		  if (status < 0) return status;

		  status = search_for_urls (leaf->body, leaf->body_fp,
									&leaf->urls, &size, &fp);
		  if (status < 0) return status;
		}


	  /* Now search for random body text that looks like email addresses
		 or message IDs. */
	  {
		int32 size = 0, fp = 0;
		status = search_for_addrs_or_ids (leaf->body, leaf->body_fp,
										  &leaf->addrs_or_ids,
										  &size, &fp);
		if (status < 0) return status;
	  }
	}

  if (leaf->container_headers)
	{
	  status = scan_text_headers (leaf);
	  if (status < 0) return status;
	}

  return 0;
}


struct scan_text_closure {
  Leaf *leaf;
  int32 urls_size, urls_fp;
  int32 addr_size, addr_fp;
};

static int
scan_text_headers_mapper (const char *name, const char *value, void *closure)
{
  int status;
  struct scan_text_closure *stc = (struct scan_text_closure *) closure;

  if (header_type(name) != HeaderText)
	return 0;

  status = search_for_urls (value, strlen(value),
							&stc->leaf->urls,
							&stc->urls_size, &stc->urls_fp);
  if (status < 0) return status;

  status = search_for_addrs_or_ids (value, strlen(value),
									&stc->leaf->addrs_or_ids,
									&stc->addr_size, &stc->addr_fp);
  if (status < 0) return status;

  return 0;
}

static int
scan_text_headers (Leaf *leaf)
{
  /* Now search through the textual headers for URLs and addresses.
	 (That is, don't look To, From, etc -- we already got those.)
   */
  struct scan_text_closure stc;
  memset (&stc, 0, sizeof(stc));
  stc.leaf = leaf;
  return MimeHeaders_map_headers (leaf->container_headers,
								  scan_text_headers_mapper,
								  &stc);
}





static void
decode_location_header (char *value)
{
  /* The value of the Content-Base header is a number of "words".  Whitespace
	 in this header is not significant -- it is assumed that any real
	 whitespace in the URL has already been encoded, and whitespace has been
	 inserted to allow the lines in the mail header to be wrapped reasonably.
	 Creators are supposed to insert whitespace every 40 characters or less. */
  const char *in = value;
  char *out = value;
  for (; *in; in++)
	if (!isspace(*in) && *in != '"')   /* ignore whitespace and quotes */
	  *out++ = *in;
  *out++ = 0;
}


static int
free_leaf_tree (Leaf *leaf)
{
  int status = 0;
  if (leaf->kids)
	{
	  Leaf *L = leaf->kids;
	  leaf->kids = 0;
	  status = free_leaf_tree (L);
	}
  if (leaf->next)
	{
	  Leaf *L = leaf->next;
	  leaf->next = 0;
	  if (status < 0)
	    free_leaf_tree (L);
	  else
		status = free_leaf_tree (L);
	}
  if (leaf->body)
	{
	  free (leaf->body);
	  leaf->body = 0;
	}
  if (leaf->urls)
	{
	  free (leaf->urls);
	  leaf->urls = 0;
	}
  if (leaf->addrs_or_ids)
	{
	  free (leaf->addrs_or_ids);
	  leaf->addrs_or_ids = 0;
	}
  if (leaf->location)
	{
	  free (leaf->location);
	  leaf->location = 0;
	}
  if (leaf->leaf_headers)
	MimeHeaders_free (leaf->leaf_headers);
  if (leaf->container_headers)
	MimeHeaders_free (leaf->container_headers);
  free (leaf->content_type);
  free (leaf);
  return status;
}


static int
free_mime_leaves (LeafParseObject *pobj)
{
  Leaf *L;
  int status = 0;
  L = pobj->open_leaves;
  pobj->open_leaves = 0;
  if (L) status = free_leaf_tree (L);
  L = pobj->toplevel_leaves;
  pobj->toplevel_leaves = 0;
  if (L)
	if (status < 0)
	  free_leaf_tree (L);
	else
	  status = free_leaf_tree (L);
  return status;
}


static int write_mime_leaf_list (Leaf *leaf_list, XP_Bool refs_only);


static int
write_mime_leaf_reference (Leaf *leaf)
{
  char *name = 0, *desc = 0;
  int status = 0;
  if (leaf->leaf_headers)
	{
	  name = MimeHeaders_get_name(leaf->leaf_headers);
	  desc = MimeHeaders_get(leaf->leaf_headers, HEADER_CONTENT_DESCRIPTION,
							 FALSE, FALSE);
	}

  status = leaf->pobj->write_attachment (name, leaf->content_type, desc,
										 leaf->location,
										 leaf->urls,
										 leaf->addrs_or_ids,
										 (leaf->body_fp ? leaf->body : 0),
										 leaf->body_fp,
										 (leaf->body_fp ? 0 : leaf->unique_id),
										 leaf->pobj->write_closure);
  if (name) free (name);
  if (desc) free (desc);
  return status;
}

static int
write_mime_leaf_1 (Leaf *leaf)
{
  int status = 0;
  XP_Bool msg_p = (!strcasecmp (leaf->content_type, MESSAGE_RFC822) ||
				   !strcasecmp (leaf->content_type, MESSAGE_NEWS));
  if (msg_p)
	{
	  XP_ASSERT(leaf->body_fp == 0);
	  XP_ASSERT(leaf->container_headers);

	  status = leaf->pobj->write_message_begin (leaf->unique_id,
												leaf->container_headers,
												leaf->pobj->filename,
												leaf->pobj->start_pos,
												leaf->urls,
												leaf->addrs_or_ids,
												leaf->pobj->write_closure);
	  if (status < 0) return status;

	  if (leaf->kids)
		{
		  status = write_mime_leaf_list (leaf->kids, TRUE);
		  if (status < 0) return status;
		}

	  status = leaf->pobj->write_message_end (leaf->pobj->write_closure);
	  if (status < 0) return status;

	  if (leaf->kids)
		{
		  status = write_mime_leaf_list (leaf->kids, FALSE);
		  if (status < 0) return status;
		}
	}
  else
	{
	  XP_ASSERT(!leaf->kids);
	  status = write_mime_leaf_reference (leaf);
	}

  return status;
}

static int
write_mime_leaf_list (Leaf *leaf_list, XP_Bool refs_only)
{
  Leaf *L;
  int status = 0;
  int32 i, nleaves;
  Leaf **leaves;

  if (!leaf_list->next) 	/* shortcut... */
	{
	  if (refs_only)
		return write_mime_leaf_reference (leaf_list);
	  else if (leaf_list->kids)
		return write_mime_leaf_1 (leaf_list);
	}

  nleaves = 0;
  for (L = leaf_list; L; L = L->next)
	nleaves++;
  leaves = (Leaf **) malloc(sizeof(Leaf)*nleaves);
  if (!leaves) return MK_OUT_OF_MEMORY;
  i = nleaves;
  for (L = leaf_list; L; L = L->next)
	leaves[--i] = L;
  for (i = 0; i < nleaves; i++)
	{
	  if (refs_only)
		status = write_mime_leaf_reference (leaves[i]);
	  else if (leaves[i]->kids)
		status = write_mime_leaf_1 (leaves[i]);
	  if (status < 0) break;
	}
  free (leaves);
  return status;
}


static int
dummy_output_init_fn (const char *type,
					  const char *charset,
					  const char *name,
					  const char *x_mac_type,
					  const char *x_mac_creator,
					  void *stream_closure)
{
  XP_ASSERT(0);
  return -1;
}

static int
dummy_output_fn(char *buf, int32 size, void *closure)
{
  XP_ASSERT(0);
  return -1;
}


LeafParseObject *
leaf_init (const char *filename, int32 start_pos,
		   int (*write_message_begin) (int32 db_id,
									   MimeHeaders *headers,
									   const char *filename,
									   int32 file_position,
									   const char *urls,
									   const char *addrs_or_ids,
									   void *closure),
		   int (*write_attachment) (const char *name,
									const char *type,
									const char *description,
									const char *location,
									const char *urls,
									const char *addrs_or_ids,
									const char *text,
									int32 text_length,
									int32 sub_message_db_id,
									void *closure),
		   int (*write_message_end) (void *closure),
		   void *closure)
{
  int status = 0;
  LeafParseObject *pobj = XP_NEW (LeafParseObject);

  if (!pobj) return 0; /* MK_OUT_OF_MEMORY */
  XP_MEMSET(pobj, 0, sizeof(*pobj));

  pobj->filename = (filename ? strdup(filename) : 0);
  pobj->start_pos = start_pos;
  pobj->write_message_begin = write_message_begin;
  pobj->write_attachment = write_attachment;
  pobj->write_message_end = write_message_end;
  pobj->write_closure = closure;

  /* #### opt->url = */
  pobj->opt.write_leaf_p = TRUE;

  pobj->opt.output_init_fn		= dummy_output_init_fn;
  pobj->opt.output_fn			= dummy_output_fn;

  pobj->opt.charset_conversion_fn= 0;
  pobj->opt.rfc1522_conversion_fn= 0;
  pobj->opt.reformat_date_fn		= 0;

#if 0 /* #### */
  pobj->opt.file_type_fn		= file_type;
  pobj->opt.type_description_fn	= 0;
  pobj->opt.type_icon_name_fn	= type_icon;
#endif

  pobj->opt.leaf_closure		= pobj;
  pobj->opt.leaf_begin			= mime_leaf_begin;
  pobj->opt.leaf_write			= mime_leaf_write;
  pobj->opt.leaf_end			= mime_leaf_end;

  pobj->obj = mime_new ((MimeObjectClass *)&mimeMessageClass,
						(MimeHeaders *) NULL,
						MESSAGE_RFC822);
  if (!pobj->obj)
	{
	  XP_FREE(pobj);
	  return 0;
	}

  pobj->obj->options = &pobj->opt;


  status = pobj->obj->class->initialize(pobj->obj);
  if (status >= 0)
	status = pobj->obj->class->parse_begin(pobj->obj);
  if (status < 0)
	{
	  XP_FREE(pobj->obj);
	  XP_FREE(pobj);
	  return 0;
	}
  return pobj;
}

int
leaf_write (LeafParseObject *pobj, char *buf, int32 size)
{
  XP_ASSERT(pobj && pobj->obj);
  if (!pobj || !pobj->obj) return -1;
  return pobj->obj->class->parse_buffer(buf, size, pobj->obj);
}

int
leaf_done (LeafParseObject *pobj, XP_Bool abort_p)
{
  int status;
  XP_ASSERT(pobj && pobj->obj);
  if (!pobj || !pobj->obj) return -1;

  status = pobj->obj->class->parse_eof(pobj->obj, FALSE);
  if (status >= 0)
	status = pobj->obj->class->parse_end(pobj->obj, FALSE);
  mime_free(pobj->obj);

  if (status >= 0)
	status = write_mime_leaf_list (pobj->toplevel_leaves, FALSE);

  free_mime_leaves (pobj);

  if (pobj->filename)
	free(pobj->filename);
  XP_FREE(pobj);
  return status;
}



/* Header utilities
 */

#define HEADER_APPROVED				"Approved"
#define HEADER_CONTENT_ID			"Content-ID"
#define HEADER_CONTENT_MD5			"Content-MD5"
#define HEADER_ERRORS_TO			"Errors-To"
#define HEADER_EXPIRES				"Expires"
#define HEADER_NNTP_POSTING_HOST	"NNTP-Posting-Host"
#define HEADER_PATH					"Path"
#define HEADER_POSTED_DATE			"Posted-Date"
#define HEADER_RECEIVED				"Received"
#define HEADER_RETURN_PATH			"Return-Path"
#define HEADER_RETURN_RECEIPT_TO	"Return-Receipt-To"
#define HEADER_X_FACE				"X-Face"
#define HEADER_X_SENDER				"X-Sender"

HeaderType
header_type (const char *header_name)
{
  switch (*header_name)
	{
	case 'a': case 'A':
	  if (!strcasecmp(header_name, HEADER_APPROVED))
		return HeaderAddress;
	  break;
	case 'b': case 'B':
	  if (!strcasecmp(header_name, HEADER_BCC))
		return HeaderAddress;
	  break;
	case 'c': case 'C':
	  if (!strcasecmp(header_name, HEADER_CC))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_CONTENT_BASE))
		return HeaderURL;
	  if (!strcasecmp(header_name, HEADER_CONTENT_LOCATION))
		return HeaderURL;
	  if (!strcasecmp(header_name, HEADER_CONTENT_LENGTH))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_CONTENT_TRANSFER_ENCODING))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_CONTENT_TYPE))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_CONTENT_ID))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_CONTENT_MD5))
		return HeaderIgnore;
	  break;
	case 'd': case 'D':
	  if (!strcasecmp(header_name, HEADER_DATE))
		return HeaderDate;
	  break;
	case 'e': case 'E':
	  if (!strcasecmp(header_name, HEADER_ERRORS_TO))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_EXPIRES))
		return HeaderDate;
	  break;
	case 'f': case 'F':
	  if (!strcasecmp(header_name, HEADER_FOLLOWUP_TO))
		return HeaderNewsgroup;
	  if (!strcasecmp(header_name, HEADER_FROM))
		return HeaderAddress;
	  break;
	case 'l': case 'L':
	  if (!strcasecmp(header_name, HEADER_LINES))
		return HeaderIgnore;
	  break;
	case 'm': case 'M':
	  if (!strcasecmp(header_name, HEADER_MESSAGE_ID))
		return HeaderID;
	  if (!strcasecmp(header_name, HEADER_MIME_VERSION))
		return HeaderIgnore;
	  break;
	case 'n': case 'N':
	  if (!strcasecmp(header_name, HEADER_NEWSGROUPS))
		return HeaderNewsgroup;
	  if (!strcasecmp(header_name, HEADER_NNTP_POSTING_HOST))
		return HeaderIgnore;
	  break;
	case 'p': case 'P':
	  if (!strcasecmp(header_name, HEADER_PATH))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_POSTED_DATE))
		return HeaderIgnore;
	  break;
	case 'r': case 'R':
	  if (!strcasecmp(header_name, HEADER_RECEIVED))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_REFERENCES))
		return HeaderID;
	  if (!strcasecmp(header_name, HEADER_REPLY_TO))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_RESENT_CC))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_RESENT_DATE))
		return HeaderDate;
	  if (!strcasecmp(header_name, HEADER_RESENT_FROM))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_RESENT_MESSAGE_ID))
		return HeaderID;
	  if (!strcasecmp(header_name, HEADER_RESENT_SENDER))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_RESENT_TO))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_RETURN_PATH))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_RETURN_RECEIPT_TO))
		return HeaderAddress;
	  break;
	case 's': case 'S':
	  if (!strcasecmp(header_name, HEADER_SENDER))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_SUPERSEDES))
		return HeaderID;
	  break;
	case 't': case 'T':
	  if (!strcasecmp(header_name, HEADER_TO))
		return HeaderAddress;
	  break;
	case 'x': case 'X':
	  if (!strncasecmp(header_name, "X-Mozilla-", 10))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_XREF))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_X_SENDER))
		return HeaderAddress;
	  if (!strcasecmp(header_name, HEADER_X_FACE))
		return HeaderIgnore;
	  if (!strcasecmp(header_name, HEADER_X_SENDER))
		return HeaderAddress;
	  if (!strncasecmp(header_name, "X-VM-", 5))	/* Hi Kyle */
		return HeaderIgnore;
	  if (!strncasecmp(header_name, "X-Sun-", 6))
		return HeaderIgnore;
	  break;
	}

  return HeaderText;
}
