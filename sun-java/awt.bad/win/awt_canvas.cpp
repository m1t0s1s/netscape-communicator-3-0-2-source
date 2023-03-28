#include "awt_canvas.h"
#include "awt_toolkit.h"

// All java header files are extern "C"
extern "C" {
#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WCanvasPeer.h"
};

AwtCanvas::AwtCanvas(HWCanvasPeer *peer) : AwtChildWindow((JHandle *)peer)
{
    DEFINE_AWT_SENTINAL("CAN");
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WCanvasPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WCanvasPeer_create(HWCanvasPeer *self, HWComponentPeer *hParent)
{
    AwtCanvas    *pNewCanvas;

    CHECK_NULL(self,    "CanvasPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewCanvas = new AwtCanvas(self);
    CHECK_ALLOCATION(pNewCanvas);

// What about locking the new object??
    pNewCanvas->CreateWidget( pNewCanvas->GetParentWindowHandle(hParent) );
}


};  // end of extern "C"


