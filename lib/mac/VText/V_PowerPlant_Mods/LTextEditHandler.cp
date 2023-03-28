//	===========================================================================
//	LTextEditHandler.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextEditHandler.h"
#include	<LTextSelection.h>
#include	<LTextModel.h>
#include	<LTextElemAEOM.h>
#include	<LStream.h>
#include	<PP_KeyCodes.h>
#include	<LAETypingAction.h>
#include	<LView.h>
#include	<UEventUtils.h>
#include	<UKeyFilters.h>
#include	<UMemoryMgr.h>

#pragma	warn_unusedarg off

const Int32	LTextEditHandler::kNormalCharBuffMaxSize = 256;
Int32		LTextEditHandler::sNormalCharBuffSize;
char		LTextEditHandler::sNormalCharBuff[kNormalCharBuffMaxSize];

//	===========================================================================
//	Maintenance:

LTextEditHandler::LTextEditHandler()
{
	mTextEngine = NULL;
	mTextModel = NULL;
	
	mHasBufferedKeyEvent = false;
	mUpDownHorzPixel = -1;
}

LTextEditHandler::~LTextEditHandler()
{
	SetTextModel(NULL);
	SetTextEngine(NULL);
}

#ifndef	PP_No_ObjectStreaming

void	LTextEditHandler::WriteObject(LStream &inStream) const
{
	inherited::WriteObject(inStream);
	
	inStream <<	mTextEngine;
	inStream <<	mTextModel;
}

void	LTextEditHandler::ReadObject(LStream &inStream)
{
	inherited::ReadObject(inStream);
	
	SetTextEngine(ReadStreamable<LTextEngine *>()(inStream));
	SetTextModel(ReadStreamable<LTextModel *>()(inStream));
}

#endif

void	LTextEditHandler::Reset(void)
{
	mHasBufferedKeyEvent = false;
	mUpDownHorzPixel = -1;

	inherited::Reset();
}

LTextEngine *	LTextEditHandler::GetTextEngine(void)
{
	return mTextEngine;
}

void	LTextEditHandler::SetTextEngine(LTextEngine *inTextEngine)
{
	ReplaceSharable(mTextEngine, inTextEngine, this);
}

LTextModel *	LTextEditHandler::GetTextModel(void)
{
	return mTextModel;
}

void	LTextEditHandler::SetTextModel(LTextModel *inTextModel)
{
	ReplaceSharable(mTextModel, inTextModel, this);
}

void	LTextEditHandler::SetSelection(LSelection *inSelection)
{
	inherited::SetSelection(inSelection);
	
	mTextSelection = NULL;
	if (mSelection) {
//		if (member(mSelection, LTextSelection))
			mTextSelection = (LTextSelection *)mSelection;
	}
}

//	===========================================================================
//	Implementation
//		LEventHandler

Boolean	LTextEditHandler::DragIsAcceptable(
	DragReference	inDragRef)
{
	if (!inherited::DragIsAcceptable(inDragRef))
		return false;

	if (!mTextEngine)
		return false;

	if (!(mTextEngine->GetAttributes() & textEngineAttr_Editable))
		return false;

	return true;
}

LHandlerView	*LTextEditHandler::sSavedHandlerView = NULL;

void	LTextEditHandler::EnterDropArea(
	DragReference	inDragRef,
	Boolean			inDragHasLeftSender)
{
	if (mTextEngine) {
		sSavedHandlerView = mTextEngine->GetHandlerView();
	} else {
		sSavedHandlerView = NULL;
	}

	inherited::EnterDropArea(inDragRef, inDragHasLeftSender);
}

void	LTextEditHandler::LeaveDropArea(
	DragReference	inDragRef)
{
	inherited::LeaveDropArea(inDragRef);

	if (mTextEngine) {
		mTextEngine->SetHandlerView(sSavedHandlerView);
		sSavedHandlerView = NULL;
	}

	LView::OutOfFocus(NULL);
}

//	===========================================================================
//	for better updating...

#include	<LAEAction.h>


LAction *	LTextEditHandler::MakeCreateTask(void)
{
	LAEAction	*action = (LAEAction *)inherited::MakeCreateTask();
	action->SetRecalcAccumulator(mTextEngine);
	return action;
}

