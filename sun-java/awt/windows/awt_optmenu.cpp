/*
 * @(#)awt_optmenu.cpp	1.20 95/11/29 Patrick P. Chan
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
// optmenu.cpp : implementation file
//

#include "stdafx.h"
#include "awt.h"
#include "awt_fontmetrics.h"
#include "awt_window.h"
#include "awt_optmenu.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AwtOptionMenu

IMPLEMENT_DYNCREATE(AwtOptionMenu, CComboBox)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

AwtOptionMenu::AwtOptionMenu()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtOptionMenu");

	m_nMaxWidth = 0;
 	m_bgfgColor[FALSE] = RGB(255, 255, 255);
 	m_bgfgColor[TRUE] = RGB(0, 0, 0);
 	GetApp()->CreateBrush(m_bgfgColor[FALSE]);
 	GetApp()->CreateBrush(m_bgfgColor[TRUE]);
        m_nItems = 0;
}

AwtOptionMenu::~AwtOptionMenu()
{
 	GetApp()->ReleaseBrush(m_bgfgColor[FALSE]);
 	GetApp()->ReleaseBrush(m_bgfgColor[TRUE]);

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtOptionMenu::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
							 long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((Hjava_awt_Choice *)p1, (AwtWindow*)p2);
			break;
		case DISPOSE:
			((AwtOptionMenu*)p1)->Dispose();
			break;
		case ADDITEM:			
			((AwtOptionMenu *)p1)->AddItem((char *)p2, (long)p3);
			break;
		case SELECT:			
			((AwtOptionMenu *)p1)->Select((long)p2);
			break;
		case REMOVE:
			((AwtOptionMenu *)p1)->Remove((long)p2);
			break;
		case SETCOLOR:
			((AwtOptionMenu*)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		case RESHAPE:
			((AwtOptionMenu*)p1)->Reshape((BOOL)p2, (long)p3, (long)p4, 
										  (BOOL)p5, (long)p6, (long)p7);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

// This is a temporary workaround; motif widgets have a 2 pixel blank border around their buttons.
#define TEMP_BORDER 2

AwtOptionMenu *AwtOptionMenu::Create(Hjava_awt_Choice *pJavaObject, AwtWindow *pParent)
{
	AwtOptionMenu *pOptionMenu;
	CRect rect;
	
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtOptionMenu *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_OPTIONMENU, CREATE,
			(long)pJavaObject, (long)pParent);
	}
	rect = GetApp()->ComputeRect(NULL, 25);
	pOptionMenu = new AwtOptionMenu();
	pOptionMenu->m_pJavaObject = pJavaObject;
	rect.bottom += 150;		// size for drop-down list box
	pOptionMenu->CComboBox::Create(CBS_DROPDOWNLIST|WS_CHILD, rect, pParent, 
		pParent->GetCmdID());
	rect.bottom -= 150;		// size of the combobox control
	rect.right += 2 * TEMP_BORDER;	// <<workaround>>
	rect.bottom += 4 * TEMP_BORDER;	// <<workaround>>
	pOptionMenu->Reshape(TRUE, rect.left, rect.top, TRUE, rect.Width(), rect.Height());
	return pOptionMenu;
}

void AwtOptionMenu::Dispose() {
	delete this;
}

void AwtOptionMenu::SetColor(BOOL bFg, COLORREF color) {
	GetApp()->ReleaseBrush(m_bgfgColor[bFg]);
	m_bgfgColor[bFg] = color;
	GetApp()->CreateBrush(color);
}

void AwtOptionMenu::AddItem(char *pzItem, long nPos)
{
	int err;
	
	err = InsertString(nPos, pzItem);
	ASSERT(err != CB_ERR && err != CB_ERRSPACE);
	free(pzItem);
        m_nItems++;

	// If no selection, select the first item.
	if (GetCurSel() == CB_ERR) {
		SetCurSel(0);
	}
	
	// resize the option menu if necessary
	CFont *pFont = GetFont();
	CSize size = AwtFontMetrics::StringWidth(pFont, pzItem);
	if (size.cx > m_nMaxWidth) {
		CRect rect;

		GetClientRect(&rect);
		rect.left = 0;
		// I thought AdjustWindowRectEx adjusted for width of vertical scroll bar 
		// but it doesn't seem to.  Need to figure out the right thing to do.
		rect.right = size.cx + 30;	
		rect.top = 0;
		VERIFY(::AdjustWindowRectEx(&rect, WS_VSCROLL|WS_BORDER, FALSE, GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0));
		Reshape(FALSE, 0, 0, TRUE, rect.Width(), rect.Height());
		m_nMaxWidth = size.cx;
	}
}

void AwtOptionMenu::Select(long nPos)
{
    VERIFY(SetCurSel((UINT)nPos) != CB_ERR);
}

void AwtOptionMenu::Remove(long nPos)
{
    VERIFY(DeleteString((UINT)nPos) != CB_ERR);
    m_nItems--;
}

int AwtOptionMenu::Count()
{
    return m_nItems;
}

void AwtOptionMenu::Reshape(BOOL bMove, long nX, long nY, 
				 BOOL bResize, long nWidth, long nHeight)
{
	Classjava_awt_Choice *pClass = unhand(m_pJavaObject);

	if (bMove) {
		pClass->x = nX;
		pClass->y = nY;
	}
	if (bResize) {
		pClass->width = nWidth;
		pClass->height = nHeight;
	}
	//bResize = FALSE;	// <<workaround>>
	VERIFY(SetWindowPos(NULL, nX+TEMP_BORDER, nY+TEMP_BORDER, nWidth - 2*TEMP_BORDER, nHeight-2*TEMP_BORDER,
		SWP_NOACTIVATE|SWP_NOZORDER|(bMove ? 0 : SWP_NOMOVE)
		|(bResize ? 0 : SWP_NOSIZE)));
}

BEGIN_MESSAGE_MAP(AwtOptionMenu, CComboBox)
	//{{AFX_MSG_MAP(AwtOptionMenu)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// AwtOptionMenu message handlers

BOOL AwtOptionMenu::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	if (message == WM_COMMAND && HIWORD(wParam) == CBN_SELCHANGE) {
		int nSel = GetCurSel();
		Classjava_awt_Choice *pClass = unhand(m_pJavaObject);

		if (nSel != CB_ERR) {
			pClass->selectedIndex = nSel;
			AWT_TRACE(("[%x]:optionMenu.action(%d)\n", m_hWnd, nSel));
			GetApp()->DoCallback(m_pJavaObject, "action", "(I)V", 1, nSel);
		}
		return TRUE;
	}
	return CComboBox::OnChildNotify(message, wParam, lParam, pLResult);
}

HBRUSH AwtOptionMenu::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	//HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_LISTBOX) {
		CFont *pFont = GetFont();
		CSize size = AwtFontMetrics::StringWidth(pFont, "M");
		CRect rect;

		pWnd->GetClientRect(rect);
		//printf("y(%d) (%d %d %d %d)\n", size.cy, rect.left, rect.top, rect.right, rect.bottom);
		if (rect.Height() <= size.cy) {
			// Add size.cy/2 to account for borders
			rect.bottom = rect.top + min(200, GetCount() * size.cy + size.cy/2);
			VERIFY(pWnd->SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(),
				SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE));
		}
	}
	
	VERIFY(::SetBkColor(pDC->m_hDC, m_bgfgColor[FALSE]) != CLR_INVALID);
	VERIFY(::SetTextColor(pDC->m_hDC, m_bgfgColor[TRUE]) != CLR_INVALID);
	return GetApp()->GetBrush(m_bgfgColor[FALSE]);
}

/////////////////////////////////////////////////////////////////////////////
// AwtButton diagnostics

#ifdef _DEBUG
void AwtOptionMenu::AssertValid() const
{
	CComboBox::AssertValid();
}

void AwtOptionMenu::Dump(CDumpContext& dc) const
{
	//CWnd::Dump(dc);
	dc << "AwtOptionMenu.m_hWnd = " << m_hWnd << "\n";
}

#endif //_DEBUG

