//	===========================================================================
//	LEventHandler.cp			©1994-5 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LEventHandler.h"
#include	<LSelection.h>
#include	<LSelectableItem.h>
#include	<UCursor.h>
#include	<LModelProperty.h>
#include	<LSelectHandlerView.h>
#include	<LStream.h>

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#pragma	warn_unusedarg off

LManipulator *	LEventHandler::sLastManDown = NULL;

//	===========================================================================
//	Maintenance

LEventHandler::LEventHandler()
{
	mEvtItem = NULL;
	mEvtThing = NULL;
	mEvtManipulator = NULL;
	mEvtSelection = NULL;

	mSelection = NULL;
	mDragItem = NULL;

	mSelectionCanMove = false;
}

LEventHandler::~LEventHandler()
{
	ReplaceSharable(mEvtItem, (LSelectableItem *)NULL, this);
	ReplaceSharable(mEvtThing, (LManipulator *)NULL, this);
	ReplaceSharable(mEvtSelection, (LSelection *)NULL, this);
	ReplaceSharable(mEvtManipulator, (LManipulator *)NULL, this);
	
	ReplaceSharable(mDragItem, (LManipulator *)NULL, this);

	ReplaceSharable(mSelection, (LSelection *)NULL, this);

	ReplaceSharable(sLastManDown, (LManipulator *)NULL, this);
}

#ifndef	PP_No_ObjectStreaming

void	LEventHandler::WriteObject(LStream &inStream) const
{
	inherited::WriteObject(inStream);
	
	inStream << mSelection;
}

void	LEventHandler::ReadObject(LStream &inStream)
{
	inherited::ReadObject(inStream);
	
	SetSelection(ReadStreamable<LSelection *>()(inStream));
}

#endif

void	LEventHandler::Reset(void)
{
	OSErr	err = noErr;

	ReplaceSharable(mEvtItem, (LSelectableItem *)NULL, this);
	ReplaceSharable(mEvtThing, (LManipulator *)NULL, this);
	ReplaceSharable(mEvtSelection, (LSelection *)NULL, this);
	ReplaceSharable(mEvtManipulator, (LManipulator *)NULL, this);

	ReplaceSharable(mDragItem, (LManipulator *)NULL, this);

	ReplaceSharable(sLastManDown, (LManipulator *)NULL, this);

	mSelectionCanMove = false;

	inherited::Reset();
}

LSelection *	LEventHandler::GetSelection(void)
{
	return mSelection;
}

void	LEventHandler::SetSelection(LSelection *inSelection)
{
	ReplaceSharable(mSelection, inSelection, this);
}

void	LEventHandler::NoteOverNewThing(LManipulator *inThing)
/*
	This function is called with appropriate parameters when
	the mouse is over a new manipulator or newly over no manipulator
	(inItem will be NULL).
	
	Override & inherit to provide for cursor shape feedback on based on
	mouse position over manipulators, selectable items, or selections
	(check inItem->ItemType() to determine which).
	
	NOTE:	A first place to look to overide this functionality is in
	LHandlerView
*/
{
	if (mHandlerView)
		mHandlerView->NoteOverNewThing(inThing);
}

void	LEventHandler::NoteOverNewThingSelf(LManipulator *inThing)
/*
	Override & inherit to provide for cursor shape feedback on based on
	mouse position over manipulators, selectable items, or selections
	(check inItem->ItemType() to determine which).
	
	NOTE:	A first place to look to overide this functionality is in
	LHandlerView
*/
{
	if (inThing) {
		switch(inThing->ItemType()) {
			case kManipulator:
				UCursor::Tick(cu_Hand);
				break;
			case kSelectableItem:
			case kSelection:
				UCursor::Tick(cu_Arrow);
				break;
		}
	} else {
		UCursor::Tick(cu_Arrow);
	}
}

