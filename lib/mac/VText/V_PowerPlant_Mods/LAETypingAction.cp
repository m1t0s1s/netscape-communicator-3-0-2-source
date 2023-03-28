// ===========================================================================
//	LAETypingAction.cp			й1995 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LAETypingAction.h"
#include	<UAppleEventsMgr.h>
#include	<LTextEngine.h>
#include	<LTextSelection.h>
#include	<LTextModel.h>
#include	<LTextElemAEOM.h>
#include	<UMemoryMgr.h>
#include	<PP_Resources.h>
#include	<LDynamicArray.h>
#include	<PP_KeyCodes.h>

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#pragma	warn_unusedarg off

LAETypingAction::LAETypingAction()
{
	Assert_(false);		//	Parameters required
}

LAETypingAction::LAETypingAction(
	LTextEngine	*inTextEngine
)
:	LAEAction(STRx_RedoEdit, str_Typing)
{
	mTextEngine = inTextEngine;
	ThrowIfNULL_(mTextEngine);

	mAccumulator = mTextEngine;
	mTextSelection = NULL;
	mFinalized = false;
	mLastRange = LTextEngine::sTextUndefined;

	mOldRange = LTextEngine::sTextUndefined;
	mOldRangeOrig = LTextEngine::sTextUndefined;

	SetPreRecordSelection(false);
	SetRecordOnlyFinalState(true);
}

LAETypingAction::~LAETypingAction()
{
}

void	LAETypingAction::SetSelection(
	LSelection	*inSelection,
	DescType	inRedoSelectionModifier,
	DescType	inUndoSelectionModifier)
{
	inherited::SetSelection(inSelection, inRedoSelectionModifier, inUndoSelectionModifier);
	
	if (mSelection)
//	if (mSelection && member(mSelection, LTextSelection))
		mTextSelection = (LTextSelection *) mSelection;
	else
		mTextSelection = NULL;
}

void	LAETypingAction::FinishCreateSelf(void)
{
	ThrowIfNULL_(mTextEngine);
	ThrowIfNULL_(mTextSelection);

	mTextSelection->SetTypingAction(this);
	
	if (mTextSelection->ListCount() != 1)
		Throw_(paramErr);
	
	mOldRangeOrig = mTextSelection->GetSelectionRange();
	mOldTotalRange = mTextEngine->GetTotalRange();
	LModelObject	*elem = mTextSelection->ListNthItem(1);
	elem->MakeSpecifier(mOldOSpec.mDesc);
}

void	LAETypingAction::RedoSelf(void)
{
	if (mFinalized) {
		inherited::RedoSelf();
	} else {
		if (!mIsDone) {
			
			//	Redo typing
			mTextEngine->TextReplaceByPtr(mOldRange,
							(Ptr) mNewText.GetOffsetPtr(0), mNewText.GetSize());
			mIsDone = true;
		
			//	Update selection
			TextRangeT	range(mOldRange.Start() + mNewText.GetSize());
			mTextSelection->SetSelectionRange(range);
		}
	}
}

void	LAETypingAction::UndoSelf(void)
{
	if (mFinalized) {
		inherited::UndoSelf();
	} else {
		if (mIsDone) {
		
			//	remember new text ospec now for use later...
			TextRangeT	newTextRange(mOldRange);
			newTextRange.SetLength(mNewText.GetSize());
			LModelObject	*elem = mTextSelection->GetTextModel()->MakeNewElem(newTextRange, cChar);
			AEDisposeDesc(&mNewOSpec.mDesc);
			elem->MakeSpecifier(mNewOSpec.mDesc);
			
			//	Undo typing
			mTextEngine->TextReplaceByPtr(newTextRange,
							(Ptr) mOldText.GetOffsetPtr(0), mOldText.GetSize());
			mIsDone = false;
			
			//	Update selection
			mTextSelection->SetSelectionRange(mOldRangeOrig);
		}
	}
}

Boolean	LAETypingAction::CanRedo(void) const
{
	return !mIsDone && TypingBuffered();
}

Boolean	LAETypingAction::CanUndo(void) const
{
	return mIsDone && TypingBuffered();
}

Int32	LAETypingAction::TypingBuffered(void) const
{
	return mNewText.GetSize() || mOldText.GetSize();
}

Boolean	LAETypingAction::NewTypingLocation(void)	
{
	return	mFinalized ||
			!mIsDone ||
			mLastRange != mTextSelection->GetSelectionRange();
}

void	LAETypingAction::UpdateTypingLocation(void)
{
	mLastRange = mTextSelection->GetSelectionRange();
}

