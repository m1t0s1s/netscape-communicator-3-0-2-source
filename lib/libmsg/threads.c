/* -*- Mode: C; tab-width: 4 -*-
   thread.c --- threading and sorting.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 13-Jun-95.
 */

#include "msg.h"
#include "threads.h"


extern int MK_OUT_OF_MEMORY;


/*
  When threading, we have as input a stream of message headers (basically.)
  They may be in any order, and we need to thread them correctly anyway.
  We need to be very careful about memory consumption, since we're potentially
  dealing with a lot of data.

  for each set of message headers as they arrive:

   - make a `struct MSG_ThreadEntry' for it.
   - If the message ID is not in the referer_table already:
     - make a `struct msg_container' for it, containing the MSG_ThreadEntry.
     - enter the container in the referer_table under the message ID.
   - else if it is in the table:
     - stuff the MSG_ThreadEntry into the existing container.

   - for each of the references of the message:
     - If the reference ID is not in the referer_table already:
       - make an empty `struct msg_container' for it.
       - enter the container in the referer_table under the reference ID.
     - make its parent be the next reference in the chain.

   - if the (new) container has a parent already, replace that parent with the
     parent according to the "real" references field (the older parent was
     assumed from a references field of another message, and is less correct.)

  when we've got them all:
   - compute the root set:
     - for each entry in the hash table:
       - if there is no parent, this entry is in the root set.

   - iterate over each message object in the root set.
     - if that msg_container is empty, and has only one child,
       then free the container and promote its children to siblings;
       adjust their parent slots accordingly.
     - else, iterate over each of its children
       - if the child has no MSG_ThreadEntry (it's a dummy) then
         promote it's children to be siblings, adjust their parent
         slots, and free this child.
       - recurse this step on the children.

  Now we've got a tree which can only have messageless msg_container objects
  in the root set, not lower down in the tree.  And if there is a messageless
  msg_container, then it has at least 2 children.

  Now we need to gather together messages with no referers but the same
  subjects.

  - For each message in the root set:
    - hash subjects to the first member of the root set with that subject.

  - For each message in the root set which does not have references:
    - look up its subject in the table
    - if the value hashed with that subject is not this message,
      make this message be a child of that msg_container (removing
      it from the root set.)

  At this point, we've reduced the size of the root set, and increased the
  number of children at depth 1.  Only links have moved - nothing has been
  allocated or freed.

  Now, we are done threading, and can get rid of the msg_container objects,
  and end up with only MSG_ThreadEntry structures that are linked together.

  PROBLEMS:
  ========
   This might use more memory than it should.  Having a structure and hash
   table entry for every referenced message worries me.  Having a hash table
   entry for every subject worries me.

   It seems like it should be possible to do this in fewer passes, but that
   probably wouldn't be a tangible win, and might complicate the code more.

   The failure modes in out-of-memory conditions need to be examined carefully.
   We should endeavor to give the user as much data as we can when we fail,
   rather than simply not presenting anything.

   In a couple of places, I use real call-stack-based recursion, getting as
   deep as the number of existing messages in a thread.  This is almost
   guarenteed to blow up on the toy computers.  I guess those routines should
   be rewritten to use malloc and iteration.  Oh boy.

   - Try to combine the msg_container and MSG_ThreadEntry structs?
     Let the `next' and `first_child' slots overlap at the beginning,
	 and simply allocate smaller blocks for containers?
   - Once we've got a MSG_ThreadEntry, we might be able to find a way
     to do without the msg_container.  Need a way to tell them apart.
	 Need a way to replace the parental pointer to the container.
 */

#define USE_XP_ALLOCSTRUCT

struct msg_container
{
  struct MSG_ThreadEntry *msg;
  struct msg_container *parent;
  struct msg_container *first_child;
  struct msg_container *next;
};


struct msg_thread_state_extrastrings {
  char* str;
  struct msg_thread_state_extrastrings* next;
};

struct msg_thread_state
{
  MWContext* context;
  XP_HashTable id_table;
  XP_HashTable subject_table;

  /* Usually, the keys we put in the hash table are already existing.
	 Occasionally, however, we have to cons up a new, bogus string.  In
	 that case, we write the strings down in the extrastrings struct,
	 so that we can free their storage when we're all done.  Sigh. ###*/

  struct msg_thread_state_extrastrings* extrastrings;

  struct msg_container *root_containers;
  struct MSG_ThreadEntry *root_messages;

  msg_DummyThreadEntryCreator dummy_creator;
  void *dummy_creator_arg;

  char **string_table;  /* Don't use this until msg_DoneThreading() has
						   been called, since it may be relocated between
						   calls to msg_ThreadMessage(). */

  MSG_SORT_KEY sort_key;
  XP_Bool sort_forward_p;
  XP_Bool thread_p;

  XP_Bool news_p;		/* Whether the MSG_ThreadEntry structures represent
						   mail articles or news articles, so that we know
						   which part of the `data' union to look at when
						   sorting by message number. */

  XP_AllocStructInfo container_blocks;
};


#define msg_new_container(STATE) \
 (struct msg_container *) XP_AllocStruct (&(STATE)->container_blocks)

#define msg_free_container(STATE,CONTAINER) \
 XP_FreeStruct (&(STATE)->container_blocks, CONTAINER)


# ifndef DEBUG

#  define msg_insert_container(LISTP,NEWE,TABLE,KEY,FORWARD) \
    ((NEWE)->next = *(LISTP), *(LISTP) = (NEWE))

# else /* DEBUG */

static void
msg_insert_container (struct msg_container **listP,
					  struct msg_container *new_entry,
					  char **table, MSG_SORT_KEY key, XP_Bool forward_p)
{
  XP_ASSERT (new_entry);
  if (! new_entry) return;
  XP_ASSERT (!new_entry->next);
  new_entry->next = *listP;
  *listP = new_entry;
}
# endif /* DEBUG */


