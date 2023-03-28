//	===========================================================================
//	LEventMachine.cp			©1994-5 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LEventMachine.h"
#include	<LHandlerView.h>

#include	<LManipulator.h>

#include	<UCursor.h>
#include	<UEventUtils.h>

#include	<LCommander.h>
#include	<LAction.h>
#include	<LStream.h>

#ifndef __WINDOWS__
#include <Windows.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
/*
	NOTICE	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	If you try to understand this code make certain you read the separate 
	documentation. The documentation includes a state diagram for the state
	machine implemented by this event handler.  Knowledge of that state
	diagram makes this code much more understandable.
*/
//	===========================================================================

LEventMachine	*LEventMachine::sActiveDragger = NULL;

//	===========================================================================
//	Maintenance

LEventMachine::LEventMachine()
{
	mCommander = NULL;
	mHandlerView = NULL;
	mEvtState = mLastEvtState = evs_Cold;
	mMaybeDragDragged = false;
	mMultiClickDistance = 4;

	mAllowSingleGestureDrag = false;

	mMouseBounds.top = mMouseBounds.left = mMouseBounds.right =
		mMouseBounds.bottom = 0;

	mBDArea = mBDAreaOutside = mMouseBounds;
	mBDArea.top += ScrollBar_Size / 2;
	mBDArea.left += ScrollBar_Size / 2;
	mBDArea.right -= ScrollBar_Size / 2;
	mBDArea.bottom -= ScrollBar_Size / 2;

	mBDDelay = 2;
	mBDInitialDelay = 10;
	
	SetStateTable();
	SetConditionTable();
	SetActionTable();
	SetNoteTable();
	SetFallbackTable();
}

LEventMachine::~LEventMachine()
{
}

#ifndef	PP_No_ObjectStreaming

void	LEventMachine::WriteObject(LStream &inStream) const
{
	inStream << mMouseBounds;
	inStream << mAllowSingleGestureDrag;
	inStream << mMultiClickDistance;
	inStream << mBDDelay;
	inStream << mBDInitialDelay;
	inStream << mBDArea;
	inStream << mBDAreaOutside;
	
	inStream.WriteVP(GetCommander());
}

void	LEventMachine::ReadObject(LStream &inStream)
{
	inStream >> mMouseBounds;
	inStream >> mAllowSingleGestureDrag;
	inStream >> mMultiClickDistance;
	inStream >> mBDDelay;
	inStream >> mBDInitialDelay;
	inStream >> mBDArea;
	inStream >> mBDAreaOutside;
	
	SetCommander(ReadVoidPtr<LCommander *>()(inStream));
}

#endif

//	===========================================================================
//	Options / setup

void	LEventMachine::SetMouseBounds(const Rect &inBounds)
{
	Int32	delta;
	
	delta = mBDAreaOutside.top - mMouseBounds.top;
	mBDAreaOutside.top = inBounds.top + delta;
	delta = mBDArea.top - mMouseBounds.top;
	mBDArea.top = inBounds.top + delta;
	
	delta = mBDAreaOutside.left - mMouseBounds.left;
	mBDAreaOutside.left = inBounds.left + delta;
	delta = mBDArea.left - mMouseBounds.left;
	mBDArea.left = inBounds.left + delta;
	
	delta = mBDAreaOutside.right - mMouseBounds.right;
	mBDAreaOutside.right = inBounds.right + delta;
	delta = mBDArea.right - mMouseBounds.right;
	mBDArea.right = inBounds.right + delta;
	
	delta = mBDAreaOutside.bottom - mMouseBounds.bottom;
	mBDAreaOutside.bottom = inBounds.bottom + delta;
	delta = mBDArea.bottom - mMouseBounds.bottom;
	mBDArea.bottom = inBounds.bottom + delta;
	
	mMouseBounds = inBounds;
}

void	LEventMachine::GetMouseBounds(Rect *outBounds)
{
	*outBounds = mMouseBounds;
}

void	LEventMachine::SetBoundaryRects(const Rect &inInside, const Rect &inOutside)
{
	mBDAreaOutside = inOutside;
	mBDArea = inInside;
}

LHandlerView *	LEventMachine::GetHandlerView(void)
{
	return mHandlerView;
}

void	LEventMachine::SetHandlerView(LHandlerView *inHandlerView)
{
	mHandlerView = inHandlerView;
}

LCommander *	LEventMachine::GetCommander(void) const
{
	return mCommander;
}

void	LEventMachine::SetCommander(LCommander *inCommander)
{
	mCommander = inCommander;
}

//	===========================================================================
//	Table accessors:

EventConditionT *	LEventMachine::GetConditionTable(void)
{
	return mConditions;
}

