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
#include "awt_dc.h"
#include "awt_palette.h"
#include "awt_image.h"
#include "awt_querydc.h"
#include "awt_graphics.h"
#include "math.h"
#include "awt_imagescale.h"
#include "awt_toolkit.h"

extern "C" {
#include "java_awt_Rectangle.h"
#include "java_awt_Color.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "sun_awt_image_ImageRepresentation.h"
#include "java_awt_image_ImageConsumer.h"
#include "java_awt_image_ImageObserver.h"
#include "sun_awt_windows_WGraphics.h"
};


#define IMAGE_SIZEINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_DRAWINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_SOMEBITS |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)


#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif


#define HINTS_DITHERFLAGS   (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT)
#define HINTS_SCANLINEFLAGS (java_awt_image_ImageConsumer_COMPLETESCANLINES)

const int COLOR4 = 0;
const int COLOR8 = 1;

ColorInfo g_colorInfo[2];

/* efficiently map an RGB value to a pixel value */
#define RGBMAP(pColorInfo, r, g, b)	 \
	((pColorInfo)->m_rgbCube[(int)((r) + CMAP_BIAS) >> (int)(8L - CMAP_BITS)]\
	 	[(int)((g) + CMAP_BIAS) >> (int)(8L - CMAP_BITS)]\
	 	[(int)((b) + CMAP_BIAS) >> (int)(8L - CMAP_BITS)])

#define COMPOSE24(I, r, g, b)		\
	((((b) & 0xffL) << ((I)->bOff)) |	\
	 (((g) & 0xffL) << ((I)->gOff)) |	\
	 (((r) & 0xffL) << ((I)->rOff)))


#define SetRGBQUAD(p, r, g, b)	\
{				\
	(p)->rgbRed = r;	\
	(p)->rgbGreen = g;	\
	(p)->rgbBlue = b;	\
	(p)->rgbReserved = 0;	\
}




#define WIDTHBYTES(i)     ((unsigned long)(((i) + 3) & (~3L)))  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (long)WIDTHBYTES((long)bi.biWidth * (long)(bi.biBitCount>>3))

#define IMAGE_SIZEINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_HEIGHT)

//#define DEBUG_IMAGE


// Initialize static variables
int AwtImage::m_bitspixel = 0;

// This is data structure holds the common colormap that is used to paint
// all images.  When an image is painted, the BITMAPINFOHEADER of 
// the image is copied into m_pBitmapInfo and then m_pBitmapInfo is used to
// paint the image.  This strategy eliminates an alloc/free when painting
// an image.

AwtImage::AwtImage() : AwtObject(NULL)
{
    m_awtObjType = AWT_IMAGE;
    m_selectMask = 0;
    
    // Back pointer to the java_awt_image_ImageRepresentation
    m_pJavaObject = NULL;

    // Dimension of the original image
    m_nSrcW = 0;
    m_nSrcH = 0;

    // Dimension of the resulting image
    m_nDstW = 0;
    m_nDstH = 0;
    
    // Size of the conversion buffer
    m_nBufScan  = 0;

    // Rendering hints
    m_nHints    = 0;

    // The dithering  and alpha error buffers
    m_pDitherErrors = NULL;
    m_pAlphaErrors  = NULL;

    // The pixel recoding buffer.
    m_pRecode    = NULL;
    m_pIsRecoded = NULL;

    // The pixel delivery tracking fields
    m_nLastLine = 0;
    m_pSeen     = NULL;
    m_hRgn      = NULL;

    // Consumer buffer for both 24 and 8 bit images.
    m_hDIB        = NULL;
    m_pBitmapData = NULL;
    m_pBitmapInfo = NULL;

    // Handle to the bitmap.  May be NULL.
    m_hBitmap     = NULL;
    m_hOldBitmap  = NULL;
    
    // Non-NULL if image was converted to an icon.
    m_hIcon = NULL;

    // Transparency information
    m_bHasTransparentPixels = FALSE;
    m_hTransparencyMask     = NULL;
    m_pTransparencyData     = NULL;
    m_pTransMaskInfo        = NULL;
    m_hTransDIB             = NULL;

    m_bgcolor               = RGB(0, 0, 0);

    // Non-NULL if inage is an off-screen image.
    m_pAwtDC      = NULL;

    // Image palette information
    m_pAwtPalette = NULL;
    m_hPal        = NULL;

    m_pRGBQuad    = NULL;
    m_bitPerPixel = 8;
    m_hBMPDC      = NULL; 
    m_hMaskDC     = NULL;
}

