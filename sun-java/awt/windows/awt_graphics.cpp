/*
 * @(#)awt_graphics.cpp	1.56 96/01/05
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
#include <limits.h>
#include <math.h>
#include "awt.h"
#include "awtapp.h"
#include "awt_fontmetrics.h"
#include "awt_graphics.h"
#include "awt_image.h"
#include "awt_label.h"
#include "awt_window.h"

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

#define OX		(unhand(m_pJavaObject)->originX)
#define OY		(unhand(m_pJavaObject)->originY)
#define TX(x)		((x) + OX)
#define TY(y)		((y) + OY)

static CMapPtrToPtr cleanupList;

AwtGraphics::AwtGraphics()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtGraphics");

	m_hDC = NULL;
	m_pSharedDC = NULL;
	m_bPenSet = FALSE;
	m_hPen = NULL;
	m_color = RGB(0, 0, 0);
	m_xorcolor = RGB(255, 255, 255);
	m_bBrushSet = FALSE;
	m_hBrush = NULL;
	m_pFont = NULL;
	m_bPaletteSelected = FALSE;
	m_rectClip = CRect(0, 0, 0, 0);
	m_bClipping = FALSE;
	m_rop = R2_COPYPEN;
	VERIFY(m_hClipRgn = ::CreateRectRgn(m_rectClip.left, m_rectClip.top, m_rectClip.right, m_rectClip.bottom));

        cleanupList.SetAt(this, this);
}

AwtGraphics::~AwtGraphics()
{
	if (m_pSharedDC != NULL) {
	    m_pSharedDC->Detach(this);
	    m_pSharedDC = NULL;
	}

	if (m_hPen != NULL) {
	    VERIFY(::DeleteObject(m_hPen));
	    m_hPen = NULL;
	}

	if (m_hBrush != NULL) {
	    VERIFY(::DeleteObject(m_hBrush));
	    m_hBrush = NULL;
	}
	
	VERIFY(::DeleteObject(m_hClipRgn));
	m_hClipRgn = NULL;

    cleanupList.RemoveKey(this);

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtGraphics::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						  long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((Hsun_awt_win32_Win32Graphics *)p1, (AwtWindow*)p2);
			break;
		case CREATECOPY:
			return (long)CreateCopy((Hsun_awt_win32_Win32Graphics *)p1, (AwtGraphics*)p2);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

HWND g_hWnd;
DWORD g_threadID;
HANDLE g_hClientEvent;
HANDLE g_hServerEvent;
HANDLE g_hThread;
DWORD ThreadFunc(DWORD);
HDC g_hResultDC;
AwtGraphics *AwtGraphics::Create(Hsun_awt_win32_Win32Graphics *pJavaObject, AwtCWnd *pWindow)
{
	AwtGraphics *pGraphics = new AwtGraphics();

	pGraphics->m_pJavaObject = pJavaObject;
        cleanupList.RemoveKey(pGraphics);  // window object will delete.
#if 0
// This code causes a significant resource leak, because the ReleaseDC
// in ~AwtGraphics() always fails.
	if (GetApp()->m_bWin95OS) {
		pGraphics->m_hDC = ::GetDC(pWindow->m_hWnd);
	} else {
#endif
	    if (pWindow->m_pSharedDC == NULL) {
		if (g_threadID == NULL) {
			g_hClientEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
			g_hServerEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
			if ((g_hThread = ::CreateThread(NULL, 1024, (LPTHREAD_START_ROUTINE)ThreadFunc, 0, THREAD_TERMINATE, &g_threadID)) == NULL) {
				SignalError(0, JAVAPKG "OutOfMemoryError", 0);
			}
			ASSERT(g_threadID != NULL);
		}
		g_hWnd = pWindow->m_hWnd;
		VERIFY(::SetEvent(g_hServerEvent));
		VERIFY(::WaitForSingleObject(g_hClientEvent, INFINITE) != WAIT_FAILED);
		VERIFY(::ResetEvent(g_hClientEvent));
		pWindow->m_pSharedDC = new AwtSharedDC(pWindow, g_hResultDC);
	    }
	    pGraphics->m_pSharedDC = pWindow->m_pSharedDC;
	    pGraphics->m_pSharedDC->Attach();
#if 0
	}
#endif
        if (pWindow->IsKindOf(RUNTIME_CLASS(AwtWindow))) {
            pGraphics->SetColor(((AwtWindow*)pWindow)->m_bgfgColor[FALSE]);
        } else if (pWindow->IsKindOf(RUNTIME_CLASS(AwtLabel))) {
            pGraphics->SetColor(((AwtLabel*)pWindow)->m_pParent->m_bgfgColor[FALSE]);
        }

        // Offset the origin so that 0,0 is the top-left of the frame,
        // instead of the client window, for top-level windows.
	CWnd *parent = pWindow->GetParent();
        CWnd *pWnd = parent;
	while (pWnd != NULL && !pWnd->IsKindOf(RUNTIME_CLASS(MainFrame)))
            pWnd = pWnd->GetParent();
        MainFrame* frame = (MainFrame*)pWnd;
        if (frame != NULL && parent->m_hWnd == frame->m_hWnd) {
	    long insetX;
	    long insetY;
            frame->SubtractInsetPoint(insetX, insetY);
            unhand(pJavaObject)->originX += insetX;
            unhand(pJavaObject)->originY += insetY;
        }

	return pGraphics;
}

AwtGraphics *AwtGraphics::CreateCopy(Hsun_awt_win32_Win32Graphics *pJavaObject, AwtGraphics *pSource)
{
	AwtGraphics *pGraphics;
	AwtCWnd *pWnd = pSource->GetWnd();

	if (pWnd != NULL) {
	    pGraphics = AwtGraphics::Create(pJavaObject, pWnd);
	} else {
	    pGraphics = new AwtGraphics();

	    pGraphics->m_pJavaObject = pJavaObject;
	    pGraphics->m_pSharedDC = pSource->m_pSharedDC;
	    pGraphics->m_pSharedDC->Attach();
	}

	if (pSource->m_bClipping) {
	    int x = pSource->m_rectClip.left;
	    int y = pSource->m_rectClip.top;
	    int w = pSource->m_rectClip.right - x;
	    int h = pSource->m_rectClip.bottom - y;
	    x -= unhand(pJavaObject)->originX;
	    y -= unhand(pJavaObject)->originY;
	    pGraphics->SetClipRect(x, y, w, h);
	}
	pGraphics->SetRop(pSource->m_rop, pSource->m_color, RGB(0, 0, 0));
	pGraphics->SetColor(pSource->m_color);
	pGraphics->SetFont(pSource->m_pFont);
	return pGraphics;
}

AwtGraphics *AwtGraphics::Create(Hsun_awt_win32_Win32Graphics *pJavaObject, AwtImage *pImage)
{
	AwtGraphics *pGraphics = new AwtGraphics();

	pGraphics->m_pJavaObject = pJavaObject;
	pGraphics->m_pSharedDC = pImage->m_pSharedDC;
	pGraphics->m_pSharedDC->Attach();

	return pGraphics;
}

void AwtGraphics::Dispose() {
    delete this;
}

void AwtGraphics::ValidateDC() {
    m_hDC = m_pSharedDC->GrabDC(this);
    if (m_hDC == NULL) {
	return;
    }
    if (m_pFont != NULL) {
	VERIFY(::SelectObject(m_hDC, m_pFont->m_hObject) != NULL);
    }
    VERIFY(::SetTextColor(m_hDC, m_color) != CLR_INVALID);
    VERIFY(::SetROP2(m_hDC, m_rop));
    m_bPaletteSelected = FALSE;
    if (m_bClipping) {
	VERIFY(::SelectClipRgn(m_hDC, m_hClipRgn) != ERROR);
    }
}

void AwtGraphics::InvalidateDC() {
    SetPenColor(FALSE);
    SetBrushColor(FALSE);
    ::SelectObject(m_hDC, ::GetStockObject(SYSTEM_FONT));
    if (m_bPaletteSelected) {
	::SelectObject(m_hDC, ::GetStockObject(DEFAULT_PALETTE));
	m_bPaletteSelected = FALSE;
    }
    if (m_bClipping) {
	::SelectClipRgn(m_hDC, NULL);
    }
    m_hDC = NULL;
}

static COLORREF XorResult(COLORREF fgcolor, COLORREF xorcolor) {
    COLORREF c;
    if (AwtImage::m_bitspixel > 8) {
	c = (fgcolor & 0xff000000) | ((fgcolor ^ xorcolor) & 0x00ffffff);
    } else {
	c = PALETTEINDEX(GetNearestPaletteIndex(AwtImage::m_hPal, fgcolor) ^
			 GetNearestPaletteIndex(AwtImage::m_hPal, xorcolor));
    }
    return c;
}

void AwtGraphics::SetColor(COLORREF color)
{
    if (m_rop == R2_XORPEN) {
	ChangeColor(XorResult(color, m_xorcolor));
    } else {
	ChangeColor(color);
    }
}

void AwtGraphics::ChangeColor(COLORREF color)
{
    if (m_color != color) {
	m_color = color;
	if (m_hDC != NULL) {
	    VERIFY(::SetTextColor(m_hDC, color) != CLR_INVALID);
	}
	if (m_hPen != NULL) {
	    if (m_hDC != NULL) {
		SetPenColor(FALSE);
	    }
	    VERIFY(::DeleteObject(m_hPen));
	    m_hPen = NULL;
	}
	if (m_hBrush != NULL) {
	    if (m_hDC != NULL) {
		SetBrushColor(FALSE);
	    }
	    VERIFY(::DeleteObject(m_hBrush));
	    m_hBrush = NULL;
	}
    }
}

void AwtGraphics::SetFont(AwtFont *pFont)
{
	if (m_pFont != pFont) {	
	    if (m_hDC != NULL) {
		VERIFY(::SelectObject(m_hDC, pFont->m_hObject) != NULL);
	    }
	    m_pFont = pFont;
	}
}

void AwtGraphics::SetRop(int rop, COLORREF fgcolor, COLORREF color)
{
    if (m_rop != rop) {
	if (m_hDC != NULL) {
	    VERIFY(::SetROP2(m_hDC, rop));
	}
	m_rop = rop;
    }
    if (m_rop == R2_XORPEN) {
	ChangeColor(XorResult(fgcolor, color));
	m_xorcolor = color;
    } else {
	ChangeColor(fgcolor);
    }
}

void AwtGraphics::DrawLine(long nX1, long nY1, long nX2, long nY2)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetPenColor(TRUE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::MoveToEx(hDC, TX(nX1), TY(nY1), NULL));
	VERIFY(::LineTo(hDC, TX(nX2), TY(nY2)));
	// Paint the end-point.
	VERIFY(::LineTo(hDC, TX(nX2+1), TY(nY2)));
	//GdiFlush();
}

void AwtGraphics::ClearRect(long nX, long nY, long nW, long nH)
{
    if (nW <= 0 || nH <= 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
    HBRUSH hBrush;
    AwtCWnd *pWnd = GetWnd();
    if (pWnd != NULL) {
        // Use the brush that the window cached
        COLORREF color;
        if (pWnd->IsKindOf(RUNTIME_CLASS(AwtWindow))) {
            color = ((AwtWindow*)pWnd)->m_bgfgColor[FALSE];
        } else if (pWnd->IsKindOf(RUNTIME_CLASS(AwtLabel))) {
            color = ((AwtLabel*)pWnd)->m_pParent->m_bgfgColor[FALSE];
        }
        hBrush = GetApp()->GetBrush(color);
    } else {
        hBrush = (HBRUSH)::GetStockObject( WHITE_BRUSH );
    }
    HBRUSH hOldBrush = (HBRUSH) ::SelectObject(hDC, hBrush);
    CALLINGWINDOWS(GetWnd()->m_hWnd);
    ::FillRect(hDC, CRect(TX(nX), TY(nY), TX(nX+nW), TY(nY+nH)), hBrush);
    VERIFY(::SelectObject(hDC, hOldBrush));
    //
    // The temporary HBRUSH does not need to be deleted since it is either 
    // cached or a stock windows object
    //
	//GdiFlush();
}

void AwtGraphics::FillRect(long nX, long nY, long nW, long nH)
{		
    if (nW <= 0 || nH <= 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetBrushColor(TRUE);
	SetPenColor(FALSE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Rectangle(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
	//GdiFlush();
}

void AwtGraphics::RoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH)
{		
    if (nW < 0 || nH < 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetPenColor(TRUE);
	SetBrushColor(FALSE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	::RoundRect(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
		    nArcW, nArcH);
}

void AwtGraphics::FillRoundRect(long nX, long nY, long nW, long nH, long nArcW, long nArcH)
{		
    if (nW <= 0 || nH <= 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetPenColor(FALSE);
	SetBrushColor(TRUE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	::RoundRect(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
		    nArcW, nArcH);
}

void AwtGraphics::Ellipse(long nX, long nY, long nW, long nH)
{		
    if (nW < 0 || nH < 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetPenColor(TRUE);
	SetBrushColor(FALSE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Ellipse(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
}

void AwtGraphics::FillEllipse(long nX, long nY, long nW, long nH)
{		
    if (nW <= 0 || nH <= 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetPenColor(FALSE);
	SetBrushColor(TRUE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Ellipse(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
}

void AngleToCoord(long nAngle, long nW, long nH, long *nX, long *nY) {
	const double pi = 3.1415926535;
	const double toRadians = 2 * pi / 360;

	*nX = (long)(cos((double)nAngle * toRadians) * nW);
	*nY = -(long)(sin((double)nAngle * toRadians) * nH);
}

void AwtGraphics::Arc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle)
{		
    if (nW < 0 || nH < 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	long nSX, nSY, nDX, nDY;
	BOOL bEndAngleNeg = nEndAngle < 0;

	nEndAngle += nStartAngle;
	if (bEndAngleNeg) {
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
	SetPenColor(TRUE);
	SetBrushColor(FALSE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Arc(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
		     TX(nSX), TY(nSY), TX(nDX), TY(nDY)));
}

void AwtGraphics::FillArc(long nX, long nY, long nW, long nH, long nStartAngle, long nEndAngle)
{		
    if (nW <= 0 || nH <= 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	long nSX, nSY, nDX, nDY;
	BOOL bEndAngleNeg = nEndAngle < 0;

	nEndAngle += nStartAngle;
	if (bEndAngleNeg) {
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
	SetPenColor(FALSE);
	SetBrushColor(TRUE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Pie(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
		     TX(nSX), TY(nSY), TX(nDX), TY(nDY)));
}

void AwtGraphics::ChangePen(BOOL bSetPen)
{
    if (bSetPen) {
	if (!m_bPaletteSelected) {
		m_bPaletteSelected = TRUE;
		VERIFY(::SelectPalette(m_hDC, AwtImage::m_hPal, FALSE));
		UINT result = ::RealizePalette(m_hDC);
	}
	if (m_hPen == NULL) {
		m_hPen = ::CreatePen(PS_SOLID, 1, m_color);
		m_bPenSet = FALSE;
	}
	if (!m_bPenSet) {
	    ::SelectObject(m_hDC, m_hPen);
	    m_bPenSet = TRUE;
	}
    } else if (m_bPenSet) {
	::SelectObject(m_hDC, ::GetStockObject(NULL_PEN));
	m_bPenSet = FALSE;
    }
}

void AwtGraphics::ChangeBrush(BOOL bSetBrush)
{
    if (bSetBrush) {
	if (!m_bPaletteSelected) {
		m_bPaletteSelected = TRUE;
		VERIFY(::SelectPalette(m_hDC, AwtImage::m_hPal, FALSE));
		UINT result = ::RealizePalette(m_hDC);
	}
	if (m_hBrush == NULL) {
		m_hBrush = ::CreateSolidBrush(m_color);
		m_bBrushSet = FALSE;
	}
	if (!m_bBrushSet) {
	    ::SelectObject(m_hDC, m_hBrush);
	    m_bBrushSet = TRUE;
	}
    } else if (m_bBrushSet) {
	::SelectObject(m_hDC, ::GetStockObject(NULL_BRUSH));
	m_bBrushSet = FALSE;
    }
}

void AwtGraphics::Polygon(POINT *pPoints, int nPoints)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	int i;
	POINT *p = pPoints;

    for (i=0; i < nPoints; i++) {
		p->x = TX(p->x);
		p->y = TY(p->y);
		p++;
    }
	SetPenColor(TRUE);
	SetBrushColor(FALSE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Polyline(hDC, pPoints, nPoints));
	//GdiFlush();
}

void AwtGraphics::FillPolygon(POINT *pPoints, int nPoints)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	int i;
	POINT *p = pPoints;

    for (i=0; i < nPoints; i++) {
		p->x = TX(p->x);
		p->y = TY(p->y);
		p++;
    }
	SetPenColor(FALSE);
	SetBrushColor(TRUE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Polygon(hDC, pPoints, nPoints));
	//GdiFlush();
}

void AwtGraphics::DrawRect(long nX, long nY, long nW, long nH)
{
    if (nW < 0 || nH < 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	SetPenColor(TRUE);
	SetBrushColor(FALSE);
	CALLINGWINDOWS(GetWnd()->m_hWnd);
	VERIFY(::Rectangle(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
	//GdiFlush();
}

void AwtGraphics::DrawString(char *pzString, long nX, long nY)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	if (m_pFont != NULL && m_pFont->m_nAscent < 0) {
		AwtFontMetrics::SetFontAscent(m_pFont);
	}
	if (m_rop == R2_XORPEN) {
	    BeginPath(hDC);
	}
	VERIFY(::TextOut(hDC, TX(nX), TY(nY - (m_pFont ? m_pFont->m_nAscent : 0)), 
		pzString, strlen(pzString)));
	if (m_rop == R2_XORPEN) {
	    EndPath(hDC);
	    SetBrushColor(TRUE);
	    SetPenColor(FALSE);
	    FillPath(hDC);  
	}
	//GdiFlush();
	free(pzString);
}

long AwtGraphics::DrawStringWidth(char *pzString, long nX, long nY)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return 0;
    }
	CSize size;

	if (m_pFont != NULL && m_pFont->m_nAscent < 0) {
		AwtFontMetrics::SetFontAscent(m_pFont);
	}
	if (m_rop == R2_XORPEN) {
	    BeginPath(hDC);
	}
	VERIFY(::TextOut(hDC, TX(nX), TY(nY - (m_pFont ? m_pFont->m_nAscent : 0)), 
		pzString, strlen(pzString)));
	if (m_rop == R2_XORPEN) {
	    EndPath(hDC);
	    SetBrushColor(TRUE);
	    SetPenColor(FALSE);
	    FillPath(hDC);  
	}
	VERIFY(::GetTextExtentPoint32(hDC, pzString, strlen(pzString), &size));
	//GdiFlush();
	free(pzString);
	return size.cx;
}

void AwtGraphics::DrawImage(AwtImage *pImage, long nX, long nY,
			    struct Hjava_awt_Color *c)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
    int xormode = (m_rop == R2_XORPEN);
    HBRUSH hOldBrush;
    HBRUSH hBrush;
    if (xormode) {
	SetPenColor(FALSE);
	hBrush = ::CreateSolidBrush(m_xorcolor);
	hOldBrush = (HBRUSH) ::SelectObject(hDC, hBrush);
    }
    if (pImage->m_pSharedDC != NULL) {
	HDC hMemDC = pImage->m_pSharedDC->GetDC();
	if (hMemDC != NULL) {
	    // See comment at top of AwtImage::Draw about BitBlt rop codes...
	    VERIFY(::BitBlt(hDC, TX(nX), TY(nY),
			    pImage->m_nDstW, pImage->m_nDstH,
			    hMemDC, 0, 0,
			    xormode ? 0x00960000 : SRCCOPY));
	}
    } else {
	pImage->Draw(hDC, TX(nX), TY(nY), xormode, m_xorcolor, hBrush, c);
    }
    if (xormode) {
	VERIFY(::SelectObject(hDC, hOldBrush));
	VERIFY(::DeleteObject(hBrush));
    }
    m_bPaletteSelected = TRUE;
}

void AwtGraphics::CopyArea(long nDstX, long nDstY, long nX, long nY, long nW, long nH)
{
    if (nW <= 0 || nH <= 0) {
	return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
 	CRect rectClip;
	long dX = TX(nDstX)-TX(nX);
	long dY = TY(nDstY)-TY(nY);
	CRgn rgnUpdate;
	CRect rectSrc(TX(nX), TY(nY), 
		TX(nX+nW), TY(nY+nH));
	CRect rectDst = rectSrc + CPoint(dX, dY);

	rgnUpdate.CreateRectRgn(0, 0, 0, 0);
	if (!m_bClipping) {
		rectClip.UnionRect(rectSrc, rectDst);
	} else {
		VERIFY(::GetRgnBox(m_hClipRgn, &rectClip) != ERROR);
	}
	VERIFY(::ScrollDC(hDC, dX, dY, rectSrc, rectClip, (HRGN)rgnUpdate.m_hObject, NULL));
	
	// If the src-dst rectangle equals the update rectangle, then don't
	// bother invalidating the region; it's assumed the client will
	// repaint the pixels in src-dst.
	VERIFY(rgnUpdate.GetRgnBox(&rectClip) != ERROR);
	rectSrc.SubtractRect(rectSrc, rectDst);
	AwtCWnd *pWnd = GetWnd();
	if (rectSrc != rectClip && pWnd != NULL) {
		// REMIND: must somehow send exposures for bitmaps...
		pWnd->InvalidateRgn(&rgnUpdate);
	}
}

void AwtGraphics::SetClipRect(long nX, long nY, long nW, long nH)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
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
    if (!m_bClipping ||
	m_rectClip.left != x1 || m_rectClip.top != y1 ||
	m_rectClip.right != x2 || m_rectClip.bottom != y2) {
	if (m_bClipping) {
	    m_rectClip &= CRect(x1, y1, x2, y2);
	} else {
	    m_rectClip = CRect(x1, y1, x2, y2);
	}
	::SetRectRgn(m_hClipRgn,
		     m_rectClip.left,
		     m_rectClip.top,
		     m_rectClip.right,
		     m_rectClip.bottom);
    }
    m_bClipping = TRUE;
    VERIFY(::SelectClipRgn(hDC, m_hClipRgn) != ERROR);
}

void AwtGraphics::ClearClipRect()
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
	return;
    }
	m_bClipping = FALSE;
	VERIFY(::SelectClipRgn(hDC, NULL) != ERROR);
}

void AwtGraphics::GetClipRect(RECT *rect)
{
	if (m_bClipping) {
		rect->left = (m_rectClip.left - OX);
		rect->top = (m_rectClip.top - OY);
		rect->right = (m_rectClip.right - OX);
		rect->bottom = (m_rectClip.bottom - OY);
	} else {
		rect->left = LONG_MIN;
		rect->top = LONG_MIN;
		rect->right = LONG_MAX;
		rect->bottom = LONG_MAX;
	}
}

void AwtGraphics::Cleanup() {
    // Delete any AwtGraphics objects which weren't disposed.
    POSITION pos = cleanupList.GetStartPosition();
    while (pos != NULL) {
        void *key, *p;
        cleanupList.GetNextAssoc(pos, key, p);
        AwtGraphics* pGraphics = (AwtGraphics*)p;
        if (pGraphics != NULL) {
            delete pGraphics;
        }
    }
    cleanupList.RemoveAll();

    ::TerminateThread(g_hThread, 0);
    ::CloseHandle(g_hServerEvent);
    ::CloseHandle(g_hClientEvent);
    ::CloseHandle(g_hThread);
}

// The purpose of this thread is simply to create DC's.  The problem
// is that if the thread that creates the DC goes away, the DC becomes invalid.
// Also, we can't use the WServer thread to create the DC's because
// that would introduce a deadlock.
// <<Need to worry about cleaning up this thread.>>
DWORD ThreadFunc(DWORD dwParam)
{
	while (TRUE) {
		VERIFY(::WaitForSingleObject(g_hServerEvent, INFINITE) != WAIT_FAILED);
		VERIFY(::ResetEvent(g_hServerEvent));
		VERIFY(g_hResultDC = ::GetDC(g_hWnd));
		VERIFY(::SetBkMode(g_hResultDC, TRANSPARENT) != NULL);
		::SelectObject(g_hResultDC, ::GetStockObject(NULL_BRUSH));
		::SelectObject(g_hResultDC, ::GetStockObject(NULL_PEN));
		GdiFlush();
		VERIFY(::SetEvent(g_hClientEvent));
	}
	return 0;
}

HDC AwtSharedDC::GrabDC(AwtGraphics *pNewOwner) {
    if (m_pOwner != NULL) {
	m_pOwner->InvalidateDC();
    }
    if (m_hDC != NULL) {
	m_pOwner = pNewOwner;
    }
    return m_hDC;
}

void AwtSharedDC::Detach(AwtGraphics *pOldOwner) {
    if (pOldOwner == NULL) {
	// Must be the destination drawable that is detaching
	FreeDC();
    } else if (m_pOwner == pOldOwner) {
	// Release this Device Context and leave it "ownerless"
	m_pOwner->InvalidateDC();
	m_pOwner = NULL;
    }
    if (--m_useCount == 0) {
	delete this;
    }
}

void AwtSharedDC::FreeDC() {
    if (m_hDC == NULL) {
	return;
    }
    if (m_pOwner != NULL) {
	m_pOwner->InvalidateDC();
	m_pOwner = NULL;
    }
    if (m_pWnd == NULL) {
	VERIFY(::DeleteDC(m_hDC));
    } else {
	::ReleaseDC(m_pWnd->m_hWnd, m_hDC);
	m_pWnd = NULL;
    }
    m_hDC = NULL;
}
