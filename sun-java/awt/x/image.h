/*
 * @(#)image.h	1.28 95/12/01 Jim Graham
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

/*
 * It is not optimal to support all image conversion operations with a
 * single function.  Typically, a number of variations of the generalized
 * image function are created based on making a number of assumptions about
 * various input and output options which are likely to occur frequently
 * and which simplify the conversion loops.
 *
 * Input data can be:
 *	8 or 32 bits per pixel
 *	all opaque, opaque & transparent, fully alpha qualified
 *	in its own kind of ColorModel, in one of the default ColorModel
 *		classes, or in the same ColorModel as the screen
 *	an opaque bg color or not (to resolve alpha/transparency)
 *
 * The output (screen data) may need:
 *	1, 4, 8, 16, or 32 bits per pixel
 *	a transparency mask or not
 *	dithering (< 8 bit output) or closest color approximations
 *	a different size than the input data
 *
 * Providing a variation for 8 or 32 bit input is a necessity.
 * Providing a variation for output bits per pixel is a necessity.  Also,
 *	different depths will be needed on different machines/displays.
 * Providing a variation for dithering versus closest color is also pretty
 *	much a necessity (if we assume that we will support dithering).
 * Providing a variation for a generic ColorModel versus IndexColorModel
 *	or DirectColorModel is a high priority optimization.
 * Providing a variation for simple Alpha is a fairly high priority
 *	optimization, but can be solved with special case tests inside
 *	the rendering loop.
 *
 * The generic function will be available for any of the cases that a
 * given platform or implementor does not wish to specialize.  This
 * function will:
 *
 *	- Choose a source coordinate for the destination coordinate
 *	- Use a switch and pointer casting for input bits per pixel to
 *		fetch a pixel into a 32-bit int
 *	- Use a Java callback to the ColorModel to convert the pixel
 *		into a 32-bit Alpha/R/G/B quantity
 *	- If a solid background color is provided, then use the Alpha
 *		to blend the R/G/B of the pixel with the bg color
 *	- Else, use a threshold on the Alpha to determine transparency
 *		and update the transparency mask
 *	- Use closest color approximation to choose the output pixel as
 *		a 32-bit int
 *	- Use a switch and pointer casting for output bits per pixel to
 *		store the 32-bit pixel into the output buffer
 *
 * Some support needs to be provided by the device:
 *
 *	- A macro to choose the closest pixel for an RGB triplet
 *	- A macro to map a pixel quickly to a color cube
 *	- A macro to allocate an output pixel store for a given
 *		width/height/depth
 *	- A macro to allocate a transparency mask
 *	- Macros to manipulate output pixel stores and masks:
 *		- Get a pointer to a scan line
 *		- Store a pixel and increment pointer
 *		- Increment to next scan line
 *
 * Note that many of these macros will take as an argument a pointer to
 * a structure provided by the device to describe its pixel data and that
 * structure can contain pointers to functions to implement any of the
 * above.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "monitor.h"
#include "color.h"
#include "awt.h"
#include "java_awt_image_ColorModel.h"
#include "sun_awt_image_Image.h"
#include "sun_awt_image_ImageRepresentation.h"

/*
 * This union is a utility structure for manipulating pixel pointers
 * of variable depths.
 */
typedef union {
    void *vp;
    unsigned char *bp;
    unsigned short *sp;
    unsigned int *ip;
} pixptr;

#define SCALEFAILURE	-1
#define SCALENOOP	0
#define SCALESUCCESS	1

#define ALPHASHIFT	24
#define REDSHIFT	16
#define GREENSHIFT	8
#define BLUESHIFT	0
#define COLORMASK	((0xff << REDSHIFT) |	\
			 (0xff << GREENSHIFT) |	\
			 (0xff << BLUESHIFT))

#define ALPHABLEND(fg, a, bg) \
	((bg) + (((a) * ((fg) - (bg))) / 255))

#define RGBTOGRAY(r, g, b) ((int) (.299 * r + .587 * g + .114 * b))

