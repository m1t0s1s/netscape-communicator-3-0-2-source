#ifndef AWT_GRAPHICS_H
#define AWT_GRAPHICS_H

#include "awt_defs.h"
#include "awt_component.h"
#include "awt_palette.h"
#include "awt_resource.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif // INTLUNICODE

class AwtDC; 
class AwtImage; 
//class AwtPen; 
class AwtFont; 

struct Hjava_awt_Color;

#define LONG_MIN	(-2147483647L - 1) 
#define LONG_MAX	  2147483647L	

DECLARE_SUN_AWT_WINDOWS_PEER(WGraphics);
//typedef struct  Classsun_awt_windows_WGraphics WGraphics;
//typedef struct Hsun_awt_windows_WGraphics HWGraphics;
DECLARE_SUN_AWT_IMAGE_PEER(ImageRepresentation);


#define AWT_GRAPHICS_INIT_RECT(rect,l,t,r,b) \
  rect.left   = (int)l; \
  rect.top    = (int)t; \
  rect.right  = (int)r; \
  rect.bottom = (int)b;

       
#define AWT_GRAPHICS_NONZERO_CLIPRECT              \
   (   ((m_ClipRect.bottom-m_ClipRect.top) >0 )    \
       && ((m_ClipRect.right-m_ClipRect.left) >0 ) )


class AwtGraphics : public AwtObject
{
public:

                            AwtGraphics();
    virtual                 ~AwtGraphics();
    virtual BOOL            CallMethod(AwtMethodInfo *info) {return TRUE;}

							// constructors
                            AwtGraphics(HWGraphics *self, HWComponentPeer *canvas);
                            AwtGraphics(HWGraphics *self, HImageRepresentation  *ir);

            void            InitCopy(HWGraphics *self);

                            // font info
            AwtFont        *GetAwtFont(void)	{ return m_pFont; }
            HFONT           GetFont(void)       { return m_pFont->GetFont(); }
            void            SetFont(Hjava_awt_Font *font);

                            // color info
            COLORREF        GetColor(void)      { return m_color; }
	        void            SetColor(COLORREF color);
            void            ChangeColor(COLORREF color);
            COLORREF        XorResult(COLORREF fgcolor, COLORREF xorcolor);

                            // brush used by this graphics
            AwtCachedBrush *GetAwtBrush(void)	{ return m_pBrush; }
            HBRUSH          GetBrush(void)      { return m_pBrush->GetBrush(); }
                
                            // pen used by this graphic
            AwtCachedPen   *GetAwtPen(void)     { return m_pPen; }
            HPEN            GetPen(void)        { return m_pPen->GetPen(); }
            
                            // ROP info
            DWORD           GetRop(void)        { return m_rop; }
            void            SetRop(DWORD rop, COLORREF fgcolor, COLORREF color);
            DWORD           GetROP2(void)       { return m_rop; }

                            // palette
            HPALETTE        GetPalette(void)    { return m_pPalette->GetPalette(); } // REMIND: get the actual color palette;

                            // image
            AwtImage       *GetAwtImage(void)   { return m_pImage; }

                            // clipping region
            RECT           *GetClipRect(void)   { return &m_ClipRect; }
            void            SetClipRect(long nX, long nY, long nW, long nH);
            void            GetClipRect(RECT *rect);

                            //
                            // graphics functions
                            //
            void            DrawLine(long nX1, long nY1, long nX2, long nY2);
            void            Polygon(POINT *pPoints, int nPoints);
            void            FillPolygon(POINT *pPoints, int nPoints);
            void            DrawRect(long nX, long nY, long nW, long nH);
            void            FillRect(long nX, long nY, long nW, long nH);
            void            ClearRect(long nX, long nY, long nW, long nH);
            void            RoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH);
            void            FillRoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH);
            void            Ellipse(long nX, long nY, long nW, long nH);
            void            FillEllipse(long nX, long nY, long nW, long nH);
	           
            void            AngleToCoords(long nAngle, long *nX, long *nY);
            void            Arc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle);
            void            FillArc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle);

#ifdef INTLUNICODE
           // Draws the string at the specified point.  (nX, nY) designates
           // the reference point of the string - i.e. the top of the string
           // plus the ascent.
           void            DrawString(WinCompStr *pzString, long nX, long nY);

           // Like DrawString except that the width of the string is returned.
           long            DrawStringWidth(WinCompStr *pzString, long nX, long nY);
#else
           // Draws the string at the specified point.  (nX, nY) designates
           // the reference point of the string - i.e. the top of the string
           // plus the ascent.
           void            DrawString(char *pzString, long nX, long nY);

           // Like DrawString except that the width of the string is returned.
           long            DrawStringWidth(char *pzString, long nX, long nY);
#endif	// INTLUNICODE
            // Draws the image at ('nX', 'nY') with optional background color c.
            void            DrawImage(AwtImage *pImage, long nX, long nY,
		                                    struct Hjava_awt_Color *c);

            void            CopyArea(long nDstX, long nDstY, long nX, long nY, long nW, long nH);


#ifdef DEBUG_CONSOLE
    char* PrintStatus();
#endif

    //
    // data members
    //
protected:
    AwtDC*                  m_pAwtDC;

    AwtFont*                m_pFont;
    AwtCachedBrush*         m_pBrush;
    AwtCachedPen*           m_pPen;
    AwtPalette*             m_pPalette;
    //AwtObject*              m_pAwtDCObject;

    AwtImage*               m_pImage;

   	DWORD                   m_rop;
    RECT                    m_ClipRect;
    COLORREF                m_color;
	COLORREF                m_xorcolor;

};



#endif  // AWT_GRAPHICS_H

