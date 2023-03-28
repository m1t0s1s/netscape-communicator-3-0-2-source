#include "awt_component.h"
#include "awt_toolkit.h"        // RaiseJavaEvent

#ifdef INTLUNICODE
#include "awt_i18n.h"
#include "awt_wcompstr.h"
#endif	// INTLUNICODE
// All java header files are extern "C"
extern "C" {
#include "java_awt_Component.h"
#include "java_awt_Color.h"
#include "java_awt_Event.h"
#include "java_awt_Font.h"

#include "sun_awt_windows_WComponentPeer.h"
};


//-------------------------------------------------------------------------
//
// Component constructor
//
//-------------------------------------------------------------------------
AwtComponent::AwtComponent(JHandle *peer) : AwtObject(peer)
{
    SET_OBJ_IN_PEER((HWComponentPeer *)peer, this);

    // we should ask windows about default background and foreground.
    // The best would be asking to the host application if any.
    // Default Foreground color is BLACK
    m_ForegroundColor = AwtCachedBrush::Create( RGB(0, 0, 0) );
    // Default Background color is GRAY
    m_BackgroundColor = AwtCachedBrush::Create( RGB(192, 192, 192) );

    // Initialize the size of the client area...
    m_ClientArea.x = 0;
    m_ClientArea.y = 0;

    m_pFont  = NULL;
    m_pAwtDC = NULL;

    LL_UI2L(m_TimeLastMouseDown, 0); 

    m_nMouseCount = 1;
    m_nCaptured = 0;

    //
    // By default, only keyboard events are sent to java before the native
    // widget...
    //
    m_bPreprocessAllEvents = FALSE;
    m_awtObjType = AWT_COMPONENT;
#ifdef INTLUNICODE
	m_primaryEncoding = libi18n::GetPrimaryEncoding();
#endif

}


//-------------------------------------------------------------------------
//
// Component destructor
//
//-------------------------------------------------------------------------
AwtComponent::~AwtComponent()
{
    m_ForegroundColor->Release();
    m_BackgroundColor->Release();

    // The DC MUST be destroyed by now...
    ASSERT( m_pAwtDC == NULL );

    if (m_pFont) {
        m_pFont->Release();
        m_pFont = NULL;
    }

    // remove pending events from the event queue
    if (m_pJavaPeer) {
        SET_OBJ_IN_PEER((HWComponentPeer *)m_pJavaPeer, NULL);
        AwtToolkit::theToolkit->RemoveEventsForObject(m_pJavaPeer);
        m_pJavaPeer = NULL;
    }
}


//-------------------------------------------------------------------------
//
// Dispose is a java hook to delete native resources
//
//-------------------------------------------------------------------------
void AwtComponent::Dispose(void)
{
    // Release the AwtDC if one was created...
    if (m_pAwtDC) {
        // disconnect yourself from the DC object
        // no release on the real DC is needed since 
        // we are using common dc here
        m_pAwtDC->ResetTargetObject();
        m_pAwtDC->Release();
        m_pAwtDC = NULL;
    }

    AwtObject::Dispose();
}


//-------------------------------------------------------------------------
//
// Every functions which need a thread switch go thru this function
// by calling SendMessage (..WM_AWT_CALLMETHOD..) in AwtToolkit::CallAwtMethod.
//
//-------------------------------------------------------------------------
BOOL AwtComponent::CallMethod(AwtMethodInfo *info)
{
    BOOL bRet = TRUE;

    switch (info->methodId) {
      case AwtComponent::CREATE:
        CreateWidget( (HWND)(info->p1) );
        break;

      case AwtComponent::DESTROY:
        Destroy((HWND)info->p1);
        break;

      case AwtComponent::GET_FOCUS:
        GetFocus();
        break;

      case AwtComponent::HANDLE_EVENT:
        HandleEvent( (JavaEventInfo *)(info->p1), (Event*)info->p2 );
        break;

      default:
        bRet = FALSE;
        break;
    }
    return bRet;
    
}


//-------------------------------------------------------------------------
//
// Attach store a back-pointer to the dc object.
// Addref so that the lifetime of the dc object is at least as ours
//
//-------------------------------------------------------------------------
void AwtComponent::Attach(AwtDC* pDC) 
{ 
    m_pAwtDC = pDC;
    m_pAwtDC->AddRef();
}


