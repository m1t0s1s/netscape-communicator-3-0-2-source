
#include "awt_debug.h"
#include "awt_toolkit.h"
#include "awt_window.h"

extern "C" {
#include "interpreter.h"
#include "javaString.h"

#ifdef DEBUG_PEER
#include "sun_awt_windows_WDebugPeer.h"
#endif
}

HHOOK AwtDebug::m_hCallHook = 0;
HHOOK AwtDebug::m_hGetHook = 0;
AwtDebug* AwtDebug::m_pDebugObjsCollection[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int AwtDebug::m_nObjs = 0;


AwtDebug::AwtDebug(BOOL bOpenConsole)
{
    m_bTraceOn = TRUE;
    m_bMsgHookOn = FALSE;

    m_bLogOnFileFlags = 0; // no log on files

    m_hObjectLog = 0;
    m_hMessageLog = 0;
    m_hLog = 0;

    m_hWnd = 0;
    m_hMLEdit = 0;

    int i;
    for (i = 0; i < 10; i++)
        m_hWndHookedCollection[i] = 0;
    m_nHandlesHooked = 0;

    if (bOpenConsole) {
        //
        // create debug console
        //
        WNDCLASS wc;

        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = AwtDebug::DebugProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 4;
        wc.hInstance        = ::GetModuleHandle(NULL);
        wc.hIcon            = NULL;
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE+1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "AwtDebugConsoleClass";
    
        if (::RegisterClass(&wc)) {
    
            //
            // Create the new window
            //
            m_hWnd = ::CreateWindow("AwtDebugConsoleClass",
                                    "Debug Console",
                                    WS_OVERLAPPEDWINDOW,
                                    0,
                                    0,
                                    300,
                                    300,
                                    NULL,
                                    NULL,
                                    ::GetModuleHandle(NULL),
                                    NULL);

            m_hMLEdit = ::CreateWindow("EDIT",
                                    "",
                                    WS_CHILD | WS_VSCROLL | WS_BORDER | WS_VISIBLE |
                                        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                    0,
                                    0,
                                    0,
                                    0,
                                    m_hWnd,
                                    NULL,
                                    ::GetModuleHandle(NULL),
                                    NULL);

            // create menu
            HMENU hMenu = ::CreateMenu();
            HMENU hSubMenu = ::CreatePopupMenu();
            ::AppendMenu(hSubMenu, MF_STRING, MENU_LOG, "Log Status");
            ::AppendMenu(hSubMenu, MF_STRING, MENU_MSGS, "Log Messages");

            ::AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "Options");
            ::AppendMenu(hMenu, MF_STRING, MENU_CLEAR, "Clear!");

            ::SetMenu(m_hWnd, hMenu);

            ::CheckMenuItem(hSubMenu, MENU_LOG, MF_CHECKED);


            ::SetWindowLong(m_hWnd, 0, (LONG)this);
            ::UpdateWindow(m_hWnd);
            ::ShowWindow(m_hWnd, SW_SHOW);
        }
    }

    // add this to the list of debug objs (used by the hook proc)
    if (AwtDebug::m_nObjs < 10)
        AwtDebug::m_pDebugObjsCollection[AwtDebug::m_nObjs++] = this;
}


AwtDebug::~AwtDebug()
{
    if (m_hWnd)
        ::DestroyWindow(m_hWnd);

    if (m_hObjectLog)
        ::_lclose(m_hObjectLog);

    if (m_hMessageLog)
        ::_lclose(m_hMessageLog);

    if (m_hLog)
        ::_lclose(m_hLog);

    // delete object from collection and compact collection
    RemoveHook();

}


//
// open the log file as specified
//
void AwtDebug::SetObjectLogFile(const char* pszFileName)
{
    OFSTRUCT ofs;
    if (m_hObjectLog ) {
        ::_lclose(m_hObjectLog );
        m_hObjectLog = 0;
    }

    if (pszFileName) {
        m_hObjectLog = ::OpenFile(pszFileName, &ofs, OF_CREATE | OF_READWRITE);
        if (m_hObjectLog != HFILE_ERROR)
            m_bLogOnFileFlags |= LOG_OBJECT_TO_FILE;
    }
}


