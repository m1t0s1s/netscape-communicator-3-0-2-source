extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_MenuBar.h"
#include "java_awt_MenuItem.h"
#include "java_awt_Menu.h"
#include "java_awt_CheckboxMenuItem.h"
#include "java_awt_Frame.h"
#include "java_util_Vector.h"
#include "sun_awt_macos_MMenuBarPeer.h"
#include "sun_awt_macos_MFramePeer.h"
};

#include <UTextTraits.h>
#include <Menus.h>
#include "MMenuBarPeer.h"
#include "MFramePeer.h"
#include "MToolkit.h"

#ifdef UNICODE_FONTLIST
#include "csid.h"
enum {
	kScriptCodeInMenu = 28	// $1C in keyboard equivalent field mean the text is in the script
							// code specified in the icon field.
};
#include "LJavaFontList.h"
#include "LMacCompStr.h"
#endif

enum {
	kMenuBarLeftMargin				= 	8,
	kMenuBarRightMargin 			=	8,
	kMenuBarMenuSeparation			= 	8,
	kMenuTitleSelectionExtraWidth	= 	4,
	kMenuOffsetFromBottom		 	=	4,
	kFirstMenuIDForJavaMenuBar		= 	26,
	kTotalMenusForJavaMenuBar		= 	16,
	kMenuBarTextTraitsID			= 	531
};

MMenuBarPeer::
MMenuBarPeer(struct SPaneInfo &newScrollbarInfo):LPane(newScrollbarInfo)
{
	mMenuList = new LList(sizeof(MenuListItem));
	mHelpMenuList = new LList(sizeof(MenuListItem));
}

MMenuBarPeer::
~MMenuBarPeer()
{
	delete mMenuList;
	delete mHelpMenuList;
}		

Boolean
MMenuBarPeer::FocusDraw()
{
	Boolean	shouldRefocus = ShouldRefocusAWTPane(this);
	Boolean  result = false;
	gInPaneFocusDraw = true;
	result = inherited::FocusDraw();
	gInPaneFocusDraw = false;
	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);
	return result;
}

SInt16				gNextMenuIDToUse = 0;
ClassClass			*gMenuItemClass = NULL;
ClassClass			*gMenuClass = NULL;
ClassClass			*gCheckboxMenuItemClass = NULL;

