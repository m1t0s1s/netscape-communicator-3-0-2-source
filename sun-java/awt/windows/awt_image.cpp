/*
 * @(#)awt_image.cpp	1.39 96/01/05 Patrick P. Chan
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


NOTE: ALL THE DITHERING CODE WAS QUICKLY SLAPPED TOGETHER.  IT NEEDS
A LOT OF CLEAN-UP WORK AND TESTING. 


*/
#include "stdafx.h"
#include "awt.h"
#include "awtapp.h"
#include "awt_image.h"
#include "awt_graphics.h"
#include "math.h"
#include "malloc.h"
#include "awt_imagescale.h"

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

#include "java_awt_image_ImageConsumer.h"

#define HINTS_DITHERFLAGS (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT)
#define HINTS_SCANLINEFLAGS (java_awt_image_ImageConsumer_COMPLETESCANLINES)

const int COLOR4 = 0;
const int COLOR8 = 1;

ColorInfo g_colorInfo[2];

/* efficiently map an RGB value to a pixel value */
#define RGBMAP(pColorInfo, r, g, b)	 \
	((pColorInfo)->m_rgbCube[(int)((r) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]\
	 	[(int)((g) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]\
	 	[(int)((b) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)])

#define COMPOSE24(I, r, g, b)		\
	((((b) & 0xff) << ((I)->bOff)) |	\
	 (((g) & 0xff) << ((I)->gOff)) |	\
	 (((r) & 0xff) << ((I)->rOff)))

// Initialize static variables
HPALETTE AwtImage::m_hPal = NULL;
int AwtImage::m_bitspixel = 0;

// This is data structure holds the common colormap that is used to paint
// all images.  When an image is painted, the BITMAPINFOHEADER of 
// the image is copied into gpBitmapInfo and then gpBitmapInfo is used to
// paint the image.  This strategy eliminates an alloc/free when painting
// an image.
BITMAPINFO *gpBitmapInfo = NULL;
BITMAPINFO *gpTransMaskInfo = NULL;

AwtImage::AwtImage()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtImage");

	m_nSrcW = 0;
	m_nSrcH = 0;
	m_nDstW = 0;
	m_nDstH = 0;
	m_pBitmapData = NULL;
	m_hBitmap = NULL;
	m_bHasTransparentPixels = FALSE;
	m_hTransparencyMask = NULL;
	m_pTransparencyData = NULL;
	m_bgcolor = 0;
	m_pRecode = NULL;
	m_pIsRecoded = NULL;
	m_pDitherErrors = NULL;
	m_pAlphaErrors = NULL;
	m_hIcon = NULL;
	m_pSharedDC = NULL;
	m_nLastLine = 0;
	m_pSeen = NULL;
	m_hRgn = NULL;
}

AwtImage::~AwtImage()
{
	if (m_hBitmap != NULL) {
		VERIFY(::DeleteObject(m_hBitmap));
	}
	if (m_hTransparencyMask != NULL) {
		VERIFY(::DeleteObject(m_hTransparencyMask));
	}
	if (m_pSharedDC != NULL) {
	    m_pSharedDC->Detach(NULL);
	    m_pSharedDC = NULL;
	}
	if (m_pDitherErrors) {
	    delete []m_pDitherErrors;
	}
	if (m_pAlphaErrors) {
	    delete []m_pAlphaErrors;
	}
	if (m_pRecode) {
	    delete []m_pRecode;
	}
	if (m_pIsRecoded) {
	    delete []m_pIsRecoded;
	}
	if (m_pSeen) {
	    delete []m_pSeen;
	}
	if (m_hRgn) {
	    VERIFY(::DeleteObject(m_hRgn));
	}
	// m_pBitmapData is deleted when m_hBitmap is deleted
	// m_pTransparencyData is deleted when m_hTransparencyMask is deleted
 
    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

int AwtImage::BufInit(int x1, int y1, int x2, int y2)
{
    if (m_pBitmapData == NULL) {
	m_nBufScan = (m_nDstW + 3) & (~3);
	gpBitmapInfo->bmiHeader.biWidth = m_nBufScan;
	gpBitmapInfo->bmiHeader.biHeight = -m_nDstH;
	HDC hDC = ::CreateDC("DISPLAY", NULL, NULL, NULL);
	m_hBitmap = ::CreateDIBSection(hDC, gpBitmapInfo, DIB_RGB_COLORS,
				       &m_pBitmapData, NULL, 0);
	::DeleteDC(hDC);
    }
    return (int) m_pBitmapData;
}

MaskBits *AwtImage::GetMaskBuf(BOOL create) {
    if (m_pTransparencyData == NULL && create) {
	int nPaddedWidth = (m_nDstW + 31) & (~31);
	gpTransMaskInfo->bmiHeader.biWidth = nPaddedWidth;
	gpTransMaskInfo->bmiHeader.biHeight = -m_nDstH;
	HDC hDC = ::CreateDC("DISPLAY", NULL, NULL, NULL);
	m_hTransparencyMask = ::CreateDIBSection(hDC, gpTransMaskInfo,
						 DIB_RGB_COLORS,
						 &m_pTransparencyData,
						 NULL, 0);
	if (m_pTransparencyData != NULL) {
	    if ((m_nHints & HINTS_SCANLINEFLAGS) != 0) {
		memset(m_pTransparencyData, 0xff, (nPaddedWidth*m_nDstH)>>3);
	    } else {
		memset(m_pTransparencyData, 0x0, (nPaddedWidth*m_nDstH)>>3);
		if (m_hRgn != NULL) {
		    HDC hMemDC = ::CreateCompatibleDC(hDC);
		    ::SelectObject(hMemDC, m_hTransparencyMask);
		    ::SelectClipRgn(hMemDC, m_hRgn);
		    HBRUSH hBrush = (HBRUSH)::GetStockObject( BLACK_BRUSH );
		    ::FillRect(hMemDC, CRect(0, 0, m_nDstW, m_nDstH), hBrush);
		    VERIFY(::DeleteDC(hMemDC));
            // The HBRUSH is a stock object, so dont delete it
		}
	    }
	}
        VERIFY(::DeleteDC(hDC));
    }
    // Getting the mask data invalidates the values of the transparent pixels
    m_bgcolor = 0;
    return (MaskBits *) m_pTransparencyData;
}

DitherError *AwtImage::GetDitherBuf(int x1, int y1, int x2, int y2)
{
    if ((m_nHints & HINTS_DITHERFLAGS) != 0) {
	if (m_pDitherErrors == 0) {
	    int size = sizeof(DitherError) * (m_nDstW + 2);
	    m_pDitherErrors = new DitherError[m_nDstW + 2];
	    if (m_pDitherErrors) {
		memset(m_pDitherErrors, 0, size);
	    }
	}
    }
    return m_pDitherErrors;
}

AlphaError *AwtImage::AlphaSetup(int x1, int y1, int x2, int y2, BOOL create)
{
    if ((m_nHints & HINTS_DITHERFLAGS) != 0) {
	if (create && m_pAlphaErrors == 0) {
	    int size = sizeof(AlphaError) * (m_nDstW + 2);
	    m_pAlphaErrors = new AlphaError[m_nDstW + 2];
	    if (m_pAlphaErrors) {
		memset(m_pAlphaErrors, 0, size);
	    }
	}
    }
    return m_pAlphaErrors;
}

void AwtImage::BufDone(int x1, int y1, int x2, int y2) {
    int ytop = y1;
    if ((m_nHints & HINTS_SCANLINEFLAGS) != 0) {
	int l;
	if (m_pSeen == NULL) {
	    m_pSeen = new char[m_nDstH];
	    memset(m_pSeen, 0, m_nDstH);
	}
	char *pB = ((char *) m_pBitmapData) + y1 * m_nBufScan;
	char *pM = (char *) m_pTransparencyData;
	int tscan;
	if (pM != NULL) {
	    tscan = ((m_nDstW + 31) & (~31)) >> 3;
	    pM += y1 * tscan;
	}
	for (l = y1 - 1; l >= 0; l--) {
	    if (m_pSeen[l]) {
		break;
	    }
	    memcpy(pB - m_nBufScan, pB, m_nBufScan);
	    pB -= m_nBufScan;
	    if (pM != NULL) {
		memcpy(pM - tscan, pM, tscan);
		pM -= tscan;
	    }
	    ytop = l;
	}
	for (l = y1; l < y2; l++) {
	    m_pSeen[l] = 1;
	}
    } else {
	HRGN hR = ::CreateRectRgn(x1, y1, x2, y2);
	if (m_hRgn == NULL) {
	    m_hRgn = hR;
	} else {
	    ::CombineRgn(m_hRgn, m_hRgn, hR, RGN_OR);
	    ::DeleteObject(hR);
	}
    }
    if (m_nLastLine < y2) {
	m_nLastLine = y2;
    }
    Hjava_awt_Rectangle *newbits = unhand(m_pJavaObject)->newbits;
    if (newbits != 0) {
	Classjava_awt_Rectangle *pNB = unhand(newbits);
	pNB->x = x1;
	pNB->y = ytop;
	pNB->width = x2 - x1;
	pNB->height = y2 - ytop;
    }
};

void AwtImage::Init(Hsun_awt_image_ImageRepresentation *pJavaObject)
{
    m_pJavaObject = pJavaObject;
    m_nDstW = pJavaObject->obj->width;
    m_nDstH = pJavaObject->obj->height;
    m_nSrcW = pJavaObject->obj->srcW;
    m_nSrcH = pJavaObject->obj->srcH;
    m_nHints = pJavaObject->obj->hints;
}

BOOL AwtImage::Create(Hsun_awt_image_ImageRepresentation *pJavaObject,
		      long nW, long nH)
{
	long nPaddedWidth = (nW + 3) & ~0x3;
	
	m_pJavaObject = pJavaObject;
	m_nDstW = m_nSrcW = nW;
	m_nDstH = m_nSrcH = nH;
	HDC hDC = ::CreateDC("DISPLAY", NULL, NULL, NULL);
	m_hBitmap = ::CreateCompatibleBitmap(hDC, nW, nH);
	if (m_hBitmap) {
	    HDC hMemDC = ::CreateCompatibleDC(hDC);

	    // The bitmap should be initially all white
	    ::SelectObject(hMemDC, m_hBitmap);
	    ::FillRect(hMemDC, CRect(0, 0, nW, nH),
		       (HBRUSH)::GetStockObject( WHITE_BRUSH ));
	    VERIFY(::SetBkMode(hMemDC, TRANSPARENT) != NULL);
	    ::SelectObject(hMemDC, ::GetStockObject(NULL_BRUSH));
	    ::SelectObject(hMemDC, ::GetStockObject(NULL_PEN));
	    m_pSharedDC = new AwtSharedDC(NULL, hMemDC);
	}

	::DeleteDC(hDC);

	return (m_hBitmap != NULL);
}

BOOL AwtImage::freeRenderData(BOOL force) {
	if (m_pDitherErrors) {
	    delete []m_pDitherErrors;
	    m_pDitherErrors = NULL;
	}
	if (m_pAlphaErrors) {
	    delete []m_pAlphaErrors;
	    m_pAlphaErrors = NULL;
	}
	if (m_pRecode) {
	    delete []m_pRecode;
	    m_pRecode = NULL;
	}
	if (m_pIsRecoded) {
	    delete []m_pIsRecoded;
	    m_pIsRecoded = NULL;
	}
	if (m_pSeen) {
	    delete []m_pSeen;
	    m_pSeen = NULL;
	}
	if (m_hRgn) {
	    VERIFY(::DeleteObject(m_hRgn));
	    m_hRgn = NULL;
	}
	BOOL ret = (!force && (m_nHints & HINTS_DITHERFLAGS) == 0);
	m_nHints |= HINTS_DITHERFLAGS;
	return ret;
}

// <<Not properly cleaning up icons.>>
// <<Also, we're redithering which isn't great>>
HICON AwtImage::ToIcon() {
	if (m_hIcon != NULL) {
		return m_hIcon;
	}
	if (m_pBitmapData != NULL) {
		int nW = ::GetSystemMetrics(SM_CXICON);
		int nH = ::GetSystemMetrics(SM_CYICON);
 		BYTE *pMask = NULL;
		HDC hDC;
		ICONINFO iconInfo;
		HBITMAP hMask;

		// Invert the bitmap data.

		hMask = m_hTransparencyMask;
		if (!hMask) {
			pMask = new BYTE[nW * nH / 8];
			memset(pMask, 0, nW * nH / 8);
			hMask= ::CreateBitmap(nW, nH, 1, 1, pMask);
		}
		iconInfo.fIcon = TRUE;
		iconInfo.hbmMask = hMask;
		VERIFY(hDC = ::CreateDC("DISPLAY", NULL, NULL, NULL));
		iconInfo.hbmColor = GetDDB(hDC);
		VERIFY(::DeleteDC(hDC));
		m_hIcon = ::CreateIconIndirect(&iconInfo);
		VERIFY(::DeleteObject(hMask));

		// free resources.
		if (pMask) {
			delete [] pMask;
		}
	}
	if (m_hIcon == NULL) {
		m_hIcon = ((CAwtApp *)AfxGetApp())->GetAwtIcon();
	}
	return m_hIcon;
}

/*
 * A note on the rop codes used below:
 *
 * In BitBlt, Windows uses the low order 8 bits of the high order word
 * (bits 16-23) to control how to combine the current brush, the source
 * bitmap, and the destination device.  Each bit refers to what to set
 * the resulting bit to based on a particular combination of 1 or 0 bits
 * in the 3 inputs as follows:
 *
 * B0 - brush  bit is 0,    B1 - brush  bit is 1
 * S0 - source bit is 0,    S1 - source bit is 1
 * D0 - dest   bit is 0,    D1 - dest   bit is 1
 *
 * bit 16 - B0, S0, D0
 * bit 17 - B0, S0, D1
 * bit 18 - B0, S1, D0
 * bit 19 - B0, S1, D1
 * bit 20 - B1, S0, D0
 * bit 21 - B1, S0, D1
 * bit 22 - B1, S1, D0
 * bit 23 - B1, S1, D1
 *
 * Thus, if bit 16 is 1, then a 1 bit is stored in the destination if
 * B0, S0, D0, that is all 3 corresponding bits of the brush, the source
 * bitmap, and the original destination pixel are 0.
 */
void AwtImage::Draw(HDC hDC, long nX, long nY,
		    int xormode, COLORREF xorcolor, HBRUSH xorBrush,
		    struct Hjava_awt_Color *c)
{
	HDC hMemDC;
	HBITMAP hOldBmp = NULL;
	BOOL bRealizePalette = TRUE;

	if (bRealizePalette) {
		VERIFY(::SelectPalette(hDC, m_hPal, FALSE));
		UINT result = ::RealizePalette(hDC);
	}

	VERIFY(hMemDC = ::CreateCompatibleDC(hDC));

	HBITMAP hBitmap = GetDDB(hDC);
	if (m_hTransparencyMask) {
	    hOldBmp = (HBITMAP)::SelectObject(hMemDC, m_hTransparencyMask);
	    COLORREF color;
	    HBRUSH hBrush;
	    int rop;
	    if (xormode) {
		hBrush = xorBrush;
		color = xorcolor;
		rop = 0x00e20000;
	    } else if (c == 0) {
		hBrush = NULL;
		color = PALETTERGB(0, 0, 0);
		rop = 0x00220000;
	    } else {
		hBrush = NULL;
		Classjava_awt_Color *pColor = (Classjava_awt_Color *)c->obj;
		color = PALETTERGB((pColor->value>>16)&0xff,
				   (pColor->value>>8)&0xff,
				   (pColor->value)&0xff);
		rop = 0x00e20000;
	    }
	    if (color != m_bgcolor) {
		HDC hMemDC2;
		HBITMAP hOldBitmap2;
		HBRUSH hOldBrush;
		hOldBmp = (HBITMAP)::SelectObject(hMemDC, m_hTransparencyMask);
		if (!xormode && c != 0) {
		    hBrush = ::CreateSolidBrush(color);
		}
		VERIFY(hMemDC2 = ::CreateCompatibleDC(hDC));
		hOldBitmap2 = (HBITMAP) ::SelectObject(hMemDC2, hBitmap);
		if (hBrush != NULL) {
		    hOldBrush = (HBRUSH) ::SelectObject(hMemDC2, hBrush);
		}
		VERIFY(::BitBlt(hMemDC2, 0, 0, m_nDstW, m_nLastLine,
				hMemDC, 0, 0, rop));
		if (hBrush != NULL) {
		    VERIFY(::SelectObject(hMemDC2, hOldBrush));
		}
		VERIFY(::SelectObject(hMemDC2, hOldBitmap2));
		VERIFY(::DeleteDC(hMemDC2));
		if (!xormode && c != 0) {
		    VERIFY(::DeleteObject(hBrush));
		}
		m_bgcolor = color;
	    }
	    if (xormode || c != 0) {
		::SelectObject(hMemDC, hBitmap);
		VERIFY(::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
				hMemDC, 0, 0,
				xormode ? 0x00960000 : SRCCOPY));
	    } else {
		VERIFY(::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
				hMemDC, 0, 0, SRCAND));

		::SelectObject(hMemDC, hBitmap);
		VERIFY(::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
				hMemDC, 0, 0, SRCPAINT));
	    }
	} else {
		int hasRgn;
		HRGN hR;
		if (m_hRgn != NULL) {
		    hR = ::CreateRectRgn(0, 0, 0, 0);
		    hasRgn = ::GetClipRgn(hDC, hR);
		    VERIFY(::OffsetClipRgn(hDC, -nX, -nY) != ERROR);
		    VERIFY(::ExtSelectClipRgn(hDC, m_hRgn, RGN_AND) != ERROR);
		    VERIFY(::OffsetClipRgn(hDC, nX, nY) != ERROR);
		}
		hOldBmp = (HBITMAP)::SelectObject(hMemDC, hBitmap);
		VERIFY(::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
				hMemDC, 0, 0,
				xormode ? 0x00960000 : SRCCOPY));
		if (m_hRgn != NULL) {
			if (hasRgn) {
				VERIFY(::SelectClipRgn(hDC, hR) != ERROR);
			} else {
				VERIFY(::SelectClipRgn(hDC, NULL) != ERROR);
			}
		}
	}
	
	VERIFY(::SelectObject(hMemDC, hOldBmp) != NULL);
	VERIFY(::DeleteDC(hMemDC));
}

