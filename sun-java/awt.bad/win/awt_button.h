#ifndef AWT_BUTTON_H
#define AWT_BUTTON_H

#include "awt_defs.h"
#include "awt_window.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif

DECLARE_SUN_AWT_WINDOWS_PEER(WButtonPeer);
DECLARE_SUN_AWT_WINDOWS_PEER(WCheckboxPeer);

class AwtButton : public AwtWindow 
{
public:
                            AwtButton(HWButtonPeer *peer);
                            AwtButton(HWCheckboxPeer *peer);
#ifdef	INTLUNICODE
							~AwtButton();
#endif	// INTLUNICODE
    //
    // Set the label of the AwtButton
    //

            void            SetLabel(Hjava_lang_String *string);
            void            SetState(long lState);

    virtual BOOL            OnCommand(UINT notifyCode, UINT id);
    virtual BOOL            OnPaint(HDC hdc, int x, int y, int w, int h);

#ifdef INTLUNICODE
			BOOL			OnDrawItem(UINT idCtl, LPDRAWITEMSTRUCT lpdis);
			BOOL			OnDeleteItem(UINT idCtl, LPDELETEITEMSTRUCT lpdis);
#endif	// INTLUNICODE

    // a control never want focus explicitly on mouse down, 
    // since windows is taking care of it
    virtual BOOL            WantFocusOnMouseDown() {return FALSE;}
    // capture the mouse if required
    virtual void            CaptureMouse(AwtButtonType button, UINT metakeys);
    // release the mouse if captured
    virtual BOOL            ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y);

protected:
    //
    // Window class and style of an AwtButton window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();

#ifdef INTLUNICODE
			BOOL DrawWin31Item(UINT idCtl, LPDRAWITEMSTRUCT lpdis);
			void DrawWin31Radio(LPDRAWITEMSTRUCT lpdis);
			void DrawWin31Button(LPDRAWITEMSTRUCT lpdis);
			void DrawWin31Checkbox(LPDRAWITEMSTRUCT lpdis);
			void DrawWin31RadioAndCheckboxText(LPDRAWITEMSTRUCT lpdis);
#ifdef _WIN32
			BOOL DrawWin95Item(UINT idCtl, LPDRAWITEMSTRUCT lpdis);
			void DrawWin95Radio(LPDRAWITEMSTRUCT lpdis);
			void DrawWin95Button(LPDRAWITEMSTRUCT lpdis);
			void DrawWin95Checkbox(LPDRAWITEMSTRUCT lpdis);
			void DrawWin95RadioAndCheckboxText(LPDRAWITEMSTRUCT lpdis);
#endif	// _WIN32
#endif	// INTLUNICODE

private:
    DWORD   m_dwButtonType;
#ifdef INTLUNICODE
	BOOL	m_checked;
	WinCompStr	*m_pLabel;
#endif	// INTLUNICODE
};


#endif  // AWT_BUTTON_H
