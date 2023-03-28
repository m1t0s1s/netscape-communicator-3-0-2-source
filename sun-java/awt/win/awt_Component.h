#ifndef AWT_COMPONENT_H
#define AWT_COMPONENT_H

#include "awt_defs.h"
#include "awt_object.h"
#include "awt_resource.h"
#include "awt_dc.h"
#include "awt_font.h"
#include "awt_event.h"

#if !defined(_AWT_IDCSERVICE_H)
#include "awt_IDCService.h"
#endif

class AwtWindow;
struct Hjava_awt_Insets;

//
// Declare the Java peer class
//
DECLARE_SUN_AWT_WINDOWS_PEER(WComponentPeer);
DECLARE_JAVA_AWT_PEER(Event);

typedef struct JavaEventInfo JavaEventInfo;
class AwtComponent : public AwtObject, public IDCService
{
public:
    //
    // Enumeration of the different colors associated with an AwtComponent
    //
    enum AwtColorType {
        Foreground = 0,
        Background,
        AwtMaxColorType,
    };

    //
    // Enumeration of the possible mouse buttons 
    //
    enum AwtButtonType {
        AwtNoButton   = 0,
        AwtLeftButton = 1,
        AwtRightButton,
        AwtMiddleButton,
    };

    //
    // Enumeration of the methods which are accessable on the "main GUI thread"
    // via the CallMethod(...) mechanism...
    //
    enum {
        CREATE       = 0x0101,
        DESTROY, 
        GET_FOCUS,
        HANDLE_EVENT,
        GETDC,
    };

public:
                            AwtComponent(JHandle *peer);
    virtual                 ~AwtComponent();

    virtual void            Dispose(void);

    virtual BOOL            CallMethod(AwtMethodInfo *info);

    virtual void            HandleEvent(JavaEventInfo* pInfo, Event* pEvent) = 0;

    // IDCService interface
    virtual AwtDC*          GetAwtDC() { return m_pAwtDC; }
    virtual void            Attach(AwtDC* pDC);
    virtual void            Detach(HDC hdc) { }
    virtual HDC             GetDC() { return ::GetDC(GetWindowHandle()); }
    virtual void            ReleaseDC(AwtDC* pdc);
    virtual HBRUSH          GetBackground() { return m_BackgroundColor->GetBrush(); }

    //
    // Create a new Windows "widget" for the AwtComponent
    //
    virtual void            CreateWidget(HWND parent) = 0;
    virtual void            Destroy(HWND hWnd) = 0;

    //
    // Show/hide the AwtComponent
    //
    virtual void            Show(BOOL bState) = 0;

    //
    // Move/resize the AwtComponent
    //
    virtual void            Reshape(long x, long y, long width, long height) = 0;

    //
    // Enable/disable the AwtComponent
    //
    virtual void            Enable(BOOL bState) = 0;

    // Request the input focus...
    virtual void            GetFocus(void) = 0;

    // Move the focus to the next AwtComponent
    virtual AwtComponent*   NextFocus(void) = 0;
    virtual HWND            GetWindowHandle(void) = 0;

    // Check if have to get focus
    virtual BOOL            WantFocusOnMouseDown() = 0;

    // capture the mouse if required
    virtual void            CaptureMouse(AwtButtonType button, UINT metakeys) = 0;

    // release the mouse if captured
    virtual BOOL            ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y) = 0;

    // Return the Window handle for the parent of a component...
    virtual HWND            GetParentWindowHandle(HWComponentPeer *hParent);

    //
    // Methods to get/set the current foreground and background colors
    //    
    virtual AwtCachedBrush* GetColor(AwtColorType type);
    virtual BOOL            SetColor(AwtColorType type, COLORREF color);

    //
    // Methods to get/set the current font
    //    
    virtual AwtFont*        GetFont(void);
    virtual BOOL            SetFont(AwtFont* font);
#ifdef INTLUNICODE
	virtual	int16			GetPrimaryEncoding();
