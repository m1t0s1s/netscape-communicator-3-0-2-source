//	===========================================================================
//	LSelectHandlerView.cp			©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LSelectHandlerView.h"
#include	<LSelection.h>
#include	<LEventHandler.h>
#include	<PP_Messages.h>
#include	<LClipboardTube.h>
#include	<LTargeter.h>
#include	<UDrawingState.h>
#include	<UDrawingUtils.h>
#include	<LWindow.h>
#include	<UDesktop.h>
#include	<StClasses.h>
#include	<LStream.h>

#ifndef		__DESK__
#include	<Desk.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LSelectHandlerView::LSelectHandlerView()
{
	mSelection = NULL;
	mTargeter = NULL;

	pTargeter = 0;
	
	mScrollingUpdate = false;
}

LSelectHandlerView::LSelectHandlerView(LStream *inStream)
	:	LHandlerView(inStream)
{
	mSelection = NULL;
	mTargeter = NULL;

	pTargeter = 0;	
	
	mScrollingUpdate = false;

#ifndef	PP_No_ObjectStreaming
	SetSelection(ReadStreamable<LSelection *>()(*inStream));
#else
	Int32	dummy;
	
	*inStream >> dummy;
	Assert_(dummy == typeNull);
#endif

	*inStream >> pTargeter;
}

LSelectHandlerView::LSelectHandlerView(LStream *inStream, Boolean inOldStyle)
	:	LHandlerView(inStream, inOldStyle)
//	TEMPORARY Constructor
{
	mSelection = NULL;
	mTargeter = NULL;

	mScrollingUpdate = false;
	mOldSelectionInvalid = false;

	pTargeter = 0;	
}

LSelectHandlerView::~LSelectHandlerView()
{
	mTargeter = NULL;	//	Subviews are automatically deleted
	
	ReplaceSharable(mSelection, (LSelection *)NULL, this);
}

void	LSelectHandlerView::FinishCreateSelf(void)
{
	mOldSelectionInvalid = true;

	if (mTargeter == NULL) {
		switch (pTargeter) {

			case -2:	//	for enclosing a scroller
			{
				LTargeter *aTargeter = new LTargeter();
				aTargeter->AttachPane(GetSuperView());
				aTargeter->AttachTarget(this);
				SetTargeter(aTargeter);
				break;
			}
			
			case -1:
			{
				LTargeter *aTargeter = new LTargeter();
				aTargeter->AttachPane(this);
				aTargeter->AttachTarget(this);
				SetTargeter(aTargeter);
				break;
			}
		}
	}
	
	LHandlerView::FinishCreateSelf();

	MutateSelection(IsOnDuty());	//	Initialize old mOldSelectionRgn
}

LSelection *	LSelectHandlerView::GetSelection(void)
{
	return mSelection;
}

void	LSelectHandlerView::SetSelection(LSelection *inSelection)
{
	if (mSelection)
		mSelection->RemoveListener(this);

	ReplaceSharable(mSelection, inSelection, this);

	if (mSelection)
		mSelection->AddListener(this);
		
	MutateSelection(IsOnDuty());
}

//	===========================================================================
//	Selection support...

void	LSelectHandlerView::DrawSelf(void)
//	perform your pre-selection drawing then inherit/call this method.
{
	if (!mScrollingUpdate)
		DrawSelfSelection();
//	else
//		let ScrollImageBy take care of it...
}

void	LSelectHandlerView::DrawSelfSelection(void)
{
	if (mOldSelectionInvalid) {
		GetPresentHiliteRgn(IsOnDuty(), mOldSelectionRgn);
		mOldSelectionInvalid = false;
	}
	
	URegions::Hilite(mOldSelectionRgn);
}

void	LSelectHandlerView::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	FocusDraw();
	
	if (IsActive() && !IsTarget() && !inMouseDown.delaySelect)
		ShouldBeTarget();
	
	LHandlerView::ClickSelf(inMouseDown);
}

