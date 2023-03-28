//##############################################################################
//##############################################################################
//
//	File:		MChoicePeer.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################
#include "MToolkit.h"

#include <Types.h>
#include <Memory.h>
#include <LButton.h>
#include <LStdControl.h>
#include <LView.h>

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_Choice.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MChoicePeer.h"

};

#include "MChoicePeer.h"
#include "MCanvasPeer.h"
#include "MComponentPeer.h"
#include "LMacGraphics.h"
#include <UTextTraits.h>

#ifdef UNICODE_FONTLIST
enum {
	kScriptCodeInMenu = 28	// $1C in keyboard equivalent field mean the text is in the script
							// code specified in the icon field.
};
#endif

enum {
	kTemplatePopupMenuResID				= -3496
};

//##############################################################################
//##############################################################################
#pragma mark MComponentPeer IMPLEMENTATION
#pragma mark -

Handle									gActualPopupMenu = NULL;
Boolean									gChoiceInitializing = false;

MChoicePeer								*gCurrentChoicePeer = NULL;

MChoicePeer::
MChoicePeer(struct SPaneInfo &newScrollbarInfo):LPane(newScrollbarInfo)
{
}

MChoicePeer::
~MChoicePeer()
{
	DisposeJavaDataForPane(this);
}

Int32
MChoicePeer::GetValue() const
{
	Classsun_awt_macos_MComponentPeer	*componentPeer = (Classsun_awt_macos_MComponentPeer *)unhand(PaneToPeer(this));
	Hjava_awt_Choice					*choiceHandle;

	choiceHandle = (Hjava_awt_Choice *)componentPeer->target;
	return unhand(choiceHandle)->selectedIndex;
}


Boolean 
MChoicePeer::FocusDraw()
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

void
MChoicePeer::CalculateMenuDimensions(short &width)
{
	MenuHandle	choiceMenu;
	
	choiceMenu = (MenuHandle)unhand((Hsun_awt_macos_MChoicePeer *)(PaneToPeer(this)))->mActualMenuContents;

	if ((**choiceMenu).menuWidth == -1) {

		//	Switch to the Window Manager port, and set the font and
		//	size to be appropriate for the popup.  Save everything 
		//	away so that we can restore ports Õn stuff properly.
		
		CGrafPtr	windowManagerPort;
		GrafPtr		savedPort;
		short		savedFont,
					savedSize,
					savedSysFont;
		
		GetPort(&savedPort);
		GetCWMgrPort(&windowManagerPort);
		SetPort((GrafPtr)windowManagerPort);
		
		savedFont = windowManagerPort->txFont;
		savedSize = windowManagerPort->txSize;

		UTextTraits::SetPortTextTraits(kDefaultAWTTextTraitsID);
		savedSysFont = LMGetSysFontFam();
		LMSetSysFontFam(9);

		CalcMenuSize(choiceMenu);

		//	Restore ports and Quickdraw info
		
		LMSetSysFontFam(savedSysFont);
		::TextFont(savedFont);
		::TextSize(savedSize);
		SetPort(savedPort);

	}
		
	width = (**choiceMenu).menuWidth;
}

void 
MChoicePeer::DrawSelf()
{
	Rect			frameRect,
					popupRectangle,
					tempRect;
	MenuHandle		choiceMenu;
	Str255			menuItemText;
	Boolean			enabled = IsEnabled();
	short			width;
	
	CalcLocalFrameRect(frameRect);

	UTextTraits::SetPortTextTraits(kDefaultAWTTextTraitsID);
	
	choiceMenu = (MenuHandle)unhand((Hsun_awt_macos_MChoicePeer *)(PaneToPeer(this)))->mActualMenuContents;

	CalculateMenuDimensions(width);
	
	//	The choice peerÕs rectangle is a sub-rectangle
	//	of the frame rectangle.  It is kChoicePeerDefaultHeight
	//	pixels high, and menuWidth pixels wide.

	popupRectangle = frameRect;	
	
	if (width > (popupRectangle.right - popupRectangle.left - kChoicePeerDefaultExtraWidth))
		width = popupRectangle.right - popupRectangle.left - kChoicePeerDefaultExtraWidth;
		
	if (width < 0)
		width = 0;
	
	popupRectangle.bottom = popupRectangle.top + kChoicePeerDefaultHeight;
	popupRectangle.right = popupRectangle.left + width + kChoicePeerDefaultExtraWidth;
	
	//	Draw the choice peerÕs frame and its drop shadow.  If the peer is 
	//	disabled, then draw in grey.
	
	if (enabled)
		::ForeColor(blackColor);
	
	else {

		//	FIX ME
	
//		RGBColor	disabledGrey = { 140 << 8, 140 << 8, 140 << 8 };
//		::RGBForeColor();

	}
	
	::BackColor(whiteColor);
	
	FrameRect(&popupRectangle);
	
	tempRect = popupRectangle;
	InsetRect(&tempRect, 1, 1);
	EraseRect(&tempRect);
		
	MoveTo(popupRectangle.left + 3, popupRectangle.bottom);
	LineTo(popupRectangle.right, popupRectangle.bottom);
	LineTo(popupRectangle.right, popupRectangle.top + 3);
	
	//	Draw the text of the current item.
	
	::MoveTo(popupRectangle.left + kChoicePeerItemTextIndent, popupRectangle.bottom - kChoicePeerItemTextIndentUp);
	
	GetMenuItemText(choiceMenu, GetValue() + 1, menuItemText); 
#ifdef UNICODE_FONTLIST
	// Ok, the text is in other script. We have to swtich the font before we draw it.
	short cmd;
	::GetItemCmd(choiceMenu, GetValue() + 1, &cmd);
	if(cmd == kScriptCodeInMenu)
	{
		short script;
		::GetItemIcon(choiceMenu, GetValue() + 1, &script);
		::TextFont(::GetScriptVariable(script, smScriptSysFondSize) >> 16);
	}
#endif	
	DrawString(menuItemText);
	//	Draw the arrow.
	
	for (UInt32 currentLine = 0; currentLine < 5; currentLine++) {
		::MoveTo(popupRectangle.right - kChoicePeerArrowRightIndent + currentLine, 
			popupRectangle.top + kChoicePeerArrowDownIndent + currentLine);
		::Line(8 - (currentLine * 2), 0);
	}
	
}

