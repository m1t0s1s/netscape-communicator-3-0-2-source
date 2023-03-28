//	===========================================================================
//	LTextView.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

//	#define		INCLUDE_TX
//	#define	DEBUGRECTS	1
//	#define	DEBUGRECTS	2

#include	"LTextView.h"
#include	<LTextEngine.h>
#include	<LTextModel.h>
#include	<LTextElemAEOM.h>
#include	<LTextSelection.h>
#include	<LTextEditHandler.h>
#include	<LAETypingAction.h>
#include	<LListIterator.h>
#include	<PP_Messages.h>
#include	<LStream.h>
#include	<UTextTraits.h>
#include	<UDrawingState.h>
#include	<UDrawingUtils.h>
#include	<UCursor.h>
#include	<UGWorld.h>
#include	<StClasses.h>

#ifndef		__RESOURCES__
#include	<Resources.h>
#endif

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#pragma	warn_unusedarg off

const Int16		Unwrapped_Width	= 1000;

#define	IsActiveView()	(IsOnDuty() || mHasActiveTextView)

//	===========================================================================
//	Maintenance

/*-
LTextView*	LTextView::CreateTextViewStream(LStream	*inStream)
{
	return new LTextView(inStream);
}
*/

LTextView::LTextView()
{
	mTextModel = NULL;
	mTextEngine = NULL;
	mTextSelection = NULL;
	mHasActiveTextView = false;

	mOldSelectionIsCaret = false;
	mCaretOn = false;
	mCaretTime = TickCount();
	
	mPrintRange = LTextEngine::sTextAll;
	mImageWidthFollowsFrame = false;
}

LTextView::LTextView(LStream	*inStream)
:	LSelectHandlerView(inStream)
{
	mTextModel = NULL;
	mTextEngine = NULL;
	mTextSelection = NULL;
	mHasActiveTextView = false;
	
	mOldSelectionIsCaret = false;
	mCaretOn = false;
	mCaretTime = TickCount();

	mPrintRange = LTextEngine::sTextAll;

#ifndef	PP_No_ObjectStreaming
	SetTextModel(ReadStreamable<LTextModel *>()(*inStream));
	SetTextEngine(ReadStreamable<LTextEngine *>()(*inStream));
#else
	Int32	dummy;
	
	*inStream >> dummy;
	Assert_(dummy == typeNull);
	
	*inStream >> dummy;
	Assert_(dummy == typeNull);
#endif

//	*inStream >> mImageWidthFollowsFrame;
mImageWidthFollowsFrame = true;
}

LTextView::LTextView(LStream *inStream, Boolean inOldStyle)
:	LSelectHandlerView(inStream, inOldStyle)
//	TEMPORARY Constructor
{
	mTextModel = NULL;
	mTextEngine = NULL;
	mHasActiveTextView = false;

	mOldSelectionIsCaret = false;
	mCaretOn = false;
	mCaretTime = TickCount();
	
	mPrintRange = LTextEngine::sTextAll;
}

void	LTextView::FinishCreateSelf(void)
{
	inheritView::FinishCreateSelf();
	
	SetSelection(mSelection);	//	update mTextSelection (object type isn't
								//	known during stream constructor).
}

LTextView::~LTextView()
{
	PostAnAction(NULL);
	
	SetTextEngine(NULL);
	SetTextModel(NULL);
}

//	===========================================================================
//	Text object access

LTextEngine *	LTextView::GetTextEngine(void)
{
	return mTextEngine;
}

void	LTextView::SetTextEngine(LTextEngine *inTextEngine)
{
	if (mTextEngine != inTextEngine) {

		if (mTextEngine) {
			mTextEngine->RemoveListener(this);
			if (mTextEngine->GetHandlerView() == this) {
				mTextEngine->SetHandlerView(NULL);
				OutOfFocus(this);
			}
		}
	
		ReplaceSharable(mTextEngine, inTextEngine, this);
		
		if (mTextEngine)
			mTextEngine->AddListener(this);
	
		FixImage();
		FixView();
	}
}

LTextModel *	LTextView::GetTextModel(void)
{
	return mTextModel;
}

void	LTextView::SetTextModel(LTextModel *inTextModel)
{
	ReplaceSharable(mTextModel, inTextModel, this);
}

LTextSelection*	LTextView::GetTextSelection(void)
{
	return mTextSelection;
}

void	LTextView::SetSelection(LSelection *inSelection)
{
	inherited::SetSelection(inSelection);
	
	mTextSelection = dynamic_cast<LTextSelection *>(mSelection);
	
	ThrowIf_(mTextSelection != mSelection);
}