#define SET_BYTE_PIXELS_SIG	"(IIIILjava/awt/image/ColorModel;[BII)V"

#define HINTS_OFFSCREENSEND (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |  \
			     java_awt_image_ImageConsumer_COMPLETESCANLINES | \
			     java_awt_image_ImageConsumer_SINGLEPASS)

extern "C" {
#define AWT_LOCK() GetApp()->MonitorEnter()
#define AWT_UNLOCK() GetApp()->MonitorExit()
}

void AwtImage::sendPixels(Classsun_awt_image_OffScreenImageSource *osis)
{
    struct Hjava_awt_image_ImageConsumer *ich = osis->theConsumer;
    struct execenv *ee;
    struct Hjava_awt_image_ColorModel *cm;
    HObject *pixh;
    char *buf;
    int i;
    HDC hDC;

    ee = EE();
    if (osis->theConsumer == 0) {
	return;
    }
    cm = getColorModel();
    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				"setColorModel",
				"(Ljava/awt/image/ColorModel;)V",
				cm);
    if (osis->theConsumer == 0 || exceptionOccurred(ee)) {
	return;
    }
    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				"setHints",
				"(I)V",
				HINTS_OFFSCREENSEND);
    if (osis->theConsumer == 0 || exceptionOccurred(ee)) {
	return;
    }
    int nPaddedWidth = (m_nDstW + 3) & (~3);
    pixh = (HObject *) ArrayAlloc(T_BYTE, nPaddedWidth);
    buf = (char *) unhand(pixh);
    VERIFY(hDC = ::CreateDC("DISPLAY", NULL, NULL, NULL));
    BITMAPINFO *pBitmapInfo = (BITMAPINFO*) malloc(sizeof(BITMAPINFO)
						   + 256*sizeof(RGBQUAD));
    pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBitmapInfo->bmiHeader.biWidth = m_nDstW; 
    pBitmapInfo->bmiHeader.biHeight = m_nDstH;
    pBitmapInfo->bmiHeader.biPlanes = 1;
    pBitmapInfo->bmiHeader.biBitCount = 0;
    pBitmapInfo->bmiHeader.biCompression = BI_RGB;
    pBitmapInfo->bmiHeader.biSizeImage = nPaddedWidth * m_nDstH;
    pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
    pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
    pBitmapInfo->bmiHeader.biClrUsed = 256;
    pBitmapInfo->bmiHeader.biClrImportant = 256;
    AWT_LOCK();
    VERIFY(::GetDIBits(hDC, m_hBitmap, 0, m_nDstH, NULL,
		       pBitmapInfo, DIB_RGB_COLORS));
    BYTE *pTemp = new BYTE[nPaddedWidth * pBitmapInfo->bmiHeader.biHeight];
    pBitmapInfo->bmiHeader.biHeight = -pBitmapInfo->bmiHeader.biHeight;
    VERIFY(::GetDIBits(hDC, m_hBitmap, 0, m_nDstH, pTemp,
		       pBitmapInfo, DIB_RGB_COLORS));
    AWT_UNLOCK();
    for (i = 0; i < m_nDstH; i++) {
	if (osis->theConsumer == 0) {
	    break;
	}
	memcpy(buf, &pTemp[i * nPaddedWidth], m_nDstW);
	execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				    "setPixels",
				    SET_BYTE_PIXELS_SIG,
				    0, i, m_nDstW, 1,
				    cm, pixh, 0, nPaddedWidth);
	if (exceptionOccurred(ee)) {
	    break;
	}
    }
    delete [] pTemp;
    VERIFY(::DeleteDC(hDC));
    free(pBitmapInfo);
    if (buf == 0) {
	/* This can never happen, but we need to make a reference to buf
	 * at the bottom of this function so the buffer doesn't move
	 * while we are using it.
	 */
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
}

