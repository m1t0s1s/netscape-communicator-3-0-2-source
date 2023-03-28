#ifndef AWT_DIALOG_H
#define AWT_DIALOG_H

#include "awt_defs.h"
#include "awt_javawindow.h"


DECLARE_SUN_AWT_WINDOWS_PEER(WDialogPeer);


class AwtDialog : public AwtJavaWindow
{
public:
                            AwtDialog(HWComponentPeer *peer);

    virtual BOOL            OnMove(int newX, int newY);

            void            ShowModal(long spinLoop);
            void            HideModal();
            void            DisableWindows();

protected:
    //
    // Window class and style of an AwtDialog window
    //
    virtual DWORD           WindowStyle();


private:

    BOOL m_bModalLoop;
    BOOL m_bInitiateModal;

    HWND* m_pWindows;
    int m_nWindowsCount;
    HWND m_hForegroundWindow;
};


#endif