Boolean	LEventHandler::OverDifferentThing(
				const Point		inWhere,		//	local coordinates
				LManipulator	*inOldThing,	//	Old item pointer
				LManipulator	**outNewThing	//	Location for new item ptr.
)
/*
	Note, this method gives priority to "things" in the order:
	
		Manipulators (not selections or selectable items)
		Selections
		Selectable items
*/
{
	LManipulator	*itemFound = NULL;
	
	Assert_(outNewThing != NULL);
	
	if (mInBounds)
		itemFound = OverManipulator(inWhere);
	ReplaceSharable(mEvtManipulator, itemFound, this);

	if (mInBounds)
		itemFound = OverSelection(inWhere);
	ReplaceSharable(mEvtSelection, (LSelection *)itemFound, this);

	if (mInBounds)
		itemFound = OverItem(inWhere);
	ReplaceSharable(mEvtItem, (LSelectableItem *)itemFound, this);

	do {
		itemFound = mEvtManipulator;
		if (itemFound)
			break;
		
		itemFound = mEvtSelection;
		if (itemFound)
			break;
		
		itemFound = mEvtItem;
	} while (false);
	
	//	Return
	if (itemFound != inOldThing) {
		*outNewThing = itemFound;
		return true;
	} else {
		return false;
	}
}

LManipulator *	LEventHandler::OverManipulator(Point inWhere)
{
	LManipulator	*rval = NULL;
	
	if (sLastManDown && sLastManDown->PointInRepresentation(inWhere))
		rval = sLastManDown;
	
	if (!rval)
		rval = mHandlerView->OverManipulator(inWhere);
	
	if (rval)
		return rval;
	
	return OverManipulatorSelf(inWhere);
}

LSelection *	LEventHandler::OverSelection(Point inWhere)
{
	LSelection *rval = mHandlerView->OverSelection(inWhere);
	
	if (rval)
		return rval;
	
	return OverSelectionSelf(inWhere);
}

LSelectableItem *	LEventHandler::OverReceiver(Point inWhere)
{
	LSelectableItem *rval = mHandlerView->OverReceiver(inWhere);
	
	if (rval)
		return rval;
	
	return OverReceiverSelf(inWhere);
}

LSelectableItem *	LEventHandler::OverItem(Point inWhere)
{
	LSelectableItem *rval = mHandlerView->OverItem(inWhere);
	
	if (rval)
		return rval;
	
	return OverItemSelf(inWhere);
}

LManipulator *	LEventHandler::OverManipulatorSelf(Point inWhere)
/*
	If overridden, inherit this method.  In the overriding method,
	return the inherited rval if it is non-NULL.
*/
{
	if (mEvtManipulator && mEvtManipulator->PointInRepresentation(inWhere))
		return mEvtManipulator;
			
	return NULL;
}

LSelection *	LEventHandler::OverSelectionSelf(Point inWhere)
/*
	Clients likely will not need to override this method.
*/
{
/*
	Note this checks mSelection and not mEvtSelection -- mEvtSelection
	can pretty much be considered a Boolean flag.
*/
	if (mSelection && mSelection->PointInRepresentation(inWhere))
		return mSelection;
	
	return NULL;
}

LSelectableItem *	LEventHandler::OverReceiverSelf(Point inWhere)
/*
	This is defined here just to avoid ugly casts.

	Functionality is normally provided by a subclass of LHandlerView or
	possibly an event handler subclass.
*/
{
	return NULL;
}

LSelectableItem *	LEventHandler::OverItemSelf(Point inWhere)
{
	if (mEvtItem && mEvtItem->PointInRepresentation(inWhere))
		return mEvtItem;
	
	return NULL;
}

void	LEventHandler::SetEvtThing(LManipulator *inThing)
{
	Assert_(inThing != mEvtThing);
	
	if (mEvtThing)
		mEvtThing->MouseExit();
		
	ReplaceSharable(mEvtThing, inThing, this);
	
	if (mEvtThing)
		mEvtThing->MouseEnter();
}

void	LEventHandler::PreScroll(Point inVector)
{
	inherited::PreScroll(inVector);
	
	if (mDragItem && (GetEvtState() == evs_Drag))
		mDragItem->UnDrawDragFeedback(mLastEvtMouse);
}

void	LEventHandler::PostScroll(Point inVector)
{
	inherited::PostScroll(inVector);
	
	if (mDragItem && (GetEvtState() == evs_Drag))
		mDragItem->DrawDragFeedback(mEvtMouse);
}

//	===========================================================================
//	Implementation

DragTypeT	LEventHandler::GetDragType(void)
{
	DragTypeT	rval = dragType_Unknown;

	if (mEvtThing) {
		switch(mEvtThing->ItemType()) {

			case kManipulator:
				rval = dragType_Manipulator;
				break;
			
			case kSelectableItem:
			case kSelection:
				rval = dragType_Selecting;
				break;
		}
	}
			
	return rval;
}
		