Boolean	LTextView::GetImageWidthFollowsFrame(void) const
{
	return mImageWidthFollowsFrame;
}

void	LTextView::SetImageWidthFollowsFrame(Boolean inFollows)
{
	mImageWidthFollowsFrame = inFollows;
	OutOfFocus(this);
}

//	===========================================================================
//	Implementation

void	LTextView::SpendTime(const EventRecord &inMacEvent)
{
	if (!mTextEngine)
		return;

	LHandlerView	*handlerView = mTextEngine->GetHandlerView();

	Try_ {	//	If the handler view gets changed (through a FocusDraw)... boom

		LSelectHandlerView::SpendTime(inMacEvent);
		
		if (IsOnDuty()) {
			FocusDraw();
			CaretMaintenance();
		}

		mTextEngine->SetHandlerView(handlerView);
		OutOfFocus(this);
	} Catch_(inErr) {
		mTextEngine->SetHandlerView(handlerView);
		OutOfFocus(this);
		Throw_(inErr);
	} EndCatch_;
}

void	LTextView::ListenToMessage(MessageT inMessage, void *ioParam)
{
	LHandlerView	*handlerView = NULL;
	
	if ((inMessage == msg_BroadcasterDied) && (ioParam == mTextEngine)) {
		mTextEngine = NULL;
		return;
	}

	if (mTextEngine)
		handlerView = mTextEngine->GetHandlerView();

	Try_ {	//	If the handler view gets changed (through a FocusDraw)... boom

		switch (inMessage) {
	
			case msg_ModelScopeChanged:
			{
				FocusDraw();
				if (mTextEngine) {
					SDimension32	size,
									oldSize;
					mTextEngine->GetImageSize(&size);
					GetImageSize(oldSize);
					inheritView::ResizeImageBy(size.width - oldSize.width, size.height - oldSize.height, true);
				}
				break;
			}
	
			case msg_ModelAboutToChange:
			{
				FocusDraw();
				CaretOff();
				mOldSelectionChanged = false;
				break;
			}
				
			case msg_ModelChanged:
			{
				FocusDraw();
				if (mOldSelectionChanged) {
					Assert_(mTextEngine && (mTextEngine->GetNestingLevel() == 1));
					if (mOldSelectionIsSubstantive != (mSelection && mSelection->IsSubstantive()))
						SetUpdateCommandStatus(true);
			
					FocusDraw();
					MutateSelection(IsOnDuty());
					CheckSelectionScroll();
				}
				CaretOn();
				break;
			}
	
			case msg_InvalidateImage:
			{
				FocusDraw();
				Refresh();
				break;
			}
			
			case msg_UpdateImage:
			{
				if (!mTextEngine)
					return;

				ThrowIfNULL_(ioParam);
				TextUpdateRecordT *update = (TextUpdateRecordT *)ioParam;
				UpdateTextImage(update);
				break;
			}
			
			//	for multiview...
			case msg_SelectionAboutToChange:
			{
				mOldSelectionIsSubstantive = mSelection && mSelection->IsSubstantive();
				
				FocusDraw();
				if (mTextEngine && (mTextEngine->GetNestingLevel() == 0)) {
					CaretOff();
					inherited::ListenToMessage(inMessage, ioParam);
				}
				break;
			}

			case msg_SelectionChanged:
			{
				FocusDraw();
				if (mTextEngine && (mTextEngine->GetNestingLevel() == 0)) {
					
					CheckSelectionScroll();

					//	inherited::ListenToMessage(inMessage, ioParam);
					//
					//	Not!!!
					//
					//	The inherited method would always call SetUpdateCommandStatus(true)
					//	which would result in UpdateMenus being called for every typing
					//	event.  This would slow down typing drastically.  Instead,
					//	SetUpdateCommandStatus is called only when mSelection->IsSubstantive
					//	changes.
					{
						if (mOldSelectionIsSubstantive != (mSelection && mSelection->IsSubstantive()))
							SetUpdateCommandStatus(true);
				
						FocusDraw();
						MutateSelection(IsOnDuty());
					}
					CaretOn();
				} else {
					mOldSelectionChanged = true;
				}
				break;
			}

			default:
			{
				inherited::ListenToMessage(inMessage, ioParam);
				break;
			}
		}

		if (mTextEngine)
			mTextEngine->SetHandlerView(handlerView);
		OutOfFocus(this);
		if (handlerView)
			handlerView->FocusDraw();
	} Catch_(inErr) {
		if (mTextEngine)
			mTextEngine->SetHandlerView(handlerView);
		OutOfFocus(this);
		if (handlerView)
			handlerView->FocusDraw();
		Throw_(inErr);
	} EndCatch_;
}

