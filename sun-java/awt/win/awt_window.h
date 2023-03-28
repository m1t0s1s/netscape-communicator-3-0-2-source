#ifndef AWT_WINDOW_H
#define AWT_WINDOW_H

#include "awt_defs.h"
#include "awt_Component.h"

typedef struct JavaEventInfo JavaEventInfo;
DECLARE_JAVA_AWT_PEER(Event);

class AwtWindow : public AwtComponent
{
public:
                            AwtWindow(JHandle *peer);
                            ~AwtWindow();

    virtual void            HandleEvent(JavaEventInfo* pInfo, Event *pEvent);
                            
    //
    // Create a new Windows "widget" for the AwtComponent
    //
    virtual void            CreateWidget(HWND parent);
    virtual void            Destroy(HWND hWnd);
    
    virtual void            Show(BOOL bState);
    virtual void            Enable(BOOL bState);

    virtual BOOL            SetFont(AwtFont* font);
  
    virtual void            Reshape(long x, long y, long width, long height);

    virtual void            GetFocus(void);
    virtual AwtComponent*   NextFocus(void);
    
    virtual HWND            GetWindowHandle(void);

    // return whether SetFocus has to be called or not
    virtual BOOL            WantFocusOnMouseDown();
    // capture the mouse if required
    virtual void            CaptureMouse(AwtButtonType button, UINT metakeys);
    // release the mouse if captured
    virtual BOOL            ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y);

    virtual BOOL            OnNCHitTest(LPPOINT ppt, int nHitTest);
    virtual BOOL            OnPaint(HDC hdc, int x, int y, int w, int h);
    virtual BOOL            OnDestroy(void);

protected:
    virtual MSG *           GetCurrentMessage(void);

    //
    // Window class and style of the Windows "widget" 
    //
    virtual const char *    WindowClass() = 0;
    virtual DWORD           WindowStyle() = 0;
    virtual DWORD           WindowExStyle() { return 0L;}

	// Delivers hinst parameter to ::CreateWindow
    virtual HINSTANCE		WindowHInst();

    //
    // Event dispatcher which translates Windows events into method calls
    // to the appropriate event handler for the object...
    //
    virtual BOOL            Dispatch(UINT Msg, WPARAM WParam, LPARAM lParam, 
                                    LRESULT *result);
    
    
            void            SubclassWindow(BOOL bState);
                    
private:
    static  void            RegisterAtoms  (void);
    static  void            UnRegisterAtoms(void);

    //
    // Helper functions for translating HWNDs to AwtWindows
    //
    static  void            SetThisPtr(HWND hWnd, AwtWindow* pThis);
    static  AwtWindow*      GetThisPtr(HWND hWnd);
    static  void            RemoveThisPtr(HWND hWnd);

    //
    // Subclassed window procedure used to call event handlers...
    //
    static LRESULT AWT_CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM WParam, 
                                    LPARAM lParam);

    static LRESULT AWT_CALLBACK KeyHook(int code, WPARAM wParan, LPARAM lParam);

    //
    // Data members:
    //
protected:
    // Handle to the native window.
    HWND        m_hWnd;
    WNDPROC     m_PrevWndProc;

    // Copy of the current windows message being processed...
    MSG *       m_pMsg;

private:
    static HHOOK    m_hKeyboardHook;
    static UINT     m_nHookCount;

    static ATOM     thisPtr;
    
#if !defined(_WIN32)
    static ATOM     thisLow;
#endif


#ifdef DEBUG_CONSOLE
    friend class AwtDebug;
#endif
    
};

#endif
