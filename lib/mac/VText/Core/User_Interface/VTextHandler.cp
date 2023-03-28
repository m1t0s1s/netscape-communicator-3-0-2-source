//	===========================================================================
//	VTextHandler.cp
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextHandler.h"
#include	<LStyleSet.h>
#include	<LTextSelection.h>
#include	<VTextElemAEOM.h>
#include	<LTextModel.h>
#include	<UMemoryMgr.h>
#include	<LRulerSet.h>
#include	<VTypingAction.h>
#include	<VTextView.h>
#include	<VTSMProxy.h>

#include	<PP_KeyCodes.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance:

VTextHandler::VTextHandler()
{
	mMemoryOrderArrowKeys = false;
	mTSMProxy = NULL;
}

VTextHandler::~VTextHandler()
{
	delete mTSMProxy;
}

void	VTextHandler::SetTSMProxy(VInlineTSMProxy *inProxy)
{
	mTSMProxy = inProxy;
}

void	VTextHandler::Reset(void)
{
	inherited::Reset();
}
	
void	VTextHandler::FollowKeyScript(void)
{
//	keep pending style (script) synchronized with keyboard script
	if (mTextSelection) {
		ScriptCode	keyScript = GetScriptManagerVariable(smKeyScript),
					styleScript = mTextSelection->GetCurrentStyle()->GetScript();
					
		if (styleScript != keyScript) {
			if (mTSMProxy)
				mTSMProxy->FlushInput();
			
			LStyleSet	*set = mTextModel->StyleSetForScript(
				mTextSelection->GetSelectionRange(), keyScript
			);
			ThrowIfNULL_(set);
			mTextSelection->SetPendingStyle(set);
		}
	}
}

void	VTextHandler::FollowStyleScript(void)
{
//	keep keyboard script synchronized with pending style
	if (mTextSelection) {
		ScriptCode	keyScript = GetScriptManagerVariable(smKeyScript),
					styleScript = mTextSelection->GetCurrentStyle()->GetScript();
					
		if (keyScript != styleScript)
			KeyScript(styleScript);

		keyScript = GetScriptManagerVariable(smKeyScript);
		Assert_(keyScript == styleScript);	//	something with the script manager is odd
	}
}

void	VTextHandler::ApplyPendingStyle(const TextRangeT &inRange)
{
	if (mTextSelection && mTextSelection->GetPendingStyle() && mTextEngine && inRange.Length())
		mTextEngine->SetStyleSet(inRange, mTextSelection->GetPendingStyle());
	mTextSelection->SetPendingStyle(NULL);	//	not necessary to update command status
}

void	VTextHandler::PurgePendingStyle(void)
{
	if (mTextSelection)
		mTextSelection->SetPendingStyle(NULL);
}

Boolean	VTextHandler::DoCondition(EventConditionT inCondition)
{
	Boolean	rval = inherited::DoCondition(inCondition);
	
	switch (inCondition) {
		case eva_Up:
		case eva_DStop:
			PurgePendingStyle();
			FollowStyleScript();
			break;
		
		default:
			if (GetEvtState() == evs_Idle) {
				/*	This seems like a real odd time/place to do this but, we receive
					no notification from the OS that the key script has changed "now."
					This code catches the cases when the key script has changed, but no
					good time to check (like key handling) has occured.
				*/
				FollowKeyScript();
			}
			break;
	}
		
	return rval;
}

LAETypingAction *	VTextHandler::NewTypingAction(void)
{
	VTypingAction *rval = new VTypingAction(mTextEngine);

	rval->SetSelection(mSelection, kAEAfter);
	rval->FinishCreateSelf();

	return rval;
}