void	LTextView::CheckSelectionScroll(void)
{
	if ((mOnDuty == triState_On) || (mOnDuty == triState_Latent)) {
		if (mSelection && mTextEngine) {
			TextRangeT	range = ((LTextSelection*)mSelection)->GetSelectionRange();
			mTextEngine->ScrollToRange(range);
		}
	}
}

Boolean	LTextView::ObeyCommand(
	CommandT	inCommand,
	void*		ioParam)
{
	Boolean				cmdHandled = true;
	
	switch (inCommand) {
	
		case cmd_SelectAll:
		{
			if (mSelection) {
				LTextElemAEOM 	*newSelect = NULL;
				if (mTextModel) {
					newSelect = mTextModel->MakeNewElem(LTextEngine::sTextAll, cChar);
				}
				mSelection->SelectSimple(newSelect);
			}
			cmdHandled = true;
			break;
		}

		default:
			cmdHandled = LSelectHandlerView::ObeyCommand(inCommand, ioParam);
			break;
	}
	
	return cmdHandled;
}

void	LTextView::FindCommandStatus(
	CommandT	inCommand,
	Boolean		&outEnabled,
	Boolean		&outUsesMark,
	Char16		&outMark,
	Str255		outName)
{
	switch (inCommand) {
	
		case cmd_SelectAll:
			outUsesMark = false;
			if (mSelection)
				outEnabled = true;
			break;
			
		default:
			LSelectHandlerView::FindCommandStatus(inCommand, outEnabled,
									outUsesMark, outMark, outName);
			switch (inCommand) {
				case cmd_Clear:
				case cmd_Cut:
				case cmd_Paste:
					if (!mTextEngine || !(mTextEngine->GetAttributes() & textEngineAttr_Editable))
						outEnabled = false;
					break;
			}
			break;
	}
}

LSelectableItem *	LTextView::OverItem(Point inWhere)
{
	LSelectableItem	*rval = NULL;
	
	if (mTextEngine && mTextModel && mEventHandler) {
		FocusDraw();
		
		//	Find corresponding text offset
		TextRangeT		range;
		Boolean			leadingEdge;
		SPoint32		where;
		LocalToImagePoint(inWhere, where);
		mTextEngine->Image2Range(where, &leadingEdge, &range);
		if (range.Length()) {
			if (leadingEdge)
				range.Front();
			else
				range.Back();
		}
		
		//	Determine part type to find
		DescType		findType = cChar;
		LTextElemAEOM	*oldElem = NULL;
		rval = inherited::OverItem(inWhere);
		if (rval) {									//	inWhere is guaranteed to hit rval
//			member(rval, LTextElemAEOM);
			oldElem = (LTextElemAEOM *)rval;
			findType = oldElem->GetModelKind();	//	in case count > 4
		}
		switch (mEventHandler->GetClickCount()) {
			case 0:
			case 1:
				findType = cChar;
				break;
			case 2:
				findType = cWord;
				break;
			case 3:
				findType = cLine;
				break;
			case 4:
				findType = cParagraph;
				break;
		}
		
		//	Return old equivalent elem or make a new elem
		TextRangeT	foundRange = range;
		DescType	partFound = cChar;
		if (findType != cChar) {
			DescType type = mTextEngine->FindPart(LTextEngine::sTextAll, &foundRange, findType);
			if (type != typeNull)
				partFound = type;
		}
		if (!oldElem || (oldElem->GetRange() != foundRange))
			rval = mTextModel->MakeNewElem(foundRange, partFound);
	}
		
	return rval;
}

LSelectableItem *	LTextView::OverReceiver(Point inWhere)
{
	LSelectableItem	*rval = NULL;
	
	do {
		if (mEventHandler)
			rval = mEventHandler->OverReceiverSelf(inWhere);
		
		if (rval)
			break;
		
		if (!mTextEngine || !mTextModel || !mEventHandler)
			break;
			
		FocusDraw();
	
		//	Find corresponding text offset
		TextRangeT		range;
		Boolean			leadingEdge;
		SPoint32		where;
		LocalToImagePoint(inWhere, where);
		mTextEngine->Image2Range(where, &leadingEdge, &range);
		if (range.Length()) {
			if (leadingEdge)
				range.Front();
			else
				range.Back();
		}
		rval = mTextModel->MakeNewElem(range, cChar);
	} while (false);
	
	return rval;
}