// First check to see if (r,g,b) has been allocated in 'pPalEntries'.  
// If not, allocate the color.  If the 'pPalEntries' is full, return -1.
// If the peFlags field is not-zero, it means it has been allocated.
int AllocateColor(PALETTEENTRY *pPalEntries, int nPaletteSize, int r, int g, int b)
{
	int i;
	int firstEmpty = -1;

	r = min(255, r);
	g = min(255, g);
	b = min(255, b);
	for (i=0; i<nPaletteSize; i++) {
		if (firstEmpty < 0 && pPalEntries[i].peFlags == 0) {
			firstEmpty = i;
		}
		if (pPalEntries[i].peFlags != 0 
			&& pPalEntries[i].peRed == r
			&& pPalEntries[i].peGreen == g
			&& pPalEntries[i].peBlue == b) {
				return i;
		}
	}
	if (firstEmpty < 0)	{
		return -1;
	}
	pPalEntries[firstEmpty].peRed = r;
	pPalEntries[firstEmpty].peGreen = g;
	pPalEntries[firstEmpty].peBlue = b;
	pPalEntries[firstEmpty].peFlags = PC_NOCOLLAPSE;
	return firstEmpty;
}

#define SetRGBQUAD(p, r, g, b)	\
    {				\
	(p)->rgbRed = r;	\
	(p)->rgbGreen = g;	\
	(p)->rgbBlue = b;	\
	(p)->rgbReserved = 0;	\
    }