LAction *	LTextEditHandler::MakeCopyTask(void)
{
	LAEAction	*action = (LAEAction *)inherited::MakeCopyTask();
	action->SetRecalcAccumulator(mTextEngine);
	return action;
}

LAction *	LTextEditHandler::MakeMoveTask(void)
{
	LAEAction	*action = (LAEAction *)inherited::MakeMoveTask();
	action->SetRecalcAccumulator(mTextEngine);
	return action;
}

LAction *	LTextEditHandler::MakeOSpecTask(void)
{
	LAEAction	*action = (LAEAction *)inherited::MakeOSpecTask();
	action->SetRecalcAccumulator(mTextEngine);
	return action;
}
	
LAction *	LTextEditHandler::MakeLinkTask(void)
{
	LAEAction	*action = (LAEAction *)inherited::MakeLinkTask();
	action->SetRecalcAccumulator(mTextEngine);
	return action;
}

//	===========================================================================
//	Undoable typing

#include	<StClasses.h>

void	LTextEditHandler::MouseMove(void)
{
	switch (mEvtState) {
		case evs_Drag:
		{
			inherited::MouseMove();
			break;
		}
		
		default:
			inherited::MouseMove();
			break;
	}
}

void	LTextEditHandler::KeyDown(void)
{
StProfile	profileMe;

	mKeyEvent = true;

	inherited::KeyDown();	//	Update state machine

	//	Trivial reject
	if (!mTextEngine || !mTextSelection || !mTextModel || (mTextSelection->ListCount() != 1))
		return;

	//	Ignore key presses while the button is down (but save the last one).
	if (mEvtState == evs_Maybe || mEvtState == evs_Delayed || mEvtState == evs_Drag) {
		mBufferedKeyEvent = mEvtRecord;
		mHasBufferedKeyEvent = true;
		return;
	}
	
	if (!(mEvtRecord.modifiers & cmdKey)) {

		if (IsNonTypingKey(mEvtRecord)) {
		
			ObscureCursor();
			
			DoNonTypingKey();
			
			PurgePendingStyle();
			FollowStyleScript();	//	if moved into new script range

		} else if (IsTypingKey(mEvtRecord)) {

			if (mTextEngine->GetAttributes() & textEngineAttr_Editable) {
				StValueChanger<Boolean>	tempAlwaysChange(mTextEngine->ScrollPreferences().doAlways, true);

				FollowKeyScript();	//	if key script was changed
				LAETypingAction			*typing = mTextSelection->GetTypingAction();
		
				//	New typing action?
				if (!typing || typing->NewTypingLocation()) {
					typing = NewTypingAction();
					PostAction(typing);
				}
		
				ObscureCursor();
				
				if (IsNormalChar(mEvtRecord))
					DoNormalChars();
				else
					DoTypingKey();
	
				typing->UpdateTypingLocation();
			} else {
				Throw_(wrPermErr);	//	should read "can't type in read only text engine"
			}
		}
		
		mKeyHandled = true;
	}
}

Boolean	LTextEditHandler::DoLiteralEvent(const EventRecord &inEvent)
{
	mKeyEvent = false;	//	very ugly coupling because KeyDown & etc don't have a "handled" return value
	mKeyHandled = false;
	
	Boolean	rval = inherited::DoLiteralEvent(inEvent);
	
	if (mKeyEvent && !mKeyHandled)
		rval = false;

	return rval;
}

LAETypingAction *	LTextEditHandler::NewTypingAction(void)
{
	LAETypingAction	*rval = new LAETypingAction(mTextEngine);

	rval->SetSelection(mSelection, kAEAfter);
	rval->FinishCreateSelf();
	
	return rval;
}

void	LTextEditHandler::FollowKeyScript(void)
{
}

void	LTextEditHandler::FollowStyleScript(void)
{
}

void	LTextEditHandler::ApplyPendingStyle(const TextRangeT &inRange)
{
}

void	LTextEditHandler::PurgePendingStyle(void)
{
}