void	LTextView::NoteOverNewThing(LManipulator *inThing)
{
	if (inThing) {
		switch(inThing->ItemType()) {

			case kManipulator:
				UCursor::Reset();	//	?
				break;

			case kSelection:
				if (mEventHandler && ((LDataDragEventHandler *)mEventHandler)->
						GetStartsDataDrags()) {
					LSelectHandlerView::NoteOverNewThing(inThing);	//	Ie. data drag "hand"
					break;
				} else {
					//	fall through
				}

			case kSelectableItem:
				UCursor::Tick(cu_IBeam);
				break;

		}
	} else {
		UCursor::Reset();
	}
}

Boolean	LTextView::FocusDraw(void)
{
	Boolean			rval;
	
	rval = inheritView::FocusDraw();
	
	if (mTextEngine == NULL)
		return rval;
	
	mTextEngine->SetHandlerView(this);
	mTextEngine->SetPort(UQDGlobals::GetCurrentPort());
	FixView();

	return rval;
}

void	LTextView::BeTarget(void)
{
	Assert_(!mHasActiveTextView);
	
	FocusDraw();
	
	LSelectHandlerView::BeTarget();
	
	mHasActiveTextView = true;
	
	CaretOn();
	
	if (mTextModel)
		LModelObject::DoAESwitchTellTarget(mTextModel);
}

void	LTextView::DontBeTarget(void)
{
	Assert_(mHasActiveTextView);
	
	FocusDraw();

	mHasActiveTextView = false;

	CaretOff();
	
	LSelectHandlerView::DontBeTarget();	
}

void	LTextView::SubtractSelfErasingAreas(RgnHandle inSourceRgn)
{
	StRegion	myRgn;
	Rect		r;
	
	if (CalcPortFrameRect(r)) {
		RectRgn(myRgn, &r);
		DiffRgn(inSourceRgn, myRgn, inSourceRgn);
	}

//	NO!	This pane's entire region was already subtracted above!
//
//	inherited::SubtractSelfErasingAreas(inSourceRgn);
//
//	NO!
}

void	LTextView::DrawSelf(void)
{
StProfile	profileMe;

	if (mTextEngine) {
		SPoint32		ul,
						lr;
		SDimension32	size;
		
		GrafPtr			port = UQDGlobals::GetCurrentPort();
		ThrowIfNULL_(port);
		
		Rect			r;
		SectRect( &(*(port->visRgn))->rgnBBox, &(*(port->clipRgn))->rgnBBox, &r);
		LocalToImagePoint(topLeft(r), ul);
		LocalToImagePoint(botRight(r), lr);
		size.width = lr.h - ul.h;
		size.height = lr.v - ul.v;
		
		mTextEngine->DrawArea(ul, size);
	} else {
		Rect	r;
	
		if (CalcLocalFrameRect(r))
			EraseRect(&r);	
	}

	#ifdef	DEBUGRECTS
		DrawDebugRects();
	#endif
	
	LSelectHandlerView::DrawSelf();
}

void	LTextView::DrawSelfSelection(void)
{
	if (mOldSelectionIsCaret) {
		if (mCaretOn)
			CaretUpdate();
	} else {
		inherited::DrawSelfSelection();
	}
}

void	LTextView::CaretUpdate(void)
{
	if (mOldSelectionIsCaret) {
		if (IsActiveView()) {
			StColorPenState	savePen;
		
			PenPat(&UQDGlobals::GetQDGlobals()->black);
			PenMode(srcXor);
			InvertRgn(mOldSelectionRgn.mRgn);
		}
	}
}

void	LTextView::CaretMaintenance(void)
{
	if (mOldSelectionIsCaret) {
		if ((TickCount() - mCaretTime) > GetCaretTime()) {
			if (mCaretOn)
				CaretOff();
			else
				CaretOn();
		}
	}
}

void	LTextView::CaretOn(void)
{
	if (mOldSelectionIsCaret) {
		if (!mCaretOn)
			CaretUpdate();
		
		mCaretOn = IsActiveView();
		mCaretTime = TickCount();
	}
}

void	LTextView::CaretOff(void)
{
	if (mOldSelectionIsCaret) {
		if (mCaretOn)
			CaretUpdate();
	
		mCaretOn = false;
		mCaretTime = TickCount();
	}
}

#define	SelectionIsCaret(isActive)	(								\
									mSelection						\
								&& 	isActive						\
								&&	!mSelection->IsSubstantive()	\
								&&	(mSelection->ListCount() == 1)	\
)																	\

