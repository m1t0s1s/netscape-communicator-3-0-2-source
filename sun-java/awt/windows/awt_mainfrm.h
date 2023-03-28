/*
 * @(#)awt_mainfrm.h	1.22 95/12/04
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
// mainfrm.h : interface of the MainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _MAINFRAME_H_
#define _MAINFRAME_H_

#include "awt.h"
#include "awtapp.h"
#include "awt_image.h"

#define WM_SETMESSAGESTRING 0x0362  // from mfc/include/afxpriv.h

class MainFrame : public CFrameWnd
{
public: 
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, DRAWMENUBAR, SETRESIZABLE, SETTEXT, SETICON, SETMINSIZE, 
	  	  SHOWSTATUSBAR, SHOWINGSTATUSBAR, SETSTATUSMESSAGE, RESHAPE,
	  	  CREATEMENUBAR, DISPOSEMENUBAR, SETCURSOR};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	MainFrame();
	virtual ~MainFrame();
	DECLARE_DYNCREATE(MainFrame)

// Operations
public:
	// Creates the Windows Frame object.  If pParent is not NULL, the new Frame
	// is a modal frame and pParent is the owner.
	static MainFrame *Create(Hjava_awt_Frame *pJavaObject, BOOL bHasTitleBar, 
				 MainFrame *pParent, long nWidth, long nHeight,
				 COLORREF bgColor);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	// Returns the main window.
	CView *GetMainWindow() { return m_pMainWindow; };

	void EnableTitleBar(BOOL bEnable);

	void SetResizable(BOOL bResizable);

	// Sets the text in the frame's title bar.	'pzText' will be freed by this member.
	// If pzText == NULL, the title is cleared.
	void SetText(char *pzText);
	
	// Must be called whenever a change is made to the menubar.  If 'hMenu' != NULL,
	// call SetMenu.
	void DrawMenuBar(HMENU hMenu = NULL);

	// Sets the frame's icon.
	void SetIcon(AwtImage *pImage);

	// Returns next available command id.  Also associates nCmdID with pValue.
	UINT CreateCmdID(void *pValue);

	// Returns the pointer value associated with nCmdID.
	void *GetCmdValue(UINT nCmdID);

	// Removes the nCmdID from the command-id map.  Also returns the pValue associated
	// with nCmdID

	void *DeleteCmdValue(UINT nCmdID);

	// Sets the min size of the frame.
	void SetMinSize(int width, int height);

	// Menubar methods.
	HMENU CreateMenuBar();
	void DisposeMenuBar(HMENU hMenu);

	// Shows or hides the status bar.
	void ShowStatusBar(BOOL bShow);
	
	// Returns whether or not the StatusBar is visible.
	BOOL ShowingStatusBar();
	
	// Displays 'pzMessage' in the status bar.
	// This member assumes responsibility for freeing 'pzMessage'.
	void SetStatusMessage(char *pzMessage);

	void SetInsets(Hjava_awt_Insets *insets);
	void SubtractInsetPoint(long& x, long& y);
	void SubtractInsetSize(long& width, long& height);
	void Reshape(long nX, long nY, long nWidth, long nHeight);

	void SetCursor(long cursorType);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_FRAME, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_FRAME, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);
     // Returns whether the frame is embedded or a top level window
     BOOL IsEmbedded() {
         return (m_parentWnd != (HWND)NULL);
     }
 
// Overrides
protected:
	// Handles selections from menu items.
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MainFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

    virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

// Variables
protected:
	// True if frame has a titlebar.
	BOOL m_bHasTitleBar;
	
	// True if frame has a menubar.
	BOOL m_bHasMenuBar;
	
	// Background color
	COLORREF m_bgColor;

	// When client asks for cmd ID, give it the value in this variable.
	UINT m_nextCmdID;

	// Table of commands to pointers.
	CMapWordToPtr m_cmdMap;

	// Used to hold the min size of the frame.  Initially, x is set
	// to -1 so that it gets initialized by the system.
	POINT m_minTrackSize;

	// Pointer to the associated Frame Oak object.  This variable should be
	// set to non-NULL only after the Windows window has been created.  
	Hjava_awt_Frame *m_pJavaObject;

	// Pointer to main window.
	CView *m_pMainWindow;

// Implementation
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:  // control bar embedded members
	CStatusBar  m_wndStatusBar;

	// The current cursor.
	HCURSOR m_cursor;

protected: 
// Generated message map functions
	//{{AFX_MSG(MainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
        afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	// TRUE while in Create method.  This is to prevent any calls
	// to the Oak object callback methods while in the Create method.
	BOOL m_bInCreate;

	// TRUE iff status bar is being displayed.
	BOOL m_bShowingStatusBar;

	// The frame's icon.  If NULL, just paint the HotJava logo.
	HICON m_hIcon;

	// The frame's parent (if any)
	HWND    m_parentWnd;

	// a cache of the insets being used
	int top, left, bottom, right;
};

/////////////////////////////////////////////////////////////////////////////

#endif _MAINFRAME_H_
