/* -*- Mode: C; tab-width: 4 -*-
 newsrc.c --- reading and writing newsrc files.
 Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 Created: Jamie Zawinski <jwz@netscape.com>, 10-May-95.
 */

#include "msg.h"
#include "newsrc.h"
#include "xp_hash.h"
#include "undo.h"


extern int MK_MIME_ERROR_WRITING_FILE;
extern int MK_OUT_OF_MEMORY;


/* A compressed encoding for lines from the newsrc.
   These lines look like

   news.answers: 1-29627,29635,29658,32861-32863

   so the data has these properties:

   - strictly increasing
   - large subsequences of monotonically increasing ranges
   - gaps in the set are usually small, but not always
   - consecutive ranges tend to be large

   The biggest win is to run-length encode the data, storing ranges as two
   numbers (start+length or start,end). We could also store each number as a
   delta from the previous number for further compression, but that gets kind
   of tricky, since there are no guarentees about the sizes of the gaps, and
   we'd have to store variable-length words.

   Current data format:

   DATA := SIZE [ CHUNK ]*
   CHUNK := [ RANGE | VALUE ]
   RANGE := -LENGTH START
   START := VALUE
   LENGTH := int32
   VALUE := a literal positive integer, for now
   it could also be an offset from the previous value.
   LENGTH could also perhaps be a less-than-32-bit quantity,
   at least most of the time.

   Lengths of CHUNKs are stored negative to distinguish the beginning of
   a chunk from a literal: negative means two-word sequence, positive
   means one-word sequence.

   0 represents a literal 0, but should not occur, and should never occur
   except in the first position.

   A length of -1 won't occur either, except temporarily - a sequence of
   two elements is represented as two literals, since they take up the same
   space.

   Another optimization we make is to notice that we typically ask the
   question ``is N a member of the set'' for increasing values of N. So the
   set holds a cache of the last value asked for, and can simply resume the
   search from there.  */

struct msg_NewsRCSet {
  char *name;					/* name of newsgroup.  If NULL, then this
								   isn't really a set for a group; it's just
								   an internal set of article numbers, and
								   so shouldn't be involved in Undo and
								   stuff. */
  XP_Bool is_subscribed;		/* are we subscribed to the group? */
  int32 *data;					/* the numbers composing the `chunks' */
  uint32 data_size;				/* size of that malloc'ed block */
  uint32 length;				/* active area */

  int32 cached_value;			/* a potential set member, or -1 if unset */
  int32 cached_value_index;		/* the index into `data' at which a search
								   to determine whether `cached_value' was a
								   member of the set ended. */
};

struct msg_NewsRCFile {
  int group_count;
  struct msg_NewsRCSet **groups;
  XP_AllocStructInfo blocks;
  char *options_lines;

  XP_HashTable groups_table;
};


msg_NewsRCSet *
msg_GetNewsRCSet (MWContext *context, const char *group_name,
				  struct msg_NewsRCFile *file)
{
  msg_NewsRCSet *value;
  if (!file) return 0;
  if (!file->groups_table) return 0;
  value = (msg_NewsRCSet *) XP_Gethash (file->groups_table, group_name, 0);
  XP_ASSERT (!value || (value->name && !XP_STRCMP (group_name, value->name)));
  return value;
}


msg_NewsRCSet *
msg_MakeNewsrcSet(void)
{
  msg_NewsRCSet *set = XP_NEW_ZAP(msg_NewsRCSet);
  if (! set)
	return 0;
  set->cached_value = -1;
  set->data_size = 10;
  set->data = (int32 *) XP_ALLOC (sizeof (int32) * set->data_size);
  if (! set->data)
	{
	  XP_FREE(set);
	  return 0;
	}
  return set;
}

void
msg_FreeNewsrcSet (msg_NewsRCSet *set)
{
  if (!set)
	return;
  XP_ASSERT(set->name == NULL);	/* Just a cross-check -- we never set the name
								   on any set used just for internal
								   bookkeeping, right?  And we never call this
								   routine except for such sets, right? */
  FREEIF (set->name);
  FREEIF (set->data);
  XP_FREE (set);
}


static msg_NewsRCSet *
msg_make_newsrc_set (XP_AllocStructInfo *blocks)
{
  msg_NewsRCSet *set = (msg_NewsRCSet *) XP_AllocStructZero (blocks);
  if (! set)
	return 0;
  set->cached_value = -1;
  set->data_size = 10;
  set->data = (int32 *) XP_ALLOC (sizeof (int32) * set->data_size);
  if (! set->data)
	{
	  XP_FreeStruct (blocks, set);
	  return 0;
	}
  return set;
}

static void
msg_free_newsrc_set (msg_NewsRCSet *set, XP_AllocStructInfo *blocks)
{
  if (!set)
	return;
  FREEIF (set->name);
  FREEIF (set->data);
  XP_FreeStruct (blocks, set);
}


/* When more room is needed in a msg_NewsRCSet, this expands it.
   Returns TRUE if succesful, FALSE if unable to allocate the space.
   */
static XP_Bool
msg_grow_newsrc_set (msg_NewsRCSet *set)
{
  int new_size;
  int32 *new_data;
  XP_ASSERT(set);
  if (! set)
	return FALSE;
  new_size = set->data_size * 2;
  new_data = (int32 *) XP_REALLOC (set->data, sizeof (int32) * new_size);
  if (! new_data)
	return FALSE;
  set->data_size = new_size;
  set->data = new_data;
  return TRUE;
}


/* This takes a sequence of numbers from the newsrc file and converts them
   to a compressed `msg_NewsRCSet' object.
   */
static msg_NewsRCSet *
msg_parse_newsrc_set (const char * numbers, XP_AllocStructInfo *blocks)
{
  msg_NewsRCSet *output = msg_make_newsrc_set (blocks);
  int32 *head, *tail, *end;

  if (! output)
	return 0;

  head = output->data;
  tail = head;
  end = head + output->data_size;

  if(!numbers)
	{
	  output->length = 0;
	  return(output);
	}

  while (isspace (*numbers)) numbers++;
  while (*numbers)
	{
	  int32 from = 0;
	  int32 to;

	  if (tail >= end - 4) /* out of room! */
		{
		  int32 tailo = tail - head;
		  if (! msg_grow_newsrc_set (output))
			{
			  msg_free_newsrc_set (output, blocks);
			  return 0;
			}
		  /* data may have been relocated */
		  head = output->data;
		  tail = head + tailo;
		  end = head + output->data_size;
		}

	  while (isspace (*numbers)) numbers++;
	  if (*numbers && !isdigit (*numbers)) /* illegal character */
		break;
	  while (isdigit (*numbers))
		from = (from * 10) + (*numbers++ - '0');
	  while (isspace (*numbers)) numbers++;
	  if (*numbers != '-')
		{
		  to = from;
		}
	  else
		{
		  to = 0;
		  numbers++;
		  while (*numbers >= '0' && *numbers <= '9')
			to = (to * 10) + (*numbers++ - '0');
		  while (isspace (*numbers)) numbers++;
		}

	  if (to < from) to = from; /* illegal */

	  /* This is a total kludge - if the newsrc file specifies a range 1-x as
		 being read, we internally pretend that article 0 is read as well.
		 (But if only 2-x are read, then 0 is not read.)  This is needed
		 because some servers think that article 0 is an article (I think) but
		 some news readers (including Netscape 1.1) choke if the .newsrc file
		 has lines beginning with 0... */
	  if (from == 1) from = 0;

	  if (to == from) /* Write it as a literal */
		{
		  *tail = from;
		  tail++;
		}
	  else /* Write it as a range. */
		{
		  *tail = -(to - from);
		  tail++;
		  *tail = from;
		  tail++;
		}

	  while (*numbers == ',' || isspace (*numbers))
		numbers++;
	}

  output->length = tail - head; /* size of data */
  return output;
}


