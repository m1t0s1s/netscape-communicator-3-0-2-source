/*
 * @(#)awtapp.h	1.25 95/12/04
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
// awtapp.h : main header file for the AWT application
//

#ifndef _AWTAPP_H_
#define _AWTAPP_H_


#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "awt.h"



// If defined this symbol enables the debug version of new
//#define ENABLE_DEBUG_NEW

// This macros should be used just before calling a Windows procedure.
// It records the location of the call.  This information is meant to be
// used in the case where you're looking at a stack trace that starts with WndProc 
// and you suspect that it was triggered by making a Windows call.  
// The CALLINGWINDOWS information gives you a hint.
// To disable this feature, comment out the following line.
//#define ENABLE_CALLINGWINDOWS
#ifdef ENABLE_CALLINGWINDOWS
#define CALLINGWINDOWS_QUEUESIZE 5
typedef struct {
    char *pzFile;	// file from which the Windows call was made.
    int nLine;		// line number in file from which the Windows 
			//   call was made.
    HWND hWnd;		// the target of the Windows call.
    MSG	lastMsg;	// last windows message received by 
			//   PreTranslateMsg in main message loop.
    DWORD msgLoopThread;// the thread id of the WServer thread.
    int *msgLoopMonitor;// the WServer thread's monitor.
} CallingWindowsStruct;
extern CallingWindowsStruct CallingWindows[];
#define CALLINGWINDOWS(paramHWnd) {\
	memcpy(&CallingWindows[1], &CallingWindows[0], \
	       sizeof(CallingWindowsStruct) * (CALLINGWINDOWS_QUEUESIZE - 1));\
	CallingWindows[0].pzFile = THIS_FILE;\
	CallingWindows[0].nLine = __LINE__;\
	CallingWindows[0].hWnd = paramHWnd;\
	memset(&CallingWindows[0].lastMsg, 0, sizeof(MSG));}
#else
#define CALLINGWINDOWS(paramHWnd) ((void)0)
#endif

// If defined, a thread is created to call the client's
// handleEvent methods.  Otherwise, the main WServer thread
// will call the methods directly.
#define CALLBACKTHREAD

/////////////////////////////////////////////////////////////////////////////
// CAwtApp:
// See awt.cpp for the implementation of this class
//

// Awt message.
typedef struct {
    UINT nClass;
    UINT nMsg;
    HANDLE hEvent;

    // Tells the receipient to signal 'hEvent' after the message is processed.
    BOOL bSignalEvent;		

    long p1, p2, p3, p4, p5, p6, p7, p8;
    long nResult;
} AwtMsg;

// Callback Thread message.
typedef struct {
    void *m_pJavaObject;
    char *m_pzMethodName;
    char *m_pzMethodSig;
    int m_nParamCount;
    long p1, p2, p3, p4, p5, p6, p7;
} CallbackThreadMsg;


// Brush cache data structure
typedef struct {
    int m_nRefCount;
    HBRUSH m_hBrush;
} BrushCacheRec;

class CAwtApp : public CWinApp
{
public:
// Constants
    enum {
        AWT_BUTTON = WM_USER+1,
        AWT_COMP,
        AWT_LIST,
        AWT_OPTIONMENU,
        AWT_SCROLLBAR,
        AWT_BITMAPBUTTON,
        AWT_FONT,
        AWT_FRAME,
        AWT_LABEL,
        AWT_FILEDIALOG,
        AWT_GRAPHICS,
        AWT_EDIT,
        AWT_MSGDIALOG,
        AWT_WINDOW,
        AWT_MENU,
        AWT_SHOWWND,
        AWT_CLEANUP,
        AWT_CLASS_FIRST = AWT_BUTTON,
        AWT_CLASS_LAST = AWT_CLEANUP
    };

#define WM_AWT_DISPATCH_EVENT WM_USER+100

    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

// Constructors/destructors
    CAwtApp();
    ~CAwtApp();

// Operations
    // Initializes the CAwtApp object.
    void Init(Hsun_awt_win32_MToolkit *pJavaObject,
              Hjava_lang_Thread *pParent);

    // This method retrieves and dispatches Windows messages.
    // This method does not return until the application exits.
    // This method must be called before any of the other AWT
    // classes can be used.
    void MainLoop(Hsun_awt_win32_MToolkit *pJavaObject);

    // This method processes all the callback messages to java objects.
    void CallbackLoop(Hsun_awt_win32_MToolkit *pJavaObject);

    // Given a string, returns the size appropriate for a button
    // or label such that the button or label is just big enough
    // to contain the string.  pzString can be NULL.
    // The width of the rectangle will be increased by the width of
    // an average character * nAdditionalChars.
    CRect ComputeRect(char *pzString, int nAdditionalChars = 0);

    // These members maintain a brush cache for painting the control
    // backgrounds in order to minimize the number of active brushes.
    // When a control is created, it calls CreateBrush.  When it
    // changes the brush color, it should call ReleaseBrush and
    // then CreateBrush.  When the control is deleted, it should
    // call ReleaseBrush.
    // When a control is created, it should call CreateBrush to
    // make sure the brush exists in the brush cache.
    void CreateBrush(COLORREF color);
    HBRUSH GetBrush(COLORREF color);
    void ReleaseBrush(COLORREF color);

    // Acquires the monitor on the server's Java object.
    void MonitorEnter();

    // Releases the monitor on the server's Java object.
    void MonitorExit(); 

    // If the current thread is the event thread, calls SetWindowPos;
    // Otherwise asynchronously sends a message to the event thread 
    // asking it to call SetWindowPos.	
    void SetWndPos(HWND hWnd, int nX, int nY, int nWidth, int nHeight, 
                   UINT nFlags);

    // If the current thread is the event thread, calls ShowWindow;
    // Otherwise asynchronously sends a message to the event thread 
    // asking it to call ShowWindow.
    // If 'bBringToTop', the window is moved in front of all other 
    // sibling windows.
    void ShowWnd(HWND hWnd, UINT nFlags, BOOL bBringToTop = FALSE);

    // Returns an icon handle to the AWT's default icon.
    HICON GetAwtIcon(void);
	
    // Returns TRUE if current thread is the server thread.
    BOOL IsServerThread();
	
// Awt message routines.
    // Posts the message to the server's work queue.
    BOOL PostServerQ(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Posts an awt message to the server thread.  Returns immediately 
    // and does not wait for a reply. If the current thread is the server 
    // thread, the message is executed immediately.
    void PostAwtMsg(UINT nClass, UINT nMsg, long p1 = NULL, long p2 = NULL, 
                    long p3 = NULL, long p4 = NULL, long p5 = NULL, 
                    long p6 = NULL, long p7 = NULL);

    // Sends an awt message to the server thread and waits for a reply.  
    // If the current thread is the server thread, the message is executed 
    // immediately.
    long SendAwtMsg(UINT nClass, UINT nMsg, long p1 = NULL, long p2 = NULL, 
                    long p3 = NULL, long p4 = NULL, long p5 = NULL, 
                    long p6 = NULL, long p7 = NULL, long p8 = NULL);

    // Route the message to the appropriate class.  Returns the result 
    // from executing the message.
    long HandleMsg(UINT nClass, UINT nMsg, long p1 = NULL, long p2 = NULL, 
                   long p3 = NULL, long p4 = NULL, long p5 = NULL, 
                   long p6 = NULL, long p7 = NULL, long p8 = NULL);

    // Sends a message to the callback thread to execute a java method.
    void DoCallback(void *pJavaObject, char *pzMethodName, 
                    char *pzMethodSig, int nParamCount = 0, 
                    long p1 = NULL, long p2 = NULL, long p3 = NULL, 
                    long p4 = NULL, long p5 = NULL, long p6 = NULL,
                    long p7 = NULL);

    // Executes a java method.  This one actually does the work while 
    // DoCallback merely sends a message to the callback thread to do it.
    void DoCallback2(ExecEnv *ee, void *pJavaObject, char *pzMethodName, 
                     char *pzMethodSig, int nParamCount = 0, 
                     long p1 = NULL, long p2 = NULL, long p3 = NULL, 
                     long p4 = NULL, long p5 = NULL, long p6 = NULL,
                     long p7 = NULL);

    // Retrieves an awt message from the pool.  Creates a new one if 
    // the pool is empty.
    AwtMsg *GetAwtMsgPool();

    // Returns 'pMsg' back into the awt message pool.
    void PutAwtMsgPool(AwtMsg *pMsg);

    // Add an allocated resource to the cleanup list.
    void cleanupListAdd(CObject *pObj);

    // Remove an allocated resource from the cleanup list.
    void cleanupListRemove(CObject *pObj);

    // Cleanup everything on the cleanup list.
    void cleanup();

// Public Variables
    // This flag enables tracing of the awt calls.  The call and the 
    // parameters are displayed on stdout.
    BOOL bTraceAwt;

    // The WServer's Java object.
    Hsun_awt_win32_MToolkit *m_pJavaObject;

    // This thread handle is set by the thread that calls MainLoop.
    DWORD m_threadID;

    // Font used by buttons without the focus.  Never NULL.
    HFONT m_hBtnFont;

    // Font used by button with the focus.  Never NULL.
    HFONT m_hBtnFontBold;

    // TRUE iff program is running the Windows 95 UI shell.
    BOOL m_bWin95UI;

    // TRUE iff program is running on Windows 95
    BOOL m_bWin95OS;

// Callback thread data
    // The callback thread waits on this event for more work.
    HANDLE m_hCallbackThreadEvent;

    /// Callback thread's identifier.
    LPDWORD m_IDCallbackThread;

    // Callback thread's message queue.
    CPtrList *m_pCallbackThreadQ;

    // Callback thread's message queue mutex.
    HANDLE m_hCallbackThreadQMutex;

// Private Variables
private:
    // The brush cache.
    CMapPtrToPtr brushCache;

    // An event that is signalled once all the initialization
    // has been done.
    HANDLE m_hReadyEvent;

    // Average size of a character in the normal button font.
    CSize m_aveSize;

    // EnterMonitor sets it and ExitMonitor resets it.
    BOOL m_bInMonitor;

    // Pool of awt messages.  If at the time the pool is accessed there
    // are no messages, a new one is created.  There is currently no attempt
    // to limit the size of the pool.
    CPtrList m_AwtMsgPool;

    // Protects m_pAwtMsgPool;
    CRITICAL_SECTION m_csAwtMsgPool;

    // List of CObjects to free on exit.
    CMapPtrToPtr *m_cleanupList;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAwtApp)
public:
    virtual BOOL InitInstance();
    virtual BOOL PumpMessage();
    virtual BOOL OnIdle(LONG lCount);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CAwtApp)
    afx_msg LRESULT OnAwtDispatchEvent(WPARAM wParam, LPARAM lMsgPtr);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // _AWTAPP_H_