//-------------------------------------------------------------------------
//
// In a generic component we use a commom=n dc
// On ReleaseDC() called via AwtDC::Unlock(AwtGraphics*) we want to
// release the dc and zero AwtDC data member so that next time we get in
// we get a fresh dc and reload all the values
//
//-------------------------------------------------------------------------
void AwtComponent::ReleaseDC(AwtDC* pdc) 
{
    HDC hdc = pdc->GetDC();

    pdc->ResetOwnerGraphics();
    pdc->ResetDC();
    VERIFY(::ReleaseDC(GetWindowHandle(), hdc)); 
}


//-------------------------------------------------------------------------
//
// Return the proper brush
//
//-------------------------------------------------------------------------
AwtCachedBrush* AwtComponent::GetColor(AwtColorType type)
{
    AwtCachedBrush *color;

    if (type == Foreground) {
        color = m_ForegroundColor;
    }
    else if (type == Background) {
        color = m_BackgroundColor;
    }
    else {
        color = NULL;
    }

    return color;
}


//-------------------------------------------------------------------------
//
// Set foreground or background color for this component
//
//-------------------------------------------------------------------------
BOOL AwtComponent::SetColor(AwtColorType type, COLORREF color)
{
    BOOL bStatus = TRUE;
    HWND hwnd = GetWindowHandle();

    if (type == Foreground) {
        if (color != m_ForegroundColor->GetColor()) {
            m_ForegroundColor->Release();
            m_ForegroundColor = AwtCachedBrush::Create( color );
            if (hwnd)
                ::InvalidateRect(hwnd, NULL, FALSE);
        }
    }
    else if (type == Background) {
        if (color != m_BackgroundColor->GetColor()) {
            m_BackgroundColor->Release();
            m_BackgroundColor = AwtCachedBrush::Create( color );
            if (hwnd)
                ::InvalidateRect(hwnd, NULL, TRUE);
        }
    }
    else {
        bStatus = FALSE;
    }

    return bStatus;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
AwtFont* AwtComponent::GetFont(void)
{
    return m_pFont;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtComponent::SetFont(AwtFont* font)
{
    if (font) {
        font->AddRef();
    }

    //
    // Release the current font...
    //
    if (m_pFont) {
        m_pFont->Release();
    }

    m_pFont = font;

    return TRUE;
}

#ifdef INTLUNICODE
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
int16 AwtComponent::GetPrimaryEncoding()
{
	return m_primaryEncoding;
}
#endif


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
HWND AwtComponent::GetParentWindowHandle(HWComponentPeer *hParent)
{
    AwtComponent *pParent;

    pParent = (hParent) ? GET_OBJ_FROM_PEER(AwtComponent, hParent) : NULL;
    return    (pParent) ? pParent->GetWindowHandle()               : NULL;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
long AwtComponent::GetMouseModifiers(UINT flags)
{
    long result = 0;

    if (flags & MK_CONTROL) {
        result |= java_awt_Event_CTRL_MASK;
    }

    if (flags & MK_SHIFT) {
        result |= java_awt_Event_SHIFT_MASK;
    }

    if ((flags & MK_MBUTTON) ||
                (::GetKeyState(VK_MENU) & 0x80)) {
        result |= java_awt_Event_ALT_MASK;
    }

    if (flags & MK_RBUTTON) {
        result |= java_awt_Event_META_MASK;
    }

    return result;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
long AwtComponent::GetKeyModifiers(BYTE *keyState)
{
    long result = 0;

    if (keyState[VK_CONTROL] & 0x80) {
        result |= java_awt_Event_CTRL_MASK;
    }

    if (keyState[VK_SHIFT] & 0x80) {
        result |= java_awt_Event_SHIFT_MASK;
    }

    if ((keyState[VK_MBUTTON] & 0x80) ||
                (keyState[VK_MENU] & 0x80)) {
        result |= java_awt_Event_ALT_MASK;
    }

    if (keyState[VK_RBUTTON] & 0x80) {
        result |= java_awt_Event_META_MASK;
    }

    return result;
}


//-------------------------------------------------------------------------
//
// change the virtual key coming from windows into an ascii code
//
//-------------------------------------------------------------------------
BOOL AwtComponent::TranslateToAscii(BYTE *keyState, UINT virtualCode, 
                                    UINT scanCode, WORD *asciiKey)
{
#ifdef _WIN32
    WORD asciiBuf;
#else
    DWORD asciiBuf;
#endif
    BOOL bIsExtended;

    bIsExtended = TRUE;
    *asciiKey   = 0;
    switch (virtualCode) {
        case VK_HOME:  *asciiKey = java_awt_Event_HOME;   break;
        case VK_END:   *asciiKey = java_awt_Event_END;    break;
        case VK_PRIOR: *asciiKey = java_awt_Event_PGUP;   break;
        case VK_NEXT:  *asciiKey = java_awt_Event_PGDN;   break;
        case VK_UP:    *asciiKey = java_awt_Event_UP;     break;
        case VK_DOWN:  *asciiKey = java_awt_Event_DOWN;   break;
        case VK_LEFT:  *asciiKey = java_awt_Event_LEFT;   break;
        case VK_RIGHT: *asciiKey = java_awt_Event_RIGHT;  break;
        case VK_F1:    *asciiKey = java_awt_Event_F1;     break;
        case VK_F2:    *asciiKey = java_awt_Event_F2;     break;
        case VK_F3:    *asciiKey = java_awt_Event_F3;     break;
        case VK_F4:    *asciiKey = java_awt_Event_F4;     break;
        case VK_F5:    *asciiKey = java_awt_Event_F5;     break;
        case VK_F6:    *asciiKey = java_awt_Event_F6;     break;
        case VK_F7:    *asciiKey = java_awt_Event_F7;     break;
        case VK_F8:    *asciiKey = java_awt_Event_F8;     break;
        case VK_F9:    *asciiKey = java_awt_Event_F9;     break;
        case VK_F10:   *asciiKey = java_awt_Event_F10;    break;
        case VK_F11:   *asciiKey = java_awt_Event_F11;    break;
        case VK_F12:   *asciiKey = java_awt_Event_F12;    break;

        case VK_DELETE:*asciiKey = '\177'; bIsExtended = FALSE;   break;
        case VK_RETURN:*asciiKey = '\n';   bIsExtended = FALSE;   break;

        default:        
            bIsExtended = FALSE;

            if (::ToAscii(virtualCode, scanCode, keyState, &asciiBuf, FALSE) == 1) {
                *asciiKey = (char)asciiBuf;
            }

            //!DR 
            // Well VkKeyScan is supposed to be the inverse of ToAscii, 
            // so I catch asciiKey right away and pass it to the VkKeyScan.
            // In the <ctrl>C case I never get back the 'C'.
            // ToAscii returns 3 and this value is never changed by VkKeyScan.
            // This is causing a problem on 95 where the <ctrl>C sequence 
            // doesn't generate a copy command for edit fields.

            // Ok, even <ctrl>H <ctrl>J <ctrl>I <ctrl>M <ctrl>[ have the same
            // behavior. They are particular combination to simulate esc, tab and so...

            //SHORT check = ::VkKeyScan(*asciiKey);

            break;
    }
    return bIsExtended;
}


//-------------------------------------------------------------------------
//
// Windows Event handlers...
//
//-------------------------------------------------------------------------

BOOL AwtComponent::OnClose( void )
{
    if (m_pJavaPeer) {

        //
        // Call sun.awt.windows.WComponentPeer::handleQuit(...)
        //
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::Quit,
                                   ll_zero_const, NULL);
    }
    
    return TRUE;    // Stop the event here...
}


BOOL AwtComponent::OnDestroy( void )
{
    return FALSE;
}


BOOL AwtComponent::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                                
  //REMIND: Query the component size and return the actual size

    // discard (0, 0, 0, 0) rect
    if (x || y || w || h)
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::Expose, 
                                    ll_zero_const, NULL,
                                    x, y, w, h);
    return FALSE;
}


BOOL AwtComponent::OnEraseBackground( HDC hdc )
{
    return TRUE;
}



BOOL AwtComponent::OnCtlColor( HDC hdc )
{
    ::SetBkColor  (hdc, m_BackgroundColor->GetColor());
    ::SetTextColor(hdc, m_ForegroundColor->GetColor());

    return TRUE;
}


BOOL AwtComponent::OnSize( UINT sizeCode, UINT newWidth, UINT newHeight )
{
    HWComponentPeer     *peer;
    Hjava_awt_Component *target;

    //
    // Update the cached size in the AwtComponent
    //
    m_ClientArea.x = newWidth;
    m_ClientArea.y = newHeight;

    if (m_pJavaPeer) {
        //
        // Update the width & height in java.awt.Component
        //
        // Since the target's width and height properties are not public
        // they cannot be updated within WComponentPeer::handleResize() 
        //
        peer = (HWComponentPeer *)m_pJavaPeer;
        if (unhand(peer)->target) {
            target = unhand(peer)->target;
            unhand(target)->width  = newWidth;
            unhand(target)->height = newHeight;
        }

        //
        // Call netscape.awt.windows.WComponentPeer::handleResize(...)
        //
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::Resize,
                                   ll_zero_const, NULL,
                                   newWidth, newHeight);
    }

    return FALSE;   // Pass the event on to Windows
}


