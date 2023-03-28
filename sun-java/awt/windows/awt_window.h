/*
 * @(#)window.h	1.13 95/09/18 Patrick P. Chan
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
// window.h : interface of the AwtWindow class
//
/*
	Implementing mouse semantics:  Whenever the window W has the focus
	and if mouse motion is enabled,	a timer is enabled.  The timer checks 
	the window w underneath the cursor.  If w = W or if w is a descendent 
	of W, a mouse move is generated on W.  w is cached so that the next time
	the timer ticks, if w is still underneath the cursor, there's no need
	to check whether w is a descendent of W.
*/
/////////////////////////////////////////////////////////////////////////////

#ifndef _AWTWINDOW_H_
#define _AWTWINDOW_H_

#include "stdafx.h"
#include "awt.h"
#include "awt_font.h"
#include "awt_image.h"
#include "awt_mainfrm.h"
#include "awt_event.h"

#define ID_TIMER 1

class AwtCWnd : public CWnd
{
public:
	// Device Context associated with this window.
	class AwtSharedDC *m_pSharedDC;
};

class AwtWindow : public AwtCWnd
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, SETCOLOR, SHOWSCROLLBAR, SCROLLWND, 
              SETMARGIN, RESHAPE};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtWindow();
	// Creates a window at (0, 0) with it's own DC.
	virtual ~AwtWindow();
	DECLARE_DYNCREATE(AwtWindow)

// Operations
	static AwtWindow *Create(Hjava_awt_Canvas *pWindowObject, 
		AwtWindow *pParent, 
		long nWidth, long nHeight,
		COLORREF bgColor);
	
	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	// Sets BgFg color.  
	void SetColor(BOOL bFg, COLORREF color);

	// If 'bShow' is TRUE, shows the window's scrollbar; otherwise hides the window's scrollbar.  
	// 'nBar' can be either SB_VERT or SB_HORZ.  
	void ShowScrollBar(UINT nBar, BOOL bShow);

	// Sets the scrollbar values.  'nBar' can be either SB_VERT or SB_HORZ.
	// 'nMin', 'nValue', and 'nMax' can have the value INT_MAX which means that the value
	// should not be changed.
	void SetScrollValues(UINT nBar, int nMin, int nValue, int nMax);

	// Scrolls the entire window by (nDx, nDy) (see ::ScrollWindow).  Also scrolls all the
	// child windows.
	void ScrollWnd(long nDx, long nDy);

	// Returns the frame that contains this window.
	MainFrame *GetFrame();

	// Draws a 3-D border along the perimeter of the window.  The border
	// is 'margin' pixels wide.  The border is inset.
	void SetMargin(long margin);

	// Returns a monotonically increasing unsigned value.  There is no
	// attempt to reclaim command ID's.  The value will wrap to 0 after
	// overflow.
	UINT GetCmdID();

	// If 'bMove', moves the window.  If 'bResize', resizes the window.
	void Reshape(BOOL bMove, long nX, long nY, 
				 BOOL bResize, long nWidth, long nHeight);

    void Draw(CPaintDC& dc);    /// replaces OnDraw(...)
// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_WINDOW, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_WINDOW, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

	// Returns a handle to the AwtApp instance.
//	CAwtApp *GetApp() { return (CAwtApp *)AfxGetApp(); };

// IMPLEMENTATION

// Variables
public:
	// BgFg color.
	COLORREF m_bgfgColor[2];

	// Oak Window object.  Set to non-NULL only
	// after Create has completed.
	Hjava_awt_Canvas *m_pJavaObject;

	// Frame that contains this window.  Set to non-NULL only
	// after Create has completed.
	MainFrame *m_pFrame;

private:
#ifndef MOUSE_EVENT_FILTER
	// TRUE if mouse is down in window.
	BOOL m_bMouseDown;

	// TRUE if mouse is within the visible domain of the window.  Set to FALSE
	// whenever the timer is killed.
	BOOL m_bMouseInside;

	// Holds the most recent coordinate from a move message.
	CPoint m_curPt;

	// Window that's currently under the cursor.  Used by
	// the timer.  When the timer is disabled this variable should
	// be set to NULL.
	HWND m_hCurWnd;

	// TRUE if m_hCurWnd is this window or a descendent of this window.
	// Set to FALSE if m_hCurWnd == NULL
	BOOL m_bCurWndIsChild;
#endif
	// Size of border around window.
	int m_nBorderSize;

	// When client asks for cmd ID, give it the value in this variable.
	UINT m_nNextCmdID;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


protected:
// Operations
	// Generates the various mouse messages.
	void MouseMoved(UINT nFlags, CPoint &point);

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AwtWindow)
///	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Generated message map functions
protected:
	//{{AFX_MSG(AwtWindow)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern long GetModifiers(UINT nFlags);
extern __int64 nowMillis();
extern int GetClickCount();
extern int translateToAscii(UINT *nChar, long modifiers);
extern int clickCount;


/////////////////////////////////////////////////////////////////////////////

#endif // _AWTWINDOW_H
