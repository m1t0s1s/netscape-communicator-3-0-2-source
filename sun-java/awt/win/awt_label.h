#ifndef AWT_LABEL_H
#define AWT_LABEL_H

#include "awt_defs.h"
#include "awt_child.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif 

DECLARE_SUN_AWT_WINDOWS_PEER(WLabelPeer);

class AwtLabel : public AwtWindow
{
public:
                            AwtLabel(HWLabelPeer *peer);
#ifdef INTLUNICODE
                            ~AwtLabel();
#endif
    //
    // Set the label of the AwtLabel
    //

            void            SetText(Hjava_lang_String *string);
            void            SetAlignment(DWORD alignment);

            BOOL            OnPaint(HDC hdc, int x, int y, int w, int h);
            BOOL            SetFont(AwtFont* font);

protected:

    virtual const char*     WindowClass();
    virtual DWORD           WindowStyle();

    DWORD   m_dwAlignment;
#ifdef INTLUNICODE
	WinCompStr	*m_pLabel;
#endif

private:
    static BOOL m_bIsRegistered;

};


#endif  // AWT_LABEL_H
