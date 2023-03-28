/*
 * @(#)awt_menu.h	1.17 95/11/29 Patrick P. Chan
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
#include "awt_mainfrm.h"

class AwtMenu : public CMenu
{
public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE, APPENDITEM, APPENDSEPARATOR, ENABLE,
              ENABLEITEM, ISENABLED, SETMARK, ISMARKED, DISPOSEITEM};

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Construction
	AwtMenu();
	~AwtMenu();
	DECLARE_DYNCREATE(AwtMenu)

// Operations
public:
	// Crates the Windows menu control
	static AwtMenu *Create(void *pJavaObject, MainFrame *pFrame, HMENU hMenuBar, char *pzLabel);
	UINT AppendItem(Hjava_awt_MenuItem *pItemObject, char *pzItemName);
	void AppendSeparator();
	void Enable(BOOL bEnable);
	void EnableItem(UINT nCmdID, BOOL bEnable);
	BOOL IsEnabled(UINT nCmdID);
	void SetMark(UINT nCmdID, BOOL bMark);
	BOOL IsMarked(UINT nCmdID);
	MainFrame *GetFrame() { return m_pFrame; };
	void DisposeItem(UINT nCmdID);
	void Dispose();

	void InvokeItem(UINT nCmdID);

// Awt Message-Related Operations
	
	// Posts a message to the server thread.  Returns immediately and does not wait for a reply.
	void PostMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
					long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		GetApp()->PostAwtMsg(CAwtApp::AWT_MENU, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Sends a message to the server thread and waits for a reply.  
	// This call blocks until the operation is completed.  The result is returned.
	long SendMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						long p4 = NULL, long p5 = NULL, long p6 = NULL) {
		return GetApp()->SendAwtMsg(CAwtApp::AWT_MENU, nMsg, (long)this, p1, p2, p3, p4, p5, p6);
	}

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, long p3 = NULL, 
						  long p4 = NULL, long p5 = NULL, long p6 = NULL, long p7 = NULL, long p8 = NULL);


// Variables
private:
	MainFrame *m_pFrame;
	HMENU m_hMenuBar;
	void *m_pJavaObject;

	// Maps CmdID's to Hjava_awt_MenuItem objects.
	CMapWordToPtr m_ItemMap;
};