void	LAETypingAction::PrimeFirstRemember(const TextRangeT &inRange)
{
	TextRangeT		superRange = mTextSelection->GetTextModel()->GetTextLink()->GetRange();

	Assert_(mOldRange == LTextEngine::sTextUndefined);
	Assert_(mOldText.GetSize() == 0);
	
	mOldRange = inRange;
	mOldRange.WeakCrop(superRange);
	
	//	Find mOldContIdx and mOldCharIdx
	{
		DescType	partType;
		Int32		oldTextIndex1,
					oldTextIndex2;

		mOldContType = cParagraph;			
		mOldContRange = mOldRange;
		partType = mTextEngine->FindPart(superRange, &mOldContRange, mOldContType);
		if (partType == typeNull)
			mOldContRange = superRange;
		
		//	container range
		oldTextIndex1 = oldTextIndex2 = 1;
		partType = mTextEngine->FindSubRangePartIndices(
									superRange, mOldContRange, mOldContType,
									&oldTextIndex1, &oldTextIndex2);
		mOldContIdx.SetStart(oldTextIndex1);
		if (oldTextIndex2 > oldTextIndex1) {
			mOldContIdx.SetLength(oldTextIndex2 - oldTextIndex1 + 1);
		} else {
			if (mOldRange.Length()) {
				mOldContIdx.SetLength(1);
			} else {
				mOldContIdx.SetInsertion(mOldRange.Insertion());
			}
		}

		//	characters being replaced
		oldTextIndex1 = oldTextIndex2 = 1;
		partType = mTextEngine->FindSubRangePartIndices(
									mOldContRange, mOldRange, cChar,
									&oldTextIndex1, &oldTextIndex2);
		mOldCharIdx.SetStart(oldTextIndex1);
		if (oldTextIndex2 > oldTextIndex1) {
			mOldCharIdx.SetLength(oldTextIndex2 - oldTextIndex1 + 1);
		} else {
			if (mOldRange.Length())
				mOldCharIdx.SetLength(1);
			else
				mOldCharIdx.SetInsertion(mOldRange.Insertion());
		}
	}
		
	
	if (!(mOldRange == mOldRangeOrig))
		AEDisposeDesc(&mOldOSpec.mDesc);
}

void	LAETypingAction::KeyStreamRememberNew(const TextRangeT &inRange)
/*
	Remember newly typed characters as indicated by inRange.
*/
{
	TextRangeT	range = inRange;
		
	Assert_(!mFinalized);
	
	if (range.Length()) {
		Ptr	destination = (Ptr) mNewText.Append(NULL, range.Length());
		ThrowIfNULL_(destination);
		mTextEngine->TextGetThruPtr(range, (Ptr)destination);
	}
}

void	LAETypingAction::KeyStreamRememberAppend(const TextRangeT &inRange)
/*
	Append text to mOldText (initial selection removal or forward delete)
	
	InRange indicates text to remember.  Text is appended to mOldText.
	
	NOTE:	This routine requires modification for typing undo for styled
			text.  mOldText must be augmented by a mechanism to
			contain style information.
*/
{
	TextRangeT	range = inRange;
	
	Assert_(!mFinalized);		

	if (mOldRange == LTextEngine::sTextUndefined) {

		PrimeFirstRemember(range);

	} else {

		if (range.Length()) {
		
			Int32		rangeCharCount = mTextEngine->CountParts(range, cChar);
			DescType	partType;
			
			AEDisposeDesc(&mOldOSpec.mDesc);
			
			if (mOldRange.End() == mOldContRange.End()) {
			
				//	update container range
				TextRangeT	extensionRange = range;
				partType = mTextEngine->FindPart(
								mTextSelection->GetTextModel()->GetTextLink()->GetRange(),
								&extensionRange, mOldContType);
				if (partType == typeNull)
					Throw_(paramErr);
					
				//	The container found may have been the result of a join so be sure only the
				//	new part of the extension is included.
				extensionRange.MoveStart(inRange.Start());
				
				Int32		extensionParts = mTextEngine->CountParts(extensionRange, mOldContType);
				if (!extensionParts)
					Throw_(paramErr);
					
				mOldContRange.SetLength(mOldContRange.Length() + extensionRange.Length());
				mOldContIdx.SetLength(mOldContIdx.Length() + extensionParts);
			}

			//	update old range
			mOldRange.SetLength(mOldRange.Length() + range.Length());
			mOldCharIdx.SetLength(mOldCharIdx.Length() + rangeCharCount);
Assert_(mOldCharIdx.Length());
		}
	}

	if (range.Length()) {
		Ptr		destination = (Ptr) mOldText.Append(NULL, range.Length());;
		ThrowIfNULL_(destination);
		mTextEngine->TextGetThruPtr(range, destination);
	}
}

