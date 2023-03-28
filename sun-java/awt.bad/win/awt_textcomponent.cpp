
#include "awt_textcomponent.h"
#include "awt_defs.h"
#ifdef INTLUNICODE
#include "awt_i18n.h"
#endif // INTLUNICODE


// All java header files are extern "C"
extern "C" {
#include "java_awt_Component.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WTextComponentPeer.h"
};


// -----------------------------------------------------------------------
//
// Constructor
//
// -----------------------------------------------------------------------
AwtTextComponent::AwtTextComponent(JHandle *peer) : AwtWindow(peer)
{
    DEFINE_AWT_SENTINAL("TXC");
    hinst = (HINSTANCE) NULL;
}


// -----------------------------------------------------------------------
//
// Destructor
//
// -----------------------------------------------------------------------
AwtTextComponent::~AwtTextComponent()
{
#if 0
    //
    // REMIND:
    // If we free the Edit Control's buffer here, Windows crashes...
    // It must be freed when the EDIT control has been deleted
    //
#if !defined(_WIN32)
    if (hinst != (HINSTANCE) NULL) {
        ::GlobalFree(hinst);
        hinst = NULL;
    }
#endif  // !_WIN32
#endif // 0
}


// -----------------------------------------------------------------------
//
// Return the window ex-style used to create an AwtListbox window
//
// -----------------------------------------------------------------------
DWORD AwtTextComponent::WindowExStyle()
{
#ifdef _WIN32
    return WS_EX_CLIENTEDGE;
#else
    return 0;
#endif
}


// -----------------------------------------------------------------------
//
// Special hInst parameter to ::CreateWindow  
//
// -----------------------------------------------------------------------
HINSTANCE AwtTextComponent::WindowHInst()
{
#ifdef _WIN32
    return ::GetModuleHandle(__awt_dllName);
#else
    // REMIND: The size should be different for TextAreas & TextFields
    //         Maybe the GlobalAlloc should be done in the constructor ??
    if (hinst == (HINSTANCE) NULL) {
        hinst = (HINSTANCE) ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_SHARE, 8192L);
    }
    return hinst;
#endif
}

// -----------------------------------------------------------------------
//
// Toggle whether the TextArea is Read-Only
//
// -----------------------------------------------------------------------
void AwtTextComponent::SetEditable(BOOL bState)
{
    ASSERT( ::IsWindow(m_hWnd) );
    VERIFY( ::SendMessage(m_hWnd, EM_SETREADONLY, (WPARAM) !bState, 0L) );
}


// -----------------------------------------------------------------------
//
// Return a copy of the contents of the edit control.
//
// -----------------------------------------------------------------------
Hjava_lang_String* AwtTextComponent::GetText(void)
{
    int len;
    char *buffer = NULL;
    Hjava_lang_String *string = NULL;

    ASSERT( ::IsWindow(m_hWnd) );

    //
    // Lock the object to prevent other threads from changing the
    // contents of the Edit control..
    //
    Lock();

    len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        ::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
#ifdef INTLUNICODE
		string = (Hjava_lang_String*)libi18n::intl_makeJavaString(GetPrimaryEncoding(), buffer, len);
#else	// INTLUNICODE
		string = makeJavaString(buffer, len);
#endif	// INTLUNICODE
	}

    Unlock();
    return string;
}


// -----------------------------------------------------------------------
//
// Set the edit buffer to the contents of a Java String.
//
// This method does not require locking because no member variables
// are accessed and SendMessage is synchronized already 
//
//
// -----------------------------------------------------------------------
void AwtTextComponent::SetText(Hjava_lang_String *string)
{
    char *buffer;
#ifdef INTLUNICODE
	buffer = libi18n::intl_allocCString(this->GetPrimaryEncoding() , string);
	if(buffer) {
		::SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)buffer);
		free(buffer);
	}
#else	// INTLUNICODE
    long len;

    len    = javaStringLength(string);
    buffer = (char *)malloc(len+1);

    if (buffer) {
        javaString2CString(string, buffer, len+1);

		::SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)buffer);

		free(buffer);
	}
#endif	// INTLUNICODE
}


