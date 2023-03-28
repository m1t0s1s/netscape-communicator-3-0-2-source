/*
 * 95/12/09 @(#)awt_mainfrm.cpp	1.30
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
// mainfrm.cpp : implementation of the MainFrame class
//

#include "stdafx.h"
#include "awt.h"
#include "awtapp.h"
#include "awt_mainfrm.h"		 
#include "awt_menu.h"
#include "awt_button.h"
#include "awt_optmenu.h"
#include "awt_window.h"
#include "java_awt_Insets.h"

#ifdef NETSCAPE 
#include "java_applet_AppletContext.h"
#include "netscape_applet_MozillaAppletContext.h"
#include "netscape_applet_EmbeddedAppletFrame.h"
#include <java.h>   // LJAppletData 
#endif  /* NETSCAPE */
 
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MainFrame

IMPLEMENT_DYNCREATE(MainFrame, CFrameWnd)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(MainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(MainFrame)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_PALETTECHANGED()
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	//ID_INDICATOR_CAPS,
	//ID_INDICATOR_NUM,
	//ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// MainFrame construction/destruction

MainFrame::MainFrame()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtMainFrame");

	m_bAutoMenuEnable = FALSE;	
	m_minTrackSize.x = -1;
	m_nextCmdID = 1;
	m_pJavaObject = NULL;
	m_pMainWindow = NULL;
	m_parentWnd = NULL;
	m_bInCreate = TRUE;
	m_bShowingStatusBar = FALSE;
	m_hIcon = NULL;
	m_bHasMenuBar = FALSE;
	m_bHasTitleBar = FALSE;
        m_cursor = NULL;
}

