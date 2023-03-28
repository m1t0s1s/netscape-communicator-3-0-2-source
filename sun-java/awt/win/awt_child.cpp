#include "awt_child.h"
#include "awt_toolkit.h"


BOOL AwtChildWindow::m_bIsRegistered = FALSE;

AwtChildWindow::AwtChildWindow(JHandle *peer) : AwtWindow(peer)
{
}


// -----------------------------------------------------------------------
//
// Return the window class of an AwtPanel window.
// Register the class if necessary...
//
// -----------------------------------------------------------------------
const char * AwtChildWindow::WindowClass()
{
    if (!AwtChildWindow::m_bIsRegistered) {
        WNDCLASS wc;

#ifdef _WIN32
        wc.style            = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        //wc.style            = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        //wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
#else
        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        //wc.style            = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
#endif
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = ::GetModuleHandle(__awt_dllName);
        wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)::GetStockObject(GRAY_BRUSH);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "AwtChildClass";
    
        AwtChildWindow::m_bIsRegistered = ::RegisterClass(&wc);
    }
    return "AwtChildClass";
    
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtChildWindow
//
// -----------------------------------------------------------------------
DWORD AwtChildWindow::WindowStyle()
{
    return WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
}


//-------------------------------------------------------------------------
//
// On ReleaseDC() don't release the dc because it's private.
//
//-------------------------------------------------------------------------
void AwtChildWindow::ReleaseDC(AwtDC* pdc) 
{
    // OWNDC does nothing. GetDC gets called once
    // we might release the dc on detach.....or never...

    // just for other reason....
    //::ReleaseDC(GetWindowHandle(), pdc->GetDC()); 
    //pdc->ResetOwnerGraphics();
    //pdc->ResetDC();
    //AwtComponent::ReleaseDC(pdc);
}

//-------------------------------------------------------------------------
//
// call ReleaseDC to be a nice guy
//
//-------------------------------------------------------------------------
void AwtChildWindow::Dispose(void)
{
    if (m_pAwtDC) {
        ::ReleaseDC(m_hWnd, m_pAwtDC->GetDC());
        m_pAwtDC->ResetDC(); // zero m_hObject in AwtDC. Should never be used from now on
    }

    // call base
    AwtComponent::Dispose();
}

