#include <limits.h>
#include <math.h>

#include "awt_graphics.h"

#include "awt_component.h"
#include "awt_image.h"
#include "awt_dc.h"
#include "awt_font.h"
#include "awt_toolkit.h"

#if !defined(_AWT_IDCSERVICE_H)
#include "awt_IDCService.h"
#endif

// All java header files are extern "C"
extern "C" {
#include "java_awt_Font.h"
#include "java_awt_Color.h"
#include "java_awt_Rectangle.h"
#include "java_awt_Graphics.h"

#include "sun_awt_image_ImageRepresentation.h"
#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WGraphics.h"
#include "java_awt_image_ImageObserver.h"
};


//#define DEBUG_GDI


// Does origin translation
#define OX              ((int)(unhand((HWGraphics *)m_pJavaPeer)->originX))
#define OY              ((int)(unhand((HWGraphics *)m_pJavaPeer)->originY) )
#define TX(x)           ((int)((x) + OX))  
#define TY(y)           ((int)((y) + OY))


//------------------------------------------------------------------------
//
// Graphics constructor. Build a graphic object for a component
//
//------------------------------------------------------------------------
AwtGraphics::AwtGraphics(HWGraphics *self, HWComponentPeer *canvas) : AwtObject((JHandle *)self)
{
    DEFINE_AWT_SENTINAL("GRA");

    // assign yourself to the peer object
    unhand(self)->pData = (long)this;
    
    AwtComponent* pComp = GET_OBJ_FROM_PEER(AwtComponent, canvas);

    // assign yourself a DC
    if (pComp) {
        m_pAwtDC = AwtDC::Create((IDCService*) pComp);

        //set the clipping region to be the size of the client window (if any)
        ::SetRect(&m_ClipRect, 0, 0, 0, 0);
        HWND hOwnerWnd = ((AwtComponent*)pComp)->GetWindowHandle();
        if (hOwnerWnd) 
        {
            ::GetClientRect(hOwnerWnd, &m_ClipRect);
        }
        if( IsRectEmpty(&m_ClipRect) )
        {
           //::SetRect(&m_ClipRect, LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);
           //SetRectRgn() has problems with LONG_MIN, LONG_MAX
            ::SetRect(&m_ClipRect, 0, 0, 16383, 16383);
        }
    }
    else 
        m_pAwtDC = 0;


    // assign default values
    m_pFont     = 0;
    m_pBrush    = AwtCachedBrush::Create(RGB(255, 255, 255));
    m_pPen      = AwtCachedPen::Create(RGB(0, 0, 0));
    // get the palette
    //m_pPalette  = NULL;
    //!DR watch out
    m_pPalette  = AwtToolkit::GetToolkitDefaultPalette(); //REMIND : get the actual AwtPalette from Toolkit.
    m_pPalette->AddRef();

    m_pImage    = 0;

    m_color     = RGB(0, 0, 0);
    m_xorcolor  = RGB(255, 255, 255);
    m_rop       = R2_COPYPEN;
    m_pImage    = NULL;

#ifdef DEBUG_GDI
    char buf[100];
    wsprintf(buf, "Create pGraphics = 0x%x, thrid=%d\n",m_hObject, PR_GetCurrentThreadID() );
    OutputDebugString(buf);
#endif
}


//------------------------------------------------------------------------
//
// Graphics constructor. Build a graphic object for an image
//
//------------------------------------------------------------------------
AwtGraphics::AwtGraphics(HWGraphics *self, HImageRepresentation  *irh) : AwtObject((JHandle *)self)
{
    DEFINE_AWT_SENTINAL("GRA");

    // assign yoursel to the java peer
    unhand(self)->pData = (long)this;

    // the image must exist
    AwtImage *pFromImage = (AwtImage *) unhand(irh)->pData;
    CHECK_NULL(pFromImage, "Null image to create graphics for");
    
    // Setup defaults.
    m_pFont     = 0;
    m_pPen      = AwtCachedPen::Create(RGB(255, 0, 0));
    m_pBrush    = AwtCachedBrush::Create(RGB(255, 255, 255));
    // watch out palette reference counting
    //m_pPalette  = NULL;
    //!DR watch out
    m_pPalette  = AwtToolkit::GetToolkitDefaultPalette(); 
    m_pPalette->AddRef();

    m_color     = RGB(0, 0, 0);
    m_rop       = R2_COPYPEN;
    m_xorcolor  = RGB(255, 255, 255);


    if(pFromImage != NULL) 
    {
        ::SetRect(&m_ClipRect, 0, 0, pFromImage->GetWidth(), pFromImage->GetHeight());

        m_pImage = pFromImage;

        // Note that the following DC will be the same as the one
        // allocated in BOOL AwtImage::Create(HImageRepresentation *pJavaObject,
        // long nW, long nH) with a added reference count.
        m_pAwtDC = AwtDC::Create((IDCService*) pFromImage);
    } 
    else 
    {
        //REMIND: When is the case when a G is created without a component?
        m_pAwtDC = 0;
        m_pImage = 0;

        //::SetRect(&m_ClipRect, LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);
        ::SetRect(&m_ClipRect, 0, 0, 16383, 16383);
    }
}


