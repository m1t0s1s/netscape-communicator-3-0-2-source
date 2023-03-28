#include "awt_palette.h"
#include "awt_toolkit.h"
#include "awt_component.h"
#include "awt_image.h"

#include "prlink.h" // PR_LoadStaticLibrary() 


// All java header files are extern "C"
extern "C" {
#include "java_awt_Event.h"
#include "java_awt_image_ColorModel.h"

#include "sun_awt_windows_WToolkit.h"
#include "sun_awt_windows_WComponentPeer.h"
};

//
// Definitions of int64 constants 
//
#ifndef HAVE_LONG_LONG

#ifdef IS_LITTLE_ENDIAN
int64 ll_zero_const = { 0, 0 };
int64 ll_one_const  = { 1, 0 };
#else
int64 ll_zero_const = { 0, 0 };
int64 ll_one_const  = { 0, 1 };
#endif

#endif  /* ! HAVE_LONG_LONG */

//
// Maximum delay in milliseconds before the Awt Event Queue is 
// flushed, and all the events are delivered to java
//
#define MAX_AWT_EVENT_DELAY     100

//
// Private Windows message used to force method calls to be made on the
// "Main Gui thread"...
//
#define WM_AWT_CALLMETHOD   (WM_USER+1)

AwtToolkit *AwtToolkit::theToolkit = NULL;
HDC         AwtToolkit::m_hDC      = NULL;

AwtWindow* AwtToolkit::m_HoldMouse = 0;
UINT AwtToolkit::m_TimerId = 0;

HINSTANCE AwtToolkit::m_hDllInstance = 0;

#ifdef DEBUG_CONSOLE
AwtDebug* AwtToolkit::m_pDebug = 0;
#endif

#if !defined(_WIN32)

extern "C" PRStaticLinkTable FAR awt_nodl_tab[];

/*
** This routine returns the nodl_table containing ALL java native methods.
** This table is used by NSPR when looking up symbols in the DLL.
**
** Since Win16 GetProcAddress() is NOT case-sensitive, it is necessary to
** use the nodl table to maintain the case-sensitive java native method names...
*/
extern "C" PRStaticLinkTable * CALLBACK __export __loadds NODL_TABLE(void)
{
    return awt_nodl_tab;
}

//
//
// Dll entry point. Keep the dll instance
//
//
int CALLBACK LibMain( HINSTANCE hModule, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    AwtToolkit::m_hDllInstance = hModule;
    return TRUE;
}

extern "C" BOOL CALLBACK __loadds WEP(BOOL fSystemExit)
{
    // Delete any remaining AwtObjects
    AwtObject::DeleteAllObjects();

    AwtCachedBrush::DeleteCache();
    AwtCachedPen::DeleteCache();
    AwtPalette::DeleteCache();

    // Delete the AwtToolkit object
    AwtToolkit::theToolkit->Dispose();

    return TRUE;
}


#else /* WIN32 */