void
msg_FreeNewsRCFile (struct msg_NewsRCFile *file)
{
  int i;
  if (! file) return;
  FREEIF (file->options_lines);
  if (file->groups_table)
	XP_HashTableDestroy (file->groups_table);
  for (i = 0; i < file->group_count; i++)
	msg_free_newsrc_set (file->groups[i], &file->blocks);
  FREEIF (file->groups);
  XP_FREE (file);
}


int
msg_CopySet(msg_NewsRCSet* dest, msg_NewsRCSet* source)
{
  XP_ASSERT(dest && source && (dest->name == NULL || source->name == NULL));
  if (!dest || !source || (dest->name && source->name)) return -1;
  if (!dest->data || dest->data_size < source->length) {
	dest->data_size = source->length;
	if (dest->data_size < 10) dest->data_size = 10;
	FREEIF(dest->data);
	dest->data = (int32*) XP_ALLOC(dest->data_size * sizeof(int32));
	if (!dest->data) return MK_OUT_OF_MEMORY;
  }
  dest->length = source->length;
  XP_MEMCPY(dest->data, source->data, source->length * sizeof(int32));
  dest->cached_value = -1;
  dest->is_subscribed = source->is_subscribed;
  return 0;
}



msg_NewsRCSet *
msg_AddNewsRCSet (const char *group_name, struct msg_NewsRCFile *file)
{
  struct msg_NewsRCSet *set;
  struct msg_NewsRCSet **sets;
  if (!file) return 0;
  if (!file->groups_table) return 0;

  /* Make sure this group isn't in the file already. */
  set = (msg_NewsRCSet *) XP_Gethash (file->groups_table, group_name, 0);
  XP_ASSERT (!set);
  if (set) return set;

  /* Expand the list of groups, and make a set object. */
  sets = (struct msg_NewsRCSet **)
	XP_REALLOC (file->groups, (file->group_count + 2) * sizeof (*sets));
  if (!sets) return 0;
  file->groups = sets;
  set = msg_make_newsrc_set (&file->blocks);
  if (!set) return 0;
  set->is_subscribed = FALSE;
  set->name = XP_STRDUP (group_name);
  if (!set->name)
	{
	  XP_FREE (set);
	  return 0;
	}

  /* Store the new set into the table */
  if (XP_Puthash (file->groups_table, (void *) set->name, (void *) set) < 0)
	{
	  XP_FREE (set->name);
	  XP_FREE (set);
	  return 0;
	}

  /* Finally store it into the array. */
  file->groups[file->group_count++] = set;
  file->groups[file->group_count] = 0;
  return set;
}


XP_Bool
msg_GetSubscribed(struct msg_NewsRCSet* set)
{
  return set->is_subscribed;
}

void
msg_SetSubscribed(struct msg_NewsRCSet* set, XP_Bool value)
{
  set->is_subscribed = value;
}


#ifdef XP_WIN16
#pragma optimize("", off)
#endif /* XP_WIN16 */

/* Returns the lowest non-member of the set greater than 0.
 */
int32
msg_FirstUnreadArt (msg_NewsRCSet *set)
{
  XP_ASSERT(set);
  if (! set)
	return 1;
  else if (set->length <= 0)
	return 1;
  else if(set->data [0] < 0 && set->data [1] != 1 && set->data [1] != 0)
	/* first range not equal to 0 or 1, always return 1 */
	return 1;
  else if (set->data [0] < 0) /* it's a range */
	/* If there is a range [N-M] we can presume that M+1 is not in the set. */
	return (set->data [1] - set->data [0] + 1);
  else /* it's a literal */
	/* If there is a literal N,
	 * then it is either 1 or greater than 1. In either case
	 * we want to return 1
	 */
	return 1;
}

#ifdef XP_WIN16
#pragma optimize("", on)
#endif /* XP_WIN16 */

/* This takes a `msg_NewsRCSet' object and converts it to a string suitable
   for printing in the newsrc file.

   The assumption is made that the `msg_NewsRCSet' is already compressed to
   an optimal sequence of ranges; no attempt is made to compress further.
   */
static char *
msg_format_newsrc_set (msg_NewsRCSet *set)
{
  int32 size;
  int32 *head;
  int32 *tail;
  int32 *end;
  int32 s_size;
  char *s_head;
  char *s, *s_end;
  int name_len;
  int32 last_art = -1;

  XP_ASSERT(set);
  if (! set) return 0;

  size = set->length;
  head = set->data;
  tail = head;
  end = head + size;

  name_len = (set->name ? XP_STRLEN (set->name) : 0);
  s_size = name_len + (size * 8) + 10;
  s_head = (char *) XP_ALLOC (s_size);
  s = s_head;
  s_end = s + s_size;

  if (! s) return 0;

  if (name_len)
	{
	  XP_MEMCPY (s_head, set->name, name_len);
	  s = s_head + name_len;
	  *s++ = (set->is_subscribed ? ':' : '!');
	  if (size != 0)
		*s++ = ' ';
	  *s = 0;
	}

  while (tail < end)
	{
	  int32 from;
	  int32 to;

	  if (s > (s_end - (12 * 2 + 10))) /* 12 bytes for each number (enough
										  for "2147483647" aka 2^31-1), plus
										  10 bytes of slop/paranoia. */
		{
		  int32 so = s - s_head;
		  s_size += 200;
		  s_head = (char *) XP_REALLOC (s_head, s_size);
		  s = s_head + so;
		  if (! s_head)
			return 0;
		  s_end = s_head + s_size;
		}

	  if (*tail < 0) /* it's a range */
		{
		  from = tail[1];
		  to = from + (-(tail[0]));
		  tail += 2;
		}
	  else /* it's a literal */
		{
		  from = *tail;
		  to = from;
		  tail++;
		}
	  if (from == 0) {
		from = 1;				/* See 'kludge' comment above */
	  }
	  if (from <= last_art) from = last_art + 1;
	  if (from <= to) {
		if (from < to) {
		  PR_snprintf(s, s_end - s, "%lu-%lu,", from, to);
		} else {
		  PR_snprintf(s, s_end - s, "%lu,", from);
		}
		s += XP_STRLEN(s);
		last_art = to;
	  }
	}
  if (last_art >= 0) {
	s--;							/* Strip off the last ',' */
  }

#if defined(XP_WIN)
  *s++ = CR;
  *s++ = LF;
#elif defined(XP_MAC)
  *s++ = CR;
#else
  *s++ = LF;
#endif
  *s = 0;

  return s_head;
}


