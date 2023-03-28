
#include "awt_frame.h"
#include "awt_toolkit.h"

// All java header files are extern "C"
extern "C" {
#include "javastring.h"

#include "java_awt_Insets.h"
#include "java_awt_Component.h"
#include "java_awt_MenuBar.h"
#include "java_awt_Frame.h"
#include "sun_awt_image_ImageRepresentation.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WFramePeer.h"

#include "sun_awt_windows_WMenuBarPeer.h"

#ifdef NETSCAPE 
#include "netscape_applet_EmbeddedAppletFrame.h"
#include <java.h>   // LJAppletData 
#endif  /* NETSCAPE */
};


AwtFrame::AwtFrame(HWComponentPeer *peer) : AwtJavaWindow(peer)
{
    DEFINE_AWT_SENTINAL("FRM");
    m_MenuBar = 0;
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtEmbeddedFrame window
//
// -----------------------------------------------------------------------
DWORD AwtFrame::WindowStyle()
{
    return WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
}


//-------------------------------------------------------------------------
//
// Load or delete a menu bar for this frame
//
//-------------------------------------------------------------------------
void AwtFrame::SetMenuBar(AwtMenuBar* menuBar) 
{ 
    HMENU hMenuBar = 0;

    if (menuBar)
        hMenuBar = *menuBar;

    if (::SetMenu(m_hWnd, hMenuBar)) {
        Lock();

        m_MenuBar = menuBar;

        Unlock();
    }
}


//-------------------------------------------------------------------------
//
// Set the cursor specified from lCursorID (coming from Frame.java)
//
//-------------------------------------------------------------------------
void AwtFrame::SetCursor(long lCursorID)
{

    HCURSOR cursor = 0;

    switch(lCursorID) {
        case java_awt_Frame_DEFAULT_CURSOR:
        default:
            cursor = ::LoadCursor(NULL, IDC_ARROW);
        break;

        case java_awt_Frame_CROSSHAIR_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_CROSS);
        break;

        case java_awt_Frame_TEXT_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_IBEAM);
        break;
        
        case java_awt_Frame_WAIT_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_WAIT);
        break;
        
        case java_awt_Frame_NW_RESIZE_CURSOR:
        case java_awt_Frame_SE_RESIZE_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_SIZENWSE);
        break;
        
        case java_awt_Frame_SW_RESIZE_CURSOR:
        case java_awt_Frame_NE_RESIZE_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_SIZENESW);
        break;
        
        case java_awt_Frame_N_RESIZE_CURSOR:
        case java_awt_Frame_S_RESIZE_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_SIZENS);
        break;
        
        case java_awt_Frame_W_RESIZE_CURSOR:
        case java_awt_Frame_E_RESIZE_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_SIZEWE);
        break;
        
        case java_awt_Frame_HAND_CURSOR:
            cursor = ::LoadCursor(NULL, IDC_UPARROW);
        break;
        
        case java_awt_Frame_MOVE_CURSOR:
#if defined(_WIN32)
            cursor = ::LoadCursor(NULL, IDC_SIZEALL);
#else
            cursor = ::LoadCursor(NULL, IDC_SIZE);
#endif // !_WIN32
        break;
    }
    
    Lock();
    // got a cursor, set it if different 
    if (cursor && cursor != m_hCursor) {
        m_hCursor = cursor;
    }
    else
        cursor = 0;
    Unlock();

    if (cursor)
        ::SetCursor(cursor);
}


