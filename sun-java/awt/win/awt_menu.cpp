#include "awt_menu.h"
#include "awt_toolkit.h"        // RaiseJavaEvent
#include "awt_frame.h"
#include "awt_defs.h"
#ifdef INTLUNICODE
#include "awt_wcompstr.h"
#endif

// All java header files are extern "C"
extern "C" {
#include "java_awt_Menu.h"
#include "java_awt_Menubar.h"
#include "java_awt_MenuItem.h"
#include "java_awt_CheckboxMenuItem.h"
#include "java_awt_Frame.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WFramePeer.h"
#include "sun_awt_windows_WMenuPeer.h"
#include "sun_awt_windows_WMenuBarPeer.h"
#include "sun_awt_windows_WMenuItemPeer.h"
#include "sun_awt_windows_WCheckboxMenuItemPeer.h"
};


//-------------------------------------------------------------------------
//
// Constructors and destructor
//
//-------------------------------------------------------------------------
AwtMenu::AwtMenu(JHandle* peer) : AwtObject(peer)
{
    m_hMenu = 0;
    m_Owner = 0;
    m_cIDs = 1; // in sub-menus keeps the id 
    m_cPositions = 0; // position generator
}


AwtMenu::AwtMenu(HWMenuPeer* peer) : AwtObject((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("MEN");

    SET_OBJ_IN_MENU_PEER((HWMenuPeer *)peer, this);

    m_hMenu = 0;
    m_Owner = 0;
    m_cIDs = 0; // in sub-menus keeps the id 
    m_cPositions = 0; // position generator
}


AwtMenu::~AwtMenu()
{
    if (m_pJavaPeer) {
        SET_OBJ_IN_MENU_PEER((HWMenuPeer *)m_pJavaPeer, NULL);
        m_pJavaPeer = NULL;
    }

    if (m_hMenu)
        ::DestroyMenu(m_hMenu);

}


//-------------------------------------------------------------------------
//
// asynchronuos call to create menus in the gui thread
//
//-------------------------------------------------------------------------
BOOL AwtMenu::CallMethod(AwtMethodInfo *info)
{
    BOOL bRet = TRUE;

    switch (info->methodId) {
      case AwtMenu::CREATEMENU:
        CreateMenu((HWND)(info->p1));
        break;

      case AwtMenu::CREATEPOPUP:
        CreateMenu((AwtMenu*)(info->p1), (Hjava_lang_String*)(info->p2));
        break;

      default:
        bRet = FALSE;
        break;
    }

    return bRet;
    
}


// -----------------------------------------------------------------------
//
// Create a menu for an AwtMenu
//
// This method MUST be executed on the "main gui thread".  Since this
// method is ALWAYS executed on the same thread no locking is necessary.
//
// -----------------------------------------------------------------------
void AwtMenu::CreateMenu(AwtMenu* owner, Hjava_lang_String* menuName)
{
#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread" because it creates a window...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtMenu::CREATEPOPUP, (long)owner, (long)menuName);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }
#endif

    //
    // This method is now being executed on the "main gui thread"
    //
    ASSERT( AwtToolkit::theToolkit->IsGuiThread() );

    //!DR check for out of memory/resource problems
    //
    // Create the new menu and notify the owner
    //
    m_hMenu = ::CreatePopupMenu();
    m_Owner = owner;
    // m_cIDs keep the id of this menu
    m_cIDs = owner->AddMenu(menuName, (HWMenuItemPeer*)m_pJavaPeer, this);

    owner->UpdateMenu();

}


//-------------------------------------------------------------------------
//
// Add a menu or menu item
//
//-------------------------------------------------------------------------
UINT AwtMenu::AddMenu(Hjava_lang_String* menuRefName, HWMenuItemPeer* peer, AwtMenu* menu)
{
    UINT cID;
    UINT cFlags = 0;

#ifdef INTLUNICODE
    WinMenuCompStr	*pStr = NULL;
    pStr = new WinMenuCompStr(menuRefName);    
    if (pStr) {

        Lock();

        // update position so that sub-menus get the right position if created
        cID = m_cPositions++;

        if (menu) {
            cFlags |= MF_POPUP | MF_OWNERDRAW;
            // add menu to the list of submenus
            m_OwnerLink.Append(menu->GetListEl());
        }
        else {
            if (pStr->IsMenuSeparator())
                cFlags |= MF_SEPARATOR;
            else {
                cFlags |= MF_OWNERDRAW;
            }
            // reassign cID if here so the right id gets assigned
            cID = AddPeer(peer);
        }
	
	char FAR* p = (char FAR*)pStr;	
        ::AppendMenu(m_hMenu,
                        cFlags,
                        (menu) ? (UINT)(HMENU)*menu : cID,
                        (cFlags & MF_SEPARATOR ) ? "-" : p);
        if(cFlags & MF_SEPARATOR)
        {
            delete pStr;
        }
        else
        {
            /* add the pStr to the m_stringList so we could delete it in the
               m_stringList deconstructor */
            m_stringList.AddItem( 
                (menu) ? (UINT)(HMENU)*menu : cID,
                pStr); 				/* string */
        }

        Unlock();

    }
    else
        cID = 0;

#else	// INTLUNICODE
    char* pszMenuRefItem;
    int32_t len;

    len = javaStringLength(menuRefName);
    pszMenuRefItem = (char *)malloc(len+1);
    
    if (pszMenuRefItem) {
        HMENU hm;

        javaString2CString(menuRefName, pszMenuRefItem, len+1);

        Lock();

        // update position so that sub-menus get the right position if created
        cID = m_cPositions++;

        if (menu) {
            cFlags |= MF_POPUP;
            // add menu to the list of submenus
            m_OwnerLink.Append(menu->GetListEl());
        }
        else {
            if (!strcmp(pszMenuRefItem, "-"))
                cFlags |= MF_SEPARATOR;
            // reassign cID if here so the right id gets assigned
            cID = AddPeer(peer);
        }

        hm = m_hMenu;

        Unlock();

        ::AppendMenu(hm,
                        cFlags,
                        (menu) ? (UINT)(HMENU)*menu : cID,
                        pszMenuRefItem);


        free(pszMenuRefItem);
    }
    else
        cID = 0;
#endif	// INTLUNICODE
    return cID;
}


//-------------------------------------------------------------------------
//
// Enable or disable a menu
//
//-------------------------------------------------------------------------
void AwtMenu::Enable(long state)
{
    // dispatch to the owner
    m_Owner->EnableItem(m_cIDs, state, MF_BYPOSITION);
}


//-------------------------------------------------------------------------
//
// Enable or disable a menu item
//
//-------------------------------------------------------------------------
void AwtMenu::EnableItem(UINT id, long state, UINT cFlag)
{
    HMENU hm;

    Lock();

    hm  = m_hMenu;

    Unlock();

    cFlag |= (state) ? MF_ENABLED : MF_GRAYED;
    ::EnableMenuItem(hm, id, cFlag);
}


//-------------------------------------------------------------------------
//
// Change a menu label
//
//-------------------------------------------------------------------------
void AwtMenu::Modify(Hjava_lang_String* itemName)
{
    m_Owner->ModifyItem(m_cIDs, itemName);
}
    

//-------------------------------------------------------------------------
//
// Change a menu item label
//
//-------------------------------------------------------------------------
void AwtMenu::ModifyItem(UINT id, Hjava_lang_String* itemName)
{
#ifdef INTLUNICODE
    WinMenuCompStr *pStr = new WinMenuCompStr(itemName);
    if(pStr)
    {
        char FAR* p = (char FAR*)pStr;
        ::ModifyMenu( m_hMenu, 
			id,
            MF_OWNERDRAW,
            0,
            p);
        /* add the pStr to the m_stringList so we could delete it in the
           m_stringList deconstructor */
        m_stringList.ModifyItem(id, pStr);
    }
#else  // INTLUNICODE
    char* pszNewItemName;
    int32_t len;

    len = javaStringLength(itemName);
    pszNewItemName = (char *)malloc(len+1);
    
    if (pszNewItemName) {

        javaString2CString(itemName, pszNewItemName, len+1);

        ::ModifyMenu( m_hMenu, 
                        id,
                        MF_STRING,
                        0,
                        pszNewItemName);

        free(pszNewItemName);
    }
#endif	// INTLUNICODE
}


//-------------------------------------------------------------------------
//
// Remove a menu
//
//-------------------------------------------------------------------------
void AwtMenu::Remove()
{

    m_Owner->RemoveItem(m_cIDs, MF_BYPOSITION);

    m_link.Remove();
    m_Owner->UpdatePositions(m_cIDs);

    UpdateMenu();
    
}


//-------------------------------------------------------------------------
//
// Remove a menu item. Just zero the peer in the array
//
//-------------------------------------------------------------------------
void AwtMenu::RemoveItem(UINT id, UINT cFlag)
{
    Lock();

    ::RemoveMenu(m_hMenu, id, cFlag);

    if (cFlag == 0)
        // it's a menu item, remove from the array of peers
        RemovePeer(id);

    m_cPositions--;

    Unlock();
}


//-------------------------------------------------------------------------
//
// Update a menu
//
//-------------------------------------------------------------------------
void AwtMenu::UpdateMenu()
{
    if (m_Owner)
        m_Owner->UpdateMenu();
}


//-------------------------------------------------------------------------
//
// Check or uncheck a menu item
//
//-------------------------------------------------------------------------
void AwtMenu::SetItemState(UINT id, long state)
{
    Lock();

    ::CheckMenuItem(m_hMenu, id, (state) ? MF_CHECKED : MF_UNCHECKED);

    Unlock();
}


//-------------------------------------------------------------------------
//
// Query a menu item state
//
//-------------------------------------------------------------------------
long AwtMenu::GetState(UINT id)
{
    UINT cFlags; 

    Lock();

    cFlags = ::GetMenuState(m_hMenu, id, MF_BYCOMMAND);

    Unlock();

    return (cFlags & MF_CHECKED) ? 1 : 0;
}


//-------------------------------------------------------------------------
//
// Call AddPeer on the menu bar
//
// This method MUST be called inside a lock
//
//-------------------------------------------------------------------------
UINT AwtMenu::AddPeer(HWMenuItemPeer* itemPeer)
{
    if (m_Owner)
        return m_Owner->AddPeer(itemPeer);
    else
        return (UINT)-1;
}


//-------------------------------------------------------------------------
//
// Call RemovePeer on the menu bar
//
// Remove a peer at the specified position from the array of peers
//
//-------------------------------------------------------------------------
void AwtMenu::RemovePeer(UINT id)
{
    if (m_Owner)
        // walk up to the menu bar or main menu
        m_Owner->RemovePeer(id);

}


void AwtMenu::UpdatePositions(UINT pos)
{
    AwtCList* link = &m_OwnerLink;

    while (link->next != &m_OwnerLink) {
        if (MENU_PTR(link->next)->GetPosition() > pos)
            MENU_PTR(link->next)->SetPosition(MENU_PTR(link->next)->GetPosition() - 1);

        link = link->next;
    }
}


//-------------------------------------------------------------------------
//
// Constructors and destructor
//
//-------------------------------------------------------------------------
AwtMenuBar::AwtMenuBar(HWMenuBarPeer* peer) : AwtMenu((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("MNB");

    SET_OBJ_IN_MENU_PEER((HWMenuBarPeer *)peer, this);

    m_PeerCollection = 0;
    m_currentSize = 0;

    m_hWnd = 0;

    m_HelpMenu = 0;
}


AwtMenuBar::~AwtMenuBar()
{
    if (m_PeerCollection)
        ::free(m_PeerCollection);
}


// -----------------------------------------------------------------------
//
// Create a menu bar for an AwtMenu
//
// This method MUST be executed on the "main gui thread".  Since this
// method is ALWAYS executed on the same thread no locking is necessary.
//
// -----------------------------------------------------------------------
void AwtMenuBar::CreateMenu(HWND hFrameWnd)
{

#ifdef _WIN32
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread" because it creates a window...
    //
    if (! AwtToolkit::theToolkit->IsGuiThread()) {
        AwtMethodInfo info(this, AwtMenu::CREATEMENU, (long)hFrameWnd);
        AwtToolkit::theToolkit->CallAwtMethod( &info );
        return;
    }
#endif

    //
    // This method is now being executed on the "main gui thread"
    //
    ASSERT( AwtToolkit::theToolkit->IsGuiThread() );

    //!DR check for out of memory/resource problems
    //
    // Create the new menu. Java will call SteMenu on the frame 
    //
    m_hMenu = ::CreateMenu();

    // allocate room for the peer collection
    m_PeerCollection = (HWMenuItemPeer**) ::malloc(sizeof(HWMenuItemPeer*) * MENU_ITEMS);
    ::memset(m_PeerCollection, 0, sizeof(HWMenuItemPeer*) * MENU_ITEMS);
    m_currentSize = MENU_ITEMS;

    m_hWnd = hFrameWnd;

}


//-------------------------------------------------------------------------
//
// Add a menu to a menu bar
//
//-------------------------------------------------------------------------
UINT AwtMenuBar::AddMenu(Hjava_lang_String* menuRefName, HWMenuItemPeer* peer, AwtMenu* menu)
{
    char* pszMenuRefItem;
    int32_t len;
    UINT cID = 0;
    UINT cFlags = MF_BYPOSITION | MF_POPUP;

    len = javaStringLength(menuRefName);
    pszMenuRefItem = (char *)malloc(len+1);
    
    if (pszMenuRefItem) {

        javaString2CString(menuRefName, pszMenuRefItem, len+1);

        Lock();

        // add menu to the list of submenus
        m_OwnerLink.Append(menu->GetListEl());

#ifdef INTLUNICODE
        WinMenuCompStr	*pStr = NULL;
        pStr = new WinMenuCompStr(menuRefName);
        if(pStr)
        {
              char FAR* p = (char FAR*) pStr;
              ::InsertMenu(m_hMenu,
                        (m_HelpMenu) ? m_cPositions - 1 : (UINT)-1,
                        cFlags | MF_OWNERDRAW,
                        (UINT)(HMENU)*menu,
                        p);

             /* add the pStr to the m_stringList so we could delete it in the
                m_stringList deconstructor */
              m_stringList.AddItem( 
                  ::GetMenuItemID(m_hMenu, 
                                  (::GetMenuItemCount((HMENU)*menu) - 1)),
                  pStr); 				/* string */
        } else
#endif	// INTLUNICODE
        ::InsertMenu(m_hMenu,
                        (m_HelpMenu) ? m_cPositions - 1 : (UINT)-1,
                        cFlags,
                        (UINT)(HMENU)*menu,
                        pszMenuRefItem);

        // update position so that menus get the right position 
        cID = m_cPositions++;

        if (!stricmp(pszMenuRefItem, "help"))
            m_HelpMenu = menu;
        else if (m_HelpMenu) {
            m_HelpMenu->SetPosition(cID);
            cID--;
        }

        Unlock();

        free(pszMenuRefItem);
    }

    return cID;
}


//-------------------------------------------------------------------------
//
// Event firing
//
//-------------------------------------------------------------------------
BOOL AwtMenuBar::OnSelect(UINT id)
{
    Lock();

    if (id) {

        if (m_pJavaPeer) {
            int64 time = PR_NowMS(); // time in milliseconds

            if (m_PeerCollection[id - 1]) {
                AwtToolkit::RaiseJavaEvent((JHandle*)m_PeerCollection[id - 1], 
                                            AwtEventDescription::ButtonAction,
                                            time, 
                                            NULL);
            }
        }

    }

    Unlock();

    return TRUE;
}


//-------------------------------------------------------------------------
//
// Update a menu
//
//-------------------------------------------------------------------------
void AwtMenuBar::UpdateMenu()
{
    if (m_hWnd)
        ::DrawMenuBar(m_hWnd);
}


//-------------------------------------------------------------------------
//
// Add a menu item peer to the collection of peers.
// This collection is used to fire events back to the proper peer.
//
// This method MUST be called inside a lock
//
//-------------------------------------------------------------------------
UINT AwtMenuBar::AddPeer(HWMenuItemPeer* itemPeer)
{
    if (m_PeerCollection) {

        // add the peer at the position specified by the identifier
        if (m_cIDs == m_currentSize) {
            m_PeerCollection = (HWMenuItemPeer**)::realloc(m_PeerCollection, m_currentSize * sizeof(HWMenuItemPeer*) + 
                                                                sizeof(HWMenuItemPeer*) * MENU_ITEMS);
            ::memset(m_PeerCollection + m_currentSize, 
                        0, 
                        sizeof(HWMenuItemPeer*) * MENU_ITEMS);

            m_currentSize += MENU_ITEMS;
        }
            
        m_PeerCollection[m_cIDs - 1] = itemPeer;

        return m_cIDs++;

    }
    else
        return (UINT)-1;
}


//-------------------------------------------------------------------------
//
// Remove a peer at the specified position from the array of peers
//
//-------------------------------------------------------------------------
void AwtMenuBar::RemovePeer(UINT id)
{

    if (m_PeerCollection) {
        Lock();

        SET_OBJ_IN_MENU_PEER((HWMenuItemPeer *)m_PeerCollection[id - 1], NULL);
        m_PeerCollection[id - 1] = 0;

        Unlock();
    }

}


//-------------------------------------------------------------------------
//
// Native methods for sun.awt.windows.WComponentPeer
//
//-------------------------------------------------------------------------

extern "C" {

//
// MenuBar
//
void 
sun_awt_windows_WMenuBarPeer_create(HWMenuBarPeer* self,
                                    Hjava_awt_Frame* frame)
{

    AwtMenuBar* menuBar = new AwtMenuBar(self);

    CHECK_ALLOCATION(menuBar);

    // get the c++ frame object out of the java object
    HWFramePeer* peer = (HWFramePeer*)(unhand(frame)->peer);
    AwtFrame* frameObj = GET_OBJ_FROM_PEER(AwtFrame, peer);

    if (frameObj)

        // call create which will switch to the main gui thread
        menuBar->CreateMenu(frameObj->GetWindowHandle());

}


void 
sun_awt_windows_WMenuBarPeer_dispose(HWMenuBarPeer* self)
{
    CHECK_MENU_PEER(self);
    AwtMenuBar *obj = GET_OBJ_FROM_MENU_PEER(AwtMenuBar, self);

    if (obj)
        // dispose is implemented by AwtObject and delete this
        obj->Dispose();
}


//
// Menu
//
void 
sun_awt_windows_WMenuPeer_createMenu(HWMenuPeer* self,
                                     Hjava_awt_MenuBar * menuBar,
                                     Hjava_lang_String * menuName,
                                     /*boolean*/ long state)
{
    CHECK_NULL(menuName, "null text");

    // get the c++ menu object out of the java object
    // If we are here a menu bar must exist and we are adding a popup menu
    HWMenuBarPeer* peer = (HWMenuBarPeer*)(unhand(menuBar)->peer);
    AwtMenu* menuBarObj = GET_OBJ_FROM_MENU_PEER(AwtMenu, peer);
    if (menuBarObj) {
        AwtMenu* menu = new AwtMenu(self);

        CHECK_ALLOCATION(menu);

        // call create which will switch to the main gui thread
        menu->CreateMenu(menuBarObj, menuName);
        menu->Enable(state);
    }

}


void 
sun_awt_windows_WMenuPeer_createSubMenu(HWMenuPeer* self,
                                        Hjava_awt_Menu * menu,
                                        Hjava_lang_String * menuName,
                                        /*boolean*/ long state)
{
    CHECK_NULL(menuName, "null text");

    // get the c++ menu object out of the java object
    // If we are here a menu must exist and we are adding a sub-menu
    HWMenuPeer* peer = (HWMenuPeer*)(unhand(menu)->peer);
    AwtMenu* menuObj = GET_OBJ_FROM_MENU_PEER(AwtMenu, peer);
    if (menuObj) {
        AwtMenu* subMenu = new AwtMenu(self);

        CHECK_ALLOCATION(menu);

        // call create which will switch to the main gui thread
        subMenu->CreateMenu(menuObj, menuName);
        subMenu->Enable(state);
    }

}


void 
sun_awt_windows_WMenuPeer_pEnable(HWMenuPeer* self,
                                    /*boolean*/ long state)
{
    CHECK_MENU_PEER(self);

    AwtMenu *obj = GET_OBJ_FROM_MENU_PEER(AwtMenu, self);

    if (obj)
        // enable or disable a menu
        obj->Enable(state);

}


void 
sun_awt_windows_WMenuPeer_pSetLabel(HWMenuPeer* self,
                                        Hjava_lang_String * item)
{
    CHECK_MENU_PEER(self);
    CHECK_NULL(item, "null text");

    AwtMenu *obj = GET_OBJ_FROM_MENU_PEER(AwtMenu, self);

    if (obj)
        // since a label was defined already we modify it
        obj->Modify(item);

}


void 
sun_awt_windows_WMenuPeer_dispose(HWMenuPeer* self)
{
    CHECK_MENU_PEER(self);
    AwtMenu *obj = GET_OBJ_FROM_MENU_PEER(AwtMenu, self);

    if (obj) {
        obj->Remove();

        // dispose is implemented by AwtObject
        obj->Dispose();
    }
}


//
// MenuItem
//
void 
sun_awt_windows_WMenuItemPeer_create(HWMenuItemPeer* self,
                                     Hjava_awt_Menu * ownerMenu,
                                     Hjava_lang_String * item,
                                      /*boolean*/ long state)
{
    CHECK_NULL(item, "null text");

    // get the c++ menu object out of the java object
    // A menu must exist to create a menu item
    HWMenuPeer* peer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* menu = GET_OBJ_FROM_MENU_PEER(AwtMenu, peer);
    
    if (menu) {
        // get the menu id
        UINT id = menu->AddMenu(item, self);

        menu->EnableItem(id, state);

        // store the id in the peer.
        // A menu item has no c++ object associated 
        SET_OBJ_IN_MENU_PEER(self, id);
    }
}


void 
sun_awt_windows_WMenuItemPeer_pEnable(HWMenuItemPeer* self,
                                      Hjava_awt_Menu * ownerMenu,
                                      /*boolean*/ long state)
{
    CHECK_MENU_PEER(self);

    // get the c++ menu object out of the java object
    HWMenuPeer* peer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* menu = GET_OBJ_FROM_MENU_PEER(AwtMenu, peer);
    if (menu) 
        // enable or disable a menu item
        menu->EnableItem(unhand(self)->pNativeMenu, state);

}


void 
sun_awt_windows_WMenuItemPeer_pSetLabel(HWMenuItemPeer* self,
                                        Hjava_awt_Menu * ownerMenu,
                                        Hjava_lang_String * item)
{
    CHECK_MENU_PEER(self);
    CHECK_NULL(item, "null text");

    // get the c++ menu object out of the java object
    HWMenuPeer* peer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* menu = GET_OBJ_FROM_MENU_PEER(AwtMenu, peer);
    if (menu) 
        // since a label was defined already we modify it
        menu->ModifyItem(unhand(self)->pNativeMenu, item);

}


void 
sun_awt_windows_WMenuItemPeer_pDispose(HWMenuItemPeer* self,
                                        Hjava_awt_Menu * ownerMenu)
{
    //CHECK_MENU_PEER(self);
    
    // no c++ object is defined for a menu item, so we just remove it
    HWMenuPeer* ownerPeer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* ownerObj = GET_OBJ_FROM_MENU_PEER(AwtMenu, ownerPeer);

    if (ownerObj)
        ownerObj->RemoveItem(unhand(self)->pNativeMenu);
}


//
// CheckboxMenuItem
//
void 
sun_awt_windows_WCheckboxMenuItemPeer_create(HWCheckboxMenuItemPeer* self,
                                     Hjava_awt_Menu * ownerMenu,
                                     Hjava_lang_String * item,
                                     /*boolean*/ long enable,
                                     /*boolean*/ long state)
{
    CHECK_NULL(item, "null text");

    // get the c++ menu object out of the java object
    // A menu must exist to create a menu item
    HWMenuPeer* peer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* menu = GET_OBJ_FROM_MENU_PEER(AwtMenu, peer);
    
    if (menu) {
        // get the menu id
        UINT id = menu->AddMenu(item, (HWMenuItemPeer*)self);
        menu->SetItemState(id, state);
        menu->EnableItem(id, enable);

        // store the id in the peer.
        // A menu item has no c++ object associated 
        SET_OBJ_IN_MENU_PEER(self, id);
    }
}


long 
sun_awt_windows_WCheckboxMenuItemPeer_getState(HWCheckboxMenuItemPeer* self,
                                                Hjava_awt_Menu * ownerMenu)
{
    CHECK_MENU_PEER_AND_RETURN(self, 0);
    HWMenuPeer* ownerPeer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* ownerObj = GET_OBJ_FROM_MENU_PEER(AwtMenu, ownerPeer);

    if (ownerObj)
        return ownerObj->GetState(unhand(self)->pNativeMenu);

    return 0;
}


void 
sun_awt_windows_WCheckboxMenuItemPeer_pSetState(HWCheckboxMenuItemPeer* self,
                                                Hjava_awt_Menu * ownerMenu,
                                               /*boolean*/ long state)
{
    CHECK_MENU_PEER(self);
    HWMenuPeer* ownerPeer = (HWMenuPeer*)(unhand(ownerMenu)->peer);
    AwtMenu* ownerObj = GET_OBJ_FROM_MENU_PEER(AwtMenu, ownerPeer);

    if (ownerObj)
        // check or uncheck a menu item
        ownerObj->SetItemState(unhand(self)->pNativeMenu, state);
}

}
