#include "awt_event.h"
#include "awt_window.h"
#include "awt_palette.h"
#include "awt_toolkit.h"
#include "awt_defs.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif
// All java header files are extern "C"
extern "C" {
#include "java_awt_Component.h"
#include "java_awt_Insets.h"
#include "java_awt_Event.h"

#include "sun_awt_windows_WComponentPeer.h"
};



HHOOK AwtWindow::m_hKeyboardHook = (HHOOK)NULL;
UINT  AwtWindow::m_nHookCount    = 0;

ATOM  AwtWindow::thisPtr         = 0;

#if !defined(_WIN32)
ATOM  AwtWindow::thisLow         = 0;
#endif


AwtWindow::AwtWindow(JHandle *peer) : AwtComponent(peer)
{
    m_pMsg = NULL;
    m_hWnd = NULL;
    m_PrevWndProc = NULL;

    if (AwtWindow::thisPtr == 0) {
        AwtWindow::RegisterAtoms();
    }
}


// -----------------------------------------------------------------------
//
// Default hInst parameter to ::CreateWindow  
//
// -----------------------------------------------------------------------
HINSTANCE
AwtWindow::WindowHInst()
{
	return ::GetModuleHandle(__awt_dllName);
}

// -----------------------------------------------------------------------
//
// The AwtComponent is being destroyed.  
//
// Destroy the native window if it still exists.  
//
// -----------------------------------------------------------------------
AwtWindow::~AwtWindow()
{
    if (m_hWnd) {
        Destroy(m_hWnd);
    }
}


