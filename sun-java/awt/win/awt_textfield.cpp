
#include "awt_textfield.h"
#include "awt_toolkit.h"

// All java header files are extern "C"
extern "C" {
#include "java_awt_TextField.h"

#include "sun_awt_windows_WTextComponentPeer.h"
#include "sun_awt_windows_WTextFieldPeer.h"
};


AwtTextField::AwtTextField(HWTextFieldPeer *peer) : AwtTextComponent((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("TXF");
}


// -----------------------------------------------------------------------
//
// Return the window class of an AwtTextArea window.
//
// -----------------------------------------------------------------------
const char * AwtTextField::WindowClass()
{
    return "EDIT";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtButton window
//
// -----------------------------------------------------------------------
DWORD AwtTextField::WindowStyle()
{
    return WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS |
            ES_AUTOHSCROLL;
}


// -----------------------------------------------------------------------
//
// Set the echo (password) character 
//
// -----------------------------------------------------------------------
void AwtTextField::SetEchoCharacter(UINT echoChar)
{
    ASSERT( ::IsWindow(m_hWnd) );
	::SendMessage(m_hWnd, EM_SETPASSWORDCHAR, (WPARAM) echoChar, 0L);	
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtTextField::OnKeyDown( UINT virtualCode, UINT scanCode)
{
    if (m_pJavaPeer) {
        //
        // Call the base class to dispatch the key event
        //
        AwtTextComponent::OnKeyDown(virtualCode, scanCode);

	//
	// If the key is the return-key then dispatch an action 
	// event as well...
	//
	if (virtualCode == VK_RETURN) {
            int64 time = PR_NowMS(); // time in milliseconds
	    WORD asciiKey = '\n';

	    //
            // Get the current state of every key on the keyboard.  This info
            // is necessary for determining if any modifier keys 
	    // (ie. CTRL, SHIFT, etc) are involved...
            //
            ::GetKeyboardState(m_keyState);

            //
            // Call sun.awt.windows.WComponentPeer::handleAction(...)
            //
            AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
			AwtEventDescription::Action,
                        time, GetCurrentMessage(),
                        0, 0, asciiKey, GetKeyModifiers(m_keyState));

        }
        return TRUE;
    }
    
    return FALSE;
}


BOOL AwtTextField::OnSize( UINT sizeCode, UINT newWidth, UINT newHeight )
{
    ::InvalidateRect(m_hWnd, NULL, TRUE);
    return AwtWindow::OnSize(sizeCode, newWidth, newHeight);
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WTextAreaPeer
//
//-------------------------------------------------------------------------

extern "C" {

void 
sun_awt_windows_WTextFieldPeer_create(HWTextFieldPeer* self,
									  HWComponentPeer *hParent)
{
    AwtTextField *pNewTextField;

    CHECK_NULL(self,    "TextFieldPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewTextField = new AwtTextField(self);
    CHECK_ALLOCATION(pNewTextField);

    // REMIND: What about locking the new object??
    pNewTextField->CreateWidget( pNewTextField->GetParentWindowHandle(hParent) );
}


void 
sun_awt_windows_WTextFieldPeer_setEchoCharacter(HWTextFieldPeer* self,
												unicode uChar)
{
    CHECK_PEER(self);

    AwtTextField *obj = GET_OBJ_FROM_PEER(AwtTextField, self);

    obj->SetEchoCharacter(uChar);
}


}; // end of extern "C"

