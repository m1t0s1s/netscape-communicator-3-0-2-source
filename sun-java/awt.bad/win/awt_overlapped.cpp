#include "awt_overlapped.h"


BOOL AwtOverlappedWindow::m_bIsRegistered=FALSE;

AwtOverlappedWindow::AwtOverlappedWindow(JHandle *peer) : AwtWindow(peer)
{
    m_hCursor = 0;
}


AwtOverlappedWindow::~AwtOverlappedWindow()
{
}


// -----------------------------------------------------------------------
//
// Return the window class of an AwtOverlappedWindow window.
// Register the class if necessary...
//
// -----------------------------------------------------------------------
const char * AwtOverlappedWindow::WindowClass()
{
    if (!AwtOverlappedWindow::m_bIsRegistered) {
        WNDCLASS wc;

        //wc.style            = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        //wc.style            = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
#ifdef _WIN32
        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
#else
        wc.style            = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
#endif
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = ::GetModuleHandle(__awt_dllName);
        wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "AwtOverlappedWindowClass";
    
        AwtOverlappedWindow::m_bIsRegistered = ::RegisterClass(&wc);
    }

    m_hCursor = ::LoadCursor(NULL, IDC_ARROW);
    ::SetCursor(m_hCursor);

    return "AwtOverlappedWindowClass";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtOverlappedWindow window
//
// -----------------------------------------------------------------------
DWORD AwtOverlappedWindow::WindowStyle()
{
    return WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
}


