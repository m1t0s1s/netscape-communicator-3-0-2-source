//##############################################################################
//##############################################################################
//
//	File:		MListPeer.cp
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
#include "java_awt_List.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MListPeer.h"

};

#ifdef UNICODE_FONTLIST
#include "LMacCompStr.h"
#endif

#include "MListPeer.h"
#include "MCanvasPeer.h"
#include "MToolkit.h"
#include <Controls.h>
#include <LowMem.h>

enum {
	kListPeerCellHeight			= 16,
	kListPeerScrollBarWidth		= 16,
	kListItemIndent				= 5
};

//##############################################################################
//##############################################################################
#pragma mark MComponentPeer IMPLEMENTATION
#pragma mark -


MListPeer::
MListPeer(struct SPaneInfo &newScrollbarInfo):LPane(newScrollbarInfo)
{
	WindowPtr		controlWindow;
	Rect			boundsRect;

#ifdef UNICODE_FONTLIST
	mItemList = new LList(sizeof(LMacCompStr *));
#else
	mItemList = new LList(sizeof(char *));
#endif

	mSelectionList = new LList(sizeof(UInt32));
	mScrollPosition = 1;
	mScrollControl = NULL;
	
	mCanMultiplySelect = false;
	
	controlWindow = (WindowPtr)((newScrollbarInfo.superView)->GetMacPort());
	
	CalcLocalFrameRect(boundsRect);
	boundsRect.left = boundsRect.right - kListPeerScrollBarWidth;
	
	mScrollControl = ::NewControl(controlWindow, &boundsRect, "\p", false, 0, 0, 0, scrollBarProc, (long)this);
}

MListPeer::
~MListPeer()
{
	//	Throw away all of the items in the list.
	
	LListIterator	itemIterator(*mItemList, iterate_FromStart);
#ifdef UNICODE_FONTLIST
	LMacCompStr		*nextString;
	while (itemIterator.Next(&nextString))
		delete nextString;
#else
	char			*nextString;
	while (itemIterator.Next(&nextString))
		free(nextString);
#endif

	delete mItemList;
	delete mSelectionList;
	
	::DisposeControl(mScrollControl);

	DisposeJavaDataForPane(this);
}

Boolean 
MListPeer::FocusDraw()
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
MListPeer::PlaceInSuperImageAt(Int32 inHorizOffset, Int32 inVertOffset, Boolean inRefresh)
{
	inherited::PlaceInSuperImageAt(inHorizOffset, inVertOffset, inRefresh);

	Rect	boundsRect;
	CalcLocalFrameRect(boundsRect);
	boundsRect.left = boundsRect.right - kListPeerScrollBarWidth;
	
	(**mScrollControl).contrlVis = 0;
	::MoveControl(mScrollControl, boundsRect.left, boundsRect.top);
	::SizeControl(mScrollControl, boundsRect.right - boundsRect.left, boundsRect.bottom - boundsRect.top);
	(**mScrollControl).contrlVis = 255;
}

void
MListPeer::ResizeFrameBy(Int16 inWidthDelta, Int16 inHeightDelta, Boolean inRefresh)
{
	inherited::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);

	Rect	boundsRect;
	CalcLocalFrameRect(boundsRect);
	boundsRect.left = boundsRect.right - kListPeerScrollBarWidth;
	
	(**mScrollControl).contrlVis = 0;
	::MoveControl(mScrollControl, boundsRect.left, boundsRect.top);
	::SizeControl(mScrollControl, boundsRect.right - boundsRect.left, boundsRect.bottom - boundsRect.top);
	(**mScrollControl).contrlVis = 255;

	AdjustScrollBar();
}

UInt32
MListPeer::GetItemsOnScreen()
{
	Rect		boundsRect;
	UInt32		visItems;
	
	CalcLocalFrameRect(boundsRect);
	
	visItems = ((boundsRect.bottom - boundsRect.top) / kListPeerCellHeight);
	
	if (visItems == 0)
		visItems = 1;

	return visItems;
}


