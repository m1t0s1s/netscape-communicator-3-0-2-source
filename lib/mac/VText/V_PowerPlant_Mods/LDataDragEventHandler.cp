//	===========================================================================
//	LDataDragEventHandler.cp		©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LDataDragEventHandler.h"
#include	<LSelection.h>
#include	<LSelectableItem.h>
#include	<LAEAction.h>
#include	<UAppleEventsMgr.h>
#include	<UDrawingState.h>
#include	<UMemoryMgr.h>
#include	<LDragTube.h>
#include	<URegions.h>
#include	<LSelectHandlerView.h>
#include	<LStream.h>
#include	<StClasses.h>
#include	<UCursor.h>

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#pragma	warn_unusedarg off

#define	ZOOMSTEPS	12
#define	DragTask	LDataDragTask::sActiveTask

//	===========================================================================
//	Maintenance

DragSendDataUPP			LDataDragEventHandler::sDragSendDataUPP
							= NewDragSendDataProc(LDropArea::HandleDragSendData);

Boolean	LDataDragEventHandler::sPreScrollWasHilited;
LDataDragEventHandler	*LDataDragEventHandler::sReceiveHandler = NULL;

LDataDragEventHandler::LDataDragEventHandler()
:	LDropArea(UQDGlobals::GetCurrentPort())
{
	mReceiver = NULL;
	mPossibleReceiver = NULL;
	mStartsDataDrags = true;
	mReceivesDataDrags = true;
}

LDataDragEventHandler::~LDataDragEventHandler()
{
	ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
	ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);
}

#ifndef	PP_No_ObjectStreaming

void	LDataDragEventHandler::WriteObject(LStream &inStream) const
{
	inStream << mStartsDataDrags;
	inStream << mReceivesDataDrags;

	LEventHandler::WriteObject(inStream);	
}

void	LDataDragEventHandler::ReadObject(LStream &inStream)
{
	inStream >> mStartsDataDrags;
	inStream >> mReceivesDataDrags;

	LEventHandler::ReadObject(inStream);
}

#endif

void	LDataDragEventHandler::Reset(void)
{
	OSErr	err = noErr;

	inheritEventHandler::Reset();
		
	ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
	ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);
	
	Try_ {
		delete DragTask;
		DragTask = NULL;
	} Catch_(err) {
		DragTask = NULL;
	} EndCatch_;
}

Boolean	LDataDragEventHandler::GetStartsDataDrags(void)
{
	return mStartsDataDrags;
}

void	LDataDragEventHandler::SetStartsDataDrags(Boolean inStartsDataDrags)
{
	mStartsDataDrags = inStartsDataDrags;
}

Boolean	LDataDragEventHandler::GetReceivesDataDrags(void)
{
	return mReceivesDataDrags;
}

void	LDataDragEventHandler::SetReceivesDataDrags(Boolean inReceivesDataDrags)
{
	mReceivesDataDrags = inReceivesDataDrags;
}
												
LDataDragTask	*	LDataDragEventHandler::MakeDragTask(
	LDataDragEventHandler	*inHandler,
	Boolean					inHandlerIsSource)
{
	return new LDataDragTask(inHandler, inHandlerIsSource);
}

//	===========================================================================
//	Drag actions:

DataDragT	LDataDragEventHandler::FindDragSemantic(void)
{
	OSErr			err;
	DragAttributes	attributes;
	DataDragT		rval;
	short			modifiers,
					downModifiers,
					upModifiers;
	
	err = GetDragAttributes(DragTask->mDragRef, &attributes);
	ThrowIfOSErr_(err);
	
/*-
	if (!(attributes & dragInsideSenderApplication))
		Throw_(paramErr);	//	Can't be here because sender does the drag.
*/
	
	if ((attributes & dragInsideSenderWindow) && DragTask->mSourceHandler &&
		(DragTask->mSourceHandler == DragTask->GetReceiveHandler())
	) {
		rval = dataDrag_Move;
	} else {
		rval = dataDrag_Copy;
	}
	
	err = GetDragModifiers(DragTask->mDragRef, &modifiers, &downModifiers, &upModifiers);
	ThrowIfOSErr_(err);
	
	if ((upModifiers & optionKey) || (downModifiers & optionKey) || (modifiers & optionKey))
		rval = dataDrag_Copy;

	//	dataDrag_OSpec & Link could be determined through overriding and examing
	//	modifiers.
	
	return rval;
}

//	===========================================================================
//	New features -- data dragging:

