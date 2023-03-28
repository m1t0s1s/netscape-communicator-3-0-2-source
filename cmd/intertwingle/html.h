/* -*- Mode: C; tab-width: 4 -*-
   html.c --- extract more-or-less plain-text from HTML
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 28-Jul-97
 */

#ifndef __HTML_H__
#define __HTML_H__

XP_BEGIN_PROTOS

/* Strips HTML tags out of the given buffer, modifying it in place (since
   the resultant text will be the same size or smaller.)

   Any HREFs in the text will be returned to `urls_return' -- this
   will be a double-null-terminated list of URLs of the form
   "url-A\000url-b\000url-C\000\000".

   If `base' is provided, it is the default base for relative URLs in the
   document; any <BASE> tag in the document will override this, and will
   be returned in its place.

   Returns negative on failure (but `buf' may already have been destroyed.)
 */
extern int html_to_plaintext (char *buf, int32 *sizeP, char **baseP,
							  char **urls_return);

/* Strips out overstriking/underlining (for example "_^Hf_^Ho_^Ho" => "foo".)
 */
extern int strip_overstriking (char *buf, int32 *sizeP);


/* Searches for URLs in text/plain documents; returns the same format as
   html_to_plaintext() does.
 */
extern int search_for_urls (const char *buf, int32 size,
							char **result, int32 *result_size,
							int32 *result_fp);

extern int search_for_addrs_or_ids (const char *buf, int32 size,
									char **result, int32 *result_size,
									int32 *result_fp);

XP_END_PROTOS

#endif /*  __HTML_H__ */
