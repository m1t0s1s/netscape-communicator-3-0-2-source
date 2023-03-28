/*
 * @(#)awt_imageconvert.cpp	1.5 95/11/30 Jim Graham
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

#include "stdafx.h"
#include "awt_imagescale.h"
#include "java_awt_image_ColorModel.h"
#include "java_awt_image_IndexColorModel.h"
#include "java_awt_image_DirectColorModel.h"

long
AwtImage::GenericImageConvert(struct Hjava_awt_image_ColorModel *colormodel,
			      int bgcolor, int srcX, int srcY,
			      int srcW, int srcH, void *srcpix,
			      int srcOff, int srcBPP, int srcScan,
			      int srcTotalWidth, int srcTotalHeight,
			      int dstTotalWidth, int dstTotalHeight,
			      AwtImage *ird)
{
    pixptr srcP;
    int dx, dy, sx, sy;
    int dstX1, dstX2, dstY1, dstY2;
    int src32, pixel;
    int bgalpha, bgred, bggreen, bgblue;
    int rgb, alpha, red, green, blue;
    struct methodblock *mb = 0;
    ClassClass *cb;
    struct execenv *ee;
    unsigned int ID;
    int len;
    DeclareDstBufVars(ird);
    DeclareMaskVars(ird);
    DeclarePixelVars(ird);

    if (srcX < 0 || srcY < 0 || srcW < 0 || srcH < 0
	|| srcX + srcW > srcTotalWidth || srcY + srcH > srcTotalHeight)
    {
	return SCALEFAILURE;
    }

    if (srcW == 0 || srcH == 0) {
	return SCALENOOP;
    }

    bgalpha = (bgcolor >> ALPHASHIFT) & 0xff;
    if (bgalpha) {
	bgred = (bgcolor >> REDSHIFT) & 0xff;
	bggreen = (bgcolor >> GREENSHIFT) & 0xff;
	bgblue = (bgcolor >> BLUESHIFT) & 0xff;
    }

    switch (srcBPP) {
    case 8: src32 = 0; break;
    case 32: src32 = 1; break;
    default: return SCALEFAILURE;
    }

    if (srcTotalWidth == dstTotalWidth) {
	dstX1 = srcX;
	dstX2 = srcX + srcW;
    } else {
	dstX1 = DEST_XY_RANGE_START(srcX, srcTotalWidth, dstTotalWidth);
	dstX2 = DEST_XY_RANGE_START(srcX+srcW, srcTotalWidth, dstTotalWidth);
	if (dstX2 <= dstX1) {
	    return SCALENOOP;
	}
    }

    if (srcTotalHeight == dstTotalHeight) {
	dstY1 = srcY;
	dstY2 = srcY + srcH;
	srcP.vp = srcpix;
	if (src32) {
	    srcP.ip += srcOff;
	} else {
	    srcP.bp += srcOff;
	}
    } else {
	dstY1 = DEST_XY_RANGE_START(srcY, srcTotalHeight, dstTotalHeight);
	dstY2 = DEST_XY_RANGE_START(srcY+srcH, srcTotalHeight, dstTotalHeight);
	if (dstY2 <= dstY1) {
	    return SCALENOOP;
	}
    }

    if (!InitDstBuf(ird, dstX1, dstY1, dstX2, dstY2)) {
	/* ERROR("OutOfMemoryError", 0); */
	return SCALEFAILURE;
    }
    SetDstBufLoc(ird, dstX1, dstY1);

    SetMaskLoc(ird, dstX1, dstY1);

    ee = EE();
    cb = colormodel->methods->classdescriptor;
    ID = NameAndTypeToHash("getRGB", "(I)I");
    for (len = cb->methodtable_size; --len>=0; ) {
	if ((mb = cb->methodtable->methods[len]) != 0 && mb->fb.ID == ID) {
	    break;
	}
    }
    if (len < 0) {
	/* ERROR("NoSuchMethodException", 0); */
    AWT_TRACE(("no getRGB(I)I\n"));
	return SCALEFAILURE;
    }

    PixelDecodeSetup(ird, colormodel);
    DitherSetup(ird, dstX1, dstY1, dstX2, dstY2);
    AlphaErrorInit(ird, dstX1, dstX1, dstY1, dstX2, dstY2, 0);
    PixelEncodeSetup(ird);

    for (dy = dstY1; dy < dstY2; dy++) {
	if (srcTotalHeight == dstTotalHeight) {
	    sy = dy;
	} else {
	    sy = SRC_XY(dy, srcTotalHeight, dstTotalHeight);
	    srcP.vp = srcpix;
	    if (src32) {
		srcP.ip += srcOff + (srcScan * (sy - srcY));
	    } else {
		srcP.bp += srcOff + (srcScan * (sy - srcY));
	    }
	}
	StartMaskLine();
	StartDitherLine(ird);
	for (dx = dstX1; dx < dstX2; dx++) {
	    if (srcTotalWidth == dstTotalWidth) {
		sx = dx;
		if (src32) {
		    pixel = *srcP.ip++;
		} else {
		    pixel = *srcP.bp++;
		}
	    } else {
		sx = SRC_XY(dx, srcTotalWidth, dstTotalWidth);
		if (src32) {
		    pixel = srcP.ip[sx];
		} else {
		    pixel = srcP.bp[sx];
		}
	    }
	    if (PixelDecode(ird, pixel)) {
		return SCALEFAILURE;
	    }
	    ApplyAlpha(ird);
	    DitherPixel(ird, pixel, red, green, blue);
	    PixelEncode(ird, pixel, red, green, blue);
	    StorePixel(ird, pixel, dx, dy);
	}
	EndMaskLine();
	EndDstBufLine(ird);
	if (srcTotalHeight == dstTotalHeight) {
	    if (src32) {
		srcP.ip += (srcScan - srcW);
	    } else {
		srcP.bp += (srcScan - srcW);
	    }
	}
    }
    DstBufComplete(ird);
    return SCALESUCCESS;
}