/* Re-compresses a `msg_NewsRCSet' object.

   The assumption is made that the `msg_NewsRCSet' is syntactically correct
   (all ranges have a length of at least 1, and all values are non-
   decreasing) but will optimize the compression, for example, merging
   consecutive literals or ranges into one range.

   Returns TRUE if successful, FALSE if there wasn't enough memory to
   allocate scratch space.

   #### This should be changed to modify the buffer in place.
   */
XP_Bool
msg_OptimizeNewsRCSet (msg_NewsRCSet *set)
{
  int32 input_size;
  int32 output_size;
  int32 *input_tail;
  int32 *output_data;
  int32 *output_tail;
  int32 *input_end;
  int32 *output_end;

  XP_ASSERT(set);
  if (! set)
	return FALSE;

  input_size = set->length;
  output_size = input_size + 1;
  input_tail = set->data;
  output_data = (int32 *) XP_ALLOC (sizeof (int32) * output_size);
  output_tail = output_data;
  input_end = input_tail + input_size;
  output_end = output_data + output_size;

  if (! output_data)
	return FALSE;

  /* We're going to modify the set, so invalidate the cache. */
  set->cached_value = -1;

  while (input_tail < input_end)
	{
	  int32 from, to;
	  XP_Bool range_p = (*input_tail < 0);

	  if (range_p) /* it's a range */
		{
		  from = input_tail[1];
		  to = from + (-(input_tail[0]));

		  /* Copy it over */
		  *output_tail++ = *input_tail++;
		  *output_tail++ = *input_tail++;

		  if (output_tail >= output_end)
			abort ();
		}
	  else /* it's a literal */
		{
		  from = *input_tail;
		  to = from;

		  /* Copy it over */
		  *output_tail++ = *input_tail++;
		  if (output_tail >= output_end)
			abort ();
		}

	  /* As long as this chunk is followed by consecutive chunks,
		 keep extending it. */
	  while (input_tail < input_end &&
			 ((*input_tail > 0 && /* literal... */
			   *input_tail == to + 1) || /* ...and consecutive, or */
			  (*input_tail <= 0 && /* range... */
			   input_tail[1] == to + 1))) /* ...and consecutive. */
		{
		  if (! range_p)
			{
			  /* convert the literal to a range. */
			  output_tail++;
			  output_tail [-2] = 0;
			  output_tail [-1] = from;
			  range_p = TRUE;
			}

		  if (*input_tail > 0) /* literal */
			{
			  output_tail[-2]--; /* increase length by 1 */
			  to++;
			  input_tail++;
			}
		  else
			{
			  int32 L2 = (- *input_tail) + 1;
			  output_tail[-2] -= L2; /* increase length by N */
			  to += L2;
			  input_tail += 2;
			}
		}
	}

  XP_FREE (set->data);
  set->data = output_data;
  set->data_size = output_size;
  set->length = output_tail - output_data;

  /* One last pass to turn [N - N+1] into [N, N+1]. */
  output_tail = output_data;
  output_end = output_tail + set->length;
  while (output_tail < output_end)
	{
	  if (*output_tail < 0) /* it's a range */
		{
		  if (output_tail[0] == -1)
			{
			  output_tail[0] = output_tail[1];
			  output_tail[1]++;
			}
		  output_tail += 2;
		}
	  else /* it's a literal */
		{
		  output_tail++;
		}
	}

  return TRUE;
}


/* Whether a number is a member of a set. */
XP_Bool
msg_NewsRCIsRead (msg_NewsRCSet *set, int32 number)
{
  XP_Bool value = FALSE;
  int32 size;
  int32 *head;
  int32 *tail;
  int32 *end;

  XP_ASSERT(set);
  if (! set)
	return FALSE;

  size = set->length;
  head = set->data;
  tail = head;
  end = head + size;

  /* If there is a value cached, and that value is smaller than the
	 value we're looking for, skip forward that far. */
  if (set->cached_value > 0 &&
	  set->cached_value < number)
	tail += set->cached_value_index;

  while (tail < end)
	{
	  if (*tail < 0) /* it's a range */
		{
		  int32 from = tail[1];
		  int32 to = from + (-(tail[0]));
		  if (from > number)
			{
			  /* This range begins after the number - we've passed it. */
			  value = FALSE;
			  goto DONE;
			}
		  else if (to >= number)
			{
			  /* In range. */
			  value = TRUE;
			  goto DONE;
			}
		  else
			{
			  tail += 2;
			}
		}
	  else /* it's a literal */
		{
		  if (*tail == number)
			{
			  /* bang */
			  value = TRUE;
			  goto DONE;
			}
		  else if (*tail > number)
			{
			  /* This literal is after the number - we've passed it. */
			  value = FALSE;
			  goto DONE;
			}
		  else
			{
			  tail++;
			}
		}
	}

DONE:
  /* Store the position of this chunk for next time. */
  set->cached_value = number;
  set->cached_value_index = tail - head;

  return value;
}


/* Adds a number to a set.
   Returns negative if out of memory.
   Returns 1 if a change was made, 0 if it was already read.
   */
int
msg_MarkArtAsRead (msg_NewsRCSet *set, int32 number)
{
  int32 size;
  int32 *head;
  int32 *tail;
  int32 *end;

  XP_ASSERT(set);
  if (! set)
	return -1;

  size = set->length;
  head = set->data;
  tail = head;
  end = head + size;

  XP_ASSERT (number >= 0);
  if (number < 0)
	return 0;

  /* We're going to modify the set, so invalidate the cache. */
  set->cached_value = -1;

  while (tail < end)
	{
	  if (*tail < 0) /* it's a range */
		{
		  int32 from = tail[1];
		  int32 to = from + (-(tail[0]));

		  if (from <= number && to >= number)
			{
			  /* This number is already present - we don't need to do
				 anything. */
			  return 0;
			}

		  if (to > number)
			/* We have found the point before which the new number
			   should be inserted. */
			break;

		  tail += 2;
		}
	  else /* it's a literal */
		{
		  if (*tail == number)
			{
			  /* This number is already present - we don't need to do
				 anything. */
			  return 0;
			}

		  if (*tail > number)
			/* We have found the point before which the new number
			   should be inserted. */
			break;

		  tail++;
		}
	}

  /* At this point, `tail' points to a position in the set which represents
	 a value greater than `new'; or it is at `end'. In the interest of
	 avoiding massive duplication of code, simply insert a literal here and
	 then run the optimizer.
	 */
  {
	int32 mid = (tail - head);

	if (set->data_size <= set->length + 1)
	  {
		int endo = end - head;
		if (! msg_grow_newsrc_set (set))
		  return MK_OUT_OF_MEMORY;
		head = set->data;
		end = head + endo;
	  }

	if (tail == end) /* at the end */
	  {
		/* Add a literal to the end. */
		set->data [set->length++] = number;
	  }
	else /* need to insert (or edit) in the middle */
	  {
		int32 i;
		for (i = size; i > mid; i--)
		  set->data [i] = set->data [i-1];
		set->data [i] = number;
		set->length++;
	  }
  }

  msg_OptimizeNewsRCSet (set);
  return 1;
}