void	LSelectHandlerView::Click(
	SMouseDownEvent	&inMouseDown)
{
	Boolean			delaySelect = inMouseDown.delaySelect;
	Boolean			isNonTargetDrag;
	Point			localPt;
	
	isNonTargetDrag = !IsTarget();
	isNonTargetDrag = isNonTargetDrag && mEventHandler && mSelection;
	if (isNonTargetDrag) {
		localPt = inMouseDown.whereLocal;
		PortToLocalPoint(localPt);
		
		FocusDraw();
		isNonTargetDrag = mSelection->PointInRepresentation(localPt);
		//	Conditional to allow picking of only already selected elements...
		//	otherwise selection could be changed w/o bringing a window to
		//	the foreground.  That would be a Mac HI violation.
	}
	
	if (!isNonTargetDrag || !mEventHandler) {
		LView::Click(inMouseDown);
	} else {
		FocusDraw();
		
		//	sort of LPane::Click(inMouseDown);
		inMouseDown.delaySelect = true;
		inMouseDown.whereLocal = localPt;
		UpdateClickCount(inMouseDown);		
		if (ExecuteAttachments(msg_Click, &inMouseDown)) {

			ClickSelf(inMouseDown);

			Assert_(mEventHandler->GetEvtState() == evs_Maybe);
			mEventHandler->Track();
			
			if (!mEventHandler->LastMaybeDragDragged())
				ShouldBeTarget();
		}
	}
}

void LSelectHandlerView::ShouldBeTarget(void)
{
	LWindow	*window = LWindow::FetchWindowObject(GetMacPort());
	if (window) {
		LCommander	*presentTarget = GetTarget(),
					*p = presentTarget;
		Boolean		windowHasTarget = false;
		
		while (p) {
			if (p == window) {
				windowHasTarget = true;
				break;
			}
			p = p->GetSuperCommander();
		}
		
		if (windowHasTarget) {
			LCommander::SwitchTarget(this);
		} else {
			window->SetLatentSub(this);
		}
	}
}

//	===========================================================================
//	LCommander things

Boolean	LSelectHandlerView::HandleKeyPress(const EventRecord &inKeyEvent)
{
	Boolean	rval = false;
	
	FocusDraw();
	
	if (!mEventHandler || !mEventHandler->DoEvent(inKeyEvent))
		rval = LCommander::HandleKeyPress(inKeyEvent);

	return rval;
}

void	LSelectHandlerView::BeTarget(void)
{
	LCommander::BeTarget();
	
	if (mTargeter)
		mTargeter->ShowFocus();

	FocusDraw();
		
	MutateSelection(true);
}

void	LSelectHandlerView::DontBeTarget(void)
{
	LCommander::DontBeTarget();
	
	if (mTargeter)
		mTargeter->HideFocus();

	FocusDraw();

	MutateSelection(false);
}

void	LSelectHandlerView::SetTargeter(LTargeter *inTargeter)
{
	mTargeter = inTargeter;
}

Boolean	LSelectHandlerView::ObeyCommand(
	CommandT	inCommand,
	void*		ioParam)
{
	Boolean		cmdHandled = true;
	LAction		*action = NULL;
	
	if (inCommand == msg_TabSelect) {
		//	Want the command handled but don't want the selection changed.
		cmdHandled = true;			
	} else if (!mSelection) {
		FocusDraw();
		cmdHandled = inheritCommander::ObeyCommand(inCommand, ioParam);
	} else {
		switch (inCommand) {
			case cmd_Cut:
				action = mSelection->MakeCutAction();
				PostAnAction(action);
				cmdHandled = true;
				break;
	
			case cmd_Copy:
				action = mSelection->MakeCopyAction();
				PostAnAction(action);
				cmdHandled = true;
				break;
	
			case cmd_Paste:
				action = mSelection->MakePasteAction();
				PostAnAction(action);
				cmdHandled = true;
				break;
	
			case cmd_Clear:
				action = mSelection->MakeClearAction();
				PostAnAction(action);
				cmdHandled = true;
				break;

			default:
				cmdHandled = inheritCommander::ObeyCommand(inCommand, ioParam);
				break;
		}
	}

	return cmdHandled;
}

void	LSelectHandlerView::FindCommandStatus(
	CommandT	inCommand,
	Boolean		&outEnabled,
	Boolean		&outUsesMark,
	Char16		&outMark,
	Str255		outName)
{
	switch (inCommand) {
		case cmd_Copy:
		case cmd_Clear:
		case cmd_Cut:
		case cmd_Paste:
		case cmd_SelectAll:
			outUsesMark = false;
			outEnabled = false;
	}

	switch (inCommand) {
	
		case cmd_Copy:
		case cmd_Clear:
		case cmd_Cut:
			if (mSelection && mSelection->IsSubstantive())
					outEnabled = true;
			break;
					
		case cmd_Paste:
			if (mSelection) {
				LClipboardTube	tube;
				FlavorType		flavor;
				
				flavor = mSelection->PickFlavorFrom(&tube);
				if (flavor != typeNull)
					outEnabled = true;
			}
			break;

//		case cmd_SelectAll:		//	Must be done by overrider

		default:
			inheritCommander::FindCommandStatus(inCommand, outEnabled,
									outUsesMark, outMark, outName);
			break;
	}
}