void AwtDebug::SetMessageLogFile(const char* pszFileName)
{
    OFSTRUCT ofs;
    if (m_hMessageLog) {
        ::_lclose(m_hMessageLog);
        m_hMessageLog = 0;
    }

    if (pszFileName) {
        m_hMessageLog = ::OpenFile(pszFileName, &ofs, OF_CREATE | OF_READWRITE);
        if (m_hMessageLog != HFILE_ERROR)
            m_bLogOnFileFlags |= LOG_MESSAGE_TO_FILE;
    }
}


void AwtDebug::SetLogFile(const char* pszFileName)
{
    OFSTRUCT ofs;
    if (m_hLog) {
        ::_lclose(m_hLog);
        m_hLog = 0;
    }

    if (pszFileName) {
        m_hLog = ::OpenFile(pszFileName, &ofs, OF_CREATE | OF_READWRITE);
        if (m_hLog != HFILE_ERROR)
            m_bLogOnFileFlags |= LOG_CONSOLE_TO_FILE;
    }
}


//
// set log files flags. Using the proper flag you may selectively choose 
// when a log message is recorded to a file
//
void AwtDebug::SetLogOnFileFlags(int nFilesState)
{
    m_bLogOnFileFlags = nFilesState;
}


void AwtDebug::ModifyLogOnFileFlag(int nFileState, BOOL bAdd)
{
    if (bAdd)
        m_bLogOnFileFlags |= nFileState;
    else
        m_bLogOnFileFlags -= nFileState;
}

    
// 
// set trace on or off
//
void AwtDebug::Trace(BOOL bTrace)
{
    m_bTraceOn = bTrace;
}


//
// clear window
//
void AwtDebug::ClearLog()
{
    ::SendMessage(m_hMLEdit, WM_SETTEXT, 0, (LPARAM)"");
}


//
// log routines
//

