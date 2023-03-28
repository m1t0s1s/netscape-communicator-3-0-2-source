/* -*- Mode: C; tab-width: 4 -*-
   mbox.c --- BSD/SYSV mailbox file parser.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Jul-97.

   This sectionizes a mailbox file into its component messages; it understands
   both the BSD and Solaris formats ("From " and "Content-Length".)  See also
   http://www.netscape.com/eng/mozilla/2.0/relnotes/demo/content-length.html
   for related ranting.
 */

#ifndef __MBOX_H__
#define __MBOX_H__

extern int
map_over_messages (FILE *in, const char *filename,
				   void * (*start_msg) (const char *filename,
										int32 start_pos,
										void *closure),
				   int (*write_block) (void *closure, char *buf, int32 size),
				   int (*close_msg) (void *closure, Bool error_p),
				   void *closure);

#endif /*  __MBOX_H__ */
