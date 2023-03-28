#include "awt_scrollbar.h"
#include "awt_toolkit.h"

// All java header files are extern "C"
extern "C" {
#include "java_awt_Scrollbar.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WScrollbarPeer.h"
};


AwtScrollbar::AwtScrollbar(HWScrollbarPeer *peer) : AwtWindow((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("BAR");
    m_nPageScrollValue = 10;
    m_nLineScrollValue = 1;
}

// -----------------------------------------------------------------------
//
// Return the window class of an AwtScrollbar window.
//
// -----------------------------------------------------------------------
const char * AwtScrollbar::WindowClass()
{
    return "SCROLLBAR";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtScrollbar window
//
// -----------------------------------------------------------------------
DWORD AwtScrollbar::WindowStyle()
{
    DWORD dwStyles = WS_CHILD | WS_CLIPSIBLINGS;

	// get hand on the checkbox object (via peer)
    Hjava_awt_Scrollbar *target;
    target = (Hjava_awt_Scrollbar *)
                unhand((HWComponentPeer *)m_pJavaPeer)->target;

    // find out orientation
    if (unhand(target)->orientation == java_awt_Scrollbar_HORIZONTAL)
        dwStyles |= SBS_HORZ;
    else
        dwStyles |= SBS_VERT;

    return dwStyles;
}


// -----------------------------------------------------------------------
//
// Set the thumb position
//
// -----------------------------------------------------------------------
void AwtScrollbar::SetPosition(int nPos)
{
    ASSERT(m_hWnd);

    Lock();

    ::SetScrollPos(m_hWnd, SB_CTL, nPos, TRUE);

    Unlock();
}


// -----------------------------------------------------------------------
//
// Set scrollbar range
//
// -----------------------------------------------------------------------
void AwtScrollbar::SetRange(int nMin, int nMax)
{
    ASSERT(m_hWnd);

    Lock();

    ::SetScrollRange(m_hWnd, SB_CTL, nMin, nMax, TRUE);

    Unlock();
}


// -----------------------------------------------------------------------
//
// Set thumb size.
// This function is not implemented at this time.
//
// -----------------------------------------------------------------------
void AwtScrollbar::SetThumbSize(int nVisible)
{
    //Lock();

    //m_nPageScrollValue = nVisible;

    //Unlock();
}


// -----------------------------------------------------------------------
//
// Set the number of units to scroll for a page down or a page up
//
// -----------------------------------------------------------------------
void AwtScrollbar::SetPageIncrement(int nPageScroll)
{
    Lock();

    m_nPageScrollValue = nPageScroll;

    Unlock();
}


// -----------------------------------------------------------------------
//
// Handling WM_VSCROLL (WM_HSCROLL) to make the scroll work
//
// -----------------------------------------------------------------------
void AwtScrollbar::OnScroll(UINT scrollCode, int cPos)
{

    //Lock();

    switch (scrollCode) {

        // scroll one line right or down
        // SB_LINERIGHT and SB_LINEDOWN are actually the same value
        //case SB_LINERIGHT: 
        case SB_LINEDOWN: {
            
            int newPosition = GetScrollPos(m_hWnd, SB_CTL) + m_nLineScrollValue;

		    //SetScrollPos(m_hWnd, 
            //                SB_CTL, 
            //                newPosition, 
            //                TRUE);

            if (m_pJavaPeer) {
                int64 time = PR_NowMS(); // time in milliseconds

                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::LineDown,
                                            time, 
                                            NULL,
                                            newPosition);
            }

	        break;
        }

        // scroll one line left or up
	    //case SB_LINELEFT:
	    case SB_LINEUP: {
            
            int newPosition = GetScrollPos(m_hWnd, SB_CTL) - m_nLineScrollValue;

		    //SetScrollPos(m_hWnd, 
            //                SB_CTL, 
            //                newPosition, 
            //                TRUE);

            if (m_pJavaPeer) {
                int64 time = PR_NowMS(); // time in milliseconds

                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::LineUp,
                                            time, 
                                            NULL,
                                            newPosition);
            }

            break;
        }

        // Scrolls one page down.
	    //case SB_PAGERIGHT:  
	    case SB_PAGEDOWN: {
            
            int newPosition = GetScrollPos(m_hWnd, SB_CTL) + m_nPageScrollValue;

		    //SetScrollPos(m_hWnd, 
			//			    SB_CTL, 
		    //			    newPosition, 
			//			    TRUE);

            if (m_pJavaPeer) {
                int64 time = PR_NowMS(); // time in milliseconds

                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::PageDown,
                                            time, 
                                            NULL,
                                            newPosition);
            }

	        break;
        }

        // Scrolls one page up.
        //case SB_PAGELEFT:
	    case SB_PAGEUP: {
            
            int newPosition = GetScrollPos(m_hWnd, SB_CTL) - m_nPageScrollValue;

		    //SetScrollPos(m_hWnd, 
			//			    SB_CTL, 
			//			    newPosition, 
			//			    TRUE);

            if (m_pJavaPeer) {
                int64 time = PR_NowMS(); // time in milliseconds

                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::PageUp,
                                            time, 
                                            NULL,
                                            newPosition);
            }

	        break;
        }

	    // Scrolls to the absolute position. The current position is specified by 
        // the cPos parameter.
        //case SB_THUMBPOSITION: 
        case SB_THUMBTRACK: {
            
		    //SetScrollPos(m_hWnd, 
            //                SB_CTL, 
            //                cPos, 
            //                TRUE);

            if (m_pJavaPeer) {
                int64 time = PR_NowMS(); // time in milliseconds

                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::DragAbsolute,
                                            time, 
                                            NULL,
                                            cPos);
            }

	        break;
        }
    }

    //Unlock();

}