/* Removes a number from a set.
   Returns negative if out of memory.
   Returns 1 if a change was made, 0 if it was already unread.
   */
int
msg_MarkArtAsUnread (msg_NewsRCSet *set, int32 number)
{
  int32 size;
  int32 *head;
  int32 *tail;
  int32 *end;

  XP_ASSERT (set);
  if (! set)
	return -1;

  size = set->length;
  head = set->data;
  tail = head;
  end = head + size;

  XP_ASSERT (number >= 0);
  if (number < 0) /* bogus */
	return 0;

  /* We're going to modify the set, so invalidate the cache. */
  set->cached_value = -1;

  while (tail < end)
	{
	  int32 mid = (tail - set->data);

	  if (*tail < 0) /* it's a range */
		{
		  int32 from = tail[1];
		  int32 to = from + (-(tail[0]));

		  if (number < from || number > to)
			{
			  /* Not this range */
			  tail += 2;
			  continue;
			}

		  if (to == from + 1)
			{
			  /* If this is a range [N - N+1] and we are removing M
				 (which must be either N or N+1) replace it with the
				 literal M. This reduces the length by 1. */
			  set->data [mid] = number;
			  while (++mid < set->length)
				set->data [mid] = set->data [mid+1];
			  set->length--;
			  msg_OptimizeNewsRCSet (set);
			  return 1;
			}
		  else if (to == from + 2)
			{
			  /* If this is a range [N - N+2] and we are removing M,
				 replace it with the literals L,M (that is, either
				 (N, N+1), (N, N+2), or (N+1, N+2). The overall
				 length remains the same. */
			  set->data [mid] = from;
			  set->data [mid+1] = to;
			  if (from == number)
				set->data [mid] = from+1;
			  else if (to == number)
				set->data [mid+1] = to-1;
			  msg_OptimizeNewsRCSet (set);
			  return 1;
			}
		  else if (from == number)
			{
			  /* This number is at the beginning of a long range (meaning a
				 range which will still be long enough to remain a range.)
				 Increase start and reduce length of the range. */
			  set->data [mid]++;
			  set->data [mid+1]++;
			  msg_OptimizeNewsRCSet (set);
			  return 1;
			}
		  else if (to == number)
			{
			  /* This number is at the end of a long range (meaning a range
				 which will still be long enough to remain a range.)
				 Just decrease the length of the range. */
			  set->data [mid]++;
			  msg_OptimizeNewsRCSet (set);
			  return 1;
			}
		  else
			{
			  /* The number being deleted is in the middle of a range which
				 must be split. This increases overall length by 2.
				 */
			  int32 i;
			  int32 endo = end - head;
			  if (! msg_grow_newsrc_set (set))
				return MK_OUT_OF_MEMORY;
			  head = set->data;
			  end = head + endo;

			  for (i = set->length + 2; i > mid + 2; i--)
				set->data [i] = set->data [i-2];

			  set->data [mid] = (- (number - from - 1));
			  set->data [mid+1] = from;
			  set->data [mid+2] = (- (to - number - 1));
			  set->data [mid+3] = number + 1;
			  set->length += 2;

			  /* Oops, if we've ended up with a range with a 0 length,
				 which is illegal, convert it to a literal, which reduces
				 the overall length by 1. */
			  if (set->data [mid] == 0) /* first range */
				{
				  set->data [mid] = set->data [mid+1];
				  for (i = mid + 1; i < set->length; i++)
					set->data [i] = set->data [i+1];
				  set->length--;
				}
			  if (set->data [mid+2] == 0) /* second range */
				{
				  set->data [mid+2] = set->data [mid+3];
				  for (i = mid + 3; i < set->length; i++)
					set->data [i] = set->data [i+1];
				  set->length--;
				}
			  msg_OptimizeNewsRCSet (set);
			  return 1;
			}
		}
	  else /* it's a literal */
		{
		  if (*tail != number)
			{
			  /* Not this literal */
			  tail++;
			  continue;
			}

		  /* Excise this literal. */
		  set->length--;
		  while (mid < set->length)
			{
			  set->data [mid] = set->data [mid+1];
			  mid++;
			}
		  msg_OptimizeNewsRCSet (set);
		  return 1;
		}
	}

  /* It wasn't here at all. */
  return 0;
}


static int32*
msg_emit_range(int32* tmp, int32 a, int32 b)
{
  if (a == b) {
	*tmp++ = a;
  } else {
	XP_ASSERT(a < b && a >= 0);
	*tmp++ = -(b - a);
	*tmp++ = a;
  }
  return tmp;
}


/* Adds a range of numbers to a set.
   Returns negative if out of memory.
   Returns 1 if a change was made, 0 if they were all already read.
   */
int
msg_MarkRangeAsRead (msg_NewsRCSet *set, int32 start, int32 end)
{
  uint32 tmplength;
  int32* tmp;
  int32* in;
  int32* out;
  int32* tail;
  int32 a;
  int32 b;
  XP_Bool didit = FALSE;

  XP_ASSERT(set);
  if (! set)
	return -1;

  /* We're going to modify the set, so invalidate the cache. */
  set->cached_value = -1;

  XP_ASSERT(start <= end);
  if (start == end) {
	return msg_MarkArtAsRead(set, start);
  }

  tmplength = set->length + 2;
  tmp = (int32*) XP_ALLOC(sizeof(int32) * tmplength);

  if (!tmp) return MK_OUT_OF_MEMORY;

  in = set->data;
  out = tmp;
  tail = in + set->length;

#define EMIT(x, y) out = msg_emit_range(out, x, y)

  while (in < tail) {
	if (*in < 0) {
	  b = - *in++;
	  a = *in++;
	  b += a;
	} else {
	  a = b = *in++;
	}
	if (a <= start && b >= end) {
	  XP_FREE(tmp);
	  return 0;
	}
	if (start > b + 1) {
	  EMIT(a, b);
	} else if (end < a - 1) {
	  EMIT(start, end);
	  EMIT(a, b);
	  didit = TRUE;
	  break;
	} else {
	  start = start < a ? start : a;
	  end = end > b ? end : b;
	}
  }
  if (!didit) EMIT(start, end);
  while (in < tail) {
	*out++ = *in++;
  }

#undef EMIT

  XP_FREE(set->data);
  set->data = tmp;
  set->length = out - tmp;
  set->data_size = tmplength;
  return 1;
}


/* Remove a range of numbers from a set.
   Returns negative if out of memory.
   Returns 1 if a change was made, 0 if they were all already unread.
   #### Optimize this.
   */