BOOL AwtFrame::OnSetCursor(HWND hCursorWnd, WORD wHittest, WORD wMouseMsg)
{
    if (wHittest == HTCLIENT) {
        
        Lock();

        HCURSOR hc = m_hCursor;

        Unlock();

        ::SetCursor(hc);

        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------
//
// Resizing a frame may cause the menu bar to be on different lines
//
//-------------------------------------------------------------------------
BOOL AwtFrame::OnSize( UINT sizeCode, UINT newWidth, UINT newHeight )
{
    Hjava_awt_Insets* insets = (Hjava_awt_Insets*)
                                    unhand((HWFramePeer*)m_pJavaPeer)->insets;
    CalculateInsets(insets);
    return AwtJavaWindow::OnSize(sizeCode, newWidth, newHeight);
}


//-------------------------------------------------------------------------
//
// Select a menu item. Pass the id to the menu object
//
//-------------------------------------------------------------------------
BOOL AwtFrame::OnSelect(UINT id)
{
    if (m_MenuBar)
        return m_MenuBar->OnSelect(id);
    else
        return FALSE;
}

    
//-------------------------------------------------------------------------
//
// AwtEmbeddedFrame implementation
//
//-------------------------------------------------------------------------
AwtEmbeddedFrame::AwtEmbeddedFrame(HWComponentPeer *peer) : AwtFrame(peer)
{
    DEFINE_AWT_SENTINAL("EMB");
}


// -----------------------------------------------------------------------
//
// Create a AwtEmbeddedFrame component. Overlod AwtFrame
//
// -----------------------------------------------------------------------
void AwtEmbeddedFrame::CreateWidget(HWND hwndParent)
{
    Hjava_awt_Component *target;

#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread" because it creates a window...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtComponent::CREATE, (long)hwndParent);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }
#endif

    //
    // This method is now being executed on the "main gui thread"
    //
    ASSERT( AwtToolkit::theToolkit->IsGuiThread() );

    target = (Hjava_awt_Component *)
                unhand((HWComponentPeer *)m_pJavaPeer)->target;

    CHECK_NULL( target, "Target is null." );

    //
    // Create the new window
    //
    m_hWnd = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                            WindowClass(),
                            "",
                            WindowStyle(),
                            (int) unhand(target)->x,
                            (int) unhand(target)->y,
                            (int) unhand(target)->width,
                            (int) unhand(target)->height,
                            hwndParent,
                            NULL,
                            WindowHInst(),
                            NULL);

    //
    // Subclass the window to allow windows events to be intercepted...
    //
    ASSERT( ::IsWindow(m_hWnd) );
    SubclassWindow(TRUE);
}


BOOL AwtEmbeddedFrame::OnSize( UINT sizeCode, UINT newWidth, UINT newHeight )
{
    return AwtContainer::OnSize(sizeCode, newWidth, newHeight);
}


void AwtEmbeddedFrame::Resizable(BOOL bState)
{
}

// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtEmbeddedFrame window
//
// -----------------------------------------------------------------------
DWORD AwtEmbeddedFrame::WindowStyle()
{
    return WS_CHILD | WS_CLIPCHILDREN;
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WFramePeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WFramePeer_create(HWFramePeer *self, HWComponentPeer *hParent)
{
    AwtFrame           *pNewFrame;
    Hjava_awt_Frame    *target;

    CHECK_NULL(self,    "FramePeer is null.");

#ifdef NETSCAPE
    target     = (Hjava_awt_Frame*)unhand(self)->target;
    if (target) {
        ClassClass *embeddedAppletFrameClass;
        bool_t      isEmbedded;

        embeddedAppletFrameClass = FindClass(0, "netscape/applet/EmbeddedAppletFrame", (bool_t)0);
        isEmbedded = is_instance_of((JHandle *)target, embeddedAppletFrameClass, EE());
        if (isEmbedded) {
            Classnetscape_applet_EmbeddedAppletFrame *pAppletFrame;
            LJAppletData *ad;
 
            pAppletFrame = (Classnetscape_applet_EmbeddedAppletFrame *)unhand(target);
            if( pAppletFrame && pAppletFrame->pData) {
                ad = (LJAppletData *)pAppletFrame->pData;

                pNewFrame = new AwtEmbeddedFrame((HWComponentPeer*)self);
                CHECK_ALLOCATION(pNewFrame);

                pNewFrame->CreateWidget((HWND)ad->window);
                CHECK_PEER(self);
                return;
            }
        }
    }
#endif

    pNewFrame = new AwtFrame((HWComponentPeer*)self);
    CHECK_ALLOCATION(pNewFrame);

// What about locking the new object??
    pNewFrame->CreateWidget( pNewFrame->GetParentWindowHandle(hParent) );
    CHECK_PEER(self);

}


void
sun_awt_windows_WFramePeer_setTitle(HWFramePeer *self, Hjava_lang_String *title)
{
    CHECK_PEER(self);
    CHECK_OBJECT(title);

    int32_t len;
    char *buffer = NULL;
    AwtFrame *obj = GET_OBJ_FROM_PEER(AwtFrame, self);

    len = javaStringLength(title);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(title, buffer, len+1);

        ::SetWindowText(obj->GetWindowHandle(), buffer);

        free(buffer);
    }
}


void
sun_awt_windows_WFramePeer_setMenuBar(HWFramePeer *self, Hjava_awt_MenuBar *mb)
{
    CHECK_PEER(self);
    AwtMenuBar* menuBarObj;

    AwtFrame *obj = GET_OBJ_FROM_PEER(AwtFrame, self);

    if (mb && unhand(mb)) {
        HWMenuBarPeer* peer = (HWMenuBarPeer*)(unhand(mb)->peer);
        menuBarObj = GET_OBJ_FROM_MENU_PEER(AwtMenuBar, peer);
    }
    else
        menuBarObj = NULL;

    obj->SetMenuBar(menuBarObj);
}


void
sun_awt_windows_WFramePeer_setResizable(HWFramePeer *self, long resizable)
{
    CHECK_PEER(self);

    AwtFrame *obj = GET_OBJ_FROM_PEER(AwtFrame, self);

    obj->Resizable(resizable);
}


void
sun_awt_windows_WFramePeer_setCursor(HWFramePeer *self, long cursorType)
{
    CHECK_PEER(self);

    AwtFrame *obj = GET_OBJ_FROM_PEER(AwtFrame, self);

    obj->SetCursor(cursorType);
}


void
sun_awt_windows_WFramePeer_widget_setIconImage(HWFramePeer *self, 
                                               Hsun_awt_image_ImageRepresentation *ir)
{
    CHECK_PEER(self);
    CHECK_OBJECT(ir);

    AwtFrame *obj = GET_OBJ_FROM_PEER(AwtFrame, self);

    UNIMPLEMENTED();
}


}; // end of extern "C"


