
#include "awt_javawindow.h"
#include "awt_toolkit.h"

#include "resource.rh"

extern "C" {
#include "javastring.h"

#include "java_awt_Insets.h"
#include "java_awt_Component.h"
#include "java_awt_Window.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WContainerPeer.h"
#include "sun_awt_windows_WWindowPeer.h"
};

char* AwtJavaWindow::m_pszWarningString = 0;


AwtJavaWindow::AwtJavaWindow(HWComponentPeer *peer) : AwtContainer((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("JWN");
    m_hStatusWnd = 0;
}


AwtJavaWindow::~AwtJavaWindow()
{
}


// -----------------------------------------------------------------------
//
// Create an AwtJavaWindow. Add a status bar to the normal creation to hold
// "Unsigned Java Applet Window"
//
// This method MUST be executed on the "main gui thread".  Since this
// method is ALWAYS executed on the same thread no locking is necessary.
//
// -----------------------------------------------------------------------
void AwtJavaWindow::CreateWidget(HWND hwndParent)
{
    Hjava_awt_Window *target;

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

    target = (Hjava_awt_Window *)
                unhand((HWComponentPeer *)m_pJavaPeer)->target;

    CHECK_NULL( target, "Target is null." );


    //
    // Create the new window
    //
    m_hWnd = ::CreateWindow(WindowClass(),
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

    // check if a warning string is around ("Unsigned Java Applet")
    Hjava_lang_String* string = unhand(target)->warningString;
    if (string) {
        RECT rc;

        // assign the warning string to a static so that we don't have to
        // reload it for any other applet
        if (!AwtJavaWindow::m_pszWarningString) {
            int32_t len    = javaStringLength(string);
            AwtJavaWindow::m_pszWarningString = (char *)malloc(len+1);
            if (AwtJavaWindow::m_pszWarningString)
                javaString2CString(string, AwtJavaWindow::m_pszWarningString, len+1);
        }

        ::GetClientRect(m_hWnd, &rc);

        //
        // create a status bar 
        //
        m_hStatusWnd = ::CreateWindow("AwtWarningLabelClass",
                                "",
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                0, 
                                rc.bottom - STATUS_BAR_HEIGHT,
                                rc.right,
                                (int) STATUS_BAR_HEIGHT,
                                m_hWnd,
                                NULL,
                                ::GetModuleHandle(NULL),
                                NULL);
    }
    //
    // Subclass the window to allow windows events to be intercepted...
    //
    ASSERT( ::IsWindow(m_hWnd) );
    SubclassWindow(TRUE);
}


DWORD AwtJavaWindow::WindowStyle()
{
    return WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
}


// -----------------------------------------------------------------------
//
// Adjust the size according to the existence of the warning status bar
//
// -----------------------------------------------------------------------
BOOL AwtJavaWindow::OnSize(UINT sizeCode, UINT newWidth, UINT newHeight)
{
    AwtContainer::OnSize(sizeCode, newWidth, newHeight);

    // update information if the warning status bar is there
    if (m_hStatusWnd) {
        RECT rc;
        ::GetClientRect(m_hWnd, &rc);
        ::SetWindowPos(m_hStatusWnd, 
                            HWND_TOP,
                            0, 
                            rc.bottom - STATUS_BAR_HEIGHT, 
                            rc.right, 
                            STATUS_BAR_HEIGHT, 
                            SWP_NOACTIVATE);
    }

    return FALSE;   // Pass the event on to Windows
}


//-------------------------------------------------------------------------
//
// Move update the x and y position for the component and fire a move event
// The (x,y) position is in screen coordinates.
//-------------------------------------------------------------------------
BOOL AwtJavaWindow::OnMove(int newX, int newY)
{
    Hjava_awt_Component *target;
    if (m_pJavaPeer) {
        //
        // Update the width & height in java.awt.Component
        //
        // Since the target's width and height properties are not public
        // they cannot be updated within WComponentPeer::handleResize() 
        //
        target = unhand((HWComponentPeer *)m_pJavaPeer)->target;
        if (target) {
            unhand(target)->x  = newX;
            unhand(target)->y  = newY;
        }

#if 0
        //
        // Call netscape.awt.windows.WComponentPeer::handleResize(...)
        //
        AwtToolkit::RaiseJavaEvent(m_pJavaPeer, AwtEventDescription::Move,
                                   ll_zero_const, NULL,
                                   newX, newY);
#endif
    }
    return FALSE;
}
    
    // -----------------------------------------------------------------------
//
// Allow the AwtJavaWindow to be resized by the user.  Resizable frames have
// a thick border (ie. WS_THICHFRAME) which can be dragged.
//
// -----------------------------------------------------------------------
void AwtJavaWindow::Resizable(BOOL bState)
{
    long style;
    ASSERT( ::IsWindow(m_hWnd) );
   
    // Get the current window style...
    style = ::GetWindowLong(m_hWnd, GWL_STYLE);

    // A window is resizable if the WS_THICKFRAME style is set.
    if (bState) {
        style |= WS_THICKFRAME;
    } else {
        style &= ~WS_THICKFRAME;
    }

    // Set the new window style...
    VERIFY(::SetWindowLong(m_hWnd, GWL_STYLE, style));
}


// -----------------------------------------------------------------------
//
// Move a window at the bottom of the z order
//
// -----------------------------------------------------------------------
void AwtJavaWindow::MoveToBack()
{
    ASSERT( ::IsWindow(m_hWnd) );
    ::SetWindowPos(m_hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}


//-------------------------------------------------------------------------
//
// Adjust insets according to the existence of the warning status bar
//
//-------------------------------------------------------------------------
void AwtJavaWindow::CalculateInsets(Hjava_awt_Insets *insets)
{
    RECT outside;
    RECT inside;

    if (insets) {
        ASSERT( ::IsWindow(m_hWnd ) );
        //
        // Get the inside and outside dimensions of the component
        //
        ::GetWindowRect(m_hWnd, &outside);
        ::GetClientRect(m_hWnd, &inside);

        //
        // Map the inside dimensions from client coordinates to screen
        // coordinates.
        //
        ::MapWindowPoints(m_hWnd, NULL, (LPPOINT)&inside, 2);

        //
        // Calculate the insets for the component...
        //
        unhand(insets)->top    = inside.top     - outside.top;
        unhand(insets)->bottom = outside.bottom - inside.bottom;
        unhand(insets)->left   = inside.left    - outside.left;
        unhand(insets)->right  = outside.right  - inside.right;
    }

    // add the status bar size if we need to
    if (m_hStatusWnd) {
        unhand(insets)->bottom += STATUS_BAR_HEIGHT;
    }

}


//
// native functions
//
extern "C" {

void 
sun_awt_windows_WWindowPeer_create(HWWindowPeer* self,
                                    HWComponentPeer* parent)
{
    AwtJavaWindow    *pNewWindow;

    CHECK_NULL(self, "WindowPeer is null.");
    //CHECK_NULL(parent, "ComponentPeer is null.");

    pNewWindow = new AwtJavaWindow((HWComponentPeer*) self);
    CHECK_ALLOCATION(pNewWindow);

    pNewWindow->CreateWidget( pNewWindow->GetParentWindowHandle(parent) );
}

void 
sun_awt_windows_WWindowPeer_toBack(HWWindowPeer* self)
{
    CHECK_PEER(self);
    AwtJavaWindow* pWindow = GET_OBJ_FROM_PEER(AwtJavaWindow, self);
    if (pWindow)
        pWindow->MoveToBack();
}

};


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//
// Warning status bar window procedure
//
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
LRESULT AWT_CALLBACK StatusFrameProc(HWND hWnd, 
                                     UINT Msg, 
                                     WPARAM WParam, 
                                     LPARAM lParam)
{
    switch (Msg) {

		case WM_ERASEBKGND:
		return TRUE;

        case WM_PAINT: 
        {
            HDC hdc;
            PAINTSTRUCT ps;
			HBRUSH hBkColor;
			RECT rc;
			BITMAP bmp;
			bmp.bmWidth = 0; // zero the field we'll use

            hdc = ::BeginPaint(hWnd, &ps);
			
	        // select the font into the dc
	        HGDIOBJ hOldFont = ::SelectObject(hdc, ::GetStockObject(ANSI_VAR_FONT));

			// fill client area with background color
			::GetClientRect(hWnd, &rc);
			hBkColor = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
			::FillRect(hdc, &rc, hBkColor);

            // load the broken key bitmap (unsigned java applet)
			HBITMAP hbKey = ::LoadBitmap(AwtToolkit::m_hDllInstance, MAKEINTRESOURCE(IDB_INSECURE));
			if (hbKey) {
				HDC hComp = ::CreateCompatibleDC(hdc);
				if (hComp) {

					if (::SelectObject(hComp, hbKey)) {

						// just to be sure the same mode is applied
						::SetMapMode(hComp, ::GetMapMode(hdc));

						// get the bitmap size
						if (::GetObject(hbKey, sizeof(BITMAP), &bmp)) {
							// splash the bitmap on screen
							::BitBlt(hdc, 10, 4, bmp.bmWidth, bmp.bmHeight, hComp, 0, 0, SRCCOPY);
						}
					}

					// delete the compatible dc
					::DeleteDC(hComp);
				}

				// delete the bitmap
				::DeleteObject(hbKey);
			}

            // create pens for 3d status bar look
            HPEN hPen1 = ::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNHIGHLIGHT));
            HPEN hPen2 = ::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNSHADOW));

            if (AwtJavaWindow::m_pszWarningString) {
			    // get pszWarningString length
			    SIZE size;
			    ::GetTextExtentPoint(hdc, AwtJavaWindow::m_pszWarningString, 27, &size);
			    size.cx += size.cx / 4;

                // paint the box
                ::MoveToEx(hdc, 20 + bmp.bmWidth + size.cx, 3, NULL);

                ::SelectObject(hdc, hPen1);
                ::LineTo(hdc, 20 + bmp.bmWidth + size.cx, 19);
                ::LineTo(hdc, 20 + bmp.bmWidth, 19);

                ::SelectObject(hdc, hPen2);
                ::LineTo(hdc, 20 + bmp.bmWidth, 3);
                ::LineTo(hdc, 20 + bmp.bmWidth + size.cx, 3);

			    // paint a sort of 3D border
                ::SelectObject(hdc, hPen1);
                ::MoveToEx(hdc, 0, 0, NULL);
                ::LineTo(hdc, rc.right, 0);

                // paint the text
                ::SetTextColor(hdc, ::GetSysColor(COLOR_BTNTEXT));

			    int nMode = ::SetBkMode(hdc, TRANSPARENT);

                ::TextOut(hdc, 20 + bmp.bmWidth + 3, 4, AwtJavaWindow::m_pszWarningString, 27);

                ::SetBkMode(hdc, nMode);
                ::SelectObject(hdc, hOldFont);
            }

            ::EndPaint(hWnd, &ps);

            // clean up
            ::DeleteObject(hPen1);
            ::DeleteObject(hPen2);
            ::DeleteObject(hBkColor);
            return 0;
        }

    }

    return ::CallWindowProc(::DefWindowProc, hWnd, Msg, WParam, lParam);
}