Boolean	LTextEditHandler::IsTypingKey(const EventRecord &inEvent) const
{
	Boolean rval = false;

	switch (inEvent.what) {
		case keyDown:
		case keyUp:
		case autoKey:
		{
			char	key = inEvent.message & charCodeMask;
			
			switch (key) {

				case char_Backspace:
				case char_FwdDelete:
				case char_Return:
				case char_Tab:
					rval = true;
					break;
				
				default:
					rval = IsNormalChar(inEvent);
					if (!rval)
						rval = UKeyFilters::IsPrintingChar(key);
					break;
			}
			break;
		}
	}

	return rval;
}

Boolean	LTextEditHandler::IsNonTypingKey(const EventRecord &inEvent) const
{
	Boolean rval = false;

	switch (inEvent.what) {
		case keyDown:
		case keyUp:
		case autoKey:
			switch (inEvent.message & charCodeMask) {
				case char_LeftArrow:
				case char_RightArrow:
				case char_UpArrow:
				case char_DownArrow:
				case char_Home:
				case char_End:
				case char_PageUp:
				case char_PageDown:
					rval = true;
					break;
			}
			break;
	}

	return rval;
}

Boolean	LTextEditHandler::IsNormalChar(const EventRecord &inEvent) const
{
	char	key = inEvent.message & charCodeMask;
	Boolean rval = false;
	
	switch (inEvent.what) {
		case keyDown:
		case keyUp:
		case autoKey:
			rval = (' ' <= key) && (key <= '~');
			break;
	}

	return rval;
}

void	LTextEditHandler::DoNonTypingKey(void)
{
	TextRangeT		range = mTextSelection->GetSelectionRange(),
					postSelRange = range;
		
	DoNonTypingKeySelf(range, &postSelRange);
	
	//	Autoscrolling
	Boolean	doTop = range.Start() != postSelRange.Start();
	SScrollPrefs	&prefs = mTextEngine->ScrollPreferences();
	StValueChanger<Boolean>	tempEndChange(prefs.doEnd, doTop ? false : true);
	StValueChanger<Boolean>	tempAlwaysChange(prefs.doAlways, true);
	
	//	Selection update
	mTextSelection->SetSelectionRange(postSelRange);
}

void	LTextEditHandler::DoNonTypingKeySelf(
	const TextRangeT	&inPreSelectionRange,
	TextRangeT			*outPostSelectionRange)
{
	TextRangeT	range = (inPreSelectionRange.Undefined() ? LTextEngine::sTextAll : inPreSelectionRange);
	Boolean		shrink = (range.Length() && !(mEvtRecord.modifiers & shiftKey));
	char		key = mEvtRecord.message & charCodeMask;

	//	Remap option up and down to pageup and pagedown
	if (mEvtRecord.modifiers & optionKey) {
		if (key == char_UpArrow)
			key = char_PageUp;
		if (key == char_DownArrow)
			key = char_PageDown;
	}
	
	Boolean	forward;
	switch (key)
	{
		case char_Home:
			shrink = false;	//	and fall through
		case char_LeftArrow:
		case char_UpArrow:
		case char_PageUp:
			forward = true;
			break;
			
		case char_End:
			shrink = false;	//	and fall through
		case char_RightArrow:
		case char_DownArrow:
		case char_PageDown:
			forward = false;			
	}

	if (forward)
		range.Front();
	else
		range.Back();

	if (!shrink) {

		switch (key) {
			case char_Home:
				range = LTextEngine::sTextStart;
				break;
				
			case char_End:
				range = LTextEngine::sTextEnd;
				break;
				
			case char_LeftArrow:
			case char_RightArrow:
			{
				Int32		offset = range.Start(),
							size;
						
				if (forward) {
					size = mTextEngine->PrevCharSize(offset);
					offset -= size;
					range.SetInsertion(rangeCode_After);
				} else {
					size = mTextEngine->ThisCharSize(offset);
					offset += size;
					range.SetInsertion(rangeCode_Before);
				}
				range.SetStart(offset);
				mUpDownHorzPixel = -1;
				break;
			}
				
			case char_UpArrow:
			case char_DownArrow:
			case char_PageUp:
			case char_PageDown:
			{
				Assert_(!range.Length());
				Int32	pvDelta;
				switch (key) {
				
					case char_DownArrow:
						pvDelta = 1;
						range.SetInsertion(rangeCode_After);
						break;
						
					case char_UpArrow:
						pvDelta = mTextEngine->GetRangeHeight(range);
						break;
					
					case char_PageUp:
					case char_PageDown:
					{
						SPoint32		origin;
						SDimension32	size;
						mTextEngine->GetViewRect(&origin, &size);
						pvDelta = size.height;
						break;
					}
				}
			
				SPoint32	p;
				mTextEngine->Range2Image(range, true, &p);
				if (mUpDownHorzPixel < 0) {
					mUpDownHorzPixel = p.h;
				} else {
					p.h = mUpDownHorzPixel;
				}
				if (forward)
					p.v -= pvDelta;
				else
					p.v += pvDelta;
				
				Boolean		leadingEdge;
				mTextEngine->Image2Range(p, &leadingEdge, &range);
				if (range.Length()) {
					if (leadingEdge)
						range.Front();
					else
						range.Back();
				} else {
#if	0
					if (
						(mTextEngine->GetTotalRange().End() > range.Start()) &&
						(mTextEngine->ThisChar(range.Start()) == 0x0d) &&
						!forward
					) {
						range.SetStart(range.Start() + 1);
					}
#endif
				}
				break;
			}
		}
	}
	
	if (mEvtRecord.modifiers & shiftKey)
		range.Union(inPreSelectionRange);

	*outPostSelectionRange = range;
}

