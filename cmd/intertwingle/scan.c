/* -*- Mode: C; tab-width: 4 -*-
   scan.c --- extract text from MIME objects.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-Jul-97.
 */

#include <stdio.h>
#include "config.h"
#include "mbox.h"

#include "libmime.h"	/* for MimeHeaders, for leaf.h */

#include "leaf.h"
#include "pprint.h"

int IT_READ_ERROR = -69690;	/* #### */
int IT_SEEK_ERROR = -69691;	/* #### */

typedef struct msg {
  int number;
  LeafParseObject *pobj;
} msg;


static void *
msg_start (const char *filename, int32 start_pos, void *closure)
{
  int *count = (int *) closure;
  msg *m = XP_NEW_ZAP(msg);
  if (!m) return 0;
  m->number = (*count)++;

  m->pobj = leaf_init (filename, start_pos,
					   pprint_begin, pprint_attachment, pprint_end,
					   (void *) stdout);
  if (!m->pobj)
    {
      XP_FREE(m);
      return 0;
    }

  return m;
}

static int
msg_write (void *closure, char *buf, int32 size)
{
  msg *m = (msg *) closure;
  return leaf_write(m->pobj, buf, size);
}

static int
msg_close(void *closure, Bool error_p)
{
  msg *m = (msg *) closure;
  int status;
  status = leaf_done (m->pobj, error_p);
  free (m);
  return status;
}


int
main (int argc, char **argv)
{
  int count = 0;
  int status = 0;
  const char *name;
  FILE *in;
  if (argc == 1)
    in = stdin, name = 0;
  else if (argc == 2)
    {
	  name = argv[1];
      in = fopen(name, "r");
      if (!in) exit(1);
    }
  else
    {
      fprintf(stderr, "usage: %s [ mbox-file ]\n", argv[0]);
      exit(1);
    }

  status = map_over_messages (in, name,
							  msg_start, msg_write, msg_close, &count);
  fprintf(stdout, "\n");
  exit (status < 0 ? 1 : 0);
}
