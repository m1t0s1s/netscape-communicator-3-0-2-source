/*
 * @(#)image.c	1.42 95/12/01 Jim Graham
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

#include "image.h"
#include "java_awt_image_ImageConsumer.h"
#include "java_awt_image_ImageObserver.h"
#include "java_awt_image_ColorModel.h"
#include "java_awt_image_IndexColorModel.h"
#include "java_awt_Color.h"
#include "java_awt_Graphics.h"
#include "java_awt_Rectangle.h"
#include "sun_awt_image_OffScreenImageSource.h"

typedef unsigned long Pixel;

typedef struct Hjava_awt_image_ColorModel *CMHandle;
typedef Classjava_awt_image_ColorModel *CMPtr;
typedef struct Hsun_awt_image_Image *IHandle;
typedef Classsun_awt_image_Image *IPtr;
typedef struct Hsun_awt_image_ImageRepresentation *IRHandle;
typedef Classsun_awt_image_ImageRepresentation *IRPtr;
typedef struct Hsun_awt_image_OffScreenImageSource *OSISHandle;
typedef Classsun_awt_image_OffScreenImageSource *OSISPtr;
typedef struct Hjava_awt_image_ImageConsumer *ICHandle;
typedef struct Hjava_awt_Graphics *GHandle;
typedef Classjava_awt_Graphics *GPtr;
typedef struct HArrayOfByte *ByteHandle;
typedef unsigned char *BytePtr;
typedef struct HArrayOfInt *IntHandle;
typedef unsigned int *IntPtr;

#define IMAGE_SIZEINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_DRAWINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_SOMEBITS |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_FULLDRAWINFO (java_awt_image_ImageObserver_FRAMEBITS |	\
			    java_awt_image_ImageObserver_ALLBITS)

#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)

#define HINTS_DITHERFLAGS (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT)
#define HINTS_SCANLINEFLAGS (java_awt_image_ImageConsumer_COMPLETESCANLINES)

#define HINTS_OFFSCREENSEND (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |  \
			     java_awt_image_ImageConsumer_COMPLETESCANLINES | \
			     java_awt_image_ImageConsumer_SINGLEPASS)

Pixel awt_white;

GC
awt_getImageGC(Pixmap pixmap) {
    static GC awt_imagegc;

    if (!awt_imagegc) {
	awt_white = awtImage->ColorMatch(255, 255, 255);
	awt_imagegc = XCreateGC(awt_display, pixmap, 0, NULL);
	XSetForeground(awt_display, awt_imagegc, awt_white);
    }
    return awt_imagegc;
}

IRData *
image_getIRData(IRHandle irh)
{
    IRPtr ir = unhand(irh);
    IRData *ird = (IRData *) ir->pData;

    if (ird == 0) {
	if ((ir->availinfo & IMAGE_SIZEINFO) != IMAGE_SIZEINFO) {
	    return ird;
	}
	ird = (IRData *) sysMalloc(sizeof(IRData));
	if (ird == 0) {
	    return ird;
	}
	memset(ird, 0, sizeof(IRData));
	ird->hJavaObject = irh;
	ird->pixmap = XCreatePixmap(awt_display,
				    awt_root,
				    ir->width, ir->height,
				    awtImage->Depth);
	XFillRectangle(awt_display, ird->pixmap, awt_getImageGC(ird->pixmap),
		       0, 0, ir->width, ir->height);
	ird->depth = awtImage->awtBitsPerPixel;
	ird->dstW = ir->width;
	ird->dstH = ir->height;
	ird->srcW = ir->srcW;
	ird->srcH = ir->srcH;
	ird->hints = ir->hints;
	ir->pData = (int) ird;
    } else if (ird->hints == 0) {
	ird->hints = ir->hints;
    }

    return ird;
}

Drawable
image_getIRDrawable(Hsun_awt_image_ImageRepresentation *ir) {
    IRData *ird;
    if (ir == 0) {
	return 0;
    }
    ird = image_getIRData(ir);
    return (ird == 0) ? 0 : ird->pixmap;
}

void
image_FreeBufs(IRData *ird)
{
    if (ird->buffer) {
	sysFree(ird->buffer);
	ird->buffer = 0;
    }
    if (ird->xim) {
	XFree(ird->xim);
	ird->xim = 0;
    }
    if (ird->maskbuf) {
	sysFree(ird->maskbuf);
	ird->maskbuf = 0;
    }
    if (ird->maskim) {
	XFree(ird->maskim);
	ird->maskim = 0;
    }
    ird->bufwidth = ird->bufheight = 0;
}

void
image_FreeRenderData(IRData *ird)
{
    if (ird->errors) {
	sysFree(ird->errors);
	ird->errors = 0;
    }
    if (ird->recode) {
	sysFree(ird->recode);
	ird->recode = 0;
    }
    if (ird->isrecoded) {
	sysFree(ird->isrecoded);
	ird->isrecoded = 0;
    }
    if (ird->curpixels) {
	XDestroyRegion(ird->curpixels);
	ird->curpixels = 0;
    }
    if (ird->curlines.seen) {
	sysFree(ird->curlines.seen);
	ird->curlines.seen = 0;
    }
    image_FreeBufs(ird);
}

MaskBits *
image_InitMask(IRData *ird)
{
    MaskBits *mask;
    int scansize = (ird->bufwidth + 7) >> 3;
    Region pixrgn;

    mask = ird->maskbuf = (void *) sysMalloc(scansize * ird->bufheight + 1);
    if (mask != 0) {
	ird->maskim = XCreateImage(awt_display, awt_visual, 1,
				   XYBitmap, 0, ird->maskbuf,
				   ird->bufwidth, ird->bufheight,
				   8, scansize);
	if (ird->maskim == 0) {
	    sysFree(ird->maskbuf);
	    ird->maskbuf = 0;
	} else {
	    ird->maskim->bitmap_bit_order = MSBFirst;
	    ird->maskim->bitmap_unit = 8;
	    memset(mask, 0xff, scansize * ird->bufheight);
	}
    }
    if (!ird->mask) {
	ird->mask = XCreatePixmap(awt_display, awt_root,
				  ird->dstW, ird->dstH, 1);
	if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
	    XFillRectangle(awt_display, ird->mask, awt_maskgc,
			   0, 0, ird->dstW, ird->dstH);
	} else {
	    XSetForeground(awt_display, awt_maskgc, 0);
	    XFillRectangle(awt_display, ird->mask, awt_maskgc,
			   0, 0, ird->dstW, ird->dstH);
	    XSetForeground(awt_display, awt_maskgc, 1);
	    pixrgn = ird->curpixels;
	    if (pixrgn != 0) {
		XSetRegion(awt_display, awt_maskgc, pixrgn);
		XFillRectangle(awt_display, ird->mask, awt_maskgc,
			       0, 0, ird->dstW, ird->dstH);
		XSetClipMask(awt_display, awt_maskgc, None);
	    }
	}
    }

    return mask;
}

int
image_BufAlloc(IRData *ird, int x1, int y1, int x2, int y2)
{
    int w = x2 - x1;
    int h = y2 - y1;
    int bpp, slp, bpsl;
    MaskBits *mask = ird->maskbuf;

    if (w >= 0 && h >= 0 && (ird->bufwidth < w || ird->bufheight < h)) {
	image_FreeBufs(ird);
	bpp = (ird->depth > 8) ? 32 : 8;
	slp = awtImage->wsImageFormat.scanline_pad;
	bpsl = ((w * bpp + slp - 1) & (~ (slp - 1))) >> 3;
	ird->bufwidth = w;
	ird->bufheight = h;
	ird->buffer = (void *) sysMalloc(h * bpsl);
	if (ird->buffer != 0) {
	    ird->xim = XCreateImage(awt_display,
				    awt_visual,
				    awtImage->Depth, ZPixmap, 0,
				    (u_char *) ird->buffer,
				    w, h, slp, bpsl);
	    if (ird->xim != 0 && mask) {
		image_InitMask(ird);
	    }
	}
	if (ird->buffer == 0 || ird->xim == 0 || ((mask != 0) &&
						  ((ird->maskbuf == 0) ||
						   (ird->maskim == 0))))
	{
	    image_FreeBufs(ird);
	    ird->bufwidth = ird->bufheight = 0;
	    return 0;
	}
	ird->xim->bits_per_pixel = bpp;
	ird->bufscan = bpsl;
    }

    return 1;
}

DitherError *
image_DitherSetup(IRData *ird, int x1, int y1, int x2, int y2)
{
    int size;

    if (ird->depth <= 8 && (ird->hints & HINTS_DITHERFLAGS) != 0) {
	if (ird->errors == 0) {
	    size = sizeof(DitherError) * (ird->dstW + 2);
	    ird->errors = sysMalloc(size);
	    if (ird->errors) {
		memset(ird->errors, 0, size);
	    }
	}
    }
    return ird->errors;
}

AlphaError *
image_AlphaInit(IRData *ird, int x1, int y1, int x2, int y2, int create)
{
    int size;

    if ((ird->hints & HINTS_DITHERFLAGS) != 0) {
	if (create && ird->aerrors == 0) {
	    size = sizeof(AlphaError) * (ird->dstW + 2);
	    ird->aerrors = sysMalloc(size);
	    if (ird->aerrors) {
		memset(ird->aerrors, 0, size);
	    }
	}
    }
    return ird->aerrors;
}

int
image_Done(IRData *ird, int x1, int y1, int x2, int y2)
{
    int bpp;
    int w = x2 - x1;
    int h = y2 - y1;
    int ytop = y1;
    GC imagegc;
    Hjava_awt_Rectangle *newbits;

    if (ird->pixmap == None || ird->xim == 0) {
	return 0;
    }
    bpp = awtImage->wsImageFormat.bits_per_pixel;
    imagegc = awt_getImageGC(ird->pixmap);
    if (ird->xim->bits_per_pixel == bpp) {
	XPutImage(awt_display, ird->pixmap, imagegc,
		  ird->xim, 0, 0, x1, y1, w, h);
    } else {
	u_long *buf2;
	XImage *xim2;
	int slp = awtImage->wsImageFormat.scanline_pad;
	int i, j;

	buf2 = (u_long *) sysMalloc((bpp * w + slp - 1) / 8 * h);
	if (buf2 == (u_long *) 0) {
	    return 0;
	}

	xim2 = XCreateImage(awt_display, awt_visual,
			    awtImage->Depth, ZPixmap, 0,
			    (u_char *) buf2,
			    w, h, slp, 0);

	if (ird->xim->bits_per_pixel == 8) {
	    u_char *p = (u_char *) ird->buffer;
	    for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
		    XPutPixel(xim2, i, j, *p++);
		}
	    }
	} else {
	    u_long *p = (u_long *) ird->buffer;
	    for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
		    XPutPixel(xim2, i, j, *p++);
		}
	    }
	}
	XPutImage(awt_display, ird->pixmap, imagegc,
		  xim2, 0, 0, x1, y1, w, h);
	sysFree(buf2);
	XFree(xim2);
    }
    if (ird->mask) {
	XPutImage(awt_display, ird->mask, awt_maskgc,
		  ird->maskim, 0, 0, x1, y1, w, h);
	/* We just invalidated any background color preparation... */
	ird->bgcolor = 0;
    }
    if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
	char *seen = ird->curlines.seen;
	int num, l;
	if (seen == 0) {
	    seen = sysMalloc(ird->dstH);
	    memset(seen, 0, ird->dstH);
	    ird->curlines.seen = seen;
	    num = 0;
	} else {
	    num = ird->curlines.num;
	}
	for (l = y1 - 1; l >= 0; l--) {
	    if (seen[l]) {
		break;
	    }
	    XCopyArea(awt_display, ird->pixmap, ird->pixmap, imagegc,
		      x1, y1, w, 1, x1, l);
	    if (ird->mask) {
		XCopyArea(awt_display, ird->mask, ird->mask, awt_maskgc,
			  x1, y1, w, 1, x1, l);
	    }
	    ytop = l;
	}
	for (l = y1; l < y2; l++) {
	    seen[l] = 1;
	}
	if (num < y2) {
	    num = y2;
	}
	ird->curlines.num = num;
    } else {
	XRectangle rect;
	Region curpixels = ird->curpixels;

	rect.x = x1;
	rect.y = y1;
	rect.width = w;
	rect.height = h;

	if (curpixels == 0) {
	    curpixels = XCreateRegion();
	    ird->curpixels = curpixels;
	}
	XUnionRectWithRegion(&rect, curpixels, curpixels);
    }
    newbits = unhand(ird->hJavaObject)->newbits;
    if (newbits != 0) {
	Classjava_awt_Rectangle *pNB = unhand(newbits);
	pNB->x = x1;
	pNB->y = ytop;
	pNB->width = w;
	pNB->height = y2 - ytop;
    }
    return 1;
}