Boolean	LEventHandler::CheckCursor(void)
{
	Boolean	rval = false;
	
	if (mInBounds) {
		rval = inherited::CheckCursor();
		NoteOverNewThing(mEvtThing);
	}
	
	return rval;
}

Boolean	LEventHandler::CheckThings(void)
{
	Boolean			rval = false;
	LManipulator	*tempThing;
	
	if (OverDifferentThing(mEvtMouse, mEvtThing, &tempThing)) {
		SetEvtThing(tempThing);
		rval = true;
	}
	return rval;
}

void	LEventHandler::MouseDown(void)
{
	inherited::MouseDown();
	
	if (mEvtThing)
		mEvtThing->MouseDown();
}

void	LEventHandler::MouseUp(void)
{
	if (mEvtThing)
		mEvtThing->MouseUp();

	inherited::MouseUp();
}

Boolean	LEventHandler::DoCondition(EventConditionT inCondition)
{
	Boolean	rval = false;

	switch (inCondition) {

		//	Actions
		case eva_Down:
			mEvtClickCount++;
			mMaybeDragDragged = false;
			mSelectionCanMove = false;
			mEvtClickChangedSelection = false;
			CheckThings();
			
			if (mEvtSelection && (mEvtClickCount == 1)) {
				mSelectionCanMove = true;
			} else {
			
				CheckThings();	//	adjust for count increment...

				if (mSelection) {
					mEvtClickChangedSelection = true;
					
					if (mEvtRecord.modifiers & cmdKey) {
						mSelection->SelectDiscontinuous(mEvtItem);
					} else if (mEvtRecord.modifiers & shiftKey) {
						mSelection->SelectContinuous(mEvtItem);
					} else {
						mSelection->SelectSimple(mEvtItem);
					}
					
					CheckThings();
					if (mAllowSingleGestureDrag && mEvtSelection) {
						mSelectionCanMove = true;
						CheckCursor();
					}
				}
			}
			break;
		
		case eva_Up:
			if (mSelection && !mEvtClickChangedSelection) {
				mSelection->SelectSimple(mEvtItem);	//	A click through de-selection / selection
				CheckThings();
				CheckCursor();
			}
			break;

		case eva_DStart:
		{
			mMaybeDragDragged = true;
			mDragType = GetDragType();
			
			switch (mDragType) {

				case dragType_Manipulator:
					Assert_(mDragItem == NULL);
					ReplaceSharable(mDragItem, mEvtThing, this);
					if (mDragItem)
						mDragItem->DragStart(mEvtLastButtonWhere);	//	Use real down location
					break;
			}
			break;
		}
		
		case eva_DMove:
		{
			if (mInBounds) {
				switch (mDragType) {
	
					case dragType_Selecting:
						CheckThings();
						if (mSelection)
							mSelection->SelectContinuous(mEvtItem);
						break;
					
					case dragType_Manipulator:
						if (mDragItem)
							mDragItem->DragMove(mLastEvtMouse, mEvtMouse);
						break;
				}
			}
			CheckBoundaryDrag();
			break;
		}
		
		case eva_DStop:
		{
			switch (mDragType) {

				case dragType_Manipulator:
					if (mDragItem)
						mDragItem->DragStop(mEvtMouse);
					break;
			}
			ReplaceSharable(mDragItem, (LManipulator *)NULL, this);
			mEvtClickCount = 0;
			CheckThings();
			CheckCursor();	//	note absent if
			break;
		}

		//	Button conditions
		case evc_Out:
			if (CheckThings())
				CheckCursor();
			rval = mEvtThing != sLastManDown;
			break;
			
		case evc_In:
			if (CheckThings())
				CheckCursor();
			rval = mEvtThing == sLastManDown;
			break;
		
		case evc_Button:
			if (CheckThings())
				CheckCursor();
			if (mEvtThing)
				rval = !mEvtThing->Dragable();
			break;
		
		case evc_NButton:
			rval = !DoCondition(evc_Button);
			break;
		
		//	Button actions
		case eva_BSet:
			ThrowIfNULL_(mEvtThing);
			ReplaceSharable(sLastManDown, mEvtThing, this);
			break;
				
		case eva_BClear:
			ReplaceSharable(sLastManDown, (LManipulator *)NULL, this);
			break;

		//	Default
		default:
			rval = inherited::DoCondition(inCondition);
			break;
	}
	
	return rval;
}
