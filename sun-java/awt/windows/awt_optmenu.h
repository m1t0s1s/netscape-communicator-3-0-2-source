/*
 * @(#)awt_optmenu.h	1.16 95/11/29 Patrick P. Chan
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
// optmenu.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// AwtOptionMenu window

class AwtOptionMenu : public CComboBox
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, SETCOLOR, ADDITEM, REMOVE, SELECT, RESHAPE};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtOptionMenu();
	DECLARE_DYNCREATE(AwtOptionMenu)

// Operations
	// Creates a optionmenu control.
	static AwtOptionMenu *Create(Hjava_awt_Choice *pJavaObject, AwtWindow *pParent);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();
	
	// Sets BgFg color.  
	void SetColor(BOOL bFg, COLORREF color);

	// Sets the item at location 'nPos' (0-based).
	// Automatically creates enough slots to accommodate 'nPos'.
	// 'pzItem' will be freed by this member.
	void AddItem(char *pzItem, long nPos);

	// Selects the item at 'nPos'.
	void Select(long nPos);

	// Removes the item at location 'nPos'.
	void Remove(long nPos);

	// Returns the number of items (non-blocking).
	int Count();

	// If 'bMove', moves the window.  If 'bResize', resizes the window.
	void Reshape(BOOL bMove, long nX, long nY, 
				 BOOL bResize, long nWidth, long nHeight);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_OPTIONMENU, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_OPTIONMENU, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);


// Variables																	
public:
	COLORREF m_bgfgColor[2];

private:
	// Pointer to oak optionmenu object.
	Hjava_awt_Choice *m_pJavaObject;	

	// The width longest item called by AddItem.
	long m_nMaxWidth;

protected:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AwtOptionMenu)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~AwtOptionMenu();

	// Generated message map functions
protected:
	//{{AFX_MSG(AwtOptionMenu)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
    int m_nItems;
};

/////////////////////////////////////////////////////////////////////////////
