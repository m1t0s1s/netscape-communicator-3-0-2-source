/* -*- Mode: C; tab-width: 4 -*-
   mbox.c --- BSD/SYSV mailbox file parser.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Jul-97.

   This sectionizes a mailbox file into its component messages; it understands
   both the BSD and Solaris formats ("From " and "Content-Length".)  See also
   http://www.netscape.com/eng/mozilla/2.0/relnotes/demo/content-length.html
   for related ranting.
 */

#include <stdio.h>

#ifdef STANDALONE

# define int32 int
# define XP_Bool int
# define FALSE 0
# define TRUE 1
# define XP_ASSERT(x)
# define XP_ASSERT(x)
# define IT_SEEK_ERROR -2
# define IT_READ_ERROR -3
# define MK_OUT_OF_MEMORY -4

#else /* !STANDALONE */

# include "config.h"
# include "mbox.h"

extern int IT_SEEK_ERROR;
extern int IT_READ_ERROR;
extern int MK_OUT_OF_MEMORY;

#endif /* !STANDALONE */


#define TRUST_CONTENT_LENGTH
#define CONTENT_LENGTH_SLACK 100

#ifdef TRUST_CONTENT_LENGTH

/* Returns the file-byte-index of the end of the message, if it can be 
   determined from the given Content-Length, and if it checks out.
   If the Content-Length is obvious nonsense, or if any read/seek errors
   occur, returns IT_SEEK_ERROR.

   The file-read-pointer is expected to be just past the headers of the
   message in question (poised to read the first byte of the body.)
   It will be left in that same place upon return.

   The Content-Length is checked for sanity (by examining the file at
   the indicated position) and is adjusted if it seems to be off by a
   small amount.  It can be off by up to N bytes in either direction;
   the correct value is assumed to be the closer of the two nearest
   message-delimiters within that range (either earlier or later.)
 */
static int
find_content_end (FILE *in, int32 content_length)
{
  char buf [CONTENT_LENGTH_SLACK * 2 + 1];
  int32 start_pos = ftell (in);
  int32 eof = -1;
  int read_start, read_end, target;
  int32 early_match, late_match;
  int bytes;
  int32 result = -1;
  char *s;

  XP_ASSERT(content_length >= 0 && start_pos >= 0);
  if (content_length < 0 || start_pos < 0) return -1;

  target = start_pos + content_length;
  read_start = target - ((sizeof(buf)-1) / 2);

  /* Read a block centered on the position indicated by Content-Length
	 (with `slack' bytes in either direction.) */
  {
    char *b = buf;
    int bytes_left = sizeof(buf)-1;

    if (read_start < start_pos)
      {
		bytes_left -= (start_pos - read_start);
		read_start = start_pos;
      }

    if (fseek (in, read_start, SEEK_SET))
	  /* can't seek there? */
	  goto DONE;

    do
      {
		bytes = fread (b, 1, bytes_left, in);
		if (bytes == 0)
		  {
			eof = read_start + (b - buf);
			break;
		  }
		else if (bytes < 0)
		  {
			result = IT_READ_ERROR;
			goto DONE;
		  }

		b += bytes;
		bytes_left -= bytes;
      } while (bytes_left > 0);

    read_end = read_start + (b - buf);
  }

  /* Find the first mailbox delimiter at or before the position indicated
     by Content-Length.
  */
  early_match = -1;
  s = buf + (target - read_start) + 6;
  if (s > buf + (read_end - read_start))
    s = buf + (read_end - read_start);
  s -= 6;
  for (; s > buf; s--)
    if (*s == '\n' && !strncmp (s, "\nFrom ", 6))
      {
		early_match = read_start + (s - buf);
		break;
      }

  /* Find the first mailbox delimiter at or past the position indicated
     by Content-Length.
  */
  late_match = -1;
  if (read_end > target)
    {
      s = buf + (target - read_start) - 6;
      for (; s < (buf + (read_end - read_start) - 6); s++)
		if (*s == '\n' && !strncmp (s, "\nFrom ", 6))
		  {
			late_match = read_start + (s - buf);
			break;
		  }
    }

  if (late_match == -1 && eof != -1)
    {
      /* EOF counts as a match. */
      late_match = eof - 1;
    }

  if (early_match == late_match)
    {
      if (late_match < target)
		late_match = -1;
      else
		early_match = -1;
    }
  else if (early_match == -1 && late_match < target)
    {
      early_match = late_match;
      late_match = -1;
    }

  if (early_match >= 0 && late_match >= 0)
    {
      /* Found two -- pick the closest one. */
      if ((target - early_match) < (late_match - target))
		result = early_match;
      else
		result = late_match;
    }
  else if (early_match >= 0)
    result = early_match;
  else
    result = late_match;

#ifdef DEBUG
  if (result == -1)
    fprintf (stderr,
			 "\ncontent-length is nonsense (or off by more than %d bytes)\n",
			 CONTENT_LENGTH_SLACK);
  else if (result != target)
    {
      if (result == late_match)
		fprintf (stderr,
				 "\ncontent-length was %ld bytes too small (pos %ld)\n",
				 (long) late_match - target, (long) result);
      else
		fprintf (stderr,
				 "\ncontent-length was %ld bytes too large (pos %ld)\n",
				 (long) target - early_match, (long) result);
    }
#endif /* DEBUG */

 DONE:
  if (fseek (in, start_pos, SEEK_SET))
	return IT_SEEK_ERROR;

  return result;
}