BOOL AwtScrollbar::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                                
    return AwtComponent::OnPaint(hdc, x, y, w, h);
}
    
    
// -----------------------------------------------------------------------
//
// Set the number of units to scroll for a line down or a line up
//
// -----------------------------------------------------------------------
void AwtScrollbar::SetLineIncrement(int nLineScroll)
{
    Lock();

    m_nLineScrollValue = nLineScroll;

    Unlock();
}


//
// WARNING - WARNING - WARNING - WARNING - WARNING 
//
// scrollbars eat WM_LBUTTONUP so the current implementation of 
// CaptureMouse ReleaseMouse won't work.
// We need a workaround for this

//-------------------------------------------------------------------------
//
// Capture the mouse (if it's not the left button) so that mouse events 
// get re-direct to the proper component. 
// This allow MouseDrag to work correctly and MouseUp to be sent to the 
// component which received the mouse down.
// Left down is a special case because windows needs to handle that.
//
//-------------------------------------------------------------------------
void AwtScrollbar::CaptureMouse(AwtButtonType button, UINT metakeys)
{
    // capture the mouse if it was not captured already.
    // This may happen because of a previous call to CaptureMouse
    // or because the left mouse was clicked and windows captured
    // the mouse for us.
    // Here we use m_nCaptured with a different semantic.
    // It's setted to 1 if SetCapture gets called, otherwise is 0
#if 0
    if (button == AwtLeftButton)
        m_nCaptured = 0;
    else if (!(metakeys & MK_LBUTTON) && !m_nCaptured) {
        ::SetCapture(m_hWnd);
        m_nCaptured = 1;
    }
#endif
}


//-------------------------------------------------------------------------
//
// Release the mouse if it was captured previously. We might not have 
// captured the mouse at all if the left button is the first to be pressed
// and the last released
//
//-------------------------------------------------------------------------
BOOL AwtScrollbar::ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y)
{
#if 0
    // if at least the left button is pressed don't bother...
    if (!(metakeys & MK_LBUTTON)) {

        // check if no button is pressed and if we called SetCapture release it
        if (!(metakeys & MK_RBUTTON) && !(metakeys & MK_MBUTTON)) {
            if (m_nCaptured) {
                ::ReleaseCapture();
                m_nCaptured = 0;
            }
        }

        // the left button has just been released...
        else if (button == AwtLeftButton) {
            // ...see if there is any other button pressed...
            if ((metakeys & MK_RBUTTON) || (metakeys & MK_MBUTTON)) {
                    // Windows is giving up the mouse capture, it's time to take it. 
                    // Let the defproc do MouseUp first so it will release the mouse 
                    // before we capture otherwise WM_LBUTTONUP will release our capture
                    ::CallWindowProc(m_PrevWndProc, 
                                        m_hWnd, 
                                        WM_LBUTTONUP, 
                                        (WPARAM)metakeys, 
                                        MAKELPARAM(x, y));
                    ::SetCapture(m_hWnd);
                    m_nCaptured = 1;
                    return TRUE;
            }
        }

    }
#endif

    return FALSE;
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WScrollbarPeer
//
//-------------------------------------------------------------------------

extern "C" {

void 
sun_awt_windows_WScrollbarPeer_pCreate(HWScrollbarPeer* self,
                                       HWComponentPeer * hParent)
{
    AwtScrollbar *pNewScrollbar;

    CHECK_NULL(self,    "ScrollbarPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewScrollbar = new AwtScrollbar(self);
    CHECK_ALLOCATION(pNewScrollbar);

// What about locking the new object??
    pNewScrollbar->CreateWidget( pNewScrollbar->GetParentWindowHandle(hParent));
}


void 
sun_awt_windows_WScrollbarPeer_setValue(HWScrollbarPeer* self,
                                        long lPos)
{
    CHECK_PEER(self);

    AwtScrollbar *obj = GET_OBJ_FROM_PEER(AwtScrollbar, self);

    obj->SetPosition(lPos);
}


void 
sun_awt_windows_WScrollbarPeer_setValues(HWScrollbarPeer* self,
                                         long lPos,
                                         long lThumbSize,
                                         long lMin,
                                         long lMax)
{
    CHECK_PEER(self);

    AwtScrollbar *obj = GET_OBJ_FROM_PEER(AwtScrollbar, self);

    obj->Lock();

    obj->SetRange(lMin, lMax);
    obj->SetPosition(lPos);
    obj->SetThumbSize(lThumbSize);

    obj->Unlock();
}


void 
sun_awt_windows_WScrollbarPeer_setLineIncrement(HWScrollbarPeer* self,
                                                long lLineValue)
{
    CHECK_PEER(self);

    AwtScrollbar *obj = GET_OBJ_FROM_PEER(AwtScrollbar, self);

    obj->SetLineIncrement(lLineValue);
}


void 
sun_awt_windows_WScrollbarPeer_setPageIncrement(HWScrollbarPeer* self,
                                                long lPageValue)
{
    CHECK_PEER(self);

    AwtScrollbar *obj = GET_OBJ_FROM_PEER(AwtScrollbar, self);

    obj->SetPageIncrement(lPageValue);
}


};  // end of extern "C"