void	LAETypingAction::KeyStreamRememberPrepend(const TextRangeT &inRange)
/*
	Prepend text to mOldText (initial selection removal or backspace)
	
	InRange indicates text to remember.  Text is prepended to mOldText.
	
	Before any text is actually prepended to mOldText, characters will first
	be removed and forgotten from mNewText.  Only when mNewText is empty will
	characters be prepended.
	
	NOTE:	This routine requires modification for typing undo for styled
			text.  mOldText must be augmented by a mechanism to
			contain style information.
*/
{
	TextRangeT	range = inRange;
	
	Assert_(!mFinalized);

	if (mOldRange == LTextEngine::sTextUndefined) {

		PrimeFirstRemember(range);

	} else {
		
		//	First forget newly typed characters
		if (mNewText.GetSize()) {
			Int32	n = URange32::Min(range.Length(), mNewText.GetSize());
			mNewText.Delete(max_Int32, n);
			range.SetLength(range.Length() - n);
		}
		
		//	Then remember old characters
		if (range.Length()) {
		
			if (range.End() != mOldRange.Start())
				Throw_(paramErr);
			
			Int32		rangeCharCount = mTextEngine->CountParts(range, cChar);
			DescType	partType;
			
			AEDisposeDesc(&mOldOSpec.mDesc);
				
			if (range.Start() < mOldContRange.Start()) {
			
				//	update container range
				TextRangeT	extensionRange = range;
				partType = mTextEngine->FindPart(
								mTextSelection->GetTextModel()->GetTextLink()->GetRange(),
								&extensionRange, mOldContType);
				if ((partType == typeNull) || (extensionRange.End() != mOldContRange.Start()))
					Throw_(paramErr);

				Int32		extensionChars = mTextEngine->CountParts(extensionRange, cChar),
							extensionParts = mTextEngine->CountParts(extensionRange, mOldContType);
				if (!extensionChars || !extensionParts)
					Throw_(paramErr);
					
				mOldContRange.MoveStart(extensionRange.Start());
				mOldContIdx.MoveStart(mOldContIdx.Start() - extensionParts);
				
				//	and fix char idx
				mOldCharIdx.SetStart(mOldCharIdx.Start() + extensionChars);
			}
			
			//	update old range
			mOldRange.MoveStart(range.Start());
			mOldCharIdx.MoveStart(mOldCharIdx.Start() - rangeCharCount);		
Assert_(mOldCharIdx.Length());
		}
	}

	if (range.Length()) {
		Ptr		destination = (Ptr) mOldText.Prepend(NULL, range.Length());
		ThrowIfNULL_(destination);
		mTextEngine->TextGetThruPtr(range, destination);
	}
}

void	LAETypingAction::Finalize(void)
{
	if (!mFinalized) {

		if (mTextSelection) {
			if (mTextSelection->GetTypingAction() == this)
				mTextSelection->SetTypingAction(NULL);
		}

		mFinalized = true;
		
		{
			AEDisposeDesc(&mOriginalSelection.mDesc);
			LAEStream	ae;
			
			WriteOldTarget(ae);
			ae.Close(&mOriginalSelection.mDesc);
			
			SetPreRecordSelection(true);
		}
		
		//	ее	Doer
		if (mNewText.GetSize()) {
		
			//	е	Build a CreateElement event
			LAEStream	ae(kAECoreSuite, kAECreateElement);
			
				//	keyAEObjectClass
				ae.WriteKey(keyAEObjectClass);
				ae.WriteTypeDesc(cChar);
			
				//	keyAEInsertHere
				ae.WriteKey(keyAEInsertHere);
				ae.WriteSpecifier(mSelection);
			
				//	keyAEData
				ae.WriteKey(keyAEData);
				WriteTypingData(ae);
				
			ae.Close(&mRedoEvent.mDesc);

		} else {
			
			//	е	Build Delete event
			LAEStream	ae(kAECoreSuite, kAEDelete);
				ae.WriteKey(keyDirectObject);
				ae.WriteSpecifier(mSelection);
			ae.Close(&mRedoEvent.mDesc);
		}
			
		//	ее	Undoer
		if (mIsDone) {
			TextRangeT		newTextRange(mOldRange.Start());
			newTextRange.SetLength(mNewText.GetSize());
			LModelObject	*elem = mTextSelection->GetTextModel()->MakeNewElem(newTextRange, cChar);
			AEDisposeDesc(&mNewOSpec.mDesc);
			elem->MakeSpecifier(mNewOSpec.mDesc);
		} else {
			Assert_(mNewOSpec.mDesc.descriptorType != typeNull);
		}

		if (mOldText.GetSize()) {
		
			//	е	Build a CreateElement event
			LAEStream	ae(kAECoreSuite, kAECreateElement);
							
				//	keyAEObjectClass
				ae.WriteKey(keyAEObjectClass);
				ae.WriteTypeDesc(cChar);

				//	keyAEData
				ae.WriteKey(keyAEData);
				{
					Size			size = mOldText.GetSize();
					StHandleLocker	lock(mOldText.mData);
					ae.WriteDesc(typeChar, *(mOldText.mData), size);
				}
				
				//	keyAEInsertHere
				ae.WriteKey(keyAEInsertHere);
				ae.WriteDesc(mNewOSpec);
		
			ae.Close(&mUndoEvent.mDesc);

		} else {
			
			//	е	Build Delete event
			LAEStream	ae(kAECoreSuite, kAEDelete);
				ae.WriteKey(keyDirectObject);
				ae.WriteDesc(mNewOSpec);
			ae.Close(&mUndoEvent.mDesc);
		}
	}
	inherited::Finalize();
}