void	LDataDragEventHandler::DataDragDo(void)
{
	OSErr			err;
	StRegion		dragRgn;
	Point			origin = {0,0};
	Boolean			dragDone = false;

	Assert_(mReceiver == NULL);
	Assert_(mPossibleReceiver == NULL);
	Assert_(mSelection);
	Assert_(sDragSendDataUPP);
	
	Try_ {
		//	Make the drag... task & tube
		{
			DragTask = MakeDragTask(this, true);
		
			Assert_(DragTask->mDragTube == NULL);
			DragTask->mDragTube = new LDragTube(DragTask->mDragRef, mSelection);
			
			//	Use send data callback
			err = SetDragSendProc(DragTask->mDragRef, sDragSendDataUPP, (LDropArea *)this);
			ThrowIfOSErr_(err);
		}

		//	Reconstruct mouseDown event...
		EventRecord	event = mEvtRecord;
		event.where = mEvtLastButtonWhere;
		LocalToGlobal(&event.where);
		event.what = mouseDown;
		
		//	Drag outline...
//		Assert_(member(mHandlerView, LSelectHandlerView));
		((LSelectHandlerView *)mHandlerView)->GetPresentHiliteRgn(false, dragRgn, event.where.h, event.where.v);
		/*
			It is merely a matter of convention but...
			the "origin less" drag outlines made by inheritEventHandler
			need to be converted to the global origin regions
			used by the OS Drag Manager.
		*/
		LocalToGlobal(&origin);
		origin.h += event.where.h;
		origin.v += event.where.v;
		URegions::Translate(dragRgn, origin);

		Try_ {

			//	Do the drag tracking and maybe the actual transfer...
			err = TrackDrag(DragTask->mDragRef, &event, dragRgn);
			
LView::OutOfFocus(NULL);	//	who knows what got focused.
FocusDropArea();

			//	If transfer was delayed, make sure it is now done.
			if (err == noErr) {
				DragTask->DoTransfer();
				dragDone = true;
			}

LView::OutOfFocus(NULL);	//	who knows what got focused.
FocusDropArea();

		} Catch_(err) {

			//	Rejection zoom
			Point		startPt,
						endPt,
						zoomVector;
			OSErr		newErr;
			
			OffsetRgn(dragRgn, -origin.h, -origin.v);
			newErr = GetDragMouse(DragTask->mDragRef, &startPt, &endPt);
			Assert_(newErr == noErr);
			endPt = mEvtRecord.where;
			zoomVector.h = endPt.h - startPt.h;
			zoomVector.v = endPt.v - startPt.v;
			origin.h = origin.v = 0;
			LocalToGlobal(&origin);
			origin.h += startPt.h;
			origin.v += startPt.v;
			URegions::Translate(dragRgn, origin);
			newErr = ZoomRegion(dragRgn, zoomVector, ZOOMSTEPS, zoomDecelerate);
			Assert_(newErr == noErr);
	
LView::OutOfFocus(NULL);	//	who knows what got focused.
FocusDropArea();

			if (err != userCanceledErr)
				Throw_(err);

		} EndCatch_;
		
		//	Accepting zoom
		//	???

		//	Clean up...
		Reset();
		mMaybeDragDragged = true;	//	oh yes it did!
		
	} Catch_(inErr) {
	
		Reset();

LView::OutOfFocus(NULL);	//	who knows what got focused.
FocusDropArea();

		Throw_(inErr);

	} EndCatch_;
}

//	---------------------------------------------------------------------------
//	Receive Handlers

void	LDataDragEventHandler::DataDragMoveIn(void)
{
	if (DragTask->mSourceHandler != this) {
		mPreDragState = GetEvtState();
		SetEvtState(evs_Drag);
		mDragType = dragType_Data;
	}
	
	if (mReceiver)
		mReceiver->UnDrawSelfReceiver();
	ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
	ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);

	DragTask->NoteReceiveHandler(this);
}

void	LDataDragEventHandler::DataDragMoveOut(void)
{	
	DragTask->NoteReceiveHandler(NULL);	//	will be ignored if it has been "notified"

	if (mReceiver)
		mReceiver->UnDrawSelfReceiver();
	ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
	ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);

	if (DragTask->mSourceHandler != this) {
		SetEvtState(mPreDragState);
	}
	
	if (DragTask->mSourceHandler == NULL) {
		delete DragTask;
		DragTask = NULL;
	}
}

