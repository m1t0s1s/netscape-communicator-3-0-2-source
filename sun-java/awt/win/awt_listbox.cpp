#include "awt_listbox.h"
#include "awt_toolkit.h"
#include "awt_defs.h"

#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#include "awt_i18n.h"
#endif
// All java header files are extern "C"
extern "C" {
#include "java_awt_List.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WListPeer.h"
};


AwtListbox::AwtListbox(HWListPeer *peer) : AwtWindow((JHandle *)peer)
{
    DEFINE_AWT_SENTINAL("LST");
    m_dwMultipleSelection = 0; // no multiple selection by default
}

// -----------------------------------------------------------------------
//
// Return the window class of an AwtListbox window.
//
// -----------------------------------------------------------------------
const char * AwtListbox::WindowClass()
{
    return "LISTBOX";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtListbox window
//
// -----------------------------------------------------------------------
DWORD AwtListbox::WindowStyle()
{
    return WS_CHILD | WS_VSCROLL | WS_BORDER | WS_CLIPSIBLINGS
#ifdef INTLUNICODE
			| LBS_OWNERDRAWFIXED
#endif // INTLUNICODE
            | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY 
            | m_dwMultipleSelection;
}


DWORD AwtListbox::WindowExStyle()
{
#ifdef _WIN32
    return WS_EX_CLIENTEDGE;
#else
    return 0;
#endif
}


// -----------------------------------------------------------------------
//
// Set or reset the list box flag to allow multiple selection
// If the list box has been created already we must create a new
// list, copy all the value into the new list and show it.
//
// -----------------------------------------------------------------------
void AwtListbox::SetMultipleSelections(HWListPeer* peer, long lMS)
{

    Lock();

    if ((m_dwMultipleSelection && !lMS) ||
                (!m_dwMultipleSelection && lMS)) {
        
        m_dwMultipleSelection = (lMS) ? LBS_MULTIPLESEL : 0;
    
        if (::IsWindow(m_hWnd)) {
            long lItems;
            HWND oldWnd = m_hWnd;

            SubclassWindow(FALSE);

            // create a new list box
            CreateWidget( GetParent(m_hWnd));

            // copy all the elements
            lItems = ::SendMessage(oldWnd, LB_GETCOUNT, 0, 0L);
            if (lItems != LB_ERR && lItems > 0) {
                long bSelectionDone = 0;

                for (long i = 0; i < lItems; i++) {
#ifdef INTLUNICODE
                    // get string at index
                    LRESULT ret =  ::SendMessage(oldWnd,
                                    LB_GETITEMDATA,
                                    (WPARAM)i,
                                    (LPARAM)0);
					VERIFY(ret != LB_ERR);
					if(ret != LB_ERR)
					{
						// We need to clone it

						WinCompStr	*pNew = new WinCompStr(* (WinCompStr*)ret);

						// insert string at index
						::SendMessage(m_hWnd,
										LB_INSERTSTRING,
										(WPARAM)i,
										(LPARAM)pNew);
					}
#else	// INTLUNICODE
                    char buffer[128]; //for now, this should be changed to a malloc

                    // get string at index
                    ::SendMessage(oldWnd,
                                    LB_GETTEXT,
                                    (WPARAM)i,
                                    (LPARAM)buffer);

                    // insert string at index
                    ::SendMessage(m_hWnd,
                                    LB_INSERTSTRING,
                                    (WPARAM)i,
                                    (LPARAM)buffer);
#endif	// INTLUNICODE
                    // keep selection. 
                    // From single to multiple is the selected item, if any.
                    // From multiple to single is the first selected item encountered
                    if (!bSelectionDone) {
                        bSelectionDone = ::SendMessage(oldWnd,
                                                        LB_GETSEL,
                                                        (WPARAM)i,
                                                        0);
                        if (bSelectionDone)
                            if (m_dwMultipleSelection)
                                ::SendMessage(m_hWnd, 
                                                LB_SETSEL,
                                                (WPARAM)bSelectionDone,
                                                (LPARAM)i);
                            else
                                ::SendMessage(m_hWnd, 
                                                LB_SETCURSEL,
                                                (WPARAM)i,
                                                0);
                    }
                }
            } 
            if (m_pFont)
                ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)(m_pFont->GetFont()), 0); 
			
			::UpdateWindow(m_hWnd);
            ::ShowWindow(m_hWnd, SW_SHOW);

            Destroy(oldWnd);
            
        } // if window exists

    } // if multiselection changed
	
    Unlock();
}