//-------------------------------------------------------------------------
//
// Move update the x and y position for the component and fire a move event
// The (x,y) position is in screen coordinates.
//-------------------------------------------------------------------------
BOOL AwtComponent::OnMove(int newX, int newY)
{
    return FALSE;
}


//-------------------------------------------------------------------------
//
// Handle any mouse down. Fire java MouseDown event and capture 
// the mouse, if required, so MOUSE_DRAG is direct to the proper
// component.
//
//-------------------------------------------------------------------------
BOOL AwtComponent::OnMouseDown(AwtButtonType button, UINT metakeys, 
                               int x, int y)
{
    BOOL bRet = FALSE;

    if (m_pJavaPeer) {
        MSG *pMsg;
        int64 dblClickTime;
        int64 clickDelta;
        int64 time = PR_NowMS(); // time in milliseconds

        LL_UI2L(dblClickTime, ::GetDoubleClickTime());
        LL_SUB(clickDelta, time, m_TimeLastMouseDown);
        // increment or reset count according to when we received this message
        m_nMouseCount = (LL_CMP(clickDelta, >, dblClickTime)) ? 1
                                                            : m_nMouseCount+1;
        // keep the time this click happens, so we can correctly
        // send a mouse count.
        m_TimeLastMouseDown = time;

        // ask the component if mouse down needs to force a SetFocus.
        // Usually we don't need to call SetFocus on controls (actually 
        // for radio and scrollbar we *must* not) but we have to when
        // the window receiving the click is inside the navigator
        if (WantFocusOnMouseDown())
            GetFocus();

        // CaptureMouse to make MouseDown/MouseDrag/MouseUp events working
        CaptureMouse(button, metakeys);

        //
        pMsg = (m_bPreprocessAllEvents) ? GetCurrentMessage() : NULL;

        //
        // Call sun.awt.windows.WComponentPeer::handleMouseDown(...)
        //
        // REMIND: add support for click > 1
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::MouseDown,
                                   time, pMsg,
                                   x, y, m_nMouseCount, GetMouseModifiers(metakeys));

        bRet = m_bPreprocessAllEvents;

    }

    return bRet;
}


