/*
 * @(#)awt_menu.cpp	1.20 95/11/27  Thomas Ball
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
#include "stdafx.h"

#include "awt.h"
#include "awt_menu.h"
#include "awt_mainfrm.h"

IMPLEMENT_DYNCREATE(AwtMenu, CMenu)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/destruction

AwtMenu::AwtMenu()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtMenu");
    m_pFrame = NULL;
    m_hMenuBar = NULL;
    m_pJavaObject = NULL;
}

AwtMenu::~AwtMenu()
{
    ((CAwtApp *)AfxGetApp())->cleanupListRemove(this);
    DestroyMenu();
    m_hMenu = NULL;

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

/////////////////////////////////////////////////////////////////////////////
// AwtMenu operations

long AwtMenu::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
						long p4, long p5, long p6, long p7, long p8) {
	switch (nMsg) {
		case CREATE:				
			return (long)Create((void*)p1, (MainFrame*)p2, (HMENU)p3,
					(char*)p4);
			break;
		case DISPOSE:
			((AwtMenu *)p1)->Dispose();
			break;
		case APPENDITEM:
			return ((AwtMenu *)p1)->AppendItem((Hjava_awt_MenuItem*)p2, (char*)p3);
			break;
		case APPENDSEPARATOR:
			((AwtMenu *)p1)->AppendSeparator();
			break;
		case ENABLE:
			((AwtMenu *)p1)->Enable((BOOL)p2);
			break;
		case ENABLEITEM:
			((AwtMenu *)p1)->EnableItem((UINT)p2, (BOOL)p3);
			break;
		case ISENABLED:
			return ((AwtMenu *)p1)->IsEnabled((UINT)p2);
			break;
		case SETMARK:
			((AwtMenu *)p1)->SetMark((UINT)p2, (BOOL)p3);
			break;
		case ISMARKED:
			return ((AwtMenu *)p1)->IsMarked((UINT)p2);
			break;
		case DISPOSEITEM:
			((AwtMenu *)p1)->DisposeItem((UINT)p2);
			break;
		default:
			ASSERT(FALSE);
	}
	return 0;
}

AwtMenu *AwtMenu::Create(void *pJavaObject, MainFrame *pFrame, HMENU hMenuBar, 
                         char *pzLabel)
{
    CAwtApp* pApp = (CAwtApp *)AfxGetApp();
    if (!pApp->IsServerThread()) {
        return (AwtMenu*)((CAwtApp *)AfxGetApp())->SendAwtMsg(
            CAwtApp::AWT_MENU, CREATE, (long)pJavaObject, 
            (long)pFrame, (long)hMenuBar, (long)pzLabel);
    }
    AwtMenu *pMenu = new AwtMenu;

    pMenu->m_pJavaObject = pJavaObject;
    pMenu->m_pFrame = pFrame;
    pMenu->m_hMenuBar = hMenuBar;
    VERIFY(pMenu->CreateMenu());
    pApp->cleanupListAdd(pMenu);

    // Insert menu into menu bar, keeping the help menu (if any) last.
    int nMenus = ::GetMenuItemCount(hMenuBar);
    BOOL fInserted = FALSE;
    for (int i = 0; i < nMenus; i++) {
        HMENU hSubMenu = ::GetSubMenu(hMenuBar, i);
        if (hSubMenu == NULL) {
            // It's not a popup menu, so it can't be the Help menu.
            break;
        }
        AwtMenu *pSubMenu = (AwtMenu*)FromHandle(hSubMenu);
        if (pSubMenu == NULL) {
            // Not an AwtMenu.
            break;
        }
        Hjava_awt_Menu *hmenu = (Hjava_awt_Menu *)(pSubMenu->m_pJavaObject);
        if (unhand(hmenu)->isHelpMenu) {
            VERIFY(::InsertMenu(hMenuBar, i, 
                                MF_BYPOSITION | MF_POPUP,
                                (UINT)pMenu->GetSafeHmenu(), pzLabel));
            fInserted = TRUE;
            break;
        }
    }
    if (fInserted == FALSE) {
	VERIFY(::AppendMenu(hMenuBar, MF_POPUP, (UINT)pMenu->GetSafeHmenu(), 
                            pzLabel));
    }
    pFrame->DrawMenuBar();
    return pMenu;
}

UINT AwtMenu::AppendItem(Hjava_awt_MenuItem *pItemObject, char *pzItemName)
{
	UINT result = m_pFrame->CreateCmdID(this);

	AppendMenu(MF_STRING, result, pzItemName);
	m_ItemMap.SetAt(result, pItemObject);
	return result;
}


void AwtMenu::AppendSeparator()
{
	AppendMenu(MF_SEPARATOR);
}

void AwtMenu::InvokeItem(UINT nCmdID)
{
    void* p;
    if (m_ItemMap.Lookup(nCmdID, p)) {
        Hjava_awt_MenuItem *pHMenuItem = (Hjava_awt_MenuItem*)p;
        Classjava_awt_MenuItem *pClass = (Classjava_awt_MenuItem*)unhand(pHMenuItem);
        Hsun_awt_win32_MMenuItemPeer *pPeer = 
            (Hsun_awt_win32_MMenuItemPeer*)pClass->peer;
        Classsun_awt_win32_MMenuItemPeer *pPeerClass = 
            (Classsun_awt_win32_MMenuItemPeer*)unhand(pPeer);
        BOOL isMarked = IsMarked(nCmdID);

        if (pPeerClass->isCheckbox) {
            AWT_TRACE(("%x:menu.action(%d)\n", pPeer, !isMarked));
            GetApp()->DoCallback(pHMenuItem, "action", "(Z)V", 1, !isMarked);
        } else {
            AWT_TRACE(("%x:menu.action()\n", pPeer));
            GetApp()->DoCallback(pHMenuItem, "action", "()V");
        }
    } else {
        ASSERT(FALSE);
    }
}

void AwtMenu::Enable(BOOL bEnable)
{
    int nMenus = ::GetMenuItemCount(m_hMenuBar);
    HMENU my_hmenu = GetSafeHmenu();
    for (int i = 0; i < nMenus; i++) {
        HMENU hSubMenu = ::GetSubMenu(m_hMenuBar, i);
        if (hSubMenu == my_hmenu) {
            VERIFY(::EnableMenuItem(m_hMenuBar, i, MF_BYPOSITION
                                    |(bEnable ? MF_ENABLED : MF_GRAYED)));
	    break;
        }
    }
}

BOOL AwtMenu::IsEnabled(UINT nCmdID)
{
    UINT nState = GetMenuState(nCmdID, MF_BYCOMMAND);
    return (nState & MF_ENABLED) != 0;
}

void AwtMenu::EnableItem(UINT nCmdID, BOOL bEnable)
{
    BOOL bIsMarked = IsMarked(nCmdID);
    VERIFY(EnableMenuItem(nCmdID, 
                          MF_BYCOMMAND
                          |(bEnable ? MF_ENABLED : MF_GRAYED)
                          |(bIsMarked ? MF_CHECKED : MF_UNCHECKED)) != -1);
}

void AwtMenu::SetMark(UINT nCmdID, BOOL bMark)
{
    CheckMenuItem(nCmdID, bMark ? MF_CHECKED : MF_UNCHECKED);
}

BOOL AwtMenu::IsMarked(UINT nCmdID)
{
    UINT nState = GetMenuState(nCmdID, MF_BYCOMMAND);
    return (nState & MF_CHECKED) != 0;
}

void AwtMenu::DisposeItem(UINT nCmdID)
{
    DeleteMenu(nCmdID, MF_BYCOMMAND);
    VERIFY(m_ItemMap.RemoveKey(nCmdID));
    m_pFrame->DrawMenuBar();
    m_pFrame->DeleteCmdValue(nCmdID);
}

void AwtMenu::Dispose()
{
    int nMenus = ::GetMenuItemCount(m_hMenuBar);
    HMENU my_hmenu = GetSafeHmenu();
    for (int i = 0; i < nMenus; i++) {
        HMENU hSubMenu = ::GetSubMenu(m_hMenuBar, i);
        if (hSubMenu == my_hmenu) {
	    VERIFY(::RemoveMenu(m_hMenuBar, i, MF_BYPOSITION));
	    break;
        }
    }
    m_pFrame->DrawMenuBar();
    delete this;
}