AwtImage::~AwtImage()
{
 if( m_hDIB != NULL)
 {
    GlobalFree(m_hDIB);
 }
 if( m_pBitmapInfo != NULL )
 {
    free(m_pBitmapInfo);
 }

 if( m_hTransDIB != NULL)
 {
    GlobalFree(m_hTransDIB);
 }
 if( m_pTransMaskInfo != NULL )
 {
    free(m_pTransMaskInfo);
 }

 if(m_hBitmap != NULL)
 {
   if (m_pAwtDC) {
      VERIFY(::SelectObject(m_pAwtDC->GetDC(), m_hOldBitmap));
   }
   VERIFY(::DeleteObject(m_hBitmap));
   m_hBitmap  = NULL;
 }
 if(m_hTransparencyMask != NULL)
 {
   VERIFY(::DeleteObject(m_hTransparencyMask));
   m_hTransparencyMask = NULL;
 }


 if(m_hBMPDC != NULL)
 {
   VERIFY(::DeleteDC(m_hBMPDC));
   m_hBMPDC = NULL;
 }
 if(m_hMaskDC != NULL)
 {
   VERIFY(::DeleteDC(m_hMaskDC));
   m_hMaskDC = NULL;
 }

    if (m_pAwtDC != NULL) {
        HDC hdc = m_pAwtDC->GetDC();
        
        // disconnect yourself from the DC object
        m_pAwtDC->ResetTargetObject(); // zero m_pTargetObj in AwtDC
        m_pAwtDC->ResetDC(); // zero m_hObject in AwtDC
        m_pAwtDC->Release(); // release awt dc object
        m_pAwtDC = 0;

        VERIFY(::DeleteDC(hdc));
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
}


//-------------------------------------------------------------------------
//
// Detach the m_pAwtDC. It's time to destroy the dc we allocated
// on GetDC()
//
//-------------------------------------------------------------------------
void AwtImage::Detach(HDC hdc)
{
    if (hdc)
        ::DeleteDC(hdc);
}


//-------------------------------------------------------------------------
//
// Get the dc for this component 
//
//-------------------------------------------------------------------------
HDC AwtImage::GetDC()
{
    HWND hWndDesktop = ::GetDesktopWindow();
    HDC hDC    = ::GetWindowDC(hWndDesktop);
    HDC hRetDC = ::CreateCompatibleDC(hDC);
    VERIFY(::ReleaseDC(hWndDesktop, hDC));

    return hRetDC;
}


#ifdef DEBUG_IMAGE
void AwtImage::PrintRGBQUADS(RGBQUAD *pRgbq, int numColors)
{
   char buf[100];

   for(int i=0; i<numColors; i++)
   {
      wsprintf(buf, "{%d,%d,%d}\n",pRgbq[i].rgbRed,
                                 pRgbq[i].rgbGreen,
                                 pRgbq[i].rgbBlue);
      OutputDebugString(buf);
   }
}
#endif

void AwtImage::InitRGBQUADS(RGBQUAD *pRgbq, int numColors)
{
   for(int i=0; i<numColors; i++)
   {
						pRgbq[i].rgbRed      = 0;
						pRgbq[i].rgbGreen    = 0,
						pRgbq[i].rgbBlue     = 0;
						pRgbq[i].rgbReserved = 0;
   }
}


//-------------------------------------------------------------------------
// Note that for both 8 bit and 24 bit input image source we use a 8 bit 
// destination buffer.
// Due to this in case of 24 bit source, we would be dooing a 24 bit to 8 bit
// conversion and then blting onto the screen. This is fine in case of a 8 bit
// output device. But in case of 24 bit output device htis conversion is redundant. 
// But a inherent advantage in this scheme is that we will save on memory as we 
// would be storing a 24 bit image as a 8 bit DIB.
//-------------------------------------------------------------------------
void AwtImage::Alloc8BitConsumerBuffer()
{
    if (m_pBitmapData == NULL) {
        int numColors = 256; // 0-256

        m_nBufScan = (m_nDstW + 3) & (~3L);

        m_pBitmapInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+numColors*sizeof(RGBQUAD));
        m_pRGBQuad    = (RGBQUAD*)((BYTE*)m_pBitmapInfo + sizeof(BITMAPINFOHEADER));

        // Initialize the palette colors to BLACK (ie. RGB(0,0,0))
        InitRGBQUADS(m_pRGBQuad, numColors);

        // Get the palette colors and set them in the dib.
        if(m_pAwtPalette != NULL) {
            m_pAwtPalette->GetRGBQUADS(m_pRGBQuad); 
        }

#ifdef DEBUG_IMAGE
        PrintRGBQUADS(m_pRGBQuad, numColors);
#endif

        m_pBitmapInfo->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        m_pBitmapInfo->bmiHeader.biWidth           = m_nDstW;//m_nBufScan;
        m_pBitmapInfo->bmiHeader.biHeight          = m_nDstH; // DIB Origin is at lower left corner
        m_pBitmapInfo->bmiHeader.biPlanes          = 1;	//nb: NOT equal to planes from GIF
        m_pBitmapInfo->bmiHeader.biBitCount        = 8;
        m_pBitmapInfo->bmiHeader.biCompression     = BI_RGB;
        m_pBitmapInfo->bmiHeader.biSizeImage       = 0;
        m_pBitmapInfo->bmiHeader.biXPelsPerMeter   = 0;
        m_pBitmapInfo->bmiHeader.biYPelsPerMeter   = 0;
        m_pBitmapInfo->bmiHeader.biClrUsed         = numColors;
        m_pBitmapInfo->bmiHeader.biClrImportant    = numColors;


        m_hDIB = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
                               (long)m_pBitmapInfo->bmiHeader.biHeight * DIBWIDTHBYTES(m_pBitmapInfo->bmiHeader));
        CHECK_NULL(m_hDIB, "OutOfMemoryError");

        m_pBitmapData = (BYTE  FAR *)GlobalLock(m_hDIB);
        m_hBitmap = NULL;
    }
}


//-------------------------------------------------------------------------
// This is not used now and should be used when we have a 24 bit image source.
// Righ now we use the 8 bit destination. This should change to a 24 bit 
// destination code when we plan on doing image editting and hence have more 
// accuracy.
//-------------------------------------------------------------------------

void AwtImage::Alloc24BitConsumerBuffer()
{
    if (m_pBitmapData == NULL) 
    {
        m_pBitmapInfo   = (BITMAPINFO*)malloc(sizeof(BITMAPINFO));
       // Get the palette colors and set them in the dib.
        m_pBitmapInfo->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        m_pBitmapInfo->bmiHeader.biWidth           = m_nDstW;
        m_pBitmapInfo->bmiHeader.biHeight          = m_nDstH; // DIB Origin is at lower left corner
        m_pBitmapInfo->bmiHeader.biPlanes          = 1;	//nb: NOT equal to planes from GIF
        m_pBitmapInfo->bmiHeader.biBitCount        = 24;
        m_pBitmapInfo->bmiHeader.biCompression     = BI_RGB;
        m_pBitmapInfo->bmiHeader.biSizeImage       = 0;
        m_pBitmapInfo->bmiHeader.biXPelsPerMeter   = 0;
        m_pBitmapInfo->bmiHeader.biYPelsPerMeter   = 0;
        m_pBitmapInfo->bmiHeader.biClrUsed         = 0;
        m_pBitmapInfo->bmiHeader.biClrImportant    = 0;


        m_hDIB = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
                            (long)m_pBitmapInfo->bmiHeader.biHeight * DIBWIDTHBYTES(m_pBitmapInfo->bmiHeader));
        CHECK_NULL(m_hDIB, "OutOfMemoryError");

        m_pBitmapData = (BYTE FAR *)GlobalLock(m_hDIB);
        m_hBitmap = NULL;
        m_nBufScan = DIBWIDTHBYTES(m_pBitmapInfo->bmiHeader);
    }
}