MainFrame::~MainFrame()
{
    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
    if (m_cursor != NULL) {
        ::DestroyCursor(m_cursor);
    }

#ifdef NETSCAPE
    //
    // If an embedded MainFrame is being destroyed, reparent it to the 
    // desktop window before destroying it...  
    // 
    // This will prevent any notifications messages that result from the 
    // destruction from being passed up the hierarchy to the browser window.
    // Since the browser window is in a different thread, it may be blocked
    // which would result in a deadlock (since it could not process the 
    // notification).
    //
    if( IsEmbedded() ) {
        SetParent( GetDesktopWindow() );
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////
// MainFrame operations

long MainFrame::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						  long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:				
			return (long)Create((Hjava_awt_Frame*)p1, (BOOL)p2, 
					    (MainFrame*)p3, (long)p4, (long)p5, 
					    (COLORREF)p7);
			break;
		case DISPOSE:
			((MainFrame *)p1)->Dispose();
			break;
		case SETTEXT:
			((MainFrame *)p1)->SetText((char*)p2);
			break;
		case SETICON:
			((MainFrame *)p1)->SetIcon((AwtImage*)p2);
			break;
		case SETMINSIZE:
			((MainFrame*)p1)->SetMinSize((long)p2, (long)p3);
			break;
		case CREATEMENUBAR:
			return (long)((MainFrame *)p1)->CreateMenuBar();
			break;
		case DRAWMENUBAR:
			((MainFrame*)p1)->DrawMenuBar((HMENU)p2);
			break;
		case DISPOSEMENUBAR:
			((MainFrame*)p1)->DisposeMenuBar((HMENU)p2);
			break;
		case SETRESIZABLE:
			((MainFrame*)p1)->SetResizable((BOOL)p2);
			break;
		case SHOWINGSTATUSBAR:
			return (long)((MainFrame*)p1)->ShowingStatusBar();
			break;
		case SHOWSTATUSBAR:
			((MainFrame*)p1)->ShowStatusBar((BOOL)p2);
			break;
		case SETSTATUSMESSAGE:
			((MainFrame*)p1)->SetStatusMessage((char*)p2);
			break;
	        case SETCURSOR:
                	((MainFrame*)p1)->SetCursor(p2);
                        break;
		case RESHAPE:
			((MainFrame*)p1)->Reshape((long)p2, (long)p3, (long)p4, (long)p5);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

MainFrame *MainFrame::Create(Hjava_awt_Frame *pJavaObject, BOOL bHasTitleBar, 
			     MainFrame *pParent, long nWidth, long nHeight,
		 	     COLORREF bgColor)
{
    static CString pzClassName;
    MainFrame *pFrame;
    HCURSOR cursor = GetApp()->LoadStandardCursor(IDC_ARROW);

    if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
        return (MainFrame *)((CAwtApp *)AfxGetApp())->SendAwtMsg(
            CAwtApp::AWT_FRAME, CREATE,
            (long)pJavaObject, (long)bHasTitleBar, (long)pParent, 
            (long)nWidth, (long)nHeight, (long)bgColor);
    }
    if (pzClassName.GetLength() == 0) {
        pzClassName = AfxRegisterWndClass(CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW,
            cursor, NULL);
    }
    pFrame = new MainFrame();
    pFrame->m_pJavaObject = pJavaObject;
    pFrame->m_bgColor = bgColor;
    pFrame->m_bgColor = RGB(192, 192, 192);	//<<temp hack>>
    pFrame->m_cursor = cursor;

    CALLINGWINDOWS(pFrame->GetSafeHwnd());
 
#ifdef NETSCAPE
     ClassClass*             embeddedAppletFrameClass;
     bool_t                  isEmbedded;
 
     pFrame->m_parentWnd = (HWND)0;
     embeddedAppletFrameClass = FindClass(0, "netscape/applet/EmbeddedAppletFrame", (bool_t)0);
     isEmbedded = is_instance_of((JHandle *)pJavaObject, embeddedAppletFrameClass, EE());
     if (isEmbedded) {
         Classnetscape_applet_EmbeddedAppletFrame *pAppletFrame;
         LJAppletData *ad;
 
         pAppletFrame = (Classnetscape_applet_EmbeddedAppletFrame *)unhand(pJavaObject);
         if( pAppletFrame && pAppletFrame->pData) {
             ad = (LJAppletData *)pAppletFrame->pData;
             pFrame->m_parentWnd = (HWND)ad->window;
#if 0
             //
             // If the Awt Palette has bot been initialize yet, then 
             // initialize it...
             //
             if( AwtImage::m_hPal == NULL ) {
                if( ad->fe_data ) {
                    AwtImage::InitializePalette( (HPALETTE)ad->fe_data );
                } else {
                    HDC hDC = ::GetDC( ::GetDesktopWindow() );
                    AwtImage::InitializePalette( hDC );
                    ::DeleteDC(hDC);
                }
             }
#endif
         }
     }
 
     if( pFrame->IsEmbedded() ) {
         VERIFY(pFrame->CFrameWnd::CreateEx(WS_EX_NOPARENTNOTIFY, pzClassName, NULL, 
                WS_CHILD, 0, 0, nWidth, nHeight, pFrame->m_parentWnd, NULL));
     } else {
         VERIFY(pFrame->CFrameWnd::Create(pzClassName, NULL, WS_OVERLAPPEDWINDOW,	
                rectDefault, pParent));

        pFrame->SetFocus();
     }
#else /* ! NETSCAPE */
 
 	VERIFY(pFrame->CFrameWnd::Create(pzClassName, NULL, WS_OVERLAPPEDWINDOW,	
 		rectDefault, pParent));

    pFrame->SetFocus();
#endif /* !NETSCAPE */

    // Test for whether target is NULL was made in awt.cpp
    if (unhand(pJavaObject)->warningString) {
        pFrame->SetStatusMessage(strdup(
            makeCString(unhand(pJavaObject)->warningString)));
        pFrame->ShowStatusBar(TRUE);
    }
	
    pFrame->EnableTitleBar(bHasTitleBar);
    //pFrame->Reshape(TRUE, 0, 0, TRUE, nWidth, nHeight);
    pFrame->m_bInCreate = FALSE;
    return pFrame;
}

// Subtract inset values from a window origin.
void MainFrame::SubtractInsetPoint(long& x, long& y)
{
    x -= left;
    y -= top;
}

// Subtract inset values from a window size -- this actually makes the
// size bigger!
void MainFrame::SubtractInsetSize(long& width, long& height)
{
    width -= left + right;
    height -= top + bottom;
}

void MainFrame::SetInsets(Hjava_awt_Insets *insets)
{
    // Code to calculate insets. Stores results in frame's data
    // members. MainFrame::SetInsets call stores results
    // in the java-accessible awt peer.
    RECT outside;
    RECT inside;

    GetWindowRect(&outside);
    GetClientRect(&inside);
    ClientToScreen(&inside);

    top = inside.top - outside.top;
    bottom = outside.bottom - inside.bottom;
    left = inside.left - outside.left;
    right = outside.right - inside.right;

    if (m_pJavaObject != NULL) {
        if (unhand(m_pJavaObject)->warningString) {
            RECT msgRect;
            CWnd* pMsgWnd = GetMessageBar();
            if (pMsgWnd != NULL) {
                GetMessageBar()->GetWindowRect(&msgRect);
                bottom += msgRect.bottom - msgRect.top;
            }
        }
        Hsun_awt_win32_MFramePeer *frameP = 
            (Hsun_awt_win32_MFramePeer*)unhand(m_pJavaObject)->peer;
        if (frameP != NULL) {
            Hjava_awt_Insets* insets = unhand(frameP)->insets;
            unhand(insets)->top = top;
            unhand(insets)->bottom = bottom;
            unhand(insets)->left = left;
            unhand(insets)->right = right;
        }
    }

    if (insets != NULL) {
        Classjava_awt_Insets *insetsPtr = unhand(insets);
	insetsPtr->top = top;
	insetsPtr->left = left;
	insetsPtr->bottom = bottom;
	insetsPtr->right = right;
    }
}

void MainFrame::Dispose()
{
	delete this;
}

HMENU MainFrame::CreateMenuBar()
{
    if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
        return (HMENU)((CAwtApp *)AfxGetApp())->SendAwtMsg(
            CAwtApp::AWT_FRAME, CREATEMENUBAR);
    }
    return ::CreateMenu();
}

