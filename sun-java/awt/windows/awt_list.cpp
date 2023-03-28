/*
 * @(#)awt_list.cpp	1.23 95/11/29 Thomas Ball
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
// list.cpp : implementation file
//

#include "stdafx.h"
#include "awtapp.h"
#include "awt.h"
#include "awt_fontmetrics.h"
#include "awt_window.h"
#include "awt_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(AwtList, CListBox)

/////////////////////////////////////////////////////////////////////////////
// AwtList

AwtList::AwtList()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtList");

	m_nMaxWidth = 0;
	m_bgfgColor[FALSE] = RGB(255, 255, 255);
	m_bgfgColor[TRUE] = RGB(0, 0, 0);
	GetApp()->CreateBrush(m_bgfgColor[FALSE]);
	GetApp()->CreateBrush(m_bgfgColor[TRUE]);
}

AwtList::~AwtList()
{
	GetApp()->ReleaseBrush(m_bgfgColor[FALSE]);
	GetApp()->ReleaseBrush(m_bgfgColor[TRUE]);

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtList::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((Hjava_awt_List *)p1, 
				(AwtWindow*)p2);
			break;
		case DISPOSE:
			((AwtList *)p1)->Dispose();
			break;
		case SETCOLOR:
			((AwtList*)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		case SETMULTIPLESELECTIONS:
			((AwtList *)p1)->SetMultipleSelections((BOOL)p2);
			break;
		case ADDITEM:			
			((AwtList *)p1)->AddItem((char *)p2, (long)p3);
			break;
		case DELETEITEMS:			
			((AwtList *)p1)->DeleteItems((long)p2, (long)p3);
			break;
		case ISSELECTED:
			return ((AwtList*)p1)->IsSelected((long)p2);
			break;
		case SELECT:
			((AwtList *)p1)->Select((long)p2, (BOOL)p3);
			break;
		case MAKEVISIBLE:
			((AwtList *)p1)->MakeVisible((long)p2);
			break;
		case RESHAPE:
			((AwtList*)p1)->Reshape((BOOL)p2, (long)p3, (long)p4, 
									(BOOL)p5, (long)p6, (long)p7);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtList *AwtList::Create(Hjava_awt_List *pJavaObject, AwtWindow *pParent)
{
	AwtList *pList;
	CRect rect(0, 0, 100, 100);
	DWORD dwStyle;
	
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtList *)
                ((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_LIST, CREATE,
                                                     (long)pJavaObject, 
                                                     (long)pParent);
	}
	pList = new AwtList();
	pList->m_pJavaObject = pJavaObject;
	pList->m_bMulti = FALSE;
	dwStyle = WS_CHILD | WS_VSCROLL | LBS_NOTIFY | WS_BORDER;
	
	int nCmdID = pParent->GetCmdID();
	VERIFY(pList->CreateEx(GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0, _T("LISTBOX"), NULL, dwStyle, 
		rect.left, rect.top, rect.Width(), rect.Height(), pParent->GetSafeHwnd(), 
		(HMENU)nCmdID, 0));
	rect.bottom = 0;
	VERIFY(::AdjustWindowRectEx(&rect, WS_VSCROLL|WS_BORDER, FALSE, GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0));
	pList->Reshape(TRUE, rect.left, rect.top, TRUE, rect.Width(), rect.Height());
	return pList;
}

void AwtList::Dispose() {
	delete this;
}

void AwtList::SetColor(BOOL bFg, COLORREF color)
{
	GetApp()->ReleaseBrush(m_bgfgColor[bFg]);
	m_bgfgColor[bFg] = color;
	GetApp()->CreateBrush(color);
}

void AwtList::SetMultipleSelections(BOOL bMulti) {
    if (bMulti == m_bMulti) {
        return;
    }

    // Re-create the list box
    CStringArray strings;
    CString text;
    int nCount, i, nCurSel;
    DWORD dwStyle = GetStyle();
    if (bMulti) {
        dwStyle |= LBS_MULTIPLESEL;
    } else {    
        dwStyle &= ~LBS_MULTIPLESEL;
    } 
    m_bMulti = bMulti;

    CRect rect;
    AwtWindow* pParent = (AwtWindow*) GetParent();
    nCount = GetCount();
    strings.SetSize(nCount);
    for(i = 0; i < nCount; i++) {
        GetText(i, text);
        strings[i] = text;
    }
    nCurSel = GetCurSel();

    CFont* curFont = GetFont();
    GetWindowRect(&rect);
    pParent->ScreenToClient(&rect);
    rect.bottom += 6;			// add border thickness.
    DestroyWindow();
    int nCmdID = pParent->GetCmdID();
    VERIFY(CreateEx(GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0, 
                    _T("LISTBOX"), NULL, dwStyle, 
                    rect.left, rect.top, rect.Width(), rect.Height(), 
                    pParent->GetSafeHwnd(), (HMENU)nCmdID, 0));
    SetFont(curFont);

    ResetContent();  
    for(i = 0; i < nCount; i++) {
        AddString(strings[i]);
    }
    SetCurSel(nCurSel);
}

void AwtList::AddItem(char *pzItem, long index)
{
	int err = InsertString(index, pzItem);

	ASSERT(err != LB_ERR && err != LB_ERRSPACE);
	free(pzItem);
}

void AwtList::MakeVisible(int nPos)
{
	VERIFY(SetTopIndex(nPos) != LB_ERR);
}

BOOL AwtList::IsSelected(int nPos)
{
	return GetSel(nPos) ? TRUE : FALSE;
}

void AwtList::Select(int nPos, BOOL bSelect)
{
	if (m_bMulti) {
		SetSel(nPos, bSelect);
	} else {
		if (bSelect) {
			SetCurSel(nPos);
		} else {
			SetCurSel(-1);
		}
	}
}

void AwtList::DeleteItems(int nStart, int nEnd)
{
	int i;
	
	if (nEnd - nStart + 1 == GetCount()) {
		ResetContent();
	} else {
		for (i=nStart; i<=nEnd; i++) {
			DeleteString(i);
		}
	}
}

void AwtList::Reshape(BOOL bMove, long nX, long nY, BOOL bResize, long nWidth, long nHeight)
{
	Classjava_awt_List *pClass = unhand(m_pJavaObject);

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

BEGIN_MESSAGE_MAP(AwtList, CListBox)
	//{{AFX_MSG_MAP(AwtList)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// AwtList message handlers

BOOL AwtList::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	if (message == WM_COMMAND && HIWORD(wParam) == LBN_DBLCLK) {
		int nSel = GetCurSel();

		AWT_TRACE(("%x:list.action(%d)\n", m_pJavaObject, nSel));
		GetApp()->DoCallback(m_pJavaObject, "action", "(I)V", 1, nSel == LB_ERR ? -1 : nSel);
		return TRUE;
	} else if (message == WM_COMMAND && HIWORD(wParam) == LBN_SELCHANGE) {
		int nSel = GetCurSel();

		AWT_TRACE(("%x:list.handleListChanged(%d)\n", m_pJavaObject, nSel));
		GetApp()->DoCallback(m_pJavaObject, "handleListChanged", "(I)V",1, nSel == LB_ERR ? -1 : nSel);
		return TRUE;
	}

	return CListBox::OnChildNotify(message, wParam, lParam, pLResult);
}

/////////////////////////////////////////////////////////////////////////////
// AwtButton diagnostics

#ifdef _DEBUG
void AwtList::AssertValid() const
{
	CListBox::AssertValid();
}

void AwtList::Dump(CDumpContext& dc) const
{
	//CWnd::Dump(dc);
	dc << "AwtList.m_hWnd = " << m_hWnd << "\n";
}

#endif //_DEBUG