// -----------------------------------------------------------------------
//
// Return the starting position of the selected text in the Edit buffer...
//
// This method does not require locking because no member variables
// are accessed!.  The SendMessage(...) synchronizes the access to the 
// edit buffer.
//
// -----------------------------------------------------------------------
long AwtTextComponent::GetSelectionStart(void)
{
    long selStart;

    ASSERT( ::IsWindow(m_hWnd) );

#ifdef INTLUNICODE
    Lock();
#endif

#ifdef _WIN32
    /*selStart =*/ ::SendMessage(m_hWnd, EM_GETSEL, (WPARAM)&selStart, NULL);
#else
    selStart = ::SendMessage(m_hWnd, EM_GETSEL, 0, 0L);
    selStart = LOWORD(selStart);
#endif

#ifdef INTLUNICODE
//
//	For Unicode support, we need to convert the selStart which EM_GETSEL return from
//	byte count to character length
//
	if(selStart > 0)
	{
		int len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
		unsigned char *buffer = NULL;
		if(len > 0)
			buffer = (unsigned char *)malloc(len+1);
		if (buffer) {
			::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
			selStart = libi18n::INTL_TextByteCountToCharLen(this->GetPrimaryEncoding(), 
						buffer, selStart);
			free(buffer);
		}
	}
    Unlock();
#endif

    return selStart;
}


// -----------------------------------------------------------------------
//
// Return the ending position of the selected text in the Edit buffer...
//
// This method does not require locking because no member variables
// are accessed!.  The SendMessage(...) synchronizes the access to the 
// edit buffer.
//
// -----------------------------------------------------------------------
long AwtTextComponent::GetSelectionEnd(void)
{
    long selEnd;

    ASSERT( ::IsWindow(m_hWnd) );

#ifdef INTLUNICODE
    Lock();
#endif

#ifdef _WIN32
    /*selEnd =*/ ::SendMessage(m_hWnd, EM_GETSEL, NULL, (LPARAM)&selEnd);
#else
    selEnd = ::SendMessage(m_hWnd, EM_GETSEL, 0, 0L);
    selEnd = HIWORD(selEnd);
#endif

#ifdef INTLUNICODE
//
//	For Unicode support, we need to convert the selEnd which EM_GETSEL return from
//	byte count to character length
//
	if(selEnd > 0)
	{
		int len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
		unsigned char* buffer = NULL;
		if(len > 0)
			buffer = (unsigned char *)malloc(len+1);
		if (buffer) {
			::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
			selEnd = libi18n::INTL_TextByteCountToCharLen(this->GetPrimaryEncoding(), 
						buffer, selEnd);
			free(buffer);
		}
	}
    Unlock();
#endif

    return selEnd;
}


// -----------------------------------------------------------------------
//
// Select the specified range of text in the edit control...
//
// -----------------------------------------------------------------------
void AwtTextComponent::SelectText(long start, long end)
{
    ASSERT( ::IsWindow(m_hWnd) );

#ifdef INTLUNICODE
//
//	For Unicode support, we need to convert the selEnd which EM_GETSEL return from
//	byte count to character length
//
	Lock();
    int len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
    unsigned char * buffer = NULL;
	if(len > 0)
		buffer = (unsigned char *)malloc(len+1);
    if (buffer) {
        ::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
		int charLen = libi18n::INTL_TextByteCountToCharLen(this->GetPrimaryEncoding(), 
					buffer, len);
		if((start > 0) && (start <= charLen))
		{
			start = libi18n::INTL_TextCharLenToByteCount(this->GetPrimaryEncoding(), 
					buffer, start);
		}
		if((end > 0) && (end <= charLen))
		{
			end = libi18n::INTL_TextCharLenToByteCount(this->GetPrimaryEncoding(), 
					buffer, end);
		}
		free(buffer);
	}

    Unlock();
#endif


    //
    // Select the specified range of text in the Edit control
    //
#ifdef _WIN32
    ::SendMessage(m_hWnd, EM_SETSEL, (WPARAM)start, (LPARAM)end);
#else
    ::SendMessage(m_hWnd, EM_SETSEL, (WPARAM)1, MAKELONG((int)start, (int)end));
#endif
}


BOOL AwtTextComponent::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                                
    return AwtComponent::OnPaint(hdc, x, y, w, h);
}
    

BOOL AwtTextComponent::OnEraseBackground( HDC hdc )
{
    //return TRUE;
    return FALSE;
}