MenuHandle
MMenuBarPeer::BuildMenuHandle(Hjava_awt_Menu *javaMenuObject)
{
	Classjava_awt_Menu		*javaMenu = unhand(javaMenuObject);
	Classjava_util_Vector	*menuItemVector = unhand(javaMenu->items);
	MenuHandle				menuHandle;
	Str255					javaMenuString;

	//	If we have used up all the menus that we can, then bail.

	if (gNextMenuIDToUse == kFirstMenuIDForJavaMenuBar + kTotalMenusForJavaMenuBar)
		return NULL;
	
	gNextMenuIDToUse++;
	
	//	Allocate a new mac menu for the java menu.
	
	menuHandle = NewMenu(gNextMenuIDToUse, "\px");
	if (menuHandle == NULL)
		return NULL;
		
	//	Make sure that we have loaded all of the class objects for 
	//	all of the menu item types.  We need to compare against these
	//	later.
		
	if (gMenuItemClass == NULL)
		gMenuItemClass = FindClass(0, "java/awt/MenuItem", (bool_t)0);

	if (gCheckboxMenuItemClass == NULL)
		gCheckboxMenuItemClass = FindClass(0, "java/awt/CheckboxMenuItem", (bool_t)0);
		
	if (gMenuClass == NULL)
		gMenuClass = FindClass(0, "java/awt/Menu", (bool_t)0);

	//	Iterate through all of the items in the java menu,
	//	adding them to the mac menu.
	
	UInt32					totalItemCount = menuItemVector->elementCount;
	Hjava_lang_Object 		**arrayBase = *((Hjava_lang_Object ***)menuItemVector->elementData);
	UInt32					nextItemToAdd = 1;
	
	for (UInt32 currentItemNum = 0; currentItemNum < totalItemCount; currentItemNum++) {
		
		Hjava_lang_Object 		*currentMenuItem;
		Boolean					isMenuItem, isMenu, isCheckboxMenuItem;
		Str255					menuItemText;
		
		currentMenuItem = arrayBase[currentItemNum];
		
		isCheckboxMenuItem = is_instance_of((JHandle *)currentMenuItem, gCheckboxMenuItemClass, EE());
		if (isCheckboxMenuItem) {

			Classjava_awt_CheckboxMenuItem	*menuItem = unhand((Hjava_awt_CheckboxMenuItem *)currentMenuItem);
			
#ifdef UNICODE_FONTLIST
			short encoding = intl_javaString2CEncodingString(menuItem->label, 
										(unsigned char *)(menuItemText + 1), 255);
#else
			javaString2CString(menuItem->label, (char *)(menuItemText + 1), 255);
#endif	
			menuItemText[0] = strlen((char *)(menuItemText + 1));
			
			AppendMenu(menuHandle, "\px");
			SetMenuItemText(menuHandle, nextItemToAdd, menuItemText);
			if (menuItem->enabled)
				EnableItem(menuHandle, nextItemToAdd);
			else
				DisableItem(menuHandle, nextItemToAdd);
				
			if (menuItem->state)
				CheckItem(menuHandle, nextItemToAdd, true);

#ifdef UNICODE_FONTLIST
			if(! UEncoding::IsMacRoman(encoding))
			{
				SetItemCmd(menuHandle, nextItemToAdd, kScriptCodeInMenu);
				SetItemIcon(menuHandle, nextItemToAdd, UEncoding::GetScriptID(encoding));				
			}
#endif			
			nextItemToAdd++;

		}
		
		else {

			isMenu = is_instance_of((JHandle *)currentMenuItem, gMenuClass, EE());
			if (isMenu) {
			
				Classjava_awt_Menu	*menuItem = unhand((Hjava_awt_Menu *)currentMenuItem);
#ifdef UNICODE_FONTLIST
				intl_javaString2CString(CS_MAC_ROMAN, menuItem->label, 
										(unsigned char *)(menuItemText + 1), 255);
#else
				javaString2CString(menuItem->label, (char *)(menuItemText + 1), 255);
#endif	
				menuItemText[0] = strlen((char *)(menuItemText + 1));
				
				AppendMenu(menuHandle, "\px");
				SetMenuItemText(menuHandle, nextItemToAdd, menuItemText);
				if (menuItem->enabled)
					EnableItem(menuHandle, nextItemToAdd);
				else
					DisableItem(menuHandle, nextItemToAdd);

				//	Call ourselves recusively to build the sub menu.

				MenuHandle		subMenu = BuildMenuHandle((Hjava_awt_Menu *)currentMenuItem);
			
				//	Set the menu item to be a hierarchical.
				
				SetItemCmd(menuHandle, nextItemToAdd, hMenuCmd);
				SetItemMark(menuHandle, nextItemToAdd, (**subMenu).menuID);
			
				nextItemToAdd++;
			
			}
			
			else {

				isMenuItem = is_instance_of((JHandle *)currentMenuItem, gMenuItemClass, EE());
				if (isMenuItem) {
				
					Classjava_awt_MenuItem	*menuItem = unhand((Hjava_awt_MenuItem *)currentMenuItem);
#ifdef UNICODE_FONTLIST
					short encoding = intl_javaString2CEncodingString(menuItem->label, 
										(unsigned char *)(menuItemText + 1), 255);
#else
					javaString2CString(menuItem->label, (char *)(menuItemText + 1), 255);
#endif	
					menuItemText[0] = strlen((char *)(menuItemText + 1));
					
					AppendMenu(menuHandle, "\px");
					SetMenuItemText(menuHandle, nextItemToAdd, menuItemText);
					if (menuItem->enabled)
						EnableItem(menuHandle, nextItemToAdd);
					else
						DisableItem(menuHandle, nextItemToAdd);
#ifdef UNICODE_FONTLIST
					if(! UEncoding::IsMacRoman(encoding))
					{
						SetItemCmd(menuHandle, nextItemToAdd, kScriptCodeInMenu);
						SetItemIcon(menuHandle, nextItemToAdd, UEncoding::GetScriptID(encoding));				
					}
#endif			
					
					nextItemToAdd++;
				}
		
			}

		
		}
		
		
	}
	
	//	Insert the new menu into the hierarchical portion of the 
	//	menu bar so that it will actually pop up.  Also add it 
	//	to our list of active menus so that we cleanup correctly 
	//	later.
	
	if (menuHandle != NULL) {
	
		ActiveMenuRecord		newItem;
	
		InsertMenu(menuHandle, -1);
		
		newItem.macMenu = menuHandle;
		newItem.javaMenu = javaMenuObject;
		
		mActiveMenuList->InsertItemsAt(1, 1, &newItem);

	}

	return menuHandle;
	
}

