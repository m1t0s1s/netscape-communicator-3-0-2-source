/* -*- Mode: C; tab-width: 4 -*-
   msgutils.h --- various and sundry
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 6-Jun-95.
 */



/* Interface to FE_GetURL, which should never be called directly by anything
   else in libmsg.  The "issafe" argument is TRUE if this is a URL which we
   consider very safe, and will never screw up or be screwed by any activity
   going on in the foreground.  Be very very sure of yourself before ever
   passing TRUE to the "issafe" argument. */

extern int msg_GetURL(MWContext* context, URL_Struct* url, XP_Bool issafe);


/* Interface to XP_InterruptContext(), which should never be called directly
   by anything else in libmsg.  The "safetoo" argument is TRUE if we really
   want to interrupt everything, even "safe" background streams.  In most
   cases, it should be False.*/

extern void msg_InterruptContext(MWContext* context, XP_Bool safetoo);



extern int msg_GrowBuffer (uint32 desired_size,
						   uint32 element_size, uint32 quantum,
						   char **buffer, uint32 *size);

extern int msg_LineBuffer (const char *net_buffer, int32 net_buffer_size,
						   char **bufferP, uint32 *buffer_sizeP,
						   uint32 *buffer_fpP,
						   XP_Bool convert_newlines_p,
						   int32 (*per_line_fn) (char *line, uint32
												 line_length, void *closure),
						   void *closure);
						   
extern int msg_ReBuffer (const char *net_buffer, int32 net_buffer_size,
						 uint32 desired_buffer_size,
						 char **bufferP, uint32 *buffer_sizeP,
						 uint32 *buffer_fpP,
						 int32 (*per_buffer_fn) (char *buffer,
												 uint32 buffer_size,
												 void *closure),
						 void *closure);

extern NET_StreamClass *msg_MakeRebufferingStream (NET_StreamClass *next,
												   URL_Struct *url,
												   MWContext *context);


extern void msg_RedisplayMessages(MWContext* context);

/* Given a string and a length, removes any "Re:" strings from the front.
   It also deals with that dumbass "Re[2]:" thing that some losing mailers do.

   Returns TRUE if it made a change, FALSE otherwise.

   The string is not altered: the pointer to its head is merely advanced,
   and the length correspondingly decreased.
 */
extern XP_Bool msg_StripRE(const char **stringP, uint32 *lengthP);
