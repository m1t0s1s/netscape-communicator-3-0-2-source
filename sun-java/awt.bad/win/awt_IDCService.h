
#if !defined(_AWT_IDCSERVICE_H)
#define _AWT_IDCSERVICE_H

#include <windows.h>

class AwtDC;

class IDCService {
public:
                            // return the attached component
    virtual AwtDC*          GetAwtDC()              = 0;
                            // attach and detach from a component
    virtual void            Attach(AwtDC* pDC)      = 0;
    virtual void            Detach(HDC hdc)         = 0;

                            // dc queries. Those function are component specific
    virtual HDC             GetDC()                 = 0;
    virtual void            ReleaseDC(AwtDC* pdc)   = 0;

                            // obtain the background color 
    virtual HBRUSH          GetBackground()         = 0;
    virtual HWND            GetWindowHandle()       = 0;
};

#endif