void
MListPeer::HideSelf()
{
	(**mScrollControl).contrlVis = 0;
}

void
MListPeer::ShowSelf()
{
	(**mScrollControl).contrlVis = 255;
}

void
MListPeer::EnableSelf(void)
{
	FocusDraw();						// Control will redraw now
	::HiliteControl(mScrollControl, 0);
}

void
MListPeer::DisableSelf(void)
{
	FocusDraw();						// Control will redraw now
	::HiliteControl(mScrollControl, 255);
}


void
MListPeer::DoRelativeScroll(SInt16 fromLocation, SInt16 toLocation)
{
	SInt16			scrollMax = GetControlMaximum(mScrollControl);
	UInt16			getItemsOnScreen = GetItemsOnScreen();
	Rect			frameRect;	
	RgnHandle 		updateRegion = NewRgn();

	CalcLocalFrameRect(frameRect);
	::InsetRect(&frameRect, 1, 1);
	frameRect.right -= kListPeerScrollBarWidth - 1;

	if (toLocation < 1)
		toLocation = 1;
	
	if (toLocation > scrollMax)
		toLocation = scrollMax;

	if (fromLocation == toLocation)
		return;
		
	mScrollPosition = toLocation;
	SetControlValue(mScrollControl, mScrollPosition);
	
	Boolean completeRefresh = false;
	
	FocusDraw();
	
	if (fromLocation < toLocation) {
	
		//	Scroll down
	
		if (toLocation < fromLocation + getItemsOnScreen) {

			ScrollRect(&frameRect, 0, (fromLocation - toLocation) * 
					kListPeerCellHeight, updateRegion);

		}
		
		else {
			completeRefresh = true;
		}
	
	}
	
	else {
	
		//	Scroll up
	
		if (toLocation > fromLocation - getItemsOnScreen) {

			ScrollRect(&frameRect, 0, (fromLocation - toLocation) 
					* kListPeerCellHeight, updateRegion);

		}
		
		else {
			completeRefresh = true;
		}
	
	}
	
	if (completeRefresh) {
		::RectRgn(updateRegion, &frameRect);
	}
	
	SetClip(updateRegion);
	
	Draw(NULL);
	
	DisposeRgn(updateRegion);

	LView::OutOfFocus(NULL);		

}

pascal void ListBoxScrollActionProc(ControlRef theControl, ControlPartCode partCode)
{
	MListPeer	*listPeer = (MListPeer *)GetControlReference(theControl);

	switch (partCode) {
	
		case kControlPageUpPart:
			listPeer->DoRelativeScroll(listPeer->mScrollPosition, listPeer->mScrollPosition - listPeer->GetItemsOnScreen());
			break;

		case kControlPageDownPart:
			listPeer->DoRelativeScroll(listPeer->mScrollPosition, listPeer->mScrollPosition + listPeer->GetItemsOnScreen());
			break;

		case kControlUpButtonPart:
			listPeer->DoRelativeScroll(listPeer->mScrollPosition, listPeer->mScrollPosition - 1);
			break;

		case kControlDownButtonPart:
			listPeer->DoRelativeScroll(listPeer->mScrollPosition, listPeer->mScrollPosition + 1);
			break;

		default:
			break;
	
	}
}

#if GENERATINGCFM
RoutineDescriptor	gListBoxScrollActionProcRD = BUILD_ROUTINE_DESCRIPTOR(uppControlActionProcInfo, &ListBoxScrollActionProc); 
#else
#define gListBoxScrollActionProcRD ListBoxScrollActionProc
#endif