void	LAETypingAction::WriteTypingData(LAEStream &inStream) const
{
	Size			size = mNewText.GetSize();
	StHandleLocker	lock(mNewText.mData);
	inStream.WriteDesc(typeChar, *(mNewText.mData), size);
}

void	LAETypingAction::WriteOldTarget(LAEStream &inStream) const
/*
	Write ospec for text to be replaced by typing to inStream.
	
	Handles whether the
		target can simply be the selection via mInsertTextHere, or
		must fabricate a special character based range ospec.
*/
{
	if (mOldOSpec.mDesc.descriptorType != typeNull) {

		inStream.WriteDesc(mOldOSpec.mDesc);

	} else {

		if (mOldCharIdx.Length() <= 0)
			Throw_(paramErr);	//	should never happen!
								//	mOldSpec should still be valid above!

		inStream.OpenRecord(typeObjectSpecifier);

			inStream.WriteKey(keyAEDesiredClass);
			inStream.WriteTypeDesc(cChar);
			
			inStream.WriteKey(keyAEContainer);
			inStream.OpenRecord(typeObjectSpecifier);
				inStream.WriteKey(keyAEDesiredClass);
				inStream.WriteTypeDesc(mOldContType);
				inStream.WriteKey(keyAEContainer);
				inStream.WriteSpecifier(mTextSelection->GetTextModel());
				WriteRangeIndexKey(	inStream,
									mOldContIdx.Start(), mOldContIdx.End() -1,
									mOldContType, mOldContType
				);
			inStream.CloseRecord();
			WriteRangeIndexKey(	inStream,
								mOldCharIdx.Start(), mOldCharIdx.End() -1,
								cChar, cChar
			);

		inStream.CloseRecord();
	}
}

void	LAETypingAction::WriteRangeIndexKey(
	LAEStream	&inStream,
	Int32 		inIndex1,
	Int32		inIndex2,
	DescType	inType1,
	DescType	inType2) const
{
	if (inIndex2 == inIndex1) {

		inStream.WriteKey(keyAEKeyForm);
		inStream.WriteEnumDesc(formAbsolutePosition);

		inStream.WriteKey(keyAEKeyData);
		inStream.WriteDesc(typeLongInteger, &inIndex1, sizeof(inIndex1));

	} else if (inIndex2 > inIndex1) {

		AEDesc	currentContainer = {typeCurrentContainer, NULL};

		inStream.WriteKey(keyAEKeyForm);
		inStream.WriteEnumDesc(formRange);

		inStream.WriteKey(keyAEKeyData);
		inStream.OpenRecord(typeRangeDescriptor);

			inStream.WriteKey(keyAERangeStart);
			inStream.OpenRecord(typeObjectSpecifier);
				inStream.WriteKey(keyAEDesiredClass);
				inStream.WriteTypeDesc(inType1);
				inStream.WriteKey(keyAEContainer);
				inStream.WriteDesc(currentContainer);
				inStream.WriteKey(keyAEKeyForm);
				inStream.WriteEnumDesc(formAbsolutePosition);
				inStream.WriteKey(keyAEKeyData);
				inStream.WriteDesc(typeLongInteger, &inIndex1, sizeof(inIndex1));
			inStream.CloseRecord();
			
			inStream.WriteKey(keyAERangeStop);
			inStream.OpenRecord(typeObjectSpecifier);
				inStream.WriteKey(keyAEDesiredClass);
				inStream.WriteTypeDesc(inType2);
				inStream.WriteKey(keyAEContainer);
				inStream.WriteDesc(currentContainer);
				inStream.WriteKey(keyAEKeyForm);
				inStream.WriteEnumDesc(formAbsolutePosition);
				inStream.WriteKey(keyAEKeyData);
				inStream.WriteDesc(typeLongInteger, &inIndex2, sizeof(inIndex2));
			inStream.CloseRecord();
		inStream.CloseRecord();
	} else {
		WriteRangeIndexKey(inStream, inIndex2, inIndex1, inType2, inType1);
	}
}