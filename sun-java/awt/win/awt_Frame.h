#ifndef AWT_FRAME_H
#define AWT_FRAME_H

#include "awt_defs.h"
#include "awt_javawindow.h"
#include "awt_menu.h"


DECLARE_SUN_AWT_WINDOWS_PEER(WFramePeer);


class AwtFrame : public AwtJavaWindow
{
public:
                            AwtFrame(HWComponentPeer *peer);

            void            SetMenuBar(AwtMenuBar* menuBar);

            void            SetCursor(long lCursorID);
    virtual BOOL            OnSetCursor(HWND hCursorWnd, WORD wHittest, WORD wMouseMsg);
    virtual BOOL            OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);

    virtual BOOL            OnSelect(UINT id);

protected:
    //
    // Window class and style of an AwtEmbeddedFrame window
    //
    virtual DWORD           WindowStyle();


private:

    AwtMenuBar* m_MenuBar;
};


class AwtEmbeddedFrame : public AwtFrame
{
public:
                            AwtEmbeddedFrame(HWComponentPeer *peer);

    virtual void            CreateWidget(HWND parent);
    
    virtual BOOL            OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);
    virtual void            Resizable(BOOL bState);

protected:
    //
    // Window class and style of an AwtEmbeddedFrame window
    //
    virtual DWORD           WindowStyle();

};


#endif  // AWT_FRAME_H
