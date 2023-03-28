#include "awt_container.h"
#include "awt_toolkit.h"

#include "resource.rh"

extern "C" {
#include "java_awt_Insets.h"

#include "sun_awt_windows_WContainerPeer.h"
};

LRESULT AWT_CALLBACK StatusFrameProc(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM lParam);

BOOL AwtContainer::m_bIsRegistered = FALSE;


AwtContainer::AwtContainer(JHandle *peer) : AwtWindow(peer)
{
    m_hCursor = 0;
}


AwtContainer::~AwtContainer()
{
}


// -----------------------------------------------------------------------
//
// Return the window class of an AwtContainer
// Register the class if necessary...
//
// -----------------------------------------------------------------------
const char * AwtContainer::WindowClass()
{
    if (!AwtContainer::m_bIsRegistered) {
        WNDCLASS wc;

        // register the generic awt window used by every top level
        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = ::GetModuleHandle(__awt_dllName);
        wc.hIcon            = ::LoadIcon(AwtToolkit::m_hDllInstance, MAKEINTRESOURCE(IDI_JAVACUP));
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "AwtGenericWindowClass";
    
        AwtContainer::m_bIsRegistered = ::RegisterClass(&wc);

        // register the warning status bar
        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc      = ::StatusFrameProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = WindowHInst();
        wc.hIcon            = NULL;
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE+1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "AwtWarningLabelClass";
    
        ::RegisterClass(&wc);

    }

    m_hCursor = ::LoadCursor(NULL, IDC_ARROW);
    ::SetCursor(m_hCursor);

    return "AwtGenericWindowClass";
}


//-------------------------------------------------------------------------
//
// Compute the difference between Window Rect and Client Rect
//
//-------------------------------------------------------------------------
void AwtContainer::CalculateInsets(Hjava_awt_Insets *insets)
{
    // a generic container doesn't compute insets
    // see subclasses for implementation
}


extern "C" {

void
sun_awt_windows_WContainerPeer_calculateInsets(HWContainerPeer *self, 
                                                Hjava_awt_Insets *insets)
{
    CHECK_PEER(self);
    CHECK_OBJECT(insets);

    AwtContainer *obj = GET_OBJ_FROM_PEER(AwtContainer, self);

    obj->CalculateInsets(insets);
}


};