void	VTextHandler::DoNonTypingKeySelf(
	const TextRangeT	&inPreSelectionRange,
	TextRangeT			*outPostSelectionRange)
{
	if (mMemoryOrderArrowKeys) {
		inherited::DoNonTypingKeySelf(inPreSelectionRange, outPostSelectionRange);
		return;
	}

	char		key = mEvtRecord.message & charCodeMask;

	switch (key) {
	
		case char_LeftArrow:
		case char_RightArrow:
		{
			
			TextRangeT		range = inPreSelectionRange.Undefined()
								? LTextEngine::sTextAll
								: inPreSelectionRange;
			Boolean			collapse = range.Length() && !(mEvtRecord.modifiers & shiftKey);
			Boolean			advance;
			Handle			text = mTextEngine->GetTextHandle();
			StHandleLocker	lock(text);

			Boolean		rulerIsLeftToRight = mTextEngine->GetRulerSet(range)->IsLeftToRight();
			TextRangeT	tRange = range;
			Int32		offset = tRange.Start(),
						size;
			
			//	get char direction
			Boolean		charIsLeftToRight = rulerIsLeftToRight;
			switch (tRange.Insertion()) {

				case rangeCode_Before:
					size = mTextEngine->PrevCharSize(offset);
					offset -= size;
					tRange.SetRange(offset, offset + size);
					break;
					
				case rangeCode_After:
					size = mTextEngine->ThisCharSize(offset);
					tRange.SetRange(offset, offset + size);
					break;
					
				default:
					//	nada
					break;
			}
			if (tRange.Length()) {
				TextRangeT		charPt = tRange;
				charPt.Front();
				TextDirectionE	charDirection = mTextEngine->GetStyleSet(tRange)->CharDirection(tRange, *text, charPt);
				charIsLeftToRight = rulerIsLeftToRight;
				if (charDirection != textDirection_Undefined)
					charIsLeftToRight = (charDirection == textDirection_LeftToRight);
			}

			if (charIsLeftToRight)
				advance = key == char_RightArrow;
			else
				advance = key == char_LeftArrow;

			if (mEvtRecord.modifiers & shiftKey) {
				if (advance) {
					range.SetLength(range.Length() + mTextEngine->ThisCharSize(range.End()));
				} else {
					range.MoveStart(range.Start() - mTextEngine->PrevCharSize(range.Start()));
				}
			} else {

				if (advance)
					range.Back();
				else
					range.Front();

				if (!collapse) {
				
					if (!TryAdvance(key, charIsLeftToRight, range, tRange)) {
						SPoint32	where;
						Boolean		leadingEdge;
						
						if (key == char_RightArrow) {
							mTextEngine->Range2Image(tRange, !charIsLeftToRight, &where);
							where.h += 1;
						} else {
							Assert_(key == char_LeftArrow);

							mTextEngine->Range2Image(tRange, charIsLeftToRight, &where);
							where.h -= 1;
						}

						mTextEngine->Image2Range(where, &leadingEdge, &tRange);
						if (tRange.Length()) {

							if (leadingEdge)
								tRange.Back();
							else
								tRange.Front();

						} else {
							//	we're trying to move past a line end
							//	up or down is determined by line direction and end is
							//	determine by which direction we were headed
							if (key == char_RightArrow) {
								where.h = 0;
								if (rulerIsLeftToRight)
									where.v += 1;
								else
									where.v -= mTextEngine->GetRangeHeight(tRange);
							} else {
								where.h = max_Int16;
								if (rulerIsLeftToRight)
									where.v -= mTextEngine->GetRangeHeight(tRange);
								else
									where.v += 1;
							}
							mTextEngine->Image2Range(where, &leadingEdge, &tRange);
							if (tRange.Length()) {
								if (leadingEdge)
									tRange.Front();
								else
									tRange.Back();
							}
/*-
							TextRangeT	paraRange = tRange;
							mTextEngine->FindPart(LTextEngine::sTextAll, &lineRange, cParagraph);
							if (tRange.Start() > ((lineRange.Start() + lineRange.End()) >> 1)) {

								//	go to succeeding line
								tRange = lineRange;
								tRange.Back();
								tRange.SetLength(tRange.Length() + mTextEngine->ThisCharSize(tRange.End()));
								tRange.Back();
								
							} else {

								//	go to preceeding line
								tRange = lineRange;
								tRange.Front();
								tRange.MoveStart(tRange.Start() - mTextEngine->PrevCharSize(tRange.Start()));
								tRange.Front();
							}
*/
						}
					}
					range = tRange;
					
					mUpDownHorzPixel = -1;

				}
				
			}

			*outPostSelectionRange = range;
			break;
		}
			
		default:
			inherited::DoNonTypingKeySelf(inPreSelectionRange, outPostSelectionRange);
			break;
	}
}

Boolean	VTextHandler::TryAdvance(
	char				inKey,
	Boolean				inCharIsLeftToRight,
	const TextRangeT	&inInsertion,
	TextRangeT			&ioRange
)
{
	Boolean		rval = true;

	if (inCharIsLeftToRight && (inInsertion.Insertion() == rangeCode_After) && (inKey == char_RightArrow)) {
		//	right
		ioRange.Back();
	} else if (inCharIsLeftToRight && (inInsertion.Insertion() == rangeCode_Before) && (inKey == char_LeftArrow)) {
		//	left
		ioRange.Front();
	} else if (!inCharIsLeftToRight && (inInsertion.Insertion() == rangeCode_After) && (inKey == char_LeftArrow)) {
		//	left
		ioRange.Back();
	} else if (!inCharIsLeftToRight && (inInsertion.Insertion() == rangeCode_Before) && (inKey == char_RightArrow)) {
		//	right
		ioRange.Front();
	} else {
		rval = false;
	}
	
	return rval;
}

/*-
Boolean	VTextHandler::DoLiteralEvent(const EventRecord &inEvent)
{
	Boolean		handled = false;
	
	if (mTSMProxy)
		handled = mTSMProxy->DoEvent(inEvent);
	
	if (!handled)
		handled = inherited::DoLiteralEvent(inEvent);
	
	return handled;
}	

void	VTextHandler::Activate(void)
{
	inherited::Activate();

	if (mTSMProxy)
		mTSMProxy->Activate();
}	

void	VTextHandler::Deactivate(void)
{
	if (mTSMProxy)
		mTSMProxy->Deactivate();
		
	inherited::Deactivate();
}	
*/
