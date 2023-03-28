/* -*- Mode: C; tab-width: 4 -*-
   mimemrel.h --- definition of the MimeMultipartRelated class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMREL_H_
#define _MIMEMREL_H_

#include "mimemult.h"
#include "xp_hash.h"


/* The MimeMultipartRelated class implements the multipart/related MIME 
   container, which allows `sibling' sub-parts to refer to each other.
 */

typedef struct MimeMultipartRelatedClass MimeMultipartRelatedClass;
typedef struct MimeMultipartRelated      MimeMultipartRelated;

struct MimeMultipartRelatedClass {
	MimeMultipartClass multipart;
};

extern MimeMultipartRelatedClass mimeMultipartRelatedClass;

struct MimeMultipartRelated {
	MimeMultipart multipart;	/* superclass variables */

	char* base_url;				/* Base URL (if any) for the whole
								   multipart/related. */

	char* head_buffer;			/* Buffer used to remember the text/html 'head'
								   part. */
	int32 head_buffer_fp;		/* Active length. */
	int32 head_buffer_size;		/* How big it is. */
	
	char *file_buffer_name;		/* The name of a temp file used when we
								   run out of room in the head_buffer. */
	XP_File file_stream;		/* A stream to it. */

	MimeHeaders* buffered_hdrs;	/* The headers of the 'head' part. */

	XP_Bool head_loaded;		/* Whether we've already passed the 'head'
								   part. */
	MimeObject* headobj;		/* The actual text/html head object. */

	XP_HashTable hash;			/* Conversion between URLs and part URLs. */

	int (*real_output_fn) (char *buf, int32 size, void *stream_closure);
	void* real_output_closure;

	char* curtag;
	int32 curtag_max;
	int32 curtag_length;



};

#endif /* _MIMEMREL_H_ */
