#ifndef AWT_PANEL_H
#define AWT_PANEL_H

#include "awt_container.h"


DECLARE_SUN_AWT_WINDOWS_PEER(WPanelPeer);


class AwtPanel : public AwtContainer
{
public:

                            AwtPanel(HWPanelPeer *peer);

            void            SetWindowStyle(DWORD style) { m_dwWindowStyle = style; }

protected:

    virtual DWORD           WindowStyle();

private:

    DWORD m_dwWindowStyle;
};

#endif


