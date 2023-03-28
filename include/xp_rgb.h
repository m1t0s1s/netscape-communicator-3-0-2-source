/* -*- Mode: C; tab-width: 4 -*-
   xp_rgb.c --- parsing color names to RGB triplets.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: John Giannandrea <jg@netscape.com>, 28-Sep-95
 */

#include "xp.h"

XP_BEGIN_PROTOS

/* Looks up the passed name via a caseless compare in a static table
   of color names.  If found it sets the passed pointers to contain
   the red, green, and blue values for that color.  On success the return
   code is 0.  Returns 1 if no match is found for the color.
 */
extern intn XP_ColorNameToRGB(char *name, uint8 *r, uint8 *g, uint8 *b);

XP_END_PROTOS