void
MChoicePeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	MenuHandle		choiceMenu;
	Rect			frameRect,
					popupRectangle;
	Point			selectPoint;
	long			newValue,
					previousValue = GetValue();
	short			width;

	CalculateMenuDimensions(width);

	//	In order to pervent flicker, erase the current contents of
	//	the popup, and redraw when we are done.

	CalcLocalFrameRect(frameRect);
	
	popupRectangle = frameRect;	
	
	popupRectangle.bottom = popupRectangle.top + kChoicePeerDefaultHeight;
	popupRectangle.right = popupRectangle.left + width + kChoicePeerDefaultExtraWidth;

	FocusDraw();

	choiceMenu = (MenuHandle)unhand((Hsun_awt_macos_MChoicePeer *)(PaneToPeer(this)))->mActualMenuContents;
	
	selectPoint.h = frameRect.left;
	selectPoint.v = frameRect.top;
	
	LocalToPortPoint(selectPoint);
	PortToGlobalPoint(selectPoint);
	
	//	Temporarily insert this menu into the hierarchical portion
	//	of the menu bar and call PopupMenuSelect to let the
	//	user make the choice.

	InsertMenu(choiceMenu, -1);

	//	Save off our port and port information
	{
	
		CGrafPtr								wMgrCPort;
		GetCWMgrPort(&wMgrCPort);
		StPortOriginState 						savedPortInfo((GrafPtr)wMgrCPort);
		StTextState								savedFontInfo;
		SInt16									savedFont;
		
		UTextTraits::SetPortTextTraits(kDefaultAWTTextTraitsID);
		savedFont = LMGetSysFontFam();
		LMSetSysFontFam(9);
		
		//	Mark the currently selected item in the choice peer
		//	with the '¥' character.

		SetItemMark(choiceMenu, previousValue + 1, '¥');

		newValue = PopUpMenuSelect(choiceMenu, selectPoint.v + 1, selectPoint.h + 1, previousValue + 1);
		
		LMSetSysFontFam(savedFont);

	}
	
	//	Remove the checkmark set above.
	
	SetItemMark(choiceMenu, previousValue + 1, 0x00);

	//	Remove the menu from the hierarchical portion of the menu
	//	bar until the next time we need it.
	
	DeleteMenu((**choiceMenu).menuID);

	LView::OutOfFocus(NULL);

	//	If the selection from the popup was non zero, then call the
	//	peerÕs action procedure (remember, selection in Choice.class
	//	are zero based, Mac Menu Manager selections are 1 based), 
	//	and redraw the popup with the newly selected value.

	if (newValue != 0) {
		newValue = (newValue & 0xFFFF) - 1;
		MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleAction", "(I)V", newValue);
	}
	
}

//##############################################################################
//##############################################################################


