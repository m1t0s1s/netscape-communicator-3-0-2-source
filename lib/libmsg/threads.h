/* -*- Mode: C; tab-width: 4 -*-
   threads.h --- prototypes for the threading module.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 14-May-95.
 */

#ifndef _THREADS_H_
#define _THREADS_H_

#include "msg.h"
#include "xp_hash.h"

struct msg_thread_state; /* opaque */

/* When we need to create/destry a synthetic ThreadEntry, we call these. */
typedef MSG_ThreadEntry * (*msg_DummyThreadEntryCreator) (void *arg);
typedef void (*msg_DummyThreadEntryDestroyer) (struct MSG_ThreadEntry *dummy,
											   void *arg);

XP_BEGIN_PROTOS

/* Creates and initializes a state object to hold the thread data.
   The first argument should be the caller's best guess as to how
   many messages there will be; it is used for the initial size of
   the hash tables, so it's ok for it to be too small.  The other
   arguments specify the sorting behavior.
 */
extern struct msg_thread_state *
msg_BeginThreading (MWContext* context,
					uint32 message_count_guess,
					XP_Bool news_p,
					MSG_SORT_KEY sort_key,
					XP_Bool sort_forward_p,
					XP_Bool thread_p,
					msg_DummyThreadEntryCreator dummy_creator,
					void *dummy_creator_arg);

/* Add a message to the thread data.
   The msg->next slot will be ignored (and not modified.)
 */
extern int
msg_ThreadMessage (struct msg_thread_state *state,
				   char **string_table,
				   struct MSG_ThreadEntry *msg);

/* Threads the messages, discards the state object, and returns a
   tree of MSG_ThreadEntry objects.
   The msg->next and msg->first_child slots of all messages will
   be overwritten.
 */
extern struct MSG_ThreadEntry *
msg_DoneThreading (struct msg_thread_state *state,
				   char **string_table);

/* Given an existing tree of MSG_ThreadEntry structures, re-sorts them.
   This changes the `next' and `first_child' links, and may create or
   destroy "dummy" thread entries (those with the EXPIRED flag set.)
   The new root of the tree is returned.
 */
extern struct MSG_ThreadEntry *
msg_RethreadMessages (MWContext* context,
					  struct MSG_ThreadEntry *tree,
					  uint32 approximate_message_count,
					  char **string_table,
					  XP_Bool news_p,
					  MSG_SORT_KEY sort_key, XP_Bool sort_forward_p,
					  XP_Bool thread_p,
					  msg_DummyThreadEntryCreator dummy_creator,
					  msg_DummyThreadEntryDestroyer dummy_destroyer,
					  void *dummy_arg);

XP_END_PROTOS

#endif /* _THREADS_H_ */
