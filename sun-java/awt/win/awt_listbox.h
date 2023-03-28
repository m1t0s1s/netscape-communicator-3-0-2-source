#ifndef AWT_LISTBOX_H
#define AWT_LISTBOX_H

#include "awt_defs.h"
#include "awt_window.h"

DECLARE_SUN_AWT_WINDOWS_PEER(WListPeer);

class AwtListbox : public AwtWindow 
{

public:
                            AwtListbox(HWListPeer *peer);
            
            void            SetMultipleSelections(HWListPeer* peer, long bMS);

            void            AddItem(Hjava_lang_String *item, long index);
            void            DeleteItems(long start, long end);

            void            SetSelection(long index, BOOL bSelection);
            void            MakeVisible(long index);
            long            IsSelected(long index);

            BOOL            OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);
            BOOL            OnCommand(UINT notifyCode, UINT id);
            BOOL            OnPaint(HDC hdc, int x, int y, int w, int h);
            BOOL            OnEraseBackground(HDC hdc);

    // a control never want focus explicitly on mouse down, 
    // since windows is taking care of it
    virtual BOOL            WantFocusOnMouseDown() {return FALSE;}
    // capture the mouse if required
    virtual void            CaptureMouse(AwtButtonType button, UINT metakeys);
    // release the mouse if captured
    virtual BOOL            ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y);

#ifdef INTLUNICODE
    virtual BOOL            SetFont(AwtFont* font);
#endif

protected:
    //
    // Window class and style of an AwtListbox window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();
    virtual DWORD           WindowExStyle();

private:
    DWORD   m_dwMultipleSelection;
};


#endif  // AWT_LISTBOX_H