void
sun_awt_image_ImageRepresentation_offscreenInit(IRHandle irh)
{
    IRPtr ir;
    IRData *ird;

    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    ir = unhand(irh);
    if (ir->width <= 0 || ir->height <= 0) {
	SignalError(0, JAVAPKG "IllegalArgumentException", 0);
	AWT_UNLOCK();
	return;
    }
    ir->availinfo = IMAGE_OFFSCRNINFO;
    ird = image_getIRData(irh);
    if (ird == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    AWT_UNLOCK();
}

long
sun_awt_image_ImageRepresentation_setBytePixels(IRHandle irh,
						long x, long y,
						long w, long h,
						CMHandle cmh, ByteHandle arrh,
						long off, long scansize)
{
    IRPtr ir;
    IRData *ird;
    int ret;

    if (irh == 0 || cmh == 0 || arrh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    if (w <= 0 || h <= 0) {
	return -1;
    }
    if (obj_length(arrh) < (scansize * h)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    AWT_LOCK();
    ir = unhand(irh);
    ird = image_getIRData(irh);
    if (ird == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return -1;
    }
    ret = GenericImageConvert(cmh, 0, x, y, w, h,
			      unhand(arrh), off, 8, scansize,
			      ir->srcW, ir->srcH,
			      ird->dstW, ird->dstH, ird);
    AWT_UNLOCK();
    return (ret == SCALESUCCESS);
}

long
sun_awt_image_ImageRepresentation_setIntPixels(IRHandle irh,
					       long x, long y, long w, long h,
					       CMHandle cmh, IntHandle arrh,
					       long off, long scansize)
{
    IRPtr ir;
    IRData *ird;
    int ret;

    if (irh == 0 || cmh == 0 || arrh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    if (w <= 0 || h <= 0) {
	return -1;
    }
    if (obj_length(arrh) < (scansize * h)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    AWT_LOCK();
    ir = unhand(irh);
    ird = image_getIRData(irh);
    if (ird == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return -1;
    }
    ret = GenericImageConvert(cmh, 0, x, y, w, h,
			      unhand(arrh), off, 32, scansize,
			      ir->srcW, ir->srcH,
			      ird->dstW, ird->dstH, ird);
    AWT_UNLOCK();
    return (ret == SCALESUCCESS);
}

long
sun_awt_image_ImageRepresentation_finish(IRHandle irh, long force)
{
    IRData *ird;
    int ret = 0;

    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    AWT_LOCK();
    ird = image_getIRData(irh);
    if (ird != 0) {
	image_FreeRenderData(ird);
	ret = (!force
	       && ird->depth <= 8
	       && (ird->hints & HINTS_DITHERFLAGS) == 0);
	ird->hints = 0;
    }
    AWT_UNLOCK();
    return ret;
}

#define INIT_GC(disp, gdata)\
if (gdata==0 || gdata->gc == 0 && !awt_init_gc(disp,gdata)) {\
    AWT_UNLOCK();\
    return;\
}

int
awt_imageDraw(Drawable win, GC gc, IRHandle irh,
	      int xormode, unsigned long xorpixel, unsigned long fgpixel,
	      long x, long y, struct Hjava_awt_Color *c, XRectangle *clip)
{
    IRPtr		ir;
    IRData		*ird;
    long		wx;
    long		wy;
    long		ix;
    long		iy;
    long		iw;
    long		ih;
    long		diff;
    Region		pixrgn = 0;

    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    ir = unhand(irh);
    if ((ir->availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
	return 1;
    }
    ird = image_getIRData(irh);
    if (ird == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    } else {
	if (ird->pixmap == None) {
	    return 1;
	}
	if (win == 0) {
	    SignalError(0, JAVAPKG "NullPointerException", "win");
	    return 0;
	}

	if ((ir->availinfo & IMAGE_FULLDRAWINFO) != 0) {
	    ix = iy = 0;
	    iw = ir->width;
	    ih = ir->height;
	} else if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
	    ix = iy = 0;
	    iw = ir->width;
	    ih = ird->curlines.num;
	} else {
	    XRectangle rect;
	    pixrgn = ird->curpixels;
	    if (!pixrgn) {
		return 1;
	    }
	    XClipBox(pixrgn, &rect);
	    ix = rect.x;
	    iy = rect.y;
	    iw = rect.width;
	    ih = rect.height;
	}
	wx = x + ix;
	wy = y + iy;

	if (clip) {
	    diff = (clip->x - wx);
	    if (diff > 0) {
		iw -= diff;
		if (iw <= 0) {
		    return 1;
		}
		ix += diff;
		wx = clip->x;
	    }
	    diff = (clip->y - wy);
	    if (diff > 0) {
		ih -= diff;
		if (ih <= 0) {
		    return 1;
		}
		iy += diff;
		wy = clip->y;
	    }
	    diff = clip->x + clip->width - wx;
	    if (iw > diff) {
		iw = diff;
		if (iw <= 0) {
		    return 1;
		}
	    }
	    diff = clip->y + clip->height - wy;
	    if (ih > diff) {
		ih = diff;
		if (ih <= 0) {
		    return 1;
		}
	    }
	}

	if (ird->mask) {
	    if (c == 0) {
		XSetClipMask(awt_display, gc, ird->mask);
	    } else {
		Pixel bgcolor;
		if (xormode) {
		    bgcolor = xorpixel;
		} else {
		    bgcolor = awt_getColor(c);
		}
		if (ird->bgcolor != bgcolor + 1) {
		    /* Set the "transparent pixels" in the pixmap to
		     * the specified solid background color and just
		     * do an unmasked copy into place below.
		     */
		    GC imagegc = awt_getImageGC(ird->pixmap);
		    XSetFunction(awt_display, awt_maskgc, GXxor);
		    XFillRectangle(awt_display, ird->mask, awt_maskgc,
				   0, 0, ird->dstW, ird->dstH);
		    XSetClipMask(awt_display, imagegc, ird->mask);
		    XSetForeground(awt_display, imagegc, bgcolor);
		    XFillRectangle(awt_display, ird->pixmap, imagegc,
				   0, 0, ird->dstW, ird->dstH);
		    XSetForeground(awt_display, imagegc, awt_white);
		    XSetClipMask(awt_display, imagegc, None);
		    XFillRectangle(awt_display, ird->mask, awt_maskgc,
				   0, 0, ird->dstW, ird->dstH);
		    XSetFunction(awt_display, awt_maskgc, GXcopy);
		    ird->bgcolor = bgcolor + 1;
		}
	    }
	} else if (pixrgn) {
	    XSetRegion(awt_display, gc, pixrgn);
	}
	if ((ird->mask && c == 0) || pixrgn) {
	    XSetClipOrigin(awt_display, gc, x, y);
	}
	if (xormode) {
	    XSetForeground(awt_display, gc, xorpixel);
	    XFillRectangle(awt_display, win, gc, wx, wy, iw, ih);
	}
	XCopyArea(awt_display,
		  ird->pixmap, win, gc,
		  ix, iy, iw, ih, wx, wy);
	if (xormode) {
	    XSetForeground(awt_display, gc, fgpixel ^ xorpixel);
	}
	if ((ird->mask && c == 0) || pixrgn) {
	    if (clip) {
		XSetClipRectangles(awt_display, gc, 0, 0, clip, 1, YXBanded);
	    } else {
		XSetClipMask(awt_display, gc, None);
	    }
	}
    }
    return 1;
}

void
sun_awt_image_ImageRepresentation_disposeImage(IRHandle irh)
{
    IRPtr ir;
    IRData *ird;

    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    ir = unhand(irh);
    ird = (IRData *) ir->pData;
    if (ird != 0) {
	if (ird->pixmap != None) {
	    XFreePixmap(awt_display, ird->pixmap);
	}
	if (ird->mask != None) {
	    XFreePixmap(awt_display, ird->mask);
	}
	image_FreeRenderData(ird);
	sysFree(ird);
    }
    ir->pData = 0;
    AWT_UNLOCK();
}

#define SET_BYTE_PIXELS_SIG	"(IIIILjava/awt/image/ColorModel;[BII)V"
#define SET_INT_PIXELS_SIG	"(IIIILjava/awt/image/ColorModel;[III)V"

void
sun_awt_image_OffScreenImageSource_sendPixels(OSISHandle osish)
{
    OSISPtr osis = unhand(osish);
    IRHandle irh = osis->baseIR;
    IRPtr ir;
    IRData *ird;
    struct execenv *ee;
    extern struct Hjava_awt_image_ColorModel *awt_getColorModel(void);
    struct Hjava_awt_image_ColorModel *cm;
    HObject *pixh;
    char *buf;
    int i, w, h, d;
    XImage *xim;
    XID pixmap;

    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    ir = unhand(irh);
    if ((ir->availinfo & IMAGE_OFFSCRNINFO) != IMAGE_OFFSCRNINFO) {
	SignalError(0, JAVAPKG "IllegalAccessError", 0);
	return;
    }
    AWT_LOCK();
    ird = image_getIRData(irh);
    if (ird == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    w = ird->dstW;
    h = ird->dstH;
    d = ird->depth;
    pixmap = ird->pixmap;
    AWT_UNLOCK();
    ee = EE();
    if (osis->theConsumer == 0) {
	return;
    }
    cm = awt_getColorModel();
    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				"setColorModel",
				"(Ljava/awt/image/ColorModel;)V",
				cm);
    if (osis->theConsumer == 0) {
	return;
    }
    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				"setHints",
				"(I)V",
				HINTS_OFFSCREENSEND);
    if (osis->theConsumer == 0) {
	return;
    }
    AWT_LOCK();
    if (d > 8) {
	pixh = (HObject *) ArrayAlloc(T_INT, w);
	buf = (char *) unhand(pixh);
	xim = XCreateImage(awt_display, awt_visual,
			   awtImage->Depth, ZPixmap, 0, buf,
			   w, 1, 32, 0);
	xim->bits_per_pixel = 32;
    } else {
	/* XGetSubImage takes care of expanding to 8 bit if necessary. */
	/* But, that is not a supported feature.  There are better ways */
	/* of handling this... */
	pixh = (HObject *) ArrayAlloc(T_BYTE, w);
	buf = (char *) unhand(pixh);
	xim = XCreateImage(awt_display, awt_visual,
			   awtImage->Depth, ZPixmap, 0, buf,
			   w, 1, 8, 0);
	xim->bits_per_pixel = 8;
    }
    AWT_UNLOCK();
    for (i = 0; i < h; i++) {
	if (osis->theConsumer == 0) {
	    break;
	}
	AWT_LOCK();
	XGetSubImage(awt_display, pixmap,
		     0, i, w, 1, ~0,
		     ZPixmap, xim, 0, 0);
	AWT_UNLOCK();
	if (d > 8) {
	    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
					"setPixels",
					SET_INT_PIXELS_SIG,
					0, i, w, 1,
					cm, pixh, 0, w);
	} else {
	    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
					"setPixels",
					SET_BYTE_PIXELS_SIG,
					0, i, w, 1,
					cm, pixh, 0, w);
	}
	if (exceptionOccurred(ee)) {
	    break;
	}
    }
    AWT_LOCK();
    XFree(xim);
    AWT_UNLOCK();
    if (buf == 0) {
	/* This can never happen, but we need to make a reference to buf
	 * at the bottom of this function so the buffer doesn't move
	 * while we are using it.
	 */
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
}