// -----------------------------------------------------------------------
//
// Create a Windows widget for an AwtWindow component
//
// This method MUST be executed on the "main gui thread".  Since this
// method is ALWAYS executed on the same thread no locking is necessary.
//
// -----------------------------------------------------------------------
void AwtWindow::CreateWidget(HWND hwndParent)
{
    Hjava_awt_Component *target;

#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread" because it creates a window...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtComponent::CREATE, (long)hwndParent);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }
#endif

    //
    // This method is now being executed on the "main gui thread"
    //
    ASSERT( AwtToolkit::theToolkit->IsGuiThread() );

    target = (Hjava_awt_Component *)
                unhand((HWComponentPeer *)m_pJavaPeer)->target;

    CHECK_NULL( target, "Target is null." );

    //
    // Create the new window
    //
#ifdef _WIN32
    m_hWnd = ::CreateWindowEx(WindowExStyle(),
#else
    m_hWnd = ::CreateWindow(
#endif
							WindowClass(),
                            "",
                            WindowStyle(),
                            (int) unhand(target)->x,
                            (int) unhand(target)->y,
                            (int) unhand(target)->width,
                            (int) unhand(target)->height,
                            hwndParent,
                            NULL,
                            WindowHInst(),
                            NULL);

    //
    // Subclass the window to allow windows events to be intercepted...
    //
    ASSERT( ::IsWindow(m_hWnd) );
    SubclassWindow(TRUE);
}


// -----------------------------------------------------------------------
//
// Destroy a Windows widget for an AwtWindow component
//
// This method MUST be executed on the "main gui thread".  Since this
// method is ALWAYS executed on the same thread no locking is necessary.
//
// -----------------------------------------------------------------------
void AwtWindow::Destroy(HWND hWnd)
{
#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread" because it creates a window...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtComponent::DESTROY, (long)hWnd);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }
#endif

    ASSERT( ::IsWindow(hWnd) );

    ::DestroyWindow(hWnd);
}

    
// -----------------------------------------------------------------------
//
// This is a callback from java to ask to process a specific event.
// The original message as coming from windows is in JavaEventInfo::m_msg.
//
// -----------------------------------------------------------------------
void AwtWindow::HandleEvent(JavaEventInfo* pInfo, Event *pEvent)
{
    long modifiers = pEvent->modifiers;
    MSG* pMsg = &(pInfo->m_msg);

    if ((pMsg == NULL) || (pMsg->message == 0)) {
        return;
    }

#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread" because it directly calls the
    // Window procedure of the window...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtComponent::HANDLE_EVENT, (long)pInfo, (long)pEvent);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }

    //
    // This method is now being executed on the "main gui thread"
    //
    ASSERT( AwtToolkit::theToolkit->IsGuiThread() );
#endif

    // Reset the keyboard state momentarily as long as the keyboard
    // message is being handled and then reset the keyboard state to the
    // current state. This allows the java client app to override the 
    // modifiers before they are handled by the control.

    BYTE  keyState[256];
    ::GetKeyboardState(keyState);

    // reset modifiers keys if java is asking so
    if (modifiers & java_awt_Event_CTRL_MASK) 
    {
      m_keyState[VK_CONTROL] |= 0x80;
    }
    else
    {
      m_keyState[VK_CONTROL] = 0;
    }

    if(modifiers & java_awt_Event_SHIFT_MASK) 
    {
      m_keyState[VK_SHIFT] |= 0x80;
    }
    else
    {
      m_keyState[VK_SHIFT] = 0;
    } 

    // java_awt_Event_ALT_MASK is used either to signal an alt key
    // and/or the middle mouse button pressed
    if(modifiers & java_awt_Event_ALT_MASK) 
    {
        m_keyState[VK_MBUTTON] |= 0x80;
        m_keyState[VK_MENU] |= 0x80;
    }
    else
    {
        m_keyState[VK_MBUTTON] = 0;
        m_keyState[VK_MENU] = 0;
    }


    if(modifiers & java_awt_Event_META_MASK) 
    {
       m_keyState[VK_RBUTTON] |= 0x80;
    }
    else
    {
       m_keyState[VK_RBUTTON] = 0;
    }

    // in case we are processing a key event we check to see if the ascii code is
    // the same we passed into java. If so, java didn't change the key (maybe the modifiers
    // but we just handled that case) so just keep going and use the value cached in the hook proc.
    // This cover the <ctrl>C problem in win95. When <ctrl>C is received VkKeyScan (in JavaEventInfo::JavaCharToWindows)
    // cannot backup the ascii value to the original virtual-key.
    // (java uses ascii so we call ToAscii in AwtWindow::TranslateToAscii).
    // This is not a problem in NT (since copy still works) but screw 95 up (we don't get WM_CHAR and
    // so WM_COPY).
    // If the value is different java did change the value so call JavaEventInfo::JavaCharToWindows
    // to figure out what character we have.
    switch (pEvent->id) 
    {
        case java_awt_Event_KEY_EVENT:
        case java_awt_Event_KEY_PRESS:
        case java_awt_Event_KEY_RELEASE:
        case java_awt_Event_KEY_ACTION:
        case java_awt_Event_KEY_ACTION_RELEASE:
            if (pInfo->m_arg3 != pEvent->key)
                 pMsg->wParam = JavaEventInfo::JavaCharToWindows(pEvent->key, m_keyState);
            break;
    }
    ::SetKeyboardState(m_keyState);

    //
    // Key-down messages are intercepted by a keyboard-hook when they
    // are removed from the message queue via ::GetMessage().
    //
    if (pMsg->message == WM_KEYDOWN) 
    {
        ::TranslateMessage(pMsg);
        ::DispatchMessage(pMsg);
    } 
    else if (m_PrevWndProc) 
    {
        (void)::CallWindowProc(m_PrevWndProc, pMsg->hwnd, 
                               pMsg->message, pMsg->wParam, pMsg->lParam);
    }
    ::SetKeyboardState(keyState);
}


// -----------------------------------------------------------------------
//
// Show / Hide the window...
//
// -----------------------------------------------------------------------
void AwtWindow::Show(BOOL bState)
{
    ASSERT( ::IsWindow(m_hWnd) );
    if (bState) {
        ::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
    }
    else
        ::ShowWindow(m_hWnd, SW_HIDE);
}


// -----------------------------------------------------------------------
//
// Enable / Disable the window...
//
// -----------------------------------------------------------------------
void AwtWindow::Enable(BOOL bState)
{
    ASSERT( ::IsWindow(m_hWnd) );
    ::EnableWindow(m_hWnd, bState);
}


// -----------------------------------------------------------------------
//
// Set the font for the window...
//
// -----------------------------------------------------------------------
BOOL AwtWindow::SetFont(AwtFont* font)
{
    BOOL bStatus;

    bStatus = AwtComponent::SetFont(font);
    if (font && bStatus) {
#ifdef INTLUNICODE
        ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)font->GetFontForEncoding(this->GetPrimaryEncoding()), 
                      MAKELPARAM(TRUE, 0));
#else	// INTLUNICODE
        ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)font->GetFont(), 
                      MAKELPARAM(TRUE, 0));
