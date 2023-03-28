/* -*- Mode: C; tab-width: 4 -*-
   pprint.c --- pretty-print the output of the mailbox parser.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 1-Aug-97.
   This takes a MIME object and pulls the leaf-nodes out of it, via libmime.
 */

#ifndef __PPRINT_H__
#define __PPRINT_H__

XP_BEGIN_PROTOS

extern int pprint_begin (int32 db_id,
						 MimeHeaders *headers,
						 const char *filename,
						 int32 file_position,
						 const char *urls,
						 const char *addrs_or_ids,
						 void *closure);

extern int pprint_attachment (const char *name,
							  const char *type,
							  const char *description,
							  const char *location,
							  const char *urls,
							  const char *addrs_or_ids,
							  const char *text,
							  int32 text_length,
							  int32 sub_message_db_id,
							  void *closure);

extern int pprint_end (void *closure);

XP_END_PROTOS

#endif /*  __PPRINT_H__ */