void AwtImage::AllocConsumerBuffer(HColorModel * hCM)
{
    ColorModel *pCM  = (ColorModel *)unhand(hCM);
    int bitsPerPixel = (int)pCM->pixel_bits;

    /*
    CHECK_BOUNDARY_CONDITION( (bitsPerPixel != 8 && bitsPerPixel != 24 && 
                               bitsPerPixel != 32 ),
                               "Image source format unsupported" );
    */
    //
    // bitsPerPixel MUST be either 8, 24, 32.  
    // If color depth is 32 bits per pixel create a 24 bit buffer...
    //
    if(GetDepth(hCM) <= 8) {
        Alloc8BitConsumerBuffer();
    } else {
        Alloc24BitConsumerBuffer();
    }
}


	long AwtImage::GetDepth(HColorModel *colormodel) 
 {
    ColorModel *pCM  = (ColorModel *)unhand(colormodel);
    int bitsPerPixel = pCM->pixel_bits;
    //CHECK_BOUNDARY_CONDITION_RETURN((bitsPerPixel != 8 && bitsPerPixel != 24 && bitsPerPixel != 32),"Image source fornat unsupported", 8);

     HWND hWnd = ::GetDesktopWindow();
     HDC hDC=::GetDC(hWnd);

     int devcap = ::GetDeviceCaps(hDC, RASTERCAPS);
     int devBitsPerPixel = 0;
     if( (devcap & RC_PALETTE)  > 0)
     {
       // This is a palette  supporting device. So, create a palette with 
       // size same as that of in the device palette.
       devBitsPerPixel = ::GetDeviceCaps(hDC, BITSPIXEL);
     }
  	 ::ReleaseDC(hWnd, hDC);


    if(  (bitsPerPixel    <= 8)
       ||(devBitsPerPixel == 8)
      )
    {
      return 8;
    }
    else
    {
      return 24;
    }
 }



MaskBits *AwtImage::GetMaskBuf(BOOL create) 
{
    HWND hWndDesktop;
    HDC hDC;
    if (m_pTransparencyData == NULL && create) {
        int32_t nPaddedWidth = (m_nDstW + 31) & (~31L);
        int32_t dibWd        = (nPaddedWidth) >> 3L;

        m_pTransMaskInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+2*sizeof(RGBQUAD));

        m_pTransMaskInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        m_pTransMaskInfo->bmiHeader.biWidth         = m_nDstW;
        m_pTransMaskInfo->bmiHeader.biHeight        = m_nDstH;
        m_pTransMaskInfo->bmiHeader.biPlanes        = 1;
        m_pTransMaskInfo->bmiHeader.biBitCount      = 1;
        m_pTransMaskInfo->bmiHeader.biCompression   = BI_RGB;
        m_pTransMaskInfo->bmiHeader.biSizeImage     = 0;
        m_pTransMaskInfo->bmiHeader.biXPelsPerMeter = 0;
        m_pTransMaskInfo->bmiHeader.biYPelsPerMeter = 0;
        m_pTransMaskInfo->bmiHeader.biClrUsed       = 2;
        m_pTransMaskInfo->bmiHeader.biClrImportant  = 2;

        SetRGBQUAD(&(m_pTransMaskInfo->bmiColors[0]), 255, 255, 255);
        SetRGBQUAD(&(m_pTransMaskInfo->bmiColors[1]), 0, 0, 0);

        hWndDesktop = ::GetDesktopWindow();
        hDC = ::GetWindowDC(hWndDesktop);

        m_hTransDIB = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
                                    (long)m_pTransMaskInfo->bmiHeader.biHeight * dibWd);

        if (m_hTransDIB) {
            m_pTransparencyData = (void FAR *)GlobalLock(m_hTransDIB);
        } else {
            //REMIND: Report malloc error?
            ASSERT(0);
            m_pTransparencyData = NULL;
        }

        m_hTransparencyMask = NULL;

       
        if (m_pTransparencyData != NULL) {
            if ((m_nHints & HINTS_SCANLINEFLAGS) != 0) {
                //memset(m_pTransparencyData, 0xff, (nPaddedWidth*m_nDstH)>>3);
                //memset(m_pTransparencyData, 0xff, (nPaddedWidth*m_nDstH)>>3);
            } else {
                //REMIND: Clean this
                //memset(m_pTransparencyData, 0xff, (nPaddedWidth*m_nDstH)>>3);
                if (m_hRgn != NULL) {
                    RECT rect;
                    HBRUSH hBrush = (HBRUSH)::GetStockObject( BLACK_BRUSH );
                    HDC hMemDC    = ::CreateCompatibleDC(hDC);

                    ::SelectObject(hMemDC, m_hTransparencyMask);
                    ::SelectClipRgn(hMemDC, m_hRgn);
                    rect.left   = 0;
                    rect.top    = 0;
                    rect.right  = (int)m_nDstW;
                    rect.bottom = (int)m_nDstH;

                    ::FillRect(hMemDC, &rect, hBrush);

                    // Select a NULL region before deleting the DC
                    ::SelectClipRgn(hMemDC, NULL);
                    VERIFY(::DeleteDC(hMemDC));
                    // The HBRUSH is a stock object, so dont delete it
                }
            }
        }
        VERIFY(::ReleaseDC(hWndDesktop, hDC));
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
	char FAR *pB = ((char FAR *) m_pBitmapData) + y1 * m_nBufScan;
	char FAR *pM = (char FAR *) m_pTransparencyData;
	int tscan;
	if (pM != NULL) {
	    tscan = ((m_nDstW + 31) & (~31L)) >> 3L;
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



void AwtImage::Init(HImageRepresentation *pJavaObject, HColorModel *cmh)
{
    m_pJavaObject = pJavaObject;
    m_nDstW  = unhand(pJavaObject)->width;
    m_nDstH  = unhand(pJavaObject)->height;
    m_nSrcW  = unhand(pJavaObject)->srcW;
    m_nSrcH  = unhand(pJavaObject)->srcH;
    m_nHints = unhand(pJavaObject)->hints;

    if(cmh != NULL)
    {
        //m_pAwtPalette = AwtToolkit::GetImagePalette();
        m_pAwtPalette = AwtToolkit::GetToolkitDefaultPalette();
        m_pAwtPalette->AddRef();
        m_hPal = m_pAwtPalette->GetPalette();
    }
}

BOOL AwtImage::Create(HImageRepresentation *pJavaObject,
		      long nW, long nH)
{
 /*
 ** This routine sets up a DC with a compatible DDB where all Graphics operations
 ** can be done. It is called from hte offscreeninit api. 
 ** This routine and Graphics routines share a commmon DC.
 ** This is because, a single bitmap can be drawn into from multiple 
 ** graphics contexts got via getGraphics.
 ** This shared DC is used only when there is graphics to do.
 ** When a image is to be copied and blted this shared DC is not used.
 ** This is checked in Graphics::DrawImage which then calls the image->draw()
 ** function. The image->draw() function takes care of transparent bitmaps and
 ** directly blts it from the DIB.
 ** REMIND: When does this bitmap get deleted?
 **         There is no image dispose for this image to be deleted.
 **         GC will pick it up when irh is not referenced anymore.
 ** BUG:    There is no way of drawing into a DIB which has been filled with a image.
 */
    HDC hDC, hDDC;
    RECT rect;
    long nPaddedWidth = (nW + 3) & ~0x3L;

    m_pJavaObject = pJavaObject;
    m_nDstW  = m_nSrcW = nW;
    m_nDstH  = m_nSrcH = nH;

    rect.left   = 0;
    rect.top    = 0;
    rect.right  = nW;
    rect.bottom = nH;

    m_pAwtDC = AwtDC::Create((IDCService *)this);
    ASSERT( m_pAwtDC->m_refCount == 1 );
/// m_pAwtDC->AddRef();
    hDC = m_pAwtDC->Lock(NULL);
    m_pAwtDC->SetRgn(NULL, &rect);

    hDDC      = ::GetWindowDC( ::GetDesktopWindow() );
    m_hBitmap = ::CreateCompatibleBitmap(hDDC, nW, nH);
    VERIFY(::ReleaseDC(::GetDesktopWindow(), hDDC));
    VERIFY( m_hOldBitmap = (HBITMAP)::SelectObject(hDC, m_hBitmap));

    ::FillRect(hDC, &rect, (HBRUSH)::GetStockObject( WHITE_BRUSH ));
    //VERIFY(::SetBkMode(hDC, TRANSPARENT) != NULL);

    m_pAwtDC->Unlock(NULL);
    return (m_hBitmap  != NULL);
}


BOOL AwtImage::freeRenderData(BOOL force) 
{
    BOOL ret;

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

    ret = (!force && (m_nHints & HINTS_DITHERFLAGS) == 0);
    m_nHints |= HINTS_DITHERFLAGS;

    return ret;
}

// <<Not properly cleaning up icons.>>
// <<Also, we're redithering which isn't great>>
#ifdef _WIN32
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
                VERIFY( hDC = ::GetWindowDC(::GetDesktopWindow()) );
		iconInfo.hbmColor = GetDDB(hDC);
                VERIFY( ::ReleaseDC(::GetDesktopWindow(), hDC) );
		m_hIcon = ::CreateIconIndirect(&iconInfo);
		VERIFY(::DeleteObject(hMask));

		// free resources.
		if (pMask) {
			delete [] pMask;
		}
	}
	return m_hIcon;
}