void	LEventMachine::SetConditionTable(EventConditionT *inTable)
{
	if (!inTable)
		inTable = sConditions;
	
	mConditions = inTable;
}

EventStateT *	LEventMachine::GetStateTable(void)
{
	return mNextState;
}

void	LEventMachine::SetStateTable(EventStateT *inTable)
{
	if (!inTable)
		inTable = sNextState;
	
	mNextState = inTable;
}

EventConditionT *	LEventMachine::GetActionTable(void)
{
	return mActions;
}

void	LEventMachine::SetActionTable(EventConditionT *inTable)
{
	if (!inTable)
		inTable = sActions;
	
	mActions = inTable;
}

EventConditionT *	LEventMachine::GetNoteTable(void)
{
	return mNotes;
}

void	LEventMachine::SetNoteTable(EventConditionT *inTable)
{
	if (!inTable)
		inTable = sNotes;
	
	mNotes = inTable;
}

EventConditionT *	LEventMachine::GetFallbackTable(void)
{
	return mFallbacks;
}

void	LEventMachine::SetFallbackTable(EventConditionT *inTable)
{
	if (!inTable)
		inTable = sFallbacks;
	
	mFallbacks = inTable;
}

EventConditionT	LEventMachine::FindCondition(EventStateT inState, EventVerbT inVerb)
{
	ThrowIfNULL_(mConditions);
	
	return *(mConditions + (inState * evv_EventMachine_Count) + inVerb);
}

EventStateT	LEventMachine::FindNextState(EventStateT inState, EventVerbT inVerb)
{
	ThrowIfNULL_(mNextState);
	
	return *(mNextState + (inState * evv_EventMachine_Count) + inVerb);
}

EventConditionT	LEventMachine::FindAction(EventStateT inState, EventVerbT inVerb)
{
	ThrowIfNULL_(mActions);
	
	return *(mActions + (inState * evv_EventMachine_Count) + inVerb);
}

EventConditionT	LEventMachine::FindNote(EventStateT inState, EventVerbT inVerb)
{
	ThrowIfNULL_(mNotes);
	
	return *(mNotes + (inState * evv_EventMachine_Count) + inVerb);
}

EventConditionT	LEventMachine::FindFallback(EventStateT inState, EventVerbT inVerb)
{
	ThrowIfNULL_(mNotes);
	
	return *(mFallbacks + (inState * evv_EventMachine_Count) + inVerb);
}

Boolean		LEventMachine::GetAllowSingleGestureDrag(void)
{
	return mAllowSingleGestureDrag;
}

void	LEventMachine::SetAllowSingleGestureDrag(Boolean inValue)
{
	mAllowSingleGestureDrag = inValue;
}

//	===========================================================================
//	Required incoming hooks

void	LEventMachine::Activate(void)
{
	mActive = true;
}

void	LEventMachine::Deactivate(void)
{
	mActive = false;
}

//	===========================================================================
//	Query

EventRecord &	LEventMachine::GetEvtRecord(void)
{
	return mEvtRecord;
}

Point & LEventMachine::GetEvtMouse(void)
{
	return mEvtMouse;
}

Int16	LEventMachine::GetClickCount(void)
{
	return mEvtClickCount;
}

Boolean	LEventMachine::IsTardy(void) const
{
	return (mEvtRecord.when - mEvtLastButtonTime) > GetDblTime();
}

#ifndef	abs
#define	abs(a)	((a) > 0 ? (a) : -(a))
#endif

Boolean	LEventMachine::PointIsClose(
	Point	oldPt,
	Point	newPt)
{
	Int16	dx = newPt.h - oldPt.h,
			dy = newPt.v - oldPt.v,
			d = mMultiClickDistance >> 1;

	dx = abs(dx);
	dy = abs(dy);
	
	return ( (dx <= d) && (dy <= d) );
}

Boolean	LEventMachine::PointInHandlerRgn(Point inWhere)
/*
	Returns true if the mouse is over the mMouseBounds rectangle AND
	there is not a different (floating) window under the mouse location.
*/
{
	if (PtInRect(inWhere, &mMouseBounds)) {
		WindowPtr	handlerWindow,
					windowOver;
	
		handlerWindow = (WindowPtr)mHandlerView->GetMacPort();
		LocalToGlobal(&inWhere);
		FindWindow(inWhere, &windowOver);
		if (windowOver == handlerWindow)
			return true;
	}
	
	return false;
}

Boolean	LEventMachine::LastMaybeDragDragged(void)
{
	return mMaybeDragDragged;
}

//	===========================================================================
//	Overrides

void	LEventMachine::OutOfBounds(Point inWhere)
/*
	Returns whether "NoteOutOfBounds" handled event.
	
	Seldom will client code need to override this.
*/
{
	Assert_(mInBounds);
	
	mInBounds = false;
	UCursor::Reset();
}