void
MMenuBarPeer::RebuildMenuListGeometry()
{
	Classjava_awt_MenuBar	*menuBarObject = unhand(mMenuBarObjectHandle);
	Classjava_util_Vector	*menuVector = unhand(menuBarObject->menus);
	UInt32					totalMenuCount,
							currentMenuNum,
							currentOffset = kMenuBarLeftMargin;
	
	//	Rebuild our menu list.
	
	mMenuList->RemoveItemsAt(mMenuList->GetCount(), 1);
	
	//	Figure out the displacements for the items in the
	//	menu title.
	
	totalMenuCount = menuVector->elementCount;
	
	Hjava_awt_Menu 		**arrayBase = *((Hjava_awt_Menu ***)menuVector->elementData);
	
	for (currentMenuNum = 1; currentMenuNum <= totalMenuCount; currentMenuNum++) {
		
		Classjava_awt_Menu	*currentMenu;
		MenuListItem		currentItem;
		Str255				menuTitleText;
		UInt32				currentTitleWidth;
	
		memset(&currentItem, 0, sizeof(MenuListItem));
	
		//	Get the java object for the current item in the
		//	menu list.
	
		currentItem.menu = arrayBase[currentMenuNum - 1];
		currentMenu = unhand(currentItem.menu);
	
		//	É and its text
		
#ifdef UNICODE_FONTLIST
		// Get the fontlist
		LJavaFontList fontlist;
		fontlist.SetFontSize(9);
		LMacCompStr string(currentMenu->label);
		currentTitleWidth = string.Width(&fontlist);
#else
		javaString2CString(currentMenu->label, (char *)(menuTitleText + 1), 255);
		menuTitleText[0] = strlen((char *)(menuTitleText + 1));
		
		//	Measure the text and store away its sizing information.
		
		currentTitleWidth = ::StringWidth(menuTitleText);
#endif	
		
		if (currentItem.menu != menuBarObject->helpMenu) {
	
			currentItem.menuLeft = currentOffset;
			currentOffset += currentTitleWidth;
			currentItem.menuRight = currentOffset;
		
			currentOffset += kMenuBarMenuSeparation;

		} else {
			
			Rect	localFramRect;
			CalcLocalFrameRect(localFramRect);
			currentItem.menuRight = localFramRect.right - kMenuBarRightMargin;
			currentItem.menuLeft = currentItem.menuRight - currentTitleWidth;
		
		}
		
		mMenuList->InsertItemsAt(1, arrayIndex_Last, &currentItem);
		
	}
		
}

void
MMenuBarPeer::DrawSelf()
{
	Rect		frameRect;

	CalcLocalFrameRect(frameRect);
	
	ForeColor(whiteColor);
	frameRect.bottom--;
	PaintRect(&frameRect);

	//	Draw the separator line.
	
	ForeColor(blackColor);
	MoveTo(frameRect.left, frameRect.bottom);
	LineTo(frameRect.right - 1, frameRect.bottom);

	//	Make sure that all of our positioning information
	//	is correct.
	
	RebuildMenuListGeometry();
	
	//	Draw the menu titles.

	UInt32	totalMenuCount,
			currentMenuNum;
				
	mMenuList->Lock();

	totalMenuCount = mMenuList->GetCount();

	for (currentMenuNum = 1; currentMenuNum <= totalMenuCount; currentMenuNum++) {

		Classjava_awt_Menu	*currentMenu;
		MenuListItem		*currentItemPtr;
		Str255				menuTitleText;
	
		//	Get the java object for the current item in the
		//	menu list.
	
		currentItemPtr = (MenuListItem *)(mMenuList->GetItemPtr(currentMenuNum));
		currentMenu = unhand(currentItemPtr->menu);
		
		//	É and its text
		
	// 	i18n fix!!!
#ifdef UNICODE_FONTLIST
		// Get the fontlist
		LJavaFontList fontlist;
		fontlist.SetFontSize(9);
		LMacCompStr string(currentMenu->label);
		TextFace(bold);
		ForeColor(blackColor);
		BackColor(whiteColor);
		string.Draw(&fontlist, currentItemPtr->menuLeft, kMenuBarHeight - kMenuOffsetFromBottom);
#else
		javaString2CString(currentMenu->label, (char *)(menuTitleText + 1), 255);
		menuTitleText[0] = strlen((char *)(menuTitleText + 1));
		MoveTo(currentItemPtr->menuLeft, kMenuBarHeight - kMenuOffsetFromBottom);
		TextFace(bold);
		ForeColor(blackColor);
		BackColor(whiteColor);
		DrawString(menuTitleText);
#endif	


	}

	mMenuList->Unlock();
	
	
}

