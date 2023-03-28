/*
 * @(#)awtapp.cpp	1.40 95/12/08
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
/*-
 * awtapp.cpp : Defines the class behaviors for the application.
 */

#include "stdafx.h"
#include "awt.h"
#include "awtapp.h"
#include "awt_button.h"
#include "awt_comp.h"
#include "awt_edit.h"
#include "awt_event.h"
#include "awt_filedlg.h"
#include "awt_font.h"
#include "awt_fontmetrics.h"
#include "awt_graphics.h"
#include "awt_label.h"
#include "awt_list.h"
#include "awt_optmenu.h"
#include "awt_mainfrm.h"
#include "awt_menu.h"
#include "awt_sbar.h"
#include "awt_window.h"

extern "C" {
#include "javaThreads.h"
#include "exceptions.h"
#if defined(NETSCAPE)
extern _declspec(dllexport) int sysAtexit(void (*func)(void));

#define GET_APP_PALETTE_API "GetApplicationPalette"
typedef HPALETTE (*GetApplicationPalette)(void);
#else
extern int sysAtexit(void (*func)(void));
#endif				/* NETSCAPE */
}

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef ENABLE_CALLINGWINDOWS
CallingWindowsStruct CallingWindows[CALLINGWINDOWS_QUEUESIZE];
#endif

/////////////////////////////////////////////////////////////////////////////
// CAwtApp

BEGIN_MESSAGE_MAP(CAwtApp, CWinApp)
	//{{AFX_MSG_MAP(CAwtApp)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	// AWT message routing
//	ON_AWT_MESSAGE(WM_AWT_DISPATCH_EVENT, OnAwtDispatchEvent)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DllMain entry point

// This code is derived from DllMain in winmain.cpp.  There may
// be missing code.
extern "C" BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
	CAwtApp* pApp = (CAwtApp *)AfxGetApp();

	if (dwReason == DLL_PROCESS_ATTACH) {
	    // initialize DLL instance
	    if (!AfxWinInit(hInstance, NULL, &afxChNil, 0)) {
		AfxWinTerm();
		return FALSE;
	    }
	    // initialize application instance
	    if (pApp != NULL && !pApp->InitInstance()) {
		pApp->ExitInstance();
		AfxWinTerm();
		return FALSE;
	    }
	} else if (dwReason == DLL_PROCESS_DETACH) {
	    AfxWinTerm();

#if defined(NETSCAPE) && defined(DEBUG)
        AwtCList *ptr;
        ptr = (AwtCList *)__AllAwtObjects.next;
        PR_LOG(AWT, out, ("Dumping remaining AwtObjects:"));
        while (ptr != &__AllAwtObjects) {
            PR_LOG(AWT, out, ("    %s", ptr->name));
            ptr = (AwtCList *)ptr->next;
        }
#endif
        }
	return TRUE;
}