void sun_awt_macos_MChoicePeer_create(struct Hsun_awt_macos_MChoicePeer *choicePeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MChoicePeer	*choicePeer = (Classsun_awt_macos_MChoicePeer*)unhand(choicePeerHandle);
	Classjava_awt_Choice			*choiceTarget = (Classjava_awt_Choice*)unhand(choicePeer->target);
	LView							*superView = (LView *)PeerToPane(hostPeerHandle);
	struct SPaneInfo				newChoiceInfo;
	
	memset(&newChoiceInfo, 0, sizeof(newChoiceInfo));
	
	newChoiceInfo.paneID 		= MChoicePeer::class_ID;
	newChoiceInfo.visible 		= triState_Off;
	newChoiceInfo.left			= choiceTarget->x;
	newChoiceInfo.top			= choiceTarget->y;
	newChoiceInfo.width			= choiceTarget->width;
	newChoiceInfo.height		= choiceTarget->height;
	newChoiceInfo.left			= choiceTarget->x;
	newChoiceInfo.top			= choiceTarget->y;
	newChoiceInfo.width			= 90;
	newChoiceInfo.enabled 		= triState_Off;
	newChoiceInfo.userCon 		= (UInt32)choicePeerHandle;
	newChoiceInfo.superView		= superView;
	
	gChoiceInitializing = true;
	
	MChoicePeer *choicePane = NULL;
	
	try {
		choicePane = new MChoicePeer(newChoiceInfo);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	choicePane->Enable();
	choicePane->Show();
	
	gChoiceInitializing = false;

	//	Each choice peer needs to maintian its own private copy
	//	of a MenuHandle to keep its items close at hand.  We
	//	create this MenuHandle from a tempalte MENU resource
	//	with ID kTemplatePopupMenuResID.  We detach it so that
	//	other clients get their own, unique copy.
	
	MenuHandle privateCopy = GetMenu(kTemplatePopupMenuResID);
	DetachResource((Handle)privateCopy);
	
	choicePeer->mActualMenuContents = (long)privateCopy;
	choicePeer->mIsChoice = true;

	choicePeer->mOwnerPane = (long)choicePane;
	choicePeer->mRecalculateClip = true;

	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MChoicePeer_addItem(struct Hsun_awt_macos_MChoicePeer *componentPeerHandle, struct Hjava_lang_String *textHandle, long itemNumber)
{
	Classsun_awt_macos_MChoicePeer	*componentPeer = unhand(componentPeerHandle);
	MChoicePeer 					*componentPane = (MChoicePeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)componentPeerHandle);
	Str255							itemText;
	
	if (componentPane == NULL)
		return;
	
	// 	i18n fix!!!
#ifdef UNICODE_FONTLIST
	short encoding = intl_javaString2CEncodingString(textHandle, 
						(unsigned char *)(itemText + 1), 255);
#else
	javaString2CString(textHandle, (char *)(itemText + 1), 255);
#endif
	itemText[0] = strlen((char *)(itemText + 1));

	//	Convert the java string into a C string.  Maximum length is 255 - 5
	//	because of Menu Manager bug which overflows over 255.

	if (itemText[0] > 250)
		itemText[0] = 250;
	
	MenuHandle		choiceMenu = (MenuHandle)(componentPeer->mActualMenuContents);
	
	//	Substitute a longer dash for a normal dash if it is the the first item
	//	in the list.  This makes sure that the item is more that just a separator.
	
	if (itemText[1] == '-')
		itemText[1] = 'Ñ';
		
	InsertMenuItem(choiceMenu, "\px", itemNumber);
	SetMenuItemText(choiceMenu, itemNumber + 1,itemText);
#ifdef UNICODE_FONTLIST
	if(! UEncoding::IsMacRoman(encoding))
	{
		SetItemCmd(choiceMenu, itemNumber + 1, kScriptCodeInMenu);
		SetItemIcon(choiceMenu, itemNumber + 1, UEncoding::GetScriptID(encoding));				
	}
#endif
}

void sun_awt_macos_MChoicePeer_remove(struct Hsun_awt_macos_MChoicePeer *componentPeerHandle, long whichItem)
{
	Classsun_awt_macos_MChoicePeer	*componentPeer = unhand(componentPeerHandle);
	MChoicePeer 					*componentPane = (MChoicePeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)componentPeerHandle);
	MenuHandle						choiceMenu = (MenuHandle)(componentPeer->mActualMenuContents);

	if (componentPane == NULL)
		return;
	
	DeleteMenuItem(choiceMenu, whichItem);

}

void sun_awt_macos_MChoicePeer_select(struct Hsun_awt_macos_MChoicePeer *componentPeerHandle, long whichItem)
{
	Classsun_awt_macos_MComponentPeer	*componentPeer = unhand((Hsun_awt_macos_MComponentPeer *)componentPeerHandle);
	MChoicePeer 						*componentPane = (MChoicePeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)componentPeerHandle);
	Hjava_awt_Choice					*choiceHandle;
	Rect								tempRect;

	if (componentPane == NULL)
		return;
	
	choiceHandle = (Hjava_awt_Choice *)componentPeer->target;
	unhand(choiceHandle)->selectedIndex = whichItem;
	
	//	Invalidate the text area, minus the scrollbars
	//	so that the text is re-drawn.
	//	dkc 2/7/96

	LView::OutOfFocus(NULL);		
	componentPane->FocusDraw();
	componentPane->CalcLocalFrameRect(tempRect);
	::InvalRect(&tempRect);	
}