void MainFrame::DrawMenuBar(HMENU hMenu)
{
	CALLINGWINDOWS(m_hWnd);
	if (hMenu != NULL) {
		::SetMenu(m_hWnd, hMenu);
		m_bHasMenuBar = TRUE;
	}
	CFrameWnd::DrawMenuBar();
}

void MainFrame::DisposeMenuBar(HMENU hMenu)
{
    if (hMenu != NULL) {
	::DestroyMenu(hMenu);
	m_bHasMenuBar = FALSE;
        CFrameWnd::DrawMenuBar();
    }
}

void MainFrame::SetText(char *pzText)
{
	CALLINGWINDOWS(m_hWnd);
	if (pzText != NULL) {
		CFrameWnd::SetWindowText(pzText);
		free(pzText);
	} else {
		CFrameWnd::SetWindowText("");
	}
}

void MainFrame::SetIcon(AwtImage *pImage)
{
    if (IsIconic()) {
        InvalidateRect(NULL, TRUE);
    }
    if (m_hIcon != NULL) {
        ::SetClassLong(m_hWnd, GCL_HICON, (long)m_hIcon);
    } else {
        // Load the AWT icon.
        HICON hicon = ((CAwtApp *)AfxGetApp())->GetAwtIcon();
        if (hicon != NULL) {
            ::SetClassLong(m_hWnd, GCL_HICON, (long)hicon);
        }
    }
}

void MainFrame::SetCursor(long cursorType)
{
    LPCTSTR cursor;
    switch (cursorType) {
      case java_awt_Frame_DEFAULT_CURSOR:
      default:
          cursor = IDC_ARROW;
          break;
      case java_awt_Frame_CROSSHAIR_CURSOR:
          cursor = IDC_CROSS;
          break;
      case java_awt_Frame_TEXT_CURSOR:
          cursor = IDC_IBEAM;
          break;
      case java_awt_Frame_WAIT_CURSOR:
          cursor = IDC_WAIT;
          break;
      case java_awt_Frame_NE_RESIZE_CURSOR:
      case java_awt_Frame_SW_RESIZE_CURSOR:
          cursor = IDC_SIZENESW;
          break;
      case java_awt_Frame_SE_RESIZE_CURSOR:
      case java_awt_Frame_NW_RESIZE_CURSOR:
          cursor = IDC_SIZENWSE;
          break;
      case java_awt_Frame_N_RESIZE_CURSOR:
      case java_awt_Frame_S_RESIZE_CURSOR:
          cursor = IDC_SIZENS;
          break;
      case java_awt_Frame_W_RESIZE_CURSOR:
      case java_awt_Frame_E_RESIZE_CURSOR:
          cursor = IDC_SIZEWE;
          break;
      case java_awt_Frame_HAND_CURSOR:
          cursor = IDC_ICON;
          break;
      case java_awt_Frame_MOVE_CURSOR:
          cursor = IDC_SIZE;
          break;
    }
    ::DestroyCursor(m_cursor);
    m_cursor = GetApp()->LoadStandardCursor(cursor);
    if (m_cursor == NULL) {
        m_cursor = GetApp()->LoadStandardCursor(IDC_ARROW);
        ASSERT(m_cursor != NULL);
    }
}

void MainFrame::SetResizable(BOOL bResizable)
{
    if ( ! IsEmbedded() ) {
	CALLINGWINDOWS(m_hWnd);
	if (bResizable) {
		::SetWindowLong(m_hWnd, GWL_STYLE, GetStyle() | WS_THICKFRAME);
	} else {
	        ::SetWindowLong(m_hWnd, GWL_STYLE, GetStyle() & ~WS_THICKFRAME);
	}
    }
}