//
// Initialize the Awt palette from a prototype color palette.
//
void AwtImage::InitializePalette(HPALETTE hPal)
{
	int i, ri, gi, bi;
	ColorInfo *pColorInfo;
		
	pColorInfo = &g_colorInfo[COLOR8];
	gpBitmapInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+256*sizeof(RGBQUAD));
	gpBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	gpBitmapInfo->bmiHeader.biWidth = 0; 
	gpBitmapInfo->bmiHeader.biHeight = 0;
	gpBitmapInfo->bmiHeader.biPlanes = 1;	//nb: NOT equal to planes from GIF
	gpBitmapInfo->bmiHeader.biBitCount = 8;
	gpBitmapInfo->bmiHeader.biCompression = BI_RGB;
	gpBitmapInfo->bmiHeader.biSizeImage = 0;
	gpBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	gpBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	gpBitmapInfo->bmiHeader.biClrUsed = 256;
	gpBitmapInfo->bmiHeader.biClrImportant = 256;
	pColorInfo->m_pPaletteColors = (RGBQUAD*)((LPSTR)gpBitmapInfo + sizeof(BITMAPINFOHEADER));

	// Create a BitmapInfo object for transparency masks
	gpTransMaskInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+2*sizeof(RGBQUAD));
	gpTransMaskInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	gpTransMaskInfo->bmiHeader.biWidth = 0; 
	gpTransMaskInfo->bmiHeader.biHeight = 0;
	gpTransMaskInfo->bmiHeader.biPlanes = 1;
	gpTransMaskInfo->bmiHeader.biBitCount = 1;
	gpTransMaskInfo->bmiHeader.biCompression = BI_RGB;
	gpTransMaskInfo->bmiHeader.biSizeImage = 0;
	gpTransMaskInfo->bmiHeader.biXPelsPerMeter = 0;
	gpTransMaskInfo->bmiHeader.biYPelsPerMeter = 0;
	gpTransMaskInfo->bmiHeader.biClrUsed = 2;
	gpTransMaskInfo->bmiHeader.biClrImportant = 2;
	SetRGBQUAD(&(gpTransMaskInfo->bmiColors[0]), 255, 255, 255);
	SetRGBQUAD(&(gpTransMaskInfo->bmiColors[1]), 0, 0, 0);

	// Create and initialize a palette
    HDC hDC = ::GetDC( ::GetDesktopWindow() );
	int nEntries = max(256, ::GetDeviceCaps(hDC, SIZEPALETTE));
	m_bitspixel = ::GetDeviceCaps(hDC, BITSPIXEL);
    ::DeleteDC(hDC);

 	int nIndex = 0;
	char *buf = new char[sizeof(LOGPALETTE) + nEntries * sizeof(PALETTEENTRY)];
	LOGPALETTE *pLogPal = (LOGPALETTE*)buf;
	PALETTEENTRY *pPalEntries = (PALETTEENTRY *)(&(pLogPal->palPalEntry[0])); 
	int result = max(256, ::GetPaletteEntries(hPal, 0, nEntries, pPalEntries));

	if (result == 256 ) {
	    // Copy the colors from the prototype palette
		for ( i=0; i<nEntries; i++) {
			pColorInfo->m_pPaletteColors[i].rgbRed = pPalEntries[i].peRed;
			pColorInfo->m_pPaletteColors[i].rgbGreen = pPalEntries[i].peGreen,
			pColorInfo->m_pPaletteColors[i].rgbBlue = pPalEntries[i].peBlue;
			pColorInfo->m_pPaletteColors[i].rgbReserved = 0;
		}
	    // 
	    // Match colors for the color cube to the closest color in the 
	    // prototype palette
	    for (bi = 0 ; bi <= CMAP_COLS ; bi += 1) {
			int b = bi << (8 - CMAP_BITS);
			for (gi = 0 ; gi <= CMAP_COLS ; gi += 1) {
			    int g = gi << (8 - CMAP_BITS);
			    for (ri = 0 ; ri <= CMAP_COLS ; ri += 1) {
					int r = ri << (8 - CMAP_BITS);
					pColorInfo->m_rgbCube[ri][gi][bi] = 
						AwtImage::ClosestMatchPalette(pColorInfo->m_pPaletteColors, 256, r, g, b);
			    }
			}
	    }
	}
	pLogPal->palNumEntries = 256;
	pLogPal->palVersion = 0x300;
	m_hPal = ::CreatePalette(pLogPal);

    delete buf;
}


