/*
 * @(#)awt_button.cpp	1.18 95/11/29 Patrick P. Chan
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
#include "awtapp.h"
#include "awt_button.h"


IMPLEMENT_DYNCREATE(AwtButton, CButton)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(AwtButton, CButton)
	//{{AFX_MSG_MAP(AwtButton)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

AwtButton::AwtButton()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtButton");

	m_bEnabled = TRUE;
	m_bgfgColor[FALSE] = RGB(255, 255, 255);
	m_bgfgColor[TRUE] = RGB(0, 0, 0);
	GetApp()->CreateBrush(m_bgfgColor[FALSE]);
	GetApp()->CreateBrush(m_bgfgColor[TRUE]);
}

AwtButton::~AwtButton()
{
	GetApp()->ReleaseBrush(m_bgfgColor[FALSE]);
	GetApp()->ReleaseBrush(m_bgfgColor[TRUE]);

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}


long AwtButton::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						  long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((void *)p1, (UINT)p2, (AwtWindow*)p3, (char*)p4);
			break;
		case DISPOSE:
			((AwtButton *)p1)->Dispose();
			break;
		case GETSTATE:
			return (long)((AwtButton*)p1)->GetState();
			break;
		case SETSTATE:
			((AwtButton*)p1)->SetState((BOOL)p2);
			break;
		case SETTEXT:
			((AwtButton*)p1)->SetText((char*)p2);
			break;
		case SETCOLOR:
			((AwtButton*)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtButton *AwtButton::CreateButton(Hjava_awt_Button *pJavaObject, AwtWindow *pParent, char *pzLabel)
{
	return Create((void*)pJavaObject, BS_PUSHBUTTON, pParent, pzLabel);	
}

AwtButton *AwtButton::CreateToggle(Hjava_awt_Checkbox *pJavaObject, AwtWindow *pParent, char *pzLabel)
{
	return Create((void*)pJavaObject, BS_CHECKBOX, pParent, pzLabel);	
}

AwtButton *AwtButton::CreateRadio(Hjava_awt_Checkbox *pJavaObject, AwtWindow *pParent, char *pzLabel)
{
	return Create((void*)pJavaObject, BS_RADIOBUTTON, pParent, pzLabel);	
}

AwtButton *AwtButton::Create(void *pJavaObject, UINT nButtonType, AwtWindow *pParent, char *pzLabel) 
{
	AwtButton *pButton;

	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtButton *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_BUTTON, CREATE,
			(long)pJavaObject, (long)nButtonType, (long)pParent, (long)pzLabel);
	}
	pButton = new AwtButton();
	pButton->Init(pJavaObject, nButtonType, pParent, pzLabel);
	return pButton;
}

// This is a temporary workaround; motif widgets have a 2 pixel blank border around their buttons.
#define TEMP_BORDER 2

void AwtButton::Init(void *pJavaObject, UINT nButtonType, AwtWindow *pParent, char *pzLabel)
{
	CRect rect;
	DWORD dwDLU = ::GetDialogBaseUnits();
	WORD nXdlu = LOWORD(dwDLU);
	WORD nYdlu = HIWORD(dwDLU);

	m_pParent = pParent;
	m_bPushButton = nButtonType == BS_PUSHBUTTON || nButtonType == BS_OWNERDRAW;
	if (m_bPushButton) {
		m_pButtonObject = (Hjava_awt_Button *)pJavaObject;
	} else {
		m_pToggleObject = (Hjava_awt_Checkbox *)pJavaObject;
	}
	rect = GetApp()->ComputeRect(pzLabel, m_bPushButton ? 0 : 1);
	rect.right += 2 * TEMP_BORDER;	// <<workaround>>
	rect.bottom += 2 * TEMP_BORDER;	// <<workaround>>

	CALLINGWINDOWS(m_hWnd);
	CButton::Create(pzLabel, WS_CHILD|WS_CLIPSIBLINGS
							 |nButtonType,
		rect, pParent, pParent->GetCmdID());
	CALLINGWINDOWS(m_hWnd);
	SendMessage(WM_SETFONT, (WPARAM)GetApp()->m_hBtnFont);
}

void AwtButton::Dispose()
{
	delete this;
}

void AwtButton::SetColor(BOOL bFg, COLORREF color)
{
	GetApp()->ReleaseBrush(m_bgfgColor[bFg]);
	m_bgfgColor[bFg] = color;
	GetApp()->CreateBrush(color);
}

void AwtButton::SetState(BOOL bOn)
{
	CALLINGWINDOWS(m_hWnd);
	SetCheck(bOn);
}

BOOL AwtButton::GetState()
{
	CALLINGWINDOWS(m_hWnd);
	return GetCheck();
}

void AwtButton::SetText(char *pzLabel)
{
	BOOL bNull = FALSE;
	
	if (pzLabel == NULL) {
		pzLabel = "";
		bNull = TRUE;
	}
	SetWindowText(pzLabel);
	if (bNull) {
		free(pzLabel);
	}
}

/////////////////////////////////////////////////////////////////////////////
// AwtButton message handlers

void AwtButton::OnSetFocus(CWnd* pOldWnd) 
{
	CALLINGWINDOWS(m_hWnd);
	CButton::OnSetFocus(pOldWnd);
	
	/*if (m_bPushButton) {
		CALLINGWINDOWS(m_hWnd);
		SendMessage(WM_SETFONT, (WPARAM)GetApp()->m_hBtnFontBold);
	}*/
}

void AwtButton::OnKillFocus(CWnd* pNewWnd) 
{
	CALLINGWINDOWS(m_hWnd);
	CButton::OnKillFocus(pNewWnd);
	
	/*if (m_bPushButton) {
		CALLINGWINDOWS(m_hWnd);
		SendMessage(WM_SETFONT, (WPARAM)GetApp()->m_hBtnFont);
	}*/
}

BOOL AwtButton::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	UINT nCode = GetApp()->m_bWin95OS ? HIWORD(lParam) : HIWORD(wParam);	// need to verify this.

	if (message == WM_COMMAND && nCode == BN_CLICKED && m_bEnabled) {
		if (m_bPushButton) {
			AWT_TRACE(("button.action\n"));
			GetApp()->DoCallback(m_pButtonObject, "action", "()V");
		} else {
			BOOL bState = !GetState();
			SetState(bState);
			AWT_TRACE(("button.action(%d)\n", bState));
			GetApp()->DoCallback(m_pToggleObject, "action", "(Z)V", 1, bState);
		}
		return TRUE;
	}
	
	return CButton::OnChildNotify(message, wParam, lParam, pLResult);
}

/////////////////////////////////////////////////////////////////////////////
// AwtButton diagnostics

#ifdef _DEBUG
void AwtButton::AssertValid() const
{
	CWnd::AssertValid();
}

void AwtButton::Dump(CDumpContext& dc) const
{
	//CWnd::Dump(dc);
	dc << "AwtButton.m_hWnd = " << m_hWnd << "\n";
}

#endif //_DEBUG