//-------------------------------------------------------------------------
//
// Destructor and dispose. Free resources
//
//-------------------------------------------------------------------------
AwtGraphics::~AwtGraphics()
{
    if (m_pAwtDC != NULL) 
        m_pAwtDC->Release();

    if (m_pFont != NULL) 
        m_pFont->Release();
    
    if (m_pBrush != NULL) 
        m_pBrush->Release();
	
    if (m_pPen != NULL) 
        m_pPen->Release();

    if(m_pPalette != NULL)
        m_pPalette->Release();

    if (m_pJavaPeer !=  NULL) { 
        unhand((HWGraphics *)m_pJavaPeer)->pData = NULL;
        m_pJavaPeer =  NULL;
    }
    
#ifdef DEBUG_GDI
    char buf[100];
    wsprintf(buf, "Destroy pGraphics  = 0x%x, thrid=%d\n",m_hObject, PR_GetCurrentThreadID() );
    OutputDebugString(buf);
#endif
}


//-------------------------------------------------------------------------
//
// Called after the copy constructor from the stub to update some basic info
//
//-------------------------------------------------------------------------
void AwtGraphics::InitCopy(HWGraphics *self)
{
    m_pJavaPeer = (JHandle *)self;
    unhand(self)->pData = (long)this;

    // for any cached resources AddRef to keep counter right
    if (m_pAwtDC != NULL) 
        m_pAwtDC->AddRef();

    if (m_pFont != NULL) 
        m_pFont->AddRef();
    
    if (m_pBrush != NULL) 
        m_pBrush->AddRef();
	
    if (m_pPen != NULL) 
        m_pPen->AddRef();

    if(m_pPalette != NULL)
        m_pPalette->AddRef();
}
                         

//-------------------------------------------------------------------------
//
// Update font. Find the native font from the java one
//
//-------------------------------------------------------------------------
void AwtGraphics::SetFont(Hjava_awt_Font *font)
{
    //AwtFont *pFont = AwtFont::FindFont(font);
    AwtFont * pFont = AwtFont::GetFontFromObject(font);
    pFont->AddRef();

    // select the font in the dc
    if (pFont && m_pAwtDC) {
        m_pAwtDC->SetFont(this, pFont);
    }

    if (m_pFont) {
        m_pFont->Release();
    }
    
    m_pFont = pFont;
}


//-------------------------------------------------------------------------
//
// Calculates the appropriate new COLORREF according to the
// current ROP and calls ChangeColor on the new COLORREF
//
//-------------------------------------------------------------------------
void AwtGraphics::SetColor(COLORREF color)
{
    ChangeColor( (m_rop == R2_XORPEN) ? XorResult(color, m_xorcolor) : color );
}