/* Returns TRUE if `child' is a descendant of `parent', at any depth.
   This is used for detecting circularities (and assumes the tree is
   not yet circular.)
 */
static XP_Bool
msg_find_child (struct msg_container *parent, struct msg_container *child)
{
  struct msg_container *rest;
 RECURSE:
  if (!parent || !child) return FALSE;
  for (rest = parent->first_child; rest; rest = rest->next)
	{
	  if (rest == child)
		return TRUE;
	  else if (rest->first_child)
		{
		  if (!rest->next)
			/* If this is the last item in the list, we don't have to recurse,
			   we can do a tail-call.  I can't believe I'm using compilers so
			   crummy that I have do this by hand. */
			{
			  parent = rest;
			  goto RECURSE;
			}
		  else
			{
			  /* #### toy-computer-danger, real recursion. */
			  if (msg_find_child (rest, child))
				return TRUE;
			}
		}
	}
  return FALSE;
}


/* Adds a message to the table, and allocates various pieces of data.
   Actual threading happens at the end.
 */
int
msg_ThreadMessage (struct msg_thread_state *state,
				   char **string_table,
				   struct MSG_ThreadEntry *msg)
{
  struct msg_container *container;
  struct msg_container *parent_container;
  char* idstr;

  XP_ASSERT (msg);
  if (! msg) return -1;
  XP_ASSERT (!msg->first_child);
  /* Note that this pass (ThreadMessage) does not use msg->next;
	 that is overridden by the last pass (DoneThreading.) */
  XP_ASSERT (!msg->first_child);
  msg->first_child = 0;
  msg->next = 0;

  XP_ASSERT (msg->id > 0);

  /* If we're not threading, don't bother using the hash tables at all.
	 #### Actually, we could further optimize this case by not using
	 msg_container structures, as we use exactly as many of those as
	 we use MSG_ThreadEntry structures.  We could do the work of
	 msg_relink_threads() directly in the first pass, and use less
	 memory (and no intermediate state.)
   */

  idstr = string_table[msg->id];

  if (!state->thread_p)
	container = 0;
  else
	container = (struct msg_container *)
	  XP_Gethash (state->id_table, idstr, 0);

  if (container && container->msg) {
	/* Humph.  There's another message out there with the same message-id */
	/* as this one.  For now (it's wrong, but...) we'll cons up a new */
	/* place in the hash table to use. ###HACK### */
	int count = 0;
	char buf[15];
	struct msg_thread_state_extrastrings* tmp;

	while (container) {	
	  PR_snprintf(buf, sizeof(buf), "<Bogus-id:%d>", count++);
	  container = (struct msg_container *)
		XP_Gethash (state->id_table, buf, 0);
	}
	idstr = XP_STRDUP(buf);
	if (!idstr) return MK_OUT_OF_MEMORY;
	tmp = XP_NEW_ZAP(struct msg_thread_state_extrastrings);
	if (!tmp) {
	  XP_FREE(idstr);
	  return MK_OUT_OF_MEMORY;
	}
	tmp->str = idstr;
	tmp->next = state->extrastrings;
	state->extrastrings = tmp;
  }
	  

  if (! container)
	{
	  container = msg_new_container (state);
	  if (! container)
		return MK_OUT_OF_MEMORY;
	  container->msg = msg;
	  container->parent = 0;
	  container->first_child = 0;
	  container->next = 0;

	  if (state->thread_p && !(msg->flags & MSG_FLAG_EXPUNGED))
		if (XP_Puthash (state->id_table,
						(void *) idstr,
						(void *) container)
			< 0)
		  {
			msg_free_container (state, container);
			return MK_OUT_OF_MEMORY;
		  }
	}
  else	/* there is a container already, meaning that we have seen a
		   forward-reference to this message (child preceeds parent.) */
	{
	  container->msg = msg;
	}

  /* Make entries in the hash table for each of the referred messages,
	 and link them to each other.
   */
  parent_container = 0;
  if (state->thread_p && msg->references)
	{
	  uint16 *references = msg->references;
	  while (*references)
		{
		  struct msg_container *ref = (struct msg_container *)
			XP_Gethash (state->id_table, string_table [*references], 0);

		  if (!ref)
			{
			  ref = msg_new_container (state);
			  if (! ref) return MK_OUT_OF_MEMORY;
			  ref->msg = 0;
			  ref->parent = 0;
			  ref->first_child = 0;
			  ref->next = 0;

			  if (XP_Puthash (state->id_table,
							  (void *) string_table [*references],
							  (void *) ref)
				  < 0)
				{
				  msg_free_container (state, ref);
				  return MK_OUT_OF_MEMORY;
				}
			}

		  /* If we have references A B C D, make D be a child of C, etc,
			 except if they have parents already. */
		  if (parent_container &&
			  parent_container != ref &&
			  !ref->parent &&
			  !msg_find_child (ref, parent_container)) /* avoid loop */
			{
			  XP_ASSERT (! ref->next);  /* no parent, so no next. */
			  ref->parent = parent_container;

			  msg_insert_container (&parent_container->first_child,
									ref, string_table, state->sort_key,
									state->sort_forward_p);
			}

		  parent_container = ref;
		  references++;
		}
	}

  /* Detect a potential circularity: if we have a parent container, we're
	 about to make this container be it's child.  If by walking forward in
	 this container's children, we can reach the parent container, then
	 making that link would create a loop.  So, in that case, throw away
	 this link to the parent.  This linearizes the loop at some random
	 place.
   */
  if (parent_container && msg_find_child (container, parent_container))
	parent_container = 0;

