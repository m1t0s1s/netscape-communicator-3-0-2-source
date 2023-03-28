/*
 * @(#)awt_image.h	1.29 96/01/05 Patrick P. Chan
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
#ifndef _AWTIMAGE_H_
#define _AWTIMAGE_H_

#include "awtapp.h"

#define CMAP_BITS 3
#define CMAP_COLS (1 << CMAP_BITS)
#define CMAP_BIAS (1 << (7 - CMAP_BITS))

typedef struct _ColorInfo {
	RGBQUAD *m_pPaletteColors;
	BYTE m_rgbCube[CMAP_COLS+1][CMAP_COLS+1][CMAP_COLS+1];
} ColorInfo;

typedef unsigned char MaskBits;

extern ColorInfo g_colorInfo[];
	
class AwtImage
{
public:
    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

	AwtImage();
	~AwtImage();

public:
// Operations
	// Creates an offscreen DIB of the specified width and height
	BOOL Create(Hsun_awt_image_ImageRepresentation *pJavaObject,
		    long nW, long nH);

	// Inits the fields needed to scale an image from an
	// ImageRepresentation object.
	void Init(Hsun_awt_image_ImageRepresentation *pJavaObject);

	// Returns the width of the bitmap.
	long GetWidth() {return m_nDstW;};

	// Returns the height of the bitmap.
	long GetHeight() {return m_nDstH;};

	// Returns the width of the source data.
	long GetSrcWidth() {return m_nSrcW;};

	// Returns the height of the source data.
	long GetSrcHeight() {return m_nSrcH;};

	// Returns the depth of the output device.
	long GetDepth() {return 8;};

	// Allocates/verifies the output buffer for pixel conversion.
	// returns result code - nonzero for success.
	int BufInit(int x1, int y1, int x2, int y2);

	// Returns a pointer to the output buffer for pixel conversion.
	void *GetDstBuf() {return (void *) m_pBitmapData;};

	// Returns a pointer to the buffer for mask bits.
	// It creates the buffer if the argument is TRUE.
	MaskBits *GetMaskBuf(BOOL create);

	// Returns the real width of the memory buffer.
	long GetBufScan() {return m_nBufScan;};

	// Returns a pointer to the dithering error buffer.
	struct _DitherError *GetDitherBuf(int x1, int y1, int x2, int y2);

	// Returns a pointer to the alpha dithering error buffer.
	struct _AlphaError *AlphaSetup(int x1, int y1, int x2, int y2,
				       BOOL create);

	// Returns a pointer to the array for recoding pixels.
	int *GetRecodeBuf(BOOL create) {
	    if (m_pRecode == NULL && create) {
		m_pRecode = new int[256];
		m_pIsRecoded = new unsigned char[256];
	    }
	    return m_pRecode;
	};

	// Returns a pointer to the array of booleans indicating
	// which pixel values have been recoded.
	unsigned char *GetIsRecodedBuf() {
	    if (m_pIsRecoded) {
		memset(m_pIsRecoded, 0, 256);
	    }
	    return m_pIsRecoded;
	};

	// Returns the closest pixel in the color cube.
	int DitherMap(int r, int g, int b) {
	    return (g_colorInfo[1].m_rgbCube
		    [(int)((r) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]
		    [(int)((g) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]
		    [(int)((b) + CMAP_BIAS) >> (int)(8 - CMAP_BITS)]);
	}

	// Returns the actual color definition of a given pixel.
	RGBQUAD *PixelColor(int pixel) {
	    return g_colorInfo[1].m_pPaletteColors + pixel;
	}

	// Returns the closest pixel match to the given r, g, b, pairing.
	int ColorMatch(int r, int g, int b) {
	    return ClosestMatchPalette(g_colorInfo[1].m_pPaletteColors,
				       256, r, g, b);
	}

	// Finishes processing a buffer of converted pixels.
	void BufDone(int dstX1, int dstY1, int dstX2, int dstY2);

	long GenericImageConvert(struct Hjava_awt_image_ColorModel *colormodel,
				 int bgcolor, int x, int y, int w, int h,
				 void *srcpix,
				 int srcOff, int scrBPP, int srcScan,
				 int srcW, int srcH, int dstW, int dstH,
				 AwtImage *pImage);

	// Completes all image conversion/scaling operations.
	// Returns true if a resend is needed for correct dithering.
	BOOL freeRenderData(BOOL force);

	// Sends the pixels associated with this offscreen image to
	// an ImageConsumer.
	void sendPixels(struct Classsun_awt_image_OffScreenImageSource *osis);

	// Returns the handle to the bitmap.  May be NULL.
	// The bitmap may not yet be converted to a DDB in which case this
	// call will cause the conversion.  Assumes that the palette is
	// already selected into 'hDC'.
	// <<note: I've tried to convert the bitmap using a DC created with
	// ::CreateCompatibleDC(NULL) and selecting in the palette; this
	// doesn't seem to work. 
	HBITMAP GetDDB(HDC hDC) {return m_hBitmap;};

	// Draws the image on 'hDC' at ('nX', 'nY') with optional background
	// color c, in copy or xor mode.
	void Draw(HDC hDC, long nX, long nY,
		  int xormode, COLORREF xorcolor, HBRUSH xorBrush,
		  struct Hjava_awt_Color *c);

	// Returns an icon from image.
	HICON ToIcon();

	// This member should be called by the application class during its initialization.
	static void InitializePalette(HDC hDC);
	static void InitializePalette(HPALETTE hPal);

	// Returns a ColorModel object which represents the color space
	// of the screen.
	static struct Hjava_awt_image_ColorModel *getColorModel();

	// This member should be called by the application class during its finalization.
	static void DisposePalette();

	// Takes the specified rgb value, goes through m_paletteColors looking for the closest
	// matching color and returns an index into m_paletteColors.  'r','g','b' can be
	// greater than 256.
	static int ClosestMatchPalette(RGBQUAD *p, int nPaletteSize, int r, int g, int b);

// Variables
	// Transparent pixels.
	BOOL m_bHasTransparentPixels;
	HBITMAP m_hTransparencyMask;
	void *m_pTransparencyData;
	COLORREF m_bgcolor;

	// Handle of palette.  This palette is selected into the DC whenever
	// a DDB needs to be painted.  Also, all DIB's are converted to DIB
	// using this palette.  If NULL, the palette could not be created;
	// this is an error.
	static HPALETTE m_hPal;
	static int m_bitspixel;

	// If non-NULL, the image is an offscreen image.  To paint on the
	// image, grab the DC from this shared DC object.  Only one
 	// AwtGraphics object can be associated with the image at one time.
	class AwtSharedDC *m_pSharedDC;
//private:
// Variables
	// Handle to the bitmap.  May be NULL.
	HBITMAP m_hBitmap;

	// Non-NULL if image was converted to an icon.
	HICON m_hIcon;

	// The dimensions of the original image data.
	long m_nSrcW;
	long m_nSrcH;

	// The dimensions of the resulting bitmap.
	long m_nDstW;
	long m_nDstH;

	// The dimensions of the conversion buffer.
	long m_nBufScan;

	// The rendering hints.
	long m_nHints;

	// The converstion buffer.
	void *m_pBitmapData;

	// The dithering and alpha error buffers.
	struct _DitherError *m_pDitherErrors;
	struct _AlphaError *m_pAlphaErrors;

	// The pixel recoding buffer.
	int *m_pRecode;
	unsigned char *m_pIsRecoded;

	// The pixel delivery tracking fields.
	long m_nLastLine;
	char *m_pSeen;
	HRGN m_hRgn;

	// Back pointer to the Java object for this Image.
	Hsun_awt_image_ImageRepresentation *m_pJavaObject;
};

#endif // _AWTIMAGE_H_