int
msg_MarkRangeAsUnread (msg_NewsRCSet *set, int32 start, int32 end)
{
  int status = 0;
  XP_ASSERT (set);
  if (! set)
	return -1;

  /* We're going to modify the set, so invalidate the cache. */
  set->cached_value = -1;

  XP_ASSERT (start <= end);
  while (start <= end)
	{
	  int s2 = msg_MarkArtAsUnread (set, start++);
	  if (s2 < 0) return s2;
	  if (s2 > 0) status = 1;
	}
  return status;
}


/* Given a range of articles, returns the number of articles in that
   range which are currently not marked as read.
   */
int32
msg_NewsRCUnreadInRange (msg_NewsRCSet *set,
 int32 range_start,
 int32 range_end)
{
  int32 count;
  int32 *head;
  int32 *tail;
  int32 *end;

  XP_ASSERT (set);
  if (! set)
	return (range_end - range_start);
  XP_ASSERT (range_start >= 0);
  XP_ASSERT (range_end >= 0);
  XP_ASSERT (range_end >= range_start);

  head = set->data;
  tail = head;
  end = head + set->length;

  count = range_end - range_start + 1;

  while (tail < end)
	{
	  if (*tail < 0) /* it's a range */
		{
		  int32 from = tail[1];
		  int32 to = from + (-(tail[0]));
		  if (from < range_start)
			from = range_start;
		  if (to > range_end)
			to = range_end;

		  if (to >= from)
			count -= (to - from + 1);

		  tail += 2;
		}
	  else /* it's a literal */
		{
		  if (*tail >= range_start && *tail <= range_end)
			count--;
		  tail++;
		}
	  XP_ASSERT (count >= 0);
	}
  return count;
}


/* Given a set and an index into it, moves forward in that set until RANGE_SIZE
   non-members have been seen, and then returns that number.  The idea here is
   that we want to request 100 articles from the news server (1-100), but if
   the user has already read 50 of them (say, numbers 20-80), we really want to
   request 150 of them (1-150) in order to get 100 *unread* articles.

   #### This could be made a lot more efficient, but it probably doesn't
   matter.  */
int32
msg_FindNewsRCSetNonmemberRange (msg_NewsRCSet *set,
								 int32 start, int32 range_size)
{
  int32 end, count;
  XP_ASSERT (set);
  if (! set)
	return FALSE;
  XP_ASSERT (start >= 0);
  XP_ASSERT (range_size > 0);

  end = start;
  count = 0;
  while (count < range_size)
	{
	  if (msg_NewsRCIsRead (set, end))
		count++;
	  end++;
	}
  return end;
}


int msg_FirstUnreadRange(msg_NewsRCSet* set, int32 min, int32 max,
						 int32* first, int32* last)
{
  int32 size;
  int32 *head;
  int32 *tail;
  int32 *end;
  int32 from = 0;
  int32 to = 0;
  int32 a;
  int32 b;

  *first = *last = 0;

  XP_ASSERT(set);
  if (! set) return -1;

  XP_ASSERT(min <= max && min > 0);
  if (min > max || min <= 0) return -1;

  size = set->length;
  head = set->data;
  tail = head;
  end = head + size;

  while (tail < end) {
	a = to + 1;
	if (*tail < 0) {			/* We got a range. */
	  from = tail[1];
	  to = from + (-(tail[0]));
	  tail += 2;
	} else {
	  from = to = tail[0];
	  tail++;
	}
	b = from - 1;
	/* At this point, [a,b] is the range of unread articles just before
	   the current range of read articles [from,to].  See if this range
	   intersects the [min,max] range we were given. */
	if (a > max) return 0;	/* It's hopeless; there are none. */
	if (a <= b && b >= min) {
	  /* Ah-hah!  We found an intersection. */
	  *first = a > min ? a : min;
	  *last = b < max ? b : max;
	  return 0;
	}
  } 
  /* We found no holes in the newsrc that overlaps the range, nor did
	 we hit something read beyond the end of the range.  So, the great infinite
	 range of unread articles at the end of any newsrc line intersects the
	 range we want, and we just need to return that. */
  a = to + 1;
  *first = a > min ? a : min;
  *last = max;
  return 0;
}


int msg_LastUnreadRange(msg_NewsRCSet* set, int32 min, int32 max,
						int32* first, int32* last)
{
  int32 size;
  int32 *head;
  int32 *tail;
  int32 *end;
  int32 from = 0;
  int32 to = 0;
  int32 a;
  int32 b;

  XP_ASSERT(first && last);
  if (!first || !last) return -1;

  *first = *last = 0;

  XP_ASSERT(set);
  if (! set) return -1;

  XP_ASSERT(min <= max && min > 0);
  if (min > max || min <= 0) return -1;

  size = set->length;
  head = set->data;
  tail = head;
  end = head + size;

  while (tail < end) {
	a = to + 1;
	if (*tail < 0) {			/* We got a range. */
	  from = tail[1];
	  to = from + (-(tail[0]));
	  tail += 2;
	} else {
	  from = to = tail[0];
	  tail++;
	}
	b = from - 1;
	/* At this point, [a,b] is the range of unread articles just before
	   the current range of read articles [from,to].  See if this range
	   intersects the [min,max] range we were given. */
	if (a > max) return 0;	/* We're done.  If we found something, it's already
							   sitting in [*first,*last]. */
	if (a <= b && b >= min) {
	  /* Ah-hah!  We found an intersection. */
	  *first = a > min ? a : min;
	  *last = b < max ? b : max;
	  /* Continue on, looking for a later range. */
	}
  }
  if (to < max) {
	/* The great infinite range of unread articles at the end of any newsrc
	   line intersects the range we want, and we just need to return that. */
	a = to + 1;
	*first = a > min ? a : min;
	*last = max;
  }
  return 0;
}


typedef struct msg_preserveset_info {
  MWContext* context;
  msg_NewsRCSet* set;
  int32* data;
  uint32 length;
} msg_preserveset_info;


static int
msg_preserveset_undoit(void* closure)
{
  msg_preserveset_info* info = (msg_preserveset_info*) closure;
  MSG_Folder* folder;
  if (info->set->data_size < info->length) {
	int32* newdata = (int32*) XP_ALLOC(info->length * sizeof(int32));
	if (!newdata) return MK_OUT_OF_MEMORY;
	if (info->set->data) XP_FREE(info->set->data);
	info->set->data = newdata;
	info->set->data_size = info->length;
  }
  /* We're going to modify the set, so invalidate the cache. */
  info->set->cached_value = -1;
  XP_MEMCPY(info->set->data, info->data, info->length * sizeof(int32));
  info->set->length = info->length;
  folder = msg_FindNewsgroupFolder(info->context, info->set->name);
  if (folder) {
	uint32 count =
	  msg_NewsRCUnreadInRange(info->set, 1,
							  folder->data.newsgroup.last_message);
	msg_UpdateNewsFolderMessageCount(info->context, folder, count, FALSE);
  }
  return 0;
}


