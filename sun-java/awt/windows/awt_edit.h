/*
 * @(#)awt_edit.h	1.14 95/11/29 Patrick P. Chan
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
#ifndef _AWTEDIT_H_
#define _AWTEDIT_H_

#include "awtapp.h"
#include "awt_window.h"

class AwtEdit : public CEdit
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, GETTEXT, SETTEXT, INSERTTEXT, REPLACETEXT, SETFONT, 
		  SETSELECTION, GETSELECTIONSTART, GETSELECTIONEND,
		  GETECHOCHARACTER, SETECHOCHARACTER,
		  SETEDITABLE, SETCOLOR};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

	AwtEdit();
	~AwtEdit();
	DECLARE_DYNCREATE(AwtEdit)

// Operations								 
	// If 'pFont' == NULL, use default font.  If 'pzInitValue' != NULL,
	// this method assumes responsibility for freeing 'pzInitValue'.
	static AwtEdit *Create(Hjava_awt_TextField *pJavaObject, AwtWindow *pParent,
		AwtFont *pFont, char *pzInitValue, BOOL bEditable);
	static AwtEdit *Create(Hjava_awt_TextArea *pJavaObject, AwtWindow *pParent,
		AwtFont *pFont, char *pzInitValue, BOOL bEditable, int nColumns, int nRows);

	void Dispose();
		
	// Sets BgFg color.
	void SetColor(BOOL bFg, COLORREF color);
	
	// Returns the text in 'string'.
	void GetText(CString &string);

	// Returns the size of the text 'pzText' using the current font.
	CSize ComputeSize(char *pzText);
	
	// 'pzText' will be freed by this member.  'pzText' can be NULL which
	// means to clear the all text.
	void SetText(char *pzText);

	// 'pzText' will be freed by this member.  
	// The selection will contain the inserted text.	
	void InsertText(char *pzText, long nPos);

	// 'pzText' will be freed by this member.  The selection will end up
	// at nStart.
	void ReplaceText(char *pzText, long nStart, long nEnd);

	// Returns the number of characters in the control.
	// Newlines are considered 1 character.
	long GetLength();

	// The position nPos is a location where newlines are a single character.
	// Returns a position where newlines are two characters long.
	long ToNative(long nPos);

	// Sets the selection [nStart, nEnd).
	void SetSelection(long nStart, long nEnd);

	// Returns the start of the selection.
	long GetSelectionStart();

	// Returns the end of the selection.
	long GetSelectionEnd();

	// If 'pFont' == NULL, use the default BtnFont.
	void SetFont(AwtFont *pFont);
	
	void SetEditable(BOOL bEditable);

	// If the echo character is not 0, all characters will
	// be displayed as the echo character.  When the echo
	// character is set or reset, all characters in the edit
	// control are updated.
	char GetEchoCharacter();
	void SetEchoCharacter(char ch);

	static void AwtMsgProc(AwtMsg *pMsg);
	
// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_EDIT, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_EDIT, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

private:
// Operations
	// If 'pFont' == NULL, use the default font.							 	
	static AwtEdit *Create(BOOL bField, void *pJavaObject, AwtWindow *pParent, 
		AwtFont *pFont, char *pzInitValue, BOOL bEditable, int nColumns, int nRows);

// Variables
public:
	COLORREF m_bgfgColor[2];

protected:
	AwtWindow *m_pParent;

	Hjava_awt_TextField *m_pFieldObject;
	Hjava_awt_TextArea *m_pAreaObject;

	BOOL m_bEditable;

	// BgFg color.
	COLORREF m_color[2];

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AwtEdit)
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(AwtEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

#endif // _AWTEDIT_H_
