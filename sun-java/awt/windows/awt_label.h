/*
 * @(#)awt_label.h	1.18 96/01/05 Patrick P. Chan
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
#include "awt_window.h"

#ifndef _AWTLABEL_H_
#define _AWTLABEL_H_

class AwtLabel : public AwtCWnd
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, SETFONT, SETCOLOR, SETTEXT};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtLabel();
	~AwtLabel();
	DECLARE_DYNCREATE(AwtLabel)

// Operations
public:
	// Creates the Window static control with the specified label.
	// The label is left justified.  The text color black by default.
	// This member assumes responsibility for freeing pzLabel.
	static AwtLabel *Create(Hjava_awt_Label *pJavaObject, AwtWindow *pParent, char *pzLabel);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	// Sets BgFg color.  
	void SetColor(BOOL bFg, COLORREF color);

	// Sets the font.
	void SetFont(AwtFont *pFont);

	// Sets the text alignment, using java.awt.Label constants.
	void SetAlignment(long awt_alignment);

	// Changes the label the width is adjusted.  
	// This member assumes responsibility for freeing pzLabel. NULL == "".
	void SetText(char *pzLabel);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_LABEL, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_LABEL, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

// Variables
	// This window's parent.
	AwtWindow *m_pParent;

private:
	// The Java label object.
	Hjava_awt_Label *m_pJavaObject;

	// BgFg color.
	COLORREF m_color[2];

	// Current font.
	AwtFont *m_pFont;

	// Current alignment.
	UINT m_alignment;

// Generated message map functions
protected:
	//{{AFX_MSG(AwtLabel)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

#endif // _AWTLABEL_H_