void	LTextEditHandler::DoTypingKey(void)
{
	StRecalculator	immediateChange(mTextEngine);	//	No flashing and all at once
	TextRangeT		range = mTextSelection->GetSelectionRange(),
					postSelRange = range;
	OSErr			err = noErr;
		
	Try_ {

		DoTypingKeySelf(range, &postSelRange);
		
		EventRecord	event;
		Point		where;
		while (EventAvail(keyDownMask+autoKeyMask, &event)) {

			if (!IsTypingKey(event))
				break;
			
			GetNextEvent(keyDownMask+autoKeyMask, &event);
			UEventUtils::FleshOutEvent(mEvtRecord.modifiers & btnState, &event);
			UpdateLastEvt(event, where);
			where = event.where;
			GlobalToLocal(&where);
			UpdateLastEvt(event, where);
			
			range = postSelRange;
			DoTypingKeySelf(range, &postSelRange);
		}
		
	} Catch_(inErr) {
		err = inErr;
	} EndCatch_;
		
	//	Adjust selection (outside of StRecalculator so engine is fully recalculated)
	mTextSelection->SetSelectionRange(postSelRange);
	ThrowIfOSErr_(err);
}

void	LTextEditHandler::DoNormalChars(void)
{
	StRecalculator	immediateChange(mTextEngine);	//	No flashing -- all at once
	TextRangeT		range = mTextSelection->GetSelectionRange(),
					postSelRange = range;
	OSErr			err = noErr;
	LAETypingAction	*typing = mTextSelection->GetTypingAction();
	ThrowIfNULL_(typing);
	
	if (!mTextEngine->SpaceForBytes(1))
		ThrowOSErr_(teScrapSizeErr);
		
	Try_ {

		EventRecord		event;
		Point			where;
		
		sNormalCharBuffSize = 0;
		sNormalCharBuff[sNormalCharBuffSize++] = mEvtRecord.message & charCodeMask;
		
		while (true) {
			if (!mTextEngine->SpaceForBytes(1))
				break;
			
			if (!EventAvail(keyDownMask+autoKeyMask, &event))
				break;

			if (!IsNormalChar(event))
				break;
			
			GetNextEvent(keyDownMask+autoKeyMask, &event);
			UEventUtils::FleshOutEvent(mEvtRecord.modifiers & btnState, &event);
			UpdateLastEvt(event, where);
			where = event.where;
			GlobalToLocal(&where);
			UpdateLastEvt(event, where);
			
			sNormalCharBuff[sNormalCharBuffSize++] = mEvtRecord.message & charCodeMask;
			
			if (sNormalCharBuffSize == kNormalCharBuffMaxSize)
				break;
		}
		
		if (!sNormalCharBuffSize)
			Throw_(paramErr);

		TextRangeT	newCharRange(range.Start(), range.Start() + sNormalCharBuffSize);
			
		typing->KeyStreamRememberPrepend(range);
		mTextEngine->TextReplaceByPtr(range, sNormalCharBuff, sNormalCharBuffSize);
		ApplyPendingStyle(newCharRange);
		typing->KeyStreamRememberNew(newCharRange);

		postSelRange = newCharRange;
		postSelRange.Back();
		
		Assert_(postSelRange.Start() == newCharRange.End());
		
	} Catch_(inErr) {
		err = inErr;
	} EndCatch_;
	
	//	Adjust selection (outside of StRecalculator so engine is fully recalculated)
	mTextSelection->SetSelectionRange(postSelRange);
	ThrowIfOSErr_(err);
}