void MainFrame::EnableTitleBar(BOOL bEnable)
{
    if ( ! IsEmbedded() ) {
	CALLINGWINDOWS(m_hWnd);
	if (bEnable) {
		::SetWindowLong(m_hWnd, GWL_STYLE, GetStyle() | WS_CAPTION);
	} else {
		::SetWindowLong(m_hWnd, GWL_STYLE, GetStyle() & ~WS_CAPTION);
	}
	m_bHasTitleBar = bEnable;
    }
}

void MainFrame::SetMinSize(int width, int height)
{
	CRect rect;

	GetWindowRect(rect);
	m_minTrackSize.x = width;
	m_minTrackSize.y = height;
}

void MainFrame::Reshape(long nX, long nY, long nWidth, long nHeight)
{
	CRect rect;

	if ( !IsEmbedded() ) {
	    rect = CRect(nX, nY, nX+nWidth, nY+nHeight);
	} else {
	    rect = CRect(0, 0, nWidth, nHeight);
	}
	VERIFY(SetWindowPos(NULL, rect.left, rect.top, 
		rect.Width(), rect.Height(), SWP_NOACTIVATE|SWP_NOZORDER));
}

UINT MainFrame::CreateCmdID(void *pValue)
{
	void *junk;
	UINT result = m_nextCmdID;

	if (m_cmdMap.GetCount() == 0xFFFF) {
		MessageBox("Fatal Error: there are more than 0xFFFF commands defined");
		AfxAbort();
	}
	while (m_cmdMap.Lookup(result, junk)) {
		if (++result == 0xFFFF) result = 0;
	}
	m_nextCmdID = result + 1;
	if (m_nextCmdID == 0xFFFF) m_nextCmdID = 0;
	m_cmdMap[result] = pValue;
	return result;
}

void *MainFrame::GetCmdValue(UINT nCmdID)
{
	void *result;

    if (!m_cmdMap.Lookup(nCmdID, result))
		result = NULL;
	return result;
}

void *MainFrame::DeleteCmdValue(UINT nCmdID)
{
	void *result;
	
	VERIFY(m_cmdMap.Lookup(nCmdID, result));
	VERIFY(m_cmdMap.RemoveKey(nCmdID));
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// StatusBar members

void MainFrame::ShowStatusBar(BOOL bShow)
{
	if (! IsEmbedded() && bShow != m_bShowingStatusBar) {
		m_bShowingStatusBar = bShow;
		m_wndStatusBar.ShowWindow(bShow ? SW_SHOW : SW_HIDE);
		RecalcLayout();
                RedrawWindow();
	}
}

BOOL MainFrame::ShowingStatusBar()
{
	return m_bShowingStatusBar;
}
	
void MainFrame::SetStatusMessage(char *pzMessage) 
{
        if ( ! IsEmbedded() ) {
	    m_wndStatusBar.SetPaneText(0, pzMessage, TRUE);
	}
	free(pzMessage);
}

/////////////////////////////////////////////////////////////////////////////
// MainFrame overrides

int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    CCreateContext *pContext = new CCreateContext;

    // set it up so that the new is automatically created
    lpCreateStruct->lpCreateParams = pContext;
    pContext->m_pNewViewClass = RUNTIME_CLASS(AwtWindow);

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    delete pContext;

    if (!IsEmbedded()) {
	if (!m_wndStatusBar.Create(this, 
	   WS_CHILD|CBRS_BOTTOM|CBRS_BORDER_ANY|CBRS_BORDER_3D) ||
	    !m_wndStatusBar.SetIndicators(indicators,
					  sizeof(indicators)/sizeof(UINT)))
	    {
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	    }
    }

    // <<can't cast AwtWindow to CView; try RUNTIME_CLASS(CView)
    // and keeping main pane elsewhere>>
    m_pMainWindow = (CView*)GetDlgItem(AFX_IDW_PANE_FIRST);
    ::SetWindowLong(m_pMainWindow->GetSafeHwnd(), GWL_STYLE, m_pMainWindow->GetStyle() | WS_CLIPCHILDREN);
    ((AwtWindow *)m_pMainWindow)->SetMargin(0);
    ((AwtWindow *)m_pMainWindow)->SetColor(FALSE, m_bgColor);
    ((AwtWindow *)m_pMainWindow)->m_pJavaObject = (Hjava_awt_Canvas*)m_pJavaObject;
    ((AwtWindow *)m_pMainWindow)->m_pFrame = this;

    return 0;
}

