#ifndef AWT_MENU_H
#define AWT_MENU_H

#include "awt_defs.h"
#include "awt_object.h"
#include "awt_resource.h"
#include "awt_event.h"
#include "awt_wcompstr.h"


#define MENU_ITEMS          10

class AwtFrame;

#define MENU_PTR(_qp) \
    ((AwtMenu*) ((char*) (_qp) - offsetof(AwtMenu,m_link)))

//
// Declare the Java peer class
//
DECLARE_SUN_AWT_WINDOWS_PEER(WMenuBarPeer);
DECLARE_SUN_AWT_WINDOWS_PEER(WMenuPeer);
DECLARE_SUN_AWT_WINDOWS_PEER(WMenuItemPeer);
DECLARE_SUN_AWT_WINDOWS_PEER(WCheckboxMenuItemPeer);

class AwtMenu : public AwtObject
{

public:

    //
    // Enumeration of the methods which are accessable on the "main GUI thread"
    // via the CallMethod(...) mechanism...
    //
    enum {
        CREATEMENU       = 0x0101,
        CREATEPOPUP
    };

public:
                            AwtMenu(JHandle* peer);
                            AwtMenu(HWMenuPeer* peer);

    virtual                 ~AwtMenu();

    virtual BOOL            CallMethod(AwtMethodInfo *info);

    virtual void            CreateMenu(HWND hFrameWnd) {}
    virtual void            CreateMenu(AwtMenu* owner, Hjava_lang_String* menuName);

    virtual UINT            AddMenu(Hjava_lang_String* menuRefName, HWMenuItemPeer* peer, AwtMenu* menu = 0);

            void            Enable(long state);
            void            EnableItem(UINT position, long state, UINT cFlag = 0);
            void            SetItemState(UINT position, long state);

            void            Modify(Hjava_lang_String* itemName);
            void            ModifyItem(UINT position, Hjava_lang_String* itemName);
            void            Remove();
            void            RemoveItem(UINT position, UINT cFlag = 0);

            long            GetState(UINT position);

    operator HMENU          () { return m_hMenu; }

    virtual void            UpdateMenu();

    virtual UINT            AddPeer(HWMenuItemPeer* itemPeer);
    virtual void            RemovePeer(UINT position);

            AwtCList&       GetListEl() {return m_link;}
            void            UpdatePositions(UINT pos);

            UINT            GetPosition() { return m_cIDs;}
            void            SetPosition(UINT pos) {m_cIDs = pos;}

protected:

    // the list element
    AwtCList                m_link;

    // keep the submenus in a list. This is a sort of list head
    AwtCList                m_OwnerLink;

    HMENU                   m_hMenu;
    AwtMenu*                m_Owner;

    UINT                    m_cIDs;
    UINT                    m_cPositions;

#ifdef INTLUNICODE
    WinMenuCompStrList      m_stringList;
#endif

};


class AwtMenuBar : public AwtMenu {

public:
                            AwtMenuBar(HWMenuBarPeer* peer);

    virtual                 ~AwtMenuBar();

    virtual void            CreateMenu(HWND hFrameWnd);

    virtual UINT            AddMenu(Hjava_lang_String* menuRefName, HWMenuItemPeer* peer, AwtMenu* menu = 0);

    virtual BOOL            OnSelect(UINT id);

    virtual void            UpdateMenu();

    virtual UINT            AddPeer(HWMenuItemPeer* itemPeer);
    virtual void            RemovePeer(UINT position);
protected:

    HWMenuItemPeer**        m_PeerCollection;
    size_t                  m_currentSize;

    HWND                    m_hWnd;

    AwtMenu*                m_HelpMenu; // help menu, so it will keep the last position

};

#endif