void	LTextEditHandler::DoTypingKeySelf(
	const TextRangeT	&inPreSelectionRange,
	TextRangeT			*outPostSelectionRange)
{
	TextRangeT		range = inPreSelectionRange;
	char			key = mEvtRecord.message & charCodeMask;
	LAETypingAction	*typing = mTextSelection->GetTypingAction();
	ThrowIfNULL_(typing);

	//	adjust operand
	switch (key) {

		case char_Backspace:
			if (!range.Length()) {
				range = TextRangeT(range.Start() - mTextEngine->PrevCharSize(range.Start()), range.End());
			}
			break;

		case char_FwdDelete:
			if (!range.Length())
				range.SetLength(mTextEngine->ThisCharSize(range.Start()));
			break;
	}
	range.WeakCrop(mTextModel->GetTextLink()->GetRange());
	*outPostSelectionRange = range;

	//	Do operation
	switch (key) {

		case char_Backspace:
			if (range.Length()) {
				typing->KeyStreamRememberPrepend(range);
				mTextEngine->TextDelete(range);
			}
			break;

		case char_FwdDelete:
			if (range.Length()) {
				typing->KeyStreamRememberAppend(range);
				mTextEngine->TextDelete(range);
			}
			break;

		default:
		{
			TextRangeT	newCharRange(range.Start(), range.Start()+1);
			
			if (!mTextEngine->SpaceForBytes(1 - range.Length()))
				ThrowOSErr_(teScrapSizeErr);

			typing->KeyStreamRememberPrepend(range);
			mTextEngine->TextReplaceByPtr(range, &key, 1);
			ApplyPendingStyle(newCharRange);
			typing->KeyStreamRememberNew(newCharRange);
			
			*outPostSelectionRange = newCharRange;
			outPostSelectionRange->Back();
			break;
		}
	}

	//	adjust selection
	switch (key) {

		case char_Backspace:
		case char_FwdDelete:
			outPostSelectionRange->SetLength(0);
			PurgePendingStyle();
			break;
	}
	
}

Boolean	LTextEditHandler::DoCondition(EventConditionT inCondition)
{
	Boolean	rval = false;

	switch (inCondition) {
		case eva_DMove:
		{
			if (mInBounds) {
				switch (mDragType) {
	
					case dragType_Selecting:
						//	We only need to update mEvtItem.
						//	(Should this be folded back into LEventHandler?)

						if (mInBounds)
							ReplaceSharable(mEvtItem, OverItem(mEvtMouse), this);

						if (mSelection)
							mSelection->SelectContinuous(mEvtItem);
						break;
					
					default:
						rval = inherited::DoCondition(inCondition);
						break;
				}
			}
			CheckBoundaryDrag();
			break;
		}
		
		case eva_DStop:
		{
			if (mDragType == dragType_Selecting && mHasBufferedKeyEvent) {

				//	Process the key
				//
				//	(This is out of order -- it makes more sense to do
				//	it after the drag has completely stopped and after the "note"
				//	has been sent.  But, that would require replicating more code.)
				EventRecord	savedEvent = mEvtRecord;
				mEvtRecord = mBufferedKeyEvent;
				KeyDown();
				mHasBufferedKeyEvent = false;
				mEvtRecord = savedEvent;
			}
			rval = inherited::DoCondition(inCondition);
			break;
		}
		
		case eva_Down:
			mUpDownHorzPixel = -1;
			rval = inherited::DoCondition(inCondition);
			break;
		
		default:
			rval = inherited::DoCondition(inCondition);
			break;
	}
	
	return rval;
}
