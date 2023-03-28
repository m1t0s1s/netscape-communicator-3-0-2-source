#ifndef AWT_TEXTCOMPONENT_H
#define AWT_TEXTCOMPONENT_H

#include "awt_defs.h"
#include "awt_window.h"

DECLARE_SUN_AWT_WINDOWS_PEER(WTextComponentPeer);

class AwtTextComponent : public AwtWindow 
{
public:
                                AwtTextComponent(JHandle *peer);
                                ~AwtTextComponent();

            void                SetEditable(BOOL bState);

    virtual Hjava_lang_String * GetText(void);
    virtual void		SetText(Hjava_lang_String *string);

    virtual long                GetSelectionStart(void);
    virtual long                GetSelectionEnd(void);
    virtual void                SelectText(long start, long end);

            BOOL                OnPaint(HDC hdc, int x, int y, int w, int h);
            BOOL                OnEraseBackground(HDC hdc);

    // a control never want focus explicitly on mouse down, 
    // since windows is taking care of it
    virtual BOOL                WantFocusOnMouseDown() {return FALSE;}
    // capture the mouse if required
    virtual void                CaptureMouse(AwtButtonType button, UINT metakeys);
    // release the mouse if captured
    virtual BOOL                ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y);

protected:
    virtual HINSTANCE		WindowHInst(); // Override 

    virtual DWORD           WindowExStyle();

private:
    HINSTANCE hinst;
};

#endif