//-------------------------------------------------------------------------
//
// change the color for pen, brush and text
//
//-------------------------------------------------------------------------
void AwtGraphics::ChangeColor(COLORREF color)
{
    if (m_color != color) 
    {
        AwtCachedPen* pPen; 
        AwtCachedBrush* pBrush; 

        VERIFY(pPen = AwtCachedPen::Create(color));
        VERIFY(pBrush = AwtCachedBrush::Create(color));

        // notify DC
        if( m_pAwtDC != NULL)
        {
            m_pAwtDC->SetPen(this, pPen);
            m_pAwtDC->SetBrush(this, pBrush);
            m_pAwtDC->SetTxtColor(this, color);
        }

        // release old pen
        if (m_pPen) {
            m_pPen->Release();
        }

        // release old brush
        if (m_pBrush) {
            m_pBrush->Release();
        }

        // assign new values
        m_pPen   = pPen;
        m_pBrush = pBrush;
        m_color = color;
    }
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
COLORREF AwtGraphics::XorResult(COLORREF fgcolor, COLORREF xorcolor) 
{
    COLORREF c;
   	//c = (fgcolor & 0xff000000) | ((fgcolor ^ xorcolor) & 0x00ffffff);
    if (m_pPalette->GetBitsPerPixel() > 8) {
        c = (fgcolor & 0xff000000L) | ((fgcolor ^ xorcolor) & 0x00ffffffL);
    } else {
        c = PALETTEINDEX(GetNearestPaletteIndex(m_pPalette->GetPalette(), fgcolor) ^
                         GetNearestPaletteIndex(m_pPalette->GetPalette(), xorcolor));
    }
    return c;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtGraphics::SetRop(DWORD rop, COLORREF fgcolor, COLORREF color)
{
    if (m_rop != rop) {
       	m_rop = rop;
    }
    
    if (m_rop == R2_XORPEN) {
        ChangeColor(XorResult(fgcolor, color));
        m_xorcolor = color;
    } else {
        ChangeColor(fgcolor);
    }

    if( m_pAwtDC != NULL) {
        m_pAwtDC->SetRop(this, m_rop);
    }
}


//-------------------------------------------------------------------------
//
// set the clip rect for this graphic
//
//-------------------------------------------------------------------------
void AwtGraphics::SetClipRect(long nX, long nY, long nW, long nH)
{
    int x1, y1, x2, y2;
    x1 = TX(nX);
    y1 = TY(nY);
    if (nW <= 0 || nH <= 0) {
        x2 = x1;
        y2 = y1;
    } else {
        x2 = x1 + nW;
        y2 = y1 + nH;
    }
    /*
    m_ClipRect.left   = x1;
    m_ClipRect.top    = y1;
    m_ClipRect.right  = x2;
    m_ClipRect.bottom = y2;
    */
    //{
    RECT newRect;
    newRect.left   = x1;
    newRect.top    = y1;
    newRect.right  = x2;
    newRect.bottom = y2;

    ::IntersectRect(&m_ClipRect, &m_ClipRect, &newRect);
    //}

    // we always push a clipping region on lock so there is no reason
    // to push it here
    //if( m_pAwtDC != NULL)
    //{
    //    m_pAwtDC->SetRgn(this, &m_ClipRect);
    //}
}
 

//-------------------------------------------------------------------------
//
// return the clipping region
//
//-------------------------------------------------------------------------
void AwtGraphics::GetClipRect(RECT *rect)
{
    rect->left   = (m_ClipRect.left   - OX);
    rect->top    = (m_ClipRect.top    - OY);
    rect->right  = (m_ClipRect.right  - OX);
    rect->bottom = (m_ClipRect.bottom - OY);
}


//-------------------------------------------------------------------------
//
// Draw a line from point (nX1, NY1) to point (nX2 NY2)
//
//-------------------------------------------------------------------------
void AwtGraphics::DrawLine(long nX1, long nY1, long nX2, long nY2)
{
    HDC hDC = m_pAwtDC->Lock(this);

    if (hDC != NULL) {
        VERIFY(::MoveToEx(hDC, TX(nX1), TY(nY1), NULL));
        VERIFY(::LineTo(hDC, TX(nX2), TY(nY2)));
        // Paint the end-point.
        VERIFY(::LineTo(hDC, TX(nX2+1), TY(nY2)));
    }

    m_pAwtDC->Unlock(this);
}


//-------------------------------------------------------------------------
//
// Draw a polygon given an array of points
//
//-------------------------------------------------------------------------
void AwtGraphics::Polygon(POINT *pPoints, int nPoints)
{
    HDC hDC = m_pAwtDC->Lock(this);

    if (hDC != NULL) {
        // select the NULL_BRUSH
        m_pAwtDC->SetBrush(this, 0);

        int i;
        POINT *p = pPoints;

        // translate points
        for (i=0; i < nPoints; i++) {
            p->x = TX(p->x);
            p->y = TY(p->y);
            p++;
        }

        VERIFY(::Polyline(hDC, pPoints, nPoints));

        // select old brush back
        m_pAwtDC->SetBrush(this, m_pBrush);
    }
    
    m_pAwtDC->Unlock(this);
}


//-------------------------------------------------------------------------
//
// Draw a polygon given an array of points and fill it with the current brush
//
//-------------------------------------------------------------------------
void AwtGraphics::FillPolygon(POINT *pPoints, int nPoints)
{
    HDC hDC = m_pAwtDC->Lock(this);

    if (hDC != NULL) {
        int i;
        POINT *p = pPoints;

        for (i=0; i < nPoints; i++) {
            p->x = TX(p->x);
            p->y = TY(p->y);
            p++;
        }

        m_pAwtDC->SetPen(this, 0);

        VERIFY(::Polygon(hDC, pPoints, nPoints));

        m_pAwtDC->SetPen(this, m_pPen);
    }
    
    m_pAwtDC->Unlock(this);
}


//-------------------------------------------------------------------------
//
// Draw a rect 
//
//-------------------------------------------------------------------------
void AwtGraphics::DrawRect(long nX, long nY, long nW, long nH)
{
    // don't process empty rect
    if (nW >= 0 && nH >= 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        if (hDC != NULL) {
            // select the NULL_BRUSH in the DC
	        m_pAwtDC->SetBrush(this, 0);

	        VERIFY(::Rectangle(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));

            // select old brush back
            m_pAwtDC->SetBrush(this, m_pBrush);
        }

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw a rect and fill with the current brush
//
//-------------------------------------------------------------------------
void AwtGraphics::FillRect(long nX, long nY, long nW, long nH)
{		
    if (nW > 0 && nH > 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        if (hDC != NULL) {
            m_pAwtDC->SetPen(this, 0);

            VERIFY(::Rectangle(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
            
            m_pAwtDC->SetPen(this, m_pPen);
        }

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Clear the specifed rect and fill with the current component brush
//
//-------------------------------------------------------------------------
void AwtGraphics::ClearRect(long nX, long nY, long nW, long nH)
{
    if (nW > 0 && nH > 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        if (hDC != NULL) {
            // try to get the component background brush (what if we fail?)
            HBRUSH hBkgnd = m_pAwtDC->GetComponentBkgnd();

            if (hBkgnd != NULL) {
                RECT rect;
                rect.left   = TX(nX);
                rect.top    = TX(nY);
                rect.right  = TX(nX+nW+1);
                rect.bottom = TX(nY+nH+1);
                ::FillRect(hDC, &rect, hBkgnd);

            }
        }
                
        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw a round rectangle
//
//-------------------------------------------------------------------------
void AwtGraphics::RoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH)
{		
    if (nW >= 0 && nH >= 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        if (hDC != NULL) {
            // select the NULL_BRUSH into the DC
	        m_pAwtDC->SetBrush(this, 0);

            ::RoundRect(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1), nArcW, nArcH);

            // select old brush back
            m_pAwtDC->SetBrush(this, m_pBrush);
        }

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw a round rectangle and fill with the current brush
//
//-------------------------------------------------------------------------
void AwtGraphics::FillRoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH)
{		
    if (nW > 0 && nH > 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        m_pAwtDC->SetPen(this, 0);

        if (hDC != NULL) {
            ::RoundRect(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1), nArcW, nArcH);
        }

        m_pAwtDC->SetPen(this, m_pPen);

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw an ellipse 
//
//-------------------------------------------------------------------------
void AwtGraphics::Ellipse(long nX, long nY, long nW, long nH)
{		
    if (nW >= 0 && nH >= 0) {
        HDC hDC = m_pAwtDC->Lock(this);
    
        if (hDC != NULL) {
            // select the NULL_BRUSH
	        m_pAwtDC->SetBrush(this, 0);

            VERIFY(::Ellipse(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));

            // select old brush back
            m_pAwtDC->SetBrush(this, m_pBrush);
        }

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw an ellipse and fill with the current brush
//
//-------------------------------------------------------------------------
void AwtGraphics::FillEllipse(long nX, long nY, long nW, long nH)
{		
    if (nW > 0 && nH > 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        m_pAwtDC->SetPen(this, 0);

        if (hDC != NULL) {

            VERIFY(::Ellipse(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));

        }

        m_pAwtDC->SetPen(this, m_pPen);

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Converts an angle given in 1/64ths of a degree to a pair of coordinates
// that intersects the line produced by the angle.
//
//-------------------------------------------------------------------------
void AngleToCoord(long nAngle, long nW, long nH, long *nX, long *nY) 
{
	const double pi = 3.1415926535;
	const double toRadians = 2 * pi / 360;

	*nX = (long)(cos((double)nAngle * toRadians) * nW);
	*nY = -(long)(sin((double)nAngle * toRadians) * nH);
}


//-------------------------------------------------------------------------
//
// Draw an arc
//
//-------------------------------------------------------------------------
void AwtGraphics::Arc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle)
{		
    if (nW >= 0 && nH >= 0) {
        HDC hDC = m_pAwtDC->Lock(this);
    
        if (hDC != NULL) {
            long nSX, nSY, nDX, nDY;
            BOOL bEndAngleNeg = nEndAngle < 0;

            nEndAngle += nStartAngle;
            if (bEndAngleNeg) 
            {
                // swap angles
                long tmp = nStartAngle;
                nStartAngle = nEndAngle;
                nEndAngle = tmp;
            }

            AngleToCoord(nStartAngle, nW, nH, &nSX, &nSY);
            nSX += nX + nW/2;
            nSY += nY + nH/2;
            AngleToCoord(nEndAngle, nW, nH, &nDX, &nDY);
            nDX += nX + nW/2;
            nDY += nY + nH/2;

            // an arc is not filled
            VERIFY(::Arc(hDC, 
                            TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
	                        TX(nSX), TY(nSY), TX(nDX), TY(nDY)));
        }

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw a pie
//
//-------------------------------------------------------------------------
void AwtGraphics::FillArc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle)
{		
    if (nW > 0 && nH > 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        if (hDC != NULL) {

            long nSX, nSY, nDX, nDY;
            BOOL bEndAngleNeg = nEndAngle < 0;

            nEndAngle += nStartAngle;
            if (bEndAngleNeg) 
            {
                // swap angles
                long tmp = nStartAngle;
                nStartAngle = nEndAngle;
                nEndAngle = tmp;
            }

            AngleToCoord(nStartAngle, nW, nH, &nSX, &nSY);
            nSX += nX + nW/2;
            nSY += nY + nH/2;
            AngleToCoord(nEndAngle, nW, nH, &nDX, &nDY);
            nDX += nX + nW/2;
            nDY += nY + nH/2;

            m_pAwtDC->SetPen(this, 0);

            VERIFY(::Pie(hDC, 
                        TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
		                TX(nSX), TY(nSY), TX(nDX), TY(nDY)));

            m_pAwtDC->SetPen(this, m_pPen);
        }

        m_pAwtDC->Unlock(this);
    }
}


//-------------------------------------------------------------------------
//
// Draw a string
//
//-------------------------------------------------------------------------
#ifdef INTLUNICODE
void AwtGraphics::DrawString(WinCompStr *pzString, long nX, long nY)
#else  // INTLUNICODE
void AwtGraphics::DrawString(char *pzString, long nX, long nY)
#endif // INTLUNICODE
{
    HDC hDC = m_pAwtDC->Lock(this);

    if (hDC != NULL) {

#ifdef _WIN32_UNDEF
        if (m_rop == R2_XORPEN) 
        {
            BeginPath(hDC);
        }
#endif //_WIN32

#ifdef INTLUNICODE
       if(pzString != NULL)
           pzString->TextOutWidth(hDC, TX(nX), TY(nY), m_pFont);
#else  // INTLUNICODE
        VERIFY(::TextOut(hDC, 
                            TX(nX), 
                            TY(nY - (m_pFont ? m_pFont->GetFontAscent() : 0)), 
                            pzString, 
                            strlen(pzString)));
#endif // INTLUNICODE

#ifdef _WIN32_UNDEF
        if (m_rop == R2_XORPEN) {
            EndPath(hDC);
            FillPath(hDC);  
        }
#endif //_WIN32
    }

    m_pAwtDC->Unlock(this);
}


//-------------------------------------------------------------------------
//
// Draw a string and return the length
//
//-------------------------------------------------------------------------
#ifdef INTLUNICODE
long AwtGraphics::DrawStringWidth(WinCompStr *pzString, long nX, long nY)
#else  // INTLUNICODE
long AwtGraphics::DrawStringWidth(char *pzString, long nX, long nY)
#endif // INTLUNICODE
{
    SIZE size;

    HDC hDC = m_pAwtDC->Lock(this);

    // draw the string
    DrawString(pzString, nX, nY);

    if (hDC != NULL) {
#ifdef INTLUNICODE
       if(pzString != NULL)
           pzString->GetTextExtent(hDC, m_pFont, &size);
#else  // INTLUNICODE
        // get the string length
#ifdef _WIN32_UNDEF
        VERIFY(::GetTextExtentPoint32(hDC, pzString, strlen(pzString), &size));
#else  //_WIN32
        VERIFY(::GetTextExtentPoint(hDC, pzString, strlen(pzString), &size));
#endif //_WIN32
#endif // INTLUNICODE
    }

    m_pAwtDC->Unlock(this);

    return size.cx;
}


//-------------------------------------------------------------------------
//
// Draw an image
//
//-------------------------------------------------------------------------
void AwtGraphics::DrawImage(AwtImage *pImage, long nX, long nY, Hjava_awt_Color *c)
{
    HBRUSH hOldBrush = NULL;
    HBRUSH hBrush    = NULL;
    HDC    hDC       = m_pAwtDC->Lock(this);

    if (hDC != NULL) {
	if (m_rop == R2_XORPEN) {
	    hBrush    = ::CreateSolidBrush(m_xorcolor);
	    hOldBrush = (HBRUSH) ::SelectObject(hDC, hBrush);
	}

	// Check to see if there is a shared DC where graphics operations
	// have been done. This is the only way of distinguishing a image with 
	// graphics to be done from the one which has a DIB to act as a image consumer
	if (pImage->m_pAwtDC != NULL) 
	{
	    //REMIND: what are the bits to be set here?
	    pImage->m_pAwtDC->Lock();
 
	    HDC hMemDC = pImage->m_pAwtDC->GetDC();
	    if (hMemDC != NULL) 
	    {
		// See comment at top of AwtImage::Draw about BitBlt rop codes...
		VERIFY(::BitBlt(hDC, TX(nX), TY(nY),
				pImage->m_nDstW, pImage->m_nDstH,
				hMemDC, 0, 0,
				(m_rop == R2_XORPEN) ? 0x00960000L : SRCCOPY));

		::GdiFlush();
	    }
	    pImage->m_pAwtDC->Unlock();
	} 
	else 
	{
	    // This image is a image consumer and all of the image 
	    // data should be ready by now.


	    pImage->Draw(hDC, TX(nX), TY(nY), (m_rop == R2_XORPEN), m_xorcolor, hBrush, (HColor *)c);
	    ::GdiFlush();
	}

	//
	// Free the xor brush if one was created...
	//
	if (hBrush) {
	    ::SelectObject(hDC, hOldBrush);
	    VERIFY( ::DeleteObject(hBrush) );
	}
    }
    // REMIND: select back the appropriate stuff
    m_pAwtDC->Unlock(this);
}


//-------------------------------------------------------------------------
//
// Copy an area identified by (nX, nY, nW, nH) into (nX + nDX, nY + nDY, nW, nH)
// no stretching involved
//
//-------------------------------------------------------------------------
void AwtGraphics::CopyArea(long nDX, long nDY, long nX, long nY, long nW, long nH)
{
    if (nW > 0 && nH > 0) {
        HDC hDC = m_pAwtDC->Lock(this);

        if (hDC != NULL) {
            long dX = nDX;
            long dY = nDY;
            RECT rectSrc;
            RECT rectDst;
            RECT rectInvalid;
            HRGN hRgn;

            // calculate source rectangle
            rectSrc.left  = TX(nX);
            rectSrc.top   = TY(nY);
            rectSrc.right =	TX(nX+nW);
            rectSrc.bottom = TY(nY+nH);

            rectDst.left  = rectSrc.left + nDX; 
            rectDst.top   = rectSrc.top + nDY; 
            rectDst.right =	rectSrc.right + nDX; 
            rectDst.bottom = rectSrc.bottom  + nDY; 

            hRgn = ::CreateRectRgn(0, 0, 0, 0);
            VERIFY(::ScrollDC(hDC, dX, dY, &rectSrc, &m_ClipRect, hRgn, &rectInvalid));

            //\\//\\//\\//......
            ::SubtractRect(&rectDst, &rectSrc, &rectDst);
            if (!EqualRect(&rectDst, &rectInvalid))
                m_pAwtDC->InvalidateRgn(hRgn);

            if (hRgn) {
                ::DeleteObject(hRgn);
            }
        }

        m_pAwtDC->Unlock(this);
    }
}


#ifdef DEBUG_CONSOLE

char* AwtGraphics::PrintStatus()
{
    char* buffer = new char[1024];

    wsprintf(buffer, "**** [%lx] Graphics Status: ****\r\n\tClipRect = %d, %d, %d, %d\r\n\tBrush = %lx\r\n\tPen = %lx\r\n****************\r\n",
				*(long*)this,
                m_ClipRect.left,
                m_ClipRect.top,
                m_ClipRect.right,
                m_ClipRect.bottom,
				(m_pBrush) ? m_pBrush->GetColor() : -1L,
				(m_pPen) ? m_pPen->GetColor() : -1L);

	return buffer;
}

#endif


extern "C" {

void sun_awt_windows_WGraphics_createFromComponent(HWGraphics *self, HWComponentPeer *canvas)
{
    AwtGraphics *pNewGraphics;

    CHECK_NULL(self, "self is NULL");
    CHECK_NULL(canvas, "ComponentPeer is null.");

    pNewGraphics = new AwtGraphics(self, canvas);
    CHECK_ALLOCATION(pNewGraphics);
}

void sun_awt_windows_WGraphics_createFromGraphics(HWGraphics *self, HWGraphics *g)
{
    AwtGraphics *pNewGraphics;
    AwtGraphics *pFromGraphics;

    CHECK_GRAPHICS_PEER(g);
    CHECK_NULL(self, "self is NULL");

    // invoke the copy constructor for the graphic to be cloned
    pFromGraphics = GET_OBJ_FROM_GRAPHICS(g);
    pNewGraphics  = new AwtGraphics(*pFromGraphics);
    CHECK_ALLOCATION(pNewGraphics);

    // update the values we don't want from the copy constructor (like the peer)
    pNewGraphics->InitCopy(self);
}

void
sun_awt_windows_WGraphics_imageCreate(HWGraphics *self, 
                                      HImageRepresentation *ir)
{
    AwtGraphics *pNewGraphics;

    CHECK_NULL(self, "self is NULL");
    CHECK_NULL(ir, "ImageRepresentation is null.");

    pNewGraphics = new AwtGraphics(self, ir);
    CHECK_ALLOCATION(pNewGraphics);
}

void
sun_awt_windows_WGraphics_pSetFont(HWGraphics *self, Hjava_awt_Font *font)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_NULL(font, "FontPeer is null.");

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    pGraphics->SetFont(font);

}

void
sun_awt_windows_WGraphics_pSetForeground(HWGraphics *self, Hjava_awt_Color *c)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_NULL(c, "Colorpeer is null.");
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    DWORD    javaColor = c->obj->value;
    COLORREF color     = PALETTERGB((javaColor>>16)& 0xff, 
                                    (javaColor>>8) & 0xff, 
                                    (javaColor)    & 0xff);

    pGraphics->SetColor(color);
}

void
sun_awt_windows_WGraphics_dispose(HWGraphics *self)
{
    CHECK_NULL(self, "self is null.");
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    if (pGraphics) {
        pGraphics->Dispose();
    }
}

void
sun_awt_windows_WGraphics_drawString(HWGraphics *self,
                                     Hjava_lang_String *text,
                                     long x,
                                     long y)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_NULL(text, "StringPeer is null.");

#ifdef INTLUNICODE
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    WinCompStr* pwStr = new WinCompStr(text);
    CHECK_ALLOCATION(pwStr);
    pGraphics->DrawString(pwStr, x, y);
    delete pwStr;
#else  // INTLUNICODE
    char *buffer;
    int32_t len;
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    len    = javaStringLength(text);
    buffer = (char *)malloc(len + 1);
    CHECK_ALLOCATION(buffer);

    javaString2CString(text, buffer, len + 1);
    pGraphics->DrawString(buffer, x, y);

    free(buffer);
#endif // INTLUNICODE
}

long
sun_awt_windows_WGraphics_drawStringWidth( HWGraphics *self,
                                           Hjava_lang_String *text,
                                           long x,
                                           long y)
{
    long result = -1L;

    CHECK_GRAPHICS_PEER_AND_RETURN(self, result);
    CHECK_NULL_AND_RETURN(text, "StringPeer is null.", result);

#ifdef INTLUNICODE
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    WinCompStr* pwStr = new WinCompStr(text);
    CHECK_ALLOCATION_AND_RETURN(pwStr, result);
    result = pGraphics->DrawStringWidth(pwStr, x, y);
    delete pwStr;
#else  // INTLUNICODE
    int32_t len;
    char *buffer;
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    len    = javaStringLength(text);
    buffer = (char *)malloc(len + 1);
    CHECK_ALLOCATION_AND_RETURN(buffer, result);

    javaString2CString(text, buffer, len + 1);
    result = pGraphics->DrawStringWidth(buffer, x, y);

    free(buffer);
#endif // INTLUNICODE
    return result;
}

void
sun_awt_windows_WGraphics_drawChars( HWGraphics *self,
                                     HArrayOfChar *text,
                                     long offset,
                                     long length,
                                     long x,
                                     long y)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_NULL(text, "StringPeer is null.");
    CHECK_BOUNDARY_CONDITION((offset < 0 || length < 0 || offset + length > (long)obj_length(text)),
                             "ArrayIndexOutOfBoundsException");

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    WCHAR *pSrc= unhand(text)->body + offset;
#ifdef INTLUNICODE
    WCHAR *buffer;
    buffer = (WCHAR *)malloc(sizeof(WCHAR)*(length + 1));
    CHECK_ALLOCATION(buffer);

    memcpy(buffer, pSrc, sizeof(WCHAR)*length);
    buffer[length] = 0;
  
    WinCompStr *pwStr = new WinCompStr(buffer);
    free(buffer);
    CHECK_ALLOCATION(pwStr);

    pGraphics->DrawString(pwStr, x, y);
    delete pwStr;
#else  // INTLUNICODE
    char *buffer;
    long i;

    buffer = (char *)malloc(length + 1);
    CHECK_ALLOCATION(buffer);

    for (i=0; i<length; i++) {
        buffer[i] = (char)pSrc[i];
    }
    buffer[length] = 0;

    pGraphics->DrawString(buffer, x, y);

    free(buffer);
#endif // INTLUNICODE
}

long
sun_awt_windows_WGraphics_drawCharsWidth( HWGraphics *self,
                                          HArrayOfChar *text,
                                          long offset,
                                          long length,
                                          long x,
                                          long y)
{
    long result = -1L;

    CHECK_GRAPHICS_PEER_AND_RETURN(self, result);
    CHECK_NULL_AND_RETURN(text, "StringPeer is null.", result);

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    WCHAR *pSrc= unhand(text)->body + offset;
#ifdef INTLUNICODE
    WCHAR *buffer;
    buffer = (WCHAR *)malloc(sizeof(WCHAR)*(length + 1));
    CHECK_ALLOCATION_AND_RETURN(buffer, result);

    memcpy(buffer, pSrc, sizeof(WCHAR)*length);
    buffer[length] = 0;
  
    WinCompStr *pwStr = new WinCompStr(buffer);
    free(buffer);
    CHECK_ALLOCATION_AND_RETURN(pwStr, result);

    result = pGraphics->DrawStringWidth(pwStr, x, y);
    delete pwStr;
#else  // INTLUNICODE
    char *buffer;
    long i;

    buffer = (char *)malloc(length + 1);
    CHECK_ALLOCATION_AND_RETURN(buffer, result);

    for (i=0; i<length; i++) {
        buffer[i] = (char)pSrc[i];
    }
    buffer[length] = 0;

    result = pGraphics->DrawStringWidth(buffer, x, y);

    free(buffer);
#endif // INTLUNICODE
    return result;
}

void
sun_awt_windows_WGraphics_drawBytes( HWGraphics *self,
                                     HArrayOfByte *text,
                                     long offset,
                                     long length,
                                     long x,
                                     long y)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_NULL(text, "StringPeer is null.");
    CHECK_BOUNDARY_CONDITION((offset < 0 || length < 0 || offset + length > (long)obj_length(text)),
                             "ArrayIndexOutOfBoundsException");

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    char *pSrc= unhand(text)->body + offset;
#ifdef INTLUNICODE
    WCHAR *buffer;
    buffer = (WCHAR*) malloc(sizeof(WCHAR)*sizeof(length + 1));
    CHECK_ALLOCATION(buffer);

    for(long i = 0; i < length ; i++)
        buffer[i] = (WCHAR) (0x00ff & pSrc[i]);
    buffer[length] = 0;

    WinCompStr *pwStr = new WinCompStr(buffer);
    free(buffer);
    CHECK_ALLOCATION(pwStr);

    pGraphics->DrawString(pwStr, x, y);
    delete pwStr;

#else  // INTLUNICODE
    char *buffer;

    buffer = (char *)malloc(length + 1);
    CHECK_ALLOCATION(buffer);

    memcpy(buffer, pSrc, length);
    buffer[length] = 0;
     
    pGraphics->DrawString(buffer, x, y);

    free(buffer);
#endif // INTLUNICODE
}

long
sun_awt_windows_WGraphics_drawBytesWidth( HWGraphics *self,
                                          HArrayOfByte *text,
                                          long offset,
                                          long length,
                                          long x,
                                          long y)
{
    long result = -1L;

    CHECK_GRAPHICS_PEER_AND_RETURN(self, result);
    CHECK_NULL_AND_RETURN(text, "StringPeer is null.", result);
    CHECK_BOUNDARY_CONDITION_RETURN((offset < 0 || length < 0 || offset + length > (long)obj_length(text)),
                                    "ArrayIndexOutOfBoundsException", result);

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    char *pSrc= unhand(text)->body + offset;
#ifdef INTLUNICODE
    WCHAR *buffer;
    buffer = (WCHAR*) malloc(sizeof(WCHAR)*sizeof(length + 1));
    CHECK_ALLOCATION_AND_RETURN(buffer, result);

    for(long i = 0; i < length ; i++)
        buffer[i] = (WCHAR) (0x00ff & pSrc[i]);
    buffer[length] = 0;

    WinCompStr *pwStr = new WinCompStr(buffer);
    free(buffer);
    CHECK_ALLOCATION_AND_RETURN(pwStr, result);

    result = pGraphics->DrawStringWidth(pwStr, x, y);
    delete pwStr;

#else  // INTLUNICODE
    char *buffer;

    buffer = (char *)malloc(length + 1);
    CHECK_ALLOCATION_AND_RETURN(buffer, result);

    memcpy(buffer, pSrc, length);
    buffer[length] = 0;

    result = pGraphics->DrawStringWidth(buffer, x, y);

    free(buffer);
#endif // INTLUNICODE
    return result;
}

void
sun_awt_windows_WGraphics_drawLine( HWGraphics *self,
                                    long x1, long y1,
                                    long x2, long y2)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    pGraphics->DrawLine(x1, y1, x2, y2);
}

void
sun_awt_windows_WGraphics_clearRect( HWGraphics *self,
                                     long x, long y,
                                     long w, long h)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	 
    pGraphics->ClearRect(x, y, w, h);
}

void
sun_awt_windows_WGraphics_fillRect( HWGraphics *self,
                                    long x, long y,
                                    long w, long h)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    pGraphics->FillRect(x, y, w, h);
}

void
sun_awt_windows_WGraphics_drawRect( HWGraphics *self,
                                    long x, long y,
                                    long w, long h)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    pGraphics->DrawRect(x, y, w, h);
}

void
sun_awt_windows_WGraphics_drawRoundRect( HWGraphics *self,
                                         long x, long y, long w, long h,
                                         long arcWidth, long arcHeight)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    pGraphics->RoundRect(x, y, w, h, arcWidth, arcHeight);
}

void
sun_awt_windows_WGraphics_fillRoundRect( HWGraphics *self,
                                         long x, long y, long w, long h,
                                         long arcWidth, long arcHeight)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    pGraphics->FillRoundRect(x, y, w, h, arcWidth, arcHeight);
}


void
sun_awt_windows_WGraphics_setPaintMode( HWGraphics *self)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics     *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    Hjava_awt_Color *fg        = unhand(self)->foreground;

    if (fg) {
        DWORD javaColor = fg->obj->value;
        COLORREF fgcolor = PALETTERGB((javaColor>>16)& 0xff,
                                      (javaColor>>8) & 0xff,
                                      (javaColor)    & 0xff);

        pGraphics->SetRop(R2_COPYPEN, fgcolor, RGB(0, 0, 0));
    }
}

void
sun_awt_windows_WGraphics_setXORMode( HWGraphics *self,
                                      Hjava_awt_Color *c)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics     *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    Hjava_awt_Color *fg        = unhand(self)->foreground;

    if (fg) {
        DWORD javaColor, xorColor;
        COLORREF color, fgcolor;

        javaColor = fg->obj->value;
        xorColor  = c->obj->value;

        color = PALETTERGB((xorColor>>16)& 0xff,
                           (xorColor>>8) & 0xff,
                           (xorColor)    & 0xff);

	fgcolor = PALETTERGB((javaColor>>16)& 0xff,
                             (javaColor>>8) & 0xff,
                             (javaColor)    & 0xff);

        pGraphics->SetRop(R2_XORPEN, fgcolor, color);
    }
}


void
sun_awt_windows_WGraphics_clipRect( HWGraphics *self,
                                    long x, long y,
                                    long w, long h)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    pGraphics->SetClipRect(x, y, w, h);
}

void
sun_awt_windows_WGraphics_getClipRect( HWGraphics *self, 
                                       Hjava_awt_Rectangle *clip)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    RECT rect;
    CHECK_BOUNDARY_CONDITION((clip == 0), "null clip");

    pGraphics->GetClipRect(&rect);
    unhand(clip)->x      = rect.left;
    unhand(clip)->y      = rect.top;
    unhand(clip)->width  = rect.right  - rect.left;
    unhand(clip)->height = rect.bottom - rect.top;
}