void	LTextView::GetPresentHiliteRgn(
	Boolean		inAsActive,
	RgnHandle	ioRegion,
	Int16		inOriginH,
	Int16		inOriginV
)
{
	if (mSelection) {
		if (mSelection->IsSubstantive()) {
			inherited::GetPresentHiliteRgn(inAsActive, ioRegion, inOriginH, inOriginV);
		} else {
			Point	origin;
			origin.h = inOriginH;
			origin.v = inOriginV;
			mSelection->MakeRegion(origin, &ioRegion);	//	don't make an outline
														//	because that would just be
														//	strange looking.
	
			//	Clip to visible area...
			Rect		r;
			if (CalcLocalFrameRect(r)) {
				StRegion	tempRgn(r);
	
				SectRgn(ioRegion, tempRgn, ioRegion);
				ThrowIfOSErr_(QDError());
			}
		}
	} else {
		URegions::MakeEmpty(ioRegion);
	}
	
	mOldSelectionIsCaret = SelectionIsCaret(inAsActive);
}

void	LTextView::MutateSelectionSelf(
	Boolean			inOldIsCaret,
	const RgnHandle	inOldRegion,
	Boolean			inNewIsCaret,
	const RgnHandle	inNewRegion) const
{
	if (!inOldIsCaret && !inNewIsCaret) {

		StRegion	tempRgn(inOldRegion);
		
		XorRgn(tempRgn, inNewRegion, tempRgn);
		ThrowIfOSErr_(QDError());
		
		URegions::Hilite(tempRgn);
	
	} else if (!inOldIsCaret && inNewIsCaret) {
	
		URegions::Hilite(inOldRegion);
		
	} else if (inOldIsCaret && !inNewIsCaret) {
		
		URegions::Hilite(inNewRegion);

	} else {
	
		//	nada
	
	}
}

void	LTextView::MutateSelection(Boolean inAsActive)
{
	Boolean		oldIsCaret = mOldSelectionIsCaret;
	StRegion	oldRegion(mOldSelectionRgn);
	
	GetPresentHiliteRgn(inAsActive, mOldSelectionRgn);
	mOldSelectionInvalid = false;
	
	MutateSelectionSelf(oldIsCaret, oldRegion, mOldSelectionIsCaret, mOldSelectionRgn);
}