  if (container->parent)
	{
	  /* If it has a parent already, that's there because we saw this message
		 in a references field, and presumed a parent based on the other
		 entries in that field.  Now that we have the actual message, we can
		 be more definitive, so throw away the old parent and use this new one.
		 Find this container in the parent's child-list, and remove it.

		 Note that this could cause this message to now have no parent, if it
		 has no references field, but some message referred to it as the
		 non-first element of its references.
	   */
	  struct msg_container *rest, *prev;
	  for (prev = 0, rest = container->parent->first_child;
		   rest;
		   prev = rest, rest = rest->next)
		if (rest == container)
		  break;
	  XP_ASSERT (rest);   /* we had better have found it in the parent... */
	  if (rest)
		{
		  if (prev)
			prev->next = container->next;
		  else
			container->parent->first_child = container->next;
		  container->next = 0;
		  container->parent = 0;
		}
	}

  container->parent = parent_container;

  if (parent_container)		/* the last reference in the list */
	{
	  msg_insert_container (&parent_container->first_child, container,
							string_table, state->sort_key,
							state->sort_forward_p);
	}
  else if (! state->thread_p)
	{
	  /* Only when threading is on do we populate the root set during the
		 first pass (since the root set is the *only* list -- no container
		 has anything in its first_child slot.)  When threading, the root
		 set is populated at the end by searching the hash table for
		 containerless messages, but when not threading, we don't even
		 allocate the hash tables.
	   */
	  msg_insert_container (&state->root_containers, container,
							string_table, state->sort_key,
							state->sort_forward_p);
	}

  return 0;
}


#if defined(DEBUG) && !defined(XP_WIN)
extern void msg_print_thread_entry (struct MSG_ThreadEntry *msg,
									char **string_table, uint32 depth);

static void
msg_print_tree (struct msg_container *container, char **string_table,
				uint32 depth, struct msg_container *parent)
{
  struct msg_container *rest = container;
  while (rest)
	{
	  XP_ASSERT (rest->parent == parent);
	  if (rest->msg)
		msg_print_thread_entry (rest->msg, string_table, depth);
	  else
		{
		  struct MSG_ThreadEntry dummy;
		  struct msg_container *x;
		  dummy.flags = MSG_FLAG_EXPIRED;
		  for (x = rest->first_child; x && !x->msg; x = x->first_child)
			;
		  dummy.subject = (x ? x->msg->subject : 0);
		  msg_print_thread_entry (&dummy, string_table, depth);
		}
	  if (rest->first_child)
		msg_print_tree (rest->first_child, string_table, depth + 1, rest);
	  rest = rest->next;
	}
}
#endif


/* Walk through the table and find all messages with no parents.
   Gather those into state->root_containers.
 */
static XP_Bool
msg_gather_root_mapper (XP_HashTable table,
						const void *key, void *value,
						void *closure)
{
  struct msg_container *container = (struct msg_container *) value;
  struct msg_thread_state *state = (struct msg_thread_state *) closure;
  XP_ASSERT (container);
  if (! container)
	return TRUE;
  if (container->parent)
	return TRUE;
  XP_ASSERT (!container->next); /* If it has no parent, it has no next. */

  msg_insert_container (&state->root_containers, container,
						state->string_table, state->sort_key,
						state->sort_forward_p);
  return TRUE;
}


static int
msg_compute_root_set (struct msg_thread_state *state)
{
  XP_ASSERT (! state->root_containers);
  XP_MapRemhash (state->id_table, msg_gather_root_mapper, (void *) state);
  XP_HashTableDestroy (state->id_table);
  state->id_table = 0;
  return 0;
}


/* Helper for msg_clean_threads(), below. */
static int
msg_clean_thread (struct msg_thread_state *state,
				  struct msg_container **chain_start, XP_Bool root_p)
{
  struct msg_container *container, *prev, *next;
 RECURSE:
  for (prev = 0, container = *chain_start, next = container->next;
	   container;
	   prev = container, container = next,
		 next = container ? container->next : 0)
	{
	  if (!container->msg && !container->first_child)
		/* Expired message, no kids.  Nuke it. */
		{
		  if (prev)
			prev->next = container->next;
		  else
			*chain_start = container->next;

		  /* We free all of the containers at the end in one pass, so it's
			 not strictly necessary to free it here; however, we will be
			 allocating memory after this (when we thread by subject) so
			 it would be good to free the containers now, since there are
			 a lot of them, and freeing them could make necessary room for
			 the hash table.

			 #### Idea: if threading by subject, and sorting, operated on
			 MSG_ThreadEntry() objects instead of msg_container objects,
			 then we could free *all* of the containers after
			 msg_clean_threads() -- actually, combine msg_clean_threads()
			 and msg_relink_threads() into one pass.
		   */
		  msg_free_container (state, container);

		  /* Set container to prev so that prev keeps its same value
			 the next time through the loop. */
		  container = prev;
		}
	  else if (!container->msg &&
			   container->first_child &&
			   (!root_p || !container->first_child->next))
		/* Expired root message, 1 kid; or, expired non-root message,
		   any number of kids.  Promote the kids to this level.
		 */
		{
		  struct msg_container *kids = container->first_child;
		  struct msg_container *tail;
		  if (prev)
			prev->next = kids;
		  else
			*chain_start = kids;

          /* Win16 lossage alert!
             If the middle clause is just 'tail->next' the optimizer
             will run the loop until tail is NULL.  Adding the redundant
             test makes life all better.  Whatever.  chouck 26-Aug-95 */
		  for (tail = kids; tail && tail->next; tail = tail->next)
			{
			  XP_ASSERT (tail->parent == container);
			  tail->parent = container->parent;
			}
		  XP_ASSERT (tail->parent == container);
		  tail->parent = container->parent;
		  tail->next = container->next;

		  /* Since we've inserted items in the chain, `next' currently points
			 to the item after them; reset that so that we process the newly
			 promoted items the very next time around. */
		  next = kids;

		  /* See comment above previous call to msg_free_container(). */
		  msg_free_container (state, container);

		  /* Set container to prev so that prev keeps its same value
			 the next time through the loop. */
		  container = prev;
		}
	  else if (container->first_child)
		/* A real message with kids; or an expired message with 2 or more kids.
		   Iterate over its children, and try to strip out the junk.
		 */
		{
		  if (! next)
			/* If this is the last item in the list, we don't have to recurse,
			   we can do a tail-call.  I can't believe I'm using compilers so
			   crummy that I have do this by hand. */
			{
			  chain_start = &container->first_child;
			  root_p = FALSE;
			  goto RECURSE;
			}
		  else
			{
			  /* #### toy-computer-danger, real recursion. */
			  int status = msg_clean_thread (state,
											 &container->first_child, FALSE);
			  if (status < 0)
				return status;
			}
		}
	}