void
sun_awt_windows_WGraphics_copyArea( HWGraphics *self,
                                    long x, long y,
                                    long width, long height,
                                    long dx, long dy)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);

    pGraphics->CopyArea(dx, dy, x, y, width, height);
}


static POINT *
transformPoints(HWGraphics *self, HArrayOfInt *xpoints, HArrayOfInt *ypoints,
                long npoints)
{
    ArrayOfInt      *xpoints_array = unhand(xpoints);
    ArrayOfInt      *ypoints_array = unhand(ypoints);
    POINT           *points;
    int             i;

    CHECK_BOUNDARY_CONDITION_RETURN(((int)obj_length(ypoints) < npoints || (int)obj_length(xpoints) < npoints),
                                    "IllegalArgumentException", NULL);

    points = (POINT*)malloc(sizeof(POINT) * npoints);
    CHECK_ALLOCATION_AND_RETURN(points, NULL);

    for (i=0; i<npoints; ++i) {
        points[i].x = (xpoints_array->body)[i];
        points[i].y = (ypoints_array->body)[i];
    }

    return points;
}

void
sun_awt_windows_WGraphics_drawPolygon( HWGraphics *self,
                                       HArrayOfInt *xpoints,
                                       HArrayOfInt *ypoints,
                                       long npoints)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_BOUNDARY_CONDITION((xpoints == NULL || ypoints == NULL),
                             "NullPointerException");

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    POINT *points = transformPoints(self, xpoints, ypoints, npoints);

    if (points) {
        pGraphics->Polygon(points, npoints);
        free(points);
    }
}