#endif	// INTLUNICODE
    }

    return bStatus;
}

// -----------------------------------------------------------------------
//
// Move and resize the window...
//
// -----------------------------------------------------------------------
void AwtWindow::Reshape(long x, long y, long width, long height)
{
    HWND hwnd;

    ASSERT( ::IsWindow(m_hWnd) );
    RECT rcParent, rcWindow;

    // the coordinates are in parent coordinates.
    // so if this is not a top level window convert the coordinates
    //if ((hwnd = ::GetWindow(m_hWnd, GW_OWNER))) {

    //    ::GetWindowRect(hwnd, &rcParent);

    //    x += rcParent.left;
    //    y += rcParent.top;
    //}
    //else if ((hwnd = ::GetParent(m_hWnd))) {
    if (::GetWindow(m_hWnd, GW_OWNER) == NULL) {
        if ((hwnd = ::GetParent(m_hWnd))) {

            ::GetWindowRect(hwnd, &rcParent);
            ::GetClientRect(hwnd, &rcWindow);
            ::MapWindowPoints(hwnd, NULL, (LPPOINT)&rcWindow, 2);

            //if (::GetWindow(m_hWnd, GW_OWNER) == NULL) {
                x -= rcWindow.left - rcParent.left;
                y -= rcWindow.top - rcParent.top;
            //}
        }
        //else if ((hwnd = ::GetWindow(m_hWnd, GW_OWNER))) {
        //    ::GetWindowRect(hwnd, &rcParent);

        //    x += rcParent.left;
        //    y += rcParent.top;
        //}
    }

    VERIFY( ::SetWindowPos(m_hWnd, NULL, x, y, width, height, 
                           SWP_NOZORDER | SWP_NOACTIVATE) );
}


// -----------------------------------------------------------------------
//
// Give this component the input focus...
//
// -----------------------------------------------------------------------
void AwtWindow::GetFocus(void)
{
#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtComponent::GET_FOCUS);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }
#endif

    ASSERT( ::IsWindow(m_hWnd) );
    ::SetFocus(m_hWnd);
}


// -----------------------------------------------------------------------
//
// Return the component that should receive the input focus.
//
// -----------------------------------------------------------------------
AwtComponent* AwtWindow::NextFocus(void)
{
    ASSERT(0);
    return NULL;
}


//-------------------------------------------------------------------------
//
// Return the "windows" window handle
//
//-------------------------------------------------------------------------
HWND AwtWindow::GetWindowHandle(void)
{
    ASSERT( ::IsWindow(m_hWnd) );
    return m_hWnd;
}
    

//-------------------------------------------------------------------------
//
// Return true if it doesn't have the focus. This function is also 
// implemented in windows controls and it always returns false because 
// windows handle the focus correctly there
//
//-------------------------------------------------------------------------
BOOL AwtWindow::WantFocusOnMouseDown()
{
    return ((m_hWnd == ::GetFocus()) ? FALSE : TRUE);
}


//-------------------------------------------------------------------------
//
// Capture the mouse so that mouse events get re-direct to the 
// proper component. This allow MouseDrag to work correctly and
// MouseUp to be sent to the component which received the muose down
//
//-------------------------------------------------------------------------
void AwtWindow::CaptureMouse(AwtButtonType button, UINT metakeys)
{
    // capture the mouse if it was not captured already

    // increment the counter so that if a left/right click
    // occurs while a right/left button is pressed already
    // we don't realese too soon
    if (!m_nCaptured++)
        ::SetCapture(m_hWnd);
}


//-------------------------------------------------------------------------
//
// Release the mouse if the counter reaches zero. We need a counter so
// that if a second button get pressed while the other is pressed already
// we are still able to send MouseUp to the right component
//
//-------------------------------------------------------------------------
BOOL AwtWindow::ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y)
{
    if (m_nCaptured && !--m_nCaptured) 
        ::ReleaseCapture();

    // defproc process the message
    return FALSE; 
}