/*
 * The following mapping is used between coordinates when scaling an
 * image:
 *
 *	srcXY = floor(((dstXY + .5) * srcWH) / dstWH)
 *	      = floor((dstXY * srcWH + .5 * srcWH) / dstWH)
 *	      = floor((2 * dstXY * srcWH + srcWH) / (2 * dstWH))
 *
 * Since the numerator can always be assumed to be non-negative for
 * all values of dstXY >= 0 and srcWH,dstWH >= 1, then the floor
 * function can be calculated using the standard C integer division
 * operator.
 *
 * To calculate back from a source range of pixels to the destination
 * range of pixels that they will affect, we need to find a srcXY
 * that satisfies the following inequality based upon the above mapping
 * function:
 *
 *	srcXY <= (2 * dstXY * srcWH + srcWH) / (2 * dstWH) < (srcXY+1)
 *	2 * srcXY * dstWH <= 2 * dstXY * srcWH + srcWH < 2 * (srcXY+1) * dstWH
 *
 * To calculate the lowest dstXY that satisfies these constraints, we use
 * the first half of the inequality:
 *
 *	2 * dstXY * srcWH + srcWH >= 2 * srcXY * dstWH
 *	2 * dstXY * srcWH >= 2 * srcXY * dstWH - srcWH
 *	dstXY >= (2 * srcXY * dstWH - srcWH) / (2 * srcWH)
 *	dstXY = ceil((2 * srcXY * dstWH - srcWH) / (2 * srcWH))
 *	dstXY = floor((2 * srcXY * dstWH - srcWH + 2*srcWH - 1) / (2 * srcWH))
 *	dstXY = floor((2 * srcXY * dstWH + srcWH - 1) / (2 * srcWH))
 *
 * Since the numerator can be shown to be non-negative, we can calculate
 * this with the standard C integer division operator.
 *
 * To calculate the highest dstXY that satisfies these constraints, we use
 * the second half of the inequality:
 *
 *	2 * dstXY * srcWH + srcWH < 2 * (srcXY+1) * dstWH
 *	2 * dstXY * srcWH < 2 * (srcXY+1) * dstWH - srcWH
 *	dstXY < (2 * (srcXY+1) * dstWH - srcWH) / (2 * srcWH)
 *	dstXY = ceil((2 * (srcXY+1) * dstWH - srcWH) / (2 * srcWH)) - 1
 *	dstXY = floor((2 * (srcXY+1) * dstWH - srcWH + 2 * srcWH - 1)
 *		      / (2 * srcWH)) - 1
 *	dstXY = floor((2 * (srcXY+1) * dstWH + srcWH - 1) / (2 * srcWH)) - 1
 *
 * Again, the numerator is always non-negative so we can use integer division.
 */

#define SRC_XY(dstXY, srcWH, dstWH) \
    (((2 * (dstXY) * (srcWH)) + (srcWH)) / (2 * (dstWH)))

#define DEST_XY_RANGE_START(srcXY, srcWH, dstWH) \
    (((2 * (srcXY) * (dstWH)) + (srcWH) - 1) / (2 * (srcWH)))

#define DEST_XY_RANGE_END(srcXY, srcWH, dstWH) \
    (((2 * ((srcXY) + 1) * (dstWH)) + (srcWH) - 1) / (2 * (srcWH)) - 1)

/*
 * The data structures for tracking various pieces of information about
 * scaling and converting the image data.  REMIND: This should probably
 * be moved to a Solaris-specific header file.
 */

typedef struct {
    int r, g, b;
} DitherError;

typedef struct {
    int a;
} AlphaError;

typedef struct {
    Hsun_awt_image_ImageRepresentation *hJavaObject;	/* backptr handle */
    XID pixmap;			/* The X11 pixmap containing the image */
    XID mask;			/* The X11 pixmap with the transparency mask */
    int bgcolor;		/* The current bg color installed in pixmap */

    int depth;			/* The depth of the destination image */
    int dstW;			/* The width of the destination pixmap */
    int dstH;			/* The height of the destination pixmap */
    int srcW;			/* The width of the original source image */
    int srcH;			/* The height of the original source image */

    void *buffer;		/* The local temp buffer for output pixels */
    XImage *xim;		/* The Ximage structure for the temp buffer */
    void *maskbuf;		/* The local temp buffer for mask bits */
    XImage *maskim;		/* The Ximage structure for the mask */
    int bufwidth;		/* The width of the temp buffer */
    int bufheight;		/* The height of the temp buffer */
    int bufscan;		/* The scan line width of the temp buffer */

    int hints;			/* The delivery hints from the producer */
    DitherError *errors;	/* The dithering errors from previous lines */
    AlphaError *aerrors;	/* The alpha dithering errors */

    int *recode;		/* The src byte to output pixel array */
    unsigned char *isrecoded;	/* Flags indicating src bytes converted */

    Region curpixels;		/* The region of randomly converted pixels */
    struct {
	int num;		/* The last fully delivered scanline */
	char *seen;		/* The lines which have been delivered */
    } curlines;			/* For hints=COMPLETESCANLINES */
} IRData;