void	LDataDragEventHandler::DataDragTrackMove(void)
{
	Point			mouse,
					temp;
	OSErr			err;
	LSelectableItem	*itemFound,
					*newReceiver;
	
	
	err = GetDragMouse(DragTask->mDragRef, &mouse, &temp);
	ThrowIfOSErr_(err);
	temp = mouse;
	GlobalToLocal(&mouse);
	
	mEvtMouse = mouse;
	CheckBoundaryDrag();
	
	mouse = temp;
	GlobalToLocal(&mouse);

	itemFound = OverReceiver(mouse);
	ReplaceSharable(mPossibleReceiver, itemFound, this);
	newReceiver = mPossibleReceiver;
	
	//	Filter out if receiver is the selection & in the sender drop area
	if (	newReceiver &&
			mSelection &&
			DragTask->mSourceHandler &&
			(DragTask->mSourceHandler->mSelection == mSelection)
	) {
		if (FindDragSemantic() == dataDrag_Move) {
			if (	!mSelection->IndependentFrom(newReceiver) ||
					mSelection->AdjacentWith(newReceiver)
			) {
				//	Can't "move" a selection to inside or adjacent with itself
				newReceiver = NULL;
			}
		} else {
			if (mSelection->ListEquivalentItem(newReceiver)) {
				//	It doesn't make sense to "copy" the selection to itself.
				newReceiver = NULL;
			}
		}
	}
	
	//	Filter out if receiver can't receive the drop
	if (newReceiver) {
		Assert_(DragTask->mDragTube != NULL);
		if (newReceiver->PickFlavorFrom(DragTask->mDragTube) == typeNull)
			newReceiver = NULL;
	}

	if (mReceiver != newReceiver) {
		if (mReceiver)
			mReceiver->UnDrawSelfReceiver();
		ReplaceSharable(mReceiver, newReceiver, this);
		if (mReceiver)
			mReceiver->DrawSelfReceiver();			
	}
	
	if (mReceiver)
		mReceiver->DrawSelfReceiverTick();
}

void	LDataDragEventHandler::PreScroll(Point inVector)
{
	inheritEventHandler::PreScroll(inVector);
	
	if (mReceiver)
		mReceiver->UnDrawSelfReceiver();

	if (mIsHilited) {
		sPreScrollWasHilited = true;
	//	DragPreScroll(mDragRef, -inVector.h, -inVector.v);
		HideDragHilite(DragTask->mDragRef);	//	Above would be better but exact scroll amounts are needed.
		mIsHilited = false;
	} else {
		sPreScrollWasHilited = false;
	}
}

void	LDataDragEventHandler::PostScroll(Point inVector)
{
	inheritEventHandler::PostScroll(inVector);
	
	if (sPreScrollWasHilited) {
	//	DragPostScroll(mDragRef);
		HiliteDropArea(DragTask->mDragRef);	//	Above would be better but exact scroll amounts are needed.  
		mIsHilited = true;
	}

	if (mReceiver)
		mReceiver->DrawSelfReceiver();
}

Boolean LDataDragEventHandler::PtInBoundaryArea(Point inWhere)
{
	Boolean	rval;
	
	if (mDragType == dragType_Data)
		rval = !PtInRect(inWhere, &mBDArea) && PtInRect(inWhere, &mBDAreaOutside);
	else
		rval = inheritEventHandler::PtInBoundaryArea(inWhere);

	return rval;
}

//	===========================================================================
//	Implementation:

void	LDataDragEventHandler::CheckDragRef(DragReference inDragRef)
{
	if (DragTask) {
		if (DragTask->mDragRef != inDragRef)
			Throw_(paramErr);
	} else {
		DragTask = MakeDragTask(this);
		DragTask->mDragRef = inDragRef;
		DragTask->CheckTube();
	}
}

Boolean	LDataDragEventHandler::PointInDropArea(Point inGlobalPt)
{
	Point	localPt = inGlobalPt;
	
	mHandlerView->GlobalToPortPoint(localPt);
	mHandlerView->PortToLocalPoint(localPt);
	
	return PtInRect(localPt, &mMouseBounds);
}

void	LDataDragEventHandler::SetMouseBounds(const Rect &inBounds)
{
	inheritEventHandler::SetMouseBounds(inBounds);
}

DragTypeT	LDataDragEventHandler::GetDragType(void)
{
	DragTypeT	rval = LEventHandler::GetDragType();
	
	if (mEvtThing) {
		if ((mEvtThing->ItemType() == kSelection) && mSelectionCanMove && GetStartsDataDrags())
			rval = dragType_Data;
	}
	
	return rval;
}
		