#else   // !WIN32

#pragma message("AwtImage::ToIcon(...) is unimplemented.")
HICON AwtImage::ToIcon() {
    return NULL;
}
#endif  // _WIN32



#ifdef DEBUG_IMAGE
void AwtImage::PrintImage()
{
   char buf[100];
   BYTE FAR *bp = m_pBitmapData;
   int scanLineSize = (m_nSrcW + 3) & ~0x3L;//ULONG Aligned.

   for(int i=0; i<m_nSrcH; i++)
   {
      bp=m_pBitmapData+(i*scanLineSize);
      for(int j=0; j<m_nSrcW; j++)
      {
        wsprintf(buf, "%d ", *bp++);
        OutputDebugString(buf);
      }
      wsprintf(buf, "\n");
      OutputDebugString(buf);
   }
}

void AwtImage::Print24BitImage()
{
   char buf[100];
   BYTE FAR *bp = m_pBitmapData;
   int scanLineSize = (m_nSrcW*3 + 3) & ~0x3;//ULONG Aligned.

   for(int i=0; i<m_nSrcH; i++)
   {
      bp=m_pBitmapData+(i*scanLineSize);
      for(int j=0; j<m_nSrcW; j++)
      {
        wsprintf(buf, "(%d %d %d) ", *bp++,*bp++,*bp++);
        OutputDebugString(buf);
      }
      wsprintf(buf, "\n");
      OutputDebugString(buf);
   }
}
#endif //DEBUG_IMAGE







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
//------------------------------------------------------------------------
// Notes: As per AWT api we would never be able to tell when the image is
// completely read in. This is because the image consumer method setBytePixels
// accepts rectangular destination co-ordinates, which means that any part
// of the image can be read in at any point of time! and also irh does not 
// provide a flag to indicate the end of image. 
// Due to this we would not be able to delete the DIB at a certain point 
// of time. Hence we keep the DIB around and recreate the DDB for every draw.
// Also, if we kept the DDB around it will take up GDI resources. This means
// for images like the moleculeviewer applet which creates atleast 40 small 
// images we will loose on GDI heavily. This happens in old Awt too.
// So, in order to save on a duplicate copy, save on GDI resources and be 
// portable accross Win-16, recreating the DDB and associated DCs is the 
// best bet.
//------------------------------------------------------------------------