//-------------------------------------------------------------------------
//
// Handle any mouse up. Fire java MouseUp event and release  
// the mouse, if required.
//
//-------------------------------------------------------------------------
BOOL AwtComponent::OnMouseUp(AwtButtonType button, UINT metakeys, 
                             int x, int y)
{
    BOOL bRet = FALSE;

    if (m_pJavaPeer) {
        MSG *pMsg;
        int64 time = PR_NowMS(); // time in milliseconds

        //
        pMsg = (m_bPreprocessAllEvents) ? GetCurrentMessage() : NULL;

        //
        // Call sun.awt.windows.WComponentPeer::handleMouseUp(...)
        //
        // REMIND: add support for click > 1
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::MouseUp,
                                   time, pMsg,
                                   x, y, 1, GetMouseModifiers(metakeys));

        // call ReleaseMouse which might have to let defproc to pre-process
        // the message (see button), so the return value is either what
        // ReleaseMouse is returning or m_bPreprocessAllEvents
        bRet == ReleaseMouse(button, metakeys, x, y) 
                            || m_bPreprocessAllEvents;

    }

    // don't call defproc, we did already
    return bRet;
}


//-------------------------------------------------------------------------
//
// Mouse Double click is not used at all in java, so just send a mouse up
// since windows events sequence for a double click is
// MouseDown, MouseUp, MouseDblClick, MouseUp.
// In java there is the mouse count concept wich MouseDown brings with
// itself. Mouse count indicates how many consecutives click are
// performed in a defined amount of time
//
//-------------------------------------------------------------------------
BOOL AwtComponent::OnMouseDblClick(AwtButtonType button, UINT metakeys, 
                                   int x, int y)
{
    return OnMouseDown(button, metakeys, x, y);
}