void
MMenuBarPeer::RemoveAndDisposeMenus()
{
	UInt32		totalMenus,
				currentItemNum;
	
	totalMenus = mActiveMenuList->GetCount();
	
	for (currentItemNum = 1; currentItemNum <= totalMenus; currentItemNum++) {
	
		ActiveMenuRecord		currentItem;
		
		mActiveMenuList->FetchItemAt(currentItemNum, &currentItem);
		
		DeleteMenu((**(currentItem.macMenu)).menuID);
	
		DisposeMenu(currentItem.macMenu);
		
	}
	
	delete mActiveMenuList;
	
}

void
MMenuBarPeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	MenuListItem		*hitMenuPtr = NULL;

	//	Make sure that we have focus
	
	FocusDraw();
	
	//	Make sure that all of our positioning information
	//	is correct.
	
	RebuildMenuListGeometry();
	
	//	Check to see if the click was actually in a menu title.  
	
	UInt32	totalMenuCount,
			currentMenuNum;
				
	mMenuList->Lock();

	totalMenuCount = mMenuList->GetCount();

	for (currentMenuNum = 1; currentMenuNum <= totalMenuCount; currentMenuNum++) {

		Classjava_awt_Menu	*currentMenu;
		MenuListItem		*currentItemPtr;
		Str255				menuTitleText;
	
		//	Get the java object for the current item in the
		//	menu list.
	
		currentItemPtr = (MenuListItem *)(mMenuList->GetItemPtr(currentMenuNum));
		
		if ((currentItemPtr->menuRight >= inMouseDown.whereLocal.h) &&
			(currentItemPtr->menuLeft <= inMouseDown.whereLocal.h)) {
			
			hitMenuPtr = currentItemPtr;
			break;	
			
		}

	}
	
	//	If we did hit a menu title, then proceed with menu selection.
	
	if (hitMenuPtr != NULL) {

		Rect			menuTitleRect;
		MenuHandle		menuHandle;
		Point			popupPoint;
		
		CalcLocalFrameRect(menuTitleRect);
		menuTitleRect.left = hitMenuPtr->menuLeft - kMenuTitleSelectionExtraWidth;
		menuTitleRect.right = hitMenuPtr->menuRight + kMenuTitleSelectionExtraWidth;
	
		menuTitleRect.bottom--;
	
		::InvertRect(&menuTitleRect);
	
		gNextMenuIDToUse = kFirstMenuIDForJavaMenuBar;
	
		mActiveMenuList = new LList(sizeof(ActiveMenuRecord));
	
		menuHandle = BuildMenuHandle(hitMenuPtr->menu);
		
		::CalcMenuSize(menuHandle);
		
		popupPoint.v = menuTitleRect.bottom + 1;
		if (hitMenuPtr->menu != unhand(mMenuBarObjectHandle)->helpMenu)
			popupPoint.h = menuTitleRect.left + 1;
		else
			popupPoint.h = menuTitleRect.right - (**menuHandle).menuWidth - 1;
		
		LocalToPortPoint(popupPoint);
		PortToGlobalPoint(popupPoint);
		
		UInt32		selectionResult;
		
		selectionResult = PopUpMenuSelect(menuHandle, popupPoint.v, popupPoint.h, 0);

		//	If an item was actually selection, then dispatch it to the
		//	appropriate item.

		Hjava_awt_MenuItem		*selectedItem = NULL;

		if (selectionResult != NULL)
			selectedItem = DispatchMenuSelection(selectionResult);
			
		RemoveAndDisposeMenus();
		
		FocusDraw();
		::InvertRect(&menuTitleRect);
	
		mMenuList->Unlock();

		if (selectedItem != NULL)
			MToolkitExecutJavaDynamicMethod(unhand(selectedItem)->peer, "handleAction", "(I)V", 0, 0, 0, 0);

	}
	else {
	
		mMenuList->Unlock();
	}
}

