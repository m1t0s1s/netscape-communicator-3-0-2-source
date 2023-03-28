/* -*- Mode: C; tab-width: 8 -*-
   icons.h --- icon creation stuff
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 24-Jul-95.
 */
#ifndef __xfe_icons_h_
#define __xfe_icons_h_

typedef struct fe_icon {
  Pixmap pixmap;
  Pixmap mask;
  Dimension width, height;
} fe_icon;

extern void
fe_MakeIcon(MWContext *context, Pixel transparent_color, fe_icon* result,
	    char *name,
	    int width, int height,
	    unsigned char *mono_data,
	    unsigned char *color_data,
	    unsigned char *mask_data,
	    Boolean hack_mask_and_cmap_p);


#endif /* __xfe_icons_h_ */
