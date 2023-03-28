/*
 * @(#)awt_label.cpp	1.21 96/01/05 Patrick P. Chan
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
#include "awt_label.h"
#include "awt_graphics.h"
#include "java_awt_label.h"

IMPLEMENT_DYNCREATE(AwtLabel, CWnd)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(AwtLabel, CWnd)
	//{{AFX_MSG_MAP(AwtLabel)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	// Standard printing commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Construction/destruction

AwtLabel::AwtLabel()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtLabel");

        m_pSharedDC = NULL;
	m_pFont = NULL;
        m_alignment = DT_LEFT;
}

AwtLabel::~AwtLabel()
{
    if (m_pSharedDC != NULL) {
        m_pSharedDC->Detach(NULL);
        m_pSharedDC = NULL;
    }
    DestroyWindow();
    m_hWnd = NULL;

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtLabel::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						 long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((Hjava_awt_Label*)p1, (AwtWindow*)p2, (char*)p3);
			break;
		case DISPOSE:
			((AwtLabel *)p1)->Dispose();
			break;
		case SETFONT:
			((AwtLabel *)p1)->SetFont((AwtFont*)p2);
			break;
		case SETCOLOR:
			((AwtLabel *)p1)->SetColor((BOOL)p2, (COLORREF)p3);
			break;
		case SETTEXT:
			((AwtLabel *)p1)->SetText((char *)p2);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// AwtWindow operations

AwtLabel *AwtLabel::Create(Hjava_awt_Label *pJavaObject, AwtWindow *pParent, char *pzLabel)
{
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtLabel *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_LABEL, CREATE, 
	   			(long)pJavaObject, (long)pParent, (long)pzLabel);
	}
	CRect rect;
	AwtLabel *pLabel = new AwtLabel();
	
	pLabel->m_pParent = pParent;
	pLabel->m_pJavaObject = pJavaObject;
	pLabel->m_color[TRUE] = RGB(0, 0, 0);

	rect = GetApp()->ComputeRect(pzLabel);
	pLabel->CWnd::Create(NULL, pzLabel, WS_CHILD|WS_CLIPSIBLINGS,
							rect, pParent, 0);
	return pLabel;
}

void AwtLabel::Dispose()
{
    delete this;
}

void AwtLabel::SetFont(AwtFont *pFont)
{
	m_pFont = pFont;
	InvalidateRect(NULL);
}

void AwtLabel::SetColor(BOOL bFg, COLORREF color)
{
	m_color[bFg] = color; 
	InvalidateRect(NULL);
}

void AwtLabel::SetText(char *pzLabel)
{
//temp workaround; don't understand the problem yet.
if (this == NULL) return;

	CRect rect;
	BOOL bNull = FALSE;
	
	if (pzLabel == NULL) {
		pzLabel = "";
		bNull = TRUE;
	}
	SetWindowText(pzLabel);
	InvalidateRect(NULL);
	rect = GetApp()->ComputeRect(pzLabel);
//	Reshape(FALSE, 0, 0, TRUE, rect.Width(), rect.Height());
	if (!bNull) {
		free(pzLabel);
	}
}

void AwtLabel::SetAlignment(long awt_alignment)
{
    switch(awt_alignment) {
      default:
          AWT_TRACE(("Invalid label alignment: %ld\n", awt_alignment));
      case java_awt_Label_LEFT:
          m_alignment = DT_LEFT;
          break;
      case java_awt_Label_CENTER:
          m_alignment = DT_CENTER;
          break;
      case java_awt_Label_RIGHT:
          m_alignment = DT_RIGHT;
          break;
    }
    InvalidateRect(NULL);
}

/////////////////////////////////////////////////////////////////////////////
// AwtWindow message handlers

void AwtLabel::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	CRect margin;
	CString label;
	
	GetClientRect(&rect);

	dc.SetTextColor(m_color[TRUE]);
	dc.SetBkMode(TRANSPARENT);
	dc.SelectObject(m_pFont);
	dc.FillRect(rect, &CBrush(m_pParent->m_bgfgColor[FALSE]));
	
	margin = GetApp()->ComputeRect(NULL);
	rect.left += margin.right / 2;
	GetWindowText(label);
	dc.DrawText(label, label.GetLength(), rect, 
                    m_alignment | DT_VCENTER | DT_SINGLELINE);

	dc.SelectStockObject(SYSTEM_FONT);
}

/////////////////////////////////////////////////////////////////////////////
// AwtLabel diagnostics

#ifdef _DEBUG
void AwtLabel::AssertValid() const
{
	CWnd::AssertValid();
}

void AwtLabel::Dump(CDumpContext& dc) const
{
	dc << "\nAwtLabel.m_hWnd = " << (UINT)m_hWnd << "\n";
}

#endif //_DEBUG

