/*
 * @(#)awt_comp.cpp	1.22 95/12/05
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
#include "awt_comp.h"
#include "awt_edit.h"
#include "awt_list.h"
#include "awt_fontmetrics.h"
#include "awt_label.h"
#include "awt_sbar.h"
#include "awt_window.h"

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

// Map HWND's to Java components.
CMapPtrToPtr componentList;

AwtComp::AwtComp()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtComp");
}

AwtComp::~AwtComp()
{
    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtComp::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						  long p4, long p5, long p6, long p7, long p8) {
	if (p1 == NULL) {
        AWT_TRACE(("NULL window in CompMsg #%d(%d,%d,%d,%d,%d,%d,%d,%d)\n",
		    nMsg, p1, p2, p3, p4, p5, p6, p7, p8));
	    return 0;
	}
	if (!::IsWindow(((CWnd*)p1)->m_hWnd)) {
		return 0;
	}
	
	switch (nMsg) {
		case INITIALIZE:
			Initialize((CWnd*)p1, (BOOL)p2, (Hjava_awt_Component *)p3);
			break;
		case SHOW:
			Show((CWnd*)p1, (BOOL)p2);
			break;
		case ENABLE:
			Enable((CWnd*)p1, (BOOL)p2);
			break;
		case SETFOCUS:
			SetFocus((CWnd*)p1);
			break;
		case NEXTFOCUS:
			NextFocus((CWnd*)p1);
			break;
		case RESHAPE:
			Reshape((CWnd*)p1, p2, p3, p4, p5);
			break;
		case REPAINT:
			Repaint((CWnd*)p1, (Hjava_awt_Component*)p2, p3, p4, p5, p6);
			break;
		case DISPOSE:
			Dispose((CWnd*)p1);
			break;
		case SETCOLOR:
			SetColor((CWnd*)p1, (BOOL)p2, (COLORREF)p3);
			break;
		case SETFONT:
			SetFont((CWnd*)p1, (AwtFont*)p2);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

void AwtComp::Initialize(CWnd *pWnd, BOOL bEnabled, Hjava_awt_Component *pJavaObject)
{
    AwtEvent* pEvent = new AwtEvent(pJavaObject);
    componentList.SetAt((void*)(pWnd->GetSafeHwnd()), pEvent);
    if (!bEnabled) {
        Enable(pWnd, bEnabled);
    }
}

void AwtComp::Show(CWnd *pWnd, BOOL bOn, BOOL bOnTop)
{
	pWnd->ShowWindow(bOn ? SW_SHOW : SW_HIDE);
	if (bOnTop) {
		pWnd->BringWindowToTop();
	}
}

void AwtComp::Enable(CWnd *pWnd, BOOL bOn)
{
	pWnd->EnableWindow(bOn);
}

void AwtComp::SetFocus(CWnd *pWnd)
{
	pWnd->SetFocus();
}

void AwtComp::NextFocus(CWnd *pWnd)
{
}

void AwtComp::Reshape(CWnd *pWnd, long nX, long nY, long nWidth, long nHeight)
{
	CWnd *parent = pWnd->GetParent();
        MainFrame* frame = (MainFrame*)pWnd->GetParentFrame();
        if ((CView*)parent == frame->GetMainWindow()) {
            frame->SubtractInsetPoint(nX, nY);
        }
	pWnd->SetWindowPos(NULL, nX, nY, nWidth, nHeight, SWP_NOACTIVATE|SWP_NOZORDER);
	// For some reason, text fields do not repaint themselves after being
	// resized.  Text areas do however, but we don't distinguish.  Fix this later.
	if (pWnd->IsKindOf(RUNTIME_CLASS(AwtEdit))) {
		pWnd->InvalidateRect(NULL);
	}
}

void AwtComp::Repaint(CWnd *pWnd, Hjava_awt_Component* pJavaObject, long nX, long nY, long nWidth, long nHeight)
{
	//pWnd->InvalidateRect(CRect(CPoint(nX, nY), CSize(nWidth, nHeight)), FALSE);
	GetApp()->DoCallback(pJavaObject, "handleRepaint", "(IIII)V",
		4, nX, nY, nWidth, nHeight);
}

void AwtComp::Dispose(CWnd *pWnd)
{
    AwtEvent* pEvent = LookupComponent(pWnd->GetSafeHwnd());
    if (pEvent != NULL) {
        delete pEvent;
        componentList.RemoveKey((void*)(pWnd->GetSafeHwnd()));
    }
    delete pWnd;
}

void AwtComp::SetColor(CWnd *pWnd, BOOL bFg, COLORREF color)
{
	// need to determine what kind of object pWnd is so that
	// we can call the SetColor method.  The alternative to this
	// is to modify the peer code in java so that setBackground and
	// setForeground is overriden in all the awt objects.
	if (pWnd->IsKindOf(RUNTIME_CLASS(AwtScrollbar))) {
		AwtScrollbar *pSbar = (AwtScrollbar*)pWnd;

		pSbar->SetColor(bFg, color);
	} else if (pWnd->IsKindOf(RUNTIME_CLASS(AwtWindow))) {
		AwtWindow *pWindow = (AwtWindow*)pWnd;

		pWindow->SetColor(bFg, color);
	} else if (pWnd->IsKindOf(RUNTIME_CLASS(AwtEdit))) {
		AwtEdit *pEdit = (AwtEdit*)pWnd;

		pEdit->SetColor(bFg, color);
	} else if (pWnd->IsKindOf(RUNTIME_CLASS(AwtButton))) {
		AwtButton *pButton = (AwtButton*)pWnd;

		pButton->SetColor(bFg, color);
	} else if (pWnd->IsKindOf(RUNTIME_CLASS(AwtList))) {
		AwtList *pList = (AwtList*)pWnd;

		pList->SetColor(bFg, color);
	}
//	pWnd->InvalidateRect(NULL);
}

void AwtComp::SetFont(CWnd *pWnd, AwtFont *pFont)
{
	if (pWnd->IsKindOf(RUNTIME_CLASS(AwtLabel))) {
		AwtLabel *pLabel = (AwtLabel*)pWnd;

		pLabel->SetFont(pFont);
		return;
	}
	if (pFont != NULL && pFont->m_nAscent < 0) {
		AwtFontMetrics::SetFontAscent(pFont);
	}
	pWnd->SendMessage(WM_SETFONT, (WPARAM)pFont->GetSafeHandle());
	pWnd->InvalidateRect(NULL);
}

// Given an HWND, return the associated AwtWindow.
AwtEvent* AwtComp::LookupComponent(HWND hwnd) {
    void *p;
    if (componentList.Lookup((void *)hwnd, p)) {
        return (AwtEvent*)p;
    }
    return NULL;
}

void AwtComp::Cleanup() {
    POSITION pos = componentList.GetStartPosition();
    while (pos != NULL) {
        void *key, *p;
        componentList.GetNextAssoc(pos, key, p);
        AwtEvent* pEvent = (AwtEvent*)p;
        if (pEvent != NULL) {
            delete pEvent;
        }
    }
    componentList.RemoveAll();
}