// -----------------------------------------------------------------------
//
//
// -----------------------------------------------------------------------
MSG * AwtWindow::GetCurrentMessage(void)
{
    ASSERT( AwtToolkit::theToolkit->IsGuiThread() );
    return m_pMsg;
}


// -----------------------------------------------------------------------
//
// Subclass (or remove the subclass from) this component's window
//
// -----------------------------------------------------------------------
void AwtWindow::SubclassWindow(BOOL bState)
{
    ASSERT( ::IsWindow(m_hWnd) );
    
    if (bState) {
        SetThisPtr(m_hWnd, this);

        //
        // Subclass the window
        //
        m_PrevWndProc = (WNDPROC)::SetWindowLong(m_hWnd, GWL_WNDPROC, 
                                                 (LONG)AwtWindow::WndProc);
        ASSERT(m_PrevWndProc);

        //
        // Install the Keyboard hook if this is the first time a window
        // is being subclassed...
        //
        if (m_nHookCount++ == 0) {
            m_hKeyboardHook = 
                ::SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)AwtWindow::KeyHook,
                                   ::GetModuleHandle(__awt_dllName), 
#ifdef _WIN32
                                   AwtToolkit::theToolkit->GetGuiThreadId()
#else
                                   ::GetCurrentTask()
#endif
                                   );
        }
    } 
    else {
        //
        // Remove the Keyboard hook if this is the last window being 
        // "un-subclassed"...
        //
        if (--m_nHookCount == 0) {
            VERIFY( ::UnhookWindowsHookEx(m_hKeyboardHook) );
            m_hKeyboardHook = NULL;
        }

        (void) ::SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG)m_PrevWndProc);
        m_PrevWndProc = NULL;

        RemoveThisPtr(m_hWnd);
    }
}

// -----------------------------------------------------------------------
//
// Register the atom(s) used to attach the "this pointer" of the C++ object
// to the window of the widget...
//
// -----------------------------------------------------------------------
void AwtWindow::RegisterAtoms(void)
{
    if (AwtWindow::thisPtr == 0) {
        AwtWindow::thisPtr = ::GlobalAddAtom("ThisPtr");
#if !defined(_WIN32)
        AwtWindow::thisLow = ::GlobalAddAtom("ThisLow");
#endif
    }
}

// -----------------------------------------------------------------------
//
// Remove all registered atoms
//
// -----------------------------------------------------------------------
void AwtWindow::UnRegisterAtoms(void)
{
    ::GlobalDeleteAtom(AwtWindow::thisPtr);
#if !defined(_WIN32)
    ::GlobalDeleteAtom(AwtWindow::thisLow);
#endif
}


// -----------------------------------------------------------------------
//
// Associate the "this pointer" of an AwtComponent with a window handle
//
// -----------------------------------------------------------------------
void AwtWindow::SetThisPtr(HWND hWnd, AwtWindow* pThis)
{
    ASSERT( ::IsWindow(hWnd) );

#if defined(_WIN32 )
    ::SetProp(hWnd, MAKEINTATOM(AwtWindow::thisPtr), (HANDLE)pThis);
#else
    ::SetProp(hWnd, MAKEINTATOM(AwtWindow::thisPtr), (HANDLE)HIWORD(pThis));
    ::SetProp(hWnd, MAKEINTATOM(AwtWindow::thisLow), (HANDLE)LOWORD(pThis));
#endif
}


// -----------------------------------------------------------------------
//
// Return the "this pointer" of the AwtComponent which corresponds to the
// given window handle.
//
// -----------------------------------------------------------------------
AwtWindow* AwtWindow::GetThisPtr(HWND hWnd)
{
    void* pThis = NULL;

    if (hWnd) {
        ASSERT( ::IsWindow(hWnd) );

#if defined(_WIN32 )
        pThis = (void*)::GetProp(hWnd, MAKEINTATOM(AwtWindow::thisPtr));
#else
        WORD thisHi, thisLo;

        thisHi = (WORD) ::GetProp(hWnd, MAKEINTATOM(AwtWindow::thisPtr));
        thisLo = (WORD) ::GetProp(hWnd, MAKEINTATOM(AwtWindow::thisLow));

        pThis = MAKELP(thisHi, thisLo);
#endif
    }

    return (AwtWindow *)pThis;
}