void	LEventMachine::InBounds(Point inWhere)
/*
	Returns whether "NoteInBounds" handled event.
	
	Seldom will client code need to override this.
*/
{
	Assert_(!mInBounds);

	mInBounds = true;
	UCursor::Reset();
}

void	LEventMachine::BoundaryDrag(Point inWhere)
/*
	Default behavior is for autoscrolling
*/
{
	Point	vector = {0, 0};
	
	if (inWhere.h <= mBDArea.left)
		vector.h = inWhere.h - mBDArea.left;
	else if (mBDArea.right <= inWhere.h)
		vector.h = inWhere.h - mBDArea.right;
		
	if (inWhere.v <= mBDArea.top)
		vector.v = inWhere.v - mBDArea.top;
	else if (mBDArea.bottom <= inWhere.v)
		vector.v = inWhere.v - mBDArea.bottom;
	
	AutoScrollVector(vector);
}

void	LEventMachine::AutoScrollVector(Point inVector)
/*
	Default method calls AutoScrollImage of owning view.
	
	Note:  This default method defeats the "ramping" nature of inVector.
*/
{
	Point	simulatedWhere;
	
	if (inVector.h < 0)
		simulatedWhere.h = mMouseBounds.left + inVector.h;
	if (inVector.h == 0)
		simulatedWhere.h = mMouseBounds.left;
	if (inVector.h > 0)
		simulatedWhere.h = mMouseBounds.right + inVector.h;
	
	if (inVector.v < 0)
		simulatedWhere.v = mMouseBounds.top + inVector.v;
	if (inVector.v == 0)
		simulatedWhere.v = mMouseBounds.top;
	if (inVector.v > 0)
		simulatedWhere.v = mMouseBounds.bottom + inVector.v;
	
	if (mHandlerView && ((inVector.h != 0) || (inVector.v != 0))) {
		PreScroll(inVector);
		mHandlerView->AutoScrollImage(simulatedWhere);
		PostScroll(inVector);
	}
}

void	LEventMachine::PostAction(LAction *inAction)
/*
	To which class should this really belong?
*/
{
	if (mCommander) {
		mCommander->PostAction(inAction);
	} else {
		LCommander::PostAnAction(inAction);
	}
}

void	LEventMachine::PreScroll(Point inVector)
{
	FocusDraw();
}

void	LEventMachine::PostScroll(Point inVector)
{
	FocusDraw();
}

Boolean	LEventMachine::CheckCursor(void)
{
	return false;
}

Boolean	LEventMachine::CheckThings(void)
{
	return false;
}

//	===========================================================================
//	Implementation

Boolean	LEventMachine::IsActive(void)
{
	return mActive;
}

void	LEventMachine::UpdateLastEvt(EventRecord &inEvent, Point inLocalMouse)
{
	//	A new event for the handler
	mLastEvtRecord = mEvtRecord;
	mLastEvtMouse = mEvtMouse;
	mLastEvtState = mEvtState;
	mEvtMouse = inLocalMouse;
	mEvtRecord = inEvent;
//	SetEvtState(?);	//	Determined by subsequent action and state info
}

void	LEventMachine::FocusDraw(void)
{
	if (mHandlerView)
		mHandlerView->FocusDraw();
}

Boolean	LEventMachine::PtInBoundaryArea(Point inWhere)
{
	return !PtInRect(inWhere, &mBDArea);
}

void	LEventMachine::CheckBoundaryDrag(void)
{
	switch (GetEvtState()) {
		case evs_Drag:

			//	IsActive() test required to prevent views in non
			//	front-most windows from auto scrolling.  Allowing such
			//	would me a Mac HI violation.
			if (mHandlerView && PtInBoundaryArea(mEvtMouse) && mHandlerView->IsActive()) {
				Int32	tc = TickCount();
				Boolean	doNotify = false;
				
				if (!mBDHasEntered) {
					mBDHasEntered = true;
					mBDFirstTime = PtInRect(mEvtMouse, &mBDAreaOutside);
					mLastBDTime = tc;
				} else {
					if (!PtInRect(mEvtMouse, &mBDAreaOutside))
						mBDFirstTime = false;
				}
				
				if (mBDFirstTime) {
					doNotify = (tc - mLastBDTime) > mBDInitialDelay;
				} else {
					doNotify = (tc - mLastBDTime) > mBDDelay;
				}
				
				if (doNotify) {
					BoundaryDrag(mEvtMouse);
					mBDFirstTime = false;
					mLastBDTime = tc;
				}
			} else {
				mBDHasEntered = false;
			}
			break;
	}
}