BOOL AwtComponent::OnMouseMove( AwtButtonType button, UINT metakeys, 
                                int x, int y )
{
    if (m_pJavaPeer) {
        AwtEventDescription::EventId messageType;
        int64 time = PR_NowMS(); // time in milliseconds

        if (button == AwtNoButton) {
            messageType = AwtEventDescription::MouseMove;
        } else {
            messageType = AwtEventDescription::MouseDrag;
        }

        //
        // Call sun.awt.windows.WComponentPeer::handleMouseMoved(...)
        // or   sun.awt.windows.WComponentPeer::handleMouseDrag(...)
        //
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, messageType,
                                   time, NULL,
                                   x, y, GetMouseModifiers(metakeys));
    }
    return FALSE;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtComponent::OnKeyDown( UINT virtualCode, UINT scanCode)
{
    WORD asciiKey;
    AwtEventDescription::EventId messageType;

    //
    // Set the high order bit on the scan-code to indicate that it is a
    // key-down transition...  This is required by TranslateToAscii(...)
    // in some situations.  
    //
    // See the Windows documentation on ::ToAscii() for more info.
    //
#ifdef WIN32
    scanCode |= 0x80000000;
#else
/// scanCode |= 0x8000;
#endif

    if (m_pJavaPeer) {
        //
        // Get the current state of every key on the keyboard.  This info
        // is necessary for both TranskateToAscii(...) and determining if
        // any modifier keys (ie. CTRL, SHIFT, etc) are involved...
        //
        ::GetKeyboardState(m_keyState);

        //
        // Convert the virtual key code to an Ascii character.
        // 
        // TranslateToAscii(...) returns TRUE if the virtual key represents
        // an extended character (ie. F1, etc)
        //
        asciiKey = 0;
        if (TranslateToAscii(m_keyState, virtualCode, scanCode, &asciiKey)) {
            messageType = AwtEventDescription::ActionKeyDown;
        } else {
            messageType = AwtEventDescription::KeyDown;
        }

        if (asciiKey) {
            int64 time = PR_NowMS(); // time in milliseconds
            //
            // Call sun.awt.windows.WComponentPeer::handleKeyPress(...) 
            // or   sun.awt.windows.WComponentPeer::handleAction(...)
            //
            AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                        messageType,
                        time, GetCurrentMessage(),
                        0, 0, asciiKey, GetKeyModifiers(m_keyState));

            return TRUE;
        }
    }
    
    return FALSE;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtComponent::OnKeyUp( UINT virtualCode, UINT scanCode)
{
    WORD asciiKey;
    AwtEventDescription::EventId messageType;

    if (m_pJavaPeer) {
        //
        // Get the current state of every key on the keyboard.  This info
        // is necessary for both TranskateToAscii(...) and determining if
        // any modifier keys (ie. CTRL, SHIFT, etc) are involved...
        //
        ::GetKeyboardState(m_keyState);

        //
        // Convert the virtual key code to an Ascii character.
        // 
        // TranslateToAscii(...) returns TRUE if the virtual key represents
        // an extended character (ie. F1, etc)
        //
        asciiKey = 0;
        if (TranslateToAscii(m_keyState, virtualCode, scanCode, &asciiKey)) {
            messageType = AwtEventDescription::ActionKeyUp;
        } else {
            messageType = AwtEventDescription::KeyUp;
        }

        if (asciiKey) {
            int64 time = PR_NowMS(); // time in milliseconds
            //
            // Call sun.awt.windows.WComponentPeer::handleKeyRelease(...) 
            // or   sun.awt.windows.WComponentPeer::handleActionRelease(...)
            //
            AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                        messageType,
                        time, GetCurrentMessage(),
                        0, 0, asciiKey, GetKeyModifiers(m_keyState));
    
            return TRUE;
        }
    }
    
    return FALSE;
}


BOOL AwtComponent::OnChar( UINT virtualCode )
{
    return FALSE;
}


BOOL AwtComponent::OnSetFocus( void )
{
    if (m_pJavaPeer) 
    {
        //
        // Call sun.awt.windows.WComponentPeer::handleQuit(...)
        //
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::GotFocus,
                                   ll_zero_const, NULL, 
                                   0, 0, 0, 0);
    }

    return FALSE;
}

BOOL AwtComponent::OnKillFocus( void )
{
    if (m_pJavaPeer) 
    {
        //
        // Call sun.awt.windows.WComponentPeer::handleQuit(...)
        //
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::LostFocus,
                                   ll_zero_const, NULL, 
                                   0, 0, 0, 0);
    }
    return FALSE;
}


