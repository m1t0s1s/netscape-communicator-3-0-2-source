#include "awt_label.h"
#include "awt_toolkit.h"
#include "awt_font.h"
#include "awt_resource.h"
#include "awt_defs.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif

// All java header files are extern "C"
extern "C" {
#include "java_awt_Label.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WLabelPeer.h"
};


BOOL AwtLabel::m_bIsRegistered = FALSE;


AwtLabel::AwtLabel(HWLabelPeer *peer) : AwtWindow((JHandle *)peer)
{
    DEFINE_AWT_SENTINAL("LBL");
    m_dwAlignment = SS_LEFT;
#ifdef INTLUNICODE
	m_pLabel = NULL;
#endif	// INTLUNICODE
}

#ifdef INTLUNICODE
AwtLabel::~AwtLabel()
{
	if(m_pLabel)
		delete m_pLabel;
}
#endif // INTLUNICODE


// -----------------------------------------------------------------------
//
// Set the label text for the Label
//
// -----------------------------------------------------------------------
void AwtLabel::SetText(Hjava_lang_String *string)
{
    char *buffer = NULL;

    ASSERT( ::IsWindow(m_hWnd) );

#ifdef INTLUNICODE
	if(m_pLabel)
	{
		delete m_pLabel;
		m_pLabel = NULL;
	}
	m_pLabel = new WinCompStr(string);
	::SendMessage(m_hWnd, WM_SETTEXT, 0 , (LPARAM)"");
	::InvalidateRect(m_hWnd, NULL, TRUE);
#else	// INTLUNICODE
    int32_t len;
    len = javaStringLength(string);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(string, buffer, len+1);

        ::SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)buffer);
        ::InvalidateRect(m_hWnd, NULL, TRUE);

        free(buffer);
    }
#endif	// INTLUNICODE
}


// -----------------------------------------------------------------------
//
// Set the alignment for the Label. This is actually done changing
// the windows style using SetWindowLong
//
// -----------------------------------------------------------------------
void AwtLabel::SetAlignment(DWORD alignment)
{
    Lock();

	// save the new alignment if different
    if (m_dwAlignment != alignment) {

        m_dwAlignment = alignment;

        if (::IsWindow(m_hWnd)) {
            // let the new alignment be visible
            ::InvalidateRect(m_hWnd, NULL, TRUE);
        }
    }

    Unlock();
}


// -----------------------------------------------------------------------
//
// Repaint the text with the proper alignment
//
// -----------------------------------------------------------------------
BOOL AwtLabel::OnPaint(HDC hdc, int x, int y, int w, int h)
{
    PAINTSTRUCT ps;
    RECT rc;

    HDC hDC = ::BeginPaint(m_hWnd, &ps);
    
    ::SetBkColor(hDC, m_BackgroundColor->GetColor());
    ::SetTextColor(hDC, m_ForegroundColor->GetColor());

    // in general we should select the old font back into the dc
    // but we are always pushing the object instance value into the dc
    // when we use it, so....
    if (m_pFont)
        ::SelectObject(hDC, m_pFont->GetFont());

    ::GetClientRect(m_hWnd, &rc);
    ::FillRect(hDC, &rc, m_BackgroundColor->GetBrush()); 

#ifdef INTLUNICODE 
	AwtFont* pFont = this->GetFont();
	if(pFont != NULL)
		m_pLabel->DrawText(hDC, pFont, &rc, m_dwAlignment);
#else	// INTLUNICODE
    long lCaptionSize;
    char* pszCaption;
    lCaptionSize = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0L);
    pszCaption = new char[lCaptionSize + 1];
    ::SendMessage(m_hWnd, WM_GETTEXT, lCaptionSize + 1, (LPARAM)pszCaption);

    ::DrawText(hDC,
                pszCaption,
                -1,
                &rc,
                m_dwAlignment | DT_SINGLELINE | DT_VCENTER);
#endif	// INTLUNICODE

    ::EndPaint(m_hWnd, &ps);

#ifndef INTLUNICODE 
    delete[] pszCaption;
#endif	// not INTLUNICODE

    return TRUE;
}


// -----------------------------------------------------------------------
//
// Set the font for the window...
//
// -----------------------------------------------------------------------
BOOL AwtLabel::SetFont(AwtFont* font)
{
    BOOL bStatus;

    bStatus = AwtComponent::SetFont(font);
    if (font && bStatus) {
        //::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)font->GetFont(), 
        //              MAKELPARAM(TRUE, 0));
        // apparently if the window is not a control a repaint does not occur
        // and, more in general, SETFONT seems to be useless, so let's invalidate
        // and do all the work in paint
        ::InvalidateRect(m_hWnd, NULL, TRUE);
    }

    return bStatus;
}


// -----------------------------------------------------------------------
//
// Return the window class of an AwtPanel window.
// Register the class if necessary...
//
// -----------------------------------------------------------------------
const char * AwtLabel::WindowClass()
{
    if (!AwtLabel::m_bIsRegistered) {
        WNDCLASS wc;

        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = ::GetModuleHandle(__awt_dllName);
        wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)::GetStockObject(GRAY_BRUSH);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "AwtLabelClass";
    
        AwtLabel::m_bIsRegistered = ::RegisterClass(&wc);
    }

    return "AwtLabelClass";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create a label
//
// -----------------------------------------------------------------------
DWORD AwtLabel::WindowStyle()
{
    return WS_CHILD | WS_CLIPSIBLINGS;
}

//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WLabelPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WLabelPeer_create(HWLabelPeer *self, 
                                  HWComponentPeer *hParent)
{
    AwtLabel *pNewLabel;

    CHECK_NULL(self,    "LabelPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewLabel = new AwtLabel(self);
    CHECK_ALLOCATION(pNewLabel);

// What about locking the new object??
    pNewLabel->CreateWidget( pNewLabel->GetParentWindowHandle(hParent) );
}


void
sun_awt_windows_WLabelPeer_setText(HWLabelPeer *self, 
                                     Hjava_lang_String *string)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtLabel *obj = GET_OBJ_FROM_PEER(AwtLabel, self);
    obj->SetText(string);
}


void 
sun_awt_windows_WLabelPeer_setAlignment(HWLabelPeer* self,
                                        long alignment)
{
    DWORD dwWAlignment;
    CHECK_PEER(self);

    AwtLabel *obj = GET_OBJ_FROM_PEER(AwtLabel, self);
	
    switch (alignment) {
      case java_awt_Label_LEFT:
    	//dwWAlignment = SS_LEFT;
    	dwWAlignment = DT_LEFT;
        break;

      case java_awt_Label_CENTER:
	    //dwWAlignment = SS_CENTER;
	    dwWAlignment = DT_CENTER;
        break;

      case java_awt_Label_RIGHT:
	    //dwWAlignment = SS_RIGHT;
	    dwWAlignment = DT_RIGHT;
        break;
    }
	
    obj->SetAlignment(dwWAlignment);

}

};  // end of extern "C"