Hjava_awt_MenuItem * 
MMenuBarPeer::DispatchMenuSelection(UInt32 selectionResult)
{
	SInt16					selectedMenuID;
	UInt16					selectedItemNum;
	ActiveMenuRecord		nextActiveItem;
	
	selectedMenuID = (selectionResult >> 16);
	selectedItemNum = (selectionResult & 0x0000FFFF);
	
	//	Search through the list of active menus until we find the 
	//	one with the id we are looking for.
	
	LListIterator	menuFinder(*mActiveMenuList, iterate_FromStart);
	
	while (menuFinder.Next(&nextActiveItem)) {

		// 	When we find a match, grab the java object for the menu
		//	item, and send it an action message.
	
		if ((**nextActiveItem.macMenu).menuID == selectedMenuID) {
		
			Hjava_awt_Menu			*selectedMenu = nextActiveItem.javaMenu;
			Hjava_util_Vector		*menuItemVector = unhand(selectedMenu)->items;
			Hjava_awt_MenuItem 		**menuItemArrayBase = *((Hjava_awt_MenuItem ***)(unhand(menuItemVector)->elementData));
			Hjava_awt_MenuItem		*selectedItem = menuItemArrayBase[selectedItemNum - 1];

			return selectedItem;

			break;
			
		}
	
	}
	
	return NULL;
	
}