//-------------------------------------------------------------------------
//
// Capture the mouse (if it's not the left button) so that mouse events 
// get re-direct to the proper component. 
// This allow MouseDrag to work correctly and MouseUp to be sent to the 
// component which received the mouse down.
// Left down is a special case becouse windows needs to do that.
//
//-------------------------------------------------------------------------
void AwtTextComponent::CaptureMouse(AwtButtonType button, UINT metakeys)
{
    // capture the mouse if it was not captured already.
    // This may happen because of a previous call to CaptureMouse
    // or because the left mouse was clicked and windows captured
    // the mouse for us.
    // Here we use m_nCaptured with a different semantic.
    // It's setted to 1 if SetCapture gets called, otherwise is 0
    if (button == AwtLeftButton)
        m_nCaptured = 0;
    else if (!(metakeys & MK_LBUTTON) && !m_nCaptured) {
        ::SetCapture(m_hWnd);
        m_nCaptured = 1;
    }
}


//-------------------------------------------------------------------------
//
// Release the mouse if the counter reaches zero. We need a counter so
// that if a second button get pressed while the other is pressed already
// we are still able to send MouseUp to the right component
//
//-------------------------------------------------------------------------
BOOL AwtTextComponent::ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y)
{
    // if at least the left button is pressed don't bother...
    if (!(metakeys & MK_LBUTTON)) {

        // check if no button is pressed and if we called SetCapture release it
        if (!(metakeys & MK_RBUTTON) && !(metakeys & MK_MBUTTON)) {
            if (m_nCaptured) {
                ::ReleaseCapture();
                m_nCaptured = 0;
            }
        }

        // the left button has just been released...
        else if (button == AwtLeftButton) {
            // ...see if there is any other button pressed...
            if ((metakeys & MK_RBUTTON) || (metakeys & MK_MBUTTON)) {
                    // Windows is giving up the mouse capture, it's time to take it. 
                    // Let the defproc do MouseUp first so it will release the mouse 
                    // before we capture otherwise WM_LBUTTONUP will release our capture
                    ::CallWindowProc(m_PrevWndProc, 
                                        m_hWnd, 
                                        WM_LBUTTONUP, 
                                        (WPARAM)metakeys, 
                                        MAKELPARAM(x, y));
                    ::SetCapture(m_hWnd);
                    m_nCaptured = 1;
                    return TRUE;
            }
        }

    }

    return FALSE;
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WTextAreaPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WTextComponentPeer_widget_setEditable(HWTextComponentPeer *self, 
													long flag)
{
    CHECK_PEER(self);

    AwtTextComponent *obj = GET_OBJ_FROM_PEER(AwtTextComponent, self);

    obj->SetEditable((BOOL)flag);
}


void
sun_awt_windows_WTextComponentPeer_select(HWTextComponentPeer *self, 
										long start, long end)
{
    CHECK_PEER(self);

    AwtTextComponent *obj = GET_OBJ_FROM_PEER(AwtTextComponent, self);
    
    obj->SelectText(start, end);
}


long
sun_awt_windows_WTextComponentPeer_getSelectionStart(HWTextComponentPeer *self)
{
    CHECK_PEER_AND_RETURN(self, 0);

    AwtTextComponent *obj = GET_OBJ_FROM_PEER(AwtTextComponent, self);
    
    return obj->GetSelectionStart();
}


long
sun_awt_windows_WTextComponentPeer_getSelectionEnd(HWTextComponentPeer *self)
{
    CHECK_PEER_AND_RETURN(self, 0);

    AwtTextComponent *obj = GET_OBJ_FROM_PEER(AwtTextComponent, self);

    return obj->GetSelectionEnd();
}


void
sun_awt_windows_WTextComponentPeer_setText(HWTextComponentPeer *self, 
                                      Hjava_lang_String *string)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtTextComponent *obj = GET_OBJ_FROM_PEER(AwtTextComponent, self);

    obj->SetText(string);
}


Hjava_lang_String *
sun_awt_windows_WTextComponentPeer_getText(HWTextComponentPeer *self)
{
    CHECK_PEER_AND_RETURN(self, NULL);

    Hjava_lang_String *string;
    AwtTextComponent *obj = GET_OBJ_FROM_PEER(AwtTextComponent, self);

    string = obj->GetText();
    return string;
}

}; // end of extern "C"

