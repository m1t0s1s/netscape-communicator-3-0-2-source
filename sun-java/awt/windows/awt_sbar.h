/*
 * @(#)awt_sbar.h	1.15 95/11/29 Patrick P. Chan
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
#ifndef _AWTSCROLLBAR_H_
#define _AWTSCROLLBAR_H_

#include "awt_window.h"


class AwtScrollbar : public CScrollBar
{
public: 
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, SETCOLOR, SETVALUE, SETVALUES, MINIMUM, MAXIMUM, 
		  VALUE, SHOW, RESHAPE};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtScrollbar();
	~AwtScrollbar();
	DECLARE_DYNCREATE(AwtScrollbar)

// Operations
public:
	// Creates the Windows scroll bar control.  pJavaObject refers to the oak object.
	// pParent will be the parent of the scrollbar.
 	static AwtScrollbar *Create(Hjava_awt_Scrollbar *pJavaObject, AwtWindow *pParent, 
		BOOL bVert, BOOL bStandard);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	// Sets BgFg color.
	void SetColor(BOOL bFg, COLORREF color);

	void SetValue(long nValue);
	void SetValues(long nMinPos, long nValue, long nMaxPos);

	long Minimum();

	long Maximum();

	long Value();

	void Show(BOOL bShow);

	// If 'bMove', moves the window.  If 'bResize', resizes the window.
	void Reshape(BOOL bMove, long nX, long nY, 
				 BOOL bResize, long nWidth, long nHeight);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_SCROLLBAR, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_SCROLLBAR, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

// Variables
public:
	// The Java scrollbar object.
	Hjava_awt_Scrollbar *m_pJavaObject;

	COLORREF m_bgfgColor[2];

private:
	// This window's parent.
	AwtWindow *m_pParent;
	
	// TRUE iff this vertical scrollbar.
	BOOL m_bVert;

	// If not NULL, this is a standard scrollbar and m_pWindow is the window's handle; 
	// otherwise, this is a scrollbar control.
	AwtWindow *m_pWindow;

	// BgFg color.
	COLORREF m_color[2];

// Overrides
protected:
	// Generated message map functions
	//{{AFX_MSG(AwtScrollbar)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};


#endif // _AWTSCROLLBAR_H_
