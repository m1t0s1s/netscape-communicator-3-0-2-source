#ifndef AWT_SCROLLBAR_H
#define AWT_SCROLLBAR_H

#include "awt_defs.h"
#include "awt_window.h"

DECLARE_SUN_AWT_WINDOWS_PEER(WScrollbarPeer);

class AwtScrollbar : public AwtWindow 
{
public:
                            AwtScrollbar(HWScrollbarPeer *peer);

            void            SetPosition(int nPos);
            void            SetRange(int nMin, int nMax);
            void            SetThumbSize(int nVisible);
            void            SetPageIncrement(int nPageScroll);
            void            SetLineIncrement(int nLineScroll);

            void            OnScroll(UINT scrollCode, int cPos);
            BOOL            OnPaint(HDC hdc, int x, int y, int w, int h);

    // a control never want focus explicitly on mouse down, 
    // since windows is taking care of it
    virtual BOOL            WantFocusOnMouseDown() {return FALSE;}
    // capture the mouse if required
    virtual void            CaptureMouse(AwtButtonType button, UINT metakeys);
    // release the mouse if captured
    virtual BOOL            ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y);
            
protected:
    //
    // Window class and style of an AwtScrollbar window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();

private:
    int     m_nPageScrollValue;
    int     m_nLineScrollValue;

};


#endif  // AWT_SCROLLBAR_H
