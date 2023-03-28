/*
 * @(#)awt_filedlg.cpp	1.15 95/12/08
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
// filedlg.cpp : implementation file
//

#include "stdafx.h"
#include "awt.h"
#include "awt_filedlg.h"
#include "awt_window.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// AwtFileDialog dialog

long AwtFileDialog::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
							  long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:
			return (long)Create((Hjava_awt_FileDialog*)p1, (MainFrame*)p2, 
				(BOOL)p3, (char*)p4, (char*)p5);
			break;
		case SHOW:
			((AwtFileDialog*)p1)->Show((BOOL)p2);
			break;
		case SETDIRECTORY:
			((AwtFileDialog*)p1)->SetDirectory((char*)p2);
			break;
		case SETFILE:
			((AwtFileDialog*)p1)->SetFile((char*)p2);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtFileDialog::AwtFileDialog(MainFrame *pParent, BOOL bOpen) : CFileDialog(bOpen, "*.*", NULL,
	OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, 
	"| All Files (*.*) | *.* ||", pParent)
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtFileDialog");

	//{{AFX_DATA_INIT(AwtFileDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

AwtFileDialog::~AwtFileDialog()
{
	if (m_ofn.lpstrFile != NULL) {
		free((char*)m_ofn.lpstrFile);
		m_ofn.lpstrFile = NULL;
	}
	if (m_ofn.lpstrTitle != NULL) {
		free((char*)m_ofn.lpstrTitle);
		m_ofn.lpstrTitle = NULL;
	}
	if (m_ofn.lpstrInitialDir != NULL) {
		free((char*)m_ofn.lpstrInitialDir);
		m_ofn.lpstrInitialDir = NULL;
	}

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

AwtFileDialog *AwtFileDialog::Create(Hjava_awt_FileDialog *pJavaObject, 
	MainFrame *pParent, BOOL bOpen, char *pzTitle, char *pzFile)
{
	if (!((CAwtApp *)AfxGetApp())->IsServerThread()) {
	    return (AwtFileDialog *)((CAwtApp *)AfxGetApp())->SendAwtMsg(CAwtApp::AWT_FILEDIALOG, CREATE,
			(long)pJavaObject, (long)pParent, (long)bOpen, (long)pzTitle, (long)pzFile);
	}
	AwtFileDialog *pDialog = new AwtFileDialog(pParent, bOpen);

        pDialog->m_ofn.lpstrFile = (char*)calloc(1, MAX_PATH+1);
        pDialog->m_ofn.nMaxFile = MAX_PATH;

	pDialog->m_pJavaObject = pJavaObject;
	pDialog->m_ofn.lpstrTitle = pzTitle;
        strncpy(pDialog->m_ofn.lpstrFile, pzFile, MAX_PATH);
        pDialog->m_ofn.lpstrFilter = NULL;
        pDialog->m_ofn.nFilterIndex = 0;

	return pDialog;
}

void AwtFileDialog::Show(BOOL show)
{
    CString result;
    int len;

    if (show) {
	switch (DoModal()) {
	case IDOK:
		result = GetPathName();
		len = result.GetLength();

		GetApp()->DoCallback(m_pJavaObject, "handleSelected", "(Ljava/lang/String;)V", 
			1, (long)makeJavaString(result.GetBuffer(1), len));
		result.ReleaseBuffer(len);
		break;
	case IDCANCEL:
		GetApp()->DoCallback(m_pJavaObject, "handleCancel", "()V"); 
		break;
	}
    } else {
        if (::IsWindow(m_hWnd)) {
            ShowWindow(SW_HIDE);
        }
    }
}

void AwtFileDialog::SelectedFile(CString *string)
{
	*string = GetPathName();
}

void AwtFileDialog::SetDirectory(char *pzDirectory)
{
    if (m_ofn.lpstrInitialDir != NULL) {
        free((void*)m_ofn.lpstrInitialDir);
    }
    m_ofn.lpstrInitialDir = pzDirectory;
}

void AwtFileDialog::SetFile(char *pzFile)
{
    strncpy(m_ofn.lpstrFile, pzFile, MAX_PATH);
}

void AwtFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AwtFileDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AwtFileDialog, CFileDialog)
	//{{AFX_MSG_MAP(AwtFileDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// AwtFileDialog message handlers

/////////////////////////////////////////////////////////////////////////////
// AwtButton diagnostics

#ifdef _DEBUG
void AwtFileDialog::AssertValid() const
{
	CFileDialog::AssertValid();
}

void AwtFileDialog::Dump(CDumpContext& dc) const
{
	//CWnd::Dump(dc);
	dc << "AwtFileDialog.m_hWnd = " << m_hWnd << "\n";
}

#endif //_DEBUG