// log string
void AwtDebug::Log(char* pszString) 
{ 
    //Lock();

    if (m_hLog && (m_bLogOnFileFlags & LOG_CONSOLE_TO_FILE))
        _lwrite(m_hLog, pszString, lstrlen(pszString));

    if (m_bTraceOn && m_hWnd) {

#ifdef _WIN32
        ::SendMessage(m_hWnd, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
#else
        ::SendMessage(m_hWnd, EM_SETSEL, (WPARAM)1, (LPARAM)-1L)
#endif
        ::SendMessage(m_hMLEdit, EM_REPLACESEL, 0, (LPARAM)pszString);
    }

    //Unlock();
}


// java string log
void AwtDebug::Log(Hjava_lang_String* string)
{
    char* pszString;
    int32_t len;

    if (string) {
        len = javaStringLength(string);
        pszString = (char *)malloc(len+1);
        if (pszString) {

            javaString2CString(string, pszString, len+1);

            char* pszNewString = AddCRtoBuffer(pszString);

            //Lock();

            // log into the message log file
            if (m_hMessageLog && (m_bLogOnFileFlags & LOG_MESSAGE_TO_FILE))
                ::_lwrite(m_hMessageLog, pszString, strlen(pszString)); 
            Log(pszNewString); 

            //Unlock();

            free(pszNewString);
        }
    }
    else
        Log("");
}


// log object status
void AwtDebug::Log(AwtObject* pObject) 
{ 
#ifdef DEBUG_CONSOLE
    char* pszString = pObject->PrintStatus();
#else
    char* pszString = NULL;
#endif

    //Lock();
    if (pszString) {

        // Log into object log file
        if (m_hObjectLog && (m_bLogOnFileFlags & LOG_OBJECT_TO_FILE))
            _lwrite(m_hObjectLog, pszString, lstrlen(pszString));

        Log(pszString); 

        delete[] pszString;
    }

    //Unlock();

}


void AwtDebug::Log(AwtCachedResource* pObject) 
{ 
#ifdef DEBUG_CONSOLE
    char* pszString = pObject->PrintStatus();
#else
    char* pszString = NULL;
#endif

    //Lock();

    if (pszString) {

        // Log into object log file
        if (m_hObjectLog && (m_bLogOnFileFlags & LOG_OBJECT_TO_FILE))
            _lwrite(m_hObjectLog, pszString, lstrlen(pszString));

        Log(pszString); 

        delete[] pszString;
    }

    //Unlock();

}


// log java call stack
void AwtDebug::Log(JHandle *pJavaPeer)
{
    char* pszString;
    int32_t len;

    Hjava_lang_String* string = 
        (Hjava_lang_String*)execute_java_dynamic_method(NULL, 
                                                        pJavaPeer, 
                                                        "printStack", 
                                                        "()Ljava/lang/String;");
    if (string) {
        len = javaStringLength(string);
        pszString = (char *)malloc(len+1);
        if (pszString) {

            javaString2CString(string, pszString, len+1);

            char* pszNewString = AddCRtoBuffer(pszString);

            //Lock();

            // log into message log file
            if (m_hMessageLog && (m_bLogOnFileFlags & LOG_MESSAGE_TO_FILE))
                ::_lwrite(m_hMessageLog, pszString, strlen(pszString)); 

            Log(pszNewString); 

            //Unlock();

            free(pszNewString);
        }
    }
    else
        Log("printStack failed\r\n");
}


// same as wsprintf, print a formatted string up to 256 chars
void AwtDebug::Log(const char* lpszControlString, ...)
{
    char buffer[256];
    va_list args = va_start(args, lpszControlString);
    vsprintf(buffer, lpszControlString, args);

    //Lock();

    // log into message log file
    if (m_hMessageLog && (m_bLogOnFileFlags & LOG_MESSAGE_TO_FILE))
        ::_lwrite(m_hMessageLog, buffer, 256); 

    Log(buffer);

    //Unlock();
}


//
// create a hook on the message loop to minitor incoming messages
//
void AwtDebug::LogMessages(BOOL bHookWin)
{
    if (bHookWin && !AwtDebug::m_hCallHook) {
        AwtDebug::m_hCallHook = 
            ::SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)AwtDebug::CallHookProc,
                                ::GetModuleHandle(NULL), 
#ifdef _WIN32
                                AwtToolkit::theToolkit->GetGuiThreadId()
#else
                                ::GetCurrentTask()
#endif
                                );
        AwtDebug::m_hGetHook = 
            ::SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)AwtDebug::GetHookProc,
                                ::GetModuleHandle(NULL), 
#ifdef _WIN32
                                AwtToolkit::theToolkit->GetGuiThreadId()
#else
                                ::GetCurrentTask()
#endif
                                );
    }
    
    if (!bHookWin) {
        RemoveHook();
    }

    m_bMsgHookOn = bHookWin;
}


//
// Message hook proc
//
LRESULT CALLBACK AwtDebug::CallHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION) {
        int i = 0;
        char pszMessage[128];
        CWPSTRUCT* pMsg = (CWPSTRUCT*)lParam;

        // if hwnd is 0; ignore the message
        if (pMsg->hwnd) {

            if (AwtDebug::GetMessageString(pMsg->message, pszMessage, 128))
                wsprintf(pszMessage, "0x%X", pMsg->message);
        
            // forward the message to any created object
            for (i = 0; i < AwtDebug::m_nObjs; i++) {
                if (AwtDebug::m_pDebugObjsCollection[i])
                    AwtDebug::m_pDebugObjsCollection[i]->CheckMessage(pMsg->hwnd, 
                                                                        pszMessage, 
                                                                        pMsg->wParam,
                                                                        pMsg->lParam);
            }
        }
    }

    return ::CallNextHookEx(AwtDebug::m_hCallHook, code, wParam, lParam);
}


//
// Message hook proc
//
LRESULT CALLBACK AwtDebug::GetHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION) {
        int i = 0;
        char pszMessage[128];
        MSG* pMsg = (MSG*)lParam;

        // if hwnd is 0; ignore the message
        if (pMsg->hwnd) {

            if (AwtDebug::GetMessageString(pMsg->message, pszMessage, 128))
                wsprintf(pszMessage, "0x%X", pMsg->message);
        
            // forward the message to any created object
            for (i = 0; i < AwtDebug::m_nObjs; i++) {
                if (AwtDebug::m_pDebugObjsCollection[i])
                    AwtDebug::m_pDebugObjsCollection[i]->CheckMessage(pMsg->hwnd, 
                                                                        pszMessage, 
                                                                        pMsg->wParam,
                                                                        pMsg->lParam);
            }
        }
    }

    return ::CallNextHookEx(AwtDebug::m_hGetHook, code, wParam, lParam);
}