void
sun_awt_windows_WGraphics_fillPolygon( HWGraphics *self,
                                       HArrayOfInt *xpoints,
                                       HArrayOfInt *ypoints,
                                       long npoints)
{
    CHECK_GRAPHICS_PEER(self);
    CHECK_BOUNDARY_CONDITION((xpoints == NULL || ypoints == NULL),
                             "NullPointerException");

    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
    POINT *points = transformPoints(self, xpoints, ypoints, npoints);

    if (points) {
        pGraphics->FillPolygon(points, npoints);
        free(points);
    }
}


void
sun_awt_windows_WGraphics_drawOval( HWGraphics *self,
                                    long x, long y, long w, long h)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    pGraphics->Ellipse(x, y, w, h);
}


void
sun_awt_windows_WGraphics_fillOval( HWGraphics *self,
                                    long x, long y, long w, long h)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    pGraphics->FillEllipse(x, y, w, h);
}

void
sun_awt_windows_WGraphics_drawArc( HWGraphics *self,
                                   long x, long y, long w, long h,
                                   long startAngle, long endAngle)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    if (endAngle != 0) {
        pGraphics->Arc(x, y, w, h, startAngle, endAngle);
    }
}

void
sun_awt_windows_WGraphics_fillArc( HWGraphics *self,
                                   long x, long y, long w, long h,
                                   long startAngle, long endAngle)
{
    CHECK_GRAPHICS_PEER(self);
    AwtGraphics *pGraphics = GET_OBJ_FROM_GRAPHICS(self);
	
    if (endAngle != 0) {
        pGraphics->FillArc(x, y, w, h, startAngle, endAngle);
    }
}

};  // end of extern "C"

