#ifndef AWT_CHILD_H
#define AWT_CHILD_H

#include "awt_defs.h"
#include "awt_window.h"


class AwtChildWindow : public AwtWindow
{
public:
                            AwtChildWindow(JHandle *peer);

    virtual void            Dispose(void);

    // don't need to release the dc since we are using CS_OWNDC.
    // One fresh dc will be allocated any window of this class. 
    virtual void            ReleaseDC(AwtDC* pdc);
    virtual void            Detach(HDC hdc) { ::ReleaseDC(m_hWnd, hdc); } // final release (OWNDC)

protected:
    //
    // Window class and style of an AwtPanel window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();
    
    //
    // Data members:
    //
private:
    static BOOL m_bIsRegistered;
};


#endif  // AWT_CHILD_H
