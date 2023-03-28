/* -*- Mode: C; tab-width: 4 -*-
   leaf.c --- extract text from MIME objects.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-Jul-97.
   This takes a MIME object and pulls the leaf-nodes out of it, via libmime.
 */

#ifndef __LEAF_H__
#define __LEAF_H__

typedef struct LeafParseObject LeafParseObject;

typedef enum { HeaderAddress,		/* contains only 822 mailboxes */
			   HeaderNewsgroup,		/* contains only newsgroup names */
			   HeaderID,			/* contains only message-IDs */
			   HeaderURL,			/* contains line-wrapped URLs */
			   HeaderText,			/* contains random text */
			   HeaderDate,			/* contains a date */
			   HeaderIgnore			/* should not be indexed */
} HeaderType;

XP_BEGIN_PROTOS

extern LeafParseObject *
leaf_init (const char *filename, int32 start_pos,
		   int (*write_message_begin) (int32 db_id,
									   MimeHeaders *headers,
									   const char *filename,
									   int32 file_position,
									   const char *urls,
									   const char *addrs_ids,
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
		   void *closure);
extern int leaf_write (LeafParseObject *pobj, char *buf, int32 size);
extern int leaf_done (LeafParseObject *pobj, XP_Bool abort_p);

extern HeaderType header_type (const char *header_name);

XP_END_PROTOS

#endif /*  __LEAF_H__ */