void
MListPeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	Rect				localFrameRect;
	static UInt32 		gLastSelectedItem = 0;

	CalcLocalFrameRect(localFrameRect);
	::InsetRect(&localFrameRect, 1, 1);

	if (::PtInRect(inMouseDown.whereLocal, &localFrameRect)) {
	
		localFrameRect.right -= (kListPeerScrollBarWidth - 1);

		//	In content area

		if (::PtInRect(inMouseDown.whereLocal, &localFrameRect)) {
		
			UInt32		hitItem;
			
			hitItem = (inMouseDown.whereLocal.v - localFrameRect.top) / kListPeerCellHeight;
			
			if (hitItem > GetItemsOnScreen())
				hitItem = 0;
			else
				hitItem += mScrollPosition;
			
			if ((hitItem > 0) && (hitItem <= mItemList->GetCount())) {
				
				if ((!mCanMultiplySelect) || ((inMouseDown.macEvent.modifiers & shiftKey) == 0))
					DeselectAll(hitItem);
				
				SelectItem(hitItem);
			
				if ((GetClickCount() > 1) && (gLastSelectedItem == hitItem)) {

					MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleAction", "(I)V", hitItem - 1);	
	
				}
	
				gLastSelectedItem = hitItem;

			}
			
			else if (((inMouseDown.macEvent.modifiers & shiftKey) == 0))
				DeselectAll(0);
		
		}
		
		//	In scrollbar area
		
		else {
		
			UInt16				newPosition;
			short				partCode;
		
			switch (::TestControl(mScrollControl, inMouseDown.whereLocal)) {
			
				case kControlPageUpPart:
				case kControlPageDownPart:
				case kControlUpButtonPart:
				case kControlDownButtonPart:
					partCode = TrackControl(mScrollControl, inMouseDown.whereLocal, (ControlActionUPP)&gListBoxScrollActionProcRD);
					break;
					
				case kControlIndicatorPart:
					
					//	Let the user drag the thumb, and then re-position the 
					//	list to the resulting position.
					
					TrackControl(mScrollControl, inMouseDown.whereLocal, NULL);
					
					newPosition = GetControlValue(mScrollControl);
					
					DoRelativeScroll(mScrollPosition, newPosition);
					
					break;
					
				default:
					break;
					
			}
				
		}
	
	}
	
}

void
MListPeer::DrawSelf()
{
	Rect		localFrameRect,
				localFrameRectOrig;
	UInt32		currentCellNum,
				lastCellToDraw;
	UInt32		currentOffset;
	UInt32		selectionBoxTop;

	CalcLocalFrameRect(localFrameRectOrig);
	FrameRect(&localFrameRectOrig);
		
	localFrameRect = localFrameRectOrig;
	
	::InsetRect(&localFrameRect, 1, 1);
	localFrameRect.right -= (kListPeerScrollBarWidth - 1);
	::EraseRect(&localFrameRect);
	
	currentOffset = localFrameRect.top + kListPeerCellHeight - 3;
	selectionBoxTop = localFrameRect.top;
	
	lastCellToDraw = mScrollPosition + GetItemsOnScreen() - 1;
	if (lastCellToDraw > mItemList->GetCount())
		lastCellToDraw = mItemList->GetCount();
	
	RgnHandle		savedClip,
					cellRgn;
	
	savedClip = ::NewRgn();
	cellRgn = ::NewRgn();
	
	::GetClip(savedClip);
	
	for (currentCellNum = mScrollPosition; currentCellNum <= lastCellToDraw; currentCellNum++) {
	
		Rect	cellRect;
#ifdef UNICODE_FONTLIST
		LMacCompStr	*currentCellText;
#else
		char	*currentCellText;
#endif
		cellRect = localFrameRect;
		cellRect.top = selectionBoxTop;
		cellRect.bottom = cellRect.top + kListPeerCellHeight;

		//	Make sure that we only draw within the cell 

		::RectRgn(cellRgn, &cellRect);
		::SectRgn(cellRgn, savedClip, cellRgn);
		::SetClip(cellRgn);
		
		//	Get the cell data, and draw its text.
			
		mItemList->FetchItemAt(currentCellNum, &currentCellText);
		
#ifdef UNICODE_FONTLIST
		LJavaFontList *fontlist = (LJavaFontList *)unhand(PaneToPeer(this))->pInternationalData;
		currentCellText->Draw(fontlist, localFrameRect.left + kListItemIndent, currentOffset);
#else
		::MoveTo(localFrameRect.left + kListItemIndent, currentOffset);
		::DrawText(currentCellText, 0, strlen(currentCellText));
#endif		
		//	Invert the item if it is selected.
		
		if (IsItemSelected(currentCellNum)) {
		
			LMSetHiliteMode(LMGetHiliteMode() & !(1 << hiliteBit));
			::InvertRect(&cellRect);
		
		}
		
		currentOffset += kListPeerCellHeight;
		selectionBoxTop += kListPeerCellHeight;
	
	}

	::SetClip(savedClip);
	::DisposeRgn(savedClip);
	::DisposeRgn(cellRgn);
	
	Draw1Control(mScrollControl);
	
}

