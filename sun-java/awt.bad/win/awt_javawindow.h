#ifndef AWT_JAVAWINDOW_H
#define AWT_JAVAWINDOW_H


#include "awt_container.h"


DECLARE_SUN_AWT_WINDOWS_PEER(WWindowPeer);


class AwtJavaWindow : public AwtContainer
{
public:

                            AwtJavaWindow(HWComponentPeer *peer);
                            ~AwtJavaWindow();

    virtual void            CreateWidget(HWND parent);

    virtual BOOL            OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);
    virtual BOOL	    OnMove(int newX, int newY);

    virtual void            Resizable(BOOL bState);

    virtual void            MoveToBack();

    virtual void            CalculateInsets(Hjava_awt_Insets *insets);

protected:

            DWORD           WindowStyle();

protected:

    HWND m_hStatusWnd;    

public:
    
    static char* m_pszWarningString;

};

#endif

