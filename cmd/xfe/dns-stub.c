/* -*- Mode: C; tab-width: 8 -*-
   dns-stub.c --- SunOS sucks and then you die.
   This is linked into the SunOS executable which is NOT linked with -lresolv.
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 31-Oct-94.
 */

#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

struct state _res;

int 
res_init (void)
{
  return 0;
}