BOOL MainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT nID = wParam;
	CObject *pObject = (CObject *)GetCmdValue(nID);

	if (pObject != NULL) {
		if (pObject->IsKindOf(RUNTIME_CLASS(AwtMenu))) {
			((AwtMenu *)pObject)->InvokeItem(nID);
			return TRUE;
		}
	}
	return CFrameWnd::OnCommand(wParam, lParam);
}

void MainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	if (m_minTrackSize.x < 0)
		m_minTrackSize = lpMMI->ptMinTrackSize;
	else
		lpMMI->ptMinTrackSize = m_minTrackSize;
}

/////////////////////////////////////////////////////////////////////////////
// MainFrame diagnostics

#ifdef _DEBUG
void MainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void MainFrame::Dump(CDumpContext& dc) const
{
	//CFrameWnd::Dump(dc);
	dc << "AwtFrame.m_hWnd = " << m_hWnd << "\n";
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// MainFrame message handlers

BOOL MainFrame::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}


void MainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CRect rect;
	Classjava_awt_Frame *pClass;
	
	CFrameWnd::OnSize(nType, cx, cy);

 	if (m_bInCreate || m_pMainWindow == NULL) 
            return;
 	pClass = unhand(m_pJavaObject);
        GetWindowRect(&rect);
        SetInsets(NULL);
 	pClass->width = rect.Width();
 	pClass->height = rect.Height();
 	pClass->x = rect.left;
 	pClass->y = rect.top;

 	AWT_TRACE(("frame.resize\n"));
 	GetApp()->DoCallback(m_pJavaObject, "handleResize", "(II)V", 
                             2, pClass->width, pClass->height);
}

void MainFrame::OnClose() 
{
	GetApp()->DoCallback(m_pJavaObject, "handleQuit", "()V");
}

void MainFrame::OnPaint() 
{
    CPaintDC dc(this);
    if (IsEmbedded() || m_wndStatusBar.IsWindowVisible() == FALSE) {
        return;
    }

    // Paint status bar a warning color, per Netscape's "request".
    CRect rect;
    m_wndStatusBar.GetItemRect(0, &rect);
    rect.left = 0;
    CPaintDC dcStatus(&m_wndStatusBar);
    HBRUSH hBrush = ::CreateSolidBrush(RGB(255, 225, 0));
    if (hBrush != NULL) {
        ::FillRect(dcStatus.m_hDC, &rect, hBrush);
        ::DeleteObject(hBrush);
    }

    // Draw text.
    m_wndStatusBar.GetItemRect(0, &rect);
    int nOldMode = dcStatus.SetBkMode(TRANSPARENT);
    COLORREF crTextColor = dcStatus.SetTextColor(RGB(0, 0, 0));
    dcStatus.SetTextAlign(TA_LEFT | TA_BOTTOM);
    CString text;
    m_wndStatusBar.GetPaneText(0, text);
    HGDIOBJ hOldFont = dcStatus.SelectObject(GetApp()->m_hBtnFont);
    dcStatus.ExtTextOut(rect.left, rect.bottom, ETO_CLIPPED, 
                        &rect, text, text.GetLength(), NULL);
    dcStatus.SelectObject(hOldFont);
}

BOOL MainFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
    cs.lpszClass = AfxRegisterWndClass ( 
        CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
        AfxGetApp()->LoadStandardCursor(IDC_ARROW),
        (HBRUSH) (COLOR_WINDOW + 1), 
        ((CAwtApp *)AfxGetApp())->GetAwtIcon());
    return TRUE;
}

void MainFrame::OnPaletteChanged(CWnd* pFocusWnd) 
{
    CDC *pDC;

    CFrameWnd::OnPaletteChanged(pFocusWnd);
    
    // Don't do this if we caused it to happen.
    if(pFocusWnd == (CWnd *)this) {
        return;
    }

    pDC = GetDC();
    VERIFY(::SelectPalette(pDC->GetSafeHdc(), AwtImage::m_hPal, pFocusWnd->IsChild(this)));

    m_pMainWindow->Invalidate(TRUE);
    m_pMainWindow->UpdateWindow();

    VERIFY(ReleaseDC(pDC));
}

// Suppress MFC's setting of the status bar.
LRESULT MainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
    // The following is copied verbatim from mfc/src/winfrm.cpp.
    UINT nIDLast = m_nIDLastMessage;
    m_nIDLastMessage = (UINT)wParam;
    m_nIDTracking = (UINT)wParam;
    return nIDLast;
}

LRESULT MainFrame::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // Never pass WM_SETCURSOR messages to the parent window
    // Let the child window handle it...
    if( (message == WM_SETCURSOR) && IsEmbedded() ) {
        return FALSE;
    }
    return CFrameWnd::DefWindowProc(message, wParam, lParam);
}