extern IRData *image_getIRData(Hsun_awt_image_ImageRepresentation *ir);
extern Drawable image_getIRDrawable(Hsun_awt_image_ImageRepresentation *ir);

/*
 * Macros for manipulating the destination pixel buffer.
 */

extern int image_BufAlloc(IRData *, int, int, int, int);
extern int image_Done(IRData *, int, int, int, int);

#define DeclareDstBufVars(ird) \
    pixptr dstP;				\
    int dstAdjust

#define InitDstBuf(ird, x1, y1, x2, y2) \
    (image_BufAlloc(ird, x1, y1, x2, y2)		\
     && (dstAdjust = ird->bufscan - (x2 - x1) * ird->depth / 8) >= 0)

#define SetDstBufLoc(ird, x, y) \
    (dstP.vp = ird->buffer,				\
     (dstP.bp += ((y - dstY1) * ird->bufscan + (x - dstX1) * ird->depth / 8)))

#define StorePixel(ird, pixel, x, y) \
    (ird->depth == 32 ? (*dstP.ip++ = pixel) : (*dstP.bp++ = pixel))

#define EndDstBufLine(ird) (dstP.bp += dstAdjust)

#define DstBufComplete(ird) \
    ((ep && dstX1 && (ep = ird->errors) && (ep[0].r = er,	\
					    ep[0].g = eg,	\
					    ep[0].b = eb)),	\
     (aep && dstX1 && (aep = ird->aerrors) && (aep[0].a = ea)),	\
     image_Done(ird, dstX1, dstY1, dstX2, dstY2))

/*
 * Macros for manipulating a mask.
 */

typedef unsigned char MaskBits;
extern MaskBits *image_InitMask(IRData *);

#define DeclareMaskVars(ird) \
    MaskBits *mask = ird->maskbuf, *maskp, maskbits, maskcurbit; int maskadjust

#define InitMask(ird, x, y) \
    (mask || (mask = image_InitMask(ird), SetMaskLoc(ird, x, y)))

#define SetMaskLoc(ird, x, y) \
    ((mask = ird->maskbuf)						\
     && (maskp = (mask + (y - dstY1) * ((ird->bufwidth + 7) >> 3)	\
		  + ((x - dstX1) >> 3)),				\
	 maskbits = *maskp,						\
	 maskcurbit = (0x80 >> ((x - dstX1) & 7)),			\
	 maskadjust = (((ird->bufwidth + 7) >> 3)			\
		       - ((dstX2 >> 3) - (dstX1 >> 3)))))

#define StartMaskLine() \
    (mask && (maskbits = *maskp, maskcurbit = 0x80))

#define IncrementMaskBit() \
    (((maskcurbit >>= 1) == 0)				\
     && (*maskp++ = maskbits,				\
	 maskbits = *maskp,				\
	 maskcurbit = 0x80))

#define ClearMaskBit(ird, x, y) \
    (InitMask(ird, x, y), (mask && (maskbits &= ~maskcurbit,	\
				    IncrementMaskBit())))

#define SetMaskBit(ird, x, y) \
    (mask && (maskbits |= maskcurbit, IncrementMaskBit()))

#define EndMaskLine() \
    (mask && (*maskp = maskbits, maskp += maskadjust))

/*
 * Macros for manipulating pixel values.
 */

extern DitherError *image_DitherSetup(IRData *, int, int, int, int);
extern AlphaError *image_AlphaInit(IRData *, int, int, int, int, int);

#define DeclarePixelVars(ird) \
    Classjava_awt_image_IndexColorModel *cm;		\
    Classjava_awt_image_DirectColorModel *dcm;		\
    unsigned char *cmred, *cmgreen, *cmblue, *cmalpha;	\
    DitherError *ep;					\
    AlphaError *aep;					\
    ColorEntry *cp;					\
    int er, eg, eb, ea, e1, e2, e3;			\
    int grayscale = (awtImage->rOff == -1);		\
    int trans_pixel

#define PixelDecodeSetup(ird, colormodel) \
    ((obj_classblock(colormodel)					\
      == FindClass(ee, "java/awt/image/IndexColorModel", TRUE))		\
     ? (cm = (Classjava_awt_image_IndexColorModel *)unhand(colormodel),	\
	cmgreen = (unsigned char *) unhand(cm->green),			\
	cmblue = (unsigned char *) unhand(cm->blue),			\
	cmalpha = (cm->alpha						\
		   ? (unsigned char *) unhand(cm->alpha)		\
		   : 0),						\
	trans_pixel = cm->transparent_index,				\
	cmred = (unsigned char *) unhand(cm->red))			\
     : ((((obj_classblock(colormodel)					\
	   == FindClass(ee, "java/awt/image/DirectColorModel", TRUE))	\
	  && (dcm =							\
	      (Classjava_awt_image_DirectColorModel *)unhand(colormodel))\
	  && (dcm->red_bits == 8)					\
	  && (dcm->green_bits == 8)					\
	  && (dcm->blue_bits == 8)					\
	  && (dcm->alpha_bits == 8 || dcm->alpha_bits == 0))		\
	 || (dcm = 0)),							\
	cmred = 0))

