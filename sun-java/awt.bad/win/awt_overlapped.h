#ifndef AWT_OVERLAPPED_H
#define AWT_OVERLAPPED_H

#include "awt_defs.h"
#include "awt_window.h"


class AwtOverlappedWindow : public AwtWindow
{
public:
                            AwtOverlappedWindow(JHandle *peer);

                            ~AwtOverlappedWindow();

    //
    // Event handlers for Windows events... The return value
    // indicates whether the event was handled.  If FALSE is 
    // returned, then the DefWindowProc(...) will be called.
    //


protected:
    //
    // Window class and style of an AwtOverlapped window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();
    
    //
    // Data members:
    //

    HCURSOR m_hCursor;

private:
    static BOOL m_bIsRegistered;
};


#endif  // AWT_OVERLAPPED_H
