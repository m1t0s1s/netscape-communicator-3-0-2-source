/*
 * @(#)awt_edit.cpp	1.21 95/11/28 Patrick P. Chan
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
#include "awt.h"
#include "awtapp.h"
#include "awt_fontmetrics.h"
#include "awt_edit.h"


IMPLEMENT_DYNCREATE(AwtEdit, CEdit)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(AwtEdit, CEdit)
	//{{AFX_MSG_MAP(AwtEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

AwtEdit::AwtEdit()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtEdit");

	m_pFieldObject = NULL;
	m_pAreaObject = NULL;
	m_bgfgColor[FALSE] = RGB(255, 255, 255);
	m_bgfgColor[TRUE] = RGB(0, 0, 0);
	GetApp()->CreateBrush(m_bgfgColor[FALSE]);
	GetApp()->CreateBrush(m_bgfgColor[TRUE]);
}

AwtEdit::~AwtEdit()
{
	GetApp()->ReleaseBrush(m_bgfgColor[FALSE]);
	GetApp()->ReleaseBrush(m_bgfgColor[TRUE]);
	DestroyWindow();
        m_hWnd = NULL;

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtEdit::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((BOOL)p1, (void*)p2,
				(AwtWindow*)p3, (AwtFont*)p4, (char *)p5, (BOOL)p6,
				(int)p7, (int)p8);
			break;
		case DISPOSE:
			((AwtEdit*)p1)->Dispose();
			break;
		case SETCOLOR:
			((AwtEdit*)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		case GETTEXT:
			((AwtEdit*)p1)->GetText(*(CString*)(p2));
			break;
		case SETTEXT:
			((AwtEdit*)p1)->SetText((char *)p2);
			break;
		case INSERTTEXT:
			((AwtEdit*)p1)->InsertText((char *)p2, (long)p3);
			break;
		case REPLACETEXT:
			((AwtEdit*)p1)->ReplaceText((char *)p2, (long)p3, (long)p4);
			break;
		case SETSELECTION:
			((AwtEdit*)p1)->SetSelection((long)p2, (long)p3);
			break;
		case GETSELECTIONSTART:
			return (long)((AwtEdit*)p1)->GetSelectionStart();
			break;
		case GETSELECTIONEND:
			return (long)((AwtEdit*)p1)->GetSelectionEnd();
			break;
		case SETFONT:
			((AwtEdit*)p1)->SetFont((AwtFont*)p2);
			break;
		case GETECHOCHARACTER:
			return (long)((AwtEdit*)p1)->GetEchoCharacter();
			break;
		case SETECHOCHARACTER:
			((AwtEdit*)p1)->SetEchoCharacter((char)p2);
			break;
		case SETEDITABLE:
			((AwtEdit*)p1)->SetEditable((BOOL)p2);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtEdit *AwtEdit::Create(Hjava_awt_TextField *pJavaObject, AwtWindow *pParent, 
	AwtFont *pFont, char *pzInitValue, BOOL bEditable)
{
	return Create(TRUE, (void*)pJavaObject, pParent, pFont, pzInitValue, bEditable, 0, 0);
}

AwtEdit *AwtEdit::Create(Hjava_awt_TextArea *pJavaObject, AwtWindow *pParent, 
	AwtFont *pFont, char *pzInitValue, BOOL bEditable, int nColumns, int nRows)
{
	return Create(FALSE, (void*)pJavaObject, pParent, pFont, pzInitValue, bEditable, nColumns, nRows);
}

// This is a temporary workaround; motif widgets have a 2 pixel blank border around their buttons.
#define TEMP_BORDER 3

AwtEdit *AwtEdit::Create(BOOL bField, void *pJavaObject, AwtWindow *pParent, 
	AwtFont *pFont, char *pzInitValue, BOOL bEditable, int nColumns, int nRows)
{
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtEdit *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_EDIT, CREATE, (long)bField, (long)pJavaObject, 
	    	(long)pParent, (long)pFont, (long)pzInitValue, (long)bEditable, nColumns, nRows);
	}

	AwtEdit *pEdit = new AwtEdit();
	DWORD dwStyle, dwExStyle;
	CRect rect;
	
	dwStyle = WS_CHILD|WS_CLIPSIBLINGS|WS_BORDER|ES_AUTOVSCROLL|ES_AUTOHSCROLL;
	if (bField) {
		pEdit->m_pFieldObject = (Hjava_awt_TextField *)pJavaObject;
	} else {
		pEdit->m_pAreaObject = (Hjava_awt_TextArea *)pJavaObject;
		dwStyle |= ES_MULTILINE|WS_HSCROLL|WS_VSCROLL;
	}
	dwExStyle = GetApp()->m_bWin95UI ? WS_EX_CLIENTEDGE : 0;
		
	pEdit->m_pParent = pParent;
	pEdit->m_bEditable = bEditable;
	if (!bEditable) dwStyle |= ES_READONLY;
	rect= GetApp()->ComputeRect(pzInitValue);
	VERIFY(pEdit->CWnd::CreateEx(dwExStyle, _T("EDIT"), NULL, dwStyle, rect.left, rect.right,
		rect.Width(), rect.Height(), pParent->GetSafeHwnd(), 0, 0));

	// Reshape after creation because we need to get the control's font.
	pEdit->SetFont(pFont);
	pEdit->SetText(pzInitValue);
	if (nColumns > 0 && nRows > 0) {
		CSize size = AwtFontMetrics::TextSize(pFont ? pFont : pEdit->GetFont(), 
			nColumns, nRows);
		rect.left = rect.top = 0;
		rect.right = size.cx + ::GetSystemMetrics(SM_CYVSCROLL) + 4;  // need to figure out how to remove the extra constant.
		rect.bottom = size.cy + ::GetSystemMetrics(SM_CYVSCROLL) + 8;
		VERIFY(::AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle));
	}
	return pEdit;
}

void AwtEdit::Dispose() {
    delete this;
}

void AwtEdit::SetColor(BOOL bFg, COLORREF color)
{
	GetApp()->ReleaseBrush(m_bgfgColor[bFg]);
	m_bgfgColor[bFg] = color;
	GetApp()->CreateBrush(color);
}

void AwtEdit::GetText(CString &string)
{
	GetWindowText(string);
}

void AwtEdit::SetText(char *pzText)
{
	SetWindowText(pzText);

	if (pzText != NULL) {
/*
		// Reshape the control such that all of the text is displayed
		// without scroll bars.
		if (m_pAreaObject != NULL) {
			Classawt_TextArea *pClass = unhand(m_pAreaObject);
			CSize size = ComputeSize(pzText);
 			CRect rect(0, 0, size.cx, size.cy);

			// I thought AdjustWindowRectEx adjusted for width of the scroll bars 
			// but it doesn't seem to.  Need to figure out the right thing to do.
			rect.right += 20;
			rect.bottom += 20;	
			VERIFY(::AdjustWindowRectEx(&rect, WS_HSCROLL|WS_VSCROLL, 
				FALSE, GetApp()->m_bWin95 ? WS_EX_CLIENTEDGE : 0));
			Reshape(FALSE, 0, 0, TRUE, rect.right, rect.bottom);
			//pClass->width = rect.right;
			//pClass->height = rect.bottom;
		}
*/
		free(pzText);
	}
}