Boolean	LDataDragEventHandler::DoCondition(EventConditionT inCondition)
{
	Boolean	rval = false;
	
	switch (inCondition) {
		
		case eva_DStart:
		{
			mDragType = GetDragType();
			
			if (mDragType == dragType_Data) {
				DataDragDo();
			} else {
				rval = inheritEventHandler::DoCondition(inCondition);
			}
			break;
		}
		
		default:
			rval = inheritEventHandler::DoCondition(inCondition);
			break;
	}
	
	return rval;
}

void	LDataDragEventHandler::NoteOverNewThingSelf(LManipulator *inThing)
{
	if (GetStartsDataDrags() && inThing && (inThing->ItemType() == kSelection))
		UCursor::Tick(cu_Hand);
	else
		inheritEventHandler::NoteOverNewThingSelf(inThing);
}

LSelectableItem *	LDataDragEventHandler::OverReceiverSelf(Point inWhere)
{
	LSelectableItem *rval = inheritEventHandler::OverReceiverSelf(inWhere);
	
	if (rval)
		return rval;
	
	if (mReceiver)
		if (mReceiver->PointInRepresentation(inWhere))
			return mReceiver;
	
	if (mPossibleReceiver)
		if (mPossibleReceiver->PointInRepresentation(inWhere))
			return mPossibleReceiver;
	
	return NULL;
}

//	===========================================================================
//	Implementation -- DropArea:

void	LDataDragEventHandler::EnterDropArea(
	DragReference	inDragRef,
	Boolean			inDragHasLeftSender)
{
	CheckDragRef(inDragRef);
		
	Try_ {
		LDropArea::EnterDropArea(inDragRef, inDragHasLeftSender);
		mBDHasEntered = false;	//	Ugly fix -- should be somewhere else.
		DataDragMoveIn();
	} Catch_(inErr) {
		ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
		ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);
		Throw_(inErr);
	} EndCatch_;
}

void	LDataDragEventHandler::LeaveDropArea(
	DragReference	inDragRef)
{
	CheckDragRef(inDragRef);
	
	Try_ {
		DataDragMoveOut();
		LDropArea::LeaveDropArea(inDragRef);
	} Catch_(inErr) {
		ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
		ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);
		Throw_(inErr);
	} EndCatch_;
}

void	LDataDragEventHandler::InsideDropArea(
	DragReference	inDragRef)
{
	CheckDragRef(inDragRef);
	
	Try_ {
		LDropArea::InsideDropArea(inDragRef);
		DataDragTrackMove();
	} Catch_(inErr) {
		ReplaceSharable(mReceiver, (LSelectableItem *)NULL, this);
		ReplaceSharable(mPossibleReceiver, (LSelectableItem *)NULL, this);
		Throw_(inErr);
	} EndCatch_;
}

void	LDataDragEventHandler::DoDragReceive(DragReference inDragRef)
{
	CheckDragRef(inDragRef);

	DragTask->NoteReceiveHandler(this);
	DragTask->SetReceiver(mReceiver);
	DragTask->NoteTransfer();
}

void	LDataDragEventHandler::DoDragSendData(
	FlavorType		inFlavor,
	ItemReference	inItemRef,
	DragReference	inDragRef)
{
	CheckDragRef(inDragRef);
	
	DragTask->NoteTransfer();
	
	Assert_(DragTask->mDragTube);
	DragTask->mDragTube->SendFlavorData(inItemRef, inFlavor);
}

Boolean	LDataDragEventHandler::DragIsAcceptable(
	DragReference	inDragRef)
{
	if (!GetReceivesDataDrags())
		return false;

	return true;	//	Assume receivability -- mReceiver will make
					//	the final decision.
}

void	LDataDragEventHandler::FocusDropArea(void)
{
	Assert_(mHandlerView);
	mHandlerView->FocusDraw();
}

void	LDataDragEventHandler::HiliteDropArea(
	DragReference	inDragRef)
{
	Rect	dropRect;
	
	Assert_(mHandlerView);
	mHandlerView->CalcLocalFrameRect(dropRect);
	
	RgnHandle	dropRgn = ::NewRgn();
	::RectRgn(dropRgn, &dropRect);
	::ShowDragHilite(inDragRef, dropRgn, true);
	::DisposeRgn(dropRgn);
}