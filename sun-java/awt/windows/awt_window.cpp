/*
 * @(#)awt_window.cpp	1.49 96/01/05
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
// awtview.cpp : implementation of the AwtWindow class
//

#include "stdafx.h"
#include "limits.h"

#include "awt.h"
#include "awtapp.h"
#include "awt_comp.h"
#include "awt_button.h"
#include "awt_edit.h"
#include "awt_list.h"
#include "awt_sbar.h"
#include "awt_window.h"
#include "awt_graphics.h"
#include "java_awt_Event.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static int caps_lock = 0;
static UINT multiClickTime = 0;
static ULONG lastTime = 0;
extern int GetClickCount();
int clickCount = 0;

/////////////////////////////////////////////////////////////////////////////
// AwtWindow

IMPLEMENT_DYNCREATE(AwtWindow, CWnd)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(AwtWindow, CWnd)
	//{{AFX_MSG_MAP(AwtWindow)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
	// Standard printing commands
	//ON_COMMAND(ID_FILE_PRINT, CWnd::OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_PREVIEW, CWnd::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Construction/destruction

AwtWindow::AwtWindow()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtWindow");

	m_pJavaObject = NULL;
	m_pFrame = NULL;
        m_pSharedDC = NULL;
#ifndef MOUSE_EVENT_FILTER
	m_bMouseDown = FALSE;
	m_bMouseInside = FALSE;
#endif
	m_nBorderSize = 0;
	m_nNextCmdID = 1;

	m_bgfgColor[FALSE] = RGB(255, 255, 255);
	m_bgfgColor[TRUE] = RGB(0, 0, 0);
	GetApp()->CreateBrush(m_bgfgColor[FALSE]);
	GetApp()->CreateBrush(m_bgfgColor[TRUE]);
        ((CAwtApp *)AfxGetApp())->cleanupListAdd(this);

        multiClickTime = GetDoubleClickTime();
}

AwtWindow::~AwtWindow()
{
    ((CAwtApp *)AfxGetApp())->cleanupListRemove(this);
    GetApp()->ReleaseBrush(m_bgfgColor[FALSE]);
    GetApp()->ReleaseBrush(m_bgfgColor[TRUE]);
    if (m_pSharedDC != NULL) {
        m_pSharedDC->Detach(NULL);
        m_pSharedDC = NULL;
    }
    DestroyWindow();
    m_hWnd = NULL;

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

/////////////////////////////////////////////////////////////////////////////
// AwtWindow operations

long AwtWindow::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						  long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:				
			return (long)Create((Hjava_awt_Canvas*)p1, (AwtWindow*)p2, (long)p3,
					(long)p4, (COLORREF)p5);
			break;
		case DISPOSE:
			((AwtWindow *)p1)->Dispose();
			break;
		case SETMARGIN:
			((AwtWindow *)p1)->SetMargin((long)p2);
			break;
		case SHOWSCROLLBAR:
			((AwtWindow *)p1)->ShowScrollBar((UINT)p2, (BOOL)p3);
			break;
		case SCROLLWND:
			((AwtWindow *)p1)->ScrollWnd((int)p2, (int)p3);
			break;
		case SETCOLOR:
			((AwtButton*)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		case RESHAPE:
			((AwtWindow*)p1)->Reshape((BOOL)p2, (long)p3, (long)p4, 
										  (BOOL)p5, (long)p6, (long)p7);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtWindow *AwtWindow::Create(Hjava_awt_Canvas *pJavaObject, AwtWindow *pParent, 
	long nWidth, long nHeight,
	COLORREF bgColor)
{	
	Classjava_awt_Canvas *pClass = unhand(pJavaObject);
	CWnd *pWnd;
	AwtWindow *pWindow;
	
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtWindow *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_WINDOW, CREATE, 
			(long)pJavaObject, (long)pParent, (long)nWidth, (long)nHeight,
				(long)bgColor);
	}
	if (pParent->IsKindOf(RUNTIME_CLASS(MainFrame))) {
		MainFrame *pFrame = (MainFrame *)pParent;
		pParent = (AwtWindow*)pFrame->GetMainWindow();
	}
	pWindow = new AwtWindow();
	long nX = pClass->x;
	long nY = pClass->y;
	pClass->width = nWidth;
	pClass->height = nHeight;

	VERIFY(pWindow->CreateEx(0, NULL, NULL, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 
		nX, nY, nWidth, nHeight, pParent->GetSafeHwnd(), 0, NULL));
	pWindow->SetColor(FALSE, bgColor);
/// rjp - causes deadlock        pWindow->SetFocus();

	// find frame
	pWnd = pWindow->GetParent();
	while (pWnd != NULL && !pWnd->IsKindOf(RUNTIME_CLASS(MainFrame)))
		pWnd = pWnd->GetParent();
	pWindow->m_pFrame = (MainFrame *)pWnd;
	pWindow->m_pJavaObject = pJavaObject;

// This is too expensive.
// Need to change to a scheme where there's only one timer for the whole application.
// Every window would "register" when the mouse entered, then the timer would keep
// checking if the cursor was still in the registered window.  If not,
// call the mouseExit handler.
VERIFY(pWindow->SetTimer(ID_TIMER, 200, NULL));

	return pWindow;
}

void AwtWindow::Dispose()
{
    delete this;
}

void AwtWindow::SetColor(BOOL bFg, COLORREF color)
{
	GetApp()->ReleaseBrush(m_bgfgColor[bFg]);
	m_bgfgColor[bFg] = color;
	GetApp()->CreateBrush(color);
}

void AwtWindow::ShowScrollBar(UINT nBar, BOOL bShow) {
	CWnd::ShowScrollBar(nBar, bShow);
}

void AwtWindow::SetScrollValues(UINT nBar, int nMin, int nValue, int nMax)
{
	int nMinTmp, nMaxTmp;

	GetScrollRange(nBar, &nMinTmp, &nMaxTmp);
	if (nMin == INT_MAX) {
		nMin = nMinTmp;
	}
	if (nValue == INT_MAX) {
		nValue = GetScrollPos(nBar);
	}
	if (nMax == INT_MAX) {
		nMax = nMaxTmp;
	}
	if (nMin == nMax) {
		nMax++;
	}
	SetScrollRange(nBar, nMin, nMax, TRUE);
	SetScrollPos(nBar, nValue);
}

void AwtWindow::ScrollWnd(long nDx, long nDy)
{
	ScrollWindow(nDx, nDy, NULL);
	UpdateWindow();
}

MainFrame *AwtWindow::GetFrame()
{
	return m_pFrame;
}

void AwtWindow::SetMargin(long margin)
{
	m_nBorderSize = margin;
	if (m_nBorderSize > 0) {
		ModifyStyle(0, WS_BORDER);
		ModifyStyleEx(0, GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0);
	} else {
		ModifyStyle(WS_BORDER, 0);
		ModifyStyleEx(GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0, 0);
	}
	InvalidateRect(NULL);
}

UINT AwtWindow::GetCmdID()
{
	return m_nNextCmdID++;
}

void AwtWindow::Reshape(BOOL bMove, long nX, long nY, BOOL bResize, long nWidth, long nHeight)
{
	Classjava_awt_Canvas *pClass = unhand(m_pJavaObject);

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

/////////////////////////////////////////////////////////////////////////////
// AwtWindow overrides

void AwtWindow::OnPaint() {
    CPaintDC dc(this);
    AWT_TRACE(("window.paint[%d, %d, %d, %d] erasebg=%s\n",
                dc.m_ps.rcPaint.left,
                dc.m_ps.rcPaint.top,
                dc.m_ps.rcPaint.right,
                dc.m_ps.rcPaint.bottom,
                (dc.m_ps.fErase ? "TRUE" : "FALSE")));
    Draw(dc);
}

void AwtWindow::Draw(CPaintDC& dc) 
{
	CRect rect;

    rect = dc.m_ps.rcPaint;
    // Do not send expose if area is 0
    if( rect.Width() && rect.Height() ) {
        if( dc.m_ps.fErase ) {
          	VERIFY(::FillRect(dc.m_hDC, rect, GetApp()->GetBrush(m_bgfgColor[FALSE])));
        }
    	if (m_pJavaObject != NULL) {
    		AWT_TRACE(("[%x]:window.handleExpose(%d, %d, %d, %d)\n", 
    			m_hWnd, rect.left, rect.top, rect.Width(), rect.Height()));
    		GetApp()->DoCallback(m_pJavaObject, "handleExpose", "(IIII)V",
    			4, rect.left, rect.top, rect.Width(), rect.Height());
        }
    }
}

BOOL AwtWindow::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    ::SetCursor(m_pFrame->m_cursor);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// AwtWindow printing

BOOL AwtWindow::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	//return DoPreparePrinting(pInfo);
	return FALSE;
}

void AwtWindow::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void AwtWindow::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// AwtWindow diagnostics

#ifdef _DEBUG
void AwtWindow::AssertValid() const
{
	CWnd::AssertValid();
}

void AwtWindow::Dump(CDumpContext& dc) const
{
	//CWnd::Dump(dc);
	dc << "\nAwtWindow.m_hWnd = " << (UINT)m_hWnd << "\n";
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// AwtWindow message handlers

BOOL AwtWindow::PreCreateWindow(CREATESTRUCT& cs) 
{
	static CString pzClassName;
	
	if (pzClassName.GetLength() == 0) {
 		pzClassName = AfxRegisterWndClass(0,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	cs.lpszClass = pzClassName;
	return CWnd::PreCreateWindow(cs);
}

void AwtWindow::OnSize(UINT nType, int cx, int cy) 
{
	Classjava_awt_Canvas *pClass;
	CRect rect;
	
	CWnd::OnSize(nType, cx, cy);

	if (m_pJavaObject == NULL) {
		return;
	}
	pClass = unhand(m_pJavaObject);
        GetWindowRect(&rect);
 	pClass->width = rect.Width();
 	pClass->height = rect.Height();
 	pClass->x = rect.left;
 	pClass->y = rect.top;
}

// Converts Windows modifier bits to awt modifier bits.
long GetModifiers(UINT nFlags)
{
	int nModifiers = 0;
	int state;

	if ((1<<24) & nFlags) {
		nModifiers |= java_awt_Event_META_MASK;
	}	
	state = ::GetAsyncKeyState(VK_CONTROL);
	if (HIBYTE(state)) {
		nModifiers |= java_awt_Event_CTRL_MASK;
	}		
	state = ::GetAsyncKeyState(VK_RBUTTON);
	if (HIBYTE(state)) {
	    nModifiers |= java_awt_Event_META_MASK;
	}
	state = ::GetAsyncKeyState(VK_SHIFT);
	if (HIBYTE(state)) {
		nModifiers |= java_awt_Event_SHIFT_MASK;
	}
	state = ::GetAsyncKeyState(VK_MBUTTON);
	if (HIBYTE(state)) {
	    nModifiers |= java_awt_Event_ALT_MASK;
	}
	return nModifiers;
}

#define FT2INT64(ft) \
	((__int64)(ft).dwHighDateTime << 32 | (__int64)(ft).dwLowDateTime)

__int64 nowMillis()
{
    SYSTEMTIME st, st0;
    FILETIME   ft, ft0;

    (void)memset(&st0, 0, sizeof st0);
    st0.wYear  = 1970;
    st0.wMonth = 1;
    st0.wDay   = 1;
    SystemTimeToFileTime(&st0, &ft0);

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    return (FT2INT64(ft) - FT2INT64(ft0)) / 10000;
}

int GetClickCount()
{
    ULONG now = (ULONG)nowMillis();
    if ((now - lastTime) <= (ULONG)multiClickTime) {
        clickCount++;
    } else {
        clickCount = 1;
    }
    lastTime = now;
    return clickCount;
}

void AwtWindow::OnLButtonDown(UINT nFlags, CPoint point) 
{					
	if (m_pJavaObject == NULL) {
		CWnd::OnLButtonDown(nFlags, point);
		return;
	}
	SetFocus();
	SetCapture();

#ifndef MOUSE_EVENT_FILTER
	if (m_bMouseDown) {	
		// Missed the buttonUp so simulate it at this point.
		OnLButtonUp(nFlags, point);
	}
	if (!m_bMouseInside) {
		// Simulate a mouse move to this position since we missed it.
		MouseMoved(nFlags, point);
	}

	m_bMouseDown = TRUE;
	AWT_TRACE(("[%x]:handleMouseDown(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, "handleMouseDown", "(JIIIII)V", 7,
                 (long)(tm&0xffffffff), (long)(tm>>32), NULL, point.x, point.y, 
                 GetClickCount(), GetModifiers(nFlags));
#endif
}

void AwtWindow::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// Ignore button up if the button down was in another window.
#ifdef MOUSE_EVENT_FILTER
	if (::GetCapture() != m_hWnd) {
#else
	if (m_pJavaObject == NULL || !m_bMouseDown) {
#endif
		CWnd::OnLButtonUp(nFlags, point);
		return;
	}
	
	::ReleaseCapture();

#ifndef MOUSE_EVENT_FILTER
	// Simulate a mouse move to this position just in case we missed it.
	MouseMoved(nFlags, point);

	m_bMouseDown = FALSE;
	AWT_TRACE(("[%x]:handleMouseUp(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, "handleMouseUp", "(JIIII)V", 6, 
                             (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                             point.x, point.y, GetModifiers(nFlags));
#endif
}

void AwtWindow::OnMButtonDown(UINT nFlags, CPoint point) 
{					
	if (m_pJavaObject == NULL) {
		CWnd::OnMButtonDown(nFlags, point);
		return;
	}
	SetCapture();

#ifndef MOUSE_EVENT_FILTER
	if (m_bMouseDown) {	
		// Missed the buttonUp so simulate it at this point.
		OnMButtonUp(nFlags, point);
	}
	if (!m_bMouseInside) {
		// Simulate a mouse move to this position since we missed it.
		MouseMoved(nFlags, point);
	}

	m_bMouseDown = TRUE;
	AWT_TRACE(("[%x]:handleMouseDown(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, "handleMouseDown", "(JIIIII)V", 7, 
		(long)(tm&0xffffffff), (long)(tm>>32), NULL, point.x, point.y, 
                GetClickCount(), GetModifiers(nFlags));
#endif
}

void AwtWindow::OnMButtonUp(UINT nFlags, CPoint point) 
{
	// Ignore button up if the button down was in another window.
#ifdef MOUSE_EVENT_FILTER
	if (::GetCapture() != m_hWnd) {
#else
	if (m_pJavaObject == NULL || !m_bMouseDown) {
#endif
		CWnd::OnMButtonUp(nFlags, point);
		return;
	}
	
	::ReleaseCapture();

#ifndef MOUSE_EVENT_FILTER
	// Simulate a mouse move to this position just in case we missed it.
	MouseMoved(nFlags, point);

	m_bMouseDown = FALSE;
	AWT_TRACE(("[%x]:handleMouseUp(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, "handleMouseUp", "(JIIII)V", 6, 
                             (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                             point.x, point.y, GetModifiers(nFlags));
#endif
}

void AwtWindow::OnRButtonDown(UINT nFlags, CPoint point) 
{					
	if (m_pJavaObject == NULL) {
		CWnd::OnRButtonDown(nFlags, point);
		return;
	}
	SetCapture();

#ifndef MOUSE_EVENT_FILTER
	if (m_bMouseDown) {	
		// Missed the buttonUp so simulate it at this point.
		OnRButtonUp(nFlags, point);
	}
	if (!m_bMouseInside) {
		// Simulate a mouse move to this position since we missed it.
		MouseMoved(nFlags, point);
	}

	m_bMouseDown = TRUE;
	AWT_TRACE(("[%x]:handleMouseDown(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, "handleMouseDown", "(JIIIII)V", 7, 
		(long)(tm&0xffffffff), (long)(tm>>32), NULL, point.x, point.y, 
                GetClickCount(), GetModifiers(nFlags));
#endif
}

void AwtWindow::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// Ignore button up if the button down was in another window.
#ifdef MOUSE_EVENT_FILTER
	if (::GetCapture() != m_hWnd) {
#else
	if (m_pJavaObject == NULL || !m_bMouseDown) {
#endif
		CWnd::OnRButtonUp(nFlags, point);
		return;
	}
	
	::ReleaseCapture();

#ifndef MOUSE_EVENT_FILTER
	// Simulate a mouse move to this position just in case we missed it.
	MouseMoved(nFlags, point);

	m_bMouseDown = FALSE;
	AWT_TRACE(("[%x]:handleMouseUp(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, "handleMouseUp", "(JIIII)V", 6, 
                             (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                             point.x, point.y, GetModifiers(nFlags));
#endif
}

void AwtWindow::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_pJavaObject == NULL) {
		CWnd::OnMouseMove(nFlags, point);
		return;
	}

#ifndef MOUSE_EVENT_FILTER
	if ((nFlags&(MK_LBUTTON|MK_MBUTTON|MK_RBUTTON)) && !m_bMouseDown) {
		// The button down message was missed.  This can happen if the window
		// got the focus while some button was down.  Simulate a button down.
		m_bMouseDown = TRUE;
		AWT_TRACE(("[%x]:handleMouseDown(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
		__int64 tm = nowMillis();
		GetApp()->DoCallback(m_pJavaObject, "handleMouseDown", "(JIIIII)V", 7,
			(long)(tm&0xffffffff), (long)(tm>>32), NULL, point.x, point.y, 
                        GetClickCount(), GetModifiers(nFlags));
	}

	MouseMoved(nFlags, point);
#endif
}											 

void AwtWindow::MouseMoved(UINT nFlags, CPoint &point)
{
#ifndef MOUSE_EVENT_FILTER
	CRect domain;
	BOOL bMouseInside;
	CPoint screenPt;
  
	// test if pt is in visible portion of window or in any one of its descendents
	GetClientRect(domain);
	bMouseInside = domain.PtInRect(point);
	if (bMouseInside) {
		// is inside the domain but is it on some descendent?
		HWND hWnd;
		CPoint screenPt = point;

		ClientToScreen(&screenPt);
		hWnd = ::WindowFromPoint(screenPt);
  	
		if (hWnd != m_hCurWnd) {
			m_bCurWndIsChild = hWnd == m_hWnd || ::IsChild(m_hWnd, hWnd);
			m_hCurWnd = hWnd;
		}
		bMouseInside = m_bCurWndIsChild;
	}
  
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	if (bMouseInside && !m_bMouseInside) {
		m_bMouseInside = TRUE;
		AWT_TRACE(("[%x]:handleMouseEnter(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
		GetApp()->DoCallback(m_pJavaObject, "handleMouseEnter", "(JII)V", 4, 
			(long)(tm&0xffffffff), (long)(tm>>32), point.x, point.y);
	} else if (!bMouseInside && m_bMouseInside) {
		m_bMouseInside = FALSE;
		AWT_TRACE(("[%x]:handleMouseExit(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
		GetApp()->DoCallback(m_pJavaObject, "handleMouseExit", "(JII)V", 4, 
			(long)(tm&0xffffffff), (long)(tm>>32), point.x, point.y);
	}
	if (m_bMouseDown) {
		AWT_TRACE(("[%x]:handleMouseDrag(%d, %d, %d)\n", m_hWnd, 
                           point.x, point.y, GetModifiers(nFlags)));
		GetApp()->DoCallback(m_pJavaObject, "handleMouseDrag", "(JIIII)V",
                                     6, (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                                     point.x, point.y, GetModifiers(nFlags));
	} else if (bMouseInside) {
		AWT_TRACE(("[%x]:handleMouseMoved(%d, %d, %d)\n", m_hWnd, point.x, point.y, GetModifiers(nFlags)));
		GetApp()->DoCallback(m_pJavaObject, "handleMouseMoved", "(JIIII)V", 
                                     6, (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                                     point.x, point.y, GetModifiers(nFlags));
	}
	m_curPt = point;
#endif
}

void AwtWindow::OnTimer(UINT nIDEvent) 
{
	CALLINGWINDOWS(m_hWnd);
	CWnd::OnTimer(nIDEvent);
	CALLINGWINDOWS(m_hWnd);

#ifndef MOUSE_EVENT_FILTER
	// If button down, drag messages will be generated from
	// move messages.
	if (!m_bMouseDown && nIDEvent == ID_TIMER) {
		CPoint pt;

		if (GetCursorPos(&pt)) {
			ScreenToClient(&pt);
			if (m_curPt != pt) {
				MouseMoved(0, pt);
			}
		}
	}
#endif
}

void AwtWindow::OnKillFocus(CWnd* pNewWnd) 
{
	CALLINGWINDOWS(m_hWnd);
	CWnd::OnKillFocus(pNewWnd);

#ifndef MOUSE_EVENT_FILTER	
	if (m_pJavaObject == NULL) {
		return;
	}
	MouseMoved(0, CPoint(-1, -1));
	// Simulate mouseUp.
	if (m_bMouseDown) {
		m_bMouseDown = FALSE;
		CALLINGWINDOWS(m_hWnd);
		::ReleaseCapture();
		AWT_TRACE(("[%x]:handleMouseUp(%d, %d, %d)\n", m_hWnd, -1, -1, 0));
		__int64 tm = nowMillis();		// kludge for passing in 64-bit values
		GetApp()->DoCallback(m_pJavaObject, "handleMouseUp", "(JIIII)V", 6,
			(long)(tm&0xffffffff), (long)(tm>>32), NULL, -1, -1, 0);
	}
	AWT_TRACE(("[%x]:lostFocus()\n", m_hWnd));
	GetApp()->DoCallback(m_pJavaObject, "lostFocus", "(I)V", 1, NULL);
	//KillTimer(ID_TIMER);
	m_bMouseInside = FALSE;
	m_hCurWnd = NULL;
#endif
}

void AwtWindow::OnSetFocus(CWnd* pOldWnd) 
{
	CALLINGWINDOWS(m_hWnd);
	CWnd::OnSetFocus(pOldWnd);

#ifndef MOUSE_EVENT_FILTER	
	if (m_pJavaObject == NULL) {
		return;
	}

	AWT_TRACE(("[%x]:gotFocus()\n", m_hWnd));
	GetApp()->DoCallback(m_pJavaObject, "gotFocus", "(I)V", NULL);

	// Simulate mouse move in case we missed it.
	CPoint pt;
	if (GetCursorPos(&pt)) {
		ScreenToClient(&pt);
		MouseMoved(0, pt);
	}
#endif

        VERIFY(SetTimer(ID_TIMER, 200, NULL));
}

int translateToAscii(UINT *nChar, long modifiers)
{
    int state;

    *nChar = MapVirtualKey(*nChar, 2);	
    state = ::GetAsyncKeyState(VK_CAPITAL);

    if (LOBYTE(state)) {
	caps_lock = !caps_lock;
    }
    if (caps_lock != 0) {
	if (*nChar >= 'a' && *nChar <= 'z') {
	    *nChar = (*nChar - 'a') + 'A';
	}
    }
    if ((modifiers&java_awt_Event_SHIFT_MASK) == 0) {
	if (caps_lock == 0) {
	    if (*nChar >= 'A' && *nChar <= 'Z') {
		*nChar = (*nChar - 'A') + 'a';
	    }
	}
    } else {
	switch (*nChar) {
	  case '`':
	    *nChar = '~';
	    break;
	  case '1':
	    *nChar = '!';
	    break;
	  case '2':
	    *nChar = '@';
	    break;
	  case '3':
	    *nChar = '#';
	    break;
	  case '4':
	    *nChar = '$';
	    break;
	  case '5':
	    *nChar = '%';
	    break;
	  case '6':
	    *nChar = '^';
	    break;
	  case '7':
	    *nChar = '&';
	    break;
	  case '8':
	    *nChar = '*';
	    break;
	  case '9':
	    *nChar = '(';
	    break;	
	  case '0':
	    *nChar = ')';
	    break;
	  case '-':
	    *nChar = '_';
	    break;
	  case '=':
	    *nChar = '+';
	    break;
	  case '\\':
	    *nChar = '|';
	    break;
	  case '[':
	    *nChar = '{';
	    break;
	  case ']':
	    *nChar = '}';
	    break;
	  case ';':
	    *nChar = ':';
	    break;
	  case '\'':
	    *nChar = '"';
	    break;
	  case ',':
	    *nChar = '<';
	    break;
	  case '.':
	    *nChar = '>';
	    break;
	  case '/':
	    *nChar = '?';
	    break;
	  default:
	    break;
	}
    }
    if ((modifiers&java_awt_Event_CTRL_MASK) != 0) {
        switch (*nChar) {
        case '[':
        case ']':
        case '\\':
        case '_':
	    *nChar -= 64;
            break;
        default:
	   if (isalpha(*nChar)) {
	       *nChar = tolower(*nChar) - 'a' + 1;
	   }
       }
    }

    return 1;
}

void AwtWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
#ifndef KEYBOARD_EVENT_FILTER
	clickCount = 0;
    	char *upCall = "handleActionKey";

	switch (nChar) {
	case VK_HOME:
		nChar = java_awt_Event_HOME;
		break;
	case VK_END:
		nChar = java_awt_Event_END;
		break;
	case VK_PRIOR:
		nChar = java_awt_Event_PGUP;
		break;
	case VK_NEXT:
		nChar = java_awt_Event_PGDN;
		break;
	case VK_UP:
		nChar = java_awt_Event_UP;
		break;
	case VK_DOWN:
		nChar = java_awt_Event_DOWN;
		break;
	case VK_RIGHT:
		nChar = java_awt_Event_RIGHT;
		break;
	case VK_LEFT:
		nChar = java_awt_Event_LEFT;
		break;
	case VK_F1:
		nChar = java_awt_Event_F1;
		break;
	case VK_F2:
		nChar = java_awt_Event_F2;
		break;
	case VK_F3:
		nChar = java_awt_Event_F3;
		break;
	case VK_F4:
		nChar = java_awt_Event_F4;
		break;
	case VK_F5:
		nChar = java_awt_Event_F5;
		break;
	case VK_F6:
		nChar = java_awt_Event_F6;
		break;
	case VK_F7:
		nChar = java_awt_Event_F7;
		break;
	case VK_F8:
		nChar = java_awt_Event_F8;
		break;
	case VK_F9:
		nChar = java_awt_Event_F9;
		break;
	case VK_F10:
		nChar = java_awt_Event_F10;
		break;
	case VK_F11:
		nChar = java_awt_Event_F11;
		break;
	case VK_F12:
		nChar = java_awt_Event_F12;
		break;
	case VK_DELETE:
		nChar = '\177';
		upCall = "handleKeyPress";
		break;
	default:
		return;
		break;
	} 
	AWT_TRACE(("[%x]:%s(%d, %d, %d, %d)\n", 
		m_hWnd, upCall, m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags)));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, upCall, "(JIIIII)V", 7, 
                             (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                             m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags));
#endif
}

void AwtWindow::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
#ifndef KEYBOARD_EVENT_FILTER
	char *upCall = "handleActionKeyRelease";
	long modifiers = GetModifiers(nFlags);
        clickCount = 0;

	switch (nChar) {
	case VK_HOME:
		nChar = java_awt_Event_HOME;
		break;
	case VK_END:
		nChar = java_awt_Event_END;
		break;
	case VK_PRIOR:
		nChar = java_awt_Event_PGUP;
		break;
	case VK_NEXT:
		nChar = java_awt_Event_PGDN;
		break;
	case VK_UP:
		nChar = java_awt_Event_UP;
		break;
	case VK_DOWN:
		nChar = java_awt_Event_DOWN;
		break;
	case VK_RIGHT:
		nChar = java_awt_Event_RIGHT;
		break;
	case VK_LEFT:
		nChar = java_awt_Event_LEFT;
		break;
	case VK_F1:
		nChar = java_awt_Event_F1;
		break;
	case VK_F2:
		nChar = java_awt_Event_F2;
		break;
	case VK_F3:
		nChar = java_awt_Event_F3;
		break;
	case VK_F4:
		nChar = java_awt_Event_F4;
		break;
	case VK_F5:
		nChar = java_awt_Event_F5;
		break;
	case VK_F6:
		nChar = java_awt_Event_F6;
		break;
	case VK_F7:
		nChar = java_awt_Event_F7;
		break;
	case VK_F8:
		nChar = java_awt_Event_F8;
		break;
	case VK_F9:
		nChar = java_awt_Event_F9;
		break;
	case VK_F10:
		nChar = java_awt_Event_F10;
		break;
	case VK_F11:
		nChar = java_awt_Event_F11;
		break;
	case VK_F12:
		nChar = java_awt_Event_F12;
		break;
	case VK_DELETE:
		nChar = '\177';
		upCall = "handleKeyRelease";
		break;
	case VK_RETURN:
		nChar = '\n';
		upCall = "handleKeyRelease";
		break;
	default:
		if (!translateToAscii(&nChar, modifiers)) {
		    return;
		}
		upCall = "handleKeyRelease";
		break;
	} 
	AWT_TRACE(("[%x]:%s(%d, %d, %d, %d)\n", 
		m_hWnd, upCall, m_curPt.x, m_curPt.y, nChar, modifiers));
	__int64 tm = nowMillis();		// kludge for passing in 64-bit values
	GetApp()->DoCallback(m_pJavaObject, upCall, 
				"(JIIIII)V", 7,
				(long)(tm&0xffffffff),
				(long)(tm>>32), NULL, 
				m_curPt.x, m_curPt.y, nChar, modifiers);
#endif
}

void AwtWindow::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
#ifndef KEYBOARD_EVENT_FILTER
    clickCount = 0;
    switch (nChar) {
      case VK_RETURN:	// Enter
	nChar = '\n';
	break;
      case VK_DELETE:
	return;
	break;
      default:
	break;
    } 
    AWT_TRACE(("[%x]:handleKeyPress(%d, %d, %d, %d)\n", 
				    m_hWnd, m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags)));
    __int64 tm = nowMillis();		// kludge for passing in 64-bit values
    GetApp()->DoCallback(m_pJavaObject, "handleKeyPress", "(JIIIII)V", 7, 
			 (long)(tm&0xffffffff), (long)(tm>>32), NULL,
                         m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags));
#endif
}

HBRUSH AwtWindow::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
    // Win95 doesn't return the correct nCtlColor item (NT does), so we 
    // can't switch based on that value.
    if (pWnd->IsKindOf(RUNTIME_CLASS(AwtButton))) {
        AwtButton *pButton = (AwtButton*)pWnd;
        VERIFY(::SetBkColor(pDC->m_hDC, pButton->m_bgfgColor[FALSE]) != CLR_INVALID);
        VERIFY(::SetBkMode(pDC->m_hDC, TRANSPARENT) != NULL);
        return GetApp()->GetBrush(pButton->m_bgfgColor[FALSE]);
    }

    if (pWnd->IsKindOf(RUNTIME_CLASS(AwtEdit))) {
        AwtEdit *pEdit = (AwtEdit*)pWnd;

        VERIFY(::SetBkColor(pDC->m_hDC, pEdit->m_bgfgColor[FALSE]) != CLR_INVALID);
        VERIFY(::SetTextColor(pDC->m_hDC, pEdit->m_bgfgColor[TRUE]) != CLR_INVALID);
        return GetApp()->GetBrush(pEdit->m_bgfgColor[FALSE]);
    }

    if (pWnd->IsKindOf(RUNTIME_CLASS(AwtScrollbar))) {
        AwtScrollbar *pSbar = (AwtScrollbar*)pWnd;
        VERIFY(::SetBkColor(pDC->m_hDC, pSbar->m_bgfgColor[FALSE]) != CLR_INVALID);
        VERIFY(::SetTextColor(pDC->m_hDC, pSbar->m_bgfgColor[TRUE]) != CLR_INVALID);
        return GetApp()->GetBrush(pSbar->m_bgfgColor[FALSE]);
    }

    if (pWnd->IsKindOf(RUNTIME_CLASS(AwtList))) {
        AwtList *pList = (AwtList*)pWnd;
        VERIFY(::SetBkColor(pDC->m_hDC, pList->m_bgfgColor[FALSE]) != CLR_INVALID);
        VERIFY(::SetTextColor(pDC->m_hDC, pList->m_bgfgColor[TRUE]) != CLR_INVALID);
        return GetApp()->GetBrush(pList->m_bgfgColor[FALSE]);
    }

    CALLINGWINDOWS(m_hWnd);
    return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
}

BOOL AwtWindow::OnEraseBkgnd(CDC* pDC) 
{
    return FALSE;
}

void AwtWindow::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	char *pEvent = NULL;
	int nMin, nMax;
	AwtScrollbar *pAwtScrollbar;
	
	if (pScrollBar == NULL) {
		return;
	}
	pAwtScrollbar = (AwtScrollbar*)pScrollBar;
	pAwtScrollbar->GetScrollRange(&nMin, &nMax);
	switch (nSBCode) {
	case SB_BOTTOM:
		break;
	case SB_ENDSCROLL:
		break;
	case SB_LINEDOWN:
		nPos = pAwtScrollbar->GetScrollPos();
		if ((int)nPos < nMax) {
			nPos = min((int)nPos + 1, nMax);
			pEvent = nSBCode == SB_LINEDOWN ? "lineDown" : "lineUp";
		}
		break;
	case SB_LINEUP:
		nPos = pAwtScrollbar->GetScrollPos();
		if ((int)nPos > nMin) {
			nPos = max(nMin, (int)nPos - 1);
			pEvent = nSBCode == SB_LINEDOWN ? "lineDown" : "lineUp";
		}
		break;
	case SB_PAGEDOWN:
		nPos = pAwtScrollbar->GetScrollPos();
		nPos = min((int)nPos + 10, nMax);
		pEvent = "pageDown";
		break;
	case SB_PAGEUP:
		nPos = pAwtScrollbar->GetScrollPos();
		nPos = max((int)nPos - 10, nMin);
		pEvent = "pageUp";
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		pEvent = "dragAbsolute";
		break;
	case SB_TOP:
		break;
	}

	void *pJavaObject = m_pJavaObject;
	if (pAwtScrollbar != NULL) {
		pJavaObject = pAwtScrollbar->m_pJavaObject;
	}
	if (pEvent != NULL) {
		AWT_TRACE(("[%x]:%s\n", m_hWnd, pEvent));
		GetApp()->DoCallback(pJavaObject, pEvent, "(I)V", 1, nPos);
	}

	CALLINGWINDOWS(m_hWnd);
	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void AwtWindow::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	char *pEvent = NULL;
	int nMin, nMax;
	AwtScrollbar *pAwtScrollbar;
	
	if (pScrollBar == NULL) {
		return;
	}
	pAwtScrollbar = (AwtScrollbar*)pScrollBar;
	
	pAwtScrollbar->GetScrollRange(&nMin, &nMax);
	switch (nSBCode) {
	case SB_BOTTOM:
		break;
	case SB_ENDSCROLL:
		break;
	case SB_LINEDOWN:
		nPos = pAwtScrollbar->GetScrollPos();
		if ((int)nPos < nMax) {
			nPos = min((int)nPos + 1, nMax);
			pEvent = nSBCode == SB_LINEDOWN ? "lineDown" : "lineUp";
		}
		break;
	case SB_LINEUP:
		nPos = pAwtScrollbar->GetScrollPos();
		if ((int)nPos > nMin) {
			nPos = max(nMin, (int)nPos - 1);
			pEvent = nSBCode == SB_LINEDOWN ? "lineDown" : "lineUp";
		}
		break;
	case SB_PAGEDOWN:
		nPos = pAwtScrollbar->GetScrollPos();
		nPos = min((int)nPos + 10, nMax);
		pEvent = "pageDown";
		break;
	case SB_PAGEUP:
		nPos = pAwtScrollbar->GetScrollPos();
		nPos = max((int)nPos - 10, nMin);
		pEvent = "pageUp";
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		pEvent = "dragAbsolute";
		break;
	case SB_TOP:
		break;
	}

	void *pJavaObject = m_pJavaObject;
	if (pAwtScrollbar != NULL) {
		pJavaObject = pAwtScrollbar->m_pJavaObject;
	}
	if (pEvent != NULL) {
		AWT_TRACE(("[%x]:%s\n", m_hWnd, pEvent));
		GetApp()->DoCallback(pJavaObject, pEvent, "(I)V", 1, nPos);
	}
	
	CALLINGWINDOWS(m_hWnd);
	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}