BOOL AwtComponent::OnSetCursor(HWND hCursorWnd, WORD wHittest, WORD wMouseMsg)
{
    return FALSE;
}


BOOL AwtComponent::OnHScroll(UINT scrollCode, int cPos )
{
    return FALSE;
}


BOOL AwtComponent::OnVScroll(UINT scrollCode, int cPos )
{
    return FALSE;
}

BOOL AwtComponent::OnCommand(UINT notifyCode, UINT id)
{
    return FALSE;
}

// -----------------------------------------------------------------------
//
// menu notification coming from WM_COMMAND
// The menu id is the only information available
//
// -----------------------------------------------------------------------
BOOL AwtComponent::OnSelect(UINT id)
{
    return FALSE;
}


BOOL AwtComponent::OnMenuSelect(HMENU hMenu, UINT notifyCode, UINT id)
{
    return FALSE;
}


BOOL AwtComponent::OnNCHitTest(LPPOINT ppt, int nHitTest)
{
    return FALSE;
}

#ifdef INTLUNICODE

BOOL AwtComponent::OnDrawMenuItem(UINT idCtl, LPDRAWITEMSTRUCT lpdis)
{
	if( (lpdis) && (lpdis->itemID >= 0))
	{
		// Behavior in WinNT and Win30
		//
		//	ODS_CHECKED		With/Without Check mark in the beginning
		//	ODS_DISABLED	?
		//	ODS_FOCUS		?
		//	ODS_SELECTED	ODS_GRAYED		Background Color	Text Color			
		//	On				On				COLOR_HIGHLIGHT		COLOR_GRAYTEXT	<-	
		//	On				Off				COLOR_HIGHLIGHT		COLOR_HIGHLIGHTTEXT	
		//	Off				On				COLOR_MENU			COLOR_GRAYTEXT		
		//	Off				Off				COLOR_MENU			COLOR_MENUTEXT		
		// 
		// Behavior in Win95 and new WinNT

		DWORD	textcolor;
		DWORD	bkcolor;
		HBRUSH	bkbrush;
		// Setup Color
		if(lpdis->itemState & ODS_SELECTED)
		{
			bkcolor = ::GetSysColor(COLOR_HIGHLIGHT);
			bkbrush	= ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
			if(lpdis->itemState & ODS_GRAYED)
				textcolor = ::GetSysColor(COLOR_GRAYTEXT);
			else
				textcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		}
		else
		{
			bkcolor = ::GetSysColor(COLOR_MENU);
			bkbrush	= ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
			if(lpdis->itemState & ODS_GRAYED)
				textcolor = ::GetSysColor(COLOR_GRAYTEXT);
			else
				textcolor = ::GetSysColor(COLOR_MENUTEXT);
		}
		::SetTextColor(lpdis->hDC, textcolor);
		::SetBkColor(lpdis->hDC, bkcolor);
		::FillRect (lpdis->hDC, &lpdis->rcItem, bkbrush);

		::DeleteObject(bkbrush);

		if(lpdis->itemData)
		{
			WinMenuCompStr * pStr = (WinMenuCompStr *) lpdis->itemData;
			AwtVFontList *vfont = libi18n::GetMenuVFontList();
			if(vfont)
			{
				TEXTMETRIC metrics;
				pStr->GetTextMetrics(lpdis->hDC, vfont, &metrics);
				int baseline = metrics.tmExternalLeading + metrics.tmAscent;
				if(lpdis->itemState & ODS_CHECKED)
				{
					// We should draw the check mark here
					// Let figure out how to do it later
					HBITMAP	checkmark = libi18n::GetCheckmarkBitmap();
					// ::DrawBitmap(lpdis->hDC, checkmark, lpdis->rcItem.left+4, lpdis->rcItem.top );
					{
						BITMAP bm;
						HDC	hdcMem;
						DWORD	dwSize;
						POINT	ptSize, ptOrg;
						hdcMem = ::CreateCompatibleDC(lpdis->hDC);
						::SelectObject(hdcMem, checkmark);
						::SetMapMode(hdcMem, GetMapMode(lpdis->hDC));
						::GetObject(checkmark, sizeof(BITMAP), (LPVOID)&bm);
						ptSize.x = bm.bmWidth;
						ptSize.y = bm.bmHeight;
						DPtoLP(lpdis->hDC, &ptSize, 1);
						ptOrg.x = 0;
						ptOrg.y = 0;
						DPtoLP(hdcMem, &ptOrg, 1);
						::BitBlt(lpdis->hDC, 
								lpdis->rcItem.left+4, lpdis->rcItem.top+2,
								ptSize.x, ptSize.y,
								hdcMem, ptOrg.x, ptOrg.y, SRCCOPY);
						::DeleteDC(hdcMem);
					}
				}
				::SetBkMode(lpdis->hDC, TRANSPARENT);
				if(libi18n::iswin95() && 
					(lpdis->itemState & ODS_GRAYED) &&
					!(lpdis->itemState & ODS_SELECTED) )
				{
					// Draw the ghost text
					::SetTextColor(lpdis->hDC, RGB(0xFF,0xFF,0xFF));

					pStr->TextOutWidth(lpdis->hDC, 
										lpdis->rcItem.left+16, 
										lpdis->rcItem.top+baseline+1, 
										vfont);	
					::SetTextColor(lpdis->hDC, textcolor);
				}
				pStr->TextOutWidth(lpdis->hDC, 
										lpdis->rcItem.left+15, 
										lpdis->rcItem.top+baseline, 
										vfont);	
			}
		}
		return TRUE;
	}
	return FALSE;
}
BOOL AwtComponent::OnMeasureMenuItem(UINT idCtl, LPMEASUREITEMSTRUCT lpmis)
{
	BOOL ret = FALSE;
	if((lpmis) && (lpmis->itemData))
	{
		HDC hDC = libi18n::GetMenuDC();
		AwtVFontList *vfont = libi18n::GetMenuVFontList();
		if(hDC && vfont)
		{
			WinMenuCompStr* pStr = (WinMenuCompStr*)lpmis->itemData;
			SIZE size;
			pStr->GetTextExtent(hDC, vfont, &size);
			lpmis->itemWidth= size.cx + 8 + 8;
			lpmis->itemHeight= size.cy + 2;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL AwtComponent::OnMeasureItem(UINT idCtl, LPMEASUREITEMSTRUCT lpmis)
{
	return FALSE;
}


BOOL AwtComponent::OnDrawItem(UINT idCtl, LPDRAWITEMSTRUCT lpdis)
{
	if( (! lpdis) || (lpdis->itemID < 0))
		return FALSE;

	if(lpdis->itemState & ODS_SELECTED)
	{
		::SetTextColor(lpdis->hDC, m_BackgroundColor->GetColor());
		::FillRect (lpdis->hDC, &lpdis->rcItem, (HBRUSH) m_ForegroundColor->GetBrush());
		::SetBkColor(lpdis->hDC, m_ForegroundColor->GetColor());
	}
	else
	{
		::SetTextColor(lpdis->hDC, m_ForegroundColor->GetColor());
		::FillRect (lpdis->hDC, &lpdis->rcItem, (HBRUSH) m_BackgroundColor->GetBrush());
		::SetBkColor(lpdis->hDC, m_BackgroundColor->GetColor());
	}
	if(lpdis->itemState & ODS_FOCUS)
		::DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

	if(lpdis->itemData)
	{
		WinCompStr * pWcompstr = (WinCompStr *) lpdis->itemData;
		lpdis->rcItem.left += 3;
		pWcompstr->DrawText(lpdis->hDC, m_pFont, &lpdis->rcItem, DT_LEFT);
	}
	return TRUE;
}

BOOL AwtComponent::OnDeleteItem(UINT idCtl, LPDELETEITEMSTRUCT lpdis)
{
	if(lpdis && (lpdis->itemData) && (lpdis->itemData) &&
		((lpdis->CtlType == ODT_COMBOBOX) ||
		 (lpdis->CtlType == ODT_LISTBOX)) )
	{
		WinCompStr *pDelete = (WinCompStr *) lpdis->itemData;
		delete pDelete;
		return TRUE;
	}
	return FALSE;
}

#endif	// INTLUNICODE


//-------------------------------------------------------------------------
//
// Native methods for sun.awt.windows.WComponentPeer
//
//-------------------------------------------------------------------------

extern "C" {

void 
sun_awt_windows_WComponentPeer_dispose(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->Dispose();
}


void 
sun_awt_windows_WComponentPeer_show(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->Show(TRUE);
}


void 
sun_awt_windows_WComponentPeer_hide(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->Show(FALSE);
}


void 
sun_awt_windows_WComponentPeer_enable(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->Enable(TRUE);
}


void 
sun_awt_windows_WComponentPeer_disable(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->Enable(FALSE);
}


void 
sun_awt_windows_WComponentPeer_setForeground(HWComponentPeer *self, 
                                             struct Hjava_awt_Color *hColor)
{
    CHECK_PEER(self);

    if (hColor) {
        AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);
        Classjava_awt_Color *color = (Classjava_awt_Color *)unhand(hColor);

        DWORD rgbValue = color->value;
        COLORREF rgb = PALETTERGB( (rgbValue>>16)&0xFF, 
                                   (rgbValue>>8) &0xFF, 
                                   (rgbValue)    &0xFF);
    
        obj->SetColor(AwtComponent::Foreground, rgb);
    }
}


void 
sun_awt_windows_WComponentPeer_setBackground(HWComponentPeer *self, 
                                             struct Hjava_awt_Color *hColor)
{
    CHECK_PEER(self);

    if (hColor) {
        AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);
        Classjava_awt_Color *color = (Classjava_awt_Color *)unhand(hColor);
        DWORD rgbValue = color->value;
        COLORREF rgb = PALETTERGB( (rgbValue>>16)&0xFF, 
                                   (rgbValue>>8) &0xFF, 
                                   (rgbValue)    &0xFF);
    
        obj->SetColor(AwtComponent::Background, rgb);
    }
}


void 
sun_awt_windows_WComponentPeer_setFont(HWComponentPeer *self, 
                                       struct Hjava_awt_Font *font)
{
    CHECK_PEER(self);
    CHECK_OBJECT(font);

    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);
    AwtFont *pFont;

    //
    // Find or create an AwtFont object for the java_awt_Font...
    //
    // AwtFont::GetFontFromObject(...) does NOT increase the reference 
    // count on the AwtFont.  The AddRef(...) is performed in 
    // AwtComponent::SetFont(...)
    //
    pFont = AwtFont::GetFontFromObject(font);
    obj->SetFont(pFont);
}


void 
sun_awt_windows_WComponentPeer_requestFocus(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->GetFocus();
}


void 
sun_awt_windows_WComponentPeer_nextFocus(HWComponentPeer *self)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->NextFocus();
}