#endif // INTLUNICODE

    //
    // Event handlers for Windows events... The return value
    // indicates whether the event was handled.  If FALSE is 
    // returned, then the DefWindowProc(...) will be called.
    //
    virtual BOOL        OnClose  (void);
    virtual BOOL        OnDestroy(void);
    
    virtual BOOL        OnPaint(HDC hdc, int x, int y, int w, int h);
    virtual BOOL        OnEraseBackground(HDC hdc);
    virtual BOOL        OnCtlColor(HDC hdc);

    virtual BOOL        OnSize(UINT sizeCode, UINT newWidth, UINT newHeight);
    virtual BOOL        OnMove(int newX, int newY);

    virtual BOOL        OnMouseDown    (AwtButtonType button, UINT metakeys, 
                                        int x, int y);
    virtual BOOL        OnMouseUp      (AwtButtonType button, UINT metakeys,
                                        int x, int y);
    virtual BOOL        OnMouseDblClick(AwtButtonType button, UINT metakeys,
                                        int x, int y);

    virtual BOOL        OnMouseMove    (AwtButtonType button, UINT metakeys, 
                                        int x, int y);

    virtual BOOL        OnKeyDown(UINT virtualCode, UINT scanCode); 
    virtual BOOL        OnKeyUp  (UINT virtualCode, UINT scanCode);
    virtual BOOL        OnChar   (UINT virtualCode);

    virtual BOOL        OnSetFocus(void);
    virtual BOOL        OnKillFocus(void);
    
    virtual BOOL        OnSetCursor(HWND hCursorWnd, WORD wHittest, WORD wMouseMsg);

    virtual BOOL        OnHScroll(UINT scrollCode, int cPos);
    virtual BOOL        OnVScroll(UINT scrollCode, int cPos);
                        //!dario! this is implemented by our scrollbar and
                        //!dario! may disappear in a near future
    virtual void        OnScroll(UINT scrollCode, int cPos) {;}

    virtual BOOL        OnCommand(UINT notifyCode, UINT id);

    virtual BOOL        OnSelect(UINT id);

    virtual BOOL        OnMenuSelect(HMENU hMenu, UINT notifyCode, UINT id);
    
    virtual BOOL        OnNCHitTest(LPPOINT ppt, int nHitTest);

#ifdef INTLUNICODE
    virtual BOOL		OnDrawMenuItem(UINT idCtl, LPDRAWITEMSTRUCT lpdis);
    virtual BOOL		OnMeasureMenuItem(UINT idCtl, LPMEASUREITEMSTRUCT lpmis);

    virtual BOOL		OnDrawItem(UINT idCtl, LPDRAWITEMSTRUCT lpdis);
    virtual BOOL		OnMeasureItem(UINT idCtl, LPMEASUREITEMSTRUCT lpmis);
    virtual BOOL		OnDeleteItem(UINT idCtl, LPDELETEITEMSTRUCT lpdis);
#endif	// INTLUNICODE

protected:
    //
    // Return the windows MSG structure for the current message.  If
    // a message is NOT being processed, or the MSG structure is unavailable,
    // then NULL is returned.  The returned structure is temporary and
    // MUST be copied.
    // 
    // This method MUST only be called on the "main gui thread"
    //
    virtual MSG *       GetCurrentMessage(void) = 0;


            long        GetMouseModifiers(UINT flags);
            long        GetKeyModifiers(BYTE *state);

            BOOL        TranslateToAscii(BYTE *keyState, UINT virtualCode, 
                                         UINT scanCode, WORD *asciiKey);
    //
    // Data members:
    //
protected:
    // Foreground color
    AwtCachedBrush* m_ForegroundColor;
    
    // Background color
    AwtCachedBrush* m_BackgroundColor;

    // Current font
    AwtFont*    m_pFont;

    // Current AwtDC
    AwtDC*      m_pAwtDC;

    // Current size
    POINT       m_ClientArea;

    // Previous window that had "capture"
    int64       m_TimeLastMouseDown;
    int         m_nMouseCount;
    int         m_nCaptured;

    // This flag indicates whether ALL events should be sent to Java before
    // passing them onto the native widget, or only keyboard events.
    //
    // This flag is only necessary to maintain compatability with previous 
    // versions of Awt which ONLY allowed sent keyboard events to Java "first".
    // 
    BOOL        m_bPreprocessAllEvents;
    BYTE        m_keyState[256];

#ifdef INTLUNICODE
	int16		m_primaryEncoding;
#endif
};



#endif // AWT_COMPONENT_H
