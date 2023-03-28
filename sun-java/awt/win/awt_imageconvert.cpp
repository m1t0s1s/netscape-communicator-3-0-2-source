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

#include "awt_imagescale.h"

extern "C" {
#include "java_awt_image_ColorModel.h"
#include "java_awt_image_IndexColorModel.h"
#include "java_awt_image_DirectColorModel.h"
};


long
AwtImage::GenericImageConvert(struct Hjava_awt_image_ColorModel *colormodel,
			      int32_t bgcolor, int32_t srcX, int32_t srcY,
			      int32_t srcW, int32_t srcH, void *srcpix,
			      int32_t srcOff, int32_t srcBPP, int32_t srcScan,
			      int32_t srcTotalWidth, int32_t srcTotalHeight,
			      int32_t dstTotalWidth, int32_t dstTotalHeight,
			      AwtImage *ird)
{
    pixptr srcP;
    int32_t dx, dy, sx, sy;
    int32_t dstX1, dstX2, dstY1, dstY2;
    int32_t  pixel;
    int32_t rgb;
    int bgalpha, bgred, bggreen, bgblue;
    int alpha, red, green, blue;
    int src32;
    struct methodblock *mb = 0;
    ClassClass *cb;
    struct execenv *ee;
    hash_t ID;
    int32_t len;
    long colorDepth = ird->GetDepth(colormodel);

////DeclareDstBufVars(ird);
    pixptr dstP;				\
    int dstAdjust;

////DeclareMaskVars(ird);
    MaskBits maskbits, maskcurbit;
    MaskBits *mask = ird->GetMaskBuf(FALSE);
    MaskBits *maskp;
    int32_t maskadjust;

    
    
////DeclarePixelVars(ird);
    Classjava_awt_image_IndexColorModel *cm;
    Classjava_awt_image_DirectColorModel *dcm;
    unsigned char *cmred, *cmgreen, *cmblue, *cmalpha;
    DitherError *ep, *errors;
    AlphaError *aep, *aerrors;
    RGBQUAD *cp;
    int32_t *recode = 0;
    unsigned char *isrecoded = 0;
    int er, eg, eb, ea, e1, e2, e3;
    int32_t trans_pixel;



    if (srcX < 0 || srcY < 0 || srcW < 0 || srcH < 0 || 
        (srcX + srcW > srcTotalWidth) || (srcY + srcH > srcTotalHeight))
    {
     	return SCALEFAILURE;
    }

    if (srcW == 0 || srcH == 0) {
     	return SCALENOOP;
    }

    bgalpha = (bgcolor >> ALPHASHIFT) & 0xff;
    if (bgalpha) {
        bgred   = (bgcolor >> REDSHIFT)   & 0xff;
        bggreen = (bgcolor >> GREENSHIFT) & 0xff;
        bgblue  = (bgcolor >> BLUESHIFT)  & 0xff;
    }

    switch (srcBPP) {
        case 8:  src32 = 0; break;
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

////if (!InitDstBuf(ird, dstX1, dstY1, dstX2, dstY2)) {
    ird->AllocConsumerBuffer(colormodel);
    if ((dstAdjust = ird->GetBufScan() - (dstX2 - dstX1) * colorDepth / 8L) < 0) {
        /* ERROR("OutOfMemoryError", 0); */
	return SCALEFAILURE;
    }

////SetDstBufLoc(ird, dstX1, dstY1);
    dstP.vp = ird->GetDstBuf();
    // REMIND: pointer arithmetic !!
    dstP.bp += (((ird->GetHeight() - (dstY1+1))*ird->GetBufScan())  + (dstX1) * colorDepth / 8L);
    // End of SetBufLoc(...)

////SetMaskLoc(ird, dstX1, dstY1);
    if (mask || (mask = ird->GetMaskBuf(FALSE)) != NULL) {
    // REMIND: pointer arithmetic !!
        maskp = mask + (ird->GetHeight() - (dstY1+1)) * (((ird->GetWidth()+31)&(~31L)) >> 3L)
                     + (dstX1 >> 3L);

        maskbits   = *maskp;
        maskcurbit = 0x80 >> (dstX1 & 7);
        maskadjust = (((ird->GetWidth()+31)&(~31L)) >> 3L) - ((dstX2 >> 3L) - (dstX1 >> 3L));
    }
    // End of SetMaskLoc(...)

    ee = EE();
    /* SECURITY XXX: WHAT IS THE AWT DOING RELYING ON PRIVATE DATA
     * STRUCTURES OF THE INTERPRETER?!
     *
     * The same problem occurs in
     *   sun-java/awt/x/imageconvert.c
     *   sun-java/awt/win/awt_imageconvert.cpp
     *   sun-java/awt/windows/awt_imageconvert.cpp
     *
     * Hack for 3.01: leave this in, but call getRGB as a universal method,
     * i.e. one that has only arguments and a result of local types.
     */
    cb = colormodel->methods->classdescriptor;
    ID = NameAndTypeToHash("getRGB", "(I)I", 0);
    for (len = cb->methodtable_size; --len>=0; ) {
	if ((mb = cb->methodtable->methods[len]) != 0 && mb->fb.ID == ID) {
	    break;
	}
    }
    if (len < 0) {
	/* ERROR("NoSuchMethodException", 0); */
	return SCALEFAILURE;
    }

////PixelDecodeSetup(ird, colormodel);
    // Index color model...
    if (obj_classblock(colormodel) == FindClass(ee, "java/awt/image/IndexColorModel", TRUE)) {
        cm      = (Classjava_awt_image_IndexColorModel *)unhand(colormodel);
	cmred   = (unsigned char *) unhand(cm->red);
	cmgreen = (unsigned char *) unhand(cm->green);
	cmblue  = (unsigned char *) unhand(cm->blue);
        if (cm->alpha) {
            cmalpha = (unsigned char *) unhand(cm->alpha);
        } else {
            cmalpha = NULL;
        }
	trans_pixel = cm->transparent_index;
    } 
    // Direct Color Model...
    else if (obj_classblock(colormodel) == FindClass(ee, "java/awt/image/DirectColorModel", TRUE)) {
        dcm = (Classjava_awt_image_DirectColorModel *)unhand(colormodel);
        if ( (dcm->red_bits != 8) || (dcm->green_bits != 8) || (dcm->blue_bits != 8) ||
	     (dcm->alpha_bits != 8 && dcm->alpha_bits != 0) ) {
            dcm = NULL;
        }
	cmred = NULL;
    }
    // End of PixelDecodeSetup(...)

////DitherSetup(ird, dstX1, dstY1, dstX2, dstY2);
    errors = ep = ird->GetDitherBuf(dstX1, dstY1, dstX2, dstY2);

////AlphaErrorInit(ird, dstX1, dstX1, dstY1, dstX2, dstY2, 0);
    aerrors = aep = ird->AlphaSetup(dstX1, dstY1, dstX2, dstY2, FALSE);
    if (aerrors) {
        ea = NULL;
        // REMIND: pointer arithmetic !!
        aep += dstX1 - dstX1;
    }
    // End of AlphaErrorInit(...)

////PixelEncodeSetup(ird);
    if (srcBPP == 8 && !ep) {
        recode    = ird->GetRecodeBuf(TRUE);
        isrecoded = ird->GetIsRecodedBuf();
    }
    // End of PixelEncodeSetup(...)

    for (dy = dstY1; dy < dstY2; dy++) {

////    SetDstBufLoc(ird, dstX1, dy);
        dstP.vp = ird->GetDstBuf();
        // REMIND: pointer arithmetic !!
        dstP.bp += (((ird->GetHeight() - (dy+1))*ird->GetBufScan())  + (dstX1) * colorDepth / 8L);
        // End of SetDstBufLoc(...)

////    SetMaskLoc(ird, dstX1, dy);
        if (mask || (mask = ird->GetMaskBuf(FALSE)) != NULL) {
            // REMIND: pointer arithmetic !!
            maskp = mask + (ird->GetHeight() - (dy+1)) * (((ird->GetWidth()+31)&(~31L)) >> 3L)
                         + (dstX1 >> 3L);

            maskbits   = *maskp;
            maskcurbit = 0x80 >> (dstX1 & 7);
            maskadjust = (((ird->GetWidth()+31)&(~31L)) >> 3L) - (dstX2 >> 3L) - (dstX1 >> 3L);
        }
        // End of SetMaskLoc(...)

        if (srcTotalHeight == dstTotalHeight) {
            sy = dy;
        } else {
            sy = SRC_XY(dy, srcTotalHeight, dstTotalHeight);
            srcP.vp = srcpix;
            // REMIND: pointer arithmetic !!
            if (src32) {
                srcP.ip += srcOff + (srcScan * (sy - srcY));
            } else {
                srcP.bp += srcOff + (srcScan * (sy - srcY));
            }
	}

////    StartMaskLine();
        if (mask) {
            maskbits = *maskp;
            maskcurbit = 0x80 >> (dstX1 & 7);
        }
        // End of StartMaskLine(...)

////    StartDitherLine(ird);
        if (ep) {
            ep = errors;
            if (dstX1) {
                er = ep[0].r;
                eg = ep[0].g;
                eb = ep[0].b;
                ep += dstX1;
            } else {
                er = eg = eb = 0;
            }
        }
        if (aep) {
            aep = aerrors;
             if (dstX1) {
                ea = aep[0].a;
                aep += dstX1;
            } else {
                ea = 0;
            }
        }
        // End of StartDitherLine(...)

        for (dx = dstX1; dx < dstX2; dx++) {
            //
            // Retrieve the next pixel from the buffer...
            //
            if (srcTotalWidth == dstTotalWidth) {
                sx = dx;
                // REMIND: pointer arithmetic !!
                if (src32) {
                    pixel = *srcP.ip++;
                } else {
                    pixel = *srcP.bp++;
                }
            } else {
                sx = SRC_XY(dx, srcTotalWidth, dstTotalWidth);
                // REMIND: pointer arithmetic !!
                if (src32) {
                    pixel = srcP.ip[sx];
                } else {
                    pixel = srcP.bp[sx];
                }
            }

////        if (PixelDecode(ird, pixel))  {
////            return SCALEFAILURE;
////        }

            //
            // Decode the pixel using the Index color model...
            //
            if (cmred) {
                if ((uint32_t)pixel > 255) {
                    SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
                    return SCALEFAILURE;
                } 
                
                if (pixel == trans_pixel) {
                    alpha = 0;
                } else if (cmalpha) {
                    alpha = cmalpha[pixel];
                } else {
                    alpha = 255;
                }

                red   = cmred[pixel];
                green = cmgreen[pixel];
                blue  = cmblue[pixel];
            }
            //
            // Decode the pixel using the Direct color model...
            //
            else if (dcm) {
                if (dcm->alpha_bits == 0) {
                    alpha = 255;
                } else {
                    alpha = (pixel >> dcm->alpha_offset) & 0xff;
                }

                red   = (pixel >> dcm->red_offset)   & 0xff;
                green = (pixel >> dcm->green_offset) & 0xff;
                blue  = (pixel >> dcm->blue_offset)  & 0xff;
            } 
            //
            // Ask the java ColorModel object to decode the pixel...
            //
            else {
                rgb = do_execute_java_method(ee, (void *) colormodel,
                                             "getRGB","(I)I", mb,
                                             FALSE, pixel);
                if (exceptionOccurred(ee)) {
                    return SCALEFAILURE;
                }
                alpha = (rgb >> ALPHASHIFT) & 0xff;
                red   = (rgb >> REDSHIFT)   & 0xff;
                green = (rgb >> GREENSHIFT) & 0xff;
                blue  = (rgb >> BLUESHIFT)  & 0xff;
            }
            // End of PixelDecode(...)
            
////        ApplyAlpha(ird);
            if (aep) {
                alpha += aep[1].a;
                aep[1].a = ea;
                if (alpha < 128) {
////                ClearMaskBit(ird, dx, dy);
//++                InitMask(ird, dx, dy);
                    if (mask == NULL) {
                        mask = ird->GetMaskBuf(TRUE);

//++                    SetMaskLoc(ird, dx, dy);
                        if (mask) {
                            // REMIND: pointer arithmetic !!
                            maskp = mask + (ird->GetHeight() - (dy+1)) * (((ird->GetWidth()+31)&(~31L)) >> 3L) + (dx >> 3L);
                            maskbits = *maskp;
                            maskcurbit = 0x80 >> (dx & 7);
                            maskadjust = (((ird->GetWidth()+31) & (~31L)) >> 3L) - ((dstX2 >> 3L) - (dstX1 >> 3L));
                        }
                        // End of SetMaskLoc(...)
                    }
                    // End of InitMask(...)

                    if (mask) {
                        maskbits &= ~maskcurbit;
//++                    IncrementMaskBit();
                        if ((maskcurbit >>= 1) == 0) {
                            *maskp++ = maskbits;
                            maskbits = *maskp;
                            maskcurbit = 0x80;
                        }
                        // End of IncrementMaskBit(...)
                    }
                    // End of ClearMaskBit(...)

                    ea = alpha;
                    alpha = 0;
                }
                else {
////                SetMaskBit(ird, dx, dy);
                    if(mask) {
                        maskbits |= maskcurbit;
//++                    IncrementMaskBit();
                        if ((maskcurbit >>= 1) == 0) {
                            *maskp++ = maskbits;
                            maskbits = *maskp;
                            maskcurbit = 0x80;
                        }
                        // End of IncrementMaskBit(...)
                    }
                    // End of SetMaskBit(...)
                    ea = alpha - 255;
                }
////            DitherDist(aep, e1, e2, e3, ea, a);
                e3 = (ea << 1);
                e1 = e3 + ea;
                e2 = e3 + e1;
                e3 += e2;

                aep[0].a += e1 >>= 4;
                aep[1].a += e2 >>= 4;
                aep[2].a += e3 >>= 4;
                ea -= e1 + e2 + e3;         
                // End if DitherDist(...)

                aep++;
            }
            else if (alpha == 255) {
////            SetMaskBit(ird, dx, dy);
                if(mask) {
                    maskbits |= maskcurbit;
//++                IncrementMaskBit();
                    if ((maskcurbit >>= 1) == 0) {
                        *maskp++ = maskbits;
                        maskbits = *maskp;
                        maskcurbit = 0x80;
                    }
                    // End of IncrementMaskBit(...)
                }
                // End of SetMaskBit(...)
            }
            else {
                if (bgalpha != 0) {
                    red   = ALPHABLEND(red, alpha, bgred);
                    green = ALPHABLEND(green, alpha, bggreen);
                    blue  = ALPHABLEND(blue, alpha, bgblue);
                    alpha = 255;
                } else {
                    /* There should never be a mask in this case... */
                    if (alpha == 0) {
////                    ClearMaskBit(ird, dx, dy);
//++                    InitMask(ird, dx, dy);
                        if (mask == NULL) {
                            mask = ird->GetMaskBuf(TRUE);

//++                        SetMaskLoc(ird, dx, dy);
                            if (mask) {
                                // REMIND: pointer arithmetic !!
                                maskp = mask + (ird->GetHeight() - (dy+1)) * (((ird->GetWidth()+31)&(~31L)) >> 3L) + (dx >> 3L);
                                maskbits = *maskp;
                                maskcurbit = 0x80 >> (dx & 7);
                                maskadjust = (((ird->GetWidth()+31) & (~31L)) >> 3L) - ((dstX2 >> 3L) - (dstX1 >> 3L));
                            }
                            // End of SetMaskLoc(...)
                        }
                        // End of InitMask(...)

                        if (mask) {
                            maskbits &= ~maskcurbit;
//++                        IncrementMaskBit();
                            if ((maskcurbit >>= 1) == 0) {
                                *maskp++ = maskbits;
                                maskbits = *maskp;
                                maskcurbit = 0x80;
                            }
                            // End of IncrementMaskBit(...)
                        }
                        // End of ClearMaskBit(...)

                    } else {
////                    AlphaErrorInit(ird, dx, dstX1, dstY1, dstX2, dstY2, 1);
                        aerrors = aep = ird->AlphaSetup(dstX1, dstY1, dstX2, dstY2, TRUE);
                        if (aerrors) {
                            ea = NULL;
                            // REMIND: pointer arithmetic !!
                            aep += dx - dstX1;
                        }   
                        // End of AlphaErrorInit(...)
                        
                        if (alpha < 128) {
////                        ClearMaskBit(ird, dx, dy);
//++                        InitMask(ird, dx, dy);
                            if (mask == NULL) {
                                mask = ird->GetMaskBuf(TRUE);

//++                            SetMaskLoc(ird, dx, dy);
                                if (mask) {
                                    // REMIND: pointer arithmetic !!
                                    maskp = mask + (ird->GetHeight() - (dy+1)) * (((ird->GetWidth()+31)&(~31L)) >> 3L) + (dx >> 3L);
                                    maskbits = *maskp;
                                    maskcurbit = 0x80 >> (dx & 7);
                                    maskadjust = (((ird->GetWidth()+31) & (~31L)) >> 3L) - ((dstX2 >> 3L) - (dstX1 >> 3L));
                                }
                                // End of SetMaskLoc(...)
                            }
                            // End of InitMask(...)

                            if (mask) {
                                maskbits &= ~maskcurbit;
//++                            IncrementMaskBit();
                                if ((maskcurbit >>= 1) == 0) {
                                    *maskp++ = maskbits;
                                    maskbits = *maskp;
                                    maskcurbit = 0x80;
                                }
                                // End of IncrementMaskBit(...)
                            }
                            // End of ClearMaskBit(...)

                            ea = alpha;
                            alpha = 0;
                        } else {
////                        SetMaskBit(ird, dx, dy);
                            if(mask) {
                                maskbits |= maskcurbit;
//++                            IncrementMaskBit();
                                if ((maskcurbit >>= 1) == 0) {
                                    *maskp++ = maskbits;
                                    maskbits = *maskp;
                                    maskcurbit = 0x80;
                                }
                                // End of IncrementMaskBit(...)
                            }
                            // End of SetMaskBit(...)

                            ea = alpha - 255;
                        }

                        if (aep) {
////                        DitherDist(aep, e1, e2, e3, ea, a);
                            e3 = (ea << 1);
                            e1 = e3 + ea;
                            e2 = e3 + e1;
                            e3 += e2;

                            aep[0].a += e1 >>= 4;
                            aep[1].a += e2 >>= 4;
                            aep[2].a += e3 >>= 4;
                            ea -= e1 + e2 + e3;     
                            // End if DitherDist(...)

                            aep++;
                        }
                    }
                }
            }
            // End of ApplyAlpha(...)

            //
            // If the image depth is 8-bits then dither the colors
            //
            if( colorDepth == 8) {
////            DitherPixel(ird, pixel, red, green, blue);
                if (ep) {
                    /* add previous errors */
                    red   += ep[1].r;
                    green += ep[1].g;
                    blue  += ep[1].b;

                    /* bounds checking */
                    e1 = DitherBound(red);
                    e2 = DitherBound(green);
                    e3 = DitherBound(blue);

                    /* Store the closest color in the destination pixel */
                    pixel = ird->DitherMap(e1, e2, e3);
                    cp    = ird->PixelColor(pixel);

                    /* Set the error from the previous lap */
                    ep[1].r = er; 
                    ep[1].g = eg; 
                    ep[1].b = eb;

                    /* compute the errors */
                    er = e1 - cp->rgbRed; 
                    eg = e2 - cp->rgbGreen; 
                    eb = e3 - cp->rgbBlue;

                    /* distribute the errors */
////                DitherDist(ep, e1, e2, e3, er, r);
                    e3 = (er << 1);
                    e1 = e3 + er;
                    e2 = e3 + e1;
                    e3 += e2;

                    ep[0].r += e1 >>= 4;
                    ep[1].r += e2 >>= 4;
                    ep[2].r += e3 >>= 4;
                    er -= e1 + e2 + e3;     
                    // End if DitherDist(...)

////                DitherDist(ep, e1, e2, e3, eg, g);
                    e3 = (eg << 1);
                    e1 = e3 + eg;
                    e2 = e3 + e1;
                    e3 += e2;

                    ep[0].g += e1 >>= 4;
                    ep[1].g += e2 >>= 4;
                    ep[2].g += e3 >>= 4;
                    eg -= e1 + e2 + e3;     
                    // End if DitherDist(...)

////                DitherDist(ep, e1, e2, e3, eb, b);
                    e3 = (eb << 1);
                    e1 = e3 + eb;
                    e2 = e3 + e1;
                    e3 += e2;

                    ep[0].b += e1 >>= 4;
                    ep[1].b += e2 >>= 4;
                    ep[2].b += e3 >>= 4;
                    eb -= e1 + e2 + e3;     
                    // End if DitherDist(...)
                    ep++;
                }
                // End of DitherPixel(...)
                
////            PixelEncode(ird, pixel, red, green, blue);
                if (alpha == 0) {
                    pixel = ird->DitherMap(0, 0, 0);
                } 
                else if (recode) {
                    if (!isrecoded[pixel]) {
                        isrecoded[pixel]++;
                        recode[pixel] = ird->ColorMatch(red, green, blue);
                    }
                    pixel = recode[pixel];
                } 
                else if (ep == 0) {
                    pixel = ird->ColorMatch(red, green, blue);
                }   
                // End of PixelEncode(...)
            }
////        StorePixel(ird, pixel, red, green, blue);
            if (colorDepth == 24) {
                *dstP.bp++ = (unsigned char)blue;
                *dstP.bp++ = (unsigned char)green;
                *dstP.bp++ = (unsigned char)red;
            } 
            // 8-bit color depth
            else {
                *dstP.bp++ = (unsigned char)pixel;
            }
            // End of StorePixel(...)
        }

////    EndMaskLine();
        if (mask) {
            *maskp = maskbits;
            // REMIND: pointer arithmetic !!
            maskp += maskadjust;
        }
        // End of EndMaskLine(...)
        
////    EndDstBufLine(ird);
        // REMIND: pointer arithmetic !!
        dstP.bp += dstAdjust;
        // End of EndDstBufLine(...)

	if (srcTotalHeight == dstTotalHeight) {
            // REMIND: pointer arithmetic !!
	    if (src32) {
		srcP.ip += (srcScan - srcW);
	    } else {
		srcP.bp += (srcScan - srcW);
	    }
	}
    }

    ////DstBufComplete(ird);
    if (ep && dstX1) {
        ep = errors;
        ep[0].r = er;
        ep[0].g = eg;
        ep[0].b = eb;
    }
    if (aep && dstX1) {
        aep = aerrors;
        aep[0].a = ea;
    }
    ird->BufDone(dstX1, dstY1, dstX2, dstY2);
    // End of DstBufComplete(...)

    return SCALESUCCESS;
}