Boolean
MListPeer::GetItemCell(UInt32 whichItem, Rect &itemRect)
{
	Rect		localFrameRect;
	UInt32		visibleItems;

	if (whichItem < mScrollPosition)
		return false;
		
	CalcLocalFrameRect(localFrameRect);
	::InsetRect(&localFrameRect, 1, 1);
	localFrameRect.right -= (kListPeerScrollBarWidth - 1);
	
	itemRect = localFrameRect;
	
	visibleItems = GetItemsOnScreen();
	
	if (whichItem >= mScrollPosition + visibleItems)
		return false;
		
	whichItem -= mScrollPosition;
	
	itemRect.top = itemRect.top + whichItem * kListPeerCellHeight; 
	itemRect.bottom = itemRect.top + kListPeerCellHeight;
	
	return true;
	
}


void
MListPeer::InvalidateContent()
{
	Rect		localFrameRect;
	FocusDraw();
	CalcLocalFrameRect(localFrameRect);
	::InsetRect(&localFrameRect, 1, 1);
	localFrameRect.right -= (kListPeerScrollBarWidth - 1);
	::InvalRect(&localFrameRect);
}

Boolean
MListPeer::IsItemSelected(UInt32 whichItem)
{	
	return (mSelectionList->FetchIndexOf(&whichItem) != 0);
}

void
MListPeer::SelectItem(UInt32 selectedItem)
{

	if (!IsItemSelected(selectedItem)) {
	
		//	Invert the cell.
		
		Rect		invertRect;
		
		FocusDraw();
		
		if (GetItemCell(selectedItem, invertRect)) {
			LMSetHiliteMode(LMGetHiliteMode() & !(1 << hiliteBit));
			::InvertRect(&invertRect);
		}
		
		//	Add the item to the selection list.
		
		mSelectionList->InsertItemsAt(1, 1, &selectedItem);
	
		//	Call through to the java object to notify of the selection.

		MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleListChanged", "(I)V", selectedItem - 1);	
	
	}
	
}

void
MListPeer::DeselectItem(UInt32 deselectedItem)
{

	if (IsItemSelected(deselectedItem)) {
	
		//	Invert the cell.
		
		Rect		invertRect;
		
		FocusDraw();
		
		if (GetItemCell(deselectedItem, invertRect)) {
			LMSetHiliteMode(LMGetHiliteMode() & !(1 << hiliteBit));
			::InvertRect(&invertRect);
		}
		
		//	Add the item to the selection list.
		
		mSelectionList->Remove(&deselectedItem);
	
		//	Call through to the java object to notify of the selection.

		MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleListChanged", "(I)V", deselectedItem - 1);	
	
	}

}

