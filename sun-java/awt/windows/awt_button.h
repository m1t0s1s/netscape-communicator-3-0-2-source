/*
 * @(#)awt_button.h	1.15 95/11/29 Patrick P. Chan
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
#ifndef _AWTBUTTON_H_
#define _AWTBUTTON_H_

#include "awtapp.h"
#include "awt_window.h"


class AwtButton : public CButton
{
public: 
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, SETCOLOR, GETSTATE, SETSTATE, SETTEXT};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtButton();
	~AwtButton();
	DECLARE_DYNCREATE(AwtButton)

// Operations
	// Creates the Windows button control.  pJavaObject refers to the oak object.
	// pParent will be the parent of the button.  pzLabel is the label
	// on the button.  The button is created enabled.
	static AwtButton *CreateButton(Hjava_awt_Button *pJavaObject, AwtWindow *pParent, char *pzLabel);
	static AwtButton *CreateToggle(Hjava_awt_Checkbox *pJavaObject, AwtWindow *pParent, char *pzLabel);
	static AwtButton *CreateRadio(Hjava_awt_Checkbox *pJavaObject, AwtWindow *pParent, char *pzLabel);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	void SetColor(BOOL bFg, COLORREF color);

	// Can only be used on a toggle button.
	// If bOn is TRUE, checks the toggle button; otherwise removes the check.
	void SetState(BOOL bOn);
	
	// Can only be used on a toggle button.
	// Returns the state of the toggle.
	BOOL GetState();

	// Changes the label of the button.  NULL == "".
	void SetText(char *pzLabel);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_BUTTON, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_BUTTON, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

protected:
// Operations
	// Code shared by CreateButton and CreateToggle
	static AwtButton *Create(void *pOakObject, UINT nButtonType, AwtWindow *pParent, char *pzLabel);

// Variables
public:
	COLORREF m_bgfgColor[2];

protected:
	// This window's parent.
	AwtWindow *m_pParent;
	
	// The Oak button object.
	Hjava_awt_Button *m_pButtonObject;
	Hjava_awt_Checkbox *m_pToggleObject;

	// TRUE iff this is a push button control.
	BOOL m_bPushButton;

	// TRUE iff the button is enabled.
	BOOL m_bEnabled;

// Overrides
protected:
	// Called by 'Create' and by subsclasses to initialize the button.
	void Init(void *pJavaObject, UINT nButtonType, AwtWindow *pParent, char *pzLabel);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AwtButton)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(AwtButton)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};


#endif // _AWTBUTTON_H_
