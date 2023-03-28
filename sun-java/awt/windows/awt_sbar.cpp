/*
 * @(#)awt_sbar.cpp	1.16 95/11/29 Patrick P. Chan
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
#include "limits.h"

#include "awt_sbar.h"

IMPLEMENT_DYNCREATE(AwtScrollbar, CScrollBar)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(AwtScrollbar, CScrollBar)
	//{{AFX_MSG_MAP(AwtScrollbar)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

AwtScrollbar::AwtScrollbar()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtScrollBar");

	m_pWindow = NULL;
	m_bgfgColor[FALSE] = RGB(255, 255, 255);
	m_bgfgColor[TRUE] = RGB(0, 0, 0);
	GetApp()->CreateBrush(m_bgfgColor[FALSE]);
	GetApp()->CreateBrush(m_bgfgColor[TRUE]);
}

AwtScrollbar::~AwtScrollbar()
{
	GetApp()->ReleaseBrush(m_bgfgColor[FALSE]);
	GetApp()->ReleaseBrush(m_bgfgColor[TRUE]);

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtScrollbar::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
							 long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((Hjava_awt_Scrollbar*)p1, (AwtWindow*)p2,
				(BOOL)p3, (BOOL)p4);
			break;
		case DISPOSE:
			((AwtScrollbar*)p1)->Dispose();
			break;
		case SETCOLOR:
			((AwtScrollbar*)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		case SETVALUE:
			((AwtScrollbar*)p1)->SetValue(p2);
			break;
		case SETVALUES:
			((AwtScrollbar*)p1)->SetValues(p2, p3, p4);
			break;
		case MINIMUM:
			return ((AwtScrollbar*)p1)->Minimum();
			break;
		case MAXIMUM:
			return ((AwtScrollbar*)p1)->Maximum();
			break;
		case VALUE:
			return ((AwtScrollbar*)p1)->Value();
			break;
		case SHOW:
			((AwtScrollbar*)p1)->Show((BOOL)p2);
			break;
		case RESHAPE:
			((AwtScrollbar*)p1)->Reshape((BOOL)p2, (long)p3, (long)p4, 
										  (BOOL)p5, (long)p6, (long)p7);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtScrollbar *AwtScrollbar::Create(Hjava_awt_Scrollbar *pJavaObject, AwtWindow *pParent, 
	BOOL bVert, BOOL bStandard)
{
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtScrollbar *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_SCROLLBAR, CREATE,
			(long)pJavaObject, (long)pParent, (long)bVert, (long)bStandard);
	}
	AwtScrollbar *pScrollbar = new AwtScrollbar();

	if (bStandard) {
		pScrollbar->m_pWindow = pParent;
		pParent->ShowScrollBar(bVert ? SB_VERT : SB_HORZ, TRUE);
	} else {
		CRect rect(0, 0, 50, 14);
		DWORD dwDLU = ::GetDialogBaseUnits();
		WORD nXdlu = LOWORD(dwDLU);
		WORD nYdlu = HIWORD(dwDLU);

		// Convert dlu's to pixels
		rect.right = MulDiv(rect.right, nXdlu, 4);
		rect.bottom = MulDiv(rect.bottom, nYdlu, 8);
		pScrollbar->CScrollBar::Create(bVert ? SBS_VERT : SBS_HORZ, rect, pParent, 0);
	}
	pScrollbar->m_pJavaObject = pJavaObject;
	pScrollbar->m_bVert = bVert;
	return pScrollbar;
}

void AwtScrollbar::Dispose()
{
	delete this;
}

void AwtScrollbar::SetColor(BOOL bFg, COLORREF color) {
	GetApp()->ReleaseBrush(m_bgfgColor[bFg]);
	m_bgfgColor[bFg] = color;
	GetApp()->CreateBrush(color);
}

void AwtScrollbar::SetValue(long nValue)
{
	int nMinPos, nMaxPos;

	if (m_pWindow != NULL) {
		::GetScrollRange(m_pWindow->m_hWnd, m_bVert ? SB_VERT : SB_HORZ, &nMinPos, &nMaxPos);
		m_pWindow->SetScrollValues(m_bVert ? SB_VERT : SB_HORZ, nMinPos, nValue, nMaxPos);
	} else {
		SetScrollPos(nValue);
	}
}

void AwtScrollbar::SetValues(long nMinPos, long nValue, long nMaxPos)
{
	if (m_pWindow != NULL) {
		m_pWindow->SetScrollValues(m_bVert ? SB_VERT : SB_HORZ, nMinPos, nValue, nMaxPos);
	} else {
		SetScrollRange(nMinPos, nMaxPos);
		SetScrollPos(nValue);
	}
}

long AwtScrollbar::Minimum()
{
	int nMinPos, nMaxPos;

	if (m_pWindow != NULL) {
		::GetScrollRange(m_pWindow->m_hWnd, m_bVert ? SB_VERT : SB_HORZ, &nMinPos, &nMaxPos);
	} else {
		GetScrollRange(&nMinPos, &nMaxPos);
	}
	return nMinPos;
}

long AwtScrollbar::Maximum()
{
	int nMinPos, nMaxPos;

	if (m_pWindow != NULL) {
		::GetScrollRange(m_pWindow->m_hWnd, m_bVert ? SB_VERT : SB_HORZ, &nMinPos, &nMaxPos);
	} else {
		GetScrollRange(&nMinPos, &nMaxPos);
	}
	return nMaxPos;
}

long AwtScrollbar::Value()
{
	if (m_pWindow != NULL) {
		return ::GetScrollPos(m_pWindow->m_hWnd, m_bVert ? SB_VERT : SB_HORZ);
	} else {
		return GetScrollPos();
	}
}

void AwtScrollbar::Show(BOOL bShow)
{
	if (m_pWindow != NULL) {
		VERIFY(::ShowScrollBar(m_pWindow->m_hWnd, m_bVert ? SB_VERT : SB_HORZ, bShow));
	} else {
		GetApp()->ShowWnd(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
	}
}

void AwtScrollbar::Reshape(BOOL bMove, long nX, long nY, BOOL bResize, long nWidth, long nHeight)
{
	if (m_pWindow != NULL) {
		Classjava_awt_Scrollbar *pClass = unhand(m_pJavaObject);
		if (bMove) {
			pClass->x = nX;
			pClass->y = nY;
		}
		if (bResize) {
			pClass->width = nWidth;
			pClass->height = nHeight;
		}
		VERIFY(SetWindowPos(NULL, nX, nY, nWidth, nHeight,
			SWP_NOACTIVATE|SWP_NOZORDER|(bMove ? 0 : SWP_NOMOVE)
			|(bResize ? 0 : SWP_NOSIZE)));
	}
}


/////////////////////////////////////////////////////////////////////////////
// AwtScrollBar diagnostics

#ifdef _DEBUG
void AwtScrollbar::AssertValid() const
{
	CScrollBar::AssertValid();
}

void AwtScrollbar::Dump(CDumpContext& dc) const
{
	//CWnd::Dump(dc);
	dc << "AwtScrollbar.m_hWnd = " << m_hWnd << "\n";
}

#endif //_DEBUG

