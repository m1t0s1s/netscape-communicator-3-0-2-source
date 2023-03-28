#include "awt_Combobox.h"
#include "awt_toolkit.h"
#include "awt_defs.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif // INTLUNICODE

// All java header files are extern "C"
extern "C" {
#include "java_awt_Choice.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WChoicePeer.h"
};


AwtCombobox::AwtCombobox(HWChoicePeer *peer) : AwtWindow((JHandle *)peer)
{
    DEFINE_AWT_SENTINAL("CMB");
}

// -----------------------------------------------------------------------
//
// Return the window class of an AwtLabel window.
//
// -----------------------------------------------------------------------
const char * AwtCombobox::WindowClass()
{
    return "COMBOBOX";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtLabel window
//
// -----------------------------------------------------------------------
DWORD AwtCombobox::WindowStyle()
{
#ifdef INTLUNICODE
    return WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | WS_CLIPSIBLINGS | CBS_OWNERDRAWFIXED ; 
#else
    return WS_CHILD | WS_VSCROLL | CBS_HASSTRINGS | CBS_DROPDOWNLIST | WS_CLIPSIBLINGS; 
#endif
}


DWORD AwtCombobox::WindowExStyle()
{
#ifdef _WIN32
    return WS_EX_CLIENTEDGE;
#else
    return 0;
#endif
}


// -----------------------------------------------------------------------
//
// Add an item at the specified index
// No lock is needed because SendMessage is synchronized
//
// -----------------------------------------------------------------------
void AwtCombobox::AddItem(Hjava_lang_String * string, long lIndex)
{
    ASSERT( ::IsWindow(m_hWnd) );

#ifdef INTLUNICODE
	// We free the WinCompStr in the OnDeleteItem()
	WinCompStr *pNewItem = new WinCompStr(string);

	if(pNewItem)
        ::SendMessage(m_hWnd, CB_INSERTSTRING, (WPARAM)lIndex, (LPARAM)(LPSTR)pNewItem);

#else	// INTLUNICODE

   int32_t len;
   char *buffer = NULL;

   len = javaStringLength(string);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(string, buffer, len+1);

		::SendMessage(m_hWnd, CB_INSERTSTRING, (WPARAM)lIndex, (LPARAM)(LPSTR)buffer);

        free(buffer);
    }
#endif	// INTLUNICODE
}


// -----------------------------------------------------------------------
//
// Select an item specified by index
// No lock is needed because SendMessage is synchronized
//
// -----------------------------------------------------------------------
void AwtCombobox::Select(long lIndex)
{
    ASSERT( ::IsWindow(m_hWnd) );

	::SendMessage(m_hWnd, CB_SETCURSEL, (WPARAM)lIndex, 0L);
}


// -----------------------------------------------------------------------
//
// Delete an item at the specified index
// No lock is needed because SendMessage is synchronized
//
// -----------------------------------------------------------------------
void AwtCombobox::DeleteItem(long lIndex)
{
    ASSERT( ::IsWindow(m_hWnd) );

	::SendMessage(m_hWnd, CB_DELETESTRING, (WPARAM)lIndex, 0L);

}


//
//
//
//
void AwtCombobox::AdjustSize(int nHeight)
{
    RECT rect;

    ASSERT( ::IsWindow(m_hWnd) );

    Lock();

    ::GetWindowRect(m_hWnd, &rect);
    ::SetWindowPos(m_hWnd, (HWND)0, 0, 0, 
                    rect.right - rect.left, 
                    rect.bottom - rect.top + nHeight, 
                    SWP_NOMOVE | SWP_NOZORDER);

    Unlock();

}


// -----------------------------------------------------------------------
//
// catch onSize 
// Apparently notifing java about a combo or list box size change
// enter in an infinite loop (try to change the size back and forth!!??)
//
// -----------------------------------------------------------------------
BOOL AwtCombobox::OnSize( UINT sizeCode, UINT newWidth, UINT newHeight )
{
    return FALSE;
}