void	LTextView::UpdateTextImage(TextUpdateRecordT *inUpdate)
{
	FocusDraw();

	Range32T frameInterval;			//	in image y-pixels
	frameInterval = TextRangeT(mFrameLocation.v - mImageLocation.v);
	frameInterval.SetLength(mFrameSize.height);				

	if (!frameInterval.Intersects(inUpdate->gap)) {
	
		//	¥	Then lets not so trivially handle this...
		if (inUpdate->gap.Start() > frameInterval.Start()) {
			//	change is completely below this frame
		} else if (inUpdate->scrollAmount < 0) {
			Range32T	zappedBit(inUpdate->scrollStart, inUpdate->scrollStart - inUpdate->scrollAmount);
			
			if (zappedBit.Contains(frameInterval)) {
				inherited::ScrollImageBy(0, mImageLocation.v - mFrameLocation.v + inUpdate->gap.Start(), false);
				FocusDraw();
				DrawSelf();
			} else if (frameInterval.Contains(
						Range32T(inUpdate->scrollStart - inUpdate->scrollAmount)
			) ) {
				inherited::ScrollImageBy(0, mImageLocation.v - mFrameLocation.v + inUpdate->gap.End(), false);
				FocusDraw();
				DrawSelf();
			} else {
				//	view is below changes.
				inherited::ScrollImageBy(0, inUpdate->scrollAmount, false);
			}
		} else {
			//	view is below changes.
			inherited::ScrollImageBy(0, inUpdate->scrollAmount, false);
		}
		MutateSelection(IsActiveView());
		
	} else {

		//	in local coords...
		Rect		rFrame,
					rGap,
					rAbove,		//	region in frame above gap
					rScroll;	//	scrolled rect pre-scroll location

								//	rGap in 32bit! local coords
		Int32		rgTop = inUpdate->gap.Start() + mPortOrigin.v + mImageLocation.v,
					rgLeft = 0 + mFrameLocation.h + mPortOrigin.h,
					rgRight = rgLeft + mFrameSize.width, 
					rgBottom = inUpdate->gap.End() + mPortOrigin.v + mImageLocation.v;

		if (CalcLocalFrameRect(rFrame)) {
			rGap.top = URange32::Max(rFrame.top, rgTop);
			rGap.left = URange32::Max(rFrame.left, rgLeft);
			rGap.right = URange32::Min(rFrame.right, rgRight);
			rGap.bottom = URange32::Min(rFrame.bottom, rgBottom);
		} else {
			Assert_(false);	//	huh?
		}
		rAbove = rFrame;
		rAbove.bottom = rGap.top;
		
		Boolean		oldSelectionIsCaret = mOldSelectionIsCaret;
		StRegion	rOld(mOldSelectionRgn.mRgn),
					rScreen,
					rTemp,
					rMask;
		StRegion	rgn;
		
		GetPresentHiliteRgn(IsActiveView(), mOldSelectionRgn);
	
		//	¥	Scroll
		if (inUpdate->scrollAmount &&
			frameInterval.Contains(Range32T(inUpdate->scrollStart))
		) {
			Rect	r;
	
			RectRgn(rMask, &rAbove);
			SectRgn(rOld, rMask, rScreen);

			r.top = inUpdate->scrollStart + mPortOrigin.v + mImageLocation.v;
			r.left = mFrameLocation.h + mPortOrigin.h;
			r.right = r.left + mFrameSize.width;
			r.bottom = mFrameLocation.v + mFrameSize.height + mPortOrigin.v;
			
			if (inUpdate->scrollAmount < (r.bottom - r.top)) {
				ScrollRect(&r, 0, inUpdate->scrollAmount, rgn);
				
				//	old selection...
				rScroll = r;
				if (inUpdate->scrollAmount > 0)
					rScroll.bottom -= inUpdate->scrollAmount;
				else
					rScroll.top -= inUpdate->scrollAmount;
				RectRgn(rMask, &rScroll);
				SectRgn(rOld, rMask, rTemp);
				OffsetRgn(rTemp, 0, inUpdate->scrollAmount);
				UnionRgn(rScreen, rTemp, rScreen);
			}
			if (inUpdate->scrollAmount < 0) {
				//	(offscreening this section would be a waste)
//				RGBColor	green = {0, 0xffff, 0};	RGBForeColor(&green);	PaintRgn(rgn);
				StClipRgnState	clipMe(rgn);
				DrawSelf();

				//	old selection...
				SectRgn(mOldSelectionRgn, rgn, rTemp);
				UnionRgn(rScreen, rTemp, rScreen);
			}
		} else {
			CopyRgn(rOld, rScreen);
		}
	
		//	¥	Gap
		Rect rReveal;			
		GetRevealedRect(rReveal);
		PortToLocalPoint(topLeft(rReveal));
		PortToLocalPoint(botRight(rReveal));

//-		if (!EmptyRect(&rGap))	{
		if (!EmptyRect(&rGap) && IsVisible() && ::SectRect(&rGap, &rReveal, &rGap))	{
			RectRgn(rgn, &rGap);
//			RGBColor	green = {0, 0xffff, 0};	RGBForeColor(&green);	PaintRgn(rgn);
			Boolean	didit = false;
#if	1
			Try_ {
				StOffscreenGWorld	smoothit(rGap, 0, useTempMem);
//?				StClipRgnState		clipMe(rgn);
				mTextEngine->SetPort(UQDGlobals::GetCurrentPort());
				DrawSelf();
				didit = true;
			} Catch_(inErr) {
				// 	failure okay -- just draw ugly
			} EndCatch_;
			mTextEngine->SetPort(UQDGlobals::GetCurrentPort());
#endif
			if (!didit) {
				StClipRgnState		clipMe(rgn);
				DrawSelf();
			}

			//	knock out gap area and update selection
			DiffRgn(rScreen, rgn, rScreen);
			DiffRgn(mOldSelectionRgn, rgn, rTemp);
			MutateSelectionSelf(oldSelectionIsCaret, rScreen, mOldSelectionIsCaret, rTemp);
		} else {
			MutateSelectionSelf(oldSelectionIsCaret, rScreen, mOldSelectionIsCaret, mOldSelectionRgn);
		}
	}
}


void	LTextView::DrawDebugRects(void)
{
#ifdef	DEBUGRECTS
	StColorState	savePen;
	RGBColor		red = {0xffff, 0, 0},			//	Image
					green = {0, 0xffff, 0},			//	Frame
					blue = {0, 0, 0xffff},			//	text frame
					purple = {0xffff, 0, 0xffff};	//	text image
	Rect			r,
					cr;
	SPoint32		o;
	SDimension32	s;

	StClipRgnState	clipSaver(GetMacPort()->portRect);

	RGBForeColor(&green);
	CalcLocalFrameRect(r);
	FrameRect(&r);

	if (mTextEngine) {
		mTextEngine->GetViewRect(&o, &s);
		r.top = o.v;
		r.left = o.h;
		r.right = r.left + s.width;
		r.bottom = r.top + s.height;
		RGBForeColor(&blue);
		FrameRect(&r);
	}
	
	//	image rects
	#if	DEBUGRECTS > 1
		GetScrollPosition(o);
		GetImageSize(s);
		r.top = 0;
		r.left = 0;
		r.right = r.left + s.width;
		r.bottom = r.top + s.height;
		RGBForeColor(&red);
		FrameRect(&r);
	
		if (mTextEngine) {
			mTextEngine->GetImageSize(&s);
			r.top = 0;
			r.left = 0;
			r.right = r.left + s.width;
			r.bottom = r.top + s.height;
			RGBForeColor(&purple);
			FrameRect(&r);
		}
	#endif
	
#endif
}

