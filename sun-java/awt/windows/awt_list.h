/*
 * @(#)awt_list.h	1.16 95/11/29 Thomas Ball
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
// list.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// AwtList window

class AwtList : public CListBox
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, SETCOLOR, SETMULTIPLESELECTIONS, ADDITEM, DELETEITEMS, 
		  ISSELECTED, SELECT, MAKEVISIBLE, RESHAPE};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtList();
	DECLARE_DYNCREATE(AwtList)

// Operations
public:
	// Creates a listbox control.
	static AwtList *Create(Hjava_awt_List *pJavaObject, AwtWindow *pParent);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	void SetColor(BOOL bFg, COLORREF color);
	
	// If bMulti is TRUE, the list will allow multiple selections.
	void SetMultipleSelections(BOOL bMulti);
	
	// Adds the item at the specified position in the list where
	// -1 means add to the end of the list.  pzItem will be freed by
	// this member.  The list maintains a maxWidth value which is the length
	// of the longest item in the list.  Calling AddItem might increase
	// maxWidth.  maxWidth is never decreased even if the longest item
	// is delete by DeleteItems.  If the list was created 'bResizable' TRUE,
	// the actual width of the list will be adjusted to match maxWidth.
	void AddItem(char *pzItem, long index);

	// Scroll the control so that nPos is visible somewhere near the middle.
	void MakeVisible(int nPos);

	// Returns true if the item at nPos is selected.
	BOOL IsSelected(int nPos);

	// Selects/deselects the nPos item.
	void Select(int nPos, BOOL bSelect);

	// Remove the items nStart to nEnd, inclusive.
	void DeleteItems(int nStart, int nEnd);

	// Reshapes the control.
	void Reshape(BOOL bMove, long nX, long nY, BOOL bResize, long nWidth, long nHeight);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_LIST, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_LIST, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);

// Variables
public:
	COLORREF m_bgfgColor[2];

private:
	// Pointer to oak list object.
	Hjava_awt_List *m_pJavaObject;

	// TRUE if multi-selection list box.
	BOOL m_bMulti;

	// BgFg color
	COLORREF m_color[2];

	// The width longest item called by AddItem.
	long m_nMaxWidth;

protected:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AwtList)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~AwtList();

	// Generated message map functions
protected:
	//{{AFX_MSG(AwtList)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