  return 0;
}


/* Walk through the thread and discard any empty container objects.
   After calling this, there will only be empty container objects
   at depth 0, and those will all have at least two kids.
 */
static int
msg_clean_threads (struct msg_thread_state *state)
{
  int status = 0;
  if (state->root_containers)
	status = msg_clean_thread (state, &state->root_containers, TRUE);
/*  XP_HashTableDestroy (state->id_table);
  state->id_table = 0; */
  return status;
}

/* Given a message tree that is already threaded by references,
   this attempts to merge root sets which have common subjects.
 */
static int
msg_thread_by_subject (struct msg_thread_state *state)
{
  int32 subject;
  struct msg_container *container, *prev, *next;

  if (! state->root_containers)
	return 0;

  /* Iterate over the root set, and hash every subject to the root thread.
   */
  for (container = state->root_containers, next = container->next;
	   container;
	   container = next, next = container ? container->next : 0)
	{
	  struct msg_container *old;
	  if (!container->msg)
		/* A dummy message in the root set.  Save its subject. */
		{
		  /* Only root-set members may be dummies at this point, and all
			 dummies have at least two kids. */
		  XP_ASSERT (container->first_child && container->first_child->msg);
		  if (container->first_child && container->first_child->msg)
			subject = container->first_child->msg->subject;
		}
	  else
		/* A real message in the root set. */
		{
		  subject = container->msg->subject;
		}

	  old = (struct msg_container *) XP_Gethash (state->subject_table,
												 (void *) subject, 0);

	  /* Add this message to the hash table if:
		 - there is no message in the table with this subject, or
		 - the message in the table has a "Re:" version of this subject,
		   and this message has a non-"Re:" version of this subject.
		   The non-re version is more interesting.
		 - this one is "empty" and the old one is not - the empty one is
		   more interesting as a "root", so put it in the table instead.
		*/
	  if (!old ||
		  (old->msg && !container->msg) ||
		  (old->msg       &&  (old->msg->flags       & MSG_FLAG_HAS_RE) &&
		   container->msg && !(container->msg->flags & MSG_FLAG_HAS_RE))
		  )
		if (XP_Puthash (state->subject_table,
						(void *) subject, (void *) container)
			< 0)
		  {
			XP_HashTableDestroy (state->subject_table);
			state->subject_table = 0;
			return MK_OUT_OF_MEMORY;
		  }
	}

  /* Iterate over the root set, and gather together referenceless messages
	 with the same subject.
   */
  for (prev = 0, container = state->root_containers, next = container->next;
	   container;
	   prev = container, container = next,
		 next = container ? container->next : 0)
	{
	  struct msg_container *old;

#if 0
	  if (! container->msg)		/* it's a dummy. */
		continue;
#endif

#if 0
	  if (container->msg->references)	/* It has references, so we don't need
										   to play stupid subject games. */
		continue;
#endif

	  /* This message is in the root set, but has no references field.
		 Let's try to find another message with the same subject and
		 make this be a child or sibling of that. */

	  if (!container->msg)
		{
		  /* Only root-set members may be dummies at this point, and all
			 dummies have at least two kids. */
		  XP_ASSERT (container->first_child && container->first_child->msg);
		  if (container->first_child && container->first_child->msg)
			subject = container->first_child->msg->subject;
		}
	  else
		/* A real message in the root set. */
		{
		  subject = container->msg->subject;
		}

	  /* Don't thread together all subjectless messages; let them dangle. */
	  if (subject == 0)
		continue;
	  if (!XP_STRCMP (state->string_table[subject],
					  "(no subject)"))  /* ####i18n */
		continue;

	  old = (struct msg_container *) XP_Gethash (state->subject_table,
												 (void *) subject, 0);
	  XP_ASSERT (old);
	  if (! old) continue;

	  if (old == container)				/* oops, that's us */
		continue;

	  /* Ok, so now we have found another container in the root set with
		 the same subject.  There are a few possibilities:

		 - If both are empty, append one's children to the other, and remove
		   the now-empty container.

		 - If one container is empty and the other is not, make the non-empty
		   one be a child of the other, and a sibling of the other "real"
		   messages with the same subject.

		 - If that container is a message, and that message's subject does
		   not begin with "Re:", but *this* message's subject does, then
		   make this be a child of the other.

		 - If that container is a message, and that message's subject begins
		   with "Re:", but *this* message's subject does *not*, then make that
		   be a child of this one - they were misordered.  (This happens
		   somewhat implicitly, since if there are two messages, one with Re:
		   and one without, the one without will be in the hash table,
		   regardless of the order in which they were seen.)

		 - Otherwise, make a new empty container and make both messages be a
		   child of it.  This catches the both-are-replies and neither-are-
		   replies cases, and makes them be siblings instead of asserting a
		   hierarchical relationship which might not be true.

		   (People who reply to messages without using "Re:" and without using
		   a References line will break this slightly.  Those people suck.)
	   */

	  /* Remove the "second" message from the root set. */
	  if (prev)
		prev->next = container->next;
	  else
		state->root_containers = container->next;

	  container->next = 0;

	  if (!old->msg && !container->msg)
		/* They're both empty; merge them.
		 */
		{
		  struct msg_container *rest;
		  XP_ASSERT (!old->parent);
		  XP_ASSERT (!container->parent);
		  XP_ASSERT (old->first_child->msg->subject ==
					 container->first_child->msg->subject);
		  for (rest = old->first_child; rest && rest->next; rest = rest->next)
			XP_ASSERT (rest->parent == old);
		  rest->next = container->first_child;
		  for (rest = container->first_child; rest; rest = rest->next)
			{
			  XP_ASSERT (rest->parent == container);
			  rest->parent = old;
			}
		  container->first_child = 0;
		}
	  else if (!old->msg ||							/* old is empty */
			   (container->msg &&
				/* new has Re: and old doesn't... */
				(container->msg->flags & MSG_FLAG_HAS_RE) &&
				(! (old->msg->flags & MSG_FLAG_HAS_RE))
				))
		/* Make this message be a child of the other.
		 */
		{
		  XP_ASSERT (!container->parent); /* It came from the root set. */
		  container->parent = old;		  /* It is now to be a child. */
		  msg_insert_container (&old->first_child, container,
								state->string_table, state->sort_key,
								state->sort_forward_p);
		}
	  else
		/* Make the old and new messages be children of a new empty container.
		   We do this by creating a new container object for old->msg and
		   emptying the old container, so that the hash table still points
		   to the one that is at depth 0 instead of depth 1.
		 */
		{
		  struct msg_container *rest;
		  struct msg_container *newc = msg_new_container (state);
		  if (! newc) return MK_OUT_OF_MEMORY;
		  XP_MEMSET (newc, 0, sizeof(*newc));
		  /* old keeps its `next' and `parent' (= 0), but not its `msg' or
			 its `first_child'. */
		  newc->msg = old->msg;
		  newc->first_child = old->first_child;
		  for (rest = newc->first_child; rest; rest = rest->next)
			{
			  XP_ASSERT (rest->parent == old);
			  rest->parent = newc;
			}
		  old->msg = 0;
		  old->first_child = 0;

		  XP_ASSERT (!container->parent); /* It came from the root set. */
		  container->parent = old;
		  newc->parent = old;

		  XP_ASSERT (newc->msg);
		  XP_ASSERT (container->msg);
		  msg_insert_container (&old->first_child, newc,
								state->string_table, state->sort_key,
								state->sort_forward_p);
		  msg_insert_container (&old->first_child, container,
								state->string_table, state->sort_key,
								state->sort_forward_p);

		  /* `old' will now have exactly two children. */
		}

	  container = prev; /* keep the same value for "prev" next time around. */
	}

  XP_HashTableDestroy (state->subject_table);
  state->subject_table = 0;
  return 0;
}