void sun_awt_macos_MMenuBarPeer_create(struct Hsun_awt_macos_MMenuBarPeer *menuBarPeerHandle, struct Hjava_awt_Frame *containerFrameHandle)
{

	Classsun_awt_macos_MMenuBarPeer	*menuBarPeer = (Classsun_awt_macos_MMenuBarPeer*)unhand(menuBarPeerHandle);
	Classjava_awt_MenuBar			*menuBarTarget = (Classjava_awt_MenuBar*)unhand(menuBarPeer->target);
	Classjava_awt_Frame				*containerFrame = unhand(containerFrameHandle);
	Hsun_awt_macos_MFramePeer		*containerFramePeer = (Hsun_awt_macos_MFramePeer *)(containerFrame->peer);
	MFramePeer						*framePeer = (MFramePeer *)(unhand(containerFramePeer)->mOwnerPane);
	LWindow							*containerWindow = (LWindow *)(framePeer->GetSuperView());
	struct SPaneInfo				newMenuBarInfo;
	SDimension16					superViewSize;
	Str255							menuBarText;
	
	//	Shorten the framePeer in inside the container window
	//	and move it down to make room for the menu bar.
	
	framePeer->GetFrameSize(superViewSize);
	framePeer->ResizeFrameTo(superViewSize.width, superViewSize.height - MMenuBarPeer::kMenuBarHeight, false);
	framePeer->PlaceInSuperFrameAt(0, MMenuBarPeer::kMenuBarHeight, false);
	
	//	Resize the container window to accomidate the menu
	//	bar, too.
	
	containerWindow->GetFrameSize(superViewSize);
	containerWindow->ResizeImageTo(superViewSize.width, superViewSize.height + MMenuBarPeer::kMenuBarHeight, false);
	
	memset(&newMenuBarInfo, 0, sizeof(newMenuBarInfo));
	
	newMenuBarInfo.paneID 			= MMenuBarPeer::class_ID;
	newMenuBarInfo.visible 			= triState_Off;
	newMenuBarInfo.left				= 0;
	newMenuBarInfo.top				= 0;
	newMenuBarInfo.width			= superViewSize.width;
	newMenuBarInfo.height			= MMenuBarPeer::kMenuBarHeight;
	newMenuBarInfo.enabled 			= triState_Off;
	newMenuBarInfo.userCon 			= (UInt32)menuBarPeerHandle;
	newMenuBarInfo.superView		= containerWindow;
	newMenuBarInfo.bindings.right 	= true;
	newMenuBarInfo.bindings.left 	= true;
	
	MMenuBarPeer *menuBarPane = NULL;
	
	try {
		menuBarPane = new MMenuBarPeer(newMenuBarInfo);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	menuBarPane->mMenuBarObjectHandle = unhand(menuBarPeerHandle)->target;
	menuBarPane->mContainerFrameHandle = containerFrameHandle;

	menuBarPeer->pData = (long)menuBarPane;

	//	We create the menu bar invisible and disabled; we have
	//	to explicitly show and enable it.

	menuBarPane->Show();
	menuBarPane->Enable();

	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MMenuBarPeer_dispose(struct Hsun_awt_macos_MMenuBarPeer *menuBarPeerHandle)
{

	Classsun_awt_macos_MMenuBarPeer		*menuBarPeer = (Classsun_awt_macos_MMenuBarPeer*)unhand(menuBarPeerHandle);
	MMenuBarPeer						*menuBarPane = (MMenuBarPeer *)(menuBarPeer->pData);

	if (menuBarPane == NULL)
		return;

	Classjava_awt_Frame					*containerFrame = unhand(menuBarPane->mContainerFrameHandle);
	Hsun_awt_macos_MFramePeer			*containerFramePeer = (Hsun_awt_macos_MFramePeer *)(containerFrame->peer);
	MFramePeer							*framePeer = (MFramePeer *)(unhand(containerFramePeer)->mOwnerPane);
	SDimension16						superViewSize;

	//	Lengthen the framePeer in inside the container window
	//	and move it up.
	
	framePeer->GetFrameSize(superViewSize);
	framePeer->ResizeFrameTo(superViewSize.width, superViewSize.height + MMenuBarPeer::kMenuBarHeight, false);
	framePeer->PlaceInSuperFrameAt(0, 0, false);

	delete menuBarPane;

}

void sun_awt_macos_MMenuBarPeer_addMenu(struct Hsun_awt_macos_MMenuBarPeer *menuBarPeerHandle, struct Hjava_awt_Menu *newMenu)
{
	Classsun_awt_macos_MMenuBarPeer		*menuBarPeer = (Classsun_awt_macos_MMenuBarPeer*)unhand(menuBarPeerHandle);
	MMenuBarPeer						*menuBarPane = (MMenuBarPeer *)(menuBarPeer->pData);

	if (menuBarPane == NULL)
		return;

	Rect		refreshRect;
	menuBarPane->CalcLocalFrameRect(refreshRect);
	menuBarPane->FocusDraw();
	::InvalRect(&refreshRect);
}

void sun_awt_macos_MMenuBarPeer_delMenu(struct Hsun_awt_macos_MMenuBarPeer *menuBarPeerHandle, long whichMenu)
{
	Classsun_awt_macos_MMenuBarPeer		*menuBarPeer = (Classsun_awt_macos_MMenuBarPeer*)unhand(menuBarPeerHandle);
	MMenuBarPeer						*menuBarPane = (MMenuBarPeer *)(menuBarPeer->pData);

	if (menuBarPane == NULL)
		return;

	Rect		refreshRect;
	menuBarPane->CalcLocalFrameRect(refreshRect);
	menuBarPane->FocusDraw();
	::InvalRect(&refreshRect);
}

void sun_awt_macos_MMenuBarPeer_addHelpMenu(struct Hsun_awt_macos_MMenuBarPeer *menuBarPeerHandle, struct Hjava_awt_Menu *newMenu)
{
	Classsun_awt_macos_MMenuBarPeer		*menuBarPeer = (Classsun_awt_macos_MMenuBarPeer*)unhand(menuBarPeerHandle);
	MMenuBarPeer						*menuBarPane = (MMenuBarPeer *)(menuBarPeer->pData);

	if (menuBarPane == NULL)
		return;

	Rect		refreshRect;
	menuBarPane->CalcLocalFrameRect(refreshRect);
	menuBarPane->FocusDraw();
	::InvalRect(&refreshRect);
}