void AwtImage::Draw(HDC hDC, long nX, long nY,
		    DWORD xormode, COLORREF xorcolor, HBRUSH xorBrush,
		    HColor  *c)
{
    HBITMAP hBitmap     = NULL;
    HBITMAP hOldBmpB    = NULL;
    HBITMAP hOldBmpM    = NULL;
    UINT    result      = 0;


    VERIFY( m_pBitmapData );
    //PrintImage();
    //Print24BitImage();

    //
    // Select and Realize the image's palette into the DC if one
    // is available...
    //
    if (m_hPal != NULL) {
        VERIFY( ::SelectPalette(hDC, m_hPal, TRUE) );
        //VERIFY( ::SelectPalette(hDC, m_hPal, FALSE) );
        result = ::RealizePalette(hDC);
    }

    m_hBMPDC = ::CreateCompatibleDC(hDC);
    VERIFY( m_hBMPDC );

    if(m_hBitmap == NULL) {
        m_hBitmap = ::CreateCompatibleBitmap(hDC, GetWidth(), GetHeight());
        VERIFY( m_hBitmap );
    }

    if( (m_hBitmap != NULL) &&(m_pBitmapData != NULL) ) {
        ::SetDIBits(hDC, m_hBitmap, 0, GetHeight(), m_pBitmapData, 
                    m_pBitmapInfo, DIB_RGB_COLORS );
    }

    m_hMaskDC = ::CreateCompatibleDC(hDC);
    VERIFY( m_hMaskDC );

    if( (m_hTransparencyMask == NULL) && (m_pTransparencyData != NULL) ) {
        m_hTransparencyMask = ::CreateCompatibleBitmap(hDC, GetWidth(), GetHeight());
        VERIFY( m_hTransparencyMask );
    }

    if( (m_hTransparencyMask != NULL) && (m_pTransparencyData != NULL) ) {
        ::SetDIBits(hDC, m_hTransparencyMask, 0, GetHeight(), m_pTransparencyData, 
                    m_pTransMaskInfo , DIB_RGB_COLORS );
    }



    hBitmap = GetDDB(hDC);
    VERIFY( hBitmap );

    if (m_hTransparencyMask) {
        hOldBmpB = (HBITMAP)::SelectObject(m_hBMPDC, hBitmap);
        /*///////////////
            VERIFY( ::BitBlt(hDC, 0, 0, m_nDstW, m_nLastLine,
                             m_hBMPDC, 0, 0, SRCCOPY) );
            ::GdiFlush();
        *////////////////

        hOldBmpM = (HBITMAP)::SelectObject(m_hMaskDC, m_hTransparencyMask);
        /*///////////////
            VERIFY( ::BitBlt(hDC, 0, 0, m_nDstW, m_nLastLine,
                             m_hMaskDC, 0, 0, SRCCOPY) );
            ::GdiFlush();
        *////////////////

        COLORREF color;
        HBRUSH hBrush;
        DWORD rop;

        if (xormode) {
            hBrush = xorBrush;
            color  = xorcolor;
            rop    = 0x00e20000L;
        } 
        else if (c == 0) {
            hBrush = NULL;
            color  = PALETTERGB(0, 0, 0);
            rop    = 0x00220000L;
        } 
        else {
            DWORD javaColor = c->obj->value;

            hBrush = NULL;
            color  = PALETTERGB((javaColor >> 16L) & 0xff,
                                (javaColor >> 8L)  & 0xff,
                                (javaColor)        & 0xff);
            rop    = 0x00e20000L;
        }


///     if (color != m_bgcolor) 
	{
            HBRUSH hOldBrush = NULL;

            if (!xormode && c != 0) {
                hBrush = ::CreateSolidBrush(color);
            }

            if (hBrush != NULL) {
                hOldBrush = (HBRUSH) ::SelectObject(m_hBMPDC, hBrush);
            }

            VERIFY( ::BitBlt(m_hBMPDC, 0, 0, m_nDstW, m_nLastLine,
                             m_hMaskDC, 0, 0, rop) );
            /*///////////////
                VERIFY( ::BitBlt(hDC, 0, 0, m_nDstW, m_nLastLine,
                                 m_hBMPDC, 0, 0, SRCCOPY) );
                ::GdiFlush();
            *////////////////

            if (hBrush != NULL) {
                VERIFY( ::SelectObject(m_hBMPDC, hOldBrush) );
            }

            if (!xormode && c != 0) {
                VERIFY(::DeleteObject(hBrush));
            }
            m_bgcolor = color;
        }

        if (xormode || c != 0) {
            VERIFY( ::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
                             m_hBMPDC, 0, 0,
                             xormode ? 0x00960000L : SRCCOPY) );
        } else {
            VERIFY( ::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
                             m_hMaskDC, 0, 0, SRCAND) );

            //::GdiFlush();

            VERIFY( ::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
                             m_hBMPDC, 0, 0, SRCPAINT) );
            //::GdiFlush();
        }

    } 
    //
    // No transparency mask...
    //
    else {
        int hasRgn;
        HRGN hR  = NULL;

        if (m_hRgn != NULL) {

#ifdef _WIN32
            hR = ::CreateRectRgn(0, 0, 0, 0);
            hasRgn = ::GetClipRgn(hDC, hR);
#else   // !_WIN32
            RECT rcClip;

            hasRgn = ::GetClipBox(hDC, &rcClip);
            hR     = ::CreateRectRgn(rcClip.left, rcClip.top,
                                     rcClip.right, rcClip.bottom);
            hasRgn = ((hasRgn == SIMPLEREGION) || (hasRgn == COMPLEXREGION));
#endif  // !_WIN32

            VERIFY( ::OffsetClipRgn(hDC, -nX, -nY) != ERROR );

#ifdef _WIN32
            VERIFY( ::ExtSelectClipRgn(hDC, m_hRgn, RGN_AND) != ERROR );
#else
            //REMIND: What do we do here? There is no such call in win16
            ASSERT(0);
#endif
            VERIFY(::OffsetClipRgn(hDC, nX, nY) != ERROR);
        }

        hOldBmpB = (HBITMAP)::SelectObject(m_hBMPDC, hBitmap);
        VERIFY( ::BitBlt(hDC, nX, nY, m_nDstW, m_nLastLine,
                         m_hBMPDC, 0, 0,
                         xormode ? 0x00960000L : SRCCOPY) );
        //::GdiFlush();
        /*
        if (m_hRgn != NULL) {
            if (hasRgn) {
                VERIFY( ::SelectClipRgn(hDC, hR) != ERROR );
            } else {
                VERIFY( ::SelectClipRgn(hDC, NULL) != ERROR );
            }
        }
        */

        if (hR != NULL) {
            VERIFY( ::SelectClipRgn(hDC, NULL) != ERROR );
            VERIFY( ::DeleteObject(hR) );
        }

    }

    //
    // Finished drawing... Now clean everything up...
    //
    if (hOldBmpB != NULL) {
        VERIFY( ::SelectObject(m_hBMPDC, hOldBmpB) );
    }

    if (hOldBmpM != NULL) {
        VERIFY(::SelectObject(m_hMaskDC, hOldBmpM));
    }

    if (m_hBitmap != NULL) {
        VERIFY(::DeleteObject(m_hBitmap));
        m_hBitmap  = NULL;
    }

    if (m_hTransparencyMask != NULL) { 
        VERIFY(::DeleteObject(m_hTransparencyMask));
        m_hTransparencyMask = NULL;
    }

    if (m_hBMPDC != NULL) {
        VERIFY( ::DeleteDC(m_hBMPDC) );
        m_hBMPDC = NULL;
    }

    if (m_hMaskDC != NULL) {
        VERIFY( ::DeleteDC(m_hMaskDC) );
        m_hMaskDC = NULL;
    }
}