void
MListPeer::DeselectAll(UInt32 exceptItem)
{
	//	Iterate through all of the items in the list, deselecting
	//	as we go.
	
	LListIterator		selectionIterator(*mSelectionList, iterate_FromEnd);
	UInt32				currentItem;
	
	while (selectionIterator.Previous(&currentItem)) {
	
		if (exceptItem != currentItem)
			DeselectItem(currentItem);
	
	}
	
}


void
MListPeer::AdjustScrollBar()
{
	UInt32		itemsOnScreen = GetItemsOnScreen();
	UInt32		currentListItems = mItemList->GetCount();
	SInt32		scrollMax;
	
	//	Make sure that the changes we make to the scrollbar
	//	don't cause flicker.
	
	(**mScrollControl).contrlVis = 0;

	if (currentListItems != 0) {
	
		scrollMax = currentListItems - itemsOnScreen + 1;
		
		if (scrollMax < 1)
			scrollMax = 1;
			
		if (scrollMax > mItemList->GetCount())
			scrollMax = 0;

		SetControlValue(mScrollControl, mScrollPosition);
		SetControlMinimum(mScrollControl, 1);
		SetControlMaximum(mScrollControl, scrollMax);
		
		if (mScrollPosition > scrollMax)
			mScrollPosition = scrollMax;
		
		//	Whoops.  Check for a scroll position of zero
		//	which will cause a crash -dkc 6/26/96
		if (mScrollPosition == 0)
			mScrollPosition = 1;

	}
		
	else {
	
		SetControlValue(mScrollControl, 0);
		SetControlMinimum(mScrollControl, 0);
		SetControlMaximum(mScrollControl, 0);
		
		mScrollPosition = 1;
		
	}	

	(**mScrollControl).contrlVis = 255;
		
	//	Make sure that we re-draw the scrollbar on the list.
	
	FocusDraw();
	Rect invalRect = (**mScrollControl).contrlRect;
	::InvalRect(&invalRect);

}


//##############################################################################
//##############################################################################


void sun_awt_macos_MListPeer_create(struct Hsun_awt_macos_MListPeer *listPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MListPeer	*listPeer = (Classsun_awt_macos_MListPeer*)unhand(listPeerHandle);
	Classjava_awt_List				*listTarget = (Classjava_awt_List*)unhand(listPeer->target);
	LView							*superView = (LView *)PeerToPane(hostPeerHandle);
	struct SPaneInfo				newListInfo;
	ResIDT							textTraitsId = 0;
	ListHandle						listHandle;
	
	memset(&newListInfo, 0, sizeof(newListInfo));
	
	newListInfo.paneID 			= MListPeer::class_ID;
	newListInfo.visible 		= triState_Off;
	newListInfo.left			= listTarget->x;
	newListInfo.top				= listTarget->y;
	newListInfo.width			= listTarget->width;
	newListInfo.height			= listTarget->height;
	newListInfo.enabled 		= triState_Off;
	newListInfo.userCon 		= (UInt32)listPeerHandle;
	newListInfo.superView		= superView;
	
	MListPeer *listPane = new MListPeer(newListInfo);
	if (listPane == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	listPane->Enable();

	listPeer->mOwnerPane = (long)listPane;
	listPeer->mRecalculateClip = true;

	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MListPeer_setMultipleSelections(struct Hsun_awt_macos_MListPeer *listPeer, long multipleSelections)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);
	
	if (listPane == NULL)
		return;
	
	listPane->mCanMultiplySelect = multipleSelections;
}

long sun_awt_macos_MListPeer_isSelected(struct Hsun_awt_macos_MListPeer *listPeer, long itemNum)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);
	
	if (listPane == NULL)
		return false;
	
	itemNum++;
	
	return (listPane->mSelectionList->FetchIndexOf(&itemNum) != 0);
}

void sun_awt_macos_MListPeer_addItem(struct Hsun_awt_macos_MListPeer *listPeer, struct Hjava_lang_String *string, long index)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);
#ifdef UNICODE_FONTLIST
	LMacCompStr 	*newListItem = NULL;
