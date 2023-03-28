/*
 * @(#)color.c	1.14 95/11/27 Arthur van Hoff, Patrick Naughton
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

/*-
 *	Image dithering and rendering code for X11.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "color.h"
#include "debug.h"
#include "java_awt_Color.h"

/* Constraint on the remaining number of colors that can be allocated
   for PseudoColor visuals */
static unsigned int remaining_colors = (unsigned int)-1;

static unsigned int GetMaxColors(void) {
    return (unsigned int)-1;
}
unsigned int (*awt_GetMaxColors)(void) = GetMaxColors;

#define CLIP(val,min,max)	((val < min) ? min : ((val > max) ? max : val))

/* efficiently map an RGB value to a pixel value */
#define RGBMAP(r, g, b)		\
	(awt_RGBCube[(int)((r) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]	\
		    [(int)((g) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]	\
		    [(int)((b) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)])

#define COMPOSETC(I, r, g, b)		\
	((((b) & 0xff) << ((I)->bOff)) |	\
	 (((g) & 0xff) << ((I)->gOff)) |	\
	 (((r) & 0xff) << ((I)->rOff)))

#define RGBTOGRAY(r, g, b) ((int) (.299 * r + .587 * g + .114 * b))

enum {
    FREE_COLOR		= 0,
    LIKELY_COLOR	= 1,
    ALLOCATED_COLOR	= 2
};

int awt_num_colors;
ColorEntry awt_Colors[256];

/* the color cube */
unsigned char awt_RGBCube[CMAP_COLS + 1][CMAP_COLS + 1][CMAP_COLS + 1];

/* the StaticGray ramp */
unsigned char grays[256];
unsigned char bwgamma[256];

/* the function pointers for doing image manipulation */
awtImageData *awtImage;

int awt_color_match(int, int, int);
#if 0
int awt_color_match24(int, int, int);
#endif
int awt_color_matchTC(int, int, int);
int awt_color_matchGS(int, int, int);

awtImageData awtImage8 = {
    8,
    8,
    0, 0, 0,
    0, 0, 0,
    { 0 },
    { 8, 8, 8 },
    awt_color_match
};

#if 0
awtImageData awtImage24 = {
    24,
    32,
    0, 8, 16,
    { 0 },
    { 8, 8, 8 },
    awt_color_match24
};
#endif
awtImageData awtImageTC = {
    0, /* these need to be computed as we go... */
    32, /* although who uses this one? (awtBitsPerPixel) */
    0, 0, 0,
    0, 0, 0,
    { 0 },
    { 8, 8, 8 },
    awt_color_matchTC
};

awtImageData awtImageGS = {
    8,
    8,
    -1, -1, -1,
    0, 0, 0, /* not used */
    { 0 },
    { 8, 8, 8 },
    awt_color_matchGS
};

#ifdef DEBUG
static int debug_colormap = 0;
#endif

/*
 * Find the best color.
 */
#if 0
int
awt_color_match24(int r, int g, int b)
{
    r = CLIP(r, 0, 255);
    g = CLIP(g, 0, 255);
    b = CLIP(b, 0, 255);
    return COMPOSE24(awtImage, r, g, b);
}
#endif
int
awt_color_matchTC(int r, int g, int b)
{
    r = (r&255) >> awtImage->rChop;
    g = (g&255) >> awtImage->gChop;
    b = (b&255) >> awtImage->bChop;
    return COMPOSETC(awtImage, r, g, b);
}

int
awt_color_matchGS(int r, int g, int b)
{
    r = CLIP(r, 0, 255);
    g = CLIP(g, 0, 255);
    b = CLIP(b, 0, 255);
    return grays[RGBTOGRAY(r, g, b)];
}

int
awt_color_match(int r, int g, int b)
{
    int mindist = 256 * 256 * 256, besti = 0;
    int i, t, d;
    ColorEntry *p = awt_Colors;

    r = CLIP(r, 0, 255);
    g = CLIP(g, 0, 255);
    b = CLIP(b, 0, 255);

    for (i = 0 ; i < awt_num_colors ; i++, p++)
	if (p->flags == ALLOCATED_COLOR) {
	    t = p->r - r;
	    d = t * t;
	    if (d >= mindist)
		continue;
	    t = p->g - g;
	    d += t * t;
	    if (d >= mindist)
		continue;
	    t = p->b - b;
	    d += t * t;
	    if (d >= mindist)
		continue;
	    if (d == 0)
		return i;
	    if (d < mindist) {
		besti = i;
		mindist = d;
	    }
	}
    return besti;
}

/*
 * Allocate a color in the X color map and return the index.
 */
static int
alloc_col(Display *dpy, Colormap cm, int r, int g, int b)
{
    XColor col;

    r = CLIP(r, 0, 255);
    g = CLIP(g, 0, 255);
    b = CLIP(b, 0, 255);

    col.flags = DoRed | DoGreen | DoBlue;
    col.red   = (r << 8) | r;
    col.green = (g << 8) | g;
    col.blue  = (b << 8) | b;
#ifdef NETSCAPE
    if ((remaining_colors > 0) && XAllocColor(dpy, cm, &col)) {
        remaining_colors--;
#else
    if (XAllocColor(dpy, cm, &col)) {
#endif
#ifdef DEBUG
	if (debug_colormap)
	    printf("allocated %d (%d,%d, %d)\n", col.pixel, r, g, b);
#endif
	awt_Colors[col.pixel].flags = ALLOCATED_COLOR;
	awt_Colors[col.pixel].r = col.red   >> 8;
	awt_Colors[col.pixel].g = col.green >> 8;
	awt_Colors[col.pixel].b = col.blue  >> 8;
	return col.pixel;
    }

    return awt_color_match(r, g, b);
}

/*
 * called from X11Server_create() in xlib.c 
 */
int
awt_allocate_colors()
{
    Display *dpy;
    unsigned long freecolors[256], plane_masks[1];
    XColor cols[256];
    Colormap cm;
    int i, ri, gi, bi, nfree, depth, screen;
    XPixmapFormatValues *pPFV;
    int numpfv;
    XVisualInfo *pVI;
    char *forcemono;
    char *forcegray;

    forcemono = getenv("FORCEMONO");
    forcegray = getenv("FORCEGRAY");
    if (forcemono && !forcegray)
	forcegray = forcemono;

    /*
     * Get the colormap and make sure we have the right visual
     */
    dpy = awt_display;
    screen = awt_screen;
    cm = awt_cmap;
    depth = awt_depth;
    pVI = &awt_visInfo;
    awt_num_colors = awt_visInfo.colormap_size;

    if (depth > 8) {
	int size;
	awtImage = &awtImageTC;
	awtImage->Depth = depth;
	awtImage->rOff = 0;
	for (i = pVI->red_mask; (i & 1) == 0; i >>= 1) {
	    awtImage->rOff++;
	}
	size = 0;
	for (; i; i >>= 1) {
	    size++;
	}
	awtImage->rChop = 8-size;
	awtImage->gOff = 0;
	for (i = pVI->green_mask; (i & 1) == 0; i >>= 1) {
	    awtImage->gOff++;
	}
	size = 0;
	for (; i; i >>= 1) {
	    size++;
	}
	awtImage->gChop = 8-size;
	awtImage->bOff = 0;
	for (i = pVI->blue_mask; (i & 1) == 0; i >>= 1) {
	    awtImage->bOff++;
	}
	size = 0;
	for (; i; i >>= 1) {
	    size++;
	}
	awtImage->bChop = 8-size;
    } else if (pVI->class == StaticGray || pVI->class == GrayScale
	       || (forcegray && depth <= 8)) {
	awtImage = &awtImageGS;
	awtImage->Depth = depth;
    } else {
	awtImage = &awtImage8;
	awtImage->Depth = depth;
#ifdef NETSCAPE
	remaining_colors = (*awt_GetMaxColors)();
#endif
    }

    pPFV = XListPixmapFormats(dpy, &numpfv);
    if (pPFV) {
	for (i = 0; i < numpfv; i++) {
	    if (pPFV[i].depth == depth) {
		awtImage->wsImageFormat = pPFV[i];
		break;
	    }
	}
	XFree(pPFV);
    }

    if (depth > 8) {
	return 1;
    }

    if (awt_num_colors > 256) {
	return 0;
    }

    /*
     * Initialize colors array
     */
    for (i = 0; i < awt_num_colors; i++) {
	cols[i].pixel = i;
    }

    XQueryColors(dpy, cm, cols, awt_num_colors);
    for (i = 0; i < awt_num_colors; i++) {
	awt_Colors[i].r = cols[i].red >> 8;
	awt_Colors[i].g = cols[i].green >> 8;
	awt_Colors[i].b = cols[i].blue >> 8;
	awt_Colors[i].flags = LIKELY_COLOR;
    }

    /*
     * Determine which colors in the colormap can be allocated and mark
     * them in the colors array
     */
    nfree = 0;
    for (i = 128; i > 0; i >>= 1) {
	if (XAllocColorCells(dpy, cm, False, plane_masks, 0,
			     freecolors + nfree, i)) {
	    nfree += i;
	}
    }

    for (i = 0; i < nfree; i++) {
	awt_Colors[freecolors[i]].flags = FREE_COLOR;
    }

#ifdef DEBUG
    if (debug_colormap) {
	printf("%d free.\n", nfree);
    }
#endif

    XFreeColors(dpy, cm, freecolors, nfree, 0);

    /*
     * Allocate the colors that are already allocated by other
     * applications
     */
    for (i = 0; i < awt_num_colors; i++) {
	if (awt_Colors[i].flags == LIKELY_COLOR) {
	    awt_Colors[i].flags = FREE_COLOR;
	    alloc_col(dpy, cm,
		      awt_Colors[i].r,
		      awt_Colors[i].g,
		      awt_Colors[i].b);
	}
    }
#ifdef DEBUG
    if (debug_colormap) {
	printf("got the already allocated ones\n");
    }
#endif

    /*
     * Allocate more colors, filling the color space evenly.
     */

    alloc_col(dpy, cm, 255, 255, 255);
    alloc_col(dpy, cm, 255, 0, 0);

    if (awtImage == &awtImageGS) {
	int g;

	if (!forcemono) {
	    for (i = 128; i > 0; i >>= 1) {
		for (g = i; g < 256; g += (i * 2)) {
		    alloc_col(dpy, cm, g, g, g);
		}
	    }
	}

	for (g = 0; g < 256; g++) {
	    ColorEntry *p;
	    int mindist, besti;
	    int d;

	    p = awt_Colors;
	    mindist = 256;
	    besti = 0;
	    for (i = 0 ; i < awt_num_colors ; i++, p++) {
		if (forcegray && (p->r != p->g || p->g != p->b))
		    continue;
		if (forcemono && p->g != 0 && p->g != 255)
		    continue;
		if (p->flags == ALLOCATED_COLOR) {
		    d = p->g - g;
		    if (d < 0) d = -d;
		    if (d < mindist) {
			besti = i;
			if (d == 0) {
			    break;
			}
			mindist = d;
		    }
		}
	    }

	    grays[g] = besti;
	}

	if (forcemono || (depth == 1)) {
	    double pow(double, double);

	    char *gammastr = getenv("HJGAMMA");
	    double gamma = atof(gammastr ? gammastr : "1.6");
	    if (gamma < 0.01) gamma = 1.0;
#ifdef DEBUG
	    if (debug_colormap) {
		fprintf(stderr, "gamma = %f\n", gamma);
	    }
#endif
	    for (i = 0; i < 256; i++) {
		bwgamma[i] = (int) (pow(i/255.0, gamma) * 255);
#ifdef DEBUG
		if (debug_colormap) {
		    fprintf(stderr, "%3d ", bwgamma[i]);
		    if ((i & 7) == 7)
			fprintf(stderr, "\n");
		}
#endif
	    }
	} else {
	    for (i = 0; i < 256; i++) {
		bwgamma[i] = i;
	    }
	}

	/* Now we fill in the color cube so that the RGB dithering
	 * algorithm can dither the grayscale values.
	 */
	for (bi = 0 ; bi <= CMAP_COLS ; bi += 1) {
	    int b = bi << (8 - CMAP_BITS);
	    for (gi = 0 ; gi <= CMAP_COLS ; gi += 1) {
		int g = gi << (8 - CMAP_BITS);
		for (ri = 0 ; ri <= CMAP_COLS ; ri += 1) {
		    int r = ri << (8 - CMAP_BITS);
		    if (awt_RGBCube[ri][gi][bi] == 0) {
			awt_RGBCube[ri][gi][bi] = grays[RGBTOGRAY(r, g, b)];
		    }
		}
	    }
	}

#ifdef DEBUG
	if (debug_colormap) {
	    fprintf(stderr, "GrayScale initialized\n");
	    fprintf(stderr, "color table:\n");
	    for (i = 0; i < awt_num_colors; i++) {
		fprintf(stderr, "%3d: %3d %3d %3d\n",
			i, awt_Colors[i].r, awt_Colors[i].g, awt_Colors[i].b);
	    }
	    fprintf(stderr, "gray table:\n");
	    for (i = 0; i < 256; i++) {
		fprintf(stderr, "%3d ", grays[i]);
		if ((i & 7) == 7)
		    fprintf(stderr, "\n");
	    }
	}
#endif
	return 1;
    }

    alloc_col(dpy, cm, 0, 255, 0);
    alloc_col(dpy, cm, 0, 0, 255);
    alloc_col(dpy, cm, 255, 255, 0);
    alloc_col(dpy, cm, 255, 0, 255);
    alloc_col(dpy, cm, 0, 255, 255);
    alloc_col(dpy, cm, 235, 235, 235);
    alloc_col(dpy, cm, 224, 224, 224);
    alloc_col(dpy, cm, 214, 214, 214);
    alloc_col(dpy, cm, 192, 192, 192);
    alloc_col(dpy, cm, 162, 162, 162);
    alloc_col(dpy, cm, 128, 128, 128);
    alloc_col(dpy, cm, 105, 105, 105);
    alloc_col(dpy, cm, 64, 64, 64);
    alloc_col(dpy, cm, 32, 32, 32);
    alloc_col(dpy, cm, 255, 128, 128);
    alloc_col(dpy, cm, 128, 255, 128);
    alloc_col(dpy, cm, 128, 128, 255);
    alloc_col(dpy, cm, 255, 255, 128);
    alloc_col(dpy, cm, 255, 128, 255);
    alloc_col(dpy, cm, 128, 255, 255);

#ifdef DEBUG
    if (debug_colormap)
	printf("got critical ones\n");
#endif

    /* allocate some colors */
    for (bi = 0 ; bi <= CMAP_COLS ; bi += 4) {
	int b = bi << (8 - CMAP_BITS);
	for (gi = 0 ; gi <= CMAP_COLS ; gi += 2) {
	    int g = gi << (8 - CMAP_BITS);
	    for (ri = 0; ri <= CMAP_COLS ; ri += 1) {
		int r = ri << (8 - CMAP_BITS);
		if (awt_RGBCube[ri][gi][bi] == 0) {
		    awt_RGBCube[ri][gi][bi] = alloc_col(dpy, cm, r, g, b);
		}
	    }
	}
    }
    /* match remainder */
    for (bi = 0 ; bi <= CMAP_COLS ; bi += 1) {
	int b = bi << (8 - CMAP_BITS);
	for (gi = 0 ; gi <= CMAP_COLS ; gi += 1) {
	    int g = gi << (8 - CMAP_BITS);
	    for (ri = 0 ; ri <= CMAP_COLS ; ri += 1) {
		int r = ri << (8 - CMAP_BITS);
		if (awt_RGBCube[ri][gi][bi] == 0) {
		    awt_RGBCube[ri][gi][bi] = awt_color_match(r, g, b);
		}
	    }
	}
    }

#ifdef DEBUG
    if (debug_colormap) {
        printf("got cube\n");
    }
#endif

#ifdef DEBUG
    if (debug_colormap) {
	int alloc_count = 0;
	int reuse_count = 0;
	int free_count = 0;
	int saw[256];
	for (i = 0; i < awt_num_colors; i++) {
	    switch (awt_Colors[i].flags) {
	      case ALLOCATED_COLOR:
		alloc_count++;
		break;
	      case LIKELY_COLOR:
		reuse_count++;
		break;
	      case FREE_COLOR:
		free_count++;
		break;
	    }
	}
	printf("%d total, %d allocated, %d reused, %d still free.\n",
	       awt_num_colors, alloc_count, reuse_count, free_count);

	for (i = 0; i < awt_num_colors; i++) {
	    saw[i] = 0;
	}
	for (bi = 0; bi <= CMAP_COLS; bi++) {
	    for (gi = 0; gi <= CMAP_COLS; gi++) {
		for (ri = 0; ri <= CMAP_COLS; ri++) {
		    saw[awt_RGBCube[ri][gi][bi]]++;
		}
	    }
	}
	for (i = 0; i < awt_num_colors; i++) {
	    printf("%d %d\n", i, saw[i]);
	}
    }
#endif
    return 1;
}

struct Hjava_awt_image_ColorModel *
awt_getColorModel() {
    struct Hjava_awt_image_ColorModel *awt_colormodel;
	if (awt_visInfo.class == TrueColor) {
	    awt_colormodel = (struct Hjava_awt_image_ColorModel *)
		execute_java_constructor(EE(),
					 "java/awt/image/DirectColorModel",
					 0, "(IIIII)",
					 awt_visInfo.depth,
					 awt_visInfo.red_mask,
					 awt_visInfo.green_mask,
					 awt_visInfo.blue_mask,
					 0);
	} else {
	    HArrayOfByte *hR = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
	    HArrayOfByte *hG = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
	    HArrayOfByte *hB = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
	    char *aBase, *a;
	    ColorEntry *c;
	    /* These loops copy backwards so that the aBase pointer won't
	     * be dropped by the optimizer and allow the garbage collector
	     * to move the array being initialized.
	     */
	    aBase = unhand(hR)->body;
	    for (a = aBase + 255, c = awt_Colors + 255; a >= aBase; a--, c--) {
		*a = c->r;
	    }
	    aBase = unhand(hG)->body;
	    for (a = aBase + 255, c = awt_Colors + 255; a >= aBase; a--, c--) {
		*a = c->g;
	    }
	    aBase = unhand(hB)->body;
	    for (a = aBase + 255, c = awt_Colors + 255; a >= aBase; a--, c--) {
		*a = c->b;
	    }
	    awt_colormodel = (struct Hjava_awt_image_ColorModel *)
		execute_java_constructor(EE(),
					 "java/awt/image/IndexColorModel",
					 0, "(II[B[B[B)",
					 awt_visInfo.depth,
					 awt_num_colors,
					 hR, hG, hB);
	}
    return awt_colormodel;
}

#define red(v)		(((v) >> 16) & 0xFF)
#define green(v)	(((v) >>  8) & 0xFF)
#define blue(v)		(((v) >>  0) & 0xFF)

int
awt_getColor(struct Hjava_awt_Color *this)
{
    if (this) {
	int col = unhand(this)->pData;
	if (col) {
	    return col - 1;
	}
	col = unhand(this)->value;
	col = awtImage->ColorMatch(red(col), green(col), blue(col));
	unhand(this)->pData = col + 1;
	return col;
    } 
    return 0;
}
