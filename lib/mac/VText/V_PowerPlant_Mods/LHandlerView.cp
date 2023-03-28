//	===========================================================================
//	LHandlerView.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LHandlerView.h"
#include	<PP_Messages.h>
#include	<LList.h>
#include	<LListIterator.h>
#include	<LTargeter.h>
#include	<UDrawingState.h>
#include	<LWindow.h>
#include	<LEventHandler.h>
#include	<LStream.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LHandlerView::LHandlerView()
{
	mEventHandler = NULL;
}

#ifdef	EXP_OFFSCREEN
	LHandlerView::LHandlerView(const LView &inOriginal)
	//	:	LView(inOriginal)
#else
	LHandlerView::LHandlerView(const LView &inOriginal)
		:	LView(inOriginal)
#endif
{
	mEventHandler = NULL;
}

#ifdef	EXP_OFFSCREEN
	LHandlerView::LHandlerView(LStream *inStream)
		:	LOffscreenView(inStream)
#else
	LHandlerView::LHandlerView(LStream *inStream)
		:	LView(inStream)
#endif
{
	mEventHandler = NULL;

#ifndef	PP_No_ObjectStreaming
	SetEventHandler(ReadStreamable<LEventHandler *>()(*inStream));
#else
	Int32	dummy;
	
	*inStream >> dummy;
	Assert_(dummy == typeNull);
#endif
}

#ifdef	EXP_OFFSCREEN
	LHandlerView::LHandlerView(LStream *inStream, Boolean inOldStyle)
		:	LOffscreenView(inStream)
#else
	LHandlerView::LHandlerView(LStream *inStream, Boolean inOldStyle)
		:	LView(inStream)
#endif
//	TEMPORARY Constructor
{
Assert_(inOldStyle);
	mEventHandler = NULL;
}

LHandlerView::~LHandlerView()
{
	ReplaceSharable(mEventHandler, (LEventHandler *)NULL, this);
}

void	LHandlerView::FinishCreateSelf(void)
{
	LView::FinishCreateSelf();
	
	if (mEventHandler) {
		if (IsActive()) {
			FocusDraw();
	
			mEventHandler->Activate();
			StartIdling();
		} else {
			FocusDraw();
	
			StopIdling();
			mEventHandler->Deactivate();
		}
	}
}

LEventHandler *	LHandlerView::GetEventHandler(void)
{
	return mEventHandler;
}

void	LHandlerView::SetEventHandler(LEventHandler *inEventHandler)
{
	if (mEventHandler)
		mEventHandler->SetHandlerView(NULL);

	ReplaceSharable(mEventHandler, inEventHandler, this);
	
	if (mEventHandler)
		mEventHandler->SetHandlerView(this);
}

//	===========================================================================
//	Called by mEventHandler (override)

void	LHandlerView::NoteOverNewThing(LManipulator *inThing)
/*
	When overriding, calling this inherited method will take care of several
	things for you.
	
	Typically you would have a block something like:
	
	
	if (inThing) {
		switch(inThing->ItemType()) {
		
			case kManipulator:
				UCursor::Tick(?);						//	A cursor for a manipulator
				break;
				
			case kSelectableItem:
				UCursor::Tick(?);						//	A cursor for a selectable object
				break;

			case kSelection:
				inherited::NoteOverNewThing(inThing);	//	Default data drag "hand"
				break;
				
		}
	} else {
		UCursor::Set(?);
	}
*/
{
	if (mEventHandler)
		mEventHandler->NoteOverNewThingSelf(inThing);
}

/*
	Override these to link in application specific manipulators and
	selectable items.
	
	Shared object policy warning:

		1)
		See the external documentation.
		
		2)
		When implementing your "model" be sure it follows shared object
		policies on its selectable items.  Ie, when a selectable item is
		constructed, make the "model" claim ownership on that selectable
		item.  That claim should not be released until the model is destroyed.
		
		3)
		Note that manipulators, selectable items, and selections are shared
		objects and their use must follow shared object policies.  So...
		
		If methods derived from those below store a manipulator, selectable
		item, or a selection pointer in any semi-persistent variable, that
		variable and its use must follow shared object policies.  A "semi-
		persistent variable" in this case is any variable that will retain
		reference to a manipulator, selectable item, or selection past the
		lifetime of the given Over... method call.
		
		Also note that it is the caller's responsibility to follow shared
		object policies on the returned value.  Meaning:  don't call
		AddUser for the return value.  Since typically only event
		handlers will call these methods, you wont have to be concerned
		with any of this.  In most cases a method such as below will be
		sufficient:
		
			LManipulator * CAModelView::OverSelectableItem(Point inWhere)
			{
				LManipulator	*rval = inherited::OverSelectableItem(inWhere);
				
				if (rval)
					return rval;
				
				rval = myModel->FindSelectableItemFromPoint(inWhere);
				
				return rval;
			}
*/