//	===========================================================================
//	LCommander things

void	LSelectHandlerView::ListenToMessage(MessageT inMessage, void *ioParam)
{
	switch (inMessage) {

		case msg_SelectionChanged:
		{
			SetUpdateCommandStatus(true);
			
			FocusDraw();
			MutateSelection(IsOnDuty());
			break;
		}
	}
}

//	===========================================================================
//	LSelection hiliting...

void	LSelectHandlerView::GetPresentHiliteRgn(
	Boolean		inAsActive,
	RgnHandle	ioRegion,
	Int16		inOriginH,
	Int16		inOriginV)
{
	ThrowIfNULL_(ioRegion);
	
	if (mSelection) {
		Point	origin = {0, 0};	
			
		mSelection->MakeRegion(origin, &ioRegion);
	
		//	Clip to visible area...
		Rect		r;
		if (CalcLocalFrameRect(r)) {
			StRegion	tempRgn(r);

			SectRgn(ioRegion, tempRgn, ioRegion);
			ThrowIfOSErr_(QDError());
		}
	
		if (!inAsActive)
			URegions::MakeRegionOutline(ioRegion, &ioRegion);
		
		if (inOriginH || inOriginV) {
			origin.h = -inOriginH;
			origin.v = -inOriginV;	
			URegions::Translate(ioRegion, origin);
		}
	} else {
		URegions::MakeEmpty(ioRegion);
	}
}

void	LSelectHandlerView::MutateSelection(Boolean inAsActive)
{
	FocusDraw();
	StRegion	tempRgn(mOldSelectionRgn.mRgn);

	GetPresentHiliteRgn(inAsActive, mOldSelectionRgn);
	mOldSelectionInvalid = false;

	XorRgn(tempRgn, mOldSelectionRgn, tempRgn);
	ThrowIfOSErr_(QDError());

	URegions::Hilite(tempRgn);
}

void	LSelectHandlerView::MoveBy(
	Int32	inHorizDelta,
	Int32	inVertDelta,
	Boolean	inRefresh)
{
	mOldSelectionInvalid = true;
	LHandlerView::MoveBy(inHorizDelta, inVertDelta, inRefresh);
}

void	LSelectHandlerView::ResizeFrameBy(
	Int16		inWidthDelta,
	Int16		inHeightDelta,
	Boolean		inRefresh)
{
	mOldSelectionInvalid = true;
	LHandlerView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);
	
	OutOfFocus(this);
	FocusDraw();
}

void	LSelectHandlerView::ScrollImageBy(
	Int32		inLeftDelta,			// Pixels to scroll horizontally
	Int32		inTopDelta,				// Pixels to scroll vertically
	Boolean		inRefresh)
{
	FocusDraw();	//	for floating windows that obscure this view...
	SectRgn(mOldSelectionRgn, UQDGlobals::GetCurrentPort()->visRgn, mOldSelectionRgn);
	ThrowIfOSErr_(QDError());

	SPoint32	oldPortOrigin;	//	Type is important to avoid truncation
								//	errors below.

	oldPortOrigin.h = mPortOrigin.h;
	oldPortOrigin.v = mPortOrigin.v;
	
	{
		StChange<Boolean>	tempChange(&mScrollingUpdate, true);

		LHandlerView::ScrollImageBy(inLeftDelta, inTopDelta, inRefresh);
	}

FocusDraw();
	if (	(mPortOrigin.h != (oldPortOrigin.h + inLeftDelta)) ||
			(mPortOrigin.v != (oldPortOrigin.v + inTopDelta))
	) {
		//	The Quickdraw coordinate system just got wrapped so...
		//
		//	Convert:	the old hilite rgn in the old coordinate system to...
		//				the old hilite rgn in the NEW coordinate system.
		SPoint32	delta32;
		Point		delta;
		
		delta32.h = mPortOrigin.h - (oldPortOrigin.h + inLeftDelta);
		delta32.v = mPortOrigin.v - (oldPortOrigin.v + inTopDelta);
		Assert_((min_Int16 <= delta.h) && (delta.h <= max_Int16));
		Assert_((min_Int16 <= delta.v) && (delta.v <= max_Int16));
		delta.h = delta32.h;
		delta.v = delta32.v;
		URegions::Translate(mOldSelectionRgn, delta);
	}
	
	if (inRefresh && (inLeftDelta || inTopDelta)) {
		MutateSelection(IsOnDuty());
	}
}