#define DitherSetup(ird, x1, y1, x2, y2) \
    (ep = image_DitherSetup(ird, x1, y1, x2, y2))

#define AlphaErrorInit(ird, x, x1, y1, x2, y2, create) \
    ((aep = image_AlphaInit(ird, x1, y1, x2, y2, create))	\
     && (ea = 0, aep += (x - dstX1)))

#define PixelEncodeSetup(ird) \
    ((srcBPP == 8 && !grayscale && !ird->errors)		\
     ? ((((ird->recode != 0)					\
	  || ((ird->recode = sysMalloc(256 * sizeof(int))),	\
	      (ird->isrecoded = sysMalloc(256 * sizeof(char)))))	\
	 ? (memset(ird->isrecoded, 0, 256)) : 0),		\
	(ird->recode))						\
     : ((ird->recode != 0)					\
	? (sysFree(ird->recode),				\
	   sysFree(ird->isrecoded),				\
	   ird->recode = 0)					\
	: 0))

#define StartDitherLine(ird) \
    do {				\
	if (ep) {			\
	    ep = ird->errors;		\
	    if (dstX1) {		\
		er = ep[0].r;		\
		eg = ep[0].g;		\
		eb = ep[0].b;		\
		ep += dstX1;		\
	    } else {			\
		er = eg = eb = 0;	\
	    }				\
	}				\
	if (aep) {			\
	    aep = ird->aerrors;		\
	    if (dstX1) {		\
		ea = aep[0].a;		\
		aep += dstX1;		\
	    } else {			\
		ea = 0;			\
	    }				\
	}				\
    } while(0)

#define PixelDecode(ird, pixel) \
    (cmred							\
     ? (((unsigned int) pixel) > 255)				\
     ? (SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0), 1)\
     : (alpha = ((pixel == trans_pixel)				\
		 ? 0						\
		 : (cmalpha					\
		    ? cmalpha[pixel]				\
		    : 255)),					\
	red = cmred[pixel],					\
	green = cmgreen[pixel],					\
	blue = cmblue[pixel], 0)				\
     : (dcm							\
	? (alpha = ((dcm->alpha_bits == 0)			\
		    ? 255					\
		    : ((pixel >> dcm->alpha_offset) & 0xff)),	\
	   red = ((pixel >> dcm->red_offset) & 0xff),		\
	   green = ((pixel >> dcm->green_offset) & 0xff),	\
	   blue = ((pixel >> dcm->blue_offset) & 0xff), 0)	\
	: (rgb = do_execute_java_method(ee, (void *) colormodel,\
					"getRGB","(I)I", mb,	\
					FALSE, pixel),		\
	   alpha = (rgb >> ALPHASHIFT) & 0xff,			\
	   red = (rgb >> REDSHIFT) & 0xff,			\
	   green = (rgb >> GREENSHIFT) & 0xff,			\
	   blue = (rgb >> BLUESHIFT) & 0xff,			\
	   exceptionOccurred(ee))))

#define DitherBound(c) \
    (((c) < 0) ? 0 : (((c) > 255) ? 255 : (c)))