/* Helpers for msg_relink_threads(), below. */

static MSG_ThreadEntry *
msg_make_dummy (struct msg_thread_state *state,
				struct msg_container *container)
{
  struct MSG_ThreadEntry *msg;
  if (state->dummy_creator) {
	msg = (*state->dummy_creator) (state->dummy_creator_arg);
  } else {
	 msg = XP_NEW (struct MSG_ThreadEntry);
	 if (msg) XP_MEMSET (msg, 0, sizeof (*msg));
  }
  if (! msg) return 0;
  msg->flags |= MSG_FLAG_EXPIRED;
  XP_ASSERT (container->first_child && container->first_child->msg);

#if 0
  /* Copy the subject of the first child into the dummy.
	 This is done again later, to more slots than subject,
	 in msg_sort_threads(), so that a dummy container is
	 sorted as its first child.  But we need to set the
	 subject early so that we can gather together disjoint
	 sub-trees with the same subject.
   */
  if (container->first_child && container->first_child->msg)
	msg->subject = container->first_child->msg->subject;
#endif

  return msg;
}

static int
msg_relink_threads_1 (struct msg_thread_state *state,
					  struct msg_container *chain_start,
					  uint32 depth)
{
  struct msg_container *container, *next;
 RECURSE:
  for (container = chain_start, next = container->next;
	   container;
	   container = next, next = container ? container->next : 0)
	{
	  struct msg_container *cnext = container->next;
	  struct msg_container *ckid = container->first_child;
	  struct MSG_ThreadEntry *msg = container->msg;

	  /* empty containers should only occur at top-level. */
	  XP_ASSERT (depth == 0 || msg);

	  /* containers at top-level shouldn't have parents; others should. */
	  XP_ASSERT (depth == 0 ? (!container->parent) : (!!container->parent));

	  /* If the container is empty, at this point, we need to make an
		 empty message to go along with it. */
	  if (! msg)
		{
		  msg = msg_make_dummy (state, container);
		  if (! msg) return MK_OUT_OF_MEMORY;
		}

#if 0
	  /* We probably don't need to bother freeing the containers here;
		 the only reason to do that would be to reduce the local maximum
		 of allocated memory, but since we don't allocate much memory here
		 (at most one MSG_ThreadEntry for each empty msg_container at top-
		 level, which shouldn't be many) the overhead of freeing the
		 container is probably not worth it.
	   */
	  msg_free_container (state, container);
#endif

/*	  msg->depth = depth;*/

	  if (cnext)
		{
		  /* empty containers should only occur at top-level. */
		  XP_ASSERT (depth == 0 || cnext->msg);
		  /* If the next container is empty, at this point, we need to make an
			 empty message to go along with it. */
		  if (! cnext->msg)
			{
			  cnext->msg = msg_make_dummy (state, cnext);
			  if (! cnext->msg) return MK_OUT_OF_MEMORY;
			}

		  msg->next = cnext->msg;
		}

	  if (ckid)
		{
		  /* empty containers should only occur at top-level. */
		  XP_ASSERT (ckid->msg);
		  /* If the child container is empty, at this point, we need to make an
			 empty message to go along with it. */
		  if (! ckid->msg)
			{
			  ckid->msg = msg_make_dummy (state, ckid);
			  if (! ckid->msg) return MK_OUT_OF_MEMORY;
			}

		  msg->first_child = ckid->msg;

		  if (!next)
			/* If this is the last item in the list, we don't have to recurse,
			   we can do a tail-call.  I can't believe I'm using compilers so
			   crummy that I have do this by hand. */
			{
			  depth++;
			  chain_start = ckid;
			  goto RECURSE;
			}
		  else
			{
			  /* #### toy-computer-danger, real recursion. */
			  msg_relink_threads_1 (state, ckid, depth + 1);
			}
		}
	}

  return 0;
}