#define SET_BYTE_PIXELS_SIG	"(IIIILjava/awt/image/ColorModel;[BII)V"

#define HINTS_OFFSCREENSEND (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |  \
			     java_awt_image_ImageConsumer_COMPLETESCANLINES | \
			     java_awt_image_ImageConsumer_SINGLEPASS)


void AwtImage::sendPixels(OffScreenImageSource *osis)
{
    HImageConsumer *ich = osis->theConsumer;
    struct execenv *ee;
    struct Hjava_awt_image_ColorModel *cm;
    HObject *pixh;
    char *buf;
    int32_t i, j=0;
    HDC hDC;

    ee = EE();
    if (osis->theConsumer == 0) {
	return;
    }
    /*
    cm = getColorModel();
    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				"setColorModel",
				"(Ljava/awt/image/ColorModel;)V",
				cm);
    */


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
    int32_t nPaddedWidth = (m_nDstW + 3L) & (~3L);
    pixh = (HObject *) ArrayAlloc(T_BYTE, nPaddedWidth);
    buf = (char *) unhand(pixh);
    VERIFY( hDC = ::GetWindowDC(::GetDesktopWindow()) );
    BITMAPINFO *pBitmapInfo = (BITMAPINFO*) malloc(sizeof(BITMAPINFO)
						   + 256*sizeof(RGBQUAD));
    pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBitmapInfo->bmiHeader.biWidth = m_nDstW; 
    pBitmapInfo->bmiHeader.biHeight = m_nDstH;
    pBitmapInfo->bmiHeader.biPlanes = 1;
    pBitmapInfo->bmiHeader.biBitCount = 8;
    pBitmapInfo->bmiHeader.biCompression = BI_RGB;
    pBitmapInfo->bmiHeader.biSizeImage = nPaddedWidth * m_nDstH;
    pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
    pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
    pBitmapInfo->bmiHeader.biClrUsed = 256;
    pBitmapInfo->bmiHeader.biClrImportant = 256;
    //BYTE FAR *pTemp = new BYTE[nPaddedWidth * pBitmapInfo->bmiHeader.biHeight];


    HGLOBAL   hDIB =
    GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
     (long)pBitmapInfo->bmiHeader.biHeight * nPaddedWidth );
    CHECK_NULL(hDIB, "OutOfMemoryError");
    BYTE FAR *pTemp = (BYTE  FAR *)GlobalLock(hDIB);

#ifdef _WIN32
    pBitmapInfo->bmiHeader.biHeight = -pBitmapInfo->bmiHeader.biHeight;
#else
    pBitmapInfo->bmiHeader.biHeight = pBitmapInfo->bmiHeader.biHeight;
#endif
    HBITMAP hBitmap = NULL;
    if(  (m_hBitmap     == NULL) 
       &&(m_pBitmapData != NULL)
      )
    {
        // This is the case when the DIB is filled and our bitmap
        // will be created and destroyed after each draw.
        // We do not use the DIB as is because the original DIB may
        // be either 24 bit or 8 bits. But this routine will return 
        // 8 bit only. 
        hBitmap = ::CreateCompatibleBitmap(hDC, GetWidth(), GetHeight());
        VERIFY( hBitmap );

        if( (hBitmap != NULL) &&(m_pBitmapData != NULL) ) {
            ::SetDIBits(hDC, hBitmap, 0, GetHeight(), m_pBitmapData, 
                        m_pBitmapInfo, DIB_RGB_COLORS );
        }
    }
    else
    {
       hBitmap =  m_hBitmap;
    }


    VERIFY(::GetDIBits(hDC, hBitmap, 0, m_nDstH, pTemp,
		       pBitmapInfo, DIB_RGB_COLORS));

    if(  (hBitmap   != NULL)
       &&(m_hBitmap != hBitmap)
      )
    {
       VERIFY(::DeleteObject(hBitmap));
       hBitmap  = NULL;
    }

    m_pRGBQuad  = (RGBQUAD*)((BYTE*)pBitmapInfo + sizeof(BITMAPINFOHEADER));    
    cm = getColorModel(m_pRGBQuad);
    execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				"setColorModel",
				"(Ljava/awt/image/ColorModel;)V",
				cm);
    
    
#ifdef _WIN32
    for (i = 0; i < m_nDstH; i++) 
#else
    for (i = (m_nDstH-1); i >=0 ; i--) 
