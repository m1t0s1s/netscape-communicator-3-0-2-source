
#include "awt_dialog.h"
#include "awt_toolkit.h"

// All java header files are extern "C"
extern "C" {
#include "java_awt_Component.h"
#include "java_awt_Dialog.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WDialogPeer.h"
};


AwtDialog::AwtDialog(HWComponentPeer *peer) : AwtJavaWindow(peer)
{
    DEFINE_AWT_SENTINAL("DLG");

    m_bModalLoop = FALSE;
    m_bInitiateModal = FALSE;
    m_pWindows = 0;
    m_nWindowsCount = 0;
    m_hForegroundWindow = 0;
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtDialog window
//
// -----------------------------------------------------------------------
DWORD AwtDialog::WindowStyle()
{
    return WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
}


//-------------------------------------------------------------------------
//
// Size only needs to do update java info for top level windows
//
//-------------------------------------------------------------------------
BOOL AwtDialog::OnMove(int newX, int newY)
{
#if 0
    // this code is currently dead!!!!!!!!!!!!!!!!!!
    // the reason being the old awt does not work always in parent's
    // coordinates even though specs seem to say so
    Hjava_awt_Component *target;
    RECT rc;
    RECT rcParent;
    HWND hwnd;

    if (m_pJavaPeer) {

        ::GetWindowRect(m_hWnd, &rc);        

        // if there is an owner switch coordinates system
        hwnd = ::GetWindow(m_hWnd, GW_OWNER);

        ASSERT(hwnd);

        ::GetWindowRect(hwnd, &rcParent);

        // update x and y position
        target = unhand((HWComponentPeer *)m_pJavaPeer)->target;
        if (target) {
            unhand(target)->x  = rc.left - rcParent.left;
            unhand(target)->y = rc.top - rcParent.top;
        }
    }
    
    return FALSE;
#endif

    // apparently no one cares about having all the java window with a parent
    // to be expressed in parent coordinates system
    return AwtJavaWindow::OnMove(newX, newY);
}


// -----------------------------------------------------------------------
//
// Modal Dialog specific code
//
// -----------------------------------------------------------------------

typedef struct _countWindows {
    HWND hMyself;
    int counter;
} COUNTWINDOWS;

typedef struct _disableWindows {
    COUNTWINDOWS cw;
    HWND* pWindows;
} DISABLEWINDOWS;


//
// count the number of top level windows in this thread
//
BOOL CALLBACK CountWindows(HWND hwnd, LPARAM lParam)
{
    COUNTWINDOWS* cw = (COUNTWINDOWS*)lParam;

    // if this is not the dialog neither is disabled, count it
    if (hwnd != cw->hMyself && ::IsWindowEnabled(hwnd))
        cw->counter++;

    return TRUE;
}


//
// disable all the top level window
//
BOOL CALLBACK DisableWindows(HWND hwnd, LPARAM lParam)
{
    DISABLEWINDOWS* dw = (DISABLEWINDOWS*)lParam;

    // if we filled the collection return FALSE and stop counting
    // it is ok or some windows will be left out
    if (dw->cw.counter == 0)
        return FALSE;

    // disable the window and add it to the list of handles
    if (hwnd != dw->cw.hMyself && ::IsWindowEnabled(hwnd)) {
        if (::GetParent(hwnd) == NULL) {
            ::EnableWindow(hwnd, FALSE);
            dw->cw.counter--;
            dw->pWindows[dw->cw.counter] = hwnd;
        }
    }

    return TRUE;
}


// -----------------------------------------------------------------------
//
// Disable all dialog's frame owner windows
//
// -----------------------------------------------------------------------
void AwtDialog::DisableWindows()
{
    DISABLEWINDOWS lParam;
    lParam.cw.hMyself = m_hWnd;
    lParam.cw.counter = 0;

#ifdef _WIN32
    // count how many windows to be disabled
    ::EnumThreadWindows(AwtToolkit::theToolkit->GetGuiThreadId(),
                        ::CountWindows,
                        (LPARAM)&lParam);
#else
    ::EnumWindows(::CountWindows,
                        (LPARAM)&lParam);
#endif

    m_nWindowsCount = lParam.cw.counter;

    if (m_nWindowsCount) {

        m_pWindows = new HWND[m_nWindowsCount];

        // disable the windows and add them to the collection
        lParam.pWindows = m_pWindows;

#ifdef _WIN32
        ::EnumThreadWindows(AwtToolkit::theToolkit->GetGuiThreadId(),
                            ::DisableWindows,
                            (LPARAM)&lParam);
#else
        ::EnumWindows(::DisableWindows,
                        (LPARAM)&lParam);
#endif

    }
}


// -----------------------------------------------------------------------
//
// Show implementation for dialogs
//
// -----------------------------------------------------------------------
void AwtDialog::ShowModal(long spinLoop)
{
#ifdef _WIN32
    m_hForegroundWindow = ::GetForegroundWindow();
#else
    m_hForegroundWindow = ::GetActiveWindow();
#endif

    // show the dialog
    Show(TRUE);

    if (m_bInitiateModal == FALSE) {

        m_bInitiateModal = TRUE;

        // disable all the top level windows on this thread
        DisableWindows();

        // this just to cover our ass from the insanity of modality 
        // in java. Enable this since this might have been a hidden 
        // instance in the past. Bullshit!!!
        ::EnableWindow(m_hWnd, TRUE);

#ifdef _WIN32
        ::SetForegroundWindow(m_hWnd);
#else
        ::SetActiveWindow(m_hWnd);
#endif

        //
        // spin a callback loop if required.
        // This happens when a modal dialog is open from a method running
        // in the callback loop (which is the most common case, click on 
        // a button and open a dialog..), we don't return until the dialog
        // is closed, so in order to have java working we must spin a callback loop
        //
        if (spinLoop) {
            m_bModalLoop = FALSE;
            AwtToolkit::theToolkit->CallbackLoop(&m_bModalLoop);
        }
    }
}


// -----------------------------------------------------------------------
//
// Hide implementation for dialogs
//
// -----------------------------------------------------------------------
void AwtDialog::HideModal()
{
    HWND hwnd;

    if (m_bInitiateModal) {
        // stop callback loop
        m_bModalLoop = TRUE;

        // enable all the previously disbaled windows
        for (int i = 0; i < m_nWindowsCount; i++) 
            if (m_pWindows[i])
                ::EnableWindow(m_pWindows[i], TRUE);

#ifdef _WIN32
        delete[] m_pWindows;

        ::SetForegroundWindow(m_hForegroundWindow);
#else
        //delete m_pWindows;

        ::SetActiveWindow(m_hForegroundWindow);
#endif

        m_pWindows = 0;

        m_bInitiateModal = FALSE;
    }

    // hide the dialog
    Show(FALSE);
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WDialogPeer
//
//-------------------------------------------------------------------------

extern "C" {

void 
sun_awt_windows_WDialogPeer_create(HWDialogPeer* self,
                                    HWComponentPeer* parent)
{
    AwtDialog    *pNewDialog;

    CHECK_NULL(self, "DialogPeer is null.");
    //CHECK_NULL(parent, "ComponentPeer is null.");

    pNewDialog = new AwtDialog((HWComponentPeer*) self);
    CHECK_ALLOCATION(pNewDialog);

    pNewDialog->CreateWidget( pNewDialog->GetParentWindowHandle(parent) );
}


void 
sun_awt_windows_WDialogPeer_setTitle(HWDialogPeer* self, 
                                        Hjava_lang_String* title)
{
    CHECK_PEER(self);
    CHECK_OBJECT(title);

    int32_t len;
    char *buffer = NULL;
    AwtDialog *obj = GET_OBJ_FROM_PEER(AwtDialog, self);

    len = javaStringLength(title);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(title, buffer, len+1);

        ::SetWindowText(obj->GetWindowHandle(), buffer);

        free(buffer);
    }
}


void 
sun_awt_windows_WDialogPeer_setResizable(HWDialogPeer* self,
                                            /*boolean*/ long resizable)
{
    CHECK_PEER(self);
    AwtDialog* pDialog = GET_OBJ_FROM_PEER(AwtDialog, self);
    if (pDialog)
        pDialog->Resizable((resizable) ? TRUE : FALSE);
}


void 
sun_awt_windows_WDialogPeer_showModal(HWDialogPeer* self, 
                                        long spinLoop)
{
    CHECK_PEER(self);

    AwtDialog *obj = GET_OBJ_FROM_PEER(AwtDialog, self);

    if (obj)
        obj->ShowModal(spinLoop);
}


void 
sun_awt_windows_WDialogPeer_hideModal(HWDialogPeer* self)
{
    CHECK_PEER(self);

    AwtDialog *obj = GET_OBJ_FROM_PEER(AwtDialog, self);

    if (obj)
        obj->HideModal();
}


}; // end of extern "C"