// -----------------------------------------------------------------------
//
// Add an item at the specified index
// No lock is needed because SendMessage is synchronized
//
// -----------------------------------------------------------------------
void AwtListbox::AddItem(Hjava_lang_String *item, long index)
{
    ASSERT( ::IsWindow(m_hWnd) );
#ifdef INTLUNICODE

	// We free the WinCompStr in the OnDeleteItem()
	WinCompStr *pNewItem = new WinCompStr(item);

	if(pNewItem)
        ::SendMessage(m_hWnd, LB_INSERTSTRING, (WPARAM)index, (LPARAM)(LPSTR)pNewItem);

#else	// INTLUNICODE
    int32_t len;
    char *buffer = NULL;


    len = javaStringLength(item);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(item, buffer, len+1);

        ::SendMessage(m_hWnd, LB_INSERTSTRING, (WPARAM)index, (LPARAM)(LPSTR)buffer);

        free(buffer);
    }
#endif	// INTLUNICODE
}


// -----------------------------------------------------------------------
//
// Delete a consecutive set of items (as specified by start/end)
// Lock because of the for(;;)
//
// -----------------------------------------------------------------------
void AwtListbox::DeleteItems(long start, long end)
{
    ASSERT( ::IsWindow(m_hWnd) );

    Lock();

    for (long i = end; i >= start; i--) 
        ::SendMessage(m_hWnd, LB_DELETESTRING, (WPARAM)i, 0L);

    Unlock();
}


// -----------------------------------------------------------------------
//
// Select or deselect the item at index according to bSelection
//
//! do we really need to lock?
//
// -----------------------------------------------------------------------
void AwtListbox::SetSelection(long index, BOOL bSelection)
{
    ASSERT( ::IsWindow(m_hWnd) );

    Lock();

    // use m_dwMultipleSelection to figure if multiple selection is setted
    if (m_dwMultipleSelection) {

        ::SendMessage(m_hWnd, LB_SETSEL, (WPARAM) bSelection, (LPARAM) index);
    }
    else {
        // single line list box, if deselect is called reset any selection
        // in case of deselection don't check if index is selected just
        // nuke whatever selection was there
        if (!bSelection)
            index = -1;

        ::SendMessage(m_hWnd, LB_SETCURSEL, (WPARAM) index, 0L);
    }

    Unlock();
}


// -----------------------------------------------------------------------
//
// Scroll a specified item into view
//
// -----------------------------------------------------------------------
void AwtListbox::MakeVisible(long index)
{
    ASSERT( ::IsWindow(m_hWnd) );

    ::SendMessage(m_hWnd, LB_SETTOPINDEX, (WPARAM) index, 0L);
}


// -----------------------------------------------------------------------
//
// Return whether the specified item is selected or not
//
// -----------------------------------------------------------------------
long AwtListbox::IsSelected(long index)
{
    ASSERT( ::IsWindow(m_hWnd) );

    return ::SendMessage(m_hWnd, LB_GETSEL, (WPARAM)index, 0L);
}


// -----------------------------------------------------------------------
//
// catch onSize so we don't let any events to flow
// Apparently notifing java about a combo or list box size change
// enter in an infinite loop (try to change the size back and forth!!??)
//
// -----------------------------------------------------------------------
BOOL AwtListbox::OnSize( UINT sizeCode, UINT newWidth, UINT newHeight )
{
    return FALSE;
}


// -----------------------------------------------------------------------
//
// raise java events from here!
// 
// -----------------------------------------------------------------------
BOOL AwtListbox::OnCommand(UINT notifyCode, UINT id)
{

    switch (notifyCode) {
        case LBN_DBLCLK:
            if (m_pJavaPeer) {
                int64 time = PR_NowMS(); // time in milliseconds

                // Call sun.awt.windows.WChoicePeer::handleAction(...)
                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::ListAction,
                                            time, 
                                            NULL,
                                            ::SendMessage(m_hWnd, LB_GETCURSEL, 0, 0L));
            }
            
            break;

        case LBN_SELCHANGE:
            if (m_pJavaPeer) {
                long index = ::SendMessage(m_hWnd, LB_GETCURSEL, 0,0L);
                int64 time = PR_NowMS(); // time in milliseconds

                // Call sun.awt.windows.WListPeer::handleListChanged(...)
                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::ListChanged,
                                            time, 
                                            NULL,
                                            index,
                                            ::SendMessage(m_hWnd, LB_GETSEL, (WPARAM)index, 0L));
            }
            
            break;

    }

    return FALSE; // let the def proc to process the message anyway
}