#endif
    {
	      if (osis->theConsumer == 0) 
       {
	          break;
	      }
	      memcpy(buf, &pTemp[i * nPaddedWidth], m_nDstW);
	      execute_java_dynamic_method(ee, (HObject *) osis->theConsumer,
				    "setPixels",
				    SET_BYTE_PIXELS_SIG,
				    0, j, m_nDstW, 1,
				    cm, pixh, 0, nPaddedWidth);
	       if (exceptionOccurred(ee)) 
        {
	           break;
	       }
       j++;
    }
    //delete [] pTemp;
    GlobalFree(hDIB);
    VERIFY( ::ReleaseDC(::GetDesktopWindow(), hDC) );
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


#define SetRGBQUAD(p, r, g, b)	\
    {				\
	(p)->rgbRed      = r;	\
	(p)->rgbGreen    = g;	\
	(p)->rgbBlue     = b;	\
	(p)->rgbReserved = 0;	\
    }


void AwtImage::DisposePalette()
{
    if (m_pBitmapInfo) {
        free(m_pBitmapInfo);
        m_pBitmapInfo = NULL;
    }

    if (m_pTransMaskInfo) {
        free(m_pTransMaskInfo);
        m_pTransMaskInfo = NULL;
    }

    if (m_pAwtPalette != NULL) {
        m_pAwtPalette->Release();
        m_pAwtPalette = NULL;
    }   

    if (g_colorInfo[COLOR4].m_pPaletteColors) {
        free(g_colorInfo[COLOR4].m_pPaletteColors);
        g_colorInfo[COLOR4].m_pPaletteColors = NULL;
    }
}