/* This chains all of the MSG_ThreadEntry objects together in an identical
   structure to the msg_container objects which point to them; and then
   frees all of the msg_container objects.
 */
static int
msg_relink_threads (struct msg_thread_state *state)
{
  int status;
  if (!state->root_containers)
	return 0;

  if (!state->root_containers->msg)
	{
	  state->root_containers->msg = msg_make_dummy (state,
													state->root_containers);
	  if (!state->root_containers->msg)
		return MK_OUT_OF_MEMORY;
	}

  state->root_messages = state->root_containers->msg;

  status = msg_relink_threads_1 (state, state->root_containers, 0);
  if (status >= 0)
	state->root_containers = 0;
  return status;
}


/* Comparison functions to be passed to qsort.  These are individual
   functions so that they run real fast and stuff.

   Those comparison functions which need to compare strings need to
   get at the string table.  But since qsort doesn't give us a closure
   argument, we need to pass the table in via a global.  Doesn't that
   suck a great deal?  I think so.  This will be a problem if we ever
   have multiple threads at the C level, which I predict we won't.
 */

static char **msg_qsort_string_table_kludge = 0;


/* This needs to compare unsigned longs and return an int, so even on
   systems where int and long are the same size, care must be taken to not
   get roundoff errors when the result fits in a unsigned int-or-long, but
   not in a signed int.  If we only needed to compare ints (or longs) then
   we could do
       #if sizeof(int) == sizeof(long)
       # define COMPARE_INT(x,y) (int) ((x) - (y))
   but the sign bit screws us.
 */
#define COMPARE_INT(x,y) \
  (int) ((unsigned long) (x) == (unsigned long) (y) ? 0 : \
		 (unsigned long) (x) >  (unsigned long) (y) ? 1 : -1)

static int
msg_compare_date_forward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  XP_ASSERT (ma && mb);
  return (COMPARE_INT (ma->date,
					   mb->date));
}

static int
msg_compare_date_backward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  XP_ASSERT (a && b);
  return (COMPARE_INT (mb->date,
					   ma->date));
}

static int
msg_compare_mail_msg_number_forward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  XP_ASSERT (ma && mb);
  return (COMPARE_INT (ma->data.mail.file_index,
					   mb->data.mail.file_index));
}

static int
msg_compare_mail_msg_number_backward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  XP_ASSERT (ma && mb);
  return (COMPARE_INT (mb->data.mail.file_index,
					   ma->data.mail.file_index));
}

static int
msg_compare_news_msg_number_forward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  XP_ASSERT (ma && mb);
  return (COMPARE_INT (ma->data.news.article_number,
					   mb->data.news.article_number));
}

static int
msg_compare_news_msg_number_backward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  XP_ASSERT (ma && mb);
  return (COMPARE_INT (mb->data.news.article_number,
					   ma->data.news.article_number));
}

/* Compare two string fields in two MSG_ThreadEntry structures.  If they
   are equal, then use the date fields instead. */
#define INTL_SORT 1	/* Added by ftang */
#ifdef INTL_SORT
#include "xplocale.h"
#define COMPARE_STRING_FIELDS(a, b, field)						 \
{																 \
  int result;													 \
  XP_ASSERT(a && b && msg_qsort_string_table_kludge);			 \
  /* #### this is the i18n version of strcasecomp */			 \
  result = XP_StrColl(msg_qsort_string_table_kludge[a->field],	 \
					   msg_qsort_string_table_kludge[b->field]); \
  return result ? result : COMPARE_INT(a->date, b->date);		 \
}
#else
#define COMPARE_STRING_FIELDS(a, b, field)						 \
{																 \
  int result;													 \
  XP_ASSERT(a && b && msg_qsort_string_table_kludge);			 \
  /* #### use i18n version of strcasecomp */					 \
  result = strcasecomp(msg_qsort_string_table_kludge[a->field],	 \
					   msg_qsort_string_table_kludge[b->field]); \
  return result ? result : COMPARE_INT(a->date, b->date);		 \
}
#endif

static int
msg_compare_subject_forward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  COMPARE_STRING_FIELDS(ma, mb, subject);
}

static int
msg_compare_subject_backward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  COMPARE_STRING_FIELDS(mb, ma, subject);
}

static int
msg_compare_sender_forward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  COMPARE_STRING_FIELDS(ma, mb, sender);
}

static int
msg_compare_sender_backward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  COMPARE_STRING_FIELDS(mb, ma, sender);
}

static int
msg_compare_recipient_forward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  COMPARE_STRING_FIELDS(ma, mb, recipient);
}

static int
msg_compare_recipient_backward (const void *a, const void *b)
{
  register struct MSG_ThreadEntry *ma = *(struct MSG_ThreadEntry **) a;
  register struct MSG_ThreadEntry *mb = *(struct MSG_ThreadEntry **) b;
  COMPARE_STRING_FIELDS(mb, ma, recipient);
}

#undef COMPARE_INT

/* This sorts a single chain of ThreadEntries connected by the `next' slot.
   It doesn't look at the children.
 */
static struct MSG_ThreadEntry *
msg_sort_threads_2 (struct msg_thread_state *state,
					struct MSG_ThreadEntry *list)
{
  struct MSG_ThreadEntry *rest;
  struct MSG_ThreadEntry **array, **out;
  uint32 L = 0;
  uint32 i;
  MWContext* context = state->context;
  int (*sort_fn) (const void *a, const void *b);