BOOL AwtListbox::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                                
    return AwtComponent::OnPaint(hdc, x, y, w, h);
}
    

BOOL AwtListbox::OnEraseBackground( HDC hdc )
{
    return FALSE;
}



//-------------------------------------------------------------------------
//
// Capture the mouse (if it's not the left button) so that mouse events 
// get re-direct to the proper component. 
// This allow MouseDrag to work correctly and MouseUp to be sent to the 
// component which received the mouse down.
// Left down is a special case because windows needs to handle that.
//
//-------------------------------------------------------------------------
void AwtListbox::CaptureMouse(AwtButtonType button, UINT metakeys)
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
// Release the mouse if it was captured previously. We might not have 
// captured the mouse at all if the left button is the first to be pressed
// and the last released
//
//-------------------------------------------------------------------------
BOOL AwtListbox::ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y)
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

#ifdef INTLUNICODE
BOOL AwtListbox::SetFont(AwtFont *font)
{
    BOOL bStatus;
    bStatus = AwtComponent::SetFont(font);
    if(font && bStatus)
    {
        HDC hDC;
        VERIFY( hDC = ::GetDC(m_hWnd));
        if(hDC)
        {
            TEXTMETRIC metrics;
            ::SelectObject(hDC, (HGDIOBJ) font->GetFont());
            VERIFY( ::GetTextMetrics(hDC, &metrics));
            VERIFY( ::ReleaseDC(m_hWnd, hDC));
            ::SendMessage(m_hWnd, LB_SETITEMHEIGHT, 0,
                  MAKELPARAM(metrics.tmHeight + metrics.tmExternalLeading 
				                              + metrics.tmInternalLeading, 0));
			::RedrawWindow(m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
		}
    }
    return bStatus;
}
#endif

//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WListboxPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WListPeer_create(HWListPeer *self, 
                                    HWComponentPeer *hParent)
{
    AwtListbox   *pNewListbox;

    CHECK_NULL(self,    "ListboxPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewListbox = new AwtListbox(self);
    CHECK_ALLOCATION(pNewListbox);

// What about locking the new object??
    pNewListbox->CreateWidget( pNewListbox->GetParentWindowHandle(hParent) );
}


#if 0
void
sun_awt_windows_WListPeer_createWidget(HWListPeer *self, 
                                        HWComponentPeer *hParent)
{
    CHECK_PEER(self);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

// What about locking the new object??
    obj->CreateWidget( obj->GetParentWindowHandle(hParent) );
}
#endif


void 
sun_awt_windows_WListPeer_setMultipleSelections(HWListPeer* self,
                                                /*boolean*/ long lSelection)
{
    CHECK_PEER(self);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    obj->SetMultipleSelections(self, lSelection);
}

												
void 
sun_awt_windows_WListPeer_addItem( HWListPeer* self, 
                                   Hjava_lang_String *string,
                                   long lIndex)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    obj->AddItem(string, lIndex);
}


void 
sun_awt_windows_WListPeer_delItems( HWListPeer* self,
                                    long start,
                                    long end)
{
    CHECK_PEER(self);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    obj->DeleteItems(start, end);
}


void 
sun_awt_windows_WListPeer_select( HWListPeer* self,
                                  long index)
{
    CHECK_PEER(self);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    obj->SetSelection(index, TRUE);
}


void 
sun_awt_windows_WListPeer_deselect( HWListPeer* self,
                                    long index)
{
    CHECK_PEER(self);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    obj->SetSelection(index, FALSE);
}


void 
sun_awt_windows_WListPeer_makeVisible( HWListPeer* self,
                                       long index)
{
    CHECK_PEER(self);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    obj->MakeVisible(index);
}


/*boolean*/ long 
sun_awt_windows_WListPeer_isSelected( HWListPeer* self,
                                      long index)
{
    CHECK_PEER_AND_RETURN(self, FALSE);

    AwtListbox *obj = GET_OBJ_FROM_PEER(AwtListbox, self);

    return obj->IsSelected(index);
}


};  // end of extern "C"