// 
// check if interested in the incoming message
// this routine should implement some kind of filter
//
void AwtDebug::CheckMessage(HWND wnd, LPSTR msg, WPARAM wParam, LPARAM lParam)
{
    char buffer[256];
    int i;

    if (m_bMsgHookOn && m_nHandlesHooked) {
        
        // check if the incoming window handle is registered with this
        // debug instance
        for (i = 0; i < m_nHandlesHooked; i++) {
            if (m_hWndHookedCollection[i] == wnd)
                break;
        }

        if (i == m_nHandlesHooked)
            return;
    
#ifdef DEBUG_CONSOLE
        AwtWindow* objPtr = AwtWindow::GetThisPtr(wnd);
#else
        AwtWindow* objPtr = 0;
#endif
        char * pszObjName = (objPtr) ? (char*)objPtr + 4 : "NUL";
        wsprintf(buffer, "%s on window 0x%X[%s] (wParam=0x%X; lParam=0x%X)\r\n", 
                                                    msg, 
                                                    wnd, 
                                                    pszObjName,
                                                    wParam, 
                                                    lParam);
        
        //wsprintf(buffer, "%s on window 0x%X (wParam=0x%X; lParam=0x%X)\r\n", msg, wnd, wParam, lParam);
        //Lock();

        if (m_hMessageLog && (m_bLogOnFileFlags & LOG_MESSAGE_TO_FILE))
            ::_lwrite(m_hMessageLog, buffer, strlen(buffer)); 

        Log(buffer);

        //Unlock();

    }
}


//
// register a window handle to be monitored from the hook procedure
//
int AwtDebug::RegisterWindowHandle(HWND hWnd)
{
    if (m_nHandlesHooked < 10) {
        m_hWndHookedCollection[m_nHandlesHooked++] = hWnd;
        return m_nHandlesHooked;
    }
    else
        return -1;
}


//
// unregister a window handle 
//
HWND AwtDebug::UnregisterWindowHandle(HWND hWnd)
{
    if (m_nHandlesHooked) {
        for (int i = 0; i < m_nHandlesHooked; i++) {
            if (m_hWndHookedCollection[i] == hWnd) {
                for (int j = i + 1; j < m_nHandlesHooked; j++)
                    m_hWndHookedCollection[j - 1] = m_hWndHookedCollection[j];

                m_hWndHookedCollection[--m_nHandlesHooked] = 0;

                return hWnd;
            }
        }
    }
 
    return 0;
}


//
// Debug Console window proc
//
LRESULT CALLBACK AwtDebug::DebugProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AwtDebug* thisObj = (AwtDebug*)::GetWindowLong(hWnd, 0);
    switch(msg) {

    case WM_SIZE:

        if (thisObj)
            return thisObj->OnSize(LOWORD(lParam), HIWORD(lParam));

        break;

    case WM_CLOSE:

        if (thisObj)
            thisObj->OnClose();

        break;

    case WM_COMMAND:
        if (!lParam && !HIWORD(wParam) && thisObj) {
            switch (LOWORD(wParam)) {
                case MENU_LOG: 
                {
                    HMENU hMenu = ::GetMenu(hWnd);
                    HMENU hSubMenu = ::GetSubMenu(hMenu, 0);

                    UINT uState = ::GetMenuState(hSubMenu, MENU_LOG, 0);
                    // invert selection
                    uState = (uState & MF_CHECKED) ? MF_UNCHECKED : MF_CHECKED;
                    ::CheckMenuItem(hSubMenu, MENU_LOG, uState);

                    thisObj->Trace((uState & MF_CHECKED) ? 1 : 0);
                    break;
                }
                case MENU_MSGS: 
                {
                    HMENU hMenu = ::GetMenu(hWnd);
                    HMENU hSubMenu = ::GetSubMenu(hMenu, 0);

                    UINT uState = ::GetMenuState(hSubMenu, MENU_MSGS, 0);
                    // invert selection
                    uState = (uState & MF_CHECKED) ? MF_UNCHECKED : MF_CHECKED;
                    ::CheckMenuItem(hSubMenu, MENU_MSGS, uState);

                    thisObj->LogMessages((uState & MF_CHECKED) ? 1 : 0);
                    break;
                }
                case MENU_CLEAR:
                    thisObj->ClearLog();
                    break;
            }

            return 0L;
        }
        
        break;

    }

    return ::CallWindowProc(::DefWindowProc, hWnd, msg, wParam, lParam);

}

