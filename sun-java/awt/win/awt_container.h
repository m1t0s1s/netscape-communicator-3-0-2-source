#ifndef AWT_CONTAINER_H
#define AWT_CONTAINER_H

#include "awt_defs.h"
#include "awt_window.h"

#define STATUS_BAR_HEIGHT		21


DECLARE_SUN_AWT_WINDOWS_PEER(WContainerPeer);


class AwtContainer : public AwtWindow
{
public:
                            AwtContainer(JHandle *peer);
                            ~AwtContainer();

    virtual void            CalculateInsets(Hjava_awt_Insets *insets);

protected:
    //
    // Window class and style of an AwtContainer window
    //
    virtual const char *    WindowClass();

protected:
    //
    // Data members:
    //
    HCURSOR m_hCursor;

private:

    static BOOL m_bIsRegistered;
};


#endif  // AWT_CONTAINER_H