static void
msg_preserveset_freeit(void* closure)
{
  msg_preserveset_info* info = (msg_preserveset_info*) closure;
  FREEIF(info->data);
  XP_FREE(info);
}


extern int
msg_PreserveSetInUndoHistory(MWContext* context, msg_NewsRCSet* set)
{
  UndoState* undo;
  msg_preserveset_info* info;
  if (!context || !context->msgframe || !context->msgframe->undo || !set) {
	return -1;
  }
  XP_ASSERT(set->name != NULL);
  if (set->name == NULL) return -1;
  undo = context->msgframe->undo;
  info = XP_NEW_ZAP(msg_preserveset_info);
  if (!info) return MK_OUT_OF_MEMORY;
  info->context = context;
  info->set = set;
  if (set->data && set->length > 0) {
	info->data = (int32*) XP_ALLOC(set->length * sizeof(int32));
	if (!info->data) {
	  XP_FREE(info);
	  return MK_OUT_OF_MEMORY;
	}
	XP_MEMCPY(info->data, set->data, set->length * sizeof(int32));
	info->length = set->length;
  }
  return UNDO_LogEvent(undo, msg_preserveset_undoit, msg_preserveset_freeit,
					   info);
}




/* Reading the file
 */

struct msg_newsrc_parse_data
{
  uint32 sets_size;
  struct msg_NewsRCFile *file_data;
};

static int32
msg_process_newsrc_line (char *line, uint32 line_size, void *closure)
{
  struct msg_newsrc_parse_data *data =
	(struct msg_newsrc_parse_data *) closure;

  /* guard against blank line lossage */
  if (line[0] == '#' || line[0] == CR || line[0] == LF)
	return 0;

  line[line_size-1] = 0;
  if ((line[0] == 'o' || line[0] == 'O') &&
	  !strncasecomp (line, "options", 7))
	{
	  char *new_data;

	OPTIONS:
	  if (data->file_data->options_lines)
		{
		  new_data =
			(char *) XP_REALLOC (data->file_data->options_lines,
								 XP_STRLEN (data->file_data->options_lines)
								 + XP_STRLEN (line) + 4);
		  if (! new_data)
			return MK_OUT_OF_MEMORY;
		  XP_STRCAT (new_data, line);
		  XP_STRCAT (new_data, LINEBREAK);
		}
	  else
		{
		  new_data = (char *) XP_ALLOC (XP_STRLEN (line) + 3);
		  if (! new_data)
			return MK_OUT_OF_MEMORY;
		  XP_STRCPY (new_data, line);
		  XP_STRCAT (new_data, LINEBREAK);
		}
	  data->file_data->options_lines = new_data;
	  return 0;
	}
  else
	{
	  char *s;
	  char *end = line + line_size;
	  static msg_NewsRCSet *set;
	  int status = 0;

	  for (s = line; s < end; s++)
		if (*s == ':' || *s == '!')
		  break;

	  if (*s == 0)
		/* What the hell is this?? Well, don't just throw it away... */
		goto OPTIONS;

	  set = msg_parse_newsrc_set (s + 1, &data->file_data->blocks);
	  if (! set)
		return MK_OUT_OF_MEMORY;

	  set->is_subscribed = (*s == ':');
	  set->name = (char *) XP_ALLOC (s - line + 1);
	  if (!set->name)
		{
		  msg_free_newsrc_set (set, &data->file_data->blocks);
		  return MK_OUT_OF_MEMORY;
		}
	  XP_MEMCPY (set->name, line, s-line);
	  set->name [s-line] = 0;

	  if (data->file_data->group_count + 3 >= data->sets_size)
		status = msg_GrowBuffer (data->file_data->group_count + 3,
								 sizeof(*data->file_data->groups),
								 1024, (char **) &data->file_data->groups,
								 (uint32*) &data->sets_size);
	  if (status < 0)
		{
		  msg_free_newsrc_set (set, &data->file_data->blocks);
		  return status;
		}
	  data->file_data->groups [data->file_data->group_count++] = set;

	  if (XP_Puthash (data->file_data->groups_table,
					  (void *) set->name, (void *) set) < 0)
		return MK_OUT_OF_MEMORY;

	  return 0;
	}
}

static int
newsrc_cmp (const void *obj1, const void *obj2)
{
  XP_ASSERT (obj1 && obj2);
  return XP_STRCMP ((char *) obj1, (char *) obj2);
}


struct msg_NewsRCFile *
msg_ReadNewsRCFile (MWContext *context, XP_File file)
{
  struct msg_newsrc_parse_data data;
  int status = 0;
  char *ibuffer = 0;
  uint32 ibuffer_size = 0;
  uint32 ibuffer_fp = 0;
  int size = 1024;

  char *buffer = (char *) XP_ALLOC (size);
  if (! buffer) return 0;

  XP_MEMSET (&data, 0, sizeof(data));
  data.file_data = XP_NEW (struct msg_NewsRCFile);
  if (!data.file_data)
	{
	  XP_FREE (buffer);
	  return 0;
	}
  XP_MEMSET (data.file_data, 0, sizeof(*data.file_data));
  XP_InitAllocStructInfo (&data.file_data->blocks,
						  sizeof (struct msg_NewsRCSet));
  data.file_data->groups_table = XP_HashTableNew (100, XP_StringHash,
												  newsrc_cmp);
  if (!data.file_data->groups_table)
	{
	  XP_FREE (buffer);
	  XP_FREE (data.file_data);
	  return 0;
	}

  do
	{
	  status = XP_FileRead (buffer, size, file);
	  if (status > 0) {
		msg_LineBuffer (buffer, status,
						&ibuffer, &ibuffer_size, &ibuffer_fp,
						FALSE,
						msg_process_newsrc_line, &data);
	  }
	}
  while (status > 0);

  if (status == 0 && ibuffer_fp > 0) {
	/* Flush out the last line. */
	status = msg_process_newsrc_line(ibuffer, ibuffer_fp, &data);
	ibuffer_fp = 0;
  }


  FREEIF (buffer);
  FREEIF (ibuffer);
  if (status < 0)
	{
	  msg_FreeNewsRCFile (data.file_data);
	  return 0;
	}

  /* realloc it smaller */
  if (data.file_data->group_count + 1 < data.sets_size)
	{
	  struct msg_NewsRCSet **ns = (struct msg_NewsRCSet **)
		XP_REALLOC (data.file_data->groups,
					(data.file_data->group_count + 1) * sizeof(*ns));
	  if (ns) data.file_data->groups = ns;
	}
  if (data.file_data->groups) {
	data.file_data->groups [data.file_data->group_count] = 0;
  }
  return data.file_data;
}