LPARAM AwtDebug::OnSize(int width, int height)
{
    ::MoveWindow(m_hMLEdit, 0, 0, width, height, FALSE);

    return 0;
}

void AwtDebug::OnClose()
{
    m_hWnd = 0;
}


// -----------------------------------------------------------------------
//
// Returns a copy of null-terminated buffer where each '\n' has been 
// converted to '\r\n'.  
//
// The buffer is realloc()'ed to accomodate the extra '\r' characters.
//
// -----------------------------------------------------------------------
char * AwtDebug::AddCRtoBuffer(char *buffer) 
{
    int nNewlines, nLen;
    char *pStr;
    char *result;

    //
    // Count the # of newlines in buffer
    //
    nNewlines = 0;
    for (nLen=0; buffer[nLen]; nLen++) {
        if (buffer[nLen] == '\n') {
            nNewlines++;
        }
    }

    //
    // If the buffer has no newlines, then return the buffer as-is
    //
    if (nNewlines == 0) {
        return buffer;
    }

    //
    // Realloc the buffer to accomodate the extra '\r' required for
    // each newline...
    //
    result = (char *)realloc(buffer, nLen + nNewlines + 1);


    //
    // Walk back through the buffer adding the '\r' characters 
    // along the way...
    //
    buffer = result + nLen;
    pStr   = result + nLen + nNewlines;

    while (buffer >= result) {
        *pStr-- = *buffer;
        if (*buffer == '\n') {
            *pStr-- = '\r';
        }
        buffer--;
    }

    return result;
}


AwtDebug* AwtDebug::RemoveHook()
{
    AwtDebug* pRet = 0;
    // delete object from collection and compact collection
    for (int i = 0; i < AwtDebug::m_nObjs; i++) {
        if (AwtDebug::m_pDebugObjsCollection[i] == this) {

            for (int j = i + 1; j < AwtDebug::m_nObjs; j++)
                AwtDebug::m_pDebugObjsCollection[j - 1] = AwtDebug::m_pDebugObjsCollection[j];


            AwtDebug::m_pDebugObjsCollection[--AwtDebug::m_nObjs] = 0; // zero last element

            pRet = this;

            break;
        }
    }

    if (AwtDebug::m_hCallHook && !AwtDebug::m_nObjs) {
        // last object, unhook 
        ::UnhookWindowsHookEx(AwtDebug::m_hCallHook);
        AwtDebug::m_hCallHook = 0;
        ::UnhookWindowsHookEx(AwtDebug::m_hGetHook);
        AwtDebug::m_hGetHook = 0;
    }

    return pRet;
}
    

//
// translate a message id into a string
//
UINT AwtDebug::GetMessageString(UINT message, LPSTR pszMessage, size_t strSize)
{
    if (::LoadString(AwtToolkit::m_hDllInstance, message, pszMessage, strSize)) 
        return 0;
    else
        return message;
}


//-------------------------------------------------------------------------
//
// Native methods for sun.awt.windows.WDebugPeer
//
//-------------------------------------------------------------------------
#ifdef DEBUG_PEER

extern "C" {

void 
sun_awt_windows_WDebugPeer_printString(Hsun_awt_windows_WDebugPeer *self,
                                       struct Hjava_lang_String *string)
{
    if (AwtToolkit::m_pDebug)
        AwtToolkit::m_pDebug->Log(string); 
}

};

#endif