void 
sun_awt_windows_WComponentPeer_reshape(HWComponentPeer *self, 
                                       long x, long y, long w, long h)
{
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    obj->Reshape(x, y, w, h);
}


long 
sun_awt_windows_WComponentPeer_handleEvent(HWComponentPeer *self, 
                                           struct Hjava_awt_Event *event)
{
    // apparently when a top level window gets destroyed
    // we got at least one of this, but the C++ object is not
    // around anymore. In this case we don't want to assert
    if (unhand(self)->pNativeWidget != NULL) {

        CHECK_PEER_AND_RETURN(self, FALSE);
        AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);
        JavaEventInfo *pInfo;
        Classjava_awt_Event *pEvent = (Classjava_awt_Event *)unhand(event);
 
        pInfo = (JavaEventInfo *)unhand(event)->data;
        if (pInfo && (pInfo->m_object == (JHandle *)self)) {

            // handle event has been changed so that more info than just
            // the original message can be passed in.
            // This is a requirement for handling keys messages because
            // of the <ctrl>C problem on win95 (see AwtWindow::HandleMessage, 
            // AwtComponent::TranslateToAscii or JavaEventInfo::JavaCharToWindows)
            //obj->HandleEvent(&(pInfo->m_msg), pEvent);
            obj->HandleEvent(pInfo, pEvent);

            pInfo->Delete();
            unhand(event)->data = 0;
            return TRUE;
        }
    }

    return FALSE;
}


Hjava_awt_Event*
sun_awt_windows_WComponentPeer_setData(HWComponentPeer *self, 
                                       long data, struct Hjava_awt_Event *event)
{
    CHECK_PEER_AND_RETURN(self, NULL);
    CHECK_OBJECT_AND_RETURN(event, NULL);

    unhand(event)->data = data;

    return event;
}


void 
sun_awt_windows_WComponentPeer_widget_repaint(HWComponentPeer *self, 
                                              long x, long y, long w, long h)
{
    if(unhand(self)->pNativeWidget==NULL)
    {
       return;
    }
    CHECK_PEER(self);
    AwtComponent *obj = GET_OBJ_FROM_PEER(AwtComponent, self);

    ASSERT( obj->GetPeer() );
    AwtToolkit::RaiseJavaEvent(obj->GetPeer(), AwtEventDescription::Repaint, 
                                ll_zero_const, NULL,
                                x,y,w,h );
}


}; // end of extern "C"