#else
	char 			*newListItem;
#endif
	
	if (listPane == NULL)
		return;
	
	if (index == -1)
		index = 0x7FFFFFFF;
	else
		index++;
	
	// 	i18n fix!!!
#ifdef UNICODE_FONTLIST
	try {
		newListItem = new LMacCompStr(string);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
#else
	newListItem = allocCString(string);
#endif

	listPane->mItemList->InsertItemsAt(1, index, &newListItem);
	if (MemError() != noErr) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
	
	//	Go through the current selection, move all the items up that 
	//	are after the one we just inserted.
	
	UInt32	totalItems = listPane->mSelectionList->GetCount(),
			currentItemNum;
	UInt32	*currentItem;
	
	for (currentItemNum = 1; currentItemNum <= totalItems; currentItemNum++) {
		currentItem = (UInt32 *)(listPane->mSelectionList->GetItemPtr(currentItemNum));
		if (*currentItem >= index)
			*currentItem++;
	}
	
	listPane->InvalidateContent();
	
	listPane->AdjustScrollBar();

}

void sun_awt_macos_MListPeer_delItems(struct Hsun_awt_macos_MListPeer *listPeer, long firstItem, long lastItem)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);
	
	if (listPane == NULL)
		return;
	
	UInt32			totalListItems = listPane->mItemList->GetCount();

	//	If there are no items in the list, then 
	//	do nothings
	
	if (totalListItems == 0)
		return;

	firstItem++;
	lastItem++;
	
	if (lastItem > totalListItems)
		lastItem = totalListItems;
		
	if (firstItem > totalListItems)
		firstItem = totalListItems;
		
	//	Deselect all the items we are deleting, and remove them
	//	as we go.
	
	UInt32	currentDeadItem;
			
	for (currentDeadItem = lastItem; currentDeadItem >= firstItem; currentDeadItem--) {
#ifdef UNICODE_FONTLIST
		LMacCompStr	*deadSpace;
		listPane->mSelectionList->Remove(&currentDeadItem);
		listPane->mItemList->FetchItemAt(currentDeadItem, &deadSpace);
		listPane->mItemList->RemoveItemsAt(1, currentDeadItem);
		delete deadSpace;
#else
		char	*deadSpace;
		listPane->mSelectionList->Remove(&currentDeadItem);
		listPane->mItemList->FetchItemAt(currentDeadItem, &deadSpace);
		listPane->mItemList->RemoveItemsAt(1, currentDeadItem);
		free(deadSpace);
#endif
	}

	//	Go through the current selection, move all the items down that 
	//	are after the one we just inserted.
	
	UInt32	totalItems = listPane->mSelectionList->GetCount(),
			currentItemNum;
	UInt32	*currentItem;
	
	for (currentItemNum = 1; currentItemNum <= totalItems; currentItemNum++) {
		currentItem = (UInt32 *)(listPane->mSelectionList->GetItemPtr(currentItemNum));
		if (*currentItem > lastItem)
			*currentItem -= (lastItem - firstItem + 1);
	}
	
	listPane->InvalidateContent();
	
	listPane->AdjustScrollBar();
	
}

void sun_awt_macos_MListPeer_select(struct Hsun_awt_macos_MListPeer *listPeer, long itemNum)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);

	if (listPane == NULL)
		return;

	listPane->SelectItem(itemNum + 1);
}

void sun_awt_macos_MListPeer_deselect(struct Hsun_awt_macos_MListPeer *listPeer, long itemNum)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);

	if (listPane == NULL)
		return;

	listPane->DeselectItem(itemNum + 1);
}

void sun_awt_macos_MListPeer_makeVisible(struct Hsun_awt_macos_MListPeer *listPeer, long itemNum)
{
	MListPeer		*listPane = (MListPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)listPeer);

	if (listPane == NULL)
		return;
		
	// FIX ME (IMPLEMENT!)

}