int
msg_WriteNewsRCFile (MWContext *context, struct msg_NewsRCFile *data,
					 XP_File file)
{
  int status = 0;
  int i;
  if (data->options_lines)
	{
	  status = XP_FileWrite ((void *) data->options_lines,
							 XP_STRLEN (data->options_lines),
							 file);
	  if (status < XP_STRLEN (data->options_lines)) {
		return MK_MIME_ERROR_WRITING_FILE;
	  }
	}

  for (i = 0; i < data->group_count; i++)
	{
	  struct msg_NewsRCSet *group = data->groups[i];
	  char *line;
	  int length;
	  if (!group)
		continue;
	  line = msg_format_newsrc_set (group);
	  if (!line)
		return MK_OUT_OF_MEMORY;
	  length = XP_STRLEN(line);
	  status = XP_FileWrite ((void *) line, length, file);
	  XP_FREE (line);
	  if (status < length) return MK_MIME_ERROR_WRITING_FILE;
	}
  return 0;
}


struct msg_NewsRCFile *
msg_MakeDefaultNewsRCFileData (MWContext *context, XP_Bool default_host_p)
{
  const char *default_groups[] = {
	"news.announce.newusers", "news.newusers.questions", "news.answers" };
  int i;
  struct msg_NewsRCFile *file_data = XP_NEW (struct msg_NewsRCFile);
  if (!file_data) return 0;
  XP_MEMSET (file_data, 0, sizeof(*file_data));
  XP_InitAllocStructInfo (&file_data->blocks, sizeof (struct msg_NewsRCSet));
  file_data->groups_table = XP_HashTableNew (100, XP_StringHash, newsrc_cmp);
  if (!file_data->groups_table) goto FAIL;
  file_data->group_count = (default_host_p ?
							(sizeof(default_groups) / sizeof(*default_groups))
							: 0);
  file_data->groups = (struct msg_NewsRCSet **)
	XP_ALLOC (sizeof (struct msg_NewsRCSet *) * (file_data->group_count + 1));
  if (!file_data->groups) goto FAIL;
  XP_MEMSET (file_data->groups, 0,
			 sizeof(*file_data->groups) * file_data->group_count);

  for (i = 0; i < file_data->group_count; i++)
	{
	  file_data->groups[i] = msg_make_newsrc_set (&file_data->blocks);
	  if (!file_data->groups[i]) goto FAIL;
	  file_data->groups[i]->is_subscribed = TRUE;
	  file_data->groups[i]->name = XP_STRDUP(default_groups[i]);
	  if (!file_data->groups[i]->name) goto FAIL;
	  if (XP_Puthash (file_data->groups_table,
					  (void *) file_data->groups[i]->name,
					  (void *) file_data->groups[i]) < 0)
		goto FAIL;
	}
  file_data->groups[i] = 0;
  return file_data;

FAIL:
  msg_FreeNewsRCFile (file_data);
  return 0;
}


int
msg_MapNewsRCGroupNames (MWContext *context,
						 msg_SubscribedGroupNameMapper mapper,
						 void *closure)
{
  int i, status;
  struct msg_NewsRCFile *file;

  XP_ASSERT (context->msgframe && context->type == MWContextNews);
  if (!context->msgframe || context->type != MWContextNews)
	return -1;

  file = context->msgframe->data.news.newsrc_file;
  if (! file) return 0;
  for (i = 0; i < file->group_count; i++)
	{
	  if (!file->groups[i]->is_subscribed)
		continue;
	  status = (*mapper) (context, file->groups[i]->name, closure);
	  if (status < 0) return status;
	}
  return 0;
}


#if 0 /* A buttload of test cases for the above */

static XP_AllocStructInfo allocinfo =
{ XP_INITIALIZE_ALLOCSTRUCTINFO(sizeof(struct msg_NewsRCSet)) };

#define countof(x) (sizeof(x) / sizeof(*(x)))

static void
test_decoder (const char *string)
{
  msg_NewsRCSet *set;
  char *new;
  if (! (set = msg_parse_newsrc_set(string, &allocinfo)))
	abort ();
  if (!(new = msg_format_newsrc_set (set)))
	abort ();
  printf ("\t\"%s\"\t--> \"%s\"\n", string, new);
  free (new);
  msg_free_newsrc_set (set, &allocinfo);
}

static void
test_adder (void)
{
  char *string;
  msg_NewsRCSet *set;
  char *s;
  uint32 i;

#define START(STRING) \
  string = STRING;	  \
  if (!(set = msg_parse_newsrc_set(string, &allocinfo))) abort ()

#define FROB(N,PUSHP)											\
  i = N;														\
  if (!(s = msg_format_newsrc_set (set))) abort ();				\
  printf ("%3lu: %-58s %c %3lu =\n", set->length, s,			\
		  (PUSHP ? '+' : '-'), i);								\
		  XP_FREE (s);											\
		  if (PUSHP												\
			  ? msg_MarkArtAsRead (set, i) < 0					\
			  : msg_MarkArtAsUnread (set, i) < 0)				\
			abort ();											\
		  if (! (s = msg_format_newsrc_set (set))) abort ();	\
		  printf ("%3lu: %-58s optimized =\n", set->length, s);	\
		  XP_FREE (s);											\
		  if (! msg_OptimizeNewsRCSet (set)) abort ()

#define END()										\
  if (!(s = msg_format_newsrc_set (set))) abort ();	\
  printf ("%3lu: %s\n\n", set->length, s);			\
  XP_FREE (s);										\
  msg_free_newsrc_set (set, &allocinfo)


  START("0-70,72-99,105,107,110-111,117-200");

  FROB(205, TRUE);
  FROB(206, TRUE);
  FROB(207, TRUE);
  FROB(208, TRUE);
  FROB(208, TRUE);
  FROB(109, TRUE);
  FROB(72, TRUE);

  FROB(205, FALSE);
  FROB(206, FALSE);
  FROB(207, FALSE);
  FROB(208, FALSE);
  FROB(208, FALSE);
  FROB(109, FALSE);
  FROB(72, FALSE);

  FROB(72, TRUE);
  FROB(109, TRUE);
  FROB(208, TRUE);
  FROB(208, TRUE);
  FROB(207, TRUE);
  FROB(206, TRUE);
  FROB(205, TRUE);

  FROB(205, FALSE);
  FROB(206, FALSE);
  FROB(207, FALSE);
  FROB(208, FALSE);
  FROB(208, FALSE);
  FROB(109, FALSE);
  FROB(72, FALSE);

  FROB(100, TRUE);
  FROB(101, TRUE);
  FROB(102, TRUE);
  FROB(103, TRUE);
  FROB(106, TRUE);
  FROB(104, TRUE);
  FROB(109, TRUE);
  FROB(108, TRUE);
  END();

  START("1-6"); FROB(7, FALSE); END();
  START("1-6"); FROB(6, FALSE); END();
  START("1-6"); FROB(5, FALSE); END();
  START("1-6"); FROB(4, FALSE); END();
  START("1-6"); FROB(3, FALSE); END();
  START("1-6"); FROB(2, FALSE); END();
  START("1-6"); FROB(1, FALSE); END();
  START("1-6"); FROB(0, FALSE); END();

  START("1-3"); FROB(1, FALSE); END();
  START("1-3"); FROB(2, FALSE); END();
  START("1-3"); FROB(3, FALSE); END();

  START("1,3,5-7,9,10"); FROB(5, FALSE); END();
  START("1,3,5-7,9,10"); FROB(6, FALSE); END();
  START("1,3,5-7,9,10"); FROB(7, FALSE); FROB(7, TRUE); FROB(8, TRUE);
  FROB (4, TRUE); FROB (2, FALSE); FROB (2, TRUE);

  FROB (4, FALSE); FROB (5, FALSE); FROB (6, FALSE); FROB (7, FALSE);
  FROB (8, FALSE); FROB (9, FALSE); FROB (10, FALSE); FROB (3, FALSE);
  FROB (2, FALSE); FROB (1, FALSE); FROB (1, FALSE); FROB (0, FALSE);
  END();
#undef START
#undef FROB
#undef END
}