void	LTextView::FixView(void)
{
	SPoint32		location;
	SDimension32	size;
	
	if (mTextEngine) {
		GetViewArea(&location, &size);
		mTextEngine->SetViewRect(location, size);
	}
}

void	LTextView::FixImage(void)
{
	SDimension32	size = {0, 0},
					oldSize;

	//	change the image width
	FocusDraw();
		
	//	Wrap to frame size...
	if (mTextEngine  && mImageWidthFollowsFrame) {
		SPoint32		location;
		Rect			r;

		GetScrollPosition(location);

		if (!CalcLocalFrameRect(r))
			Throw_(paramErr);
	
		size.width = r.right - r.left + location.h;
		size.height = 0;

		mTextEngine->GetImageSize(&oldSize);
		if (oldSize.width != size.width) {
			mTextEngine->SetImageWidth(size.width);
			//	Will likely cause an update that
			//	will fix the size of the scrollbars
		}
	}
	
	if (mTextEngine) {
		mTextEngine->GetImageSize(&size);
	} else {
		size.width = mFrameSize.width;
		size.height = mFrameSize.height;
	}

	GetImageSize(oldSize);
	inheritView::ResizeImageBy(size.width - oldSize.width, size.height - oldSize.height, true);
}

void	LTextView::GetViewArea(SPoint32 *outOrigin, SDimension32 *outSize)
{
	Rect			r;

	if (!CalcLocalFrameRect(r))
		Throw_(paramErr);

	GetScrollPosition(*outOrigin);
	outSize->width = r.right - r.left;
	outSize->height = URange32::Min(r.bottom - r.top, mImageSize.height - outOrigin->v);
	outSize->height = URange32::Max(0, outSize->height);
}

void	LTextView::MoveBy(
	Int32	inHorizDelta,
	Int32	inVertDelta,
	Boolean	inRefresh)
{
	inheritView::MoveBy(inHorizDelta, inVertDelta, inRefresh);
	FixView();
}

void	LTextView::ResizeFrameBy(
	Int16		inWidthDelta,
	Int16		inHeightDelta,
	Boolean		inRefresh)
{
	inheritView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);

	FixImage();
}

void	LTextView::ResizeImageBy(
	Int32		inWidthDelta,
	Int32		inHeightDelta,
	Boolean		inRefresh)
{
	SDimension32	size;

	GetImageSize(size);
	size.width += inWidthDelta;

	FocusDraw();	//	for good measure
	
	if (mTextEngine)
		mTextEngine->SetImageWidth(size.width);	//	will do a broadcast back (in
											//	an update event
											//	of ModelScopeChanged that
											//	will in turn call the inherited
											//	method.
}

void	LTextView::ScrollImageBy(
	Int32		inLeftDelta,
	Int32		inTopDelta,
	Boolean		inRefresh)
{
FocusDraw();
	SectRgn(mOldSelectionRgn, UQDGlobals::GetCurrentPort()->visRgn, mOldSelectionRgn);
	ThrowIfOSErr_(QDError());

	SPoint32	oldPortOrigin;	//	Type is important to avoid truncation
								//	errors below.

	oldPortOrigin.h = mPortOrigin.h;
	oldPortOrigin.v = mPortOrigin.v;
	
	FocusDraw();
	CaretOff();
	
	{
		StChange<Boolean>	tempChange(&mScrollingUpdate, true);

FocusDraw();
		if (mTextEngine)
			mTextEngine->ScrollView(inLeftDelta, inTopDelta, inRefresh);
FocusDraw();

	}

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
		MutateSelection(IsActiveView());
	}
	
	CaretOn();

	#ifdef	DEBUGRECTS
		FocusDraw();
		DrawDebugRects();
	#endif
}

void	LTextView::SavePlace(
	LStream		*outPlace)
{
	SPoint32		location = {0, 0};
	SDimension32	size = {0, 0};

	if (mTextEngine)
		mTextEngine->GetViewRect(&location, &size);

	LView::SavePlace(outPlace);
	
	outPlace->WriteData(&location, sizeof(location));
	outPlace->WriteData(&size, sizeof(size));
}