void handleExit(void)
{
    (CAwtApp *)AfxGetApp()->ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CAwtApp construction
extern "C" ClassClass *GetThreadClassBlock();

CAwtApp::CAwtApp()
{
    // Initialize the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST;

    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtApp");

	OSVERSIONINFO osVersionInfo;	
	
	m_hBtnFont = NULL;
	m_hBtnFontBold = NULL;
	m_bInMonitor = FALSE;
	m_hReadyEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_bWin95UI = (BYTE)::GetVersion() >= 4;
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	VERIFY(::GetVersionEx(&osVersionInfo));
	m_bWin95OS = osVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT;
        m_cleanupList = new CMapPtrToPtr;
	bTraceAwt = FALSE;

	// Awt message pool.
	::InitializeCriticalSection(&m_csAwtMsgPool);

}

CAwtApp::~CAwtApp()
{
    AwtImage::DisposePalette();
    if (m_hBtnFont != NULL) {
        VERIFY(::DeleteObject(m_hBtnFont));
    }
    if (m_hBtnFontBold != NULL) {
        VERIFY(::DeleteObject(m_hBtnFontBold));
    }
    if (m_hReadyEvent != NULL) {
        ::CloseHandle(m_hReadyEvent);
    }

    // Awt brush cache.
    POSITION pos = brushCache.GetStartPosition();
    while (pos != NULL) {
        void *key;
        BrushCacheRec* pRec;
        void *p;
        brushCache.GetNextAssoc(pos, key, p);
        pRec = (BrushCacheRec*)p;
        ::DeleteObject(pRec->m_hBrush);
        delete pRec;
    }
    brushCache.RemoveAll();

    // Awt message pool.
    ::DeleteCriticalSection(&m_csAwtMsgPool);
    while (!m_AwtMsgPool.IsEmpty()) {
        AwtMsg *pMsg = (AwtMsg*)m_AwtMsgPool.RemoveHead();

        VERIFY(::CloseHandle(pMsg->hEvent));
        delete pMsg;
    }

    CAwtApp *pApp = (CAwtApp*)AfxGetApp();
    while (!pApp->m_pCallbackThreadQ->IsEmpty()) {
        CallbackThreadMsg *pMsg = 
            (CallbackThreadMsg *)pApp->m_pCallbackThreadQ->RemoveHead();
        AWT_TRACE(("deleting callback msg %s\n", pMsg->m_pzMethodName));
        delete pMsg;
    }
    delete pApp->m_pCallbackThreadQ;

    ::CloseHandle(pApp->m_hCallbackThreadEvent);
    ::CloseHandle(pApp->m_hCallbackThreadQMutex);

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

/////////////////////////////////////////////////////////////////////////////
// CAwtApp initialization

BOOL CAwtApp::InitInstance()
{
	HDC hDC = ::CreateCompatibleDC(NULL);

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	//Enable3dControls();

	//LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// this variable is used by the app framework which assumes this is not NULL.
	// An app typically has at least one window open so this is justified.  
	// However, this is not true in our case so we create a fake window and
	// have it point to it.
	m_pMainWnd = new CWnd();

#if !defined(NETSCAPE)
	// Check for at least 8-bit color
	if (::GetDeviceCaps(hDC, SIZEPALETTE) > 0 && ::GetDeviceCaps(hDC, SIZEPALETTE) < 256) {
		::MessageBox(NULL, "Sorry, but you must have at least 256 colors for this alpha release of HotJava/NT",
			"Error", MB_APPLMODAL);	
		return FALSE;
	}
	AwtImage::InitializePalette(hDC);
#else
	// Check for at least 8-bit color
	if (::GetDeviceCaps(hDC, SIZEPALETTE) > 0 && ::GetDeviceCaps(hDC, SIZEPALETTE) < 256) {
		::MessageBox(NULL, "Sorry, but you must have at least 256 colors to display java applets",
			"Error", MB_APPLMODAL);	
		return FALSE;
		// The Palette is initialized in CAwtApp::MainLoop()
	}
#endif // NETSCAPE

	// Create the button fonts used by buttons created in AWT
	LOGFONT logfont;
	UINT cyPixelsPerInch = ::GetDeviceCaps(hDC, LOGPIXELSY);

	memset(&logfont, 0, sizeof(logfont));
	logfont.lfHeight = -MulDiv(10, cyPixelsPerInch, 72);
	logfont.lfWeight = FW_NORMAL;
	logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	lstrcpy(logfont.lfFaceName, _T("MS Sans Serif"));
	m_hBtnFont = ::CreateFontIndirect(&logfont);
//m_hBtnFont = NULL;
	if (m_hBtnFont == NULL) {
		TRACE0("Warning: Using system font for button text.\n");
		m_hBtnFontBold = (HFONT)::GetStockObject(SYSTEM_FONT);
	}

	logfont.lfWeight = FW_BOLD;
	if (m_hBtnFont != NULL) {
		m_hBtnFontBold = ::CreateFontIndirect(&logfont);
	}
	if (m_hBtnFontBold == NULL) {
		TRACE0("Warning: Using system font for button text.\n");
		m_hBtnFontBold = (HFONT)::GetStockObject(SYSTEM_FONT);
	}

	// Get DLU for button font
	::SelectObject(hDC, m_hBtnFontBold);
	::GetTextExtentPoint(hDC, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", 52, &m_aveSize);
	::SelectObject(hDC, ::GetStockObject(SYSTEM_FONT));
	
	::DeleteDC(hDC);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAwtApp object

CAwtApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Operations

void CAwtApp::Init(Hsun_awt_win32_MToolkit *pJavaObject, Hjava_lang_Thread *pParentObject) {
	Classjava_lang_Thread *pParent = (Classjava_lang_Thread*)unhand(pParentObject);

	// Start callback thread
	m_hCallbackThreadQMutex = CreateMutex(NULL, FALSE, NULL);
	m_pCallbackThreadQ = new CPtrList();
	m_hCallbackThreadEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void CAwtApp::MainLoop(Hsun_awt_win32_MToolkit *pJavaObject)
{
	MSG msg;
	char *pAwtEnv = getenv("AWT_DEBUG");
	int i;

#ifdef _DEBUG
	printf("DEBUG version of awt.dll\n");
#endif
	//GdiSetBatchLimit(1);
	if (pAwtEnv) {
		printf("AWT_DEBUG=%s\n", pAwtEnv);
		for (i=0; i<(int)strlen(pAwtEnv); i++) {
			if (pAwtEnv[i] == 't') {
				bTraceAwt = TRUE;
				AWT_TRACE(("tracing all awt calls\n"));
			}
		}
	}

	// Create the message queue and then signal ready. 
	::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE);

	m_pJavaObject = pJavaObject;
	m_threadID = GetCurrentThreadId();
#ifdef ENABLE_CALLINGWINDOWS
	CallingWindows[0].msgLoopThread = m_threadID;
	CallingWindows[0].msgLoopMonitor = (int *)obj_monitor(m_pJavaObject);
#endif
        sysAtexit(handleExit);

#ifdef NETSCAPE
	//
	// Call back to the Application to query the default palette via the
	// exported GetApplicationPalette() call
	//
	HPALETTE hPal;
	GetApplicationPalette pfGetAppPalette = (GetApplicationPalette)
						    ::GetProcAddress( ::GetModuleHandle(NULL), 
						    GET_APP_PALETTE_API);

	if( pfGetAppPalette )  {
	    hPal = (*pfGetAppPalette)();
	}
	if( hPal ) {
	    AwtImage::InitializePalette(hPal);
	} else {
	    HDC hDC = ::CreateCompatibleDC(NULL);
	    AwtImage::InitializePalette(hDC);
	    ::DeleteDC(hDC);
	}
#endif // NETSCAPE

	MonitorEnter();
	monitorNotify(obj_monitor(m_pJavaObject));
	//MonitorExit();
	Run();
	MonitorExit();
} 

BOOL CAwtApp::PumpMessage()
{
	ASSERT_VALID(this);

#ifdef _DEBUG
	if (m_nDisablePumpCount != 0)
	{
		TRACE0("Error: CWinThread::PumpMessage called when not permitted.\n");
		ASSERT(FALSE);
	}
#endif

    // Release the AWT lock before blocking in GetMessage(...)
    MonitorExit();
	if (!::GetMessage(&m_msgCur, NULL, NULL, NULL))
	{
        MonitorEnter();
#ifdef _DEBUG
		if (afxTraceFlags & traceAppMsg)
			TRACE0("CWinThread::PumpMessage - Received WM_QUIT.\n");
		m_nDisablePumpCount++; // application must die
			// Note: prevents calling message loop things in 'ExitInstance'
			// will never be decremented
#endif
		return FALSE;
	}
    // Get the AWT lock
    MonitorEnter();

#ifdef _DEBUG
#ifndef MSVC4
	if (afxTraceFlags & traceAppMsg)
		_AfxTraceMsg(_T("PumpMessage"), &m_msgCur);
#endif
#endif

	// process this message
	if (!PreTranslateMessage(&m_msgCur) && (m_msgCur.message != WM_NULL) )
	{
		::TranslateMessage(&m_msgCur);
		::DispatchMessage(&m_msgCur);
	}
	return TRUE;
}

BOOL CAwtApp::OnIdle(LONG lCount) 
{
	MonitorExit();
	CALLINGWINDOWS(0);
	VERIFY(::WaitMessage());

	MonitorEnter();
	return 1;
}

void CAwtApp::CallbackLoop(Hsun_awt_win32_MToolkit *pJavaObject)
{
    CallbackThreadMsg *pMsg;
    CAwtApp *pApp = (CAwtApp*)AfxGetApp();

    EE();
    while (TRUE) {
        ::WaitForSingleObject(pApp->m_hCallbackThreadEvent, INFINITE);

        // check the queue for any items
        while (TRUE) {
            pMsg = NULL;
            ::WaitForSingleObject(pApp->m_hCallbackThreadQMutex, INFINITE);
            if (!pApp->m_pCallbackThreadQ->IsEmpty()) {
                pMsg = (CallbackThreadMsg *)pApp->m_pCallbackThreadQ->RemoveHead();
            }
            ::ReleaseMutex(pApp->m_hCallbackThreadQMutex);

            if (pMsg == NULL) {
                break;
            } else {
                // Copy msg, as pMsg needs to be freed before msg is
                // executed, as it may exit.
                CallbackThreadMsg msg;
                memcpy(&msg, pMsg, sizeof(CallbackThreadMsg));
                delete pMsg;

                AWT_TRACE(("CB %s %s\n", 
                          msg.m_pzMethodName, msg.m_pzMethodSig));
                pApp->DoCallback2(EE(), msg.m_pJavaObject, 
                                  msg.m_pzMethodName, msg.m_pzMethodSig,
                                  msg.m_nParamCount,  msg.p1, msg.p2, 
                                  msg.p3, msg.p4, msg.p5, msg.p6, msg.p7);
            }
        }
    }
}

void CAwtApp::MonitorExit()
{
	monitorExit(obj_monitor(m_pJavaObject));
}

void CAwtApp::MonitorEnter()
{
	monitorEnter(obj_monitor(m_pJavaObject));
}


CRect CAwtApp::ComputeRect(char *pzString, int nAdditionalChars) {
	CRect result(0, 0, 0, 0);
	CSize stringSize(0, 0);

	if (pzString == NULL || strlen(pzString) == 0) {
		pzString = "M";
	}

	stringSize = AwtFontMetrics::StringWidth(CFont::FromHandle(m_hBtnFontBold), pzString);

	// Surround the text with 3 DLU's
	result.right = (6 + nAdditionalChars * 4) * m_aveSize.cx;
	result.right /= 4 * 52;
	result.right += stringSize.cx;
	result.bottom = 6 * m_aveSize.cy;
	result.bottom /= 8;
	result.bottom += stringSize.cy;
	return result;
}

void CAwtApp::CreateBrush(COLORREF color) 
{
    void *p;
    BrushCacheRec *pRec;

    if (brushCache.Lookup((void*)color, p)) {
        pRec = (BrushCacheRec*)p;
        pRec->m_nRefCount++;
        return;
    }
    pRec = new BrushCacheRec;
    pRec->m_nRefCount = 1;
    pRec->m_hBrush = ::CreateSolidBrush(color);
    brushCache[(void*)color] = pRec;
}

HBRUSH CAwtApp::GetBrush(COLORREF color) 
{
    void *p;
    BrushCacheRec *pRec;

    if (brushCache.Lookup((void*)color, p)) {
        pRec = (BrushCacheRec*)p;
        return pRec->m_hBrush;
    }
    ASSERT(FALSE);
    return NULL;
}

void CAwtApp::ReleaseBrush(COLORREF color) 
{
    BrushCacheRec *pRec;
    void *p;

    if (brushCache.Lookup((void*)color, p)) {
        pRec = (BrushCacheRec*)p;
        pRec->m_nRefCount--;
        if (pRec->m_nRefCount == 0) {
            ::DeleteObject(pRec->m_hBrush);
            delete pRec;
            VERIFY(brushCache.RemoveKey((void*)color));
        }
        return;
    }
    ASSERT(FALSE);
}

HICON CAwtApp::GetAwtIcon(void)
{
#ifdef NETSCAPE
    return LoadIcon( "AWT_ICON" );
#else
    HMODULE h = GetModuleHandle("AWT");
    if (h == NULL) {
        h = GetModuleHandle("AWT_G");
    }
    if (h != NULL) {
        return ::LoadIcon(h, "AWT_ICON");
    }
    return NULL;
#endif
}

BOOL CAwtApp::IsServerThread() {
	return ::GetCurrentThreadId() == m_threadID;
}


AwtMsg *CAwtApp::GetAwtMsgPool()
{
	AwtMsg *pMsg;

	::EnterCriticalSection(&m_csAwtMsgPool);
	if (m_AwtMsgPool.IsEmpty()) {
		pMsg = new AwtMsg;
		pMsg->hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	} else {
		pMsg = (AwtMsg*)m_AwtMsgPool.RemoveHead();
	}
	::LeaveCriticalSection(&m_csAwtMsgPool);
	return pMsg;
}

void CAwtApp::PutAwtMsgPool(AwtMsg *pMsg)
{
	::EnterCriticalSection(&m_csAwtMsgPool);
	m_AwtMsgPool.AddHead(pMsg);
	::LeaveCriticalSection(&m_csAwtMsgPool);
}

BOOL CAwtApp::PostServerQ(UINT uMsg, WPARAM wParam, LPARAM lParam) {
//printf("post([%x], %x, %x)", ::GetCurrentThreadId(), uMsg, lParam);
	return ::PostThreadMessage(m_threadID, uMsg, wParam, lParam);
}
	
void CAwtApp::ShowWnd(HWND hWnd, UINT nFlags, BOOL bBringToTop)
{
	PostAwtMsg(AWT_SHOWWND, 0, (long)hWnd, nFlags, bBringToTop);
}

void CAwtApp::PostAwtMsg(UINT nClass, UINT nMsg, long p1, long p2, long p3, 
						long p4, long p5, long p6, long p7)
{
//printf("post(%x, %x)", nClass, nMsg);
	if (IsServerThread()) {
		//MonitorExit();
		HandleMsg(nClass, nMsg, p1, p2, p3, p4, p5, p6, p7);
		//MonitorEnter();
	} else {
		AwtMsg *pMsg = GetAwtMsgPool();

		pMsg->nClass = nClass;	
		pMsg->nMsg = nMsg;
		pMsg->p1 = p1;
		pMsg->p2 = p2;
		pMsg->p3 = p3;
		pMsg->p4 = p4;
		pMsg->p5 = p5;
		pMsg->p6 = p6;
		pMsg->p7 = p7;
		pMsg->bSignalEvent = FALSE;
		VERIFY(PostServerQ(nClass, 0, (LPARAM)pMsg));
	}
}

long CAwtApp::SendAwtMsg(UINT nClass, UINT nMsg, long p1, long p2, long p3, 
						long p4, long p5, long p6, long p7, long p8)
{
//printf("send(%x, %x)", nClass, nMsg);
	long nResult;

	if (IsServerThread()) {
		//MonitorExit();
		nResult = HandleMsg(nClass, nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
		//MonitorEnter();
	} else {
		AwtMsg *pMsg = GetAwtMsgPool();

		pMsg->nClass = nClass;	
		VERIFY(::ResetEvent(pMsg->hEvent));
		pMsg->nMsg = nMsg;
		pMsg->p1 = p1;
		pMsg->p2 = p2;
		pMsg->p3 = p3;
		pMsg->p4 = p4;
		pMsg->p5 = p5;
		pMsg->p6 = p6;
		pMsg->p7 = p7;
		pMsg->p8 = p8;
		pMsg->bSignalEvent = TRUE;
		VERIFY(PostServerQ(nClass, 0, (LPARAM)pMsg));
		MonitorExit(); 
#if 1
	AWT_TRACE(("SendAwtMsg - Entering wait loop\n"));
    while( ::WaitForSingleObject(pMsg->hEvent, 250) == WAIT_TIMEOUT ) {
        MSG msg;
        ::PeekMessage( &msg, NULL, NULL, NULL, PM_NOREMOVE );
        AWT_TRACE(("SendAwtMsg - wait timeout...\n"));
    }
	AWT_TRACE(("SendAwtMsg - Finished wait loop\n"));
#else
		::WaitForSingleObject(pMsg->hEvent, INFINITE);
#endif
		MonitorEnter(); 
		nResult = pMsg->nResult;

		PutAwtMsgPool(pMsg);
	}
	return nResult;
}

BOOL CAwtApp::PreTranslateMessage(MSG* pMsg) 
{
#ifdef ENABLE_CALLINGWINDOWS
	CALLINGWINDOWS(pMsg->hwnd);
	CallingWindows[0].lastMsg = *pMsg;
#endif

	if (pMsg->hwnd == NULL && pMsg->message >= AWT_CLASS_FIRST && pMsg->message <= AWT_CLASS_LAST) {
		AwtMsg *pAwtMsg = (AwtMsg*)pMsg->lParam;
		long nResult;

		nResult = HandleMsg(pMsg->message, pAwtMsg->nMsg, pAwtMsg->p1, pAwtMsg->p2, pAwtMsg->p3, 
					   pAwtMsg->p4, pAwtMsg->p5, pAwtMsg->p6, pAwtMsg->p7, pAwtMsg->p8);
		if (pAwtMsg->bSignalEvent) {
			pAwtMsg->nResult = nResult;
			VERIFY(::SetEvent((HANDLE)pAwtMsg->hEvent));
		} else {
			PutAwtMsgPool(pAwtMsg);
		}		
		return TRUE;
	}
	CALLINGWINDOWS(pMsg->hwnd);
        if (pMsg->message == WM_AWT_DISPATCH_EVENT &&
            pMsg->wParam == FILTER_SIGNATURE) {
            // Dispatch the message posted by DispatchInputEvent().
            MSG* pEventMsg = (MSG*)(pMsg->lParam);
            ::TranslateMessage(pEventMsg);
            ::DispatchMessage(pEventMsg);
            free(pEventMsg);		// allocated in DispatchInputEvent()
            return TRUE;
        }
        if (::HandleInputEvent(pMsg) == TRUE) {
            return TRUE;
        }
	return CWinApp::PreTranslateMessage(pMsg);
}


long CAwtApp::HandleMsg(UINT nClass, UINT nMsg, long p1, long p2, long p3, 
	long p4, long p5, long p6, long p7, long p8)
{
	long nResult;

	switch (nClass) {
		case AWT_SHOWWND:
			CALLINGWINDOWS((HWND)p1);
			::ShowWindow((HWND)p1, (BOOL)p2);
			if ((BOOL)p3) {
				:: BringWindowToTop((HWND)p1);
			}
			break;
	        case AWT_CLEANUP:
			cleanup();
			break;
		case AWT_COMP:
			nResult = AwtComp::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_FRAME:
			nResult = MainFrame::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_GRAPHICS:
			nResult = AwtGraphics::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_MENU:
			nResult = AwtMenu::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_WINDOW:
			nResult = AwtWindow::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_BUTTON:
			nResult = AwtButton::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_LIST:
			nResult = AwtList::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_OPTIONMENU:
			nResult = AwtOptionMenu::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_SCROLLBAR:
			nResult = AwtScrollbar::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_EDIT:
			nResult = AwtEdit::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_LABEL:
			nResult = AwtLabel::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_FILEDIALOG:
			nResult = AwtFileDialog::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		case AWT_FONT:
			nResult = AwtFont::HandleMsg(nMsg, p1, p2, p3, p4, p5, p6, p7, p8);
			break;
		default:
			return FALSE;
	}
	return nResult;
}

// Callback Thread 

void CAwtApp::DoCallback(void *pJavaObject, char *pzMethodName, char *pzMethodSig,
	int nParamCount, long p1, long p2, long p3, long p4, long p5, long p6, long p7)
{
	CallbackThreadMsg *pMsg;
	::WaitForSingleObject(m_hCallbackThreadQMutex, INFINITE);
	if (pzMethodName != 0 && !m_pCallbackThreadQ->IsEmpty()) {
	    if (strcmp(pzMethodName, "handleRepaint") == 0) {
		pMsg = (CallbackThreadMsg *) m_pCallbackThreadQ->GetTail();
		if (pMsg && pMsg->m_pJavaObject == pJavaObject
		    && strcmp(pMsg->m_pzMethodName, pzMethodName) == 0) {
		    if (p1 < pMsg->p1) {
			pMsg->p3 += pMsg->p1 - p1;
			pMsg->p1 = p1;
		    }
		    if (p2 < pMsg->p2) {
			pMsg->p4 += pMsg->p2 - p2;
			pMsg->p2 = p2;
		    }
		    if (p1 + p3 > pMsg->p1 + pMsg->p3) {
			pMsg->p3 = p1 + p3 - pMsg->p1;
		    }
		    if (p2 + p4 > pMsg->p2 + pMsg->p4) {
			pMsg->p4 = p2 + p4 - pMsg->p2;
		    }
		    ::ReleaseMutex(m_hCallbackThreadQMutex);
		    AWT_TRACE(("collapsed repaint\n"));
		    return;
		}
	    } else if (strcmp(pzMethodName, "handleMouseMoved") == 0
		       || strcmp(pzMethodName, "handleMouseDrag") == 0) {
		pMsg = (CallbackThreadMsg *) m_pCallbackThreadQ->GetTail();
		if (pMsg && pMsg->m_pJavaObject == pJavaObject
		    && strcmp(pMsg->m_pzMethodName, pzMethodName) == 0) {
		    if (pMsg->p5 == p5) {
			pMsg->p1 = p1;
			pMsg->p2 = p2;
			pMsg->p3 = p3;
			pMsg->p4 = p4;
			::ReleaseMutex(m_hCallbackThreadQMutex);
			AWT_TRACE(("collapsed mouse event\n"));
			return;
		    }
		}
	    }
	}
	// need to eventually use a pool
	pMsg = new CallbackThreadMsg();

	pMsg->m_pJavaObject = pJavaObject;
	pMsg->m_pzMethodName = pzMethodName;
	pMsg->m_pzMethodSig = pzMethodSig;
	pMsg->m_nParamCount = nParamCount;
	pMsg->p1 = p1;
	pMsg->p2 = p2;
	pMsg->p3 = p3;
	pMsg->p4 = p4;
	pMsg->p5 = p5;
	pMsg->p6 = p6;
        pMsg->p7 = p7;

	m_pCallbackThreadQ->AddTail(pMsg);
	::SetEvent((HANDLE)m_hCallbackThreadEvent);
    ::ReleaseMutex(m_hCallbackThreadQMutex);
}

void CAwtApp::DoCallback2(ExecEnv *ee, void *pJavaObject, char *pzMethodName, char *pzMethodSig,
	int nParamCount, long p1, long p2, long p3, long p4, long p5, long p6, long p7)
{
	Hjava_lang_Object *pPeer = (Hjava_lang_Object*)
		(unhand((Hjava_awt_Component *)pJavaObject)->peer);
		
	if (pPeer == NULL) {	// this might be true during create of some awt objects
		return;
	}
	switch (nParamCount) {	// is there a better way?
	case 0:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig);
		break;
	case 1:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1);
		break;
	case 2:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1, p2);
		break;
	case 3:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1, p2, p3);
		break;
	case 4:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1, p2, p3, p4);
		break;
	case 5:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1, p2, p3, p4, p5);
		break;
	case 6:
		execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1, p2, p3, p4, p5, p6);
		break;
        case 7:
        	execute_java_dynamic_method(ee, pPeer, pzMethodName, pzMethodSig,
			p1, p2, p3, p4, p5, p6, p7);
		break;
	default:
		VERIFY(FALSE);
	}
	GdiFlush();
	if (exceptionOccurred(EE())) {
		exceptionDescribe(EE());
		exceptionClear(EE());
	}
}