static void
test_ranges(void)
{
  char *string;
  msg_NewsRCSet *set;
  char *s;
  uint32 i;
  uint32 j;

#define START(STRING) \
  string = STRING;	  \
  if (!(set = msg_parse_newsrc_set(string, &allocinfo))) abort ()

#define FROB(N,M)												\
  i = N;														\
  j = M;														\
  if (!(s = msg_format_newsrc_set (set))) abort ();				\
  printf ("%3lu: %-58s + %3lu-%3lu =\n", set->length, s, i, j);	\
  XP_FREE (s);													\
  switch (msg_MarkRangeAsRead(set, i, j)) {						\
  case 0:														\
	printf("(no-op)\n");										\
	break;														\
  case 1:														\
	break;														\
  default:														\
	abort();													\
  }																\
  if (! (s = msg_format_newsrc_set (set))) abort ();			\
  printf ("%3lu: %-58s\n", set->length, s);						\
  XP_FREE (s);


#define END()										\
  if (!(s = msg_format_newsrc_set (set))) abort ();	\
  printf ("%3lu: %s\n\n", set->length, s);			\
  XP_FREE (s);										\
  msg_free_newsrc_set (set, &allocinfo)


  START("20-40,72-99,105,107,110-111,117-200");

  FROB(205, 208);
  FROB(50, 70);
  FROB(0, 10);
  FROB(112, 113);
  FROB(101, 101);
  FROB(5, 75);
  FROB(103, 109);
  FROB(2, 20);
  FROB(1, 9999);

  END();


#undef START
#undef FROB
#undef END
}




static void
test_member (XP_Bool with_cache)
{
  msg_NewsRCSet *set;
  char *s;

  s = "1-70,72-99,105,107,110-111,117-200";
  printf ("\n\nTesting %s (with%s cache)\n", s, with_cache ? "" : "out");
  if (!(set = msg_parse_newsrc_set (s, &allocinfo)))
	abort ();

#define TEST(N)											   \
  if (! with_cache) set->cached_value = -1;				   \
  if (!(s = msg_format_newsrc_set (set))) abort ();		   \
  printf (" %3d = %s\n", N,								   \
		  (msg_NewsRCIsRead (set, N) ? "true" : "false")); \
  XP_FREE (s);

  TEST(-1);
  TEST(0);
  TEST(1);
  TEST(20);
  
  msg_free_newsrc_set (set, &allocinfo);
  s = "0-70,72-99,105,107,110-111,117-200";
  printf ("\n\nTesting %s (with%s cache)\n", s, with_cache ? "" : "out");
  if (!(set = msg_parse_newsrc_set (s, &allocinfo)))
	abort ();
  
  TEST(-1);
  TEST(0);
  TEST(1);
  TEST(20);
  TEST(69);
  TEST(70);
  TEST(71);
  TEST(72);
  TEST(73);
  TEST(74);
  TEST(104);
  TEST(105);
  TEST(106);
  TEST(107);
  TEST(108);
  TEST(109);
  TEST(110);
  TEST(111);
  TEST(112);
  TEST(116);
  TEST(117);
  TEST(118);
  TEST(119);
  TEST(200);
  TEST(201);
  TEST(65535);
#undef TEST

  msg_free_newsrc_set (set, &allocinfo);
}


static void
test_newsrc (char *file)
{
  FILE *fp = fopen (file, "r");
  char buf [1024];
  if (! fp) abort ();
  while (fgets (buf, sizeof (buf), fp))
	{
	  if (!strncmp (buf, "options ", 8))
		fwrite (buf, 1, strlen (buf), stdout);
	  else
		{
		  char *sep = buf;
		  while (*sep != 0 && *sep != ':' && *sep != '!')
			sep++;
		  if (*sep) sep++;
		  while (isspace (*sep)) sep++;
		  fwrite (buf, 1, sep - buf, stdout);
		  if (*sep)
			{
			  char *s;
			  msg_NewsRCSet *set = msg_parse_newsrc_set (sep, &allocinfo);
			  if (! set)
				abort ();
			  if (! msg_OptimizeNewsRCSet (set))
				abort ();
			  if (! ((s = msg_format_newsrc_set (set))))
				abort ();
			  msg_free_newsrc_set (set, &allocinfo);
			  fwrite (s, 1, strlen (s), stdout);
			  free (s);
			  fwrite ("\n", 1, 1, stdout);
			}
		}
	}
  fclose (fp);
}

void
main ()
{

  test_decoder ("");
  test_decoder (" ");
  test_decoder ("0");
  test_decoder ("1");
  test_decoder ("123");
  test_decoder (" 123 ");
  test_decoder (" 123 4");
  test_decoder (" 1,2, 3, 4");
  test_decoder ("0-70,72-99,100,101");
  test_decoder (" 0-70 , 72 - 99 ,100,101 ");
  test_decoder ("0 - 268435455");
  /* This one overflows - we can't help it.
	 test_decoder ("0 - 4294967295"); */

  test_adder ();

  test_ranges();

  test_member (FALSE);
  test_member (TRUE);

  test_newsrc ("/u/montulli/.newsrc");
  /* test_newsrc ("/u/jwz/.newsrc");*/
}

/* Stupid routines to help me link the test case. */

void msg_undo_StartBatch(MWContext* context) {}
void msg_undo_EndBatch(MWContext* context) {}
int msg_GrowBuffer (uint32 desired_size, uint32 element_size, uint32 quantum,
					char **buffer, uint32 *size) {}
int msg_LineBuffer (const char *net_buffer, int32 net_buffer_size,
					char **bufferP, uint32 *buffer_sizeP, uint32 *buffer_fpP,
					XP_Bool convert_newlines_p,
					int32 (*per_line_fn) (char *line, uint32 line_length,
										  void *closure),
					void *closure) {}
struct MSG_Folder *msg_FindNewsgroupFolder(MWContext *context,
										   const char *group) {}
int msg_UpdateNewsFolderMessageCount (MWContext *context,
									  struct MSG_Folder *folder,
									  uint32 unread_messages,
									  XP_Bool remove_if_empty) {}


int MKLib_trace_flag;

#endif /* 0 */


#ifdef PROFILE
#pragma profile off
#endif