  if (! list)
	return 0;

  switch (state->sort_key)
	{
	case MSG_SortByDate:
	  sort_fn = (state->sort_forward_p
				 ? msg_compare_date_forward
				 : msg_compare_date_backward);
	  break;
	case MSG_SortBySubject:
	  sort_fn = (state->sort_forward_p
				 ? msg_compare_subject_forward
				 : msg_compare_subject_backward);
	  break;
	case MSG_SortBySender:
	  if (MSG_DisplayingRecipients(context)) {
		sort_fn = (state->sort_forward_p
				   ? msg_compare_recipient_forward
				   : msg_compare_recipient_backward);
	  } else {
		sort_fn = (state->sort_forward_p
				   ? msg_compare_sender_forward
				   : msg_compare_sender_backward);
	  }	  
	  break;
	case MSG_SortByMessageNumber:
	  sort_fn = (state->news_p
				 ? (state->sort_forward_p
					? msg_compare_news_msg_number_forward
					: msg_compare_news_msg_number_backward)
				 : (state->sort_forward_p
					? msg_compare_mail_msg_number_forward
					: msg_compare_mail_msg_number_backward));
	  break;
	default:
	  XP_ASSERT (0);
	  return list;
	  break;
	}

  for (rest = list; rest; rest = rest->next)
	L++;
  array = (struct MSG_ThreadEntry **)
	XP_ALLOC ((L + 1) * sizeof (struct MSG_ThreadEntry *));
  if (! array)
	return 0;
  out = array;
  for (rest = list; rest; rest = rest->next)
	*out++ = rest;
  *out = 0;

  msg_qsort_string_table_kludge = state->string_table;
  qsort ((char *) array, L, sizeof (struct MSG_ThreadEntry *), sort_fn);
  msg_qsort_string_table_kludge = 0;
  for (out = array, i = 0; i < L; out++, i++)
	out[0]->next = out[1];
  list = array[0];
  XP_FREE (array);
  return (list);
}


/* This sorts a chain of ThreadEntries connected by the `next' slot,
   and then recurses, and sorts the lists in each of their `first_child' slots.
 */
static struct MSG_ThreadEntry *
msg_sort_threads_1 (struct msg_thread_state *state,
					struct MSG_ThreadEntry *list)
{
  struct MSG_ThreadEntry *parent = 0;
  struct MSG_ThreadEntry *value = 0;
  struct MSG_ThreadEntry *rest;
 RECURSE:
  XP_ASSERT (list);
  list = msg_sort_threads_2 (state, list);
  XP_ASSERT (list);
  if (parent)
	parent->first_child = list;
  if (! value)
	value = list;

  for (rest = list; rest; rest = rest->next)
	{
	  XP_ASSERT (! (rest->flags & MSG_FLAG_EXPIRED)); /* only at top-level! */
	  if (!rest->first_child)
		;
	  else if (!rest->next)
		/* If this is the last item in the list, we don't have to recurse,
		   we can do a tail-call.  I can't believe I'm using compilers so
		   crummy that I have do this by hand. */
		{
		  parent = rest;
		  list = rest->first_child;
		  goto RECURSE;
		}
	  else
		{
		  /* #### toy-computer-danger, real recursion. */
		  rest->first_child = msg_sort_threads_1 (state, rest->first_child);
		}
	}
  return value;
}


/* Given a list of ThreadEntry structures, this *first* sorts the children of
   those thread entries, then makes some modifications to the `expired' entries
   in this list, then sorts this list.

   The difference between this and msg_sort_threads_1() is that this sorts the
   children first (which is necessary to get correct ordering of the expired
   threads - we need to know what the "first child is") but the implementation
   of msg_sort_threads_1() is better for non-root lists because it is able to
   eliminate recursion (this function is not tail recursive, but that one is.)
 */
static struct MSG_ThreadEntry *
msg_sort_threads (struct msg_thread_state *state,
				  struct MSG_ThreadEntry *list)
{
  struct MSG_ThreadEntry *rest;

  if (! list) return 0;

  /* Sort the children of the elements in this list.
   */
  for (rest = list; rest; rest = rest->next)
	if (rest->first_child)
	  rest->first_child = msg_sort_threads_1 (state, rest->first_child);

  /* For each element in this list which is `expired', fill the sortable
	 slots of that element with the elements of its first child.
   */
  for (rest = list; rest; rest = rest->next)
	if (rest->flags & MSG_FLAG_EXPIRED)
	  {
		XP_ASSERT (rest->first_child);
		if (rest->first_child)
		  {
			rest->sender    = rest->first_child->sender;
			rest->recipient = rest->first_child->recipient;
			rest->subject   = rest->first_child->subject;
			rest->lines     = rest->first_child->lines;
			rest->date      = rest->first_child->date;
			if (state->news_p)
			  {
				rest->data.news.article_number =
				  rest->first_child->data.news.article_number;
			  }
			else
			  {
				rest->data.mail.file_index =
				  rest->first_child->data.mail.file_index;
				rest->data.mail.byte_length =
				  rest->first_child->data.mail.byte_length;
			  }
		  }
	  }

  /* Now that the `expired' meta-parents are sortable, sort this list itself.
   */
  list = msg_sort_threads_2 (state, list);
  XP_ASSERT (list);
  return list;
}



/* Hash table comparison function for string keys. */
static int
msg_strcmp (const void *obj1, const void *obj2)
{
  XP_ASSERT (obj1 && obj2);
  return XP_STRCMP ((char *) obj1, (char *) obj2);
}

/* Hash table comparison function for integer keys. */
static int
msg_intcmp (const void *obj1, const void *obj2)
{
  return (int) (((long) obj2) - ((long) obj1));
}

/* No-op hash function for integer keys. */
static uint32
msg_inthash (const void *x)
{
  return (int) x;
}