void AwtImage::InitializePalette(HDC hDC)
{
	int i, ri, gi, bi;
	ColorInfo *pColorInfo;
		
	pColorInfo = &g_colorInfo[COLOR8];
	gpBitmapInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+256*sizeof(RGBQUAD));
	gpBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	gpBitmapInfo->bmiHeader.biWidth = 0; 
	gpBitmapInfo->bmiHeader.biHeight = 0;
	gpBitmapInfo->bmiHeader.biPlanes = 1;	//nb: NOT equal to planes from GIF
	gpBitmapInfo->bmiHeader.biBitCount = 8;
	gpBitmapInfo->bmiHeader.biCompression = BI_RGB;
	gpBitmapInfo->bmiHeader.biSizeImage = 0;
	gpBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	gpBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	gpBitmapInfo->bmiHeader.biClrUsed = 256;
	gpBitmapInfo->bmiHeader.biClrImportant = 256;
	pColorInfo->m_pPaletteColors = (RGBQUAD*)((LPSTR)gpBitmapInfo + sizeof(BITMAPINFOHEADER));

	// Create a BitmapInfo object for transparency masks
	gpTransMaskInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+2*sizeof(RGBQUAD));
	gpTransMaskInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	gpTransMaskInfo->bmiHeader.biWidth = 0; 
	gpTransMaskInfo->bmiHeader.biHeight = 0;
	gpTransMaskInfo->bmiHeader.biPlanes = 1;
	gpTransMaskInfo->bmiHeader.biBitCount = 1;
	gpTransMaskInfo->bmiHeader.biCompression = BI_RGB;
	gpTransMaskInfo->bmiHeader.biSizeImage = 0;
	gpTransMaskInfo->bmiHeader.biXPelsPerMeter = 0;
	gpTransMaskInfo->bmiHeader.biYPelsPerMeter = 0;
	gpTransMaskInfo->bmiHeader.biClrUsed = 2;
	gpTransMaskInfo->bmiHeader.biClrImportant = 2;
	SetRGBQUAD(&(gpTransMaskInfo->bmiColors[0]), 255, 255, 255);
	SetRGBQUAD(&(gpTransMaskInfo->bmiColors[1]), 0, 0, 0);

	m_bitspixel = ::GetDeviceCaps(hDC, BITSPIXEL);
	// Create and initialize a palette
	int nEntries = max(256, ::GetDeviceCaps(hDC, SIZEPALETTE));
 	int nIndex = 0;
	char *buf = new char[sizeof(LOGPALETTE) + nEntries * sizeof(PALETTEENTRY)];
	LOGPALETTE *pLogPal = (LOGPALETTE*)buf;
	PALETTEENTRY *pPalEntries = (PALETTEENTRY *)(&(pLogPal->palPalEntry[0])); 
	int result = max(256, ::GetSystemPaletteEntries(hDC, 0, nEntries, pPalEntries));

	if (result == nEntries) {  
		if (result == 256 ) {
			// Temporarily use the peFlags field to indicate whether or not a color has been allocated
			for (i=0; i<256; i++) {
				pPalEntries[i].peFlags = 0;
			}
			for (i=0; i<10; i++) {
				pPalEntries[i].peFlags = 1;
				pPalEntries[255-i].peFlags = 1;
			}
		    
			// Allocate special colors.
		    AllocateColor(pPalEntries, 256, 255, 255, 255);
		    AllocateColor(pPalEntries, 256, 255, 0, 0);
		    AllocateColor(pPalEntries, 256, 0, 255, 0);
		    AllocateColor(pPalEntries, 256, 0, 0, 255);
		    AllocateColor(pPalEntries, 256, 255, 255, 0);
		    AllocateColor(pPalEntries, 256, 255, 0, 255);
		    AllocateColor(pPalEntries, 256, 0, 255, 255);
		    AllocateColor(pPalEntries, 256, 235, 235, 235);
		    AllocateColor(pPalEntries, 256, 224, 224, 224);
		    AllocateColor(pPalEntries, 256, 214, 214, 214);
		    AllocateColor(pPalEntries, 256, 192, 192, 192);
		    AllocateColor(pPalEntries, 256, 162, 162, 162);
		    AllocateColor(pPalEntries, 256, 128, 128, 128);
		    AllocateColor(pPalEntries, 256, 105, 105, 105);
		    AllocateColor(pPalEntries, 256, 64, 64, 64);
		    AllocateColor(pPalEntries, 256, 32, 32, 32);
		    AllocateColor(pPalEntries, 256, 255, 128, 128);
		    AllocateColor(pPalEntries, 256, 128, 255, 128);
		    AllocateColor(pPalEntries, 256, 128, 128, 255);
		    AllocateColor(pPalEntries, 256, 255, 255, 128);
		    AllocateColor(pPalEntries, 256, 255, 128, 255);
		    AllocateColor(pPalEntries, 256, 128, 255, 255);

	    	// Allocate some colors.
		    for (bi = 0 ; bi <= CMAP_COLS ; bi += 2) {
				int b = bi << (8 - CMAP_BITS);
				for (gi = 0 ; gi <= CMAP_COLS ; gi += 2) {
				    int g = gi << (8 - CMAP_BITS);
				    for (ri = 0; ri <= CMAP_COLS ; ri += 1) {
						int r = ri << (8 - CMAP_BITS);
						if (pColorInfo->m_rgbCube[ri][gi][bi] == 0) {
						    pColorInfo->m_rgbCube[ri][gi][bi] = AllocateColor(pPalEntries, 256, r, g, b);
						}
				    }
			    }
			}
		    
		    // Match remainder.
		    BOOL bFull = FALSE;
		    for (bi = 0 ; bi <= CMAP_COLS ; bi += 1) {
				int b = bi << (8 - CMAP_BITS);
				for (gi = 0 ; gi <= CMAP_COLS ; gi += 1) {
				    int g = gi << (8 - CMAP_BITS);
				    for (ri = 0 ; ri <= CMAP_COLS ; ri += 1) {
						int r = ri << (8 - CMAP_BITS);
						if (pColorInfo->m_rgbCube[ri][gi][bi] == 0) {
							if (bFull) {
								pColorInfo->m_rgbCube[ri][gi][bi] = 
									AwtImage::ClosestMatchPalette(pColorInfo->m_pPaletteColors, 256, r, g, b);
							} else {
							    int loc = AllocateColor(pPalEntries, 256, r, g, b);
								if (loc >= 0) {
							    	pColorInfo->m_rgbCube[ri][gi][bi] = loc;
								} else {
									// If color couldn't be allocated, match it.
									// Initialize m_paletteColors8.
									for ( i=0; i<nEntries; i++) {
										pColorInfo->m_pPaletteColors[i].rgbRed = pPalEntries[i].peRed;
										pColorInfo->m_pPaletteColors[i].rgbGreen = pPalEntries[i].peGreen,
										pColorInfo->m_pPaletteColors[i].rgbBlue = pPalEntries[i].peBlue;
										pColorInfo->m_pPaletteColors[i].rgbReserved = 0;
									}

								    pColorInfo->m_rgbCube[ri][gi][bi] = 
								    	AwtImage::ClosestMatchPalette(pColorInfo->m_pPaletteColors, 256, r, g, b);
								    bFull = TRUE;
								}
							}
						}
				    }
				}
		    }
			// Now clear flags of system colors.
			for (i=0; i<10; i++) {
				pPalEntries[i].peFlags = 0;
				pPalEntries[255-i].peFlags = 0;
			}
		}
		pLogPal->palNumEntries = 256;
		pLogPal->palVersion = 0x300;
		m_hPal = ::CreatePalette(pLogPal);
	}
	delete buf;

	// Now allocate the 4-bit color palette.
	pColorInfo = &g_colorInfo[COLOR4];
	pColorInfo->m_pPaletteColors = (RGBQUAD*)malloc(16*sizeof(RGBQUAD));
	/* 9: 166, 202, 240 	246: 255 251 240 -- non-VGA colors */	
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[0]), 0, 0, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[1]), 128, 0, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[2]), 0, 128, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[3]), 128, 128, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[4]), 0, 0, 128);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[5]), 128, 0, 128);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[6]), 0, 128, 128);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[7]), 192, 192, 192);
    //SetRGBQUAD(&(pColorInfo->m_pPaletteColors[*]), 192, 220, 192);	// non-VGA colors
    //SetRGBQUAD(&(pColorInfo->m_pPaletteColors[*]), 160, 160, 164);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[8]), 128, 128, 128);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[9]), 255, 0, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[10]), 0, 255, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[11]), 255, 255, 0);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[12]), 0, 0, 255);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[13]), 255, 0,255);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[14]), 0, 255, 255);
    SetRGBQUAD(&(pColorInfo->m_pPaletteColors[15]), 255, 255, 255);
    for (bi = 0 ; bi <= CMAP_COLS ; bi += 1) {
		int b = bi << (8 - CMAP_BITS);
		for (gi = 0 ; gi <= CMAP_COLS ; gi += 1) {
		    int g = gi << (8 - CMAP_BITS);
		    for (ri = 0 ; ri <= CMAP_COLS ; ri += 1) {
				int r = ri << (8 - CMAP_BITS);
				if (pColorInfo->m_rgbCube[ri][gi][bi] == 0) {
					pColorInfo->m_rgbCube[ri][gi][bi] = 
						AwtImage::ClosestMatchPalette(pColorInfo->m_pPaletteColors, 16, r, g, b);
				}
		    }
		}
    }

}

