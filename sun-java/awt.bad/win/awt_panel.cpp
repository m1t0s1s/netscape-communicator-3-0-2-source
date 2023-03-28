
#include "awt_panel.h"

extern "C" {

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WPanelPeer.h"
};


AwtPanel::AwtPanel(HWPanelPeer *peer) : AwtContainer((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("PAN");
    m_dwWindowStyle = WS_CHILD;
}


// -----------------------------------------------------------------------
//
// Return the window style used to create a java Panel window
//
// -----------------------------------------------------------------------
DWORD AwtPanel::WindowStyle()
{
    return m_dwWindowStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS; 
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WPanelPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WPanelPeer_create(HWPanelPeer *self, HWComponentPeer *hParent)
{
    AwtPanel *pNewPanel;

    CHECK_NULL(self,    "PanelPeer is null.");
    //CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewPanel = new AwtPanel(self);
    CHECK_ALLOCATION(pNewPanel);

    // if no parent it's an overlapped
    if (!hParent)
        pNewPanel->SetWindowStyle(WS_POPUP | WS_BORDER);


// What about locking the new object??
    pNewPanel->CreateWidget( pNewPanel->GetParentWindowHandle(hParent) );
}


};  // end of extern "C"
