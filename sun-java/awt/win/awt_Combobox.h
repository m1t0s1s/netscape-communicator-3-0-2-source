#ifndef AWT_COMBOBOX_H
#define AWT_COMBOBOX_H

#include "awt_defs.h"
#include "awt_window.h"

DECLARE_SUN_AWT_WINDOWS_PEER(WChoicePeer);

class AwtCombobox : public AwtWindow 
{
public:
                            AwtCombobox(HWChoicePeer *peer);


			void			AddItem(Hjava_lang_String * string, long lIndex);
			void			Select(long lIndex);
			void			DeleteItem(long lIndex);
            void            AdjustSize(int nHeight);

            BOOL            OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);
            BOOL            OnCommand(UINT notifyCode, UINT id);

            BOOL            OnMouseDown    (AwtButtonType button, UINT metakeys, 
                                            int x, int y);
            BOOL            OnMouseUp      (AwtButtonType button, UINT metakeys,
                                            int x, int y);
            BOOL            OnPaint(HDC hdc, int x, int y, int w, int h);

    // a control never want focus explicitly on mouse down, 
    // since windows is taking care of it
    virtual BOOL            WantFocusOnMouseDown() {return FALSE;}
    // capture the mouse if required
    virtual void            CaptureMouse(AwtButtonType button, UINT metakeys) {}
    // release the mouse if captured
    virtual BOOL            ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y) {return FALSE;}

#ifdef INTLUNICODE
    virtual BOOL            SetFont(AwtFont* font);
#endif

protected:
    //
    // Window class and style of an AwtLabel window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();
    virtual DWORD           WindowExStyle();

};


#endif  // AWT_COMBOBOX_H