// -----------------------------------------------------------------------
//
// raise java events from here!
// 
// -----------------------------------------------------------------------
BOOL AwtCombobox::OnCommand(UINT notifyCode, UINT id)
{
    if (notifyCode == CBN_SELCHANGE) {
        if (m_pJavaPeer) {
            int64 time = PR_NowMS(); // time in milliseconds

            // Call sun.awt.windows.WChoicePeer::action(...)
            AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                        AwtEventDescription::ListAction,
                                        time, 
                                        NULL,
                                        SendMessage(m_hWnd, CB_GETCURSEL, 0,0L));
        }
    }
    else if (notifyCode == CBN_DROPDOWN) {

        long lItems = ::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0L);
        if (lItems) {
            long lHeight = ::SendMessage(m_hWnd, CB_GETITEMHEIGHT, 0, 0L);

            if (lItems >= 9) 
                lItems = 9;
            else
                lItems++;

            AdjustSize((int)lItems * lHeight);
        }
    }

    return FALSE; // let the def proc to process the message anyway
}


BOOL AwtCombobox::OnMouseDown(AwtButtonType button, 
                                UINT metakeys, 
                                int x, 
                                int y)
{
    return AwtComponent::OnMouseDown(button, metakeys, x, y);
    //return FALSE;
}


BOOL AwtCombobox::OnMouseUp(AwtButtonType button, 
                                UINT metakeys,
                                int x, 
                                int y)
{
    return AwtComponent::OnMouseUp(button, metakeys, x, y);
    //return FALSE;
}


BOOL AwtCombobox::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                                
    return AwtComponent::OnPaint(hdc, x, y, w, h);
}
    

#ifdef INTLUNICODE
BOOL AwtCombobox::SetFont(AwtFont *font)
{
    BOOL bStatus;
    bStatus = AwtWindow::SetFont(font);
    if(font && bStatus)
    {
        HDC hDC;
        VERIFY( hDC = ::GetDC(m_hWnd));
        if(hDC)
        {
			// Deside the height first
            TEXTMETRIC metrics;
            ::SelectObject(hDC, (HGDIOBJ) font->GetFont());
            VERIFY(::GetTextMetrics(hDC, &metrics));
            VERIFY(::ReleaseDC(m_hWnd, hDC));
			long h = metrics.tmHeight + metrics.tmExternalLeading 
									  + metrics.tmInternalLeading;
            // Change the height of the item
			::SendMessage(m_hWnd, CB_SETITEMHEIGHT, 0, 
                  MAKELPARAM(h , 0));

			// Change the height of the edit box
			::SendMessage(m_hWnd, CB_SETITEMHEIGHT, -1, 
                  MAKELPARAM(h+6 , 0));


			::RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME );
		}
    }
    return bStatus;
}
#endif
//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WChoicePeer
//
//-------------------------------------------------------------------------

extern "C" {

void 
sun_awt_windows_WChoicePeer_create(HWChoicePeer* self,
								   HWComponentPeer * hParent)
{
    AwtCombobox *pNewCombo;

    CHECK_NULL(self,    "LabelPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewCombo = new AwtCombobox(self);
    CHECK_ALLOCATION(pNewCombo);

// What about locking the new object??
    pNewCombo->CreateWidget( pNewCombo->GetParentWindowHandle(hParent) );
    //pNewCombo->AdjustSize();
}


void 
sun_awt_windows_WChoicePeer_addItem(HWChoicePeer* self,
									Hjava_lang_String * string,
									long lIndex)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtCombobox *obj = GET_OBJ_FROM_PEER(AwtCombobox, self);

	obj->AddItem(string, lIndex);
}


void 
sun_awt_windows_WChoicePeer_select(HWChoicePeer* self,
								   long lIndex)
{
    CHECK_PEER(self);

    AwtCombobox *obj = GET_OBJ_FROM_PEER(AwtCombobox, self);

	obj->Select(lIndex);
}


void 
sun_awt_windows_WChoicePeer_remove(HWChoicePeer* self,
								   long lIndex)
{
    CHECK_PEER(self);

    AwtCombobox *obj = GET_OBJ_FROM_PEER(AwtCombobox, self);

	obj->DeleteItem(lIndex);
}


};  // end of extern "C"