// -----------------------------------------------------------------------
//
// Remove the "this pointer" from the given window handle
//
// -----------------------------------------------------------------------
void AwtWindow::RemoveThisPtr(HWND hWnd)
{
    ASSERT( ::IsWindow(hWnd) );

#if defined(_WIN32 )
    ::RemoveProp(hWnd, MAKEINTATOM(AwtWindow::thisPtr));
#else
    ::RemoveProp(hWnd, MAKEINTATOM(AwtWindow::thisPtr));
    ::RemoveProp(hWnd, MAKEINTATOM(AwtWindow::thisLow));
#endif
}


// -----------------------------------------------------------------------
//
//
// -----------------------------------------------------------------------
LRESULT AWT_CALLBACK AwtWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, 
                                    LPARAM lParam)
{
    MSG msg;
    DWORD pos;
    LRESULT result = 0;
    AwtWindow* pThis = AwtWindow::GetThisPtr(hWnd);

    ASSERT( pThis );
    if (pThis) {
        //
        // Save the DefWindowProc pointer in a local variable, since it will
        // be lost when a WM_DESTROY message is processed...
        //
        WNDPROC DefProc = pThis->m_PrevWndProc;

        //
        // Copy the current windows message into a termporary MSG structure...
        // This structure can be accessed via the GetCurrentMessage() method.
        //
        pos = ::GetMessagePos();

        msg.hwnd    = hWnd;
        msg.message = message;
        msg.wParam  = wParam;
        msg.lParam  = lParam;
        msg.time    = ::GetMessageTime();
        msg.pt.x    = (int)(short)LOWORD(pos);
        msg.pt.y    = (int)(short)HIWORD(pos);

        pThis->m_pMsg = &msg;

        if (pThis->Dispatch(message, wParam, lParam, &result) == FALSE) {
            result = ::CallWindowProc(DefProc, hWnd, message, wParam, lParam);
        }

        // Clear the cached MSG pointer
        pThis->m_pMsg = NULL;
    }

    return result;
}

// -----------------------------------------------------------------------
//
//
// -----------------------------------------------------------------------
LRESULT AWT_CALLBACK AwtWindow::KeyHook(int code, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;

    if (code == HC_ACTION) {
        HWND hWnd = ::GetFocus();
        AwtWindow* pThis = AwtWindow::GetThisPtr(hWnd);
        BOOL bIsKeyDown = ! (lParam & 0x80000000L);

        if (/*bIsKeyDown &&*/ pThis) {
            MSG msg;
            DWORD pos;
            UINT scanCode  = HIWORD(lParam) & 0xFF;

            //
            // Copy the current windows message into a termporary MSG 
            // structure... This structure can be accessed via the 
            // AwtComponent::GetCurrentMessage() method.
            //
            pos = ::GetMessagePos();

            msg.hwnd    = hWnd;
            msg.message = (bIsKeyDown ? WM_KEYDOWN : WM_KEYUP);
            msg.wParam  = wParam;
            msg.lParam  = lParam;
            msg.time    = ::GetMessageTime();
            msg.pt.x    = (int)(short)LOWORD(pos);
            msg.pt.y    = (int)(short)HIWORD(pos);

            pThis->m_pMsg = &msg;

            // Is the key being released or pressed?
            if (bIsKeyDown) {
                ret = pThis->OnKeyDown(wParam, scanCode);
            } else {
                ret = pThis->OnKeyUp(wParam, scanCode);
            }

            // Clear the cached MSG pointer.
            pThis->m_pMsg = NULL;
        } else {
            ret = ::CallNextHookEx(m_hKeyboardHook, code, wParam, lParam);
        }
    }

    return ret;
}


