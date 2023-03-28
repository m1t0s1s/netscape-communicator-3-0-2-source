/*
 * @(#)awt_graphics.h	1.33 96/01/05 Patrick P. Chan
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
#ifndef _AWTGRAPHICS_H_
#define _AWTGRAPHICS_H_

#include "awtapp.h"
#include "awt_image.h"
#include "awt_window.h"

class AwtSharedDC
{
public:
    AwtSharedDC(AwtCWnd *pWndDest, HDC hDC) {
	m_useCount = 1;
	m_pOwner = NULL;
	m_hDC = hDC;
	m_pWnd = pWndDest;
    }

    ~AwtSharedDC() {
	// This should be a NOP, but just in case...
	FreeDC();
    }

    // Get the Window being drawn to
    // (NULL if this is an offscreen graphics)
    AwtCWnd *GetWnd() {
	return m_pWnd;
    }

    // Get the Device Context without "grabbing it"
    // (only used as a source DC for off-screen blits)
    HDC GetDC() {
	return m_hDC;
    }

    // Grab this Device Context and assign a new "owner"
    HDC GrabDC(class AwtGraphics *pNewOwner);

    // Attach this Shared DC to some object (increment reference count)
    void Attach() {
	++m_useCount;
    }

    // Detach this Shared DC from some object
    // (decrement reference count and free if necessary)
    void Detach(class AwtGraphics *pOldOwner);

private:
    // Get rid of the Windows DC object
    void FreeDC();

    int			 m_useCount;
    class AwtGraphics	*m_pOwner;
    HDC			 m_hDC;
    AwtCWnd		*m_pWnd;
};

class AwtGraphics
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, CREATECOPY};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

	AwtGraphics();
	~AwtGraphics();

// Operations
	// Creates a graphics context on 'pWindow'.
	static AwtGraphics *Create(Hsun_awt_win32_Win32Graphics *pJavaObject, AwtCWnd *pWindow);
	static AwtGraphics *Create(Hsun_awt_win32_Win32Graphics *pJavaObject, AwtImage *pImage);
	static AwtGraphics *CreateCopy(Hsun_awt_win32_Win32Graphics *pJavaObject, AwtGraphics *pSource);

 	void Dispose();

	AwtCWnd *GetWnd() {
	    return m_pSharedDC->GetWnd();
	}

	HDC GetDC() {
	    if (m_hDC == NULL) {
		ValidateDC();
	    }
	    return m_hDC;
	}
	void ValidateDC();
	void InvalidateDC();


	// Sets the m_color instance variable to the absolute COLORREF
	void ChangeColor(COLORREF color);

	// Calculates the appropriate new COLORREF according to the
	// current ROP and calls ChangeColor on the new COLORREF
	void SetColor(COLORREF color);

	void SetFont(AwtFont *pFont);

	void SetRop(int rop, COLORREF fgcolor, COLORREF color);

	void SetClipRect(long nX, long nY, long nW, long nH);
	void ClearClipRect();

	// Sets 'rect' with the clipping rectangle.  If there is none, 'rect'
	// becomes CRect(MINLONG, MINLONG, MAXLONG, MAXLONG).
	void GetClipRect(RECT *rect);

	// Draws a line using the foreground color between points (nX1, nY1)
	// and (nX2, nY2).  The end-point is painted.
	void DrawLine(long nX1, long nY1, long nX2, long nY2);

	void Polygon(POINT *pPoints, int nPoints);
	
	void FillPolygon(POINT *pPoints, int nPoints);
	
	void ClearRect(long nX, long nY, long nW, long nH);

	void FillRect(long nX, long nY, long nW, long nH);

	void DrawRect(long nX, long nY, long nW, long nH);

	void RoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH);
	
	void FillRoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH);
	
	void Ellipse(long nX, long nY, long nW, long nH);
	
	void FillEllipse(long nX, long nY, long nW, long nH);
	
	// Converts an angle given in 1/64ths of a degree to a pair of coordinates
	// that intersects the line produced by the angle.
	void AngleToCoords(long nAngle, long *nX, long *nY);
	
	void Arc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle);
	
	void FillArc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle);
	
	void SetBrushColor(BOOL bSetBrush) {
	    if (bSetBrush != m_bBrushSet) {
		ChangeBrush(bSetBrush);
	    }
	};
	void ChangeBrush(BOOL bSetBrush);
	void SetPenColor(BOOL bSetPen) {
	    if (bSetPen != m_bPenSet) {
		ChangePen(bSetPen);
	    }
	};
	void ChangePen(BOOL bSetPen);

	// Draws the string at the specified point.  (nX, nY) designates
	// the reference point of the string - i.e. the top of the string
	// plus the ascent.
	void DrawString(char *pzString, long nX, long nY);

	// Like DrawString except that the width of the string is returned.
	long DrawStringWidth(char *pzString, long nX, long nY);
	
	// Draws the image at ('nX', 'nY') with optional background color c.
	void DrawImage(AwtImage *pImage, long nX, long nY,
		       struct Hjava_awt_Color *c);

	void CopyArea(long nDstX, long nDstY, long nX, long nY, long nW, long nH);

	// Delete any AwtGraphics objects which were not disposed.
	static void Cleanup();

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_BUTTON, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_BUTTON, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);
// Variables
public:
	HDC m_hDC;
	class AwtSharedDC *m_pSharedDC;

private:
// Variables
	BOOL m_bPaletteSelected;
	CRect m_rectClip;
	HRGN m_hClipRgn;
	BOOL m_bClipping;

	AwtFont *m_pFont;

	COLORREF m_color;
	COLORREF m_xorcolor;
	BOOL m_bBrushSet;
	HBRUSH m_hBrush;
	BOOL m_bPenSet;
	HPEN m_hPen;
	int m_rop;

	Hsun_awt_win32_Win32Graphics *m_pJavaObject;
};

#endif // _AWTGRAPHICS_H_