struct Hjava_awt_image_ColorModel *AwtImage::getColorModel(RGBQUAD *pRGBQuad)
{
    struct Hjava_awt_image_ColorModel *awt_colormodel;
    HArrayOfByte *hR = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
    HArrayOfByte *hG = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
    HArrayOfByte *hB = (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
    char *aBase, *a;
    RGBQUAD *c, *cBase;
    
    cBase = pRGBQuad;
    //cBase = g_colorInfo[1].m_pPaletteColors;
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

#define STATIC_COLOR_OFFSET 10


// -----------------------------------------------------------------------
// Copy the image into a DIB with origin at lower left hand corner.
// This handles the IndexColorModel case.
// -----------------------------------------------------------------------
long
AwtImage::copyImagePartInto8BitBuffer(
			      int bgcolor, int srcX, int srcY,
			      int srcW, int srcH, BYTE *srcpix,
			      int srcOff)
{
    int32_t scanLineSize = (m_nSrcW + 3) & ~0x3L;//ULONG Aligned.
    BYTE FAR *srcStartByte = (BYTE FAR *)srcpix+srcOff;
    BYTE FAR *dstStartLine = m_pBitmapData
                          +((m_nSrcH-srcY-1)*scanLineSize); 
   BYTE FAR *dstStartByte = dstStartLine+srcX;
   CHECK_BOUNDARY_CONDITION_RETURN(( (dstStartByte+srcW) > (dstStartLine+scanLineSize) ),"Destination buffer overshoot", AWT_IMAGE_COPY_FAILURE);

   for(int i=0; i<srcH; i++)
   {
   /*
#ifdef  _WIN32
     memcpy(dstStartByte, srcStartByte,  srcW);
#else
     _fmemccpy( dstStartByte, srcStartByte, 0, srcW);
#endif
*/
      for(int j=0; j<srcW; j++)
      {
         *dstStartByte++ = (BYTE)((*srcStartByte++)+STATIC_COLOR_OFFSET);
      }
     //dstStartLine-=scanLineSize;
     //dstStartByte = dstStartLine+srcX;
     dstStartByte = (dstStartLine-=scanLineSize)+srcX; 
   }
   return AWT_IMAGE_COPY_SUCCESS;
}


// -----------------------------------------------------------------------
//
// Copy the image into a DIB with origin at lower left hand corner.
// The first cut assumes a dcm will have a default masks set.
// This handles the DirectColorModel case.
// REMIND: Try getRed from Java colormodel once this works and the cm is 
//         different from index or direct.
// -----------------------------------------------------------------------

long
AwtImage::copyImagePartInto24BitBuffer(
			      int bgcolor, int srcX, int srcY,
			      int srcW, int srcH, int *srcpix,
			      int srcOff)
{
   int scanLineSize = (m_nSrcW*3 + 3) & ~0x3L;//ULONG Aligned.
   int FAR *srcStartInt = (int FAR *)srcpix+srcOff;
   BYTE FAR *dstStartLine = m_pBitmapData
                          +((m_nSrcH-srcY-1)*scanLineSize); 
   BYTE FAR *dstStartByte = dstStartLine+(srcX*3);
   int pixel = 0;
   CHECK_BOUNDARY_CONDITION_RETURN(( (dstStartByte+(srcW*3)) > (dstStartLine+scanLineSize) ),"Destination buffer overshoot", 0);
   for(int i=0; i<srcH; i++)
   {
      for(int j=0; j<srcW; j++)
      {
         *dstStartByte++ = (BYTE)(GETBLUE (*srcStartInt));
         *dstStartByte++ = (BYTE)(GETGREEN(*srcStartInt));
         *dstStartByte++ = (BYTE)(GETRED  (*srcStartInt++));
      }
      dstStartByte = (dstStartLine-=scanLineSize)+(srcX*3); 
   }
   m_nLastLine += srcH;
   return AWT_IMAGE_COPY_SUCCESS;
}


// API from Java side.


extern "C" {
void
sun_awt_image_ImageRepresentation_offscreenInit(HImageRepresentation *irh)
{
    CHECK_NULL(irh, "ImageRepresentation is null.");

    ImageRepresentation * ir = unhand(irh);
    CHECK_BOUNDARY_CONDITION((ir->width <= 0 || ir->height <= 0),"IllegalArgumentException");


    AwtImage *pImage = new AwtImage;
    CHECK_NULL(pImage, "OutOfMemoryError");
    if (!pImage->Create(irh, ir->width, ir->height)) 
    {
	      SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	      delete pImage;
    } 
    else 
    {
	     unhand(irh)->pData = (long)pImage;
	     unhand(irh)->availinfo = IMAGE_OFFSCRNINFO;
    }
}

long
sun_awt_image_ImageRepresentation_setBytePixels(HImageRepresentation *irh,
						long x, long y,
						long w, long h,
						HColorModel *cmh, HArrayOfByte *arrh,
						long off, long scansize)
{
    CHECK_BOUNDARY_CONDITION_RETURN((irh == 0 || cmh == 0 || arrh == 0),"NullPointerException", -1);
    //CHECK_BOUNDARY_CONDITION_RETURN((w <= 0 || h <= 0),"IllegalArgumentException", -1);
    if((w <= 0 || h <= 0))
    {
       return -1;
    }
    CHECK_BOUNDARY_CONDITION_RETURN(((long)obj_length(arrh) < (scansize * h)),"ArrayIndexOutOfBoundsException", -1);

    long ret=0;
    AwtImage *pImage = (AwtImage *) unhand(irh)->pData;
    if (pImage == NULL &&
	        (unhand(irh)->availinfo & IMAGE_SIZEINFO) == IMAGE_SIZEINFO)
    {
		pImage = new AwtImage();
		pImage->Init(irh, cmh); // Will init m_pAwtPalette
		unhand(irh)->pData = (long) pImage;
    }

    CHECK_NULL_AND_RETURN(pImage, "OutOfMemoryError", ret);
    ret = pImage->GenericImageConvert(cmh, 0, x, y, w, h,
					   unhand(arrh), off, 8, scansize,
					   pImage->GetSrcWidth(),
					   pImage->GetSrcHeight(),
					   pImage->GetWidth(),
					   pImage->GetHeight(),
					   pImage);
    return ret;
}

long
sun_awt_image_ImageRepresentation_setIntPixels(HImageRepresentation * irh,
					       long x, long y, long w, long h,
					       HColorModel * cmh, HArrayOfInt  *arrh,
					       long off, long scansize)
{
    CHECK_BOUNDARY_CONDITION_RETURN((irh == 0 || cmh == 0 || arrh == 0),"NullPointerException", -1);
    //CHECK_BOUNDARY_CONDITION_RETURN((w <= 0 || h <= 0),"IllegalArgumentException", -1);
    if((w <= 0 || h <= 0))
    {
       return -1;
    }
    CHECK_BOUNDARY_CONDITION_RETURN(((long)obj_length(arrh) < (scansize * h)),"ArrayIndexOutOfBoundsException", -1);

    long ret=0;
    AwtImage *pImage = (AwtImage *) unhand(irh)->pData;
    if (pImage == NULL &&
	        (unhand(irh)->availinfo & IMAGE_SIZEINFO) == IMAGE_SIZEINFO)
    {
	      pImage = new AwtImage();
	      pImage->Init(irh, cmh);
       /*
       if( pImage->m_pAwtPalette != NULL )
       {
          switch (pImage->m_pAwtPalette->GetBitsPerPixel())
          {
             // Right now we support only 24 bit RGB input.
             case 24:
             {
               // Never called as this case is handled by 
               // the setIntPixels. But may be used later if 
               // we send continuous byte streams even for 24 bit.
        	      pImage->Alloc24BitConsumerBuffer();
               break;
             }
             default :
               break;
          }
       }
       */
	      unhand(irh)->pData = (long) pImage;
    }

    CHECK_NULL_AND_RETURN(pImage, "OutOfMemoryError", -1);
    ret = pImage->GenericImageConvert(cmh, 0, x, y, w, h,
					   unhand(arrh), off, 32, scansize,
					   pImage->GetSrcWidth(),
					   pImage->GetSrcHeight(),
					   pImage->GetWidth(),
					   pImage->GetHeight(),
					   pImage);
    return ret;
}

long
sun_awt_image_ImageRepresentation_finish(HImageRepresentation * irh, long force)
{
    int ret = 0;
    CHECK_NULL_AND_RETURN(irh, "NullPointerException", ret);
    AwtImage *pImage = (AwtImage *) unhand(irh)->pData;

    if (pImage != 0) 
    {
     	 ret = pImage->freeRenderData(force);
    }
    return ret;
}

void
sun_awt_image_ImageRepresentation_imageDraw(HImageRepresentation * irh, HGraphics *gh,
					    long x, long y,
					    HColor *c)
{
    HWGraphics *self;
    CHECK_BOUNDARY_CONDITION((irh == 0 || gh == 0),"NullPointerException");
    self = (HWGraphics *)gh;
    ImageRepresentation *ir = unhand(irh);
    //CHECK_BOUNDARY_CONDITION(((ir->availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO),"NullPointerException");

    AwtImage *pImage = (AwtImage *)ir->pData;
    AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	if (pGraphics != NULL && pImage != NULL) 
	{
		pGraphics->DrawImage(pImage, x, y, c);
	}
}

void
sun_awt_image_ImageRepresentation_disposeImage(HImageRepresentation * irh)
{
//REMIND: This is never called. from any peers.!!

    CHECK_NULL(irh, "NullPointerException");
    AwtImage *pImage = (AwtImage *) unhand(irh)->pData;
    if (pImage != NULL) 
    {
	      delete pImage;
	      unhand(irh)->pData = NULL;
    }
}

void
sun_awt_image_OffScreenImageSource_sendPixels(HOffScreenImageSource *osish)
{
    OffScreenImageSource * osis = unhand(osish);
    HImageRepresentation * irh = osis->baseIR;
    ImageRepresentation * ir;

    CHECK_NULL(irh, "NullPointerException");
    ir = unhand(irh);

    CHECK_BOUNDARY_CONDITION(((ir->availinfo & IMAGE_OFFSCRNINFO) != IMAGE_OFFSCRNINFO),"IllegalAccessError");
    AwtImage *pImage = (AwtImage *) unhand(irh)->pData;
    if (pImage == NULL &&
	        (unhand(irh)->availinfo & IMAGE_SIZEINFO) == IMAGE_SIZEINFO)
    {
	      pImage = new AwtImage();
	      pImage->Init(irh, NULL);
	      unhand(irh)->pData = (long) pImage;
    }
    CHECK_NULL(pImage, "OutOfMemoryError");
    pImage->sendPixels(osis);
}

};  // end of extern "C"