void AwtEdit::InsertText(char *pzText, long nPos)
{
	SetSel(ToNative(nPos), ToNative(nPos));
	ReplaceSel(pzText);
	if (pzText != NULL) {
		free(pzText);
	}
}

void AwtEdit::ReplaceText(char *pzText, long nStart, long nEnd)
{
	SetSel(ToNative(nStart), ToNative(nEnd));
	ReplaceSel(pzText);
	if (pzText != NULL) {
		free(pzText);
	}
}

long AwtEdit::GetLength()
{
	return GetWindowTextLength() - GetLineCount() + 1;
}

CSize AwtEdit::ComputeSize(char *pzText)
{
	CDC *pDC = GetDC();
	CFont *pCurFont = GetFont();
	CFont *pOldFont;
	int i = 0;
	int nLen;
	CSize size;
	CSize maxSize(0, 0);
	
	VERIFY(pOldFont = (CFont*)pDC->SelectObject(pCurFont));
	while (*pzText != 0) {
		char *pzEnd = strchr(pzText, '\n');
		if (pzEnd == NULL) {
			pzEnd = strchr(pzText, 0);
		}
		nLen = pzEnd - pzText;
		VERIFY(::GetTextExtentPoint32(pDC->m_hDC, pzText, nLen, &size));
		maxSize.cx = max(maxSize.cx, size.cx);
		maxSize.cy += size.cy;
		pzText += nLen;
		if (*pzText != 0) {
			pzText++;
		}
	}
	VERIFY((CFont *)pDC->SelectObject(pOldFont));
	ReleaseDC(pDC);
	return maxSize;
}

void AwtEdit::SetFont(AwtFont *pFont) {
	CRect rect;

	if (pFont == NULL) {
		SendMessage(WM_SETFONT, (WPARAM)GetApp()->m_hBtnFont);
	} else {
		SendMessage(WM_SETFONT, (WPARAM)pFont->m_hObject);
	}
}

char AwtEdit::GetEchoCharacter()
{
	return GetPasswordChar();
}

void AwtEdit::SetEchoCharacter(char ch)
{
	SetPasswordChar(ch);
}

void AwtEdit::SetEditable(BOOL bEditable)
{
	m_bEditable = bEditable;
	SetReadOnly(!bEditable);
}

long AwtEdit::ToNative(long nPos) {
	int nResult;

	nResult = nPos + LineFromChar(nPos);			
	while (nPos > nResult - LineFromChar(nResult)) {
		nResult++;
	}
	if (LineFromChar(nResult) != LineFromChar(nResult + 1)) {
		nResult++;
	}
	return nResult;
}

void AwtEdit::SetSelection(long nStart, long nEnd)
{
	SetSel(ToNative(nStart), ToNative(nEnd));
}

long AwtEdit::GetSelectionStart() {
	int nStart, nEnd;

	GetSel(nStart, nEnd);
	return nStart - LineFromChar(nStart);
}

long AwtEdit::GetSelectionEnd() {
	int nStart, nEnd;

	GetSel(nStart, nEnd);
	return nEnd - LineFromChar(nEnd);
}

void AwtEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == 13 && m_pFieldObject != NULL) {
		AWT_TRACE(("textField.action\n"));
		GetApp()->DoCallback(m_pFieldObject, "action", "()V");
	} else {
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}

/////////////////////////////////////////////////////////////////////////////
// AwtEdit diagnostics

#ifdef _DEBUG
void AwtEdit::AssertValid() const
{
	CWnd::AssertValid();
}

void AwtEdit::Dump(CDumpContext& dc) const
{
	dc << "\nAwtEdit.m_hWnd = " << (UINT)m_hWnd << "\n";
}

#endif //_DEBUG