void AwtImage::DisposePalette()
{
	free(gpBitmapInfo);
	free(gpTransMaskInfo);
	if (m_hPal != NULL) {
		::DeleteObject(m_hPal);
	}
	free(g_colorInfo[COLOR4].m_pPaletteColors);
}

int AwtImage::ClosestMatchPalette(RGBQUAD *p, int nPaletteSize, int r, int g, int b)
{
    int mindist = 256 * 256 * 256, besti = 0;
    int i, t, d;

    if (r > 255) {
		r = 255;
    }
    if (g > 255) {
		g = 255;
    }
    if (b > 255) {
		b = 255;
    }

    for (i = 0 ; i < nPaletteSize ; i++, p++) {
	    t = p->rgbRed - r;
	    d = t * t;
	    if (d >= mindist)
			continue;
	    t = p->rgbGreen- g;
	    d += t * t;
	    if (d >= mindist)
			continue;
	    t = p->rgbBlue- b;
	    d += t * t;
	    if (d >= mindist)
			continue;
	    if (d == 0)	{
			return i;
		}
	    if (d < mindist) {
			besti = i;
			mindist = d;
		}
	}
    return besti;
}

struct Hjava_awt_image_ColorModel *AwtImage::getColorModel()
{
    struct Hjava_awt_image_ColorModel *awt_colormodel;
    HArrayOfByte *hR = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
    HArrayOfByte *hG = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
    HArrayOfByte *hB = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
    char *aBase, *a;
    RGBQUAD *c, *cBase;
    
    cBase = g_colorInfo[1].m_pPaletteColors;
    /* These loops copy backwards so that the aBase pointer won't
     * be dropped by the optimizer and allow the garbage collector
     * to move the array being initialized.
     */
    aBase = unhand(hR)->body;
    for (a = aBase + 255, c = cBase + 255; a >= aBase; a--, c--) {
	*a = c->rgbRed;
    }
    aBase = unhand(hG)->body;
    for (a = aBase + 255, c = cBase + 255; a >= aBase; a--, c--) {
	*a = c->rgbGreen;
    }
    aBase = unhand(hB)->body;
    for (a = aBase + 255, c = cBase + 255; a >= aBase; a--, c--) {
	*a = c->rgbBlue;
    }
    awt_colormodel = (struct Hjava_awt_image_ColorModel *)
	execute_java_constructor(EE(),
				 "java/awt/image/IndexColorModel",
				 0, "(II[B[B[B)",
				 8, 256, hR, hG, hB);
    return awt_colormodel;
}
