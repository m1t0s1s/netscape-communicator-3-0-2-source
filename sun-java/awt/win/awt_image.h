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
#ifndef AWTIMAGE_H
#define AWTIMAGE_H

#include "awt_defs.h"
#include "awt_object.h"
#include <memory.h>     // memset(...)

#if !defined(_AWT_IDCSERVICE_H)
#include "awt_IDCService.h"
#endif

DECLARE_SUN_AWT_IMAGE_PEER(ImageRepresentation);
DECLARE_JAVA_AWT_IMAGE_PEER(ColorModel);
DECLARE_JAVA_AWT_IMAGE_PEER(ImageConsumer);
DECLARE_SUN_AWT_IMAGE_PEER(OffScreenImageSource);
DECLARE_SUN_AWT_WINDOWS_PEER(WGraphics);
DECLARE_JAVA_AWT_PEER(Graphics);
DECLARE_JAVA_AWT_PEER(Color);

#define  AWT_IMAGE_COPY_FAILURE 0
#define  AWT_IMAGE_COPY_SUCCESS 1

#define RED_MASK    0x00ff0000L
#define GREEN_MASK  0x0000ff00L
#define BLUE_MASK   0x000000ffL

#define RED_OFFSET    16L
#define GREEN_OFFSET  8L
#define BLUE_OFFSET   0L


#define GETRED(rgb_pixel) \
  ((rgb_pixel & RED_MASK) >> RED_OFFSET)

#define GETBLUE(rgb_pixel) \
  ((rgb_pixel & BLUE_MASK) >> BLUE_OFFSET)

#define GETGREEN(rgb_pixel) \
  ((rgb_pixel & GREEN_MASK) >> GREEN_OFFSET)




#define CMAP_BITS 3L
#define CMAP_COLS (1L << CMAP_BITS)
#define CMAP_BIAS (1L << (7L - CMAP_BITS))

typedef struct _ColorInfo {
	RGBQUAD *m_pPaletteColors;
	BYTE m_rgbCube[CMAP_COLS+1][CMAP_COLS+1][CMAP_COLS+1];
} ColorInfo;
typedef unsigned char MaskBits;


#include "awt_palette.h"

extern ColorInfo g_colorInfo[];
	
class AwtImage : public AwtObject, public IDCService
{
public:
    // Debug link used for list of ALL objects...

	AwtImage();
	~AwtImage();

public:
    virtual void            Dispose(void){;}
    virtual BOOL            CallMethod(AwtMethodInfo *info){return TRUE;}

    // IDCService interface
    virtual AwtDC*          GetAwtDC() { return m_pAwtDC; }
    // the dc got created in Create()
    virtual void            Attach(AwtDC* pDC) {} // attach does nothing on image 
    virtual void            Detach(HDC hdc);
    virtual HDC             GetDC();
    virtual void            ReleaseDC(AwtDC* pdc) {} // don't delete now
    virtual HBRUSH          GetBackground() { return (HBRUSH)::GetStockObject( WHITE_BRUSH ); } 
    virtual HWND            GetWindowHandle() { return NULL; }

  void Alloc8BitConsumerBuffer();
  void Alloc24BitConsumerBuffer();
  void AllocConsumerBuffer(HColorModel * hCM);
  long copyImagePartInto8BitBuffer(
			      int bgcolor, int srcX, int srcY,
			      int srcW, int srcH, BYTE *srcpix,
			      int srcOff);
  long copyImagePartInto24BitBuffer(
			      int bgcolor, int srcX, int srcY,
			      int srcW, int srcH, int *srcpix,
			      int srcOff);
  void PrintRGBQUADS(RGBQUAD *pRgbq, int numColors);
  void InitRGBQUADS(RGBQUAD *pRgbq, int numColors);
  void PrintImage();
  void Print24BitImage();

// Operations
	// Creates an offscreen DIB of the specified width and height
	BOOL Create(HImageRepresentation *pJavaObject,
		    long nW, long nH);

	// Inits the fields needed to scale an image from an
	// ImageRepresentation object.
	void Init(HImageRepresentation *pJavaObject, HColorModel *cmh);

	// Returns the width of the bitmap.
	long GetWidth() {return m_nDstW;};

	// Returns the height of the bitmap.
	long GetHeight() {return m_nDstH;};

	// Returns the width of the source data.
	long GetSrcWidth() {return m_nSrcW;};

	// Returns the height of the source data.
	long GetSrcHeight() {return m_nSrcH;};

