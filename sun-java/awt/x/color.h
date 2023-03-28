/*
 * @(#)color.h	1.8 95/11/27 Sami Shaio
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#ifndef _COLOR_H_
#define _COLOR_H_

#include "awt.h"

#define CMAP_BITS		3
#define CMAP_COLS		(1 << CMAP_BITS)
#define CMAP_BIAS		(1 << (7 - CMAP_BITS))

typedef struct ColorEntry {
	unsigned char r, g, b;
	unsigned char flags;
} ColorEntry;

extern int awt_num_colors;
extern ColorEntry awt_Colors[256];

extern unsigned char awt_RGBCube[CMAP_COLS + 1][CMAP_COLS + 1][CMAP_COLS + 1];

extern unsigned char grays[256];
extern unsigned char bwgamma[256];

typedef struct {
    int Depth;
    int awtBitsPerPixel;
    int rOff;
    int gOff;
    int bOff;
    int rChop; /* these are only for TrueColor... */
    int gChop;
    int bChop;
    XVisualInfo wsVisualInfo;
    XPixmapFormatValues wsImageFormat;
    int (*ColorMatch)(int, int, int);
} awtImageData;

extern awtImageData *awtImage;

/*
 * Find the best color
 */
extern int awt_color_match(int r, int g, int b);


extern int awt_allocate_colors(void);

#endif           /* _COLOR_H_ */
