/*
 * @(#)awt_filedlg.h	1.17 95/12/08
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
// filedlg.h : header file
//

#ifndef _AWTFILEDIALOG_H_
#define _AWTFILEDIALOG_H_

#include "awt.h"
#include "awt_window.h"

/////////////////////////////////////////////////////////////////////////////
// AwtFileDialog dialog

class AwtFileDialog : public CFileDialog
{
public:
	enum {CREATE, SHOW, SETDIRECTORY, SETFILE};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtFileDialog(MainFrame *pParent, BOOL bOpen);   // standard constructor
	~AwtFileDialog();

// Dialog Data
	//{{AFX_DATA(AwtFileDialog)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Operations
	// This doesn't really create the dialog; it's been created
	// by the constructor.  However, it's here for consistency with
	// the rest of the AWT classes.  
	// If bOpen is true, an openfile dialog is created; otherwise a
	// savefile dialog is created.
	// 'pzTitle' appears in the title bar while 'pzFile' is the initial filename.
	// This member assumes responsibility for freeing 'pzTitle' and 'pzFile'.
	static AwtFileDialog *Create(Hjava_awt_FileDialog *pJavaObject, 
		MainFrame *pParent, BOOL bOpen, char *pzTitle, char *pzFile);

	// Displays the dialog box.  Does not return until the user
	// is done with it.  
	void Show(BOOL show);

	// Sets 'string' with the selected file.
	void SelectedFile(CString *string);

	// This member assumes responsibility for freeing 'pzDirectory'.
	void SetDirectory(char *pzDirectory);

	void SetFile(char *pzFile);

// Awt Message-Related Operations

	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_FILEDIALOG, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_FILEDIALOG, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

// Variables
	Hjava_awt_FileDialog *m_pJavaObject;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AwtFileDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) { return FALSE; };
	BOOL OnCommand(WPARAM wParam, LPARAM lParam) { return FALSE; }; 

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(AwtFileDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

#endif // _AWTFILEDIALOG_H_
