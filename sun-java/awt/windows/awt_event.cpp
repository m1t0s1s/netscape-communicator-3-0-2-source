/*
 * @(#)awt_event.cpp	1.10 95/12/09 Thomas Ball
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
// awt_event.cpp : input event routines.

#include "stdafx.h"
#include "awt_event.h"
#include "awt_comp.h"
#include "awt_window.h"
#include "awtapp.h"

// Enable this define to suppress mouse event filtering.
#define KEYBOARD_ONLY_EVENTS

static UINT JavaCharToWindows(UINT nChar);

BOOL HandleInputEvent(MSG *pMsg) {
#ifdef NEW_EVENT_MODEL
    AwtEvent* pEvent = AwtComp::LookupComponent(pMsg->hwnd);
    if (pEvent != NULL) {
        pEvent->CopyMessage(pMsg);
        switch (pMsg->message) {
# ifdef KEYBOARD_EVENT_FILTER
          case WM_KEYDOWN:
            return pEvent->PreKeyDown();
          case WM_KEYUP:
            return pEvent->PreKeyUp();
          case WM_CHAR:
            return pEvent->PreChar();
# endif
# ifdef MOUSE_EVENT_FILTER
          case WM_LBUTTONDOWN:
          case WM_MBUTTONDOWN:
          case WM_RBUTTONDOWN:
              return pEvent->PreButtonDown();
          case WM_LBUTTONUP:
          case WM_MBUTTONUP:
          case WM_RBUTTONUP:
              return pEvent->PreButtonUp(TRUE);
          case WM_MOUSEMOVE:
              return pEvent->PreMouseMove();
          case WM_TIMER:
              return pEvent->PreTimer();
          case WM_SETFOCUS:
              return pEvent->PreSetFocus();
          case WM_KILLFOCUS:
              return pEvent->PreKillFocus();
# endif

          default:
              break;
        }
        pEvent->FreeMessage();
    }
    return FALSE;
              
#else
    return FALSE;
#endif // NEW_EVENT_MODEL
}

void DispatchInputEvent(Classjava_awt_Event *eventPtr) {
#ifdef NEW_EVENT_MODEL
    MSG* pMsg = (MSG*)eventPtr->data;

    // Update message fields with any changes in the Event.
    // Currently only key and mouse position substitution is supported.
    switch (eventPtr->id) {
# ifdef KEYBOARD_EVENT_FILTER
      case java_awt_Event_KEY_EVENT:
      case java_awt_Event_KEY_PRESS:
      case java_awt_Event_KEY_RELEASE:
      case java_awt_Event_KEY_ACTION:
      case java_awt_Event_KEY_ACTION_RELEASE:
          pMsg->wParam = JavaCharToWindows(eventPtr->key);
          break;
# endif
# ifdef MOUSE_EVENT_FILTER
      case java_awt_Event_MOUSE_EVENT:
      case java_awt_Event_MOUSE_DOWN:
      case java_awt_Event_MOUSE_UP:
      case java_awt_Event_MOUSE_MOVE:
      case java_awt_Event_MOUSE_ENTER:
      case java_awt_Event_MOUSE_EXIT:
      case java_awt_Event_MOUSE_DRAG:
          pMsg->lParam = MAKELONG(eventPtr->x, eventPtr->y);
          break;
# endif
      default:
          return;
    }

    // Forward this message to the main thread for processing.
    VERIFY(PostThreadMessage(GetApp()->m_threadID, WM_AWT_DISPATCH_EVENT, 
                             FILTER_SIGNATURE, (LPARAM)pMsg));
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Helper routines

static UINT JavaCharToWindows(UINT nChar) {
    switch (nChar) {
      case java_awt_Event_HOME:
          return VK_HOME;
      case java_awt_Event_END:
          return VK_END;
      case java_awt_Event_PGUP:
          return VK_PRIOR;
      case java_awt_Event_PGDN:
          return VK_NEXT;
      case java_awt_Event_UP:
          return VK_UP;
      case java_awt_Event_DOWN:
          return VK_DOWN;
      case java_awt_Event_RIGHT:
          return VK_RIGHT;
      case java_awt_Event_LEFT:
          return VK_LEFT;
      case java_awt_Event_F1:
          return VK_F1;
      case java_awt_Event_F2:
          return VK_F2;
      case java_awt_Event_F3:
          return VK_F3;
      case java_awt_Event_F4:
          return VK_F4;
      case java_awt_Event_F5:
          return VK_F5;
      case java_awt_Event_F6:
          return VK_F6;
      case java_awt_Event_F7:
          return VK_F7;
      case java_awt_Event_F8:
          return VK_F8;
      case java_awt_Event_F9:
          return VK_F9;
      case java_awt_Event_F10:
          return VK_F10;
      case java_awt_Event_F11:
          return VK_F11;
      case java_awt_Event_F12:
          return VK_F12;
      case '\177':
          return VK_DELETE;
      case '\n':
          return VK_RETURN;
      default:
          return nChar;
    } 
}

/////////////////////////////////////////////////////////////////////////////
// Class methods

AwtEvent::AwtEvent(Hjava_awt_Component* pObject) {
    pJavaObject = pObject;
    pMsg = NULL;
    m_bMouseDown = FALSE;
    m_bMouseInside = FALSE;
    m_bCurWndIsChild = FALSE;
    m_curPt.x = 0, m_curPt.y = 0;
    m_hCurWnd = NULL;
}

// Copy an external message into our event.  This memory is either freed
// by CAwtApp::PreTranslateMessage if the event was routed to the applet,
// or in FreeMessage() below.
void AwtEvent::CopyMessage(MSG* pNewMsg) {
    pMsg = (MSG*)malloc(sizeof(MSG));
    memcpy(pMsg, pNewMsg, sizeof(MSG));
}

// Free the message copy.  This needs to be done if it isn't set to Java
// as event data.
void AwtEvent::FreeMessage() {
    free(pMsg);
    pMsg = NULL;
}

BOOL AwtEvent::PreKeyDown()
{
    UINT nChar = pMsg->wParam;
    UINT nFlags = HIWORD(pMsg->lParam);
    clickCount = 0;
    char *upCall = "handleActionKey";

    switch (nChar) {
    case VK_HOME:
        nChar = java_awt_Event_HOME;
        break;
    case VK_END:
        nChar = java_awt_Event_END;
        break;
    case VK_PRIOR:
        nChar = java_awt_Event_PGUP;
        break;
    case VK_NEXT:
        nChar = java_awt_Event_PGDN;
        break;
    case VK_UP:
        nChar = java_awt_Event_UP;
        break;
    case VK_DOWN:
        nChar = java_awt_Event_DOWN;
        break;
    case VK_RIGHT:
        nChar = java_awt_Event_RIGHT;
        break;
    case VK_LEFT:
        nChar = java_awt_Event_LEFT;
        break;
    case VK_F1:
        nChar = java_awt_Event_F1;
        break;
    case VK_F2:
        nChar = java_awt_Event_F2;
        break;
    case VK_F3:
        nChar = java_awt_Event_F3;
        break;
    case VK_F4:
        nChar = java_awt_Event_F4;
        break;
    case VK_F5:
        nChar = java_awt_Event_F5;
        break;
    case VK_F6:
        nChar = java_awt_Event_F6;
        break;
    case VK_F7:
        nChar = java_awt_Event_F7;
        break;
    case VK_F8:
        nChar = java_awt_Event_F8;
        break;
    case VK_F9:
        nChar = java_awt_Event_F9;
        break;
    case VK_F10:
        nChar = java_awt_Event_F10;
        break;
    case VK_F11:
        nChar = java_awt_Event_F11;
        break;
    case VK_F12:
        nChar = java_awt_Event_F12;
        break;
    case VK_DELETE:
        nChar = '\177';
        upCall = "handleKeyPress";
        break;
    default:
        FreeMessage();
        return FALSE;
        break;
    } 
    AWT_TRACE(("[%x]:%s(%d, %d, %d, %d)\n", pMsg->hwnd, upCall, 
               m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags)));
    __int64 tm = nowMillis();		// kludge for passing in 64-bit values
    GetApp()->DoCallback(pJavaObject, upCall, "(JIIIII)V", 7, 
                         (long)(tm&0xffffffff), (long)(tm>>32), (long)pMsg, 
                         m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags));
    return TRUE;
}

BOOL AwtEvent::PreKeyUp()
{
    UINT nChar = pMsg->wParam;
    UINT nFlags = HIWORD(pMsg->lParam);
    char *upCall = "handleActionKeyRelease";
    long modifiers = GetModifiers(nFlags);
    clickCount = 0;

    switch (nChar) {
    case VK_HOME:
        nChar = java_awt_Event_HOME;
        break;
    case VK_END:
        nChar = java_awt_Event_END;
        break;
    case VK_PRIOR:
        nChar = java_awt_Event_PGUP;
        break;
    case VK_NEXT:
        nChar = java_awt_Event_PGDN;
        break;
    case VK_UP:
        nChar = java_awt_Event_UP;
        break;
    case VK_DOWN:
        nChar = java_awt_Event_DOWN;
        break;
    case VK_RIGHT:
        nChar = java_awt_Event_RIGHT;
        break;
    case VK_LEFT:
        nChar = java_awt_Event_LEFT;
        break;
    case VK_F1:
        nChar = java_awt_Event_F1;
        break;
    case VK_F2:
        nChar = java_awt_Event_F2;
        break;
    case VK_F3:
        nChar = java_awt_Event_F3;
        break;
    case VK_F4:
        nChar = java_awt_Event_F4;
        break;
    case VK_F5:
        nChar = java_awt_Event_F5;
        break;
    case VK_F6:
        nChar = java_awt_Event_F6;
        break;
    case VK_F7:
        nChar = java_awt_Event_F7;
        break;
    case VK_F8:
        nChar = java_awt_Event_F8;
        break;
    case VK_F9:
        nChar = java_awt_Event_F9;
        break;
    case VK_F10:
        nChar = java_awt_Event_F10;
        break;
    case VK_F11:
        nChar = java_awt_Event_F11;
        break;
    case VK_F12:
        nChar = java_awt_Event_F12;
        break;
    case VK_DELETE:
        nChar = '\177';
        upCall = "handleKeyRelease";
        break;
    case VK_RETURN:
        nChar = '\n';
        upCall = "handleKeyRelease";
        break;
    default:
        if (!translateToAscii(&nChar, modifiers)) {
            FreeMessage();
            return FALSE;
        }
        upCall = "handleKeyRelease";
        break;
    } 
    AWT_TRACE(("[%x]:%s(%d, %d, %d, %d)\n", 
               pMsg->hwnd, upCall, m_curPt.x, m_curPt.y, nChar, modifiers));
    __int64 tm = nowMillis();		// kludge for passing in 64-bit values
    GetApp()->DoCallback(pJavaObject, upCall, "(JIIIII)V", 7,
                         (long)(tm&0xffffffff), (long)(tm>>32), (long)pMsg, 
                         m_curPt.x, m_curPt.y, nChar, modifiers);
    return TRUE;
}

BOOL AwtEvent::PreChar()
{
    UINT nChar = pMsg->wParam;
    UINT nFlags = HIWORD(pMsg->lParam);
    clickCount = 0;
    switch (nChar) {
    case VK_RETURN:	// Enter
        nChar = '\n';
        break;
    case VK_DELETE:
        FreeMessage();
        return FALSE;
        break;
    default:
        break;
    } 
    AWT_TRACE(("[%x]:handleKeyPress(%d, %d, %d, %d)\n", 
               pMsg->hwnd, m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags)));
    __int64 tm = nowMillis();		// kludge for passing in 64-bit values
    GetApp()->DoCallback(pJavaObject, "handleKeyPress", "(JIIIII)V", 7, 
                         (long)(tm&0xffffffff), (long)(tm>>32), (long)pMsg,
                         m_curPt.x, m_curPt.y, nChar, GetModifiers(nFlags));
    return TRUE;
}

// Convert screen coordinates to coordinates relative to the window's frame,
// if the parent is a frame.  The point is converted to client coordinates
// if the parent isn't a frame.  All of this is necessary because of our
// screwy inset logic.
static void ScreenToFrame(HWND hwnd, LPPOINT lpPoint) {
    CWnd* pWnd = CWnd::FromHandlePermanent(hwnd);
    if (pWnd == NULL) {
        ::ScreenToClient(hwnd, lpPoint);
        return;
    }
    CFrameWnd* pFrame = pWnd->GetParentFrame();
    if (pFrame == NULL || pFrame != pWnd->GetParent()) {
        ::ScreenToClient(hwnd, lpPoint);
        return;
    }
    pFrame->ScreenToClient(lpPoint);
}

BOOL AwtEvent::PreButtonDown()
{
    POINT pt = pMsg->pt;
    ScreenToFrame(pMsg->hwnd, &pt);

    if (m_bMouseDown) {	
        // Missed the buttonUp so simulate it at this point.
        PreButtonUp(FALSE);
    }
    if (!m_bMouseInside) {
        // Simulate a mouse move to this position since we missed it.
        DoMouseMove(pMsg->wParam, pt, FALSE);
    }

    m_bMouseDown = TRUE;
    AWT_TRACE(("[%x]:handleMouseDown(%d, %d, %d)\n", 
               pMsg->hwnd, pt.x, pt.y, GetModifiers(pMsg->wParam)));
    __int64 tm = nowMillis();
    GetApp()->DoCallback(pJavaObject, "handleMouseDown", "(JIIIII)V", 7,
                         (long)(tm&0xffffffff), (long)(tm>>32), (long)pMsg, 
                         pt.x, pt.y, GetClickCount(), 
                         GetModifiers(pMsg->wParam));
    return TRUE;
}

BOOL AwtEvent::PreButtonUp(BOOL fPostMsg)
{
    // Ignore button up if the button down was in another window.
    if (!m_bMouseDown) {
        FreeMessage();
        return FALSE;
    }
	
    POINT pt = pMsg->pt;
    ScreenToFrame(pMsg->hwnd, &pt);

    // Simulate a mouse move to this position just in case we missed it.
    DoMouseMove(pMsg->wParam, pt, FALSE);

    m_bMouseDown = FALSE;
    AWT_TRACE(("[%x]:handleMouseUp(%d, %d, %d)\n", 
               pMsg->hwnd, pt.x, pt.y, GetModifiers(pMsg->wParam)));
    __int64 tm = nowMillis();
    GetApp()->DoCallback(pJavaObject, "handleMouseUp", "(JIIII)V", 6, 
                         (long)(tm&0xffffffff), (long)(tm>>32), 
                         (fPostMsg) ? (long)pMsg : NULL, 
                         pt.x, pt.y, GetModifiers(pMsg->wParam));
    return TRUE;
}

BOOL AwtEvent::PreMouseMove()
{
    UINT nFlags = pMsg->wParam;
    POINT pt = pMsg->pt;
    ScreenToFrame(pMsg->hwnd, &pt);

    if ((nFlags&(MK_LBUTTON|MK_MBUTTON|MK_RBUTTON)) && !m_bMouseDown) {
        // The button down message was missed.  This can happen if the window
        // got the focus while some button was down.  Simulate a button down.
        m_bMouseDown = TRUE;
        AWT_TRACE(("[%x]:handleMouseDown(%d, %d, %d)\n", 
                   pMsg->hwnd, pt.x, pt.y, GetModifiers(nFlags)));
        __int64 tm = nowMillis();
        GetApp()->DoCallback(pJavaObject, "handleMouseDown", "(JIIIII)V", 7,
                             (long)(tm&0xffffffff), (long)(tm>>32), 
                             NULL, pt.x, pt.y, 
                             GetClickCount(), GetModifiers(nFlags));
    }

    DoMouseMove(nFlags, pt, TRUE);
    return TRUE;
}

void AwtEvent::DoMouseMove(UINT nFlags, POINT pt, BOOL fPostMsg)
{
    CRect domain;
    BOOL bMouseInside;
  
    // test if pt is in visible portion of window or in any one of its 
    // descendents
    ::GetClientRect(pMsg->hwnd, domain);
    bMouseInside = domain.PtInRect(pt);
    if (bMouseInside) {
        // is inside the domain but is it on some descendent?
        HWND hCurrentWnd;
        CPoint screenPt = pt;

        ::ClientToScreen(pMsg->hwnd, &screenPt);
        hCurrentWnd = ::WindowFromPoint(screenPt);
  	
        if (hCurrentWnd != m_hCurWnd) {
            m_bCurWndIsChild = 
                (hCurrentWnd == pMsg->hwnd) || ::IsChild(pMsg->hwnd, hCurrentWnd);
            m_hCurWnd = hCurrentWnd;
        }
        bMouseInside = m_bCurWndIsChild;
    }

    __int64 tm = nowMillis();
    if (bMouseInside && !m_bMouseInside) {
        m_bMouseInside = TRUE;
        AWT_TRACE(("[%x]:handleMouseEnter(%d, %d, %d)\n", 
                   pMsg->hwnd, pt.x, pt.y, GetModifiers(pMsg->wParam)));
        GetApp()->DoCallback(pJavaObject, "handleMouseEnter", "(JII)V", 4, 
                             (long)(tm&0xffffffff), (long)(tm>>32), 
                             pt.x, pt.y);
    } else if (!bMouseInside && m_bMouseInside) {
        m_bMouseInside = FALSE;
        AWT_TRACE(("[%x]:handleMouseExit(%d, %d, %d)\n", 
                   pMsg->hwnd, pt.x, pt.y, GetModifiers(pMsg->wParam)));
        GetApp()->DoCallback(pJavaObject, "handleMouseExit", "(JII)V", 4, 
                             (long)(tm&0xffffffff), (long)(tm>>32), 
                             pt.x, pt.y);
    }
    if (m_bMouseDown) {
        AWT_TRACE(("[%x]:handleMouseDrag(%d, %d, %d)\n", pMsg->hwnd, 
                   pt.x, pt.y, GetModifiers(pMsg->wParam)));
        GetApp()->DoCallback(pJavaObject, "handleMouseDrag", "(JIIII)V",
                             6, (long)(tm&0xffffffff), (long)(tm>>32), 
                             (fPostMsg) ? (long)pMsg : NULL, 
                             pt.x, pt.y, GetModifiers(pMsg->wParam));
    } else if (bMouseInside) {
        AWT_TRACE(("[%x]:handleMouseMoved(%d, %d, %d)\n", 
                   pMsg->hwnd, pt.x, pt.y, GetModifiers(pMsg->wParam)));
        GetApp()->DoCallback(pJavaObject, "handleMouseMoved", "(JIIII)V", 
                             6, (long)(tm&0xffffffff), (long)(tm>>32), 
                             (fPostMsg) ? (long)pMsg : NULL, 
                             pt.x, pt.y, GetModifiers(pMsg->wParam));
    } else if (fPostMsg) {
        FreeMessage();
    }
    m_curPt = pt;
}

BOOL AwtEvent::PreTimer()
{
    UINT nIDEvent = pMsg->wParam;

    // If button down, drag messages will be generated from
    // move messages.
    if (!m_bMouseDown && nIDEvent == ID_TIMER) {
        POINT pt;
        if (::GetCursorPos(&pt)) {
            ScreenToFrame(pMsg->hwnd, &pt);
            if (m_curPt != pt) {
                DoMouseMove(0, pt, FALSE);
            }
        }
    }
    FreeMessage();
    return FALSE;
}

BOOL AwtEvent::PreSetFocus()
{
    // Simulate mouse move in case we missed it.
    CPoint pt;
    if (::GetCursorPos(&pt)) {
        ScreenToFrame(pMsg->hwnd, &pt);
        DoMouseMove(0, pt, FALSE);
    }

    AWT_TRACE(("[%x]:gotFocus()\n", pMsg->hwnd));
    GetApp()->DoCallback(pJavaObject, "gotFocus", "(I)V", NULL);
    FreeMessage();
    return FALSE;
}

BOOL AwtEvent::PreKillFocus()
{
    DoMouseMove(0, CPoint(-1, -1), FALSE);

    // Simulate mouseUp.
    if (m_bMouseDown) {
        m_bMouseDown = FALSE;
        CALLINGWINDOWS(pMsg->hwnd);
        ::ReleaseCapture();
        AWT_TRACE(("[%x]:handleMouseUp(%d, %d, %d)\n", pMsg->hwnd, -1, -1, 0));
        __int64 tm = nowMillis();
        GetApp()->DoCallback(pJavaObject, "handleMouseUp", "(JIIII)V", 6,
                             (long)(tm&0xffffffff), (long)(tm>>32), NULL, 
                             -1, -1, 0);
    }
    AWT_TRACE(("[%x]:lostFocus()\n", pMsg->hwnd));
    GetApp()->DoCallback(pJavaObject, "lostFocus", "(I)V", 1, NULL);
    m_bMouseInside = FALSE;
    m_hCurWnd = NULL;
    FreeMessage();
    return FALSE;
}