/* Creates and initializes a state object to hold the thread data.
   The first argument should be the caller's best guess as to how
   many messages there will be; it is used for the initial size of
   the hash tables, so it's ok for it to be too small.  The other
   arguments specify the sorting behavior.
 */
struct msg_thread_state *
msg_BeginThreading (MWContext* context,
					uint32 message_count_guess,
					XP_Bool news_p,
					MSG_SORT_KEY sort_key, XP_Bool sort_forward_p,
					XP_Bool thread_p,
					msg_DummyThreadEntryCreator dummy_creator,
					void *dummy_creator_arg)
{
  struct msg_thread_state *state = XP_NEW (struct msg_thread_state);
  if (! state)
	return 0;
  XP_MEMSET (state, 0, sizeof(*state));
  state->context = context;
  state->news_p = news_p;
  state->sort_key = sort_key;
  state->sort_forward_p = sort_forward_p;
  state->thread_p = thread_p;
  state->dummy_creator = dummy_creator;
  state->dummy_creator_arg = dummy_creator_arg;

  XP_InitAllocStructInfo (&state->container_blocks,
						  sizeof (struct msg_container));

  if (state->thread_p)
	{
	  state->id_table = XP_HashTableNew (message_count_guess * 2,
										 XP_StringHash, msg_strcmp);
	  if (! state->id_table)
		{
		  XP_FREE (state);
		  return 0;
		}
	  state->subject_table = XP_HashTableNew (message_count_guess * 3 / 2,
											  msg_inthash, msg_intcmp);
	  if (! state->id_table)
		{
		  XP_HashTableDestroy (state->id_table);
		  XP_FREE (state);
		  return 0;
		}
	}

  return state;
}


/* Threads the messages, discards the state object, and returns a
   tree of MSG_ThreadEntry objects.
 */
struct MSG_ThreadEntry *
msg_DoneThreading (struct msg_thread_state *state,
				   char **string_table)
{
  struct MSG_ThreadEntry *result = 0;
  int status = 0;
  struct msg_thread_state_extrastrings* tmp;

  state->string_table = string_table;

  if (state->thread_p)
	{
	  status = msg_compute_root_set (state);
	  if (status < 0) goto DONE;

	  status = msg_clean_threads (state);
	  if (status < 0) goto DONE;

	  status = msg_thread_by_subject (state);
	  if (status < 0) goto DONE;
	}

  status = msg_relink_threads (state);
  if (status < 0) goto DONE;

  if (state->root_messages) {
	state->root_messages = msg_sort_threads (state, state->root_messages);
  }

 DONE:
  XP_FreeAllStructs (&state->container_blocks);
  state->root_containers = 0;

  if (state->id_table) XP_HashTableDestroy (state->id_table);
  if (state->subject_table) XP_HashTableDestroy (state->subject_table);

  while ((tmp = state->extrastrings)) {
	state->extrastrings = tmp->next;
	XP_FREE(tmp->str);
	XP_FREE(tmp);
  }

  result = state->root_messages;
  XP_FREE (state);
  return result;
}


static int
msg_rethread_1 (struct msg_thread_state *state,
				char **string_table,
				struct MSG_ThreadEntry *list,
				msg_DummyThreadEntryDestroyer dummy_destroyer)
{
  struct MSG_ThreadEntry *next, *rest, *kid;
  int status;
 RECURSE:
  XP_ASSERT (list);
  if (! list) return 0;

  for (rest = list, kid = rest->first_child, next = rest->next;
	   rest;
	   rest = next,
		 kid  = rest ? rest->first_child : 0,
		 next = rest ? rest->next : 0)
	{
	  rest->first_child = 0;
	  rest->next = 0;

	  if (rest->flags & MSG_FLAG_EXPIRED)
		{
		  if (dummy_destroyer)
			(*dummy_destroyer) (rest, state->dummy_creator_arg);
		  else
			XP_FREE (rest);
		}
	  else
		{
		  status = msg_ThreadMessage (state, string_table, rest);
		  if (status < 0) return status;
		}

	  if (!kid)
		;
	  else if (!next)
		/* If this is the last item in the list, we don't have to recurse,
		   we can do a tail-call.  I can't believe I'm using compilers so
		   crummy that I have do this by hand. */
		{
		  list = kid;
		  goto RECURSE;
		}
	  else
		{
		  /* #### toy-computer-danger, real recursion. */
		  status = msg_rethread_1 (state, string_table, kid, dummy_destroyer);
		  if (status < 0) return status;
		}
	}
  return 0;
}


/* Given an existing tree of MSG_ThreadEntry structures, re-sorts them.
   This changes the `next' and `first_child' links, and may create or
   destroy "dummy" thread entries (those with the EXPIRED flag set.)
   The new root of the tree is returned.
 */
struct MSG_ThreadEntry *
msg_RethreadMessages (MWContext* context,
					  struct MSG_ThreadEntry *tree,
					  uint32 approximate_message_count,
					  char **string_table,
					  XP_Bool news_p,
					  MSG_SORT_KEY sort_key, XP_Bool sort_forward_p,
					  XP_Bool thread_p,
					  msg_DummyThreadEntryCreator dummy_creator,
					  msg_DummyThreadEntryDestroyer dummy_destroyer,
					  void *dummy_arg)
{
  struct msg_thread_state *state;
  int status = 0;

  XP_ASSERT (tree);
  if (! tree) return 0;

  state = msg_BeginThreading (context, approximate_message_count,
							  news_p, sort_key, sort_forward_p, thread_p,
							  dummy_creator, dummy_arg);
  if (!state) return 0;

  status = msg_rethread_1 (state, string_table, tree, dummy_destroyer);
  if (status < 0)
	/* #### bad - we've trashed the list! */
	/* #### leaks the `state' */
	return 0;

  return msg_DoneThreading (state, string_table);
}
