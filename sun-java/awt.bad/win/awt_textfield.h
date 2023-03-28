#ifndef AWT_TEXTFIELD_H
#define AWT_TEXTFIELD_H

#include "awt_defs.h"
#include "awt_textcomponent.h"

DECLARE_SUN_AWT_WINDOWS_PEER(WTextFieldPeer);

class AwtTextField : public AwtTextComponent 
{
public:
                            AwtTextField(HWTextFieldPeer *peer);

            void            SetEchoCharacter(UINT echoChar);

            BOOL            OnKeyDown( UINT virtualCode, UINT scanCode);
    virtual BOOL            OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);

protected:
    //
    // Window class and style of an AwtTextArea window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();
};

#endif