void	LTextView::RestorePlace(
	LStream		*inPlace)
{
	SPoint32		location;
	SDimension32	size;

	LView::RestorePlace(inPlace);

	inPlace->ReadData(&location, sizeof(location));
	inPlace->ReadData(&size, sizeof(size));

	if (mTextEngine)
		mTextEngine->SetViewRect(location, size);
}

void	LTextView::SetPrintRange(const TextRangeT &inRange)
{
	mPrintRange = inRange;
	if (mTextEngine)
		mPrintRange.Crop(mTextEngine->GetTotalRange());
}

const TextRangeT &	LTextView::GetPrintRange(void) const
{
	return mPrintRange;
}

Boolean	LTextView::NextPanelTop(Int32 *ioPanelTop) const
/*
	ioPanelTop is assumed correct on entry
*/
{
	Int32	oldTop = *ioPanelTop;
	Boolean	anotherExists = false;
	
	do {
		if (!mTextEngine)
			break;
		
		//	trivial reject if ioPanelTop is already past mPrintRange bottom
		Range32T	printV(mTextEngine->GetRangeTop(mPrintRange));
		printV.SetLength(mTextEngine->GetRangeHeight(mPrintRange));
		if (*ioPanelTop >= printV.End())
			break;
		
		//	get frame lower left image point
		SDimension16	frameSize;
		GetFrameSize(frameSize);
		SPoint32	llPt;
		llPt.h = 0;
		llPt.v = *ioPanelTop + frameSize.height;
		
		//	get vertical range of last partially(?) visible line
		TextRangeT	llRange;
		Boolean		leadingEdge;
		mTextEngine->Image2Range(llPt, &leadingEdge, &llRange);
		Range32T	llV(mTextEngine->GetRangeTop(llRange));
		llV.SetLength(mTextEngine->GetRangeHeight(llRange));

		if (printV.End() <= llPt.v) {
			*ioPanelTop = printV.End();
		} else {
			*ioPanelTop = llV.Start();
		}
		
		if (*ioPanelTop <= oldTop) {
			//	Then a line is so tall it couldn't fit in a single panel so go ahead and split it
			*ioPanelTop = oldTop + frameSize.height;
		}
		
		if (*ioPanelTop >= printV.End())
			break;
			
		anotherExists = true;

		if (oldTop >= *ioPanelTop) {
			Assert_(false);
			anotherExists = false;
		}
	} while (false);
	
	return anotherExists;
}

void	LTextView::CountPanels(
	Uint32	&outHorizPanels,
	Uint32	&outVertPanels)
{
	SDimension32	imageSize;
	SDimension16	frameSize;

	GetImageSize(imageSize);
	GetFrameSize(frameSize);
	
	outHorizPanels = 1;
	if (frameSize.width > 0  &&  imageSize.width > 0) {
		outHorizPanels = ((imageSize.width - 1) / frameSize.width) + 1;
	}
	
	outVertPanels = 1;

	if (mTextEngine) {
		//	Find the real number of vertical panels
		Int32		panelTop = mTextEngine->GetRangeTop(mPrintRange);
		while (NextPanelTop(&panelTop))
			outVertPanels++;
	}
}

Boolean	LTextView::ScrollToPanel(const PanelSpec &inPanel)
{
	Boolean		panelInImage = false;
	
	if (mTextEngine) {
		Int32		panelTop = mTextEngine->GetRangeTop(mPrintRange),
					panelBottom,
					panelLeft;
		Int32		vertPanel;
		
		vertPanel = 0;
		
		do {
			vertPanel++;
		} while ((vertPanel < inPanel.vertIndex) && NextPanelTop(&panelTop));
		
		if (vertPanel == inPanel.vertIndex) {
			SDimension16	frameSize;
			GetFrameSize(frameSize);
			
			panelBottom = panelTop;
			NextPanelTop(&panelBottom);
			panelLeft = frameSize.width * (inPanel.horizIndex -1);
			
			OutOfFocus(this);
			FocusDraw();
			ScrollImageTo(panelLeft, panelTop, false);
			Rect	clipRect;
			if (CalcLocalFrameRect(clipRect)) {
				mRevealedRect.bottom = mRevealedRect.top + panelBottom - panelTop;
				clipRect.bottom = clipRect.top + panelBottom - panelTop;
				::ClipRect(&clipRect);
			}
			panelInImage = true;
		}
	}
	
	return panelInImage;
}