//
//
// Dll entry point. Keep the dll instance
//
//
BOOL APIENTRY DllMain(  HINSTANCE hModule, 
                        DWORD reason, 
                        LPVOID lpReserved )
{
    switch( reason ) {
        case DLL_PROCESS_ATTACH:
			AwtToolkit::m_hDllInstance = hModule;
        case DLL_THREAD_ATTACH:
    
        case DLL_THREAD_DETACH:
    
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif // _WIN32

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
AwtToolkit::AwtToolkit() : AwtObject(NULL)
{
    WNDCLASS wc;

#if !defined(_WIN32) && defined(DEBUG)
	__asm{ int 3}
#endif
    //
    // Remove from the chain of allocated AwtObjects...
    // The AwtToolkit instance is global and MUST have a longer 
    // lifetime than any other AwtObject.
    //
    // Since the AwtToolkit is the FIRST object to be created, 
    // the liveChain does not need to be locked...
    //
    m_link.Remove();

    //
    // Store the thread ID of the current thread.  
    // The AwtToolkit MUST be created on the "main GUI thread" of 
    // the application.
    //
    m_GuiThreadId  = PR_GetCurrentThreadID();

    m_bShutdown    = FALSE;
    m_eventInQueue = NULL;
    m_eventTimeout = NULL;

    wc.style            = 0;
    wc.lpfnWndProc      = AwtToolkit::WindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ::GetModuleHandle(__awt_dllName);
    wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "AwtToolkitClass";
    
    ::RegisterClass(&wc);

    //
    // Create the Awt Dispatch Window
    //
    m_hAwtDispatchWnd = ::CreateWindow("AwtToolkitClass",
                                       "AwtDispatchWnd",
                                       WS_DISABLED,
                                       -50, -50,
                                       10, 10,
                                       NULL,
                                       NULL,
                                       ::GetModuleHandle(__awt_dllName),
                                       NULL);

    ASSERT( ::IsWindow(m_hAwtDispatchWnd) );

#ifdef DEBUG_CONSOLE
    //AwtToolkit::m_pDebug = new AwtDebug();
#endif

}

#ifdef _WIN32
#define GET_APP_PALETTE_API "GET_APPLICATION_PALETTE"
#else
#define GET_APP_PALETTE_API "_GET_APPLICATION_PALETTE"
#endif

typedef HPALETTE (*GetApplicationPalette)(void);

AwtPalette *AwtToolkit::GetImagePalette(void)
{
   AwtPalette *pAwtPalette=NULL;
   HWND hWnd = ::GetDesktopWindow();
   HDC  hDC  = ::GetDC(hWnd);
   pAwtPalette = AwtPalette::Create(hDC);
   VERIFY( ::ReleaseDC(hWnd, hDC) );
   return pAwtPalette;
}


AwtPalette *AwtToolkit::GetToolkitDefaultPalette(void)
{
    HPALETTE hPal = (HPALETTE)NULL;
    AwtPalette *pAwtPalette=NULL;
    GetApplicationPalette pfGetAppPalette;

    //
    // Look for the exported entry point in the EXE which returns the
    // correct palette...
    //
    // NOTE:
    //    To locate an entry point in an EXE in Win16, NULL must be passed
    //    as the Task Handle...
    //
    pfGetAppPalette = (GetApplicationPalette)::GetProcAddress( 
#ifdef _WIN32
            ::GetModuleHandle(NULL), 
#else
            NULL,
#endif
            GET_APP_PALETTE_API);

    if( pfGetAppPalette ) {
        hPal = (*pfGetAppPalette)();
    }

    if( hPal ) {
        pAwtPalette = AwtPalette::Create(hPal);
    } 
    
    if(pAwtPalette == NULL) {
        if(m_hDC == NULL) {
            HWND hWnd = ::GetDesktopWindow();

            m_hDC=::GetDC(hWnd);
            pAwtPalette = AwtPalette::Create(m_hDC);
            VERIFY( ::ReleaseDC(hWnd, m_hDC) );
        } else {
            pAwtPalette = AwtPalette::Create(m_hDC);
        }
    }
   return pAwtPalette;
}


AwtToolkit::~AwtToolkit()
{
    ASSERT( ::IsWindow(m_hAwtDispatchWnd) );

    //
    // Destroy the Awt Dispatch Window
    //    
    ::DestroyWindow(m_hAwtDispatchWnd);
    m_hAwtDispatchWnd = NULL;

    ::UnregisterClass("AwtToolkitClass", ::GetModuleHandle(__awt_dllName));

    AwtToolkit::theToolkit = NULL;

#ifdef DEBUG_CONSOLE
    //delete AwtToolkit::m_pDebug;
#endif

}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtToolkit::CreateToolkit(void)
{
    if (AwtToolkit::theToolkit == NULL) {
        AwtToolkit::theToolkit = new AwtToolkit();
        return TRUE;
    }
            
    return FALSE;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtToolkit::CallMethod(AwtMethodInfo *info)
{
    return FALSE;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtToolkit::SetAwtColorPalette(AwtPalette* pAwtPal)
{
    m_pAwtImagePalette = pAwtPal;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
HPALETTE AwtToolkit::GetDefaultPalette(void)
{
   return NULL;
}



//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtToolkit::CallAwtMethod(AwtMethodInfo *info)
{
    if (! IsGuiThread()) {
        ASSERT( ::IsWindow(m_hAwtDispatchWnd) );

        return ::SendMessage(m_hAwtDispatchWnd, WM_AWT_CALLMETHOD, 0, 
                             (LPARAM)info);
    }

    return info->Invoke();
}


//-------------------------------------------------------------------------
//
// Call an event handler in the java Peer object. (ie. handleXXX(...))
//
//-------------------------------------------------------------------------
void AwtToolkit::RaiseJavaEvent(JHandle *obj, 
                                AwtEventDescription::EventId id,
                                int64 timestamp, 
                                MSG *pMsg, 
                                long arg1, long arg2, long arg3, long arg4,
                                long arg5)

{
    JavaEventInfo *pInfo;

    pInfo = JavaEventInfo::Create();
    if (pInfo != NULL) {
        pInfo->Initialize(obj, id,
                          timestamp, pMsg,
                          arg1, arg2, arg3, arg4, arg5);

        theToolkit->PostEvent(pInfo);
    }
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtToolkit::FlushEventQueue(void)
{
#ifdef _WIN32
    ::SetEvent(m_eventInQueue);
    ::SetEvent(m_eventTimeout);
#else
#pragma message("AwtToolkit::FlushEventQueue(...) unimplemented")
#endif
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtToolkit::RemoveEventsForObject(JHandle *obj)
{
    AwtCList *qp;
    JavaEventInfo *pInfo;

    // Lock the event queue...
    m_eventMutex.Lock();

    if (!m_eventQueue.IsEmpty()) {
        qp = m_eventQueue.next;
        while (qp != &m_eventQueue) {
            pInfo = (JavaEventInfo *)JAVA_EVENT_PTR(qp);

            qp = qp->next;
            if (pInfo->m_object == obj) {
                //
                // Remove the event from the event queue and place it
                // back on the free chain
                //
                pInfo->Delete();
            }
        }
    }

    // Unlock the event queue...
    m_eventMutex.Unlock();
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtToolkit::PostEvent(JavaEventInfo *pNewEvent)
{
    BOOL bEventCombined;
    JavaEventInfo *pInfo;
    int id;

    //
    // If the event queue is no longer running do not post the
    // event
    if (m_bShutdown) {
        return;
    }


    // Lock the event object...
    m_eventMutex.Lock();

    //
    // First try to combine the event with an already pending event.
    //
    // An event may be combined if an event mangler exists for its
    // event type...
    //
    bEventCombined = FALSE;
    id = pNewEvent->m_eventId;
    if (!m_eventQueue.IsEmpty() && (id < AwtEventDescription::MaxEvent)) {
        AwtCList *qp;
        EventMangler *p;

         //
         // If a mangler exists, then walk through the pending events
         // trying to combine...
         //
        p = AwtEventDescription::m_AwtEventMap[id].m_mangler;
        if (p != NULL) {

            qp = m_eventQueue.next;
            while (qp != &m_eventQueue) {
                pInfo = (JavaEventInfo *)JAVA_EVENT_PTR(qp);
                if (pInfo->m_eventId == id) {
                    if (p->Combine(pInfo, pNewEvent)) {
                        bEventCombined = TRUE;
                        break;
                    }
                }
                qp = qp->next;
            }
        }
    }

    //
    // If the event could not be combined with a pending event, then
    // add the event to the event queue..
    //
    if (!bEventCombined) {
        // Add the event to the event queue
        pNewEvent->AddToQueue(m_eventQueue);

        //
        // Notify the callback thread that an event is waiting to
        // be dispatched...
        //
#ifdef _WIN32
        ::SetEvent(m_eventInQueue);

        if (pNewEvent->DispatchEventNow()) {
            ::SetEvent(m_eventTimeout);
        }
#endif
    } 
    //
    // The event was combined with an existing event.  So free the new
    // event now...
    //
    else {
        pNewEvent->Delete();
    }

    // Unlock the event object...
    m_eventMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Dispatch events to Java.
//
// This method is called by the thread that is responsible for delivering
// Awt events to Java components.
//
//-------------------------------------------------------------------------
void AwtToolkit::CallbackLoop(void)
{
    JavaEventInfo *pInfo;

    //
    // Initialize the events...
    //
#if defined(_WIN32)
    m_eventInQueue = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    m_eventTimeout = ::CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

    CallbackLoop(&m_bShutdown);

    //
    // The AwtCallback thread is terminating...
    //     1. Free the event objects created on entry
    //     2. Move any pending events onto the free chain.  Then clean up
    //        the free chain.
    //
#if defined(_WIN32)
    ::CloseHandle(m_eventInQueue);
    ::CloseHandle(m_eventTimeout);
#endif
    m_eventInQueue = m_eventTimeout = NULL;

    m_eventMutex.Lock();

    while (!m_eventQueue.IsEmpty()) {
        pInfo = (JavaEventInfo *)JAVA_EVENT_PTR( m_eventQueue.next );

        // Place the event back on the free chain...
        pInfo->Delete();
    }

    JavaEventInfo::DeleteFreeChain();
    m_eventMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Callback loop which delivers Awt events to java cmponents...
//
//-------------------------------------------------------------------------
void AwtToolkit::CallbackLoop(BOOL* pExitCodition)
{
    JavaEventInfo *pInfo;
    DWORD dwLastMS, dwNowMS;

#if !defined(_WIN32)
    int64 sleepTime;
#endif

    dwLastMS = 0;
    dwNowMS  = 0;

    while (!*pExitCodition) {
        //
        // Wait until there is work to do...
        //
        // The callback thread empties the event queue when the following
        // conditions have been met:
        //     1. At least one event in the queue
        //     2. It has been at least MAX_AWT_EVENT_DELAY milliseconds
        //        since events were last processed.
        //
        // If the m_eventTimeout Event is explicitly signaled, this 
        // indicates that there is an event in the queue that MUST be
        // processed immediately !
        //
#if defined(_WIN32)
        dwLastMS = ::GetTickCount();
        (void)::WaitForSingleObject(m_eventInQueue, INFINITE);
        
        dwNowMS  = ::GetTickCount();
        if( (dwNowMS - dwLastMS) < MAX_AWT_EVENT_DELAY ) {
            (void)::WaitForSingleObject(m_eventTimeout, MAX_AWT_EVENT_DELAY);
        }
#else
#pragma message("AwtToolkit::EventLoop() needs work!!")
        LL_UI2L(sleepTime, 100000L);
        PR_Sleep(sleepTime);
#endif
        //
        // Process ALL events in the queue...
        //
        while (!m_eventQueue.IsEmpty()) {
            // Remove the next event from the queue...
            m_eventMutex.Lock();
            
            pInfo = (JavaEventInfo *)JAVA_EVENT_PTR( m_eventQueue.next );
            pInfo->m_link.Remove();

            m_eventMutex.Unlock();

            //
            // Dispatch the event to Java...
            //
            // While the event is being processed, the event queue is 
            // unlocked.  This allows other threads to post events
            // (and possibly combine events) while the callback thread
            // is running...
            //
            pInfo->Dispatch();
            pInfo->Delete();
#if !defined(_WIN32)
            LL_UI2L(sleepTime, 100L);
            PR_Sleep(sleepTime);
#endif
        }
#if defined(_WIN32)
        ::ResetEvent(m_eventInQueue);
        ::ResetEvent(m_eventTimeout);
#endif
    }
}


#if !defined(_WIN32)
//-------------------------------------------------------------------------
//
// GdiFlush is not available in Windows 3.1.  
// 
//
//-------------------------------------------------------------------------
BOOL GdiFlush(void)
{
    return TRUE;
}

#endif



//-------------------------------------------------------------------------
//
// AwtToolkit Window Procedure.  This is used to call methods of AwtObject
// on the "main GUI thread"...
//
//-------------------------------------------------------------------------
LRESULT AWT_CALLBACK AwtToolkit::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, 
                                            LPARAM lParam)
{
    if (msg == WM_AWT_CALLMETHOD) {
        AwtMethodInfo *info = (AwtMethodInfo *)lParam;

        return info->Invoke();
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
UINT AwtToolkit::CreateTimer()
{
    if (!AwtToolkit::m_TimerId)
        AwtToolkit::m_TimerId = ::SetTimer(NULL, 0, 200, AwtToolkit::TimerProc);

    return AwtToolkit::m_TimerId;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtToolkit::DestroyTimer()
{
    if (AwtToolkit::m_TimerId) {
        ::KillTimer(NULL, AwtToolkit::m_TimerId);
        AwtToolkit::m_TimerId = 0;
    }

}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AWT_CALLBACK AwtToolkit::TimerProc(HWND hWnd, UINT msg, UINT event, DWORD time)
{
    AwtToolkit::ProcessMouseTimer();
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtToolkit::ProcessMouseTimer()
{
    
    if (AwtToolkit::m_HoldMouse && ::IsWindow(AwtToolkit::m_HoldMouse->GetWindowHandle())) {
        POINT mp;

        // get mouse position
        ::GetCursorPos(&mp);

        if (::WindowFromPoint(mp) != AwtToolkit::m_HoldMouse->GetWindowHandle()) {
            int64 time = PR_NowMS(); // time in milliseconds

            ::ScreenToClient(AwtToolkit::m_HoldMouse->GetWindowHandle(), &mp);

            AwtToolkit::RaiseJavaEvent(AwtToolkit::m_HoldMouse->GetPeer(), 
                                        AwtEventDescription::MouseExit,
                                        time, 
                                        NULL,
                                        mp.x,
                                        mp.y);

            // we are out of this window and of any awt window, destroy timer
            AwtToolkit::DestroyTimer();
            AwtToolkit::m_HoldMouse = 0;
        }

        //AwtToolkit::m_HoldMouse->Unlock();
    }
    else {
        AwtToolkit::DestroyTimer();
        AwtToolkit::m_HoldMouse = 0;
    }
}


//-------------------------------------------------------------------------
//
// Native method implementations for netscape.awt.WToolkit
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WToolkit_init(HWToolkit *self, Hjava_lang_Thread *)
{
    CHECK_OBJECT(self);

    AwtToolkit::CreateToolkit();
}



void
sun_awt_windows_WToolkit_callbackLoop(HWToolkit *self)
{
    CHECK_OBJECT(self);

    AwtToolkit::theToolkit->CallbackLoop();
}


void
sun_awt_windows_WToolkit_eventLoop(HWToolkit *self)
{
    MSG msg;
    CHECK_OBJECT(self);

    //
    // Notify the peer object that the AwtToolkit object is initialized
    //
    monitorEnter(obj_monitor(self));
    monitorNotify(obj_monitor(self));
    monitorExit(obj_monitor(self));
#if defined(_WIN32)
    while (::GetMessage(&msg, NULL, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage (&msg);
    }
#else
	/*
	** Delegate Windows input event queueing to another message loop
	** Hopefully, the browser is pumping messages, and that is what will
	** trigger entry into the various AWT WndProcs.
	*/
#endif

}



Hjava_awt_image_ColorModel *
sun_awt_windows_WToolkit_makeColorModel(HWToolkit *self)
{
    //CHECK_OBJECT_AND_RETURN(self, NULL);

    Hjava_awt_image_ColorModel *model = NULL;


    RGBQUAD *pRGBQuad = (RGBQUAD*)malloc(256*sizeof(RGBQUAD));

    // Initialize the palette colors to BLACK (ie. RGB(0,0,0))
    AwtPalette *pAwtPalette = AwtToolkit::GetToolkitDefaultPalette();
    pAwtPalette->AddRef();

    if(pAwtPalette != NULL) 
    {
        pAwtPalette->GetRGBQUADS(pRGBQuad); 
    }
    model = AwtImage::getColorModel(pRGBQuad);
    free(pRGBQuad);
    pAwtPalette->Release();

    return model;
}


void
sun_awt_windows_WToolkit_sync(HWToolkit *self)
{
    CHECK_OBJECT(self);

    VERIFY( ::GdiFlush() );
}



long 
sun_awt_windows_WToolkit_getScreenHeight(HWToolkit *self)
{
    HDC hDCDesktop;
    HWND hWndDesktop;
    long height;

    hWndDesktop = ::GetDesktopWindow();
    hDCDesktop  = ::GetDC(hWndDesktop);

    height = ::GetDeviceCaps(hDCDesktop, VERTRES);

    VERIFY( ::ReleaseDC(hWndDesktop, hDCDesktop) );

    return height;
}

long 
sun_awt_windows_WToolkit_getScreenWidth(HWToolkit *self)
{
    HDC hDCDesktop;
    HWND hWndDesktop;
    long width;

    hWndDesktop = ::GetDesktopWindow();
    hDCDesktop  = ::GetDC(hWndDesktop);

    width = ::GetDeviceCaps(hDCDesktop, HORZRES);

    VERIFY( ::ReleaseDC(hWndDesktop, hDCDesktop) );

    return width;
}


long 
sun_awt_windows_WToolkit_getScreenResolution(HWToolkit *self)
{
    HDC hDCDesktop;
    HWND hWndDesktop;
    long resolution;

    hWndDesktop = ::GetDesktopWindow();
    hDCDesktop  = ::GetDC(hWndDesktop);

    resolution  = ::GetDeviceCaps(hDCDesktop, LOGPIXELSX);

    VERIFY( ::ReleaseDC(hWndDesktop, hDCDesktop) );

    return resolution;
}


void 
sun_awt_windows_WToolkit_notImplemented(HWToolkit *self)
{
    CHECK_OBJECT(self);

    BREAK_TO_DEBUGGER;
}


}; // end of extern "C"

#if !defined(_WIN32)
//
// new() and delete() customizations so that we don't pull in the
// rtl's _malloc/_free routines in WIN16.
//

void  __far *operator new( size_t size )
{
	return malloc(size);
}

void operator delete( void __far *ptr )
{
	free(ptr);
}

#endif

