#ifndef AWT_TOOLKIT_H      
#define AWT_TOOLKIT_H

#include "awt_defs.h"
#include "awt_clist.h"
#include "awt_object.h"

// for mouse enter/exit events
#include "awt_window.h"


// All java header files are extern "C"
extern "C" {
#include "java_lang_Thread.h"
#include "java_awt_Event.h"
};

#include "awt_event.h"

#ifdef DEBUG_CONSOLE
#include "awt_debug.h"
#endif


//
// Foreward declarations
//
class AwtPalette;
class AwtEventInfo;

DECLARE_SUN_AWT_WINDOWS_PEER(WToolkit);

//
// AwtToolkit represents the global object responsible for calling methods
// and dispatching events for both AwtObjects and Java objects...
//
class AwtToolkit : public AwtObject
{
public:
    static BOOL             CreateToolkit(void);

    virtual BOOL            CallMethod(AwtMethodInfo *info);


    //
    // Install a global color palette to be used for all Awt image
    // operations.
    //
            void            SetAwtColorPalette(AwtPalette* pAwtPal);

    //
    // Invoke a method on an AwtObject.  If the call is made in
    // the current thread, then the method will be invoked directly.
    // Otherwise, a Dispatch event will be send to the AwtDispatchWnd
    // and the method will be invoked on the GuiThread.
    //
    // The bAsync flag indicates whether the calling thread should block
    // until the method call has been performed.
    //
            BOOL            CallAwtMethod(AwtMethodInfo *info);

    //
    // Call a java event handler.
    //
    static  void            RaiseJavaEvent(JHandle *obj, 
                                           AwtEventDescription::EventId id,
                                           int64 timestamp, 
                                           MSG *pMsg, 
                                           long arg1=0, long arg2=0, 
                                           long arg3=0, long arg4=0,
                                           long arg5=0);

    //
    // Return whether the current thread is the application's
    // Gui thread.  
    //
#if defined(_WIN32)
            BOOL            IsGuiThread(void) { 
                                return m_GuiThreadId == (DWORD)PR_GetCurrentThreadID(); 
                            }
#else
            BOOL            IsGuiThread(void) { return TRUE; }  // REMIND: rjp - fix this
#endif

            DWORD           GetGuiThreadId(void) {
                                return m_GuiThreadId;
                            }

            void            PostEvent(JavaEventInfo *pInfo);
//            void            PostEvent(JHandle *obj, int id,
//                                      int64 time, 
//                                      long p1=0, long p2=0, long p3=0, 
//                                      long p4=0, long p5=0);

            void            FlushEventQueue(void);

            void            RemoveEventsForObject(JHandle *obj);

            void            CallbackLoop(void);
            void            CallbackLoop(BOOL* pExitCodition);

            void            ExitCallbackLoop(void) { m_bShutdown = TRUE; }

            void            Cleanup(void);
static      HPALETTE        GetDefaultPalette(void);
static      AwtPalette     *GetToolkitDefaultPalette(void);
static      AwtPalette     *GetImagePalette(void);

static      UINT            CreateTimer();
static      void            DestroyTimer();

static      void AWT_CALLBACK TimerProc(HWND hWnd, UINT msg, UINT event, DWORD time);

static      void            ProcessMouseTimer();   


protected:
    //
    // AwtToolkit instances can only be created or destroyed via the 
    // CreateToolkit() and Dispose() methods...
    //
                            AwtToolkit();
    virtual                 ~AwtToolkit();



private:
    //
    // Window procedure used to receive Awt messages...
    //
    static LRESULT AWT_CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM WParam, 
                                           LPARAM lParam);

    //
    // Data members:
    //
protected:
    // Handle of the window used to receive Awt dispatch messages.
    HWND        m_hAwtDispatchWnd;

    // Color palette used for all image operations.
    AwtPalette* m_pAwtImagePalette;

    // Thread Id of the "main" Gui thread.
    DWORD       m_GuiThreadId;


    // Flag to notify the callback thread to terminate
    BOOL        m_bShutdown;

    // Events used by the callback thread...
    HANDLE      m_eventInQueue;
    HANDLE      m_eventTimeout;

    // Mutex used to guard the queue of pending Awt events
    AwtMutex    m_eventMutex;

    // Awt event queue...
    AwtCList    m_eventQueue;

public:
    // Global AwtToolkit Instance
    static AwtToolkit* theToolkit;
    static HDC m_hDC;

    // global information for mouse enter/exit events
    static AwtWindow* m_HoldMouse;
    static UINT m_TimerId;


public:
	static HINSTANCE m_hDllInstance;

    // define the debug console
#ifdef DEBUG_CONSOLE
    static AwtDebug* m_pDebug;
#endif

};



#endif  // AWT_TOOLKIT_H