#define DitherMap(r, g, b) \
    (awt_RGBCube[(int)((r) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]	\
		[(int)((g) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]	\
		[(int)((b) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)])

#define DitherDist(ep, e1, e2, e3, ec, c) \
    do {			\
	e3 = (ec << 1);		\
	e1 = e3 + ec;		\
	e2 = e3 + e1;		\
	e3 += e2;		\
				\
	ep[0].c += e1 >>= 4;	\
	ep[1].c += e2 >>= 4;	\
	ep[2].c += e3 >>= 4;	\
	ec -= e1 + e2 + e3;	\
    } while (0)

#define ApplyAlpha(ird) \
    if (aep) {								\
	alpha += aep[1].a;						\
	aep[1].a = ea;							\
	if (alpha < 128) {						\
	    ClearMaskBit(ird, dx, dy);					\
	    ea = alpha;							\
	} else {							\
	    SetMaskBit(ird, dx, dy);					\
	    ea = alpha - 255;						\
	}								\
	DitherDist(aep, e1, e2, e3, ea, a);				\
	aep++;								\
    } else if (alpha == 255) {						\
	SetMaskBit(ird, dx, dy);					\
    } else {								\
	if (bgalpha != 0) {						\
	    /* Blend colors, ignore bgalpha - it's just a flag */	\
	    red = ALPHABLEND(red, alpha, bgred);			\
	    green = ALPHABLEND(green, alpha, bggreen);			\
	    blue = ALPHABLEND(blue, alpha, bgblue);			\
	    /* There should never be a mask in this case... */		\
	} else {							\
	    if (alpha == 0) {						\
		ClearMaskBit(ird, dx, dy);				\
	    } else {							\
		AlphaErrorInit(ird, dx, dstX1, dstY1, dstX2, dstY2, 1);	\
		if (alpha < 128) {					\
		    ClearMaskBit(ird, dx, dy);				\
		    ea = alpha;						\
		} else {						\
		    SetMaskBit(ird, dx, dy);				\
		    ea = alpha - 255;					\
		}							\
		if (aep) {						\
		    DitherDist(aep, e1, e2, e3, ea, a);			\
		    aep++;						\
		}							\
	    }								\
	}								\
    }

#define DitherPixel(ird, pixel, red, green, blue) \
    if (grayscale) {							\
	pixel = RGBTOGRAY(red, green, blue);				\
	if (ep) {							\
	    /* add previous errors */					\
	    pixel += ep[1].g;						\
									\
	    /* bounds checking */					\
	    e2 = DitherBound(pixel);					\
									\
	    /* Store the closest color in the destination pixel */	\
	    e2 = bwgamma[e2];						\
	    pixel = grays[e2];						\
	    cp = awt_Colors + pixel;					\
									\
	    /* Set the error from the previous lap */			\
	    ep[1].g = eg;						\
									\
	    /* compute the errors */					\
	    eg = e2 - cp->g;						\
									\
	    /* distribute the errors */					\
	    DitherDist(ep, e1, e2, e3, eg, g);				\
	    ep++;							\
	} else {							\
	    pixel = grays[bwgamma[pixel]];				\
	}								\
    } else if (ep) {							\
	/* add previous errors */					\
	red += ep[1].r;							\
	green += ep[1].g;						\
	blue += ep[1].b;						\
									\
	/* bounds checking */						\
	e1 = DitherBound(red);						\
	e2 = DitherBound(green);					\
	e3 = DitherBound(blue);						\
									\
	/* Store the closest color in the destination pixel */		\
	pixel = DitherMap(e1, e2, e3);					\
	cp = awt_Colors + pixel;					\
									\
	/* Set the error from the previous lap */			\
	ep[1].r = er; ep[1].g = eg; ep[1].b = eb;			\
									\
	/* compute the errors */					\
	er = e1 - cp->r; eg = e2 - cp->g; eb = e3 - cp->b;		\
									\
	/* distribute the errors */					\
	DitherDist(ep, e1, e2, e3, er, r);				\
	DitherDist(ep, e1, e2, e3, eg, g);				\
	DitherDist(ep, e1, e2, e3, eb, b);				\
	ep++;								\
    }

#define PixelEncode(ird, pixel, r, g, b) \
    (ird->recode					\
     ? (ird->isrecoded[pixel]				\
	? pixel = ird->recode[pixel]			\
	: (ird->isrecoded[pixel]++,			\
	   (ird->recode[pixel]				\
	       = (*awtImage->ColorMatch)(r, g, b),	\
	    pixel = ird->recode[pixel])))		\
     : ((ep == 0 && !grayscale) ? pixel = (*awtImage->ColorMatch)(r, g, b) : pixel))

extern int GenericImageConvert(struct Hjava_awt_image_ColorModel *colormodel,
			       int bgcolor, int srcX, int srcY, int srcW, int srcH,
			       void *srcpix, int srcOff, int srcBPP, int srcScan,
			       int srcTotalWidth, int srcTotalHeight,
			       int dstTotalWidth, int dstTotalHeight,
			       IRData *ird);

struct Hsun_awt_image_ImageRepresentation;
extern int awt_imageDraw(Drawable win, GC gc,
			 struct Hsun_awt_image_ImageRepresentation *irh,
			 int xormode,
			 unsigned long xorpixel, unsigned long fgpixel,
			 long x, long y,
			 struct Hjava_awt_Color *c,
			 XRectangle *clip);
