#ifndef AWT_CANVAS_H
#define AWT_CANVAS_H

#include "awt_defs.h"
#include "awt_child.h"


DECLARE_SUN_AWT_WINDOWS_PEER(WCanvasPeer);
DECLARE_SUN_AWT_WINDOWS_PEER(WPanelPeer);

class AwtCanvas : public AwtChildWindow
{
public:
                            AwtCanvas(HWCanvasPeer *peer);
};

#endif