void CAwtApp::cleanupListAdd(CObject *pObj)
{
    m_cleanupList->SetAt(pObj, pObj);
}

void CAwtApp::cleanupListRemove(CObject *pObj)
{
    if (m_cleanupList != NULL) {
        m_cleanupList->RemoveKey(pObj);
    }
}

void CAwtApp::cleanup()
{
    AwtComp::Cleanup();
    AwtFont::Cleanup();
    AwtGraphics::Cleanup();

    POSITION pos = m_cleanupList->GetStartPosition();
    while (pos != NULL) {
        CObject *pObj;
        void *key, *p;
        m_cleanupList->GetNextAssoc(pos, key, p);
        pObj = (CObject*)p;
        if (pObj->IsKindOf(RUNTIME_CLASS(AwtMenu))) {
            delete ((AwtMenu*)pObj);
        } else if (pObj->IsKindOf(RUNTIME_CLASS(AwtWindow))) {
            delete ((AwtWindow*)pObj);
        } else {
            delete pObj;
        }
    }
    m_cleanupList->RemoveAll();
    delete m_cleanupList;
    m_cleanupList = NULL;
}

int CAwtApp::ExitInstance() 
{
    // Delete any outstanding resources.
    if (m_cleanupList != NULL) {
        SendAwtMsg(AWT_CLEANUP, 0);
    }

    // free MFC memory leaks
    if (m_pszExeName != NULL) {
        free((void*)m_pszExeName); 
        m_pszExeName = NULL;
    }
    if (m_pszAppName != NULL) {
        free((void*)m_pszAppName); 
        m_pszAppName = NULL;
    }
    if (m_pszHelpFilePath != NULL) {
        free((void*)m_pszHelpFilePath);
        m_pszHelpFilePath = NULL;
    }
    if (m_pszProfileName != NULL) {
        free((void*)m_pszProfileName); 
        m_pszProfileName = NULL;
    }

    delete m_pMainWnd;
    return CWinApp::ExitInstance();
}