LManipulator *		LHandlerView::OverManipulator(Point inWhere)
/*
	See large note above.
*/
{
	LManipulator	*rval = NULL;
	
	if (mEventHandler)
		rval = mEventHandler->OverManipulatorSelf(inWhere);
	
	return rval;
}

LSelectableItem *	LHandlerView::OverItem(Point inWhere)
/*
	See large note above.
*/
{
	LSelectableItem	*rval = NULL;
	
	if (mEventHandler)
		rval = mEventHandler->OverItemSelf(inWhere);
	
	return rval;
}

LSelectableItem *	LHandlerView::OverReceiver(Point inWhere)
/*
	See large note above.
*/
{
	LSelectableItem	*rval = NULL;
	
	if (mEventHandler)
		rval = mEventHandler->OverReceiverSelf(inWhere);
	
	if (rval)
		return rval;
	
	//	Add in OverItem results...
	rval = OverItem(inWhere);
	
	return rval;
}

LSelection *		LHandlerView::OverSelection(Point inWhere)
/*
	You should not need to override this method!
	
	It is included in this section for completeness.
*/
{
	LSelection	*rval = NULL;
	
	if (mEventHandler)
		rval = mEventHandler->OverSelectionSelf(inWhere);
	
	return rval;
}

//	===========================================================================
//	Linkage to LEventHandler

void	LHandlerView::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	FocusDraw();
	
	if (mEventHandler)
			mEventHandler->DoEvent(inMouseDown.macEvent);
}

void	LHandlerView::EventMouseUp(const EventRecord &inMacEvent)
{
	EventRecord	rec = inMacEvent;

	LView::EventMouseUp(inMacEvent);

	FocusDraw();
	
	if (mEventHandler)
		mEventHandler->DoEvent(rec);
}

void	LHandlerView::SpendTime(const EventRecord &inMacEvent)
{
	EventRecord	rec = inMacEvent;
		
	if (mEventHandler) {
		FocusDraw();
		mEventHandler->DoEvent(rec);
	}
}

void	LHandlerView::AdjustCursorSelf(
	Point				inPortPt,
	const EventRecord	&inMacEvent)
{
	/*	NOT!
	
		if (mEventHandler) {
			FocusDraw();
			mEventHandler->DoEvent(inMacEvent);
		}
		
		NOT!
	
		LApplication doesn't process a "Null event" to AdjustCursor but just
		returns the "current" event.  Doing so will mess up the event handler
		state machine.
	
	*/
}

//	===========================================================================
//	Linkage to event handler

void	LHandlerView::ActivateSelf(void)
{
	LView::ActivateSelf();
	
	if (mEventHandler) {
		FocusDraw();

		mEventHandler->Activate();
		StartIdling();
	}
}

void	LHandlerView::DeactivateSelf(void)
{
	LView::DeactivateSelf();

	if (mEventHandler) {
		FocusDraw();

		StopIdling();
		mEventHandler->Deactivate();
	}
}

//	===========================================================================
//	Implementation

Boolean	LHandlerView::FocusDraw(void)
{
	Boolean	rval = LView::FocusDraw();
	
	if (rval)
		FixHandlerFrame();
	
	return rval;
}

void	LHandlerView::ResizeFrameBy(
	Int16	inWidthDelta, 
	Int16	inHeightDelta,
	Boolean	inRefresh)
{
	LView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);
	
	FixHandlerFrame();
}
#include	<StClasses.h>

void	LHandlerView::ScrollImageBy(
	Int32		inLeftDelta,
	Int32		inTopDelta,
	Boolean		inRefresh)
{
	LView::ScrollImageBy(inLeftDelta, inTopDelta, inRefresh);
	
	FixHandlerFrame();
}

void	LHandlerView::SetScrollPosition(const SPoint32 &inLocation)
{
	mImageLocation.h = mFrameLocation.h - inLocation.h;
	mImageLocation.v = mFrameLocation.v - inLocation.v;

	CalcPortOrigin();
	OutOfFocus(this);

	Assert_(mSubPanes.GetCount() == 0);

	if (mSuperView != nil) {
		mSuperView->SubImageChanged(this);
	}
}

void	LHandlerView::FixHandlerFrame(void)
{
	Rect	frame;
	Boolean	b;
	
	b = CalcLocalFrameRect(frame);
	Assert_(b);
	
	if (mEventHandler)
		mEventHandler->SetMouseBounds(frame);
}

