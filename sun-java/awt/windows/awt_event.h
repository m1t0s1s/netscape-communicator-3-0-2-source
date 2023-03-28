/*
 * @(#)awt_event.h	1.5 95/12/08 Thomas Ball
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

#ifndef _AWTEVENT_H_
#define _AWTEVENT_H_

#include "stdafx.h"
#include "awt.h"

// Enable the following lines to turn on the new input event routing:
// NEW_EVENT_MODEL:  general input event filtering support.
// KEYBOARD_EVENT_FILTER:  code specific to key events
// MOUSE_EVENT_FILTER:  code specific to mouse events.
#define NEW_EVENT_MODEL
#define KEYBOARD_EVENT_FILTER
//#define MOUSE_EVENT_FILTER

#define FILTER_SIGNATURE 25549

class AwtEvent {
public:
    MSG* pMsg;
    Hjava_awt_Component* pJavaObject;

    AwtEvent(Hjava_awt_Component* pObject);
    void CopyMessage(MSG* pMsg);
    void FreeMessage();
    Hsun_awt_win32_MComponentPeer* pPeer() {
        return (Hsun_awt_win32_MComponentPeer *)unhand(pJavaObject)->peer;
    };

    // Mouse variables
    BOOL m_bMouseDown;
    BOOL m_bMouseInside;
    CPoint m_curPt;
    HWND m_hCurWnd;
    BOOL m_bCurWndIsChild;

    BOOL PreKeyDown();
    BOOL PreKeyUp();
    BOOL PreChar();
    BOOL PreButtonDown();
    BOOL PreButtonUp(BOOL fPostMsg);
    BOOL PreMouseMove();
    BOOL PreTimer();
    BOOL PreSetFocus();
    BOOL PreKillFocus();
    void DoMouseMove(UINT nFlags, POINT pt, BOOL fPostMsg);
};

extern BOOL HandleInputEvent(MSG *pMsg);
extern void DispatchInputEvent(Classjava_awt_Event *eventPtr);

/////////////////////////////////////////////////////////////////////////////

#endif // _AWTEVENT_H