#endif /* TRUST_CONTENT_LENGTH */


int
map_over_messages (FILE *in, const char *filename,
				   void * (*start_msg) (const char *filename,
										int32 start_pos,
										void *closure),
				   int (*write_block) (void *closure, char *buf, int32 size),
				   int (*close_msg) (void *closure, XP_Bool error_p),
				   void *closure)
{
  char buf [10240];
  char *s;
  int32 cl = -1;
  XP_Bool got_envelope = FALSE;
  int32 start = -1;
  int32 end = -1;
  int status = 0;
  void *msg_closure = 0;

#if 0
#ifdef DEBUG
  int32 msg_num = 0;
#endif /* DEBUG */
#endif

  while ((s = fgets (buf, sizeof(buf)-1, in)))
	{
	  if ((*s == 'F' && !strncmp (s, "From ", 5)))
		{
		  start = ftell(in);
		  got_envelope = TRUE;
		  cl = -1;
		  continue;
		}

	  if (!got_envelope)
		continue;

	  if (!msg_closure)
		{
		  msg_closure = start_msg (filename, ftell(in), closure);
		  if (!msg_closure) return MK_OUT_OF_MEMORY;
		}

	  /* Pass along the headers. */
	  status = write_block (msg_closure, s, strlen(s));
	  if (status < 0)
		{
		  close_msg (msg_closure, TRUE);
		  return status;
		}

	  /* Notice the Content-Length header as it goes by. */
	  if ((*s == 'C' || *s == 'c') && !strncasecmp (s, "Content-Length:", 15))
		{
		  cl = -1;
		  sscanf (s+15, " %ld", &cl);
		}
	  /* Blank line means end of headers.  Process the body. */
	  else if (*s == '\n' || (*s == '\r' && s[1] == '\n'))
		{
		  got_envelope = FALSE;
#ifdef TRUST_CONTENT_LENGTH
		  end = (cl < 0 ? -1 : find_content_end (in, cl));
		  cl = -1;

		  if (end != -1)
			{
			  /* Read a SYSV message. */
			  int bytes;
			  int bytes_left = (end - ftell (in));
			  do
				{
				  int bb = sizeof(buf)-1;
				  if (bb > bytes_left)
					bb = bytes_left;
				  bytes = fread (buf, 1, bb, in);
				  if (bytes == 0)
					break;
				  else if (bytes < 0)
					return IT_READ_ERROR;

				  bytes_left -= bytes;

				  status = write_block (msg_closure, buf, bytes);
				  if (status < 0)
					{
					  close_msg (msg_closure, TRUE);
					  return status;
					}
				} while (bytes_left > 0);
			  end = ftell(in);
			}
		  else
#endif /* TRUST_CONTENT_LENGTH */
			{
			  /* Read a BSD message. */

#if 0
#if defined(DEBUG) && defined(TRUST_CONTENT_LENGTH)
			  fprintf(stderr, "\t\t\tmsg %ld is BSD\n", msg_num);
#endif /* DEBUG && TRUST_CONTENT_LENGTH */
#endif

			  while ((s = fgets (buf, sizeof(buf)-1, in)))
				{
				  if (*s == 'F' && !strncmp (s, "From ", 5))
					{
					  end = ftell(in) - (strlen(s) + 1);
					  if (fseek (in, end, SEEK_SET))
						return IT_SEEK_ERROR;
					  break;
					}

				  status = write_block (msg_closure, buf, strlen(s));
				  if (status < 0)
					{
					  close_msg (msg_closure, TRUE);
					  return status;
					}
				}
			  if (end <= start)
				end = ftell(in) - 1;
			}

		  status = close_msg (msg_closure, FALSE);
		  if (status < 0) return status;
		  msg_closure = 0;

#if 0
#ifdef DEBUG
		  fprintf(stderr, "msg %ld: %ld - %ld (%ld)\n", msg_num,
				  start, end, end-start);
		  msg_num++;
#endif /* DEBUG */
#endif
		}
	}

  if (msg_closure)
	{
	  status = close_msg (msg_closure, FALSE);
	  if (status < 0) return status;
	}

  return 0;
}



#ifdef STANDALONE

static int debug_num = 0;
static int debug_size = 0;
static int debug_tick = 0;

static void *
debug_start(void *ignore)
{
  if (debug_tick) abort();
  debug_tick++;
  debug_num++;
  debug_size = 0;
/*  fprintf(stderr, "open %d\n", debug_num-1); */
  return (void*) debug_num;
}

static int
debug_write (void *closure, char *buf, int32 size)
{
/*  fprintf(stderr, "write %d %ld\n", debug_num-1, size); */
  if (((int)closure) != debug_num) abort();
  debug_size += size; return 1; 
}

static int
debug_close(void *closure, XP_Bool error_p)
{
  debug_tick--;
  if (debug_tick) abort();
  if (((int)closure) != debug_num) abort();
  fprintf (stderr, "close %d: size %d\n", debug_num-1, debug_size);
  return 1; 
}


int
main (int argc, char **argv)
{
  FILE *f = fopen(argv[1], "r");
  map_over_messages (f, argv[1], debug_start, debug_write, debug_close, 0);
  if (debug_tick) abort();
  fclose(f);
  return 0;
}

#endif /* STANDALONE */
