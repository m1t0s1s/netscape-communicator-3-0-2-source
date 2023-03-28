
#if !defined(AWT_DEBUG_H)
#define AWT_DEBUG_H

#include "awt_object.h"

#include <windows.h>
#include "awt_resource.h"


#define MENU_LOG            100
#define MENU_CLEAR          101
#define MENU_MSGS           102


#define LOG_OBJECT_TO_FILE          0x1
#define LOG_MESSAGE_TO_FILE         0x2
#define LOG_CONSOLE_TO_FILE         0x4


class AwtDebug {

public:
                                AwtDebug(BOOL bOpenConsole = TRUE);
                                ~AwtDebug();

    //
    // file logging definitions.
    // use them to open log files
    //
                                // an object log file will log *only* the information passed
                                // to Log(AwtObject*) which in turn call PrintStatus 
                                // on the object passed in. Implement PrintStutus
    void                        SetObjectLogFile(const char* pszFileName);
                                // a message log file logs windows messages and java strings
    void                        SetMessageLogFile(const char* pszFileName);
                                // a log file is the same as the console window
                                // every log information goes there
    void                        SetLogFile(const char* pszFileName);
    

    //
    // filter on the logs file, according to the flag passed in
    // the object will write that "class" of messages on the proper file 
    //
    // LOG_OBJECT_TO_FILE      if setted log on the object is allowed
    // LOG_MESSAGE_TO_FILE     if setted log on the message is allowed
    // LOG_CONSOLE_TO_FILE     if setted log on the console is allowed
    //                                
                                // set the flag
    void                        SetLogOnFileFlags(int nFilesState); // m_bLogOnFileFlags = nFilesState
                                // modify the flag (add or remove the specified flag)
    void                        ModifyLogOnFileFlag(int nFileState, BOOL bAdd = TRUE); 


    //
    // utility function to start/stop logging and to activate deactivate windows message hook
    //
                                // activate or deactivate the trace on the console window
    void                        Trace(BOOL bTrace);
                                // enable or disable the windows messages hook 
    void                        LogMessages(BOOL bHookWin);
                                // register a window handle with the hook procedure so that 
                                // we monitor the incoming windows messages
    int                         RegisterWindowHandle(HWND hWnd);
    HWND                        UnregisterWindowHandle(HWND hWnd);
                                // clear the console window
    void                        ClearLog();


    // 
    // log functions
    // 
                                // string log
    void                        Log(char* pszString);
                                // java string log
    void                        Log(Hjava_lang_String* string);
                                // object log (call PrintStatus on pObject)
    void                        Log(AwtObject* pObject); 
    void                        Log(AwtCachedResource* pObject); 
                                // print the java calls stack
    void                        Log(JHandle *pJavaPeer);
                                // same as wsprintf, print a formatted string up to 256 chars
    void                        Log(const char* lpszControlString, ...);


                                // hook and window procs
    static LRESULT CALLBACK     GetHookProc(int code, WPARAM wParan, LPARAM lParam);
    static LRESULT CALLBACK     CallHookProc(int code, WPARAM wParan, LPARAM lParam);
    static LRESULT CALLBACK     DebugProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

                                // window messages handling
    LPARAM                      OnSize(int width, int height);
    void                        OnClose();

                                // check if interested in the hooked message
    void                        CheckMessage(HWND wnd, LPSTR msg, WPARAM wParam, LPARAM lParam);

    //
    // Syncronization methods to lock and unlock the object.
    //
    inline  void                Lock    (void) { monitorEnter((uint32)this);  }
    inline  void                Unlock  (void) { monitorExit ((uint32)this);  }


protected:

    char *                      AddCRtoBuffer(char *buffer) ;
    AwtDebug*                   RemoveHook();

    static UINT                 GetMessageString(UINT message, LPSTR pszMessage, size_t strSize);

protected:

    HWND m_hWnd;
    HWND m_hMLEdit;

    BOOL m_bTraceOn;
    BOOL m_bMsgHookOn;

    int m_bLogOnFileFlags;

    HFILE m_hObjectLog;
    HFILE m_hMessageLog;
    HFILE m_hLog;

    HWND m_hWndHookedCollection[10];
    int m_nHandlesHooked;

    static HHOOK m_hCallHook;
    static HHOOK m_hGetHook;

    static AwtDebug* m_pDebugObjsCollection[10]; // just 10 max debug objs
    static int m_nObjs;
};


#endif
