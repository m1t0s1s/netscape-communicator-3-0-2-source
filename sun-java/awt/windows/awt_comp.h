/*
 * @(#)awt_comp.h	1.14 95/12/05
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
#ifndef _AWTCOMP_H_
#define _AWTCOMP_H_

#include "awt_font.h"
#include "awt_event.h"

extern CMapPtrToPtr componentList;

class AwtComp
{
public:
	// These are the messages that can be sent to this class.
	enum {INITIALIZE, SHOW, ENABLE, SETFOCUS, NEXTFOCUS,
		  RESHAPE, REPAINT, DISPOSE, SETCOLOR, SETFONT};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

	AwtComp();
	~AwtComp();

	static void Initialize(CWnd *pWnd, BOOL bEnabled, 
                               Hjava_awt_Component *pJavaObject);
	static void Show(CWnd *pWnd, BOOL bShow, BOOL bOnTop = FALSE);
	static void Enable(CWnd *pWnd, BOOL bEnable);
	static void SetFocus(CWnd *pWnd);
	static void NextFocus(CWnd *pWnd);
	static void Reshape(CWnd *pWnd, long nX, long nY, long nWidth, long nHeight);
	static void Repaint(CWnd *pWnd, Hjava_awt_Component* pJavaObject, long nX, long nY, long nWidth, long nHeight);
	static void Dispose(CWnd *pWnd);
	static void SetColor(CWnd *pWnd, BOOL bFg, COLORREF color);
	static void SetFont(CWnd *pWnd, AwtFont *pFont);
	static AwtEvent* LookupComponent(HWND hwnd);
	static void Cleanup();

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	static void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		((CAwtApp*)AfxGetApp())->PostAwtMsg(CAwtApp::AWT_COMP, nMsg, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	static long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return ((CAwtApp*)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_COMP, nMsg, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);
};

#endif // _AwtComp_H_