	// Returns the depth of the output device.
	long GetDepth(HColorModel *colormodel); 

	// Allocates/verifies the output buffer for pixel conversion.
	// returns result code - nonzero for success.
	int BufInit(int x1, int y1, int x2, int y2);

	// Returns a pointer to the output buffer for pixel conversion.
	void *GetDstBuf() { return (void *)m_pBitmapData;}

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
	int32_t *GetRecodeBuf(BOOL create) {
	    if (m_pRecode == NULL && create) {
		m_pRecode = new int32_t[256];
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
	int DitherMap(int r, int g, int b) 
 {
	    return m_pAwtPalette->GetRgbVal(r, g, b);
	}

	// Returns the actual color definition of a given pixel.
	RGBQUAD *PixelColor(int pixel) {
	    return (RGBQUAD *)&m_pRGBQuad[pixel];
	}

	// Returns the closest pixel match to the given r, g, b, pairing.
	int ColorMatch(int r, int g, int b) {
	    return m_pAwtPalette->ClosestMatchPalette(r, g, b);
	}


	// Finishes processing a buffer of converted pixels.
	void BufDone(int dstX1, int dstY1, int dstX2, int dstY2);

	long GenericImageConvert(struct Hjava_awt_image_ColorModel *colormodel,
				 int32_t bgcolor, 
                                 int32_t x, int32_t y, int32_t w, int32_t h,
				 void *srcpix,
				 int32_t srcOff, int32_t scrBPP, int32_t srcScan,
				 int32_t srcW, int32_t srcH, int32_t dstW, int32_t dstH,
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
	HBITMAP GetDDB(HDC hDC) {return m_hBitmap;}

	// Draws the image on 'hDC' at ('nX', 'nY') with optional background
	// color c, in copy or xor mode.
	void Draw(HDC hDC, long nX, long nY,
		  DWORD xormode, COLORREF xorcolor, HBRUSH xorBrush,
		  HColor *c);

	// Returns an icon from image.
	HICON ToIcon();

	// This member should be called by the application class during its initialization.
	void InitializePalette(HDC hDC);
	void InitializePalette(HPALETTE hPal, int dummy);

	// Returns a ColorModel object which represents the color space
	// of the screen.
	static struct Hjava_awt_image_ColorModel *getColorModel(RGBQUAD *pRGBQuad);

	// This member should be called by the application class during its finalization.
	void DisposePalette();


	// Handle of palette.  This palette is selected into the DC whenever
	// a DDB needs to be painted.  Also, all DIB's are converted to DIB
	// using this palette.  If NULL, the palette could not be created;
	// this is an error.
	HPALETTE m_hPal;
	static int m_bitspixel;

	// If non-NULL, the image is an offscreen image.  To paint on the
	// image, grab the DC from this shared DC object.  Only one
 	// AwtGraphics object can be associated with the image at one time.
	//class AwtSharedDC *m_pSharedDC;
	class AwtDC *m_pAwtDC;
//private:
// Variables
	// Handle to the bitmap.  May be NULL.
	HBITMAP m_hBitmap;
        HBITMAP m_hOldBitmap;

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


	// The dithering and alpha error buffers.
	struct _DitherError *m_pDitherErrors;
	struct _AlphaError *m_pAlphaErrors;

	// The pixel recoding buffer.
	int32_t *m_pRecode;
	unsigned char *m_pIsRecoded;

	// The pixel delivery tracking fields.
	long m_nLastLine;
	char *m_pSeen;
	HRGN m_hRgn;

 // Consumer buffer for both 24 and 8 bit images.
 HGLOBAL   m_hDIB;
 BITMAPINFO *m_pBitmapInfo;
	BYTE FAR *m_pBitmapData;

 HGLOBAL     m_hTransDIB;
 BITMAPINFO *m_pTransMaskInfo;
	HBITMAP     m_hTransparencyMask;

// Variables
	// Transparent pixels.
	BOOL m_bHasTransparentPixels;
	void FAR *m_pTransparencyData;
	COLORREF m_bgcolor;


	// Back pointer to the Java object for this Image.
	HImageRepresentation *m_pJavaObject;

 AwtPalette *m_pAwtPalette;
 int         m_bitPerPixel;
 RGBQUAD    *m_pRGBQuad;
 protected:

 int         m_selectMask;
 HDC         m_hMaskDC;
 HDC         m_hBMPDC;
};

#endif // _AWTIMAGE_H_