// -----------------------------------------------------------------------
//
//
// -----------------------------------------------------------------------
BOOL AwtWindow::Dispatch(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT * result)
{
    AwtComponent *pControl;
    BOOL bRet = FALSE;

    *result = 0;
    switch( msg ) {
        case WM_CLOSE:
            bRet = OnClose();
            break;

        case WM_DESTROY:
            bRet = OnDestroy();
            break;

        case WM_PAINT: {
            RECT r;

            ::GetUpdateRect(m_hWnd, &r, FALSE);

            bRet = OnPaint((HDC)wParam, r.left, r.top, 
                           (r.right-r.left), (r.bottom - r.top));
            break;
        }

        case WM_ERASEBKGND:
            bRet = OnEraseBackground((HDC)wParam);
            // If the message has been handled, then return non-zero
            *result = bRet;
            break;

        case WM_COMMAND:
#ifdef _WIN32
            pControl = AwtWindow::GetThisPtr( (HWND)lParam );

			// when WM_COMMAND comes in from menu there is no lParam
			// and so no object is able to receive the OnCommand.
			// In this, since we know it is a menu, we call OnSelect
			// on this
            if (pControl) {
                bRet = pControl->OnCommand( HIWORD(wParam), LOWORD(wParam) );
            }
            else if (HIWORD(wParam) == 0) {
                // this is a menu, so notify the owner
                OnSelect(LOWORD(wParam));
            }
#else
            pControl = AwtWindow::GetThisPtr( (HWND)LOWORD(lParam) );

            if (pControl) {
                bRet = pControl->OnCommand( HIWORD(lParam), wParam );
            }
            else if (HIWORD(lParam) == 0) {
                // this is a menu, so notify the owner
                OnSelect(wParam);
            }
#endif
            // Return 0 if the command was handled...
            *result = !bRet;
            break;

#ifdef _WIN32
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSCROLLBAR:
            pControl = AwtWindow::GetThisPtr( (HWND)lParam );

#else
        case WM_CTLCOLOR:
            pControl = AwtWindow::GetThisPtr( (HWND)LOWORD(lParam) );
#endif
            if (pControl) {
                bRet = pControl->OnCtlColor( (HDC)wParam );
                if (bRet) {
                    *result = (LRESULT)pControl->GetColor(Background)->GetBrush();
                }
            }
            break;

        case WM_SIZE: 
        {
            RECT rc;

            ::GetWindowRect(m_hWnd, &rc);
            
            // the coordinates has to be the parent coordinates.
            // so if this is not a top level window convert the coordinates
            //HWND hwnd = ::GetParent(m_hWnd);
            //if (!hwnd)
            //    hwnd = ::GetWindow(m_hWnd, GW_OWNER);

            //if (hwnd) {
            //    RECT rcParent;
            //    ::GetWindowRect(hwnd, &rcParent);
            //    rc.left -= rcParent.left;
            //    rc.top -= rcParent.top;
            //    rc.right -= rcParent.left;
            //    rc.bottom -= rcParent.top;
            //}

            bRet = OnSize( (UINT)wParam, rc.right - rc.left, rc.bottom - rc.top);
            // If the message has been handled, then return zero
            *result = ! bRet;
            break;
        }

        case WM_MOVE:
        {
            RECT rc;

            ::GetWindowRect(m_hWnd, &rc);
            bRet = OnMove(rc.left, rc.top);
            break;
        }

        case WM_LBUTTONDOWN:
            bRet = OnMouseDown(AwtLeftButton, wParam, 
                               (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            bRet = OnMouseUp(AwtLeftButton, wParam,
                             (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_LBUTTONDBLCLK:
            bRet = OnMouseDblClick(AwtLeftButton, wParam,
                                   (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_RBUTTONDOWN:
            bRet = OnMouseDown(AwtRightButton, wParam,
                               (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_RBUTTONUP:
            bRet = OnMouseUp(AwtRightButton, wParam,
                             (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_RBUTTONDBLCLK:
            bRet = OnMouseDblClick(AwtRightButton, wParam,
                                   (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_MBUTTONDOWN:
            bRet = OnMouseDown(AwtMiddleButton, wParam,
                               (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_MBUTTONUP:
            bRet = OnMouseUp(AwtMiddleButton, wParam,
                             (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_MBUTTONDBLCLK:
            bRet = OnMouseDblClick(AwtMiddleButton, wParam,
                                   (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_MOUSEMOVE: {
            AwtButtonType button = AwtNoButton;

            if      (wParam & MK_LBUTTON)  button = AwtLeftButton;
            else if (wParam & MK_MBUTTON)  button = AwtMiddleButton;
            else if (wParam & MK_RBUTTON)  button = AwtRightButton;

            bRet = OnMouseMove(button, wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;
        }
        
//        case WM_KEYDOWN:
//            bRet = OnKeyDown(wParam);
//            break;

        case WM_CHAR:
            bRet = OnChar(wParam);
            break;

//        case WM_KEYUP:
//            bRet = OnKeyUp(wParam, (HIWORD(lParam) & 0xFF));
//            break;

        case WM_SETFOCUS:
            bRet = OnSetFocus();

            // Return 0 if the setfocus was handled...
            *result = !bRet;
            break;
        case WM_KILLFOCUS:
            bRet = OnKillFocus();

            *result = !bRet;
            break;
        case WM_SETCURSOR:
            bRet = OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            // keep the TRUE when returned from the function
            *result = bRet;
            break;

        case WM_HSCROLL:
#if _WIN32
            bRet = OnHScroll(LOWORD(wParam), (short)HIWORD(wParam));
#else
            bRet = OnHScroll(wParam, LOWORD(lParam));
#endif

            //!DR this is to call OnScroll on our scroll bar implementation.
            //!DR We should catch the LButtonDown and do everything on our own...
#ifdef _WIN32
            pControl = AwtWindow::GetThisPtr( (HWND)lParam );

            if (pControl) {
                pControl->OnScroll( LOWORD(wParam), (short)HIWORD(wParam) );
            }
#else
            pControl = AwtWindow::GetThisPtr( (HWND)LOWORD(lParam) );

            if (pControl) {
                pControl->OnScroll( wParam, HIWORD(lParam));
            }
#endif
            break;

        case WM_VSCROLL:
#if _WIN32
            bRet = OnVScroll(LOWORD(wParam), (short)HIWORD(wParam));
#else
            bRet = OnVScroll(wParam, LOWORD(lParam));
#endif

            //!DR this is to call OnScroll on our scroll bar implementation.
            //!DR We should catch the LButtonDown and do everything on our own...
#ifdef _WIN32
            pControl = AwtWindow::GetThisPtr( (HWND)lParam );

            if (pControl) {
                pControl->OnScroll( LOWORD(wParam), (short)HIWORD(wParam) );
            }
#else
            pControl = AwtWindow::GetThisPtr( (HWND)LOWORD(lParam) );

            if (pControl) {
                pControl->OnScroll( wParam, HIWORD(lParam));
            }
#endif

            break;

        case WM_MENUSELECT:
#ifdef _WIN32
            bRet = OnMenuSelect((HMENU)lParam, (UINT) HIWORD(wParam), (UINT) LOWORD(wParam));
#else
            bRet = OnMenuSelect((HMENU)HIWORD(lParam), (UINT) LOWORD(lParam), (UINT)wParam);
#endif

            break;

        case WM_NCHITTEST: 
        {
            POINT pt;
            pt.x = (int)(short)LOWORD(lParam);
            pt.y = (int)(short)HIWORD(lParam);
            bRet = OnNCHitTest(&pt, wParam);
            break;
        }

        case WM_PALETTECHANGED: 
        {
            HWND hwndPalChg = (HWND) wParam; // handle of window that changed palette 

            if( hwndPalChg == m_hWnd)
            {
              break;
            }

            // The following code is for all other windows to realize 
            // their palettes as background palettes when the window in
            // focus gets to realize its palette  as a foreground palette. 
            // By this all windows should have the best possible colors.


            HPALETTE hPal = AwtToolkit::GetToolkitDefaultPalette()->GetPalette();
            HDC hDC = ::GetDC(m_hWnd);

            VERIFY(::SelectPalette(hDC, hPal, TRUE));
            VERIFY(::ReleaseDC(m_hWnd, hDC));

            ::InvalidateRect(m_hWnd,	NULL, TRUE);
            ::UpdateWindow(m_hWnd);
            break;
        }
        case WM_QUERYNEWPALETTE:
        {
            HPALETTE hNewPal = AwtToolkit::GetToolkitDefaultPalette()->GetPalette();
            HDC      hDC     = ::GetDC(m_hWnd);
            HPALETTE hOldPal = ::SelectPalette(hDC, hNewPal, TRUE);
            int i = ::RealizePalette(hDC);       // Realize drawing palette.
            VERIFY(::ReleaseDC(m_hWnd, hDC));

            //if (i)    
            //{                     
                ::SelectClipRgn(hDC, NULL);
                ::InvalidateRect(m_hWnd, NULL, TRUE);    
                ::UpdateWindow(m_hWnd);
            //}
                                                     
            hDC = ::GetDC(m_hWnd);
            VERIFY(::SelectPalette(hDC, hOldPal, TRUE));
            VERIFY(::ReleaseDC(m_hWnd, hDC));
            break;
        }
#ifdef INTLUNICODE
		case WM_DRAWITEM:
		if(lParam)
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
			if(lpdis->CtlType == ODT_MENU)
					bRet = OnDrawMenuItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);      
			else
			{
				pControl = AwtWindow::GetThisPtr( (HWND)lpdis->hwndItem);
				if (pControl) {
					bRet = pControl->OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);            
				}
			}
			break;
		}
		case WM_MEASUREITEM:
		if(lParam)
		{
			LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;
			if(lpmis->CtlType == ODT_MENU)
	            bRet = OnMeasureMenuItem((UINT)wParam, (LPMEASUREITEMSTRUCT)lParam);
			break;
		}
		case WM_DELETEITEM:
		{
	        bRet = OnDeleteItem((UINT)wParam, (LPDELETEITEMSTRUCT)lParam);
			break;
		}
#endif

    }

    return bRet;
}


BOOL AwtWindow::OnNCHitTest(LPPOINT ppt, int nHitTest)
{

    // if m_HoldMouse is not this we are moving over a different window
    if (AwtToolkit::m_HoldMouse != this) {
        
        int64 time = PR_NowMS(); // time in milliseconds

        // if m_HoldMouse exists fire an exit event
        if (AwtToolkit::m_HoldMouse) {
            ::ScreenToClient(AwtToolkit::m_HoldMouse->GetWindowHandle(), ppt);
            AwtToolkit::RaiseJavaEvent(AwtToolkit::m_HoldMouse->GetPeer(), 
                                        AwtEventDescription::MouseExit,
                                        time, 
                                        NULL,
                                        ppt->x,
                                        ppt->y);
            ::ClientToScreen(AwtToolkit::m_HoldMouse->GetWindowHandle(), ppt);
        }
        else 
            // first time in,  install the mouse timer
            AwtToolkit::CreateTimer();


        // update static information
        AwtToolkit::m_HoldMouse = this;

        //Lock();
        
        // fire java event. Mouse position in client coordinates
        ::ScreenToClient(m_hWnd, ppt);

        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                    AwtEventDescription::MouseEnter,
                                    time, 
                                    NULL,
                                    ppt->x,
                                    ppt->y);

        //Unlock();
    }


    return FALSE;
}


BOOL AwtWindow::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                                
  //REMIND: Query the component size and return the actual size

    PAINTSTRUCT ps;
    HDC hDC = ::BeginPaint(m_hWnd, &ps);

    // discard (0, 0, 0, 0) rect
    if (ps.rcPaint.left || ps.rcPaint.right || ps.rcPaint.top || ps.rcPaint.bottom) {
        ::FillRect(hDC, &ps.rcPaint, m_BackgroundColor->GetBrush());

        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::Expose,
                                   ll_zero_const, NULL,
                                   x, y, w, h);
    }
    ::EndPaint(m_hWnd, &ps);

    return TRUE;
}


BOOL AwtWindow::OnDestroy( void )
{
    // Release the AwtDC if one was created...
    if (m_pAwtDC) {
        m_pAwtDC->ResetTargetObject();
        m_pAwtDC->Release();
        m_pAwtDC = NULL;
    }

    //
    // Restore the previous Window Procedure before the window is destroyed
    //
    SubclassWindow(FALSE);

    //
    // check out if this window was holding the mouse 
    // We want to deal with AwtToolkit::m_HoldMouse only in the gui thread
    // so we cut down our chances to dead lock
    //
    if (AwtToolkit::m_HoldMouse == this) {
        // delete timer
        AwtToolkit::DestroyTimer();
        AwtToolkit::m_HoldMouse = 0;
    }

    m_hWnd = NULL;
    return AwtComponent::OnDestroy();
}

